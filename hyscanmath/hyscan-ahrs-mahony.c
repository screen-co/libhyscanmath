/* hyscan-ahrs-mahony.c
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

/**
 * SECTION: hyscan-ahrs-mahony
 * @Short_description: реализация фильтра Mahony для датчика курсовертикали
 * @Title: HyScanAHRSMahony
 *
 * Класс содержит реализацию фильтра Mahony для датчика кусовертикали в
 * исполнении Sebastian Madgwick. Класс реализует интерфейс #HyScanAHRS.
 *
 * Для управления параметрами фильтра используется функция
 * #hyscan_ahrs_mahony_set_gains.
 */

#include "hyscan-ahrs-mahony.h"

#include <math.h>

#define DEFAULT_KP     5.0f
#define DEFAULT_KI     0.001f

enum
{
  PROP_O,
  PROP_SAMPLE_RATE
};

struct _HyScanAHRSMahonyPrivate
{
  gfloat       inv_sample_rate;        /* 1.0 / Частота дискретизации. */

  gfloat       two_kp;                 /* 2.0 * proportional gain (Kp) */
  gfloat       two_ki;                 /* 2.0 * integral gain (Ki) */

  gfloat       w0;                     /* Кватернион текущей ориентации. */
  gfloat       w1;
  gfloat       w2;
  gfloat       w3;

  gfloat       integral_fbx;           /* integral error terms scaled by Ki */
  gfloat       integral_fby;
  gfloat       integral_fbz;
};

static void    hyscan_ahrs_mahony_interface_init           (HyScanAHRSInterface   *iface);
static void    hyscan_ahrs_mahony_set_property             (GObject               *object,
                                                            guint                  prop_id,
                                                            const GValue          *value,
                                                            GParamSpec            *pspec);
static void    hyscan_ahrs_mahony_object_constructed       (GObject               *object);

static void    hyscan_ahrs_mahony_update                   (HyScanAHRS            *ahrs,
                                                            gfloat                 gx,
                                                            gfloat                 gy,
                                                            gfloat                 gz,
                                                            gfloat                 ax,
                                                            gfloat                 ay,
                                                            gfloat                 az,
                                                            gfloat                 mx,
                                                            gfloat                 my,
                                                            gfloat                 mz);

static void    hyscan_ahrs_mahony_update_imu               (HyScanAHRS            *ahrs,
                                                            gfloat                 gx,
                                                            gfloat                 gy,
                                                            gfloat                 gz,
                                                            gfloat                 ax,
                                                            gfloat                 ay,
                                                            gfloat                 az);

static HyScanAHRSAngles
               hyscan_ahrs_mahony_get_angles               (HyScanAHRS            *ahrs);

G_DEFINE_TYPE_WITH_CODE (HyScanAHRSMahony, hyscan_ahrs_mahony, G_TYPE_OBJECT,
                         G_ADD_PRIVATE (HyScanAHRSMahony)
                         G_IMPLEMENT_INTERFACE (HYSCAN_TYPE_AHRS, hyscan_ahrs_mahony_interface_init))

