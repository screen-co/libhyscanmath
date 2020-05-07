/* hyscan-fft.c
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

/**
 * SECTION: hyscan-fft
 * @Short_description: класс расчета БПФ
 * @Title: HyScanFFT
 *
 * Класс HyScanFFT используется для выполнения расчета БПФ над действительными 
 * или комплексными данными. Является оберткой класса PFFFT_Setup. (см. pffft.h)
 *
 * Объект для выполнения расчета создаётся функцией #hyscan_fft_new.
 *
 * Функции класса можно условно разделить на 2 типа:
 *  - функции в которых результат расчета записывается во входной массив
 *    (#hyscan_fft_transform_real, #hyscan_fft_transform_complex);
 *  - функции в которых возвращается константный результат расчета
 *    (#hyscan_fft_transform_const_real, #hyscan_fft_transform_const_complex);
 * 
 * Расчет БПФ производится над массивом строго фиксированного размера 
 * (числа кратные степени 2 в диапазоне от 32 до 1048576). Так как
 * количество отсчётов в выборке представления сигнала может быть не равно этому
 * размеру, то для удобства получения размера преобразования используется функция 
 * #hyscan_fft_get_transform_size.
 *
 * В функциях #hyscan_fft_transform_real и #hyscan_fft_transform_complex память
 * для входных данных должна быть выделена/освобождена с помощью функций 
 * #hyscan_fft_alloc и #hyscan_fft_free, затем пользователь помещает массив 
 * с выборкой представления сигнала в эту память дополнительно указав
 * количество отсчетов выборки для корректного проведения операции масштабирования
 * результата.
 * 
 * В результате БПФ над комплексными данными получается массив размером fft_size
 * чередующихся действительных и мнимых чисел, в первой записи которого расположено 
 * значение соответствующее несущей частоте сигнала (в случае применения функции
 * #hyscan_fft_set_transposition значение соответствующее несущей частоте сигнала
 * будет находиться в позиции fft_size/2, где fft_size - размер преобразования,
 * полученный с помощью функции #hyscan_fft_get_transform_size.
 *
 * В результате БПФ над действительными данными получается массив размером fft_size
 * действительных чисел, информативную часть которого представляет половина массива.
 * Значения будут расположены в порядке возрастания соответствующих частот
 * в диапазоне [0; data_rate/2 - data_rate/fft_size] с шагом data_rate/fft_size,
 * где fft_size - размер преобразования, полученный с помощью функции 
 * #hyscan_fft_get_transform_size, data_rate - частота дискретизации сигнала.
 *
 * Расчет БПФ также может быть задействован в реализации задачи получения спектра
 * сигнала. Так как расчет ведется без учета частоты дискретизации необходимо
 * привязать индексы результирующего массива к значениям частоты выраженной в Герц.
 * Для этих целей предназначена функция #hyscan_fft_set_transposition в которую 
 * передаются несущая частота и частота дискретизации излучаемого сигнала.
 * Функция #hyscan_fft_set_transposition применима только для функций БПФ 
 * над комплексными данными.
 * В ней применяется перестановка значений результирующего массива, при которой
 * значения перемещены согласно возрастанию частоты (каждому значению результирующего 
 * массива соответствует определенная частота) и в центре располагается значение 
 * соответствующее несущей частоте сигнала или гетеродина.
 *
 * В случае если производится преобразование частоты сигнала путем управления 
 * частотой гетеродина, в функцию #hyscan_fft_set_transposition должно быть 
 * передано соответствующее значение частоты гетеродина для последующего согласования.
 */

#include "hyscan-fft.h"
#include <hyscan-buffer.h>
#include <string.h>
#include <math.h>
#include "pffft.h"

