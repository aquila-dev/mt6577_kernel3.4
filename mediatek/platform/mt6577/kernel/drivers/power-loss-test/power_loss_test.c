#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/autoconf.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <asm/processor.h>
#include <asm/uaccess.h>

#include "power_loss_test.h"


#define PWR_LOSS_MT6575      
//#define PWR_LOSS_MT6573      

/* use software reset to do power loss test, 
 * if not defined means to use hardware(ATE) 
 * reset */
//#define PWR_LOSS_SW_RESET         

/* CONFIG_** macro will defined in linux .config file */
#ifdef CONFIG_PWR_LOSS_MTK_DEBUG
#define PWR_LOSS_DEBUG
#endif


#define PWR_LOSS_DEVNAME            "power_loss_test"  //name in /proc/devices & sysfs
#define PWR_LOSS_FIRST_MINOR         0
#define PWR_LOSS_MAX_MINOR_COUNT     1
#define PWR_LOSS_CBUF_LEN            32

#ifndef HZ
#define HZ	100
#endif

#ifdef PWR_LOSS_SW_RESET
    #ifdef PWR_LOSS_RANDOM_SW_RESET
        #define PWR_LOSS_SLEEP_MAX_TIME      (60*HZ)    //240second
    #else
        #define PWR_LOSS_SLEEP_TIME          (6000)    //60second   
    #endif /* end of PWR_LOSS_RANDOM_SW_RESET */
#endif /* end of PWR_LOSS_SW_RESET */

#ifdef PWR_LOSS_MT6575
extern void wdt_arch_reset(char mode);
#elif defined PWR_LOSS_MT6573
    #include "../../../core/include/mach/mt6573_reg_base.h"
    #include "../../../core/include/mach/mt6573_typedefs.h"
    #define PWR_LOSS_WDT_MODE               (AP_RGU_BASE+0)    
    #define PWR_LOSS_WDT_LENGTH             (AP_RGU_BASE+4)  
    #define PWR_LOSS_WDT_RESTART            (AP_RGU_BASE+8)
#endif /* end of PWR_LOSS_MT6575 */


static dev_t sg_pwr_loss_devno;
static struct cdev*   sg_pwr_loss_dev       = NULL;
static struct class*  sg_pwr_loss_dev_class = NULL;
static struct device* sg_pwr_loss_dev_file  = NULL;

int pwr_loss_open(struct inode *inode, struct file *file)
{
#ifdef PWR_LOSS_DEBUG
    printk("Power Loss Test: Open operation !\n");
#endif
    return 0;
}

int pwr_loss_release(struct inode *inode, struct file *file)
{
#ifdef PWR_LOSS_DEBUG
    printk("Power Loss Test: Release operation !\n");
#endif
    return 0;
}

int pwr_loss_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    char l_buf[PWR_LOSS_CBUF_LEN] = {0};
   
#ifdef PWR_LOSS_DEBUG
    printk("Power Loss Test: IOCTL operation ! CMD=%d\n", cmd);
#endif
    
    switch (cmd){
        case PRINT_REBOOT_TIMES:
            ret  = copy_from_user((int *)l_buf, (int *)arg, sizeof(int));
            if (ret != 0){
                printk("Power Loss Test: IOCTL->PRINT_REBOOT_TIMES %d Bytes can't be copied \n", ret);
            }
            printk("Power Loss Test: -----------System Reboot Successfully Times= %d---------------!\n", ((int *)l_buf)[0]);
            break;
        case PRINT_DATA_COMPARE_ERR:
            printk("Power Loss Test: -----------Data Compare Error---------------!\n");
            break;
        case PRINT_FILE_OPERATION_ERR:
            printk("Power Loss Test: -----------File Operation Error---------------!\n");
            break;
        case PRINT_GENERAL_INFO:
            ret  = copy_from_user(l_buf,(int *)arg,(sizeof(l_buf[0])*(sizeof(l_buf))));
            if (ret != 0){
                printk("Power Loss Test: IOCTL->PRINT_REBOOT_TIMES %d Bytes can't be copied \n", ret);
            }
            
            l_buf[(sizeof(l_buf[0])*(sizeof(l_buf)))-1] = '\0';

#ifdef PWR_LOSS_DEBUG
            printk("%s", l_buf);
#endif
            break;
        default:
            printk("Power Loss Test: cmd code Error!\n");
            break;
    }
    
    return 0;    
}

