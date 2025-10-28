# ShortChain — A Simple Assembler, Linker, and Emulator Toolchain

ShortChain is a compact educational toolchain consisting of a simple assembler, linker, and emulator.
It demonstrates the fundamental concepts behind the compilation process — from translating assembly code to binary, linking object files into executables, and finally running them in an emulated environment.

This project is not a replacement for the GNU toolchain, but rather an introduction to its core ideas.
It is intended for learning purposes, providing a transparent and minimal example of how assembly, linking, and execution work under the hood.

---

## Features

* **Assembler** — Translates custom assembly source code into object files.
* **Linker** — Combines one or more object files into a single executable binary.
* **Emulator** — Executes the linked binaries in a simulated CPU environment.
* **Readable output files** — In addition to binary files, each stage can generate a human-readable text version of its output, useful for debugging and understanding the process.
---

## Requirements

To build ShortChain, you need the following tools installed on your system:

* **g++** (C++17 or newer)
* **flex**
* **bison**
* **make** (optional, but recommended)

On Debian/Ubuntu-based systems:

```bash
sudo apt install g++ flex bison make
```

On Arch Linux:

```bash
sudo pacman -S base-devel flex bison
```

---

## Building

Clone the repository and build the project using `make`:

```bash
git clone https://github.com/petrouvik/short-chain
cd shortchain
make
```

This will produce three binaries in the `out/` directory:

* `assembler`
* `linker`
* `emulator`

---

## Usage

### 1. Assembling

The assembler translates a `.s` file into an object file (`.o`) and a human-readable listing file (`.txt`).

```bash
./out/assembler program.s -o program.o
```

Output:

* `program.o` — binary object file used by the linker
* `program.o.txt` — text representation of the object file

For more information about available options, run:

```bash
./out/assembler -h
```

---

### 2. Linking

The linker combines one or more object files into an executable binary.

```bash
./out/linker program.o other.o -hex -o executable.hex -place=text@0x40000000
```

Output:

* `executable.hex` — binary executable file used by the emulator
* `executable.hex.txt` — human-readable representation of the linked binary

For more information about available options, run:

```bash
./out/linker -h
```

---

### 3. Emulation

The emulator executes the linked binary file:

```bash
./out/emulator executable.hex
```

For more information about available options, run:

```bash
./out/emulator -h
```

---

Detailed documentation about usage of these commands can be found in the `docs/` directory.

---

## Outputs and Internals

Each tool in the chain produces both a **binary file** (for machine use) and a **text file** (for humans).
These text files reveal the internal representation of the binary — including symbol tables, relocation entries, and instruction encodings — making the process transparent and easy to follow.

This dual-output approach helps users see exactly what happens during assembly and linking, something that is normally hidden when using real-world toolchains.

---

## Why ShortChain?

ShortChain focuses on **clarity and simplicity** rather than performance or completeness.
It’s a minimal, readable, and fully functional example of a real toolchain — ideal for students, educators, and anyone curious about low-level software development.

Unlike large toolchains such as GCC, Short Chain’s components are small enough to study in full, making it easy to understand how:

* Assembly is translated into machine code and how the object files are constructed
* Symbols and relocations are resolved by the linker
* Instructions are fetched and executed in an emulated CPU

---

## Documentation

More details about the instruction set, memory layout, and CPU architecture can be found in the [docs/](docs/) directory:
* [Assembler](docs/assembler.md)
  * [Assembler instructions](docs/instructions.md)
  * [Assembler directives](docs/directives.md)
* [Object file format](docs/shelf.md)
* [Linker](docs/linker.md)
* [Emulator](docs/emulator.md)
  * [Architecture Overview](docs/architecture.md)


---

## License

This project is licensed under the **GNU General Public License v3.0 (GPL-3.0)**.
You are free to use, modify, and distribute this software under the terms of the GPL.
See the [LICENSE](LICENSE) file for the full license text.

