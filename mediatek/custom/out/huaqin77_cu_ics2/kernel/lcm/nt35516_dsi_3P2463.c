/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2010. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
 */

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

#include <linux/string.h>

#include "lcm_drv.h"
#ifndef BUILD_UBOOT
#include <linux/delay.h>
#include <linux/kernel.h>       ///for printk
#include <linux/proc_fs.h>
#include "cust_leds.h"
#endif

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  										(540)
#define FRAME_HEIGHT 										(960)

#define REGFLAG_DELAY             							0XFE
#define REGFLAG_END_OF_TABLE      							0xFF   // END OF REGISTERS MARKER

#ifndef TRUE
    #define TRUE 1
#endif

#ifndef FALSE
    #define FALSE 0
#endif

/* lcm_id, mika add  */
#define LCM_ID (0x00)

#ifdef BUILD_UBOOT
#undef printk
#define printk printf
#endif


static unsigned int lcm_esd_test = TRUE;      ///only for ESD test
#ifndef BUILD_UBOOT
#ifdef HQ_LCD_ESD_CHECK
 bool havesuspended = FALSE;
extern int mt65xx_led_set_cust_public(struct cust_mt65xx_led *cust, int level);
extern unsigned int bl_brightness;
#endif
#endif
static unsigned int lcm_read_reg_value();
// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)    								(lcm_util.set_reset_pin((v)))

#define UDELAY(n) 											(lcm_util.udelay(n))
//#define MDELAY(n) 											(lcm_util.mdelay(n))
#define MDELAY(n) 											mdelay(n)


// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq_HQ(cmd, count, ppara, force_update)	lcm_util.dsi_set_cmdq_HQ(cmd, count, ppara, force_update)  //kaka_12_0121
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)					lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)										lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)   			lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)    
#define read_reg_v3(cmd, buffer, buffer_size)               lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)  //kaka_12_0131 //tangzp_120516  ??

static struct LCM_setting_table {
    unsigned cmd;
    unsigned char count;
    unsigned char para_list[64];
};


/* proc file system */
#ifndef BUILD_UBOOT
#define NT35516_CONFIG_PROC_FILE "nt35516_config"
static struct proc_dir_entry *nt35516_config_proc = NULL;
#endif






#define LCM_INIT_DEBUG
#define LCM_USE_VIRTUAL_INVERTION
static struct LCM_setting_table lcm_initialization_setting[] = {
	
	/*
	Note :

	Data ID will depends on the following rule.
	
		count of parameters > 1	=> Data ID = 0x39
		count of parameters = 1	=> Data ID = 0x15
		count of parameters = 0	=> Data ID = 0x05

	Structure Format :

	{DCS command, count of parameters, {parameter list}}
	{REGFLAG_DELAY, milliseconds of time, {}},

	...

	Setting ending by predefined flag
	
	{REGFLAG_END_OF_TABLE, 0x00, {}}
	*/

#ifdef LCM_INIT_DEBUG       

		/* level 3 */
		#if 0
		{0xFF, 4, {0xAA,0x55,0x25,0x01}},
        {REGFLAG_DELAY, 2, {}},
        
        {0xF2, 35, {0x00,0x00,0x4A,0x0A,0xA8,
					0x00,0x00,0x00,0x00,0x00,
					0x00,0x00,0x00,0x00,0x00,
					0x00,0x00,0x0B,0x00,0x00,
					0x00,0x00,0x00,0x00,0x00,
					0x00,0x00,0x00,0x40,0x01,
					0x51,0x00,0x01,0x00,0x01}},
        {REGFLAG_DELAY, 10, {}},
        
        {0xF3, 7, {0x02,0x03,0x07,0x45,0x88,0xD1,0x0D}},
        {REGFLAG_DELAY, 2, {}},
		#endif
        /***************************************/
        /* CMD2, Page 0 */
        /***************************************/
        {0xF0, 5, {0x55,0xAA,0x52,0x08,0x00}},
        {REGFLAG_DELAY, 2, {}},

        {0xB1, 3, {0xCC,0x00,0x00}},		/* dispplay option contorl, mode: 4c, mode: cc */
        {REGFLAG_DELAY, 2, {}},

        {0xB8, 4, {0x01,0x02,0x02,0x02}},	/*EQ control function for source driver  */
        {REGFLAG_DELAY, 2, {}},

		#ifdef LCM_USE_VIRTUAL_INVERTION
		#else
		
		{0xB7, 2, {0x33,0x33}},		/* invertion control */
        {REGFLAG_DELAY, 2, {}},

        {0xBC, 3, {0x04,0x04,0x04}},		/* invertion control */
        {REGFLAG_DELAY, 2, {}},
        #endif

		{0xC9, 6, {0x63,0x06,0x0D,0x1A,0x17,0x00}},
        {REGFLAG_DELAY, 2, {}},

        /***************************************/        
        /* CMD2, Page 1 */
        /***************************************/
        {0xF0, 5, {0x55,0xAA,0x52,0x08,0x01}},
        {REGFLAG_DELAY, 2, {}},

        {0xB0, 3, {0x05,0x05,0x05}},	/* Page.1, set AVDD voltage*/
        {REGFLAG_DELAY, 2, {}},
        
        {0xB1, 3, {0x05,0x05,0x05}},	/* page.1, set AVEE voltage */
        {REGFLAG_DELAY, 2, {}},

        {0xB2, 3, {0x01,0x01,0x01}},	/* page,1, set VCL voltage */
        {REGFLAG_DELAY, 2, {}},

        {0xB3, 3, {0x0E,0x0E,0x0E}},	/* Page.1, set VGH voltage */
        {REGFLAG_DELAY, 2, {}},

		#ifdef LCM_USE_VIRTUAL_INVERTION
        {0xB4, 3, {0x0A,0x0A,0x0A}},	/* Page.1, set VGLX voltage */
		#else
        {0xB4, 3, {0x08,0x08,0x08}},
		#endif
        {REGFLAG_DELAY, 2, {}},

        {0xB6, 3, {0x44,0x44,0x44}},	/* Page.1, set AVDD boosting times/frequency */
        {REGFLAG_DELAY, 2, {}},

