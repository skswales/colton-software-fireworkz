/* sk_stylc.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Style conversion routines */

/* MRJC November 1992 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

/*
internal routines
*/

_Check_return_
static STATUS
style_ruler_measurement(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     STATUS message,
    _InVal_     PIXIT param,
    _InVal_     BOOL horizontal,
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/);

#define is_style_inline(id) ( \
    ((id) >= IL_STYLE_NAME) && ((id) < IL_STYLE_NAME + elemof32(style_encoding_index)) )

#define style_bit_number_from_inline_id(id) ( \
    (STYLE_BIT_NUMBER) style_encoding_index[(id - IL_STYLE_NAME)] )

typedef const struct STYLE_ENCODING * PC_STYLE_ENCODING;

typedef BOOL /*FALSE->eq;TRUE->neq*/ (* P_PROC_STYLE_COMPARE_NEQ) (
    _InRef_     PC_STYLE p_style_1,
    _InRef_     PC_STYLE p_style_2,
    _InRef_     PC_STYLE_ENCODING p_style_encoding);

#define PROC_STYLE_COMPARE_NEQ_PROTO(_proc_name) \
_Check_return_ \
extern BOOL /*FALSE->eq;TRUE->neq*/ \
_proc_name( \
    _InRef_     PC_STYLE p_style_1, \
    _InRef_     PC_STYLE p_style_2, \
    _InRef_     PC_STYLE_ENCODING p_style_encoding)

PROC_STYLE_COMPARE_NEQ_PROTO(proc_style_ps_tab_list_compare_neq);
PROC_STYLE_COMPARE_NEQ_PROTO(proc_style_colour_compare_neq);
PROC_STYLE_COMPARE_NEQ_PROTO(proc_style_line_space_compare_neq);
PROC_STYLE_COMPARE_NEQ_PROTO(proc_style_tstr_compare_neq);
PROC_STYLE_COMPARE_NEQ_PROTO(proc_style_ustr_compare_neq);

typedef STATUS (* P_PROC_STYLE_DUPLICATE) (
    _InoutRef_  P_STYLE p_style_out,
    _InRef_     PC_STYLE p_style_in,
    _InRef_     PC_STYLE_ENCODING p_style_encoding);

#define PROC_STYLE_DUPLICATE_PROTO(_proc_name) \
_Check_return_ \
extern STATUS \
_proc_name( \
    _InoutRef_  P_STYLE p_style_out, \
    _InRef_     PC_STYLE p_style_in, \
    _InRef_     PC_STYLE_ENCODING p_style_encoding)

PROC_STYLE_DUPLICATE_PROTO(proc_style_array_duplicate);

typedef STATUS (* P_PROC_STYLE_DATA_FROM_INLINE) (
    /*_Inout_*/ const P_DOCU p_docu,
    _InoutRef_  P_STYLE p_style,
    _InRef_     PC_STYLE_ENCODING p_style_encoding,
    _In_reads_c_(INLINE_OVH) const PC_USTR_INLINE ustr_inline);

#define PROC_STYLE_DATA_FROM_INLINE_PROTO(_proc_name) \
_Check_return_ \
extern STATUS \
_proc_name( \
    /*_Inout_*/ const P_DOCU p_docu, \
    _InoutRef_  P_STYLE p_style, \
    _InRef_     PC_STYLE_ENCODING p_style_encoding, \
    _In_reads_c_(INLINE_OVH) const PC_USTR_INLINE ustr_inline)

PROC_STYLE_DATA_FROM_INLINE_PROTO(proc_style_ps_tab_list_data_from_inline);
PROC_STYLE_DATA_FROM_INLINE_PROTO(proc_style_numform_data_from_inline);
PROC_STYLE_DATA_FROM_INLINE_PROTO(proc_style_app_font_name_data_from_inline);
PROC_STYLE_DATA_FROM_INLINE_PROTO(proc_style_tstr_data_from_inline);

typedef STATUS (* P_PROC_STYLE_INLINE_FROM_DATA) (
    /*_Inout_*/ const P_DOCU p_docu,
    _InRef_     PC_STYLE p_style,
    _InRef_     PC_STYLE_ENCODING p_style_encoding,
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/);

#define PROC_STYLE_INLINE_FROM_DATA_PROTO(_proc_name) \
_Check_return_ \
extern STATUS \
_proc_name( \
    _DocuRef_   P_DOCU p_docu, \
    _InRef_     PC_STYLE p_style, \
    _InRef_     PC_STYLE_ENCODING p_style_encoding, \
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/)

PROC_STYLE_INLINE_FROM_DATA_PROTO(proc_style_ps_tab_list_inline_from_data);
PROC_STYLE_INLINE_FROM_DATA_PROTO(proc_style_numform_inline_from_data);
PROC_STYLE_INLINE_FROM_DATA_PROTO(proc_style_tstr_inline_from_data);

typedef STATUS (* P_PROC_STYLE_TEXT) (
    /*_Inout_*/ const P_DOCU p_docu,
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _InRef_     PC_ANY p_member);

#define PROC_STYLE_TEXT_PROTO(_proc_name) \
_Check_return_ \
extern STATUS \
_proc_name( \
    /*_Inout_*/ const P_DOCU p_docu, \
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/, \
    _InRef_     PC_ANY p_member)

PROC_STYLE_TEXT_PROTO(proc_style_text_font_name);
PROC_STYLE_TEXT_PROTO(proc_style_text_justify);
PROC_STYLE_TEXT_PROTO(proc_style_text_justify_v);
PROC_STYLE_TEXT_PROTO(proc_style_text_none);
PROC_STYLE_TEXT_PROTO(proc_style_text_on_off);
PROC_STYLE_TEXT_PROTO(proc_style_text_points);
PROC_STYLE_TEXT_PROTO(proc_style_text_ruler_x);
PROC_STYLE_TEXT_PROTO(proc_style_text_ruler_y);

/*
style encoding tables

these two tables must be in order of STYLE_SW bit number
*/

typedef struct STYLE_ENCODING
{
    U8 inline_data_type;
    U8 inline_id;
    U8 _spare[2];
#define S_E_T_BYTES(inline_data_type, inline_id) \
    inline_data_type, inline_id, { 0, 0 }

    U32 member_size; /* SKS 13.02.2102 was U16 but ARM half-word load slow */
    U32 member_offset;

    P_PROC_STYLE_DATA_FROM_INLINE p_proc_style_data_from_inline;
    P_PROC_STYLE_INLINE_FROM_DATA p_proc_style_inline_from_data;
    P_PROC_STYLE_COMPARE_NEQ p_proc_style_compare_neq;
    P_PROC_STYLE_DUPLICATE p_proc_style_duplicate;
    P_PROC_STYLE_TEXT p_proc_style_text;
}
STYLE_ENCODING;

