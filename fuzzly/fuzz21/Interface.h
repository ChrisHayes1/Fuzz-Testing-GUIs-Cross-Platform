//
// Created by devbox on 11/22/21.
//

#ifndef FUZZ_INTERFACE_H
#define FUZZ_INTERFACE_H

#include <unistd.h>

/*
 * Each interface represents a connection to one side of the X-system
 * The interface will either act as a server and connect to the client,
 * or act as a client and connect to the server.
 */

const int X_PORT = 6001;
const int X_LIMIT = 9;

enum I_TYPE {CLIENT=0, XSERVER=1};
enum endianness {LITTLE_ENDIAN_NEW = 0154, BIG_ENDIAN_NEW    = 0102};

class Interface {
protected:
    int fd;
    int port;
    enum I_TYPE cxn_type;
    char endian;
    unsigned short  major_protocol, minor_protocol;
public:
    // Constructors & Destructors
    Interface(I_TYPE _cxn_type, int _port);
    ~Interface(){close(fd);}
    // Getters and setters
    int getFD(){return fd;}
    unsigned short  getMajor(){return major_protocol;}
    unsigned short  getMinor(){return minor_protocol;}
    char getEndianess(){return endian;}
    // Methods
    int connect_server(Interface * to_client);
    int connect_client();

private:
    int client_authenticate();
};


#endif //FUZZ_INTERFACE_H
