ACC=
CC=$(ACC) gcc
REMCFLAGS	      =  -Wall -Wshadow -I.. -DLIST_HISTOGRAM -DLIST_STATS $(SIMGRAPH)

DEST	      = .

HDRS	      = algorithm.h\
		algorithmTable.h\
		assign2.h\
		miscfns.h\
		pim.h\
		rr.h\
		scheduleStats.h\

INSTALL	      = /etc/install

LD	      = ld

LDFLAGS	      =

LIBS	      =

OBJS	      = ${SRCS:.c=.o}

PRINT	      = pr

PROGRAM       = algorithms.o

SHELL	      = /bin/sh

SRCS          = ap2driver.c \
        assign2sap.c \
        fifo.c \
        future.c \
        gs_lqf.c \
        gs_ocf.c \
        gsaMatch.c \
        ilpf.c \
        ilqf.c \
        iocf.c \
        iopf.c \
        lpf.c \
        lpf_delay.c \
        lqf.c \
        maximum.c \
        maxrand.c \
        maxsize.c \
        mucf.c \
        mcast_conc_residue.c \
        mcast_dist_residue.c \
        mcast_random.c \
        mcast_slip.c \
        mcast_tatra.c \
        mcast_wt_fanout.c \
        mcast_wt_residue.c \
        miscfns.c\
        neural.c \
        nullSchedulingAlgorithm.c \
        ocf.c \
        opf.c \
        opf_delay.c \
        pim.c \
        pri_fifo.c \
        pri_lqf.c \
        pri_mcast_random.c \
        pri_ocf.c \
        pri_islip.c\
        pri_combo.c\
        pristrict_lqf.c \
        pristrict_ocf.c \
        rr.c \
        scheduleStats.c \
        islip.c \
        wfa.c \
        wwfa.c

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

# DO NOT DELETE THIS LINE -- make depend depends on it.

