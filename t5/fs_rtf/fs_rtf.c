/* fs_rtf.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* RTF save object module for Fireworkz */

/* JAD Jul 1993 */

#include "common/gflags.h"

#include "fs_rtf/fs_rtf.h"

#include "ob_skel/ff_io.h"

#include "cmodules/unicode/u2000.h" /* 2000..206F General Punctuation */
#include "cmodules/utf16.h"

#if RISCOS
#if defined(BOUND_MESSAGES_OBJECT_ID_FS_RTF)
extern PC_U8 rb_fs_rtf_msg_weak;
#define P_BOUND_MESSAGES_OBJECT_ID_FS_RTF &rb_fs_rtf_msg_weak
#else
#define P_BOUND_MESSAGES_OBJECT_ID_FS_RTF DONT_LOAD_MESSAGES_FILE
#endif
#else
#define P_BOUND_MESSAGES_OBJECT_ID_FS_RTF DONT_LOAD_MESSAGES_FILE
#endif

#define P_BOUND_RESOURCES_OBJECT_ID_FS_RTF DONT_LOAD_RESOURCES

/*
internal routines
*/

_Check_return_
static STATUS
style_enum_layers_from_slr(
    _DocuRef_   P_DOCU p_docu,
    P_ARRAY_INDEX p_index,
    _InoutRef_  P_STYLE p_style_out,
    _InRef_     PC_SLR p_slr,
    _InRef_     PC_STYLE_SELECTOR p_style_selector);

/* Saving */

_Check_return_
static STATUS
rtf_output_fonttbl(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format);

_Check_return_
static STATUS
rtf_output_colortbl(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format);

_Check_return_
static STATUS
rtf_output_stylesheet(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format);

_Check_return_
static STATUS
rtf_save_style_attributes(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    _InRef_     PC_STYLE p_style,
    _InRef_     PC_STYLE_SELECTOR p_style_selector,
    _InVal_     BOOL turn_off,
    P_BOOL p_ignore);

_Check_return_
static STATUS
stylesheet_number_from_style_name(
    _In_z_      PCTSTR tstr_style_name);

_Check_return_
static STATUS
rtf_save_data_ref(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DATA_REF p_data_ref,
    P_OBJECT_POSITION p_object_position,
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    _In_z_      PC_USTR_INLINE ustr_inline,
    P_ARRAY_HANDLE p_h_style_list);

static ARRAY_HANDLE   h_colour_table_save;    /* list of all colours used in Fireworkz styles */

static ARRAY_HANDLE   h_stylesheet_save;      /* keeps names in stylesheet number order */

static BOOL           first_section;

#define RTF_LINESEP_STR "\x0D\x0A"

/**************************************************** Save Code *******************************************************/

_Check_return_
static STATUS
rtf_save_hefo_section(
    _DocuRef_   P_DOCU p_docu,
    P_ROW p_row,
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    P_PAGE_HEFO_BREAK p_page_hefo_break)
{
    PC_USTR_INLINE ustr_inline;
    DATA_REF data_ref;
    OBJECT_POSITION object_position;
    object_position_init(&object_position);
    object_position.object_id = OBJECT_ID_HEFO;

    if(page_hefo_selector_bit_test(&p_page_hefo_break->selector, PAGE_HEFO_HEADER_ODD))
    {
        P_HEADFOOT_DEF p_headfoot_def = &p_page_hefo_break->header_odd;
        status_return(plain_write_a7str(p_ff_op_format, "{" "\\" "header" "\\" "pard" "\\" "plain "));
        if(NULL != (ustr_inline = ustr_inline_from_h_ustr(&p_headfoot_def->headfoot.h_data)))
        {
            data_ref.arg.row = *p_row;
            data_ref.data_space = DATA_HEADER_ODD;
            status_return(rtf_save_data_ref(p_docu, &data_ref, &object_position, p_ff_op_format, ustr_inline, &p_headfoot_def->headfoot.h_style_list));
        }
        status_return(plain_write_a7str(p_ff_op_format, RTF_LINESEP_STR "}" RTF_LINESEP_STR));
    }

    if(page_hefo_selector_bit_test(&p_page_hefo_break->selector, PAGE_HEFO_HEADER_EVEN))
    {
        P_HEADFOOT_DEF p_headfoot_def = &p_page_hefo_break->header_even;
        status_return(plain_write_a7str(p_ff_op_format, "{" "\\" "headerl "));
        if(NULL != (ustr_inline = ustr_inline_from_h_ustr(&p_headfoot_def->headfoot.h_data)))
        {
            data_ref.arg.row = *p_row;
            data_ref.data_space = DATA_HEADER_EVEN;
            status_return(rtf_save_data_ref(p_docu, &data_ref, &object_position, p_ff_op_format, ustr_inline, &p_headfoot_def->headfoot.h_style_list));
        }
        status_return(plain_write_a7str(p_ff_op_format, RTF_LINESEP_STR "}" RTF_LINESEP_STR));
    }

    if(page_hefo_selector_bit_test(&p_page_hefo_break->selector, PAGE_HEFO_HEADER_FIRST))
    {
        P_HEADFOOT_DEF p_headfoot_def = &p_page_hefo_break->header_first;
        status_return(plain_write_a7str(p_ff_op_format, "{" "\\" "headerf "));
        if(NULL != (ustr_inline = ustr_inline_from_h_ustr(&p_headfoot_def->headfoot.h_data)))
        {
            data_ref.arg.row = *p_row;
            data_ref.data_space = DATA_HEADER_FIRST;
            status_return(rtf_save_data_ref(p_docu, &data_ref, &object_position, p_ff_op_format, ustr_inline, &p_headfoot_def->headfoot.h_style_list));
        }
        status_return(plain_write_a7str(p_ff_op_format, RTF_LINESEP_STR "}" RTF_LINESEP_STR));
    }

    if(page_hefo_selector_bit_test(&p_page_hefo_break->selector, PAGE_HEFO_FOOTER_ODD))
    {
        P_HEADFOOT_DEF p_headfoot_def = &p_page_hefo_break->footer_odd;
        status_return(plain_write_a7str(p_ff_op_format, "{" "\\" "footer "));
        if(NULL != (ustr_inline = ustr_inline_from_h_ustr(&p_headfoot_def->headfoot.h_data)))
        {
            data_ref.arg.row = *p_row;
            data_ref.data_space = DATA_FOOTER_ODD;
            status_return(rtf_save_data_ref(p_docu, &data_ref, &object_position, p_ff_op_format, ustr_inline, &p_headfoot_def->headfoot.h_style_list));
        }
        status_return(plain_write_a7str(p_ff_op_format, RTF_LINESEP_STR "}" RTF_LINESEP_STR));
    }

    if(page_hefo_selector_bit_test(&p_page_hefo_break->selector, PAGE_HEFO_FOOTER_EVEN))
    {
        P_HEADFOOT_DEF p_headfoot_def = &p_page_hefo_break->footer_even;
        status_return(plain_write_a7str(p_ff_op_format, "{" "\\" "footerl "));
        if(NULL != (ustr_inline = ustr_inline_from_h_ustr(&p_headfoot_def->headfoot.h_data)))
        {
            data_ref.arg.row = *p_row;
            data_ref.data_space = DATA_FOOTER_EVEN;
            status_return(rtf_save_data_ref(p_docu, &data_ref, &object_position, p_ff_op_format, ustr_inline, &p_headfoot_def->headfoot.h_style_list));
        }
        status_return(plain_write_a7str(p_ff_op_format, RTF_LINESEP_STR "}" RTF_LINESEP_STR));
    }

    if(page_hefo_selector_bit_test(&p_page_hefo_break->selector, PAGE_HEFO_FOOTER_FIRST))
    {
        P_HEADFOOT_DEF p_headfoot_def = &p_page_hefo_break->footer_first;
        status_return(plain_write_a7str(p_ff_op_format, "{" "\\" "footerf "));
        if(NULL != (ustr_inline = ustr_inline_from_h_ustr(&p_headfoot_def->headfoot.h_data)))
        {
            data_ref.arg.row = *p_row;
            data_ref.data_space = DATA_FOOTER_FIRST;
            status_return(rtf_save_data_ref(p_docu, &data_ref, &object_position, p_ff_op_format, ustr_inline, &p_headfoot_def->headfoot.h_style_list));
        }
        status_return(plain_write_a7str(p_ff_op_format, RTF_LINESEP_STR "}" RTF_LINESEP_STR));
    }

    return(STATUS_OK);
}

