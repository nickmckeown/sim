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
static int **makeGraph();
static int printGraph();
static int clearGraph();
static int **findMaxWeightMatch();

void
opf_delay(action, aSwitch, argc, argv)
SwitchAction action;
Switch *aSwitch;
int argc;
char **argv;
{
	int input, output;

	if(debug_algorithm)
		printf("Algorithm 'opf_delay()' called by switch %d\n", aSwitch->switchNumber);
	
	switch(action) {
		case SCHEDULING_USAGE:
			fprintf(stderr, "No Options for \"opf_delay\" scheduling algorithm:\n");
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
		printf("Algorithm 'opf_delay()' completed for switch %d\n", aSwitch->switchNumber);
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
	return (0);
}

#define K 34
/* #define P 1 */
#define P 32

static
int **
findMaxWeightMatch(aSwitch)
Switch *aSwitch;
{
	int input, output;
	int age;
	Cell *aCell;

	static int **graph=NULL;
	static int **occupancy=NULL;
	static int **lqf_match=NULL;
	static int *si=NULL;
	static int *so=NULL;
	static int **col_sum=NULL;
	static int **row_sum=NULL;
	int **match=NULL;
	int all_backlog;
	int i, n, numFabricOutputs;

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
   if( !occupancy )
      occupancy = makeGraph(n);
   if( !lqf_match )
      lqf_match = makeGraph(n);
	if( !si )
		si = (int *)malloc(sizeof(int)*n);
	if( !so )
		so = (int *)malloc(sizeof(int)*n);

	if( !row_sum ){
	  row_sum = (int **)malloc(sizeof(int *)*K);
	  for(i=0; i<K; i++)
		 row_sum[i] = (int *) malloc(sizeof(int)*n);
	}
	if( !col_sum ){
	  col_sum = (int **)malloc(sizeof(int *)*K);
	  for(i=0; i<K; i++)
		 col_sum[i] = (int *) malloc(sizeof(int)*n);
	}

	clearGraph(graph,n);
	clearGraph(occupancy,n);
	/* clear col. and row sums */

	for(input=0; input<n; input++){
	  row_sum[0][input] = 0;
	  col_sum[0][input] = 0;
	}

	/* cal. col. and row sums */
	for(input=0; input<n; input++){
		inputBuffer = aSwitch->inputBuffer[input];	  
		for(output=0; output<aSwitch->numOutputs; output++){
		  if( inputBuffer->fifo[output]->number ){
			 aCell = inputBuffer->fifo[output]->head->Object;
		    age = 1 + now - aCell->commonStats.arrivalTime;
		  }
		  else
		    age = 0;
		  col_sum[0][output] += age;
		  row_sum[0][input] += age;
		}
	}
	/* Fill in request graph according to col. and row sums */
	all_backlog=0;

	for(input=0; input<aSwitch->numInputs; input++)	{
	  inputBuffer = aSwitch->inputBuffer[input];
	  for(output=0; output<aSwitch->numOutputs; output++){
		 if(inputBuffer->fifo[output]->number >0){
			all_backlog += inputBuffer->fifo[output]->number;
			graph[input][output]=100*(col_sum[P][output]+row_sum[P][input]) + 1; 
			/*
			  graph[input][output]=n*(col_sum[output]+row_sum[input])+lqf_match[input][output]; */

			occupancy[input][output]= inputBuffer->fifo[output]->number;
		 }
		 else
			graph[input][output]=0;
	  }
	}

	/* pipeline weights */
	for(i=0; i<P; i++){
	  for(input=0; input<n; input++){
		 col_sum[P-i][input] = col_sum[P-1-i][input];
		 row_sum[P-i][input] = row_sum[P-1-i][input];
	  }
	}

	/*	printf(" %d \t %d backlog\n", now, all_backlog);
	fprintf(TRACE_FILE," %d \t %d backlog\n", now, all_backlog); */
	if(debug_algorithm) 
	{
		printf("Traffic:\n");
		printGraph(occupancy, aSwitch);
	}

	match = ap2driver(n, graph);
	
	if(debug_algorithm) 
	{
		printf("Match:\n");
		printGraph(match, aSwitch);
		printf("\n");
	}

	return(match);
}


