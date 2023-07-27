# avrdis
AVR Disassembler for the 8-bit AVRs.

It generates AVRASM assembly source out of the ihex file provided. Alternatively, it generates the listing with the word addressess and instruction words along with the assembly source.
Output is written to stdout. In case of a successful run, the exit status is 0. Otherwise it is 1. Error messages are written to stderr.

## Design and Implementation considerations

- Maximum portability. It is written in C and uses libc only, no external libraries needed.
- Simple, straightforward structure of project with only a few modules for the different parts.
- The modules are decoupled, it is easy to add new input format parsers without touching the other modules.

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
```
