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

/*******************************************************************************
 *
 * Filename:
 * ---------
 *   auddrv_def.h
 *
 * Project:
 * --------
 *   Android Audio Driver
 *
 * Description:
 * ------------
 *   Kernel Audio Stream Operation
 *
 * Author:
 * -------
 *   Stan Huang (mtk01728)
 *
 *------------------------------------------------------------------------------
 * $Revision$
 * $Modtime:$
 * $Log:$
 *
 *
 *
 *******************************************************************************/
#ifndef _AUDDRV_AUDIO_STREAM_DEF_H_
#define _AUDDRV_AUDIO_STREAM_DEF_H_

/*****************************************************************************
*                     C O M P I L E R   F L A G S
******************************************************************************
*/

//#define SOUND_FAKE_READ

//#define DEBUG_AUDDRV
//#define DEBUG_AFE_REG
//#define DEBUG_ANA_REG


#ifdef DEBUG_AUDDRV
#define PRINTK_AUDDRV(format, args...) xlog_printk(ANDROID_LOG_VERBOSE, "Sound", format, ##args )
#else
#define PRINTK_AUDDRV(format, args...)
#endif

#ifdef DEBUG_AFE_REG
#define PRINTK_AFE_REG(format, args...) xlog_printk(ANDROID_LOG_VERBOSE, "Sound", format, ##args )
#else
#define PRINTK_AFE_REG(format, args...)
#endif

#ifdef DEBUG_ANA_REG
#define PRINTK_ANA_REG(format, args...) xlog_printk(ANDROID_LOG_VERBOSE, "Sound", format, ##args )
#else
#define PRINTK_ANA_REG(format, args...)
#endif


/*****************************************************************************
*                E X T E R N A L   R E F E R E N C E S
******************************************************************************
*/


/*****************************************************************************
*                          C O N S T A N T S
******************************************************************************
*/


/*****************************************************************************
*                         D A T A   T Y P E S
******************************************************************************
*/

typedef struct
{
   kal_uint32 pucPhysBufAddr;
   kal_uint8 *pucVirtBufAddr;
   kal_int32 u4BufferSize;
   kal_int32 u4DataRemained;
   kal_uint32 u4SampleNumMask;    // sample number mask
   kal_uint32 u4SamplesPerInt;    // number of samples to play before interrupting
   kal_int32 u4WriteIdx;          // Previous Write Index.
   kal_int32 u4DMAReadIdx;        // Previous DMA Read Index.
   kal_uint32 u4fsyncflag;
   kal_uint32 uResetFlag;
   struct file *flip;
} AFE_BLOCK_T;


typedef struct
{
   AFE_BLOCK_T    rBlock;
   kal_uint32     u4BufferSize;
   kal_bool       bRunning;
   kal_uint32     open_cnt;
} AFE_DL_CONTROL_T;

typedef struct
{
   AFE_BLOCK_T    rBlock;
   kal_uint32     u4BufferSize;
   kal_bool       bRunning;
   kal_uint32     open_cnt;
} AFE_UL_CONTROL_T;

typedef struct
{
   AFE_BLOCK_T    rBlock;
   kal_uint32     u4BufferSize;
   kal_bool       bRunning;
   kal_uint32     open_cnt;
} I2S_INPUT_CONTROL_T;

typedef struct
{
   AFE_BLOCK_T    rBlock;
   kal_uint32     u4BufferSize;
   kal_bool       bRunning;
   kal_uint32     open_cnt;
} I2S_OUTPUT_CONTROL_T;

typedef struct
{
   AFE_BLOCK_T    rBlock;
   kal_uint32     u4BufferSize;
   kal_bool       bRunning;
   kal_uint32     open_cnt;
} DAI_INPUT_CONTROL_T;

typedef struct
{
   kal_uint32     status;
   kal_uint32     open_cnt;
} DAI_OUTPUT_CONTROL_T;

typedef struct
{
   AFE_BLOCK_T    rBlock;
   kal_uint32     u4BufferSize;
   kal_bool       bRunning;
   kal_uint32     open_cnt;
} AWB_INPUT_CONTROL_T;


/*****************************************************************************
*                        F U N C T I O N   D E F I N I T I O N
******************************************************************************
*/

#endif
