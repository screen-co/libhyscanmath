/* interferometry-test.c
 *
 * Copyright 2020 Screen LLC, Andrei Fadeev <andrei@webcontrol.ru>
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

#include "hyscan-beamforming.h"
#include "hyscan-signal.h"
#include "hyscan-fft.h"

#include <string.h>
#include <math.h>

#define SOUND_VELOCITY   1500.0
#define TARGET_THRESHOLD 0.5

int
main (int    argc,
      char **argv)
{
  gdouble discretization = 80000.0; /* Частота дискретизации. */
  gdouble frequency = 100000.0;     /* Частота сигнала. */
  gdouble bandwidth = 20000.0;      /* Полоса для ЛЧМ сигнала. */
  gdouble duration = 0.001;         /* Длительность сигнала. */
  gchar  *type = NULL;              /* Тип сигнала. */
  gdouble field_of_view = 90.0;     /* Угол обзора. */
  gint    n_channels = 32;          /* Число приёмных каналов. */
  gint    n_targets = 32;           /* Число целей. */
  gint    k_distance = 5;           /* Коэффициент расстояния между целями по дальности. */

  gdouble target_abegin;
  gdouble target_astep;
  gdouble target_rbegin;
  gdouble target_rstep;

  gdouble *antenna_offsets;
  gint *antenna_groups;

  HyScanComplexFloat *signal;
  HyScanComplexFloat **zi;
  guint zi_points;

  HyScanComplexFloat **data;
  gint n_points;

  HyScanDOA *doa;
  gdouble max_amp;
  gint max_amp_i;

  gint freq_shift;
  gint channel_i;
  gint target_i;
  gint point_i;

  HyScanFFT *fft;
  HyScanBeamforming *bf;

  /* Разбор командной строки. */
  {
    gchar **args;
    GError *error = NULL;
    GOptionContext *context;
    GOptionEntry entries[] =
      {
        { "discretization", 'd', 0, G_OPTION_ARG_DOUBLE, &discretization, "Signal discretization [80000], Hz", NULL },
        { "frequency", 'f', 0, G_OPTION_ARG_DOUBLE, &frequency, "Signal frequency [100000], Hz", NULL },
        { "bandwidth", 'w', 0, G_OPTION_ARG_DOUBLE, &bandwidth, "LFM signal bandwidth [20000], Hz", NULL },
        { "duration", 't', 0, G_OPTION_ARG_DOUBLE, &duration, "Signal duration [0.001], s", NULL },
        { "signal", 's', 0, G_OPTION_ARG_STRING, &type, "Signal type (tone, lfm)[lfm]", NULL },
        { "field-of-view", 'v', 0, G_OPTION_ARG_DOUBLE, &field_of_view, "Field of view (10-90)[90], deg", NULL },
        { "n-channels", 'c', 0, G_OPTION_ARG_INT, &n_channels, "Number of channels (2-32)[32]", NULL },
        { "n-targets", 'n', 0, G_OPTION_ARG_INT, &n_targets, "Number of targets (1-99)[31]", NULL },
        { "k-distance", 'k', 0, G_OPTION_ARG_INT, &k_distance, "Target step factor (1-10)[5]", NULL },
        { NULL }
      };

#ifdef G_OS_WIN32
    args = g_win32_get_command_line ();
#else
    args = g_strdupv (argv);
#endif

    context = g_option_context_new ("");
    g_option_context_set_help_enabled (context, TRUE);
    g_option_context_add_main_entries (context, entries, NULL);
    g_option_context_set_ignore_unknown_options (context, FALSE);
    if (!g_option_context_parse_strv (context, &args, &error))
      {
        g_print ("%s\n", error->message);
        return -1;
      }

    if ((discretization < 1.0) || (frequency < 1.0) ||
        (bandwidth < 1.0) || (duration < 1e-7))
      {
        g_print ("%s", g_option_context_get_help (context, FALSE, NULL));
        return 0;
      }

    g_option_context_free (context);
    g_strfreev (args);
  }

  fft = hyscan_fft_new ();

  /* Параметры сигнала по умолчанию. */
  if (type == NULL)
    type = g_strdup ("lfm");

  if (g_strcmp0 (type, "tone") == 0)
    bandwidth = 1.0 / duration;

  if (discretization < 2 * bandwidth)
    discretization = 2 * bandwidth;

  /* Ограничение по параметрам. */
  n_channels = CLAMP (n_channels, 2, 32);
  n_targets = CLAMP (n_targets, 1, 99);
  k_distance = CLAMP (k_distance, 1, 10);

  /* Переводим угол обзора в радианы. */
  field_of_view = CLAMP (field_of_view, 10.0, 90.0);
  field_of_view = field_of_view * (G_PI / 180.0);

  /* Начальный угол и дистанция расстановки целей и шаг изменения. */
  target_astep = field_of_view / (n_targets + 1);
  target_abegin = -field_of_view / 2.0 + target_astep * 0.75;
  target_rstep = k_distance * discretization / bandwidth;
  n_points = target_rstep * (n_targets + 1);
  n_points = hyscan_fft_get_transform_size (n_points);
  target_rbegin = (n_points - target_rstep * (n_targets - 1)) / 2;
  target_rstep = (target_rstep / discretization) * (SOUND_VELOCITY / 2.0);
  target_rbegin = (target_rbegin / discretization) * (SOUND_VELOCITY / 2.0);

  /* Информация. */
  g_print ("Signal: %s, duration = %f, bandwidth = %f\n", type, duration, bandwidth);
  g_print ("Targets angle begin = %f\n", 57.2959 * target_abegin);
  g_print ("Targets angle step = %f\n", 57.2959 * target_astep);
  g_print ("Targets distance begin = %f\n", target_rbegin);
  g_print ("Targets distance step = %f\n", target_rstep);
  g_print ("Number of targets = %d\n", n_targets);
  g_print ("Number of points = %d\n", n_points);

  /* Смещения и группировка приёмных антенн. */
  antenna_offsets = g_new0 (gdouble, n_channels);
  antenna_groups = g_new0 (gint, n_channels);
  for (channel_i = 0; channel_i < n_channels; channel_i++)
    {
      gdouble lambda = SOUND_VELOCITY / frequency;
      gdouble max_offset = (n_channels - 1)* (lambda / 2.0);

      antenna_offsets[channel_i] = channel_i * (lambda / 2.0) - (max_offset / 2.0);
      antenna_groups [channel_i] = (channel_i < (n_channels / 2)) ? 1 : 2;
    }

  /* Образ сигнала в частотной области. */
  signal = hyscan_fft_alloc (HYSCAN_FFT_TYPE_COMPLEX, n_points);
  memset (signal, 0, n_points * sizeof (HyScanComplexFloat));

  zi = g_new0 (HyScanComplexFloat*, n_channels);
  if (g_strcmp0 (type, "tone") == 0)
    {
      for (channel_i = 0; channel_i < n_channels; channel_i++)
        {
          zi[channel_i] = hyscan_signal_image_tone (discretization,
                                                    0.0,
                                                    duration,
                                                    &zi_points);
        }

      memcpy (signal, zi[0], zi_points * sizeof (HyScanComplexFloat));
    }
  else
    {
      for (channel_i = 0; channel_i < n_channels; channel_i++)
        {
          zi[channel_i] = hyscan_signal_image_lfm (discretization,
                                                   -bandwidth /2.0,
                                                   bandwidth /2.0,
                                                   duration,
                                                   &zi_points);
        }

      memcpy (signal, zi[0], zi_points * sizeof (HyScanComplexFloat));
    }

  hyscan_fft_transform_complex (fft, HYSCAN_FFT_DIRECTION_FORWARD, signal, n_points);

  /* Сдвиг частот в преобразовании FFT. */
  freq_shift = n_points / 2;

  /* Формируем отражения от целей. По углу цели расставлены с равномерным шагом
   * относительно перпендикуляра к анетене. Половина целей находится с одной
   * стороны от перпендикуляра, половина с другой. Шаг изменения угла цели
   * field_of_view / n_targets. Дистанция до цели изменяется с равномерным шагом
   * равным 10 * discretization/ bandwidth. Это примерно 10 расстояний разрешающей
   * способности по сигналу. Первая цель находится на расстоянии равном
   * длительности излучаемого сигнала. */
  data = g_new0 (HyScanComplexFloat*, n_channels);
  for (channel_i = 0; channel_i < n_channels; channel_i++)
    {
      data[channel_i] = hyscan_fft_alloc (HYSCAN_FFT_TYPE_COMPLEX, n_points);
      memset (data[channel_i], 0, n_points * sizeof (HyScanComplexFloat));

      /* Отражение от каждой цели для текущего канала. */
      for (target_i = 0; target_i < n_targets; target_i++)
        {
          gdouble x_sonar;
          gdouble r_target;
          gdouble a_target;
          gdouble x_target;
          gdouble y_target;
          gdouble r_signal;
          gdouble dt_signal;

          /* Местоположение приёмной антенны канала. */
          x_sonar = antenna_offsets[channel_i];

          /* Координата цели в координатах дистанция, угол. */
          r_target = target_rbegin + target_i * target_rstep;
          a_target = target_abegin + target_i * target_astep;
          x_target = r_target * sin (a_target);
          y_target = r_target * cos (a_target);

          /* Расстояние, прошедшее сигналом, до каждой цели и обратно. */
          r_signal = sqrt (x_target * x_target + y_target * y_target);
          r_signal += sqrt ((x_target - x_sonar) * (x_target - x_sonar) +
                            y_target * y_target);

          /* Задержка прохождения сигнала. */
          dt_signal = r_signal / SOUND_VELOCITY;

          /* Рассчитываем отражения от цели для каждой частоты. */
          for (point_i = 0; point_i < n_points; point_i++)
            {
              gint32 k_i;
              gdouble frequency_i;
              gdouble phase;
              gfloat re1, im1;
              gfloat re2, im2;

              k_i = point_i - freq_shift;
              if (k_i < 0)
                k_i += n_points;
              if (k_i >= n_points)
                k_i -= n_points;

              frequency_i = discretization * (((gdouble)k_i/(n_points - 1.0)) - 0.5) + frequency;
              phase = -2.0 * G_PI * frequency_i * dt_signal;

              re1 = cos (phase);
              im1 = sin (phase);
              re2 = signal[point_i].re;
              im2 = signal[point_i].im;

              data[channel_i][point_i].re += re1 * re2 - im1 * im2;
              data[channel_i][point_i].im += re1 * im2 + im1 * re2;
            }
        }

      /* Преобразование во временную область. */
      hyscan_fft_transform_complex (fft, HYSCAN_FFT_DIRECTION_BACKWARD, data[channel_i], n_points);
    }

  /* Объект обработки интерферометрических данных. */
  bf = hyscan_beamforming_new ();
  hyscan_beamforming_configure (bf,
                                n_channels,
                                discretization,
                                frequency,
                                frequency,
                                antenna_offsets,
                                antenna_groups,
                                field_of_view,
                                SOUND_VELOCITY);

  /* Образы излучаемых сигналов. */
  hyscan_beamforming_set_signals (bf, (const HyScanComplexFloat **)zi, zi_points);

  doa = g_new0 (HyScanDOA, n_points);
  hyscan_beamforming_get_doa (bf, doa, (const HyScanComplexFloat **)data, n_points);

  /* Нормируем амплитуды целей. */
  max_amp = 0.0;
  for (point_i = 0; point_i < n_points; point_i++)
    {
      if (max_amp < doa[point_i].amplitude)
        max_amp = doa[point_i].amplitude;
    }
  for (point_i = 0; point_i < n_points; point_i++)
    doa[point_i].amplitude /= max_amp;

  /* Сравниваем местоположение целей с заданным. */
  max_amp_i = 0;
  max_amp = 0.0;
  target_i = 0;
  for (target_i = 0, point_i = 0; point_i < n_points; point_i++)
    {
      gdouble r_target;
      gdouble a_target;

      /* Ищем пики целей. */
      if ((doa[point_i].amplitude > TARGET_THRESHOLD) &&
          (max_amp < doa[point_i].amplitude))
        {
          max_amp = doa[point_i].amplitude;
          max_amp_i = point_i;
          continue;
        }
      else if ((max_amp < TARGET_THRESHOLD) ||
               (doa[point_i].amplitude > TARGET_THRESHOLD))
        {
          continue;
        }

      /* Действительное местоположение цели.  */
      r_target = target_rbegin + target_i * target_rstep;
      a_target = target_abegin + target_i * target_astep;

      g_print ("target %d, amlitude %f, angle %f %f (%f), distance %f %f (%f)\n",
               target_i,
               doa[max_amp_i].amplitude,
               57.2958 * a_target, 57.2958 * doa[max_amp_i].angle, 57.2958 * (a_target - doa[max_amp_i].angle),
               r_target, doa[max_amp_i].distance, r_target - doa[max_amp_i].distance);

      target_i += 1;
      max_amp = 0.0;
    }

  g_free (type);

  g_free (antenna_offsets);
  g_free (antenna_groups);

  hyscan_fft_free (signal);
  for (channel_i = 0; channel_i < n_channels; channel_i++)
    hyscan_fft_free (data[channel_i]);
  g_free (data);
  g_free (doa);

  g_object_unref (bf);
  g_object_unref (fft);

  return 0;
}
