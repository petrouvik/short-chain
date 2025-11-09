# Assembler instruction set

This document lists available instructions that the assembler can recognize and encode.
Each entry below lists the **instruction format** and its **effect** in pseudo-code.

---

| Mnemonic  | Format                      | Description                                                                                                         |
|-----------|-----------------------------|-------------------------------------------------------------------------------------------------------------------- |
| **halt**  | `halt`                      | Stops program execution.                                                                                            |
| **int**   | `int`                       | Triggers a software interrupt.                                                                                      |
| **iret**  | `iret`                      | Pops `pc` and `status` from the stack (returns from interrupt).                                                     |
| **call**  | `call operand`              | Pushes the current `pc` onto the stack and jumps to `operand`. <br> `push pc; pc ← operand;`                        |
| **ret**   | `ret`                       | Pops `pc` from the stack. <br> `pop pc;`                                                                            |
| **jmp**   | `jmp operand`               | Unconditional jump to `operand`. <br> `pc ← operand;`                                                               |
| **beq**   | `beq %gpr1, %gpr2, operand` | Branches to `operand` if registers are equal. <br> `if (gpr1 == gpr2) pc ← operand;`                                |
| **bne**   | `bne %gpr1, %gpr2, operand` | Branches to `operand` if registers are not equal. <br> `if (gpr1 != gpr2) pc ← operand;`                            |
| **bgt**   | `bgt %gpr1, %gpr2, operand` | Branches to `operand` if `gpr1` is greater than `gpr2` (signed). <br> `if (gpr1 > gpr2) pc ← operand;`              |
| **push**  | `push %gpr`                 | Decrements the stack pointer and stores a register value to the stack. <br> `sp ← sp - 4; mem32[sp] ← gpr;`         |
| **pop**   | `pop %gpr`                  | Loads a value from the stack into a register and increments the stack pointer. <br> `gpr ← mem32[sp]; sp ← sp + 4;` |
| **xchg**  | `xchg %gprS, %gprD`         | Exchanges the values of two registers. <br> `temp ← gprD; gprD ← gprS; gprS ← temp;`                                |
| **add**   | `add %gprS, %gprD`          | Adds source to destination register. <br> `gprD ← gprD + gprS;`                                                     |
| **sub**   | `sub %gprS, %gprD`          | Subtracts source from destination register. <br> `gprD ← gprD - gprS;`                                              |
| **mul**   | `mul %gprS, %gprD`          | Multiplies destination by source. <br> `gprD ← gprD * gprS;`                                                        |
| **div**   | `div %gprS, %gprD`          | Divides destination by source (integer division). <br> `gprD ← gprD / gprS;`                                        |
| **not**   | `not %gpr`                  | Bitwise negation. <br> `gpr ← ~gpr;`                                                                                |
| **and**   | `and %gprS, %gprD`          | Bitwise AND. <br> `gprD ← gprD & gprS;`                                                                             |
| **or**    | `or %gprS, %gprD`           | Bitwise OR. <br> `gprD ← gprD \| gprS;`                                                                             |
| **xor**   | `xor %gprS, %gprD`          | Bitwise XOR. <br> `gprD ← gprD ^ gprS;`                                                                             |
| **shl**   | `shl %gprS, %gprD`          | Logical left shift. <br> `gprD ← gprD << gprS;`                                                                     |
| **shr**   | `shr %gprS, %gprD`          | Logical right shift. <br> `gprD ← gprD >> gprS;`                                                                    |
| **ld**    | `ld operand, %gpr`          | Loads a value from memory or an immediate operand into a register. <br> `gpr ← operand;`                            |
| **st**    | `st %gpr, operand`          | Stores a register value into memory or an operand. <br> `operand ← gpr;`                                            |
| **csrrd** | `csrrd %csr, %gpr`          | Reads a control/status register into a general-purpose register. <br> `gpr ← csr;`                                  |
| **csrwr** | `csrwr %gpr, %csr`          | Writes a general-purpose register into a control/status register. <br> `csr ← gpr;`                                 |

---

## Operands

The term **`%gprX`** refers to one of the **general-purpose registers (GPRs)** of the target architecture.
The available general-purpose registers are:

```
r0, r1, r2, r3, r4, r5, r6, r7,
r8, r9, r10, r11, r12, r13, r14 (sp), r15 (pc)
```

* `r14` serves as the **stack pointer (`sp`)**.
* `r15` serves as the **program counter (`pc`)**.

The term **`%csrX`** refers to one of the **control and status registers (CSRs)** of the architecture.
The available control and status registers are:

```
status, handler, cause
```

---

### Operand Notation

