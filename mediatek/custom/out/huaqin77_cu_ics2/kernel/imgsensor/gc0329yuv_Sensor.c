/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of MediaTek Inc. (C) 2005
*
*  BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
*  THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
*  RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON
*  AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
*  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
*  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
*  NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
*  SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
*  SUPPLIED WITH THE MEDIATEK SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH
*  THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. MEDIATEK SHALL ALSO
*  NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO BUYER'S
*  SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
*
*  BUYER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE
*  LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
*  AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
*  OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY BUYER TO
*  MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
*
*  THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
*  WITH THE LAWS OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF
*  LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING THEREOF AND
*  RELATED THERETO SHALL BE SETTLED BY ARBITRATION IN SAN FRANCISCO, CA, UNDER
*  THE RULES OF THE INTERNATIONAL CHAMBER OF COMMERCE (ICC).
*
*****************************************************************************/

/*****************************************************************************
 *
 * Filename:
 * ---------
 *   gc0329yuv_Sensor.c
 *
 * Project:
 * --------
 *   -----
 *
 * Description:
 * ------------
 *   Image sensor driver function
 *
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by PVCS VM. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/
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
#include "kd_imgsensor_errcode.h"
#include "kd_camera_feature.h"

#include "gc0329yuv_Sensor.h"
#include "gc0329yuv_Camera_Sensor_para.h"
#include "gc0329yuv_CameraCustomized.h"

#ifdef MT6575
#include <mach/mt6575_boot.h> /*for BOOTMODE */
#endif

//#define GC0329YUV_DEBUG
#ifdef GC0329YUV_DEBUG
#define SENSORDB printk
#else
#define SENSORDB(x,...)
#endif
#if 0
static struct i2c_client * g_pstI2Cclient = NULL;

int iReadReg_Byte(u8 addr, u8 *buf, u16 i2cId)
{
    int Val;
    unsigned char addr_t = addr; 
    g_pstI2Cclient->addr = i2cId;

    Val = i2c_master_send(g_pstI2Cclient, &addr_t, 1);
    if(Val != 1)
    {
        printk("[CAMERA SENSOR] I2C send failed!! \n");
        return -1;
 
    }
    
    Val = i2c_master_recv(g_pstI2Cclient, (char *)buf, 1);
    if(Val != 1)
    {
        printk("[CAMERA SENSOR] I2C read failed!! \n");
        return -1;
    }
    
    return 0;
}

/*=============================================================================*/
/* iWriteReg_Byte */
/*=============================================================================*/

int iWriteReg_Byte(u8 addr, u8 buf, u32 size, u16 i2cId)
{
    int Val;
    u8 data[2] = {addr, buf};
    g_pstI2Cclient->addr = i2cId;

    Val = i2c_master_send(g_pstI2Cclient, data, 2);
    if(Val < 2)
    {
        printk("[CAMERA SENSOR] I2C send failed!! \n");
        return -1;
    }

    return 0;
}
#endif
//extern BOOTMODE get_boot_mode(void);
static DEFINE_SPINLOCK(gc0329_yuv_drv_lock);

extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);
static void GC0329_write_cmos_sensor(kal_uint8 addr, kal_uint8 para)
{
    
 //   iWriteReg_Byte((u8)addr, (u8)para, 1, GC0329_WRITE_ID);
 	char puSendCmd[2] = {(char)(addr & 0xFF) , (char)(para & 0xFF)};
	
	iWriteRegI2C(puSendCmd , 2,GC0329_WRITE_ID);
}

kal_uint8 GC0329_read_cmos_sensor(kal_uint8 addr)
{
 //   kal_uint8 get_byte = 0;
 //   iReadReg_Byte((u8)addr, &get_byte, GC0329_WRITE_ID);
 //   return get_byte;
    kal_uint16 get_byte=0;
    char puSendCmd = { (char)(addr & 0xFF) };
	iReadRegI2C(&puSendCmd , 1, (u8*)&get_byte,1,GC0329_WRITE_ID);
	
    return get_byte;
}


/*******************************************************************************
 * // Adapter for Winmo typedef
 ********************************************************************************/
#define WINMO_USE 0

#define Sleep(ms) mdelay(ms)
#define RETAILMSG(x,...)
#define TEXT

kal_bool   GC0329_MPEG4_encode_mode = KAL_FALSE;
kal_uint16 GC0329_dummy_pixels = 0, GC0329_dummy_lines = 0;
kal_bool   GC0329_MODE_CAPTURE = KAL_FALSE;
kal_bool   GC0329_CAM_BANDING_50HZ = KAL_FALSE;

kal_bool  GC0329_NIGHT_MODE = KAL_FALSE;

kal_uint32 GC0329_isp_master_clock;
static kal_uint32 GC0329_g_fPV_PCLK = 26;

kal_uint8 GC0329_sensor_write_I2C_address = GC0329_WRITE_ID;
kal_uint8 GC0329_sensor_read_I2C_address = GC0329_READ_ID;

UINT8 GC0329PixelClockDivider=0;

MSDK_SENSOR_CONFIG_STRUCT GC0329SensorConfigData;

#define GC0329_SET_PAGE0 	GC0329_write_cmos_sensor(0xfe, 0x00)
#define GC0329_SET_PAGE1 	GC0329_write_cmos_sensor(0xfe, 0x01)



/*************************************************************************
 * FUNCTION
 *	GC0329_SetShutter
 *
 * DESCRIPTION
 *	This function set e-shutter of GC0329 to change exposure time.
 *
 * PARAMETERS
 *   iShutter : exposured lines
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
void GC0329_Set_Shutter(kal_uint16 iShutter)
{
} /* Set_GC0329_Shutter */

/*************************************************************************
 * FUNCTION
 *	GC0329_read_Shutter
 *
 * DESCRIPTION
 *	This function read e-shutter of GC0329 .
 *
 * PARAMETERS
 *  None
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
kal_uint16 GC0329_Read_Shutter(void)
{
    	kal_uint8 temp_reg1, temp_reg2;
	kal_uint16 shutter;

	temp_reg1 = GC0329_read_cmos_sensor(0x04);
	temp_reg2 = GC0329_read_cmos_sensor(0x03);
	spin_lock(&gc0329_yuv_drv_lock);
	shutter = (temp_reg1 & 0xFF) | (temp_reg2 << 8);
	spin_unlock(&gc0329_yuv_drv_lock);
	return shutter;
} /* GC0329_read_shutter */


/*************************************************************************
 * FUNCTION
 *	GC0329_write_reg
 *
 * DESCRIPTION
 *	This function set the register of GC0329.
 *
 * PARAMETERS
 *	addr : the register index of GC0329
 *  para : setting parameter of the specified register of GC0329
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
void GC0329_write_reg(kal_uint32 addr, kal_uint32 para)
{
	GC0329_write_cmos_sensor(addr, para);
} /* GC0329_write_reg() */

/*************************************************************************
 * FUNCTION
 *	GC0329_read_cmos_sensor
 *
 * DESCRIPTION
 *	This function read parameter of specified register from GC0329.
 *
 * PARAMETERS
 *	addr : the register index of GC0329
 *
 * RETURNS
 *	the data that read from GC0329
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
kal_uint32 GC0329_read_reg(kal_uint32 addr)
{
	return GC0329_read_cmos_sensor(addr);
} /* OV7670_read_reg() */

/*************************************************************************
* FUNCTION
*	GC0329_awb_enable
*
* DESCRIPTION
*	This function enable or disable the awb (Auto White Balance).
*
* PARAMETERS
*	1. kal_bool : KAL_TRUE - enable awb, KAL_FALSE - disable awb.
*
* RETURNS
*	kal_bool : It means set awb right or not.
*
*************************************************************************/
static void GC0329_awb_enable(kal_bool enalbe)
{	 
	kal_uint16 temp_AWB_reg = 0;

	temp_AWB_reg = GC0329_read_cmos_sensor(0x42);
	
	if (enalbe)
	{
		GC0329_write_cmos_sensor(0x42, (temp_AWB_reg |0x02));
	}
	else
	{
		GC0329_write_cmos_sensor(0x42, (temp_AWB_reg & (~0x02)));
	}

}
/*************************************************************************
 * FUNCTION
 *	GC0329_config_window
 *
 * DESCRIPTION
 *	This function config the hardware window of GC0329 for getting specified
 *  data of that window.
 *
 * PARAMETERS
 *	start_x : start column of the interested window
 *  start_y : start row of the interested window
 *  width  : column widht of the itnerested window
 *  height : row depth of the itnerested window
 *
 * RETURNS
 *	the data that read from GC0329
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
void GC0329_config_window(kal_uint16 startx, kal_uint16 starty, kal_uint16 width, kal_uint16 height)
{
} /* GC0329_config_window */

