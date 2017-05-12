
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
/*  Finds maximum (random) match */

static int **makeGraph();
static int **shuffleGraph();
static int **unshuffleGraph();
static int **makeGraph();
static void printGraph();
static void clearGraph();
static int numOutputsSet();
static int FindOutputSet();
static int DepthFirstFromInput();
static int DepthFirstFromOutput();
static void initialMatch();

void
maxrand(action, aSwitch, argc, argv)
  SwitchAction action;
Switch *aSwitch;
int argc;
char **argv;
{
  int input, output;

  if(debug_algorithm)
    printf("Algorithm 'maxrand()' called by switch %d\n", aSwitch->switchNumber);
	
  switch(action) {
  case SCHEDULING_USAGE:
    fprintf(stderr, "No Options for \"maxrand\" scheduling algorithm:\n");
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
      static int *si=NULL;
      static int *so=NULL;
      int **newgraph;	
      int size;

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
      if( !si )
	{
	  si = (int *)malloc(sizeof(int)*aSwitch->numInputs);
	  printf("Make si\n");
	}
      if( !so )
	{
	  so = (int *)malloc(sizeof(int)*aSwitch->numOutputs);
	  printf("Make so\n");
	}
	
      clearGraph(graph,aSwitch);
      clearGraph(match,aSwitch);
      clearGraph(path,aSwitch);

      /* Fill in request graph */
      size=0;
      for(input=0; input<aSwitch->numInputs; input++)
	{
	  inputBuffer = aSwitch->inputBuffer[input];
	  for(output=0; output<aSwitch->numOutputs; output++)
	    {
	      if(inputBuffer->fifo[output]->number)
		{
		  graph[input][output]=1;
		  size++;
		}
	    }
	}
      /* printf(" %d ", size); */


      if(debug_algorithm) 
	{
	  printf("Traffic:\n");
	  printGraph(graph, aSwitch);
	}

      /* RANDOMIZE GRAPH */
      newgraph = shuffleGraph(graph, si, so, aSwitch);

      initialMatch(newgraph, match, aSwitch);

      for(input=0; input<aSwitch->numInputs; input++)
	{	
	  if(numOutputsSet(match, input, aSwitch) == 0) 
	    DepthFirstFromInput(newgraph,match,path,input,aSwitch)	;
	}
			
      /* UN-RANDOMIZE GRAPH */
      newgraph = unshuffleGraph(match, si, so, aSwitch);
      if(debug_algorithm) 
	{
	  printf("Match:\n");
	  printGraph(newgraph, aSwitch);
	  printf("\n");
	}


      size=0;
      for(output=0; output<aSwitch->numOutputs; output++)
	for(input=0; input<aSwitch->numInputs; input++)
	  {
	    if(newgraph[input][output])
	      {
		size++;
		aSwitch->fabric.Interconnect.Crossbar.Matrix[output].input = input;
		aSwitch->fabric.Interconnect.Crossbar.Matrix[output].cell = 
		aSwitch->inputBuffer[input]->fifo[output]->head->Object; 
		/* Sundar*/
	      }
	  }
      /* printf(" %d\n", size); */
      break;
    }

  default:
    break;
  }

	

  if(debug_algorithm)
    printf("Algorithm 'maxrand()' completed for switch %d\n", aSwitch->switchNumber);
}


static int **
unshuffleGraph(graph, si, so, aSwitch)
  int **graph;
int *si, *so;
Switch *aSwitch;
{
  int input, output;

  static int **newgraph=NULL;
  if( !newgraph )
    newgraph = makeGraph(aSwitch);

  /* UnShuffle the graph */
  for(input=0; input<aSwitch->numInputs; input++)
    for(output=0; output<aSwitch->numOutputs; output++)
      newgraph[input][output] = graph[si[input]][so[output]];

  return(newgraph);
}
static int **
shuffleGraph(graph, si, so, aSwitch)
  int **graph;
int *si, *so;
Switch *aSwitch;
{
  int input, output;
  int selection;
  static int **newgraph=NULL;
  if( !newgraph )
    newgraph = makeGraph(aSwitch);

  for(input=0; input<aSwitch->numInputs; input++)
    si[input]=NONE;
  for(output=0; output<aSwitch->numOutputs; output++)
    so[output]=NONE;

  /* Shuffle set of inputs */
  for(input=0; input<aSwitch->numInputs; input++)
    {
      do {
	selection = lrand48()%aSwitch->numInputs;
      } while(si[selection] != NONE);
      si[selection] = input;
    }
  /* Shuffle set of outputs */
  for(output=0; output<aSwitch->numOutputs; output++)
    {
      do {
	selection = lrand48()%aSwitch->numOutputs;
      } while(so[selection] != NONE);
      so[selection] = output;
    }

  /* Shuffle the graph */
  for(input=0; input<aSwitch->numInputs; input++)
    for(output=0; output<aSwitch->numOutputs; output++)
      newgraph[si[input]][so[output]] = graph[input][output];

  return(newgraph);
	
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

  if( (result = DepthFirstFromInput(graph, match, path, i, aSwitch) ))
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

static 
void clearGraph(graph, aSwitch)
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

static void initialMatch(graph, match, aSwitch)
  int **graph, **match;
Switch *aSwitch;
{
  int input, output;
  for(input=0; input<aSwitch->numInputs;input++)
    {
      for(output=0; output<aSwitch->numOutputs;output++)
	{
	  if( graph[input][output] && FindOutputSet(match, output, aSwitch) == NONE )
	    {	
	      match[input][output] = 1;
	      break;
	    }
	}
    }
}
