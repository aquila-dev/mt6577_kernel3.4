#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/console.h>
#include <linux/fb.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/notifier.h>

#include "g2d_drv_fb.h"
#include "g2d_drv.h"
#include "g2d_m4u.h"

#include <mach/m4u.h>

#define FB_MAX_NUM_DEVICES FB_MAX

static int fb_table[FB_MAX_NUM_DEVICES][2];
static int fb_device_num = 0;

#if defined(MTK_M4U_SUPPORT)
static unsigned int fb_base_va = 0;
static unsigned int fb_base_mva = 0;
#endif

static int fb_front_buffer = -1;
static int fb_current_buffer = 0;
fb_buffer_t *fb_queue = NULL;

static unsigned int LCM(unsigned int x, unsigned int y)
{
    unsigned int a, b;
    a = x;
    b = y;

    while (b != 0)
    {
        unsigned int r = a % b;
        a = b;
        b = r;
    }

    return (a == 0) ? 0 : ((x / a) * y);
}

static inline unsigned int ROUND(unsigned int x, unsigned int y)
{
    unsigned int div = x / y;
    unsigned int rem = x % y;

    return (div + ((rem == 0) ? 0 : 1)) * y;
}

static int _fb_device_init(int id)
{
    struct fb_info *fb_candidate;
    struct module *fb_owner;
    unsigned long fb_size;
    int color_format;

    fb_candidate = registered_fb[id];

    if (fb_candidate == NULL)
        return -1;

    // set framebuffer to use ARGB8888
    {
        int err;
        struct fb_var_screeninfo info;
        info = fb_candidate->var;

        info.activate       = FB_ACTIVATE_NOW;
        info.bits_per_pixel = 32;
        info.transp.offset  = 24;
        info.transp.length  = 8;
        info.red.offset     = 16;
        info.red.length     = 8;
        info.green.offset   = 8;
        info.green.length   = 8;
        info.blue.offset    = 0;
        info.blue.length    = 8;

        err = fb_set_var(fb_candidate, &info);

        if (err != 0)
        {
            G2D_ERR("fb device(%d) fb_set_var failed (error: %d)\n", id, err);
            return -1;
        }
    }

    fb_size = (fb_candidate->screen_size) != 0 ? fb_candidate->screen_size : fb_candidate->fix.smem_len;

    if (fb_size == 0 || fb_candidate->fix.line_length == 0)
        return -1;

    fb_owner = fb_candidate->fbops->owner;
    if (!try_module_get(fb_owner))
    {
        G2D_ERR("fb device(%d) cannot get framebuffer module\n", id);
        return -1;
    }

    if (fb_candidate->fbops->fb_open != NULL)
    {
        int err;
        err = fb_candidate->fbops->fb_open(NULL, fb_candidate, 0);
        if (err != 0)
        {
            G2D_ERR("fb device(%d) cannot open framebuffer (error: %d)\n", id, err);
            return err;
        }
    }

    if (fb_candidate->var.bits_per_pixel == 16)
    {
        if ((fb_candidate->var.red.length == 5) &&
            (fb_candidate->var.green.length == 6) &&
            (fb_candidate->var.blue.length == 5) &&
            (fb_candidate->var.red.offset == 11) &&
            (fb_candidate->var.green.offset == 5) &&
            (fb_candidate->var.blue.offset == 0) &&
            (fb_candidate->var.red.msb_right == 0))
        {
            color_format = G2D_COLOR_FORMAT_RGB565;
        }
        else
        {
            G2D_ERR("fb device(%d) unknown format\n", id);
            return -1;
        }
    }
    else if (fb_candidate->var.bits_per_pixel == 32)
    {
        if ((fb_candidate->var.red.length == 8) &&
            (fb_candidate->var.green.length == 8) &&
            (fb_candidate->var.blue.length == 8) &&
            (fb_candidate->var.red.offset == 16) &&
            (fb_candidate->var.green.offset == 8) &&
            (fb_candidate->var.blue.offset == 0) &&
            (fb_candidate->var.red.msb_right == 0))
        {
            color_format = G2D_COLOR_FORMAT_PARGB8888;
        }
        else
        {
            G2D_ERR("fb device(%d) unknown format\n", id);
            return -1;
        }
    }
    else
    {
        G2D_ERR("fb device(%d) unknown format\n", id);
        return -1;
    }

    return color_format;
}

