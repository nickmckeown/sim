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
#include "pim.h"
#include "scheduleStats.h"

static int selectGrant();
static int selectAccept();
static void createScheduleState();
static InputSchedulerState *createInScheduler();
static OutputSchedulerState *createOutScheduler();

/*  Determines switch configuration 					*/
/*  Configuration is filled into aSwitch->fabric.interconnect.matrix array 	*/

void
pim(action, aSwitch, argc, argv)
  SwitchAction action;
Switch *aSwitch;
int argc;
char **argv;
{
  SchedulerState 			*scheduleState;
  InputSchedulerState 	*inputSchedule;
  OutputSchedulerState	*outputSchedule;
  int input, output;

  int numIterations=1; /* Default value */

  if(debug_algorithm)
    printf("Algorithm 'pim()' called by switch %d\n", aSwitch->switchNumber);
	
  switch(action) {
  case SCHEDULING_USAGE:
    fprintf(stderr, "Options for \"pim\" scheduling algorithm:\n");
    fprintf(stderr, "    -n number_of_iterations. Default: 1\n");
    break;
  case SCHEDULING_INIT:	
    if(debug_algorithm)
      printf("	SCHEDULING_INIT\n");

    /* Parse options */
    if( aSwitch->scheduler.schedulingState == NULL )
      {	
	extern int opterr;
	extern int optopt;
	extern int optind;
	extern char *optarg;
	int c;


	opterr=0;
	optind=1;
	while( (c = getopt(argc, argv, "n:")) != EOF )
	  switch (c)
	    {
	    case 'n':
	      numIterations = atoi(optarg);
	      break;
	    case '?':
	      fprintf(stderr, "-------------------------------\n");
	      fprintf(stderr, "pim: Unrecognized option -%c\n", optopt);
	      pim(SCHEDULING_USAGE);
	      fprintf(stderr, "--------------------------------\n");
	      exit(1);
	    default:	
	      break;
	    } 
	printf("Numiterations: %d\n", numIterations);
	/* Allocate scheduler state for this switch */
	createScheduleState(aSwitch);
	scheduleState = (SchedulerState *) aSwitch->scheduler.schedulingState;
	scheduleState->numIterations = numIterations;
      }
			


    break;

  case SCHEDULING_EXEC:
    {
      int iteration;
      int numConnectionsThisIteration;
      int numFabricOutputs;

      if(debug_algorithm)
	{
	  printf("	SCHEDULING_EXEC\n");
	  printf("    Num iterations: %d\n", numIterations);
	}

      scheduleState = (SchedulerState *) aSwitch->scheduler.schedulingState;

      /************** INITIALIZE ***************/
      /* Set up inputs (grants and accepts) */
      numFabricOutputs = aSwitch->numOutputs * 
	aSwitch->fabric.Xbar_numOutputLines;
      for(input=0; input<aSwitch->numInputs; input++)
	{
	  inputSchedule = scheduleState->inputSched[input];
	  inputSchedule->accept = NONE;
		
	  for(output=0; output<numFabricOutputs; output++)
	    {
	      inputSchedule->grant[output] = 0;
	    }
	}

      /* Set up outputs (accepts) */
      for(output=0; output<numFabricOutputs; output++)
	{
	  outputSchedule = scheduleState->outputSched[output];
	  outputSchedule->accept=NONE;
	  outputSchedule->grant=NONE;
	}
      /************** END INITIALIZE ***************/

      for(iteration=0; iteration<scheduleState->numIterations; iteration++)
	{
	  numConnectionsThisIteration=0;
	  for(output=0; output<numFabricOutputs; output++)
	    {

	      /* grant an input for this output */
	      if(debug_algorithm)
		printf("Selecting grant for output %d\n", output);
	      input = selectGrant(aSwitch, output);
	      if(debug_algorithm)
		printf("Selected input %d\n", input);
	      if( input != NONE ) 
		{
		  inputSchedule = scheduleState->inputSched[input];
		  outputSchedule = scheduleState->outputSched[output];
		  inputSchedule->grant[output] = 1;
		  outputSchedule->grant = input;
		}
	    }
	
	
	  for(input=0; input<aSwitch->numInputs; input++)
	    {
	      /* accept output for each input  */
	      output = selectAccept(aSwitch, input);
	      if( output >= 0 ) {
		numConnectionsThisIteration++;
		inputSchedule = scheduleState->inputSched[input];
		outputSchedule = scheduleState->outputSched[output];
		inputSchedule->accept = output;
		outputSchedule->accept = input;
						
	      }
	    }
	  if(!numConnectionsThisIteration)
	    break;
	}
      scheduleStats(SCHEDULE_STATS_NUM_ITERATIONS, aSwitch, &iteration);

      for(output=0; output<numFabricOutputs; output++)
	{
	  input = scheduleState->outputSched[output]->accept;
	  aSwitch->fabric.Xbar_matrix[output].input = input;
	  if(input != NONE)
	    {
	      aSwitch->fabric.Xbar_matrix[output].cell = (Cell *)
		aSwitch->inputBuffer[input]->fifo[output]->head->Object;
	    }
	}

      break;
    }
  case SCHEDULING_REPORT_STATS:
    scheduleStats( SCHEDULE_STATS_PRINT_NUM_ITERATIONS, aSwitch );
    break;
  case SCHEDULING_INIT_STATS:
    scheduleStats( SCHEDULE_STATS_INIT, aSwitch );			
    break;
  default:
    break;
  }

	

  if(debug_algorithm)
    printf("Algorithm 'pim()' completed for switch %d\n", aSwitch->switchNumber);
}

