/* -------------------------------------------------------------------------
 * Copyright (C) 2011 Inside contactless
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * ------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------
   This skeleton driver is provided as an example, to speed up porting of 
   the Open NFC stack onto a platform embedding MicroRead chip connected 
   through I2C.

  The main adaptation consists in :
		- definition of the 2 GPIOs (IRQOUT, and RST/WakeUP), and their initialization.
		- the name of the  I2C driver available for MicroRead  ("MicroRead" in this example ).

   Typically 4 lines (at least) should be modified:
      - Platform IRQ Number used with IRQOUT, if not dynamically allocated
      - GPIO_NFC_IRQOUT definition (IRQOUT pin of MicroRead)
      - GPIO_NFC_WAKEUP definition (RESETWAKE_UP pin of MicroRead)
      - "struct i2c_device_id" line for I2C Slave definition for MicroRead and the initialization of the I2C bus (i2c_get_adapter, ...).

  Else, with a bit of luck, all the rest should be useable as is.
  And , of course,  this is not the only way to do the job.
 * ------------------------------------------------------------------------- */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/kthread.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/irq.h>
#include <linux/workqueue.h>
#include <linux/poll.h>
#include <linux/slab.h>

#include <asm/uaccess.h>
//#include <asm/gpio.h>
#include <asm/delay.h>

#include <linux/gpio.h>

#include <mach/mt6577_gpio.h>
#include <mach/eint.h>
#include <cust_gpio_usage.h>
#include <cust_eint.h>

#include "open_nfc_main.h"



/* define to debug all function calls */
#define TRACE_THIS_MODULE	1

/***************************************/
/******  Platform dependent  ***********/

/* GPIO line to request for chipset IRQ_OUT. Undefine if this is not used. */
#define GPIO_NFC_IRQOUT	      69

/* IRQ number to use if this is not dynamic in the platform. */
/* #define IRQ_STATIC_NUMBER    63 */

/* GPIO line to request for chipset RESET/WAKE_UP. Undefine if this is not available. */
#define GPIO_NFC_WAKEUP	      33

/* Use workeque instead of mutex, because calling mutex_lock will sometimes hang the process */
#define USE_WORKQUEUE


/* I2C parameters, must be in sync with your board configuration */
#define I2C_ID_NAME	"nfcc" // "MicroRead"
#define I2C_DEVICE_ADDR	0 //  0x5E

#define NFC_I2C_BUSNUM  0
static struct i2c_board_info __initdata nfc_board_info = {I2C_BOARD_INFO(I2C_ID_NAME, 0x5E)};


/* define this to start reading I2C as soon as driver is loaded (this wakes up the chip) 
   undefine to start reading I2C only when user-space application connects to the device */
//#define WAKE_AND_PURGE_I2C_ON_LOAD 1

/***************************************/

#if (!defined( GPIO_NFC_IRQOUT )) && (! defined (IRQ_STATIC_NUMBER))
#error "GPIO_NFC_IRQOUT or IRQ_STATIC_NUMBER must be defined"
#endif

#define I2C_READ_DATA_LENGTH   32 //MEDIATEK HW LIMITATION 	/* Number of bytes to Read during an I2C Read cycle (MicroRead TX FIFO Size + Len + CRC) */
#define I2C_WRITE_DATA_LENGTH  32 //MEDIATEK HW LIMITATION : separate Rx/Tx sizes
#define I2C_READ_NB_MSG	       4	/* Number of messages to buffer before discarding them */

#ifdef TRACE_THIS_MODULE
#define ENTER() \
	printk(KERN_ERR "%s\n", __FUNCTION__)
#else /* TRACE_THIS_MODULE */
#define ENTER()
#endif /* TRACE_THIS_MODULE */

/* The normal driver driver sequence is:
 - open
 - configure
 - connect
 - ... reset / read / write / poll
 - close
 */
enum custom_state {
	CUSTOM_INIT = 0,
	CUSTOM_PROBED,
	CUSTOM_OPENED,
	CUSTOM_CONFIGURED,
	CUSTOM_READY
};

   /* Context variable */
struct open_nfc_custom_device {
	/* configuration stuff */

	enum custom_state state;

	/* lock */
	struct mutex    mutex;	/* mutex for concurrency */

