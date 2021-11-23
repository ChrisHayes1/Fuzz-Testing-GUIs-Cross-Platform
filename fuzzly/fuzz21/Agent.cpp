//
// Created by devbox on 11/22/21.
//
#include <sys/socket.h>
#include <sys/poll.h>
#include <sstream>
#include <cstring>
#include <stdio.h>
#include <errno.h>

#include "Agent.h"
#include "Interface.h"
#include "Logger.h"



Agent::Agent(Interface * _to_client, Interface * _to_xserver)
    : to_client(_to_client), to_xserver(_to_xserver){

}

Agent::~Agent(){

}


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
                fprintf(stderr, "Error %i on select()\n", errno);
                //terminate();
                return -1;
                break;
            case 0:
                // No messages, might be good point to insert
                // logger("Pull returned 0\n");
                break;
            default:
                int mreturn = 0;
                for (int i = nfds-1; i >=0; i--) { // backwards to check server first
                    if (fds[i].revents != 0) {  // Did we find something?
                        if (fds[i].revents != POLLIN) {
                            //Error occured
                            slog << "ERROR: poll event returned " << fds[i].revents << endl;
                            logger(slog.str());
                            return -1;
                        } else { // found a message
                            if (i == XSERVER) {
                                logger("Server sent a message\n");
                                mreturn = transfer_msg(this->to_xserver, this->to_client);
                                if (mreturn < 0) { return mreturn; }
                            } else if (i == CLIENT) {
                                logger("Client sent a message\n");
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
//    recv_length = source->recv_msg();
    recv_length = recv(source->getFD(), this->message, BUFFER_SIZE, 0);
    slog << "   We received a message from " << source->getName() << " of msg of size " << recv_length << endl;
    dump_msg(source);
    logger(slog.str());

    if (recv_length < 0){
        logger("ERROR: Receive from source failed\n");
        return -2;
    }
    else if (recv_length == 0){
        logger("NOTE: Source socket gracefully failed\n", ERR);
        return -1;
    } else {
        // send_length = dest->send_msg(source->getMessage(), recv_length);
        //this->garble_msg();
        send_length = send(dest->getFD(), this->message, recv_length, 0);
        slog << "   We are sending a message to " << dest->getName() << " of msg of size " << send_length << endl;
        logger(slog.str());
        if (send_length < recv_length){
            logger("ERROR: Entire message not sent\n");
        }
    }
    return 0;
}


void Agent::garble_msg() {

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
void Agent::dump_msg(Interface * source) {
    unsigned short seq_num = 0;
    int opcode = this->message[0];
    slog << "      OP Code: " << opcode << endl;
    if (source->getType()== XSERVER){
        memcpy(&seq_num, this->message + 2, sizeof(unsigned short));
        slog << "      Seq #" << seq_num << endl;
    }
    logger(slog.str());
}