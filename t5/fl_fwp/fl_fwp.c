/* fl_fwp.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* First Word Plus load object module for Fireworkz */

/* MRJC December 31 1992 */

#include "common/gflags.h"

#include "fl_fwp/fl_fwp.h"

/*
callback routines
*/

#if RISCOS
#define MSG_WEAK &rb_fl_fwp_msg_weak
extern PC_U8 rb_fl_fwp_msg_weak;
#endif

#define P_BOUND_RESOURCES_OBJECT_ID_FWP DONT_LOAD_RESOURCES

/*
internal routines
*/

static void
fwp_file_strip(
    _InRef_     P_ARRAY_HANDLE p_h_data /*data modified*/);

_Check_return_
static S32
fwp_para_count(
    _InRef_     PC_ARRAY_HANDLE p_h_data);

_Check_return_
static STATUS
fwp_para_make(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _OutRef_    P_ARRAY_HANDLE p_h_hilites,
    _InRef_     PC_ARRAY_HANDLE p_h_data,
    _InoutRef_  P_U32 p_offset);

_Check_return_
static STATUS
fwp_regions_make(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_ARRAY_HANDLE p_h_hilites,
    _InRef_     PC_SLR p_slr);

/* 1) load FWP file into memory */
/* 2) paragraphise it - this overwrites unwanted line breaks with spaces,
                      - highlights go to temporary codes
                      - all other characters are blatted with spaces
      at the end, we will know how many paragraphs there are */
/* 3) insert relevant space into the Fireworkz document */
/* 4) load file; strip trailing and multiple spaces */

T5_MSG_PROTO(static, fwp_msg_insert_foreign, _InoutRef_ P_MSG_INSERT_FOREIGN p_msg_insert_foreign)
{
    P_POSITION p_position = &p_msg_insert_foreign->position;
    STATUS status = STATUS_OK;
    ARRAY_HANDLE h_data = 0;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(status_ok(status = file_memory_load(p_docu, &h_data, p_msg_insert_foreign->filename, NULL, 1)))
    {
        S32 para_count;

        fwp_file_strip(&h_data);

        if((para_count = fwp_para_count(&h_data)) != 0)
        {
            DOCU_AREA docu_area;
            POSITION position_actual;

            docu_area_init(&docu_area);
            docu_area.br.slr.col = 1;
            docu_area.br.slr.row = para_count;

            if(status_ok(status = cells_docu_area_insert(p_docu, p_position, &docu_area, &position_actual)))
            {
                S32 row_count = para_count;
                U32 para_offset = 0;
                POSITION position = position_actual;

                while(row_count--)
                {
                    ARRAY_HANDLE h_hilites;
                    QUICK_UBLOCK_WITH_BUFFER(quick_ublock_para, 500);
                    quick_ublock_with_buffer_setup(quick_ublock_para);

                    if(status_ok(status = fwp_para_make(&quick_ublock_para, &h_hilites, &h_data, &para_offset)))
                    {
                        if((0 != quick_ublock_bytes(&quick_ublock_para)) && status_ok(status = quick_ublock_nullch_add(&quick_ublock_para)))
                        {
                            LOAD_CELL_FOREIGN load_cell_foreign;
                            zero_struct(load_cell_foreign);
                            status_consume(object_data_from_position(p_docu, &load_cell_foreign.object_data, &position, P_OBJECT_POSITION_NONE));
                            load_cell_foreign.data_type = OWNFORM_DATA_TYPE_TEXT;

                            load_cell_foreign.ustr_inline_contents = quick_ublock_ustr_inline(&quick_ublock_para);
                            load_cell_foreign.original_slr = position.slr;
                            status = object_call_id(OBJECT_ID_TEXT, p_docu, T5_MSG_LOAD_CELL_FOREIGN, &load_cell_foreign);

                            status_assert(fwp_regions_make(p_docu, &h_hilites, &position.slr));

                            position.slr.row += 1;
                            quick_ublock_dispose(&quick_ublock_para);
                        }
                    }

                    al_array_dispose(&h_hilites);

                    status_break(status);
                }
            }
        }
    }

    al_array_dispose(&h_data);
    return(status);
}

