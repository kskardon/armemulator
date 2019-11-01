.global cmpbne_a
.func cmpbne_a

cmpbne_a:
    cmp r0, r1
    bne end
    add r0, r0, r1

end:
    bx lr
