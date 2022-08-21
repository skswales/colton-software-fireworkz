/* fl_123.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1988-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Lotus 1-2-3 spreadsheet load object module for Fireworkz */

/* MRJC original 123 converter February 1988 / for Fireworkz July 1993 */

#include "common/gflags.h"

#include "fl_123/fl_123.h"

#include "ob_skel/ff_io.h"

#ifndef          __ev_eval_h
#include "cmodules/ev_eval.h"
#endif

#ifndef          __typepack_h
#include "cmodules/typepack.h"
#endif

#if RISCOS
#define MSG_WEAK &rb_fl_123_msg_weak
extern PC_U8 rb_fl_123_msg_weak;
#endif
#define P_BOUND_RESOURCES_OBJECT_ID_FL_LOTUS123 NULL

/*
internal routines
*/

_Check_return_
static STATUS
lotus123_check_date(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _InRef_     PC_BYTE p_format,
    _InRef_     PC_F64 p_f64);

_Check_return_
static STATUS
lotus123_decode_format(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock_format /*appended*/,
    _InRef_     PC_BYTE p_format);

_Check_return_
static STATUS
lotus123_decode_formula(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    /*in*/      PC_BYTE p_formula_size,
    _InRef_     PC_SLR p_slr);

_Check_return_
static U32 /* length out */
text_from_f64(
    _Out_writes_z_(elemof_buffer) P_USTR buffer,
    _InVal_     U32 elemof_buffer,
    _InRef_     PC_F64 p_f64);

enum SEARCH_TYPES
{
    SEARCH_TYPE_MATCH,
    SEARCH_NEXT_ROW
};

enum LOTUS123_DATA_TYPES
{
    LOTUS123_DATA_TEXT,
    LOTUS123_DATA_SS
};

enum LOTUS123_FORMAT_TYPES
{
    LOTUS123_FORMAT_NONE,
    LOTUS123_FORMAT_LEFT,
    LOTUS123_FORMAT_CENTRE,
    LOTUS123_FORMAT_RIGHT
};

/* Lotus file headers */

static const BYTE lf_BOF_opcode[] = { '\x00', '\x00' };

/* files with original 1-2-3 opcodes */

static const BYTE lf_BOF_WK1[]      = { '\x02', '\x00', '\x04', '\x04' }; /* 1-2-3 file */
static const BYTE lf_BOF_S[]        = { '\x02', '\x00', '\x05', '\x04' }; /* Symphony file */
static const BYTE lf_BOF_S2[]       = { '\x02', '\x00', '\x06', '\x04' }; /* Symphony file */

/* files with new 1-2-3 opcodes */

static const BYTE lf_BOF_WK3[]      = { '\x1A', '\x00', '\x00', '\x10', '\x04', '\x00' };

static const BYTE lf_BOF_123_4[]    = { '\x1A', '\x00', '\x02', '\x10', '\x04', '\x00' };
static const BYTE lf_BOF_123_6[]    = { '\x1A', '\x00', '\x03', '\x10', '\x04', '\x00' };
static const BYTE lf_BOF_123_7[]    = { '\x1A', '\x00', '\x04', '\x10', '\x04', '\x00' };
static const BYTE lf_BOF_123_SS98[] = { '\x1A', '\x00', '\x05', '\x10', '\x04', '\x00' };

static S32 lf_version = 1;

#define MAXSTACK 100

static P_P_USTR arg_stack; /* [MAXSTACK]*/

_Check_return_
static inline U16
v3_map_opcode(
    _InVal_     U16 opcode)
{
    switch(opcode)
    {
    default:                return(L_BOF);
    case LWK3_EOF:          return(L_EOF);
    case LWK3_LABEL:        return(L_LABEL);
    case LWK3_NUMBER:       return(L_NUMBER);
    case LWK3_SMALLNUMBER:  return(L_NUMBER);
    case LWK3_FORMULA:      return(L_FORMULA);
    }
}

_Check_return_
static inline U16
v123_map_opcode(
    _InVal_     U16 opcode)
{
    switch(opcode)
    {
    default:                return(L_BOF);
    case L123_EOF:          return(L_EOF);
    case L123_LABEL:        return(L_LABEL);
    case L123_NUMBER:       return(L_NUMBER);
    case L123_IEEENUMBER:   return(L_NUMBER);
    case L123_FORMULA:      return(L_FORMULA);
    }
}

/******************************************************************************
*
* read a byte from memory
*
******************************************************************************/

_Check_return_
static inline U8
lotus123_read_U8(
    _In_reads_bytes_c_(1) PC_BYTE p)
{
    return(PtrGetByte(p));
}

/******************************************************************************
*
* read a signed word from memory
*
******************************************************************************/

_Check_return_
static S16
lotus123_read_S16(
    _In_reads_bytes_c_(2) PC_BYTE p)
{
    union _lotus123_read_s16
    {
        S16 s16;
        BYTE bytes[2];
    } u;

    u.bytes[0] = PtrGetByteOff(p, 0);
    u.bytes[1] = PtrGetByteOff(p, 1);

    return(u.s16);
}

/******************************************************************************
*
* read an unsigned word from memory
*
******************************************************************************/

_Check_return_
static U16
lotus123_read_U16(
    _In_reads_bytes_c_(2) PC_BYTE p)
{
    union _lotus123_read_u16
    {
        U16 u16;
        BYTE bytes[2];
    } u;

    u.bytes[0] = PtrGetByteOff(p, 0);
    u.bytes[1] = PtrGetByteOff(p, 1);

    return(u.u16);
}

/******************************************************************************
*
* read a double from memory
*
******************************************************************************/

_Check_return_
static F64
lotus123_read_F64(
    _In_reads_bytes_c_(8) PC_BYTE p)
{
    union _lotus123_read_f64
    {
        F64 f64;
        BYTE bytes[8];
        S32 wordz[2];
    } u;

#if RISCOS

    /* this for the 8087 -> ARM */
    u.bytes[4] = *p++;
    u.bytes[5] = *p++;
    u.bytes[6] = *p++;
    u.bytes[7] = *p++;

    u.bytes[0] = *p++;
    u.bytes[1] = *p++;
    u.bytes[2] = *p++;
    u.bytes[3] = *p++;

    if(((u.wordz[0] & 0x7FFFFFFF) >> 20) == 0x7FF)
        u.f64 = 0.0;

#elif WINDOWS

    u.bytes[0] = *p++;
    u.bytes[1] = *p++;
    u.bytes[2] = *p++;
    u.bytes[3] = *p++;

    u.bytes[4] = *p++;
    u.bytes[5] = *p++;
    u.bytes[6] = *p++;
    u.bytes[7] = *p++;

#endif /* OS */

    return(u.f64);
}

/******************************************************************************
*
* read a long double from memory as a double
*
******************************************************************************/

_Check_return_
static F64
lotus123_read_F64_from_x86_extended_precision(
    _In_reads_bytes_c_(10) PC_BYTE p)
{
    union _lotus123_read_f64
    {
        F64 f64;
        BYTE bytes[8];
        S32 wordz[2];
    } u;

    U32 b;

    /* source: [79] sign; [78:64] exponent - fifteen bits; [63] int part; [62:0] mantissa */
    /* target: [63] sign; [62:52] exponent - eleven bits;                 [51:0] mantissa (without integer part) */

#if RISCOS

    /* this for the 8087 E.P. -> ARM (hi/lo words of double are swapped) */

    /* mantissa */
    /* nothing taken from p[0] */       /* discards source [7:0] */
    b  = ((U32) p[1] << 0);

    b |= ((U32) p[2] << 8);
    b >>= 3;
    u.bytes[0 +4] = (BYTE) ((b) & 0xFF);        /* store source [18:11], discards source [10:8] */
    b >>= (8-3);

    b |= ((U32) p[3] << 8);
    b >>= 3;
    u.bytes[1 +4] = (BYTE) ((b >> 3) & 0xFF);   /* store source [26:19] */
    b >>= (8-3);

    b |= ((U32) p[4] << 8);
    b >>= 3;
    u.bytes[2 +4] = (BYTE) ((b >> 3) & 0xFF);   /* store source [34:27] */
    b >>= (8-3);

    b |= ((U32) p[5] << 8);
    b >>= 3;
    u.bytes[3 +4] = (BYTE) ((b >> 3) & 0xFF);   /* store source [42:35] */
    b >>= (8-3);

    b |= ((U32) p[6] << 8);
    b >>= 3;
    u.bytes[4 -4] = (BYTE) ((b >> 3) & 0xFF);   /* store source [50:43] */
    b >>= (8-3);

    b |= ((U32) p[7] << 8);
    b >>= 3;
    u.bytes[5 -4] = (BYTE) ((b >> 3) & 0xFF);   /* store source [58:51] */
    b >>= (8-3);

    u.bytes[6 -4] = (BYTE) ((b >> 3) & 0x0F);   /* store source [62:59], discards source [63] */

    /* sign */
    u.bytes[7 -4]  = p[9] & 0x80;       /* store source [79] */

    /* exponent-lo */
    b  = ((U32) p[8] << 0);             /* source [71:64] */

    /* exponent-hi */
    b |= ((U32) (p[9] & 0x7F) << 8);    /* source [78:72] */

    /* change bias */
    b -= 0x4000; /* valid ex.pr. exponent bits in [14:0] */
    b += 0x0400; /* valid double exponent bits in [10:0] */

    u.bytes[6 -4] |= (b << 4) & 0xF0;   /* double exponent [3:0] */
    u.bytes[7 -4] |= (b >> 4) & 0x7F;   /* double exponent [10:4] */

#elif WINDOWS

    /* mantissa */
    /* nothing taken from p[0] */           /* discards source [7:0] */
    b  = ((U32) p[1] << 0);

    b |= ((U32) p[2] << 8);
    b >>= 3;
    u.bytes[0] = (BYTE) ((b) & 0xFF);       /* store source [18:11], discards source [10:8] */
    b >>= (8-3);

    b |= ((U32) p[3] << 8);
    b >>= 3;
    u.bytes[1] = (BYTE) ((b >> 3) & 0xFF);  /* store source [26:19] */
    b >>= (8-3);

    b |= ((U32) p[4] << 8);
    b >>= 3;
    u.bytes[2] = (BYTE) ((b >> 3) & 0xFF);  /* store source [34:27] */
    b >>= (8-3);

    b |= ((U32) p[5] << 8);
    b >>= 3;
    u.bytes[3] = (BYTE) ((b >> 3) & 0xFF);  /* store source [42:35] */
    b >>= (8-3);

    b |= ((U32) p[6] << 8);
    b >>= 3;
    u.bytes[4] = (BYTE) ((b >> 3) & 0xFF);  /* store source [50:43] */
    b >>= (8-3);

    b |= ((U32) p[7] << 8);
    b >>= 3;
    u.bytes[5] = (BYTE) ((b >> 3) & 0xFF);  /* store source [58:51] */
    b >>= (8-3);

    u.bytes[6] = (BYTE) ((b >> 3) & 0x0F);  /* store source [62:59], discards source [63] */

    /* sign */
    u.bytes[7]  = p[9] & 0x80;              /* store source [79] */

    /* exponent-lo */
    b  = ((U32) p[8] << 0);                 /* source [71:64] */

    /* exponent-hi */
    b |= ((U32) (p[9] & 0x7F) << 8);        /* source [78:72] */

    /* change bias */
    b -= 0x4000; /* valid ex.pr. exponent bits in [14:0] */
    b += 0x0400; /* valid double exponent bits in [10:0] */

    u.bytes[6] |= (b << 4) & 0xF0;          /* double exponent [3:0] */
    u.bytes[7] |= (b >> 4) & 0x7F;          /* double exponent [10:4] */

#endif /* OS */

    return(u.f64);
}

