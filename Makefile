ifndef TARGET_LIB
TARGET_APP ?= bzcli
endif

TOP_DIR = $(shell pwd)

ifdef CLI_GEN_INSTALL_DIR
CLIGEN_LIB_DIR := $(CLI_GEN_INSTALL_DIR)/lib
CLIGEN_INC_DIR := $(CLI_GEN_INSTALL_DIR)/include
else
CLIGEN_LIB_DIR ?= /home/wanrenyong/Work/opensource/cligen-5.4.0/build/lib
CLIGEN_INC_DIR ?= /home/wanrenyong/Work/opensource/cligen-5.4.0/build/include
endif

EXTERNAL_LIB += cligen
EXTERNAL_LIB_DIR += $(CLIGEN_LIB_DIR)
EXTERNAL_INC_DIR += $(CLIGEN_INC_DIR)
#Extra flags
LOCAL_C_FLAGS += $(addprefix -I, $(EXTERNAL_INC_DIR))
LOCAL_LD_FLAGS += $(addprefix -L, $(EXTERNAL_LIB_DIR)) $(addprefix -l, $(EXTERNAL_LIB))
LOCAL_LD_LIBS += $(EXTRA_LD_LIBS)

#default is $PWD/build
ifdef BUILD_DIR
LOCAL_BUILD_DIR := $(BUILD_DIR)
endif

#default is $LOCAL_BUILD_DIR/obj
ifdef BUILD_OBJ_DIR
LOCAL_BUILD_OBJ_DIR := $(BUILD_OBJ_DIR)
endif

#default is $LOCAL_BUILD_DIR/lib
ifdef INSTALL_LIB_DIR
LOCAL_INSTALL_LIB_DIR := $(INSTALL_LIB_DIR)
endif

#default is $LOCAL_BUILD_DIR/bin
ifdef INSTALL_BIN_DIR
LOCAL_INSTALL_BIN_DIR := $(INSTALL_BIN_DIR)
endif

ARG ?= -f test.cli


include build.mk

help:
	@echo "Usage: make [Options]"
	@echo "  Options:"
	@echo "     run [ARG=arg]             - Build and run bzcli"
	@echo "     CLI_GEN_INSTALL_DIR=<dir> - cligen install path which contains lib and include directory"
	@echo "     CLIGEN_LIB_DIR=<dir>      - cligen lib path"
	@echo "     CLIGEN_INC_DIR=<dir>      - cligen include path"
	@echo "     clean                     - Clean BUILD_DIR"


run:all
	$(Q) LD_LIBRARY_PATH="$(EXTERNAL_LIB_DIR)"  $(LOCAL_INSTALL_BIN_DIR)/$(TARGET_APP) $(ARG)

