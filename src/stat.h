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

#define INCLUDED_STAT

typedef enum {
	STAT_TYPE_AVERAGE,
	STAT_TYPE_TIME_AVERAGE,
} StatType;

typedef struct {
	StatType type;
	int enable;
    unsigned long number;
    double sum;
    double sumSquares;
    unsigned long lastChangeTime;
} Stat;

void initStat(Stat *aStat, StatType type, unsigned long now);
void enableStat(Stat *aStat);
void disableStat(Stat *aStat);
void updateStat(Stat *aStat, long aValue, unsigned long now);
void printStat(FILE *fp, char *aString, Stat *aStat);
unsigned long returnNumberStat(Stat *aStat);
double returnAvgStat(Stat *aStat);
double returnEX2Stat(Stat *aStat);
