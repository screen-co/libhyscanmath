
set (TEST_LIBRARIES ${GLIB2_LIBRARIES}
                    ${MATH_LIBRARIES}
                    ${HYSCAN_LIBRARIES}
                    hyscanmath)

add_executable (fft-test fft-test.c)
add_executable (convolution-test convolution-test.c)

target_link_libraries (fft-test ${TEST_LIBRARIES})
target_link_libraries (convolution-test ${TEST_LIBRARIES})
