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

#ifndef __HISTOGRAMS__
#define __HISTOGRAMS__

union StatsValue {
	int anInt;
	double aDouble;
	void *aPtr;
};

typedef enum {
	STATS_INIT,
	STATS_UPDATE,
	STATS_PRINT,
} StatsCmd;

typedef enum {
	HISTOGRAM_DISABLE = 0,
	HISTOGRAM_ENABLE = 1,
} HistogramEnableFlag;

/* Integer histogram bins */
struct HistogramBin {
	double number;
	union {
		struct {
			double lower;
			double upper;
		} DoubleLimit;
		struct {
			int lower;
			int upper;
		} IntLimit;
		int intValue;
		double doubleValue;
		void *dataStructValue;
	} un;
#define doubleExactValue un.doubleValue
#define doubleLowerLimit un.DoubleLimit.lower
#define doubleUpperLimit un.DoubleLimit.upper
#define intExactValue un.intValue
#define intLowerLimit un.IntLimit.lower
#define intUpperLimit un.IntLimit.upper
#define structExactValue un.dataStructValue
};

typedef enum {
	HISTOGRAM_STATIC,
	HISTOGRAM_DYNAMIC,
} HistogramType;

typedef enum {
	HISTOGRAM_INTEGER_EXACT,
	HISTOGRAM_INTEGER_LIMIT,
	HISTOGRAM_DOUBLE_EXACT,
	HISTOGRAM_DOUBLE_LIMIT,
	HISTOGRAM_STRUCTURE_EXACT,
	HISTOGRAM_INTEGER_EXACT_INC, /* same as HISTOGRAM_INTEGER_EXACT */
	HISTOGRAM_INTEGER_EXACT_SUM, /* Increment bin by "amount" */
} HistogramValueType;

typedef enum {
	HISTOGRAM_STEP_LOG,
	HISTOGRAM_STEP_LINEAR,
} HistogramStepType;

struct Histogram {
	/* Name string. Used for debugging */
	char *name;
	/* Histogram type */
	HistogramType type;

	HistogramEnableFlag enable;     /* 0=> disabled. Else, enabled. */

	/* Histogram type of values */
	HistogramValueType valueType;

	/* Histogram type of steps (LOG or LIN) */
	HistogramStepType stepType;

	double minBin;		/* Minimum value bin (for HISTOGRAM_STATIC) */
	double stepSize; 	/* Stepsize (for HISTOGRAM_STATIC) */

	/* Number of bins */
	int number;
	/* Max Number of bins (for HISTOGRAM_DYNAMIC) */
	int maxNumber;
	/* Size of data structure held (for HISTOGRAM_STRUCTURE_EXACT) */
	int dataStructureSize;
	/* Function for printing histogram bin (for HISTOGRAM_STRUCTURE_EXACT) */
	void (*histogramPrintFunction)();
	/* Number of entries that did not correspond to a bin. */
	double noBin;
	struct HistogramBin *bin;
};
extern void initHistogram(char *name, struct Histogram *histogram, 
                          HistogramType type,
                          HistogramValueType valueType, 
                          HistogramStepType stepType,
                          int numBins, double minBin, double binStepSize, 
                          int dataStructureSize, 
                          void (*histogramPrintFunction)());

extern void destroyHistogram(struct Histogram *histogram);
extern void resetHistogram(struct Histogram *histogram);
extern void updateHistogram(struct Histogram *histogram, 
                            union StatsValue aValue,
                            double amount);
extern void enableHistogram(struct Histogram *histogram);
extern void disableHistogram(struct Histogram *histogram);
extern void writeHistogram(FILE *fp, struct Histogram *histogram);
extern void readHistogram(FILE *fp, struct Histogram *histogram);
extern void printHistogram(FILE *fp, struct Histogram *histogram);


#endif
