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
public:
    Agent(Interface * _to_client, Interface * _to_xserver);
    ~Agent();
    int converse();
private:
    int pass_msg(Interface * source, Interface * dest);
};


#endif //FUZZ_AGENT_H