/***********************************************************************/

static int
selectGrant(aSwitch, output)
  Switch *aSwitch;
int output;
{
  /*
    Randomly select between all requesting input ports
    */
  int input, switchOutput, index;
  int *requestors, num_rqsts=0;
  int selected;
  SchedulerState	*scheduleState=(SchedulerState *)aSwitch->scheduler.schedulingState;
  OutputSchedulerState *outputSchedule=scheduleState->outputSched[output];
  InputBuffer  *inputBuffer;

  switchOutput = output / aSwitch->fabric.Xbar_numOutputLines;
	
  /* Allocate temporary array to store inputs that request this output */
  requestors = (int *) malloc( aSwitch->numInputs * sizeof(int) );

  /* Check to see if output has already been accepted by an input from
       an earlier iteration */
  if( outputSchedule->accept >= 0 )
    {
      if(debug_algorithm)
	printf("Output %d already accepted by input %d\n", output, outputSchedule->accept);
      free(requestors);
      return(NONE);
    }

  /* Build array of requesting input ports for this output. 		*/
  /* i.e which input ports have a cell destined for this output 	*/
  /* and that input has not already accepted any other output 	*/
  for(input=0; input<aSwitch->numInputs; input++)
    {
      inputBuffer = aSwitch->inputBuffer[input];

      if( (inputBuffer->fifo[switchOutput]->number > 0) && 
	  ( scheduleState->inputSched[input]->accept ==NONE) )
	requestors[num_rqsts++] = input;
    }

  /* Choose randomly between inputs */
  if(num_rqsts == 0) 
    {
      if(debug_algorithm)
	printf("Output %d received no requests\n", output);
      free(requestors);
      return(NONE);
    }
  else if(debug_algorithm)
    printf("Output %d received %d requests\n", output, num_rqsts);

  index = nrand48(outputSchedule->rseed)%num_rqsts; /* Uniform [0,num_rqsts-1] */
  selected = requestors[index];
  if(debug_algorithm)
    printf("Output %d granting to input %d\n", output, selected);

  free(requestors);

  return(selected);
}

static int
selectAccept(aSwitch, input)
  Switch *aSwitch;
