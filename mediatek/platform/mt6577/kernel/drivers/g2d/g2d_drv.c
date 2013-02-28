#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/module.h>
#include <linux/autoconf.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/kdev_t.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/param.h>
#include <linux/uaccess.h>
#include <linux/sched.h>

#include <linux/workqueue.h>
#include <linux/semaphore.h>
#include <linux/slab.h>

#include <asm/io.h>

#include <mach/irqs.h>
#include <mach/mt6577_reg_base.h>
#include <mach/mt6577_irq.h>
#include <mach/mt6577_clock_manager.h>

#include "g2d_drv_api.h"
#include "g2d_drv_fb.h"
#include "g2d_drv.h"
#include "g2d_m4u.h"

typedef struct {
    struct work_struct work;
    struct semaphore lock;
    struct semaphore sync;
    bool sync_cmd;
    g2d_context_t ctx;
} g2d_command_t;

static dev_t g2d_devno;
static struct cdev *g2d_cdev;
static struct class *g2d_class = NULL;

static spinlock_t g2d_cmd_spinlock;

struct workqueue_struct *g2d_workqueue = NULL;
static wait_queue_head_t isr_wait_queue;
g2d_command_t *g2d_cmd_buffer = NULL;
static int g2d_current_cmd = 0;


/*************************************************************************************/

void g2d_drv_power_on(void)
{
    BOOL ret;

    ret = hwEnableClock(MT65XX_PDN_MM_G2D, "G2D");

    NOT_REFERENCED(ret);
}

void g2d_drv_power_off(void)
{
    BOOL ret;

    ret = hwDisableClock(MT65XX_PDN_MM_G2D, "G2D");

    NOT_REFERENCED(ret);
}

/*************************************************************************************/

void _g2d_reg_dump(void)
{
    unsigned int reg_value = 0;
    unsigned int index = 0;

    G2D_INF("********************\n");
    G2D_INF("reg (base : 0x%x):\n", G2D_BASE);

    for (index = 0 ; index < 0x158 ; index += 4)
    {
        reg_value = ioread32(G2D_BASE + index);
        G2D_INF("0x%x (0x%x)\n", index, reg_value);
    }

    G2D_INF("********************\n");
}

/*************************************************************************************/

static irqreturn_t g2d_drv_isr(int irq, void *dev_id)
{
    for(;;)
    {
        if (!g2d_drv_get_status()) break;
        G2D_DBG("handle interrupt, status : %d\n", g2d_drv_get_status());
    }
    wake_up_interruptible(&isr_wait_queue);

    return IRQ_HANDLED;
}

/*************************************************************************************/

void _g2d_command_handler(g2d_command_t *cmd)
{
    G2D_DBG("start to handle command (0x%04x)\n", cmd->ctx.flag);

    switch (cmd->ctx.flag)
    {
    case G2D_FLAG_END_OF_FRAME:
        g2d_drv_fb_queue_buffer(cmd->ctx.fb_id);
        break;

    case G2D_FLAG_FLUSH:
        break;

    case G2D_FLAG_MIRROR_FRONT_FB:
        if (g2d_drv_fb_fill_mirror_cmd(&cmd->ctx) != 0) break;
    default:
        {
            g2d_drv_power_on();

            g2d_drv_run(&cmd->ctx);

            wait_event_interruptible(isr_wait_queue, !g2d_drv_get_status());

#ifdef G2D_DEBUG
            _g2d_reg_dump();
#endif

            g2d_drv_reset();

            g2d_drv_power_off();
        }
        break;
    }

    G2D_DBG("handle command (0x%04x) W2M(0x%08x) L0(0x%08x) L1(0x%08x) L2(0x%08x) L3(0x%08x) done\n", cmd->ctx.flag, ioread32(0xF20C5044), ioread32(0xF20C5084), ioread32(0xF20C50C4), ioread32(0xF20C5104), ioread32(0xF20C5144));

#ifdef G2D_QUEUE
    if (cmd->sync_cmd)
    {
        cmd->sync_cmd = false;
        up(&cmd->sync);
    }
    up(&cmd->lock);
#endif
}

