/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of MediaTek Inc. (C) 2008
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
 *   sensor.c
 *
 * Project:
 * --------
 *   DUMA
 *
 * Description:
 * ------------
 *   Source code of Sensor driver
 *
 *
 * Author:
 * -------
 *   PC Huang (MTK02204)
 *
 *============================================================================
 *             HISTORY
 * Below this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *------------------------------------------------------------------------------
 * $Revision:$
 * $Modtime:$
 * $Log:$
 *
 * 07 11 2011 jun.pei
 * [ALPS00059464] hi704 sensor check in
 * .
 *
 *
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
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
//#include <mach/mt6516_pll.h>

#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"
#include "kd_camera_feature.h"

#include "hi704yuv_Sensor.h"
#include "hi704yuv_Camera_Sensor_para.h"
#include "hi704yuv_CameraCustomized.h"

#define HI704YUV_DEBUG
#ifdef HI704YUV_DEBUG
#define SENSORDB printk
#else
#define SENSORDB(x,...)
#endif

#if 0
extern int iReadReg(u16 a_u2Addr , u8 * a_puBuff , u16 i2cId);
extern int iWriteReg(u16 a_u2Addr , u32 a_u4Data , u32 a_u4Bytes , u16 i2cId);
static int sensor_id_fail = 0; 
#define HI704_write_cmos_sensor(addr, para) iWriteReg((u16) addr , (u32) para ,1,HI704_WRITE_ID)
#define HI704_write_cmos_sensor_2(addr, para, bytes) iWriteReg((u16) addr , (u32) para ,bytes,HI704_WRITE_ID)
kal_uint16 HI704_read_cmos_sensor(kal_uint32 addr)
{
kal_uint16 get_byte=0;
    iReadReg((u16) addr ,(u8*)&get_byte,HI704_WRITE_ID);
    return get_byte;
}

#endif
static kal_uint32 HI704_pv_exposure_lines;
static DEFINE_SPINLOCK(hi704_yuv_drv_lock);

extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);
kal_uint16 HI704_write_cmos_sensor(kal_uint8 addr, kal_uint8 para)
{
    char puSendCmd[2] = {(char)(addr & 0xFF) , (char)(para & 0xFF)};
    iWriteRegI2C(puSendCmd , 2,HI704_WRITE_ID);
    return 0;
}
kal_uint16 HI704_read_cmos_sensor(kal_uint8 addr)
{
    kal_uint16 get_byte=0;
    char puSendCmd = { (char)(addr & 0xFF) };
    iReadRegI2C(&puSendCmd , 1, (u8*)&get_byte,1,HI704_WRITE_ID);
    return get_byte;
}


/*******************************************************************************
* // Adapter for Winmo typedef 
********************************************************************************/
#define WINMO_USE 0

#define Sleep(ms) mdelay(ms)
#define RETAILMSG(x,...)
#define TEXT


/*******************************************************************************
* follow is define by jun
********************************************************************************/
MSDK_SENSOR_CONFIG_STRUCT HI704SensorConfigData;

