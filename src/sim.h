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
#include <unistd.h> // for getopts
#include <string.h> //for memset operations
#include <sys/types.h>
#include <math.h>
#include "bitmap.h"
#include "stat.h"
#include "histogram.h"
#include "lists.h"
#include "switchStats.h"
#include "types.h"
#include "latencyStats.h" 
#include "functionTable.h" 


#define debug_sim 0
#define debug_memory 0
#define debug_cell 0

#define safe_sqrt(x)  (((x) < 0)? (0): sqrt((x)))

extern Switch	    *createSwitch();
extern InputBuffer  *createInputBuffer();
extern OutputBuffer *createOutputBuffer();
extern InputBuffer  *createLiteInputBuffer();
extern OutputBuffer *createLiteOutputBuffer();

extern Cell     *createCell();
extern void     destroyCell();
extern Cell    *createMulticastCell();
extern Cell    *copyCell();
extern double   erand48();
extern double   drand48();
extern long   lrand48();
extern long   nrand48();
extern long   mrand48();
extern long   jrand48();
extern void   srand48();


extern Switch **switches;
extern int numSwitches;
extern long simulationLength;

