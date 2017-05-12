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

#include <stdio.h>
#include <stdlib.h>
#include "lists.h"
#include <string.h>

#ifdef FREELIST
#include "circBuffer.h"
#endif

#if defined(LIST_STATS) || defined(LIST_HISTOGRAM)
extern long now;
#endif

/** static char ident[] = "@(#) Nick McKeown's List handling routines Rev
  3.0"; **/

/**************************************************/
/* Miscellaneous, general purpose list functions. */
/* Author: Nick McKeown                           */
/**************************************************/

#define LISTDEBUG
#ifdef LISTDEBUG
int listDebug=1;
#else
int listDebug=0;
#endif

#ifdef FREELIST
static struct Ring *freeListList = NULL ;
static struct Ring *freeElementList = NULL ;
#endif

/*****************************************************************/
/***************************  createList() ***********************/
/*****************************************************************/
struct List * createList(char *name)
{
  struct List *aList;
  int type;

#ifdef FREELIST 
  aList = (struct List *) ringMalloc(&freeListList, sizeof(struct List));
#else
  aList = (struct List *) malloc( sizeof(struct List) );
#endif
  memset(aList, 0, sizeof(struct List));
  if( !aList )
    {
      perror("createList:");
      exit(1);
    }

  aList->name = (char *) malloc(sizeof(char)*(strlen(name)+1));
  strcpy(aList->name,name);
  aList->number = 0;
  aList->maxNumber = -1; /* No limit set. */
  aList->head = aList->tail = NULL;

#ifdef LIST_MINMAX
  aList->max = 0;
  aList->min = 0;
#endif // LIST_MINMAX

#ifdef LIST_STATS
  aList->listStats = (Stat *) calloc(NUM_LIST_STATS_TYPES, sizeof(Stat));
  for( type=0; type<NUM_LIST_STATS_TYPES; type++ )			
    disableListStats(aList, type);
#endif // LIST_STATS

#ifdef LIST_HISTOGRAM
  aList->listHistogram = (struct Histogram *) 
      calloc(NUM_LIST_HISTOGRAM_TYPES, sizeof(struct Histogram));
  for( type=0; type<NUM_LIST_HISTOGRAM_TYPES; type++ )			
    disableListHistogram(aList, type);
#endif // LIST_HISTOGRAM

  return( aList );
}

/*****************************************************************/
/***************************  createLiteList() *******************/
/*****************************************************************/
struct List * createLiteList(char *name)
{
  struct List *aList;

#ifdef FREELIST 
  aList = (struct List *) ringMalloc(&freeListList, sizeof(struct List));
#else
  aList = (struct List *) malloc( sizeof(struct List) );
#endif
  memset(aList, 0, sizeof(struct List));
  if( !aList )
    {
      perror("createList:");
      exit(1);
    }

  aList->name = (char *) malloc(sizeof(char)*(strlen(name)+1));
  strcpy(aList->name,name);
  aList->number = 0;
  aList->maxNumber = -1; /* No limit set. */
  aList->head = aList->tail = NULL;

#ifdef LIST_MINMAX
  aList->max = 0;
  aList->min = 0;
#endif // LIST_MINMAX

/* LiteLists don't have Stats and Histograms allocated */
#ifdef LIST_STATS
  aList->listStats = (Stat *) NULL;
#endif // LIST_STATS

#ifdef LIST_HISTOGRAM
  aList->listHistogram = (struct Histogram *) NULL;
#endif // LIST_HISTOGRAM

  return( aList );
}

/********************************************************************/
/***************************  createElement() ***********************/
/********************************************************************/
/* Create element that points to "object" */
struct Element *createElement(void *object)
{
  struct Element *anElement;

#ifdef FREELIST	
  anElement = (struct Element *) ringMalloc(&freeElementList, sizeof(struct Element));
#else
  anElement = (struct Element *) malloc( sizeof(struct Element) );
#endif
  if( !anElement )
    {
      perror("createElement:");
      return(NULL);
    }

  anElement->next = anElement->prev = NULL;	
  anElement->Obj.object = object;

#if defined(LIST_STATS) || defined(LIST_HISTOGRAM)
  anElement->arrivalTime = 0;
#endif

  return(anElement);
}

