/* hyscan-signal.c
 *
 * Copyright 2016-2019 Screen LLC, Andrei Fadeev <andrei@webcontrol.ru>
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

#include "hyscan-signal.h"
#include <math.h>

/**
 * hyscan_signal_image_tone:
 * @discretization_frequency: частота дискретизации сигнала, Гц
 * @signal_frequency: несущая частота сигнала, Гц
 * @duration: длительность сигнала, с
 * @n_points: расчитанный размер образа сигнала в точках
 *
 * Функция расчитывает образ тонального сигнала для выполнения свёртки.
 *
 * Returns: (array length=n_points) (transfer full): Образ сигнала.
 *          Для освобождения #g_free.
 */
HyScanComplexFloat *
hyscan_signal_image_tone (gdouble  discretization_frequency,
                          gdouble  signal_frequency,
                          gdouble  duration,
                          guint   *n_points)
{
  HyScanComplexFloat *image;
  guint i;

  *n_points = duration * discretization_frequency;
  image = g_new0 (HyScanComplexFloat, *n_points);

  for (i = 0; i < *n_points; i++)
    {
      gdouble time = i * (1.0 / discretization_frequency);
      gdouble phase = 2.0 * G_PI * signal_frequency * time;

      image[i].re = cos (phase);
      image[i].im = sin (phase);
    }

  return image;
}

/**
 * hyscan_signal_image_tone:
 * @discretization_frequency: частота дискретизации сигнала, Гц
 * @start_frequency: начальная частота сигнала, Гц
 * @end_frequency: конечная частота сигнала, Гц
 * @duration: длительность сигнала, с
 * @n_points: расчитанный размер образа сигнала в точках
 *
 * Функция расчитывает образ ЛЧМ сигнала для выполнения свёртки.
 *
 * Returns: (array length=n_points) (transfer full): Образ сигнала.
 *          Для освобождения #g_free.
 */
HyScanComplexFloat *
hyscan_signal_image_lfm (gdouble  discretization_freq,
                         gdouble  start_frequency,
                         gdouble  end_frequency,
                         gdouble  duration,
                         guint   *n_points)
{
  HyScanComplexFloat *image;
  gdouble bandwidth;
  guint i;

  bandwidth = end_frequency - start_frequency;
  *n_points = duration * discretization_freq;
  image = g_new0 (HyScanComplexFloat, *n_points);

  for (i = 0; i < *n_points; i++)
    {
      gdouble time = i * (1.0 / discretization_freq);
      gdouble phase =  2.0 * G_PI * start_frequency * time + G_PI * bandwidth * time * time / duration;

      image[i].re = cos (phase);
      image[i].im = sin (phase);
    }

  return image;
}
