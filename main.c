#include "vc4d.h"
#include "draw.h"

#ifdef PISTORM32
#include "vc4.h"
#endif

#include <exec/execbase.h>
#include <exec/resident.h>
#include <exec/initializers.h>
#include <exec/alerts.h>
#include <proto/exec.h>
#include <proto/Warp3D.h> // N.B. Only in this file!

int main(void) {
    return -1;
}
const struct Resident RomTag;

static const char LibName[] = "Warp3D.library";
static const char IdString[] = "vc4d " VC4D_STR(VC4D_VERSION) "." VC4D_STR(VC4D_REVISION) " (DD.MM.YYYY)";

static void DoFatalInitAlert(ULONG num)
{
    // Seems like you need to be in supervisor mode to call Alert with AT_DeadEnd ???
    struct ExecBase * const SysBase = *(struct ExecBase**)4;
    SuperState();
    Alert(AT_DeadEnd | num);
    for (;;)
        ;
}

static APTR LibInit(BPTR seglist asm("a0"), VC4D* vc4d asm("d0"), struct ExecBase *sysbase asm("a6"))
{
	vc4d->lib.lib_Node.ln_Name = (char*) LibName;
	vc4d->lib.lib_IdString = (char*) IdString;

	/* setup private data */
	vc4d->seglist = seglist;
	vc4d->sysbase = sysbase;

    SYSBASE;

    if (!(SysBase->AttnFlags & AFF_68020))
        DoFatalInitAlert(0x68020);

    vc4d->dosbase = (struct DosBase*)OpenLibrary(DOSNAME, 39); // Require KS3.0
    if (!vc4d->dosbase)
        DoFatalInitAlert(AG_OpenLib|AO_DOSLib);

    vc4d->gfxbase = (struct GfxBase*)OpenLibrary("graphics.library", 39);
    if (!vc4d->gfxbase)
        DoFatalInitAlert(AG_OpenLib|AO_GraphicsLib);

    vc4d->utilbase = OpenLibrary("utility.library", 39);
    if (!vc4d->utilbase)
        DoFatalInitAlert(AG_OpenLib|AO_UtilityLib);

    vc4d->cgfxbase = OpenLibrary("cybergraphics.library", 0);
    if (!vc4d->cgfxbase)
        DoFatalInitAlert(AG_OpenLib);

#ifdef PISTORM32
    int res = vc4_init(vc4d);
    if (res)
        DoFatalInitAlert(0x00440000|(res&0xff));
#endif

	return vc4d;
}

static void LibCleanup(VC4D* vc4d asm("a6"))
{
    SYSBASE;
#ifdef PISTORM32
    vc4_free(vc4d);
#endif
    if (vc4d->dosbase)
        CloseLibrary((struct Library*)vc4d->dosbase);
    if (vc4d->gfxbase)
        CloseLibrary((struct Library*)vc4d->gfxbase);
    if (vc4d->utilbase)
        CloseLibrary(vc4d->utilbase);
    if (vc4d->cgfxbase)
        CloseLibrary(vc4d->cgfxbase);
}

static VC4D* LibOpen(VC4D* vc4d asm("a6"))
{
    ++vc4d->lib.lib_OpenCnt;
    vc4d->lib.lib_Flags &= ~LIBF_DELEXP;
    return vc4d;
}

static BPTR LibExpunge(VC4D* vc4d asm("a6"))
{
    SYSBASE;
    if (vc4d->lib.lib_OpenCnt) {
        vc4d->lib.lib_Flags |= LIBF_DELEXP;
        return NULL;
    }
    Remove(&vc4d->lib.lib_Node);
    LibCleanup(vc4d);
    BPTR seglist = vc4d->seglist;
    int neg = vc4d->lib.lib_NegSize;
    FreeMem((char*)vc4d -neg, vc4d->lib.lib_PosSize + neg);
    return seglist;
}

static BPTR LibClose(VC4D * vc4d asm("a6"))
{
    if (--vc4d->lib.lib_OpenCnt)
        return 0;
    if (!(vc4d->lib.lib_Flags & LIBF_DELEXP))
        return 0;
    return LibExpunge(vc4d);
}

static ULONG LibReserved(void)
{
    return 0;
}

