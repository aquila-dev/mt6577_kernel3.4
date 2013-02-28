#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>

#include "kd_camera_hw.h"

#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_camera_feature.h"

/******************************************************************************
 * Debug configuration
******************************************************************************/
#define PFX "[kd_camera_hw]"
#define PK_DBG_NONE(fmt, arg...)    do {} while (0)
#define PK_DBG_FUNC(fmt, arg...)    printk(KERN_INFO PFX "%s: " fmt, __FUNCTION__ ,##arg)

#define DEBUG_CAMERA_HW_K
#ifdef DEBUG_CAMERA_HW_K
#define PK_DBG PK_DBG_FUNC
#define PK_ERR(fmt, arg...)         printk(KERN_ERR PFX "%s: " fmt, __FUNCTION__ ,##arg)
#else
#define PK_DBG(a,...)
#define PK_ERR(a,...)
#endif

static u32 pinSetIdx = 0;//default main sensor
#define IDX_PS_CMRST 0
#define IDX_PS_CMPDN 4

#define IDX_PS_MODE 1
#define IDX_PS_ON   2
#define IDX_PS_OFF  3

#define GPIO_CAMERA_AF_EN_PIN  0xff
#define GPIO_CAMERA_LDO_EN_PIN 0xff
#define GPIO_CAMERA_LDO_EN_PIN_M_GPIO 0xff

static u32 pinSet[2][8] = {
                    //for main sensor 
                    {   
                        GPIO_CAMERA_CMRST_PIN,
                        GPIO_CAMERA_CMRST_PIN_M_GPIO,   /* mode */
                        GPIO_OUT_ONE,                   /* ON state */
                        GPIO_OUT_ZERO,                  /* OFF state */
                        GPIO_CAMERA_CMPDN_PIN,
                        GPIO_CAMERA_CMPDN_PIN_M_GPIO,
			GPIO_OUT_ONE,
			GPIO_OUT_ZERO,     
                        
                    },
                    //for sub sensor 
                    {
                        GPIO_CAMERA_CMRST1_PIN,
                        GPIO_CAMERA_CMRST1_PIN_M_GPIO,
                        GPIO_OUT_ONE,
                        GPIO_OUT_ZERO,
                        GPIO_CAMERA_CMPDN1_PIN,
                        GPIO_CAMERA_CMPDN1_PIN_M_GPIO,
                        GPIO_OUT_ZERO,
                        GPIO_OUT_ONE,
                    }
                };
 
 static void Rst_PDN_Init(void)
{
    if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE]))
    {
        PK_DBG("[CAMERA LENS] set gpio mode failed!! \n");
    }

    if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN],GPIO_DIR_OUT))
    {
        PK_DBG("[CAMERA LENS] set gpio dir failed!! \n");
    }

    if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_MODE]))
    {
        PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");
    }

    if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST],GPIO_DIR_OUT))
    {
        PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");
    }
}
              
static void disable_inactive_sensor(void)
{
    //disable inactive sensor
    if (GPIO_CAMERA_INVALID != pinSet[1-pinSetIdx][IDX_PS_CMRST]) {
        if(mt_set_gpio_out(pinSet[1-pinSetIdx][IDX_PS_CMRST],pinSet[1-pinSetIdx][IDX_PS_CMRST+IDX_PS_OFF]))
        {
            PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");
        } //low == reset sensor
        
        if(mt_set_gpio_out(pinSet[1-pinSetIdx][IDX_PS_CMPDN],pinSet[1-pinSetIdx][IDX_PS_CMPDN+IDX_PS_OFF]))
        {
            PK_DBG("[CAMERA LENS] set gpio failed!! \n");
        } //high == power down lens module
    }        
}

static int kd_poweron_devices(MT65XX_POWER_VOLTAGE VOL_D2, MT65XX_POWER_VOLTAGE VOL_A, MT65XX_POWER_VOLTAGE VOL_D, MT65XX_POWER_VOLTAGE VOL_A2, char *mode_name)
{
    int ret = 0;

    if(VOL_D2 >= 0)
        ret = hwPowerOn(CAMERA_POWER_VCAM_D2, VOL_D2, mode_name);

    if(ret != TRUE){
        PK_DBG("[CAMERA SENSOR] Fail to enable digital power VCAM_D2\n");
        goto poweronerr0;
    }

    if(VOL_A > 0)
        ret = hwPowerOn(CAMERA_POWER_VCAM_A, VOL_A,mode_name);
    if(ret != TRUE){
        PK_DBG("[CAMERA SENSOR] Fail to enable digital power VCAM_A\n");
        goto poweronerr1;
    }

    if(VOL_D > 0)
        ret = hwPowerOn(CAMERA_POWER_VCAM_D, VOL_D,mode_name);
    if(ret != TRUE){
        PK_DBG("[CAMERA SENSOR] Fail to enable digital power VCAM_D\n");
        goto poweronerr2;
    }

    if(VOL_A2 > 0)
        ret = hwPowerOn(CAMERA_POWER_VCAM_A2, VOL_A2,mode_name);
    if(ret != TRUE){
        PK_DBG("[CAMERA SENSOR] Fail to enable digital power VCAM_A2\n");
        goto poweronerr3;
    }

poweronerr3:
poweronerr2:
poweronerr1:
poweronerr0:
    return ret;
}
static int kd_mt9p017mipi_poweron( char *mode_name)
{
    int ret;

    ret = kd_poweron_devices(VOL_1800, VOL_2800, VOL_1800, VOL_2800, mode_name);

#if 0
    if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_D2, VOL_1800,mode_name))
            {
                PK_DBG("[CAMERA SENSOR] Fail to enable digital power\n");
                //return -EIO;
                goto poweronerr;
            }


        if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_A, VOL_2800,mode_name))
        {
            PK_DBG("[CAMERA SENSOR] Fail to enable analog power\n");
                //return -EIO;
                goto poweronerr;
            }                    

         
        if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_D, VOL_2800,mode_name))     
        {
                 PK_DBG("[CAMERA SENSOR] Fail to enable digital power\n");
            //return -EIO;
            goto poweronerr;
        }
  
        
        if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_A2, VOL_2800,mode_name))
        {
            PK_DBG("[CAMERA SENSOR] Fail to enable analog power\n");
            //return -EIO;
            goto poweronerr;
        }        
