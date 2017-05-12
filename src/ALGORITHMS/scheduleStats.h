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


extern void scheduleCellStats();
extern void scheduleStats();

typedef enum {
	SCHEDULE_CELL_STATS_INIT,
	SCHEDULE_CELL_STATS_HEAD_ARRIVAL,
	SCHEDULE_CELL_STATS_UPDATE_GRANT,
	SCHEDULE_CELL_STATS_UPDATE_ACCEPT,
	SCHEDULE_CELL_STATS_PRINT_GRANT,
	SCHEDULE_CELL_STATS_PRINT_ACCEPT,
	SCHEDULE_CELL_STATS_PRINT_ALL,
} ScheduleCellStatsMode;

typedef enum {
	SCHEDULE_STATS_INIT,
	SCHEDULE_STATS_PRINT_ALL,
	SCHEDULE_STATS_NUM_SYNC,
	SCHEDULE_STATS_PRINT_NUM_SYNC,
	SCHEDULE_STATS_NUM_ITERATIONS,
	SCHEDULE_STATS_PRINT_NUM_ITERATIONS,
} ScheduleStatsMode;

struct ScheduleInfo {
	long headArrivalTime;	/* Arrival time at head of input queue */
	long grantTime;			/* Time of first grant at current switch */
	long acceptTime;		/* Time of accept at current switch */
};
