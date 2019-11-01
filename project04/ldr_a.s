.global ldr_a
.func ldr_a

ldr_a:
   // sub sp, sp, #16
   // str r0, [sp, #4]
    ldr r0, [r1]
    bx lr
