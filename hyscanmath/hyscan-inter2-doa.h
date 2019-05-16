/* hyscan-inter2-doa.h
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

#ifndef __HYSCAN_INTER2_DOA_H__
#define __HYSCAN_INTER2_DOA_H__

#include <hyscan-types.h>

G_BEGIN_DECLS

#define HYSCAN_TYPE_INTER2_DOA             (hyscan_inter2_doa_get_type ())
#define HYSCAN_INTER2_DOA(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), HYSCAN_TYPE_INTER2_DOA, HyScanInter2DOA))
#define HYSCAN_IS_INTER2_DOA(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HYSCAN_TYPE_INTER2_DOA))
#define HYSCAN_INTER2_DOA_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), HYSCAN_TYPE_INTER2_DOA, HyScanInter2DOAClass))
#define HYSCAN_IS_INTER2_DOA_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), HYSCAN_TYPE_INTER2_DOA))
#define HYSCAN_INTER2_DOA_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), HYSCAN_TYPE_INTER2_DOA, HyScanInter2DOAClass))

typedef struct _HyScanInter2DOA HyScanInter2DOA;
typedef struct _HyScanInter2DOAPrivate HyScanInter2DOAPrivate;
typedef struct _HyScanInter2DOAClass HyScanInter2DOAClass;

struct _HyScanInter2DOA
{
  GObject parent_instance;

  HyScanInter2DOAPrivate *priv;
};

struct _HyScanInter2DOAClass
{
  GObjectClass parent_class;
};

HYSCAN_API
GType              hyscan_inter2_doa_get_type  (void);

HYSCAN_API
HyScanInter2DOA *  hyscan_inter2_doa_new       (void);

HYSCAN_API
void               hyscan_inter2_doa_configure (HyScanInter2DOA           *inter2doa,
                                                gdouble                    signal_frequency,
                                                gdouble                    antenna_base,
                                                gdouble                    data_rate,
                                                gdouble                    sound_velocity);

HYSCAN_API
gdouble            hyscan_inter2_doa_get_alpha (HyScanInter2DOA           *inter2doa);

HYSCAN_API
void               hyscan_inter2_doa_get       (HyScanInter2DOA           *inter2doa,
                                                HyScanDOA                 *doa,
                                                const HyScanComplexFloat  *data1,
                                                const HyScanComplexFloat  *data2,
                                                guint32                    n_points);

G_END_DECLS

#endif /* __HYSCAN_INTER2_DOA_H__ */
