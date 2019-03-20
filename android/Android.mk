
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SHARED_LIBRARIES := libcjson liblog libnativehelper

LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := jni.c
#LOCAL_CFLAGS += -DLOG

LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)
LOCAL_MODULE := libcjson-jni
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := cjson
LOCAL_SRC_FILES := $(call all-java-files-under,src)
include $(BUILD_STATIC_JAVA_LIBRARY)
