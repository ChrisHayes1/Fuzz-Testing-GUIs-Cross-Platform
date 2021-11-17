//
// Created by devbox on 11/15/21.
//

#ifndef FUZZ_PIPELINE_H
#define FUZZ_PIPELINE_H

struct SockMsgAuth {
    enum endianness endian;
    unsigned short protocol_major_version;
    unsigned short protocol_minor_version;
    unsigned short  auth_name_len;
    unsigned short auth_data_len;
};

class Pathways {
public:
    virtual int connect(int port) = 0;
protected:
    SockMsgAuth auth_msg;


};


#endif //FUZZ_PIPELINE_H
