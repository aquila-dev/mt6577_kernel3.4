#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <asm/io.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/earlysuspend.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/aee.h>
#include <linux/timer.h>
#include <asm/system.h>
#include <asm-generic/irq_regs.h>

#include <asm/mach/map.h>
#include <mach/sync_write.h>
#include <mach/mt6577_irq.h>
#include <mach/mt6577_clock_manager.h>
#include <mach/mt6577_emi_mpu.h>
#include <mach/mt6577_device_apc.h>
#include <mach/irqs.h>
#include <asm/cacheflush.h>

//#include <mach/mt6577_m4u.h>

#include <mach/mt6575_spc.h>
#include "mt6575_spc_reg.h"
#include <linux/xlog.h>




#define MTK_SPC_MSG
#ifdef MTK_SPC_MSG
    #define SPCMSG(string, args...)	xlog_printk(ANDROID_LOG_INFO, "SPC", "[pid=%d]"string,current->tgid,##args);
#else
    #define SPCMSG(string, args...)
#endif

#define SPCERR(string, args...) do { \
    xlog_printk(ANDROID_LOG_ERROR, "SPC", "[pid=%d]error: "string,current->tgid,##args);  \
    aee_kernel_exception("SPC", "[SPC_K] error:"string,##args); \
}while(0)


#define MTK_SPC_DEV_MAJOR_NUMBER 187
static struct cdev * g_pMTKSPC_CharDrv = NULL;
static dev_t g_MTKSPCdevno ;//= MKDEV(MTK_SPC_DEV_MAJOR_NUMBER,0);
#define SPC_DEVNAME "SPC_device"


#define LARB_NUM 4
unsigned int spc_word_offset[LARB_NUM]= {0, 2, 5, 6};
unsigned int spc_bit_offset[LARB_NUM] = {0, 21, 24, 21};
unsigned int spc_engine_num[LARB_NUM] = {27, 31, 9, 4}; 
unsigned int spc_engine_mask[LARB_NUM]= {0, 0, 0, 0};

const char* spcPortName[SPC_MASTER_NUMBER] = {
    [0] = "undefine",
    
    [1 ] = "defect",       // 1
    [2 ] = "cam",
    [3 ] = "rotdma0_out1",
    [4 ] = "rotdma0_out2",
    [5 ] = "rotdma1_out1", // 5
    [6 ] = "rotdma1_out2", 
    [7 ] = "tvrot_out1",
    [8 ] = "tvrot_out2",
    [9 ] = "fd1",
    [10] = "pca",         // 10
    [11] = "jpegdma_r",    
    [12] = "jpegdma_w",
    [13] = "rdma0_out3",
    [14] = "jpeg_dec1",
    [15] = "tvrot_out3",  // 15
    [16] = "rotdma0_out3", 
    [17] = "rotdma1_out3",
    [18] = "emi_18",
      
    [19] = "rotdma2_out1",
    [20] = "rotdma2_out2",  // 20
    [21] = "rotdma3_out1",
    [22] = "rotdma3_out2",
    [23] = "rotdma4_out1",
    [24] = "rotdma4_out2",
    [25] = "tvc_pfh",       // 25
    [26] = "tvc_resz",
    [27] = "ovl_dcp",
    [28] = "vrz",
    [29] = "greq_blkw",
    [30] = "greq_blkr",     // 30
    [31] = "rotdma1_out3",
    [32] = "eis",
    [33] = "rotdma2_out3",
    [34] = "rotdma3_out3",
    [35] = "rotdma4_out3",  // 35
    [36] = "emi_36",
              
    [37] = "vdec_post1",
    [38] = "emi_38",
              
    [39] = "audio",
    [40] = "emi_40"         // 40
};



unsigned int spc_status_check(void);
unsigned int spc_dump_reg(void);
unsigned int spc_register_isr(void);
unsigned int spc_get_engine_id_offset(unsigned int spc_index);
unsigned int spc_clear_irq(void);


int spc_test(int code) 
{
    
    if(code < 0)
        return 0;
    
  if(code==1) //for test
  {
      SPCMSG("spc_test, code=1 \n");
      {
          SPC_cfg spc_struct;
          spc_struct.RegionID = 1;       
          spc_struct.Enable = 1;         

          spc_struct.EngineMaskLARB0 = LARB0_ALL_EN;
          spc_struct.EngineMaskLARB1 = LARB1_ALL_EN; 
          spc_struct.EngineMaskLARB2 = LARB2_ALL_EN;
          spc_struct.EngineMaskLARB3 = LARB3_ALL_EN;
          
          spc_struct.ReadEn = 1;         
          spc_struct.WriteEn = 1;        
          spc_struct.StartAddr = MM_SYSRAM_BASE_PA;      
          spc_struct.EndAddr = MM_SYSRAM_BASE_PA + MM_SYSRAM_SIZE -1;
          spc_config(&spc_struct);       	
      }       
      return 0; 	
  } 

  if(code==2) //for test
  {
      SPCMSG("spc_test, code=2 \n");
      {
          SPC_cfg spc_struct;
          spc_struct.RegionID = 1;       
          spc_struct.Enable = 1;         
          
          spc_struct.EngineMaskLARB0 = LARB0_ALL_EN;
          spc_struct.EngineMaskLARB1 = LARB1_ALL_EN;//SPC_MONITOR_ALL_LARB1 & (~(1<<(SPC_ID_ROTDMA2_OUT3-spc_get_engine_id_offset(1))));
          spc_struct.EngineMaskLARB2 = LARB2_ALL_EN;
          spc_struct.EngineMaskLARB3 = LARB3_ALL_EN;
          
          spc_struct.ReadEn = 1;         
          spc_struct.WriteEn = 1;        
          spc_struct.StartAddr = 0xC2020000;      
          spc_struct.EndAddr = 0xC2020000 + 32*1024 -1;
          spc_config(&spc_struct);       	
      }       
      return 0; 	
  } 

  if(code==3) //for test
  {
      SPCMSG("spc_test , code=3 \n");
      {
          SPC_cfg spc_struct;
          spc_struct.RegionID = 1;       
          spc_struct.Enable = 1;         
          spc_struct.EngineMaskLARB0 = LARB0_TVROT_OUT1_EN |LARB0_TVROT_OUT2_EN |LARB0_TVROT_OUT3_EN ;
          spc_struct.EngineMaskLARB1 = LARB1_TVC_PFH_EN | LARB1_TVC_RESZ_EN;
          spc_struct.EngineMaskLARB2 = 0; //SPC_MONITOR_ALL_LARB2;
          spc_struct.EngineMaskLARB3 = 0; //SPC_MONITOR_ALL_LARB3;
          spc_struct.ReadEn = 1;         
          spc_struct.WriteEn = 1;        
          spc_struct.StartAddr = MM_SYSRAM_BASE_PA;      
          spc_struct.EndAddr = MM_SYSRAM_BASE_PA + MM_SYSRAM_SIZE -1;
          spc_config(&spc_struct);       	
      }       
      return 0; 	
  } 
  if(code==4) //for test
  {
      SPCMSG("spc_test, 4 \n");
      spc_status_check();     
      return 0; 	
  } 
  if(code==5) //for test
  {
      SPCMSG("spc_test, 5\n");
      // SPC related
      {
          spc_register_isr();
      }       
      return 0; 	
  }  
  if(code==6) //for test
  {
      SPCMSG("spc_test, eModuleID=103 \n");
      // SPC related
      {
          SPC_cfg spc_struct;
          spc_struct.RegionID = 1;       
          spc_struct.Enable = 1;         
          

          spc_struct.EngineMaskLARB0 = 0; //SPC_MONITOR_ALL_LARB0;
          spc_struct.EngineMaskLARB1 = 0;
          spc_struct.EngineMaskLARB2 = 0; //SPC_MONITOR_ALL_LARB2;
          spc_struct.EngineMaskLARB3 = 0; //SPC_MONITOR_ALL_LARB3;

          
          spc_struct.ReadEn = 0;         
          spc_struct.WriteEn = 1;        
          spc_struct.StartAddr = MM_SYSRAM_BASE_PA;      
          spc_struct.EndAddr = MM_SYSRAM_BASE_PA + MM_SYSRAM_SIZE -1;
          spc_config(&spc_struct);       	
      }       
      return 0; 	
  }
  if(code==7) //for test
  {

      unsigned int *sysram_va;
      SPCMSG("spc_test,7 \n");
      
      {
          SPC_cfg spc_struct;
          spc_struct.RegionID = 1;       
          spc_struct.Enable = 1;         

          spc_struct.EngineMaskLARB0 = LARB0_ALL_EN;
          spc_struct.EngineMaskLARB1 = LARB1_ALL_EN; 
          spc_struct.EngineMaskLARB2 = LARB2_ALL_EN;
          spc_struct.EngineMaskLARB3 = LARB3_ALL_EN;
          
          spc_struct.ReadEn = 0;         
          spc_struct.WriteEn = 0;        
          spc_struct.StartAddr = MM_SYSRAM_BASE_PA;      
          spc_struct.EndAddr = MM_SYSRAM_BASE_PA + MM_SYSRAM_SIZE -1;
          spc_config(&spc_struct);       	
      }       

      sysram_va = (unsigned int*)ioremap_nocache(MM_SYSRAM_BASE_PA, MM_SYSRAM_SIZE);

      SPCMSG("cpu read sysram: 0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x\n",
          sysram_va[0],sysram_va[1],sysram_va[2],sysram_va[3],
          sysram_va[4],sysram_va[5],sysram_va[6],sysram_va[7]);
      

      
  return 0;    
}

  

  return 0;    
}

#define __SPC_FORBID_ALL__

unsigned int spc_config(SPC_cfg *pSpcConf)
{
    unsigned int reg_value, reg_addr, master_bit;
    unsigned int i,j;
    unsigned int access_type;
        
    SPCMSG("enter spc_config() \n");
    SPCMSG("[SPC] RegionID=0x%x, enable=%d, EngineMaskLARB0=0x%x, EngineMaskLARB1=0x%x, EngineMaskLARB2=0x%x \n",
        pSpcConf->RegionID, 
        pSpcConf->Enable,
        pSpcConf->EngineMaskLARB0, 
        pSpcConf->EngineMaskLARB1,  
        pSpcConf->EngineMaskLARB2);	
    SPCMSG("[SPC] EngineMaskLARB3=0x%x, ReadEn=0x%x, WriteEn=0x%x, StartAddr=0x%x, EndAddr=0x%x \n",
        pSpcConf->EngineMaskLARB3, 
        pSpcConf->ReadEn,
        pSpcConf->WriteEn,
        pSpcConf->StartAddr,
        pSpcConf->EndAddr);	

    ASSERT(pSpcConf->EndAddr > pSpcConf->StartAddr);
        
    // config SPC registers    
    if(pSpcConf->Enable == 0)
    {
        reg_value = SPC_ReadReg32(REG_SMI_SRNG_ACTL1);
        reg_value &= (~0x3ff);
        SPC_WriteReg32(REG_SMI_SRNG_ACTL1, reg_value);
        return 0;
    }
    else
    {
        //config SRAM region 1 as secure region
        reg_value = SPC_ReadReg32(REG_SMI_SRAM_RANGE0);
    
        reg_value &= 0xffff0000; //clear low 16bit
        reg_value |= ((pSpcConf->StartAddr)&0x7ffff) >> 3;  //16bit valid addr [18:3]
        SPC_WriteReg32(REG_SMI_SRAM_RANGE0, reg_value);
 
        reg_value &= 0xffff0000;
        reg_value |= ((pSpcConf->EndAddr)&0x7ffff) >> 3;
        SPC_WriteReg32(REG_SMI_SRAM_RANGE1, reg_value);     

        SPC_WriteReg32(REG_SMI_SRAM_RANGE2, SPC_ReadReg32(REG_SMI_SRAM_RANGE1));
        SPC_WriteReg32(REG_SMI_SRAM_RANGE3, SPC_ReadReg32(REG_SMI_SRAM_RANGE1));

        if(pSpcConf->ReadEn==1 && pSpcConf->WriteEn==1)
        {
            access_type = 1;
        }
        else if(pSpcConf->ReadEn==0 && pSpcConf->WriteEn==1)
        {
             access_type = 2;   
        }
    #ifdef __SPC_FORBID_ALL__
        else if(pSpcConf->ReadEn==0 && pSpcConf->WriteEn==0)
        {
            access_type = 3;
        }
    #endif
        else
        {
             SPCMSG("error: SPC can only monitor wirte-only or write+read access type! \n");   
             ASSERT(0);
        }
        
        reg_value = SPC_ReadReg32(REG_SMI_SRNG_ACTL1);
        reg_value &= (~0x3ff);
        reg_value |= (access_type | (access_type<<2) | (access_type<<4));
        SPC_WriteReg32(REG_SMI_SRNG_ACTL1, reg_value);
        
        //enable abort signal when illegal access happen
        ///> disable APB violation
        reg_value = SPC_ReadReg32(REG_SMI_D_VIO_CON0);
        reg_value |= (1<<5);
        SPC_WriteReg32(REG_SMI_D_VIO_CON0, reg_value);        

        reg_value = SPC_ReadReg32(REG_SMI_D_VIO_CON1);
        reg_value |= (1<<5);
        SPC_WriteReg32(REG_SMI_D_VIO_CON1, reg_value);     
        
        reg_value = SPC_ReadReg32(REG_SMI_D_VIO_CON2);
        reg_value |= (1<<5);
        SPC_WriteReg32(REG_SMI_D_VIO_CON2, reg_value);  
        
        spc_engine_mask[0] = pSpcConf->EngineMaskLARB0;
        spc_engine_mask[1] = pSpcConf->EngineMaskLARB1;
        spc_engine_mask[2] = pSpcConf->EngineMaskLARB2;
        spc_engine_mask[3] = pSpcConf->EngineMaskLARB3;
        for(i=0;i<LARB_NUM;i++)
        for(j=0;j<spc_engine_num[i];j++)
        {
            reg_addr = REG_SMI_SECUR_CON0 + spc_word_offset[i]*4 + (spc_bit_offset[i]+j*3)/30*4;
            reg_value = SPC_ReadReg32(reg_addr);
            master_bit = 1<<((spc_bit_offset[i]+j*3)%30);  
            if(spc_engine_mask[i]&(1<<j)) //set engine to be non-secure
            {
            		SPC_WriteReg32(reg_addr, reg_value&(~master_bit));
            }
            else
            {
            	  SPC_WriteReg32(reg_addr, reg_value|(master_bit));
            }
        }        
    }    
   
    return 0;    
}

EXPORT_SYMBOL(spc_config);

unsigned int spc_status_check()
{
    
    if(SPC_ReadReg32(REG_SMI_D_VIO_STA0)!=0)
    {
         SPCMSG("SPC domain0 abort happens! STA0=0x%x \n", SPC_ReadReg32(REG_SMI_D_VIO_STA0)); 
         SPC_WriteReg32(REG_SMI_D_VIO_STA0, SPC_ReadReg32(REG_SMI_D_VIO_STA0)); //write bit=1 to clear the bit 
    }
    if(SPC_ReadReg32(REG_SMI_D_VIO_STA1)!=0)
    {
         SPCMSG("SPC domain1 abort happens! STA1=0x%x \n", SPC_ReadReg32(REG_SMI_D_VIO_STA1)); 
         SPC_WriteReg32(REG_SMI_D_VIO_STA1, SPC_ReadReg32(REG_SMI_D_VIO_STA1)); //write bit=1 to clear the bit 
    }
    if(SPC_ReadReg32(REG_SMI_D_VIO_STA2)!=0)
    {
         SPCMSG("SPC domain2 abort happens! STA0=0x%x \n", SPC_ReadReg32(REG_SMI_D_VIO_STA2)); 
         SPC_WriteReg32(REG_SMI_D_VIO_STA2, SPC_ReadReg32(REG_SMI_D_VIO_STA2)); //write bit=1 to clear the bit 
    }
            
    if(SPC_ReadReg32(REG_SMI_VIO_DBG0)&(1<<29))
    {
        SPCMSG("sram read violation! \n");    
        SPCMSG("violation masterID=%d, masterName=%s, domainID=%d, addr=0x%x \n", 
                SPC_ReadReg32(REG_SMI_VIO_DBG0)&0x7f,
                spcPortName[SPC_ReadReg32(REG_SMI_VIO_DBG0)&0x7f],
                (SPC_ReadReg32(REG_SMI_VIO_DBG0)>>12)&0x3,
                SPC_ReadReg32(REG_SMI_VIO_DBG1));
    }
    if(SPC_ReadReg32(REG_SMI_VIO_DBG0)&(1<<28))
    {
        SPCMSG("sram write violation! \n");    
        SPCMSG("violation masterID=%d, masterName=%s, domainID=%d, addr=0x%x \n", 
                SPC_ReadReg32(REG_SMI_VIO_DBG0)&0x7f,
                spcPortName[SPC_ReadReg32(REG_SMI_VIO_DBG0)&0x7f],
                (SPC_ReadReg32(REG_SMI_VIO_DBG0)>>12)&0x3,
                SPC_ReadReg32(REG_SMI_VIO_DBG1));
    }
    if(SPC_ReadReg32(REG_SMI_VIO_DBG0)&(1<<24))
    {
        SPCMSG("APB violation! \n");    
        SPCMSG("violation masterID=%d, masterName=%s, domainID=%d, addr=0x%x \n", 
                SPC_ReadReg32(REG_SMI_VIO_DBG0)&0x7f,
                spcPortName[SPC_ReadReg32(REG_SMI_VIO_DBG0)&0x7f],
                (SPC_ReadReg32(REG_SMI_VIO_DBG0)>>12)&0x3,
                SPC_ReadReg32(REG_SMI_VIO_DBG1)&0x7fff);        
    }  
    
    SPC_WriteReg32(REG_SMI_VIO_DBG0, 1<<31); //clear interrupt
  
    return 0;
}

unsigned int spc_dump_reg()
{
    SPCMSG("SPC Reg Dump Start !\n");
    SPCMSG("(+0x500)SMI_SEN        = 0x%x \n", SPC_ReadReg32(REG_SMI_SEN        ));
    SPCMSG("(+0x520)SMI_SRAM_RANGE0= 0x%x \n", SPC_ReadReg32(REG_SMI_SRAM_RANGE0));
    SPCMSG("(+0x524)SMI_SRAM_RANGE1= 0x%x \n", SPC_ReadReg32(REG_SMI_SRAM_RANGE1));
    SPCMSG("(+0x528)SMI_SRAM_RANGE2= 0x%x \n", SPC_ReadReg32(REG_SMI_SRAM_RANGE2));
    SPCMSG("(+0x52C)SMI_SRAM_RANGE3= 0x%x \n", SPC_ReadReg32(REG_SMI_SRAM_RANGE3));
    SPCMSG("(+0x530)SMI_SRNG_ACTL0 = 0x%x \n", SPC_ReadReg32(REG_SMI_SRNG_ACTL0 ));
    SPCMSG("(+0x534)SMI_SRNG_ACTL1 = 0x%x \n", SPC_ReadReg32(REG_SMI_SRNG_ACTL1 ));
    SPCMSG("(+0x538)SMI_SRNG_ACTL2 = 0x%x \n", SPC_ReadReg32(REG_SMI_SRNG_ACTL2 ));
    SPCMSG("(+0x53C)SMI_SRNG_ACTL3 = 0x%x \n", SPC_ReadReg32(REG_SMI_SRNG_ACTL3 ));
    SPCMSG("(+0x540)SMI_SRNG_ACTL4 = 0x%x \n", SPC_ReadReg32(REG_SMI_SRNG_ACTL4 ));
    SPCMSG("(+0x550)SMI_D_VIO_CON0 = 0x%x \n", SPC_ReadReg32(REG_SMI_D_VIO_CON0 ));
    SPCMSG("(+0x554)SMI_D_VIO_CON1 = 0x%x \n", SPC_ReadReg32(REG_SMI_D_VIO_CON1 ));
    SPCMSG("(+0x558)SMI_D_VIO_CON2 = 0x%x \n", SPC_ReadReg32(REG_SMI_D_VIO_CON2 ));
    SPCMSG("(+0x560)SMI_D_VIO_STA0 = 0x%x \n", SPC_ReadReg32(REG_SMI_D_VIO_STA0 ));
    SPCMSG("(+0x564)SMI_D_VIO_STA1 = 0x%x \n", SPC_ReadReg32(REG_SMI_D_VIO_STA1 ));
    SPCMSG("(+0x568)SMI_D_VIO_STA2 = 0x%x \n", SPC_ReadReg32(REG_SMI_D_VIO_STA2 ));
    SPCMSG("(+0x570)SMI_VIO_DBG0   = 0x%x \n", SPC_ReadReg32(REG_SMI_VIO_DBG0   ));
    SPCMSG("(+0x570)SMI_VIO_DBG1   = 0x%x \n", SPC_ReadReg32(REG_SMI_VIO_DBG1   ));
    SPCMSG("(+0x5C0)SMI_SECUR_CON0 = 0x%x \n", SPC_ReadReg32(REG_SMI_SECUR_CON0 ));
    SPCMSG("(+0x5C4)SMI_SECUR_CON1 = 0x%x \n", SPC_ReadReg32(REG_SMI_SECUR_CON1 ));
    SPCMSG("(+0x5C8)SMI_SECUR_CON2 = 0x%x \n", SPC_ReadReg32(REG_SMI_SECUR_CON2 ));
    SPCMSG("(+0x5CC)SMI_SECUR_CON3 = 0x%x \n", SPC_ReadReg32(REG_SMI_SECUR_CON3 ));
    SPCMSG("(+0x5D0)SMI_SECUR_CON4 = 0x%x \n", SPC_ReadReg32(REG_SMI_SECUR_CON4 ));
    SPCMSG("(+0x5D4)SMI_SECUR_CON5 = 0x%x \n", SPC_ReadReg32(REG_SMI_SECUR_CON5 ));
    SPCMSG("(+0x5D8)SMI_SECUR_CON6 = 0x%x \n", SPC_ReadReg32(REG_SMI_SECUR_CON6 ));
    SPCMSG("(+0x5DC)SMI_SECUR_CON7 = 0x%x \n", SPC_ReadReg32(REG_SMI_SECUR_CON7 ));	
    SPCMSG("SPC Reg Dump End !\n");
    
    return 0;
}

static irqreturn_t MTK_SPC_isr(int irq, void *dev_id)
{
    if ( readl(AP_DEVAPC0_DXS_VIO_STA) != 0x4)
    {
    	SPCMSG("NOT spc abort \n");
        return IRQ_NONE;
    }

    if(SPC_ReadReg32(REG_SMI_D_VIO_STA0)==0 &&
    	 SPC_ReadReg32(REG_SMI_D_VIO_STA1)==0 &&
    	 SPC_ReadReg32(REG_SMI_D_VIO_STA2)==0)
    {
    	  SPCMSG("NOT spc abort (but DEVAPC says it is...) ?\n");
          return IRQ_NONE;
    }

    SPCERR("SPC violation happens!!\n");

    spc_status_check();    	

    spc_clear_irq();
    	
    return IRQ_HANDLED;
}


unsigned int spc_register_isr()
{
    unsigned int ret;

#if defined(CONFIG_MT6575_FPGA)
    //!!! TODO: ask the irq in fpga
    
    return 0;
#else

    ret = request_irq(MT6577_APARM_DOMAIN_IRQ_ID, (irq_handler_t)MTK_SPC_isr, IRQF_TRIGGER_LOW | IRQF_SHARED, SPC_DEVNAME, g_pMTKSPC_CharDrv);
    if(ret != 0)
    {
        SPCMSG("request SPC IRQ line failed! ret=0x%x \n", ret);
        return ret;
    }
    else
    {
        SPCMSG("spc_register_isr success! \n");
    }

#endif

    return 0;
}


void spc_unregister_isr(void)
{

#if defined(CONFIG_MT6575_FPGA)
    return;
#else
    free_irq(MT6577_APARM_DOMAIN_IRQ_ID ,  g_pMTKSPC_CharDrv);
#endif
}



#define ABORT_SMI  0x80000000   // todo: ask Eddie what dose this macro mean
unsigned int spc_clear_irq()
{
    writel(readl(MM_DEVAPC0_D0_VIO_STA) | ABORT_SMI , MM_DEVAPC0_D0_VIO_STA);
    while((readl(MM_DEVAPC0_D0_VIO_STA) & ABORT_SMI) != 0); 
   
    writel(0x4, AP_DEVAPC0_DXS_VIO_STA);
    while(( readl(AP_DEVAPC0_DXS_VIO_STA) & 0x4) != 0);
    return 0;
}


static int SPC_open(struct inode * a_pstInode, struct file * filep)
{

    return 0;
}
 
static int SPC_release(struct inode * a_pstInode, struct file * filep)
{
    return 0;
}

static int SPC_flush(struct file * a_pstFile , fl_owner_t a_id)
{
    return 0;
}


long SPC_unlocked_ioctl(struct file *filep, unsigned int cmd, unsigned long para)
{
    int ret;
    SPC_cfg cfg;

    switch(cmd)
    {

        case MT6575SPC_CONFIG :			
            ret = copy_from_user(&cfg, (void*)para , sizeof(SPC_cfg));
            if(ret)
            {
            	SPCMSG(" MT6575SPC_CONFIG, copy_from_user failed: %d\n", ret);
            	return -EFAULT;
            }  

            spc_config(&cfg);

            if(ret)
            {
            	SPCMSG(" m4u_alloc_mva failed: %d\n", ret);
            	return -EFAULT;
            } 
            break;

        case MT6575SPC_DUMP_REG :
            spc_dump_reg();
            break;


        case MT6575SPC_STATUS_CHECK :
            spc_status_check();    	
            break;

        case MT6575SPC_CMD :
            spc_test(para);
            break;

        default :
            break;
    }



    return 0;
}


static const struct file_operations g_stMTK_SPC_fops = 
{
	.owner = THIS_MODULE,
	.open = SPC_open,
	.release = SPC_release,
	.flush = SPC_flush,
	.unlocked_ioctl = SPC_unlocked_ioctl
};

static struct class *pSPCClass = NULL;
static int __init MTK_SPC_Init(void)
{
    int ret;

    SPCMSG("MT6575_SPC_init() \n");

    if (alloc_chrdev_region(&g_MTKSPCdevno, 0, 1,"spc"))
    {
        SPCMSG(" Allocate device no failed\n");
        return -EAGAIN;
    }
    else
    {
        SPCMSG(" spc devices number is: %d\n", (int)g_MTKSPCdevno);
    }
    
    
    g_pMTKSPC_CharDrv = cdev_alloc();
    g_pMTKSPC_CharDrv->owner = THIS_MODULE;
    g_pMTKSPC_CharDrv->ops = &g_stMTK_SPC_fops;
    ret = cdev_add(g_pMTKSPC_CharDrv, g_MTKSPCdevno, 1);


    pSPCClass = class_create(THIS_MODULE, "spc");
    if (IS_ERR(pSPCClass)) {
        int ret = PTR_ERR(pSPCClass);
        SPCMSG("Unable to create class, err = %d\n", ret);
        return ret;
    }
    device_create(pSPCClass, NULL, g_MTKSPCdevno, NULL, "spc");

 
    writel(readl(AP_DEVAPC0_APC_CON) &  (0xFFFFFFFF ^ (1<<2)), AP_DEVAPC0_APC_CON);
    writel(readl(AP_DEVAPC1_APC_CON) &  (0xFFFFFFFF ^ (1<<2)), AP_DEVAPC1_APC_CON);
    writel(readl(MM_DEVAPC0_APC_CON) &  (0xFFFFFFFF ^ (1<<2)), MM_DEVAPC0_APC_CON);
    writel(readl(MM_DEVAPC1_APC_CON) &  (0xFFFFFFFF ^ (1<<2)), MM_DEVAPC1_APC_CON);
    writel(0x0000007F, AP_DEVAPC0_DXS_VIO_STA);
    //writel(0x00FF00FB, AP_DEVAPC0_DXS_VIO_MASK);  // 0xfb:MM, 0xfd:EMI, 0xf9:Both
    writel(readl(AP_DEVAPC0_DXS_VIO_MASK)&(~0x4), AP_DEVAPC0_DXS_VIO_MASK);  // 0xfb:MM, 0xfd:EMI, 0xf9:Both

    writel(readl(MM_DEVAPC0_D0_VIO_STA) | ABORT_SMI , MM_DEVAPC0_D0_VIO_STA);
    writel(readl(MM_DEVAPC0_D0_VIO_MASK) & ~ABORT_SMI , MM_DEVAPC0_D0_VIO_MASK);

    spc_register_isr();

    return 0;

}

static void __exit MTK_SPC_Exit(void)
{
    SPCMSG("MT6575_SPC_Exit() \n");

    spc_unregister_isr();

}

module_init(MTK_SPC_Init);
module_exit(MTK_SPC_Exit);
                      

MODULE_DESCRIPTION("MTK SPC driver");
MODULE_AUTHOR("MTK80347 <Xiang.Xu@mediatek.com>");
MODULE_LICENSE("Proprietary");
