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

#include <math.h>
#include "sim.h"

#ifdef _SIM_

#include "graph.h"  //SG
#include <assert.h> //SG
#include <string.h> //SG

#endif // _SIM_


#define INPUT_FIFO_NUM_BINS 256
#define INPUT_FIFO_BIN_STEPSIZE (double) 1.0
#define INPUT_FIFO_MIN_BIN (double) 1.0

/**************************************************************/
/***** General stats routines: switch stats and cell stats ****/
/**************************************************************/

void switchStats( mode, aSwitch, input )
  SwitchStatsCommand mode;
Switch *aSwitch;
int input;
{
  int output;
  char title[256];
  int numFIFOs, priority, pri;

  switch( mode )
    {
    case SWITCH_STATS_RESET:
      {
	int  allOutputsEnabled=0;
	
	/*******************************************************/
	/* Reset stats/ histograms for all input fifos.        */
	/* Note: This does *NOT* enable any stats/histograms.  */
	/*******************************************************/
	for(input=0; input<aSwitch->numInputs; input++)
	  {
	    numFIFOs = aSwitch->numOutputs * aSwitch->numPriorities;
	    for(output=0; output<numFIFOs; output++)
	      {
		/****** Reset *******/
		resetListStats( 
			       aSwitch->inputBuffer[input]->fifo[output],
			       LIST_STATS_ARRIVALS, now);
		resetListHistogram(
				   aSwitch->inputBuffer[input]->fifo[output],
				   LIST_HISTOGRAM_ARRIVALS);
		resetListStats( 
			       aSwitch->inputBuffer[input]->fifo[output],
			       LIST_STATS_DEPARTURES, now);
		resetListHistogram(
				   aSwitch->inputBuffer[input]->fifo[output],
				   LIST_HISTOGRAM_DEPARTURES);
		resetListStats( 
			       aSwitch->inputBuffer[input]->fifo[output],
			       LIST_STATS_LATENCY, now);
		resetListHistogram(
				   aSwitch->inputBuffer[input]->fifo[output],
				   LIST_HISTOGRAM_LATENCY);
		resetListStats( 
			       aSwitch->inputBuffer[input]->fifo[output],
			       LIST_STATS_TA_OCCUPANCY, now);

	      }
	    /* MULTICAST */
	    for(priority=0;priority<aSwitch->numPriorities; priority++)
	      {    
		resetListStats( 
			   aSwitch->inputBuffer[input]->mcastFifo[priority],
                           LIST_STATS_ARRIVALS, now);
		resetListHistogram(
			       aSwitch->inputBuffer[input]->mcastFifo[priority],
			       LIST_HISTOGRAM_ARRIVALS);
		resetListStats( 
			   aSwitch->inputBuffer[input]->mcastFifo[priority],
			   LIST_STATS_DEPARTURES, now);
		resetListHistogram(
			       aSwitch->inputBuffer[input]->mcastFifo[priority],
			       LIST_HISTOGRAM_DEPARTURES);
		resetListStats( 
			       aSwitch->inputBuffer[input]->mcastFifo[priority],
                           LIST_STATS_TA_OCCUPANCY, now);
		resetListHistogram(
				   aSwitch->inputBuffer[input]->mcastFifo[priority],
				   LIST_HISTOGRAM_TIME);
		resetListStats( 
			       aSwitch->inputBuffer[input]->mcastFifo[priority],
                           LIST_STATS_LATENCY, now);
		resetListHistogram(
			       aSwitch->inputBuffer[input]->mcastFifo[priority],
			       LIST_HISTOGRAM_LATENCY);

	      }
	  }
	/****************************************************/
	/* Reset stats and histograms for all output fifos. */
	/****************************************************/
	for(output=0; output<aSwitch->numOutputs; output++)
	  for(priority=0; priority<aSwitch->numPriorities; priority++)
	  {
	    /****** Reset *********/
	    resetListStats( 
			   aSwitch->outputBuffer[output]->fifo[priority],
			   LIST_STATS_ARRIVALS, now);
	    resetListHistogram(
			       aSwitch->outputBuffer[output]->fifo[priority],
			       LIST_HISTOGRAM_ARRIVALS);
	    resetListStats( 
			   aSwitch->outputBuffer[output]->fifo[priority],
			   LIST_STATS_TA_OCCUPANCY, now);
	    resetListHistogram(
			       aSwitch->outputBuffer[output]->fifo[priority],
			       LIST_HISTOGRAM_TIME);
	    resetListStats( 
			   aSwitch->outputBuffer[output]->fifo[priority],
			   LIST_STATS_LATENCY, now);
	    resetListHistogram(
			       aSwitch->outputBuffer[output]->fifo[priority],
			       LIST_HISTOGRAM_LATENCY);
	  }
				

	/* Reset stats for aggregates at input and ouput buffers. */
	for(input=0; input<aSwitch->numInputs; input++)
	  {
	    initStat(&aSwitch->inputBuffer[input]->bufferStats, 
		     STAT_TYPE_AVERAGE, now);
	    aSwitch->inputBuffer[input]->numOverflows = 0;
	    /*** Input Buffer Arrival Occupancy Histogram ***/
	    sprintf(title,"Aggregated Time Avg Arrival Occupancy for Input Buffer %d", input);
	    initHistogram(
			  title,
			  &aSwitch->inputBuffer[input]->bufferHistogram,
			  HISTOGRAM_STATIC, HISTOGRAM_INTEGER_LIMIT,
			  HISTOGRAM_STEP_LINEAR,
			  INPUT_FIFO_NUM_BINS, INPUT_FIFO_MIN_BIN,
			  INPUT_FIFO_BIN_STEPSIZE, 0, (void (*)()) NULL);
	    resetHistogram(
			   &aSwitch->inputBuffer[input]->bufferHistogram);
	    /* find out if we need to enable the histogram. We do that
	       only if all of the LIST_HISTOGRAM_TIME histograms
	       (input,*) have been enabled */
	    allOutputsEnabled = 1;
	    for (output = 0; output < aSwitch->numOutputs; output++)
	      {
		if (!LIST_HISTOGRAM_IS_ENABLED(aSwitch->inputBuffer[input]->fifo[output], LIST_HISTOGRAM_TIME)) 
		  {
		    allOutputsEnabled = 0;
		    break;
		  }
	      }
	    if (allOutputsEnabled)
	      enableHistogram(&aSwitch->inputBuffer[input]->bufferHistogram); 
	  }
	initStat(&aSwitch->aggregateInputBufferStats, 
		 STAT_TYPE_AVERAGE, now);
	initStat(&aSwitch->aggregateOutputBufferStats,
		 STAT_TYPE_AVERAGE, now);
	initStat(&aSwitch->aggregateBufferStats,
		 STAT_TYPE_AVERAGE, now);

	break;
      }
    case SWITCH_STATS_NEW_ARRIVAL:
      /* Called when new cell arrives at input buffer */
			/*
      {
	int sum=0;
	union StatsValue statsValue;

	for(priority=0;priority<aSwitch->numPriorities; priority++)
	  sum += aSwitch->inputBuffer[input]->mcastFifo[priority]->number;
	 
	numFIFOs = aSwitch->numOutputs * aSwitch->numPriorities;
	for( output=0; output<numFIFOs; output++)
	  {
	    sum+= aSwitch->inputBuffer[input]->fifo[output]->number;
	  }
	statsValue.anInt = sum;
	updateHistogram(&aSwitch->inputBuffer[input]->bufferHistogram,
			statsValue, 0);
      }
			*/

      break;
    case SWITCH_STATS_PER_CELL_UPDATE:
      /* Called at the end of every cell time */
      {
	long num;
	long inputSum;

	long allInputsSum=0;
	long allOutputsSum=0;
	int  allInputsEnabled=0, allOutputsEnabled=0;
	struct List *fifo;
	union StatsValue statsValue;

	/**************************************************/
	/************* Input Buffer Stats *****************/
	/**************************************************/
	/* Update time average histograms for FIFO lists. */
	for(input=0; input<aSwitch->numInputs; input++)	
	  {
	    numFIFOs = aSwitch->numOutputs * aSwitch->numPriorities;
	    for(output=0; output<numFIFOs; output++)
	      {
		updateListHistogram(
				    aSwitch->inputBuffer[input]->fifo[output], 
				    LIST_HISTOGRAM_TIME,
				    aSwitch->inputBuffer[input]->fifo[output]->number);
	      }

	    for(priority=0;priority>aSwitch->numPriorities; priority++)
	      updateListHistogram(
				  aSwitch->inputBuffer[input]->mcastFifo[priority], 
				  LIST_HISTOGRAM_TIME,
				  aSwitch->inputBuffer[input]->mcastFifo[priority]->number);
	  }

	/* Find sum of occupancy of all fifos for this switch. */
	allInputsEnabled = 1;
	for(input=0; input<aSwitch->numInputs; input++)	
	  {
	    /* Find sum of occupancy of all fifos at this input. */
	    inputSum=0; 
	    allOutputsEnabled = 1;
	    numFIFOs = aSwitch->numOutputs * aSwitch->numPriorities;
	    for(output=0; output<numFIFOs; output++)
	      {
		fifo = aSwitch->inputBuffer[input]->fifo[output];
		num = fifo->number;
		inputSum += num;
		if(LIST_STAT_IS_ENABLED(fifo, LIST_STATS_TA_OCCUPANCY) )
		  {
		        updateListStats(aSwitch->inputBuffer[input]->fifo[output],
                LIST_STATS_TA_OCCUPANCY,num, now);	
		  }
		else
		  {
		    allInputsEnabled = 0;
		    allOutputsEnabled = 0;
		  }
	      }
	    if( allOutputsEnabled )
	      {
		updateStat(&aSwitch->inputBuffer[input]->bufferStats,
			   inputSum, now);

		allInputsSum += inputSum;
	      }
	    if (aSwitch->inputBuffer[input]->bufferHistogram.enable)
	      {
		statsValue.aDouble = 0.0;
		statsValue.aPtr = NULL;    /* Sundar, changed the input order */
															/* This is a quick fix. Should be looked
																 into */
		statsValue.anInt = inputSum;

		updateHistogram(&aSwitch->inputBuffer[input]->bufferHistogram,
				statsValue, 0);
	      }
	  } /* end of for(input=...) */
	if( allInputsEnabled )
	  updateStat(&aSwitch->aggregateInputBufferStats, 
		     allInputsSum, now);

	/**************************************************/
	/************ Output Buffer Stats *****************/
	/**************************************************/
	for(output=0; output<aSwitch->numOutputs; output++)	
	  for(priority=0;priority<aSwitch->numPriorities; priority++)
	  {
	    num = aSwitch->outputBuffer[output]->fifo[priority]->number;
	    allOutputsSum += num;
	    updateListStats(aSwitch->outputBuffer[output]->fifo[priority],
			    LIST_STATS_TA_OCCUPANCY,num, now);
	    updateListHistogram(aSwitch->outputBuffer[output]->fifo[priority],
				LIST_HISTOGRAM_TIME, num);
	    
	    if(!LIST_STAT_IS_ENABLED(aSwitch->outputBuffer[output]->fifo[priority],
				     LIST_STATS_TA_OCCUPANCY) )
	      allOutputsEnabled=0;
	  }
	if( allOutputsEnabled )
	  updateStat(&aSwitch->aggregateOutputBufferStats, 
		     allOutputsSum, now);

	if( allInputsEnabled && allOutputsEnabled )
	  updateStat(&aSwitch->aggregateBufferStats, 
		     allInputsSum + allOutputsSum, now);

	break;
      }
    case SWITCH_STATS_PRINT_ALL:
      {
	double EX, EX2, SD;
	double inputEX, inputEX2, inputSD;
	double switchEX, switchEX2, switchSD;
	int allInputsEnabled, allOutputsEnabled,statNotEnabled;
	int type, anyHistogramEnabled;
	struct List *fifo;

	double discrepancy; /* Measure of spread of avg buffer lengths*/
	double sumAvgs=0.0, sumsqAvgs=0.0;

	long duration = now - resetStatsTime;

	/* Avoid divide by zero errors */	
	if( !duration )	
	  {
	    printf("Duration is zero\n");
	    break;
	  }

	/***************************************************/
	/************* Input Buffer Stats *****************/
	/***************************************************/
	for(type=0; type<NUM_LIST_STATS_TYPES; type++)
	  {
	    printf("\n");
	    switch(type)
	      {
	      case LIST_STATS_TA_OCCUPANCY:
		printf("  INPUT BUFFER TIME AVG OCCUPANCY STATS\n");
		printf("  -------------------------------------\n");
		break;
	      case LIST_STATS_ARRIVALS:
		printf("  INPUT BUFFER ARRIVAL STATS\n");
		printf("  --------------------------\n");
		break;
	      case LIST_STATS_DEPARTURES:
		printf("  INPUT BUFFER DEPARTURE STATS\n");
		printf("  ----------------------------\n");
		break;
	      case LIST_STATS_LATENCY:
		printf("  INPUT BUFFER LATENCY STATS\n");
		printf("  --------------------------\n");
		break;
	      }
	    printf("  I/P O/P   Pri  Avg   SD\n");
	    printf("  -----------------------\n");
	
	    allInputsEnabled = 1;
	    statNotEnabled = 1;
	    sumAvgs = 0; sumsqAvgs = 0;
	    for(input=0; input<aSwitch->numInputs; input++)
	      {

		allOutputsEnabled = 1;
		for(output=0; output<aSwitch->numOutputs; output++)
		  for(priority=0;priority<aSwitch->numPriorities; priority++)
		    {
		      pri = aSwitch->numPriorities * output + priority;
		      fifo = aSwitch->inputBuffer[input]->fifo[pri];
		      if( fifo->listStats[type].enable )
			{
			  EX  = returnAvgStat(&fifo->listStats[type]);
			  EX2 = returnEX2Stat(&fifo->listStats[type]);
			  SD  = safe_sqrt(EX2 -  EX*EX);
			  printf("  %3d %3d %3d   %.5f %.5f (%ld)\n", 
			       input, output, priority, EX, SD, 
                   returnNumberStat(&fifo->listStats[type]));
			  
			  /* "discrepancy" is the SD of the means. */
			  sumAvgs += EX;
			  sumsqAvgs += EX*EX;
			statNotEnabled = 0;
			}
		      else
			allOutputsEnabled = 0;
		    }
		
		/* If all output fifos for this input were enabled, */
		/* print the average over this input. */
		if( (type == LIST_STATS_TA_OCCUPANCY) 
		    && allOutputsEnabled )
		  {
		    inputEX = returnAvgStat(
					    &aSwitch->inputBuffer[input]->bufferStats);
		    inputEX2 = returnEX2Stat(
					     &aSwitch->inputBuffer[input]->bufferStats);
		    inputSD = safe_sqrt(inputEX2 - inputEX*inputEX);
		    printf("  %3d   X %.3f %.3f (%ld)\n",input,inputEX,inputSD,
                returnNumberStat(&aSwitch->inputBuffer[input]->bufferStats));
		  }
		else
		  allInputsEnabled = 0;
	
	      }
	    if( (type == LIST_STATS_TA_OCCUPANCY) && 
		allInputsEnabled )
	      {
		switchEX = returnAvgStat(
					 &aSwitch->aggregateInputBufferStats);
		switchEX2 = returnEX2Stat(
					  &aSwitch->aggregateInputBufferStats);
		switchSD = safe_sqrt(switchEX2 - switchEX*switchEX);
		printf("    X   X %.3f %.3f (%ld)\n\n",
		       switchEX, switchSD, returnNumberStat(
                                     &aSwitch->aggregateInputBufferStats));
		sumAvgs /= aSwitch->numInputs * aSwitch->numOutputs;
		sumsqAvgs /= aSwitch->numInputs * aSwitch->numOutputs;
		discrepancy = safe_sqrt( sumsqAvgs - sumAvgs*sumAvgs );
		printf("    SD of mean buffer occupancies: %f\n\n", discrepancy);
	      }

	    /* MULTICAST */
	    for(input=0;input<aSwitch->numInputs;input++)
	      for(priority=0;priority<aSwitch->numPriorities; priority++)
		{
		  fifo = aSwitch->inputBuffer[input]->mcastFifo[priority];
		  if( fifo->listStats[type].enable )
		    {
		      EX  = returnAvgStat(&fifo->listStats[type]);
		      EX2 = returnEX2Stat(&fifo->listStats[type]);
		      SD  = safe_sqrt(EX2 -  EX*EX);
		      printf("  %3d  M  %.3f %.3f (%ld)\n", 
			   input, EX, SD, returnNumberStat(&fifo->listStats[type]));
		    statNotEnabled = 0;
		  }
	      }


	    if( statNotEnabled )
	      printf("    No Stats Enabled. \n");
						
	  }

	printf("\n");
	/***************************************************/
	/************* Input Buffer Histograms *************/
	/***************************************************/
	for(type=0; type<NUM_LIST_HISTOGRAM_TYPES; type++)
	  {
	    printf("\n");
	    switch(type)
	      {
	      case LIST_HISTOGRAM_TIME:
		printf("  INPUT BUFFER TIME AVG OCCUPANCY HISTOGRAMS\n");
		printf("  -------------------------------------\n");
		break;
	      case LIST_HISTOGRAM_ARRIVALS:
		printf("  INPUT BUFFER ARRIVAL HISTOGRAMS\n");
		printf("  --------------------------\n");
		break;
	      case LIST_HISTOGRAM_DEPARTURES:
		printf("  INPUT BUFFER DEPARTURE HISTOGRAMS\n");
		printf("  ----------------------------\n");
		break;
	      case LIST_HISTOGRAM_LATENCY:
		printf("  INPUT BUFFER LATENCY HISTOGRAMS\n");
		printf("  --------------------------\n");
		break;
	      }
	    anyHistogramEnabled = 0;
	    for(input=0; input<aSwitch->numInputs; input++)
	      {
		for(output=0; output<aSwitch->numOutputs*aSwitch->numPriorities; output++)
		  {
		    fifo = aSwitch->inputBuffer[input]->fifo[output];
		    if( LIST_HISTOGRAM_IS_ENABLED(fifo, type) )
		      {
			printf("\n");
			printListHistogram(stdout, fifo, type);
			anyHistogramEnabled = 1;
		      }
		  }
		for(priority=0;priority<aSwitch->numPriorities;priority++) 
		  {
		    fifo = aSwitch->inputBuffer[input]->mcastFifo[priority];
		    if( LIST_HISTOGRAM_IS_ENABLED(fifo, type) )
		      {
			printf("\n");
			printListHistogram(stdout, fifo, type);
			anyHistogramEnabled = 1;
		      }
		  }
		if (type == LIST_HISTOGRAM_TIME && aSwitch->inputBuffer[input]->bufferHistogram.enable)
		  printHistogram(stdout, &(aSwitch->inputBuffer[input]->bufferHistogram));
	      } /* endfor (input=...) */
	    if (anyHistogramEnabled == 0)
	      printf("    No Histograms Enabled. \n");
	    
	  } /* for (type=...) */

	/***************************************************/
	/************* Output Buffer Stats *****************/
	/***************************************************/
	for(type=0; type<NUM_LIST_STATS_TYPES; type++)
	  {
	    printf("\n");
	    switch(type)
	      {
	      case LIST_STATS_TA_OCCUPANCY:
		printf("  OUTPUT BUFFER TIME AVG OCCUPANCY STATS\n");
		printf("  --------------------------------------\n");
		break;
	      case LIST_STATS_ARRIVALS:
		printf("  OUTPUT BUFFER ARRIVAL STATS\n");
		printf("  ---------------------------\n");
		break;
	      case LIST_STATS_DEPARTURES:
		printf("  OUTPUT BUFFER DEPARTURE STATS\n");
		printf("  -----------------------------\n");
		break;
	      case LIST_STATS_LATENCY:
		printf("  OUTPUT BUFFER LATENCY STATS\n");
		printf("  ---------------------------\n");
		break;
	      }

	    printf("  O/P   Avg   SD\n");
	    printf("  --------------\n");
	
	    allOutputsEnabled = 1;
	    statNotEnabled = 1;
	    sumAvgs = 0; sumsqAvgs = 0;
	    for(output=0; output<aSwitch->numOutputs; output++)
	      for(priority=0;priority<aSwitch->numPriorities; priority++)
	      {
		fifo = aSwitch->outputBuffer[output]->fifo[priority];
		if( LIST_STAT_IS_ENABLED(fifo, type) )
		  {
		    EX   = returnAvgStat(&fifo->listStats[type]);
		    EX2  = returnEX2Stat(&fifo->listStats[type]);
		    SD  = safe_sqrt(EX2 -  EX*EX);
		    printf("  %3d %.3f %.3f (%ld)\n", output, EX, SD,
                returnNumberStat(&fifo->listStats[type]));
	
		    /* "discrepancy" is the SD of the means. */
		    sumAvgs += EX;
		    sumsqAvgs += EX*EX;
		    statNotEnabled = 0;
		  }
		else
		  allOutputsEnabled = 0;
	      }
	
	    if( (type == LIST_STATS_TA_OCCUPANCY) && allInputsEnabled )
	      {
		switchEX = returnAvgStat(
					 &aSwitch->aggregateOutputBufferStats);
		switchEX2 = returnEX2Stat(
					  &aSwitch->aggregateOutputBufferStats);
		switchSD = safe_sqrt(switchEX2 - switchEX*switchEX);
		printf("    X %.3f %.3f (%ld)\n\n", switchEX, switchSD, 
            returnNumberStat(&aSwitch->aggregateOutputBufferStats));
		
		sumAvgs /= aSwitch->numInputs * aSwitch->numOutputs;
		sumsqAvgs /= aSwitch->numInputs * aSwitch->numOutputs;
		discrepancy = safe_sqrt( sumsqAvgs - sumAvgs*sumAvgs );
		printf("    SD of mean buffer occupancies: %f\n\n", 
		       discrepancy);
	      }
	    if( statNotEnabled )
	      printf("    No Stats Enabled. \n");
	  }
					
	/***************************************************/
	/************* Output Buffer Histograms *************/
	/***************************************************/
	for(type=0; type<NUM_LIST_HISTOGRAM_TYPES; type++)
	  {
	    for(output=0; output<aSwitch->numOutputs; output++)
	      for(priority=0;priority<aSwitch->numPriorities; priority++)
	      {
		fifo = aSwitch->outputBuffer[output]->fifo[priority];
		if( LIST_HISTOGRAM_IS_ENABLED(fifo, type) )
		  printListHistogram(stdout, fifo, type);
	      }
	  }

	/****************************************************/
	/***** Switch overall buffer occupancy stats ********/
	/****************************************************/
	if( allOutputsEnabled && allInputsEnabled )
	  {
	    printf("  SWITCH OVERALL BUFFER OCCUPANCY STATS\n");
	    printf("  -------------------------------------\n");
	    printf("  Avg   SD\n");
	    printf("  --------\n");
	    switchEX = returnAvgStat(&aSwitch->aggregateBufferStats);
	    switchEX2 = returnEX2Stat(&aSwitch->aggregateBufferStats);
	    switchSD = safe_sqrt(switchEX2 - switchEX*switchEX);
	    printf(" %.3f %.3f (%ld)\n", switchEX, switchSD,
            returnNumberStat(&aSwitch->aggregateBufferStats));
	  }
      
	break;
      }
    }
}
/**********************************************************************/
void burstStats( mode, aSwitch, output, input )
  BurstStatsCommand mode;
