/****************************************************************************
 File        : p9813.c

 Description : Library for addressing "Total Control Lighting" RGB LED
               pixels from CoolNeon.com using an FTDI USB-to-serial cable
               or breakout board.  See included README.txt for background
               and detailed explanation of its use.
               Presently depends on FTDI's D2XX library (libftd2xx), which
               is closed-source.  Might change this to use the open-source
               libftdi, or perhaps an either/or compile-time switch.

 History     : 6/15/2011  P. Burgess  Initial implementation
               6/18/2011  P. Burgess  Cleaned up and library-ified, added
                                      throughput and current statistics.
               6/28/2011  P. Burgess  Changed overall behavior to simplify
                                      use with continuous streaming data
                                      such as video.  Added remapping
                                      functionality and CBUS clock mode.

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
#include <math.h>
#include <time.h>
#ifdef CYGWIN
  #include <sys/time.h>
  #define va_list void
  #include <w32api/windef.h>
  #include <w32api/winbase.h>
#endif
#include <ftd2xx.h>
#include "p9813.h"
#include "calibration.h"

#define DEFAULT_GAMMA 2.4

/* The strandBitMask[] array maps pixel strands (numbered 0 to 6) to
   the corresponding pins that issue data through the FTDI chip.
   Element #7 is normally reserved for the serial clock signal, which
   can be remapped to a different pin just like any of the strands.
   If CBUS serial clock has been enabled, pin 7 is available as another
   data strand and does not need to be used for clocking.  See README.txt.

   In the library's default startup configuration, the DTR and RTS pins
   are toggled together to allow the same code to work unmodified with
   both the standard FTDI cable and others such as the SparkFun breakout; 
   the two types each place a different signal line in the last position.
   If using a full FTDI breakout, those bits can be separated with calls
   to TCsetStrandPin() in order to control the full allotment of strands
   independently. */

static unsigned char
	strandBitMask[8] = {
	  TC_FTDI_TX,                 /* Strand 0 data */
	  TC_FTDI_RX,                 /* Strand 1 data */
	  TC_FTDI_DTR | TC_FTDI_RTS,  /* Strand 2 data */
	  0,                          /*      ...      */
	  0,
	  0,
	  0,
	  TC_FTDI_CTS,                /* Serial clock  */
	};

static unsigned char
	bytesPerPixel   = 64,   /* Software bitbang clock by default */
	*pixelOutBuffer = NULL,
	rgbGamma[256][3];
static double
	*pixelCurrent   = NULL;
static FT_HANDLE
	ftdiHandle      = NULL;
static unsigned int
	nStrands        = 0,
	pixelsPerStrand = 0;

/* This internal function handles the actual FTDI init and memory alloc
   for the library, with graceful cleanup in all error cases.  Keeps
   subsequent TCopen() function simpler with regards to error handling. */
