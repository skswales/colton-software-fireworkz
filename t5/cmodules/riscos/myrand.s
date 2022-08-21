; cmodules/riscos/myrand.s (ARM assembler)

; This Source Code Form is subject to the terms of the Mozilla Public
; License, v. 2.0. If a copy of the MPL was not distributed with this
; file, You can obtain one at http://mozilla.org/MPL/2.0/.

; Copyright (C) 1991-1998 Colton Software Limited
; Copyright (C) 1998-2015 R W Colton

        GET     as_flags_h

        ;GET     as_regs_h
        GET     as_macro_h

        AREA    |C$$code|,CODE,READONLY

; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
; extern int myrand(P_MYRAND_SEED p_myrand_seed, U32 n /*excl*/, U32 bias)
;                   a1 (R0)                      a2 (R1)         a3 (R2)

; Fast pseudo-random binary sequence generator

; a4, ip, lr used as temporary registers

        BeginExternal myrand

        FunctionEntry "v5,v6","MakeFrame"

        LDMIA   a1, {v5, v6}            ; Load 33 bit seed (32 bit word in Ra (=v5) and a single bit in LSB of Rb (=v6))

        MOV     ip, #0                  ; Find a mask (=ip) > n (can't have n == &FFFFFFFF)

10      MOV     ip, ip, LSL #1
        ORR     ip, ip, #1
        CMP     ip, a2
        BLS     %BT10

; Core code derived from ARM Assembler Release 2 p.186

20      TST     v6, v6, LSR #1
        MOVS    lr, v5, RRX             ; Rc (=lr)
        ADC     v6, v6, v6
        EOR     lr, lr, v5, LSL #12     ; 'involved!' goes the comment in the book
        EOR     v5, lr, lr, LSR #20     ; 'similarly involved!'

        AND     a4, v5, ip              ; rval (=a4) now in range [0..mask]

        CMP     a4, a2                  ; Test for rval < n
        BHS     %BT20                   ; Too large this time, so loop

        STMIA   a1, {v5, v6}            ; Store updated 33 bit seed

        ADD     a1, a4, a3              ; Final result (=a1) := bias (=a3) + rval (=a4)

        FunctionReturn "v5,v6","fpbased"

; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

        END ; of cmodules/riscos/myrand.s
