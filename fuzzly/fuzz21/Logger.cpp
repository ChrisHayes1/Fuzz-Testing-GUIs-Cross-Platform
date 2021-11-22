//
// Created by devbox on 11/22/21.
//

#include "Logger.h"

using namespace std;


int DISPLAY_MSGS = 1;
stringstream slog;

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