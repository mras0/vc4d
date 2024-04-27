#ifndef DRAW_H
#define DRAW_H

#include "vc4d.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vertex {
    float x, y, z, w;
    float r, g, b, a;
    float u, v;
} vertex;

int draw_init(VC4D* vc4d, VC4D_Context* ctx);
void draw_setup(VC4D* vc4d, VC4D_Context* ctx, const VC4D_Texture* tex);
void draw_triangle(VC4D* vc4d, VC4D_Context* ctx, const vertex* v1, const vertex* v2, const vertex* v3);
void draw_flush(VC4D* vc4d, VC4D_Context* ctx);
void draw_clear_region(VC4D* vc4d, VC4D_Context* ctx, ULONG dst_bus, ULONG width, ULONG height, ULONG rowdelta, ULONG value);

#ifdef __cplusplus
}
#endif

#endif
