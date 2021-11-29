//
// Created by devbox on 11/22/21.
//
#include <sys/socket.h>
#include <sys/poll.h>
#include <sstream>
#include <cstring>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <X11/X.h>

#include "Agent.h"
#include "Interface.h"
#include "Logger.h"

Agent::Agent(Interface * _to_client, Interface * _to_xserver, int _seed) {
    to_client= _to_client;
    to_xserver = _to_xserver;
    clock_gettime(CLOCK_MONOTONIC, &last_injection);
}

Agent::~Agent(){}

int Agent::converse(){
    struct pollfd fds[2];
    int    nfds = 2;
    int fd_available;
    int count = 0;
    int disp_movement = 1000000;
    memset(fds, 0, sizeof(fds));
    fds[CLIENT].fd = this->to_client->getFD();
    fds[CLIENT].events = POLLIN;
    fds[XSERVER].fd = this->to_xserver->getFD();
    fds[XSERVER].events = POLLIN;

    while(true){
//        if (count++%disp_movement == 0) {logger(".");}
//        if (count%(disp_movement*10) == 0) {
//            logger("\n");
//            //verify_empty(fds, nfds);
//        }
        // Check if there is a message, and deal with potential errors
        fd_available = poll(fds, nfds, 0);
        switch (fd_available) {
        case -1:
            slog << "Poll returned error number " << errno << endl;
            logger(slog.str(), ERR);
            log_error("Poll returned msg: ");
            return -1;
            break;
        case 0:
            // No messages, might be good point to insert
            // Injects messages at set intervals.
            if (this->inj_mode > 0 && this->msg_count > this->startgap){
                clock_gettime(CLOCK_MONOTONIC, &this->current_time);
                elapsedTime = ((this->current_time.tv_nsec - this->last_injection.tv_nsec)/1000000)
                        + (this->current_time.tv_sec - this->last_injection.tv_sec) * 1000;
                if (elapsedTime >= this->inj_rate){
                    clock_gettime(CLOCK_MONOTONIC, &this->last_injection);
                    inject_message(this->to_client);
                }
            }
            break;
        default:
            int mreturn = 0;
            for (int i = nfds-1; i >=0; i--) { // backwards to check server first
                if (fds[i].revents != 0) {  // Did we find something?
                    if (fds[i].revents != POLLIN) {
                        //Error occured
                        slog << "ERROR: revents event returned " << fds[i].revents;
                        switch (fds[i].revents){
                            case POLLERR:
                                slog << " POLLERR (read end of " << (i==XSERVER ? "xserver" : "client") << " pipe has closed) " << endl;
                                break;
                            case POLLHUP:
                                slog << " POLLHUP (hang up - peer closed cxn)" << endl;
                                break;
                            case POLLNVAL:
                                slog << " POLLNVAL (fd not open)" << endl;
                                break;
                        }
                        logger(slog.str());
                        return -4;
                    } else { // found a message
                        if (i == XSERVER) {
                            // logger("Server sent a message\n");
                            mreturn = transfer_msg(this->to_xserver, this->to_client);
                            if (mreturn < 0) { return mreturn; }
                        } else if (i == CLIENT) {
                            // logger("Client sent a message\n");
                            mreturn = transfer_msg(this->to_client, this->to_xserver);
                            if (mreturn < 0) { return mreturn; }
                        }
                    }
                }
            }
        }
    }
    return 1;
}