/* Таблица допустимых размеров FFT преобразований. */
static guint
fft_sizes[] = {32, 64, 96, 128, 160, 192, 256, 288, 320, 384, 480, 512, 576, 640, 768,
               800, 864, 960, 1024, 1152, 1280, 1440, 1536, 1600, 1728, 1920, 2048, 2304,
               2400, 2560, 2592, 2880, 3072, 3200, 3456, 3840, 4000, 4096, 4320, 4608,
               4800, 5120, 5184, 5760, 6144, 6400, 6912, 7200, 7680, 7776, 8000, 8192,
               8640, 9216, 9600, 10240, 10368, 11520, 12000, 12288, 12800, 12960, 13824,
               14400, 15360, 15552, 16000, 16384, 17280, 18432, 19200, 20000, 20480, 20736,
               21600, 23040, 23328, 24000, 24576, 25600, 25920, 27648, 28800, 30720, 31104,
               32000, 32768, 34560, 36000, 36864, 38400, 38880, 40000, 40960, 41472, 43200,
               46080, 46656, 48000, 49152, 51200, 51840, 55296, 57600, 60000, 61440, 62208,
               64000, 64800, 65536, 69120, 69984, 72000, 73728, 76800, 77760, 80000, 81920,
               82944, 86400, 92160, 93312, 96000, 98304, 100000, 102400, 103680, 108000,
               110592, 115200, 116640, 120000, 122880, 124416, 128000, 129600, 131072,
               138240, 139968, 144000, 147456, 153600, 155520, 160000, 163840, 165888,
               172800, 180000, 184320, 186624, 192000, 194400, 196608, 200000, 204800,
               207360, 209952, 216000, 221184, 230400, 233280, 240000, 245760, 248832,
               256000, 259200, 262144, 276480, 279936, 288000, 294912, 300000, 307200,
               311040, 320000, 324000, 327680, 331776, 345600, 349920, 360000, 368640,
               373248, 384000, 388800, 393216, 400000, 409600, 414720, 419904, 432000,
               442368, 460800, 466560, 480000, 491520, 497664, 500000, 512000, 518400,
               524288, 540000, 552960, 559872, 576000, 583200, 589824, 600000, 614400,
               622080, 629856, 640000, 648000, 655360, 663552, 691200, 699840, 720000,
               737280, 746496, 768000, 777600, 786432, 800000, 819200, 829440, 839808,
               864000, 884736, 900000, 921600, 933120, 960000, 972000, 983040, 995328,
               1000000, 1024000, 1036800, 1048576};

struct _HyScanFFTPrivate
{
  PFFFT_Setup        *fft;                /* Объект производящий БПФ. */
  HyScanFFTType       type;               /* Тип обрабатываемых данных. */
  HyScanFFTDirection  direction;          /* Направление преобразования. */
  guint32             fft_size;           /* Размер преобразования. */
  
  HyScanBuffer       *temp_buff;          /* Буфер для перестановки участков массива. */
  HyScanComplexFloat *ibuff;              /* Буфер хранения результата в const функциях. */
  HyScanComplexFloat *wbuff;              /* Рабочий буфер для обработки данных. */

  gboolean            transposition;      /* Признак применения режима согласования частот. */
  
  gdouble             frequency0;         /* Несущая частота излучаемого сигнала, Гц. */
  gdouble             heterodyne;         /* Частота гетеродина, Гц. */
  gdouble             data_rate;          /* Частота дискретизации, Гц. */
};

static void      hyscan_fft_object_constructed    (GObject            *object);

static void      hyscan_fft_object_finalize       (GObject            *object);

static gboolean  hyscan_fft_prepare               (HyScanFFTPrivate   *priv,
                                                   HyScanFFTType       type,
                                                   HyScanFFTDirection  direction,
                                                   guint32             size);

static void      hyscan_fft_alloc_internal        (HyScanFFTPrivate   *priv);

static gboolean  hyscan_fft_handler_create        (HyScanFFTPrivate   *priv);

static void      hyscan_fft_scale                 (HyScanComplexFloat *data,
                                                   guint32             size,
                                                   gdouble             scale);

static void      hyscan_fft_transposition         (HyScanFFTPrivate   *priv,
                                                   HyScanComplexFloat *ibuff);

G_DEFINE_TYPE_WITH_PRIVATE (HyScanFFT, hyscan_fft, G_TYPE_OBJECT)

static void
hyscan_fft_class_init (HyScanFFTClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = hyscan_fft_object_constructed;
  object_class->finalize = hyscan_fft_object_finalize;
}

static void
hyscan_fft_init (HyScanFFT *fft)
{
  fft->priv = hyscan_fft_get_instance_private (fft);
}

static void
hyscan_fft_object_constructed (GObject *object)
{
  HyScanFFT *fft = HYSCAN_FFT (object);
  HyScanFFTPrivate *priv = fft->priv;

  priv->fft = NULL;
  priv->type = HYSCAN_FFT_TYPE_INVALID;
  priv->fft_size = fft_sizes[0];
  priv->transposition = FALSE;

  priv->temp_buff = hyscan_buffer_new ();
}

