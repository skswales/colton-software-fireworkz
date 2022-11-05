#!/bin/sh
# Expect errors from
# ob_rec that needs low-level DPLib
arm-none-eabi-gcc \
 -std=c99 \
 -I../.. \
 -I../../external/cs-nonfree/Acorn/Library/32/CLib/msvchack \
 -I../../external/cs-nonfree/Acorn/Library/32/tboxlibs \
 -funsigned-char \
 -fno-builtin \
 -DCROSS_COMPILE -DHOST_GCCSDK -DTARGET_RISCOS -DRELEASED \
 -c ../../*/*.c