#endif
        // wait power to be stable 
        mdelay(5); 

        disable_inactive_sensor();//disable inactive sensor
        if (GPIO_CAMERA_INVALID != pinSet[1-pinSetIdx][IDX_PS_CMRST]) {
            if(mt_set_gpio_mode(pinSet[1-pinSetIdx][IDX_PS_CMRST],pinSet[1-pinSetIdx][IDX_PS_CMRST+IDX_PS_MODE])){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
            if(mt_set_gpio_mode(pinSet[1-pinSetIdx][IDX_PS_CMPDN],pinSet[1-pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!! \n");}
            if(mt_set_gpio_dir(pinSet[1-pinSetIdx][IDX_PS_CMRST],GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
            if(mt_set_gpio_dir(pinSet[1-pinSetIdx][IDX_PS_CMPDN],GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! \n");}
            if(mt_set_gpio_out(pinSet[1-pinSetIdx][IDX_PS_CMRST],pinSet[1-pinSetIdx][IDX_PS_CMRST+IDX_PS_OFF])){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");} //low == reset sensor
            if(mt_set_gpio_out(pinSet[1-pinSetIdx][IDX_PS_CMPDN],pinSet[1-pinSetIdx][IDX_PS_CMPDN+IDX_PS_OFF])){PK_DBG("[CAMERA LENS] set gpio failed!! \n");} //high == power down lens module
            
        }        

        //enable active sensor
        //RST pin
        if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
            if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_MODE])){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
            if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST],GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
            if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_OFF])){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
            mdelay(10);
            if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_ON])){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
            mdelay(5);

            //PDN pin
            if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!! \n");}
            if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN],GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! \n");}
            if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_ON])){PK_DBG("[CAMERA LENS] set gpio failed!! \n");}
        }
poweronerr:
    return ret;

}
static int kd_mt9p017mipi_powerdown( char *mode_name)
{
    int ret;

    return ret;
}

static int kd_s5k4e1fxmipi_poweron( char *mode_name)
{
        int ret;

        //add by zym for s5k4e1 enable af 
        mt_set_gpio_out(220,1);
        ret = kd_poweron_devices(VOL_1800, VOL_2800, VOL_1800, VOL_2800, mode_name);
                #if 0
		mt_set_gpio_mode(GPIO_CAMERA_CMRST1_PIN,GPIO_CAMERA_CMRST1_PIN_M_GPIO);
		mt_set_gpio_dir(GPIO_CAMERA_CMRST1_PIN,GPIO_DIR_OUT);
		mt_set_gpio_mode(GPIO_CAMERA_CMPDN1_PIN,GPIO_CAMERA_CMPDN1_PIN_M_GPIO);
		mt_set_gpio_dir(GPIO_CAMERA_CMPDN1_PIN,GPIO_DIR_OUT);

		mt_set_gpio_out(GPIO_CAMERA_CMRST1_PIN,GPIO_OUT_ZERO);
		mt_set_gpio_out(GPIO_CAMERA_CMPDN1_PIN,GPIO_OUT_ZERO);
		
		/////////////////////
		mt_set_gpio_mode(GPIO_CAMERA_CMRST_PIN,GPIO_CAMERA_CMRST_PIN_M_GPIO);
		mt_set_gpio_dir(GPIO_CAMERA_CMRST_PIN,GPIO_DIR_OUT);
		mt_set_gpio_out(GPIO_CAMERA_CMRST_PIN,GPIO_OUT_ZERO);
		//mt_set_gpio_mode(GPIO_CAMERA_CMPDN_PIN,GPIO_CAMERA_CMPDN_PIN_M_GPIO);
		//mt_set_gpio_dir(GPIO_CAMERA_CMPDN_PIN,GPIO_DIR_OUT);
		//mt_set_gpio_out(GPIO_CAMERA_CMPDN_PIN,GPIO_OUT_ZERO);
		
		mdelay(1);  
                #endif

        #if 0
		if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_D, VOL_1800,mode_name))     
		{
		         PK_DBG("[CAMERA SENSOR] Fail to enable digital power\n");
		    //return -EIO;
		    goto _kdCISModulePowerOn_exit_;
		}
  
        	mdelay(1); 
		if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_A, VOL_2800,mode_name))
		{
		    PK_DBG("[CAMERA SENSOR] Fail to enable analog power\n");
		        //return -EIO;
		        goto _kdCISModulePowerOn_exit_;
		    }    

			mdelay(1); 
		
			 if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_D2, VOL_2800,mode_name))
		    {
		        PK_DBG("[CAMERA SENSOR] Fail to enable digital power\n");
		        //return -EIO;
		        goto _kdCISModulePowerOn_exit_;
		    }
			mdelay(1);
				
		if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_A2, VOL_2800,mode_name))
		{
		    PK_DBG("[CAMERA SENSOR] Fail to enable analog power\n");
		    //return -EIO;
		    goto _kdCISModulePowerOn_exit_;
		}        
		#endif
		// wait power to be stable 
		mdelay(5); 
		//enable active sensor
		//RST pin
		#if 0
		mt_set_gpio_out(GPIO_CAMERA_CMRST_PIN,GPIO_OUT_ONE);
			//PK_DBG("[CAMERA SENSOR] sofia PDN!!\n");
			//mt_set_gpio_out(GPIO_CAMERA_CMPDN_PIN,GPIO_OUT_ONE);
		mdelay(100); 
		//while(1){};
                #endif

        disable_inactive_sensor();//disable inactive sensor
        if (GPIO_CAMERA_INVALID != pinSet[1-pinSetIdx][IDX_PS_CMRST]) {
            if(mt_set_gpio_mode(pinSet[1-pinSetIdx][IDX_PS_CMRST],pinSet[1-pinSetIdx][IDX_PS_CMRST+IDX_PS_MODE])){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
            if(mt_set_gpio_mode(pinSet[1-pinSetIdx][IDX_PS_CMPDN],pinSet[1-pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!! \n");}
            if(mt_set_gpio_dir(pinSet[1-pinSetIdx][IDX_PS_CMRST],GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
            if(mt_set_gpio_dir(pinSet[1-pinSetIdx][IDX_PS_CMPDN],GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! \n");}
            if(mt_set_gpio_out(pinSet[1-pinSetIdx][IDX_PS_CMRST],pinSet[1-pinSetIdx][IDX_PS_CMRST+IDX_PS_OFF])){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");} //low == reset sensor
            if(mt_set_gpio_out(pinSet[1-pinSetIdx][IDX_PS_CMPDN],pinSet[1-pinSetIdx][IDX_PS_CMPDN+IDX_PS_OFF])){PK_DBG("[CAMERA LENS] set gpio failed!! \n");} //high == power down lens module
            
        }        

        //enable active sensor
        //RST pin
        if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
            if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_MODE])){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
            if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST],GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
            if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_OFF])){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
            mdelay(10);
            if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_ON])){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
            mdelay(1);
            //PDN pin
            if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!! \n");}
            if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN],GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! \n");}
            if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_ON])){PK_DBG("[CAMERA LENS] set gpio failed!! \n");}
        }
