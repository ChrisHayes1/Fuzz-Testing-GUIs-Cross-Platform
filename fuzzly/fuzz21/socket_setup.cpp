/*
 *  $Id: server_connect.c,v 1.3 1995/05/01 04:58:08 ajitk Exp $
 *
 *  Implements the server_connect function which connects itself to the
 *  actual X server.
 */

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

#include "_const.h"
#include "_types.h"
#include "_globals.h"
#include "_macros.h"
#include "_functions.h"

/* ------------------------------------------------------------------ */

/*
 *  client_connect
 *
 *  Wait for a connection from the client.  We allow a maximum of one
 *  client only.
 */

void dumpSS(char * msg, int size){
    cerr << "Dumping message:" << endl;
    for (int i = 0; i < size; i++){
        cerr << msg[i];
    }
    cerr << endl;
}

int client_authenticate(int client_message_socket, enum endianness *endian_ptr,
                        unsigned short *protocol_major_version_ptr,
                        unsigned short *protocol_minor_version_ptr){
    short auth_name_len, auth_data_len, total_len;
    char buffer[12], *auth_buffer;

    /*
     *  Discard the authorization part of the opening message.
     *  This shouldn't reach the real X server.
     */
    // THB - added tracking rcv length, length is funky, maybe it doesnt matter.
    int x = recv(client_message_socket, buffer, 12, 0);
    if (x < 12)
    {
        logger("(client_connect): Can't recv data.\n", ERR);
        perror("(client_connect)");
        close(client_message_socket);
        return -1;
    }

    /*
     *  Copy required data out.
     */
    *endian_ptr = (enum endianness) buffer[0];
    memcpy(protocol_major_version_ptr, buffer + 2, sizeof(*protocol_major_version_ptr));
    memcpy(protocol_minor_version_ptr, buffer + 4, sizeof(*protocol_minor_version_ptr));
    memcpy(&auth_name_len, buffer + 6, sizeof(auth_name_len));
    memcpy(&auth_data_len, buffer + 8, sizeof(auth_data_len));
    total_len = (auth_name_len + auth_data_len);

    //THB - for EC
    if (DISPLAY_MSGS) {
        fprintf(stderr, "...protocol_major_version_ptr: %u\n", *protocol_major_version_ptr);
        fprintf(stderr, "...protocol_minor_version_ptr: %u\n", *protocol_minor_version_ptr);
        fprintf(stderr, "...endian_ptr: %u\n", *endian_ptr);
        fprintf(stderr, "...auth_name_len: %u\n", auth_name_len);
        fprintf(stderr, "...auth_data_len: %u\n", auth_data_len);
        fprintf(stderr, "...total_len: %i\n", total_len);
    }

    if (total_len > 0)
    {
        auth_buffer = (char*)malloc(total_len);
        if (auth_buffer == NULL)
        {
            logger("(client_connect): Can't allocate temporary buffer.\n", ERR);
            close(client_message_socket);
            return -1;
        }

        int auth_buffer_length = recv(client_message_socket, auth_buffer, BUFFER_SIZE, 0);
        slog << "Auth Buffer received message of size " << auth_buffer_length;
        logger(slog.str());

        if (auth_buffer_length < total_len)
        {
            logger("(client_connect): Couldn't discard authorization data.", ERR);
            perror("(client_connect)");
            close(client_message_socket);
            free(auth_buffer);
            return -1;
        }
        char author_name[auth_name_len];
        char * author_data;
        memcpy(&author_name, auth_buffer , auth_name_len);
        //memcpy(&author_data, auth_buffer+auth_name_len, auth_data_len);
        if (DISPLAY_MSGS) {
            //fprintf(stderr, "...Author Name: %s\n", author_name);
            //fprintf(stderr, "...Author Data: %s\n", author_data);
        }
        dumpSS(auth_buffer, total_len);
        free(auth_buffer);
    }

    return client_message_socket;
}

int client_connect(int port, enum endianness *endian_ptr,
                   unsigned short *protocol_major_version_ptr,
                   unsigned short *protocol_minor_version_ptr)
{
    int	client_side_socket, client_message_socket;
    struct sockaddr_in	my_ip_address;
    sockaddr client_ip_address;
    socklen_t client_ip_address_len = sizeof(client_ip_address);


    /*
     *  Check parameters.  port shouldn't be unreasonable.
     */
    if (port <= X_PORT  ||  port > X_PORT + X_LIMIT)
    {
        if (DISPLAY_MSGS) {
            fprintf(stderr, "%s (client_connect): Bad parameters.\n",
                    progname);
        }
        return -1;
    }

    /*
     *  Get the port, listen on it.  Check for errors on every system call
     *  so that we don't exit without closing the socket.
     */

    /*
     *  Create socket.
     */
    client_side_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_side_socket == -1)
    {
        if (DISPLAY_MSGS) {
            fprintf(stderr, "%s (client_connect): Can't create socket.\n",
                    progname);
            perror("(client_connect)");
        }
        return -1;
    }

    /*
     *  Bind the socket.
     */
    bzero((char *) &my_ip_address, sizeof(my_ip_address));
    my_ip_address.sin_family	= AF_INET;
    my_ip_address.sin_port		= htons(port);
    my_ip_address.sin_addr.s_addr	= INADDR_ANY;
    if (bind(client_side_socket, (struct sockaddr *) &my_ip_address,
             sizeof(my_ip_address)) == -1)
    {
        if (DISPLAY_MSGS) {
            fprintf(stderr, "%s (client_connect): Can't bind socket.\n",
                    progname);
            perror("(client_connect)");
        }
        close(client_side_socket);
        return -1;
    }

    /*
     *  Listen on the socket to set the backlog.  We don't want any
     *  backlog because we will be accepting one client atmost.
     */
    if (listen(client_side_socket, 5) == -1)
    {
        if (DISPLAY_MSGS) {
            fprintf(stderr,
                    "%s (client_connect): Can't set backlog on socket.\n",
                    progname);
            perror("(client_connect)");
        }
        close(client_side_socket);
        return -1;
    }

    /*
     *  accept the connection.
     * THB: Why not use the same socket, why set to client_msg_socket?
     */
    if ((client_message_socket = accept(client_side_socket,
                                        &client_ip_address,
                                        &client_ip_address_len)) == -1)
    {
        if (DISPLAY_MSGS) {
            fprintf(stderr, "%s (client_connect): Can't accept connection.\n",
                    progname);
            perror("(client_connect)");
        }
        close(client_side_socket);
        return -1;
    }

    // Deal with client authentication
    close(client_side_socket);
    return client_authenticate(client_message_socket, endian_ptr, protocol_major_version_ptr, protocol_minor_version_ptr);
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

