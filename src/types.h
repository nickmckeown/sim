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

#ifndef _TYPES_H
#define _TYPES_H

#define MAXSTRING 512

typedef enum {
  CONTINUE_SIMULATION=0,
  STOP_SIMULATION=1,
} SimulationRunState;

typedef enum {NO, YES} boolean;
#define NONE (-1)
#define ALL  (-2)

#define UCAST (0)
#define MCAST (1)

/* Global time, in cells, since simulation began */
extern long now;
extern long resetStatsTime;

/* Structure of a Cell */
typedef struct Cell *CellPtr;
typedef struct {
  int vci;
  int priority;   
  int multicast;	/* Set if cell is multicast. */
  Bitmap outputs;	/* Which outputs to send this cell to at this switch */

  /* Switch dependent information, derived from VCI and specific to switch.*/
  void *switchDependentHeader; 

  /* Input action dependent information */
  void *inputActionHeader;
  

  /**********************************************************/
  /****************** Cell statistics. **********************/
  /**********************************************************/
  /* Global stats common to all cells and switches. */
  struct {
    unsigned long ID; 		/* Unique ID among all created cells. */
    long createTime;
    long arrivalTime;		/* Arrival time at current switch */
    long fabricArrivalTime;	/* Arrival time at switch fabric */
    long outputArrivalTime;	/* Arrival time at output queue */
    long headArrivalTime;	/* Arrival time at head of input queue */
    long dequeueTime; /* Time when cell is actually dequeued (Multicast) */
    int  numSwitchesVisited; 
    int  inputPort; 		/* Arrival port at current switch. */
    long  oqDepartTime;             /* Departure time if using output queueing. */

  } commonStats;
  
  /* Fabric Specific Statistics. Created, updated and destroyed by fabric. */
  void *fabricStats;
  
  /* Scheduling Algorithm Specific Statistics. */
  /* Created, updated and destroyed by scheduling algorithm. */
  void *algorithmStats;
  
} Cell;

/****************************************************************/
/******* Structure and Elements of a Switch *********************/
/****************************************************************/

/* Structure of input buffer for each switch */
typedef struct InputBuffer *InputBufferPtr;
typedef struct {
  struct List **fifo; 	/* Input fifos: one per output. */
  
  void *traffic; 			/* Placeholder for traffic stats for this input */
  int (*trafficModel)();  /* Traffic generating function */
  Stat bufferStats;		/* Aggregate occupancy statistics for this input. */
  struct Histogram bufferHistogram;	/* Agg occupancy statistics for this input. */
  int  numOverflows;		/* Total number of cells dropped at this input. */
  
  struct List **mcastFifo;		/* Multicast queue for this input. */

} InputBuffer;

/* Scheduling Algorithm for this switch. */
typedef struct {
  void 		(*schedulingAlgorithm)();
  void 		*schedulingState;	/* Algorithm dependent state      */	
  void		*schedulingStats;	/* Algorithm dependent statistics */ 		
  
  void 		(*mcast_schedulingAlgorithm)();
  void 		*mcast_schedulingState;	/* Algorithm dependent state      */	
  void		*mcast_schedulingStats;	/* Algorithm dependent statistics */ 		
} Scheduler;

typedef struct {
  int input;		/* Input that this output will be connected to. */
  Cell *cell;	/* Cell that is to be transferred. */
} MatrixEntry;

typedef struct {

  /* Function that moves cell from input fifos to fabric */
  /* and from fabric to output queues. */
  void (*fabricAction)();
  
  void *fabricState; /* Fabric dependent state for this switch. */
  
  /*******************************/
  /* Description of interconnect */
  /*******************************/
  union {
    /* Crossbar fabric: Foreach output, which input is connected */
    struct {
      int NumOutputs;
      MatrixEntry	*Matrix; 
    } Crossbar;
#define Xbar_matrix Interconnect.Crossbar.Matrix
#define Xbar_numOutputLines Interconnect.Crossbar.NumOutputs

    /* Other interconnects (e.g buffered Banyan) will go here. */
  } Interconnect;

  void *fabricStats;	/* Fabric dependent stats for this switch */
  
} Fabric;

/* Structure of output buffer for each switch */
typedef struct OutputBuffer *OutputBufferPtr;
typedef struct {
  struct List **fifo;           /* add priorities to the output buffer queues */ 
  BurstStat burstinessStats;	/* Burstiness statistics for this buffer */
} OutputBuffer;

/* Structure of a switch */
typedef struct {
  int switchNumber;	 /* Unique switch identifier. 		*/
  int numInputs;  	 /* Number of inputAction.		*/
  int numOutputs; 	 /* Number of outputActions. 	*/
  int numPriorities;     /* Number of levels of priority */

  /***************************************/
  /********** Components of switch. ******/
  /***************************************/
  InputBuffer	**inputBuffer;	 /* numInput Input buffers.			*/
  Scheduler   scheduler;		 /* Scheduling Algorithm. 			*/
  Fabric		fabric;
  OutputBuffer **outputBuffer; /* numOutput Output buffers.		*/

  /******************************************************/
  /* Actions for cells entering and exiting from switch */
  /******************************************************/
  Cell * (*inputAction)();	/* Action for adding cells to input. */
  void *inputActionState; /* Action dependent state for this switch. */
  void (*outputAction)();	/* Action for removing cells from output */
  void *outputActionState;/* Action dependent state for this switch. */
  
  /***************************************/
  /****** Statistics place holder ********/
  /***************************************/
  /* Statistics over all input buffers. */
  Stat 		aggregateInputBufferStats;	 
  /* Statistics over all output buffers. */
  Stat 		aggregateOutputBufferStats;	 
  /* Statistics over all buffers. */
  Stat 		aggregateBufferStats;	 
  
  struct {
    Stat inputLatency; 			/* Queueing + service delay in input. */
    Stat fabricLatency;			/* Delay through fabric. */
    Stat outputLatency;			/* Queueing delay in output. */
    Stat switchLatency;			/* Delay through switch. */
    Stat avgSchLatency;			/* Scheduling delay of a cell */
    Stat avgHolLatency;			/* Delay at HOL before being dequeued*/
    Stat avgInputLatency; 
    Stat avgOutputLatency;
    
    Stat *schedulingLatency; /* Scheduling delay of a cell (per i/p Q) */
    Stat *holLatency; /* Delay at HOL (per i/p Q)*/
    Stat *priInputLatencyUcast;           /* Input latency for each priority, ucast */
    Stat *priInputLatencyMcast;           /* Input latency for each priority, mcast */
    Stat *priOutputLatencyUcast;           /* Output latency for each priority, ucast */
    Stat *priOutputLatencyMcast;           /* Output latency for each priority, mcast */
  } latencyStats;
  
} Switch;

#endif