/******************************************************************************
*
* Outputs all RTF header information
* such as info, fonttbl, colortbl, character set, version etc.
*
******************************************************************************/

_Check_return_
static STATUS
rtf_save_fileheader_info(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    _InRef_     PC_STYLE p_style)
{
    S32 default_font_number;
    U8Z buffer[256];

    /* main document block, version and character set. */
    /* NB any \u characters emitted have no associate substitute character */
    status_return(plain_write_a7str(p_ff_op_format, "{" "\\" "rtf1" "\\" "ansi" "\\" "uc0"));

    /* default font */
    status_return(default_font_number = create_error(fontmap_app_index_from_app_font_name(array_tstr(&p_style->font_spec.h_app_name_tstr))));

    consume_int(xsnprintf(buffer, elemof32(buffer),
                          "\\" "deff" S32_FMT RTF_LINESEP_STR, default_font_number));
    status_return(plain_write_a7str(p_ff_op_format, buffer));

    status_return(rtf_output_fonttbl(p_ff_op_format));

    status_return(rtf_output_colortbl(p_docu, p_ff_op_format));

    status_return(rtf_output_stylesheet(p_docu, p_ff_op_format));

    consume_int(xsnprintf(buffer, elemof32(buffer),
                          "\\" "margl" S32_FMT "\\" "margr" S32_FMT
                          "\\" "margt" S32_FMT "\\" "margb" S32_FMT RTF_LINESEP_STR,
                          p_docu->page_def.margin_left, p_docu->page_def.margin_right + 180,
                          p_docu->page_def.margin_top, p_docu->page_def.margin_bottom));
    status_return(plain_write_a7str(p_ff_op_format, buffer));

    return(STATUS_OK);
}

/******************************************************************************
*
* Output RTF font table block
*
******************************************************************************/

_Check_return_
static STATUS
rtf_output_fonttbl(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
    FONT_SPEC font_spec;
    ARRAY_INDEX app_index;

    status_return(plain_write_a7str(p_ff_op_format, "{" "\\" "fonttbl"));

    for(app_index = 0; status_ok(fontmap_font_spec_from_app_index(&font_spec, app_index)); ++app_index)
    {
        STATUS status;
        QUICK_TBLOCK_WITH_BUFFER(quick_tblock, 64);
        quick_tblock_with_buffer_setup(quick_tblock);

        if(status_ok(status = quick_tblock_printf(&quick_tblock, TEXT(RTF_LINESEP_STR) TEXT("{") TEXT("\\") TEXT("f") S32_TFMT TEXT("\\") TEXT("f"), (S32) app_index)))
        if(status_ok(status = fontmap_rtf_class_from_font_spec(&quick_tblock, &font_spec)))
        if(status_ok(status = quick_tblock_tchar_add(&quick_tblock, CH_SPACE)))
        {
            ARRAY_HANDLE_TSTR h_host_name_tstr;
            if(status_ok(status = fontmap_host_base_name_from_app_font_name(&h_host_name_tstr, array_tstr(&font_spec.h_app_name_tstr))))
                status = quick_tblock_tstr_add(&quick_tblock, array_tstr(&h_host_name_tstr));
            al_array_dispose(&h_host_name_tstr);
        }
        if(status_ok(status))
        if(status_ok(quick_tblock_tstr_add(&quick_tblock, TEXT(";}"))))
            status = plain_write_tchars(p_ff_op_format, quick_tblock_tchars(&quick_tblock), quick_tblock_chars(&quick_tblock));

        quick_tblock_dispose(&quick_tblock);

        font_spec_dispose(&font_spec);

        status_return(status);
    }

    return(plain_write_a7str(p_ff_op_format, RTF_LINESEP_STR "}" RTF_LINESEP_STR));
}

/******************************************************************************
*
* Lookup colour in table.
*
* Return index of element as colour table entry
*
******************************************************************************/

_Check_return_
static ARRAY_INDEX
find_colour_in_table(
    _InRef_     PC_ARRAY_HANDLE p_h_colour_table,
    _InRef_     PC_RGB p_rgb,
    _OutRef_    P_BOOL p_found)
{
    const ARRAY_INDEX n_elements = array_elements(p_h_colour_table);
    ARRAY_INDEX index;

    for(index = 0; index < n_elements; index++)
    {
        PC_RGB p_table_rgb = array_ptrc(p_h_colour_table, RGB, index);

        if((p_table_rgb->r == p_rgb->r) && (p_table_rgb->g == p_rgb->g) && (p_table_rgb->b == p_rgb->b))
        {
            *p_found = TRUE;
            return(index);
        }
    }

    *p_found = FALSE;
    return(0);
}

/******************************************************************************
*
* Lookup colour in table, adding as necessary.
*
* Return index of element as colour table entry
*
******************************************************************************/

_Check_return_
static STATUS
ensure_colour_in_table(
    _InoutRef_  P_ARRAY_HANDLE p_h_colour_table,
    _InRef_     PC_RGB p_rgb)
{
    BOOL found;
    ARRAY_INDEX index = find_colour_in_table(p_h_colour_table, p_rgb, &found);

    if(!found)
    {
        SC_ARRAY_INIT_BLOCK array_init_block = aib_init(4, sizeof32(RGB), TRUE);
        index = array_elements(p_h_colour_table);
        status_return(al_array_add(p_h_colour_table, RGB, 1, &array_init_block, p_rgb));
    }

    return(index);
}

/******************************************************************************
*
* Output RTF color table block
*
******************************************************************************/

_Check_return_
static STATUS
ensure_style_colour_in_table(
    _InRef_     PC_STYLE p_style,
    _InVal_     STYLE_BIT_NUMBER style_bit_number,
    _InRef_     PC_RGB p_rgb)
{
    if(!style_bit_test(p_style, style_bit_number))
        return(STATUS_OK);

    return(ensure_colour_in_table(&h_colour_table_save, p_rgb));
}

