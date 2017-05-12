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
**      Assignment problem solver (2-dimension linear sum)
**
**  ABSTRACT:
**
**	An implementation of the Shortest Augmenting Path algorithm for
**	solving linear-sum assignment problems (based on the FORTRAN code
**	described by Burkard and Derigs [1], and modified for high-speed
**	update of labels and duals).  A dense list of costs in row-major 
**	order is input and a permutation vector is returned containing an 
**	optimal assignment.  The value of the function is the value of 
**	the optimal assignment.  The dual multipliers are returned in
**	two separate arrays.
**
**      For cost data of type other than int, change COST to the 
**	appropriate type and redefine MAXCOST in assign2.h (can be done 
**	with a define on the compiler command line).  (Note that 
**	the code has not been tested with floating-point costs.)
**	Unsigned types probably won't work, since the duals may take
**	values with either sign.
**
**	Note:  This code does not check for overflow.  If "nonexistent"
**	arcs are given large costs, these must be kept small enough
**	so that a "feasible" solution involving them will not cause
**	numerical difficulty (e.g. MAXCOST/n).
**
**	Note on passing arrays:  This code behaves as if it could 
**	dynamically dimension the array parameters, ala FORTRAN.  In 
**	particular, the ccost[] array could be either an oversized 
**	two-dimensional array, an exact-sized two-dimensional array, 
**	or an exact-sized one-dimensional array.  The parameter nn gives 
**	the actual order of the problem, and the parameter nmax gives 
**	the length of a physical "row" in the two-dimensional case.  
**	For a one-dimensional ccost array, pass the same value for 
**	both nn and nmax.  Pedants, note that to be technically correct C,
**	a two-dimensional array should be passed by passing the address
**	of the first integer, e.g., &c[0][0] rather than simply c.
**
**      Note to FORTRAN programmers:  C assumes arrays are indexed from 
**	zero, so the permutation returned is of indices 0,...,n-1.
**	C also assumes multidimensional arrays are stored in *row-major* 
**	order.  If you pass a two-dimensional ccost array, assign2() will 
**	return a permutation of row indices rather than column indices.  
**	If you pass a one-dimensional cost array with costs in row-major 
**	order, assign2() will behave normally.  All parameters are 
**	call-by-reference to facilitate direct calls from FORTRAN.
**
**	[1] Burkard, R. N. and N. Derigs, _Assignment_and_Matching_
**	    Problems:_Solution_Methods_with_FORTRAN_Programs_,
**	    Springer Verlag, 1980.
**
**  AUTHOR:
**
**	Copyright (c) 1992, 1993 by 
**
**			Matthew J. Saltzman
**			Mathematical Sciences Dept.
**			Box 341907
**			Clemson University
**			Clemson SC  29634-1907
**
**			mjs@clemson.edu
**
**      Permission is granted to copy or distribute this file or
**      object code generated from it in its original form
**      (including this notice).  Any modifications included 
**      must be noted in the MODIFICATION HISTORY section below,
**      including the date and author's name.  Report problems or
**      suggestions to the author at the above address.
**
**  CREATION DATE:	23-Dec-1991
**
**  MODIFICATION HISTORY:
**
**	26-Feb-1993	(mjs) Changed linked list structure for labeled
**			and unlabeled columns to partitioned array structure.
**			Added check for unsuccessful memory allocation.
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

#define MAXCOST INT_MAX

/*
**
**  MACRO DEFINITIONS
**
**/

#define RCOST(i, j) (cost[i][j] - iweight[i] - jweight[j])
/* reduced cost of assignment */
#define UNMARKED    (-1)

/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**      Solve 2-dimensional linear sum assignment problem using
**	shortest augmenting path algorithm.
**
**  FORMAL PARAMETERS:
**
**      int   ccost[nn][nmax]	Dense row-major list of cost coefficients
**	int    imark[nn]	Permutation vector for assignment
**	int   iweight[nn]	Dual vars for rows
**	int   jweight[nn]	Dual vars for cols
**	int    *nn		Actual order of problem
**      int    *nmax            Physical size of a row
**
**  IMPLICIT INPUTS:
**
**	none
**
**  IMPLICIT OUTPUTS:
**
**	none
**
**  FUNCTION VALUE:
**
**      cost of optimal assignment
**
**  SIDE EFFECTS:
**
**      none
**
**--
**/

int    assign2(ccost, imark, iweight, jweight, nn, nmax)
  int    *ccost;	/* in:  1-dim array of assignment costs */
