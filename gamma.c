/****************************************************************************
 File        : gamma.c

 Description : Example program for the p9813 library.  Renders a ramp of
               gray intensities across all pixels (for multiple strands,
               all strands will display the same ramp), applying the given
               gamma-correction curve.  The goal is to identify a gamma
               value that provides a perceptually linear range of values
               through the ramp.

               Example calling sequence:

               gamma -s 4 -p 25 -g 2.2

               The first two parameters set the number of LED strands and
               the number of pixels per strand, respectively; the above
               example would be for 4 strands of 25 pixels each, or 100
               pixels total.  Default state is for one strand of 25 pixels.
               If strands are different lengths, specify longest strand.
               The next parameter sets the gamma adjustment value; the
               default for this program is 1.0 (no correction applied),
               though the default behavior for the library if left
               unspecified is 2.4.  Other values may be tested here and
               used in one's own code with a call to TCsetGammaSimple();
               issuing a value to this program does not set this new state
               permanently.  Parameters may be issued in any order, and
               may be ommitted to use corresponding defaults.

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
#include <string.h>
#include <unistd.h>
#include "p9813.h"

int main(int argc,char *argv[])
{
	double  g         = 1.0;
	int     i,s,p,totalPixels,
	  nStrands        = 1,
	  pixelsPerStrand = 25,
	  *remap          = NULL;
	TCpixel *pixelBuf;

	while((i = getopt(argc,argv,"s:p:g:")) != -1)
	{
		switch(i)
		{
		   case 's':
			nStrands        = strtol(optarg,NULL,0);
			break;
		   case 'p':
			pixelsPerStrand = strtol(optarg,NULL,0);
			break;
		   case 'g':
			g               = strtod(optarg,NULL);
			break;
		   case '?':
		   default:
			(void)printf(
			  "usage: %s [-s strands] [-p pixels] [-g gamma]\n",
			  argv[0]);
			return 1;
		}
	}

	/* Allocate pixel array.  Only one strand's worth is allocated;
	   since all strands show the same ramp, remapping is used to
	   redirect all strands to the same data. */
	i = pixelsPerStrand * sizeof(TCpixel);
	if(NULL == (pixelBuf = (TCpixel *)malloc(i)))
	{
		printf("Could not allocate space for %d pixels (%d bytes).\n",
		  pixelsPerStrand,i);
		return 1;
	}
	/* Allocate remap array if needed (if multiple strands). */
	if(nStrands > 1)
	{
		totalPixels = nStrands * pixelsPerStrand;
		i           = totalPixels * sizeof(int);
		if(NULL == (remap = (int *)malloc(i)))
		{
			printf("Could not allocate map "
			  "for %d pixels (%d bytes).\n",
			  totalPixels,i);
			return 1;
		}
	}

	/* Initialize library, open FTDI device.  Baud rate errors
	   are non-fatal; program displays a warning but continues. */
	if((i = TCopen(nStrands,pixelsPerStrand)) != TC_OK)
	{
		TCprintError(i);
		if(i < TC_ERR_DIVISOR) return 1;
	}

	if((i = TCsetGammaSimple(g)) != TC_OK) TCprintError(i);

	/* Render gamma-adjusted ramp of intensities across one strand.
	   "Correct" ramp should appear perceptually linear. */
	for(p=0;p<pixelsPerStrand;p++)
	{
		i = 255 * p / pixelsPerStrand;
		pixelBuf[p] = TCrgb(i,i,i);
	}

	/* Render remapping table to point all strands to the same data. */
	if(nStrands > 1)
	{
		for(p=0;p<pixelsPerStrand;p++) remap[p] = p;
		for(s=1;s<nStrands;s++)
		{
			memcpy(&remap[s * pixelsPerStrand],remap,
			  pixelsPerStrand * sizeof(int));
		}
	}

	if((i = TCrefresh(pixelBuf,remap,NULL)) != TC_OK) TCprintError(i);

	TCclose();
	if(remap) free(remap);
	free(pixelBuf);
	return 0;
}
