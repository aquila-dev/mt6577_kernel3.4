#include <mach/mt6577_reg_base.h>
#include <mach/mt6577_typedefs.h>

#ifndef __G2D_DRV_REG_H__
#define __G2D_DRV_REG_H__

#define G2D_base                                             G2D_BASE
#define REG_G2D_START                                        *(volatile kal_uint16 *)(G2D_base + 0x0000)
#define REG_G2D_MODE_CON                                     *(volatile kal_uint32 *)(G2D_base + 0x0004)
#define REG_G2D_RESET                                        *(volatile kal_uint16 *)(G2D_base + 0x0008)
#define REG_G2D_STATUS                                       *(volatile kal_uint16 *)(G2D_base + 0x000C)
#define REG_G2D_IRQ                                          *(volatile kal_uint16 *)(G2D_base + 0x0010)
#define REG_G2D_SLOW_DOWN                                    *(volatile kal_uint32 *)(G2D_base + 0x0014)
#define REG_G2D_CACHE_CON                                    *(volatile kal_uint32 *)(G2D_base + 0x0018)

#define REG_G2D_ROI_CON                                      *(volatile kal_uint32 *)(G2D_base + 0x0040)
#define REG_G2D_W2M_ADDR                                     *(volatile kal_uint32 *)(G2D_base + 0x0044)
#define REG_G2D_W2M_PITCH                                    *(volatile kal_uint32 *)(G2D_base + 0x0048)
#define REG_G2D_ROI_OFS                                      *(volatile kal_uint32 *)(G2D_base + 0x004C)
#define REG_G2D_ROI_SIZE                                     *(volatile kal_uint32 *)(G2D_base + 0x0050)
#define REG_G2D_ROI_BGCLR                                    *(volatile kal_uint32 *)(G2D_base + 0x0054)

#define REG_G2D_CLP_MIN                                      *(volatile kal_uint32 *)(G2D_base + 0x0058)
#define REG_G2D_CLP_MAX                                      *(volatile kal_uint32 *)(G2D_base + 0x005C)
#define REG_G2D_AVO_CLR                                      *(volatile kal_uint32 *)(G2D_base + 0x0060)
#define REG_G2D_REP_CLR                                      *(volatile kal_uint32 *)(G2D_base + 0x0064)
#define REG_G2D_W2M_MOFS                                     *(volatile kal_uint32 *)(G2D_base + 0x0068)

#define REG_G2D_L0_CON                                       *(volatile kal_uint32 *)(G2D_base + 0x0080)
#define REG_G2D_L0_ADDR                                      *(volatile kal_uint32 *)(G2D_base + 0x0084)
#define REG_G2D_L0_PITCH                                     *(volatile kal_uint16 *)(G2D_base + 0x0088)
#define REG_G2D_L0_OFS                                       *(volatile kal_uint32 *)(G2D_base + 0x008C)
#define REG_G2D_L0_SIZE                                      *(volatile kal_uint32 *)(G2D_base + 0x0090)
#define REG_G2D_L0_SRCKEY                                    *(volatile kal_uint32 *)(G2D_base + 0x0094)
#define REG_G2D_L0_RECTANGLE_FILL_COLOR                      *(volatile kal_uint32 *)(G2D_base + 0x0094)
#define REG_G2D_L0_FONT_FOREGROUND_COLOR                     *(volatile kal_uint32 *)(G2D_base + 0x0094)

#define REG_G2D_L1_CON                                       *(volatile kal_uint32 *)(G2D_base + 0x00C0)
#define REG_G2D_L1_ADDR                                      *(volatile kal_uint32 *)(G2D_base + 0x00C4)
#define REG_G2D_L1_PITCH                                     *(volatile kal_uint16 *)(G2D_base + 0x00C8)
#define REG_G2D_L1_OFS                                       *(volatile kal_uint32 *)(G2D_base + 0x00CC)
#define REG_G2D_L1_SIZE                                      *(volatile kal_uint32 *)(G2D_base + 0x00D0)
#define REG_G2D_L1_SRCKEY                                    *(volatile kal_uint32 *)(G2D_base + 0x00D4)
#define REG_G2D_L1_RECTANGLE_FILL_COLOR                      *(volatile kal_uint32 *)(G2D_base + 0x00D4)
#define REG_G2D_L1_FONT_FOREGROUND_COLOR                     *(volatile kal_uint32 *)(G2D_base + 0x00D4)

