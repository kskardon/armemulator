.global cmpb_a
.func cmpb_a

cmpb_a:
    cmp r0, r1
    b add

sub:
    sub r0, r0, r1

add:
    add r0, r0, r1

end:
    bx lr
