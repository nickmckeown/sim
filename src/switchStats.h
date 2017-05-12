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


/* Default Histogram size for input buffers. */
#define INPUT_BUFFER_NUM_HISTOGRAM_BINS 20
#define INPUT_BUFFER_MIN_HISTOGRAM_BIN  (double)2.0
#define INPUT_BUFFER_HISTOGRAM_STEP     (double)2.0

/* Default Histogram size for output buffers. */
#define OUTPUT_BUFFER_NUM_HISTOGRAM_BINS 20
#define OUTPUT_BUFFER_MIN_HISTOGRAM_BIN  (double)2.0
#define OUTPUT_BUFFER_HISTOGRAM_STEP     (double)2.0

typedef enum {
	SWITCH_STATS_RESET,
	SWITCH_STATS_NEW_ARRIVAL,
	SWITCH_STATS_PER_CELL_UPDATE,
	SWITCH_STATS_PRINT_ALL,
} SwitchStatsCommand;

typedef enum {
	BURST_STATS_RESET,
	BURST_STATS_UPDATE,
	BURST_STATS_PRINT,
} BurstStatsCommand;

typedef struct {
	int lastValue;				/* Source/dest of last cell */
	int currentBurstLength;		 
	Stat stats;
} BurstStat;
