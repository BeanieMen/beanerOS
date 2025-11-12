# 🧠 beanerOS Roadmap
> From bootloader → custom kernel → userspace → Pokémon Red on terminal emulator

---

## 🧱 Phase 0 — Current foundation
✅ Bootloader (`loader.s`, `link.ld`)  
✅ TTY text output (`src/driver/tty.c`)  
✅ Basic kernel main and Makefile  
✅ libc integration (stdio, stdlib, string)

**Status: Complete**

---

## ⚙️ Phase 1 — Core kernel systems
> Goal: Run user-mode programs safely and allocate memory dynamically.

✅ **Physical Memory Manager (PMM)**
  - Bitmap allocator for physical frames (`pmm_alloc_frame`, `pmm_free_frame`).
✅ **Paging / Virtual Memory Manager (VMM)**
  - Enable paging, identity-map kernel, separate user/kernel space.
  - Implement `vmm_map`, `vmm_unmap`, and page fault handler.
✅ **Kernel heap**
  - Implement `kmalloc` / `kfree` for dynamic allocation.
✅ **Interrupts & exceptions**
  - Remap PIC, handle IRQs, and basic ISR registration.
✅ **Timer (PIT or APIC)**
  - Periodic tick for scheduling and sleeping.
✅ **Keyboard driver**
  - PS/2 polling input (IRQ-based ready, needs integration).
✅ **Serial driver**
  - Output debug logs to COM1 (`/dev/ttyS0`).
✅ **Scheduler**
  - Task structs, context switching, and round-robin scheduling (framework ready).

**Status: Complete - All core systems implemented**

---

## 🧩 Phase 2 — User mode & syscalls
> Goal: Run programs in ring 3 and communicate via syscalls.

- [ ] **Privilege switch**
  - Setup TSS and user mode stack.
  - Implement `enter_user()` routine.
- [ ] **Syscall mechanism**
  - Use `int 0x80` or `syscall` instruction.
  - Syscalls: `read`, `write`, `exit`, `fork`, `exec`.
- [ ] **ELF loader**
  - Load ELF binaries into user address space.
  - Map sections, stack, and jump to entry point.
- [ ] **Init process**
  - First userspace process that spawns the shell.

---

## 💾 Phase 3 — Filesystem & storage
> Goal: Enable persistent file loading and ROM access for the emulator.

- [ ] **Virtual File System (VFS) layer**
  - Common API for all FS drivers.
- [ ] **RAM-based initrd**
  - Temporary in-memory FS for user binaries and test data.
- [ ] **File API**
  - `open`, `read`, `write`, `close`, `stat`.
- [ ] **Disk driver (optional)**
  - Start with ATA/IDE PIO.

---

## 🧰 Phase 4 — Userland foundation
> Goal: Minimal shell and toolchain support.

- [ ] **libc port (Newlib or musl)**
  - Implement low-level syscall bindings.
- [ ] **Shell**
  - Interactive shell running via syscalls.
- [ ] **Basic utilities**
  - `cat`, `ls`, `echo`, `cp`, `sleep`, etc.
- [ ] **Cross-compiler toolchain**
  - Build `binutils` + `gcc` targeting `beaneros` (`x86_64-beaner-elf`).
  - Build kernel and user programs with it.
- [ ] **User binary packaging**
  - Add Makefile rule to build `initrd` with ELF binaries.

---

## 🎮 Phase 5 — Game Boy emulator (userspace app)
> Goal: Run Pokémon Red via a custom emulator in the terminal.

### Emulator Core
- [ ] **CPU (LR35902)** — implement all opcodes.
- [ ] **Memory Management Unit (MMU)** — ROM/RAM banking, I/O, HRAM.
- [ ] **Timer & Interrupts** — sync with PIT or host timer.
- [ ] **Cartridge support** — MBC1, MBC3 for Pokémon Red.

### Display
- [ ] **PPU (pixel renderer)** — render 160×144 GB framebuffer.
- [ ] **Terminal output**
  - Convert framebuffer to ASCII shades for VGA text output.
  - Optional: color via VGA attribute bytes.

### Input
- [ ] Map keyboard keys → Game Boy buttons (A, B, Start, Select, D-pad).

### File I/O
- [ ] Load `.gb` ROM from filesystem (`/roms/pokemon_red.gb`).
- [ ] Implement save file support (battery SRAM → file).

### Timing
- [ ] Cycle-accurate frame timing (60 FPS target).

---

## 🧪 Phase 6 — Test & polish
- [ ] **Run Blargg test ROMs** to verify CPU correctness.
- [ ] **Stability tests:** long sessions, multiple processes.
- [ ] **Optimize memory usage and syscall overhead.**
- [ ] **Add config file or boot parameter support.**

---

## 🏁 End Goal
✅ Boot beanerOS kernel  
✅ Load userland  
✅ Run Game Boy emulator as a userspace binary  
✅ Play **Pokémon Red** in the terminal (VGA ASCII mode)  

---

## 📚 References
- [Pan Docs (Game Boy hardware reference)](https://gbdev.io/pandocs/)
- [xv6 source (MIT)](https://github.com/mit-pdos/xv6-public)
- [Little OS Book](https://littleosbook.github.io/)
- [SerenityOS](https://github.com/SerenityOS/serenity)
- [Blargg’s test ROMs](https://github.com/retrio/gb-test-roms)

---

*Author: [@BeanieMen](https://github.com/BeanieMen)*  
*Roadmap generated with ❤️ to guide beanerOS toward full userland and emulation support.*