	/* I2C Driver related stuff */
	struct i2c_client *i2c_client;	/* I2C Driver registering structure */
	// unsigned int    irqout; /* request_irq() not used on MTK platform */
	struct work_struct irqout_event_work;	/* WorkQueue, for bottom-half processing of IRQOUT */
	struct work_struct reset_event_work;	/* WorkQueue, decreasing pending on reset */
	struct timer_list sResetTimer;	/* 10 ms one-shot Timer, used for RESET sequence */

	/* I2C receiver */
	uint8_t         rx_buffer[I2C_READ_NB_MSG][I2C_READ_DATA_LENGTH];
	uint32_t        rx_ready, rx_rcvnext;
	int             rx_listener, rx_failures;
	struct timer_list rx_timer;	/* 100 ms one-shot Timer, used for read retries */

	/* I2C transmitter */
	int             reset_pending;

	/* process synchronization (poll) */
	wait_queue_head_t read_queue;
	wait_queue_head_t reset_queue;
};

static struct open_nfc_custom_device *open_nfc_p_device = NULL;

   /* Local Prototypes */
static void     open_nfc_irqout_worker(struct work_struct *work);
static irqreturn_t open_nfc_i2c_interrupt(int irq, void *dev_id);


/* Function to cleanup the state of the driver, reverts to CUSTOM_PROBED state. */
static void
opennfc_struct_cleanup(struct open_nfc_custom_device *p_device)
{
	ENTER();
	if (!p_device) {
		printk("opennfc_struct_cleanup: Internal error p_device is missing\n");
		return;
	}

	mutex_lock(&p_device->mutex);

	if (p_device->state > CUSTOM_PROBED) {	/* it is opened */

		/* There is no listener anymore */
		p_device->rx_listener = 0;

		p_device->state = CUSTOM_PROBED;
	}

	mutex_unlock(&p_device->mutex);
}

/**
  * Function called when the user opens /dev/nfcc
  *
  * @return 0 on success, a negative value on failure.
  */
int
open_nfc_custom_open(struct inode *inode, struct file *filp)
{
	struct open_nfc_custom_device *p_device = open_nfc_p_device;
	int             retval = 0;

	ENTER();
	
	if (!p_device) {
		printk("open_nfc_custom_open: Internal error p_device is missing\n");
		return -ENODEV;
	}

	mutex_lock(&p_device->mutex);

	if (p_device->state == CUSTOM_INIT) {
		printk(KERN_ERR "I2C controler was not probed, unable to send i2c data to the MR!\n");
		retval = -ENODEV;
		goto end;
	}

	if (p_device->state != CUSTOM_PROBED) {
		printk(KERN_ERR "The device is already opened\n");
		retval = -EBUSY;
		goto end;
	}

	filp->private_data = open_nfc_p_device;

	p_device->state = CUSTOM_OPENED;
      end:
	mutex_unlock(&p_device->mutex);

	return retval;
}

/**
  * Processes the OPEN_NFC_IOC_CONFIGURE ioctl
  *
  * @return 0 on success, a negative value on failure.
  */

long
open_nfc_custom_ioctl_configure(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int             retval = 0;
	struct open_nfc_custom_device *p_device = filp->private_data;

	ENTER();
	
	if (!p_device) {
		printk("open_nfc_custom_ioctl_configure: Internal error p_device is missing\n");
		return -ENODEV;
	}


	mutex_lock(&p_device->mutex);

	if (p_device->state != CUSTOM_OPENED) {
		retval = EALREADY;
		goto end;
	}


	p_device->state = CUSTOM_CONFIGURED;
	retval = 0;
      end:
	mutex_unlock(&p_device->mutex);

	return retval;
}

/**
  * Processes the OPEN_NFC_IOC_CONNECT ioctl
  *
  * @return 0 on success, a negative value on failure.
  */

long
open_nfc_custom_ioctl_connect(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct open_nfc_custom_device *p_device = filp->private_data;
	int             retval = 0;

	ENTER();
	
	if (!p_device) {
		printk("open_nfc_custom_ioctl_connect: Internal error p_device is missing\n");
		return -ENODEV;
	}

	mutex_lock(&p_device->mutex);

	if (p_device->state != CUSTOM_CONFIGURED) {
		printk("CONNECT called while device was not configured / already connected\n");
		retval = -EALREADY;
		goto end;
	}

	p_device->state = CUSTOM_READY;

	/* start collecting the received messages */
	p_device->rx_listener = 1;

#ifndef WAKE_AND_PURGE_I2C_ON_LOAD
//	enable_irq(p_device->irqout);
  mt65xx_eint_unmask(CUST_EINT_NFC_NUM);
#endif /* WAKE_AND_PURGE_I2C_ON_LOAD */
       
      end:
	mutex_unlock(&p_device->mutex);

	return retval;
}

