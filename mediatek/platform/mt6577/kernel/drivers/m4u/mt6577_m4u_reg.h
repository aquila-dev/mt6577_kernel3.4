#ifndef _MT6577_M4U_REG_H
#define _MT6577_M4U_REG_H

#include <asm/io.h>
#include <mach/mt6577_reg_base.h>
	 
/* ===============================================================
 * 					  M4U definition
 * =============================================================== */
#define SMI_COMMON_BASE          SMI_BASE
#define M4U_BASE0                SMI_LARB0_BASE
#define M4U_BASE1                SMI_LARB1_BASE
#define M4U_BASE2                SMI_LARB2_BASE
#define M4U_BASE3                SMI_LARB3_BASE

#define REG_MMU_PT_BASE_ADDR     0x800
#define REG_MMU_PROG_EN          0x810
#define ENABLE_MANUAL_MMU 0x1

#define REG_MMU_PROG_VA          0x814

///> Bit[31:12] Virtual address
#define TLB_TAG_VA_MASK          0xFFFFF000
///> Bit11 Lock bit
#define TLB_TAG_LOCK_BIT (0x1<<11)
#define TLB_TAG_VALID_BIT (0x1<<10)

#define REG_MMU_PROG_DSC         0x818
#define REG_MMU_RT_START_A0      0x820
#define RANGE_ENABLE (0x1<<11)

#define REG_MMU_RT_END_A0        0x824
#define REG_MMU_RT_START_A1      0x828
#define REG_MMU_RT_END_A1        0x82C
#define REG_MMU_RT_START_A2      0x830
#define REG_MMU_RT_END_A2        0x834
#define REG_MMU_RT_START_A3      0x838
#define REG_MMU_RT_END_A3        0x83C
#define REG_MMU_SQ_START_A0      0x840
#define REG_MMU_SQ_END_A0        0x844
#define REG_MMU_SQ_START_A1      0x848
#define REG_MMU_SQ_END_A1        0x84C
#define REG_MMU_SQ_START_A2      0x850
#define REG_MMU_SQ_END_A2        0x854
#define REG_MMU_SQ_START_A3      0x858
#define REG_MMU_SQ_END_A3        0x85C
#define REG_MMU_PFH_DIST0        0x880
#define REG_MMU_PFH_DIST1        0x884
#define REG_MMU_PFH_DIST2        0x888
#define REG_MMU_PFH_DIST3        0x88C
#define REG_MMU_PFH_DIST4        0x890
#define REG_MMU_PFH_DIST5        0x894
#define REG_MMU_PFH_DISTS0       0x8C0
#define REG_MMU_PFH_DISTS1       0x8C4
#define REG_MMU_PFH_DIR0         0x8F0
#define REG_MMU_PFH_DIR1         0x8F4
#define REG_MMU_MAIN_TAG0        0x900
#define REG_MMU_MAIN_TAG1        0x904
#define REG_MMU_MAIN_TAG2        0x908
#define REG_MMU_MAIN_TAG3        0x90C
#define REG_MMU_MAIN_TAG4        0x910
#define REG_MMU_MAIN_TAG5        0x914
#define REG_MMU_MAIN_TAG6        0x918
#define REG_MMU_MAIN_TAG7        0x91C
#define REG_MMU_MAIN_TAG8        0x920
#define REG_MMU_MAIN_TAG9        0x924
#define REG_MMU_MAIN_TAG10       0x928
#define REG_MMU_MAIN_TAG11       0x92C
#define REG_MMU_MAIN_TAG12       0x930
#define REG_MMU_MAIN_TAG13       0x934
#define REG_MMU_MAIN_TAG14       0x938
#define REG_MMU_MAIN_TAG15       0x93C
#define REG_MMU_MAIN_TAG16       0x940
#define REG_MMU_MAIN_TAG17       0x944
#define REG_MMU_MAIN_TAG18       0x948
#define REG_MMU_MAIN_TAG19       0x94C
#define REG_MMU_MAIN_TAG20       0x950
#define REG_MMU_MAIN_TAG21       0x954
#define REG_MMU_MAIN_TAG22       0x958
#define REG_MMU_MAIN_TAG23       0x95C
#define REG_MMU_MAIN_TAG24       0x960
#define REG_MMU_MAIN_TAG25       0x964
#define REG_MMU_MAIN_TAG26       0x968
#define REG_MMU_MAIN_TAG27       0x96C
#define REG_MMU_MAIN_TAG28       0x970
#define REG_MMU_MAIN_TAG29       0x974
#define REG_MMU_MAIN_TAG30       0x978
#define REG_MMU_MAIN_TAG31       0x97C
#define REG_MMU_PRE_TAG0         0x980
#define REG_MMU_PRE_TAG1         0x984
#define REG_MMU_PRE_TAG2         0x988
#define REG_MMU_PRE_TAG3         0x98C
#define REG_MMU_PRE_TAG4         0x990
#define REG_MMU_PRE_TAG5         0x994
#define REG_MMU_PRE_TAG6         0x998
#define REG_MMU_PRE_TAG7         0x99C
#define REG_MMU_PRE_TAG8         0x9A0
#define REG_MMU_PRE_TAG9         0x9A4
#define REG_MMU_PRE_TAG10        0x9A8
#define REG_MMU_PRE_TAG11        0x9AC
#define REG_MMU_PRE_TAG12        0x9B0
#define REG_MMU_PRE_TAG13        0x9B4
#define REG_MMU_PRE_TAG14        0x9B8
#define REG_MMU_PRE_TAG15        0x9BC
#define REG_MMU_PRE_TAG16        0x9C0
#define REG_MMU_PRE_TAG17        0x9C4
#define REG_MMU_PRE_TAG18        0x9C8
#define REG_MMU_PRE_TAG19        0x9CC
#define REG_MMU_PRE_TAG20        0x9D0
#define REG_MMU_PRE_TAG21        0x9D4
#define REG_MMU_PRE_TAG22        0x9D8
#define REG_MMU_PRE_TAG23        0x9DC
#define REG_MMU_PRE_TAG24        0x9E0
#define REG_MMU_PRE_TAG25        0x9E4
#define REG_MMU_PRE_TAG26        0x9E8
#define REG_MMU_PRE_TAG27        0x9EC
#define REG_MMU_PRE_TAG28        0x9F0
#define REG_MMU_PRE_TAG29        0x9F4
#define REG_MMU_PRE_TAG30        0x9F8
#define REG_MMU_PRE_TAG31        0x9FC
#define REG_MMU_READ_ENTRY       0xA00
#define REG_MMU_DES_RDATA        0xA04
#define REG_MMU_CTRL_REG         0xA10
#define REG_MMU_IVRP_PADDR       0xA14
#define REG_MMU_INT_CONTROL      0xA20
#define INT_CLR_BIT (0x1<<12)

