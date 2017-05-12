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
#include "fabric.h"
#include "inputAction.h"
#include <string.h>


struct CrossbarFabricState {
  int maxCompare;   /* If set, compare size of match with max sized match. */
};

struct CrossbarStats {
  Stat matchSize;
  Stat matchCompare;
};

int resetMatrix(Switch *aSwitch );
extern int findSizeMaxMatch(Switch *aSwitch);
extern void printCell(FILE*, Cell*);

/* 
   Crossbar switch. 
   Using aSwitch->fabric.Xbar_matrix, take one cell from each scheduled
   input fifo and move to output fifo. 
   Only one cell may be taken from each input buffer.
   Only one cell may be added to each output buffer.
*/
int 
crossbar(action, aSwitch, argc, argv)
  FabricAction action;
Switch *aSwitch;
int argc;
char **argv;
{
  /* 
     For each output in the matrix for this switch we:
     remove one cell from the corresponding input buffer
     check to see if any more destinations for the cell
     if so, add it to the input buffer at its destination
     if not, destroy the cell
     */
  Cell *aCell;
  struct CrossbarFabricState *fabricState;
  struct CrossbarStats *fabricStats;

  int outputLineWidth=1;

  if(debug_fabric)
    printf("Fabric 'crossbar()' called by switch %d\n",aSwitch->switchNumber);

  switch(action)
    {
    case FABRIC_USAGE:
      fprintf(stderr, "Options for \"crossbar\" fabric:\n");
      fprintf(stderr, "  -m Compare match with maximum size\n");
      fprintf(stderr, "  -o numOutputLines per output. Default: 1\n");
      break;
    case FABRIC_INIT:
      {
	extern int opterr, optind;
	extern int optopt;
	extern char *optarg;
	int i,c;

	if( debug_fabric )
	  printf("     Fabric: INIT crossbar matrix\n");
	if( !aSwitch->fabric.Xbar_matrix )
	  {

	    /* Parse options */	
	    opterr=0;
	    optind=1;

	    fabricState = (struct CrossbarFabricState *)
	      malloc( sizeof(struct CrossbarFabricState) );
	    aSwitch->fabric.fabricState = fabricState;
	    fabricState->maxCompare = 0;

	    for(i=0; i<=argc; i++)
	      {
		c = getopt(argc, argv, "mo:");
		switch (c)
		  {
		  case 'm':
		    fabricState->maxCompare = 1;
		    break;
		  case 'o':
		    outputLineWidth = atoi(optarg);
		    break;
		  default:
		    optind++;
		    break;
		  }
	      }

	    aSwitch->fabric.Xbar_numOutputLines = outputLineWidth; 
	    /* Create switch routing matrix */
	    aSwitch->fabric.Xbar_matrix = 
	      (MatrixEntry *) malloc( sizeof(MatrixEntry) *
				      aSwitch->numOutputs *
				      aSwitch->fabric.Xbar_numOutputLines);

	    if( outputLineWidth != 1 )
	      printf("Crossbar output line width: %d\n", 
		     aSwitch->fabric.Xbar_numOutputLines);
	  }
	/* Init switch routing matrix */
	resetMatrix(aSwitch);

	break;
      }
    case FABRIC_EXEC:
      {
	struct List  *toFifo;
	struct Element *anElement;
	int switchInput, switchOutput;
	int fabricOutput;
	int matchSize=0, maxSizeMatch=0, numFabricOutputs;
	int priority;

	if( debug_fabric )
	  printf("     Fabric: EXEC crossbar matrix\n");

	fabricState = aSwitch->fabric.fabricState;
	fabricStats = aSwitch->fabric.fabricStats;

	if( fabricState->maxCompare )
	  maxSizeMatch = findSizeMaxMatch(aSwitch);

	numFabricOutputs =
	  aSwitch->numOutputs * aSwitch->fabric.Xbar_numOutputLines;
	for(fabricOutput=0; fabricOutput<numFabricOutputs; fabricOutput++)
	  {
	    switchOutput = fabricOutput/aSwitch->fabric.Xbar_numOutputLines;
	    switchInput = aSwitch->fabric.Xbar_matrix[fabricOutput].input;
	    aCell = aSwitch->fabric.Xbar_matrix[fabricOutput].cell;
	    if( switchInput != NONE )
	      {

		aCell = (aSwitch->inputAction)(INPUTACTION_TRANSMIT, 
					       aSwitch, switchInput, (Cell *)aCell, fabricOutput);
		priority = aCell->priority;
		toFifo = aSwitch->outputBuffer[switchOutput]->fifo[priority];   /* TL */
		anElement = createElement(aCell);
		addElement(toFifo, anElement);

		matchSize++;
		
		if( debug_fabric )
		  printf("     Fabric: Update stats\n");
		/* Update latency stats for this cell */
		latencyStats(LATENCY_STATS_ARRIVE_FABRIC, NULL, aCell);
		latencyStats(LATENCY_STATS_ARRIVE_OUTPUT, NULL, aCell);
		if(debug_fabric)
		  printf("Cell from input %d to output %d priority %d at time %lu\n",
			 switchInput, fabricOutput, priority, now);
		if(debug_fabric)
		  printCell(stdout, aCell);
		
	      }
	  }
	updateStat(&fabricStats->matchSize, (long) matchSize, now);

	if( fabricState->maxCompare )
	  {
	    updateStat(&fabricStats->matchCompare, 
		       (long) maxSizeMatch-matchSize, now);
	  }
	resetMatrix(aSwitch);

	break;
      }
    case FABRIC_STATS_PRINT:
      {
	fabricStats = aSwitch->fabric.fabricStats;
	fabricState = aSwitch->fabric.fabricState;

	printf("\n");
	printf("  Crossbar statistics\n");
	printf("  -------------------\n");
	printf("                       Avg       SD     Number\n");
	printf("                     -------------------------\n");
	printStat(stdout, "Match Size:         ", 
		  &fabricStats->matchSize);
	if( fabricState->maxCompare )
	  printStat(stdout, "Match diff from Max:", 
		    &fabricStats->matchCompare);
	break;
      }
    case FABRIC_STATS_INIT:
      {
	fabricState = aSwitch->fabric.fabricState;
	fabricStats = aSwitch->fabric.fabricStats;
	if( !aSwitch->fabric.fabricStats )
	  {
	    /* Create and init fabric stats */
	    fabricStats= (struct CrossbarStats *)
	      malloc(sizeof(struct CrossbarStats));
	    aSwitch->fabric.fabricStats = (void *) fabricStats;
	  }
	initStat(&fabricStats->matchSize, STAT_TYPE_AVERAGE, now);
	initStat(&fabricStats->matchCompare, STAT_TYPE_AVERAGE, now);
	enableStat(&fabricStats->matchSize);
	if( fabricState->maxCompare )
	  enableStat(&fabricStats->matchCompare);
	break;
      }
    default:
      fprintf(stderr, "\nCommand not implemented in crossbar.c\n");
      exit(2);
    }
  if( debug_fabric )
    printf("     Fabric: Completed\n");

  return (0);
}

int resetMatrix( aSwitch )
  Switch *aSwitch;
{
  int numFabricOutputs;
  
  numFabricOutputs = aSwitch->numOutputs*aSwitch->fabric.Xbar_numOutputLines;
  
  if( debug_fabric )
    printf("     Fabric: Reset crossbar matrix\n");
  memset(aSwitch->fabric.Xbar_matrix, NONE, numFabricOutputs*sizeof(MatrixEntry));
  return (0);
}