/*****************************************************************/
/***************************  addElement() ***********************/
/*****************************************************************/
/* Element is added to tail of list */
int addElement(struct List *aList, struct Element *anElement)
{
#ifdef LIST_STATS
  updateListStats(aList, LIST_STATS_ARRIVALS, aList->number, now);
  updateListStats(aList, LIST_STATS_TA_OCCUPANCY, aList->number, now);
#endif // LIST_STATS

#ifdef LIST_HISTOGRAM
  updateListHistogram(aList, LIST_HISTOGRAM_ARRIVALS, aList->number);
#endif // LIST_HISTOGRAM


#if defined(LIST_STATS) || defined(LIST_HISTOGRAM)
  /* Mark element as "arrived" at list */
  anElement->arrivalTime = now;
#endif

  if( aList->number == aList->maxNumber)
    return(LIST_ERROR_FULL);

  if( aList->number ) /* NOT EMPTY */
    {
      aList->tail->next = anElement;
      anElement->prev = aList->tail;
      anElement->next = NULL;
      aList->tail = anElement;
    }
  else
    {
      aList->head = anElement;
      aList->tail = anElement;
      anElement->next = NULL;
      anElement->prev = NULL;
    }


  aList->number++;

#ifdef LIST_MINMAX
  if(aList->number > aList->max)
    aList->max = aList->number;
#endif

  if(listDebug)
    if( !checkList(aList, "addElement") ) 
      exit(1);

  return(LIST_OK);

}

/***********************************************************************/
/***************************  addElementBefore() ***********************/
/***********************************************************************/
int addElementBefore(struct List *aList, struct Element *beforeElement, 
		     struct Element *anElement)
{

#ifdef LIST_STATS
  updateListStats(aList, LIST_STATS_ARRIVALS, aList->number, now);
  updateListStats(aList, LIST_STATS_TA_OCCUPANCY, aList->number, now);
#endif // LIST_STATS

#ifdef LIST_HISTOGRAM
  updateListHistogram(aList, LIST_HISTOGRAM_ARRIVALS, aList->number);
#endif // LIST_HISTOGRAM

#if defined(LIST_STATS) || defined(LIST_HISTOGRAM)
  /* Mark element as "arrived" at list */
  anElement->arrivalTime = now;
#endif

  if( aList->number == aList->maxNumber)
    return(LIST_ERROR_FULL);

  if( aList->number == 0 )
    {
      if(aList->name != NULL)
	fprintf(stderr, "addElementBefore: list %s is empty\n", aList->name);
      exit(1);
    }

  if( beforeElement == aList->head )
    {
      anElement->next = beforeElement;
      aList->head = anElement;
      anElement->prev = NULL;
      beforeElement->prev = anElement;
    }
  else
    {
      anElement->prev = beforeElement->prev;
      anElement->prev->next = anElement;
      anElement->next = beforeElement;
      beforeElement->prev = anElement;
    }


  aList->number++;
#ifdef LIST_MINMAX
  if(aList->number > aList->max)
    aList->max = aList->number;
#endif

  if(listDebug)	
    if( !checkList(aList, "addElementBefore") )
      exit(1);
  return(LIST_OK);
}

/**********************************************************************/
/***************************  addElementAfter() ***********************/
/**********************************************************************/
int addElementAfter(struct List *aList, struct Element *afterElement, 
                    struct Element *anElement)
{
#ifdef LIST_STATS
  updateListStats(aList, LIST_STATS_ARRIVALS, aList->number, now);
  updateListStats(aList, LIST_STATS_TA_OCCUPANCY, aList->number, now);
#endif // LIST_STATS

#ifdef LIST_HISTOGRAM
  updateListHistogram(aList, LIST_HISTOGRAM_ARRIVALS, aList->number);
#endif // LIST_HISTOGRAM

#if defined(LIST_STATS) || defined(LIST_HISTOGRAM)
  /* Mark element as "arrived" at list */
  anElement->arrivalTime = now;
#endif

  if( aList->number == aList->maxNumber)
    return(LIST_ERROR_FULL);

  if( aList->number == 0 )
    {
      if(aList->name != NULL)
	fprintf(stderr, "addElementAfter: list %s is empty\n", aList->name);
      exit(1);
    }
  if( afterElement == aList->tail )
    {
      anElement->next = NULL;
      aList->tail = anElement;
      anElement->prev = afterElement;
      afterElement->next = anElement;
    }
  else
    {
      anElement->prev = afterElement;
      anElement->next = afterElement->next;
      anElement->next->prev = anElement;
      afterElement->next = anElement;
    }

	
  aList->number++;
#ifdef LIST_MINMAX
  if(aList->number > aList->max)
    aList->max = aList->number;
#endif

