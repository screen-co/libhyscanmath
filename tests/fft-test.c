 /* fft-test.c
 *
 * Copyright 2020 Screen LLC, Maxim Sidorenko <sidormax@mail.ru>
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

#include <hyscan-fft.h>
#include <hyscan-buffer.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#define ERROR_LIMIT 1E-6

/* Структура хранит информацию о частоте. */
typedef struct
{
  guint32  index;             /* Индекс в результирующем массиве. */
  gdouble  value;             /* Скорректированное значение частоты. */
  gdouble  user_value;        /* Значение частоты введенное пользователем. */
} FrequencyInfo;

/* Структура хранит информацию о параметрах тестируемой функции. */
typedef struct
{
  gchar   *type;              /* Название типа функции. */
  gchar   *name;              /* Название тестируемой функции для отображения. */
  gboolean constant;          /* Признак функции, возвращающей константный результат. */
  gboolean is_complex;        /* Признак функции над комплексными данными. */
  gboolean transposition;     /* Признак применения функции согласования частот. */
} FuncInfo;

/* Таблица с параметрами тестов. */
FuncInfo func_info[] = {
  { "real",                   "FFT test real",                          FALSE, FALSE, FALSE },
  { "complex",                "FFT test complex",                       FALSE, TRUE,  FALSE },
  { "complex_transpos",       "FFT test complex + transposition",       FALSE, TRUE,  TRUE  },
  { "const_real",             "FFT test const real",                    TRUE,  FALSE, FALSE },
  { "const_complex",          "FFT test const complex",                 TRUE,  TRUE,  FALSE },
  { "const_complex_transpos", "FFT test const complex + transposition", TRUE,  TRUE,  TRUE  },
  { NULL }
};

/* Глобальные переменные. */
HyScanFFT *fft;             /* Экземпляр расчитывающий БПФ. */ 
GTimer    *timer;           /* Таймер для замера времени расчета. */ 

gdouble amplitude = 1.0;
gdouble frequency = 100000.0;
gdouble heterodyne = 100000.0;
gdouble discretization = 1000000.0;
guint32 n_points  = 100000;

/* Функция создает тестовые действительные данные. */
gfloat *
create_real_data (GArray  *freq_array,
                  gdouble  amplitude,
                  gdouble  discretization,
                  guint32  n_points,
                  gboolean corrected)
{
  gfloat *data; 
  FrequencyInfo *frequency_info;
  gdouble frequency;
  guint i, j;

  data = g_new0 (gfloat, n_points);
  for (i = 0; i < n_points; ++i)
    {
      gdouble time = (1 / discretization) * i;
      
      for (j = 0; j < freq_array->len; ++j)
        {
          frequency_info = &g_array_index (freq_array, FrequencyInfo, j);
          
          if (corrected)
            frequency = frequency_info->value;
          else
            frequency = frequency_info->user_value;

          gdouble phase = 2.0 * G_PI * frequency * time;
          data[i] += (amplitude * cos (phase));
        }
    }

  return data;  
}

/* Функция создает тестовые комплексные данные. */
HyScanComplexFloat *
create_complex_data (GArray  *freq_array,
                     gdouble  amplitude,
                     gdouble  heterodyne,
                     gdouble  discretization,
                     guint32  n_points,
                     gboolean corrected)
{
  FrequencyInfo *frequency_info;
  HyScanComplexFloat *data; 
  gdouble frequency;
  guint i, j;

  data = g_new0 (HyScanComplexFloat, n_points);
  for (i = 0; i < n_points; ++i)
    {
      gdouble time = (1 / discretization) * i;

      for (j = 0; j < freq_array->len; ++j)
        {
          frequency_info = &g_array_index (freq_array, FrequencyInfo, j);
          
          if (corrected)
            frequency = frequency_info->value;
          else
            frequency = frequency_info->user_value;
          
          gdouble phase = 2.0 * G_PI * (frequency - heterodyne) * time;
          data[i].re += amplitude * cos (phase);
          data[i].im += amplitude * sin (phase);
        }
    }
  
  return data;  
}

