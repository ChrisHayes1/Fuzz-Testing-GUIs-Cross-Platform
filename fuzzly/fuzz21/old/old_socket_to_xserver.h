//
// Created by devbox on 11/15/21.
//

#ifndef FUZZ_PIPE_TO_XSERVER_H
#define FUZZ_PIPE_TO_XSERVER_H


class PipeToXServer : public SockCxn {
public:
    int connect(int port);
};


#endif //FUZZ_PIPE_TO_XSERVER_H