        {0xB7, 3, {0x34,0x34,0x34}},	/* Page.1, set AVEE boosting times/frequency */
        {REGFLAG_DELAY, 2, {}},

		#ifdef LCM_USE_VIRTUAL_INVERTION
        {0xB8, 3, {0x20,0x20,0x20}},	/* Page.1, set VCL boosting times/frequency */
		#else
        {0xB8, 3, {0x10,0x10,0x10}},
        #endif
		{REGFLAG_DELAY, 2, {}},

        //{0xB9, 3, {0x26,0x26,0x26}},	/* Page.1, set VGH boosting times/frequency */
        {0xB9, 3, {0x36,0x36,0x36}},	/* Page.1, set VGH boosting times/frequency */
        {REGFLAG_DELAY, 2, {}},

        {0xBA, 3, {0x24,0x24,0x24}},	/* Page.1, set VGLX boosting times/frequency */
        {REGFLAG_DELAY, 2, {}},

        {0xBC, 3, {0x00,0xC8,0x00}},	/* Page.1, set VGMP/VGSP voltages */
        {REGFLAG_DELAY, 2, {}},

        {0xBD, 3, {0x00,0xC8,0x00}},	/* Page.1, set VGMN/VGSN voltages */
        {REGFLAG_DELAY, 2, {}},

        {0xBE, 1, {0x78}},	// 76		/* Page.1, set DC VCOM offset */
        {REGFLAG_DELAY, 2, {}},
                        
        {0xC0, 2, {0x04,0x00}},			/* gpio config */
        {REGFLAG_DELAY, 2, {}},
                
        {0xCA, 1, {0x00}},				/* Page.1, gate signal voltage control*/
        {REGFLAG_DELAY, 2, {}},

        {0xD0,        4,        {0x0A,0x10,0x0D,0x0F}},	/* Page.1, set gradient for gamma divider */ 
        {REGFLAG_DELAY, 2, {}},
		
		/* Page.1, set gamma 2.2 correction characteristic for positive ”Red” (node 1~node 8) */
		{0xD1,        16,        {0x00,0x70,0x00,0xCE,0x00,0xF7,0x01,0x10,0x01,0x21,0x01,0x44,0x01,0x62,0x01,0x8D}},
        {REGFLAG_DELAY, 2, {}},

		/* Page.1, set gamma 2.2 correction characteristic for positive ”Red” (node 9~node 16) */
		{0xD2,        16,        {0x01,0xAF,0x01,0xE4,0x02,0x0C,0x02,0x4D,0x02,0x82,0x02,0x84,0x02,0xB8,0x02,0xF0}},
		{REGFLAG_DELAY, 2, {}},

		/* Page.1, set gamma 2.2 correction characteristic for positive ”Red” (node 17~node 24) */
		{0xD3,        16,        {0x03,0x14,0x03,0x42,0x03,0x5E,0x03,0x80,0x03,0x97,0x03,0xB0,0x03,0xC0,0x03,0xDF}},
        {REGFLAG_DELAY, 2, {}},
                                                                                                        
		/* Page.1, set gamma 2.2 correction characteristic for positive ”Red” (node 25~node 26) */
		{0xD4,        4,        {0x03,0xFD,0x03,0xFF}},
        {REGFLAG_DELAY, 2, {}},

		/* Page.1, set gamma 2.2 correction characteristic for positive ”Green” (node 1~node 8) */
		{0xD5,        16,        {0x00,0x70,0x00,0xCE,0x00,0xF7,0x01,0x10,0x01,0x21,0x01,0x44,0x01,0x62,0x01,0x8D}},
        {REGFLAG_DELAY, 2, {}},

		/* Page.1, set gamma 2.2 correction characteristic for positive ”Green” (node 9~node 16) */
		{0xD6,        16,        {0x01,0xAF,0x01,0xE4,0x02,0x0C,0x02,0x4D,0x02,0x82,0x02,0x84,0x02,0xB8,0x02,0xF0}},
        {REGFLAG_DELAY, 2, {}},

		/* Page.1, set gamma 2.2 correction characteristic for positive ”Green” (node 17~node 24) */
		{0xD7,        16,        {0x03,0x14,0x03,0x42,0x03,0x5E,0x03,0x80,0x03,0x97,0x03,0xB0,0x03,0xC0,0x03,0xDF}},
        {REGFLAG_DELAY, 2, {}},

		/* Page.1, set gamma 2.2 correction characteristic for positive ”Green” (node 25~node 26) */
		{0xD8,        4,        {0x03,0xFD,0x03,0xFF}},
        {REGFLAG_DELAY, 2, {}},                                                                                

		/* Page.1, set gamma 2.2 correction characteristic for positive ”Blue” (node 1~node 8) */
		{0xD9,        16,        {0x00,0x70,0x00,0xCE,0x00,0xF7,0x01,0x10,0x01,0x21,0x01,0x44,0x01,0x62,0x01,0x8D}},
        {REGFLAG_DELAY, 2, {}},

		/* Page.1, set gamma 2.2 correction characteristic for positive ”Blue” (node 9~node 16) */
		{0xDD,        16,        {0x01,0xAF,0x01,0xE4,0x02,0x0C,0x02,0x4D,0x02,0x82,0x02,0x84,0x02,0xB8,0x02,0xF0}},
        {REGFLAG_DELAY, 2, {}},

		/* Page.1, set gamma 2.2 correction characteristic for positive ”Blue” (node 17~node 24) */
		{0xDE,        16,        {0x03,0x14,0x03,0x42,0x03,0x5E,0x03,0x80,0x03,0x97,0x03,0xB0,0x03,0xC0,0x03,0xDF}},
        {REGFLAG_DELAY, 2, {}},
                                                                                                
		/* Page.1, set gamma 2.2 correction characteristic for positive ”Blue” (node 25~node 26) */
		{0xDF,        4,        {0x03,0xFD,0x03,0xFF}},
        {REGFLAG_DELAY, 2, {}},

		/* Page.1, set gamma 2.2 correction characteristic for negative ”Red” (node 1~node 8) */
		{0xE0,        16,        {0x00,0x70,0x00,0xCE,0x00,0xF7,0x01,0x10,0x01,0x21,0x01,0x44,0x01,0x62,0x01,0x8D}},
        {REGFLAG_DELAY, 2, {}},

