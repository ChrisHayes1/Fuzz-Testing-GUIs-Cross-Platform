/*
 *  $Id: copy_message.c,v 1.4 1995/05/01 04:56:14 ajitk Exp $
 *
 *  Implements the copy_message function which copies messages from the source
 *  socket to the destination socket.  Also provides an option to make itself
 *  (non) blocking.
 */

//#include <fcntl.h>		/* network related stuff */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
//#include <sys/resource.h>
#include <sys/poll.h>
#include <cstring>
#include <stdio.h>		/* general */
#include <errno.h>

#include "_const.h"
#include "_types.h"
#include "_globals.h"
#include "_functions.h"

//Specific to verify, can remove when done with it.
#include <fcntl.h>

int copy_msgs(int source_socket, int dest_socket){
    char buffer[BUFFER_SIZE];
    int recv_length, send_length;
    logger("   Ready to recv -> ");
    recv_length = recv(source_socket, buffer, BUFFER_SIZE, 0);
    slog << "Receive msg of size " << recv_length << endl;
    logger(slog.str());
    if (recv_length < 0){
        logger("ERROR: Receive from source failed\n");
        return -2;
    }
    else if (recv_length == 0){
        logger("NOTE: Source socket gracefully failed\n", ERR);
        return -1;
    } else {
        slog << "   Sending msg of size " << recv_length << "-> ";
        logger(slog.str());
        send_length = send(dest_socket, buffer, recv_length, 0);
        if (send_length < recv_length){
            logger("ERROR: Entire message not sent\n");
        }
        logger("Message sent\n");
    }
    return 0;
}

void verify_empty(struct pollfd* fds, int nfds){
    for (int i = 0; i < nfds; i++){
        int flags;
        int	status;
        char buffer[BUFFER_SIZE];

        flags = fcntl(fds[i].fd, F_GETFL, 0);
        status = fcntl(fds[i].fd, F_SETFL, FNDELAY);
        if (status == -1){logger("Error on status\n");}
        int recv_length = recv(fds[i].fd, buffer, BUFFER_SIZE, 0);
        if (recv_length == -1){
            if (errno != EWOULDBLOCK){
                logger("Unexpected error on receive\n");
                status = fcntl(fds[i].fd, F_SETFL, flags & ~FNDELAY);
                return;
            }
        } else {
            slog << "Unexpected message found on verify of size " << recv_length
            << endl;
            logger(slog.str());
        }
        status = fcntl(fds[i].fd, F_SETFL, flags & ~FNDELAY);
    }
}


int converse(int client_socket, int server_socket){
    struct pollfd fds[2];
    int    nfds = 2;
    int fd_available;
    int count = 0;
    int disp_movement = 1000000;
    memset(fds, 0, sizeof(fds));
    fds[CLIENT].fd = client_socket;
    fds[CLIENT].events = POLLIN;
    fds[XSERVER].fd = server_socket;
    fds[XSERVER].events = POLLIN;

    while(true){
        if (count++%disp_movement == 0) {logger(".");}
        if (count%(disp_movement*10) == 0) {
            logger("\n");
            //verify_empty(fds, nfds);
        }
        // Check if there is a message, and deal with potential errors
        fd_available = poll(fds, nfds, 0);
        switch (fd_available) {
        case -1:
            fprintf(stderr, "Error %i on select()\n", errno);
            //terminate();
            return -1;
            break;
        case 0:
            // No messages, might be good point to insert
            // logger("Pull returned 0\n");
            break;
        default:
            slog << "Poll returned " << fd_available << endl;
            logger(slog.str(), ERR);
            int mreturn = 0;
            for (int i = 0; i < nfds; i++) {
                logger(":");
                if (fds[i].revents != 0) {  // Did we find something?
                    logger("Event found - ");
                    if (fds[i].revents != POLLIN) {
                        //Error occured
                        slog << "ERROR: poll event returned " << fds[i].revents << endl;
                        logger(slog.str());
                        return -1;
                    } else { // found a message
                        if (i == XSERVER) {
                            logger("Server sent a message\n");
                            mreturn = copy_msgs(server_socket, client_socket);
                            if (mreturn < 0) { return mreturn; }
                        } else if (i == CLIENT) {
                            logger("Client sent a message\n");
                            mreturn = copy_msgs(client_socket, server_socket);
                            if (mreturn < 0) { return mreturn; }
                        }
                    }
                }
            }
        }
    }
    return 1;
}


