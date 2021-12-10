#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>

#include "Interface.h"
#include "Agent.h"
#include "Logger.h"

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

int seed = -1;


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

int mod_rate = 4;
int inj_rate = 10; //seconds

/*
# garble_mode
#   0 - passthrough
#   1 - garble
#   2 - kill_seq
#   3 - kill_length

# inj_mode
#   0 - passthrough
#   1 - noise
#   2 - events
#   3 - kmevents
#   4 - replay
#   5 - mixed (replay + events);
 */

int inj_mode = 0;
int mod_mode = 0;

/* -------------------------------------------------------------- */
int parse_command_line(int argc, char *argv[ ]);
void usage();
void install_sigint_handler(void);
void install_sigquit_handler(void);
/* -------------------------------------------------------------- */


int main(int argc, char *argv[ ])
{
    // Verify good args and parse, otherwise exit
    if (parse_command_line(argc, argv) == -1) {return 1;}
    // Randomize

    if (seed != -1){
        srand(seed);
        logger("Randomized via seed\n");
    } else {
        time_t t;
        srand((unsigned) time(&t));
        logger("Randomized via time\n");
    }


    // int		client_response, xserver_response;
    Interface * to_client = new Interface(CLIENT, port);
    Interface * to_xserver = new Interface(XSERVER, X_PORT);

    // Additional setup
    install_sigint_handler();
    install_sigquit_handler();


    /***
     * Setup connections
     ***/

    //Connect to client
    logger("Connecting with client\n");
//    client_response = to_client->connect_client();
    if (to_client->connect_client() == -1)
    {
        fprintf(stderr, "(main): Can't connect to client.\n");
        return 2;
    }

    //Connect to server
    logger("Connecting with server\n");
//    xserver_response = to_xserver->connect_server(to_client);
    if (to_xserver->connect_server(to_client) == -1)
    {
        //delete to_client;
        fprintf(stderr, "%s (main): Can't connect to server.\n", progname);
        return 3;
    }


    /****
     * Main loop - Converse passes messages until error or exit
     ***/
    logger("Passing Messages\n");
    Agent * agent = new Agent(to_client, to_xserver);
    agent->set_inj_mode(inj_mode);
    agent->set_inj_rate(inj_rate);
    agent->set_mod_mode(mod_mode);
    agent->set_mod_rate(mod_rate);
    agent->set_startgap(startgap);

    int resp = agent->converse();
    if( resp < 0){
        switch (resp){
            case 0:
                logger("Converse() reported a normal return\n");
                break;
            case -1:
                logger("Converse() reported that poll returned an error\n");
                break;
            case -2:
                logger("Converse() reported that recv failed\n");
                break;
            case -3:
                logger("Converse() reported that the client exited gracefully\n");
                break;
            case -4:
                logger("Converse() reported that poll.revents returned unexpected value\n");
                break;
        }


        // -1 -> pull returned an error
        // -2 -> recv from source failed - xfer msg
        // -3 -> recv got 0, source gracefully failed
        // -4 -> revents returned unexpected value
    }


    /***
     * Clean up and return
     ***/
     //close(client_socket);
     //close(server_socket);
    delete agent;
    delete to_client;
    delete to_xserver;

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
                    "       [-port <port>] [-seed <seed>]\n",
            progname);
    fprintf(stdout, "       %s -h[elp]\n", progname);
    fprintf(stdout, "       <mod_mode> = garble, kill_seq, kill_length, passthrough\n");
    fprintf(stdout, "       <mod_rate>      = # unaltered messages between modifier\n");
    fprintf(stdout, "       <inj_mode>    = noise, events, kmevents, replay, mixed, passthrough\n");
    fprintf(stdout, "       <inj_rate>  = # milliseconds between injections\n");
    fprintf(stdout, "       <startgap>  = # unaltered messages before first operation\n");
    fprintf(stdout, "       <port>      = port on which Winjig listens\n");
//    fprintf(stdout, "       <direction> = s2c, c2s, both\n");
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
        if (strcmp(&argv[i][1], "mod_mode") == 0)
        {
            if (i + 1 == argc)
            {
                fprintf(stderr, "%s (parse_command_line): "
                                "mode argument missing.\n", progname);
                usage();
                return -1;
            }

            if (strcmp(argv[i + 1], "passthrough") == 0)
                mod_mode = 0;
            else if (strcmp(argv[i + 1], "garble") == 0)
                mod_mode = 1;
            else if (strcmp(argv[i + 1], "kill_length") == 0)
                mod_mode = 2;
            else if (strcmp(argv[i + 1], "kill_seq") == 0)
                mod_mode = 3;
            else
            {
                fprintf(stderr, "%s (parse_command_line): "
                                "Invalid mode argument (%s).\n",
                        progname, argv[i + 1]);
                usage();
                return -1;
            }
            i += 2;
        } else if (strcmp(&argv[i][1], "inj_mode") == 0) {
            if (i + 1 == argc)
            {
                fprintf(stderr, "%s (parse_command_line): "
                                "mode argument missing.\n", progname);
                usage();
                return -1;
            }

            if (strcmp(argv[i + 1], "passthrough") == 0)
                inj_mode = 0;
            else if (strcmp(argv[i + 1], "noise") == 0)
                inj_mode = 1;
            else if (strcmp(argv[i + 1], "events") == 0)
                inj_mode = 2;
            else if (strcmp(argv[i + 1], "kmevents") == 0)
                inj_mode = 3;
            else if (strcmp(argv[i + 1], "replay") == 0)
                inj_mode = 4;
            else if (strcmp(argv[i + 1], "mixed") == 0)
                inj_mode = 5;
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
        else if (strcmp(&argv[i][1], "mod_rate") == 0)
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
            mod_rate = atoi(argv[i + 1]);
            i += 2;
        }
        else if (strcmp(&argv[i][1], "inj_rate") == 0)
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
            inj_rate = atoi(argv[i + 1]);
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
//        else if (strcmp(&argv[i][1], "direction") == 0)
//        {
//            if (i + 1 == argc)
//            {
//                fprintf(stderr, "%s (parse_command_line): "
//                                "direction argument missing.\n",
//                        progname);
//                usage();
//                return -1;
//            }
//
//            if (strcmp(argv[i + 1], "s2c") == 0)
//                direction = 0;
//            else if (strcmp(argv[i + 1], "c2s") == 0)
//                direction = 1;
//            else if (strcmp(argv[i + 1], "both") == 0)
//                direction = 2;
//            else
//            {
//                fprintf(stderr, "%s (parse_command_line): "
//                                "direction argument (%s) wrong.\n",
//                        progname, argv[i + 1]);
//                usage();
//                return -1;
//            }
//            i += 2;
//        }
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
    cout << "SIG QUIT" << endl;
    exit(7);
}

void signal_handler_sigint(int signum)
{
    cout << "SIG INT" << endl;
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
