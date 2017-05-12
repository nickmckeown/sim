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


extern void printCell(FILE*, Cell*);

int 
outputQueued(action, aSwitch, argc, argv)
  FabricAction action;
Switch *aSwitch;
int argc;
char **argv;
{
  /* 
   * Drain every input queue and deliver cells to output.
   */
  Cell *aCell;
  Cell *outCell;

  if(debug_fabric)
    printf("Fabric 'outputQueued()' called by switch %d\n",aSwitch->switchNumber);

  switch(action)
    {
    case FABRIC_USAGE:
      fprintf(stderr, "No options for \"outputQueued\" fabric.\n");
      break;
    case FABRIC_INIT:
      {
	if( debug_fabric )
	  printf("     Fabric: INIT outputQueued \n");
	break;
      }
    case FABRIC_EXEC:
      {
	struct List  *toFifo;
        InputBuffer *inputBuffer;
	struct Element *anElement;
	int input, output, out, priority;

	if( debug_fabric )
	  printf("     Fabric: EXEC outputQueued \n");

	for(input=0; input<aSwitch->numInputs; input++)
        {
                inputBuffer = aSwitch->inputBuffer[input];
                /* Transfer all MCAST cells to outputs */
                for(priority=0;priority<aSwitch->numPriorities;priority++)
	        {
                    while(inputBuffer->mcastFifo[priority]->number)
                    {
	                anElement = inputBuffer->mcastFifo[priority]->head;
                        aCell = (Cell *) anElement->Object;
                        for(output=0; output<aSwitch->numOutputs; output++)
                        {
                                if( bitmapIsBitSet(output, &aCell->outputs))
                                {
		                        outCell = (aSwitch->inputAction)(INPUTACTION_TRANSMIT, aSwitch, input, (Cell *)aCell, output);
                                        toFifo = aSwitch->outputBuffer[output]->fifo[priority];
                                        anElement = createElement(outCell);
                                        addElement(toFifo, anElement);
                                        if( debug_fabric )
                                                printf("     Fabric: Update stats\n");
                                        /* Update latency stats for this cell */
                                        latencyStats(LATENCY_STATS_ARRIVE_FABRIC, NULL, outCell);
                                        latencyStats(LATENCY_STATS_ARRIVE_OUTPUT, NULL, outCell);
                                        if(debug_fabric)
                                        {
                                                printf("Cell %lu from input %d to output %d priority %d at time %lu\n", outCell->commonStats.ID, input, output, priority, now);
                                                printCell(stdout, outCell);
                                        }
                                }
                        }
                    }
                }
                /* Transfer all UCAST cells to outputs */
	        for(output=0; output<aSwitch->numOutputs; output++)
                    for(priority=0;priority<aSwitch->numPriorities;priority++)
	            {
                            out = aSwitch->numPriorities*output + priority;
                            while(inputBuffer->fifo[out]->number)
                            {
	                        anElement = inputBuffer->fifo[out]->head;
                                aCell = (Cell *) anElement->Object;
		                aCell = (aSwitch->inputAction)(INPUTACTION_TRANSMIT, aSwitch, input, (Cell *)aCell, output);
                                toFifo = aSwitch->outputBuffer[output]->fifo[priority];
                                anElement = createElement(aCell);
                                addElement(toFifo, anElement);
                                if( debug_fabric )
                                   printf("     Fabric: Update stats\n");
                                /* Update latency stats for this cell */
                                latencyStats(LATENCY_STATS_ARRIVE_FABRIC, NULL, aCell);
                                latencyStats(LATENCY_STATS_ARRIVE_OUTPUT, NULL, aCell);
                                if(debug_fabric)
                                {
                                        printf("Cell from input %d to output %d priority %d at time %lu\n", input, output, priority, now);
                                        printCell(stdout, aCell);
                                }
                             }
                    }

        }
	break;
      }
    case FABRIC_STATS_PRINT:
      {
	break;
      }
    case FABRIC_STATS_INIT:
      {
	break;
      }
    default:
      fprintf(stderr, "\nCommand not implemented in outputQueued fabric.c\n");
      exit(2);
    }
  if( debug_fabric )
    printf("     Fabric: Completed\n");

  return (0);
}