Switch *aSwitch;
int output, input;
{
  switch( mode )
    {
    case BURST_STATS_RESET:
      {
	BurstStat *burstStat;
	/* Init burstiness stats */
	burstStat = &aSwitch->outputBuffer[output]->burstinessStats;
	burstStat->lastValue = NONE;
	burstStat->currentBurstLength = 0;
	initStat(&burstStat->stats, STAT_TYPE_AVERAGE, now);
	break;
      }
    case BURST_STATS_UPDATE:
      /* Called from outputAction */
      {
	/**************************************************/
	/************ Output Burstiness Stats *************/
	/**************************************************/
	/*   If cell is full and same source, burstlength++
	     If cell is full but different source, 	
	     add burstlength to total, 
	     */
	BurstStat *burstStat;

	burstStat = &aSwitch->outputBuffer[output]->burstinessStats;
	/* From same input as last time ? */
	if(input == NONE) 
	  {
	    if( burstStat->lastValue != NONE )
	      updateStat(&burstStat->stats, 
			 burstStat->currentBurstLength, now);
	    burstStat->currentBurstLength = 0;
	    burstStat->lastValue = NONE;
	  }
	else if (input != burstStat->lastValue) 
	  {
	    if( burstStat->lastValue != NONE )
	      updateStat(&burstStat->stats, 
			 burstStat->currentBurstLength, now);
	    burstStat->currentBurstLength = 1;
	    burstStat->lastValue = input;
	  }
	else
	  burstStat->currentBurstLength++;
	break;		
      }
    case BURST_STATS_PRINT:
      {
	BurstStat *burstStat;
	double EX, EX2, SD;

	/***************************************************/
	/************* Output Burstiness Stats *************/
	/***************************************************/
	burstStat = &aSwitch->outputBuffer[output]->burstinessStats;

	if( burstStat->stats.number )
	  {
	    EX  = returnAvgStat(&burstStat->stats);
	    EX2 = returnEX2Stat(&burstStat->stats);
	    SD = safe_sqrt( EX2 - (EX*EX));
	  }
	else
	  EX = EX2 = SD = 0.0;

	printf("  %3d    %.3f   %.3f\n", output, EX, SD);
	break;
      }
    }
}

