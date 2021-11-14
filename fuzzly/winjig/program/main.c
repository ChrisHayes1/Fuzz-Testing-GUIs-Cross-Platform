/*
 *  $Id: main.c,v 1.3 1995/05/01 04:53:56 ajitk Exp $
 *
 *  Establish a connection with a client.  Then establish contact with the
 *  server.  Send messages back and forth until one of them closes the
 *  connection.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <ctype.h>

#include "const.h"
#include "types.h"
#include "globals.h"
#include "functions.h"

/* --------------------------------------------------------------------- */

/*
 *  progname
 *
 *  Name of the program.  Useful for displaying error messages.
 */

char *progname;

/* --------------------------------------------------------------------- */

/*
 *  The garbler and injector functions.
 */
int garbler(int sock, char *buf, int *len_ptr, int max_len, 
	    unsigned int sequence);
int random_events(int sock, char *buf, int *len_ptr, int max_len, 
		  unsigned int sequence);

/* --------------------------------------------------------------------- */

/*
 *  Global variables set by command line arguments and their default
 *  values.
 */


/*
 *  seed
 *
 *  Seed for the random number generator.
 */

int seed = 1857;


/*
 *  direction
 *
 *  Direction in which garbling takes place.
 *  0 = server to client direction
 *  1 = client to server direction
 *  2 = both directions
 *  Random events go in the direction to server to client.  For random
 *  events, this parameter, if present, is ignored.
 */

int direction = 0;


/*
 *  port
 *
 *  Port on which Winjig listens.  This must be between 6001 and 6009.
 */

int port = 6001;


/*
 *  startgap
 *
 *  Number of messages that pass between the client and server before
 *  Winjig kicks in.  Apart from substituting the authorization
 *  cookie in the first packet from the client to the server, Winjig
 *  will not touch the first few messages between the server and
 *  client.  Doing so would almost certainly fail the connection request.
 */

int startgap = 100;


/*
 *  rate
 *
 *  The rate at which Winjig will garble/inject.  This is measured in
 *  number of messages that pass unharmed before Winjig garbles/injects
 *  a message.
 */

int rate = 100;


/*
 *  mode
 *
 *  What Winjig will do.
 *  0 = create random messages
 *  1 = garble messages
 *  2 = create random events
 *  3 = create random keyboard and mouse events only
 *  Currently, Winjig does not allow combinations of the above, though
 *  that wouldn't be too difficult to add.
 */

int mode = 0;

/* -------------------------------------------------------------- */

/*
 *  usage
 *
 *  Output useful usage information on stderr.
 */

void usage(void)
{
	fprintf(stderr, "Usage: %s [-mode <mode>] [-rate <rate>] "
		"[-startgap <startgap>]\n"
		"       [-port <port>] [-direction <dir> [-seed <seed>]\n",
		progname);
	fprintf(stderr, "       %s -h[elp]\n", progname);
	fprintf(stderr, "       <mode>      = messages, garbling, events, kmevents\n");
	fprintf(stderr, "       <rate>      = # unaltered messages between operations\n");
	fprintf(stderr, "       <startgap>  = # unaltered messages before first operation\n");
	fprintf(stderr, "       <port>      = port on which Winjig listens\n");
	fprintf(stderr, "       <direction> = s2c, c2s, both\n");
	fprintf(stderr, "       <seed>      = seed for random number generator\n");
}

/* -------------------------------------------------------------- */


