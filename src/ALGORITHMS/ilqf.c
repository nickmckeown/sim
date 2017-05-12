
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
#include "ilqf.h"
#include "scheduleStats.h"

static int selectGrant();
static int selectAccept();
static void createScheduleState();
static InputSchedulerState *createInScheduler();
static OutputSchedulerState *createOutScheduler();

/*  Determines switch configuration 					*/
/*  Configuration is filled into aSwitch->fabric.Xbar_matrix array 	*/

typedef struct {	
  struct Element *next;
  struct Element *prev;
  int *inputAcceptPointer;
  int *outputGrantPointer;
  int *connection;
} State;

void
ilqf(action, aSwitch, argc, argv)
  SwitchAction action;
Switch *aSwitch;
int argc;
char **argv;
{
  SchedulerState 			*scheduleState;
  InputSchedulerState 	*inputSchedule;
  OutputSchedulerState	*outputSchedule;
  int input, output;
  Cell *aCell;
  struct List *fifo;

  if(debug_algorithm)
    printf("Algorithm 'ilqf()' called by switch %d\n", aSwitch->switchNumber);
	
  switch(action) {
  case SCHEDULING_USAGE:
    fprintf(stderr, "Options for \"ilqf\" scheduling algorithm:\n");
    fprintf(stderr, "    -n number_of_iterations. Default: 1\n");
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
	scheduleState->numIterations=1;

	opterr=0;
	optind=1;
	while( (c = getopt(argc, argv, "n:")) != EOF)
	  switch (c)
	    {
	    case 'n':
	      scheduleState->numIterations = atoi(optarg);
	      break;
	    case '?':
	      fprintf(stderr, "-------------------------------\n");
	      fprintf(stderr, "ilqf: Unrecognized option -%c\n", optopt);
	      ilqf(SCHEDULING_USAGE);
	      fprintf(stderr, "--------------------------------\n");
	      exit(1);
	    default:
	      break;
	    }


	/* Reset and initialize scheduling stats for this switch. */
	scheduleCellStats( SCHEDULE_CELL_STATS_INIT, aSwitch );

	printf("numIterations %d\n", scheduleState->numIterations);
      }

    break;

  case SCHEDULING_EXEC:
    {
      int iteration;

      if(debug_algorithm) printf("	SCHEDULING_EXEC\n");

      scheduleState = (SchedulerState *) aSwitch->scheduler.schedulingState;

      /************* INITIALIZE ****************/
      /* Set up inputs (grants and accepts) */
      for(input=0; input<aSwitch->numInputs; input++)
	{
	  inputSchedule = scheduleState->inputSched[input];
	  inputSchedule->accept = NONE;
	}

      /* Set up outputs (accepts) */
      for(output=0; output<aSwitch->numOutputs; output++)
	{
	  outputSchedule = scheduleState->outputSched[output];
	  outputSchedule->accept=NONE;
	  outputSchedule->grant=NONE;
	}

      /* Check for newly arrived cells at head of input queues. */
      for(input=0; input<aSwitch->numInputs; input++)
	for(output=0; output<aSwitch->numOutputs; output++)
	  {
	    fifo = aSwitch->inputBuffer[input]->fifo[output];
	    if( fifo->number )
	      {
		aCell = fifo->head->Object;
		scheduleCellStats(SCHEDULE_CELL_STATS_HEAD_ARRIVAL,
				  aSwitch, aCell);
	      }
	  }
      /************* END INITIALIZE ****************/

      for(iteration=0; iteration<scheduleState->numIterations; iteration++)
	{
	  for(output=0; output<aSwitch->numOutputs; output++)
	    {

	      /* grant an input for this output */
	      if(debug_algorithm)
		printf("Selecting grant for output %d\n", output);
	      input = selectGrant(aSwitch, output);
	      if(debug_algorithm)
		printf("Selected input %d\n", input);
	      if( input != NONE ) 
		{
		  outputSchedule = scheduleState->outputSched[output];
		  outputSchedule->grant = input;
		}
	    }
	
	
	  for(input=0; input<aSwitch->numInputs; input++)
	    {
	      /* accept output for each input  */
	      output = selectAccept(aSwitch, input);
	      if( output >= 0 ) {
		inputSchedule = scheduleState->inputSched[input];
		outputSchedule = scheduleState->outputSched[output];
		inputSchedule->accept = output;
		outputSchedule->accept = input;
	      }
	    }
	}

      for(output=0; output<aSwitch->numOutputs; output++)
	{
	  input = scheduleState->outputSched[output]->accept;
	  aSwitch->fabric.Xbar_matrix[output].input = input;
	  if(input!=NONE)
	    aSwitch->fabric.Xbar_matrix[output].cell = (Cell *)
	      aSwitch->inputBuffer[input]->fifo[output]->head->Object;
	}

      break;
    }

  case SCHEDULING_REPORT_STATS:
    scheduleCellStats( SCHEDULE_CELL_STATS_PRINT_ALL, aSwitch );
    break;
  case SCHEDULING_INIT_STATS:
    scheduleCellStats( SCHEDULE_CELL_STATS_INIT, aSwitch );
    break;
  case SCHEDULING_REPORT_STATE:
    break;
  case SCHEDULING_CHECK_STATE_PERIOD:	
    break;

  default:
    break;
  }

	

  if(debug_algorithm)
    printf("Algorithm 'ilqf()' completed for switch %d\n", aSwitch->switchNumber);
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
  SchedulerState	*scheduleState=(SchedulerState *)aSwitch->scheduler.schedulingState;
  OutputSchedulerState *outputSchedule=scheduleState->outputSched[output];
  InputBuffer  *inputBuffer;
  Cell *aCell;

  int maxQueueOccupancy=0;
  static int *maxQueue=(int *) NULL;
  int numEqual=0;
  int selection;

  if( maxQueue == (int *) NULL)
    maxQueue = (int *) malloc( (1+aSwitch->numInputs) * sizeof(int));

	/* Check to see if output has already been accepted by an input from
       an earlier iteration */
  if( outputSchedule->accept != NONE )
    {
      if(debug_algorithm)
	printf("Output %d already accepted by input %d\n", output, outputSchedule->accept);
      return(NONE);
    }

  /*********************************/
  /* Pick input with longest queue */
  /*********************************/

  for(input=0; input<aSwitch->numInputs; input++)
    {
      inputBuffer = aSwitch->inputBuffer[input];
      if ( scheduleState->inputSched[input]->accept == NONE )
	{
	  if(inputBuffer->fifo[output]->number > maxQueueOccupancy) 
	    {
	      numEqual=0;
	      maxQueue[numEqual] = input;
	      maxQueueOccupancy = inputBuffer->fifo[output]->number;
	    }
	  else if( inputBuffer->fifo[output]->number == maxQueueOccupancy)
	    {
	      numEqual++;
	      maxQueue[numEqual] = input;
	    }
	}
    }

  if( maxQueueOccupancy == 0 )
    {
      if(debug_algorithm)
	printf("No inputs requested output %d. None granted.\n", output);
      return(NONE);
    }

  if( numEqual == 0 )
    input = maxQueue[0];
  else
    {
      selection  = (int)lrand48()%(numEqual+1); 
      input = maxQueue[ selection ];
    }
  inputBuffer = aSwitch->inputBuffer[input];

	/* STATS */
	/* Mark grantTime for cell at head of line */
  aCell = (Cell *) inputBuffer->fifo[output]->head->Object;
  scheduleCellStats( SCHEDULE_CELL_STATS_UPDATE_GRANT, 
		     aSwitch, aCell);
  /* END STATS */

  if(debug_algorithm)
    printf("Output %d granting to input %d\n", output, input);
  return(input);

}