static struct file_operations pwr_loss_fops =
{
    .owner = THIS_MODULE,
    .open = pwr_loss_open,
    .release = pwr_loss_release,
    .unlocked_ioctl	= pwr_loss_ioctl,
};

#ifdef PWR_LOSS_SW_RESET
#if PWR_LOSS_RANDOM_SW_RESET
int pwr_loss_reset_thread(void *p)
{    
    signed long ret = 0;
    int HZ_val = HZ;
    struct timespec current_time;
    long sec_time = 0;
    long nsec_time = 0;
    signed long sleep_time = 0;    
   
#ifdef PWR_LOSS_MT6573
    volatile unsigned short *Reg1 = (unsigned short *)PWR_LOSS_WDT_MODE;
    volatile unsigned short *Reg2 = (unsigned short *)PWR_LOSS_WDT_LENGTH;
    volatile unsigned short *Reg3 = (unsigned short *)PWR_LOSS_WDT_RESTART;
#endif 

    current_time = current_kernel_time();
    sec_time = current_time.tv_sec;
    nsec_time = current_time.tv_nsec;

#ifdef PWR_LOSS_DEBUG
    printk("Power Loss Test: current time Second=%d, nSecond=%d \n", sec_time, nsec_time);
#endif

    sleep_time = (sec_time + (nsec_time % 1000000) + ((nsec_time * 3) % 100000))
                  % PWR_LOSS_SLEEP_MAX_TIME;

#ifdef PWR_LOSS_DEBUG
    printk("Power Loss Test: sleep time =%d\n", sleep_time);
#endif
    while (1){
        printk("Power Loss Test: wait for reset...!\n");
        set_current_state(TASK_UNINTERRUPTIBLE);
        ret = schedule_timeout(sleep_time);
        printk("Power Loss Test: ret = %d, do reset now...\n",ret);

#ifdef PWR_LOSS_MT6575
    #ifdef CONFIG_MTK_MTD_NAND
    #endif
        wdt_arch_reset(0xff);
#elif defined PWR_LOSS_MT6573
    #ifdef CONFIG_MTK_MTD_NAND
        if(!mt6573_nandchip_Reset()){
            printk("NAND_MVG mt6573_nandchip_Reset Failed!\n");
        }
    #endif 

        /* reset by watch dog */
        *Reg1 = 0x2200;
        *Reg2 = (0x3F<<5)|0x8;
        *Reg3 = 0x1971;
        *Reg1 = 0x2217;
#endif
        while(1);
    }
}
#else
int pwr_loss_reset_thread(void *p)
{
    signed long ret = 0;
    signed long l_val1 = 0;
    signed long l_val2 = 0;
    signed long l_count = 0;

#ifdef PWR_LOSS_MT6573
    volatile unsigned short *Reg1 = (unsigned short *)PWR_LOSS_WDT_MODE;
    volatile unsigned short *Reg2 = (unsigned short *)PWR_LOSS_WDT_LENGTH;
    volatile unsigned short *Reg3 = (unsigned short *)PWR_LOSS_WDT_RESTART;
#endif 

#ifdef PWR_LOSS_DEBUG
    printk("Power Loss Test: sleep time = 100sec\n");
#endif

    while (1){
        printk("Power Loss Test: wait for reset...!\n");
        set_current_state(TASK_UNINTERRUPTIBLE);
        ret = schedule_timeout(PWR_LOSS_SLEEP_TIME);
        printk("Power Loss Test: ret = %d, do reset now...\n",ret);

#ifdef PWR_LOSS_MT6575
    #ifdef CONFIG_MTK_MTD_NAND
    #endif
        wdt_arch_reset(0xff);
#elif defined  PWR_LOSS_MT6573
    #ifdef CONFIG_MTK_MTD_NAND
        if(!mt6573_nandchip_Reset()){
            printk("NAND_MVG mt6573_nandchip_Reset Failed!\n");
        }
    #endif 

        /* reset by watch dog */
        *Reg1 = 0x2200;
        *Reg2 = (0x3F<<5)|0x8;
        *Reg3 = 0x1971;
        *Reg1 = 0x2217;
#endif
        while(1);
    }
}
#endif /* end of PWR_LOSS_RANDOM_SW_RESET */
#endif /* end of PWR_LOSS_SW_RESET */


