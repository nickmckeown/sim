
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
#include "sim.h"
#include "traffic.h"


int
null(action, aSwitch, input, argc, argv)
  TrafficAction action;
Switch *aSwitch;
int input;
int argc;
char **argv;
{


  switch( action )
    {	
    case TRAFFIC_USAGE:
      fprintf(stderr, "No options for \"null\" traffic model.\n");
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


	while( (c = getopt(argc, argv, "")) != EOF) 
	  switch(c)
            {
	    case '?':
	      fprintf(stderr, "-------------------------------\n");
	      fprintf(stderr, "null: Unrecognized option -%c\n", optopt);
	      null(TRAFFIC_USAGE);
	      fprintf(stderr, "--------------------------------\n");
	      exit(1);
	    default:
	      break;
	    }

	break;
      }
    case TRAFFIC_GENERATE:
      {

	    if( debug_traffic) printf("		GEN_TRAFFIC: ");
	    break;

	  }
    case REPORT_TRAFFIC_STATS:
        printf("  %d  null\n", input);
        break;
    default:
        break;
    }

  return(CONTINUE_SIMULATION);
}