static const STYLE_ENCODING
style_encoding_table[STYLE_SW_COUNT] =
{
    { S_E_T_BYTES(IL_TYPE_USTR,   IL_STYLE_NAME),
      sizeofmemb32(STYLE, h_style_name_tstr),           offsetof32(STYLE, h_style_name_tstr),
      proc_style_tstr_data_from_inline, proc_style_tstr_inline_from_data, proc_style_tstr_compare_neq, proc_style_array_duplicate, NULL },
    { S_E_T_BYTES(IL_TYPE_S32,    IL_STYLE_KEY),
      sizeofmemb32(STYLE, style_key),                   offsetof32(STYLE, style_key),
      NULL, NULL, NULL, NULL, proc_style_text_none },
    { S_E_T_BYTES(IL_TYPE_S32,    IL_STYLE_HANDLE),
      sizeofmemb32(STYLE, style_handle),                offsetof32(STYLE, style_handle),
      NULL, NULL, NULL, NULL, NULL },
    { S_E_T_BYTES(IL_TYPE_S32,    IL_STYLE_SEARCH),
      sizeofmemb32(STYLE, search),                      offsetof32(STYLE, search),
      NULL, NULL, NULL, NULL, proc_style_text_none },

    { S_E_T_BYTES(IL_TYPE_S32,    IL_STYLE_CS_WIDTH),
      sizeofmemb32(STYLE, col_style.width),             offsetof32(STYLE, col_style) + offsetof32(COL_STYLE, width),
      NULL, NULL, NULL, NULL, proc_style_text_ruler_x },
    { S_E_T_BYTES(IL_TYPE_USTR,   IL_STYLE_CS_COL_NAME),
      sizeofmemb32(STYLE, col_style.h_numform),         offsetof32(STYLE, col_style) + offsetof32(COL_STYLE, h_numform),
      proc_style_numform_data_from_inline, proc_style_numform_inline_from_data, proc_style_ustr_compare_neq, proc_style_array_duplicate, NULL },

    { S_E_T_BYTES(IL_TYPE_S32,    IL_STYLE_RS_HEIGHT),
      sizeofmemb32(STYLE, row_style.height),            offsetof32(STYLE, row_style) + offsetof32(ROW_STYLE, height),
      NULL, NULL, NULL, NULL, proc_style_text_ruler_y },
    { S_E_T_BYTES(IL_TYPE_U8,     IL_STYLE_RS_HEIGHT_FIXED),
      sizeofmemb32(STYLE, row_style.height_fixed),      offsetof32(STYLE, row_style) + offsetof32(ROW_STYLE, height_fixed),
      NULL, NULL, NULL, NULL, proc_style_text_on_off },
    { S_E_T_BYTES(IL_TYPE_U8,     IL_STYLE_RS_UNBREAKABLE),
      sizeofmemb32(STYLE, row_style.unbreakable),       offsetof32(STYLE, row_style) + offsetof32(ROW_STYLE, unbreakable),
      NULL, NULL, NULL, NULL, proc_style_text_on_off },
    { S_E_T_BYTES(IL_TYPE_USTR,   IL_STYLE_RS_ROW_NAME),
      sizeofmemb32(STYLE, row_style.h_numform),         offsetof32(STYLE, row_style) + offsetof32(ROW_STYLE, h_numform),
      proc_style_numform_data_from_inline, proc_style_numform_inline_from_data, proc_style_ustr_compare_neq, proc_style_array_duplicate, NULL },

    { S_E_T_BYTES(IL_TYPE_S32,    IL_STYLE_PS_MARGIN_LEFT),
      sizeofmemb32(STYLE, para_style.margin_left),      offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, margin_left),
      NULL, NULL, NULL, NULL, proc_style_text_ruler_x },
    { S_E_T_BYTES(IL_TYPE_S32,    IL_STYLE_PS_MARGIN_RIGHT),
      sizeofmemb32(STYLE, para_style.margin_right),     offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, margin_right),
      NULL, NULL, NULL, NULL, proc_style_text_ruler_x },
    { S_E_T_BYTES(IL_TYPE_S32,    IL_STYLE_PS_MARGIN_PARA),
      sizeofmemb32(STYLE, para_style.margin_para),      offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, margin_para),
      NULL, NULL, NULL, NULL, proc_style_text_ruler_x },
    { S_E_T_BYTES(IL_TYPE_ANY,    IL_STYLE_PS_TAB_LIST),
      sizeofmemb32(STYLE, para_style.h_tab_list),       offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, h_tab_list),
      proc_style_ps_tab_list_data_from_inline, proc_style_ps_tab_list_inline_from_data, proc_style_ps_tab_list_compare_neq, proc_style_array_duplicate, proc_style_text_none },
    { S_E_T_BYTES(IL_TYPE_ANY,    IL_STYLE_PS_RGB_BACK),
      sizeofmemb32(STYLE, para_style.rgb_back),         offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, rgb_back),
      NULL, NULL, proc_style_colour_compare_neq, NULL, proc_style_text_none },
    { S_E_T_BYTES(IL_TYPE_S32,    IL_STYLE_PS_PARA_START),
      sizeofmemb32(STYLE, para_style.para_start),       offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, para_start),
      NULL, NULL, NULL, NULL, proc_style_text_ruler_y },
    { S_E_T_BYTES(IL_TYPE_S32,    IL_STYLE_PS_PARA_END),
      sizeofmemb32(STYLE, para_style.para_end),         offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, para_end),
      NULL, NULL, NULL, NULL, proc_style_text_ruler_y },
    { S_E_T_BYTES(IL_TYPE_ANY,    IL_STYLE_PS_LINE_SPACE),
      sizeofmemb32(STYLE, para_style.line_space),       offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, line_space),
      NULL, NULL, proc_style_line_space_compare_neq, NULL, proc_style_text_none },
    { S_E_T_BYTES(IL_TYPE_U8,     IL_STYLE_PS_JUSTIFY),
      sizeofmemb32(STYLE, para_style.justify),          offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, justify),
      NULL, NULL, NULL, NULL, proc_style_text_justify },
    { S_E_T_BYTES(IL_TYPE_U8,     IL_STYLE_PS_JUSTIFY_V),
      sizeofmemb32(STYLE, para_style.justify_v),        offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, justify_v),
      NULL, NULL, NULL, NULL, proc_style_text_justify_v },
    { S_E_T_BYTES(IL_TYPE_U8,     IL_STYLE_PS_NEW_OBJECT),
      sizeofmemb32(STYLE, para_style.new_object),       offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, new_object),
      NULL, NULL, NULL, NULL, NULL },
    { S_E_T_BYTES(IL_TYPE_USTR,   IL_STYLE_PS_NUMFORM_NU),
      sizeofmemb32(STYLE, para_style.h_numform_nu),     offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, h_numform_nu),
      proc_style_numform_data_from_inline, proc_style_numform_inline_from_data, proc_style_ustr_compare_neq, proc_style_array_duplicate, NULL },
    { S_E_T_BYTES(IL_TYPE_USTR,   IL_STYLE_PS_NUMFORM_DT),
      sizeofmemb32(STYLE, para_style.h_numform_dt),     offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, h_numform_dt),
      proc_style_numform_data_from_inline, proc_style_numform_inline_from_data, proc_style_ustr_compare_neq, proc_style_array_duplicate, NULL },
    { S_E_T_BYTES(IL_TYPE_USTR,   IL_STYLE_PS_NUMFORM_SE),
      sizeofmemb32(STYLE, para_style.h_numform_se),     offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, h_numform_se),
      proc_style_numform_data_from_inline, proc_style_numform_inline_from_data, proc_style_ustr_compare_neq, proc_style_array_duplicate, NULL },
    { S_E_T_BYTES(IL_TYPE_ANY,    IL_STYLE_PS_RGB_BORDER),
      sizeofmemb32(STYLE, para_style.rgb_border),       offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, rgb_border),
      NULL, NULL, proc_style_colour_compare_neq, NULL, proc_style_text_none },
    { S_E_T_BYTES(IL_TYPE_U8,     IL_STYLE_PS_BORDER),
      sizeofmemb32(STYLE, para_style.border),           offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, border),
      NULL, NULL, NULL, NULL, proc_style_text_none },
    { S_E_T_BYTES(IL_TYPE_ANY,    IL_STYLE_PS_RGB_GRID_LEFT),
      sizeofmemb32(STYLE, para_style.rgb_grid_left),    offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, rgb_grid_left),
      NULL, NULL, proc_style_colour_compare_neq, NULL, proc_style_text_none },
    { S_E_T_BYTES(IL_TYPE_ANY,    IL_STYLE_PS_RGB_GRID_TOP),
      sizeofmemb32(STYLE, para_style.rgb_grid_top),     offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, rgb_grid_top),
      NULL, NULL, proc_style_colour_compare_neq, NULL, proc_style_text_none },
    { S_E_T_BYTES(IL_TYPE_ANY,    IL_STYLE_PS_RGB_GRID_RIGHT),
      sizeofmemb32(STYLE, para_style.rgb_grid_right),   offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, rgb_grid_right),
      NULL, NULL, proc_style_colour_compare_neq, NULL, proc_style_text_none },
    { S_E_T_BYTES(IL_TYPE_ANY,    IL_STYLE_PS_RGB_GRID_BOTTOM),
      sizeofmemb32(STYLE, para_style.rgb_grid_bottom),  offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, rgb_grid_bottom),
      NULL, NULL, proc_style_colour_compare_neq, NULL, proc_style_text_none },
    { S_E_T_BYTES(IL_TYPE_U8,     IL_STYLE_PS_GRID_LEFT),
      sizeofmemb32(STYLE, para_style.grid_left),        offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, grid_left),
      NULL, NULL, NULL, NULL, proc_style_text_none },
    { S_E_T_BYTES(IL_TYPE_U8,     IL_STYLE_PS_GRID_TOP),
      sizeofmemb32(STYLE, para_style.grid_top),         offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, grid_top),
      NULL, NULL, NULL, NULL, proc_style_text_none },
    { S_E_T_BYTES(IL_TYPE_U8,     IL_STYLE_PS_GRID_RIGHT),
      sizeofmemb32(STYLE, para_style.grid_right),       offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, grid_right),
      NULL, NULL, NULL, NULL, proc_style_text_none },
    { S_E_T_BYTES(IL_TYPE_U8,     IL_STYLE_PS_GRID_BOTTOM),
      sizeofmemb32(STYLE, para_style.grid_bottom),      offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, grid_bottom),
      NULL, NULL, NULL, NULL, proc_style_text_none },
    { S_E_T_BYTES(IL_TYPE_U8,     IL_STYLE_PS_PROTECT),
      sizeofmemb32(STYLE, para_style.protect),          offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, protect),
      NULL, NULL, NULL, NULL, proc_style_text_on_off },

    { S_E_T_BYTES(IL_TYPE_USTR,   IL_STYLE_FS_NAME),
      sizeofmemb32(STYLE, font_spec.h_app_name_tstr),   offsetof32(STYLE, font_spec) + offsetof32(FONT_SPEC, h_app_name_tstr),
      proc_style_app_font_name_data_from_inline, proc_style_tstr_inline_from_data, proc_style_tstr_compare_neq, proc_style_array_duplicate, proc_style_text_font_name },
    { S_E_T_BYTES(IL_TYPE_S32,    IL_STYLE_FS_SIZE_X),
      sizeofmemb32(STYLE, font_spec.size_x),            offsetof32(STYLE, font_spec) + offsetof32(FONT_SPEC, size_x),
      NULL, NULL, NULL, NULL, proc_style_text_points },
    { S_E_T_BYTES(IL_TYPE_S32,    IL_STYLE_FS_SIZE_Y),
      sizeofmemb32(STYLE, font_spec.size_y),            offsetof32(STYLE, font_spec) + offsetof32(FONT_SPEC, size_y),
      NULL, NULL, NULL, NULL, proc_style_text_points },
    { S_E_T_BYTES(IL_TYPE_U8,     IL_STYLE_FS_UNDERLINE),
      sizeofmemb32(STYLE, font_spec.underline),         offsetof32(STYLE, font_spec) + offsetof32(FONT_SPEC, underline),
      NULL, NULL, NULL, NULL, proc_style_text_on_off },
    { S_E_T_BYTES(IL_TYPE_U8,     IL_STYLE_FS_BOLD),
      sizeofmemb32(STYLE, font_spec.bold),              offsetof32(STYLE, font_spec) + offsetof32(FONT_SPEC, bold),
      NULL, NULL, NULL, NULL, proc_style_text_on_off },
    { S_E_T_BYTES(IL_TYPE_U8,     IL_STYLE_FS_ITALIC),
      sizeofmemb32(STYLE, font_spec.italic),            offsetof32(STYLE, font_spec) + offsetof32(FONT_SPEC, italic),
      NULL, NULL, NULL, NULL, proc_style_text_on_off },
    { S_E_T_BYTES(IL_TYPE_U8,     IL_STYLE_FS_SUPERSCRIPT),
      sizeofmemb32(STYLE, font_spec.superscript),       offsetof32(STYLE, font_spec) + offsetof32(FONT_SPEC, superscript),
      NULL, NULL, NULL, NULL, proc_style_text_on_off },
    { S_E_T_BYTES(IL_TYPE_U8,     IL_STYLE_FS_SUBSCRIPT),
      sizeofmemb32(STYLE, font_spec.subscript),         offsetof32(STYLE, font_spec) + offsetof32(FONT_SPEC, subscript),
      NULL, NULL, NULL, NULL, proc_style_text_on_off },
    { S_E_T_BYTES(IL_TYPE_ANY,    IL_STYLE_FS_COLOUR),
      sizeofmemb32(STYLE, font_spec.colour),            offsetof32(STYLE, font_spec) + offsetof32(FONT_SPEC, colour),
      NULL, NULL, proc_style_colour_compare_neq, NULL, proc_style_text_none },
};

