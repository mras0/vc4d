#include "draw.h"
#include <proto/exec.h>

#define XSTEP 16

#define FMIN(a,b) ((a) < (b) ? (a) : (b))
#define FMAX(a,b) ((a) > (b) ? (a) : (b))

static uint32_t texinfo[3];

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
    SYSBASE;
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

    vc4_wait_qpu(vc4d);

    uint32_t* unif = vc4d->uniform_mem.hostptr;
    *unif++ = LE32((maxX - minX) / XSTEP);
    *unif++ = LE32((maxY - minY + (VC4_MAX_QPUS - 1)) / VC4_MAX_QPUS);
    *unif++ = LE32(PHYS_TO_BUS((ULONG)wctx->vmembase + 4*minX + wctx->bprow*(minY + wctx->yoffset)));
    *unif++ = LE32(wctx->bprow);
    *unif++ = texinfo[0]; // tex addr
    *unif++ = texinfo[1]; // tex w
    *unif++ = texinfo[2]; // tex h
    *unif++ = lef32(A01);
    *unif++ = lef32(A12);
    *unif++ = lef32(A20);
    *unif++ = lef32(B01);
    *unif++ = lef32(B12);
    *unif++ = lef32(B20);
    *unif++ = lef32(w0_row);
    *unif++ = lef32(w1_row);
    *unif++ = lef32(w2_row);
#define VARYING(n) \
    *unif++ = lef32(v0->n); \
    *unif++ = lef32((v1->n - v0->n) * invarea2); \
    *unif++ = lef32((v2->n - v0->n) * invarea2)
    VARYING(w);
    VARYING(u);
    VARYING(v);
    VARYING(r);
    VARYING(g);
    VARYING(b);
    VARYING(a);
#undef VARYING
    CacheClearE(vc4d->uniform_mem.hostptr, (ULONG)unif - (ULONG)vc4d->uniform_mem.hostptr, CACRF_ClearD);
    vc4_run_qpu(vc4d, VC4_MAX_QPUS, vc4d->shader_mem.busaddr, vc4d->uniform_mem.busaddr);
}

void draw_setup(VC4D* vc4d, VC4D_Context* ctx, const VC4D_Texture* tex)
{
    (void)ctx;
    if (tex) {
        texinfo[0] = LE32(tex->texture_mem.busaddr);
        texinfo[1] = LE32(tex->w3d.texwidth);
        texinfo[2] = LE32(tex->w3d.texheight);
    } else {
        texinfo[0] = LE32(vc4d->shader_mem.busaddr + vc4d->shader_dummy_tex_offset);
        texinfo[1] = LE32(1);
        texinfo[2] = LE32(1);
    }
}


void draw_flush(VC4D* vc4d, VC4D_Context* ctx)
{
    (void)ctx;
    vc4_wait_qpu(vc4d);
}
