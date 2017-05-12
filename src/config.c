
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


/*
	Configure simulation from a file.
	File has FIXED format:

		Numswitches %d
		Switch 0
			Numinputs  %d
			Numoutputs %d
			PriorityLevels %d  (optional)
			InputAction %s  ( parameters of action)
			OutputAction %s ( parameters of action)
			Fabric     %s  ( parameters of fabric)
			Algorithm  %s  ( parameters of algorithm, e.g: "-N 4" )
			Input       TrafficModel       Parameters
			0	    %s                 %s (e.g: "-u 0.5")
			1	    %s                 %s (e.g: "-u 0.5")
			...			...
			Numinputs-1 %s	               %s (e.g: "-u 0.5")
			Stats
				Arrivals	(0,0) (0,1) ... 
				Departures 	(0,0) (0,1) ... 
				Latency		(0,0) (0,1) ... 
				Occupancy	(0,0) (0,1) ... 
			Histograms
				Arrivals	(0,0) (0,1) ... 
				Departures 	(0,0) (0,1) ... 
				Latency		(0,0) (0,1) ... 
				Occupancy	(0,0) (0,1) ... 
		Switch 1
		...
		Switch Numswitches-1

Stats/Histograms: Determines which stats/histograms to gather for general
                  input and output fifos only. 
                  The tuple: (in,out) means inputBuffer[in]->fifo[out].
                  Some statistics are always gathered (e.g. overall cell
                  latency) and are not controllable by the user.
*/

#include <string.h>
#include "sim.h"
#include "algorithm.h"
#include "fabric.h"
#include "traffic.h"
#include "inputAction.h"
#include "outputAction.h"

#include "algorithmTable.h"
#include "fabricTable.h"
#include "trafficTable.h"
#include "inputActionTable.h"
#include "outputActionTable.h"

/***********   Constants **************************/

/* Change these parameters to change the way histograms */
/* are collected and printed. For example, to make the  */
/* INPUT_FIFO histograms have bins: 0,1,2,3,4,... then  */
/* use the following values:                            */
/* #define INPUT_FIFO_NUM_BINS      20                  */
/* #define INPUT_FIFO_MIN_BIN       (double) 1.0                  */
/* #define INPUT_FIFO_BIN_STEPSIZE  (double) 1.0                  */
/* #define INPUT_FIFO_HIST_TYPE     HISTOGRAM_STEP_LINEAR         */

#define INPUT_FIFO_NUM_BINS      20
#define INPUT_FIFO_MIN_BIN       (double) 2.0
#define INPUT_FIFO_BIN_STEPSIZE  (double) 2.0
#define INPUT_FIFO_HIST_TYPE     HISTOGRAM_STEP_LOG

/*
#define OUTPUT_FIFO_NUM_BINS     20
#define OUTPUT_FIFO_MIN_BIN      (double) 2.0
#define OUTPUT_FIFO_BIN_STEPSIZE (double) 2.0
#define OUTPUT_FIFO_HIST_TYPE     HISTOGRAM_STEP_LOG
*/
#define OUTPUT_FIFO_NUM_BINS     20
#define OUTPUT_FIFO_MIN_BIN      (double) 1.0
#define OUTPUT_FIFO_BIN_STEPSIZE (double) 1.0
#define OUTPUT_FIFO_HIST_TYPE     HISTOGRAM_STEP_LINEAR


/***********  Defined elsewhere and used here ***********/
extern void FatalError(); /* in sim.c */

/*********************  Defined here ********************/
static int parseTuples();
static void parseRestOfLine();



