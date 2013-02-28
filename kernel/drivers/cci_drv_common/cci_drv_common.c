//---start add by Rachel

#include <mach/mt6577_gpio.h>
#include <linux/cci_drv_common.h>

#include <linux/init.h>
#include <asm/string.h>
#include <linux/module.h>	/* Specifically, a module */
#include <linux/kernel.h>	/* We're doing kernel work */
#include <linux/proc_fs.h>	/* Necessary because we use the proc fs */

char *CCI_PLATFORM_BOOTARG;

static int prcb_version_proc_read(char *page, char **start, off_t off,
				  int count, int *eof, void *data)
{
       char *p = page;
	int len = 0;

       p += sprintf(p, "%s", CCI_PLATFORM_BOOTARG);
        
	*start = page + off;
	len = p- page;
	if(len > off)
		len -= off;
	else
		len = 0;

	return len < count ? len : count;
}

void setup_cci_project(char *str)
{
	if(strstr(str,"EVT")) {
	       CCI_PLATFORM_BOOTARG = "EVT";
	}
	if(strstr(str,"DVT1")) {
	       CCI_PLATFORM_BOOTARG = "DVT1";
	}
	if(strstr(str,"DVT2")) {
	       CCI_PLATFORM_BOOTARG = "DVT2";
	}
	if(strstr(str,"PVT")) {
	       CCI_PLATFORM_BOOTARG = "PVT";
	}
	if(strstr(str,"MP-B")) {
	       CCI_PLATFORM_BOOTARG = "MP-B";
	}
	else if(strstr(str,"MP-W")) {
	       CCI_PLATFORM_BOOTARG = "MP-W";
	}	
	else if(strstr(str,"MP")) {
	       CCI_PLATFORM_BOOTARG = "MP";
	}
}


static int __init build_proc_project(void) 
{
        /* create proc entry at /proc/cust_project */
        create_proc_read_entry("cust_project", 0644,  NULL,prcb_version_proc_read, NULL);
        
	 return 0;
}
module_init(build_proc_project);

//---end add by Rachel

