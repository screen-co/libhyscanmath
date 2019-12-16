/* hyscan-ahrs-mahony.h
 *
 * Copyright 2019 Screen LLC, Andrei Fadeev <andrei@webcontrol.ru>
 *
 * License: LGPL (see: https://code.google.com/archive/p/imumargalgorithm30042010sohm/)
 *
 * Madgwick's implementation of Mayhony's AHRS algorithm.
 * See: http://www.x-io.co.uk/open-source-imu-and-ahrs-algorithms/
 *
 * Date         Author          Notes
 * 29/09/2011   SOH Madgwick    Initial release
 * 02/10/2011   SOH Madgwick    Optimized for reduced CPU load
 *
 * Algorithm paper:
 * http://ieeexplore.ieee.org/xpl/login.jsp?tp=&arnumber=4608934&url=http%3A%2F%2Fieeexplore.ieee.org%2Fstamp%2Fstamp.jsp%3Ftp%3D%26arnumber%3D4608934
 */

#ifndef __HYSCAN_AHRS_MAHONY_H__
#define __HYSCAN_AHRS_MAHONY_H__

#include <hyscan-ahrs.h>

G_BEGIN_DECLS

#define HYSCAN_TYPE_AHRS_MAHONY             (hyscan_ahrs_mahony_get_type ())
#define HYSCAN_AHRS_MAHONY(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), HYSCAN_TYPE_AHRS_MAHONY, HyScanAHRSMahony))
#define HYSCAN_IS_AHRS_MAHONY(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HYSCAN_TYPE_AHRS_MAHONY))
#define HYSCAN_AHRS_MAHONY_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), HYSCAN_TYPE_AHRS_MAHONY, HyScanAHRSMahonyClass))
#define HYSCAN_IS_AHRS_MAHONY_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), HYSCAN_TYPE_AHRS_MAHONY))
#define HYSCAN_AHRS_MAHONY_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), HYSCAN_TYPE_AHRS_MAHONY, HyScanAHRSMahonyClass))

typedef struct _HyScanAHRSMahony HyScanAHRSMahony;
typedef struct _HyScanAHRSMahonyPrivate HyScanAHRSMahonyPrivate;
typedef struct _HyScanAHRSMahonyClass HyScanAHRSMahonyClass;

struct _HyScanAHRSMahony
{
  GObject parent_instance;

  HyScanAHRSMahonyPrivate *priv;
};

struct _HyScanAHRSMahonyClass
{
  GObjectClass parent_class;
};

HYSCAN_API
GType                  hyscan_ahrs_mahony_get_type     (void);

HYSCAN_API
HyScanAHRSMahony *     hyscan_ahrs_mahony_new          (gfloat                 sample_rate);

HYSCAN_API
void                   hyscan_ahrs_mahony_set_gains    (HyScanAHRSMahony      *ahrs,
                                                        gfloat                 kp,
                                                        gfloat                 ki);

G_END_DECLS

#endif /* __HYSCAN_AHRS_MAHONY_H__ */
