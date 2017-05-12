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

#ifdef _SIM_

#include "graph.h" // SG
#include <assert.h> // SG
#include <string.h> // SG

#endif // _SIM_

/* totalLatency was a local static in latencyStats functions, I now made it
local static in this file to be able to access it for SimGraph polling */

/* Latency of all cells through all switches. */

static Stat totalLatency;

/*****************************************************************/
/** Per Cell stats: latency through different switch components **/
/*****************************************************************/
double
latencyStats(LatencyStatsCommand mode, Switch *aSwitch, Cell *aCell)
{
  static unsigned long cellID=0;
  int ip;

  /* static Stat totalLatency; */
  /* This line is commented and moved outside the function */

  switch( mode )
    {
    case LATENCY_STATS_CELL_INIT:
      aCell->commonStats.ID = cellID++;
      aCell->commonStats.createTime = now;
      aCell->commonStats.arrivalTime = NONE;
      aCell->commonStats.fabricArrivalTime = NONE;
      aCell->commonStats.outputArrivalTime = NONE;
      aCell->commonStats.numSwitchesVisited = 0;
      aCell->commonStats.inputPort = NONE;
      aCell->commonStats.headArrivalTime = NONE;
      aCell->commonStats.dequeueTime = NONE;

      break;
    case LATENCY_STATS_SWITCH_INIT:
      initStat(&totalLatency, STAT_TYPE_AVERAGE, now);
      initStat(&aSwitch->latencyStats.inputLatency, STAT_TYPE_AVERAGE, now);
      initStat(&aSwitch->latencyStats.fabricLatency, STAT_TYPE_AVERAGE, now);
      initStat(&aSwitch->latencyStats.outputLatency, STAT_TYPE_AVERAGE, now);
      initStat(&aSwitch->latencyStats.switchLatency, STAT_TYPE_AVERAGE, now);
      initStat(&aSwitch->latencyStats.avgSchLatency, STAT_TYPE_AVERAGE, now);
      initStat(&aSwitch->latencyStats.avgHolLatency, STAT_TYPE_AVERAGE, now);
      initStat(&aSwitch->latencyStats.avgInputLatency, STAT_TYPE_AVERAGE, now);
      initStat(&aSwitch->latencyStats.avgOutputLatency, STAT_TYPE_AVERAGE, now);

      aSwitch->latencyStats.schedulingLatency =
	(Stat*)malloc(aSwitch->numInputs * sizeof(Stat));
      aSwitch->latencyStats.holLatency = (Stat*)malloc(aSwitch->numInputs * sizeof(Stat));
      
      for(ip = 0; ip < aSwitch->numInputs; ip++)
	{
	  initStat(&aSwitch->latencyStats.schedulingLatency[ip], 
		   STAT_TYPE_AVERAGE, now);
	  initStat(&aSwitch->latencyStats.holLatency[ip], 
		   STAT_TYPE_AVERAGE, now);
	  enableStat(&aSwitch->latencyStats.schedulingLatency[ip]);
	  enableStat(&aSwitch->latencyStats.holLatency[ip]);
	}

      /* Initialize statistics for per priority input latency */
      aSwitch->latencyStats.priInputLatencyUcast =              
	(Stat*)malloc(aSwitch->numPriorities * sizeof(Stat));
      aSwitch->latencyStats.priInputLatencyMcast =              
	(Stat*)malloc(aSwitch->numPriorities * sizeof(Stat));
      
      aSwitch->latencyStats.priOutputLatencyUcast =              
	(Stat*)malloc(aSwitch->numPriorities * sizeof(Stat));      
      aSwitch->latencyStats.priOutputLatencyMcast =              
	(Stat*)malloc(aSwitch->numPriorities * sizeof(Stat));      

      for(ip = 0; ip < aSwitch->numPriorities; ip++)
	{

	  initStat(&aSwitch->latencyStats.priInputLatencyUcast[ip], 
		   STAT_TYPE_AVERAGE, now);
	  initStat(&aSwitch->latencyStats.priInputLatencyMcast[ip], 
		   STAT_TYPE_AVERAGE, now);
	  initStat(&aSwitch->latencyStats.priOutputLatencyUcast[ip], 
		   STAT_TYPE_AVERAGE, now);
	  initStat(&aSwitch->latencyStats.priOutputLatencyMcast[ip], 
		   STAT_TYPE_AVERAGE, now);
	  enableStat(&aSwitch->latencyStats.priInputLatencyUcast[ip]);
	  enableStat(&aSwitch->latencyStats.priInputLatencyMcast[ip]);
	  enableStat(&aSwitch->latencyStats.priOutputLatencyUcast[ip]);
	  enableStat(&aSwitch->latencyStats.priOutputLatencyMcast[ip]);
	}

      enableStat(&totalLatency);
      enableStat(&aSwitch->latencyStats.inputLatency);
      enableStat(&aSwitch->latencyStats.fabricLatency);
      enableStat(&aSwitch->latencyStats.outputLatency);
      enableStat(&aSwitch->latencyStats.switchLatency);
      enableStat(&aSwitch->latencyStats.avgSchLatency);
      enableStat(&aSwitch->latencyStats.avgHolLatency);
      enableStat(&aSwitch->latencyStats.avgInputLatency);
      enableStat(&aSwitch->latencyStats.avgOutputLatency);


      break;

    case LATENCY_STATS_ARRIVAL_TIME:
      /* Called by inputAction when cell arrives at a switch input. */
      aCell->commonStats.arrivalTime = now;
      aCell->commonStats.numSwitchesVisited++;
      break;

    case LATENCY_STATS_DEQUEUE_TIME:
      /* Called by inputAction when cell is actually dequeued from the
         input queue (for multicast dequeue time is different from
         fabricArrival time) */
      aCell->commonStats.dequeueTime = now;
      
    case LATENCY_STATS_ARRIVE_FABRIC:
      /* Called by fabricAction when cell is removed from input buffer.*/
      aCell->commonStats.fabricArrivalTime = now;
      break;
    case LATENCY_STATS_ARRIVE_OUTPUT:
      /* Called by fabricAction when cell is added to output buffer.*/
      aCell->commonStats.outputArrivalTime = now;
      break;
    case LATENCY_STATS_SWITCH_UPDATE:
      {
	/* Called as cell leaves switch to update per-cell switch stats. */
	long aValue;
	int aPriority;

	/* inputLatency */
	aValue = (aCell->commonStats.fabricArrivalTime
		  - aCell->commonStats.arrivalTime);

	/* priInputLatency */
	aPriority = aCell->priority;
	if( aCell->multicast )
		updateStat(&aSwitch->latencyStats.priInputLatencyMcast[aPriority], aValue, now);
		else
	updateStat(&aSwitch->latencyStats.priInputLatencyUcast[aPriority], aValue, now);
	updateStat(&aSwitch->latencyStats.inputLatency, aValue, now);
	updateStat(&aSwitch->latencyStats.avgInputLatency, aValue, now);

	/* fabricLatency */
	aValue = (aCell->commonStats.outputArrivalTime
		  - aCell->commonStats.fabricArrivalTime);
	updateStat(&aSwitch->latencyStats.fabricLatency, aValue, now);

	/* outputLatency */
	aValue = (now - aCell->commonStats.outputArrivalTime);
	aPriority = aCell->priority;
	if( aCell->multicast )
		updateStat(&aSwitch->latencyStats.priOutputLatencyMcast[aPriority], aValue, now);
	else
		updateStat(&aSwitch->latencyStats.priOutputLatencyUcast[aPriority], aValue, now);

	updateStat(&aSwitch->latencyStats.outputLatency, aValue, now);
	updateStat(&aSwitch->latencyStats.avgOutputLatency, aValue, now);

	/* switchLatency */
	aValue = (now - aCell->commonStats.arrivalTime);
	updateStat(&aSwitch->latencyStats.switchLatency, aValue, now);
      
	/* Scheduling latency (per input queue) */
	if(aCell->commonStats.headArrivalTime != NONE)
	  {
	    aValue = (aCell->commonStats.fabricArrivalTime
		      - aCell->commonStats.headArrivalTime);
	    updateStat(&aSwitch->latencyStats.avgSchLatency, aValue, now);
	    updateStat(&aSwitch->latencyStats.schedulingLatency[aCell->commonStats.inputPort], 
		       aValue, now);
	  }

	/* Waiting time at HOL (per input queue) */
	if(aCell->commonStats.headArrivalTime != NONE &&
	   aCell->commonStats.dequeueTime != NONE)
	  {
	    aValue = (aCell->commonStats.dequeueTime
		      - aCell->commonStats.headArrivalTime);
	    updateStat(&aSwitch->latencyStats.avgHolLatency, aValue, now);
	    updateStat(&aSwitch->latencyStats.holLatency[aCell->commonStats.inputPort], 
		       aValue, now);
	  }

	break;
      }
    case LATENCY_STATS_CELL_UPDATE:
      {
	/* Called when cell is destroyed. */
	long aValue;

	/* totalLatency */
	aValue = (now - aCell->commonStats.createTime);
	updateStat(&totalLatency, aValue, now);
	break;
      }
    case LATENCY_STATS_RETURN_AVG:
      {
	double averageLatency;
	if( !totalLatency.number )
	  return(0);
	averageLatency = returnAvgStat(&totalLatency);
	return( averageLatency );
	break;
      }
    case LATENCY_STATS_SWITCH_PRINT:
      {		
	if( !aSwitch->latencyStats.switchLatency.number )
	  return(0);

	printf("\n");
	printf("  Latency statistics\n");
	printf("  ------------------\n");
	printf("                       Avg       SD     Number\n");
	printf("                     -------------------------\n");
	printStat(stdout, "Input Latency: ", 
		  &aSwitch->latencyStats.inputLatency);
	printStat(stdout, "Fabric Latency: ", 
		  &aSwitch->latencyStats.fabricLatency);
	printStat(stdout, "Output Latency: ", 
		  &aSwitch->latencyStats.outputLatency);
	printStat(stdout, "Switch Latency: ", 
		  &aSwitch->latencyStats.switchLatency);

	printf("\n");
	if(aSwitch->numPriorities > 1)
	  {
	    printf("UNICAST:\n");
	    printf("    Input latency per priority:---->\n");
	    for(ip = 0; ip < aSwitch->numPriorities; ip++)
	      {
		printf("     %d", ip);
		printStat(stdout, "",
			  &aSwitch->latencyStats.priInputLatencyUcast[ip]);
	      }
	    printf("MULTICAST:\n");
	    printf("    Input latency per priority:---->\n");
	    for(ip = 0; ip < aSwitch->numPriorities; ip++)
	      {
		printf("     %d", ip);
		printStat(stdout, "",
			  &aSwitch->latencyStats.priInputLatencyMcast[ip]);
	      }
	    printStat(stdout, "Avg: ", &aSwitch->latencyStats.avgInputLatency);
	    printf("\n");

	    printf("UNICAST:\n");
	    printf("    Output latency per priority:---->\n");
	    for(ip = 0; ip < aSwitch->numPriorities; ip++)
	      {
		printf("     %d", ip);
		printStat(stdout, "",&aSwitch->latencyStats.priOutputLatencyUcast[ip]);
	      }
	    printf("MULTICAST:\n");
	    printf("    Output latency per priority:---->\n");
	    for(ip = 0; ip < aSwitch->numPriorities; ip++)
	      {
		printf("     %d", ip);
		printStat(stdout, "",&aSwitch->latencyStats.priOutputLatencyMcast[ip]);
	      }
	    printStat(stdout, "Avg: ", &aSwitch->latencyStats.avgOutputLatency);
	  }

	printf("\n");

/*
	printf("    Scheduling latency:---->\n");
	for(ip = 0; ip < aSwitch->numInputs; ip++)
	  {
	    printf("      %d ", ip);
	    printStat(stdout, "",
		      &aSwitch->latencyStats.schedulingLatency[ip]);
	  }
	printStat(stdout, "Avg: ", &aSwitch->latencyStats.avgSchLatency);

	printf("\n");
	printf("    HOL latency:---->\n");
	for(ip = 0; ip < aSwitch->numInputs; ip++)
	  {
	    printf("      %d ", ip);
	    printStat(stdout, "",
		      &aSwitch->latencyStats.holLatency[ip]);
	  }
	printStat(stdout, "Avg: ", &aSwitch->latencyStats.avgHolLatency);
*/

			free(aSwitch->latencyStats.schedulingLatency);
			free(aSwitch->latencyStats.holLatency);
			free(aSwitch->latencyStats.priInputLatencyUcast);
			free(aSwitch->latencyStats.priOutputLatencyUcast);
			free(aSwitch->latencyStats.priInputLatencyMcast);
			free(aSwitch->latencyStats.priOutputLatencyMcast);

	break;
      }
    case LATENCY_STATS_CELL_PRINT:
      {		
	printf("-----------------------------\n");
	printStat(stdout, "Total Latency over all cells: ", &totalLatency);
	printf("-----------------------------\n");
	break;
      }
    default:
      fprintf(stderr, "Illegal LatenyStatsCommand: %d\n", mode);
      exit(1);
      break;
    }
  return (0);
}


