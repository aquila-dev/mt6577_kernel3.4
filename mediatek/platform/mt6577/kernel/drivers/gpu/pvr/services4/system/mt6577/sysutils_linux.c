/**********************************************************************
 *
 * Copyright (C) Imagination Technologies Ltd. All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope it will be useful but, except 
 * as otherwise stated in writing, without any warranty; without even the 
 * implied warranty of merchantability or fitness for a particular purpose. 
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 * 
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 *
 * Contact Information:
 * Imagination Technologies Ltd. <gpl-support@imgtec.com>
 * Home Park Estate, Kings Langley, Herts, WD4 8LZ, UK 
 *
 ******************************************************************************/

#include <linux/version.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/hardirq.h>
#include <linux/mutex.h>

#include "mach/mt6577_clock_manager.h"

#include "sgxdefs.h"
#include "services_headers.h"
#include "sysinfo.h"
#include "sgxapi_km.h"
#include "sysconfig.h"
#include "sgxinfokm.h"
#include "syslocal.h"

#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include "mach/mt6577_boot.h"
#include "mach/camera_sysram.h"

#define	ONE_MHZ	1000000
#define	HZ_TO_MHZ(m) ((m) / ONE_MHZ)

#define SGX_PARENT_CLOCK "core_ck"

#if defined(LDM_PLATFORM) && !defined(PVR_DRI_DRM_NOT_PCI)
extern struct platform_device *gpsPVRLDMDev;
#endif

#define ONLY_MFG_DEDICATE_BANK 0
#define USE_GDC_SHARE_MACRO 1
#define USE_SYS_CLOCK 0
#if defined(MTK_USE_GDC)
#if 0 
static IMG_VOID SysDumpGDCTag(const char* szTag)
{
	int i;
	if (szTag){
		PVR_DPF((PVR_DBG_WARNING, "%s", szTag));
	}
	for (i = 0; i < 16; i++){
		PVR_DPF((PVR_DBG_WARNING, "GDC_TAG%X_CTL: 0x%x", i, DRV_Reg32(MFG_APB_BASE + 0x10 + 4*i)));
	}
}
#endif
static IMG_VOID SysInitGDC(void)
{
	//SysDumpGDCTag("Before");
#if USE_GDC_SHARE_MACRO
	enable_clock(MT65XX_PDN_MM_GDC_SHARE_MACRO, "MFG");
	enable_clock(MT65XX_PDN_MM_GDC_SHARE_MACRO_HALF, "MFG");
#endif
	//DRV_WriteReg32(SMI_LARB3_BASE+0x70, 0x20FF0420);//C20C1070: Port Priority
	//DRV_WriteReg32(SMI_LARB3_BASE+0x70, 0x2004FF20);//C20C1070: Port Priority
	//DRV_WriteReg32(SMI_LARB3_BASE+0x70, 0x04040404);//C20C1070: Port Priority
	DRV_WriteReg32(SMI_LARB3_BASE+0x70, 0x01010101);//C20C1070: Port Priority
	DRV_WriteReg32(SMI_LARB3_BASE+0x74, 0x01);//C20C1074

	SYSRAM_MFG_SWITCH_BANK(false);

#if ONLY_MFG_DEDICATE_BANK
	DRV_WriteReg32(MFG_APB_BASE+0x08, 0xff);
#else
	DRV_WriteReg32(MFG_APB_BASE+0x08, 0xffffffff);
	//DRV_WriteReg32(SMI_BASE+0x30,	  0x7);
#endif

	if (get_chip_eco_ver() == CHIP_E1) {
	DRV_WriteReg32(MFG_APB_BASE+0x50, 0x3);
	} else {
		DRV_WriteReg32(MFG_APB_BASE+0x50, 0x1);
	}
	DRV_WriteReg32(MFG_APB_BASE+0x0C, 0x0);//Set GDC SW Reset
	DRV_WriteReg32(MFG_APB_BASE+0x0C, 0x1);//Set GDC SW Reset
	DRV_WriteReg32(MFG_APB_BASE+0x04, 0x1);//Set GDC clear
	DRV_WriteReg32(MFG_APB_BASE+0x04, 0x0);//Set GDC clear
	
	DRV_WriteReg32(MFG_APB_BASE+0x5C, 0x400);//Set SLF_CNT
	//SysDumpGDCTag("After");

	//PVRSRVReleasePrintf("[SysInitGDC]MFG_APB_BASE+0x08 = 0x%08x", DRV_Reg32(MFG_APB_BASE+0x08));
	//PVRSRVReleasePrintf("[SysInitGDC]MFG_APB_BASE+0x0C = 0x%08x", DRV_Reg32(MFG_APB_BASE+0x0C));
	//PVRSRVReleasePrintf("MFG Init GDC");
}

