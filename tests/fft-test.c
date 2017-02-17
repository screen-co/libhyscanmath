#include <math.h>
#include <string.h>
#include <hyscan-convolution.h>

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
               524288};

int
main (int    argc,
      char **argv)
{
  HyScanConvolution *convolution;
  HyScanComplexFloat *image;
  HyScanComplexFloat *data;
  GTimer *timer;

  guint factor;
  guint i, j, k;

  factor = 1;
  timer = g_timer_new ();
  convolution = hyscan_convolution_new ();

  if ((argc == 2) && (g_strcmp0 (argv[1], "mp") == 0))
    factor = 32;

  for (i = 0; i < sizeof (fft_sizes) / sizeof (guint); i++)
    {
      image = g_new0 (HyScanComplexFloat, fft_sizes[i]);
      data = g_new0 (HyScanComplexFloat, factor * fft_sizes[i]);

      g_print ("fft %d: ", fft_sizes[i]);

      for (j = 0; j < fft_sizes[i]; j++)
        {
          gdouble phase = (2.0 * G_PI) / (fft_sizes[i] - 1) * j;
          image[j].re = cos (phase);
          image[j].im = sin (phase);
        }

      if (!hyscan_convolution_set_image (convolution, image, fft_sizes[i]))
        {
          g_print ("failed\n");
          return -1;
        }
      else
        {
          guint convs = 0;
          gdouble elapsed = 0.0;

          while ((elapsed < 1.0) || (convs < 2))
            {
              for (k = 0; k < factor; k++)
                memcpy (data + (k * fft_sizes[i]), image, fft_sizes[i] * sizeof (HyScanComplexFloat));

              g_timer_start (timer);
              hyscan_convolution_convolve (convolution, data, factor * fft_sizes[i]);
              elapsed += g_timer_elapsed (timer, NULL);

              convs += 1;
            }
          g_print ("%f conv/s\n", convs / elapsed);
        }

      g_free (image);
      g_free (data);
    }

  g_object_unref (convolution);
  g_timer_destroy (timer);

  g_print ("All done\n");

  return 0;
}