static void _g2d_workqueue_handler(struct work_struct *data)
{
    g2d_command_t *cmd = container_of(data, g2d_command_t, work);

    _g2d_command_handler(cmd);
}

#ifdef G2D_QUEUE
int _g2d_create_workqueue(void)
{
    g2d_workqueue = create_singlethread_workqueue("g2d_workqueue");
    init_waitqueue_head(&isr_wait_queue);

    return (g2d_workqueue) ? 0 : -EFAULT;
}

void _g2d_destroy_workqueue(void)
{
    destroy_workqueue(g2d_workqueue);
}

int _g2d_create_queuebuffer(void)
{
    int i;

    if (!g2d_cmd_buffer)
    {
        g2d_cmd_buffer = (g2d_command_t *)kzalloc(sizeof(g2d_command_t) * G2D_QUEUE_BUFFER_COUNT, GFP_KERNEL);
        if (!g2d_cmd_buffer) return -EFAULT;

        for (i = 0; i < G2D_QUEUE_BUFFER_COUNT; i++)
        {
            INIT_WORK(&g2d_cmd_buffer[i].work, _g2d_workqueue_handler);
            sema_init(&g2d_cmd_buffer[i].lock, 1);
            sema_init(&g2d_cmd_buffer[i].sync, 0);
        }
    }

    return 0;
}

void _g2d_destroy_queuebuffer(void)
{
    if (g2d_cmd_buffer)
    {
        kfree(g2d_cmd_buffer);
        g2d_cmd_buffer = NULL;
    }
}

/*************************************************************************************/

int _g2d_queue_command(unsigned long arg, bool sync)
{
    int next_cmd = 0;

    spin_lock(&g2d_cmd_spinlock);

    next_cmd = g2d_current_cmd + 1;
    if (next_cmd >= G2D_QUEUE_BUFFER_COUNT) next_cmd = 0;

    down(&g2d_cmd_buffer[next_cmd].lock);

    if (copy_from_user(&g2d_cmd_buffer[next_cmd].ctx, (void *)arg, sizeof(g2d_context_t)))
    {
        G2D_ERR("copy from user is failed\n");
        spin_unlock(&g2d_cmd_spinlock);
        return -EFAULT;
    }

    g2d_cmd_buffer[next_cmd].sync_cmd = sync;

    queue_work(g2d_workqueue, &g2d_cmd_buffer[next_cmd].work);

    g2d_current_cmd = next_cmd;

    spin_unlock(&g2d_cmd_spinlock);

    if (sync) down(&g2d_cmd_buffer[next_cmd].sync);

    return 0;
}

int _g2d_queue_command2(g2d_context_t *ctx, bool sync)
{
    int next_cmd = 0;

    spin_lock(&g2d_cmd_spinlock);

    next_cmd = g2d_current_cmd + 1;
    if (next_cmd >= G2D_QUEUE_BUFFER_COUNT) next_cmd = 0;

    down(&g2d_cmd_buffer[next_cmd].lock);

    memcpy(&g2d_cmd_buffer[next_cmd].ctx, ctx, sizeof(g2d_context_t));

    g2d_cmd_buffer[next_cmd].sync_cmd = sync;

    queue_work(g2d_workqueue, &g2d_cmd_buffer[next_cmd].work);

    g2d_current_cmd = next_cmd;

    spin_unlock(&g2d_cmd_spinlock);

    if (sync) down(&g2d_cmd_buffer[next_cmd].sync);

    return 0;
}

#endif // G2D_QUEUE

/*************************************************************************************/

