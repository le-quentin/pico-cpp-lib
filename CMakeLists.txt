cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)
if (PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

project(
        pico-cpp-lib VERSION 1.0.0 
        DESCRIPTION "A c++ library to control various electronic components with Raspberry Pi Pico" 
        LANGUAGES C CXX ASM
)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS True)

# Set the environment install dirs
include(GNUInstallDirs)

# Initialize the SDK
pico_sdk_init()

add_library(pico-cpp-lib STATIC
    src/FourDigitsDisplay.cpp
    src/PushButton.cpp
    src/ir-remotes/TDJL20KEYS.cpp
    src/ir-remotes/protocols/NEC.cpp
)
set_target_properties(pico-cpp-lib PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER "include/IrqFifo.h;include/FourDigitsDisplay.h;include/PushButton.h;include/ir-remotes/TDJL20KEYS.h;include/ir-remotes/protocols/NEC.h")

configure_file(pico-cpp-lib.pc.in pico-cpp-lib.pc @ONLY)

target_include_directories(pico-cpp-lib PUBLIC include)

# pull in common dependencies
target_link_libraries(pico-cpp-lib pico_stdlib)

# install the lib
 install(TARGETS pico-cpp-lib
     LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY "include/" # source directory
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/pico-cpp-lib" # target directory
        FILES_MATCHING # install only matched files
        PATTERN "*.h" # select header files
)
install(FILES ${CMAKE_BINARY_DIR}/pico-cpp-lib.pc
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)
