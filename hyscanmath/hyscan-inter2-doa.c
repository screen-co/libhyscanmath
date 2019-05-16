/* hyscan-inter2-doa.c
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

/**
 * SECTION: hyscan-inter2-doa
 * @Short_description: класс обработки пространственных данных для двух каналов
 * @Title: HyScanInter2DOA
 *
 * Класс предназначен для обработки 2-х канальных интерферометрических данных.
 *
 * Создание объекта производится с помощью функции #hyscan_inter2_doa_new.
 *
 * Установка параметров обрабатываемых данных осуществляется с помощью функции
 * #hyscan_inter2_doa_configure. После установки параметров можно узнать
 * диапазон изменения угла азимута цели с помощью функции
 * #hyscan_inter2_doa_get_alpha.
 *
 * Обработка пространственных данных осуществляется с помощью функции
 * #hyscan_inter2_doa_get.
 */

#include "hyscan-inter2-doa.h"

#include <hyscan-buffer.h>
#include <math.h>

struct _HyScanInter2DOAPrivate
{
  gdouble          signal_frequency;       /* Несущая частота сигнала, Гц. */
  gdouble          antenna_base;           /* Расстояние между антеннами, м. */
  gdouble          data_rate;              /* Частота дискретизации. */
  gdouble          sound_velocity;         /* Скорость звука, м/с. */

  gdouble          wave_length;            /* Длина волны, м. */
  gdouble          distance_step;          /* Шаг увеличения дальности для каждого отсчёта. */
  gdouble          phase_range;            /* Диапазон изменения фазы. */
  gdouble          alpha;                  /* Максимальный/минимальный угол по азимуту, рад. */
};

G_DEFINE_TYPE_WITH_PRIVATE (HyScanInter2DOA, hyscan_inter2_doa, G_TYPE_OBJECT)

static void
hyscan_inter2_doa_class_init (HyScanInter2DOAClass *klass)
{
}

static void
hyscan_inter2_doa_init (HyScanInter2DOA *inter2doa)
{
  inter2doa->priv = hyscan_inter2_doa_get_instance_private (inter2doa);
}

/**
 * hyscan_inter2_doa_new:
 *
 * Функция создаёт новый объект #HyScanInter2DOA.
 *
 * Returns: #HyScanInter2DOA. Для удаления #g_object_unref.
 */
HyScanInter2DOA *
hyscan_inter2_doa_new (void)
{
  return g_object_new (HYSCAN_TYPE_INTER2_DOA, NULL);
}

/**
 * hyscan_inter2_doa_configure:
 * @inter2doa: указатель на #HyScanInter2DOA
 * @signal_frequency: несущая частота сигнала, Гц
 * @antenna_base: расстояние между антенами, м
 * @data_rate: частота дискретизации, Гц
 * @sound_velocity: скрость звука, м/с
 *
 * Функция задаёт параметры обрабатываемых данных. Эта функция должна быть
 * вызвана до вызова функций #hyscan_inter2_doa_get_alpha и
 * #hyscan_inter2_doa_get.
 */
void
hyscan_inter2_doa_configure (HyScanInter2DOA *inter2doa,
                             gdouble          signal_frequency,
                             gdouble          antenna_base,
                             gdouble          data_rate,
                             gdouble          sound_velocity)
{
  HyScanInter2DOAPrivate *priv;

  g_return_if_fail (HYSCAN_IS_INTER2_DOA (inter2doa));

  priv = inter2doa->priv;

  priv->signal_frequency = signal_frequency;
  priv->antenna_base = antenna_base;
  priv->data_rate = data_rate;
  priv->sound_velocity = sound_velocity;

  priv->wave_length = sound_velocity / signal_frequency;
  priv->distance_step = sound_velocity / (2.0 * data_rate);

  priv->phase_range = priv->wave_length / (2.0 * G_PI * antenna_base);
  priv->alpha = asinf (priv->phase_range * G_PI);
  priv->alpha = ABS (priv->alpha);
}

/**
 * hyscan_inter2_doa_configure:
 * @inter2doa: указатель на #HyScanInter2DOA
 *
 * Функция возвращает диапазон изменения угла азимута цели. Азимут цели
 * может находится в пределах +/- возвращаемого значения.
 *
 * Returns: Диапазон изменения угла по азимуту, рад.
 */
gdouble
hyscan_inter2_doa_get_alpha (HyScanInter2DOA *inter2doa)
{
  g_return_val_if_fail (HYSCAN_IS_INTER2_DOA (inter2doa), 0.0);

  return inter2doa->priv->alpha;
}

/**
 * hyscan_inter2_doa_configure:
 * @inter2doa: указатель на #HyScanInter2DOA
 * @doa: (out) (array length=n_points) (transfer none): буфер для расчитанных данных
 * @data1: (in) (array length=n_points) (transfer none): данные первого канала
 * @data2: (in) (array length=n_points) (transfer none): данные второго канала
 * @n_points: число точек данныx
 *
 * Функция расчитывает дистанцию, угол и амлитуду точек цели.
 */
void
hyscan_inter2_doa_get (HyScanInter2DOA          *inter2doa,
                       HyScanDOA                *doa,
                       const HyScanComplexFloat *data1,
                       const HyScanComplexFloat *data2,
                       guint32                   n_points)
{
  gfloat phase_range;
  gfloat step;
  guint32 i;

  g_return_if_fail (HYSCAN_IS_INTER2_DOA (inter2doa));

  phase_range = inter2doa->priv->phase_range;
  step = inter2doa->priv->distance_step;

  for (i = 0; i < n_points; i++)
    {
      gfloat re1 = data1[i].re;
      gfloat im1 = data1[i].im;
      gfloat re2 = data2[i].re;
      gfloat im2 = data2[i].im;

      gfloat re = re1 * re2 - im1 * -im2;
      gfloat im = re1 * -im2 + im1 * re2;
      gfloat phase = atan2f (im, re);

      doa[i].angle = asinf (phase * phase_range);
      doa[i].distance = i * step;
      doa[i].amplitude = sqrtf (re1 * re1 + im1 * im1) * sqrtf (re2 * re2 + im2 * im2);
    }
}