static long g2d_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int ret = 0;

    switch (cmd)
    {
#ifdef G2D_QUEUE
        case G2D_IOCTL_ASYNCCMD:
            ret = _g2d_queue_command(arg, false);
            G2D_DBG("insert async command successfully\n");
            break;

        case G2D_IOCTL_SYNCCMD:
            ret = _g2d_queue_command(arg, true);
            G2D_DBG("insert sync command successfully\n");
            break;
#else
        case G2D_IOCTL_SYNCCMD:
            spin_lock(&g2d_cmd_spinlock);
            if (copy_from_user(&g2d_cmd_buffer->ctx, (void *)arg, sizeof(g2d_context_t)))
            {
                G2D_ERR("copy from user is failed\n");
                return -EFAULT;
            }
            _g2d_command_handler(g2d_cmd_buffer);
            spin_unlock(&g2d_cmd_spinlock);
            break;
#endif
        case G2D_IOCTL_CONNECTFB:
            if (g2d_drv_fb_init() != 0)
            {
                G2D_ERR("failed to connect fb driver\n");
                return -EFAULT;
            }
            break;

        case G2D_IOCTL_DISCONNECTFB:
            g2d_drv_fb_deinit();
            break;

        case G2D_IOCTL_DEQUEUEFB:
            {
                fb_buffer_t *buffer = g2d_drv_fb_dequeue_buffer();

                if (buffer == NULL)
                {
                    G2D_ERR("failed to get framebuffer\n");
                    return -EFAULT;
                }

                if (copy_to_user((void *)arg, &buffer->info, sizeof(g2d_buffer_t)))
                {
                    G2D_ERR("copy to user is failed\n");
                    return -EFAULT;
                }
/*
#ifdef G2D_QUEUE
                {
                    g2d_context_t ctx;
                    memset(&ctx, 0, sizeof(g2d_context_t));
                    ctx.flag = G2D_FLAG_MIRROR_FRONT_FB;
                    ctx.fb_id = buffer->info.idx;
                    _g2d_queue_command2(&ctx, false);
                }
#else
                {
                    spin_lock(&g2d_cmd_spinlock);
                    g2d_cmd_buffer->ctx.flag = G2D_FLAG_MIRROR_FRONT_FB;
                    g2d_cmd_buffer->ctx.fb_id = buffer->info.idx;
                    _g2d_command_handler(g2d_cmd_buffer);
                    spin_unlock(&g2d_cmd_spinlock);
                }
#endif
*/
            }
            break;

        case G2D_IOCTL_ENQUEUEFB:
#ifdef G2D_QUEUE
            {
                g2d_context_t ctx;
                memset(&ctx, 0, sizeof(g2d_context_t));
                ctx.flag = G2D_FLAG_END_OF_FRAME;
                if (copy_from_user(&ctx.fb_id, (void *)arg, sizeof(int)))
                {
                    G2D_ERR("copy from user is failed\n");
                    return -EFAULT;
                }
                _g2d_queue_command2(&ctx, false);
            }
#else
            {
                spin_lock(&g2d_cmd_spinlock);
                g2d_cmd_buffer->ctx.flag = G2D_FLAG_END_OF_FRAME;
                if (copy_from_user(&g2d_cmd_buffer->ctx.fb_id, (void *)arg, sizeof(int)))
                {
                    G2D_ERR("copy from user is failed\n");
                    return -EFAULT;
                }
                _g2d_command_handler(g2d_cmd_buffer);
                spin_unlock(&g2d_cmd_spinlock);
            }
#endif
            break;
    }
    
    return ret;
}

static int g2d_open(struct inode *inode, struct file *file)
{
    G2D_DBG("driver open\n");
    return 0;
}

static ssize_t g2d_read(struct file *file, char __user *data, size_t len, loff_t *ppos)
{
    G2D_DBG("driver read\n");
    return 0;
}

static int g2d_release(struct inode *inode, struct file *file)
{
    G2D_DBG("G2D Driver release\n");
	return 0;
}

static int g2d_flush(struct file * a_pstFile , fl_owner_t a_id)
{
    // To Do : error handling here
    return 0;
}

static struct file_operations g2d_fops = {
    .owner          = THIS_MODULE,
    .unlocked_ioctl = g2d_unlocked_ioctl,
    .open           = g2d_open,
    .release        = g2d_release,
    .flush          = g2d_flush,
    .read           = g2d_read,
};

/*************************************************************************************/