/******************************************************************************
*
* furtle about in FWP file and remove junk
*
* FWP_LF/FWP_RET normalised to FWP_RET for further processing
*
******************************************************************************/

static void
fwp_file_strip(
    _InRef_     P_ARRAY_HANDLE p_h_data /*data modified*/)
{
    const U32 n_file_bytes = array_elements(p_h_data);
    const P_U8 p_u8_start = array_range(p_h_data, U8, 0, n_file_bytes);
    const P_U8 p_u8_end = p_u8_start + n_file_bytes;
    P_U8 p_u8 = p_u8_start;
    U8 soft = 0, lf_cr = 0;

    while(p_u8 < p_u8_end)
    {
        switch(*p_u8)
        {
        case CH_TAB:
            ++p_u8; /* TABs OK */
            break;

        case FWP_LF:
        case FWP_RET:
            if(!lf_cr)
            {
                lf_cr = 1;
                *p_u8++ = (U8) (soft ? FWP_SS1 : FWP_RET);
                soft = 0;
                continue;
            }
            *p_u8++ = FWP_SS1;
            break;

        case FWP_CPB:
            *p_u8++ = FWP_SS1;
            if(p_u8 < p_u8_end)
                *p_u8++ = FWP_SS1;
            break;

        case FWP_UPB: /* SKS 03feb93 */
            *p_u8++ = FWP_RET;
            break;

        case FWP_FN:
            assert0();
            *p_u8++ = FWP_SS1;
            while(p_u8 < p_u8_end && *p_u8 != FWP_FN)
                *p_u8++ = FWP_SS1;
            if(p_u8 < p_u8_end)
                *p_u8++ = FWP_SS1;
            break;

        case FWP_SH:
            *p_u8++ = UCH_SOFT_HYPHEN;
            soft = 1;
            lf_cr = 0;
            continue;

        case FWP_ESC:
            switch(p_u8[1] & 0xC0)
            {
            case 0x80: /* leave highlights intact */
                p_u8 += 2;
                break;

            case 0xC0: /* skip to <27,0> see RISC OS 3 user guide Fancy text file format */
                *p_u8++ = FWP_SS1;
                if(p_u8 < p_u8_end)
                    *p_u8++ = FWP_SS1;
                while(p_u8 < p_u8_end && !(p_u8[0] == FWP_ESC && p_u8[1] == 0))
                    *p_u8++ = FWP_SS1;

            default: /* assume others are two character sequences */
                *p_u8++ = FWP_SS1;
                if(p_u8 < p_u8_end)
                    *p_u8++ = FWP_SS1;
                break;
            }
            break;

        /* soft spaces */
        case FWP_SS1:
        case FWP_SS2:
        case FWP_SS3:
            *p_u8++ = FWP_SS1;
            soft = 1;
            lf_cr = 0;
            continue;

        /* wipe out format line */
        case FWP_FMT:
            {
            while((*p_u8 != FWP_LF)
                  &&
                  (*p_u8 != FWP_RET)
                  &&
                  (p_u8 < p_u8_end))
            {
                *p_u8++ = FWP_SS1;
            }

            if(p_u8 < p_u8_end)
                *p_u8++ = FWP_SS1;
            break;
            }

        case CH_DELETE:
            *p_u8++ = FWP_SS1;
            break;

        default:
            if(*p_u8 <= 0x1F)
                *p_u8++ = FWP_SS1; /* SKS 03feb93 for unhandled CtrlChar */
            else
                ++p_u8;  /* a normal character */
            break;
        }

        soft = lf_cr = 0;
    }
}

/******************************************************************************
*
* count the paragraphs in a furtled file
*
******************************************************************************/

_Check_return_
static S32
fwp_para_count(
    _InRef_     PC_ARRAY_HANDLE p_h_data)
{
    const U32 n_file_bytes = array_elements(p_h_data);
    const PC_U8 p_u8_start = array_rangec(p_h_data, U8, 0, n_file_bytes);
    const PC_U8 p_u8_end = p_u8_start + n_file_bytes;
    PC_U8 p_u8 = p_u8_start;
    S32 para_count = 0;

    while(p_u8 < p_u8_end)
        if(*p_u8++ == FWP_RET)
            ++para_count;

    return(para_count);
}

