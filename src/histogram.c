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
#include "histogram.h"

#ifdef DEBUG_HISTOGRAM
int debugHistogram=1;
#else
int debugHistogram=0;
#endif

void initHistogram(char *name, struct Histogram *histogram, HistogramType type,
                   HistogramValueType valueType, HistogramStepType stepType,
                   int numBins, double minBin, double binStepSize, 
                   int dataStructureSize, void (*histogramPrintFunction)())

{
  int bin;

  histogram->type = type;
  histogram->valueType = valueType;
  histogram->name = (char *) malloc(strlen(name)+1);
  strcpy(histogram->name, name);
  histogram->enable = HISTOGRAM_DISABLE;

  histogram->histogramPrintFunction = (void (*)()) histogramPrintFunction;

  if( debugHistogram )
    {
      fprintf(stderr, "Initiating histogram \"%s\" of type ", name);
      if( type == HISTOGRAM_STATIC )
	{
	  fprintf(stderr, "HISTOGRAM_STATIC\n");
	  fprintf(stderr, "    Fixed number of bins: %d\n", numBins);
	}
      else
	{
	  fprintf(stderr, "HISTOGRAM_DYNAMIC\n");
	  fprintf(stderr, "    Max number of bins: %d\n", numBins);
	}
	
      switch( valueType )
	{
	case HISTOGRAM_INTEGER_EXACT:
	case HISTOGRAM_INTEGER_EXACT_INC:
	  fprintf(stderr, "HISTOGRAM_INTEGER_EXACT \n");
	  break;
	case HISTOGRAM_INTEGER_EXACT_SUM:
	  fprintf(stderr, "HISTOGRAM_INTEGER_EXACT SUM\n");
	  break;
	case HISTOGRAM_INTEGER_LIMIT:
	  fprintf(stderr, "HISTOGRAM_INTEGER_LIMIT \n");
	  if( stepType == HISTOGRAM_STEP_LINEAR )
	    fprintf(stderr, "Step type: LINEAR\n");
	  else
	    fprintf(stderr, "Step type: LOG\n");
	  fprintf(stderr, "    Minimum bin: %g\n", minBin);
	  fprintf(stderr, "      Step Size: %g\n", binStepSize);
	  break;
	case HISTOGRAM_DOUBLE_EXACT:
	  fprintf(stderr, "HISTOGRAM_DOUBLE_EXACT \n");
	  break;
	case HISTOGRAM_DOUBLE_LIMIT:
	  fprintf(stderr, "HISTOGRAM_DOUBLE_LIMIT \n");
	  if( stepType == HISTOGRAM_STEP_LINEAR )
	    fprintf(stderr, "Step type: LINEAR\n");
	  else
	    fprintf(stderr, "Step type: LOG\n");
	  fprintf(stderr, "    Minimum bin: %g\n", minBin);
	  fprintf(stderr, "      Step Size: %g\n", binStepSize);
	  break;
	case HISTOGRAM_STRUCTURE_EXACT:
	  fprintf(stderr, "HISTOGRAM_STRUCTURE_EXACT \n");
	  break;
	}
    }
	

  if( histogram->type == HISTOGRAM_STATIC )
    {
      histogram->stepType = stepType;
      histogram->minBin = minBin;
      histogram->stepSize = binStepSize;
    }

  histogram->bin = (struct HistogramBin *) 
    malloc( numBins * sizeof(struct HistogramBin) );
  if( !histogram->bin )
    {
      fprintf(stderr, "Couldn't allocate histogram memory\n");
      exit(1);		
    }

  switch( type )
    {
    case HISTOGRAM_STATIC:
      histogram->number = numBins;
      histogram->noBin=0.0;
		
		
      for(bin=0; bin<histogram->number; bin++)
	{	
	  histogram->bin[bin].number = 0;
	  switch( valueType )
	    {
	    case HISTOGRAM_DOUBLE_LIMIT:
	      if(bin==0)
		{
		  histogram->bin[bin].doubleLowerLimit = 0;
		  histogram->bin[bin].doubleUpperLimit = minBin;
		}
	      else
		{
		  histogram->bin[bin].doubleLowerLimit =  
		    histogram->bin[bin-1].doubleUpperLimit; 
		  if( histogram->stepType == HISTOGRAM_STEP_LOG )
		    histogram->bin[bin].doubleUpperLimit =  
		      binStepSize * histogram->bin[bin].doubleLowerLimit;
		  else if( histogram->stepType == HISTOGRAM_STEP_LINEAR )
		    histogram->bin[bin].doubleUpperLimit =  
		      binStepSize + histogram->bin[bin].doubleLowerLimit;
		}
	      break;
	    case HISTOGRAM_INTEGER_LIMIT:
	      if(bin==0)
		{
		  histogram->bin[bin].intLowerLimit = 0;
		  histogram->bin[bin].intUpperLimit = (int)minBin;
		}
	      else
		{
		  histogram->bin[bin].intLowerLimit =  
		    histogram->bin[bin-1].intUpperLimit; 
		  if( histogram->stepType == HISTOGRAM_STEP_LOG )
		    histogram->bin[bin].intUpperLimit =  (int) 
		      binStepSize * histogram->bin[bin].intLowerLimit;
		  else if( histogram->stepType == HISTOGRAM_STEP_LINEAR )
		    histogram->bin[bin].intUpperLimit =  (int) 
		      binStepSize + histogram->bin[bin].intLowerLimit;
		}
	      break;
	    default:
	      fprintf(stderr, "Histogram: not implemented yet\n");
	    }
	}	
      break;
    case HISTOGRAM_DYNAMIC:
      histogram->maxNumber = numBins;
      histogram->number = 0;
      histogram->noBin=0.0;
      for(bin=0; bin<histogram->number; bin++)
	{	
	  histogram->bin[bin].number=0;
	}
      histogram->dataStructureSize = dataStructureSize;
      break;
    }
}