static IMG_VOID SysDeInitGDC(void)
{
	//PVRSRVReleasePrintf("Wait MFG_IDLE");
	while(1){
		if ((DRV_Reg32(MMSYS2_CONFG_BASE+0x518) & 0x1)) {
			DRV_WriteReg32(MFG_APB_BASE+0x08, 0xff);
			DRV_WriteReg32(MFG_APB_BASE+0x04, 0x1);//Set GDC clear
			DRV_WriteReg32(MFG_APB_BASE+0x04, 0x0);//Set GDC clear
			SYSRAM_MFG_SWITCH_BANK(false);
			break;
		}
	}
	//PVRSRVReleasePrintf("MFG_IDLE");
	//PVRSRVReleasePrintf("MFG_APB_BASE = 0x%08x", DRV_Reg32(MFG_APB_BASE));
#if USE_GDC_SHARE_MACRO
	disable_clock(MT65XX_PDN_MM_GDC_SHARE_MACRO_HALF, "MFG");
	disable_clock(MT65XX_PDN_MM_GDC_SHARE_MACRO, "MFG");
#endif
}
#endif

static PVRSRV_ERROR PowerLockWrap(SYS_SPECIFIC_DATA *psSysSpecData, IMG_BOOL bTryLock)
{
	if (!in_interrupt())
	{
		if (bTryLock)
		{
			int locked = mutex_trylock(&psSysSpecData->sPowerLock);
			if (locked == 0)
			{
				return PVRSRV_ERROR_RETRY;
			}
		}
		else
		{
		mutex_lock(&psSysSpecData->sPowerLock);
	}
}

	return PVRSRV_OK;
}

static IMG_VOID PowerLockUnwrap(SYS_SPECIFIC_DATA *psSysSpecData)
{
	if (!in_interrupt())
	{
		mutex_unlock(&psSysSpecData->sPowerLock);
	}
}

PVRSRV_ERROR SysPowerLockWrap(IMG_BOOL bTryLock)
{
	SYS_DATA	*psSysData;

	SysAcquireData(&psSysData);

	return PowerLockWrap(psSysData->pvSysSpecificData, bTryLock);
}

IMG_VOID SysPowerLockUnwrap(IMG_VOID)
{
	SYS_DATA	*psSysData;

	SysAcquireData(&psSysData);

	PowerLockUnwrap(psSysData->pvSysSpecificData);
}

IMG_BOOL WrapSystemPowerChange(SYS_SPECIFIC_DATA *psSysSpecData)
{
	return IMG_TRUE;
}

IMG_VOID UnwrapSystemPowerChange(SYS_SPECIFIC_DATA *psSysSpecData)
{
}

#if defined(SGX_DYNAMIC_TIMING_INFO)
IMG_VOID SysGetSGXTimingInformation(SGX_TIMING_INFORMATION *psTimingInfo)
{
#if !defined(NO_HARDWARE)
	PVR_ASSERT(atomic_read(&gpsSysSpecificData->sSGXClocksEnabled) != 0);
#endif
	psTimingInfo->ui32CoreClockSpeed = SYS_SGX_CLOCK_SPEED;
	psTimingInfo->ui32HWRecoveryFreq = SYS_SGX_HWRECOVERY_TIMEOUT_FREQ;
	psTimingInfo->ui32uKernelFreq = SYS_SGX_PDS_TIMER_FREQ;
#if defined(SUPPORT_ACTIVE_POWER_MANAGEMENT)
	psTimingInfo->bEnableActivePM = IMG_TRUE;
#else
	psTimingInfo->bEnableActivePM = IMG_FALSE;
#endif 
	psTimingInfo->ui32ActivePowManLatencyms = SYS_SGX_ACTIVE_POWER_LATENCY_MS;
}
#endif

