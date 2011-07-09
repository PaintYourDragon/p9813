                      p9813 RGB LED Pixel library
                     Copyright 2011 Phillip Burgess
                        www.PaintYourDragon.com


                               BACKGROUND

This library serves to interface Total Control Lighting RGB LED pixels
(from www.CoolNeon.com) to a PC host (Windows, Mac, Linux) using an
FTDI USB-to-serial cable or breakout board.  The goal is to provide an
inexpensive, high-speed PC connection for displaying video or for other
tasks that may be too demanding for a standalone microcontroller.
This basic interface is far less costly than the high-end Elite or Pro
controllers from the same company, and the code is open and adaptible.
The FTDI cable can provide significantly higher throughput than using a
microcontroller such as Arduino as a PC-to-pixel bridge, costs less,
and by centralizing all code on the PC host (rather than split between
host and microcontroller) eases maintenance and debugging.

The simplest interface is either the OEM FTDI USB-to-serial cable
(typically around $18) or the SparkFun FTDI Basic Breakout ($15).
Many Arduino users will already have one of these lying around -- it's
the same adapter used for programming Arduino Pro, LilyPad and similar
devices.  These interfaces can control up to three LED pixel strands
of any length.  More sophisticated FTDI interfaces such as the Sparkfun
Breakout Board for FT232RL USB to Serial ($15) can control up to eight
pixel strands, and with the potential for higher data rates.

Using an Arduino as a PC-to-pixel bridge is limited to the maximum serial
speed of 115,200 bits per second.  Each Total Control Lighting pixel must
receive 32 bits of data, yielding a theoretical absolute upper refresh
rate of 3,600 pixels per second (or 120 pixels at 30 Hz, and so forth).
Using an FTDI adapter as the PC-to-pixel bridge, the data rate can in
some cases push upwards of eight million bits per second.  The simple
FTDI cable can transfer up to 45,000 pixels/second (or about 1,500 pixels
at 30 Hz) while the full FTDI breakout board can manage up to 240,000
pixels/second (about 8,000 pixels at 30 Hz).  Actual throughput will vary
with the number of pixel strands and the host computer's ability to
render frames at these rates, but most modern PCs should be able to keep
up with something close to those numbers.

This idea relies on FTDI "bitbang mode," which runs contrary to the normal
"serial bridge" mode that these adapters typically use.  See this article
for an explanation of bitbang mode:

  http://hackaday.com/2009/09/22/introduction-to-ftdi-bitbang-mode/

As a bonus feature, the library can estimate power use for LED pixel
displays without requiring a meter or even the full complement of LEDs
to be connected.  This can be useful for planning power supply, battery
and wiring needs at an early stage before scaling up a project.

The code here consists of a "raw" C library and a few simple command-line
utilities with which to test it and to learn from; there is no polished
GUI provided.  The intent is that one may link this library into their
own platform-specific front-end application, with whatever presentation
that may require.

IMPORTANT NOTE ABOUT COMPATIBILITY: There are many similar-looking makes
and models of RGB LED pixels.  This code -- and in fact the entire
approach of using an FTDI cable as an interface -- WILL NOT WORK with
other pixels based on the LPD1101 or LPD6803 driver chips, ever, despite
their outwardly identical appearance.  Those types require a rock-steady
combined serial and PWM clock which the FTDI interface cannot provide,
though they are well suited for microcontroller use.  The P9813 driver
chip supplies its own internal PWM clock independent of serial I/O, which
makes this entire FTDI scheme possible.

There are repeated references to Total Control Lighting and CoolNeon.com
throughout this code and documentation, but this work was developed
independently without their direct influence -- they were simply the
first (and, as of this writing, only) stateside distributor of P9813-
based LED pixels.  If you encounter difficulty with the library, please
bring it to the author's attention and do not pester CoolNeon with
software support issues.  Thanks!



                           INTERFACING HARDWARE

The Total Control Lighting pixels have four colored wires with the
following functions:

	RED    +5VDC
	WHITE  Serial Data
	GREEN  Serial Clock
	BLUE   Ground

Cool Neon sells their pixel strands with a weatherproof plug soldered
at each end.  These plugs have a specific "gender" for daisy-chaining
multiple strands.  We'll refer to the data input side as the "head" of
the strand, and the opposite end (after the last pixel, where additional
strands can be daisy-chained) as the "tail":

	HEAD -> pixel -> pixel-> ... -> pixel -> pixel -> TAIL

To connect these strands to our own devices (rather than just the
controllers they offer), it's highly recommended to order some extra
tail end plugs; they are a peculiar type that is hard to come by.
Alternately, the tail end can be cut off a strand and used as an adapter.
With its head end intact, that strand will still work with the standard
controllers and can be attached to the tail end of other strands, but can
no longer have additional strands easily attached to its tail:

	FTDI -> TAIL -> HEAD -> pixel -> ... -> pixel -> pixel

The wires inside these head/tail plugs are color-coded differently:

	RED     +5VDC
	YELLOW  Serial Clock
	GREEN   Serial Data
	BLUE    Ground

Some FTDI adapters have labeled pins, while others are color-coded.  The
colors and corresponding functions are:

	BLACK   Ground
	BROWN   CTS
	RED     +5V
	ORANGE  TX
	YELLOW  RX
	GREEN   RTS (OEM FTDI cable) or DTR (third-party adapters)

