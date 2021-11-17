//
// Created by devbox on 11/17/21.
//
#include <sstream>
#include <iostream>

std::stringstream slog;
int DISPLAY_MSGS = 1;

void logger(string msg_log, LogMode log_mode){
    if (DISPLAY_MSGS && (log_mode==ERR || log_mode==BOTH)) {
        cerr << msg_log;
    }
    if (log_mode==OUT || log_mode==BOTH) {cout << msg_log;}
    slog.str("");
    slog.clear();
}



