/* alps/ALPS_SW/TRUNK/MAIN/alps/kernel/drivers/fm/mt6516_fm.c
 *
 * (C) Copyright 2009 
 * MediaTek <www.MediaTek.com>
 * MingHsien Hsieh <minghsien.hsieh@MediaTek.com>
 *
 * MT6573 Jogball driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/types.h>
#include <linux/earlysuspend.h>
#include <mach/mt6575_reg_base.h>
#include <mach/mt6575_typedefs.h>
#include <mach/mt6575_gpt.h>
#include <mach/mt6575_gpio.h>
#include <mt6575_kpd.h>
#include <mach/irqs.h>
#include <asm/io.h>
#include <linux/platform_device.h>
#include <linux/autoconf.h>
#include <cust_jogball.h>
#include <mach/eint.h>
#include <cust_eint.h>
#include <linux/slab.h>

/******************************************************************************
 * export functions
*******************************************************************************/
extern void mt65xx_eint_unmask(unsigned int eint_num);
extern void mt65xx_eint_mask(unsigned int eint_num);
extern void mt65xx_eint_set_polarity(unsigned int eint_num, unsigned int pol);
extern void mt65xx_eint_set_hw_debounce(unsigned int eint_num, unsigned int ms);
extern unsigned int mt65xx_eint_set_sens(unsigned int eint_num, unsigned int sens);
extern void mt65xx_eint_registration(unsigned int eint_num, unsigned int is_deb_en, 
																		unsigned int pol, void (EINT_FUNC_PTR)(void), 
																		unsigned int is_auto_umask);
/******************************************************************************
 * macro
*******************************************************************************/
#if 0
//add
#define CUST_EINT_POLARITY_LOW              0
#define CUST_EINT_POLARITY_HIGH             1
#define CUST_EINT_DEBOUNCE_DISABLE          0
#define CUST_EINT_DEBOUNCE_ENABLE           1
#define CUST_EINT_EDGE_SENSITIVE            0
#define CUST_EINT_LEVEL_SENSITIVE           1


#define CUST_EINT_HALL_1_NUM              15
#define CUST_EINT_HALL_1_DEBOUNCE_CN      1
#define CUST_EINT_HALL_1_POLARITY         CUST_EINT_POLARITY_LOW
#define CUST_EINT_HALL_1_SENSITIVE        CUST_EINT_EDGE_SENSITIVE
#define CUST_EINT_HALL_1_DEBOUNCE_EN      CUST_EINT_DEBOUNCE_ENABLE

#define CUST_EINT_HALL_2_NUM              12
#define CUST_EINT_HALL_2_DEBOUNCE_CN      1
#define CUST_EINT_HALL_2_POLARITY         CUST_EINT_POLARITY_LOW
#define CUST_EINT_HALL_2_SENSITIVE        CUST_EINT_EDGE_SENSITIVE
#define CUST_EINT_HALL_2_DEBOUNCE_EN      CUST_EINT_DEBOUNCE_ENABLE

#define CUST_EINT_HALL_3_NUM              7
#define CUST_EINT_HALL_3_DEBOUNCE_CN      1
#define CUST_EINT_HALL_3_POLARITY         CUST_EINT_POLARITY_LOW
#define CUST_EINT_HALL_3_SENSITIVE        CUST_EINT_EDGE_SENSITIVE
#define CUST_EINT_HALL_3_DEBOUNCE_EN      CUST_EINT_DEBOUNCE_ENABLE

#define CUST_EINT_HALL_4_NUM              6
#define CUST_EINT_HALL_4_DEBOUNCE_CN      1
#define CUST_EINT_HALL_4_POLARITY         CUST_EINT_POLARITY_LOW
#define CUST_EINT_HALL_4_SENSITIVE        CUST_EINT_EDGE_SENSITIVE
#define CUST_EINT_HALL_4_DEBOUNCE_EN      CUST_EINT_DEBOUNCE_ENABLE
//end add
#endif
#define EINT_JBD_UP_NUM                     CUST_EINT_HALL_1_NUM   //ok
#define EINT_JBD_UP_DEBOUNCE_CN             CUST_EINT_HALL_1_DEBOUNCE_CN
#define EINT_JBD_UP_POLARITY                CUST_EINT_HALL_1_POLARITY
#define EINT_JBD_UP_SENSITIVE               CUST_EINT_HALL_1_SENSITIVE
#define EINT_JBD_UP_DEBOUNCE_EN             CUST_EINT_HALL_1_DEBOUNCE_EN

#define EINT_JBD_LF_NUM                     CUST_EINT_HALL_2_NUM   //ok
#define EINT_JBD_LF_DEBOUNCE_CN             CUST_EINT_HALL_2_DEBOUNCE_CN
#define EINT_JBD_LF_POLARITY                CUST_EINT_HALL_2_POLARITY
#define EINT_JBD_LF_SENSITIVE               CUST_EINT_HALL_2_SENSITIVE
#define EINT_JBD_LF_DEBOUNCE_EN             CUST_EINT_HALL_2_DEBOUNCE_EN

#define EINT_JBD_RG_NUM                     CUST_EINT_HALL_3_NUM   //ok
#define EINT_JBD_RG_DEBOUNCE_CN             CUST_EINT_HALL_3_DEBOUNCE_CN
#define EINT_JBD_RG_POLARITY                CUST_EINT_HALL_3_POLARITY
#define EINT_JBD_RG_SENSITIVE               CUST_EINT_HALL_3_SENSITIVE
#define EINT_JBD_RG_DEBOUNCE_EN             CUST_EINT_HALL_3_DEBOUNCE_EN
             
#define EINT_JBD_DN_NUM                     CUST_EINT_HALL_4_NUM  //ok
#define EINT_JBD_DN_DEBOUNCE_CN             CUST_EINT_HALL_4_DEBOUNCE_CN
#define EINT_JBD_DN_POLARITY                CUST_EINT_HALL_4_POLARITY
#define EINT_JBD_DN_SENSITIVE               CUST_EINT_HALL_4_SENSITIVE
#define EINT_JBD_DN_DEBOUNCE_EN             CUST_EINT_HALL_4_DEBOUNCE_EN
/*----------------------------------------------------------------------------*/
#define JBD_DEBUG
/*----------------------------------------------------------------------------*/
/* debug macros */
#define JBD_PFX "<JBD> "
#define JBD_MSG(a,arg...) printk(JBD_PFX a,##arg)
#define JBD_FUN(a,arg...) printk(JBD_PFX "%s\n", __func__)
#define JBD_ERR(a,arg...) printk(JBD_PFX a,##arg)
#define JBD_DBG(...)      printk(__VA_ARGS__)
/******************************************************************************
 * enumeration
*******************************************************************************/
typedef enum {
    JBD_DIR_UP  = 0,
    JBD_DIR_DN  = 1,
    JBD_DIR_LF  = 2,
    JBD_DIR_RG  = 3,
    JBD_DIR_NUM = 4,
} JBD_DIR;
/*----------------------------------------------------------------------------*/
#define JBD_BIT_UP  (1 << JBD_DIR_UP)
#define JBD_BIT_DN  (1 << JBD_DIR_DN)
#define JBD_BIT_LF  (1 << JBD_DIR_LF)
#define JBD_BIT_RG  (1 << JBD_DIR_RG)
/*----------------------------------------------------------------------------*/
typedef enum {
    JBD_TRC_KEY_DOWN    = 0x0001,
    JBD_TRC_KEY_UP      = 0x0002,   
    JBD_TRC_EINT        = 0x0004,
    JBD_TRC_TASKLET     = 0x0008,
    JBD_TRC_QUEUE       = 0x0010,
}JBD_TRC;
/******************************************************************************
 * configuration
*******************************************************************************/
/* register, address, configurations */
#define JBD_DEVICE          "mt6575-jb"
/******************************************************************************
 * structure
*******************************************************************************/
struct jbd_dir
{
    /*trackball class*/
    s16      step_x;    /*the step contributed to x-axis*/
    s16      step_y;    /*the step contributed to y-axis*/
    /*keyboard class*/
    int      eint_no;   /*eint number*/  
    int      key_evt;   /*key event*/
    atomic_t val;       /*current value in the direction, poll mode only*/
    atomic_t cnt;       /*counter, poll & eint mode*/
    atomic_t polarity;  /*eint polarity*/
    int64_t  last_trig; /*last trigger time. converted from timespec*/
    int64_t  first_acc; /*the time stamp for first accumulation, it's defined when count becomes from zero to positive*/
};
/*----------------------------------------------------------------------------*/
struct eint_queue
{
    atomic_t head;
    atomic_t tail;
    atomic_t data[20];
    int size;
};
/*----------------------------------------------------------------------------*/
struct jbd_device
{
    struct jogball_hw *hw;    
    struct input_dev *dev;
    struct timer_list timer;
    atomic_t    report_cls;
    
    /*trackball class*/
    struct tasklet_struct eint_ball;
    atomic_t    gain_x;
    atomic_t    gain_y;
    struct eint_queue queue;

