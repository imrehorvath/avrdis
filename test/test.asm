    .org 0x0000
    ldi r16, 8
    out 0x3e, r16
    ldi r16, 255
    out 0x3d, r16
    cbi 0x05, 0
    sbi 0x04, 0
L0: sbi 0x05, 0
    nop
    cbi 0x05, 0
    rjmp L0