/******************************************************************************
*
* load all the cells we find going through the file
*
* SKS new routine 02feb96 replaced core of below fn
*
******************************************************************************/

_Check_return_
static STATUS
lotus123_load_all_cells_wk1(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_ARRAY_HANDLE p_h_data,
    _InoutRef_  P_PROCESS_STATUS p_process_status)
{
    STATUS status = STATUS_OK;
    const U32 n_file_bytes = array_elements32(p_h_data);
    PC_BYTE p_file_start = array_rangec(p_h_data, BYTE, 0, n_file_bytes);
    U32 opcode_offset = 0;
    U16 opcode = L_NONE;
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock_result, 50);
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock_formula, 200);
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock_format, 50);
    quick_ublock_with_buffer_setup(quick_ublock_result);
    quick_ublock_with_buffer_setup(quick_ublock_formula);
    quick_ublock_with_buffer_setup(quick_ublock_format);

    for(;;)
    {
        PC_BYTE p_data = P_BYTE_NONE;
        PC_BYTE p_format = P_BYTE_NONE;
        U8 data_type = LOTUS123_DATA_SS;
        U8 format_type = LOTUS123_FORMAT_NONE;
        BOOL is_date_format = FALSE;
        BOOL looping = TRUE;
        SLR slr;

        if(opcode_offset >= n_file_bytes)
            break;

        /* search for the right sort of opcode */
        do  {
            PC_BYTE p_opcode = p_file_start + opcode_offset;
            U16 length;

            p_data = p_opcode;

            /* read opcode */
            opcode = lotus123_read_U16(p_data);
            p_data += 2;

            length = lotus123_read_U16(p_data);
            p_data += 2;

            switch(opcode)
            {
            case L_EOF:
                looping = FALSE;
                break;

            case L_INTEGER:
            case L_NUMBER:
            case L_LABEL:
            case L_FORMULA:
                p_format = p_data; /* skip format byte */
                p_data += 1;

                slr.col = (COL) lotus123_read_U16(p_data);
                p_data += 2;

                slr.row = (ROW) lotus123_read_U16(p_data);
                p_data += 2;

                /* p_data points to cell contents */

                looping = FALSE;
                break;

            default:
                break;
            }

            opcode_offset += (length + 4);
        }
        while(looping);

        if(L_EOF == opcode)
            break; /* that's all folks!!! */

        /* deal with different cell types */
        switch(opcode)
        {
        case L_INTEGER:
            {
            S16 s16 = lotus123_read_S16(p_data);
            F64 f64 = (F64) s16;

            status = lotus123_decode_format(&quick_ublock_format, p_format);

            if(status_ok(status))
                status = lotus123_check_date(&quick_ublock_result, p_format, &f64);

            if(status == 1)
                is_date_format = TRUE;
            else if(status == 0)
                status = quick_ublock_printf(&quick_ublock_result, USTR_TEXT(S16_FMT), s16);

            break;
            }

        case L_NUMBER:
        case L_FORMULA:
            {
            F64 f64 = lotus123_read_F64(p_data);

            status = lotus123_decode_format(&quick_ublock_format, p_format);

            if(status_ok(status))
                status = lotus123_check_date(&quick_ublock_result, p_format, &f64);

            if(status == 1)
                is_date_format = TRUE;
            else if(status == 0)
            {
                UCHARZ buffer[50];
                const U32 len = text_from_f64(ustr_bptr(buffer), elemof32(buffer), &f64);
                status = quick_ublock_uchars_add(&quick_ublock_result, ustr_bptr(buffer), len);
            }

            if(opcode == L_FORMULA)
                status = lotus123_decode_formula(&quick_ublock_formula, p_data + 8, &slr);

            break;
            }

        case L_LABEL:
            {
            PC_SBSTR sbstr = PtrAddBytes(PC_SBSTR, p_data, 1);

            data_type = LOTUS123_DATA_TEXT;

            /* deal with label alignment byte */
            switch(p_data[0])
            {
            default: default_unhandled();
            case CH_APOSTROPHE:
                break;

            case CH_QUOTATION_MARK:
                format_type = LOTUS123_FORMAT_RIGHT;
                break;

            case CH_CIRCUMFLEX_ACCENT:
                format_type = LOTUS123_FORMAT_CENTRE;
                break;

            case CH_BACKWARDS_SLASH:
                /* repeating */
                break;
            }

            status = quick_ublock_sbstr_add_n(&quick_ublock_result, sbstr, strlen_without_NULLCH);

            break;
            }
        }

        /* need to CH_NULL-terminate to pass via insert_cell_contents_foreign */
        if(status_ok(status) && (0 != quick_ublock_bytes(&quick_ublock_result)))
            status = quick_ublock_nullch_add(&quick_ublock_result);

        if(status_ok(status) && (0 != quick_ublock_bytes(&quick_ublock_formula)))
            status = quick_ublock_nullch_add(&quick_ublock_formula);

        if(status_ok(status))
        {
            if(quick_ublock_bytes(&quick_ublock_result) || quick_ublock_bytes(&quick_ublock_formula))
            {
                LOAD_CELL_FOREIGN load_cell_foreign;
                OBJECT_ID object_id;

                zero_struct(load_cell_foreign);
                status_consume(object_data_from_slr(p_docu, &load_cell_foreign.object_data, &slr));
                load_cell_foreign.original_slr = slr;

                switch(data_type)
                {
                default: default_unhandled();
#if CHECKING
                case LOTUS123_DATA_TEXT:
#endif
                    object_id = OBJECT_ID_TEXT;
                    load_cell_foreign.data_type = OWNFORM_DATA_TYPE_TEXT;
                    load_cell_foreign.ustr_inline_contents = quick_ublock_ustr_inline(&quick_ublock_result);
                    break;

                case LOTUS123_DATA_SS:
                    object_id = OBJECT_ID_SS;
                    load_cell_foreign.data_type = OWNFORM_DATA_TYPE_FORMULA;

                    /* this determines whether cells are recalced after load - supply some
                     * contents and the evaluator won't bother to recalc the cell
                     */
                    load_cell_foreign.ustr_formula = quick_ublock_bytes(&quick_ublock_formula)
                                                   ? quick_ublock_ustr(&quick_ublock_formula)
                                                   : NULL;

                    if(NULL == load_cell_foreign.ustr_formula)
                        load_cell_foreign.ustr_inline_contents =
                            quick_ublock_bytes(&quick_ublock_result)
                                ? quick_ublock_ustr_inline(&quick_ublock_result)
                                : NULL;

                    if(0 != quick_ublock_bytes(&quick_ublock_format))
                    {
                        PC_USTR ustr_format = quick_ublock_ustr(&quick_ublock_format);

                        if(CH_NULL != PtrGetByte(ustr_format))
                        {
                            ARRAY_HANDLE_USTR array_handle_ustr = 0;

                            if(status_ok(status = al_ustr_set(&array_handle_ustr, ustr_format)))
                            {
                                if(is_date_format)
                                {
                                    style_bit_set(&load_cell_foreign.style, STYLE_SW_PS_NUMFORM_DT);
                                    load_cell_foreign.style.para_style.h_numform_dt = array_handle_ustr;
                                }
                                else
                                {
                                    style_bit_set(&load_cell_foreign.style, STYLE_SW_PS_NUMFORM_NU);
                                    load_cell_foreign.style.para_style.h_numform_nu = array_handle_ustr;
                                }
                            }

                            /* NB handle will be donated to style system */
                        }
                    }

                    break;
                }

                status = insert_cell_contents_foreign(p_docu, object_id, T5_MSG_LOAD_CELL_FOREIGN, &load_cell_foreign);
            }
        }

        p_process_status->data.percent.current = ((S32) 100 * (S32) opcode_offset) / n_file_bytes;
        process_status_reflect(p_process_status);

        quick_ublock_dispose(&quick_ublock_result);
        quick_ublock_dispose(&quick_ublock_formula);
        quick_ublock_dispose(&quick_ublock_format);

        status_break(status);
    }

    assert(opcode_offset <= n_file_bytes);

    quick_ublock_dispose(&quick_ublock_result);
    quick_ublock_dispose(&quick_ublock_formula);
    quick_ublock_dispose(&quick_ublock_format);

    return(status);
}

