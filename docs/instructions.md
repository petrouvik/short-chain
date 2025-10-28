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


