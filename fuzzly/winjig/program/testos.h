/*
 *  $Id: testos.h,v 1.1 1995/05/01 00:28:04 ajitk Exp $
 *
 *  This header file defines a unique name for different OSes after
 *  checking through various machine and compiler dependent names to
 *  find the right one.  This standard name is used in other parts of
 *  the program.
 */

#ifndef _TESTOS_H
#define _TESTOS_H

/* ---------------------------------------------------------------- */

/*
 *  HP/UX is defined by hpux in GCC and __hpux by DEC cc.
 */

#if defined(hpux)  ||  defined(__hpux)
#	define	HPUX
#endif

/* ---------------------------------------------------------------- */

#endif  /* _TESTOS_H */
