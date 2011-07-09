/****************************************************************************
 File        : TotalControl.java

 Description : Processing library for "Total Control Lighting" RGB LED
               pixels from CoolNeon.com using an FTDI USB-to-serial cable
               or breakout board.  Requires the p9813 C library to be
               previously installed in /usr/local -- see documentation
               accompanying that library for an explanation of its use.

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

package TotalControl;

public class TotalControl {
	static 
	{    	
		System.loadLibrary("TotalControl");
		TCinitStats();
	}

	private static native int TCopen(int nStrands,int pixelsPerStrand);

	public static int open(int nStrands,int pixelsPerStrand) 
	{
		return TCopen(nStrands,pixelsPerStrand); 
	}

	private static native int TCsetGamma0();
	private static native int TCsetGamma1(float g);
	private static native int TCsetGamma9(
	  int rMin,int rMax,float rGamma,
	  int gMin,int gMax,float gGamma,
	  int bMin,int bMax,float bGamma);

	public static int setGamma()
	{
		return TCsetGamma0();
	}

	public static int setGamma(float g)
	{
		return TCsetGamma1(g);
	}

	public static int setGamma(
	  int rMin,int rMax,float rGamma,
	  int gMin,int gMax,float gGamma,
	  int bMin,int bMax,float bGamma)
	{
		return TCsetGamma9(
		  rMin,rMax,rGamma,
		  gMin,gMax,gGamma,
		  bMin,bMax,bGamma);
	}

	private static native void TCinitStats();

	public static void initStats()
	{
		TCinitStats();
	}

	private static native int TCrefresh0();
	private static native int TCrefresh1(int[] pixels);
	private static native int TCrefresh2(int[] pixels,int[] remap);

	public static int refresh()
	{
		return TCrefresh0();
	}

	public static int refresh(int[] pixels)
	{
		return TCrefresh1(pixels);
	}

	public static int refresh(int[] pixels,int[] remap)
	{
		return TCrefresh2(pixels,remap);
	}

	private static native int TCsetStrandPin(int strand,short bit);

	public static int setStrandPin(int strand,short bit)
	{
		return TCsetStrandPin(strand,bit);
	}

	private static native void TCclose();

	public static void close()
	{
		TCclose();
	}

	private static native void TCprintStats();

	public static void printStats()
	{
		TCprintStats();
	}

	private static native void TCprintError(int status);

	public static void printError(int status)
	{
		TCprintError(status);
	}
}
