.global sum_array_a
.func sum_array_a

/* r0 - int *array */
/* r1 - int len */
/* r2 - int i */
/* r3 - sum */

sum_array_a:
    mov r2, #0
    mov r3, #0

loop:
    cmp r2, r1
    beq endloop
    /* Put the value in r0 into r12 */
    ldr r12, [r0]
    /* Add number to sum */
    add r3, r12, r3
    /* Increment i by 1 and the memory space by 4 */
    add r2, r2, #1
    add r0, r0, #4
    b loop

endloop:
    mov r0, r3
    bx lr
