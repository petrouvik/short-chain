#=================================================
# 0 - ALL INSTRUCTION-OPERAND COMIBNATIONS TEST
#=================================================

#run with: ./assembler -o test01.o test01.S; ./linker -o program.hex -hex -place=text@0x40000000 -place=handlerSection@0 -place=sectionAt0x1000@0x1000 -place=sectionAt0x2000@0x2000 -place=sectionAt0x3000@0x3000 -place=sectionAt0x4000@0x4000 -place=sectionAt0x5000@0x5000 -place=sectionAt0x7000@0x7000 -place=sectionAt0x8000@0x8000 test01.o; ./emulator program.hex



.section text
#Set the stack pointer
ld $0xFFFFFEFE, %sp

#-----------------------------------
# Interrupting and calling test
#-----------------------------------
# Expected output: 012
#-----------------------------------
#Set the interrupt handler routine (and test csrwr)
ld $handler, %r1
csrwr %r1, %handler

#Set %r1 to 0 and test .equ
.equ zero, (l2 - l1) - (-(l2 + l2) + (l1 + l1)) - 23 + 3 - (-20)
l1:
l2:
ld $zero, %r1
call printR1 #Output should be: 0

#Test int (also test call <symbol>)
int
call printR1 #Output should be: 1

#Test call <literal>
call 0x1000
.section sectionAt0x1000 #Use -place=sectionAt0x1000@0x1000
ld $2, %r1
ret
.section text
call printR1 #Output should be: 2

#-----------------------------------
# Jump and branch tests
#-----------------------------------
# Expected output: 34567812
#-----------------------------------

#Test jmp <literal>
jmp 0x2000
ld $99, %r1     #should be skipped
.section sectionAt0x2000 #Use -place=sectionAt0x2000@0x2000
ld $3, %r1
jmp jmpReturn
.section text
jmpReturn: call printR1 #Output should be: 3

#Test jmp <symbol>
ld $4, %r1
jmp jmpTarget
ld $99, %r1 #should be skipped
jmpTarget:
call printR1 #Output should be: 4

#Test beq %gpr1, %gpr2, <literal>
ld $5, %r1
ld $5, %r2
beq %r1, %r2, 0x3000
ld $99, %r1     #should be skipped
.section sectionAt0x3000 #Use -place=sectionAt0x3000@0x3000
jmp beqReturn
.section text
beqReturn:call printR1 #Output should be: 5

#Test beq %gpr1, %gpr2, <symbol>
ld $6, %r1
ld $6, %r2
beq %r1, %r2, beqTarget
ld $99, %r1
beqTarget:
call printR1 #Output should be: 6

#Test bne %gpr1, %gpr2, <literal>
ld $7, %r1
ld $8, %r2
bne %r1, %r2, 0x4000
ld $99, %r1
.section sectionAt0x4000 #Use -place=sectionAt0x4000@0x4000
jmp bneReturn
.section text
bneReturn: call printR1 #Output should be: 7

#Test bne %gpr1, %gpr2, <symbol>
ld $8, %r1
ld $9, %r2
bne %r1, %r2, bneTarget
ld $99, %r1
bneTarget:
call printR1 #Output should be: 8

#Test bgt %gpr1, %gpr2, <literal>
ld $1, %r1
ld $0, %r2
bgt %r1, %r2, 0x5000
ld $99, %r1
.section sectionAt0x5000 #Use -place=sectionAt0x5000@0x5000
jmp bgtReturn
.section text
bgtReturn: call printR1 #Output should be: 1

#Test bgt %gpr1, %gpr2, <symbol>
.equ minusFourtyTwo, -42
ld $2, %r1
ld $minusFourtyTwo, %r2
bgt %r1, %r2, bgtTarget
ld $99, %r1
bgtTarget:
call printR1 #Output should be: 2

#-----------------------------------
# Stack operations
#-----------------------------------
# Expected output: 3
#-----------------------------------
ld $3, %r1
push %r1
ld $99, %r1
pop %r1
call printR1 #Output should be: 3

#-----------------------------------
# Arithmetic and logic tests
#-----------------------------------
# Expected output: 2322224?;83
#-----------------------------------
ld $1, %r1
ld $2, %r2

xchg %r2, %r1
call printR1 #Output: 2 (%r1 = 2, %r2 = 1)

