#=================================================
# 2 - RELOCATABLE LINKING TEST, file 1/3
#=================================================
# Expected output: 5
#=================================================
#run with: ./assembler -o test21.o test21.S;./assembler -o test22.o test22.S; ./assembler -o test23.o test23.S; ./linker -relocatable -o intermediate.o test21.o test22.o; ./linker -hex -o program.hex -place=text@0x40000000 intermediate.o test23.o; ./emulator program.hex

.extern file2Var, file2Code, printSum
.global file1Var

.section text

#Set the stack pointer
ld $0xFFFFFEFE, %sp

#Increment file2Var
ld file2Var, %r1
ld $1, %r2
add %r2, %r1
st %r1, file2Var

call file2Code

call printSum

halt

file1Var:
.word 1


