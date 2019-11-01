.global cmpbeq_a
.func cmpbeq_a

cmpbeq_a:
    cmp r0, r1
    beq sub

add:
    add r0, r0, #3

sub:
    sub r0, r0, r1

end:
    bx lr
