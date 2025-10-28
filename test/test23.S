#=================================================
# 2 - RELOCATABLE LINKING TEST, file 3/3
#=================================================

.extern file1Var, file2Var
.global printSum
.equ term_out, 0xFFFFFF00
.equ charBase, 48

.section text
printSum:
ld file1Var, %r1
ld file2Var, %r2
add %r2, %r1
ld $charBase, %r3
add %r3, %r1
st %r1, term_out
ret