static inline int _g2d_drv_fb_create_buffer_queue(int idx)
{
    int i;
    struct fb_info *fb_device;
    unsigned int base_addr;
    unsigned int width;
    unsigned int height;
    unsigned int stride;
    unsigned int lcm;
    unsigned int buffer_size;
    unsigned int round_size;
    unsigned long fb_size;
    unsigned int buffer_num;
    unsigned int buffer_skip;
    unsigned int bpp;

    if (fb_device_num == 0)
    {
        G2D_ERR("cannot find fb device\n");
        return -EFAULT;
    }

    fb_device = registered_fb[fb_table[idx][0]];

    fb_queue = (fb_buffer_t *)kzalloc(sizeof(fb_buffer_t) * MAX_FB_BUFFER_COUNT, GFP_KERNEL);
    if (!fb_queue) return -EFAULT;

#if defined(MTK_M4U_SUPPORT)
    fb_base_va = (unsigned int)ioremap_nocache(fb_device->fix.smem_start, fb_device->fix.smem_len);
    g2d_allocate_mva(fb_base_va, &fb_base_mva, fb_device->fix.smem_len);
    base_addr   = fb_base_mva;
#else
    base_addr   = fb_device->fix.smem_start;
#endif

    width       = fb_device->var.xres;
    height      = fb_device->var.yres;
    stride      = fb_device->fix.line_length;
    lcm         = LCM(stride, FB_PAGE_SIZE);
    buffer_size = height * stride;
    round_size  = ROUND(buffer_size, lcm);
    fb_size     = (fb_device->screen_size) != 0 ? fb_device->screen_size : fb_device->fix.smem_len;
    bpp         = (fb_table[idx][1] == G2D_COLOR_FORMAT_RGB565) ? 2 : 4;

    buffer_num  = fb_size / round_size;
    if (buffer_num < MAX_FB_BUFFER_COUNT)
    {
        G2D_ERR("fb buffers are not enough (%d)\n", buffer_num);
        return -EFAULT;
    }

    buffer_skip = buffer_num - MAX_FB_BUFFER_COUNT;

    for (i = 0; i < MAX_FB_BUFFER_COUNT; i++)
    {
        unsigned int buffer_index = i + buffer_skip;
        unsigned int buffer_offset = buffer_index * round_size;

        sema_init(&fb_queue[i].lock, 1);

        fb_queue[i].info.addr   = base_addr + buffer_offset;
        fb_queue[i].info.idx    = i;
        fb_queue[i].info.width  = width;
        fb_queue[i].info.height = height;
        fb_queue[i].info.pitch  = stride / bpp;
        fb_queue[i].info.format = fb_table[idx][1];

        fb_queue[i].offset      = buffer_offset / stride;

        G2D_DBG("******************************\n");
        G2D_DBG("Buffer(%d)\n", i);
        G2D_DBG("  Address : %08x\n", fb_queue[i].info.addr);
        G2D_DBG("  width   : %d\n",   fb_queue[i].info.width);
        G2D_DBG("  height  : %d\n",   fb_queue[i].info.height);
        G2D_DBG("  pitch   : %d\n",   fb_queue[i].info.pitch);
        G2D_DBG("  format  : %d\n",   fb_queue[i].info.format);
        G2D_DBG("  offset  : %d\n",   fb_queue[i].offset);
    }

    G2D_INF("***** connect to fb successfully *****\n");
    return 0;
}

static inline void _g2d_drv_fb_destroy_buffer_queue(int idx)
{
    if (fb_queue)
    {
#if defined(MTK_M4U_SUPPORT)
        struct fb_info *fb_device = registered_fb[fb_table[idx][0]];
        g2d_deallocate_mva(fb_base_va, fb_base_mva, fb_device->fix.smem_len);
        iounmap((void *)fb_base_va);
        fb_base_va = 0;
        fb_base_mva = 0;
#endif

        kfree(fb_queue);
        fb_queue = NULL;
    }
}