int main(int argc, char *argv[ ])
{
	int parse_command_line(int argc, char *argv[ ]);
	
	enum endianness endian;
	unsigned short  major, minor;
	int             return_status_1, return_status_2;
	int		client_socket, server_socket;
	int		count = 0;
	int		(*server_to_client_garbler)(int source_socket,
				    char *buf, int *len_ptr,
				    int max_buflen, unsigned int sequence);
	int		(*client_to_server_garbler)(int source_socket,
				    char *buf, int *len_ptr,
				    int max_buflen, unsigned int sequence);
	int		start_garbling = 0;
	int		destination_socket;

	progname = argv[0];
	if (parse_command_line(argc, argv) == -1)
		return 1;

	install_sigint_handler();
	install_sigquit_handler();

    fprintf(stderr, "THB - Connecting with client\n");
	srandom(1857);
	client_socket = client_connect(port, &endian, &major, &minor);
	if (client_socket == -1)
	{
		fprintf(stderr, "%s (main): Can't connect to client.\n", progname);
		return 2;
	}

    fprintf(stderr, "THB - Connecting with server\n");
	server_socket = server_connect(endian, major, minor);
	if (server_socket == -1)
	{
		close(client_socket);
		fprintf(stderr, "%s (main): Can't connect to server.\n", progname);
		return 3;
	}

	if (mode == 0)		/* random messages */
	{
		server_to_client_garbler = client_to_server_garbler = NULL;
		if (direction == 0)
			destination_socket = client_socket;
		else if (direction == 1)
			destination_socket = server_socket;
	}
	else if (mode == 1)	/* garbling */
	{
		if (direction == 0)
		{
			server_to_client_garbler = garbler;
			client_to_server_garbler = NULL;
		}
		else if (direction == 1)
		{
			client_to_server_garbler = garbler;
			server_to_client_garbler = NULL;
		}
		else
		{
			client_to_server_garbler = garbler;
			server_to_client_garbler = garbler;
		}
	}
	else if (mode == 2  ||  mode == 3)	/* random events */
	{
        fprintf(stderr, "THB - Setting client garbler to random events\n");
		server_to_client_garbler = random_events;
		client_to_server_garbler = NULL;
	}


    fprintf(stderr, "THB - Entering main loop\n****************************\n");
    long int t_count = 0;
	while ((return_status_1 = copy_message(client_socket, server_socket,
		       NON_BLOCKING, server_to_client_garbler)) == 1  &&
	       (return_status_2 = copy_message(server_socket, client_socket,
		       NON_BLOCKING, client_to_server_garbler)) == 1)
	{
        fprintf(stderr, "=%ld\n", ++t_count);
		if (mode == 0)		/* random message */
		{
            fprintf(stderr, "@");
			/*
			 *  no random messages until startgap.
			 */
			if (!start_garbling  &&  count++ < startgap)
				continue;

			/*
			 *  random messages only once every rate messages.
			 */
			else if (start_garbling  &&  count++ < rate)
				continue;

			count = 0;

			if (direction == 2)
			{
				/*
				 *  Random messages in either direction.
				 *  Determine the destination randomly.
				 */
				if (random() & 0x1)
					destination_socket = client_socket;
				else
					destination_socket = server_socket;
			}
			if (send_random_message(destination_socket, NULL) != 0)
			{
				fprintf(stderr, "%s (main): Couldn't send "
						"random message to client.\n",
					progname);
				close(client_socket);
				close(server_socket);
				return 4;
			}
			printf("RANDOM MESSAGE SENT\n");
		}  /* if (mode == 0) */
	}  /* while (...) */

    fprintf(stderr, "THB - done with main loop - closing");

	if (return_status_1 == 0)
	{
		fprintf(stderr, "%s (main): Server closed connection.\n", progname);
		close(client_socket);
		close(server_socket);
		return 0;
	}
	if (return_status_2 == 0)
	{
		fprintf(stderr, "%s (main): Client closed connection.\n", progname);
		close(client_socket);
		close(server_socket);
		return 0;
	}
	if (return_status_1 < 0)
	{
		fprintf(stderr, "%s (main): Failed when copying message from "
				"server to client.  Return code = %d\n",
			progname, return_status_1);
		close(client_socket);
		close(server_socket);
		return 5;
	}
	if (return_status_2 < 0)
	{
		fprintf(stderr, "%s (main): Failed when copying message from "
				"client to server.  Return code = %d\n",
			progname, return_status_2);
		close(client_socket);
		close(server_socket);
		return 6;
	}
	
	close(client_socket);
	close(server_socket);

	return 0;
}

/* ---------------------------------------------------------------- */

/*
 *  parse_command_line
 *
 *  Go through the command line, looking for command line arguments.
 *  Set the appropriate global variable.  Output error messages/usage
 *  information as appropriate.
 */