PVRSRV_ERROR EnableSGXClocks(SYS_DATA *psSysData)
{
#if !defined(NO_HARDWARE)
	SYS_SPECIFIC_DATA *psSysSpecData = (SYS_SPECIFIC_DATA *) psSysData->pvSysSpecificData;

	
	if (atomic_read(&psSysSpecData->sSGXClocksEnabled) != 0)
	{
		return PVRSRV_OK;
	}

	PVR_DPF((PVR_DBG_MESSAGE, "EnableSGXClocks: Enabling SGX Clocks"));

#if defined(LDM_PLATFORM) && !defined(PVR_DRI_DRM_NOT_PCI) && defined(CONFIG_PM_RUNTIME)
	{
		
		int res = pm_runtime_get_sync(&gpsPVRLDMDev->dev);
		if (res < 0)
		{
			PVR_DPF((PVR_DBG_ERROR, "EnableSGXClocks: pm_runtime_get_sync failed (%d)", -res));
			return PVRSRV_ERROR_UNABLE_TO_ENABLE_CLOCK;
		}
	}
#endif


	//disable_clock(MT65XX_PDN_MM_MFG_HALF, "MFG");
	//disable_clock(MT65XX_PDN_MM_MFG, "MFG");
	//disable_clock(MT65XX_PDN_MM_G3D, "MFG");

#if USE_SYS_CLOCK
    #if !defined(MTK_CLK_CTRL)
	DRV_WriteReg16(PLL_CON11, DRV_Reg16(PLL_CON11)|0x0C00); // F3D_CK_SEL
    #endif
	DRV_WriteReg32(MMSYS2_CONFG_BASE + 0x0504, 0x4); // MFG_CORE_CK_SEL DCM
#else
    #if !defined(MTK_CLK_CTRL)
	DRV_WriteReg16(PLL_CON11, DRV_Reg16(PLL_CON11)|0x0C00); // F3D_CK_SEL
    #endif
	DRV_WriteReg32(MMSYS2_CONFG_BASE + 0x0504, 0x6); // MFG_CORE_CK_SEL DCM
#endif

	//DRV_WriteReg32(MMSYS2_CONFG_BASE+0x400, DRV_Reg32(MMSYS2_CONFG_BASE+0x400)|0x4);
	enable_clock(MT65XX_PDN_MM_G3D, "MFG");
	enable_clock(MT65XX_PDN_MM_MFG, "MFG");
	enable_clock(MT65XX_PDN_MM_MFG_HALF, "MFG");
	DRV_WriteReg32(MMSYS2_CONFG_BASE+0x400, DRV_Reg32(MMSYS2_CONFG_BASE+0x400)|0x4);


#if defined(MTK_USE_GDC)
	SysInitGDC();
#endif

#if 0
	DRV_WriteReg16(0xF0007404, 0x6);
	DRV_WriteReg16(0xF0007400, 0x4800);
	DRV_WriteReg16(0xF0007400, 0x0800);
	DRV_WriteReg16(0xF0007400, 0x8800);
	while(DRV_Reg16(0xF0007404) & 0x8000){}
	PVRSRVReleasePrintf("MainPLL: 0x%x", DRV_Reg16(0xF0007410));

	DRV_WriteReg16(0xF0007404, 0x15);
	DRV_WriteReg16(0xF0007400, 0x4800);
	DRV_WriteReg16(0xF0007400, 0x0800);
	DRV_WriteReg16(0xF0007400, 0x8800);
	while(DRV_Reg16(0xF0007404) & 0x8000){}
	PVRSRVReleasePrintf("MEMPLL: 0x%x", DRV_Reg16(0xF0007410));
#endif

	SysEnableSGXInterrupts(psSysData);

	atomic_set(&psSysSpecData->sSGXClocksEnabled, 1);

#else	
	PVR_UNREFERENCED_PARAMETER(psSysData);
#endif
	return PVRSRV_OK;
}


