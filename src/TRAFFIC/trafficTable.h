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

/* Traffic Model Table */
extern int     null();
extern int     bernoulli_iid_uniform();
extern int     bernoulli_iid_nonuniform();
extern int     bursty();
extern int     bursty_nonuniform();
extern int     keepfull();
extern int     trace();
extern int     tracePacket();
extern int     periodicTrace();

char null_desc[] =  "Null: does nothing. Generates no cells. Zippo.";
char bernoulli_uniform_desc[] =  "Bernoulli arrivals, iid, destinations uniformly distributed over all outputs, UCAST or MCAST";
char bernoulli_nonuniform_desc[] =  "Bernoulli arrivals, iid, destinations defined by array of utilizations";
char bursty_desc[] = "Bursts of cells in busy-idle periods, destinations uniformly distributed cell by cell or burst-by-burst over all outputs";
char bursty_nonuniform_desc[] = "Bursts of cells in busy-idle periods, destinations defined by array of utilizations";
char keepfull_desc[] =  "Always keep input-output buffer full according to nonuniform traffic pattern";
char trace_desc[] =  "Trace driven from a file of <time,vci> cells";
char tracePacket_desc[] =  "Trace driven from a file of <time,destination,length> packets";
char periodicTrace_desc[] =  "Trace driven from a file: one file for whole switch, connect to any one input";


FunctionTable trafficTable[] = {
    {"bernoulli_iid_uniform", bernoulli_uniform_desc, (void *) bernoulli_iid_uniform},
    {"bernoulli_iid_nonuniform", bernoulli_nonuniform_desc, (void *) bernoulli_iid_nonuniform},
    {"bursty", bursty_desc, (void *) bursty},
    {"bursty_nonuniform", bursty_nonuniform_desc, (void *) bursty_nonuniform},
    {"keepfull", keepfull_desc, (void *) keepfull},
    {"trace", trace_desc, (void *) trace},
    {"tracePacket", tracePacket_desc, (void *) tracePacket},
    {"null", null_desc, (void *) null},
    {"periodicTrace", periodicTrace_desc, (void *) periodicTrace},
	{NULL,NULL,NULL}
};

