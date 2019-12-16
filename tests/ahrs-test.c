/* ahrs-test.c
 *
 * Copyright 2019 Screen LLC, Andrei Fadeev <andrei@webcontrol.ru>
 *
 * This file is part of HyScanMath.
 *
 * HyScanMath is dual-licensed: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * HyScanMath is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Alternatively, you can license this code under a commercial license.
 * Contact the Screen LLC in this case - <info@screen-co.ru>.
 */

/* HyScanMath имеет двойную лицензию.
 *
 * Во-первых, вы можете распространять HyScanMath на условиях Стандартной
 * Общественной Лицензии GNU версии 3, либо по любой более поздней версии
 * лицензии (по вашему выбору). Полные положения лицензии GNU приведены в
 * <http://www.gnu.org/licenses/>.
 *
 * Во-вторых, этот программный код можно использовать по коммерческой
 * лицензии. Для этого свяжитесь с ООО Экран - <info@screen-co.ru>.
 */

#include <hyscan-ahrs-mahony.h>
#include <math.h>

#define        TEST_TIME       1000.0
#define        SAMPLE_RATE     100.0
#define        ANGLE_STEP      (G_PI / (TEST_TIME * SAMPLE_RATE))
#define        MAX_ERROR       (1e-2)

int
main (int    argc,
      char **argv)
{
  HyScanAHRS *ahrs;
  HyScanAHRSMahony *mahony;

  HyScanAHRSAngles angles;
  gfloat ax, ay, az;
  gfloat mx, my, mz;
  gfloat roll;
  gfloat pitch;
  gfloat heading;

  mahony = hyscan_ahrs_mahony_new (SAMPLE_RATE);
  hyscan_ahrs_mahony_set_gains (mahony, 5.0, 0.001);
  ahrs = HYSCAN_AHRS (mahony);

  g_print ("Checking heading...\n");

  /* Разворачиваем датчик в положение по курсу -180.0 градусов. */
  heading = -G_PI;
  do
    {
      ax = 0.0;
      ay = 0.0;
      az = 1.0;

      mx = cos (heading);
      my = -sin (heading);
      mz = 0.0;

      hyscan_ahrs_update (ahrs, 0.0, 0.0, 0.0, ax, ay, az, mx, my, mz);
      angles = hyscan_ahrs_get_angles (ahrs);
    }
  while (fabs (heading - angles.heading) > MAX_ERROR / 10.0);

  /* Проворачиваем датчик по курсу на 360.0 градусов. */
  while (heading <= G_PI)
    {
      ax = 0.0;
      ay = 0.0;
      az = 1.0;

      mx = cos (heading);
      my = -sin (heading);
      mz = 0.0;

      hyscan_ahrs_update (ahrs, 0.0, 0.0, 0.0, ax, ay, az, mx, my, mz);
      angles = hyscan_ahrs_get_angles (ahrs);

      if (fabs (heading - angles.heading) > MAX_ERROR)
        g_error ("heading error: input %f, output %f", heading, angles.heading);

      heading += ANGLE_STEP;
    }

  hyscan_ahrs_reset (ahrs);

  g_print ("Checking roll...\n");

  /* Разворачиваем датчик в положение по крену -45.0 градусов. */
  roll = -G_PI / 4.0;
  do
    {
      ax = 0.0;
      ay = -sin (roll);
      az = cos (roll);

      mx = 1.0;
      my = 0.0;
      mz = 0.0;

      hyscan_ahrs_update (ahrs, 0.0, 0.0, 0.0, ax, ay, az, mx, my, mz);
      angles = hyscan_ahrs_get_angles (ahrs);
    }
  while (fabs (roll - angles.roll) > MAX_ERROR / 10.0);

  /* Поворачиваем датчик по крену до +45.0 градусов. */
  while (roll <= (G_PI / 4.0))
    {
      ax = 0.0;
      ay = -sin (roll);
      az = cos (roll);

      mx = 1.0;
      my = 0.0;
      mz = 0.0;

      hyscan_ahrs_update (ahrs, 0.0, 0.0, 0.0, ax, ay, az, mx, my, mz);
      angles = hyscan_ahrs_get_angles (ahrs);

      if (fabs (roll - angles.roll) > MAX_ERROR)
        g_error ("roll error: input %f, output %f", roll, angles.roll);

      roll += ANGLE_STEP;
    }

  hyscan_ahrs_reset (ahrs);

  g_print ("Checking pitch...\n");

  /* Разворачиваем датчик в положение по дифференту -45.0 градусов. */
  pitch = -G_PI / 4.0;
  do
    {
      ax = sin (pitch);
      ay = 0.0;
      az = cos (pitch);

      mx = cos (pitch);
      my = 0.0;
      mz = sin (pitch);

      hyscan_ahrs_update (ahrs, 0.0, 0.0, 0.0, ax, ay, az, mx, my, mz);
      angles = hyscan_ahrs_get_angles (ahrs);
    }
  while (fabs (pitch - angles.pitch) > MAX_ERROR);

  /* Поворачиваем датчик по дифференту до +45.0 градусов. */
  while (pitch <= (G_PI / 4.0))
    {
      ax = sin (pitch);
      ay = 0.0;
      az = cos (pitch);

      mx = cos (pitch);
      my = 0.0;
      mz = sin (pitch);

      hyscan_ahrs_update (ahrs, 0.0, 0.0, 0.0, ax, ay, az, mx, my, mz);
      angles = hyscan_ahrs_get_angles (ahrs);

      if (fabs (pitch - angles.pitch) > MAX_ERROR)
        g_error ("pitch error: input %f, output %f", pitch, angles.pitch);

      pitch += ANGLE_STEP;
    }

  g_object_unref (mahony);

  g_print ("All done.\n");

  return 0;
}
