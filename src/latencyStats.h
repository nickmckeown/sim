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

#ifndef _LATENCY_STATS_H
#define _LATENCY_STATS_H

typedef enum {
	LATENCY_STATS_CELL_INIT,
	LATENCY_STATS_SWITCH_INIT,
	LATENCY_STATS_ARRIVAL_TIME,
  LATENCY_STATS_DEQUEUE_TIME,
	LATENCY_STATS_ARRIVE_FABRIC,
	LATENCY_STATS_ARRIVE_OUTPUT,
	LATENCY_STATS_SWITCH_UPDATE,
	LATENCY_STATS_CELL_UPDATE,
	LATENCY_STATS_RETURN_AVG,
	LATENCY_STATS_SWITCH_PRINT,
	LATENCY_STATS_CELL_PRINT,
} LatencyStatsCommand;

double latencyStats(LatencyStatsCommand mode, Switch *aSwitch, Cell *aCell);
#endif
