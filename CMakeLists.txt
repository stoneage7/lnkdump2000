cmake_minimum_required(VERSION 3.0)

project(lnkdump2k VERSION 1.0)

configure_file(config.h.in config.h)

set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
set(CPACK_PACKAGE_VERSION_MAJOR "${lnkdump2k_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${lnkdump2k_VERSION_MINOR}")
include(CPack)

set(OpenGL_GL_PREFERENCE "GLVND")

find_package(FLTK REQUIRED)
find_package(OpenGL REQUIRED)
include_directories(${FLTK_INCLUDE_DIRS})

add_custom_command(
        OUTPUT "lnk.cxx" "lnk.h"
        COMMAND fluid -c ${CMAKE_CURRENT_SOURCE_DIR}/lnk.ui
        DEPENDS lnk.ui
)

add_custom_command(
        OUTPUT "blank.cxx" "blank.h"
        COMMAND fluid -c ${CMAKE_CURRENT_SOURCE_DIR}/blank.ui
        DEPENDS blank.ui
)

add_custom_command(
        OUTPUT "about.cxx" "about.h"
        COMMAND fluid -c ${CMAKE_CURRENT_SOURCE_DIR}/about.ui
        DEPENDS about.ui
)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(
        lnkdump2k main.cpp parse.cpp encoding.cpp output.cpp struct.cpp themes.cpp
        enc_single.inc enc_asian.inc
        lnk.cxx blank.cxx about.cxx
)

set(CMAKE_CXX_FLAGS "-O2 -Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -Wall -Wextra")

target_compile_features(lnkdump2k PUBLIC cxx_std_20)
target_link_libraries(lnkdump2k fltk fltk_images -static-libgcc -static-libstdc++)


install(TARGETS lnkdump2k RUNTIME DESTINATION bin)
