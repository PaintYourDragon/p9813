/****************************************************************************
 File        : p9813.h

 Description : Header file for use with the p9813 library, for addressing
               "Total Control Lighting" RGB LED pixels from CoolNeon.com
               using an FTDI USB-to-serial cable or breakout board.  See
               included README.txt for background and detailed explanation
               of its use.

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

#ifndef _P9813_H_
#define _P9813_H_

#include <stdint.h>

typedef uint32_t TCpixel;

/* Special mode bits optionally added to first parameter to TCopen()    */
#define TC_CBUS_CLOCK 8   /* Use hardware for serial clock, not bitbang */
/* Hardware (CBUS) clock yields 2X throughput boost but requires a full
   FTDI breakout board with a specially-configured chip; this will not
   work with standard FTDI adapter cable (e.g. LilyPad programmer).
   See README.txt for further explanation.                              */

/* Special constants for the optional remap array passed to TCrefresh() */
#define TC_PIXEL_UNUSED       -1     /* Pixel is attached but not used  */
#define TC_PIXEL_DISCONNECTED -2     /* Pixel is not attached to strand */

/* FTDI pin-to-bitmask mappings */

#define TC_FTDI_TX  0x01  /* Avail on all FTDI adapters,  strand 0 default */
#define TC_FTDI_RX  0x02  /* Avail on all FTDI adapters,  strand 1 default */
#define TC_FTDI_RTS 0x04  /* Avail on FTDI-branded cable, strand 2 default */
#define TC_FTDI_CTS 0x08  /* Avail on all FTDI adapters,  clock default    */
#define TC_FTDI_DTR 0x10  /* Avail on third-party cables, strand 2 default */
#define TC_FTDI_DSR 0x20  /* Avail on full breakout board */
#define TC_FTDI_DCD 0x40  /* Avail on full breakout board */
#define TC_FTDI_RI  0x80  /* Avail on full breakout board */

/* Status codes returned by various functions */

typedef enum {
	TC_OK = 0,        /* Function completed successfully      */
	TC_ERR_VALUE,     /* Parameter out of range               */
	TC_ERR_MALLOC,    /* malloc() failure                     */
	TC_ERR_OPEN,      /* Could not open FTDI device           */
	TC_ERR_WRITE,     /* Error writing to FTDI device         */
	TC_ERR_MODE,      /* Could not enable async bit bang mode */
	TC_ERR_DIVISOR,   /* Could not set baud divisor           */
	TC_ERR_BAUDRATE   /* Could not set baud rate              */
} TCstatusCode;

/* Structure and variable types */

typedef struct {
	unsigned long frames;         /* Total number of frames output  */
	unsigned long bits;           /* Current frame, bits output     */
	unsigned long bitsTotal;      /* Total bits output, all frames  */
	unsigned long bps;            /* Write speed, current frame     */
	unsigned long bpsAvg;         /* Average bits/sec, all frames   */
	unsigned long usecIo;         /* I/O time for current frame     */
	unsigned long usecFrame;      /* Total time for current frame   */
	unsigned long usecIoTotal;    /* Total time for I/O, all frames */
	unsigned long usecFrameTotal; /* Total time for all frames      */
	double        fps;            /* Frames/sec, current frame      */
	double        fpsAvg;         /* Average fps, all frames        */
	double        ma;             /* Current used by frame          */
	double        maAvg;          /* Average current use            */
	double        maMax;          /* Peak current use               */
	double        mah;            /* Charge used by prior frame     */
	double        mahTotal;       /* Total charge used thus far     */
	unsigned long reserved;
} TCstats;

/* Function prototypes */

/* Merge separate R,G,B into TCpixel format; not a real function.
   Might add actual functions for HSL, HSV, etc. later on. */
#define TCrgb(R,G,B) (((R) << 16) | ((G) << 8) | (B))

extern TCstatusCode
	TCopen(unsigned char,int),
	TCinitStats(TCstats*),
	TCrefresh(TCpixel*,int*,TCstats*),
	TCsetGamma(unsigned char,unsigned char,double,
	           unsigned char,unsigned char,double,
	           unsigned char,unsigned char,double),
	TCsetGammaSimple(double),
	TCsetStrandPin(int,unsigned char);
extern void
	TCclose(void),
	TCdisableGamma(void),
	TCprintStats(TCstats*),
	TCprintError(TCstatusCode);

#endif  /* _P9813_H_ */