_Check_return_
static STATUS
lotus123_load_all_cells_wk3(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_ARRAY_HANDLE p_h_data,
    _InoutRef_  P_PROCESS_STATUS p_process_status)
{
    STATUS status = STATUS_OK;
    const U32 n_file_bytes = array_elements32(p_h_data);
    PC_BYTE p_file_start = array_rangec(p_h_data, BYTE, 0, n_file_bytes);
    U32 opcode_offset = 0;
    U16 opcode = L_NONE;
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock_result, 50);
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock_formula, 200);
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock_format, 50);
    quick_ublock_with_buffer_setup(quick_ublock_result);
    quick_ublock_with_buffer_setup(quick_ublock_formula);
    quick_ublock_with_buffer_setup(quick_ublock_format);

    for(;;)
    {
        PC_BYTE p_data = P_BYTE_NONE;
        PC_BYTE p_format = P_BYTE_NONE;
        U8 data_type = LOTUS123_DATA_SS;
        U8 format_type = LOTUS123_FORMAT_NONE;
        BOOL is_date_format = FALSE;
        BOOL looping = TRUE;
        SLR slr;

        if(opcode_offset >= n_file_bytes)
            break;

        /* search for the right sort of opcode */
        do  {
            PC_BYTE p_opcode = p_file_start + opcode_offset;
            U16 length;

            p_data = p_opcode;

            /* read opcode */
            opcode = lotus123_read_U16(p_data);
            p_data += 2;

            length = lotus123_read_U16(p_data);
            p_data += 2;

            switch(opcode)
            {
            case LWK3_EOF:
                looping = FALSE;
                break;

            case LWK3_LABEL:
                {
                static const U8 format = L_SPECL | L_GENFMT;
                p_format = &format;

                slr.row = (ROW) lotus123_read_U16(p_data);
                p_data += 2;

                p_data += 1;

                slr.col = (COL) lotus123_read_U8(p_data);
                p_data += 1;

                /* p_data points to cell contents */

                looping = FALSE;
                break;
                }

            case LWK3_NUMBER:
            case LWK3_FORMULA:
                {
                static const U8 format = L_SPECL | L_GENFMT;
                p_format = &format;

                slr.row = (ROW) lotus123_read_U16(p_data);
                p_data += 2;

                p_data += 1;

                slr.col = (COL) lotus123_read_U8(p_data);
                p_data += 1;

                /* p_data points to cell contents */

                looping = FALSE;
                break;
                }

            default:
                break;
            }

            opcode_offset += (length + 4);
        }
        while(looping);

        if(LWK3_EOF == opcode)
            break; /* that's all folks!!! */

        /* deal with different cell types */
        switch(opcode)
        {
        case LWK3_NUMBER:
        case LWK3_FORMULA:
            {
            F64 f64 = lotus123_read_F64_from_x86_extended_precision(p_data);

            status = lotus123_decode_format(&quick_ublock_format, p_format);

            if(status_ok(status))
                status = lotus123_check_date(&quick_ublock_result, p_format, &f64);

            if(status == 1)
                is_date_format = TRUE;
            else if(status == 0)
            {
                UCHARZ buffer[50];
                const U32 len = text_from_f64(ustr_bptr(buffer), elemof32(buffer), &f64);
                status = quick_ublock_uchars_add(&quick_ublock_result, ustr_bptr(buffer), len);
            }

            if((opcode == LWK3_FORMULA) && (lf_version == 1))
                status = lotus123_decode_formula(&quick_ublock_formula, p_data + 8, &slr);

            break;
            }

        case LWK3_LABEL:
            {
            PC_SBSTR sbstr = PtrAddBytes(PC_SBSTR, p_data, 1);

            data_type = LOTUS123_DATA_TEXT;

            /* deal with label alignment byte */
            switch(p_data[0])
            {
            default: default_unhandled();
            case CH_APOSTROPHE:
                break;

            case CH_QUOTATION_MARK:
                format_type = LOTUS123_FORMAT_RIGHT;
                break;

            case CH_CIRCUMFLEX_ACCENT:
                format_type = LOTUS123_FORMAT_CENTRE;
                break;

            case CH_BACKWARDS_SLASH:
                /* repeating */
                break;
            }

            status = quick_ublock_sbstr_add_n(&quick_ublock_result, sbstr, strlen_without_NULLCH);

            break;
            }
        }

        /* need to CH_NULL-terminate to pass via insert_cell_contents_foreign */
        if(status_ok(status) && (0 != quick_ublock_bytes(&quick_ublock_result)))
            status = quick_ublock_nullch_add(&quick_ublock_result);

        if(status_ok(status) && (0 != quick_ublock_bytes(&quick_ublock_formula)))
            status = quick_ublock_nullch_add(&quick_ublock_formula);

        if(status_ok(status))
        {
            if(quick_ublock_bytes(&quick_ublock_result) || quick_ublock_bytes(&quick_ublock_formula))
            {
                LOAD_CELL_FOREIGN load_cell_foreign;
                OBJECT_ID object_id;

                zero_struct(load_cell_foreign);
                status_consume(object_data_from_slr(p_docu, &load_cell_foreign.object_data, &slr));
                load_cell_foreign.original_slr = slr;

                switch(data_type)
                {
                default: default_unhandled();
#if CHECKING
                case LOTUS123_DATA_TEXT:
#endif
                    object_id = OBJECT_ID_TEXT;
                    load_cell_foreign.data_type = OWNFORM_DATA_TYPE_TEXT;
                    load_cell_foreign.ustr_inline_contents = quick_ublock_ustr_inline(&quick_ublock_result);
                    break;

                case LOTUS123_DATA_SS:
                    object_id = OBJECT_ID_SS;
                    load_cell_foreign.data_type = OWNFORM_DATA_TYPE_FORMULA;

                    /* this determines whether cells are recalced after load - supply some
                     * contents and the evaluator won't bother to recalc the cell
                     */
                    load_cell_foreign.ustr_formula = quick_ublock_bytes(&quick_ublock_formula)
                                                   ? quick_ublock_ustr(&quick_ublock_formula)
                                                   : NULL;

                    if(NULL == load_cell_foreign.ustr_formula)
                        load_cell_foreign.ustr_inline_contents =
                            quick_ublock_bytes(&quick_ublock_result)
                                ? quick_ublock_ustr_inline(&quick_ublock_result)
                                : NULL;

                    if(0 != quick_ublock_bytes(&quick_ublock_format))
                    {
                        PC_USTR ustr_format = quick_ublock_ustr(&quick_ublock_format);

                        if(CH_NULL != PtrGetByte(ustr_format))
                        {
#if 0 /* All format strings generated above are currently useless for WK3 */
                            if(is_date_format)
                            {
                                load_cell_foreign.ustr_format_dt = quick_ublock_ustr(&quick_ublock_format);
                            }
                            else
                            {
                                load_cell_foreign.ustr_format_nu = ustr_format;
                            }
#endif
                        }
                    }

                    break;
                }

                status = insert_cell_contents_foreign(p_docu, object_id, T5_MSG_LOAD_CELL_FOREIGN, &load_cell_foreign);
            }
        }

        p_process_status->data.percent.current = ((S32) 100 * (S32) opcode_offset) / n_file_bytes;
        process_status_reflect(p_process_status);

        quick_ublock_dispose(&quick_ublock_result);
        quick_ublock_dispose(&quick_ublock_formula);
        quick_ublock_dispose(&quick_ublock_format);

        status_break(status);
    }

    assert(opcode_offset <= n_file_bytes);

    quick_ublock_dispose(&quick_ublock_result);
    quick_ublock_dispose(&quick_ublock_formula);
    quick_ublock_dispose(&quick_ublock_format);

    return(status);
}

_Check_return_
static STATUS
lotus123_auto_width_table(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     COL n_cols,
    _InVal_     ROW n_rows)
{
    STATUS status = STATUS_OK;
    COL col = 0;
    DOCU_AREA docu_area;
    STYLE style;

    docu_area_init(&docu_area);
    docu_area_set_whole_col(&docu_area);

    style_init(&style);
    style_bit_set(&style, STYLE_SW_CS_WIDTH);

    while(col < n_cols)
    {
        COL_AUTO_WIDTH col_auto_width;
        col_auto_width.col = col;
        col_auto_width.row_s = 0;
        col_auto_width.row_e = n_rows;
        col_auto_width.allow_special = TRUE;
        status_assert(object_call_id(p_docu->object_id_flow, p_docu, T5_MSG_COL_AUTO_WIDTH, &col_auto_width));
        style.col_style.width = col_auto_width.width;

        if(style.col_style.width != 0)
        {
            STYLE_DOCU_AREA_ADD_PARM style_docu_area_add_parm;
            STYLE_DOCU_AREA_ADD_STYLE(&style_docu_area_add_parm, &style);
            docu_area.tl.slr.col = col;
            docu_area.br.slr.col = col + 1;
            status_break(status = style_docu_area_add(p_docu, &docu_area, &style_docu_area_add_parm));
        }

        ++col;
    }

    return(status);
}

/******************************************************************************
*
* table of Lotus operators and equivalents that we can handle for load
*
* this table is in opcode (fno) order
*
******************************************************************************/

typedef struct OPR_EQUIVALENT
{
    PC_A7STR p_text_t5;
    U8 ftype;
    U8 fno;
    U8 n_args;
#define OPR_ARGS_VAR '\xFF'
}
OPR_EQUIVALENT; typedef const OPR_EQUIVALENT * PC_OPR_EQUIVALENT; /* don't you dare remove that const! */

#define opr_entry(fno, ftype, n_args, p_text_t5) \
    { (p_text_t5), (ftype), (fno), (n_args) }

