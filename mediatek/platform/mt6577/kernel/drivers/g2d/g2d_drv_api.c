#include "g2d_drv.h"
#include "g2d_drv_api.h"
#include "g2d_drv_reg.h"

#include <linux/sched.h>
#include <linux/kernel.h>
#include <asm/io.h>

/*******************************************************************************/

static unsigned int _g2d_get_pitch_byte(int pitch_pixel, int color_format)
{
    unsigned int pitch_byte;

    pitch_byte = pitch_pixel;

    switch (color_format)
    {
        case G2D_COLOR_FORMAT_RGB565:
        case G2D_COLOR_FORMAT_UYVY:
            pitch_byte *= 2;
            break;

        case G2D_COLOR_FORMAT_RGB888:
        case G2D_COLOR_FORMAT_BGR888:
            pitch_byte *= 3;
            break;

        case G2D_COLOR_FORMAT_ARGB8888:
        case G2D_COLOR_FORMAT_ABGR8888:
        case G2D_COLOR_FORMAT_PARGB8888:
        case G2D_COLOR_FORMAT_PABGR8888:
        case G2D_COLOR_FORMAT_XRGB8888:
        case G2D_COLOR_FORMAT_XBGR8888:
        case G2D_COLOR_FORMAT_BGRA8888:
            pitch_byte *= 4;
            break;

        default:
            G2D_ERR("***** invalid color format (%02X) *****\n", color_format);
            ASSERT(0);
            break;
    }

    return pitch_byte;
}

static void _g2d_drv_set_dst_info(g2d_context_t *g2d_ctx)
{
   unsigned int pitch;

   // 2 byte aligned for RGB565, 4 byte aligned for ARGB8888
   SET_G2D_W2M_ADDR(g2d_ctx->dst.addr);

   // destination pitch
   pitch = _g2d_get_pitch_byte(g2d_ctx->dst.pitch, g2d_ctx->dst.format);
   ASSERT(((unsigned) pitch) <= 0x2000);
   SET_G2D_W2M_PITCH(pitch);

   // destination rectangle start point(x,y)
   ASSERT(((unsigned)((g2d_ctx->dst.offset.x) + 2048)) < 4096);
   ASSERT(((unsigned)((g2d_ctx->dst.offset.y) + 2048)) < 4096);
   SET_G2D_W2M_OFFSET(g2d_ctx->dst.offset.x, g2d_ctx->dst.offset.y);

   // setting destination color format bits
   SET_G2D_W2M_COLOR_FORMAT(g2d_ctx->dst.format);
}

static void _g2d_drv_set_layer_info(g2d_layer_t *g2d_layer, int id)
{
    unsigned int layer_width  = g2d_layer->width;
    unsigned int layer_height = g2d_layer->height;
    unsigned int layer_pitch  = _g2d_get_pitch_byte(g2d_layer->pitch, g2d_layer->format);

    CLR_G2D_LAYER_CON(id);
    SET_G2D_LAYER_CON_COLOR_FORMAT(id, g2d_layer->format);
    SET_G2D_LAYER_CON_ROTATE(id, g2d_layer->transform);
    SET_G2D_LAYER_OFFSET(id, g2d_layer->offset.x, g2d_layer->offset.y);

    if (!g2d_layer->rectangle)
    {
        SET_G2D_LAYER_ADDR(id, g2d_layer->addr);
        ASSERT(((unsigned) layer_pitch) <= 0x2000);
        SET_G2D_LAYER_PITCH(id, layer_pitch);
    }
    else
    {
        ENABLE_G2D_LAYER_CON_RECT_FILL(id);
        SET_G2D_LAYER_RECTANGLE_FILL_COLOR(id, g2d_layer->rect_color);
    }

    SET_G2D_LAYER_SIZE(id, layer_width, layer_height);

    if (g2d_layer->blending)
    {
        ENABLE_G2D_LAYER_CON_ALPHA(id);
        SET_G2D_LAYER_CON_ALPHA(id, g2d_layer->const_alpha);
    }
    else
    {
        DISABLE_G2D_LAYER_CON_ALPHA(id);
    }

    ENABLE_G2D_ROI_LAYER(id);
}

