; riscos/ho_asm.s (ARM assembler)

; This Source Code Form is subject to the terms of the Mozilla Public
; License, v. 2.0. If a copy of the MPL was not distributed with this
; file, You can obtain one at http://mozilla.org/MPL/2.0/.

; Copyright (C) 1988-1998 Colton Software Limited
; Copyright (C) 1998-2015 R W Colton

        GET     as_flags_h

        ;GET     as_regs_h
        GET     as_macro_h

XOS_Bit                 * 1 :SHL: 17

XOS_WriteC              * XOS_Bit :OR: &000000
XOS_Control             * XOS_Bit :OR: &00000f
XOS_ReadMonotonicTime   * XOS_Bit :OR: &000042
XOS_Plot                * XOS_Bit :OR: &000045
XOS_WriteN              * XOS_Bit :OR: &000046

XFont_LoseFont          * XOS_Bit :OR: &040082
XFont_SetFont           * XOS_Bit :OR: &04008A

XWimp_SlotSize          * XOS_Bit :OR: &0400ec

XHourglass_On           * XOS_Bit :OR: &0406c0
XHourglass_Off          * XOS_Bit :OR: &0406c1
XHourglass_Smash        * XOS_Bit :OR: &0406c2
XHourglass_Start        * XOS_Bit :OR: &0406c3
XHourglass_Percentage   * XOS_Bit :OR: &0406c4

; +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

        AREA    |C$$zidata|,DATA,NOINIT

; +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

ctrlflagp
        %       4

; +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

        AREA    |C$$code|,CODE,READONLY

; +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
; extern void EscH(
; r0 a1     int * ctrlflg);

        BeginExternal EscH

        ; LinkNotStacked - no FunctionEntry here

        LDR     ip, =ctrlflagp
        STR     a1, [ip, #0]        ; save address of flag

        MOV     r0, #0
        MOV     r1, #0
        ADR     r2, EscapeHandler
        MOV     r3, #0
        SWI     XOS_Control

        FunctionReturn "","LinkNotStacked"

; +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
; Will be in SVC32 on 32-bit systems

        BeginInternal EscapeHandler

        STR     r0, [r13, #-4]! ; r13_svc

        AND     r11, r11, #&40  ; set flag or clear flag on bit 6 of R11
        LDR     r0, =ctrlflagp
        LDR     r0, [r0]
        STR     r11, [r0, #0]

        LDR     r0, [r13], #4   ; r13_svc
        MOV     pc, lr          ; 26-bit or 32-bit return; flags have not been modified

; +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
; extern void *
; EventH(void);

        BeginExternal EventH

        ; LinkNotStacked - no FunctionEntry here

        MOV     r0, #0
        MOV     r1, #0
        MOV     r2, #0
        ADR     r3, EventHandler
        MOV     ip, r3
        SWI     XOS_Control
        CMP     ip, r3
        MOVEQ   r0, #0
        MOVNE   r0, r3

        FunctionReturn "","LinkNotStacked"

; +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
; Will be in SVC32 / IRQ32 on 32-bit systems

        BeginInternal EventHandler

        MOV     pc, lr          ; 26-bit or 32-bit return; flags have not been modified

 [ {FALSE}
        TEQ     r0, r0          ; sets Z (can be omitted if not in User mode)
        TEQ     pc, pc          ; EQ if in a 32-bit mode, NE if 26-bit
        MOVEQ   pc, lr          ; 32-bit return

        MOVS    pc, lr          ; 26-bit return, preserving flags
 ]

; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++
; typedef U32 MONOTIME;

; extern MONOTIME
; monotime(void);

        BeginExternal monotime

        ; LinkNotStacked - no FunctionEntry here

        SWI     XOS_ReadMonotonicTime

;;; MOV a1,a1, LSR # 4 ; slow down time by 16

        FunctionReturn "","LinkNotStacked"

; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++
; extern os_error *
; os_swix(
; r0 a1     int swicode,
; r1 a2     os_regset * r);

        IMPORT  _kernel_swi

        BeginExternal os_swix

        MOV     a3, a2
        B       _kernel_swi

; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++

        BeginExternal bbc_vdu

        ; LinkNotStacked - no FunctionEntry here

        SWI     XOS_WriteC
        MOVVC   a1, #0

        FunctionReturn "","LinkNotStacked"

; ++++++++++++++++++++++++
; extern _kernel_oserror *
; os_writeN(
; r0 a1     const char * s
; r1 a2     U32 count

        BeginExternal os_writeN

        ; LinkNotStacked - no FunctionEntry here

        SWI     XOS_WriteN
        MOVVC   a1, #0

        FunctionReturn "","LinkNotStacked"

; ++++++++++++++++++++++++
; extern _kernel_oserror *
; os_plot(
; r0 a1     int plot_code
; r1 a2     int x
; r2 a3     int y

        BeginExternal os_plot

        ; LinkNotStacked - no FunctionEntry here

        SWI     XOS_Plot
        MOVVC   a1, #0

        FunctionReturn "","LinkNotStacked"

; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++

        BeginExternal riscos_hourglass_off ; (void)

        ; LinkNotStacked - no FunctionEntry here

        SWI     XHourglass_Off

        FunctionReturn "","LinkNotStacked"

; ++++++++++++++++++++++++

        BeginExternal riscos_hourglass_on ; (void)

        ; LinkNotStacked - no FunctionEntry here

        SWI     XHourglass_On

        FunctionReturn "","LinkNotStacked"

; ++++++++++++++++++++++++

        BeginExternal riscos_hourglass_percent ; (a1 = p)

        ; LinkNotStacked - no FunctionEntry here

        SWI     XHourglass_Percentage

        FunctionReturn "","LinkNotStacked"

; ++++++++++++++++++++++++

        BeginExternal visdelay_percent ; (a1 = p) keep PDM happy

        ; LinkNotStacked - no FunctionEntry here

        SWI     XHourglass_Percentage

        FunctionReturn "","LinkNotStacked"

; ++++++++++++++++++++++++

        BeginExternal riscos_hourglass_smash ; (void)

        ; LinkNotStacked - no FunctionEntry here

        SWI     XHourglass_Smash

        FunctionReturn "","LinkNotStacked"

; ++++++++++++++++++++++++

        BeginExternal riscos_hourglass_start ; (a1 = after_cs)

        ; LinkNotStacked - no FunctionEntry here

        SWI     XHourglass_Start

        FunctionReturn "","LinkNotStacked"

; ++++++++++++++++++++++++

 [ PROFILING
        BeginInternal riscos_hourglass_ends_here
 ]

; +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
; extern _kernel_oserror *
; font_LoseFont(
; r0 a1     int host_font);

        BeginExternal font_LoseFont ; (a1 = host_font)

        ; LinkNotStacked - no FunctionEntry here

        SWI     XFont_LoseFont
        MOVVC   a1, #0

        FunctionReturn "","LinkNotStacked"

; +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
; extern _kernel_oserror *
; font_SetFont(
; r0 a1     int host_font);

        BeginExternal font_SetFont ; (a1 = host_font)

        ; LinkNotStacked - no FunctionEntry here

        SWI     XFont_SetFont
        MOVVC   a1, #0

        FunctionReturn "","LinkNotStacked"

; +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

        END ; of riscos/ho_asm.s