static const OPR_EQUIVALENT
opr_equivalent[] =
{
    opr_entry(LF_CONST,      LO_CONST,              0,  ""),
    opr_entry(LF_SLR,        LO_CONST,              0,  ""),
    opr_entry(LF_RANGE,      LO_CONST,              0,  ""),
    opr_entry(LF_END,        LO_END,                0,  ""),
    opr_entry(LF_BRACKETS,   LO_BRACKETS,           0,  ""),
    opr_entry(LF_INTEGER,    LO_CONST,              0,  ""),
    opr_entry(LF_STRING,     LO_CONST,              0,  ""),

    opr_entry(LF_UMINUS,     LO_UNARY,              1,  "-"),
    opr_entry(LF_PLUS,       LO_BINARY,             2,  "+"),
    opr_entry(LF_MINUS,      LO_BINARY,             2,  "-"),
    opr_entry(LF_TIMES,      LO_BINARY,             2,  "*"),
    opr_entry(LF_DIVIDE,     LO_BINARY,             2,  "/"),
    opr_entry(LF_POWER,      LO_BINARY,             2,  "^"),
    opr_entry(LF_EQUALS,     LO_BINARY,             2,  "="),
    opr_entry(LF_NOTEQUAL,   LO_BINARY,             2,  "<>"),
    opr_entry(LF_LTEQUAL,    LO_BINARY,             2,  "<="),
    opr_entry(LF_GTEQUAL,    LO_BINARY,             2,  ">="),
    opr_entry(LF_LT,         LO_BINARY,             2,  "<"),
    opr_entry(LF_GT,         LO_BINARY,             2,  ">"),
    opr_entry(LF_AND,        LO_BINARY,             2,  "&"),
    opr_entry(LF_OR,         LO_BINARY,             2,  "|"),
    opr_entry(LF_NOT,        LO_UNARY,              1,  "!"),
    opr_entry(LF_UPLUS,      LO_UNARY,              1,  "+"),

    opr_entry(LF_NA,         LO_FUNC,               0,  "NA"),
    opr_entry(LF_ERR,        LO_FUNC,               0,  ".ERR"),
    opr_entry(LF_ABS,        LO_FUNC,               1,  "ABS"),
    opr_entry(LF_INT,        LO_FUNC,               1,  "INT"),
    opr_entry(LF_SQRT,       LO_FUNC,               1,  "SQR"), /* 123:SQRT */
    opr_entry(LF_LOG,        LO_FUNC,               1,  "LOG"), /* 123 won't support second optional parameter */
    opr_entry(LF_LN,         LO_FUNC,               1,  "LN"),
    opr_entry(LF_PI,         LO_FUNC,               0,  "PI"),
    opr_entry(LF_SIN,        LO_FUNC,               1,  "SIN"),
    opr_entry(LF_COS,        LO_FUNC,               1,  "COS"),
    opr_entry(LF_TAN,        LO_FUNC,               1,  "TAN"),
    opr_entry(LF_ATAN2,      LO_FUNC,               1,  "ATAN_2"), /* 123:ATAN2 */
    opr_entry(LF_ATAN,       LO_FUNC,               1,  "ATAN"),
    opr_entry(LF_ASIN,       LO_FUNC,               1,  "ASIN"),
    opr_entry(LF_ACOS,       LO_FUNC,               1,  "ACOS"),
    opr_entry(LF_EXP,        LO_FUNC,               1,  "EXP"),
    opr_entry(LF_MOD,        LO_FUNC,               2,  "MOD"),
    opr_entry(LF_CHOOSE,     LO_FUNC,    OPR_ARGS_VAR,  "CHOOSE"),
    opr_entry(LF_ISNA,       LO_FUNC,               1,  "ISNA"),
    opr_entry(LF_ISERR,      LO_FUNC,               1,  "ISERR"),
    opr_entry(LF_FALSE,      LO_FUNC,               0,  "FALSE"),
    opr_entry(LF_TRUE,       LO_FUNC,               1,  "TRUE"),
    opr_entry(LF_RAND,       LO_FUNC,               0,  "RAND"),
    opr_entry(LF_DATE,       LO_FUNC,               3,  "DATE"),
    opr_entry(LF_TODAY,      LO_FUNC,               0,  "NOW"),
    opr_entry(LF_PMT,        LO_FUNC,               3,  "PMT"),
    opr_entry(LF_PV,         LO_FUNC,               3,  "PV"),
    opr_entry(LF_FV,         LO_FUNC,               3,  "FV"),
    opr_entry(LF_IF,         LO_FUNC,               3,  "IF"),
    opr_entry(LF_DAY,        LO_FUNC,               1,  "DAY"),
    opr_entry(LF_MONTH,      LO_FUNC,               1,  "MONTH"),
    opr_entry(LF_YEAR,       LO_FUNC,               1,  "YEAR"),
    opr_entry(LF_ROUND,      LO_FUNC,               2,  "ROUND"),
    opr_entry(LF_TIME,       LO_FUNC,               3,  "TIME"),
    opr_entry(LF_HOUR,       LO_FUNC,               1,  "HOUR"),
    opr_entry(LF_MINUTE,     LO_FUNC,               1,  "MINUTE"),
    opr_entry(LF_SECOND,     LO_FUNC,               1,  "SECOND"),
    opr_entry(LF_ISN,        LO_FUNC,               1,  "ISNONTEXT"), /* 123:ISNUMBER */
    opr_entry(LF_ISS,        LO_FUNC,               1,  "ISTEXT"), /* 123:ISSTRING */
    opr_entry(LF_LENGTH,     LO_FUNC,               1,  "LENGTH"),
    opr_entry(LF_VALUE,      LO_FUNC,               1,  "VALUE"),
    opr_entry(LF_FIXED,      LO_FUNC,               2,  "STRING"),
    opr_entry(LF_MID,        LO_FUNC,               3,  "MID"),
    opr_entry(LF_CHR,        LO_FUNC,               1,  "CHAR"),
    opr_entry(LF_ASCII,      LO_FUNC,               1,  "CODE"),
    opr_entry(LF_FIND,       LO_FUNC,               3,  "FIND"),
    opr_entry(LF_DATEVALUE,  LO_FUNC,               1,  "DATEVALUE"),
    opr_entry(LF_TIMEVALUE,  LO_FUNC,               1,  "TIMEVALUE"),
    opr_entry(LF_CELLPOINTER,LO_FUNC,               1,  ".CELLPOINTER"),
    opr_entry(LF_SUM,        LO_FUNC,    OPR_ARGS_VAR,  "SUM"),
    opr_entry(LF_AVG,        LO_FUNC,    OPR_ARGS_VAR,  "AVG"),
    opr_entry(LF_CNT,        LO_FUNC,    OPR_ARGS_VAR,  "COUNTA"), /* 123:COUNT (maps to COUNTA according to Excel) */
    opr_entry(LF_MIN,        LO_FUNC,    OPR_ARGS_VAR,  "MIN"),
    opr_entry(LF_MAX,        LO_FUNC,    OPR_ARGS_VAR,  "MAX"),
    opr_entry(LF_VLOOKUP,    LO_FUNC,               3,  "VLOOKUP"),
    opr_entry(LF_NPV,        LO_FUNC,               2,  "NPV"),
    opr_entry(LF_VAR,        LO_FUNC,    OPR_ARGS_VAR,  "VARP"), /* 123:VAR (maps to VARP according to Excel) */
    opr_entry(LF_STD,        LO_FUNC,    OPR_ARGS_VAR,  "STDP"), /* 123:STD (maps to STDEVP according to Excel) */
    opr_entry(LF_IRR,        LO_FUNC,               2,  "IRR"),
    opr_entry(LF_HLOOKUP,    LO_FUNC,               3,  "HLOOKUP"),
    opr_entry(LF_DSUM,       LO_FUNC,               3,  ".DSUM_123"), /* 123:DSUM */ /* Fireworkz is incompatible */
    opr_entry(LF_DAVG,       LO_FUNC,               3,  ".DAVG_123"), /* 123:DAVG */ /* Fireworkz is incompatible */
    opr_entry(LF_DCNT,       LO_FUNC,               3,  ".DCOUNT_123"), /* 123:DCOUNT (maps to DCOUNTA according to Excel) */ /* Fireworkz is incompatible */
    opr_entry(LF_DMIN,       LO_FUNC,               3,  ".DMIN_123"), /* 123:DMIN */ /* Fireworkz is incompatible */
    opr_entry(LF_DMAX,       LO_FUNC,               3,  ".DMAX_123"), /* 123:DMAX */ /* Fireworkz is incompatible */
    opr_entry(LF_DVAR,       LO_FUNC,               3,  ".DVAR_123"), /* 123:DVAR (maps to DVARP according to Excel) */ /* Fireworkz is incompatible */
    opr_entry(LF_DSTD,       LO_FUNC,               3,  ".DSTD_123"), /* 123:DSTD (maps to DSTDEVP according to Excel) */ /* Fireworkz is incompatible */
    opr_entry(LF_INDEX,      LO_FUNC,               3,  "INDEX"),
    opr_entry(LF_COLS,       LO_FUNC,               1,  "COLS"),
    opr_entry(LF_ROWS,       LO_FUNC,               1,  "ROWS"),
    opr_entry(LF_REPEAT,     LO_FUNC,               2,  "REPT"), /* 123:REPEAT */
    opr_entry(LF_UPPER,      LO_FUNC,               1,  "UPPER"),
    opr_entry(LF_LOWER,      LO_FUNC,               1,  "LOWER"),
    opr_entry(LF_LEFT,       LO_FUNC,               2,  "LEFT"),
    opr_entry(LF_RIGHT,      LO_FUNC,               2,  "RIGHT"),
    opr_entry(LF_REPLACE,    LO_FUNC,               4,  "REPLACE"),
    opr_entry(LF_PROPER,     LO_FUNC,               1,  "PROPER"),
    opr_entry(LF_CELL,       LO_FUNC,               2,  ".CELL"),
    opr_entry(LF_TRIM,       LO_FUNC,               1,  "TRIM"),
    opr_entry(LF_CLEAN,      LO_FUNC,               1,  "CLEAN"),
    opr_entry(LF_S,          LO_FUNC,               1,  "T"), /* 123:"S" */
    opr_entry(LF_V,          LO_FUNC,               1,  "N"), /* 123:"V" now "N"? */
    opr_entry(LF_STREQ,      LO_FUNC,               2,  "EXACT"),
    opr_entry(LF_CALL,       LO_FUNC,               1,  ".CALL"),
    opr_entry(LF_INDIRECT,   LO_FUNC,               1,  ".INDIRECT"), /* end of 1985 documentation */

    opr_entry(LF_RATE,       LO_FUNC,               3,  "RATE"),
    opr_entry(LF_TERM,       LO_FUNC,               3,  "TERM"),
    opr_entry(LF_CTERM,      LO_FUNC,               3,  "CTERM"),
    opr_entry(LF_SLN,        LO_FUNC,               3,  "SLN"),
    opr_entry(LF_SOY,        LO_FUNC,               4,  "SYD"),
    opr_entry(LF_DDB,        LO_FUNC,               4,  "DDB"),
};

#define n_opr_equivalent elemof32(opr_equivalent)

/*
Lotus decompiler statics
*/

static S32 cursym;

static PC_BYTE p_scan;

static PC_OPR_EQUIVALENT g_p_opr_equivalent;

static int arg_stack_n;

PROC_BSEARCH_PROTO(static, proc_opr_compare, U8, OPR_EQUIVALENT)
{
    BSEARCH_KEY_VAR_DECL(PC_U8, key);
    const U8 cursym_key = *key;
    BSEARCH_DATUM_VAR_DECL(PC_OPR_EQUIVALENT, datum); /* SKS 12jun96 fixed this - used to rely on fno being first element */
    const U8 fno_datum = datum->fno;

    if(cursym_key > fno_datum)
        return(1);

    if(cursym_key < fno_datum)
        return(-1);

    return(0);
}

/******************************************************************************
*
* symbol lookahead
*
******************************************************************************/

static S32
token_check(void)
{
    cursym = *p_scan;

    g_p_opr_equivalent = (PC_OPR_EQUIVALENT)
        bsearch(&cursym, opr_equivalent, n_opr_equivalent, sizeof(opr_equivalent[0]), proc_opr_compare);

    return(cursym);
}

/******************************************************************************
*
* RPN scanner
*
******************************************************************************/

