//
// Created by devbox on 11/22/21.
//

#ifndef FUZZ_AGENT_H
#define FUZZ_AGENT_H

#include "Interface.h"
#include "Message.h"


class Agent {
    Interface * to_client;
    Interface * to_xserver;
    char message[BUFFER_SIZE];
public:
    Agent(Interface * _to_client, Interface * _to_xserver);
    ~Agent();
    int converse();
private:
    int transfer_msg(Interface * source, Interface * dest);
    void garble_msg();
    int inject_message();
    void dump_msg(Interface * source);
};


#endif //FUZZ_AGENT_H
