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
#include <math.h>
#include "stat.h"

#define safe_sqrt(x)  (((x) < 0)? (0): sqrt((x)))


/************** General routines that act on a Stat structure *******/
void initStat(Stat *aStat, StatType type, unsigned long now)
{
  switch(type)
    {
    case STAT_TYPE_TIME_AVERAGE:
      aStat->type	= STAT_TYPE_TIME_AVERAGE;
      aStat->sum = 0;
      aStat->sumSquares = 0;
      aStat->number = 0;
      aStat->lastChangeTime = now;
      break;
    case STAT_TYPE_AVERAGE:
    default:
      aStat->type	= STAT_TYPE_AVERAGE;
      aStat->sum = 0;
      aStat->sumSquares = 0;
      aStat->number = 0;
    }

}

void enableStat(Stat *aStat)
{
  aStat->enable = 1;
}

void disableStat(Stat *aStat)
{
  aStat->enable = 0;
}

void updateStat(Stat *aStat, long aValue, unsigned long now)
{
  switch(aStat->type)
    {
    case STAT_TYPE_TIME_AVERAGE:
      aStat->number += (now - aStat->lastChangeTime);
      /* Add to sum */
      aStat->sum += (double)aValue * (now - aStat->lastChangeTime);
      /* Square and add to sumSquared */

		/* aStat->sumSquares += (double) (aValue*aValue) * (now - aStat->lastChangeTime); */

		 aStat->sumSquares += ((double) aValue) * ((double) aValue) * ((double)
        (now - aStat->lastChangeTime)); 

        /* Fix - (Suggested by Paul) - Sundar */

      aStat->lastChangeTime = now;
      break;

    case STAT_TYPE_AVERAGE:
    default:
      aStat->number++;
      /* Add to sum */
      aStat->sum += (double) aValue;
      /* Square and add to sumSquared */
       /* aStat->sumSquares += (double) (aValue*aValue); */
        /* Fix - (Suggested by Paul) - Sundar */
       aStat->sumSquares += ((double) aValue) * ((double)aValue); 
      break;
    }
}

void printStat(FILE *fp, char *aString, Stat *aStat)
{
  double EX, EX2, SD;

  if( !aStat->enable ) return;

  EX = returnAvgStat(aStat);
  EX2 = returnEX2Stat(aStat);
  SD = safe_sqrt( EX2 - (EX*EX) );
  fprintf(fp, "%20s %8.8g %8.8g    (%lu)\n", aString, EX, SD, aStat->number); 
}

unsigned long returnNumberStat(Stat *aStat)
{
  return(aStat->number);
}

double returnAvgStat(Stat *aStat)
{
  double EX;

  if( !aStat->number ) return(0.0);

  EX = (aStat->sum) / ((double) aStat->number);

  return(EX);
}

double returnEX2Stat(Stat *aStat)
{
  double EX2;

  if( !aStat->number ) return(0.0);
  EX2 = (aStat->sumSquares) / ((double) aStat->number);

  return(EX2);
}