static TCstatusCode openAlloc(
  unsigned char s, /* Number of strands                  */
  int           p) /* Number of pixels in longest strand */
{
	/* Parameter validation was already done in TCopen(). */

	/* Function works from a presumed error condition progressing
	   toward success.  This makes the cleanup cases easier. */
	TCstatusCode status = TC_ERR_MALLOC;

	/* Size of pixelOutBuffer depends whether the serial clock is
	   provided by one of the CBUS pins or must be bit-banged via
	   software.  If using 8 strands, MUST use CBUS clock. */
	if(s >= TC_CBUS_CLOCK)
	{
		bytesPerPixel = 32;
		if(s > TC_CBUS_CLOCK) s -= TC_CBUS_CLOCK;
	} else
	{
		bytesPerPixel = 64;
	}

	/* All library memory use is handled in one big malloc.
	   The data types are sorted to avoid alignment issues. */
	if((pixelCurrent = (double *)malloc(
	    (s * p * sizeof(double)) +   /* pixelCurrent array +         */
	    ((p + 1) * bytesPerPixel)))) /* pixelOutBuffer (incl. latch) */
	{
		pixelOutBuffer = (unsigned char *)&pixelCurrent[s * p];

		/* Alloc successful.  Next phase... */
		status = TC_ERR_OPEN;

		/* Currently rigged for a single FTDI device,
		   and always index 0.  Might address this in
		   a future update if it's an issue. */
		if(FT_OK == FT_Open(0,&ftdiHandle))
		{
			status = TC_ERR_MODE;
			/* Currently hogs all pins as outputs,
			   whether they're used by strands or not. */
			if(FT_OK == FT_SetBitMode(ftdiHandle,255,1))
			{
				status = TC_OK; /* Tentative success */

				/* Try to set baud rate & divisor to non-
				   default values.  3090000 seems to be the
				   absolute max baud rate; even +1 more, and
				   it fails.  Failure of either of these
				   steps returns a warning but does not
				   abort; program can continue with default
				   baud rate setting.  FTDI docs suggest max
				   of 3000000; this may be pushing it. */
				if(FT_OK != FT_SetDivisor(ftdiHandle,1))
					status = TC_ERR_DIVISOR;
				if(FT_OK != FT_SetBaudRate(ftdiHandle,3090000))
					status = TC_ERR_BAUDRATE;

				/* Clear any lingering data in queue. */
				(void)FT_Purge(ftdiHandle,
				  FT_PURGE_RX | FT_PURGE_TX);

				return status; /* Success */
			}
			/* Else fatal error of some sort.
			   Clean up any interim results. */
			FT_Close(ftdiHandle);
		}
		ftdiHandle = NULL;
		free(pixelCurrent);
		pixelCurrent   = NULL;
		pixelOutBuffer = NULL;
	}
	return status; /* Fail */
}

/****************************************************************************
 Function    : TCopen()
 Description : Initializes the FTDI-to-p9813-LED library, allocating and
               initializing memory, opening the FTDI device, setting the
               gamma correction table to its default setting and issuing
               initial "all off" state to LEDs.
 Parameters  : unsigned char  Number of LED pixel strands to use.  Normally
                              1 to 7.  An 8th strand may be used only if
                              the FTDI chip is specifically configured to
                              provide an automatic serial clock signal on
                              one of the CBUS pins.  See README.txt for
                              further explanation.  It's okay to use fewer
                              than 8 strands with the CBUS clock, if the
                              value passed here is OR'd with TC_CBUS_CLOCK.
               int            Number of LED pixels per strand.  If strands
                              of different lengths are used, pass the
                              length of the longest strand.
 Returns     : TC_OK on success, else various error codes from header.
 ****************************************************************************/
TCstatusCode TCopen(
  unsigned char s,
  int           p)
{
	DWORD        out;  /* Return status from FT_Write */
	int          i,len;
	TCstatusCode status;

	if((s < 1) || (s > 16) || (p < 1)) return TC_ERR_VALUE;

	if(TC_OK != (status = openAlloc(s,p))) return status;

	nStrands        = (s > TC_CBUS_CLOCK) ? (s - TC_CBUS_CLOCK) : s;
	pixelsPerStrand = p;

	/* Issue latch sequence (sans LED data) before any other LED data
	   is written.  The latch is then subsequently written following
	   each frame of animation.  This is somewhat contrary to what the
	   datasheet says, but in practice syncs more reliably.  Latch only
	   needs to be "rendered" once at the end of pixelOutBuffer and
	   never changes after that, unless the clock pin is changed. */
	len = p * bytesPerPixel;
	bzero(&pixelOutBuffer[len],bytesPerPixel);  /* Latch is all zeros */
	if(64 == bytesPerPixel)
	{
		/* If software-bitbanging the clock, add those bits. */
		for(i=1;i<bytesPerPixel;i+=2)
			pixelOutBuffer[len + i] = strandBitMask[7];
	}
	if((FT_OK != FT_Write(ftdiHandle,&pixelOutBuffer[len],bytesPerPixel,
	  &out)) || (bytesPerPixel != out))
		return TC_ERR_WRITE;

	/* Issue initial blank image to LEDs ASAP. */
	if(TC_OK != (status = TCrefresh(NULL,NULL,NULL))) return status;

	/* Basic gamma correction is default behavior.  If gamma is not
	   desired, app should call TCdisableGamma() after TCopen(). */
	TCsetGammaSimple(DEFAULT_GAMMA);

	return TC_OK;
}

