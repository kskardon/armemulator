.global strb_a
.func strb_a

strb_a:
    sub sp, sp, #16
    strb r1, [sp, #4]
    ldrb r0, [sp, #4]
    bx lr
