//
// Created by devbox on 11/22/21.
//

#include <unistd.h>		/* gethostname */

#include <sys/param.h>		/* socket related stuff */
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <string.h>		/* general */
#include <stdio.h>
#include <stdlib.h>

#include <X11/X.h>
#include <X11/Xauth.h>		/* authorization related functions */
#include <iostream>


#include "_macros.h"

#include "Interface.h"
#include "Message.h"
#include "Logger.h"


/****
 * Public methods
 */

Interface::Interface(I_TYPE _cxn_type, int _port) : cxn_type(_cxn_type), port(_port) {}

int Interface::connect_client()
{
    /*
     *  Check parameters.  port shouldn't be unreasonable.
     */
    if (this->port <= X_PORT  ||  this->port > X_PORT + X_LIMIT)
    {
        logger("(client_connect): Bad parameters.\n", ERR);
        return -1;
    }

    /*
     *  Create socket.
     */
    int client_side_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_side_socket == -1)
    {
        logger("(client_connect): Can't create socket.\n", ERR);
        log_error("(client_connect");
        //perror("(client_connect)");
        return -1;
    }

    /*
     *  Bind the socket.
     */

    // Build address details to help bind socket
    struct sockaddr_in	my_ip_address;
    bzero((char *) &my_ip_address, sizeof(my_ip_address));
    my_ip_address.sin_family	= AF_INET;
    my_ip_address.sin_port		= htons(this->port);
    my_ip_address.sin_addr.s_addr	= INADDR_ANY;

    // Bind
    if (bind(client_side_socket, (struct sockaddr *) &my_ip_address, sizeof(my_ip_address)) == -1)
    {
        logger("(client_connect): Can't bind socket.\n", ERR);
        log_error("client_connect)");
        close(client_side_socket);
        return -1;
    }

    /*
     *  Listen on the socket to set the backlog.  We don't want any
     *  backlog because we will be accepting one client atmost.
     */
    if (listen(client_side_socket, 5) == -1)
    {
        logger("(client_connect): Can't set backlog on socket.\n", ERR);
        log_error("client_connect");
        close(client_side_socket);
        return -1;
    }

    /*
     *  Accept the connection.
     */
    sockaddr client_ip_address;
    socklen_t client_ip_address_len = sizeof(client_ip_address);
    if ((this->fd = accept(client_side_socket, &client_ip_address, &client_ip_address_len)) == -1)
    {
        logger("(client_connect): Can't accept connection.\n", ERR);
        log_error("(client_connect)");
        close(client_side_socket);
        return -1;
    }

    // Deal with client authentication
    close(client_side_socket);
    return client_authenticate();
}

int Interface::client_authenticate(){
    short auth_name_len, auth_data_len, total_len;
    char buffer[12], *auth_buffer;


    // THB - added tracking rcv length, length is funky, maybe it doesnt matter.
    int x = recv(this->fd, buffer, 12, 0);
    if (x < 12)
    {
        logger("(client_connect): Can't recv data.\n", ERR);
        perror("(client_connect)");
        close(this->fd);
        return -1;
    }

    /*
     *  Copy required data out.
     */
    //enum endianness *endian_ptr = (enum endianness) buffer[0];
    this->endian = (enum endianness) buffer[0];
    memcpy(&this->major_protocol, buffer + 2, sizeof(this->major_protocol));
    memcpy(&this->minor_protocol, buffer + 4, sizeof(this->minor_protocol));
    memcpy(&auth_name_len, buffer + 6, sizeof(auth_name_len));
    memcpy(&auth_data_len, buffer + 8, sizeof(auth_data_len));
    total_len = (auth_name_len + auth_data_len);

    //dump authentication details
    slog << "...protocol_major_version_ptr: " << this->major_protocol << endl
         << "...protocol_minor_version_ptr: " << this->minor_protocol << endl
         << "...endian_ptr: " <<  this->endian << endl
         << "...auth_name_len: " << auth_name_len << endl
         << "...auth_data_len: " << auth_data_len << endl
         << "...total_len: " << total_len << endl;
    logger(slog.str(), ERR);

    /*
     *  Discard the authorization part of the opening message.
     *  This shouldn't reach the real X server.
     */
    if (total_len > 0)
    {
        auth_buffer = (char*)malloc(total_len);
        if (auth_buffer == NULL)
        {
            logger("(client_connect): Can't allocate temporary buffer.\n", ERR);
            close(this->fd);
            return -1;
        }

        int auth_buffer_length = recv(this->fd, auth_buffer, BUFFER_SIZE, 0);
        slog << "   Auth Buffer received message of size " << auth_buffer_length << endl;
        logger(slog.str());

        if (auth_buffer_length < total_len)
        {
            logger("(client_connect): Couldn't discard authorization data.", ERR);
            perror("(client_connect)");
            close(this->fd);
            free(auth_buffer);
            return -1;
        }
        free(auth_buffer);
    }

    return 0;
}

/* ---------------------------------------------------------------- */

/*
 *  server_connect
 *
 *  This program runs on the same host as the X server.  This function queries
 *  the .Xauthority file to get the appropriate magic cookie.  It then
 *  handcrafts the connection message.  The reply from the actual server is
 *  handled as a normal message from the server to the client.  Authorization
 *  works only for MIT_MAGIC_COOKIEs.
 */

