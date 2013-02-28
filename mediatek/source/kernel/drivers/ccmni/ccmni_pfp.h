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
 *   ccmni_pfp.h
 *
 * Project:
 * --------
 *   YuSu
 *
 * Description:
 * ------------
 *   MT6516 Cross Chip Modem Network Interface - Packet Framing Protocol
 *
 * Author:
 * -------
 *   Stanley Chou (mtk01411)
 *
 ****************************************************************************/

/* Compile Option to decide if DYNAMIC_MULTIPLE_FRAME encode/decode is supported or not */

#define MAX_PDP_CONT_NUM          3
#define PFP_FRAME_START_FLAG      0xF9
#define PFP_FRAME_MAGIC_NUM       0x66
/* MAX_PFP_LEN_FIELD_VALUE is the maximum size of one IP Packet */
#define MAX_PFP_LEN_FIELD_VALUE   1500
#define FREE_RAW_DATA_BUF_SIZE    2048
#define FREE_COOKED_DATA_BUF_SIZE 2048
#define SUPPORT_FRAME_NUM 1
#define SUPPORT_PKT_NUM 100

enum frame_flag_t
{
    FRAME_START = 0,
    FRAME_CONTINUOUS,    
    FRAME_END    
};

enum unframe_state_t
{
    PARSE_PFP_FRAME_START_FLAG_STATE = 0,
    PARSE_PFP_FRAME_MAGIC_NUM_STATE,
    PARSE_PFP_FRAME_LENGTH_FIELD_STATE,
    PARSE_PFP_FRAME_GET_DATA_STATE
};

/* Following implementations are designed for PFP(Packet Frame Protocol) */ 
typedef struct
{
    int            frame_size;
    unsigned char *frame_data;
} frame_format_t;

typedef struct 
{
    int num_frames;
    int pending_data_flag;
    int consumed_length;
#ifdef __SUPPORT_DYNAMIC_MULTIPLE_FRAME__
    frame_format_t *frame_list;
#else
    frame_format_t frame_list[SUPPORT_FRAME_NUM];
#endif
} frame_info_t;

typedef struct _complete_ippkt_t
{
    int            pkt_size;   
    unsigned char *pkt_data;
#ifndef __SUPPORT_DYNAMIC_MULTIPLE_FRAME__
    int            entry_used;
#endif
    struct _complete_ippkt_t *next;
} complete_ippkt_t;

typedef struct _packet_info_t
{
    int                  num_complete_packets;
    int                  consumed_length;
    int                  try_decode_again;
    enum unframe_state_t parse_data_state;
    complete_ippkt_t    *pkt_list;
} packet_info_t;

typedef struct _ccmni_record_t
{
    /* Next expected state to be parsed while entering the pfp_unframe() again */ 
    enum unframe_state_t unframe_state;
    /* Record the latest parsed Packet length for getting the data */ 
    int pkt_size;
    /* For fast to find the last node of pkt_list to insert a new parsed IP Pkt into this pkt_list */ 
    complete_ippkt_t *last_pkt_node;
} ccmni_record_t;

extern ccmni_record_t ccmni_dev[];

/* The following buffers are used for testing purpose */
/* Store one IP Packet data */
extern unsigned char frame_cooked_data  [];
/* Pack the IP Packet into a Frame sent to Modem */
extern unsigned char frame_raw_data     [];
extern unsigned char unframe_raw_data   [];
extern unsigned char unframe_cooked_data[];


void          pfp_reset  (int ccmni_inx);
frame_info_t  pfp_frame  (unsigned char* raw_data, unsigned char* cooked_data,
                          int cooked_size, int frame_flag, int ccmni_inx);
packet_info_t pfp_unframe(unsigned char* cooked_data, int cooked_data_buf_size,
                          unsigned char* raw_data, int raw_size, int ccmni_inx);
void          traverse_pkt_list(complete_ippkt_t *node);

#ifndef __SUPPORT_DYNAMIC_MULTIPLE_FRAME__
complete_ippkt_t* get_one_available_complete_ippkt_entry(void);
void          release_one_used_complete_ippkt_entry(complete_ippkt_t* entry);   
#endif

