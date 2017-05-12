
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
#include "wwfa.h"
#include "scheduleStats.h"

static int find_match();
static int createScheduleState();
static int **makeGraph();
static int **makeGraph();
static int printGraph();
static int clearGraph();

/*  Determines switch configuration 					*/
/*  Configuration is filled into aSwitch->fabric.interconnect.matrix array 	*/

void
wwfa(action, aSwitch, argc, argv)
SwitchAction action;
Switch *aSwitch;
int argc;
char **argv;
{
	SchedulerState 			*scheduleState;

	int input;

	if(debug_algorithm)
		printf("Algorithm 'wwfa()' called by switch %d\n", aSwitch->switchNumber);
	
	switch(action) {
		case SCHEDULING_USAGE:
            fprintf(stderr, "No Options for \"wwfa\" scheduling algorithm:\n");
            break;
		case SCHEDULING_INIT:	
			if(debug_algorithm) printf("	SCHEDULING_INIT\n");
			
			if( aSwitch->scheduler.schedulingState == NULL )
			{
				extern int opterr;
				extern int optopt;
                extern int optind;
                extern char *optarg;

				/* Allocate scheduler state for this switch */
				createScheduleState(aSwitch);
				scheduleState = (SchedulerState *) aSwitch->scheduler.schedulingState;

			}

			break;

		case SCHEDULING_EXEC:
		{
		
			if(debug_algorithm) printf("	SCHEDULING_EXEC\n");

			/*************** INITIALIZE ***********/

			/*************** END INITIALIZE ***********/

			input = find_match(aSwitch);
			if(debug_algorithm) printf(" SCHEDULING_EXEC end\n");		
			break;
		}

		case SCHEDULING_INIT_STATS:
			scheduleCellStats( SCHEDULE_CELL_STATS_INIT, aSwitch);
			break;
		case SCHEDULING_REPORT_STATS:
			scheduleCellStats( SCHEDULE_CELL_STATS_PRINT_ALL, aSwitch );
			break;
		default:
			break;
	}

	if(debug_algorithm)
		printf("Algorithm 'wwfa()' completed for switch %d\n", aSwitch->switchNumber);
}

/***********************************************************************/
static int
find_match(aSwitch)
Switch *aSwitch;
{

	int input, output, i, j, k, wave, diag, point;
	SchedulerState	*scheduleState=(SchedulerState *)aSwitch->scheduler.schedulingState;
	InputBuffer  *inputBuffer;
	int N = aSwitch->numInputs;
	int cell_S[30][30];
	int cell_E[30][30];
	static int **graph=NULL;

	if( !graph )
	  graph = makeGraph(N);

	clearGraph(graph,N);
	
	point = scheduleState->pointer;
	if ((point<0)||(point >= N))
	  point = 0;
	scheduleState->pointer = point+1;

	/* note i == output, j = input */
	/* Clear all cells */
	for( i=0; i<N; i++)
	  for( j=0; j<N; j++)
		 {
			cell_E[i][j] = 1;
			cell_S[i][j] = 1;
		 }
	/* clear the macthing */
	for ( output=0; output<N; output++)
	  aSwitch->fabric.Xbar_matrix[output].input = NONE;


	/* Now do arbitration start with the first wave */

	for( wave=0; wave<N; wave++)
	  {
		 diag = (point+wave)%N;
		 for( k=0; k<N; k++)
			{
			  i = k;
			  j = ((diag-k+N)%N);
			  output = i;
			  input = j;
			  inputBuffer = aSwitch->inputBuffer[input];
			  if (wave==0)
				 { 
					/* this is the highest priority diagonal, all requests will be granted */

					if ( inputBuffer->fifo[output]->number > 0 )
					  {
						 /* mark granted */
						 cell_E[i][j] = 0;
						 cell_S[i][j] = 0;
						 graph[input][output] = 1;
						 aSwitch->fabric.Xbar_matrix[output].input = input;
						 aSwitch->fabric.Xbar_matrix[output].cell = (Cell *)
                        aSwitch->inputBuffer[input]->fifo[output]->head->Object;
					  }
				 }
			  else
				 if (( inputBuffer->fifo[output]->number > 0 ) && cell_E[i][(j-1+N)%N] && cell_S[(i-1+N)%N][j])
					{
					  cell_E[i][j] = 0;
					  cell_S[i][j] = 0;
					  graph[input][output] = 1;
					  aSwitch->fabric.Xbar_matrix[output].input = input;
					  aSwitch->fabric.Xbar_matrix[output].cell = (Cell *)
                        aSwitch->inputBuffer[input]->fifo[output]->head->Object;
					}
				 else
					{
					  cell_E[i][j] = cell_E[i][(j-1+N)%N];
					  cell_S[i][j] = cell_S[(i-1+N)%N][j];
					}
			}
	  }
	if(debug_algorithm)
	  {
		 printf("WWFA Match:\n");
		 printGraph(graph, aSwitch);
		 printf("\n");
	  }

	return (0);
}

/* Create schedule state variables for aSwitch */
static int 
createScheduleState(aSwitch)
Switch *aSwitch;
{
	SchedulerState *scheduleState;

	
	scheduleState = (SchedulerState *) malloc(sizeof(SchedulerState));
	aSwitch->scheduler.schedulingState = scheduleState;
   return (0);
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