_Check_return_
static STATUS
ensure_style_colours_in_table(
    _InRef_     PC_STYLE p_style)
{
    status_return(ensure_style_colour_in_table(p_style, STYLE_SW_FS_COLOUR,          &p_style->font_spec.colour));
    status_return(ensure_style_colour_in_table(p_style, STYLE_SW_PS_RGB_BACK,        &p_style->para_style.rgb_back));
    status_return(ensure_style_colour_in_table(p_style, STYLE_SW_PS_RGB_GRID_LEFT,   &p_style->para_style.rgb_grid_left));
    status_return(ensure_style_colour_in_table(p_style, STYLE_SW_PS_RGB_GRID_RIGHT,  &p_style->para_style.rgb_grid_right));
    status_return(ensure_style_colour_in_table(p_style, STYLE_SW_PS_RGB_GRID_TOP,    &p_style->para_style.rgb_grid_top));
    status_return(ensure_style_colour_in_table(p_style, STYLE_SW_PS_RGB_GRID_BOTTOM, &p_style->para_style.rgb_grid_bottom));
    return(STATUS_OK);
}

_Check_return_
static STATUS
rtf_output_colortbl(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
    ARRAY_INDEX i;

    {
    static const RGB rgb_black = RGB_INIT_BLACK;
    status_return(ensure_colour_in_table(&h_colour_table_save, &rgb_black));
    } /*block*/

    for(i = 1 /*skip zero*/; i < array_elements(&p_docu->h_style_list); ++i)
        status_return(ensure_style_colours_in_table(p_style_from_handle(p_docu, i)));

    /* Now search through regions for colours */
    for(i = 0; i < array_elements(&p_docu->h_style_docu_area); ++i)
    {
        const P_STYLE_DOCU_AREA p_style_docu_area = array_ptr(&p_docu->h_style_docu_area, STYLE_DOCU_AREA, i);

        if(p_style_docu_area->is_deleted)
            continue;

        status_return(ensure_style_colours_in_table(p_style_from_docu_area(p_docu, p_style_docu_area)));
    }

    status_return(plain_write_a7str(p_ff_op_format, "{" "\\" "colortbl"));

    for(i = 0; i < array_elements(&h_colour_table_save); ++i)
    {
        const PC_RGB p_rgb = array_ptrc(&h_colour_table_save, RGB, i);
        U8Z color_buffer[256];
        consume_int(xsnprintf(color_buffer, sizeof32(color_buffer),
                              RTF_LINESEP_STR "\\" "red" U32_FMT "\\" "green" U32_FMT "\\" "blue" U32_FMT ";",
                              (U32) p_rgb->r, (U32) p_rgb->g, (U32) p_rgb->b));
        status_return(plain_write_a7str(p_ff_op_format, color_buffer));
    }

    return(plain_write_a7str(p_ff_op_format, RTF_LINESEP_STR "}" RTF_LINESEP_STR));
}

/******************************************************************************
*
* Output RTF stylesheet block
*
******************************************************************************/

_Check_return_
static STATUS
rtf_output_stylesheet(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
    S32 index;
    U32 style_number = 0;
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(RTF_STYLESHEET), FALSE);
    P_RTF_STYLESHEET p_rtf_stylesheet;

    status_return(plain_write_a7str(p_ff_op_format, "{" "\\" "stylesheet"));

    for(index = 1 /*skip zero*/; index < array_elements(&p_docu->h_style_list); ++index)
    {
        const PC_STYLE p_style = array_ptrc(&p_docu->h_style_list, STYLE, index);

        if(style_bit_test(p_style, STYLE_SW_NAME))
        {
            /* Put style onto stylesheet */
            STATUS status;

            if(NULL == (p_rtf_stylesheet = al_array_extend_by(&h_stylesheet_save, RTF_STYLESHEET, 1, &array_init_block, &status)))
                return(status);

            tstr_xstrkpy(p_rtf_stylesheet->name, elemof32(p_rtf_stylesheet->name), array_tstr(&p_style->h_style_name_tstr));
            p_rtf_stylesheet->style_number = style_number; /* Should actually be same as index into array anyway ! */

            status_return(plain_write_a7str(p_ff_op_format, RTF_LINESEP_STR "{"));

            /* Write out style control words */
            status_return(rtf_save_style_attributes(p_ff_op_format, p_style, &p_style->selector, FALSE, NULL));

            /* Write name to RTF file, using style_buffer */
            status_return(plain_write_a7str(p_ff_op_format, _sbstr_from_tstr(array_tstr(&p_style->h_style_name_tstr))));

            status_return(plain_write_a7str(p_ff_op_format, ";}"));
        }
    }

    return(plain_write_a7str(p_ff_op_format, RTF_LINESEP_STR "}" RTF_LINESEP_STR));
}

