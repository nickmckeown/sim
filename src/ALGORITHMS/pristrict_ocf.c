
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
static int **findMaxWeightPriorityMatch();


void
pristrict_ocf(action, aSwitch, argc, argv)
  SwitchAction action;
Switch *aSwitch;
int argc;
char **argv;
{
  int input, output;

  if(debug_algorithm)
    printf("Algorithm 'pristrict_ocf()' called by switch %d\n", aSwitch->switchNumber);
	
  switch(action) {
  case SCHEDULING_USAGE:
    fprintf(stderr, "No Options for \"pristrict_ocf\" scheduling algorithm:\n");
    break;
  case SCHEDULING_INIT:	
    if(debug_algorithm)
      printf("	SCHEDULING_INIT\n");

    break;

  case SCHEDULING_EXEC:
    {
      int **match, size=0;
	  int n;
	  int matched_priority_queue;
	  int selectedpriority;
	  static int **prio_value_for_matchgraph=NULL;

      int numFabricOutputs = 
	aSwitch->numOutputs*aSwitch->fabric.Xbar_numOutputLines;

 	n = (numFabricOutputs > aSwitch->numInputs) ? numFabricOutputs :
	 aSwitch->numInputs;

	  if( !prio_value_for_matchgraph) {
		  prio_value_for_matchgraph = makeGraph(n);
		 }
	  clearGraph(prio_value_for_matchgraph,n);

      match = findMaxWeightPriorityMatch(aSwitch, prio_value_for_matchgraph);
	
      for(output=0; output<numFabricOutputs; output++)
	{
	  for(input=0; input<aSwitch->numInputs; input++)
	    {
	      if(match[input][output])
		{
		  size++;
		  matched_priority_queue = prio_value_for_matchgraph[input][output];
          selectedpriority = aSwitch->numPriorities * output + matched_priority_queue; 
		  aSwitch->fabric.Xbar_matrix[output].input = input;
		  if(input!=NONE)
		    aSwitch->fabric.Xbar_matrix[output].cell = (Cell *)
		      aSwitch->inputBuffer[input]->fifo[selectedpriority]->head->Object;
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
    printf("Algorithm 'pristrict_ocf()' completed for switch %d\n", aSwitch->switchNumber);
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
findMaxWeightPriorityMatch(aSwitch,prio_value_for_matchgraph)
  Switch *aSwitch;
  int **prio_value_for_matchgraph;
{
  int input, output, fabricOutput, index;

  int newoutput,newfabricOutput, newindex;
  int anotherinput;


  static int **graph=NULL;
  static int **finalpriograph=NULL;
  static int *si=NULL;
  static int *so=NULL;
  int **match=NULL;
  int **newgraph;	
  unsigned long age;
  Cell *aCell;
  int n, numFabricOutputs;

  InputBuffer *inputBuffer;
  int pri,priority;


  numFabricOutputs = aSwitch->numOutputs*aSwitch->fabric.Xbar_numOutputLines;
  n = (numFabricOutputs > aSwitch->numInputs) ? numFabricOutputs : 
    aSwitch->numInputs;

  if(debug_algorithm)
    {
      printf("	SCHEDULING_EXEC\n");
    }


  if( !graph )
    graph = makeGraph(n);
  if( !finalpriograph )
	finalpriograph = makeGraph(n);
  if( !si )
    {
      si = (int *)malloc(sizeof(int)*n);
    }
  if( !so )
    {
      so = (int *)malloc(sizeof(int)*n);
    }

  clearGraph(graph,n);
  clearGraph(finalpriograph,n);

 /* Fill in request graph */
for(priority=0;priority<aSwitch->numPriorities;priority++)
 {
  clearGraph(graph,n);

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
		  pri = aSwitch->numPriorities * output + priority;
	      if( inputBuffer->fifo[pri]->number )
		{
		  aCell = inputBuffer->fifo[pri]->head->Object;
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

/* Change values in graph - If match already occured in a
     higher priority then dont send it for match it again */

  for(input=0; input<aSwitch->numInputs; input++)
   {
     for(output=0; output<aSwitch->numOutputs; output++)
      {
      for(index=0,
        fabricOutput=output*aSwitch->fabric.Xbar_numOutputLines;
          index<aSwitch->fabric.Xbar_numOutputLines;
          index++, fabricOutput++)
        {
          if ( finalpriograph[input][fabricOutput] == 1) {

             /* ============================================ */
          /* Blank out all requests from that input */
            for(newoutput=0; newoutput<aSwitch->numOutputs; newoutput++)
            {
                for(newindex=0,
                newfabricOutput=newoutput*aSwitch->fabric.Xbar_numOutputLines;
                newindex<aSwitch->fabric.Xbar_numOutputLines;
                newindex++, newfabricOutput++)
                {
                    graph[input][newfabricOutput]=0;
                }
             } /* Blanked all requests from that input */


          /* Blank out all requests to that output */
            for(anotherinput=0; anotherinput<aSwitch->numInputs; anotherinput++)
            {
                graph[anotherinput][fabricOutput]=0;
            }

             /* ============================================ */
          } /* End of if loop */
        }
      }
    }


  /* RANDOMIZE GRAPH */
  newgraph = shuffleGraph(graph, si, so, n);

  match = ap2driver(n, newgraph);

  /* UN-RANDOMIZE GRAPH */
  newgraph = unshuffleGraph(match, si, so, n);

 /* Copy new results of the match algorithm into the finalpriograph */

  for(input=0; input<aSwitch->numInputs; input++)
   {
     for(output=0; output<aSwitch->numOutputs; output++)
      {
      for(index=0,
        fabricOutput=output*aSwitch->fabric.Xbar_numOutputLines;
          index<aSwitch->fabric.Xbar_numOutputLines;
          index++, fabricOutput++)
        {
          if ( finalpriograph[input][fabricOutput] == 0) {
          finalpriograph[input][fabricOutput]= newgraph[input][fabricOutput];
          prio_value_for_matchgraph[input][fabricOutput] = priority;
          }
        }
      }
    }
	
 }  /* End of priority Loop */

  if(debug_algorithm) 
    {
      printf("Match:\n");
      printGraph(newgraph, aSwitch);
      printf("\n");
    }

  /* return(newgraph); */
  return(finalpriograph);
}
