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
#include "inputAction.h"

#define DEFAULT_MAX_CELLS NONE

struct inputActionState {
	int maxCellsPerInputBuffer;
	int maxCellsPerFIFO;
	int insertOutputQueueingTimes;
};

extern void switchStats();

/* IFF input queue is not blocked, accept cell. */

Cell *
defaultInputAction(cmd, aSwitch, input, aCell, output, mcastflag, argc, argv)
InputActionCmd cmd;
Switch *aSwitch;
int input;
Cell *aCell;
int output;
int mcastflag;
int argc;
char **argv;
{
	struct inputActionState *actionState;
	struct Element *anElement;
	int out, pri, priority;
	int numFIFOs;
	switch( cmd )
	{
	case INPUTACTION_USAGE:
	  fprintf(stderr, "Options for \"default\" input action:\n");
	  fprintf(stderr, "  -m  maxCells per input buffer. Default: Infinite\n");
	  fprintf(stderr, "  -n  maxCells per fifo. Default: m/(# inputs)\n");
	  fprintf(stderr, "  -o  Measure output queueing departure times. Default: off\n");
	  break;
	case INPUTACTION_INIT:
	  {
	    extern int opterr;
	    extern int optopt;
	    extern int optind;
	    extern char *optarg;
	    int nflag=0;
	    int mflag=0;
	    int oflag=0;
	    int c;
	    
	    actionState = (struct inputActionState * )
	      malloc(sizeof(struct inputActionState));
	    
	    actionState->insertOutputQueueingTimes = NO;
	    opterr=0; optind=1; 
	    while( (c = getopt(argc, argv, "m:n:o")) != EOF)
	      switch (c)
		{			
		case 'm':
		  actionState->maxCellsPerInputBuffer = atoi(optarg);
		  mflag++;
		  break;
		case 'n':
		  actionState->maxCellsPerFIFO = atoi(optarg);
		  nflag++;
		  break;
		case 'o':
		  actionState->insertOutputQueueingTimes = YES;
		  oflag++;
		  break;
		case '?':
		  fprintf(stderr, "--------------------------------------------\n");
		  fprintf(stderr, "defaultInputAction: Unrecognized option -%c\n", optopt);
		  defaultInputAction(INPUTACTION_USAGE);
		  fprintf(stderr, "--------------------------------------------\n");
		  exit(1);
		  
		  break;
		default:
		  break;
		}

	    if( oflag )
	      printf("Determining output queue departure times.\n");
	    
	    if( !mflag )
	      {
		actionState->maxCellsPerInputBuffer = NONE;
		actionState->maxCellsPerFIFO = NONE;
	      }
	    else if( !nflag )
	      {
		actionState->maxCellsPerFIFO = actionState->maxCellsPerInputBuffer /
		  aSwitch->numOutputs;
	      }

	    if( actionState->maxCellsPerInputBuffer == NONE )
	      printf("    Max cells per input buffer: INFINITE\n");
	    else
	      printf("    Max cells per input buffer: %d\n", 
		     actionState->maxCellsPerInputBuffer);
	    if( actionState->maxCellsPerFIFO == NONE )
	      printf("    Max cells per input FIFO: INFINITE\n");
	    else
	      printf("    Max cells per input FIFO: %d\n", 
		     actionState->maxCellsPerFIFO);
			
	    /*******************************************/
	    /* Attach switch dependent state to switch */
	    /*******************************************/
	    aSwitch->inputActionState = (void *) actionState;
	    break;
	  }
	case INPUTACTION_RECEIVE:
	  {
	    int inputBufferOccupancy=0;
	    long numAhead=0;
            int anInput;
	    actionState =
	      (struct inputActionState *) aSwitch->inputActionState;
	    
	    
	    out = aCell->vci;
	    priority = aCell->priority;
	    if(priority > aSwitch->numPriorities)
	      {
		fprintf(stderr, "defaultInputAction.c:\n");
		fprintf(stderr, "Cells has priorities %d not supported by switch %d.\n", priority, aSwitch->numPriorities);
		exit(1);
	      }

	    pri = out * aSwitch->numPriorities + priority;
		
	    if(actionState->insertOutputQueueingTimes == YES)
	      {
		/* Determine the numahead of cell for this output */
		numAhead += aSwitch->outputBuffer[out]->fifo[priority]->number;
		for(anInput=0; anInput<aSwitch->numInputs; anInput++)
		  numAhead += aSwitch->inputBuffer[anInput]->fifo[pri]->number;
		aCell->commonStats.oqDepartTime = now + numAhead;
	      }
	    
	    if( actionState->maxCellsPerInputBuffer != NONE )
	      {
		/**************************************/
		/* Determine whether FIFO is blocked. */
		/**************************************/	
		if( ((aCell->multicast == UCAST ) && 
		     (aSwitch->inputBuffer[input]->fifo[pri]->number > 
		      actionState->maxCellsPerFIFO)) ||
		    ((aCell->multicast == MCAST ) &&    
		     (aSwitch->inputBuffer[input]->mcastFifo[priority]->number > 
		      actionState->maxCellsPerFIFO))) 
		  {
		    /******************************/
		    /* No room for cell. Drop it. */
		    /******************************/
		    aSwitch->inputBuffer[input]->numOverflows++;
		    destroyCell(aCell);
		    return (Cell*)0;
		  }
		
		/**********************************************/
		/* Determine whether input buffer is blocked. */
		/**********************************************/
		numFIFOs = aSwitch->numOutputs * aSwitch->numPriorities;
		for(pri=0; pri< numFIFOs; pri++)
		  {
		    inputBufferOccupancy += 
		      aSwitch->inputBuffer[input]->fifo[pri]->number;
		  }
		for(pri=0; pri< aSwitch->numPriorities; pri++)
		  inputBufferOccupancy += aSwitch->inputBuffer[input]->mcastFifo[pri]->number;
		if(inputBufferOccupancy > actionState->maxCellsPerInputBuffer)
		  {
		    /******************************/
		    /* No room for cell. Drop it. */
		    /******************************/
		    aSwitch->inputBuffer[input]->numOverflows++;
		    destroyCell(aCell);
		    return (Cell*)0;
		  }
	      }
	    
	    
	    /*****************************************/
	    /* Input buffer has room: cell accepted. */
	    /*****************************************/
	    anElement=createElement(aCell);
	    
	    /* VCI not implemented yet. VCI = output port. */
	    pri = aCell->vci * aSwitch->numPriorities + aCell->priority;
	    	    
	    /* Mark which input port cell arrived on */
	    aCell->commonStats.inputPort = input;
	    
	    /* Add cell to input fifo. */
	    if(aCell->multicast == UCAST ) 
	      addElement(aSwitch->inputBuffer[input]->fifo[pri], anElement);
	    if(aCell->multicast == MCAST ) 
	      addElement(aSwitch->inputBuffer[input]->mcastFifo[aCell->priority], anElement);
	    
	    /* Update latency statistics for cell arrival at switch. */
	    latencyStats(LATENCY_STATS_ARRIVAL_TIME, NULL, aCell); 
	    switchStats(SWITCH_STATS_NEW_ARRIVAL, aSwitch, input);
	    
	    break;
	  }
	  
	case INPUTACTION_TRANSMIT:
	  {
	    struct List *fromFifo;
	    
	    /* Remove next cell for given output and return pointer to cell */
	    priority = aCell->priority;
	    if( aCell->multicast == UCAST )
	      {
		pri = priority + aSwitch->numPriorities * output;
		fromFifo = aSwitch->inputBuffer[input]->fifo[pri];
		anElement = removeElement(fromFifo);
		aCell = (Cell *) anElement->Object;
		destroyElement(anElement);
	      }
	    else 
	      {
		
		fromFifo = aSwitch->inputBuffer[input]->mcastFifo[priority];
		/* Reset bit in bitmap */
		bitmapResetBit(output, &aCell->outputs);
		if( bitmapAnyBitSet(&aCell->outputs))
		  {
		    anElement = fromFifo->head;
		    aCell = (Cell *) anElement->Object;
		    aCell = copyCell(aCell);
		  }
		else
		  {
		    anElement = removeElement(fromFifo);
		    aCell = (Cell *) anElement->Object;
		    destroyElement(anElement);
		  }
		
	      }
	    return(aCell);
	    
	    break;
	  }
        case INPUTACTION_STATS_PRINT:
        {
	    int numArrivals=0;
	    int allOutputsEnabled=1;
            struct List *fifo;

	    actionState =
	      (struct inputActionState *) aSwitch->inputActionState;
               
            if( (actionState->maxCellsPerFIFO != NONE) || 
                (actionState->maxCellsPerInputBuffer != NONE)  )
            {
	        printf("\n");
	        printf("  OVERFLOWS AT EACH INPUT BUFFER:\n");
	        printf("  -------------------------------\n");
	        printf("  I/P    Num  Arrivals\n");
	        printf("  ---------------------\n");
	        for(input=0; input<aSwitch->numInputs; input++)
	                {
	                /* Number of arrivals to an input queue is the sum
	                across all output fifos for that input. */
	                /* The count of cells by each "list" excludes overflows. */
	                /* The number of arrivals to a queue =      */
	                /*	numOverflows + arrivalStat->number      */
         
	                numFIFOs = aSwitch->numOutputs * aSwitch->numPriorities;
	                for(output=0; output<numFIFOs; output++ )
	                {
		                fifo = aSwitch->inputBuffer[input]->fifo[output];
		                if( !LIST_STAT_IS_ENABLED(fifo, LIST_STATS_ARRIVALS) )
		                allOutputsEnabled=0;
		                numArrivals += 
		                returnNumberStat(&fifo->listStats[LIST_STATS_ARRIVALS]);
	                }
	                numArrivals += aSwitch->inputBuffer[input]->numOverflows;
														                	
	                if( numArrivals  &&  allOutputsEnabled )
	                        printf("  %3d  %8d  (%g%%)\n", input, 
		                aSwitch->inputBuffer[input]->numOverflows,
		                100.0 * 
		                (double)aSwitch->inputBuffer[input]->numOverflows /
		                (double)numArrivals);
	                else 
                                printf("  %3d  %8d \n", input, 
		                aSwitch->inputBuffer[input]->numOverflows);
	                }
             }
        }
	default:
	  break;
	}
	return (Cell*)0;

}