/* The P9813-based pixels normally provide a linear 1:1 mapping of color
   values to PWM duty cycle.  A fluke of human perception causes brightness
   increments at the lower end of the range to be much more noticeable than
   at the upper end; progressing linearly through RGB values will show a
   generally undesirable "topping off" in brightness.  Gamma correction
   applies a nonlinear function that results in a perceptually more linear
   sequence of brightness values.  TCopen() initializes the gamma curve to
   2.4, but this can be overridden by calling the following functions with
   alternate values.  IMPORTANT: these functions do not refresh the display;
   gamma change takes effect on subsequent call to TCrefresh(). */

/****************************************************************************
 Function    : TCsetGammaSimple()
 Description : Establishes a single gamma correction curve applied to
               subsequent TCrefresh() calls.
 Parameters  : double  Gamma correction factor.  Values greater than 1.0
                       result in dimmer (and generally more toward "correct")
                       mid-range pixels, less than 1.0 produce brighter
                       pixels.  1.0 = linear (uncorrected) gamma, which
                       produces PWM duty cycle and current usage directly
                       proportional to RGB values, but perceptually seen as
                       "too bright" in the middle.  2.4 = library default
                       and a reasonable starting point.
 Returns     : TC_OK on success, TC_ERR_VALUE if invalid parameter received.
 ****************************************************************************/
TCstatusCode TCsetGammaSimple(double g)
{
	unsigned short i;

	if(g <= 0.0) return TC_ERR_VALUE;

	for(i=0;i<256;i++)
	{
		rgbGamma[i][0] = rgbGamma[i][1] = rgbGamma[i][2] =
		  (unsigned char)(255.0 * pow((double)i / 255.0,g) + 0.5);
	}

	return TC_OK;
}

/****************************************************************************
 Function    : TCsetGamma()
 Description : Establishes brightness ranges and gamma-correction curves
               separately for red, green and blue, applied to subsequent
               TCrefresh() calls.  This helps correct color balance when
               handling images and video with specific perceived colors;
               the default off-balance color isn't nearly as distracting
               with entirely software-generated displays.  Note that this
               still isn't full-on color correction, just a simple halfway
               measure.
 Parameters  : unsigned char  Min. (dimmest) red value for output, 0-255.
               unsigned char  Max. (lightest) red value for output, 0-255.
               double         Gamma correction factor for the red color
                              component.  Behavior is the same as the
                              parameter passed to TCsetGammaSimple().
               (Parameters repeat for green and blue, 9 values total.)
 Returns     : TC_OK on success, TC_ERR_VALUE if invalid parameter received.
 ****************************************************************************/
TCstatusCode TCsetGamma(
  unsigned char rMin,
  unsigned char rMax,
  double        rGamma,
  unsigned char gMin,
  unsigned char gMax,
  double        gGamma,
  unsigned char bMin,
  unsigned char bMax,
  double        bGamma)
{
	unsigned short i;
	double         rRange,gRange,bRange,d;

	if((rGamma <= 0.0) || (gGamma <= 0.0) || (bGamma <= 0.0))
		return TC_ERR_VALUE;

	rRange = (double)(rMax - rMin);
	gRange = (double)(gMax - gMin);
	bRange = (double)(bMax - bMin);

	for(i=0;i<256;i++)
	{
		d = (double)i / 255.0;
		rgbGamma[i][0] = rMin +
		  (unsigned char)floor(rRange * pow(d,rGamma) + 0.5);
		rgbGamma[i][1] = gMin +
		  (unsigned char)floor(gRange * pow(d,gGamma) + 0.5);
		rgbGamma[i][2] = bMin +
		  (unsigned char)floor(bRange * pow(d,bGamma) + 0.5);
	}

	return TC_OK;
}

/****************************************************************************
 Function    : TCdisableGamma()
 Description : Disables gamma correction for subsequent TCrefresh() calls.
               Some programs may wish to provide their own color-correction
               models, or may have need for uncorrected "raw" color values
               (such as when calibrating current consumption).
 Parameters  : None (void).
 Returns     : Nothing (void).
 ****************************************************************************/
