#include "draw.h"
#include <proto/exec.h>

// Shader identification:
// Interpolated values: Z(?), UV, RGBA (2-3 bits)
// Z-compare: 8 (3 bits)
// Tex env: 4 (2 bits)
// Tex repeat/clamp: 2*2 (2 bits)
// Blend modes: 16*16 (4 bits each -> 8 bits) though to all are valid

#define IDENT_MASK_INTERP_Z         (1 << 0)
#define IDENT_MASK_INTERP_UV        (1 << 1)
#define IDENT_MASK_INTERP_COLOR     (1 << 2)
#define IDENT_MASK_BLEND            (1 << 3)

#define IDENT_ZMODE_SHIFT           13 // 3 bits mapped from 1..8 to 0..7
#define IDENT_GET_ZMODE(ident)      ((((ident) >> IDENT_ZMODE_SHIFT) & 7) + 1)
#define IDENT_SET_ZMODE(texenv)     (((texenv) - 1) << IDENT_ZMODE_SHIFT)

#define IDENT_TEXENV_SHIFT          16 // 2 bits mapped from 1..4 to 0..3
#define IDENT_GET_TEXENV(ident)     ((((ident) >> IDENT_TEXENV_SHIFT) & 3) + 1)
#define IDENT_SET_TEXENV(texenv)    (((texenv) - 1) << IDENT_TEXENV_SHIFT)

#define IDENT_BLEND_SRC_SHIFT       18
#define IDENT_GET_BLEND_SRC(ident)  ((((ident) >> IDENT_BLEND_SRC_SHIFT) & 15) + 1)
#define IDENT_SET_BLEND_SRC(mode)   (((mode) - 1) << IDENT_BLEND_SRC_SHIFT)

#define IDENT_BLEND_DST_SHIFT       22
#define IDENT_GET_BLEND_DST(ident)  ((((ident) >> IDENT_BLEND_DST_SHIFT) & 15) + 1)
#define IDENT_SET_BLEND_DST(mode)   (((mode) - 1) << IDENT_BLEND_DST_SHIFT)

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

static uint32_t pack_color_elem(float f)
{
    return f < 0 ? 0 : f > 1 ? 255 : (uint32_t)(f * 255.0f + 0.5f);
}

static inline uint32_t pack_color(const W3D_Color* c)
{
    return pack_color_elem(c->a) << 24 | pack_color_elem(c->r) << 16 | pack_color_elem(c->g) << 8 | pack_color_elem(c->b);
}

#define JOB_QUEUE_EMPTY(ctx) ((ctx)->last_done_job == (ctx)->cur_job)
#define JOB_QUEUE_FULL(ctx) ((ctx)->cur_job - (ctx)->last_done_job == QPU_NUM_JOBS)

static void update_job_queue(VC4D* vc4d, VC4D_Context* ctx)
{
    if (ctx->running_job && !vc4_qpu_active(vc4d)) {
        //LOG_DEBUG("%s: job %lu finished\n", __func__, ctx->last_done_job);
        ctx->running_job = FALSE;
        ++ctx->last_done_job;
    }

    if (JOB_QUEUE_EMPTY(ctx))
        return;

    if (!ctx->running_job) {
        //LOG_DEBUG("%s: running job %lu\n", __func__, ctx->last_done_job);
        VC4D_QPU_Job* job = &ctx->jobs[ctx->last_done_job % QPU_NUM_JOBS];
        vc4_run_qpu(vc4d, VC4_MAX_QPUS, job->shader_bus, ctx->uniform_mem.busaddr + (ctx->last_done_job % QPU_NUM_JOBS) * QPU_UNIFORMS_PER_JOB * 4);
        ctx->running_job = TRUE;
    }
}

static void init_job(VC4D* vc4d, VC4D_Context* ctx)
{
    while (JOB_QUEUE_FULL(ctx)) {
        LOG_DEBUG("%s: job queue is full\n", __func__);
        update_job_queue(vc4d, ctx);
    }

    //LOG_DEBUG("%s: job %lu initialized\n", __func__, ctx->cur_job);
    VC4D_QPU_Job* job = &ctx->jobs[ctx->cur_job % QPU_NUM_JOBS];
    uint32_t* unif = ((uint32_t*)ctx->uniform_mem.hostptr) + (ctx->cur_job % QPU_NUM_JOBS) * QPU_UNIFORMS_PER_JOB;
    job->num_uniforms = 0;
    job->shader_bus = ctx->cur_shader->code_mem.busaddr;
    unif[job->num_uniforms++] = 0; // Number of triangles
    unif[job->num_uniforms++] = ctx->texinfo[0]; // tex addr
    unif[job->num_uniforms++] = ctx->texinfo[1]; // tex w
    unif[job->num_uniforms++] = ctx->texinfo[2]; // tex h
    ctx->building_job = TRUE;
}

