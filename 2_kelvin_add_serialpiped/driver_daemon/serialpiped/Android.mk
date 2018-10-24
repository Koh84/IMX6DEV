LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := serialpiped.c comm.c ini.c

LOCAL_SHARED_LIBRARIES := liblog
LOCAL_STATIC_LIBRARIES := libz
LOCAL_CFLAGS := -Werror

LOCAL_MODULE := serialpiped

include $(BUILD_EXECUTABLE)


