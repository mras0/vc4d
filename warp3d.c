#include "vc4d.h"
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/asl.h>
#include <proto/cybergraphics.h>
#include <proto/utility.h>
#include <cybergraphics/cybergraphics.h>

#include "draw.h"

#define TODO(...) LOG_DEBUG("TODO: Implement %s\n", __VA_ARGS__)
#define LOG_VAL(val) LOG_DEBUG("  %-20s = 0x%08lx\n", #val, val)

#define LOCKED_START \
    CGFXBASE; \
    if (!context->HWlocked) { \
        LOG_ERROR("%s: HW not locked!\n", __func__); \
            return W3D_NOT_SUPPORTED; \
    } \
    /* XXX: Store lock in gfxdriver */ \
    context->gfxdriver = LockBitMapTags(context->drawregion, \
            LBMI_BYTESPERROW, (ULONG)&context->bprow, \
            LBMI_BASEADDRESS, (ULONG)&context->vmembase, \
            TAG_DONE); \
    if (!context->gfxdriver) { \
        LOG_ERROR("%s: Lock failed", __func__); \
        return W3D_NOT_SUPPORTED; \
    }

#define LOCKED_END UnLockBitMap(context->gfxdriver)

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
    context->scissor.width = bm->BytesPerRow / 4; // XXX: Assumes 32-bit moed
    context->scissor.height = bm->Rows;
    if (dheight)
        context->scissor.height >>= 1; // ???

    NewList((struct List*)&context->tex);
    NewList((struct List*)&context->restex);

    ((VC4D_Context*)context)->fixedcolor.r = 
    ((VC4D_Context*)context)->fixedcolor.g = 
    ((VC4D_Context*)context)->fixedcolor.b = 
    ((VC4D_Context*)context)->fixedcolor.a = 1.0f;

    *error = W3D_SUCCESS;
    return context;
}

