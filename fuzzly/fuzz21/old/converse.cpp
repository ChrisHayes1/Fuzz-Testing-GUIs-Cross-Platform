/*
 *  $Id: copy_message.c,v 1.4 1995/05/01 04:56:14 ajitk Exp $
 *
 *  Implements the copy_message function which copies messages from the source
 *  socket to the destination socket.  Also provides an option to make itself
 *  (non) blocking.
 */

//#include <fcntl.h>		/* network related stuff */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
//#include <sys/resource.h>
#include <sys/poll.h>
#include <cstring>
#include <stdio.h>		/* general */
//#include <errno.h>
#include <iostream>
#include "_const.h"
#include "_types.h"
#include "../_globals.h"
#include "_functions.h"

//Specific to verify, can remove when done with it.
#include <fcntl.h>

void dump(char * msg, int size){
    cerr << "\nDumping message: msg[0] = " << msg[0] << "********************" << endl;
    for (int i = 0; i < size; i++){
        cerr << msg[i];
    }
    cerr << "********************" << endl;
    cerr << endl << endl;

}

int copy_msgs(int source_socket, int dest_socket){
    char buffer[BUFFER_SIZE];
    int recv_length, send_length;
    logger("   Ready to recv -> ");
    recv_length = recv(source_socket, buffer, BUFFER_SIZE, 0);
    slog << "Receive msg of size " << recv_length << endl;
    logger(slog.str());
    if (recv_length < 0){
        logger("ERROR: Receive from source failed\n");
        return -2;
    }
    else if (recv_length == 0){
        logger("NOTE: Source socket gracefully failed\n", ERR);
        return -1;
    } else {
        slog << "   Sending msg of size " << recv_length << "-> " << endl;
        logger(slog.str());
        dump(buffer, recv_length);
        send_length = send(dest_socket, buffer, recv_length, 0);
        if (send_length < recv_length){
            logger("ERROR: Entire message not sent\n");
        }
        logger("Message sent: \n");
        logger(slog.str());
    }
    return 0;
}

void verify_empty(struct pollfd* fds, int nfds){
    for (int i = 0; i < nfds; i++){
        int flags;
        int	status;
        char buffer[BUFFER_SIZE];

        flags = fcntl(fds[i].fd, F_GETFL, 0);
        status = fcntl(fds[i].fd, F_SETFL, FNDELAY);
        if (status == -1){logger("Error on status\n");}
        int recv_length = recv(fds[i].fd, buffer, BUFFER_SIZE, 0);
        if (recv_length == -1){
            if (errno != EWOULDBLOCK){
                logger("Unexpected error on receive\n");
                status = fcntl(fds[i].fd, F_SETFL, flags & ~FNDELAY);
                return;
            }
        } else {
            slog << "Unexpected message found on verify of size " << recv_length
            << endl;
            logger(slog.str());
        }
        status = fcntl(fds[i].fd, F_SETFL, flags & ~FNDELAY);
    }
}


int converse(int client_socket, int server_socket){
    struct pollfd fds[2];
    int    nfds = 2;
    int fd_available;
    int count = 0;
    int disp_movement = 1000000;
    memset(fds, 0, sizeof(fds));
    fds[CLIENT].fd = client_socket;
    fds[CLIENT].events = POLLIN;
    fds[XSERVER].fd = server_socket;
    fds[XSERVER].events = POLLIN;

    while(true){
        if (count++%disp_movement == 0) {logger(".");}
        if (count%(disp_movement*10) == 0) {
            logger("\n");
            //verify_empty(fds, nfds);
        }
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
            slog << "Poll returned " << fd_available << endl;
            logger(slog.str(), ERR);
            int mreturn = 0;
            for (int i = nfds-1; i >=0; i--) { // backwards to check server first
                logger(to_string(i));
                if (fds[i].revents != 0) {  // Did we find something?
                    logger(" - Event found - ");
                    if (fds[i].revents != POLLIN) {
                        //Error occured
                        slog << "ERROR: poll event returned " << fds[i].revents << endl;
                        logger(slog.str());
                        return -1;
                    } else { // found a message
                        if (i == XSERVER) {
                            logger("Server sent a message\n");
                            mreturn = copy_msgs(server_socket, client_socket);
                            if (mreturn < 0) { return mreturn; }
                        } else if (i == CLIENT) {
                            logger("Client sent a message\n");
                            mreturn = copy_msgs(client_socket, server_socket);
                            if (mreturn < 0) { return mreturn; }
                        }
                    }
                }
            }
        }
    }
    return 1;
}