/* Функция корректирует значения частоты frequency с учетом частоты дискретизации. 
   Значения частот должны быть в диапазоне [f0 - rate/2; f0 + rate/2 - rate/fft_size] 
   для комплексных и [0; rate/2 - rate/fft_size] для действительных данных. */
gdouble
correct_frequency (gdouble  frequency0,
                   gdouble  frequency,
                   gdouble  discretization,
                   guint32  fft_size,
                   gboolean is_complex)
{
  gdouble result;
  gdouble df = (gdouble) (discretization / fft_size);

  result = df * round (frequency / df);

  if (is_complex)
    {
      if (result < frequency0 - discretization / 2)
        result = frequency0 - discretization / 2;

      if (result > frequency0 + discretization / 2 - df)
        result = frequency0 + discretization / 2 - df;
    }
  else
    {
      if (result < 0)
        result = 0;

      if (result > discretization / 2 - df)
        result = discretization / 2 - df;
    }

  return result;  
}

/* Функция корректирует значения частот и гетеродина введенных пользователем
   с целью попасть в сетку частот FFT. */
void
correct_frequencies (GArray  *freq_array,
                     gdouble  discretization,
                     gdouble *heterodyne,
                     guint32  fft_size,
                     gboolean is_complex)
{
  FrequencyInfo *frequency;
  gdouble frequency0 = 0.0;
  guint i;

  for (i = 0; i < freq_array->len; ++i)
    {
      frequency = &g_array_index (freq_array, FrequencyInfo, i);
      
      if (i == 0)
        frequency0 = frequency->value;
      
      frequency->value = correct_frequency (frequency0, frequency->user_value, 
                                            discretization, fft_size, is_complex);
    }

  *heterodyne = correct_frequency (frequency0, *heterodyne, discretization,
                                   fft_size, is_complex);
}

/* Функция возвращает индекс заданной частоты в массиве частот. */
guint32
get_index_by_frequency (gdouble *freq,
                        guint32  size,
                        gdouble  frequency)
{
  guint32 index;

  for (index = 0; index < size; ++index)
    {
      if (fabs (freq[index] - frequency) < ERROR_LIMIT)
        return index;
    }

  return 0;  
}

/* Функция тестирует функцию БПФ над действительными данными на время выполнения. */
gboolean
fft_real_test (GArray  *freq_array,
               gdouble *time,
               gboolean constant,
               gdouble  amplitude,
               gdouble  discretization,
               guint32  n_points)
{
  gfloat *generated, *result;
  guint32 fft_size;

  /* Генерация данных. */
  generated = create_real_data (freq_array, amplitude, discretization, n_points, FALSE);

  /* Получаем размер преобразования. */
  fft_size = hyscan_fft_get_transform_size (n_points);
  if (fft_size == 0)
    {
      g_free (generated);
      return FALSE;
    }

  /* Производим расчет замеряя время обработки. */
  if (constant)
    {
      g_timer_start (timer);
      result = (gfloat *) hyscan_fft_transform_const_real (fft, HYSCAN_FFT_DIRECTION_FORWARD,
                                                           generated, n_points);
      *time = g_timer_elapsed (timer, NULL);
    }
  else
    {
      /* Выделяем массив под результат, записываем в него сгенерированные данные. */
      result = hyscan_fft_alloc (HYSCAN_FFT_TYPE_REAL, fft_size);
      memcpy (result, generated, n_points * sizeof (gfloat));

      g_timer_start (timer);
      hyscan_fft_transform_real (fft, HYSCAN_FFT_DIRECTION_FORWARD, result, n_points);
      *time = g_timer_elapsed (timer, NULL);
    }

  /* Освобождаем ресурсы. */
  if (!constant)
    hyscan_fft_free ((gpointer) result);
  g_free (generated);

  return TRUE;
}

/* Функция тестирует функцию БПФ над действительными данными на правильность
   расчета значений. */
gboolean
fft_real_value_test (GArray  *freq_array,
                     gboolean constant,
                     gdouble  amplitude,
                     gdouble  discretization,
                     guint32  fft_size)
{
  FrequencyInfo *frequency;
  gfloat *generated, *result, *ampl;
  guint32 *index_array;
  guint32 half_size, index, i, j;
  gboolean status = TRUE;
  gdouble df;

  /* Генерация данных. */
  generated = create_real_data (freq_array, amplitude, discretization, fft_size, TRUE);

  /* Производим расчет. */
  if (constant)
    {
      result = (gfloat *) hyscan_fft_transform_const_real (fft, 
                                                           HYSCAN_FFT_DIRECTION_FORWARD,
                                                           generated, fft_size);
    }
  else
    {
      /* Выделяем массив под результат, записываем в него сгенерированные данные. */
      result = hyscan_fft_alloc (HYSCAN_FFT_TYPE_REAL, fft_size);
      memcpy (result, generated, fft_size * sizeof (gfloat));

      hyscan_fft_transform_real (fft, HYSCAN_FFT_DIRECTION_FORWARD, result, fft_size);
    }

  /* Получаем спектр сигнала. */
  half_size = fft_size / 2;
  ampl = g_new0 (gfloat, half_size);
  for (i = 0; i < fft_size; i += 2)
    {
      gdouble re = result[i];
      gdouble im = result[i + 1];
      
      j = i / 2;
      ampl[j] = sqrtf (re * re + im * im);
    }

  /* Вычисляем индексы соответствующие тестовым частотам с фиксацией повторяющихся. */
  df = discretization / fft_size;
  index_array = g_new0 (guint32, half_size);
  for (i = 0; i < freq_array->len; ++i)
    {
      frequency = &g_array_index (freq_array, FrequencyInfo, i);
      index = (gint32) round (frequency->value / df);

      if (index == half_size)
        index = 0;

      frequency->index = index;
      index_array[index]++;
    }
  
  g_print ("    Range: 0 Hz - %f Hz; \n", discretization / 2 - df); 
  g_print ("    Step: %f Hz; \n", df);
  g_print ("    Discretization: %f Hz; \n", discretization);
  g_print ("    Signal size: %d; \n", fft_size);
  g_print ("    FFT size: %d; \n", fft_size);
  g_print ("    Frequences:\n");

  /* Проверяем результат. */
  for (i = 0; i < freq_array->len; ++i)
    {
      guint count;

      frequency = &g_array_index (freq_array, FrequencyInfo, i);
      index = frequency->index;
      g_print ("      Frequency: %f Hz; Corrected: %f Hz; Position in FFT: %d; Amplitude: %f;\n",
               frequency->user_value, frequency->value, index, ampl[index]);

      /* В нулевом индексе значение амплитуды должно быть равно тестовому значению. */
      count = index_array[index];
      if (index == 0)
        {
          if (fabs (ampl[index] - amplitude * count) > ERROR_LIMIT)
            {
              status = FALSE;
              break;
            }
        }
      /* В ненулевом индексе значение амплитуды должно быть равно половине
         тестового значения. */
      else if (fabs (ampl[index] - amplitude * count / 2) > ERROR_LIMIT)
        {
          status = FALSE;
          break;
        }
    }

  /* Освобождаем ресурсы. */
  if (!constant)
    hyscan_fft_free ((gpointer) result);
  g_free (index_array);
  g_free (ampl);
  g_free (generated);

  return status;
}

