/*
 *  $Id: signal.c,v 1.2 1995/05/01 04:55:50 ajitk Exp $
 *
 *  Sigint and Sigquit handler.  Also the installing functions.
 */

#include <signal.h>
#include <stdlib.h>

/* --------------------------------------------------------------- */

void signal_handler_quit(void)
{
	exit(7);
}

void signal_handler_sigint(void)
{
	exit(8);
}

/* --------------------------------------------------------------- */

void install_sigint_handler(void)
{
	signal(SIGINT, signal_handler_sigint);
}

void install_sigquit_handler(void)
{
	signal(SIGQUIT, signal_handler_quit);
}
