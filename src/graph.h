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


/*******************************************************************************
This file contains all definitions for the stub for the graphing tool
*******************************************************************************/

#ifndef GRAPH_H
#define GRAPH_H

#include <netinet/in.h>
#include <sys/socket.h>
#include "aux.h"
// #include "lists.h"
//#include "sch.h"

#define GRAPH_MAX_PKT_LEN 0xFFFF
#define GRAPH_UNKNOWN_OBJECT    0xffff

typedef enum  {GRAPH_POLLING=1, GRAPH_PERIODIC_REPORT,
			   GRAPH_TRIGGER, GRAPH_STOP_PREVIOUS_REQUEST,
			   ERROR_SIGNALLING=0xFFFF /*if there was an invalid request, the
										 simulator will signal the error with
										 this reqType*/
              } reqTypeT;

typedef enum  {GRAPH_INT_VALUE=1, GRAPH_U_INT_VALUE, GRAPH_LONGLONG_VALUE,
			   GRAPH_DOUBLE_VALUE, 
			   GRAPH_INT_VALUE_MEAN_SD, GRAPH_DOUBLE_VALUE_MEAN_SD,
			   GRAPH_STRING_VALUE} value_typeT;

//PMF added the pragma pack, sothat structs and unions are indeed packed
#pragma pack(1)
#define PACKED  __attribute__((packed)) 

typedef struct 
{
  int value;
  int mean;
  int sd;
} intValueMeanSd_t;

typedef struct {
  char string_[GRAPH_MAX_PKT_LEN];
} doubleValueMaxMinMeanSd_t;

typedef union 
{
  int int_;
  u_int u_int_;
  long long longlong_;
  double double_;
  intValueMeanSd_t intValueMeanSd_;
  doubleValueMaxMinMeanSd_t doubleValueMaxMinMeanSd_;
  char string_[GRAPH_MAX_PKT_LEN];
} graph_value_t PACKED;


/*request packets*/
struct GraphPollReqPktContents
{
  unsigned object;
  /*the format for the object is: 1st byte is the module number, 2nd and 3rd
	bytes are the variable number, 3rd byte is the port number*/
#define BASE_MASK        0xFFFF0000
#define TT_BASE          0x00000
#define GRAPH_BASE       TT_BASE

#define LATENCY_BASE     0xB0000
#define OCCUPANCY_BASE   0xC0000

  
#define VARIABLE_MASK    0x01FFF
#define TROUGHPUT_CODE   0x01
#define Q_OCCUPANCY_CODE 0x02
#define CELL_COUNT_CODE  0x03
#define PKT_COUNT_CODE   0x04
#define NUM_REQUESTS_CODE 0x05
#define VARIABLES_CODE   0x06
#define PAUSE_CODE   0x010
#define CONTINUE_CODE    0x011

#define PRIORITY_SUBMASK 0x06000
#define PRIORITY_SHIFT   25
#define PRIORITY_0       0x00000
#define PRIORITY_1       0x03000
#define PRIORITY_2       0x04000
#define PRIORITY_3       0x06000

#define MULTICAST_SUBMASK 0x08000
#define MULTICAST_CODE    0x08000
#define UNICAST_CODE      0x00000

#define TT_NUM_PORTS         TT_BASE | 0x1
#define TT_NUM_DS            TT_BASE | 0x2
#define SRC_NUM_CELLS_SENT  SRC_BASE | CELL_COUNT_CODE

#define PORTNUM_MASK    0x0000
};

struct GraphPeriodReqPktContents
{
  unsigned object;
  unsigned period; /*in cell times*/
};

typedef enum {GRAPH_GREATER_THAN,
			  GRAPH_GREATER_OR_EQUAL_TO, GRAPH_LESS_THAN, 
			  GRAPH_LESS_OR_EQUAL_TO, GRAPH_EQUAL_TO} thresholdTypeT;

struct GraphTriggerReqPktContents
{
  unsigned int object;
  thresholdTypeT thresholdType; /*threshold type*/
  value_typeT thresholdValue_type; /*threshold value type*/
  graph_value_t thresholdValue PACKED; /*threshold value*/
};

struct GraphStopReqPktContents
{
  unsigned ReqID; /*this is a reference number provided by the graphing
					tool, we have to return it with every response*/  
  reqTypeT reqType;/*polling, periodic, trigger*/
};

struct GraphRequestPacket
{
  unsigned ReqID; /*this is a reference number provided by the graphing
					tool, we have to return it with every response*/  
  unsigned length;
  reqTypeT reqType;/*polling, periodic, trigger or stopPreviousReq*/