Pin #6 (green) may have either of two functions depending on the source,
but the library will work with either type.

In bitbang mode, the labeled identities of the pins no longer correspond
to their functions; they are simply "raw" TTL signal lines under software
control.  Because adapters and datasheets are labeled with the traditional
serial function for each pin, it's easier to continue to refer to it by
that label.  For example, CTS (Clear To Send, used in serial hardware
handshaking) no longer performs said function...that pin is used as the
serial clock signal...but is still referred to as CTS here.  Likewise with
the other pins.

For the simplest case of a single strand of LEDs, here's how the wiring
to an FTDI adapter would work:

    USB ----> FTDI    CTS >-----------------> Clock
             CABLE     TX >-----------------> Data    LED
                   Ground >-------+---------> Ground  STRAND ---->
                                  | +-------> +5VDC
                                  | |
                                  | |
           POWER   Ground >-------+ |
           SUPPLY   +5VDC >---------+

Power should be provided by a "wall wart" power adapter or regulated power
supply.  Do not attempt to power the strand from the FTDI cable -- the USB
line can't supply enough current.  And do not connect the +5V from the
power supply to the FTDI adapter.  This could damage the adapter, or
worse, your computer.  The ground lines should be tied to all three end
devices -- power supply, FTDI cable and LED strand.

When controlling large numbers of LEDs, better throughput can be achieved
by connecting multiple strands in parallel rather than one continuous
strand.  There's little I/O or processing overhead for these extra strands.
For example, given a single continuous strand of 600 pixels, reconfiguring
this as two parallel strands of 300 pixels each will update the display
nearly twice as fast, while three strands of 200 pixels each can be nearly
three times the speed.  Overall performance will be limited by the longest
strand, not the average length...while physical reality will ultimately
dictate specific wiring configurations, try to balance strand lengths as
best as possible.

To use multiple strands in parallel, the data line for each is connected
to the next available pin on the FTDI adapter.  Clock, +5V (from the power
supply, not the FTDI cable) and ground should now be connected to all
strands:

    USB ----> FTDI    CTS >-------+---------> Clock
             CABLE     TX >-------|---------> Data    LED
                   Ground >-------|-+-------> Ground  STRAND ---->
                       RX >-----+ | | +-----> +5VDC   "ZERO"
                  DTR/RTS >---+ | | | |
                              | | +-|-|-----> Clock
                              | +-|-|-|-----> Data    LED
                              |   | +-|-----> Ground  STRAND ---->
                              |   | | +-----> +5VDC   "ONE"
                              |   | | |
                              |   +-|-|-----> Clock
                              +-----|-|-----> Data    LED
           POWER   Ground >---------+-|-----> Ground  STRAND ---->
           SUPPLY   +5VDC >-----------+-----> +5VDC   "TWO"

To drive more than three strands in parallel requires a full breakout
board that provides access to all pins of the FTDI chip; up to eight
strands can be driven in parallel this way.  Remember that clock, +5V
(from the power supply) and ground should connect to all strands, and
then the data line to each strand is unique.  Library functions
(explained later) are available to assign which pins connect to the
data input of each strand.



                  PREREQUISITE LIBRARY AND DRIVER ISSUES

The FTDI driver is standard issue on mainstream operating systems, and
should already be present on Windows, Mac or Linux systems.  However,
the p9813 library is dependent on FTDI's "D2XX" library, which is not
present by default, even if you've previously installed other FTDI-based
applications such as Arduino.  The library files (with installation
instructions) for various operating systems can be downloaded from:

	http://www.ftdichip.com/Drivers/D2XX.htm

p9813 does not yet support the alternative open-source libftdi library,
though that might be added or entirely switched to in the future.

Linux users will need to set up a "udev rule" in order to allow non-root
access to the FTDI device.  This only needs to be done once:

	sudo cp ftdi.rules /etc/udev/rules.d/
	sudo udevadm control --reload-rules

An unfortunate artifact of the Mac and Linux versions of the FTDI driver
is that the "Virtual COM Port" kernel extension must be disabled before
bitbang mode (and the p9813 library) can be used.  Applications such as
Arduino depend on the serial port for programming and communication, so
these cannot be used at the same time.  The included Makefile includes
two commands, "make unload" and "make load", that will disable the serial
port functionality (enabling bitbang mode), and restore it (enabling
Arduino, etc.), respectively.  Remember to ALWAYS "make load" when you
are done using bitbang mode.  Forgetting this step can be very aggravating
later on when trying to do normal serial communication such as Arduino
programming.  This is a non-issue with the Windows driver, which can
switch between serial and bitbang modes transparently.



                            USING THE LIBRARY

Following the installation of the prerequisite library, the Makefile
included here will build the p9813 C library and related test programs on
most Unix-like systems (Mac OS X, Linux, and Windows if running Cygwin).
The library compiles and can be used with Xcode for Mac.  This has not
yet been tested with a Windows IDE such as Visual Studio, but can likely
be coerced with little trouble, perhaps just some tweaks to the #includes.