_kdCISModulePowerOn_exit_:
    return -EIO;

}
static int kd_s5k4e1fxmipi_powerdown( char *mode_name)
{
    int ret;
    mt_set_gpio_out(220,0);
    return ret;
}
// a51 add start
/****************************S5K5CA**********************************/

void kd_s5k5ca_gpio(int high_low)
{
    int ret;
    if(high_low)
    {
        if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN],GPIO_OUT_ZERO)){PK_DBG("[CAMERA LENS] set gpio failed!! \n");}

        if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST], GPIO_OUT_ONE)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
        mdelay(2);
        if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST], GPIO_OUT_ZERO)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
        mdelay(3);
        if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN],GPIO_OUT_ONE)){PK_DBG("[CAMERA LENS] set gpio failed!! \n");}
        mdelay(4);     
        if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST], GPIO_OUT_ONE)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
        mdelay(1);
    }
    else
    {
        if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN],GPIO_OUT_ZERO)){PK_DBG("[CAMERA LENS] set gpio failed!! \n");}

        if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST], GPIO_OUT_ONE)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
        mdelay(2);
        if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST], GPIO_OUT_ZERO)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
        mdelay(3);
        if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN],GPIO_OUT_ZERO)){PK_DBG("[CAMERA LENS] set gpio failed!! \n");}
        mdelay(4);     
        if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST], GPIO_OUT_ONE)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
        mdelay(1);

    }

}
EXPORT_SYMBOL(kd_s5k5ca_gpio);


static int kd_s5k5ca_poweron( char *mode_name)
{
    int ret;
   ret = hwPowerOn(CAMERA_POWER_VCAM_D2, VOL_2800,mode_name);
    if(ret != TRUE){
        PK_DBG("[CAMERA SENSOR] Fail to enable digital power VCAM_D2\n");
        goto poweronerr;
    }                    
    ret = hwPowerOn(CAMERA_POWER_VCAM_A, VOL_2800,mode_name);
    if(ret != TRUE){
        PK_DBG("[CAMERA SENSOR] Fail to enable digital power VCAM_A\n");
        goto poweronerr;
    }

    ret = hwPowerOn(CAMERA_POWER_VCAM_D, VOL_1800,mode_name);

    if(ret != TRUE){
        PK_DBG("[CAMERA SENSOR] Fail to enable digital power VCAM_D\n");
        goto poweronerr;
    }

    ret = hwPowerOn(CAMERA_POWER_VCAM_A2, VOL_2800,mode_name);
    if(ret != TRUE){
        PK_DBG("[CAMERA SENSOR] Fail to enable digital power VCAM_A2\n");
        goto poweronerr;
    }
   mdelay(5); 

        //disable inactive sensor
        if (GPIO_CAMERA_INVALID != pinSet[1-pinSetIdx][IDX_PS_CMRST]) {
            if(mt_set_gpio_mode(pinSet[1-pinSetIdx][IDX_PS_CMRST],pinSet[1-pinSetIdx][IDX_PS_CMRST+IDX_PS_MODE])){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
            if(mt_set_gpio_mode(pinSet[1-pinSetIdx][IDX_PS_CMPDN],pinSet[1-pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!! \n");}
            if(mt_set_gpio_dir(pinSet[1-pinSetIdx][IDX_PS_CMRST],GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
            if(mt_set_gpio_dir(pinSet[1-pinSetIdx][IDX_PS_CMPDN],GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! \n");}
            if(mt_set_gpio_out(pinSet[1-pinSetIdx][IDX_PS_CMRST],pinSet[1-pinSetIdx][IDX_PS_CMRST+IDX_PS_OFF])){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");} //low == reset sensor
            if(mt_set_gpio_out(pinSet[1-pinSetIdx][IDX_PS_CMPDN],pinSet[1-pinSetIdx][IDX_PS_CMPDN+IDX_PS_OFF])){PK_DBG("[CAMERA LENS] set gpio failed!! \n");} //high == power down lens module
            
        }        

        //enable active sensor
        //RST pin
        if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
            if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_MODE])){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
            if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST],GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
            if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_OFF])){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
            mdelay(10);
            if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_ON])){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
            mdelay(1);

            //PDN pin
            if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!! \n");}
            if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN],GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! \n");}
            if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_ON])){PK_DBG("[CAMERA LENS] set gpio failed!! \n");}
        }
