
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


#include <stdio.h>
#include <stdlib.h>
#include "sim.h"
#include "algorithm.h"

#define debug_wt_fanout 0

#ifdef USE_ALL_FUNCS
static int vectorSum();
static int vectorCommon();
#endif
static void vectorPrint();

typedef struct
{
  int grantPointer;
  int ageWeight;
  int fanoutWeight;
}WtFanoutState;

void 
mcast_wt_fanout(action, aSwitch, argc, argv)
  SwitchAction action;
Switch *aSwitch;
int argc;
char **argv;
{
  WtFanoutState *wtFanoutState;
  switch(action)
    {
    case SCHEDULING_USAGE:
      fprintf(stderr, "Options for mcast_wt_fanout:\n");
      fprintf(stderr, "-d weight_to_fanout (demand). Default: 1\n");
      fprintf(stderr, "-a weight_to_age_of_cell. Default: 1\n");
      break;
	
    case SCHEDULING_INIT:
      {
	int c;
	int ageWt, demandWt;
	extern int optind;
	extern int optopt;
	extern int opterr;
	extern char* optarg;

	ageWt = demandWt = 1;
	optind = 1;
	opterr = 0;
	while((c = getopt(argc, argv, "a:d:")) != EOF)
	  {
	    switch(c)
	      {
	      case 'a':
		sscanf(optarg, "%d", &ageWt);
		break;

	      case 'd':
		sscanf(optarg, "%d", &demandWt);
		break;

	      case '?':
		fprintf(stderr, "mcast_wt_fanout: Unrecognized option -%c\n", optopt);
		mcast_wt_fanout(SCHEDULING_USAGE);
		exit(1);

	      default:
		break;
	      }
	  }
	aSwitch->scheduler.schedulingState = malloc(sizeof(WtFanoutState));
	wtFanoutState = (WtFanoutState*)aSwitch->scheduler.schedulingState;
	wtFanoutState->grantPointer = 0;
	wtFanoutState->ageWeight = ageWt;
	wtFanoutState->fanoutWeight = demandWt;
	printf("Using fanout weight of: %d and age_weight of: %d\n", demandWt, ageWt);
	break;
      }

    case SCHEDULING_EXEC:
      {
	static int *weight = NULL;
	static int *grant = NULL;

	int input, output, numNewInputs;
	struct List *fifo;
	Cell *aCell;
	int numFabricOutputs;
	int ageWeight, fanoutWeight;
	int *grantPointer;
	int firstClashedInput;

	numFabricOutputs = aSwitch->numOutputs * aSwitch->fabric.Xbar_numOutputLines;
	if(!weight)
	  {
	    weight = (int*)malloc(sizeof(int) * aSwitch->numInputs);
	    grant = (int*)malloc(sizeof(int) * numFabricOutputs);
	  }

	wtFanoutState = (WtFanoutState*)
	  aSwitch->scheduler.schedulingState;
	grantPointer = &(wtFanoutState->grantPointer);
	ageWeight = wtFanoutState->ageWeight;
	fanoutWeight = wtFanoutState->fanoutWeight;


	/* Mark new cells at head of input queues */
	numNewInputs = 0;
	for(input = 0; input < aSwitch->numInputs; input++)
	  {
	    fifo = aSwitch->inputBuffer[input]->mcastFifo[DEFAULT_PRIORITY];
	    if(fifo->number)
	      {
		aCell = (Cell*)fifo->head->Object;
		if(aCell->commonStats.headArrivalTime == NONE)
		  {
		    aCell->commonStats.headArrivalTime = now;
		    numNewInputs++;
		  }
	      }
	  }

	if(debug_wt_fanout)
	  {
	    printf("Request matrix:\n");
	    for(input = 0; input < aSwitch->numInputs; input++)
	      {
		fifo =  aSwitch->inputBuffer[input]->mcastFifo[DEFAULT_PRIORITY];
		if(fifo->number)
		  {
		    aCell = (Cell *) fifo->head->Object;
		    for(output = 0; output < numFabricOutputs; output++)
		      {
			if(bitmapIsBitSet(output, &aCell->outputs))
			  printf("1");
			else
			  printf("0");
		      }
		  }
		else
		  {
		    for(output = 0; output < numFabricOutputs; output++)
		      {
			printf("0");
		      }
		  }
		printf("\n");
	      }
	  }




	/* Compute the weight of each input */
	for(input = 0; input < aSwitch->numInputs; input++)
	  {
	    fifo =  aSwitch->inputBuffer[input]->mcastFifo[DEFAULT_PRIORITY];
	    if(fifo->number)
	      {
		int myFanout, myAge;
		aCell = (Cell *) fifo->head->Object;
		myAge = now - aCell->commonStats.headArrivalTime;
		myFanout = bitmapNumSet(&aCell->outputs);
		weight[input] = ageWeight * myAge + 
		  fanoutWeight * (numFabricOutputs - myFanout);
	      }
	    else
	      weight[input] = NONE;
	  }
	if(debug_wt_fanout)
	  {
	    printf("Wt of each input:\n");
	    vectorPrint(stdout, weight, aSwitch->numInputs);
	  }

	firstClashedInput = NONE;
	/* Find the input with maximum weight of each output */
	for(output = 0; output < numFabricOutputs; output++)
	  {
	    int maxWeight;
	    Bitmap maxWeightInputs;

	    maxWeight = -1;
	    bitmapReset(&maxWeightInputs);
	    for(input = 0; input < aSwitch->numInputs; input++)
	      {
		fifo =  aSwitch->inputBuffer[input]->mcastFifo[DEFAULT_PRIORITY];
		if(fifo->number)
		  {
		    aCell = (Cell *) fifo->head->Object;
		    if(bitmapIsBitSet(output, &aCell->outputs))
		      {
			if(weight[input] == maxWeight)
			  {
			    bitmapSetBit(input, &maxWeightInputs);
			  }
			else if(weight[input] > maxWeight)
			  {
			    bitmapReset(&maxWeightInputs);
			    bitmapSetBit(input, &maxWeightInputs);
			    maxWeight = weight[input];
			  }
		      }
		  }
	      }
	    grant[output] = NONE;
	    if(maxWeight >= 0)
	      {
		input = *grantPointer;
		for(;;)
		  {
		    if(bitmapIsBitSet(input, &maxWeightInputs))
		      {
			grant[output] = input;
			if(bitmapNumSet(&maxWeightInputs) > 1)
			  {
			    if(firstClashedInput == NONE || input < firstClashedInput)

			      firstClashedInput = input;
			  }
			break;
		      }
		    else
		      input = (input + 1) % aSwitch->numInputs;
		  }
	      }
	  }
	if(debug_wt_fanout)
	  {
	    printf("Grants for each output:\n");
	    vectorPrint(stdout, grant, numFabricOutputs);
	  }

	/* Setup the crossbar matrix */
	for(output = 0; output < numFabricOutputs; output++)
	  {
	    input = grant[output];
	    aSwitch->fabric.Xbar_matrix[output].input = input;
	    if(input != NONE)
	      aSwitch->fabric.Xbar_matrix[output].cell = 
		(Cell *) aSwitch->inputBuffer[input]->mcastFifo[DEFAULT_PRIORITY]->head->Object;
	  }
	if(firstClashedInput != NONE)
	  *grantPointer = (firstClashedInput + 1) % aSwitch->numInputs;
	break;
      }
    default:
      break;
    }
}

    
/***********************************************************************/

#ifdef USE_ALL_FUNCS
static int
vectorSum(vector, size)
  int *vector;
int size;
{
  int i,sum=0;
  for(i=0;i<size;i++)
    {
      sum += vector[i];
    }
  return(sum);
}

static int
vectorCommon(vector1, vector2, size)
  int *vector1, *vector2;
int size;
{	
  int i, sum=0;
  for(i=0;i<size;i++)
    {
      if(vector1[i]>0 && vector2[i]>0)
	sum ++;
    }
  return(sum);
}
#endif

static void
vectorPrint(fp, vector, size)
  FILE *fp;
int *vector;
int size;
{
  int i;
  for(i=0;i<size;i++)
    {
      fprintf(fp, "%3d", vector[i]);
    }
  fprintf(fp,"\n");
}










		
