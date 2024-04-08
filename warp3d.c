#include "vc4d.h"
#include <stddef.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/asl.h>
#include <proto/cybergraphics.h>
#include <proto/utility.h>
#include <cybergraphics/cybergraphics.h>

#include "draw.h"

#ifdef PISTORM32
#define TEX_ENDIAN(x) LE32(x)
#else
#define TEX_ENDIAN(x) (x)
#endif

#define TODO(...) LOG_DEBUG("TODO: Implement %s\n", __VA_ARGS__)
#define XTODO(...) LOG_DEBUG("XTODO: Implement %s\n", __VA_ARGS__)
#define LOG_VAL(val) LOG_DEBUG("  %-20s = 0x%08lx\n", #val, val)

static BOOL CheckBitMap(VC4D* vc4d, struct BitMap* bm)
{
    CGFXBASE;
    APTR lock = LockBitMapTags(bm, TAG_DONE);
    if (lock) {
        UnLockBitMap(lock);
        return TRUE;
    }
    LOG_ERROR("Standard bitmap used!\n");
    return FALSE;
}

static inline BOOL SetScissor(VC4D* vc4d, VC4D_Context* vctx, const W3D_Scissor* scissor, const char* func)
{
    // Maybe just clip the region instead?
    // Also need to consider yoffset..
    if ((ULONG)scissor->left < vctx->width && (ULONG)(scissor->left + scissor->width) <= vctx->width &&
            (ULONG)scissor->top < vctx->height && (ULONG)(scissor->top + scissor->height) <= vctx->height) {
        vctx->w3d.scissor = *scissor;
        return TRUE;
    } else {
        LOG_ERROR("%s: Invalid scissor region %lu,%lu,%lu,%lu\n", func, scissor->left, scissor->top, scissor->width, scissor->height);
        return FALSE;
    }
}

static void NewList(struct List* l)
{
    l->lh_Head = (struct Node*)&l->lh_Tail;
    l->lh_Tail = NULL;
    l->lh_TailPred = (struct Node*)&l->lh_Head;
}

W3D_Context *
W3D_CreateContext(ULONG * error __asm("a0"), struct TagItem * CCTags __asm("a1"), VC4D* vc4d __asm("a6"))
{
    UTILBASE;

    struct BitMap*  bm = (struct BitMap*)(struct BitMap*)(struct BitMap*)(struct BitMap*)GetTagData(W3D_CC_BITMAP, 0, CCTags);                    /* destination bitmap */
    ULONG yofs      = GetTagData(W3D_CC_YOFFSET, 0, CCTags);                   /* y-Offset */
    ULONG driver    = GetTagData(W3D_CC_DRIVERTYPE, W3D_DRIVER_BEST, CCTags);  /* see below */
    ULONG w3dbm     = GetTagData(W3D_CC_W3DBM, W3D_FALSE, CCTags);             /* Use W3D_Bitmap instead of struct BitMap */
    ULONG indirect  = GetTagData(W3D_CC_INDIRECT, W3D_FALSE, CCTags);          /* Indirect drawing */
    ULONG globaltex = GetTagData(W3D_CC_GLOBALTEXENV, W3D_FALSE, CCTags);      /* SetTexEnv is global */
    ULONG dheight   = GetTagData(W3D_CC_DOUBLEHEIGHT, W3D_FALSE, CCTags);      /* Drawing area has double height */
    ULONG fast      = GetTagData(W3D_CC_FAST, W3D_FALSE, CCTags);              /* Allow Warp3D to modify passed Triangle/Lines/Points */
    ULONG modeid    = GetTagData(W3D_CC_MODEID, INVALID_ID, CCTags);           /* Specify modeID to use */

    LOG_VAL(bm        );
    LOG_VAL(yofs      );
    LOG_VAL(driver    );
    LOG_VAL(w3dbm     );
    LOG_VAL(indirect  );
    LOG_VAL(globaltex );
    LOG_VAL(dheight   );
    LOG_VAL(fast      );
    LOG_VAL(modeid    );

    if (!bm || driver == W3D_DRIVER_CPU || w3dbm || !CheckBitMap(vc4d, bm)) {
        *error = W3D_UNSUPPORTED;
        LOG_ERROR("W3D_CreateContext: Unsupported arguments.\n");
        return NULL;
    }

    // TODO: More checking

    SYSBASE;
    W3D_Context* context = AllocVec(sizeof(VC4D_Context), MEMF_CLEAR|MEMF_PUBLIC);
    if (!context) {
        *error = W3D_NOMEMORY;
        LOG_ERROR("W3D_CreateContext: Out of memory\n");
        return NULL;
    }

    context->drawregion = bm;
    context->yoffset = yofs;
    if (fast)
        context->state |= W3D_FAST;
    if (globaltex)
        context->state |= W3D_GLOBALTEXENV;
    if (indirect)
        context->state |= W3D_INDIRECT;
    if (dheight)
        context->state |= W3D_DOUBLEHEIGHT;

    context->scissor.left = 0;
    context->scissor.top = 0;
    context->scissor.width = bm->BytesPerRow / 4; // XXX: Assumes 32-bit mode
    context->scissor.height = bm->Rows;
    if (dheight)
        context->scissor.height >>= 1;

    ((VC4D_Context*)context)->width = context->scissor.width;
    ((VC4D_Context*)context)->height = context->scissor.height;

    NewList((struct List*)&context->tex);
    NewList((struct List*)&context->restex);

    ((VC4D_Context*)context)->fixedcolor.r = 
    ((VC4D_Context*)context)->fixedcolor.g = 
    ((VC4D_Context*)context)->fixedcolor.b = 
    ((VC4D_Context*)context)->fixedcolor.a = 1.0f;

    *error = W3D_SUCCESS;
    return context;
}

ULONG W3D_FreeAllTexObj(W3D_Context * context __asm("a0"), VC4D* vc4d __asm("a6"));
ULONG W3D_FreeZBuffer(W3D_Context * context __asm("a0"), VC4D* vc4d __asm("a6"));

