/* hyscan-convolution.c
 *
 * Copyright 2015-2019 Screen LLC, Andrei Fadeev <andrei@webcontrol.ru>
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
 * SECTION: hyscan-convolution
 * @Short_description: класс свёртки данных
 * @Title: HyScanConvolution
 *
 * Класс HyScanConvolution используется для выполнения свёртки данных с
 * образом. Данные и образ для свёртки должны быть представлены в комплексном
 * виде (#HyScanComplexFloat).
 *
 * Объект для выполнения свёртки создаётся функцией #hyscan_convolution_new.
 *
 * Перед выполнением свёртки необходимо задать образ с которым будет выполняться
 * свёртка. Образ можем быть задан во временной области, для этого предназначена
 * функция #hyscan_convolution_set_image_td. Или в частотной области, функция
 * #hyscan_convolution_set_image_fd. Образ для свёртки можно изменять в процессе
 * работы с объектом.
 *
 * Образ свёртки в частотной области должен иметь определённый размер. Функция
 *
 *
 * Класс поддерживает установку сразу нескольких образов, при условии что они
 * имеют одинаковый размер. Определяющим является размер для образа с номером 0.
 *
 * Функция #hyscan_convolution_convolve выполняет свертку данных.
 *
 * HyScanConvolution не поддерживает работу в многопоточном режиме.
 */

#include "hyscan-convolution.h"

#include <math.h>
#include <string.h>
#include "pffft.h"

#ifdef HYSCAN_MATH_USE_OPENMP
#include <omp.h>
#endif

typedef enum
{
  HYSCAN_CONVOLUTION_IMAGE_TD,
  HYSCAN_CONVOLUTION_IMAGE_FD
} HyScanConvolutionImageType;

/* Таблица доспустимых размеров FFT преобразований. */
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

/* Внутренние данные объекта. */
struct _HyScanConvolutionPrivate
{
  HyScanComplexFloat          *ibuff;          /* Буфер для обработки данных. */
  HyScanComplexFloat          *obuff;          /* Буфер для обработки данных. */
  HyScanComplexFloat          *wbuff;          /* Буфер для обработки данных. */
  guint32                      max_points;     /* Максимальное число точек помещающихся в буферах. */

  PFFFT_Setup                 *fft;            /* Коэффициенты преобразования Фурье. */
  guint32                      fft_size;       /* Размер преобразования Фурье. */
  gfloat                       fft_scale;      /* Коэффициент масштабирования свёртки. */
  GHashTable                  *fft_images;     /* Образы для свёртки. */
};

static void      hyscan_convolution_object_constructed   (GObject                    *object);
static void      hyscan_convolution_object_finalize      (GObject                    *object);

static void      hyscan_convolution_realloc_buffers      (HyScanConvolutionPrivate   *priv,
                                                          guint32                     n_points);

static gboolean  hyscan_convolution_set_image            (HyScanConvolutionPrivate   *priv,
                                                          guint                       index,
                                                          HyScanConvolutionImageType  type,
                                                          const HyScanComplexFloat   *image,
                                                          guint32                     n_points);

G_DEFINE_TYPE_WITH_PRIVATE (HyScanConvolution, hyscan_convolution, G_TYPE_OBJECT);

static void
hyscan_convolution_class_init (HyScanConvolutionClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS( klass );

  object_class->constructed = hyscan_convolution_object_constructed;
  object_class->finalize = hyscan_convolution_object_finalize;
}

static void
hyscan_convolution_init (HyScanConvolution *convolution)
{
  convolution->priv = hyscan_convolution_get_instance_private (convolution);
}

static void
hyscan_convolution_object_constructed (GObject *object)
{
  HyScanConvolution *convolution = HYSCAN_CONVOLUTION (object);
  HyScanConvolutionPrivate *priv = convolution->priv;

  priv->fft_images = g_hash_table_new_full (NULL, NULL, NULL, pffft_aligned_free);
}

static void
hyscan_convolution_object_finalize (GObject *object)
{
  HyScanConvolution *convolution = HYSCAN_CONVOLUTION (object);
  HyScanConvolutionPrivate *priv = convolution->priv;

  pffft_aligned_free (priv->ibuff);
  pffft_aligned_free (priv->obuff);
  pffft_aligned_free (priv->wbuff);

  g_hash_table_unref (priv->fft_images);
  g_clear_pointer (&priv->fft, pffft_destroy_setup);

  G_OBJECT_CLASS (hyscan_convolution_parent_class)->finalize (object);
}