int parse_command_line(int argc, char *argv[ ])
{
	int i = 1;

	while (i < argc)
	{
		if (argv[i][0] != '-')
		{
			fprintf(stderr, "%s (parse_command_line): Illegal option %s -- ignoring.\n",
				progname, argv[i]);
			++i;
			continue;
		}

		/*
		 *  Check against the options.
		 */
		if (strcmp(&argv[i][1], "mode") == 0)
		{
			if (i + 1 == argc)
			{
				fprintf(stderr, "%s (parse_command_line): "
					"mode argument missing.\n", progname);
				usage();
				return -1;
			}

			if (strcmp(argv[i + 1], "messages") == 0)
				mode = 0;
			else if (strcmp(argv[i + 1], "garbling") == 0)
				mode = 1;

			else if (strcmp(argv[i + 1], "events") == 0)
				mode = 2;
			else if (strcmp(argv[i + 1], "kmevents") == 0)
				mode = 3;
			else
			{
				fprintf(stderr, "%s (parse_command_line): "
					"Invalid mode argument (%s).\n",
					progname, argv[i + 1]);
				usage();
				return -1;
			}
			i += 2;
		}
		else if (strcmp(&argv[i][1], "rate") == 0)
		{
			if (i + 1 == argc)
			{
				fprintf(stderr, "%s (parse_command_line): "
					"rate argument missing.\n", progname);
				usage();
				return -1;
			}

			if (!isdigit(argv[i + 1][0]))
			{
				fprintf(stderr, "%s (parse_command_line): "
					"rate argument wrong (%s).\n",
					progname, argv[i + 1]);
				usage();
				return -1;
			}
			rate = atoi(argv[i + 1]);
			i += 2;
		}
		else if (strcmp(&argv[i][1], "startgap") == 0)
		{
			if (i + 1 == argc)
			{
				fprintf(stderr, "%s (parse_command_line): "
					"startgap argument missing.\n",
					progname);
				usage();
				return -1;
			}

			if (!isdigit(argv[i + 1][0]))
			{
				fprintf(stderr, "%s (parse_command_line): "
					"startgap argument wrong (%s).\n",
					progname, argv[i + 1]);
				usage();
				return -1;
			}
			startgap = atoi(argv[i + 1]);
			i += 2;
		}
		else if (strcmp(&argv[i][1], "port") == 0)
		{
			if (i + 1 == argc)
			{
				fprintf(stderr, "%s (parse_command_line): "
					"port argument missing.\n",
					progname);
				usage();
				return -1;
			}

			if (!isdigit(argv[i + 1][0]))
			{
				fprintf(stderr, "%s (parse_command_line): "
					"port argument wrong (%s).\n",
					progname, argv[i + 1]);
				usage();
				return -1;
			}
			port = atoi(argv[i + 1]);
			if (port <= X_PORT  ||  port > X_PORT + X_LIMIT)
			{
				fprintf(stderr, "%s (parse_command_line): "
					"port argument (%d) not in range %d..%d.\n",
					progname, port, X_PORT + 1, X_PORT + X_LIMIT);
				return -1;
			}
			i += 2;
		}
		else if (strcmp(&argv[i][1], "seed") == 0)
		{
			if (i + 1 == argc)
			{
				fprintf(stderr, "%s (parse_command_line): "
					"seed argument missing.\n",
					progname);
				usage();
				return -1;
			}

			if (!isdigit(argv[i + 1][0]))
			{
				fprintf(stderr, "%s (parse_command_line): "
					"seed argument wrong (%s).\n",
					progname, argv[i + 1]);
				usage();
				return -1;
			}
			seed = atoi(argv[i + 1]);
			i += 2;
		}
		else if (strcmp(&argv[i][1], "direction") == 0)
		{
			if (i + 1 == argc)
			{
				fprintf(stderr, "%s (parse_command_line): "
					"direction argument missing.\n",
					progname);
				usage();
				return -1;
			}

			if (strcmp(argv[i + 1], "s2c") == 0)
				direction = 0;
			else if (strcmp(argv[i + 1], "c2s") == 0)
				direction = 1;
			else if (strcmp(argv[i + 1], "both") == 0)
				direction = 2;
			else
			{
				fprintf(stderr, "%s (parse_command_line): "
					"direction argument (%s) wrong.\n",
					progname, argv[i + 1]);
				usage();
				return -1;
			}
			i += 2;
		}
		else if (strcmp(&argv[i][1], "h") == 0  ||
			 strcmp(&argv[i][1], "help") == 0)
		{
			usage();
			return -1;
		}
		else
		{
			fprintf(stderr, "%s (parse_command_line): unrecognized "
				"option %s -- ignoring.\n", progname, argv[i]);
			++i;
		}
	}  /* while (i < argc) */

	return 0;
}
