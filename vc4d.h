#ifndef VC4D_H
#define VC4D_H

#include <exec/types.h>
#include <exec/libraries.h>
#include <dos/dos.h>
#include <graphics/gfxbase.h>
#include <Warp3D/Warp3D.h>

#ifdef PISTORM32
#include "vc4.h"
#endif

#define VC4D_VERSION    3 // TODO: v4
#define VC4D_REVISION   0

#define VC4D_STR(x) VC4D_XSTR(x)
#define VC4D_XSTR(x) #x

#define VC4_SHADER_MEM_SIZE (64*1024)
#define VC4_UNIFORM_MEM_SIZE (4*1024)

#define LE32(x) __builtin_bswap32(x)

static inline ULONG LE32F(float f)
{
    union {
        ULONG u;
        float f;
    } u = { .f = f };
    return LE32(u.u);
}

typedef struct VC4D {
    struct Library lib;

    struct ExecBase* sysbase;
    struct DosBase* dosbase;
    struct GfxBase* gfxbase;
    struct Library* utilbase;
    struct Library* cgfxbase;
    BPTR seglist;

#ifdef PISTORM32
    vc4_mem shader_mem;
    vc4_mem uniform_mem;
#endif
} VC4D;

typedef struct VC4D_Context {
    W3D_Context w3d;
    W3D_Color fixedcolor;
} VC4D_Context;

typedef struct VC4D_Texture {
    W3D_Texture w3d;
    ULONG texenv;
    W3D_Color envcolor;
} VC4D_Texture;

#define SYSBASE struct ExecBase * const SysBase = vc4d->sysbase
#define DOSBASE struct DosBase * const DOSBase = vc4d->dosbase
#define GFXBASE struct GfxBase * const GfxBase = vc4d->gfxbase
#define UTILBASE struct Library * const UtilityBase = vc4d->utilbase
#define CGFXBASE struct Library* CyberGfxBase = vc4d->cgfxbase

#define LOG_DEBUG(...) log_debug(vc4d, __VA_ARGS__)
#define LOG_ERROR(...) log_debug(vc4d, "ERROR: " __VA_ARGS__)

extern void log_debug(VC4D* vc4d, const char* fmt, ...);

#endif
