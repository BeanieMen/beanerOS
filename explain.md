# beanerOS Kernel: Complete Technical Explanation

## Table of Contents
1. [Introduction and Architecture Overview](#introduction)
2. [Boot Process](#boot-process)
3. [Interrupt System](#interrupt-system)
4. [Memory Management](#memory-management)
5. [Hardware Drivers](#hardware-drivers)
6. [Filesystem](#filesystem)
7. [Shell and User Interaction](#shell)
8. [Build System and Toolchain](#build-system)
9. [Fundamental Computer Architecture Concepts](#fundamental-concepts)

---

## 1. Introduction and Architecture Overview {#introduction}

### What is beanerOS?

beanerOS is a **32-bit x86 monolithic kernel** designed to run on bare metal hardware (or in emulation via QEMU). It implements a complete operating system stack from bootloader to interactive shell, with the ultimate goal of running a Game Boy emulator to play Pokémon Red.

### Kernel Architecture

The kernel follows a **monolithic architecture** where all system services (memory management, device drivers, filesystem) run in kernel mode (Ring 0) with full hardware access. Key characteristics:

- **32-bit protected mode**: Uses x86 protected mode with paging and segmentation
- **Single address space**: All kernel code shares the same address space
- **Interrupt-driven I/O**: Hardware interaction via interrupt handlers
- **No userspace (yet)**: Everything runs in kernel mode; userspace is planned

### Memory Layout

```
0x00000000 - 0x000FFFFF : Real mode/BIOS (1 MB)
0x00100000 - 0x001FFFFF : Kernel code/data (loaded here by GRUB)
0x00200000 - 0x002FFFFF : Kernel heap (1 MB)
0x00400000 - 0x02000000 : Physical memory managed by PMM
0xB8000000             : VGA text mode buffer
```

---

## 2. Boot Process {#boot-process}

### 2.1 The Multiboot Specification

The kernel uses **GRUB (Grand Unified Bootloader)** which follows the Multiboot specification. This is a standardized way for bootloaders to load operating systems.

#### Multiboot Header (`loader.s`)

```assembly
MAGIC_NUMBER equ 0x1BADB002    ; Multiboot magic constant
FLAGS        equ 0x0            ; No special flags
CHECKSUM     equ -MAGIC_NUMBER  ; Must sum to zero with magic
```

**Concepts explained:**

- **Multiboot Magic**: The value `0x1BADB002` is a standardized identifier that tells GRUB "this is a valid kernel"
- **Checksum**: The bootloader verifies `MAGIC + FLAGS + CHECKSUM = 0` to ensure the header is valid
- **Location**: Must be in the first 8KB of the kernel file

The header is placed in the `.multiboot` section, which the linker script ensures is at the very start of the binary.

### 2.2 Bootloader Execution (`loader.s`)

```assembly
section .multiboot
align 4
    dd MAGIC_NUMBER
    dd FLAGS
    dd CHECKSUM
```

**What happens:**
1. BIOS performs POST (Power-On Self-Test)
2. BIOS loads GRUB from disk
3. GRUB finds our kernel on the ISO
4. GRUB verifies the Multiboot header
5. GRUB switches CPU to 32-bit protected mode
6. GRUB loads kernel to 0x00100000 (1 MB mark)
7. GRUB jumps to the `loader` symbol

### 2.3 Initial Assembly Setup

```assembly
section .bss
align 4
kernel_stack:
    resb KERNEL_STACK_SIZE  ; Reserve 4KB for stack

section .text
loader:
    mov esp, kernel_stack + KERNEL_STACK_SIZE  ; Set up stack
    push ebx    ; Push multiboot info pointer
    call kmain  ; Jump to C kernel
.loop:
    jmp .loop   ; Halt if kernel returns
```

**Why this is necessary:**

- **Stack setup**: C code requires a valid stack for function calls and local variables
- **ESP register**: Points to the top of the stack (grows downward on x86)
- **EBX register**: Contains pointer to Multiboot info structure (memory map, modules, etc.)
- **Protected mode**: CPU is already in 32-bit protected mode courtesy of GRUB

**Stack concept**: The stack is a LIFO (Last In, First Out) data structure used for:
- Function return addresses
- Local variables
- Function parameters
- Saved register values

### 2.4 Linker Script (`link.ld`)

```ld
ENTRY(loader)           ; Entry point symbol

SECTIONS {
    . = 0x00100000;     ; Load kernel at 1MB mark
    
    .text ALIGN (0x1000) : {
        *(.multiboot)   ; Multiboot header first
        *(.text)        ; Then all code
    }
    
    .rodata ALIGN (0x1000) : {
        *(.rodata*)     ; Read-only data (constants)
    }
    
    .data ALIGN (0x1000) : {
        *(.data)        ; Initialized global variables
    }
    
    .bss ALIGN (0x1000) : {
        *(COMMON)
        *(.bss)         ; Uninitialized globals
    }
}
```

**Purpose**: The linker script tells the linker (`ld`) how to organize the kernel binary:

- **Memory layout**: Where each section goes in memory
- **Alignment**: Each section aligned to 4KB (0x1000) page boundaries
- **Order**: Multiboot header must come first so GRUB can find it

**Section types:**
- `.text`: Executable code
- `.rodata`: Read-only data (string literals, const variables)
- `.data`: Initialized writable data
- `.bss`: Uninitialized data (zeroed by loader)

---

## 3. Interrupt System {#interrupt-system}

### 3.1 What are Interrupts?

**Interrupts** are signals that temporarily halt normal CPU execution to handle urgent events. Types:

1. **Hardware interrupts (IRQs)**: From devices (keyboard, timer, disk)
2. **Software interrupts**: Triggered by `int` instruction (system calls)
3. **Exceptions**: CPU-generated (divide by zero, page fault)

### 3.2 The Interrupt Descriptor Table (IDT)

The IDT is a table that maps interrupt numbers to handler functions. On x86, it has 256 entries.

#### IDT Structure (`src/include/idt.h`)

```c
typedef struct {
    uint16_t base_low;   // Handler address bits 0-15
    uint16_t selector;   // Code segment selector (0x08 = kernel code)
    uint8_t  zero;       // Always 0
    uint8_t  flags;      // Type and privilege level
    uint16_t base_high;  // Handler address bits 16-31
} __attribute__((packed)) idt_entry_t;
```

**Fields explained:**
- **base**: 32-bit address of the interrupt handler function
- **selector**: Which GDT segment to use (0x08 = kernel code segment from GRUB)
- **flags**: 
  - Bit 7: Present (must be 1)
  - Bits 5-6: Privilege level (0 = kernel only)
  - Bits 0-4: Gate type (0xE = 32-bit interrupt gate)

### 3.3 Setting Up the IDT (`src/kernel/idt.c`)

```c
void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt_entries[num].base_low = base & 0xFFFF;
    idt_entries[num].base_high = (base >> 16) & 0xFFFF;
    idt_entries[num].selector = sel;
    idt_entries[num].zero = 0;
    idt_entries[num].flags = flags;  // 0x8E = present, kernel, interrupt gate
}
```

**Process:**
1. Create 256 IDT entries in memory
2. For each interrupt, set the handler address
3. Load IDT address into IDTR register via `lidt` instruction

### 3.4 PIC (Programmable Interrupt Controller)

Hardware IRQs go through the PIC chip, which needs configuration:

```c
static void pic_remap(void) {
    outb(0x20, 0x11);  // Initialize PIC1
    outb(0xA0, 0x11);  // Initialize PIC2
    outb(0x21, 0x20);  // PIC1 IRQs start at interrupt 32
    outb(0xA1, 0x28);  // PIC2 IRQs start at interrupt 40
    outb(0x21, 0x04);  // PIC1 has slave at IRQ2
    outb(0xA1, 0x02);  // PIC2 cascade identity
    outb(0x21, 0x01);  // 8086 mode
    outb(0xA1, 0x01);  // 8086 mode
    outb(0x21, 0x00);  // Enable all IRQs on PIC1
    outb(0xA1, 0x00);  // Enable all IRQs on PIC2
}
```

**Why remap?** By default, hardware IRQs overlap with CPU exceptions (both use 0-15). We remap hardware IRQs to 32-47 to avoid conflicts.

### 3.5 Interrupt Handler Stubs (`src/kernel/interrupt.s`)

Assembly macros generate handler stubs:

```assembly
%macro ISR_NOERRCODE 1
global isr%1
isr%1:
    cli                ; Disable interrupts
    push byte 0        ; Dummy error code
    push byte %1       ; Interrupt number
    jmp isr_common_stub
%endmacro
```

**Common stub saves CPU state:**

```assembly
isr_common_stub:
    pusha              ; Save all general purpose registers
    mov ax, ds
    push eax           ; Save data segment
    
    mov ax, 0x10       ; Load kernel data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    push esp           ; Push stack pointer (points to register struct)
    call isr_handler   ; Call C handler
    add esp, 4         ; Clean up parameter
    
    pop eax            ; Restore data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    popa               ; Restore registers
    add esp, 8         ; Clean up error code and interrupt number
    iret               ; Return from interrupt
```

**What this does:**
1. Saves complete CPU state (registers, segments)
2. Switches to kernel data segment
3. Calls C handler with register struct
4. Restores CPU state
5. Returns to interrupted code

### 3.6 Interrupt Registration

Drivers register handlers:

```c
void register_interrupt_handler(uint8_t n, isr_t handler) {
    interrupt_handlers[n] = handler;
}

// Example: timer on IRQ 0 (interrupt 32)
register_interrupt_handler(32, &timer_callback);
```

---

## 4. Memory Management {#memory-management}

### 4.1 Physical Memory Manager (PMM)

The PMM tracks which 4KB physical memory frames are free or allocated using a **bitmap**.

#### Bitmap Data Structure (`src/mm/pmm.c`)

```c
#define FRAME_SIZE 4096       // 4KB frames
#define BITMAP_SIZE 32768     // 32KB bitmap
static uint8_t frame_bitmap[BITMAP_SIZE];
```

**Bitmap concept**: Each bit represents one 4KB frame:
- Bit = 0: Frame is free
- Bit = 1: Frame is allocated
- 32KB bitmap = 262,144 bits = 1 GB of trackable memory

#### Frame Operations

```c
static void set_frame(uint32_t frame_addr) {
    uint32_t frame = frame_addr / FRAME_SIZE;  // Convert address to frame number
    uint32_t idx = frame / 8;                  // Byte index in bitmap
    uint32_t off = frame % 8;                  // Bit offset within byte
    frame_bitmap[idx] |= (1 << off);           // Set bit to 1
}
```

**Math explanation:**
- Frame 0 = address 0x0000 - 0x0FFF
- Frame 1 = address 0x1000 - 0x1FFF
- Frame at address 0x5000 = Frame #5 = Byte 0, Bit 5

### 4.2 Virtual Memory Manager (VMM)

The VMM implements **paging**, which maps virtual addresses to physical addresses.

#### Two-Level Page Tables

x86 uses a two-level paging structure:

```
Virtual Address (32 bits):
┌────────────┬────────────┬──────────────┐
│ Dir Index  │ Table Index│    Offset    │
│  (10 bits) │  (10 bits) │   (12 bits)  │
└────────────┴────────────┴──────────────┘
   22-31         12-21          0-11
```

**Translation process:**
1. CR3 register points to page directory
2. Bits 22-31 index into page directory → get page table address
3. Bits 12-21 index into page table → get physical frame address  
4. Bits 0-11 are offset within the 4KB frame

#### Page Directory and Tables (`src/mm/vmm.c`)

```c
static uint32_t kernel_page_directory[1024] __attribute__((aligned(4096)));
static uint32_t kernel_page_tables[256][1024] __attribute__((aligned(4096)));
```

**Structure:**
- **Page Directory**: 1024 entries, each points to a page table
- **Page Table**: 1024 entries, each points to a 4KB frame
- Total coverage: 1024 × 1024 × 4KB = 4GB (full 32-bit address space)

#### Identity Mapping

```c
// Map first 256MB: virtual address = physical address
for (uint32_t i = 0; i < 256 * 1024 * 1024; i += PAGE_SIZE) {
    uint32_t pd_index = PAGE_DIRECTORY_INDEX(i);
    uint32_t pt_index = PAGE_TABLE_INDEX(i);
    kernel_page_tables[pd_index][pt_index] = i | PAGE_PRESENT | PAGE_RW;
}
```

**Why identity mapping?** Kernel code is loaded at 0x00100000 physical. After enabling paging, we want the same addresses to work, so we map virtual 0x00100000 → physical 0x00100000.

#### Enabling Paging (`src/mm/paging.s`)

```assembly
enable_paging:
    mov eax, [ebp + 8]  ; Get page directory address
    mov cr3, eax        ; Load page directory into CR3
    
    mov eax, cr0
    or eax, 0x80000000  ; Set PG bit (bit 31)
    mov cr0, eax        ; Enable paging
    ret
```

**CR3 register**: Holds physical address of current page directory. Changing CR3 switches address spaces (used for process switching in multi-process OS).

### 4.3 Kernel Heap (`src/mm/kheap.c`)

The heap provides dynamic memory allocation (like `malloc` in userspace).

#### First-Fit Allocator

```c
typedef struct heap_block {
    size_t size;               // Size of this block
    int is_free;               // Free or allocated?
    struct heap_block* next;   // Next block in linked list
} heap_block_t;
```

**Free list algorithm:**
1. Heap is a linked list of blocks
2. `kmalloc()`: Search list for first free block big enough
3. If block is much bigger, split it into two blocks
4. `kfree()`: Mark block as free, merge adjacent free blocks

**Example allocation:**

```
Initial heap (1MB free):
┌─────────────────────────────────────┐
│ [Block: 1MB, free, next=NULL]       │
└─────────────────────────────────────┘

After kmalloc(100):
┌──────────────────┬──────────────────┐
│ [100B, used]     │ [1MB-100B, free] │
└──────────────────┴──────────────────┘

After kfree():
┌─────────────────────────────────────┐
│ [Block: 1MB, free] (merged back)    │
└─────────────────────────────────────┘
```

---

## 5. Hardware Drivers {#hardware-drivers}

### 5.1 VGA Text Mode Terminal (`src/driver/tty.c`)

#### VGA Text Mode Basics

The VGA text buffer is a memory-mapped region at **0xB8000** that controls what appears on screen.

**Buffer format:**
- 80 columns × 25 rows = 2000 characters
- Each character is 2 bytes:
  ```
  Byte 0: ASCII character
  Byte 1: Color attribute (4 bits foreground, 4 bits background)
  ```

#### Writing to Screen

```c
static inline unsigned short vga_entry(unsigned char uc, unsigned char color) {
    return (unsigned short) uc | (unsigned short) color << 8;
}

void terminal_putentryat(char c, unsigned char color, unsigned int x, unsigned int y) {
    const unsigned int index = y * VGA_WIDTH + x;
    terminal_buffer[index] = vga_entry(c, color);
}
```

**Color format:**
```
┌────────┬────────┐
│ BG (4) │ FG (4) │
└────────┴────────┘
0 = Black, 1 = Blue, 2 = Green, ... 15 = White
```

#### Scrolling

When text reaches bottom of screen:

```c
static void terminal_scroll(void) {
    // Move each row up one line
    for (unsigned int y = 0; y < VGA_HEIGHT - 1; y++) {
        for (unsigned int x = 0; x < VGA_WIDTH; x++) {
            terminal_buffer[y * VGA_WIDTH + x] = 
                terminal_buffer[(y + 1) * VGA_WIDTH + x];
        }
    }
    // Clear bottom line
    for (unsigned int x = 0; x < VGA_WIDTH; x++) {
        terminal_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = vga_entry(' ', color);
    }
}
```

#### Cursor Control

The hardware cursor position is controlled via VGA controller ports:

```c
static void update_cursor(unsigned int x, unsigned int y) {
    unsigned short pos = y * VGA_WIDTH + x;  // Linear position
    
    outb(0x3D4, 0x0F);           // Select cursor low byte register
    outb(0x3D5, pos & 0xFF);     // Write low byte
    outb(0x3D4, 0x0E);           // Select cursor high byte register
    outb(0x3D5, (pos >> 8) & 0xFF); // Write high byte
}
```


### 5.2 PS/2 Keyboard Driver (`src/driver/keyboard.c`)

#### Scancode Architecture

When you press a key, the keyboard sends a **scancode** (not ASCII):

```c
static const char scancode_to_ascii[] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    ...
};
```

- Scancode 0x02 = key "1"
- Scancode 0x1E = key "A"
- Scancode 0x10 = key "Q"

#### Interrupt-Driven Input

```c
static void keyboard_irq_handler(registers_t* regs) {
    unsigned char scancode = inb(0x60);  // Read scancode from port 0x60
    
    if (scancode == 0x2A || scancode == 0x36) {
        shift_pressed = 1;  // Shift key pressed
    } else if (scancode == 0xAA || scancode == 0xB6) {
        shift_pressed = 0;  // Shift key released (bit 7 set = release)
    } else if (!(scancode & 0x80)) {  // Key press (not release)
        char ascii = shift_pressed ? 
            scancode_to_ascii_shifted[scancode] : 
            scancode_to_ascii[scancode];
        
        // Add to circular buffer
        keyboard_buffer[buffer_write] = ascii;
        buffer_write = (buffer_write + 1) % KEYBOARD_BUFFER_SIZE;
    }
}
```

**Circular buffer**: Producer (interrupt handler) writes, consumer (shell) reads. Wrap around at buffer end.

```
Buffer:  [a][b][c][d][e][ ][ ][ ]
Read:     ^                 
Write:                   ^
```

### 5.3 PIT Timer (`src/driver/timer.c`)

The **Programmable Interval Timer** generates periodic interrupts.

#### Frequency Calculation

```c
void timer_init(uint32_t frequency) {
    uint32_t divisor = 1193180 / frequency;  // PIT base frequency: 1.193180 MHz
    
    outb(0x43, 0x36);  // Command: Channel 0, rate generator mode
    
    outb(0x40, divisor & 0xFF);        // Send low byte
    outb(0x40, (divisor >> 8) & 0xFF); // Send high byte
}
```

**Example**: For 100 Hz:
- Divisor = 1193180 / 100 = 11931
- Timer fires every 10ms

#### Usage

```c
static volatile uint32_t tick_count = 0;

static void timer_callback(registers_t* regs) {
    tick_count++;  // Increment on each interrupt
}

void timer_wait(uint32_t ticks) {
    uint32_t target = tick_count + ticks;
    while (tick_count < target) {
        __asm__ __volatile__("hlt");  // Wait for interrupt
    }
}
```

### 5.4 ATA/IDE Disk Driver (`src/driver/ata.c`)

#### PIO Mode (Programmed I/O)

The driver uses PIO mode where CPU reads/writes disk via I/O ports:

```c
int ata_read_sectors(uint32_t lba, uint8_t sector_count, uint8_t *buffer) {
    ata_wait_bsy();  // Wait for controller to be ready
    
    // Send LBA address in 28-bit LBA mode
    outb(ata_base + ATA_REG_DRIVE, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ata_base + ATA_REG_SECCOUNT, sector_count);
    outb(ata_base + ATA_REG_LBA_LO, (uint8_t)lba);
    outb(ata_base + ATA_REG_LBA_MID, (uint8_t)(lba >> 8));
    outb(ata_base + ATA_REG_LBA_HI, (uint8_t)(lba >> 16));
    outb(ata_base + ATA_REG_COMMAND, ATA_CMD_READ_PIO);
    
    // Read each sector (512 bytes)
    for (uint8_t i = 0; i < sector_count; i++) {
        ata_wait_drq();  // Wait for data ready
        
        // Read 256 words (512 bytes)
        for (int j = 0; j < 256; j++) {
            uint16_t data = inw(ata_base + ATA_REG_DATA);
            buffer[i * 512 + j * 2] = (uint8_t)data;
            buffer[i * 512 + j * 2 + 1] = (uint8_t)(data >> 8);
        }
    }
    return 0;
}
```

**LBA (Logical Block Addressing)**: Linear sector addressing
- LBA 0 = First sector on disk
- LBA 1 = Second sector
- Each sector = 512 bytes

**Register layout:**
- 0x1F0: Data port (16-bit reads/writes)
- 0x1F2: Sector count
- 0x1F3-0x1F5: LBA low, mid, high bytes
- 0x1F6: Drive and LBA high nibble
- 0x1F7: Command/Status register

### 5.5 Serial Port (`src/driver/serial.c`)

Used for debugging output to COM1 (logged by QEMU).

```c
void serial_init(void) {
    outb(PORT + 1, 0x00);  // Disable interrupts
    outb(PORT + 3, 0x80);  // Enable DLAB (divisor latch access)
    outb(PORT + 0, 0x03);  // Divisor low byte (38400 baud)
    outb(PORT + 1, 0x00);  // Divisor high byte
    outb(PORT + 3, 0x03);  // 8N1 (8 bits, no parity, 1 stop bit)
    outb(PORT + 2, 0xC7);  // Enable FIFO
}

void serial_putchar(char c) {
    while ((inb(PORT + 5) & 0x20) == 0);  // Wait for transmit buffer empty
    outb(PORT, c);
}
```

---

## 6. Filesystem {#filesystem}

### 6.1 FAT32 Overview

**FAT32** (File Allocation Table, 32-bit) is a simple filesystem used on USB drives, SD cards, and our disk image.

#### Key Structures

**Boot Sector** (first sector on disk):
```c
typedef struct {
    uint8_t  jump[3];              // Jump instruction
    char     oem_name[8];          // OEM identifier
    uint16_t bytes_per_sector;     // Usually 512
    uint8_t  sectors_per_cluster;  // Usually 8 (4KB clusters)
    uint16_t reserved_sectors;     // Size of boot area
    uint8_t  fat_count;            // Usually 2 (backup FAT)
    uint16_t root_entries;         // 0 for FAT32
    uint32_t sectors_per_fat_32;   // Size of one FAT
    uint32_t root_cluster;         // Root directory cluster (usually 2)
    // ...
} __attribute__((packed)) fat32_boot_sector_t;
```

**Disk Layout:**
```
┌────────────────┬──────┬──────┬──────────────────────┐
│ Boot Sector    │ FAT1 │ FAT2 │ Data Area (clusters) │
│ (Reserved)     │      │      │                      │
└────────────────┴──────┴──────┴──────────────────────┘
Sector 0         Sector X      Sector Y
```

#### Clusters and the FAT

- **Cluster**: Smallest allocation unit (typically 4KB = 8 sectors)
- **FAT**: Table mapping cluster → next cluster in file

**Example file spanning clusters:**
```
File: "hello.txt" (10KB) starting at cluster 3

FAT table:
Cluster 3 → 5    (next part of file)
Cluster 5 → 8    (next part)
Cluster 8 → EOF  (end of file marker 0x0FFFFFFF)

File is stored in clusters: 3 → 5 → 8
```

#### Directory Entries

```c
typedef struct {
    char     name[11];             // 8.3 filename (padded with spaces)
    uint8_t  attributes;           // 0x10 = directory, 0x20 = archive
    uint8_t  reserved;
    uint8_t  creation_time_tenth;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_access_date;
    uint16_t first_cluster_high;   // High 16 bits of cluster
    uint16_t last_modification_time;
    uint16_t last_modification_date;
    uint16_t first_cluster_low;    // Low 16 bits of cluster
    uint32_t file_size;            // Size in bytes
} __attribute__((packed)) fat32_dir_entry_t;
```

**8.3 filename format**: 
- "README.TXT" → "README  TXT" (name padded to 8, ext to 3)
- "HELLO.C" → "HELLO   C  "

### 6.2 FAT32 Operations

#### Reading a File

```c
int fat32_read_file(fat32_fs_t *fs, const char *filename, uint8_t *buffer) {
    // 1. Find directory entry
    fat32_dir_entry_t entry;
    if (fat32_find_entry(fs, filename, &entry) != 0) return -1;
    
    // 2. Get starting cluster
    uint32_t cluster = ((uint32_t)entry.first_cluster_high << 16) | 
                       entry.first_cluster_low;
    
    // 3. Follow cluster chain
    uint32_t bytes_read = 0;
    while (cluster < 0x0FFFFFF8) {  // While not EOF
        // Convert cluster to LBA
        uint32_t lba = cluster_to_lba(fs, cluster);
        
        // Read cluster (usually 8 sectors = 4KB)
        ata_read_sectors(lba, fs->boot_sector.sectors_per_cluster, 
                        buffer + bytes_read);
        
        bytes_read += fs->boot_sector.sectors_per_cluster * 512;
        
        // Get next cluster from FAT
        cluster = fat32_get_next_cluster(fs, cluster);
    }
    
    return bytes_read;
}
```

#### Getting Next Cluster from FAT

```c
uint32_t fat32_get_next_cluster(fat32_fs_t *fs, uint32_t cluster) {
    // Calculate FAT sector and offset
    uint32_t fat_sector = fs->fat_start + (cluster * 4) / 512;
    uint32_t fat_offset = (cluster * 4) % 512;
    
    // Read FAT sector
    uint8_t buffer[512];
    ata_read_sectors(fat_sector, 1, buffer);
    
    // Extract next cluster (32-bit value)
    uint32_t *fat_entry = (uint32_t *)(buffer + fat_offset);
    return *fat_entry & 0x0FFFFFFF;  // Mask top 4 bits (reserved)
}
```

**FAT Chain Example:**
```
Cluster 5's entry in FAT:
FAT sector = 0 + (5 * 4) / 512 = 0 + 20/512 = sector 0
FAT offset = (5 * 4) % 512 = 20 bytes into sector

Read sector 0 of FAT
Extract 32-bit value at offset 20
Result = next cluster number (or 0x0FFFFFFF for EOF)
```

---

## 7. Shell and User Interaction {#shell}

### 7.1 Shell Loop (`src/kernel/shell.c`)

```c
void shell_loop(void) {
    char cmd_buffer[256];
    unsigned int cmd_pos = 0;
    
    while (1) {
        printf("beanerOS> ");
        
        // Read command
        while (1) {
            char c = keyboard_getchar();  // Blocks until key pressed
            
            if (c == '\n') {
                terminal_putchar('\n');
                cmd_buffer[cmd_pos] = '\0';
                process_command(cmd_buffer);
                cmd_pos = 0;
                break;
            } else if (c == '\b' && cmd_pos > 0) {
                cmd_pos--;
                terminal_backspace();
            } else if (c >= 32 && c <= 126 && cmd_pos < 255) {
                cmd_buffer[cmd_pos++] = c;
                terminal_putchar(c);
            }
        }
    }
}
```

### 7.2 Command Processing

```c
static void process_command(const char *cmd) {
    if (strcmp(cmd, "ls") == 0) {
        cmd_ls(&g_fs);
    } else if (strcmp(cmd, "pwd") == 0) {
        printf("%s\n", g_fs.current_dir);
    } else if (strncmp(cmd, "cat ", 4) == 0) {
        cmd_cat(&g_fs, cmd + 4);  // Pass filename after "cat "
    } else if (strncmp(cmd, "cd ", 3) == 0) {
        cmd_cd(&g_fs, cmd + 3);
    } else if (strcmp(cmd, "help") == 0) {
        cmd_help();
    } else {
        printf("Unknown command: %s\n", cmd);
    }
}
```

### 7.3 Command Implementation Example: `cat`

```c
void cmd_cat(fat32_fs_t *fs, const char *filename) {
    // Find file
    fat32_dir_entry_t entry;
    if (fat32_find_entry(fs, filename, &entry) != 0) {
        printf("File not found: %s\n", filename);
        return;
    }
    
    // Allocate buffer for file contents
    uint8_t *buffer = kmalloc(entry.file_size + 1);
    if (!buffer) {
        printf("Out of memory\n");
        return;
    }
    
    // Read entire file
    if (fat32_read_file(fs, filename, buffer) > 0) {
        buffer[entry.file_size] = '\0';  // Null terminate
        printf("%s", buffer);
    }
    
    kfree(buffer);
}
```


---

## 8. Build System and Toolchain {#build-system}

### 8.1 Cross-Compilation

The kernel is built with a **cross-compiler** (i686-elf-gcc) that targets x86 bare metal (no OS).

**Why cross-compile?** Your host OS's compiler produces binaries for your host OS (Linux, macOS, etc.). We need binaries for bare metal x86.

### 8.2 Compilation Flags

```makefile
CFLAGS = -m32 -nostdlib -nostdinc -fno-builtin -fstack-protector-strong \
         -nostartfiles -nodefaultlibs -Wall -Wextra -Werror -c \
         -I$(INCLUDE_DIR) -I$(LIBC_DIR)/include -D__is_libk
```

**Flags explained:**
- `-m32`: Generate 32-bit x86 code
- `-nostdlib`: Don't link standard library (no printf, malloc from libc)
- `-nostdinc`: Don't use standard includes (we provide our own)
- `-fno-builtin`: Don't assume functions like memcpy exist
- `-fstack-protector-strong`: Add stack overflow protection
- `-nostartfiles`: Don't use standard startup files (_start, crt0)
- `-Wall -Wextra -Werror`: Enable all warnings, treat as errors
- `-c`: Compile only, don't link
- `-I`: Include directories for headers

### 8.3 Build Process

```makefile
# Compile assembly
loader.o: loader.s
    nasm -f elf32 loader.s -o loader.o

# Compile C source
kernel/kernel.o: kernel/kernel.c
    gcc $(CFLAGS) kernel/kernel.c -o kernel/kernel.o

# Link kernel
kernel.elf: loader.o kernel.o driver.o mm.o ...
    ld -T link.ld -melf_i386 *.o -o kernel.elf

# Create ISO
os.iso: kernel.elf
    mkdir -p iso/boot/grub
    cp kernel.elf iso/boot/
    genisoimage -R -b boot/grub/stage2_eltorito \
                -no-emul-boot -boot-load-size 4 \
                -o os.iso iso/
```

**ISO structure:**
```
os.iso
├── boot/
│   ├── kernel.elf       (our kernel)
│   └── grub/
│       ├── stage2_eltorito  (GRUB bootloader)
│       └── menu.lst         (boot configuration)
```

### 8.4 Running in QEMU

```bash
qemu-system-i386 -m 32 -cdrom os.iso -boot d -serial file:serial.log \
                 -drive file=disk.img,format=raw,if=ide
```

**Parameters:**
- `-m 32`: 32MB RAM
- `-cdrom os.iso`: Boot from our ISO
- `-boot d`: Boot from CD-ROM
- `-serial file:serial.log`: Redirect COM1 to file
- `-drive`: Attach disk image as IDE drive

---

## 9. Fundamental Computer Architecture Concepts {#fundamental-concepts}

### 9.1 x86 Processor Architecture

#### Registers

The x86 CPU has several types of registers:

**General Purpose Registers (32-bit):**
- **EAX**: Accumulator (arithmetic operations, return values)
- **EBX**: Base register (base pointer for memory access)
- **ECX**: Counter (loop counter)
- **EDX**: Data register (I/O operations, multiply/divide)
- **ESI**: Source Index (string/memory operations)
- **EDI**: Destination Index (string/memory operations)
- **EBP**: Base Pointer (stack frame pointer)
- **ESP**: Stack Pointer (top of stack)

**Segment Registers (16-bit):**
- **CS**: Code Segment (where executable code is)
- **DS**: Data Segment (where data is)
- **SS**: Stack Segment (where stack is)
- **ES, FS, GS**: Extra segments

**Control Registers:**
- **CR0**: Control bits (paging enable, protection mode)
- **CR3**: Page directory base address
- **CR4**: Extended control bits

**Special Registers:**
- **EIP**: Instruction Pointer (next instruction address)
- **EFLAGS**: Status flags (carry, zero, overflow, interrupt enable)

### 9.2 CPU Modes

#### Real Mode (16-bit)

- Direct memory access (no protection)
- 1MB address space limit
- Used by BIOS
- Segmented memory model

#### Protected Mode (32-bit)

- Memory protection via segmentation and paging
- 4GB address space
- Privilege levels (Ring 0-3)
- Virtual memory support
- Our kernel runs here

**Privilege Rings:**
```
┌─────────────────────────────────┐
│         Ring 3 (User)           │  ← Userspace programs
├─────────────────────────────────┤
│         Ring 2, 1               │  ← Unused in most OS
├─────────────────────────────────┤
│         Ring 0 (Kernel)         │  ← Our kernel, full hardware access
└─────────────────────────────────┘
```

### 9.3 Memory Segmentation

x86 uses **segmentation** where addresses are calculated:

```
Linear Address = Segment Base + Offset
```

GRUB sets up a **flat memory model**:
- All segment bases = 0
- All segment limits = 4GB
- Effectively disables segmentation

**Global Descriptor Table (GDT):**
GRUB provides a GDT with segments:
- 0x00: Null descriptor
- 0x08: Kernel code segment (4GB, executable, Ring 0)
- 0x10: Kernel data segment (4GB, writable, Ring 0)

### 9.4 Paging

Paging provides:
1. **Virtual memory**: Programs use virtual addresses, CPU translates to physical
2. **Memory protection**: Pages can be read-only, no-execute
3. **Memory isolation**: Different page tables for different processes

**Page attributes:**
- **Present (P)**: Page is in memory
- **Read/Write (R/W)**: 0 = read-only, 1 = read-write
- **User/Supervisor (U/S)**: 0 = kernel only, 1 = user accessible
- **Write-Through (PWT)**: Caching policy
- **Cache Disable (PCD)**: Disable caching for this page
- **Accessed (A)**: Set by CPU when page is accessed
- **Dirty (D)**: Set by CPU when page is written to

### 9.5 Interrupts Deep Dive

#### Interrupt Vector Table

x86 reserves interrupt numbers:
- **0-31**: CPU exceptions
  - 0: Divide by zero
  - 6: Invalid opcode
  - 13: General protection fault
  - 14: Page fault
- **32-47**: Hardware IRQs (after PIC remap)
  - 32: Timer (IRQ 0)
  - 33: Keyboard (IRQ 1)
  - 46: Primary ATA (IRQ 14)
- **48-255**: Available for software interrupts

#### Interrupt Handling Process

1. Hardware raises interrupt signal
2. CPU finishes current instruction
3. CPU disables interrupts (CLI)
4. CPU pushes EFLAGS, CS, EIP to stack
5. CPU jumps to handler from IDT
6. Handler saves registers, processes interrupt
7. Handler signals EOI to PIC
8. Handler restores registers, executes IRET
9. CPU restores EFLAGS, CS, EIP and continues

### 9.6 I/O Port Architecture

x86 has separate I/O address space (65536 ports):

```c
// Read byte from port
uint8_t inb(uint16_t port) {
    uint8_t result;
    __asm__ __volatile__("inb %1, %0" : "=a"(result) : "dN"(port));
    return result;
}

// Write byte to port  
void outb(uint16_t port, uint8_t data) {
    __asm__ __volatile__("outb %0, %1" : : "a"(data), "dN"(port));
}
```

**Common ports:**
- 0x20, 0x21: PIC1 (master)
- 0xA0, 0xA1: PIC2 (slave)
- 0x40-0x43: PIT (timer)
- 0x60, 0x64: PS/2 keyboard
- 0x1F0-0x1F7: Primary ATA
- 0x3D4, 0x3D5: VGA controller
- 0x3F8-0x3FF: COM1 (serial port)

### 9.7 Stack Architecture

The stack is a memory region that grows downward:

```
High Address
┌─────────────┐
│             │
│    Heap     │  ← Dynamic allocations (grows up)
│             │
├─────────────┤
│             │
│    Free     │
│             │
├─────────────┤
│             │
│    Stack    │  ← Function calls (grows down)
│             │
└─────────────┘
Low Address
```

**Function call process:**
```
1. Caller pushes arguments right-to-left
2. Caller executes CALL (pushes return address, jumps)
3. Callee pushes EBP (save old base pointer)
4. Callee sets EBP = ESP (new base pointer)
5. Callee reserves space for locals (sub ESP, X)
6. Function executes
7. Callee restores ESP to EBP (mov ESP, EBP)
8. Callee pops EBP
9. Callee executes RET (pops return address, jumps)
10. Caller cleans up arguments
```

**Stack frame:**
```
Higher addresses
├─────────────┤
│  Argument 2 │  EBP + 12
├─────────────┤
│  Argument 1 │  EBP + 8
├─────────────┤
│Return Addr  │  EBP + 4
├─────────────┤
│  Old EBP    │  ← EBP points here
├─────────────┤
│  Local 1    │  EBP - 4
├─────────────┤
│  Local 2    │  EBP - 8
├─────────────┤  ← ESP points here
Lower addresses
```

### 9.8 ELF Binary Format

Our kernel is an ELF (Executable and Linkable Format) binary:

**ELF Header:**
```c
typedef struct {
    uint8_t  e_ident[16];     // Magic: 0x7F 'E' 'L' 'F'
    uint16_t e_type;          // Type: ET_EXEC (executable)
    uint16_t e_machine;       // Architecture: EM_386 (Intel 80386)
    uint32_t e_version;       // Version: EV_CURRENT
    uint32_t e_entry;         // Entry point address (loader)
    uint32_t e_phoff;         // Program header offset
    uint32_t e_shoff;         // Section header offset
    // ...
} Elf32_Ehdr;
```

**Program Headers** tell loader where to load sections:
```c
typedef struct {
    uint32_t p_type;    // PT_LOAD = loadable segment
    uint32_t p_offset;  // Offset in file
    uint32_t p_vaddr;   // Virtual address to load at
    uint32_t p_paddr;   // Physical address
    uint32_t p_filesz;  // Size in file
    uint32_t p_memsz;   // Size in memory (.bss is larger)
    uint32_t p_flags;   // Permissions (R/W/X)
    uint32_t p_align;   // Alignment
} Elf32_Phdr;
```

### 9.9 Calling Conventions

**cdecl** (C declaration) convention used by our kernel:

1. **Arguments**: Pushed right-to-left on stack
2. **Return value**: In EAX register
3. **Caller cleanup**: Caller removes arguments from stack
4. **Preserved registers**: EBP, EBX, ESI, EDI must be saved/restored by callee
5. **Scratch registers**: EAX, ECX, EDX can be modified by callee

**Example:**
```c
int add(int a, int b) {
    return a + b;
}

int result = add(3, 5);
```

**Assembly equivalent:**
```assembly
; Caller:
push 5          ; Push arg 2
push 3          ; Push arg 1
call add        ; Push return address, jump
add esp, 8      ; Clean up 8 bytes of arguments
mov [result], eax ; Store return value

; Callee (add):
add:
    push ebp            ; Save old base pointer
    mov ebp, esp        ; Set up new base pointer
    mov eax, [ebp+8]    ; Load arg 1 (a) into EAX
    add eax, [ebp+12]   ; Add arg 2 (b) to EAX
    pop ebp             ; Restore base pointer
    ret                 ; Return (pop return address, jump)
```

### 9.10 Inline Assembly

GCC inline assembly allows C code to include assembly:

```c
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ __volatile__(
        "outb %0, %1"           // Assembly instruction
        :                        // No output operands
        : "a"(val), "dN"(port)  // Input: val in AL, port in DX
    );
}
```

**Syntax:**
```c
__asm__ __volatile__(
    "assembly code"
    : output operands
    : input operands
    : clobbered registers
);
```

**Constraints:**
- `"a"`: EAX register
- `"b"`: EBX register
- `"c"`: ECX register
- `"d"`: EDX register
- `"r"`: Any general purpose register
- `"m"`: Memory operand
- `"i"`: Immediate constant
- `"N"`: 0-255 immediate

---

## 10. Advanced Topics

### 10.1 GRUB Multiboot Information

GRUB passes information to kernel via multiboot structure:

```c
typedef struct multiboot_info {
    uint32_t flags;          // Which fields are valid
    uint32_t mem_lower;      // KB of lower memory (< 1MB)
    uint32_t mem_upper;      // KB of upper memory (> 1MB)
    uint32_t boot_device;    // BIOS boot device
    uint32_t cmdline;        // Kernel command line
    uint32_t mods_count;     // Number of loaded modules
    uint32_t mods_addr;      // Address of module list
    uint32_t syms[4];        // Symbol table info
    uint32_t mmap_length;    // Memory map size
    uint32_t mmap_addr;      // Memory map address
} multiboot_info_t;
```

**Memory map** tells us which RAM regions are usable:
```
0x00000000-0x0009FFFF: Usable (640 KB)
0x000A0000-0x000FFFFF: Reserved (VGA, BIOS ROM)
0x00100000-0x07FFFFFF: Usable (127 MB)
```

### 10.2 DMA (Direct Memory Access)

Future enhancement: Use DMA for disk I/O instead of PIO:
- CPU programs DMA controller
- DMA controller transfers data directly between disk and RAM
- CPU is free to do other work
- DMA controller interrupts when transfer complete

### 10.3 Virtual File System (VFS)

For multiple filesystem support, implement VFS layer:

```c
typedef struct vfs_operations {
    int (*mount)(const char *device);
    int (*read)(const char *path, void *buf, size_t size);
    int (*write)(const char *path, const void *buf, size_t size);
    // ...
} vfs_ops_t;

// Register filesystems
vfs_register("fat32", &fat32_ops);
vfs_register("ext2", &ext2_ops);
```

### 10.4 System Calls

For userspace support, implement syscall interface:

```c
// Userspace triggers interrupt 0x80
int sys_read(int fd, void *buf, size_t count) {
    int result;
    __asm__ __volatile__(
        "int $0x80"
        : "=a"(result)
        : "a"(SYS_READ), "b"(fd), "c"(buf), "d"(count)
    );
    return result;
}

// Kernel syscall handler
void syscall_handler(registers_t *regs) {
    switch (regs->eax) {  // Syscall number in EAX
        case SYS_READ:
            regs->eax = do_sys_read(regs->ebx, regs->ecx, regs->edx);
            break;
        // ...
    }
}
```

### 10.5 Multitasking

Implement preemptive multitasking:

```c
typedef struct task {
    uint32_t esp;           // Saved stack pointer
    uint32_t ebp;           // Saved base pointer
    uint32_t eip;           // Saved instruction pointer
    page_directory_t *pdir; // Page directory
    struct task *next;      // Next task in list
} task_t;

void schedule(void) {
    current_task = current_task->next;
    switch_to_task(current_task);
}

// Called from timer interrupt
void timer_callback(registers_t *regs) {
    tick_count++;
    if (tick_count % 10 == 0) {  // Every 100ms
        schedule();
    }
}
```

---

## 11. Debugging Techniques

### 11.1 Serial Logging

Always log important events:

```c
serial_write("[KERNEL] PMM initialized\n");
serial_printf("Frame %d allocated at 0x%x\n", frame_num, addr);
```

QEMU saves to `serial.log` for post-mortem debugging.

### 11.2 GDB Debugging

```bash
# Terminal 1: Start QEMU in debug mode
qemu-system-i386 -s -S -cdrom os.iso

# Terminal 2: Connect GDB
gdb kernel.elf
(gdb) target remote localhost:1234
(gdb) break kmain
(gdb) continue
(gdb) info registers
(gdb) x/10i $eip  # Disassemble 10 instructions at EIP
```

### 11.3 Bochs Debugger

Bochs emulator has built-in debugger:

```
<bochs:1> lb 0x00100000    # Set breakpoint
<bochs:2> c                 # Continue
<bochs:3> info cpu          # Show CPU state
<bochs:4> x /10 0xB8000    # Examine memory
```

### 11.4 Printf Debugging

Strategic prints help trace execution:

```c
printf("[DEBUG] About to read sector %d\n", lba);
ata_read_sectors(lba, 1, buffer);
printf("[DEBUG] Sector read complete, first byte: 0x%x\n", buffer[0]);
```

---

## 12. Common Pitfalls and Solutions

### 12.1 Triple Fault

**Symptom**: QEMU reboots immediately
**Cause**: Usually bad interrupt handler or page fault in page fault handler
**Solution**: Use serial logging before enabling interrupts

### 12.2 Stack Overflow

**Symptom**: Random crashes, corrupted data
**Cause**: Deep recursion, large local variables
**Solution**: Increase stack size, use heap for large buffers

### 12.3 Page Fault

**Symptom**: CPU exception 14
**Cause**: Access invalid memory, wrong page table setup
**Debug**: Read CR2 register (contains fault address)

```c
void page_fault_handler(registers_t *regs) {
    uint32_t fault_addr;
    __asm__ __volatile__("mov %%cr2, %0" : "=r"(fault_addr));
    printf("Page fault at 0x%x\n", fault_addr);
    printf("Error code: 0x%x\n", regs->err_code);
}
```

### 12.4 General Protection Fault

**Symptom**: CPU exception 13
**Cause**: Invalid segment selector, privilege violation
**Solution**: Check segment registers, ensure correct DPL in GDT/IDT

---

## Conclusion

This kernel demonstrates fundamental OS concepts:

1. **Boot process**: From BIOS to protected mode to C kernel
2. **Hardware abstraction**: Drivers for VGA, keyboard, disk, timer
3. **Memory management**: Physical frames, virtual paging, dynamic allocation
4. **Interrupt handling**: CPU exceptions and hardware IRQs
5. **Filesystem**: FAT32 implementation with read/write
6. **User interface**: Interactive shell with file operations

Each component builds on computer architecture fundamentals:
- **CPU architecture**: Registers, modes, protection rings
- **Memory**: Segmentation, paging, virtual addresses
- **I/O**: Port-based communication, interrupt-driven operation
- **Assembly**: Low-level control where C isn't sufficient

The kernel provides a foundation for future enhancements like userspace, multitasking, networking, and the ultimate goal: a Game Boy emulator running Pokémon Red.

---

## References and Further Reading

1. **Intel 64 and IA-32 Architectures Software Developer's Manual**
   - Definitive reference for x86 architecture
   - https://www.intel.com/sdm

2. **OSDev Wiki**
   - Comprehensive OS development resource
   - https://wiki.osdev.org

3. **The Little OS Book**
   - Practical guide to OS development
   - https://littleosbook.github.io

4. **xv6: A simple, Unix-like teaching operating system**
   - MIT's teaching OS with excellent commentary
   - https://pdos.csail.mit.edu/6.828/2020/xv6.html

5. **Writing a Simple Operating System from Scratch**
   - Nick Blundell's comprehensive guide
   - https://www.cs.bham.ac.uk/~exr/lectures/opsys/10_11/lectures/os-dev.pdf

6. **Multiboot Specification**
   - GRUB bootloader protocol
   - https://www.gnu.org/software/grub/manual/multiboot/multiboot.html

7. **FAT32 Specification**
   - Microsoft's FAT filesystem documentation
   - https://www.win.tue.nl/~aeb/linux/fs/fat/fat-1.html

8. **Game Boy Development Documentation**
   - Pan Docs (for future emulator work)
   - https://gbdev.io/pandocs/
