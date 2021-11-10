/*
 *  $Id: macros.h,v 1.1 1994/11/23 05:15:56 ajitk Exp $
 *
 *  Declares some useful macros.
 */

#ifndef _MACROS_H
#define _MACROS_H

/* --------------------------------------------------------------------- */

/*
 *  NEAREST_MULTIPLE_OF_4
 *
 *  Gets the nearest multiple of 4 of a given number.
 */

#define NEAREST_MULTIPLE_OF_4(x)		\
	(((x)%4 == 0) ? (x) : (x)  +  4 - (x)%4)

/* --------------------------------------------------------------------- */

#endif  /* _MACROS_H */