void 
parseConfigurationFile( configFilename )
  char *configFilename;
{
  Switch *aSwitch;
  struct List *dfifo;

  FILE *fp;
  char aString[MAXSTRING];
  char *histogramTitle=(char*)0;
  int  anInt, switchNumber;
  int  numInputs, numOutputs;
  int priorityLevels=1;    /* default to be 1 */
  int pri, start, numFIFOs;

  int numArgs;
  char **argVector;

  /* int i, type, input, output; CHANGED */
  int i, type, input, output,inputs,outputs;
  char switchAlgorithm[MAXSTRING];
  char switchFabric[MAXSTRING];
  char trafficModel[MAXSTRING];
  char inputAction[MAXSTRING];
  char outputAction[MAXSTRING];

  /* Allocate space for argVector pointers */
  argVector = (char **) malloc( MAXSTRING * sizeof(char *) );
  for(i=0; i<MAXSTRING; i++)
    argVector[i] = (char *) malloc( MAXSTRING * sizeof(char) );

  if( (strcmp(configFilename, "-") == 0) 
      || (strcmp(configFilename, "stdin") == 0) )
    fp = stdin;
  else
    fp = fopen(configFilename, "r");

  if( fp == NULL )
    FatalError("configSimulationFile(): couldn't open file.");

  /* Read file */

  /* Read number of switches, ignore lines starting with "#" */
  fscanf(fp, "%s", aString); 
  if( aString[0] == '#' )
  {
        while(fgetc(fp) != '\n');
        fscanf(fp, "%s", aString); 
  }
  if( strcasecmp(aString, "numswitches") != 0 )
    FatalError("First line of config file must be \"NumSwitches\"");
  fscanf(fp, "%d", &numSwitches);
  printf("\nNumber of switches: %d\n\n", numSwitches);

  /*************************************************************/
  /*********** Create space for all switch pointers ************/
  /*************************************************************/
  switches = (Switch **) malloc( (numSwitches)*sizeof( Switch * ) );

  for( switchNumber=0; switchNumber<numSwitches; switchNumber++ )
  {

      /* Read switch number */
      fscanf(fp, "%s", aString);
      if( strcasecmp(aString, "switch") != 0 )
	    FatalError("Switch description must start with \"Switch\"");
      fscanf(fp, "%d", &anInt);
      if( anInt != switchNumber )
	    FatalError("Wrong switch number.");
      printf("============================================\n");
      printf("================ Switch %d =================\n", switchNumber);
      printf("============================================\n");

      /* Read number of inputs for this switch */
      fscanf(fp, "%s", aString);
      if( strcasecmp(aString, "numinputs") != 0 )
	    FatalError("Expected \"Numinputs\"");
      fscanf(fp, "%d", &numInputs);

      /* Read number of outputs for this switch */
      fscanf(fp, "%s", aString);
      if( strcasecmp(aString, "numoutputs") != 0 )
	    FatalError("Expected \"Numoutputs\"");
      fscanf(fp, "%d", &numOutputs);

      /* Read number of priorities and !inputaction! */
      fscanf(fp, "%s", aString);

      /**************************************************************/
      /*      Parse (optional) PriorityLevels line.                 */
      /**************************************************************/
      if( strcasecmp(aString, "prioritylevels") == 0)
	  {
	    fscanf(fp, "%d", &priorityLevels);
	    fscanf(fp, "%s", aString);
      }

      /**********************************************************/
      /******************* Create switch ************************/
      /**********************************************************/
      printf("Creating switch with %d inputs, %d outputs and %d priorities\n", 
	     numInputs, numOutputs, priorityLevels);
      aSwitch = switches[switchNumber] = createSwitch(switchNumber,
						      numInputs, numOutputs, priorityLevels);
      


      /***********************************************************/
      /*        Parse and initialize InputAction.                */
      /***********************************************************/
	  if( strcasecmp(aString, "inputaction") != 0 )
	    FatalError("Expected \"InputAction\"");
	  fscanf(fp, "%s", inputAction);
      printf("InputAction: %s\n", inputAction);

      aSwitch->inputAction = (Cell* (*)()) 
        findFunction(inputAction, inputActionTable);
      if(aSwitch->inputAction)
      {
        parseRestOfLine( fp, &numArgs, argVector );
        (aSwitch->inputAction)(INPUTACTION_INIT,aSwitch,NULL,NULL,NULL, NULL,
			     numArgs,argVector);
      }
      else 
        FatalError("Unknown inputAction");


      /**********************************************************/
      /************ Parse and initialize OutputAction  **********/
      /**********************************************************/
      /* Read switch output action */
      fscanf(fp, "%s", aString);
      if( strcasecmp(aString, "outputaction") != 0 )
	    FatalError("Expected \"OutputAction\"");
      fscanf(fp, "%s", outputAction);
      printf("OutputAction: %s\n", outputAction);

      // aSwitch->outputAction = (int (*) ()) 
      aSwitch->outputAction = (void (*) ()) 
        findFunction(outputAction, outputActionTable);
      if( aSwitch->outputAction )
	  {
        parseRestOfLine( fp, &numArgs, argVector );
        (aSwitch->outputAction)(OUTPUTACTION_INIT,aSwitch,numArgs,argVector);
      }
      else
	    FatalError("Output Action is unknown.");

      /**********************************************************/
      /************ Parse and init switch fabric ****************/
      /**********************************************************/
      /* Read switch fabric */
      fscanf(fp, "%s", aString);

      if( strcasecmp(aString, "fabric") != 0 )
	    FatalError("Expected \"Fabric\"");
      fscanf(fp, "%s", switchFabric);
      printf("Fabric: %s\n", switchFabric);

      // aSwitch->fabric.fabricAction = (int (*) ())
      aSwitch->fabric.fabricAction = (void (*) ())
        findFunction(switchFabric, fabricTable);
      if( aSwitch->fabric.fabricAction )
	  {
        parseRestOfLine( fp, &numArgs, argVector );
        (aSwitch->fabric.fabricAction)(FABRIC_INIT, aSwitch, numArgs, argVector);
      }
      else
	    FatalError("Fabric is unknown.");

      /**********************************************************/
      /************ Read and set switch algorithm ***************/
      /**********************************************************/
      /* Read switch algorithm */
      fscanf(fp, "%s", aString);
      if( strcasecmp(aString, "algorithm") != 0 )
	    FatalError("Expected \"Algorithm\"");
      fscanf(fp, "%s", switchAlgorithm);
      printf("Switch Scheduling Algorithm: %s\n", switchAlgorithm);

      aSwitch->scheduler.schedulingAlgorithm = (void (*) ()) 
        findFunction(switchAlgorithm, algorithmTable);
      if( aSwitch->scheduler.schedulingAlgorithm )
	  {
        parseRestOfLine( fp, &numArgs, argVector );
        (aSwitch->scheduler.schedulingAlgorithm)(SCHEDULING_INIT, aSwitch, numArgs, argVector);
      }
      else
      {
	    fprintf(stderr, "Switch algorithm \"%s\" set to null.\n", 
		    switchAlgorithm);
	    aSwitch->scheduler.schedulingAlgorithm =(void (*)())nullSchedulingAlgorithm;
      }

      /********************************************************************/
      /***** For each input: Parse and init traffic Model   ***************/
      /********************************************************************/
      for( input=0; input<aSwitch->numInputs; input++ )
	  {
	    /* Read and set traffic algorithm */
	    fscanf(fp, "%d", &anInt);
	    if( anInt != input )
	        {
	        fprintf(stderr, "Wrong input number: %d, expected: %d", 
		        anInt, input);
	         exit(1);
	    }
	    fscanf(fp, "%s", trafficModel);
	    printf("Input: %d  Traffic model: %s\n", input, trafficModel);

        aSwitch->inputBuffer[input]->trafficModel = (int (*)()) 
            findFunction(trafficModel, trafficTable);
        if( aSwitch->inputBuffer[input]->trafficModel)
	    {
            parseRestOfLine( fp, &numArgs, argVector );
	        (aSwitch->inputBuffer[input]->trafficModel)
	            (TRAFFIC_INIT, aSwitch, input, numArgs, argVector);
        }
        else
        {
	        fprintf(stderr,"Traffic Model %s at switch %d input %d is unknown.\n",
		        trafficModel, aSwitch->switchNumber, input);
	        exit(1);
        }
      } 



      /**********************************************************/
      /********* Read which stats to create and reset ***********/
      /**********************************************************/
      /*
	Stats
	Arrivals 	(0,0) (0,1) ... 4 7
	Departures 	(0,0) (0,1) ... 4 7
	Latency		(0,0) (0,1) ... 4 7
	Occupancy	(0,0) (0,1) ... 4 7
	*/
      /* Read Stats Line */
      fscanf(fp, "%s", aString);
      if( strcasecmp(aString, "stats") != 0 )
	    FatalError("Expected \"Stats\"");
      printf("\nStatistics Enabled for switch: %d\n", aSwitch->switchNumber);
      printf("---------------------------------\n");

      /************************************************/
      /**************** Read Stats Line ***************/
      /************************************************/
      for( type=0; type<NUM_LIST_STATS_TYPES; type++ )
	{
	  switch(type)
	    {
	    case LIST_STATS_TA_OCCUPANCY:
	      fscanf(fp, "%s", aString);
	      if( strcasecmp(aString, "occupancy") != 0 )
		FatalError("Expected \"Occupancy\"");
	      printf("    Occupancy Statistics: \n");
	      break;
	    case LIST_STATS_ARRIVALS:
	      fscanf(fp, "%s", aString);
	      if( strcasecmp(aString, "arrivals") != 0 )
		FatalError("Expected \"Arrivals\"");
	      printf("    Arrival Statistics: \n");
	      break;
	    case LIST_STATS_DEPARTURES:
	      fscanf(fp, "%s", aString);
	      if( strcasecmp(aString, "departures") != 0 )
		FatalError("Expected \"Departures\"");
	      printf("    Departures Statistics: \n");
	      break;
	    case LIST_STATS_LATENCY:
	      fscanf(fp, "%s", aString);
	      if( strcasecmp(aString, "latency") != 0 )
		FatalError("Expected \"Latency\"");
	      printf("    Latency Statistics: \n");
	      break;
	    default:
	      break;
	    }


	  /* parseTuples returns each input,output pair in turn that */
	  /* it finds before EOL or EOF. */	
	  /* If input==NONE,then refers to an output only. */
	  /* If output==NONE,then refers to multicast. */
	  /* If input==NONE,output==NONE, neither input or output */
	  /* while( parseTuples(fp, &input, &output) != EOF Changed */
	  while( parseTuples(fp, &inputs, &outputs) != EOF )
	    {	
	      /* Check range of input and output values */
	      if( (inputs!=NONE) && (inputs>aSwitch->numInputs) )
		{
		  printf("    WARNING: there is no input %d\n", inputs);
		  continue;
		}
	      if( (outputs!=NONE) && (outputs>aSwitch->numOutputs) )
		{
		  printf("    WARNING: there is no output %d\n", outputs);
		  continue;
		}
	      if( (inputs != NONE) && (outputs != NONE)  )
		{
		  /* It's an input fifo */
		  /* Enable statistics. */
		  if(inputs==ALL)
		    { 
		      for(input=0;input<aSwitch->numInputs;input++) 
			{
			  if( outputs == ALL )
			    {
			      numFIFOs = aSwitch->numPriorities * aSwitch->numOutputs;   
			      for(output=0; output<numFIFOs; output++)
				{
				  enableListStats(
						  aSwitch->inputBuffer[input]->fifo[output],type);
				}
			    }
			  else
			    {
			      start = outputs*aSwitch->numPriorities;
			      for(pri=start; pri<start+aSwitch->numPriorities; pri++)			    
 				enableListStats(
						aSwitch->inputBuffer[input]->fifo[pri],type);
			      printf("      Enabled for i/p buffers (*,%d)\n", outputs);
			    }
			}
		      if(outputs==ALL)
			printf("      Enabled for i/p buffers (*,*)\n");
		    } /* end of if (input==ALL) */
		  else
		    {
		      if( outputs == ALL )
			{
			  numFIFOs = aSwitch->numOutputs * aSwitch->numPriorities;
			  for(output=0; output<numFIFOs; output++)
			    {
			      enableListStats(
					      aSwitch->inputBuffer[inputs]->fifo[output],
					      type);
			    }
			  printf("      Enabled for i/p buffers (%d,*)\n",  inputs);
			}
		      else
			{
			  start = aSwitch->numPriorities * outputs;       
			  for(pri = start; pri<start+aSwitch->numPriorities; pri++)
			    enableListStats(
					    aSwitch->inputBuffer[inputs]->fifo[pri],type);
			  printf("      Enabled for i/p buffer (%d,%d)\n", inputs, outputs);
			}
		    }
		} /* end if (input!=NONE && output !=NONE) */
	      else if( (outputs != NONE)  )
		{
		  /* It's an output fifo */
		  /* Enable statistics. */
		  if( outputs == ALL )
		    {
		      for(output=0; output<aSwitch->numOutputs; output++)
			for(pri=0;pri<aSwitch->numPriorities;pri++)
			{
			  enableListStats(aSwitch->outputBuffer[output]->fifo[pri],
					  type); 
			}
		      printf("      Enabled for all o/p buffers\n");
		    }
		  else
		    {
		      for(pri=0;pri<aSwitch->numPriorities;pri++)
			enableListStats(aSwitch->outputBuffer[outputs]->fifo[pri],
				      type);
		      printf("      Enabled for o/p buffer %d\n", outputs);
		    }
		}
	      else if( (inputs != NONE) && (outputs ==NONE)  )
		{
		  /* output == NONE => MULTICAST */
		  if( inputs == ALL )
		    {
		      printf("      Enabled mcast for all i/p buffers\n");
		      for(input=0;input<aSwitch->numInputs;input++)
			for(pri=0;pri<aSwitch->numPriorities;pri++)
			  {
			    enableListStats(
					    aSwitch->inputBuffer[input]->mcastFifo[pri], type);
			  }
		    }
		  else
		    {
		      for(pri=0;pri<aSwitch->numPriorities;pri++)
			enableListStats(aSwitch->inputBuffer[input]->mcastFifo[pri],
					type);
		      printf("      Enabled mcast for i/p %d\n", input);
		    }
		}
	    }
	
	}
      
      /**********************************************************/
      /********* Read which histograms to create and reset ***********/
      /**********************************************************/
      /*
	Histograms
	Arrivals 	(0,0) (0,1) ... 4 7
	Departures 	(0,0) (0,1) ... 4 7
	Latency		(0,0) (0,1) ... 4 7
	Occupancy	(0,0) (0,1) ... 4 7
	*/
      /* Read Histograms Line */
      fscanf(fp, "%s", aString);
      if( strcasecmp(aString, "histograms") != 0 )
	FatalError("Expected \"Histograms\"");

      /**********************************************************/
      /**************** Read Histogram Lines ********************/
      /**********************************************************/
      for( type=0; type<NUM_LIST_HISTOGRAM_TYPES; type++ )
	{
	  switch(type)
	    {
	    case LIST_HISTOGRAM_TIME:
	      fscanf(fp, "%s", aString);
	      if( strcasecmp(aString, "occupancy") != 0 )
		FatalError("Expected \"Occupancy\"");
	      printf("    Occupancy Histogram: \n");
	      histogramTitle = "Time Average Queue Occupancy";
	      break;
	    case LIST_HISTOGRAM_ARRIVALS:
	      fscanf(fp, "%s", aString);
	      if( strcasecmp(aString, "arrivals") != 0 )
		FatalError("Expected \"Arrivals\"");
	      printf("    Arrival Histogram: \n");
	      histogramTitle = "Occupancy seen by Arrivals";
	      break;
	    case LIST_HISTOGRAM_DEPARTURES:
	      fscanf(fp, "%s", aString);
	      if( strcasecmp(aString, "departures") != 0 )
		FatalError("Expected \"Departures\"");
	      printf("    Departures Histogram: \n");
	      histogramTitle = "Occupancy seen by Departures";
	      break;
	    case LIST_HISTOGRAM_LATENCY:
	      fscanf(fp, "%s", aString);
	      if( strcasecmp(aString, "latency") != 0 )
		FatalError("Expected \"Latency\"");
	      printf("    Latency Histogram: \n");
	      histogramTitle = "Latency Through Queue";
	      break;
	    }

	  /* parseTuples returns each input,output pair in turn that */
	  /* it finds before EOL or EOF. */	
	  /* If input==NONE,then refers to an output only. */
	  /* If input==NONE,output==NONE, neither input or output */
	  while( parseTuples(fp, &input, &output) != EOF )
	    {	
	      /* Check range of input and output values */
	      if( (input!=NONE) && (input>aSwitch->numInputs) )
		{
		  printf("    WARNING: there is no input %d\n", input);
		  continue;
		}
	      if( (output!=NONE) && (output>aSwitch->numOutputs) )
		{
		  printf("    WARNING: there is no output %d\n", output);
		  continue;
		}
	      /* Check range of input and output values */
	      if( (input!=NONE) && (input>aSwitch->numInputs) )
		{
		  printf("    WARNING: there is no input %d\n", input);
		  continue;
		}
	      if( (output!=NONE) && (output>aSwitch->numOutputs) )
		{
		  printf("    WARNING: there is no output %d\n", output);
		  continue;
		}
	
	      if( (input != NONE) && (output != NONE)  )
		{
		  /* It's an input fifo */
		  if(input == ALL ) 
		    {
		      for(input =0;input<aSwitch->numInputs; input++)
			{
			  if( output == ALL )
			    {
			      numFIFOs = aSwitch->numOutputs * aSwitch->numPriorities;
			      for(output=0; output<numFIFOs; output++)
				{
				  dfifo = aSwitch->inputBuffer[input]->fifo[output];
				  initListHistogram(dfifo, histogramTitle,
						    type, HISTOGRAM_STATIC, INPUT_FIFO_HIST_TYPE,
						    INPUT_FIFO_NUM_BINS, INPUT_FIFO_MIN_BIN,
						    INPUT_FIFO_BIN_STEPSIZE);
				  enableListHistogram(
						      aSwitch->inputBuffer[input]->fifo[output], type);
				}
			      printf("      Enabled for i/p buffers (%d,*)\n", 
				     input);
			    }
			  else
			    {
			      /* Enable time average occupancy histogram. */
			      start = output * aSwitch->numPriorities;
			      for(pri=start;pri<start+aSwitch->numPriorities; pri++)
				{
				  dfifo = aSwitch->inputBuffer[input]->fifo[pri];
				  initListHistogram(dfifo, histogramTitle,
						    type, HISTOGRAM_STATIC, INPUT_FIFO_HIST_TYPE,
						    INPUT_FIFO_NUM_BINS, INPUT_FIFO_MIN_BIN,
						    INPUT_FIFO_BIN_STEPSIZE);
				  enableListHistogram(
						      aSwitch->inputBuffer[input]->fifo[pri], type);
				}
			      printf("      Enabled for i/p buffer (%d,%d)\n",
				     input, output);
			    }
			}
		    }
		  else
		    {	
		      /* input != ALL */
		      if( output == ALL )
			{
			  numFIFOs = aSwitch->numOutputs * aSwitch->numPriorities;
			  for(output=0; output<numFIFOs; output++)
			    {
			      dfifo = aSwitch->inputBuffer[input]->fifo[output];
			      initListHistogram(dfifo, histogramTitle,
						type, HISTOGRAM_STATIC, INPUT_FIFO_HIST_TYPE,
						INPUT_FIFO_NUM_BINS, INPUT_FIFO_MIN_BIN,
						INPUT_FIFO_BIN_STEPSIZE);
			      enableListHistogram(dfifo, type);
			    }
			  printf("      Enabled for i/p buffers (%d,*)\n", 
				 input);
			}
		      else
			{
			  start = output * aSwitch->numPriorities;
			  for(pri=start;pri<start+aSwitch->numPriorities; pri++)
				{
				  dfifo = aSwitch->inputBuffer[input]->fifo[pri];
				  initListHistogram(dfifo, histogramTitle,
						    type, HISTOGRAM_STATIC, INPUT_FIFO_HIST_TYPE,
						    INPUT_FIFO_NUM_BINS, INPUT_FIFO_MIN_BIN,
						    INPUT_FIFO_BIN_STEPSIZE);
				  enableListHistogram(dfifo, type);
				  
				  printf("      Enabled for i/p buffer (%d,%d)\n", input, output);
				}
			}
		    }
		} /* end if (input!=NONE && output !=NONE) */
	      else if( output != NONE )
		{
		  /* It's an output fifo */                 
		  if( output == ALL )
		    {
		      for(output=0; output<aSwitch->numOutputs; output++)
			for(pri=0;pri<aSwitch->numPriorities;pri++)
			  {
			    /* Enable time average occupancy histogram. */
			    dfifo = aSwitch->outputBuffer[output]->fifo[pri];
			    initListHistogram(dfifo, histogramTitle,
					      type,
					      HISTOGRAM_STATIC, OUTPUT_FIFO_HIST_TYPE,
					      OUTPUT_FIFO_NUM_BINS, OUTPUT_FIFO_MIN_BIN,
					      OUTPUT_FIFO_BIN_STEPSIZE);
			    enableListHistogram(aSwitch->outputBuffer[output]->fifo[pri],
						type);
			  }
		      printf("      Enabled for all o/p buffers\n");
		    }
		  else
		    {
		      /* Enable time average occupancy histogram. */
		      for(pri=0;pri<aSwitch->numPriorities; pri++)            /* TL */
			{
			  dfifo = aSwitch->outputBuffer[output]->fifo[pri];
			  initListHistogram(dfifo, histogramTitle,
					    type,
					    HISTOGRAM_STATIC, OUTPUT_FIFO_HIST_TYPE,
					    OUTPUT_FIFO_NUM_BINS, OUTPUT_FIFO_MIN_BIN,
					    OUTPUT_FIFO_BIN_STEPSIZE);
			  enableListHistogram(aSwitch->outputBuffer[output]->fifo[pri],
					      type);
			}
		    }
		}
	      else if( (input!=NONE) && (output==NONE))
		{
		  /* MULTICAST */
		  if(input==ALL)
		    {
		      for(input=0;input<aSwitch->numInputs;input++)
			for(pri=0;pri<aSwitch->numPriorities;pri++)
			  {
			    dfifo = aSwitch->inputBuffer[input]->mcastFifo[pri];
			    initListHistogram(dfifo, histogramTitle,
					      type, HISTOGRAM_STATIC, INPUT_FIFO_HIST_TYPE,
					      INPUT_FIFO_NUM_BINS, INPUT_FIFO_MIN_BIN,
					      INPUT_FIFO_BIN_STEPSIZE);
			    enableListHistogram(dfifo,type);
			  }
		      printf("      Enabled for all mcast i/p buffers\n");
		    }
		  else
		    {
		      for(pri=0;pri<aSwitch->numPriorities;pri++)
			{
			  dfifo = aSwitch->inputBuffer[input]->mcastFifo[pri];
			  initListHistogram(dfifo, histogramTitle,
					    type, HISTOGRAM_STATIC, INPUT_FIFO_HIST_TYPE,
					    INPUT_FIFO_NUM_BINS, INPUT_FIFO_MIN_BIN,
					    INPUT_FIFO_BIN_STEPSIZE);
			  enableListHistogram(dfifo,type);
			}
		      printf("      Enabled for mcast i/p buffer %d\n",input);
		    }
		}
	    }
	}
      fflush(stdout);
		
  }

  /* Free malloc'd space for argVector */
  for(i=0; i<MAXSTRING; i++)
    free(argVector[i]);
  free(argVector);

  /* Close configuration file and return */
  fclose(fp);
  return;
}


