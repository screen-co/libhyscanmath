/**
 * \file hyscan-convolution.h
 *
 * \brief Заголовочный файл класса свёртки данных
 * \author Andrei Fadeev (andrei@webcontrol.ru)
 * \date 2015
 * \license Проприетарная лицензия ООО "Экран"
 *
 * \defgroup HyScanConvolution HyScanConvolution - класс свёртки данных
 *
 * Класс HyScanConvolution используется для выполнения свёртки данных с образом. Данные и образ
 * для свёртки должны быть представлены в комплексном виде (\link HyScanComplexFloat \endlink).
 *
 * Объект для выполнения свёртки создаётся функцией #hyscan_convolution_new.
 *
 * Перед выполнением свёртки необходимо задать образ с которым будет выполняться свёртка. Для
 * этого предназначена функция #hyscan_convolution_set_image. Образец для свёртки можно изменять
 * в процессе работы с объектом. При повторных вызовах функции #hyscan_convolution_set_image
 * будет установлен новый образ для свёртки.
 *
 * Функция #hyscan_convolution_convolve выполняет свертку данных.
 *
 * HyScanConvolution не поддерживает работу в многопоточном режиме.
 *
 */

#ifndef __HYSCAN_CONVOLUTION_H__
#define __HYSCAN_CONVOLUTION_H__

#include <glib-object.h>
#include <hyscan-types.h>

G_BEGIN_DECLS

#define HYSCAN_TYPE_CONVOLUTION             (hyscan_convolution_get_type ())
#define HYSCAN_CONVOLUTION(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), HYSCAN_TYPE_CONVOLUTION, HyScanConvolution))
#define HYSCAN_IS_CONVOLUTION(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HYSCAN_TYPE_CONVOLUTION))
#define HYSCAN_CONVOLUTION_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), HYSCAN_TYPE_CONVOLUTION, HyScanConvolutionClass))
#define HYSCAN_IS_CONVOLUTION_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), HYSCAN_TYPE_CONVOLUTION))
#define HYSCAN_CONVOLUTION_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), HYSCAN_TYPE_CONVOLUTION, HyScanConvolutionClass))

typedef struct _HyScanConvolution HyScanConvolution;
typedef struct _HyScanConvolutionPrivate HyScanConvolutionPrivate;
typedef struct _HyScanConvolutionClass HyScanConvolutionClass;

struct _HyScanConvolution
{
  GObject parent_instance;

  HyScanConvolutionPrivate *priv;
};

struct _HyScanConvolutionClass
{
  GObjectClass parent_class;
};

HYSCAN_API
GType hyscan_convolution_get_type (void);

/**
 *
 * Функция создаёт новый объект \link HyScanConvolution \endlink.
 *
 * \return Указатель на объект \link HyScanConvolution \endlink.
 *
 *
 */
HYSCAN_API
HyScanConvolution     *hyscan_convolution_new          (void);

/**
 *
 * Функция задаёт образ сигнала для свёртки.
 *
 * \param convolution указатель на объект \link HyScanConvolution \endlink;
 * \param image образ для свёртки;
 * \param n_points размер образа в точках.
 *
 * \return TRUE - если образ для свёртки установлен, FALSE - в случае ошибки.
 *
 */
HYSCAN_API
gboolean               hyscan_convolution_set_image    (HyScanConvolution         *convolution,
                                                        const HyScanComplexFloat  *image,
                                                        guint32                    n_points);

/**
 *
 * Функция выполняет свёртку данных с образом. Результат свёртки помещается
 * во входной массив. При свёртке производится автоматическое нормирование
 * на размер образа свёртки. Пользователь может указать дополнительный
 * коэффициент на который будут домножены данные после свёртки.
 *
 * \param convolution указатель на объект \link HyScanConvolution \endlink;
 * \param data данные для свёртки;
 * \param n_points размер данных в точках;
 * \param scale коэффициент масштабирования.
 *
 * \return TRUE - если свёртка выполнена, FALSE - в случае ошибки.
 *
 */
HYSCAN_API
gboolean               hyscan_convolution_convolve     (HyScanConvolution         *convolution,
                                                        HyScanComplexFloat        *data,
                                                        guint32                    n_points,
                                                        gfloat                     scale);

G_END_DECLS

#endif /* __HYSCAN_CONVOLUTION_H__ */