static struct HI704_sensor_STRUCT HI704_sensor;
static kal_uint32 HI704_zoom_factor = 0; 
static int sensor_id_fail = 0;	
const HI704_SENSOR_INIT_INFO HI704_Initial_Setting_Info[] =
{
	{0x01, 0xf1},
	{0x01, 0xf3},
	{0x01, 0xf1},

	{0x03, 0x00},
	{0x10, 0x00},
	{0x11, 0x93},
	{0x12, 0x04},     //0x00_20120214
	{0x20, 0x00},
	{0x21, 0x04},
	{0x22, 0x00},
	{0x23, 0x04},

	{0x40, 0x01},
	{0x41, 0x58},

	{0x42, 0x01}, //max fps 15
	{0x43, 0x13},   //0x14_20120214
	//BLC
	{0x80, 0x2e},
	{0x81, 0x7e},
	{0x82, 0x90},
	{0x83, 0x30},
	{0x84, 0x20},
	{0x85, 0x0b},
	{0x89, 0x48},//BLC hold
	{0x90, 0x0b},//TIME_IN
	{0x91, 0x0b},//TIME_OUT

	{0x92, 0x48},//AG_IN
	{0x93, 0x48},//AG_OUT
	{0x98, 0x38},
	{0x99, 0x43}, //Out BLC 		 //0x48_20110812
	{0xa0, 0x40}, //Dark BLC		//0x48_20110812
	{0xa8, 0x43}, //Normal BLC	   //0x48_20110812

	//Page 2  Last Update 10_03_12
	{0x03, 0x02},
	{0x20, 0x33},
	{0x21, 0x77},
	{0x22, 0xa7},
	{0x23, 0x32},
	{0x52, 0xa2},
	{0x53, 0x0a},
	{0x55, 0x18},
	{0x56, 0x0c},
	{0x60, 0xca},
	{0x61, 0xdb},
	{0x62, 0xca},
	{0x63, 0xda},
	{0x64, 0xca},
	{0x65, 0xda},

	{0x72, 0xcb},
	{0x73, 0xd8},
	{0x74, 0xcb},
	{0x75, 0xd8},
	{0x80, 0x02},
	{0x81, 0xbd},
	{0x82, 0x24},
	{0x83, 0x3e},
	{0x84, 0x24},
	{0x85, 0x3e},
	{0x92, 0x72},
	{0x93, 0x8c},
	{0x94, 0x72},
	{0x95, 0x8c},
	{0xa0, 0x03},
	{0xa1, 0xbb},
	{0xa5, 0x03},
	{0xa4, 0xbb},
	{0xa8, 0x44},
	{0xa9, 0x6a},
	{0xaa, 0x92},
	{0xab, 0xb7},
	{0xb8, 0xc9},
	{0xb9, 0xd0},
	{0xbc, 0x20},
	{0xbd, 0x28},
	{0xc0, 0xe0},
	{0xc1, 0xea},
	{0xc2, 0xe0},
	{0xc3, 0xea},
	{0xc4, 0xe1},
	{0xc5, 0xe9},
	{0xc6, 0xe1},
	{0xc7, 0xe9},
	{0xc8, 0xe1},
	{0xc9, 0xe8},
	{0xca, 0xe1},
	{0xcb, 0xe8},
	{0xcc, 0xe2},
	{0xcd, 0xe7},
	{0xce, 0xe2},
	{0xcf, 0xe7},
	{0xd0, 0xc8},
	{0xd1, 0xef},


	//Page 10
	{0x03, 0x10},
	{0x10, 0x02}, //03, //ISPCTL1, YUV ORDER(FIX)
	{0x11, 0x43},
	{0x12, 0x30}, //Y offet, dy offseet enable
#ifdef HQ_PROJECT_A61P_HUAWEI
	{0x40, 0x06},
	{0x41, 0x00},                   //0x10_20110812
#else
	{0x40, 0x01},
	{0x41, 0x01},                   //0x10_20110812
#endif
	{0x48, 0x98}, //Contrast    //0x80_20110812  //0xa8_20120214
	{0x50, 0x48}, //AGBRT

	{0x60, 0x7f},
	{0x61, 0x00}, //Use default
	{0x62, 0x88}, //SATB  (1.4x)   //0x98_20120214
#ifdef HQ_PROJECT_A61P_HUAWEI
	{0x63, 0x88}, //SATR  (1.2x)   //0x98_20120214
	
#else
	{0x63, 0xa0}, //SATR  (1.2x)   //0x98_20120214
	
#endif
	{0x64, 0x48}, //AGSAT
	{0x66, 0x90}, //wht_th2
	{0x67, 0x36}, //wht_gain  Dark (0.4x}, Normal (0.75x)//36

	//LPF
	{0x03, 0x11},
	{0x10, 0x25},	//LPF_CTL1 //0x01
	{0x11, 0x1f},	//Test Setting
	{0x20, 0x00},	//LPF_AUTO_CTL
	{0x21, 0x38},	//LPF_PGA_TH
	{0x23, 0x10},	//LPF_TIME_TH
	{0x60, 0x10},	//ZARA_SIGMA_TH //40->10	  //0x0a_20110812
	{0x61, 0x82},
	{0x62, 0x00},	//ZARA_HLVL_CTL
	{0x63, 0x80},	//ZARA_LLVL_CTL  
	{0x64, 0x83},	//ZARA_DY_CTL

	{0x67, 0x60},	//Dark
	{0x68, 0x10},	//Middle
	{0x69, 0x10},	//High

	//2D
	{0x03, 0x12},
	{0x40, 0xe9},	//YC2D_LPF_CTL1
	{0x41, 0x09},	//YC2D_LPF_CTL2
	{0x50, 0x18},	//Test Setting
	{0x51, 0x24},	//Test Setting
	{0x70, 0x1f},	//GBGR_CTL1 //0x1f
	{0x71, 0x00},	//Test Setting
	{0x72, 0x00},	//Test Setting
	{0x73, 0x00},	//Test Setting
	{0x74, 0x10},	//GBGR_G_UNIT_TH
	{0x75, 0x10},	//GBGR_RB_UNIT_TH
	{0x76, 0x20},	//GBGR_EDGE_TH
	{0x77, 0x80},	//GBGR_HLVL_TH
	{0x78, 0x88},	//GBGR_HLVL_COMP
	{0x79, 0x18},	//Test Setting
	{0xb0, 0x7d},	//dpc

	//Edge
	{0x03, 0x13},
	{0x10, 0x01},	
	{0x11, 0x89},	
	{0x12, 0x14},	
	{0x13, 0x19},	
	{0x14, 0x08},	//Test Setting
	{0x20, 0x06},	//SHARP_Negative      //0x07_20110812
	{0x21, 0x03},	//SHARP_Positive       //0x04_20110812
	{0x23, 0x30},	//SHARP_DY_CTL
	{0x24, 0x33},	//40->33
	{0x25, 0x08},	//SHARP_PGA_TH
	{0x26, 0x18},	//Test Setting
	{0x27, 0x00},	//Test Setting
	{0x28, 0x08},	//Test Setting
	{0x29, 0x50},	//AG_TH
	{0x2a, 0xe0},	//HI704_write_cmos_sensorion ratio
	{0x2b, 0x10},	//Test Setting
	{0x2c, 0x28},	//Test Setting
	{0x2d, 0x40},	//Test Setting
	{0x2e, 0x00},	//Test Setting
	{0x2f, 0x00},	//Test Setting
	{0x30, 0x11},	//Test Setting
	{0x80, 0x03},	//SHARP2D_CTL
	{0x81, 0x07},	//Test Setting
	{0x90, 0x04},	//SHARP2D_SLOPE 			 //0x06_20110812
	{0x91, 0x02},	//SHARP2D_DIFF_CTL		   //0x04_20110812
	{0x92, 0x00},	//SHARP2D_HI_CLIP
	{0x93, 0x20},	//SHARP2D_DY_CTL
	{0x94, 0x42},	//Test Setting
	{0x95, 0x60},	//Test Setting

	//Shading
	{0x03, 0x14},
	{0x10, 0x01},
	{0x20, 0x90}, //XCEN
	{0x21, 0x78}, //YCEN
	{0x22, 0x60}, //66
	{0x23, 0x50}, //50
	{0x24, 0x44}, //44

	//Page 15 CMC
	{0x03, 0x15}, 
	{0x10, 0x03},

	{0x14, 0x3c},
	{0x16, 0x2c},
	{0x17, 0x2f},

	{0x30, 0xcb},
	{0x31, 0x61},
	{0x32, 0x16},
	{0x33, 0x23},
	{0x34, 0xce},
	{0x35, 0x2b},
	{0x36, 0x01},
	{0x37, 0x34},
	{0x38, 0x75},

	{0x40, 0x87},
	{0x41, 0x18},
	{0x42, 0x91},
	{0x43, 0x94},
	{0x44, 0x9f},
	{0x45, 0x33},
	{0x46, 0x00},
	{0x47, 0x94},
	{0x48, 0x14},

	//Gamma
	//normal
#ifdef HQ_PROJECT_A61P_HUAWEI
	{0x03, 0x16},
	{0x30, 0x00},
	{0x31, 0x0b},
	{0x32, 0x20},
	{0x33, 0x36},
	{0x34, 0x5b},
	{0x35, 0x75},
    {0x36, 0x8c},
    {0x37, 0x9f},
    {0x38, 0xaf},
    {0x39, 0xbd},
    {0x3a, 0xca},
    {0x3b, 0xdd},
    {0x3c, 0xec},
    {0x3d, 0xf7},
	{0x3e, 0xff},
#else
	{0x03, 0x16},
	{0x30, 0x00},
	{0x31, 0x0a},
	{0x32, 0x1b},
	{0x33, 0x2e},
	{0x34, 0x5c},
	{0x35, 0x79},
    {0x36, 0x95},
    {0x37, 0xa4},
    {0x38, 0xb1},
    {0x39, 0xbd},
    {0x3a, 0xc8},
    {0x3b, 0xd9},
    {0x3c, 0xe8},
    {0x3d, 0xf5},
	{0x3e, 0xff},
#endif
	//Page 17 AE 
	{0x03, 0x17},
	{0xc4, 0x3c},
	{0xc5, 0x32},

	//Page 20 AE 
	{0x03, 0x20},
	{0x10, 0x0c},
	{0x11, 0x04},

	{0x20, 0x01},
	{0x28, 0x27},
	{0x29, 0xa1},

	{0x2a, 0xf0},
	{0x2b, 0x34},

	{0x30, 0xf8},

	{0x39, 0x22},
	{0x3a, 0xde},
	{0x3b, 0x23}, 
	{0x3c, 0xde},

	{0x60, 0x95}, 
	{0x68, 0x3c},
	{0x69, 0x64},
	{0x6A, 0x28},
	{0x6B, 0xc8},
#ifdef HQ_PROJECT_A61P_HUAWEI
	{0x70, 0x42},//Y Target 42
#else
	{0x70, 0x42},//Y Target 42
#endif
	{0x76, 0x22}, //Unlock bnd1
	{0x77, 0x02}, //Unlock bnd2

	{0x78, 0x12}, //Yth 1
	{0x79, 0x26}, //Yth 2
	{0x7a, 0x23}, //Yth 3

	{0x7c, 0x1c},
	{0x7d, 0x22},

	//50Hz
	{0x83, 0x00},//ExpTime 30fps
	{0x84, 0xbe},   //0xaf_20120214
	{0x85, 0x6e},   //0xc8_20120214

	{0x86, 0x00},//ExpMin
	{0x87, 0xfa},

	//50Hz_8fps
	{0x88, 0x03},//ExpMax 8fps(8fps)   //0x02_20110812
	{0x89, 0x78},							//0x49_20110812
	{0x8a, 0xac},							//0xf0_20110812

	{0x8b, 0x3f},//Exp100
	{0x8c, 0x7a},

	{0x8d, 0x34},//Exp120
	{0x8e, 0xbc},

	{0x91, 0x02},
	{0x92, 0xdc},
	{0x93, 0x6c},

	{0x94, 0x01}, //fix_step
	{0x95, 0xb7},
	{0x96, 0x74},

	{0x98, 0x8C},
	{0x99, 0x23},

	{0x9c, 0x0b}, //4shared limit	 //0x0b_20110812
	{0x9d, 0xb8},						//0x3b_20110812
	{0x9e, 0x00}, //4shared Unit
	{0x9f, 0xfa},	

	{0xb1, 0x14},
	{0xb2, 0x4a},
	{0xb4, 0x14},
	{0xb5, 0x38},
	{0xb6, 0x26},
	{0xb7, 0x20},
	{0xb8, 0x1d},
	{0xb9, 0x1b},
	{0xba, 0x1a},
	{0xbb, 0x19},
	{0xbc, 0x19},
	{0xbd, 0x18},

	{0xc0, 0x1a},
	{0xc3, 0x48},
	{0xc4, 0x48},


	//Page 22 AWB
	{0x03, 0x22},
	{0x10, 0xe2},
	{0x11, 0x26},
	{0x20, 0x34},
	{0x21, 0x40},

	{0x30, 0x80},
	{0x31, 0x80},
	{0x38, 0x12},
	{0x39, 0x33},
	{0x40, 0xf0},
	{0x41, 0x44},
	{0x42, 0x44},
	{0x43, 0xf3},
	{0x44, 0x88},
	{0x45, 0x66},
	{0x46, 0x02},

	{0x80, 0x3a},
	{0x81, 0x20},
	{0x82, 0x40},
#ifdef HQ_PROJECT_A61P_HUAWEI
	{0x83, 0x56}, //RMAX Default : 50 -> 48 -> 52 
#else
	{0x83, 0x5e}, //RMAX Default : 50 -> 48 -> 52 
#endif
	{0x84, 0x15}, //RMIN Default : 20
#ifdef HQ_PROJECT_A61P_HUAWEI
	{0x85, 0x5e}, //BMAX Default : 50, 5a -> 58 -> 55
#else
	{0x85, 0x5e}, //BMAX Default : 50, 5a -> 58 -> 55
#endif
	{0x86, 0x25}, //RMAXB Default : 50, 4d
	{0x87, 0x4d},
	{0x88, 0x38}, //RMINB Default : 3e, 45 --> 42
	{0x89, 0x3e}, //BMAXB Default : 2e, 2d --> 30
	{0x8a, 0x29}, //BMINB Default : 20, 22 --> 26 --> 29
	{0x8b, 0x02}, //OUT TH
	{0x8d, 0x22},
	{0x8e, 0x71},

	{0x8f, 0x5c},
	{0x90, 0x59},
	{0x91, 0x55},
	{0x92, 0x50},
	{0x93, 0x48},
	{0x94, 0x3e},
	{0x95, 0x37},
	{0x96, 0x30},
	{0x97, 0x29},
	{0x98, 0x26},
	{0x99, 0x20},
	{0x9a, 0x1a},
	{0x9b, 0x0b},

    //PAGE 22
    {0x03, 0x22},
    {0x10, 0xfb},

	{0x03, 0x20},
	{0x10, 0x9c},

	{0x01, 0xf0},
	{0xff, 0xff}	//End of Initial Setting

};
static void HI704_Initial_Setting(void)
{
	kal_uint32 iEcount;
	for(iEcount=0;(!((0xff==(HI704_Initial_Setting_Info[iEcount].address))&&(0xff==(HI704_Initial_Setting_Info[iEcount].data))));iEcount++)
	{	
		HI704_write_cmos_sensor(HI704_Initial_Setting_Info[iEcount].address, HI704_Initial_Setting_Info[iEcount].data);
	}
}