int g2d_drv_fb_init(void)
{
    int i;

    if (fb_device_num != 0)
    {
        G2D_ERR("already init fb devices\n");
        return 0;
    }

    for (i = 0; i < FB_MAX_NUM_DEVICES; i++)
    {
        int format = _fb_device_init(i);
        if (format != -1)
        {
            fb_table[fb_device_num][0] = i;
            fb_table[fb_device_num][1] = format;
            fb_device_num++;
        }
    }

    // always use first device
    return _g2d_drv_fb_create_buffer_queue(0);
}

void g2d_drv_fb_deinit(void)
{
    _g2d_drv_fb_destroy_buffer_queue(0);
    fb_current_buffer = 0;
    fb_front_buffer = -1;

    memset(fb_table, 0, sizeof(int) * FB_MAX_NUM_DEVICES * 2);
    fb_device_num = 0;

    G2D_INF("***** disconnect to fb *****\n");
}

fb_buffer_t *g2d_drv_fb_dequeue_buffer(void)
{
    if (fb_device_num == 0)
    {
        G2D_ERR("does not connect to fb device\n");
        return NULL;
    }

    fb_current_buffer += 1;
    if (fb_current_buffer >= MAX_FB_BUFFER_COUNT) fb_current_buffer = 0;

    G2D_DBG("> try to dequeue framebuffer(%d)...\n", fb_current_buffer);

    down(&fb_queue[fb_current_buffer].lock);

    G2D_DBG("> ... dequeue framebuffer(%d) successfully\n", fb_current_buffer);

    return &fb_queue[fb_current_buffer];
}

void _g2d_drv_fb_flip(int dev_idx, int buf_idx)
{
    struct fb_info *fb_device;
    struct fb_var_screeninfo fb_var;
    unsigned long yres_virtual;
    int err;

    fb_device = registered_fb[fb_table[dev_idx][0]];
    fb_var = fb_device->var;

    fb_var.xoffset = 0;
    fb_var.yoffset = fb_queue[buf_idx].offset;

    yres_virtual = fb_queue[buf_idx].offset + fb_var.yres;

    if (fb_var.xres_virtual != fb_var.xres || fb_var.yres_virtual < yres_virtual)
    {
        fb_var.xres_virtual = fb_var.xres;
        fb_var.yres_virtual = yres_virtual;

        fb_var.activate = FB_ACTIVATE_NOW | FB_ACTIVATE_FORCE;

        err = fb_set_var(fb_device, &fb_var);
        if (err != 0)
        {
            G2D_ERR("fb device(%d) fb_set_var failed (y offset: %lu, error: %d)\n", dev_idx, fb_queue[buf_idx].offset, err);
        }
    }
    else
    {
        err = fb_pan_display(fb_device, &fb_var);
        if (err != 0)
        {
            G2D_ERR("fb device(%d) fb_pan_display failed (y offset: %lu, error: %d)\n", dev_idx, fb_queue[buf_idx].offset, err);
        }

        fb_front_buffer = buf_idx;
    }
}

void g2d_drv_fb_queue_buffer(int idx)
{
    if (fb_device_num == 0)
    {
        G2D_ERR("does not connect to fb device\n");
        return;
    }

    G2D_DBG("< try to queue framebuffer(%d)...\n", idx);

    _g2d_drv_fb_flip(0, idx);

    up(&fb_queue[idx].lock);

    G2D_DBG("< ... queue framebuffer(%d) successfully\n", idx);
}

int g2d_drv_fb_fill_mirror_cmd(g2d_context_t *ctx)
{
    g2d_layer_t *layer;
    g2d_buffer_t *buffer;

    if (fb_front_buffer < 0)
    {
        return -1;
    }

    layer = &ctx->src[0];
    buffer = &fb_queue[fb_front_buffer].info;
    layer->addr      = buffer->addr;
    layer->width     = buffer->width;
    layer->height    = buffer->height;
    layer->pitch     = buffer->pitch;
    layer->format    = buffer->format;
    layer->transform = G2D_LX_CON_ROTATE_0;
    layer->enable    = 1;

    layer = &ctx->dst;
    buffer = &fb_queue[ctx->fb_id].info;
    layer->addr      = buffer->addr;
    layer->width     = buffer->width;
    layer->height    = buffer->height;
    layer->pitch     = buffer->pitch;
    layer->format    = buffer->format;
    layer->transform = G2D_LX_CON_ROTATE_0;
    layer->enable    = 1;

    return 0;
}