/*extern*/ const STYLE_EXPORTED_INFO
style_exported_info_table[STYLE_SW_COUNT] =
{
    { sizeofmemb32(STYLE, h_style_name_tstr),           offsetof32(STYLE, h_style_name_tstr) },
    { sizeofmemb32(STYLE, style_key),                   offsetof32(STYLE, style_key) },
    { sizeofmemb32(STYLE, style_handle),                offsetof32(STYLE, style_handle) },
    { sizeofmemb32(STYLE, search),                      offsetof32(STYLE, search) },

    { sizeofmemb32(STYLE, col_style.width),             offsetof32(STYLE, col_style) + offsetof32(COL_STYLE, width) },
    { sizeofmemb32(STYLE, col_style.h_numform),         offsetof32(STYLE, col_style) + offsetof32(COL_STYLE, h_numform) },

    { sizeofmemb32(STYLE, row_style.height),            offsetof32(STYLE, row_style) + offsetof32(ROW_STYLE, height) },
    { sizeofmemb32(STYLE, row_style.height_fixed),      offsetof32(STYLE, row_style) + offsetof32(ROW_STYLE, height_fixed) },
    { sizeofmemb32(STYLE, row_style.unbreakable),       offsetof32(STYLE, row_style) + offsetof32(ROW_STYLE, unbreakable) },
    { sizeofmemb32(STYLE, row_style.h_numform),         offsetof32(STYLE, row_style) + offsetof32(ROW_STYLE, h_numform) },

    { sizeofmemb32(STYLE, para_style.margin_left),      offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, margin_left) },
    { sizeofmemb32(STYLE, para_style.margin_right),     offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, margin_right) },
    { sizeofmemb32(STYLE, para_style.margin_para),      offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, margin_para) },
    { sizeofmemb32(STYLE, para_style.h_tab_list),       offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, h_tab_list) },
    { sizeofmemb32(STYLE, para_style.rgb_back),         offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, rgb_back) },
    { sizeofmemb32(STYLE, para_style.para_start),       offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, para_start) },
    { sizeofmemb32(STYLE, para_style.para_end),         offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, para_end) },
    { sizeofmemb32(STYLE, para_style.line_space),       offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, line_space) },
    { sizeofmemb32(STYLE, para_style.justify),          offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, justify) },
    { sizeofmemb32(STYLE, para_style.justify_v),        offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, justify_v) },
    { sizeofmemb32(STYLE, para_style.new_object),       offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, new_object) },
    { sizeofmemb32(STYLE, para_style.h_numform_nu),     offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, h_numform_nu) },
    { sizeofmemb32(STYLE, para_style.h_numform_dt),     offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, h_numform_dt) },
    { sizeofmemb32(STYLE, para_style.h_numform_se),     offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, h_numform_se) },
    { sizeofmemb32(STYLE, para_style.rgb_border),       offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, rgb_border) },
    { sizeofmemb32(STYLE, para_style.border),           offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, border) },
    { sizeofmemb32(STYLE, para_style.rgb_grid_left),    offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, rgb_grid_left) },
    { sizeofmemb32(STYLE, para_style.rgb_grid_top),     offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, rgb_grid_top) },
    { sizeofmemb32(STYLE, para_style.rgb_grid_right),   offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, rgb_grid_right) },
    { sizeofmemb32(STYLE, para_style.rgb_grid_bottom),  offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, rgb_grid_bottom) },
    { sizeofmemb32(STYLE, para_style.grid_left),        offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, grid_left) },
    { sizeofmemb32(STYLE, para_style.grid_top),         offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, grid_top) },
    { sizeofmemb32(STYLE, para_style.grid_right),       offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, grid_right) },
    { sizeofmemb32(STYLE, para_style.grid_bottom),      offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, grid_bottom) },
    { sizeofmemb32(STYLE, para_style.protect),          offsetof32(STYLE, para_style) + offsetof32(PARA_STYLE, protect) },

    { sizeofmemb32(STYLE, font_spec.h_app_name_tstr),   offsetof32(STYLE, font_spec) + offsetof32(FONT_SPEC, h_app_name_tstr) },
    { sizeofmemb32(STYLE, font_spec.size_x),            offsetof32(STYLE, font_spec) + offsetof32(FONT_SPEC, size_x) },
    { sizeofmemb32(STYLE, font_spec.size_y),            offsetof32(STYLE, font_spec) + offsetof32(FONT_SPEC, size_y) },
    { sizeofmemb32(STYLE, font_spec.underline),         offsetof32(STYLE, font_spec) + offsetof32(FONT_SPEC, underline) },
    { sizeofmemb32(STYLE, font_spec.bold),              offsetof32(STYLE, font_spec) + offsetof32(FONT_SPEC, bold) },
    { sizeofmemb32(STYLE, font_spec.italic),            offsetof32(STYLE, font_spec) + offsetof32(FONT_SPEC, italic) },
    { sizeofmemb32(STYLE, font_spec.superscript),       offsetof32(STYLE, font_spec) + offsetof32(FONT_SPEC, superscript) },
    { sizeofmemb32(STYLE, font_spec.subscript),         offsetof32(STYLE, font_spec) + offsetof32(FONT_SPEC, subscript) },
    { sizeofmemb32(STYLE, font_spec.colour),            offsetof32(STYLE, font_spec) + offsetof32(FONT_SPEC, colour) }
};

