# avrdis
AVR Disassembler for the 8-bit AVRs.

Generates AVRASM assembly source out of the input file provided. Alternatively, it generates the listing with the word addressess and instruction words along with the assembly source.
Output is written to stdout. In case of a successful run, the exit status is 0. Otherwise it's 1. Error messages are written to stderr.

## Design- and Implementation considerations

- Maximum portability. Written in C and uses libc only, no extra libraries needed.
- Loosley coupled modular design, with clearly separated parts makes it easy to extend with new input format parsers without touching other parts.

## Tipical Use Case

You have an unlocked 8-bit AVR microcontroller, and you want to tinker with the code but you don't have the source code.

## Workflow

1. Use `avrdude` to extract the flash contents into an ihex file.
2. Use `avrdis` to disassemble the code. (eg. `avrdis m328p.hex > m328p.asm`)
3. Use a code editor or IDE to edit the asm file.
4. Use `avrasm2.exe` or `avra` assemblers to compile the modified source into an ihex file.
5. Use `avrdude` to flash the modified firmware.

## Usage

First make the project.
```
$ cd avrdis
$ make
$ make install
```

This will install the `avrdis` executable into `/usr/local/bin` by default.

In case you wish to install it elsewhere, you can set the DESTDIR and the PREFIX variables accordingly.
```
$ make install DESTDIR=~/temp/ PREFIX=.
```

This will install the `avrdis` executable into `~/temp/bin`.

You can use the `-h` option to show the usage.
```
$ avrdis -h
AVR Disassembler for the 8-bit AVRs. v1.0.0 (c) Imre Horvath, 2023
Usage: avrdis [options] inputfile
Currently supported inputfile types are: IHEX
  IHEX: Intel hex format, file should have an extension .hex
Options:
  -h : Show this usage info and exit.
  -l : List word addresses and raw instructions together with the disassembled code.
  -e nnnn:nnnn : Enable disassembly of otherwise disabled region. Multiple options are possible.
                 Use hex numbers. For reference, see listing of disabled regions to stderr.
$ 
```

## Example 1

Consider an example firmware `foo.hex` with the following content.
```
$ cat foo.hex
:020000020000FC
:0200000004C03A
:0200040018954D
:10000800189503B103701127EDE0F0E0E00FF11F40
:10001800099403C003C003C003C003C0F2CFF1CFEB
:10002800F0CFF0E0E0E4D0E0C0E60AE0C89509923D
:1000380031960A95D9F7E5CF0001020304050607B2
:020048000809A5
:00000001FF
$ 
```
Run `avrdis` to dissassemble it.
```
$ avrdis foo.hex
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
$ 
```
To get the complete listing with addresses and raw words along with the disassebled code, use the `-l` option. (Note that the word addresses from the listing can be used with the `-e` option later on, when exploring. See below!)
```
$ avrdis -l foo.hex
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
$ 
```
The first two lines in the output are the program memory ranges, which are excluded from disassembly. Why are these excluded? Because `avrdis` is a simple disassembler that can only follow the relative and absolute addresses from the branching instructions and does not try to perform semantic analysis of the code, or simulation of runtime behavior to infer possible code regions for disassembly. Please note that the disabled address regions are printed to `stderr`, so you can redirect the output of the command to a file without worrying about the extra lines visible in the terminal.

The reason for this complexity comes from the fact that AVRs use a Modified Harvard Architecture which allows parts of the program memory to be accessed as data. This is very useful to store read-only data like character strings or data tables directly in the program memory. (Note that the SRAM is rather limited in AVRs compared to the program memory.)

So since code and data can co-exist in the program memory and the interpreptation of data as code can lead to issues during the disassembly, `avrdis` uses a simple approach to disassemble the parts only, those are directly accessible from the branching instructions, thus guarantied to be code.

Those parts which are potentionally data, are emitted as `.dw 0xnnnn`. To enable the disassembly of such parts in case you're sure that those are code and not data, you can use the `-e nnnn:nnnn` option to specify a range. Multiple `-e` options are allowed to specify multiple ranges.

The full disassembly of a "mixed" firmware usually takes multiple iterations using the `-e` option to explore the non-trivial parts.

After exploration of the above example, use the `-l` and `-e` options to get a listing of the properly disassembled code, with code words disassembled and data left as it is. The word address range `0x0020:0x0024` condains the data, that is referred by the `ldi` instructions at addresses `C:00015` and `C:00016` as decimal byte address high and low respectively. (Note that the word address `0x0020` translates to the byte address `0x0040` which is 0 high and 64 low in decimal.)
```
$ avrdis -l -e 4:4 foo.hex
0x000d:0x0024
C:00000 c004     rjmp L0
C:00002 9518     reti
C:00004 9518     reti
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
$ 
```

Then reveal the `ijmp` targets.
```
$ avrdis -l -e 4:4 -e d:11 foo.hex
0x0020:0x0024
C:00000 c004     rjmp L0
C:00002 9518     reti
C:00004 9518     reti
C:00005 b103 L0: in r16, 0x03
C:00006 7003     andi r16, 3
C:00007 2711     clr r17
C:00008 e0ed     ldi r30, 13
C:00009 e0f0     ldi r31, 0
C:0000a 0fe0     add r30, r16
C:0000b 1ff1     adc r31, r17
C:0000c 9409     ijmp
C:0000d c003     rjmp L1
C:0000e c003     rjmp L2
C:0000f c003     rjmp L3
C:00010 c003     rjmp L4
C:00011 c003 L1: rjmp L5
C:00012 cff2 L2: rjmp L0
C:00013 cff1 L3: rjmp L0
C:00014 cff0 L4: rjmp L0
C:00015 e0f0 L5: ldi r31, 0
C:00016 e4e0     ldi r30, 64
C:00017 e0d0     ldi r29, 0
C:00018 e6c0     ldi r28, 96
C:00019 e00a     ldi r16, 10
C:0001a 95c8 L6: lpm
C:0001b 9209     st Y+, r0
C:0001c 9631     adiw r31:r30, 1
C:0001d 950a     dec r16
C:0001e f7d9     brne L6
C:0001f cfe5     rjmp L0
C:00020 0100     .dw 0x0100
C:00021 0302     .dw 0x0302
C:00022 0504     .dw 0x0504
C:00023 0706     .dw 0x0706
C:00024 0908     .dw 0x0908
$ 
```


Finally, the raw source code can be redirected to a `.asm` file for further tinkering in a code editor. (Note that the `-l` option was dropped to get the `.asm` source only.)
```
$ avrdis -e 4:4 -e d:11 foo.hex >foo.asm
0x0020:0x0024
$ cat foo.asm
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
$ 
```
