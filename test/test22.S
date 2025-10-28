#=================================================
# 2 - RELOCATABLE LINKING TEST, file 2/3
#=================================================

.extern file1Var
.global file2Var, file2Code

.section text

file2Code:
#Increment file1Var
ld file1Var, %r1
ld $1, %r2
add %r2, %r1
st %r1, file1Var
ret

file2Var:
.word 2