_Check_return_
static STATUS
rtf_save_style_attributes(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    _InRef_     PC_STYLE p_style,
    _InRef_     PC_STYLE_SELECTOR p_style_selector,
    _InVal_     BOOL turn_off,
    P_BOOL p_ignore)
{
    STYLE_BIT_NUMBER style_bit_number;
    BOOL colout = FALSE;
    STATUS status = STATUS_OK;
    S32 style_number = -1;

    /* step through selected bits */
    for(style_bit_number = STYLE_BIT_NUMBER_FIRST;
        (style_bit_number = style_selector_next_bit(p_style_selector, style_bit_number)) >= 0;
        STYLE_BIT_NUMBER_INCR(style_bit_number))
    {
        PC_BYTE p_member = p_style_member(BYTE, p_style, style_bit_number);
        U8Z string_buffer[100];
        PC_U8Z str_out = string_buffer;

        string_buffer[0] = CH_NULL;

        switch(style_bit_number)
        {
        case STYLE_SW_NAME:
            {
            style_number = stylesheet_number_from_style_name(array_tstr((PC_ARRAY_HANDLE_TSTR) p_member));

            if((style_number == 0) && p_ignore)
            {
                if(*p_ignore)
                    return(status);  /* base style not worth saving ? */
                *p_ignore = TRUE;
            }

            if(status_ok(style_number))
                consume_int(xsnprintf(string_buffer, sizeof32(string_buffer), "\\" "s" S32_FMT, style_number));
            break;
            }

        case STYLE_SW_FS_NAME:
            {
            const PC_ARRAY_HANDLE_TSTR p_array_handle_tstr = (PC_ARRAY_HANDLE_TSTR) p_member;
            STATUS font_number = create_error(fontmap_app_index_from_app_font_name(array_tstr(p_array_handle_tstr)));
            if(status_ok(font_number))
                consume_int(xsnprintf(string_buffer, sizeof32(string_buffer), "\\" "f" S32_FMT, (S32) font_number));
            break;
            }

        case STYLE_SW_FS_SIZE_Y:
            consume_int(xsnprintf(string_buffer, sizeof32(string_buffer), "\\" "fs" S32_FMT, (S32) p_style->font_spec.size_y / (PIXITS_PER_POINT / 2) /* half points */));
            break;

        case STYLE_SW_FS_SIZE_X:
            /* Font aspect ?? */
            break;

        case STYLE_SW_FS_BOLD:
            if(p_style->font_spec.bold)
                str_out = "\\" "b";
            else if(turn_off)
                str_out = "\\" "b0";
            break;

        case STYLE_SW_FS_ITALIC:
            if(p_style->font_spec.italic)
                str_out = "\\" "i";
            else if(turn_off)
                str_out = "\\" "i0";
            break;

        case STYLE_SW_FS_UNDERLINE:
            if(p_style->font_spec.underline)
                str_out = "\\" "ul";
            else if(turn_off)
                str_out = "\\" "ul0";
            break;

        case STYLE_SW_FS_SUPERSCRIPT:
            if(p_style->font_spec.superscript)
                str_out = "\\" "up";
            else if(turn_off)
                str_out = "\\" "up0";
            break;

        case STYLE_SW_FS_SUBSCRIPT:
            if(p_style->font_spec.subscript)
                str_out = "\\" "dn";
            else if(turn_off)
                str_out = "\\" "dn0";
            break;

        case STYLE_SW_FS_COLOUR:
        case STYLE_SW_PS_RGB_BACK:
            {
            BOOL found;
            ARRAY_INDEX colour = find_colour_in_table(&h_colour_table_save, (PC_RGB) p_member, &found);
            consume_int(xsnprintf(string_buffer, sizeof32(string_buffer),
                                  (style_bit_number == STYLE_SW_FS_COLOUR)
                                      ? "\\" "cf" S32_FMT
                                      : "\\" "cb" S32_FMT,
                                  (S32) colour));
            break;
            }

        case STYLE_SW_PS_JUSTIFY:
            switch(p_style->para_style.justify)
            {
            default: default_unhandled();
            case SF_JUSTIFY_LEFT:   str_out = "\\" "ql"; break;
            case SF_JUSTIFY_CENTRE: str_out = "\\" "qc"; break;
            case SF_JUSTIFY_RIGHT:  str_out = "\\" "qr"; break;
            case SF_JUSTIFY_BOTH:   str_out = "\\" "qj"; break;
            }
            break;

        case STYLE_SW_PS_JUSTIFY_V:
            switch(p_style->para_style.justify_v)
            {
            default: default_unhandled();
            case SF_JUSTIFY_V_TOP:    str_out = "\\" "vertalt"; break;
            case SF_JUSTIFY_V_CENTRE: str_out = "\\" "vertalc"; break;
            case SF_JUSTIFY_V_BOTTOM: str_out = "\\" "vertal";  break;
            }
            break;

        case STYLE_SW_PS_MARGIN_PARA:
            consume_int(xsnprintf(string_buffer, sizeof32(string_buffer), "\\" "fi" S32_FMT, p_style->para_style.margin_para)); /* offset from lm? */
            break;

        case STYLE_SW_PS_MARGIN_LEFT:
            consume_int(xsnprintf(string_buffer, sizeof32(string_buffer), "\\" "li" S32_FMT, p_style->para_style.margin_left));
            break;

        case STYLE_SW_PS_MARGIN_RIGHT:
            consume_int(xsnprintf(string_buffer, sizeof32(string_buffer), "\\" "ri" S32_FMT, p_style->para_style.margin_right));
            break;

        case STYLE_SW_PS_PARA_START:
            consume_int(xsnprintf(string_buffer, sizeof32(string_buffer), "\\" "sb" S32_FMT, p_style->para_style.para_start));
            break;

        case STYLE_SW_PS_PARA_END:
            consume_int(xsnprintf(string_buffer, sizeof32(string_buffer), "\\" "sa" S32_FMT, p_style->para_style.para_end));
            break;

        case STYLE_SW_RS_HEIGHT:
            consume_int(xsnprintf(string_buffer, sizeof32(string_buffer), "\\" "sl-" S32_FMT, p_style->row_style.height));
            break;

        case STYLE_SW_PS_GRID_LEFT:
            if(p_style->para_style.grid_left)
                str_out = "\\" "brdrl";
            break;

        case STYLE_SW_PS_GRID_TOP:
            if(p_style->para_style.grid_top)
                str_out = "\\" "brdrt";
            break;

        case STYLE_SW_PS_GRID_RIGHT:
            if(p_style->para_style.grid_right)
                str_out = "\\" "brdrr";
            break;

        case STYLE_SW_PS_GRID_BOTTOM:
            if(p_style->para_style.grid_bottom)
                str_out = "\\" "brdrb";
            break;

        case STYLE_SW_PS_BORDER:
            if(p_style->para_style.border)
                str_out = "\\" "box";
            break;

        case STYLE_SW_PS_RGB_BORDER:
        case STYLE_SW_PS_RGB_GRID_LEFT: /* all map to same colour use */
        case STYLE_SW_PS_RGB_GRID_TOP:
        case STYLE_SW_PS_RGB_GRID_RIGHT:
        case STYLE_SW_PS_RGB_GRID_BOTTOM:
            if(!colout)
            {
                BOOL found;
                ARRAY_INDEX colour = find_colour_in_table(&h_colour_table_save, (PC_RGB) p_member, &found);
                consume_int(xsnprintf(string_buffer, sizeof32(string_buffer), "\\" "brdrcf" S32_FMT, (S32) colour));
                colout = TRUE;
            }
            break;

        case STYLE_SW_PS_TAB_LIST:
            {
            PC_ARRAY_HANDLE p_h_tab_list = &p_style->para_style.h_tab_list;
            ARRAY_INDEX tab_index;

            if(style_number == 0 && turn_off)
                break;

            for(tab_index = 0; tab_index < array_elements(p_h_tab_list); ++tab_index)
            {
                PC_TAB_INFO p_tab_info = array_ptrc(p_h_tab_list, TAB_INFO, tab_index);
                PC_A7STR tab_str_out = P_U8_NONE;

                switch(p_tab_info->type)
                {
                default: default_unhandled();
                case TAB_LEFT:                                break;
                case TAB_RIGHT:   tab_str_out = "\\" "tqr";   break;
                case TAB_CENTRE:  tab_str_out = "\\" "tqc";   break;
                case TAB_DECIMAL: tab_str_out = "\\" "tqdec"; break;
                }

                if(P_U8_NONE != tab_str_out)
                    status_break(status = plain_write_a7str(p_ff_op_format, tab_str_out));

                consume_int(xsnprintf(string_buffer, sizeof32(string_buffer), "\\" "tx" S32_FMT, p_tab_info->offset));
                status_break(status = plain_write_a7str(p_ff_op_format, string_buffer));
            }

            string_buffer[0] = CH_NULL;

            break;
            }

        default: default_unhandled();
#if CHECKING
        /* Currently untranslated codes */
        case STYLE_SW_KEY:
        case STYLE_SW_SEARCH:
        case STYLE_SW_CS_WIDTH:
        case STYLE_SW_CS_COL_NAME:
        case STYLE_SW_RS_HEIGHT_FIXED:
        case STYLE_SW_RS_UNBREAKABLE:
        case STYLE_SW_RS_ROW_NAME:
        case STYLE_SW_PS_LINE_SPACE:
        case STYLE_SW_PS_NEW_OBJECT:
        case STYLE_SW_PS_NUMFORM_NU:
        case STYLE_SW_PS_NUMFORM_DT:
        case STYLE_SW_PS_NUMFORM_SE:
        case STYLE_SW_PS_PROTECT:
            break;

        case STYLE_SW_HANDLE:
            myassert0(TEXT("unexpected style handle - what do we do?"));
#endif
            break;
        }

        if(str_out && (CH_NULL != PtrGetByte(str_out)))
            status_break(status = plain_write_a7str(p_ff_op_format, str_out));
    }

    if(status_ok(status))
        status = plain_write_ucs4_character(p_ff_op_format, CH_SPACE);

    return(status);
}

