/*
 * Copyright (C) 2010 Trusted Logic S.A.
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/jiffies.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#include <linux/spinlock.h>
#include <linux/nfc/pn544.h>

#include <cust_gpio_usage.h>
#include <cust_eint.h>

#include <mach/mt6577_gpio.h>
#include <mach/eint.h>

#define TRACE_THIS_MODULE   1

#define MAX_BUFFER_SIZE	512
//#define PN544_VEN_REVERSE

#define ENTER_DEBUG()   {}

#ifdef TRACE_THIS_MODULE
#define ENTER() \
	printk(KERN_ERR "%s\n", __FUNCTION__)
#else /* TRACE_THIS_MODULE */
#define ENTER()
#endif /* TRACE_THIS_MODULE */

#define MY_DEBUG    1
#if defined(MY_DEBUG)
#define my_pr_info      pr_err
#define my_pr_debug     pr_err
#define my_pr_err       pr_err
#define my_pr_warning   pr_err
#else
#define my_pr_info      pr_info
#define my_pr_debug     pr_debug
#define my_pr_err       pr_err
#define my_pr_warning   pr_warning
#endif


#define I2C_USE_OPEN_DRAIN  0
#if (I2C_USE_OPEN_DRAIN == 1)
#define OPEN_DRAIN_FLAG     I2C_ENEXT_FLAG
#else
#define OPEN_DRAIN_FLAG     0x0000
#endif

static int pn544_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int pn544_remove(struct i2c_client *client);
static int pn544_detect(struct i2c_client *client, int kind, struct i2c_board_info *info);

struct pn544_dev	{
	wait_queue_head_t	read_wq;
	struct mutex		read_mutex;
	struct i2c_client	*client;
	struct miscdevice	pn544_device;
	unsigned int 		ven_gpio;
	unsigned int 		firm_gpio;
	unsigned int		irq_gpio;
	bool			irq_enabled;
	spinlock_t		irq_enabled_lock;
};


#define NFC_I2C_BUSNUM  0
#define I2C_ID_NAME "pn544"
static struct i2c_board_info __initdata nfc_board_info = {I2C_BOARD_INFO(I2C_ID_NAME, 0x28)};

static const struct i2c_device_id pn544_id[] = {
	{ I2C_ID_NAME, 0 },
	{ }
};

static struct i2c_driver pn544_dev_driver = {
	.id_table	= pn544_id,
	.probe		= pn544_probe,
	.remove		= pn544_remove,
	.detect		= pn544_detect,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= I2C_ID_NAME,
	},
};

static struct pn544_i2c_platform_data pn544_platform_data = {
   .irq_gpio = GPIO_NFC_EINT_PIN,
   .ven_gpio = GPIO_NFC_VEN_PIN,
   .firm_gpio = GPIO_NFC_FIRM_PIN,
};

struct pn544_dev *pn544_dev_ptr = NULL;

#if 0
static int __sched do_usleep_range(unsigned long min, unsigned long max) 
{ 
    ktime_t kmin; 
    unsigned long delta; 
    
    kmin = ktime_set(0, min * NSEC_PER_USEC); 
    delta = max - min; 
    return schedule_hrtimeout_range(&kmin, delta, HRTIMER_MODE_REL); 
} 

/** 
* usleep_range - Drop in replacement for udelay where wakeup is flexible 
* @min: Minimum time in usecs to sleep 
* @max: Maximum time in usecs to sleep 
*/ 
void usleep_range(unsigned long min, unsigned long max) 
{ 
    __set_current_state(TASK_UNINTERRUPTIBLE); 
    do_usleep_range(min, max); 
} 
#endif

static void pn544_disable_irq(struct pn544_dev *pn544_dev)
{
	unsigned long flags;

	spin_lock_irqsave(&pn544_dev->irq_enabled_lock, flags);
	my_pr_info("%s : irq %d, enable=%d", __func__, pn544_dev->client->irq, pn544_dev->irq_enabled);
	if (pn544_dev->irq_enabled) {
		//disable_irq_nosync(pn544_dev->client->irq);
		mt65xx_eint_mask(pn544_dev->client->irq);
		pn544_dev->irq_enabled = false;
	   my_pr_info("%s : irq %d disabled", __func__, pn544_dev->client->irq);
	}
	spin_unlock_irqrestore(&pn544_dev->irq_enabled_lock, flags);
}