/******************************************************************************
*
* bundle the stripped data into data for input to the text object
*
******************************************************************************/

_Check_return_
static STATUS
fwp_para_make(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _OutRef_    P_ARRAY_HANDLE p_h_hilites,
    _InRef_     PC_ARRAY_HANDLE p_h_data,
    _InoutRef_  P_U32 p_offset)
{
    STATUS status = STATUS_OK;
    const U32 n_bytes_remain = array_elements32(p_h_data) - *p_offset;
    const PC_U8 p_u8_start = array_rangec(p_h_data, U8, *p_offset, n_bytes_remain);
    const PC_U8 p_u8_end = p_u8_start + n_bytes_remain;
    PC_U8 p_u8 = p_u8_start;
    BOOL end_para = 0, space = 0, had_text = 0;

    *p_h_hilites = 0;

    while((p_u8 < p_u8_end) && !end_para && status_ok(status))
    {
        switch(*p_u8)
        {
        case CH_TAB:
            if(had_text)
                status = inline_quick_ublock_IL_TAB(p_quick_ublock);
            ++p_u8;
            space = 0;
            break;

        case FWP_RET:
            end_para = 1;
            had_text = 0;
            ++p_u8;
            break;

        case FWP_SS1:
            space = had_text;
            ++p_u8;
            break;

        case FWP_ESC:
            {
            SC_ARRAY_INIT_BLOCK array_init_block = aib_init(2, sizeof32(HILITE), TRUE);
            P_HILITE p_hilite;

            if(space && had_text)
                status_break(status = quick_ublock_a7char_add(p_quick_ublock, CH_SPACE));

            if(NULL == (p_hilite = al_array_extend_by(p_h_hilites, HILITE, 1, &array_init_block, &status)))
                break;

            p_hilite->pos = quick_ublock_bytes(p_quick_ublock);
            p_hilite->state = p_u8[1];

            p_u8 += 2;
            space = 0;
            break;
            }

        case CH_SPACE:
            if(had_text)
            {
                status_break(status = quick_ublock_a7char_add(p_quick_ublock, CH_SPACE));
                space = 0;
            }
            ++p_u8;
            break;

        default:
            if(space && had_text)
                status_break(status = quick_ublock_a7char_add(p_quick_ublock, CH_SPACE));

            status_break(status = quick_ublock_ucs4_add(p_quick_ublock, *p_u8)); /* Probably Latin-1 */
            ++p_u8;
            had_text = 1;
            space = 0;
            break;
        }
    }

    *p_offset += PtrDiffElemU32(p_u8, p_u8_start);

    return(status);
}

/******************************************************************************
*
* make up regions from the hilite list; this
* is done in an elementary fashion, relying on
* the region adding and subsuming to do any
* sensible minimising of regions
*
******************************************************************************/

