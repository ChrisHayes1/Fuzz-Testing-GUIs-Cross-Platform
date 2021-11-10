#
#  $Id: Makefile,v 1.4 1995/05/01 04:53:41 ajitk Exp $
#
#  Makefile for Winjig.
#

CC=	gcc
CFLAGS=	-g -DERROR_MESSAGES -Wall

LD=	gcc
LDFLAGS=-g -Wall
LIBS=	-lXau

RM=	rm -f

EXE=	winjig
SRCS=	main.c signal.c client_connect.c server_connect.c copy_message.c \
	randmsg.c garbler.c insert.c random_events.c
OBJS=	$(SRCS:.c=.o)

$(EXE):	$(OBJS)
	$(LD) $(LDFLAGS) -o $(EXE) $(OBJS) $(LIBS)

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	$(RM) $(OBJS)

realclean:
	$(RM) $(OBJS) $(EXE)

depend:
	makedepend $(CFLAGS) $(SRCS)

# DO NOT DELETE THIS LINE -- make depend depends on it.