/*
 * the ordering of this table must correspond to
 * the enum order of the style inline codes
 */

static const U8
style_encoding_index[] =
{
    STYLE_SW_NAME,                      /* IL_STYLE_NAME */
    STYLE_SW_KEY,                       /* IL_STYLE_KEY */
    STYLE_SW_HANDLE,                    /* IL_STYLE_HANDLE */
    STYLE_SW_SEARCH,                    /* IL_STYLE_SEARCH */

    STYLE_SW_CS_WIDTH,                  /* IL_STYLE_CS_WIDTH */
    STYLE_SW_CS_COL_NAME,               /* IL_STYLE_CS_COL_NAME */

    STYLE_SW_RS_HEIGHT,                 /* IL_STYLE_RS_HEIGHT */
    STYLE_SW_RS_HEIGHT_FIXED,           /* IL_STYLE_RS_HEIGHT_FIXED */
    STYLE_SW_RS_UNBREAKABLE,            /* IL_STYLE_RS_UNBREAKABLE */
    STYLE_SW_RS_ROW_NAME,               /* IL_STYLE_RS_ROW_NAME */

    STYLE_SW_PS_MARGIN_LEFT,            /* IL_STYLE_PS_MARGIN_LEFT */
    STYLE_SW_PS_MARGIN_RIGHT,           /* IL_STYLE_PS_MARGIN_RIGHT */
    STYLE_SW_PS_MARGIN_PARA,            /* IL_STYLE_PS_MARGIN_PARA */
    STYLE_SW_PS_TAB_LIST,               /* IL_STYLE_PS_TAB_LIST */
    STYLE_SW_PS_RGB_BACK,               /* IL_STYLE_PS_RGB_BACK */
    STYLE_SW_PS_PARA_START,             /* IL_STYLE_PS_PARA_START */
    STYLE_SW_PS_PARA_END,               /* IL_STYLE_PS_PARA_END */
    STYLE_SW_PS_LINE_SPACE,             /* IL_STYLE_PS_LINE_SPACE */
    STYLE_SW_PS_JUSTIFY,                /* IL_STYLE_PS_JUSTIFY */
    STYLE_SW_PS_JUSTIFY_V,              /* IL_STYLE_PS_JUSTIFY_V */
    STYLE_SW_PS_NEW_OBJECT,             /* IL_STYLE_PS_NEW_OBJECT */
    STYLE_SW_PS_NUMFORM_NU,             /* IL_STYLE_PS_NUMFORM_NU */
    STYLE_SW_PS_NUMFORM_DT,             /* IL_STYLE_PS_NUMFORM_DT */
    STYLE_SW_PS_NUMFORM_SE,             /* IL_STYLE_PS_NUMFORM_SE */
    STYLE_SW_PS_RGB_BORDER,             /* IL_STYLE_PS_RGB_BORDER */
    STYLE_SW_PS_BORDER,                 /* IL_STYLE_PS_BORDER */
    STYLE_SW_PS_RGB_GRID_LEFT,          /* IL_STYLE_PS_RGB_GRID_LEFT */
    STYLE_SW_PS_RGB_GRID_TOP,           /* IL_STYLE_PS_RGB_GRID_TOP */
    STYLE_SW_PS_RGB_GRID_RIGHT,         /* IL_STYLE_PS_RGB_GRID_RIGHT */
    STYLE_SW_PS_RGB_GRID_BOTTOM,        /* IL_STYLE_PS_RGB_GRID_BOTTOM */
    STYLE_SW_PS_GRID_LEFT,              /* IL_STYLE_PS_GRID_LEFT */
    STYLE_SW_PS_GRID_TOP,               /* IL_STYLE_PS_GRID_TOP */
    STYLE_SW_PS_GRID_RIGHT,             /* IL_STYLE_PS_GRID_RIGHT */
    STYLE_SW_PS_GRID_BOTTOM,            /* IL_STYLE_PS_GRID_BOTTOM */
    STYLE_SW_PS_PROTECT,                /* IL_STYLE_PS_PROTECT */

    STYLE_SW_FS_NAME,                   /* IL_STYLE_FS_NAME */
    STYLE_SW_FS_SIZE_X,                 /* IL_STYLE_FS_SIZE_X */
    STYLE_SW_FS_SIZE_Y,                 /* IL_STYLE_FS_SIZE_Y */
    STYLE_SW_FS_UNDERLINE,              /* IL_STYLE_FS_UNDERLINE */
    STYLE_SW_FS_BOLD,                   /* IL_STYLE_FS_BOLD */
    STYLE_SW_FS_ITALIC,                 /* IL_STYLE_FS_ITALIC */
    STYLE_SW_FS_SUPERSCRIPT,            /* IL_STYLE_FS_SUPERSCRIPT */
    STYLE_SW_FS_SUBSCRIPT,              /* IL_STYLE_FS_SUBSCRIPT */
    STYLE_SW_FS_COLOUR                  /* IL_STYLE_FS_COLOUR */
};

/******************************************************************************
*
* style data <-> inline routines
*
******************************************************************************/

PROC_STYLE_DATA_FROM_INLINE_PROTO(proc_style_ps_tab_list_data_from_inline)
{
    STATUS status = STATUS_OK;
    const S32 il_data_size = inline_data_size(ustr_inline);
    const S32 list_elements = il_data_size / sizeof32(TAB_INFO);
    SC_ARRAY_INIT_BLOCK tab_init_block = aib_init(1, sizeof32(TAB_INFO), TRUE);
    P_TAB_INFO p_tab_info;

    UNREFERENCED_PARAMETER_CONST(p_docu);
    UNREFERENCED_PARAMETER_CONST(p_style_encoding);

    if(NULL != (p_tab_info = al_array_alloc(&p_style->para_style.h_tab_list, TAB_INFO, list_elements, &tab_init_block, &status)))
    {
        /* tab list may have zero elements */
        if(list_elements)
            memcpy32(p_tab_info,
                     inline_data_ptr(PC_TAB_INFO, ustr_inline),
                     list_elements * sizeof32(TAB_INFO));
    }

    return(status);
}

PROC_STYLE_INLINE_FROM_DATA_PROTO(proc_style_ps_tab_list_inline_from_data)
{
    STATUS status;

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    status = inline_quick_ublock_from_data(p_quick_ublock,
                                            p_style_encoding->inline_id,
                                            p_style_encoding->inline_data_type,
                                            p_style->para_style.h_tab_list ? array_ptr(&p_style->para_style.h_tab_list, TAB_INFO, 0) : NULL,
                                            array_elements32(&p_style->para_style.h_tab_list) * sizeof32(TAB_INFO));

    return(status);
}

PROC_STYLE_DATA_FROM_INLINE_PROTO(proc_style_numform_data_from_inline)
{
    STATUS status;
    P_ARRAY_HANDLE_USTR p_array_handle_ustr = PtrAddBytes(P_ARRAY_HANDLE_USTR, p_style, p_style_encoding->member_offset);

    UNREFERENCED_PARAMETER_CONST(p_docu);

    status = al_ustr_set(p_array_handle_ustr, inline_data_ptr(PC_USTR, ustr_inline));

    return(status);
}