  if(listDebug)	
    if( !checkList(aList, "addElementAfter") )
      exit(1);
  return(LIST_OK);
}

/***********************************************************************/
/***************************  addElementAtHead() ***********************/
/***********************************************************************/
int addElementAtHead(struct List *aList, struct Element *anElement)
{

  if( aList->number == 0 )
    {
      return( addElement( aList, anElement) );
    }
  else
    {
#ifdef LIST_STATS
      updateListStats(aList, LIST_STATS_ARRIVALS, aList->number, now);
  updateListStats(aList, LIST_STATS_TA_OCCUPANCY, aList->number, now);
#endif // LIST_STATS

#ifdef LIST_HISTOGRAM
      updateListHistogram(aList, LIST_HISTOGRAM_ARRIVALS, aList->number);
#endif // LIST_HISTOGRAM

#if defined(LIST_STATS) || defined(LIST_HISTOGRAM)
      /* Mark element as "arrived" at list */
      anElement->arrivalTime = now;
#endif

      if( aList->number == aList->maxNumber)
	return(LIST_ERROR_FULL);

      anElement->next = aList->head;
      anElement->next->prev = anElement;
      anElement->prev = NULL;
      aList->head = anElement;
      aList->number++;
    }
#ifdef LIST_MINMAX
  if(aList->number > aList->max)
    aList->max = aList->number;
#endif

  if(listDebug)	
    if( !checkList(aList, "addElementAtHead") ) 
      exit(1);
  return(LIST_OK);
}

/*****************************************************************/
/***************** removeElementFromTail() ***********************/
/*****************************************************************/
struct Element *removeElementFromTail(struct List *aList)
{
  struct Element *anElement;

  if(!aList->number)
    return(NULL);
  anElement = aList->tail;
  deleteElement( aList, anElement );
  return(anElement);
}


/*****************************************************************/
/*************************** moveElement() ***********************/
/*****************************************************************/
int moveElement(struct List *fromList, struct List *toList, 
                struct Element *anElement)
{
  if( toList->number == toList->maxNumber)
    return(LIST_ERROR_FULL);

  deleteElement( fromList, anElement );
  addElement( toList, anElement );

  return(LIST_OK);
}
	

/*******************************************************************/
/*************************** deleteElement() ***********************/
/*******************************************************************/
/* Delete element from its current position in List and return element */
struct Element *deleteElement(struct List *aList, struct Element *anElement)
{
#ifdef LIST_STATS
  updateListStats(aList, LIST_STATS_DEPARTURES, aList->number, now);
  updateListStats(aList, LIST_STATS_LATENCY, now - anElement->arrivalTime,
                  now);
  updateListStats(aList, LIST_STATS_TA_OCCUPANCY, aList->number, now);
#endif // LIST_STATS

#ifdef LIST_HISTOGRAM
  updateListHistogram(aList, LIST_HISTOGRAM_DEPARTURES, aList->number);
  updateListHistogram(aList, LIST_HISTOGRAM_LATENCY, 
		      now - anElement->arrivalTime);
#endif // LIST_HISTOGRAM

  if( aList->number == 0 )
    {
      if(aList->name != NULL)
	fprintf(stderr, "deleteElement: list %s is empty\n", aList->name);
      exit(1);
    }
  if( aList->number == 1 )
    {
      aList->head = aList->tail = NULL;
    }
  else
    {
      if(aList->head == anElement)
	{
	  aList->head = anElement->next;
	  aList->head->prev = NULL;
	}
      else if( aList->tail == anElement )
	{
	  aList->tail = anElement->prev;
	  aList->tail->next = NULL;
	}
      else
	{
	  anElement->prev->next = anElement->next;
	  anElement->next->prev = anElement->prev;
	}
    }

  aList->number--;
#ifdef LIST_MINMAX
  if(aList->number < aList->min)
    aList->min = aList->number;
#endif

  if(listDebug)	
    if( !checkList(aList, "deleteElement") )
      exit(1);

  return(anElement);
}

/***************************************************************/
/*************************** copyList() ****************************/
/***************************************************************/
/* Copies the contents of fromList to toList, creating a copy */
/* of each element on fromList */
struct List *copyList(struct List *fromList, struct List *toList)
{
  struct Element *anElement, *newElement;

