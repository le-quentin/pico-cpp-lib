cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)
include(../../pico_sdk_import.cmake)

project(ir-decoder C CXX ASM)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

# Initialize the SDK
pico_sdk_init()

add_executable(ir-decoder
        main.cpp
        ir-remotes/TDJL20KEYS.cpp
        ir-remotes/protocols/NEC.cpp
        )

set(COMPILE_DEFINITIONS "" CACHE STRING "A semicolon separated list of pre-processor definitions")
target_compile_definitions(ir-decoder PRIVATE "${COMPILE_DEFINITIONS}")
add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-maybe-uninitialized
        )

# create map/bin/hex file etc. Necessary to make u2f file.
pico_add_extra_outputs(ir-decoder)

# pull in common dependencies of pico SDK
target_link_libraries(ir-decoder pico_stdlib)


# Add pico-cpp-lib library
set(CMAKE_LIBRARY_PATH /usr/local/lib CACHE PATH "/usr/local/lib")
set(PICO_CPP_LIB_INCLUDE_PATH /usr/local/include CACHE PATH "/usr/local/include")
add_library(pico-cpp-lib STATIC IMPORTED)
set_property(TARGET pico-cpp-lib PROPERTY
    IMPORTED_LOCATION "${CMAKE_LIBRARY_PATH}/libpico-cpp-lib.a")
include_directories(${PICO_CPP_LIB_INCLUDE_PATH})
target_link_libraries(ir-decoder pico-cpp-lib)

# Add USB CDC drivers to enable usb stdio, allowing reset by usb
pico_enable_stdio_usb(ir-decoder 1)
pico_enable_stdio_uart(ir-decoder 0)