static void HI704_Init_Parameter(void)
{
    spin_lock(&hi704_yuv_drv_lock);
    HI704_sensor.first_init = KAL_TRUE;
    HI704_sensor.pv_mode = KAL_TRUE;
    HI704_sensor.night_mode = KAL_FALSE;
    HI704_sensor.MPEG4_Video_mode = KAL_FALSE;

    HI704_sensor.cp_pclk = HI704_sensor.pv_pclk;

    HI704_sensor.pv_dummy_pixels = 0;
    HI704_sensor.pv_dummy_lines = 0;
    HI704_sensor.cp_dummy_pixels = 0;
    HI704_sensor.cp_dummy_lines = 0;

    HI704_sensor.wb = 0;
    HI704_sensor.exposure = 0;
    HI704_sensor.effect = 0;
    HI704_sensor.banding = AE_FLICKER_MODE_50HZ;

    HI704_sensor.pv_line_length = 640;
    HI704_sensor.pv_frame_height = 480;
    HI704_sensor.cp_line_length = 640;
    HI704_sensor.cp_frame_height = 480;
    spin_unlock(&hi704_yuv_drv_lock);    
}

static kal_uint8 HI704_power_on(void)
{
    kal_uint8 HI704_sensor_id = 0;
    spin_lock(&hi704_yuv_drv_lock);
    HI704_sensor.pv_pclk = 13000000;
    spin_unlock(&hi704_yuv_drv_lock);
    //Software Reset
    HI704_write_cmos_sensor(0x01,0xf1);
    HI704_write_cmos_sensor(0x01,0xf3);
    HI704_write_cmos_sensor(0x01,0xf1);

    /* Read Sensor ID  */
    HI704_sensor_id = HI704_read_cmos_sensor(0x04);
    SENSORDB("[HI704YUV]:read Sensor ID:%x\n",HI704_sensor_id);	
    return HI704_sensor_id;
}


/*************************************************************************
* FUNCTION
*	HI704Open
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
UINT32 HI704Open(void)
{
    spin_lock(&hi704_yuv_drv_lock);
    sensor_id_fail = 0; 
    spin_unlock(&hi704_yuv_drv_lock);
    SENSORDB("[Enter]:HI704 Open func:");

    if (HI704_power_on() != HI704_SENSOR_ID) 
    {
        SENSORDB("[HI704]Error:read sensor ID fail\n");
        spin_lock(&hi704_yuv_drv_lock);
        sensor_id_fail = 1;
        spin_unlock(&hi704_yuv_drv_lock);
        return ERROR_SENSOR_CONNECT_FAIL;
    }

    /* Apply sensor initail setting*/
    HI704_Initial_Setting();
    HI704_Init_Parameter(); 

    SENSORDB("[Exit]:HI704 Open func\n");     
    return ERROR_NONE;
}	/* HI704Open() */