    /*keybaord class*/
    struct tasklet_struct eint_key;
    struct tasklet_struct poll_key;
    atomic_t detect; /*functionality config: 0: eint; 1: polling*/
    atomic_t delay;  /*period of tasklet*/
    atomic_t step;   /*minimum step to trigger key event*/
    atomic_t trace;  /*trace on/off*/
    int    pressed;
 
    int64_t  last_time; /*the last time stamp*/   
    atomic_t acc_cnt;   /*reset counter if the event is triggered in the period of time*/
    atomic_t inact_cnt; /*reset counter if the difference of time out exceeds this value*/
    atomic_t act_cnt;   /*not increase counter if interrupt is trigger inside a period*/

    u_long   pending;   /*bit mask, to indicate if event is pending in some direction*/    
    struct jbd_dir dir[JBD_DIR_NUM];

    /*early suspend*/
#if defined(CONFIG_HAS_EARLYSUSPEND)    
    struct early_suspend early_drv;
#endif     
};
/*----------------------------------------------------------------------------*/
struct jbd_eint_hlr 
{
    void (*eint_up);
    void (*eint_lf);
    void (*eint_rg);
    void (*eint_dn);
};
/*----------------------------------------------------------------------------*/
static struct jbd_device *g_jbd_ptr = NULL;
static struct timeval t1,t2;
/******************************************************************************
 * Sysfs attributes
*******************************************************************************/
static ssize_t jbd_show_config(struct device* dev, 
                              struct device_attribute *attr, char *buf)
{
    ssize_t res;
    struct jbd_device *obj;
    if (!dev) {
        JBD_ERR("dev is null!!\n");
        return 0;
    } else if (!(obj = (struct jbd_device*)dev_get_drvdata(dev))) {
        JBD_ERR("drv data is null!!\n");
        return 0;
    }
    res = snprintf(buf, PAGE_SIZE, "(%d, %d, %d)\n", 
                   atomic_read(&obj->detect), atomic_read(&obj->delay), atomic_read(&obj->step));     
    return res;    
}
/*----------------------------------------------------------------------------*/
static ssize_t jbd_store_config(struct device* dev, struct device_attribute *attr,
                               const char *buf, size_t count)
{
    struct jbd_device *obj;
    int detect, delay, step;
    if (!dev) {
        JBD_ERR("dev is null!!\n");
        return 0;
    } else if (!(obj = (struct jbd_device*)dev_get_drvdata(dev))) {
        JBD_ERR("drv data is null!!\n");
        return 0;
    }
    if (3 == sscanf(buf, "%d %d %d", &detect, &delay, &step)) {
        atomic_set(&obj->detect, detect);
        atomic_set(&obj->delay, delay);
        atomic_set(&obj->step, step);
    } else {
        JBD_ERR("invalid content: '%s', length = %d\n", buf, count);
    }
    return count;    
}
/*----------------------------------------------------------------------------*/
static ssize_t jbd_show_count(struct device* dev, 
                              struct device_attribute *attr, char *buf)
{
    ssize_t res;
    struct jbd_device *obj;
    if (!dev) {
        JBD_ERR("dev is null!!\n");
        return 0;
    } else if (!(obj = (struct jbd_device*)dev_get_drvdata(dev))) {
        JBD_ERR("drv data is null!!\n");
        return 0;
    }
    res = snprintf(buf, PAGE_SIZE, "(%d, %d, %d)\n", atomic_read(&obj->acc_cnt),
                   atomic_read(&obj->inact_cnt), atomic_read(&obj->act_cnt));     
    return res;    
}
/*----------------------------------------------------------------------------*/
static ssize_t jbd_store_count(struct device* dev, struct device_attribute *attr,
                               const char *buf, size_t count)
{
    struct jbd_device *obj;
    int inact, act, acc;
    if (!dev) {
        JBD_ERR("dev is null!!\n");
        return 0;
    } else if (!(obj = (struct jbd_device*)dev_get_drvdata(dev))) {
        JBD_ERR("drv data is null!!\n");
        return 0;
    }
    if (3 == sscanf(buf, "%d %d %d", &acc, &inact, &act)) {
        atomic_set(&obj->acc_cnt, acc);
        atomic_set(&obj->inact_cnt, inact);
        atomic_set(&obj->act_cnt, act);
    } else {
        JBD_ERR("invalid content: '%s', length = %d\n", buf, count);
    }
    return count;    
}
/*----------------------------------------------------------------------------*/
static ssize_t jbd_show_trace(struct device* dev, 
                             struct device_attribute *attr, char *buf)
{
    ssize_t res;
    struct jbd_device *obj;
    if (!dev) {
        JBD_ERR("dev is null!!\n");
        return 0;
    } else if (!(obj = (struct jbd_device*)dev_get_drvdata(dev))) {
        JBD_ERR("drv data is null!!\n");
        return 0;
    }
    res = snprintf(buf, PAGE_SIZE, "0x%04X\n", atomic_read(&obj->trace));     
    return res;    
}
/*----------------------------------------------------------------------------*/
static ssize_t jbd_store_trace(struct device* dev, struct device_attribute *attr,
                              const char *buf, size_t count)
{
    struct jbd_device *obj;
    int trace;
    if (!dev) {
        JBD_ERR("dev is null!!\n");
        return 0;
    } else if (!(obj = (struct jbd_device*)dev_get_drvdata(dev))) {
        JBD_ERR("drv data is null!!\n");
        return 0;
    }
    if (1 == sscanf(buf, "0x%x", &trace)) 
        atomic_set(&obj->trace, trace);
    else 
        JBD_ERR("invalid content: '%s', length = %d\n", buf, count);
    return count;    
}
/*----------------------------------------------------------------------------*/
#if 1
static ssize_t jbd_show_eint(struct device* dev, 
                             struct device_attribute *attr, char *buf)
{
    #define READREG(ADDR) (*(volatile unsigned int *)(ADDR))
    struct jbd_device *obj;
    int len = 0;
    if (!dev) {
        JBD_ERR("dev is null!!\n");
        return 0;
    } else if (!(obj = (struct jbd_device*)dev_get_drvdata(dev))) {
        JBD_ERR("drv data is null!!\n");
        return 0;
    }
    #if 0
    len += snprintf(buf+len, PAGE_SIZE-len, "IRQ_SEL : ");
    len += snprintf(buf+len, PAGE_SIZE-len, "%08X %08X %08X %08X %08X %08X %08X %08X\n",
                    READREG(MT6516_IRQ_SEL0), READREG(MT6516_IRQ_SEL1), READREG(MT6516_IRQ_SEL2), READREG(MT6516_IRQ_SEL3),
                    READREG(MT6516_IRQ_SEL4), READREG(MT6516_IRQ_SEL5), READREG(MT6516_IRQ_SEL6), READREG(MT6516_IRQ_SEL7));
    len += snprintf(buf+len, PAGE_SIZE-len, "FIQ_SEL : %08X\n", READREG(MT6516_FIQ_SEL));
    len += snprintf(buf+len, PAGE_SIZE-len, "IRQ_MASK: %08X %08X %08X %08X %08X %08X\n",
                    READREG(MT6516_IRQ_MASKL), READREG(MT6516_IRQ_MASKH), 
                    READREG(MT6516_IRQ_MASK_CLRL), READREG(MT6516_IRQ_MASK_CLRH),
                    READREG(MT6516_IRQ_MASK_SETL), READREG(MT6516_IRQ_MASK_SETH));
    len += snprintf(buf+len, PAGE_SIZE-len, "IRQ_STA : %08X %08X\n", 
                    READREG(MT6516_IRQ_STAL), READREG(MT6516_IRQ_STAH));
    len += snprintf(buf+len, PAGE_SIZE-len, "IRQ_EOI : %08X %08X\n",
                    READREG(MT6516_IRQ_EOIL), READREG(MT6516_IRQ_EOIH));
    len += snprintf(buf+len, PAGE_SIZE-len, "IRQ_SENS: %08X %08X\n",
                    READREG(MT6516_IRQ_SENSL), READREG(MT6516_IRQ_SENSH));
    len += snprintf(buf+len, PAGE_SIZE-len, "IRQ_SOFT: %08X %08X\n",
                    READREG(MT6516_IRQ_SOFTL), READREG(MT6516_IRQ_SOFTH));
    len += snprintf(buf+len, PAGE_SIZE-len, "FIQ_CON : %08X\n", READREG(MT6516_FIQ_CON));
    len += snprintf(buf+len, PAGE_SIZE-len, "FIQ_CON : %08X\n", READREG(MT6516_FIQ_EOI));
    len += snprintf(buf+len, PAGE_SIZE-len, "IRQ_STA2: %08X\n", READREG(MT6516_IRQ_STA2));
    len += snprintf(buf+len, PAGE_SIZE-len, "IRQ_EOI2: %08X\n", READREG(MT6516_IRQ_EOI2));
    len += snprintf(buf+len, PAGE_SIZE-len, "IRQ_SFT2: %08X\n", READREG(MT6516_IRQ_SOFT2));
    len += snprintf(buf+len, PAGE_SIZE-len, "EINTSTA : %08X\n", READREG(MT6516_EINT_STATUS));
    len += snprintf(buf+len, PAGE_SIZE-len, "EINTMASK: %08X\n", READREG(MT6516_EINT_MASK));
    len += snprintf(buf+len, PAGE_SIZE-len, "EINTMSKC: %08X\n", READREG(MT6516_EINT_MASK_CLR));
    len += snprintf(buf+len, PAGE_SIZE-len, "EINTMSKS: %08X\n", READREG(MT6516_EINT_MASK_SET));
    len += snprintf(buf+len, PAGE_SIZE-len, "EINTINTA: %08X\n", READREG(MT6516_EINT_INTACK));
    len += snprintf(buf+len, PAGE_SIZE-len, "EINTSENS: %08X\n", READREG(MT6516_EINT_SENS));
    len += snprintf(buf+len, PAGE_SIZE-len, "EINTCN00: %08X %08X %08X %08X %08X %08X %08X %08X\n",
                    READREG(MT6516_EINT0_CON), READREG(MT6516_EINT1_CON), READREG(MT6516_EINT2_CON), READREG(MT6516_EINT3_CON),
                    READREG(MT6516_EINT4_CON), READREG(MT6516_EINT5_CON), READREG(MT6516_EINT6_CON), READREG(MT6516_EINT7_CON));
    len += snprintf(buf+len, PAGE_SIZE-len, "EINTCN08: %08X %08X %08X %08X %08X %08X %08X %08X\n",
                    READREG(MT6516_EINT8_CON), READREG(MT6516_EINT9_CON), READREG(MT6516_EINT10_CON), READREG(MT6516_EINT11_CON),
                    READREG(MT6516_EINT12_CON), READREG(MT6516_EINT13_CON), READREG(MT6516_EINT14_CON), READREG(MT6516_EINT15_CON));
    len += snprintf(buf+len, PAGE_SIZE-len, "EINTCN16: %08X %08X %08X %08X %08X %08X %08X %08X\n",
                    READREG(MT6516_EINT16_CON), READREG(MT6516_EINT17_CON), READREG(MT6516_EINT18_CON), READREG(MT6516_EINT19_CON),
                    READREG(MT6516_EINT20_CON), READREG(MT6516_EINT21_CON), READREG(MT6516_EINT22_CON), READREG(MT6516_EINT23_CON));
   #endif
    return len;    
}
#endif
/*----------------------------------------------------------------------------*/
#if 1
static ssize_t jbd_store_eint(struct device* dev, struct device_attribute *attr,
                              const char *buf, size_t count)
{
    struct jbd_device *obj;
 //   char cmd[10];
//    int eint;
    if (!dev) {
        JBD_ERR("dev is null!!\n");
        return 0;
    } else if (!(obj = (struct jbd_device*)dev_get_drvdata(dev))) {
        JBD_ERR("drv data is null!!\n");
        return 0;
    }
    #if 0
    if (2 == sscanf(buf, "%s %d", cmd, &eint)) {
        if (eint >= 23) {
            JBD_ERR("invalid eintno: %d\n", eint);
        } else if (0 == strncmp(cmd, "ack", sizeof(cmd)))  {
            *MT6516_EINT_INTACK = (1 << (eint));
        } else if (0 == strncmp(cmd, "mask", sizeof(cmd))) {
            *MT6516_EINT_MASK_SET = (1 << eint);
        } else if (0 == strncmp(cmd, "unmask", sizeof(cmd))) {
            *MT6516_EINT_MASK_CLR = (1 << eint);
        } else {
            JBD_ERR("invalid cmd: '%s'\n", cmd);
        }
    } else {
        JBD_ERR("invalid content: '%s', length = %d\n", buf, count);
    }
    #endif
    return count;    
} 
#endif                             
/*----------------------------------------------------------------------------*/
static ssize_t jbd_show_cls(struct device* dev, 
                             struct device_attribute *attr, char *buf)
{
    ssize_t res;
    struct jbd_device *obj;
    if (!dev) {
        JBD_ERR("dev is null!!\n");
        return 0;
    } else if (!(obj = (struct jbd_device*)dev_get_drvdata(dev))) {
        JBD_ERR("drv data is null!!\n");
        return 0;
    }
    res = snprintf(buf, PAGE_SIZE, "%d\n", atomic_read(&obj->report_cls));     
    return res;    
}
/*----------------------------------------------------------------------------*/
static ssize_t jbd_store_cls(struct device* dev, struct device_attribute *attr,
                              const char *buf, size_t count)
{
    struct jbd_device *obj;
    int cls;
    if (!dev) {
        JBD_ERR("dev is null!!\n");
        return 0;
    } else if (!(obj = (struct jbd_device*)dev_get_drvdata(dev))) {
        JBD_ERR("drv data is null!!\n");
        return 0;
    }
    if (1 == sscanf(buf, "%d", &cls)) 
        atomic_set(&obj->report_cls, cls);
    else 
        JBD_ERR("invalid content: '%s', length = %d\n", buf, count);
    return count;    
}
/*----------------------------------------------------------------------------*/
static ssize_t jbd_show_gain(struct device* dev, 
                             struct device_attribute *attr, char *buf)
{
    ssize_t res;
    struct jbd_device *obj;
    if (!dev) {
        JBD_ERR("dev is null!!\n");
        return 0;
    } else if (!(obj = (struct jbd_device*)dev_get_drvdata(dev))) {
        JBD_ERR("drv data is null!!\n");
        return 0;
    }
    res = snprintf(buf, PAGE_SIZE, "(%d, %d)\n", atomic_read(&obj->gain_x), atomic_read(&obj->gain_y));     
    return res;    
}
/*----------------------------------------------------------------------------*/
static ssize_t jbd_store_gain(struct device* dev, struct device_attribute *attr,
                              const char *buf, size_t count)
{
    struct jbd_device *obj;
    int x, y;
    if (!dev) {
        JBD_ERR("dev is null!!\n");
        return 0;
    } else if (!(obj = (struct jbd_device*)dev_get_drvdata(dev))) {
        JBD_ERR("drv data is null!!\n");
        return 0;
    }
    if (2 == sscanf(buf, "%d %d", &x, &y)) {
        atomic_set(&obj->gain_x, x);
        atomic_set(&obj->gain_y, y);
    } else { 
        JBD_ERR("invalid content: '%s', length = %d\n", buf, count);
    }
    return count;    
}
/*----------------------------------------------------------------------------*/
#if 0
static ssize_t jbd_show_status(struct device* dev, 
                             struct device_attribute *attr, char *buf)
{
    ssize_t len;
    struct jbd_device *obj;
    if (!dev) {
        JBD_ERR("dev is null!!\n");
        return 0;
    } else if (!(obj = (struct jbd_device*)dev_get_drvdata(dev))) {
        JBD_ERR("drv data is null!!\n");
        return 0;
    }
    len += snprintf(buf+len, PAGE_SIZE-len, "GPIO: %d(%d) %d(%d) %d(%d) %d(%d)\n",
           GPIO_JBD_INPUT_UP_PIN,      GPIO_JBD_INPUT_UP_PIN_M_EINT,
           GPIO_JBD_INPUT_LEFT_PIN,    GPIO_JBD_INPUT_LEFT_PIN_M_EINT,
           GPIO_JBD_INPUT_RIGHT_PIN,   GPIO_JBD_INPUT_RIGHT_PIN_M_EINT,
           GPIO_JBD_INPUT_DOWN_PIN,    GPIO_JBD_INPUT_DOWN_PIN_M_EINT);
    len += snprintf(buf+len, PAGE_SIZE-len, "EINT: UP (%d %d %d %d %d)\n",
           EINT_JBD_UP_NUM, EINT_JBD_UP_DEBOUNCE_CN, EINT_JBD_UP_POLARITY, EINT_JBD_UP_SENSITIVE, EINT_JBD_UP_DEBOUNCE_EN);
    len += snprintf(buf+len, PAGE_SIZE-len, "EINT: LF (%d %d %d %d %d)\n",
           EINT_JBD_LF_NUM, EINT_JBD_LF_DEBOUNCE_CN, EINT_JBD_LF_POLARITY, EINT_JBD_LF_SENSITIVE, EINT_JBD_LF_DEBOUNCE_EN);
    len += snprintf(buf+len, PAGE_SIZE-len, "EINT: RG (%d %d %d %d %d)\n",
           EINT_JBD_RG_NUM, EINT_JBD_RG_DEBOUNCE_CN, EINT_JBD_RG_POLARITY, EINT_JBD_RG_SENSITIVE, EINT_JBD_RG_DEBOUNCE_EN);
    len += snprintf(buf+len, PAGE_SIZE-len, "EINT: RG (%d %d %d %d %d)\n",
           EINT_JBD_DN_NUM, EINT_JBD_DN_DEBOUNCE_CN, EINT_JBD_DN_POLARITY, EINT_JBD_DN_SENSITIVE, EINT_JBD_DN_DEBOUNCE_EN);
    if (obj->hw) {
        len += snprintf(buf+len, PAGE_SIZE-len, "CUST: %d (%d %d) (%d %d %d %d %d %d %d %d)\n", 
               obj->hw->report_cls,
               obj->hw->gain_x, obj->hw->gain_y, 
               obj->hw->detect, obj->hw->delay, obj->hw->gpt_num, obj->hw->gpt_period, 
               obj->hw->acc_cnt, obj->hw->inact_cnt, obj->hw->act_cnt, obj->hw->step);
    } else {
        len += snprintf(buf+len, PAGE_SIZE-len, "CUST: NULL\n");
    }
    return len;    
}
#endif