#if 0
static irqreturn_t pn544_dev_irq_handler(int irq, void *dev_id)
{
	struct pn544_dev *pn544_dev = dev_id;

	pn544_disable_irq(pn544_dev);

	/* Wake up waiting readers */
	wake_up(&pn544_dev->read_wq);

	return IRQ_HANDLED;
}
#endif
void pn544_dev_irq_handler(void)
{
	struct pn544_dev *pn544_dev = pn544_dev_ptr;

	my_pr_info("%s : &pn544_dev=%p", __func__, pn544_dev);

	pn544_disable_irq(pn544_dev);

	/* Wake up waiting readers */
	wake_up(&pn544_dev->read_wq);

	my_pr_info("%s : wake_up &read_wq=%p", __func__, &pn544_dev->read_wq);
}

static ssize_t pn544_dev_read(struct file *filp, char __user *buf,
		size_t count, loff_t *offset)
{
	struct pn544_dev *pn544_dev = filp->private_data;
	char tmp[MAX_BUFFER_SIZE];
	int ret;
	int i =0 ;
    ENTER();
	my_pr_info("%s : %d, &pn544_dev=%p", __func__, count, pn544_dev);

	if (count > MAX_BUFFER_SIZE)
		count = MAX_BUFFER_SIZE;

	my_pr_info("%s : reading %zu bytes.\n", __func__, count);

	mutex_lock(&pn544_dev->read_mutex);

	if (!mt_get_gpio_in(pn544_dev->irq_gpio)) {
		if (filp->f_flags & O_NONBLOCK) {
			ret = -EAGAIN;
			goto fail;
		}

		pn544_dev->irq_enabled = true;
		//enable_irq(pn544_dev->client->irq);
		mt65xx_eint_unmask(pn544_dev->client->irq);

	        my_pr_info("%s : mt65xx_eint_unmask %d", __func__, pn544_dev->client->irq);

		ret = wait_event_interruptible(pn544_dev->read_wq,mt_get_gpio_in(pn544_dev->irq_gpio));

	        my_pr_info("%s : wait_event_interruptible ret %d", __func__, ret);

		pn544_disable_irq(pn544_dev);

		if (ret)
			goto fail;

	}

    pn544_dev->client->addr = (pn544_dev->client->addr & I2C_MASK_FLAG  )|OPEN_DRAIN_FLAG;
    pn544_dev->client->timing = 400;
	/* Read data */
	ret = i2c_master_recv(pn544_dev->client, tmp, count);
	mutex_unlock(&pn544_dev->read_mutex);

	if (ret < 0) {
		my_pr_err("%s: i2c_master_recv returned %d\n", __func__, ret);
		return ret;
	}
	if (ret > count) {
		my_pr_err("%s: received too many bytes from i2c (%d)\n",
			__func__, ret);
		return -EIO;
	}

	if (copy_to_user(buf, tmp, ret)) {
		my_pr_warning("%s : failed to copy to user space\n", __func__);
		return -EFAULT;
	}
	return ret;

fail:
	mutex_unlock(&pn544_dev->read_mutex);
	return ret;
}

