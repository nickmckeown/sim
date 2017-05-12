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
#include "mcast_slip.h"
#include "scheduleStats.h"

static int selectGrant();
static void createScheduleState();
static InputSchedulerState *createInScheduler();
static OutputSchedulerState *createOutScheduler();

/*  Determines switch configuration 					*/
/*  Configuration is filled into aSwitch->fabric.Xbar_matrix array 	*/

void
mcast_slip(action, aSwitch, argc, argv)
  SwitchAction action;
Switch *aSwitch;
int argc;
char **argv;
{
  SchedulerState 			*scheduleState;

  OutputSchedulerState	*outputSchedule;
  int input, fabricOutput, numFabricOutputs;
  Cell *aCell;
  struct List *fifo;

  if(debug_algorithm)
    printf("Algorithm 'mcast_slip()' called by switch %d\n", aSwitch->switchNumber);
	
  switch(action) {
  case SCHEDULING_USAGE:
    fprintf(stderr, "No options for \"mcast_slip\" algorithm:\n");
    break;
  case SCHEDULING_INIT:	
    if(debug_algorithm) printf("	SCHEDULING_INIT\n");
			
    if( aSwitch->scheduler.schedulingState == NULL )
      {
	extern int opterr;
	extern int optopt;
	extern int optind;
	extern char *optarg;
	int c;


	/* Allocate scheduler state for this switch */
	createScheduleState(aSwitch);
	scheduleState = (SchedulerState *) aSwitch->scheduler.schedulingState;

	opterr=0;
	optind=1;
	while( (c = getopt(argc, argv, "")) != EOF)
	  switch (c)
	    {
	    case '?':
	      fprintf(stderr, "-------------------------------\n");
	      fprintf(stderr, "mcast_slip: Unrecognized option -%c\n", optopt);
	      mcast_slip(SCHEDULING_USAGE);
	      fprintf(stderr, "--------------------------------\n");
	      exit(1);
	    default:
	      break;
	    }


	/* Reset and initialize scheduling stats for this switch. */
	/*            	scheduleCellStats( SCHEDULE_CELL_STATS_INIT, aSwitch ); */
      }

    break;

  case SCHEDULING_EXEC:
    {
      int diff, minDiff;
      if(debug_algorithm) printf("	SCHEDULING_EXEC\n");

      scheduleState=(SchedulerState *)aSwitch->scheduler.schedulingState;
      numFabricOutputs = aSwitch->numOutputs * 
	aSwitch->fabric.Xbar_numOutputLines;

      /*************** INITIALIZE ***************/
      /* Set up outputs */
      for(fabricOutput=0;fabricOutput<numFabricOutputs;fabricOutput++)
	{
	  outputSchedule = scheduleState->outputSched[fabricOutput];
	  outputSchedule->grant=NONE;
	}




      /* Mark cells that have newly arrived at head of queue */
      for(input=0;input < aSwitch->numInputs; input++)
	{
	  fifo = aSwitch->inputBuffer[input]->mcastFifo[DEFAULT_PRIORITY];
	  if(fifo->number)
	    {
	      aCell = (Cell *) fifo->head->Object;
	      if(aCell->commonStats.headArrivalTime==NONE)
		aCell->commonStats.headArrivalTime=now;
	    }
	}

      /***************  END INITIALIZE ***************/

      /*****************************/
      /*****   Main Loop  **********/
      /*****************************/
      for(fabricOutput=0;fabricOutput<numFabricOutputs;fabricOutput++)
	{

	  /* grant an input for this output line */
	  if(debug_algorithm)
	    printf("Selecting grant for output %d\n", fabricOutput);
	  input = selectGrant(aSwitch, fabricOutput);
	  if(debug_algorithm)
	    printf("Selected input %d\n", input);
	  if( input != NONE ) 
	    {
	      outputSchedule = scheduleState->outputSched[fabricOutput];
	      outputSchedule->grant = input;
	    }
	}

      minDiff = aSwitch->numInputs;
      for(fabricOutput=0; fabricOutput<numFabricOutputs; fabricOutput++)
	{
	  outputSchedule = scheduleState->outputSched[fabricOutput];
	  input = outputSchedule->grant;
	  aSwitch->fabric.Xbar_matrix[fabricOutput].input = input;
	  if(input!=NONE)
	    {
	      aSwitch->fabric.Xbar_matrix[fabricOutput].cell = (Cell *)
		aSwitch->inputBuffer[input]->mcastFifo[DEFAULT_PRIORITY]->head->Object;
	      diff = (input + aSwitch->numInputs - scheduleState->pointer)%(aSwitch->numInputs);
	      if(diff<minDiff)
		minDiff=diff;
	    }
	}
      if(debug_algorithm)
	printf("Pointer moved from %d ", scheduleState->pointer);
      scheduleState->pointer = 
	(scheduleState->pointer+minDiff+1)%(aSwitch->numInputs);
      if(debug_algorithm)
	printf("to %d\n", scheduleState->pointer);

      break;
    }

  case SCHEDULING_REPORT_STATS:
    /*			scheduleStats( SCHEDULE_STATS_PRINT_ALL, aSwitch );
			scheduleCellStats( SCHEDULE_CELL_STATS_PRINT_ALL, aSwitch ); */
    break;
  case SCHEDULING_INIT_STATS:
    /*			scheduleStats( SCHEDULE_STATS_INIT, aSwitch );
			scheduleCellStats( SCHEDULE_CELL_STATS_INIT, aSwitch );*/
    break;

  default:
    break;
  }

	

  if(debug_algorithm)
    printf("Algorithm 'mcast_slip()' completed for switch %d\n", aSwitch->switchNumber);
}

