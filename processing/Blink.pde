/**
 * Blink
 * 
 * Simple "blinkenlights" program for the TotalControl (p9813) library.
 * Flashes one random LED at a time.
 */

import TotalControl.*;

TotalControl tc;
int          nStrands        = 1,
             pixelsPerStrand = 25;
int[]        p               = new int[nStrands * pixelsPerStrand];
/* Could also use PImage and loadPixels()/updatePixels()...many options! */

void setup() 
{
  int status = tc.open(nStrands,pixelsPerStrand);
  if(status != 0) {
    tc.printError(status);
    exit();
  }
}

void draw()
{
  int x = (int)random(nStrands * pixelsPerStrand);
  p[x]  = 0x00ffffff;
  tc.refresh(p);
  p[x]  = 0;
}

void exit()
{
  tc.close();
}

