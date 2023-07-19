#!/bin/sh

if [ ! -x ../avrdis ]; then
    echo "Please run make first in the top directory!"
    exit 1
fi

if ! ../avrdis test.hex | diff -q test.asm - >/dev/null; then
    echo "Assemby source generation has FAILED"
    exit 1
fi
echo "Assembly source generation PASSED"

if ! ../avrdis -l test.hex | diff -q test.lst - >/dev/null; then
    echo "Listing generation has FAILED"
    exit 1
fi
echo "Listing generation PASSED"

exit 0
