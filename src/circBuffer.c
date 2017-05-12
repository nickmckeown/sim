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
#include <string.h>
#include "circBuffer.h"

static int sizeAlloc=0;

struct Ring *
createRing(name, size)
  char *name;
int size;
{
  struct Ring *aRing;

  aRing = (struct Ring *) malloc(sizeof(struct Ring) + size*(sizeof(void *)));
  if( !aRing )
    {	
      perror("CreateRing");
      return(NULL);
    }

  strcpy(aRing->name, name);
  aRing->write = aRing->read = &aRing->data[size];
  aRing->number = 0;
  aRing->size = size;

#ifdef RINGDEBUG
  printf("createRing: \"%s\" created\n", aRing->name);
#endif
  return(aRing);
}

int
writeRing(aRing, value)
  struct Ring *aRing;
void *value;
{
  if(aRing->number == aRing->size)
    return((int)NULL);

  *(aRing->write--) = value;

  if(aRing->write == aRing->data)
    aRing->write = &aRing->data[aRing->size];

  aRing->number++;

#ifdef RINGDEBUG
  printf("writeRing: \"%s\", %d elements\n", aRing->name, aRing->number);
#endif

  return(1);
}

int
readRing(aRing, value)
  struct Ring *aRing;
  int **value;
{
  if(aRing->number == 0 )
    return((int)NULL);

  *value = *(aRing->read--);

  if(aRing->read == aRing->data)
    aRing->read = &aRing->data[aRing->size];

  aRing->number--;

#ifdef RINGDEBUG
  printf("readRing: \"%s\", %d elements, value 0x%lx\n", aRing->name, aRing->number, *value);
#endif

  return(1);
}

struct Ring *
changeRingSize(aRing, newsize)
  struct Ring *aRing;
int newsize;
{
  struct Ring *newRing;

  int *value;

  if( newsize < aRing->number )
    return(NULL);

  newRing = createRing(aRing->name, newsize);

  while(aRing->number)
    {
      readRing(aRing, &value);
      writeRing(newRing, value);
    }

  free(aRing);
#ifdef RINGDEBUG
  printf("changeRingSize: from %d to %d\n", aRing->size, newsize);
#endif
  return(newRing);
}

int
getAllocAmount()
{
  return(sizeAlloc);
}

void *
ringMalloc(freeList, size)
  struct Ring **freeList;
int size;				/* Size of memory chunks */
{
  char *aBlock;
  int i;
  char name[256];

  if(!*freeList)
    {		
      sprintf(name, "FreeList_%d_%d", size, FREELIST_BLOCKSIZE);
      *freeList = createRing(name, FREELIST_BLOCKSIZE);
      sizeAlloc += FREELIST_BLOCKSIZE * size;
    }
  if(!(*freeList)->number)
    {
      aBlock = (char *) malloc(FREELIST_BLOCKSIZE * size);
      if( !aBlock)
	{
	  perror("ringMalloc");
	  return(NULL);
	}
      for(i=0;i<FREELIST_BLOCKSIZE;i++)
	writeRing(*freeList, &aBlock[i*size]);
      sizeAlloc += FREELIST_BLOCKSIZE * size;
    }
  readRing(*freeList, (int **) &aBlock);
  return( (void *) aBlock);
}

void ringDealloc(freeList, anItem) 
  struct Ring **freeList;
void *anItem;
{
  if( writeRing(*freeList, anItem) == (int)NULL )
    {
      /* Free list was full, so increase size of list */
      *freeList = changeRingSize(*freeList, (*freeList)->size+FREELIST_BLOCKSIZE);
      writeRing(*freeList, anItem);
    }
}