static void
hyscan_ahrs_mahony_class_init (HyScanAHRSMahonyClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = hyscan_ahrs_mahony_set_property;
  object_class->constructed = hyscan_ahrs_mahony_object_constructed;

  g_object_class_install_property (object_class, PROP_SAMPLE_RATE,
    g_param_spec_float ("sample-rate", "SampleRrate", "Sample rate", 1.0f, 10000.0f, 10.0f,
                      G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
}

static void
hyscan_ahrs_mahony_init (HyScanAHRSMahony *mahony)
{
  mahony->priv = hyscan_ahrs_mahony_get_instance_private (mahony);
}

static void
hyscan_ahrs_mahony_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  HyScanAHRSMahony *mahony = HYSCAN_AHRS_MAHONY (object);
  HyScanAHRSMahonyPrivate *priv = mahony->priv;

  switch (prop_id)
    {
    case PROP_SAMPLE_RATE:
      priv->inv_sample_rate = 1.0f / g_value_get_float (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
hyscan_ahrs_mahony_object_constructed (GObject *object)
{
  HyScanAHRSMahony *mahony = HYSCAN_AHRS_MAHONY (object);
  HyScanAHRSMahonyPrivate *priv = mahony->priv;

  priv->two_kp = 2.0f * DEFAULT_KP;
  priv->two_ki = 2.0f * DEFAULT_KI;

  priv->w0 = 1.0f;
  priv->w1 = 0.0f;
  priv->w2 = 0.0f;
  priv->w3 = 0.0f;
}

static inline gfloat
hyscan_ahrs_mahony_inv_sqrt (gfloat x)
{
  const gfloat x2 = x * 0.5f;
  const gfloat threehalfs = 1.5f;
  union {
    gfloat f;
    guint32 i;
  } conv = {x};

  conv.i  = 0x5f3759df - ( conv.i >> 1 );
  conv.f  *= ( threehalfs - ( x2 * conv.f * conv.f ) );
  conv.f  *= ( threehalfs - ( x2 * conv.f * conv.f ) );

  return conv.f;
}

/**
 * hyscan_ahrs_mahony_new:
 * @sample_rate: чвстота дискретизации данных
 *
 * Функция создаёт новый объект #HyScanAHRSMahony.
 *
 * Returns: #HyScanAHRSMahony. Для удаления #g_object_unref.
 */
HyScanAHRSMahony *
hyscan_ahrs_mahony_new (gfloat sample_rate)
{
  return g_object_new (HYSCAN_TYPE_AHRS_MAHONY,
                       "sample-rate", sample_rate,
                       NULL);
}

/**
 * hyscan_ahrs_mahony_set_gains:
 * @mahony: указатель на #HyScanAHRSMahony
 * @kp: proportional gain
 * @ki: integral gain
 *
 * Функция задаёт коэффициенты фильтра Kp и Ki.
 */
void
hyscan_ahrs_mahony_set_gains (HyScanAHRSMahony *mahony,
                              gfloat            kp,
                              gfloat            ki)
{
  g_return_if_fail (HYSCAN_IS_AHRS_MAHONY (mahony));

  mahony->priv->two_kp = 2.0f * kp;
  mahony->priv->two_ki = 2.0f * ki;
}

static void
hyscan_ahrs_mahony_reset (HyScanAHRS *ahrs)
{
  HyScanAHRSMahony *mahony = HYSCAN_AHRS_MAHONY (ahrs);
  HyScanAHRSMahonyPrivate *priv = mahony->priv;

  priv->w0 = 1.0f;
  priv->w1 = 0.0f;
  priv->w2 = 0.0f;
  priv->w3 = 0.0f;

  priv->integral_fbx = 0.0f;
  priv->integral_fby = 0.0f;
  priv->integral_fbz = 0.0f;
}

static void
hyscan_ahrs_mahony_update (HyScanAHRS *ahrs,
                           gfloat      gx,
                           gfloat      gy,
                           gfloat      gz,
                           gfloat      ax,
                           gfloat      ay,
                           gfloat      az,
                           gfloat      mx,
                           gfloat      my,
                           gfloat      mz)
{
  HyScanAHRSMahony *mahony = HYSCAN_AHRS_MAHONY (ahrs);
  HyScanAHRSMahonyPrivate *priv = mahony->priv;

  gfloat w0, w1, w2, w3;
  gfloat w0w0, w0w1, w0w2, w0w3, w1w1, w1w2, w1w3, w2w2, w2w3, w3w3;
  gfloat hx, hy, bx, bz;
  gfloat halfvx, halfvy, halfvz, halfwx, halfwy, halfwz;
  gfloat halfex, halfey, halfez;
  gfloat qa, qb, qc;
  gfloat recip_norm;

  /* Use IMU algorithm if magnetometer measurement invalid
   * (avoids NaN in magnetometer normalisation) */
  if ((mx == 0.0f) && (my == 0.0f) && (mz == 0.0f))
    {
      hyscan_ahrs_mahony_update_imu (ahrs, gx, gy, gz, ax, ay, az);
      return;
    }

  /* Caching */
  w0 = priv->w0;
  w1 = priv->w1;
  w2 = priv->w2;
  w3 = priv->w3;

  /* Compute feedback only if accelerometer measurement valid
   * (avoids NaN in accelerometer normalisation) */
  if (!((ax == 0.0f) && (ay == 0.0f) && (az == 0.0f)))
    {
      /* Normalise accelerometer measurement */
      recip_norm = hyscan_ahrs_mahony_inv_sqrt (ax * ax + ay * ay + az * az);
      ax *= recip_norm;
      ay *= recip_norm;
      az *= recip_norm;

      /* Normalise magnetometer measurement */
      recip_norm = hyscan_ahrs_mahony_inv_sqrt (mx * mx + my * my + mz * mz);
      mx *= recip_norm;
      my *= recip_norm;
      mz *= recip_norm;

      /* Auxiliary variables to avoid repeated arithmetic */
      w0w0 = w0 * w0;
      w0w1 = w0 * w1;
      w0w2 = w0 * w2;
      w0w3 = w0 * w3;
      w1w1 = w1 * w1;
      w1w2 = w1 * w2;
      w1w3 = w1 * w3;
      w2w2 = w2 * w2;
      w2w3 = w2 * w3;
      w3w3 = w3 * w3;

      /* Reference direction of Earth's magnetic field */
      hx = 2.0f * (mx * (0.5f - w2w2 - w3w3) + my * (w1w2 - w0w3) + mz * (w1w3 + w0w2));
      hy = 2.0f * (mx * (w1w2 + w0w3) + my * (0.5f - w1w1 - w3w3) + mz * (w2w3 - w0w1));
      bx = sqrtf (hx * hx + hy * hy);
      bz = 2.0f * (mx * (w1w3 - w0w2) + my * (w2w3 + w0w1) + mz * (0.5f - w1w1 - w2w2));

      /* Estimated direction of gravity and magnetic field */
      halfvx = w1w3 - w0w2;
      halfvy = w0w1 + w2w3;
      halfvz = w0w0 - 0.5f + w3w3;
      halfwx = bx * (0.5f - w2w2 - w3w3) + bz * (w1w3 - w0w2);
      halfwy = bx * (w1w2 - w0w3) + bz * (w0w1 + w2w3);
      halfwz = bx * (w0w2 + w1w3) + bz * (0.5f - w1w1 - w2w2);

      /* Error is sum of cross product between estimated direction and
       * measured direction of field vectors */
      halfex = (ay * halfvz - az * halfvy) + (my * halfwz - mz * halfwy);
      halfey = (az * halfvx - ax * halfvz) + (mz * halfwx - mx * halfwz);
      halfez = (ax * halfvy - ay * halfvx) + (mx * halfwy - my * halfwx);

      /* Compute and apply integral feedback if enabled */
      if (priv->two_ki > 0.0f)
        {
          /* integral error scaled by Ki */
          priv->integral_fbx += priv->two_ki * halfex * priv->inv_sample_rate;
          priv->integral_fby += priv->two_ki * halfey * priv->inv_sample_rate;
          priv->integral_fbz += priv->two_ki * halfez * priv->inv_sample_rate;

          /* apply integral feedback */
          gx += priv->integral_fbx;
          gy += priv->integral_fby;
          gz += priv->integral_fbz;
        }
      else
        {
          /* prevent integral windup */
          priv->integral_fbx = 0.0f;
          priv->integral_fby = 0.0f;
          priv->integral_fbz = 0.0f;
        }

      /* Apply proportional feedback */
      gx += priv->two_kp * halfex;
      gy += priv->two_kp * halfey;
      gz += priv->two_kp * halfez;
  }

  /* Integrate rate of change of quaternion */

  /* pre-multiply common factors */
  gx *= (0.5f * priv->inv_sample_rate);
  gy *= (0.5f * priv->inv_sample_rate);
  gz *= (0.5f * priv->inv_sample_rate);
  qa = w0;
  qb = w1;
  qc = w2;
  w0 += (-qb * gx - qc * gy - w3 * gz);
  w1 += (qa * gx + qc * gz - w3 * gy);
  w2 += (qa * gy - qb * gz + w3 * gx);
  w3 += (qa * gz + qb * gy - qc * gx);

  /* Normalise quaternion */
  recip_norm = hyscan_ahrs_mahony_inv_sqrt (w0 * w0 + w1 * w1 + w2 * w2 + w3 * w3);
  w0 *= recip_norm;
  w1 *= recip_norm;
  w2 *= recip_norm;
  w3 *= recip_norm;

  /* Update quaternion */
  priv->w0 = w0;
  priv->w1 = w1;
  priv->w2 = w2;
  priv->w3 = w3;
}

static void
hyscan_ahrs_mahony_update_imu (HyScanAHRS *ahrs,
                               gfloat      gx,
                               gfloat      gy,
                               gfloat      gz,
                               gfloat      ax,
                               gfloat      ay,
                               gfloat      az)
{
  HyScanAHRSMahony *mahony = HYSCAN_AHRS_MAHONY (ahrs);
  HyScanAHRSMahonyPrivate *priv = mahony->priv;

  gfloat w0, w1, w2, w3;
  gfloat halfvx, halfvy, halfvz;
  gfloat halfex, halfey, halfez;
  gfloat qa, qb, qc;
  gfloat recip_norm;

  /* Caching */
  w0 = priv->w0;
  w1 = priv->w1;
  w2 = priv->w2;
  w3 = priv->w3;

  /* Compute feedback only if accelerometer measurement valid
   * (avoids NaN in accelerometer normalisation) */
  if(!((ax == 0.0f) && (ay == 0.0f) && (az == 0.0f)))
    {
      /* Normalise accelerometer measurement */
      recip_norm = hyscan_ahrs_mahony_inv_sqrt (ax * ax + ay * ay + az * az);
      ax *= recip_norm;
      ay *= recip_norm;
      az *= recip_norm;

      /* Estimated direction of gravity and vector perpendicular to magnetic flux */
      halfvx = w1 * w3 - w0 * w2;
      halfvy = w0 * w1 + w2 * w3;
      halfvz = w0 * w0 - 0.5f + w3 * w3;

      /* Error is sum of cross product between estimated and measured direction of gravity */
      halfex = (ay * halfvz - az * halfvy);
      halfey = (az * halfvx - ax * halfvz);
      halfez = (ax * halfvy - ay * halfvx);

      /* Compute and apply integral feedback if enabled */
      if (priv->two_ki > 0.0f)
        {
          /* integral error scaled by Ki */
          priv->integral_fbx += priv->two_ki * halfex * priv->inv_sample_rate;
          priv->integral_fby += priv->two_ki * halfey * priv->inv_sample_rate;
          priv->integral_fbz += priv->two_ki * halfez * priv->inv_sample_rate;

          /* apply integral feedback */
          gx += priv->integral_fbx;
          gy += priv->integral_fby;
          gz += priv->integral_fbz;
        }
      else
        {
          /* prevent integral windup */
          priv->integral_fbx = 0.0f;
          priv->integral_fby = 0.0f;
          priv->integral_fbz = 0.0f;
        }

      /* Apply proportional feedback */
      gx += priv->two_kp * halfex;
      gy += priv->two_kp * halfey;
      gz += priv->two_kp * halfez;
  }

  /* Integrate rate of change of quaternion */

  /* pre-multiply common factors */
  gx *= (0.5f * priv->inv_sample_rate);
  gy *= (0.5f * priv->inv_sample_rate);
  gz *= (0.5f * priv->inv_sample_rate);
  qa = w0;
  qb = w1;
  qc = w2;
  w0 += (-qb * gx - qc * gy - w3 * gz);
  w1 += (qa * gx + qc * gz - w3 * gy);
  w2 += (qa * gy - qb * gz + w3 * gx);
  w3 += (qa * gz + qb * gy - qc * gx);

  /* Normalise quaternion */
  recip_norm = hyscan_ahrs_mahony_inv_sqrt (w0 * w0 + w1 * w1 + w2 * w2 + w3 * w3);
  w0 *= recip_norm;
  w1 *= recip_norm;
  w2 *= recip_norm;
  w3 *= recip_norm;

  /* Update quaternion */
  priv->w0 = w0;
  priv->w1 = w1;
  priv->w2 = w2;
  priv->w3 = w3;
}

static HyScanAHRSAngles
hyscan_ahrs_mahony_get_angles (HyScanAHRS *ahrs)
{
  HyScanAHRSMahony *mahony = HYSCAN_AHRS_MAHONY (ahrs);
  HyScanAHRSMahonyPrivate *priv = mahony->priv;

  HyScanAHRSAngles angles;

  gfloat w0 = priv->w0;
  gfloat w1 = priv->w1;
  gfloat w2 = priv->w2;
  gfloat w3 = priv->w3;

  angles.heading = atan2f (w1*w2 + w0*w3, 0.5f - w2*w2 - w3*w3);
  angles.roll = -atan2f (w0*w1 + w2*w3, 0.5f - w1*w1 - w2*w2);
  angles.pitch = -asinf (-2.0f * (w1*w3 - w0*w2));

  return angles;
}

static void
hyscan_ahrs_mahony_interface_init (HyScanAHRSInterface *iface)
{
  iface->reset = hyscan_ahrs_mahony_reset;
  iface->update = hyscan_ahrs_mahony_update;
  iface->update_imu = hyscan_ahrs_mahony_update_imu;
  iface->get_angles = hyscan_ahrs_mahony_get_angles;
}
