cmake_minimum_required(VERSION 3.14)
project(vc4d C)

if (NOT ${CMAKE_SYSTEM_NAME} STREQUAL "AmigaOS")
    message(FATAL_ERROR "Unknown platform")
endif()

add_compile_options(-std=gnu11 -O3 -m68040 -msmall-code -mregparm=4 -fomit-frame-pointer -Wall -Wextra -Wno-pointer-sign)

add_executable(test test.c)

# Maybe much later this can be removed
set_source_files_properties(warp3d.c PROPERTIES COMPILE_FLAGS -Wno-unused-parameter)

add_executable(vc4d.library main.c warp3d.c debug.c)
target_link_options(vc4d.library PRIVATE -ffreestanding -nostdlib -nostartfiles)
#target_compile_options(vc4d.library PRIVATE -mpcrel)