		/* Page.1, set gamma 2.2 correction characteristic for negative ”Red” (node 9~node 16) */
		{0xE1,        16,        {0x01,0xAF,0x01,0xE4,0x02,0x0C,0x02,0x4D,0x02,0x82,0x02,0x84,0x02,0xB8,0x02,0xF0}},
        {REGFLAG_DELAY, 2, {}},

		/* Page.1, set gamma 2.2 correction characteristic for negative ”Red” (node 17~node 24) */
		{0xE2,        16,        {0x03,0x14,0x03,0x42,0x03,0x5E,0x03,0x80,0x03,0x97,0x03,0xB0,0x03,0xC0,0x03,0xDF}},
        {REGFLAG_DELAY, 2, {}},
                                                                                                
		/* Page.1, set gamma 2.2 correction characteristic for negative ”Red” (node 25~node 26) */
		{0xE3,        4,        {0x03,0xFD,0x03,0xFF}},
        {REGFLAG_DELAY, 2, {}},
        
		/* Page.1, set gamma 2.2 correction characteristic for negative ”Green” (node 1~node 8) */
		{0xE4,        16,        {0x00,0x70,0x00,0xCE,0x00,0xF7,0x01,0x10,0x01,0x21,0x01,0x44,0x01,0x62,0x01,0x8D}},
        {REGFLAG_DELAY, 2, {}},

		/* Page.1, set gamma 2.2 correction characteristic for negative ”Green” (node 9~node 16) */
		{0xE5,        16,        {0x01,0xAF,0x01,0xE4,0x02,0x0C,0x02,0x4D,0x02,0x82,0x02,0x84,0x02,0xB8,0x02,0xF0}},
        {REGFLAG_DELAY, 2, {}},

		/* Page.1, set gamma 2.2 correction characteristic for negative ”Green” (node 17~node 24) */
		{0xE6,        16,        {0x03,0x14,0x03,0x42,0x03,0x5E,0x03,0x80,0x03,0x97,0x03,0xB0,0x03,0xC0,0x03,0xDF}},
        {REGFLAG_DELAY, 2, {}},
                        
		/* Page.1, set gamma 2.2 correction characteristic for negative ”Green” (node 25~node 26) */
		//{0xE7,        4,        {0x03,0xFF,0x03,0xFF}},
		{0xE7,        4,        {0x03,0xFD,0x03,0xFF}},
        {REGFLAG_DELAY, 2, {}},

		/* Page.1, set gamma 2.2 correction characteristic for negative ”Blue” (node 1~node 8) */
		{0xE8,        16,        {0x00,0x70,0x00,0xCE,0x00,0xF7,0x01,0x10,0x01,0x21,0x01,0x44,0x01,0x62,0x01,0x8D}},
        {REGFLAG_DELAY, 2, {}},

		/* Page.1, set gamma 2.2 correction characteristic for negative ”Blue” (node 9~node 16) */
		{0xE9,        16,        {0x01,0xAF,0x01,0xE4,0x02,0x0C,0x02,0x4D,0x02,0x82,0x02,0x84,0x02,0xB8,0x02,0xF0}},
        {REGFLAG_DELAY, 2, {}},

		/* Page.1, set gamma 2.2 correction characteristic for negative ”Blue” (node 17~node 24) */
		{0xEA,        16,        {0x03,0x14,0x03,0x42,0x03,0x5E,0x03,0x80,0x03,0x97,0x03,0xB0,0x03,0xC0,0x03,0xDF}},
        {REGFLAG_DELAY, 2, {}},

		/* Page.1, set gamma 2.2 correction characteristic for negative ”Blue” (node 25~node 26) */
		{0xEB,        4,        {0x03,0xFD,0x03,0xFF}},
        {REGFLAG_DELAY, 2, {}},

		/* Tearing effect mode set & on  */
        {0x35,        1,        {0x00}},
        {REGFLAG_DELAY, 2, {}},

		/* memory data access control */
        //{0x36,        1,        {0x00}},
        {0x36,        1,        {0xd4}},
        {REGFLAG_DELAY, 2, {}},

		/* sleep out / boost on */
        {0x11,        1,        {0x00}},
        {REGFLAG_DELAY, 250, {}},
		
	
		/* display on */
        {0x29,        1,        {0x00}},
        {REGFLAG_DELAY, 5, {}},

        {REGFLAG_END_OF_TABLE, 0x00, {}}


	#else
	#endif
};

static struct LCM_setting_table lcm_set_window[] = {
	{0x2A,	4,	{0x00, 0x00, (FRAME_WIDTH>>8), (FRAME_WIDTH&0xFF)}},
	{0x2B,	4,	{0x00, 0x00, (FRAME_HEIGHT>>8), (FRAME_HEIGHT&0xFF)}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};


static struct LCM_setting_table lcm_sleep_out_setting[] = {

	/* Sleep Out */
	{0x11, 1, {0x00}},
        {REGFLAG_DELAY,250, {}},//450     //sleep out time long 

