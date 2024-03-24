#ifndef VC4D_H
#define VC4D_H

#include <exec/types.h>
#include <exec/libraries.h>
#include <dos/dos.h>
#include <graphics/gfxbase.h>
#include <Warp3D/Warp3D.h>

#define VC4D_VERSION    3 // TODO: v4
#define VC4D_REVISION   0

#define VC4D_STR(x) VC4D_XSTR(x)
#define VC4D_XSTR(x) #x

typedef struct VC4D {
    struct Library lib;

    struct ExecBase* sysbase;
    struct DosBase* dosbase;
    struct GfxBase* gfxbase;
    struct Library* utilbase;
    struct Library* cgfxbase;
    BPTR seglist;

    BOOL initialized;
} VC4D;

#define SYSBASE struct ExecBase * const SysBase = vc4d->sysbase
#define DOSBASE struct DosBase * const DOSBase = vc4d->dosbase
#define GFXBASE struct GfxBase * const GfxBase = vc4d->gfxbase
#define UTILBASE struct Library * const UtilityBase = vc4d->utilbase
#define CGFXBASE struct Library* CyberGfxBase = vc4d->cgfxbase

#define LOG_DEBUG(...) log_debug(vc4d, __VA_ARGS__)
#define LOG_ERROR(...) log_debug(vc4d, "ERROR: " __VA_ARGS__)

extern void log_debug(VC4D* vc4d, const char* fmt, ...);

#endif
