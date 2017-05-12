
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

/* MY T LE */
/* HEADER FILE */

#define RC_VALUE 1.0
#define GAIN_BW 0.02
#define A 100.0
#define B 100.0
#define C 40.0
#define DEFAULT_THRESHOLD 1e-6
#define DEFAULT_STEPSIZE 1E-4

typedef enum {
		INIT_ALGORITHM_STATS,
		UPDATE_ALGORITHM_ITERATIONS_STATS,
		UPDATE_ALGORITHM_BADCHOICE_STATS,
		PRINT_ALGORITHM_ITERATIONS_STATS,
		PRINT_ALGORITHM_BADCHOICE_STATS
} AlgorithmStatsAction;

struct AlgorithmStats {
	long sumBadChoice, numBadChoice;
	long sumIterations, numIterations;
};
