
add_library (${HYSCAN_MATH_LIBRARY} SHARED
             pffft.c
             hyscan-signal.c
             hyscan-convolution.c)

target_link_libraries (${HYSCAN_MATH_LIBRARY} ${GLIB2_LIBRARIES} ${MATH_LIBRARIES} ${HYSCAN_LIBRARIES})

set_target_properties (${HYSCAN_MATH_LIBRARY} PROPERTIES DEFINE_SYMBOL "HYSCAN_API_EXPORTS")
set_target_properties (${HYSCAN_MATH_LIBRARY} PROPERTIES SOVERSION ${HYSCAN_MATH_VERSION})

configure_file (hyscanmath.pc.in
                "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/hyscanmath-${HYSCAN_MAJOR_VERSION}.pc"
                @ONLY)

install (TARGETS ${HYSCAN_MATH_LIBRARY}
         COMPONENT runtime
         RUNTIME DESTINATION bin
         LIBRARY DESTINATION lib
         ARCHIVE DESTINATION lib
         PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)

install (FILES "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/hyscanmath-${HYSCAN_MAJOR_VERSION}.pc"
         COMPONENT development
         DESTINATION "lib/pkgconfig"
         PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)

install (FILES hyscan-signal.h
               hyscan-convolution.h
         COMPONENT development
         DESTINATION "include/hyscan-${HYSCAN_MAJOR_VERSION}/hyscanmath"
         PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)