//int converse(int client_socket, int server_socket){
//    struct rlimit max_file_num;
//    fd_set fdset, nullset;
//    int fd_available;
//    timeval zero_time;
//
//    // Find out max # of FDs.  Not sure why he did this as we are only using
//    // two
//    if(getrlimit(RLIMIT_NOFILE, &max_file_num)) { \
//      fprintf(stderr, "Error %i on getrlimit()\n", errno); \
//      terminate(); \
//   }
//
//
//    // sets select to timeout after 0 seconds
//    zero_time.tv_sec= 0;
//    zero_time.tv_usec= 0;
//
//    // setup select to inform us when a msg has arrived.
//    FD_ZERO(&fdset);
//    FD_SET(client_socket, &fdset);
//    FD_SET(server_socket, &fdset);
//    fd_available = select(max_file_num.rlim_max,
//                  (fd_set *)&fdset,
//                  (fd_set *)&nullset,
//                  (fd_set *)&nullset,
//                  &zero_time);
//
//    // Check if there is a message, and deal with potential errors
//    switch (fd_available){
//        case -1:
//            fprintf(stderr, "Error %i on select()\n", errno);
//            //terminate();
//            return -1;
//            break;
//        case 0:
//            // No messages, might be good point to insert
//            break;
//        default:
//            if (FD_ISSET(server_socket, &fdset)) {
//                copy_msgs(server_socket, client_socket);
//            }
//            if (FD_ISSET(client_socket, &fdset)) {
//                copy_msgs(client_socket, server_socket);
//            }
//    }
//    return 1;
//}


/* ------------------------------------------------------------------- */

/*
 *  copy_message
 *
 *  Block for the message if necessary.  Receive a message and copy it across.
 *  While copying across, for each chunk copied, the specified function
 *  is called.  If the function is NULL, nothing gets called.
 */