The library's functions and constants are prefixed with "TC" (after
"Total Control" Lighting).  Though similar LEDs may eventually become
available through other sources, this was my first exposure to P9813-
based pixels and the name stuck (the abbreviation "TCL" was not used
in order to avoid confusion with the Tool Command Language (Tcl), a
popular language with significant mindshare, but entirely unrelated).
Following the "TC" prefix, functions and data types use a brief but
descriptive name in camel case (e.g. TCsetGamma()).

Seasoned programmers can likely puzzle out the use of the library simply
by looking over the examples.  There are only ten functions and it's not
an onerous volume of code.  If you require a top-down introduction, or if
you're getting into deeper functionality like the statistics or using more
than three parallel strands, do read on.

All applications should start by including the header file:

	#include "p9813.h"

This provides definitions of status codes, data types and function
prototypes.  Most functions return a value of type TCstatusCode,
indicating either the successful completion of the function or a numeric
code (defined in the header file) that may provide some help in diagnosing
issues -- for example, some functions will return TC_ERR_VALUE if a
parameter received was outside the expected range of inputs.  It's the
responsibility of the application to respond to and/or report errors to
the user; the library does not terminate program execution in error cases,
and if left unchecked an application may stumble into further trouble.
Most functions return the code TC_OK on successful completion.

For command-line programs, the function TCprintError() provides very basic
error reporting.  Given a TCstatusCode value, it prints a message to
standard output describing the problem:

	TCstatusCode status;

	status = TCsomeFunction(42);
	if(status != TC_OK) {
		TCprintError(status);
		/* Possibly exit or clean up here */
	}

This is obviously of no use to programs with graphical user interfaces.
Such programs will need to provide their own error-reporting mechanism.

To use the LED pixels, it's necessary to first open the communication
channel.  This is done with the function TCopen(), which expects two
parameters: the number of parallel strands, and the number of pixels per
strand (if strands have different lengths, use the maximum length here).
To access one strand of 50 pixels, the syntax would be:

	int status = TCopen(1,50);

It's a very good idea to check the return value of this function, even
with very casual code that otherwise omits most error-checking.  This
first function is the most likely place the library will encounter an
error, for example if the FTDI adapter isn't plugged in.  A very simple
test might resemble this:

	if(status != TC_OK) {
		TCprintError(status);
		return;
	}

The list of possible status codes is defined in p9813.h.  Their names
and most likely causes include:

	TC_OK           : Successful operation; no error.

	TC_ERR_VALUE    : One or more parameters was outside the expected
	                  range.  The number of strands must be between
	                  1 and 8, and the number of pixels must be
	                  greater than zero.

	TC_ERR_MALLOC   : Library could not allocate memory required to
	                  function.  On most modern operating systems this
	                  is exceedingly unlikely to actually occur, and
	                  if it does crop up may simply be that a
	                  parameter is inadvertently way out of range
	                  (e.g. an uninitialized variable results in
	                  trillions of pixels).

	TC_ERR_OPEN     : The library could not open the FTDI USB device.
	                  This has many possible causes: the dreaded "did
	                  you plug it in?", or the device may already be
	                  in use by another program (or another running
	                  instance of your own program), or -- on Mac OS X
	                  and Linux systems -- the "Virtual COM Port"
	                  driver may currently be loaded and interfering
	                  with bitbang access to the device.

	TC_ERR_WRITE    : Device opened, but attempts to write data
	                  failed.  Most often due to the cable being
	                  unplugged while the program is running.

	TC_ERR_MODE     : Device opened, but could not enable asynchronous
	                  bitbang mode.  Could possibly arise with older
	                  or different FTDI devices that might not support
	                  this capability.

	TC_ERR_DIVISOR  : Either of these two status codes indicates that
	TC_ERR_BAUDRATE   the device could be opened and accessed, but the
	                  baud rate setting could not be changed to the
	                  library's requested value.  This is not
	                  necessarily a fatal condition, and applications
	                  might choose to ignore these status codes, or
	                  simply issue a warning.  The library will
	                  continue to operate at the device's default
	                  baud rate.

The TCopen() function will set the initial state of the pixel display to
all "off" or black.

The reverse of TCopen() is TCclose(), which closes the FTDI USB device (if
it was previously successfully opened) and frees any memory previously
allocated by the library.  This function expects no parameters and returns
no status code.  If the LED pixels are currently set to some "on" state,
they are left as-is; the display is not cleared.

Casually-written code can simply exit upon program completion and let the
operating system clean up the mess.  A well-behaved application should use
TCclose(), especially if it may be repeatedly opening and closing the
device.

To change the contents of the display, an array of type TCpixel is created
and loaded with color values, one per LED pixel.  TCpixel is a 32-bit
integer containing a "packed" RGB value, i.e.:

	MSB                          LSB
	00000000RRRRRRRRGGGGGGGGBBBBBBBB

The topmost 8 bits are unused, while the subequent 3 groups of 8 bits
contain red, green and blue color components, respectively, in the range
of 0 (off) to 255 (max brightness).  This specific layout was chosen
because many existing image formats, libraries and operating systems
already provide image data in this configuration.  The convenience
function TCrgb() can be used to specify a color with separate red, green
and blue components without having to combine them manually, e.g.:

	TCpixel pixArray[25];  /* Data for 25 pixels */
	pixArray[0] = TCrgb(0,0,0);
	pixArray[1] = TCrgb(100,0,42);
	...etc...
	pixArray[24] = TCrgb(3,97,255);

