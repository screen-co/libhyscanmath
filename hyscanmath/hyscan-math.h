/**
 * \file hyscan-math.h
 *
 * \brief Заголовочный файл общих функций и макросов математической библиотеки
 * \author Vladimir Maximov (vmakxs@gmail.com)
 * \date 2016
 * \license Проприетарная лицензия ООО "Экран"
 */

#ifndef __HYSCAN_MATH_H__
#define __HYSCAN_MATH_H__

#include <glib.h>

/* Преобразует градусы в радианы. */
#define DEGREES_TO_RADIANS(degrees) ((degrees) * G_PI / 180.0)

/* Преобразует радианы в градусы. */
#define RADIANS_TO_DEGREES(radians) ((radians) * 180.0 / G_PI)

#endif /* __HYSCAN_MATH_H__ */