static void _g2d_drv_set_bitblt_info(g2d_context_t *g2d_ctx)
{
    int i = 0;

    REG_G2D_MODE_CON |= G2D_MODE_CON_ENG_MODE_G2D_BITBLT_BIT;
    
    DISABLE_G2D_ROI_ALL_LAYER;

    // destination config
    if (g2d_ctx->dst.enable) _g2d_drv_set_layer_info(&g2d_ctx->dst, 0);

    // source config
    for (i = 0; i < g2d_ctx->num_layer; i++)
    {
        if (!g2d_ctx->src[i].enable)
            continue;

        _g2d_drv_set_layer_info(&g2d_ctx->src[i], i+1);
    }
}

static void _g2d_drv_set_affine_info(g2d_context_t *g2d_ctx)
{
    g2d_layer_t *layer_src;

    REG_G2D_MODE_CON |= G2D_MODE_CON_ENG_MODE_G2D_AF_BIT;
    REG_G2D_MODE_CON |= g2d_ctx->affine_smpl;

    DISABLE_G2D_ROI_ALL_LAYER;

    layer_src = &g2d_ctx->src[0];

    // set matrix
    REG_G2D_SDXDX   = layer_src->sdxdx;
    REG_G2D_SDXDY   = layer_src->sdxdy;
    REG_G2D_SDYDX   = layer_src->sdydx;
    REG_G2D_SDYDY   = layer_src->sdydy;
    REG_G2D_SX_INIT = layer_src->sx_init;
    REG_G2D_SY_INIT = layer_src->sy_init;

    // destination config
    _g2d_drv_set_layer_info(&g2d_ctx->dst, 0);

    // source config
    _g2d_drv_set_layer_info(layer_src, 1);
}

static void _g2d_drv_set_mirror_fb(g2d_context_t *g2d_ctx)
{
    REG_G2D_MODE_CON |= G2D_MODE_CON_ENG_MODE_G2D_BITBLT_BIT;

    DISABLE_G2D_ROI_ALL_LAYER;

    _g2d_drv_set_layer_info(&g2d_ctx->src[0], 0);

    DISABLE_G2D_LAYER_CON_ALPHA(0);
}

static void _g2d_drv_set_roi_info(g2d_context_t *g2d_ctx)
{
    SET_G2D_ROI_OFFSET(g2d_ctx->dst.offset.x, g2d_ctx->dst.offset.y);
    SET_G2D_ROI_SIZE(g2d_ctx->dst.width, g2d_ctx->dst.height);

    if (g2d_ctx->flag & G2D_FLAG_AFFINE_TRANSFORM)
    {
        REG_G2D_ROI_CON |= G2D_ROI_CON_ENABLE_FORCE_TS_BIT;
        REG_G2D_ROI_CON |= G2D_ROI_CON_TILE_SIZE_16x8_BIT;
    }

    if (g2d_ctx->flag & G2D_FLAG_BACKGROUND_COLOR)
    {
        SET_G2D_ROI_CON_BG_COLOR(g2d_ctx->bg_color);
    }
    else
    {
        DISABLE_G2D_ROI_CON_BG;
    }
}

/*******************************************************************************/

int g2d_drv_get_status()
{
    return (REG_G2D_STATUS & G2D_STATUS_BUSY_BIT);
}

void g2d_drv_reset(void)
{
    HARD_RESET_G2D_ENGINE;
}

void g2d_drv_run(g2d_context_t *g2d_ctx)
{
#ifdef G2D_PROFILE
    struct timeval t1, t2;
    do_gettimeofday(&t1);
#endif

    HARD_RESET_G2D_ENGINE;
    REG_G2D_MODE_CON = 0;
    REG_G2D_ROI_CON = 0;
    REG_G2D_SLOW_DOWN = 0x00400000;
    REG_G2D_IRQ |= G2D_IRQ_ENABLE_BIT;

    _g2d_drv_set_dst_info(g2d_ctx);

    if (g2d_ctx->flag & G2D_FLAG_MIRROR_FRONT_FB)
        _g2d_drv_set_mirror_fb(g2d_ctx);
    else if (g2d_ctx->flag & G2D_FLAG_AFFINE_TRANSFORM)
        _g2d_drv_set_affine_info(g2d_ctx);
    else
        _g2d_drv_set_bitblt_info(g2d_ctx);

    _g2d_drv_set_roi_info(g2d_ctx);
    
#ifdef G2D_PROFILE
    do_gettimeofday(&t2);
    G2D_DBG("register config time (%u)\n", (t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec));
#endif
            
    START_G2D_ENGINE;
}