Setting an array element does not immediately update the display.  Most
applications will want all pixels synchronized to update simultaneously
in order to create clean and steady animation.  To update the state of
the display, call the TCrefresh() function, passing the address of the
TCpixel array:

	status = TCrefresh(pixArray,NULL,NULL);

This pushes the in-memory image to the LEDs via the FTDI interface.  After
a brief interval (depending how many LEDs are in the strands), the display
will update and program execution will resume.  Usually this will be a
tiny fraction of a second, unless many thousands of LED pixels are in use.

TCrefresh() accepts two additional parameters, which will be described
later.  For now, with a single strand, it's generally sufficient to pass
NULL for both parameters, as is done above.

TCrefresh() returns a TC_OK status code on successful completion, or may
return TC_ERR_WRITE if a write error occurred (usually only if the USB
cable is physically removed).

The features explained to this point should cover most simple recreational
use of the library.  If your needs are straightforward, that may be
sufficient and you can skip ahead to the SAMPLE PROGRAMS section.  The
remaining few functions are more technical in nature.



                            GAMMA CORRECTION

The PWM signals generated internally by the P9813-based LED pixels have
a duty cycle in direct linear proportion to the color values received:

	  0 =   0/255 = 0% duty cycle (always off)
	255 = 255/255 = 100% duty cycle (always on)
	128 = 128/255 = 50.2% duty cycle (roughly half-on)

Our bodies' visual system has a non-linear response to light, being more
sensitive at the low end.  As a result, mid-range values appear noticably
brighter than their roughly 50% duty cycle would suggest.  This can be
compensated for through the process of "gamma correction," which applies
a corresponding inverse non-linear function to pixel values to produce a
perceptually linear range of brightness levels.  The higher the gamma
correction factor, the more the mid-range values are pulled down (0 and
255 will always correspond to 0% and 100% duty cycle).  By default, the
library applies a gamma value of 2.4 to all pixel values.  Your needs or
perception may vary, say if some source imagery already had a different
gamma correction curve applied:

	status = TCsetGammaSimple(1.3);

The single parameter is a positive floating-point number.  A value of 1.0
will disable gamma correction, 2.4 is the library default, and in some
situations you may want higher numbers.  The function will return TC_OK on
success or TC_ERR_VALUE if the given gamma factor is out of range (0.0 or
less).

The red, green and blue elements of the LED pixels are not uniform in
brightness.  Blue, for example, appears brighter than the other colors,
and as a result a fully "white" display may appear blue-tinged.  This
effect is subtle and generally not distracting if the application is
rendering color patterns procedurally.  When displaying existing images
or video from the computer, these differences in color are more
noticeable.  The function TCsetGamma() provides a more comprehensive
set of adjustments.  It accepts nine parameters, three each for red,
green and blue, consisting of the minimum and maximum brightness levels
and a gamma-correction value for each component:

	status = TCsetGamma(0,255,3.2,0,240,3.2,0,220,3.2);

Some applications may have occasion to disable gamma correction entirely,
preferring instead to use "raw" uncorrected color values.  For these
situations, the TCdisableGamma() function is provided, which expects no
parameters and returns no status code:

	TCdisableGamma();

The gamma-correction functions do not automatically refresh the display
when called; they apply only to subsequent TCrefresh() calls.  Generally
these functions are used only once at program start following TCopen().
The TCopen() function will reset gamma to the default 2.4, so if your
program will be performing repeated open and close operations,
TCsetGamma() (or one of the others) should be invoked following each
TCopen();



                         PERFORMANCE STATISTICS

Returning to an earlier subject, the matter of performance statistics was
glossed over in the introduction and in the explanation of the TCrefresh()
function.  The library can provide both throughput rates (bits per second,
frames per second, etc.) and current and charge estimates (milliamps for
image, cumulative mAH used, etc.).  This does not require that pixels even
be attached -- by simply programming the system as if the LEDs were
present, the estimates will be generated, making this feature useful for
planning power supply needs without having to build and measure from a
full-scale rig.  Datasheets are of limited utility in this regard, as they
generally provide only maximum ratings, not actual usage patterns for a
given application such as motion video.

An optional parameter to the TCrefresh() function is a structure that will
be filled with this information; the type is called TCstats.  Before use,
this structure needs to be initialized:

	TCstats      info;
	TCstatusCode status;
	TCpixel      pixArray[1000];

	status = TCinitStats(&info);

	/* ... image is drawn in pixArray[] here ... */

	status = TCrefresh(pixArray,NULL,&info);

The TCstats structure is detailed with comments in p9813.h.  The most
useful elements will likely be bpsAvg and fpsAvg (average serial
throughput and average frames per second), ma (and corresponding maAvg
and maMax) for estimating the current draw (in milliamps) of the LED
pixels in their new configuration (useful in planning power supply
requirements) and possibly mah and mahTotal for estimating the amount of
charge consumed by the prior frame and in total (useful in planning
battery requirements for portable applications).  There are many
additional nitty-gritty details in this structure that can usually be
overlooked, but are provided in case you need to calculate statistics not
provided by the library (for example, perhaps a running average and peak
current over the past 60 seconds only).  The sample programs also
demonstrate the use of the TCstats structure.

