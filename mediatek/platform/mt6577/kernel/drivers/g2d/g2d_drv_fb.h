#ifndef __G2D_DRV_FB_H__
#define __G2D_DRV_FB_H__

#include "g2d_drv_api.h"

#define MAX_FB_BUFFER_COUNT 2
#define FB_PAGE_SIZE 4096

typedef struct fb_buffer {
    struct semaphore lock;
    g2d_buffer_t info;
    unsigned int offset;
} fb_buffer_t;

int g2d_drv_fb_init(void);
void g2d_drv_fb_deinit(void);
fb_buffer_t *g2d_drv_fb_dequeue_buffer(void);
void g2d_drv_fb_queue_buffer(int idx);

int g2d_drv_fb_fill_mirror_cmd(g2d_context_t *ctx);

#endif // __G2D_DRV_FB_H__