static void end_job(VC4D* vc4d, VC4D_Context* ctx)
{
    if (!ctx->building_job)
        return;
    ctx->building_job = FALSE;
    // First uniform stores number of triangles
    uint32_t* unif = ((uint32_t*)ctx->uniform_mem.hostptr) + (ctx->cur_job % QPU_NUM_JOBS) * QPU_UNIFORMS_PER_JOB;
    if (!*unif || !ctx->cur_shader)
        return;
    //LOG_DEBUG("%s: job %lu queued %lu triangles\n", __func__, ctx->cur_job, *unif);
    *unif = LE32(*unif);
    VC4D_QPU_Job* job = &ctx->jobs[ctx->cur_job % QPU_NUM_JOBS];
    SYSBASE;
    CacheClearE(unif, sizeof(*unif) * job->num_uniforms, CACRF_ClearD);
    ++ctx->cur_job;
    update_job_queue(vc4d, ctx);
}

void draw_triangle(VC4D* vc4d, VC4D_Context* ctx, const vertex* v0, const vertex* v1, const vertex* v2)
{
    if (!ctx->cur_shader)
        return;

    // TODO: Back face culling
    float area2 = orient2d(v0, v1, v2);
    if (area2 < 0) {
        const vertex* t = v0;
        v0 = v2;
        v2 = t;
        area2 = -area2;
    }

    // Could use actual number of uniforms for check...
    if (ctx->jobs[ctx->cur_job % QPU_NUM_JOBS].num_uniforms + 64 >= QPU_UNIFORMS_PER_JOB) {
        //LOG_DEBUG("%s: Max job size reached\n", __func__);
        end_job(vc4d, ctx);
        init_job(vc4d, ctx);
    } else {
        update_job_queue(vc4d, ctx);
    }
    // N.B. end_job changes cur_job

    const ULONG job_num = ctx->cur_job % QPU_NUM_JOBS;
    VC4D_QPU_Job* job = &ctx->jobs[job_num];
    uint32_t* const unif = ((uint32_t*)ctx->uniform_mem.hostptr) + job_num * QPU_UNIFORMS_PER_JOB;
    uint32_t idx = job->num_uniforms;


    const ULONG ident = ctx->cur_shader->ident;

    W3D_Context* wctx = &ctx->w3d;
    int minX = imax(wctx->scissor.left,                       ftoi(fmin3(v0->x, v1->x, v2->x)));
    int maxX = imin(wctx->scissor.left + wctx->scissor.width, ftoi(fmax3(v0->x, v1->x, v2->x)));
    int minY = imax(wctx->scissor.top,                        ftoi(fmin3(v0->y, v1->y, v2->y)));
    int maxY = imin(wctx->scissor.top + wctx->scissor.height, ftoi(fmax3(v0->y, v1->y, v2->y)));

    if (minX == maxX || minY == maxY)
        return;

    minX = minX & ~(XSTEP - 1);
    maxX = (maxX + (XSTEP - 1)) & ~(XSTEP - 1);

    // Ensure QPUs are tied to rows (so they can't step on each others toes)
    int mod = minY % VC4_MAX_QPUS;
    if (mod)
        minY -= mod;

    const float A01 = v0->y - v1->y, B01 = v1->x - v0->x;
    const float A12 = v1->y - v2->y, B12 = v2->x - v1->x;
    const float A20 = v2->y - v0->y, B20 = v0->x - v2->x;

    vertex p = { .x = (float)(minX + 0.5f), .y = (float)(minY + 0.5f) };
    const float w0_row = orient2d(v1, v2, &p);
    const float w1_row = orient2d(v2, v0, &p);
    const float w2_row = orient2d(v0, v1, &p);
    const float invarea2 = 1.0f / area2;

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

    if (ident & IDENT_MASK_INTERP_Z) {
        unif[idx++] = LE32((ULONG)ctx->zbuffer_mem.busaddr + 4*minX + wctx->bprow*minY);
        VARYING(z);
    }

    if (ident & IDENT_MASK_INTERP_UV) {
        VARYING(u);
        VARYING(v);
    }

    if (ident & IDENT_MASK_INTERP_COLOR) {
        VARYING(r);
        VARYING(g);
        VARYING(b);
        VARYING(a);
    }

    // TODO:
    // unif[idx++] = LE32(pack_color(&ctx->cur_tex->envcolor));
#undef VARYING
    job->num_uniforms = idx;
    ++*unif; // Increment number of triangles
}