/******************************************************************************
*
* Look up the stylesheet number from a style name.
*
******************************************************************************/

_Check_return_
static STATUS
stylesheet_number_from_style_name(
    _In_z_      PCTSTR tstr_style_name)
{
    const ARRAY_INDEX n_styles = array_elements(&h_stylesheet_save);
    ARRAY_INDEX i;
    P_RTF_STYLESHEET p_rtf_stylesheet = array_range(&h_stylesheet_save, RTF_STYLESHEET, 0, n_styles);

    for(i = 0; i < n_styles; ++i, ++p_rtf_stylesheet)
    {
        if(0 == tstrcmp(p_rtf_stylesheet->name, tstr_style_name))
            return(i);
    }

    return(STATUS_FAIL);
}

_Check_return_
static STATUS
rtf_save_cell(
    _DocuRef_   P_DOCU p_docu,
    P_OBJECT_DATA p_object_data,
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
    STATUS status = STATUS_OK;

    if(P_DATA_NONE != p_object_data->u.p_object)
    {
        if(OBJECT_ID_SS == p_object_data->object_id)
        {
            /* get displayed not stored values */
            OBJECT_READ_TEXT object_read_text;
            QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 256);
            quick_ublock_with_buffer_setup(quick_ublock);

            object_read_text.object_data = *p_object_data;
            object_read_text.p_quick_ublock = &quick_ublock;
            object_read_text.type = OBJECT_READ_TEXT_RESULT;

            if(status_ok(status = object_call_id(object_read_text.object_data.object_id, p_docu, T5_MSG_OBJECT_READ_TEXT, &object_read_text)))
            if(0 != quick_ublock_bytes(object_read_text.p_quick_ublock))
            if(status_ok(status = quick_ublock_nullch_add(object_read_text.p_quick_ublock)))
                  status = rtf_save_data_ref(p_docu,
                                             &object_read_text.object_data.data_ref,
                                             &object_read_text.object_data.object_position_start,
                                             p_ff_op_format,
                                             quick_ublock_ustr_inline(object_read_text.p_quick_ublock),
                                             &p_docu->h_style_docu_area);
            quick_ublock_dispose(&quick_ublock);
        }
        else
        {
            SAVE_CELL_OWNFORM save_cell_ownform;
            U8Z contents_buffer[256];
            U8Z formula_buffer[256];

            zero_struct(save_cell_ownform);

            save_cell_ownform.object_data = *p_object_data;
            save_cell_ownform.p_of_op_format = NULL; /* not required for foreign */

            quick_ublock_setup(&save_cell_ownform.contents_data_quick_ublock, contents_buffer);
            quick_ublock_setup(&save_cell_ownform.formula_data_quick_ublock, formula_buffer);

            if(status_ok(status = object_call_id(p_object_data->object_id, p_docu, T5_MSG_SAVE_CELL_OWNFORM, &save_cell_ownform)))
            if(0 != quick_ublock_bytes(&save_cell_ownform.contents_data_quick_ublock))
            if(status_ok(status = quick_ublock_nullch_add(&save_cell_ownform.contents_data_quick_ublock)))
                    status = rtf_save_data_ref(p_docu,
                                               &p_object_data->data_ref,
                                               &p_object_data->object_position_start,
                                               p_ff_op_format,
                                               quick_ublock_ustr_inline(&save_cell_ownform.contents_data_quick_ublock),
                                               &p_docu->h_style_docu_area);

            quick_ublock_dispose(&save_cell_ownform.contents_data_quick_ublock);
            quick_ublock_dispose(&save_cell_ownform.formula_data_quick_ublock);
        }
    }

    return(status);
}

/******************************************************************************
*
* outputs RTF text, decoding any relevant inlines
*
******************************************************************************/

_Check_return_
static PC_U8Z
rtf_convert_ucs4_for_save(
    _Out_writes_(elemof_buffer) P_U8Z buffer,
    _InVal_     U32 elemof_buffer,
    _InVal_     UCS4 ucs4)
{
    assert(!ucs4_is_C1(ucs4)); /* hopefully no C1 to complicate things */

    switch(ucs4)
    {
    case CH_BACKWARDS_SLASH:
        return("\\" "\\");

    case CH_LEFT_CURLY_BRACKET:
        return("\\" "{");

    case CH_RIGHT_CURLY_BRACKET:
        return("\\" "}");

    case UCH_SOFT_HYPHEN:
        return("\\" "-");

    case UCH_LEFT_SINGLE_QUOTATION_MARK:
        return("\\" "lquote ");

    case UCH_RIGHT_SINGLE_QUOTATION_MARK:
        return("\\" "rquote ");

    case UCH_LEFT_DOUBLE_QUOTATION_MARK:
        return("\\" "ldblquote ");

    case UCH_RIGHT_DOUBLE_QUOTATION_MARK:
        return("\\" "rdblquote ");

    case UCH_EM_DASH:
        return("\\" "emdash ");
        break;

    case UCH_EN_DASH:
        return("\\" "endash ");
        break;

    case UCH_BULLET:
        return("\\" "bullet ");

    case CH_DELETE:
        return(P_U8_NONE);

    default:
        break;
    }

    if(ucs4 < 0x80)
    {
        buffer[0] = (U8) ucs4;
        buffer[1] = CH_NULL;
    }
    else if(ucs4_is_sbchar(ucs4))
    {
        unsigned int parameter = (unsigned int) ucs4;
        consume_int(xsnprintf(buffer, elemof_buffer, "\\" "hich" "\\" "'" "%02X" "\\" "loch" , parameter));
    }
    else if(ucs4 < 0x8000U)
    {
        S32 parameter = (S32) ucs4;
        consume_int(xsnprintf(buffer, elemof_buffer, "\\u" S32_FMT, parameter)); /* NB no substitute character is supplied with \uc0 */
    }
    else if(ucs4 < 0x10000U)
    {
        S32 parameter = (S32) ucs4 - 65536;
        consume_int(xsnprintf(buffer, elemof_buffer, "\\u" S32_FMT, parameter));
    }
#if 1 /* Word 2007 seems to save out a surrogate pair for non-BMP characters - try that */
    else if(ucs4 < UCH_UNICODE_INVALID)
    {
        WCHAR high_surrogate, low_surrogate;
        S32 parameter1, parameter2;
        utf16_char_encode_surrogates(&high_surrogate, &low_surrogate, ucs4);
        /* both surrogate words definitely have bit 15 set */
        parameter1 = (S32) high_surrogate - 65536;
        parameter2 = (S32) low_surrogate  - 65536;
        consume_int(xsnprintf(buffer, elemof_buffer, "\\u" S32_FMT "\\u" S32_FMT, parameter1, parameter2));
    }
#endif
    else
    {
        /* RTF can't handle these */
        return(P_U8_NONE);
    }

    return(buffer);
}

