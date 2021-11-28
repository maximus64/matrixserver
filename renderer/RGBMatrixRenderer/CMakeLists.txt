project(RGBMatrixRenderer)

if (${BUILD_RASPBERRYPI})
    find_library(RGB_LIBRARIES NAMES librgbmatrix.a REQUIRED)
    find_path(RGB_INCDIR NAMES rgbmatrix/led-matrix.h REQUIRED)

    include_directories("${RGB_INCDIR}/rgbmatrix")

    set(SOURCE_FILES
            RGBMatrixRenderer.cpp
            )

    add_library(RGBMatrixRenderer SHARED ${SOURCE_FILES})
    target_link_libraries(RGBMatrixRenderer PRIVATE renderer Imu ${RGB_LIBRARIES})
    target_include_directories(RGBMatrixRenderer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

    install(TARGETS RGBMatrixRenderer
            LIBRARY
            DESTINATION lib
            COMPONENT RGBMatrixRenderer
            )
endif ()

add_subdirectory(imu)
