#ifndef __HYSCAN_MATH_EXPORTS_H__
#define __HYSCAN_MATH_EXPORTS_H__

#if defined (_WIN32)
  #if defined (hyscanmath_EXPORTS)
    #define HYSCAN_MATH_EXPORT __declspec (dllexport)
  #else
    #define HYSCAN_MATH_EXPORT __declspec (dllimport)
  #endif
#else
  #define HYSCAN_MATH_EXPORT
#endif

#endif /* __HYSCAN_MATH_EXPORTS_H__ */
