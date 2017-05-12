
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
struct Person {
  int index;
  struct Person **prefList;
  int prefNext;
  struct Person *fiance;
};

int compare();
static int prefers();
static void makeManPrefList();
static void makeWomanPrefList();

int **graph=NULL;  /* Arranged: int[woman][man] */

int **
gsaMatch(iterations, numMen, numWomen, aGraph)
  int iterations;
int numMen,numWomen;
int **aGraph; /* incoming request graph */
{
  static int **matchGraph=NULL;
  static struct Person *Men=NULL, *Women=NULL;
  struct Person *aMan, *aWoman;
  int man, woman;

  /************* INITIALIZE ****************/
  graph = aGraph;
  if(!matchGraph)
    {
      matchGraph = (int **) malloc(sizeof(int *)*numWomen);
      for(woman=0; woman<numWomen;woman++)
	{
	  matchGraph[woman] = (int *) malloc(sizeof(int)*numMen);
	}
    }
  if(!Men)
    {
      Men   = (struct Person *) malloc(sizeof(struct Person) * numMen);
      for( man=0; man<numMen; man++)
	{
	  aMan = &Men[man];
	  aMan->index = man;
	  aMan->prefList = (struct Person **) 
	    malloc(sizeof(struct Person *) * numWomen);
	}
    }

  if(!Women)
    {
      Women = (struct Person *) malloc(sizeof(struct Person) * numWomen);
      for( woman=0; woman<numWomen; woman++)
	{
	  aWoman = &Women[woman];
	  aWoman->index = woman;
	  aWoman->prefList = (struct Person **) 
	    malloc(sizeof(struct Person * ) * numMen);

	}
    }

  for( man=0; man<numMen; man++)
    {
      aMan = &Men[man];
      /* Place women on man's preference list */
      makeManPrefList(aMan, Women, numWomen);
      aMan->fiance = NULL;
      aMan->prefNext = 0;
    }
  for( woman=0; woman<numWomen; woman++)
    {
      aWoman = &Women[woman];
      /* Place men on woman's preference list */
      makeWomanPrefList(aWoman, Men, numMen);
      aWoman->fiance = NULL;
      aWoman->prefNext = 0;
    }
  /************* END INITIALIZE ****************/

  for(;iterations; iterations--)
    {
      /* Every free man proposes to next woman on preference list */
      for( man=0; man<numMen; man++)
	{
	  aMan = &Men[man];
	
	  if( !aMan->fiance )
	    {
	      aWoman = aMan->prefList[aMan->prefNext++];
	      /* Ignore unacceptable partners */
	      if( graph[aWoman->index][man] == 0 )
		continue;
	      if( !aWoman->fiance )
		{
		  aWoman->fiance = aMan;
		  aMan->fiance = aWoman;
		}
	      else if( prefers(aWoman, aMan) )
		{
		  aWoman->fiance->fiance = NULL;
		  aWoman->fiance = aMan;
		  aMan->fiance = aWoman;
		}
	      else
		{
		}
	    }
	}
    }
  for(man=0; man<numMen; man++)
    for(woman=0; woman<numWomen; woman++)
      matchGraph[woman][man] = 0;
  for(man=0; man<numMen; man++)
    {
      if(Men[man].fiance)
	matchGraph[Men[man].fiance->index][man] = 1;
    }
  return(matchGraph);
}

/***********************************************************************/

static struct Person *currentMan; /* Set aMan for compare operation */

int
compareWomen(woman1, woman2)
  struct Person **woman1, **woman2;
{

  if( graph[(*woman1)->index][currentMan->index] > 
      graph[(*woman2)->index][currentMan->index] )
    return(-1);
  if( graph[(*woman1)->index][currentMan->index] < 
      graph[(*woman2)->index][currentMan->index] )
    return(1);
  return(0);
}

static void makeManPrefList(aMan, Women, numWomen)
  struct Person *aMan;
struct Person *Women;
int numWomen;
{
  int woman;

  for(woman=0; woman<numWomen; woman++)
    {
      aMan->prefList[woman] = &Women[woman];
    }
  currentMan = aMan; /* Set aMan for compare operation */
  qsort(aMan->prefList, numWomen, sizeof(struct Person *), compareWomen);

}


static struct Person *currentWoman; /* Set Woman for compare operation */

int
compareMen(man1, man2)
  struct Person **man1, **man2;
{

  if( graph[currentWoman->index][(*man1)->index] > 
      graph[currentWoman->index][(*man2)->index] )
    return(-1);
  if( graph[currentWoman->index][(*man1)->index] < 
      graph[currentWoman->index][(*man2)->index] )
    return(1);
  return(0);
}

static void makeWomanPrefList(aWoman, Men, numMen)
  struct Person *aWoman;
struct Person *Men;
int numMen;
{
  int man;

  for(man=0; man<numMen; man++)
    {
      aWoman->prefList[man] = &Men[man];
    }
  currentWoman = aWoman; /* Set woman for compare operation */
  qsort(aWoman->prefList, numMen, sizeof(struct Person *), compareMen);

}


static int
prefers(aWoman, aMan) 
  struct Person *aWoman, *aMan;
{
  int fianceValue, proposerValue;

  fianceValue    = graph[aWoman->index][aWoman->fiance->index];
  proposerValue   = graph[aWoman->index][aMan->index];
  if( proposerValue > fianceValue )
    return(1);
  else
    return(0);
}

/********** TEMP ************/

#ifdef TEST
static int **makeGraph();
main(argc, argv)
  int argc;
char **argv;
{
  int man, woman, num, times;
  long lrand48();

  num = atoi(argv[1]);
  times = atoi(argv[2]);
  graph = makeGraph(num);
  for(man=0;man<num;man++)
    for(woman=0;woman<num;woman++)
      /* graph[woman][man] = (man+1)*(woman+1); */
      graph[woman][man] = lrand48()%10; 

  printGraph(num);
  for(;times;times--)
    gsa(num, num, num);
  printGraph(num);
	
}
static int **
makeGraph(n)
  int n;
{
  int input;
  int **graph;

  graph = (int **) malloc(sizeof(int *) * n);
  for(input=0;input<n; input++)
    graph[input] = (int *) malloc(sizeof(int)*n);

  return(graph);
}

printGraph(n)
  int n;
{
  int i,j;

  for(i=0;i<n;i++)
    {
      for(j=0;j<n;j++)
	printf("%1d  ", graph[i][j]);
      printf("\n");
    }

}
#endif
