.global beq_a
.func beq_a

beq_a:
    cmp r0, r1
    beq end

loop:
    add r0, r1

end:
    bx lr
