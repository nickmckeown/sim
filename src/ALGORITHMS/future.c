
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
#include "future.h"

static InputSchedulerState *createInScheduler();
static OutputSchedulerState *createOutScheduler();
static void createScheduleState();
static long nextScheduledTime();
static struct TimeEntry *createTimeEntry();

/*  Determines switch configuration 					*/
/*  Configuration is filled into aSwitch->fabric.interconnect.matrix array 	*/

/*  

	Input schedulers:

	foreach input
		foreach output
			if new cell arrived, add a copy to toBeScheduledList[output]
			if(!empty) remove element from toBeScheduledList[output]
				Get scheduled time from output scheduler and
					return recycled slot from previous cycle
				Find location of scheduled time in timeList. 
				If location used, set as recyclable.
				else place in timeList.
		end
	end

	Output schedulers:
 
	If a time slot free, return it.
	Else, return next time free.
	If given recycled slot, add to recycleList.

*/


void
future(action, aSwitch, argc, argv)
  SwitchAction action;
Switch *aSwitch;
int argc;
char **argv;
{
  SchedulerState 			*scheduleState;
  InputSchedulerState 	*inputSchedule;
  int input, output;

  if(debug_algorithm)
    printf("Algorithm 'future()' called by switch %d\n", aSwitch->switchNumber);
	
  switch(action) {
  case SCHEDULING_USAGE:
    fprintf(stderr, "Options for \"future\" scheduling algorithm:\n");
    fprintf(stderr, "    -N num_iterations.   Default: 1\n");
    fprintf(stderr, "    -M recycle_list_max. Default: 1\n");
    break;
  case SCHEDULING_INIT:	
    if(debug_algorithm)
      printf("	INIT\n");
			
    /* Allocate scheduler state for this switch */
    if( aSwitch->scheduler.schedulingState == NULL )
      {
	extern int opterr;
	extern int optopt;
	extern int optind;
	extern char *optarg;
	int c;

	OutputSchedulerState *outputSchedule;
	int recycleListMax=1;

	createScheduleState(aSwitch);
	scheduleState = ( SchedulerState * ) aSwitch->scheduler.schedulingState;
	scheduleState->numIterations=1;

	opterr=0;
	optind=1;
	while( (c = getopt(argc, argv, "M:N:")) != EOF )
	  switch (c)
	    {
	    case 'M':
	      recycleListMax = atol(optarg);
	      break;
	    case 'N':
	      scheduleState->numIterations = atol(optarg);
	      break;
	    case '?':
	      fprintf(stderr, "-------------------------------\n");
	      fprintf(stderr, "future: Unrecognized option -%c\n", optopt);
	      future(SCHEDULING_USAGE);
	      fprintf(stderr, "--------------------------------\n");
	      exit(1);
	      break;
	    default:
	      break;
	    }

	printf("numIterations  %d\n", scheduleState->numIterations);
	printf("recycleListMax %d\n", recycleListMax);


	/* Set the max length of recycleList */
	scheduleState=aSwitch->scheduler.schedulingState;
	for(output=0; output<aSwitch->numOutputs; output++)
	  {
	    outputSchedule=scheduleState->outputSched[output];
	    outputSchedule->recycleListMax = recycleListMax;
	  }
      }

    break;

  case SCHEDULING_EXEC:
    {
      int iteration;
      InputBuffer  *inputBuffer;
      Cell *aCell;
      struct TimeEntry *aTimeEntry, *tmpTimeEntry;
      struct Element *anElement, *tmpElement;

      scheduleState = (SchedulerState *) aSwitch->scheduler.schedulingState;

      if(debug_algorithm)
	{
	  printf("	CONFIG\n");
	  printf("    Num iterations: %d\n", scheduleState->numIterations);
	}


      for(input=0; input<aSwitch->numInputs; input++)
	{
	  inputSchedule = scheduleState->inputSched[input];
	  inputBuffer = aSwitch->inputBuffer[input];

	  /* Find out if a new cell has arrived */
	  for(output=0; output<aSwitch->numOutputs; output++)
	    {
	      if(inputBuffer->fifo[output]->number)
		{
		  aCell = (Cell *)inputBuffer->fifo[output]->tail->Object;
		  if( aCell->commonStats.arrivalTime == now )
		    {
		      if(debug_algorithm)
			printf("Input %d: New cell arrived for output %d\n", input, output);
		      aTimeEntry = createTimeEntry();
		      aTimeEntry->output = output;
		      anElement = createElement(aTimeEntry);
		      addElement(inputSchedule->toBeScheduledList[output], anElement);
		      break;
		    }
		}
	    }
	}

      for(iteration=0; iteration<scheduleState->numIterations; iteration++)
	{
	  for(input=0; input<aSwitch->numInputs; input++)
	    {
	      inputSchedule = scheduleState->inputSched[input];
	      inputBuffer = aSwitch->inputBuffer[input];

	      for(output=0; output<aSwitch->numOutputs; output++)
		{
		  if( inputSchedule->toBeScheduledList[output]->number )
		    {
		      anElement = removeElement(inputSchedule->toBeScheduledList[output]);
		      aTimeEntry = (struct TimeEntry *) anElement->Object;
		      if(debug_algorithm)
			{
			  printf("Input %d: Removed timeEntry\n", input);
			  printf("Scheduling: recycling %d\n", 
				 inputSchedule->recyclableTimeValue[output]);
			}
		      aTimeEntry->timeValue = nextScheduledTime(aSwitch, input, aTimeEntry->output, inputSchedule->recyclableTimeValue[output]);
		      inputSchedule->recyclableTimeValue[output] = NONE;

		      if(debug_algorithm)
			printf("To be scheduled for time %d\n", aTimeEntry->timeValue);

		      /* Find location in of scheduled timeEntry in timeList */
						
		      tmpElement = inputSchedule->timeList->head;
		      while(tmpElement)
			{		
			  tmpTimeEntry=(struct TimeEntry *)tmpElement->Object;
			  /* List is FIFO. Head is time least in future */
			  /* If scheduled time is newer than entry, add it
			     before tmpTimeEntry */
			  if(aTimeEntry->timeValue < tmpTimeEntry->timeValue)
			    {
			      addElementBefore(inputSchedule->timeList, 
					       tmpElement, anElement );
			      break;
			    }

			  /* Clash!!! */
			  /* Put it back on the toBeScheduledList */
			  if(aTimeEntry->timeValue==tmpTimeEntry->timeValue)
			    {
			      inputSchedule->recyclableTimeValue[output] 
				= aTimeEntry->timeValue;
			      addElementAtHead( 
					       inputSchedule->toBeScheduledList[output], 
					       (struct Element *) anElement);
			      break;
			    }
			  if( tmpElement == inputSchedule->timeList->tail)
			    {
			      addElement(inputSchedule->timeList,anElement);
			      break;
			    }
			  tmpElement= tmpElement->next;
			}
		      /* If list is/was empty add this entry to it */
		      if(!inputSchedule->timeList->number)
			{
			  addElement( inputSchedule->timeList, anElement );
			}
							
		    }
		}
	    }

	}

      /* Configure Switch */
      for(input=0; input<aSwitch->numInputs; input++)
	{
	  /* Check to see if head of input timeList is for now. */
	  inputSchedule = scheduleState->inputSched[input];
	  if( inputSchedule->timeList->number )
	    {
	      anElement = inputSchedule->timeList->head;
	      aTimeEntry = ( struct TimeEntry *) anElement->Object;
	      if( aTimeEntry->timeValue == now )
		{
		  removeElement( inputSchedule->timeList );
		  aSwitch->fabric.Interconnect.Crossbar.Matrix[aTimeEntry->output].input = input;
		  aSwitch->fabric.Interconnect.Crossbar.Matrix[aTimeEntry->output].cell 
		  = aSwitch->inputBuffer[input]->fifo[aTimeEntry->output]->head->Object;
		  destroyElement( anElement );
		  free(aTimeEntry);
		}
	    }
	}

      break;
    }

  default:
    break;
  }

	

  if(debug_algorithm)
    printf("Algorithm 'future()' completed for switch %d\n", aSwitch->switchNumber);
}

