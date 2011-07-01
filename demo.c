/****************************************************************************
 File        : demo.c

 Description : Example program for the p9813 library.  Displays a soothing
               and continuously changing pattern of colors across all pixels.

               Example calling sequence:

               demo -s 4 -p 25

               The two parameters set the number of LED strands and the
               number of pixels per strand, respectively; the above example
               would be for 4 strands of 25 pixels each, or 100 pixels
               total.  Default state is for one strand of 25 pixels.
               If strands are different lengths, specify longest strand.
               Parameters may be issued in any order, and may be ommitted
               to use corresponding defaults.

 License     : Copyright 2011 Phillip Burgess.    www.PaintYourDragon.com

               This Program is free software: you can redistribute it and/or
               modify it under the terms of the GNU General Public License as
               published by the Free Software Foundation, either version 3 of
               the License, or (at your option) any later version.

               This Program is distributed in the hope that it will be
               useful, but WITHOUT ANY WARRANTY; without even the implied
               warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
               PURPOSE.  See the GNU General Public License for more details.

               You should have received a copy of the GNU General Public
               License along with this Program.  If not, see
               <http://www.gnu.org/licenses/>.

               Additional permission under GNU GPL version 3 section 7

               If you modify this Program, or any covered work, by linking
               or combining it with libftd2xx (or a modified version of that
               library), containing parts covered by the license terms of
               Future Technology Devices International Limited, the licensors
               of this Program grant you additional permission to convey the
               resulting work.
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include "p9813.h"

int main(int argc,char *argv[])
{
	double        x,s1,s2,s3;
	int           i,totalPixels,
	  nStrands           = 1,
	  pixelsPerStrand    = 25;
	unsigned char r,g,b;
	time_t        t,prev = 0;
	TCstats       stats;
	TCpixel       *pixelBuf;

	while((i = getopt(argc,argv,"s:p:")) != -1)
	{
		switch(i)
		{
		   case 's':
			nStrands        = strtol(optarg,NULL,0);
			break;
		   case 'p':
			pixelsPerStrand = strtol(optarg,NULL,0);
			break;
		   case '?':
		   default:
			(void)printf(
			  "usage: %s [-s strands] [-p pixels]\n",argv[0]);
			return 1;
		}
	}

	/* Allocate pixel array.  One TCpixel per pixel per strand. */
	totalPixels = nStrands * pixelsPerStrand;
	i           = totalPixels * sizeof(TCpixel);
	if(NULL == (pixelBuf = (TCpixel *)malloc(i)))
	{
		printf("Could not allocate space for %d pixels (%d bytes).\n",
		  totalPixels,i);
		return 1;
	}

	/* Initialize library, open FTDI device.  Baud rate errors
	   are non-fatal; program displays a warning but continues. */
	if((i = TCopen(nStrands,pixelsPerStrand)) != TC_OK)
	{
		TCprintError(i);
		if(i < TC_ERR_DIVISOR) return 1;
	}

	/* Initialize statistics structure before use. */
	TCinitStats(&stats);

	/* The demo animation sets every pixel in every frame.  Your code
	   doesn't necessarily have to --  it could just change altered
	   pixels and call TCrefresh().  The example is some swirly color
	   patterns using a combination of sine waves.  There's no meaning
	   to any of this, just applying various constants at each stage
	   in order to avoid repetition between the component colors. */
	for(x=0.0;;x += (double)pixelsPerStrand / 20000.0)
	{
		s1 = sin(x                 ) *  11.0;
		s2 = sin(x *  0.857 - 0.214) * -13.0;
		s3 = sin(x * -0.923 + 1.428) *  17.0;
		for(i=0;i<totalPixels;i++)
		{
			r   = (int)((sin(s1) + 1.0) * 127.5);
			g   = (int)((sin(s2) + 1.0) * 127.5);
			b   = (int)((sin(s3) + 1.0) * 127.5);
			pixelBuf[i] = TCrgb(r,g,b);
			s1 += 0.273;
			s2 -= 0.231;
			s3 += 0.428;
		}

		if((i = TCrefresh(pixelBuf,NULL,&stats)) != TC_OK)
			TCprintError(i);

		/* Update statistics once per second. */
		if((t = time(NULL)) != prev)
		{
#ifdef CYGWIN
			printf("\E[2J");
#else
			system("clear");
#endif
			TCprintStats(&stats);
			prev = t;
		}
	}

	TCclose();
	free(pixelBuf);
	return 0;
}