static void
hyscan_fft_object_finalize (GObject *object)
{
  HyScanFFT *fft = HYSCAN_FFT (object);
  HyScanFFTPrivate *priv = fft->priv;

  pffft_aligned_free (priv->ibuff);
  pffft_aligned_free (priv->wbuff);
  g_clear_pointer (&priv->fft, pffft_destroy_setup);

  g_object_unref (priv->temp_buff);

  G_OBJECT_CLASS (hyscan_fft_parent_class)->finalize (object);
}

/* Функция подготавливает данные класса к расчету. */
static gboolean
hyscan_fft_prepare (HyScanFFTPrivate  *priv,
                    HyScanFFTType      type,
                    HyScanFFTDirection direction,
                    guint32            size)
{
  guint32 fft_size;

  /* Определяем размер преобразования. */
  if ((fft_size = hyscan_fft_get_transform_size (size)) == 0)
    {
      g_warning ("HyScanFFT: incorrect size fft");
      return FALSE;
    }

  /* Если изменился тип входных данных или размер преобразования
     создаем новый объект расчитывающий БПФ. */
  if (type != priv->type || fft_size != priv->fft_size)
    {
      priv->type = type;
      priv->fft_size = fft_size;

      if (!hyscan_fft_handler_create (priv))
        return FALSE;

      /* Выделение памяти для рабочих буферов. */
      hyscan_fft_alloc_internal (priv);
    }

  priv->direction = direction;

  return TRUE;
}

/* Функция выделяет память для рабочих буферов. */
static void
hyscan_fft_alloc_internal (HyScanFFTPrivate *priv)
{
  pffft_aligned_free (priv->ibuff);
  pffft_aligned_free (priv->wbuff);

  if (priv->type == HYSCAN_FFT_TYPE_REAL)
    {
      priv->ibuff = pffft_aligned_malloc (priv->fft_size * sizeof (gfloat));
      priv->wbuff = pffft_aligned_malloc (priv->fft_size * sizeof (gfloat));
      memset (priv->ibuff, 0, priv->fft_size * sizeof (gfloat));
    }
  else if (priv->type == HYSCAN_FFT_TYPE_COMPLEX)
    {
      priv->ibuff = pffft_aligned_malloc (priv->fft_size * sizeof (HyScanComplexFloat));
      priv->wbuff = pffft_aligned_malloc (priv->fft_size * sizeof (HyScanComplexFloat));
      memset (priv->ibuff, 0, priv->fft_size * sizeof (HyScanComplexFloat));
    }
}

/* Функция создает объект расчета БПФ. */
static gboolean
hyscan_fft_handler_create (HyScanFFTPrivate *priv)
{
  guint type = PFFFT_REAL;

  /* Освобождаем существующий объект расчета БПФ. */
  g_clear_pointer (&priv->fft, pffft_destroy_setup);
  
  if (priv->type == HYSCAN_FFT_TYPE_REAL)
    type = PFFFT_REAL;
  else if (priv->type == HYSCAN_FFT_TYPE_COMPLEX)
    type = PFFFT_COMPLEX;

  /* Создаем новый объект расчета БПФ. */
  priv->fft = pffft_new_setup (priv->fft_size, type);
  
  if (!priv->fft)
    {
      g_warning ("HyScanFFT: can't setup fft");
      return FALSE;
    }

  return TRUE;  
}

/* Функция производит масштабирование данных. */
static void
hyscan_fft_scale (HyScanComplexFloat *data,
                  guint32             size,
                  gdouble             scale)
{
  guint i;

  for (i = 0; i < size; i++)
    {
      data[i].re *= scale;
      data[i].im *= scale;
    }
}

/* Функция производит согласование частот результирующего массива с частотой 
   гетеродина и частотой излучаемого сигнала. */
