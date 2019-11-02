.global quadratic_a
.func quadratic_a

quadratic_a:
    /* Initialize r12 to 0 */
    mov r12, #0
    mul r12, r2, r0
    add r3, r12, r3
    
    mov r12, r0
    mul r12, r0, r12
    mul r0, r12, r1    
    add r0, r3, r0    
bx lr
