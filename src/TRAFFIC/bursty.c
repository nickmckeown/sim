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
#include "sim.h"
#include "traffic.h"
#include "inputAction.h"
#include <string.h>

typedef enum {BURSTY_IDLE, BURSTY_BUSY} BurstyState;

#define toggle_state(s)  (((s) == BURSTY_IDLE)? BURSTY_BUSY: BURSTY_IDLE)

/* 
 *  Simple packet bursty model 
 *  Starts idle. 
 *  When a new burst is decided, we also decide its length in cells 
 *  If burst=0, decide how long the idle period should be in cells 
 *  If idle=0, decide how long the burst period should be in cells
 *  etc. 
 */

/* Each successive burst has a destination correlated with the last. 
 * Probabiliy that the destination changes is traffic->probDestinationSwitch 
 */

/* Sundar: state: whether currently the burst is active or not. 
 * probDestinationSwitch is set to 1.0 - prob of change
 */

typedef struct {

  double *util;
  double *Mutil;    
  double totalUtil;
  int 	state;	/* Current state: IDLE or ACTIVE */
  double 	a; /* Geometric parameter for IDLE periods */
  double 	b; /* Geometric parameter for BUSY periods */
  int 	numCellsLeft; /* Number more cells in burst/idle period */
  double 	probDestinationSwitch; /* Prob of new destination address */
  int 	changeDestPerCell; /* Change destination by cell (T) or burst (F)*/
  Bitmap   	lastMCOutput;
  int lastOutput;
  int lastPriority;
  int 	fanout;
  double mcastFraction;
  Stat	multicastStat;
  /* Count of number of cells generated for this input => utilization */
  unsigned long int 	numCellsGenerated;		
  unsigned long int 	numOutputCellsGenerated;		
  unsigned short switch_seed[3];
  unsigned short priority_seed[3];
  unsigned short out_seed[3];
  unsigned short geom_seed_a[3];
  unsigned short geom_seed_b[3];
  unsigned short mcast_seed[3];
  int cellTypeCurrentBurst; /* Whether current burst is mcast or ucast */
  int priorityLevels;
  unsigned long int * numUcastCellsperFIFO;
  unsigned long int * numMcastCellsperFIFO;

} BurstyTraffic;

static void changeState();
static int geometric();

int 
bursty(action, aSwitch, input, argc, argv)
  TrafficAction action;
