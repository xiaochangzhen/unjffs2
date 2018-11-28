############################################################################
#
# Makefile for OpenSrc
#
# (C) Copyright 2008 to 2017 Hangzhou Xiongmai Technology Stock CO., LTD.
#
#  Mar 10 15:22:50 CST 2017		xcz		File created
#
############################################################################


PWD=$(shell pwd)

PLATFORM=x86 hi3516cv300 hi3518ev200 hi3516a hi3518 hi3520d hi3535 hi3521a hi3531a hi3536 hi3798 hi3798c

TARGET_DIR=$(PWD)/../lzo-2.10
INSTALL_DIR=$(PWD)/../_install

PLATFORM_CONFIG=$(patsubst %,%_config,$(PLATFORM_CONFIG))

define fun_domake
	@if [ -f $(TARGET_DIR)/Makefile ];then make -C $(TARGET_DIR); make -C $(TARGET_DIR) install; fi
endef
define fun_clean
	@if [ -f $(TARGET_DIR)/Makefile ];then make -C $(TARGET_DIR) clean; fi
endef

.PHONY: all clean $(PLATFORM) help

all: $(PLATFORM)
	@echo $(PLATFORM)

$(PLATFORM):%:%_config 
	$(call fun_domake)

help:
	@echo "make help"
	@echo "make all"
	@echo "make [platform](such as [$(PLATFORM)])"
	@echo "make clean"

clean:
	$(call fun_clean)
	@mkdir -p $(INSTALL_DIR)/;cd $(INSTALL_DIR)/;rm -rf *
	@find $(TARGET_DIR)/ -name "Makefile" | xargs rm -f

x86_config:
	$(call fun_clean)
	@cd $(TARGET_DIR)/;./configure --prefix=$(INSTALL_DIR)/$(@:_config=)

hi3516cv300_config:
	$(call fun_clean)
	@cd $(TARGET_DIR)/;./configure --prefix=$(INSTALL_DIR)/$(@:_config=) --host=arm-hisiv500-linux

hi3518ev200_config:
	$(call fun_clean)
	@cd $(TARGET_DIR)/;./configure --prefix=$(INSTALL_DIR)/$(@:_config=) --host=arm-hisiv300-linux

hi3516a_config:
	$(call fun_clean)
	@cd $(TARGET_DIR)/;./configure --prefix=$(INSTALL_DIR)/$(@:_config=) --host=arm-hisiv300-linux \
	CFLAGS="-mcpu=cortex-a7 -mfloat-abi=softfp -mfpu=neon-vfpv4 -mno-unaligned-access -fno-aggressive-loop-optimizations" \
	CPPFLAGS="-mcpu=cortex-a7 -mfloat-abi=softfp -mfpu=neon-vfpv4 -mno-unaligned-access -fno-aggressive-loop-optimizations"

hi3518_config:
	$(call fun_clean)
	@cd $(TARGET_DIR)/;./configure --prefix=$(INSTALL_DIR)/$(@:_config=) --host=arm-hisiv100nptl-linux

hi3520d_config:
	$(call fun_clean)
	@cd $(TARGET_DIR)/;./configure --prefix=$(INSTALL_DIR)/$(@:_config=) --host=arm-hisiv100nptl-linux \
	CFLAGS="-march=armv7-a -mcpu=cortex-a9 -static" \
	CPPFLAGS="-march=armv7-a -mcpu=cortex-a9 -static"

hi3535_config:
	$(call fun_clean)
	@cd $(TARGET_DIR)/;./configure --prefix=$(INSTALL_DIR)/$(@:_config=) --host=arm-hisiv100nptl-linux \
	CFLAGS="-march=armv7-a -mcpu=cortex-a9 -static" \
	CPPFLAGS="-march=armv7-a -mcpu=cortex-a9 -static"

hi3521a_config:
	$(call fun_clean)
	@cd $(TARGET_DIR)/;./configure --prefix=$(INSTALL_DIR)/$(@:_config=) --host=arm-hisiv300-linux \
	CFLAGS="-mcpu=cortex-a7 -mfloat-abi=softfp -mfpu=neon-vfpv4 -mno-unaligned-access -fno-aggressive-loop-optimizations -static" \
	CPPFLAGS="-mcpu=cortex-a7 -mfloat-abi=softfp -mfpu=neon-vfpv4 -mno-unaligned-access -fno-aggressive-loop-optimizations -static"

hi3531a_config:
	$(call fun_clean)
	@cd $(TARGET_DIR)/;./configure --prefix=$(INSTALL_DIR)/$(@:_config=) --host=arm-hisiv300-linux \
	CFLAGS="-mcpu=cortex-a9 -mfloat-abi=softfp -mfpu=neon -mno-unaligned-access -fno-aggressive-loop-optimizations -static" \
	CPPFLAGS="-mcpu=cortex-a9 -mfloat-abi=softfp -mfpu=neon -mno-unaligned-access -fno-aggressive-loop-optimizations -static"

hi3536_config:
	$(call fun_clean)
	@cd $(TARGET_DIR)/;./configure --prefix=$(INSTALL_DIR)/$(@:_config=) --host=arm-hisiv300-linux \
	CFLAGS="-mcpu=cortex-a7 -mfloat-abi=softfp -mfpu=neon-vfpv4 -ffunction-sections -mno-unaligned-access -fno-aggressive-loop-optimizations -static" \
	CPPFLAGS="-mcpu=cortex-a7 -mfloat-abi=softfp -mfpu=neon-vfpv4 -ffunction-sections -mno-unaligned-access -fno-aggressive-loop-optimizations -static"

hi3798_config:
	$(call fun_clean)
	@cd $(TARGET_DIR)/;./configure --prefix=$(INSTALL_DIR)/$(@:_config=) --host=arm-hisiv200-linux \
	CFLAGS="-static" \
	CPPFLAGS="-static"

hi3798c_config:
	$(call fun_clean)
	@cd $(TARGET_DIR)/;./configure --prefix=$(INSTALL_DIR)/$(@:_config=) --host=arm-histbv310-linux \
	CFLAGS="-static" \
	CPPFLAGS="-static"