/*************************************************************************
 * FUNCTION
 *	GC0329_SetGain
 *
 * DESCRIPTION
 *	This function is to set global gain to sensor.
 *
 * PARAMETERS
 *   iGain : sensor global gain(base: 0x40)
 *
 * RETURNS
 *	the actually gain set to sensor.
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
kal_uint16 GC0329_SetGain(kal_uint16 iGain)
{
	return iGain;
}

/*************************************************************************
 * FUNCTION
 *	GC0329_NightMode
 *
 * DESCRIPTION
 *	This function night mode of GC0329.
 *
 * PARAMETERS
 *	bEnable: KAL_TRUE -> enable night mode, otherwise, disable night mode
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
void GC0329_night_mode(kal_bool bEnable)
{
#ifdef HQ_PROJECT_A61P_HUAWEI
			GC0329_write_cmos_sensor(0x07, 0x07);
			GC0329_write_cmos_sensor(0x08, 0xd0);
#endif	
	if (bEnable)
	{
	GC0329_write_cmos_sensor(0xfe, 0x00);
	GC0329_write_cmos_sensor(0xbf, 0x0b);
	GC0329_write_cmos_sensor(0xc0, 0x16);
	GC0329_write_cmos_sensor(0xc1, 0x29);
	GC0329_write_cmos_sensor(0xc2, 0x3c);
	GC0329_write_cmos_sensor(0xc3, 0x4f);
	GC0329_write_cmos_sensor(0xc4, 0x5f);
	GC0329_write_cmos_sensor(0xc5, 0x6f);
	GC0329_write_cmos_sensor(0xc6, 0x8a);
	GC0329_write_cmos_sensor(0xc7, 0x9f);
	GC0329_write_cmos_sensor(0xc8, 0xb4);
	GC0329_write_cmos_sensor(0xc9, 0xc6);
	GC0329_write_cmos_sensor(0xca, 0xd3);
	GC0329_write_cmos_sensor(0xcb, 0xdd);
	GC0329_write_cmos_sensor(0xcc, 0xe5);
	GC0329_write_cmos_sensor(0xcd, 0xf1);
	GC0329_write_cmos_sensor(0xce, 0xfa);
	GC0329_write_cmos_sensor(0xcf, 0xff);
	GC0329_write_cmos_sensor(0xfe, 0x00);
       	if(GC0329_MPEG4_encode_mode == KAL_TRUE) 
		{
            		if(GC0329_CAM_BANDING_50HZ == KAL_TRUE)
			{
				GC0329_write_cmos_sensor(0x07, 0x03);
				GC0329_write_cmos_sensor(0x08, 0xd8);

				GC0329_SET_PAGE1;
				GC0329_write_cmos_sensor(0x2b, 0x05);   //exp level 0  6.5fps
				GC0329_write_cmos_sensor(0x2c, 0xc0); 
				GC0329_SET_PAGE0;
			}
			else
			{ 
				GC0329_write_cmos_sensor(0x07, 0x03);
				GC0329_write_cmos_sensor(0x08, 0x40);

				GC0329_SET_PAGE1;
				GC0329_write_cmos_sensor(0x2b, 0x05);   //exp level 0  6fps
				GC0329_write_cmos_sensor(0x2c, 0x28); 
				GC0329_SET_PAGE0;
			}
			GC0329_SET_PAGE1;
			GC0329_write_cmos_sensor(0x33, 0x00);
			GC0329_SET_PAGE0;
       	 }
		else 
		{        
			GC0329_SET_PAGE1;
			GC0329_write_cmos_sensor(0x33, 0x30);
			GC0329_SET_PAGE0;
        	}

			GC0329_NIGHT_MODE = KAL_TRUE;
#ifdef HQ_PROJECT_A61P_HUAWEI	
			mdelay(100);	
#endif
	}
	else 
	{
	GC0329_write_cmos_sensor(0xfe, 0x00);
	GC0329_write_cmos_sensor(0xbf, 0x06);
	GC0329_write_cmos_sensor(0xc0, 0x14);
	GC0329_write_cmos_sensor(0xc1, 0x27);
	GC0329_write_cmos_sensor(0xc2, 0x3b);
	GC0329_write_cmos_sensor(0xc3, 0x4f);
	GC0329_write_cmos_sensor(0xc4, 0x62);
	GC0329_write_cmos_sensor(0xc5, 0x72);
	GC0329_write_cmos_sensor(0xc6, 0x8d);
	GC0329_write_cmos_sensor(0xc7, 0xa4);
	GC0329_write_cmos_sensor(0xc8, 0xb8);
	GC0329_write_cmos_sensor(0xc9, 0xc9);
	GC0329_write_cmos_sensor(0xca, 0xd6);
	GC0329_write_cmos_sensor(0xcb, 0xe0);
	GC0329_write_cmos_sensor(0xcc, 0xe8);
	GC0329_write_cmos_sensor(0xcd, 0xf4);
	GC0329_write_cmos_sensor(0xce, 0xfc);
	GC0329_write_cmos_sensor(0xcf, 0xff);
	GC0329_write_cmos_sensor(0xfe, 0x00);
        	if (GC0329_MPEG4_encode_mode == KAL_TRUE) 
		{
            		if(GC0329_CAM_BANDING_50HZ == KAL_TRUE)
			{
				GC0329_write_cmos_sensor(0x07, 0x00);
				GC0329_write_cmos_sensor(0x08, 0xf8);

				GC0329_SET_PAGE1;
				GC0329_write_cmos_sensor(0x2b, 0x02);   //exp level 0  12.5fps
				GC0329_write_cmos_sensor(0x2c, 0xe0); 
				GC0329_SET_PAGE0;
			}
			else
			{
				GC0329_write_cmos_sensor(0x07, 0x00);
				GC0329_write_cmos_sensor(0x08, 0xac);

				GC0329_SET_PAGE1;
				GC0329_write_cmos_sensor(0x2b, 0x02);   //exp level 0  12.00fps
				GC0329_write_cmos_sensor(0x2c, 0x94); 
				GC0329_SET_PAGE0;
			}
			GC0329_SET_PAGE1;
			GC0329_write_cmos_sensor(0x33, 0x00);
			GC0329_SET_PAGE0;
        	}
		else
		{
			GC0329_SET_PAGE1;
			GC0329_write_cmos_sensor(0x33, 0x20);//10
			GC0329_SET_PAGE0;
    		}
		GC0329_NIGHT_MODE = KAL_FALSE;
#ifdef HQ_PROJECT_A61P_HUAWEI	
		mdelay(100);
#endif
	}
#ifdef HQ_PROJECT_A61P_HUAWEI	
		GC0329_write_cmos_sensor(0x07, 0x00);
		GC0329_write_cmos_sensor(0x08, 0xf8);
#endif		
} /* GC0329_NightMode */