int Interface::connect_server(Interface * to_client)
{
    char			 buffer[12], *auth_buffer;
    char			 my_hostname[MAXHOSTNAMELEN];
    struct hostent		*hostent_ptr;
    Xauth			*xauth_ptr;
    unsigned short		 new_name_length, new_data_length;
    unsigned short		 total_length;
    struct sockaddr_in	 server_sock_addr;
    int			 server_side_socket;

    enum endianness endian;
    unsigned short  major, minor;

    /*
     *  Handcraft the fixed part of the connection message.
     */
    major = this->major_protocol = to_client->getMajor();
    minor = this->minor_protocol = to_client->getMinor();
    endian = this->endian = to_client->getEndianess();

    slog << "...protocol_major_version_ptr: " << major << endl
         << "...protocol_minor_version_ptr: " << minor << endl
         << "...endian_ptr: " <<  endian << endl;
    logger(slog.str());

    buffer[0] = endian;
    buffer[1] = 0;		/* unused in the message */
    memcpy(buffer + 2, &major, sizeof(major));
    memcpy(buffer + 4, &minor, sizeof(minor));



    /*
     *  Get our host's IP address.
     */
    if (gethostname(my_hostname, MAXHOSTNAMELEN) == -1)
    {
        logger("(server_connect): Can't get hostname.\n", ERR);
        log_error("server_connect");
        return -1;
    }

    if ((hostent_ptr = gethostbyname(my_hostname)) == NULL)
    {
        logger("(server_connect): Can't get IP address of %s.\n", ERR);
        log_error("(server_connect)");
        return -1;
    }

    bzero((char *) &server_sock_addr, sizeof(struct sockaddr_in));
    memcpy(&server_sock_addr.sin_addr.s_addr, hostent_ptr->h_addr_list[0],
           sizeof(struct in_addr));

    /*
     *  Get the authorization info. for display number 0 of our host.
     *  We bother about MIT Magic Cookie authorization only.
     */

    if ((xauth_ptr = XauGetAuthByAddr(FamilyInternet,
                                      sizeof(struct in_addr),
                                      (char *) &server_sock_addr.sin_addr,
                                      1, "0",
                                      strlen("MIT-MAGIC-COOKIE-1"),
                                      "MIT-MAGIC-COOKIE-1")) == NULL)
    {
        slog << "server_connect): Can't get authorization data for " << my_hostname
             << (unsigned char) hostent_ptr->h_addr_list[0][0] << "."
             << (unsigned char) hostent_ptr->h_addr_list[0][1] << "."
             << (unsigned char) hostent_ptr->h_addr_list[0][2] << "."
             << (unsigned char) hostent_ptr->h_addr_list[0][3] << endl;
        logger(slog.str());
        return -1;
    }

    /*
     *  Build up the authorization part of the connection message.
     */
    if ((auth_buffer = (char*)malloc(total_length =
                                             NEAREST_MULTIPLE_OF_4(xauth_ptr->name_length) +
                                             NEAREST_MULTIPLE_OF_4(xauth_ptr->data_length)))
        == NULL)
    {
        logger("(server_connect): Can't allocate buffer.\n", ERR);
        XauDisposeAuth(xauth_ptr);
        return -1;
    }
    slog << "   Server total length = " << total_length << endl;
    logger(slog.str());

    new_name_length = NEAREST_MULTIPLE_OF_4(xauth_ptr->name_length);
    new_data_length = NEAREST_MULTIPLE_OF_4(xauth_ptr->data_length);
    memcpy(buffer + 6, &xauth_ptr->name_length, sizeof(xauth_ptr->name_length));
    memcpy(buffer + 8, &xauth_ptr->data_length, sizeof(xauth_ptr->data_length));
    buffer[10] = buffer[11] = 0;	/* unused in the message */

    memcpy(auth_buffer, xauth_ptr->name, xauth_ptr->name_length);
    memcpy(auth_buffer + new_name_length,
           xauth_ptr->data, xauth_ptr->data_length);

    XauDisposeAuth(xauth_ptr);

    /*
     *  Get a connection to the server.  Transmit the connection message
     *  which is buffer followed by auth_buffer.
     */
    if ((server_side_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        logger("(server_connect): Can't get server side socket.\n", ERR);
        log_error("(server_connect");
        free(auth_buffer);
        return -1;
    }

    server_sock_addr.sin_family = AF_INET;
    server_sock_addr.sin_port   = htons(X_PORT);
    if (connect(server_side_socket, (struct sockaddr *) &server_sock_addr,
                sizeof(server_sock_addr)) == -1)
    {
        slog << "(server_connect): Can't connect to " << my_hostname
             << (unsigned char) hostent_ptr->h_addr_list[0][0] << "."
             << (unsigned char) hostent_ptr->h_addr_list[0][1] << "."
             << (unsigned char) hostent_ptr->h_addr_list[0][2] << "."
             << (unsigned char) hostent_ptr->h_addr_list[0][3]
             << " on port " << X_PORT;
        logger(slog.str());
        close(server_side_socket);
        free(auth_buffer);
        return -1;
    }

    /*
     *  Send the connection message in two bursts.  Since TCP
     *  doesn't care about message boundaries, this is ok.
     */
    if (send(server_side_socket, buffer, 12, 0) < 12)
    {
        logger("(server_connect): Can't send initial part of connection message.\n", ERR);
        log_error("(server_connect");
        close(server_side_socket);
        free(auth_buffer);
        return -1;
    }


    if (send(server_side_socket, auth_buffer, total_length, 0) < total_length)
    {
        logger("(server_connect): Can't send authorization part of connection message.\n", ERR);
        close(server_side_socket);
        free(auth_buffer);
        return -1;
    }

    free(auth_buffer);

    this->fd = server_side_socket;
    return 0;
}