int Agent::transfer_msg(Interface * source, Interface * dest){
    int recv_length, send_length;

    // Receive message from source
    recv_length = recv(source->getFD(), this->message[this->current_msg], BUFFER_SIZE, 0);
    this->msg_count++; // track total number of messages received
    slog << "(" << this->msg_count << ") received a message from " << source->getName() << " of msg of size " << recv_length << endl;
    logger(slog.str());
    dump_msg(source, this->message[this->current_msg]);

    if (recv_length < 0){
        logger("ERROR: Receive from source failed\n");
        return -2;
    }
    else if (recv_length == 0){
        slog << "ERROR: " << source->getName() << " socket gracefully failed" << endl;
        logger(slog.str(), ERR);
        return -3;
    } else { // Get ready to send message

        // set sequence number for items coming from server
        if (source->getType() == XSERVER && this->message[this->current_msg][0] != KeymapNotify) {
            unsigned short temp_seq = this->seq_num;
            memcpy(&this->seq_num, this->message[this->current_msg] + 2, sizeof(unsigned short));
            if (this->seq_num == 0) this->seq_num = temp_seq;
        }

        // modify message (alter_msg checks for intervals and only alters
        // when we hit interval mark).
        if (this->mod_mode>0){
            this->alter_msg(dest, recv_length, this->message[this->current_msg]);
            if (this->garbled) {dump_msg(source, this->message[this->current_msg]);}
        }

        // Send message to client
        send_length = send(dest->getFD(), this->message[this->current_msg], recv_length, 0);
//        slog << "      Sending a message to " << dest->getName() << " of msg of size " << send_length << endl;
//        logger(slog.str());
        if (send_length < recv_length){
            logger("ERROR: Entire message not sent\n");
        }
        // We keep X msgs in buffer at any point, incr allows us to track which we are on
        // This allows us to inject replay messages.  It also tracks length of stored msgs
        this->incr_msg(dest, send_length);
    }
    return 0;
}

/***
 * We want buffer to contain only events to client
 * when we replay we replay.  Update current only when we
 * add an event to the buffer, and specifically size 32.  Update length always.
 */
void Agent::incr_msg(Interface * dest, int msg_length){
    this->message_length[this->current_msg] = msg_length;
    if(dest->getType() == CLIENT
        && this->message[this->current_msg][0] >=2
        && this->message[this->current_msg][0] <= 34
        && msg_length == EVENT_MESSAGE_SIZE){
        // increment number of items buffered
        if (this->have_buffered < TRACKED_MSGS) {have_buffered++;}
        // Update message index
        current_msg = (++this->current_msg)%TRACKED_MSGS;
    }
}

/***
 * We know that errors have opcode 0,
 *  replies have opcode 1, and events have opcode 2-34.  We also know that
 *  events and errors have lengths of 32 bytes each.  Replies have a
 *  length field which allows for a length of more than 32 bytes.  Each of
 *  these messages except the KeymapNotify event has a sequence number.
 *  We record this sequence number so that we can put it in our own random
 *  events.  We wait for a message boundary (message boundaries can be
 *  detected because we know the message formats and lengths).  We then decide
 *  whether to insert a random event.
 */
void Agent::dump_msg(Interface * source, char * msg, int show_seq) {
    unsigned short temp_seq_num = 0;
    int opcode = msg[0];
    slog << "      OP Code: " << opcode  << " [index:" << this->current_msg << "]" << endl;
    logger(slog.str(), ERR);
    if (source->getType()== XSERVER || show_seq==1 ){
        memcpy(&temp_seq_num, msg + 2, sizeof(unsigned short));
        slog << "      Seq #" << temp_seq_num << " (" << this->seq_num << ")" << endl;
        logger(slog.str(), ERR);
    }
}


/**
 * Calls message modification functions
 * @param dest
 * @param recv_length
 * @param msg
 */
void Agent::alter_msg(Interface * dest, int &recv_length, char * msg) {
    //Only garble messages to client for now
    this->garbled = 0;
    if (dest->getType() != CLIENT) {return;}

    // Past startgap and at rate, lets garble
    if (this->msg_count > this->startgap && this->msg_count%mod_rate == 0){
        switch(this->mod_mode){
            case 1: // garbling
                this->garbler(recv_length, msg);
                break;
            case 2:
                this->kill_length((recv_length));
                break;
            case 3:
                this->kill_seq(recv_length, msg);
                break;
        }
    }
}