add %r2, %r1
call printR1 #Output: 3 (%r1 = 3, %r2 = 1)

sub %r2, %r1
call printR1 #Output: 2 (%r1 = 2, %r2 = 1)

mul %r2, %r1
call printR1 #Output: 2 (%r1 = 2, %r2 = 1)

div %r2, %r1
call printR1 #Output: 2 (%r1 = 2, %r2 = 1)

ld $0xFFFFFFFD, %r1 #load ~0x2 into %r1
not %r1
call printR1 #Output: 2

ld $0xC, %r1 # 0b1100
ld $0x7, %r2 # 0b0111
and %r2, %r1
call printR1 #Output: 4

ld $0xC, %r1 # 0b1100
ld $0x7, %r2 # 0b0111
or %r2, %r1
call printR1 #Output: '?'

ld $0xC, %r1 # 0b1100
ld $0x7, %r2 # 0b0111
xor %r2, %r1
call printR1 #Output: ';'

ld $1, %r1
ld $3, %r2
shl %r2, %r1
call printR1 #Output: 8

ld $0xF, %r1
ld $2, %r2
shr %r2, %r1
call printR1 #Output: 3
#-----------------------------------
# Memory access tests
#-----------------------------------
# Expected output: 4566788812345
#-----------------------------------

#Test ld $<literal>, %gpr
ld $4, %r1
call printR1 #Output: 4

#Test ld $<symbol>, %gpr
.equ ldSymbol, 5
ld $ldSymbol, %r1
call printR1 #Output: 5

#Test ld <literal>, %gpr
ld 0x7000, %r1
.section sectionAt0x7000 #Use -place=sectionAt0x7000@0x7000
ldMemory:
.word 6
.word 8
.section text
call printR1 #Output: 6

#Test ld <symbol>, %gpr
ld ldMemory, %r1
call printR1 #Output: 6

#Test ld <gpr>, %gpr
ld $7, %r2
ld %r2, %r1
call printR1 #Output: 7

#Test ld [<gpr>], %gpr
ld $0x7004, %r2
ld [%r2], %r1
call printR1 #Output: 8

#Test ld [<gpr> + <literal>], %gpr
ld $0x7000, %r2
ld [%r2 + 4], %r1
call printR1 #Output: 8

#Test ld [<gpr> + <absSymbol>], %gpr
.equ four, 4
ld $0x7000, %r2
ld [%r2 + four], %r1
call printR1 #Output: 8

#Test st %gpr, <literal>
.section sectionAt0x8000 #Use -place=sectionAt0x8000@0x8000
storeMemory:
.skip 8
.section text
ld $1, %r1

st %r1, 0x8000

ld $99, %r1
ld 0x8000, %r1
call printR1 #Output: 1

#Test st %gpr, <symbol>
ld $2, %r1

st %r1, storeMemory

ld $99, %r1
ld 0x8000, %r1
call printR1 #Output: 2

#Test st %gpr, [<gpr>]
ld $3, %r1
ld $0x8000, %r2

st %r1, [%r2]

ld $99, %r1
ld 0x8000, %r1
call printR1 #Output: 3


#Test st %gpr, [<gpr> + <literal>]
ld $4, %r1
ld $0x8000, %r2

st %r1, [%r2 + 4]

ld $99, %r1
ld 0x8004, %r1
call printR1 #Output: 4

#Test st %gpr, [<gpr> + <absSymbol>]
ld $5, %r1
ld $0x8000, %r2

st %r1, [%r2 + four]

ld $99, %r1
ld 0x8004, %r1
call printR1 #Output: 5

#-----------------------------------
# CSR and control
#-----------------------------------
# Expected output: 0
#-----------------------------------
csrrd %handler, %r1
call printR1 #Output: 0

#Test halt
halt

call printR1 #shouldn't be reachable

#-----------------------------------
# Interrupt handler section
#-----------------------------------
.section handlerSection #place at address 0
handler:
ld $1, %r1
iret


#-----------------------------------
# Utility routines
#-----------------------------------
.section text
.equ charBase, 48
.equ term_out, 0xFFFFFF00

printR1:
    push %r2
    ld $charBase, %r2
    add %r1, %r2
    st %r2, term_out
    pop %r2
    ret
.end
