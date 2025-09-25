project(hwlib-sim C)
cmake_minimum_required(VERSION 3.25)

# Find ZeroMQ dependency
find_package(PkgConfig REQUIRED)
pkg_check_modules(ZeroMQ REQUIRED libzmq)

# Find Simulith headers
set(SIMULITH_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../../simulith" CACHE PATH "Path to Simulith directory")
if(NOT EXISTS "${SIMULITH_DIR}/include/simulith_transport.h")
    message(FATAL_ERROR "Could not find simulith_transport.h in ${SIMULITH_DIR}/include. Please set SIMULITH_DIR to the correct path.")
endif()

# Prefer a pre-built simulith static library if present in the simulith build dir.
set(SIMULITH_BUILD_LIB "${SIMULITH_DIR}/build/libsimulith.a")
if(EXISTS "${SIMULITH_BUILD_LIB}")
    message(STATUS "Using prebuilt simulith: ${SIMULITH_BUILD_LIB}")
    add_library(simulith STATIC IMPORTED)
    set_target_properties(simulith PROPERTIES IMPORTED_LOCATION "${SIMULITH_BUILD_LIB}")
else()
    # Only add Simulith subdirectory if not already added
    if(NOT TARGET simulith)
        add_subdirectory(${SIMULITH_DIR} ${CMAKE_BINARY_DIR}/simulith)
    endif()
endif()

include_directories(inc
                    ${MISSION_SOURCE_DIR}/components/hwlib/fsw/public_inc
                    ${MISSION_SOURCE_DIR}/osal/src/os/inc
                    ${MISSION_SOURCE_DIR}/psp/fsw/inc
                    ${SIMULITH_DIR}/include
                    ${ZeroMQ_INCLUDE_DIRS}
                    )

aux_source_directory(src NOSLINK_SRC)

add_library(hwlib-sim STATIC ${NOSLINK_SRC})
target_link_libraries(hwlib-sim 
    simulith
    ${ZeroMQ_LIBRARIES}
    gcov
)