The term **operand** covers all syntactic forms used to specify data or jump targets in assembly instructions.
The notation differs depending on whether the instruction operates on **data** or performs a **jump/call**.

---

#### 1. Data Instructions

Data-handling instructions support the following operand notations, which define the *value of the data* being accessed or used:

| Notation               | Meaning                                                                  |
| ---------------------- | ------------------------------------------------------------------------ |
| `$<literal>`           | The constant value `<literal>`.                                          |
| `$<symbol>`            | The value associated with the symbol `<symbol>`.                         |
| `<literal>`            | The value stored in memory at address `<literal>`.                       |
| `<symbol>`             | The value stored in memory at the address of `<symbol>`.                 |
| `%<reg>`               | The value stored in register `<reg>`.                                    |
| `[%<reg>]`             | The value stored in memory at the address contained in register `<reg>`. |
| `[%<reg> + <literal>]` | The value stored in memory at the address `<reg> + <literal>`.           |
| `[%<reg> + <symbol>]`  | The value stored in memory at the address `<reg> + <symbol>`.            |

---

#### 2. Control-Flow Instructions

Jump and subroutine call instructions support the following operand forms, which define the *destination address* of the jump:

| Notation    | Meaning                                             |
| ----------- | --------------------------------------------------- |
| `<literal>` | The immediate jump address `<literal>`.             |
| `<symbol>`  | The address corresponding to the symbol `<symbol>`. |


---

## Instruction Encoding

### Overview

Each instruction in the target architecture is encoded into **4 bytes (32 bits)**.
The assembler translates each *mnemonic* into one or more such **machine instructions**, depending on the operation.
This encoding is intentionally compact and regular — but some high-level mnemonics expand into multiple instructions because the target architecture does not support them as single hardware operations.

### General Encoding Format

All 4-byte instructions share the same basic layout:

| Bits  | Field | Description                                   |
| ----- | ----- | --------------------------------------------- |
| 31–28 | `oc`  | Operation code (opcode)                       |
| 27–24 | `mod` | Modifier or sub-operation                     |
| 23–20 | `a`   | Register A                                    |
| 19–16 | `b`   | Register B                                    |
| 15–12 | `c`   | Register C                                    |
| 11–0  | `d`   | 12-bit signed displacement or immediate value |

In memory, these fields are packed into four bytes as follows:

| Byte | Contents                           |
| ---- | -----------------------------------|
| 0    | `(oc << 4) \| mod`                 |
| 1    | `(a << 4) \| b`                    |
| 2    | `(c << 4) \| ((d >> 8) & 0x0F)`    |
| 3    | `(d & 0xFF)`                       |

The displacement (`d`) is a signed 12-bit value, meaning it can range from **−2048 to +2047**.
Values outside this range cause an assembly-time error.

---

### Multi-Instruction Expansions

Not all assembly mnemonics map directly to one hardware instruction.
The assembler sometimes expands a single mnemonic into several encoded instructions to emulate higher-level behavior.

For example:

* **`call`** is encoded as two instructions (stack push and branch) followed by a 4-byte address literal.
* **`jmp`** consists of one instruction plus a 4-byte address.
* **`ld`** and **`st`** with symbol or memory operands may generate multiple machine instructions and relocations.

This is similar to how certain pseudo-instructions in RISC-V or MIPS expand into real instructions behind the scenes.

---

### Encodings

The table below summarizes mnemonics and how they translate into machine code.

