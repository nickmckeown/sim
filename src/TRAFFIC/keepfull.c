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

#include "sim.h"
#include "traffic.h"
#include "inputAction.h"

typedef struct {
  long int numCellsGenerated; /* Total number of cells generated */
  int *connected; 			/* Whether or not connected to each output */
} KeepfullTraffic;

/*
	Keep all connections full all the time. Do this, by supplying a
    cell to all input-output pairs (in the traffic pattern) that have 
	gone empty.
*/

int
keepfull(action, aSwitch, input, argc, argv)
  TrafficAction action;
Switch *aSwitch;
int input;
int argc;
char **argv;
{

  KeepfullTraffic *traffic;
  Cell *aCell;
  double psend;
  int output;
  int cflag=NO;



  switch( action )
    {	
    case TRAFFIC_USAGE:
      fprintf(stderr, "Options for \"keepfull\" traffic model:\n");
      fprintf(stderr, "    -c c0 c1 c2 c3 ... cN-1. \n");
      break;
    case TRAFFIC_INIT:
      {
	int c;
	extern int optind;
	extern int optopt;
	extern int opterr;
	extern char *optarg;


	optind=1;
	opterr=0;

	if( debug_traffic)
	  printf("Traffic init for switch %d input %d\n", aSwitch->switchNumber, input);


	/* Create traffic parameters and attach to switch */
	traffic = (KeepfullTraffic *) malloc(sizeof(KeepfullTraffic));
	traffic->connected = (int *)malloc(sizeof(int)*aSwitch->numOutputs);
	traffic->numCellsGenerated = 0;
	aSwitch->inputBuffer[input]->traffic = (void *) traffic;

	while( (c = getopt(argc, argv, "c:")) != EOF)
	  switch(c)
            {
	    case 'c':
	      cflag = YES;
	      /* optind points to next arg string */
	      for(output=0; output<aSwitch->numOutputs; output++)
		sscanf(argv[output+optind-1], "%d",
		       &traffic->connected[output]);
	      break;
	    case '?':
	      fprintf(stderr, "-------------------------------\n");
	      fprintf(stderr, "keepfull: Unrecognized option -%c\n", optopt);
	      keepfull(TRAFFIC_USAGE);
	      fprintf(stderr, "--------------------------------\n");
	      exit(1);
	    default:
	      break;
	    }

	if( !cflag )
	  {
	    fprintf(stderr, "MUST specify connections for \"keepfull\" traffic at switch %d!\n", aSwitch->switchNumber);
	    exit(1);
	  }

	/* If this is the last input for this switch, then print matrix for all 
	   inputs that use keepfull traffic */
	if( input == aSwitch->numInputs-1 )
	  {
	    printf("    Traffic requests: \n    ");
	    for(output=0; output<aSwitch->numOutputs; output++)
	      printf("%2d ", output);
	    printf("\n    -----------------------------\n    ");
	    for(input=0; input<aSwitch->numInputs; input++)
	      {
		printf("%2d |", input);
		traffic = aSwitch->inputBuffer[input]->traffic;
		for(output=0; output<aSwitch->numOutputs; output++)
		  printf("%2d ", traffic->connected[output]);
		printf("\n");
	      }
	  }

	break;
      }
    case TRAFFIC_GENERATE:
      {
	if( debug_traffic)
	  printf("		GEN_TRAFFIC: ");
	traffic = (KeepfullTraffic *)aSwitch->inputBuffer[input]->traffic;
	psend = drand48();

	/* Decide whether or not to generate a new cell */
	/* If input-output pair are connected and queue is empty, then
	   add a new cell */
	for(output=0; output<aSwitch->numOutputs; output++)
	  {
	    if( traffic->connected[output] &&
		! aSwitch->inputBuffer[input]->fifo[output]->number )
	      {
				
		if(debug_traffic)
		  printf("Switch %d,  Input %d, has new cell for %d\n", aSwitch->switchNumber, input, output);
		aCell = createCell(output, UCAST, DEFAULT_PRIORITY);	
		traffic->numCellsGenerated++;

                /* Execute the switch inputAction to accept cell */
								/* YOUNGMI: BUG FIX */
                (aSwitch->inputAction)(INPUTACTION_RECEIVE, aSwitch, input, aCell, NULL, NULL, NULL, NULL);

	      }
	    else
	      {
		if(debug_traffic)
		  printf("None\n");
	      }
	  }
	break;
      }
    case REPORT_TRAFFIC_STATS:
      {
	traffic = (KeepfullTraffic *)aSwitch->inputBuffer[input]->traffic;
	printf("  %d  Keepfull  %f\n", input, 
	       (double)traffic->numCellsGenerated/(double)now);
	break;
      }
    }

  return(CONTINUE_SIMULATION);
}