static int g2d_probe(struct platform_device *pdev)
{
    struct class_device;
    struct class_device *class_dev = NULL;
    
    G2D_INF("\n\n\n===================== G2D probe ======================\n\n\n");

    if (alloc_chrdev_region(&g2d_devno, 0, 1, G2D_DEVNAME))
    {
        G2D_ERR("can't get device major number...\n");
        return -EFAULT;
    }

    G2D_INF("get device major number (%d)\n", g2d_devno);

    g2d_cdev = cdev_alloc();
    g2d_cdev->owner = THIS_MODULE;
    g2d_cdev->ops = &g2d_fops;

    cdev_add(g2d_cdev, g2d_devno, 1);

    g2d_class = class_create(THIS_MODULE, G2D_DEVNAME);
    class_dev = (struct class_device *)device_create(g2d_class, NULL, g2d_devno, NULL, G2D_DEVNAME);

#ifdef G2D_QUEUE
    if (_g2d_create_workqueue())
    {
        G2D_ERR("failed to create workqueue\n");
        return -EFAULT;
    }

    if (_g2d_create_queuebuffer())
    {
        G2D_ERR("failed to create queue buffer\n");
        return -EFAULT;
    }
#else
    init_waitqueue_head(&isr_wait_queue);

    if (!g2d_cmd_buffer)
    {
        g2d_cmd_buffer = (g2d_command_t *)kzalloc(sizeof(g2d_command_t), GFP_KERNEL);
        if (!g2d_cmd_buffer) return -EFAULT;
    }
#endif

    mt6577_irq_set_sens(MT6577_G2D_IRQ_ID, MT65xx_EDGE_SENSITIVE);
    mt6577_irq_set_polarity(MT6577_G2D_IRQ_ID, MT65xx_POLARITY_LOW);

    if (request_irq(MT6577_G2D_IRQ_ID, g2d_drv_isr, 0, "G2D ISR" , NULL))
    {
        G2D_ERR("request irq failed\n");
    }

    G2D_INF("probe is done\n");

    NOT_REFERENCED(class_dev);
    return 0;
}

static int g2d_remove(struct platform_device *pdev)
{
    G2D_INF("device remove\n");

#ifdef G2D_QUEUE
    _g2d_destroy_queuebuffer();
    _g2d_destroy_workqueue();
#else
    kfree(g2d_cmd_buffer);
    g2d_cmd_buffer = NULL;
#endif

    free_irq(MT6577_G2D_IRQ_ID, NULL);

    return 0;
}

static void g2d_shutdown(struct platform_device *pdev)
{
    G2D_INF("g2d device shutdown\n");
}

static int g2d_suspend(struct platform_device *pdev, pm_message_t mesg)
{
    return 0;
}

static int g2d_resume(struct platform_device *pdev)
{
    return 0;
}


static struct platform_driver g2d_driver = {
    .probe		= g2d_probe,
    .remove		= g2d_remove,
    .shutdown	= g2d_shutdown,
    .suspend	= g2d_suspend,
    .resume		= g2d_resume,
    .driver     = {
        .name = G2D_DEVNAME,
    },
};

/*************************************************************************************/

static void g2d_device_release(struct device *dev)
{
}

static u64 g2d_dmamask = ~(u32)0;

static struct platform_device g2d_device = {
    .name	 = G2D_DEVNAME,
    .id      = 0,
    .dev     = {
        .release = g2d_device_release,
        .dma_mask = &g2d_dmamask,
        .coherent_dma_mask = 0xffffffff,
    },
    .num_resources = 0,
};

/*************************************************************************************/

static int __init g2d_init(void)
{
    G2D_INF("initializeing driver...\n");
	
    if (platform_device_register(&g2d_device))
    {
        G2D_ERR("failed to register device\n");
        return -ENODEV;
    }

    if (platform_driver_register(&g2d_driver))
    {
        G2D_ERR("failed to register driver\n");
        platform_device_unregister(&g2d_device);
        return -ENODEV;
    }

    return 0;
}

static void __exit g2d_exit(void)
{
    cdev_del(g2d_cdev);
    unregister_chrdev_region(g2d_devno, 1);

    platform_driver_unregister(&g2d_driver);
    platform_device_unregister(&g2d_device);
	
    device_destroy(g2d_class, g2d_devno);
    class_destroy(g2d_class);
	
    G2D_INF("exit driver...\n");
}

module_init(g2d_init);
module_exit(g2d_exit);

MODULE_DESCRIPTION("MediaTek MT6577 G2D Driver");
MODULE_AUTHOR("Nick Ko <nick.ko@mediatek.com>");