/*----------------------------------------------------------------------------*/
static DEVICE_ATTR(config,     S_IWUGO | S_IRUGO, jbd_show_config,  jbd_store_config);
static DEVICE_ATTR(count,      S_IWUGO | S_IRUGO, jbd_show_count,   jbd_store_count);
static DEVICE_ATTR(trace,      S_IWUGO | S_IRUGO, jbd_show_trace,   jbd_store_trace);
static DEVICE_ATTR(eint,       S_IWUGO | S_IRUGO, jbd_show_eint,    jbd_store_eint);
static DEVICE_ATTR(cls,        S_IWUGO | S_IRUGO, jbd_show_cls,     jbd_store_cls);
static DEVICE_ATTR(gain,       S_IWUGO | S_IRUGO, jbd_show_gain,    jbd_store_gain);
//static DEVICE_ATTR(status,               S_IRUGO, jbd_show_status,  NULL);
/*----------------------------------------------------------------------------*/
static struct device_attribute *jbd_attr_list[] = {
    &dev_attr_config,
    &dev_attr_count,    
    &dev_attr_trace,
    &dev_attr_eint,
    &dev_attr_cls,
    &dev_attr_gain,
};
/*----------------------------------------------------------------------------*/
static int jbd_create_attr(struct device *dev) 
{
    int idx, err = 0;
    int num = (int)(sizeof(jbd_attr_list)/sizeof(jbd_attr_list[0]));
    if (!dev)
        return -EINVAL;

    for (idx = 0; idx < num; idx++) {
        if ((err = device_create_file(dev, jbd_attr_list[idx]))) 
            break;
    }
    
    return err;
}
/*----------------------------------------------------------------------------*/
static int jbd_delete_attr(struct device *dev)
{
    int idx ,err = 0;
    int num = (int)(sizeof(jbd_attr_list)/sizeof(jbd_attr_list[0]));
    
    if (!dev)
        return -EINVAL;

    for (idx = 0; idx < num; idx++) 
        device_remove_file(dev, jbd_attr_list[idx]);

    return err;
}

