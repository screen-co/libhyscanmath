/* hyscan-signal.h
 *
 * Copyright 2016-2019 Screen LLC, Andrei Fadeev <andrei@webcontrol.ru>
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

#ifndef __HYSCAN_SIGNAL_H__
#define __HYSCAN_SIGNAL_H__

#include <hyscan-types.h>

G_BEGIN_DECLS

HYSCAN_API
HyScanComplexFloat    *hyscan_signal_image_tone        (gdouble                disc_freq,
                                                        gdouble                signal_freq,
                                                        gdouble                duration,
                                                        guint                 *n_points);

HYSCAN_API
HyScanComplexFloat    *hyscan_signal_image_lfm         (gdouble                disc_freq,
                                                        gdouble                start_freq,
                                                        gdouble                end_freq,
                                                        gdouble                duration,
                                                        guint                 *n_points);

G_END_DECLS

#endif /* __HYSCAN_SIGNAL_H__ */
