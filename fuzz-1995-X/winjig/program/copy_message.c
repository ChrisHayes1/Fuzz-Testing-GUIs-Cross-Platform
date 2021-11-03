/*
 *  $Id: copy_message.c,v 1.4 1995/05/01 04:56:14 ajitk Exp $
 *
 *  Implements the copy_message function which copies messages from the source
 *  socket to the destination socket.  Also provides an option to make itself
 *  (non) blocking.
 */

#include <fcntl.h>		/* network related stuff */
#include <sys/types.h>
#include <sys/socket.h>

#include <stdio.h>		/* general */
#include <errno.h>

#include "testos.h"
#include "const.h"
#include "types.h"
#include "globals.h"
#include "functions.h"

/* ------------------------------------------------------------------- */

/*
 *  copy_message
 *
 *  Block for the message if necessary.  Receive a message and copy it across.
 *  While copying across, for each chunk copied, the specified function
 *  is called.  If the function is NULL, nothing gets called.
 */

int copy_message(int dest_socket, int source_socket, enum blockstatus block,
		 int (*garbler)(int source_socket, char *buf, int *buflen_ptr,
				int max_buflen,	unsigned int sequence))
{
	int		flags = fcntl(source_socket, F_GETFL, 0);
	int		status;
	char		buffer[BUFFER_SIZE];
	int		recv_length;
	unsigned int	sequence = 0;

	/*
	 *  Set blocking status.
	 */
	
	if (flags == -1)
	{
#ifdef ERROR_MESSAGES
		fprintf(stderr, "%s (copy_message): Couldn't get flags of "
				"source socket.\n", progname);
		perror("(copy_message)");
#endif
		return -1;
	}

	switch (block)
	{
	case NON_BLOCKING:
#ifdef	HPUX
		status = fcntl(source_socket, F_SETFL, O_NDELAY);
#else
		status = fcntl(source_socket, F_SETFL, FNDELAY);
#endif
		break;

	case BLOCKING:
#ifdef	HPUX
		status = fcntl(source_socket, F_SETFL, flags & ~O_NDELAY);
#else
		status = fcntl(source_socket, F_SETFL, flags & ~FNDELAY);
#endif
		break;

	default:
#ifdef ERROR_MESSAGES
		fprintf(stderr, "%s (copy_message): Invalid blocking status.\n",
			progname);
#endif
		return -1;
	}

	if (status == -1)
	{
#ifdef ERROR_MESSAGES
		fprintf(stderr, "%s (copy_message): Can't set blocking status.\n",
			progname);
		perror("(copy_message)");
#endif
		return -1;
	}

	/*
	 *  Recv first message.
	 */
	errno = 0;
	recv_length = recv(source_socket, buffer, BUFFER_SIZE, 0);
	if (recv_length == -1)
	{
		if (block == BLOCKING  ||  errno != EWOULDBLOCK)
		{
#ifdef ERROR_MESSAGES
			fprintf(stderr, "%s (copy_message): Couldn't recv "
					"message from source.\n", progname);
			perror("(copy_message)");
#endif
			fcntl(source_socket, F_SETFL, flags); /* this could clobber errno */
			return -3;
		}
	}
	if (recv_length == 0)	/* EOF on source socket */
	{
		fcntl(source_socket, F_SETFL, flags);
		return 0;
	}

	/*
	 *  Set the source socket to non blocking.  Continue copying messages
	 *  until EWOULDBLOCK.  That way we exhaust what is currently available
	 *  even if it would not fit into the buffer in one go.
	 */
#ifdef	HPUX
	if (block == BLOCKING  &&  fcntl(source_socket, F_SETFL, O_NDELAY) == -1)
#else
	if (block == BLOCKING  &&  fcntl(source_socket, F_SETFL, FNDELAY) == -1)
#endif
	{
#ifdef ERROR_MESSAGES
		fprintf(stderr, "%s (copy_message): Unable to make source "
				"socket non-blocking.\n", progname);
		perror("(copy_message)");
#endif
		fcntl(source_socket, F_SETFL, flags);
		return -1;
	}

	if (garbler != NULL)
		if ((*garbler)(source_socket, buffer, &recv_length,
			       BUFFER_SIZE, sequence++) == -1)
		{
#ifdef ERROR_MESSAGES
			fprintf(stderr, "%s (copy_message): Garbler returned "
					"error.  Sequence = 0.\n", progname);
#endif
			fcntl(source_socket, F_SETFL, flags);
			return -5;
		}

	
	if (send(dest_socket, buffer, recv_length, 0) < recv_length)
	{
#ifdef ERROR_MESSAGES
		fprintf(stderr, "%s (copy_message): Can't send message of "
			"length %d\n", progname, recv_length);
		perror("(copy_message)");
#endif
		fcntl(source_socket, F_SETFL, flags);
		return -2;
	}

	errno = 0;
	while ((recv_length = recv(source_socket, buffer, BUFFER_SIZE, 0)) > 0)
	{
		if (garbler != NULL)
			if ((*garbler)(source_socket, buffer, &recv_length,
				       BUFFER_SIZE, sequence++) == -1)
			{
#ifdef ERROR_MESSAGES
				fprintf(stderr, "%s (copy_message): Garbler "
						"returned error.  Sequence = "
						"%d.\n",
					progname, sequence - 1);
#endif
				fcntl(source_socket, F_SETFL, flags);
				return -5;
			}

		if (send(dest_socket, buffer, recv_length, 0) < recv_length)
		{
#ifdef ERROR_MESSAGES
			fprintf(stderr, "%s (copy_message): Can't send "
					"message length %d.\n",
				progname, recv_length);
			perror("(copy_message)");
#endif
			fcntl(source_socket, F_SETFL, flags);
			return -2;
		}
	}
		
	if (recv_length == -1  &&  errno != EWOULDBLOCK)
	{
#ifdef ERROR_MESSAGES
		fprintf(stderr, "%s (copy_message): Couldn't recv message.\n",
			progname);
		perror("(copy_message)");
#endif
	}
		
	fcntl(source_socket, F_SETFL, flags);

	if (recv_length == -1  &&  errno != EWOULDBLOCK)
		return -3;
	else if (recv_length == 0)
		return 0;
	else
		return 1;
}
