.global cmpbeq_a
.func cmpbeq_a

cmpbeq_a:
    cmp r0, r1
    beq end
    add r0, r0, r1

end:
    bx lr