static void
hyscan_fft_transposition (HyScanFFTPrivate   *priv,
                          HyScanComplexFloat *ibuff)
{
  HyScanComplexFloat *temp;
  gdouble frequency0, heterodyne, data_rate, half_data_rate, mod, df;
  guint32 size_first, size_second, fft_size;
  gint32 index0;

  frequency0 = priv->frequency0;
  heterodyne = priv->heterodyne;
  data_rate = priv->data_rate;
  half_data_rate = priv->data_rate / 2;
  fft_size = priv->fft_size;
  df = (gdouble) (data_rate / fft_size);

  /* Проверяем значение гетеродина которое должно быть в пределах
     [frequency0 - data_rate/2; frequency0 + data_rate/2 - df]. */
  if (heterodyne < frequency0 - half_data_rate)
    heterodyne = frequency0 - half_data_rate;
  else if (heterodyne > frequency0 + half_data_rate - df)
    heterodyne = frequency0 + half_data_rate - df;

  /* Определяем размеры перемещаемых частей в массиве. Для этого расчитаем 
     позицию 0-го элемента после операции согласования частот, которая и разделит
     массив на две части.*/
  mod = fmod (frequency0 - heterodyne, data_rate);
  index0 = fft_size / 2 - (guint32) (fft_size * mod / data_rate);

  size_first = fft_size - index0;
  size_second = fft_size - size_first;

  /* Перемена мест двух участков массива. */
  hyscan_buffer_set_complex_float (priv->temp_buff, ibuff, size_first);
  memcpy (ibuff, ibuff + size_first, size_second * sizeof (HyScanComplexFloat));
  temp = hyscan_buffer_get_complex_float (priv->temp_buff, &size_first);
  memcpy (ibuff + size_second, temp, size_first * sizeof (HyScanComplexFloat));
}

/**
 * hyscan_fft_new:
 *
 * Функция создаёт новый объект #HyScanFFT.
 *
 * Returns: #HyScanFFT. Для удаления #g_object_unref.
 */
HyScanFFT *
hyscan_fft_new ()
{
  return g_object_new (HYSCAN_TYPE_FFT, NULL);
}

/**
 * hyscan_fft_set_transposition:
 * @fft: указатель на #HyScanFFT
 * @transposition: признак применения режима согласования частот
 * @frequency0: несущая частота излучаемого сигнала, Гц
 * @heterodyne: частота гетеродина, Гц
 * @data_rate: частота дискретизации, Гц
 *
 * Функция включает/отключает режим при котором результирующий массив согласуется
 * с частотой гетеродина и частотой излучаемого сигнала. Операция производится
 * только над комплексными отсчетами.
 */
void
hyscan_fft_set_transposition (HyScanFFT *fft,
                              gboolean   transposition,
                              gdouble    frequency0,
                              gdouble    heterodyne,
                              gdouble    data_rate)
{
  HyScanFFTPrivate *priv;

  g_return_if_fail (HYSCAN_IS_FFT (fft));

  priv = fft->priv;

  priv->transposition = transposition;
  priv->frequency0 = frequency0;
  priv->heterodyne = heterodyne;
  priv->data_rate = data_rate;
}

/**
 * hyscan_fft_transform_real:
 * @fft: указатель на #HyScanFFT
 * @direction: направление преобразования
 * @data: (inout) (array length=hyscan_fft_get_transform_size(n_points)) массив
          с входными данными, после выполнения расчета хранит результат преобразования
 * @n_points: количество значащих отсчетов входных данных
 *
 * Функция производит расчет БПФ над действительными данными, результат записывается
 * в массив входных данных data. Память для входного массива должна быть выделена и 
 * освобождена с помощью функций #hyscan_fft_alloc и #hyscan_fft_free.
 * 
 * Returns: TRUE в случае успеха, иначе FALSE.
 */
gboolean
hyscan_fft_transform_real (HyScanFFT         *fft,
                           HyScanFFTDirection direction,
                           gfloat            *data,
                           guint32            n_points)
{
  HyScanFFTPrivate *priv;
  gdouble fft_scale;

  g_return_val_if_fail (HYSCAN_IS_FFT (fft), FALSE);

  priv = fft->priv;

  if (data == NULL)
    return FALSE;

  /* Подготавливаем данные. */
  if (!hyscan_fft_prepare (priv, HYSCAN_FFT_TYPE_REAL, direction, n_points))
    return FALSE;

  /* Расчет. */
  pffft_transform_ordered (priv->fft,
                           (const gfloat*) data,
                           data,
                           (gfloat*) priv->wbuff,
                           priv->direction ? PFFFT_BACKWARD : PFFFT_FORWARD);

  /* Масштабирование. */
  fft_scale = 1.0 / n_points;
  hyscan_fft_scale ((HyScanComplexFloat*) data, priv->fft_size / 2, fft_scale);

  return TRUE;
}

