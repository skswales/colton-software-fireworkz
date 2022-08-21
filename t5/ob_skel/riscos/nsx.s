; riscos/nsx.s (ARM assembler)

        AREA    |C$$code|, CODE, READONLY

        EXPORT  |_kernel_stkovf_split_0frame|
        EXPORT  |_kernel_stkovf_split|

        EXPORT  |__rt_stkovf_split_small|
        EXPORT  |__rt_stkovf_split_big|

        IMPORT  |raise|

SIGSTAK * 7

|_kernel_stkovf_split_0frame|
|_kernel_stkovf_split|
|__rt_stkovf_split_small|
|__rt_stkovf_split_big|
        MOV     ip, sp
        STMFD   sp!, {fp, ip, lr, pc}
        SUB     fp, ip, #4

        ;STR     r0, [r0, -r0]

        MOV     r0, #SIGSTAK
        BL      raise

        TEQ     R0, R0              ; sets Z (can be omitted if not in User mode)
        TEQ     pc, pc              ; EQ if in a 32-bit mode, NE if 26-bit
        LDMEQFD fp, {fp, sp, pc}    ; 32-bit compatible

        LDMFD   fp, {fp, sp, pc}^   ; 26-bit compatible

        END     ; of riscos/nsx.s