void
W3D_DestroyContext(W3D_Context * context __asm("a0"), VC4D* vc4d __asm("a6"))
{
    SYSBASE;

    if (context->HWlocked)
        LOG_ERROR("W3D_DestroyContext: destryoing context with locked HW!\n");

    W3D_FreeZBuffer(context, vc4d);
    W3D_FreeAllTexObj(context, vc4d);

    FreeVec(context);
}

// N.B. bitmask starting from 0
static const char* const StateNames[26] = {
"W3D_AUTOTEXMANAGEMENT",
"W3D_SYNCHRON",
"W3D_INDIRECT",
"W3D_GLOBALTEXENV",
"W3D_DOUBLEHEIGHT",
"W3D_FAST",
"W3D_AUTOCLIP",
"W3D_TEXMAPPING",
"W3D_PERSPECTIVE",
"W3D_GOURAUD",
"W3D_ZBUFFER",
"W3D_ZBUFFERUPDATE",
"W3D_BLENDING",
"W3D_FOGGING",
"W3D_ANTI_POINT",
"W3D_ANTI_LINE",
"W3D_ANTI_POLYGON",
"W3D_ANTI_FULLSCREEN",
"W3D_DITHERING",
"W3D_LOGICOP",
"W3D_STENCILBUFFER",
"W3D_ALPHATEST",
"W3D_SPECULAR",
"W3D_TEXMAPPING3D",
"W3D_SCISSOR",
"W3D_CHROMATEST",
};

ULONG
W3D_GetState(W3D_Context * context __asm("a0"), ULONG state __asm("d1"), VC4D* vc4d __asm("a6"))
{
    return (context->state & state) == state ? W3D_ENABLED : W3D_DISABLED;
}

ULONG
W3D_SetState(W3D_Context * context __asm("a0"), ULONG state __asm("d0"), ULONG action __asm("d1"), VC4D* vc4d __asm("a6"))
{
    if (state < W3D_AUTOTEXMANAGEMENT || state > W3D_CHROMATEST || (state & (state -1)) || (action != W3D_ENABLE && action != W3D_DISABLE)) {
        LOG_ERROR("W3D_SetState: Invalid input state=0x%lx action=0x%lx\n", state, action);
        return W3D_UNSUPPORTED;
    }

    // Supported here just means we won't complain about it changes to it..
    const ULONG supported =
//        W3D_AUTOTEXMANAGEMENT |
//        W3D_SYNCHRON |
//        W3D_INDIRECT |
        W3D_GLOBALTEXENV |
        /*W3D_DOUBLEHEIGHT |  -- What does changing this imply? */
        W3D_FAST |
        /*W3D_AUTOCLIP |*/
        W3D_TEXMAPPING |
        W3D_PERSPECTIVE |
        W3D_GOURAUD |
        /*W3D_ZBUFFER |
        W3D_ZBUFFERUPDATE |*/
//        W3D_BLENDING |
        /*W3D_FOGGING |*/
        /*W3D_ANTI_POINT |*/
        /*W3D_ANTI_LINE |*/
        /*W3D_ANTI_POLYGON |*/
        /*W3D_ANTI_FULLSCREEN |*/
        W3D_DITHERING //|
        /*W3D_LOGICOP |*/
        /*W3D_STENCILBUFFER |*/
        /*W3D_ALPHATEST |*/
        /*W3D_SPECULAR |*/
        /*W3D_TEXMAPPING3D |*/
        /*W3D_SCISSOR |*/
        /*W3D_CHROMATEST*/;

    if (!(state & supported)) {
        for (ULONG i = 0; i < 26; ++i) {
            if (state == (1U << (i+1))) {
                LOG_DEBUG("TODO: W3D_SetState %s %s\n", action == W3D_ENABLE ? "Enable" : "Disable", StateNames[i]);
                break;
            }
        }
    }

    // Fake everything..
    if (action == W3D_ENABLE)
        context->state |= state;
    else
        context->state &= ~state;
    return W3D_SUCCESS;
}

ULONG
W3D_CheckDriver( VC4D* vc4d __asm("a6"))
{
    (void)vc4d;
    return W3D_DRIVER_3DHW;
}

ULONG
W3D_LockHardware(W3D_Context * context __asm("a0"), VC4D* vc4d __asm("a6"))
{
    if (context->HWlocked) {
        LOG_ERROR("W3D_LockHardware: Already locked!\n");
        return W3D_SUCCESS;
    }

    CGFXBASE;
    context->gfxdriver = LockBitMapTags(context->drawregion,
            LBMI_BYTESPERROW, (ULONG)&context->bprow,
            LBMI_BASEADDRESS, (ULONG)&context->vmembase,
            TAG_DONE);
    if (!context->gfxdriver) {
        LOG_ERROR("%s: Lock failed", __func__);
        return W3D_NOT_SUPPORTED;
    }

    context->HWlocked = W3D_TRUE;
    return W3D_SUCCESS;
}

void
W3D_UnLockHardware(W3D_Context * context __asm("a0"), VC4D* vc4d __asm("a6"))
{
    if (context->HWlocked != W3D_TRUE) {
        LOG_ERROR("W3D_UnLockHardware: Called without being locked!\n");
        return;
    }
    draw_flush(vc4d, (VC4D_Context*)context);
    CGFXBASE;
    UnLockBitMap(context->gfxdriver);
    context->HWlocked = W3D_FALSE;
}

void
W3D_WaitIdle(W3D_Context * context __asm("a0"), VC4D* vc4d __asm("a6"))
{
    draw_flush(vc4d, (VC4D_Context*)context);
}

