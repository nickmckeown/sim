ACC=
CC=$(ACC) gcc
REMCFLAGS	  = -Wall -Wshadow -DLIST_STATS -DLIST_HISTOGRAM $(SIMGRAPH) \
		    -I.. -I../INPUTACTIONS

DEST	      = .

HDRS	      = traffic.h \
		trafficTable.h

INSTALL	      = /etc/install

LD	      = ld

LDFLAGS	      =

LIBS	      =

MAKEFILE      = Makefile

OBJS	      = ${SRCS:.c=.o}

PRINT	      = pr

PROGRAM       = traffic.o

SHELL	      = /bin/sh

SRCS	      = bernoulli_iid_nonuniform.c \
		bernoulli_iid_uniform.c \
		null.c \
		keepfull.c \
		periodicTrace.c \
		trace.c \
		tracePacket.c \
		bursty_nonuniform.c \
		bursty.c

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
		if [ $(DEST) != . ]; then \
		(rm -f $(DEST)/$(PROGRAM); $(INSTALL) -f $(DEST) $(PROGRAM)); fi

print:;		@$(PRINT) $(HDRS) $(SRCS)

tags:           $(HDRS) $(SRCS); @ctags $(HDRS) $(SRCS)

update:		$(DEST)/$(PROGRAM)

###
# DO NOT DELETE THIS LINE -- make depend depends on it.

bernoulli_iid_nonuniform.o: ../sim.h ../bitmap.h ../stat.h ../histogram.h
bernoulli_iid_nonuniform.o: ../lists.h ../switchStats.h ../types.h
bernoulli_iid_nonuniform.o: ../latencyStats.h ../functionTable.h traffic.h
bernoulli_iid_nonuniform.o: ../INPUTACTIONS/inputAction.h
bernoulli_iid_uniform.o: ../sim.h ../bitmap.h ../stat.h ../histogram.h
bernoulli_iid_uniform.o: ../lists.h ../switchStats.h ../types.h
bernoulli_iid_uniform.o: ../latencyStats.h ../functionTable.h traffic.h
bernoulli_iid_uniform.o: ../INPUTACTIONS/inputAction.h
null.o: ../sim.h ../bitmap.h ../stat.h ../histogram.h ../lists.h
null.o: ../switchStats.h ../types.h ../latencyStats.h ../functionTable.h
null.o: traffic.h
keepfull.o: ../sim.h ../bitmap.h ../stat.h ../histogram.h ../lists.h
keepfull.o: ../switchStats.h ../types.h ../latencyStats.h ../functionTable.h
keepfull.o: traffic.h ../INPUTACTIONS/inputAction.h
periodicTrace.o: ../sim.h ../bitmap.h ../stat.h ../histogram.h ../lists.h
periodicTrace.o: ../switchStats.h ../types.h ../latencyStats.h
periodicTrace.o: ../functionTable.h traffic.h ../INPUTACTIONS/inputAction.h
trace.o: ../sim.h ../bitmap.h ../stat.h ../histogram.h ../lists.h
trace.o: ../switchStats.h ../types.h ../latencyStats.h ../functionTable.h
trace.o: traffic.h ../INPUTACTIONS/inputAction.h
tracePacket.o: ../sim.h ../bitmap.h ../stat.h ../histogram.h ../lists.h
tracePacket.o: ../switchStats.h ../types.h ../latencyStats.h
tracePacket.o: ../functionTable.h traffic.h ../INPUTACTIONS/inputAction.h
tracePacket.o: trace.h
bursty_nonuniform.o: ../sim.h ../bitmap.h ../stat.h ../histogram.h ../lists.h
bursty_nonuniform.o: ../switchStats.h ../types.h ../latencyStats.h
bursty_nonuniform.o: ../functionTable.h traffic.h
bursty_nonuniform.o: ../INPUTACTIONS/inputAction.h
bursty.o: ../sim.h ../bitmap.h ../stat.h ../histogram.h ../lists.h
bursty.o: ../switchStats.h ../types.h ../latencyStats.h ../functionTable.h
bursty.o: traffic.h ../INPUTACTIONS/inputAction.h