PROC_STYLE_INLINE_FROM_DATA_PROTO(proc_style_numform_inline_from_data)
{
    STATUS status;
    PC_ARRAY_HANDLE_USTR p_array_handle_ustr = PtrAddBytes(PC_ARRAY_HANDLE_USTR, p_style, p_style_encoding->member_offset);

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    assert(IL_TYPE_USTR == p_style_encoding->inline_data_type);

    status = inline_quick_ublock_from_ustr(p_quick_ublock,
                                           ENUM_UNPACK(IL_CODE, p_style_encoding->inline_id),
                                           array_elements(p_array_handle_ustr) ? array_ustr(p_array_handle_ustr) : ustr_empty_string);

    return(status);
}

PROC_STYLE_DATA_FROM_INLINE_PROTO(proc_style_app_font_name_data_from_inline)
{
    STATUS status = STATUS_OK;
    P_ARRAY_HANDLE_TSTR p_array_handle_tstr = PtrAddBytes(P_ARRAY_HANDLE_TSTR, p_style, p_style_encoding->member_offset);
    PC_USTR ustr = inline_data_ptr(PC_USTR, ustr_inline);
    PCTSTR tstr = _tstr_from_ustr(ustr);

    UNREFERENCED_PARAMETER_CONST(p_docu);

    *p_array_handle_tstr = 0;

    if(0 != inline_data_size(ustr_inline))
        status = al_tstr_set(p_array_handle_tstr, fontmap_remap(tstr));

    return(status);
}

PROC_STYLE_DATA_FROM_INLINE_PROTO(proc_style_tstr_data_from_inline)
{
    STATUS status = STATUS_OK;
    P_ARRAY_HANDLE_TSTR p_array_handle_tstr = PtrAddBytes(P_ARRAY_HANDLE_TSTR, p_style, p_style_encoding->member_offset);
    PC_USTR ustr = inline_data_ptr(PC_USTR, ustr_inline);
    PCTSTR tstr = _tstr_from_ustr(ustr);

    UNREFERENCED_PARAMETER_CONST(p_docu);

    *p_array_handle_tstr = 0;

    if(0 != inline_data_size(ustr_inline))
        status = al_tstr_set(p_array_handle_tstr, tstr);

    return(status);
}

PROC_STYLE_INLINE_FROM_DATA_PROTO(proc_style_tstr_inline_from_data)
{
    STATUS status;
    PC_ARRAY_HANDLE_TSTR p_array_handle_tstr = PtrAddBytes(PC_ARRAY_HANDLE_TSTR, p_style, p_style_encoding->member_offset);
    PC_USTR ustr = array_elements(p_array_handle_tstr) ? _ustr_from_tstr(array_tstr(p_array_handle_tstr)) : ustr_empty_string;

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    assert(IL_TYPE_USTR == p_style_encoding->inline_data_type);

    status = inline_quick_ublock_from_ustr(p_quick_ublock,
                                           ENUM_UNPACK(IL_CODE, p_style_encoding->inline_id),
                                           ustr);

    return(status);
}

/******************************************************************************
*
* style compare routines
*
******************************************************************************/

PROC_STYLE_COMPARE_NEQ_PROTO(proc_style_ps_tab_list_compare_neq)
{
    UNREFERENCED_PARAMETER_InRef_(p_style_encoding);

    if(p_style_1->para_style.h_tab_list == p_style_2->para_style.h_tab_list)
        return(FALSE);

    if(array_elements(&p_style_1->para_style.h_tab_list) != array_elements(&p_style_2->para_style.h_tab_list))
        return(TRUE);

    {
    P_TAB_INFO p_tab_info_1 = array_ptr(&p_style_1->para_style.h_tab_list, TAB_INFO, 0);
    P_TAB_INFO p_tab_info_2 = array_ptr(&p_style_2->para_style.h_tab_list, TAB_INFO, 0);
    ARRAY_INDEX i;

    for(i = 0; i < array_elements(&p_style_1->para_style.h_tab_list); i += 1, p_tab_info_1 += 1, p_tab_info_2 += 1)
        if( (p_tab_info_1->type   != p_tab_info_2->type  ) ||
            (p_tab_info_1->offset != p_tab_info_2->offset) )
            return(TRUE);

    return(FALSE);
    } /*block*/
}

PROC_STYLE_COMPARE_NEQ_PROTO(proc_style_colour_compare_neq)
{
    PC_RGB p_rgb_1 = PtrAddBytes(PC_RGB, p_style_1, p_style_encoding->member_offset);
    PC_RGB p_rgb_2 = PtrAddBytes(PC_RGB, p_style_2, p_style_encoding->member_offset);

    return(!(p_rgb_1->r == p_rgb_2->r &&
             p_rgb_1->g == p_rgb_2->g &&
             p_rgb_1->b == p_rgb_2->b &&
             p_rgb_1->transparent == p_rgb_2->transparent));
}

PROC_STYLE_COMPARE_NEQ_PROTO(proc_style_line_space_compare_neq)
{
    UNREFERENCED_PARAMETER_InRef_(p_style_encoding);

    return(!(p_style_1->para_style.line_space.type    == p_style_2->para_style.line_space.type &&
             p_style_1->para_style.line_space.leading == p_style_2->para_style.line_space.leading));
}

PROC_STYLE_COMPARE_NEQ_PROTO(proc_style_tstr_compare_neq)
{
    PC_ARRAY_HANDLE_TSTR p_array_handle_tstr_1 = PtrAddBytes(PC_ARRAY_HANDLE_TSTR, p_style_1, p_style_encoding->member_offset);
    PC_ARRAY_HANDLE_TSTR p_array_handle_tstr_2 = PtrAddBytes(PC_ARRAY_HANDLE_TSTR, p_style_2, p_style_encoding->member_offset);

    if(*p_array_handle_tstr_1 == *p_array_handle_tstr_2)
        return(FALSE);

    if((0 == *p_array_handle_tstr_1) || (0 == *p_array_handle_tstr_2))
        return(TRUE);

    return(!tstr_compare_equals(array_tstr(p_array_handle_tstr_1), array_tstr(p_array_handle_tstr_2)));
}

PROC_STYLE_COMPARE_NEQ_PROTO(proc_style_ustr_compare_neq)
{
    PC_ARRAY_HANDLE_USTR p_array_handle_ustr_1 = PtrAddBytes(PC_ARRAY_HANDLE_USTR, p_style_1, p_style_encoding->member_offset);
    PC_ARRAY_HANDLE_USTR p_array_handle_ustr_2 = PtrAddBytes(PC_ARRAY_HANDLE_USTR, p_style_2, p_style_encoding->member_offset);

    if(*p_array_handle_ustr_1 == *p_array_handle_ustr_2)
        return(FALSE);

    if((0 == *p_array_handle_ustr_1) || (0 == *p_array_handle_ustr_2))
        return(TRUE);

    return(!ustr_compare_equals(array_ustr(p_array_handle_ustr_1), array_ustr(p_array_handle_ustr_2)));
}

/******************************************************************************
*
* routine to compare two styles
*
******************************************************************************/

