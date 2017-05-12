
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

#define debug_tatra 0
#define anim 1

extern void FatalError(char*);
#define ANIM_START resetStatsTime
#define ANIM_FILE "anim.txt"

static int demand_compare();
static void addToTatraMatrix();
void printTatraMatrix();
void animPrintTatraMatrix();
#ifdef USE_ALL_FUNCS
static int vectorSum();
static int vectorCommon();
static void vectorPrint();
#endif


typedef struct tatra_state
{
  int** tatraMatrix;
  int numOutputs, numRows;
  int* peakRow;
  int headRow;
}TatraState;

/*
typedef struct tatra_stats
{
    Stat headOfQueueStats;
};
*/

typedef struct 
{
  int input;
  int demand;
} Demand;


void 
mcast_tatra(action, aSwitch, argc, argv)
  SwitchAction action;
Switch *aSwitch;
int argc;
char** argv;
{
  int **tatraMatrix;
  int headRow;
  TatraState *tatraState;
  int i, j;
  static FILE *animFp;

  switch(action)
    {
    case SCHEDULING_USAGE:
      fprintf(stderr, "No options for \"mcast_tatra\".\n");
      break;
	
    case SCHEDULING_INIT:
      {
	int numFabricOutputs;
	numFabricOutputs = aSwitch->numOutputs * aSwitch->fabric.Xbar_numOutputLines;
	aSwitch->scheduler.schedulingState = malloc(sizeof(TatraState));
	tatraState = (TatraState*)aSwitch->scheduler.schedulingState;
	    
	tatraMatrix = (int**)malloc(numFabricOutputs * sizeof(int*));
	for(i = 0; i < numFabricOutputs; i++)
	  {
	    tatraMatrix[i] = (int*)malloc(aSwitch->numInputs * sizeof(int));
	    tatraState->peakRow = (int*)malloc(aSwitch->numInputs * sizeof(int));
	    for(j = 0; j < aSwitch->numInputs; j++) {
	      tatraMatrix[i][j] = NONE;
	      tatraState->peakRow[j] = NONE;
	    }
	  }
	tatraState->tatraMatrix = tatraMatrix;
	tatraState->numOutputs = numFabricOutputs;
	tatraState->numRows = aSwitch->numInputs;
	tatraState->headRow = 0;
	if(anim)
	  {
	    animFp = fopen(ANIM_FILE, "wt");
	    if(!animFp)
	      {
		FatalError("TATRA: Unable to open ANIM_FILE\n");
	      }
	    fprintf(animFp, "M %d\nN %d\n", 
		    aSwitch->numInputs, aSwitch->numOutputs);
	  }          
      
	break;
      }

    case SCHEDULING_EXEC:
      {
	static Demand *demand = NULL;
	int input, output, numNewInputs;
	struct List *fifo;
	Cell *aCell;
	int numFabricOutputs;

	int numInputs;

	numFabricOutputs = aSwitch->numOutputs * aSwitch->fabric.Xbar_numOutputLines;
	if(!demand)
	  {
	    demand = (Demand*) malloc(sizeof(Demand) * aSwitch->numInputs);
	  }

	tatraState = (TatraState*)aSwitch->scheduler.schedulingState;
	tatraMatrix = tatraState->tatraMatrix;
	headRow = tatraState->headRow;
	numInputs = aSwitch->numInputs;

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
		    demand[numNewInputs].input = input;
		    demand[numNewInputs].demand = bitmapNumSet(&aCell->outputs);
		    numNewInputs++;
		  }
	      }
	  }

	/* Sort inputs in order of increasing demands, with demand being 
	   num of outputs requested*/
	qsort((char*)demand, numNewInputs, sizeof(Demand), demand_compare);

	/* Add new cells at the head of queues into the tatraMatrix in order of demand*/
	for(i = 0; i < numNewInputs; i++)
	  {
	    input = demand[i].input;
	    fifo = aSwitch->inputBuffer[input]->mcastFifo[DEFAULT_PRIORITY];
	    aCell = (Cell*)fifo->head->Object;
	    if(debug_tatra)
	      {
		int op;
		printf("Dropping input %d into the TATRA matrix with outputs: ", 
		       input);
		for(op = 0; op < numFabricOutputs; op++)
		  {
		    if(bitmapIsBitSet(op, &aCell->outputs))
		      printf("1 ");
		    else
		      printf("0 ");
		  }
		printf("\n");
	      }
        
	    if(anim && now > ANIM_START)
	      {
		int op;
          
		fprintf(animFp, "D %d\n", input);
		for(op = 0; op < numFabricOutputs; op++)
		  {
		    int height = -1;
            
		    if(bitmapIsBitSet(op, &aCell->outputs))
		      {
			int r;
			r = headRow;
              
			while(tatraMatrix[op][r] != NONE)
			  r = (r + 1)%numInputs;
			height = (r - headRow + numInputs)%numInputs;
		      }
		    fprintf(animFp, "%4d", height);
		  }
		fprintf(animFp, "\n");
	      }
        
	    addToTatraMatrix(tatraState, input, &aCell->outputs);
	    if(debug_tatra)
	      {
		printf("After rearrangenment: --------->\n");
		printTatraMatrix(stdout, tatraState);
	      }
        
	    if(anim && now > ANIM_START)
	      {
		fprintf(animFp, "T\n");
		animPrintTatraMatrix(animFp, tatraState);
	      }
	  }
					
	for(output=0; output<numFabricOutputs; output++)
	  {
	    input = tatraMatrix[output][headRow];
	    aSwitch->fabric.Xbar_matrix[output].input = input;
	    if(input != NONE)
	      aSwitch->fabric.Xbar_matrix[output].cell = 
		(Cell *) aSwitch->inputBuffer[input]->mcastFifo[DEFAULT_PRIORITY]->head->Object;
	    tatraMatrix[output][headRow] = NONE;
	  }
	for(input = 0; input < aSwitch->numInputs; input++)
	  {
	    if(tatraState->peakRow[input] == headRow)
	      tatraState->peakRow[input] = NONE;
	  }
	tatraState->headRow = (headRow + 1) % tatraState->numRows;
	if(debug_tatra)
	  {
	    printf("After serving: --------->\n");
	    printTatraMatrix(stdout, tatraState);
	  }
	if(anim && now >= ANIM_START)
	  {
	    if(now > ANIM_START)
	      fprintf(animFp, "T\n");
	    else
	      fprintf(animFp, "I\n");
	    animPrintTatraMatrix(animFp, tatraState);
	  }
	break;
      }
    default:
      break;
    }
}
	    