poweronerr:
    return ret;

}
  


static int kd_s5k5ca_st_hq_poweron( char *mode_name)
{
    int ret;
#if 1
#if defined(HQ_PROJECT_A20)
	{
	int ver;
	extern int get_mb_version(void);
	ver = get_mb_version();
	if(ver == 2)
    	ret = kd_poweron_devices(VOL_1800, VOL_2800, VOL_1800, VOL_2800, mode_name);
	else
    	ret = kd_poweron_devices(VOL_2800, VOL_2800, VOL_1800, VOL_2800, mode_name);
	}
#else

	ret = kd_poweron_devices(VOL_1800, VOL_2800, VOL_1800, VOL_2800, mode_name);

#endif
#else
    /*ergate-016*/
#if defined(HQ_PROJECT_A20)
	{
		int ver;
		extern int get_mb_version(void);
		ver = get_mb_version();
		if(ver == 2)
    		ret = hwPowerOn(CAMERA_POWER_VCAM_D2, VOL_1800,mode_name);
		else
			ret = hwPowerOn(CAMERA_POWER_VCAM_D2, VOL_2800,mode_name);
	}
#else
	ret = hwPowerOn(CAMERA_POWER_VCAM_D2, VOL_2800,mode_name);
#endif

    if(ret != TRUE){
        PK_DBG("[CAMERA SENSOR] Fail to enable digital power VCAM_D2\n");
        goto poweronerr;
    }                    
    ret = hwPowerOn(CAMERA_POWER_VCAM_A, VOL_2800,mode_name);
    if(ret != TRUE){
        PK_DBG("[CAMERA SENSOR] Fail to enable digital power VCAM_A\n");
        goto poweronerr;
    }

    ret = hwPowerOn(CAMERA_POWER_VCAM_D, VOL_1500,mode_name);
    if(ret != TRUE){
        PK_DBG("[CAMERA SENSOR] Fail to enable digital power VCAM_D\n");
        goto poweronerr;
    }

    ret = hwPowerOn(CAMERA_POWER_VCAM_A2, VOL_2800,mode_name);
    if(ret != TRUE){
        PK_DBG("[CAMERA SENSOR] Fail to enable digital power VCAM_A2\n");
        goto poweronerr;
    }
#endif
    mdelay(5);// wait power to be stable  
    disable_inactive_sensor();//disable inactive sensor
    if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
        //PDN pin
        if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN],GPIO_OUT_ZERO)){PK_DBG("[CAMERA LENS] set gpio failed!! \n");}
     
        if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST], GPIO_OUT_ONE)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
        mdelay(2);
        if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST], GPIO_OUT_ZERO)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
        mdelay(3);
 if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN],GPIO_OUT_ZERO)){PK_DBG("[CAMERA LENS] set gpio failed!! \n");}
        mdelay(4);     
        if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST], GPIO_OUT_ONE)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
        mdelay(1);
    }
poweronerr:
    return ret;

}


static int kd_s5k5ca_powerdown( char *mode_name)
{
    int ret;
    // todo
    return ret;
}
/////////////////////////////////////////////////////


static int kd_s5k4ecgx_poweron( char *mode_name)
{
    int ret;
   

    //ret = hwPowerOn(CAMERA_POWER_VCAM_A, VOL_2800,mode_name);
    ret = hwPowerOn(CAMERA_POWER_VCAM_D, VOL_1200,mode_name);
    if(ret != TRUE){
        PK_DBG("[CAMERA SENSOR] Fail to enable digital power VCAM_D\n");
        goto poweronerr;
    }
    mdelay(5);

    //ret = hwPowerOn(CAMERA_POWER_VCAM_D, VOL_1200,mode_name);
    ret = hwPowerOn(CAMERA_POWER_VCAM_A, VOL_2800,mode_name);
    if(ret != TRUE){
        PK_DBG("[CAMERA SENSOR] Fail to enable digital power VCAM_A\n");
        goto poweronerr;
    }
    
    mdelay(5); 
     ret = hwPowerOn(CAMERA_POWER_VCAM_D2, VOL_2800,mode_name);
    if(ret != TRUE){
        PK_DBG("[CAMERA SENSOR] Fail to enable digital power VCAM_D2\n");
        goto poweronerr;
    }     


    ret = hwPowerOn(CAMERA_POWER_VCAM_A2, VOL_2800,mode_name);
    if(ret != TRUE){
        PK_DBG("[CAMERA SENSOR] Fail to enable digital power VCAM_A2\n");
        goto poweronerr;
    }

   mdelay(5); 

        //disable inactive sensor
        if (GPIO_CAMERA_INVALID != pinSet[1-pinSetIdx][IDX_PS_CMRST]) {
            if(mt_set_gpio_mode(pinSet[1-pinSetIdx][IDX_PS_CMRST],pinSet[1-pinSetIdx][IDX_PS_CMRST+IDX_PS_MODE])){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
            if(mt_set_gpio_mode(pinSet[1-pinSetIdx][IDX_PS_CMPDN],pinSet[1-pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!! \n");}
            if(mt_set_gpio_dir(pinSet[1-pinSetIdx][IDX_PS_CMRST],GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
            if(mt_set_gpio_dir(pinSet[1-pinSetIdx][IDX_PS_CMPDN],GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! \n");}
            if(mt_set_gpio_out(pinSet[1-pinSetIdx][IDX_PS_CMRST],pinSet[1-pinSetIdx][IDX_PS_CMRST+IDX_PS_OFF])){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");} //low == reset sensor
            if(mt_set_gpio_out(pinSet[1-pinSetIdx][IDX_PS_CMPDN],pinSet[1-pinSetIdx][IDX_PS_CMPDN+IDX_PS_OFF])){PK_DBG("[CAMERA LENS] set gpio failed!! \n");} //high == power down lens module
            
        }        

        //enable active sensor
        //RST pin
        if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
            if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_MODE])){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
            if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST],GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
            if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_OFF])){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
            mdelay(10);
            if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_ON])){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
            mdelay(1);

            //PDN pin
            if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!! \n");}
            if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN],GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! \n");}
            if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_ON])){PK_DBG("[CAMERA LENS] set gpio failed!! \n");}
        }
