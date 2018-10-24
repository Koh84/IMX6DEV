LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := i2c_bar_test.c 

LOCAL_SHARED_LIBRARIES := liblog
LOCAL_STATIC_LIBRARIES := libz
LOCAL_CFLAGS := -Werror

LOCAL_MODULE := i2c_bar_test

include $(BUILD_EXECUTABLE)


