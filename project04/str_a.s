.global str_a
.func str_a

str_a:
    sub sp, sp, #16
    str r1, [sp, #4]
    ldr r0, [sp, #4]
    bx lr
