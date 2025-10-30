# Assembler Directives

Assembler directives are commands recognized by the assembler but are not translated into machine instructions.
They are used to control the assembly process, allocate memory, and define symbols and sections.

---

## `.global <symbol_list>`

Exports one or more symbols, making them visible and accessible to the linker and other object files.

* **Parameters:** a comma-separated list of symbol names.
* **Example:**

  ```asm
  .global main, loop_start
  ```

---

## `.extern <symbol_list>`

Declares one or more symbols as external — meaning they are defined in another module or object file.

* **Parameters:** a comma-separated list of symbol names.
* **Example:**

  ```asm
  .extern printf, malloc
  ```

---

## `.section <section_name>`

Starts a new assembly section. The previously active section is automatically closed.
Each section can have an arbitrary name provided as the directive parameter.

* **Common sections:** `.text`, `.data`, `.bss`, or custom names.
* **Example:**

  ```asm
  .section .data
  ```

---

## `.word <symbol_or_literal_list>`

Allocates memory for one or more 4-byte words, initializing them with the provided values.
Each initializer can be a **literal** or a **symbol**.

* **Parameters:** a comma-separated list of literals or symbols.
* **Example:**

  ```asm
  .word 0x1234, counter, 42
  ```

---

## `.skip <literal>`

Allocates a block of memory of the specified size (in bytes).
The allocated space is automatically initialized to zero.

* **Parameter:** a single integer literal.
* **Example:**

  ```asm
  .skip 128
  ```

---

## `.ascii <string>`

Allocates one byte for each character in the provided string literal.
The allocated space is initialized with ASCII values of the string’s characters.

* **Parameter:** a string enclosed in quotation marks.
* **Example:**

  ```asm
  .ascii "Hello, world!"
  ```

---

## `.equ <new_symbol>, <expression>`

Defines a new symbol whose value equals the result of the given expression.
This is similar to defining a constant or macro. 

Expressions may contain an arbitrary number of **literals** and/or **symbols**. 

Supported operators are binary `+` and `-`, and unary `-`. Parentheses can also be used.

The result of the expression must be known during assembly - if there is a non-absolute symbol in the expression (a symbol which isn't defined through `.equ`), then
it must cancel out with a symbol from the same section, otherwise the assembler will see this as an error.
* **Parameters:**

  * `<new_symbol>` — name of the symbol to define.
  * `<expression>` — a constant expression or calculation.
* **Example:**

  ```asm
  .equ BUFFER_SIZE, 1024
  .equ STRING_SIZE, stringEnd - stringStart
  ```

---

## `.end`

Marks the end of the assembly source file.
The assembler stops processing at this point, ignoring any remaining text.

* **Example:**

  ```asm
  .end
  ```

---
