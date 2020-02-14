/* hyscan-stats.c
 *
 * Copyright 2020 Screen LLC, Alexey Sakhnov <alexsakhnov@gmail.com>
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

#include "hyscan-stats.h"
#include <math.h>

static gdouble    hyscan_stats_fit_angle     (gdouble     angle);

static gdouble
hyscan_stats_fit_angle (gdouble angle)
{
  if (angle < 0)
    angle += 360.0;

  if (angle >= 360)
    angle -= 360;

  return angle;
}

/**
 * hyscan_stats_avg_circular:
 * @values:
 * @n_values:
 *
 * Returns: круговое среднее
 */
gdouble
hyscan_stats_avg_circular (const gdouble *values,
                           gsize          n_values)
{
  gsize i;
  gdouble sum_sin = 0.0, sum_cos = 0.0;

  if (n_values == 0)
    return 0.0;

  for (i = 0; i < n_values; i++)
    {
      gdouble angle;

      angle = values[i] / 180.0 * G_PI;
      sum_sin += sin (angle);
      sum_cos += cos (angle);
    }

  return hyscan_stats_fit_angle (atan2 (sum_sin, sum_cos) / G_PI * 180.0);
}

/**
 * hyscan_stats_avg_circular_weighted:
 * @values:
 * @weights:
 * @n_values:
 *
 * Returns: круговое среднее с весами
 */
gdouble
hyscan_stats_avg_circular_weighted (const gdouble *values,
                                    const gdouble *weights,
                                    gsize          n_values)
{
  gsize i;
  gdouble sum_sin = 0.0, sum_cos = 0.0;

  if (n_values == 0)
    return 0.0;

  for (i = 0; i < n_values; i++)
    {
      gdouble angle;

      angle = values[i] / 180.0 * G_PI;
      sum_sin += weights[i] * sin (angle);
      sum_cos += weights[i] * cos (angle);
    }

  return hyscan_stats_fit_angle (atan2 (sum_sin, sum_cos) / G_PI * 180.0);
}

/**
 * hyscan_stats_var_circular:
 * @avg
 * @values
 * @n_values
 *
 * Returns: среднеквадратичное отклонение круговой величины
 */
gdouble
hyscan_stats_var_circular (gdouble        avg,
                           const gdouble *values,
                           gsize          n_values)
{
  gsize i;
  gdouble sum = 0.0;
  gdouble d_angle;

  if (n_values == 0)
    return 0.0;

  for (i = 0; i < n_values; i++)
    {
      d_angle = fabs (values[i] - avg);
      if (d_angle > 180.0)
        d_angle = 360.0 - d_angle;

      sum += d_angle * d_angle;
    }

  return sqrt (sum / n_values);
}
