/* hyscan-beamforming.c
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

/**
 * SECTION: hyscan-beamforming
 * @Short_description: класс пространственной обработки данных многоканальных систем
 * @Title: HyScanConvolution
 *
 * Класс HyScanBeamforming используется для обработки данных многоканальных
 * систем и рассчёта углов прихода эхо-сигналов по дистанции с использованием
 * алгоритма лучеформирования с последующим уточнением фазовым методом.
 *
 * Объект для обработки создаётся функцией #hyscan_beamforming_new.
 *
 * Приёмные антенны должны быть разбиты на две группы, используемые для
 * формирования двух лучей при фазовом уточнении угла. Антенны с номерами
 * групп 1 и 3 относятся к первой группе, а 2 и 3 ко второй. Т.е. одна
 * и таже антенна может входить в обе группы. Антенны с номерами групп
 * отличными от 1, 2 или 3, исключаются из обработки.
 *
 * Образы сигналов для свёртки можно установить функцией
 * #hyscan_beamforming_set_signals.
 *
 * Рассчёт углов прихода эхо-сигналов осуществляется функцией
 * #hyscan_beamforming_get_doa.
 */

#include "hyscan-beamforming.h"
#include "hyscan-convolution.h"
#include "hyscan-fft.h"

#include <string.h>
#include <math.h>

#define NEAR_ZERO        1e-5                  /* Практически ноль. */
#define MAX_N_CHANNELS   128                   /* Максимально возможное число каналов. */
#define MAX_N_BEAMS      1024                  /* Маскимально возможное число лучей. */
#define FFT_SIZE         256                   /* Размер преобразования FFT для тонального сигнала. */

struct _HyScanBeamformingPrivate
{
  guint32                n_channels;           /* Число приёмных каналов. */
  gdouble                data_rate;            /* Частота дискретизации, Гц. */
  gdouble                signal_frequency;     /* Центральная частота сигнала, Гц. */
  gdouble                signal_heterodyne;    /* Частота гетеродина, Гц. */
  gdouble               *antenna_offsets;      /* Смещения антенн в решётке, м. */
  gint                  *antenna_groups;       /* Группировка антенн. */
  gdouble                field_of_view;        /* Ширина сектора обзора по углу места, рад. */
  gdouble                sound_velocity;       /* Скорость звука в воде, м/c. */
  gdouble                distance_step;        /* Шаг изменения дистанции по строке, м. */

  guint32                n_beams;              /* Число формируемых лучей. */
  gdouble               *beams_a;              /* Углы формируемых лучей. */
  gdouble               *beams_a_sin;          /* Синусы углов формируемых лучей. */
  gdouble               *beams_k;              /* Коэффициенты для фазового метода уточнения угла. */

  guint32                max_n_points;         /* Максимальное число точек в строке. */
  HyScanComplexFloat  ***ach;                  /* Данные с учётом диаграммо-образующих коэффициентов. */
  HyScanComplexFloat   **beams;                /* Сформированные лучи. */

  HyScanFFT             *fft;                  /* Объект FFT преобразования. */
  HyScanConvolution     *convolution;          /* Объект свёртки данных. */
};

static void    hyscan_beamforming_object_constructed (GObject                   *object);
static void    hyscan_beamforming_object_finalize    (GObject                   *object);

static void    hyscan_beamforming_realloc_buffers    (HyScanBeamformingPrivate  *priv,
                                                      guint32                    max_n_points);
static void    hyscan_beamforming_free_buffers       (HyScanBeamformingPrivate  *priv);

G_DEFINE_TYPE_WITH_PRIVATE (HyScanBeamforming, hyscan_beamforming, G_TYPE_OBJECT)

static void
hyscan_beamforming_class_init (HyScanBeamformingClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = hyscan_beamforming_object_constructed;
  object_class->finalize = hyscan_beamforming_object_finalize;
}

static void
hyscan_beamforming_init (HyScanBeamforming *bf)
{
  bf->priv = hyscan_beamforming_get_instance_private (bf);
}

