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

#include <unistd.h>
#include <time.h>
#include <math.h>
#include "sim.h"

#include "algorithm.h"
#include "fabric.h"
#include "traffic.h"
#include "inputAction.h"
#include "outputAction.h"


#ifdef _SIM_
#include "graph.h" // SG
#endif // _SIM_


#define DEFAULT_SIMULATION_LENGTH 100
#define STOP_THRESHOLD 0.0001 
#define NUM_TIMES_MUST_MEET_STOP_CONDITION 10
#define CHECK_STOP (simulationLength/50+1)

#define CHECK_MEMORY_USAGE (simulationLength/50+1)
#define MAX_ALLOWED_MEMORY_USAGE 20000000

/*** static char ident[] = "@(#)ATM Switch Simulator Revision 2.31"; ***/


/**********************static functions **************/
static time_t timeDiff();
static void headerInformation();
static int memoryUsage();
static int checkStopCondition();

/******functions defined here and also used elsewhere***********/

void FatalError();

/******functions defined elsewhere and used here***********/
extern void parseConfigurationFile();
extern void switchStats();/* from switchStats.c */
extern void burstStats(); /* from switchStats.c */
extern void printInputFifoLevels(); /* from debug.c */
extern void printMatrix();/* from debug.c */
extern int getAllocAmount();/* from circBuffer.c */


/*****************************************************/
/********** Universal time measured in cells. ********/
/*****************************************************/
long now=0;
long resetStatsTime=NONE;
unsigned long globalSeed=0;

#ifdef _SIM_
struct GRAPH graphTool; // SG
#endif // _SIM_


Switch **switches;
int numSwitches=1;
long simulationLength=DEFAULT_SIMULATION_LENGTH; 

/**************************************************/
/*************main function ***********************/
/**************************************************/
int main(argc, argv)
  int argc;