| Mnemonic            | Effective Translation                                                                                   | Notes / Encoding Pattern                                             |
| ------------------- | ------------------------------------------------------------------------------------------------------- | -------------------------------------------------------------------- |
| **halt**            | `makeInstruction(0x0,0x0,0x0,0x0,0x0,0x000)`                                                            | Encodes to a single 4-byte word of all zeros.                        |
| **intr**            | `makeInstruction(0x1,0x0,0x0,0x0,0x0,0x000)`                                                            | Software interrupt trigger instruction.                              |
| **iret**            | `makeInstruction(0x9,0x6,STATUS,SP,0x0,0x004)` + `makeInstruction(0x9,0x3,PC,SP,0x0,0x008)`             | Restores STATUS and PC from stack; expands to 2 instructions.        |
| **call addr**       | `makeInstruction(0x2,0x1,PC,0x0,0x0,0x004)` + `makeInstruction(0x3,0x0,PC,0x0,0x0,0x004)` + `<address>` | Pushes PC and jumps; When returning, the execution will jump *over* the target address literal.   |
| **ret**             | `makeInstruction(0x9,0x3,PC,SP,0x0,0x004)`                                                              | Pops PC from stack; returns from subroutine.                         |
| **jmp addr**        | `makeInstruction(0x3,0x8,PC,0x0,0x0,0x000)` + `<address>`                                               | Unconditional jump; includes 4-byte address literal.                 |
| **beq r1,r2,addr**  | `makeInstruction(0x3,0x9,PC,r1,r2,0x004)` + `makeInstruction(0x3,0x0,PC,0x0,0x0,0x004)` + `<address>`   | Conditional branch if equal; expands to 2 instructions plus address. |
| **bne r1,r2,addr**  | `makeInstruction(0x3,0xA,PC,r1,r2,0x004)` + `makeInstruction(0x3,0x0,PC,0x0,0x0,0x004)` + `<address>`   | Conditional branch if not equal.                                     |
| **bgt r1,r2,addr**  | `makeInstruction(0x3,0xB,PC,r1,r2,0x004)` + `makeInstruction(0x3,0x0,PC,0x0,0x0,0x004)` + `<address>`   | Conditional branch if greater.                                       |
| **push r**          | `makeInstruction(0x8,0x1,SP,0x0,r,-4)`                                                                  | Decrements stack pointer and stores register value.                  |
| **pop r**           | `makeInstruction(0x9,0x3,r,SP,0x0,0x004)`                                                               | Loads from stack into register and increments SP.                    |
| **xchg rS,rD**      | `makeInstruction(0x4,0x0,0x0,rS,rD,0x000)`                                                              | Exchanges contents of two registers.                                 |
| **add rS,rD**       | `makeInstruction(0x5,0x0,rD,rD,rS,0x000)`                                                               | Adds source to destination.                                          |
| **sub rS,rD**       | `makeInstruction(0x5,0x1,rD,rD,rS,0x000)`                                                               | Subtracts source from destination.                                   |
| **mul rS,rD**       | `makeInstruction(0x5,0x2,rD,rD,rS,0x000)`                                                               | Multiplies destination by source.                                    |
| **div rS,rD**       | `makeInstruction(0x5,0x3,rD,rD,rS,0x000)`                                                               | Divides destination by source.                                       |
| **not r**           | `makeInstruction(0x6,0x0,r,r,0x0,0x000)`                                                                | Bitwise NOT.                                                         |
| **and rS,rD**       | `makeInstruction(0x6,0x1,rD,rD,rS,0x000)`                                                               | Bitwise AND.                                                         |
| **or rS,rD**        | `makeInstruction(0x6,0x2,rD,rD,rS,0x000)`                                                               | Bitwise OR.                                                          |
| **xor rS,rD**       | `makeInstruction(0x6,0x3,rD,rD,rS,0x000)`                                                               | Bitwise XOR.                                                         |
| **shl rS,rD**       | `makeInstruction(0x7,0x0,rD,rD,rS,0x000)`                                                               | Logical shift left.                                                  |
| **shr rS,rD**       | `makeInstruction(0x7,0x1,rD,rD,rS,0x000)`                                                               | Logical shift right.                                                 |
| **ld imm, r**        | `makeInstruction(0x9,0x3,r,PC,0x0,0x004)` + `<imm>`                                                    | Loads an immediate value into register.                             |
| **ld addr, r**     | `makeInstruction(0x9,0x3,r,PC,0x0,0x004)` + `<address>` + `makeInstruction(0x9,0x2,r,r,0x0,0x000)`       | Loads memory contents from absolute address.                         |
| **ld reg, r**        | `makeInstruction(0x9,0x1,r,reg,0x0,0x000)`                                                             | Loads from register.                                                 |
| **ld [reg], r**      | `makeInstruction(0x9,0x2,r,reg,0x0,0x000)`                                                             | Loads value from memory address stored in register.                  |
| **ld [reg+disp], r** | `makeInstruction(0x9,0x2,r,reg,0x0,disp)`                                                              | Loads from memory address computed as register + displacement.       |
| **st r,[addr]**     | `makeInstruction(0x8,0x2,PC,0x0,r,0x004)` + `makeInstruction(0x3,0x0,PC,0x0,0x0,0x004)` + `<address>`   | Stores register to absolute memory address.                          |
| **st r,[reg]**      | `makeInstruction(0x8,0x0,reg,0x0,r,0x000)`                                                              | Stores to memory pointed to by register.                             |
| **st r,[reg+disp]** | `makeInstruction(0x8,0x0,reg,0x0,r,disp)`                                                               | Stores to memory at register + displacement.                         |
| **csrrd csr,gpr**   | `makeInstruction(0x9,0x0,gpr,csr,0x0,0x000)`                                                            | Reads from control/status register.                                  |
| **csrwr gpr,csr**   | `makeInstruction(0x9,0x4,csr,gpr,0x0,0x000)`                                                            | Writes to control/status register.                                   |

