#=================================================
# 1 - TERMINAL DEVICE TEST
#=================================================
# Expected output: if the user inputs a digit it gets echoed and added to the total in r1. Execution stops when the user inputs 0.
#=================================================
#run with: ./assembler -o test11.o test11.S; ./linker -hex -o program.hex -place=text@0x40000000 test11.o; ./emulator program.hex
.equ term_out, 0xFFFFFF00
.equ term_in, 0xFFFFFF04

.equ digit0, 48

.section text
#Set the stack pointer
ld $0xFFFFFEFE, %sp


#Set r1 to 0
ld %r0, %r1

#Set the interrupt handler
ld $handler, %r2
csrwr %r2, %handler

ld $1, %r2 #load something into r2 so the loop doesn't immediately stop
loop: bne %r2, %r0, loop
halt


handler:
ld term_in, %r2
ld $digit0, %r3
sub %r3, %r2

#Check if input is a digit
bgt %r0, %r2, end
ld $9, %r3
bgt %r2, %r3, end

# Add to total
add %r2, %r1

#Print
ld $digit0, %r3
add %r2, %r3
st %r3, term_out

end:
iret