/* Функция тестирует функцию БПФ над комплексными данными и замеряет время выполнения. */
gboolean
fft_complex_test (GArray  *freq_array,
                  gdouble *time,
                  gboolean constant,
                  gboolean transposition,
                  gdouble  amplitude,                  
                  gdouble  heterodyne,
                  gdouble  discretization,
                  guint32  n_points)
{
  FrequencyInfo *frequency;
  HyScanComplexFloat *generated, *result;
  guint32 fft_size;

  /* Генерация комплексных данных. */
  generated = create_complex_data (freq_array, amplitude, heterodyne,
                                   discretization, n_points, FALSE);

  /* Получаем размер преобразования. */
  fft_size = hyscan_fft_get_transform_size (n_points);
  if (fft_size == 0)
    {
      g_free (generated);
      return FALSE;
    }

  /* Режим согласования частот. */
  frequency = &g_array_index (freq_array, FrequencyInfo, 0);
  hyscan_fft_set_transposition (fft, transposition, frequency->user_value, 
                                heterodyne, discretization);

  /* Применяем функцию расчета (*transform_const или *transform) замеряя время обработки. */
  if (constant)
    {
      g_timer_start (timer);
      result = (HyScanComplexFloat *) hyscan_fft_transform_const_complex (fft, 
                                                                          HYSCAN_FFT_DIRECTION_FORWARD, 
                                                                          generated,
                                                                          n_points);
      *time = g_timer_elapsed (timer, NULL);
    }
  else
    {
      /* Выделяем массив под результат, записываем в него сгенерированные данные. */
      result = (HyScanComplexFloat *) hyscan_fft_alloc (HYSCAN_FFT_TYPE_COMPLEX, fft_size);
      memcpy (result, generated, n_points * sizeof (HyScanComplexFloat));

      g_timer_start (timer);
      hyscan_fft_transform_complex (fft, HYSCAN_FFT_DIRECTION_FORWARD, result, n_points);      
      *time = g_timer_elapsed (timer, NULL);
    }

  /* Освобождаем ресурсы. */
  if (!constant)
    hyscan_fft_free ((gpointer) result);
  g_free (generated);

  return TRUE;
}

/* Функция тестирует функцию БПФ над комплексными данными на правильность 
   расчета значений. */
