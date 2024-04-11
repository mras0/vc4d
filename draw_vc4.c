#include "draw.h"
#include <proto/exec.h>

// Shader identification:
// Interpolated values: Z(?), UV, RGBA (2-3 bits)
// Z-compare: 8 (3 bits)
// Tex env: 4 (2 bits)
// Tex repeat/clamp: 2*2 (2 bits)
// Blend modes: 16*16 (8 bits) though to all are valid

#define IDENT_MASK_INTERP_UV      (1 << 0)
#define IDENT_MASK_INTERP_COLOR   (1 << 1)

#define IDENT_TEXENV_SHIFT        16 // 2 bits mapped from 1..4 to 0..3
#define IDENT_GET_TEXENV(ident)     ((ident >> IDENT_TEXENV_SHIFT) + 1)
#define IDENT_SET_TEXENV(texenv)    ((texenv - 1) << IDENT_TEXENV_SHIFT)

#define XSTEP 16

#define FMIN(a,b) ((a) < (b) ? (a) : (b))
#define FMAX(a,b) ((a) > (b) ? (a) : (b))

static inline float orient2d(const vertex* a, const vertex* b, const vertex* c)
{
    return (b->x - a->x) * (c->y - a->y) - (b->y - a->y) * (c->x - a->x);
}

static inline int imin(int a, int b)
{
    return a < b ? a : b;
}

static inline int imax(int a, int b)
{
    return a > b ? a : b;
}

static inline float fmin3(float a, float b, float c)
{
    return FMIN(a, FMIN(b, c));
}

static inline float fmax3(float a, float b, float c)
{
    return FMAX(a, FMAX(b, c));
}

static inline int ftoi(float f)
{
    const int bias = 8192;
    return (int)(f + bias + 0.5f) - bias;
}

static inline uint32_t lef32(float f)
{
    return LE32(*(uint32_t*)&f);
}

void draw_triangle(VC4D* vc4d, VC4D_Context* ctx, const vertex* v0, const vertex* v1, const vertex* v2)
{
    W3D_Context* wctx = &ctx->w3d;

    float area2 = orient2d(v0, v1, v2);
    if (area2 < 0) {
        // TODO: Back face culling
        const vertex* t = v0;
        v0 = v2;
        v2 = t;
        area2 = -area2;
    }

    int minX = imax(wctx->scissor.left,                       ftoi(fmin3(v0->x, v1->x, v2->x)));
    int maxX = imin(wctx->scissor.left + wctx->scissor.width, ftoi(fmax3(v0->x, v1->x, v2->x)));
    int minY = imax(wctx->scissor.top,                        ftoi(fmin3(v0->y, v1->y, v2->y)));
    int maxY = imin(wctx->scissor.top + wctx->scissor.height, ftoi(fmax3(v0->y, v1->y, v2->y)));

    if (minX == maxX || minY == maxY)
        return;

    minX = minX & ~(XSTEP - 1);
    maxX = (maxX + (XSTEP - 1)) & ~(XSTEP - 1);

    const float A01 = v0->y - v1->y, B01 = v1->x - v0->x;
    const float A12 = v1->y - v2->y, B12 = v2->x - v1->x;
    const float A20 = v2->y - v0->y, B20 = v0->x - v2->x;

    vertex p = { .x = (float)(minX + 0.5f), .y = (float)(minY + 0.5f) };
    const float w0_row = orient2d(v1, v2, &p);
    const float w1_row = orient2d(v2, v0, &p);
    const float w2_row = orient2d(v0, v1, &p);
    const float invarea2 = 1.0f / area2;

    uint32_t* unif = ctx->uniform_mem.hostptr;
    if (*unif >= BATCH_MAX_TRINAGLES)
        draw_flush(vc4d, ctx);


    uint32_t idx = 1 + ctx->uniform_offset;

    unif[idx++] = LE32((maxX - minX) / XSTEP);
    unif[idx++] = LE32((maxY - minY + (VC4_MAX_QPUS - 1)) / VC4_MAX_QPUS);
    unif[idx++] = LE32(PHYS_TO_BUS((ULONG)wctx->vmembase + 4*minX + wctx->bprow*(minY + wctx->yoffset)));
    unif[idx++] = LE32(wctx->bprow);
    unif[idx++] = lef32(A01);
    unif[idx++] = lef32(A12);
    unif[idx++] = lef32(A20);
    unif[idx++] = lef32(B01);
    unif[idx++] = lef32(B12);
    unif[idx++] = lef32(B20);
    unif[idx++] = lef32(w0_row);
    unif[idx++] = lef32(w1_row);
    unif[idx++] = lef32(w2_row);
#define VARYING(n) \
    unif[idx++] = lef32(v0->n); \
    unif[idx++] = lef32((v1->n - v0->n) * invarea2); \
    unif[idx++] = lef32((v2->n - v0->n) * invarea2)
    VARYING(w);

    unif[idx++] = ctx->texinfo[0]; // tex addr
    unif[idx++] = ctx->texinfo[1]; // tex w
    unif[idx++] = ctx->texinfo[2]; // tex h
    VARYING(u);
    VARYING(v);

    VARYING(r);
    VARYING(g);
    VARYING(b);
    VARYING(a);
#undef VARYING
    ctx->uniform_offset = idx - 1;
    ++*unif;
}

