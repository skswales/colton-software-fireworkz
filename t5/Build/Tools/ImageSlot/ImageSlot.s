; ImageSlot

; This Source Code Form is subject to the terms of the Mozilla Public
; License, v. 2.0. If a copy of the MPL was not distributed with this
; file, You can obtain one at http://mozilla.org/MPL/2.0/.

; Copyright (C) 1991-1998 Colton Software Limited
; Copyright (C) 1998-2015 R W Colton

        GBLL    No26bitCode
        GBLL    No32bitCode
No26bitCode SETL {FALSE}
No32bitCode SETL {TRUE}

        GET     Hdr:ListOpts
        GET     Hdr:CPU.Generic26
        GET     Hdr:CPU.Generic32

OS_Exit * &000011

XOS_Bit * 1 :SHL: 17

XOS_File * XOS_Bit :OR: &000008
XOS_GBPB * XOS_Bit :OR: &00000C
XOS_Find * XOS_Bit :OR: &00000D
XOS_ReadUnsigned * XOS_Bit :OR: &000021
XOS_ChangeEnvironment * XOS_Bit :OR: &000040
XOS_ConvertCardinal2 * XOS_Bit :OR: &0000D6

XWimp_SlotSize * XOS_Bit :OR: &0400EC

        ORG     0

; R12 based workspace

slotsize_for_load * 0
extra_bytes * 4
startup_bytes * 8
xxx * 12
yyy * 16
filename * 20
filesize * 24
filehandle * 28
header_offset * 32
just_checking * 36
aif_header * 40 ; 128 bytes

; AIF header layout

AIF_BL_DecompressedCode * 0*4
AIF_BL_SelfRelocCode * 1*4
AIF_BL_ZeroInitCode * 2*4
AIF_BL_ImageEntryPoint * 3*4
AIF_SWI_OS_Exit * 4*4
AIF_ImageReadOnlySize * 5*4
AIF_ImageReadWriteSize * 6*4
AIF_ImageDebugInfoSize * 7*4
AIF_ImageZeroInitSize * 8*4
AIF_ImageDebugType * 9*4
AIF_ImageBase * 10*4
AIF_Workspace * 11*4
AIF_Reserved1 * 12*4
AIF_Reserved2 * 13*4
AIF_Reserved3 * 14*4
AIF_Reserved4 * 15*4
AIF_ZeroInitCode * 16*4 ; 16 words

; +++++++++++++++++++++++++++++++++++++++++++++++++++++
; in:  R1 pointer to arguments

