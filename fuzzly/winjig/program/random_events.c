/*
 *  $Id: random_events.c,v 1.3 1995/05/01 04:55:33 ajitk Exp $
 *
 *  Implements the random_events function which generates random keyboard
 *  and mouse events.
 */

#include <assert.h>
#include <string.h>
#include <X11/X.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>

#include "testos.h"
#include "functions.h"
#include "const.h"

/* -------------------------------------------------------------------- */

/*
 *  random_events
 *
 *  Intercept all messages from the server to the client.  These messages can
 *  be one of replies, events, or errors.  We know that errors have opcode 0,
 *  replies have opcode 1, and events have opcode 2-34.  We also know that
 *  events and errors have lengths of 32 bytes each.  Replies have a
 *  length field which allows for a length of more than 32 bytes.  Each of
 *  these messages except the KeymapNotify event has a sequence number.
 *  We record this sequence number so that we can put it in our own random
 *  events.  We wait for a message boundary (message boundaries can be
 *  detected because we know the message formats and lengths).  We then decide
 *  whether to insert a random event.
 *  The opcode, the timestamp, and the sequence number in the random event
 *  are valid.  The rest of the event is random.
 */

int random_events(int source_socket, char *buf, int *len_ptr, int max_len,
		  unsigned int sequence)
{
	/*
	 *  We need to keep the current sequence number in the messages from
	 *  the server to the client.
	 */
	static unsigned short	sequence_number;
	unsigned short		temp_sequence_number;

	/*
	 *  KLUDGE: When the server replies to the connect request,
	 *  the packet is of type reply but its length field is at
	 *  position 6,7 instead of 4,5,6,7 (this is disgusting).  So,
	 *  we have to detect this first message and look at the proper
	 *  position.  This flag is set to true initially.  This indicates
	 *  that we are processing the first message.  We then set it
	 *  to false so that subsequent messages are processed normally.
	 */
	static bool		first_time = true;

	/*
	 *  TCP doesn't respect message boundaries.  We may get a portion
	 *  of a message at a time.  We accumulate the bytes recved until
	 *  we get atleast 32 bytes.
	 */
	static unsigned int	n_bytes_recved;
	static char		bytes_recved[MESSAGE_SIZE];

	int			flags;
	int			reqd_length;
	unsigned int		length_increase;
	int			opcode;
	int			length = 0;
	
	/*
	 *  Since the recvs are non-blocking, we may be called even when there
	 *  is no message.  Check this case and get out.
	 */
    fprintf(stderr, "!!! THB - ENTERING RANDOM EVENT\n");
	if (*len_ptr <= 0)
	{
		if (n_bytes_recved == 0)
		{
			length_increase = insert_random_event(buf, 
				0, sequence_number);
			*len_ptr = length_increase;
		}
		return 0;
	}

	/*
	 *  TCP is not message oriented.  We may get less than a message's
	 *  worth.  Save whatever we have got.
	 */
	if (n_bytes_recved + *len_ptr < MESSAGE_SIZE)
	{
		memcpy(bytes_recved + n_bytes_recved, buf, *len_ptr);
		n_bytes_recved += *len_ptr;
		return 0;
	}

	/*
	 *  We have got enough bytes.  If it is a KeymapNotify event that
	 *  we have got, all we know is the message boundary.  We can insert
	 *  a random event there if we wish.  For other events, errors, and
	 *  replies that have a length field of 0, we copy the sequence number
	 *  and since we know the message boundary (message size = 32 bytes)
	 *  we can insert a random event if we wish.  For replies with length
	 *  field > 0, we get the remaining stuff (possibly by recving from
	 *  source_socket) to get the message boundary.
	 */
	memcpy(bytes_recved + n_bytes_recved, buf, MESSAGE_SIZE - n_bytes_recved);
	opcode = bytes_recved[0];

	if (opcode == KeymapNotify  ||  opcode > LASTEvent)
	{
		char *buf_end = buf + (MESSAGE_SIZE - n_bytes_recved);
		
		assert(*len_ptr - (MESSAGE_SIZE - (int) n_bytes_recved) >= 0);
		if (*len_ptr - (MESSAGE_SIZE - n_bytes_recved) > 0)
		{
			memcpy(bytes_recved, buf_end,
			       *len_ptr - (MESSAGE_SIZE - n_bytes_recved));
			n_bytes_recved = MESSAGE_SIZE - n_bytes_recved;
		}

		length_increase = insert_random_event(buf_end, 
			*len_ptr - (MESSAGE_SIZE - n_bytes_recved),
			sequence_number);
		*len_ptr += length_increase;
		return 0;
	}

	memcpy(&temp_sequence_number, bytes_recved + 2, sizeof(unsigned short));
	sequence_number = (sequence_number < temp_sequence_number) 
				? temp_sequence_number 
				: sequence_number;

	if (opcode == 1) {
        if (first_time) {
            first_time = false;
            length = *((unsigned short *) (bytes_recved + 6)) - 6;
            /*
             *  Reason for the 6: Length field is 8 + .../4
             *  but actual length in 4 byte units is (8 + ...)/4.
             *  (8 - 8/4) == 6.
             */
        } else {
            length = *((unsigned *) (bytes_recved + 4));
        }
    }

	if (opcode != 1  ||  (opcode == 1  &&  length == 0))
	{
		char *buf_end = buf + (MESSAGE_SIZE - n_bytes_recved);

		assert(*len_ptr - (MESSAGE_SIZE - (int) n_bytes_recved) >= 0);
		if (*len_ptr - (MESSAGE_SIZE - n_bytes_recved) > 0)
		{
			memcpy(bytes_recved, buf_end,
			       *len_ptr - (MESSAGE_SIZE - n_bytes_recved));
			n_bytes_recved = MESSAGE_SIZE - n_bytes_recved;
		}

		length_increase = insert_random_event(buf_end, 
			*len_ptr - (MESSAGE_SIZE - n_bytes_recved),
			sequence_number);
		*len_ptr += length_increase;

		return 0;
	}

	length *= 4;

	if (length == *len_ptr - (MESSAGE_SIZE - n_bytes_recved))
	{
		/*
		 *  We have just enough bytes for the entire reply.
		 *  Discard the reply from the internal buffer.
		 */
		n_bytes_recved = 0;
		length_increase = insert_random_event(buf + *len_ptr,
			0, sequence_number);
		*len_ptr += length_increase;
		return 0;
	}

	if (length < *len_ptr - (MESSAGE_SIZE - n_bytes_recved))
	{
		char *buf_end = buf + (MESSAGE_SIZE - n_bytes_recved);
		
		memcpy(bytes_recved, buf_end + length,
		       n_bytes_recved =
		       *len_ptr - length - (MESSAGE_SIZE - n_bytes_recved));
		length_increase = insert_random_event(buf_end + length,
			*len_ptr - length - (MESSAGE_SIZE - n_bytes_recved),
			sequence_number);
		*len_ptr += length_increase;
		return 0;
	}

	/*
	 *  We don't have enough data to discard.  We do a blocking recv.
	 *  of the remaining bytes.  We attach the remaining bytes to buf.
	 */
	flags = fcntl(source_socket, F_GETFL, 0);
	if (flags == -1)
		return -1;
#ifdef	HPUX
	if (fcntl(source_socket, F_SETFL, flags & ~O_NDELAY) == -1)
#else
	if (fcntl(source_socket, F_SETFL, flags & ~FNDELAY) == -1)
#endif
		return -1;

	reqd_length = length - (*len_ptr - (MESSAGE_SIZE - n_bytes_recved));

	while (reqd_length > 0)
	{
		int new_length;
		
		if ((new_length = recv(source_socket,
				       buf + *len_ptr, reqd_length, 0)) <= 0)
		{
			fcntl(source_socket, F_SETFL, flags);
			return -1;
		}
		reqd_length -= new_length;
		*len_ptr += new_length;
	}

	if (fcntl(source_socket, F_SETFL, flags) == -1)
		return -1;

	n_bytes_recved = 0;

	length_increase = insert_random_event(buf + *len_ptr, 0,
					      sequence_number);
	*len_ptr += length_increase;
	return 0;
}
