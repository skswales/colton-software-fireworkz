; > c32/s.h_flags

; This Source Code Form is subject to the terms of the Mozilla Public
; License, v. 2.0. If a copy of the MPL was not distributed with this
; file, You can obtain one at https://mozilla.org/MPL/2.0/.

; Copyright (C) 1989-1998 Colton Software Limited
; Copyright (C) 1998-2015 R W Colton

    GBLL    NAMES
    GBLL    PROFILING
    GBLL    RELEASED

NAMES     SETL {TRUE}
PROFILING SETL {TRUE}
RELEASED  SETL {FALSE}

PROFILING SETL {FALSE}

;;;NAMES     SETL {FALSE}
RELEASED  SETL {TRUE}

 [ (:LNOT: RELEASED) :LOR: PROFILING
    KEEP  ; Keep debug info
 ]

; "No26bitCode" means don't rely on 26-bit instructions
; (eg TEQP and LDM ^) - the code will work on 32-bit systems.

; "No32bitCode" means don't rely on 32-bit instructions
; (eg MSR and MRS) - the code will work on RISC OS 3.1.

; Setting both to {TRUE} is too much for the macros to cope with -
; you will have to use run-time code as shown below.

    GBLL    No26bitCode
    GBLL    No32bitCode
No26bitCode SETL {FALSE}
No32bitCode SETL {TRUE}

    END
