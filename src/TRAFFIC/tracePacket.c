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
#include <string.h>
#include <sys/types.h>
#include <unistd.h>


#include "sim.h"
#include "traffic.h"
#include "inputAction.h"
#include "trace.h"

/* #define debug_traffic 1 */

/************************************************************/
/* Trace Packet file format:	               				*/
/*		LengthInCells (int) TimeInCells(double) VCI(u_long)	*/
/************************************************************/

typedef struct {
  FILE *fp;	/* Pointer to trace file */
  int child; /* 1=>fp points to a pipe, not a file */

  struct TracePacket packet;

  unsigned long numCellsLeft;
  /* Number of cells left to be sent for current packet */

  unsigned long numCellsGenerated;

  int period;			/* How often traffic is called. Scales "now" */
	
  char *traceFileName;
  int  lineNumber;     /* Line number being read by trace file. */

  int offset;
} TraceTraffic;

static int readPacket();

/* Trace file: reads <time,vci> from file (must be specified) until */
/* EOF is reached or simulation stops. */
int
tracePacket(action, aSwitch, input, argc, argv)
  TrafficAction action;
Switch *aSwitch;
int input;
int argc;
char **argv;
{

  TraceTraffic *traffic;
  Cell *aCell;
  unsigned int output;
  unsigned int vci;

  switch( action )
    {	
    case TRAFFIC_USAGE:
      fprintf(stderr,"Options for \"trace\" traffic model:\n");
      fprintf(stderr,"\"tracePacket\" can not be called from command line option\n");
      break;
    case TRAFFIC_INIT:
      {
	int c;
	int i,j;
	char *pos;
	extern int optind;
	extern int optopt;
	extern int opterr;
	extern char *optarg;
	int fflag=NO;
	int pflag=NO;

	optind=1;
	opterr=0;

	if( debug_traffic)
	  printf("Traffic init for switch %d input %d\n", 
		 aSwitch->switchNumber, input);

	traffic = (TraceTraffic *) malloc(sizeof(TraceTraffic));
	aSwitch->inputBuffer[input]->traffic = (void *) traffic;
	traffic->period=1;

	for(i=0;i<=argc;i++)
	  {
	    c = getopt(argc, argv, "f:p:");
	    switch(c)
	      {
	      case 'p':
		pflag = YES;
		traffic->period = atoi(optarg);
		break;
	      case 'f':
		fflag = YES;
		traffic->traceFileName = malloc(256);
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
		  {
		    *pos=' ';
		  }
		printf("Filename %s\n", traffic->traceFileName);
		break;
	      default:
		optind++; 
		break;
	      }
	  }
	if( fflag == NO )
	  {
	    fprintf(stderr, "MUST specify trace file name\n");
	    exit(1);
	  }

	/* Open trace file. */
	if( (pos = strstr(traffic->traceFileName, "|") ) )
	  {
	    char *path;
	    char **myargv;
	    int  myargc=0;
	    int pipefd[2];

	    /* Remove "|" character */
	    *pos='\0';

	    myargv = (char **) malloc(256*(sizeof(char *)));
	    path = strtok(traffic->traceFileName, " ");
	    printf("exec: %s ", traffic->traceFileName);
	    myargv[0] = path;
	    while((myargv[++myargc]=strtok(NULL," ")) 
		  != NULL )
	      {
		printf("%s ", myargv[myargc]);
	      }
	    printf("\n");

	    pipe(pipefd);
	    if( (traffic->child=fork()) == 0 )
	      {
		close(1);
		if( dup(pipefd[1]) == -1)
		  {
		    perror("dup");
		    exit(1);
		  }
		close(pipefd[0]);
		execv(path, myargv);
		perror("child");
		exit(1);
	      }
	    if(traffic->child==-1 )
	      perror("fork");
	    traffic->fp = fdopen(pipefd[0], "r");
	    if(!traffic->fp)
	      {
		perror("tracePacket");
		exit(1);
	      }
	    close(pipefd[1]);
	  }
	else if( strstr(traffic->traceFileName, ".gz" ) || 
		 strstr(traffic->traceFileName, ".Z" ) ) 
	  {
	    char cmd[256];
	    sprintf(cmd, "gunzip -c %s", traffic->traceFileName);
	    traffic->fp = popen(cmd, "r");
	    if(!traffic->fp)
	      {
		perror("tracePacket");
		exit(1);
	      }
	  }
	else
	  traffic->fp = fopen(traffic->traceFileName, "r");
	if( !traffic->fp )
	  {
	    perror("tracePacket");
	    fprintf(stderr, "Could not open trace file %s\n", 
		    traffic->traceFileName);
	    exit(1);
	  }


	/* Read next packet from trace file. */
	readPacket(traffic->fp, &traffic->packet);		
	/*
	  printf("First packet: ");
	  writePacket(traffic->fp, traffic);
	  */

	/* Initialize traffic structure */
	traffic->numCellsGenerated = 0;
	traffic->lineNumber = 1;
	traffic->numCellsLeft = 0;
	traffic->offset = lrand48();
		
	break;
      }
    case TRAFFIC_GENERATE:
      {
	if( debug_traffic)
	  printf("		GEN_TRAFFIC: ");
	traffic = (TraceTraffic *)aSwitch->inputBuffer[input]->traffic;

	/* If no more cells to send and it is not time for the next */
	/* packet, then return */
	if( !(traffic->numCellsLeft) && 
	    (traffic->period*traffic->packet.time > (double) now) )
	  return(CONTINUE_SIMULATION);

	/* New packet? */
	if( !(traffic->numCellsLeft) && 
	    (traffic->period*traffic->packet.time <= (double) now) )
	  {
	    traffic->numCellsLeft = traffic->packet.length;
	  }

	if( traffic->numCellsLeft )
	  {
	    vci = traffic->packet.dst+traffic->offset;
	    output = vci % aSwitch->numOutputs;
	    if(debug_traffic) 
	      printf("Switch %d,  Input %d, has cell for %u\n", 
		     aSwitch->switchNumber, input, traffic->packet.dst);
	    aCell = createCell(output, UCAST, DEFAULT_PRIORITY);	
	    traffic->numCellsGenerated++;
	    traffic->numCellsLeft--;

	
	    /* Execute the switch inputAction to accept cell */
			/* YOUNGMI: BUG FIX */
	    (aSwitch->inputAction)(INPUTACTION_RECEIVE, aSwitch, input, aCell, NULL, NULL, NULL, NULL);
	  }

	if( !traffic->numCellsLeft )
	  {
	    /* Read time of next packet. */
	    if( readPacket(traffic->fp, &traffic->packet) == EOF )
	      {
		return(STOP_SIMULATION);
	      }
	    /*
	      printf("Next packet: ");
	      writePacket(traffic->fp, traffic);
	      */
	  }
	if(now%10000==0)
	  traffic->offset++;

	break;
      }
    case REPORT_TRAFFIC_STATS:
      {
	traffic = (TraceTraffic *)aSwitch->inputBuffer[input]->traffic;
	printf("  %d  tracePacket  %f\n", input,
	       (double)traffic->numCellsGenerated/(double)now);
	break;
      }
    }

  return(0);
}

/* Read packet from trace file, ignoring comment lines */
static int
readPacket(fp, packet)
  FILE *fp;
struct TracePacket *packet;
{
  int num;
  num = fread(packet, sizeof(struct TracePacket), 1, fp);
  if(!num) 
    return(EOF);
  return(1);
}

#ifdef USE_ALL_FUNCS
static int
writePacket(fp, traffic)
  FILE *fp;
TraceTraffic *traffic;
{
  printf("Time: %f Numcells: %u Remaining: %lu VCI: 0x%x\n",
	 traffic->packet.time, traffic->packet.length,
	 traffic->numCellsLeft, traffic->packet.dst);
  return (0);
}
#endif
