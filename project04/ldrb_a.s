.global ldrb_a
.func ldrb_a

ldrb_a:
    sub sp, sp, #16
    str r1, [sp, #8]
    ldrb r0, [sp, #8]
    bx lr