/***********************************************************************/
static int
selectGrant(aSwitch, output)
  Switch *aSwitch;
int output;
{
  /*
    Select next requesting input from schedule
    */
  int input;
  SchedulerState	*scheduleState =
    (SchedulerState *)aSwitch->scheduler.schedulingState;
  OutputSchedulerState *outputSchedule=scheduleState->outputSched[output];
  InputBuffer  *inputBuffer;
  Cell *aCell;

  int increment;
  int sched_input;


	

	/******************************************/
	/* Grant to first input at or after the pointer */
	/******************************************/

  for(increment=0; increment < aSwitch->numInputs; increment++)
    {
      sched_input = (scheduleState->pointer + increment)%(aSwitch->numInputs);
      input = outputSchedule->RR_grants[sched_input];
      inputBuffer = aSwitch->inputBuffer[input];
      if( inputBuffer->mcastFifo[DEFAULT_PRIORITY]->number > 0 ) 
	{
	  aCell = (Cell *) inputBuffer->mcastFifo[DEFAULT_PRIORITY]->head->Object;
	  if(bitmapIsBitSet(output, &aCell->outputs))
	    {
	      /* STATS */
	      /* Mark grantTime for cell at head of line */
	      /* scheduleCellStats( SCHEDULE_CELL_STATS_UPDATE_GRANT, 
		 aSwitch, aCell);*/
	      /* END STATS */
	
	      if(debug_algorithm)
		printf("Output %d granting to input %d\n", output, input);
	      return(input);
	    }
	}
    }

  if(debug_algorithm)
    printf("No inputs requested output %d. None granted.\n", output);

  return(NONE);
}


/* Create schedule state variables for aSwitch */
static void
createScheduleState(aSwitch)
  Switch *aSwitch;
{
  SchedulerState *scheduleState;


  int input, output, numFabricOutputs;

  numFabricOutputs = aSwitch->numOutputs*aSwitch->fabric.Xbar_numOutputLines;
	
  scheduleState = (SchedulerState *) malloc(sizeof(SchedulerState));
  aSwitch->scheduler.schedulingState = scheduleState;

	
  scheduleState->pointer = 0;

  scheduleState->inputSched = (InputSchedulerState **) 
    malloc( aSwitch->numInputs * sizeof(InputSchedulerState *) );
  for(input=0; input<aSwitch->numInputs; input++)
    scheduleState->inputSched[input] = createInScheduler(aSwitch, input);

  scheduleState->outputSched = (OutputSchedulerState **) 
    malloc( numFabricOutputs * sizeof(OutputSchedulerState *) );
  for(output=0; output<numFabricOutputs; output++)
    scheduleState->outputSched[output]=createOutScheduler(aSwitch, output);
	
}

static InputSchedulerState *
createInScheduler(aSwitch, input)
  Switch *aSwitch;
int input;
{	
  SchedulerState 		*scheduleState=aSwitch->scheduler.schedulingState;
  InputSchedulerState	**inputSchedule=scheduleState->inputSched;
  int i, numFabricOutputs;

  numFabricOutputs = aSwitch->numOutputs*aSwitch->fabric.Xbar_numOutputLines;

  inputSchedule[input] = (InputSchedulerState *) 
    malloc( sizeof( InputSchedulerState ) );
  inputSchedule[input]->RR_accepts = 
    (int *) malloc( sizeof(int)*numFabricOutputs);
  for(i=0; i<numFabricOutputs; i++ )
    inputSchedule[input]->RR_accepts[i] = i;

  return(inputSchedule[input]);
	
}

static OutputSchedulerState *
createOutScheduler(aSwitch, output)
  Switch *aSwitch;
int output;
{
  SchedulerState 			*scheduleState=aSwitch->scheduler.schedulingState;
  OutputSchedulerState 	**outputSchedule=scheduleState->outputSched;
  int i;

  outputSchedule[output] = (OutputSchedulerState *) malloc( sizeof( OutputSchedulerState ) );

  outputSchedule[output]->RR_grants = (int *) malloc( sizeof(int)*aSwitch->numInputs);
  for(i=0; i<aSwitch->numInputs; i++ )
    outputSchedule[output]->RR_grants[i] = i;

  return(outputSchedule[output]);
}
