# DO NOT USE MAKE TO COMPILE, please use createsim.
ACC=
CC = $(ACC) gcc 
LD = gcc
REMCFLAGS=	-Wall -Wshadow \
		-ITRAFFIC -IALGORITHMS -IINPUTACTIONS -IOUTPUTACTIONS \
		-IFABRICS \
		-DLIST_STATS -DLIST_HISTOGRAM -DFREELIST $(SIMGRAPH)


DEST	      = .

HDRS	      = bitmap.h \
		circBuffer.h \
		histogram.h \
		latencyStats.h \
		lists.h \
		sim.h \
		stat.h \
		switchStats.h\
		types.h\
		$(AUXHEAD) \
		$(GRAPHHEAD)

INSTALL	      = /etc/install

LDFLAGS	      = -lm 

LIBS	      = ALGORITHMS/algorithms.o \
		INPUTACTIONS/inputActions.o \
		OUTPUTACTIONS/outputActions.o \
		FABRICS/fabrics.o \
		TRAFFIC/traffic.o 

OBJS	      = ${SRCS:.c=.o}

OTHERGRAPHOBJS   =  aux.o \
					graph.o


PRINT	      = pr

PROGRAM       = sim

SHELL	      = /bin/sh

SRCS	      = bitmap.c \
		cell.c \
		circBuffer.c \
		config.c \
		create.c \
		debug.c \
		functionTable.c \
		histogram.c \
		latencyStats.c \
		lists.c \
		sim.c \
		stat.c \
		switchStats.c \
		$(AUXSRC) \
	    $(GRAPHSRC)

SYSHDRS	      =

all:	
			$(MAKE) optall; mv sim ../bin

this:
			$(MAKE) thisoptall

optall:        trafficopt algorithmsopt fabricsopt inputActionsopt outputActionsopt thisoptall

debugall:     trafficdbg algorithmsdbg fabricsdbg inputActionsdbg outputActionsdbg thisdebugall

thisoptall:
			$(MAKE) $(PROGRAM) CFLAGS="-O3 $(REMCFLAGS)" 
			
thisdebugall:
			$(MAKE) $(PROGRAM) CFLAGS="-g $(REMCFLAGS)"
		
trafficopt:;	@cd TRAFFIC; $(MAKE) optall; cd ..

algorithmsopt:;	@cd ALGORITHMS; $(MAKE) optall; cd ..

fabricsopt:;	@cd FABRICS; $(MAKE) optall; cd ..

inputActionsopt:;	@cd INPUTACTIONS; $(MAKE) optall; cd ..

outputActionsopt:;	@cd OUTPUTACTIONS; $(MAKE) optall; cd ..

##NOW THE DEBUG ONES
trafficdbg:;	@cd TRAFFIC; $(MAKE) debugall; cd ..

algorithmsdbg:;	@cd ALGORITHMS; $(MAKE) debugall; cd ..

fabricsdbg:;	@cd FABRICS; $(MAKE) debugall; cd ..

inputActionsdbg:;	@cd INPUTACTIONS; $(MAKE) debugall; cd ..

outputActionsdbg:;	@cd OUTPUTACTIONS; $(MAKE) debugall; cd ..

##Now this directory's files
$(PROGRAM)::     $(OBJS) $(LIBS)
		@echo "Linking clean sim: $(PROGRAM) ..."
		$(LD) $(OBJS) $(LIBS) -o $(PROGRAM) $(LDFLAGS)
		@echo "done"

clean:;		rm -f $(OBJS) core $(OTHERGRAPHOBJS) core  $(PROGRAM)

cleanall:	cleantraffic cleanalgorithms cleanfabrics cleaninput cleanoutput clean

cleantraffic:;		cd TRAFFIC; $(MAKE) clean; cd ..

cleanalgorithms:;	cd ALGORITHMS; $(MAKE) clean; cd ..

cleanfabrics:;		cd FABRICS; $(MAKE) clean; cd ..

cleaninput:;		cd INPUTACTIONS; $(MAKE) clean; cd ..

cleanoutput:;		cd OUTPUTACTIONS; $(MAKE) clean; cd ..


