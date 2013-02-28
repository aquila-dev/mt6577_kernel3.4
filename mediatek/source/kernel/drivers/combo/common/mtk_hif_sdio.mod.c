#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
 .name = KBUILD_MODNAME,
 .init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
 .exit = cleanup_module,
#endif
 .arch = MODULE_ARCH_INIT,
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

MODULE_ALIAS("sdio:c*v037Ad018A*");
MODULE_ALIAS("sdio:c*v037Ad018B*");
MODULE_ALIAS("sdio:c*v037Ad018C*");
MODULE_ALIAS("sdio:c*v037Ad6619*");
MODULE_ALIAS("sdio:c*v037Ad020A*");
MODULE_ALIAS("sdio:c*v037Ad020B*");
MODULE_ALIAS("sdio:c*v037Ad020C*");
MODULE_ALIAS("sdio:c*v037Ad5921*");
MODULE_ALIAS("sdio:c*v037Ad6628*");