poweronerr:
    return ret;

}

static int kd_s5k4ecgx_powerdown( char *mode_name)
{
    int ret;
    // todo
    return ret;
}


static int kd_ov5647_poweron( char *mode_name)
{
    int ret;
    printk("<6>OV5647 kd_ov5647_poweron start..\n");

    ret = hwPowerOn(CAMERA_POWER_VCAM_D2, VOL_2800,mode_name);
    if(ret != TRUE){
        PK_DBG("[CAMERA SENSOR] Fail to enable digital power VCAM_D2\n");
        goto poweronerr;
    }    
    mdelay(1);    
        
    ret = hwPowerOn(CAMERA_POWER_VCAM_A, VOL_2800,mode_name);
    if(ret != TRUE){
        PK_DBG("[CAMERA SENSOR] Fail to enable digital power VCAM_A\n");
        goto poweronerr;
    }
    mdelay(1);

    ret = hwPowerOn(CAMERA_POWER_VCAM_D, VOL_1500,mode_name);
    if(ret != TRUE){
        PK_DBG("[CAMERA SENSOR] Fail to enable digital power VCAM_D\n");
        goto poweronerr;
    }

    ret = hwPowerOn(CAMERA_POWER_VCAM_A2, VOL_2800,mode_name);
    if(ret != TRUE){
        PK_DBG("[CAMERA SENSOR] Fail to enable digital power VCAM_A2\n");
        goto poweronerr;
    }    
    mdelay(10);// wait power to be stable  >5ms

        //disable inactive sensor
        if (GPIO_CAMERA_INVALID != pinSet[1-pinSetIdx][IDX_PS_CMRST]) {
            if(mt_set_gpio_mode(pinSet[1-pinSetIdx][IDX_PS_CMRST],pinSet[1-pinSetIdx][IDX_PS_CMRST+IDX_PS_MODE])){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
            if(mt_set_gpio_mode(pinSet[1-pinSetIdx][IDX_PS_CMPDN],pinSet[1-pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!! \n");}
            if(mt_set_gpio_dir(pinSet[1-pinSetIdx][IDX_PS_CMRST],GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
            if(mt_set_gpio_dir(pinSet[1-pinSetIdx][IDX_PS_CMPDN],GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! \n");}
            if(mt_set_gpio_out(pinSet[1-pinSetIdx][IDX_PS_CMRST],pinSet[1-pinSetIdx][IDX_PS_CMRST+IDX_PS_OFF])){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");} //low == reset sensor
            if(mt_set_gpio_out(pinSet[1-pinSetIdx][IDX_PS_CMPDN],pinSet[1-pinSetIdx][IDX_PS_CMPDN+IDX_PS_OFF])){PK_DBG("[CAMERA LENS] set gpio failed!! \n");} //high == power down lens module
            
        }        

        //enable active sensor
        //RST pin
        if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
            if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_MODE])){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
            if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST],GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
            if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_OFF])){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
            mdelay(10);
            if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_ON])){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
            mdelay(1);

            //PDN pin
            if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!! \n");}
            if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN],GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! \n");}
            if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_OFF])){PK_DBG("[CAMERA LENS] set gpio failed!! \n");}
        }

poweronerr:
    return ret;
}

static int kd_ov5647_powerdown( char *mode_name)
{
    int ret;
    // todo
    return ret;
}

/////////////////////////////////////////////////////
// a51 add end
/****************************HI704**********************************/

static int kd_hi704yuv_poweron( char *mode_name)
{
    int ret;


    	ret = kd_poweron_devices(VOL_1800, VOL_2800, VOL_1800, /*VOL_2800*/0, mode_name);
#if 0
	ret = hwPowerOn(CAMERA_POWER_VCAM_D2, VOL_1800,mode_name);
	
	if(ret != TRUE){
        PK_DBG("[CAMERA SENSOR] Fail to enable digital power VCAM_D2\n");
        goto poweronerr;
    	}                    
    	ret = hwPowerOn(CAMERA_POWER_VCAM_A, VOL_2800,mode_name);
    	if(ret != TRUE){
        PK_DBG("[CAMERA SENSOR] Fail to enable digital power VCAM_A\n");
        goto poweronerr;
   	 }

    ret = hwPowerOn(CAMERA_POWER_VCAM_D, VOL_1800,mode_name);
    if(ret != TRUE){
        PK_DBG("[CAMERA SENSOR] Fail to enable digital power VCAM_D\n");
        goto poweronerr;
    }

	/*ergate-033*/
	if(pinSetIdx == 1)
	{
    	ret = hwPowerOn(CAMERA_POWER_VCAM_A2, VOL_2800,mode_name);
	}
    if(ret != TRUE){
        PK_DBG("[CAMERA SENSOR] Fail to enable digital power VCAM_A2\n");
        goto poweronerr;
	}
#endif
	
   	mdelay(5);// wait power to be stable  
    disable_inactive_sensor();//disable inactive sensor
	int pinSetIdx = 1;
    if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
        //PDN pin
        if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_ON])){PK_DBG("[CAMERA LENS] set gpio failed!! \n");}
     
        if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_ON])){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
        if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_OFF])){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
        mdelay(10);
        if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_ON])){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
        mdelay(5);
    }