_Check_return_
extern BOOL /*any diffs?*/
style_compare(
    _OutRef_    P_STYLE_SELECTOR p_style_selector /* bit set for different fields */,
    _InRef_     PC_STYLE p_style_1,
    _InRef_     PC_STYLE p_style_2)
{
    STYLE_SELECTOR temp;
    STYLE_BIT_NUMBER style_bit_number;

    /* first, get differences in supplied fields */
    void_style_selector_xor(p_style_selector, &p_style_1->selector, &p_style_2->selector);

    /* get all supplied fields */
    void_style_selector_or(&temp, &p_style_1->selector, &p_style_2->selector);

    /* clear out differences, leaving fields to compare */
    void_style_selector_xor(&temp, &temp, p_style_selector);

    for(style_bit_number = STYLE_BIT_NUMBER_FIRST;
        (style_bit_number = style_selector_next_bit(&temp, style_bit_number)) >= 0;
        STYLE_BIT_NUMBER_INCR(style_bit_number))
    {
        const PC_STYLE_ENCODING p_style_encoding = &style_encoding_table[style_bit_number];
        BOOL neq = FALSE;

        if(p_style_encoding->p_proc_style_compare_neq)
            neq = (* p_style_encoding->p_proc_style_compare_neq) (p_style_1, p_style_2, p_style_encoding);
        else
        {
            const PC_BYTE p_data_1 = PtrAddBytes(PC_BYTE, p_style_1, p_style_encoding->member_offset);
            const PC_BYTE p_data_2 = PtrAddBytes(PC_BYTE, p_style_2, p_style_encoding->member_offset);

            switch(p_style_encoding->inline_data_type)
            {
            case IL_TYPE_USTR:
                neq = (0 != C_stricmp((PC_U8Z) p_data_1, (PC_U8Z) p_data_2));
                break;

            case IL_TYPE_U8:
                neq = (*((PC_U8) p_data_1) != *((PC_U8) p_data_2));
                break;

            case IL_TYPE_S32:
                neq = (*((PC_S32) p_data_1) != *((PC_S32) p_data_2));
                break;

            case IL_TYPE_F64:
                neq = (*((PC_F64) p_data_1) != *((PC_F64) p_data_2));
                break;

#if CHECKING
            /* these are too hard */
            case IL_TYPE_NONE:
            case IL_TYPE_ANY:
#endif
            default: default_unhandled();
                break;
            }
        }

        if(neq)
            style_selector_bit_set(p_style_selector, style_bit_number);
    }

    return(style_selector_any(p_style_selector));
}

/******************************************************************************
*
* copy a style - this copies selected parts of a style into another style record;
* indirected resources are not duplicated, so the 'owner' of the resources does not change
* see also style_duplicate
*
******************************************************************************/

#if defined(FAST_BITMAP_OPS_NN) && 0

/* another sicko speedup */

extern void
style_copy(
    _InoutRef_  P_STYLE p_style_out,
    _InRef_     PC_STYLE p_style_in,
    _InRef_     PC_STYLE_SELECTOR p_style_selector)
{
    STYLE_SELECTOR temp;
    STYLE_BIT_NUMBER style_bit_number;

#if CHECKING
    { /* check the tables over */
    static BOOL checked = FALSE;
    if(!checked)
    {
        checked = TRUE;
        for(style_bit_number = STYLE_BIT_NUMBER_FIRST;
            style_bit_number < STYLE_SW_COUNT;
            STYLE_BIT_NUMBER_INCR(style_bit_number))
        {
            const PC_STYLE_EXPORTED_INFO p_style_info = &style_exported_info_table[style_bit_number];
            const PC_STYLE_ENCODING p_style_encoding = &style_encoding_table[style_bit_number];
            assert(p_style_info->member_offset == p_style_encoding->member_offset);
            assert(p_style_info->member_size == p_style_encoding->member_size);
            assert(style_bit_number_from_inline_id(p_style_encoding->inline_id) == style_bit_number);
        }
    }
    } /*block*/
#endif

    /* which bits should we be interested in from this style? */
    void_style_selector_and(&temp, &p_style_in->selector, p_style_selector);

    /* step through selected bits */
    for(style_bit_number = STYLE_BIT_NUMBER_FIRST;
        (style_bit_number = style_selector_next_bit(&temp, style_bit_number)) >= 0;
        STYLE_BIT_NUMBER_INCR(style_bit_number))
    {
        const PC_STYLE_EXPORTED_INFO p_style_info = &style_exported_info_table[style_bit_number];
        U32 member_size = p_style_info->member_size;
        const U32 member_offset = p_style_info->member_offset;
        PC_BYTE p_member_read  = PtrAddBytes(PC_BYTE, p_style_in,  member_offset);
        P_BYTE  p_member_write = PtrAddBytes(P_BYTE,  p_style_out, member_offset);

        assert(0 != member_size);

        /* turn on only when being utterly paranoid. Doesn't matter on x86 etc. */
#if RISCOS && CHECKING && 1
        if(sizeof32(U32) == member_size)
        {
            assert(0 == ((uintptr_t) (p_member_read ) & (member_size - 1)));
            assert(0 == ((uintptr_t) (p_member_write) & (member_size - 1)));
        }
#endif

        if(sizeof32(U32) == member_size) /* SKS 21apr95 avoid procedure call overhead */
            * (P_U32) p_member_write = * (PC_U32) p_member_read;
        else if(sizeof32(U8) == member_size)
            * (P_U8)  p_member_write = * (PC_U8)  p_member_read;
        else
            memcpy32(p_member_write, p_member_read, member_size);
    }

    /* *add* copied bits to selector */
    void_style_selector_or(&p_style_out->selector, &p_style_out->selector, &temp);
}

#else

extern void
style_copy(
    _InoutRef_  P_STYLE p_style_out,
    _InRef_     PC_STYLE p_style_in,
    _InRef_     PC_STYLE_SELECTOR p_style_selector)
{
    STYLE_BIT_NUMBER style_bit_number;

#if CHECKING
    { /* check the tables over */
    static BOOL checked = FALSE;
    if(!checked)
    {
        checked = TRUE;
        for(style_bit_number = STYLE_BIT_NUMBER_FIRST;
            style_bit_number < STYLE_SW_COUNT;
            STYLE_BIT_NUMBER_INCR(style_bit_number))
        {
            const PC_STYLE_EXPORTED_INFO p_style_info = &style_exported_info_table[style_bit_number];
            const PC_STYLE_ENCODING p_style_encoding = &style_encoding_table[style_bit_number];
            assert(p_style_info->member_offset == p_style_encoding->member_offset);
            assert(p_style_info->member_size == p_style_encoding->member_size);
            assert(style_bit_number_from_inline_id(p_style_encoding->inline_id) == style_bit_number);
        }
    }
    } /*block*/
#endif

    /* step through selected bits */
    for(style_bit_number = STYLE_BIT_NUMBER_FIRST;
        (style_bit_number = style_selector_next_bit_in_both(&p_style_in->selector, p_style_selector, style_bit_number)) >= 0;
        STYLE_BIT_NUMBER_INCR(style_bit_number))
    {
        const PC_STYLE_ENCODING p_style_encoding = &style_encoding_table[style_bit_number];
        const U32 member_size = p_style_encoding->member_size;
        PC_BYTE p_member_read  = PtrAddBytes(PC_BYTE, p_style_in,  p_style_encoding->member_offset);
        P_BYTE  p_member_write = PtrAddBytes(P_BYTE,  p_style_out, p_style_encoding->member_offset);

        assert(0 != member_size);

        /* turn on only when being utterly paranoid. Doesn't matter on x86 etc. */
#if RISCOS && CHECKING && 1
        if(sizeof32(U32) == member_size)
        {
            assert(0 == ((uintptr_t) (p_member_read ) & (member_size - 1)));
            assert(0 == ((uintptr_t) (p_member_write) & (member_size - 1)));
        }
#endif

        if(sizeof32(U32) == member_size) /* SKS 21apr95 avoid procedure call overhead */
            * (P_U32) p_member_write = * (PC_U32) p_member_read;
        else if(sizeof32(U8) == member_size)
            * (P_U8)  p_member_write = * (PC_U8)  p_member_read;
        else
            memcpy32(p_member_write, p_member_read, member_size);

        style_selector_bit_set(&p_style_out->selector, style_bit_number);
    }
}

#endif

/******************************************************************************
*
* duplicate routines
*
******************************************************************************/

PROC_STYLE_DUPLICATE_PROTO(proc_style_array_duplicate)
{
    P_ARRAY_HANDLE p_array_handle_out = PtrAddBytes(P_ARRAY_HANDLE, p_style_out, p_style_encoding->member_offset);
    PC_ARRAY_HANDLE p_array_handle_in = PtrAddBytes(PC_ARRAY_HANDLE, p_style_in, p_style_encoding->member_offset);
    return(al_array_duplicate(p_array_handle_out, p_array_handle_in));
}

/******************************************************************************
*
* duplicate a style - this makes another copy of indirected
* resources owned by the input style in a style record
* remember to dispose of it when finished
*
******************************************************************************/

