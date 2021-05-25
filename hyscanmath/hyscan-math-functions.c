/* hyscan-math-functions.c
 *
 * Copyright 2018-2019 Screen LLC, Maxim Sidorenko <sidormax@mail.ru>
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
 * SECTION: hyscan-math-functions
 * @Short_description: функции математических обработок над данными массива.
 *
 * Функции #hyscan_math_function_avr, #hyscan_math_function_avr_square,
 * #hyscan_math_function_avr_square_dev, #hyscan_math_function_min,
 * #hyscan_math_function_max возвращают результат в виде структуры HyScanComplexFloat,
 * например после выполнения функции #hyscan_math_function_max над массивом действительных
 * чисел gfloat* в поле re возвращаемой структуры будет находится максимальное 
 * значение, в поле im будет 0. После применения этой же функции над массивом 
 * комплексных чисел в поле re - максимальное значение re составляющих массива,
 * в поле im - максимальное значение im составляющих массива.
 *
 * Функция #hyscan_math_function_phase_diff предназначена для расчета разности фаз
 * двух комплексных чисел.
 */

#include "hyscan-math-functions.h"
#include <math.h>

/**
 * hyscan_math_function_avr:
 * @buffer: буфер с входными данными;
 * @min: начало расчетного диапазона в массиве;
 * @max: конец расчетного диапазона в массиве;
 *
 * Функция возвращает среднее арифметическое значение чисел массива real_data 
 * (результат будет находиться в поле re результирующей структуры, поле im равно 0)
 * или структуру со средним арифметическим значением по каждой из компонент 
 * для complex_data в диапазоне min - max.
 *
 * Returns: структура #HyScanComplexFloat. При отсутствии данных или невалидном диапазоне
 * возвращает структуру #HyScanComplexFloat с нулевыми значениями.
 */
HyScanComplexFloat
hyscan_math_function_avr (HyScanBuffer *buffer,
                          gint64        min,
                          gint64        max)
{
  guint i;
  gfloat sum_re, sum_im;
  HyScanComplexFloat result;
  HyScanComplexFloat *complex_data;
  gfloat *real_data;
  guint32 n_values;

  result.re = 0;
  result.im = 0;
  sum_re = 0;
  sum_im = 0;
  
  if ((hyscan_buffer_get_data_size (buffer)) == 0)
    return result;

  /* Обработка массива с действительными числами.*/
  if (hyscan_buffer_get_data_type (buffer) == HYSCAN_DATA_FLOAT)
    {
      real_data = hyscan_buffer_get_float (buffer, &n_values);
      
      if (min < 0 || min > n_values || max <= min || max > n_values)
        return result;
      
      for (i = min; i < max; ++i)
        sum_re += real_data[i];

      result.re = sum_re / (max - min);
      result.im = 0;
    }
  /* Обработка массива с комплексными числами.*/
  else
    {
      complex_data = hyscan_buffer_get_complex_float (buffer, &n_values);

      if (min < 0 || min > n_values || max <= min || max > n_values)
        return result;

      for (i = min; i < max; ++i)
        {
          sum_re += complex_data[i].re;
          sum_im += complex_data[i].im;
        }

      result.re = sum_re / (max - min);
      result.im = sum_im / (max - min);
    }

  return result;
}

/**
 * hyscan_math_function_avr_square:
 * @buffer: буфер с входными данными;
 * @min: начало расчетного диапазона в массиве;
 * @max: конец расчетного диапазона в массиве;
 *
 * Функция возвращает среднеквадратичное значение чисел массива real_data 
 * (результат будет находиться в поле re результирующей структуры, поле im равно 0)
 * или структуру со среднеквадратичным значением чисел по каждой из компонент для complex_data.
 *
 * Returns: структура #HyScanComplexFloat. При отсутствии данных или невалидном диапазоне
 * возвращает структуру #HyScanComplexFloat с нулевыми значениями.
 */
