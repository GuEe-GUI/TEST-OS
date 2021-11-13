LOAD_SECTOR_OFFSET   = 1
LOAD_SECTORS         = 8

KERNEL_SECTOR_OFFEST = 9
KERNEL_SECTORS       = 348

CROSS_COMPILE := x86_64-elf-

AS = nasm
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld

BUILD_DIR = build

BOOT_BIN    = $(BUILD_DIR)/boot.bin
LOADER_BIN  = $(BUILD_DIR)/loader.bin
KERNEL_FILE = $(BUILD_DIR)/kernel.elf
TEST_OS_IMG = TEST_OS.img

MODE := __DEBUG__

ASM_KERNEL_FLAGS = -f elf32
C_KERNEL_FLAGS = -Iinclude -Ikernel -m32 -D$(MODE) -Wall -nostdlib -fno-builtin -fno-leading-underscore
CPP_KERNEL_FLAGS = -Wno-unused-command-line-argument -ffreestanding -fno-cxx-exceptions -fno-exceptions -fno-rtti -fno-unwind-tables -ibuiltininc -nogpulib -nostdlib

LD_FLAGS = -m elf_i386 -e _start -Ttext 0x80100000

OBJS = \
	_Start.o\
	_IO.o\
	_Interrupt.o\
	_Thread.o\
	_Eval.o\
	start.o\
	vbe.o\
	interrupt.o\
	timer.o\
	rtc.o\
	string.o\
	console.o\
	assert.o\
	text.o\
	memory.o\
	printk.o\
	bitmap.o\
	keyboard.o\
	thread.o\
	eval.o\
	cmds.o

OBJS := $(addprefix $(BUILD_DIR)/,${OBJS})

all: $(shell mkdir -p $(BUILD_DIR)) $(TEST_OS_IMG) run

$(TEST_OS_IMG): $(BOOT_BIN) $(LOADER_BIN) $(KERNEL_FILE)
	qemu-img create $(TEST_OS_IMG) 1440K
	dd if=$(BOOT_BIN) of=$(TEST_OS_IMG) bs=512 count=1 conv=notrunc
	dd if=$(LOADER_BIN) of=$(TEST_OS_IMG) bs=512 seek=$(LOAD_SECTOR_OFFSET) count=$(LOAD_SECTORS) conv=notrunc
	dd if=$(KERNEL_FILE) of=$(TEST_OS_IMG) bs=512 seek=$(KERNEL_SECTOR_OFFEST) count=$(KERNEL_SECTORS) conv=notrunc

run: $(TEST_OS_IMG)
	qemu-system-i386 -name "TEST OS" -m 128 -rtc base=localtime -boot a -drive file=$(TEST_OS_IMG),format=raw,index=0,if=floppy

clean:
	mkdir -p $(BUILD_DIR)
	rm -f $(BUILD_DIR)/*
	rm -f *.img

$(BUILD_DIR)/%.bin: boot/%.asm
	$(AS) -o $@ $<

$(KERNEL_FILE): $(OBJS)
	$(LD) $(LD_FLAGS) -o $(KERNEL_FILE) $^

$(BUILD_DIR)/%.o: */%.asm
	$(AS) $(ASM_KERNEL_FLAGS) -o $@ $<

-include $(BUILD_DIR)/*.d
$(BUILD_DIR)/%.o: */%.c
	$(CC) -MMD $(C_KERNEL_FLAGS) -c $< -o $@