void GC0329_Sensor_Init(void)
{
	GC0329_write_cmos_sensor(0xfe, 0x80);
	GC0329_write_cmos_sensor(0xfc, 0x12);
	GC0329_write_cmos_sensor(0xfc, 0x12);
	GC0329_write_cmos_sensor(0xfe, 0x00);
	
#if defined(HQ_PROJECT_A30)
	GC0329_SET_PAGE0;
	GC0329_write_cmos_sensor(0x05, 0x02); 	
	GC0329_write_cmos_sensor(0x06, 0xcf); 
	GC0329_write_cmos_sensor(0x07, 0x00);
	GC0329_write_cmos_sensor(0x08, 0xf8);

	GC0329_SET_PAGE1;
	GC0329_write_cmos_sensor(0x29, 0x00);   //anti-flicker step [11:8]
	GC0329_write_cmos_sensor(0x2a, 0x5c);   //anti-flicker step [7:0]

	GC0329_write_cmos_sensor(0x2b, 0x02);   //exp level 0  12.5fps
	GC0329_write_cmos_sensor(0x2c, 0x84);  // 2e0
	GC0329_write_cmos_sensor(0x2d, 0x02);   //exp level 0  10fps
	GC0329_write_cmos_sensor(0x2e, 0xe0);  //398
	GC0329_write_cmos_sensor(0x2f, 0x03);   //exp level 0  7.69fps
	GC0329_write_cmos_sensor(0x30, 0x98);  // 4ac
	GC0329_write_cmos_sensor(0x31, 0x07);   //exp level 0  5.00fps
	GC0329_write_cmos_sensor(0x32, 0x30); 
	GC0329_SET_PAGE0;
#endif

	GC0329_write_cmos_sensor(0xf0, 0x07);
	GC0329_write_cmos_sensor(0xf1, 0x01);
	                             
	GC0329_write_cmos_sensor(0x73, 0x90);
	GC0329_write_cmos_sensor(0x74, 0x80);
	GC0329_write_cmos_sensor(0x75, 0x80);
	GC0329_write_cmos_sensor(0x76, 0x94);
	                            
	GC0329_write_cmos_sensor(0x42, 0x00);
	GC0329_write_cmos_sensor(0x77, 0x57);
	GC0329_write_cmos_sensor(0x78, 0x4d);
	GC0329_write_cmos_sensor(0x79, 0x45);
	GC0329_write_cmos_sensor(0x42, 0xfc);
	                             

	GC0329_write_cmos_sensor(0xfc, 0x16);
	                              
	GC0329_write_cmos_sensor(0x0a, 0x00);
	GC0329_write_cmos_sensor(0x0c, 0x00);

	GC0329_write_cmos_sensor(0x17, 0x14);//0x14

	GC0329_write_cmos_sensor(0x19, 0x05);
	GC0329_write_cmos_sensor(0x1b, 0x24);
	GC0329_write_cmos_sensor(0x1c, 0x04);
	GC0329_write_cmos_sensor(0x1e, 0x00);
	GC0329_write_cmos_sensor(0x1f, 0xc0);
	GC0329_write_cmos_sensor(0x20, 0x00);
	GC0329_write_cmos_sensor(0x21, 0x48);
	GC0329_write_cmos_sensor(0x23, 0x22);
	GC0329_write_cmos_sensor(0x24, 0x16);
	                             
	GC0329_write_cmos_sensor(0x26, 0xf7);
	GC0329_write_cmos_sensor(0x32, 0x04);
	GC0329_write_cmos_sensor(0x33, 0x20);
	GC0329_write_cmos_sensor(0x34, 0x20);
	GC0329_write_cmos_sensor(0x35, 0x20);
	GC0329_write_cmos_sensor(0x36, 0x20);
	                               
	GC0329_write_cmos_sensor(0x40, 0xff);
#ifdef HQ_PROJECT_A52_HUAWEI
	GC0329_write_cmos_sensor(0x41, 0x24);
#else
	GC0329_write_cmos_sensor(0x41, 0x20);
#endif
	GC0329_write_cmos_sensor(0x42, 0xfe);
	GC0329_write_cmos_sensor(0x46, 0x02);
	GC0329_write_cmos_sensor(0x4b, 0xcb);
	GC0329_write_cmos_sensor(0x4d, 0x01);
	GC0329_write_cmos_sensor(0x4f, 0x01);
	GC0329_write_cmos_sensor(0x70, 0x48);
	                               
	//GC0329_write_cmos_sensor(0xb0, 0x);
	//GC0329_write_cmos_sensor(0xbc, 0x);
	//GC0329_write_cmos_sensor(0xbd, 0x);
	//GC0329_write_cmos_sensor(0xbe, 0x);

	GC0329_write_cmos_sensor(0x80, 0xe7);
	GC0329_write_cmos_sensor(0x82, 0xe5);
	GC0329_write_cmos_sensor(0x87, 0x4a);
			
	                               

	GC0329_write_cmos_sensor(0xfe, 0x01);
	GC0329_write_cmos_sensor(0x18, 0x22);
	GC0329_write_cmos_sensor(0xfe, 0x00);
	GC0329_write_cmos_sensor(0x9c, 0x0a);
	GC0329_write_cmos_sensor(0xa4, 0x40);
	GC0329_write_cmos_sensor(0xa5, 0x21);
	GC0329_write_cmos_sensor(0xa7, 0x35);
#if defined(HQ_PROJECT_A30)
	GC0329_write_cmos_sensor(0xdd, 0xff);
	GC0329_write_cmos_sensor(0x95, 0x33); // 35
#elif defined(HQ_PROJECT_A61P_HUAWEI)
	GC0329_write_cmos_sensor(0xdd, 0x54);	
	GC0329_write_cmos_sensor(0x95, 0x55);//35
#else                               
	GC0329_write_cmos_sensor(0xdd, 0x54);	
	GC0329_write_cmos_sensor(0x95, 0x65);//35	 
#endif

	GC0329_write_cmos_sensor(0xfe, 0x00);
	GC0329_write_cmos_sensor(0xbf, 0x06);
	GC0329_write_cmos_sensor(0xc0, 0x14);
	GC0329_write_cmos_sensor(0xc1, 0x27);
	GC0329_write_cmos_sensor(0xc2, 0x3b);
	GC0329_write_cmos_sensor(0xc3, 0x4f);
	GC0329_write_cmos_sensor(0xc4, 0x62);
	GC0329_write_cmos_sensor(0xc5, 0x72);
	GC0329_write_cmos_sensor(0xc6, 0x8d);
	GC0329_write_cmos_sensor(0xc7, 0xa4);
	GC0329_write_cmos_sensor(0xc8, 0xb8);
	GC0329_write_cmos_sensor(0xc9, 0xc9);
	GC0329_write_cmos_sensor(0xca, 0xd6);
	GC0329_write_cmos_sensor(0xcb, 0xe0);
	GC0329_write_cmos_sensor(0xcc, 0xe8);
	GC0329_write_cmos_sensor(0xcd, 0xf4);
	GC0329_write_cmos_sensor(0xce, 0xfc);
	GC0329_write_cmos_sensor(0xcf, 0xff);
	GC0329_write_cmos_sensor(0xfe, 0x00);
	                               
	GC0329_write_cmos_sensor(0xfe, 0x00);
#if 1
    	GC0329_write_cmos_sensor(0xb3, 0x44);//0x44
	GC0329_write_cmos_sensor(0xb4, 0xfd);//0xfd
	GC0329_write_cmos_sensor(0xb5, 0x02);//0x02
#else
    	GC0329_write_cmos_sensor(0xb3, 0x42);//0x44
	GC0329_write_cmos_sensor(0xb4, 0xfa);//0xfd
	GC0329_write_cmos_sensor(0xb5, 0x05);//0x02
#endif
	GC0329_write_cmos_sensor(0xb6, 0xfa);
	GC0329_write_cmos_sensor(0xb7, 0x48);
	GC0329_write_cmos_sensor(0xb8, 0xf0);
	                               
	GC0329_write_cmos_sensor(0x50, 0x01);
	GC0329_write_cmos_sensor(0x19, 0x05);
	GC0329_write_cmos_sensor(0x20, 0x01);
	GC0329_write_cmos_sensor(0x22, 0xba);

	GC0329_write_cmos_sensor(0x21, 0x48);
	                              
	GC0329_write_cmos_sensor(0xfe, 0x00);
#if defined(HQ_PROJECT_A61P_HUAWEI)
	GC0329_write_cmos_sensor(0xd1, 0x2a);//0x38 2d//33
	GC0329_write_cmos_sensor(0xd2, 0x2a);//0x38 2d
	GC0329_write_cmos_sensor(0xde, 0x20);//0x21
#elif defined(HQ_PROJECT_A52_HUAWEI)
	GC0329_write_cmos_sensor(0xd1, 0x33);//0x38 2d
	GC0329_write_cmos_sensor(0xd2, 0x33);//0x38 2d
	GC0329_write_cmos_sensor(0xde, 0x21);//0x38 2d
#else
	GC0329_write_cmos_sensor(0xd1, 0x2d);//0x38 2d
	GC0329_write_cmos_sensor(0xd2, 0x2d);//0x38 2d	
#endif
                               
	GC0329_write_cmos_sensor(0xfe, 0x01);
	GC0329_write_cmos_sensor(0x10, 0x40);
	GC0329_write_cmos_sensor(0x11, 0xa1);
	GC0329_write_cmos_sensor(0x12, 0x07);
#if defined(HQ_PROJECT_A30)||defined(HQ_PROJECT_A61P_HUAWEI)
	GC0329_write_cmos_sensor(0x13, 0x48); // 50
#else
	GC0329_write_cmos_sensor(0x13, 0x50); 
#endif
	GC0329_write_cmos_sensor(0x17, 0x8b);//0x88
	GC0329_write_cmos_sensor(0x21, 0xb0);
	GC0329_write_cmos_sensor(0x22, 0x48);
	GC0329_write_cmos_sensor(0x3c, 0x95);
	GC0329_write_cmos_sensor(0x3d, 0x50);
	GC0329_write_cmos_sensor(0x3e, 0x48);
	                  // awb
					  ////////////////////AWB////////////////////
#ifdef HQ_PROJECT_A52_HUAWEI
	GC0329_write_cmos_sensor(0xfe  ,0x01);
	GC0329_write_cmos_sensor(0x06  ,0x16);
	GC0329_write_cmos_sensor(0x07  ,0x06);
	GC0329_write_cmos_sensor(0x08  ,0x98);
	GC0329_write_cmos_sensor(0x09  ,0xee);
	GC0329_write_cmos_sensor(0x50  ,0xfc); //RGB high
	GC0329_write_cmos_sensor(0x51  ,0x28); //20 //Y2C diff
	GC0329_write_cmos_sensor(0x52  ,0x0b); //10 //28 //Y2C diff big
	GC0329_write_cmos_sensor(0x53  ,0x10);//16//20 //c inter
	GC0329_write_cmos_sensor(0x54  ,0x10);//14//30//C inter
	GC0329_write_cmos_sensor(0x55  ,0x10);//16//c max
	GC0329_write_cmos_sensor(0x56  ,0x20);//10//c max big
	GC0329_write_cmos_sensor(0x57  ,0x40); //Y high
	GC0329_write_cmos_sensor(0x58  ,0x60); //70//number limit  X4
	GC0329_write_cmos_sensor(0x59  ,0x28); //08 //AWB adjust temp curve
	GC0329_write_cmos_sensor(0x5a  ,0x02); //25 //[3:0]light gain range x10
	GC0329_write_cmos_sensor(0x5b  ,0x63); //62 //move th
	GC0329_write_cmos_sensor(0x5c  ,0x3d); //37 //show and mode [2]big C mode [1]dark mode [0] block move mode
	GC0329_write_cmos_sensor(0x5d  ,0x73); //52//AWB margin
	GC0329_write_cmos_sensor(0x5e  ,0x11); //11 //19//temp curve_enable
	GC0329_write_cmos_sensor(0x5f  ,0x40); //5K gain
	GC0329_write_cmos_sensor(0x60  ,0x40); //5K gain
	GC0329_write_cmos_sensor(0x61  ,0xc8); //sinT
	GC0329_write_cmos_sensor(0x62  ,0xa0); //cosT
	GC0329_write_cmos_sensor(0x63  ,0x40); //30//AWB X1 cut
	GC0329_write_cmos_sensor(0x64  ,0x50); //60//AWB X2 cut
	GC0329_write_cmos_sensor(0x65  ,0x98); //a0//AWB Y1 cut
	GC0329_write_cmos_sensor(0x66  ,0xfa); //ea//AWB Y2 cut
	GC0329_write_cmos_sensor(0x67  ,0x70); //70 //AWB R gain limit
	GC0329_write_cmos_sensor(0x68  ,0x68); //58 //AWB G gain Limit
	GC0329_write_cmos_sensor(0x69  ,0xa0); //85 //7d //AWB B gain limit
	GC0329_write_cmos_sensor(0x6a  ,0x40);
	GC0329_write_cmos_sensor(0x6b  ,0x39);
	GC0329_write_cmos_sensor(0x6c  ,0x20);
	GC0329_write_cmos_sensor(0x6d  ,0x30);
	GC0329_write_cmos_sensor(0x6e  ,0x41); //outdoor gain limit enable [7]use exp or luma value to adjust outdoor 
	GC0329_write_cmos_sensor(0x70  ,0x02); //50
	GC0329_write_cmos_sensor(0x71  ,0x00); //when outdoor , add high luma gray pixel weight
	GC0329_write_cmos_sensor(0x72  ,0x10);
	GC0329_write_cmos_sensor(0x73  ,0x40); //95// when exp < th, outdoor mode open
	GC0329_write_cmos_sensor(0x74  ,0x32);
	GC0329_write_cmos_sensor(0x75  ,0x40);
	GC0329_write_cmos_sensor(0x76  ,0x30);
	GC0329_write_cmos_sensor(0x77  ,0x48);
	GC0329_write_cmos_sensor(0x7a  ,0x50);
	GC0329_write_cmos_sensor(0x7b  ,0x20); // Yellow R2B	B2G limit, >it, as Yellow

	GC0329_write_cmos_sensor(0x80  ,0x58); //R gain high limit
	GC0329_write_cmos_sensor(0x81  ,0x50); //G gain high limit
	GC0329_write_cmos_sensor(0x82  ,0x44); //B gain high limit
	GC0329_write_cmos_sensor(0x83  ,0x40); //R gain low limit
	GC0329_write_cmos_sensor(0x84  ,0x40); //G gain low limit
	GC0329_write_cmos_sensor(0x85  ,0x40); //B gain low limit


	GC0329_write_cmos_sensor(0x74  ,0x40);//A R2G L
	GC0329_write_cmos_sensor(0x75  ,0x58);//A R2G H
	GC0329_write_cmos_sensor(0x76  ,0x24);//20//30//A B2G L
	GC0329_write_cmos_sensor(0x77  ,0x40);//34//38//48//A B2G H
	GC0329_write_cmos_sensor(0x78  ,0x20);//A G L
	GC0329_write_cmos_sensor(0x79  ,0x60);//A G H
	GC0329_write_cmos_sensor(0x7a  ,0x58);//60//YELLOW R2G
	GC0329_write_cmos_sensor(0x7b  ,0x20);//Yellow R2B  B2G limit, >it, as Yellow
	GC0329_write_cmos_sensor(0x7c  ,0x30);//YELLOW G H
	GC0329_write_cmos_sensor(0x7d  ,0x35);//BREAK B2G THD
	GC0329_write_cmos_sensor(0x7e  ,0x10);//OFFSET B2G 
	GC0329_write_cmos_sensor(0x7f  ,0x08);//10//20//CT change THD
#else
	GC0329_write_cmos_sensor(0xfe, 0x01);
	GC0329_write_cmos_sensor(0x06, 0x06);
	GC0329_write_cmos_sensor(0x07, 0x06);
	GC0329_write_cmos_sensor(0x08, 0xa6);
	GC0329_write_cmos_sensor(0x09, 0xee);
	                              
	GC0329_write_cmos_sensor(0x50, 0xfc);
	GC0329_write_cmos_sensor(0x51, 0x30);
	GC0329_write_cmos_sensor(0x52, 0x10);
	GC0329_write_cmos_sensor(0x53, 0x10);
	GC0329_write_cmos_sensor(0x54, 0x10);
	GC0329_write_cmos_sensor(0x55, 0x16);
	GC0329_write_cmos_sensor(0x56, 0x10);
	//GC0329_write_cmos_sensor(0x57, 0x);
	GC0329_write_cmos_sensor(0x58, 0x60);
	GC0329_write_cmos_sensor(0x59, 0x08);
	GC0329_write_cmos_sensor(0x5a, 0x02);
	GC0329_write_cmos_sensor(0x5b, 0x63);
	GC0329_write_cmos_sensor(0x5c, 0x33);//37 FOR   A52
	GC0329_write_cmos_sensor(0x5d, 0x73);
	GC0329_write_cmos_sensor(0x5e, 0x11);
	GC0329_write_cmos_sensor(0x5f, 0x40);
	GC0329_write_cmos_sensor(0x60, 0x40);
	GC0329_write_cmos_sensor(0x61, 0xc8);
	GC0329_write_cmos_sensor(0x62, 0xa0);
	GC0329_write_cmos_sensor(0x63, 0x40);
	GC0329_write_cmos_sensor(0x64, 0x50);
	GC0329_write_cmos_sensor(0x65, 0x98);
	GC0329_write_cmos_sensor(0x66, 0xfa);
	GC0329_write_cmos_sensor(0x67, 0x80);
	GC0329_write_cmos_sensor(0x68, 0x60);
	GC0329_write_cmos_sensor(0x69, 0x90);
	GC0329_write_cmos_sensor(0x6a, 0x40);
	GC0329_write_cmos_sensor(0x6b, 0x39);
	GC0329_write_cmos_sensor(0x6c, 0x30);
	GC0329_write_cmos_sensor(0x6d, 0x30);
	GC0329_write_cmos_sensor(0x6e, 0x41);
	GC0329_write_cmos_sensor(0x70, 0x10);
	GC0329_write_cmos_sensor(0x71, 0x00);
	GC0329_write_cmos_sensor(0x72, 0x10);
	GC0329_write_cmos_sensor(0x73, 0x40);
	                               
	GC0329_write_cmos_sensor(0x74, 0x32);
	GC0329_write_cmos_sensor(0x75, 0x40);
	GC0329_write_cmos_sensor(0x76, 0x30);
	GC0329_write_cmos_sensor(0x77, 0x48);
	GC0329_write_cmos_sensor(0x7a, 0x50);
	GC0329_write_cmos_sensor(0x7b, 0x20);
	                               
	//GC0329_write_cmos_sensor(0x74, 0x);
	//GC0329_write_cmos_sensor(0x75, 0x);
	//GC0329_write_cmos_sensor(0x76, 0x);
	//GC0329_write_cmos_sensor(0x77, 0x);
	//GC0329_write_cmos_sensor(0x78, 0x);
	//GC0329_write_cmos_sensor(0x79, 0x);
	//GC0329_write_cmos_sensor(0x7a, 0x);
	//GC0329_write_cmos_sensor(0x7b, 0x);
	//GC0329_write_cmos_sensor(0x7c, 0x);
	//GC0329_write_cmos_sensor(0x7d, 0x);
	//GC0329_write_cmos_sensor(0x7e, 0x);
	//GC0329_write_cmos_sensor(0x7f, 0x);
	                               
	GC0329_write_cmos_sensor(0x80, 0xe7);//70
	GC0329_write_cmos_sensor(0x81, 0x58);
#endif			  
           

#if defined(HQ_PROJECT_A30)
	GC0329_write_cmos_sensor(0x82, 0x66); // 42
#elif defined(HQ_PROJECT_A52_HUAWEI)
	GC0329_write_cmos_sensor(0x82, 0x44); //42
#else
	GC0329_write_cmos_sensor(0x82, 0x66); //42
#endif
	GC0329_write_cmos_sensor(0x83, 0x40);
	GC0329_write_cmos_sensor(0x84, 0x40);
	GC0329_write_cmos_sensor(0x85, 0x40);
	GC0329_write_cmos_sensor(0x87, 0x4a);//add

	GC0329_write_cmos_sensor(0xd0, 0x00);
	GC0329_write_cmos_sensor(0xd2, 0x2c);
	GC0329_write_cmos_sensor(0xd3, 0x80);//40
	                               
	GC0329_write_cmos_sensor(0x9c, 0x02);
	GC0329_write_cmos_sensor(0x9d, 0x10);
#ifdef HQ_PROJECT_A52_HUAWEI
	GC0329_write_cmos_sensor(0x9c  ,0x02);
	GC0329_write_cmos_sensor(0x9d  ,0x30);//10 //08//20
	// LSC
	GC0329_write_cmos_sensor(0xfe  ,0x01);//    
	GC0329_write_cmos_sensor(0xa0  ,0x00);//    
	GC0329_write_cmos_sensor(0xa1  ,0x42);//    
	GC0329_write_cmos_sensor(0xa2  ,0x4f);//    
	GC0329_write_cmos_sensor(0xa3  ,0x00);//    
	GC0329_write_cmos_sensor(0xa8  ,0x38);//    
	GC0329_write_cmos_sensor(0xa9  ,0x22);//    
	GC0329_write_cmos_sensor(0xaa  ,0x23);//    
	GC0329_write_cmos_sensor(0xab  ,0x55);//    
	GC0329_write_cmos_sensor(0xac  ,0x34);//    
	GC0329_write_cmos_sensor(0xad  ,0x32);//    
	GC0329_write_cmos_sensor(0xae  ,0x4e);//    
	GC0329_write_cmos_sensor(0xaf  ,0x20);//    
	GC0329_write_cmos_sensor(0xb0  ,0x2c);//    
	GC0329_write_cmos_sensor(0xb1  ,0x1f);//    
	GC0329_write_cmos_sensor(0xb2  ,0x00);//    
	GC0329_write_cmos_sensor(0xb3  ,0x05);//    
	GC0329_write_cmos_sensor(0xb4  ,0x5b);//    
	GC0329_write_cmos_sensor(0xb5  ,0x4c);//    
	GC0329_write_cmos_sensor(0xb6  ,0x4e);//    
	GC0329_write_cmos_sensor(0xba  ,0x63);//    
	GC0329_write_cmos_sensor(0xbb  ,0x52);//    
	GC0329_write_cmos_sensor(0xbc  ,0x58);//    
	GC0329_write_cmos_sensor(0xc0  ,0x1b);//    
	GC0329_write_cmos_sensor(0xc1  ,0x13);//    
	GC0329_write_cmos_sensor(0xc2  ,0x14);//    
	GC0329_write_cmos_sensor(0xc6  ,0x24);//    
	GC0329_write_cmos_sensor(0xc7  ,0x1c);//    
	GC0329_write_cmos_sensor(0xc8  ,0x1d);//    
	GC0329_write_cmos_sensor(0xb7  ,0xd7);//    
	GC0329_write_cmos_sensor(0xb8  ,0xa1);//    
	GC0329_write_cmos_sensor(0xb9  ,0xbc);//    
	GC0329_write_cmos_sensor(0xbd  ,0xc6);//    
	GC0329_write_cmos_sensor(0xbe  ,0x7e);//    
	GC0329_write_cmos_sensor(0xbf  ,0x6a);//    
	GC0329_write_cmos_sensor(0xc3  ,0x18);//    
	GC0329_write_cmos_sensor(0xc4  ,0x21);//    
	GC0329_write_cmos_sensor(0xc5  ,0x1c);//    
	GC0329_write_cmos_sensor(0xc9  ,0x11);//    
	GC0329_write_cmos_sensor(0xca  ,0x19);//    
	GC0329_write_cmos_sensor(0xcb  ,0x20);// 
#else
	GC0329_write_cmos_sensor(0xfe, 0x01);
	GC0329_write_cmos_sensor(0xa0, 0x00);
	GC0329_write_cmos_sensor(0xa1, 0x3c);
	GC0329_write_cmos_sensor(0xa2, 0x50);
	GC0329_write_cmos_sensor(0xa3, 0x00);
	GC0329_write_cmos_sensor(0xa4, 0x40);
	GC0329_write_cmos_sensor(0xa8, 0x08);//r 0a
	GC0329_write_cmos_sensor(0xa9, 0x00);
	GC0329_write_cmos_sensor(0xaa, 0x00);
	GC0329_write_cmos_sensor(0xab, 0x08);//r 04
	GC0329_write_cmos_sensor(0xac, 0x00);
	GC0329_write_cmos_sensor(0xad, 0x00);//7
	GC0329_write_cmos_sensor(0xae, 0x08);//r 0e
	GC0329_write_cmos_sensor(0xaf, 0x00);
	GC0329_write_cmos_sensor(0xb0, 0x00);
	GC0329_write_cmos_sensor(0xb1, 0x08);//r 09
	GC0329_write_cmos_sensor(0xb2, 0x00);
	GC0329_write_cmos_sensor(0xb3, 0x00);
	GC0329_write_cmos_sensor(0xb4, 0x20);//30
	GC0329_write_cmos_sensor(0xb5, 0x20);//16
	GC0329_write_cmos_sensor(0xb6, 0x20);//20
	GC0329_write_cmos_sensor(0xba, 0x20);//3a
	GC0329_write_cmos_sensor(0xbb, 0x20);//1e
	GC0329_write_cmos_sensor(0xbc, 0x20);//28
	GC0329_write_cmos_sensor(0xc0, 0x20);//12
	GC0329_write_cmos_sensor(0xc1, 0x20);//10
	GC0329_write_cmos_sensor(0xc2, 0x20);//14
	GC0329_write_cmos_sensor(0xc6, 0x20);//1d
	GC0329_write_cmos_sensor(0xc7, 0x20);//12
	GC0329_write_cmos_sensor(0xc8, 0x20);//10
	GC0329_write_cmos_sensor(0xb7, 0x00);
	GC0329_write_cmos_sensor(0xb8, 0x00);
	GC0329_write_cmos_sensor(0xb9, 0x00);
	GC0329_write_cmos_sensor(0xbd, 0x00);
	GC0329_write_cmos_sensor(0xbe, 0x00);
	GC0329_write_cmos_sensor(0xbf, 0x00);
	GC0329_write_cmos_sensor(0xc3, 0x00);
	GC0329_write_cmos_sensor(0xc4, 0x00);
	GC0329_write_cmos_sensor(0xc5, 0x00);
	GC0329_write_cmos_sensor(0xc9, 0x00);
	GC0329_write_cmos_sensor(0xca, 0x00);
	GC0329_write_cmos_sensor(0xcb, 0x00);
#endif 
	GC0329_write_cmos_sensor(0xa4  ,0x00);//    
	GC0329_write_cmos_sensor(0xa5  ,0x00);//    
	GC0329_write_cmos_sensor(0xa6  ,0x00);//    
	GC0329_write_cmos_sensor(0xa7  ,0x00);//    
	GC0329_write_cmos_sensor(0xfe  ,0x00);//	
	// XY 059 H


#ifdef HQ_PROJECT_A61P_HUAWEI
	GC0329_write_cmos_sensor(0x7a, 0x7c);//0x7e
#else
	GC0329_write_cmos_sensor(0x7a, 0x7e);
#endif
	GC0329_write_cmos_sensor(0x7b, 0x7f);
	GC0329_write_cmos_sensor(0x7c, 0x80);

	GC0329_write_cmos_sensor(0xa0, 0xaf);
	GC0329_write_cmos_sensor(0xa2, 0xff);
#ifdef HQ_PROJECT_A61P_HUAWEI
	GC0329_write_cmos_sensor(0xd5, 0xfe);//0x00
#else	
	GC0329_write_cmos_sensor(0xd5, 0x03);//0x00
#endif                               
	GC0329_write_cmos_sensor(0x44, 0xa0);
/*********************update 0329 c paramaters************************/
	GC0329_write_cmos_sensor(0xfe, 0x01);
	GC0329_write_cmos_sensor(0x18, 0x22);
	GC0329_write_cmos_sensor(0x21, 0xc0);
	GC0329_write_cmos_sensor(0x06, 0x12);
	GC0329_write_cmos_sensor(0x08, 0x9c);
	GC0329_write_cmos_sensor(0x51, 0x28);
	GC0329_write_cmos_sensor(0x52, 0x10);
	GC0329_write_cmos_sensor(0x53, 0x20);
	GC0329_write_cmos_sensor(0x54, 0x40);
	GC0329_write_cmos_sensor(0x55, 0x16);
	GC0329_write_cmos_sensor(0x56, 0x30);
	GC0329_write_cmos_sensor(0x58, 0x60);
	GC0329_write_cmos_sensor(0x59, 0x08);
	GC0329_write_cmos_sensor(0x5c, 0x35);
	GC0329_write_cmos_sensor(0x5d, 0x72);
#ifdef HQ_PROJECT_A61P_HUAWEI
	GC0329_write_cmos_sensor(0x67  ,0x70); 
	GC0329_write_cmos_sensor(0x68  ,0x69);
	GC0329_write_cmos_sensor(0x69  ,0xa0);
#else	
	GC0329_write_cmos_sensor(0x67, 0x80);
	GC0329_write_cmos_sensor(0x68, 0x60);
	GC0329_write_cmos_sensor(0x69, 0x90);
#endif
	GC0329_write_cmos_sensor(0x6c, 0x30);
	GC0329_write_cmos_sensor(0x6d, 0x60);
	GC0329_write_cmos_sensor(0x70, 0x10);
	GC0329_write_cmos_sensor(0xfe, 0x00);
	GC0329_write_cmos_sensor(0x9c, 0x0a);
	GC0329_write_cmos_sensor(0xa0, 0xaf);
	GC0329_write_cmos_sensor(0xa2, 0xff);
	GC0329_write_cmos_sensor(0xa4, 0x60);
	GC0329_write_cmos_sensor(0xa5, 0x31);
	GC0329_write_cmos_sensor(0xa7, 0x35);
	GC0329_write_cmos_sensor(0x42, 0xfe);
	GC0329_write_cmos_sensor(0xfe, 0x00);

/*********************update 0329 c paramaters************************/

	
        GC0329_write_cmos_sensor(0x17, 0x17);
}