int     *imark;	/* out: 1-dim permutation array contains */
/* optimal assignment on return */
int    *iweight;	/* out: row and col dual vars */	
int    *jweight;	
int     *nn;		/* in:  actual size of problem is nn X nn */
int     *nmax;		/* in:  physical size of array is nn X nmax */
{
  int     n = *nn;		/* local copy */
  int    z;			/* opt value */
  int    **cost = (int **) malloc(n * sizeof(int *));
  int     *jmark = (int *) malloc(n * sizeof(int));

  if ( cost == NULL || jmark == NULL ) {
    fputs("assign2: out of memory\n", stderr);
    exit(1);
  }

  /* 
   * cost[] gives indirect 2-dim access to elements of ccost;
   * initialize matching to empty
   */
  {
    int     i;

    for ( i = 0;  i < n;  i++ ) {
      cost[i] = ccost + i * (*nmax);
      imark[i] = jmark[i] = UNMARKED;
    }
  }

  /*
   * find initial row & col weights, initial matching
   */
  {
    int     i, j, ij;

    for ( i = 0;  i < n;  i++ ) {
      for ( iweight[i] = cost[i][0], ij = 0, j = 1;  j < n;  j++ ) 
	if ( cost[i][j] < iweight[i] ) {
	  iweight[i] = cost[i][j];
	  ij = j;
	}

      for ( j = ij;  j < n && (jmark[j] != UNMARKED ||
			       cost[i][j] != iweight[i]);  j++ );
      if ( j < n ) {		/* it's a match */
	jmark[j] = i;
	imark[i] = j;
      }
    }

    for ( j = 0;  j < n;  j++ ) {
      if ( jmark[j] != UNMARKED )	/* col already matched */
	jweight[j] = 0;
      else {
	for ( jweight[j] = cost[0][j] - iweight[0], ij = 0, i = 1;  
	      i < n;  i++ )
	  if ( cost[i][j] - iweight[i] < jweight[j] ) {
	    jweight[j] = cost[i][j] - iweight[i];
	    ij = i;
	  }
	      
	for ( i = ij;  i < n && (imark[i] != UNMARKED ||
				 cost[i][j] - iweight[i] != jweight[j]);  i++ );
	if ( i < n ) {		/* it's a match */
	  jmark[j] = i;
	  imark[i] = j;
	}
      }
    }
  }

  /*
   * augment matching until optimal
   */
  {
    int     r;			/* row to augment from */
    int     *jlist = (int *) malloc(n * sizeof(int));
    /* partitioned lists of labels */
    int     *jnext = (int *) malloc(n * sizeof(int));
    /* row that contributes current col label */
    int    *jlabel = (int *) malloc(n * sizeof(int));    /* label values */ 
    /* row labels are given by ilabel[r] == 0, */
    /* ilabel[i] == jlabel[imark[i]] */

    if ( jlist == NULL || jnext == NULL || jlabel == NULL ) {
      fputs("assign2: out of memory\n", stderr);
      exit(1);
    }

    for ( r = 0;  r < n;  r++ ) {

      if ( imark[r] == UNMARKED ) {	/* unmarked row found: grow a tree */

	int     j, k;		/* unlabeled column, list index */
	int     jmin=0, kmin=0;	/* memory for column, index */
	int    delta, d;	/* total, current path length */
	int     jlhdr = n;	/* split jlist into labeled, unlabeled */
	/* parts; col jlist[k] is unlabeled */
	/* if 0 <= k < jlhdr, labeled o/w */

	/*
	 * start with current unmarked row labeled, all cols unlabeled
	 */
	{
	  for ( d = MAXCOST, k = 0;  k < n;  k++ ) {
	    jlist[k] = k;
	    jlabel[k] = RCOST(r, k);
	    jnext[k] = r;

	    if ( jlabel[k] < d )
	      d = jlabel[jmin = kmin = k];
	  }
	}

	/*
	 * find an augmenting path
	 */
	{
	  while ( TRUE ) {

	    int     i;		/* row matched to last labeled column */

	    /*
	     * move min-label col from unlabeled list to labeled list
	     */
	    {
	      int     jtmp = jlist[kmin];	/* temp for list swapping */

	      jlhdr--;			/* make room in labeled list */
	      jlist[kmin] = jlist[jlhdr];	
	      /* delete from non-labeled list */
	      jlist[jlhdr] = jtmp;	/* insert at front of labeled list */
	      delta = d;		/* accumulated length */
	    }

	    if ( (i = jmark[jmin]) == UNMARKED )
	      break;			/* BREAK: augmenting path found */

	    /*
	     * update temp labels of unlabeled cols; 
	     * find min-label col, then find predecessor
	     */
	    {
	      int    rcost;		/* reduced cost */

	      for ( d = MAXCOST, k = 0;  k < jlhdr;  k++ ) {
		j = jlist[k];
		if ( (rcost = delta + RCOST(i, j)) < jlabel[j] ) {
		  jlabel[j] = rcost;
		  jnext[j] = i;
		}
		if ( jlabel[j] < d ) {	/* remember min label & location */
		  kmin = k;
		  d = jlabel[jmin = j];
		}
	      }
	    }
	  }
	}

	/*
	 * update duals (rows to be updated are still matched to labeled cols)
	 */
	{
	  if ( delta > 0 ) {		/* no perm label larger than delta */
	    for ( k = jlhdr;  k < n;  k++ ) {
	      j = jlist[k];
	      jweight[j] += jlabel[j] - delta;
	      if( jmark[j] >= 0 )
	     	iweight[jmark[j]] += delta - jlabel[j];
	    }
	    iweight[r] += delta;
	  }
	}

	/* 
	 * exchange marked & unmarked cells along aug. path;
	 * current jmin is unmarked col
	 */
	{
	  int     jjk, ik;

	  do {
	    ik = jmark[jmin] = jnext[jmin];
	    jjk = imark[ik];
	    imark[ik] = jmin;
	    jmin = jjk;
	  } while ( ik != r );
	}
      }
    }

    free(jlabel);
    free(jnext);
    free(jlist);
  }

  /*
   * calculate cost of optimal assignment
   */
  {
    int     i;

    for ( z = 0, i = 0;  i < n;  i++ )
      z += cost[i][imark[i]];
  }

  free(jmark);
  free(cost);

  return z;
}


void    identify()
{
  printf("assign2sap.c (Copyright 1992, 1993 by Matthew J. Saltzman)\n\n");
}
