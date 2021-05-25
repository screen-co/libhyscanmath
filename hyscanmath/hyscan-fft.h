/* hyscan-fft.h
 *
 * Copyright 2020 Screen LLC, Maxim Sidorenko <sidormax@mail.ru>
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

#ifndef __HYSCAN_FFT_H__
#define __HYSCAN_FFT_H__

#include <glib-object.h>
#include <hyscan-types.h>

G_BEGIN_DECLS

#define HYSCAN_TYPE_FFT             (hyscan_fft_get_type ())
#define HYSCAN_FFT(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), HYSCAN_TYPE_FFT, HyScanFFT))
#define HYSCAN_IS_FFT(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HYSCAN_TYPE_FFT))
#define HYSCAN_FFT_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), HYSCAN_TYPE_FFT, HyScanFFTClass))
#define HYSCAN_IS_FFT_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), HYSCAN_TYPE_FFT))
#define HYSCAN_FFT_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), HYSCAN_TYPE_FFT, HyScanFFTClass))

typedef struct _HyScanFFT HyScanFFT;
typedef struct _HyScanFFTPrivate HyScanFFTPrivate;
typedef struct _HyScanFFTClass HyScanFFTClass;

/**
 * HyScanFFTType:
 * @HYSCAN_FFT_TYPE_INVALID:        Недопустимый тип, ошибка.
 * @HYSCAN_FFT_TYPE_REAL:           Действительные.
 * @HYSCAN_FFT_TYPE_COMPLEX:        Комплексные.
 *
 * Тип входных данных.
 */
typedef enum
{
  HYSCAN_FFT_TYPE_INVALID,
  HYSCAN_FFT_TYPE_REAL,
  HYSCAN_FFT_TYPE_COMPLEX

} HyScanFFTType;

/**
 * HyScanFFTDirection:
 * @HYSCAN_FFT_DIRECTION_FORWARD:   Прямое.
 * @HYSCAN_FFT_DIRECTION_BACKWARD:  Обратное.
 *
 * Тип преобразования.
 */
typedef enum
{
  HYSCAN_FFT_DIRECTION_FORWARD,
  HYSCAN_FFT_DIRECTION_BACKWARD

} HyScanFFTDirection;


struct _HyScanFFT
{
  GObject parent_instance;

  HyScanFFTPrivate *priv;
};

struct _HyScanFFTClass
{
  GObjectClass parent_class;
};

HYSCAN_API
GType                      hyscan_fft_get_type                  (void);

HYSCAN_API
HyScanFFT *                hyscan_fft_new                       (void);

HYSCAN_API
void                       hyscan_fft_set_transposition         (HyScanFFT                *fft,
                                                                 gboolean                  transposition,
                                                                 gdouble                   signal_frequency,
                                                                 gdouble                   signal_heterodyne,
                                                                 gdouble                   data_rate);

HYSCAN_API
gboolean                   hyscan_fft_transform_real            (HyScanFFT                *fft,
                                                                 HyScanFFTDirection        direction,
                                                                 gfloat                   *data,
                                                                 guint32                   n_points);

HYSCAN_API
gboolean                   hyscan_fft_transform_complex         (HyScanFFT                *fft,
                                                                 HyScanFFTDirection        direction,
                                                                 HyScanComplexFloat       *data,
                                                                 guint32                   n_points);

HYSCAN_API
const gfloat *             hyscan_fft_transform_const_real      (HyScanFFT                *fft,
                                                                 HyScanFFTDirection        direction,
                                                                 const gfloat             *data,
                                                                 guint32                   n_points);

HYSCAN_API
const HyScanComplexFloat * hyscan_fft_transform_const_complex   (HyScanFFT                *fft,
                                                                 HyScanFFTDirection        direction,
                                                                 const HyScanComplexFloat *data,
                                                                 guint32                   n_points);

HYSCAN_API
guint32                    hyscan_fft_get_transform_size        (guint32                   size);

HYSCAN_API
gpointer                   hyscan_fft_alloc                     (HyScanFFTType             type,
                                                                 guint32                   n_points);
HYSCAN_API
void                       hyscan_fft_free                      (gpointer                  data);

G_END_DECLS

#endif /* __HYSCAN_FFT_H__ */
