
add_executable (fft-test fft-test.c)
add_executable (convolution-test convolution-test.c)

target_link_libraries (fft-test ${GLIB2_LIBRARIES} ${MATH_LIBRARIES} ${HYSCAN_LIBRARIES} hyscanmath)
target_link_libraries (convolution-test ${GLIB2_LIBRARIES} ${MATH_LIBRARIES} ${HYSCAN_LIBRARIES} hyscanmath)
