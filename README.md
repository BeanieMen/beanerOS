# 🧠 beanerOS

> A custom x86 operating system built from scratch, designed to run a Game Boy emulator and play Pokémon Red in the terminal.

![beanerOS Boot](https://github.com/user-attachments/assets/d5e61bbf-e717-4f8f-87b8-f4f8e8681182)

## 🎯 Project Vision

beanerOS is an ambitious hobby OS project that aims to create a complete computing stack:

**GRUB Bootloader** → **Custom Kernel** → **Userspace** → **Game Boy Emulator** → **Pokémon Red**

The end goal is to boot into a terminal-based Game Boy emulator running Pokémon Red, all running on custom-built operating system components.

## 🚀 Quick Start

### Prerequisites

- **QEMU** for emulation
- **GCC cross-compiler** for x86 (i686-elf-gcc)
- **NASM** assembler
- **GNU Make**

### Building

```bash
# Clone the repository
git clone https://github.com/BeanieMen/beanerOS.git
cd beanerOS

# Build the kernel and create bootable ISO
make

# Run in QEMU
make run
```

### Testing with Disk Image

```bash
# Create a FAT32 disk image with test files
./tools/make_disk.sh

# Run with disk attached
make run-disk
```

## 📁 Project Structure

```
beanerOS/
├── loader.s              # Assembly bootloader
├── link.ld               # Linker script
├── Makefile              # Build system
├── src/
│   ├── include/          # Header files
│   ├── kernel/           # Core kernel components
│   │   ├── kernel.c      # Main kernel entry
│   │   ├── shell.c       # Interactive shell
│   │   ├── commands.c    # Shell commands (ls, cat, etc.)
│   │   ├── idt.c         # Interrupt descriptor table
│   │   └── interrupt.s   # Assembly interrupt handlers
│   ├── mm/               # Memory management
│   │   ├── pmm.c         # Physical memory manager
│   │   ├── vmm.c         # Virtual memory manager
│   │   ├── kheap.c       # Kernel heap allocator
│   │   └── paging.s      # Page table setup
│   ├── driver/           # Hardware drivers
│   │   ├── ata.c         # ATA/IDE disk driver
│   │   ├── keyboard.c    # PS/2 keyboard driver
│   │   ├── serial.c      # Serial port driver
│   │   ├── timer.c       # PIT timer driver
│   │   ├── tty.c         # VGA text mode terminal
│   │   └── io.c          # Low-level I/O utilities
│   ├── fs/               # Filesystem implementations
│   │   └── fat32.c       # Complete FAT32 filesystem
│   └── libc/             # C standard library subset
├── iso/                  # GRUB boot configuration
├── tools/                # Development utilities
└── README.md
```

## ⚙️ What's Implemented

beanerOS is built as a 32-bit x86 kernel with comprehensive low-level systems:

- **Bootloader**: Custom assembly bootloader (`loader.s`)
- **Kernel**: Monolithic kernel with full memory management
- **Filesystem**: Complete FAT32 implementation with file/directory operations
- **Drivers**: ATA disk, PS/2 keyboard, serial I/O, PIT timer, VGA text mode
- **Shell**: Interactive command shell with filesystem utilities (`ls`, `cat`, `echo`, `touch`, `mkdir`, `cd`, `pwd`)

See [ROADMAP.md](ROADMAP.md) for detailed progress and upcoming features.

## 🎮 Game Boy Emulator Goal

The ultimate objective is a terminal-based Game Boy emulator that can run Pokémon Red:

```
beanerOS> gameboy /roms/pokemon_red.gb
██████╗░░█████╗░██╗░░██╗███████╗███╗░░░███╗░█████╗░███╗░░██╗
██╔══██╗██╔══██╗██║░██╔╝██╔════╝████╗░████║██╔══██╗████╗░██║
██████╔╝██║░░██║█████═╝░█████╗░░██╔████╔██║██║░░██║██╔██╗██║
██╔═══╝░██║░░██║██╔═██╗░██╔══╝░░██║╚██╔╝██║██║░░██║██║╚████║
██║░░░░░╚█████╔╝██║░╚██╗███████╗██║░╚═╝░██║╚█████╔╝██║░╚███║
╚═╝░░░░░░╚════╝░╚═╝░░╚═╝╚══════╝╚═╝░░░░░╚═╝░╚════╝░╚═╝░░╚══╝
```

## 🧪 Testing & Development

### Running Tests
```bash
# Build and run
make && make run

# Run with serial output
make run-serial

# Debug with GDB
make debug
```

### Development Tools
- **QEMU**: Primary testing platform
- **GDB**: Kernel debugging
- **Serial Logging**: Debug output via COM1

## 📚 Learning Resources

This project draws inspiration from:
- [xv6 Operating System](https://github.com/mit-pdos/xv6-public) - MIT's teaching OS
- [Little OS Book](https://littleosbook.github.io/) - OS development guide
- [SerenityOS](https://github.com/SerenityOS/serenity) - Modern OS implementation
- [Pan Docs](https://gbdev.io/pandocs/) - Game Boy hardware reference

## 🤝 Contributing

This is a learning project, but contributions are welcome! Areas of interest:

- **Userspace Implementation**: Syscalls, ELF loading, process management
- **Emulator Development**: Game Boy CPU/PPU/APU emulation
- **Additional Drivers**: Network, USB, audio
- **Testing**: Unit tests, integration tests, performance benchmarks

## 📄 License

This project is released under the MIT License. See LICENSE for details.

## 🙏 Acknowledgments

- Built with ❤️ by [@BeanieMen](https://github.com/BeanieMen)
- Inspired by the OS development community
- Special thanks to the authors of xv6, SerenityOS, and other educational OS projects

---

*From bootloader to Pokémon Red: One kernel at a time.*