/*************************************************************************
 * FUNCTION
 *	GC0329Open
 *
 * DESCRIPTION
 *	This function initialize the registers of CMOS sensor
 *
 * PARAMETERS
 *	None
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
UINT32 GC0329Open(void)
{
    kal_uint16 sensor_id=0;
    int i;

    GC0329_write_cmos_sensor(0xfc, 0x16);
    Sleep(20);

    do
    {
        	// check if sensor ID correct
        	for(i = 0; i < 3; i++)
		{
	            	sensor_id = GC0329_read_cmos_sensor(0x00);
	            	printk("GC0329 Sensor id = %x\n", sensor_id);
	            	if (sensor_id == GC0329_SENSOR_ID)
			{
	               	break;
	            	}
        	}
        	mdelay(50);
    }while(0);

    if(sensor_id != GC0329_SENSOR_ID)
    {
        SENSORDB("GC0329 Sensor id read failed, ID = %x\n", sensor_id);
        return ERROR_SENSOR_CONNECT_FAIL;
    }

    RETAILMSG(1, (TEXT("Sensor Read ID OK \r\n")));
	GC0329_MPEG4_encode_mode = KAL_FALSE;   // ZYX 
    // initail sequence write in
    GC0329_Sensor_Init();
    return ERROR_NONE;
} /* GC0329Open */