/**
  * Function called when the user reads data
  *
  * @return 0 on success, a negative value on failure.
  */
ssize_t
open_nfc_custom_read(struct file * filp, char __user * buf, size_t count, loff_t * f_pos)
{
	struct open_nfc_custom_device *p_device = filp->private_data;
	ssize_t         retval;

	ENTER();
	
	if (!p_device) {
		printk("open_nfc_custom_read: Internal error p_device is missing\n");
		return -ENODEV;
	}

	/* we allow read only if the connection with the NFC Controller has been established */
	mutex_lock(&p_device->mutex);

	if (p_device->state != CUSTOM_READY) {
		printk("Read called while device was not configured / connected\n");
		retval = -ENOTCONN;
		goto end;
	}

	if (p_device->rx_rcvnext <= p_device->rx_ready) {
		/* no data available */
		retval = -EAGAIN;
		goto end;
	}

	/* we have data to read in p_device->rx_buffer[p_device->rx_ready % I2C_READ_NB_MSG] */

	retval = p_device->rx_buffer[p_device->rx_ready % I2C_READ_NB_MSG][0] + 2;	/* Add "Len" + CRC fields ; ignore padding bytes */
	if (count < retval) {
		/* supplied buffer is too small */
		printk(KERN_ERR "open_nfc_custom_read : provided buffer too short.\n");
		retval = -ENOSPC;
		goto end;
	}

	if (copy_to_user(buf, p_device->rx_buffer[p_device->rx_ready % I2C_READ_NB_MSG], retval)) {
		printk(KERN_ERR "open_nfc_custom_read : unable to access to user buffer.\n");
		retval = -EFAULT;
		goto end;
	}

	p_device->rx_ready++;

      end:
	mutex_unlock(&p_device->mutex);

	return retval;
}

/**
  * Function called when the user writes data
  *
  * @return 0 on success, a negative value on failure.
  */

ssize_t
open_nfc_custom_write(struct file * filp, const char __user * buf, size_t count, loff_t * f_pos)
{
	struct open_nfc_custom_device *p_device = filp->private_data;
	int             retval;
	uint8_t         tmpbuf[I2C_WRITE_DATA_LENGTH];
	size_t          sent = 0;

	ENTER();
	
	if (!p_device) {
		printk("open_nfc_custom_write: Internal error p_device is missing\n");
		return -ENODEV;
	}

	mutex_lock(&p_device->mutex);

	if (p_device->state != CUSTOM_READY) {
		printk("Write called while device was not configured / connected\n");
		retval = -ENOTCONN;
		goto end;
	}

	while (sent < count) {
		/* bytes to send: min( remaining, I2C_WRITE_DATA_LENGTH ) */
		size_t          tosend = I2C_WRITE_DATA_LENGTH;
		if ((count - sent) < I2C_WRITE_DATA_LENGTH)
			tosend = count - sent;

		if (copy_from_user(tmpbuf, buf + sent, tosend) != 0) {
			printk(KERN_ERR "open_nfc_custom_write  : copy_from_user failed\n");
			retval = -EFAULT;
			goto end;
		}

		retval = i2c_master_send(p_device->i2c_client, tmpbuf, tosend);
		if (retval != tosend) {
			printk(KERN_ERR "open_nfc_custom_write : i2c_master_send() failed (returns %d)\n", retval);
			// retval = -EFAULT;
			retval = tosend;
			goto end;
		}

		sent += retval;
	}

	retval = sent;

      end:

	mutex_unlock(&p_device->mutex);

	return retval;
}


/**
  * Processes the 10 ms RESET Timeout handler
  *
  * @return 0 on success, a negative value on failure.
  */
#ifdef GPIO_NFC_WAKEUP

