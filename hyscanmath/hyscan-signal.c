/*
 * \file hyscan-signal.c
 *
 * \brief Исходный файл функций для расчёта сигналов и их образов
 * \author Andrei Fadeev (andrei@webcontrol.ru)
 * \date 2016
 * \license Проприетарная лицензия ООО "Экран"
 *
 */

#include "hyscan-signal.h"
#include <math.h>

/* Функция расчитывает образ тонального сигнала для выполнения свёртки. */
HyScanComplexFloat *
hyscan_signal_image_tone (gdouble  disc_freq,
                          gdouble  signal_freq,
                          gdouble  duration,
                          guint   *n_points)
{
  HyScanComplexFloat *image;
  guint i;

  *n_points = duration * disc_freq;
  image = g_new0 (HyScanComplexFloat, *n_points);

  for (i = 0; i < *n_points; i++)
    {
      gdouble time = i * (1.0 / disc_freq);
      gdouble phase = 2.0 * G_PI * signal_freq * time;

      image[i].re = cos (phase);
      image[i].im = sin (phase);
    }

  return image;
}

/* Функция расчитывает образ сигнала ЛЧМ для выполнения свёртки. */
HyScanComplexFloat *
hyscan_signal_image_lfm (gdouble  disc_freq,
                         gdouble  start_freq,
                         gdouble  end_freq,
                         gdouble  duration,
                         guint   *n_points)
{
  HyScanComplexFloat *image;
  gdouble bandwidth;
  guint i;

  bandwidth = end_freq - start_freq;
  *n_points = duration * disc_freq;
  image = g_new0 (HyScanComplexFloat, *n_points);

  for (i = 0; i < *n_points; i++)
    {
      gdouble time = i * (1.0 / disc_freq);
      gdouble phase =  2.0 * G_PI * start_freq * time + G_PI * bandwidth * time * time / duration;

      image[i].re = cos (phase);
      image[i].im = sin (phase);
    }

  return image;
}