  if( !fromList )
    return(NULL);

  if( !toList	)
    toList = createList("CopiedList");

  for(anElement=fromList->head; anElement; anElement=anElement->next)
    {
      newElement = createElement(anElement->Object);
      if( newElement == NULL )
	return(NULL);
      addElement(toList, newElement);
    }
  return(toList);
}

/********************************************************************/
/*************************** destroyList() ***********************/
/********************************************************************/
void destroyList(struct List *aList)
{
  int i;
  struct Element *anElement, *nextElement;

  if( aList->number )
    {
      for( anElement=aList->head; anElement; )
	{
	  nextElement = anElement->next;
	  deleteElement(aList, anElement);
	  destroyElement(anElement);
	  anElement = nextElement;
	}
    }

  if(aList->listStats)
  {
    free(aList->listStats);
  }

  if(aList->listHistogram)
  {
    for(i=0; i<NUM_LIST_HISTOGRAM_TYPES; i++)
        destroyHistogram(&(aList->listHistogram[i]));
  }

  free(aList);
}
/********************************************************************/
/*************************** destroyElement() ***********************/
/********************************************************************/
void destroyElement(struct Element *anElement)
{

#ifdef FREELIST
  ringDealloc(&freeElementList, anElement);
#else
  free(anElement);
#endif
}

/*******************************************************************/
/*************************** removeElement() ***********************/
/*******************************************************************/
/* Remove next element from HEAD of list */
struct Element *
removeElement(struct List *aList)
{
  struct Element *anElement=aList->head;

#ifdef LIST_STATS
  updateListStats(aList, LIST_STATS_DEPARTURES, aList->number, now);
  updateListStats(aList, LIST_STATS_LATENCY, now - anElement->arrivalTime,
                  now);
  updateListStats(aList, LIST_STATS_TA_OCCUPANCY, aList->number, now);
#endif // LIST_STATS

#ifdef LIST_HISTOGRAM
  updateListHistogram(aList, LIST_HISTOGRAM_DEPARTURES, aList->number);
  updateListHistogram(aList, LIST_HISTOGRAM_LATENCY, 
		      now - anElement->arrivalTime);
#endif // LIST_HISTOGRAM

  if( aList->number )
    {
      aList->head = aList->head->next;
      if( aList->head )
	aList->head->prev = NULL;
      if( --aList->number == 0 )
	aList->tail = NULL;
    }
  else
    {
      fprintf(stderr,"Tried to remove an element from an empty list.\n");
      if(aList->name != NULL)
	fprintf(stderr, "List: %s\n", aList->name);
      exit(1);
    }

#ifdef LIST_MINMAX
  if(aList->number < aList->min)
    aList->min = aList->number;
#endif
  if(listDebug)
    if( !checkList(aList, "removeElement") ) 
      exit(1);

  return(anElement);
}

/*******************************************************/
/****************** Object Routines ********************/
/*******************************************************/

/*************************************************/
/****************** addObject ********************/
/*************************************************/
int addObject(struct List *aList, void *object)
{
  struct Element *anElement = createElement(object);
  if( anElement == NULL )
    return(0);
  addElement(aList, anElement);
  return(1);
}

/***********************************************/
/****************** removeObject ***************/
/***********************************************/
void *
removeObject(struct List *aList)
{
  struct Element *anElement;
  void *object;
  anElement = removeElement(aList);

  if(!anElement)
    return((void *)NULL);

  object = anElement->Object;
  destroyElement(anElement);

  return(object);
}


/***********************************************/
/****************** deleteObject ***************/
/***********************************************/
void *
deleteObject(struct List *aList, void *object)
{
  struct Element *anElement;

  anElement = findObjectInList(aList, object);
  if(!anElement)
    return((void *)NULL);

  deleteElement(aList, anElement);
  destroyElement(anElement);
  return(object);
}


/***********************************************/
/************** findObjectInList ***************/
/***********************************************/
struct Element *
findObjectInList(struct List *aList, void *object)
{
  struct Element *anElement;

  EACH_ELEMENT(aList, anElement)
    {
      if(anElement->Object == object)
	return(anElement);
    }
  return((struct Element *) NULL);
}	

/**********************************************/
/***************** ObjectInList ***************/
/**********************************************/
int 
ObjectInList(struct List *aList, void *object)
{
  void *anObject;

  EVERY(anObject, aList)
    {
      if(anObject==object)
	return(1);
    }

  return(0);
}

