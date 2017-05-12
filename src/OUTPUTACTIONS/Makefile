ACC=
CC=$(ACC) gcc
REMCFLAGS     = -Wall -Wshadow -I.. -DLIST_STATS -DLIST_HISTOGRAM $(SIMGRAPH)

DEST	      = .

HDRS	      = outputAction.h \
		outputActionTable.h

INSTALL	      = /etc/install

LD	      = ld

LDFLAGS	      =

LIBS	      =

MAKEFILE      = Makefile

OBJS	      = ${SRCS:.c=.o}

PRINT	      = pr

PROGRAM       = outputActions.o

SHELL	      = /bin/sh

SRCS	      = defaultOutputAction.c \
		strictPriorityOutputAction.c \
		subportOutputAction.c

SYSHDRS	      =

all:		    
		$(MAKE) optall

optall:		
		$(MAKE) $(PROGRAM) CFLAGS="-O3 $(REMCFLAGS)"

debugall:	
		$(MAKE) $(PROGRAM) CFLAGS="-g $(REMCFLAGS)"


$(PROGRAM)::     $(OBJS) $(LIBS)
		@echo "Linking clean sim: $(PROGRAM) ..."
		$(LD) -r $(OBJS) $(LDFLAGS) $(LIBS) -o $(PROGRAM)
		@echo "done"

clean:;		rm -f $(OBJS) $(PROGRAM) core

clobber:;	rm -f $(OBJS) $(PROGRAM) core tags

##depend:;	@mkmf -f $(MAKEFILE) ROOT=$(ROOT)

depend:;	makedepend -o.o -- $(REMCFLAGS) -- $(SRCS)

echo:;		@echo $(HDRS) $(SRCS)

index:;		ctags -wx $(HDRS) $(SRCS)

install::	$(PROGRAM)
		@echo Installing $(PROGRAM) in $(DEST)
#		@-strip $(PROGRAM)
		@if [ $(DEST) != . ]; then \
		(rm -f $(DEST)/$(PROGRAM); $(INSTALL) -f $(DEST) $(PROGRAM)); fi

print:;		@$(PRINT) $(HDRS) $(SRCS)

tags:           $(HDRS) $(SRCS); @ctags $(HDRS) $(SRCS)

update:		$(DEST)/$(PROGRAM)

###
# DO NOT DELETE THIS LINE -- make depend depends on it.

defaultOutputAction.o: ../sim.h ../bitmap.h ../stat.h ../histogram.h
defaultOutputAction.o: ../lists.h ../switchStats.h ../types.h
defaultOutputAction.o: ../latencyStats.h ../functionTable.h outputAction.h
strictPriorityOutputAction.o: ../sim.h ../bitmap.h ../stat.h ../histogram.h
strictPriorityOutputAction.o: ../lists.h ../switchStats.h ../types.h
strictPriorityOutputAction.o: ../latencyStats.h ../functionTable.h
strictPriorityOutputAction.o: outputAction.h
subportOutputAction.o: ../sim.h ../bitmap.h ../stat.h ../histogram.h
subportOutputAction.o: ../lists.h ../switchStats.h ../types.h
subportOutputAction.o: ../latencyStats.h ../functionTable.h outputAction.h