/* Reset all bins to zero. */
void resetHistogram(struct Histogram *histogram)
{
  int bin;

  for( bin=0; bin<histogram->number; bin++)
    histogram->bin[bin].number = 0;
  histogram->noBin=0.0;

  switch( histogram->type )
    {
    case HISTOGRAM_STATIC:
      break;
    case HISTOGRAM_DYNAMIC:
      histogram->number = 0;
      break;		
    default:
      fprintf(stderr, "resetHistogram: Unknown type.\n");
      exit(1);
    }
}

void updateHistogram(struct Histogram *histogram, union StatsValue aValue,
                     double amount)
  /* Used for _SUM histograms when a bin can be incremented by 
                more than 1 at a time */
{
  int foundBin=0;
  int bin;
	
  if( histogram->enable == HISTOGRAM_DISABLE )
    {
      if(debugHistogram) fprintf(stderr, "Histogram \"%s\" disabled\n", 
				 histogram->name);
      return;
    }

  switch( histogram->valueType )
    {
    case HISTOGRAM_DOUBLE_LIMIT:
      {
	double value = aValue.aDouble;
	if(value > histogram->bin[histogram->number-1].doubleUpperLimit)
	  {
	    foundBin = 0;
	    goto finishedSearch;
	  }
	switch( histogram->stepType )
	  {
	  case HISTOGRAM_STEP_LINEAR: 
	    {
	      if( value < histogram->minBin )
		bin=0;
	      else
		bin=(int)((value-histogram->minBin)/
			  histogram->stepSize) + 1;
	      if(debugHistogram)
		if((value < histogram->bin[bin].doubleLowerLimit)
		   || (value >= histogram->bin[bin].doubleUpperLimit))
		  fprintf(stderr, "Histogram Update: value %g does not fit in bin %d of histogram \"%s\" with limits [%g,%g]\n", 
			  value, bin, histogram->name, 
			  histogram->bin[bin].doubleLowerLimit, 
			  histogram->bin[bin].doubleUpperLimit);
	      break;
	    }
	  case HISTOGRAM_STEP_LOG: 	
	    {
	      /* Find bin by Successive Approximation. */
	      int binDiff;
	      int numtimes=0;
	      bin = histogram->number / 2;
	      binDiff = bin;
	      while(!foundBin)
		{
		  if( numtimes++ > histogram->number )
		    {
		      fprintf(stderr, "Histogram error: %s\n", histogram->name);
		      printHistogram( stderr, histogram );
		      exit(1);
		    }
		  binDiff /= 2; 
		  if(binDiff==0) binDiff=1;
		  if( value >= histogram->bin[bin].doubleLowerLimit )
		    {
		      if( value < histogram->bin[bin].doubleUpperLimit )
			foundBin=1;
		      else
			bin += binDiff;		
		    }
		  else
		    bin -= binDiff;
		}
	      if(debugHistogram)
		if((value < histogram->bin[bin].doubleLowerLimit)
		   || (value >= histogram->bin[bin].doubleUpperLimit))
		  fprintf(stderr, "Histogram Update: value %g does not fit in bin %d of histogram \"%s\" with limits [%g,%g]\n", 
			  value, bin, histogram->name, 
			  histogram->bin[bin].doubleLowerLimit, 
			  histogram->bin[bin].doubleUpperLimit);
	      break;
	    }
	  default:
	    fprintf(stderr,"Illegal histogram step type: %d\n",
		    histogram->stepType);
	    exit(1);
	  }
	histogram->bin[bin].number++;
	foundBin=1;
	goto finishedSearch;
	break;
      }
    case HISTOGRAM_INTEGER_EXACT:
    case HISTOGRAM_INTEGER_EXACT_INC:
      {
	int value = aValue.anInt;

	for( bin=0; bin<histogram->number; bin++)
	  {
	    if((value == histogram->bin[bin].intExactValue) &&
	       (histogram->bin[bin].number != 0.0) )
	      {
		histogram->bin[bin].number++;
		foundBin=1;
		goto finishedSearch;
	      }
	  }
      }
    break;
    case HISTOGRAM_INTEGER_EXACT_SUM:
      {
	int value  = aValue.anInt;

	for( bin=0; bin<histogram->number; bin++)
	  {
	    if((value == histogram->bin[bin].intExactValue) &&
	       (histogram->bin[bin].number != 0.0) )
	      {
		histogram->bin[bin].number += amount;
		foundBin=1;
		goto finishedSearch;
	      }
	  }
      }
    break;
    case HISTOGRAM_INTEGER_LIMIT:
      {
	int value = aValue.anInt;

	if( value > histogram->bin[histogram->number-1].intUpperLimit )
	  {
	    foundBin = 0;
	    goto finishedSearch;
	  }

	switch( histogram->stepType )
	  {
	  case HISTOGRAM_STEP_LINEAR: 
	    {
	      if( value < (int)histogram->minBin)
		bin=0;
	      else
		bin = ((value-(int)histogram->minBin)/
		       (int)histogram->stepSize) + 1;
	      if(debugHistogram)
		if((value < histogram->bin[bin].intLowerLimit)
		   || (value >= histogram->bin[bin].intUpperLimit))
		  fprintf(stderr, "Histogram Update: value %d does not fit in bin %d of histogram \"%s\" with limits [%d,%d]\n", value, bin, histogram->name, histogram->bin[bin].intLowerLimit, histogram->bin[bin].intUpperLimit);
	      break;
	    }
	  case HISTOGRAM_STEP_LOG: 	
	    {
	      /* Find bin by Successive Approximation. */
	      int binDiff;
	      int numtimes=0;
	      bin = histogram->number / 2;
	      binDiff = bin;
	      while(!foundBin)
		{
		  if( numtimes++ > histogram->number )
		    {
		      fprintf(stderr, "Histogram error: %s. Value: %d\n",
			      histogram->name, value);
		      printHistogram(stderr, histogram);
		      exit(1);
		    }
		  binDiff /= 2;
		  if(binDiff==0) binDiff=1;
		  if( value >= histogram->bin[bin].intLowerLimit )
		    {
		      if( value < histogram->bin[bin].intUpperLimit )
			foundBin=1;
		      else
			bin += binDiff;		
		    }
		  else
		    bin -= binDiff;
		}
	      if(debugHistogram)
		if((value < histogram->bin[bin].intLowerLimit)
		   || (value >= histogram->bin[bin].intUpperLimit))
		  fprintf(stderr, "Histogram Update: value %d does not fit in bin %d of histogram \"%s\" with limits [%d,%d]\n", value, bin, histogram->name, histogram->bin[bin].intLowerLimit, histogram->bin[bin].intUpperLimit);
	      break;
	    }
	  default:
	    fprintf(stderr,"Illegal histogram step type: %d\n",
		    histogram->stepType);
	    exit(1);
	  }
	histogram->bin[bin].number++;
	foundBin=1;
	goto finishedSearch;
	break;
      }
    case HISTOGRAM_DOUBLE_EXACT:
      {
	double value = aValue.aDouble;
	for( bin=0; bin<histogram->number; bin++)
	  {
	    if((value == histogram->bin[bin].doubleExactValue) &&
	       (histogram->bin[bin].number != 0.0) )
	      {
		histogram->bin[bin].number++;
		foundBin=1;
		goto finishedSearch;
	      }
	  }
      }
    break;
    case HISTOGRAM_STRUCTURE_EXACT:
      {
	void *valuePtr = aValue.aPtr; 

	for( bin=0; bin<histogram->number; bin++)
	  {
	    if(!memcmp(valuePtr, histogram->bin[bin].structExactValue,
		       histogram->dataStructureSize) &&
	       (histogram->bin[bin].number != 0.0) )
	      {
		histogram->bin[bin].number++;
		foundBin=1;
		goto finishedSearch;
	      }
	  }
      }
    break;
    default:
      fprintf(stderr, "Histogram: Illegal valueType!\n");
      exit(1);
    }

finishedSearch:
  if(!foundBin)
    {
      if( histogram->type == HISTOGRAM_STATIC )
	histogram->noBin++;
      else
	{
	  /* Allocate new bin */
	  if( histogram->number >= histogram->maxNumber )
	    {
	      fprintf(stderr, "Histogram \"%s\" overflowed\n", histogram->name);
	      histogram->noBin++;
	    }
	  else 
	    {
	      switch( histogram->valueType )
		{
		  /* Use next empty bin */
		case HISTOGRAM_INTEGER_EXACT:
		case HISTOGRAM_INTEGER_EXACT_INC:
		  {
		    int value = aValue.anInt;
		    histogram->bin[histogram->number].intExactValue = 
		      value;
		    histogram->bin[histogram->number].number=1.0; 
		    histogram->number++;
		    break;
		  }
		case HISTOGRAM_INTEGER_EXACT_SUM:
		  {
		    int value = aValue.anInt;
		    histogram->bin[histogram->number].intExactValue = 
		      value;
		    histogram->bin[histogram->number].number=amount; 
		    histogram->number++;
		    break;
		  }
		case HISTOGRAM_DOUBLE_EXACT:
		  {
		    double value = aValue.aDouble;
		    histogram->bin[histogram->number].doubleExactValue = 
		      value;
		    histogram->bin[histogram->number].number=1.0; 
		    histogram->number++;
		    break;
		  }
		case HISTOGRAM_STRUCTURE_EXACT:
		  {
		    void *value = aValue.aPtr;
		    histogram->bin[histogram->number].structExactValue =
		      (void *) malloc( histogram->dataStructureSize);
		    memcpy(histogram->bin[histogram->number].structExactValue, value, histogram->dataStructureSize);
		    histogram->bin[histogram->number].number=1.0; 
		    histogram->number++;
		    break;
		  }
		default:
		  fprintf(stderr, "Histogram %s: illegal valuetype!\n",
			  histogram->name);
		  exit(1);
		}
	    }
			
	}
    }
}

