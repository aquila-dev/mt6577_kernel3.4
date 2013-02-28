/** 
 * @file 
 *   hal_api.h 
 *
 * @par Project:
 *   MFlexVideo 
 *
 * @par Description:
 *   Hardware Abstraction Layer APIs
 *
 * @par Author:
 *   Jackal Chen (mtk02532)
 *
 * @par $Revision: #9 $
 * @par $Modtime:$
 * @par $Log:$
 *
 */

#ifndef _HAL_API_H_
#define _HAL_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "val_types.h"
#include "hal_types.h"
#include "mfv_drv_base.h"


typedef enum
{     
    HAL_CMD_SET_CMD_QUEUE,
    HAL_CMD_SET_POWER,
    HAL_CMD_SET_ISR,
    HAL_CMD_GET_CACHE_CTRL_ADDR,
    HAL_CMD_MAX = 0xFFFFFFFF
} HAL_CMD_T;

/*=============================================================================
 *                             Function Declaration
 *===========================================================================*/
/**
* @par Function       
*   eHalInit
* @par Description    
*   Init HAL
* @param              
*   *a_phHalHandle         [IN/OUT]   The VAL_HANDLE_T structure
* @par Returns        
*   VAL_RESULT_T
*/
VAL_RESULT_T eHalInit(
    VAL_HANDLE_T    *a_phHalHandle
    );
/**
* @par Function       
*   eHalDeInit
* @par Description    
*   DeInit HAL
* @param              
*   *a_phHalHandle         [IN/OUT]   The VAL_HANDLE_T structure
* @par Returns        
*   VAL_RESULT_T
*/
VAL_RESULT_T eHalDeInit(
    VAL_HANDLE_T    *a_phHalHandle
    );
/**
* @par Function       
*   eHalCmdProc
* @par Description    
*   set command to MFV HW
* @param              
*   a_prParam         [IN]   The VAL_HANDLE_T structure
* @param              
*   a_prParam         [IN]   The HAL_CMD_T structure
* @param              
*   a_prParam         [IN]   The VAL_VOID_T structure
* @par Returns        
*   VAL_RESULT_T
*/
VAL_RESULT_T eHalCmdProc(
    VAL_HANDLE_T    a_hHalHandle,
    HAL_CMD_T       a_eHalCmd,
    VAL_VOID_T      *a_pvInParam,
    VAL_VOID_T      *a_pvOutParam
    );    



#ifdef __cplusplus
}
#endif

#endif // #ifndef _HAL_API_H_