/******************************************************************************
 *     Keyboard class: EINT implementation
*******************************************************************************/
void jbd_kb_eint_up(void)
{
    struct jbd_device *jb = g_jbd_ptr;
    if (!jb)
        return;
    set_bit(JBD_DIR_UP, &jb->pending);
    tasklet_schedule(&jb->eint_key);
    if (atomic_read(&jb->trace) & JBD_TRC_EINT)
        JBD_MSG("eint: up (0x%04X)\n", atomic_read(&jb->dir[JBD_DIR_UP].polarity));
}
/*----------------------------------------------------------------------------*/
void jbd_kb_eint_lf(void)
{
    struct jbd_device *jb = g_jbd_ptr;
    if (!jb)
        return;
    set_bit(JBD_DIR_LF, &jb->pending);
    tasklet_schedule(&jb->eint_key);
    if (atomic_read(&jb->trace) & JBD_TRC_EINT)
        JBD_MSG("eint: lf (0x%04X)\n", atomic_read(&jb->dir[JBD_DIR_LF].polarity));    
}
/*----------------------------------------------------------------------------*/
void jbd_kb_eint_rg(void)
{
    struct jbd_device *jb = g_jbd_ptr;
    if (!jb)
        return;   
    
    set_bit(JBD_DIR_RG, &jb->pending);
    tasklet_schedule(&jb->eint_key);
    if (atomic_read(&jb->trace) & JBD_TRC_EINT)
        JBD_MSG("eint: rg (0x%04X)\n", atomic_read(&jb->dir[JBD_DIR_RG].polarity));    
}
/*----------------------------------------------------------------------------*/
void jbd_kb_eint_dn(void)
{
    struct jbd_device *jb = g_jbd_ptr;
    if (!jb)
        return;

    set_bit(JBD_DIR_DN, &jb->pending);
    tasklet_schedule(&jb->eint_key);
    if (atomic_read(&jb->trace) & JBD_TRC_EINT)
        JBD_MSG("eint: dn (0x%04X)\n", atomic_read(&jb->dir[JBD_DIR_DN].polarity));
}
/*----------------------------------------------------------------------------*/
struct jbd_eint_hlr kb_eint_hlrs = {
    jbd_kb_eint_up,
    jbd_kb_eint_lf,
    jbd_kb_eint_rg,
    jbd_kb_eint_dn,
};

