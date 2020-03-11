/* hyscan-convolution.h
 *
 * Copyright 2015-2019 Screen LLC, Andrei Fadeev <andrei@webcontrol.ru>
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

#ifndef __HYSCAN_CONVOLUTION_H__
#define __HYSCAN_CONVOLUTION_H__

#include <glib-object.h>
#include <hyscan-types.h>

G_BEGIN_DECLS

#define HYSCAN_TYPE_CONVOLUTION             (hyscan_convolution_get_type ())
#define HYSCAN_CONVOLUTION(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), HYSCAN_TYPE_CONVOLUTION, HyScanConvolution))
#define HYSCAN_IS_CONVOLUTION(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HYSCAN_TYPE_CONVOLUTION))
#define HYSCAN_CONVOLUTION_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), HYSCAN_TYPE_CONVOLUTION, HyScanConvolutionClass))
#define HYSCAN_IS_CONVOLUTION_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), HYSCAN_TYPE_CONVOLUTION))
#define HYSCAN_CONVOLUTION_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), HYSCAN_TYPE_CONVOLUTION, HyScanConvolutionClass))

typedef struct _HyScanConvolution HyScanConvolution;
typedef struct _HyScanConvolutionPrivate HyScanConvolutionPrivate;
typedef struct _HyScanConvolutionClass HyScanConvolutionClass;

struct _HyScanConvolution
{
  GObject parent_instance;

  HyScanConvolutionPrivate *priv;
};

struct _HyScanConvolutionClass
{
  GObjectClass parent_class;
};

HYSCAN_API
GType               hyscan_convolution_get_type       (void);

HYSCAN_API
HyScanConvolution * hyscan_convolution_new            (void);

HYSCAN_API
guint32             hyscan_convolution_get_fft_size   (guint32                    size);

HYSCAN_API
gboolean            hyscan_convolution_set_image_td   (HyScanConvolution         *convolution,
                                                       guint                      index,
                                                       const HyScanComplexFloat  *image,
                                                       guint32                    n_points);

HYSCAN_API
gboolean            hyscan_convolution_set_image_fd   (HyScanConvolution         *convolution,
                                                       guint                      index,
                                                       const HyScanComplexFloat  *image,
                                                       guint32                    n_points);

HYSCAN_API
gboolean            hyscan_convolution_convolve       (HyScanConvolution         *convolution,
                                                       guint                      index,
                                                       HyScanComplexFloat        *data,
                                                       guint32                    n_points,
                                                       gfloat                     scale);

G_END_DECLS

#endif /* __HYSCAN_CONVOLUTION_H__ */
