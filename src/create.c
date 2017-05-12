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

#include "sim.h"
#include <string.h>

extern void FatalError(); /* in sim.c */
Switch *
createSwitch( number, inputs, outputs, priorities )
  int number, inputs, outputs, priorities;
{
  int input, output;
  Switch *aSwitch;

  aSwitch = (Switch *) malloc( sizeof( Switch ) );	
  if( aSwitch == NULL )
    FatalError("createSwitch(): Malloc failed.\n");
  memset(aSwitch, 0, sizeof(Switch));

  aSwitch->switchNumber  = number;
  aSwitch->numInputs  = inputs;
  aSwitch->numOutputs = outputs;
  aSwitch->numPriorities = priorities;   
	
  aSwitch->inputBuffer = (InputBuffer **)malloc(inputs*sizeof(InputBuffer *));
  if( aSwitch->inputBuffer == NULL )
    FatalError("createSwitch(): Malloc failed for inputs.\n");

  for( input=0; input<inputs; input++ )
    aSwitch->inputBuffer[input] = createInputBuffer( input, outputs, priorities );  

  aSwitch->outputBuffer = (OutputBuffer **) 
    malloc(outputs*sizeof(OutputBuffer *));
  if( aSwitch->outputBuffer == NULL )
    FatalError("createSwitch(): Malloc failed for outputs.\n");
  for( output=0; output<outputs; output++ )
    aSwitch->outputBuffer[output] = createOutputBuffer(output, priorities);

  aSwitch->scheduler.schedulingAlgorithm = (void (*)()) NULL;
  aSwitch->scheduler.schedulingStats = (void *) NULL;
  aSwitch->scheduler.schedulingState = (void *) NULL;
  aSwitch->fabric.fabricState = (void *) NULL;
  aSwitch->inputAction = (void *) NULL;
  aSwitch->inputActionState = (void *) NULL;
  aSwitch->outputAction = (void *) NULL;
  aSwitch->outputActionState = (void *) NULL;

  return(aSwitch);

}

InputBuffer *
createInputBuffer( input, outputs, priorities )
  int input, outputs, priorities;
{
  InputBuffer *inputBuffer;
  int output, pri; 	
  char inputBufferName[256];
  int numFIFOs;    

  inputBufferName[0] = '\0';
  inputBuffer = (InputBuffer *) malloc( sizeof( InputBuffer ) );
  if( inputBuffer == NULL )
    FatalError("createInputBuffer(): Malloc failed.\n");

  inputBuffer->traffic = NULL;

  numFIFOs = outputs * priorities;   
  inputBuffer->fifo = (struct List **) malloc( numFIFOs*sizeof(struct List *));
  if( inputBuffer->fifo == NULL )
    FatalError("createInputBuffer(): Malloc failed.\n");

  for( output=0; output<numFIFOs; output++ )
    {
      sprintf(inputBufferName, "Input Buffer (%d,%d), pri=%d", input, output/priorities, output%priorities);
      inputBuffer->fifo[output] = createList(inputBufferName);
    }

  inputBuffer->mcastFifo = (struct List **) malloc( priorities*sizeof(struct List *));
  if( inputBuffer->mcastFifo == NULL )
    FatalError("createInputBuffer(): Malloc failed.\n");
  
  for( pri=0; pri<priorities; pri++ )
    {
      sprintf(inputBufferName, "Input %d mcast buffer, pri=%d", input,pri);
      inputBuffer->mcastFifo[pri] = createList(inputBufferName);
    }
  
  
  return( inputBuffer );
	
}

OutputBuffer *
createOutputBuffer(output, priorities)
  int output, priorities;
{
  OutputBuffer *outputBuffer;
  char outputBufferName[256];
  int pri;

  outputBufferName[0] = '\0';

  outputBuffer = (OutputBuffer *) malloc( sizeof( OutputBuffer ) );
  if( outputBuffer == NULL )
    FatalError("createOutputBuffer(): Malloc failed.\n");

  outputBuffer->fifo = (struct List **) malloc( priorities*sizeof(struct List *)); 
  for( pri=0; pri<priorities; pri++ )                       /* TL */
    {
      sprintf(outputBufferName, "Output Buffer %d, pri=%d", output,pri);
      outputBuffer->fifo[pri] = createList(outputBufferName);
    }
  return( outputBuffer );

}