_Check_return_
static STATUS
rtf_save_UTF8_chars(
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    _In_        PC_UCHARS_INLINE uchars_inline)
{
    STATUS status = STATUS_OK;
    const PC_UTF8 utf8 = inline_data_ptr(PC_UTF8, uchars_inline);
    const U32 il_data_size = (U32) inline_data_size(uchars_inline);
    U32 offset = 0;

    while(offset < il_data_size)
    {
        U32 bytes_of_char;
        const UCS4 ucs4 = utf8_char_decode_off(utf8, offset, bytes_of_char);
        U8Z buffer[32];
        PC_A7STR str_out = rtf_convert_ucs4_for_save(buffer, elemof32(buffer), ucs4);

        if(P_U8_NONE != str_out)
            status_break(status = plain_write_a7str(p_ff_op_format, str_out));

        offset += bytes_of_char;
    }

    return(status);
}

_Check_return_
static STATUS
rtf_save_data_ref(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DATA_REF p_data_ref,
    P_OBJECT_POSITION p_object_position,
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    _In_z_      PC_USTR_INLINE ustr_inline,
    P_ARRAY_HANDLE p_h_style_list)
{
    STATUS status = STATUS_OK;
    U32 offset = 0;
    STYLE_SELECTOR selector;
    ARRAY_HANDLE h_style_sub_changes = 0;
    S32 current_style_sub = 0;
    ARRAY_INDEX index = 0;
    STYLE para_style;
    STYLE current_style;

    if(P_DATA_NONE == ustr_inline)
        return(STATUS_OK);

    style_init(&para_style);
    style_init(&current_style);

    style_selector_clear(&selector);
    style_selector_bit_set(&selector, STYLE_SW_NAME);
    style_selector_bit_set(&selector, STYLE_SW_FS_NAME);
    style_selector_bit_set(&selector, STYLE_SW_FS_SIZE_Y);
    style_selector_bit_set(&selector, STYLE_SW_FS_SIZE_X);
    style_selector_bit_set(&selector, STYLE_SW_FS_BOLD);
    style_selector_bit_set(&selector, STYLE_SW_FS_ITALIC);
    style_selector_bit_set(&selector, STYLE_SW_FS_UNDERLINE);
    style_selector_bit_set(&selector, STYLE_SW_FS_SUPERSCRIPT);
    style_selector_bit_set(&selector, STYLE_SW_FS_SUBSCRIPT);
    style_selector_bit_set(&selector, STYLE_SW_FS_COLOUR);

    h_style_sub_changes = style_sub_changes(p_docu, &selector, p_data_ref, p_object_position, p_h_style_list);

    if(DATA_SLOT == p_data_ref->data_space)
    {
        STYLE_SELECTOR selector_more;
        BOOL done_base = FALSE;

        style_selector_copy(&selector_more, &selector);
        style_selector_bit_set(&selector_more, STYLE_SW_PS_RGB_BACK);
        style_selector_bit_set(&selector_more, STYLE_SW_PS_JUSTIFY);
        style_selector_bit_set(&selector_more, STYLE_SW_PS_TAB_LIST);
        style_selector_bit_set(&selector_more, STYLE_SW_PS_MARGIN_PARA);
        style_selector_bit_set(&selector_more, STYLE_SW_PS_MARGIN_LEFT);
        style_selector_bit_set(&selector_more, STYLE_SW_PS_MARGIN_RIGHT);
        style_selector_bit_set(&selector_more, STYLE_SW_PS_PARA_START);
        style_selector_bit_set(&selector_more, STYLE_SW_PS_PARA_END);
        style_selector_bit_set(&selector_more, STYLE_SW_PS_GRID_LEFT);
        style_selector_bit_set(&selector_more, STYLE_SW_PS_GRID_TOP);
        style_selector_bit_set(&selector_more, STYLE_SW_PS_GRID_RIGHT);
        style_selector_bit_set(&selector_more, STYLE_SW_PS_GRID_BOTTOM);
        style_selector_bit_set(&selector_more, STYLE_SW_PS_RGB_GRID_LEFT);
        style_selector_bit_set(&selector_more, STYLE_SW_PS_RGB_GRID_TOP);
        style_selector_bit_set(&selector_more, STYLE_SW_PS_RGB_GRID_RIGHT);
        style_selector_bit_set(&selector_more, STYLE_SW_PS_RGB_GRID_BOTTOM);

        /* get ALL applied style names etc. */

        while(status_ok(style_enum_layers_from_slr(p_docu, &index, &para_style, &p_data_ref->arg.slr, &selector_more)))
        {
            if(style_selector_any(&para_style.selector))
                status_return(rtf_save_style_attributes(p_ff_op_format, &para_style, &para_style.selector, TRUE, &done_base));
            style_copy(&current_style, &para_style, &selector_more);
            style_selector_clear(&para_style.selector);
        }

    }

    while(status_ok(status))
    {
        PC_A7STR str_out = P_U8_NONE;
        U8Z buffer[32];

        if(is_inline_off(ustr_inline, offset))
        {
            switch(inline_code_off(ustr_inline, offset))
            {
            default:             /*assert0();*/             break;
            case IL_TAB:         str_out = "\\" "tab ";     break;
            case IL_PAGE_Y:      str_out = "\\" "chpgn ";   break;
            case IL_DATE:        str_out = "\\" "chdate ";  break; /* may wish to look at argument to see if time/data present (e.g. 'd' vs 'h') */
            case IL_SOFT_HYPHEN: str_out = "\\" "-";        break;
            case IL_RETURN:      str_out = "\\" "line ";    break;
            case IL_UTF8:        status = rtf_save_UTF8_chars(p_ff_op_format, PtrAddBytes(PC_UCHARS_INLINE, ustr_inline, offset)); break;
            }

            offset += inline_bytecount_off(ustr_inline, offset);
        }
        else
        {
            U32 bytes_of_char;
            UCS4 ucs4 = ustr_char_decode_off((PC_USTR) ustr_inline, offset, /*ref*/bytes_of_char); /* handled inlines above */

            if(CH_NULL == ucs4)
                break;

            if(current_style_sub < array_elements(&h_style_sub_changes))
            {
                P_STYLE_SUB_CHANGE p_style_sub_change = array_ptr(&h_style_sub_changes, STYLE_SUB_CHANGE, current_style_sub);

                if((S32) offset >= p_style_sub_change->position.object_position.data)
                {
                    STYLE_SELECTOR style_selector_diff;

                    /* Get difference between the two styles, and save any relevant highlights */
                    consume_bool(style_compare(&style_selector_diff, &current_style, &p_style_sub_change->style));
                    void_style_selector_and(&style_selector_diff, &style_selector_diff, &p_style_sub_change->style.selector);
                    if(style_selector_bit_test(&style_selector_diff, STYLE_SW_NAME) && (stylesheet_number_from_style_name(array_tstr(&p_style_sub_change->style.h_style_name_tstr)) == 0))
                        style_selector_bit_clear(&style_selector_diff, STYLE_SW_NAME);  /* don't attempt to save basestyle (since it won't do owt) */

                    if(style_selector_any(&style_selector_diff))
                        status_break(status = rtf_save_style_attributes(p_ff_op_format, &p_style_sub_change->style, &style_selector_diff, TRUE, NULL));

                    style_copy(&current_style, &p_style_sub_change->style, &p_style_sub_change->style.selector);

                    current_style_sub += 1;
                }
            }

            str_out = rtf_convert_ucs4_for_save(buffer, elemof32(buffer), ucs4);

            offset += bytes_of_char;
        }

        if(P_U8_NONE != str_out)
            status = plain_write_a7str(p_ff_op_format, str_out);
    }

    style_dispose(&current_style);

    return(status);
}