/*************************************************************************
* FUNCTION
*	HI704_GetSensorID
*
* DESCRIPTION
*	This function get the sensor ID
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
static kal_uint32 HI704_GetSensorID(kal_uint32 *sensorID)
{
    SENSORDB("[Enter]:HI704 Open func ");
    *sensorID = HI704_power_on() ;

    if (*sensorID != HI704_SENSOR_ID) 
    {
        SENSORDB("[HI704]Error:read sensor ID fail\n");
        spin_lock(&hi704_yuv_drv_lock);
        sensor_id_fail = 1;
        spin_unlock(&hi704_yuv_drv_lock);
        *sensorID = 0xFFFFFFFF; 
        return ERROR_SENSOR_CONNECT_FAIL;
    }	   

    return ERROR_NONE;    
}   /* HI704Open  */


/*************************************************************************
* FUNCTION
*	HI704Close
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
UINT32 HI704Close(void)
{

	return ERROR_NONE;
}	/* HI704Close() */


static void HI704_Set_Mirror_Flip(kal_uint8 image_mirror)
{
    /********************************************************
    * Page Mode 0: Reg 0x0011 bit[1:0] = [Y Flip : X Flip]
    * 0: Off; 1: On.
    *********************************************************/    
    SENSORDB("[Enter]:HI704 set Mirror_flip func:image_mirror=%d\n",image_mirror);
	
    HI704_write_cmos_sensor(0x03,0x00);     //Page 0	
    
    spin_lock(&hi704_yuv_drv_lock);
    HI704_sensor.mirror = (HI704_read_cmos_sensor(0x11) & 0xfc); 

    switch (image_mirror) 
    {
     #ifdef HQ_PROJECT_A61P_HUAWEI
        case IMAGE_NORMAL:
		    HI704_sensor.mirror |= 0x03;
	        break;
	    case IMAGE_H_MIRROR:
		    HI704_sensor.mirror |= 0x01;
	        break;
	    case IMAGE_V_MIRROR:
		    HI704_sensor.mirror |= 0x02;
	        break;
    	case IMAGE_HV_MIRROR:
		    HI704_sensor.mirror |= 0x00;
	        break;
    	default:
	        HI704_sensor.mirror |= 0x03;
     #else
	 case IMAGE_NORMAL:
                    HI704_sensor.mirror |= 0x00;
                break;
            case IMAGE_H_MIRROR:
                    HI704_sensor.mirror |= 0x01;
                break;
            case IMAGE_V_MIRROR:
                    HI704_sensor.mirror |= 0x02;
                break;
        case IMAGE_HV_MIRROR:
                    HI704_sensor.mirror |= 0x03;
                break;
        default:
                HI704_sensor.mirror |= 0x00;
     #endif
    }
    
    spin_unlock(&hi704_yuv_drv_lock);
    HI704_write_cmos_sensor(0x11, HI704_sensor.mirror);
    SENSORDB("[Exit]:HI704 set Mirror_flip func\n");
}

static void HI704_set_dummy(kal_uint16 dummy_pixels,kal_uint16 dummy_lines)
{	
    HI704_write_cmos_sensor(0x03, 0x00);                        //Page 0
    HI704_write_cmos_sensor(0x40,((dummy_pixels & 0x0F00))>>8);       //HBLANK
    HI704_write_cmos_sensor(0x41,(dummy_pixels & 0xFF));
    HI704_write_cmos_sensor(0x42,((dummy_lines & 0xFF00)>>8));       //VBLANK ( Vsync Type 1)
    HI704_write_cmos_sensor(0x43,(dummy_lines & 0xFF));
}  


// 640 * 480
static void HI704_Set_VGA_mode(void)
{
    HI704_write_cmos_sensor(0x01, HI704_read_cmos_sensor(0x01)|0x01);   //Sleep: For Write Reg

    HI704_write_cmos_sensor(0x03, 0x00);
    HI704_write_cmos_sensor(0x10, 0x00);        //VGA Size

    HI704_write_cmos_sensor(0x20, 0x00);
    HI704_write_cmos_sensor(0x21, 0x04);

    HI704_write_cmos_sensor(0x40, 0x01);        //HBLANK: 0x70 = 112
    HI704_write_cmos_sensor(0x41, 0x58);
    HI704_write_cmos_sensor(0x42, 0x00);        //VBLANK: 0x04 = 4
    HI704_write_cmos_sensor(0x43, 0x13);

    HI704_write_cmos_sensor(0x03, 0x11);
    HI704_write_cmos_sensor(0x10, 0x25);  

    HI704_write_cmos_sensor(0x03, 0x20);
    HI704_pv_exposure_lines = (HI704_read_cmos_sensor(0x80) << 16)|(HI704_read_cmos_sensor(0x81) << 8)|HI704_read_cmos_sensor(0x82);


    HI704_write_cmos_sensor(0x03, 0x20);

    HI704_write_cmos_sensor(0x10, 0x1c);   //Close AE
    HI704_write_cmos_sensor(0x18, HI704_read_cmos_sensor(0x18)|0x08);   //Reset AE
#ifdef HQ_PROJECT_A61P_HUAWEI
    HI704_write_cmos_sensor(0x83, HI704_pv_exposure_lines >> 16);
    HI704_write_cmos_sensor(0x84, (HI704_pv_exposure_lines >> 8) & 0x000000FF);
    HI704_write_cmos_sensor(0x85, HI704_pv_exposure_lines & 0x000000FF);
#else	
    HI704_write_cmos_sensor(0x83, HI704_pv_exposure_lines >> 16);
    HI704_write_cmos_sensor(0x84, (HI704_pv_exposure_lines >> 8) & 0x000000FF);
    HI704_write_cmos_sensor(0x85, HI704_pv_exposure_lines & 0x000000FF);
#endif	
    HI704_write_cmos_sensor(0x86, 0x00);
    HI704_write_cmos_sensor(0x87, 0xfa);

    HI704_write_cmos_sensor(0x8b, 0x3f);
    HI704_write_cmos_sensor(0x8c, 0x7a);
    HI704_write_cmos_sensor(0x8d, 0x34);
    HI704_write_cmos_sensor(0x8e, 0xbc);

    HI704_write_cmos_sensor(0x9c, 0x0b);
    HI704_write_cmos_sensor(0x9d, 0xb8);
    HI704_write_cmos_sensor(0x9e, 0x00);
    HI704_write_cmos_sensor(0x9f, 0xfa);

    HI704_write_cmos_sensor(0x01, HI704_read_cmos_sensor(0x01)&0xfe);   //Exit Sleep: For Write Reg

    HI704_write_cmos_sensor(0x03, 0x20);
    HI704_write_cmos_sensor(0x10, 0x9c);   //Open AE
    HI704_write_cmos_sensor(0x18, HI704_read_cmos_sensor(0x18)&0xf7);   //Reset AE

}