static int
selectAccept(aSwitch, input)
  Switch *aSwitch;
int input;
{
  /*
    Choose output to accept from schedule
    */
  SchedulerState	*scheduleState=(SchedulerState *)aSwitch->scheduler.schedulingState;
  InputSchedulerState *inputSchedule=scheduleState->inputSched[input];
  OutputSchedulerState *outputSchedule;
  InputBuffer  *inputBuffer=aSwitch->inputBuffer[input];
  Cell *aCell;
  int output;

  static int *maxQueue=(int *) NULL;
  int maxQueueOccupancy=0;
  int numEqual=0;

  if( maxQueue == (int *) NULL)
    maxQueue = (int *) malloc( (1+aSwitch->numOutputs) * sizeof(int));

  if( inputSchedule->accept != NONE )
    return( NONE );

  for( output=0; output<aSwitch->numOutputs; output++ )
    {
      outputSchedule = scheduleState->outputSched[output];
      if( outputSchedule->grant == input )
	{
	  if(inputBuffer->fifo[output]->number > maxQueueOccupancy)
	    {
	      numEqual = 0;
	      maxQueue[numEqual] = output;
	      maxQueueOccupancy = inputBuffer->fifo[output]->number;
	    }
	  else if(inputBuffer->fifo[output]->number == maxQueueOccupancy)
	    {
	      numEqual++;
	      maxQueue[numEqual] = output;
	    }
	}
    }

  if( numEqual == 0 )
    output = maxQueue[0];
  else
    {
      output = maxQueue[ (int)lrand48()%(numEqual+1) ];
    }

  if( maxQueueOccupancy == 0 )
    {
      if(debug_algorithm)
	printf("Input %d not accepting any output\n", input);

      return(NONE);
    }

  /* STATS */
  /* Mark cell at head of line as accepted */
  aCell = (Cell *)
    aSwitch->inputBuffer[input]->fifo[output]->head->Object;
  scheduleCellStats( SCHEDULE_CELL_STATS_UPDATE_ACCEPT, 
		     aSwitch, aCell);

  if(debug_algorithm)
    printf("Input %d accepting output %d\n", input, output);
  return(output);	


}


