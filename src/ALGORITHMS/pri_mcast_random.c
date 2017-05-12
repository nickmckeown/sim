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
#include <string.h>  // For memset operations

#include "sim.h"
#include "algorithm.h"




/*  Determines switch configuration  for multicast input fifos */
/*  Configuration is filled into aSwitch->fabric.interconnect.matrix array 	*/

/* Each output selects randomly and unifromly over the requesting inputs. */

unsigned short int aSeed[3];

static int selectInput();

void
pri_mcast_random(action, aSwitch, argc, argv)
  SwitchAction action;
Switch *aSwitch;
int argc;
char **argv;
{
  int input, output;
  int out;

  if(debug_algorithm)
    printf("Algorithm 'pri_mcast_random()' called by switch %d\n", aSwitch->switchNumber);
	
  switch(action) {
  case SCHEDULING_USAGE:
    fprintf(stderr, "No options for \"pri_mcast_random\" scheduling algorithm.\n");
    break;
  case SCHEDULING_INIT:	
    if(debug_algorithm)
      printf("	SCHEDULING_INIT\n");

    aSeed[0] = 0xde76 ^ globalSeed;
    aSeed[1] = 0xab34 ^ globalSeed;
    aSeed[2] = 0x7223 ^ globalSeed;

    break;

  case SCHEDULING_EXEC:
    {
      int  numFabricOutputs;
      int priority;
      static int *outputSelected=(int *) NULL;
      static int *inputMask =(int *) NULL;
      static int *outputMask =(int *) NULL;

      if(debug_algorithm)
	{
	    if(debug_algorithm) printf("    SCHEDULING_EXEC, %ld\n", now);
	}

      numFabricOutputs = aSwitch->numOutputs *
	aSwitch->fabric.Xbar_numOutputLines;


      if( !outputSelected )
      {
        outputSelected = (int *) malloc(aSwitch->numOutputs * sizeof(int));
        inputMask = (int *) malloc(aSwitch->numInputs * sizeof(int));
        outputMask = (int *) malloc(numFabricOutputs * sizeof(int));
      }

      memset(outputSelected, 0, aSwitch->numOutputs * sizeof(int));

        /* Build masks of inputs and outputs already selected by
         * a previous scheduling algorithm.
         */ 
      memset(inputMask, 0, numFabricOutputs * sizeof(int));
      for(output=0; output<numFabricOutputs;output++)
        if(aSwitch->fabric.Xbar_matrix[output].input != NONE)
        {
                inputMask[aSwitch->fabric.Xbar_matrix[output].input] = 1;
                outputMask[output] = 1;
        }
        else outputMask[output] = 0;

      for(priority=0; priority<aSwitch->numPriorities; priority++)
	for(output=0; output<numFabricOutputs; output++)
	  {
	    out = output / aSwitch->fabric.Xbar_numOutputLines;

	    if(outputSelected[out] || outputMask[output]) continue;

	    /* grant an input for this output */
	    if(debug_algorithm)
	      printf("Selecting grant for output %d\n", output);
	    input = selectInput(aSwitch, output, priority, inputMask);
	    if(debug_algorithm)
	      printf("Selected input %d\n", input);

	    aSwitch->fabric.Xbar_matrix[output].input = input;
	    if( input != NONE ) 
	      {
		aSwitch->fabric.Xbar_matrix[output].cell = (Cell *)
		  aSwitch->inputBuffer[input]->mcastFifo[priority]->head->Object;
		
		if(outputSelected[out])
		  printf("conflicts at output %d\n", out);
		outputSelected[out] = 1;
	      }
	  }
      
      break;
    }
  default:
    break;
  }

	

  if(debug_algorithm)
    printf("Algorithm 'pri_mcast_random()' completed for switch %d\n", 
	   aSwitch->switchNumber);
}

/***********************************************************************/

static int
selectInput(aSwitch, output, priority, inputMask)
  Switch *aSwitch;
int output, priority;
int *inputMask;
{
  /*
    Randomly select between all requesting input ports
    */
  int input, switchOutput, index;
  int num_rqsts=0;
  int selected;
  InputBuffer  *inputBuffer;
  Cell *aCell;
  static int * requestors = (int *) NULL;

  switchOutput = output / aSwitch->fabric.Xbar_numOutputLines;

  /* Allocate temporary array to store inputs that request this output */
  if(!requestors)
        requestors = (int *) malloc( aSwitch->numInputs * sizeof(int) );


  /* Build array of requesting input ports for this output. 		*/
  /* i.e which input ports have a cell destined for this output 	*/
  /* and that input has not already accepted any other output 	*/
  for(input=0; input<aSwitch->numInputs; input++)
    {
      if(inputMask[input]) continue;
      inputBuffer = aSwitch->inputBuffer[input];
      if(inputBuffer->mcastFifo[priority]->number) 
	{
	  aCell = (Cell *) inputBuffer->mcastFifo[priority]->head->Object;
	  if( bitmapIsBitSet(output, &aCell->outputs) )
	    requestors[num_rqsts++] = input;
	}
    }

  /* Choose randomly between inputs */
  if(num_rqsts == 0) 
    {
      if(debug_algorithm)
	printf("Output %d received no requests\n", output);
      return(NONE);
    }
  else if(debug_algorithm)
    printf("Output %d received %d requests\n", output, num_rqsts);

  index = nrand48(aSeed)%num_rqsts; /* U[0,num_rqsts-1] */
  selected = requestors[index];
  if(debug_algorithm)
    printf("Output %d granting to input %d\n", output, selected);

  return(selected);
}