/* Функция выделяет память для буферов данных. */
static void
hyscan_convolution_realloc_buffers (HyScanConvolutionPrivate *priv,
                                    guint32                   n_points)
{
  if (n_points > priv->max_points)
    {
      priv->max_points = n_points;
      pffft_aligned_free (priv->ibuff);
      pffft_aligned_free (priv->obuff);
      pffft_aligned_free (priv->wbuff);
      priv->ibuff = pffft_aligned_malloc (priv->max_points * sizeof(HyScanComplexFloat));
      priv->obuff = pffft_aligned_malloc (priv->max_points * sizeof(HyScanComplexFloat));
      priv->wbuff = pffft_aligned_malloc (priv->max_points * sizeof(HyScanComplexFloat));
    }
}

/* Функция задаёт образ для свёртки. */
static gboolean
hyscan_convolution_set_image (HyScanConvolutionPrivate   *priv,
                              guint                       index,
                              HyScanConvolutionImageType  type,
                              const HyScanComplexFloat   *image,
                              guint32                     n_points)
{
  HyScanComplexFloat *fft_image;
  guint32 conv_size;
  guint32 fft_size;
  guint32 i;

  /* Очищаем текущий образ. */
  if (index == 0)
    g_hash_table_remove_all (priv->fft_images);
  else
    g_hash_table_remove (priv->fft_images, GINT_TO_POINTER (index));

  /* Пользователь отменил свёртку. */
  if (image == NULL)
    return TRUE;

  /* Ищем оптимальный размер свёртки для библиотеки pffft (см. pffft.h).
   * Для образа во временной области размер FFT преобразования увеличиваем в
   * два раза. А для частотной области размер образа должен быть точно равен
   * размеру FFT преобразования. */
  conv_size = (type == HYSCAN_CONVOLUTION_IMAGE_TD) ? 2 * n_points : n_points;
  fft_size = hyscan_convolution_get_fft_size (conv_size);
  if (fft_size == 0)
    {
      g_warning ("HyScanConvolution: fft size too big");
      return FALSE;
    }
  else if ((type == HYSCAN_CONVOLUTION_IMAGE_FD) && (conv_size != fft_size))
    {
      g_warning ("HyScanConvolution: image size mismatch with fft size");
      return FALSE;
    }

  /* Параметры преобразования Фурье. */
  if (index == 0)
    {
      if (priv->fft_size != fft_size)
        {
          g_clear_pointer (&priv->fft, pffft_destroy_setup);

          priv->fft = pffft_new_setup (fft_size, PFFFT_COMPLEX);
          if (priv->fft == NULL)
            {
              g_warning ("HyScanConvolution: can't setup fft");
              return FALSE;
            }

          priv->fft_size = fft_size;

          /* Обновляем буферы. */
          hyscan_convolution_realloc_buffers (priv, 16 * priv->fft_size);

          /* Коэффициент масштабирования свёртки. */
          priv->fft_scale = 1.0 / ((gfloat) priv->fft_size * (gfloat) n_points);
        }
    }
  else if (priv->fft_size != fft_size)
    {
      g_warning ("HyScanConvolution: fft size mismatch");
      return FALSE;
    }

  /* Буфер для образа в частотной области. */
  fft_image = pffft_aligned_malloc (priv->fft_size * sizeof(HyScanComplexFloat));

  /* Подготавливаем образ во временной области к свёртке и
   * делаем его комплексно сопряжённым. */
  if (type == HYSCAN_CONVOLUTION_IMAGE_TD)
    {
      HyScanComplexFloat *image_buff;

      image_buff = pffft_aligned_malloc (priv->fft_size * sizeof(HyScanComplexFloat));
      memset (image_buff, 0, priv->fft_size * sizeof(HyScanComplexFloat));
      memcpy (image_buff, image, n_points * sizeof(HyScanComplexFloat));

      pffft_transform_ordered (priv->fft,
                               (const gfloat*) image_buff,
                               (gfloat*) image_buff,
                               (gfloat*) priv->wbuff,
                               PFFFT_FORWARD);

      for (i = 0; i < priv->fft_size; i++)
        image_buff[i].im = -image_buff[i].im;

      pffft_zreorder (priv->fft,
                      (const gfloat*) image_buff,
                      (gfloat*) fft_image,
                      PFFFT_BACKWARD);

      pffft_aligned_free (image_buff);
    }

  /* Образ в частотной области преобразуем ко внутреннему представлению PFFFT. */
  else
    {
      HyScanComplexFloat *image_buff;

      image_buff = pffft_aligned_malloc (priv->fft_size * sizeof(HyScanComplexFloat));

      memcpy (image_buff, image, n_points * sizeof(HyScanComplexFloat));
      pffft_zreorder (priv->fft,
                      (const gfloat*) image_buff,
                      (gfloat*) fft_image,
                      PFFFT_BACKWARD);

      pffft_aligned_free (image_buff);
    }

  g_hash_table_insert (priv->fft_images, GINT_TO_POINTER (index), fft_image);

  return TRUE;
}

