/* hyscan-beamforming.h
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

#ifndef __HYSCAN_BEAMFORMING_H__
#define __HYSCAN_BEAMFORMING_H__

#include <hyscan-types.h>

G_BEGIN_DECLS

#define HYSCAN_TYPE_BEAMFORMING             (hyscan_beamforming_get_type ())
#define HYSCAN_BEAMFORMING(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), HYSCAN_TYPE_BEAMFORMING, HyScanBeamforming))
#define HYSCAN_IS_BEAMFORMING(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HYSCAN_TYPE_BEAMFORMING))
#define HYSCAN_BEAMFORMING_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), HYSCAN_TYPE_BEAMFORMING, HyScanBeamformingClass))
#define HYSCAN_IS_BEAMFORMING_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), HYSCAN_TYPE_BEAMFORMING))
#define HYSCAN_BEAMFORMING_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), HYSCAN_TYPE_BEAMFORMING, HyScanBeamformingClass))

typedef struct _HyScanBeamforming HyScanBeamforming;
typedef struct _HyScanBeamformingPrivate HyScanBeamformingPrivate;
typedef struct _HyScanBeamformingClass HyScanBeamformingClass;

struct _HyScanBeamforming
{
  GObject parent_instance;

  HyScanBeamformingPrivate *priv;
};

struct _HyScanBeamformingClass
{
  GObjectClass parent_class;
};

HYSCAN_API
GType                  hyscan_beamforming_get_type     (void);

HYSCAN_API
HyScanBeamforming *    hyscan_beamforming_new          (void);

HYSCAN_API
gboolean               hyscan_beamforming_configure    (HyScanBeamforming         *bf,
                                                        guint                      n_channels,
                                                        gdouble                    data_rate,
                                                        gdouble                    signal_frequency,
                                                        gdouble                    signal_heterodyne,
                                                        const gdouble             *antenna_offsets,
                                                        const gint                *antenna_groups,
                                                        gdouble                    field_of_view,
                                                        gdouble                    sound_velocity);

HYSCAN_API
gboolean               hyscan_beamforming_set_signals  (HyScanBeamforming         *bf,
                                                        const HyScanComplexFloat **signals,
                                                        guint32                    n_points);

HYSCAN_API
gboolean               hyscan_beamforming_get_doa      (HyScanBeamforming         *bf,
                                                        HyScanDOA                 *doa,
                                                        const HyScanComplexFloat **data,
                                                        guint32                    n_points);

G_END_DECLS

#endif /* __HYSCAN_BEAMFORMING_H__ */