  union {
	struct GraphPollReqPktContents poll;
	struct GraphPeriodReqPktContents period;
	struct GraphTriggerReqPktContents trigger;
	struct GraphStopReqPktContents stop;
  } contents PACKED;
};

#define REQ_HDR_SZ 3*sizeof(unsigned)
//#define REQ_PKT_SZ REQ_HDR_SZ + 2*sizeof(unsigned)
#define POLL_REQ_CONTENTS_SZ    sizeof(struct GraphPollReqPktContents)
#define PERIOD_REQ_CONTENTS_SZ  sizeof(struct GraphPeriodReqPktContents)
#define TRIGGER_REQ_CONTENTS_SZ sizeof(struct GraphTriggerReqPktContents)
#define TRIGGER_REQ_CONTENTS_HDR_SZ sizeof(struct GraphTriggerReqPktContents)-sizeof(graph_value_t)
#define STOP_REQ_CONTENTS_SZ    sizeof(struct GraphStopReqPktContents)



/*response packets*/
struct GraphPollRespPktContents
{
  unsigned time_high; /*in cell times*/
  unsigned time_low; 
  /*the time is transmitted as to ints so that it avoid the gaps left by gcc
	when using long long*/
  //  long long time; /*in cell times*/

  value_typeT value_type;
  graph_value_t value;
};

struct GraphPeriodRespPktContents
{
  unsigned time_high; /*in cell times*/
  unsigned time_low; 
  /*the time is transmitted as to ints so that it avoid the gaps left by gcc
	when using long long*/
  //  long long time; /*in cell times*/

  value_typeT value_type;
  graph_value_t value PACKED;
};

struct GraphTriggerRespPktContents
{
  unsigned time_high; /*in cell times*/
  unsigned time_low; 
  /*the time is transmitted as to ints so that it avoid the gaps left by gcc
	when using long long*/
  //  long long time; /*in cell times*/

  value_typeT value_type;
  graph_value_t value;
};

struct GraphResponsePacket
{
  unsigned ReqID; /*this is a reference number provided by the graphing
					tool, we have to return it with every response*/  

  unsigned length;
  reqTypeT reqType;
  union {
	struct GraphPollRespPktContents poll;
	struct GraphPeriodRespPktContents period;
	struct GraphTriggerRespPktContents trigger;
  } contents PACKED;
  
};

//PMF added the pragma pack, sothat structs and unions are indeed packed
#pragma pack()

#define RESP_HDR_SZ 2*sizeof(unsigned)+sizeof(reqTypeT)
#define RESP_PKT_SZ RESP_HDR_SZ + sizeof(unsigned) + sizeof(long long)
#define POLL_RESP_CONTENTS_SZ     sizeof(struct GraphPollRespPktContents)
#define PERIOD_RESP_CONTENTS_SZ   sizeof(struct GraphPeriodRespPktContents)
#define TRIGGER_RESP_CONTENTS_SZ  sizeof(struct GraphTriggerRespPktContents)
//#define POLL_RESP_CONTENTS_HDR_SZ    sizeof(unsigned) + sizeof(long long)
//#define PERIOD_RESP_CONTENTS_HDR_SZ  sizeof(unsigned) + sizeof(long long)
//#define TRIGGER_RESP_CONTENTS_HDR_SZ sizeof(unsigned) + sizeof(long long)
#define POLL_RESP_CONTENTS_HDR_SZ    sizeof(struct GraphPollRespPktContents)-sizeof(graph_value_t)
#define PERIOD_RESP_CONTENTS_HDR_SZ  sizeof(struct GraphPeriodRespPktContents)-sizeof(graph_value_t)
#define TRIGGER_RESP_CONTENTS_HDR_SZ sizeof(struct GraphTriggerRespPktContents)-sizeof(graph_value_t)

//#define PERIOD_RESP_CONTENTS_HDR_SZ  sizeof(unsigned) * 2 + sizeof(value_typeT)

/*internal structures*/

struct GraphRequest
{
  unsigned ReqID; /*this is a reference number provided by the graphing
					tool, we have to return it with every response*/  
  unsigned object;
  unsigned objectNum;
  
};

struct GRAPH
{
  struct TT *tt;             

#define DEFAULT_MAX_NUM_REQS 256
  int maxNumReqs;            
  struct GraphRequest *GRequests;
  
  int numReqsActive;
  int numReqsReceived;

  int listenSock, dataSock;
  int TCPportNum;               
#define DEFAULT_TCP_PORT 6968
  struct sockaddr_in clientAddr;
  int state;
#define GRAPH_LISTENING 0
#define GRAPH_ACCEPTED 1
  