void TCdisableGamma(void)
{
	unsigned short i;

	for(i=0;i<256;i++)
		rgbGamma[i][0] = rgbGamma[i][1] = rgbGamma[i][2] = i;
}

/****************************************************************************
 Function    : TCinitStats()
 Description : Initializes a TCstats structure prior to use by subequent
               calls to the TCrefresh() function.  This generally only needs
               to be called once at program start, unless there's a specific
               desire to reset statistics for a new time interval.  Always
               init using this function; don't assume bzero() will cut it.  
 Parameters  : TCstats *  Pointer to TCstats structure.  Contents (if any)
                          will be overwritten.
 Returns     : Nothing (void).
 ****************************************************************************/
TCstatusCode TCinitStats(TCstats *stats)
{
	if(!stats) return TC_ERR_VALUE;

	bzero(stats,sizeof(TCstats));
	stats->fps      =
	stats->fpsAvg   =
	stats->ma       =
	stats->maMax    =
	stats->maAvg    =
	stats->mah      =
	stats->mahTotal = 0.0;

	return TC_OK;
}

#define EST_CURRENT(R,G,B)                                                \
	/* Base current...                 */                             \
	((double)CAL_CURRENT_OFF / (double)CAL_N_PIXELS) +                \
	/* Plus RGB current...             */                             \
	((((double)R*(double)CAL_CURRENT_R/(double)CAL_N_PIXELS) +        \
	  ((double)G*(double)CAL_CURRENT_G/(double)CAL_N_PIXELS) +        \
	  ((double)B*(double)CAL_CURRENT_B/(double)CAL_N_PIXELS))/255.0 * \
	/* ...times combinational factors. */                             \
	(1.0-((double)R*(double)G/(255.0*255.0)*(1.0-CAL_COMBO_RG))) *    \
	(1.0-((double)G*(double)B/(255.0*255.0)*(1.0-CAL_COMBO_GB))) *    \
	(1.0-((double)R*(double)B/(255.0*255.0)*(1.0-CAL_COMBO_RB))))

/****************************************************************************
 Function    : TCrefresh()
 Description : Updates LED display; pushes data out "on the wire" via the
               FTDI adapter.
 Parameters  : TCPixel *  Image data, as a one-dimensional array.  If NULL,
                          entire image is set to "off" state.
               int *      Optional remapping table, assigns each pixel in
                          each strand to a position in the TCpixel array
                          passed as the first marameter.  If NULL, each
                          element of the TCpixel array is assumed to
                          correspond sequentially to each pixel in each
                          strand, and gaps in strands are not handled.
               TCstats *  Optional pointer to structure for receiving
                          performance statistics.  Pass NULL if this
                          information is not needed.
 Returns     : TC_OK on success, TC_ERR_WRITE on I/O error.
 ****************************************************************************/