static ssize_t pn544_dev_write(struct file *filp, const char __user *buf,
		size_t count, loff_t *offset)
{
	struct pn544_dev  *pn544_dev;
	char tmp[MAX_BUFFER_SIZE];
	char *data_t = tmp;
	int ret;
	int i =0;

    ENTER();
	my_pr_info("%s : %d", __func__, count);

	pn544_dev = filp->private_data;

	if (count > MAX_BUFFER_SIZE)
		count = MAX_BUFFER_SIZE;

	if (copy_from_user(tmp, buf, count)) {
		my_pr_err("%s : failed to copy from user space\n", __func__);
		return -EFAULT;
	}
	my_pr_info("%s : writing %zu bytes.\n", __func__, count);
	/* Write data */
	pn544_dev->client->addr = (pn544_dev->client->addr & I2C_MASK_FLAG  )|OPEN_DRAIN_FLAG;
	pn544_dev->client->timing = 400;
#if 0  
	ret = i2c_master_send(pn544_dev->client, tmp, count);
	//add retry by NXP suggestion,0719
	if(ret == -EREMOTEIO)
	{
		usleep_range(6000,10000);
		//msleep(10);			
		ret = i2c_master_send(pn544_dev->client, tmp, count);
		my_pr_info("[PN544_DEBUG],I2C Writer Retry,%d",ret);
	}
#endif
    ret=0;
    if(count > 256)
    {
	    int sendOutCnt=0;
	    int remainSize=0;
	    int loop=0;
	    
	    remainSize=count%200;
	    loop=count/200;

	    for(i=0;i<loop;i++)
	    {                       
            sendOutCnt = i2c_master_send(pn544_dev->client, data_t, 200);
			//add retry by NXP suggestion,0719
            if(sendOutCnt == -EREMOTEIO)
            {
	            usleep_range(6000,10000);
	            //msleep(10);                 
	            sendOutCnt = i2c_master_send(pn544_dev->client, data_t, 200);
	            my_pr_info("[PN544_DEBUG],I2C Writer Retry,%d", sendOutCnt);
            }
			if (sendOutCnt != 200) {
				my_pr_err("%s : i2c_master_send failed, returned1 %d\n", __func__, sendOutCnt);
				ret = -EIO;
			}
			else
			{
			    my_pr_err("%s : i2c_master_send success1!\n", __func__);
			}
            ret+=sendOutCnt;
            data_t+=sendOutCnt;
	    }
		
	    if(remainSize>0)
        {
		    sendOutCnt = i2c_master_send(pn544_dev->client, data_t, remainSize);	    
		    ret+=sendOutCnt;  
	    }
		if (sendOutCnt != remainSize) {
			my_pr_err("%s : i2c_master_send failed, returned2 %d\n", __func__, sendOutCnt);
			ret = -EIO;
		}
		else
		{
		    my_pr_err("%s : i2c_master_send success2!\n", __func__);
		}
		
		my_pr_info("%s : count:%zu,loop:%d,remainSize:%d\n", __func__, count,loop,remainSize);
    }
    else
    {
		ret = i2c_master_send(pn544_dev->client, tmp, count);
		//add retry by NXP suggestion,0719
		if(ret == -EREMOTEIO)
		{
			usleep_range(6000,10000);
			//msleep(10);			
			ret = i2c_master_send(pn544_dev->client, tmp, count);
			my_pr_info("[PN544_DEBUG],I2C Writer Retry,%d",ret);
		}
    }

	if (ret != count) {
		my_pr_err("%s : i2c_master_send failed, returned3 %d\n", __func__, ret);
		ret = -EIO;
	}
	else
	{
	    my_pr_err("%s : i2c_master_send success3!\n", __func__);
	}
	
        my_pr_info("%s : writing %zu bytes. Status %zu \n", __func__, count,ret);
	return ret;
}

static int pn544_dev_open(struct inode *inode, struct file *filp)
{
	struct pn544_dev *pn544_dev = container_of(filp->private_data,
						struct pn544_dev,
						pn544_device);
    ENTER();
	filp->private_data = pn544_dev;
	pn544_dev_ptr = pn544_dev;
    ENTER();
	my_pr_info("%s : %d,%d, &pn544_dev_ptr=%p", __func__, imajor(inode), iminor(inode), pn544_dev_ptr);

	return 0;
}

