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

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "sim.h"
#include "algorithm.h"
#include "neural.h"

static int **unshuffleGraph();
static int **shuffleGraph();
static int **makeGraph();
static void algorithmStats();
static int maxrand();
static void clearGraph();
static void printGraph();
static int DepthFirstFromInput();
static int DepthFirstFromOutput();
static int FindOutputSet();
static int numOutputsSet();



void
neural(action, aSwitch, argc, argv)
  SwitchAction action;
Switch *aSwitch;
int argc;
char **argv;
{
  static double **u, **v, **gain;
  static double *ColSum, *RowSum;
  static int initialized=0;
  static double threshold=DEFAULT_THRESHOLD;
  static double stepsize=DEFAULT_STEPSIZE;
  static double weight_a=A;
  static double weight_b=B;
  static double weight_c=C;
  static double gain_bw=GAIN_BW;
  static double gainNoiseFactor=0.0;
  static double rc_value=RC_VALUE;

  static int compareMaxFlag=0;
  int size=0, maxSize=0;

  InputBuffer  *inputBuffer;

  int input,output;
  int selected_input;
  double temp, rate;
  double bias;
  double columnSum, rowSum;
  double diff, maxdiff;
  double gainNoise;

  int iterations=0;
  int badChoice=0;

  if(debug_algorithm)
    printf("Algorithm 'neural()' called by switch %d\n", 
	   aSwitch->switchNumber);


  switch(action) {
  case SCHEDULING_USAGE:
    fprintf(stderr,"Options for \"neural\" scheduling algorithm:\n");
    fprintf(stderr,"    -t threshold. Default: %f\n",DEFAULT_THRESHOLD);
    fprintf(stderr,"    -s stepsize.  Default: %f\n",DEFAULT_STEPSIZE);
    fprintf(stderr,"    -a weight_A.  Default: %f\n",A);
    fprintf(stderr,"    -b weight_B.  Default: %f\n",B);
    fprintf(stderr,"    -c weight_C.  Default: %f\n",C);
    fprintf(stderr,"    -r RC_value.  Default: %f\n",RC_VALUE);
    fprintf(stderr,"    -g gain_bw.   Default: %f\n",GAIN_BW);
    fprintf(stderr,"    -f gain_noise.Default: 0.0\n");
    fprintf(stderr,"    -m : Compare with Maximum matching\n");
    break;
  case SCHEDULING_INIT:
    if(debug_algorithm) printf("    SCHEDULING_INIT\n");

    if( !initialized )
      {
	/* Parse inputs */
	extern int opterr;
	extern int optopt;
	extern int optind;
	extern char *optarg;
	int c;


	opterr=0;
	optind=1;
	while( (c = getopt(argc, argv, "t:s:a:b:c:r:g:f:m")) != EOF )
	  switch (c)
	    {
	    case 't':
	      threshold = atof(optarg);
	      break;
	    case 's':
	      stepsize = atof(optarg);
	      break;
	    case 'a':
	      weight_a = atof(optarg);
	      break;
	    case 'b':
	      weight_b = atof(optarg);
	      break;
	    case 'c':
	      weight_c = atof(optarg);
	      break;
	    case 'r':
	      rc_value = atof(optarg);
	      break;
	    case 'g':
	      gain_bw = atof(optarg);
	      break;
	    case 'f':
	      gainNoiseFactor = atof(optarg);
	      break;
	    case 'm':
	      compareMaxFlag++;
	      break;
	    case '?':
	      fprintf(stderr, "-------------------------------\n");
	      fprintf(stderr, "neural: Unrecognized option -%c\n", optopt);
	      neural(SCHEDULING_USAGE);
	      fprintf(stderr, "--------------------------------\n");
	      exit(1);
	    default:
	      break;
	    }
	printf("Threshold %f\n", threshold);
	printf("Stepsize  %f\n", stepsize);
	printf("Weight A  %f\n", weight_a);
	printf("Weight B  %f\n", weight_b);
	printf("Weight C  %f\n", weight_c);
	printf("RC Value  %f\n", rc_value);
	printf("Gain BW   %f\n", gain_bw);
	printf("Gain Noise Factor  %f\n", gainNoiseFactor);

	/* Allocate arrays for input, output and differences */
	u = (double **) calloc( aSwitch->numInputs, sizeof(double *));
	v = (double **) calloc( aSwitch->numInputs, sizeof(double *));

	/* Allocate array of gain per amplifier */
	gain = (double **) calloc( aSwitch->numInputs, sizeof(double *));

	for(input=0;input<aSwitch->numInputs;input++)
	  {
	    u[input] = (double *) calloc(aSwitch->numOutputs,sizeof(double));
	    v[input] = (double *) calloc(aSwitch->numOutputs,sizeof(double));
	    gain[input] = (double *) calloc(aSwitch->numOutputs,sizeof(double));
	    RowSum = (double *) calloc(aSwitch->numOutputs, sizeof(double));

	  }
	for(output=0;output<aSwitch->numOutputs;output++)
	  ColSum = (double *) calloc(aSwitch->numInputs, sizeof(double));

	/* Initialize random gains for amplifiers */
	for(input=0;input<aSwitch->numInputs;input++)
	  {
	    for(output=0;output<aSwitch->numOutputs;output++)
	      {
		gainNoise = (drand48() - 0.5)*gainNoiseFactor;
		gain[input][output] = (1.0+gainNoise)*gain_bw;
	      }
	  }
				

	/* Attach algorithm dependent stats to switch */
	aSwitch->scheduler.schedulingStats = 
	  (void *) malloc(sizeof(struct AlgorithmStats));
	algorithmStats(INIT_ALGORITHM_STATS, aSwitch);

	initialized = 1;
      }

    break;
  case SCHEDULING_EXEC:
    if(debug_algorithm) printf("    SCHEDULING_EXEC\n");

    /* Initialize input and output arrays */
    for (input = 0; input < aSwitch->numInputs; input++){
      inputBuffer = aSwitch->inputBuffer[input];
      for (output = 0; output < aSwitch->numOutputs; output++){
	/* Check to see if input queue is occupied */
	if (inputBuffer->fifo[output]->number > 0) 
	  {
	    bias = 0.2 * drand48() - 0.1;
	    u[input][output] = (1.0+bias)*gain[input][output];
	  }
	else
	  {
	    bias = 0.2 * drand48() - 0.1;
	    u[input][output] = -(1.0+bias) * gain[input][output];
	  }
		
	v[input][output] =  0.5 * ( 1.0 + tanh(u[input][output]/gain[input][output]));  	
      }
    }

    /* 
       Iterate until "Convergence". 
       This is defined as when the maximum difference on two
       consecutive iterations for any output is less than "threshold"
       */
    do {
      maxdiff=0.0;

      /* Calculate initial row and column sums */
      for (output = 0; output < aSwitch->numOutputs; output++){
	ColSum[output] = 0;
      }
      for (input = 0; input < aSwitch->numInputs; input++) {
	RowSum[input] = 0;
	for (output = 0; output < aSwitch->numOutputs; output++){
	  RowSum[input] += v[input][output];
	  ColSum[output] += v[input][output];
	}
      }

      for (input = 0; input < aSwitch->numInputs; input++) {
	for (output = 0; output < aSwitch->numOutputs; output++){
	  rowSum = RowSum[input] - v[input][output];	 
	  columnSum = ColSum[output] - v[input][output];	 

	  rate = (-1.0 * u[input][output] / rc_value) - (weight_a * columnSum) - 
	    (weight_b * rowSum) + (weight_c/2.0); 
		
	  u[input][output] += rate * stepsize;
						
	}
      }		

      /* Update outputs */
      for (input = 0; input < aSwitch->numInputs; input++) {
	for (output = 0; output < aSwitch->numOutputs; output++){
	  temp = v[input][output];
	  v[input][output] = 0.5 * ( 1 + tanh(u[input][output]/gain[input][output]));  	
		
	  diff = fabs(v[input][output] - temp);
	  if (diff > maxdiff) maxdiff = diff;
	}
      }
		
      iterations++;	
    } while (maxdiff > threshold);
	
    if(debug_algorithm)	
      {
	/* Check that we get sensible values */
	for (output = 0; output < aSwitch->numOutputs; output++) {
	  for (input = 0; input < aSwitch->numInputs; input++) {
	    printf("%6.6f ", v[input][output]);
	  }
	  printf("\n");
	}
	printf("\n");
	printf("Number of iterations: %d\n", iterations);	
      }

    /* Compare match with the size for a maximum matching */
    if( compareMaxFlag ) 
      {
	size=0;
	for(input=0; input<aSwitch->numInputs; input++)
	  {
	    inputBuffer = aSwitch->inputBuffer[input];
	    for(output=0; output<aSwitch->numOutputs; output++)
	      {
		if(inputBuffer->fifo[output]->number)
		  if(v[input][output] > 0.5) 
		    size++;
	      }
	  }

	printf("N: %d ", size); 
	fflush(stdout);
	maxSize = maxrand(aSwitch);
	printf("M: %d", maxSize);
	if( maxSize != size )	
	  printf(" <------------\n");
	else
	  printf("\n");
				
      }

    for (output = 0; output < aSwitch->numOutputs; output++) {
      selected_input = NONE;
      for (input = 0; input < aSwitch->numInputs; input++) {
	inputBuffer = aSwitch->inputBuffer[input];
	if( (v[input][output] > 0.5) &&
	    (inputBuffer->fifo[output]->number > 0)) {
	  if( selected_input == NONE )
	    selected_input = input; 	
	}
	if( (v[input][output] > 0.5) &&
	    (inputBuffer->fifo[output]->number == 0)) 
	  badChoice++;
      }

	  if( selected_input != NONE ) {

      aSwitch->fabric.Interconnect.Crossbar.Matrix[output].input = selected_input;
      aSwitch->fabric.Interconnect.Crossbar.Matrix[output].cell =
		aSwitch->inputBuffer[selected_input]->fifo[output]->head->Object;
	 }

    }	
    /* STATS */
    /* Update stats for number of iterations */
    algorithmStats( UPDATE_ALGORITHM_ITERATIONS_STATS, aSwitch, iterations );
    /* Update stats for number of "bad" choices */
    algorithmStats( UPDATE_ALGORITHM_BADCHOICE_STATS, aSwitch, badChoice );

    break;
  case SCHEDULING_INIT_STATS:
    algorithmStats( INIT_ALGORITHM_STATS, aSwitch );
    break;
  case SCHEDULING_REPORT_STATS:
    algorithmStats( PRINT_ALGORITHM_ITERATIONS_STATS, aSwitch );
    algorithmStats( PRINT_ALGORITHM_BADCHOICE_STATS, aSwitch );
    break;
  default:
    break;
  }


  if(debug_algorithm)
    printf("Algorithm 'neural()' completed for switch %d\n", aSwitch->switchNumber);

}


