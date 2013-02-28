#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include "musbfsh_core.h"
#include "musbfsh_mt65xx.h"
#include <mach/mt6577_gpio.h>
#include <mach/mt6577_clock_manager.h>
#include <mach/mt6577_pm_ldo.h>

// unsigned char __iomem *usb11_phy_addr = (unsigned char __iomem*)USB11_PHY_ADDR;

void mt65xx_usb11_phy_poweron(void)
{
    unsigned char reg_value = 0;
    INFO("mt65xx_usb11_phy_poweron++\r\n");
	
    enable_pll(MT65XX_UPLL, "USB11");
	udelay(100); // PHY power stable time	

	/* reverse preloader's sin @mt6577_usbphy.c */
	USB11PHY_CLR8(U1PHTCR2+3, force_usb11_avalid | force_usb11_bvalid | force_usb11_sessend | force_usb11_vbusvalid);
	USB11PHY_CLR8(U1PHTCR2+2, RG_USB11_AVALID | RG_USB11_BVALID | RG_USB11_SESSEND | RG_USB11_VBUSVALID);
	USB11PHY_CLR8(U1PHYCR1+2, force_usb11_en_fs_ls_rcv | force_usb11_en_fs_ls_tx);
	/**************************************/
     
	USB11PHY_SET8(U1PHYCR0+1, RG_USB11_FSLS_ENBGRI);
	
	USB11PHY_SET8(U1PHTCR2+3, force_usb11_avalid | force_usb11_sessend | force_usb11_vbusvalid);
	USB11PHY_SET8(U1PHTCR2+2, RG_USB11_AVALID | RG_USB11_VBUSVALID);
	USB11PHY_CLR8(U1PHTCR2+2, RG_USB11_SESSEND);
    
    udelay(100);
}

void mt65xx_usb11_phy_savecurrent(void)
{
    unsigned char reg_value = 0;
    INFO("mt65xx_usb11_phy_savecurrent++\r\n");

	USB11PHY_SET8(U1PHTCR2+3, force_usb11_avalid | force_usb11_sessend | force_usb11_vbusvalid);
	USB11PHY_CLR8(U1PHTCR2+2, RG_USB11_AVALID | RG_USB11_VBUSVALID);
	USB11PHY_SET8(U1PHTCR2+2, RG_USB11_SESSEND);

    USB11PHY_CLR8(U1PHYCR0+1, RG_USB11_FSLS_ENBGRI);
	
	USB11PHY_SET8(U1PHYCR1+2, force_usb11_en_fs_ls_rcv | force_usb11_en_fs_ls_tx);
	USB11PHY_CLR8(U1PHYCR1+3, RG_USB11_EN_FS_LS_RCV | RG_USB11_EN_FS_LS_TX);
	
    disable_pll(MT65XX_UPLL, "USB11");
}

void mt65xx_usb11_phy_recover(void)
{
    unsigned char reg_value = 0;
    INFO("mt65xx_usb11_phy_recover++\r\n");
	
    enable_pll(MT65XX_UPLL, "USB11");

	USB11PHY_SET8(U1PHTCR2+3, force_usb11_avalid | force_usb11_sessend | force_usb11_vbusvalid);
	USB11PHY_SET8(U1PHTCR2+2, RG_USB11_AVALID | RG_USB11_VBUSVALID);
	USB11PHY_CLR8(U1PHTCR2+2, RG_USB11_SESSEND);
	
	USB11PHY_CLR8(U1PHYCR1+2, force_usb11_en_fs_ls_rcv | force_usb11_en_fs_ls_tx);
	USB11PHY_CLR8(U1PHYCR1+3, RG_USB11_EN_FS_LS_RCV | RG_USB11_EN_FS_LS_TX);
	
    USB11PHY_SET8(U1PHYCR0+1, RG_USB11_FSLS_ENBGRI);

    udelay(100);
}

static bool clock_enabled = false;
	
void mt65xx_usb11_clock_enable(bool enable)
{
    INFO("mt65xx_usb11_clock_enable++\r\n");
    if(enable){
        if(clock_enabled)//already enable
            return;
        else{
            enable_clock (MT65XX_PDN_PERI_USB2, "USB11");
            clock_enabled = true;
            }
        }
    else{
        if(!clock_enabled)//already disabled.
            return;
        else{
			disable_clock (MT65XX_PDN_PERI_USB2, "USB11");
            clock_enabled = false;
            }
        }
    return;
}

int mt65xx_usb11_poweron(int on){
    static bool recover = false;
    static bool oned = false;
    INFO("mt65xx_usb11_poweron++\r\n");
    if(on){
        if(oned) {
            return; //already powered on
        } else{
            mt65xx_usb11_clock_enable (true);	            	
            if(!recover){
                mt65xx_usb11_phy_poweron();
                recover = true;
            } else {
                mt65xx_usb11_phy_recover();
            }
            oned = true;
        }
    } else{
        if(!oned) {
            return; //already power off
        } else{
            mt65xx_usb11_phy_savecurrent();
            mt65xx_usb11_clock_enable(false);
            oned = false;
        }
    }
    return;
}

void mt65xx_usb11_vbus(struct musbfsh *musbfsh, int is_on)
{
    static int oned = 0;
    INFO("mt65xx_usb11_vbus++,is_on=%d\r\n",is_on);
#if 0
    mt_set_gpio_mode(GPIO67,0);//should set GPIO_OTG_DRVVBUS_PIN as gpio mode. 
    mt_set_gpio_dir(GPIO67,GPIO_DIR_OUT);
    if(is_on){
        if(oned)
            return;
        else{
            mt_set_gpio_out(GPIO67,GPIO_OUT_ONE);
            oned = 1;
            }
        }
    else{
        if(!oned)
            return;
        else{
            mt_set_gpio_out(GPIO67,GPIO_OUT_ZERO);
            oned = 0;
            }
        }
#endif
    return;
}

int musbfsh_platform_init(struct musbfsh *musbfsh)
{
    INFO("musbfsh_platform_init++\n");
    if(!musbfsh){
        ERR("musbfsh_platform_init,error,musbfsh is NULL");
        return -1;
    }
    musbfsh->board_set_vbus = mt65xx_usb11_vbus;
    musbfsh->board_set_power = mt65xx_usb11_poweron;
    hwPowerOn(MT65XX_POWER_LDO_VUSB, VOL_3300, "USB11"); // don't need to power on PHY for every resume
    mt65xx_usb11_poweron(true);
    return 0;
}

int musbfsh_platform_exit(struct musbfsh *musbfsh)
{
    INFO("musbfsh_platform_exit++\r\n");
	mt65xx_usb11_poweron(false);
	hwPowerDown(MT65XX_POWER_LDO_VUSB, "USB11"); // put it here because we can't shutdown PHY power during suspend
	return 0;
}

void musbfsh_platform_enable(struct musbfsh *musbfsh)
{
    INFO("musbfsh_platform_enable++\r\n");
}

void musbfsh_platform_disable(struct musbfsh *musbfsh)
{
    INFO("musbfsh_platform_disable++\r\n");
}
    
void musbfsh_hcd_release (struct device *dev) 
{
    INFO("musbfsh_hcd_release++,dev = 0x%08X.\n", (uint32_t)dev);
}  
//-------------------------------------------------------------------------