static void
open_nfc_reset_timeout(unsigned long arg)
{
	struct open_nfc_custom_device *p_device = open_nfc_p_device;

	ENTER();
	
	if (!p_device) {
		printk("open_nfc_reset_timeout: Internal error p_device is missing\n");
		return;
	}
	else{
#ifdef USE_WORKQUEUE
        printk("open_nfc_reset_timeout workqueue\n");
        schedule_work(&p_device->reset_event_work);
#else
        printk("open_nfc_reset_timeout directcall\n");
    	mutex_lock(&p_device->mutex);
    
    	p_device->reset_pending--;
    
    	if (p_device->reset_pending == 0) {
    		wake_up(&p_device->reset_queue);
    	}
    	mutex_unlock(&p_device->mutex);
#endif
	}

}
#endif /* GPIO_NFC_WAKEUP */

/**
  * Processes the OPEN_NFC_IOC_RESET ioctl
  *
  * @return 0 on success, a negative value on failure.
  */

long
open_nfc_custom_ioctl_reset(struct file *filp, unsigned int cmd, unsigned long arg)
{
#ifndef GPIO_NFC_WAKEUP /* otherwise WAKEUP is not available */
	printk("open_nfc_custom_ioctl_reset: The GPIO line is not defined\n");
	return -ENODEV;
#else /* GPIO_NFC_WAKEUP */
	
	struct open_nfc_custom_device *p_device = filp->private_data;
	int             rc = 0;

	ENTER();
	
	if (!p_device) {
		printk("open_nfc_custom_ioctl_reset: Internal error p_device is missing\n");
		return -ENODEV;
	}

	mutex_lock(&p_device->mutex);

	if (p_device->state != CUSTOM_READY) {
		printk("Reset called while device was not configured / connected\n");
		rc = -ENOTCONN;
		goto out;
	}

	/* Assert RST/WakeUP for at least 1 micro-second */
	/* Wake up the chipset now */
  mt_set_gpio_out(GPIO_NFC_VEN_PIN, GPIO_OUT_ONE);
	udelay(2);
  mt_set_gpio_out(GPIO_NFC_VEN_PIN, GPIO_OUT_ZERO);

	/* Hold Time of 10 ms before sending data to MicroRead */
	p_device->sResetTimer.expires = jiffies + msecs_to_jiffies(10);
	p_device->sResetTimer.data = (unsigned long) 0;
	p_device->sResetTimer.function = open_nfc_reset_timeout;

	add_timer(&p_device->sResetTimer);

	p_device->reset_pending++;

	/* Discard the received data since it arrived before the reset */
	p_device->rx_ready = 0;
	p_device->rx_rcvnext = 0;
      out:
	mutex_unlock(&p_device->mutex);

	return rc;
#endif /* GPIO_NFC_WAKEUP */
}

/**
  * Process the poll()
  *
  * @return the poll status
  */

unsigned int
open_nfc_custom_poll(struct file *filp, poll_table * wait)
{
	struct open_nfc_custom_device *p_device = filp->private_data;
	unsigned int    mask = 0;

	ENTER();
	
	if (!p_device) {
		printk("open_nfc_custom_poll: Internal error p_device is missing\n");
		return -ENODEV;
	}

	/* We accept the function be called in all states */

	poll_wait(filp, &p_device->read_queue, wait);
	poll_wait(filp, &p_device->reset_queue, wait);

	mutex_lock(&p_device->mutex);

	if (p_device->reset_pending == 0) {
		mask = POLLOUT | POLLWRNORM;
	}

	if (p_device->rx_rcvnext > p_device->rx_ready) {
		mask |= POLLIN | POLLRDNORM;
	}

	mutex_unlock(&p_device->mutex);

	return mask;
}

/**
  * Function called when the user closes file
  *
  * @return 0 on success, a negative value on failure.
  */
int
open_nfc_custom_release(struct inode *inode, struct file *filp)
{
	struct open_nfc_custom_device *p_device = filp->private_data;

	ENTER();
	
	if (!p_device) {
		printk("open_nfc_custom_release: Internal error p_device is missing\n");
		return -ENODEV;
	}

	mutex_lock(&p_device->mutex);

	opennfc_struct_cleanup(p_device);	/* Go back to PROBED state */

#ifndef WAKE_AND_PURGE_I2C_ON_LOAD
//	disable_irq(p_device->irqout);
    mt65xx_eint_mask(CUST_EINT_NFC_NUM);
#endif /* WAKE_AND_PURGE_I2C_ON_LOAD */

	mutex_unlock(&p_device->mutex);

	return 0;
}

