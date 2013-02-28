#######################################
# Sepcify source files
#######################################

lib_obj := src/mtk_drvb_mod.o
lib_obj += prebuilt/sec_drvb.o

obj-m += mtk_drvb_77.o

# HEADER FILES
EXTRA_CFLAGS += -I$(src) -I$(src)/inc

# INCLUDE DRVB LIBRARY	
mtk_drvb_77-y += $(lib_obj)	