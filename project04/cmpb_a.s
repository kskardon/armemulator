.global cmpb_a
.func cmpb_a

cmpb_a:
    cmp r0, r1
    b end

sub:
    sub r0, r0, r1

end:
    bx lr