TCstatusCode TCrefresh(
  TCpixel *pixelInBuffer,
  int     *remap,
  TCstats *stats)
{
	DWORD          out;
	int            i,s,p,len,absPixel,mappedPixel;
	unsigned char  r,g,b,strand,*addr;
	unsigned long  rgb,time1;
	struct timeval t;
	TCstatusCode   status;

	/* PHASE 1: Convert data from pixelInBuffer to pixelOutBuffer ----- */

	/* Clear output buffer, leaving latch intact at end.  For software-
	   bitbanged clock signal, clock ticks are added now rather than in
	   subsequent loop because the strand/pixel remapping tables could
	   leave gaps in the sequence -- it isn't guaranteed to have touched
	   every pixel.  This is normal and not a bad thing. */
	if(bytesPerPixel == 64)
	{
		strand = strandBitMask[7];
		len    = pixelsPerStrand * bytesPerPixel;
		for(i=0;i<len;)
		{
			pixelOutBuffer[i++] = 0;
			pixelOutBuffer[i++] = strand;
		}
	} else
	{
		bzero(pixelOutBuffer,pixelsPerStrand * bytesPerPixel);
	}

	/* The structure of the pixelOutBuffer[] array is described in the
	   Hack-a-Day article referenced in the README.  Picture it like one
	   long player piano roll, where each key on the piano corresponds
	   to one GPIO bit.  Thus data (including the clock signal) must be
	   "turned sideways" in this array, through a series of bitwise
	   operations. */
	for(absPixel=s=0;s<nStrands;s++)
	{
	  strand = strandBitMask[s];
	  for(p=0;p<pixelsPerStrand;p++,absPixel++)
	  {
	    mappedPixel = remap ? remap[absPixel] : absPixel;

	    /* Get RGB value and current use for this pixel */
	    if((NULL == pixelInBuffer) || (mappedPixel < 0))
	    {
	      rgb                    = 0xff000000;
	      pixelCurrent[absPixel] =
	        (mappedPixel == TC_PIXEL_DISCONNECTED) ? 0.0 :
	        ((double)CAL_CURRENT_OFF / (double)CAL_N_PIXELS);
	    } else
	    {
	      /* Separate components, run through gamma tables. */
	      r = rgbGamma[(pixelInBuffer[mappedPixel] >> 16) & 0xff][0];
	      g = rgbGamma[(pixelInBuffer[mappedPixel] >>  8) & 0xff][1];
	      b = rgbGamma[ pixelInBuffer[mappedPixel]        & 0xff][2];

	      /* And reassemble into P9813 32-bit format. */
	      rgb = (b << 16) | (g << 8) | r | /* 24-bit color +  */
	        (~(((b & 0xc0) << 22) |        /* checksum as per */
	           ((g & 0xc0) << 20) |        /* LED datasheet   */
	           ((r & 0xc0) << 18)) & 0xff000000);

	      pixelCurrent[absPixel] = EST_CURRENT(r,g,b);
	    }

	    /* Turn pixel "sideways" into output buffer. */
	    addr = &pixelOutBuffer[p * bytesPerPixel]; /* Base addr */
	    if(bytesPerPixel == 64)
	    {
	      for(;rgb;rgb<<=1)
	      {
	        if(rgb & 0x80000000)
	        {
	          addr[0] |= strand;
	          addr[1] |= strand;
	        }
	        addr += 2;
	      }
	    } else
	    {
	      for(;rgb;rgb<<=1)
	      {
	        if(rgb & 0x80000000) *addr |= strand;
	        addr++;
	      }
	    }
	  }
	}

	/* PHASE 2: Issue serial data. ------------------------------------ */

	/* Total number of bytes to output; includes latch data at end. */
	len = (pixelsPerStrand + 1) * bytesPerPixel;

	/* Get current time (in microseconds) both before and after
	   write operation.  This is to isolate I/O-bound statistics
	   from overall timing data (which includes frame rendering
	   time, etc.). */
	gettimeofday(&t,NULL);
	time1 = (t.tv_sec * 1000000) + t.tv_usec;

	/* Function does not immediately return on write error.  Some
	   of the subsequent statistics may still be valid for reference
	   use, even if not issued to the chip (e.g. estimating the
	   total current use of specific LED patterns). */
	status = ((FT_OK == FT_Write(ftdiHandle,pixelOutBuffer,len,&out)) &&
	          (len == out)) ? TC_OK : TC_ERR_WRITE;

	/* PHASE 3: (Optionally) generate statistics ---------------------- */

	if(stats)
	{
		double        sum;
		int           i;
		unsigned long time2;

		gettimeofday(&t,NULL);
		time2 = (t.tv_sec * 1000000) + t.tv_usec;

		/* Parallel output bits are included in I/O calculations. */
		stats->bits       = nStrands * len;
		if(bytesPerPixel == 64) stats->bits /= 2;
		stats->bitsTotal += stats->bits;

		/* Get I/O elapsed time and compute throughput for this
		   single frame. */
		if((stats->usecIo = (time2 - time1)) > 0)
		{
			stats->bps = (unsigned long)(
			  ((double)stats->bits * 1000000.0) /
			   (double)stats->usecIo);
			stats->usecIoTotal += stats->usecIo;
		} else
		{
			stats->bps = 0;  /* Probably I/O error */
		}

		/* Compute average throughput from total bits output and
		   cumulative I/O time.  Avoid divide-by-zero first: */
		if(stats->usecIoTotal)
		{
			stats->bpsAvg = (unsigned long)(
			  ((double)stats->bitsTotal * 1000000.0) /
			   (double)stats->usecIoTotal);
		} else
		{
			stats->bpsAvg = stats->bps;
		}

		/* Some figures cannot be calculated until multiple frames
		   have been rendered and output. */
		if(stats->frames)
		{
			/* The 'reserved' element of the stats structure
			   is actually the saved value of 'time2' from the
			   prior invocation of this function; used to
			   determine the total processing time for frame. */
			stats->usecFrame = time2 - stats->reserved;
			if(stats->usecFrame)
			{
			  stats->fps = 1000000.0 / (double)stats->usecFrame;
			  stats->usecFrameTotal += stats->usecFrame;
			} else
			{
			  stats->fps = 0.0;  /* Probably I/O error */
			}

			if(stats->usecFrameTotal)
			{
				stats->fpsAvg = (double)stats->frames *
				  1000000.0 / (double)stats->usecFrameTotal;
			}

			/* Milliamp-hour calculations need to work from the
			   PRIOR frame, so don't calculate the mA value of
			   the new frame yet!  Use the old one... */
			stats->mah = stats->ma * ((double)stats->usecFrame) /
			  (1000000.0 * 60.0 * 60.0);
			stats->mahTotal += stats->mah;

			/* Average current is back-calculated from total
			   mAH and total time, NOT simply total current
			   and total frames.  This gives an average-per-
			   unit-of-time (generally constant by the laws
			   of physics) rather than an average-per-frame
			   (variable by CPU power and frame complexity). */
			stats->maAvg = stats->mahTotal *
			  (1000000.0 * 60.0 * 60.0) /
			  (double)stats->usecFrameTotal;
		}

		/* With mAH calculations done, the mA estimate can now be
		   updated for the new frame. */
		len = nStrands * pixelsPerStrand;
		for(sum=0.0,i=0;i<len;i++) sum += pixelCurrent[i];
		stats->ma = sum;
		if(stats->ma > stats->maMax) stats->maMax = stats->ma;

		stats->reserved = time2;  /* Save for next time */
		stats->frames++;
	}

	return status;
}

