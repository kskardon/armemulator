.global strlen_a
.func strlen_a

/* r0 - String */
/* r1 - int strlen */
/* r2 - value of NUL */
/* r12 - value in string */

strlen_a:
    mov r1, #0

loop:
    ldrb r12, [r0]
    /* If r12 is not the NULL character */
    cmp r12, #0
    /* If the value of r0 is equal to NULL, end the loop */
    beq endloop
    /* Increment the strlen */
    add r1, r1, #1
    /* Increment the space in memory */
    add r0, r0, #1   
    b loop 
    
endloop:
    mov r0, r1
    bx lr
