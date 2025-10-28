# CPU Architecture

This document describes how the emulated CPU decodes and executes instructions.

Each instruction is **4 bytes (32 bits)** long and consists of several fields that determine the operation, addressing mode, and operands.

---

## Instruction Format
| I               | II                | III               | IV                |
| --------------- | ----------------- | ----------------- | ----------------- |
| **OC** **MOD**  | **RegA**  **RegB**| **RegC** **Disp** | **Disp**  **Disp**|

---

| Byte | Bits | Field                        | Description                                                                                                                       |
| ---- | ---- | ---------------------------- | --------------------------------------------------------------------------------------------------------------------------------- |
| I    | 7–4  | **OC**                       | Operation code — specifies the instruction type.                                                                                  |
| I    | 3–0  | **MOD**                      | Modifier — specifies how the instruction behaves.                                                                                 |
| II   | 7–4  | **RegA**                     | Register field A — define which general-purpose or control registers are used. Each register is identified by a 4-bit index.      |
| II   | 3-0  | **RegB**                     | Register field B — define which general-purpose or control registers are used. Each register is identified by a 4-bit index.      |
| III  | 7–4  | **RegC**                     | Register field C — define which general-purpose or control registers are used. Each register is identified by a 4-bit index.      |
| III  | 3-0  | **Disp**                     | Most significant 4 bits of a 12-bit displacement value.                                                                           |
| IV   | 7–0  | **Disp**                     | Least significant 8 bits of a 12-bit displacement value.                                                                          |

---

## Register Encoding

* General-purpose registers (`r0`–`r15`) are indexed according to their names.
  Example: `r0` = 0, `r1` = 1, …, `r15` = 15.
* Control and status registers are assigned the following indices:

  * `status` = 0
  * `handler` = 1
  * `cause` = 2

---

## Instruction Overview

Below is an overview of how each instruction type is decoded and executed.

---

### Halt

|I          |II         |III        |IV         |
|-----------|-----------|-----------|-----------|
|`7654 3210`|`7654 3210`|`7654 3210`|`7654 3210`|
|`0000 0000`|`0000 0000`|`0000 0000`|`0000 0000`|

Stops the processor and halts the execution of all further instructions.

---

### Software Interrupt
|I          |II         |III        |IV         |
|-----------|-----------|-----------|-----------|
|`7654 3210`|`7654 3210`|`7654 3210`|`7654 3210`|
|`0001 0000`|`0000 0000`|`0000 0000`|`0000 0000`|

Generates a software interrupt request.

Execution:

```
push status
push pc
cause <= 4
status <= status & (~0x1)
pc <= handler
```

---

### Subroutine Call

|I          |II         |III        |IV         |
|-----------|-----------|-----------|-----------|
|`7654 3210`|`7654 3210`|`7654 3210`|`7654 3210`|
|`0010 MMMM`|`AAAA BBBB`|`0000 DDDD`|`DDDD DDDD`|

Calls a subroutine and saves the return address on the stack.

| MOD    | Behavior                                     |
| ------ | -------------------------------------------- |
| `0000` | `push pc; pc <= gpr[A] + gpr[B] + D;`        |
| `0001` | `push pc; pc <= mem32[gpr[A] + gpr[B] + D];` |

---

### Jump

|I          |II         |III        |IV         |
|-----------|-----------|-----------|-----------|
|`7654 3210`|`7654 3210`|`7654 3210`|`7654 3210`|
|`0011 MMMM`|`AAAA BBBB`|`CCCC DDDD`|`DDDD DDDD`|

Performs unconditional or conditional jumps depending on the modifier.

| MOD    | Behavior                                              |
| ------ | ----------------------------------------------------- |
| `0000` | `pc <= gpr[A] + D;`                                   |
| `0001` | `if (gpr[B] == gpr[C]) pc <= gpr[A] + D;`             |
| `0010` | `if (gpr[B] != gpr[C]) pc <= gpr[A] + D;`             |
| `0011` | `if (gpr[B] signed> gpr[C]) pc <= gpr[A] + D;`        |
| `1000` | `pc <= mem32[gpr[A] + D];`                            |
| `1001` | `if (gpr[B] == gpr[C]) pc <= mem32[gpr[A] + D];`      |
| `1010` | `if (gpr[B] != gpr[C]) pc <= mem32[gpr[A] + D];`      |
| `1011` | `if (gpr[B] signed> gpr[C]) pc <= mem32[gpr[A] + D];` |

