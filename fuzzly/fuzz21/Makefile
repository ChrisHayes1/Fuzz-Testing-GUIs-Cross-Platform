#
#  $Id: Makefile,v 1.4 1995/05/01 04:53:41 ajitk Exp $
#
#  Makefile for Winjig.
#

CC=	g++
CFLAGS=	-g -DERROR_MESSAGES -Wall

LD=	g++
LDFLAGS=-g -Wall -DERROR_MESSAGES
LIBS=	-lXau

RM=	rm -f

EXE=	fuzz21
SRCS=	main.cpp Interface.cpp Agent.cpp Message.cpp Logger.cpp converse.cpp socket_setup.cpp
OBJS=	$(SRCS:.cpp=.o)

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