/**
 * hyscan_convolution_new:
 *
 * Функция создаёт новый объект #HyScanConvolution.
 *
 * Returns: #HyScanConvolution. Для удаления #g_object_unref.
 */
HyScanConvolution *
hyscan_convolution_new (void)
{
  return g_object_new (HYSCAN_TYPE_CONVOLUTION, NULL);
}

/**
 * hyscan_convolution_get_fft_size:
 * @fft_size: желаемый размер FFT преобразования
 *
 * Функция возвращает допустимый размер FFT преобразования для
 * указанного желаемого. Возвращаемый размер всегда больше или
 * равен желаемому.
 *
 * Returns: Допустимый размер FFT преобразования или 0.
 */
guint32
hyscan_convolution_get_fft_size (guint32 fft_size)
{
  guint i;

  for (i = 0; i < G_N_ELEMENTS (fft_sizes); i++)
    {
      if (fft_sizes[i] >= fft_size)
        return fft_sizes[i];
    }

  return 0;
}

/**
 * hyscan_convolution_set_image_td:
 * @convolution: указатель на #HyScanConvolution
 * @index: номер образа сигнала
 * @image: (nullable) (array length=n_points) (transfer none): образ для свёртки
 * @n_points: размер образа в точках
 *
 * Функция задаёт образ для свёртки во временной области. Если образ установлен
 * в NULL, свёртка отключается.
 *
 * При установке образа с номером 0, остальные образы обнуляются. Их необходимо
 * задать заново и их размер должен совпадать с нулевым образом.
 *
 * Returns: %TRUE если образ для свёртки установлен, иначе %FALSE.
 */
gboolean
hyscan_convolution_set_image_td (HyScanConvolution        *convolution,
                                 guint                     index,
                                 const HyScanComplexFloat *image,
                                 guint32                   n_points)
{
  g_return_val_if_fail (HYSCAN_IS_CONVOLUTION (convolution), FALSE);

  return hyscan_convolution_set_image (convolution->priv,
                                       index,
                                       HYSCAN_CONVOLUTION_IMAGE_TD,
                                       image,
                                       n_points);
}

/**
 * hyscan_convolution_set_image_fd:
 * @convolution: указатель на #HyScanConvolution
 * @index: номер образа сигнала
 * @image: (nullable) (array length=n_points) (transfer none): образ для свёртки
 * @n_points: размер образа в точках
 *
 * Функция задаёт образ для свёртки в частотной области. Если образ установлен
 * в NULL, свёртка отключается.
 *
 * При установке образа с номером 0, остальные образы обнуляются. Их необходимо
 * задать заново и их размер должен совпадать с нулевым образом.
 *
 * Returns: %TRUE если образ для свёртки установлен, иначе %FALSE.
 */
gboolean
hyscan_convolution_set_image_fd (HyScanConvolution        *convolution,
                                 guint                     index,
                                 const HyScanComplexFloat *image,
                                 guint32                   n_points)
{
  g_return_val_if_fail (HYSCAN_IS_CONVOLUTION (convolution), FALSE);

  return hyscan_convolution_set_image (convolution->priv,
                                       index,
                                       HYSCAN_CONVOLUTION_IMAGE_FD,
                                       image,
                                       n_points);
}

/**
 * hyscan_convolution_convolve:
 * @convolution: указатель на #HyScanConvolution
 * @index: номер образа сигнала
 * @data: (array length=n_points) (transfer none): данные для свёртки
 * @n_points: размер данных в точках
 * @scale: коэффициент масштабирования
 *
 * Функция выполняет свёртку данных с образом. Результат свёртки помещается
 * во входной массив. При свёртке производится автоматическое нормирование
 * на размер образа свёртки. Пользователь может указать дополнительный
 * коэффициент на который будут домножены данные после свёртки.
 *
 * Returns: %TRUE если свёртка выполнена, иначе %FALSE.
 */
