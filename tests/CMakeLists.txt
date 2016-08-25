
add_executable (convolution-test convolution-test.c)

target_link_libraries (convolution-test ${ADDON_LIBRARIES} ${GLIB2_LIBRARIES} ${HYSCAN_LIBRARIES} hyscanmath)
