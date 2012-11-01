LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := jni_rftg
LOCAL_SRC_FILES := \
	engine.c \
	init.c \
	android.c \
	common-gui.c \
	ai.c \
	loadsave.c \
	net.c

LOCAL_CFLAGS := -DANDROID

LOCAL_LDLIBS :=	-llog

include $(BUILD_SHARED_LIBRARY)