_Check_return_
static STATUS
fwp_regions_add(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_HILITE p_hilite_on,
    _InRef_     PC_HILITE p_hilite_off,
    _InRef_     PC_SLR p_slr)
{
    DOCU_AREA docu_area;
    STYLE_DOCU_AREA_ADD_PARM style_docu_area_add_parm;
    STYLE style;
    int state;

    docu_area_init(&docu_area);
    docu_area.tl.slr = *p_slr;
    docu_area.br.slr.col = p_slr->col + 1;
    docu_area.br.slr.row = p_slr->row + 1;

    if(p_hilite_on->pos >= 0)
    {
        docu_area.tl.object_position.object_id = OBJECT_ID_TEXT;
        docu_area.tl.object_position.data = p_hilite_on->pos;
    }
    else
        object_position_init(&docu_area.tl.object_position);

    if(p_hilite_off->pos >= 0)
    {
        docu_area.br.object_position.object_id = OBJECT_ID_TEXT;
        docu_area.br.object_position.data = p_hilite_off->pos;
    }
    else
        object_position_init(&docu_area.br.object_position);

    state = (p_hilite_on->state ^ p_hilite_off->state) & p_hilite_on->state;

    style_init(&style);

    if(state & FWP_STATE_BOLD)
    {
        style_bit_set(&style, STYLE_SW_FS_BOLD);
        style.font_spec.bold = 1;
    }

    if(state & FWP_STATE_UNDERLINE)
    {
        style_bit_set(&style, STYLE_SW_FS_UNDERLINE);
        style.font_spec.underline = 1;
    }

    if(state & FWP_STATE_ITALIC)
    {
        style_bit_set(&style, STYLE_SW_FS_ITALIC);
        style.font_spec.italic = 1;
    }

    if(state & FWP_STATE_SUPER)
    {
        style_bit_set(&style, STYLE_SW_FS_SUPERSCRIPT);
        style.font_spec.superscript = 1;
    }

    if(state & FWP_STATE_SUB)
    {
        style_bit_set(&style, STYLE_SW_FS_SUBSCRIPT);
        style.font_spec.subscript = 1;
    }

    p_hilite_on->state &= ~state;

    STYLE_DOCU_AREA_ADD_STYLE(&style_docu_area_add_parm, &style);
    return(style_docu_area_add(p_docu, &docu_area, &style_docu_area_add_parm));
}

_Check_return_
static STATUS
fwp_regions_make(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_ARRAY_HANDLE p_h_hilites,
    _InRef_     PC_SLR p_slr)
{
    const ARRAY_INDEX n_elements = array_elements(p_h_hilites);

    if(0 != n_elements)
    {
        ARRAY_INDEX i;
        PC_HILITE p_hilite = array_rangec(p_h_hilites, HILITE, 0, n_elements);
        HILITE hilite_state[N_HILITES] = { 0 };
        HILITE hilite_off;
        S32 x;

        hilite_state[0] = *p_hilite++;

        for(i = 1; i < n_elements; ++i, ++p_hilite)
        {
            U8 state;

            /* generate regions for effects being switched off */
            for(x = 0; x < N_HILITES; ++x)
                if(hilite_state[x].state & ~p_hilite->state)
                    status_return(fwp_regions_add(p_docu, &hilite_state[x], p_hilite, p_slr));

            /* accumulate current state */
            state = p_hilite->state;
            for(x = 0; x < N_HILITES; ++x)
                state &= ~hilite_state[x].state;

            if(state)
            {
                for(x = 0; x < N_HILITES; ++x)
                    if(!hilite_state[x].state)
                    {
                        hilite_state[x] = *p_hilite;
                        hilite_state[x].state &= state;
                        break;
                    }
            }
        }

        /* all regions should be off at the end */
        hilite_off.pos = - 1;
        hilite_off.state = 0;
        for(x = 0; x < N_HILITES; ++x)
            if(hilite_state[x].state & ~hilite_off.state)
                status_return(fwp_regions_add(p_docu, &hilite_state[x], &hilite_off, p_slr));
    }

    return(STATUS_OK);
}

/******************************************************************************
*
* First Word Plus file converter object event handler
*
******************************************************************************/

T5_MSG_PROTO(static, fwp_msg_initclose, _InRef_ PC_MSG_INITCLOSE p_msg_initclose)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__STARTUP:
        return(resource_init(OBJECT_ID_FL_FWP, MSG_WEAK, P_BOUND_RESOURCES_OBJECT_ID_FWP));

    case T5_MSG_IC__EXIT1:
        return(resource_close(OBJECT_ID_FL_FWP));

    default:
        return(STATUS_OK);
    }
}

OBJECT_PROTO(extern, object_fl_fwp);
OBJECT_PROTO(extern, object_fl_fwp)
{
    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        return(fwp_msg_initclose(p_docu, t5_message, (PC_MSG_INITCLOSE) p_data));

    case T5_MSG_INSERT_FOREIGN:
        return(fwp_msg_insert_foreign(p_docu, t5_message, (P_MSG_INSERT_FOREIGN) p_data));

    default:
        return(STATUS_OK);
    }
}

/* end of fl_fwp.c */