void HI704_night_mode(kal_bool enable)
{

	SENSORDB("[Enter]HI704 night mode func:enable = %d\n",enable);
	SENSORDB("HI704_sensor.video_mode = %d\n",HI704_sensor.MPEG4_Video_mode); 
        SENSORDB("HI704_sensor.night_mode = %d\n",HI704_sensor.night_mode);

	HI704_sensor.night_mode = enable;


	if(enable)
	{
        //HI704_Cal_Min_Frame_Rate(HI704_MIN_FRAMERATE_5); 
		HI704_write_cmos_sensor(0x03, 0x00); 	
		HI704_write_cmos_sensor(0x01, 0xf1);
		
		HI704_write_cmos_sensor(0x03, 0x20);
		HI704_write_cmos_sensor(0x10, 0x1c);
		
		HI704_write_cmos_sensor(0x03, 0x20);
		//HI704_write_cmos_sensor(0x18, 0x38);

		HI704_write_cmos_sensor(0x88, 0x08);
		HI704_write_cmos_sensor(0x89, 0x2e);
		HI704_write_cmos_sensor(0x8a, 0xba);
			
		HI704_write_cmos_sensor(0x03, 0x00); 	
		HI704_write_cmos_sensor(0x01, 0xf0);

		HI704_write_cmos_sensor(0x03, 0x20);
		HI704_write_cmos_sensor(0x10, 0x9c);

		//HI704_write_cmos_sensor(0x18, 0x30);
       }
	else
	{
	    //HI704_Cal_Min_Frame_Rate(HI704_MIN_FRAMERATE_10);
		HI704_write_cmos_sensor(0x03, 0x00); 	
		HI704_write_cmos_sensor(0x01, 0xf1);
		
		HI704_write_cmos_sensor(0x03, 0x20);
		HI704_write_cmos_sensor(0x10, 0x1c);
		
		HI704_write_cmos_sensor(0x03, 0x20);
		//HI704_write_cmos_sensor(0x18, 0x38);

		HI704_write_cmos_sensor(0x88, 0x04);
		HI704_write_cmos_sensor(0x89, 0xf5);
		HI704_write_cmos_sensor(0x8a, 0x88);
			
		HI704_write_cmos_sensor(0x03, 0x00); 	
		HI704_write_cmos_sensor(0x01, 0xf0);

		HI704_write_cmos_sensor(0x03, 0x20);
		HI704_write_cmos_sensor(0x10, 0x9c);

		//HI704_write_cmos_sensor(0x18, 0x30);
    }
}

/*************************************************************************
* FUNCTION
*	HI704Preview
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
static UINT32 HI704Preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    spin_lock(&hi704_yuv_drv_lock);
    //sensor_config_data->SensorImageMirror = IMAGE_HV_MIRROR; 
    if(HI704_sensor.first_init == KAL_TRUE)
    {
        HI704_sensor.MPEG4_Video_mode = HI704_sensor.MPEG4_Video_mode;
    }
    else
    {
        HI704_sensor.MPEG4_Video_mode = !HI704_sensor.MPEG4_Video_mode;
    }
    spin_unlock(&hi704_yuv_drv_lock);

    SENSORDB("[Enter]:HI704 preview func:");		
    SENSORDB("HI704_sensor.video_mode = %d\n",HI704_sensor.MPEG4_Video_mode); 

    spin_lock(&hi704_yuv_drv_lock);
    HI704_sensor.first_init = KAL_FALSE;	
    HI704_sensor.pv_mode = KAL_TRUE;		
    spin_unlock(&hi704_yuv_drv_lock);

    {   
        SENSORDB("[HI704]preview set_VGA_mode\n");
        HI704_Set_VGA_mode();
    }

    HI704_Set_Mirror_Flip(sensor_config_data->SensorImageMirror);

    SENSORDB("[Exit]:HI704 preview func\n");
    return TRUE; 
}	/* HI704_Preview */


UINT32 HI704Capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    SENSORDB("[HI704][Enter]HI704_capture_func\n");
    spin_lock(&hi704_yuv_drv_lock);
    HI704_sensor.pv_mode = KAL_FALSE;	
    spin_unlock(&hi704_yuv_drv_lock);

    return ERROR_NONE;
}	/* HM3451Capture() */


UINT32 HI704GetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{
    SENSORDB("[Enter]:HI704 get Resolution func\n");

    pSensorResolution->SensorFullWidth=HI704_IMAGE_SENSOR_FULL_WIDTH - 10;  
    pSensorResolution->SensorFullHeight=HI704_IMAGE_SENSOR_FULL_HEIGHT - 10-10;
    pSensorResolution->SensorPreviewWidth=HI704_IMAGE_SENSOR_PV_WIDTH - 16;
    pSensorResolution->SensorPreviewHeight=HI704_IMAGE_SENSOR_PV_HEIGHT - 12-10;

    SENSORDB("[Exit]:HI704 get Resolution func\n");	
    return ERROR_NONE;
}	/* HI704GetResolution() */

UINT32 HI704GetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
					  MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
					  MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
    SENSORDB("[Enter]:HI704 getInfo func:ScenarioId = %d\n",ScenarioId);

    pSensorInfo->SensorPreviewResolutionX=HI704_IMAGE_SENSOR_PV_WIDTH;
    pSensorInfo->SensorPreviewResolutionY=HI704_IMAGE_SENSOR_PV_HEIGHT;
    pSensorInfo->SensorFullResolutionX=HI704_IMAGE_SENSOR_FULL_WIDTH;
    pSensorInfo->SensorFullResolutionY=HI704_IMAGE_SENSOR_FULL_HEIGHT;

	pSensorInfo->SensorCameraPreviewFrameRate=30;
	pSensorInfo->SensorVideoFrameRate=30;
	pSensorInfo->SensorStillCaptureFrameRate=30;
	pSensorInfo->SensorWebCamCaptureFrameRate=15;
	pSensorInfo->SensorResetActiveHigh=FALSE;//low is to reset 
	pSensorInfo->SensorResetDelayCount=4;  //4ms 
	pSensorInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_YVYU;
	pSensorInfo->SensorClockPolarity=SENSOR_CLOCK_POLARITY_LOW;	
	pSensorInfo->SensorClockFallingPolarity=SENSOR_CLOCK_POLARITY_LOW;
	pSensorInfo->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	pSensorInfo->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	pSensorInfo->SensorInterruptDelayLines = 1; 
	pSensorInfo->SensroInterfaceType=SENSOR_INTERFACE_TYPE_PARALLEL;

    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_100_MODE].MaxWidth=CAM_SIZE_VGA_WIDTH; //???
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_100_MODE].MaxHeight=CAM_SIZE_VGA_HEIGHT;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_100_MODE].ISOSupported=TRUE;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_100_MODE].BinningEnable=FALSE;

    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_200_MODE].MaxWidth=CAM_SIZE_VGA_WIDTH;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_200_MODE].MaxHeight=CAM_SIZE_VGA_HEIGHT;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_200_MODE].ISOSupported=TRUE;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_200_MODE].BinningEnable=FALSE;

    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_400_MODE].MaxWidth=CAM_SIZE_VGA_WIDTH;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_400_MODE].MaxHeight=CAM_SIZE_VGA_HEIGHT;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_400_MODE].ISOSupported=TRUE;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_400_MODE].BinningEnable=FALSE;

    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_800_MODE].MaxWidth=CAM_SIZE_VGA_WIDTH;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_800_MODE].MaxHeight=CAM_SIZE_VGA_HEIGHT;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_800_MODE].ISOSupported=TRUE;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_800_MODE].BinningEnable=TRUE;

    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].MaxWidth=CAM_SIZE_VGA_WIDTH;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].MaxHeight=CAM_SIZE_VGA_HEIGHT;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].ISOSupported=TRUE;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].BinningEnable=TRUE;

    pSensorInfo->CaptureDelayFrame = 4; 
    pSensorInfo->PreviewDelayFrame = 4;//10; 
    pSensorInfo->VideoDelayFrame = 0; 
    pSensorInfo->SensorMasterClockSwitch = 0; 
    pSensorInfo->SensorDrivingCurrent = ISP_DRIVING_8MA;   	

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
        pSensorInfo->SensorGrabStartX = 1; 
        pSensorInfo->SensorGrabStartY = 10;  	
        break;
    case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
    case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
        pSensorInfo->SensorClockFreq=26;
        pSensorInfo->SensorClockDividCount=	3;
        pSensorInfo->SensorClockRisingCount= 0;
        pSensorInfo->SensorClockFallingCount= 2;
        pSensorInfo->SensorPixelClockCount= 3;
        pSensorInfo->SensorDataLatchCount= 2;
        pSensorInfo->SensorGrabStartX = 1; 
        pSensorInfo->SensorGrabStartY = 10;//1;     			
        break;
    default:
        pSensorInfo->SensorClockFreq=26;
        pSensorInfo->SensorClockDividCount=3;
        pSensorInfo->SensorClockRisingCount=0;
        pSensorInfo->SensorClockFallingCount=2;
        pSensorInfo->SensorPixelClockCount=3;
        pSensorInfo->SensorDataLatchCount=2;
        pSensorInfo->SensorGrabStartX = 1; 
        pSensorInfo->SensorGrabStartY = 10;//1;     			
        break;
    }
    //	HI704_PixelClockDivider=pSensorInfo->SensorPixelClockCount;
    memcpy(pSensorConfigData, &HI704SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));

    SENSORDB("[Exit]:HI704 getInfo func\n");	
    return ERROR_NONE;
}	/* HI704GetInfo() */