void enableHistogram(struct Histogram *histogram)
{
  histogram->enable = HISTOGRAM_ENABLE;
}
void disableHistogram(struct Histogram *histogram)
{
  histogram->enable = HISTOGRAM_DISABLE;
}

void writeHistogram(FILE *fp, struct Histogram *histogram)
{
  int bin;

  if( fwrite(histogram, sizeof(struct Histogram), 1, fp) != 1)
    {
      fprintf(stderr, "Couldn't write histogram \"%s\"to file\n", 
	      histogram->name);
      exit(1);
    }

  for( bin=0; bin<histogram->number; bin++ )
    {
      if(fwrite(&histogram->bin[bin], sizeof(struct HistogramBin),1,fp) != 1)
	{
	  fprintf(stderr, "Couldn't write histogram \"%s\"to file\n", 
		  histogram->name);
	  exit(1);
	}
    }
}

void readHistogram(FILE *fp, struct Histogram *histogram)
{
  int bin;

  if( fread(histogram, sizeof(struct Histogram), 1, fp) != 1)
    {
      fprintf(stderr, "Couldn't read histogram \"%s\" from file\n", 
	      histogram->name);
      exit(1);
    }

  for( bin=0; bin<histogram->number; bin++ )
    {
      if(fread(&histogram->bin[bin], sizeof(struct HistogramBin),1,fp) != 1)
	{
	  fprintf(stderr, "Couldn't read histogram \"%s\" from file\n", 
		  histogram->name);
	  exit(1);
	}
    }
}


