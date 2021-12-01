//
// Created by devbox on 11/22/21.
//

#ifndef FUZZ_INTERFACE_H
#define FUZZ_INTERFACE_H

#include <unistd.h>
#include "Logger.h"

/*
 * Each interface represents a connection to one side of the X-system
 * The interface will either act as a server and connect to the client,
 * or act as a client and connect to the server.
 */

const int X_PORT = 6000;
const int X_LIMIT = 9;

enum I_TYPE {CLIENT=0, XSERVER=1};
enum endianness {LITTLE_ENDIAN_NEW = 0154, BIG_ENDIAN_NEW    = 0102};

const int BUFFER_SIZE = 16384;

class Interface {
protected:
    int fd;
    int port;
    enum I_TYPE cxn_type;
    enum endianness endian;
    unsigned short  major_protocol, minor_protocol;
    char message[BUFFER_SIZE];
    int msg_count;
    uint16_t msg_size=0;
    string name;
public:
    // Constructors & Destructors
    Interface();
    Interface(I_TYPE _cxn_type, int _port);
    ~Interface(){close(fd);}
    // Getters and setters
    int getFD(){return fd;}
    unsigned short  getMajor(){return major_protocol;}
    unsigned short  getMinor(){return minor_protocol;}
    enum endianness getEndianess(){return endian;}
    string getName(){return name;}
    enum I_TYPE getType(){return cxn_type;}
    char * getMessagePtr(){return message;}
    // Methods
    int connect_server(Interface * to_client);
    int connect_client();
//    int send_msg(char * buffer, int msg_length);
//    int recv_msg();
//    int garble_msg();
//    int inject_message();
private:
    int client_authenticate();
    friend class Agent;
//    void dump_msg();
};


#endif //FUZZ_INTERFACE_H