static void
hyscan_beamforming_object_constructed (GObject *object)
{
  HyScanBeamforming *bf = HYSCAN_BEAMFORMING (object);
  HyScanBeamformingPrivate *priv = bf->priv;

  priv->fft = hyscan_fft_new ();
  priv->convolution = hyscan_convolution_new ();
}

static void
hyscan_beamforming_object_finalize (GObject *object)
{
  HyScanBeamforming *bf = HYSCAN_BEAMFORMING (object);
  HyScanBeamformingPrivate *priv = bf->priv;

  g_clear_object (&priv->convolution);
  g_clear_object (&priv->fft);

  hyscan_beamforming_free_buffers (priv);

  G_OBJECT_CLASS (hyscan_beamforming_parent_class)->finalize (object);
}

/* Функция выделяет память для внутренних буферов данных. */
static void
hyscan_beamforming_realloc_buffers (HyScanBeamformingPrivate *priv,
                                    guint32                   max_n_points)
{
  guint32 channel_i;
  guint32 beam_i;

  if (priv->max_n_points >= max_n_points)
    return;

  for (beam_i = 0; beam_i < priv->n_beams; beam_i++)
    {
      for (channel_i = 0; channel_i < priv->n_channels; channel_i++)
        {
          priv->ach[channel_i][beam_i] = g_renew (HyScanComplexFloat,
                                                  priv->ach[channel_i][beam_i],
                                                  max_n_points);
        }

      priv->beams[beam_i] = g_renew (HyScanComplexFloat,
                                     priv->beams[beam_i],
                                     max_n_points);
    }

  priv->max_n_points = max_n_points;
}

/* Функция освобождаетпамять занятую внутренними буферами. */
static void
hyscan_beamforming_free_buffers (HyScanBeamformingPrivate *priv)
{
  guint32 channel_i;
  guint32 beam_i;

  g_clear_pointer (&priv->antenna_offsets, g_free);
  g_clear_pointer (&priv->antenna_groups, g_free);
  g_clear_pointer (&priv->beams_a_sin, g_free);
  g_clear_pointer (&priv->beams_a, g_free);
  g_clear_pointer (&priv->beams_k, g_free);

  if (priv->ach != NULL)
    {
      for (channel_i = 0; channel_i < priv->n_channels; channel_i++)
        {
          for (beam_i = 0; beam_i < priv->n_beams; beam_i++)
            g_free (priv->ach[channel_i][beam_i]);
          g_free (priv->ach[channel_i]);
        }
      g_clear_pointer (&priv->ach, g_free);
    }

  if (priv->beams != NULL)
    {
      for (beam_i = 0; beam_i < priv->n_beams; beam_i++)
        g_free (priv->beams[beam_i]);
      g_clear_pointer (&priv->beams, g_free);
    }

  priv->n_channels = 0;
}

/**
 * hyscan_beamforming_new:
 *
 * Функция создаёт новый объект #HyScanBeamforming.
 *
 * Returns: #HyScanBeamforming. Для удаления #g_object_unref.
 */
HyScanBeamforming *
hyscan_beamforming_new (void)
{
  return g_object_new (HYSCAN_TYPE_BEAMFORMING, NULL);
}

/**
 * hyscan_beamforming_configure:
 * @bf: указатель на #HyScanBeamforming
 * @n_channels: число приёмных каналов
 * @data_rate: частота дискретизации, Гц
 * @signal_frequency: центральная частота сигнала, Гц
 * @signal_heterodyne: частота цифрового гетеродина, Гц
 * @antenna_offsets: смещения антенн в решётке, м
 * @antenna_groups: группировка антенн в решётке
 * @field_of_view: ширина сектора обзора по углу места, рад
 * @sound_velocity: скорость звука в воде, м/с
 *
 * Функция устанавливает параметры диаграммо-образования.
 *
 * Returns: %TRUE если параметры установлены, иначе %FALSE.
 */
