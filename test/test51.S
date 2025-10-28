#=================================================
# 5 - Interrupts test
#=================================================
# Expected output: "SI" and when the user presses 'h' or 'l', to increase or decrease the timer delay, timer will start writing "T". When the user presses 'e' the program should stop.
#=================================================
#run with: ./assembler -o test51.o test51.S; ./linker -hex -o program.hex test51.o -place=text@0x40000000; ./emulator program.hex;
.equ term_out, 0xFFFFFF00
.equ term_in, 0xFFFFFF04
.equ tim_cfg, 0xFFFFFF10

.equ I, 'I'
.equ T, 'T'
.equ S, 'S'

.section text
#Set the stack pointer
ld $0xFFFFFEFE, %sp

#Set the interrupt handler
ld $handler, %r2
csrwr %r2, %handler

#Software interrupt
int

#Illegal instruction interrupt (division by 0)
ld %r0, %r1
div %r1, %r0


#Endless loop
loop: beq %r0, %r0, loop

handler:
    push %r1
    push %r2

    csrrd %cause, %r1

    ld $1, %r2
    beq %r1, %r2, ILLEGAL

    ld $2, %r2
    beq %r1, %r2, TIMER

    ld $3, %r2
    beq %r1, %r2, TERMINAL

    ld $4, %r2
    beq %r1, %r2, SOFTWARE


END:pop %r2
    pop %r1
    iret

ILLEGAL:
    #Print out I
    ld $I, %r1
    st %r1, term_out
    jmp END

TIMER:
    #Print out T
    ld $T, %r1
    st %r1, term_out
    jmp END

TERMINAL:
    #Print out key
    ld term_in, %r1
    st %r1, term_out

    #If key is h, increase timer delay
    ld $'h', %r2
    bne %r1, %r2, NOTh
    
    ld tim_cfg, %r1
    ld $1, %r2
    add %r2, %r1
    st %r1, tim_cfg

    jmp END


NOTh:
    #If key is l, decrease timer delay
    ld $'l', %r2
    bne %r1, %r2, NOTl
    
    ld tim_cfg, %r1
    ld $1, %r2
    sub %r2, %r1
    st %r1, tim_cfg

    jmp END
NOTl:    
    ld $'e', %r2
    bne %r1, %r2, END
    halt #end program if user presses 'e'

SOFTWARE:
    #Print out S
    ld $S, %r1
    st %r1, term_out
    jmp END