/**
 * Randomlay adjusts portion of message (but leves sequence alone)
 * @param recv_length
 * @param msg
 */
void Agent::garbler(int &recv_length, char * msg)
{
    time_t t;
    srand((unsigned) time(&t) + this->msg_count);
    logger("      -->Garbling Message\n");
    // Change around 8 bytes.

    for (int i = 0; i < 3 + rand() % 4 - 2; ++i){
        int m_rand = rand() % recv_length;
        // If you change seq number XCB shuts down cxn
        if (m_rand>= 1 && m_rand <= 3) {continue;}
        msg[m_rand] = (char) (rand() % 256);
        slog << "         garbler msg[" << m_rand << "]" << endl;
        logger(slog.str(), ERR);
    }
    this->garbled = 1;
}

/**
 * Returns message size smaller than that sent
 * @param recv_length
 */
void Agent::kill_length(int &recv_length)
{
    time_t t;
    srand((unsigned) time(&t) + this->msg_count);

    /*
     *  Update: Below causes xcb to shut down connection as well
     *  Seems to not like lengths < actual message size.
    */
    slog << "     -->kill_length recv: " << recv_length;
    recv_length += rand() % 4 - 5;

    if (recv_length < 0)
        recv_length = 0;
    else if (recv_length > BUFFER_SIZE)
        recv_length = BUFFER_SIZE;

    slog << ", sent: " << recv_length << endl;
    logger(slog.str(), ERR);
    this->garbled = 1;

}

/**
 * Kills sequence number located at bytes 2,3
 */
void Agent::kill_seq(int &recv_length, char * msg)
{
    if (recv_length < 5) return;
    /*
     * Killing sequence caused xcb to crash
    */
    time_t t;
    srand((unsigned) time(&t) + this->msg_count);

    logger("      -->Kill Sequence\n");
    for (int i = 2; i < 4; ++i){
        msg[i] = (char) (rand() % 256);
    }
    this->garbled = 1;
}

/**
 * Injects messages into stream when no messages are being
 * sent.  Injections are based on mode.
 *
 * @param dest: Where we are sending the messages to.  Currently
 * only set up to send to client.  Can send messages and events
 *
 * Current generators
 *  random message
 *  legal KM event
 *  legal event
 *
 * note: Only want to inject either every x messages or at every
 * y intervals.  Requiring injections to occur at the same interval as
 * garbling means you can only crash programs you interact with.  By
 * using an interval we can consistently inject messages without interaction.
 * However, legal copied events would still require at least some towards client
 * interaction to build up a message base.  Ideally injected messages would
 * be garbled as well.
 */
void Agent::inject_message(Interface * dest){
    char msg[MESSAGE_SIZE];
    int send_length = 0;
    slog << "(" << this->msg_count << ") #### Injecting a message\n";


    int switch_mode = this->inj_mode;
    if (switch_mode == 5) {switch_mode=rand()%2 + 3;}
    switch(switch_mode){
        case 1: // inject randomly generated messages
            send_length = generate_noise(msg);
            break;
        case 2: // inject random events
        case 3: // inject random KM events
            slog << "      --> Generate Events\n";
            send_length = generate_events(msg);
            break;
        case 4: // replay legal events
            slog << "      --> Replay\n";
            send_length = replay_events(msg);
            //logger(slog.str());
            // alter_msg(dest, send_length, msg); // Thought I could garble replay
            // Above occasionally caused stack smashing errors unclear why.
            break;
        case 5:
            break;
        default:
            return;
    }


    // Need to send injected messages
    if (send_length > 0){
        logger(slog.str());
        this->msg_count++;
        send_length = send(dest->getFD(), msg, send_length, 0);
        slog << "      --> Sent injected Message to " << dest->getName() << " of msg of size " << send_length << endl;
        logger(slog.str());
        dump_msg(dest, msg,  1);
        logger("        DUMP complete\n");
    } else {
        logger("      --> injection aborted\n");
    }
}