gboolean
hyscan_beamforming_configure (HyScanBeamforming *bf,
                              guint              n_channels,
                              gdouble            data_rate,
                              gdouble            signal_frequency,
                              gdouble            signal_heterodyne,
                              const gdouble     *antenna_offsets,
                              const gint        *antenna_groups,
                              gdouble            field_of_view,
                              gdouble            sound_velocity)
{
  HyScanBeamformingPrivate *priv;
  gdouble antenna_offset_min;
  gdouble antenna_offset_max;
  gdouble antenna_base;
  gdouble antenna_d1;
  gdouble antenna_d2;
  gdouble lambda0;
  guint32 channel_i;
  guint32 beam_i;
  guint32 n_drx;

  g_return_val_if_fail (HYSCAN_IS_BEAMFORMING (bf), FALSE);

  priv = bf->priv;

  /* Проверяем входные параметры. */
  if ((n_channels == 0) || (n_channels > MAX_N_CHANNELS) ||
      (sound_velocity < NEAR_ZERO) ||
      (data_rate < NEAR_ZERO) ||
      (signal_frequency < NEAR_ZERO) ||
      (field_of_view < NEAR_ZERO) ||
      (antenna_offsets == NULL) ||
      (antenna_groups == NULL))
    {
      g_warning ("HyScanBeamforming: error in input parameters");
      return FALSE;
    }

  /* Смещение антенн в решётке ограничено +-1 метр. */
  for (channel_i = 0; channel_i < priv->n_channels; channel_i++)
    {
      if ((antenna_offsets[channel_i] < -1.0) || (antenna_offsets[channel_i] > 1.0))
        {
          g_warning ("HyScanBeamforming: error in antenna offsets");
          return FALSE;
        }
    }

  /* Очищаем текущую конфигурацию. */
  hyscan_beamforming_free_buffers (priv);
  hyscan_convolution_set_image_fd (priv->convolution, 0, NULL, 0);

  /* Новые параметры обработки. */
  priv->n_channels = n_channels;
  priv->data_rate = data_rate;
  priv->signal_frequency = signal_frequency;
  priv->signal_heterodyne = signal_heterodyne;
  priv->antenna_offsets = g_memdup (antenna_offsets, n_channels * sizeof (gdouble));
  priv->antenna_groups = g_memdup (antenna_groups, n_channels * sizeof (gint));
  priv->field_of_view = field_of_view;
  priv->sound_velocity = sound_velocity;

  /* Шаг изменения дистанции по строке. */
  priv->distance_step = priv->sound_velocity / priv->data_rate / 2.0;

  /* Размер решётки. */
  antenna_offset_min =  G_MAXDOUBLE;
  antenna_offset_max = -G_MAXDOUBLE;
  for (channel_i = 0; channel_i < priv->n_channels; channel_i++)
    {
      if (priv->antenna_offsets[channel_i] < antenna_offset_min)
        antenna_offset_min = priv->antenna_offsets[channel_i];
      if (priv->antenna_offsets[channel_i] > antenna_offset_max)
        antenna_offset_max = priv->antenna_offsets[channel_i];
    }

  /* Фазовый центр 1-ой подрешётки - группы 1 и 3. */
  antenna_d1 = 0.0;
  n_drx = 0;
  for (channel_i = 0; channel_i < priv->n_channels; channel_i++)
    {
      if ((priv->antenna_groups[channel_i] == 1) ||
          (priv->antenna_groups[channel_i] == 3))
        {
          antenna_d1 += priv->antenna_offsets[channel_i];
          n_drx += 1;
        }
    }
  if (n_drx == 0)
    {
      g_warning ("HyScanBeamforming: empty group 1");
      return FALSE;
    }
  antenna_d1 /= n_drx;

  /* Фазовый центр 2-ой подрешётки - группы 2 и 3. */
  antenna_d2 = 0.0;
  n_drx = 0;
  for (channel_i = 0; channel_i < priv->n_channels; channel_i++)
    {
      if ((priv->antenna_groups[channel_i] == 2) ||
          (priv->antenna_groups[channel_i] == 3))
        {
          antenna_d2 += priv->antenna_offsets[channel_i];
          n_drx += 1;
        }
    }
  if (n_drx == 0)
    {
      g_warning ("HyScanBeamforming: empty group 2");
      return FALSE;
    }
  antenna_d2 /= n_drx;

  /* База между двумя группами аннтен в решётке. */
  antenna_base = antenna_d2 - antenna_d1;

  /* Максимальное число формируемых лучей. */
  lambda0 = priv->sound_velocity / priv->signal_frequency;
  priv->n_beams = 8 * ceil (priv->field_of_view /
                            asin (lambda0 / (antenna_offset_max - antenna_offset_min + lambda0)));

  /* Число лучей должно быть не менее чем число каналов и не более MAX_N_BEAMS. */
  priv->n_beams = CLAMP (priv->n_beams, priv->n_channels, MAX_N_BEAMS);

  /* Углы формируемых лучей и коэффициенты для метода фазового уточнения углов. */
  priv->beams_a = g_new (gdouble, priv->n_beams);
  priv->beams_a_sin = g_new (gdouble, priv->n_beams);
  priv->beams_k = g_new (gdouble, priv->n_beams);
  for (beam_i = 0; beam_i < priv->n_beams; beam_i++)
    {
      priv->beams_a[beam_i] = -priv->field_of_view * (1.0 - (gdouble)beam_i / (priv->n_beams - 1)) +
                               (priv->field_of_view / 2.0);
      priv->beams_a_sin[beam_i] = sin (priv->beams_a[beam_i]);
      priv->beams_k[beam_i] = priv->sound_velocity /
                              (2.0 * G_PI * priv->signal_frequency * antenna_base * cos (priv->beams_a[beam_i]));
    }

  /* Буферы данных. */
  priv->beams = g_new0 (HyScanComplexFloat*, priv->n_beams);
  priv->ach = g_new0 (HyScanComplexFloat**, priv->n_channels);
  for (channel_i = 0; channel_i < priv->n_channels; channel_i++)
    priv->ach[channel_i] = g_new0 (HyScanComplexFloat*, priv->n_beams);

  return TRUE;
}