#define REG_G2D_L2_CON                                       *(volatile kal_uint32 *)(G2D_base + 0x0100)
#define REG_G2D_L2_ADDR                                      *(volatile kal_uint32 *)(G2D_base + 0x0104)
#define REG_G2D_L2_PITCH                                     *(volatile kal_uint16 *)(G2D_base + 0x0108)
#define REG_G2D_L2_OFS                                       *(volatile kal_uint32 *)(G2D_base + 0x010C)
#define REG_G2D_L2_SIZE                                      *(volatile kal_uint32 *)(G2D_base + 0x0110)
#define REG_G2D_L2_SRCKEY                                    *(volatile kal_uint32 *)(G2D_base + 0x0114)
#define REG_G2D_L2_RECTANGLE_FILL_COLOR                      *(volatile kal_uint32 *)(G2D_base + 0x0114)
#define REG_G2D_L2_FONT_FOREGROUND_COLOR                     *(volatile kal_uint32 *)(G2D_base + 0x0114)

#define REG_G2D_L3_CON                                       *(volatile kal_uint32 *)(G2D_base + 0x0140)
#define REG_G2D_L3_ADDR                                      *(volatile kal_uint32 *)(G2D_base + 0x0144)
#define REG_G2D_L3_PITCH                                     *(volatile kal_uint16 *)(G2D_base + 0x0148)
#define REG_G2D_L3_OFS                                      *(volatile kal_uint32 *)(G2D_base + 0x014C)
#define REG_G2D_L3_SIZE                                      *(volatile kal_uint32 *)(G2D_base + 0x0150)
#define REG_G2D_L3_SRCKEY                                    *(volatile kal_uint32 *)(G2D_base + 0x0154)
#define REG_G2D_L3_RECTANGLE_FILL_COLOR                      *(volatile kal_uint32 *)(G2D_base + 0x0154)
#define REG_G2D_L3_FONT_FOREGROUND_COLOR                     *(volatile kal_uint32 *)(G2D_base + 0x0154)

// Shared Register
#define REG_G2D_SDXDX                                        REG_G2D_L2_ADDR
#define REG_G2D_SDXDY                                        REG_G2D_L2_SIZE
#define REG_G2D_SDYDX                                        REG_G2D_L2_OFS
#define REG_G2D_SDYDY                                        REG_G2D_L2_SRCKEY
#define REG_G2D_SX_INIT                                      REG_G2D_L3_ADDR
#define REG_G2D_SY_INIT                                      REG_G2D_L3_SRCKEY

#define G2D_LAYER_ADDR_OFFSET                                0x40

#define REG_G2D_LAYER_CON(n)                                 *((volatile kal_uint32 *) (G2D_base + 0x0080 + (n) * G2D_LAYER_ADDR_OFFSET))
#define REG_G2D_LAYER_ADDR(n)                                *((volatile kal_uint32 *) (G2D_base + 0x0084 + (n) * G2D_LAYER_ADDR_OFFSET))
#define REG_G2D_LAYER_PITCH(n)                               *((volatile kal_uint16 *) (G2D_base + 0x0088 + (n) * G2D_LAYER_ADDR_OFFSET))
#define REG_G2D_LAYER_OFFSET(n)                              *((volatile kal_uint32 *) (G2D_base + 0x008C + (n) * G2D_LAYER_ADDR_OFFSET))
#define REG_G2D_LAYER_SIZE(n)                                *((volatile kal_uint32 *) (G2D_base + 0x0090 + (n) * G2D_LAYER_ADDR_OFFSET))
#define REG_G2D_LAYER_SRC_KEY(n)                             *((volatile kal_uint32 *) (G2D_base + 0x0094 + (n) * G2D_LAYER_ADDR_OFFSET))
#define REG_G2D_LAYER_RECTANGLE_FILL_COLOR(n)                *((volatile kal_uint32 *) (G2D_base + 0x0094 + (n) * G2D_LAYER_ADDR_OFFSET))
#define REG_G2D_LAYER_FONT_FOREGROUND_COLOR(n)               *((volatile kal_uint32 *) (G2D_base + 0x0094 + (n) * G2D_LAYER_ADDR_OFFSET))