/***************************************************************/
/*************************** checkList() ***********************/
/***************************************************************/
/* Check to see if correct number in list and links are OK */
/* Returns 1 if all OK */
int checkList(struct List *aList, char *string)
{
  int i;
  struct Element *anElement;

  /* Is list empty ? */
  if( !aList->number && aList->head==NULL && aList->tail==NULL )
    return(1);

  /* Is head entry's prev == NULL ? */
  if( aList->head->prev != NULL )
    {
      printf("%s: List %s head->prev != NULL\n", string, aList->name);
      printList( stderr, aList );
      return(0);
    }

  /* Is tail entry's next == NULL ? */
  if( aList->tail->next != NULL )
    {
      printf("%s: List %s tail->next != NULL\n", string, aList->name);
      printList( stderr, aList );
      return(0);
    }

  /* Is there a head ? */
  if( aList->number && !aList->head )
    {
      printf("%s: List %s has %d entries but no head!\n", string, aList->name, aList->number);
      printList( stderr, aList );
      return(0);
    }

  /* Is there a tail ? */
  if( aList->number && !aList->tail )
    {
      printf("%s: List %s has %d entries but no tail!\n", string, aList->name, aList->number);
      printList( stderr, aList );
      return(0);
    }

  /* Check to see if NULL entry in next pointers before we reach the tail */
  anElement = aList->head;
  for(i=1; i<aList->number; i++ )
    {
      if( anElement->next == NULL )
	{
	  printf("%s: List %s has NULL entry at entry %d before tail.!\n", string, aList->name, i);
	  printList( stderr, aList );
	  return(0);
	}
      anElement = anElement->next;
    }

  /* Check to see if NULL entry in prev pointers before we reach the head */
  anElement = aList->tail;
  for(i=1; i<aList->number; i++ )
    {
      if( anElement->prev == NULL )
	{
	  printf("%s: List %s has NULL entry at entry %d before head.!\n", string, aList->name, i);
	  printList( stderr, aList );
	  return(0);
	}
      anElement = anElement->prev;
    }

  return(1);
	
}