/******************************************************************************
*
* returns next style in list applied to p_slr
*
******************************************************************************/

_Check_return_
static STATUS
style_enum_layers_from_slr(
    _DocuRef_   P_DOCU p_docu,
    P_ARRAY_INDEX p_index,
    _InoutRef_  P_STYLE p_style_out,
    _InRef_     PC_SLR p_slr,
    _InRef_     PC_STYLE_SELECTOR p_style_selector)
{
    STATUS status = STATUS_FAIL;

    for(;;)
    {
        P_STYLE_DOCU_AREA p_style_docu_area;
        ARRAY_INDEX i = ++(*p_index); /* SKS 27sep94 old code could wander reading off array end */

        if(i >= array_elements(&p_docu->h_style_docu_area))
            break;

        p_style_docu_area = array_ptr(&p_docu->h_style_docu_area, STYLE_DOCU_AREA, i);

        if(p_style_docu_area->is_deleted)
            continue;

        if(slr_in_docu_area(&p_style_docu_area->docu_area, p_slr)
        && (OBJECT_ID_NONE == p_style_docu_area->docu_area.tl.object_position.object_id)
        && !p_style_docu_area->internal)
        {
            const PC_STYLE p_style_t = p_style_from_docu_area(p_docu, p_style_docu_area);
            STYLE_SELECTOR selector_set;
            void_style_selector_and(&selector_set, p_style_selector, &p_style_t->selector);
            style_copy(p_style_out, p_style_t, &selector_set);
            status = STATUS_OK;
            break;
        }
    }

    return(status);
}

