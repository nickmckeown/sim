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
  graph.c

  This file contains the code for the stub for the graphing tool.  
*******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <assert.h>
#include <ctype.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>

#include "sim.h"
#include "graph.h"

#ifndef SHUT_RD
#define SHUT_RD 0
#endif

#ifndef SHUT_WR
#define SHUT_WR 1
#endif

#ifndef SHUT_RDWR
#define SHUT_RDWR 2
#endif

#ifdef DEBUG_ALL
#define DEBUG_GRAPH
#endif


#define READ_BLOCK(graph, sock, p, len, recvLen) \
	while (len) { \
	  if ((recvLen = recv(sock, p, len, 0)) < 0) { \
	    err_ret("GRAPH: can't recv packet (%d)\n", errno); \
	    graph_close_socket((struct GRAPH *)graph); \
	    return; \
	  } \
      len -= recvLen; \
      p += recvLen; \
    } 

extern int latency_get_object_value();
extern int latency_get_object_name();
extern int occupancy_get_object_value();
extern int occupancy_get_object_name();

extern long int now;
extern struct GRAPH graphTool; // Amr
long simGraphPauseTime = -1;
extern Switch **switches;
#define create_GraphRequestPacket() (struct GraphRequestPacket*) malloc(sizeof(struct GraphRequestPacket))
#define destroy_GraphRequestPacket free

/*******************************************************************************
  graph_init(struct GRAPH *graph)

  This function will initialize any internal variable or structure.
  It is called from the config.c file, before the configuration file is
  parsed.
*******************************************************************************/

void graph_init(struct GRAPH *graph)
{
  char     queueName[256];

  graph->maxNumReqs =  DEFAULT_MAX_NUM_REQS;
  graph->TCPportNum = DEFAULT_TCP_PORT;
  graph->numReqsActive = 0;
  graph->numReqsReceived = 0;
  graph->paused = 0;

  /*create lists*/  
  sprintf(queueName, "List of periodic and triggered requests");
  graph->listOfRequests = createList(queueName);
  // resetList(graph->listOfRequests);
  graph->listOfRequests->maxNumber = graph->maxNumReqs;

}


/*******************************************************************************
  graph_config(void *tt, int from, int to, char *var, char *value)

  This function will configure various parameters in the dataslice structure.
  It is called from the config.c file, and so the instance numbers, variable,
  and value strings have already been parsed.
*******************************************************************************/

void graph_config(void *tt, int from, int to, char *var, char *value)
{
  struct GRAPH *graph = tt;

  if ((from < 0) || (from > to) || (to > 0))
    err_quit("Tried to config GRAPH that does not exist.\n");

  if (!strncmp(var, "maxNumReqs", 11))
    {
      if (sscanf(value, "%d", &graph->maxNumReqs) != 1)
        err_quit("GRAPH: Cannot read maxNumReqs.\n");
    }
  else if (!strncmp(var, "TCPportNum", 11))
    {
      if (sscanf(value, "%d", &graph->TCPportNum) != 1)
        err_quit("GRAPH: Cannot read TCPportNum.\n");
    }    
  else       
    err_quit("Unknown variable %s in GRAPH config\n", var);
  
}


