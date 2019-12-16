/* hyscan-ahrs.c
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
 * SECTION: hyscan-ahrs
 * @Short_description: интерфейс фильтрации данных датчиков курсовертикали
 * @Title: HyScanAHRS
 *
 * Интерфейс обеспечивает унифицированный механизм обработки данных датчиков
 * курсовертикали (AHRS) и гиростабилизатора (IMU).
 *
 * Для полноценной работы датчика необходимы данные от магнитометра, акселерометра
 * и гироскопа (функция #hyscan_ahrs_update). В этом случае доступны все три угла
 * ориентации в пространстве, т.е. датчик работает в режиме курсовертикали.
 *
 * Для режима гиростабилизатора достаточно данных только от акселерометра и
 * гироскопа (функция #hyscan_ahrs_update_imu). В этом случае, курс содержит
 * относительное значение и может иметь ощутимую величину дрейфа.
 *
 * Данные от акселерометра и магнитометра могут иметь относительные значения,
 * при условии их нормирования. Данные гироскопа должны содержать значения
 * угловой скорости в рад/с.
 *
 * При ориентации датчика в пространстве предполагается, что ось X направлена
 * вперёд, ось Y вправо, ось Z вниз. При этом, значения heading и yaw совпадают
 * друг с другом. Положительные значения углов отсчитываются при повороте датчика
 * вокруг соответствующей оси по часовой стрелке. Текущие значения углов в виде
 * структуры #HyScanAHRSAngles, можно получить при помощи функции
 * #hyscan_ahrs_get_angles
 */

#include "hyscan-ahrs.h"

G_DEFINE_INTERFACE (HyScanAHRS, hyscan_ahrs, G_TYPE_OBJECT)

static void
hyscan_ahrs_default_init (HyScanAHRSInterface *iface)
{
}

/**
 * hyscan_ahrs_reset:
 * @ahrs: указатель на #HyScanAHRS
 *
 * Функция сбрасывает фильтр датчика в начальное состояние. Конкретное
 * состояние зависит от реализации фильтра.
 */
void
hyscan_ahrs_reset (HyScanAHRS *ahrs)
{
  HyScanAHRSInterface *iface;

  g_return_if_fail (HYSCAN_IS_AHRS (ahrs));

  iface = HYSCAN_AHRS_GET_IFACE (ahrs);
  if (iface->reset != NULL)
    (* iface->reset) (ahrs);
}

/**
 * hyscan_ahrs_update:
 * @ahrs: указатель на #HyScanAHRS
 * @gx: угловая скорость вокруг оси x, рад/с
 * @gy: угловая скорость вокруг оси y, рад/с
 * @gz: угловая скорость вокруг оси z, рад/с
 * @ax: проекция вектора ускорения на ось x
 * @ay: проекция вектора ускорения на ось y
 * @az: проекция вектора ускорения на ось z
 * @mx: проекция магнитного поля на ось x
 * @my: проекция магнитного поля на ось y
 * @mz: проекция магнитного поля на ось z
 *
 * Функция обновляет состояние фильтра в соответствии с текущими значениями
 * от магнитометра, акселерометра и гироскопа.
 */
void
hyscan_ahrs_update (HyScanAHRS *ahrs,
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
  HyScanAHRSInterface *iface;

  g_return_if_fail (HYSCAN_IS_AHRS (ahrs));

  iface = HYSCAN_AHRS_GET_IFACE (ahrs);
  if (iface->update != NULL)
    (* iface->update) (ahrs, gx, gy, gz, ax, ay, az, mx, my, mz);
}

/**
 * hyscan_ahrs_update_imu:
 * @ahrs: указатель на #HyScanAHRS
 * @gx: угловая скорость вокруг оси x, рад/с
 * @gy: угловая скорость вокруг оси y, рад/с
 * @gz: угловая скорость вокруг оси z, рад/с
 * @ax: проекция вектора ускорения на ось x
 * @ay: проекция вектора ускорения на ось y
 * @az: проекция вектора ускорения на ось z
 *
 * Функция обновляет состояние фильтра в соответствии с текущими значениями
 * от магнитометра и акселерометра.
 */
void
hyscan_ahrs_update_imu (HyScanAHRS *ahrs,
                        gfloat      gx,
                        gfloat      gy,
                        gfloat      gz,
                        gfloat      ax,
                        gfloat      ay,
                        gfloat      az)
{
  HyScanAHRSInterface *iface;

  g_return_if_fail (HYSCAN_IS_AHRS (ahrs));

  iface = HYSCAN_AHRS_GET_IFACE (ahrs);
  if (iface->update_imu != NULL)
    (* iface->update_imu) (ahrs, gx, gy, gz, ax, ay, az);
}

/**
 * hyscan_ahrs_get_angles:
 * @ahrs: указатель на #HyScanAHRS
 *
 * Функция возвращает текущие значения углов ориентации датчика.
 *
 * Returns: значения углов ориентации датчика
 */
HyScanAHRSAngles
hyscan_ahrs_get_angles (HyScanAHRS *ahrs)
{
  HyScanAHRSInterface *iface;
  HyScanAHRSAngles empty = {0.0, 0.0, 0.0};

  g_return_val_if_fail (HYSCAN_IS_AHRS (ahrs), empty);

  iface = HYSCAN_AHRS_GET_IFACE (ahrs);
  if (iface->get_angles != NULL)
    return (* iface->get_angles) (ahrs);

  return empty;
}
