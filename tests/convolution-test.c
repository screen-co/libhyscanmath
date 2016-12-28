#include <hyscan-convolution.h>
#include <hyscan-signal.h>

#include <glib/gstdio.h>
#include <string.h>
#include <math.h>

int
main (int argc, char **argv)
{
  gdouble frequency = 0.0;        /* Частота сигнала. */
  gdouble bandwidth = 0.0;        /* Полоса для ЛЧМ сигнала. */
  gdouble duration = 0.0;         /* Длительность сигнала. */
  gdouble discretization = 0.0;   /* Частота дискретизации. */
  gdouble error = 1.0;            /* Допустимая ошибка. */
  gchar *signal = NULL;           /* Тип сигнала. */

  HyScanConvolution *convolution;
  HyScanComplexFloat *image;
  HyScanComplexFloat *data;
  gfloat *amplitude;
  gdouble square1;
  gdouble square2;
  guint image_size;
  guint data_size;
  guint i, j;

  /* Разбор командной строки. */
  {
    gchar **args;
    GError *error = NULL;
    GOptionContext *context;
    GOptionEntry entries[] =
      {
        { "discretization", 'd', 0, G_OPTION_ARG_DOUBLE, &discretization, "Signal discretization, Hz", NULL },
        { "frequency", 'f', 0, G_OPTION_ARG_DOUBLE, &frequency, "Signal frequency, Hz", NULL },
        { "bandwidth", 'w', 0, G_OPTION_ARG_DOUBLE, &bandwidth, "LFM signal bandwidth, Hz", NULL },
        { "duration", 't', 0, G_OPTION_ARG_DOUBLE, &duration, "Signal duration, s", NULL },
        { "error", 'e', 0, G_OPTION_ARG_DOUBLE, &error, "Admissible error, %", NULL },
        { "signal", 's', 0, G_OPTION_ARG_STRING, &signal, "Signal type (tone, lfm)", NULL },
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
        g_message (error->message);
        return -1;
      }

    if (signal == NULL)
      signal = g_strdup ("tone");

    if ((discretization < 1.0) || (frequency < 1.0) || (duration < 1e-7))
      {
        g_print ("%s", g_option_context_get_help (context, FALSE, NULL));
        return 0;
      }

    if (bandwidth < 1.0)
      bandwidth = 0.2 * frequency;

    g_option_context_free (context);
    g_strfreev (args);
  }

  /* Объект выполнения свёртки. */
  convolution = hyscan_convolution_new ();

  /* Создаём образец сигнала для свёртки. */
  if (g_strcmp0 (signal, "tone") == 0)
    {
      image = hyscan_signal_image_tone (discretization,
                                        frequency,
                                        duration,
                                        &image_size);
    }
  else if (g_strcmp0 (signal, "lfm") == 0)
    {
      image = hyscan_signal_image_lfm (discretization,
                                       frequency - (bandwidth / 2.0),
                                       frequency + (bandwidth / 2.0),
                                       duration,
                                       &image_size);
    }
  else
    {
      g_error ("unsupported signal %s", signal);
    }

  /* Тестовые данные для проверки свёртки. Массив размером 4 * signal_size.
     Сигнал располагается со смещением в две длительности. Все остальные индексы
     массива заполнены нулями. */
  data_size = 4 * image_size;
  data = g_new0 (HyScanComplexFloat, data_size);
  amplitude = g_new0 (gfloat, data_size);

  /* Данные для свёртки. */
  for (i = 2 * image_size, j = 0; j < image_size; i++, j++)
    data[i] = image[j];

  /* Выполняем свёртку. */
  hyscan_convolution_set_image (convolution, image, image_size);
  hyscan_convolution_convolve (convolution, data, data_size);

  /* Для тонального сигнала проверяем, что его свёртка совпадает с треугольником,
     начинающимся с signal_size, пиком на 2 * signal_size и спадающим до 3 * signal_size. */
  if (g_strcmp0 (signal, "tone") == 0)
    {
      for (i = 2 * image_size, j = 0; j < image_size; j++)
        {
          if (j == 0)
            {
              amplitude[i] = 1.0;
            }
          else
            {
              amplitude[i + j] = 1.0 - (gfloat) j / image_size;
              amplitude[i - j] = 1.0 - (gfloat) j / image_size;
            }
        }
    }

  /* Для ЛЧМ сигнала проверяем, что его свёртка совпадает с функцией sinc, симметричной
     относительно 2 * signal_size. */
  else if (g_strcmp0 (signal, "lfm") == 0)
    {
      for (i = 2 * image_size, j = 0; j < image_size; j++)
        {
          gdouble time = j * (1.0 / discretization);
          gdouble phase = 2.0 * M_PI * frequency * time;

          phase = G_PI * time * bandwidth;

          if (j == 0)
            {
              amplitude[i] = 1.0;
            }
          else
            {
              amplitude[i + j] = fabs (sin (phase) / phase);
              amplitude[i - j] = fabs (sin (phase) / phase);
            }
        }

    }

  /* Разница между аналитическим видом свёртки и реально полученным. */
  square1 = 0.0;
  square2 = 0.0;
  for (i = 0; i < data_size; i++)
    {
      square1 += amplitude[i];
      square2 += sqrt (data[i].re * data[i].re + data[i].im * data[i].im);
    }

  if ((100.0 * (fabs (square1 - square2) / square1)) > error)
    g_error ("convolution error %.3f%% > %.3f%%", 100.0 * (fabs (square1 - square2) / square1), error);

  g_message ("done");

  /* Удаляем объект свёртки. */
  g_object_unref (convolution);

  g_free (image);
  g_free (signal);
  g_free (amplitude);
  g_free (data);

  return 0;
}