/*************************************************************/
/* Read from input fp until '\n' or EOF  is reached.         */
/* If a pair: (a,b) is found, set *input=a, *output=b.       */
/* If a single integer is found, set *input=NONE, *output=a. */
/* If no more singles/pairs, return EOF.                     */
/*************************************************************/
static int parseTuples( fp, input, output )
  FILE *fp;
int *input;
int *output;
{
  int c;

  c = getc(fp);
  switch(c)
    {
    case '\n':
    case EOF:
      return(EOF);
      break;
    case '(':
      /* Beginning of a tuple. */
      while( (c=getc(fp)) == ' ');
      switch(c)
	{
	case '*':
	  *input = ALL;
	  break;
	default:
	  ungetc(c, fp);
	  fscanf(fp, "%d", input);
	  break;
	}

      /* Read up to ',' seperator */
      while( (c=getc(fp)) != ',');

      /* Read to check whether *all* outputs for this input. */
      while( (c=getc(fp)) == ' ');
      switch(c)
	{
	case '*':
	  while( (c=getc(fp)) != ')');
	  *output = ALL;
	  return(1);
	case 'm':
	case 'M':
	  /* Multicast */
	  while( (c=getc(fp)) != ')');
	  *output = NONE;
	  return(1);
	default:
	  ungetc(c, fp);
	  fscanf(fp, "%d", output);
	  while( (c=getc(fp)) != ')');
	  return(1);
	}
      break;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      /* Put the char back and read the whole integer. */
      /* A single integer => an output. */
      ungetc(c, fp);
      fscanf(fp, "%d", output);
      *input=NONE;
      return(1);
    case '*':
      *input  = NONE;
      *output = ALL;
      return(1);
    default:
      *input=NONE;
      *output=NONE;
      return(1);
    }
}