The TCstats structure only needs to be initialized once, prior to its
first use.  Repeated calls to TCrefresh() will then keep running totals
in this structure.  This can be reset at any time with another call to
TCinitStats().

For command-line programs, the function TCprintStats() formats and
outputs the contents of this structure to standard output:

	TCprintStats(&info);

As with TCprintError(), graphical applications should generate their
own display from this information and not use this function; it is merely
a convenience for casual programs.

The figures provided by the library are only estimates and should be
padded with a considerable engineering overhead when used for power
supply or battery planning.  They were derived from calibration data
collected with a limited supply of LEDs in an indoor environment, using
a regulated 5V power supply.  There exist small manufacturing variances
among every LED pixel produced, power supply outputs do vary, and there
exist some inaccuracies in the model used to estimate current use.
Your results may vary.  Please don't rely on these numbers for your
safety or well-being; use at your own risk.  The software license
specifically disclaims any sort of warranty.  



                             PIXEL REMAPPING

The wiring between Total Control pixels places certain constraints on
how pixels can be physically positioned in an installation; the furthest
gap between pixels is about 10 centimeters.  For certain layouts such as
a 2D grid, either a zig-zag or a spiral arrangement of pixels is often
used.  This does not always correlate well with the memory layout that
computers typically use when representing images internally.

Let's suppose we have a 12x10 color bitmap image (this would comprise
120 pixels total).  The order of pixels in memory is usually sequential
by row...arranged top-to-bottom, left-to-right.  The array index of each
pixel in memory would resemble this:

          0   1   2   3   4   5   6   7   8   9  10  11
         12  13  14  15  16  17  18  19  20  21  22  23
         24  25  26  27  28  29 ... ... ...
        
                    ... ... ...  90  91  92  93  94  95
         96  97  98  99 100 101 102 103 104 105 106 107
        108 109 110 111 112 113 114 115 116 117 118 119

Notice how at the end of each row, the pixel position winds back to
the left column as the array index increases by one.  This pattern is
consistent with how CRT monitors and video signals normally paint an
image.  But in physical space, that long right-to-left distance exceeds
the maximum pixel spacing that the wiring will allow.  One option is to
cut up the pixel strand and install longer wires, but a much simpler
approach is to install alternating rows in the reverse order: odd rows
are left-to-right, even rows are right-to-left.

My test fixture is similar to this setup, but slightly more peculiar.
It should make a good example.  Having acquired pixel strands at
different times and with varying lengths, I've ended up with two 50 pixel
strands and one 25 pixel strand, for 125 pixels total.  The 12x10 layout
makes for a nice grid, with five extra unused pixels left over.  Because
the vertical 10 pixel measurement divides nicely into 50, the zig-zag is
vertical rather than horizontal:

                      Strand 1 Head
                        50 pixels
                            |
        O---O   O---O   O   O   O---O   O---O   O---O
        |   |   |   |   |   |   |   |   |   |   |   |
        O   O   O   O   O   O   O   O   O   O   O   O
        |   |   |   |   |   |   |   |   |   |   |   |
        O   O   O   O   O   O   O   O   O   O   O   O
        |   |   |   |   |   |   |   |   |   |   |   |
        O   O   O   O   O   O   O   O   O   O   O   O
        |   |   |   |   |   |   |   |   |   |   |   |
        O   O   O   O   O   O   O   O   O   O   O   O
        |   |   |   |   |   |   |   |   |   |   |   |
        O   O   O   O   O   O   O   O   O   O   O   O
        |   |   |   |   |   |   |   |   |   |   |   |
        O   O   O   O   O   O   O   O   O   O   O   O
        |   |   |   |   |   |   |   |   |   |   |   |
        O   O   O   O   O   O   O   O   O   O   O   O
        |   |   |   |   |   |   |   |   |   |   |   |      Five
        O   O   O   O   O   O   O   O   O   O   O   O     unused
        |   |   |   |   |   |   |   |   |   |   |   |     pixels
        O   O---O   O---O   O---O   O---O   O   O   O--O--O--O--O--O
        |                                       |
  Strand 0 Head                           Strand 2 Head
    50 pixels                               25 pixels (20 used)

A zig-zag is not a requirement...it could be any arrangement that covers
all points and meets the 10cm distance limit...a spiral or random walk or
fractal Hilbert curve or whatever...the zig-zag was just easier for my
primitive mammal brain to grasp.  Additionally, all strand heads would
ideally be placed close together for easier wiring to the FTDI chip, but
I chose to place heads and tails in proximity to more easily test
different configurations -- one long strand vs. multiple shorter ones.

Needless to say, this arrangement of pixels is entirely inconsistent
with the representation of an image in memory.  And as installations
increase in complexity and vary in shape, the matter will only become
more pronounced.  The remapping feature will take care of this mess.