HyScanComplexFloat
hyscan_math_function_avr_square (HyScanBuffer *buffer,
                                 gint64        min,
                                 gint64        max)
{
  guint i;
  gfloat sum_re, sum_im;
  HyScanComplexFloat result;
  HyScanComplexFloat *complex_data;
  gfloat *real_data;
  guint32 n_values;

  result.re = 0;
  result.im = 0;
  sum_re   = 0;
  sum_im   = 0;

  if ((hyscan_buffer_get_data_size (buffer)) == 0)
    return result;
  
  /* Обработка массива с действительными числами.*/  
  if (hyscan_buffer_get_data_type (buffer) == HYSCAN_DATA_FLOAT)
    {
      real_data = hyscan_buffer_get_float (buffer, &n_values);
      
      if (min < 0 || min > n_values || max <= min || max > n_values)
        return result;
      
      for (i = min; i < max; ++i)
        sum_re += pow (real_data[i], 2);

      result.re = sqrtf (sum_re / (max - min));
      result.im = 0;      
    }
  /* Обработка массива с комплексными числами.*/  
  else
    {
      complex_data = hyscan_buffer_get_complex_float (buffer, &n_values);

      if (min < 0 || min > n_values || max <= min || max > n_values)
        return result;

      for (i = min; i < max; ++i)
        {
          sum_re += pow (complex_data[i].re, 2);
          sum_im += pow (complex_data[i].im, 2);
        }

      result.re = sqrtf (sum_re / (max - min));
      result.im = sqrtf (sum_im / (max - min));
    }

  return result;
}

/**
 * hyscan_math_function_avr_square_dev:
 * @buffer: буфер с входными данными;
 * @min: начало расчетного диапазона в массиве;
 * @max: конец расчетного диапазона в массиве;
 *
 * Функция возвращает среднеквадратичное отклонение значений массива real_data 
 * (результат будет находиться в поле re результирующей структуры, поле im равно 0)
 * или структуру со среднеквадратичным отклонением значений по каждой из компонент для complex_data.
 *
 * Returns: структура #HyScanComplexFloat. При отсутствии данных или невалидном диапазоне
 * возвращает структуру #HyScanComplexFloat с нулевыми значениями.
 */
HyScanComplexFloat
hyscan_math_function_avr_square_dev (HyScanBuffer *buffer,
                                     gint64        min,
                                     gint64        max)
{
  guint i;
  HyScanComplexFloat avr, result;
  HyScanComplexFloat *complex_data;
  gfloat *real_data;
  guint32 n_values;

  result.re = 0;
  result.im = 0;
  gfloat sum_re = 0;
  gfloat sum_im = 0;

  if ((hyscan_buffer_get_data_size (buffer)) == 0)
    return result;

  avr = hyscan_math_function_avr (buffer, min, max);
  
  /* Обработка массива с действительными числами.*/
  if (hyscan_buffer_get_data_type (buffer) == HYSCAN_DATA_FLOAT)
    {
      real_data = hyscan_buffer_get_float (buffer, &n_values);
      
      if (min < 0 || min > n_values || max <= min || max > n_values)
        return result;

      
      for (i = min; i < max; ++i)
        sum_re += pow (real_data[i] - avr.re, 2);

      result.re = sqrtf (sum_re / (max - min));
      result.im = 0;
    }
  /* Обработка массива с комплексными числами.*/  
  else
    {
      complex_data = hyscan_buffer_get_complex_float (buffer, &n_values);

      if (min < 0 || min > n_values || max <= min || max > n_values)
        return result;

      for (i = min; i < max; ++i)
        {
          sum_re += pow (complex_data[i].re - avr.re, 2);
          sum_im += pow (complex_data[i].im - avr.im, 2);
        }

      result.re = sqrtf (sum_re / (max - min));
      result.im = sqrtf (sum_im / (max - min));
    }

  return result;
}

/**
 * hyscan_math_function_min:
 * @buffer: буфер с входными данными;
 * @min: начало расчетного диапазона в массиве;
 * @max: конец расчетного диапазона в массиве;
 *
 * Функция возвращает минимальное значение чисел массива real_data 
 * (результат будет находиться в поле re результирующей структуры, поле im равно 0)
 * или структуру с минимальным значением чисел по каждой из компонент для complex_data.
 *
 * Returns: структура #HyScanComplexFloat. При отсутствии данных или невалидном диапазоне
 * возвращает структуру #HyScanComplexFloat с нулевыми значениями.
 */
