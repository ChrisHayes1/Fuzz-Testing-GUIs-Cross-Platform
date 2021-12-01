//
// Created by devbox on 11/22/21.
//

#ifndef FUZZ_AGENT_H
#define FUZZ_AGENT_H

#include "Interface.h"
#include <time.h>

const int MESSAGE_SIZE = 1024;
const int BUFFER_SIZE = 16384;
const int EVENT_MESSAGE_SIZE = 32;
const int TRACKED_MSGS = 32;
class Agent {
    Interface * to_client;
    Interface * to_xserver;
    char message[TRACKED_MSGS][BUFFER_SIZE];
    size_t message_length[TRACKED_MSGS];
    int current_msg = 0;
    int startgap;
    int mod_mode;
    int mod_rate;
    int inj_mode;
    int inj_rate;
    int msg_count = 0;
    timespec last_injection;
    timespec current_time;
    long elapsedTime;
    unsigned short seq_num=0;
    unsigned short req_num=0;
    int have_buffered = 0; // count of # of buffered msgs, caps at TRACKED_MSGS
    bool valid_seq = false;
public:
    Agent(Interface * _to_client, Interface * _to_xserver, int seed);
    ~Agent();
    int converse();
    // Getters and Setters
    void set_mod_rate(int _rate){mod_rate = _rate;}
    void set_mod_mode(int _mode) {mod_mode = _mode;}
    void set_inj_rate(int _rate){inj_rate = _rate;}
    void set_inj_mode(int _mode) {inj_mode = _mode;}
    void set_startgap(int _startgap){startgap=_startgap;}
private:
    int transfer_msg(Interface * source, Interface * dest);
    int Recv(Interface * source);
    int Send(Interface * dest, int msg_length);
    void dump_msg(Interface * source, char * msg);
    void incr_msg(Interface * dest, int msg_length);
    // Calls garblers
    void alter_msg(Interface * source, int &recv_length, char * msg);
    // Garblers
    void garbler(int &recv_length, char * msg);
    void kill_seq(int &recv_length, char * msg);
    void kill_length(int &recv_length);
    // Calls Injectors
    void inject_message(Interface * dest);
    // Injectors
    int generate_noise( char * buffer);
    int generate_events( char * buffer);
    int replay_events( char * buffer);

};


#endif //FUZZ_AGENT_H
