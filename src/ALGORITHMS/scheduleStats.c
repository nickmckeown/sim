/* ****************************************************************
 * Copyright Stanford University 1998,99 - All Rights Reserved
 ****************************************************************** 

 * Permission to use, copy, modify, and distribute this software 
 * and its documentation for any purpose is hereby granted without 
 * fee, provided that the above copyright notice appears in all copies
 * and that both the copyright notice, this permission notice, and 
 * the following disclaimer appear in supporting documentation, and 
 * that the name of Stanford University, not be used in advertising or 
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.
 * 
 * STANFORD UNIVERSITY, DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND 
 * FITNESS. IN NO EVENT SHALL STANFORD UNIVERSITY BE LIABLE FOR ANY 
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER 
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION 
 * OF CONTRACT, NEGLIGENCE OR OTHER ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */


/*
 * The Sim Web Site : http://klamath.stanford.edu/tools/SIM
 * The SIM Mailing List: sim-simulator@lists.stanford.edu

 * Send mail to the above email address with "subscribe sim-simulator" in
 * the body of the message.
 *
 */


#include "sim.h"
#include "scheduleStats.h"

/* Locally defined scheduling statistics that are attached to: */
/*  aSwitch->scheduler.schedulingStats */
struct SchedulingStats {
  double sumGrantDelay;
  unsigned long numGrantCells;
  double sumAcceptDelay;
  unsigned long numAcceptCells;
  Stat numSynchronized;
  Stat numIterations;
};

/* Used by input-buffered crossbar switches to maintain
   stats of grant and accept phases. */
/* Statistics are maintained per switch in switch->scheduler.schedulingStats */
void scheduleCellStats( mode, aSwitch, aCell )
  ScheduleCellStatsMode mode;
Switch *aSwitch;
Cell *aCell;
{





  struct ScheduleInfo *scheduleInfo;
  struct SchedulingStats *schedulingStats;

  switch( mode )
    {
    case SCHEDULE_CELL_STATS_INIT:
      /* Called to create and reset scheduling specific stats */
      /* for a switch. */

      if( !aSwitch->scheduler.schedulingStats )
	aSwitch->scheduler.schedulingStats = 
	  (void *) malloc(sizeof(struct SchedulingStats));

      schedulingStats = aSwitch->scheduler.schedulingStats;
      schedulingStats->sumGrantDelay=0;
      schedulingStats->numGrantCells=0;
      schedulingStats->sumAcceptDelay=0;
      schedulingStats->numAcceptCells=0;

      break;
    case SCHEDULE_CELL_STATS_HEAD_ARRIVAL:
      /* Determine if cell has just arrived at head of queue. */
      if( !aCell->switchDependentHeader )
	{
	  /* Create schedulerStats structure and attach to cell */
	  scheduleInfo = (struct ScheduleInfo *) 
	    malloc( sizeof( struct ScheduleInfo ) );
	  aCell->switchDependentHeader = (void *) scheduleInfo;
	  scheduleInfo->headArrivalTime = now; 
	}
			
      break;
    case SCHEDULE_CELL_STATS_UPDATE_GRANT:
      scheduleInfo = (struct ScheduleInfo *)aCell->switchDependentHeader;
      /* Scheduler-specific stats stored on switch */
      schedulingStats = aSwitch->scheduler.schedulingStats;

      scheduleInfo->grantTime = now;
      schedulingStats->sumGrantDelay += 
	now - scheduleInfo->headArrivalTime;
      schedulingStats->numGrantCells++;
			
      break;
    case SCHEDULE_CELL_STATS_PRINT_GRANT:
      /* Scheduler-specific stats stored on switch */
      schedulingStats = aSwitch->scheduler.schedulingStats;
      printf("Average Grant Latency  %8.4f\n",
	     schedulingStats->sumGrantDelay / 
	     (double) schedulingStats->numGrantCells);
      break;
    case SCHEDULE_CELL_STATS_UPDATE_ACCEPT:
      /* Scheduler-specific stats stored on cell */
      scheduleInfo = (struct ScheduleInfo *)aCell->switchDependentHeader;
      scheduleInfo->acceptTime = now;

      /* Scheduler-specific stats stored on switch */
      schedulingStats = aSwitch->scheduler.schedulingStats;
      schedulingStats->sumAcceptDelay += now - scheduleInfo->grantTime;
      schedulingStats->numAcceptCells++;
      break;
    case SCHEDULE_CELL_STATS_PRINT_ACCEPT:
      /* Scheduler-specific stats stored on switch */
      schedulingStats = aSwitch->scheduler.schedulingStats;
      printf("Average Accept Latency %8.4f\n",
	     schedulingStats->sumAcceptDelay / 
	     (double) schedulingStats->numAcceptCells);
      break;
    case SCHEDULE_CELL_STATS_PRINT_ALL:
      scheduleCellStats( SCHEDULE_CELL_STATS_PRINT_GRANT, aSwitch );
      scheduleCellStats( SCHEDULE_CELL_STATS_PRINT_ACCEPT, aSwitch );
      break;
    default:
      fprintf(stderr, "Unknown cell stats mode!\n");
    }
}

void scheduleStats( mode, aSwitch, value )
  ScheduleStatsMode mode;
Switch *aSwitch;
void *value;
{
  struct SchedulingStats *schedulingStats; 

  if( !aSwitch->scheduler.schedulingStats )
    aSwitch->scheduler.schedulingStats = 
      (void *) malloc(sizeof(struct SchedulingStats));

  schedulingStats = aSwitch->scheduler.schedulingStats;
  switch(mode)
    {
    case SCHEDULE_STATS_INIT:	
      initStat(&schedulingStats->numSynchronized, STAT_TYPE_AVERAGE, now);
      initStat(&schedulingStats->numIterations, STAT_TYPE_AVERAGE, now);
      enableStat(&schedulingStats->numSynchronized);
      enableStat(&schedulingStats->numIterations);
      break;
    case SCHEDULE_STATS_PRINT_ALL:	
      scheduleStats( SCHEDULE_STATS_PRINT_NUM_SYNC, aSwitch);
      scheduleStats( SCHEDULE_STATS_PRINT_NUM_ITERATIONS, aSwitch);
      break;
    case SCHEDULE_STATS_NUM_SYNC:
      {
	int *numSynch=value;
	updateStat(&schedulingStats->numSynchronized, (long) *numSynch, now);
	break;
      }
    case SCHEDULE_STATS_PRINT_NUM_SYNC:
      {
	printf("\n");
	printStat(stdout, "Avg Number Synchronized Output Schedulers: ", 
		  &schedulingStats->numSynchronized);
	break;
      }
    case SCHEDULE_STATS_NUM_ITERATIONS:
      {
	int *numIterations=value;
	updateStat(&schedulingStats->numIterations, (long) *numIterations, now);
	break;
      }
    case SCHEDULE_STATS_PRINT_NUM_ITERATIONS:
      {
	printStat(stdout, "Avg Number of Iterations: ", 
		  &schedulingStats->numIterations);
	break;
      }
    default:
      fprintf(stderr, "Unknown stats mode!\n");
    }
}


