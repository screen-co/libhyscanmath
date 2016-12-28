
add_executable (convolution-test convolution-test.c)

target_link_libraries (convolution-test ${GLIB2_LIBRARIES} ${MATH_LIBRARIES} ${HYSCAN_LIBRARIES} hyscanmath)
