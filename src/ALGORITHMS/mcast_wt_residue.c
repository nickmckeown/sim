
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
#include <string.h>
#include "sim.h"
#include "algorithm.h"

#define debug_wt_residue 0

#ifdef USE_ALL_FUNCS
static int vectorSum();
#endif
static int vectorCommon();
static void vectorPrint();

typedef struct
{
  int grantPointer;
  int ageWeight;
  int residueWeight;
}WtResidueState;

void 
mcast_wt_residue(action, aSwitch, argc, argv)
  SwitchAction action;
Switch *aSwitch;
int argc;
char **argv;
{
  WtResidueState *wtResidueState;
  switch(action)
    {
    case SCHEDULING_USAGE:
      fprintf(stderr, "Options for mcast_wt_residue:\n");
      fprintf(stderr, "-d weight_to_residue (demand). Default: 1\n");
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
		fprintf(stderr, "mcast_wt_residue: Unrecognized option -%c\n", optopt);
		mcast_wt_residue(SCHEDULING_USAGE);
		exit(1);

	      default:
		break;
	      }
	  }
	aSwitch->scheduler.schedulingState = malloc(sizeof(WtResidueState));
	wtResidueState = (WtResidueState*)aSwitch->scheduler.schedulingState;
	wtResidueState->grantPointer = 0;
	wtResidueState->ageWeight = ageWt;
	wtResidueState->residueWeight = demandWt;
	printf("Using residue weight of: %d and age_weight of: %d\n", demandWt, ageWt);
	break;
      }

    case SCHEDULING_EXEC:
      {
	static int *residue=NULL;
	static int **request = NULL;
	static int *weight = NULL;
	static int *grant = NULL;

	int input, output, numNewInputs;
	struct List *fifo;
	Cell *aCell;
	int numFabricOutputs;
	int ageWeight, residueWeight;
	int *grantPointer;
	int firstClashedInput;

	numFabricOutputs = aSwitch->numOutputs * aSwitch->fabric.Xbar_numOutputLines;
	if(!residue)
	  {
	    residue = (int*)malloc(sizeof(int) * numFabricOutputs);
	    request = (int**) malloc(sizeof(int*) * aSwitch->numInputs);
	    for(input = 0; input < aSwitch->numInputs; input++)
	      {
		request[input] = (int*)malloc(sizeof(int) * numFabricOutputs);
	      }
	    weight = (int*)malloc(sizeof(int) * aSwitch->numInputs);
	    grant = (int*)malloc(sizeof(int) * numFabricOutputs);
	  }

	wtResidueState = (WtResidueState*)
	  aSwitch->scheduler.schedulingState;
	grantPointer = &(wtResidueState->grantPointer);
	ageWeight = wtResidueState->ageWeight;
	residueWeight = wtResidueState->residueWeight;

	/* INITIALIZE */
	for(input=0;input < aSwitch->numInputs; input++)
	  {
	    memset(request[input], 0, sizeof(int) * numFabricOutputs);
	  }
	memset(residue, 0, sizeof(int) * numFabricOutputs);


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

	/* Make the Request matrix and Residue vector */
	if(debug_wt_residue)
	  {
	    printf("-------------\n");
	    printf("Request:\n");
	  }

	for(input=0; input<aSwitch->numInputs; input++)
	  {
	    fifo = aSwitch->inputBuffer[input]->mcastFifo[DEFAULT_PRIORITY];
	    if(fifo->number)
	      {
		aCell = (Cell *) fifo->head->Object;
		for(output=0;output<numFabricOutputs;output++)
		  {
		    /* The range of outputs set by the TRAFFIC algo is different from the
		       range being checked here. ritesh */
			
		    if( bitmapIsBitSet(output, &aCell->outputs) )
		      {
			residue[output]++;
			request[input][output]=1;
		      }
		  }
	      }

	    if(debug_wt_residue)
	      vectorPrint(stdout, request[input], numFabricOutputs);
	  }
	for(output=0;output<numFabricOutputs;output++)
	  if( residue[output]>0 ) residue[output]--;

	if(debug_wt_residue)
	  {
	    printf("Residue:\n");
	    vectorPrint(stdout, residue, numFabricOutputs);
	    printf("-------------\n");
	  }

	/* Compute the weight of each input */
	for(input = 0; input < aSwitch->numInputs; input++)
	  {
	    fifo =  aSwitch->inputBuffer[input]->mcastFifo[DEFAULT_PRIORITY];
	    if(fifo->number)
	      {
		int myResidue, myAge;
		aCell = (Cell *) fifo->head->Object;
		myAge = now - aCell->commonStats.headArrivalTime;
		myResidue = vectorCommon(request[input], residue, 
					 numFabricOutputs);
		weight[input] = ageWeight * myAge + 
		  residueWeight * (numFabricOutputs - myResidue);
	      }
	    else
	      weight[input] = NONE;

	  }
	if(debug_wt_residue)
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
		if(request[input][output])
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
	if(debug_wt_residue)
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
#endif
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










		
