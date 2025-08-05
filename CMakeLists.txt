cmake_minimum_required(VERSION 3.25)
project(CFS_HWLIB C)

include(../../../comp/CompFlags.cmake)

# hwlib include directories
include_directories(fsw/mission_inc
                    fsw/platform_inc
                    fsw/public_inc)

# include directories for flight configuration
IF(OSAL_SYSTEM_BSPTYPE STREQUAL "tryspace-linux")
    message(STATUS "hwlib: NON-FLIGHT configuration")
ELSE()
    message(STATUS "hwlib: linux configuration")
    aux_source_directory(fsw/linux APP_SRC_FILES)
ENDIF()

# create the hwlib app module
aux_source_directory(fsw/src APP_SRC_FILES)

# stub out hardware for all non-flight configurations
IF(OSAL_SYSTEM_BSPTYPE STREQUAL "tryspace-linux")
    # Include Simulith headers and sim source files directly
    set(SIMULITH_BUILD_DIR "${CMAKE_SOURCE_DIR}/../simulith/build")

    # Add include directories for sim headers
    list(APPEND HWLIB_SIM_INCLUDE_DIRS
        sim/inc
        ../../../simulith/include)
    
    # Add sim source files and torquer_lib.c to APP_SRC_FILES
    list(APPEND APP_SRC_FILES
        sim/src/libcan.c
        sim/src/libgpio.c
        sim/src/libi2c.c
        sim/src/libmem.c
        sim/src/libsocket.c
        sim/src/libspi.c
        sim/src/libtrq.c
        sim/src/libuart.c)

    # Actually set the include directories
    include_directories(${HWLIB_SIM_INCLUDE_DIRS})

    # Link to pre-built Simulith library and ZeroMQ
    if(EXISTS "${SIMULITH_BUILD_DIR}/libsimulith.a")
        message(STATUS "hwlib: Using pre-built Simulith library: ${SIMULITH_BUILD_DIR}/libsimulith.a")
        set(HWLIB_EXTRA_LIBS ${SIMULITH_BUILD_DIR}/libsimulith.a zmq)
    else()
        message(WARNING "hwlib: Pre-built Simulith library not found at ${SIMULITH_BUILD_DIR}/libsimulith.a")
        message(STATUS "hwlib: Attempting to link with simulith target")
        set(HWLIB_EXTRA_LIBS simulith zmq)
    endif()
ELSEIF(CFE_SYSTEM_PSPNAME STREQUAL "pc-linux")
    set(HWLIB_EXTRA_LIBS socketcan)
    add_definitions(-DHWLIB_SOCKETCAN=1)
ENDIF()

add_cfe_app(hwlib ${APP_SRC_FILES})
if(DEFINED HWLIB_EXTRA_LIBS)
    target_link_libraries(hwlib ${HWLIB_EXTRA_LIBS})
endif()
