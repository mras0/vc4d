#include "draw.h"
#include <utility>
#include <stdint.h>
#include <stddef.h>

// Meh, if constexpr is not supported by gcc 6.5
#define IF_CONSTEXPR(cond) if (cond)

inline void *operator new(size_t, void *p)     throw() { return p; }
inline void *operator new[](size_t, void *p)   throw() { return p; }
inline void  operator delete  (void *, void *) throw() { };
inline void  operator delete[](void *, void *) throw() { };

typedef uint32_t color_t;

static const VC4D_Texture* current_texture;

struct fcolor {
    float a, r, g, b;
};

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

static void to_fcolor(fcolor& f, ULONG c)
{
    f.a = ((c >> 24) & 0xff) / 255.0f;
    f.r = ((c >> 16) & 0xff) / 255.0f;
    f.g = ((c >> 8) & 0xff) / 255.0f;
    f.b = ((c >> 0) & 0xff) / 255.0f;
}

static ULONG from_fcolor(const fcolor& f)
{
    return color_t(255*clamp(f.a, 0.0f, 1.0f)) << 24 |
        color_t(255*clamp(f.r, 0.0f, 1.0f)) << 16 |
        color_t(255*clamp(f.g, 0.0f, 1.0f)) << 8 |
        color_t(255*clamp(f.b, 0.0f, 1.0f));
}

// Cs -> texture source color
// Cp -> previous (texture stage) color
//
static fcolor color_mul(const fcolor& cs, const fcolor& cp)
{
    return {
        cs.a * cp.a,
        cs.r * cp.r,
        cs.g * cp.g,
        cs.b * cp.b
    };
}

static fcolor color_decal(const fcolor& cs, const fcolor& cp)
{
    // Cp(1-As) + CsAs = Cp + As*(Cs-Cp)
    return {
        cp.a,
        cp.r + cs.a * (cs.r - cp.r),
        cp.g + cs.a * (cs.g - cp.g),
        cp.b + cs.a * (cs.b - cp.b),
    };
}

static void store_col(color_t& dest, color_t val)
{
    // XXX TEMP
    dest = __builtin_bswap32(val);
}

template<ULONG TexEnv>
struct sampler {
    static color_t calc(float u, float v, const fcolor& c) {
        const W3D_Texture* wtex = &current_texture->w3d;
        const int iu = ftoi(u) & (wtex->texwidth - 1);
        const int iv = ftoi(v) & (wtex->texheight - 1);

        const color_t tex_col = ((const color_t*)wtex->texdata)[iu + iv * wtex->texwidth];

        fcolor tc;
        switch (TexEnv) {
            default:
            case W3D_REPLACE:/* unlit texturing */
                return tex_col;
            case W3D_DECAL:/* RGB: same as W3D_REPLACE with primitive (lit-texturing) */
                to_fcolor(tc, tex_col);
                return from_fcolor(color_decal(tc, c));
            case W3D_MODULATE:/* lit-texturing by modulation */
                to_fcolor(tc, tex_col);
                return from_fcolor(color_mul(tc, c));
            //case W3D_BLEND:/* blend with environment color */ 
        }
    }
};
using sampler_func_ptr = decltype(&sampler<W3D_REPLACE>::calc);

#define INTERP_DEF(name) \
    float name, name##_xdelta, name##_ydelta

#define INTERP_INIT(name) \
    name##_xdelta = det(v2->name - v1->name, v2->y - v1->y, v3->name - v1->name, v3->y - v1->y) * invdet; \
    name##_ydelta = det(v2->x - v1->x, v2->name - v1->name, v3->x - v1->x, v3->name - v1->name) * invdet