gboolean
fft_complex_value_test (GArray  *freq_array,
                        gboolean constant,
                        gboolean transposition,
                        gdouble  amplitude,                  
                        gdouble  heterodyne,
                        gdouble  discretization,
                        guint32  fft_size)
{
  FrequencyInfo *frequency;
  HyScanComplexFloat *generated, *result;
  guint32 *index_array;
  gdouble *freq;
  gfloat *ampl;
  gdouble frequency0, df, mod;
  gint32 fix, offset;
  gboolean status = TRUE;
  guint32 i;

  /* Генерация комплексных данных. */
  generated = create_complex_data (freq_array, amplitude, heterodyne,
                                   discretization, fft_size, TRUE);

  frequency = &g_array_index (freq_array, FrequencyInfo, 0);
  frequency0 = frequency->value;
  
  /* Режим согласования частот. */
  hyscan_fft_set_transposition (fft, transposition, frequency0, heterodyne, discretization);

  /* Применяем функцию расчета (*transform_const или *transform). */
  if (constant)
    {
      result = (HyScanComplexFloat *) hyscan_fft_transform_const_complex (fft, 
                                                                          HYSCAN_FFT_DIRECTION_FORWARD, 
                                                                          generated,
                                                                          fft_size);
    }
  else
    {
      /* Выделяем массив под результат, записываем в него сгенерированные данные. */
      result = (HyScanComplexFloat *) hyscan_fft_alloc (HYSCAN_FFT_TYPE_COMPLEX, fft_size);
      memcpy (result, generated, fft_size * sizeof (HyScanComplexFloat));

      hyscan_fft_transform_complex (fft, HYSCAN_FFT_DIRECTION_FORWARD, result, fft_size);
    }

  /* Рассчитываем спектр сигнала. */
  ampl = g_new0 (gfloat, fft_size);
  for (i = 0; i < fft_size; i++)
    {
      gdouble re = result[i].re;
      gdouble im = result[i].im;

      ampl[i] = sqrtf (re * re + im * im);
    }

  /* Расчитываем массив частот. */
  freq = g_new0 (gdouble, fft_size);

  mod = fmod (frequency0 - heterodyne, discretization);
  df = (gdouble) (discretization / fft_size);
  fix = (frequency0 - heterodyne) / df;
  for (i = 0; i < fft_size; ++i)
    {
      gdouble value;
      gint32 index = i;

      if (!transposition)
        {
          offset = fft_size * mod / discretization + fft_size / 2;
          index = i - offset;

          if (index < 0)
            index += fft_size;
          else if (index > (gint32) fft_size - 1)
            index -= fft_size;
        }

      value = (gdouble) index / (gdouble) fft_size - 0.5;
      freq[i] = value * discretization + df * fix + heterodyne;
    }

  g_print ("    Range: %f Hz - %f Hz; \n", 
           frequency0 - discretization / 2,  frequency0 + discretization / 2 - df);
  g_print ("    Step: %f Hz; \n", df);
  g_print ("    Frequency 0: %f Hz; \n", frequency0);
  g_print ("    Heterodyne: %f Hz; \n", heterodyne);
  g_print ("    Discretization: %f Hz; \n", discretization);
  g_print ("    Signal size: %d; \n", n_points);
  g_print ("    FFT size: %d; \n", fft_size);
  g_print ("    Frequences:\n");
  
  /* Вычисляем индексы соответствующие тестовым частотам с фиксацией повторяющихся. */
  index_array = g_new0 (guint32, fft_size);
  for (i = 0; i < freq_array->len; ++i)
    {
      frequency = &g_array_index (freq_array, FrequencyInfo, i);
      frequency->index = get_index_by_frequency (freq, fft_size, frequency->value);
      index_array[frequency->index]++;
    }

  /* Проверяем результат. */
  for (i = 0; i < freq_array->len; ++i)
    {
      guint32 index;

      frequency = &g_array_index (freq_array, FrequencyInfo, i);
      index = frequency->index;

      g_print ("      Frequency: %f Hz; Corrected: %f Hz; Position in FFT: %d; Amplitude: %f;\n",
               frequency->user_value, frequency->value, index, ampl[index]);
      if (fabs (ampl[index] - amplitude * index_array[index]) > ERROR_LIMIT)
        {
          status = FALSE;
          break;
        }
    }

  /* Освобождаем ресурсы. */
  if (!constant)
    hyscan_fft_free ((gpointer) result);
  g_free (index_array);
  g_free (freq);
  g_free (ampl);
  g_free (generated);

  return status;
}

/* Функция выполняет тест на время выполнения (повтор указанного количества раз
   c замером среднего времени), а также проверку расчитанных значений. */
gboolean
fft_test (GArray  *freq_array,
          guint    n_iterations,
          gboolean constant,
          gboolean is_complex,
          gboolean transposition)
{
  gdouble time;
  gdouble total = 0.0;
  gdouble corrected_heterodyne;
  gboolean status = FALSE;
  guint32 fft_size;
  guint i;

  /* Тестируем функции на время выполнения. */
  for (i = 0; i < n_iterations; ++i)
    {
      if (is_complex)
        {
          status = fft_complex_test (freq_array, &time, constant, transposition,
                                     amplitude, heterodyne, discretization, n_points);
        }
      else
        {
          status = fft_real_test (freq_array, &time, constant, amplitude, 
                                  discretization, n_points);
        }

      if (!status)
        {
          total = 0.0;
          break;
        }

      total += time;
    }

  g_print ("  Time test:\n");
  g_print ("    Iterations: %d;\n", n_iterations);
  g_print ("    Average time: %f s;\n", total / n_iterations);
  g_print ("    Status: %s\n", status ? "OK" : "FAIL.");
  
  /* Тестируем функции на правильность расчета значений. */
  g_print ("  Value test:\n");

  /* Получаем размер преобразования. Заданное количество отсчетов n_points 
     округляем до fft_size для получения в спектре целого значения амплитуды
     тестового сигнала. */
  fft_size = hyscan_fft_get_transform_size (n_points);
  if (fft_size == 0)
    return FALSE;

  /* Корректируем значения частот и гетеродина. */
  corrected_heterodyne = heterodyne;
  correct_frequencies (freq_array, discretization, &corrected_heterodyne,
                       fft_size, is_complex);

  if (is_complex)
    {
      status = fft_complex_value_test (freq_array, constant, transposition,
                                       amplitude, corrected_heterodyne,
                                       discretization, fft_size);
    }
  else
    {
      status = fft_real_value_test (freq_array, constant, amplitude,
                                    discretization, fft_size);
    }

  g_print ("    Status: %s\n\n", status ? "OK" : "FAIL.");

  if (!status)
    return FALSE;

  return TRUE;
}