/***************************************************************/
/*************************** writeList() ***********************/
/***************************************************************/
/* Writes list objects to a file */
void writeList(FILE *fp, struct List *aList, int objSize)
{
  struct Element *anElement = aList->head;

#ifdef LIST_HISTOGRAM
  int histogram;
#endif

  if( fwrite(&aList->number, sizeof(int), 1, fp) != 1)
    {
      fprintf(stderr, "Couldn't write list \"%s\" to file\n", 
	      aList->name);
      exit(1);
    }
  if( fwrite(&aList->maxNumber, sizeof(int), 1, fp) != 1)
    {
      fprintf(stderr, "Couldn't write list \"%s\" to file\n", 
	      aList->name);
      exit(1);
    }
  while(anElement)
    {
      if( fwrite(anElement->Object, objSize, 1, fp) != 1)
	{
	  fprintf(stderr, "Couldn't write list \"%s\" to file\n", 
		  aList->name);
	  exit(1);
	}
      anElement = anElement->next;
    }
#ifdef LIST_MINMAX
  if( fwrite(aList->max, sizeof(int), 1, fp) != 1)
    {
      fprintf(stderr, "Couldn't write list \"%s\" to file\n", 
	      aList->name);
      exit(1);
    }
  if( fwrite(aList->min, sizeof(int), 1, fp) != 1)
    {
      fprintf(stderr, "Couldn't write list \"%s\" to file\n", 
	      aList->name);
      exit(1);
    }
#endif

#ifdef LIST_STATS
  if( fwrite(aList->listStats, sizeof(Stat), NUM_LIST_STATS_TYPES, fp) 
      != NUM_LIST_STATS_TYPES)
    {
      fprintf(stderr, "Couldn't write list \"%s\" to file\n", 
	      aList->name);
      exit(1);
    }
#endif

#ifdef LIST_HISTOGRAM
  for( histogram = 0; histogram<NUM_LIST_HISTOGRAM_TYPES; histogram++)
    writeHistogram(fp, &aList->listHistogram[histogram] );
#endif
	
}
/**************************************************************/
/*************************** readList() ***********************/
/**************************************************************/
/* Reads list objects from a file */
void readList(FILE *fp, struct List *aList, int objSize)
{
  struct Element *anElement;
  char *object;	
  int i;

#ifdef LIST_HISTOGRAM
  int histogram;
#endif

  if( fread(&aList->number, sizeof(int), 1, fp) != 1)
    {
      fprintf(stderr, "Couldn't read list \"%s\" from file\n", 
	      aList->name);
      exit(1);
    }
  if( fread(&aList->maxNumber, sizeof(int), 1, fp) != 1)
    {
      fprintf(stderr, "Couldn't read list \"%s\" from file\n", 
	      aList->name);
      exit(1);
    }
  for( i=0; i<aList->number; i++)
    {
      object = (char *) malloc(objSize);
      anElement = createElement(object);
      if( fread(anElement->Object, objSize, 1, fp) != 1)
	{
	  fprintf(stderr, "Couldn't read list \"%s\" from file\n", 
		  aList->name);
	  exit(1);
	}
      addElement(aList, anElement);
    }

#ifdef LIST_MINMAX
  if( fread(&aList->max, sizeof(int), 1, fp) != 1)
    {
      fprintf(stderr, "Couldn't read list \"%s\" from file\n", 
	      aList->name);
      exit(1);
    }
  if( fread(&aList->min, sizeof(int), 1, fp) != 1)
    {
      fprintf(stderr, "Couldn't read list \"%s\" from file\n", 
	      aList->name);
      exit(1);
    }
#endif


#ifdef LIST_STATS
  if( fread(aList->listStats, sizeof(Stat), NUM_LIST_STATS_TYPES, fp) 
      != NUM_LIST_STATS_TYPES)
    {
      fprintf(stderr, "Couldn't read list \"%s\" from file\n", 
	      aList->name);
      exit(1);
    }
#endif

#ifdef LIST_HISTOGRAM
  for( histogram = 0; histogram<NUM_LIST_HISTOGRAM_TYPES; histogram++)
    readHistogram(fp, &aList->listHistogram[histogram] );
#endif
	
}
/***************************************************************/
/*************************** printList() ***********************/
/***************************************************************/
void printList(FILE *fp, struct List *aList)
{
  struct Element *anElement;

  fprintf(fp, "	List Name: %s\n", aList->name);
  fprintf(fp, "	Number of elements: %d\n", aList->number);
  if( aList->maxNumber >= 0 )
    fprintf(fp, "	Max num elements: %d\n", aList->maxNumber);
  fprintf(fp, "	Head: 0x%x   Tail: 0x%x\n", 
	  (unsigned int) aList->head, 
	  (unsigned int) aList->tail);

  anElement = aList->head;

  fprintf(fp, "	Element       Next          Prev\n");	
  while(anElement)
    {
      fprintf(fp, "	0x%8.8x    0x%-8x    0x%-8x\n", 
	      (unsigned int) anElement,
	      (unsigned int) anElement->next, 
	      (unsigned int) anElement->prev);
      anElement = anElement->next;
    }
  fprintf(fp, "\n");
}


/**************************************************************/
/*********************  STATS ROUTINES ************************/
/**************************************************************/
#ifdef LIST_STATS
void
resetListStats(struct List *aList, ListStatsType listStatsType, 
               unsigned long unow)
{
  if( listStatsType == LIST_STATS_TA_OCCUPANCY )
    initStat(&aList->listStats[listStatsType], STAT_TYPE_TIME_AVERAGE, unow);
  else
    initStat(&aList->listStats[listStatsType], STAT_TYPE_AVERAGE, unow);
}

void
enableListStats(struct List *aList, ListStatsType listStatsType)
{
  enableStat(&aList->listStats[listStatsType]);
}

void
disableListStats(struct List *aList, ListStatsType listStatsType)
{
  disableStat(&aList->listStats[listStatsType]);
}

void
updateListStats(struct List *aList, ListStatsType listStatsType,
                long aValue, unsigned long unow)
{
  if(!aList->listStats) return;
  if( LIST_STAT_IS_ENABLED(aList, listStatsType) )
    updateStat(&aList->listStats[listStatsType], aValue, unow);
  return;
}

