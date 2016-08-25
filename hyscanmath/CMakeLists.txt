
add_library (hyscanmath SHARED
             pffft.c
             hyscan-convolution.c)

target_link_libraries (hyscanmath ${ADDON_LIBRARIES} ${GLIB2_LIBRARIES} ${HYSCAN_LIBRARIES})