/*************************************************************************
 * FUNCTION
 *	GC0329Close
 *
 * DESCRIPTION
 *	This function is to turn off sensor module power.
 *
 * PARAMETERS
 *	None
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
UINT32 GC0329Close(void)
{
    return ERROR_NONE;
} /* GC0329Close */

/*************************************************************************
 * FUNCTION
 * GC0329Preview
 *
 * DESCRIPTION
 *	This function start the sensor preview.
 *
 * PARAMETERS
 *	*image_window : address pointer of pixel numbers in one period of HSYNC
 *  *sensor_config_data : address pointer of line numbers in one period of VSYNC
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
UINT32 GC0329Preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
        MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)

{
    kal_uint32 iTemp;
    kal_uint16 iStartX = 0, iStartY = 1;

    if(sensor_config_data->SensorOperationMode == MSDK_SENSOR_OPERATION_MODE_VIDEO)		// MPEG4 Encode Mode
    {
        RETAILMSG(1, (TEXT("Camera Video preview\r\n")));
		spin_lock(&gc0329_yuv_drv_lock);
        GC0329_MPEG4_encode_mode = KAL_TRUE;
		spin_unlock(&gc0329_yuv_drv_lock);
       
    }
    else
    {
        RETAILMSG(1, (TEXT("Camera preview\r\n")));
		spin_lock(&gc0329_yuv_drv_lock);
        GC0329_MPEG4_encode_mode = KAL_FALSE;
		spin_unlock(&gc0329_yuv_drv_lock);
    }
    image_window->GrabStartX= IMAGE_SENSOR_VGA_GRAB_PIXELS;
    image_window->GrabStartY= IMAGE_SENSOR_VGA_GRAB_LINES;
    image_window->ExposureWindowWidth = IMAGE_SENSOR_PV_WIDTH;
    image_window->ExposureWindowHeight =IMAGE_SENSOR_PV_HEIGHT;

    // copy sensor_config_data
    memcpy(&GC0329SensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
    return ERROR_NONE;
} /* GC0329Preview */