Switch *aSwitch;
int input;
int argc;
char **argv;
{
  BurstyTraffic *traffic;
  Cell *aCell;
  double psend;
  int output;
  int thispriority=0;
  Bitmap mcOutput;


  double burstLength=10;
  double utilization=0.5;
  double probDestinationSwitch=1.0;
  int changeDestPerCell=NO;
  double  mcastFraction=0.5;
	
  int rflag = NO;
  int uflag = NO;
  int bflag = NO;
  int dflag = NO;
  int mflag = NO;
  int Rflag=NO;
  int fflag = NO;
  int pflag = NO;

  int pri, priority;
  char * rstring = "";     /* the string of utilization of each priority */
  char * Rstring = "";     /* the string of utilization for priorities */
  char * str;
  double total;
  int index;
  int reindex=0;
  double prioritysum =0;
   
  int priorityLevels=1;

  switch(action) 
    {
    case TRAFFIC_USAGE:
      fprintf(stderr, "Options for \"bursty\" traffic model:\n");
      fprintf(stderr, "    -u total utilization.       Default: %f\n", utilization);
	  fprintf(stderr, "    -p number of prioritylevels of traffic generated. Default: %d\n", priorityLevels);
	  fprintf(stderr, "    -r r0:r1:r2:r3: ...rP-1, unicast utilization ratio per priority level, where P is the number of priority levels \n");
      fprintf(stderr, "    -b mean_burst_length. Default: %f\n", burstLength);
      fprintf(stderr, "    -d destination switching probability. Default: %f\n", probDestinationSwitch);
      fprintf(stderr, "    -c Change destination per-cell rather than per-burst. Default: per burst\n");
      fprintf(stderr, "    -m fraction (in range [0-1]) of traffic generated that is multicast. Default: %f\n", mcastFraction);
      fprintf(stderr, "    -R r0:r1:r2:r3... rP-1, multicast utilization tatio per priority level, defaults to unicast utilization per priority level if not specified\n");
      fprintf(stderr, "    -f fixed fanout. Default: U[0,2^N]\n");
      break;

    case TRAFFIC_INIT:
      {
	double idleLength;
	int fanout=1;
	int c;
	extern int optind;
	extern int optopt;
	extern int opterr;
	extern char *optarg;
	    
	if( debug_traffic)
	  printf("BURSTY: Traffic init for switch %d input %d\n", aSwitch->switchNumber, input);

	optind=1;
	opterr=0;
	while( (c = getopt(argc, argv, "b:u:d:cm:f:p:r:R:")) != EOF)
	  switch(c)
	    {
	    case 'b':
	      sscanf(optarg, "%lf", &burstLength);
	      bflag = 1;
	      break;
	    case 'u':
	      sscanf(optarg, "%lf", &utilization); 
	      /* uflag = 1; */
		  uflag = YES;
		  /* ustring = optarg; */
	      break;
	    case 'r':
		  rflag = YES;
		  rstring = optarg; 
	      break;
	    case 'R':
	      Rflag = YES;
	      Rstring = optarg;
	      break;
		case 'p':          
          pflag = YES;
	      priorityLevels = atoi(optarg);

				if ( priorityLevels > aSwitch->numPriorities) {
        fprintf(stderr, "input %d: Number of traffic priorities %d is greater than the number of switch priorities %d", input, priorityLevels, aSwitch->numPriorities);
				exit(1);
				} 
		  break;
	    case 'd':	
	      sscanf(optarg, "%lf", &probDestinationSwitch);
	      dflag = 1;
	      break;
	    case 'c':
	      changeDestPerCell = YES;
	      break;
	    case 'm':
	      mflag = YES;
				if ( strncmp(optarg,"-",1) == 0 ) {
					/* Was actually a null option */
						optind = optind -1;
						printf("Got a null option \n");
					/* optarg will be set properly when called */
				}
				else {
        sscanf(optarg, "%lf", &mcastFraction);
				}
	      break;
	    case 'f':
	      fflag = YES;
       	if (strncmp(optarg,"-",1) == 0 ) {
					 optind = optind -1;
					 printf("Got a null option \n");
				}
			 	else {
	     		 fanout = atoi(optarg);
				}
	      break;
	    case '?':
	      fprintf(stderr, "-------------------------------\n");
	      fprintf(stderr, "bursty: Unrecognized option -%c\n", optopt);
	      bursty(TRAFFIC_USAGE);
	      fprintf(stderr, "--------------------------------\n");
	      exit(1);
	    default:
	      break;
	    }
	    
	traffic = (BurstyTraffic *) malloc(sizeof(BurstyTraffic));
	initStat(&traffic->multicastStat, STAT_TYPE_AVERAGE, now);
	enableStat(&traffic->multicastStat);

    if ( pflag == NO)  /* Sundar */
	  printf(" Using default priority levels \n");

	printf("Levels of priority %d\n", priorityLevels);
    traffic->util = malloc(sizeof(double) * priorityLevels); 
	traffic->Mutil = malloc(sizeof(double) * priorityLevels);

  	/* if pflag is there ==> that is uflag is there then rflag must be there. */

	if(rflag &&  !pflag ) {
	  fprintf(stderr,"bursty:\n");
	  fprintf(stderr,"option -p is missing when -r is there \n");
	  exit(1);
	}

	if( !uflag ) 
	  {
		if (priorityLevels == 1) {
	  		printf("    Using default utilization\n");
			traffic->util[0] = utilization;
		    traffic->totalUtil = utilization; /* Bug Fix v2.01-Sundar */
		}
		else {
		  fprintf(stderr,"bursty:\n");
		  fprintf(stderr,"Utilization not specified with multiple priorities \n");
		  exit(1);
		}
     }
   else                    /* parse the utilization string */ 
	{
		if(!rflag &&  pflag ) {
		  fprintf(stderr,"bursty:\n");
		  fprintf(stderr,"option -r is missing\n");
		  exit(1);
		}

		if(!rflag &&  !pflag ) {
		  traffic->util[0] = utilization;
		  traffic->totalUtil = utilization;
		}

		if(rflag &&  pflag ) {

         prioritysum = 0;
		 index = 0; 
         traffic->totalUtil=0;
		 str = strtok(rstring, ":");
		 while(str != NULL) {
		   traffic->util[index] = atof(str);
           prioritysum = prioritysum + traffic->util[index];
		   /* traffic->totalUtil += traffic->util[index]; */
		   index= index + 1;
		   /*   printf("PRESENT priority %d  prioritysum %f utilization %f \n", index, prioritysum, traffic->util[index]); */
		   str = strtok((char *)NULL, ":");
		 }

		if(index != priorityLevels) {
		 fprintf(stderr, "bursty:\n");
		 fprintf(stderr, "Levels of priority and number of elements in utilization arrays dont match \n");
		 exit(1);
		}

        reindex=0;
        traffic->totalUtil=0;
        for (reindex=0; reindex < priorityLevels; reindex++ ) {
            traffic->util[reindex] = (traffic->util[reindex]/prioritysum)*
utilization;
            traffic->totalUtil += traffic->util[reindex];
        }

		if(traffic->totalUtil > 1.0) {
		fprintf(stderr, "bursty:\n");
		fprintf(stderr, "Utilization of priorities sums to greater than 1 \n");
		exit(1);
		}

	  }	
	}

	if (Rflag == NO) {
	  for(index=0; index<priorityLevels; index=index+1)
		printf("priority %d   utilization %f\n", index, traffic->util[index]); 
	} else {
	  for(index=0; index<priorityLevels; index=index+1)
		printf("priority %d   unicast utilization %f\n", index, traffic->util[index] * (1-traffic->mcastFraction)); 
	}
	if( !bflag )
		printf("    Using default mean burst length\n");
	printf("    Burstlength %f\n", burstLength);
	if( !dflag )
	  printf("    Using default burst-by-burst destination switching probability\n");
	printf("    Burst-by-burst destination switching probability %f\n", probDestinationSwitch);
	if( changeDestPerCell )
	  printf("    Destination changed cell by cell.\n");
	else
	  printf("    Destination changed burst by burst.\n");
			

	idleLength = burstLength*(1-utilization)/utilization;
	traffic->a = idleLength/( 1+idleLength );
	traffic->b = burstLength/( 1+burstLength );

	bitmapReset(&traffic->lastMCOutput);
	traffic->lastOutput = 0;
	traffic->lastPriority = 0;
	traffic->changeDestPerCell = changeDestPerCell;
	traffic->probDestinationSwitch = probDestinationSwitch;
	traffic->state = BURSTY_IDLE;
	traffic->numCellsLeft = 0;
	traffic->numCellsGenerated = 0;
	traffic->numOutputCellsGenerated = 0;
	traffic->mcastFraction = 0;

	if( mflag == YES )
	  {
			traffic->mcastFraction = mcastFraction;

	    printf("    Generate UNICAST and MULTICAST cells.\n");
      printf("    Fraction of cells that are multicast %f\n", mcastFraction);
	    if( fflag == YES ) {
		  printf("    Fanout %d\n", fanout);
		  traffic->fanout = fanout;
	    }
	    else {
		  printf("    Using default fanout: U[0,2^%d]\n",
		       aSwitch->numOutputs);
		  traffic->fanout = NONE;
	    }
	  }
	if( Rflag == NO ) {
	    if(priorityLevels == 1) {
			traffic->Mutil[0] = utilization;
	    }
	    else {
		  for(index=0;index<priorityLevels;index++)
			traffic->Mutil[index] = traffic->util[index];
	    }
	}
	else {                       /* parse the utilization string */ 

		if (!mflag) {
			fprintf(stderr, "bursty:\n");
			fprintf(stderr, "-m option not indicated with -R option \n");
			exit(1);
		}
		if (!rflag) {
			fprintf(stderr, "bursty:\n");
			fprintf(stderr, "-r option not indicated with -R option \n");
			exit(1);
		}

	    prioritysum =0;
	    index = 0; 
	    str = strtok(Rstring, ":");
	    while(str != NULL) {
			traffic->Mutil[index] = atof(str);
            prioritysum = prioritysum + traffic->Mutil[index];
			/*			printf("PRESENT priority %d  Mcast prioritysum %f Mcast utilization %f \n", index, prioritysum, traffic->Mutil[index]);  */
			index= index + 1;
			str = strtok((char *)NULL, ":");
	    }
	    if(index != priorityLevels) {
			fprintf(stderr, "bursty:\n");
			fprintf(stderr, "Levels of priority and number of elements in multicast utilization arrays don't match\n");
			exit(1);
	    }

        reindex=0; 
        for (reindex=0; reindex < priorityLevels; reindex++ ) {
			traffic->Mutil[reindex] = (traffic->Mutil[reindex]/prioritysum)*
utilization;
			/*			printf("PRESENT priority %d  prioritysum %f utilization %f \n", reindex, prioritysum, traffic->Mutil[reindex]);  */
        }

	}

	if (Rflag == YES) {
	  for(index=0; index<priorityLevels; index=index+1)
		printf("priority %d  multicast utilization %f\n", index, traffic->Mutil[index] * traffic->mcastFraction); 
	}
    
	traffic->priorityLevels = priorityLevels; /* Sundar */
	traffic->priority_seed[0] = 0x4b82 ^ input ^ globalSeed;
	traffic->priority_seed[1] = 0x7ae2 ^ input ^ globalSeed;
	traffic->priority_seed[2] = 0x3e8a ^ input ^ globalSeed;
	traffic->switch_seed[0] = 0x4282 ^ input ^ globalSeed;
	traffic->switch_seed[1] = 0x7de2 ^ input ^ globalSeed;
	traffic->switch_seed[2] = 0x3e7a ^ input ^ globalSeed;
	traffic->out_seed[0]  = 0x65df ^ input ^ globalSeed;
	traffic->out_seed[1]  = 0x9843 ^ input ^ globalSeed;
	traffic->out_seed[2]  = 0xdd62 ^ input ^ globalSeed;
	traffic->geom_seed_a[0]  = 0xd34e ^ input ^ globalSeed;
	traffic->geom_seed_a[1]  = 0xba91 ^ input ^ globalSeed;
	traffic->geom_seed_a[2]  = 0x0c89 ^ input ^ globalSeed;
	traffic->geom_seed_b[0]  = 0x4ef1 ^ input ^ globalSeed;
	traffic->geom_seed_b[1]  = 0x916a ^ input ^ globalSeed;
	traffic->geom_seed_b[2]  = 0x30c8 ^ input ^ globalSeed;
	traffic->mcast_seed[0]  = 0x128e ^ input ^ globalSeed;
	traffic->mcast_seed[1]  = 0x3c47 ^ input ^ globalSeed;
	traffic->mcast_seed[2]  = 0x20d8 ^ input ^ globalSeed;
	    
	aSwitch->inputBuffer[input]->traffic = (void *) traffic;
	    
	if(debug_traffic)
	  printf("BURSTY: a=%e, b=%e\n", traffic->a, traffic->b);

      /* to calculate the arrival rate of cells for each priority */

	traffic->numMcastCellsperFIFO = (unsigned long int *)
		malloc(aSwitch->numPriorities * sizeof(unsigned long int));
	traffic->numUcastCellsperFIFO = (unsigned long int *)
		malloc(aSwitch->numPriorities * aSwitch->numOutputs * sizeof(unsigned long int));
	for(index=0; index<aSwitch->numPriorities; index++)
		traffic->numMcastCellsperFIFO[index] = 0;
	for(index=0; index<aSwitch->numOutputs * aSwitch->numPriorities; index++)
		traffic->numUcastCellsperFIFO[index] = 0;
    break;
    }
	
    case TRAFFIC_GENERATE:
      {
	if( debug_traffic)
	  printf("        BURSTY: GEN_TRAFFIC: ");

	traffic = (BurstyTraffic *)aSwitch->inputBuffer[input]->traffic;

	if( traffic->numCellsLeft == 0 ) {
	    changeState(traffic, aSwitch->numOutputs);
	}

	traffic->numCellsLeft--;

	if(traffic->state == BURSTY_BUSY) {
	/* Now we have to send a packet as we are in a burst state */

	 if(traffic->cellTypeCurrentBurst == UCAST) {

	/* If we have to change the output port per cell and the random value 
	 also asks us to do so then call random function for new output */

	  if( (traffic->changeDestPerCell)
		&&( erand48(traffic->switch_seed) < traffic->probDestinationSwitch )){
		    traffic->lastOutput = nrand48(traffic->out_seed)%(aSwitch->numOutputs);
			/* Determine what priority the cell has */
			psend = erand48(traffic->priority_seed);  
		    /* erand returns a random number between 0 and 1 */
            psend = psend * traffic->totalUtil;   
			/* This makes psend lesser than or equal to totalUtil */
			total = traffic->totalUtil;
			for(priority=traffic->priorityLevels-1; priority>=0; priority--)
			{
			  total -= traffic->util[priority];
			  if( psend > total ) break;
			}
		    traffic->lastPriority = priority;
	  }
		    
	output = traffic->lastOutput; 
	thispriority = traffic->lastPriority; 
		    
	if(debug_traffic)
	  printf("BURSTY: Input %d has cell for %d\n", 
		 input, output);
	/* aCell = createCell(output, UCAST, DEFAULT_PRIORITY); */
	aCell = createCell(output, UCAST, thispriority);

 	pri = aSwitch->numPriorities * output + thispriority;
	traffic->numUcastCellsperFIFO[pri]++;
	traffic->numOutputCellsGenerated++;

	}

	else { /* cellType of this burst == MULTICAST */ 

	/* If we have to change the output port per cell and the random value 
	   also asks us to do so then call random function for new output */

	  if( (traffic->changeDestPerCell)
	  &&( erand48(traffic->switch_seed) < traffic->probDestinationSwitch)){
		    bitmapReset(&traffic->lastMCOutput);
			/* Determine what priority the cell has */
			psend = erand48(traffic->priority_seed);  /* load seed ? */
		    /* erand returns a random number between 0 and 1 */
            psend = psend * traffic->totalUtil; 
			/* This makes psend lesser than or equal to totalUtil */
			total = traffic->totalUtil;
			for(priority=traffic->priorityLevels-1; priority>=0; priority--)
			{
				total -= traffic->Mutil[priority];
				if( psend > total ) break;
			}
		    traffic->lastPriority = priority;
	 }
          
	mcOutput = traffic->lastMCOutput; 
	thispriority = traffic->lastPriority; 

	aCell = createMulticastCell(&mcOutput,
				    traffic->fanout,
				    traffic->out_seed,
				    aSwitch->numOutputs, thispriority);
	traffic->lastMCOutput = aCell->outputs;
	if(debug_traffic) {
	    printf("Switch %d,Input %d, has new cell for outputs: ",
		   aSwitch->switchNumber, input);
	    bitmapPrint(stdout,&aCell->outputs,aSwitch->numOutputs);
	 }
	updateStat(&traffic->multicastStat,bitmapNumSet(&aCell->outputs), now);

	traffic->numMcastCellsperFIFO[thispriority]++;
	traffic->numOutputCellsGenerated += traffic->fanout;
	}
	
	traffic->numCellsGenerated++;
	/* Execute the switch inputAction to accept cell */
	(aSwitch->inputAction)(INPUTACTION_RECEIVE, aSwitch, input, aCell,
		NULL, NULL, NULL, NULL);
		
    }
	else {
	    if(debug_traffic)
	      printf("None\n");
	}
	    
	break;
    }
    case REPORT_TRAFFIC_STATS:
    {
	traffic = (BurstyTraffic *)aSwitch->inputBuffer[input]->traffic;
	printf("  %d  bursty  %f\n", input, (double)traffic->numCellsGenerated/(double)now);

	/* Print arrival rate for Unicast Queue (0,0) */
	if(input == 0) {
	  for(pri=0;pri<aSwitch->numPriorities;pri++)
		printf("  (0,0)  %3d\t%f\n",pri,(double)traffic->numUcastCellsperFIFO[pri]/now);
		if(traffic->mcastFraction > 0.0 ) {
		   printf("MCAST input cells:\n");
		   for(pri=0;pri<aSwitch->numPriorities;pri++)
		   printf("  (0)  %3d\t%f\n",pri,(double)traffic->numMcastCellsperFIFO[pri]/now);
		   printf("MCAST output cells:\n");
		   for(pri=0;pri<aSwitch->numPriorities;pri++)
				printf("  (0)  %3d\t%f\n",pri,(double)returnAvgStat(&traffic->multicastStat)
			   *traffic->numMcastCellsperFIFO[pri]/now);
		}
	}
 
	if(traffic->mcastFraction > 0.0)
	  printStat(stdout, "Fanout stats:", &traffic->multicastStat);
	break;
    }
	
    }
    
  return(CONTINUE_SIMULATION);
}


