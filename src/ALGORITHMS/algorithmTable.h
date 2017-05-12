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


/* Switch Algorithm table */

extern void     nullSchedulingAlgorithm();
extern void     fifo();
extern void     future();
extern void     gs_lqf();
extern void     gs_ocf();
extern void     ilpf();
extern void     ilqf();
extern void     iocf();
extern void     iopf();
extern void     lpf();
extern void     lpf_delay();
extern void     lqf();
extern void     ocf();
extern void     pim();
extern void     islip();
extern void     maximum();
extern void     maxrand();
extern void     maxsize();
extern void     mcast_conc_residue();
extern void     mcast_slip();
extern void     mcast_dist_residue();
extern void     mcast_random();
extern void     mcast_tatra();
extern void     mcast_wt_fanout();
extern void     mcast_wt_residue();
extern void     mucf();
extern void     neural();
extern void     opf();
extern void     opf_delay();
extern void     pri_fifo();
extern void     pri_lqf();
extern void     pri_mcast_random();
extern void     pri_ocf();
extern void     pri_islip();
extern void     pri_combo();
extern void     pristrict_lqf();
extern void     pristrict_ocf();
extern void     rr();
extern void     wfa();
extern void     wwfa();

FunctionTable algorithmTable[] = {
	{"null",    "Null algorithm. Does nothing",   (void *) nullSchedulingAlgorithm},
    {"fifo",    "Random FIFO. Single FIFO per input",    (void *) fifo},
    {"future", "Extension to Karol/Eng/Obara Recycle Algorithm, also includes multiple iterations", (void *) future},
    {"gs_lqf", "Gale-Shapley Algorithm. Weight = occupancy", (void *) gs_lqf},
    {"gs_ocf", "Gale-Shapley Algorithm. Weight = waiting time", (void *) gs_ocf},
    {"ilpf", "Using iLLF Array", (void *) ilpf},
    {"ilqf", "Iterative lqf", (void *) ilqf},
    {"iocf", "Iterative ocf", (void *) iocf},
    {"iopf", "Using iLLF Array", (void *) iopf},
    {"lpf",     "Maximum, using Matthew J. Saltzman's code. Weight = port occupancy", (void *) lpf},
    {"lpf_delay", "Maximum, using Matthew J. Saltzman's code. Weight = port occupancy", (void *) lpf_delay},
    {"lqf", "Maximum, using Matthew J. Saltzman's code. Weight = occupancy", (void *) lqf},
    {"maximum", "Tom Anderson's depth-first-search algorithm", (void *) maximum},
    {"maxrand", "Maximum, with randomization of choice", (void *) maxrand},
    {"maxsize", "Maximum, using Matthew J. Saltzman's code", (void *) maxsize},
    {"mcast_conc_residue", "Multicast:concentrate residue", (void *) mcast_conc_residue},
    {"mcast_dist_residue", "Multicast:distribute residue", (void *) mcast_dist_residue},
    {"mcast_random", "Multicast: random assignment", (void *) mcast_random},
    {"mcast_slip", "Multicast: single pointer", (void *) mcast_slip},
    {"mcast_tatra", "Multicast: Tatra, Balaji, McKeown and Ahuja", (void *) mcast_tatra},
    {"mcast_wt_fanout", "Multicast: Weights for fanout", (void *) mcast_wt_fanout},
    {"mcast_wt_residue", "Multicast: Weights for residue", (void *) mcast_wt_residue},
    {"mucf", "Iterative most urgent cell first (smallest cushion)", (void *) mucf},
    {"neural",  "Mustafa Mehmet Ali's neural net: Globecom 89", (void *) neural},
    {"ocf", "Maximum, using Matthew J. Saltzman's code. Weight = celltime", (void *) ocf},
    {"opf",     "Maximum, using Matthew J. Saltzman's code. Weight = cell time", (void *) opf},
    {"opf_delay", "Maximum, using Matthew J. Saltzman's code. Weight = celltime", (void *) opf_delay},
    {"pim",     "DEC SRC Parallel Iterative Matching", (void *) pim},
    {"pri_fifo", "Fifo with \"strict\" priorities", (void *) pri_fifo},
    {"pri_lqf", "Maximum with priorities, using Matthew J. Saltzman's code. Weight = occupancy", (void *) pri_lqf},
    {"pri_mcast_random", "Multicast: random assignment with strict priorities", (void *) pri_mcast_random},
    {"pri_ocf", "Maximum with priorities, using Matthew J. Saltzman's code. Weight = celltime", (void *) pri_ocf},
    {"pri_islip", "islip with \"strict\" priorities", (void *) pri_islip},
    {"pri_combo", "pri_fifo with pri_mcast_random", (void *) pri_combo},
    {"pristrict_lqf", "Maximum with strict priorities, using Matthew J. Saltzman's code. Weight = occupancy", (void *) pristrict_lqf},
    {"pristrict_ocf", "Maximum, with strict priorities, using Matthew J. Saltzman's code. Weight = celltime", (void *) pristrict_ocf},
    {"rr",      "A Basic round robin algorithm",    (void *) rr},
  	{"islip", "Slip to avoid starvation", (void *) islip},
    {"wwfa", "Wrapped Wavefront Arbitrator", (void *) wwfa},
    {"wfa", "Using WFA Array", (void *) wfa},
	{NULL,NULL,NULL}
};