/* When I2C readings are failing too much, we delay the next reading by masking the IRQ */
static void
open_nfc_enable_irq(unsigned long arg) {
	ENTER();
	
//	enable_irq(arg);
  mt65xx_eint_unmask(CUST_EINT_NFC_NUM);
	
}


/**
  * Perform an I2C Read cycle, upon IRQOUT.
  *
  * @return void
  */
static void
open_nfc_irqout_read(struct open_nfc_custom_device *p_device)
{
	int             rc = 0, delay_irq = 0;
	unsigned char  *p_buffer;
	static unsigned char dummy[I2C_READ_DATA_LENGTH];

	ENTER();
	
	/* Do NOT interrupt an outgoing WRITE cycle */
	mutex_lock(&p_device->mutex);

	if (!p_device->rx_listener) {
		/* Nobody opened /dev/nfcc, read to relax the MR but discard data */
		p_buffer = dummy;	/* Read to have IRQOUT acknowledged, but discard the DATA to dummy buffer */
	} else {
		if (p_device->rx_rcvnext >= p_device->rx_ready + I2C_READ_NB_MSG)
			p_device->rx_ready = p_device->rx_rcvnext + 1 - I2C_READ_NB_MSG;	/* discard oldest message from the queue */

		p_buffer = p_device->rx_buffer[p_device->rx_rcvnext % I2C_READ_NB_MSG];
	}

  memset(p_buffer, 0, I2C_READ_DATA_LENGTH);

	rc = i2c_master_recv(p_device->i2c_client, p_buffer, I2C_READ_DATA_LENGTH);

  printk("2c_master_recv() => %d [ 0x%2x 0x%2x 0x%2x 0x%2x ... ]\n", rc, p_buffer[0], p_buffer[1], p_buffer[2], p_buffer[3]);

	if (rc != I2C_READ_DATA_LENGTH) {
		printk("open_nfc_irqout_read failed (returns %d) - #%d\n", rc, p_device->rx_failures);
		
		if (p_device->rx_failures++ > 10) {
			/* Delay before next attempt at reading */
			p_device->rx_timer.expires = jiffies + msecs_to_jiffies(1000);
			p_device->rx_timer.data = 0; /*(unsigned long) p_device->irqout;*/
			p_device->rx_timer.function = open_nfc_enable_irq;

			add_timer(&p_device->rx_timer);
			
			delay_irq = 1;
		}
	} else {
		p_device->rx_failures = 0;
		if (p_buffer[0] > I2C_READ_DATA_LENGTH - 2) {
			printk(KERN_ERR "open_nfc_irqout_read ERROR: invalid length read from I2C Device, message discarded\n");
		} else {
			if (p_device->rx_listener) {
				p_device->rx_rcvnext++;
				/* wake up poll() */
				wake_up(&p_device->read_queue);
			}
		}
	}

	mutex_unlock(&p_device->mutex);
	
	/* Re-enable the interrupt */
	if (!delay_irq) {
//		enable_irq(p_device->irqout);
    mt65xx_eint_unmask(CUST_EINT_NFC_NUM);
	}
}

/**
  * Processing, after IRQOUT processing in bottom-half.
  * Re-enable the LEVEL-sensitive IRQOUT.
  *
  * @return void
  */
static void
open_nfc_irqout_worker(struct work_struct *work)
{
	struct open_nfc_custom_device *p_device = open_nfc_p_device;

	ENTER();
	
	if (!p_device) {
		printk("open_nfc_irqout_worker: Internal error p_device is missing, disabling the IRQ\n");
		return;
	}

	/* Process the IRQOUT event (will re-eanble the IRQ) */
	open_nfc_irqout_read(p_device);

}

/**
  * Reset processing
  * 
  *
  * @return void
  */
static void
open_nfc_reset_worker(struct work_struct *work)
{
	struct open_nfc_custom_device *p_device = open_nfc_p_device;

	ENTER();
	
	mutex_lock(&p_device->mutex);

	p_device->reset_pending--;

	if (p_device->reset_pending == 0) {
		wake_up(&p_device->reset_queue);
	}
	mutex_unlock(&p_device->mutex);
}

