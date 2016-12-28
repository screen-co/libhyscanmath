
add_library (hyscanmath SHARED
             pffft.c
             hyscan-signal.c
             hyscan-convolution.c)

target_link_libraries (hyscanmath ${GLIB2_LIBRARIES} ${MATH_LIBRARIES} ${HYSCAN_LIBRARIES})

set_target_properties (hyscanmath PROPERTIES DEFINE_SYMBOL "HYSCAN_API_EXPORTS")
