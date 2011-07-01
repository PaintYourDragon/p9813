/****************************************************************************
 File        : calibration.h

 Description : Defines constants used by the p9813 library in estimating
               LED current consumption, which may be helpful in determining
               power supply and battery capacity requirements.  A single 25
               pixel strand of Total Control Lighting 8mm "bullet" LEDs was
               fed a regulated +5.0 Volts, then current readings in various
               modes were taken with a multimeter.  If working with a
               slightly different voltage or with different LED types, use
               the included "test" program to set up the same lighting
               scenarios, take current measurements (in milliamps) and
               replace those values in the list below.  The C code should
               not require any changes.

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

#ifndef _CALIBRATION_H_
#define _CALIBRATION_H_

/* Baseline current reading (in milliamps) with all LEDs off;
   this establishes the power used by the driver chips alone:  */
#define CAL_N_PIXELS     25  /* Number of LED pixels in test   */
#define CAL_CURRENT_OFF  15  /* Current (mA) with all LEDs off */

/* "Raw" current readings (in milliamps) from the meter,
   not yet factoring out the baseline usage:                   */
#define CAL_RAW_R       510  /* All pixels 100% red            */
#define CAL_RAW_G       491  /* All pixels 100% green          */
#define CAL_RAW_B       491  /* All pixels 100% blue           */
#define CAL_RAW_RG      956  /* All pixels 100% red+green      */
#define CAL_RAW_GB      929  /* All pixels 100% green+blue     */
#define CAL_RAW_RB      957  /* All pixels 100% red+blue       */
#define CAL_RAW_RGB    1322  /* All pixels 100% red+green+blue */

/* ----- Shouldn't need to edit anything below this line ----- */

/* Subtract the baseline usage to isolate actual LED current:  */
#define CAL_CURRENT_R  (CAL_RAW_R  - CAL_CURRENT_OFF)
#define CAL_CURRENT_G  (CAL_RAW_G  - CAL_CURRENT_OFF)
#define CAL_CURRENT_B  (CAL_RAW_B  - CAL_CURRENT_OFF)
#define CAL_CURRENT_RG (CAL_RAW_RG - CAL_CURRENT_OFF)
#define CAL_CURRENT_GB (CAL_RAW_GB - CAL_CURRENT_OFF)
#define CAL_CURRENT_RB (CAL_RAW_RB - CAL_CURRENT_OFF)

/* Current consumption of the LEDs scales very near linearly
   with the PWM duty cycle -- close enough that it can
   reasonably be left out of the model.  However...

   "Combination" currents when measured don't equal the sum of
   the component currents.  For example, a pixel with either
   red or green active will match the above readings, but a
   pixel with both red AND green active will draw slightly
   less current than the prior two figures combined.  This
   curious parasitic phenomenon appears in all combinations
   (R+G, G+B, R+B, R+G+B) and is compounded in the R+G+B case.
   The model takes this into account...when using the "test"
   program, the estimated current figure is typically within
   1% of an actual metered result.  However however...

   Using the "random" program, you may sometimes get meter
   readings that are +/-3% off the estimate.  Part of this is
   simply because the combination model isn't perfect.  But
   it's also because the estimates are derived from averages
   of the whole LED strand, but manufacturing tolerances are
   such that each pixel is in reality slightly different, and
   this will be most apparent with certain lighting
   combintations.  Given more frames over more time, the
   overall measured average should level out closer to the
   software's estimate.                                        */

#define CAL_COMBO_RG \
  ((double)CAL_CURRENT_RG / (double)(CAL_CURRENT_R + CAL_CURRENT_G))
#define CAL_COMBO_GB \
  ((double)CAL_CURRENT_GB / (double)(CAL_CURRENT_G + CAL_CURRENT_B))
#define CAL_COMBO_RB \
  ((double)CAL_CURRENT_RB / (double)(CAL_CURRENT_R + CAL_CURRENT_B))

#endif  /* _CALIBRATION_H_ */
