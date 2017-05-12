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

#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#include "sim.h"
#include "traffic.h"
#include "inputAction.h"

/********************************************************************/
/* Trace file format:							                    */
/*		TIME(u_long) U|M (char) (PRIORITY)(int) DESTN(int|bitmap) 	*/
/*		TIME(u_long) U|M (char) (PRIORITY)(int) DESTN(int|bitmap) 	*/
/*		TIME(u_long) U|M (char) (PRIORITY)(int) DESTN(int|bitmap) 	*/
/*		....                ...					                    */
/*		TIME(u_long) U|M (char) (PRIORITY)(int) DESTN(int|bitmap) 	*/
/*		"STOP" (optional)						                    */
/* STOP => end simulation when this trace file	                    */
/*			completes.							                    */
/********************************************************************/

typedef struct {
  FILE *fp;	/* Pointer to trace file */
  int child; /* 1=>fp points to a pipe, not a file */

  unsigned long nextCellTime;	/* Time of next cell from trace file.       */
  /* Used as a lookahead to speed up polling. */
  unsigned long numCellsGenerated;

  int prioritiesFlag;
  char *traceFileName;
  int  lineNumber;     /* Line number being read by trace file. */
  int  fileCompleted;
} TraceTraffic;

extern void printCell();

static int readNextCellTime();
static Cell * readNextCell();

/* Trace file: reads <time,vci> from file (must be specified) until */
/* EOF is reached or simulation stops. */
int
trace(action, aSwitch, input, argc, argv)
  TrafficAction action;
Switch *aSwitch;
int input;
int argc;
char **argv;
{

  TraceTraffic *traffic;
  Cell *aCell;
  char *pos;
  int output;

  switch( action )
    {	
    case TRAFFIC_USAGE:
      fprintf(stderr, "Options for \"trace\" traffic model:\n");
      fprintf(stderr, "   -p. Trace file contains priorities.\n");
      fprintf(stderr, "   -f filename. Name of trace file.\n");
      fprintf(stderr, "   File format:\n");
      fprintf(stderr, "    TIME(u_long) 'U'|'M'(char) PRIORITY(int - optional) DESTN(int or bitmap-string)\n");
      fprintf(stderr, "   Note: \"trace\" can not be called from command line option\n");
      break;
    case TRAFFIC_INIT:
      {
    int j;
	char c;

	extern int optind;
	extern int optopt;
	extern int opterr;
	extern char *optarg;
	int fflag=NO;
	int pflag=NO;

	optind=1;
	opterr=0;

	if( debug_traffic) 
	  printf("Traffic init for switch %d input %d\n", aSwitch->switchNumber, input);

	traffic = (TraceTraffic *) malloc(sizeof(TraceTraffic));
	aSwitch->inputBuffer[input]->traffic = (void *) traffic;
    traffic->prioritiesFlag = 0;


	while( (c = getopt(argc, argv, "f:p")) != EOF)
	  switch(c)
	    {
	    case 'f':
        {
            fflag = YES;
            traffic->traceFileName = malloc(strlen(optarg));
            strcpy(traffic->traceFileName, optarg);
            if( traffic->traceFileName[0] == '"')
            {
                traffic->traceFileName++;
                for(j=optind; j<argc; j++)
                {
                 sprintf(traffic->traceFileName, "%s %s",
                    traffic->traceFileName, argv[j]);
                 if( strstr(argv[j], "\"") )
                    break;
                }
             }
            while((pos=strstr(traffic->traceFileName, "\"")))
                *pos=' ';
	      break;
        }
	    case 'p':
	      pflag = YES;
          traffic->prioritiesFlag = 1;
          break;
	    case '?':
	      fprintf(stderr, "-------------------------------\n");
	      fprintf(stderr, "trace: Unrecognized option -%c\n", optopt);
	      trace(TRAFFIC_USAGE);
	      fprintf(stderr, "--------------------------------\n");
	      exit(1);
	    default:
	      break;
	    }
	if( fflag == NO )
	  {
	    fprintf(stderr, "MUST specify trace file name\n");
	    exit(1);
	  }

	/* Open trace file. */

    /* Is it to be exec'd and piped in? */
    if( (pos = strstr(traffic->traceFileName, "|") ) )
    {
        fprintf(stderr,"trace: This mode not supported \n");
        exit(1);
    }
    else if( strstr(traffic->traceFileName, ".gz" ) ||
         strstr(traffic->traceFileName, ".Z" ) )
      {
        char cmd[256];
        sprintf(cmd, "gunzip -c %s", traffic->traceFileName);
        printf("        Uncompressing and reading from: %s \n", traffic->traceFileName);
        traffic->fp = popen(cmd, "r");
        if(!traffic->fp)
          {
        perror("trace");
        exit(1);
          }
      }
    else
    {
	    traffic->fp = fopen(traffic->traceFileName, "r");
        printf("        Reading from: %s \n", traffic->traceFileName);
    }

	if( !traffic->fp )
	  {
	    perror("Trace:");
	    fprintf(stderr, "Could not open trace file %s\n", 
		    traffic->traceFileName);
	    exit(1);
	  }

	/* Initialize traffic structure */
	traffic->numCellsGenerated = 0;
	traffic->lineNumber = 1;
	traffic->fileCompleted = 0;

	/* Read time of first cell. */
	readNextCellTime(traffic->fp, traffic);
		
	break;
      }
    case TRAFFIC_GENERATE:
      {
	if( debug_traffic)
	  printf("		GEN_TRAFFIC: ");
	traffic = (TraceTraffic *)aSwitch->inputBuffer[input]->traffic;

	/* If file has been completed or if it is not time for */
	/* the next cell, then return */
	if( traffic->fileCompleted || traffic->nextCellTime > now )
	  return(CONTINUE_SIMULATION);

	/* Schedule cell. */
	aCell = readNextCell(traffic->fp, traffic);
    if(aCell->priority >= aSwitch->numPriorities)
    {
        fprintf(stderr, " Time %lu : trace file %s contains cell with priority %d. Switch only supports %d priorities!\n", now, traffic->traceFileName, aCell->priority, aSwitch->numPriorities);
        exit(1);
    }

	if(debug_traffic) 
	  printf("Switch %d,  Input %d, has new cell for %d\n", aSwitch->switchNumber, input, output);
	traffic->numCellsGenerated++;

	/* Execute the switch inputAction to accept cell */
	(aSwitch->inputAction)(INPUTACTION_RECEIVE, aSwitch, input, aCell, NULL, NULL, NULL, NULL);

	/* Read time of next cell. */
	switch(readNextCellTime(traffic->fp, traffic))
	  {
	  case STOP_SIMULATION:
	    traffic->fileCompleted = 1;
	    printf("Simulation stopped in file \"%s\", input %d\n",
		   traffic->traceFileName, input);
	    return(STOP_SIMULATION);
	  case EOF:
	    traffic->fileCompleted = 1;
	    return(EOF);
	  default:
	    break;
	  }

	if( traffic->nextCellTime <= now )
	  {
	    fprintf(stderr, "Time error in trace file %s at line %d\n", 
		    traffic->traceFileName, traffic->lineNumber);
	    fprintf(stderr, "Time now: %lu. Time of cell: %lu\n", 
		    now, traffic->nextCellTime);
	    exit(1);
	  }

	break;
      }
    case REPORT_TRAFFIC_STATS:
      {
	traffic = (TraceTraffic *)aSwitch->inputBuffer[input]->traffic;
	printf("  %d  trace  %f\n", input,
	       (double)traffic->numCellsGenerated/(double)now);
	break;
      }
    }

  return(CONTINUE_SIMULATION);
}