T5_MSG_PROTO(static, rtf_msg_save_foreign, _InoutRef_ P_MSG_SAVE_FOREIGN p_msg_save_foreign)
{
    P_FF_OP_FORMAT      p_ff_op_format = p_msg_save_foreign->p_ff_op_format;
    SCAN_BLOCK          scan_block;
    STATUS              status = STATUS_OK;
    S32                 table_width = 0;
    STYLE               base_style;
    STYLE               style_previous, style_current;
    STYLE_SELECTOR      selector_font;
    STYLE_SELECTOR      selector_col_wid;
    P_PAGE_HEFO_BREAK   p_page_hefo_break;
    ROW                 row = -1;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    /* Write ASCII-7 encoded file - we hex encode all the top bit set and Unicode characters */
    status_assert(foreign_save_set_io_type(p_ff_op_format, IO_TYPE_ASCII));

    style_init(&base_style);
    style_init(&style_previous);
    style_init(&style_current);

    style_selector_clear(&selector_font);
    style_selector_bit_set(&selector_font, STYLE_SW_FS_NAME);

    style_selector_clear(&selector_col_wid);
    style_selector_bit_set(&selector_col_wid, STYLE_SW_CS_WIDTH);
    style_selector_bit_set(&selector_col_wid, STYLE_SW_PS_GRID_LEFT);
    style_selector_bit_set(&selector_col_wid, STYLE_SW_PS_GRID_RIGHT);
    style_selector_bit_set(&selector_col_wid, STYLE_SW_PS_GRID_TOP);
    style_selector_bit_set(&selector_col_wid, STYLE_SW_PS_GRID_BOTTOM);
    style_selector_bit_set(&selector_col_wid, STYLE_SW_PS_RGB_GRID_LEFT);
    style_selector_bit_set(&selector_col_wid, STYLE_SW_PS_RGB_GRID_RIGHT);
    style_selector_bit_set(&selector_col_wid, STYLE_SW_PS_RGB_GRID_TOP);
    style_selector_bit_set(&selector_col_wid, STYLE_SW_PS_RGB_GRID_BOTTOM);

    /* Initialise static array handles */
    h_stylesheet_save = 0;
    h_colour_table_save = 0;

    { /* NOTE: Take base style as that affecting cell A1 */
    SLR slr;
    slr.col = 0;
    slr.row = 0;
    style_from_slr(p_docu, &base_style, &selector_font, &slr);
}

/* Save out RTF header information */

    /* Main document block */
    status_return(rtf_save_fileheader_info(p_docu, p_ff_op_format, &base_style));

/* Save out cell data */

    status_return(plain_write_a7str(p_ff_op_format, "\\" "sectd " "{"));

    style_dispose(&base_style);

/* header/footer enumeration */

    p_page_hefo_break = page_hefo_break_enum(p_docu, &row);

    if(status_done(cells_scan_init(p_docu, &scan_block, SCAN_ACROSS, SCAN_AREA, &p_ff_op_format->of_op_format.save_docu_area, OBJECT_ID_NONE)))
    {
        OBJECT_DATA object_data;
        SLR init_slr, last_slr;

        init_slr = p_ff_op_format->of_op_format.save_docu_area.tl.slr;

        last_slr.col = init_slr.col;
        last_slr.row = init_slr.row;

        first_section = TRUE;

        /*status_return(plain_write_a7str(p_ff_op_format, "{"));*/

        while(status_done(cells_scan_next(p_docu, &object_data, &scan_block)))
        {
            save_reflect_status((P_OF_OP_FORMAT) p_ff_op_format, cells_scan_percent(&scan_block));

            if(p_page_hefo_break && row <= object_data.data_ref.arg.slr.row)
            {
                /* save out hefo information & get next one */
                status_return(plain_write_ucs4_character(p_ff_op_format, CH_RIGHT_CURLY_BRACKET));
                if(!first_section)
                {
                    status_return(plain_write_a7str(p_ff_op_format, "{" "\\" "sect" "}"));
                    status_return(plain_write_a7str(p_ff_op_format, "\\" "sectd"));
                    first_section = FALSE;
                }
                status_return(rtf_save_hefo_section(p_docu, &row, p_ff_op_format, p_page_hefo_break));
                p_page_hefo_break = page_hefo_break_enum(p_docu, &row);
                status_return(plain_write_a7str(p_ff_op_format, "{"));
            }

            if( ((last_slr.col != 0) && (last_slr.row < object_data.data_ref.arg.slr.row) /* in case text below single col in table */ )
                ||
                (last_slr.col > object_data.data_ref.arg.slr.col) /* next row down in a table */ )
            {
                /* make sure all table cells accounted for, since Word gets a bit stressed if this isn't the case */
                for(; last_slr.col < table_width; last_slr.col++)
                    status_return(plain_write_a7str(p_ff_op_format, "\\" "cell "));
                status_return(plain_write_a7str(p_ff_op_format, RTF_LINESEP_STR "\\" "intbl"));
                status_return(plain_write_a7str(p_ff_op_format, "\\" "row "));
                status_return(plain_write_a7str(p_ff_op_format, "\\" "pard "));
                last_slr.row++;
                last_slr.col = init_slr.col;
            }

            if(last_slr.row < object_data.data_ref.arg.slr.row) /* not in table (probably), but at next row */
            {
                /* output as many newlines as needed */
                do  {
                    status_return(plain_write_a7str(p_ff_op_format, "\\" "par"));
                }
                while(++last_slr.row < object_data.data_ref.arg.slr.row);

                status_return(plain_write_a7str(p_ff_op_format, "}" RTF_LINESEP_STR "{" "\\" "pard "));

                last_slr.col = init_slr.col;
            }

      /* If there's any data to the right, assume we are in a table & output table header info */

            if(   object_data.data_ref.arg.slr.col != 0
                  &&
                  cells_block_is_blank(p_docu, 0, object_data.data_ref.arg.slr.col /* exclusive */, object_data.data_ref.arg.slr.row, 1)
               || (!cells_block_is_blank(p_docu, object_data.data_ref.arg.slr.col+1, n_cols_logical(p_docu), object_data.data_ref.arg.slr.row, 1)
                   && (object_data.data_ref.arg.slr.col == 0
                      ||
                      cells_block_is_blank(p_docu, 0, object_data.data_ref.arg.slr.col /* exclusive */, object_data.data_ref.arg.slr.row, 1)))
               || (OBJECT_ID_SS == object_data.object_id) && object_data.data_ref.arg.slr.col == 0)
            {
                S32 width = 0;
                STYLE style_col;
                SLR max_slr, slr = object_data.data_ref.arg.slr;
                U8Z buffer[20];

                style_init(&style_col);
                slr.col = 0; /* make sure tables start in column A, even though it might be blank */

                status_return(plain_write_a7str(p_ff_op_format, "\\" "trowd " "\\" "trgaph108" "\\" "trleft-108 "));

                max_slr.row = object_data.data_ref.arg.slr.row;
                for( max_slr.col = n_cols_physical(p_docu); max_slr.col != 0 && slr_is_blank(p_docu, &max_slr); max_slr.col--)
                { /*EMPTY*/ }
                table_width = max_slr.col+1;

                for(; slr.col < table_width; slr.col++)
                {
                    STYLE_SELECTOR selector_color;
                    S32  set = 0;

                /* Ought to see if widths same as row above, and not do anything
                    if this is the case ......hmmm                                 */

                    style_selector_clear(&selector_color);
                    style_from_slr(p_docu, &style_col, &selector_col_wid, &slr);
                    width += style_col.col_style.width;
                    if(style_col.para_style.grid_left)
                    {
                        set = 1;
                        style_selector_bit_set(&selector_color, STYLE_SW_PS_RGB_GRID_LEFT);
                        status_return(plain_write_a7str(p_ff_op_format, "\\" "clbrdrl"));
                    }
                    if(style_col.para_style.grid_right)
                    {
                        set = 1;
                        style_selector_clear(&selector_color);
                        style_selector_bit_set(&selector_color, STYLE_SW_PS_RGB_GRID_RIGHT);
                        status_return(plain_write_a7str(p_ff_op_format, "\\" "clbrdrr"));
                    }
                    if(style_col.para_style.grid_top)
                    {
                        set = 1;
                        style_selector_clear(&selector_color);
                        style_selector_bit_set(&selector_color, STYLE_SW_PS_RGB_GRID_TOP);
                        status_return(plain_write_a7str(p_ff_op_format, "\\" "clbrdrt"));
                    }
                    if(style_col.para_style.grid_bottom)
                    {
                        set = 1;
                        style_selector_clear(&selector_color);
                        style_selector_bit_set(&selector_color, STYLE_SW_PS_RGB_GRID_BOTTOM);
                        status_return(plain_write_a7str(p_ff_op_format, "\\" "clbrdrb"));
                    }
                    if(set)
                        status_return(rtf_save_style_attributes(p_ff_op_format, &style_col, &selector_color, TRUE, NULL));

                    consume_int(xsnprintf(buffer, elemof32(buffer), "\\" "cellx" S32_FMT, width));
                    status_return(plain_write_a7str(p_ff_op_format, buffer));
                    style_dispose(&style_col);
                }

                status_return(plain_write_a7str(p_ff_op_format, "\\" "intbl "));
            }

            if(last_slr.col < object_data.data_ref.arg.slr.col)
            {
                /* output as many cell words as needed */
                do  {
                    status_return(plain_write_a7str(p_ff_op_format, "\\" "cell "));
                }
                while(++last_slr.col < object_data.data_ref.arg.slr.col);
            }

            last_slr = object_data.data_ref.arg.slr;

            status_return(rtf_save_cell(p_docu, &object_data, p_ff_op_format));
        }

        if( ((last_slr.col != 0) && (last_slr.row < object_data.data_ref.arg.slr.row) /* in case text below single col in table */ )
            ||
            (last_slr.col > object_data.data_ref.arg.slr.col) /* next row down in a table */ )
        {
            /* make sure all table cells accounted for, since Word gets a bit stressed if this isn't the case */
            for(; last_slr.col < table_width; last_slr.col++)
                status_return(plain_write_a7str(p_ff_op_format, "\\" "cell "));
        }
    }

    /* Terminate last row */
    status_return(plain_write_a7str(p_ff_op_format, RTF_LINESEP_STR "\\" "par "));

    /* Section block */
    status_return(plain_write_a7str(p_ff_op_format, "}" RTF_LINESEP_STR));

    /* Main document block */
    status_return(plain_write_a7str(p_ff_op_format, "}" RTF_LINESEP_STR));

    al_array_dispose(&h_colour_table_save);
    al_array_dispose(&h_stylesheet_save);
    style_dispose(&style_current);
    style_dispose(&style_previous);

    return(status);
}

/******************************************************************************
*
* RTF converter object event handler
*
******************************************************************************/

T5_MSG_PROTO(static, rtf_msg_initclose, _InRef_ PC_MSG_INITCLOSE p_msg_initclose)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__STARTUP:
        return(resource_init(OBJECT_ID_FS_RTF, P_BOUND_MESSAGES_OBJECT_ID_FS_RTF, P_BOUND_RESOURCES_OBJECT_ID_FS_RTF));

    case T5_MSG_IC__EXIT1:
        return(resource_close(OBJECT_ID_FS_RTF));

    default:
        return(STATUS_OK);
    }
}

OBJECT_PROTO(extern, object_fs_rtf);
OBJECT_PROTO(extern, object_fs_rtf)
{
    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        return(rtf_msg_initclose(p_docu, t5_message, (PC_MSG_INITCLOSE) p_data));

    case T5_MSG_SAVE_FOREIGN:
        return(rtf_msg_save_foreign(p_docu, t5_message, (P_MSG_SAVE_FOREIGN) p_data));

    default:
        return(STATUS_OK);
    }
}

/* end of fs_rtf.c */
