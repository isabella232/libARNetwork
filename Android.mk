LOCAL_PATH := $(call my-dir)

# JNI Wrapper
include $(CLEAR_VARS)

LOCAL_CFLAGS := -g
ifeq ($(TARGET_ARCH_ABI), armeabi-v7a)
    LOCAL_CFLAGS += -mfloat-abi=softfp -mfpu=neon
endif
LOCAL_MODULE := libarnetwork_android
LOCAL_SRC_FILES := JNI/c/ARNETWORK_JNIIOBufferParam.c JNI/c/ARNETWORK_JNIManager.c
LOCAL_LDLIBS := -llog -lz
LOCAL_SHARED_LIBRARIES := libARNetwork-prebuilt libARSAL-prebuilt
include $(BUILD_SHARED_LIBRARY)