gboolean
hyscan_convolution_convolve (HyScanConvolution  *convolution,
                             guint               index,
                             HyScanComplexFloat *data,
                             guint32             n_points,
                             gfloat              scale)
{
  HyScanConvolutionPrivate *priv;

  HyScanComplexFloat *fft_image;

  guint32 full_size;
  guint32 half_size;
  gint32 n_fft;
  gint32 i;

  g_return_val_if_fail (HYSCAN_IS_CONVOLUTION (convolution), FALSE);

  priv = convolution->priv;

  /* Свертка выполняется блоками по fft_size элементов, при этом каждый
   * следующий блок смещен относительно предыдущего на (fft_size / 2)
   * элементов. Входные данные находятся в ibuff, где над ними производится
   * прямое преобразование Фурье с сохранением результата в obuff, но уже
   * без перекрытия, т.е. с шагом fft_size. Затем производится перемножение
   * ("свертка") с нужным образом и обратное преобразование Фурье в ibuff.
   * Таким образом в ibuff оказываются необходимые данные, разбитые на
   * некоторое число блоков, в каждом из которых нам нужны только первые
   * (fft_size / 2) элементов. Так как операции над блоками происходят
   * независимо друг от друга этот процесс можно выполнять параллельно, что
   * и производится за счет использования библиотеки OpenMP. */

  /* Образ свёртки. */
  fft_image = g_hash_table_lookup (priv->fft_images, GINT_TO_POINTER (index));
  if (priv->fft == NULL || fft_image == NULL)
    return FALSE;

  full_size = priv->fft_size;
  half_size = priv->fft_size / 2;

  /* Число блоков преобразования Фурье над одной строкой. */
  n_fft = (n_points / half_size);
  if (n_points % half_size)
    n_fft += 1;

  /* Обновляем буферы. */
  hyscan_convolution_realloc_buffers(priv, n_fft * priv->fft_size);

  /* Копируем данные во входной буфер. */
  memcpy (priv->ibuff,
          data,
          n_points * sizeof(HyScanComplexFloat));

  /* Зануляем конец буфера по границе half_size. */
  memset (priv->ibuff + n_points,
          0,
          ((n_fft + 1) * half_size - n_points) * sizeof(HyScanComplexFloat));

  /* Прямое преобразование Фурье. */
#ifdef HYSCAN_MATH_USE_OPENMP
#pragma omp parallel for
#endif
  for (i = 0; i < n_fft; i++)
    {
      pffft_transform (priv->fft,
                       (const gfloat*) (priv->ibuff + (i * half_size)),
                       (gfloat*) (priv->obuff + (i * full_size)),
                       (gfloat*) (priv->wbuff + (i * full_size)),
                       PFFFT_FORWARD);
    }

  /* Свёртка и обратное преобразование Фурье. */
#ifdef HYSCAN_MATH_USE_OPENMP
#pragma omp parallel for
#endif
  for (i = 0; i < n_fft; i++)
    {
      guint32 offset = i * full_size;
      guint32 used_size = MIN ((n_points - i * half_size), half_size);

      /* Обнуляем выходной буфер, т.к. функция zconvolve_accumulate добавляет
       * полученный результат к значениям в этом буфере (нам это не нужно). */
      memset (priv->ibuff + offset,
              0,
              full_size * sizeof(HyScanComplexFloat));

      /* Выполняем свёртку. */
      pffft_zconvolve_accumulate (priv->fft,
                                  (const gfloat*) (priv->obuff + offset),
                                  (const gfloat*) fft_image,
                                  (gfloat*) (priv->ibuff + offset),
                                  scale * priv->fft_scale);

      /* Выполняем обратное преобразование Фурье. */
      pffft_zreorder (priv->fft,
                      (gfloat*) (priv->ibuff + offset),
                      (gfloat*) (priv->obuff + offset),
                      PFFFT_FORWARD);

      pffft_transform_ordered (priv->fft,
                               (gfloat*) (priv->obuff + offset),
                               (gfloat*) (priv->ibuff + offset),
                               (gfloat*) (priv->wbuff + offset),
                               PFFFT_BACKWARD);

      /* Копируем результат обратно в буфер пользователя. */
      memcpy (data + offset / 2,
              priv->ibuff + offset,
              used_size * sizeof (HyScanComplexFloat));
    }

  return TRUE;
}
