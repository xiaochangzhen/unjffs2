
PLATFORM=x86 hi3516cv300 hi3518ev200 hi3516a hi3518 hi3520d hi3535 hi3521a hi3531a hi3536 hi3798 hi3798c
PLATFORM_AFTER_CLEAN=$(patsubst %,%_after_clean,$(PLATFORM))
PLATFORM_CLEAN=$(patsubst %,%_clean,$(PLATFORM))
TARGET_DIR=./unjffs2

x86:
all: $(PLATFORM_AFTER_CLEAN)

.PHONY: all help $(PLATFORM_CLEAN) clean

help: 
	echo "make all";
	echo "make [platform](such as [$(PLATFORM)])";
	echo "make help";
	echo "make clean";

$(PLATFORM_AFTER_CLEAN):%_after_clean:%_clean %

$(PLATFORM_CLEAN):
	cd $(TARGET_DIR);make clean
clean:
	cd $(TARGET_DIR);make clean
x86:
	cd $(TARGET_DIR);make PLATFORM=x86

hi3516cv300:
	cd $(TARGET_DIR);make PLATFORM=$@ CROSS=arm-hisiv500-linux-
hi3518ev200:
	cd $(TARGET_DIR);make PLATFORM=$@ CROSS=arm-hisiv300-linux-
hi3516a:
	cd $(TARGET_DIR);make PLATFORM=$@ CROSS=arm-hisiv300-linux- \
	    PLATFORM_FLAGS="-mcpu=cortex-a7 -mfloat-abi=softfp -mfpu=neon-vfpv4 -mno-unaligned-access -fno-aggressive-loop-optimizations"
hi3518:
	cd $(TARGET_DIR);make PLATFORM=$@ CROSS=arm-hisiv100nptl-linux-
hi3520d:
	cd $(TARGET_DIR);make PLATFORM=$@ CROSS=arm-hisiv100nptl-linux- \
	    PLATFORM_FLAGS="-march=armv7-a -mcpu=cortex-a9 -static"
hi3535:
	cd $(TARGET_DIR);make PLATFORM=$@ CROSS=arm-hisiv100nptl-linux- \
	    PLATFORM_FLAGS="-march=armv7-a -mcpu=cortex-a9 -static"
hi3521a:
	cd $(TARGET_DIR);make PLATFORM=$@ CROSS=arm-hisiv300-linux- \
	    PLATFORM_FLAGS="-mcpu=cortex-a7 -mfloat-abi=softfp -mfpu=neon-vfpv4 -mno-unaligned-access -fno-aggressive-loop-optimizations -static"
hi3531a:
	cd $(TARGET_DIR);make PLATFORM=$@ CROSS=arm-hisiv300-linux- \
	    PLATFORM_FLAGS="-mcpu=cortex-a9 -mfloat-abi=softfp -mfpu=neon -mno-unaligned-access -fno-aggressive-loop-optimizations -static"
hi3536:
	cd $(TARGET_DIR);make PLATFORM=$@ CROSS=arm-hisiv300-linux- \
	    PLATFORM_FLAGS="-mcpu=cortex-a7 -mfloat-abi=softfp -mfpu=neon-vfpv4 -ffunction-sections -mno-unaligned-access -fno-aggressive-loop-optimizations -static"
hi3798:
	cd $(TARGET_DIR);make PLATFORM=$@ CROSS=arm-hisiv200-linux-
	    PLATFORM_FLAGS="-static"
hi3798c:
	cd $(TARGET_DIR);make PLATFORM=$@ CROSS=arm-histbv310-linux-
	    PLATFORM_FLAGS="-static"
