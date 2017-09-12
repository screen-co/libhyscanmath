
set (TEST_LIBRARIES ${GLIB2_LIBRARIES}
                    ${MATH_LIBRARIES}
                    ${HYSCAN_LIBRARIES}
                    ${HYSCAN_MATH_LIBRARY})

add_executable (fft-test fft-test.c)
add_executable (convolution-test convolution-test.c)

target_link_libraries (fft-test ${TEST_LIBRARIES})
target_link_libraries (convolution-test ${TEST_LIBRARIES})

install (TARGETS fft-test
                 convolution-test
         COMPONENT test
         RUNTIME DESTINATION bin
         LIBRARY DESTINATION lib
         ARCHIVE DESTINATION lib
         PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)
