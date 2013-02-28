#ifndef __G2D_DRV_H__
#define __G2D_DRV_H__

#include <linux/xlog.h>

//#define G2D_PROFILE

#define G2D_DEVNAME "mtkg2d"

#define G2D_QUEUE

//#define G2D_DEBUG
#ifdef G2D_DEBUG
#define G2D_DBG(...) xlog_printk(ANDROID_LOG_DEBUG, G2D_DEVNAME, __VA_ARGS__)
#else
#define G2D_DBG(...)
#endif

#define G2D_INF(...) xlog_printk(ANDROID_LOG_INFO,  G2D_DEVNAME, __VA_ARGS__)
#define G2D_WRN(...) xlog_printk(ANDROID_LOG_WRAN,  G2D_DEVNAME, __VA_ARGS__)
#define G2D_ERR(...) xlog_printk(ANDROID_LOG_ERROR, G2D_DEVNAME, __VA_ARGS__)

#define G2D_QUEUE_BUFFER_COUNT 5

#endif // __G2D_DRV_H__

