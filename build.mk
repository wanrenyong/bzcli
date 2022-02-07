ifndef LOCAL_BUILD_DIR
ifdef BUILD_DIR
LOCAL_BUILD_DIR := $(BUILD_DIR)
else
LOCAL_BUILD_DIR ?= $(shell pwd)/build
endif
endif



LOCAL_BUILD_OBJ_DIR ?= $(LOCAL_BUILD_DIR)/obj
LOCAL_INSTALL_LIB_DIR ?= $(LOCAL_BUILD_DIR)/lib
LOCAL_INSTALL_BIN_DIR ?= $(LOCAL_BUILD_DIR)/bin


LOCAL_SRC_FILES ?= $(wildcard *.c)
LOCAL_SRC_FILES += $(LOCAL_EXTRA_SRC_FILES)
LOCAL_OBJ_FILES := $(addprefix $(LOCAL_BUILD_OBJ_DIR)/, $(patsubst %.c, %.o, $(LOCAL_SRC_FILES)))
LOCAL_DFILES += $(LOCAL_OBJ_FILES:.o=.d)


CC ?= gcc
AR ?= ar

ifdef V
ifeq ("$(origin V), "command line)
DO_VERBOSE = 1
endif
endif

ifndef DO_VERBOSE
Q = @
export Q
endif

LOCAL_LD_LIBS += $(LOCAL_EXTRA_LIBS)
LOCAL_LD_FLAGS += $(LOCAL_EXTRA_LD_FLAGS)
LOCAL_C_FLAGS += $(LOCAL_EXTRA_C_FLAGS)



ifdef TARGET_APP
REAL_TARGET := $(LOCAL_INSTALL_BIN_DIR)/$(TARGET_APP)
else
ifdef TARGET_LIB
REAL_TARGET := $(LOCAL_INSTALL_LIB_DIR)/$(TARGET_LIB)
else
$(error TARGET_APP or TARGET_APP is not defined!)_
endif
endif

all: $(REAL_TARGET)


ifdef TARGET_APP
$(REAL_TARGET): $(LOCAL_OBJ_FILES)
ifdef Q
	@echo LD $(notdir $@)
endif
	@ mkdir -p $(dir $@)
	$(Q) $(CC) -o $@ $^ $(LOCAL_LD_LIBS) $(LOCAL_LD_FLAGS)
else
$(REAL_TARGET): $(LOCAL_OBJ_FILES)
ifdef Q
	@echo AR $(notdir $@)
endif
	@ mkdir -p $(dir $@)
	$(Q) $(AR) -rcs $@ $^ > /dev/null 2>&1	
endif

clean:
ifndef BUILD_DIR
	@rm -rf $(LOCAL_BUILD_DIR)
else
	@ rm -rf $(REAL_TARGET) $(LOCAL_OBJ_FILES) $(LOCAL_DFILES)
endif


-include $(LOCAL_DFILES)

$(LOCAL_OBJ_FILES): $(LOCAL_BUILD_OBJ_DIR)/%.o : %.c
ifdef Q
	@echo CC $(notdir $@)
endif
	@ mkdir -p $(dir $@)
	$(Q) $(CC) -o $@ -MMD -c $< $(LOCAL_C_FLAGS) 
	