static S32
token_scan(void)
{
    if(cursym != -1)
    {
        /* work out how to skip symbol */
        ++p_scan;

        switch(g_p_opr_equivalent->ftype)
        {
        case LO_CONST:
            switch(cursym)
            {
            case LF_CONST:
                p_scan += 8;
                break;
            case LF_SLR:
                p_scan += 4;
                break;
            case LF_RANGE:
                p_scan += 8;
                break;
            case LF_INTEGER:
                p_scan += 2;
                break;
            case LF_STRING:
                while(*p_scan++)
                { /*EMPTY*/ }
                break;
            default:
                break;
            }
            break;

        case LO_FUNC:
            /* skip argument count */
            if(OPR_ARGS_VAR == g_p_opr_equivalent->n_args)
                ++p_scan;
            break;

        default:
            break;
        }
    }

    return(token_check());
}

/******************************************************************************
*
* free all elements on stack
*
******************************************************************************/

static void
stack_free(void)
{
    while(0 != arg_stack_n)
        ustr_clr(&arg_stack[--arg_stack_n]);
}

/******************************************************************************
*
* convert SLR
*
******************************************************************************/

static void
slr_convert(
    /*_Out_*/   P_U8 p_u8_out,
    _InoutRef_  P_U32 p_len,
    /*_In_*/    PC_BYTE p_u8_in,
    _InRef_     PC_SLR p_slr,
    _InoutRef_opt_ P_SLR p_slr_first)
{
    PC_BYTE p_u8 = p_u8_in;
    U32 len = 0;
    COL col;
    ROW row;
    BOOL col_dollar = 0;
    BOOL row_dollar = 0;
    UCHARZ buffer[50];

    col = (COL) lotus123_read_U16(p_u8);
    p_u8 += 2;
    col_dollar = ((col & 0x8000) == 0);
    col &= 0x00FF;
    if(!col_dollar)
        col = p_slr->col + ((col << (COL_BITS - 8)) >> (COL_BITS - 8)); /* sign-extend for relative cell reference */

    row = (ROW) lotus123_read_U16(p_u8);
    p_u8 += 2;
    row_dollar = ((row & 0x8000) == 0);
    row &= 0x1FFF;
    if(!row_dollar)
        row = p_slr->row + ((row << (ROW_BITS - 13)) >> (ROW_BITS - 13)); /* sign-extend for relative cell reference */
    ++row;

    if(col_dollar)
        buffer[len++] = CH_DOLLAR_SIGN;

    len += xtos_ustr_buf(ustr_AddBytes_wr(buffer, len), elemof32(buffer) - len, col, FALSE);

    if(row_dollar)
        buffer[len++] = CH_DOLLAR_SIGN;

    len += xsnprintf(buffer + len, elemof32(buffer) - len, ROW_FMT, row);

    /* if on second ref., check for highest and swap */
    if((NULL != p_slr_first) && ((col < p_slr_first->col) || (row < p_slr_first->row)))
    {
        memmove32(p_u8_out + len, p_u8_out, *p_len);
        memcpy32(p_u8_out, buffer, len);
    }
    else
    {
        /* save actual addresses for comparison */
        if(NULL != p_slr_first)
        {
            p_slr_first->col = col;
            p_slr_first->row = row;
        }

        /* add (second) string */
        memcpy32(p_u8_out + *p_len, buffer, len);
    }

    *p_len += len;
}

/******************************************************************************
*
* convert floating point number to string
*
******************************************************************************/

_Check_return_
static U32 /* length out */
text_from_f64(
    _Out_writes_z_(elemof_buffer) P_USTR buffer,
    _InVal_     U32 elemof_buffer,
    _InRef_     PC_F64 p_f64)
{
    U32 len = ustr_xsnprintf(buffer, elemof_buffer, USTR_TEXT("%.15g"), *p_f64);
    PC_USTR ustr_exp;

    /* search for exponent and remove leading zeros because
     * they confuse the Z88; remove the + for good measure
    */
    if(NULL != (ustr_exp = ustrchr(buffer, 'e')))
    {
        U8 sign;
        P_USTR ustr_exps;

        ustr_IncByte(ustr_exp);
        sign = PtrGetByte(ustr_exp);
        ustr_exps = de_const_cast(P_USTR, ustr_exp);

        if(CH_PLUS_SIGN == sign)
        {
            ustr_IncByte(ustr_exp);
        }
        else if(CH_MINUS_SIGN__BASIC == sign)
        {
            ustr_IncByte(ustr_exp);
            ustr_IncByte_wr(ustr_exps);
        }

        while(CH_DIGIT_ZERO == PtrGetByte(ustr_exp))
            ustr_IncByte(ustr_exp);

        memcpy32(ustr_exps, ustr_exp, ustrlen32(ustr_exp));
        len -= PtrDiffBytesU32(ustr_exp, ustr_exps);
        PtrPutByteOff(buffer, len, CH_NULL);
    }

    return(len);
}

/******************************************************************************
*
* convert constant to a string
*
******************************************************************************/

_Check_return_
_Ret_maybenull_
static P_USTR
const_convert(
    _InVal_     S32 sym,
    /*_In_*/    PC_BYTE p_u8_const,
    _InRef_     PC_SLR p_slr,
    _OutRef_    P_STATUS p_status)
{
    P_USTR ustr_res;
    UCHARZ buffer[200];
    U32 len = 0;

    switch(sym)
    {
    case LF_CONST:
        {
        F64 f64 = lotus123_read_F64(p_u8_const);
        len = text_from_f64(ustr_bptr(buffer), elemof32(buffer), &f64);
        break;
        }

    case LF_SLR:
        slr_convert(buffer, &len, p_u8_const, p_slr, NULL);
        break;

    case LF_RANGE:
        {
        SLR slr_first = {0, 0};
        slr_convert(buffer, &len, p_u8_const, p_slr, &slr_first);
        slr_convert(buffer, &len, p_u8_const + 4, p_slr, &slr_first);
        break;
        }

    case LF_INTEGER:
        {
        S16 s16 = lotus123_read_S16(p_u8_const);
        len = ustr_xsnprintf(ustr_bptr(buffer), elemof32(buffer), USTR_TEXT(S16_FMT), s16);
        break;
        }

    case LF_STRING:
        {
        PC_U8Z p_u8_in = (PC_U8Z) p_u8_const;
        U8 ch;
        buffer[len++] = CH_QUOTATION_MARK;
        while((ch = *p_u8_in++) != CH_NULL)
            buffer[len++] = ch;
        buffer[len++]= CH_QUOTATION_MARK;
        break;
        }

    default:
        break;
    }

    buffer[len] = CH_NULL;
    assert(len <= elemof32(buffer));

    *p_status = ustr_set_n(&ustr_res, ustr_bptr(buffer), len);

    return(ustr_res);
}

_Check_return_
_Ret_z_ /* never NULL */
static inline P_USTR
stack_element_peek(
    _InVal_     U32 element_idx)
{
    P_USTR ret;
    PTR_ASSERT(arg_stack);
    assert(arg_stack_n >= (S32) (1 + element_idx));
    ret = arg_stack[arg_stack_n - (1 + element_idx)];
    PTR_ASSERT(ret);
    return(ret);
}

_Check_return_
_Ret_z_ /* never NULL */
static inline P_USTR
stack_element_pop(void)
{
    P_USTR ret;
    PTR_ASSERT(arg_stack);
    assert(arg_stack_n >= 1);
    ret = arg_stack[--arg_stack_n];
    PTR_ASSERT(ret);
    return(ret);
}

_Check_return_
static inline STATUS
stack_element_push(
    _In_z_      P_USTR p_new_element)
{
    PTR_ASSERT(p_new_element);
    assert(arg_stack_n < MAXSTACK);
    if(arg_stack_n >= MAXSTACK)
        return(LOTUS123_ERR_EXP);
    arg_stack[arg_stack_n++] = p_new_element;
    return(STATUS_OK);
}

/******************************************************************************
*
* decompile a Lotus formula
*
******************************************************************************/

