
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

#include <string.h>
#include "sim.h"
#include "algorithm.h"
#include "assign2.h"


/*  Determines switch configuration 					*/
/*  Configuration is filled into aSwitch->fabric.Xbar_matrix array 	*/
/*  Finds maximum sized match using M Saltzman's code */

static int **makeGraph();
static int **shuffleGraph();
static int **unshuffleGraph();
static int **makeGraph();
static void printGraph();
static void clearGraph();
static int **findMaxWeightMatch();


void
ocf(action, aSwitch, argc, argv)
  SwitchAction action;
Switch *aSwitch;
int argc;
char **argv;
{
  int input, output;

  if(debug_algorithm)
    printf("Algorithm 'ocf()' called by switch %d\n", aSwitch->switchNumber);
	
  switch(action) {
  case SCHEDULING_USAGE:
    fprintf(stderr, "No Options for \"ocf\" scheduling algorithm:\n");
    break;
  case SCHEDULING_INIT:	
    if(debug_algorithm)
      printf("	SCHEDULING_INIT\n");

    break;

  case SCHEDULING_EXEC:
    {
      int **match, size=0;
      int numFabricOutputs = 
	aSwitch->numOutputs*aSwitch->fabric.Xbar_numOutputLines;

      match = findMaxWeightMatch(aSwitch);
	
      for(output=0; output<numFabricOutputs; output++)
	{
	  for(input=0; input<aSwitch->numInputs; input++)
	    {
	      if(match[input][output])
		{
		  size++;
		  aSwitch->fabric.Xbar_matrix[output].input = input;
		  if(input!=NONE)
		    aSwitch->fabric.Xbar_matrix[output].cell = (Cell *)
		      aSwitch->inputBuffer[input]->fifo[output]->head->Object;
		}
	    }
	}
      if( debug_algorithm )
	printf(" Size: %d\n", size); 

      break;
    }

  default:
    break;
  }

	

  if(debug_algorithm)
    printf("Algorithm 'ocf()' completed for switch %d\n", aSwitch->switchNumber);
}


static int **
unshuffleGraph(graph, si, so, n)
  int **graph;
int *si, *so;
int n;
{
  int input, output;

  static int **newgraph=NULL;
  if( !newgraph )
    newgraph = makeGraph(n);

  /* UnShuffle the graph */
  for(input=0; input<n; input++)
    for(output=0; output<n; output++)
      newgraph[input][output] = graph[si[input]][so[output]];

  return(newgraph);
}
static int **
shuffleGraph(graph, si, so, n)
  int **graph;
int *si, *so;
int n;
{
  static unsigned short int mySeed[3];
  static int init=0;

  int input, output;
  int selection;
  static int **newgraph=NULL;

  if( !newgraph )
    newgraph = makeGraph(n);

  for(input=0; input<n; input++)
    si[input]=NONE;
  for(output=0; output<n; output++)
    so[output]=NONE;

  if(!init)
    {
      mySeed[0] = 0x1243;
      mySeed[1] = 0xab43;
      mySeed[2] = 0xfc92;
      init=1;
    }
  /* Shuffle set of inputs */
  for(input=0; input<n; input++)
    {
      do {
	selection = nrand48(mySeed)%n;
      } while(si[selection] != NONE);
      si[selection] = input;
    }
  /* Shuffle set of outputs */
  for(output=0; output<n; output++)
    {
      do {
	selection = nrand48(mySeed)%n;
      } while(so[selection] != NONE);
      so[selection] = output;
    }

  /* Shuffle the graph */
  for(input=0; input<n; input++)
    for(output=0; output<n; output++)
      newgraph[si[input]][so[output]] = graph[input][output];

  return(newgraph);
	
}

/***********************************************************************/
static int **
makeGraph(n)
  int n;
{
  int input;
  int **graph;

  graph = (int **) malloc(sizeof(int *) * n);
  for(input=0;input<n; input++)
    graph[input] = (int *) malloc(sizeof(int)*n);

  return(graph);
}

static 
void
clearGraph(graph, n)
  int **graph;
int n;
{
  int input;
  for(input=0;input<n; input++)
    memset(graph[input], 0, sizeof(int)*n);
}

static void
printGraph(graph, aSwitch)
  int **graph;
Switch *aSwitch;
{
  int input, output, numFabricOutputs;

  numFabricOutputs = aSwitch->numOutputs*aSwitch->fabric.Xbar_numOutputLines;
  for(input=0;input<aSwitch->numInputs; input++)
    {
      for(output=0;output<numFabricOutputs; output++)
	{
	  printf("%1d ", graph[input][output]);
	}
      printf("\n");
    }
}

static
int **
findMaxWeightMatch(aSwitch)
  Switch *aSwitch;
{
  int input, output, fabricOutput, index;

  static int **graph=NULL;
  static int *si=NULL;
  static int *so=NULL;
  int **match=NULL;
  int **newgraph;	
  unsigned long age;
  Cell *aCell;
  int n, numFabricOutputs;

  InputBuffer *inputBuffer;

  numFabricOutputs = aSwitch->numOutputs*aSwitch->fabric.Xbar_numOutputLines;
  n = (numFabricOutputs > aSwitch->numInputs) ? numFabricOutputs : 
    aSwitch->numInputs;

  if(debug_algorithm)
    {
      printf("	SCHEDULING_EXEC\n");
    }


  if( !graph )
    graph = makeGraph(n);
  if( !si )
    {
      si = (int *)malloc(sizeof(int)*n);
    }
  if( !so )
    {
      so = (int *)malloc(sizeof(int)*n);
    }

  clearGraph(graph,n);

  /* Fill in request graph */
  for(input=0; input<aSwitch->numInputs; input++)
    {
      inputBuffer = aSwitch->inputBuffer[input];
      for(output=0; output<aSwitch->numOutputs; output++)
	{
	  for(index=0,
		fabricOutput=output*aSwitch->fabric.Xbar_numOutputLines;
	      index<aSwitch->fabric.Xbar_numOutputLines;
	      index++, fabricOutput++)
	    {
	      if( inputBuffer->fifo[output]->number )
		{
		  aCell = inputBuffer->fifo[output]->head->Object;
		  age = 1 + now - aCell->commonStats.arrivalTime;
		  graph[input][fabricOutput]=age;
		}
	    }
	}
    }
  /* printf(" %d ", weight); */


  if(debug_algorithm) 
    {
      printf("Traffic:\n");
      printGraph(graph, aSwitch);
    }

  /* RANDOMIZE GRAPH */
  newgraph = shuffleGraph(graph, si, so, n);

  match = ap2driver(n, newgraph);

	
  /* UN-RANDOMIZE GRAPH */
  newgraph = unshuffleGraph(match, si, so, n);
  if(debug_algorithm) 
    {
      printf("Match:\n");
      printGraph(newgraph, aSwitch);
      printf("\n");
    }

  return(newgraph);
}
