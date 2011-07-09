CC = gcc

# Platform-specific rules
ifeq ($(shell uname -s),Darwin)
  # Mac OS X
  CFLAGS     = -m32 -fast -fomit-frame-pointer
  LDFLAGS    = -m32 -framework JavaVM -dynamiclib
  CLASSPATH  = /Applications/Processing.app/Contents/Resources/Java/core.jar
  HEADERPATH = /System/Library/Frameworks/JavaVM.framework/Headers
  DESTPATH   = ~/Documents/Processing/Libraries/TotalControl
  JNILIB     = libTotalControl.jnilib
endif
ifeq ($(shell uname -s),Linux)
  CFLAGS     = -O3 -fomit-frame-pointer
  LDFLAGS    = -lm -shared
  JDK        = /usr/lib/jvm/java-6-sun
  CLASSPATH  = $(JDK)/lib/visualvm/platform/core/core.jar
  HEADERPATH = $(JDK)/include -I$(JDK)/include/linux
  DESTPATH   = ~/sketchbook/libraries/TotalControl
  JNILIB     = libTotalControl.so

endif
ifeq ($(findstring CYGWIN,$(shell uname -s)),CYGWIN)
  CFLAGS     = -O3 -fomit-frame-pointer -DCYGWIN -I/usr/local/include -mno-cygwin
  LDFLAGS    = -shared -Wl,--add-stdcall-alias -Wl,-L/usr/local/lib -mno-cygwin
  JDK        = /cygdrive/c/Program\ Files/Java/jdk1.6.0_26
  CLASSPATH  = $(JDK)/lib/visualvm/platform/core/core.jar
  HEADERPATH = $(JDK)/include -I$(JDK)/include/win32
  DESTPATH   = /cygdrive/c/Documents\ and\ Settings/Phillip\ Burgess/My\ Documents/Processing/libraries/TotalControl
  JNILIB     = TotalControl.dll

endif

all: TotalControl.jar $(JNILIB)

TotalControl.jar: TotalControl.java
	javac -d . -classpath $(CLASSPATH) TotalControl.java
	jar -fc TotalControl.jar TotalControl

TotalControl_TotalControl.h: TotalControl.jar
	javah TotalControl.TotalControl

$(JNILIB): TotalControl.c TotalControl_TotalControl.h
	$(CC) $(CFLAGS) -I$(HEADERPATH) TotalControl.c -c
	$(CC) $(LDFLAGS) TotalControl.o -lp9813 -lftd2xx -o $(JNILIB)

install:
	mkdir -p $(DESTPATH)/library
	mkdir -p $(DESTPATH)/examples/Blink
	cp TotalControl.jar $(DESTPATH)/library
	cp $(JNILIB) $(DESTPATH)/library
	cp export.txt $(DESTPATH)/library
	cp Blink.pde $(DESTPATH)/examples/Blink

clean:
	rm -rf *.o TotalControl TotalControl.jar TotalControl_TotalControl.h $(JNILIB)

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
# Linux driver stuff
  unload:
	sudo modprobe -r ftdi_sio
	sudo modprobe -r usbserial
  load:
	sudo modprobe ftdi_sio
	sudo modprobe usbserial
endif
endif
