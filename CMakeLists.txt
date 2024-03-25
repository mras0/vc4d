cmake_minimum_required(VERSION 3.14)
project(vc4d C CXX)

if (NOT ${CMAKE_SYSTEM_NAME} STREQUAL "AmigaOS")
    message(FATAL_ERROR "Unknown platform")
endif()

add_compile_options(-mregparm=4 -g3 -O2 -msmall-code -fomit-frame-pointer -Wall -Wextra)


set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11 -Wno-pointer-sign")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fbbb=- -std=c++17 -fno-rtti -fno-exceptions -fno-builtin")

add_executable(test test.c)

# Maybe much later this can be removed
set_source_files_properties(warp3d.c PROPERTIES COMPILE_FLAGS -Wno-unused-parameter)

set_source_files_properties(cppsupport.c PROPERTIES COMPILE_FLAGS -fno-aggressive-loop-optimizations)

add_executable(vc4d.library main.c warp3d.c debug.c cppsupport.c draw.cpp)
target_link_options(vc4d.library PRIVATE -ffreestanding -nostdlib -nostartfiles)
#target_compile_options(vc4d.library PRIVATE -mpcrel)
