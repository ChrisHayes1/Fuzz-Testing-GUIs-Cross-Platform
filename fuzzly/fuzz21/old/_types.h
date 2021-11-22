/*
 *  $Id: types.h,v 1.1 1994/11/23 05:15:56 ajitk Exp $
 *
 *  Some basic types.
 */

#ifndef _TYPES_H
#define _TYPES_H

/* ---------------------------------------------------------------- */

/*
 *  bool
 *
 *  The boolean type.
 */

//typedef enum tag_bool
//{
//	false, true
//} bool;

/* ---------------------------------------------------------------- */

/*
 *  endianness
 *
 *  Specifies whether something is little endian or big endian.  The actual
 *  values used are tailored to the value sent as part of the connection
 *  message.
 */

//enum endianness
//{
//	LITTLE_ENDIAN_NEW = 0154,
//	BIG_ENDIAN_NEW    = 0102
//};

/* ---------------------------------------------------------------- */

/*
 *  blockstatus
 *
 *  Specifies whether a (non) blocking recv should be done.
 */

//enum blockstatus
//{
//	NON_BLOCKING,
//	BLOCKING
//};

/* ---------------------------------------------------------------- */

//enum LogMode {OUT, ERR, BOTH};

#endif  /* _TYPES_H */