int server_connect(enum endianness endian,
                   unsigned short protocol_major_version,
                   unsigned short protocol_minor_version)
{
    char			 buffer[12], *auth_buffer;
    char			 my_hostname[MAXHOSTNAMELEN];
    struct hostent		*hostent_ptr;
    Xauth			*xauth_ptr;
    unsigned short		 new_name_length, new_data_length;
    unsigned short		 total_length;
    struct sockaddr_in	 server_sock_addr;
    int			 server_side_socket;

    /*
     *  Handcraft the fixed part of the connection message.
     */
    buffer[0] = (char) endian;
    buffer[1] = 0;		/* unused in the message */
    memcpy(buffer + 2, &protocol_major_version,
           sizeof(protocol_major_version));
    memcpy(buffer + 4, &protocol_minor_version,
           sizeof(protocol_minor_version));

    /*
     *  Get our host's IP address.
     */
    if (gethostname(my_hostname, MAXHOSTNAMELEN) == -1)
    {
        if (DISPLAY_MSGS) {
            fprintf(stderr, "%s (server_connect): Can't get hostname.\n",
                    progname);
            perror("(server_connect)");
        }
        return -1;
    }
    if ((hostent_ptr = gethostbyname(my_hostname)) == NULL)
    {
        if (DISPLAY_MSGS) {
            fprintf(stderr,
                    "%s (server_connect): Can't get IP address of %s.\n",
                    progname, my_hostname);
            perror("(server_connect)");
        }
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
        if (DISPLAY_MSGS) {
            fprintf(stderr, "%s (server_connect): Can't get "
                            "authorization data for %s [%d.%d.%d.%d].\n",
                    progname, my_hostname,
                    (unsigned char) hostent_ptr->h_addr_list[0][0],
                    (unsigned char) hostent_ptr->h_addr_list[0][1],
                    (unsigned char) hostent_ptr->h_addr_list[0][2],
                    (unsigned char) hostent_ptr->h_addr_list[0][3]);
        }
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
        if (DISPLAY_MSGS) {
            fprintf(stderr, "%s (server_connect): Can't allocate buffer.\n",
                    progname);
        }
        XauDisposeAuth(xauth_ptr);
        return -1;
    }

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
        if (DISPLAY_MSGS) {
            fprintf(stderr, "%s (server_connect): Can't get server side socket.\n",
                    progname);
            perror("(server_connect)");
        }
        free(auth_buffer);
        return -1;
    }
    server_sock_addr.sin_family = AF_INET;
    server_sock_addr.sin_port   = htons(X_PORT);
    if (connect(server_side_socket, (struct sockaddr *) &server_sock_addr,
                sizeof(server_sock_addr)) == -1)
    {
        if (DISPLAY_MSGS) {
            fprintf(stderr,
                    "%s (server_connect): Can't connect to %s [%u.%u.%u.%u], port %d.\n",
                    progname, my_hostname,
                    hostent_ptr->h_addr_list[0][0],
                    hostent_ptr->h_addr_list[0][1],
                    hostent_ptr->h_addr_list[0][2],
                    hostent_ptr->h_addr_list[0][3],
                    X_PORT);
        }
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
        if (DISPLAY_MSGS) {
            fprintf(stderr, "%s (server_connect): Can't send initial part "
                            "of connection message.\n", progname);
            perror("(server_connect)");
        }
        close(server_side_socket);
        free(auth_buffer);
        return -1;
    }

    slog << "   Buffer: " << endl;
    logger(slog.str(), ERR);
    dumpSS(buffer, total_length);

    slog << "   Auth Buffer: " << endl;
    logger(slog.str(), ERR);
    dumpSS(auth_buffer, total_length);

    if (send(server_side_socket, auth_buffer, total_length, 0) < total_length)
    {
        if (DISPLAY_MSGS) {
            fprintf(stderr, "%s (server_connect): Can't send authorization part"
                            " of connection data.\n", progname);
        }
        close(server_side_socket);
        free(auth_buffer);
        return -1;
    }

    free(auth_buffer);

    return server_side_socket;
}
