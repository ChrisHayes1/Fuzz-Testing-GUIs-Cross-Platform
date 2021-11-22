//
// Created by devbox on 11/22/21.
//

#ifndef FUZZ_LOGGER_H
#define FUZZ_LOGGER_H

#include <iostream>
#include <sstream>

using namespace std;

extern stringstream slog;
enum LogMode {OUT, ERR, BOTH};

void logger(string msg_log, LogMode log_mode = BOTH);
void log_error(string p_msg);

#endif //FUZZ_LOGGER_H