/* Read cell time from trace file, ignoring comment lines */
static int
readNextCellTime(fp, traffic)
  FILE *fp;
TraceTraffic *traffic;
{
  char c;

  /* Check for and ignore comment lines. */
  c = getc(fp);
  if( c == '#' )
    {
      while( ( (c=getc(fp)) != '\n') && (c!=EOF));
      traffic->lineNumber++;
      return( readNextCellTime(fp, traffic) );
    }
  else if( c == '\n' )
    {
      traffic->lineNumber++;
      return( readNextCellTime(fp, traffic) );
    }
  else if( c == EOF )
    return(EOF);
  else if( c == 'S' )
    return(STOP_SIMULATION);
  ungetc(c, fp);

  /* Read cell time */
  if( !fscanf(fp, "%lu", &traffic->nextCellTime) )
    {
      fprintf(stderr, "Error reading time from trace file %s line %d\n",
	      traffic->traceFileName, traffic->lineNumber);
      exit(1);
    }
  return(CONTINUE_SIMULATION);
}

/* Read U|M, Priority(optional) and destination bitmap from trace file */
static Cell *
readNextCell(fp, traffic)
FILE *fp;
TraceTraffic *traffic;
{
    int output, priority;
    char c;
    Cell *aCell;

  /* Read U|M flag */
  while((c=fgetc(fp)) == ' ');

  /* Read Priority(optional) field flag */
  if(traffic->prioritiesFlag)
  {
    if( !fscanf(fp, "%d", &priority) )
        {
        fprintf(stderr, "Error reading priority from trace file %s line %d\n",
	        traffic->traceFileName, traffic->lineNumber);
        exit(1);
        }

  }
  else 
    priority = DEFAULT_PRIORITY;

  switch(c)
  {
    case 'U':
    case 'u':
        if( !fscanf(fp, "%d", &output) )
            {
            fprintf(stderr, "Error reading output from trace file %s line %d\n",
	            traffic->traceFileName, traffic->lineNumber);
            exit(1);
            }
	    aCell = createCell(output, UCAST, priority);	
        if(debug_traffic)
            printf("TRACE: time %lu UCAST cell generated at priority %d for output %d\n", now, priority, output);
        break;
    case 'M':
    case 'm':
	    aCell = createCell(0, MCAST, priority);	
        if( !bitmapRead(fp, &(aCell->outputs)))
        {
            fprintf(stderr, "Error reading bitmap from trace file %s line %d\n",
	            traffic->traceFileName, traffic->lineNumber);
            exit(1);
        }
        if( !bitmapAnyBitSet( &(aCell->outputs ) ) )
        {
            fprintf(stderr, "Error reading bitmap from trace file %s line %d\n",
	            traffic->traceFileName, traffic->lineNumber);
            exit(1);
        }
        if(debug_traffic)
        {
            printf("TRACE: time %lu MCAST cell generated at priority %d for outputs: ", now, priority);
            printCell(stdout, aCell);
        }
        break;
    default:
      fprintf(stderr, "Error reading U|M flag from trace file %s line %d\n",
	      traffic->traceFileName, traffic->lineNumber);
      exit(1);
  }

  return(aCell);
}
