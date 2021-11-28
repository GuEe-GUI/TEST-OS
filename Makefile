LOAD_SECTOR_OFFSET   = 1
LOAD_SECTORS         = 8

KERNEL_SECTOR_OFFEST = 9
KERNEL_SECTORS       = 348

KERNEL_LINKER_ADDR   = 0x80100000

CROSS_COMPILE := x86_64-elf-
RAM           := 128

AS = nasm
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld

BUILD_DIR = build

BOOT_BIN    = $(BUILD_DIR)/boot.bin
LOADER_BIN  = $(BUILD_DIR)/loader.bin
KERNEL_FILE = $(BUILD_DIR)/kernel.elf
TEST_OS_IMG = TEST-OS.img
FAT_FS_IMG  = FAT-FS.img

MODE := __DEBUG__

ASM_KERNEL_FLAGS = -f elf32
C_KERNEL_FLAGS = -O1 -Iinclude -Ikernel -m32 -D$(MODE) -Wall -nostdlib -fno-builtin -fno-leading-underscore
CPP_KERNEL_FLAGS = -Wno-unused-command-line-argument -ffreestanding -fno-cxx-exceptions -fno-exceptions -fno-rtti -fno-unwind-tables -ibuiltininc -nogpulib -nostdlib

LD_FLAGS = -m elf_i386 -e _start -Ttext $(KERNEL_LINKER_ADDR)

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
	console.o\
	assert.o\
	memory.o\
	printk.o\
	bitmap.o\
	keyboard.o\
	ide.o\
	disk.o\
	fat32.o\
	thread.o\
	eval.o\
	cmds.o\
	string.o\
	text.o

OBJS := $(addprefix $(BUILD_DIR)/,${OBJS})

all: $(shell mkdir -p $(BUILD_DIR)) $(TEST_OS_IMG) run

$(TEST_OS_IMG): $(BOOT_BIN) $(LOADER_BIN) $(KERNEL_FILE)
	@echo [QEMU] $@
	@qemu-img create $@ 1440K -q
	@echo [DD] $(BOOT_BIN)
	@dd if=$(BOOT_BIN) of=$@ bs=512 count=1 conv=notrunc
	@echo [DD] $(LOADER_BIN)
	@dd if=$(LOADER_BIN) of=$@ bs=512 seek=$(LOAD_SECTOR_OFFSET) count=$(LOAD_SECTORS) conv=notrunc
	@echo [DD] $(KERNEL_FILE)
	@dd if=$(KERNEL_FILE) of=$@ bs=512 seek=$(KERNEL_SECTOR_OFFEST) count=$(KERNEL_SECTORS) conv=notrunc

$(FAT_FS_IMG):
	@echo [QEMU] $@
	@qemu-img create $@ 64M -q

run: $(TEST_OS_IMG) $(FAT_FS_IMG)
	@echo [QEMU] RAM=$(RAM)MB $<
	@qemu-system-i386 \
		-name "TEST OS" \
		-m $(RAM) \
		-rtc base=localtime \
		-drive file=$(FAT_FS_IMG),format=raw,if=ide \
		-boot a -drive file=$(TEST_OS_IMG),format=raw,index=0,if=floppy

clean:
	@echo [RM] $(BUILD_DIR)
	@rm -rf $(BUILD_DIR)
	@echo [RM] $(TEST_OS_IMG)
	@rm -f $(TEST_OS_IMG)

$(BUILD_DIR)/%.bin: boot/%.asm
	@echo [AS] $<
	@$(AS) -o $@ $<

$(KERNEL_FILE): $(OBJS)
	@echo [LD] $(KERNEL_LINKER_ADDR) $@
	@$(LD) $(LD_FLAGS) -o $(KERNEL_FILE) $^

$(BUILD_DIR)/%.o: */%.asm
	@echo [AS] $<
	@$(AS) $(ASM_KERNEL_FLAGS) -o $@ $<

-include $(BUILD_DIR)/*.d
$(BUILD_DIR)/%.o: */%.c
	@echo [CC] $<
	@$(CC) -MMD $(C_KERNEL_FLAGS) -c $< -o $@