int
main (int    argc,
      char **argv)
{
  GArray *freq_array;
  gchar *types = "all";
  gboolean status;
  guint i;
  
  guint n_iterations = 1;
  gchar **frequences = NULL;
    
  /* Считываем аргументы командной строки. */
  {
    gchar **args;
    GError *error = NULL;
    GOptionContext *context;
    GOptionEntry entries[] =
      {
        { "types", 't', 0, G_OPTION_ARG_STRING, &types, "Transform types (all, complex, real, "
                                                        "complex_transpos, const_complex, const_real, "
                                                        "const_complex_transpos)", NULL },
        { "amplitude", 'a', 0, G_OPTION_ARG_DOUBLE, &amplitude, "Signal amplitude", NULL },
        { "frequences", 'f', 0, G_OPTION_ARG_STRING_ARRAY, &frequences, "Signal frequences, Hz", NULL},
        { "heterodyne", 'h', 0, G_OPTION_ARG_DOUBLE, &heterodyne, "Heterodyne frequency, Hz", NULL },
        { "discretization", 'd', 0, G_OPTION_ARG_DOUBLE, &discretization, "Signal discretization, Hz", NULL },
        { "n_points", 'n', 0, G_OPTION_ARG_INT, &n_points, "Number of values array (32 - 1048576)", NULL },
        { "n_iterations", 'i', 0, G_OPTION_ARG_INT, &n_iterations, "Number of iterations", NULL },
        { NULL }
      };

#ifdef G_OS_WIN32
    args = g_win32_get_command_line ();
#else
    args = g_strdupv (argv);
#endif

    context = g_option_context_new ("");
    g_option_context_set_help_enabled (context, TRUE);
    g_option_context_add_main_entries (context, entries, NULL);
    g_option_context_set_ignore_unknown_options (context, FALSE);
    if (!g_option_context_parse_strv (context, &args, &error))
      {
        g_print ("%s\n", error->message);
        return -1;
      }

    g_option_context_free (context);
    g_strfreev (args);
  }

  timer = g_timer_new ();

  /* Массив с информацией о тестируемых частотах. */
  freq_array = g_array_new (FALSE, FALSE, sizeof (FrequencyInfo));

  if (frequences == NULL)
    {
      FrequencyInfo frequency_info;
      frequency_info.user_value = frequency;

      g_array_append_val (freq_array, frequency_info);
    }
  else
    {
      for (i = 0; frequences[i] != NULL; ++i)
        {
          FrequencyInfo frequency_info;
          frequency_info.user_value = g_ascii_strtod (frequences[i], NULL);
          
          g_array_append_val (freq_array, frequency_info);
        }
    }  

  /* Создаем объект производящий расчет БПФ. */
  fft = hyscan_fft_new ();

  /* Тестируем функции. */
  for (i = 0; func_info[i].type != NULL; ++i)
    {
      if (g_strcmp0 (types, "all") == 0 || g_strcmp0 (types, func_info[i].type) == 0)
        {
          g_print ("%s:\n", func_info[i].name);
          status = fft_test (freq_array, n_iterations, func_info[i].constant,
                             func_info[i].is_complex, func_info[i].transposition);
          if (!status)
            break;
        }
    }

  /* Освобождаем ресурсы. */
  g_object_unref (fft);
  g_array_free (freq_array, TRUE);
  g_timer_destroy (timer);
  g_strfreev (frequences);

  g_print ("All done.\n");

  return 0;
};