HyScanComplexFloat
hyscan_math_function_min (HyScanBuffer *buffer,
                          gint64        min,
                          gint64        max)
{
  guint i;
  HyScanComplexFloat result;
  HyScanComplexFloat *complex_data;
  gfloat *real_data;
  guint32 n_values;

  result.re = 0;
  result.im = 0;
  gfloat min_re_value = 0;
  gfloat min_im_value = 0;

  if ((hyscan_buffer_get_data_size (buffer)) == 0)
    return result;  

  /* Обработка массива с действительными числами.*/
  if (hyscan_buffer_get_data_type (buffer) == HYSCAN_DATA_FLOAT)
    {
      real_data = hyscan_buffer_get_float (buffer, &n_values);

      if (min < 0 || min > n_values || max <= min || max > n_values)
        return result;

      min_re_value = real_data[min];

      for (i = (min + 1); i < max; ++i)
        if (real_data[i] < min_re_value)
          min_re_value = real_data[i];

      result.re = min_re_value;
      result.im = 0;
    }
  /* Обработка массива с комплексными числами.*/ 
  else
    {
      complex_data = hyscan_buffer_get_complex_float (buffer, &n_values);

      if (min < 0 || min > n_values || max <= min || max > n_values)
        return result;

      min_re_value = complex_data[min].re;
      min_im_value = complex_data[min].im;

      for (i = (min + 1); i < max; ++i)
        {
          if (complex_data[i].re < min_re_value)
            min_re_value = complex_data[i].re;
          
          if (complex_data[i].im < min_im_value)
            min_im_value = complex_data[i].im;
        }

      result.re = min_re_value;
      result.im = min_im_value;
    }

  return result;
}

/**
 * hyscan_math_function_max:
 * @buffer: буфер с входными данными;
 * @min: начало расчетного диапазона в массиве;
 * @max: конец расчетного диапазона в массиве;
 *
 * Функция возвращает максимальное значение чисел массива real_data 
 * (результат будет находиться в поле re результирующей структуры, поле im равно 0)
 * или структуру с максимальным значением чисел по каждой из компонент для complex_data.
 *
 * Returns: структура #HyScanComplexFloat. При отсутствии данных или невалидном диапазоне
 * возвращает структуру #HyScanComplexFloat с нулевыми значениями.
 */
HyScanComplexFloat
hyscan_math_function_max (HyScanBuffer *buffer,
                          gint64        min,
                          gint64        max)
{
  guint i;
  gfloat max_re_value, max_im_value;
  HyScanComplexFloat result;
  HyScanComplexFloat *complex_data;
  gfloat *real_data;
  guint32 n_values;

  result.re = 0;
  result.im = 0;

  if ((hyscan_buffer_get_data_size (buffer)) == 0)
    return result; 

  /* Обработка массива с действительными числами.*/
  if (hyscan_buffer_get_data_type (buffer) == HYSCAN_DATA_FLOAT)
    {
      real_data = hyscan_buffer_get_float (buffer, &n_values);

      if (min < 0 || min > n_values || max <= min || max > n_values)
        return result;

      max_re_value = real_data[min];

      for (i = (min + 1); i < max; ++i)
        if (real_data[i] > max_re_value)
          max_re_value = real_data[i];

      result.re = max_re_value;
      result.im = 0;
    }
  /* Обработка массива с комплексными числами.*/
  else
    {
      complex_data = hyscan_buffer_get_complex_float (buffer, &n_values);

      if (min < 0 || min > n_values || max <= min || max > n_values)
        return result;

      max_re_value = complex_data[min].re;
      max_im_value = complex_data[min].im;

      for (i = (min + 1); i < max; ++i)
        {
          if (complex_data[i].re > max_re_value)
            max_re_value = complex_data[i].re;
          
          if (complex_data[i].im > max_im_value)
            max_im_value = complex_data[i].im;
        }

      result.re = max_re_value;
      result.im = max_im_value;
    }

  return result;
}

/**
 * hyscan_math_function_phase_diff:
 * @value1: первое комплексное число;
 * @value2: второе комплексное число;
 *
 * Функция возвращает разность фаз двух комплексных чисел.
 *
 * Returns: значение gdouble.
 */
gfloat
hyscan_math_function_phase_diff (HyScanComplexFloat value1,
                                 HyScanComplexFloat value2)
{
  gfloat re, im, phase;

  re = value1.re * value2.re - value1.im * -value2.im;
  im = value1.re * -value2.im + value1.im * value2.re;
  phase = atan2f (im, re);

  return phase;
}
