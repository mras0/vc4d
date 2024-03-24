#include "draw.h"
#include <utility>
#include <stdint.h>

typedef uint32_t color_t;

template<typename T>
T clamp(T val, T lo, T hi)
{
    return val < lo ? lo : val > hi ? hi : val;
}

static inline int ftoi(float f)
{
    const int bias = 8192;
    return (int)(f + bias + 0.5f) - bias;
}

static inline float det(float a, float b, float c, float d)
{
    return a * d - b * c;
}

static color_t color_mul(color_t c, float rs, float gs, float bs)
{
    int r = ((c >> 16) & 0xff) * clamp(rs, 0.0f, 1.0f);
    int g = ((c >> 8) & 0xff) * clamp(gs, 0.0f, 1.0f);
    int b = ((c >> 0) & 0xff) * clamp(bs, 0.0f, 1.0f);
    return r << 16 | g << 8 | b;
}

static void store_col(color_t& dest, color_t val)
{
    // XXX TEMP
    dest = __builtin_bswap32(val);
}

static color_t sample_texture(const W3D_Texture* tex, float u, float v, float z)
{
    if (!tex)
        return ~UINT32_C(0);
    const int iu = ftoi(u * z) & (tex->texwidth - 1);
    const int iv = ftoi(v * z) & (tex->texheight - 1);

    const color_t* d = (const color_t*)tex->texsource;
    return d[iu + iv * tex->texwidth];
}

extern "C" void draw_triangle(W3D_Context* ctx, const vertex* v1, const vertex* v2, const vertex* v3, const W3D_Texture* tex)
{
     // XXX Temp
    uint32_t smodulo = ctx->bprow / sizeof(color_t);
    color_t* screen = (color_t*)ctx->vmembase + smodulo * ctx->yoffset;
    const int clip_x0 = ctx->scissor.left;
    const int clip_x1 = ctx->scissor.left + ctx->scissor.width;
    const int clip_y0 = ctx->scissor.top;
    const int clip_y1 = ctx->scissor.top + ctx->scissor.height;

    if (v1->y > v2->y)
        std::swap(v1, v2);
    if (v2->y > v3->y)
        std::swap(v2, v3);
    if (v1->y > v2->y)
        std::swap(v1, v2);

    int y1 = clamp(ftoi(v1->y), clip_y0, clip_y1);
    int y2 = clamp(ftoi(v2->y), clip_y0, clip_y1);
    int y3 = clamp(ftoi(v3->y), clip_y0, clip_y1);

    float invdet = 1.0f / det(v2->x - v1->x, v2->y - v1->y, v3->x - v1->x, v3->y - v1->y);

#define ALL_COORDS(X)   \
    X(w); \
    X(u); \
    X(v); \
    X(r); \
    X(g); \
    X(b); \
    X(a)

#define INTERP_INIT(name) \
    float name##_xdelta = det(v2->name - v1->name, v2->y - v1->y, v3->name - v1->name, v3->y - v1->y) * invdet; \
    float name##_ydelta = det(v2->x - v1->x, v2->name - v1->name, v3->x - v1->x, v3->name - v1->name) * invdet

#define INTERP_CALC(name) float name = (v1->name + (x0 + 0.5 - v1->x) * name##_xdelta + (y + 0.5 - v1->y) * name##_ydelta)

#define INTERP_UPDATE(name) name += name##_xdelta

    ALL_COORDS(INTERP_INIT);

#define PER_PIXEL \
    const float z = 1.0f / w; \
    store_col(screen[x + y * smodulo], color_mul(sample_texture(tex, u, v, z), r * z, g * z, b * z));\
    ALL_COORDS(INTERP_UPDATE)

    for (int y = y1; y < y2; ++y) {
        int x0, x1;

        x0 = ftoi((y + 0.5 - v1->y) * (v2->x - v1->x) / (v2->y - v1->y) + v1->x);
        x1 = ftoi((y + 0.5 - v1->y) * (v3->x - v1->x) / (v3->y - v1->y) + v1->x);

        x0 = clamp(x0, clip_x0, clip_x1);
        x1 = clamp(x1, clip_x0, clip_x1);


        if (x0 == x1)
            continue;

        if (x0 > x1)
            std::swap(x0, x1);

        ALL_COORDS(INTERP_CALC);
        for (int x = x0; x < x1; x++) {
            PER_PIXEL;
        }
    }

    for (int y = y2; y < y3; ++y) {
        int x0, x1;

        x0 = ftoi((y + 0.5 - v2->y) * (v3->x - v2->x) / (v3->y - v2->y) + v2->x);
        x1 = ftoi((y + 0.5 - v1->y) * (v3->x - v1->x) / (v3->y - v1->y) + v1->x);

        x0 = clamp(x0, clip_x0, clip_x1);
        x1 = clamp(x1, clip_x0, clip_x1);

        if (x0 == x1)
            continue;

        if (x0 > x1)
            std::swap(x0, x1);

        ALL_COORDS(INTERP_CALC);
        for (int x = x0; x < x1; x++) {
            PER_PIXEL;
        }
    }
#undef ALL_COORDS
#undef INTERP_INIT
#undef INTERP_CALC
#undef INTERP_UPDATE
}
