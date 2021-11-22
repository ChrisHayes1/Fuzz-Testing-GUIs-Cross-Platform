/*
 *  $Id: randmsg.c,v 1.2 1995/05/01 04:54:56 ajitk Exp $
 *
 *  Implements the send_random_message function.
 */

#include <sys/types.h>
#include <sys/socket.h>

#include <stdio.h>
#include <stdlib.h>

#include "../../fuzz21/old/_const.h"
#include "globals.h"
#include "functions.h"

/* ------------------------------------------------------------------ */

/*
 *  send_random_message
 *
 *  Calls the supplied random message generator to generate a message.
 *  If no message generator is supplied, this function will generate a
 *  completely random message of random length.  It will then send the message
 *  to the dest_socket.
 */

int send_random_message(int dest_socket, int (*gen_msg)(char *buf, int *len_ptr))
{
	char buffer[BUFFER_SIZE];
	int  buflen = BUFFER_SIZE;

	if (gen_msg != NULL)
	{
		if ((*gen_msg)(buffer, &buflen) == -1)
		{
#ifdef ERROR_MESSAGES
			fprintf(stderr, "%s (send_random_message): Message "
					"generator returned error.\n",
				progname);
#endif
			return -1;
		}
	}
	else
	{
		/*
		 *  Generate completely random message of random length.
		 */
		int i;
		
		buflen = (int) (random() % BUFFER_SIZE);

		for (i = 0; i < buflen; ++i)
			buffer[i] = (char) (random() % 256);
	}

	if (send(dest_socket, buffer, buflen, 0) < buflen)
	{
#ifdef ERROR_MESSAGES
		fprintf(stderr, "%s (send_random_message): Can't send message "
				"of length %d bytes.\n", progname, buflen);
		perror("(send_random_message)");
#endif
		return -2;
	}

	return 0;
}