static long
nextScheduledTime(aSwitch, input, output, recyclableTimeValue)
  Switch *aSwitch;
int input;
int output;
int recyclableTimeValue;
{
  SchedulerState          *scheduleState=aSwitch->scheduler.schedulingState;
  OutputSchedulerState    *outputSchedule=scheduleState->outputSched[output];
  long returnValue=NONE;
  struct TimeEntry *aTimeEntry, *recyclableTimeEntry;
  struct Element *anElement, *nextElement;


  /* Update nextFreeTime in case t hasn't happened yet */
  if( outputSchedule->nextFreeTime < now )
    outputSchedule->nextFreeTime = now;

  /* Remove any obsolete, unusable timeEntries from recyclable list */
  anElement = outputSchedule->recycleList->head;
  while( anElement )
    {
      aTimeEntry = (struct TimeEntry *) anElement->Object;
      if( aTimeEntry->timeValue < now )
	{
	  /* Remove and destroy */
	  nextElement = (struct Element *) anElement->next;
	  deleteElement( outputSchedule->recycleList, 
			 (struct Element *) anElement );
	  destroyElement(anElement);	
	  if(debug_algorithm)
	    printf("Output %d: Deleted obsolete time entry for time %d at time %ld\n", output, aTimeEntry->timeValue, now);
	  free(aTimeEntry);

	  anElement = nextElement;
	}
      else
	anElement = anElement->next;
    }

  /* If can use a recycled time entry then do so. Else get the next time */
  anElement = outputSchedule->recycleList->head;
  while( anElement )
    {
      aTimeEntry = (struct TimeEntry *) anElement->Object;
      nextElement = (struct Element *) anElement->next;
      /* Check that the entry did not come from this input */
      if( aTimeEntry->input != input )
	{
	  deleteElement( outputSchedule->recycleList, anElement );
	  if(debug_algorithm)
	    printf("Returning recycled entry.\n"); 
	  returnValue = aTimeEntry->timeValue;
	  destroyElement(anElement);
	  free(aTimeEntry);
	  break;
	}
      anElement = nextElement;
    }
  if( returnValue == NONE )
    returnValue = outputSchedule->nextFreeTime++;

  if( recyclableTimeValue != NONE )
    {
      /* Add to list if list is not full. Else drop the timeValue */
      if( outputSchedule->recycleList->number < outputSchedule->recycleListMax )
	{
	  if(debug_algorithm)
	    printf("Adding returned recycled entry to list.\n"); 
	  recyclableTimeEntry = createTimeEntry();
	  recyclableTimeEntry->timeValue = recyclableTimeValue;
	  recyclableTimeEntry->output = output;
	  recyclableTimeEntry->input = input;
	  anElement = createElement(recyclableTimeEntry);
	  addElement(outputSchedule->recycleList, anElement);
	}
      else
	outputSchedule->droppedRecyclableEntries++;
    }

  return(returnValue);
}



