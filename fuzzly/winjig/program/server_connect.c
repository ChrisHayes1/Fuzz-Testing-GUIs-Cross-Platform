/*
 *  $Id: server_connect.c,v 1.3 1995/05/01 04:58:08 ajitk Exp $
 *
 *  Implements the server_connect function which connects itself to the
 *  actual X server.
 */

#include <unistd.h>		/* gethostname */

#include <sys/param.h>		/* socket related stuff */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <string.h>		/* general */
#include <stdio.h>
#include <stdlib.h>

#include <X11/X.h>
#include <X11/Xauth.h>		/* authorization related functions */

#include "../../fuzz21/old/_const.h"
#include "types.h"
#include "globals.h"
#include "macros.h"
#include "functions.h"

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
	if ((auth_buffer = malloc(total_length =
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