//int copy_message(int dest_socket, int source_socket, enum blockstatus block,
//		 int (*garbler)(int source_socket, char *buf, int *buflen_ptr,
//				int max_buflen,	unsigned int sequence))
//{
//	int		flags = fcntl(source_socket, F_GETFL, 0);
//	int		status;
//	char		buffer[BUFFER_SIZE];
//	int		recv_length;
//	unsigned int	sequence = 0;
//
//	/*
//	 *  Set blocking status.
//	 */
//
//	if (flags == -1)
//	{
//		if (DISPLAY_MSGS) {
//			fprintf(stderr, "%s (copy_message): Couldn't get flags of "
//					"source socket.\n", progname);
//			perror("(copy_message)");
//		}
//		return -1;
//	}
//
//	switch (block)
//	{
//		case NON_BLOCKING:
//			status = fcntl(source_socket, F_SETFL, FNDELAY);
//			break;
//
//		case BLOCKING:
//			status = fcntl(source_socket, F_SETFL, flags & ~FNDELAY);
//			break;
//
//		default:
//			if (DISPLAY_MSGS) {
//				fprintf(stderr, "%s (copy_message): Invalid blocking status.\n",
//					progname);
//			}
//			return -1;
//	}
//
//	if (status == -1)
//	{
//		if (DISPLAY_MSGS) {
//			fprintf(stderr, "%s (copy_message): Can't set blocking status.\n",
//				progname);
//			perror("(copy_message)");
//		}
//		return -1;
//	}
//
//	/*
//	 *  Recv first message.
//	 *  When non-blocking recv_length of -1 means no messages were found
//	 *  unclear why the program continues when no message is found?
//	 */
//	errno = 0;
//	recv_length = recv(source_socket, buffer, BUFFER_SIZE, 0);
//	if (recv_length == -1)
//	{
//		if (block == BLOCKING  ||  errno != EWOULDBLOCK)
//		{
//
//			if (DISPLAY_MSGS) {
//				fprintf(stderr, "%s (copy_message): Couldn't recv "
//						"message from source.\n", progname);
//				perror("(copy_message)");
//			}
//			fcntl(source_socket, F_SETFL, flags); /* this could clobber errno */
//			return -3;
//		}
//	}
//
//	if (recv_length == 0)	/* EOF on source socket */
//	{
//		fcntl(source_socket, F_SETFL, flags);
//		return 0;
//	}
//
//    fprintf(stderr,"THB - msg 1 (%d) :%s:\n", recv_length, buffer);
//    fprintf(stderr,"%s\n", buffer);
//
//	/*
//	 *  Set the source socket to non blocking.  Continue copying messages
//	 *  until EWOULDBLOCK.  That way we exhaust what is currently available
//	 *  even if it would not fit into the buffer in one go.
//	 */
//
//	if (block == BLOCKING  &&  fcntl(source_socket, F_SETFL, FNDELAY) == -1)
//	{
//		if (DISPLAY_MSGS) {
//			fprintf(stderr, "%s (copy_message): Unable to make source "
//					"socket non-blocking.\n", progname);
//			perror("(copy_message)");
//		}
//		fcntl(source_socket, F_SETFL, flags);
//		return -1;
//	}
//
//	if (garbler != NULL) {
//        fprintf(stderr, "...Calling garbler 1st\n");
//        if ((*garbler)(source_socket, buffer, &recv_length,
//                       BUFFER_SIZE, sequence++) == -1) {
//			if (DISPLAY_MSGS) {
//				fprintf(stderr, "%s (copy_message): Garbler returned "
//						"error.  Sequence = 0.\n", progname);
//			}
//            fcntl(source_socket, F_SETFL, flags);
//            return -5;
//        }
//    }
//
//    fprintf(stderr,"...sending 1st msg to Xserver\n");
//	if (send(dest_socket, buffer, recv_length, 0) < recv_length)
//	{
//#ifdef ERROR_MESSAGES
//		fprintf(stderr, "%s (copy_message): Can't send message of "
//			"length %d\n", progname, recv_length);
//		perror("(copy_message)");
//#endif
//		fcntl(source_socket, F_SETFL, flags);
//		return -2;
//	}
//
//	errno = 0;
//	while ((recv_length = recv(source_socket, buffer, BUFFER_SIZE, 0)) > 0)
//	{
//        fprintf(stderr,"THB - msg w (%d) :%x:\n", recv_length, buffer);
//        fprintf(stderr,"%s\n", buffer);
//		if (garbler != NULL) {
//            fprintf(stderr, "...Calling garbler again\n");
//            if ((*garbler)(source_socket, buffer, &recv_length,
//                           BUFFER_SIZE, sequence++) == -1) {
//#ifdef ERROR_MESSAGES
//                fprintf(stderr, "%s (copy_message): Garbler "
//                        "returned error.  Sequence = "
//                        "%d.\n",
//                    progname, sequence - 1);
//#endif
//                fcntl(source_socket, F_SETFL, flags);
//                return -5;
//            }
//        }
//
//        fprintf(stderr,"...sending wth msg (:%s:) to Xserver\n", buffer);
//		if (send(dest_socket, buffer, recv_length, 0) < recv_length)
//		{
//#ifdef ERROR_MESSAGES
//			fprintf(stderr, "%s (copy_message): Can't send "
//					"message length %d.\n",
//				progname, recv_length);
//			perror("(copy_message)");
//#endif
//			fcntl(source_socket, F_SETFL, flags);
//			return -2;
//		}
//	}
//
//	if (recv_length == -1  &&  errno != EWOULDBLOCK)
//	{
//#ifdef ERROR_MESSAGES
//		fprintf(stderr, "%s (copy_message): Couldn't recv message.\n",
//			progname);
//		perror("(copy_message)");
//#endif
//	}
//
//	fcntl(source_socket, F_SETFL, flags);
//
//	if (recv_length == -1  &&  errno != EWOULDBLOCK){
//        fprintf(stderr, "<--returning -1\n");
//        return -3;
//    }
//	else if (recv_length == 0) {
//        fprintf(stderr, "<--returning 0\n");
//        return 0;
//    }
//	else {
//        fprintf(stderr, "<--returning 1\n");
//        return 1;
//    }
//}
