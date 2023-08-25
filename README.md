# avrdis
AVR Disassembler for the 8-bit AVRs.

It generates AVRASM assembly source out of the ihex file provided. Alternatively, it generates the listing with the word addressess and instruction words along with the assembly source.
Output is written to stdout. In case of a successful run, the exit status is 0. Otherwise it is 1. Error messages are written to stderr.

## Design and Implementation considerations

- Maximum portability. It is written in C and uses libc only, no extra libraries needed.
- Simple, straightforward structure of project with only a few modules.
- Loosely coupled modules make it is easy to add new input format parsers without touching other parts.

## Suggested Use Case

When you have an 8-bit AVR microcontroller with an unlocked flash and you want to tinker with the code but you do not have the source.

## Workflow

1. Use `avrdude` to extract the flash contents into an ihex file.
2. Use `avrdis` to disassemble the code. (eg. `avrdis m328p.hex > m328p.asm`)
3. Use a code editor or IDE to edit the asm file.
4. Use `avrasm2.exe` or `avra` assemblers to compile the modified source into an ihex file.
5. Use `avrdude` to flash the modified firmware.

## Usage

```
AVR Disassembler for the 8-bit AVRs. v1.0.0 (c) Imre Horvath, 2023
Usage: avrdis [options] inputfile
Currently supported inputfile types are: IHEX
  IHEX: Intel hex format, file should have an extension .hex
Options:
  -h : Show this usage info and exit.
  -l : List word addresses and raw instructions with the disassembled code.
  -e nnnn:nnnn : Enable disassembly of otherwise disabled region. Multiple options are possible.
                 Use hex numbers. For reference, see listing of disabled regions to stderr.
```

## Example 1

Consider an example firmware `foo.hex` with the following content.
```
:020000020000FC
:0200000004C03A
:0200040018954D
:10000800189503B103701127EDE0F0E0E00FF11F40
:10001800099403C003C003C003C003C0F2CFF1CFEB
:10002800F0CFF0E0E0E4D0E0C0E60AE0C89509923D
:1000380031960A95D9F7E5CF0001020304050607B2
:020048000809A5
:00000001FF
```
We run `avrdis` to dissassemble it and get the following.
```
% avrdis foo.hex
0x0004:0x0004
0x000d:0x0024
    .org 0x0000
    rjmp L0
    .org 0x0002
    reti
    .org 0x0004
    .dw 0x9518
L0: in r16, 0x03
    andi r16, 3
    clr r17
    ldi r30, 13
    ldi r31, 0
    add r30, r16
    adc r31, r17
    ijmp
    .dw 0xc003
    .dw 0xc003
    .dw 0xc003
    .dw 0xc003
    .dw 0xc003
    .dw 0xcff2
    .dw 0xcff1
    .dw 0xcff0
    .dw 0xe0f0
    .dw 0xe4e0
    .dw 0xe0d0
    .dw 0xe6c0
    .dw 0xe00a
    .dw 0x95c8
    .dw 0x9209
    .dw 0x9631
    .dw 0x950a
    .dw 0xf7d9
    .dw 0xcfe5
    .dw 0x0100
    .dw 0x0302
    .dw 0x0504
    .dw 0x0706
    .dw 0x0908
% 
```
To see the instruction word addresses next to the disassebled code, we can use the `-l` option.
```
% avrdis -l foo.hex
0x0004:0x0004
0x000d:0x0024
C:00000 c004     rjmp L0
C:00002 9518     reti
C:00004 9518     .dw 0x9518
C:00005 b103 L0: in r16, 0x03
C:00006 7003     andi r16, 3
C:00007 2711     clr r17
C:00008 e0ed     ldi r30, 13
C:00009 e0f0     ldi r31, 0
C:0000a 0fe0     add r30, r16
C:0000b 1ff1     adc r31, r17
C:0000c 9409     ijmp
C:0000d c003     .dw 0xc003
C:0000e c003     .dw 0xc003
C:0000f c003     .dw 0xc003
C:00010 c003     .dw 0xc003
C:00011 c003     .dw 0xc003
C:00012 cff2     .dw 0xcff2
C:00013 cff1     .dw 0xcff1
C:00014 cff0     .dw 0xcff0
C:00015 e0f0     .dw 0xe0f0
C:00016 e4e0     .dw 0xe4e0
C:00017 e0d0     .dw 0xe0d0
C:00018 e6c0     .dw 0xe6c0
C:00019 e00a     .dw 0xe00a
C:0001a 95c8     .dw 0x95c8
C:0001b 9209     .dw 0x9209
C:0001c 9631     .dw 0x9631
C:0001d 950a     .dw 0x950a
C:0001e f7d9     .dw 0xf7d9
C:0001f cfe5     .dw 0xcfe5
C:00020 0100     .dw 0x0100
C:00021 0302     .dw 0x0302
C:00022 0504     .dw 0x0504
C:00023 0706     .dw 0x0706
C:00024 0908     .dw 0x0908
% 
```
The first two lines in the output are the program memory ranges, which are excluded from disassembly. Why are these excluded? Because `avrdis` is a simple disassembler that can only follow the relative and absolute addresses from the branching instructions and does not try to perform semantic analysis of the code, or simulation of runtime behaviour to infer possible code regions for disassembly. Please note that the disabled address regions are printed to `stderr`, so you can redirect the output of the command to a file without worrying about the extra lines visible in the terminal.

But why don't just disassemble firmware as a whole. That's because AVRs use a Modified Harvard Architecture which allows parts of the program memory to be accessed as data. This is very useful to store read-only data like character strings or data tables directly in the program memory and access them directly from there without the need to copy them to the SRAM first. Note that the SRAM is rather limited in AVRs compared to the program memory (flash).

So since code and data can co-exist in the program memory and the interpreptation of data as code can lead to issues, `avrdis` uses a simple approach to disassemble the parts only, those are directly accessible from the branching instructions, thus guarantied to be code.

Those parts which are potentionally data, are emitted as `.dw 0xnnnn`. To enable the disassembly of such parts in case you're sure that those are code and not data, you can use the option `-e nnnn:nnnn` to specify a range. Multiple `-e` options are allowed to specify disjunct ranges.

The full disassembly of a "mixed" firmware usually takes multiple iterations using the option `-e` to explore the non-trivial parts.

To completly disassemble the above example we can use the option `-e` as shown below. Please note that the last words emmited with `.dw` are the data bytes from 0 to 9 and are not instructions to be executed.
```
% avrdis -e 0:10 foo.hex
0x0020:0x0024
    .org 0x0000
    rjmp L0
    .org 0x0002
    reti
    .org 0x0004
    reti
L0: in r16, 0x03
    andi r16, 3
    clr r17
    ldi r30, 13
    ldi r31, 0
    add r30, r16
    adc r31, r17
    ijmp
    rjmp L1
    rjmp L2
    rjmp L3
    rjmp L4
L1: rjmp L5
L2: rjmp L0
L3: rjmp L0
L4: rjmp L0
L5: ldi r31, 0
    ldi r30, 64
    ldi r29, 0
    ldi r28, 96
    ldi r16, 10
L6: lpm
    st Y+, r0
    adiw r31:r30, 1
    dec r16
    brne L6
    rjmp L0
    .dw 0x0100
    .dw 0x0302
    .dw 0x0504
    .dw 0x0706
    .dw 0x0908
% 
```
