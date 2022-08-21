/* tx_form.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Text formatting for Fireworkz */

/* MRJC February 1992 */

enum CHUNK_FORMAT_TYPES
{
    CHUNK_FREE = 0,
    CHUNK_TAB,
    CHUNK_DATE,
    CHUNK_FILE_DATE,
    CHUNK_PAGE_X,
    CHUNK_PAGE_Y,
    CHUNK_SS_NAME,
    CHUNK_MS_FIELD,
    CHUNK_WHOLENAME,
    CHUNK_LEAFNAME,
    CHUNK_HYPHEN,
    CHUNK_RETURN,
    CHUNK_SOFT_HYPHEN,
    CHUNK_UTF8,
    CHUNK_UTF8_AS_TEXT,
    CHUNK_COUNT
};

typedef struct CHUNK
{
    enum CHUNK_FORMAT_TYPES type; /* type of format for chunk */
    FONTY_CHUNK fonty_chunk;    /* fonty chunk block */
    PIXIT base_line;            /* unsigned offset of base line from chunk top */
    PIXIT leading;              /* spacing required */
    S32 input_ix;               /* index into input string */
    S32 input_len;              /* length of chunk */
    S32 trail_spaces;           /* number of trailing spaces */
    PIXIT width;                /* total width of chunk */
    PIXIT lead_space;           /* space at start of chunk */
}
CHUNK, * P_CHUNK; typedef const CHUNK * PC_CHUNK;

typedef struct SEGMENT
{
    PIXIT width;                /* width of segment */
    PIXIT base_line;            /* offset of base line from top of segment (~max ascent) */
    PIXIT leading;              /* vertical spacing for segment */
    ARRAY_INDEX start_chunk;    /* starting chunk */
    ARRAY_INDEX end_chunk;      /* ending chunk */
    SKEL_POINT skel_point;      /* top left of segment */
    PIXIT format_width;         /* total width used for formatting */
}
SEGMENT, * P_SEGMENT; typedef const SEGMENT * PC_SEGMENT;

/*
exported routines
*/

_Check_return_
extern ARRAY_INDEX
chunk_ix_from_inline_ix(
    P_FORMATTED_TEXT p_formatted_text,
    _InVal_     S32 string_ix);

extern void
formatted_text_box(
    _OutRef_    P_SKEL_RECT p_skel_rect_para,
    _OutRef_    P_SKEL_RECT p_skel_rect_text,
    P_FORMATTED_TEXT p_formatted_text,
    _InRef_     PC_PAGE_NUM p_page_offset);

/*ncr*/
extern S32
formatted_text_dispose_chunks(
    _InoutRef_  P_FORMATTED_TEXT p_formatted_text,
    _InVal_     ARRAY_INDEX chunk_ix);

extern void
formatted_text_dispose_segments(
    _InoutRef_  P_FORMATTED_TEXT p_formatted_text);

_Check_return_
extern STATUS
formatted_text_duplicate(
    _OutRef_    P_FORMATTED_TEXT p_formatted_text_out,
    _InRef_     P_FORMATTED_TEXT p_formatted_text_in);

extern void
formatted_text_init(
    _OutRef_    P_FORMATTED_TEXT p_formatted_text);

/*ncr*/
extern S32
formatted_text_justify_vertical(
    P_FORMATTED_TEXT p_formatted_text,
    P_TEXT_FORMAT_INFO p_text_format_info,
    _InRef_     PC_PAGE_NUM p_page_offset);

extern PIXIT
formatted_text_width_max(
    P_FORMATTED_TEXT p_formatted_text,
    P_TEXT_FORMAT_INFO p_text_format_info);

_Check_return_
extern STATUS
plain_text_from_formatted_text(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_ARRAY_HANDLE p_h_plain_text,
    P_FORMATTED_TEXT p_formatted_text,
    _In_z_      PC_USTR_INLINE ustr_inline,
    _InRef_     PC_PAGE_NUM p_page_num,
    _InRef_     PC_STYLE p_style_text_global);

_Check_return_
extern ARRAY_INDEX
segment_ix_from_inline_ix(
    P_FORMATTED_TEXT p_formatted_text,
    _InVal_     S32 string_ix);

extern P_SKEL_RECT
skel_rect_from_segment(
    _OutRef_    P_SKEL_RECT p_skel_rect,
    _InRef_     PC_SEGMENT p_segment,
    _InRef_     PC_PAGE_NUM p_page_offset);

_Check_return_
extern STATUS
text_from_field_uchars(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _In_reads_(uchars_n) PC_UCHARS_INLINE uchars_inline CODE_ANALYSIS_ONLY_ARG(_InVal_ U32 uchars_n),
    _InRef_opt_ PC_PAGE_NUM p_page_num,
    _InRef_opt_ PC_STYLE p_style_text_global);

/*ncr*/
extern BOOL
text_location_from_skel_point(
    _DocuRef_   P_DOCU p_docu,
    P_TEXT_LOCATION p_text_location,
    _In_z_      PC_USTR_INLINE ustr_inline,
    P_FORMATTED_TEXT p_formatted_text,
    P_SKEL_POINT p_skel_point,
    _InRef_     PC_PAGE_NUM p_page_offset);

_Check_return_
extern STATUS
text_segment(
    _DocuRef_   P_DOCU p_docu,
    P_FORMATTED_TEXT p_formatted_text,
    P_TEXT_FORMAT_INFO p_text_format_info,
    P_INLINE_OBJECT p_inline_object);

/*ncr*/
extern BOOL
text_skel_point_from_location(
    _DocuRef_   P_DOCU p_docu,
    P_TEXT_LOCATION p_text_location,
    P_USTR_INLINE ustr_inline,
    P_FORMATTED_TEXT p_formatted_text,
    _InVal_     S32 end,
    _InRef_     PC_PAGE_NUM p_page_offset);

/* end of tx_form.h */
