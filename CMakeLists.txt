cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

# Set the environment install dirs
include(GNUInstallDirs)


project(
        pico-cpp-lib VERSION 1.0.0 
        DESCRIPTION "A c++ library to control various electronic components with Raspberry Pi Pico" 
        LANGUAGES C CXX ASM
)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

# Initialize the SDK
pico_sdk_init()

add_library(pico-cpp-lib STATIC
        src/FourDigitsDisplay.cpp
)
set_target_properties(pico-cpp-lib PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER include/FourDigitsDisplay.h)

configure_file(pico-cpp-lib.pc.in pico-cpp-lib.pc @ONLY)

target_include_directories(pico-cpp-lib PUBLIC include)

# pull in common dependencies
target_link_libraries(pico-cpp-lib pico_stdlib)

# install the lib
install(TARGETS pico-cpp-lib
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/pico-cpp-lib)
install(FILES ${CMAKE_BINARY_DIR}/pico-cpp-lib.pc
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)