_Check_return_
static STATUS
lotus123_decode_formula(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    /*in*/      PC_BYTE p_formula_size,
    _InRef_     PC_SLR p_slr)
{
    STATUS status = STATUS_OK;
    PC_BYTE p_formula = p_formula_size + 2;
    S32 no_brackets = 0;
    P_USTR p_ele_new;

    /* set scanner index */
    arg_stack_n = 0;
    p_scan = p_formula;
    token_check();

    for(;;)
    {
        if(NULL == g_p_opr_equivalent)
            break; /* SKS 16aug95 get out of here with unrecognized stuff - punter sent in interesting files */

        switch(g_p_opr_equivalent->ftype)
        {
        case LO_CONST:
            if(NULL == (p_ele_new = const_convert(cursym, p_scan + 1, p_slr, &status)))
                break;

            status = stack_element_push(p_ele_new);
            break;

        case LO_END:
            break;

        case LO_BRACKETS:
            if(!no_brackets)
            {
                P_USTR p_ele_stack;
                P_U8 p_u8;
                U32 len_stack, len;

                if(0 == arg_stack_n)
                    status_break(status = create_error(LOTUS123_ERR_EXP));

                p_ele_stack = stack_element_pop();
                len_stack = ustrlen32(p_ele_stack);

                len = len_stack + 2 + 1; /* add two for brackets and one for CH_NULL */

                status_break(status = ustr_set_n(&p_ele_new, NULL, len));

                p_u8 = (P_U8) p_ele_new;

                *p_u8++ = CH_LEFT_PARENTHESIS;

                memcpy32(p_u8, p_ele_stack, len_stack);
                p_u8 += len_stack;

                *p_u8++ = CH_RIGHT_PARENTHESIS;

                *p_u8++ = CH_NULL;

                ustr_clr(&p_ele_stack);

                status = stack_element_push(p_ele_new);
            }

            no_brackets = 0;
            break;

        case LO_UNARY:
            {
            P_USTR p_ele_stack;
            P_U8 p_u8;
            U32 len_name, len_stack, alloc_len;

            if(0 == arg_stack_n)
                status_break(status = create_error(LOTUS123_ERR_EXP));

            p_ele_stack = stack_element_pop();
            len_stack = ustrlen32(p_ele_stack);

            len_name = strlen32(g_p_opr_equivalent->p_text_t5);

            /* extra for new operator and one for CH_NULL */
            alloc_len = len_stack + len_name + 1;

            status_break(status = ustr_set_n(&p_ele_new, NULL, alloc_len));

            p_u8 = (P_U8) p_ele_new;

            memcpy32(p_u8, g_p_opr_equivalent->p_text_t5, len_name);
            p_u8 += len_name;

            memcpy32(p_u8, p_ele_stack, len_stack);
            p_u8 += len_stack;

            *p_u8++ = CH_NULL;

            ustr_clr(&p_ele_stack);

            status = stack_element_push(p_ele_new);
            break;
            }

        case LO_BINARY:
            {
            P_USTR p_ele_1, p_ele_2;
            P_U8 p_u8;
            U32 len_add, ele_1_len, ele_2_len, alloc_len;

            if(arg_stack_n < 2)
                status_break(status = create_error(LOTUS123_ERR_EXP));

            p_ele_2 = stack_element_pop();
            ele_2_len = ustrlen32(p_ele_2);

            p_ele_1 = stack_element_pop();
            ele_1_len = ustrlen32(p_ele_1);

            len_add = strlen32(g_p_opr_equivalent->p_text_t5);

            /* two arguments, operator and CH_NULL */
            alloc_len = ele_1_len + ele_2_len + len_add + 1;

            status_break(status = ustr_set_n(&p_ele_new, NULL, alloc_len));

            p_u8 = (P_U8) p_ele_new;

            memcpy32(p_u8, p_ele_1, ele_1_len);
            p_u8 += ele_1_len;

            memcpy32(p_u8, g_p_opr_equivalent->p_text_t5, len_add);
            p_u8 += len_add;

            memcpy32(p_u8, p_ele_2, ele_2_len);
            p_u8 += ele_2_len;

            *p_u8++ = CH_NULL;

            ustr_clr(&p_ele_1);
            ustr_clr(&p_ele_2);

            status = stack_element_push(p_ele_new);
            break;
            }

        case LO_FUNC:
            {
            int narg;
            U32 len_name;
            U32 len_tot;
            P_U8 p_u8;

            /* work out number of arguments */
            if((narg = g_p_opr_equivalent->n_args) == OPR_ARGS_VAR)
                narg = (int) *(p_scan + 1);

            if(arg_stack_n < narg)
                status_break(status = create_error(LOTUS123_ERR_EXP));

            len_tot = 1; /* always CH_NULL */

            /* add length of name */
            len_name = strlen32(g_p_opr_equivalent->p_text_t5);
            len_tot += len_name;

            if(narg)
            {
                U32 arg_idx = narg;

                /* add up the length of all the arguments */
                do  {
                    P_USTR ustr_temp = stack_element_peek(--arg_idx);
                    len_tot += ustrlen32(ustr_temp);
                }
                while(arg_idx);

                /* add in space for commas and function brackets */
                len_tot += narg - 1;
                if(!no_brackets)
                    len_tot += 2;
            }

            status_break(status = ustr_set_n(&p_ele_new, NULL, len_tot));

            p_u8 = (P_U8) p_ele_new;

            memcpy32(p_u8, g_p_opr_equivalent->p_text_t5, len_name);
            p_u8 += len_name;

            if(narg)
            {
                U32 arg_idx = narg;

                if(!no_brackets)
                    *p_u8++ = CH_LEFT_PARENTHESIS;

                do  {
                    P_USTR ustr_temp = stack_element_peek(--arg_idx);
                    U32 len_temp = ustrlen32(ustr_temp);
                    memcpy32(p_u8, ustr_temp, len_temp);
                    p_u8 += len_temp;
                    if(arg_idx)
                        *p_u8++ = CH_COMMA;
                    ustr_clr(&ustr_temp);
                }
                while(arg_idx);

                if(!no_brackets)
                    *p_u8++ = CH_RIGHT_PARENTHESIS;
            }

            *p_u8++ = CH_NULL;

            arg_stack_n -= narg;

            status = stack_element_push(p_ele_new);

            no_brackets = 0;
            break;
            }
        }

        if(cursym == LF_END)
            break;

        status_break(status);

        token_scan();
    }

    if(status_ok(status))
        if(arg_stack_n != 1)
            status = create_error(LOTUS123_ERR_EXP);

    if(status_ok(status))
        status = quick_ublock_ustr_add(p_quick_ublock, stack_element_peek(0));

    stack_free();
    return(status);
}

/******************************************************************************
*
* locate a record of a given type in the Lotus file
*
* --in--
* type contains type of record to locate
* flag indicates type of search:
*   specific type
*   next row after a given row
*
******************************************************************************/

/*ncr*/
static S32 /* offset of found record */
lotus123_find_record(
    _InRef_     PC_ARRAY_HANDLE p_h_data,
    _InoutRef_opt_ P_SLR p_slr_max,
    _InVal_     U16 record_type,
    _InVal_     S32 search_type,
    _InoutRef_opt_ P_SLR p_slr,
    _InVal_     S32 start_offset)
{
    S32 res = -1;
    const U32 n_file_bytes = array_elements32(p_h_data);
    PC_BYTE p_file_start = array_rangec(p_h_data, BYTE, 0, n_file_bytes);
    U32 opcode_offset = start_offset;
    U16 opcode = L_NONE;

    /* search for required opcode */
    while((opcode != L_EOF) && (res < 0) && (opcode_offset <= n_file_bytes))
    {
        PC_BYTE p_opcode = p_file_start + opcode_offset;
        PC_BYTE p_data = p_opcode;
        U16 length;

        /* read opcode */
        opcode = lotus123_read_U16(p_data);
        p_data += 2;

        length = lotus123_read_U16(p_data);
        p_data += 2;

        /* map opcode */
        if(3 == lf_version)
            opcode = v3_map_opcode(opcode);

        switch(search_type)
        {
        case SEARCH_TYPE_MATCH:
            if(opcode == record_type)
                res = (S32) (p_data - p_file_start); /* return offset of contents */
            break;

        case SEARCH_NEXT_ROW:
            switch(opcode)
            {
            case L_INTEGER:
            case L_NUMBER:
            case L_LABEL:
            case L_FORMULA:
                {
                COL col;
                ROW row;

                if(3 == lf_version)
                {
                    col = (COL) lotus123_read_U16(p_data);
                    p_data += 2;

                    if(L_LABEL != opcode)
                        p_data += 1;

                    row = (ROW) lotus123_read_U16(p_data);
                    p_data += 2;
                }
                else /* 1 == lf_version */
                {
                    /* skip format byte */
                    p_data += 1;

                    col = (COL) lotus123_read_U16(p_data);
                    p_data += 2;

                    row = (ROW) lotus123_read_U16(p_data);
                    p_data += 2;
                }

                /* p_data points to cell contents */

                if((NULL != p_slr) && (col == p_slr->col) && (row > p_slr->row))
                {
                    p_slr->row = row;
                    res = (S32) opcode_offset; /* return offset of header */
                }

                /* set maximum SLR found */
                if(NULL != p_slr_max)
                {
                    p_slr_max->col = MAX(p_slr_max->col, col);
                    p_slr_max->row = MAX(p_slr_max->row, row);
                }

                break;
                }

            default:
                if(opcode == record_type)
                    res = (S32) opcode_offset; /* return offset of header */
                break;
            }
            break;

        default: default_unhandled(); break;
        }

        opcode_offset += length + 4;
    }

    assert(opcode_offset <= n_file_bytes);

    return(res);
}

/******************************************************************************
*
* read LOTUS file limits
*
******************************************************************************/

