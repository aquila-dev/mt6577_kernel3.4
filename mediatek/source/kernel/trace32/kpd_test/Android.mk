LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := kpd_autotest.c
LOCAL_MODULE := kpd_auto
include $(BUILD_EXECUTABLE)