ULONG
W3D_CheckIdle(     W3D_Context * context __asm("a0"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_SUCCESS;
}

ULONG
W3D_Query(W3D_Context * context __asm("a0"), ULONG query __asm("d0"), ULONG destfmt __asm("d1"), VC4D* vc4d __asm("a6"))
{
    // TODO: Pretend everything is supported for now
#define QUERY(x) case x: LOG_DEBUG("%s: TODO %s\n", __func__, #x); break
    switch (query) {
        case W3D_Q_DRAW_TRIANGLE:
        case W3D_Q_TEXMAPPING:
        case W3D_Q_PERSPECTIVE:
        case W3D_Q_GOURAUDSHADING:
            break;
        case W3D_Q_MAXTEXWIDTH:
        case W3D_Q_MAXTEXHEIGHT:
        case W3D_Q_MAXTEXWIDTH_P:
        case W3D_Q_MAXTEXHEIGHT_P:
            return 2048;
    QUERY(W3D_Q_LINEAR_REPEAT);
    QUERY(W3D_Q_LINEAR_CLAMP);
    QUERY(W3D_Q_PERSP_REPEAT);
    QUERY(W3D_Q_PERSP_CLAMP);
    QUERY(W3D_Q_DRAW_POINT);
    QUERY(W3D_Q_DRAW_LINE);
    QUERY(W3D_Q_DRAW_POINT_X);
    QUERY(W3D_Q_DRAW_LINE_X);
    QUERY(W3D_Q_DRAW_LINE_ST);
    QUERY(W3D_Q_DRAW_POLY_ST);
    QUERY(W3D_Q_DRAW_POINT_FX);
    QUERY(W3D_Q_DRAW_LINE_FX);
    QUERY(W3D_Q_MIPMAPPING);
    QUERY(W3D_Q_BILINEARFILTER);
    QUERY(W3D_Q_MMFILTER);
    QUERY(W3D_Q_ENV_REPLACE);
    QUERY(W3D_Q_ENV_DECAL);
    QUERY(W3D_Q_ENV_MODULATE);
    QUERY(W3D_Q_ENV_BLEND);
    QUERY(W3D_Q_WRAP_ASYM);
    QUERY(W3D_Q_SPECULAR);
    QUERY(W3D_Q_BLEND_DECAL_FOG);
    QUERY(W3D_Q_TEXMAPPING3D);
    QUERY(W3D_Q_CHROMATEST);
    QUERY(W3D_Q_FLATSHADING);
    QUERY(W3D_Q_ZBUFFER);
    QUERY(W3D_Q_ZBUFFERUPDATE);
    QUERY(W3D_Q_ZCOMPAREMODES);
    QUERY(W3D_Q_ALPHATEST);
    QUERY(W3D_Q_ALPHATESTMODES);
    QUERY(W3D_Q_BLENDING);
    QUERY(W3D_Q_SRCFACTORS);
    QUERY(W3D_Q_DESTFACTORS);
    QUERY(W3D_Q_ONE_ONE);
    QUERY(W3D_Q_FOGGING);
    QUERY(W3D_Q_LINEAR);
    QUERY(W3D_Q_EXPONENTIAL);
    QUERY(W3D_Q_S_EXPONENTIAL);
    QUERY(W3D_Q_INTERPOLATED);
    QUERY(W3D_Q_ANTIALIASING);
    QUERY(W3D_Q_ANTI_POINT);
    QUERY(W3D_Q_ANTI_LINE);
    QUERY(W3D_Q_ANTI_POLYGON);
    QUERY(W3D_Q_ANTI_FULLSCREEN);
    QUERY(W3D_Q_DITHERING);
    QUERY(W3D_Q_PALETTECONV);
    QUERY(W3D_Q_SCISSOR);
    QUERY(W3D_Q_RECTTEXTURES);
    QUERY(W3D_Q_LOGICOP);
    QUERY(W3D_Q_MASKING);
    QUERY(W3D_Q_STENCILBUFFER);
    QUERY(W3D_Q_STENCIL_MASK);
    QUERY(W3D_Q_STENCIL_FUNC);
    QUERY(W3D_Q_STENCIL_SFAIL);
    QUERY(W3D_Q_STENCIL_DPFAIL);
    QUERY(W3D_Q_STENCIL_DPPASS);
    QUERY(W3D_Q_STENCIL_WRMASK);
    QUERY(W3D_Q_DRAW_POINT_TEX);
    QUERY(W3D_Q_DRAW_LINE_TEX);
        default:
    LOG_ERROR("%s: Unknown query value %lu\n", __func__, query);
    return W3D_UNSUPPORTED;
    }

    return W3D_FULLY_SUPPORTED;
}

ULONG
W3D_GetTexFmtInfo(W3D_Context * context __asm("a0"), ULONG format __asm("d0"), ULONG destfmt __asm("d1"), VC4D* vc4d __asm("a6"))
{
    // N.B. context can be NULL
    // TODO: For now just pretend everything is supported and fast
    return W3D_TEXFMT_FAST | W3D_TEXFMT_SUPPORTED;
}

static const char* const FormatNames[12] = {
    "Invalid",
    "W3D_CHUNKY",
    "W3D_A1R5G5B5",
    "W3D_R5G6B5",
    "W3D_R8G8B8",
    "W3D_A4R4G4B4",
    "W3D_A8R8G8B8",
    "W3D_A8",
    "W3D_L8",
    "W3D_L8A8",
    "W3D_I8",
    "W3D_R8G8B8A8",
};

static inline ULONG Expand4_8(ULONG val)
{
    val &= 0xf;
    return val << 4 | val;
}

W3D_Texture *
W3D_AllocTexObj(W3D_Context * context __asm("a0"), ULONG * error __asm("a1"), struct TagItem * ATOTags __asm("a2"), VC4D* vc4d __asm("a6"))
{
    SYSBASE;
    UTILBASE;

    const ULONG image      = GetTagData(W3D_ATO_IMAGE      , 0, ATOTags);        /* texture image */
    const ULONG format     = GetTagData(W3D_ATO_FORMAT     , 0, ATOTags);        /* source format */
    const ULONG width      = GetTagData(W3D_ATO_WIDTH      , 0, ATOTags);        /* border width */
    const ULONG height     = GetTagData(W3D_ATO_HEIGHT     , 0, ATOTags);        /* border height */
    const ULONG mimap      = GetTagData(W3D_ATO_MIPMAP     , 0, ATOTags);        /* mipmap mask */
    const ULONG palette    = GetTagData(W3D_ATO_PALETTE    , 0, ATOTags);        /* texture palette */
    const ULONG usermip    = GetTagData(W3D_ATO_MIPMAPPTRS , 0, ATOTags);        /* array of user-supplied mipmaps */

    //LOG_DEBUG("W3D_AllocTexObj\n");
    //LOG_VAL(image  );
    ////LOG_VAL(format );
    //LOG_DEBUG("  %-20s = 0x%08lx (%s)\n", "format", format, format <= W3D_R8G8B8A8 ? FormatNames[format] : "<Invalid>");
    //LOG_VAL(width  );
    //LOG_VAL(height );
    //LOG_VAL(mimap  );
    //LOG_VAL(palette);
    //LOG_VAL(usermip);
    LOG_DEBUG("%s: %lux%lu %s\n", __func__, width, height, format <= W3D_R8G8B8A8 ? FormatNames[format] : "<Invalid>");
    if (mimap || usermip)
        LOG_DEBUG("%s: TODO support mipmaps\n");
    if (palette)
        LOG_DEBUG("%s: TODO support palette\n");

    if (!image || !format || format > W3D_R8G8B8A8 || !width || !height) {
        *error = W3D_ILLEGALINPUT;
        LOG_ERROR("W3D_AllocTexObj: Missing or illegal input values\n");
        return NULL;
    }

    if ((width & (width - 1)) || (height & (height-1))) {
        *error = W3D_UNSUPPORTEDTEXSIZE;
        LOG_ERROR("W3D_AllocTexObj: Non-power of 2 texture\n");
        return NULL;
    }

    W3D_Texture* texture = AllocVec(sizeof(VC4D_Texture), MEMF_PUBLIC | MEMF_CLEAR);
    if (!texture) {
        *error = W3D_NOMEMORY;
        LOG_ERROR("W3D_AllocTexObj: Out of memory\n");
        return NULL;
    }

#ifdef PISTORM32
    vc4_mem* m = &((VC4D_Texture*)texture)->texture_mem;
    int ret = vc4_mem_alloc(vc4d, m, sizeof(uint32_t) * width * height);
    if (ret) {
        *error = W3D_NOMEMORY;
        LOG_ERROR("W3D_AllocTexObj: Out of memory for texture data\n");
        FreeVec(texture);
        return NULL;
    }
    ULONG* d = m->hostptr;
#else
    texture->texdata = AllocVec(sizeof(uint32_t) * width * height, MEMF_PUBLIC);
    if (!texture->texdata) {
        *error = W3D_NOMEMORY;
        LOG_ERROR("W3D_AllocTexObj: Out of memory for texture data\n");
        FreeVec(texture);
        return NULL;
    }
    ULONG* d = texture->texdata;
#endif

    texture->texsource = (APTR)image;
    texture->texfmtsrc = format;
    texture->texwidth = width;
    texture->texheight = height;

    UBYTE* s = (APTR)image;
    ULONG n = width * height;

    if (format == W3D_A8R8G8B8) {
        while (n--) {
            *d++ = TEX_ENDIAN(s[0] << 24 | s[1] << 16 | s[2] << 8 | s[3]);
            s += 4;
        }
    } else if (format == W3D_R8G8B8A8) {
        while (n--) {
            *d++ = TEX_ENDIAN(s[0] << 16 | s[1] << 8 | s[2] << 0 | s[3] << 24);
            s += 4;
        }
    } else if (format == W3D_R8G8B8) {
        while (n--) {
            *d++ = TEX_ENDIAN(0xff << 24 | s[0] << 16 | s[1] << 8 | s[2]);
            s += 3;
        }
    } else if (format == W3D_A4R4G4B4) {
        while (n--) {
            UWORD val = *(UWORD*)s;
            *d++ = TEX_ENDIAN(Expand4_8(val >> 12) << 24 | Expand4_8(val >> 8) << 16 | Expand4_8(val >> 4) << 8 | Expand4_8(val));
            s += 2;
        }
    } else {
        LOG_ERROR("TODO: Support texture format: %s\n", FormatNames[texture->texfmtsrc]);
    }

    AddTail((struct List*)&context->restex, &texture->link);

    *error = W3D_SUCCESS;
    return texture;
}

void
W3D_FreeTexObj(W3D_Context * context __asm("a0"), W3D_Texture * texture __asm("a1"), VC4D* vc4d __asm("a6"))
{
    SYSBASE;
    if (!texture)
        return;
    Remove(&texture->link);
#ifdef PISTORM32
    vc4_mem_free(vc4d, &((VC4D_Texture*)texture)->texture_mem);
#else
    FreeVec(texture->texdata);
#endif
    FreeVec(texture);
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
    XTODO(__func__);
    return W3D_SUCCESS;
}

ULONG
W3D_SetTexEnv(W3D_Context * context __asm("a0"), W3D_Texture * texture __asm("a1"), ULONG envparam __asm("d1"), W3D_Color * envcolor __asm("a2"), VC4D* vc4d __asm("a6"))
{
    if (envparam < W3D_REPLACE || envparam > W3D_BLEND) {
        LOG_ERROR("W3D_SetTexEnv: Invalid envparm %ld\n", envparam);
        return W3D_ILLEGALINPUT;
    }
    if (context->state & W3D_GLOBALTEXENV) {
        for (texture = (W3D_Texture*)context->restex.mlh_Head ; texture->link.ln_Succ != NULL ; texture = (W3D_Texture*)texture->link.ln_Succ ) {
            ((VC4D_Texture*)texture)->texenv = envparam;
            if (envcolor)
                ((VC4D_Texture*)texture)->envcolor = *envcolor;
        }
    } else {
        ((VC4D_Texture*)texture)->texenv = envparam;
        if (envcolor)
            ((VC4D_Texture*)texture)->envcolor = *envcolor;
    }
    return W3D_SUCCESS;
}

ULONG
W3D_SetWrapMode(W3D_Context * context __asm("a0"), W3D_Texture * texture __asm("a1"), ULONG mode_s __asm("d0"), ULONG mode_t __asm("d1"), W3D_Color * bordercolor __asm("a2"), VC4D* vc4d __asm("a6"))
{
    (void)texture; (void)bordercolor;

    if (mode_s != W3D_REPEAT || mode_t != W3D_REPEAT) {
        LOG_DEBUG("W3D_SetWrapMode: Unsupported mode: s=%ld t=%ld\n", mode_s, mode_t);
        return W3D_UNSUPPORTED;
    }

    return W3D_SUCCESS;
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
    return W3D_SUCCESS;
}

ULONG
W3D_UploadTexture(     W3D_Context * context __asm("a0"),
     W3D_Texture * texture __asm("a1"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_SUCCESS;
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

static void init_vertex(vertex* v, const W3D_Vertex* w, BOOL perspective)
{
    v->x = w->x;
    v->y = w->y;

    if (perspective) {
        v->w = w->w; // 1->0f / w->z;
        v->u = w->u * v->w;
        v->v = w->v * v->w;
        v->r = w->color.r * v->w;
        v->g = w->color.g * v->w;
        v->b = w->color.b * v->w;
        v->a = w->color.a * v->w;
    } else {
        v->w = 1.0f;
        v->u = w->u;
        v->v = w->v;
        v->r = w->color.r;
        v->g = w->color.g;
        v->b = w->color.b;
        v->a = w->color.a;
    }
}

ULONG
W3D_DrawTriangle(W3D_Context * context __asm("a0"), W3D_Triangle * triangle __asm("a1"), VC4D* vc4d __asm("a6"))
{
    if (!context->HWlocked) {
        LOG_ERROR("%s: Hardware not locked!\n", __func__);
        return W3D_NOT_SUPPORTED;
    }

    vertex a, b, c;

    const BOOL perspective = (context->state & W3D_PERSPECTIVE) != 0;

    init_vertex(&a, &triangle->v1, perspective);
    init_vertex(&b, &triangle->v2, perspective);
    init_vertex(&c, &triangle->v3, perspective);
    draw_setup(vc4d, (VC4D_Context*)context, (VC4D_Texture*)triangle->tex);

    draw_triangle(vc4d, (VC4D_Context*)context, &a, &b, &c);
    return W3D_SUCCESS;
}

#define VSWAP(a,b) do { vertex* t = a; a = b; b = t; } while (0)

ULONG
W3D_DrawTriFan(W3D_Context * context __asm("a0"), W3D_Triangles * triangles __asm("a1"), VC4D* vc4d __asm("a6"))
{
    if (triangles->vertexcount == 0)
        return W3D_SUCCESS;

    if (triangles->vertexcount < 3) {
        LOG_ERROR("W3D_DrawTriFan: Invalid number of vertices %ld\n", triangles->vertexcount);
        return W3D_ILLEGALINPUT;
    }

    if (!context->HWlocked) {
        LOG_ERROR("%s: Hardware not locked!\n", __func__);
        return W3D_UNSUPPORTED;
    }

    draw_setup(vc4d, (VC4D_Context*)context, (VC4D_Texture*)triangles->tex);

    const BOOL perspective = (context->state & W3D_PERSPECTIVE) != 0;
    vertex a, b, c;
    init_vertex(&a, &triangles->v[0], perspective);
    init_vertex(&b, &triangles->v[1], perspective);
    init_vertex(&c, &triangles->v[2], perspective);

    draw_triangle(vc4d, (VC4D_Context*)context, &a, &b, &c);
    vertex* v2 = &b;
    vertex* v3 = &c;
    for (int i = 3; i < triangles->vertexcount; ++i) {
        VSWAP(v2, v3);

        init_vertex(v3, &triangles->v[i], perspective);

        draw_triangle(vc4d, (VC4D_Context*)context, &a, v2, v3);

    }

    return W3D_SUCCESS;
}

ULONG
W3D_DrawTriStrip(W3D_Context * context __asm("a0"), W3D_Triangles * triangles __asm("a1"), VC4D* vc4d __asm("a6"))
{
    if (triangles->vertexcount == 0)
        return W3D_SUCCESS;

    if (triangles->vertexcount < 3) {
        LOG_ERROR("%s: Invalid number of vertices %ld\n", __func__, triangles->vertexcount);
        return W3D_ILLEGALINPUT;
    }

    if (!context->HWlocked) {
        LOG_ERROR("%s: Hardware not locked!\n", __func__);
        return W3D_UNSUPPORTED;
    }

    draw_setup(vc4d, (VC4D_Context*)context, (VC4D_Texture*)triangles->tex);

    const BOOL perspective = (context->state & W3D_PERSPECTIVE) != 0;
    vertex a, b, c;
    init_vertex(&a, &triangles->v[0], perspective);
    init_vertex(&b, &triangles->v[1], perspective);
    init_vertex(&c, &triangles->v[2], perspective);

    draw_triangle(vc4d, (VC4D_Context*)context, &a, &b, &c);
    vertex* v1 = &a;
    vertex* v2 = &b;
    vertex* v3 = &c;
    for (int i = 3; i < triangles->vertexcount; ++i) {
        VSWAP(v1,v2);
        VSWAP(v2,v3);
        init_vertex(v3, &triangles->v[i], perspective);
        draw_triangle(vc4d, (VC4D_Context*)context, v1, v2, v3);
    }

    return W3D_SUCCESS;
}

ULONG
W3D_SetAlphaMode(     W3D_Context * context __asm("a0"),
     ULONG mode __asm("d1"),
     W3D_Float * refval __asm("a1"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_SUCCESS;
}

ULONG
W3D_SetBlendMode(     W3D_Context * context __asm("a0"),
     ULONG srcfunc __asm("d0"),
     ULONG dstfunc __asm("d1"),
 VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_SUCCESS;
}

ULONG
W3D_SetDrawRegion(W3D_Context * context __asm("a0"), struct BitMap * bm __asm("a1"), int yoffset __asm("d1"), W3D_Scissor * scissor __asm("a2"), VC4D* vc4d __asm("a6"))
{
    if (bm) {
        // Probably best not to do this every time...
        //if (!CheckBitMap(vc4d, bm))
        //    return W3D_ILLEGALBITMAP;
        // TODO: Maybe check bitmap a bit more?
        context->drawregion = bm;
    }
    if (scissor)
        SetScissor(vc4d, (VC4D_Context*)context, scissor, __func__);

    // TODO: Check yoffset is in range (take scissor into acount)
    context->yoffset = yoffset;
    return W3D_SUCCESS;
}

ULONG
W3D_SetFogParams(W3D_Context * context __asm("a0"), W3D_Fog * fogparams __asm("a1"), ULONG fogmode __asm("d1"), VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_SUCCESS;
}

ULONG
W3D_SetColorMask(     W3D_Context * context __asm("a0"),
     W3D_Bool red __asm("d0"),
     W3D_Bool green __asm("d1"),
     W3D_Bool blue __asm("d2"),
     W3D_Bool alpha __asm("d3"),
 VC4D* vc4d __asm("a6"))
{
    XTODO(__func__);
    return W3D_SUCCESS;
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

ULONG W3D_FreeZBuffer(W3D_Context * context __asm("a0"), VC4D* vc4d __asm("a6"))
{
#ifdef PISTORM32
    vc4_mem_free(vc4d, &((VC4D_Context*)context)->zbuffer_mem);
#endif
    return W3D_SUCCESS;
}

ULONG W3D_AllocZBuffer(W3D_Context * context __asm("a0"), VC4D* vc4d __asm("a6"))
{
    W3D_FreeZBuffer(context, vc4d);
#ifdef PISTORM32
    VC4D_Context* vctx = (VC4D_Context*)context;
    int ret = vc4_mem_alloc(vc4d, &vctx->zbuffer_mem, vctx->width * vctx->height * 4);
    if (ret) {
        LOG_ERROR("%s: Failed to allocate z-buffer! %ld\n", __func__, ret);
        return W3D_NOGFXMEM;
    }
#endif

    return W3D_SUCCESS;
}


ULONG
W3D_ClearZBuffer(W3D_Context * context __asm("a0"), W3D_Double * clearvalue __asm("a1"), VC4D* vc4d __asm("a6"))
{
#ifdef PISTORM32
    VC4D_Context* vctx = (VC4D_Context*)context;
    float* buffer = vctx->zbuffer_mem.hostptr;
    if (!buffer) {
        LOG_ERROR("%s: No zbuffer\n", __func__);
        return W3D_NOZBUFFER;
    }
    const float val = *clearvalue;
    for (ULONG i = vctx->width * vctx->height; i--; )
        *buffer++ = val;
#endif
    return W3D_SUCCESS;
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
W3D_ReadZSpan(W3D_Context * context __asm("a0"), ULONG x __asm("d0"), ULONG y __asm("d1"), ULONG n __asm("d2"), W3D_Double * z __asm("a1"), VC4D* vc4d __asm("a6"))
{
#ifdef PISTORM32
    VC4D_Context* vctx = (VC4D_Context*)context;
    float* buffer = vctx->zbuffer_mem.hostptr;
    if (!buffer) {
        LOG_ERROR("%s: No zbuffer\n", __func__);
        return W3D_NOZBUFFER;
    }

    if (y >= vctx->height || x >= vctx->width || n >= vctx->width || x + n > vctx->width) {
        LOG_ERROR("%s: x=%lu, y=%lu, n=%lu out of range!\n", __func__, x, y, n);
        return W3D_NOTVISIBLE;
    }

    buffer += x + y * vctx->width;
    while (n--)
        *z++ = *buffer++;

#endif
    return W3D_SUCCESS;
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
    XTODO(__func__);
    return W3D_SUCCESS;
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
W3D_GetDriverState(W3D_Context * context __asm("a0"), VC4D* vc4d __asm("a6"))
{
    // TODO: Actually check somehow?
    return W3D_SUCCESS;
}

ULONG
W3D_Flush(W3D_Context * context __asm("a0"), VC4D* vc4d __asm("a6"))
{
    draw_flush(vc4d, (VC4D_Context*)context);
    return W3D_SUCCESS;
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
W3D_WriteZSpan(W3D_Context * context __asm("a0"), ULONG x __asm("d0"), ULONG y __asm("d1"), ULONG n __asm("d2"), W3D_Double * z __asm("a1"), UBYTE * masks __asm("a2"), VC4D* vc4d __asm("a6"))
{
#ifdef PISTORM32
    VC4D_Context* vctx = (VC4D_Context*)context;
    float* buffer = vctx->zbuffer_mem.hostptr;
    if (!buffer) {
        LOG_ERROR("%s: No zbuffer\n", __func__);
        return;
    }

    if (y >= vctx->height || x >= vctx->width || n >= vctx->width || x + n > vctx->width) {
        LOG_ERROR("%s: x=%lu, y=%lu, n=%lu out of range!\n", __func__, x, y, n);
        return;
    }

    buffer += x + y * vctx->width;
    if (masks) {
        while (n--) {
            if (*masks)
                *buffer = *z;
            ++masks;
            ++buffer;
            ++z;
        }
    } else {
        while (n--)
            *buffer++ = *z++;
    }

#endif
}

ULONG
W3D_SetCurrentColor(W3D_Context * context __asm("a0"), W3D_Color * color __asm("a1"), VC4D* vc4d __asm("a6"))
{
    TODO(__func__);
    return W3D_SUCCESS;
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
W3D_FreeAllTexObj(W3D_Context * context __asm("a0"), VC4D* vc4d __asm("a6"))
{
    while (!IsListEmpty((struct List*)&context->restex)) {
        W3D_Texture* tex = (W3D_Texture*)((ULONG)context->restex.mlh_Head->mln_Succ - offsetof(W3D_Texture, link));
        W3D_FreeTexObj(context, tex, vc4d);
    }

    return W3D_SUCCESS;
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
    .formats = W3D_FMT_B8G8R8A8, // TODO: Support more formats...
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

static BOOL ModeSupported(VC4D* vc4d, ULONG modeId)
{
    CGFXBASE;
    if (!IsCyberModeID(modeId))
        return FALSE;
    ULONG d = GetCyberIDAttr(CYBRIDATTR_DEPTH, modeId);
    ULONG bpp = GetCyberIDAttr(CYBRIDATTR_BPPIX, modeId);
    ULONG fmt = GetCyberIDAttr(CYBRIDATTR_PIXFMT, modeId);
    return d == 24 && bpp == 4 && fmt == PIXFMT_BGRA32;
}

static BOOL ScreenModeHook(struct Hook* hook __asm("a0"), ULONG modeId __asm("a1"), struct ScreenModeRequester* req __asm("a2"))
{
    (void)req;
    struct ScreenModeHookData* ctx = hook->h_Data;
    VC4D* vc4d = ctx->vc4d;
    CGFXBASE;

    if (!ModeSupported(vc4d, modeId))
        return FALSE;

    ULONG w = GetCyberIDAttr(CYBRIDATTR_WIDTH, modeId);
    ULONG h = GetCyberIDAttr(CYBRIDATTR_HEIGHT, modeId);
    ULONG d = GetCyberIDAttr(CYBRIDATTR_DEPTH, modeId);
    ULONG bpp = GetCyberIDAttr(CYBRIDATTR_BPPIX, modeId);
    ULONG fmt = GetCyberIDAttr(CYBRIDATTR_PIXFMT, modeId);

    if (w < ctx->minw || w > ctx->maxw || h < ctx->minh || h > ctx->maxh)
        return FALSE;

    LOG_DEBUG("Mode %08lX: %lux%lux%lu bpp=%lu fmt=0x%lx\n", modeId, w, h, d, bpp, fmt);

    return TRUE;
}


ULONG
W3D_RequestMode(struct TagItem * taglist __asm("a0"), VC4D* vc4d __asm("a6"))
{
#if 0
    CGFXBASE;
    // Hard code this for a bit..
    if (IsCyberModeID(0x50031303)) {
        LOG_DEBUG("Warning: Mode hardcoded in W3D_RequestMode\n");
        return 0x50031303;
    }
#endif

    struct ScreenModeHookData hookData;
    struct Hook hook = {0};
    hook.h_Entry = (APTR)ScreenModeHook;
    hook.h_Data = &hookData;

    hookData.vc4d = vc4d;
    hookData.tags = taglist;
    hookData.minw = 0;
    hookData.maxw = ~0UL;
    hookData.minh = 0;
    hookData.maxh = ~0UL;

    struct TagItem* ti;

    struct TagItem tags[32];
    const int maxtags = sizeof(tags)/sizeof(tags) - 2; // Room for hook + done
    int numtags = 0;
    LOG_DEBUG("W3D_RequestMode\n");
    for (ti = taglist; ti->ti_Tag != TAG_DONE; ++ti)  {
        if (ti->ti_Tag >= ASL_TB && ti->ti_Tag <= ASL_LAST_TAG) {
            if (ti->ti_Tag == ASLSM_FilterFunc) {
                LOG_ERROR("Warning: overriden filter function!\n");
                continue;
            }
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

    UTILBASE;
    if ((ti = FindTagItem(ASLSM_MinWidth, taglist)) != NULL) hookData.minw = ti->ti_Data;
    if ((ti = FindTagItem(ASLSM_MaxWidth, taglist)) != NULL) hookData.maxw = ti->ti_Data;
    if ((ti = FindTagItem(ASLSM_MinHeight, taglist)) != NULL) hookData.minh = ti->ti_Data;
    if ((ti = FindTagItem(ASLSM_MaxHeight, taglist)) != NULL) hookData.maxh = ti->ti_Data;

    GFXBASE;
    LONG mode = INVALID_ID;
//static BOOL ScreenModeHook(struct Hook* hook __asm("a0"), ULONG modeId __asm("a1"), struct ScreenModeRequester* req __asm("a2"))
    for (LONG modeId = INVALID_ID; (modeId = NextDisplayInfo(modeId)) != INVALID_ID;)
    {
        if (ScreenModeHook(&hook, modeId, NULL)) {
            // More than one mode could match
            if (mode != INVALID_ID) {
                LOG_DEBUG("More than one matches\n");
                goto request;
            }
            mode = modeId;
        }
    }

    if (mode != INVALID_ID)
        LOG_DEBUG("Only one mode matches: 0x%lx\n", mode);
    else
        LOG_ERROR("No modes match!\n");
    return mode;
request:
    mode = INVALID_ID;

    SYSBASE;
    struct Library* AslBase = OpenLibrary("asl.library", 39);
    if (!AslBase) {
        LOG_DEBUG("Could not open asl.library\n");
        return INVALID_ID;
    }

    struct ScreenModeRequester *req;
    if ((req = AllocAslRequest(ASL_ScreenModeRequest, tags))) {
        if (AslRequest(req, tags))
            mode = req->sm_DisplayID;
        FreeAslRequest(req);
    } else {
        LOG_DEBUG("AllocAslRequest failed\n");
    }
    CloseLibrary(AslBase);

    LOG_DEBUG("mode selected: %lx\n", mode);

    return mode;
}

void
W3D_SetScissor(W3D_Context * context __asm("a0"), W3D_Scissor * scissor __asm("a1"), VC4D* vc4d __asm("a6"))
{
    if (scissor) {
        SetScissor(vc4d, (VC4D_Context*)context, scissor, __func__);
    } else {
        context->scissor.top = context->scissor.left = 0;
        context->scissor.width = ((VC4D_Context*)context)->width;
        context->scissor.height = ((VC4D_Context*)context)->height;
    }
}

void
W3D_FlushFrame(W3D_Context * context __asm("a0"), VC4D* vc4d __asm("a6"))
{
    draw_flush(vc4d, (VC4D_Context*)context);
}

W3D_Driver *
W3D_TestMode(ULONG ModeID __asm("d0"), VC4D* vc4d __asm("a6"))
{
    CGFXBASE;
    ULONG w = GetCyberIDAttr(CYBRIDATTR_WIDTH, ModeID);
    ULONG h = GetCyberIDAttr(CYBRIDATTR_HEIGHT, ModeID);
    ULONG d = GetCyberIDAttr(CYBRIDATTR_DEPTH, ModeID);
    ULONG bpp = GetCyberIDAttr(CYBRIDATTR_BPPIX, ModeID);
    ULONG fmt = GetCyberIDAttr(CYBRIDATTR_PIXFMT, ModeID);
    BOOL supported = ModeSupported(vc4d, ModeID);
    LOG_DEBUG("TODO: Proper W3D_TestMode 0x%lx %ldx%ldx%ld bpp=%ld fmt=0x%lx supported=%ld\n", ModeID, w, h, d, bpp, fmt, (int)supported);
    if (!supported) {
        LOG_DEBUG("TODO: %s\n", __func__);
    //    return NULL;
    }
    return (W3D_Driver*)&driver;
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
W3D_ClearDrawRegion(W3D_Context * context __asm("a0"), ULONG color __asm("d0"), VC4D* vc4d __asm("a6"))
{
    if (!context->HWlocked) {
        LOG_ERROR("%s: Hardware not locked!\n", __func__);
        return W3D_UNSUPPORTED;
    }

    // TODO: Optimize, and do correctly

    uint32_t smodulo = context->bprow / sizeof(uint32_t);
    uint32_t* screen = (uint32_t*)context->vmembase + smodulo * context->yoffset;

    const int clip_x0 = context->scissor.left;
    const int clip_x1 = context->scissor.left + context->scissor.width;
    const int clip_y0 = context->scissor.top;
    const int clip_y1 = context->scissor.top + context->scissor.height;

    for (int y = clip_y0; y < clip_y1; ++y) {
        for (int x = clip_x0; x < clip_x1; ++x) {
            screen[x + y * smodulo] = LE32(0xffff00ff);
        }
    }


    return W3D_SUCCESS;
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
W3D_DrawTriStripV(W3D_Context * context __asm("a0"), W3D_TrianglesV * triangles __asm("a1"), VC4D* vc4d __asm("a6"))
{
    if (triangles->vertexcount == 0)
        return W3D_SUCCESS;

    if (triangles->vertexcount < 3) {
        LOG_ERROR("%s: Invalid number of vertices %ld\n", __func__, triangles->vertexcount);
        return W3D_ILLEGALINPUT;
    }

    if (!context->HWlocked) {
        LOG_ERROR("%s: Hardware not locked!\n", __func__);
        return W3D_UNSUPPORTED;
    }

    draw_setup(vc4d, (VC4D_Context*)context, (VC4D_Texture*)triangles->tex);

    const BOOL perspective = (context->state & W3D_PERSPECTIVE) != 0;
    vertex a, b, c;
    init_vertex(&a, triangles->v[0], perspective);
    init_vertex(&b, triangles->v[1], perspective);
    init_vertex(&c, triangles->v[2], perspective);

    draw_triangle(vc4d, (VC4D_Context*)context, &a, &b, &c);
    vertex* v1 = &a;
    vertex* v2 = &b;
    vertex* v3 = &c;
    for (int i = 3; i < triangles->vertexcount; ++i) {
        VSWAP(v1,v2); // v2=a,v1=b
        VSWAP(v2,v3); // v2=c,v3=a
        init_vertex(v3, triangles->v[i], perspective);
        // TODO: Consider order
        draw_triangle(vc4d, (VC4D_Context*)context, v1, v2, v3);
    }

    return W3D_SUCCESS;
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
W3D_BestModeID(struct TagItem * tags __asm("a0"), VC4D* vc4d __asm("a6"))
{
    UTILBASE;
    CGFXBASE;
    GFXBASE;

    const ULONG driver     = GetTagData(W3D_BMI_DRIVER , 0, tags);
    const ULONG width      = GetTagData(W3D_BMI_WIDTH  , 0, tags);
    const ULONG height     = GetTagData(W3D_BMI_HEIGHT , 0, tags);
    const ULONG depth      = GetTagData(W3D_BMI_DEPTH  , 0, tags);

    LOG_DEBUG("%s %lux%lux%lu driver=0x%lx\n", __func__, width, height, depth, driver);

    LONG mode = INVALID_ID;
    int bestErr = 1000000;
    for (LONG modeId = INVALID_ID; (modeId = NextDisplayInfo(modeId)) != INVALID_ID;) {
        if (!ModeSupported(vc4d, modeId))
            continue;
        ULONG w = GetCyberIDAttr(CYBRIDATTR_WIDTH, modeId);
        ULONG h = GetCyberIDAttr(CYBRIDATTR_HEIGHT, modeId);
        if (w == width && h == height) {
            LOG_DEBUG("%s %lux%lu exact match found: 0x%lx\n", __func__, width, height, modeId);
            return modeId;
        }
        int we = (int)(w - width);
        int he = (int)(h - height);
        int err = we*we+he*he;
        if (err < bestErr) {
            //LOG_DEBUG("%s %lux%lu best so far 0x%lx (%lux%lu)\n", __func__, width, height, modeId, w, h);
            bestErr = err;
            mode = modeId;
        }
    }

    if (mode == INVALID_ID) {
        LOG_ERROR("%s Could not find %lux%lu mode\n", __func__, width, height);
    }

    return mode;
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
    return W3D_SUCCESS;
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