_Check_return_
static STATUS
lotus123_read_file_limits(
    _OutRef_    P_S32 p_s_col,
    _OutRef_    P_S32 p_e_col,
    _OutRef_    P_S32 p_s_row,
    _OutRef_    P_S32 p_e_row,
    _InRef_     PC_ARRAY_HANDLE p_h_data)
{
    const U32 n_file_bytes = array_elements32(p_h_data);
    PC_BYTE p_file_start = array_rangec(p_h_data, BYTE, 0, n_file_bytes);
    S32 file_pos;
    U8 search_for_limits = 0;

    *p_s_col = *p_e_col = 0;
    *p_s_row = *p_e_row = 0;

    /* check start of file */
    if(n_file_bytes <= sizeof32(lf_BOF_opcode) + sizeof32(lf_BOF_WK1))
        return(create_error(LOTUS123_ERR_BADFILE));

    if(0 != memcmp32(p_file_start, lf_BOF_opcode, sizeof32(lf_BOF_opcode)))
        return(create_error(LOTUS123_ERR_BADFILE));

    if(0 == memcmp32(p_file_start + 2, lf_BOF_WK1, sizeof32(lf_BOF_WK1)))
        lf_version = 1;
    else if(0 == memcmp32(p_file_start + 2, lf_BOF_S, sizeof32(lf_BOF_S))) /*Symphony*/
        lf_version = 1;
    else if(0 == memcmp32(p_file_start + 2, lf_BOF_S2, sizeof32(lf_BOF_S2))) /*Symphony*/
        lf_version = 1;
    else if(n_file_bytes <= sizeof32(lf_BOF_opcode) + sizeof32(lf_BOF_WK3)) /* larger ones */
        return(create_error(LOTUS123_ERR_BADFILE));
    else if(0 == memcmp32(p_file_start + 2, lf_BOF_WK3, sizeof32(lf_BOF_WK3)))
        lf_version = 3;
    else if(0 == memcmp32(p_file_start + 2, lf_BOF_123_4, sizeof32(lf_BOF_123_4))) /*WK4*/
        lf_version = 3;
    else if(0 == memcmp32(p_file_start + 2, lf_BOF_123_6, sizeof32(lf_BOF_123_6)))
        lf_version = 3;
    else if(0 == memcmp32(p_file_start + 2, lf_BOF_123_7, sizeof32(lf_BOF_123_7)))
        lf_version = 3;
    else if(0 == memcmp32(p_file_start + 2, lf_BOF_123_SS98, sizeof32(lf_BOF_123_SS98)))
        lf_version = 3;
    else
        return(create_error(LOTUS123_ERR_BADFILE));

#if 1
    { /* dump all records */
    static const PCTSTR opcodes_1[] = {
        TEXT("BOF"), /*0x00*/       /* Beginning of file */
        TEXT("EOF"),                /* End of file */
        TEXT(".CALCMODE"),          /* Calculation method */
        TEXT(".CALCORDER"),         /* Calculation order */
        TEXT(".SPLIT"),             /* Split window type */ /* 1-2-3 only */
        TEXT(".SYNC"),              /* Split window sync */ /* 1-2-3 only */
        TEXT("RANGE"),              /* Range of cells written to worksheet file */
        TEXT(".WINDOW1"),           /* Window 1 record */ /* 1-2-3 only */
        TEXT(".COLW1"),             /* Column width */
        TEXT(".WINTWO"),            /* Window 2 record */ /* 1-2-3 only */
        TEXT(".COLW2"),             /* Column width, Window 2 */ /* 1-2-3 only */
        TEXT(".NAME"),              /* Name of range */ /* 1-2-3 only */
        TEXT("BLANK"),              /* Blank cell */
        TEXT("INTEGER"),            /* Integer number cell */
        TEXT("NUMBER"),             /* Floating point number */
        TEXT("LABEL"), /* 0x0F*/    /* Label cell */

        TEXT("FORMULA"), /*0x10*/   /* Formula cell */
        TEXT(".11"),
        TEXT(".12"),
        TEXT(".13"),
        TEXT(".14"),
        TEXT(".15"),
        TEXT(".16"),
        TEXT(".17"),
        TEXT(".TABLE"),             /* Table range */
        TEXT(".QRANGE"),            /* Query range */ /* 1-2-3 only */
        TEXT(".PRANGE"),            /* Print range */ /* 1-2-3 only */
        TEXT(".SRANGE"),            /* Sort range */ /* 1-2-3 only */
        TEXT(".FRANGE"),            /* Fill range */
        TEXT(".KRANGE1"),           /* Primary sort key range */
        TEXT(".1E"),
        TEXT(".1F"), /* 0x1F*/

        TEXT(".HRANGE"), /*0x20*/   /* Distribution range */
        TEXT(".21"),
        TEXT(".22"),
        TEXT(".KRANGE2"),           /* Secondary sort key range */ /* 1-2-3 only */
        TEXT(".PROTEC"),            /* Global protection */
        TEXT(".FOOTER"),            /* Print footer */ /* 1-2-3 only */
        TEXT(".HEADER"),            /* Print header */ /* 1-2-3 only */
        TEXT(".SETUP"),             /* Print setup */ /* 1-2-3 only */
        TEXT(".MARGINS"),           /* Print margins code */ /* 1-2-3 only */
        TEXT(".LABELFMT"),          /* Label alignment */
        TEXT(".TITLES"),            /* Print borders */ /* 1-2-3 only */
        TEXT(".2B"),
        TEXT(".2C"),
        TEXT(".GRAPH"),             /* Current graph settings */ /* 1-2-3 only */
        TEXT(".NGRAPH"),            /* Named current graph settings */ /* 1-2-3 only */
        TEXT(".CALCCOUNT"), /*0x2F*//* Iteration count */

        TEXT(".UNFORMATTED"), /*0x30*//* Formatted/unformatted print */ /* 1-2-3 only */
        TEXT(".CURSORW12"),         /* Cursor location */ /* 1-2-3 only */
        TEXT(".WINDOW"),            /* Window record structure */ /* Symphony only */
        TEXT(".STRING")             /* Value of string formula */ /* Symphony only */
        /* 0x34..0x36 */
        /* 0x37 PASSWORD */         /* Symphony only. */
        /* 0x38 LOCKED */           /* Lock Flag */ /* Symphony only */
        /* 0x3C QUERY */            /* Query settings */ /* Symphony only */
        /* 0x3D QUERYNAME */        /* Current Query Name */ /* Symphony only */
        /* 0x3E PRINT */            /* Print record */ /* Symphony only */
        /* 0x3F PRINTNAME */        /* Current Print Record Name */ /* Symphony only */

        /* 0x40 GRAPH2 */           /* Graph record */ /* Symphony only */
        /* 0x41 GRAPHNAME */        /* Current Graph Record Name */ /* Symphony only */
        /* 0x42 ZOOM */             /* Original coordinates expanded window */ /* Symphony only */
        /* 0x43 SYMSPLIT */         /* Number of split windows */ /* Symphony only */
        /* 0x44 NSROWS */           /* Number of screen rows */ /* Symphony only */
        /* 0x45 NSCOLS */           /* Number of screen columns */ /* Symphony only */
        /* 0x46 RULER */            /* Name ruler range */ /* Symphony only */
        /* 0x47 NNAME */            /* Named sheet range */ /* Symphony only */
        /* 0x48 ACOMM */            /* Autoload communications file */ /* Symphony only */
        /* 0x49 AMACRO */           /* Autoexecute macro address */ /* Symphony only */
        /* 0x4A PARSE */            /* Query parse information */ /* Symphony only */
    };

    static const PCTSTR opcodes_3[] = {
        TEXT("WK3_BOF"), /*0x00*/   /* Beginning of file */
        TEXT("WK3_EOF"),            /* End of file */
        TEXT(".02"),
        TEXT(".03"),
        TEXT(".04"),
        TEXT(".05"),
        TEXT(".06"),
        TEXT(".07"),
        TEXT(".08"),
        TEXT(".09"),
        TEXT(".0A"),
        TEXT(".0B"),
        TEXT(".0C"),
        TEXT(".0D"),
        TEXT(".0E"),
        TEXT(".0F"), /* 0x0F*/

        TEXT(".10"), /* 0x10*/
        TEXT(".11"),
        TEXT(".12"),
        TEXT(".13"),
        TEXT(".14"),
        TEXT(".15"),
        TEXT("WK3_LABEL"),
        TEXT("WK3_NUMBER"),
        TEXT("WK3_SMALLNUMBER"),
        TEXT("WK3_FORMULA"),
        TEXT(".1A"),
        TEXT(".WK3_STYLE"),
        TEXT(".1C"),
        TEXT(".1D"),
        TEXT(".1E"),
        TEXT(".1F"), /* 0x1F*/

        TEXT(".20"), /* 0x20*/
        TEXT(".21")
    };

    U32 opcode_offset = 0;
    U16 opcode = L_NONE;

    while(opcode_offset <= n_file_bytes)
    {
        PC_BYTE p_opcode = p_file_start + opcode_offset;
        PC_BYTE p_data = p_opcode;
        U16 length;

        /* read opcode */
        opcode = lotus123_read_U16(p_data);
        p_data += 2;

        length = lotus123_read_U16(p_data);
        p_data += 2;

        if(lf_version == 3)
            reportf(TEXT("%.4X: OP=0x%.2X len=%2d %s %s"), opcode_offset, opcode, length, (opcode < elemof32(opcodes_3)) ? opcodes_3[opcode] : TEXT(""), (v3_map_opcode(opcode) != L_BOF) ? TEXT("***") : TEXT(""));
        else
            reportf(TEXT("%.4X: OP=0x%.2X len=%2d %s"),    opcode_offset, opcode, length, (opcode < elemof32(opcodes_1)) ? opcodes_1[opcode] : TEXT(""));

        opcode_offset += length + 4;

        if(L_EOF == opcode)
            break;
    }

    assert(opcode_offset <= n_file_bytes);
    } /*block*/

#endif

    if((1 == lf_version) && status_ok(file_pos = lotus123_find_record(p_h_data, NULL, L_RANGE, SEARCH_TYPE_MATCH, NULL, 0)))
    {
        U16 u16;

        /* starting column */
        if((u16 = lotus123_read_U16(p_file_start + file_pos)) > LOTUS123_MAXCOL)
            search_for_limits = 1;
        else
            *p_s_col = (S32) u16;
        file_pos += 2;

        /* starting row */
        if((u16 = lotus123_read_U16(p_file_start + file_pos)) > LOTUS123_MAXROW)
            search_for_limits = 1;
        else
            *p_s_row = (S32) u16;
        file_pos += 2;

        /* ending column (inclusive) */
        if(((u16 = lotus123_read_U16(p_file_start + file_pos)) > LOTUS123_MAXCOL) || (u16 == 0))
            search_for_limits = 1;
        else
            *p_e_col = (S32) u16;
        file_pos += 2;

        /* ending row (inclusive) */
        if(((u16 = lotus123_read_U16(p_file_start + file_pos)) > LOTUS123_MAXROW) || (u16 == 0))
            search_for_limits = 1;
        else
            *p_e_row = (S32) u16;
        file_pos += 2;

        /* SKS 02feb96 bodge for wanky Psion A-Link converter which lies about the extents */
        if((*p_s_col == 0) && (*p_s_row == 0) && (*p_e_col == 255) && (*p_e_row == 255))
            search_for_limits = 1;
    }
    else
        /* 2.11.93 */
        search_for_limits = 1;

    if(search_for_limits)
    {
        SLR slr_max;

        slr_max.col = -1;
        slr_max.row = -1;

        if(lotus123_find_record(p_h_data, &slr_max, L_EOF, SEARCH_NEXT_ROW, NULL, 0) < 0)
            return(create_error(LOTUS123_ERR_BADFILE));

        /* watch out for FM3 formatting-only files */
        if((slr_max.col == -1) || (slr_max.row == -1))
            return(create_error(LOTUS123_ERR_BADFILE));

        *p_s_col = 0;
        *p_s_row = 0;
        *p_e_col = MIN(slr_max.col, LOTUS123_MAXCOL - 1);
        *p_e_row = MIN(slr_max.row, LOTUS123_MAXROW - 1);
    }

#if 0
    /* test switched off 5.11.93 */
    if(*p_s_col == *p_e_col || *p_s_row == *p_e_row)
        status = create_error(LOTUS123_ERR_BADFILE);
#endif

    /* make into exclusive */
    *p_e_col += 1;
    *p_e_row += 1;

    return(STATUS_OK);
}

/******************************************************************************
*
* check Lotus cell format for a date
*
******************************************************************************/

#define SECS_IN_24 ((S32) 60 * (S32) 60 * (S32) 24)

