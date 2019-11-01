.global ldr_a
.func ldr_a

ldr_a:
    ldr r1, [r2, #1]
    ldr r0, [r1, #4]
    ldr r0, [r1]
    ldr r0, [r1, r0]
    ldr r0, [r2, #-1]
    bx lr
