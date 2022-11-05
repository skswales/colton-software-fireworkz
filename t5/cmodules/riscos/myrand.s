; cmodules/riscos/myrand.s (ARM assembler)

; This Source Code Form is subject to the terms of the Mozilla Public
; License, v. 2.0. If a copy of the MPL was not distributed with this
; file, You can obtain one at https://mozilla.org/MPL/2.0/.

; Copyright (C) 1991-1998 Colton Software Limited
; Copyright (C) 1998-2015 R W Colton

        GET     as_flags_h
        GET     Hdr:ListOpts
        GET     Hdr:APCS.APCS-32
        GET     Hdr:Macros
        GET     as_macro_h

        AREA    |C$$code|,CODE,READONLY

; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
; extern uint32_t myrand(P_MYRAND_SEED p_myrand_seed)
;                        a1 (R0)

; Fast pseudo-random binary sequence generator

; a2 (R1), a3 (R2), a4 (R3) used as temporary registers

        BeginExternal myrand

        ; LinkNotStacked - no FunctionEntry here

        MOV     R3, R0                  ; Remember where to store the updated 33 bit seed

        LDMIA   R0, {R0, R1}            ; Load 33 bit seed (32 bit word in R0 and a single bit in LSB of R1)

; Core code derived from ARM Assembler Release 2 p.186

        TST     R1, R1, LSR #1          ; Top bit into carry
        MOVS    R2, R0, RRX             ; Which is then used in 33 bit rotate right into R2
        ADC     R1, R1, R1              ; Carry into LSB of R1
        EOR     R2, R2, R0, LSL #12     ; 'involved!' goes the comment in the book
        EOR     R0, R2, R2, LSR #20     ; 'similarly involved!'

        STMIA   R3, {R0, R1}            ; Store updated 33 bit seed, returning R0

        Return "","LinkNotStacked"

; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
; extern uint32_t myrand_n(P_MYRAND_SEED p_myrand_seed, uint32_t n /*excl*/)
;                          a1 (R0)                      a2 (R1)

; Fast pseudo-random binary sequence generator

; R2, R3, ip, lr used as temporary registers

        BeginExternal myrand_n

        FunctionEntry "",""

        MOV     ip, #0                  ; Find a mask (=ip) > n (can't have n == &FFFFFFFF)

10      MOV     ip, ip, LSL #1
        ORR     ip, ip, #1
        CMP     ip, a2
        BLS     %BT10

        MOV     R3, R0                  ; Remember where to store the updated 33 bit seed
        MOV     lr, R1                  ; Remember n

        LDMIA   R0, {R0, R1}            ; Load 33 bit seed (32 bit word in R0 and a single bit in LSB of R1)

; Core code derived from ARM Assembler Release 2 p.186

20      TST     R1, R1, LSR #1          ; Top bit into carry
        MOVS    R2, R0, RRX             ; Which is then used in 33 bit rotate right into R2
        ADC     R1, R1, R1              ; Carry into LSB of R1
        EOR     R2, R2, R0, LSL #12     ; 'involved!' goes the comment in the book
        EOR     R0, R2, R2, LSR #20     ; 'similarly involved!'

        AND     R0, R0, ip              ; rval (=R0) now in range [0..mask]

        CMP     R0, lr                  ; Test for rval < n
        BHS     %BT20                   ; Too large this time, so loop

        STMIA   R3, {R0, R1}            ; Store updated 33 bit seed

        Return "",""

; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

        END ; of cmodules/riscos/myrand.s
