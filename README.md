# armemulator

TO COMPILE:
    1. enter directory
    2. type make
    3. type make test

This program emulates the following ARM instructions:
    
    CONTROL/BRANCHING INSTRUCTIONS
      -b, bl, bx, beq, bne, bgt, blt, bge, ble
    
    DATA PROCESSING INSTRUCTIONS
      -mov, cmp, add, sub, mul
      
    MEMORY INSTRUCTIONS
      -ldr, ldrb, str, strb
      
The output is the results of fib_rec, fib_iter, strlen, quadratic, sum_array, and find_max programs run
in all three environments: C language, ARM native, and ARM emulator.

In addition to this, the "Dynamic Analysis" section ofthis program provides statistics regarding how many 
total branch instructions have executed.
    