IMG_VOID DisableSGXClocks(SYS_DATA *psSysData)
{
#if !defined(NO_HARDWARE)
	SYS_SPECIFIC_DATA *psSysSpecData = (SYS_SPECIFIC_DATA *) psSysData->pvSysSpecificData;

	
	if (atomic_read(&psSysSpecData->sSGXClocksEnabled) == 0)
	{
		return;
	}

	PVR_DPF((PVR_DBG_MESSAGE, "DisableSGXClocks: Disabling SGX Clocks"));

	SysDisableSGXInterrupts(psSysData);

#if defined(MTK_USE_GDC)
	SysDeInitGDC();
#endif
	
	DRV_WriteReg32(MMSYS2_CONFG_BASE+0x400, DRV_Reg32(MMSYS2_CONFG_BASE+0x400)&~4);
	disable_clock(MT65XX_PDN_MM_MFG_HALF, "MFG");
	disable_clock(MT65XX_PDN_MM_MFG, "MFG");
	disable_clock(MT65XX_PDN_MM_G3D, "MFG");
	//DRV_WriteReg32(MMSYS2_CONFG_BASE+0x400, DRV_Reg32(MMSYS2_CONFG_BASE+0x400)&~4);

#if defined(LDM_PLATFORM) && !defined(PVR_DRI_DRM_NOT_PCI) && defined(CONFIG_PM_RUNTIME)
	{
		int res = pm_runtime_put_sync(&gpsPVRLDMDev->dev);
		if (res < 0)
		{
			PVR_DPF((PVR_DBG_ERROR, "DisableSGXClocks: pm_runtime_put_sync failed (%d)", -res));
		}
	}
#endif

	
	atomic_set(&psSysSpecData->sSGXClocksEnabled, 0);

#else	
	PVR_UNREFERENCED_PARAMETER(psSysData);
#endif	
}

