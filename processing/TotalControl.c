/****************************************************************************
 File        : TotalControl.c

 Description : JNI (Java Native Interface) wrapper for using the P9813
               library in Processing.  This is purely functional and
               uncommented -- see the original code (p9813.c) for an
               explanation of its use.

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

#include <jni.h>
#include "TotalControl_TotalControl.h"
#include "p9813.h"

static TCstats stats;  /* JNI version uses a single common stats structure */

JNIEXPORT jint JNICALL Java_TotalControl_TotalControl_TCopen(
  JNIEnv *env,
  jclass  this,
  jint    nStrands,
  jint    pixelsPerStrand) 
{
	return TCopen(nStrands,pixelsPerStrand);
}

JNIEXPORT jint JNICALL Java_TotalControl_TotalControl_TCsetGamma0(
  JNIEnv *env,
  jclass  this)
{
	TCdisableGamma();
	return TC_OK;
}

JNIEXPORT jint JNICALL Java_TotalControl_TotalControl_TCsetGamma1(
  JNIEnv *env,
  jclass  this,
  jfloat  g)
{
	return TCsetGammaSimple(g);
}

JNIEXPORT jint JNICALL Java_TotalControl_TotalControl_TCsetGamma9(
  JNIEnv *env,
  jclass  this,
  jint    rMin,
  jint    rMax,
  jfloat  rGamma,
  jint    gMin,
  jint    gMax,
  jfloat  gGamma,
  jint    bMin,
  jint    bMax,
  jfloat  bGamma)
{
	return TCsetGamma(
	  rMin,rMax,rGamma,
	  gMin,gMax,gGamma,
	  bMin,bMax,bGamma);
}

JNIEXPORT void JNICALL Java_TotalControl_TotalControl_TCinitStats(
  JNIEnv *env,
  jclass  this)
{
	TCinitStats(&stats);
}

JNIEXPORT jint JNICALL Java_TotalControl_TotalControl_TCrefresh0(
  JNIEnv *env,
  jclass  this)
{
	return TCrefresh(NULL,NULL,&stats);
}

JNIEXPORT jint JNICALL Java_TotalControl_TotalControl_TCrefresh1(
  JNIEnv   *env,
  jclass    this,
  jintArray pixels)
{
	jint *pixPtr,status = 0;

	if((pixPtr = (*env)->GetIntArrayElements(env,pixels,NULL)))
	{
		status = TCrefresh((TCpixel *)pixPtr,NULL,&stats);
		(*env)->ReleaseIntArrayElements(env,pixels,pixPtr,0);
	}

	return status;
}

JNIEXPORT jint JNICALL Java_TotalControl_TotalControl_TCrefresh2(
  JNIEnv   *env,
  jclass    this,
  jintArray pixels,
  jintArray remap)
{
	jint *pixPtr,*remapPtr,status = 0;

	if((pixPtr = (*env)->GetIntArrayElements(env,pixels,NULL)))
	{
		if((remapPtr = (*env)->GetIntArrayElements(env,remap,NULL)))
		{
			status = TCrefresh((TCpixel *)pixPtr,
			  (int *)remapPtr,&stats);
			(*env)->ReleaseIntArrayElements(env,remap,remapPtr,0);
		}
		(*env)->ReleaseIntArrayElements(env,pixels,pixPtr,0);
	}

	return status;
}

JNIEXPORT jint JNICALL Java_TotalControl_TotalControl_TCsetStrandPin(
  JNIEnv *env,
  jclass  this,
  jint    strand,
  jshort  bit)
{
	return TCsetStrandPin(strand,bit);
}

JNIEXPORT void JNICALL Java_TotalControl_TotalControl_TCclose(
  JNIEnv *env,
  jclass  this)
{
	TCclose();
}

JNIEXPORT void JNICALL Java_TotalControl_TotalControl_TCprintStats(
  JNIEnv *env,
  jclass  this)
{
	TCprintStats(&stats);
	fflush(stdout);
}

JNIEXPORT void JNICALL Java_TotalControl_TotalControl_TCprintError(
  JNIEnv *env,
  jclass  this,
  jint    status)
{
	TCprintError(status);
	fflush(stdout);
}