UINT32 HI704Control(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
					  MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
    SENSORDB("[Enter]:HI704 Control func:ScenarioId = %d\n",ScenarioId);

    switch (ScenarioId)
    {
    case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
    case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
    case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:
        HI704Preview(pImageWindow, pSensorConfigData); 
        break;
    case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
    case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
        HI704Capture(pImageWindow, pSensorConfigData); 
        break;
    default:
        break; 
    }

    SENSORDB("[Exit]:HI704 Control func\n");	
    return TRUE;
}	/* HI704Control() */


/*************************************************************************
* FUNCTION
*	HI704_set_param_wb
*
* DESCRIPTION
*	wb setting.
*
* PARAMETERS
*	none
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
BOOL HI704_set_param_wb(UINT16 para)
{
    //This sensor need more time to balance AWB, 
    //we suggest higher fps or drop some frame to avoid garbage color when preview initial
    SENSORDB("[Enter]HI704 set_param_wb func:para = %d\n",para);

    if(HI704_sensor.wb == para) return KAL_TRUE;	

    spin_lock(&hi704_yuv_drv_lock);
    HI704_sensor.wb = para;
    spin_unlock(&hi704_yuv_drv_lock);
    
    switch (para)
    {            
    case AWB_MODE_AUTO:
        {
        HI704_write_cmos_sensor(0x03, 0x22);			
        HI704_write_cmos_sensor(0x11, 0x2e);				
        //HI704_write_cmos_sensor(0x80, 0x38);
        //HI704_write_cmos_sensor(0x82, 0x38);				
#ifdef HQ_PROJECT_A61P_HUAWEI				
		        HI704_write_cmos_sensor(0x83, 0x56); 
#else
		 	HI704_write_cmos_sensor(0x83, 0x5e);  
#endif
		        HI704_write_cmos_sensor(0x84, 0x15);   //0x1e_20120214
#ifdef HQ_PROJECT_A61P_HUAWEI
	            	HI704_write_cmos_sensor(0x85, 0x5e);  //0x52_20120214
#else
			HI704_write_cmos_sensor(0x85, 0x4f);  //0x52_20120214
#endif
		        HI704_write_cmos_sensor(0x86, 0x1c);
        }                
        break;
    case AWB_MODE_CLOUDY_DAYLIGHT:
    {
        HI704_write_cmos_sensor(0x03, 0x22);
        HI704_write_cmos_sensor(0x11, 0x28);
        HI704_write_cmos_sensor(0x80, 0x4a);//71
		HI704_write_cmos_sensor(0x82, 0x2a);//2b
		HI704_write_cmos_sensor(0x83, 0x4c);//72
		HI704_write_cmos_sensor(0x84, 0x4a);//70
		HI704_write_cmos_sensor(0x85, 0x2a);//2b
        HI704_write_cmos_sensor(0x86, 0x28);
        }			   
        break;
    case AWB_MODE_DAYLIGHT:
        {
        HI704_write_cmos_sensor(0x03, 0x22);
        HI704_write_cmos_sensor(0x11, 0x28);          
        HI704_write_cmos_sensor(0x80, 0x59);
        HI704_write_cmos_sensor(0x82, 0x29);
        HI704_write_cmos_sensor(0x83, 0x60);
        HI704_write_cmos_sensor(0x84, 0x50);
        HI704_write_cmos_sensor(0x85, 0x2f);
        HI704_write_cmos_sensor(0x86, 0x23);
        }      
        break;
    case AWB_MODE_INCANDESCENT:	
        {
        HI704_write_cmos_sensor(0x03, 0x22);
        HI704_write_cmos_sensor(0x11, 0x28);          
        HI704_write_cmos_sensor(0x80, 0x29);
        HI704_write_cmos_sensor(0x82, 0x54);
        HI704_write_cmos_sensor(0x83, 0x2e);
        HI704_write_cmos_sensor(0x84, 0x23);
        HI704_write_cmos_sensor(0x85, 0x58);
        HI704_write_cmos_sensor(0x86, 0x4f);
        }		
        break;  
    case AWB_MODE_FLUORESCENT:
        {
        HI704_write_cmos_sensor(0x03, 0x22);
        HI704_write_cmos_sensor(0x11, 0x28);
        HI704_write_cmos_sensor(0x80, 0x41);
        HI704_write_cmos_sensor(0x82, 0x42);
        HI704_write_cmos_sensor(0x83, 0x44);
        HI704_write_cmos_sensor(0x84, 0x34);
        HI704_write_cmos_sensor(0x85, 0x46);
        HI704_write_cmos_sensor(0x86, 0x3a);
        }	
        break;  
    case AWB_MODE_TUNGSTEN:
        {
        HI704_write_cmos_sensor(0x03, 0x22);
        HI704_write_cmos_sensor(0x80, 0x24);
        HI704_write_cmos_sensor(0x81, 0x20);
        HI704_write_cmos_sensor(0x82, 0x58);
        HI704_write_cmos_sensor(0x83, 0x27);
        HI704_write_cmos_sensor(0x84, 0x22);
        HI704_write_cmos_sensor(0x85, 0x58);
        HI704_write_cmos_sensor(0x86, 0x52);
        }
        break;
    default:
        return FALSE;
    }

    return TRUE;	
} /* HI704_set_param_wb */