poweronerr:
    return ret;


}
static int kd_hi704yuv_powerdown( char *mode_name)
{
    int ret;

    return ret;
}
/****************************HI704**********************************/ 
static int kd_gc0329yuv_poweron( char *mode_name)
{
    int ret;


	
    	ret = kd_poweron_devices(VOL_1800, VOL_2800, VOL_1800,0/*VOL_2800*/, mode_name);
	

    /*ergate-008*/
#if 0 	
/*ergate-016*/
#if defined(HQ_PROJECT_A52)
	
		ret = hwPowerOn(CAMERA_POWER_VCAM_D2, VOL_2800,mode_name);
#else
	ret = hwPowerOn(CAMERA_POWER_VCAM_D2, VOL_2800,mode_name);
#endif

    if(ret != TRUE){
        PK_DBG("[CAMERA SENSOR] Fail to enable digital power VCAM_D2\n");
        goto poweronerr;
    }                    
    ret = hwPowerOn(CAMERA_POWER_VCAM_A, VOL_2800,mode_name);
    if(ret != TRUE){
        PK_DBG("[CAMERA SENSOR] Fail to enable digital power VCAM_A\n");
        goto poweronerr;
    }

    ret = hwPowerDown(CAMERA_POWER_VCAM_D, mode_name);
    if(ret != TRUE){
        PK_DBG("[CAMERA SENSOR] Fail to disable digital power VCAM_D\n");
        goto poweronerr;
    }

	/*ergate-033*/
    //ret = hwPowerOn(CAMERA_POWER_VCAM_A2, VOL_2800,mode_name);
    if(ret != TRUE){
        PK_DBG("[CAMERA SENSOR] Fail to enable digital power VCAM_A2\n");
        goto poweronerr;
    }
#endif 
    mdelay(5);// wait power to be stable  
    disable_inactive_sensor();//disable inactive sensor
   
	int pinSetIdx = 1;
    if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
        //PDN pin
        if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_ON])){PK_DBG("[CAMERA LENS] set gpio failed!! \n");}
     
        if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_ON])){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
        if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_OFF])){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
        mdelay(10);
        if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_ON])){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
        mdelay(5);
    }
poweronerr:
    return ret;

}
static int kd_gc0329yuv_powerdown( char *mode_name)
{
    int ret;

    return ret;
}

/****************************OV3660**********************************/

static int kd_ov3660_poweron( char *mode_name)
{
    int ret;

    ret = hwPowerOn(CAMERA_POWER_VCAM_D, VOL_1500,mode_name);

    if(ret != TRUE){
        PK_DBG("[CAMERA SENSOR] Fail to enable digital power VCAM_D2\n");
        goto poweronerr;
    }                    
    ret = hwPowerOn(CAMERA_POWER_VCAM_A, VOL_2800,mode_name);
    if(ret != TRUE){
        PK_DBG("[CAMERA SENSOR] Fail to enable digital power VCAM_A\n");
        goto poweronerr;
    }
#if 1
	ret = hwPowerOn(CAMERA_POWER_VCAM_D2, VOL_1800,mode_name);
	if(ret != TRUE){
		PK_DBG("[CAMERA SENSOR] Fail to enable digital power VCAM_D\n");
		goto poweronerr;
	}
#endif
	
    mdelay(5);// wait power to be stable  
    disable_inactive_sensor();//disable inactive sensor
    if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {

         if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN],GPIO_OUT_ZERO)){PK_DBG("[CAMERA LENS] set gpio failed!! \n");}
  
        if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST], GPIO_OUT_ZERO)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
        mdelay(10);// wait power to be stable  
        if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST], GPIO_OUT_ONE)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
        mdelay(1);// wait power to be stable  
         if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN],GPIO_OUT_ZERO)){PK_DBG("[CAMERA LENS] set gpio failed!! \n");}
    }
poweronerr:
    return ret;
}
static int kd_ov3660_powerdown( char *mode_name)
{
    int ret;
    // todo
    return ret;
}
/****************************OV3660**********************************/


// ynn add start 
/****************************OV3640**********************************/

static int kd_ov3640_poweron( char *mode_name)
{
    int ret;

    ret = hwPowerOn(CAMERA_POWER_VCAM_D, VOL_1500,mode_name);

    if(ret != TRUE){
        PK_DBG("[CAMERA SENSOR] Fail to enable digital power VCAM_D2\n");
        goto poweronerr;
    }                    
    ret = hwPowerOn(CAMERA_POWER_VCAM_A, VOL_2800,mode_name);
    if(ret != TRUE){
        PK_DBG("[CAMERA SENSOR] Fail to enable digital power VCAM_A\n");
        goto poweronerr;
    }
#if 1
    ret = hwPowerOn(CAMERA_POWER_VCAM_D2, VOL_1800,mode_name);
    if(ret != TRUE){
        PK_DBG("[CAMERA SENSOR] Fail to enable digital power VCAM_D\n");
        goto poweronerr;
    }
#endif
    ret = hwPowerOn(CAMERA_POWER_VCAM_A2, VOL_2800,mode_name);
    if(ret != TRUE){
        PK_DBG("[CAMERA SENSOR] Fail to enable digital power VCAM_A2\n");
        goto poweronerr;
    }
    
    mdelay(5);// wait power to be stable  
    disable_inactive_sensor();//disable inactive sensor
    if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {

         if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN],GPIO_OUT_ZERO)){PK_DBG("[CAMERA LENS] set gpio failed!! \n");}
  
        if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST], GPIO_OUT_ZERO)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
        mdelay(10);// wait power to be stable  
        if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST], GPIO_OUT_ONE)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
        mdelay(1);// wait power to be stable  
         if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN],GPIO_OUT_ZERO)){PK_DBG("[CAMERA LENS] set gpio failed!! \n");}
    }