/*******************************************************************************
  graph_start(struct GRAPH *graph)

  This function starts all data slice state that was dependent on the
  configuration parameters.  It is called once, at the beginning of the 
  simulation, once the module has been configured.
*******************************************************************************/
void graph_start(struct GRAPH *graph)
{
  struct sockaddr_in sockAddr;
  
#ifdef DEBUG_GRAPH
	printf("GRAPH config: maxNumReqs = %d\n", graph->maxNumReqs);
#endif

  if (graph->maxNumReqs == 0)
    err_quit("Must supply all GRAPH'es with maxNumRequests!!\n");

  graph->GRequests = (struct GraphRequest *)
	malloc(sizeof(struct GraphRequest) * graph->maxNumReqs);

  memset(graph->GRequests, 0, sizeof(struct GraphRequest) * graph->maxNumReqs);

  /*now start the socket */
  if ((graph->listenSock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	err_quit("GRAPH: can't create socket (%d)", errno);
  }

  memset(&sockAddr, 0, sizeof(sockAddr));
  
  sockAddr.sin_family = AF_INET;
  sockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  sockAddr.sin_port = htons(graph->TCPportNum);
  
  if ((bind(graph->listenSock, (struct sockaddr *)&sockAddr, sizeof(sockAddr))) < 0) {
	err_quit("GRAPH: can't bind socket (%d)", errno);
  }
	
#ifdef DEBUG_GRAPH
  printf("GRAPH: listening...\n");
#endif
  listen(graph->listenSock, 5);
  graph->state = GRAPH_LISTENING;

}

/*******************************************************************************
  graph_end(struct GRAPH *graph)

  This function will end the GRAPH.  It is called once per GRAPH,
  at the end of the simulation.
*******************************************************************************/
void graph_end(struct GRAPH *graph) 
{
  /*
   *  We're done! 
   */
  printf("GRAPH: FINISHED, %d requests active\n", graph->numReqsActive);
}

/*******************************************************************************
  graph_check_socket_ready(void *txI, void *rxI, void *arg)

  This function will check if the socket that has been opened by graph has
  any outstanding connection or request. If so, it will call the
  corresponding routine to service that request. It will do this at most
  num_times before returning.
*******************************************************************************/
void graph_check_socket_ready(void *txI, void *rxI, void *arg)
{
  /*  struct TT *tt = (struct TT*)txI; */
  struct GRAPH *graph = (struct GRAPH*)rxI;
  fd_set rfds;
  struct timeval tv;
  int retval, cliAddrSize;

  
  FD_ZERO(&rfds);
  /* Don't block. */
#ifdef GRAPH_CLEAR_COUNTS
  tv.tv_sec = 0;
  tv.tv_usec = 100000;
#else
  tv.tv_sec = 0; //slow down a little bit
  tv.tv_usec = 0;
#endif

  if(graph->state == GRAPH_LISTENING) {
    /*accept incoming connection*/
    /* Watch graph->listenSock to see when it has input. */
    FD_SET(graph->listenSock, &rfds);
		
    if ((retval = select(graph->listenSock + 1, &rfds, NULL, NULL, &tv))) {
#ifdef DEBUG_GRAPH
      printf("GRAPH: accepting connection.\n");
      assert(FD_ISSET(graph->listenSock, &rfds));
#endif
      cliAddrSize = sizeof(graph->clientAddr);
      if ((graph->dataSock = 
           accept(graph->listenSock, (struct sockaddr *)&graph->clientAddr, 
                  &cliAddrSize))
          < 0) {
        err_ret("GRAPH: can't accept connection (%d)", errno);
      }
      graph->state = GRAPH_ACCEPTED;
    } else {
#ifdef DEBUG_GRAPH
      printf("GRAPH: No connection.\n");
#endif
    }
	  
  } else {
    /*read incoming data*/
    /* Watch graph->dataSock to see when it has input. */
    FD_SET(graph->dataSock, &rfds);
	
    if ((retval = select(graph->dataSock + 1, &rfds, NULL, NULL, &tv))) {
      /* Don't rely on the value of tv now! */
#ifdef DEBUG_GRAPH
      printf("GRAPH: Data is available now.\n");
      assert(FD_ISSET(graph->dataSock, &rfds));
#endif
      graph_read_request(graph);
    } else {
#ifdef DEBUG_GRAPH
      //	  printf("GRAPH: No data.\n");
#endif
    }
  }
  
}

/*******************************************************************************
  graph_pause_on_socket(void *txI, void *rxI, void *arg)

  This function will block listening to the socket waiting for an incoming 
  continue command from the graphing tool.
*******************************************************************************/
void graph_pause_on_socket(void *txI, void *rxI, void *arg)
{
  /*  struct TT *tt = (struct TT*)txI; */
  struct GRAPH *graph = (struct GRAPH*)rxI;
  fd_set rfds;
  int retval;

  graph->paused = 1;

#ifdef DEBUG_GRAPH
  printf("GRAPH: graph_pause_on-socket()\n");
#endif
  
  FD_ZERO(&rfds);

  if(graph->state != GRAPH_ACCEPTED){
#ifdef DEBUG_GRAPH
    printf("GRAPH: cannot wait for a continue command if not connected.\n");
#endif
  } else{

    while(graph->paused) {
#ifdef DEBUG_GRAPH
      printf("GRAPH: waiting for a continue command...\n");
#endif
      /* Watch graph->dataSock to see when it has input. */
      FD_SET(graph->dataSock, &rfds);
	
      if ((retval = select(graph->dataSock + 1, &rfds, NULL, NULL, NULL))) {
        /* Don't rely on the value of tv now! */
#ifdef DEBUG_GRAPH
        printf("GRAPH: Data is available now.\n");
        assert(FD_ISSET(graph->dataSock, &rfds));
#endif
        graph_read_request(graph);
      }
    }
  }
  
}


/*******************************************************************************
  graph_delete_request(struct GRAPH *graph)

  This function will delete a previous request that was registered in the
  event list.
*******************************************************************************/
void graph_delete_request(struct GRAPH *graph, int ReqID, reqTypeT ReqType)
{
  struct GraphRequestPacket *pkt;
  struct Element *anElement;
  
#ifdef DEBUG_GRAPH
  printf("GRAPH: graph_delete_request: ReqID: %d, ReqType: %d)\n", 
     ReqID, ReqType); 
#endif

  if (ReqType == GRAPH_PERIODIC_REPORT) {
	
    /* we have to eliminate a request of ID ReqID from the event list*/

    for (anElement = graph->listOfRequests->head; anElement;  
         anElement = anElement->next) {
      pkt =
        (struct GraphRequestPacket *)anElement->Object;
	    if (pkt->ReqID == ReqID) {
        printf("GRAPH: found matching req in list of req: ReqID: %d, ReqType: %d\n", 
               ReqID, ReqType); 
        pkt = deleteObject(graph->listOfRequests, pkt);
        free(pkt);
        break;
      }
    }

  }
  
}

/*******************************************************************************
  graph_read_request(struct GRAPH *graph)

  This function will read a single request from the socket and it will
  process it.
*******************************************************************************/
void graph_read_request(struct GRAPH *graph)
{
  int len, recvLen;
  struct GraphRequestPacket *pkt, *pkt2;
  char *p;
  int sock = graph->dataSock;
  register unsigned object, period;
  graph_value_t aux_value;
  char *msg = aux_value.string_;
  struct Element *anElement;
  int ReqID;
  

  pkt2 = create_GraphRequestPacket();
  pkt = &graph->in_packet;
  assert(pkt2 && pkt);
  
  /*first read the type of request*/  
  p = (char *)pkt;
  len = 3*sizeof(unsigned);
  READ_BLOCK(graph, sock, p, len, recvLen);

  pkt2->ReqID = ntohl(pkt->ReqID);
  pkt2->length = ntohl(pkt->length);
  pkt2->reqType = ntohl(pkt->reqType);

  //  graph->numReqsReceived++;

  switch (pkt2->reqType) {
  case GRAPH_POLLING:
	assert (POLL_REQ_CONTENTS_SZ + REQ_HDR_SZ <= pkt2->length);
	p = (char *)&pkt->contents;
	len = pkt2->length - REQ_HDR_SZ;
	READ_BLOCK(graph, sock, p, len, recvLen);
	
	object = pkt2->contents.poll.object = ntohl(pkt->contents.poll.object);
	graph_process_request(graph, object & (BASE_MASK | VARIABLE_MASK),
						  object & PORTNUM_MASK, pkt2->ReqID, 
						  GRAPH_POLLING);
#ifdef DEBUG_GRAPH
	printf("GRAPH: graph_read_request(POLLING, ReqID: %d, object: %d)\n", 
		   pkt2->ReqID, object); 
#endif
	destroy_GraphRequestPacket(pkt2);	
	
	break; 
  case GRAPH_PERIODIC_REPORT:
	assert (PERIOD_REQ_CONTENTS_SZ + REQ_HDR_SZ <= pkt2->length);
	p = (char *)&pkt->contents;
	len = pkt2->length - REQ_HDR_SZ;
	READ_BLOCK(graph, sock, p, len, recvLen);

	object = pkt2->contents.period.object = ntohl(pkt->contents.period.object);
	period = pkt2->contents.period.period = ntohl(pkt->contents.period.period)*CELL_TIME;
	
#ifdef DEBUG_GRAPH
    printf("GRAPH: graph_read_request(PERIODIC, ReqID: %d, object: %d, period: %d)\n", 
           pkt2->ReqID, object, period); 
#endif
    /*process it*/
	if (((object & BASE_MASK) == TT_BASE) && 
	    ((object & VARIABLE_MASK) == PAUSE_CODE)){
    /*reschedule for next cell time*/
    simGraphPauseTime = now + period * CELL_TIME;
    
#ifdef DEBUG_GRAPH
    printf("GRAPH: graph_read_request setting a pause in %d cell times at " 
           PRINT_TIME_FORMAT"\n", period, FORMAT_TIME(now)); 
#endif
    destroy_GraphRequestPacket(pkt2);
  } else if (((object & BASE_MASK) == TT_BASE) && 
	     ((object & VARIABLE_MASK) == CONTINUE_CODE)){
#ifdef DEBUG_GRAPH
    printf("GRAPH: graph_read_request continuing with the simulation at " 
           PRINT_TIME_FORMAT"\n", FORMAT_TIME(now)); 
#endif
    graph->paused = 0;
    destroy_GraphRequestPacket(pkt2);
  } else {
    anElement = createElement(pkt2);
    assert(anElement);
    if (addElement(graph->listOfRequests, anElement) == LIST_ERROR_FULL)
      err_quit("GRAPH: listOfRequests is full. QueueSize=%d\n",
               graph->listOfRequests->number);
    
    /*process it*/
  }
  
	break;
  case GRAPH_TRIGGER:
	assert (TRIGGER_REQ_CONTENTS_SZ + REQ_HDR_SZ <= pkt2->length);
	p = (char *)&pkt->contents;
	len = pkt2->length - REQ_HDR_SZ;
	READ_BLOCK(graph, sock, p, len, recvLen);

	pkt2->contents.trigger.object = ntohl(pkt->contents.trigger.object);
	pkt2->contents.trigger.thresholdType = 
	  ntohl(pkt->contents.trigger.thresholdType);
	graph_hton_parameter(&pkt2->contents.trigger.thresholdValue, 
						 &pkt2->contents.trigger.thresholdValue,
						 pkt2->contents.trigger.thresholdType, 
						 pkt2->length - REQ_HDR_SZ - TRIGGER_REQ_CONTENTS_HDR_SZ);


	err_quit("GRAPH: the trigger type hasn't been implemented yet");
	sprintf(msg,"GRAPH: the trigger type hasn't been implemented yet\n" );
	graph_send_response(graph, pkt2->ReqID, 
						ERROR_SIGNALLING, &aux_value, strlen(msg),
						GRAPH_STRING_VALUE, now);
#ifdef DEBUG_GRAPH
	  printf("GRAPH: graph_read_request(TRIGGER, ReqID: %d, object: %d)\n", 
			 pkt2->ReqID, object); 
#endif
	destroy_GraphRequestPacket(pkt2);
	break;
  case GRAPH_STOP_PREVIOUS_REQUEST:
	assert (STOP_REQ_CONTENTS_SZ + REQ_HDR_SZ <= pkt2->length);
	p = (char *)&pkt->contents;
	len = pkt2->length - REQ_HDR_SZ;
	READ_BLOCK(graph, sock, p, len, recvLen);
	
	ReqID = pkt2->contents.stop.ReqID = ntohl(pkt->contents.stop.ReqID);
	object = pkt2->contents.stop.reqType = ntohl(pkt->contents.stop.reqType);

	graph_delete_request(graph, ReqID, object);
	

#ifdef DEBUG_GRAPH
	printf("GRAPH: graph_read_request(STOP_PREV_REQ, ReqID: %d, object: %d, ReqID: %d)\n", 
		   pkt2->ReqID, object, ReqID); 
#endif
	destroy_GraphRequestPacket(pkt2);	
	
	break; 
  default:
	err_quit("GRAPH: unknown request type (%d)\n", pkt2->reqType);
	sprintf(msg, "GRAPH: unknown request type (%d)\n", pkt2->reqType);
	graph_send_response(graph, pkt2->ReqID, 
						ERROR_SIGNALLING, &aux_value, strlen(msg),
						GRAPH_STRING_VALUE, now);
	destroy_GraphRequestPacket(pkt2);
	break;
  }
  
}

/*******************************************************************************
  graph_send_response(struct GRAPH *graph, unsigned ReqID, 
					  unsigned reqType, unsigned value, long long time)

  This function will send a response to a previous request through a
  socket. value is a parameter of type value_type and length len.
*******************************************************************************/
void graph_send_response(struct GRAPH *graph, unsigned ReqID, 
						 unsigned reqType, graph_value_t *value, unsigned len,
						 unsigned value_type, long long ttime)
{
  struct GraphResponsePacket *pkt = &graph->out_packet;
  int sock = graph->dataSock;
  char *p;
  int length=0, sentLen;
  graph_value_t aux1;

  assert((value_type != GRAPH_INT_VALUE) || (len == sizeof(unsigned)));

  pkt->ReqID = htonl(ReqID);
  pkt->reqType = htonl(reqType);  
  aux1.longlong_ = (long long)FORMAT_TIME(ttime);
  switch (reqType) {
  case GRAPH_POLLING:
	length = RESP_HDR_SZ + POLL_RESP_CONTENTS_HDR_SZ + len;
	pkt->length = htonl(length);
	graph_hton_parameter(&pkt->contents.poll.value, value, 
						 value_type, len);
	pkt->contents.poll.value_type = htonl(value_type);
	pkt->contents.poll.time_high = htonl((int) (aux1.longlong_ >> 32));
	pkt->contents.poll.time_low = htonl((int) (aux1.longlong_));
#ifdef DEBUG_GRAPH
	printf("GRAPH:  time: %d + %d ("
		   PRINT_TIME_FORMAT ")\n", 
		   ntohl(pkt->contents.poll.time_high), ntohl(pkt->contents.poll.time_low), FORMAT_TIME(ttime)); 
	if( value_type == GRAPH_INT_VALUE) {
	  printf("GRAPH: graph_send_response(POLLING, ReqID: %d, value: %d, INT, time: "
			 PRINT_TIME_FORMAT ")\n", 
			 ReqID, value->int_, FORMAT_TIME(ttime)); 
	  assert(len == sizeof(int));
	} else if( value_type == GRAPH_STRING_VALUE) {
	  printf("GRAPH: graph_send_response(POLLING, ReqID: %d, len: %d (%d) => %d, value: %s, STR, time: "
			 PRINT_TIME_FORMAT ")\n", 
			 ReqID, len, strlen(value->string_), length, (char*)value, FORMAT_TIME(ttime)); 
	} else if( value_type == GRAPH_DOUBLE_VALUE_MEAN_SD) {
	  printf("GRAPH: graph_send_response(POLLING, ReqID: %d, len: %d (%d), value: %s, VMMMS, time: "
			 PRINT_TIME_FORMAT ")\n", 
			 ReqID, len, strlen(value->string_), (char*)value, FORMAT_TIME(ttime)); 
	}
#endif
	break; 
  case GRAPH_PERIODIC_REPORT:
	length = RESP_HDR_SZ + PERIOD_RESP_CONTENTS_HDR_SZ + len;
	pkt->length = htonl(length);
	graph_hton_parameter(&pkt->contents.period.value, value, 
						 value_type, len);
	pkt->contents.period.value_type = htonl(value_type);
	pkt->contents.period.time_high = htonl((int) (aux1.longlong_ >> 32));
	pkt->contents.period.time_low = htonl((int) (aux1.longlong_));
#ifdef DEBUG_GRAPH
	printf("GRAPH:  time: %lld = %d + %d ("
		   PRINT_TIME_FORMAT ")\n", 
		   aux1.longlong_, ntohl(pkt->contents.period.time_high),
		   ntohl(pkt->contents.period.time_low), FORMAT_TIME(ttime)); 
	printf("GRAPH: length %d = %d + %d + %d\n", 
		   length, RESP_HDR_SZ, PERIOD_RESP_CONTENTS_HDR_SZ, len);
	if((value_type == GRAPH_INT_VALUE) || ( value_type == GRAPH_U_INT_VALUE)) {
	  printf("GRAPH: graph_send_response(PERIODIC, ReqID: %d, value: %d, INT, time: "
			 PRINT_TIME_FORMAT")\n", 
			 ReqID, value->int_, FORMAT_TIME(ttime)); 
	  assert(len == sizeof(int));
	} else if( value_type == GRAPH_STRING_VALUE) {
	  printf("GRAPH: graph_send_response(PERIODIC, ReqID: %d, value: %s, STR, time: "
			 PRINT_TIME_FORMAT")\n", 
			 ReqID, value->string_, FORMAT_TIME(ttime)); 
	} else if( value_type == GRAPH_DOUBLE_VALUE_MEAN_SD) {
	  printf("GRAPH: graph_send_response(PERIODIC, ReqID: %d, value: %s, VMMMS, time: "
			 PRINT_TIME_FORMAT")\n", 
			 ReqID, value->string_, FORMAT_TIME(ttime)); 
	}
#endif
	break;
  case GRAPH_TRIGGER:
	length = RESP_HDR_SZ + TRIGGER_RESP_CONTENTS_HDR_SZ + len;
	pkt->length = htonl(length);
	graph_hton_parameter(&pkt->contents.trigger.value, value, 
						 value_type, len);
	pkt->contents.trigger.value_type = htonl(value_type);
	pkt->contents.trigger.time_high =htonl((int) (aux1.longlong_ >> 32));
	pkt->contents.trigger.time_low = htonl((int) (aux1.longlong_));
	break;
#ifdef DEBUG_GRAPH
	if((value_type == GRAPH_INT_VALUE) || ( value_type == GRAPH_U_INT_VALUE)) {
	  printf("GRAPH: graph_send_response(TRIGGER, ReqID: %d, value: %d, INT, time: "
			 PRINT_TIME_FORMAT")\n", 
			 ReqID, value->int_, FORMAT_TIME(ttime)); 
	  assert(len == sizeof(int));
	}else if( value_type == GRAPH_STRING_VALUE) {
	  printf("GRAPH: graph_send_response(PERIODIC, ReqID: %d, value: %s, STR, time: "
			 PRINT_TIME_FORMAT")\n", 
			 ReqID, value->string_, FORMAT_TIME(ttime)); 
	}else if( value_type == GRAPH_DOUBLE_VALUE_MEAN_SD) {
	  printf("GRAPH: graph_send_response(PERIODIC, ReqID: %d, value: %s, VMMMS, time: "
			 PRINT_TIME_FORMAT")\n", 
			 ReqID, value->string_, FORMAT_TIME(ttime)); 
	}
#endif
  case ERROR_SIGNALLING:
	length = RESP_HDR_SZ + POLL_RESP_CONTENTS_HDR_SZ + len;
	pkt->length = htonl(length);
	graph_hton_parameter(&pkt->contents.poll.value, value, 
						 value_type, len);
	pkt->contents.poll.value_type = htonl(value_type);
	pkt->contents.poll.time_high = htonl((int) (aux1.longlong_ >> 32));
	pkt->contents.poll.time_low = htonl((int) (aux1.longlong_));
	break;
  default:
	err_quit("GRAPH: unknown response type (%d)\n", reqType);
	break;
  }

  /*repeat until completely sent*/
  p = (char *) pkt;

#ifdef DEBUG_GRAPH
  printf("GRAPH: graph_send_response: ");
  for (sentLen = 0; sentLen < length; sentLen++) { printf(" 0x%x", p[sentLen]);}
  printf("\n");
#endif 
  
  while(length) {
	if ((sentLen = send(sock, p, length, 0)) < 0) {
	  err_ret("GRAPH: can't send packet (%d)\n", errno);
	  graph_close_socket(graph);
	  return;
	}
	length -= sentLen;
	p += sentLen;
  }  

}
  
/*******************************************************************************
  graph_hton_parameter(char *value_out, char *value_in,
						   unsigned value_type, unsigned len)

  This function will change the host to network ordering of the given
  value, of type value_type and length len.
*******************************************************************************/
void  graph_hton_parameter(graph_value_t *value_out, graph_value_t *value_in,
						   unsigned value_type, unsigned len)
{
  switch(value_type) {
  case GRAPH_INT_VALUE:
	value_out->int_ = htonl(value_in->int_);
	assert(len == sizeof(int));
	break;
  case GRAPH_U_INT_VALUE:
	value_out->u_int_ = htonl(value_in->u_int_);
	assert(len == sizeof(unsigned));
	break;
  case GRAPH_LONGLONG_VALUE:
	value_out->longlong_ = (long long)
	  (htonl((value_in->longlong_>> 8) & 0xFF)& 0xFF) << 8 ||
	  htonl(value_in->longlong_ & 0xFF)& 0xFF;
	assert(len == sizeof(unsigned));
	break;
  case GRAPH_INT_VALUE_MEAN_SD:
	value_out->intValueMeanSd_.value = htonl(value_in->intValueMeanSd_.value);
	value_out->intValueMeanSd_.mean = htonl(value_in->intValueMeanSd_.mean);
	value_out->intValueMeanSd_.sd = htonl(value_in->intValueMeanSd_.sd);
	assert(len == sizeof(intValueMeanSd_t));
	break;
  case GRAPH_DOUBLE_VALUE:
	value_out->double_ = value_in->double_; /*????is this correct????*/
	/* we might have to convert the double to a string, then send it, and
	   convert it back to double at the other end*/
	assert(len == sizeof(double));
	break;
  case GRAPH_DOUBLE_VALUE_MEAN_SD:
	strncpy(value_out->doubleValueMaxMinMeanSd_.string_, value_in->doubleValueMaxMinMeanSd_.string_, len);
	break;
  case  GRAPH_STRING_VALUE:
	strncpy(value_out->string_, value_in->string_, len);
	break;
  default:
	  err_quit("GRAPH: unknown value_type (%d)\n", value_type);
  break;
  }
  
}

/*******************************************************************************
  graph_close_socket(struct GRAPH *graph)

  This function will close the data socket.
*******************************************************************************/
void graph_close_socket(struct GRAPH *graph)
{
  shutdown(graph->dataSock, SHUT_RDWR);
  graph->state = GRAPH_LISTENING;
  graph->dataSock = -1;
}
  
/*******************************************************************************
  graph_periodicReporting(void *txI, void *rxI, void *arg)

  This event handler will wake upo periodically and it will get the
  results and send them to the graphing tool.
*******************************************************************************/
void graph_periodicReporting(void *txI, void *rxI, void *arg)
{
  struct GRAPH *graph = (struct GRAPH*)rxI;
  struct GraphRequestPacket *pkt = (struct GraphRequestPacket*)arg;
  
  
  graph_process_request(graph, pkt->contents.period.object & 
						(BASE_MASK | VARIABLE_MASK),
						pkt->contents.period.object & PORTNUM_MASK, 
						pkt->ReqID, GRAPH_PERIODIC_REPORT);
  

}

/*******************************************************************************
  graph_process_request(struct GRAPH *graph, unsigned object,
							 unsigned objectNum, unsigned ReqID, 
							 unsigned reqType)

  This function will process a single poll or periodic request and send
  the response back. 
*******************************************************************************/
void   graph_process_request(struct GRAPH *graph, unsigned object,
							 unsigned objectNum, unsigned ReqID, 
							 unsigned reqType)
{
  graph_value_t value;
  unsigned len, value_type;
  int retval = GRAPH_UNKNOWN_OBJECT;
  
  graph->numReqsReceived++;

  switch (object & BASE_MASK) {

  case GRAPH_BASE:
	if (objectNum == 0) {
	  retval = graph_get_object_value(graph, object, objectNum, &value,
									  &value_type, &len); 
	}
	break;
  case LATENCY_BASE:
	if (objectNum == 0) {
	  retval = latency_get_object_value( NULL, object, objectNum, &value,
					&value_type, &len); 
	}
	break;
  case OCCUPANCY_BASE:
	if (objectNum == 0) {
	  retval = occupancy_get_object_value( switches[0], object, objectNum, &value,
					&value_type, &len); 
	}
	break;
  
  
  default:
	err_quit("GRAPH: unknown base type (0x%x)\n", object | BASE_MASK);
	break;
  }

  if (retval) {
	err_ret("GRAPH: unknown object (0x%x)\n", object);
	value.int_ = 0;
	graph_send_response(graph, ReqID, ERROR_SIGNALLING,
					&value, sizeof(unsigned), GRAPH_INT_VALUE, now);
  } else 
	graph_send_response(graph, ReqID, reqType, &value, len, value_type,now);
  

}

/*******************************************************************************
  graph_get_object_value(struct GRAPH *graph, 
                       unsigned object, unsigned objectNum, 
                       unsigned *value, unsigned *length)

  Reads the value of object objectNum, and then returns its value and length.
  If no error occurs, a NULL is returned.
*******************************************************************************/
int graph_get_object_value(struct GRAPH *graph,
						   unsigned object, unsigned objectNum, 
						   graph_value_t *value, unsigned *value_type, 
						   unsigned *length)
{
  int MC, prio, len;

  assert((object & BASE_MASK) == GRAPH_BASE);

#ifdef DEBUG_GRAPH
  printf("GRAPH: graph_get_object_value.\n");
#endif
  
  switch(object & VARIABLE_MASK) {
  case NUM_REQUESTS_CODE:

	// this is fake, we should improve it later.
	sprintf(value->doubleValueMaxMinMeanSd_.string_, 
			"%g\n%g\n%g\n%g\n%g\n",  (double)graph->numReqsReceived, 
			(double)graph->numReqsReceived + 5, 
			(double)graph->numReqsReceived/ 10.0, 
			((double)graph->numReqsReceived)/2.0, 
			((double)graph->numReqsReceived)/4.0);
	*value_type = GRAPH_DOUBLE_VALUE_MEAN_SD;
	*length = strlen(value->doubleValueMaxMinMeanSd_.string_);  
  

	return 0;
	break;
  case VARIABLES_CODE:
    prio = object & PRIORITY_SUBMASK;
    MC = object & MULTICAST_SUBMASK;
    if ((prio > 0) || (MC > 0)) {
      err_ret("GRAPH: this object (0x%x) can't be queried\n", object);
      return GRAPH_UNKNOWN_OBJECT;
      break;
    }
    len = GRAPH_MAX_PKT_LEN;
    value->string_[0]='\0';
	graph_get_object_name(&graphTool, value->string_, &len);
  latency_get_object_name(NULL, value->string_, &len);
  occupancy_get_object_name( switches[0], value->string_, &len);
#ifdef DEBUG_GRAPH
	printf("graph_get_object_value: len = %d => \"%s\"\n", GRAPH_MAX_PKT_LEN -
	       len, value->string_);
#endif

    *value_type = GRAPH_STRING_VALUE;
    *length = GRAPH_MAX_PKT_LEN - len;
    return 0;
    break;
    /*
      case 0:
      switch(object & PRIORITY_MASK) {
      case 0:
      switch(object & MULTICAST_MASK) {
      case 0:
	
      default:
      err_ret("TT: this object (0x%x) can't be queried\n", object);
      return GRAPH_UNKNOWN_OBJECT;
      break;
      }

      default:
      err_ret("TT: this object (0x%x) can't be queried\n", object);
      return GRAPH_UNKNOWN_OBJECT;
      break;
      }
    */

	
  default:
	err_ret("GRAPH: this object (0x%x) can't be queried\n", object);
	return GRAPH_UNKNOWN_OBJECT;
	break;
  }
  return(0);
}

/*******************************************************************************
  graph_get_object_name(struct GRAPH *graph, 
						   char *nameStr, int *length)

  Writes the variables that this modules supports into nameStr if there is
  enough space left.
*******************************************************************************/
void graph_get_object_name(struct GRAPH *graph,
						   char *nameStr, int *length)
{
  char str[MAX_STRING_LEN];
  int strLen;
  
  sprintf(str, "%d, %d, GRAPH: NUM_REQUESTS\n", 
		  GRAPH_BASE, NUM_REQUESTS_CODE);
  strLen = strlen(str);
  if (*length - strLen > 0) {
	strcat(nameStr, str);
	*length -= strLen;
  }
  

#ifdef DEBUG_GRAPH
  printf("GRAPH: graph_get_object_name.\n");
#endif


}