/*************************************************************************
* FUNCTION
*	HI704_set_param_effect
*
* DESCRIPTION
*	effect setting.
*
* PARAMETERS
*	none
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
BOOL HI704_set_param_effect(UINT16 para)
{
   SENSORDB("[Enter]HI704 set_param_effect func:para = %d\n",para);
   
    if(HI704_sensor.effect == para) return KAL_TRUE;

    spin_lock(&hi704_yuv_drv_lock);
    HI704_sensor.effect = para;
    spin_unlock(&hi704_yuv_drv_lock);
    
    switch (para)
    {
    case MEFFECT_OFF:
        {
        HI704_write_cmos_sensor(0x03, 0x10);
        HI704_write_cmos_sensor(0x11, 0x03);
        HI704_write_cmos_sensor(0x12, 0x30);
        HI704_write_cmos_sensor(0x13, 0x00);
        HI704_write_cmos_sensor(0x44, 0x80);
        HI704_write_cmos_sensor(0x45, 0x80);

        HI704_write_cmos_sensor(0x47, 0x7f);
        HI704_write_cmos_sensor(0x03, 0x13);
        HI704_write_cmos_sensor(0x20, 0x07);
        HI704_write_cmos_sensor(0x21, 0x07);
        }
        break;
    case MEFFECT_SEPIA:
        {
        HI704_write_cmos_sensor(0x03, 0x10);
        HI704_write_cmos_sensor(0x11, 0x03);
        HI704_write_cmos_sensor(0x12, 0x23);
        HI704_write_cmos_sensor(0x13, 0x00);
        HI704_write_cmos_sensor(0x44, 0x70);
        HI704_write_cmos_sensor(0x45, 0x98);

	            HI704_write_cmos_sensor(0x47, 0x7f);
	            HI704_write_cmos_sensor(0x03, 0x13);
	            HI704_write_cmos_sensor(0x20, 0x07);
	            HI704_write_cmos_sensor(0x21, 0x07);
            }	
			break;  
		case MEFFECT_NEGATIVE:
			{
	            HI704_write_cmos_sensor(0x03, 0x10);
	            HI704_write_cmos_sensor(0x11, 0x03);
	            HI704_write_cmos_sensor(0x12, 0x08);
	            HI704_write_cmos_sensor(0x13, 0x00);
	            HI704_write_cmos_sensor(0x14, 0x00);
            }
			break; 
		case MEFFECT_SEPIAGREEN:		
			{
	            HI704_write_cmos_sensor(0x03, 0x10);
	            HI704_write_cmos_sensor(0x11, 0x03);
	            HI704_write_cmos_sensor(0x12, 0x03);
	            HI704_write_cmos_sensor(0x40, 0x00);
	            HI704_write_cmos_sensor(0x13, 0x00);
	            HI704_write_cmos_sensor(0x44, 0x30);
	            HI704_write_cmos_sensor(0x45, 0x50);
            }	
			break;
		case MEFFECT_SEPIABLUE:
			{
			  	HI704_write_cmos_sensor(0x03, 0x10);
				HI704_write_cmos_sensor(0x11, 0x03);
				HI704_write_cmos_sensor(0x12, 0x03);
				HI704_write_cmos_sensor(0x40, 0x00);
				HI704_write_cmos_sensor(0x13, 0x00);
				HI704_write_cmos_sensor(0x44, 0xb0);
				HI704_write_cmos_sensor(0x45, 0x40);
		    }     
			break;        
		case MEFFECT_MONO:			
			{
				HI704_write_cmos_sensor(0x03, 0x10);
				HI704_write_cmos_sensor(0x11, 0x03);
				HI704_write_cmos_sensor(0x12, 0x03);
				HI704_write_cmos_sensor(0x40, 0x00);
				HI704_write_cmos_sensor(0x44, 0x80);
				HI704_write_cmos_sensor(0x45, 0x80);
            }
			break;

		default:
			return KAL_FALSE;
	}

    return KAL_TRUE;
} /* HI704_set_param_effect */

/*************************************************************************
* FUNCTION
*	HI704_set_param_banding
*
* DESCRIPTION
*	banding setting.
*
* PARAMETERS
*	none
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
BOOL HI704_set_param_banding(UINT16 para)
{
    SENSORDB("[Enter]HI704 set_param_banding func:para = %d\n",para);

    if(HI704_sensor.banding == para) return KAL_TRUE;

    spin_lock(&hi704_yuv_drv_lock);
    HI704_sensor.banding = para;
    spin_unlock(&hi704_yuv_drv_lock);

    switch (para)
    {
    case AE_FLICKER_MODE_50HZ:
        {
        HI704_write_cmos_sensor(0x03,0x20);
        HI704_write_cmos_sensor(0x10,0x9c);
        }
        break;
    case AE_FLICKER_MODE_60HZ:
        {
        HI704_write_cmos_sensor(0x03,0x20);
        HI704_write_cmos_sensor(0x10,0x8c);
        }
        break;
    default:
        return KAL_FALSE;
    }
    
    return KAL_TRUE;
} /* HI704_set_param_banding */




/*************************************************************************
* FUNCTION
*	HI704_set_param_exposure
*
* DESCRIPTION
*	exposure setting.
*
* PARAMETERS
*	none
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
BOOL HI704_set_param_exposure(UINT16 para)
{
    SENSORDB("[Enter]HI704 set_param_exposure func:para = %d\n",para);

    if(HI704_sensor.exposure == para) return KAL_TRUE;

    spin_lock(&hi704_yuv_drv_lock);
    HI704_sensor.exposure = para;
    spin_unlock(&hi704_yuv_drv_lock);

	   HI704_write_cmos_sensor(0x03,0x10);
	   HI704_write_cmos_sensor(0x12,HI704_read_cmos_sensor(0x12)|0x10);
	switch (para)
	{
		case AE_EV_COMP_13:  //+4 EV
			HI704_write_cmos_sensor(0x40,0x60);
			break;  
		case AE_EV_COMP_10:  //+3 EV
			HI704_write_cmos_sensor(0x40,0x48);
			break;    
		case AE_EV_COMP_07:  //+2 EV
			HI704_write_cmos_sensor(0x40,0x30);
			break;    
		case AE_EV_COMP_03:	 //	+1 EV	
			HI704_write_cmos_sensor(0x40,0x18);	
			break;    
		case AE_EV_COMP_00:  // +0 EV
#ifdef HQ_PROJECT_A61P_HUAWEI
		    HI704_write_cmos_sensor(0x40,0x06);   //0x00_20120214
#else
		    HI704_write_cmos_sensor(0x40,0x08);   //0x00_20120214
#endif
			break;    
		case AE_EV_COMP_n03:  // -1 EV
			HI704_write_cmos_sensor(0x40,0x98);
			break;    
		case AE_EV_COMP_n07:	// -2 EV		
			HI704_write_cmos_sensor(0x40,0xb0);	
			break;    
		case AE_EV_COMP_n10:   //-3 EV
			HI704_write_cmos_sensor(0x40,0xc8);
			break;
		case AE_EV_COMP_n13:  // -4 EV
			HI704_write_cmos_sensor(0x40,0xe0);
			break;
		default:
			return FALSE;
	}

    return TRUE;	
} /* HI704_set_param_exposure */


