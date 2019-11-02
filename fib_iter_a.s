.global fib_iter_a
.func fib_iter_a

/* r0 - n (the nth number to iterate through) */
/* r1 - prev prev number */
/* r2 - prev number */
/* r3 - cur_num */
/* r12 - i index */

fib_iter_a:
    mov r1, #0
    mov r2, #0
    mov r3, #0
    mov r12, #1

    /* If n is 0, return 0 through cur_num */
    cmp r0, #0
    beq endwhile

    /* If n is not 0, set cur_num back to 1 */
    add r3, r3, #1

while:
    /* Compare the index of 1 to n */
    cmp r12, r0
    /* Break equal */
    beq endwhile
    /* Work on numbers here */
    /* Store prev in prev prev */ 
    mov r1, r2
    /* Store current num in prev */
    mov r2, r3
    /* Add prev prev and prev and store in current number */
    add r3, r1, r2
    /*sub r0, r0, #1*/
    add r12, r12, #1
    b while

endwhile:
    mov r0, r3
    bx lr
    

/* Number not accurate when n is 0, so maybe try to get r3 to 0 initially? */

