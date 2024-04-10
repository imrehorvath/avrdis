#!/bin/sh

if [ ! -x ../avrdis ]; then
    echo "Please run make in the top directory first!"
    exit 1
fi

if ! ../avrdis test_src.hex 2>/dev/null | diff test_plain.asm -; then
    echo "Plain assemby source generation has FAILED"
    exit 1
fi
echo "Plain assembly source generation PASSED"

if ! ../avrdis -l test_src.hex 2>/dev/null | diff test_plain.lst -; then
    echo "Listing generation has FAILED"
    exit 1
fi
echo "Listing generation PASSED"

if ! ../avrdis -l -e 8:9 test_src.hex 2>/dev/null | diff test_ena.lst -; then
    echo "Enable region for disassembly in listing has FAILED"
    exit 1
fi
echo "Enable region for disassembly in listing PASSED"

exit 0
