/* hyscan-ahrs.h
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

#ifndef __HYSCAN_AHRS_H__
#define __HYSCAN_AHRS_H__

#include <hyscan-api.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define HYSCAN_TYPE_AHRS            (hyscan_ahrs_get_type ())
#define HYSCAN_AHRS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), HYSCAN_TYPE_AHRS, HyScanAHRS))
#define HYSCAN_IS_AHRS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HYSCAN_TYPE_AHRS))
#define HYSCAN_AHRS_GET_IFACE(obj)  (G_TYPE_INSTANCE_GET_INTERFACE ((obj), HYSCAN_TYPE_AHRS, HyScanAHRSInterface))

typedef struct _HyScanAHRS HyScanAHRS;
typedef struct _HyScanAHRSInterface HyScanAHRSInterface;
typedef struct _HyScanAHRSAngles HyScanAHRSAngles;

struct _HyScanAHRSInterface
{
  GTypeInterface       g_iface;

  void                 (*reset)                (HyScanAHRS    *ahrs);

  void                 (*update)               (HyScanAHRS    *ahrs,
                                                gfloat         gx,
                                                gfloat         gy,
                                                gfloat         gz,
                                                gfloat         ax,
                                                gfloat         ay,
                                                gfloat         az,
                                                gfloat         mx,
                                                gfloat         my,
                                                gfloat         mz);

  void                 (*update_imu)           (HyScanAHRS    *ahrs,
                                                gfloat         gx,
                                                gfloat         gy,
                                                gfloat         gz,
                                                gfloat         ax,
                                                gfloat         ay,
                                                gfloat         az);

  HyScanAHRSAngles     (*get_angles)           (HyScanAHRS    *ahrs);
};

/**
 * HyScanAHRSAngles:
 * @heading: магнитный курс, радианы
 * @roll: угол крена, радианы
 * @pitch: угол дифферента, радианы
 *
 * Углы ориентации датчика в пространстве.
 */
struct _HyScanAHRSAngles
{
  gfloat               heading;
  gfloat               roll;
  gfloat               pitch;
};

HYSCAN_API
GType                  hyscan_ahrs_get_type    (void);

HYSCAN_API
void                   hyscan_ahrs_reset       (HyScanAHRS    *ahrs);

HYSCAN_API
void                   hyscan_ahrs_update      (HyScanAHRS    *ahrs,
                                                gfloat         gx,
                                                gfloat         gy,
                                                gfloat         gz,
                                                gfloat         ax,
                                                gfloat         ay,
                                                gfloat         az,
                                                gfloat         mx,
                                                gfloat         my,
                                                gfloat         mz);

HYSCAN_API
void                   hyscan_ahrs_update_imu  (HyScanAHRS    *ahrs,
                                                gfloat         gx,
                                                gfloat         gy,
                                                gfloat         gz,
                                                gfloat         ax,
                                                gfloat         ay,
                                                gfloat         az);

HYSCAN_API
HyScanAHRSAngles       hyscan_ahrs_get_angles  (HyScanAHRS    *ahrs);

G_END_DECLS

#endif /* __HYSCAN_AHRS_H__ */