ImageSlot ROUT

        STMDB   R13!,{R14}

        MOV     R0,#0
        STR     R0,[R12,#extra_bytes]
        STR     R0,[R12,#startup_bytes]
        STR     R0,[R12,#just_checking]
        STR     R0,[R12,#header_offset]

10 ; skip leading spaces to start of first arg (filename)
        LDRB    R0,[R1],#1
        CMP     R0,#&20            ; =" "
        BEQ     %BT10
        SUB     R1,R1,#1

        ADRCC   R0,Error_Usage ; early end - first arg is mandatory
        BCC     ErrorExit

        STR     R1,[R12,#filename]

20 ; skip to end of first arg
        LDRB    R0,[R1],#1
        CMP     R0,#&21            ; ="!"
        BCS     %BT20
        SUB     R1,R1,#1

30 ; skip spaces to start of second arg
        LDRB    R0,[R1],#1
        CMP     R0,#&20            ; =" "
        BEQ     %BT30
        SUB     R1,R1,#1

        BCC     %FT50 ; no second arg present

        BL      ReadSize
        STR     R2,[R12,#extra_bytes]

40 ; skip spaces to start of third arg
        LDRB    R0,[R1],#1
        CMP     R0,#&20            ; =" "
        BEQ     %BT40
        SUB     R1,R1,#1

        BCC     %FT50 ; no third arg present

        TEQ     R0,#&2D            ; ="-" ; implies '-check'
        STREQ   R0,[R12,#just_checking]
        BEQ     %FT50

        BL      ReadSize
        STR     R2,[R12,#startup_bytes]

50 ; read file info
        MOV     R0,#17
        LDR     R1,[R12,#filename]
        SWI     XOS_File
        BVS     ErrorExit

        TEQ     R0,#1 ; if it's not a file, generate suitable error
        MOVNE   R2,R0
        MOVNE   R0,#19
        SWINE   XOS_File
        BVS     ErrorExit

        STR     R4,[R12,#filesize]
        CMP     R4,#&80
        BCC     UseCurrentFilesize

TryCurrentOffset ; looped back to from below

        MOV     R0,#&4F ; open file for reading, must exist etc.
        LDR     R1,[R12,#filename]
        SWI     XOS_Find
        BVS     ErrorExit

        STR     R0,[R12,#filehandle]

        LDR     R4,[R12,#header_offset]
        MOV     R4,R4,LSL #7 ; *= 128

        MOV     R0,#3 ; read 128 bytes of AIF header
        LDR     R1,[R12,#filehandle]
        ADD     R2,R12,#aif_header
        MOV     R3,#128
        SWI     XOS_GBPB

; close file handle, preserving error

        MOV     R6,R0
        MOVVC   R7,#0
        MOVVS   R7,#V_bit

        MOV     R0,#0
        LDR     R1,[R12,#filehandle]
        SWI     XOS_Find
        BVS     ErrorExit

        MOV     R0,R6
        TST     R7,#V_bit
        BNE     ErrorExit

; check AIF header validity
; first four words must be one of
; BL somewhere
; BLNV somewhere
; MOV R0,R0
; followed by SWI OS_Exit

        LDR     R1,MovR0R0Literal

        LDR     R0,[R12,#aif_header + AIF_BL_DecompressedCode]
        TEQ     R0,R1
        ANDNE   R0,R0,#&FF000000
        TEQNE   R0,#&EB000000
        TEQNE   R0,#&FB000000
        BNE     UseCurrentFilesize

        LDR     R0,[R12,#aif_header + AIF_BL_SelfRelocCode]
        TEQ     R0,R1
        ANDNE   R0,R0,#&FF000000
        TEQNE   R0,#&EB000000
        TEQNE   R0,#&FB000000
        BNE     UseCurrentFilesize

        LDR     R0,[R12,#aif_header + AIF_BL_ZeroInitCode]
        TEQ     R0,R1
        ANDNE   R0,R0,#&FF000000
        TEQNE   R0,#&EB000000
        TEQNE   R0,#&FB000000
        BNE     UseCurrentFilesize

        LDR     R0,[R12,#aif_header + AIF_BL_ImageEntryPoint]
        TEQ     R0,R1
        ANDNE   R0,R0,#&FF000000
        TEQNE   R0,#&EB000000
        TEQNE   R0,#&FB000000
        BNE     UseCurrentFilesize

        LDR     R0,[R12,#aif_header + AIF_SWI_OS_Exit]
        LDR     R1,OsExitLiteral
        TEQ     R0,R1
        BNE     TryNextOffset

; it's a good AIF header
; next four words are sizes (RO, RW, DBG, ZI)

        ADD     R0,R12,#aif_header + AIF_ImageReadOnlySize
        LDMIA   R0,{R0-R3}

        LDR     R4,[R12,#aif_header + AIF_ImageBase]

        ADD     R4,R4,R0 ; := address of end of RO area

        ADD     R4,R4,R1 ; := address of end of RW area

        ADD     R3,R3,#&2000       ; allow for a 8k stack
        ADD     R3,R3,#&027C       ; and &27C extra for RTL

; add the greater of (ZI + &227C) and DBG area size

        CMP     R2,R3
        ADDGE   R4,R4,R2
        ADDLT   R4,R4,R3

        SUB     R4,R4,#&8000 ; := required wimp slot size (slot starts at &8000)

; store this in 'filesize' if it's greater than the actual file size

        LDR     R0,[R12,#filesize]
        CMP     R4,R0
        STRGT   R4,[R12,#filesize]

; some programs (like old PipeDream 4's)
; have a dummy AIF header with app registration info
; followed by the real AIF header (app linked to &8080)

TryNextOffset
        LDR     R6,[R12,#header_offset]
        ADD     R6,R6,#1
        STR     R6,[R12,#header_offset]
        CMP     R6,#3
        BNE     TryCurrentOffset

; Finished with AIF header stuff

UseCurrentFilesize ROUT

        MVN     R0,#0 ; -1 (Read)
        BL      SlotSize
        STR     R0,[R12,#xxx]

        MOV     R0,#14 ; Application Space
        MOV     R1,#0 ; (Read)
        SWI     XOS_ChangeEnvironment
        SUB     R1,R1,#&8000 ; := size of application space (which starts at &8000)
        STR     R1,[R12,#yyy]

        LDR     R2,[R12,#xxx]
        CMP     R1,R2
        STRGT   R1,[R12,#xxx]
        STRGT   R2,[R12,#yyy]

; any 'startup_bytes' to consider on top of initial slot required?

        LDR     R2,[R12,#startup_bytes]
        TEQ     R2,#0
        BEQ     %FT60

; slotsize_for_load := filesize + extra_bytes + startup_bytes

        LDR     R1,[R12,#filesize]
        ADD     R1,R1,R2
        LDR     R2,[R12,#extra_bytes]
        ADD     R1,R1,R2
        STR     R1,[R12,#slotsize_for_load]

        LDR     R2,[R12,#yyy]
        LDR     R3,[R12,#xxx]
        CMP     R3,R2
        BEQ     %FT55

        CMP     R1,R2
        ADRGT   R0,Error_AppNeedsMoreMemory
        BGT     ErrorExit

        B       SuccessExit

55
        LDR     R0,[R12,#slotsize_for_load]
        BL      SlotSize

        MVN     R0,#0 ; -1 (Read)
        BL      SlotSize

        LDR     R2,[R12,#slotsize_for_load]
        CMP     R0,R2
        BCC     DoError_AppNeedsAtLeast

60 ; slotsize_for_load := filesize + extra_bytes

        LDR     R2,[R12,#extra_bytes]
        LDR     R1,[R12,#filesize]
        ADD     R1,R1,R2
        STR     R1,[R12,#slotsize_for_load]

        LDR     R2,[R12,#yyy]
        LDR     R3,[R12,#xxx]
        CMP     R3,R2
        BEQ     %FT70

        CMP     R1,R2
        ADRGT   R0,Error_AppNeedsMoreMemory
        BGT     ErrorExit

        B       SuccessExit

70
        LDR     R2,[R12,#just_checking]
        TEQ     R2,#0
        BNE     %FT80

; Set the slot size that we've determined we need in order to load app

        LDR     R0,[R12,#slotsize_for_load]
        BL      SlotSize

80
        MVN     R0,#0 ; -1 ( Read)
        BL      SlotSize

        LDR     R2,[R12,#slotsize_for_load]
        CMP     R0,R2
        BCC     DoError_AppNeedsAtLeast

; .....................................................

SuccessExit
        CMP     R14,R14 ; Clear V bit

        LDMIA   R13!,{PC}

; +++++++++++++++++++++++++++++++++++++++++++++++++++++

DoError_AppNeedsAtLeast ROUT

        LDR     R0,[R12,#xxx]
        BL      SlotSize
        B       . + 4

        LDR     R0,[R12,#slotsize_for_load]
        MOV     R1,#&0400          ; =1024
        SUB     R1,R1,#1           ; := 1023
        ADD     R0,R0,R1           ; round up to the next kb
        BIC     R0,R0,R1
        MOV     R0,R0,LSR #10      ; /= 1024

; convert R0 as number of kb needed in middle of error string

        ADR     R1,Error_AppNeedsAtLeastNumber
        MOV     R2,#7
        SWI     XOS_ConvertCardinal2

; copy the error suffix down in memory to the end of the number

        ADR     R0,Error_AppNeedsAtLeastSuffix
99
        LDRB    R2,[R0],#1
        STRB    R2,[R1],#1
        TEQ     R2,#0
        BNE     %BT99

        ADR     R0,Error_AppNeedsAtLeast

; .....................................................

ErrorExit
        MOV     R14,#0
        CMP     R14,#&80000000 ; Set V bit

        LDMIA   R13!,{PC}

; +++++++++++++++++++++++++++++++++++++++++++++++++++++

Error_AppNeedsAtLeast
        DCD &00000001
        DCB "Application needs at least "
Error_AppNeedsAtLeastNumber
        DCB "4194304"
Error_AppNeedsAtLeastSuffix
        DCB "K to start up",0

        ALIGN

; +++++++++++++++++++++++++++++++++++++++++++++++++++++

Error_Usage
        DCD &00000001
        DCB "Syntax: "
        DCB "*ImageSlot "
        DCB "<filename> "
        DCB "[<extra bytes>[K|M] "
        DCB "[<startup bytes>[K|M]]|[-check]]"
        DCB 0

        ALIGN

; +++++++++++++++++++++++++++++++++++++++++++++++++++++

Error_AppNeedsMoreMemory
        DCD &00000001
        DCB "Application will need more memory to run as a subprogram",0

        ALIGN

; +++++++++++++++++++++++++++++++++++++++++++++++++++++
; in:  R1 pointer to string to consider as size

; out: R0 corrupt
;      R1 updated past string
;      R2 number of bytes

ReadSize ROUT

        MOV     R2,#0
        SWI     XOS_ReadUnsigned
        BVS     ErrorExit

        LDRB    R0,[R1,#0]

; is it some form of kilobyte suffix?
        TEQ     R0,#&6B            ; ="k"
        TEQNE   R0,#&4B            ; ="K"
        ADDEQ   R1,R1,#1
        MOVEQ   R2,R2,LSL #10

; is it some form of megabyte suffix?
        TEQ     R0,#&6D            ; ="m"
        TEQNE   R0,#&4D            ; ="M"
        ADDEQ   R1,R1,#1
        MOVEQ   R2,R2,LSL #20

        MOV     PC,R14

; +++++++++++++++++++++++++++++++++++++++++++++++++++++
; in:  R0 -1 Read
;      R0 >0 Set

SlotSize ROUT

        MVN     R1,#0 ; -1
        MVN     R2,#0 ; -1
        SWI     XWimp_SlotSize
        MOV     PC,R14

; +++++++++++++++++++++++++++++++++++++++++++++++++++++

MovR0R0Literal
        MOV     R0,R0

; +++++++++++++++++++++++++++++++++++++++++++++++++++++

OsExitLiteral
        SWI     OS_Exit

; +++++++++++++++++++++++++++++++++++++++++++++++++++++

IdString
        DCB "ImageSlot 0.12",0

        ALIGN

; +++++++++++++++++++++++++++++++++++++++++++++++++++++

        END