---

### Atomic Exchange

|I          |II         |III        |IV         |
|-----------|-----------|-----------|-----------|
|`7654 3210`|`7654 3210`|`7654 3210`|`7654 3210`|
|`0100 0000`|`0000 BBBB`|`CCCC 0000`|`0000 0000`|

Atomically swaps the values of two registers.

```
temp <= gpr[B]
gpr[B] <= gpr[C]
gpr[C] <= temp
```

---

### Arithmetic Operations

|I          |II         |III        |IV         |
|-----------|-----------|-----------|-----------|
|`7654 3210`|`7654 3210`|`7654 3210`|`7654 3210`|
|`0101 MMMM`|`AAAA BBBB`|`CCCC 0000`|`0000 0000`|

Performs arithmetic operations depending on the modifier.

| MOD    | Behavior                     |
| ------ | ---------------------------- |
| `0000` | `gpr[A] <= gpr[B] + gpr[C];` |
| `0001` | `gpr[A] <= gpr[B] - gpr[C];` |
| `0010` | `gpr[A] <= gpr[B] * gpr[C];` |
| `0011` | `gpr[A] <= gpr[B] / gpr[C];` |

---

### Logical Operations

|I          |II         |III        |IV         |
|-----------|-----------|-----------|-----------|
|`7654 3210`|`7654 3210`|`7654 3210`|`7654 3210`|
|`0110 MMMM`|`AAAA BBBB`|`CCCC 0000`|`0000 0000`|

Performs logical bitwise operations depending on the modifier.

| MOD    | Behavior                     |
| ------ | ---------------------------- |
| `0000` | `gpr[A] <= ~gpr[B];`         | 
| `0001` | `gpr[A] <= gpr[B] & gpr[C];` |
| `0010` | `gpr[A] <= gpr[B] \| gpr[C];` |           
| `0011` | `gpr[A] <= gpr[B] ^ gpr[C];` |

---

### Shift Operations

|I          |II         |III        |IV         |
|-----------|-----------|-----------|-----------|
|`7654 3210`|`7654 3210`|`7654 3210`|`7654 3210`|
|`0111 MMMM`|`AAAA BBBB`|`CCCC 0000`|`0000 0000`|

Performs shift operations depending on the modifier.

| MOD    | Behavior                      |
| ------ | ----------------------------- |
| `0000` | `gpr[A] <= gpr[B] << gpr[C];` |
| `0001` | `gpr[A] <= gpr[B] >> gpr[C];` |

---

### Store
|I          |II         |III        |IV         |
|-----------|-----------|-----------|-----------|
|`7654 3210`|`7654 3210`|`7654 3210`|`7654 3210`|
|`1000 MMMM`|`AAAA BBBB`|`CCCC DDDD`|`DDDD DDDD`|

Stores data into memory depending on the modifier.

| MOD    | Behavior                                         |
| ------ | ------------------------------------------------ |
| `0000` | `mem32[gpr[A] + gpr[B] + D] <= gpr[C];`          |
| `0001` | `gpr[A] <= gpr[A] + D; mem32[gpr[A]] <= gpr[C];` |
| `0010` | `mem32[mem32[gpr[A] + gpr[B] + D]] <= gpr[C];`   |

---

### Load

|I          |II         |III        |IV         |
|-----------|-----------|-----------|-----------|
|`7654 3210`|`7654 3210`|`7654 3210`|`7654 3210`|
|`1001 MMMM`|`AAAA BBBB`|`CCCC DDDD`|`DDDD DDDD`|

Loads data into registers or control/status registers.

| MOD    | Behavior                                         |
| ------ | ------------------------------------------------ |
| `0000` | `gpr[A] <= csr[B];`                              |
| `0001` | `gpr[A] <= gpr[B] + D;`                          |
| `0010` | `gpr[A] <= mem32[gpr[B] + gpr[C] + D];`          |
| `0011` | `gpr[A] <= mem32[gpr[B]]; gpr[B] <= gpr[B] + D;` |
| `0100` | `csr[A] <= gpr[B];`                              |
| `0101` | `csr[A] <= csr[B];`                              |
| `0110` | `csr[A] <= mem32[gpr[B] + gpr[C] + D];`          |
| `0111` | `csr[A] <= mem32[gpr[B]]; gpr[B] <= gpr[B] + D;` |

---

