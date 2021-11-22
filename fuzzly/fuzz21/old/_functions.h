/*
 *  $Id: functions.h,v 1.3 1994/12/16 05:00:26 ajitk Exp $
 *
 *  Prototypes of functions which are available.
 */


#ifndef _FUNCTIONS_H
#define _FUNCTIONS_H

#include <string>
#include "_types.h"
#include "../Interface.h"

using namespace std;

/* ------------------------------------------------------------------- */

/*
 *  install_sigint_handler
 *
 *  Description:
 *  ------------
 *	Installs a handler which calls exit() when SIGINT is received.
 *	All open sockets are closed automatically.
 */

//void install_sigint_handler(void);
//
//
///*
// *  install_sigquit_handler
// *
// *  Description:
// *  ------------
// *	Same as above but installs SIGQUIT handler.
// */
//
//void install_sigquit_handler(void);

/* ------------------------------------------------------------------- */

/*
 *  client_connect
 *
 *  Description:
 *  ------------
 *	Waits for a connection from a client. Retains the endianness and
 *	protocol version information.  Discards the authorization information.
 *
 *  Arguments:
 *  ----------
 *	Name		Description
 *	----		-----------
 *	port		The port at which the program listens for connect
 *			requests from clients.  Should be between X_PORT
 *			and X_PORT + X_LIMIT.
 *	endian_ptr	The endianness info. that the client sends is stored
 *			in *endian_ptr
 *	protocol_major_version_ptr
 *	protocol_minor_version_ptr
 *			The major and minor versions of the protocol the client
 *			expects are stored in *protocol_major_version_ptr and
 *			*protocol_minor_version_ptr respy.
 *
 *  Return:
 *  -------
 *	The socket for communicating with the client on success, -1 on error.
 */

//int client_connect(int port, enum endianness *endian_ptr,
//		   unsigned short *protocol_major_version_ptr,
//		   unsigned short *protocol_minor_version_ptr);

/* --------------------------------------------------------------------- */

/*
 *  server_connect
 *
 *  Description:
 *  ------------
 *	Establishes connection with the actual X server at port X_PORT.  Sends
 *	the clients endianness and protocol version info. along with a
 *	concocted authorization string.  Connects to the server via Internet
 *	protocols and allows for MIT Magic Cookie authorization only.
 *
 *  Arguments:
 *  ----------
 *	Name		Description
 *	----		-----------
 *	endian		Endianness info that the client sent
 *	protocol_major_version
 *	protocol_minor_version
 *			Major and minor revisions of the protocol that the
 *			client expects.
 *
 *  Return:
 *  -------
 *	Socket to communicate with the actual X server on success, -1 on error.
 */

//int server_connect(enum endianness endian,
//		   unsigned short protocol_major_version,
//		   unsigned short protocol_minor_version);

/* ------------------------------------------------------------------- */

/*
 *  copy_message
 *
 *  Description:
 *  ------------
 *	Copies a message from the source socket to the destination socket.
 *	A message is defined as a set of data recved beyond which a blocking
 *	would result.  Since the buffer is fixed size, many recv may have to be
 *	done before all the data is obtained.  The first recv may be blocking
 *	or non blocking as decided by the block parameter.  The remaining
 *	recv's to receive this block are all non-blocking.  For each block
 *	recved, a garbling function (if supplied) is called.  This function
 *	may delete/insert/change bits in the message.  The block is then sent
 *	to its destination.
 *
 *  Arguments:
 *  ----------
 *	Name		Description
 *	----		-----------
 *	dest_socket	Destination of recved messages
 *	source_socket	Source of the messages.
 *	block		Specifies whether the function should block for
 *			the first recv.
 *	garbler		Function which is called for each block recved.  This
 *			function can add/delete/change bits in the recved
 *			block.
 *			Arguments:
 *				source_socket	Socket from which to read
 *						messages.
 *				buf		the recved block (this may be
 *						changed by the function).
 *				buflen_ptr	a value return.  The initial
 *						length of the buffer is sent in.
 *						The final length of the buffer
 *						is sent out by the function.
 *				max_buflen	the function cannot insert
 *						bits such that the length of
 *						buffer exceeds max_buflen.
 *				sequence	count of the number of blocks
 *						processed during this call of
 *						copy_message.
 *			Return:
 *				0 on success, -1 to abort copy_message.
 *
 *  Return:
 *  -------
 *	1	data copied successfully.
 *	0	source socket has reached EOF.
 *	-1	some error occurred.
 *	-2	some error occurred during a send operation.
 *	-3	some error occurred during a recv operation.
 *	-4	non-blocking recv requested and nothing turned up.
 *	-5	garbler asked for an abort.
 */

//int converse(int client_socket, int server_socket);
//int copy_msgs(int source_socket,int dest_socket);
//int copy_message(int dest_socket, int source_socket, enum blockstatus block,
//		 int (*garbler)(int source_socket, char *buf, int *buflen_ptr,
//				int max_buflen, unsigned int sequence));

/* ------------------------------------------------------------------- */

/*
 *  send_random_message
 *
 *  Description:
 *  ------------
 *	Generate and send a random message.
 *
 *  Arguments:
 *  ----------
 *	Name		Description
 *	----		-----------
 *	dest_socket	Socket which recves the message.
 *	gen_msg		Function which is called to generate the message to
 *			send.  If NULL, a completely random message of random
 *			length is sent.
 *			Arguments:
 *				buf	the buffer in which the generated msg
 *					must be placed.
 *				len_ptr	value return.  The length of buf is
 *					sent in.  The length of the generated
 *					message is sent out.
 *		        Return:
 *				0 on successful generation, -1 if
 *				send_random_message must be aborted.
 *
 *  Return:
 *  -------
 *	0	success
 *	-1	gen_msg requested abort.
 *	-2	send operation failed.
 */

//int send_random_message(int dest_socket, int (*gen_msg)(char *buf, int *len_ptr));
//
///* --------------------------------------------------------------------- */
//
//int insert_random_event(char * b, unsigned int len, int seq);
//
///* --------------------------------------------------------------------- */
//
//
//
//void logger(string msg_log, LogMode log_mode=BOTH);

#endif  /* _FUNCTIONS_H */