static void
changeState(traffic, numOutputs)
  BurstyTraffic *traffic;
int numOutputs;
{
  int burstSize;
  double newpsend;
  int  newpriority;
  double newtotal;

  burstSize=0;
  while(burstSize == 0) {
      /* Determine length of busy/idle period */

      if(traffic->state == BURSTY_BUSY) {
	    burstSize = geometric(traffic->a, traffic->geom_seed_a);
      }
      else {
	    burstSize = geometric(traffic->b, traffic->geom_seed_b);
	  }

      if( debug_traffic)
	    printf("Chose period length of %d\n", burstSize);
        traffic->state = toggle_state(traffic->state);
  }

  traffic->numCellsLeft = burstSize;
  if( traffic->state == BURSTY_BUSY ) {
      /* New burst is starting. Determine whether ucast or mcast */
      if( erand48(traffic->mcast_seed) < traffic->mcastFraction ) {
           traffic->cellTypeCurrentBurst = MCAST;
      }
      else {
           traffic->cellTypeCurrentBurst = UCAST;
      }

  /* 
   * Check to see whether this next burst will have the same or a 
   * different destination address  as the last.
   * If erand48(switch_seed) < probDestinationSwitch then choose a new
   * destination else keep the same destination.
   */

  if((! traffic->changeDestPerCell) &&
	 (erand48(traffic->switch_seed) < traffic->probDestinationSwitch )) {
	  if(traffic->cellTypeCurrentBurst == UCAST) {
	    	traffic->lastOutput = nrand48(traffic->out_seed)%(numOutputs);
			/* Determine what priority the cell has */
			newpsend = erand48(traffic->priority_seed);   
		    /* erand returns a random number between 0 and 1 */
        	newpsend = newpsend * traffic->totalUtil; 
			/* This makes psend lesser than or equal to totalUtil */
			newtotal = traffic->totalUtil;
			for(newpriority=traffic->priorityLevels-1; newpriority>=0; newpriority--) {
				newtotal -= traffic->util[newpriority];
				if( newpsend > newtotal ) break;
			}
		   traffic->lastPriority = newpriority;
	  }
	  else
	    {
	      traffic->lastOutput = 0;
		  /* Determine what priority the cell has */
		  newpsend = erand48(traffic->priority_seed);  
	      /* erand returns a random number between 0 and 1 */
          newpsend = newpsend * traffic->totalUtil;   
		  /* This makes psend lesser than or equal to totalUtil */
		  newtotal = traffic->totalUtil;
		  for(newpriority=traffic->priorityLevels-1; newpriority>=0; newpriority--) {
			newtotal -= traffic->Mutil[newpriority];
			if( newpsend > newtotal ) break;
		  }
		  traffic->lastPriority = newpriority;
	    }
	}
  }
}
	

static int
geometric(p, seed)
  double p;
unsigned short *seed;
{
  double q = 1-p;
  double term = q;
  double sum = q;
  double prevsum = q;
  int x = 0;
  double r;

  /* term is the last qp^x */

  r = erand48(seed);

  if(r <= sum)
    return 0;

  while(r > sum)
    {
      x++;
      prevsum = sum;
      term = term*p;
      sum += term;
    }

  /*
    error = (r - prevsum)/(sum - prevsum);
    errp = erand48(seed);
    if(error < errp)
    return (x - 1);
    else
    */

  return x;
}
	