static int 
demand_compare(d1, d2)
  Demand *d1, *d2;
{
  double d;
  static unsigned short seed[3] = {0x11ac, 0xf12b, 0x2671};
  
  if(d1->demand < d2->demand)
    return -1;
  if(d1->demand > d2->demand)
    return 1;
  
  d = erand48(seed);
  if(d < 0.5)
    return 1;
  else
    return -1;
}

static void
addToTatraMatrix(tatraState, input, request)
  TatraState *tatraState;
int input;
Bitmap *request;
{
  int output, row;
  int headRow = tatraState->headRow;
  int numRows = tatraState->numRows;
  int numOutputs = tatraState->numOutputs;
  int **tatraMatrix = tatraState->tatraMatrix;
  int *peakRow = tatraState->peakRow;
  int maxDD, maxRow, maxOutput;
  
  if(peakRow[input] != NONE)
    {
      FatalError("TATRA: peakRow != NONE in TatraMatrix");
    }
  
  maxRow = -1;
  maxOutput = -1;
  maxDD = -1;
  
  for(output = 0; output < numOutputs; output++)
    {
      if(bitmapIsBitSet(output, request))
	{
	  int startRow;
	  int opRow=0;
	  int opDD = numRows;
      
	  row = startRow = (headRow - 1 + numRows)%numRows;
	  do
	    {
	      int ip, raise1;
        
	      ip = tatraMatrix[output][row];
	      raise1 = 0;
	      if(ip != NONE)
		{
		  raise1 = (peakRow[ip] - row + numRows)%numRows;
		  if(debug_tatra)
		    {
		      printf("op = %d, ip = %d raise = %d\n", output, ip, raise1);
		    }
		  if(raise1 == 0)
		    break;
		}
	      opDD = (row + numRows - headRow)%numRows;
	      opRow = row;
	      row = (row - 1 + numRows) % numRows;
	    }while(row != startRow);
	  if(opDD > maxDD)
	    {
	      maxDD = opDD;
	      maxRow = opRow;
	      maxOutput = output;
	    }
	}
    }
  peakRow[input] = maxRow;
  if(debug_tatra)
    {
      printf("Peak of input %d at row: %d output: %d DD: %d\n",
	     input, maxRow, maxOutput, maxDD);
    }
  
  for(output = 0; output < numOutputs; output++)
    {
      if(bitmapIsBitSet(output, request))
	{
	  row = headRow;
	  do
	    {
	      int ip;
	      int thisDD;

	      ip = tatraMatrix[output][row];
	      if(ip == NONE)
		{
		  tatraMatrix[output][row] = input;
		  break;
		}
	      thisDD = (peakRow[ip] - headRow + numRows)%numRows;
          
	      if(maxDD < thisDD)
		{
		  int r;
		  int curValue;

		  curValue = input;
		  r = row;
		  do
		    {
		      int tmp;
		      tmp = tatraMatrix[output][r];
		      tatraMatrix[output][r] = curValue;
		      curValue = tmp;
		      r = (r + 1)%numRows;
		    } while(r != headRow);
		  break;
		}
	      row = (row + 1) % numRows;
	    }while(row != headRow);
	}
    }
}

void
printTatraMatrix(fp, tatraState)
  FILE *fp;     
TatraState *tatraState;
{
  int row;
  int output;

  row = tatraState->headRow;

  do
    {
      for(output = 0; output < tatraState->numOutputs; output++)
	{
	  if(tatraState->tatraMatrix[output][row] == -1)
	    fprintf(fp, "   -");
	  else
	    fprintf(fp, "%4d", tatraState->tatraMatrix[output][row]);
	}
      fprintf(fp, "\n");
      row = (row + 1) % tatraState->numRows;
    } while(row != tatraState->headRow);
}

void
animPrintTatraMatrix(fp, tatraState)
  FILE *fp;     
TatraState *tatraState;
{
  int row;
  int startRow;
  int output;
  int numRows;
  
  numRows = tatraState->numRows;

  startRow = row = (tatraState->headRow - 1 + numRows)%numRows;

  do
    {
      for(output = 0; output < tatraState->numOutputs; output++)
	{
	  fprintf(fp, "%4d", tatraState->tatraMatrix[output][row]);
	}
      fprintf(fp, "\n");
      row = (row - 1 + numRows) % numRows;
    } while(row != startRow);
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

static void
vectorPrint(fp, vector, size)
  FILE *fp;
int *vector;
int size;
{
  int i;
  for(i=0;i<size;i++)
    {
      fprintf(fp, "%d", vector[i]);
    }
  fprintf(fp,"\n");
}
#endif
