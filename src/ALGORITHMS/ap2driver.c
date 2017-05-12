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

/*
**++
**  FACILITY:
**
**      Test driver for Hungarian algorithm for the assignment problem
**
**  ABSTRACT:
**
**      [@tbs@]
**
**  AUTHORS:
**
**      Matthew J. Saltzman
**
**
**  CREATION DATE:     20-Oct-1991
**
**  MODIFICATION HISTORY:
**--
**/


/*
**
**  INCLUDE FILES
**
**/

#include <stdio.h> 
#include <stdlib.h>
#include "assign2.h"
int **
ap2driver(n, graph)
  int n;
int **graph;
{

  static int   *ccost, **cost;
  static int   *idual, *jdual;
  static int    *assn;
  int    z, i,j;

  int    printflag = 0;

  if( !ccost )
    {
      ccost = (int *) malloc(n * n * sizeof(int));
      cost = (int **) malloc(n * sizeof(int *));
      idual = (int *) malloc(n * sizeof(int));
      jdual = (int *) malloc(n * sizeof(int));
      assn = (int *) malloc(n * sizeof(int));
    }

  for ( cost[0] = ccost, i = 1;  i < n;  i++ )
    cost[i] = cost[i - 1] + n;

  for ( i = 0;  i < n;  i++ )
    for ( j = 0;  j < n;  j++ )
      {
	/* Modify graph 1<->0 */
	/* cost[i][j] = graph[i][j]==0 ? 1:0;*/
	cost[i][j] = -1*graph[i][j];
      }

  z = assign2(ccost, assn, idual, jdual, &n, &n);

  if ( printflag ) {
    printf("assignment\n");
    for (i = 0;  i < n;  i++)
      printf("%d ", assn[i] + 1);
    printf("\nidual\n");
    for (i = 0;  i < n;  i++)
      printf("%d ", idual[i]);
    printf("\njdual\n");
    for (i = 0;  i < n;  i++)
      printf("%d ", jdual[i]);
    printf("\n");
  }

  return(assign2graph(n, graph, assn));

}

int **
assign2graph(n, graph, assn)
  int n;
int **graph;
int *assn;
{
  static int **match=NULL;
  int i,j;

  if(!match)
    {
      match = (int **) malloc( n * sizeof(int *) );
      for(i=0; i<n; i++)
	match[i] = (int *) malloc( n * sizeof(int) );
    }

  /* Fill in match array from assn  */
  for(i=0; i<n; i++)
    for(j=0; j<n; j++)
      {
	if( (assn[i]==j) && (graph[i][j]) )
	  match[i][j] = 1;
	else
	  match[i][j] = 0;
      }
	
  return(match);
}
