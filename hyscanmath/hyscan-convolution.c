/*
 * \file hyscan-convolution.c
 *
 * \brief Исходный файл класса свёртки данных
 * \author Andrei Fadeev (andrei@webcontrol.ru)
 * \date 2015
 * \license Проприетарная лицензия ООО "Экран"
 *
 */

#include "hyscan-convolution.h"

#include <math.h>
#include <string.h>
#include "pffft.h"

#ifdef HYSCAN_MATH_USE_OPENMP
#include <omp.h>
#endif

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
  HyScanComplexFloat          *fft_image;      /* Образец сигнала для свёртки. */
};

static void    hyscan_convolution_object_finalize      (GObject                       *object);

static void    hyscan_convolution_realloc_buffers      (HyScanConvolutionPrivate      *priv,
                                                        guint                          n_points);

G_DEFINE_TYPE_WITH_PRIVATE (HyScanConvolution, hyscan_convolution, G_TYPE_OBJECT);

static void
hyscan_convolution_class_init (HyScanConvolutionClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS( klass );

  object_class->finalize = hyscan_convolution_object_finalize;
}

static void
hyscan_convolution_init (HyScanConvolution *convolution)
{
  convolution->priv = hyscan_convolution_get_instance_private (convolution);
}

static void
hyscan_convolution_object_finalize (GObject *object)
{
  HyScanConvolution *convolution = HYSCAN_CONVOLUTION (object);
  HyScanConvolutionPrivate *priv = convolution->priv;

  pffft_aligned_free (priv->ibuff);
  pffft_aligned_free (priv->obuff);
  pffft_aligned_free (priv->wbuff);

  pffft_aligned_free (priv->fft_image);
  pffft_destroy_setup (priv->fft);

  G_OBJECT_CLASS (hyscan_convolution_parent_class)->finalize (object);
}

/* Функция выделяет память для буферов джанных. */
static void
hyscan_convolution_realloc_buffers (HyScanConvolutionPrivate *priv,
                                    guint                     n_points)
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

/* Функция создаёт новый объект HyScanConvolution. */
HyScanConvolution *
hyscan_convolution_new (void)
{
  return g_object_new (HYSCAN_TYPE_CONVOLUTION, NULL);
}

/* Функция задаёт образ сигнала для свёртки. */
gboolean
hyscan_convolution_set_image (HyScanConvolution        *convolution,
                              const HyScanComplexFloat *image,
                              guint32                   n_points)
{
  HyScanConvolutionPrivate *priv;

  HyScanComplexFloat *image_buff;

  guint32 conv_size = 2 * n_points;
  guint32 opt_size = 0;
  guint32 i;

  g_return_val_if_fail (HYSCAN_IS_CONVOLUTION (convolution), FALSE);

  priv = convolution->priv;

  /* Отменяем свёртку с текущим сигналом. */
  g_clear_pointer (&priv->fft, pffft_destroy_setup);
  g_clear_pointer (&priv->fft_image, pffft_aligned_free);
  priv->fft_size = 2;

  /* Пользователь отменил свёртку. */
  if (image == NULL)
    return TRUE;

  /* Ищем оптимальный размер свёртки для библиотеки pffft_new_setup (см. pffft.h). */
  opt_size = G_MAXUINT32;
  for (i = 0; i < (sizeof (fft_sizes) / sizeof (guint)); i++)
    if (conv_size <= fft_sizes[i])
      {
        opt_size = fft_sizes[i];
        break;
      }

  if (opt_size == G_MAXUINT32)
    {
      g_critical ("HyScanConvolution: fft size too big");
      return FALSE;
    }

  priv->fft_size = opt_size;

  /* Обновляем буферы. */
  hyscan_convolution_realloc_buffers(priv, 16 * priv->fft_size);

  /* Коэффициент масштабирования свёртки. */
  priv->fft_scale = 1.0 / ((gfloat) priv->fft_size * (gfloat) n_points);

  /* Параметры преобразования Фурье. */
  priv->fft = pffft_new_setup (priv->fft_size, PFFFT_COMPLEX);
  if (!priv->fft)
    {
      g_critical ("HyScanConvolution: can't setup fft");
      return FALSE;
    }

  /* Копируем образ сигнала. */
  priv->fft_image = pffft_aligned_malloc (priv->fft_size * sizeof(HyScanComplexFloat));
  memset (priv->fft_image, 0, priv->fft_size * sizeof(HyScanComplexFloat));
  memcpy (priv->fft_image, image, n_points * sizeof(HyScanComplexFloat));

  /* Подготавливаем образ к свёртке и делаем его комплексно сопряжённым. */
  image_buff = pffft_aligned_malloc (priv->fft_size * sizeof(HyScanComplexFloat));

  pffft_transform_ordered (priv->fft,
                           (const gfloat*) priv->fft_image,
                           (gfloat*) image_buff,
                           (gfloat*) priv->wbuff,
                           PFFFT_FORWARD);

  for (i = 0; i < priv->fft_size; i++)
    image_buff[i].im = -image_buff[i].im;

  pffft_zreorder (priv->fft,
                  (const gfloat*) image_buff,
                  (gfloat*) priv->fft_image,
                  PFFFT_BACKWARD);

  pffft_aligned_free (image_buff);

  return TRUE;
}

/* Функция выполняет свертку данных.
 * Свертка выполняется блоками по fft_size элементов, при этом каждый следующий блок смещен относительно
 * предыдущего на (fft_size / 2) элементов. Входные данные находятся в ibuff, где над ними производится
 * прямое преобразование Фурье с сохранением результата в obuff, но уже без перекрытия, т.е. с шагом fft_size.
 * Затем производится перемножение ("свертка") с нужным образ сигнала и обратное преобразование Фурье
 * в ibuff. Таким образом в ibuff оказываются необходимые данные, разбитые на некоторое число блоков,
 * в каждом из которых нам нужны только первые (fft_size / 2) элементов. Так как операции над блоками
 * происходят независимо друг от друга этот процесс можно выполнять параллельно, что и производится
 * за счет использования библиотеки OpenMP. */
gboolean
hyscan_convolution_convolve (HyScanConvolution  *convolution,
                             HyScanComplexFloat *data,
                             guint32             n_points,
                             gfloat              scale)
{
  HyScanConvolutionPrivate *priv;

  guint32 full_size;
  guint32 half_size;
  gint32 n_fft;
  gint32 i;

  g_return_val_if_fail (HYSCAN_IS_CONVOLUTION (convolution), FALSE);

  priv = convolution->priv;

  /* Свёртка невозможна. */
  if (priv->fft == NULL || priv->fft_image == NULL)
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

      /* Обнуляем выходной буфер, т.к. функция zconvolve_accumulate добавляет полученный результат
         к значениям в этом буфере (нам это не нужно) ...*/
      memset (priv->ibuff + offset,
              0,
              full_size * sizeof(HyScanComplexFloat));

      /* ... и выполняем свёртку. */
      pffft_zconvolve_accumulate (priv->fft,
                                  (const gfloat*) (priv->obuff + offset),
                                  (const gfloat*) priv->fft_image,
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
