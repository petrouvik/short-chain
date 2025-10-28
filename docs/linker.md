# Linker

The linker is responsible for combining multiple relocatable object files produced by the assembler into a single output file.
It can operate in two distinct modes depending on the provided options:

* **Executable (hex) mode** – produces a final, memory-mapped output file ready for the emulator.
* **Relocatable mode** – produces a merged relocatable object file that can be passed again to the linker.

---

## Overview

The linker performs the following major steps internally:

1. **Reading input files** – Each input object file (in `.o` format) is parsed.
   All section headers, symbols, relocation entries, and section contents are collected.

2. **Symbol resolution** – The linker validates global and local symbol definitions:

   * It detects and reports multiple global definitions.
   * It ensures that all referenced global symbols are defined in at least one input file.
   * Undefined local symbols cause an error.

3. **Section merging** – Sections with identical names (e.g. `.text`, `.data`) are concatenated in memory.
   The linker calculates offsets for each section’s contribution to the final layout.

4. **Address assignment** –

   * User-specified base addresses can be provided with the `-place=SECTION@ADDRESS` option.
   * Remaining sections are assigned addresses automatically so that they do not overlap.

5. **Relocation processing** –
   Each relocation entry is applied by patching the affected bytes in the merged sections.
   The relocation type determines how symbol values are adjusted (e.g. direct vs PC-relative).

---

## Input and Output

### Input Files

* One or more relocatable object files produced by the assembler (`.o` files).
* Each file must conform to the internal **SHELF** format used by this toolchain.

### Output File

The type of output depends on the selected mode:

| Mode        | Option         | Output File                                  | Description                                                                                                                       |
| ----------- | -------------- | -------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------- |
| Executable  | `-hex`         | `<output>.hex` (binary) + `<output>.hex.txt` | Produces a memory image where each line represents an address and byte value. This file can be executed directly by the emulator. |
| Relocatable | `-relocatable` | `<output>.o` (binary) + `<output>.o.txt`     | Produces a single relocatable object file containing merged sections, symbols, and relocations, suitable for further linking.     |

The `.txt` companion file is a human-readable dump automatically generated for inspection.

---

## Command-line Options

| Option                | Description                                                                                                  |
| --------------------- | ------------------------------------------------------------------------------------------------------------ |
| `-o <file>`           | Specifies the name of the output file.                                                                       |
| `-hex`                | Generates a final executable memory image for the emulator.                                                  |
| `-relocatable`        | Generates a merged relocatable object file.                                                                  |
| `-place=SECTION@ADDR` | Assigns a starting address to a named section. Can be specified multiple times. Ignored in relocatable mode. |
| `-h`                  | Displays help information.                                                                                   |

Exactly one of `-hex` or `-relocatable` **must** be provided.

---

## Usage Examples

### Produce an executable hex file

```bash
linker file1.o file2.o -o program.hex -hex -place=text@0x40000000 -place=data@0x0
```

### Produce a merged relocatable file

```bash
linker file1.o file2.o -o combined.o -relocatable
```

---

## Output Format

In **hex mode**, the linker outputs a binary file containing pairs of:

* **4 bytes**: memory address
* **1 byte**: stored value

The `.txt` version of the same file contains addresses and bytes formatted in hex for readability.
Gaps in the address space are indicated with `...`.

In **relocatable mode**, the linker writes a full relocatable object containing:

* Section headers
* Merged section data
* Updated symbol table
* Updated relocation entries

This file can be passed again to the linker to build larger programs.
