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
#include "algorithm.h"

typedef struct {
  int *oldestCellFifo;
  int *inputList;
  int maxCells;	/* Maximum number of cells per output buffer. */
} SchedulerState;

/*  Determines switch configuration 					*/
/*  Configuration is filled into aSwitch->fabric.Xbar_matrix array 	*/
/*  Algorithm:
		Look at each output. Determine which inputs have cells for this o/p
        Choose uniformly over those that do.

		To determine single fifo ordering, we check first to see
        which is the cell across the heads of all inpout queues
        that arrived least recently.
*/


void
fifo(action, aSwitch, argc, argv)
  SwitchAction action;
Switch *aSwitch;
int argc;
char **argv;
{
  SchedulerState *scheduleState;
  int input, output;



  if(debug_algorithm)
    printf("Algorithm 'fifo()' called by switch %d\n", aSwitch->switchNumber);
	
  switch(action) {
  case SCHEDULING_USAGE:
    fprintf(stderr, "Options for \"fifo\" scheduling algorithm:\n");
    fprintf(stderr, "  -m maxCells per output buffer. Default: infinite\n");
    break;
  case SCHEDULING_INIT:	
    if(debug_algorithm)
      printf("	INIT\n");

    if( !aSwitch->scheduler.schedulingState )
      {
	extern int opterr;
	extern int optopt;
	extern int optind;
	extern char *optarg;
	int c;


	/* Allocate scheduler state for this switch */
	scheduleState = (SchedulerState *)
	  malloc(sizeof(SchedulerState));
	scheduleState->inputList = (int *) 
	  malloc(aSwitch->numInputs * sizeof(int));
	scheduleState->oldestCellFifo = (int *) 
	  malloc(aSwitch->numInputs * sizeof(unsigned long));
	aSwitch->scheduler.schedulingState = scheduleState;
	scheduleState->maxCells = NONE;

	opterr=0;
	optind=1;
	while( (c = getopt(argc, argv, "m:")) != EOF )
	  switch (c)
	    {
	    case 'm':
	      scheduleState->maxCells = atoi(optarg);
	      break;
	    case '?':
	      fprintf(stderr, "-------------------------------\n");
	      fprintf(stderr, "fifo: Unrecognized option -%c\n", optopt);
	      fifo(SCHEDULING_USAGE);
	      fprintf(stderr, "--------------------------------\n");
	      exit(1);
	    default:	
	      break;
	    } 

	if( scheduleState->maxCells == NONE )
	  printf("Infinite output buffers.\n");
	else
	  printf("Max number of cells per output: %d\n", scheduleState->maxCells);


      }

    break;

  case SCHEDULING_EXEC:
    {
      unsigned long oldestCellTime;
      int numSelectedMe, chosen;
      Cell *aCell;
      int switchOutput, fabricOutput, index;

      if(debug_algorithm)
	printf("	CONFIG\n");

      scheduleState = (SchedulerState *) 
	aSwitch->scheduler.schedulingState;

      /* First look at each input queue and determine the oldest
	 element at the head of the queues */
      for(input=0; input<aSwitch->numInputs; input++)
	{
	  oldestCellTime = now+1;
	  scheduleState->oldestCellFifo[input] = NONE;
	  for(output=0; output<aSwitch->numOutputs; output++)
	    {
	      if( (aSwitch->inputBuffer[input]->fifo[output]->number) &&
		  ( (scheduleState->maxCells == NONE) ||
		    (aSwitch->outputBuffer[output]->fifo[DEFAULT_PRIORITY]->number <
		     scheduleState->maxCells) ) )
		{
		  aCell= (Cell *)
		    aSwitch->inputBuffer[input]->fifo[output]->head->Object;
		  if( aCell->commonStats.arrivalTime < oldestCellTime )
		    {
		      oldestCellTime = aCell->commonStats.arrivalTime;
		      scheduleState->oldestCellFifo[input] = output;
		    }
			
		}
	    }
	}


      /* Foreach output, make a list of inputs requesting it
	 and then choose "numOutputLines"  of them randomly */
      for(switchOutput=0;switchOutput<aSwitch->numOutputs;switchOutput++) 
	{
	  numSelectedMe=0;
	  for(input=0; input<aSwitch->numInputs; input++)
	    {
	      if(scheduleState->oldestCellFifo[input] == switchOutput) 
		{
		  scheduleState->inputList[ numSelectedMe++ ] = input;
		  if(debug_algorithm)
		    printf("Ouput %d: input %d selected me.\n", 
			   switchOutput, input);
		}
					
	    }

	  if(debug_algorithm) 
	    printf("Ouput %d: %d selected me.\n", switchOutput,
		   numSelectedMe);
	  for(index=0; (index<aSwitch->fabric.Xbar_numOutputLines) &&
		index < numSelectedMe; index++)
	    {
	      fabricOutput = (switchOutput * 
			      aSwitch->fabric.Xbar_numOutputLines) + index; 	
	      do {
		chosen = lrand48()%numSelectedMe;	
	      } while(scheduleState->inputList[chosen]==NONE);
	      input = scheduleState->inputList[chosen];
	      aSwitch->fabric.Xbar_matrix[fabricOutput].input = input;
	      aSwitch->fabric.Xbar_matrix[fabricOutput].cell
		= aSwitch->inputBuffer[input]->fifo[fabricOutput]->head->Object;
	      scheduleState->inputList[chosen] = NONE;
	      if(debug_algorithm) 
		printf("Ouput %d: Chose input %d\n", fabricOutput, 
		       input);
	    }
				
	}


      break;
    }
  default:
	 break;
  }


  if(debug_algorithm)
    printf("Algorithm 'fifo()' completed for switch %d\n", aSwitch->switchNumber);
}

/***********************************************************************/