/***
 *  send_random_message
 *
 *  Calls the supplied random message generator to generate a message.
 *  If no message generator is supplied, this function will generate a
 *  completely random message of random length.  It will then send the message
 *  to the dest_socket.
 */

/***
*  Generate completely random message of random length.
*/
int Agent::generate_noise(char * buffer)
{
    int msg_len;
    time_t t;
    srand((unsigned) time(&t) + this->msg_count);

    msg_len = (int) (rand() % MESSAGE_SIZE);
    for (int i = 0; i < msg_len; ++i)
        buffer[i] = (char) (rand() % 256);

    return msg_len;
}

/*
 *  $Id: insert.c,v 1.4 1995/05/01 04:54:34 ajitk Exp $
 *
 *  generate_events()
 *  This function inserts a random event into the supplied buffer.
 *  The correct sequence number is set.  Further, insertion is 
 *  itself performed randomly.  If insertion is done, the number of bytes
 *  inserted is returned, else 0.
 *
 *  All events are 32 bytes long.  The first byte is a event code which
 *  is 2-34. The bytes 3 and 4 are the sequence number.
 *  The next four bytes are the timestamp which is the integer 
 *  returned by the time() system call.
 *  All the other fields are set to random values.
 *   
 *  [OPCODE][KEY\BUTTON CODE][SEQ][SEQ]
 *  [TS][TS][TS][TS]
 *  [WIN][WIN][WIN][WIN]
 *  [WIN][WIN][WIN][WIN]
 *  [WIN][WIN][WIN][WIN]
 *  next 12 are random
 */

int Agent::generate_events(char * b){
    time_t timeval, t;
    srand((unsigned) time(&t) + this->msg_count);
    
    /* byte 0 is opcode. */
    if (this->inj_mode == 3) // keyboard & mouse events have opcodes in range 2-5
        b[0] = (char) (rand() % 4 + 2);
    else // legal events have opcodes 2-33
        b[0] = (char) (rand() % 33 + 2);

    /*
     *  byte 1 depends on what b[0] is.
     *  if b[0] == 2 or 3 (keyboard) -->b[1] = KEYCODE (8-255)
     *  else -->b[1] = BUTTON (1-3)
     */
    if (b[0] == 2  ||  b[0] == 3)
        b[1] = (char)(rand() % 248 + 8);
    else
        b[1] = (char)(rand() % 3 + 1);

    /* bytes 2 and 3 contain the last 16 bits of the seq. number */
    logger(slog.str());
    memcpy(&b[2], &this->seq_num, sizeof(unsigned short));

    /* The next four bytes contain the timestamp */
    timeval = time(0);
    memcpy((void *)&b[4], (int *)&timeval, sizeof(int));

    /* There are three window fields */
    for (int i = 0; i < 3; i++)
    {
        int win;
        win = random() % 10;
        memcpy(&b[8 + i * 4], &win, sizeof(int));
    }
    
    /* The rest of the buffer are all arbitrary characters */
    for (int i = 20; i < EVENT_MESSAGE_SIZE; i++)
        b[i] = (char) (rand() % 256);

    return EVENT_MESSAGE_SIZE;
}

/**
 * Choose random message from buffer (ignores current because current could
 * contain message to server not to client.
 * @param buffer
 * @return
 */
int Agent::replay_events(char * buffer){

    if (this->have_buffered==0) {
        logger(".....no buffered events\n");
        return 0;
    }
    time_t t;
    srand((unsigned) time(&t) + this->msg_count);

    int index;
    while((index = rand()%this->have_buffered) == this->current_msg);

    slog << "      --> index = " << index << endl;
    memcpy(buffer, this->message[index], *this->message_length);
    memcpy(&buffer[2], &this->seq_num, sizeof(unsigned short));
//    buffer = this->message[index];
    return this->message_length[index];
}




