# Makefile for KernOS bootloader and kernel development.
# Builds Stage1, Stage2, the kernel image, and a raw disk image suitable for QEMU.

NASM = nasm
LD = ld
CC = gcc
OBJCOPY = objcopy

BOOTDIR = boot
KERNELDIR = kernel
BUILD = build
IMAGE = kernos.img

STAGE1_SRC = $(BOOTDIR)/stage1.asm
STAGE2_SRC = $(BOOTDIR)/stage2.asm
KERNEL_ENTRY = $(KERNELDIR)/entry.asm
KERNEL_C = $(KERNELDIR)/kernel.c

STAGE1_BIN = $(BUILD)/stage1.bin
STAGE2_BIN = $(BUILD)/stage2.bin
KERNEL_ENTRY_OBJ = $(BUILD)/kernel_entry.o
KERNEL_OBJ = $(BUILD)/kernel.o
KERNEL_BIN = $(BUILD)/kernel.bin

STAGE2_SECTORS = 32
KERNEL_START_SECTOR = 33

all: $(IMAGE)

$(BUILD):
	mkdir -p $(BUILD)

$(STAGE1_BIN): $(STAGE1_SRC) | $(BUILD)
	$(NASM) -f bin -o $@ $^

$(STAGE2_BIN): $(STAGE2_SRC) | $(BUILD)
	$(NASM) -f bin -o $@ $^

$(KERNEL_ENTRY_OBJ): $(KERNEL_ENTRY) | $(BUILD)
	$(NASM) -f elf64 -o $@ $^

$(KERNEL_OBJ): $(KERNEL_C) | $(BUILD)
	$(CC) -ffreestanding -O2 -m64 -c -o $@ $^

$(KERNEL_BIN): $(KERNEL_ENTRY_OBJ) $(KERNEL_OBJ) | $(BUILD)
	$(LD) -T $(KERNELDIR)/linker.ld -o $@ $^

$(IMAGE): $(STAGE1_BIN) $(STAGE2_BIN) $(KERNEL_BIN)
	rm -f $@
	dd if=/dev/zero of=$@ bs=512 count=2880 conv=notrunc status=none
	dd if=$(STAGE1_BIN) of=$@ bs=512 count=1 conv=notrunc status=none
	dd if=$(STAGE2_BIN) of=$@ bs=512 count=$(STAGE2_SECTORS) seek=1 conv=notrunc status=none
	KERNEL_SIZE=$(shell stat -c%s $(KERNEL_BIN)); \
	KERNEL_SECTORS=$$(((KERNEL_SIZE + 511) / 512)); \
	dd if=$(KERNEL_BIN) of=$@ bs=512 seek=$(KERNEL_START_SECTOR) count=$$KERNEL_SECTORS conv=notrunc status=none

clean:
	rm -rf $(BUILD) $(IMAGE)