const APTR FuncTab[] = {
    &LibOpen,                  // -6
    &LibClose,                 // -12
    &LibExpunge,               // -18
    &LibReserved,              // -24
    //
    //    Context functions
    //
    &W3D_CreateContext,        // -30
    &W3D_DestroyContext,       // -36
    &W3D_GetState,             // -42
    &W3D_SetState,             // -48
    //
    //    Driver functions
    //
    &W3D_CheckDriver,          // -54
    &W3D_LockHardware,         // -60
    &W3D_UnLockHardware,       // -66
    &W3D_WaitIdle,             // -72
    &W3D_CheckIdle,            // -78
    &W3D_Query,                // -84
    &W3D_GetTexFmtInfo,        // -90
    //
    //    Texture functions
    //
    &W3D_AllocTexObj,          // -96
    &W3D_FreeTexObj,           // -102
    &W3D_ReleaseTexture,       // -108
    &W3D_FlushTextures,        // -114
    &W3D_SetFilter,            // -120
    &W3D_SetTexEnv,            // -126
    &W3D_SetWrapMode,          // -132
    &W3D_UpdateTexImage,       // -138
    &W3D_UploadTexture,        // -144
    //
    //    Drawing functions
    //
    &W3D_DrawLine,             // -150
    &W3D_DrawPoint,            // -156
    &W3D_DrawTriangle,         // -162
    &W3D_DrawTriFan,           // -168
    &W3D_DrawTriStrip,         // -174
    //
    //    Effect functions
    //
    &W3D_SetAlphaMode,         // -180
    &W3D_SetBlendMode,         // -186
    &W3D_SetDrawRegion,        // -192
    &W3D_SetFogParams,         // -198
    &W3D_SetColorMask,         // -204
    &W3D_SetStencilFunc,       // -210
    //
    //    ZBuffer functions
    //
    &W3D_AllocZBuffer,         // -216
    &W3D_FreeZBuffer,          // -222
    &W3D_ClearZBuffer,         // -228
    &W3D_ReadZPixel,           // -234
    &W3D_ReadZSpan,            // -240
    &W3D_SetZCompareMode,      // -246
    //
    //    Stencil buffer functions
    //
    &W3D_AllocStencilBuffer,   // -252
    &W3D_ClearStencilBuffer,   // -258
    &W3D_FillStencilBuffer,    // -264
    &W3D_FreeStencilBuffer,    // -270
    &W3D_ReadStencilPixel,     // -276
    &W3D_ReadStencilSpan,      // -282
    //
    //    New functions
    //
    &W3D_SetLogicOp,           // -288
    &W3D_Hint,                 // -294
    &W3D_SetDrawRegionWBM,     // -300
    &W3D_GetDriverState,       // -306
    &W3D_Flush,                // -312
    &W3D_SetPenMask,           // -318
    &W3D_SetStencilOp,         // -324
    &W3D_SetWriteMask,         // -330
    &W3D_WriteStencilPixel,    // -336
    &W3D_WriteStencilSpan,     // -342
    &W3D_WriteZPixel,          // -348
    &W3D_WriteZSpan,           // -354
    &W3D_SetCurrentColor,      // -360
    &W3D_SetCurrentPen,        // -366
    &W3D_UpdateTexSubImage,    // -372
    &W3D_FreeAllTexObj,        // -378
    &W3D_GetDestFmt,           // -384
    //
    //  V2
    //
    &W3D_DrawLineStrip,        // -390
    &W3D_DrawLineLoop,         // -396
    &W3D_GetDrivers,           // -402
    &W3D_QueryDriver,          // -408
    &W3D_GetDriverTexFmtInfo,  // -414
    &W3D_RequestMode,          // -420
    &W3D_SetScissor,           // -426
    &W3D_FlushFrame,           // -432
    &W3D_TestMode,             // -438
    &W3D_SetChromaTestBounds,  // -444
    &W3D_ClearDrawRegion,      // -450
    //
    //  V3
    //
    &W3D_DrawTriangleV,        // -456
    &W3D_DrawTriFanV,          // -462
    &W3D_DrawTriStripV,        // -468
    &W3D_GetScreenmodeList,    // -474
    &W3D_FreeScreenmodeList,   // -480
    &W3D_BestModeID,           // -486

    (APTR) -1
};

_Static_assert(6 * (sizeof(FuncTab)/sizeof(*FuncTab)-1) == 486, "Invalid table size");

static const UWORD InitTab[] = {
    INITBYTE(OFFSET(VC4D,lib.lib_Node.ln_Type), NT_LIBRARY),
    INITBYTE(OFFSET(VC4D,lib.lib_Flags), LIBF_CHANGED | LIBF_SUMUSED),
    INITWORD(OFFSET(VC4D,lib.lib_Version), VC4D_VERSION),
    INITWORD(OFFSET(VC4D,lib.lib_Revision), VC4D_REVISION),
    0
};

static const ULONG LibInitTab[4] = {
    sizeof(VC4D), // Size of library
    (ULONG)FuncTab, // Function initializers
    (ULONG)InitTab, // Data table in InitStruct format
    (ULONG)LibInit,
};

const struct Resident RomTag = {
    RTC_MATCHWORD,
    (struct Resident*) &RomTag,
    (struct Resident*) &RomTag + 1,
    RTF_AUTOINIT,
    VC4D_VERSION,
    NT_LIBRARY,
    0,
    (char*) LibName,
    (char*) IdString,
    (APTR) LibInitTab
};


