include $(MTK_ROOT_BUILD)/Makefile
CCCI_CORE_DRIVER_DIR :=  $(wildcard $(obj)/$(call to-root,$(obj))mediatek/source/kernel/drivers/ccci)
ifeq ($(CCCI_CORE_DRIVER_DIR),)
$(error Invalid ccci source path, please check)
endif


obj-$(CONFIG_MTK_CCCI_DRIVER) += ccci_plat.o
ccci_plat-y := src/ccci_platform.module

ifeq ($(MTK_EMMC_SUPPORT),yes)
	ext-libs := $(MTK_PATH_PLATFORM)/drivers/masp/prebuilt/libasp_EMMC.a
else 
	ext-libs := $(MTK_PATH_PLATFORM)/drivers/masp/prebuilt/libasp_NAND.a
endif

ifeq ($(CUSTOM_SEC_AUTH_SUPPORT),yes)
CCCI_CUSTOM_DRIVER_DIR := $(call to-root,$(obj))mediatek/custom/out/$(call lc,$(MTK_PROJECT))/kernel/ccci
ccci_plat-y += $(CCCI_CUSTOM_DRIVER_DIR)/cust_auth.o
else
ext-libs += $(MTK_PATH_PLATFORM)/drivers/masp/prebuilt/libasp_auth.a
ccci_plat-y += auth/sec_wrapper.module
endif

ldflags-y += --whole-archive $(ext-libs)  --no-whole-archive

ccflags-y  += -I$(CCCI_CORE_DRIVER_DIR)/include -I$(obj)/include

clean-files += $(patsubst $(obj)/%,%,$(wildcard *.o)\
	 $(wildcard .*cmd) $(wildcard *~))

clean-files += $(patsubst $(obj)/%,%,$(wildcard src/*.o)\
	 $(wildcard src/.*cmd) $(wildcard src/*~))

ifeq ($(CUSTOM_SEC_AUTH_SUPPORT),yes)
clean-files += $(CCCI_CUSTOM_DRIVER_DIR)/cust_auth.o
else
clean-files += $(patsubst $(obj)/%,%,$(wildcard auth/*.o))
endif

#obj-  := dummy.o
#my-dir := $(dir $(lastword $(MAKEFILE_LIST)))
#
##CCCI_PLATFORM_DRIVER_FILES := $(filter-out %.h,$(wildcard $(CCCI_PLATFORM_DRIVER_DIR)/*.c)\
##			$(wildcard $(CCCI_PLATFORM_DRIVER_DIR)/prebuilt/lib*.o))
#
#my-files := $(patsubst $(obj)/%,%,$(wildcard $(my-dir)/*.c)  $(wildcard $(my-dir)/prebuilt/*.o))
#ifeq ($(MTK_EMMC_SUPPORT),yes)
#	my-libs := $(wildcard $(my-dir)prebuilt/libasp_EMMC.a)
#else 
#	my-libs := $(wildcard $(my-dir)prebuilt/libasp_NAND.a)
#endif
##my-libs := 
##$(info  lib===$(my-libs))
#ifneq ($(my-libs),)
# 
#ldflags-y += -L$(my-dir) --whole-archive $(my-libs)  --no-whole-archive
##--start-group --end-group
#endif
#ccflags-y  +=-I$(my-dir)/include
#ccci-y +=$(my-files:.c=.o)
#
#clean-files += $(patsubst $(obj)/%,%,$(wildcard $(my-dir)/*.o)\
#	 $(wildcard $(my-dir)/.*cmd) $(wildcard $(my-dir)/*~))
