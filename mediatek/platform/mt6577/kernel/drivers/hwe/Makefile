#######################################
# Sepcify source files
#######################################

lib_obj := src/mtk_hwe_init_mod.o
lib_obj += prebuilt/mtk_hwe_init.o

obj-m += mtk_hwe.o

# HEADER FILES
EXTRA_CFLAGS += -I$(src)

# INCLUDE DRVB LIBRARY	
mtk_hwe-y += $(lib_obj)	