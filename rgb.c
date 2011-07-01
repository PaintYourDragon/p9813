/****************************************************************************
 File        : rgb.c

 Description : Example program for the p9813 library.  Issues a single color
               to all LED pixels on all strands.  This is used for setting
               the display to a known state in order to measure calibration
               constants.  See calibration.h for further explanation.

               Example calling sequence:

               rgb -s 4 -p 25 -r 20 -g 0 -b 255 -c

               The first two parameters set the number of LED strands and
               the number of pixels per strand, respectively; the above
               example would be for 4 strands of 25 pixels each, or 100
               pixels total.  Default state is for one strand of 25 pixels.
               If strands are different lengths, specify the length of the
               longest strand.  The next three parameters set the red,
               green and blue color components for all pixels; slightly
               magenta-tinged blue in the above case.  Default state is
               0 0 0, or all pixels off.  The last parameter tells the
               program to display statistical information (bandwidth,
               frames-per-second, current use, etc.) continuously;
               otherwise, statistics are printed once and the program then
               exits; the latter is the default.  In either case, the LEDs
               will be left in this state.  Parameters may be issued in any
               order, and may be ommitted to use corresponding defaults.

               This code illustrates an extreme use of the 'remap'
               functionality: only a single TCpixel is used, with every
               pixel on every strand remapped to that same element.  It
               doesn't have to be done this way (could have just used a
               big array of TCpixels with no remapping), but wanted a
               thorough example of this feature in action.

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
#include <strings.h>
#include <unistd.h>
#include <time.h>
#include "p9813.h"

int main(int argc,char *argv[])
{
	int     i,*remap,
	  nStrands        = 1,
	  pixelsPerStrand = 25;
	unsigned char
	  r               = 0,  /* Red component for all pixels          */
	  g               = 0,  /* Green component for all pixels        */
	  b               = 0,  /* Blue component for all pixels         */
	  continuous      = 0;  /* Show continuous statistics, else exit */
	TCpixel pixel;
	TCstats stats;

	/* Bounds checking is NOT performed on inputs.  This is
	   intentional, so that "reasonable" range values are
	   reinforced to the user -- the program will report any
	   out-of-range conditions returned by the library. */

	while((i = getopt(argc,argv,"r:g:b:s:p:c")) != -1)
	{
		switch(i)
		{
		   case 'r':
			r               = strtol(optarg,NULL,0);
			break;
		   case 'g':
			g               = strtol(optarg,NULL,0);
			break;
		   case 'b':
			b               = strtol(optarg,NULL,0);
			break;
		   case 's':
			nStrands        = strtol(optarg,NULL,0);
			break;
		   case 'p':
			pixelsPerStrand = strtol(optarg,NULL,0);
			break;
		   case 'c':
			continuous      = 1;
			break;
		   case '?':
		   default:
			(void)printf(
			  "usage: %s [-r rval] [-g gval] [-b bval] "
			  "[-s strands] [-p pixels] [-c]\n",argv[0]);
			return 1;
		}
	}

	/* Allocate remapping array (one int per pixel per strand) */
	i = nStrands * pixelsPerStrand;
	if(NULL == (remap = (int *)malloc(i * sizeof(int))))
	{
		printf("Could not allocate map for %d pixels (%d bytes).\n",
		  i,(int)(i * sizeof(int)));
		return 1;
	}

	/* Initialize library, open FTDI device.  Baud rate errors
	   are non-fatal; program displays a warning but continues. */
	if((i = TCopen(nStrands,pixelsPerStrand)) != TC_OK)
	{
		TCprintError(i);
		if(i < TC_ERR_DIVISOR) return 1;
	}

	/* This program needs to issue "raw" pixel values for testing
	   and power calibration, so disable gamma correction. */
	TCdisableGamma();

	/* All pixels on all strands will be remapped to the same
	   single input.  Initialize that one RGB value, then set the
	   entire remap array to 0 to point to the first (and in this
	   case the only) TCpixel element. */
	pixel = TCrgb(r,g,b);
	bzero(remap,nStrands * pixelsPerStrand * sizeof(int));

	/* Initialize statistics structure before use. */
	TCinitStats(&stats);

	if(continuous)
	{
		time_t t,prev = 0;

		/* If running in continuous mode, the statistical display
		   is updated roughly once per second, but the TCrefresh()
		   call is made to run in a tight loop during this inverval
		   in order to provide a frames-per-second estimate. */
		for(;;)
		{
			if((i = TCrefresh(&pixel,remap,&stats)) != TC_OK)
				TCprintError(i);
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
	} else
	{
		/* Non-continuous -- display statistics once and exit. */
		if((i = TCrefresh(&pixel,remap,&stats)) != TC_OK)
			TCprintError(i);
		TCprintStats(&stats);
	}

	TCclose();
	free(remap);
	return 0;
}
