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
#include "sim.h"
#include "traffic.h"
#include "inputAction.h"

/* 
*	Fanout is the number of outputs of a multicast cell.
*	mcastfraction is a number between 0,1 which is the fraction
*	of multicast cells.
*	traffic is being defined as bernoulli traffic now. Traffic 
*	is the main structure for the switch.
*	Each priority level in the traffic has a utitization value.
*	uflag is the utilization flag and is NOT the UNICAST flag.
*	totalUtil is the sum of the utilizations of each priority 
*	level.
*/


typedef struct {
  double *util;    
  double *Mutil;    
  double totalUtil; 
  int fanout;
  double mcastFraction;
  unsigned long int numCellsGenerated;
  unsigned long int numOutputCellsGenerated;
  unsigned long int * numUcastCellsperFIFO;
  unsigned long int * numMcastCellsperFIFO;
  unsigned short int load_seed[3];
  unsigned short int out_seed[3];
  unsigned short int mcast_seed[3];
  int priorityLevels;
  Stat multicastStat;
} BernoulliTraffic;

int
bernoulli_iid_uniform(action, aSwitch, input, argc, argv)
  TrafficAction action;
Switch *aSwitch;
int input;
int argc;
char **argv;
{

  BernoulliTraffic *traffic;
  Cell *aCell;
  double psend;
  double pmulti;
  int output;
  int fanout=1;
  int uflag=NO;
  int rflag=NO;
  int mflag=NO;
  int Rflag=NO;
  int fflag=NO;
  int pflag=NO;           
  
  int pri, priority;           /* TL */
  char * ustring = "";     /* the string of utilization */
  char * rstring = "";     /* the string of utilization for priorities */
  char * Rstring = "";     /* the string of utilization for priorities */
  char * str;
  double total;
  int index;
  int reindex =0;
  double prioritysum =0;
  int cellType;

  int priorityLevels=1;
  double utilization=0.5;
  double mcastFraction=0.5;

  switch( action )
    {	
    case TRAFFIC_USAGE:
      fprintf(stderr, "Options for bernoulli_iid_uniform traffic model:\n ");
      fprintf(stderr, "    -u utilization. Default: %f\n", utilization);
      fprintf(stderr, "    -r r0:r1:r2:r3... rP-1, unicast utilization ratio per priority level, where P is the number of priority levels \n");
      fprintf(stderr, "    -p number of prioritylevels of traffic generated. Default: %d\n", priorityLevels);
      fprintf(stderr, "    -m fraction (in range [0-1]) of cells that are multicast. Default: %f\n", mcastFraction);
      fprintf(stderr, "    -R r0:r1:r2:r3... rP-1, multicast utilization per priority level, defaults to unicast utilization per priority level if not specified\n");
      fprintf(stderr, "    -f fixed fanout for multicast cells. If not set, fanout U[0,2^N]\n");
      break;
    case TRAFFIC_INIT:
      {
	int c;
	int oldoptind;
	extern int optind;
	extern int optopt;
	extern int opterr;
	extern char *optarg;

	optind=1;
	oldoptind=1;
	opterr=0;

	if( debug_traffic)
	  printf("Traffic init for switch %d input %d\n", aSwitch->switchNumber, input);

	while( (c = getopt(argc, argv, "u:m:f:p:r:R:")) != EOF) {

	  switch(c)
            {
	    case 'u':
	      uflag = YES;
	      ustring = optarg;       /* TL */
	      sscanf(optarg, "%lf", &utilization);
	      break;
	    case 'r':
	      rflag = YES;
	      rstring = optarg;       /* TL */
	      break;
	    case 'R':
	      Rflag = YES;
	      Rstring = optarg;
	      break;
	    case 'p':                 /* TL */
	      pflag = YES;
	      priorityLevels = atoi(optarg);

				if ( priorityLevels > aSwitch->numPriorities) {
        	fprintf(stderr, "input %d: Number of traffic priorities %d is greater than the number of switch priorities %d", input, priorityLevels, aSwitch->numPriorities);
		exit(1);
				}
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
	      mcastFraction = atof(optarg);
				}
				/* printf("MCAST FRACTION IS %lf \n", mcastFraction); */
	      break;
	    case 'f':
	      		fflag = YES;
				if ( strncmp(optarg,"-",1) == 0 ) { 
					optind = optind -1; 
				  	printf("Got a null option \n");
				}
				else {
	      			fanout = atoi(optarg);
				}
	      		if(fanout>aSwitch->numOutputs)
				{
		  			fprintf(stderr, "Fanout %d too big\n", fanout);
		  			exit(1);
				}
	      		break;

	    case '?':
	      fprintf(stderr, "-------------------------------\n");
	      fprintf(stderr, "bernoulli_iid_uniform: Unrecognized option -%c\n", optopt);
	      bernoulli_iid_uniform(TRAFFIC_USAGE);
	      fprintf(stderr, "--------------------------------\n");
	      exit(1);
	    default:
			 printf("Reached to default case \n");	
	      break;
	    }
			oldoptind = optind;
		}

	/* Sundar: You have to initialize the whole traffic structure in 
	 *  the traffic INIT case.
     */

	traffic = (BernoulliTraffic *) malloc(sizeof(BernoulliTraffic));
	traffic->mcastFraction = 0;

	initStat(&traffic->multicastStat, STAT_TYPE_AVERAGE, now);
	enableStat(&traffic->multicastStat);

	if( pflag == NO )  { 
		/* TL */
	    printf("  Using default priority levels\n");	    
	    printf("  Using default priority levels\n");	    
	}
	printf("    Levels of priority %d\n", priorityLevels);

	/* 
	 * Sundar: In the traffic structure the util member exists
	 *  for each priority level. 
	 */

	traffic->util = malloc(sizeof(double) * priorityLevels);  /* TL */
	traffic->Mutil = malloc(sizeof(double) * priorityLevels);

	/* In this logic the uflag is only going to be used to validate that
	 * the utilization values with the priorities sum up to the 
	 * value indicated in the u flag.
	 */	

	if( rflag == NO ) {
	    if(priorityLevels == 1) {
	    	if(!uflag) {
				printf("    Using default utilization\n");
			}
			traffic->util[0] = utilization;
	        traffic->totalUtil= utilization;
	    }
	    else {
			fprintf(stderr, "bernoulli_iid_uniform:\n");
			fprintf(stderr, "Utilization not specified with multiple priorities\n");
			exit(1);
	    }
	}
	else {                       /* parse the utilization string */ 

		if (!uflag) {
			fprintf(stderr, "bernoulli_iid_uniform:\n");
			fprintf(stderr, "-u option not indicated with -r option \n");
			exit(1);
		}

	    prioritysum =0;
	    index = 0; 
        traffic->totalUtil=0;
	    str = strtok(rstring, ":");
	    while(str != NULL) {
			traffic->util[index] = atof(str);
            prioritysum = prioritysum + traffic->util[index];
			/* traffic->totalUtil += traffic->util[index]; */
			/*			printf("PRESENT priority %d  prioritysum %f utilization %f \n", index, prioritysum, traffic->util[index]);  */
			index= index + 1;
			str = strtok((char *)NULL, ":");
	    }
	    if(index != priorityLevels) {
			fprintf(stderr, "bernoulli_iid_uniform:\n");
			fprintf(stderr, "Levels of priority and number of elements in utilization arrays don't match\n");
			exit(1);
	    }

        reindex=0; 
        traffic->totalUtil=0;
        for (reindex=0; reindex < priorityLevels; reindex++ ) {
			traffic->util[reindex] = (traffic->util[reindex]/prioritysum)*
utilization;
			/*			printf("PRESENT priority %d  prioritysum %f utilization %f \n", reindex, prioritysum, traffic->util[reindex]);  */
			traffic->totalUtil += traffic->util[reindex];
        }

		if ( traffic->totalUtil > 1.0 ) {
		fprintf(stderr, "Total Utilization of input %d exceeds 1.0!\n",
		input);
		exit(1);
		}

	}
	if (Rflag == NO) {
	  for(index=0; index<priorityLevels; index=index+1)
		printf("priority %d  utilization %f\n", index, traffic->util[index]); 
	} else {
	  for(index=0; index<priorityLevels; index=index+1)
		printf("priority %d  unicast utilization %f\n", index, traffic->util[index] * (1-traffic->mcastFraction)); 
	}
	    

	if( mflag == YES )
	  {
	    printf("    Generate UNICAST and MULTICAST cells.\n");
		traffic->mcastFraction = mcastFraction;
	    if( fflag == YES ) {
			printf("    Fanout %d\n", fanout);
			traffic->fanout = fanout;
	    }
	    else {
			printf("    Using default fanout: U[0,2^%d]\n", 
		       aSwitch->numOutputs);
			traffic->fanout = NONE;
	    }
		printf("    Fraction of cells that are multicast %f\n", mcastFraction);
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
			fprintf(stderr, "bernoulli_iid_uniform:\n");
			fprintf(stderr, "-m option not indicated with -R option \n");
			exit(1);
		}
		if (!rflag) {
			fprintf(stderr, "bernoulli_iid_uniform:\n");
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
			fprintf(stderr, "bernoulli_iid_uniform:\n");
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
	traffic->numCellsGenerated = 0;
	traffic->numOutputCellsGenerated = 0;
	traffic->priorityLevels = priorityLevels;   /* TL */
	traffic->load_seed[0] = 0x1234 ^ input ^ globalSeed;
	traffic->load_seed[1] = 0xde91 ^ input ^ globalSeed;
	traffic->load_seed[2] = 0x4184 ^ input ^ globalSeed;
	traffic->out_seed[0]  = 0x3234 ^ input ^ globalSeed;
	traffic->out_seed[1]  = 0x287d ^ input ^ globalSeed;
	traffic->out_seed[2]  = 0xbd39 ^ input ^ globalSeed;
	traffic->mcast_seed[0]   = 0xa561 ^ input ^ globalSeed; 
														/* whether cell is u/m */
	traffic->mcast_seed[1]   = 0xa123 ^ input ^ globalSeed; 
	traffic->mcast_seed[2]   = 0xa736 ^ input ^ globalSeed; 

	aSwitch->inputBuffer[input]->traffic = (void *) traffic;
	
	/* To calculate the arrival rate of cells for each priority */
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

	traffic = (BernoulliTraffic *)aSwitch->inputBuffer[input]->traffic;

	if(debug_traffic)
	  printf("		GEN_TRAFFIC " );

	/* Determine whether or not to send cell */
	psend = erand48(traffic->load_seed);
	if( psend < traffic->totalUtil ) {

		/* Decide if cell is unicast or multicast */
		pmulti = erand48(traffic->mcast_seed);
		if( pmulti < traffic->mcastFraction ) {
			cellType = MCAST;
			total = traffic->totalUtil;
			for(priority=traffic->priorityLevels-1; priority>=0; priority--) {
			  total -= traffic->Mutil[priority];
			  if( psend > total ) break;
		}
		}
		else
		{
			cellType = UCAST;
			total = traffic->totalUtil;
			for(priority=traffic->priorityLevels-1; priority>=0; priority--) {
			  total -= traffic->util[priority];
			  if( psend > total ) break;
		}
		}



		if(cellType == UCAST) {
			output = nrand48(traffic->out_seed)%(aSwitch->numOutputs);
			if(debug_traffic)
		  	printf("Switch %d, Input %d, new UCAST cell for %d, priority %d\n",
				 aSwitch->switchNumber, input,  output, priority);
			aCell = createCell(output, UCAST, priority);

	 /* Output is the output port on which the cell  wants to go */

			pri= aSwitch->numPriorities * output + priority;
			traffic->numUcastCellsperFIFO[pri]++;
	        traffic->numOutputCellsGenerated++;
	   	}
	    else /* MULTICAST */ {
			Bitmap mcOutput;

			bitmapReset(&mcOutput);
			aCell = createMulticastCell(&mcOutput, 
					    traffic->fanout, 
					    traffic->out_seed,
					    aSwitch->numOutputs, priority);
			if(debug_traffic) {
		    	printf("Switch %d,Input %d,has new cell priority %d for outputs: ", 
			   	aSwitch->switchNumber, input, priority);
		    	bitmapPrint(stdout,&aCell->outputs,aSwitch->numOutputs);
		  	}
            fanout = bitmapNumSet(&aCell->outputs);
			updateStat(&traffic->multicastStat, fanout, now);
			traffic->numMcastCellsperFIFO[priority]++;
	        traffic->numOutputCellsGenerated += fanout;
	   }
	    
	    traffic->numCellsGenerated++;

	    /* Execute the switch inputAction to accept cell */
	    (aSwitch->inputAction)(INPUTACTION_RECEIVE,aSwitch,input,aCell);
	  }
	else {
	    if(debug_traffic)
	        printf("None\n");
	}
	
	break;
    }
    case REPORT_TRAFFIC_STATS:
      {
	traffic = (BernoulliTraffic *)aSwitch->inputBuffer[input]->traffic;
	printf("  %d  bernoulli_iid_uniform  IN: %f OUT: %f\n", input,
	       (double)traffic->numCellsGenerated/(double)now, (double)traffic->numOutputCellsGenerated/(double)now);

	/* Print arrival rate for Unicast Queue (0,0) */
	if(input == 0) {
	  for(pri=0;pri<aSwitch->numPriorities;pri++)
	     printf("  (0,0)  %3d\t%f\n",pri,(double)traffic->numUcastCellsperFIFO[pri]/now);
	     
	  if(traffic->mcastFraction > 0.0 )
          {
		printf("MCAST input cells:\n");
		for(pri=0;pri<aSwitch->numPriorities;pri++)
			printf("  (0)  %3d\t%f\n",pri,(double)traffic->numMcastCellsperFIFO[pri]/now);
		printf("MCAST output cells:\n");
		for(pri=0;pri<aSwitch->numPriorities;pri++)
			printf("  (0)  %3d\t%f\n",pri,(double)returnAvgStat(&traffic->multicastStat)*traffic->numMcastCellsperFIFO[pri]/now);
          }
	}

	if(traffic->mcastFraction > 0.0)
	  printStat(stdout, "Fanout stats:", &traffic->multicastStat);
	break;
      }
    }

  return(CONTINUE_SIMULATION);
}