static const uint32_t qpu_loop[] = {
#include "loop.h"
};
static const uint32_t qpu_body[] = {
#include "body.h"
};
static const uint32_t qpu_tex_lookup_wrap[] = {
#include "tex_lookup_wrap.h"
};
static const uint32_t qpu_modulate[] = {
#include "w3d_modulate.h"
};
static const uint32_t qpu_replace[] = {
#include "w3d_replace.h"
};
static const uint32_t qpu_interp_color[] = {
#include "interp_color.h"
};

#define IS_BRANCH(x) ((x) >> 28 == 15)

static uint32_t* merge_code(uint32_t* dest, const uint32_t* loop, unsigned loop_icnt, const uint32_t* body, unsigned body_icnt)
{
    const uint32_t adjust = (body_icnt - 1) * 8; // -1 for marker
    while (loop_icnt--) {
        uint32_t w0 = loop[0];
        uint32_t w1 = loop[1];
        loop += 2;

        if (w0 == UINT32_MAX && w1 == UINT32_MAX) {
            for (unsigned i = 0; i < body_icnt * 2; ++i)
                *dest++ = LE32(body[i]);
            continue;
        }

        if (IS_BRANCH(w1)) {
            //assert((w1 >> 19) & 1); // Absolute branches not supported
            if ((int32_t)w0 < 0)
                w0 -= adjust;
            else
                w0 += adjust;
        }

        *dest++ = LE32(w0);
        *dest++ = LE32(w1);
    }

    return dest;
}

#define COPY_CODE(name) do { CopyMem(name, code, sizeof(name)); code += sizeof(name); } while (0)

static ULONG make_body(VC4D* vc4d, VC4D_Context* ctx, ULONG ident)
{
    SYSBASE;
    UBYTE* code = (UBYTE*)ctx->shader_temp;
    COPY_CODE(qpu_body);

    if (ident & IDENT_MASK_INTERP_UV)
        COPY_CODE(qpu_tex_lookup_wrap);

    if (ident & IDENT_MASK_INTERP_COLOR)
        COPY_CODE(qpu_interp_color);

    if (ident & IDENT_MASK_INTERP_UV) {
        switch (IDENT_GET_TEXENV(ident)) {
            default:
            case W3D_REPLACE:
                COPY_CODE(qpu_replace);
                break;
            //case W3D_DECAL:
            case W3D_MODULATE:
            //case W3D_BLEND:
                COPY_CODE(qpu_modulate);
                break;
        }
    }

    // TODO: Flat/Gourand only shading

    // TODO: Alpha blending

    return code - (UBYTE*)&ctx->shader_temp;
}
#undef COPY_CODE


