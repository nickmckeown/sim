
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
This file contains general very basic definitions for the stub for the
graphing tool and the simulator 
*******************************************************************************/
#ifndef AUX_H
#define AUX_H

#define MAX_STRING_LEN 100


#define CELL_TIME 1
#define FORMAT_TIME( a ) a
#define PRINT_TIME_FORMAT "%ld"


struct TT
{
  int numPort;             /* Number of ports */
  int numPriorities;       /* Number of priorities */
  struct GRAPH *graph;  
};


/*******************************************************************************
  err_quit(va_alist)

  This function will display the error string passed as an argument, and
  then quit the program.

  Used to indicate fatal errors.

  (argument is like that in printf)
*******************************************************************************/
void err_quit();

/*******************************************************************************
  err_ret(va_alist)

  This function will print the error given as an argument, and then return.

  Used to indicate non-fatal errors.

  (argument is like that in printf)
*******************************************************************************/
void err_ret();

#endif //AUX_H
