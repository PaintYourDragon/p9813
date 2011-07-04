EXECS      = rgb gamma random demo
LIB_LED    = libp9813.a
CC         = gcc
LDFLAGS    = -lftd2xx

# Platform-specific rules
ifeq ($(shell uname -s),Darwin)
  # Mac OS X
  CFLAGS   = -fast -fomit-frame-pointer
endif
ifeq ($(shell uname -s),Linux)
  CFLAGS   = -O3 -fomit-frame-pointer
  LDFLAGS += -lpthread -lrt -lm
endif
ifeq ($(findstring CYGWIN,$(shell uname -s)),CYGWIN)
  CFLAGS   = -O3 -fomit-frame-pointer -DCYGWIN
  LDFLAGS  = /usr/local/lib/ftd2xx.lib
endif

all: $(EXECS)

rgb: rgb.c $(LIB_LED)
	$(CC) $(CFLAGS) rgb.c $(LIB_LED) $(LDFLAGS) -o rgb

gamma: gamma.c $(LIB_LED)
	$(CC) $(CFLAGS) gamma.c $(LIB_LED) $(LDFLAGS) -o gamma

random: random.c $(LIB_LED)
	$(CC) $(CFLAGS) random.c $(LIB_LED) $(LDFLAGS) -o random

demo: demo.c $(LIB_LED)
	$(CC) $(CFLAGS) demo.c $(LIB_LED) $(LDFLAGS) -o demo

$(LIB_LED): p9813.o
	ar -r $(LIB_LED) p9813.o

p9813.o: p9813.c p9813.h calibration.h
	$(CC) $(CFLAGS) p9813.c -c

install:
	sudo cp p9813.h    /usr/local/include/
	sudo cp $(LIB_LED) /usr/local/lib/

clean:
	rm -f $(EXECS) *.o *.a core

# On Mac and Linux, the Virtual COM Port driver must be unloaded in
# order to use bitbang mode.  Use "make unload" to do this, but ALWAYS
# remember to "make load" when done, in order to restore normal serial
# port functionality!

ifeq ($(shell uname -s),Darwin)
# Mac driver stuff
  DRIVER    = com.FTDI.driver.FTDIUSBSerialDriver
  KEXTFLAGS = -b
  unload:
	sudo kextunload $(KEXTFLAGS) $(DRIVER)
  load:
	sudo kextload $(KEXTFLAGS) $(DRIVER)
else
ifeq ($(shell uname -s),Linux)
# Linux driver stuff - Careful! This is persistent between reboots!
  unload:
	sudo modprobe -r ftdi_sio
	sudo modprobe -r usbserial
  load:
	sudo modprobe ftdi_sio
	sudo modprobe usbserial
endif
endif