/*
#if (defined(DEBUG) || defined(TIMING)) && !defined(PVR_NO_OMAP_TIMER)
#if defined(PVR_OMAP_USE_DM_TIMER_API)
#define	GPTIMER_TO_USE 11
static PVRSRV_ERROR AcquireGPTimer(SYS_SPECIFIC_DATA *psSysSpecData)
{
	PVR_ASSERT(psSysSpecData->psGPTimer == NULL);

	
	psSysSpecData->psGPTimer = omap_dm_timer_request_specific(GPTIMER_TO_USE);
	if (psSysSpecData->psGPTimer == NULL)
	{
	
		PVR_DPF((PVR_DBG_WARNING, "%s: omap_dm_timer_request_specific failed", __FUNCTION__));
		return PVRSRV_ERROR_CLOCK_REQUEST_FAILED;
	}

	
	omap_dm_timer_set_source(psSysSpecData->psGPTimer, OMAP_TIMER_SRC_SYS_CLK);
	omap_dm_timer_enable(psSysSpecData->psGPTimer);

	
	omap_dm_timer_set_load_start(psSysSpecData->psGPTimer, 1, 0);

	omap_dm_timer_start(psSysSpecData->psGPTimer);

	
	psSysSpecData->sTimerRegPhysBase.uiAddr = SYS_OMAP4430_GP11TIMER_REGS_SYS_PHYS_BASE;

	return PVRSRV_OK;
}

static void ReleaseGPTimer(SYS_SPECIFIC_DATA *psSysSpecData)
{
	if (psSysSpecData->psGPTimer != NULL)
	{
			
		(void) omap_dm_timer_stop(psSysSpecData->psGPTimer);

		omap_dm_timer_disable(psSysSpecData->psGPTimer);

		omap_dm_timer_free(psSysSpecData->psGPTimer);

		psSysSpecData->sTimerRegPhysBase.uiAddr = 0;

		psSysSpecData->psGPTimer = NULL;
	}

}
#else	
static PVRSRV_ERROR AcquireGPTimer(SYS_SPECIFIC_DATA *psSysSpecData)
{
#if defined(PVR_OMAP4_TIMING_PRCM)
	struct clk *psCLK;
	IMG_INT res;
	struct clk *sys_ck;
	IMG_INT rate;
#endif
	PVRSRV_ERROR eError;

	IMG_CPU_PHYADDR sTimerRegPhysBase;
	IMG_HANDLE hTimerEnable;
	IMG_UINT32 *pui32TimerEnable;

	PVR_ASSERT(psSysSpecData->sTimerRegPhysBase.uiAddr == 0);

#if defined(PVR_OMAP4_TIMING_PRCM)
	
	psCLK = clk_get(NULL, "gpt11_fck");
	if (IS_ERR(psCLK))
	{
		PVR_DPF((PVR_DBG_ERROR, "EnableSystemClocks: Couldn't get GPTIMER11 functional clock"));
		goto ExitError;
	}
	psSysSpecData->psGPT11_FCK = psCLK;

	psCLK = clk_get(NULL, "gpt11_ick");
	if (IS_ERR(psCLK))
	{
		PVR_DPF((PVR_DBG_ERROR, "EnableSystemClocks: Couldn't get GPTIMER11 interface clock"));
		goto ExitError;
	}
	psSysSpecData->psGPT11_ICK = psCLK;

	sys_ck = clk_get(NULL, "sys_clkin_ck");
	if (IS_ERR(sys_ck))
	{
		PVR_DPF((PVR_DBG_ERROR, "EnableSystemClocks: Couldn't get System clock"));
		goto ExitError;
	}

	if(clk_get_parent(psSysSpecData->psGPT11_FCK) != sys_ck)
	{
		PVR_TRACE(("Setting GPTIMER11 parent to System Clock"));
		res = clk_set_parent(psSysSpecData->psGPT11_FCK, sys_ck);
		if (res < 0)
		{
			PVR_DPF((PVR_DBG_ERROR, "EnableSystemClocks: Couldn't set GPTIMER11 parent clock (%d)", res));
		goto ExitError;
		}
	}

	rate = clk_get_rate(psSysSpecData->psGPT11_FCK);
	PVR_TRACE(("GPTIMER11 clock is %dMHz", HZ_TO_MHZ(rate)));

	res = clk_enable(psSysSpecData->psGPT11_FCK);
	if (res < 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "EnableSystemClocks: Couldn't enable GPTIMER11 functional clock (%d)", res));
		goto ExitError;
	}

	res = clk_enable(psSysSpecData->psGPT11_ICK);
	if (res < 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "EnableSystemClocks: Couldn't enable GPTIMER11 interface clock (%d)", res));
		goto ExitDisableGPT11FCK;
	}
#endif	

	
	sTimerRegPhysBase.uiAddr = SYS_OMAP4430_GP11TIMER_TSICR_SYS_PHYS_BASE;
	pui32TimerEnable = OSMapPhysToLin(sTimerRegPhysBase,
                  4,
                  PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
                  &hTimerEnable);

	if (pui32TimerEnable == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "EnableSystemClocks: OSMapPhysToLin failed"));
		goto ExitDisableGPT11ICK;
	}

	if(!(*pui32TimerEnable & 4))
	{
		PVR_TRACE(("Setting GPTIMER11 mode to posted (currently is non-posted)"));

		
		*pui32TimerEnable |= 4;
	}

	OSUnMapPhysToLin(pui32TimerEnable,
		    4,
		    PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
		    hTimerEnable);

	
	sTimerRegPhysBase.uiAddr = SYS_OMAP4430_GP11TIMER_ENABLE_SYS_PHYS_BASE;
	pui32TimerEnable = OSMapPhysToLin(sTimerRegPhysBase,
                  4,
                  PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
                  &hTimerEnable);

	if (pui32TimerEnable == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "EnableSystemClocks: OSMapPhysToLin failed"));
		goto ExitDisableGPT11ICK;
	}

	
	*pui32TimerEnable = 3;

	OSUnMapPhysToLin(pui32TimerEnable,
		    4,
		    PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
		    hTimerEnable);

	psSysSpecData->sTimerRegPhysBase = sTimerRegPhysBase;

	eError = PVRSRV_OK;

	goto Exit;

ExitDisableGPT11ICK:
#if defined(PVR_OMAP4_TIMING_PRCM)
	clk_disable(psSysSpecData->psGPT11_ICK);
ExitDisableGPT11FCK:
	clk_disable(psSysSpecData->psGPT11_FCK);
ExitError:
#endif	
	eError = PVRSRV_ERROR_CLOCK_REQUEST_FAILED;
Exit:
	return eError;
}

static void ReleaseGPTimer(SYS_SPECIFIC_DATA *psSysSpecData)
{
	IMG_HANDLE hTimerDisable;
	IMG_UINT32 *pui32TimerDisable;

	if (psSysSpecData->sTimerRegPhysBase.uiAddr == 0)
	{
		return;
	}

	
	pui32TimerDisable = OSMapPhysToLin(psSysSpecData->sTimerRegPhysBase,
				4,
				PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
				&hTimerDisable);

	if (pui32TimerDisable == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "DisableSystemClocks: OSMapPhysToLin failed"));
	}
	else
	{
		*pui32TimerDisable = 0;

		OSUnMapPhysToLin(pui32TimerDisable,
				4,
				PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
				hTimerDisable);
	}

	psSysSpecData->sTimerRegPhysBase.uiAddr = 0;

#if defined(PVR_OMAP4_TIMING_PRCM)
	clk_disable(psSysSpecData->psGPT11_ICK);

	clk_disable(psSysSpecData->psGPT11_FCK);
#endif	
}
#endif	
#else	
*/
static PVRSRV_ERROR AcquireGPTimer(SYS_SPECIFIC_DATA *psSysSpecData)
{
	PVR_UNREFERENCED_PARAMETER(psSysSpecData);

	return PVRSRV_OK;
}
static void ReleaseGPTimer(SYS_SPECIFIC_DATA *psSysSpecData)
{
	PVR_UNREFERENCED_PARAMETER(psSysSpecData);
}
//#endif 

