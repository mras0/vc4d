#ifndef DRAW_H
#define DRAW_H

#include <Warp3D/Warp3D.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vertex {
    float x, y, w;
    float r, g, b, a;
    float u, v;
} vertex;

void draw_triangle(W3D_Context* ctx, const vertex* v1, const vertex* v2, const vertex* v3, const W3D_Texture* tex);

#ifdef __cplusplus
}
#endif

#endif