char **argv;
{
  Switch *aSwitch;
  int switchNumber;
  int input,output;

  char c;
  int i;
  extern char *optarg;
  extern int optind;
  extern int opterr;

  int simStopped=CONTINUE_SIMULATION;
  int fflag=0;
  int pflag=0;
  int checkPeriod=0;
  char *configFilename=(char *) NULL;

  /* XXX Temp for varying internal speeds */
  int trafficPeriod=1;
  int fabricPeriod=1;
  int outputPeriod=1;

#ifdef _SIM_

  /************************************************************/
    /* SIMGRAPH related global variables (SG)                   */
    /************************************************************/

    int simGraphSocketPeriod=10; // Amr
    int simGraphEnabled = 0; // Amr
    int graphTCPPort = DEFAULT_TCP_PORT;
    int graphNumRequests = DEFAULT_MAX_NUM_REQS;
    extern long simGraphPauseTime;

#endif //_SIM_

  /************************************************************/
  /* For long simulation runs: check that simulation does not */
  /* use too much memory.                                     */
  /************************************************************/
  int memUsed=0, memMaxUsed=0;

  /************************************************************/
  /******* Used to measure how long the simulation runs *******/
  /************************************************************/
  time_t startTime, stopTime;

  /*********************************************************/
  /*********** Print header information to stdout **********/
  /*********************************************************/
  headerInformation(argc,argv);

  /*********************************************************/
  /*********** Note time that simulation starts ************/
  /*********************************************************/
  time(&startTime);
  /* srand48(startTime); */

  /*********************************************************/
  /************** Parse top level user options *************/
  /*********************************************************/
  opterr=0;
  optind=1;
  for(i=0; i<=argc; i++)
    {
#ifdef _SIM_

      c = getopt(argc, argv, "vhf:r:u:l:pt:s:e:GP:R:S:"); 
		// SG: Added options: 
        //     G:Enable SIMGRAPH, P:SIMGRAPH TCP port,
        //     S:Socket Polling period, R: Number of requests.
#else
      c = getopt(argc, argv, "vhf:r:u:l:pt:s:e:"); 
#endif // _SIM_
      switch (c)
	{

#ifdef _SIM_

   /**************************************************/
     /* G, P, R, S are SIMGRAPH specific options (SG)  */
     /**************************************************/
 	case 'G': /* SimGraph Enable Flag, Default is disabled */
     simGraphEnabled = 1;
     printf("SimGraph is enabled\n");
     break;
 	case 'P': /* Graphsim TCP port: DEFAULT_TCP_PORT */
     graphTCPPort = atoi(optarg);
     printf("SimGraph TCP port number = %d\n", graphTCPPort);
     break;
 	case 'R': /* Graphsim Num Requests: DEFAULT_MAX_NUM_REQS */
     graphNumRequests = atoi(optarg);
     printf("SimGraph number of requests = %d\n", graphNumRequests);
     break;
 	case 'S': /* SimGraph Socket Period, default is 10 */
     simGraphSocketPeriod = atoi(optarg);
     printf("SimGraph socket period = %d\n", simGraphSocketPeriod);
     break;

#endif // _SIM_

	case 'l':	/* Length of simulation in cells: default=100 */
	  simulationLength =  atoi(optarg);
	  break;
	case 'r':	/* Reset Stats time */
	  resetStatsTime = atol(optarg);
	  break;
	case 't':	/* Traffic Period */
	  trafficPeriod = atoi(optarg);
	  printf("Traffic period: %d\n", trafficPeriod);
	  break;
	case 's':	/* Switch/Scheduling/Fabric Period */
	  fabricPeriod = atoi(optarg);
	  printf("Fabric period: %d\n", fabricPeriod);
	  break;
	case 'e':	/* Output/Exit Period */
	  outputPeriod = atoi(optarg);
	  printf("Output period: %d\n", outputPeriod);
	  break;
	case 'f':	/* Configuration filename */
	  fflag = 1;
      configFilename = (char *) malloc(sizeof(char)*(strlen(optarg)+1));
	  strcpy(configFilename, optarg);
	  break;
	case 'p':	/* Inidicate progress as simulation proceeds */
	  pflag = 1;
	  break;
	case 'u':       /* Sets random seed for rand generators */
	  globalSeed = atoi(optarg);
      if(globalSeed == 0)
      {
	    time((time_t *) &globalSeed);
	    printf("Randomly selected seed: ");
      }
	  printf("Simulation using seed %lu\n",globalSeed);
	  break;
	case 'v': /* Print version number */
      printf("Sim v2.36, Released May 11, 2017 \n");
      exit(0);
	case 'h':
	  fprintf(stderr, "usage: %s \n", argv[0]);
	  fprintf(stderr, "GENERAL options:\n");
	  fprintf(stderr, "    -h This help message \n");
	  fprintf(stderr, "    -l simulationlength. Default: 100\n");
	  fprintf(stderr, "    -r resetStatsTime. Default: simLength/10\n");
	  fprintf(stderr, "    -t Traffic Period Default: 1 \n");
	  fprintf(stderr, "    -s Fabric Period Default: 1 \n");
	  fprintf(stderr, "    -e Exit or Output Period Default: 1 \n");
	  fprintf(stderr, "    -p Indicate progress\n");
	  fprintf(stderr, "    -f configFilename.Default: none\n");
	  fprintf(stderr, "    -u globalSeed. Default: None \n");  
	  fprintf(stderr, "\nTraffic Models:\n");
	  fprintf(stderr, "----------------------------------------\n");
	  FOREACH_FUNCTION(trafficTable, TRAFFIC_USAGE);
	  fprintf(stderr, "\nInput Actions:\n");
	  fprintf(stderr, "----------------------------------------\n");
	  FOREACH_FUNCTION(inputActionTable, INPUTACTION_USAGE);
	  fprintf(stderr, "\nSwitch scheduling algorithms:\n");
	  fprintf(stderr, "----------------------------------------\n");
	  FOREACH_FUNCTION(algorithmTable, SCHEDULING_USAGE);
	  fprintf(stderr, "\nOutput Actions:\n");
	  fprintf(stderr, "----------------------------------------\n");
	  FOREACH_FUNCTION(outputActionTable, OUTPUTACTION_USAGE);
	  fprintf(stderr, "\nSwitch Fabrics:\n");
	  fprintf(stderr, "----------------------------------------\n");
	  FOREACH_FUNCTION(fabricTable, FABRIC_USAGE);
#ifdef _SIM_

        /**************************************************/
        /* G, P, R, S are SIMGRAPH specific options (SG)  */
        /**************************************************/
        fprintf(stderr,"\nSimGraph options:\n");
        fprintf(stderr,"    -G: Enable SimGraph. Default: disabled\n");
        fprintf(stderr,"    -P: SimGraph TCP port number.\n");
        fprintf(stderr,"    -R: SimGraph Number of Requests.\n");
        fprintf(stderr,"    -S: SimGraph Socket Polling Period. Default: 10\n");

#endif // _SIM_
	    
	  exit(0);
	default:
	  break;
	}
				
    }


  /********************************************************/
  /***** Parse config file and create switches ************/
  /********************************************************/
  if(!fflag)
    FatalError("Must specify -f option configuration file.");

  parseConfigurationFile( configFilename );

  if( resetStatsTime == NONE )
    resetStatsTime = simulationLength / 2;
  printf("ResetStatsTime %ld\n", resetStatsTime);

#ifdef _SIM_

  /************************************************************/
    /***** Initialize SIMGRAPH  (SG)                ************/
    /************************************************************/
    if( simGraphEnabled )
        {
            char aux[10];
            graph_init( &graphTool );
            sprintf(aux, "%d", graphNumRequests);
            graph_config( &graphTool, 0, 0, "maxNumReqs", aux);
            sprintf(aux, "%d", graphTCPPort);
            graph_config( &graphTool, 0, 0, "TCPportNum", aux);
            graph_start( &graphTool );
        }

#endif // _SIM_


  /**************************************************************/
  /******************* START SIMULATION *************************/
  /**************************************************************/
  for( now=0; (now<simulationLength) && (simStopped==CONTINUE_SIMULATION); 
       now++ )
    {
      if(debug_sim)
	{
	  printf("\nTIME %ld\n", now);
	  printf("---------\n");
	}


      /* Is it time to reset statistics? */
      if( now == resetStatsTime || now == 0 )
	{
	  for( switchNumber=0; switchNumber<numSwitches; switchNumber++)
	    {
	      aSwitch = switches[switchNumber];
	      printf("Resetting stats for switch: %d at time %lu\n", 
		     switchNumber, now);
	      switchStats(SWITCH_STATS_RESET, aSwitch); 
	      latencyStats(LATENCY_STATS_SWITCH_INIT, aSwitch, NULL);
	      for(output=0; output<aSwitch->numOutputs; output++)
		burstStats(BURST_STATS_RESET, aSwitch, output); 
	      (aSwitch->scheduler.schedulingAlgorithm)(SCHEDULING_INIT_STATS, aSwitch, aSwitch, NULL);
	      (aSwitch->fabric.fabricAction)(FABRIC_STATS_INIT, aSwitch);
	    }
	}


      /******************************************************/
      /********* Determine new traffic **********************/
      /************** for every switch **********************/
      /******************************************************/
      for( switchNumber=0; switchNumber<numSwitches; switchNumber++)
	{
	  if(now%trafficPeriod) continue;

	  if(debug_sim)
	    {
	      printf("SWITCH %d:\n", switchNumber);
	      printf("---------\n");
	    }
	  aSwitch = switches[switchNumber];

	  /***********************************************************/
	  /* The following is temporary until separate traffic units */
	  /* are implemented: */
	  /***********************************************************/
	  /* Determine new traffic for each input of switch */
	  if(debug_sim) printf("	New traffic\n");

	  for( input=0; input<aSwitch->numInputs; input++ )
	    {
	      if( (aSwitch->inputBuffer[input]->trafficModel)
		  (TRAFFIC_GENERATE, aSwitch, input, NULL, NULL) == 
		  STOP_SIMULATION)
		simStopped = STOP_SIMULATION;
	    }
	  if(debug_sim) printf("	Finished New traffic\n");

	}



      /**********************************************************/
      /******** Execute fabric action for all switches **********/
      /** Fabric transfers cells from input to output queues ****/
      /**********************************************************/
      for( switchNumber=0; switchNumber<numSwitches; switchNumber++)
	{

	  if(now%fabricPeriod) continue;
	
	  aSwitch = switches[switchNumber];	

	  for( input=0; input<aSwitch->numInputs; input++ )
		(aSwitch->inputAction)(INPUTACTION_PERCELL_CHECK, aSwitch, input, NULL);

	  /***************************************************/
	  /********  Execute Scheduling Algorithm ************/
	  /***************************************************/

	  if(debug_sim)	
	    {
	      printf("Levels BEFORE scheduling\n");
	      printInputFifoLevels( aSwitch );
	    }

	  if(debug_sim) printf("	Execute Scheduling Algorithm\n");
	  /* Configure switch: returns configuration matrix */
	  (aSwitch->scheduler.schedulingAlgorithm)(SCHEDULING_EXEC, aSwitch);

	  if(debug_sim)	
	    {
	      printf("Matrix AFTER scheduling\n");
	      printMatrix( aSwitch );
	    }

	  if(debug_sim)
	    (aSwitch->scheduler.schedulingAlgorithm)
	      (SCHEDULING_REPORT_STATE,aSwitch);

	  if( checkPeriod )
	    (aSwitch->scheduler.schedulingAlgorithm)
	      (SCHEDULING_CHECK_STATE_PERIOD, aSwitch); 

	  if(debug_sim) printf("	Switch fabric execution\n");
	  /* Transfer cells into/out of switch fabric. */
	  (aSwitch->fabric.fabricAction)(FABRIC_EXEC, aSwitch, NULL, NULL);
	}


      /**********************************************************/
      /******** Execute output actions for all switches *********/
      /**********************************************************/
      for( switchNumber=0; switchNumber<numSwitches; switchNumber++)
	{
	  if(now%outputPeriod) continue;

	  aSwitch = switches[switchNumber];	

	  if(debug_sim) printf("	Switch output execution\n");
	  /* Transfer cells to next switch or destroy. */
	  (aSwitch->outputAction)(OUTPUTACTION_EXEC, aSwitch);
	}

      /**********************************************************/
      /******* Per celltime statistics for each switch. *********/
      /**********************************************************/
      for( switchNumber=0; switchNumber<numSwitches; switchNumber++)
	{
	  aSwitch = switches[switchNumber];	
	  switchStats( SWITCH_STATS_PER_CELL_UPDATE, aSwitch );  

	  /* update input action */
	  for(input=0; input<aSwitch->numInputs; input++)
	    (aSwitch->inputAction)(INPUTACTION_PER_CELL_UPDATE, aSwitch, 
				   input, NULL);
	}

#ifdef _SIM_

      /**********************************************************/
      /******** Poll SimGraph stub (SG)                 *********/
      /**********************************************************/
      for( switchNumber=0; switchNumber<numSwitches && simGraphEnabled
         ; switchNumber++)
    {
      struct Element *anElement;

      aSwitch = switches[switchNumber];

      if( !(now%simGraphSocketPeriod) ) {
        graph_check_socket_ready( NULL, &graphTool, NULL);
      }

      if( now == simGraphPauseTime ) {
        graph_pause_on_socket( NULL, &graphTool, NULL);
      }

      for (anElement = graphTool.listOfRequests->head; anElement;
           anElement = anElement->next) {
        struct GraphRequestPacket *pkt =
          (struct GraphRequestPacket *)anElement->Object;
        struct GraphPeriodReqPktContents *pkt2 =
          &pkt->contents.period;
        if (pkt->reqType == GRAPH_PERIODIC_REPORT) {
          if( !(now%pkt2->period) ) {
          graph_periodicReporting(NULL, &graphTool, pkt);
          }
        }
      }
    }

#endif // _SIM_


      /**********************************************************/
      /*********** See if end condition has been met ************/
      /********  Currently based on mean cell latency ***********/
      /**********************************************************/
      if( now%CHECK_STOP == 0 )
	{
	  if( checkStopCondition() ) 
	    simStopped = STOP_SIMULATION;
	  if( pflag )	
	    fprintf(stderr, "now=%ld, %lu ", 
		    now, simulationLength);
	}

      /**********************************************************/
      /*********** See if memory usage is too high **************/
      /**********************************************************/
      if( now%CHECK_MEMORY_USAGE == 0 )
	{
	  memUsed = memoryUsage();
	  if( memUsed > memMaxUsed )
	    memMaxUsed = memUsed;
	  if( memUsed > MAX_ALLOWED_MEMORY_USAGE )
	    {
	      simStopped = STOP_SIMULATION;
	      printf("sim: too much memory allocated: %d bytes\n", memUsed);
	    }
	}
		
    }

  /***********************************************************/
  /**************** SIMULATION HAS COMPLETED *****************/
  /***********************************************************/
  printf("\n\n\n");
  printf("===============================================================\n");
  printf("======================== RESULTS ==============================\n");
  printf("===============================================================\n");
  printf("Simulation stopped at time: %lu\n", now);
  printf("Maximum memory used: %d bytes\n", memMaxUsed);
  time(&stopTime);
  printf("# Simulation runtime: %lu secs\n\n", (unsigned long) timeDiff(stopTime, startTime));

  for( switchNumber=0; switchNumber<numSwitches; switchNumber++)
    {
      printf("====================================================\n");
      printf("====================== SWITCH %4d =================\n", 
	     switchNumber);
      printf("====================================================\n");
      aSwitch = switches[switchNumber];
      /* Information about traffic */
      printf("  Traffic Information\n");
      printf("  -------------------\n");
      printf("  I/P  model  Util\n");
      for(input=0; input<aSwitch->numInputs; input++)
	{
	  (aSwitch->inputBuffer[input]->trafficModel)
	    (REPORT_TRAFFIC_STATS, aSwitch, input);
	}
      printf("\n");

      /* Print stats from input action(s). */
      (aSwitch->inputAction)(INPUTACTION_STATS_PRINT, aSwitch, 
			     NULL, NULL);

      switchStats(SWITCH_STATS_PRINT_ALL, aSwitch ); 
      (aSwitch->scheduler.schedulingAlgorithm)
	(SCHEDULING_REPORT_STATS, aSwitch);
      latencyStats(LATENCY_STATS_SWITCH_PRINT, aSwitch, NULL);
      (aSwitch->fabric.fabricAction)(FABRIC_STATS_PRINT, aSwitch);

      printf("\n");
      printf("  OUTPUT BURSTINESS\n");
      printf("  -----------------\n");
      printf("   O/P  Avg   SD   \n");
      printf("  -----------------\n");
      for( output=0; output<aSwitch->numOutputs;output++)
	burstStats(BURST_STATS_PRINT, aSwitch, output);
    }

  latencyStats(LATENCY_STATS_CELL_PRINT, NULL, NULL);

#ifdef _SIM_

    /************************************************************/
    /***** Shutdown  SIMGRAPH  (SG)                  ************/
    /************************************************************/

  if( simGraphEnabled )
        graph_end( &graphTool );

#endif // _SIM_



	free(switches); /* Added Sundar */
  exit(0);
}


