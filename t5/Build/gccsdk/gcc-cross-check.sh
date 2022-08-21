#!/bin/sh
# Expect errors from
# ob_rec that needs low-level DPLib
gcc -c ../../*/*.c  -I../.. -I ../../external/cs-nonfree/Acorn/Library/32/CLib/msvchack  -I../../external/cs-nonfree/Acorn/Library/32/tboxlibs  -funsigned-char -DCROSS_COMPILE -DHOST_GCCSDK -DTARGET_RISCOS -DRELEASED