    /* Display ON */
	{0x29, 1, {0x00}},
	{REGFLAG_DELAY, 100, {}},
	
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct LCM_setting_table lcm_sleep_in_setting[] = {
	/* Display off sequence */
	{0x28, 1, {0x00}},
	{REGFLAG_DELAY, 100, {}},

    /* Sleep Mode On */
	{0x10, 1, {0x00}},
	{REGFLAG_DELAY, 140, {}},

	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct LCM_setting_table lcm_deep_sleep_mode_in_setting[] = {
	// Display off sequence
	{0x28, 1, {0x00}},
	{REGFLAG_DELAY, 350, {}},

    // Sleep Mode On
	{0x10, 1, {0x00}},
	{REGFLAG_DELAY, 100, {}},

	/* enter deep standby mode */
	{0x4F, 1, {0x01}},
	{REGFLAG_DELAY, 150, {}},

	{REGFLAG_END_OF_TABLE, 0x00, {}}
};


static struct LCM_setting_table lcm_backlight_level_setting[] = {
	{0x51, 1, {0xFF}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct LCM_setting_table lcm_page_1_setting[] = {
	{0xF0, 5, {0x55,0xAA,0x52,0x08,0x01}},
	{REGFLAG_DELAY, 2, {}},
};

static struct LCM_setting_table lcm_read_f2_setting[] = {
		/* level 3 */
		{0xFF, 4, {0xAA,0x55,0x25,0x01}},
        {REGFLAG_DELAY, 2, {}},
};


static void lcm_init_setting()
{
	unsigned int data_array[16];
	
	data_array[0]= 0x00053902;
	data_array[1]= 0x2555AAFF;
	data_array[2]= 0x00000001;
	dsi_set_cmdq(&data_array, 3, 1);
	mdelay(2);

	data_array[0]= 0x00243902;
	data_array[1]= 0x4A0000F2;
	data_array[2]= 0x0000A80A;
	data_array[3]= 0x00000000;
	data_array[4]= 0x00000000;
	data_array[5]= 0x000B0000;
	data_array[6]= 0x00000000;
	data_array[7]= 0x00000000;
	data_array[8]= 0x51014000;
	data_array[9]= 0x01000100;
	dsi_set_cmdq(&data_array, 10, 1);
	mdelay(2);

	data_array[0]= 0x00083902;
	data_array[1]= 0x070302F3;
	data_array[2]= 0x0DD18845;
	dsi_set_cmdq(&data_array, 3, 1);
	mdelay(2);

	/* cmd2, page0 */
	data_array[0]=0x00063902;
	data_array[1]=0x52AA55F0;
	data_array[2]=0x00000008;
	dsi_set_cmdq(&data_array, 3, 1);
	mdelay(2);

	/* dispplay option contorl */
	data_array[0]=0x00043902;
	data_array[1]=0x0000CCB1;
	dsi_set_cmdq(&data_array, 2, 1);
	mdelay(2);

	data_array[0]=0x00053902;
	data_array[1]=0x020201B8;
	data_array[2]=0x00000002;
	dsi_set_cmdq(&data_array, 3, 1);
	mdelay(2);

	#ifdef LCM_USE_VIRTUAL_INVERTION
	#else
	data_array[0]=0x00033902;
//	data_array[1]=0x007171B7;
	data_array[1]=0x003333B7;
	dsi_set_cmdq(&data_array, 2, 1);
	mdelay(2);

	data_array[0]=0x00043902;
	data_array[1]=0x040404BC;
	dsi_set_cmdq(&data_array, 2, 1);
	mdelay(2);
	#endif
	
	data_array[0]=0x00073902;
	data_array[1]=0x0D0663C9;
	data_array[2]=0x0000171A;
	dsi_set_cmdq(&data_array, 3, 1);
	mdelay(2);
	
	/* cmd2, page1 */
	data_array[0]=0x00063902;
	data_array[1]=0x52AA55F0;
	data_array[2]=0x00000108;
	dsi_set_cmdq(&data_array, 3, 1);
	mdelay(2);
	
	data_array[0]=0x00043902;
	data_array[1]=0x050505B0;
	dsi_set_cmdq(&data_array, 2, 1);
	mdelay(2);

	data_array[0]=0x00043902;
	data_array[1]=0x050505B1;
	dsi_set_cmdq(&data_array, 2, 1);
	mdelay(2);
	
	data_array[0]=0x00043902;
	data_array[1]=0x010101B2;
	dsi_set_cmdq(&data_array, 2, 1);
	mdelay(2);
	
	data_array[0]=0x00043902;
	data_array[1]=0x0E0E0EB3;
	dsi_set_cmdq(&data_array, 2, 1);
	mdelay(2);
	
	#ifdef LCM_USE_VIRTUAL_INVERTION
	data_array[0]=0x00043902;
	data_array[1]=0x0A0A0AB4;
	dsi_set_cmdq(&data_array, 2, 1);
	mdelay(2);
	#else
	data_array[0]=0x00043902;
	data_array[1]=0x080808B4;
	dsi_set_cmdq(&data_array, 2, 1);
	mdelay(2);
	#endif
	
	data_array[0]=0x00043902;
	data_array[1]=0x444444B6;
	dsi_set_cmdq(&data_array, 2, 1);
	mdelay(2);

	data_array[0]=0x00043902;
	data_array[1]=0x343434B7;
	dsi_set_cmdq(&data_array, 2, 1);
	mdelay(2);
	#ifdef LCM_USE_VIRTUAL_INVERTION
	data_array[0]=0x00043902;
	data_array[1]=0x202020B8;
	dsi_set_cmdq(&data_array, 2, 1);
	mdelay(2);
	#else
	data_array[0]=0x00043902;
	data_array[1]=0x101010B8;
	dsi_set_cmdq(&data_array, 2, 1);
	mdelay(2);
	#endif

	data_array[0]=0x00043902;
	data_array[1]=0x262626B9;
	dsi_set_cmdq(&data_array, 2, 1);
	mdelay(2);

	data_array[0]=0x00043902;
	data_array[1]=0x242424BA;
	dsi_set_cmdq(&data_array, 2, 1);
	mdelay(2);

	data_array[0]=0x00043902;
	data_array[1]=0x00C800BC;
	dsi_set_cmdq(&data_array, 2, 1);
	mdelay(2);

	data_array[0]=0x00043902;
	data_array[1]=0x00C800BD;
	dsi_set_cmdq(&data_array, 2, 1);
	mdelay(2);

	data_array[0]=0x00023902;
	data_array[1]=0x00007CBE;    //7e
	dsi_set_cmdq(&data_array, 2, 1);
	mdelay(2);
	
	data_array[0]=0x00033902;
	data_array[1]=0x000004C0;
	dsi_set_cmdq(&data_array, 2, 1);
	mdelay(2);

	data_array[0]=0x00023902;
	data_array[1]=0x000000CA;
	dsi_set_cmdq(&data_array, 2, 1);
	mdelay(2);
	
	data_array[0]=0x00053902;
	data_array[1]=0x0D100AD0;
	data_array[2]=0x0000000F;
	dsi_set_cmdq(&data_array, 3, 1);
	mdelay(2);
  
	/* D1~D4 */
	data_array[0]=0x00113902;
	data_array[1]=0x007000D1;
	data_array[2]=0x01FD00B2;
	data_array[3]=0x0138011F;
	data_array[4]=0x01720157;
	data_array[5]=0x0000009C;
	dsi_set_cmdq(&data_array, 6, 1);
	mdelay(2);
	
	data_array[0]=0x00113902;
	data_array[1]=0x01BA01D2;
	data_array[2]=0x021502ED;
	data_array[3]=0x02870254;
	data_array[4]=0x02BA0289;
	data_array[5]=0x000000EE;
	dsi_set_cmdq(&data_array, 6, 1);
	mdelay(2);
	
	data_array[0]=0x00113902;
	data_array[1]=0x031303D3;
	data_array[2]=0x035C033F;
	data_array[3]=0x0394037C;
	data_array[4]=0x03C703AD;
	data_array[5]=0x000000EA;
	dsi_set_cmdq(&data_array, 6, 1);
	mdelay(2);
	
	data_array[0]=0x00053902;
	data_array[1]=0x03FD03D4;
	data_array[2]=0x000000FF;
	dsi_set_cmdq(&data_array, 3, 1);
	mdelay(2);

	/* D5~D8 */
	data_array[0]=0x00113902;
	data_array[1]=0x007000D5;
	data_array[2]=0x01FD00B2;
	data_array[3]=0x0138011F;
	data_array[4]=0x01720157;
	data_array[5]=0x0000009C;
	dsi_set_cmdq(&data_array, 6, 1);
	mdelay(2);
	
	data_array[0]=0x00113902;
	data_array[1]=0x01BA01D6;
	data_array[2]=0x021502ED;
	data_array[3]=0x02870254;
	data_array[4]=0x02BA0289;
	data_array[5]=0x000000EE;
	dsi_set_cmdq(&data_array, 6, 1);
	mdelay(2);
	
	data_array[0]=0x00113902;
	data_array[1]=0x031303D7;
	data_array[2]=0x035C033F;
	data_array[3]=0x0394037C;
	data_array[4]=0x03C703AD;
	data_array[5]=0x000000EA;
	dsi_set_cmdq(&data_array, 6, 1);
	mdelay(2);
	
	data_array[0]=0x00053902;
	data_array[1]=0x03FD03D8;
	data_array[2]=0x000000FF;
	dsi_set_cmdq(&data_array, 3, 1);
	mdelay(2);

	/* D9 DD DE DF */
	data_array[0]=0x00113902;
	data_array[1]=0x007000D9;
	data_array[2]=0x01FD00B2;
	data_array[3]=0x0138011F;
	data_array[4]=0x01720157;
	data_array[5]=0x0000009C;
	dsi_set_cmdq(&data_array, 6, 1);
	mdelay(2);
	
	data_array[0]=0x00113902;
	data_array[1]=0x01BA01DD;
	data_array[2]=0x021502ED;
	data_array[3]=0x02870254;
	data_array[4]=0x02BA0289;
	data_array[5]=0x000000EE;
	dsi_set_cmdq(&data_array, 6, 1);
	mdelay(2);
	
	data_array[0]=0x00113902;
	data_array[1]=0x031303DE;
	data_array[2]=0x035C033F;
	data_array[3]=0x0394037C;
	data_array[4]=0x03C703AD;
	data_array[5]=0x000000EA;
	dsi_set_cmdq(&data_array, 6, 1);
	mdelay(2);
	
	data_array[0]=0x00053902;
	data_array[1]=0x03FD03DF;
	data_array[2]=0x000000FF;
	dsi_set_cmdq(&data_array, 3, 1);
	mdelay(2);

	/* E0~E3 */
	data_array[0]=0x00113902;
	data_array[1]=0x007000E0;
	data_array[2]=0x01FD00B2;
	data_array[3]=0x0138011F;
	data_array[4]=0x01720157;
	data_array[5]=0x0000009C;
	dsi_set_cmdq(&data_array, 6, 1);
	mdelay(2);
	
	data_array[0]=0x00113902;
	data_array[1]=0x01BA01E1;
	data_array[2]=0x021502ED;
	data_array[3]=0x02870254;
	data_array[4]=0x02BA0289;
	data_array[5]=0x000000EE;
	dsi_set_cmdq(&data_array, 6, 1);
	mdelay(2);
	
	data_array[0]=0x00113902;
	data_array[1]=0x031303E2;
	data_array[2]=0x035C033F;
	data_array[3]=0x0394037C;
	data_array[4]=0x03C703AD;
	data_array[5]=0x000000EA;
	dsi_set_cmdq(&data_array, 6, 1);
	mdelay(2);
	
	data_array[0]=0x00053902;
	data_array[1]=0x03FD03E3;
	data_array[2]=0x000000FF;
	dsi_set_cmdq(&data_array, 3, 1);
	mdelay(2);

	/* E4~E7 */
	data_array[0]=0x00113902;
	data_array[1]=0x007000E4;
	data_array[2]=0x01FD00B2;
	data_array[3]=0x0138011F;
	data_array[4]=0x01720157;
	data_array[5]=0x0000009C;
	dsi_set_cmdq(&data_array, 6, 1);
	mdelay(2);
	
	data_array[0]=0x00113902;
	data_array[1]=0x01BA01E5;
	data_array[2]=0x021502ED;
	data_array[3]=0x02870254;
	data_array[4]=0x02BA0289;
	data_array[5]=0x000000EE;
	dsi_set_cmdq(&data_array, 6, 1);
	mdelay(2);
	
	data_array[0]=0x00113902;
	data_array[1]=0x031303E6;
	data_array[2]=0x035C033F;
	data_array[3]=0x0394037C;
	data_array[4]=0x03C703AD;
	data_array[5]=0x000000EA;
	dsi_set_cmdq(&data_array, 6, 1);
	mdelay(2);
	
	data_array[0]=0x00053902;
	data_array[1]=0x03FD03E7;
	data_array[2]=0x000000FF;
	dsi_set_cmdq(&data_array, 3, 1);
	mdelay(2);


	/* E8~EB */
	data_array[0]=0x00113902;
	data_array[1]=0x007000E8;
	data_array[2]=0x01FD00B2;
	data_array[3]=0x0138011F;
	data_array[4]=0x01720157;
	data_array[5]=0x0000009C;
	dsi_set_cmdq(&data_array, 6, 1);
	mdelay(2);
	
	data_array[0]=0x00113902;
	data_array[1]=0x01BA01E9;
	data_array[2]=0x021502ED;
	data_array[3]=0x02870254;
	data_array[4]=0x02BA0289;
	data_array[5]=0x000000EE;
	dsi_set_cmdq(&data_array, 6, 1);
	mdelay(2);
	
	data_array[0]=0x00113902;
	data_array[1]=0x031303EA;
	data_array[2]=0x035C033F;
	data_array[3]=0x0394037C;
	data_array[4]=0x03C703AD;
	data_array[5]=0x000000EA;
	dsi_set_cmdq(&data_array, 6, 1);
	mdelay(2);
	
	data_array[0]=0x00053902;
	data_array[1]=0x03FD03EB;
	data_array[2]=0x000000FF;
	dsi_set_cmdq(&data_array, 3, 1);
	mdelay(2);

	/* */
	data_array[0]=0x00023902;
	data_array[1]=0x00000035;
	dsi_set_cmdq(&data_array, 2, 1);
	mdelay(2);
	
	data_array[0]=0x00023902;
	data_array[1]=0x0000D436;
	dsi_set_cmdq(&data_array, 2, 1);
	mdelay(2);

	data_array[0]=0x00023902;
	data_array[1]=0x00000011;
	dsi_set_cmdq(&data_array, 2, 1);
	mdelay(200);
	
	#if 0
	data_array[0]=0x00023902;
	data_array[1]=0x0000773A;
	dsi_set_cmdq(&data_array, 2, 1);
	mdelay(2);
	
	data_array[0]=0x00023902;
	data_array[1]=0x00000013;
	dsi_set_cmdq(&data_array, 2, 1);
	mdelay(40);
	#endif

	data_array[0]=0x00023902;
	data_array[1]=0x00000029;
	dsi_set_cmdq(&data_array, 2, 1);
	mdelay(40);
	
}
static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
	unsigned int i;

    for(i = 0; i < count; i++) {
		
        unsigned cmd;
        cmd = table[i].cmd;
		
        switch (cmd) {
			
            case REGFLAG_DELAY :
                MDELAY(table[i].count);
                break;
				
            case REGFLAG_END_OF_TABLE :
                break;
				
            default:
				dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
       	}
    }
	
}

/*
 * proc file syestem
 */
static int nt35516_config_read_proc(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	return 0;
}


static int nt35516_config_write_proc(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	
	/* reset lcm */
	SET_RESET_PIN(1);
    SET_RESET_PIN(0);
    MDELAY(1);
    SET_RESET_PIN(1);
    MDELAY(10);
	printk("lcm reset...\n");
	
	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
	printk("lcm load initialization setting...\n");

	return 0;
}
// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
    memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}


static void lcm_get_params(LCM_PARAMS *params)
{
		memset(params, 0, sizeof(LCM_PARAMS));
	
		params->type   = LCM_TYPE_DSI;

		params->width  = FRAME_WIDTH;	/* width */
		params->height = FRAME_HEIGHT;	/* height */

		// enable tearing-free
		params->dbi.te_mode 				= LCM_DBI_TE_MODE_VSYNC_ONLY;
		params->dbi.te_edge_polarity		= LCM_POLARITY_RISING;

		params->dsi.mode   = CMD_MODE;

		// DSI
		/* Command mode setting */
		params->dsi.LANE_NUM				= LCM_TWO_LANE;
		//The following defined the fomat for data coming from LCD engine.
		params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
		params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
		params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
		params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

		// Highly depends on LCD driver capability.
		params->dsi.packet_size=256;

		// Video mode setting		
		params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;

		params->dsi.word_count=540*3;	
		params->dsi.vertical_sync_active=2;
		#if 0
		/* nomal parameters */
		params->dsi.vertical_backporch=2;
		params->dsi.vertical_frontporch=2;
		#else
		/* debug parameters */
		params->dsi.vertical_backporch=6;
		params->dsi.vertical_frontporch=6;
		#endif
		params->dsi.vertical_active_line=960;
	
		//params->dsi.line_byte=2256;		// 2256 = 752*3	need to check?
		params->dsi.line_byte=2180;		// 2256 = 752*3	need to check?
		params->dsi.horizontal_sync_active_byte=26;
		params->dsi.horizontal_backporch_byte=206;
		params->dsi.horizontal_frontporch_byte=206;	
		params->dsi.rgb_byte=(540*3+6);	
	
		params->dsi.horizontal_sync_active_word_count=20;	
		params->dsi.horizontal_backporch_word_count=200;
		params->dsi.horizontal_frontporch_word_count=200;

		// Bit rate calculation
		params->dsi.pll_div1=40;		// fref=26MHz, fvco=fref*(div1+1)	(div1=0~63, fvco=500MHZ~1GHz)
		params->dsi.pll_div2=1;			// div2=0~15: fout=fvo/(2*div2)

}


static void lcm_init(void)
{
	unsigned int data_array[16];



    SET_RESET_PIN(1);
    MDELAY(10);
    SET_RESET_PIN(0);
    MDELAY(30);
    SET_RESET_PIN(1);
    MDELAY(130);
#ifdef BUILD_UBOOT
    printf("lcm init:\n");
#endif
#if 0
	data_array[0]= 0x00053902;
	data_array[1]= 0x2555AAFF;
	data_array[2]= 0x00000001;
	dsi_set_cmdq(&data_array, 3, 1);

	data_array[0]= 0x00243902;
	data_array[1]= 0x4A0000F2;
	data_array[2]= 0x0000A80A;
	data_array[3]= 0x00000000;
	data_array[4]= 0x00000000;
	data_array[5]= 0x000B0000;
	data_array[6]= 0x00000000;
	data_array[7]= 0x00000000;
	data_array[8]= 0x51014000;
	data_array[9]= 0x01000100;
	dsi_set_cmdq(&data_array, 10, 1);

	data_array[0]= 0x00083902;
	data_array[1]= 0x070302F3;
	data_array[2]= 0x0DD18845;
	dsi_set_cmdq(&data_array, 3, 1);

	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
#else
	lcm_init_setting();
#endif
	/* create proc file system */
	#ifndef BUILD_UBOOT
	nt35516_config_proc = create_proc_entry( NT35516_CONFIG_PROC_FILE, 0666, NULL);
    
	if ( nt35516_config_proc == NULL ){
        printk("create_proc_entry %s failed\n", NT35516_CONFIG_PROC_FILE );
    } else  {
        nt35516_config_proc->read_proc = nt35516_config_read_proc;
        nt35516_config_proc->write_proc = nt35516_config_write_proc;
    }
	#endif

}

static unsigned int lcm_check_register()
{
	#ifndef BUILD_UBOOT
	unsigned char buffer[7];
    	unsigned int array[16];	//mipi command
	int i;


	/* read id test */
	push_table(lcm_read_f2_setting, sizeof(lcm_read_f2_setting) / sizeof(struct LCM_setting_table), 1);
    array[0] = 0x00073700;
    dsi_set_cmdq(array, 1, 1);
	read_reg_v3(0xF3, buffer, 7);
	
	printk("%s, nt35516 F0 register:"
					"0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
			__func__, buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6]);
	if( (buffer[0]!=0x02) || (buffer[1]!=0x03) || (buffer[2]!=0x07) )
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
	#endif

}


//#define USE_DEEP_SLEEP_MODE
static void lcm_suspend(void)
{
	#if defined(USE_DEEP_SLEEP_MODE)
	push_table(lcm_deep_sleep_mode_in_setting, sizeof(lcm_deep_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);
	#else
	push_table(lcm_sleep_in_setting, sizeof(lcm_sleep_in_setting) / sizeof(struct LCM_setting_table), 1);
	#endif
	#ifndef BUILD_UBOOT
	#ifdef HQ_LCD_ESD_CHECK
	havesuspended = TRUE;
	#endif
	#endif
}


static void lcm_resume(void)
{

	/* resume from deep sleep standby mode , registers and sdram data will loss in deep sleep standby mode
	
	 set reset pin low pluse more than 3ms 
	 and delay 4 frames or more after reset */
	#if defined(USE_DEEP_SLEEP_MODE)
	SET_RESET_PIN(1);
	MDELAY(1);
    	SET_RESET_PIN(0);
	MDELAY(5);
    	SET_RESET_PIN(1);
    	MDELAY(10);

	/* initial instuction setting and SRAM data setting */
	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);

	#else
	/* display on */
	push_table(lcm_sleep_out_setting, sizeof(lcm_sleep_out_setting) / sizeof(struct LCM_setting_table), 1);
	
	#ifndef BUILD_UBOOT
	#ifdef HQ_LCD_ESD_CHECK
	printk("lcm_resume start \n");
	if(TRUE == lcm_read_reg_value())
	{
		SET_RESET_PIN(1);
    	MDELAY(10);
    	SET_RESET_PIN(0);
    	MDELAY(30);
    	SET_RESET_PIN(1);
    	MDELAY(130);
	lcm_init_setting();
	}
	else
	{
	havesuspended = FALSE;
	printk("lcm register is ok \n");
	}
	#endif
	#endif
	#endif
}


static void lcm_update(unsigned int x, unsigned int y,
                       unsigned int width, unsigned int height)
{
	unsigned int x0 = x;
	unsigned int y0 = y;
	unsigned int x1 = x0 + width - 1;
	unsigned int y1 = y0 + height - 1;

	unsigned char x0_MSB = ((x0>>8)&0xFF);
	unsigned char x0_LSB = (x0&0xFF);
	unsigned char x1_MSB = ((x1>>8)&0xFF);
	unsigned char x1_LSB = (x1&0xFF);
	unsigned char y0_MSB = ((y0>>8)&0xFF);
	unsigned char y0_LSB = (y0&0xFF);
	unsigned char y1_MSB = ((y1>>8)&0xFF);
	unsigned char y1_LSB = (y1&0xFF);

	unsigned int data_array[16];

	data_array[0]= 0x00053902;
	data_array[1]= (x1_MSB<<24)|(x0_LSB<<16)|(x0_MSB<<8)|0x2a;
	data_array[2]= (x1_LSB);
	data_array[3]= 0x00053902;
	data_array[4]= (y1_MSB<<24)|(y0_LSB<<16)|(y0_MSB<<8)|0x2b;
	data_array[5]= (y1_LSB);
	data_array[6]= 0x002c3909;

	dsi_set_cmdq(&data_array, 7, 0);
	#ifndef BUILD_UBOOT
	#ifdef HQ_LCD_ESD_CHECK
	if(havesuspended)
	{		
		MDELAY(200);
		havesuspended = FALSE;
		struct cust_mt65xx_led *cust_led_list = get_cust_led_list();
		mt65xx_led_set_cust_public(&cust_led_list[6],bl_brightness);
	}
	#endif
	#endif
}


static void lcm_setbacklight(unsigned int level)
{
	// Refresh value of backlight level.
	lcm_backlight_level_setting[0].para_list[0] = level;

	push_table(lcm_backlight_level_setting, sizeof(lcm_backlight_level_setting) / sizeof(struct LCM_setting_table), 1);
}


static void lcm_setpwm(unsigned int divider)
{
	// TBD
}


static unsigned int lcm_getpwm(unsigned int divider)
{
	// ref freq = 15MHz, B0h setting 0x80, so 80.6% * freq is pwm_clk;
	// pwm_clk / 255 / 2(lcm_setpwm() 6th params) = pwm_duration = 23706
	unsigned int pwm_clk = 23706 / (1<<divider);	
	return pwm_clk;
}

static struct LCM_setting_table lcm_compare_page0_setting[] = {
	// Display off sequence
	{0xF0,	0x05,	{0x55, 0xAA, 0x52, 0x08, 0x00}},
	{REGFLAG_DELAY, 0x00, {}},

	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static unsigned int lcm_read_reg_value(){
		
		unsigned char buffer_0[3];
		unsigned char buffer_1[6];
        	unsigned int array[16];
		static int err_count = 0;
	
	#if 0
		push_table(lcm_compare_page0_setting, sizeof(lcm_compare_page0_setting) / sizeof(struct LCM_setting_table), 1);

		array[0] = 0x00033700;// read id return 5 bytes, so first set max return packet size
		dsi_set_cmdq(array, 1, 1);
		read_reg_v3(0xB1, buffer_0, 3);  //read device id,command is

		array[0] = 0x00013700;// read id return 5 bytes, so first set max return packet size
		dsi_set_cmdq(array, 1, 1);
		read_reg_v3(0x0A, buffer_1, 1);  //read device id,command is
		
		printk("lcm_esd test register[0xB1] value = 0x%2x, 0x%2x, 0x%2x; [0x0A]=0x%2x\n",
			buffer_0[0], buffer_0[1], buffer_0[2], buffer_1[0]);
		
		if((buffer_0[0]!=0xCC)
			||(buffer_0[1]!=0x00)
			||(buffer_0[2]!=0x00) 
			||(buffer_1[0]!=0x9C)
			||(FALSE == lcm_check_register())) {
			return TRUE;// lcm recovery
		} else {
			return FALSE;// do nothing
		}
	#else
		
		
		array[0] = 0x00063700;
		dsi_set_cmdq(array, 1, 1);
		read_reg_v3(0x0A, buffer_1, 6);  //read device id,command is

		if(buffer_1[0] == 0x9c)
		{
			if((buffer_1[3] == 0x02) && (buffer_1[4] & 0x02))
				err_count ++;
			else
				err_count = 0;
		}
		else
		{
			err_count ++;
		}
		if(err_count >= 2){
			printk("lcm_esd test register[0x0A] value = 0x%x, 0x%x, 0x%x,0x%x, 0x%x,0x%x\n",
			buffer_1[0], buffer_1[1], buffer_1[2], buffer_1[3],buffer_1[4], buffer_1[5]);
			err_count = 0;
			return TRUE;// lcm recovery
		} else {
			return FALSE;// do nothing
		}
	#endif
}

static unsigned int lcm_esd_check()
{
#ifndef BUILD_UBOOT
         printk("lcm_esd enter");

         //kaka_12_0607_add
         //return 0; //This just debug, Must removed when esd check fucnion is OK.
         //kaka_12_0607_end

        if(lcm_esd_test)
        {
		
            printk("lcm_esd test return true \n");
			/* mika */
			#if 0
			lcm_esd_test = FALSE;
            return TRUE;	// lcm recovery
			#else
			return lcm_read_reg_value();
			#endif
        }
		
		return FALSE;	// do nothind

#endif
}
static unsigned int lcm_dsi_read_test(unsigned char cmd)
{
#ifndef BUILD_UBOOT
        /// please notice: the max return packet size is 1
        /// if you want to change it, you can refer to the following marked code
        /// but read_reg currently only support read no more than 4 bytes....
        /// if you need to read more, please let BinHan knows.
/*
        unsigned int data_array[16];
        unsigned int max_return_size = 1;
        
        data_array[0]= 0x00003700 | (max_return_size << 16);    
        
        dsi_set_cmdq(&data_array, 1, 1);
*/
        return read_reg(cmd);
#endif
}

static unsigned int lcm_esd_recover()
{
    unsigned char para = 0;

#ifndef BUILD_UBOOT
    printk("lcm_esd_recover enter");
#endif
    

    SET_RESET_PIN(1);
    MDELAY(10);
    SET_RESET_PIN(0);
    MDELAY(30);
    SET_RESET_PIN(1);
    MDELAY(130);
    #if 0
	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
	MDELAY(10);
	push_table(lcm_sleep_out_setting, sizeof(lcm_sleep_out_setting) / sizeof(struct LCM_setting_table), 1);
    	MDELAY(10);
    #else
        lcm_init_setting();
    #endif
   
    dsi_set_cmdq_V2(0x35, 1, &para, 1);     ///enable TE
    MDELAY(10);

    return TRUE;
}

//#define READ_ADC_ID
extern int IMM_GetOneChannelValue(int dwChannel, int deCount);
static unsigned int lcm_compare_id()
{

	#ifdef READ_ADC_ID 
    int lcm_id0 = 0;
    int lcm_id1 = 0;
    lcm_id0 = IMM_GetOneChannelValue(1,20);
    lcm_id1 = IMM_GetOneChannelValue(2,20);

    printk("nt35516_dsi_3P2463 lcd_id0:%d lcm_id1:%d\n", 
		lcm_id0, lcm_id1);

	if( (lcm_id0>1600) && (lcm_id0<2000) && 
		(lcm_id1>1600) && (lcm_id1<2000)) {
		
		return TRUE;
	
	} else {
		return FALSE;
	}
	#else
	unsigned int id = 0;
	unsigned char buffer[3];
	unsigned int array[16];

	/* reset lcm first */
	SET_RESET_PIN(1);
    MDELAY(10);
    SET_RESET_PIN(0);
    MDELAY(30);
    SET_RESET_PIN(1);
    MDELAY(30);

	/* read id
	 * read_id return 3bytes data, 
	 * 1st module's manufacture id
	 * 2nd module/driver version id
	 * 3rd module/driver id
	 */

	array[0] = 0x00033700;// read id return two byte,version and id
    dsi_set_cmdq(array, 1, 1);

	read_reg_v2(0x04, buffer, 3);
	id = buffer[0];
	printk("%s, nt35516_dsi_3P2463 id1=0x%02x id2=0x%02x, id3=0x%02x\n",
			__func__, buffer[0], buffer[1], buffer[2]);
	
	return (LCM_ID == id) ? TRUE : FALSE ;
	#endif
}
// ---------------------------------------------------------------------------
//  Get LCM Driver Hooks
// ---------------------------------------------------------------------------
LCM_DRIVER nt35516_dsi_3P2463_lcm_drv = 
{
    .name			= "nt35516_dsi_3P2463",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
	.update         = lcm_update,
	.set_backlight	= lcm_setbacklight,
	.set_pwm        = lcm_setpwm,
	.get_pwm        = lcm_getpwm,
	.esd_check   = lcm_esd_check,
    .esd_recover   = lcm_esd_recover,
	.compare_id    = lcm_compare_id,
};