#define REG_MMU_FAULT_ST         0xA24
#define INT_TRANSLATION_FAULT               0x01
#define INT_TLB_MULTI_HIT_FAULT             0x02
#define INT_INVALID_PHYSICAL_ADDRESS_FAULT  0x04
#define INT_ENTRY_REPLACEMENT_FAULT         0x08
#define INT_TABLE_WALK_FAULT                0x10
#define INT_TLB_MISS_FAULT                  0x20
#define INT_PRE_FETCH_DMA_FIFO_OVERFLOW     0x40
#define INT_VA_TO_PA_MAPPING_FAULT          0x80

#define REG_MMU_FAULT_VA         0xA28
#define REG_MMU_INVLD_PA         0xA2C
#define REG_MMU_ACC_CNT          0xA30
#define REG_MMU_MAIN_MSCNT       0xA34
#define REG_MMU_PF_MSCNT         0xA38
#define REG_MMU_PF_CNT           0xA3C
#define REG_MMU_WRAP_SA0         0xB00
#define REG_MMU_WRAP_EA0         0xB04
#define REG_MMU_WRAP_SA1         0xB08
#define REG_MMU_WRAP_EA1         0xB0C
#define REG_MMU_WRAP_SA2         0xB10
#define REG_MMU_WRAP_EA2         0xB14
#define REG_MMU_WRAP_SA3         0xB18
#define REG_MMU_WRAP_EA3         0xB1C
#define REG_MMU_WRAP_EN0         0xB40
#define REG_MMU_WRAP_EN1         0xB44
#define REG_MMU_WRAP_EN2         0xB48
#define SMI_LARB_BW_CRDT         0xC00
 /* ===============================================================
  * 					  SMI definition
  * =============================================================== */
#define REG_SMI_LARB_CON             0x0010
#define REG_SMI_LARB_CON_SET         0x0014
#define MAU_EN_BIT  (1<<10)

