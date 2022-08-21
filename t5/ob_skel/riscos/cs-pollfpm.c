; cs-pollfpm.s

; This Source Code Form is subject to the terms of the Mozilla Public
; License, v. 2.0. If a copy of the MPL was not distributed with this
; file, You can obtain one at http://mozilla.org/MPL/2.0/.

; Copyright (C) 1989-1998 Colton Software Limited
; Copyright (C) 1998-2015 R W Colton

; SKS for Colton Software optimised use now we can rely on /fpe3 LFM/SFM

; Automagically does hourglass off/on on entry/exit

        GET     as_flags_h

        ;GET     as_regs_h
        GET     as_macro_h

XOS_Bit                 * 1 :SHL: 17

XOS_ReadMonotonicTime   * XOS_Bit :OR: &00042

XWimp_Poll              * XOS_Bit :OR: &400C7
XWimp_PollIdle          * XOS_Bit :OR: &400E1

XHourglass_Off          * XOS_Bit :OR: &406C1
XHourglass_Start        * XOS_Bit :OR: &406C3

 [ PROFILING
; Keep consistent with cmodules/profile.h
XProfiler_On            * XOS_Bit :OR: &88001
XProfiler_Off           * XOS_Bit :OR: &88002
 ]

                GBLL    POLL_HOURGLASS      ; Whether to do hourglass off/on over polling
POLL_HOURGLASS  SETL    {TRUE}

AUTO_POLLIDLE_TIME  * 2 ; cs delay till next null event

Wimp_Poll_NullMask  * 1 ; (1 << Wimp_ENull)

Wimp_ERedrawWindow  * 1
NORMAL_DELAY_PERIOD * 100 ; /* more suitable for normal apps than 33cs default */
REDRAW_DELAY_PERIOD * 200 ; /* default (cs) */

        AREA    |C$$code|,CODE,READONLY

; +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
; extern _kernel_oserror *
; wimp_poll_coltsoft(
; r0 a1     _In_        wimp_emask mask,
; r1 a2     _Out_       WimpPollBlock * block,
; r2 a3     _Inout_     int * pollword,
; r3 a4     _Out_       int * event_code);

        BeginExternal wimp_poll_coltsoft

        FunctionEntry "v1,v2,v3","MakeFrame"

        MOV     v1, r0                      ; save mask
        MOV     v2, r2                      ; save pollword^
        MOV     v3, r3                      ; save event_code^

 [ PROFILING
        SWI     XProfiler_Off
 ]
 [ POLL_HOURGLASS
        SWI     XHourglass_Off
 ]
        SUB     sp, sp, #4*((4*3)+1)
        RFS     ip
        STR     ip, [sp, #0]
        SFM     f4, 4, [sp, #4*1]           ; saves extended F4,F5,F6,F7

        TST     v1, #Wimp_Poll_NullMask
        BEQ     wimp_poll_coltsoft_auto_pollidle

        MOV     r0, v1                      ; restore mask
;               r1 still block^
;               r2 unused
        MOV     r3, v2                      ; get pollword^ in correct register for SWI
        SWI     XWimp_Poll

wimp_poll_coltsoft_common_exit ; comes here from below too

        STR     r0, [v3, #0]                ; *event_code^ = rc (or error^);
        MOVVS   v1, r0                      ; save error^
        MOVVC   v1, #0                      ; save zero -> no error

        LDR     ip, [sp, #0]
        WFS     ip
        LFM     f4, 4, [sp, #4*1]
        ADD     sp, sp, #4*(12+1)           ; restores extended F4,F5,F6,F7

 [ POLL_HOURGLASS
        TEQ     r0, #Wimp_ERedrawWindow
        MOVEQ   r0, #REDRAW_DELAY_PERIOD
        MOVNE   r0, #NORMAL_DELAY_PERIOD
        SWI     XHourglass_Start
 ]
 [ PROFILING
        SWI     XProfiler_On
 ]

        MOV     r0, v1                      ; restore error^ or zero

        FunctionReturn "v1,v2,v3","fpbased"

wimp_poll_coltsoft_auto_pollidle ; branched here from null event mask case of above

        SWI     XOS_ReadMonotonicTime
        ADD     r2, r0, #AUTO_POLLIDLE_TIME ; return nulls only every so often

        MOV     r0, v1                      ; restore mask
;               r1 still block^
;               r2 has just been set up
        MOV     r3, v2                      ; get pollword^ in correct register for SWI
        SWI     XWimp_PollIdle

        B       wimp_poll_coltsoft_common_exit

; +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 [ PROFILING
        BeginInternal wimp_poll_coltsoft_ends_here
 ]

        END     ; of s.cs-pollfpm