_Check_return_
extern STATUS
style_duplicate(
    _InoutRef_  P_STYLE p_style_out,
    _InRef_     PC_STYLE p_style_in,
    _InRef_     PC_STYLE_SELECTOR p_style_selector)
{
    STATUS status = STATUS_OK;
    STYLE_BIT_NUMBER style_bit_number;

    /* step through selected bits */
    for(style_bit_number = STYLE_BIT_NUMBER_FIRST;
        (style_bit_number = style_selector_next_bit_in_both(&p_style_in->selector, p_style_selector, style_bit_number)) >= 0;
        STYLE_BIT_NUMBER_INCR(style_bit_number))
    {
        const PC_STYLE_ENCODING p_style_encoding = &style_encoding_table[style_bit_number];

        if(p_style_encoding->p_proc_style_duplicate)
        {
            status_break(status = (* p_style_encoding->p_proc_style_duplicate) (p_style_out, p_style_in, p_style_encoding));
        }
        else
        {
            const U32 member_size = p_style_encoding->member_size;
            PC_BYTE p_member_read  = PtrAddBytes(PC_BYTE, p_style_in,  p_style_encoding->member_offset);
            P_BYTE  p_member_write = PtrAddBytes(P_BYTE,  p_style_out, p_style_encoding->member_offset);

            assert(0 != member_size);

            /* turn on only when being utterly paranoid. Doesn't matter on x86 etc. */
#if RISCOS && CHECKING && 1
            if(sizeof32(U32) == member_size)
            {
                assert(0 == ((uintptr_t) (p_member_read ) & (member_size - 1)));
                assert(0 == ((uintptr_t) (p_member_write) & (member_size - 1)));
            }
#endif

            if(sizeof32(U32) == member_size) /* SKS 21apr95 avoid procedure call overhead */
                * (P_U32) p_member_write = * (PC_U32) p_member_read;
            else if(sizeof32(U8) == member_size)
                * (P_U8)  p_member_write = * (PC_U8)  p_member_read;
            else
                memcpy32(p_member_write, p_member_read, member_size);
        }

        style_selector_bit_set(&p_style_out->selector, style_bit_number);
    }

    if(status_fail(status))
        style_dispose(p_style_out);

    return(status);
}

/******************************************************************************
*
* given a style structure, convert to an ustr_inline string
*
* --out--
* size of resulting inline string
*
******************************************************************************/

_Check_return_
extern STATUS /* size out */
style_ustr_inline_from_struct(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended,terminated*/,
    _InRef_     PC_STYLE p_style)
{
    STATUS status = STATUS_OK;
    STYLE_BIT_NUMBER style_bit_number;

    for(style_bit_number = STYLE_BIT_NUMBER_FIRST;
        (style_bit_number = style_selector_next_bit(&p_style->selector, style_bit_number)) >= 0;
        STYLE_BIT_NUMBER_INCR(style_bit_number))
    {
        const PC_STYLE_ENCODING p_style_encoding = &style_encoding_table[style_bit_number];

        if(NULL != p_style_encoding->p_proc_style_inline_from_data)
            status = (*p_style_encoding->p_proc_style_inline_from_data) (p_docu, p_style, p_style_encoding, p_quick_ublock);
        else
            status = inline_quick_ublock_from_data(p_quick_ublock,
                                                   p_style_encoding->inline_id,
                                                   p_style_encoding->inline_data_type,
                                                   PtrAddBytes(PC_BYTE, p_style, p_style_encoding->member_offset),
                                                   p_style_encoding->member_size);

        status_break(status);
    }

    /* always append a CH_NULL terminator */
    if(status_ok(status))
        status = quick_ublock_nullch_add(p_quick_ublock);

    if(status_fail(status))
        quick_ublock_dispose(p_quick_ublock);

    if(status_ok(status))
        status = quick_ublock_bytes(p_quick_ublock) - 1;

    return(status);
}

/******************************************************************************
*
* read inline style changes and poke them into a style structure
*
* selector in style structure updated for effects found
*
******************************************************************************/

_Check_return_
extern STATUS /* length processed */
style_struct_from_ustr_inline(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_STYLE p_style,
    _In_reads_c_(INLINE_OVH) PC_USTR_INLINE ustr_inline,
    _InRef_     PC_STYLE_SELECTOR p_style_selector /* effects wanted */)
{
    U32 offset = 0;
    STYLE_SELECTOR selector;

    style_selector_copy(&selector, p_style_selector);

    while(style_selector_any(&selector))
    {
        if(is_inline_off(ustr_inline, offset))
        {        
            const IL_CODE il_code = inline_code_off(ustr_inline, offset);

            if(is_style_inline(il_code))
            {
                const STYLE_BIT_NUMBER style_bit_number = style_bit_number_from_inline_id(il_code);

                if(style_selector_bit_test(&selector, style_bit_number))
                {
                    const PC_STYLE_ENCODING p_style_encoding = &style_encoding_table[style_bit_number];
                    STATUS status = STATUS_OK;

                    if(p_style_encoding->p_proc_style_data_from_inline)
                    {
                        status = (* p_style_encoding->p_proc_style_data_from_inline) (p_docu, p_style, p_style_encoding, ustr_inline_AddBytes(ustr_inline, offset));

                        if(status_fail(status))
                        {   /* discard anything we copied */
                            style_free_resources(p_style, p_style_selector);
                            return(status);
                        }
                    }
                    else
                        memcpy32(PtrAddBytes(P_BYTE, p_style, p_style_encoding->member_offset),
                                 inline_data_ptr_off(PC_BYTE, ustr_inline, offset),
                                 p_style_encoding->member_size);

                    style_bit_set(p_style, style_bit_number);

                    style_selector_bit_clear(&selector, style_bit_number);
                }
            }

            offset += inline_bytecount_off(ustr_inline, offset);
        }
        else
        {
            if(CH_NULL == PtrGetByteOff(ustr_inline, offset))
                break;

            assert0(); /* SKS - but ignore any stray chars */
            ++offset;
        }
    }

    return((S32) offset); /* bytes processed */
}

/******************************************************************************
*
* output font name after UI translation
*
******************************************************************************/

PROC_STYLE_TEXT_PROTO(proc_style_text_font_name)
{
    PC_ARRAY_HANDLE p_array_handle = (PC_ARRAY_HANDLE) p_member;
    STATUS status = STATUS_OK;

    UNREFERENCED_PARAMETER_CONST(p_docu);

    if(status_ok(status = quick_ublock_a7char_add(p_quick_ublock, CH_SPACE)))
    {
        PCTSTR tstr_app_font_name = array_tstr(p_array_handle);
        ARRAY_HANDLE_TSTR h_host_font_name_tstr;
        if(status_ok(status = fontmap_host_base_name_from_app_font_name(&h_host_font_name_tstr, tstr_app_font_name)))
            status = quick_ublock_tstr_add(p_quick_ublock, array_tstr(&h_host_font_name_tstr));
        al_array_dispose(&h_host_font_name_tstr);
    }

    return(status);
}

/******************************************************************************
*
* output justify argument
*
******************************************************************************/

PROC_STYLE_TEXT_PROTO(proc_style_text_justify)
{
    PC_U8 p_arg = (PC_U8) p_member;
    STATUS message_no;

    UNREFERENCED_PARAMETER_CONST(p_docu);

    switch(p_arg[0])
    {
    default: default_unhandled();
#if CHECKING
    case SF_JUSTIFY_LEFT:
#endif
        message_no = MSG_STYLE_TEXT_LEFT;
        break;

    case SF_JUSTIFY_CENTRE:
        message_no = MSG_STYLE_TEXT_CENTRE;
        break;

    case SF_JUSTIFY_RIGHT:
        message_no = MSG_STYLE_TEXT_RIGHT;
        break;

    case SF_JUSTIFY_BOTH:
        message_no = MSG_STYLE_TEXT_BOTH;
        break;
    }

    /* read text */
    return(resource_lookup_quick_ublock(p_quick_ublock, message_no));
}

/******************************************************************************
*
* output justify argument
*
******************************************************************************/

PROC_STYLE_TEXT_PROTO(proc_style_text_justify_v)
{
    PC_U8 p_arg = (P_U8) p_member;
    STATUS message_no;

    UNREFERENCED_PARAMETER_CONST(p_docu);

    switch(p_arg[0])
    {
    default: default_unhandled();
#if CHECKING
    case SF_JUSTIFY_V_TOP:
#endif
        message_no = MSG_STYLE_TEXT_TOP;
        break;
    case SF_JUSTIFY_V_CENTRE:
        message_no = MSG_STYLE_TEXT_CENTRE;
        break;
    case SF_JUSTIFY_V_BOTTOM:
        message_no = MSG_STYLE_TEXT_BOTTOM;
        break;
    }

    /* read text */
    return(resource_lookup_quick_ublock(p_quick_ublock, message_no));
}

