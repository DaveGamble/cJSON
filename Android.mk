# copyright 2015 Huami Inc.  All rights reserved.
#

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

#LOCAL_PRELINK_MODULE := false
#LOCAL_SHARED_LIBRARIES := liblog libcutils libnativehelper
# CJSON_OBJ = cJSON.o
# UTILS_OBJ = cJSON_Utils.o
CJSON_LIBNAME = libcjson
UTILS_LIBNAME = libcjson_utils
# CJSON_TEST = cJSON_test

# CJSON_TEST_SRC = cJSON.c test.c

# LDLIBS = -lm

LIBVERSION = 1.7.7
CJSON_SOVERSION = 1
UTILS_SOVERSION = 1

# CJSON_SO_LDFLAG=-Wl,-soname=$(CJSON_LIBNAME).so.$(CJSON_SOVERSION)
# UTILS_SO_LDFLAG=-Wl,-soname=$(UTILS_LIBNAME).so.$(UTILS_SOVERSION)

# PREFIX ?= /usr/local
# INCLUDE_PATH ?= include/cjson
# LIBRARY_PATH ?= lib

# INSTALL_INCLUDE_PATH = $(DESTDIR)$(PREFIX)/$(INCLUDE_PATH)
# INSTALL_LIBRARY_PATH = $(DESTDIR)$(PREFIX)/$(LIBRARY_PATH)

# INSTALL ?= cp -a

# validate gcc version for use fstack-protector-strong
MIN_GCC_VERSION = "4.9"
GCC_VERSION := "`$(CC) -dumpversion`"
IS_GCC_ABOVE_MIN_VERSION := $(shell expr "$(GCC_VERSION)" ">=" "$(MIN_GCC_VERSION)")
ifeq "$(IS_GCC_ABOVE_MIN_VERSION)" "1"
    CFLAGS += -fstack-protector-strong
else
    CFLAGS += -fstack-protector
endif

# $(warning IS_GCC_ABOVE_MIN_VERSION $(CFLAGS))
R_CFLAGS = -fPIC -std=c89 -pedantic -Wall -Werror -Wstrict-prototypes -Wwrite-strings -Wshadow -Winit-self -Wcast-align -Wformat=2 -Wmissing-prototypes -Wstrict-overflow=2 -Wcast-qual -Wc++-compat -Wundef -Wswitch-default -Wconversion $(CFLAGS)


LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := cJSON.c
LOCAL_CFLAGS += $(R_CFLAGS)

LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)
LOCAL_MODULE := $(CJSON_LIBNAME)
include $(BUILD_SHARED_LIBRARY)

include $(call all-makefiles-under,$(LOCAL_PATH))