#ifdef _SIM_

// All the remaining is Sim Graph Code. SG

  /************************************************************/
  /***** SIMGRAPH functions to poll occupancy (AMR)************/
  /************************************************************/

#define OCCUPANCY_PORT_CODE 0x010

/*******************************************************************************
  occupancy_get_object_value(struct GRAPH *graph, 
                       unsigned object, unsigned objectNum, 
                       unsigned *value, unsigned *length)

  Reads the value of object objectNum, and then returns its value and length.
  If no error occurs, a NULL is returned.
*******************************************************************************/
int occupancy_get_object_value(Switch *aSwitch,
			     unsigned object, unsigned objectNum, 
			     graph_value_t *value, unsigned *value_type, 
			     unsigned *length)
{
  int port;
  assert((object & BASE_MASK) == OCCUPANCY_BASE);
  port = (object & VARIABLE_MASK) - OCCUPANCY_PORT_CODE;  

#ifdef DEBUG_GRAPH
  printf("OCCUPANCY: occupancy_get_object_value.\n");
#endif

  
  if( (port >= 0) &&  (port< aSwitch->numInputs) ) {

  /*
	sprintf(value->doubleValueMaxMinMeanSd_.string_, 
		"%g\n%g\n%g\n%g\n%g\n",

		(double)returnLastStat(&aSwitch->inputBuffer[port]->bufferStats),
		(double)returnMaxStat(&aSwitch->inputBuffer[port]->bufferStats),
		(double)returnMinStat(&aSwitch->inputBuffer[port]->bufferStats),
		(double)returnAvgStat(&aSwitch->inputBuffer[port]->bufferStats),
		(double)returnSDStat(&aSwitch->inputBuffer[port]->bufferStats) );
	*/

	sprintf(value->doubleValueMaxMinMeanSd_.string_, 
		"%g\n",(double)returnAvgStat(&aSwitch->inputBuffer[port]->bufferStats) );

	*value_type = GRAPH_DOUBLE_VALUE_MEAN_SD;
	*length = strlen(value->doubleValueMaxMinMeanSd_.string_);

	return 0;
  }
  else {
	err_ret("OCCUPANCY: this object (0x%x) can't be queried\n", object);
	return GRAPH_UNKNOWN_OBJECT;
  }
  return(0);
}

/*******************************************************************************
  occupancy_get_object_name(struct GRAPH *graph, 
						   char *nameStr, int *length)

  Writes the variables that this modules supports into nameStr if there is
  enough space left.
*******************************************************************************/
void occupancy_get_object_name(Switch *aSwitch,
			     char *nameStr, int *length)
{
  char str[MAX_STRING_LEN];
  int strLen;
  int port;

  for (port=0; port< aSwitch->numInputs; port++) {
    
    sprintf(str, "%d, %d, OCCUPANCY: PORT%d\n", 
	    OCCUPANCY_BASE, OCCUPANCY_PORT_CODE+port, port );
    
    strLen = strlen(str);
    if (*length - strLen > 0) {
      strcat(nameStr, str);
      *length -= strLen;
    }
    
  }

#ifdef DEBUG_GRAPH
  printf("OCCUPANCY: occupancy_get_object_name.\n");
#endif


}

#endif // _SIM_