void
W3D_DestroyContext(W3D_Context * context __asm("a0"), VC4D* vc4d __asm("a6"))
{
    SYSBASE;

    if (context->HWlocked)
        LOG_ERROR("W3D_DestroyContext: destryoing context with locked HW!\n");

    // TODO: Probably just clean this up automatically
    if (context->zbuffer)
        LOG_ERROR("W3D_DestroyContext: destryoing context z-buffer!\n");
    if (!IsListEmpty((struct List*)&context->tex) || !IsListEmpty((struct List*)&context->restex))
        LOG_ERROR("W3D_DestroyContext: desryoing context with allocated textures\n");

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
        W3D_AUTOTEXMANAGEMENT |
        W3D_SYNCHRON |
        W3D_INDIRECT |
        W3D_GLOBALTEXENV |
        /*W3D_DOUBLEHEIGHT |  -- What does changing this imply? */
        W3D_FAST |
        /*W3D_AUTOCLIP |*/
        W3D_TEXMAPPING |
        W3D_PERSPECTIVE |
        W3D_GOURAUD |
        W3D_ZBUFFER |
        /*W3D_ZBUFFERUPDATE |*/
        W3D_BLENDING |
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

    if (state & supported) {
        if (action == W3D_ENABLE)
            context->state |= state;
        else
            context->state &= ~state;
        return W3D_SUCCESS;
    }

    for (ULONG i = 0; i < 26; ++i) {
        if (state == (1U << (i+1))) {
            LOG_DEBUG("TODO: W3D_SetState %s %s\n", action == W3D_ENABLE ? "Enable" : "Disable", StateNames[i]);
            break;
        }
    }
    return W3D_UNSUPPORTED;
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
    context->HWlocked = W3D_FALSE;
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

    LOG_DEBUG("W3D_AllocTexObj\n");
    LOG_VAL(image  );
    //LOG_VAL(format );
    LOG_DEBUG("  %-20s = 0x%08lx (%s)\n", "format", format, format <= W3D_R8G8B8A8 ? FormatNames[format] : "<Invalid>");
    LOG_VAL(width  );
    LOG_VAL(height );
    LOG_VAL(mimap  );
    LOG_VAL(palette);
    LOG_VAL(usermip);

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

    texture->texdata = AllocVec(sizeof(uint32_t) * width * height, MEMF_PUBLIC);
    if (!texture->texdata) {
        *error = W3D_NOMEMORY;
        LOG_ERROR("W3D_AllocTexObj: Out of memory for texture data\n");
        FreeVec(texture);
        return NULL;
    }

    texture->texsource = (APTR)image;
    texture->texfmtsrc = format;
    texture->texwidth = width;
    texture->texheight = height;

    UBYTE* s = (APTR)image;
    ULONG* d = texture->texdata;
    ULONG n = width * height;

    if (format == W3D_A8R8G8B8) {
        while (n--) {
            *d++ = s[0] << 24 | s[1] << 16 | s[2] << 8 | s[3];
            s += 4;
        }
    } else if (format == W3D_R8G8B8) {
        while (n--) {
            *d++ = 0xff << 24 | s[0] << 16 | s[1] << 8 | s[2];
            s += 3;
        }
    } else if (format == W3D_A4R4G4B4) {
        while (n--) {
            UWORD val = *(UWORD*)s;
            *d++ = Expand4_8(val >> 12) << 24 | Expand4_8(val >> 8) << 16 | Expand4_8(val >> 4) << 8 | Expand4_8(val);
            s += 2;
        }
    } else {
        LOG_DEBUG("TODO: Support texture format: %s\n", FormatNames[texture->texfmtsrc]);
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
    FreeVec(texture->texdata);
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
    TODO(__func__);
    return W3D_UNSUPPORTED;
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

static void init_vertex(vertex* v, const W3D_Vertex* w, BOOL perspective)
{
    v->x = w->x;
    v->y = w->y;
    v->w = w->w; // 1->0f / w->z;

    if (perspective) {
        v->u = w->u * v->w;
        v->v = w->v * v->w;
        v->r = w->color.r * v->w;
        v->g = w->color.g * v->w;
        v->b = w->color.b * v->w;
        v->a = w->color.a * v->w;
    } else {
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
    vertex a, b, c;

    const BOOL perspective = (context->state & W3D_PERSPECTIVE) != 0;

    init_vertex(&a, &triangle->v1, perspective);
    init_vertex(&b, &triangle->v2, perspective);
    init_vertex(&c, &triangle->v3, perspective);
    draw_setup((VC4D_Context*)context, (VC4D_Texture*)triangle->tex);

    LOCKED_START;
    draw_triangle((VC4D_Context*)context, &a, &b, &c);
    LOCKED_END;
    return W3D_SUCCESS;
}

ULONG
W3D_DrawTriFan(W3D_Context * context __asm("a0"), W3D_Triangles * triangles __asm("a1"), VC4D* vc4d __asm("a6"))
{
    if (triangles->vertexcount < 3) {
        LOG_ERROR("W3D_DrawTriFan: Invalid number of vertices %ld\n", triangles->vertexcount);
        return W3D_ILLEGALINPUT;
    }

    draw_setup((VC4D_Context*)context, (VC4D_Texture*)triangles->tex);

    const BOOL perspective = (context->state & W3D_PERSPECTIVE) != 0;
    vertex a, b, c;
    init_vertex(&a, &triangles->v[0], perspective);
    init_vertex(&b, &triangles->v[1], perspective);
    init_vertex(&c, &triangles->v[2], perspective);

    LOCKED_START;
    draw_triangle((VC4D_Context*)context, &a, &b, &c);
    vertex* v2 = &b;
    vertex* v3 = &c;
    for (int i = 3; i < triangles->vertexcount; ++i) {
        vertex* t = v2;
        v2 = v3;
        v3 = t;

        init_vertex(v3, &triangles->v[i], perspective);

        draw_triangle((VC4D_Context*)context, &a, v2, v3);

    }
    LOCKED_END;

    return W3D_SUCCESS;
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
W3D_SetDrawRegion(W3D_Context * context __asm("a0"), struct BitMap * bm __asm("a1"), int yoffset __asm("d1"), W3D_Scissor * scissor __asm("a2"), VC4D* vc4d __asm("a6"))
{
    if (bm) {
        // Probably best not to do this every time...
        //if (!CheckBitMap(vc4d, bm))
        //    return W3D_ILLEGALBITMAP;
        // TODO: Maybe check bitmap a bit more?
        context->drawregion = bm;
    }
    context->yoffset = yoffset;
    if (scissor) {
        // TODO: Check scissor
        context->scissor = *scissor;
    }

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
W3D_FreeZBuffer(W3D_Context * context __asm("a0"), VC4D* vc4d __asm("a6"))
{
    if (context->zbuffer) {
        LOG_DEBUG("TODO: Free Z-buffer in W3D_FreeZBuffer\n");
    }
    context->zbuffer = NULL;
    return W3D_SUCCESS;
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
W3D_RequestMode(struct TagItem * taglist __asm("a0"), VC4D* vc4d __asm("a6"))
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

