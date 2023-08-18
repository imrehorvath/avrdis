    .org 0x0000
    ldi r16, 0
    rjmp L0
    .dw 0x696d
    .dw 0x0064
L0: inc r16
    rjmp L0
    .dw 0x6e65
    .dw 0x0064
    .dw 0x0000
    .dw 0x0000