static const uint32_t qpu_outer_loop[] = {
#include "outer_loop.h"
};
static const uint32_t qpu_body[] = {
#include "body.h"
};

static const uint32_t qpu_loop_z_none[] = {
#include "loop_z_none.h"
};
static const uint32_t qpu_loop_z_less[] = {
#include "loop_z_less.h"
};


//
// Uniform loading
//
static const uint32_t qpu_load_z[] = {
#include "load_z.h"
};
static const uint32_t qpu_load_tex[] = {
#include "load_tex.h"
};
static const uint32_t qpu_load_color[] = {
#include "load_color.h"
};

// Interpolation / texture lookup
static const uint32_t qpu_tex_lookup_wrap[] = {
#include "tex_lookup_wrap.h"
};
static const uint32_t qpu_interp_color[] = {
#include "interp_color.h"
};

// TexEnv
static const uint32_t qpu_modulate[] = {
#include "w3d_modulate.h"
};
static const uint32_t qpu_decal[] = {
#include "w3d_decal.h"
};
static const uint32_t qpu_replace[] = {
#include "w3d_replace.h"
};
static const uint32_t qpu_blend[] = {
#include "w3d_blend.h"
};

// Alpha blending
static const uint32_t qpu_blend_src_one_minus_src[] = {
#include "blend_src_one_minus_src.h"
};

#define IS_BRANCH(x) ((x) >> 28 == 15)

#define MERGE_COPY(name) do { for (uint32_t i = 0; i < sizeof(name)/sizeof(*name); ++i) *dest++ = LE32(name[i]); } while (0)