PVRSRV_ERROR EnableSystemClocks(SYS_DATA *psSysData)
{
	SYS_SPECIFIC_DATA *psSysSpecData = (SYS_SPECIFIC_DATA *) psSysData->pvSysSpecificData;

	PVR_TRACE(("EnableSystemClocks: Enabling System Clocks"));

	if (!psSysSpecData->bSysClocksOneTimeInit)
	{
		mutex_init(&psSysSpecData->sPowerLock);

		atomic_set(&psSysSpecData->sSGXClocksEnabled, 0);

		psSysSpecData->bSysClocksOneTimeInit = IMG_TRUE;
	}

	return AcquireGPTimer(psSysSpecData);
}

IMG_VOID DisableSystemClocks(SYS_DATA *psSysData)
{
	SYS_SPECIFIC_DATA *psSysSpecData = (SYS_SPECIFIC_DATA *) psSysData->pvSysSpecificData;

	PVR_TRACE(("DisableSystemClocks: Disabling System Clocks"));

	
	DisableSGXClocks(psSysData);

	ReleaseGPTimer(psSysSpecData);
}

PVRSRV_ERROR SysPMRuntimeRegister(void)
{
#if defined(LDM_PLATFORM) && !defined(PVR_DRI_DRM_NOT_PCI)
	pm_runtime_enable(&gpsPVRLDMDev->dev);
#endif
	return PVRSRV_OK;
}

PVRSRV_ERROR SysPMRuntimeUnregister(void)
{
#if defined(LDM_PLATFORM) && !defined(PVR_DRI_DRM_NOT_PCI)
	pm_runtime_disable(&gpsPVRLDMDev->dev);
#endif
	return PVRSRV_OK;
}

#if defined(MTK_USE_GDC)
IMG_UINT32 SysCacheBypass(IMG_UINT32 ui32RegVal)
{
	if (get_chip_eco_ver() == CHIP_E1) {
		/* E1 chip */
		ui32RegVal |= 0x80U;
	} else {
		/* E2 chip */
		ui32RegVal |= 0x80U;
	}
	return ui32RegVal;
}
IMG_VOID OnSGXResetDone()
{
	SYSRAM_MFG_SWITCH_BANK(true);
	//PVRSRVReleasePrintf("MFG Reset Done");
}
#endif
