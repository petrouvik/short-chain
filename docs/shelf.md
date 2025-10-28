# ShELF (Short Executable and Linkable Format)

**ShELF** is a compact, simplified version of the ELF (Executable and Linkable Format), designed specifically for the educational toolchain used by this project.
It serves as an intermediate binary representation produced by the assembler and consumed by the linker.

---

## Overview

A ShELF file consists of:

1. A **ShELF header** describing the structure of the file.
2. A table of **section headers**, each describing one section of the file.
3. The **section data** itself, such as code, data, symbol tables, relocation entries, and string tables.

This design makes ShELF conceptually similar to ELF, but with greatly reduced complexity and overhead.

---

## File Layout

```
+----------------------+
| ShelfHeader          |
+----------------------+
| Section Data ...     |
+----------------------+
| Section Headers ...  |
+----------------------+
```

---

## ShelfHeader

The file begins with a fixed-size header describing where the section headers are located and how many there are.

| Field      | Type       | Description                                                             |
| ---------- | ---------- | ----------------------------------------------------------------------- |
| `magic[5]` | `uint8_t`  | File signature identifying the format (`"SHELF"`).              |
| `shoff`    | `uint32_t` | Byte offset from the beginning of the file to the section header table. |
| `shnum`    | `uint16_t` | Number of entries in the section header table.                          |
| `shstrndx` | `uint16_t` | Index of the section header string table (used for section names).      |

---

## ShelfSectionHeader

Each section of a ShELF file is described by a `ShelfSectionHeader` entry in the section header table.

| Field        | Type       | Description                                                                                                   |
| ------------ | ---------- | --------------------------------------------------------------------------------------------------------------|
| `nameOffset` | `uint32_t` | Offset into the section header string table where the section name begins.                                    |
| `type`       | `uint32_t` | Type of the section (see **ShelfSectionType** below).                                                         |
| `offset`     | `uint32_t` | File offset where the section’s data begins.                                                                  |
| `size`       | `uint32_t` | Size of the section data in bytes.                                                                            |
| `info`       | `uint32_t` | Additional section-specific information. For `SHELF_RELOC` sections it is the index of the referenced section.|
| `address`    | `uint32_t` | Load address for executable sections.                                                                         |

### ShelfSectionType

| Name              | Value | Description                                                                  |
| ----------------- | :---: | ---------------------------------------------------------------------------- |
| `SHELF_NULL`      |   0   | Unused section.                                                              |
| `SHELF_PROGBITS`  |   1   | Section containing program data or code.                                     |
| `SHELF_NOBITS`    |   2   | Section that occupies memory but has no file data (e.g. uninitialized data). |
| `SHELF_SYMTAB`    |   3   | Symbol table containing symbol definitions.                                  |
| `SHELF_STRTAB`    |   4   | String table specifically used for section names.                            |
| `SHELF_SYMSTRTAB` |   5   | String table specifically for symbol names.                                  |
| `SHELF_RELOC`     |   6   | Relocation table for a section.                                              |

---

## ShelfSymbol

Symbols represent variables, functions, and section labels. They can refer to addresses in the file or to external symbols.

| Field        | Type       | Description                                                   |
| ------------ | ---------- | ------------------------------------------------------------- |
| `nameOffset` | `uint32_t` | Offset into the symbol string table for the symbol’s name.    |
| `value`      | `uint32_t` | Symbol’s value or address.                                    |
| `size`       | `uint32_t` | Size in bytes (if applicable).                                |
| `type`       | `uint8_t`  | Symbol type (see **SymbolType**).                             |
| `bind`       | `uint8_t`  | Symbol binding (see **SymbolBind**).                          |
| `shndx`      | `uint16_t` | Section index that the symbol belongs to, or a special value. |

### SymbolType