static uint32_t* merge_code(uint32_t* dest, const uint32_t* loop, uint32_t loop_icnt, const uint32_t* body, unsigned body_icnt, ULONG ident)
{
    // Copy outer loop header
    const uint32_t* outer_loop = qpu_outer_loop;
    uint32_t outer_loop_cnt = sizeof(qpu_outer_loop) / 8;
    while (outer_loop_cnt--) {
        uint32_t w0 = *outer_loop++;
        uint32_t w1 = *outer_loop++;
        if (w0 == UINT32_MAX && w1 == UINT32_MAX) // Marker
            break;
        *dest++ = LE32(w0);
        *dest++ = LE32(w1);
    }

    uint32_t * const tri_loop_start = dest;

    if (ident & IDENT_MASK_INTERP_Z)
        MERGE_COPY(qpu_load_z);
    if (ident & IDENT_MASK_INTERP_UV)
        MERGE_COPY(qpu_load_tex);
    if (ident & IDENT_MASK_INTERP_COLOR)
        MERGE_COPY(qpu_load_color);

    uint32_t adjust = (body_icnt - 1) * 8; // -1 for marker
    while (loop_icnt--) {
        uint32_t w0 = *loop++;
        uint32_t w1 = *loop++;

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

    adjust = (dest - tri_loop_start) * 4 - 8; // - 8 for marker
    while (outer_loop_cnt--) {
        uint32_t w0 = *outer_loop++;
        uint32_t w1 = *outer_loop++;
        if (IS_BRANCH(w1)) {
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
            case W3D_DECAL:
                COPY_CODE(qpu_decal);
                break;
            case W3D_MODULATE:
                COPY_CODE(qpu_modulate);
                break;
            case W3D_BLEND:
                COPY_CODE(qpu_blend);
                break;
        }
    }

    // TODO: Flat/Gourand only shading

    // TODO: Other blend modes
    if (ident & IDENT_MASK_BLEND)
        COPY_CODE(qpu_blend_src_one_minus_src);

    return code - (UBYTE*)&ctx->shader_temp;
}
#undef COPY_CODE

#define SET_LOOP(x) do { loop = x; loop_icnt = sizeof(x) / 8; shader_bytes += sizeof(x); } while (0)

static VC4D_Shader* make_shader(VC4D* vc4d, VC4D_Context* ctx, ULONG ident)
{
    LOG_DEBUG("Compiling shader for ident=0x%lx\n", ident);
    if (ident & IDENT_MASK_INTERP_Z) {
        LOG_DEBUG("\tInterpolate Z, Mode=%lu\n", IDENT_GET_ZMODE(ident));
    }
    if (ident & IDENT_MASK_INTERP_UV) {
        LOG_DEBUG("\tInterpolate UV, TexEnv=%lu\n", IDENT_GET_TEXENV(ident));
    }
    if (ident & IDENT_MASK_INTERP_COLOR) {
        LOG_DEBUG("\tInterpolate color\n");
    }
    if (ident & IDENT_MASK_INTERP_COLOR) {
        LOG_DEBUG("\tInterpolate color\n");
    }
    if (ident & IDENT_MASK_BLEND) {
        LOG_DEBUG("\tAlpha blend src=%lu dst=%lu\n", IDENT_GET_BLEND_SRC(ident), IDENT_GET_BLEND_DST(ident));
    }

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
    ULONG shader_bytes = sizeof(qpu_outer_loop) + body_size;
    if (ident & IDENT_MASK_INTERP_Z)
        shader_bytes += sizeof(qpu_load_z);
    if (ident & IDENT_MASK_INTERP_UV)
        shader_bytes += sizeof(qpu_load_tex);
    if (ident & IDENT_MASK_INTERP_COLOR)
        shader_bytes += sizeof(qpu_load_color);

    uint32_t loop_icnt;
    const uint32_t* loop;

    // TODO: Use IDENT_GET_ZMODE
    if (ident & IDENT_MASK_INTERP_Z) {
        LOG_DEBUG("Creating shader for ZMode = %lu\n", IDENT_GET_ZMODE(ident));
        SET_LOOP(qpu_loop_z_less);
    } else {
        SET_LOOP(qpu_loop_z_none);
    }

    int ret = vc4_mem_alloc(vc4d, &s->code_mem, shader_bytes);
    if (ret) {
        LOG_ERROR("Failed to allocate memory for shader code\n", ret);
        FreeVec(s);
        return NULL;
    }
    uint32_t* end = merge_code(s->code_mem.hostptr, loop, loop_icnt, ctx->shader_temp, body_size / 8, ident);
    if ((ULONG)end - (ULONG)s->code_mem.hostptr > shader_bytes) {
        LOG_ERROR("Invalid shader size! %lu > %lu\n", (ULONG)end - (ULONG)s->code_mem.hostptr, shader_bytes);
    }
    LOG_DEBUG("Code merged\n");
    CacheClearE(s->code_mem.hostptr, shader_bytes, CACRF_ClearD);
    LOG_DEBUG("Shader done\n");
    return s;
}

#undef SET_LOOP

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

    if (mode & W3D_ZBUFFER) {
        if (!ctx->zbuffer_mem.busaddr)
            mode &= ~W3D_ZBUFFER;
    }

    if (mode & W3D_ZBUFFER)
        ident |= IDENT_MASK_INTERP_Z | IDENT_SET_ZMODE(ctx->zmode);
    if (mode & W3D_GOURAUD)
        ident |= IDENT_MASK_INTERP_COLOR;
    if (mode & W3D_TEXMAPPING)
        ident |= IDENT_MASK_INTERP_UV;
    if (mode & W3D_BLENDING)
        ident |= IDENT_MASK_BLEND | IDENT_SET_BLEND_SRC(ctx->blend_srcmode) | IDENT_SET_BLEND_DST(ctx->blend_dstmode);

    int new_job = !ctx->building_job;
    if (!ctx->cur_shader || ctx->cur_shader->ident != ident) {
        new_job = 1;

        if ((mode & W3D_ZBUFFER) && !(mode & W3D_ZBUFFERUPDATE)) {
            LOG_DEBUG("%s: TODO Z-Buffer without update!\n", __func__);
        }

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
            } else {
                LOG_ERROR("Failed to make shader!\n");
            }
        }
    }

    if (ctx->cur_tex != tex) {
        new_job = 1;
        if (tex) {
            ctx->texinfo[0] = LE32(tex->texture_mem.busaddr);
            ctx->texinfo[1] = LE32(tex->w3d.texwidth);
            ctx->texinfo[2] = LE32(tex->w3d.texheight);
        }
        ctx->cur_tex = tex;
    }

    if (new_job) {
        end_job(vc4d, ctx);
        init_job(vc4d, ctx);
    }
}

void draw_flush(VC4D* vc4d, VC4D_Context* ctx)
{
    end_job(vc4d, ctx);
    //LOG_DEBUG("%s: %lu jobs\n", __func__, ctx->cur_job - ctx->last_done_job);

    // Empty job queue
    while (!JOB_QUEUE_EMPTY(ctx))
        update_job_queue(vc4d, ctx);
}
