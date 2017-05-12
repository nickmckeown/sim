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

#define MAX_PRIORITY 10
long numCellsAllocd=0;
void *cellAlloc();
void cellFree();

#ifdef FREELIST
#include "circBuffer.h"
static struct Ring *cellFreeList=NULL;
#endif

void FatalError(); /* in sim.c */
void bitmapSetRandom(); /*  in bitmap.c */

Cell *
createCell(vci, mcastFlag, priority)
  int vci, mcastFlag, priority;
{
  Cell *cell;

#ifdef DEBUG_CELL
  cell = (Cell *) cellAlloc( sizeof(Cell) );
#else
#ifdef FREELIST
  cell = (Cell *) ringMalloc(&cellFreeList, sizeof(Cell));
#else
  cell = (Cell *) malloc( sizeof(Cell) );
#endif
#endif
  memset(cell, 0, sizeof(Cell));

  if( cell == NULL )
    FatalError("CreateCell(): malloc failed.\n");

  cell->vci = vci;

  if(priority <= MAX_PRIORITY)
    cell->priority = priority;   
  else
    cell->priority = 0;

  if( mcastFlag == MCAST )
    {
      cell->multicast = MCAST;
      bitmapReset(&cell->outputs);
    }
  else
    cell->multicast = UCAST;

  /* Initialize commonStats. */
  latencyStats(LATENCY_STATS_CELL_INIT, NULL, cell);

  cell->switchDependentHeader = (void *) NULL;
  cell->fabricStats    = (void *) NULL;
  cell->algorithmStats = (void *) NULL;

  if( debug_cell )
    printf("Created cell ID %ld at time %ld\n", cell->commonStats.ID, now);

  return(cell);
	
}

Cell *
createMulticastCell(mcastOutputs, fanout, seed, numOutputs, priority)
  Bitmap *mcastOutputs;
int fanout, numOutputs;
unsigned short *seed;
int priority;
{
  Cell *aCell;
  int output, num;
  
  aCell = createCell(0, MCAST, priority);
  if(bitmapAnyBitSet(mcastOutputs))
    {
      aCell->outputs = *mcastOutputs;
    }
  else
    {
      if( fanout != NONE )
	{
	  for(num=0; num<fanout;)
	    {
	      output=nrand48(seed)%(numOutputs);
	      if( !bitmapIsBitSet(output, &aCell->outputs) )
		{
		  bitmapSetBit(output, &aCell->outputs);
		  num++;
		}
	    }
	}
      else /* Select multicast U[0,2^N] */
	{
	  do {
	    bitmapSetRandom(&aCell->outputs, seed, numOutputs);
	  } while(!bitmapAnyBitSet(&aCell->outputs));
	}
    }
  return(aCell);
}

void printCell(fp,aCell)
  FILE *fp;
Cell *aCell;
{
  fprintf(fp, "========= Cell (0x%lx):==============\n", aCell->commonStats.ID);
  fprintf(fp, "   VCI: %u ", aCell->vci);
  fprintf(fp, "   TYPE: ");
  if( aCell->multicast == MCAST ) 
    {
      fprintf(fp, "mcast ");
      bitmapPrint(fp, &aCell->outputs,32);
    }
  else 
    fprintf(fp, "ucast\n");
  fprintf(fp, "   Priority: %u", aCell->priority);
  fprintf(fp, "   Created: %lu", aCell->commonStats.createTime);
  fprintf(fp, "   Switch Arr: %lu\n", aCell->commonStats.arrivalTime);
  fprintf(fp, "   Input Port: %u", aCell->commonStats.inputPort);
  fprintf(fp, "   Head   Arr: %lu\n", aCell->commonStats.headArrivalTime);
  fprintf(fp, "   Fabric Arr: %lu", aCell->commonStats.fabricArrivalTime);
  fprintf(fp, "   Output Arr: %lu\n", aCell->commonStats.outputArrivalTime);
  fprintf(fp, "   Num switches: %u\n",aCell->commonStats.numSwitchesVisited);
  fprintf(fp, "======================================\n");
	
}

Cell *
copyCell(aCell)
  Cell *aCell;
{
  Cell *newCell;
  newCell = createCell(0,0,0);
  memcpy(newCell, aCell, sizeof(Cell));
  return(newCell);
}

void 
destroyCell(cell)
  Cell *cell;
{

  if(cell->switchDependentHeader)
    free(cell->switchDependentHeader);
  if(cell->fabricStats)
    free( cell->fabricStats);
  if(cell->algorithmStats)
    free(cell->algorithmStats);

  if(debug_cell)
    printf("Destroyed cell ID %lu at time %lu, created at %lu\n",
           cell->commonStats.ID, now, cell->commonStats.createTime);

#ifdef DEBUG_CELL
  cellFree(cell);
#else
#ifdef FREELIST
  ringDealloc(&cellFreeList, cell);
#else
  free(cell);
#endif
#endif

}

void *
cellAlloc( somemem )
  int somemem;
{
  void *mem_ptr;
  mem_ptr = (void *) malloc( somemem );    
  numCellsAllocd++;
  printf("ALLOC:  %ld Cells allocated\n", numCellsAllocd); 
  return( mem_ptr );
}
void 
cellFree(mem)
  void *mem;
{
  free(mem);   
  numCellsAllocd--;
  printf("FREE:  %ld Cells allocated\n", numCellsAllocd);
}