/**
 * hyscan_beamforming_set_signals:
 * @bf: указатель на #HyScanBeamforming
 * @signals: (nullable): массив образов сигналов
 * @n_points: число точек в образе сигнала
 *
 * Функция устанавливает образы сигналов для свёртки. Образы передаются
 * как двумерный массив, где первый индекс соответствует номеру приёмного
 * канала. Размеры образов для всех каналов должны быть одинаковыми.
 *
 * Returns: %TRUE если образы сигналов установлены, иначе %FALSE.
 */
gboolean
hyscan_beamforming_set_signals (HyScanBeamforming         *bf,
                                const HyScanComplexFloat **signals,
                                guint32                    n_points)
{
  HyScanBeamformingPrivate *priv;

  gboolean status = FALSE;

  HyScanComplexFloat *beam_w = NULL;
  HyScanComplexFloat *signal_f = NULL;
  gdouble *k = NULL;

  gdouble fft_freq_delta;
  gint32 freq_shift;
  gint32 fft_size;

  guint32 channel_i;
  guint32 beam_i;
  guint32 conv_i;
  gint32 fft_i;

  g_return_val_if_fail (HYSCAN_IS_BEAMFORMING (bf), FALSE);

  priv = bf->priv;

  /* Размер преобразования FFT. */
  if (signals == NULL)
    fft_size = FFT_SIZE;
  else
    fft_size = hyscan_fft_get_transform_size (2 * n_points);

  /* Сдвиг частот в преобразовании FFT с частоты гетеродина. */
  freq_shift = roundf (fft_size *
                       fmodf (priv->signal_frequency - priv->signal_heterodyne, priv->data_rate) /
                       priv->data_rate);
  freq_shift = freq_shift - fft_size / 2;

  fft_freq_delta = (gint)(fft_size * (priv->signal_frequency - priv->signal_heterodyne) / priv->data_rate);
  fft_freq_delta *= (priv->data_rate / fft_size);
  fft_freq_delta += priv->signal_heterodyne;

  /* Массив волновых чисел. */
  k = g_new (gdouble, fft_size);
  for (fft_i = 0; fft_i < fft_size; fft_i++)
    {
      gdouble k_i;
      gdouble frequency_i;

      k_i = fft_i - freq_shift;
      if (k_i < 0)
        k_i += fft_size;
      if (k_i >= fft_size)
        k_i -= fft_size;

      frequency_i = priv->data_rate * (k_i / fft_size - 0.5);
      frequency_i += fft_freq_delta;
      k[fft_i] = 2.0 * G_PI * frequency_i / priv->sound_velocity;
    }

  /* Расчитываем диаграммо-формирующие коэффициенты для каждого канала. */
  signal_f = hyscan_fft_alloc (HYSCAN_FFT_TYPE_COMPLEX, fft_size);
  beam_w = g_new (HyScanComplexFloat, fft_size);
  for (channel_i = 0; channel_i < priv->n_channels; channel_i++)
    {
      /* Образ сигнала в частотной области. */
      if (signals != NULL)
        {
          memcpy (signal_f, signals[channel_i], n_points * sizeof (HyScanComplexFloat));
          memset (signal_f + n_points, 0, (fft_size - n_points) * sizeof (HyScanComplexFloat));

          hyscan_fft_transform_complex (priv->fft, HYSCAN_FFT_DIRECTION_FORWARD,
                                        signal_f, fft_size);

          /* Образ должен быть комплексно-сопряжённым. */
          for (fft_i = 0; fft_i < fft_size; fft_i++)
            signal_f[fft_i].im = -signal_f[fft_i].im;
        }

      /* Для текущего канала расчитываем коэффициенты по каждому формируемому лучу. */
      for (beam_i = 0; beam_i < priv->n_beams; beam_i++)
        {
          for (fft_i = 0; fft_i < fft_size; fft_i++)
            {
              gdouble phase;
              phase = priv->antenna_offsets[channel_i] * priv->beams_a_sin[beam_i] * k[fft_i];

              /* Если есть образы сигналов для свёртки, объединяем их с
               * диаграммо-образующими коэффициентами. */
              if (signals != NULL)
                {
                  gfloat re1 = cos (phase);
                  gfloat im1 = -sin (phase);
                  gfloat re2 = signal_f[fft_i].re;
                  gfloat im2 = signal_f[fft_i].im;
                  beam_w[fft_i].re = re1 * re2 - im1 * im2;
                  beam_w[fft_i].im = re1 * im2 + im1 * re2;
                }
              else
                {
                  beam_w[fft_i].re = cos (phase);
                  beam_w[fft_i].im = -sin (phase);
                }
            }

          /* Сохраняем полученные коэффициенты в объекте свёртки. */
          conv_i = channel_i * priv->n_beams + beam_i;
          status = hyscan_convolution_set_image_fd (priv->convolution, conv_i,
                                                    beam_w, fft_size);
          if (!status)
            goto exit;
        }
    }

  status = TRUE;

exit:
  hyscan_fft_free (signal_f);
  g_free (beam_w);
  g_free (k);

  return status;
}

