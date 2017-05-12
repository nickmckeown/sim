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

typedef struct {
  double *util; 	/* utilization ratio for priorities */
  double *Mutil;    
  double *utilization; /* Array of utilizations: 1 per output */
  double totalUtilization; /* Sum of utilizations for this input */
  double totalUtil; /* Sum of utilizations for this input */
  int fanout;
  double mcastFraction;
  long int numCellsGenerated; /* Total number of cells generated */
  long int numOutputCellsGenerated; 
			  /* Total number of outputs cells generated */
  unsigned long int * numUcastCellsperFIFO;
  unsigned long int * numMcastCellsperFIFO;
  Stat multicastStat;
  unsigned short int load_seed[3];
  unsigned short int mcast_seed[3];
  unsigned short int out_seed[3];
  unsigned short int switch_seed[3];
  int priorityLevels;
} BernoulliTraffic;

/*
	Arguments: (must be numOutputs entries in array)
	-u 0.5 0.3 0.2 .... 0.5
*/

int
bernoulli_iid_nonuniform(action, aSwitch, input, argc, argv)
  TrafficAction action;
Switch *aSwitch;
int input;
int argc;
char **argv;
{

  BernoulliTraffic *traffic;
  Cell *aCell;
  double psend;
  int thispriority;
  int output;
  int fanout=1;
  int uflag=NO;
  int mflag=NO;
  int Rflag=NO;
  int fflag=NO;
  int pflag=NO;
  int rflag=NO;

  char * rstring = ""; /* The string of utilization for each priority */
  char * Rstring = "";     /* the string of utilization for priorities */
  char * str = ""; 

  int pri;
  int priority;
  double total;
  int index;

  int reindex = 0;
  double prioritysum = 0;
  int cellType;

  int priorityLevels=1;
  static double utilization=0.5;
  double mcastFraction=0.5;

  switch( action )
    {	
    case TRAFFIC_USAGE:
      fprintf(stderr, "Options for \"bernoulli_iid_nonuniform\" traffic model:\n");
      fprintf(stderr, "    -u u0 u1 u2 u3 ... uN-1. Default: %f\n", utilization);
      fprintf(stderr, "    -m fraction (in range [0-1]) of cells that are multicast. Default: %f\n", mcastFraction);
      fprintf(stderr, "    -R r0:r1:r2:r3... rP-1, multicast utilization tatio per priority level, defaults to unicast utilization per priority level if not specified\n");
      fprintf(stderr, "    -f fixed fanout for multicast cells. If not set, fanout U[0,2^N]\n");
      fprintf(stderr, "    -p number of prioritylevels of traffic generated. Default: %d\n", priorityLevels);
      fprintf(stderr, "    -r r0:r1:r2:r3,... rP-1, unicast utilization ratio per priority level,where P is the number of priority levels \n");
      break;

    case TRAFFIC_INIT:
      {
	int c;
	extern int optind;
	extern int optopt;
	extern int opterr;
	extern char *optarg;

	optind=1;
	opterr=0;

	if( debug_traffic)
	  printf("Traffic init for switch %d input %d\n", aSwitch->switchNumber, input);

	traffic = (BernoulliTraffic *) malloc(sizeof(BernoulliTraffic));
	traffic->utilization = (double *) 
	  malloc(aSwitch->numOutputs*sizeof(double));
	traffic->mcastFraction = 0;
	traffic->numCellsGenerated = 0;
	traffic->numOutputCellsGenerated = 0;
    traffic->load_seed[0] = 0x1234 ^ input ^ globalSeed;
	traffic->load_seed[1] = 0xde91 ^ input ^ globalSeed;
	traffic->load_seed[2] = 0x4184 ^ input ^ globalSeed;
	traffic->mcast_seed[0]   = 0xa561 ^ input ^ globalSeed; 
									/* whether cell is u/m */
	traffic->mcast_seed[1]   = 0xa123 ^ input ^ globalSeed;
	traffic->mcast_seed[2]   = 0xa736 ^ input ^ globalSeed;
	traffic->out_seed[0]  = 0x3234 ^ input ^ globalSeed;
	traffic->out_seed[1]  = 0x287d ^ input ^ globalSeed;
	traffic->out_seed[2]  = 0xbd39 ^ input ^ globalSeed;
	traffic->switch_seed[0] = 0x4282 ^ input ^ globalSeed;
	traffic->switch_seed[1] = 0x7de2 ^ input ^ globalSeed;
	traffic->switch_seed[2] = 0x3e7a ^ input ^ globalSeed;

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

	initStat(&traffic->multicastStat, STAT_TYPE_AVERAGE, now);
	enableStat(&traffic->multicastStat);

	while( (c = getopt(argc, argv, "u:m:f:p:r:R:")) != EOF) {
	  switch(c)
            {
	    case 'b':
	       break;
	    case 'u':
	   	   uflag = YES;
	     	/* optind points to next arg string */
	      	traffic->totalUtilization = 0; 
	      	for(output=0; output<aSwitch->numOutputs; output++) {
		  		sscanf(argv[output+optind-1], "%lf",
			 		&traffic->utilization[output]);
		  		printf("      Output: %d utilization: %f\n",
					output, traffic->utilization[output]);
		  		/* Actually keep partial sum of utilizations */
		  		if(output>0) {
		    		traffic->utilization[output] += 
						traffic->utilization[output-1];
				}
		 	 	traffic->totalUtilization = traffic->utilization[output];
			}

            optind = optind + aSwitch->numOutputs - 1; 

			/* Bug Fix v2.01 - optind must be incremented.! Error carried 
               over from 1.x SIM versions */

	     	if( traffic->totalUtilization > 1.0 ) {
		  		fprintf(stderr, "Total utilization of input %d exceeds 1.0!\n", input);
		  		exit(1);
			}
	      	break;
	    case 'm':
			mflag=YES;
      if (strncmp(optarg,"-",1) == 0 ) {
				 /* Was actually a null option */
			 optind = optind -1;
			 printf("Got a null option \n");
			 /* optarg will be set properly when called */
			 }
			 else {
		 		sscanf(optarg, "%lf", &mcastFraction);
			 }
		  	printf("mcastFraction is %f \n", mcastFraction);
	     	break;
		case 'r':
		 	/* sscanf(optarg, "%lf", &utilization); */
		 	/* uflag = 1; */
			rflag = YES;
	    	rstring = optarg; /* Sundar */
		  	printf("rstring is %s \n", rstring);
			break;
	    case 'R':
	      Rflag = YES;
	      Rstring = optarg;
	      break;
	  case 'p':                 /* Sundar */
			pflag = YES;
	    priorityLevels = atoi(optarg);
			if ( priorityLevels > aSwitch->numPriorities) {
        fprintf(stderr, "input %d: Number of traffic priorities %d is greater than the number of switch priorities %d", input, priorityLevels, aSwitch->numPriorities);
			exit(1);
			}
		  printf("priority levels is %d \n", priorityLevels);
			break;
		case 'f':
		  fflag = YES;
			if (strncmp(optarg,"-",1) == 0) {
					optind = optind -1;
					printf("Got a null option \n");
			}
			else {
				fanout = atoi(optarg);
			}
		  printf("fanout is %d \n", fanout);
			if(fanout>aSwitch->numOutputs)
			{
				fprintf(stderr, "Fanout %d too big\n", fanout);
			  	exit(1);
			}
			break;
	    case '?':
	    	fprintf(stderr, "-------------------------------\n");
	    	fprintf(stderr, "bernoulli_iid_nonuniform: Unrecognized option -%c\n", optopt);
	    	bernoulli_iid_nonuniform(TRAFFIC_USAGE);
	    	fprintf(stderr, "--------------------------------\n");
	    	exit(1);
	    	default:
	    	break;
	    }

	} /* End of while */

	if( !uflag ) {
	    fprintf(stderr, "MUST specify utilizations for nonuniform traffic!\n");
	    exit(1);
	}

	if ( pflag == NO)  /* Sundar */
		printf(" Using default priority levels \n");
	printf("Levels of priority %d\n", priorityLevels);

	traffic->util = malloc(sizeof(double) * priorityLevels); /* Sundar */
	traffic->Mutil = malloc(sizeof(double) * priorityLevels);

	if(rflag &&  !pflag ) {
		fprintf(stderr,"bernoulli_non_uniform:\n");
		fprintf(stderr,"option -p is missing when -r is there \n");
		exit(1);
	}

	if(!rflag &&  pflag ) {
		fprintf(stderr,"bernoulli_non_uniform:\n");
		fprintf(stderr,"option -r is missing\n");
  		exit(1);
	 }

  	if(!rflag &&  !pflag ) { /* Priority Levels is then 1 */
		traffic->util[0] = traffic->totalUtilization;
		traffic->totalUtil = traffic->totalUtilization; 
																					/* Bug fix v2.01 - Sundar */
		}

  	if(rflag &&  pflag ) {
																	
    prioritysum=0;
		index = 0; traffic->totalUtil=0;
		str = strtok(rstring, ":");
    	while(str != NULL) {
			traffic->util[index] = atof(str);
            prioritysum = prioritysum + traffic->util[index];
	 		/* traffic->totalUtil += traffic->util[index]; */
			index= index + 1;
      /* printf("PRESENT priority %d  prioritysum %f utilization %f
\n", index, prioritysum, traffic->util[index]); */
	 		str = strtok((char *)NULL, ":");
	  	}

	  	if(index != priorityLevels) {
	 		fprintf(stderr, "bernoulli_iid_nonuniform:\n");
	  		fprintf(stderr, "Levels of priority and number of elements in utilization arrays dont match \n");
		 	exit(1);
	   	}

        reindex=0;
        traffic->totalUtil=0;
        for (reindex=0; reindex < priorityLevels; reindex++ ) {
  			traffic->util[reindex] = (traffic->util[reindex]/prioritysum)*
traffic->totalUtilization;
            traffic->totalUtil += traffic->util[reindex];
        }
 
		if(traffic->totalUtil > 1.0) {
			 fprintf(stderr, "bernoulli_non_uniform:\n");
			 fprintf(stderr, "Utilization of priorities sums to greater than 1 \n");
			 exit(1);
		}

	}

	if (Rflag == NO) {
	  for(index=0; index<priorityLevels; index=index+1)
		printf("priority %d   utilization %f\n", index, traffic->util[index]); 
	} else {
	  for(index=0; index<priorityLevels; index=index+1)
		printf("priority %d  unicast utilization %f\n", index, traffic->util[index] * (1-traffic->mcastFraction)); 
	}



	if( mflag == YES ) {
	  	printf("    Generate UNICAST and MULTICAST cells.\n");
	  	traffic->mcastFraction = mcastFraction;
		if( fflag == YES ) {
			printf("    Fanout %d\n", fanout);
			traffic->fanout = fanout;
		}
		else {
			printf("    Using default fanout: U[0,2^%d]\n",aSwitch->numOutputs);
			traffic->fanout = NONE;
		}
		printf("    Fraction of cells that are multicast %f\n", mcastFraction);
	}

	if( Rflag == NO ) {
	    if(priorityLevels == 1) {
			traffic->Mutil[0] = traffic->totalUtilization;
	    }
	    else {
		  for(index=0;index<priorityLevels;index++)
			traffic->Mutil[index] = traffic->util[index];
	    }
	}
	else {                       /* parse the utilization string */ 

		if (!mflag) {
			fprintf(stderr, "bernoulli_iid_nonuniform:\n");
			fprintf(stderr, "-m option not indicated with -R option \n");
			exit(1);
		}
		if (!rflag) {
			fprintf(stderr, "bernoulli_iid_nonuniform:\n");
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
			fprintf(stderr, "bernoulli_iid_nonuniform:\n");
			fprintf(stderr, "Levels of priority and number of elements in multicast utilization arrays don't match\n");
			exit(1);
	    }

        reindex=0; 
        for (reindex=0; reindex < priorityLevels; reindex++ ) {
			traffic->Mutil[reindex] = (traffic->Mutil[reindex]/prioritysum)*
traffic->totalUtilization;
			/*			printf("PRESENT priority %d  prioritysum %f utilization %f \n", reindex, prioritysum, traffic->Mutil[reindex]);  */
        }

	}
	if (Rflag == YES) {
	  for(index=0; index<priorityLevels; index=index+1)
		printf("priority %d  multicast utilization %f\n", index, traffic->Mutil[index] * traffic->mcastFraction); 
	}
 	traffic->priorityLevels = priorityLevels; /* Sundar */
	break;
    }

    case TRAFFIC_GENERATE:
      {
	if( debug_traffic)
	  printf("		GEN_TRAFFIC: ");
	traffic = (BernoulliTraffic *)aSwitch->inputBuffer[input]->traffic;
	/* psend = drand48(); */
	psend = erand48(traffic->load_seed);

	/* Decide whether or not to generate a new cell */
	if( psend < traffic->totalUtilization) {
	    for(output=0; output<(aSwitch->numOutputs)-1; output++) {
		if(debug_traffic)
			printf("psend %f, utilization: %f\n",
			 psend, traffic->utilization[output]);
		if( psend < traffic->utilization[output] )
			break;
	    }

		/* Decide if cell is unicast or multicast */
		psend = erand48(traffic->mcast_seed);
		if( psend < traffic->mcastFraction ) {
			cellType = MCAST;
		}
		else {
			cellType = UCAST;
		}

		if (cellType == UCAST) {

	   		if(debug_traffic)
	     		printf("Switch %d,  Input %d, has new cell for %d\n", aSwitch->switchNumber, input, output);

				psend = erand48(traffic->switch_seed);   /* ?? switch seed  */
				/* erand returns a random number between 0 and 1 */
	 			psend = psend * traffic->totalUtil;
				/* This makes psend lesser than or equal to totalUtil */
				total = traffic->totalUtil;
				for(priority=traffic->priorityLevels-1; priority>=0; priority--)
				{
					total -= traffic->util[priority];
					if( psend > total ) break;
				}
  				thispriority = priority;

			    /* aCell = createCell(output, UCAST, DEFAULT_PRIORITY);	 */
	    		aCell = createCell(output, UCAST, thispriority);	
	    		traffic->numCellsGenerated++;

				pri = aSwitch->numPriorities * output + priority;
				traffic->numUcastCellsperFIFO[pri]++;
				traffic->numOutputCellsGenerated++;

		}  /* Cell type is UNICAST */

		else {
			Bitmap mcOutput;
			bitmapReset(&mcOutput);
			psend = erand48(traffic->switch_seed);   /* ?? switch seed  */
		    /* erand returns a random number between 0 and 1 */
	 		psend = psend * traffic->totalUtil;
			/* This makes psend lesser than or equal to totalUtil */
			total = traffic->totalUtil;
			for(priority=traffic->priorityLevels-1; priority>=0; priority--) {
				total -= traffic->Mutil[priority];
			if( psend > total ) break;
			}

  			thispriority = priority;
			aCell = createMulticastCell(&mcOutput,
							traffic->fanout,
							traffic->out_seed,
							aSwitch->numOutputs, thispriority);
			if(debug_traffic) {
				printf("Switch %d,Input %d,has new cell priority %d for outputs: ", aSwitch->switchNumber, input, thispriority);
				bitmapPrint(stdout,&aCell->outputs,aSwitch->numOutputs);
			}
			fanout = bitmapNumSet(&aCell->outputs);
			updateStat(&traffic->multicastStat, fanout, now);
			traffic->numMcastCellsperFIFO[thispriority]++;
			traffic->numOutputCellsGenerated += fanout;


			traffic->numCellsGenerated++;
			/* Bug Fix v2.01 - Sundar */
			/* This statement was intially outside the loop 
			   and was thus being incremented twice for a unicast 
               call */

		} /* Cell type is MULTICAST */

	    /* Execute the switch inputAction to accept cell */
		/* YOUNGMI: BUG FIX */
		/* ?? Sundar - check this, why so many parameters ? */
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
	traffic = (BernoulliTraffic *)aSwitch->inputBuffer[input]->traffic;
	printf("  %d  bernoulli_iid_nonuniform  %f\n", input, 
	       (double)traffic->numCellsGenerated/(double)now);

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
