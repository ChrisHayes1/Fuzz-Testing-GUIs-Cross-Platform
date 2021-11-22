//
// Created by devbox on 11/17/21.
#include <sstream>
#include <iostream>
#include <cstring>
#include <errno.h>
#include "_types.h"

using namespace std;

//stringstream slog;
int DISPLAY_MSGS = 1;

void logger(string msg_log, LogMode log_mode){
    if (DISPLAY_MSGS && (log_mode==ERR || log_mode==BOTH)) {
        cerr << msg_log;
    }
    if (log_mode==OUT || log_mode==BOTH) {cout << msg_log;}
    slog.str("");
    slog.clear();
}

void log_error(string p_msg){
    if (DISPLAY_MSGS){
        perror(p_msg.c_str());
    }
}


