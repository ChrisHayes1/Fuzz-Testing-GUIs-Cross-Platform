//
// Created by devbox on 11/17/21.
//

#ifndef FUZZ_LOGGER_H
#define FUZZ_LOGGER_H

#include <sstream>

sstream slog;

enum LOGGER = {OUT, ERR, BOTH};

int logger(string msg_log, LOGGER=BOTH){
    if (DISPLAY_MSGS && (LOGGER=ERR || LOGGER=BOTH)) {
        cerr << msg_log;
    }
    cout << msg_log;
}

#endif //FUZZ_LOGGER_H


