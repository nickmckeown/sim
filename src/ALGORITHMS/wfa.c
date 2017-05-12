
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
static int **ReorderGraph();
static int **RecoverGraph();
static int **makeGraph();
static int printGraph();
static int clearGraph();
static int **findWfaMatch();
static int **wfa_match();


void
wfa(action, aSwitch, argc, argv)
SwitchAction action;
Switch *aSwitch;
int argc;
char **argv;
{
	int input, output;

	if(debug_algorithm)
		printf("Algorithm 'wfa()' called by switch %d\n", aSwitch->switchNumber);
	
	switch(action) {
		case SCHEDULING_USAGE:
			fprintf(stderr, "No Options for \"wfa\" scheduling algorithm:\n");
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

			match = findWfaMatch(aSwitch);
	
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
		printf("Algorithm 'wfa()' completed for switch %d\n", aSwitch->switchNumber);
}


static int **
RecoverGraph(graph, si, so, n)
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
		newgraph[si[input]][so[output]] = graph[input][output];

	return(newgraph);
}

static int **
ReorderGraph(graph, si, so, n,aSwitch)
int **graph;
int *si, *so;
int n;
Switch *aSwitch;
{
	int input, output;
	static int **newgraph=NULL;
	static int col_prt;
	static int row_prt;

	if( !newgraph )
		newgraph = makeGraph(n);

	for(input=0; input<n; input++)
		si[input]=NONE;
	for(output=0; output<n; output++)
		so[output]=NONE;

	col_prt++;
	col_prt = col_prt%n;
	if ((col_prt%n)==0)
	  row_prt = (row_prt+1)%n;

	for(output=0; output<n; output++){
	  si[output] = (output+row_prt)%n;
	  so[output] = (output+col_prt)%n;
	}
	  
	/* Shuffle the graph */
	for(input=0; input<n; input++)
	  for(output=0; output<n; output++)
		 newgraph[input][output] = graph[si[input]][so[output]];

	if(debug_algorithm){  
	/*	if(TRUE){ */

	  printf("Order\n");
	  for(output=0; output<n; output++){
		 printf("%d:",so[output]); 
	  }
	  printf("\n");
	  for(output=0; output<n; output++){
		 printf("%d:",si[output]);
	  }
	  printf("\n");

	  printf("Original traffic \n");
	  printGraph(graph, n);	 
	  printf("Reorderes traffic \n");
	  printGraph(newgraph, n);
	 	} 
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
int 
clearGraph(graph, n)
int **graph;
int n;
{
	int input;
	for(input=0;input<n; input++)
		memset(graph[input], 0, sizeof(int)*n);
	return (0);
}

static int
printGraph(graph, n)
int **graph;
int n;
{
	int input, output;


	for(input=0;input<n; input++)
	{
		for(output=0;output<n; output++)
		{
			printf("%1d ", graph[input][output]);
		}
		printf("\n");
	}
	return (0);
}

static
int **
findWfaMatch(aSwitch)
Switch *aSwitch;
{
	int input, output, fabricOutput, index;

	static int **graph=NULL;
	static int *si=NULL;
	static int *so=NULL;
	int **match=NULL;
	int **newgraph;	
	int size;
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
	size=0;
	for(input=0; input<aSwitch->numInputs; input++)
	{
		inputBuffer = aSwitch->inputBuffer[input];
		for(output=0; output<aSwitch->numOutputs; output++)
		{
			if(inputBuffer->fifo[output]->number)
			{
				for(index=0,
					fabricOutput=output*aSwitch->fabric.Xbar_numOutputLines;
					index<aSwitch->fabric.Xbar_numOutputLines;
					index++, fabricOutput++)
				{
					graph[input][fabricOutput]=1;
					size++;
				}
			}
		}
	}
	/* printf(" %d ", size); */


	if(debug_algorithm) 
	{
		printf("Traffic:\n");
		printGraph(graph, n);
	}

	/* Reorder the columns */
	newgraph = ReorderGraph(graph, si, so, n, aSwitch); 

	match = wfa_match(n,newgraph, aSwitch);

	/* Reverse the reordering */
	newgraph = RecoverGraph(match, si, so, n);
	if(debug_algorithm) 
	{
		printf("Wfa Match:\n");
		printGraph(newgraph, n);
		printf("\n");
	}

	return(newgraph); 
}


/***********************************************************************/
static int **
wfa_match(N, graph, aSwitch)
int N;
int **graph;
Switch *aSwitch;
{
	int input, output, cycle, kk, loop2;
	static int **red=NULL;
	static int **green_v=NULL;
	static int **g=NULL;
	/* reside in sim.h */

	kk = N +1; 

	if( !red )
	  red = makeGraph(kk);

	if( !green_v )
	  green_v = makeGraph(kk);

	if( !g )
	  g = makeGraph(N);

	/* Clear all cells */
	clearGraph(red,kk);
	clearGraph(green_v,kk);
	clearGraph(g,N);

	for(input=0;input<N; input++){
	  red[input][0] = 1;
	}
	for(output=0;output<N; output++){
	  green_v[0][output] = 1;
	}
	loop2 = 2*N;
	for( cycle=0; cycle<loop2; cycle++){
	  for(output=0;output<N; output++){
		 for(input=0;input<N; input++){

			g[input][output] = red[input][output]&&green_v[input][output]&&graph[input][output];

			red[input][N-output] = red[input][N-1-output]&&(!g[input][N-1-output]);
			green_v[N-input][output] = (green_v[N-1-input][output]&&(!g[N-1-input][output]));

		 }
	  }
	  /*
	  printf("red matrix is:\n");
	  printGraph(red,kk);
	  printf("green_v matrix is:\n");
	  printGraph(green_v,kk);
	  printf("request matrix is:\n");
	  printGraph(graph,N);
	  printf("g matrix is:\n");
	  printGraph(g,N); 
	  */
	}

	if(debug_algorithm) {
	  printf("Request:\n");
	  printGraph(graph, N);
	  printf("\n");

	  printf("Grant:\n");
	  printGraph(g, N);
	  printf("\n");

	}
	return g;
}