/* Update stats for number of iterations and bad choices 
	(connections selected for empty queues) */
void algorithmStats( action, aSwitch, parameter)
  AlgorithmStatsAction action;
Switch *aSwitch;
int parameter;
{
  struct AlgorithmStats *stats = aSwitch->scheduler.schedulingStats;
  switch(action)
    {
    case INIT_ALGORITHM_STATS:
      stats->sumIterations=0;
      stats->numIterations=0;
      stats->sumBadChoice=0;
      stats->numBadChoice=0;
      break;
    case UPDATE_ALGORITHM_ITERATIONS_STATS:
      stats->sumIterations += parameter;
      stats->numIterations++;
      break;
    case UPDATE_ALGORITHM_BADCHOICE_STATS:
      stats->sumBadChoice +=  parameter;
      stats->numBadChoice++;
      break;
    case PRINT_ALGORITHM_ITERATIONS_STATS:
      printf("Average number of iterations: %f\n", 
	     ( (double) stats->sumIterations ) 
	     / ( (double) stats->numIterations ));
      break;
    case PRINT_ALGORITHM_BADCHOICE_STATS:
      printf("Average number of bad choices: %f\n", 
	     ( (double) stats->sumBadChoice ) 
	     / ( (double) stats->numBadChoice ));
      break;
    }
}

static int 
maxrand(aSwitch)
  Switch *aSwitch;
{
  static int **graph=NULL;
  static int **match=NULL;
  static int **path=NULL;
  static int *si=NULL;
  static int *so=NULL;
  int **newgraph;	
  int size;

  int input, output;

  InputBuffer *inputBuffer;

  if(debug_algorithm)
    {
      printf("	MAX\n");
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

  for(input=0; input!=aSwitch->numInputs;)
    {	
      for(input=0; input<aSwitch->numInputs; input++)
	{	
	  if( (numOutputsSet(match, input, aSwitch) == 0) &&
	      DepthFirstFromInput(newgraph,match,path,input,aSwitch))	
	    {
	      break;
	    }
	}
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
      if(newgraph[input][output])
	size++;
  return(size);
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