static int __init power_loss_init(void)
{
    int err;

    printk("Power Loss Test Module Init\n");

    err = alloc_chrdev_region(&sg_pwr_loss_devno, PWR_LOSS_FIRST_MINOR, PWR_LOSS_MAX_MINOR_COUNT, PWR_LOSS_DEVNAME);
    if (err != 0){
        printk("Power Loss Test: alloc_chardev_region Failed!\n");
        return err;
    }

#ifdef PWR_LOSS_DEBUG
    printk("Power Loss Test: MAJOR =%d, MINOR=%d\n", 
                      MAJOR(sg_pwr_loss_devno), MINOR(sg_pwr_loss_devno));
#endif
    
    sg_pwr_loss_dev = cdev_alloc();
    if (NULL == sg_pwr_loss_dev){
        printk("Power Loss Test: cdev_alloc Failed\n");
        goto out2;
    }

    sg_pwr_loss_dev->owner = THIS_MODULE;
    sg_pwr_loss_dev->ops   = &pwr_loss_fops;

    err = cdev_add(sg_pwr_loss_dev, sg_pwr_loss_devno, 1);
    if (err != 0){
        printk("Power Loss Test: cdev_add Failed!\n");
        goto out2;
    }
    
    sg_pwr_loss_dev_class = class_create(THIS_MODULE, PWR_LOSS_DEVNAME);
    if (NULL == sg_pwr_loss_dev_class){
        printk("Power Loss Test: class_create Failed!\n");
        goto out1;
    }

    sg_pwr_loss_dev_file = device_create(sg_pwr_loss_dev_class, NULL, sg_pwr_loss_devno, NULL, PWR_LOSS_DEVNAME);
    if (NULL == sg_pwr_loss_dev_file){
        printk("Power Loss Test: device_create Failed!\n");
        goto out;
    }

    printk("Power Loss Test: Init Successfully!\n");
    
#ifdef PWR_LOSS_SW_RESET
    kernel_thread(pwr_loss_reset_thread, NULL, CLONE_VM);    //CLONE_KERNEL
    printk("Power Loss Test: kernel thread create Successful!\n");
#endif

    return 0;
out:
    class_destroy(sg_pwr_loss_dev_class);
out1:
    cdev_del(sg_pwr_loss_dev);
out2:
    unregister_chrdev_region(sg_pwr_loss_devno, PWR_LOSS_MAX_MINOR_COUNT);
    
    return err;
}

static void __exit power_loss_exit(void)
{
#ifdef PWR_LOSS_DEBUG
    printk("Power Loss Test: Module Exit\n");
#endif

    unregister_chrdev_region(sg_pwr_loss_devno, PWR_LOSS_MAX_MINOR_COUNT);
    cdev_del(sg_pwr_loss_dev);
    device_destroy(sg_pwr_loss_dev_class, sg_pwr_loss_devno);
    class_destroy(sg_pwr_loss_dev_class);
    
    printk("Power Loss Test:module exit Successfully!\n");
}

module_init(power_loss_init);
module_exit(power_loss_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("feifei.wang <feifei.wang@mediatek.com>");
MODULE_DESCRIPTION(" This module is for power loss test");