/*************************************************************************
 * FUNCTION
 *	GC0329Capture
 *
 * DESCRIPTION
 *	This function setup the CMOS sensor in capture MY_OUTPUT mode
 *
 * PARAMETERS
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
UINT32 GC0329Capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
        MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)

{
	spin_lock(&gc0329_yuv_drv_lock);
    GC0329_MODE_CAPTURE=KAL_TRUE;
	spin_unlock(&gc0329_yuv_drv_lock);
    image_window->GrabStartX = IMAGE_SENSOR_VGA_GRAB_PIXELS;
    image_window->GrabStartY = IMAGE_SENSOR_VGA_GRAB_LINES;
    image_window->ExposureWindowWidth= IMAGE_SENSOR_FULL_WIDTH;
    image_window->ExposureWindowHeight = IMAGE_SENSOR_FULL_HEIGHT;

    // copy sensor_config_data
    memcpy(&GC0329SensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
    return ERROR_NONE;
} /* GC0329_Capture() */

UINT32 GC0329GetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{
    pSensorResolution->SensorFullWidth=IMAGE_SENSOR_FULL_WIDTH;
    pSensorResolution->SensorFullHeight=IMAGE_SENSOR_FULL_HEIGHT;
    pSensorResolution->SensorPreviewWidth=IMAGE_SENSOR_PV_WIDTH;
    pSensorResolution->SensorPreviewHeight=IMAGE_SENSOR_PV_HEIGHT;
    return ERROR_NONE;
} /* GC0329GetResolution() */

UINT32 GC0329GetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
        MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
        MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
    pSensorInfo->SensorPreviewResolutionX=IMAGE_SENSOR_PV_WIDTH;
    pSensorInfo->SensorPreviewResolutionY=IMAGE_SENSOR_PV_HEIGHT;
    pSensorInfo->SensorFullResolutionX=IMAGE_SENSOR_FULL_WIDTH;
    pSensorInfo->SensorFullResolutionY=IMAGE_SENSOR_FULL_WIDTH;

    pSensorInfo->SensorCameraPreviewFrameRate=30;
    pSensorInfo->SensorVideoFrameRate=30;
    pSensorInfo->SensorStillCaptureFrameRate=10;
    pSensorInfo->SensorWebCamCaptureFrameRate=15;
    pSensorInfo->SensorResetActiveHigh=FALSE;
    pSensorInfo->SensorResetDelayCount=1;
    pSensorInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_YUYV;
    pSensorInfo->SensorClockPolarity=SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorClockFallingPolarity=SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorInterruptDelayLines = 1;
    pSensorInfo->SensroInterfaceType=SENSOR_INTERFACE_TYPE_PARALLEL;

    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_100_MODE].MaxWidth=CAM_SIZE_5M_WIDTH;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_100_MODE].MaxHeight=CAM_SIZE_5M_HEIGHT;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_100_MODE].ISOSupported=TRUE;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_100_MODE].BinningEnable=FALSE;

    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_200_MODE].MaxWidth=CAM_SIZE_5M_WIDTH;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_200_MODE].MaxHeight=CAM_SIZE_5M_HEIGHT;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_200_MODE].ISOSupported=TRUE;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_200_MODE].BinningEnable=FALSE;

    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_400_MODE].MaxWidth=CAM_SIZE_5M_WIDTH;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_400_MODE].MaxHeight=CAM_SIZE_5M_HEIGHT;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_400_MODE].ISOSupported=TRUE;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_400_MODE].BinningEnable=FALSE;

    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_800_MODE].MaxWidth=CAM_SIZE_1M_WIDTH;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_800_MODE].MaxHeight=CAM_SIZE_1M_HEIGHT;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_800_MODE].ISOSupported=TRUE;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_800_MODE].BinningEnable=FALSE;

    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].MaxWidth=CAM_SIZE_1M_WIDTH;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].MaxHeight=CAM_SIZE_1M_HEIGHT;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].ISOSupported=TRUE;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].BinningEnable=FALSE;
    pSensorInfo->CaptureDelayFrame = 1;
    pSensorInfo->PreviewDelayFrame = 4; // 0
    pSensorInfo->VideoDelayFrame = 4;
    pSensorInfo->SensorMasterClockSwitch = 0;
    pSensorInfo->SensorDrivingCurrent = ISP_DRIVING_2MA;

    switch (ScenarioId)
    {
    case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
    case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
    case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:
        pSensorInfo->SensorClockFreq=26;
        pSensorInfo->SensorClockDividCount=	3;
        pSensorInfo->SensorClockRisingCount= 0;
        pSensorInfo->SensorClockFallingCount= 2;
        pSensorInfo->SensorPixelClockCount= 3;
        pSensorInfo->SensorDataLatchCount= 2;
        pSensorInfo->SensorGrabStartX = IMAGE_SENSOR_VGA_GRAB_PIXELS;
        pSensorInfo->SensorGrabStartY = IMAGE_SENSOR_VGA_GRAB_LINES;

        break;
    case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
    case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
        pSensorInfo->SensorClockFreq=26;
        pSensorInfo->SensorClockDividCount= 3;
        pSensorInfo->SensorClockRisingCount=0;
        pSensorInfo->SensorClockFallingCount=2;
        pSensorInfo->SensorPixelClockCount=3;
        pSensorInfo->SensorDataLatchCount=2;
        pSensorInfo->SensorGrabStartX = IMAGE_SENSOR_VGA_GRAB_PIXELS;
        pSensorInfo->SensorGrabStartY = IMAGE_SENSOR_VGA_GRAB_LINES;
        break;
    default:
        pSensorInfo->SensorClockFreq=26;
        pSensorInfo->SensorClockDividCount= 3;
        pSensorInfo->SensorClockRisingCount=0;
        pSensorInfo->SensorClockFallingCount=2;
        pSensorInfo->SensorPixelClockCount=3;
        pSensorInfo->SensorDataLatchCount=2;
        pSensorInfo->SensorGrabStartX = IMAGE_SENSOR_VGA_GRAB_PIXELS;
        pSensorInfo->SensorGrabStartY = IMAGE_SENSOR_VGA_GRAB_LINES;
        break;
    }
	spin_lock(&gc0329_yuv_drv_lock);
    GC0329PixelClockDivider=pSensorInfo->SensorPixelClockCount;
	spin_unlock(&gc0329_yuv_drv_lock);
    memcpy(pSensorConfigData, &GC0329SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
    return ERROR_NONE;
} /* GC0329GetInfo() */


UINT32 GC0329Control(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
        MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
    switch (ScenarioId)
    {
    case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
    case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
    case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:
        GC0329Preview(pImageWindow, pSensorConfigData);
        break;
    case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
    case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
        GC0329Capture(pImageWindow, pSensorConfigData);
        break;
    }


    return TRUE;
}	/* GC0329Control() */

BOOL GC0329_set_param_wb(UINT16 para)
{
#ifdef HQ_PROJECT_A61P_HUAWEI
	u16 R,G,B;
#endif

	switch (para)
	{
		case AWB_MODE_OFF:

		break;
		
		case AWB_MODE_AUTO:
			GC0329_awb_enable(KAL_FALSE);
			GC0329_write_cmos_sensor(0x77, 0x53); //cap
			GC0329_write_cmos_sensor(0x78, 0x45);//cap
			GC0329_write_cmos_sensor(0x79, 0x40);//cap
            GC0329_awb_enable(KAL_TRUE);	
#ifdef HQ_PROJECT_A61P_HUAWEI
			R=GC0329_read_cmos_sensor(0x77);		
			G=GC0329_read_cmos_sensor(0x78);
			B=GC0329_read_cmos_sensor(0x79);
			printk("******************GC0329 WB gain R= %x************\n", R);
		    printk("******************GC0329 WB gain G= %x************\n", G);
			printk("******************GC0329 WB gain B= %x************\n", B);
#endif			
		break;
		
		case AWB_MODE_CLOUDY_DAYLIGHT: //cloudy
			GC0329_awb_enable(KAL_FALSE);
	#if defined(HQ_PROJECT_A18)|| defined(HQ_PROJECT_A16)||defined(HQ_PROJECT_A60)||defined(HQ_PROJECT_A61P_HUAWEI)	
			GC0329_write_cmos_sensor(0x77, 0x6c);//0x8c //WB_manual_gain 
	#else
	        GC0329_write_cmos_sensor(0x77, 0x8c); //WB_manual_gain 
	#endif
#ifdef HQ_PROJECT_A61P_HUAWEI
			GC0329_write_cmos_sensor(0x78, 0x5C);
#else		
			GC0329_write_cmos_sensor(0x78, 0x50);
#endif
			GC0329_write_cmos_sensor(0x79, 0x40);
		break;
		
		case AWB_MODE_DAYLIGHT: //sunny
			GC0329_awb_enable(KAL_FALSE);
			GC0329_write_cmos_sensor(0x77, 0x74); 
			GC0329_write_cmos_sensor(0x78, 0x52);
			GC0329_write_cmos_sensor(0x79, 0x40);			
		break;
		
		case AWB_MODE_INCANDESCENT: //office
			GC0329_awb_enable(KAL_FALSE);
#ifdef HQ_PROJECT_A61P_HUAWEI
			GC0329_write_cmos_sensor(0x77, 0x40);//48
			GC0329_write_cmos_sensor(0x78, 0x45);//40	
#else			
			GC0329_write_cmos_sensor(0x77, 0x48);
			GC0329_write_cmos_sensor(0x78, 0x40);
#endif			
		#if defined(HQ_PROJECT_A18)|| defined(HQ_PROJECT_A16)||defined(HQ_PROJECT_A60)||defined(HQ_PROJECT_A61P_HUAWEI)	
		    GC0329_write_cmos_sensor(0x79, 0x48);//0x5c
		#else
			GC0329_write_cmos_sensor(0x79, 0x5c);
		#endif
		break;
		
		case AWB_MODE_TUNGSTEN: //home
			GC0329_awb_enable(KAL_FALSE);
			GC0329_write_cmos_sensor(0x77, 0x40);
#ifdef HQ_PROJECT_A61P_HUAWEI
			GC0329_write_cmos_sensor(0x78, 0x48);
			GC0329_write_cmos_sensor(0x79, 0x58);
#else				
			GC0329_write_cmos_sensor(0x78, 0x54);
			GC0329_write_cmos_sensor(0x79, 0x70);
#endif			
		break;
		
		case AWB_MODE_FLUORESCENT:
			GC0329_awb_enable(KAL_FALSE);
			GC0329_write_cmos_sensor(0x77, 0x40);
			GC0329_write_cmos_sensor(0x78, 0x42);
#ifdef HQ_PROJECT_A61P_HUAWEI
			GC0329_write_cmos_sensor(0x79, 0x45);
#else 
			GC0329_write_cmos_sensor(0x79, 0x50);
#endif
		break;
		
		default:
		return FALSE;
	}

	return TRUE;
} /* GC0329_set_param_wb */

