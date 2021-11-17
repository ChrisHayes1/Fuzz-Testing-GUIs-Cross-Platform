//
// Created by devbox on 11/17/21.
//
#include <sstream>
#include <iostream>

std::stringstream slog;



int DISPLAY_MSGS = 0;

#ifdef ERROR_MESSAGES
    DISPLAY_MSGS = 1;
#endif

void logger(string msg_log, LogMode log_mode){
    if (DISPLAY_MSGS && (log_mode==ERR || log_mode==BOTH)) {
        cerr << msg_log;
    }
    if (log_mode==ERR || log_mode==BOTH) {cout << msg_log;}
}