/**
 * hyscan_fft_transform_complex:
 * @fft: указатель на #HyScanFFT
 * @direction: направление преобразования
 * @data: (inout) (array length=hyscan_fft_get_transform_size(n_points)) массив
          с входными данными, после выполнения расчета хранит результат преобразования
 * @n_points: количество значащих отсчетов входных данных
 *
 * Функция производит расчет БПФ над комплексными данными, результат записывается
 * в массив входных данных data. Память для входного массива должна быть выделена
 * и освобождена с помощью функций #hyscan_fft_alloc и #hyscan_fft_free.
 * Над данными может быть произведена операция согласования частот
 * (см. #hyscan_fft_set_transposition).
 * 
 * Returns: TRUE в случае успеха, иначе FALSE.
 */
gboolean
hyscan_fft_transform_complex (HyScanFFT          *fft,
                              HyScanFFTDirection  direction,
                              HyScanComplexFloat *data,
                              guint32             n_points)
{
  HyScanFFTPrivate *priv;
  gdouble fft_scale;

  g_return_val_if_fail (HYSCAN_IS_FFT (fft), FALSE);

  priv = fft->priv;

  if (data == NULL)
    return FALSE;

  /* Подготавливаем данные. */
  if (!hyscan_fft_prepare (priv, HYSCAN_FFT_TYPE_COMPLEX, direction, n_points))
    return FALSE;

  /* Расчет. */
  pffft_transform_ordered (priv->fft,
                           (const gfloat*) data,
                           (gfloat*) data,
                           (gfloat*) priv->wbuff,
                           priv->direction ? PFFFT_BACKWARD : PFFFT_FORWARD);

  /* Согласование частот. */
  if (priv->transposition)
    hyscan_fft_transposition (priv, data);

  /* Масштабирование. */
  fft_scale = 1.0 / n_points;
  hyscan_fft_scale (data, priv->fft_size, fft_scale);

  return TRUE;
}

/**
 * hyscan_fft_transform_const_real:
 * @fft: указатель на #HyScanFFT
 * @direction: направление преобразования
 * @data: (array length=n_points) входные данные
 * @n_points: количество отсчетов входных данных
 *
 * Функция производит расчет БПФ над действительными данными.
 * Функция возвращает указатель на внутренний буфер, данные в котором
 * действительны до следующего вызова const функций HyScanFFT. Пользователь
 * не должен модифицировать эти данные.
 * В случае успешного расчета размер результирующего массива будет иметь 
 * значение возвращаемое функцией #hyscan_fft_get_transform_size для n_points.
 * 
 * Returns: (nullable) (array length=fft_size) (transfer none):
 *          Значения действительных данных или NULL.
 */
const gfloat *
hyscan_fft_transform_const_real (HyScanFFT         *fft,
                                 HyScanFFTDirection direction,
                                 const gfloat      *data,
                                 guint32            n_points)
{
  HyScanFFTPrivate *priv;
  gdouble fft_scale;

  g_return_val_if_fail (HYSCAN_IS_FFT (fft), NULL);

  priv = fft->priv;

  if (data == NULL)
    return NULL;

  /* Подготавливаем данные. */
  if (!hyscan_fft_prepare (priv, HYSCAN_FFT_TYPE_REAL, direction, n_points))
    return NULL;

  /* Записываем входные данные в буфер. */
  memset (priv->ibuff, 0, priv->fft_size * sizeof (gfloat));
  memcpy (priv->ibuff, data, n_points * sizeof (gfloat));

  /* Расчет. */
  pffft_transform_ordered (priv->fft,
                           (const gfloat*) priv->ibuff,
                           (gfloat*) priv->ibuff,
                           (gfloat*) priv->wbuff,
                           priv->direction ? PFFFT_BACKWARD : PFFFT_FORWARD);

  /* Масштабирование. */
  fft_scale = 1.0 / n_points;
  hyscan_fft_scale (priv->ibuff, priv->fft_size / 2, fft_scale);

  return (const gfloat *) priv->ibuff;
}

