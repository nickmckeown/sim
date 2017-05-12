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
#include "rr.h"
#include "scheduleStats.h"

void countSynch(aSwitch)
  Switch *aSwitch;
{
  int *numSchedulers;
  int numSynch=0;
  int output, numFabricOutputs;
  SchedulerState  *scheduleState = 
    (SchedulerState *) aSwitch->scheduler.schedulingState;
  OutputSchedulerState    *outputSchedule;
  /*****************************************************************/
  /* Update stats for the number of synchronized output schedulers */
  /*****************************************************************/
  numSchedulers = (int *) malloc(sizeof(int) * aSwitch->numInputs);
  memset(numSchedulers, 0, sizeof(int) * aSwitch->numInputs);
  numFabricOutputs=aSwitch->numOutputs*aSwitch->fabric.Xbar_numOutputLines;
  for(output=0; output<numFabricOutputs; output++)
    {
      outputSchedule = scheduleState->outputSched[output];
      if( numSchedulers[outputSchedule->last_accepted_grant] == 1)
	numSynch++;
      numSchedulers[outputSchedule->last_accepted_grant]++;
      if( numSchedulers[outputSchedule->last_accepted_grant] > 1)
	numSynch++;
    }
  free(numSchedulers);
  scheduleStats(SCHEDULE_STATS_NUM_SYNC, aSwitch, &numSynch);
  /*****************************************************************/
}