UINT32 HI704YUVSensorSetting(FEATURE_ID iCmd, UINT32 iPara)
{
    SENSORDB("[Enter]HI704YUVSensorSetting func:cmd = %d\n",iCmd);

    switch (iCmd) 
    {
    case FID_SCENE_MODE:	    //auto mode or night mode
        if (iPara == SCENE_MODE_OFF)//auto mode
        {
            HI704_night_mode(FALSE); 
        }
        else if (iPara == SCENE_MODE_NIGHTSCENE)//night mode
        {
            HI704_night_mode(TRUE); 
        }	
        break; 	    
    case FID_AWB_MODE:
        HI704_set_param_wb(iPara);
        break;
    case FID_COLOR_EFFECT:
        HI704_set_param_effect(iPara);
        break;
    case FID_AE_EV:	    	    
        HI704_set_param_exposure(iPara);
        break;
    case FID_AE_FLICKER:	    	    	    
        HI704_set_param_banding(iPara);
        break;
    case FID_ZOOM_FACTOR:
        spin_lock(&hi704_yuv_drv_lock);
        HI704_zoom_factor = iPara; 
        spin_unlock(&hi704_yuv_drv_lock);
        break; 
    default:
        break;
    }
    return TRUE;
}   /* HI704YUVSensorSetting */

UINT32 HI704YUVSetVideoMode(UINT16 u2FrameRate)
{
    spin_lock(&hi704_yuv_drv_lock);
    HI704_sensor.MPEG4_Video_mode = KAL_TRUE;
    spin_unlock(&hi704_yuv_drv_lock);
    SENSORDB("[Enter]HI704 Set Video Mode:FrameRate= %d\n",u2FrameRate);
    SENSORDB("HI704_sensor.video_mode = %d\n",HI704_sensor.MPEG4_Video_mode);

    //if(u2FrameRate == 30) u2FrameRate = 25;
   
	
	//HI704_sensor.fix_framerate = u2FrameRate * 10;
    
    if(HI704_sensor.fix_framerate <= 300 )
    {
     // HI704_Fix_Video_Frame_Rate(HI704_sensor.fix_framerate); 
    }
    else 
    {
       // SENSORDB("Wrong Frame Rate"); 
    }
        
    return TRUE;
}

UINT32 HI704FeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
							 UINT8 *pFeaturePara,UINT32 *pFeatureParaLen)
{
    UINT16 u2Temp = 0; 
	UINT16 *pFeatureReturnPara16=(UINT16 *) pFeaturePara;
	UINT16 *pFeatureData16=(UINT16 *) pFeaturePara;
	UINT32 *pFeatureReturnPara32=(UINT32 *) pFeaturePara;
	UINT32 *pFeatureData32=(UINT32 *) pFeaturePara;
	MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData=(MSDK_SENSOR_CONFIG_STRUCT *) pFeaturePara;
	MSDK_SENSOR_REG_INFO_STRUCT *pSensorRegData=(MSDK_SENSOR_REG_INFO_STRUCT *) pFeaturePara;

	switch (FeatureId)
	{
		case SENSOR_FEATURE_GET_RESOLUTION:
			*pFeatureReturnPara16++=HI704_IMAGE_SENSOR_FULL_WIDTH;
			*pFeatureReturnPara16=HI704_IMAGE_SENSOR_FULL_HEIGHT;
			*pFeatureParaLen=4;
		     break;
		case SENSOR_FEATURE_GET_PERIOD:
			*pFeatureReturnPara16++=HI704_IMAGE_SENSOR_PV_WIDTH;//+HI704_sensor.pv_dummy_pixels;
			*pFeatureReturnPara16=HI704_IMAGE_SENSOR_PV_HEIGHT;//+HI704_sensor.pv_dummy_lines;
			*pFeatureParaLen=4;
		     break;
		case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
			//*pFeatureReturnPara32 = HI704_sensor_pclk/10;
			*pFeatureParaLen=4;
		     break;
		case SENSOR_FEATURE_SET_ESHUTTER:
	
		     break;
		case SENSOR_FEATURE_SET_NIGHTMODE:
			 HI704_night_mode((BOOL) *pFeatureData16);
		     break;
		case SENSOR_FEATURE_SET_GAIN:
			 break; 
		case SENSOR_FEATURE_SET_FLASHLIGHT:
		     break;
		case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
		     break;
		case SENSOR_FEATURE_SET_REGISTER:
			 HI704_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
		     break;
		case SENSOR_FEATURE_GET_REGISTER:
			 pSensorRegData->RegData = HI704_read_cmos_sensor(pSensorRegData->RegAddr);
		     break;
		case SENSOR_FEATURE_GET_CONFIG_PARA:
			 memcpy(pSensorConfigData, &HI704SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
			 *pFeatureParaLen=sizeof(MSDK_SENSOR_CONFIG_STRUCT);
		     break;
		case SENSOR_FEATURE_SET_CCT_REGISTER:
		case SENSOR_FEATURE_GET_CCT_REGISTER:
		case SENSOR_FEATURE_SET_ENG_REGISTER:
		case SENSOR_FEATURE_GET_ENG_REGISTER:
		case SENSOR_FEATURE_GET_REGISTER_DEFAULT:
		case SENSOR_FEATURE_CAMERA_PARA_TO_SENSOR:
		case SENSOR_FEATURE_SENSOR_TO_CAMERA_PARA:
		case SENSOR_FEATURE_GET_GROUP_INFO:
		case SENSOR_FEATURE_GET_ITEM_INFO:
		case SENSOR_FEATURE_SET_ITEM_INFO:
		case SENSOR_FEATURE_GET_ENG_INFO:
		     break;
		case SENSOR_FEATURE_GET_GROUP_COUNT:
	               // *pFeatureReturnPara32++=0;
			//*pFeatureParaLen=4;
		     break; 
		
		case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
			// get the lens driver ID from EEPROM or just return LENS_DRIVER_ID_DO_NOT_CARE
			// if EEPROM does not exist in camera module.
			*pFeatureReturnPara32=LENS_DRIVER_ID_DO_NOT_CARE;
			*pFeatureParaLen=4;
		     break;
		case SENSOR_FEATURE_SET_YUV_CMD:
			 HI704YUVSensorSetting((FEATURE_ID)*pFeatureData32, *(pFeatureData32+1));
		     break;	
		case SENSOR_FEATURE_SET_VIDEO_MODE:
		     HI704YUVSetVideoMode(*pFeatureData16);
		     break; 
		case SENSOR_FEATURE_CHECK_SENSOR_ID:
			HI704_GetSensorID(pFeatureData32); 
			break; 
		default:
			 break;			
	}
	return ERROR_NONE;
}	/* HI704FeatureControl() */


SENSOR_FUNCTION_STRUCT	SensorFuncHI704=
{
    HI704Open,
    HI704GetInfo,
    HI704GetResolution,
    HI704FeatureControl,
    HI704Control,
    HI704Close
};

UINT32 HI704_YUV_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
    /* To Do : Check Sensor status here */
    if (pfFunc!=NULL)
        *pfFunc=&SensorFuncHI704;

    return ERROR_NONE;
}	/* SensorInit() */