// bit mapping of graphic 2D engine IRQ register
#define G2D_START_BIT                                        0x00000001

// G2D_MODE_CON ENGINE_MODE
#define G2D_MODE_CON_ENG_MODE_G2D_AF_BIT                     0x0002
#define G2D_MODE_CON_ENG_MODE_G2D_BITBLT_BIT                 0x0001

// G2D_MODE_CON SMPL
#define G2D_MODE_CON_SMPL_TRUNCATED                          0x00000
#define G2D_MODE_CON_SMPL_NEAREST                            0x10000
#define G2D_MODE_CON_SMPL_BILINEAR_NO_FILTER                 0x20000
#define G2D_MODE_CON_SMPL_BILINEAR_DUP_PIXEL                 0x60000
#define G2D_MODE_CON_SMPL_BILINEAR_DST_PIXEL                 0xA0000
#define G2D_MODE_CON_SMPL_BILINEAR_USR_PIXEL                 0xE0000

// bit mapping of graphic 2D engine IRQ register
#define G2D_IRQ_ENABLE_BIT                                   0x00000001

// bit mapping of 2D engine common control register
#define G2D_RESET_WARM_RESET_BIT                             0x0001
#define G2D_RESET_HARD_RESET_BIT                             0x0002

// bit mapping of graphic 2D engine status register
#define G2D_STATUS_BUSY_BIT                                  0x00000001


#define G2D_READ_BURST_TYPE_MASK                             0x07000000
#define G2D_WRITE_BURST_TYPE_MASK                            0x00700000


#define G2D_SLOW_DOWN_COUNT_MASK                             0x0000FFFF
#define G2D_SLOW_DOWN_ENABLE_BIT                             0x80000000


#define G2D_MAX_OUT_STANDING_MASK                            0x00F0


#define G2D_ROI_CON_REPLACEMENT_ENABLE_BIT                   0x200000        // CLR_REP_EN
#define G2D_ROI_CON_CLIP_ENABLE_BIT                          0x10000
#define G2D_ROI_CON_DISABLE_BG_BIT                           0x100000        // 0x80000
#define G2D_ROI_CON_ENABLE_LAYER0_BIT                        0x80000000
#define G2D_ROI_CON_ENABLE_LAYER1_BIT                        0x40000000
#define G2D_ROI_CON_ENABLE_LAYER2_BIT                        0x20000000
#define G2D_ROI_CON_ENABLE_LAYER3_BIT                        0x10000000
#define G2D_ROI_CON_ENABLE_LAYER_MASK                        0xF0000000
#define G2D_ROI_CON_CONSTANT_ALPHA_MASK                      0xFF00          // mask for OUT_ALPHA
#define G2D_ROI_CON_ALPHA_ENABLE_BIT                         0x80            // OUT_ALP_EN
#define G2D_ROI_CON_ENABLE_FORCE_TS_BIT                      0x20000         // FRC_TS
#define G2D_ROI_CON_TILE_SIZE_8x8_BIT                        0x00000
#define G2D_ROI_CON_TILE_SIZE_16x8_BIT                       0x40000

#define G2D_COLOR_FORMAT_MASK                                0x1F

#define G2D_LX_CON_ENABLE_SRC_KEY_BIT                        0x00800000      // SKEY_EN
#define G2D_LX_CON_ENABLE_RECT_BIT                           0x00400000      // RECT_EN
#define G2D_LX_CON_ENABLE_FONT_BIT                           0x40000000      // FONT_EN
#define G2D_LX_CON_AA_FONT_BIT_MASK                          0x30000000      // IDX
#define G2D_LX_CON_CONSTANT_ALPHA_MASK                       0xFF00          // ALPHA
#define G2D_LX_CON_ALPHA_ENABLE_BIT                          0x80            // ALP_EN

#define G2D_LX_CON_ROTATE_MASK                               0x70000

//static void delay(void)
//{
//    int d; 
//    for (d = 0 ; d < 100000 ; d++) {}
//}

#define G2D_HW_OVERLAY_TOTAL_LAYER                           4

