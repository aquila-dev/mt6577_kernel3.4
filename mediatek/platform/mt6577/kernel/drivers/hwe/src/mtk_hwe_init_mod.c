#include <linux/module.h>
#include <linux/delay.h>

/*****************************************************************************
 * MODULE DEFINITION
 *****************************************************************************/
#define MOD "HWE_MOD"

extern void mtk_hwe_init(int);

#ifdef CONFIG_SMP
static void (*hwe_init_ori)(int);
extern void (*hwe_init)(int);
#endif

extern int mtk_hwe_protect(int);

static int (*hwe_protect_ori)(int);
extern int (*hwe_protect)(int);

/*****************************************************************************
 * LOCAL VARIABLES
 *****************************************************************************/

/*****************************************************************************
 * PORTING LAYER
 *****************************************************************************/
void hwe_mb(void)
{
    mb();
}

void hwe_udelay(unsigned long delay)
{
    udelay(delay);
}

static void hwe_init_impl(int cpunum)
{
    mtk_hwe_init(cpunum);
}

static int hwe_protect_impl(int cpunum)
{
    return mtk_hwe_protect(cpunum);
}

/*****************************************************************************
 * MTK HWE Init
 *****************************************************************************/
int __init mtk_hwe_mod_init (void)
{
    printk("[%s] init :", MOD);

    #ifdef CONFIG_SMP
    hwe_init_ori = hwe_init;
    hwe_init = &hwe_init_impl;
    #else
    hwe_init_impl(1);
    #endif

    hwe_protect_ori = hwe_protect;
    hwe_protect = &hwe_protect_impl;

    printk("Done");

    return 0;
}

/*****************************************************************************
 * MTK HWE De-Init
 *****************************************************************************/
void __exit mtk_hwe_mod_exit (void)
{
    printk("[%s] exit : ", MOD);

    #ifdef CONFIG_SMP
    hwe_init = hwe_init_ori;
    #endif

    hwe_protect = hwe_protect_ori;

    printk("Done\n");
}

module_init (mtk_hwe_mod_init);
module_exit (mtk_hwe_mod_exit);
MODULE_AUTHOR ("Chun-Wei.Chen <chun-wei.chen@mediatek.com>");
MODULE_DESCRIPTION ("MEDIATEK HWE Init Driver");
MODULE_LICENSE ("Proprietary");