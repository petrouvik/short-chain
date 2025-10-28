# Assembler

## Overview

The assembler is the first stage of the ShortChain toolchain.
Its purpose is to translate assembly source files (`.s`) into machine-readable **object files** (`.o`), along with their human-readable **text representations** (`.txt`).
These object files can later be processed by the **linker** to produce executable binaries.

---

## One-Pass Design

Unlike traditional multi-pass assemblers, this assembler is designed as a **one-pass assembler**.

During a single traversal of the input file, it performs the following tasks:

1. **Parses assembly instructions and directives** as they appear.
2. **Builds symbol and section tables** dynamically as symbols are encountered.
3. **Records forward reference entries** for references to undefined symbols or addresses not yet known.
4. **Backpatches machine code** based on forward reference entries, after it has passed through the whole file.

This design simplifies implementation and improves assembly speed, at the cost of requiring **forward reference entries** for any forward-referenced symbols

---

## Input

The assembler expects an input file written in the project’s custom assembly syntax (usually with extension an `.s` or `.S`).
An assembly file may contain:

* **Instructions**, describing CPU operations.
* **Directives**, used to manage sections, allocate data, and define symbols.
* **Labels**, representing symbolic addresses.

The syntax and semantics of all **instructions** and **directives** are described in detail in the following documents:

* [instructions.md](instructions.md) — Supported instruction set and operand formats
* [directives.md](directives.md) — Supported assembler directives and their behavior

---

## Output

When executed, the assembler produces **two output files**:

1. **Binary object file (`.o`)**

   * Encoded in the **ShELF (Short ELF)** format
   * Contains machine code, symbol table, and relocation records
   * Used as input to the linker

2. **Human-readable dump (`.o.txt`)**

   * Text-formatted version of the binary object
   * Shows section headers, symbols, relocations, and raw data for inspection or debugging

---

## Example

```bash
./out/assembler program.s -o program.o
```

This command assembles `program.s` and produces:

* `program.o` — relocatable binary object file
* `program.o.txt` — human-readable representation of the same object file

The generated object file can then be linked using the **linker** component of the toolchain.

---

## Error Handling

The assembler performs basic syntax and semantic checks:

* Invalid instructions, directives, or operands produce descriptive error messages.
* Reports an error for undefined references that haven't been declared global or extern.
* Reports an error for references or definitions that require the value to be known during assembly time.

Errors are reported with line information, making it easier to locate issues in the source code.

---

## Summary

The assembler provides a simple but complete foundation for understanding the early stages of a compiler toolchain.
It converts textual assembly into relocatable machine code while keeping symbol and relocation data explicit and accessible.
