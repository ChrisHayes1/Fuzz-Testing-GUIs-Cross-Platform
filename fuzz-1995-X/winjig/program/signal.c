/*
 *  $Id: signal.c,v 1.2 1995/05/01 04:55:50 ajitk Exp $
 *
 *  Sigint and Sigquit handler.  Also the installing functions.
 */

#include <signal.h>
#include <stdlib.h>

/* --------------------------------------------------------------- */

void signal_handler(void)
{
	exit(7);
}

/* --------------------------------------------------------------- */

void install_sigint_handler(void)
{
	signal(SIGINT, signal_handler);
}

void install_sigquit_handler(void)
{
	signal(SIGQUIT, signal_handler);
}
