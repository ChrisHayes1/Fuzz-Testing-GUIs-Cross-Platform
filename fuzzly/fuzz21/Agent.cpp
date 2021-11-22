//
// Created by devbox on 11/22/21.
//
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/poll.h>
#include <sstream>
#include <iostream>
#include <cstring>
#include <stdio.h>
#include <errno.h>

#include "Agent.h"
#include "Interface.h"
#include "Logger.h"
#include "_globals.h"



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
                                mreturn = pass_msg(this->to_xserver, this->to_client);
                                if (mreturn < 0) { return mreturn; }
                            } else if (i == CLIENT) {
                                logger("Client sent a message\n");
                                mreturn = pass_msg(this->to_client, this->to_xserver);
                                if (mreturn < 0) { return mreturn; }
                            }
                        }
                    }
                }
        }
    }
    return 1;
}

int Agent::pass_msg(Interface * source, Interface * dest){
    char buffer[BUFFER_SIZE];
    int recv_length, send_length;
    logger("   Ready to recv -> ");
    recv_length = recv(source->getFD(), buffer, BUFFER_SIZE, 0);
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
        //dump(buffer, recv_length);
        send_length = send(dest->getFD(), buffer, recv_length, 0);
        if (send_length < recv_length){
            logger("ERROR: Entire message not sent\n");
        }
        logger("Message sent: \n");
        logger(slog.str());
    }
    return 0;
}