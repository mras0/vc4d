cmake_minimum_required(VERSION 3.14)
project(vc4d C CXX)

if (NOT ${CMAKE_SYSTEM_NAME} STREQUAL "AmigaOS")
    message(FATAL_ERROR "Unknown platform")
endif()

find_program(VC4ASM vc4asm)
if (NOT VC4ASM)
    # TODO: Prebake some shaders and include them in repo
    message(FATAL_ERROR "vc4asm not found (https://github.com/maazl/vc4asm)")
endif()

set(QASM_HEADERS "")

add_definitions(-DTRACE_LEVEL=2)

macro(qasm name)
    set(dst ${CMAKE_CURRENT_BINARY_DIR}/${name}.h)
    set(src ${CMAKE_CURRENT_SOURCE_DIR}/qpu/${name}.qasm)
    add_custom_command(
        OUTPUT ${dst}
        COMMAND ${VC4ASM} -C${dst} -V -v ${src}
        DEPENDS ${src} ${ARGN} ${CMAKE_CURRENT_SOURCE_DIR}/qpu/common.qinc ${CMAKE_CURRENT_SOURCE_DIR}/qpu/vc4.qinc ${CMAKE_CURRENT_SOURCE_DIR}/qpu/loop_template.qasm
    )
    set(QASM_HEADERS ${QASM_HEADERS} ${dst})
endmacro()

add_compile_options(-mregparm=4 -g3 -O2 -msmall-code -fomit-frame-pointer)
#add_compile_options(-g3 -O2 -fomit-frame-pointer)
add_compile_options(-Wall -Wextra -fno-strict-aliasing)

# TODO: Make a CMake option for this
if (FALSE)
    set(DRAW_FILES draw_cpu.cpp)
    #cppsupport.c
    #set_source_files_properties(cppsupport.c PROPERTIES COMPILE_FLAGS -fno-aggressive-loop-optimizations)
else()
    add_definitions(-DPISTORM32)
    qasm(outer_loop)
    qasm(loop_z_none)
    qasm(loop_z_less)
    qasm(loop_z_less_update)
    qasm(loop_z_lequal)
    qasm(loop_z_lequal_update)
    qasm(body)
    qasm(load_z)
    qasm(load_tex)
    qasm(load_color)
    qasm(interp_color)
    qasm(tex_lookup_wrap)
    qasm(copy_interpolated_color)
    # TexEnv
    qasm(w3d_modulate)
    qasm(w3d_replace)
    qasm(w3d_decal)
    qasm(w3d_blend)
    # BlendMode
    qasm(alpha_blend)

    qasm(w3d_zero)
    qasm(w3d_one)
    qasm(w3d_src_color)
    qasm(w3d_dst_color)
    qasm(w3d_one_minus_src_color)
    qasm(w3d_one_minus_dst_color)
    qasm(w3d_src_alpha)
    qasm(w3d_one_minus_src_alpha)

    set(DRAW_FILES
        vc4.c
        draw_vc4.c
        ${QASM_HEADERS}
    )
    include_directories(${CMAKE_CURRENT_BINARY_DIR})
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11 -Wno-pointer-sign")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fbbb=- -std=c++17 -fno-rtti -fno-exceptions -fno-builtin")

add_executable(test test.c)

# Maybe much later this can be removed
set_source_files_properties(warp3d.c PROPERTIES COMPILE_FLAGS -Wno-unused-parameter)

add_executable(vc4d.library
    main.c
    warp3d.c
    debug.c
    ${DRAW_FILES}
    )
target_link_options(vc4d.library PRIVATE -ffreestanding -nostdlib -nostartfiles)
#target_compile_options(vc4d.library PRIVATE -mpcrel)