BOOL GC0329_set_param_effect(UINT16 para)
{
	kal_uint32  ret = KAL_TRUE;

	switch (para)
	{
		case MEFFECT_OFF:
			GC0329_write_cmos_sensor(0x43 , 0x00);
		break;
		
		case MEFFECT_SEPIA:
			GC0329_write_cmos_sensor(0x43 , 0x02);
			GC0329_write_cmos_sensor(0xda , 0xd0);
			GC0329_write_cmos_sensor(0xdb , 0x28);
		break;
		
		case MEFFECT_NEGATIVE:
			GC0329_write_cmos_sensor(0x43 , 0x01);
		break;
		
		case MEFFECT_SEPIAGREEN:
			GC0329_write_cmos_sensor(0x43 , 0x02);
			GC0329_write_cmos_sensor(0xda , 0xc0);
			GC0329_write_cmos_sensor(0xdb , 0xc0);
		break;
		
		case MEFFECT_SEPIABLUE:
			GC0329_write_cmos_sensor(0x43 , 0x02);
			GC0329_write_cmos_sensor(0xda , 0x50);
			GC0329_write_cmos_sensor(0xdb , 0xe0);
		break;

		case MEFFECT_MONO:
			GC0329_write_cmos_sensor(0x43 , 0x02);
			GC0329_write_cmos_sensor(0xda , 0x00);
			GC0329_write_cmos_sensor(0xdb , 0x00);
		break;
		default:
			ret = FALSE;
	}

	return ret;

} /* GC0329_set_param_effect */

BOOL GC0329_set_param_banding(UINT16 para)
{
	switch (para)
	{
		case AE_FLICKER_MODE_50HZ:
			GC0329_write_cmos_sensor(0x05, 0x02); 	
			GC0329_write_cmos_sensor(0x06, 0xcf); 
			GC0329_write_cmos_sensor(0x07, 0x00);
			GC0329_write_cmos_sensor(0x08, 0xf8);

			GC0329_SET_PAGE1;
			GC0329_write_cmos_sensor(0x29, 0x00);   //anti-flicker step [11:8]
			GC0329_write_cmos_sensor(0x2a, 0x5c);   //anti-flicker step [7:0]

			GC0329_write_cmos_sensor(0x2b, 0x02);   //exp level 0  12.5fps
			GC0329_write_cmos_sensor(0x2c, 0xe0); 
			GC0329_write_cmos_sensor(0x2d, 0x03);   //exp level 0  10fps
			GC0329_write_cmos_sensor(0x2e, 0x98); 
			GC0329_write_cmos_sensor(0x2f, 0x04);   //exp level 0  7.69fps
			GC0329_write_cmos_sensor(0x30, 0xac);
			GC0329_write_cmos_sensor(0x31, 0x07);   //exp level 0  5.00fps
			GC0329_write_cmos_sensor(0x32, 0x30); 
			GC0329_SET_PAGE0;
			spin_lock(&gc0329_yuv_drv_lock);

			GC0329_CAM_BANDING_50HZ = KAL_TRUE;

			spin_unlock(&gc0329_yuv_drv_lock);
			break;

		case AE_FLICKER_MODE_60HZ:
			GC0329_write_cmos_sensor(0x05, 0x03); 	
			GC0329_write_cmos_sensor(0x06, 0x35); 
			GC0329_write_cmos_sensor(0x07, 0x00);
			GC0329_write_cmos_sensor(0x08, 0xac);

			GC0329_SET_PAGE1;
			GC0329_write_cmos_sensor(0x29, 0x00);   //anti-flicker step [11:8]
			GC0329_write_cmos_sensor(0x2a, 0x42);   //anti-flicker step [7:0]

			GC0329_write_cmos_sensor(0x2b, 0x02);   //exp level 0  12.00fps
			GC0329_write_cmos_sensor(0x2c, 0x94); 
			GC0329_write_cmos_sensor(0x2d, 0x03);   //exp level 0  10.00fps
			GC0329_write_cmos_sensor(0x2e, 0x18); 
			GC0329_write_cmos_sensor(0x2f, 0x04);   //exp level 0  7.50fps
			GC0329_write_cmos_sensor(0x30, 0x20); 
			GC0329_write_cmos_sensor(0x31, 0x06);   //exp level 0  5.00fps
			GC0329_write_cmos_sensor(0x32, 0x30); 
			GC0329_SET_PAGE0;
			spin_lock(&gc0329_yuv_drv_lock);

			GC0329_CAM_BANDING_50HZ = KAL_FALSE;

			spin_unlock(&gc0329_yuv_drv_lock);
		break;
		default:
		return FALSE;
	}

	return TRUE;
} /* GC0329_set_param_banding */

