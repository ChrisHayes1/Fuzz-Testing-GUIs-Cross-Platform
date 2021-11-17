//
// Created by devbox on 11/17/21.
//

#ifndef FUZZ_LOGGER_H
#define FUZZ_LOGGER_H

#include <sstream>
#include <iostream>
std::stringstream slog;

enum LogMode {OUT, ERR, BOTH};

void logger(string msg_log, LogMode log_mode=BOTH){
    if (DISPLAY_MSGS && (log_mode==ERR || log_mode==BOTH)) {
        cerr << msg_log;
    }
    if (log_mode==ERR || log_mode==BOTH) {cout << msg_log;}
}

#endif //FUZZ_LOGGER_H