/***********************************************************************/

static
struct TimeEntry *
createTimeEntry()
{
  struct TimeEntry *aTimeEntry;

  aTimeEntry = (struct TimeEntry *) malloc( sizeof(struct TimeEntry) );

  return(aTimeEntry);
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
  int output;	
  SchedulerState 		*scheduleState=aSwitch->scheduler.schedulingState;
  InputSchedulerState	**inputSchedule=scheduleState->inputSched;

  inputSchedule[input] = (InputSchedulerState *) malloc( sizeof( InputSchedulerState ) );

  inputSchedule[input]->timeList = createList("timeEntryList");
	
  inputSchedule[input]->toBeScheduledList = (struct List **)
    malloc( sizeof(struct List *) * aSwitch->numOutputs );
  inputSchedule[input]->recyclableTimeValue = (int *)
    malloc( sizeof(int) * aSwitch->numOutputs );
  for(output=0; output<aSwitch->numOutputs; output++)
    {
      inputSchedule[input]->toBeScheduledList[output] = createList("2beScheduled");
      inputSchedule[input]->recyclableTimeValue[output] = NONE;
    }	

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

  outputSchedule[output]->nextFreeTime = 0;
  outputSchedule[output]->droppedRecyclableEntries = 0;
  outputSchedule[output]->recycleList = createList("RecycleList");

  return(outputSchedule[output]);
}
