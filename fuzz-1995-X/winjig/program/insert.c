/*
 *  $Id: insert.c,v 1.4 1995/05/01 04:54:34 ajitk Exp $
 *
 *  insert_random_event()
 *  This function inserts a random event into the supplied buffer.
 *  The correct sequence number is set.  Further, insertion is 
 *  itself performed randomly.  If insertion is done, the number of bytes
 *  inserted is returned, else 0.
 *
 *  All events are 32 bytes long.  The first byte is a event code which
 *  is 2-34. The bytes 3 and 4 are the sequence number.
 *  The next four bytes are the timestamp which is the integer 
 *  returned by the time() system call.
 *  All the other fields are set to random values.
 */

#include <sys/types.h>
#include <sys/time.h>
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>

#include "globals.h"

/* --------------------------------------------------------------- */

/*
 *  Constants used in insert_random_event.
 */


/*
 *  OPCODE_LIMIT
 *
 *  Keyboard and mouse events have opcodes in the range 2 to 5.
 *  OPCODE_LIMIT is used to limit the opcode to that range if keyboard
 *  and mouse events alone are selected.
 *
 */

#define OPCODE_LIMIT 4


/*
 *  CHAR_LIMIT
 *
 *  Largest value + 1 to generate for each byte in the random event.
 *  Basically, UCHAR_MAX + 1.
 */

#define CHAR_LIMIT 256


/*
 *  WINDOW_LIMIT
 *
 *  Every event has a window identifier.  This is randomly generated
 *  in insert_random_event.  WINDOW_LIMIT is the maximum value of the
 *  (random) window ID that insert_random_event will generate.
 */

#define WINDOW_LIMIT 10

/* --------------------------------------------------------------- */

/*
 *  insert_random_event
 *
 *  Generate a random event.  Will not generate an event until startgap.
 *  Will generate an event, approx., once every rate times.  Depending
 *  on mode, it will generate any opcode, or opcodes in the range 2-5.
 */

unsigned int insert_random_event(char *b, unsigned int n_bytes_to_move, 
			         unsigned short seq)
{
	long		i;
	time_t		timeval;
	static int	count = 0;

	if (count < startgap)
	{
		count++;
		return 0;
	}

	if ((random() % rate) != 0)
		return 0;

	/*
	 *  first move 32 bytes over to create space for the new event.
	 */

	if (n_bytes_to_move != 0)
	{
		if (n_bytes_to_move  <= 32)
			memcpy(&b[32], &b[0], n_bytes_to_move);
		else
		{
			printf("MEMCPY PROBLEMS\n");
			memcpy(&b[32], &b[0], n_bytes_to_move);
		}
	}
  

	/* byte 0 is opcode. */
	if (mode == 3)
		b[0] = (char) (random() % OPCODE_LIMIT + 2);
	else
		b[0] = (char) (random() % 33 + 2);

	/*
	 *  byte 1 depends on what b[0] is.
	 *  if b[0] == 2 or 3 (keyboard)
	 *      b[1] = KEYCODE (8-255)
	 *  else 
	 *      b[1] = BUTTON (1-3)
	 */

	if (b[0] == 2  ||  b[0] == 3)
		b[1] = (char)(random() % 248 + 8);
	else
		b[1] = (char)(random() % 3 + 1);

	/* bytes 2 and 3 contain the last 16 bits of the seq. number */
	memcpy(&b[2], &seq, sizeof(unsigned short));

	/* The next four bytes contain the timestamp */
	timeval = time(0);
	memcpy((void *)&b[4], (int *)&timeval, sizeof(int));

	/* There are three window fields */

	for (i = 0; i < 3; i++)
	{
		int win;
		win = random() % WINDOW_LIMIT;
		memcpy(&b[8 + i * 4], &win, sizeof(int));
	}


	/* The rest of the buffer are all arbitrary characters */

	for (i = 20; i < 32; i++)
		b[i] = (char) (random() % CHAR_LIMIT);
  
	return 32;
}