/**
 * hyscan_beamforming_get_doa:
 * @bf: указатель на #HyScanBeamforming
 * @doa: (array length=n_points) (transfer none): буффер для рассчитанных углов
 * @data: (transfer none): акустические данные каналов
 * @n_points: число точек в данных
 *
 * Функция рассчитывает углы приходы эхо-сигналов для каждой из дальностей.
 * Входные данные передаются как двумерный массив, где первый индекс
 * соответствует номеру приёмного канала.
 *
 * Returns: %TRUE если углы рассчитаны, иначе %FALSE.
 */
gboolean
hyscan_beamforming_get_doa (HyScanBeamforming         *bf,
                            HyScanDOA                 *doa,
                            const HyScanComplexFloat **data,
                            guint32                    n_points)
{
  HyScanBeamformingPrivate *priv;

  gdouble distance;
  guint32 channel_i;
  guint32 beam_i;
  guint32 point_i;

  g_return_val_if_fail (HYSCAN_IS_BEAMFORMING (bf), FALSE);

  priv = bf->priv;

  /* Не заданы параметры обработки. */
  if (priv->n_channels == 0)
    return FALSE;

  hyscan_beamforming_realloc_buffers (priv, n_points);

  /* Формируем лучи. */
  for (beam_i = 0; beam_i < priv->n_beams; beam_i++)
    {
      HyScanComplexFloat *beam = priv->beams[beam_i];

      memset (beam, 0, n_points * sizeof (HyScanComplexFloat));
      for (channel_i = 0; channel_i < priv->n_channels; channel_i++)
        {
          HyScanComplexFloat *ach = priv->ach[channel_i][beam_i];
          gint32 conv_i;

          /* Каждый канал сворачиваем с диаграммо-формирующими
           * коэффицентами для текущего луча. */
          conv_i = channel_i * priv->n_beams + beam_i;
          memcpy (ach, data[channel_i], n_points * sizeof (HyScanComplexFloat));

          if (channel_i == 1)
            {
              for (point_i = 0; point_i < n_points; point_i++)
                {
                  ach[point_i].re = -ach[point_i].re;
                  ach[point_i].im = -ach[point_i].im;
                }
            }
          
          hyscan_convolution_convolve (priv->convolution, conv_i,
                                       ach, n_points, 1.0);

          /* Формирование лучей. */
          for (point_i = 0; point_i < n_points; point_i++)
            {
              beam[point_i].re += ach[point_i].re;
              beam[point_i].im += ach[point_i].im;
            }
        }
    }

  /* Ищем максимум по углу на каждой дистанции. */
  distance = priv->distance_step;
  for (point_i = 0; point_i < n_points; point_i++)
    {
      for (beam_i = 0; beam_i < priv->n_beams; beam_i++)
        {
          HyScanComplexFloat *beam = priv->beams[beam_i];
          gfloat amplitude;
          gfloat re, im;

          re = beam[point_i].re;
          im = beam[point_i].im;
          amplitude = sqrtf (re * re + im * im);

          /* На данном этапе запоминаем индекс луча, на следующем
           * этапе уточним его в действительный угол фазовым методом.*/
          if (beam_i == 0)
            {
              doa[point_i].angle = 0.1;
              doa[point_i].distance = distance;
              doa[point_i].amplitude = amplitude;
            }
          else if (amplitude > doa[point_i].amplitude)
            {
              doa[point_i].angle = beam_i + 0.1;
              doa[point_i].amplitude = amplitude;
            }
        }

      distance += priv->distance_step;
    }

  /* Уточняем значение угла на каждой дистанции фазовым методом. */
  for (point_i = 0; point_i < n_points; point_i++)
    {
      HyScanComplexFloat ach1 = {0.0, 0.0};
      HyScanComplexFloat ach2 = {0.0, 0.0};
      HyScanComplexFloat ach12 = {0.0, 0.0};
      gfloat phase_diff;

      beam_i = doa[point_i].angle;

      /* Формируем два луча в направлении beam_i. */
      for (channel_i = 0; channel_i < priv->n_channels; channel_i++)
        {
          HyScanComplexFloat *ach = priv->ach[channel_i][beam_i];

          /* Группы антенн 1 и 3 - первый луч. */
          if ((priv->antenna_groups[channel_i] == 1) ||
              (priv->antenna_groups[channel_i] == 3))
            {
              ach1.re += ach[point_i].re;
              ach1.im += ach[point_i].im;
            }

          /* Группы антенн 2 и 3 - второй луч, комплексно-сопряжённый. */
          if ((priv->antenna_groups[channel_i] == 2) ||
              (priv->antenna_groups[channel_i] == 3))
            {
              ach2.re += ach[point_i].re;
              ach2.im -= ach[point_i].im;
            }
        }

      /* Разница фаз в лучах. */
      ach12.re = ach1.re * ach2.re - ach1.im * ach2.im;
      ach12.im = ach1.im * ach2.re + ach1.re * ach2.im;

      /* Расчёт поправки углов. */
      phase_diff = atan2f (ach12.im, ach12.re);
      doa[point_i].angle = priv->beams_a[beam_i] - asinf (phase_diff * priv->beams_k[beam_i]);
    }

  return TRUE;
}