clobber:;		rm -f $(OBJS) $(PROGRAM) core tags


clobberall:	clobbertraffic clobberalgorithms clobberfabrics clobberinput clobberoutput clobber

clobbertraffic:;	cd TRAFFIC; $(MAKE) clobber; cd ..

clobberalgorithms:;	cd ALGORITHMS; $(MAKE) clobber; cd ..

clobberfabrics:;	cd FABRICS; $(MAKE) clobber; cd ..

clobberinput:;	cd INPUTACTIONS; $(MAKE) clobber; cd ..

clobberoutput:;	cd OUTPUTACTIONS; $(MAKE) clobber; cd ..

##depend:;	@mkmf -f $(MAKEFILE) ROOT=$(ROOT)

depend:;	makedepend -o.o -- $(REMCFLAGS) -- $(SRCS)
			cd TRAFFIC; $(MAKE) depend; cd ..
			cd ALGORITHMS; $(MAKE) depend; cd ..
			cd FABRICS; $(MAKE) depend; cd ..
			cd INPUTACTIONS; $(MAKE) depend; cd ..
			cd OUTPUTACTIONS; $(MAKE) depend; cd ..

echo:;		@echo $(HDRS) $(SRCS)

index:;		ctags -wx $(HDRS) $(SRCS)

install::	$(PROGRAM)
		@echo Installing $(PROGRAM) in $(DEST)
#		@-strip $(PROGRAM)
		if [ $(DEST) != . ]; then \
		(rm -f $(DEST)/$(PROGRAM); $(INSTALL) -f $(DEST) $(PROGRAM)); fi

print:;		@$(PRINT) $(HDRS) $(SRCS)

tags:           $(HDRS) $(SRCS); ctags $(HDRS) $(SRCS)

update:		$(DEST)/$(PROGRAM)

#$(DEST)/$(PROGRAM):: $(SRCS) $(LIBS) $(HDRS) $(EXTHDRS)
#		@$(MAKE) -f $(MAKEFILE) ROOT=$(ROOT) DEST=$(DEST) install
###
# DO NOT DELETE THIS LINE -- make depend depends on it.

bitmap.o: bitmap.h
cell.o: sim.h bitmap.h stat.h histogram.h lists.h switchStats.h types.h
cell.o: latencyStats.h functionTable.h circBuffer.h
circBuffer.o: circBuffer.h
config.o: sim.h bitmap.h stat.h histogram.h lists.h switchStats.h types.h
config.o: latencyStats.h functionTable.h ALGORITHMS/algorithm.h
config.o: FABRICS/fabric.h TRAFFIC/traffic.h INPUTACTIONS/inputAction.h
config.o: OUTPUTACTIONS/outputAction.h ALGORITHMS/algorithmTable.h
config.o: FABRICS/fabricTable.h TRAFFIC/trafficTable.h
config.o: INPUTACTIONS/inputActionTable.h OUTPUTACTIONS/outputActionTable.h
create.o: sim.h bitmap.h stat.h histogram.h lists.h switchStats.h types.h
create.o: latencyStats.h functionTable.h
debug.o: sim.h bitmap.h stat.h histogram.h lists.h switchStats.h types.h
debug.o: latencyStats.h functionTable.h
functionTable.o: functionTable.h
histogram.o: histogram.h
latencyStats.o: sim.h bitmap.h stat.h histogram.h lists.h switchStats.h
latencyStats.o: types.h latencyStats.h functionTable.h
lists.o: lists.h histogram.h circBuffer.h
sim.o: sim.h bitmap.h stat.h histogram.h lists.h switchStats.h types.h
sim.o: latencyStats.h functionTable.h ALGORITHMS/algorithm.h FABRICS/fabric.h
sim.o: TRAFFIC/traffic.h INPUTACTIONS/inputAction.h
sim.o: OUTPUTACTIONS/outputAction.h
stat.o: stat.h
switchStats.o: sim.h bitmap.h stat.h histogram.h lists.h switchStats.h
switchStats.o: types.h latencyStats.h functionTable.h