#define WARM_RESET_G2D_ENGINE                                \
    do {                                                     \
        REG_G2D_START = 0;                                   \
        REG_G2D_RESET = G2D_RESET_WARM_RESET_BIT ;           \
        while (REG_G2D_STATUS & G2D_STATUS_BUSY_BIT) {}      \
        REG_G2D_RESET = 0;                                   \
        REG_G2D_RESET = G2D_RESET_HARD_RESET_BIT ;           \
        REG_G2D_RESET = 0;                                   \
    } while (0)


#define HARD_RESET_G2D_ENGINE                                \
    do {                                                     \
        REG_G2D_START = 0;                                   \
        REG_G2D_RESET = G2D_RESET_HARD_RESET_BIT ;           \
        REG_G2D_RESET = 0;                                   \
    } while (0)

#define START_G2D_ENGINE                                     \
    do {                                                     \
        REG_G2D_START = 0;                                   \
        REG_G2D_START = G2D_START_BIT;                       \
    } while (0)

// W2M
#define SET_G2D_W2M_ADDR(addr)                               \
    do {                                                     \
        REG_G2D_W2M_ADDR = addr;                             \
    } while (0)

#define SET_G2D_W2M_PITCH(pitch)                             \
    do {                                                     \
        REG_G2D_W2M_PITCH = pitch;                           \
    } while (0)

#define SET_G2D_W2M_OFFSET(x, y)                                                            \
    do {                                                                                    \
        REG_G2D_W2M_MOFS = (((kal_int16)(x) & 0xFFFF)<< 16) | ((kal_int16)(y) & 0xFFFF);  \
    } while (0)

#define SET_G2D_W2M_COLOR_FORMAT(format)                     \
    do {                                                     \
        REG_G2D_ROI_CON &= ~G2D_COLOR_FORMAT_MASK;           \
        REG_G2D_ROI_CON |= (format & G2D_COLOR_FORMAT_MASK); \
    } while (0)

// ROI
#define DISABLE_G2D_ROI_ALL_LAYER                            \
    do {                                                     \
        REG_G2D_ROI_CON &= ~G2D_ROI_CON_ENABLE_LAYER_MASK;   \
    } while (0)

#define ENABLE_G2D_ROI_LAYER(layer)                          \
    do {                                                     \
        REG_G2D_ROI_CON |= (1 << (31 - layer));              \
    } while (0)

#define DISABLE_G2D_ROI_CON_BG                               \
    do {                                                     \
        REG_G2D_ROI_CON |= G2D_ROI_CON_DISABLE_BG_BIT;       \
    } while (0)

#define SET_G2D_ROI_CON_BG_COLOR(color)                      \
    do {                                                     \
        REG_G2D_ROI_BGCLR = color;                           \
        REG_G2D_ROI_CON &= ~G2D_ROI_CON_DISABLE_BG_BIT;      \
    } while (0)

#define ENABLE_G2D_ROI_CON_ALPHA                             \
    do {                                                     \
        REG_G2D_ROI_CON |= G2D_ROI_CON_ALPHA_ENABLE_BIT;     \
    } while (0)

#define SET_G2D_ROI_CON_ALPHA(alpha)                                          \
    do {                                                                      \
        REG_G2D_ROI_CON &= ~G2D_ROI_CON_CONSTANT_ALPHA_MASK;                  \
        REG_G2D_ROI_CON |= ((alpha << 8) & G2D_ROI_CON_CONSTANT_ALPHA_MASK);  \
    } while (0)

#define SET_G2D_ROI_OFFSET(x, y)                             \
    do {                                                     \
        REG_G2D_ROI_OFS = (((x) << 16) | (0xFFFF & (y))); \
    } while (0)

#define SET_G2D_ROI_SIZE(w, h)                               \
    do {                                                     \
        REG_G2D_ROI_SIZE = (((w) << 16) | (h));              \
    } while (0)

// Layer
#define CLR_G2D_LAYER_CON(n)                                 \
    do {                                                     \
        REG_G2D_LAYER_CON(n) = 0;                            \
    } while (0)

#define SET_G2D_LAYER_CON_COLOR_FORMAT(n, format)                  \
    do {                                                           \
        REG_G2D_LAYER_CON(n) &= ~G2D_COLOR_FORMAT_MASK;            \
        REG_G2D_LAYER_CON(n) |= (format & G2D_COLOR_FORMAT_MASK);  \
    } while (0)

