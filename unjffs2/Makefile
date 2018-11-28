############################################################################
#
# Makefile for unjffs2
#
# (C) Copyright 2008 to 2020 Hangzhou Xiongmai Technology Stock CO., LTD.
#
# Fri Aug 12 16:07:21 CST 2016	xcz		File created
#
############################################################################

CROSS	=
PLATFORM =
PLATFORM_FLAGS = 

ifeq (x$(PLATFORM)y,xy)
PLATFORM=x86
endif

CC	= @echo " GCC	$@"; $(CROSS)gcc
CPP	= @echo " G++	$@"; $(CROSS)g++
LD	= @echo " LD	$@"; $(CROSS)ld
AR	= @echo " AR	$@"; $(CROSS)ar
STRIP	= @echo " STRIP $@"; $(CROSS)strip

CFLAGS = -std=gnu99

CFLAGS += -Iinclude 
#CFLAGS += -DCONFIG_JFFS2_ZLIB -DCONFIG_JFFS2_RTIME -DCONFIG_JFFS2_LZO
TARGET = ./bin/$(PLATFORM)/unjffs2
LIBS  = ./lib/liblzo2.a ./lib/libz.a

DIR_ZLIB = ../zlib-1.2.11
DIR_LZO = ../lzo

SOURCE = $(wildcard src/*.c)

OBJS = $(patsubst src/%.c,src/%.o,$(SOURCE))

$(TARGET): $(OBJS) $(LIBS)
	mkdir -p $(dir $@)
	$(CC) $(PLATFORM_FLAGS) $^ -o $@ $(LIBS)
	$(STRIP) $@

.c.o:
	$(CC)  $(CFLAGS) $(PLATFORM_FLAGS) -c -o $@ $<
	@echo $@
.cpp.o:
	$(CPP) $(CFLAGS) $(PLATFORM_FLAGS) -c -o $@ $<

./lib/liblzo2.a:
	@echo "PLATFORM $(PLATFORM) CORSS $(CROSS)"
	@cd $(DIR_LZO)/build/;make $(PLATFORM)
	@cp $(DIR_LZO)/_install/$(PLATFORM)/lib/liblzo2.a $@ -f

./lib/libz.a:
	@cd $(DIR_ZLIB);make CROSS="$(CROSS)" PLATFORM_FLAGS="$(PLATFORM_FLAGS)"
	@cp $(DIR_ZLIB)/libz.a $@ -f

clean:
	rm -f $(TARGET) $(OBJS) $(LIBS)
	@cd $(DIR_LZO)/build/;make clean
	@cd $(DIR_ZLIB);make clean
	
