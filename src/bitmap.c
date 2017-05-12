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
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "bitmap.h"

static int countBits[256];

void bitmapReset(Bitmap *bitmap)
{
  memset(bitmap, 0, sizeof(Bitmap));
}

void bitmapSetBit(int bit, Bitmap *bitmap)
{
  int byteNum;
  int bitNum;
  
  byteNum = bit/8;
  bitNum = bit%8;
  (bitmap->byte[byteNum]) |= ( 0x1<<bitNum );
}

void bitmapResetBit(int bit, Bitmap *bitmap)
{
  int byteNum;
  int bitNum;
  
  byteNum = bit/8;
  bitNum = bit%8;

  (bitmap->byte[byteNum]) &= (0xFF ^ ( 0x1<<bitNum ) );
}


int bitmapIsBitSet(int bit, Bitmap *bitmap)
{
  int byteNum;
  int bitNum;
  
  byteNum = bit/8;
  bitNum = bit%8;
  if( (bitmap->byte[byteNum]) & ( 0x1<<bitNum ) )
    return 1;
  else
    return 0;
}

int bitmapAnyBitSet(Bitmap *bitmap)
{
  int i;
  for(i = 0; i < BITMAP_BYTES; i++)
    {
      if(bitmap->byte[i])
	return 1;
    }
  return 0;
}

void bitmapSetupNumSet()
{
  int i=0,j=0;
  char c=0;
  Bitmap bitmap;

  for(i=0; i < 256; i++)
    {
	bitmap.byte[0] = c++;
	countBits[i] = 0;
	for (j=0; j<8; j++)
	  if(bitmapIsBitSet(j,&bitmap))
		 countBits[i]++;
    }
}
int bitmapNumSet(Bitmap *bitmap)
{
  static int firstTime = 1;
  int i,sum=0;
  /*  int sum2=0; */
  if (firstTime == 1) {
	firstTime = 0;
	bitmapSetupNumSet();
  }

  for(i=0; i < BITMAP_BYTES; i++)
    {
	sum+= (unsigned int)countBits[bitmap->byte[i]];
    }

  /*  for(i=0; i < 8 * BITMAP_BYTES; i++)
    {
      if(bitmapIsBitSet(i, bitmap))
	sum2++;
    }
	assert(sum == sum2); */
  return(sum);
}


void bitmapPrint(FILE *fp, Bitmap *bitmap, int length)
{
  int bit;
  for(bit=0;bit<length;bit++)
    {
      if( bitmapIsBitSet(bit, bitmap) ) 
	fprintf(fp,"1");
      else 
	fprintf(fp,"0");
    }
  fprintf(fp,"\n");
}

/* Reads bitmap as sequence of '1's and '0's from file until
 * terminated by <SPACE> or \n. Returns 1 if OK, else 0
 */
int bitmapRead(FILE *fp, Bitmap *bitmap)
{
    int i;
    char c;
    int done=0;

    bitmapReset(bitmap);
    
    while( (c=fgetc(fp)) == ' ');
    ungetc(c,fp);

    for(i=0; !done ; i++)
    {
        c = fgetc(fp);
        switch(c)
        {
            case '1':
                bitmapSetBit(i,bitmap);
                break;
            case '0':
                break;
            case ' ':
                done=1;
                break;
            case '\n':
                done=1;
                break;
            default:
                return(0);
        }
    }
    return(1);
}


void bitmapSetRandom(Bitmap *bitmap, unsigned short *seed, int numOutputs)
{
  int numBits;
  int numBytes;
  int i;
  
  numBytes = numOutputs/8;
  numBits = numOutputs%8;
  
  bitmapReset(bitmap);
  for(i = 0; i < numBytes; i++)
    {
      bitmap->byte[i] = (unsigned char)(nrand48(seed)&((1<<8)-1));
    }
  bitmap->byte[numBytes] = (unsigned char)(nrand48(seed)&((1<<numBits)-1));
}