  struct GraphRequestPacket in_packet;
  struct GraphResponsePacket out_packet;
  
  struct List *listOfRequests;

  int paused;
};

/*******************************************************************************
  graph_init(struct GRAPH *graph)

  This function will initialize any internal variable or structure.
  It is called from the config.c file, before the configuration file is
  parsed.
*******************************************************************************/

void graph_init(struct GRAPH *graph);

void graph_end(struct GRAPH *graph);

/*******************************************************************************
  graph_config(void *tt, int from, int to, char *var, char *value)

  This function will configure various parameters in the graph stub structure.
  It is called from the config.c file, and so the instance numbers, variable,
  and value strings have already been parsed.
*******************************************************************************/

void graph_config(void *tt, int from, int to, char *var, char *value);


/*******************************************************************************
  graph_start(struct GRAPH *graph)

  This function starts all state of the graph stub that was dependent on the
  configuration parameters.  It is called once, at the beginning of the 
  simulation, once the module has been configured.
*******************************************************************************/
void graph_start(struct GRAPH *graph);

/*******************************************************************************
  graph_check_socket_ready(struct GRAPH *graph, int num_times)

  This function will check if the socket that has been opened by graph has
  any outstanding connection or request. If so, it will call the
  corresponding routine to service that request. It will do this at most
  num_times before returning.
*******************************************************************************/
void graph_check_socket_ready(void *txI, void *rxI, void *arg);


/*******************************************************************************
	graph_pause_on_socket(void *txI, void *rxI, void *arg)

  This function will block listening to the socket waiting for an incoming
  continue command from the graphing tool.
*******************************************************************************/

 void graph_pause_on_socket(void *txI, void *rxI, void *arg); 
																		/* Sundar */

/*******************************************************************************
  graph_read_request(struct GRAPH *graph)

  This function will cread a single request from the socket and it will
  process it.
*******************************************************************************/
void   graph_read_request(struct GRAPH *graph);

/*******************************************************************************
  graph_send_response(struct GRAPH *graph, unsigned ReqID, 
					  unsigned reqType, unsigned value, long long ttime)

  This function will send a response to a previous request through a
  socket.
*******************************************************************************/
void graph_send_response(struct GRAPH *graph, unsigned ReqID, 
						 unsigned reqType, graph_value_t *value, unsigned len,
						 unsigned value_type, long long ttime);

/*******************************************************************************
  graph_hton_parameter(graph_value_t *value_out, graph_value_t *value_in,
						   unsigned value_type, unsigned len)

  This function will change the host to network ordering of the given
  value, of type value_type and length len.
*******************************************************************************/
void  graph_hton_parameter(graph_value_t *value_out, graph_value_t *value_in, 
						   unsigned value_type, unsigned len);

/*******************************************************************************
  graph_close_socket(struct GRAPH *graph)

  This function will close the data socket.
*******************************************************************************/
void graph_close_socket(struct GRAPH *graph);

/*******************************************************************************
  graph_periodicReporting(void *txI, void *rxI, void *arg)

  This event handler will wake upo periodically and it will get the
  results and send them to the graphing tool.
*******************************************************************************/
void graph_periodicReporting(void *txI, void *rxI, void *arg);

/*******************************************************************************
  graph_process_request(struct GRAPH *graph, unsigned object,
							 unsigned objectNum, unsigned ReqID, 
							 unsigned reqType)

  This function will process a single poll or periodic request and send
  the response back. 
*******************************************************************************/
void   graph_process_request(struct GRAPH *graph, unsigned object,
							 unsigned objectNum, unsigned ReqID, 
							 unsigned reqType);

/*******************************************************************************
  graph_get_object_value(struct GRAPH *graph,
                       unsigned object, unsigned objectNum, 
                       graph_value_t *value, unsigned *length)

  Reads the value of object objectNum, and then returns its value and length.
  If no error occurs, a NULL is returned.
*******************************************************************************/
int graph_get_object_value(struct GRAPH *graph, 
						   unsigned object, unsigned objectNum, 
						   graph_value_t *value, unsigned *value_type, 
						   unsigned *length);

/*******************************************************************************
  graph_get_object_name(struct GRAPH *graph, 
						   char *nameStr, int *length)

  Writes the variables that this modules supports into nameStr if there is
  enough space left.
*******************************************************************************/
void graph_get_object_name(struct GRAPH *graph,
						   char *nameStr, int *length);

#endif //GRAPH_H
