.global fib_rec_a
.func fib_rec_a
/* r0 - n */
/* r1 - return value*/
/* r2 - n-1 */
fib_rec_a:
    /* Allocate two words on the stack (for prev prev, prev, and cur) */
    sub sp, sp, #16
   
    /* Base case */
    cmp r0, #0
    /* If it does not equal 0, call the recursive function */
    beq zero
    cmp r0, #1
    bne fib_recursive_step
    /* Store one in r1 */
    mov r1, #1
    /* Branch to the end */
    b fib_end

fib_recursive_step:
    /* Store the link register on the stack */
    str lr, [sp]
    /* Store n (r0) on the stack */
    str r0, [sp, #4]
    /* Create n -1*/
    sub r0, r0, #1
    /* Store n-1 (r0) on the stack*/

    /* Finding fib(n-1) */
    bl fib_rec_a
    mov r1, r0
    str r1, [sp, #8]//where do i store this?
 
    ldr r0, [sp, #4]
    /* Change n-1 (r0) to n-2 */
    sub r0, r0, #2 
    /* Finding fib(n-2) */
    bl fib_rec_a
    ldr r1, [sp,#8]
    
    add r1, r0, r1

    ldr lr, [sp] // restore original lr from stack[0]
   
fib_end:
    mov r0, r1
    add sp, sp, #16
    bx lr

zero:
    mov r1, #0
    b fib_end

    .endfunc