/* Create schedule state variables for aSwitch */
static void 
createScheduleState(aSwitch)
  Switch *aSwitch;
{
  SchedulerState *scheduleState;


  int input, output;
	
  scheduleState = (SchedulerState *) malloc(sizeof(SchedulerState));
  aSwitch->scheduler.schedulingState = scheduleState;

	
  scheduleState->inputSched = (InputSchedulerState **) 
    malloc( aSwitch->numInputs * sizeof(InputSchedulerState *) );
  for(input=0; input<aSwitch->numInputs; input++)
    scheduleState->inputSched[input] = createInScheduler(aSwitch, input);

  scheduleState->outputSched = (OutputSchedulerState **) 
    malloc( aSwitch->numOutputs * sizeof(OutputSchedulerState *) );
  for(output=0; output<aSwitch->numOutputs; output++)
    scheduleState->outputSched[output] = createOutScheduler(aSwitch, output);
	
}

static InputSchedulerState *
createInScheduler(aSwitch, input)
  Switch *aSwitch;
int input;
{	
  SchedulerState 		*scheduleState=aSwitch->scheduler.schedulingState;
  InputSchedulerState	**inputSchedule=scheduleState->inputSched;


  inputSchedule[input] = (InputSchedulerState *) malloc( sizeof( InputSchedulerState ) );

  return(inputSchedule[input]);
	
}

static OutputSchedulerState *
createOutScheduler(aSwitch, output)
  Switch *aSwitch;
int output;
{
  SchedulerState 			*scheduleState=aSwitch->scheduler.schedulingState;
  OutputSchedulerState 	**outputSchedule=scheduleState->outputSched;


  outputSchedule[output] = (OutputSchedulerState *) malloc( sizeof( OutputSchedulerState ) );

  return(outputSchedule[output]);
}

#ifdef USE_ALL_FNS
/* this function is not used in here. There should be no need to use it,
   atleast not in this file */

static void
printState(aSwitch, aState)
  Switch *aSwitch;
State *aState;
{
  int input, output;


  printf("Switch %2d:", aSwitch->switchNumber);
  for(output=0; output<aSwitch->numOutputs; output++)
    printf("%3d ", output);
  printf("   Output: ");
  for(output=0; output<aSwitch->numOutputs; output++)
    printf("%2d ", output);
  printf("Input: ");
  for(input=0; input<aSwitch->numInputs; input++)
    printf("%2d ", input);
  printf("\n          ");
  /* Connection matrix */
  for(output=0; output<aSwitch->numOutputs; output++)
    {
      if( aState->connection[output] == NONE )
	printf("  X ");
      else
	printf("%3d ", aState->connection[output] );
    }
  printf("           ");
  /* Output schedulers */
  for(output=0; output<aSwitch->numOutputs; output++)
    printf("%2d ", aState->outputGrantPointer[output]);
  printf("       ");
  /* Input schedulers */
  for(input=0; input<aSwitch->numInputs; input++)
    printf("%2d ", aState->inputAcceptPointer[input]);
  printf("\n");
  printf("-----------------------------------------------------------\n");

}
#endif

