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
#include "outputAction.h"

extern int burstStats( BurstStatsCommand mode, Switch *aSwitch, 
		       int output,
		       int input );

/* Take next cell from each output buffer and destroy. */
int defaultOutputAction(cmd, aSwitch, argc, argv)
  OutputActionCmd cmd;
Switch *aSwitch;
int argc;
char **argv;
{
  Cell *aCell;
  int output;
  struct Element *anElement;
  struct List *fromFifo;

  switch(cmd)
    {
    case OUTPUTACTION_USAGE:
      break;
    case OUTPUTACTION_INIT:
		if (aSwitch->numPriorities != 1) {
        	fprintf(stderr,"defaultOuputAction, number of priorities: %d is not supported by this outputAction.  Please use just one priority.\n",aSwitch->numPriorities);
			exit(1);
        }
      break;
    case OUTPUTACTION_EXEC:
      for(output=0; output<aSwitch->numOutputs; output++)
	{
	  fromFifo = aSwitch->outputBuffer[output]->fifo[DEFAULT_PRIORITY];    /* TL */

	  /* If the output queue has a cell, remove it. */
	  if( fromFifo->number )
	    {
	      anElement = removeElement(fromFifo);
	      aCell = anElement->Object;

			
	      /* This switch has finished with this cell.    */
	      /* If any, remove the switch dependent header. */
	      if( aCell->switchDependentHeader )
		{
		  free(aCell->switchDependentHeader);
		  aCell->switchDependentHeader = NULL;
		}

	      /* Update burstiness stats for this output. */
	      burstStats(BURST_STATS_UPDATE, aSwitch, 
			 output, aCell->commonStats.inputPort);

	      /* Update latency statistics for cell and switch. */
	      latencyStats(LATENCY_STATS_SWITCH_UPDATE, aSwitch, aCell);
	      latencyStats(LATENCY_STATS_CELL_UPDATE, NULL, aCell);

	      destroyElement(anElement);
	      destroyCell(aCell);
	    }
	  else
	    {
	      /* Update burstiness stats for this output. */
	      burstStats(BURST_STATS_UPDATE, aSwitch, 
			 output, NONE);
	    }
	}
      break;
    default:
      fprintf(stderr, "Illegal OutputAction cmd: %d\n", cmd);
      exit(1);
      break;
    }
  return (0);
}