int input;
{
  /*
    Choose output to accept in an IID uniform fashion
    */
  SchedulerState	*scheduleState=(SchedulerState *)aSwitch->scheduler.schedulingState;
  InputSchedulerState *inputSchedule=scheduleState->inputSched[input];

  int index, output, num_rqsts=0;
  int *grantors;
  int granted;
  int numFabricOutputs;

  numFabricOutputs = aSwitch->numOutputs*aSwitch->fabric.Xbar_numOutputLines;
  grantors = (int *) malloc( numFabricOutputs * sizeof(int) );

	/* Build array of output ports that have granted to this input	*/
	/* And check to see if input has already accepted an output. 	*/
  for(output=0; output<numFabricOutputs; output++)
    {
      if( debug_algorithm )
	{
	  printf("		Grant from out %d to in %d is: %d\n", output, input,
		 inputSchedule->grant[output] );
	  printf("		Input %d has already accepted output %d\n", input,
		 inputSchedule->accept );
	}
      if( (inputSchedule->grant[output] != 0 ) 
	  && (inputSchedule->accept == NONE) )
	grantors[num_rqsts++] = output;
    }

  /* Choose randomly between granting outputs */
  if(num_rqsts == 0) 
    {
      if(debug_algorithm)
	printf("Input %d received no grants\n", input);
      free(grantors);
      return(NONE);
    }
  else if(debug_algorithm)
    printf("Input %d received %d grants\n", input, num_rqsts);

  index = nrand48(inputSchedule->rseed)%num_rqsts; /* Uniform [0,num_rqsts-1] */
  granted = grantors[index];
  if(debug_algorithm)
    printf("Input %d accepting output %d\n", input, granted);

  free(grantors);

  return(granted);
}


/* Create schedule state variables for aSwitch */
static void
createScheduleState(aSwitch)
  Switch *aSwitch;
{
  SchedulerState *scheduleState;


  int input, output, numFabricOutputs;

  numFabricOutputs=aSwitch->numOutputs*aSwitch->fabric.Xbar_numOutputLines;
	
  scheduleState = (SchedulerState *) malloc(sizeof(SchedulerState));
  aSwitch->scheduler.schedulingState = scheduleState;

	
  scheduleState->inputSched = (InputSchedulerState **) 
    malloc( aSwitch->numInputs * sizeof(InputSchedulerState *) );
  for(input=0; input<aSwitch->numInputs; input++)
    scheduleState->inputSched[input] = createInScheduler(aSwitch, input);

  scheduleState->outputSched = (OutputSchedulerState **) 
    malloc(numFabricOutputs * sizeof(OutputSchedulerState *));
  for(output=0; output<numFabricOutputs; output++)
    scheduleState->outputSched[output] = createOutScheduler(aSwitch, output);
	
}

static InputSchedulerState *
createInScheduler(aSwitch, input)
  Switch *aSwitch;
int input;
{	
  SchedulerState 		*scheduleState=aSwitch->scheduler.schedulingState;
  InputSchedulerState	**inputSchedule=scheduleState->inputSched;
  int numFabricOutputs;

  numFabricOutputs=aSwitch->numOutputs*aSwitch->fabric.Xbar_numOutputLines;

  inputSchedule[input] = (InputSchedulerState *) malloc( sizeof( InputSchedulerState ) );
  inputSchedule[input]->grant = (int *) malloc( sizeof(int)*numFabricOutputs);

  inputSchedule[input]->rseed[0] = 0x1239 ^ input;
  inputSchedule[input]->rseed[1] = 0xbe22 ^ input;
  inputSchedule[input]->rseed[2] = 0x90aa ^ input;

  return(inputSchedule[input]);
	
}

static OutputSchedulerState *
createOutScheduler(aSwitch, output)
  Switch *aSwitch;
int output;
{
  SchedulerState 			*scheduleState=aSwitch->scheduler.schedulingState;
  OutputSchedulerState 	**outputSchedule=scheduleState->outputSched;

  outputSchedule[output] = (OutputSchedulerState *) 
    malloc( sizeof( OutputSchedulerState ) );
  outputSchedule[output]->request = (int *) malloc( sizeof(int)*aSwitch->numInputs);
  outputSchedule[output]->rseed[0] = 0x1438 ^ output;
  outputSchedule[output]->rseed[1] = 0xbc21 ^ output;
  outputSchedule[output]->rseed[2] = 0x93ac ^ output;

  return(outputSchedule[output]);
}