poweronerr:
    return ret;
}
static int kd_ov3640_powerdown( char *mode_name)
{
    int ret;
    // todo
    return ret;
}
/****************************OV3640**********************************/


// ynn add end
#if defined(MT6577)


int kdCISModulePowerOn(CAMERA_DUAL_CAMERA_SENSOR_ENUM SensorIdx, char *currSensorName, BOOL On, char* mode_name)
{
int ret = 0;
#if 0
u32 pinSetIdx = 0;//default main sensor

#define IDX_PS_CMRST 0
#define IDX_PS_CMPDN 4

#define IDX_PS_MODE 1
#define IDX_PS_ON   2
#define IDX_PS_OFF  3
u32 pinSet[3][8] = {
                    //for main sensor 
                    {GPIO_CAMERA_CMRST_PIN,
                        GPIO_CAMERA_CMRST_PIN_M_GPIO,   /* mode */
                        GPIO_OUT_ONE,                   /* ON state */
                        GPIO_OUT_ZERO,                  /* OFF state */
                     GPIO_CAMERA_CMPDN_PIN,
                        GPIO_CAMERA_CMPDN_PIN_M_GPIO,
                        GPIO_OUT_ZERO,
                        GPIO_OUT_ONE,
                    },
                    //for sub sensor 
                    {GPIO_CAMERA_CMRST1_PIN,
                     GPIO_CAMERA_CMRST1_PIN_M_GPIO,
                        GPIO_OUT_ONE,
                        GPIO_OUT_ZERO,
                     GPIO_CAMERA_CMPDN1_PIN,
                        GPIO_CAMERA_CMPDN1_PIN_M_GPIO,
                        GPIO_OUT_ZERO,
                        GPIO_OUT_ONE,
                    },
                    //for main_2 sensor 
                    {GPIO_CAMERA_2_CMRST_PIN,
                        GPIO_CAMERA_2_CMRST_PIN_M_GPIO,   /* mode */
                        GPIO_OUT_ONE,                   /* ON state */
                        GPIO_OUT_ZERO,                  /* OFF state */
                     GPIO_CAMERA_2_CMPDN_PIN,
                        GPIO_CAMERA_2_CMPDN_PIN_M_GPIO,
                        GPIO_OUT_ZERO,
                        GPIO_OUT_ONE,
                    }
                   };
#endif
    if (DUAL_CAMERA_MAIN_SENSOR == SensorIdx){
        pinSetIdx = 0;
    }
    else if (DUAL_CAMERA_SUB_SENSOR == SensorIdx) {
        pinSetIdx = 1;
    }
    else if (DUAL_CAMERA_MAIN_SECOND_SENSOR == SensorIdx) {
        pinSetIdx = 2;
    }

	 Rst_PDN_Init(); // Set Reset and PDN pin Mode & Dir
    //power ON
    if (On) {
        //in case

#if 1 //TODO: depends on HW layout. Should be notified by SA.
        printk("Set CAMERA_POWER_PULL_PIN for power \n"); 
        if (mt_set_gpio_pull_enable(GPIO_CAMERA_LDO_EN_PIN, GPIO_PULL_DISABLE)) {PK_DBG("[[CAMERA SENSOR] Set CAMERA_POWER_PULL_PIN DISABLE ! \n"); }
        if(mt_set_gpio_mode(GPIO_CAMERA_LDO_EN_PIN, GPIO_CAMERA_LDO_EN_PIN_M_GPIO)){PK_DBG("[[CAMERA SENSOR] set CAMERA_POWER_PULL_PIN mode failed!! \n");}
        if(mt_set_gpio_dir(GPIO_CAMERA_LDO_EN_PIN,GPIO_DIR_OUT)){PK_DBG("[[CAMERA SENSOR] set CAMERA_POWER_PULL_PIN dir failed!! \n");}
        if(mt_set_gpio_out(GPIO_CAMERA_LDO_EN_PIN,GPIO_OUT_ONE)){PK_DBG("[[CAMERA SENSOR] set CAMERA_POWER_PULL_PIN failed!! \n");}
#endif
#if 0
        if(pinSetIdx == 0) {

            if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!! \n");}
            if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN],GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! \n");}
            if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_OFF])){PK_DBG("[CAMERA LENS] set gpio failed!! \n");}
        }
        
        if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_D2, VOL_1800,mode_name))
        {
            PK_DBG("[CAMERA SENSOR] Fail to enable digital power\n");
            //return -EIO;
            goto _kdCISModulePowerOn_exit_;
        }                    
        mdelay(1);
        if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_A, VOL_2800,mode_name))
        {
            PK_DBG("[CAMERA SENSOR] Fail to enable analog power\n");
            //return -EIO;
            goto _kdCISModulePowerOn_exit_;
        }

        if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_D, VOL_1500,mode_name))
        {
             PK_DBG("[CAMERA SENSOR] Fail to enable digital power\n");
             //return -EIO;
             goto _kdCISModulePowerOn_exit_;
        }
        
        if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_A2, VOL_2800,mode_name))
        {
            PK_DBG("[CAMERA SENSOR] Fail to enable analog power\n");
            //return -EIO;
            goto _kdCISModulePowerOn_exit_;
        }        
        mdelay(10);
        if(pinSetIdx == 0) {

            if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!! \n");}
            if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN],GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! \n");}
            if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_ON])){PK_DBG("[CAMERA LENS] set gpio failed!! \n");}
        }

        //disable inactive sensor
        if (GPIO_CAMERA_INVALID != pinSet[1-pinSetIdx][IDX_PS_CMRST]) {
            if(mt_set_gpio_mode(pinSet[1-pinSetIdx][IDX_PS_CMRST],pinSet[1-pinSetIdx][IDX_PS_CMRST+IDX_PS_MODE])){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
            if(mt_set_gpio_mode(pinSet[1-pinSetIdx][IDX_PS_CMPDN],pinSet[1-pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!! \n");}
            if(mt_set_gpio_dir(pinSet[1-pinSetIdx][IDX_PS_CMRST],GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
            if(mt_set_gpio_dir(pinSet[1-pinSetIdx][IDX_PS_CMPDN],GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! \n");}
            if(mt_set_gpio_out(pinSet[1-pinSetIdx][IDX_PS_CMRST],pinSet[1-pinSetIdx][IDX_PS_CMRST+IDX_PS_OFF])){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");} //low == reset sensor
            if(mt_set_gpio_out(pinSet[1-pinSetIdx][IDX_PS_CMPDN],pinSet[1-pinSetIdx][IDX_PS_CMPDN+IDX_PS_OFF])){PK_DBG("[CAMERA LENS] set gpio failed!! \n");} //high == power down lens module
        }        

        //enable active sensor
        //RST pin
        if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
            if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_MODE])){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
            if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST],GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
            if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_OFF])){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
            mdelay(10);
            if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_ON])){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
            mdelay(1);

            //PDN pin
            if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!! \n");}
            if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN],GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! \n");}
            if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_ON])){PK_DBG("[CAMERA LENS] set gpio failed!! \n");}
        }   
