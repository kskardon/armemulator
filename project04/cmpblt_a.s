.global cmpblt_a
.func cmpblt_a

cmpblt_a:
    cmp r0, r1
    blt end
    add r0, r0, r1

end:
    bx lr
