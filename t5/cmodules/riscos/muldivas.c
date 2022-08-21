        TTL     cmodules/s.muldivas

; This Source Code Form is subject to the terms of the Mozilla Public
; License, v. 2.0. If a copy of the MPL was not distributed with this
; file, You can obtain one at http://mozilla.org/MPL/2.0/.

; Copyright (C) 1991-1998 Colton Software Limited
; Copyright (C) 1998-2015 R W Colton

        GET     as_flags_h

        ;GET     as_regs_h
        GET     as_macro_h

        GBLL    MULDIV_USE_UMULL
MULDIV_USE_UMULL SETL {TRUE}
;MULDIV_USE_UMULL SETL {FALSE}

; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

        AREA    |C$$zidata|,DATA,NOINIT

muldiv64__statics
        %       2*4
MULDIV_REMAINDER * 0*4
MULDIV_OVERFLOW  * 1*4

; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

        MACRO
$label  MULDIV_UMULL
$label  DCD     &E0832190       ; UMULL a1*a2 ->a4(hi),a3(lo) for ARM600 and better processors
        MEND

; +++

        MACRO
$label  MULDIV_START
$label
        EOR     v1, a1, a2      ; v1 = sgn(a*b)

        CMP     a1, #0
        BEQ     muldiv64_zero_result
        RSBMI   a1, a1, #0      ; a1 := abs(a)

        CMP     a2, #0
        BEQ     muldiv64_zero_result
        RSBMI   a2, a2, #0      ; a2 := abs(b)

        MOVS    v2, a3          ; v2 := sgn(c)
        BEQ     muldiv64_divide_zero
        MOVPL   v3, a3          ; v3 := abs(c)
        RSBMI   v3, a3, #0
        MEND

; +++

; First, the double-length product of a1 * a2 into a4(hi) & a3(lo)
; destroying a1, a2, ip

; Derived from ARM Assembler Release 2 p.189

        MACRO
