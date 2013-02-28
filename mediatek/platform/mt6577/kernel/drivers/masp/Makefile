#######################################
# Sepcify source files
#######################################

lib_obj := src/sec_mod.o auth/sec_wrapper.o
ifeq ($(MTK_EMMC_SUPPORT),yes)
	lib_obj += prebuilt/libasp_EMMC.a
else 
	lib_obj += prebuilt/libasp_NAND.a	
endif
lib_obj += prebuilt/libasp_auth.a

obj-m += sec.o

# HEADER FILES
EXTRA_CFLAGS += -I$(src) -I$(src)/inc

# INCLUDE ASP LIBRARY
sec-y += $(lib_obj)