static long pn544_dev_ioctl(struct file *filp,
			    unsigned int cmd, unsigned long arg)
{
	struct pn544_dev *pn544_dev = filp->private_data;

    ENTER();
#if 0
	switch (cmd) {
	case PN544_SET_PWR:
		if (arg > 1) {
			mt_set_gpio_out(pn544_dev->ven_gpio, GPIO_OUT_ZERO);
			mt_set_gpio_out(pn544_dev->firm_gpio, GPIO_OUT_ONE);
			msleep(3);
		} else if (arg == 1) {
			mt_set_gpio_out(pn544_dev->ven_gpio, GPIO_OUT_ZERO);
			mt_set_gpio_out(pn544_dev->firm_gpio, GPIO_OUT_ZERO);
			msleep(3);
		} else   {
			mt_set_gpio_out(pn544_dev->ven_gpio, GPIO_OUT_ONE);
		    mt_set_gpio_out(pn544_dev->firm_gpio, GPIO_OUT_ZERO);
			msleep(70);		
		}
		break;
#endif
    my_pr_err("%s,cmd%u,arg%d\n", __func__, cmd,arg);
	switch (cmd) {
    case PN544_SET_PWR:
        if (arg == 2) {/*firmware down mode*/
            mt_set_gpio_out(pn544_dev->ven_gpio, GPIO_OUT_ZERO);
            mt_set_gpio_out(pn544_dev->firm_gpio, GPIO_OUT_ONE);
            msleep(10);
            mt_set_gpio_out(pn544_dev->ven_gpio, GPIO_OUT_ONE);
            msleep(50);
            mt_set_gpio_out(pn544_dev->ven_gpio, GPIO_OUT_ZERO);
            msleep(70);
        } else if (arg == 1) {/*power on*/
            mt_set_gpio_out(pn544_dev->ven_gpio, GPIO_OUT_ZERO);
            mt_set_gpio_out(pn544_dev->firm_gpio, GPIO_OUT_ZERO);
            msleep(3);
        } else if(arg==0){/*power off*/
            mt_set_gpio_out(pn544_dev->ven_gpio, GPIO_OUT_ONE);
            mt_set_gpio_out(pn544_dev->firm_gpio, GPIO_OUT_ZERO);
            msleep(70);             
        }else {
            my_pr_err("%s bad ioctl %u\n", __func__, cmd);
        }
        break;
	default:
		my_pr_err("%s bad ioctl %u\n", __func__, cmd);
		return -EINVAL;
	}

	return 0;
}

static const struct file_operations pn544_dev_fops = {
	.owner	= THIS_MODULE,
	.llseek	= no_llseek,
	.read	= pn544_dev_read,
	.write	= pn544_dev_write,
	.open	= pn544_dev_open,
	.unlocked_ioctl	= pn544_dev_ioctl,
};

static int pn544_detect(struct i2c_client *client, int kind, struct i2c_board_info *info)
{
    ENTER();
    my_pr_info("pn544_detect\n");
    strcpy(info->type, "pn544");
    return 0;
}

static int pn544_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	int ret;
	struct pn544_i2c_platform_data *platform_data;
	struct pn544_dev *pn544_dev;

    ENTER();
	my_pr_info("pn544_probe\n");

	//platform_data = client->dev.platform_data;
	platform_data = &pn544_platform_data;

	if (platform_data == NULL) {
		my_pr_err("%s : nfc probe fail\n", __func__);
		return  -ENODEV;
	}

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		my_pr_err("%s : need I2C_FUNC_I2C\n", __func__);
		return  -ENODEV;
	}

#if 0
	ret = gpio_request(platform_data->irq_gpio, "nfc_int");
	if (ret)
		return  -ENODEV;
	ret = gpio_request(platform_data->ven_gpio, "nfc_ven");
	if (ret)
		goto err_ven;
	ret = gpio_request(platform_data->firm_gpio, "nfc_firm");
	if (ret)
		goto err_firm;
#endif

   /* irq_gpio */
	mt_set_gpio_mode(platform_data->irq_gpio, GPIO_NFC_EINT_PIN_M_EINT);
	mt_set_gpio_dir(platform_data->irq_gpio, GPIO_DIR_IN);
	mt_set_gpio_pull_enable(platform_data->irq_gpio, GPIO_PULL_ENABLE);
	mt_set_gpio_pull_select(platform_data->irq_gpio, GPIO_PULL_DOWN);
	/* ven_gpio */
	mt_set_gpio_mode(platform_data->ven_gpio, GPIO_NFC_VEN_PIN_M_GPIO);
	mt_set_gpio_dir(platform_data->ven_gpio, GPIO_DIR_OUT);
	mt_set_gpio_out(platform_data->ven_gpio,GPIO_OUT_ZERO);
	/* firm_gpio */
	mt_set_gpio_mode(platform_data->firm_gpio, GPIO_NFC_FIRM_PIN_M_GPIO);
	mt_set_gpio_dir(platform_data->firm_gpio, GPIO_DIR_OUT);

	pn544_dev = kzalloc(sizeof(*pn544_dev), GFP_KERNEL);
	if (pn544_dev == NULL) {
		//dev_err(&client->dev,
		//		"failed to allocate memory for module data\n");
		my_pr_err("pn544_dev: failed to allocate memory for module data\n");
		ret = -ENOMEM;
		goto err_exit;
	}

	pn544_dev->irq_gpio = platform_data->irq_gpio;
	pn544_dev->ven_gpio  = platform_data->ven_gpio;
	pn544_dev->firm_gpio  = platform_data->firm_gpio;
	pn544_dev->client   = client;

	/* init mutex and queues */
	init_waitqueue_head(&pn544_dev->read_wq);
	mutex_init(&pn544_dev->read_mutex);
	spin_lock_init(&pn544_dev->irq_enabled_lock);

	pn544_dev->pn544_device.minor = MISC_DYNAMIC_MINOR;
	pn544_dev->pn544_device.name = "pn544";
	pn544_dev->pn544_device.fops = &pn544_dev_fops;

	ret = misc_register(&pn544_dev->pn544_device);
	if (ret) {
		my_pr_err("%s : misc_register failed\n", __FILE__);
		goto err_misc_register;
	}

	/* request irq.  the irq is set whenever the chip has data available
	 * for reading.  it is cleared when all data has been read.
	 */
	client->irq = CUST_EINT_NFC_NUM;
	my_pr_info("%s : requesting IRQ %d\n", __func__, client->irq);
	pn544_dev->irq_enabled = true;