_Check_return_
static STATUS
lotus123_check_date(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _InRef_     PC_BYTE p_format,
    _InRef_     PC_F64 p_f64)
{
    /* days in the month */
    static const S32 lotus123_days[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

    STATUS status = STATUS_OK;
    U8 decplc = (U8) (*p_format & L_DECPLC);

    switch(*p_format & L_FMTTYPE)
    {
    default: default_unhandled();
#if CHECKING
    case L_FIXED:
    case L_SCIFI:
    case L_CURCY:
    case L_PERCT:
    case L_COMMA:
#endif
        break;

    case L_SPECL:
        {
        switch(decplc)
        {
        default:
            break;

        /* dates */
        case L_DDMMYY:
        case L_DDMM:
        case L_MMYY:
        case L_DATETIME:
        case L_DATETIMENS:
        case L_DATEINT1:
        case L_DATEINT2:
        case L_TIMEINT1:
        case L_TIMEINT2:
            {
            S32 dateno;
            S32 dayno = 0;
            S32 year, month, day;
            S32 lasta;
            BOOL leap;
            S32 hours = 0;
            S32 minutes = 0;
            S32 seconds;

            if((*p_f64 > (F64) S32_MAX) || (*p_f64 < 0.0))
                break;

            dateno = (S32) (*p_f64);

            /* fractional part denotes hh:mm:ss bit */
            seconds = (S32) (SECS_IN_24 * (*p_f64 - (F64) dateno));

            if(0 != seconds)
            {
                minutes = seconds / 60;
                seconds = seconds % 60;

                hours   = minutes / 60;
                minutes = minutes % 60;
            }

            year = 1900 -1;

            do  {
                ++year;
                leap = (year & 3) ? FALSE : TRUE; /* same poor test as old Excel to get dates right! */
                month = 0;
                do  {
                    lasta = lotus123_days[month];
                    if(leap && (month == 1))
                        ++lasta;
                    dayno += lasta;
                    ++month;
                }
                while((dayno < dateno) && (month < 12));
            }
            while(dayno < dateno);

            day = (dateno - (dayno - lasta));

            /* Lotus format determines which components we should convert */
            switch(decplc)
            {
            default:
                status = quick_ublock_printf(p_quick_ublock,
                                             USTR_TEXT("%.2" S32_FMT_POSTFIX "." "%.2" S32_FMT_POSTFIX "." "%.4" S32_FMT_POSTFIX),
                                             day, month, year);
                break;

            case L_DATETIME:
            case L_DATETIMENS:
                status = quick_ublock_printf(p_quick_ublock,
                                             USTR_TEXT("%.2" S32_FMT_POSTFIX "." "%.2" S32_FMT_POSTFIX "." "%.4" S32_FMT_POSTFIX
                                                       " "
                                                       "%.2" S32_FMT_POSTFIX ":" "%.2" S32_FMT_POSTFIX ":" "%.2" S32_FMT_POSTFIX),
                                             day, month, year,
                                             hours, minutes, seconds);
                break;


            case L_TIMEINT1:
            case L_TIMEINT2:
                status = quick_ublock_printf(p_quick_ublock,
                                             USTR_TEXT("%.2" S32_FMT_POSTFIX ":" "%.2" S32_FMT_POSTFIX ":" "%.2" S32_FMT_POSTFIX),
                                             hours, minutes, seconds);
                break;
            }

            if(status_ok(status))
                status = STATUS_DONE;

            break;
            }
        }

        break;
        }
    }

    return(status);
}

/******************************************************************************
*
* decode Lotus cell format
*
******************************************************************************/

_Check_return_
static STATUS
lotus123_decode_format_add_places(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock_format /*appended*/,
    _InVal_     U32 decplc)
{
    if(0 == decplc)
        return(STATUS_OK);

    return(quick_ublock_ustr_add_n(p_quick_ublock_format, USTR_TEXT(".000000000000000"), 1U/*dot*/ + MIN(15U, decplc)));
}

_Check_return_
static STATUS
lotus123_decode_format(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock_format /*appended*/,
    _InRef_     PC_BYTE p_format)
{
    STATUS status = STATUS_OK;
    U8 decplc = (U8) (*p_format & L_DECPLC);

    switch(*p_format & L_FMTTYPE)
    {
    default:
        break;

    case L_FIXED:
        if(status_ok(status = quick_ublock_ustr_add(p_quick_ublock_format, USTR_TEXT("0"))))
                     status = lotus123_decode_format_add_places(p_quick_ublock_format, decplc);
        break;

    case L_SCIFI:
        if( status_ok(status = quick_ublock_ustr_add(p_quick_ublock_format, USTR_TEXT("0"))) &&
            status_ok(status = lotus123_decode_format_add_places(p_quick_ublock_format, decplc)) )
                      status = quick_ublock_ustr_add(p_quick_ublock_format, USTR_TEXT("E+00"));
        break;

    case L_CURCY:
        if( status_ok(status = quick_ublock_sbstr_add(p_quick_ublock_format, SBSTR_TEXT("\xA3" "#,##0"))) &&
            status_ok(status = lotus123_decode_format_add_places(p_quick_ublock_format, decplc)) &&
            status_ok(status = quick_ublock_sbstr_add(p_quick_ublock_format, SBSTR_TEXT(" ;[Negative](" "\xA3" "#,##0"))) &&
            status_ok(status = lotus123_decode_format_add_places(p_quick_ublock_format, decplc)) )
                      status = quick_ublock_ustr_add(p_quick_ublock_format, USTR_TEXT(")"));
        break;

    case L_PERCT:
        if( status_ok(status = quick_ublock_ustr_add(p_quick_ublock_format, USTR_TEXT("0"))) &&
            status_ok(status = lotus123_decode_format_add_places(p_quick_ublock_format, decplc)) )
                      status = quick_ublock_ustr_add(p_quick_ublock_format, USTR_TEXT("%"));
        break;

    case L_COMMA:
        if(status_ok(status = quick_ublock_ustr_add(p_quick_ublock_format, USTR_TEXT("#,##0"))))
                     status = lotus123_decode_format_add_places(p_quick_ublock_format, decplc);
        break;

    case L_SPECL:
        {
        switch(decplc)
        {
        default:
            break;

        case L_GENFMT:
            break;

        case L_DDMMYY:
        case L_DATEINT1:
            status = quick_ublock_ustr_add(p_quick_ublock_format, USTR_TEXT("dd/mm/yyyy"));
            break;

        case L_DDMM:
        case L_DATEINT2:
            status = quick_ublock_ustr_add(p_quick_ublock_format, USTR_TEXT("dd/mm"));
            break;

        case L_MMYY:
            status = quick_ublock_ustr_add(p_quick_ublock_format, USTR_TEXT("Mmm yyyy"));
            break;

        case L_TEXT:
            break;

        case L_DATETIME:
            status = quick_ublock_ustr_add(p_quick_ublock_format, USTR_TEXT("dd/mm/yyyy hh:mm:ss"));
            break;

        case L_DATETIMENS:
            status = quick_ublock_ustr_add(p_quick_ublock_format, USTR_TEXT("dd/mm/yyyy hh:mm"));
            break;

        case L_TIMEINT1:
            status = quick_ublock_ustr_add(p_quick_ublock_format, USTR_TEXT("hh:mm:ss"));
            break;

        case L_TIMEINT2:
            status = quick_ublock_ustr_add(p_quick_ublock_format, USTR_TEXT("hh:mm"));
            break;
        }

        break;
        }
    }

    if(status_ok(status) && (0 != quick_ublock_bytes(p_quick_ublock_format)))
        status = quick_ublock_nullch_add(p_quick_ublock_format);

    return(status);
}


/******************************************************************************
*
* main routine to load a Lotus 1-2-3 file
*
******************************************************************************/

T5_MSG_PROTO(static, lotus123_msg_insert_foreign, _InoutRef_ P_MSG_INSERT_FOREIGN p_msg_insert_foreign)
{
    STATUS status = STATUS_OK;
    PROCESS_STATUS process_status;
    P_POSITION p_position = &p_msg_insert_foreign->position;
    PCTSTR p_pathname = p_msg_insert_foreign->filename;
    ARRAY_HANDLE h_data = p_msg_insert_foreign->array_handle;
    BOOL data_allocated = 0;
    BOOL insert_as_table = FALSE;

    IGNOREPARM_InVal_(t5_message);

    /* If it's a Letter-derived document, insert as table */
    if(p_docu->flags.base_single_col)
        insert_as_table = TRUE;

    if(NULL == (arg_stack = al_ptr_calloc_elem(P_USTR, MAXSTACK, &status)))
        return(status);

    process_status_init(&process_status);

    if(0 != h_data)
    {
        process_status.flags.foreground = TRUE;
        process_status.reason.type = UI_TEXT_TYPE_RESID;
        process_status.reason.text.resource_id = MSG_STATUS_CONVERTING; /* that's what we're about to do */
        process_status_begin(p_docu, &process_status, PROCESS_STATUS_PERCENT);
    }
    else
    {
        if(status_ok(status = file_memory_load(p_docu, &h_data, p_pathname, &process_status, sizeof32(lf_BOF_opcode) + sizeof32(lf_BOF_WK1))))
        {
            data_allocated = TRUE;
        }
    }

    if(0 != h_data)
    {
        S32 s_col, e_col, s_row, e_row;

        if(status_ok(status = lotus123_read_file_limits(&s_col, &e_col, &s_row, &e_row, &h_data)))
        {
            DOCU_AREA docu_area;
            POSITION position_actual;
            COL n_cols = (COL) (e_col - s_col);
            ROW n_rows = (ROW) (e_row - s_row);

            docu_area_init(&docu_area);
            docu_area.br.slr.col = n_cols;
            docu_area.br.slr.row = n_rows;

            if(status_ok(status = cells_docu_area_insert(p_docu, p_position, &docu_area, &position_actual)))
            {
                status = format_col_row_extents_set(p_docu, n_cols, n_rows);

                if(status_ok(status))
                {
                    if(3 == lf_version)
                        status = lotus123_load_all_cells_wk3(p_docu, &h_data, &process_status);
                    else
                        status = lotus123_load_all_cells_wk1(p_docu, &h_data, &process_status);
                }

                if(status_ok(status) && insert_as_table)
                    status = lotus123_auto_width_table(p_docu, n_cols, n_rows);
            }
        }
    }

    process_status_end(&process_status);

    if(data_allocated) al_array_dispose(&h_data);
    al_ptr_dispose((P_P_ANY) &arg_stack);
    return(status);
}

/******************************************************************************
*
* Lotus 1-2-3 file converter object event handler
*
******************************************************************************/

T5_MSG_PROTO(static, lotus123_msg_initclose, _InRef_ PC_MSG_INITCLOSE p_msg_initclose)
{
    IGNOREPARM_DocuRef_(p_docu);
    IGNOREPARM_InVal_(t5_message);

    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__STARTUP:
        return(resource_init(OBJECT_ID_FL_LOTUS123, MSG_WEAK, P_BOUND_RESOURCES_OBJECT_ID_FL_LOTUS123));

    case T5_MSG_IC__EXIT1:
        return(resource_close(OBJECT_ID_FL_LOTUS123));

    default:
        return(STATUS_OK);
    }
}

OBJECT_PROTO(extern, object_fl_lotus123);
OBJECT_PROTO(extern, object_fl_lotus123)
{
    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        return(lotus123_msg_initclose(p_docu, t5_message, (PC_MSG_INITCLOSE) p_data));

    case T5_MSG_INSERT_FOREIGN:
        return(lotus123_msg_insert_foreign(p_docu, t5_message, (P_MSG_INSERT_FOREIGN) p_data));

    default:
        return(STATUS_OK);
    }
}

/* end of fl_123.c */
