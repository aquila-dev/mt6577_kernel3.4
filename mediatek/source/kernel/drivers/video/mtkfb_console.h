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

#ifndef __MTK_FB_CONSOLE_H__
#define __MTK_FB_CONSOLE_H__

#ifdef __cplusplus
extern "C" {
#endif


#define MFC_CHECK_RET(expr)             \
    do {                                \
        MFC_STATUS ret = (expr);        \
        ASSERT(MFC_STATUS_OK == ret);   \
    } while (0)


typedef enum
{	
   MFC_STATUS_OK                = 0,

   MFC_STATUS_INVALID_ARGUMENT  = -1,
   MFC_STATUS_NOT_IMPLEMENTED   = -2,
   MFC_STATUS_OUT_OF_MEMORY     = -3,
   MFC_STATUS_LOCK_FAIL         = -4,
   MFC_STATUS_FATAL_ERROR       = -5,
} MFC_STATUS;


typedef void* MFC_HANDLE;


/* MTK Framebuffer Console API */

MFC_STATUS MFC_Open(MFC_HANDLE *handle,
                    void *fb_addr,
                    unsigned int fb_width,
                    unsigned int fb_height,
                    unsigned int fb_bpp,
                    unsigned int fg_color,
                    unsigned int bg_color);

MFC_STATUS MFC_Close(MFC_HANDLE handle);

MFC_STATUS MFC_SetColor(MFC_HANDLE handle,
                        unsigned int fg_color, 
                        unsigned int bg_color);

MFC_STATUS MFC_ResetCursor(MFC_HANDLE handle);

MFC_STATUS MFC_Print(MFC_HANDLE handle, const char *str);

MFC_STATUS MFC_LowMemory_Printf(MFC_HANDLE handle, const char *str, UINT32 fg_color, UINT32 bg_color);

MFC_STATUS MFC_SetMem(MFC_HANDLE handle, const char *str, UINT32 color);
#ifdef __cplusplus
}
#endif

#endif // __MTK_FB_CONSOLE_H__
