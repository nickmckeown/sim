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


/* #define MAXSTRING 256  */
#define MAXSTRING 512

#ifdef LIST_STATS
#ifndef INCLUDED_STAT
#include "stat.h"
#endif
#endif

#ifdef LIST_HISTOGRAM
#include "histogram.h"
#endif

/* Basic elements of an ELEMENT */
struct Element {
	struct Element *next;
	struct Element *prev;
	union {
		void *object;			/* Object that this points to */
		int	value;
	} Obj;
#if defined(LIST_STATS) || defined(LIST_HISTOGRAM)
	long arrivalTime;
#endif
#define Object Obj.object
#define Value  Obj.value
};

#ifdef LIST_STATS
typedef enum {
	LIST_STATS_ARRIVALS,
	LIST_STATS_DEPARTURES,
	LIST_STATS_LATENCY,
	LIST_STATS_TA_OCCUPANCY,
#define NUM_LIST_STATS_TYPES 4
} ListStatsType;

#define LIST_STAT_IS_ENABLED(aList, listStatsType) \
            (aList->listStats[listStatsType].enable)
#endif

#ifdef LIST_HISTOGRAM
typedef enum {
	LIST_HISTOGRAM_ARRIVALS,
	LIST_HISTOGRAM_DEPARTURES,
	LIST_HISTOGRAM_LATENCY,
	LIST_HISTOGRAM_TIME,
#define NUM_LIST_HISTOGRAM_TYPES 4
} ListHistogramType;

typedef enum {
	LIST_HISTOGRAM_NO_CREATE = 0,
	LIST_HISTOGRAM_CREATE = 1,
} ListHistogramCreateFlag;

#define LIST_HISTOGRAM_IS_ENABLED(aList, listHistogramType) \
            (aList->listHistogram[listHistogramType].enable)
#endif

struct List {
	struct Element *next;  /* Next list in a list of lists */
	struct Element *prev;  /* Prev list in a list of lists */

	char *name;
	int number;
	struct Element *head;
	struct Element *tail;

	int maxNumber;  		/* Max number allowed in list. */	
							/* if < 0, no limit set. */

#ifdef LIST_STATS
	/* Stats for arrivals, departures and over time. */
	/* Stat listStats[NUM_LIST_STATS_TYPES]; */
	Stat *listStats; 
#endif

#ifdef LIST_HISTOGRAM
	/* Histogram for arrivals, departures and over time. */
	struct Histogram *listHistogram; 
#endif

};

/*******************************************************/
/****************** LIST MACROS ************************/
/*******************************************************/
#define LIST_EMPTY(a) (!a->number)
#define EACH_ELEMENT(a,b) for(b=a->head; b; b=b->next)

struct Element *_e;
#define EVERY(a,b) for(_e=b->head, a=_e->Object;_e;_e=_e->next, a=_e->Object)

/* Returns from calls to routines that add to a list. */
#define LIST_OK 0
#define LIST_ERROR_FULL 1

extern struct List * createList(char *name);
extern struct List * createLiteList(char *name);
extern struct Element *createElement(void *object);
extern int addElement(struct List *aList, struct Element *anElement);
extern int addElementBefore(struct List *aList, struct Element *beforeElement, 
                            struct Element *anElement);
extern int addElementAfter(struct List *aList, struct Element *afterElement, 
                    struct Element *anElement);
extern int addElementAtHead(struct List *aList, struct Element *anElement);
extern struct Element *removeElementFromTail(struct List *aList);
extern int moveElement(struct List *fromList, struct List *toList, 
                       struct Element *anElement);
extern struct Element *deleteElement(struct List *aList, 
                                     struct Element *anElement);
extern struct List *copyList(struct List *fromList, struct List *toList);
extern void destroyList(struct List *aList);
extern void destroyElement(struct Element *anElement);
extern struct Element *removeElement(struct List *aList);
extern int addObject(struct List *aList, void *object);
extern void *removeObject(struct List *aList);
extern void *deleteObject(struct List *aList, void *object);
extern struct Element *findObjectInList(struct List *aList, void *object);
extern int ObjectInList(struct List *aList, void *object);
extern int checkList(struct List *aList, char *string);
extern void writeList(FILE *fp, struct List *aList, int objSize);
extern void readList(FILE *fp, struct List *aList, int objSize);
extern void printList(FILE *fp, struct List *aList);

#ifdef LIST_STATS
extern void resetListStats(struct List *aList, ListStatsType listStatsType, 
                           unsigned long now);
extern void enableListStats(struct List *aList, ListStatsType listStatsType);
extern void disableListStats(struct List *aList, ListStatsType listStatsType);
extern void updateListStats(struct List *aList, ListStatsType listStatsType,
                            long aValue, unsigned long now);
extern void printListStats(FILE *fp, struct List *aList, 
                           ListStatsType listStatsType);
#endif // LIST_STATS

#ifdef LIST_HISTOGRAM
extern void initListHistogram(struct List *aList, char *histogramTitle,
                              ListHistogramType listHistogramType,
                              HistogramType histogramType,  		 /* HISTOGRAM_STATIC, HISTOGRAM_DYNAMIC */
                              HistogramStepType histogramStepType, /* HISTOGRAM_STEP_LINEAR, _LOG */
                              int histogramNumBins,
                              double histogramMinBin, 
                              double histogramBinStepSize);

extern void resetListHistogram(struct List *aList, ListHistogramType
                               listHistogramType);


extern void enableListHistogram(struct List *aList, 
                                ListHistogramType listHistogramType);
extern void disableListHistogram(struct List *aList, 
                                 ListHistogramType listHistogramType);

extern void updateListHistogram(struct List *aList, 
                                ListHistogramType listHistogramType,
                                long aValue);

extern void printListHistogram(FILE *fp, struct List *aList, 
                        ListHistogramType listHistogramType);

#endif // LIST_HISTOGRAM
