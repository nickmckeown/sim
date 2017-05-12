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
  int *traffic;
  long int numCellsGenerated; /* Total number of cells generated */
  int period;
} PeriodicTraceTraffic;

/*
	Read one period of traffic from input file for all inputs/outputs.
	Keep cycling through traffic pattern.
*/

int
periodicTrace(action, aSwitch, input, argc, argv)
  TrafficAction action;
Switch *aSwitch;
int input;
int argc;
char **argv;
{

  PeriodicTraceTraffic *traffic;
  Cell *aCell;
  int output, row;
  int pflag=NO;
  int fflag=NO;

  switch( action )
    {	
    case TRAFFIC_USAGE:
      fprintf(stderr, "Options for \"periodicTrace\" traffic model:\n");
      fprintf(stderr, "    -p period \n");
      break;
    case TRAFFIC_INIT:
      {
	int c;
	extern int optind;
	extern int optopt;
	extern int opterr;
	extern char *optarg;

	char filename[256];
	FILE *fp;

	optind=1;
	opterr=0;

	if( debug_traffic)
	  printf("Traffic init for switch %d input %d\n", aSwitch->switchNumber, input);


	/* Create traffic parameters and attach to switch */
	traffic = (PeriodicTraceTraffic *) malloc(sizeof(PeriodicTraceTraffic));
	traffic->numCellsGenerated = 0;
	aSwitch->inputBuffer[input]->traffic = (void *) traffic;

	while( (c = getopt(argc, argv, "f:p:")) != EOF)
	  switch(c)
            {
	    case 'p':
	      pflag = YES;
	      traffic->period = atoi(optarg);
	      break;
	    case 'f':
	      fflag = YES;
	      strcpy(filename, optarg);
	      break;
	    case '?':
	      fprintf(stderr, "-------------------------------\n");
	      fprintf(stderr, "periodicTrace: Unrecognized option -%c\n", optopt);
	      periodicTrace(TRAFFIC_USAGE);
	      fprintf(stderr, "--------------------------------\n");
	      exit(1);
	    default:
	      break;
	    }

	if( !pflag || !fflag )
	  {
	    fprintf(stderr, "MUST specify period and input file for \"periodicTrace\" traffic at switch %d!\n", aSwitch->switchNumber);
	    exit(1);
	  }

	printf("Period: %d\n", traffic->period);

	traffic->traffic = (int *) malloc(sizeof(int)*aSwitch->numInputs*aSwitch->numOutputs*traffic->period);;

	fp = fopen(filename, "r");
	printf("Filename: %s\n", filename);
	if( !fp )
	  {
	    perror("periodicTrace()");
	    exit(1);
	  }

	/* Read contents of file: should be "period" rows.  */
	/* Each row is numInputs*numOutputs long */
	for(row=0; row<traffic->period; row++)
	  {
	    for(input=0; input<aSwitch->numInputs; input++)
	      {
		for(output=0; output<aSwitch->numOutputs; output++)
		  {
		    fscanf(fp, "%d",&traffic->traffic[row*aSwitch->numInputs*aSwitch->numOutputs + input*aSwitch->numOutputs + output]);
		    if( (traffic->traffic[row*aSwitch->numInputs*aSwitch->numOutputs + input*aSwitch->numOutputs + output] != 0) &&
			(traffic->traffic[row*aSwitch->numInputs*aSwitch->numOutputs + input*aSwitch->numOutputs + output] != 1))
		      {
			fprintf(stderr, "File %s: row %d, input %d, output %d = %d\n", filename, row, input, output, traffic->traffic[row*aSwitch->numInputs*aSwitch->numOutputs + input*aSwitch->numOutputs + output]);
		      }
		  }
	      }
	  }

	for(row=0; row<traffic->period; row++)
	  {
	    for(input=0; input<aSwitch->numInputs; input++)
	      {
		for(output=0; output<aSwitch->numOutputs; output++)
		  {
		    printf("%2d ",traffic->traffic[row*aSwitch->numInputs*aSwitch->numOutputs + input*aSwitch->numOutputs + output]);
		  }
		printf(" ");
	      }
	    printf("\n");
	  }

	break;
      }
    case TRAFFIC_GENERATE:
      {
	if( debug_traffic)
	  printf("		GEN_TRAFFIC: ");
	traffic = (PeriodicTraceTraffic *)
	  aSwitch->inputBuffer[input]->traffic;

	/* If input-output pair are connected and queue is empty, then
	   add a new cell */
	row = now%traffic->period;
	for(input=0; input<aSwitch->numInputs; input++)
	  {
	    for(output=0; output<aSwitch->numOutputs; output++)
	      {
		if( traffic->traffic[row*aSwitch->numInputs*aSwitch->numOutputs + input*aSwitch->numOutputs + output] )
		  {
					
		    if(debug_traffic)
		      printf("Switch %d,  Input %d, has new cell for %d\n", aSwitch->switchNumber, input, output);
		    aCell = createCell(output, UCAST);	
		    traffic->numCellsGenerated++;
	
		    /* Execute the switch inputAction to accept cell */
				/* YOUNGMI: BUG FIX */
		    (aSwitch->inputAction)(INPUTACTION_RECEIVE, aSwitch, input, aCell);
	
		  }
		else
		  {
		    if(debug_traffic)
		      printf("None\n");
		  }
	      }
	  }
	break;
      }
    case REPORT_TRAFFIC_STATS:
      {
	traffic = (PeriodicTraceTraffic *)	
	  aSwitch->inputBuffer[input]->traffic;
	printf("  %d  PeriodicTrace  %f\n", input, 
	       (double)traffic->numCellsGenerated/(double)now);
	break;
      }
    }

  return(CONTINUE_SIMULATION);
}