Let's consider all pixels in all strands as a single contiguous range,
even if we've wired them up differently.  If the strands are different
lengths, treat them all as being equal to the longest strand...consider
any gap as if there were pixels there.  My demo rig has three strands
of 50, 50 and 25 pixels respectively, but I call TCopen() with the
values 3 and 50 (number of strands, and longest strand length).  Every
pixel then has a singular fixed index in the order of things.  Let me
inflict more ASCII art on you, because I spent a lot of time on it.
Picture every pixel having an index line this:

                      Strand 1 Head
                        50 pixels
                            |
        9---10  29--30  49  50  69--70  89--90 109-110
        |   |   |   |   |   |   |   |   |   |   |   |
        8   11  28  31  48  51  68  71  88  91  O  111
        |   |   |   |   |   |   |   |   |   |   |   |
        7   12  27  32  47  52  67  72  87  92  O  112
        |   |   |   |   |   |   |   |   |   |   |   |
        6   13  26  33  46  53  66  73  86  93  O  113
        |   |   |   |   |   |   |   |   |   |   |   |
        5   14  25  34  45  54  65  74  85  94  O  114
        |   |   |   |   |   |   |   |   |   |   |   |
        4   15  24  35  44  55  64  75  84  95 104 115
        |   |   |   |   |   |   |   |   |   |   |   |
        3   16  23  36  43  56  63  76  83  96 103 116
        |   |   |   |   |   |   |   |   |   |   |   |
        2   17  22  37  42  57  62  77  82  97 102 117
        |   |   |   |   |   |   |   |   |   |   |   |
        1   18  21  38  41  58  61  78  81  98 101 118
        |   |   |   |   |   |   |   |   |   |   |   |
        0   19--20  39--40  59--60  79--80  99 100 119--120-...-149
        |                                       |
  Strand 0 Head                           Strand 2 Head
    50 pixels                               25 pixels (20 used)

The goal now is to create an array of integers that connects each element
in this sequence back to a TCpixel index in the original image.  Which,
as you'll recall, resembles:

          0   1   2   3   4   5   6   7   8   9  10  11
         12  13  14  15  16  17  18  19  20  21  22  23
         24  25  26  27  28  29 ... ... ...
        
                    ... ... ...  90  91  92  93  94  95
         96  97  98  99 100 101 102 103 104 105 106 107
        108 109 110 111 112 113 114 115 116 117 118 119

So...the first element in this array, being the bottom-left pixel in our
fixture, maps to pixel 108 in the source image.  Second pixel then maps
to 96, and so forth.  Do you see it?  Strand 2, starting at index 50,
then points to source pixels 5, 17, 29 and so forth.  The arrangement
just happens to end with 119 both as the source and destination indices.

This could tediously be programmed as:

	int remap[150];

	remap[0] = 108;
	remap[1] = 96;
	remap[2] = 84
	...
	remap[50] = 5;
	remap[51] = 17;
	remap[52] = 29;
	...
	remap[118] = 95;
	remap[118] = 107;
	remap[119] = 119;

A preferable approach would be to initialize the remap array using a
combination of loops.  Or, if the layout is truly random, specify the
full array contents when declared (much abbreviated here):

	int remap[150] =
	{
		108, 96, 84, ... 28, 16,  4,
		  5, 17, 29, ... 93,105,117,
		118,106, 94, ... 95,107,119,                     X 120
		TC_PIXEL_UNUSED, ... TC_PIXEL_UNUSED,            X   5
		TC_PIXEL_DISCONNECTED, ... TC_PIXEL_DISCONNECTED X  30
	};

Despite the difference in strand lengths, and that not all pixels are
used, the array MUST be declared with 150 elements here because of the
initial funcion call to TCopen(3,50);  120 of those elements are
assigned pixel indices as expected.  The next five have been assigned
the value TC_PIXEL UNUSED to indicate that these pixels are physically
present on the strand, but aren't used in the display.  I paid a lot
for these and didn't want to chop them up!  So they will always be
there in an "off" state.  For the last 30 elements (needed to pad out
the array to the requisite 150), the value TC_PIXEL_DISCONNECTED tells
the current-estimating code to leave these pixels out of its
calculations, because they do not physically exist and do not contribute
to power use.  The TC_PIXEL_UNUSED pixels, despite not being used, do
draw a tiny amount of power even in their "off" state and should be
factored into the calculations.  (If it's not clear,
TC_PIXEL_DISCONNECTED will NOT save power if assigned to pixels that are
actually present.  This does not, can not and never will power down
unused pixels, it merely marks gaps in the pixel sequence to keep the
power calculations in line.)

This may all seem inordinately tedious, but starts to make sense when
dealing with video or with persistence-of-vision displays, where there's
frame after frame after frame of identically-formatted imagery passing
through.  Once the remapping array is set up, nothing more needs to be
done to the source images -- they can be left intact.