/*
	Read the rest of the line.
	Break into seperate words and place into argVector.
	Count # of args and place in numArgs
*/
static void parseRestOfLine( fp, numArgs, argVector )
  FILE *fp;
int *numArgs;
char **argVector;
{
  char line[MAXSTRING];
  char *aWord;
  int i=0;


  /* Read rest of line */
  /* Remove leading white space */
  do {
    fread(&line[0], 1, 1, fp);
  } while(line[0] == ' ' || line[0] == '\t');
  i=1;
  if( line[0] != '\n' )
    do {
      fread(&line[i], 1, 1, fp);
    } while( line[i++] != '\n' );
  line[--i] = '\0';

  if(debug_sim)
    printf("Line: %s\n", line);

  /* Initialize argVector */
  strcpy(argVector[0], "nothing");

  /* Read first word from rest of line */
  if( (aWord = (char *)strtok( line, " ")) != NULL)
    *numArgs = 2;
  else
    {
      *numArgs = 1;
      return;
    }
  strcpy( argVector[1], aWord );

  if(debug_sim)
    printf("Word: %s\n", aWord);

  while( (aWord = (char *)strtok( NULL, " ")) != NULL )
    {
      strcpy( argVector[*numArgs], aWord );
      if(debug_sim)
	printf("Word: %s\n", aWord);
      (*numArgs)++;
    }
  *argVector[*numArgs] = 0;

  if(debug_sim)
    for(i=0; i<*numArgs; i++)
      printf("Arg: %d, %s\n", i, argVector[i]);

}