#if 0
	ret = request_irq(client->irq, pn544_dev_irq_handler,
			  IRQF_TRIGGER_HIGH, client->name, pn544_dev);
	if (ret) {
		dev_err(&client->dev, "request_irq failed\n");
		goto err_request_irq_failed;
	}
#endif
	mt65xx_eint_set_sens(CUST_EINT_NFC_NUM, CUST_EINT_NFC_SENSITIVE);
	mt65xx_eint_set_hw_debounce(CUST_EINT_NFC_NUM, CUST_EINT_NFC_DEBOUNCE_CN);
	mt65xx_eint_registration(CUST_EINT_NFC_NUM, CUST_EINT_NFC_DEBOUNCE_EN, CUST_EINT_NFC_POLARITY, pn544_dev_irq_handler, 0);
	mt65xx_eint_unmask(CUST_EINT_NFC_NUM);
	pn544_disable_irq(pn544_dev);
	i2c_set_clientdata(client, pn544_dev);

	return 0;

err_request_irq_failed:
	misc_deregister(&pn544_dev->pn544_device);
err_misc_register:
	mutex_destroy(&pn544_dev->read_mutex);
	kfree(pn544_dev);
err_exit:
	gpio_free(platform_data->firm_gpio);
err_firm:
	gpio_free(platform_data->ven_gpio);
err_ven:
	gpio_free(platform_data->irq_gpio);
	return ret;
}

static int pn544_remove(struct i2c_client *client)
{
	struct pn544_dev *pn544_dev;

	pn544_dev = i2c_get_clientdata(client);
	
	my_pr_info("Reset PN544 for card emulation in power off\n");
	mt_set_gpio_out(pn544_dev->ven_gpio, GPIO_OUT_ONE);
	msleep(50);
	mt_set_gpio_out(pn544_dev->ven_gpio, GPIO_OUT_ZERO);
	msleep(50);
	mt_set_gpio_out(pn544_dev->ven_gpio, GPIO_OUT_ONE);
	
	free_irq(client->irq, pn544_dev);
	misc_deregister(&pn544_dev->pn544_device);
	mutex_destroy(&pn544_dev->read_mutex);
	gpio_free(pn544_dev->irq_gpio);
	gpio_free(pn544_dev->ven_gpio);
	gpio_free(pn544_dev->firm_gpio);
	kfree(pn544_dev);

	return 0;
}

static struct i2c_driver pn544_driver = {
	.id_table	= pn544_id,
	.probe		= pn544_probe,
	.remove		= pn544_remove,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= I2C_ID_NAME,
	},
};

/*
 * module load/unload record keeping
 */
static int __init pn544_dev_init(void)
{
    ENTER();
    my_pr_info("Loading pn544 driver\n");
    my_pr_err("pn544 err level check\n");
    my_pr_debug("pn544 err level check\n");
    i2c_register_board_info(NFC_I2C_BUSNUM, &nfc_board_info, 1);
    return i2c_add_driver(&pn544_driver);
}
module_init(pn544_dev_init);

static void __exit pn544_dev_exit(void)
{
    ENTER();
	my_pr_info("Unloading pn544 driver\n");
	i2c_del_driver(&pn544_driver);
}
module_exit(pn544_dev_exit);

MODULE_AUTHOR("Sylvain Fonteneau");
MODULE_DESCRIPTION("NFC PN544 driver");
MODULE_LICENSE("GPL");
