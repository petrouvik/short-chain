#=================================================
# 3 - DIRECTIVES TEST, file 1/2
#=================================================
# Expected output: check the files test41.o.txt and program.hex.txt
#=================================================
#run with: ./assembler -o test41.o test41.S; ./assembler -o test42.o test42.S; ./linker -hex -o program.hex -place=text@0x40000000 test41.o test42.o;

.equ five, four + 1
.equ four, 4
.equ six, four + 2
.equ seven, label2 - label1 - (five - four)
.extern externUndefined, externDefined #Symbols which are extern and undefined -> global. Symbols which are extern and defined -> local
.global globalUndefined, globalDefined #Symbols which are marked global are allowed to be undefined

.section text
externDefined:
globalDefined:
label1:
.skip 8 #First 8 bytes should be zero
label2:
.word 1, 0x2, 3, four, five, six, seven, label2, label1 

.end
.word 0xFFFFFFFF
.word 0xFFFFFFFF

You should be able to write anything after the .end directive!

Memory should look like:
00 00 00 00 00 00 00 00
01 00 00 00 02 00 00 00
03 00 00 00 04 00 00 00
05 00 00 00 06 00 00 00
07 00 00 00 08 00 00 40
00 00 00 40


