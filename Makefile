# Makefile for LittleOS

# Assembler and Linker
AS = nasm
LD = ld
CC = gcc

# Directories
SRC_DIR = src
INCLUDE_DIR = $(SRC_DIR)/include
DRIVER_DIR = $(SRC_DIR)/driver
KERNEL_DIR = $(SRC_DIR)/kernel
OBJ_DIR = $(SRC_DIR)/objects

# Flags
ASFLAGS = -f elf32
LDFLAGS = -T link.ld -melf_i386
CFLAGS = -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector \
         -nostartfiles -nodefaultlibs -Wall -Wextra -Werror -c -I$(INCLUDE_DIR)

# Output files
KERNEL = kernel.elf
ISO = os.iso

# Source files
ASM_SOURCES = loader.s
C_SOURCES = $(wildcard $(KERNEL_DIR)/*.c) $(wildcard $(DRIVER_DIR)/*.c)
ASM_OBJECTS = $(ASM_SOURCES:.s=.o)
C_OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(C_SOURCES))
OBJECTS = $(ASM_OBJECTS) $(C_OBJECTS)

# ISO directories
ISO_DIR = iso
BOOT_DIR = $(ISO_DIR)/boot
GRUB_DIR = $(BOOT_DIR)/grub

# QEMU settings
QEMU = qemu-system-i386
QEMU_FLAGS = -m 32 -cdrom $(ISO) -boot d -d cpu_reset,int,exec,cpu -D qemulog.txt

# Default target
.PHONY: all
all: $(ISO)

# Compile assembly files
%.o: %.s
	$(AS) $(ASFLAGS) $< -o $@

# Compile C sources
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< -o $@

# Link kernel
$(KERNEL): $(OBJECTS)
	$(LD) $(LDFLAGS) $(OBJECTS) -o $@

# Create ISO image
$(ISO): $(KERNEL)
	@mkdir -p $(BOOT_DIR)
	@cp $(KERNEL) $(BOOT_DIR)/
	@cp stage2_eltorito $(GRUB_DIR)/stage2_eltorito 2>/dev/null || true
	genisoimage -R \
		-b boot/grub/stage2_eltorito \
		-no-emul-boot \
		-boot-load-size 4 \
		-A os \
		-input-charset utf8 \
		-quiet \
		-boot-info-table \
		-o $(ISO) \
		$(ISO_DIR)
	@echo "ISO image created: $(ISO)"

# Run with QEMU
.PHONY: run
run: $(ISO)
	$(QEMU) $(QEMU_FLAGS)

# Clean build artifacts
.PHONY: clean
clean:
	rm -f $(ASM_OBJECTS) $(KERNEL) $(ISO)
	rm -f qemulog.txt
	rm -rf $(OBJ_DIR)

# Clean everything including ISO directory contents (except grub config)
.PHONY: distclean
distclean: clean
	rm -f $(BOOT_DIR)/$(KERNEL)

# Debug: show variables
.PHONY: debug
debug:
	@echo "OBJECTS: $(OBJECTS)"
	@echo "KERNEL: $(KERNEL)"
	@echo "ISO: $(ISO)"

# Help
.PHONY: help
help:
	@echo "LittleOS Makefile"
	@echo "================="
	@echo "Targets:"
	@echo "  all        - Build the ISO image (default)"
	@echo "  run        - Build and run the OS in QEMU"
	@echo "  clean      - Remove build artifacts"
	@echo "  distclean  - Remove all generated files"
	@echo "  debug      - Show build variables"
	@echo "  help       - Show this help message"