#define SET_G2D_LAYER_CON_ALPHA(n, alpha)                                         \
    do {                                                                          \
        REG_G2D_LAYER_CON(n) &= ~G2D_LX_CON_CONSTANT_ALPHA_MASK;                  \
        REG_G2D_LAYER_CON(n) |= ((alpha << 8) & G2D_LX_CON_CONSTANT_ALPHA_MASK);  \
    } while (0)

#define ENABLE_G2D_LAYER_CON_ALPHA(n)                        \
    do {                                                     \
        REG_G2D_LAYER_CON(n) |= G2D_LX_CON_ALPHA_ENABLE_BIT; \
    } while (0)

#define DISABLE_G2D_LAYER_CON_ALPHA(n)                             \
    do {                                                           \
        REG_G2D_LAYER_CON(n) &= ~G2D_LX_CON_ALPHA_ENABLE_BIT;      \
    } while (0)

#define ENABLE_G2D_LAYER_CON_RECT_FILL(n)                    \
    do {                                                     \
        REG_G2D_LAYER_CON(n) |= G2D_LX_CON_ENABLE_RECT_BIT;  \
    } while (0)

#define DISABLE_G2D_LAYER_CON_RECT_FILL(n)                   \
    do {                                                     \
        REG_G2D_LAYER_CON(n) &= ~G2D_LX_CON_ENABLE_RECT_BIT; \
    } while (0)

#define ENABLE_G2D_LAYER_CON_FONT(n)                         \
    do {                                                     \
        REG_G2D_LAYER_CON(n) |= G2D_LX_CON_ENABLE_FONT_BIT;  \
    } while (0)

#define SET_G2D_LAYER_CON_AA_FONT_BIT(n, bit)                  \
    do {                                                       \
        REG_G2D_LAYER_CON(n) &= ~G2D_LX_CON_AA_FONT_BIT_MASK;  \
        REG_G2D_LAYER_CON(n) |= (bit);                         \
    } while (0)

#define ENABLE_G2D_LAYER_CON_SRC_KEY(n)                        \
    do {                                                       \
        REG_G2D_LAYER_CON(n) |= G2D_LX_CON_ENABLE_SRC_KEY_BIT; \
    } while (0)

#define DISABLE_G2D_LAYER_CON_SRC_KEY(n)                        \
    do {                                                        \
        REG_G2D_LAYER_CON(n) &= ~G2D_LX_CON_ENABLE_SRC_KEY_BIT; \
    } while (0)

#define SET_G2D_LAYER_CON_ROTATE(n, rot)                     \
    do {                                                     \
        REG_G2D_LAYER_CON(n) &= ~G2D_LX_CON_ROTATE_MASK;     \
        REG_G2D_LAYER_CON(n) |= (rot);                       \
    } while (0)

#define SET_G2D_LAYER_ADDR(n, addr)                          \
    do {                                                     \
        REG_G2D_LAYER_ADDR(n) = addr;                        \
    } while (0)

#define SET_G2D_LAYER_PITCH(n, pitch)                        \
    do {                                                     \
        REG_G2D_LAYER_PITCH(n) = pitch;                      \
    } while (0)

#define SET_G2D_LAYER_OFFSET(n, x, y)                             \
    do {                                                          \
        REG_G2D_LAYER_OFFSET(n) = (((x) << 16) | (0xFFFF & (y))); \
    } while (0)

#define SET_G2D_LAYER_SIZE(n, w, h)                          \
    do {                                                     \
        REG_G2D_LAYER_SIZE(n) = (((w) << 16) | (h));         \
    } while (0)

#define SET_G2D_LAYER_SRC_KEY(n, color)                      \
    do {                                                     \
        REG_G2D_LAYER_SRC_KEY(n) = color;                    \
    } while (0)

#define SET_G2D_LAYER_RECTANGLE_FILL_COLOR(n, color)         \
    do {                                                     \
        REG_G2D_LAYER_RECTANGLE_FILL_COLOR(n) = color;       \
    } while (0)

#define SET_G2D_LAYER_FONT_FOREGROUND_COLOR(n, color)        \
    do {                                                     \
        REG_G2D_LAYER_FONT_FOREGROUND_COLOR(n) = color;      \
    } while (0)

#endif // __G2D_DRV_REG_H__
