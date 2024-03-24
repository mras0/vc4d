#include "vc4d.h"
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/asl.h>
#include <proto/cybergraphics.h>
#include <proto/utility.h>

#define TODO(...) LOG_DEBUG("TODO: Implement %s\n", __VA_ARGS__)

W3D_Context *
W3D_CreateContext(     ULONG * error __asm("a0"),
     struct TagItem * CCTags __asm("a1"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    *error = W3D_UNSUPPORTED;
    return NULL;
}

void
W3D_DestroyContext(     W3D_Context * context __asm("a0"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
}

ULONG
W3D_GetState(     W3D_Context * context __asm("a0"),
     ULONG state __asm("d1"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

ULONG
W3D_SetState(     W3D_Context * context __asm("a0"),
     ULONG state __asm("d0"),
     ULONG action __asm("d1"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

ULONG
W3D_CheckDriver( VC4D* vc4d __asm("a6"))
{
    (void)vc4d;
    return W3D_DRIVER_3DHW;
}

ULONG
W3D_LockHardware(     W3D_Context * context __asm("a0"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

void
W3D_UnLockHardware(     W3D_Context * context __asm("a0"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
}

void
W3D_WaitIdle(     W3D_Context * context __asm("a0"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
}

ULONG
W3D_CheckIdle(     W3D_Context * context __asm("a0"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

ULONG
W3D_Query(     W3D_Context * context __asm("a0"),
     ULONG query __asm("d0"),
     ULONG destfmt __asm("d1"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

ULONG
W3D_GetTexFmtInfo(     W3D_Context * context __asm("a0"),
     ULONG format __asm("d0"),
     ULONG destfmt __asm("d1"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

W3D_Texture *
W3D_AllocTexObj(     W3D_Context * context __asm("a0"),
     ULONG * error __asm("a1"),
     struct TagItem * ATOTags __asm("a2"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    *error = W3D_UNSUPPORTED;
    return NULL;
}

void
W3D_FreeTexObj(     W3D_Context * context __asm("a0"),
     W3D_Texture * texture __asm("a1"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
}

void
W3D_ReleaseTexture(     W3D_Context * context __asm("a0"),
     W3D_Texture * texture __asm("a1"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
}

void
W3D_FlushTextures(     W3D_Context * context __asm("a0"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
}

ULONG
W3D_SetFilter(     W3D_Context * context __asm("a0"),
     W3D_Texture * texture __asm("a1"),
     ULONG min __asm("d0"),
     ULONG mag __asm("d1"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

ULONG
W3D_SetTexEnv(     W3D_Context * context __asm("a0"),
     W3D_Texture * texture __asm("a1"),
     ULONG envparam __asm("d1"),
     W3D_Color * envcolor __asm("a2"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

ULONG
W3D_SetWrapMode(     W3D_Context * context __asm("a0"),
     W3D_Texture * texture __asm("a1"),
     ULONG mode_s __asm("d0"),
     ULONG mode_t __asm("d1"),
     W3D_Color * bordercolor __asm("a2"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

ULONG
W3D_UpdateTexImage(     W3D_Context * context __asm("a0"),
     W3D_Texture * texture __asm("a1"),
     void * teximage __asm("a2"),
     int level __asm("d1"),
     ULONG * palette __asm("a3"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

ULONG
W3D_UploadTexture(     W3D_Context * context __asm("a0"),
     W3D_Texture * texture __asm("a1"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

ULONG
W3D_DrawLine(     W3D_Context * context __asm("a0"),
     W3D_Line * line __asm("a1"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

ULONG
W3D_DrawPoint(     W3D_Context * context __asm("a0"),
     W3D_Point * point __asm("a1"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

ULONG
W3D_DrawTriangle(     W3D_Context * context __asm("a0"),
     W3D_Triangle * triangle __asm("a1"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

ULONG
W3D_DrawTriFan(     W3D_Context * context __asm("a0"),
     W3D_Triangles * triangles __asm("a1"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

ULONG
W3D_DrawTriStrip(     W3D_Context * context __asm("a0"),
     W3D_Triangles * triangles __asm("a1"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

ULONG
W3D_SetAlphaMode(     W3D_Context * context __asm("a0"),
     ULONG mode __asm("d1"),
     W3D_Float * refval __asm("a1"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

ULONG
W3D_SetBlendMode(     W3D_Context * context __asm("a0"),
     ULONG srcfunc __asm("d0"),
     ULONG dstfunc __asm("d1"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

ULONG
W3D_SetDrawRegion(     W3D_Context * context __asm("a0"),
     struct BitMap * bm __asm("a1"),
     int yoffset __asm("d1"),
     W3D_Scissor * scissor __asm("a2"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

ULONG
W3D_SetFogParams(     W3D_Context * context __asm("a0"),
     W3D_Fog * fogparams __asm("a1"),
     ULONG fogmode __asm("d1"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

ULONG
W3D_SetColorMask(     W3D_Context * context __asm("a0"),
     W3D_Bool red __asm("d0"),
     W3D_Bool green __asm("d1"),
     W3D_Bool blue __asm("d2"),
     W3D_Bool alpha __asm("d3"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

ULONG
W3D_SetStencilFunc(     W3D_Context * context __asm("a0"),
     ULONG func __asm("d0"),
     ULONG refvalue __asm("d1"),
     ULONG mask __asm("d2"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

ULONG
W3D_AllocZBuffer(     W3D_Context * context __asm("a0"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

ULONG
W3D_FreeZBuffer(     W3D_Context * context __asm("a0"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

ULONG
W3D_ClearZBuffer(     W3D_Context * context __asm("a0"),
     W3D_Double * clearvalue __asm("a1"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

ULONG
W3D_ReadZPixel(     W3D_Context * context __asm("a0"),
     ULONG x __asm("d0"),
     ULONG y __asm("d1"),
     W3D_Double * z __asm("a1"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

ULONG
W3D_ReadZSpan(     W3D_Context * context __asm("a0"),
     ULONG x __asm("d0"),
     ULONG y __asm("d1"),
     ULONG n __asm("d2"),
     W3D_Double * z __asm("a1"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

ULONG
W3D_SetZCompareMode(     W3D_Context * context __asm("a0"),
     ULONG mode __asm("d1"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

ULONG
W3D_AllocStencilBuffer(     W3D_Context * context __asm("a0"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

ULONG
W3D_ClearStencilBuffer(     W3D_Context * context __asm("a0"),
     ULONG * clearval __asm("a1"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

ULONG
W3D_FillStencilBuffer(     W3D_Context * context __asm("a0"),
     ULONG x __asm("d0"),
     ULONG y __asm("d1"),
     ULONG width __asm("d2"),
     ULONG height __asm("d3"),
     ULONG depth __asm("d4"),
     void * data __asm("a1"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

ULONG
W3D_FreeStencilBuffer(     W3D_Context * context __asm("a0"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

ULONG
W3D_ReadStencilPixel(     W3D_Context * context __asm("a0"),
     ULONG x __asm("d0"),
     ULONG y __asm("d1"),
     ULONG * st __asm("a1"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

ULONG
W3D_ReadStencilSpan(     W3D_Context * context __asm("a0"),
     ULONG x __asm("d0"),
     ULONG y __asm("d1"),
     ULONG n __asm("d2"),
     ULONG * st __asm("a1"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

ULONG
W3D_SetLogicOp(     W3D_Context * context __asm("a0"),
     ULONG operation __asm("d1"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

ULONG
W3D_Hint(     W3D_Context * context __asm("a0"),
     ULONG mode __asm("d0"),
     ULONG quality __asm("d1"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

ULONG
W3D_SetDrawRegionWBM(     W3D_Context * context __asm("a0"),
     W3D_Bitmap * bitmap __asm("a1"),
     W3D_Scissor * scissor __asm("a2"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

ULONG
W3D_GetDriverState(     W3D_Context * context __asm("a0"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

ULONG
W3D_Flush(     W3D_Context * context __asm("a0"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

ULONG
W3D_SetPenMask(     W3D_Context * context __asm("a0"),
     ULONG pen __asm("d1"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

ULONG
W3D_SetStencilOp(     W3D_Context * context __asm("a0"),
     ULONG sfail __asm("d0"),
     ULONG dpfail __asm("d1"),
     ULONG dppass __asm("d2"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

ULONG
W3D_SetWriteMask(     W3D_Context * context __asm("a0"),
     ULONG mask __asm("d1"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

ULONG
W3D_WriteStencilPixel(     W3D_Context * context __asm("a0"),
     ULONG x __asm("d0"),
     ULONG y __asm("d1"),
     ULONG st __asm("d2"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

ULONG
W3D_WriteStencilSpan(     W3D_Context * context __asm("a0"),
     ULONG x __asm("d0"),
     ULONG y __asm("d1"),
     ULONG n __asm("d2"),
     ULONG * st __asm("a1"),
     UBYTE * mask __asm("a2"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

void
W3D_WriteZPixel(     W3D_Context * context __asm("a0"),
     ULONG x __asm("d0"),
     ULONG y __asm("d1"),
     W3D_Double * z __asm("a1"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
}

void
W3D_WriteZSpan(     W3D_Context * context __asm("a0"),
     ULONG x __asm("d0"),
     ULONG y __asm("d1"),
     ULONG n __asm("d2"),
     W3D_Double * z __asm("a1"),
     UBYTE * maks __asm("a2"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
}

ULONG
W3D_SetCurrentColor(     W3D_Context * context __asm("a0"),
     W3D_Color * color __asm("a1"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

ULONG
W3D_SetCurrentPen(     W3D_Context * context __asm("a0"),
     ULONG pen __asm("d1"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

ULONG
W3D_UpdateTexSubImage(     W3D_Context * context __asm("a0"),
     W3D_Texture * texture __asm("a1"),
     void * teximage __asm("a2"),
     ULONG lev __asm("d1"),
     ULONG * palette __asm("a3"),
     W3D_Scissor* scissor __asm("a4"),
     ULONG srcbpr __asm("d0"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

ULONG
W3D_FreeAllTexObj(     W3D_Context * context __asm("a0"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

ULONG
W3D_GetDestFmt( VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

ULONG
W3D_DrawLineStrip(     W3D_Context * context __asm("a0"),
     W3D_Lines * lines __asm("a1"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

ULONG
W3D_DrawLineLoop(     W3D_Context * context __asm("a0"),
     W3D_Lines * lines __asm("a1"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

static const W3D_Driver driver = {
    .ChipID = W3D_CHIP_UNKNOWN,
    .formats = W3D_FMT_R8G8B8A8, // TODO: Support more formats...
    .name = "VideoCore IV",
    .swdriver = W3D_FALSE,
};

static const W3D_Driver* drivers[] = {
    &driver,
    NULL,
};

W3D_Driver * *
W3D_GetDrivers( VC4D* vc4d __asm("a6"))
{
    (void)vc4d;
    return (W3D_Driver**)drivers;
}

ULONG
W3D_QueryDriver(     W3D_Driver* driver __asm("a0"),
     ULONG query __asm("d0"),
     ULONG destfmt __asm("d1"),
 VC4D* vc4d __asm("a6"))
{
    LOG_DEBUG("W3D_QueryDriver: query=0x%lx destfmt=0x%lx\n", query, destfmt);

    if (!(driver->formats & destfmt))
        return W3D_NOT_SUPPORTED;

    // Let's pretend everything is supported for now
    return W3D_FULLY_SUPPORTED;
}

ULONG
W3D_GetDriverTexFmtInfo(     W3D_Driver* driver __asm("a0"),
     ULONG format __asm("d0"),
     ULONG destfmt __asm("d1"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

struct ScreenModeHookData {
    VC4D* vc4d;
    struct TagItem* tags;
    ULONG minw;
    ULONG maxw;
    ULONG minh;
    ULONG maxh;
};

static BOOL ScreenModeHook(struct Hook* hook __asm("a0"), ULONG modeId __asm("a1"), struct ScreenModeRequester* req __asm("a2"))
{
    struct ScreenModeHookData* ctx = hook->h_Data;
    VC4D* vc4d = ctx->vc4d;
    GFXBASE;
    CGFXBASE;

    // TODO: Filter on actually being on VC4
    if (!IsCyberModeID(modeId))
        return FALSE;

    struct DimensionInfo di;
    if (!GetDisplayInfoData(NULL, (UBYTE*)&di, sizeof(di), DTAG_DIMS, modeId))
        return FALSE;

    // TODO: Support more screen modes
    if (di.MaxDepth != 24)
        return FALSE;

    const ULONG w = 1 + di.Nominal.MaxX - di.Nominal.MinX;
    const ULONG h = 1 + di.Nominal.MaxY - di.Nominal.MinY;

    if (w < ctx->minw || w > ctx->maxw || h < ctx->minh || h > ctx->maxh)
        return FALSE;

    //if (w < ctx->width || w > 2 * ctx->width || h < ctx->height || h > 2 * ctx->width)
    //    return FALSE;

    LOG_DEBUG("ModeId %08lX %lux%lux%lu\n", modeId, w, h, (int)di.MaxDepth);
    return TRUE;
}


ULONG
W3D_RequestMode(     struct TagItem * taglist __asm("a0"),
 VC4D* vc4d __asm("a6"))
{
    // Hard code this for a bit..
    CGFXBASE;
    if (IsCyberModeID(0x50031303)) {
        LOG_DEBUG("Warning: Mode hardcode in W3D_RequestMode\n");
        return 0x50031303;
    }

    SYSBASE;
    struct Library* AslBase = OpenLibrary("asl.library", 39);
    if (!AslBase) {
        LOG_DEBUG("Could not open asl.library\n");
        return INVALID_ID;
    }

    struct TagItem* ti;
    struct Hook hook = {0};
    struct TagItem tags[32];
    const int maxtags = sizeof(tags)/sizeof(tags) - 2; // Room for hook + done
    int numtags = 0;
    LOG_DEBUG("W3D_RequestMode\n");
    for (ti = taglist; ti->ti_Tag != TAG_DONE; ++ti)  {
        if (ti->ti_Tag >= ASL_TB && ti->ti_Tag <= ASL_LAST_TAG) {
            if (numtags < maxtags)
                tags[numtags++] = *ti;
            continue;
        }
        //W3D_SMR_DRIVER      /* Driver to filter */
        //W3D_SMR_DESTFMT     /* Dest Format to filter */
        //W3D_SMR_TYPE        /* Type to filter */
        //W3D_SMR_SIZEFILTER  /* Also filter size */
        //W3D_SMR_MODEMASK    /* AND-Mask for modes */ 
        LOG_DEBUG(" Unsupported tag: 0x%08lx 0x%08lx\n", ti->ti_Tag, ti->ti_Data);
    }
    tags[numtags].ti_Tag = ASLSM_FilterFunc;
    tags[numtags++].ti_Data = (ULONG)&hook;
    tags[numtags].ti_Tag = TAG_DONE;

    struct ScreenModeHookData hookData;
    hookData.vc4d = vc4d;
    hookData.tags = taglist;
    hookData.minw = 0;
    hookData.maxw = ~0UL;
    hookData.minh = 0;
    hookData.maxh = ~0UL;

    UTILBASE;
    if ((ti = FindTagItem(ASLSM_MinWidth, taglist)) != NULL) hookData.minw = ti->ti_Data;
    if ((ti = FindTagItem(ASLSM_MaxWidth, taglist)) != NULL) hookData.maxw = ti->ti_Data;
    if ((ti = FindTagItem(ASLSM_MinHeight, taglist)) != NULL) hookData.minh = ti->ti_Data;
    if ((ti = FindTagItem(ASLSM_MaxHeight, taglist)) != NULL) hookData.maxh = ti->ti_Data;

    hook.h_Entry = (APTR)ScreenModeHook;
    hook.h_Data = &hookData;

    ULONG mode = INVALID_ID;
    struct ScreenModeRequester *req;
    if ((req = AllocAslRequest(ASL_ScreenModeRequest, tags))) {

        if (AslRequest(req, taglist))
            mode = req->sm_DisplayID;
        FreeAslRequest(req);
    } else {
        LOG_DEBUG("AllocAslRequest failed\n");
    }
    CloseLibrary(AslBase);

    return mode;
}

void
W3D_SetScissor(     W3D_Context * context __asm("a0"),
     W3D_Scissor * scissor __asm("a1"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
}

void
W3D_FlushFrame(     W3D_Context * context __asm("a0"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
}

W3D_Driver *
W3D_TestMode(     ULONG ModeID __asm("d0"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return NULL;
}

ULONG
W3D_SetChromaTestBounds(     W3D_Context * context __asm("a0"),
     W3D_Texture * texture __asm("a1"),
     ULONG rgba_lower __asm("d0"),
     ULONG rgba_upper __asm("d1"),
     ULONG mode __asm("d2"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

ULONG
W3D_ClearDrawRegion(     W3D_Context * context __asm("a0"),
     ULONG color __asm("d0"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

ULONG
W3D_DrawTriangleV(     W3D_Context * context __asm("a0"),
     W3D_TriangleV * triangle __asm("a1"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

ULONG
W3D_DrawTriFanV(     W3D_Context * context __asm("a0"),
     W3D_TrianglesV * triangles __asm("a1"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

ULONG
W3D_DrawTriStripV(     W3D_Context * context __asm("a0"),
     W3D_TrianglesV * triangles __asm("a1"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

W3D_ScreenMode *
W3D_GetScreenmodeList( VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return NULL;
}

void
W3D_FreeScreenmodeList(     W3D_ScreenMode * list __asm("a0"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
}

ULONG
W3D_BestModeID(     struct TagItem * tags __asm("a0"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

ULONG
W3D_VertexPointer(     W3D_Context* context __asm("a0"),
     void * pointer __asm("a1"),
     int stride __asm("d0"),
     ULONG mode __asm("d1"),
     ULONG flags __asm("d2"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

ULONG
W3D_TexCoordPointer(     W3D_Context* context __asm("a0"),
     void * pointer __asm("a1"),
     int stride __asm("d0"),
     int unit __asm("d1"),
     int off_v __asm("d2"),
     int off_w __asm("d3"),
     ULONG flags __asm("d4"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

ULONG
W3D_ColorPointer(     W3D_Context* context __asm("a0"),
     void * pointer __asm("a1"),
     int stride __asm("d0"),
     ULONG format __asm("d1"),
     ULONG mode __asm("d2"),
     ULONG flags __asm("d3"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

ULONG
W3D_BindTexture(     W3D_Context* context __asm("a0"),
     ULONG tmu __asm("d0"),
     W3D_Texture * texture __asm("a1"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

ULONG
W3D_DrawArray(     W3D_Context* context __asm("a0"),
     ULONG primitive __asm("d0"),
     ULONG base __asm("d1"),
     ULONG count __asm("d2"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

ULONG
W3D_DrawElements(     W3D_Context* context __asm("a0"),
     ULONG primitive __asm("d0"),
     ULONG type __asm("d1"),
     ULONG count __asm("d2"),
     void * indices __asm("a1"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_UNSUPPORTED;
}

void
W3D_SetFrontFace(     W3D_Context* context __asm("a0"),
     ULONG direction __asm("d0"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
}