/*----------------------------------------------------------------------------*/
int jbd_setup_eint(struct jbd_device *jb, struct jbd_eint_hlr *hlr)
{
    /*configure to GPIO function, external interrupt*/
    mt_set_gpio_dir(GPIO_JBD_INPUT_UP_PIN, GPIO_DIR_IN);
    mt_set_gpio_dir(GPIO_JBD_INPUT_LEFT_PIN, GPIO_DIR_IN);
    mt_set_gpio_dir(GPIO_JBD_INPUT_RIGHT_PIN, GPIO_DIR_IN);
    mt_set_gpio_dir(GPIO_JBD_INPUT_DOWN_PIN, GPIO_DIR_IN);
    mt_set_gpio_mode(GPIO_JBD_INPUT_UP_PIN, GPIO_JBD_INPUT_UP_PIN_M_EINT);
    mt_set_gpio_mode(GPIO_JBD_INPUT_LEFT_PIN, GPIO_JBD_INPUT_LEFT_PIN_M_EINT);
    mt_set_gpio_mode(GPIO_JBD_INPUT_RIGHT_PIN, GPIO_JBD_INPUT_RIGHT_PIN_M_EINT);
    mt_set_gpio_mode(GPIO_JBD_INPUT_DOWN_PIN, GPIO_JBD_INPUT_DOWN_PIN_M_EINT);
    mt_set_gpio_pull_enable(GPIO_JBD_INPUT_UP_PIN, TRUE);
    mt_set_gpio_pull_select(GPIO_JBD_INPUT_UP_PIN, GPIO_PULL_DOWN);
    mt_set_gpio_pull_enable(GPIO_JBD_INPUT_LEFT_PIN, TRUE);
    mt_set_gpio_pull_select(GPIO_JBD_INPUT_LEFT_PIN, GPIO_PULL_DOWN);
    mt_set_gpio_pull_enable(GPIO_JBD_INPUT_RIGHT_PIN, TRUE);
    mt_set_gpio_pull_select(GPIO_JBD_INPUT_RIGHT_PIN, GPIO_PULL_DOWN);
    mt_set_gpio_pull_enable(GPIO_JBD_INPUT_DOWN_PIN, TRUE);
    mt_set_gpio_pull_select(GPIO_JBD_INPUT_DOWN_PIN, GPIO_PULL_DOWN);

    /*mask interrupt before setting*/
    mt65xx_eint_mask(EINT_JBD_UP_NUM);
    mt65xx_eint_mask(EINT_JBD_LF_NUM);
    mt65xx_eint_mask(EINT_JBD_RG_NUM);
    mt65xx_eint_mask(EINT_JBD_DN_NUM); 

    atomic_set(&jb->dir[JBD_DIR_UP].polarity, EINT_JBD_UP_POLARITY);
    jb->dir[JBD_DIR_UP].eint_no = EINT_JBD_UP_NUM;
    mt65xx_eint_set_sens(EINT_JBD_UP_NUM, EINT_JBD_UP_SENSITIVE);
    mt65xx_eint_set_polarity(EINT_JBD_UP_NUM, EINT_JBD_UP_POLARITY);
    mt65xx_eint_set_hw_debounce(EINT_JBD_UP_NUM, EINT_JBD_UP_DEBOUNCE_CN);   /*the actual debounce time will be multiplied as 32*/
    mt65xx_eint_registration(EINT_JBD_UP_NUM, EINT_JBD_UP_DEBOUNCE_EN, EINT_JBD_UP_POLARITY, hlr->eint_up, 0);

    atomic_set(&jb->dir[JBD_DIR_LF].polarity, EINT_JBD_LF_POLARITY);
    jb->dir[JBD_DIR_LF].eint_no = EINT_JBD_LF_NUM;
    mt65xx_eint_set_sens(EINT_JBD_LF_NUM, EINT_JBD_UP_SENSITIVE);
    mt65xx_eint_set_polarity(EINT_JBD_LF_NUM, EINT_JBD_LF_POLARITY);
    mt65xx_eint_set_hw_debounce(EINT_JBD_LF_NUM, EINT_JBD_LF_DEBOUNCE_CN);   /*the actual debounce time will be multiplied as 32*/
    mt65xx_eint_registration(EINT_JBD_LF_NUM, EINT_JBD_LF_DEBOUNCE_EN, EINT_JBD_LF_POLARITY, hlr->eint_lf, 0);

    atomic_set(&jb->dir[JBD_DIR_RG].polarity, EINT_JBD_RG_POLARITY);
    jb->dir[JBD_DIR_RG].eint_no = EINT_JBD_RG_NUM;
    mt65xx_eint_set_sens(EINT_JBD_RG_NUM, EINT_JBD_RG_SENSITIVE);
    mt65xx_eint_set_polarity(EINT_JBD_RG_NUM, EINT_JBD_RG_POLARITY);
    mt65xx_eint_set_hw_debounce(EINT_JBD_RG_NUM, EINT_JBD_LF_DEBOUNCE_CN);   /*the actual debounce time will be multiplied as 32*/
    mt65xx_eint_registration(EINT_JBD_RG_NUM, EINT_JBD_RG_DEBOUNCE_EN, EINT_JBD_RG_POLARITY, hlr->eint_rg, 0);

    atomic_set(&jb->dir[JBD_DIR_DN].polarity, EINT_JBD_DN_POLARITY);
    jb->dir[JBD_DIR_DN].eint_no = EINT_JBD_DN_NUM;
    mt65xx_eint_set_sens(EINT_JBD_DN_NUM, EINT_JBD_DN_SENSITIVE);
    mt65xx_eint_set_polarity(EINT_JBD_DN_NUM, EINT_JBD_DN_POLARITY);
    mt65xx_eint_set_hw_debounce(EINT_JBD_DN_NUM, EINT_JBD_DN_DEBOUNCE_CN);   /*the actual debounce time will be multiplied as 32*/
    mt65xx_eint_registration(EINT_JBD_DN_NUM, EINT_JBD_DN_DEBOUNCE_EN, EINT_JBD_DN_POLARITY, hlr->eint_dn, 0);

    mt65xx_eint_unmask(EINT_JBD_UP_NUM);
    mt65xx_eint_unmask(EINT_JBD_LF_NUM);
    mt65xx_eint_unmask(EINT_JBD_RG_NUM);
    mt65xx_eint_unmask(EINT_JBD_DN_NUM); 
    return 0;
}
/*----------------------------------------------------------------------------*/
int64_t jbd_get_timecount(void)
{
    struct timeval tv;
    int64_t count;

    do_gettimeofday(&tv);
    count = tv.tv_sec*1000 + tv.tv_usec/1000; /*convert to millisecond*/
    return count;
}
/*----------------------------------------------------------------------------*/
#if 0
static void jbd_eint_key(unsigned long data)
{
    struct jbd_device *jb = (struct jbd_device *)data;
    struct jbd_dir *dir;
    int idx, step = atomic_read(&jb->step);
    int64_t timecnt = jbd_get_timecount(); 
    int64_t diff;
    int flag = 0, report = 0, condition = 0;
    int len = 0;
    char buf[512];
    int trc = atomic_read(&jb->trace);
    u_long mask;

    
    if (trc & JBD_TRC_TASKLET)
        len += snprintf(buf + len, sizeof(buf)-len, "(");
    for (idx = 0; idx < JBD_DIR_NUM; idx++) {
        dir = &jb->dir[idx];
        if ((timecnt - dir->first_acc) >= atomic_read(&jb->acc_cnt)) {
            atomic_set(&dir->cnt, 0);
            dir->first_acc = 0;
        }        
        diff = timecnt - dir->last_trig;
        if (trc & JBD_TRC_TASKLET) 
            len += snprintf(buf + len, sizeof(buf)-len, "%8lld ", diff);
        if (diff >= atomic_read(&jb->inact_cnt))
            atomic_set(&dir->cnt, 0);
        if (!test_and_clear_bit(idx, &jb->pending))
            continue;

        dir->last_trig = timecnt;
        if (diff >= atomic_read(&jb->act_cnt)) {
            atomic_inc(&dir->cnt);
            if (atomic_read(&dir->cnt) == 1)
                dir->first_acc = timecnt;
        }
        if (atomic_read(&dir->cnt) > step) {
            input_report_key(jb->dev, dir->key_evt, 1);
            kpd_backlight_handler(1, dir->key_evt);
            input_report_key(jb->dev, dir->key_evt, 0);
            kpd_backlight_handler(0, dir->key_evt);
            input_sync(jb->dev);
            atomic_set(&dir->cnt, 0);
            report |= (1 << idx);
        }

        /*set up polarity and enable corresponding interrupt again*/
        if (atomic_read(&dir->polarity) == MT65XX_EINT_POL_NEG)
            atomic_set(&dir->polarity, MT65XX_EINT_POL_POS);
        else
            atomic_set(&dir->polarity, MT65XX_EINT_POL_NEG);        
        mt65xx_eint_set_polarity(dir->eint_no, atomic_read(&dir->polarity));
        mt65xx_eint_unmask(dir->eint_no);
        flag |= (1 << idx);
    }
    
    /*eliminate the opposite direction*/
    mask = report & (JBD_BIT_UP|JBD_BIT_DN);
    if (mask) {
        if (mask == JBD_BIT_UP) {
            atomic_set(&jb->dir[JBD_DIR_DN].cnt, 0);
            condition = 0x10;
        } else if (mask == JBD_BIT_DN) {
            atomic_set(&jb->dir[JBD_DIR_UP].cnt, 0);
            condition = 0x20;
        } else if (atomic_read(&jb->dir[JBD_DIR_UP].cnt) > atomic_read(&jb->dir[JBD_DIR_DN].cnt)) {
            report &= (~JBD_BIT_DN);
            atomic_set(&jb->dir[JBD_DIR_DN].cnt, 0);
            condition = 0x30;
        } else if (atomic_read(&jb->dir[JBD_DIR_UP].cnt) < atomic_read(&jb->dir[JBD_DIR_DN].cnt)) {
            report &= (~JBD_BIT_UP);
            atomic_set(&jb->dir[JBD_DIR_UP].cnt, 0);
            condition = 0x40;
        }
    }
    mask = report & (JBD_BIT_LF|JBD_BIT_RG);
    if (mask) {
        if (mask == JBD_BIT_LF) {
            atomic_set(&jb->dir[JBD_DIR_RG].cnt, 0);
            condition |= 0x01;
        } else if (mask == JBD_BIT_RG) {
            atomic_set(&jb->dir[JBD_DIR_LF].cnt, 0);
            condition |= 0x02;            
        } else if (atomic_read(&jb->dir[JBD_DIR_LF].cnt) > atomic_read(&jb->dir[JBD_DIR_RG].cnt)) {
            report &= (~JBD_BIT_RG);
            atomic_set(&jb->dir[JBD_DIR_RG].cnt, 0);
            condition |= 0x03;            
        } else if (atomic_read(&jb->dir[JBD_DIR_LF].cnt) < atomic_read(&jb->dir[JBD_DIR_RG].cnt)) {
            report &= (~JBD_BIT_LF);
            atomic_set(&jb->dir[JBD_DIR_LF].cnt, 0);
            condition |= 0x04;            
        }
    }

    
    if (trc & JBD_TRC_TASKLET) {
        char ptr[8];
        memset(ptr, ' ', sizeof(ptr));
        if (report & JBD_BIT_UP)
            ptr[0] = 'U';
        if (report & JBD_BIT_DN)
            ptr[2] = 'D';
        if (report & JBD_BIT_LF)
            ptr[4] = 'L';
        if (report & JBD_BIT_RG)
            ptr[6] = 'R';
        ptr[7] = 0;
        len = snprintf(buf + len, sizeof(buf)-len, ") (%2d %2d %2d %2d) (%02X, %02X) %s\n", 
                       atomic_read(&jb->dir[JBD_DIR_UP].cnt), atomic_read(&jb->dir[JBD_DIR_DN].cnt),
                       atomic_read(&jb->dir[JBD_DIR_LF].cnt), atomic_read(&jb->dir[JBD_DIR_RG].cnt),
                       flag, condition, ptr);
        JBD_DBG("(%4lld %4llu %4llu %4llu) ", 
                (jb->dir[JBD_DIR_UP].first_acc) ? (timecnt - jb->dir[JBD_DIR_UP].first_acc) : (0), 
                (jb->dir[JBD_DIR_DN].first_acc) ? (timecnt - jb->dir[JBD_DIR_DN].first_acc) : (0), 
                (jb->dir[JBD_DIR_LF].first_acc) ? (timecnt - jb->dir[JBD_DIR_LF].first_acc) : (0), 
                (jb->dir[JBD_DIR_RG].first_acc) ? (timecnt - jb->dir[JBD_DIR_RG].first_acc) : (0));
        JBD_DBG("%s", buf);
    }
}
#endif

/******************************************************************************
 *     Trackball class: Poll implementation
*******************************************************************************/
/*sample implementation: four continous GPIO is used, less effort to read data
  in this method*/
