#include "draw.h"
#include <proto/exec.h>

#define XSTEP 16
#define YSTEP 1

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

#define FMIN(a,b) ((a) < (b) ? (a) : (b))
#define FMAX(a,b) ((a) > (b) ? (a) : (b))

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

    minX = minX & ~(XSTEP - 1);
    maxX = (maxX + (XSTEP - 1)) & ~(XSTEP - 1);
    minY = minY & ~(YSTEP - 1);
    maxY = (maxY + (YSTEP - 1)) & ~(YSTEP - 1);

    uint32_t* unif = vc4d->uniform_mem.hostptr;
    *unif++ = LE32((maxX - minX) / XSTEP);
    *unif++ = LE32((maxY - minY) / YSTEP);
    *unif++ = LE32(PHYS_TO_BUS((ULONG)wctx->vmembase + 4*minX + wctx->bprow*(minY + wctx->yoffset)));
    *unif++ = LE32(wctx->bprow * YSTEP);
    CacheClearE(vc4d->uniform_mem.hostptr, (ULONG)unif - (ULONG)vc4d->uniform_mem.hostptr, CACRF_ClearD);
    vc4_run_qpu(vc4d, 1, vc4d->shader_mem.busaddr, vc4d->uniform_mem.busaddr);
}

void draw_setup(VC4D* vc4d, VC4D_Context* ctx, const VC4D_Texture* tex)
{
    (void)vc4d; (void)ctx; (void)tex;
}