/**
 * hyscan_fft_transform_const_complex:
 * @fft: указатель на #HyScanFFT
 * @direction: направление преобразования
 * @data: (array length=n_points) входные данные
 * @n_points: количество отсчетов входных данных
 *
 * Функция производит расчет БПФ над комплексными данными.
 * Функция возвращает указатель на внутренний буфер, данные в котором
 * действительны до следующего вызова const функций HyScanFFT. Пользователь
 * не должен модифицировать эти данные.
 * В случае успешного расчета размер результирующего массива будет иметь 
 * значение возвращаемое функцией #hyscan_fft_get_transform_size для n_points.
 * Над данными может быть произведена операция согласования частот
 * (см. #hyscan_fft_set_transposition).
 *
 * Returns: (nullable) (array length=fft_size) (transfer none):
 *          Значения комплексных данных или NULL.
 */
const HyScanComplexFloat *
hyscan_fft_transform_const_complex (HyScanFFT                *fft,
                                    HyScanFFTDirection        direction,
                                    const HyScanComplexFloat *data,
                                    guint32                   n_points)
{
  HyScanFFTPrivate *priv;  
  gdouble fft_scale;

  g_return_val_if_fail (HYSCAN_IS_FFT (fft), NULL);

  priv = fft->priv;

  if (data == NULL)
    return NULL;

  /* Подготавливаем данные. */
  if (!hyscan_fft_prepare (priv, HYSCAN_FFT_TYPE_COMPLEX, direction, n_points))
    return NULL;

  /* Записываем входные данные в буфер. */
  memset (priv->ibuff, 0, priv->fft_size * sizeof (HyScanComplexFloat));
  memcpy (priv->ibuff, data, n_points * sizeof (HyScanComplexFloat));

  /* Расчет. */
  pffft_transform_ordered (priv->fft,
                           (const gfloat*) priv->ibuff,
                           (gfloat*) priv->ibuff,
                           (gfloat*) priv->wbuff,
                           priv->direction ? PFFFT_BACKWARD : PFFFT_FORWARD);

  /* Согласование частот. */
  if (priv->transposition)
    hyscan_fft_transposition (priv, priv->ibuff);

  /* Масштабирование. */
  fft_scale = 1.0 / n_points;
  hyscan_fft_scale (priv->ibuff, priv->fft_size, fft_scale);

  return priv->ibuff;
}

/**
 * hyscan_fft_get_transform_size:
 * @size: размер для преобразования
 *
 * Функция возвращает размер преобразования после округления size (в большую сторону).
 * Если указан некорректный размер преобразования функция вернет 0.
 * 
 * Returns: размер преобразования после округления или 0.
 */
guint32
hyscan_fft_get_transform_size (guint32 size)
{
  guint32 fft_size = 0;
  guint i;

  for (i = 0; i < (sizeof (fft_sizes) / sizeof (guint)); i++)
    {
      if (size <= fft_sizes[i])
        {
            fft_size = fft_sizes[i];
          break;
        }
    }

  return fft_size;
}

/**
 * hyscan_fft_alloc:
 * @type: тип входных данных
 * @fft_size: размер преобразования полученный с помощью #hyscan_fft_get_transform_size
 *
 * Функция выделяет специально выровненный буфер для типа данных type. Размер 
 * fft_size должен быть получен с помощью функции #hyscan_fft_get_transform_size.
 * Если указан некорректный размер преобразования функция вернет NULL.
 *
 * Returns: (nullable) указатель на выделенную память или NULL.
 */
gpointer
hyscan_fft_alloc (HyScanFFTType type,
                  guint32       fft_size)
{
  gpointer data = NULL;

  if (type == HYSCAN_FFT_TYPE_INVALID)
    return NULL;

  /* Проверяем корректность заданного размера преобразования. */
  if (fft_size != hyscan_fft_get_transform_size (fft_size))
    {
      g_warning ("HyScanFFT: incorrect size fft");
      return NULL;
    }

  if (type == HYSCAN_FFT_TYPE_REAL)
    {
      data = pffft_aligned_malloc (fft_size * sizeof (gfloat));
      memset (data, 0, fft_size * sizeof (gfloat));
    }
  else if (type == HYSCAN_FFT_TYPE_COMPLEX)
    {
      data = pffft_aligned_malloc (fft_size * sizeof (HyScanComplexFloat));
      memset (data, 0, fft_size * sizeof (HyScanComplexFloat));
    }  

  return data;
}

/**
 * hyscan_fft_free:
 * @data: указатель на данные для удаления
 *
 * Функция освобождает память.
 */
void
hyscan_fft_free (gpointer data)
{
  pffft_aligned_free (data);
}
