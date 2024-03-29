
add_library (${HYSCAN_MATH_LIBRARY} SHARED
             pffft.c
             hyscan-signal.c
             hyscan-echo-svp.c
             hyscan-convolution.c
             hyscan-inter2-doa.c
             hyscan-ahrs.c
             hyscan-ahrs-mahony.c
             hyscan-fft.c)

target_link_libraries (${HYSCAN_MATH_LIBRARY} ${GLIB2_LIBRARIES} ${MATH_LIBRARIES} ${HYSCAN_LIBRARIES})

set_target_properties (${HYSCAN_MATH_LIBRARY} PROPERTIES DEFINE_SYMBOL "HYSCAN_API_EXPORTS")
set_target_properties (${HYSCAN_MATH_LIBRARY} PROPERTIES SOVERSION ${HYSCAN_MATH_VERSION})

configure_file (hyscanmath.pc.in
                "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/hyscanmath-${HYSCAN_MAJOR_VERSION}.pc"
                @ONLY)

install (TARGETS ${HYSCAN_MATH_LIBRARY}
         COMPONENT runtime
         RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
         LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
         PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)

install (FILES "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/hyscanmath-${HYSCAN_MAJOR_VERSION}.pc"
         COMPONENT development
         DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig"
         PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)

install (FILES hyscan-signal.h
               hyscan-echo-svp.h
               hyscan-convolution.h
               hyscan-inter2-doa.h
               hyscan-ahrs.h
               hyscan-ahrs-mahony.h
               hyscan-fft.h
         COMPONENT development
         DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/hyscan-${HYSCAN_MAJOR_VERSION}/hyscanmath"
         PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)
