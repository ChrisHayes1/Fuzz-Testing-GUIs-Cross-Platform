/*
 *  $Id: client_connect.c,v 1.3 1995/05/01 04:57:50 ajitk Exp $
 *
 *  Implements the client_connect function which establishes a connection
 *  with the client program.
 */

#include <sys/types.h>		/* network stuff */
#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>		/* we consider Internet clients only */
#include <netdb.h>

#include <stdio.h>		/* general */
#include <stdlib.h>

#include "const.h"
#include "types.h"
#include "globals.h"
#include "functions.h"

/* ------------------------------------------------------------------ */

/*
 *  client_connect
 *
 *  Wait for a connection from the client.  We allow a maximum of one
 *  client only.
 */

int client_connect(int port, enum endianness *endian_ptr,
		   unsigned short *protocol_major_version_ptr,
		   unsigned short *protocol_minor_version_ptr)
{
	int			client_side_socket, client_message_socket;
	struct sockaddr_in	my_ip_address, client_ip_address;
	int			client_ip_address_len = sizeof(client_ip_address);
	short			auth_name_len, auth_data_len, total_len;
	char			buffer[12], *auth_buffer;
	
	/*
	 *  Check parameters.  port shouldn't be unreasonable.
	 */
	if (port <= X_PORT  ||  port > X_PORT + X_LIMIT)
	{
#ifdef ERROR_MESSAGES
		fprintf(stderr, "%s (client_connect): Bad parameters.\n",
			progname);
#endif
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
#ifdef ERROR_MESSAGES
		fprintf(stderr, "%s (client_connect): Can't create socket.\n",
			progname);
		perror("(client_connect)");
#endif
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
#ifdef ERROR_MESSAGES
		fprintf(stderr, "%s (client_connect): Can't bind socket.\n",
			progname);
		perror("(client_connect)");
#endif
		close(client_side_socket);
		return -1;
	}

	/*
	 *  Listen on the socket to set the backlog.  We don't want any
	 *  backlog because we will be accepting one client atmost.
	 */
	if (listen(client_side_socket, 5) == -1)
	{
#ifdef ERROR_MESSAGES
		fprintf(stderr,
			"%s (client_connect): Can't set backlog on socket.\n",
			progname);
		perror("(client_connect)");
#endif
		close(client_side_socket);
		return -1;
	}

	/*
	 *  accept the connection.
	 */
	if ((client_message_socket = accept(client_side_socket,
					    &client_ip_address,
					    &client_ip_address_len)) == -1)
	{
#ifdef ERROR_MESSAGES
		fprintf(stderr, "%s (client_connect): Can't accept connection.\n",
			progname);
		perror("(client_connect)");
#endif
		close(client_side_socket);
		return -1;
	}

	close(client_side_socket);

	/*
	 *  Discard the authorization part of the opening message.
	 *  This shouldn't reach the real X server.
	 */
	if (recv(client_message_socket, buffer, 12, 0) < 12)
	{
#ifdef ERROR_MESSAGES
		fprintf(stderr, "%s (client_connect): Can't recv data.\n",
			progname);
		perror("(client_connect)");
#endif
		close(client_message_socket);
		return -1;
	}

	/*
	 *  Copy required data out.
	 */
	memcpy(protocol_major_version_ptr, buffer + 2,
	       sizeof(*protocol_major_version_ptr));
	memcpy(protocol_minor_version_ptr, buffer + 4,
	       sizeof(*protocol_minor_version_ptr));
	*endian_ptr = (enum endianness) buffer[0];

	/*
	 *  Discard the rest of the message.
	 */
	memcpy(&auth_name_len, buffer + 6, sizeof(auth_name_len));
	memcpy(&auth_data_len, buffer + 8, sizeof(auth_data_len));
	total_len = (auth_name_len + auth_data_len);
	if (total_len > 0)
	{
		auth_buffer = malloc(total_len);
		if (auth_buffer == NULL)
		{
#ifdef ERROR_MESSAGES
			fprintf(stderr, "%s (client_connect): Can't allocate "
					"temporary buffer.\n", progname);
#endif
			close(client_message_socket);
			return -1;
		}
		if (recv(client_message_socket, auth_buffer,
			 total_len, 0) < total_len)
		{
#ifdef ERROR_MESSAGES
			fprintf(stderr, "%s (client_connect): Couldn't discard "
					"authorization data.\n", progname);
			perror("(client_connect)");
#endif
			close(client_message_socket);
			free(auth_buffer);
			return -1;
		}
		free(auth_buffer);
	}

	return client_message_socket;
}
