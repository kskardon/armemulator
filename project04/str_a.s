.global str_a
.func str_a

str_a:
    sub sp, sp, #16
    str r1, [sp, r0]
    ldr r0, [sp, #8]
    bx lr