#define REG_SMI_LARB_CON_RESET       0x0018
#define REG_SMI_ARB_CON              0x0020
#define REG_SMI_ARB_CON_SET          0x0024
#define REG_SMI_ARB_CON_CLR          0x0028
#define REG_SMI_BW_EXT_CON0          0x0030
#define REG_SMI_BW_EXT_CON1          0x0034
#define REG_SMI_PROT_THRD            0x0038
#define REG_SMI_STARV_CON0           0x0040
#define REG_SMI_STARV_CON1           0x0044
#define REG_SMI_STARV_CON2           0x0048
#define REG_SMI_BW_CRDT              0x004C
#define REG_SMI_READ_FIFO_TH         0x0050
#define REG_SMI_INT_PATH_SEL         0x0054
#define REG_SMI_BW_LIMIT_CON0        0x0058
#define REG_SMI_BW_LIMIT_CON1        0x005C
#define REG_SMI_BW_VC0               0x0080
#define REG_SMI_BW_VC1               0x0084
#define REG_SMI_BW_VC2               0x0088
#define REG_SMI_EBW_PORT             0x0100
#define REG_SMI_MMUEN0               0x0200
#define REG_SMI_MMUEN1               0x0204
#define REG_SMI_MAU_ENTR0_START      0x0300
#define REG_SMI_MAU_ENTR0_END        0x0304
#define REG_SMI_MAU_ENTR0_GID_0      0x0308
#define REG_SMI_MAU_ENTR1_START      0x0310
#define REG_SMI_MAU_ENTR1_END        0x0314
#define REG_SMI_MAU_ENTR1_GID_0      0x0318
#define REG_SMI_MAU_ENTR2_START      0x0320
#define REG_SMI_MAU_ENTR2_END        0x0324
#define REG_SMI_MAU_ENTR2_GID_0      0x0328
#define RANGE_CHECK_READ_BIT 0x40000000
#define RANGE_CHECK_WRITE_BIT 0x80000000
#define RANGE_CHECK_VIRTUAL_ADDRESS 0x40000000

#define REG_SMI_LARB_MON_ENA         0x0400
#define REG_SMI_LARB_MON_CLR         0x0404
#define REG_SMI_LARB_MON_PORT        0x0408
#define REG_SMI_LARB_MON_TYPE        0x040C
#define REG_SMI_LARB_MON_CON         0x0410
#define REG_SMI_LARB_MON_ACT_CNT     0x0420
#define REG_SMI_LARB_MON_REQ_CNT     0x0424
#define REG_SMI_LARB_MON_IDL_CNT     0x0428
#define REG_SMI_LARB_MON_BEA_CNT     0x042C
#define REG_SMI_LARB_MON_BYT_CNT     0x0430
#define REG_SMI_LARB_MON_CP_CNT      0x0434
#define REG_SMI_LARB_MON_DP_CNT      0x0438
#define REG_SMI_LARB_MON_CDP_MAX     0x043C
#define REG_SMI_LARB_MON_COS_MAX     0x0440
#define REG_SMI_LARB_MON_BUS_REQ0    0x0450
#define REG_SMI_MAU_ENTR0_STAT       0x0500
#define REG_SMI_MAU_ENTR1_STAT       0x0504
#define REG_SMI_MAU_ENTR2_STAT       0x0508
#define MAU_RANGE_ASSERTION_BIT 0x80
#define MAU_RANGE_ASSERTION_ID_MASK 0x7F
#define REG_SMI_IRQ_STATUS           0x0520

///> SMI_COMMON+
#define REG_SMI_MMU_INVALIDATE       0x804
#define INVALIDATE_ALL 0x2
#define INVALIDATE_RANGE 0x1
#define REG_SMI_MMU_INVLD_SA         0x808
#define REG_SMI_MMU_INVLD_EA         0x80C
#define REG_SMI_RCP_CTRL             0x820 //0x82C
#define REG_SMI_MON_AXI_ENA          0x9A0
#define REG_SMI_MON_AXI_CLR          0x9A4
#define REG_SMI_MON_AXI_TYPE         0x9AC
#define REG_SMI_MON_AXI_CON          0x9B0
#define REG_SMI_MON_AXI_ACT_CNT      0x9C0
#define REG_SMI_MON_AXI_REQ_CNT      0x9C4
#define REG_SMI_MON_AXI_IDL_CNT      0x9C8
#define REG_SMI_MON_AXI_BEA_CNT      0x9CC
#define REG_SMI_MON_AXI_BYT_CNT      0x9D0
#define REG_SMI_MON_AXI_CP_CNT       0x9D4
#define REG_SMI_MON_AXI_DP_CNT       0x9D8
#define REG_SMI_MON_AXI_CDP_MAX      0x9DC
#define REG_SMI_MON_AXI_COS_MAX      0x9E0
#define REG_SMI_L1LEN                0xA00
#define REG_SMI_L1ARBA               0xA04
#define REG_SMI_L1ARBB               0xA08
#define REG_SMI_L1ARBC               0xA0C
#define REG_SMI_L1ARBD               0xA10

static inline unsigned int M4U_ReadReg32(unsigned int M4uBase, unsigned int Offset) 
{
  return ioread32(M4uBase+Offset);
}
static inline void M4U_WriteReg32(unsigned int M4uBase, unsigned int Offset, unsigned int Val) 
{                   
  iowrite32(Val, M4uBase+Offset);  
  mb();
}

static inline unsigned int SMI_ReadReg32(unsigned int Offset) 
{
  return ioread32(SMI_COMMON_BASE+Offset);
}
static inline void SMI_WriteReg32(unsigned int Offset, unsigned int Val)
{                   
  iowrite32(Val, SMI_COMMON_BASE+Offset);
  mb();
}

#endif //_MMU_REG_H

