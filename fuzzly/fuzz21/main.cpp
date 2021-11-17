#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include "_const.h"
#include "_types.h"
#include "_functions.h"
#include "_logger.cpp"


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
 * log
 *
 * output to stderr and stdout for terminal and file output
 * Should be used with program call that outputs stderr to file
 */

void my_log(char* my_string){
    fprintf(stderr, "%s", my_string);
    fprintf(stdout, "%s", my_string);
}

/*
 *  usage
 *
 *  Output useful usage information on stderr.
 */

void usage(void)
{
    fprintf(stdout, "Usage: %s [-mode <mode>] [-rate <rate>] "
                    "[-startgap <startgap>]\n"
                    "       [-port <port>] [-direction <dir> [-seed <seed>]\n",
            progname);
    fprintf(stdout, "       %s -h[elp]\n", progname);
    fprintf(stdout, "       <mode>      = messages, garbling, events, kmevents\n");
    fprintf(stdout, "       <rate>      = # unaltered messages between operations\n");
    fprintf(stdout, "       <startgap>  = # unaltered messages before first operation\n");
    fprintf(stdout, "       <port>      = port on which Winjig listens\n");
    fprintf(stdout, "       <direction> = s2c, c2s, both\n");
    fprintf(stdout, "       <seed>      = seed for random number generator\n");
}

/* -------------------------------------------------------------- */


int main(int argc, char *argv[ ])
{
    int parse_command_line(int argc, char *argv[ ]);

    enum endianness endian;
    unsigned short  major, minor;
    int		client_socket, server_socket;
    int		count = 0;


    progname = argv[0];
    if (parse_command_line(argc, argv) == -1)
        return 1;

    // Setup
    install_sigint_handler();
    install_sigquit_handler();
    srandom(1857);

    //Connect to client
    logger("THB - Connecting with client\n");
    client_socket = client_connect(port, &endian, &major, &minor);
    if (client_socket == -1)
    {
        fprintf(stderr, "%s (main): Can't connect to client.\n", progname);
        return 2;
    }

    //Connect to server
    logger("THB - Connecting with server\n");
    server_socket = server_connect(endian, major, minor);
    if (server_socket == -1)
    {
        close(client_socket);
        fprintf(stderr, "%s (main): Can't connect to server.\n", progname);
        return 3;
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
