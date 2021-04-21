
cmake_minimum_required (VERSION 3.0)

project (libhyscanmath)

enable_testing()

set (HYSCAN_MAJOR_VERSION 5)
set (HYSCAN_MATH_VERSION 0)
set (HYSCAN_MATH_LIBRARY "hyscanmath-${HYSCAN_MAJOR_VERSION}")

include (FindPkgConfig)

if (NOT CMAKE_BUILD_TYPE)
  set (CMAKE_BUILD_TYPE Release)
endif ()

if (NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
  set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
  set (CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/bin")
  set (CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/bin")
endif ()

if (NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
  set (CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
  set (CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/bin")
  set (CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/bin")
endif ()

if (NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
  set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
  set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/bin")
  set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/bin")
endif ()

if (${CMAKE_C_COMPILER_ID} STREQUAL GNU)
  set (DEBUG_OPT_LEVEL "-Og")
elseif (${CMAKE_C_COMPILER_ID} STREQUAL Clang)
  set (DEBUG_OPT_LEVEL "-O1")
endif ()

if (${CMAKE_C_COMPILER_ID} STREQUAL GNU OR ${CMAKE_C_COMPILER_ID} STREQUAL Clang)
  if (HYSCAN_OPEN_MP)
    set (OMP_FLAGS "-fopenmp")
  endif ()
  set (C_FLAGS "-Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers ${OMP_FLAGS} $ENV{CFLAGS}")
  set (CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2 ${C_FLAGS}")
  set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${DEBUG_OPT_LEVEL} ${C_FLAGS}")
  set (MATH_LIBRARIES m)
elseif (${CMAKE_C_COMPILER_ID} STREQUAL MSVC)
  if (HYSCAN_OPEN_MP)
    set (OMP_FLAGS "/openmp")
  endif ()
  set (C_FLAGS "/wd4244 ${OMP_FLAGS} $ENV{CFLAGS}")
  set (CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} ${C_FLAGS}")
  set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${C_FLAGS}")
else ()
  message (FATAL_ERROR "Unsupported compiler ${CMAKE_C_COMPILER_ID}")
endif ()

if (HYSCAN_OPEN_MP)
  add_definitions (-DHYSCAN_OPEN_MP)
endif ()

pkg_check_modules (GLIB2 REQUIRED glib-2.0 gobject-2.0 gthread-2.0 gio-2.0)
add_definitions (${GLIB2_CFLAGS})
link_directories (${GLIB2_LIBRARY_DIRS})

if (HYSCAN_SYS_LIBS)
  pkg_check_modules (HYSCAN REQUIRED "hyscantypes-${HYSCAN_MAJOR_VERSION}")
  link_directories (${HYSCAN_LIBRARY_DIRS})
  add_definitions (${HYSCAN_CFLAGS})
else ()
  include_directories ("${CMAKE_CURRENT_SOURCE_DIR}/../libhyscantypes/hyscantypes")
  link_directories ("${CMAKE_CURRENT_SOURCE_DIR}/../libhyscantypes/bin")
  set (HYSCAN_LIBRARIES "hyscantypes-${HYSCAN_MAJOR_VERSION}")
endif ()

include_directories ("${CMAKE_CURRENT_SOURCE_DIR}/hyscanmath")

add_definitions (-DG_LOG_DOMAIN="HyScanMath")
add_subdirectory (hyscanmath)
add_subdirectory (tests)