/**********************************************************************/
/********************* Support Routines *******************************/
/**********************************************************************/
static int checkStopCondition()
{
  /* 
     Currently: 
     if mean delay of cells changes 
     less than STOP_THRESHOLD in a CHECK_STOP period, 
     then return STOP_SIMULATION;
     else return CONTINUE_SIMULATION;
	*/

  static double lastValue=0.0;
  static int numTimesMetStopCondition=0;
  double thisValue;
  double diff;

  thisValue = latencyStats(LATENCY_STATS_RETURN_AVG, NULL, NULL);
  if( thisValue == 0 )
    return(NO);
  diff = fabs((lastValue - thisValue) / thisValue );
  if( diff < STOP_THRESHOLD )
    numTimesMetStopCondition++;
  else
    numTimesMetStopCondition = 0;
  if( numTimesMetStopCondition >= NUM_TIMES_MUST_MEET_STOP_CONDITION )
    {
      printf("Stopped because converged at time: %ld\n", now);
      return(STOP_SIMULATION);
    }
  else
    {
      lastValue = thisValue;
      return(NO);
    }
		
}

/*************************************************************/
static int memoryUsage()
{
  return(getAllocAmount());
}

/**************************************************************/
static void headerInformation(argc, argv)
  int argc;
char **argv;
{
  int i;
  char hostname[MAXSTRING];
  time_t aTime;

  printf("##################################################\n");
  for(i=0;i<argc;i++)
    printf("%s ", argv[i]); 
  time(&aTime);
  printf("\nThis is Sim version 2.36, Released May 11, 2017 \n"); 
  printf("\n# Date %s", ctime(&aTime)); 

  gethostname(hostname, MAXSTRING);
  printf("# Machine %s\n", hostname);
  printf("##################################################\n");
}

/***************************************************************/
void FatalError(aString)
  char *aString;
{
  fprintf(stderr, "sim: Fatal Error at time %ld.\n", now);
  fprintf(stderr, "sim: %s.\n", aString);
  exit(1);
}

/***************************************************************/

time_t timeDiff(time2, time1)
  time_t time2, time1;
{
  return(time2-time1);
}