(A clever program with a webcam could help automate this entire process,
 but that's a project for another day.)

The remap array is passed to TCrefresh() as the second parameter, e.g.:

	TCpixel pixArray[120];
	int     remap[150];
	TCstats info;

	/* Initialization goes here */

	TCrefresh(pixArray,remap,&info);

An extreme case of pixel remapping can be seen in the included "rgb" demo
program.  This declares just one TCpixel of image data, but maps every
pixel on every strand to this same value.



                         RECONFIGURING I/O PINS

The p9813 library is configured by default to work with up to three LED
strands, using what seemed the most convenient arrangement of available
pins on the standard FTDI USB-to-serial adapter cable (or equivalent
adapters available from many other sources).  Driving more strands in
parallel requires an adapter with more available pins, which are usually
marketed as an "FTDI breakout board" or similar.  Accessing these
additional pins, or changing the relationship of strands to pins even
when using the standard FTDI cable, can be handled through the
TCsetStrandPin() function:

	status = TCsetStrandPin(4,TC_FTDI_DSR);

This expects two parameters: the first is the strand number for which
we'll be assigning a new pin (indexed starting from 0), and the second
is one of the pin identities defined in p9813.h.  If reassigning multiple
strands/pins, a separate call must be made for each.  The return status
code will be TC_OK on successful assignment, or TC_ERR_VALUE if the
strand or pin numbers are out of range.

TCsetStrandPin() ideally should be used before TCopen(), so that the
initial screen-clearing operation will work -- it requires knowledge
of the clock and data lines in use.

If you have occasion to send the same strand data to more than one pin,
pin names can be merged with a bitwise OR:

	status = TCsetStrandPin(2,TC_FTDI_DTR | TC_FTDI_RTS);

The library does this by default with strand 2.  Different FTDI adapter
cables provide a different function on the last pin: FTDI-branded cables
use RTS, while third-party cables generally use DTR.  Toggling both pins
allows the same code to work with either type of cable.  If using a full
breakout board, strand 2 can be reassigned to a single pin of your choice
using TCsetStrandPin().

The strand number will normally range from 0 to 2 if using an FTDI cable
(max of three strands), or 0 to 6 if using a full breakout board (max of
seven strands).  In the latter case, strands 3-6 are not assigned to
anything by default and must be explicitly configured in software.
In either case, passing "7" here normally has a special meaning, where
it's used to set the pin used for the serial clock signal.  However,
there is an exception where an eighth strand can be used...see the
section MORE SPEED below.

EXTRA BONUS CAKE: you may have a perfectly usable FTDI adapter in your
midst and not even know it.  Some Arduino boards -- those that
incorporate an FTDI chip into their design -- have a row of solder pads
or sometimes even a pre-installed header for accessing those FTDI pins
that the microcontroller itself does not use.  This is at position X3
on the Arduino Diecimila and Duemilanove, or J3 on the third-party
chipKIT Uno32.  Pin one on this header is already tied to CTS on the
FTDI chip, which is the library's default serial clock pin.  The other
three pins on this header can easily be reassigned as the data lines
for strands 0, 1 and 2:

	TCsetStrandPin(0,TC_FTDI_DSR);
	TCsetStrandPin(1,TC_FTDI_DCD);
	TCsetStrandPin(2,TC_FTDI_RI);

Remember also to connect the ground line to the strands.  There is no
ground pin on this header, so you'll have to use one of the ground
tiepoints elsewhere on the Arduino board.

Unfortunately we can't bitbang and communicate with the Arduino at the
same time.  But if the sketch can run autonomously and doesn't need any
serial input, it's possible to have the microcontroller control its own
LED strands while the PC controls others.



                               MORE SPEED!

The most demanding applications -- those requiring not just hundreds
but THOUSANDS of pixels -- will benefit from a particular hardware
configuration step.  This requires a full FTDI breakout board; the simple
6-pin adapter need not apply.  It also requires access to a Windows PC
during the configuration process, but the chip will then work across all
the different platforms after that.

Normally one pin on the FTDI chip is used by the library for bitbanging
a serial clock to synchronize the data transfer.  But there's a special
mode, not enabled on the chip by default, that handles this automatically
on a separate pin.  This frees up the former clock line to now be used as
a data line -- enabling an eighth strand of pixels -- and doubles the
overall data rate now that bandwidth isn't being spent manually
generating a clock signal.  In combination, peak throughput suddenly
jumps from about 105,000 pixels/sec to 240,000 pixels/sec.  Hell yeah.

To configure the chip, first download the FT_Prog software from the FTDI
utilities web page:

	http://www.ftdichip.com/Support/Utilities.htm

This needs to be installed on a Windows PC (or a compatible environment
such as VirtualBox).  After installing, connect the FTDI device to a USB
port, run the FT_Prog utility, and perform the following sequence:

	- From the "Devices" menu, select "Scan and parse".  In a moment
	  you should see a screen full of technical minutiae.  If you
	  foresee a need to restore the chip to its factory state later,
	  select "Save As Template" from the File menu to save this
	  original chip configuration for posterity.

	- In the left column, "Device Tree", expand the item labeled
	  "-> Hardware Specific" (click the + symbol), then expand the
	  sub-item labeled "-> IO Controls".

	- In the top right pane (Properties and Values), for item C1,
	  select "BitBang WRn" from the pop-up menu.

	- From the "Devices" menu, select "Program."  In the dialog box
	  that ensues, click the "Program" button.

	- You can now close FT_Prog, the chip is primed for high-speed
	  use.

The physical wiring to the chip is now different.  Instead of the CTS
pin as the serial clock, use the pin labeled either RXLED or CBUS1.
Unlike the software bitbang approach, this function cannot be remapped
to another pin through software; it can only be changed using FT_Prog.
CTS is now available as another strand data pin using TCsetStrandPin().

Some minor adjustments are then required of one's software:

In the call to TCopen(), the number of strands should be set to 8, or,
if using fewer than 8 strands but the high-speed mode is still desired,
add TC_CBUS_CLOCK to the number of strands, e.g.:

	status = TCopen(3 + TC_CBUS_CLOCK,100);

Secondly, if using more than the default three strands (or if a different
order or combination of pins is desired), TCsetStrandPin() should be used
to assign FTDI pins to strand data lines, e.g.:

	TCsetStrandPin(3,TC_FTDI_CTS);



                             SAMPLE PROGRAMS

A few command-line utility programs are included to test the library and
for educational purposes.  Each has different functions, though all accept
two parameters in common: -s (number of strands) and -p (number of pixels
per strand).  For example:

	./demo -s 3 -p 50

These programs all assume the default pin/strand configuration used by
the library.  If a different configuration is needed, the program(s) will
need to be modified.

rgb: displays a single color on all pixels on all strands.  Red, green and
blue color components are set with the -r, -g and -b commands, e.g.:

	./rgb -r 20 -g 0 -b 255

This also works with the -s and -p commands previously described.  If no
color is specified, all pixels will be turned off.  An additional option,
-c, keeps the program running continuously to display charge (mAH) over
time, otherwise the program exits immediately, leaving the display as-is.
Gamma correction is not used by this program; the numbers are "raw"
uncorrected pixel colors.  Its primary use is for measurement with a
multimeter to set up calibration values defined in calibration.h.

gamma: tests different gamma-correction values by displaying a gray
ramp across all pixels (repeated across all strands).  In addition to the
standard -s and -p commands, a -g command is used for specifying a gamma
value for the display, e.g.:

	./gamma -g 2.2

The default gamma value with this program is 1.0; gamma correction is
disabled unless a value is specified.

random: displays a random RGB color on each pixel.  No commands other
than the usual -s and -p.

demo: displays colorful rainbow patterns on all pixels, along with
ongoing statistics.  No commands other than -s and -p.



                            PROCESSING LIBRARY

The "processing" subdirectory contains files for building a p9813 library
for the Processing language/environment (www.processing.org).  This isn't
a drop-in library for Processing; the p9813 C library needs to have
previously been built and installed, and the Processing library then needs
to be built using "make" and "make install".  It's advisable to first
check the Makefile to validate the locations of Java- and Processing-
related files and directories, to see whether they correspond to your own
system configuration (especially with Cygwin!).

All of the C library functionality is present, but the calling syntax is
somewhat different; method overloading reduces the number of functions.
See the "Blink" example sketch for a very simple use case.

Because the Processing library guidelines recommend against prefixing
libraries with the letter "p," the library is instead called TotalControl
in reference to the Total Control Lighting LED pixels.

To use the library from your own code, import the TotalControl library
and create a TotalControl object:

	import TotalControl.*;

	TotalControl tc;

Method names are then appended to the object name, e.g.:

	int status = tc.open(1,25);

Several methods map directly to corresponding C functions of the original
library.  For example, if the TotalControl object is called "tc," these
methods then substitute for the C equivalents, with the same parameters:

	C library           Processing library
	----------------    ------------------
	TCopen()            tc.open()
	TCclose()           tc.close()
	TCsetStrandPin()    tc.setStrandPin()
	TCprintError()      tc.printError()

The remaining methods are invoked somewhat differently:

The tc.setGamma() method replaces the functions TCsetGamma(),
TCsetGammaSimple() and TCdisableGamma().  The number of parameters will
determine the behavior: the functions named correspond to nine, one and
zero parameters, respectively.

tc.refresh() replaces TCrefresh(), with slightly different parameters.
If no parameters are given, this corresponds to TCrefresh(NULL,NULL,NULL).
If one parameter, this is presumed to be the pixel data, i.e.
TCrefresh(data,NULL,NULL).  And if two parameters, these are presumed the
pixel data and remapping table, e.g. TCrefresh(data,remap,NULL).  There is
no facility to pass a TCstats structure in the Processing version of the
library.  Instead, a single instance of the structure is handled internally
by the library, and is always automatically passed to the refresh function.
tc.initStats() and tc.printStats() clear and print the contents of this
structure, but at present there's no facility for accessing individual
members in Processing.

If you start working with this library and find that the device won't open,
remember that the FTDI USB-to-serial driver needs to be disabled on Mac and
Linux systems.  "make unload" in either the C or Processing makefiles will
handle this, and "make load" will restore serial functionality.



                               KNOWN ISSUES

The bits-per-second figures in the TCstats structure are not accurate when
using Cygwin; either that platform doesn't provide microsecond timing, or
it may require use of a different time function.



                               FUTURE NOTES

Might consider extending statistics structure to include per-strand
current estimates along with the system total.  This could help with
more detailed power supply planning for multi-strand installations.
The overall usage estimates are probably sufficient for most situations,
making this a very low priority right now.

It's likely possible to use other CBUS pins along with a shift register
to add a "soft start" feature, powering up strands in sequence with a
delay in order to avoid a massive power onrush when initially switching
on a larger system.



                                 LICENSE

This Program is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the Free
Software Foundation, either version 3 of the License, or (at your option)
any later version.

This Program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License along
with this Program.  If not, see <http://www.gnu.org/licenses/>.

Additional permission under GNU GPL version 3 section 7

If you modify this Program, or any covered work, by linking or combining
it with libftd2xx (or a modified version of that library), containing
parts covered by the license terms of Future Technology Devices
International Limited, the licensors of this Program grant you additional
permission to convey the resulting work.
