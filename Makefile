# Cross toolchain: brew install aarch64-elf-gcc   (pulls in aarch64-elf-binutils)
# Emulator:        brew install qemu
CROSS   := aarch64-elf-
CC      := $(CROSS)gcc
LD      := $(CROSS)ld
OBJCOPY := $(CROSS)objcopy
QEMU    := qemu-system-aarch64

# -ffreestanding: no libc, main() is not special
# -mgeneral-regs-only: no FP/SIMD registers, so interrupt handlers only need to save x0-x30
CFLAGS  := -Wall -Wextra -O2 -g -ffreestanding -nostdlib -nostartfiles \
           -mgeneral-regs-only -I.
ASFLAGS := -g -I.

BUILD := build

CSRCS := main.c $(wildcard IO/src/*.c Memory/src/*.c interrupts/src/*.c)
# boot.S goes first; the linker script also pins .text.boot to 0x80000
SSRCS := boot.S $(wildcard interrupts/*.S)

OBJS := $(SSRCS:%.S=$(BUILD)/%.o) $(CSRCS:%.c=$(BUILD)/%.o)

all: kernel8.img

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: %.S
	@mkdir -p $(dir $@)
	$(CC) $(ASFLAGS) -c $< -o $@

kernel8.elf: $(OBJS) linker.ld
	$(LD) -nostdlib --no-warn-rwx-segments -T linker.ld $(OBJS) -o $@

kernel8.img: kernel8.elf
	$(OBJCOPY) -O binary $< $@

# The mini UART (UART1) is QEMU's second serial port, so send the first one to null
run: kernel8.img
	$(QEMU) -M raspi4b -kernel kernel8.img -serial null -serial stdio -display none

# Same, but frozen at the first instruction with a gdb server on :1234
#   aarch64-elf-gdb kernel8.elf -ex "target remote :1234"
debug: kernel8.img
	$(QEMU) -M raspi4b -kernel kernel8.img -serial null -serial stdio -display none -s -S

clean:
	rm -rf $(BUILD) kernel8.elf kernel8.img

.PHONY: all run debug clean