/**
  * IRQOUT Interrupt handler.
  * Schedules the bottom-half.
  *
  * @return the IRQ processing status
  */

static          irqreturn_t
open_nfc_i2c_interrupt(int irq, void *dev_id)
{
	struct open_nfc_custom_device *p_device = open_nfc_p_device;

	ENTER();
	
//	disable_irq_nosync(irq);
    mt65xx_eint_mask(CUST_EINT_NFC_NUM);
	
	if (!p_device) {
		printk("open_nfc_i2c_interrupt: Internal error p_device is missing, disabing the IRQ\n");
	} else {
		/* continue the processing outside the irq handler */
		schedule_work(&p_device->irqout_event_work);
	}

	return IRQ_HANDLED;
}

/**
  * Device/Driver binding: probe
  *
  * @return 0 if successfull, or error code
  */
static int
opennfc_probe(struct i2c_client *client, const struct i2c_device_id *idp)
{
	int             rc = 0;
	struct open_nfc_custom_device *p_device = open_nfc_p_device;

	ENTER();
	
	if (!p_device) {
		printk("opennfc_probe: Internal error p_device is missing\n");
		return -ENODEV;
	}

	mutex_lock(&p_device->mutex);

	if (p_device->state != CUSTOM_INIT) {
		rc = EEXIST;
		goto out;
	}

	p_device->i2c_client = client;
	i2c_set_clientdata(client, p_device);

#if 0
	rc = request_irq(p_device->irqout, open_nfc_i2c_interrupt, IRQF_TRIGGER_HIGH, "IRQOUT_input", p_device);
	if (rc < 0) {
		printk(KERN_ERR "opennfc_probe : failed to register IRQOUT\n");
		goto out;
	}
#endif
	
	/* The IRQ can be trigged here at most once, because we are holding the mutex (cannot be re-enabled after disable) */
	
#ifndef WAKE_AND_PURGE_I2C_ON_LOAD
//	disable_irq(p_device->irqout);
    mt65xx_eint_mask(CUST_EINT_NFC_NUM);
#endif /* WAKE_AND_PURGE_I2C_ON_LOAD */

	p_device->state = CUSTOM_PROBED;
	
      out:
	mutex_unlock(&p_device->mutex);

		
	return rc;
}

/**
  * Device/Driver binding: remove
  *
  * @return 0 if successfull, or error code
  */
static int
opennfc_remove(struct i2c_client *client)
{
	struct open_nfc_custom_device *p_device;

	ENTER();
	
	p_device = open_nfc_p_device;

	if (p_device && (p_device->state >= CUSTOM_PROBED)) {
		i2c_set_clientdata(p_device->i2c_client, NULL);
	}

	return 0;
}

/* 
   Client structure holds device-specific information like the 
   driver model device node, and its I2C address 
*/
static const struct i2c_device_id opennfc_id[] = {
	{ I2C_ID_NAME, I2C_DEVICE_ADDR	}, 
	{ }
};

MODULE_DEVICE_TABLE(i2c, opennfc_id);

static struct i2c_driver open_nfc_i2c_driver = {
	.driver = {
		   .owner = THIS_MODULE,
		   .name = I2C_ID_NAME,
	},
	.probe = opennfc_probe,
	.remove = __devexit_p(opennfc_remove),
	.id_table = opennfc_id,
};

/* for auto-loading */
MODULE_ALIAS(I2C_ID_NAME);	


/**
  * Specific initialization, when driver module is inserted.
  *
  *  - Request control of the GPIOs
  *  - Initialize the I2C driver & IRQ
  *  - enable the irq if REFIOH is not available (so that device is not stuck on emission)
  *  - Set REFIOH high if I2C_PROBE_NEEDS_REFIOH (lowered by probe cb), low otherwise
  *  
  * @return 0 if successfull, or error code
  */
