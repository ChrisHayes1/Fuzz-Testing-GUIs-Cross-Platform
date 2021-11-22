#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>

#include "Interface.h"
#include "Agent.h"
#include "Logger.h"
#include "_functions.h"
#include "_globals.h"
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
int parse_command_line(int argc, char *argv[ ]);
void usage();
void install_sigint_handler(void);
void install_sigquit_handler(void);
/* -------------------------------------------------------------- */


int main(int argc, char *argv[ ])
{
    enum endianness endian;
    unsigned short  major, minor;


    // Verify good args, otherwise exit
    if (parse_command_line(argc, argv) == -1) {return 1;}

    // Set interfaces to server and client
    int		client_response, xserver_response;
    int		client_socket, server_socket;
    Interface * to_client = new Interface(CLIENT, port);
    Interface * to_xserver = new Interface(XSERVER, X_PORT);

    // Additional setup
    install_sigint_handler();
    install_sigquit_handler();
    srandom(1857);

    /***
     * Setup connections
     ***/

    //Connect to client
    logger("Connecting with client\n");
    client_response = to_client->connect_client();
    //client_socket = client_connect(port, &endian, &major, &minor);
    //client_socket = client_connect(port, &endian, &major, &minor);
    if (client_response == -1)
    {
        fprintf(stderr, "(main): Can't connect to client.\n");
        return 2;
    }

//    endian = to_client->getEndianess();
//    major = to_client->getMajor();
//    minor = to_client->getMinor();


    //Connect to server
    //logger("Connecting with server\n");
    //server_socket = server_connect(endian, major, minor);
    xserver_response = to_xserver->connect_server(to_client);
    if (xserver_response == -1)
    {
        //delete to_client;
        fprintf(stderr, "%s (main): Can't connect to server.\n", progname);
        return 3;
    }

    //Agent * agent = new Agent(to_client, to_xserver);

    /****
     * Main loop
     ***/
//    if(agent->converse() < 0){
//        logger("Got response from converse of < 0\n");
//    }

    if(converse(to_client->getFD(), to_xserver->getFD()) < 0){
        logger("Got response from converse of < 0\n");
    }

    /***
     * Clean up and return
     ***/
     close(client_socket);
     close(server_socket);
    //delete agent;
    //delete to_client;
    //delete to_xserver;

    return 0;
}

/* ---------------------------------------------------------------- */

/*
 *  usage
 *
 *  Output useful usage information on stderr.
 */

void usage()
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


/* --------------------------------------------------------------- */

void signal_handler_quit(int signum)
{
    exit(7);
}

void signal_handler_sigint(int signum)
{
    exit(8);
}

/* --------------------------------------------------------------- */

void install_sigint_handler()
{
    signal(SIGINT, signal_handler_sigint);
}

void install_sigquit_handler()
{
    signal(SIGQUIT, signal_handler_quit);
}
