#=================================================
# 3 - ASCII AND EQU TEST
#=================================================
# Expected output: contents of string myString
#=================================================
#run with: ./assembler -o test31.o test31.S; ./linker -hex -o program.hex -place=text@0x40000000 test31.o; ./emulator program.hex

.equ term_out, 0xFFFFFF00
.equ myStringLength, myStringEnd - myString

.section text
#Set the stack pointer
ld $0xFFFFFEFE, %sp

ld $myStringLength, %r1
ld $myString, %r2
ld $0, %r3

LOOP:bgt %r1, %r3, CONTINUE
jmp END

CONTINUE:
ld %r2, %r4
add %r3, %r4
ld [%r4], %r4

st %r4, term_out

ld $1, %r5
add %r5, %r3
jmp LOOP
END: halt




.section data
myString:
.ascii "Greetings!\nThis is the ascii and equ test.\n\t-If you see this, it means everything is working fine."
myStringEnd:


