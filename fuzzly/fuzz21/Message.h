//
// Created by devbox on 11/22/21.
//

#ifndef FUZZ_MESSAGES_H
#define FUZZ_MESSAGES_H

const int MESSAGE_SIZE = 32;
const int BUFFER_SIZE = 16384;

class Message {
protected:
    char * msg;
public:
    Message();
    ~Message();
    //send();
    //receive();

};


#endif //FUZZ_MESSAGES_H
