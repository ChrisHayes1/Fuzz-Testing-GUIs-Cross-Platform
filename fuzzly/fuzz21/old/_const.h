/*
 *  $Id: const.h,v 1.3 1994/12/16 05:00:26 ajitk Exp $
 *
 *  Global constants used by the crasher program.
 */

#ifndef _CONST_H
#define _CONST_H

/* ------------------------------------------------------------------ */

/*
 *  X_PORT
 *
 *  Port on which the X server listens.
 */

//#define	X_PORT		6000


/*
 *  X_LIMIT
 *
 *  Limit of last port in which our server listens.
 */

//#define X_LIMIT		9

/* ------------------------------------------------------------------ */

/*
 *  MESSAGE_SIZE
 *
 *  Granularity of message recvs.  This is fixed at the size of events/errors.
 *  Do NOT change.
 */

//#define MESSAGE_SIZE	32

/* ------------------------------------------------------------------ */

/*
 *  BUFFER_SIZE
 *
 *  Size of buffer to recv messages.  The actual recv granularity is
 *  MESSAGE_SIZE.  But the random_events function may want to add stuff
 *  to the buffer before sending it out.  There is ample slack.
 */

//#define	BUFFER_SIZE	16384

//#define CLIENT  0
//#define XSERVER  1

/* ------------------------------------------------------------------ */

#endif  /* _CONST_H */
