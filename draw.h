#ifndef DRAW_H
#define DRAW_H

#include "vc4d.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vertex {
    float x, y, w;
    float r, g, b, a;
    float u, v;
} vertex;

void draw_setup(VC4D_Context* ctx, const VC4D_Texture* tex);
void draw_triangle(VC4D_Context* ctx, const vertex* v1, const vertex* v2, const vertex* v3);

#ifdef __cplusplus
}
#endif

#endif