BOOL GC0329_set_param_exposure(UINT16 para)
{

kal_uint8 value_luma,value_Y;
value_luma = (GC0329_NIGHT_MODE?0X2B:0X00);
value_Y = (GC0329_NIGHT_MODE?0X68:0X50);

	GC0329_write_cmos_sensor(0x07, 0x07);
	GC0329_write_cmos_sensor(0x08, 0xd0);
	switch (para)
	{
		case AE_EV_COMP_n13:
GC0329_write_cmos_sensor(0xf0, 0x00);//0x00
	//GC0329_write_cmos_sensor(0x07, 0x03);
	//GC0329_write_cmos_sensor(0x08, 0xf8);
			GC0329_write_cmos_sensor(0xd5, value_luma -0X40);
			GC0329_SET_PAGE1;
			GC0329_write_cmos_sensor(0x13, value_Y -0x30);
			GC0329_SET_PAGE0;
  mdelay(100);
GC0329_write_cmos_sensor(0xf0, 0x07);
	//GC0329_write_cmos_sensor(0x07, 0x00);
	//GC0329_write_cmos_sensor(0x08, 0xf8);
		break;
		
		case AE_EV_COMP_n10:
GC0329_write_cmos_sensor(0xf0, 0x00);//0x00
	//GC0329_write_cmos_sensor(0x07, 0x03);
	//GC0329_write_cmos_sensor(0x08, 0xf8);
			GC0329_write_cmos_sensor(0xd5, value_luma -0X30);
			GC0329_SET_PAGE1;
			GC0329_write_cmos_sensor(0x13, value_Y -0x28);
			GC0329_SET_PAGE0;
  mdelay(100);
GC0329_write_cmos_sensor(0xf0, 0x07);
	//GC0329_write_cmos_sensor(0x07, 0x00);
	//GC0329_write_cmos_sensor(0x08, 0xf8);
		break;
		
		case AE_EV_COMP_n07:
GC0329_write_cmos_sensor(0xf0, 0x00);//0x00
	//GC0329_write_cmos_sensor(0x07, 0x03);
	//GC0329_write_cmos_sensor(0x08, 0xf8);
			GC0329_write_cmos_sensor(0xd5, value_luma -0X20);
			GC0329_SET_PAGE1;
			GC0329_write_cmos_sensor(0x13, value_Y -0x20);
			GC0329_SET_PAGE0;
  mdelay(100);
GC0329_write_cmos_sensor(0xf0, 0x07);
	//GC0329_write_cmos_sensor(0x07, 0x00);
	//GC0329_write_cmos_sensor(0x08, 0xf8);
		break;
		
		case AE_EV_COMP_n03:
GC0329_write_cmos_sensor(0xf0, 0x00);//0x00
	//GC0329_write_cmos_sensor(0x07, 0x03);
	//GC0329_write_cmos_sensor(0x08, 0xf8);
			GC0329_write_cmos_sensor(0xd5,value_luma -0X10);
			GC0329_SET_PAGE1;
			GC0329_write_cmos_sensor(0x13, value_Y -0x18);
			GC0329_SET_PAGE0;
  mdelay(100);
GC0329_write_cmos_sensor(0xf0, 0x07);
	//GC0329_write_cmos_sensor(0x07, 0x00);
	//GC0329_write_cmos_sensor(0x08, 0xf8);
		break;
		
		case AE_EV_COMP_00:
		
	//GC0329_write_cmos_sensor(0x07, 0x03);
	//GC0329_write_cmos_sensor(0x08, 0xf8);
GC0329_write_cmos_sensor(0xf0, 0x00);//0x00
#ifdef HQ_PROJECT_A61P_HUAWEI
	GC0329_write_cmos_sensor(0xd5, value_luma -2);//0xfe);//0x00
#else
			GC0329_write_cmos_sensor(0xd5, 0x03);//0x00
#endif
		
			
		
			GC0329_SET_PAGE1;

#if defined(HQ_PROJECT_A61P_HUAWEI)||defined(HQ_PROJECT_A52_HUAWEI)
			GC0329_write_cmos_sensor(0x13, value_Y -0x08);//48
#else
			GC0329_write_cmos_sensor(0x13, 0x58);//48
#endif
		
			GC0329_SET_PAGE0;
  mdelay(100);
GC0329_write_cmos_sensor(0xf0, 0x07);
	//GC0329_write_cmos_sensor(0x07, 0x00);
	//GC0329_write_cmos_sensor(0x08, 0xf8);
		break;

		case AE_EV_COMP_03:
GC0329_write_cmos_sensor(0xf0, 0x00);//0x00
	//GC0329_write_cmos_sensor(0x07, 0x03);
	//GC0329_write_cmos_sensor(0x08, 0xf8);
			GC0329_write_cmos_sensor(0xd5, value_luma + 0X10);
			GC0329_SET_PAGE1;
			GC0329_write_cmos_sensor(0x13, value_Y + 0x30);
			GC0329_SET_PAGE0;
  mdelay(100);
GC0329_write_cmos_sensor(0xf0, 0x07);
	//GC0329_write_cmos_sensor(0x07, 0x00);
	//GC0329_write_cmos_sensor(0x08, 0xf8);
		break;
		
		case AE_EV_COMP_07:
GC0329_write_cmos_sensor(0xf0, 0x00);//0x00
	//GC0329_write_cmos_sensor(0x07, 0x03);
	//GC0329_write_cmos_sensor(0x08, 0xf8);
			GC0329_write_cmos_sensor(0xd5, value_luma + 0X20);
			GC0329_SET_PAGE1;
			GC0329_write_cmos_sensor(0x13, value_Y + 0x50);
			GC0329_SET_PAGE0;
  mdelay(100);
GC0329_write_cmos_sensor(0xf0, 0x07);
	//GC0329_write_cmos_sensor(0x07, 0x00);
	//GC0329_write_cmos_sensor(0x08, 0xf8);
		break;
		
		case AE_EV_COMP_10:
GC0329_write_cmos_sensor(0xf0, 0x00);//0x00
	//GC0329_write_cmos_sensor(0x07, 0x03);
	//GC0329_write_cmos_sensor(0x08, 0xf8);
			GC0329_write_cmos_sensor(0xd5, value_luma + 0X30);
			GC0329_SET_PAGE1;
			GC0329_write_cmos_sensor(0x13, value_Y + 0x70);
			GC0329_SET_PAGE0;
  mdelay(100);
GC0329_write_cmos_sensor(0xf0, 0x07);
	//GC0329_write_cmos_sensor(0x07, 0x00);
	//GC0329_write_cmos_sensor(0x08, 0xf8);
		break;
		
		case AE_EV_COMP_13:
GC0329_write_cmos_sensor(0xf0, 0x00);//0x00
	//GC0329_write_cmos_sensor(0x07, 0x07);
	//GC0329_write_cmos_sensor(0x08, 0xd0);
			GC0329_write_cmos_sensor(0xd5, value_luma + 0X40);
			GC0329_SET_PAGE1;
			GC0329_write_cmos_sensor(0x13, value_Y + 0x90);
			GC0329_SET_PAGE0;
  mdelay(100);
GC0329_write_cmos_sensor(0xf0, 0x07);
	//GC0329_write_cmos_sensor(0x07, 0x00);
	//GC0329_write_cmos_sensor(0x08, 0xf8);
		break;
		default:
		return FALSE;
	}
	GC0329_write_cmos_sensor(0x07, 0x00);
	GC0329_write_cmos_sensor(0x08, 0xf8);
               mdelay(100);
	return TRUE;
} /* GC0329_set_param_exposure */

UINT32 GC0329YUVSensorSetting(FEATURE_ID iCmd, UINT16 iPara)
{
    switch (iCmd) {
#ifdef 	HQ_PROJECT_A61P_HUAWEI	
	case FID_SCENE_MODE:// add 2012.4.1
	GC0329_night_mode(iPara);
	#if 0
		//printk("******************GC0329 Sensor Night Mode iPara = %d************\n", iPara);
		if (iPara == SCENE_MODE_OFF)//auto mode
		{
			GC0329_night_mode(FALSE);
		}
		else if (iPara == SCENE_MODE_NIGHTSCENE)//night mode
		{
			GC0329_night_mode(TRUE);
		}	
	#endif
		break;	
#endif	
    case FID_AWB_MODE:
        GC0329_set_param_wb(iPara);
        break;
    case FID_COLOR_EFFECT:
        GC0329_set_param_effect(iPara);
        break;
    case FID_AE_EV:
        GC0329_set_param_exposure(iPara);
        break;
    case FID_AE_FLICKER:
        GC0329_set_param_banding(iPara);
        break;
    default:
        break;
    }
    return TRUE;
} /* GC0329YUVSensorSetting */

static kal_uint32 GC0329YUVSetVideoMode(void)  //ZYX
{
   GC0329_MPEG4_encode_mode = KAL_TRUE;
    return ERROR_NONE;
}

UINT32 GC0329FeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
        UINT8 *pFeaturePara,UINT32 *pFeatureParaLen)
{
    UINT16 *pFeatureReturnPara16=(UINT16 *) pFeaturePara;
    UINT16 *pFeatureData16=(UINT16 *) pFeaturePara;
    UINT32 *pFeatureReturnPara32=(UINT32 *) pFeaturePara;
    UINT32 *pFeatureData32=(UINT32 *) pFeaturePara;
    UINT32 GC0329SensorRegNumber;
    UINT32 i;
    MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData=(MSDK_SENSOR_CONFIG_STRUCT *) pFeaturePara;
    MSDK_SENSOR_REG_INFO_STRUCT *pSensorRegData=(MSDK_SENSOR_REG_INFO_STRUCT *) pFeaturePara;

    RETAILMSG(1, (_T("gaiyang GC0329FeatureControl FeatureId=%d\r\n"), FeatureId));

    switch (FeatureId)
    {
    case SENSOR_FEATURE_GET_RESOLUTION:
        *pFeatureReturnPara16++=IMAGE_SENSOR_FULL_WIDTH;
        *pFeatureReturnPara16=IMAGE_SENSOR_FULL_HEIGHT;
        *pFeatureParaLen=4;
        break;
    case SENSOR_FEATURE_SET_VIDEO_MODE:    //  ZYX
	GC0329YUVSetVideoMode();
	break;

    case SENSOR_FEATURE_GET_PERIOD:
        *pFeatureReturnPara16++=(VGA_PERIOD_PIXEL_NUMS)+GC0329_dummy_pixels;
        *pFeatureReturnPara16=(VGA_PERIOD_LINE_NUMS)+GC0329_dummy_lines;
        *pFeatureParaLen=4;
        break;
    case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
        *pFeatureReturnPara32 = GC0329_g_fPV_PCLK;
        *pFeatureParaLen=4;
        break;
    case SENSOR_FEATURE_SET_ESHUTTER:
        break;
    case SENSOR_FEATURE_SET_NIGHTMODE:
        GC0329_night_mode((BOOL) *pFeatureData16);
        break;
    case SENSOR_FEATURE_SET_GAIN:
    case SENSOR_FEATURE_SET_FLASHLIGHT:
        break;
    case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
        GC0329_isp_master_clock=*pFeatureData32;
        break;
    case SENSOR_FEATURE_SET_REGISTER:
        GC0329_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
        break;
    case SENSOR_FEATURE_GET_REGISTER:
        pSensorRegData->RegData = GC0329_read_cmos_sensor(pSensorRegData->RegAddr);
        break;
    case SENSOR_FEATURE_GET_CONFIG_PARA:
        memcpy(pSensorConfigData, &GC0329SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
        *pFeatureParaLen=sizeof(MSDK_SENSOR_CONFIG_STRUCT);
        break;
    case SENSOR_FEATURE_SET_CCT_REGISTER:
    case SENSOR_FEATURE_GET_CCT_REGISTER:
    case SENSOR_FEATURE_SET_ENG_REGISTER:
    case SENSOR_FEATURE_GET_ENG_REGISTER:
    case SENSOR_FEATURE_GET_REGISTER_DEFAULT:
    case SENSOR_FEATURE_CAMERA_PARA_TO_SENSOR:
    case SENSOR_FEATURE_SENSOR_TO_CAMERA_PARA:
    case SENSOR_FEATURE_GET_GROUP_COUNT:
    case SENSOR_FEATURE_GET_GROUP_INFO:
    case SENSOR_FEATURE_GET_ITEM_INFO:
    case SENSOR_FEATURE_SET_ITEM_INFO:
    case SENSOR_FEATURE_GET_ENG_INFO:
        break;
    case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
        // get the lens driver ID from EEPROM or just return LENS_DRIVER_ID_DO_NOT_CARE
        // if EEPROM does not exist in camera module.
        *pFeatureReturnPara32=LENS_DRIVER_ID_DO_NOT_CARE;
        *pFeatureParaLen=4;
        break;
    case SENSOR_FEATURE_SET_YUV_CMD:
        GC0329YUVSensorSetting((FEATURE_ID)*pFeatureData32, *(pFeatureData32+1));
        break;
    default:
        break;
	}
return ERROR_NONE;
}	/* GC0329FeatureControl() */




SENSOR_FUNCTION_STRUCT	SensorFuncGC0329YUV=
{
	GC0329Open,
	GC0329GetInfo,
	GC0329GetResolution,
	GC0329FeatureControl,
	GC0329Control,
	GC0329Close
};
UINT32 GC0329_YUV_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc!=NULL)
		*pfFunc=&SensorFuncGC0329YUV;
	return ERROR_NONE;
} /* SensorInit() */