/****************************************************************************
 Function    : TCsetStrandPin()
 Description : Assign one or more pins on the FTDI adapter to a specific
               pixel strand.  As with prior functions, this does not
               refresh the display; it applies only to subsequent
               TCrefresh() commands.  This is best done before TCopen().
 Parameters  : int            Strand number to change (0-7).  7 is normally
                              reserved as the serial clock line (but may
                              still be assigned to a different pin or pins).
               unsigned char  Pin(s) on FTDI adapter that will issue serial
                              data for this strand.  Pin numbers are defined
                              in p9813.h (e.g. TC_FTDI_TX refers to the TX
                              pin on the FTDI adapter).  It's permissible to
                              OR multiple pin values to have them operate
                              together, and in fact this is the library's
                              default behavior for strand #2, which is
                              mapped to both the DTR and RTS lines.
                              Different Arduino serial adapters use a
                              Different signal pin in the last position;
                              toggling both ensures compatibility with
                              both types.
 Returns     : TC_OK on success, else various error codes from header.
 ****************************************************************************/
TCstatusCode TCsetStrandPin(
  int           strand,
  unsigned char bit)
{
	/* When checking parameters, max strand number is always 7, NOT
	   nStrands.  TCopen() may not have been called yet, so nStrands
	   is unknown.  This function is best used before TCopen() so
	   that initial latch and screen-clearing functions work. */
	if((strand < 0) || (strand > 7) || !bit) return TC_ERR_VALUE;

	strandBitMask[strand] = bit;

	/* If using bitbang clock mode, and strand 7 (clock line) is
	   requested, and pixelOutBuffer was previously allocated by
	   TCopen(), re-render the clock bits for the latch signal at
	   the end of the buffer. */
	if((bytesPerPixel == 64) && (strand == 7) && pixelOutBuffer)
	{
		unsigned int  offset;
		unsigned char i;

		offset = pixelsPerStrand * bytesPerPixel;
		for(i=1;i<bytesPerPixel;i+=2)
			pixelOutBuffer[offset + i] = bit;
	}

	return TC_OK;
}