#if 0 
void jbd_gpt_isr(u16 unused)
{    
    #define GPIO_DIN7           (GPIO_BASE + 0x0560)
    #define GPIO96 0x0001
    #define GPIO97 0x0002
    #define GPIO98 0x0004
    #define GPIO99 0x0008

    struct jbd_device *jb = g_jbd_ptr;
    u16 old_val, reg_val = __raw_readw(GPIO_DIN7);    
    if (!jb)
        return ;

    old_val = atomic_read(&jb->status);
    if((old_val & GPIO96) != (reg_val & GPIO96))
        atomic_inc(&jb->lf_cnt);

    if((old_val & GPIO97) != (reg_val & GPIO97))
        atomic_inc(&jb->dn_cnt);
               
    if((old_val & GPIO98) != (reg_val & GPIO98))
        atomic_inc(&jb->rg_cnt);    
                         
   if((old_val & GPIO99) != (reg_val & GPIO99))
        atomic_inc(&jb->up_cnt);  

   atomic_set(&jb->status, reg_val);
}

/*----------------------------------------------------------------------------*/
static void jbd_timer_fn(unsigned long arg) 
{
    struct jbd_device *jb = (struct jbd_device*)arg;
    if (jb && (jb->eint_key.state != TASKLET_STATE_RUN))
        tasklet_hi_schedule(&(jb->poll_key));
}
/*----------------------------------------------------------------------------*/
void jbd_gpt_isr(u16 unused) 
{
    struct jbd_device *jb = g_jbd_ptr;
    u16 up_val, lf_val, rg_val, dn_val;
    
    if (!jb)
        return;
    
    up_val = mt_get_gpio_in(GPIO_JBD_INPUT_UP_PIN);
    lf_val = mt_get_gpio_in(GPIO_JBD_INPUT_LEFT_PIN);
    rg_val = mt_get_gpio_in(GPIO_JBD_INPUT_RIGHT_PIN);
    dn_val = mt_get_gpio_in(GPIO_JBD_INPUT_DOWN_PIN);

    if (up_val != atomic_read(&jb->dir[JBD_DIR_UP].val)) {
        atomic_inc(&jb->dir[JBD_DIR_UP].cnt);
        set_bit(JBD_DIR_UP, &jb->pending);
        atomic_set(&jb->dir[JBD_DIR_UP].val, up_val);
    }
    if (lf_val != atomic_read(&jb->dir[JBD_DIR_LF].val)) {
        atomic_inc(&jb->dir[JBD_DIR_LF].cnt);
        set_bit(JBD_DIR_LF, &jb->pending);
        atomic_set(&jb->dir[JBD_DIR_LF].val, lf_val);
    }
    if (rg_val != atomic_read(&jb->dir[JBD_DIR_RG].val)) {
        atomic_inc(&jb->dir[JBD_DIR_RG].cnt);
        set_bit(JBD_DIR_RG, &jb->pending);
        atomic_set(&jb->dir[JBD_DIR_RG].val, rg_val);
    }
    if (dn_val != atomic_read(&jb->dir[JBD_DIR_DN].val)) {
        atomic_inc(&jb->dir[JBD_DIR_DN].cnt);
        set_bit(JBD_DIR_DN, &jb->pending);
        atomic_set(&jb->dir[JBD_DIR_DN].val, dn_val);
    }    
}
/*----------------------------------------------------------------------------*/
int jbd_XGPTConfig(struct jbd_device *jb)
{
    XGPT_CONFIG config;
    XGPT_NUM  gpt_num = jb->hw->gpt_num;    
    XGPT_CLK_DIV clkDiv = XGPT_CLK_DIV_1;

    XGPT_Reset(gpt_num);   
    XGPT_Init (gpt_num, jbd_gpt_isr);
    config.num = gpt_num;
    config.mode = XGPT_REPEAT;
    config.clkDiv = clkDiv;
    config.bIrqEnable = TRUE;
    config.u4Compare = jb->hw->gpt_period;
    
    if (XGPT_Config(config) == FALSE)
        return -1;                    
        
    XGPT_Start(gpt_num);           
	return 0;
}
/*----------------------------------------------------------------------------*/
int jbd_setup_poll(struct jbd_device *jb)
{
    int err;

    /* init timer */
    init_timer(&(jb->timer));
    jb->timer.function = jbd_timer_fn;
    jb->timer.data = (unsigned long)jb;
    mod_timer(&(jb->timer),jiffies+ atomic_read(&jb->delay)/(1000/HZ));    

    /*configure to GPIO function, input*/
    mt_set_gpio_dir(GPIO_JBD_INPUT_UP_PIN, GPIO_DIR_IN);
    mt_set_gpio_dir(GPIO_JBD_INPUT_LEFT_PIN, GPIO_DIR_IN);
    mt_set_gpio_dir(GPIO_JBD_INPUT_RIGHT_PIN, GPIO_DIR_IN);
    mt_set_gpio_dir(GPIO_JBD_INPUT_DOWN_PIN, GPIO_DIR_IN);
    mt_set_gpio_pull_enable(GPIO_JBD_INPUT_UP_PIN, FALSE);
    mt_set_gpio_pull_enable(GPIO_JBD_INPUT_LEFT_PIN, FALSE);
    mt_set_gpio_pull_enable(GPIO_JBD_INPUT_RIGHT_PIN, FALSE);
    mt_set_gpio_pull_enable(GPIO_JBD_INPUT_DOWN_PIN, FALSE);
    mt_set_gpio_mode(GPIO_JBD_INPUT_UP_PIN, GPIO_JBD_INPUT_UP_PIN_M_GPIO);
    mt_set_gpio_mode(GPIO_JBD_INPUT_LEFT_PIN, GPIO_JBD_INPUT_LEFT_PIN_M_GPIO);
    mt_set_gpio_mode(GPIO_JBD_INPUT_RIGHT_PIN, GPIO_JBD_INPUT_RIGHT_PIN_M_GPIO);
    mt_set_gpio_mode(GPIO_JBD_INPUT_DOWN_PIN, GPIO_JBD_INPUT_DOWN_PIN_M_GPIO);
    
    if ((err = jbd_XGPTConfig(jb)))
        return err;
    return 0;
}
/*----------------------------------------------------------------------------*/
static void jbd_poll_key(unsigned long data) 
{
    struct jbd_device *jb = (struct jbd_device*)data;
    struct jbd_dir *dir;
    int idx, step = atomic_read(&jb->step);
    int64_t diff, timecnt = jbd_get_timecount();     
    int pending, report = 0;
    int len = 0;
    char buf[512];    
    char *info;

    pending = jb->pending;
    for (idx = 0; idx < JBD_DIR_NUM; idx++) {
        dir = &jb->dir[idx];

        diff = timecnt - dir->last_trig;
        if (diff >= atomic_read(&jb->inact_cnt))
            atomic_set(&dir->cnt, 0);
        if (!test_and_clear_bit(idx, &jb->pending))
            continue;

        if (atomic_read(&jb->trace) & JBD_TRC_TASKLET) 
            len += snprintf(buf + len, sizeof(buf)-len, "(%d) diff = %16lld, cnt = %d\n", idx, diff, atomic_read(&dir->cnt));        
        dir->last_trig = timecnt;
        if (atomic_read(&dir->cnt) >= step) {
            input_report_key(jb->dev, dir->key_evt, 1);
            kpd_backlight_handler(1, dir->key_evt);
            input_report_key(jb->dev, dir->key_evt, 0);
            kpd_backlight_handler(0, dir->key_evt);
            input_sync(jb->dev);
            atomic_set(&dir->cnt, 0);
            report = (1 << idx);
        }
    }
    if (pending && (atomic_read(&jb->trace) & JBD_TRC_TASKLET)) { 
        info = (report) ? (" ********* ") : ("");
        len = snprintf(buf + len, sizeof(buf)-len, "tasklet (0x%04X) %s\n", report, info);        
        JBD_DBG("%s", buf);
    }

    mod_timer(&(jb->timer),jiffies + atomic_read(&jb->delay)/(1000/HZ));         
    return;    
}
/******************************************************************************
 *     Keyboard class: Common Function
*******************************************************************************/
int jbd_setup_class_key(struct jbd_device *jb)
{
    g_jbd_ptr = jb;    
    if (atomic_read(&jb->detect) == JBD_DETECT_POLL) 
        return jbd_setup_poll(jb);
    else if (atomic_read(&jb->detect) == JBD_DETECT_EINT) 
        return jbd_setup_eint(jb, &kb_eint_hlrs);
    return -EINVAL;
}
#endif
/******************************************************************************
 *     Trackball class: EINT implementation
*******************************************************************************/
static bool jbd_tb_queue_full(struct jbd_device *jb)
{
    struct eint_queue *obj = &jb->queue;
    if (atomic_read(&obj->head) == -1) {
        if (0 == ((atomic_read(&obj->tail)+1) % obj->size))
            return true;
    } else if (atomic_read(&obj->head) == ((atomic_read(&obj->tail)+1) % obj->size)) {
        return true;
    }
    return false;
}
/*----------------------------------------------------------------------------*/
static bool jbd_tb_queue_empty(struct jbd_device *jb)
{
    struct eint_queue *obj = &jb->queue;
    if (((atomic_read(&obj->head)+1)%obj->size) == atomic_read(&obj->tail))
        return true;
    else 
        return false;
}
/*----------------------------------------------------------------------------*/
static bool jbd_tb_queue_push(struct jbd_device *jb, int event)
{
    struct eint_queue *obj = &jb->queue;
    int trc = atomic_read(&jb->trace);
    if (!jbd_tb_queue_full(jb)) {
        int tail = atomic_read(&obj->tail);
        atomic_set(&obj->data[tail], event);
        atomic_set(&obj->tail, (tail+1)%obj->size);
        if (trc & JBD_TRC_QUEUE)
            JBD_DBG("push[%2d]=%2d\n", atomic_read(&obj->tail)-1, event);
        return true;
    } else {
        JBD_ERR("queue full: %d %d\n", atomic_read(&obj->head), atomic_read(&obj->tail));
        return false;
    }
}
/*----------------------------------------------------------------------------*/
static bool jbd_tb_queue_pop(struct jbd_device *jb, int *event)
{
    struct eint_queue *obj = &jb->queue;
    int trc = atomic_read(&jb->trace);
    if (!jbd_tb_queue_empty(jb)) {
        int head = (atomic_read(&obj->head)+1)%obj->size;
        atomic_set(&obj->head, head);
        *event = atomic_read(&obj->data[head]);
        if (trc & JBD_TRC_QUEUE)
            JBD_DBG("pop [%2d]=%2d\n", atomic_read(&obj->head), *event);        
        return true;
    } else {
        /*empty*/
        return false;
    }
}
/*----------------------------------------------------------------------------*/
void jbd_finish_eint(struct jbd_dir *dir)
{
    if (atomic_read(&dir->polarity) == MT65XX_EINT_POL_NEG)
        atomic_set(&dir->polarity, MT65XX_EINT_POL_POS);
    else
        atomic_set(&dir->polarity, MT65XX_EINT_POL_NEG);        
    mt65xx_eint_set_polarity(dir->eint_no, atomic_read(&dir->polarity));
    mt65xx_eint_unmask(dir->eint_no); 
}
/*----------------------------------------------------------------------------*/
void jbd_tb_eint_up(void)
{
    struct jbd_device *jb = g_jbd_ptr;
    struct jbd_dir *dir;

    do_gettimeofday(&t1); 	
    if (!jb)
        return;
    dir = &jb->dir[JBD_DIR_UP];    
    if (!jbd_tb_queue_push(jb, JBD_DIR_UP))    
        JBD_ERR("push fail\n");
        
    if (atomic_read(&dir->polarity) == MT65XX_EINT_POL_NEG)
    {
			mt_set_gpio_pull_select(GPIO_JBD_INPUT_UP_PIN, GPIO_PULL_DOWN);
    }
    else
    {
			mt_set_gpio_pull_select(GPIO_JBD_INPUT_UP_PIN, GPIO_PULL_UP); 
    }	  
    jbd_finish_eint(dir);
               
    if (atomic_read(&jb->trace) & JBD_TRC_EINT)
        JBD_MSG("eint: up (0x%04X)\n", atomic_read(&jb->dir[JBD_DIR_UP].polarity));
        
    tasklet_schedule(&jb->eint_ball);        
}
/*----------------------------------------------------------------------------*/
void jbd_tb_eint_lf(void)
{
    struct jbd_device *jb = g_jbd_ptr;
    struct jbd_dir *dir;

    do_gettimeofday(&t1); 	
    if (!jb)
        return;
    dir = &jb->dir[JBD_DIR_LF];    
    if (!jbd_tb_queue_push(jb, JBD_DIR_LF))    
        JBD_ERR("push fail\n");
        
    if (atomic_read(&dir->polarity) == MT65XX_EINT_POL_NEG)
    {
			mt_set_gpio_pull_select(GPIO_JBD_INPUT_LEFT_PIN, GPIO_PULL_DOWN);
    }
    else
    {
			mt_set_gpio_pull_select(GPIO_JBD_INPUT_LEFT_PIN, GPIO_PULL_UP); 
    }	        
    jbd_finish_eint(dir);
    
    if (atomic_read(&jb->trace) & JBD_TRC_EINT)
        JBD_MSG("eint: lf (0x%04X)\n", atomic_read(&jb->dir[JBD_DIR_LF].polarity));    
    tasklet_schedule(&jb->eint_ball);           
}
/*----------------------------------------------------------------------------*/
void jbd_tb_eint_rg(void)
{
    struct jbd_device *jb = g_jbd_ptr;
    struct jbd_dir *dir;

    do_gettimeofday(&t1); 	
    if (!jb)
        return;
    dir = &jb->dir[JBD_DIR_RG];    
    if (!jbd_tb_queue_push(jb, JBD_DIR_RG))    
        JBD_ERR("push fail\n");
        
    if (atomic_read(&dir->polarity) == MT65XX_EINT_POL_NEG)
    {
				mt_set_gpio_pull_select(GPIO_JBD_INPUT_RIGHT_PIN, GPIO_PULL_DOWN); 
    }
    else
    {
			mt_set_gpio_pull_select(GPIO_JBD_INPUT_RIGHT_PIN, GPIO_PULL_UP);
    }	       
    jbd_finish_eint(dir);
    
    if (atomic_read(&jb->trace) & JBD_TRC_EINT)
        JBD_MSG("eint: rg (0x%04X)\n", atomic_read(&jb->dir[JBD_DIR_RG].polarity));    
    tasklet_schedule(&jb->eint_ball);  
}
/*----------------------------------------------------------------------------*/
void jbd_tb_eint_dn(void)
{
    struct jbd_device *jb = g_jbd_ptr;
    struct jbd_dir *dir;

    do_gettimeofday(&t1); 	
    if (!jb)
        return;
    dir = &jb->dir[JBD_DIR_DN];    
    if (!jbd_tb_queue_push(jb, JBD_DIR_DN))    
        JBD_ERR("push fail\n");

    if (atomic_read(&dir->polarity) == MT65XX_EINT_POL_NEG)
    {
			mt_set_gpio_pull_select(GPIO_JBD_INPUT_DOWN_PIN, GPIO_PULL_DOWN); 
    }
    else
    {
			mt_set_gpio_pull_select(GPIO_JBD_INPUT_DOWN_PIN, GPIO_PULL_UP); 
    }	        
    jbd_finish_eint(dir);
    
    if (atomic_read(&jb->trace) & JBD_TRC_EINT)
        JBD_MSG("eint: dn (0x%04X)\n", atomic_read(&jb->dir[JBD_DIR_DN].polarity));
    tasklet_schedule(&jb->eint_ball);          
}
/*----------------------------------------------------------------------------*/
struct jbd_eint_hlr tb_eint_hlrs = {
    jbd_tb_eint_up,
    jbd_tb_eint_lf,
    jbd_tb_eint_rg,
    jbd_tb_eint_dn,
};
/*----------------------------------------------------------------------------*/
static void jbd_eint_ball(unsigned long data)
{
    struct jbd_device *jb = (struct jbd_device *)data;
    struct jbd_dir *dir;
    int trc = atomic_read(&jb->trace);
    int dx, dy, gain_x = atomic_read(&jb->gain_x), gain_y = atomic_read(&jb->gain_y);
    int event;

    
    while (jbd_tb_queue_pop(jb, &event)) {
        dx = dy = 0;
        dir = &jb->dir[event];
        dx += gain_x * dir->step_x;
        dy += gain_y * dir->step_y;
        input_report_rel(jb->dev, REL_X, dx);
        input_report_rel(jb->dev, REL_Y, dy);
        input_sync(jb->dev);
				do_gettimeofday(&t2);         
        if (trc & JBD_TRC_TASKLET)
        {	
            JBD_DBG("(%+4d, %+4d), time: %ld(ms) \n", dx, dy, (t2.tv_sec*1000+t2.tv_usec/1000)-(t1.tv_sec*1000+t1.tv_usec/1000));//-(jb_basetime.tv_sec*1000+jb_basetime.tv_usec/1000));
        }
    }
}
/******************************************************************************
 *     Trackball class: Common Function
*******************************************************************************/
int jbd_setup_class_ball(struct jbd_device *jb)
{
    g_jbd_ptr = jb;    
    return jbd_setup_eint(jb, &tb_eint_hlrs);
}
/*----------------------------------------------------------------------------*/
/* timer keep polling Jog Ball status                                         */
/*----------------------------------------------------------------------------*/
#if !defined(CONFIG_HAS_EARLYSUSPEND)
/*----------------------------------------------------------------------------*/
static int jbd_suspend(struct platform_device *pdev, pm_message_t state) 
{
    JBD_FUN();
    mt65xx_eint_mask(EINT_JBD_UP_NUM);
    mt65xx_eint_mask(EINT_JBD_DN_NUM);
    mt65xx_eint_mask(EINT_JBD_LF_NUM);
    mt65xx_eint_mask(EINT_JBD_RG_NUM);
    return 0;
}
/*----------------------------------------------------------------------------*/
static int jbd_resume(struct platform_device *pdev)
{
    JBD_FUN();
    mt65xx_eint_unmask(EINT_JBD_UP_NUM);
    mt65xx_eint_unmask(EINT_JBD_DN_NUM);
    mt65xx_eint_unmask(EINT_JBD_LF_NUM);
    mt65xx_eint_unmask(EINT_JBD_RG_NUM);
    if (obj->hw->report_cls == JBD_CLASS_KEYBOARD)
        jbd_setup_eint(obj, &kb_eint_hlrs);
    else 
        jbd_setup_eint(obj, &tb_eint_hlrs);
    return 0;
}
/*----------------------------------------------------------------------------*/
#else
/*----------------------------------------------------------------------------*/
static void jbd_early_suspend(struct early_suspend *h)
{
    JBD_FUN();
    mt65xx_eint_mask(EINT_JBD_UP_NUM);
    mt65xx_eint_mask(EINT_JBD_DN_NUM);
    mt65xx_eint_mask(EINT_JBD_LF_NUM);
    mt65xx_eint_mask(EINT_JBD_RG_NUM);
}
/*----------------------------------------------------------------------------*/
static void jbd_late_resume(struct early_suspend *h)
{
    struct jbd_device *obj = container_of(h, struct jbd_device, early_drv);

    JBD_FUN();
    mt65xx_eint_unmask(EINT_JBD_UP_NUM);
    mt65xx_eint_unmask(EINT_JBD_DN_NUM);
    mt65xx_eint_unmask(EINT_JBD_LF_NUM);
    mt65xx_eint_unmask(EINT_JBD_RG_NUM);
    if (obj->hw->report_cls == JBD_CLASS_KEYBOARD)
        jbd_setup_eint(obj, &kb_eint_hlrs);
    else 
        jbd_setup_eint(obj, &tb_eint_hlrs);
}
/*----------------------------------------------------------------------------*/
#endif
/*----------------------------------------------------------------------------*/
static int jbd_probe(struct platform_device *pdev) 
{
    struct jbd_device* jb = NULL;
    int idx, err = 0;

    JBD_FUN();
    jb = (struct jbd_device*)kzalloc(sizeof(*jb), GFP_KERNEL);
    if (!jb)
        return -ENOMEM;

    jb->hw = get_cust_jogball_hw();
    if (!jb->hw) {
        err = -EINVAL;
        goto err_get_cust_hw;
    }    /* allocate input device */
    jb->dev = input_allocate_device();
    if (!jb->dev) {
        err = -ENOMEM;
        goto err_input_alloc_device;
    }
  
    /* struct input_dev dev initialization and registration */
    jb->dev->name = JBD_DEVICE;    
#if 0
    if (jb->hw->report_cls == JBD_CLASS_KEYBOARD) {
        /* setup input subsystem: key class*/ 
        input_set_capability(jb->dev, EV_KEY, KEY_UP);
        input_set_capability(jb->dev, EV_KEY, KEY_DOWN);
        input_set_capability(jb->dev, EV_KEY, KEY_LEFT);
        input_set_capability(jb->dev, EV_KEY, KEY_RIGHT);    
    }
#endif	
	{     
        /* setup input subsystem: trackball class*/
        input_set_capability(jb->dev, EV_KEY, BTN_MOUSE);        
        input_set_capability(jb->dev, EV_REL, REL_X);
        input_set_capability(jb->dev, EV_REL, REL_Y);
    }

    /* reigster input device */
    if (input_register_device(jb->dev)) {
        JBD_ERR("input_register_device failed.\n");
        goto err_input_register_device;
    }

    if (jbd_create_attr(&pdev->dev)) {
        JBD_ERR("create attr fail\n");
        goto err_create_attr;
    }
    
    /* init tasklet */
	#if 0
    tasklet_init(&(jb->poll_key), jbd_poll_key, (unsigned long)jb);
    tasklet_init(&(jb->eint_key), jbd_eint_key, (unsigned long)jb);
	#endif
    tasklet_init(&(jb->eint_ball),jbd_eint_ball,(unsigned long)jb);
    platform_set_drvdata(pdev, jb);

    /* init counter */
    for (idx = 0; idx < JBD_DIR_NUM; idx++) {        
        atomic_set(&jb->dir[idx].cnt, 0);
        atomic_set(&jb->dir[idx].val, 0);
        jb->dir[idx].first_acc = 0;
        jb->dir[idx].last_trig = 0;
    }
	#if 0
    /*keyboard class*/
    jb->dir[JBD_DIR_UP].key_evt = KEY_UP;
    jb->dir[JBD_DIR_LF].key_evt = KEY_LEFT;
    jb->dir[JBD_DIR_RG].key_evt = KEY_RIGHT;
    jb->dir[JBD_DIR_DN].key_evt = KEY_DOWN;
    atomic_set(&jb->report_cls, jb->hw->report_cls);
    atomic_set(&jb->delay,  jb->hw->delay);   
    atomic_set(&jb->step,   jb->hw->step);
    atomic_set(&jb->detect, jb->hw->detect);
    atomic_set(&jb->trace,  0);
    atomic_set(&jb->acc_cnt, jb->hw->acc_cnt);
    atomic_set(&jb->inact_cnt, jb->hw->inact_cnt);
    atomic_set(&jb->act_cnt, jb->hw->act_cnt);
    jb->pending = 0;
    jb->pressed = 0;
	#endif
    /*trackball class*/
    atomic_set(&jb->gain_x, jb->hw->gain_x);
    atomic_set(&jb->gain_y, jb->hw->gain_y);
    atomic_set(&jb->queue.head, -1);
    atomic_set(&jb->queue.tail, 0);
    jb->queue.size = sizeof(jb->queue.data)/sizeof(jb->queue.data[0]);
    for (idx = 0; idx < jb->queue.size; idx++)
        atomic_set(&jb->queue.data[idx], 0);

    jb->dir[JBD_DIR_UP].step_y = -1;
    jb->dir[JBD_DIR_DN].step_y = +1;
    jb->dir[JBD_DIR_LF].step_x = -1;
    jb->dir[JBD_DIR_RG].step_x = +1;
    
#if 0
    if (jb->hw->report_cls == JBD_CLASS_KEYBOARD)
        err = jbd_setup_class_key(jb);
#endif//   else
    err = jbd_setup_class_ball(jb);
    if (err)
        JBD_ERR("setup class fail: %d\n", err);

#if defined(CONFIG_HAS_EARLYSUSPEND)
    jb->early_drv.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
    jb->early_drv.suspend = jbd_early_suspend;
    jb->early_drv.resume = jbd_late_resume;
    register_early_suspend(&jb->early_drv);
#endif

#if 0
	/* jogball: GPIO229 Up; GPIO211 Down; GPIO217 Left; GPIO215 Right. input, mode=EINT, pull enbale, pull select high*/
	mt_set_gpio_mode(229, 2);
	mt_set_gpio_dir(229, 0);
	mt_set_gpio_pull_enable(229, 1);
	mt_set_gpio_pull_select(229, 1);

	mt_set_gpio_mode(211, 2);
	mt_set_gpio_dir(211, 0);
	mt_set_gpio_pull_enable(211, 1);
	mt_set_gpio_pull_select(211, 1);

	mt_set_gpio_mode(217, 2);
	mt_set_gpio_dir(217, 0);
	mt_set_gpio_pull_enable(217, 1);
	mt_set_gpio_pull_select(217, 1);

	mt_set_gpio_mode(215, 2);
	mt_set_gpio_dir(215, 0);
	mt_set_gpio_pull_enable(215, 1);
	mt_set_gpio_pull_select(215, 1);
#endif

    JBD_MSG("%s: OK\n", __func__);
    return 0;

err_create_attr:
    input_unregister_device(jb->dev);
err_input_register_device:
    input_free_device(jb->dev);
err_get_cust_hw:    
err_input_alloc_device:
    kfree(jb);
    JBD_ERR("%s: err = %d\n", __func__, err);
    return err;
}
/*----------------------------------------------------------------------------*/
static int jbd_remove(struct platform_device *pdev)
{
    struct jbd_device *jb = (struct jbd_device*)platform_get_drvdata(pdev);
    JBD_FUN();

    if (jb) {        
        input_unregister_device(jb->dev);
        input_free_device(jb->dev);
        jbd_delete_attr(&pdev->dev);
        kfree(jb);
    }
    return 0;        
}
/*----------------------------------------------------------------------------*/
static struct platform_driver jbd_driver =
{
    .remove     = jbd_remove,
    .probe      = jbd_probe,
#if !defined(CONFIG_HAS_EARLYSUSPEND)    
    .suspend    = jbd_suspend,
    .resume     = jbd_resume,
#endif     
    .driver     = {
        .name = JBD_DEVICE,
    },
};
/*----------------------------------------------------------------------------*/
static int __init jbd_device_init(void) 
{
    JBD_FUN();
    if(platform_driver_register(&jbd_driver)!=0) {
        JBD_ERR("unable to register Jog Ball driver.\n");
        return -1;
    }
    return 0;
}
/*----------------------------------------------------------------------------*/
static void __exit jbd_device_exit(void) 
{
    platform_driver_unregister(&jbd_driver);
}
module_init(jbd_device_init);
module_exit(jbd_device_exit);

