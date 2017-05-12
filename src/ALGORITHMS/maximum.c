
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

/*  Determines switch configuration 					*/
/*  Configuration is filled into aSwitch->fabric.interconnect.matrix array 	*/

static int **makeGraph();
static void printGraph();
static void clearGraph();
static int numOutputsSet();
static int FindOutputSet();
static int DepthFirstFromInput();
static int DepthFirstFromOutput();

void
maximum(action, aSwitch, argc, argv)
  SwitchAction action;
Switch *aSwitch;
int argc;
char **argv;
{
  int input, output;

  if(debug_algorithm)
    printf("Algorithm 'maximum()' called by switch %d\n", aSwitch->switchNumber);
	
  switch(action) {
  case SCHEDULING_USAGE:
    fprintf(stderr, "No Options for \"maximum\" scheduling algorithm:\n");
    break;
  case SCHEDULING_INIT:	
    if(debug_algorithm)
      printf("	SCHEDULING_INIT\n");

    break;

  case SCHEDULING_EXEC:
    {
      static int **graph=NULL;
      static int **match=NULL;
      static int **path=NULL;

      InputBuffer *inputBuffer;

      if(debug_algorithm)
	{
	  printf("	SCHEDULING_EXEC\n");
	}


      if( !graph )
	graph = makeGraph(aSwitch);
      if( !match )
	match = makeGraph(aSwitch);
      if( !path )
	path = makeGraph(aSwitch);
	
      clearGraph(graph,aSwitch);
      clearGraph(match,aSwitch);
      clearGraph(path,aSwitch);

      /* Fill in request graph */
      for(input=0; input<aSwitch->numInputs; input++)
	{
	  inputBuffer = aSwitch->inputBuffer[input];
	  for(output=0; output<aSwitch->numOutputs; output++)
	    {
	      if(inputBuffer->fifo[output]->number)
		graph[input][output]=1;
	    }
	}

      if(debug_algorithm)
	{
	  printf("Traffic:\n");
	  printGraph(graph, aSwitch);
	}
      for(input=0; input!=aSwitch->numInputs;)
	{	
	  for(input=0; input<aSwitch->numInputs; input++)
	    {	
	      if( (numOutputsSet(match, input, aSwitch) == 0) &&
		  DepthFirstFromInput(graph,match,path,input,aSwitch))	
		{
		  break;
		}
	    }
	}
      if(debug_algorithm)
	{
	  printf("Match:\n");
	  printGraph(match, aSwitch);
	  printf("\n");
	}


      for(output=0; output<aSwitch->numOutputs; output++)
	for(input=0; input<aSwitch->numInputs; input++)
	  if(match[input][output]) {
	    aSwitch->fabric.Interconnect.Crossbar.Matrix[output].input = input;
	    aSwitch->fabric.Interconnect.Crossbar.Matrix[output].cell = 
	    aSwitch->inputBuffer[input]->fifo[output]->head->Object; /* Sundar */
	   }
      break;
    }

  default:
    break;
  }

	

  if(debug_algorithm)
    printf("Algorithm 'maximum()' completed for switch %d\n", aSwitch->switchNumber);
}

/***********************************************************************/
static int
DepthFirstFromInput(graph, match, path, startVertex, aSwitch)
  int **graph;
int **match;
int **path;
int startVertex;
Switch *aSwitch;
{
  int output;

  for(output=0; output<aSwitch->numOutputs; output++)
    {
      if( graph[startVertex][output] && !path[0][output]
	  && DepthFirstFromOutput(graph, match, path, output, aSwitch) )
	{
	  match[startVertex][output] = 1;
	  return(1);
	}
    }
  return(0);
}

static int
DepthFirstFromOutput(graph, match, path, startVertex, aSwitch)
  int **graph;
int **match;
int **path;
int startVertex;
Switch *aSwitch;
{
  int result;

  int i = FindOutputSet(match, startVertex, aSwitch);

  if( i == NONE )
    return( 1 );

  path[0][startVertex] = 1;

  if( (result = DepthFirstFromInput(graph, match, path, i, aSwitch)) )
    match[i][startVertex]=0;
  path[0][startVertex]=0;
  return(result);
}

static int
FindOutputSet(graph, output, aSwitch)
  int **graph;
int output;
Switch *aSwitch;
{
  int input;

  for(input=0; input<aSwitch->numInputs; input++)
    {	
      if( graph[input][output] )
	return(input);
    }
  return(NONE);
}


static int
numOutputsSet(graph, input, aSwitch)
  int **graph;
int input;
Switch *aSwitch;
{
  int output, sum=0;

  for(output=0; output<aSwitch->numOutputs; output++)
    sum += graph[input][output];

  return(sum);
}

static int **
makeGraph(aSwitch)
  Switch *aSwitch;
{
  int input;
  int **graph;

  graph = (int **) malloc(sizeof(int *) * aSwitch->numInputs);
  for(input=0;input<aSwitch->numInputs; input++)
    graph[input] = (int *) malloc(sizeof(int)*aSwitch->numOutputs);

  return(graph);
}
static void
clearGraph(graph, aSwitch)
  int **graph;
Switch *aSwitch;
{
  int input;
  for(input=0;input<aSwitch->numInputs; input++)
    memset(graph[input], 0, sizeof(int)*aSwitch->numOutputs);
}
static void
printGraph(graph, aSwitch)
  int **graph;
Switch *aSwitch;
{
  int input, output;
  for(input=0;input<aSwitch->numInputs; input++)
    {
      for(output=0;output<aSwitch->numOutputs; output++)
	{
	  printf("%1d ", graph[input][output]);
	}
      printf("\n");
    }
}