/****************************************************************************
 Function    : TCclose()
 Description : Close FTDI connection and free any data previously allocated
               by the library.
 Parameters  : None (void).
 Returns     : Nothing (void).
 ****************************************************************************/
void TCclose(void)
{
	if(ftdiHandle)
	{
		FT_Close(ftdiHandle);
		ftdiHandle   = NULL;
	}
	if(pixelCurrent)
	{
		free(pixelCurrent);
		pixelCurrent   = NULL;
		pixelOutBuffer = NULL;
	}
	nStrands        = 0;
	pixelsPerStrand = 0;
}

/****************************************************************************
 Function    : TCprintStats()
 Description : Displays contents of a TCstats structure to stdout.
               Somewhat of a kludge for debugging purposes -- a well-polished
               program really should display any interesting elements of the
               TCstats structure as per its own needs.
 Parameters  : TCstats *  Pointer to TCstats structure previously passed to
                          TCrefresh().
 Returns     : Nothing (void).
 ****************************************************************************/
void TCprintStats(TCstats *stats)
{
	if(!stats) return;

        (void)printf(
	  "Total frames               : %ld\n"
          "Bits in this frame         : %ld\n"
          "Total bits output          : %ld\n"
          "Write speed for this frame : %ld bits/sec\n"
          "Average write speed        : %ld bits/sec\n"
          "I/O time for this frame    : %ld uS\n"
          "Total time for this frame  : %ld uS\n"
          "Total I/O time, all frames : %ld uS\n"
          "Total time, all frames     : %ld uS (%ld seconds)\n"
          "FPS for this frame         : %.1f\n"
          "Average frames/second      : %.1f\n"
          "Current use for this frame : %.3f mA (@5.0V)\n"
          "Average current            : %.3f mA (@5.0V)\n"
          "Peak current               : %.3f mA (@5.0V)\n"
          "Charge for prior frame     : %f mAH (@5.0V)\n"
          "Total charge, all frames   : %.3f mAH (@5.0V)\n\n",
	  stats->frames,
	  stats->bits,
	  stats->bitsTotal,
	  stats->bps,
	  stats->bpsAvg,
	  stats->usecIo,
	  stats->usecFrame,
	  stats->usecIoTotal,
	  stats->usecFrameTotal,stats->usecFrameTotal / 1000000L,
	  stats->fps,
	  stats->fpsAvg,
	  stats->ma,
	  stats->maAvg,
	  stats->maMax,
	  stats->mah,
	  stats->mahTotal);
}

/****************************************************************************
 Function    : TCprintError()
 Description : Given a TCstatusCode value, prints a (hopefully) informative
               message to stdout.  As with priot function, this is a bit of
               a kludge, and a well-polished program really should handle
               error and warning situations in a manner consistent with its
               own user interface (if not command-line based).
 Parameters  : TCstatusCode   Status code value returned by any of the
                              library functions.
 Returns     : Nothing (void).
 ****************************************************************************/
void TCprintError(TCstatusCode status)
{
	const char *msg[] = {
	  "Function completed successfully -- no error.",
	  "ERROR: Parameter out of range.",
	  "ERROR: Could not allocate RAM.  Most likely a parameter is\n"
	  "       way out of range, but perhaps the system is unfathomably\n"
	  "       swamped; try quitting other programs.",
	  "ERROR: Could not open FTDI device.\n"
	  "       Is the USB cable connected?\n"
	  "       Is the Virtual COM Port driver properly disabled?\n"
	  "       Is another program already using the device?",
	  "ERROR: Failed to write to FTDI device.  Has it been disconnected?",
	  "ERROR: Could not enable bitbang mode.\n"
	  "       Is this an FTDI USB-to-serial device?",
	  "WARNING: Could not set I/O divisor.  Library code may be outside\n"
	  "         valid range for this FTDI device, but program may choose\n"
	  "         to continue with default setting.",
	  "WARNING: Could not set I/O baud rate.  Library code may be \n"
	  "         outside valid range for this FTDI device, but program\n"
	  "         may choose to continue with default setting."
	};

	if((status >= 0) && (status < (sizeof(msg) / sizeof(msg[0]))))
		(void)puts(msg[status]);
}