ap2driver.o: assign2.h
assign2sap.o: assign2.h
fifo.o: ../sim.h ../bitmap.h ../stat.h ../histogram.h ../lists.h
fifo.o: ../switchStats.h ../types.h ../latencyStats.h ../functionTable.h
fifo.o: algorithm.h
future.o: ../sim.h ../bitmap.h ../stat.h ../histogram.h ../lists.h
future.o: ../switchStats.h ../types.h ../latencyStats.h ../functionTable.h
future.o: algorithm.h future.h
gs_lqf.o: ../sim.h ../bitmap.h ../stat.h ../histogram.h ../lists.h
gs_lqf.o: ../switchStats.h ../types.h ../latencyStats.h ../functionTable.h
gs_lqf.o: algorithm.h assign2.h
gs_ocf.o: ../sim.h ../bitmap.h ../stat.h ../histogram.h ../lists.h
gs_ocf.o: ../switchStats.h ../types.h ../latencyStats.h ../functionTable.h
gs_ocf.o: algorithm.h assign2.h
ilpf.o: ../sim.h ../bitmap.h ../stat.h ../histogram.h ../lists.h
ilpf.o: ../switchStats.h ../types.h ../latencyStats.h ../functionTable.h
ilpf.o: algorithm.h assign2.h
ilqf.o: ../sim.h ../bitmap.h ../stat.h ../histogram.h ../lists.h
ilqf.o: ../switchStats.h ../types.h ../latencyStats.h ../functionTable.h
ilqf.o: algorithm.h ilqf.h scheduleStats.h
iocf.o: ../sim.h ../bitmap.h ../stat.h ../histogram.h ../lists.h
iocf.o: ../switchStats.h ../types.h ../latencyStats.h ../functionTable.h
iocf.o: algorithm.h ilqf.h scheduleStats.h
iopf.o: ../sim.h ../bitmap.h ../stat.h ../histogram.h ../lists.h
iopf.o: ../switchStats.h ../types.h ../latencyStats.h ../functionTable.h
iopf.o: algorithm.h assign2.h
lpf.o: ../sim.h ../bitmap.h ../stat.h ../histogram.h ../lists.h
lpf.o: ../switchStats.h ../types.h ../latencyStats.h ../functionTable.h
lpf.o: algorithm.h assign2.h
lpf_delay.o: ../sim.h ../bitmap.h ../stat.h ../histogram.h ../lists.h
lpf_delay.o: ../switchStats.h ../types.h ../latencyStats.h ../functionTable.h
lpf_delay.o: algorithm.h assign2.h
lqf.o: ../sim.h ../bitmap.h ../stat.h ../histogram.h ../lists.h
lqf.o: ../switchStats.h ../types.h ../latencyStats.h ../functionTable.h
lqf.o: algorithm.h assign2.h
maximum.o: ../sim.h ../bitmap.h ../stat.h ../histogram.h ../lists.h
maximum.o: ../switchStats.h ../types.h ../latencyStats.h ../functionTable.h
maximum.o: algorithm.h
maxrand.o: ../sim.h ../bitmap.h ../stat.h ../histogram.h ../lists.h
maxrand.o: ../switchStats.h ../types.h ../latencyStats.h ../functionTable.h
maxrand.o: algorithm.h
maxsize.o: ../sim.h ../bitmap.h ../stat.h ../histogram.h ../lists.h
maxsize.o: ../switchStats.h ../types.h ../latencyStats.h ../functionTable.h
maxsize.o: algorithm.h assign2.h
mucf.o: ../sim.h ../bitmap.h ../stat.h ../histogram.h ../lists.h
mucf.o: ../switchStats.h ../types.h ../latencyStats.h ../functionTable.h
mucf.o: algorithm.h ilqf.h scheduleStats.h
mcast_conc_residue.o: ../sim.h ../bitmap.h ../stat.h ../histogram.h
mcast_conc_residue.o: ../lists.h ../switchStats.h ../types.h
mcast_conc_residue.o: ../latencyStats.h ../functionTable.h algorithm.h
mcast_dist_residue.o: ../sim.h ../bitmap.h ../stat.h ../histogram.h
mcast_dist_residue.o: ../lists.h ../switchStats.h ../types.h
mcast_dist_residue.o: ../latencyStats.h ../functionTable.h algorithm.h
mcast_random.o: ../sim.h ../bitmap.h ../stat.h ../histogram.h ../lists.h
mcast_random.o: ../switchStats.h ../types.h ../latencyStats.h
mcast_random.o: ../functionTable.h algorithm.h
mcast_slip.o: ../sim.h ../bitmap.h ../stat.h ../histogram.h ../lists.h
mcast_slip.o: ../switchStats.h ../types.h ../latencyStats.h
mcast_slip.o: ../functionTable.h algorithm.h mcast_slip.h scheduleStats.h
mcast_tatra.o: ../sim.h ../bitmap.h ../stat.h ../histogram.h ../lists.h
mcast_tatra.o: ../switchStats.h ../types.h ../latencyStats.h
mcast_tatra.o: ../functionTable.h algorithm.h
mcast_wt_fanout.o: ../sim.h ../bitmap.h ../stat.h ../histogram.h ../lists.h
mcast_wt_fanout.o: ../switchStats.h ../types.h ../latencyStats.h
mcast_wt_fanout.o: ../functionTable.h algorithm.h
mcast_wt_residue.o: ../sim.h ../bitmap.h ../stat.h ../histogram.h ../lists.h
mcast_wt_residue.o: ../switchStats.h ../types.h ../latencyStats.h
mcast_wt_residue.o: ../functionTable.h algorithm.h
miscfns.o: ../sim.h ../bitmap.h ../stat.h ../histogram.h ../lists.h
miscfns.o: ../switchStats.h ../types.h ../latencyStats.h ../functionTable.h
miscfns.o: rr.h scheduleStats.h
neural.o: ../sim.h ../bitmap.h ../stat.h ../histogram.h ../lists.h
neural.o: ../switchStats.h ../types.h ../latencyStats.h ../functionTable.h
neural.o: algorithm.h neural.h
nullSchedulingAlgorithm.o: ../sim.h ../bitmap.h ../stat.h ../histogram.h
nullSchedulingAlgorithm.o: ../lists.h ../switchStats.h ../types.h
nullSchedulingAlgorithm.o: ../latencyStats.h ../functionTable.h algorithm.h
ocf.o: ../sim.h ../bitmap.h ../stat.h ../histogram.h ../lists.h
ocf.o: ../switchStats.h ../types.h ../latencyStats.h ../functionTable.h
ocf.o: algorithm.h assign2.h
opf.o: ../sim.h ../bitmap.h ../stat.h ../histogram.h ../lists.h
opf.o: ../switchStats.h ../types.h ../latencyStats.h ../functionTable.h
opf.o: algorithm.h assign2.h
opf_delay.o: ../sim.h ../bitmap.h ../stat.h ../histogram.h ../lists.h
opf_delay.o: ../switchStats.h ../types.h ../latencyStats.h ../functionTable.h
opf_delay.o: algorithm.h assign2.h
pim.o: ../sim.h ../bitmap.h ../stat.h ../histogram.h ../lists.h
pim.o: ../switchStats.h ../types.h ../latencyStats.h ../functionTable.h
pim.o: algorithm.h pim.h scheduleStats.h
pri_fifo.o: ../sim.h ../bitmap.h ../stat.h ../histogram.h ../lists.h
pri_fifo.o: ../switchStats.h ../types.h ../latencyStats.h ../functionTable.h
pri_fifo.o: algorithm.h
pri_lqf.o: ../sim.h ../bitmap.h ../stat.h ../histogram.h ../lists.h
pri_lqf.o: ../switchStats.h ../types.h ../latencyStats.h ../functionTable.h
pri_lqf.o: algorithm.h assign2.h
pri_mcast_random.o: ../sim.h ../bitmap.h ../stat.h ../histogram.h ../lists.h
pri_mcast_random.o: ../switchStats.h ../types.h ../latencyStats.h
pri_mcast_random.o: ../functionTable.h algorithm.h
pri_ocf.o: ../sim.h ../bitmap.h ../stat.h ../histogram.h ../lists.h
pri_ocf.o: ../switchStats.h ../types.h ../latencyStats.h ../functionTable.h
pri_ocf.o: algorithm.h assign2.h
pri_islip.o: ../sim.h ../bitmap.h ../stat.h ../histogram.h ../lists.h
pri_islip.o: ../switchStats.h ../types.h ../latencyStats.h ../functionTable.h
pri_islip.o: algorithm.h pri_rr.h scheduleStats.h miscfns.h
pri_combo.o: ../sim.h ../bitmap.h ../stat.h ../histogram.h ../lists.h
pri_combo.o: ../switchStats.h ../types.h ../latencyStats.h ../functionTable.h
pri_combo.o: algorithm.h
pristrict_lqf.o: ../sim.h ../bitmap.h ../stat.h ../histogram.h ../lists.h
pristrict_lqf.o: ../switchStats.h ../types.h ../latencyStats.h
pristrict_lqf.o: ../functionTable.h algorithm.h assign2.h
pristrict_ocf.o: ../sim.h ../bitmap.h ../stat.h ../histogram.h ../lists.h
pristrict_ocf.o: ../switchStats.h ../types.h ../latencyStats.h
pristrict_ocf.o: ../functionTable.h algorithm.h assign2.h
rr.o: ../sim.h ../bitmap.h ../stat.h ../histogram.h ../lists.h
rr.o: ../switchStats.h ../types.h ../latencyStats.h ../functionTable.h
rr.o: algorithm.h rr.h scheduleStats.h miscfns.h
scheduleStats.o: ../sim.h ../bitmap.h ../stat.h ../histogram.h ../lists.h
scheduleStats.o: ../switchStats.h ../types.h ../latencyStats.h
scheduleStats.o: ../functionTable.h scheduleStats.h
islip.o: ../sim.h ../bitmap.h ../stat.h ../histogram.h ../lists.h
islip.o: ../switchStats.h ../types.h ../latencyStats.h ../functionTable.h
islip.o: algorithm.h rr.h scheduleStats.h miscfns.h
wfa.o: ../sim.h ../bitmap.h ../stat.h ../histogram.h ../lists.h
wfa.o: ../switchStats.h ../types.h ../latencyStats.h ../functionTable.h
wfa.o: algorithm.h assign2.h
wwfa.o: ../sim.h ../bitmap.h ../stat.h ../histogram.h ../lists.h
wwfa.o: ../switchStats.h ../types.h ../latencyStats.h ../functionTable.h
wwfa.o: algorithm.h wwfa.h scheduleStats.h