int
open_nfc_custom_init(void)
{
	int             retval = 0;

	struct open_nfc_custom_device *p_device;

	ENTER();
	
	p_device = kmalloc(sizeof(struct open_nfc_custom_device), GFP_KERNEL);
	if (p_device == NULL) {
		return -ENOMEM;
	}
	memset(p_device, 0, sizeof(struct open_nfc_custom_device));

	init_waitqueue_head(&p_device->read_queue);
	init_waitqueue_head(&p_device->reset_queue);
	INIT_WORK(&p_device->irqout_event_work, open_nfc_irqout_worker);
	INIT_WORK(&p_device->reset_event_work, open_nfc_reset_worker);

	mutex_init(&p_device->mutex);

	init_timer(&p_device->sResetTimer);
	init_timer(&p_device->rx_timer);
	
    // Initializing the GPIO209 as clock out of 32K
    mt_set_gpio_mode(GPIO209, 3);   //set to CLKM2
    mt_set_gpio_dir(GPIO209, 1);     //set to output
    mt_set_clock_output(2 , 2 , 1); //set to mode3, clocksrc:32K, divider:1

	/* Initializing the IRQ for the chipset */

   /* irq_gpio setup */
	mt_set_gpio_mode(GPIO_NFC_EINT_PIN, GPIO_NFC_EINT_PIN_M_EINT);
	mt_set_gpio_dir(GPIO_NFC_EINT_PIN, GPIO_DIR_IN);
	mt_set_gpio_pull_enable(GPIO_NFC_EINT_PIN, GPIO_PULL_ENABLE);
	mt_set_gpio_pull_select(GPIO_NFC_EINT_PIN, GPIO_PULL_DOWN);

	/* ven_gpio setup */
	mt_set_gpio_mode(GPIO_NFC_VEN_PIN, GPIO_NFC_VEN_PIN_M_GPIO);
	mt_set_gpio_dir(GPIO_NFC_VEN_PIN, GPIO_DIR_OUT);

	mt65xx_eint_set_sens(CUST_EINT_NFC_NUM, CUST_EINT_NFC_SENSITIVE);
	mt65xx_eint_set_hw_debounce(CUST_EINT_NFC_NUM, CUST_EINT_NFC_DEBOUNCE_CN);
	mt65xx_eint_registration(CUST_EINT_NFC_NUM, CUST_EINT_NFC_DEBOUNCE_EN, CUST_EINT_NFC_POLARITY, open_nfc_i2c_interrupt, 0);
	//mt65xx_eint_unmask(CUST_EINT_NFC_NUM);
	
	/* Initializing the WAKE_UP for the chipset */
#ifdef WAKE_AND_PURGE_I2C_ON_LOAD
	/* Wake up the chipset now */
  mt_set_gpio_out(GPIO_NFC_VEN_PIN, GPIO_OUT_ONE);
	udelay(2);
  mt_set_gpio_out(GPIO_NFC_VEN_PIN, GPIO_OUT_ZERO);
#endif /* WAKE_AND_PURGE_I2C_ON_LOAD */
	

	/* I2C Driver connection */
    i2c_register_board_info(NFC_I2C_BUSNUM, &nfc_board_info, 1);

	/* Save device context (needed in .probe()) */
	open_nfc_p_device = p_device;

	/* this might be platform-dependent */
	retval = i2c_add_driver(&open_nfc_i2c_driver);
	if (retval < 0) {
		printk(KERN_ERR "open_nfc_custom_open : failed to add I2C driver\n");
		goto end;
	}

	printk(KERN_ERR "Open NFC driver loaded\n");

      end:
	return retval;
}

/**
  * Specific cleanup, when driver module is removed.
  *
  * @return void
  */
void
open_nfc_custom_exit(void)
{
	struct open_nfc_custom_device *p_device;

	ENTER();
	
	p_device = open_nfc_p_device;
	if (p_device == NULL)
		return;

	/* Go back to PROBED state if needed */
	opennfc_struct_cleanup(p_device);

	/* Remove the IRQ handler */
//	free_irq(p_device->irqout, p_device);
    mt65xx_eint_mask(CUST_EINT_NFC_NUM);

	/* stop any pending work */
	cancel_work_sync(&p_device->irqout_event_work);

	i2c_del_driver(&open_nfc_i2c_driver);

	/* release the other GPIO */
  gpio_free(GPIO_NFC_VEN_PIN);
  gpio_free(GPIO_NFC_EINT_PIN);

	del_timer(&p_device->sResetTimer);
	del_timer(&p_device->rx_timer);
	mutex_destroy(&p_device->mutex);

	/* free the custom device context */
	kfree(p_device);

	open_nfc_p_device = NULL;

}

/* EOF */