$label  MULDIV_PROCESS

 [ MULDIV_USE_UMULL
$label  MULDIV_UMULL
        B       %FT02
 |
$label  MOVS    a4, a1, LSR #16     ; a4 is ms 16 bits of a1
        BIC     a1, a1, a4, LSL #16 ; a1 is ls 16 bits
 ]
        MOV     ip, a2, LSR #16     ; ip is ms 16 bits of a2
        BIC     a2, a2, ip, LSL #16 ; a2 is ls 16 bits

        MUL     a3, a1, a2          ; Low partial product
        MUL     a2, a4, a2          ; First middle partial product
        MUL     a1, ip, a1          ; Second middle partial product
        MULNE   a4, ip, a4          ; High partial product (NE improves even further efficiency for a4 == 0)

        ADDS    a1, a1, a2          ; Add middle partial products (can't use MLA cos we need carry)
        ADDCS   a4, a4, #&00010000  ; Add carry into high partial product

        ADDS    a3, a3, a1, LSL #16 ; Add middle partial product sum into low and high words of result
        ADC     a4, a4, a1, LSR #16

02 ; a4,a3 have result here

        MEND

; +++

; Now the 64*32 bit divide
; dividend in a4(hi) and a3(lo). divisor in v3
; remainder ends up in a3; quotient in v6(lo) and v4(hi)
; uses a2(hi) and a1(lo) to hold the (shifted) divisor;
;      r14,v5 for the current bit in the quotient

        MACRO
$label  MULDIV_END
$label
        MOV     a1, v3 ; abs(c)

        MOV     a2, #0

        MOV     v6, #0 ; becomes quotient-lo
        MOV     v4, #0 ; becomes quotient-hi

        MOV     v5, #0
        MOV     r14, #1 ; used as 1 << v5

02      CMPS    a2, #&80000000
        BCS     %FT03

        CMPS    a2, a4
        CMPEQS  a1, a3          ; compare of [a2, a1] against [a4, a3]
        BCS     %FT03

        MOVS    a1, a1, ASL #1 ; left shift the 64-bit shifted divisor
        ADC     a2, a2, a2

        ADD     v5, v5, #1
        B       %BT02

03      CMPS    a2, a4
        CMPEQS  a1, a3
        BHI     %FT04

        CMPS    v5, #32
        ADDCC   v6, v6, r14, ASL v5
        SUBCS   ip, v5, #32
        ADDCS   v4, v4, r14, ASL ip

        SUBS    a3, a3, a1
        SBC     a4, a4, a2

04      MOVS    a2, a2, ASR #1
        MOV     a1, a1, RRX

        SUBS    v5, v5, #1
        BPL     %BT03

; now all we need to do is sort out the signs.

        TEQS    v1, v2          ; v1 = sgn(a*b), v2 = sgn(c): if a*b and c have opposite signs,
        RSBMI   v6, v6, #0      ; negate the quotient
        RSBMI   v4, v4, #0

        CMPS    v1, #0          ; and if the dividend was negative,
        RSBLT   a3, a3, #0      ; negate the remainder

        LDR     ip, =muldiv64__statics ; stash the remainder and overflow
        STR     a3, [ip, #MULDIV_REMAINDER]
        STR     v4, [ip, #MULDIV_OVERFLOW]

        MOV     a1, v6          ; load the quotient as result

        MEND

        AREA    |C$$code|,CODE,READONLY

        IMPORT  |raise|

; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

muldiv64_divide_zero

        MOV     a1, #2          ; SIGFPE
        BL      raise

muldiv64_zero_result

        MOV     a1, #0 ; result

        LDR     ip, =muldiv64__statics ; stash the remainder and overflow
        STR     a1, [ip, #MULDIV_REMAINDER]
        STR     a1, [ip, #MULDIV_OVERFLOW]

        FunctionReturn "v1-v6","fpbased"

; muldiv64(a, b, c)
; result = a*b/c
; result2 = a*b REM c
; the intermediate product is 64 bits long
; do everything using moduluses, and sort out signs later

; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
; extern long int muldiv64(long int a, long int b, long int c);

        BeginExternal muldiv64

        FunctionEntry "v1-v6","MakeFrame"

        MULDIV_START

md64_p1 MULDIV_PROCESS

        MULDIV_END

        FunctionReturn "v1-v6","fpbased"

; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
; extern long int muldiv64_ceil(long int a, long int b, long int c);

        BeginExternal muldiv64_ceil

        FunctionEntry "v1-v6","MakeFrame"

        MULDIV_START

md64_p2 MULDIV_PROCESS

; result(in C) = (sgn(a*b*c) -ve) ? (a*b) : ((a*b) + (c - 1)) / c

        TEQ    v1, v2
        BMI    muldiv64_ceil_continue

        ADDS   a3, a3, v3 ; add (divisor-1)
        ADC    a4, a4, #0
        SUBS   a3, a3, #1
        SBC    a4, a4, #0

muldiv64_ceil_continue

        MULDIV_END

        FunctionReturn "v1-v6","fpbased"

; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
; extern long int muldiv64_floor(long int a, long int b, long int c);

        BeginExternal muldiv64_floor

        FunctionEntry "v1-v6","MakeFrame"

        MULDIV_START

md64_p3 MULDIV_PROCESS

; result(in C) = ((sgn(a*b*c) +ve) ? (a*b) : ((a*b) - (c - 1))) / c

        TEQ    v1, v2
        BPL    muldiv64_floor_continue

        ADDS   a3, a3, v3 ; but remember we're working in unsigneds internally so add (divisor-1)!
        ADC    a4, a4, #0
        SUBS   a3, a3, #1
        SBC    a4, a4, #0

muldiv64_floor_continue

        MULDIV_END

        FunctionReturn "v1-v6","fpbased"

; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
; extern long int muldiv64_round_floor(long int a, long int b, long int c);

        BeginExternal muldiv64_round_floor

        FunctionEntry "v1-v6","MakeFrame"

        MULDIV_START

md64_p4 MULDIV_PROCESS

; result(in C) = ((sgn(a*b*c) +ve) ? (a*b + c/2) : (a*b - c/2)) / c

        MOV    ip, v3, LSR #1

        ADDS   a3, a3, ip
        ADC    a4, a4, #0

        TEQ    v1, v2
        BPL    muldiv64_round_floor_continue

        SUBS   a3, a3, #1
        SBC    a4, a4, #0

muldiv64_round_floor_continue

        MULDIV_END

        FunctionReturn "v1-v6","fpbased"

; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
; extern long int muldiv64_remainder(void);

        BeginExternal muldiv64_remainder

        ; LinkNotStacked - no FunctionEntry here

        LDR     a1, =muldiv64__statics
        LDR     a1, [a1, #MULDIV_REMAINDER]

        FunctionReturn "","LinkNotStacked"

; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
; extern long int muldiv64_overflow(void);

        BeginExternal muldiv64_overflow

        ; LinkNotStacked - no FunctionEntry here

        LDR     a1, =muldiv64__statics
        LDR     a1, [a1, #MULDIV_OVERFLOW]

        FunctionReturn "","LinkNotStacked"

; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
; extern long int muldiv64_limiting(long int a, long int b, long int c);

        BeginExternal muldiv64_limiting

        FunctionEntry "","MakeFrame"

        BL      muldiv64            ; a1 := (a1 * a2) / a3

        LDR     a2, =muldiv64__statics ; check overflow, limit if overflow != 0
        LDR     a2, [a2, #MULDIV_OVERFLOW]
        CMP     a2, #0
        MOV     a2, #&80000000 ; LONG_MAX + 1 (overflowed)
        SUBGT   a1, a2, #1 ; -> +LONG_MAX (0x7FFFFFFF)
        ADDLT   a1, a2, #1 ; -> -LONG_MAX (0x80000001)

        FunctionReturn "","fpbased"

; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
; extern void umul64(unsigned long a, unsigned long b, unsigned long * res_lo/hi);

        BeginExternal umul64

        FunctionEntry "v1","MakeFrame"

        MOV     v1, a3              ; Save result pointer

 [ MULDIV_USE_UMULL
md64_p5 MULDIV_UMULL
        B       %FT20
 |
md64_p5 MOVS    a4, a1, LSR #16     ; a4 is ms 16 bits of a1
        BIC     a1, a1, a4, LSL #16 ; a1 is ls 16 bits
 ]
        MOV     ip, a2, LSR #16     ; ip is ms 16 bits of a2
        BIC     a2, a2, ip, LSL #16 ; a2 is ls 16 bits

        MUL     a3, a1, a2          ; Low partial product
        MUL     a2, a4, a2          ; First middle partial product
        MUL     a1, ip, a1          ; Second middle partial product
        MULNE   a4, ip, a4          ; High partial product (NE improves even further efficiency for a4 == 0)

        ADDS    a1, a1, a2          ; Add middle partial products (can't use MLA cos we need carry)
        ADDCS   a4, a4, #&00010000  ; Add carry into high partial product

        ADDS    a3, a3, a1, LSL #16 ; Add middle partial product sum into low and high words of result
        ADC     a4, a4, a1, LSR #16

20      STMIA   v1, {a3, a4}        ; Store all 64 bits of result

        FunctionReturn "v1","fpbased"

; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
; extern void muldiv64_init(void);

        BeginExternal muldiv64_init

        ; LinkNotStacked - no FunctionEntry here

 [ MULDIV_USE_UMULL
        MOV     a1, #&DC000000      ; Check whether UMULL works on this processor
        ORR     a1, a1, #&000000BA
        MOV     a2, #&00000100
        MOV     a3, #0
        MOV     a4, #0
        MULDIV_UMULL
        TEQ     a4, #&000000DC
        TEQEQ   a3, #&0000BA00
        FunctionReturn "","LinkNotStacked",EQ ; [exit - UMULL is working OK ]

        ADR     a1, umulcod         ; Patch over code wherever needed to revert to working version
        LDMIA   a1, {a2, a3}

        ADRL    ip, md64_p1
        STMIA   ip, {a2, a3}

        ADRL    ip, md64_p2
        STMIA   ip, {a2, a3}

        ADR     ip, md64_p3
        STMIA   ip, {a2, a3}

        ADR     ip, md64_p4
        STMIA   ip, {a2, a3}

        ADR     ip, md64_p5
        STMIA   ip, {a2, a3}
 ]
        FunctionReturn "","LinkNotStacked"

 [ MULDIV_USE_UMULL
 ; Code to be patched back in place of UMULL; BAL

umulcod MOVS    a4, a1, LSR #16     ; a4 is ms 16 bits of a1
        BIC     a1, a1, a4, LSL #16 ; a1 is ls 16 bits
 ]

; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
; extern int myrand(P_MYRAND_SEED p_myrand_seed, U32 n /*excl*/)

; very fast pseudo-random binary sequence generator

; Derived from ARM Assembler Release 2 p.186

        BeginExternal myrand

        FunctionEntry "v5,v6","MakeFrame"

        MOV     v6, #0 ; Find a mask > n (can't have n == &FFFFFFFF)

10      MOV     v6, v6, LSL #1
        ORR     v6, v6, #1
        CMP     v6, a2
        BLS     %BT10

        MOV     ip, a1
        LDMIA   ip, {a1, a3} ; Load 33 bit seed (32 bit word and a single bit)

20      TST     a3, a3, LSR #1
        MOVS    a4, a1, RRX
        ADC     a3, a3, a3
        EOR     a4, a4, a1, LSL #12 ; 'involved' goes the comment in the book
        EOR     a1, a4, a4, LSR #20

        AND     v5, a1, v6 ; Usable result?
        CMP     v5, a2
        BHS     %BT20

        STMIA   ip, {a1, a3} ; Store new 33 bit seed

        MOV     a1, v5

        FunctionReturn "v5,v6","fpbased"

        END ; of cmodules/s.muldivas
