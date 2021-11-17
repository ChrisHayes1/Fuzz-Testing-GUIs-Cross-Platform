//
// Created by devbox on 11/15/21.
//

#ifndef FUZZ_PIPE_TO_CLIENT_H
#define FUZZ_PIPE_TO_CLIENT_H

#include "pathways.h"

class PathToClient : public Pathways {
public:
    PathToClient();
    int connect(int port);
};


#endif //FUZZ_PIPE_TO_CLIENT_H
