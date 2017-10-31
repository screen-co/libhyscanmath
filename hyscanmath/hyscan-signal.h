/**
 * \file hyscan-signal.h
 *
 * \brief Заголовочный файл функций для расчёта образов сигналов
 * \author Andrei Fadeev (andrei@webcontrol.ru)
 * \date 2016
 * \license Проприетарная лицензия ООО "Экран"
 *
 * \defgroup HyScanSignal HyScanSignal - функции для расчёта образцов сигналов
 *
 * #hyscan_signal_image_tone - функция для расчёта образа тонального сигнала;
 * #hyscan_signal_image_lfm - функция для расчёта образа ЛЧМ сигнала.
 *
 */

#ifndef __HYSCAN_SIGNAL_H__
#define __HYSCAN_SIGNAL_H__

#include <hyscan-types.h>

G_BEGIN_DECLS

/**
 *
 * Функция расчитывает образ тонального сигнала для выполнения свёртки. После
 * использования память должна быть освобождена функцией g_free.
 *
 * \param disc_freq частота дискретизаци  сигнала, Гц;
 * \param signal_freq несущая частота сигнала, Гц;
 * \param duration длительность сигнала, с;
 * \param n_points расчитанный размер образа сигнала в точках.
 *
 * \return Указатель на образ сигнала.
 *
 */
HYSCAN_API
HyScanComplexFloat    *hyscan_signal_image_tone        (gdouble                disc_freq,
                                                        gdouble                signal_freq,
                                                        gdouble                duration,
                                                        guint                 *n_points);

/**
 *
 * Функция расчитывает образ сигнала ЛЧМ для выполнения свёртки. Функция может
 * создавать образы для ЛЧМ сигнала с возрастающей или убывающей частотой. После
 * использования память должна быть освобождена функцией g_free.
 *
 * \param disc_freq частота дискретизаци  сигнала, Гц;
 * \param start_freq начальная частота сигнала, Гц;
 * \param end_freq конечная частота сигнала, Гц;
 * \param duration длительность сигнала, с;
 * \param n_points расчитанный размер образа сигнала в точках.
 *
 * \return Указатель на образ сигнала.
 *
 */
HYSCAN_API
HyScanComplexFloat    *hyscan_signal_image_lfm         (gdouble                disc_freq,
                                                        gdouble                start_freq,
                                                        gdouble                end_freq,
                                                        gdouble                duration,
                                                        guint                 *n_points);

G_END_DECLS

#endif /* __HYSCAN_SIGNAL_H__ */