void
printListStats(FILE *fp, struct List *aList, ListStatsType listStatsType)
{
  fprintf(fp, "-------------------------------\n");
  fprintf(fp, "Stats for list: %s\n", aList->name);
  fprintf(fp, "-------------------------------\n");
  switch( listStatsType )
    {
    case LIST_STATS_ARRIVALS:
      fprintf(fp, "Number of Arrivals: %ld\n", 
	      aList->listStats[listStatsType].number);
      fprintf(fp, "Average Number of elements seen by arrivals:      %g\n", 
	      returnAvgStat(&aList->listStats[listStatsType]));
      fprintf(fp, "AvgSquare of Number of elements seen by arrivals: %g\n", 
	      returnEX2Stat(&aList->listStats[listStatsType]));
      break;
    case LIST_STATS_DEPARTURES:
      fprintf(fp, "Number of Departures: %ld\n", 
	      aList->listStats[listStatsType].number);
      fprintf(fp, "Average Number of elements prior to departures:      %g\n", 
	      returnAvgStat(&aList->listStats[listStatsType]));
      fprintf(fp, "AvgSquare of Number of elements prior to departures: %g\n", 
	      returnEX2Stat(&aList->listStats[listStatsType]));
      break;
    case LIST_STATS_LATENCY:
      fprintf(fp, "Average Latency:                         %g\n", 
	      returnAvgStat(&aList->listStats[listStatsType]));
      fprintf(fp, "AvgSquare of Latency:                    %g\n", 
	      returnEX2Stat(&aList->listStats[listStatsType]));
      break;
    default:
      fprintf(fp, "Unknown LIST_STATS_TYPE: %d\n", listStatsType);
      break;
    }
}
#endif

/******************************************************************/
/*********************  HISTOGRAM ROUTINES ************************/
/******************************************************************/
#ifdef LIST_HISTOGRAM
void
initListHistogram(struct List *aList, char *histogramTitle,
                  ListHistogramType listHistogramType,
                  HistogramType histogramType,  		 /* HISTOGRAM_STATIC, HISTOGRAM_DYNAMIC */
                  HistogramStepType histogramStepType, /* HISTOGRAM_STEP_LINEAR, _LOG */
                  int histogramNumBins,
                  double histogramMinBin, double histogramBinStepSize)
{

  initHistogram( histogramTitle,
		 &aList->listHistogram[listHistogramType], histogramType, 	
		 HISTOGRAM_INTEGER_LIMIT, histogramStepType, histogramNumBins, 
		 histogramMinBin, histogramBinStepSize,
		 0, (void (*)()) NULL);
}

void
resetListHistogram(struct List *aList, ListHistogramType listHistogramType)
{
  resetHistogram(&aList->listHistogram[listHistogramType]);
}

void
enableListHistogram(struct List *aList, ListHistogramType listHistogramType)
{
  aList->listHistogram[listHistogramType].enable = HISTOGRAM_ENABLE; 
}

void
disableListHistogram(struct List *aList, ListHistogramType listHistogramType)
{
  aList->listHistogram[listHistogramType].enable = HISTOGRAM_DISABLE;
}

void
updateListHistogram(struct List *aList, ListHistogramType listHistogramType,
                    long aValue)
{
  union StatsValue statsValue;

  if(!aList->listHistogram) return;

  if( LIST_HISTOGRAM_IS_ENABLED(aList, listHistogramType) )
    {
      statsValue.aDouble = 0.0;
      statsValue.anInt = (int) aValue;
      updateHistogram(&aList->listHistogram[listHistogramType], 
		      statsValue, 0);
    }
}

void
printListHistogram(FILE *fp, struct List *aList, 
                   ListHistogramType listHistogramType)
{
  fprintf(fp, "----------------------------------------------------------\n");
  fprintf(fp, "Histogram of \"%s\" for list: %s\n", 
	  aList->listHistogram[listHistogramType].name, aList->name);
  fprintf(fp, "----------------------------------------------------------\n");
  switch( listHistogramType )
    {
    case LIST_HISTOGRAM_ARRIVALS:
      fprintf(fp, "As seen by ARRIVALS:\n");
      break;
    case LIST_HISTOGRAM_DEPARTURES:
      fprintf(fp, "As seen by DEPARTURES:\n");
      break;
    case LIST_HISTOGRAM_TIME:
      fprintf(fp, "Over TIME:\n");
      break;
    case LIST_HISTOGRAM_LATENCY:
      fprintf(fp, "Cell Latency:\n");
      break;
    default:
      fprintf(fp, "Unknown LIST_HISTOGRAM_TYPE: %d\n", listHistogramType);
      exit(1);
    }
  printHistogram(fp, &aList->listHistogram[listHistogramType]);
}
#endif
