; cmodules/riscos/vfparith.s (ARM assembler)

; This Source Code Form is subject to the terms of the Mozilla Public
; License, v. 2.0. If a copy of the MPL was not distributed with this
; file, You can obtain one at https://mozilla.org/MPL/2.0/.

; Copyright (C) 2021-2022 Stuart Swales

        GET     as_flags_h
        GET     Hdr:ListOpts
        GET     Hdr:APCS.APCS-32
        GET     Hdr:Macros
        GET     as_macro_h

; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

        AREA |C$$Code|,CODE,READONLY

; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
; extern VFP_DOUBLE vfp_double_from_f64(PC_F64 p_f64);

        BeginExternal vfp_double_from_f64
        ; LinkNotStacked - no FunctionEntry here
        LDMIA   R1,{R2,R3} ; p_f64 -> FPA order words
        ; transpose words to VFP order
        STR     R3,[R0,#0] ; result
        STR     R2,[R0,#4]
        Return  "","LinkNotStacked"

; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

        AREA |C_with_VFP$$Code|,CODE,READONLY,VFP ; Will winge at link time about VFP attribute

; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
; extern VFP_DOUBLE vfp_double_add(PC_VFP_DOUBLE a, PC_VFP_DOUBLE b);

        BeginExternal vfp_double_add
        ; LinkNotStacked - no FunctionEntry here
        VLDR    D6,[R1,#0] ; a
        VLDR    D7,[R2,#0] ; b
        VADD.F64 D7,D6,D7
        VSTR    D7,[R0,#0] ; (result)
        Return  "","LinkNotStacked"

        ;;; And NOT VMOV    R0,R1,D7 ; result (bottom 32 bits in R0, top 32 bits in R1)

 [ {FALSE}
; extern void vfp_double_add_r(P_VFP_DOUBLE result, PC_VFP_DOUBLE a, PC_VFP_DOUBLE b);

        BeginExternal vfp_double_add_r
        ; LinkNotStacked - no FunctionEntry here
        VLDR    D6,[R1,#0] ; a
        VLDR    D7,[R2,#0] ; b
        VADD.F64 D7,D6,D7
        VSTR    D7,[R0,#0] ; result
        Return  "","LinkNotStacked"
 ]

; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
; extern VFP_DOUBLE vfp_double_sub(PC_VFP_DOUBLE a, PC_VFP_DOUBLE b);

        BeginExternal vfp_double_sub
        ; LinkNotStacked - no FunctionEntry here
        VLDR    D6,[R1,#0] ; a
        VLDR    D7,[R2,#0] ; b
        VSUB.F64 D7,D6,D7
        VSTR    D7,[R0,#0] ; (result)
        Return  "","LinkNotStacked"

 [ {FALSE}
; extern void vfp_double_sub_r(P_VFP_DOUBLE result, PC_VFP_DOUBLE a, PC_VFP_DOUBLE b);

        BeginExternal vfp_double_sub_r
        ; LinkNotStacked - no FunctionEntry here
        VLDR    D6,[R1,#0] ; a
        VLDR    D7,[R2,#0] ; b
        VSUB.F64 D7,D6,D7
        VSTR    D7,[R0,#0] ; result
        Return  "","LinkNotStacked"
 ]

; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
; extern VFP_DOUBLE vfp_double_mul(PC_VFP_DOUBLE a, PC_VFP_DOUBLE b);

        BeginExternal vfp_double_mul
        ; LinkNotStacked - no FunctionEntry here
        VLDR    D6,[R1,#0] ; a
        VLDR    D7,[R2,#0] ; b
        VMUL.F64 D7,D6,D7
        VSTR    D7,[R0,#0] ; (result)
        Return  "","LinkNotStacked"

 [ {FALSE}
; extern void vfp_double_mul_r(P_VFP_DOUBLE result, PC_VFP_DOUBLE a, PC_VFP_DOUBLE b);

        BeginExternal vfp_double_mul_r
        ; LinkNotStacked - no FunctionEntry here
        VLDR    D6,[R1,#0] ; a
        VLDR    D7,[R2,#0] ; b
        VMUL.F64 D7,D6,D7
        VSTR    D7,[R0,#0] ; result
        Return  "","LinkNotStacked"
  ]

; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
; extern VFP_DOUBLE vfp_double_mla(PC_VFP_DOUBLE a, PC_VFP_DOUBLE b, PC_VFP_DOUBLE c);

        BeginExternal vfp_double_mla
        ; LinkNotStacked - no FunctionEntry here
        VLDR    D6,[R1,#0] ; a
        VLDR    D7,[R2,#0] ; b
        VLDR    D5,[R3,#0] ; c
 [ {TRUE}
        VMLA.F64 D5,D6,D7  ; D5 := D5+(D6*D7)
 |
        VMUL.F64 D7,D6,D7  ; D5 := D5+(D6*D7)
        VADD.F64 D5,D7,D5
 ]
        VSTR    D5,[R0,#0] ; (result)
        Return  "","LinkNotStacked"

; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
; extern VFP_DOUBLE vfp_double_div(PC_VFP_DOUBLE a, PC_VFP_DOUBLE b);

        BeginExternal vfp_double_div
        ; LinkNotStacked - no FunctionEntry here
        VLDR    D6,[R1,#0] ; a
        VLDR    D7,[R2,#0] ; b
        VDIV.F64 D7,D6,D7
        VSTR    D7,[R0,#0] ; (result)
        Return  "","LinkNotStacked"

 [ {FALSE}
; extern void vfp_double_div_r(P_VFP_DOUBLE result, PC_VFP_DOUBLE a, PC_VFP_DOUBLE b);

        BeginExternal vfp_double_div_r
        ; LinkNotStacked - no FunctionEntry here
        VLDR    D6,[R1,#0] ; a
        VLDR    D7,[R2,#0] ; b
        VDIV.F64 D7,D6,D7
        VSTR    D7,[R0,#0] ; result
        Return  "","LinkNotStacked"
  ]

; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
; extern int        vfp_double_compare_zero(PC_VFP_DOUBLE a);

        BeginExternal vfp_double_compare_zero
        ; LinkNotStacked - no FunctionEntry here
        VLDR    D7,[R0,#0] ; a
        VCMP.F64 D7,#0.0
        MOV     R0,#0
        VMRS    APSR_nzcv,FPSCR
        Return  "","LinkNotStacked",EQ
        MOVHI   R0,#1      ; Greater Than, or Unordered
        MOVLO   R0,#-1     ; Less Than
        Return  "","LinkNotStacked"

; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
; extern VFP_DOUBLE vfp_double_mul_sub_mul(PC_VFP_DOUBLE a, PC_VFP_DOUBLE b, PC_VFP_DOUBLE c, PC_VFP_DOUBLE d);
; returns (a*b) - (c*d) which is useful for calculating the determinant of 3x3 matrix

        BeginExternal vfp_double_mul_sub_mul
        ; LinkNotStacked - no FunctionEntry here
        VLDR    D6,[R1,#0] ; a
        VLDR    D7,[R2,#0] ; b
        LDR     ip,[sp,#0] ; d is stacked
        VLDR    D4,[R3,#0] ; c
        VLDR    D5,[ip,#0] ; d
        VMUL.F64 D6,D6,D7  ; a*b
        VMUL.F64 D4,D4,D5  ; c*d
        VSUB.F64 D4,D6,D4  ; (a*b) - (c*d)
        VSTR    D4,[R0,#0] ; result
        Return  "","LinkNotStacked"

; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

        END