#endif
        if(currSensorName && (0 == strcmp(SENSOR_DRVNAME_MT9P017_MIPI_RAW,currSensorName)))
        {
            if(TRUE != kd_mt9p017mipi_poweron(mode_name))
                 goto _kdCISModulePowerOn_exit_;
        }else if(currSensorName && (0 == strcmp(SENSOR_DRVNAME_S5K4E1FX_MIPI_RAW,currSensorName))) 
        {
            if(TRUE != kd_s5k4e1fxmipi_poweron(mode_name))
                 goto _kdCISModulePowerOn_exit_;
        }else  if(currSensorName && (0 == strcmp(SENSOR_DRVNAME_HI704_YUV,currSensorName)))
        {
            if(TRUE != kd_hi704yuv_poweron(mode_name))
                 goto _kdCISModulePowerOn_exit_;
        }else if(currSensorName && (0 == strcmp(SENSOR_DRVNAME_GC0329_YUV,currSensorName)))
        {
            if(TRUE != kd_gc0329yuv_poweron(mode_name))
                 goto _kdCISModulePowerOn_exit_;
        }
    }
    else {//power OFF

#if 0 //TODO: depends on HW layout. Should be notified by SA.
        printk("Set GPIO 94 for power OFF\n"); 
        if (mt_set_gpio_pull_enable(GPIO_CAMERA_LDO_EN_PIN, GPIO_PULL_DISABLE)) {PK_DBG("[CAMERA SENSOR] Set GPIO94 PULL DISABLE ! \n"); }
        if(mt_set_gpio_mode(GPIO_CAMERA_LDO_EN_PIN, GPIO_CAMERA_LDO_EN_PIN_M_GPIO)){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
        if(mt_set_gpio_dir(GPIO_CAMERA_LDO_EN_PIN,GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
        if(mt_set_gpio_out(GPIO_CAMERA_LDO_EN_PIN,GPIO_OUT_ZERO)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}    	    
#endif

        //PK_DBG("[OFF]sensorIdx:%d \n",SensorIdx);
	if (mt_set_gpio_pull_enable(GPIO_CAMERA_LDO_EN_PIN, GPIO_PULL_DISABLE)) {PK_DBG("[CAMERA SENSOR] Set GPIO94 PULL DISABLE ! \n"); }
    	if(mt_set_gpio_mode(GPIO_CAMERA_LDO_EN_PIN, GPIO_CAMERA_LDO_EN_PIN_M_GPIO)){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
    	if(mt_set_gpio_dir(GPIO_CAMERA_LDO_EN_PIN,GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
    	if(mt_set_gpio_out(GPIO_CAMERA_LDO_EN_PIN,GPIO_OUT_ZERO)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}  
        if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
            if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_MODE])){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
            if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!! \n");}
            if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST],GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
            if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN],GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! \n");}
            if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_OFF])){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");} //low == reset sensor
    	    if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_OFF])){PK_DBG("[CAMERA LENS] set gpio failed!! \n");} //high == power down lens module
        }       

    	if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_A,mode_name)) {
            PK_DBG("[CAMERA SENSOR] Fail to OFF analog power\n");
            //return -EIO;
            ret = -EIO;
            //goto _kdCISModulePowerOn_exit_;
        }
        if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_A2,mode_name))
        {
            PK_DBG("[CAMERA SENSOR] Fail to enable analog power\n");
            //return -EIO;
            ret = -EIO;
            //goto _kdCISModulePowerOn_exit_;
        }     	
        mdelay(1);
        if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_D, mode_name)) {
            PK_DBG("[CAMERA SENSOR] Fail to OFF digital power\n");
            //return -EIO;
            ret = -EIO;
            //goto _kdCISModulePowerOn_exit_;
        }        
        mdelay(1);
        if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_D2,mode_name))
        {
            PK_DBG("[CAMERA SENSOR] Fail to enable digital power\n");
            //return -EIO;
            ret = -EIO;
            //goto _kdCISModulePowerOn_exit_;
        }                    
    }//

	//return 0;
    return ret;

_kdCISModulePowerOn_exit_:
    return -EIO;
}

EXPORT_SYMBOL(kdCISModulePowerOn);
#else 
#error Error !! forget to implement power control for image sensor

#endif 



