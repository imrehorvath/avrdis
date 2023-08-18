; Please note that the code below is for testing the features of the disassembler only,
; and is not a code example that performs any useful stuff!
.org 0
ldi r16, 0
rjmp L1
.db "mid", 0
L1:
inc r16
rjmp L1
.db "end", 0
.dw 0x0000
.dw 0x0000