#ifdef _SIM_

// All the remaining are the Sim Graph stuff SG

  /************************************************************/
  /***** SIMGRAPH functions to poll latency   (AMR)************/
  /************************************************************/

#define TOTAL_LATENCY_CODE 0x010

/*******************************************************************************
  latency_get_object_value(struct GRAPH *graph, 
                       unsigned object, unsigned objectNum, 
                       unsigned *value, unsigned *length)

  Reads the value of object objectNum, and then returns its value and length.
  If no error occurs, a NULL is returned.
*******************************************************************************/
int latency_get_object_value(struct GRAPH *graph,
			     unsigned object, unsigned objectNum, 
			     graph_value_t *value, unsigned *value_type, 
			     unsigned *length)
{

  assert((object & BASE_MASK) == LATENCY_BASE);

#ifdef DEBUG_GRAPH
  printf("LATENCY: latency_get_object_value.\n");
#endif
  
  switch(object & VARIABLE_MASK) {

  case TOTAL_LATENCY_CODE:

  /* 
	sprintf(value->doubleValueMaxMinMeanSd_.string_, 
		"%g\n%g\n%g\
		n%g\n%g\n",

		(double)returnLastStat(&totalLatency),
		(double)returnMaxStat(&totalLatency),
		(double)returnMinStat(&totalLatency),
		(double)returnAvgStat(&totalLatency),
		(double)returnSDStat(&totalLatency) );
  */

	sprintf(value->doubleValueMaxMinMeanSd_.string_, 
		"%g\n",(double)returnAvgStat(&totalLatency) );

	*value_type = GRAPH_DOUBLE_VALUE_MEAN_SD;
	*length = strlen(value->doubleValueMaxMinMeanSd_.string_);
	
	return 0;
	break;
	
  default:
	err_ret("LATENCY: this object (0x%x) can't be queried\n", object);
	return GRAPH_UNKNOWN_OBJECT;
	break;
  }
  return(0);
}

/*******************************************************************************
  latency_get_object_name(struct GRAPH *graph, 
						   char *nameStr, int *length)

  Writes the variables that this modules supports into nameStr if there is
  enough space left.
*******************************************************************************/
void latency_get_object_name(struct GRAPH *graph,
			     char *nameStr, int *length)
{
  char str[MAX_STRING_LEN];
  int strLen;

  sprintf(str, "%d, %d, LATENCY: TOTAL_LATENCY\n", 
		  LATENCY_BASE, TOTAL_LATENCY_CODE );
  strLen = strlen(str);
  if (*length - strLen > 0) {
	strcat(nameStr, str);
	*length -= strLen;
  }

#ifdef DEBUG_GRAPH
  printf("LATENCY: latency_get_object_name.\n");
#endif

}

#endif // _SIM_
