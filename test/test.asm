    .org 0x0000
    clr r17
    in r16, 0x03
    tst r16
    breq L0
    rjmp L1
L0: inc r17
L1: call L2
    .org 0x03ff
L2: lpm
    ret