void printHistogram(FILE *fp, struct Histogram *histogram)
{
  int bin;
  double totalNumber=histogram->noBin;
  double cumulativeTotal=0;

  if( histogram->histogramPrintFunction ) 
    {
      if( strlen(histogram->name) )
	fprintf(fp, "Histogram: %s (custom)\n", histogram->name);
      (histogram->histogramPrintFunction)(fp, histogram);
      return;
    }

  if( strlen(histogram->name) )
    fprintf(fp, "Histogram: %s\n", histogram->name);
  switch( histogram->valueType )
    {
    case HISTOGRAM_STRUCTURE_EXACT:
      break;

    case HISTOGRAM_DOUBLE_EXACT:
      for(bin=0; bin<histogram->number; bin++)
        totalNumber += histogram->bin[bin].number;
      fprintf(fp,"  Bin               #    %%Total\n");
      fprintf(fp,"--------------------------------\n");
      for(bin=0; bin<histogram->number; bin++)
	{
	  if(histogram->bin[bin].number > 0.0)
	    {
	      fprintf(fp, "%-12g %8g %8.4f\n", 
		      histogram->bin[bin].doubleExactValue,
		      histogram->bin[bin].number,
		      100*(histogram->bin[bin].number)/totalNumber);
	    }
	}	
      fprintf(fp, "Total number counted: %.0f\n", 
              totalNumber);
      if( histogram->noBin > 0.0 )
	fprintf(fp, "Number not in histogram: %g\n", 
                histogram->noBin);
      fprintf(fp, "================================================\n");
      break;
    case HISTOGRAM_DOUBLE_LIMIT:
      for(bin=0; bin<histogram->number; bin++)
	totalNumber += histogram->bin[bin].number;
      fprintf(fp,"  Bin               #    %%Total  %%Cumulative\n");
      fprintf(fp,"--------------------------------------------\n");
      for(bin=0; bin<histogram->number; bin++)
	{
	  if(histogram->bin[bin].number > 0.0)
	    {
	      cumulativeTotal += histogram->bin[bin].number;
	      fprintf(fp, "%-12g %8g %8.4f %8.4f\n", 
		      histogram->bin[bin].doubleLowerLimit,
		      histogram->bin[bin].number, 
		      100*(histogram->bin[bin].number)/totalNumber,
		      100*cumulativeTotal/totalNumber);
	    }
	}
      fprintf(fp, "Total number counted: %.0f\n", 
              totalNumber);
      if( histogram->noBin > 0.0 )
	fprintf(fp, "Number not in histogram: %g\n", 
                histogram->noBin);
      fprintf(fp, "================================================\n");
      break;
    case HISTOGRAM_INTEGER_EXACT:
    case HISTOGRAM_INTEGER_EXACT_INC:
    case HISTOGRAM_INTEGER_EXACT_SUM:
      {
	for(bin=0; bin<histogram->number; bin++)
	  totalNumber += histogram->bin[bin].number;
	fprintf(fp,"  Bin               #    %%Total\n");
	fprintf(fp,"--------------------------------\n");
	for(bin=0; bin<histogram->number; bin++)
	  {
	    if(histogram->bin[bin].number > 0.0)
	      {
		fprintf(fp, "%-12d %8g %8.4f\n", 
			histogram->bin[bin].intExactValue,
			histogram->bin[bin].number,
			100*(histogram->bin[bin].number)/totalNumber);
	      }
	  }
	fprintf(fp, "Total number counted: %.0f\n", 
		totalNumber);
	if( histogram->noBin > 0.0 )
	  fprintf(fp, "Number not in histogram: %g\n", 
		  histogram->noBin);
	fprintf(fp, "================================================\n");
      }
    break;
    case HISTOGRAM_INTEGER_LIMIT:
      for(bin=0; bin<histogram->number; bin++)
	totalNumber += histogram->bin[bin].number;
      fprintf(fp,"  Bin               #    %%Total  %%Cumulative\n");
      fprintf(fp,"--------------------------------------------\n");
      for(bin=0; bin<histogram->number; bin++)
	{
	  if(histogram->bin[bin].number > 0.0)
	    {
	      cumulativeTotal += histogram->bin[bin].number;	
	      fprintf(fp, "%-12d %8g %8.4f %8.4f\n", 
		      histogram->bin[bin].intLowerLimit,
		      histogram->bin[bin].number,
		      100*(histogram->bin[bin].number)/totalNumber,
		      100*cumulativeTotal/totalNumber);
	    }
	}
      fprintf(fp, "Total number counted: %.0f\n", 
              totalNumber);
      if( histogram->noBin > 0.0 )
	fprintf(fp, "Number not in histogram: %g\n", 
                histogram->noBin);
      fprintf(fp, "================================================\n");
      break;
    default:
      fprintf(stderr, "Illegal histogram type: %d\n", 
              histogram->valueType);
      exit(1);
    }
	
}

/* Frees all the space allocated by histogram, without
 * actually freeing the histogram itself.
 */
void destroyHistogram(struct Histogram *histogram)
{   
    int i;

    /* Free contents of bins depending on type */
    if(histogram->type != HISTOGRAM_STATIC)
        if(histogram->valueType == HISTOGRAM_STRUCTURE_EXACT)
            for(i=0; i<histogram->number; i++)
                free(histogram->bin[i].structExactValue);

    free(histogram->name);
    free(histogram->bin);
}