| Name         | Value | Description                                         |
| ------------ | :---: | --------------------------------------------------- |
| `ST_NOTYPE`  |   0   | No specific type.                                   |
| `ST_ABS`     |   1   | Absolute value (does not change during relocation). |
| `ST_SECTION` |   2   | Symbol representing a section.                      |
| `ST_FUNC`    |   3   | Function symbol.                                    |
| `ST_OBJECT`  |   4   | Data object (variable).                             |

### Special Section Indexes

| Name              | Value | Description                                  |
| ----------------- | :---: | -------------------------------------------- |
| `SHELF_SHN_UNDEF` |   0   | Undefined symbol (external reference).       |
| `SHELF_SHN_ABS`   |   -1  | Absolute symbol, not affected by relocation. |

### SymbolBind

| Name         | Value | Description                               |
| ------------ | :---: | ----------------------------------------- |
| `STB_LOCAL`  |   0   | Local to the section or file.             |
| `STB_GLOBAL` |   1   | Visible to other object files (exported). |

---

## ShelfRelocation

Relocation entries describe how certain references in code or data need to be adjusted when linking or loading.

| Field      | Type       | Description                                                   |
| ---------- | ---------- | ------------------------------------------------------------- |
| `offset`   | `uint32_t` | Offset within the section to be relocated.                    |
| `symIndex` | `uint32_t` | Index of the referenced symbol in the symbol table.           |
| `type`     | `uint8_t`  | Type of relocation (see **ShelfRelocType**).                  |
| `addend`   | `int32_t`  | Constant value added to the symbol’s value during relocation. |

### ShelfRelocType

| Name             | Value | Description                                                        |
| ---------------- | :---: | ------------------------------------------------------------------ |
| `R_SHELF_NONE`   |   0   | No relocation performed.                                           |
| `R_SHELF_DIRECT` |   1   | Direct relocation — absolute address adjustment.                   |
| `R_SHELF_PC_REL` |   2   | PC-relative relocation — adjusted relative to the program counter. |

---

## Example

```asm
.global mydata
.extern extFunction, extData

.section text
ld $0xFFFFFEFE, %sp
ld $1, %r1
call extFunction
ld mydata, %r2
add %r2, %r1
ld extData, %r2
add %r2, %r1


.section data
mydata:
.word 10

.end

```
The above code will produce the following output (`.txt` file) when assembled:
```
Section Headers:
Nr  Name            Type        Size      Info      
0                   NULL        00000000  00000000
1   text            PROGBITS    0000003c  00000000
2   .relatext       RELOC       00000030  00000001
3   data            PROGBITS    00000004  00000000
4   .symtab         SYMTAB      00000060  00000000
5   .shstrtab       STRTAB      00000032  00000000
6   .symstrtab      SYMSTRTAB   00000026  00000000

#text
93 ef 00 40 fe fe ff ff 
93 1f 00 40 10 00 00 00 
21 f0 00 40 30 f0 00 40 
00 00 00 00 93 2f 00 40 
00 00 00 00 92 22 00 00 
50 11 20 00 93 2f 00 40 
00 00 00 00 92 22 00 00 
50 11 20 00 
#data
a0 00 00 00 

#.symtab
Num Value     Size      Type      Bind      Ndx       Name              
0   00000000  00000000  NOTYPE    LOCAL     UND                       
1   00000000  00000000  NOTYPE    GLOBAL    3         mydata          
2   00000000  00000000  NOTYPE    GLOBAL    UND       extFunction     
3   00000000  00000000  NOTYPE    GLOBAL    UND       extData         
4   00000000  00000000  SECTION   LOCAL     1         text            
5   00000000  00000000  SECTION   LOCAL     3         data            

#.relatext
Offset    Type              SymNdx    (SymName)         Addend    
00000020  R_SHELF_DIRECT    1         (mydata)          0       
00000018  R_SHELF_DIRECT    2         (extFunction)     0       
00000030  R_SHELF_DIRECT    3         (extData)         0       

```