static VC4D_Shader* make_shader(VC4D* vc4d, VC4D_Context* ctx, ULONG ident)
{
    LOG_DEBUG("Compiling shader for ident=0x%lx\n", ident);
    SYSBASE;
    VC4D_Shader* s = AllocVec(sizeof(*s), MEMF_CLEAR|MEMF_PUBLIC);
    if (!s) {
        LOG_ERROR("Failed to allocate normal memory for shader\n");
        return NULL;
    }
    s->ident = ident;
    ULONG body_size = make_body(vc4d, ctx, ident);
    LOG_DEBUG("bodysize = %lu\n", body_size);
    if (body_size > sizeof(ctx->shader_temp)) {
        LOG_ERROR("BUFFER OVERFLOWED!!!\n");
    }
    if (!body_size) {
        LOG_ERROR("Failed to make body for ident=%lX\n", ident);
        FreeVec(s);
        return NULL;
    }
    ULONG shader_bytes = sizeof(qpu_loop) + body_size;
    int ret = vc4_mem_alloc(vc4d, &s->code_mem, shader_bytes);
    if (ret) {
        LOG_ERROR("Failed to allocate memory for shader code\n", ret);
        FreeVec(s);
        return NULL;
    }
    merge_code(s->code_mem.hostptr, qpu_loop, sizeof(qpu_loop) / 8, ctx->shader_temp, body_size / 8);
    CacheClearE(s->code_mem.hostptr, shader_bytes, CACRF_ClearD);
    return s;
}

void draw_setup(VC4D* vc4d, VC4D_Context* ctx, const VC4D_Texture* tex)
{
    W3D_Context* wctx = (W3D_Context*)ctx;

    ULONG ident = 0;
    ULONG mode = wctx->state;
    if ((mode & W3D_TEXMAPPING) && tex && tex->texenv) {
        if (tex->texenv == W3D_REPLACE)
            mode &= ~W3D_GOURAUD; // Color is discarded anyway
        ident |= IDENT_SET_TEXENV(tex->texenv);
    } else {
        mode &= ~W3D_TEXMAPPING;
    }


    if (mode & W3D_GOURAUD)
        ident |= IDENT_MASK_INTERP_COLOR;
    if (mode & W3D_TEXMAPPING)
        ident |= IDENT_MASK_INTERP_UV;

    if (!ctx->cur_shader || ctx->cur_shader->ident != ident) {
        draw_flush(vc4d, ctx); // XXX

        ctx->cur_shader = NULL;
        for (VC4D_Shader* s = ctx->shader_hash[ident % VC4D_SHADER_HASH_SIZE]; s; s = s->next) {
            // TODO: Consider moving to front
            if (s->ident == ident) {
                ctx->cur_shader = s;
                break;
            }
        }

        if (!ctx->cur_shader) {
            VC4D_Shader* s = make_shader(vc4d, ctx, ident);
            if (s) {
                s->next = ctx->shader_hash[ident % VC4D_SHADER_HASH_SIZE];
                ctx->shader_hash[ident % VC4D_SHADER_HASH_SIZE] = s;
                ctx->cur_shader = s;
            }
        }
    }

    if (tex) {
        ctx->texinfo[0] = LE32(tex->texture_mem.busaddr);
        ctx->texinfo[1] = LE32(tex->w3d.texwidth);
        ctx->texinfo[2] = LE32(tex->w3d.texheight);
    } else {
        // XXX: TODO
        //ctx->texinfo[0] = LE32(ctx->shader_mem.busaddr + ctx->shader_dummy_tex_offset);
        ctx->texinfo[0] = LE32(ctx->uniform_mem.busaddr);
        ctx->texinfo[1] = LE32(1);
        ctx->texinfo[2] = LE32(1);
    }
}


void draw_flush(VC4D* vc4d, VC4D_Context* ctx)
{
    uint32_t* num_tri = (uint32_t*)ctx->uniform_mem.hostptr;
    if (*num_tri) {
        SYSBASE;
        LOG_DEBUG("Drawing %lu triangles\n", *num_tri);
        if (!ctx->cur_shader) {
            *num_tri = 0;
            ctx->uniform_offset = 0;
            LOG_ERROR("No shader!\n");
            return;
        }

        *num_tri = LE32(*num_tri);
        CacheClearE(ctx->uniform_mem.hostptr, 4*(1 + ctx->uniform_offset), CACRF_ClearD);
        vc4_run_qpu(vc4d, VC4_MAX_QPUS, ctx->cur_shader->code_mem.busaddr, ctx->uniform_mem.busaddr);
        vc4_wait_qpu(vc4d);
        ctx->uniform_offset = 0;
        *num_tri = 0;
    }
}