/******************************************************************************
*
* no text arguments
*
******************************************************************************/

PROC_STYLE_TEXT_PROTO(proc_style_text_none)
{
    UNREFERENCED_PARAMETER_CONST(p_docu);
    UNREFERENCED_PARAMETER_InoutRef_(p_quick_ublock);
    UNREFERENCED_PARAMETER_InRef_(p_member);

    return(STATUS_OK);
}

/******************************************************************************
*
* on/off boolean
*
******************************************************************************/

PROC_STYLE_TEXT_PROTO(proc_style_text_on_off)
{
    PC_U8 p_arg = (PC_U8) p_member;

    UNREFERENCED_PARAMETER_CONST(p_docu);

    /* read text */
    return(resource_lookup_quick_ublock(p_quick_ublock, *p_arg ? MSG_STYLE_TEXT_ON : MSG_STYLE_TEXT_OFF));
}

/******************************************************************************
*
* output points type argument
*
******************************************************************************/

PROC_STYLE_TEXT_PROTO(proc_style_text_points)
{
    PC_PIXIT p_pixit = (PC_PIXIT) p_member;

    UNREFERENCED_PARAMETER_CONST(p_docu);

    return(quick_ublock_printf(p_quick_ublock, USTR_TEXT(" " S32_FMT), (S32) *p_pixit / PIXITS_PER_POINT));
}

/******************************************************************************
*
* output number in x ruler units
*
******************************************************************************/

PROC_STYLE_TEXT_PROTO(proc_style_text_ruler_x)
{
    PC_PIXIT p_pixit = (PC_PIXIT) p_member;

    return(style_ruler_measurement(p_docu, MSG_STYLE_TEXT_PERCENT_S, (S32) *p_pixit, TRUE, p_quick_ublock));
}

/******************************************************************************
*
* output number in y ruler units
*
******************************************************************************/

PROC_STYLE_TEXT_PROTO(proc_style_text_ruler_y)
{
    PC_PIXIT p_pixit = (PC_PIXIT) p_member;

    return(style_ruler_measurement(p_docu, MSG_STYLE_TEXT_PERCENT_S, (S32) *p_pixit, FALSE, p_quick_ublock));
}

_Check_return_
static STATUS
style_ruler_measurement(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     STATUS message,
    _InVal_     PIXIT value,
    _InVal_     BOOL horizontal,
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/)
{
    SCALE_INFO scale_info;
    DISPLAY_UNIT_INFO display_unit_info;
    SS_DATA ss_data;
    STATUS status;
    QUICK_UBLOCK_WITH_BUFFER(numform_res_quick_ublock, 16);
    quick_ublock_with_buffer_setup(numform_res_quick_ublock);

    scale_info_from_docu(p_docu, horizontal, &scale_info);

    display_unit_info_from_display_unit(&display_unit_info, scale_info.display_unit);

    ss_data_set_real(&ss_data, /*FP_USER_UNIT*/ ((FP_PIXIT) value / display_unit_info.fp_pixits_per_unit));

    {
    STATUS numform_resource_id;
    UCHARZ numform_unit_ustr_buf[elemof32("0.,####")];
    NUMFORM_PARMS numform_parms;
    zero_struct(numform_parms);

    switch(display_unit_info.unit_message)
    {
    default: default_unhandled();
#if CHECKING
    case MSG_DIALOG_UNITS_CM:
#endif
                                  numform_resource_id = MSG_NUMFORM_2_DP; break;
    case MSG_DIALOG_UNITS_MM:     numform_resource_id = MSG_NUMFORM_1_DP; break;
    case MSG_DIALOG_UNITS_INCHES: numform_resource_id = MSG_NUMFORM_3_DP; break;
    case MSG_DIALOG_UNITS_POINTS: numform_resource_id = MSG_NUMFORM_1_DP; break;
    }

    resource_lookup_ustr_buffer(ustr_bptr(numform_unit_ustr_buf), elemof32(numform_unit_ustr_buf), numform_resource_id);

    numform_parms.ustr_numform_numeric = ustr_bptr(numform_unit_ustr_buf);
    numform_parms.p_numform_context = get_p_numform_context(p_docu);

    status = numform(&numform_res_quick_ublock, P_QUICK_TBLOCK_NONE, &ss_data, &numform_parms);
    } /*block*/

    if(status_ok(status))
    {
        PC_USTR format = resource_lookup_ustr(message);

        status = quick_ublock_printf(p_quick_ublock, format, quick_ublock_ustr(&numform_res_quick_ublock));

        if(status_ok(status))
            status = quick_ublock_a7char_add(p_quick_ublock, CH_SPACE);

        if(status_ok(status))
            status = resource_lookup_quick_ublock(p_quick_ublock, display_unit_info.unit_message);
    }

    status_assertc(status);

    quick_ublock_dispose(&numform_res_quick_ublock);

    return(status);
}

/******************************************************************************
*
* convert style to descriptive text
*
******************************************************************************/

_Check_return_
extern STATUS
style_text_convert(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended,terminated*/,
    _InRef_     PC_STYLE p_style,
    _InRef_     PC_STYLE_SELECTOR p_style_selector)
{
    STATUS status = STATUS_OK;
    BOOL first = TRUE;
    STYLE_SELECTOR selector;
    STYLE_BIT_NUMBER style_bit_number;

    PTR_ASSERT(p_style);

    /* get bitmap we're going to use */
    void_style_selector_and(&selector, p_style_selector, &p_style->selector);

    /* if it's a named style, just show name */
    if(style_selector_bit_test(&selector, STYLE_SW_NAME))
    {
        style_selector_clear(&selector);
        style_selector_bit_set(&selector, STYLE_SW_NAME);
    }

    /* never worry about indirections */
    style_selector_bit_clear(&selector, STYLE_SW_HANDLE);

    for(style_bit_number = STYLE_BIT_NUMBER_FIRST;
        (style_bit_number = style_selector_next_bit(&selector, style_bit_number)) >= 0;
        STYLE_BIT_NUMBER_INCR(style_bit_number))
    {
        const PC_STYLE_ENCODING p_style_encoding = &style_encoding_table[style_bit_number];
        PC_BYTE p_data = PtrAddBytes(PC_BYTE, p_style, p_style_encoding->member_offset);
        STATUS message_no = (style_bit_number - STYLE_BIT_NUMBER_FIRST) + MSG_STYLE_TEXT_CONVERT_START;

        if(!first)
            status_break(status = quick_ublock_ustr_add(p_quick_ublock, USTR_TEXT(", ")));

        /* read text */
        status_break(status = resource_lookup_quick_ublock(p_quick_ublock, message_no));

        if(p_style_encoding->p_proc_style_text)
            status = (* p_style_encoding->p_proc_style_text) (p_docu, p_quick_ublock, p_data);
        else
        {
            /* standard data types */
            switch(p_style_encoding->inline_data_type)
            {
            case IL_TYPE_USTR:
                {
                ARRAY_HANDLE array_handle = * ((PC_ARRAY_HANDLE) p_data);
                if(array_elements(&array_handle))
                    if(status_ok(status = quick_ublock_a7char_add(p_quick_ublock, CH_SPACE)))
                        status = quick_ublock_ustr_add(p_quick_ublock, array_ustr(&array_handle));
                break;
                }

            case IL_TYPE_U8:
                status = quick_ublock_printf(p_quick_ublock, USTR_TEXT(" " S32_FMT), (S32) * ((PC_U8) p_data));
                break;

            case IL_TYPE_S32:
                status = quick_ublock_printf(p_quick_ublock, USTR_TEXT(" " S32_FMT), * ((PC_S32) p_data));
                break;

            case IL_TYPE_F64:
                status = quick_ublock_printf(p_quick_ublock, USTR_TEXT(" " F64_FMT), * ((PC_F64) p_data));
                break;

            case IL_TYPE_NONE:
                break;

#if CHECKING
            /* these are too hard */
            case IL_TYPE_ANY:
#endif
            default: default_unhandled();
                break;
            }
        }

        first = FALSE;

        status_break(status);
    }

    if(status_ok(status))
        status = quick_ublock_nullch_add(p_quick_ublock);

    return(status);
}

/* end of sk_stylc.c */