#define INTERP_CALC(name) \
    name = (v1->name + (x0 + 0.5 - v1->x) * name##_xdelta + (y + 0.5 - v1->y) * name##_ydelta)

#define INTERP_UPDATE(name) \
    name += name##_xdelta


template<ULONG mode>
struct renderer {
    static constexpr bool use_tex     = (mode & W3D_TEXMAPPING) != 0;
    static constexpr bool interp_rgba = (mode & W3D_GOURAUD) != 0;
    static constexpr bool perspective = (mode & W3D_PERSPECTIVE) != 0;

#define ALL_COORDS(X)   \
    X(w); \
    X(u); \
    X(v); \
    X(r); \
    X(g); \
    X(b); \
    X(a)

#define ALL_ENABLED(X)              \
    IF_CONSTEXPR (interp_rgba) {    \
        X(r); X(g); X(b); X(a);     \
    }                               \
    IF_CONSTEXPR (use_tex) {        \
        X(u); X(v);                 \
    }                               \
    IF_CONSTEXPR (perspective)      \
        X(w)

#define PER_PIXEL \
    const float z = 1.0f / w; \
    color_t& scr = screen[x + y * smodulo]; \
    IF_CONSTEXPR (interp_rgba) { \
        IF_CONSTEXPR(perspective) { \
            fc.a = a * z; fc.r = r * z; fc.g = g * z; fc.b = b * z; \
        } \
        IF_CONSTEXPR(!perspective) { \
            fc.a = a; fc.r = r; fc.g = g; fc.b = b; \
        }\
    } \
    IF_CONSTEXPR (use_tex) { \
        IF_CONSTEXPR(perspective) \
            store_col(scr, sample(u * z, v * z, fc));\
        IF_CONSTEXPR(!perspective) \
            store_col(scr, sample(u, v, fc));\
    } \
    IF_CONSTEXPR (!use_tex) { \
        store_col(scr, from_fcolor(fc)); \
    } \
    ALL_ENABLED(INTERP_UPDATE)

    static void draw_triangle(VC4D_Context* ctx, const vertex* v1, const vertex* v2, const vertex* v3, sampler_func_ptr sample) {
         // XXX Temp
        W3D_Context* wctx = &ctx->w3d;
        uint32_t smodulo = wctx->bprow / sizeof(color_t);
        color_t* screen = (color_t*)wctx->vmembase + smodulo * wctx->yoffset;
        const int clip_x0 = wctx->scissor.left;
        const int clip_x1 = wctx->scissor.left + wctx->scissor.width;
        const int clip_y0 = wctx->scissor.top;
        const int clip_y1 = wctx->scissor.top + wctx->scissor.height;

        // TODO: Reject very small triangles
        // TODO: Back face culling
        //
        if (v1->y > v2->y)
            std::swap(v1, v2);
        if (v2->y > v3->y)
            std::swap(v2, v3);
        if (v1->y > v2->y)
            std::swap(v1, v2);

        float invdet = 1.0f / det(v2->x - v1->x, v2->y - v1->y, v3->x - v1->x, v3->y - v1->y);

        int y1 = clamp(ftoi(v1->y), clip_y0, clip_y1);
        int y2 = clamp(ftoi(v2->y), clip_y0, clip_y1);
        int y3 = clamp(ftoi(v3->y), clip_y0, clip_y1);

        ALL_COORDS(INTERP_DEF);
        fcolor fc;
        IF_CONSTEXPR (!interp_rgba) {
            fc.r = ctx->fixedcolor.r;
            fc.g = ctx->fixedcolor.r;
            fc.b = ctx->fixedcolor.b;
            fc.a = ctx->fixedcolor.a;
        }

        ALL_ENABLED(INTERP_INIT);

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

            ALL_ENABLED(INTERP_CALC);
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

            ALL_ENABLED(INTERP_CALC);
            for (int x = x0; x < x1; x++) {
                PER_PIXEL;
            }
        }
    }
};

#define SAMPLERS(X) \
    X(W3D_REPLACE) \
    X(W3D_DECAL) \
    X(W3D_MODULATE) \
    X(W3D_BLEND)

static sampler_func_ptr samplers[] = {
    nullptr,
#define SAMPLER_FUNC(st) &sampler<st>::calc,
    SAMPLERS(SAMPLER_FUNC)
#undef SAMPLER_FUNC
};

static sampler_func_ptr texture_sampler;
static decltype(&renderer<0>::draw_triangle) draw_func;

extern "C" void draw_setup(VC4D* vc4d, VC4D_Context* ctx, const VC4D_Texture* tex)
{
    (void)vc4d;
    W3D_Context* wctx = &ctx->w3d;
    ULONG mode = wctx->state;

    if ((mode & W3D_TEXMAPPING) && tex && tex->texenv) {
        texture_sampler = samplers[tex->texenv];
        current_texture = tex;
        if (tex->texenv == W3D_REPLACE)
            mode &= ~W3D_GOURAUD; // Color is discarded anyway
    } else {
        mode &= ~W3D_TEXMAPPING;
    }

    switch (mode & (W3D_TEXMAPPING | W3D_PERSPECTIVE | W3D_GOURAUD)) {
    default:
#define MODE_CASE(m) case m: draw_func = &renderer<m>::draw_triangle; break
        MODE_CASE(0                                             );
        MODE_CASE(W3D_TEXMAPPING                                );
        MODE_CASE(                 W3D_PERSPECTIVE              );
        MODE_CASE(W3D_TEXMAPPING | W3D_PERSPECTIVE              );
        MODE_CASE(                                   W3D_GOURAUD);
        MODE_CASE(W3D_TEXMAPPING |                   W3D_GOURAUD);
        MODE_CASE(                 W3D_PERSPECTIVE | W3D_GOURAUD);
        MODE_CASE(W3D_TEXMAPPING | W3D_PERSPECTIVE | W3D_GOURAUD);
#undef MODE_CASE
    }
}


extern "C" void draw_triangle(VC4D* vc4d, VC4D_Context* ctx, const vertex* v1, const vertex* v2, const vertex* v3)
{
    (void)vc4d;
    draw_func(ctx, v1, v2, v3, texture_sampler);
}


extern "C" void draw_flush(VC4D* vc4d, VC4D_Context* ctx)
{
    (void)vc4d; (void)ctx;
}
