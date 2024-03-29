/* hyscan-echo-svp.c
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

#include "hyscan-echo-svp.h"
#include <math.h>

/**
 * hyscan_echo_svp_calc:
 * @time: время приёма эхосигнала, с
 * @voffset: заглубление эхолота, м
 * @svp: (transfer none) (element-type HyScanSoundVelocity): профиль скорости звука
 *
 * Функция рассчитывает глубину с учётом профиля скорости звука.
 *
 * Returns: Глубина в метрах или отрицательное число в случае ошибки в профиле.
 */
gdouble
hyscan_echo_svp_calc (gdouble  time,
                      gdouble  voffset,
                      GList   *svp)
{
  gdouble pvelocity = 750.0;
  gdouble velocity = 750.0;
  gdouble depth = 0.0;

  if (!isfinite (time) || (time < 0.0))
    return -1.0;

  /* Если указано заглубление, пропускаем часть профиля
   * скорости звука до этого заглубления. */
  while (svp != NULL)
    {
      HyScanSoundVelocity *sv = svp->data;

      if (sv->depth > voffset)
        break;

      pvelocity = velocity;
      velocity = sv->velocity / 2.0;

      svp = g_list_next (svp);
    }

  /* Интегрируем пройденный путь с учётом профиля скорости звука. */
  while ((svp != NULL) && (time > 0.0))
    {
      HyScanSoundVelocity *sv = svp->data;
      gdouble st;

      /* Скорость звука в начальной и конечной точках профиля. */
      pvelocity = velocity;
      velocity = 0.5 * sv->velocity;

      /* Время прохождения пути между двумя точками профиля скорости звука,
       * с учётом усреднённой скорости звука. */
      st = (sv->depth - voffset - depth) / (0.5 * (pvelocity + velocity));
      if (st < 0.0)
        {
          g_warning ("HyScanEchoSVP: profile error");
          return -1.0;
        }

      /* Интегрирование по дистанции с учётом оставшегося времени. */
      st = MIN (st, time);
      depth += st * (0.5 * (pvelocity + velocity));
      time -= st;

      svp = g_list_next (svp);
    }

  /* Остаток дистанции за пределами профиля. */
  depth += time * velocity;

  return depth;
}
