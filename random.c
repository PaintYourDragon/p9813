/****************************************************************************
 File        : random.c

 Description : Example program for the p9813 library.  Sets all pixels on
               all strands to random RGB values.  Used in conjunction with
               a current meter, this can be used to test the accuracy of
               the library's current estimation feature.

               Example calling sequence:

               random -s 4 -p 25

               The two parameters set the number of LED strands and the
               number of pixels per strand, respectively; the above example
               would be for 4 strands of 25 pixels each, or 100 pixels
               total.  Default state is for one strand of 25 pixels.
               If strands are different lengths, specify the longest strand.
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
#include <time.h>
#include "p9813.h"

int main(int argc,char *argv[])
{
	int     i,totalPixels,
	  nStrands        = 1,
	  pixelsPerStrand = 25;
	TCstats stats;
	TCpixel *pixelBuf;

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

	/* Allocate pixel array (one TCpixel per pixel per strand). */
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

	/* Seed the random number generator with the current system time,
	   then set all pixels on all strands to random RGB values. */
	srand(time(NULL));
	for(i=0;i<totalPixels;i++)
		pixelBuf[i] = TCrgb(rand()&255,rand()&255,rand()&255);

	if((i = TCrefresh(pixelBuf,NULL,&stats)) == TC_OK)
		TCprintStats(&stats);
	else
		TCprintError(i);

	TCclose();
	free(pixelBuf);
	return 0;
}
