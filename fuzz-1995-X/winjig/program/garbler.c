/*
 *  $Id: garbler.c,v 1.3 1995/05/01 04:56:39 ajitk Exp $
 *
 *  The garbling function.
 */

#include <stdlib.h>
#include <stdio.h>

#include "globals.h"

/* -------------------------------------------------------------- */

/*
 *  global_sequence, first_time
 *
 *  The garbler keeps track of how many times it has been called.  It
 *  will not garble before startgap, and will garbler only once every rate
 *  times.
 */

static int global_sequence;
static int first_time;

/* -------------------------------------------------------------- */

/*
 *  garbler
 *
 *  Currently the garbler randomly modifies parts of the message.  It may
 *  also delete part of the end of the message or append some junk
 *  to the message.
 */

int garbler(int sock, char *buf, int *len_ptr, int max_len, unsigned int seq)
{
	if (first_time < startgap)
	{
		first_time++;
		return 0;
	}
	
	if (*len_ptr != -1)
	{
		if (global_sequence++ == rate)
		{
			int i;
			global_sequence = 0;

			/*
			 *  Change around 8 bytes.
			 */
			for (i = 0; i < 8 + random() % 4 - 2; ++i)
				buf[random() % *len_ptr] = (char) (random() % 256);

			/*
			 *  Change *len_ptr slightly.  This will either shave
			 *  off part of the end of the message or append some
			 *  junk to it.
			 */
			*len_ptr += random() % 8 - 4;
			if (*len_ptr < 0)
				*len_ptr = 0;
			else if (*len_ptr > max_len)
				*len_ptr = max_len;
		}
	}

	return 0;
}
