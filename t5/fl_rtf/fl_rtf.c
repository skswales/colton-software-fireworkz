/* fl_rtf.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* RTF load object module for Fireworkz */

/* JAD Jul 1993 */

#include "common/gflags.h"

#include "fl_rtf/fl_rtf.h"

#include "ob_skel/ff_io.h"

#include "cmodules/unicode/u2000.h" /* 2000..206F General Punctuation */

#if RISCOS
#define MSG_WEAK &rb_fl_rtf_msg_weak
extern PC_U8 rb_fl_rtf_msg_weak;
#endif

#define P_BOUND_RESOURCES_OBJECT_ID_FL_RTF DONT_LOAD_RESOURCES

#define TRACE_IO_RTF TRACE_APP_FOREIGN

typedef enum RTF_DESTINATION
{
    RTF_DESTINATION_NORMAL = 0,
    RTF_DESTINATION_COLORTBL,
    RTF_DESTINATION_FLDRSLT,
    RTF_DESTINATION_FONTTBL,
    RTF_DESTINATION_STYLESHEET,

    /* data for this set of destinations are discarded */
    RTF_DESTINATION_AUTHOR = 16,
    RTF_DESTINATION_FIELD,
    RTF_DESTINATION_FLDINST,
    RTF_DESTINATION_INFO,
    RTF_DESTINATION_MMATHPR,
    RTF_DESTINATION_PICT,

    RTF_DESTINATION_IGNORABLE
}
RTF_DESTINATION;

typedef struct RTF_LOAD_INFO
{
    P_MSG_INSERT_FOREIGN p_msg_insert_foreign;
    DOCNO           docno;
    PC_U8           p_u8;
    PROCESS_STATUS  process_status;
    POSITION        load_position;          /* Initial loading position */

    BOOL            inserting;

    COL             doc_cols;               /* Number of columns in document */
    ROW             doc_rows;               /* Number of rows in document */
    S32             doc_regions;            /* Number of regions created */

    ARRAY_HANDLE    h_colour_table_load;    /* List of all colours used in Fireworkz styles */
    ARRAY_HANDLE    h_rtf_font_table;       /* List of font names */
    ARRAY_HANDLE    h_stylesheet_load;      /* List of stylesheet entries */
    ARRAY_HANDLE    h_da_list;              /* Handle for arrays of regions */
    ARRAY_HANDLE    h_footnotes;            /* Footnotes to go at end of document */

    BOOL            in_table;               /* Nasty bodge to know whether we are in a table or not on loading */
    ARRAY_HANDLE    h_table_widths;         /* Widths defined for current table */

    POSITION        position;               /* Current position */

    S32             bracket_level;

    U32             ignore_chars_count;

    RTF_DESTINATION destination;

    /*
    COLORTBL destination
    */

    RGB             colortbl_rgb;

    /*
    FONTTBL destination
    */

    ARRAY_HANDLE    fonttbl_h_rtf_font_name_tstr;
    U32             fonttbl_font_number;

#define RTF_FONT_NUMBER_INVALID (0xFFFFFFFFU)

    /*
    STYLESHEET destination
    */

    ARRAY_HANDLE    stylesheet_h_style_name_tstr;
    U32             stylesheet_style_number;

    STATE_STYLE     state_style;

    TAB_TYPE        tab_type;

    S32             grid_colour;            /* Grid Colour */

    U32             _ansicpg;
    U32             _deff;
    BOOL            _loch_hich_dbch;
    S32             _trleft;
    U32             _uc;
}
RTF_LOAD_INFO, * P_RTF_LOAD_INFO;

_Check_return_
static inline U8
rtf_get_char(
    _InoutRef_  P_RTF_LOAD_INFO p_rtf_load_info)
{
    return(PtrGetByte(p_rtf_load_info->p_u8++));
}

static void
rtf_advance_char(
    _InoutRef_  P_RTF_LOAD_INFO p_rtf_load_info,
    _InVal_     S32 n_bytes /* can be -ve */)
{
    p_rtf_load_info->p_u8 += n_bytes;
}

_Check_return_
static inline U8
rtf_peek_char_off(
    _InoutRef_  P_RTF_LOAD_INFO p_rtf_load_info,
    _InVal_     U32 offset)
{
    return(PtrGetByteOff(p_rtf_load_info->p_u8, offset));
}

_Check_return_
static STATUS
rtf_destination_colortbl_char(
    _InoutRef_  P_RTF_LOAD_INFO p_rtf_load_info,
    _InVal_     U8 rtf_char);

_Check_return_
static STATUS
rtf_destination_fonttbl_char(
    _InoutRef_  P_RTF_LOAD_INFO p_rtf_load_info,
    _InVal_     U8 rtf_char);

_Check_return_
static STATUS
rtf_destination_stylesheet_char(
    _InoutRef_  P_RTF_LOAD_INFO p_rtf_load_info,
    _InVal_     U8 rtf_char);

typedef STATUS (* P_PROC_RTF_CONTROL) (
    _InoutRef_  P_RTF_LOAD_INFO p_rtf_load_info,
    _In_z_      PC_A7STR a7str_control_word,
    _In_z_      PC_A7STR a7str_parameter,
    _InoutRef_  P_ARRAY_HANDLE p_h_dest_contents,
    P_ARRAY_HANDLE p_h_sub_regions,
    _InVal_     UCS4 arg);

#define PROC_RTF_CONTROL_PROTO(_proc_name) \
static STATUS \
_proc_name( \
    _InoutRef_  P_RTF_LOAD_INFO p_rtf_load_info, \
    _In_z_      PC_A7STR a7str_control_word, \
    _In_z_      PC_A7STR a7str_parameter, \
    _InoutRef_  P_ARRAY_HANDLE p_h_dest_contents, \
    P_ARRAY_HANDLE p_h_sub_regions, \
    _InVal_     UCS4 arg)

#define func_ignore_parms() \
    UNREFERENCED_PARAMETER_InoutRef_(p_rtf_load_info); \
    UNREFERENCED_PARAMETER(a7str_control_word); \
    UNREFERENCED_PARAMETER(a7str_parameter); \
    UNREFERENCED_PARAMETER_InoutRef_(p_h_dest_contents); \
    UNREFERENCED_PARAMETER(p_h_sub_regions); \
    UNREFERENCED_PARAMETER_InVal_(arg)

_Check_return_
static STATUS
rtf_load_document_style(
    _InoutRef_  P_RTF_LOAD_INFO p_rtf_load_info);

_Check_return_
static STATUS
rtf_load_regions_to_document(
    _InoutRef_  P_RTF_LOAD_INFO p_rtf_load_info);

static void
rtf_load_decode_colour(
    _InoutRef_  P_RTF_LOAD_INFO p_rtf_load_info,
    _OutRef_    P_RGB p_rgb,
    _InVal_     S32 colour);

_Check_return_
static STATUS
rtf_load_set_width(
    _InoutRef_  P_RTF_LOAD_INFO p_rtf_load_info);

_Check_return_
static STATUS
rtf_load_check_reverse_attribute(
    _InRef_     PC_STATE_STYLE p_state_style,
    _InVal_     S32 attrib,
    _InVal_     S32 paramter);

_Check_return_
static STATUS
rtf_load_state_style_set_bit(
    _InoutRef_  P_RTF_LOAD_INFO p_rtf_load_info,
    _InoutRef_  P_STATE_STYLE p_state_style,
    _InVal_     STYLE_BIT_NUMBER style_bit_number,
    _InVal_     S32 parameter);

_Check_return_
static STATUS
rtf_load_state_style_similar_bit(
    _InoutRef_  P_RTF_LOAD_INFO p_rtf_load_info,
    P_STATE_STYLE p_state_style,
    _InVal_     STYLE_BIT_NUMBER style_bit_number,
    _InVal_     S32 parameter);

_Check_return_
static STATUS
rtf_load_process_group(
    _InoutRef_  P_RTF_LOAD_INFO p_rtf_load_info,
    _InoutRef_  P_ARRAY_HANDLE p_h_dest_contents);

_Check_return_
static STATUS
rtf_load_obtain_control_word(
    _InoutRef_  P_RTF_LOAD_INFO p_rtf_load_info,
    _InoutRef_  P_QUICK_BLOCK p_quick_block /*appended,terminated*/,
    _OutRef_    P_U32 p_parameter_offset,
    _InoutRef_  P_BOOL p_ignorable_destination /*possibly updated*/);

PROC_RTF_CONTROL_PROTO(rtf_control_ansi);
PROC_RTF_CONTROL_PROTO(rtf_control_ansicpg);
PROC_RTF_CONTROL_PROTO(rtf_control_brdrcf);
PROC_RTF_CONTROL_PROTO(rtf_control_cell);
PROC_RTF_CONTROL_PROTO(rtf_control_cellx);
PROC_RTF_CONTROL_PROTO(rtf_control_chdate);
PROC_RTF_CONTROL_PROTO(rtf_control_chpgn);
PROC_RTF_CONTROL_PROTO(rtf_control_clbrdr);
PROC_RTF_CONTROL_PROTO(rtf_control_colortbl_rgb);
PROC_RTF_CONTROL_PROTO(rtf_control_dbch);
PROC_RTF_CONTROL_PROTO(rtf_control_deff);
PROC_RTF_CONTROL_PROTO(rtf_control_f);
PROC_RTF_CONTROL_PROTO(rtf_control_footer);
PROC_RTF_CONTROL_PROTO(rtf_control_footnote);
PROC_RTF_CONTROL_PROTO(rtf_control_header);
PROC_RTF_CONTROL_PROTO(rtf_control_hich);
PROC_RTF_CONTROL_PROTO(rtf_control_intbl);
PROC_RTF_CONTROL_PROTO(rtf_control_landscape);
PROC_RTF_CONTROL_PROTO(rtf_control_line);
PROC_RTF_CONTROL_PROTO(rtf_control_loch);
PROC_RTF_CONTROL_PROTO(rtf_control_page_margin);
PROC_RTF_CONTROL_PROTO(rtf_control_paper_size);
PROC_RTF_CONTROL_PROTO(rtf_control_par);
PROC_RTF_CONTROL_PROTO(rtf_control_pard);
PROC_RTF_CONTROL_PROTO(rtf_control_row);
PROC_RTF_CONTROL_PROTO(rtf_control_rtf);
PROC_RTF_CONTROL_PROTO(rtf_control_s);
PROC_RTF_CONTROL_PROTO(rtf_control_sectd);
PROC_RTF_CONTROL_PROTO(rtf_control_tab);
PROC_RTF_CONTROL_PROTO(rtf_control_tq);
PROC_RTF_CONTROL_PROTO(rtf_control_trleft);
PROC_RTF_CONTROL_PROTO(rtf_control_trowd);
PROC_RTF_CONTROL_PROTO(rtf_control_tx);
PROC_RTF_CONTROL_PROTO(rtf_control_u);
PROC_RTF_CONTROL_PROTO(rtf_control_uc);

PROC_RTF_CONTROL_PROTO(rtf_control_hex_char);

PROC_RTF_CONTROL_PROTO(rtf_control_ignore);
PROC_RTF_CONTROL_PROTO(rtf_control_warning_charset);

PROC_RTF_CONTROL_PROTO(rtf_control_insert_char);
PROC_RTF_CONTROL_PROTO(rtf_control_style);

PROC_RTF_CONTROL_PROTO(rtf_destination_author);
PROC_RTF_CONTROL_PROTO(rtf_destination_colortbl);
PROC_RTF_CONTROL_PROTO(rtf_destination_field);
PROC_RTF_CONTROL_PROTO(rtf_destination_fldinst);
PROC_RTF_CONTROL_PROTO(rtf_destination_fldrslt);
PROC_RTF_CONTROL_PROTO(rtf_destination_fonttbl);
PROC_RTF_CONTROL_PROTO(rtf_destination_info);
PROC_RTF_CONTROL_PROTO(rtf_destination_mmathPr);
PROC_RTF_CONTROL_PROTO(rtf_destination_pict);
PROC_RTF_CONTROL_PROTO(rtf_destination_stylesheet);

#define HEADER_DEFAULT_MARGIN (PIXITS_PER_CM * 1)
#define HEADER_DEFAULT_OFFSET (68) /* same as in Letter template */

#define FOOTER_DEFAULT_MARGIN  (PIXITS_PER_CM * 1)
#define FOOTER_DEFAULT_OFFSET ((PIXITS_PER_CM * 4) / 10)

static const HEADFOOT_SIZES
headfoot_sizes_default_header = { HEADER_DEFAULT_MARGIN, HEADER_DEFAULT_OFFSET };

static const HEADFOOT_SIZES
headfoot_sizes_default_footer = { FOOTER_DEFAULT_MARGIN, FOOTER_DEFAULT_OFFSET };

typedef struct RTF_FONT_MAP
{
    PCTSTR rtf_font_name;
    PCTSTR app_font_name;
}
RTF_FONT_MAP;

static const RTF_FONT_MAP
rtf_font_map[] =
{
    /* font name in file       neutral font name */
    { TEXT("Helv"),             TEXT("Helvetica") },
    { TEXT("MS Sans Serif"),    TEXT("Helvetica") },
    { TEXT("Times New Roman"),  TEXT("Times")     },
    { TEXT("Tms Rmn"),          TEXT("Times")     }
};

/* Tufty says 'keep me sorted' */

typedef struct RTF_CONTROL_WORD
{
    PC_A7STR a7str_control_word;
    U32 control_word_length; /* filled at run-time */

    P_PROC_RTF_CONTROL p_control_proc;
    UCS4 arg;
}
RTF_CONTROL_WORD; typedef const RTF_CONTROL_WORD * PC_RTF_CONTROL_WORD;

#define RTCW(p_control_word, p_control_proc, arg) { p_control_word, 0, p_control_proc, arg },

#if CHECKING
#define COCW(p_control_word, p_control_proc, arg) { p_control_word, 0, p_control_proc, arg },
#else
#define COCW(p_control_word, p_control_proc, arg) /* { p_control_word, 0, p_control_proc, arg } */
#endif

static
RTF_CONTROL_WORD
rtf_control_words[] =
{
RTCW("'"                , rtf_control_hex_char          , 0                                 )   /* hex value for member of character set */
RTCW("-"                , rtf_control_insert_char       , UCH_SOFT_HYPHEN                   )   /* optional hyphen */
RTCW("\\"               , rtf_control_insert_char       , CH_BACKWARDS_SLASH                )   /* insert backslash into text */
RTCW("_"                , rtf_control_insert_char       , CH_HYPHEN_MINUS                   )   /* non-breaking hyphen */
COCW("adeff"            , rtf_control_ignore            , 0                                 )
COCW("adeflang"         , rtf_control_ignore            , 0                                 )
COCW("adjustright"      , rtf_control_ignore            , 0                                 )
COCW("af"               , rtf_control_ignore            , 0                                 )
COCW("afs"              , rtf_control_ignore            , 0                                 )
COCW("alang"            , rtf_control_ignore            , 0                                 )
RTCW("ansi"             , rtf_control_ansi              , 0                                 )   /* ansi */
RTCW("ansicpg"          , rtf_control_ansicpg           , 0                                 )   /* ansicpg */
COCW("aspalpha"         , rtf_control_ignore            , 0                                 )
COCW("aspnum"           , rtf_control_ignore            , 0                                 )
RTCW("author"           , rtf_destination_author        , 0                                 )   /* author (destination) */
RTCW("b"                , rtf_control_style             , STYLE_SW_FS_BOLD                  )   /* bold */
RTCW("blue"             , rtf_control_colortbl_rgb      , offsetof32(RGB, b)                )   /* blue - only in colortbl */
RTCW("brdrb"            , rtf_control_style             , STYLE_SW_PS_GRID_BOTTOM           )   /* border bottom */
RTCW("brdrcf"           , rtf_control_brdrcf            , 0                                 )   /* border colour */
RTCW("brdrl"            , rtf_control_style             , STYLE_SW_PS_GRID_LEFT             )   /* border left */
RTCW("brdrr"            , rtf_control_style             , STYLE_SW_PS_GRID_RIGHT            )   /* border right */
RTCW("brdrt"            , rtf_control_style             , STYLE_SW_PS_GRID_TOP              )   /* border top */
RTCW("bullet"           , rtf_control_insert_char       , UCH_BULLET                        )   /* insert bullet character into text */
RTCW("cb"               , rtf_control_style             , STYLE_SW_PS_RGB_BACK              )   /* bg colour */
RTCW("cell"             , rtf_control_cell              , 0                                 )   /* termination of cell */
RTCW("cellx"            , rtf_control_cellx             , 0                                 )   /* move right boundary of cell */
RTCW("cf"               , rtf_control_style             , STYLE_SW_FS_COLOUR                )   /* fg colour */
COCW("cgrid"            , rtf_control_ignore            , 0                                 )
COCW("charrsid"         , rtf_control_ignore            , 0                                 )
RTCW("chdate"           , rtf_control_chdate            , 0                                 )   /* current date */
RTCW("chpgn"            , rtf_control_chpgn             , 0                                 )   /* current page number */
COCW("chyperlink"       , rtf_control_ignore            , 0                                 )
RTCW("clbrdrb"          , rtf_control_clbrdr            , STYLE_SW_PS_GRID_BOTTOM           )   /* bottom table cell border */
RTCW("clbrdrl"          , rtf_control_clbrdr            , STYLE_SW_PS_GRID_LEFT             )   /* left table cell border */
RTCW("clbrdrr"          , rtf_control_clbrdr            , STYLE_SW_PS_GRID_RIGHT            )   /* right table cell border */
RTCW("clbrdrt"          , rtf_control_clbrdr            , STYLE_SW_PS_GRID_TOP              )   /* top table cell border */
RTCW("colortbl"         , rtf_destination_colortbl      , 0                                 )   /* colortbl (destination) */
COCW("cs"               , rtf_control_ignore            , 0                                 )
COCW("cshade"           , rtf_control_ignore            , 0                                 )
COCW("ctint"            , rtf_control_ignore            , 0                                 )
RTCW("dbch"             , rtf_control_dbch              , 0                                 )
RTCW("deff"             , rtf_control_deff              , 0                                 )   /* deff */
COCW("deflang"          , rtf_control_ignore            , 0                                 )
COCW("deflangfe"        , rtf_control_ignore            , 0                                 )
RTCW("dn"               , rtf_control_style             , STYLE_SW_FS_SUBSCRIPT             )   /* subscript */
RTCW("emdash"           , rtf_control_insert_char       , UCH_EM_DASH                       )   /* insert emdash into text */
RTCW("endash"           , rtf_control_insert_char       , UCH_EN_DASH                       )   /* insert endash into text */
RTCW("f"                , rtf_control_f                 , STYLE_SW_FS_NAME                  )   /* font */
COCW("faauto"           , rtf_control_ignore            , 0                                 )
COCW("fcs"              , rtf_control_ignore            , 0                                 )
COCW("fdecor"           , rtf_control_ignore            , 0                                 )   /* decor font family */
RTCW("fi"               , rtf_control_style             , STYLE_SW_PS_MARGIN_PARA           )   /* first line indent */
RTCW("field"            , rtf_destination_field         , 0                                 )   /* field (destination): field group control word */
RTCW("fldinst"          , rtf_destination_fldinst       , 0                                 )   /* fldinst (destination): field instructions */
RTCW("fldrslt"          , rtf_destination_fldrslt       , 0                                 )   /* fldrslt (destination): most recent calculated field result */
COCW("fmodern"          , rtf_control_ignore            , 0                                 )   /* modern font family */
COCW("fnil"             , rtf_control_ignore            , 0                                 )   /* unknown font family */
RTCW("fonttbl"          , rtf_destination_fonttbl       , 0                                 )   /* font table (destination) */
RTCW("footer"           , rtf_control_footer            , RTF_HEFO_ALL                      )   /* footer */
RTCW("footerf"          , rtf_control_footer            , RTF_HEFO_FIRST                    )   /* first pages footer */
RTCW("footerl"          , rtf_control_footer            , RTF_HEFO_LEFT                     )   /* left pages footer */
RTCW("footerr"          , rtf_control_footer            , RTF_HEFO_RIGHT                    )   /* right pages footer */
RTCW("footnote"         , rtf_control_footnote          , 0                                 )   /* footnote */
COCW("froman"           , rtf_control_ignore            , 0                                 )   /* roman font family */
RTCW("fs"               , rtf_control_style             , STYLE_SW_FS_SIZE_Y                )   /* font size */
COCW("fscript"          , rtf_control_ignore            , 0                                 )   /* script font family */
COCW("fswiss"           , rtf_control_ignore            , 0                                 )   /* swiss font family */
COCW("ftech"            , rtf_control_ignore            , 0                                 )   /* technical font family */
RTCW("green"            , rtf_control_colortbl_rgb      , offsetof32(RGB, g)                )   /* green - only in colortbl */
COCW("gutter"           , rtf_control_ignore            , 0                                 )
RTCW("header"           , rtf_control_header            , RTF_HEFO_ALL                      )   /* header */
RTCW("headerf"          , rtf_control_header            , RTF_HEFO_FIRST                    )   /* first pages header */
RTCW("headerl"          , rtf_control_header            , RTF_HEFO_LEFT                     )   /* left pages header */
RTCW("headerr"          , rtf_control_header            , RTF_HEFO_RIGHT                    )   /* right pages header */
RTCW("hich"             , rtf_control_hich              , 0                                 )
RTCW("i"                , rtf_control_style             , STYLE_SW_FS_ITALIC                )   /* italic */
RTCW("info"             , rtf_destination_info          , 0                                 )   /* info (destination) */
COCW("insrsid"          , rtf_control_ignore            , 0                                 )
RTCW("intbl"            , rtf_control_intbl             , 0                                 )   /* paragraph is part of a table */
COCW("itap"             , rtf_control_ignore            , 0                                 )
RTCW("landscape"        , rtf_control_landscape         , 0                                 )   /* paper is landscape orientation */
COCW("lang"             , rtf_control_ignore            , 0                                 )
COCW("langfe"           , rtf_control_ignore            , 0                                 )
COCW("langfenp"         , rtf_control_ignore            , 0                                 )
COCW("langnp"           , rtf_control_ignore            , 0                                 )
RTCW("ldblquote"        , rtf_control_insert_char       , UCH_LEFT_DOUBLE_QUOTATION_MARK    )   /* insert left double quote into text */
RTCW("li"               , rtf_control_style             , STYLE_SW_PS_MARGIN_LEFT           )   /* left indent */
RTCW("lin"              , rtf_control_style             , STYLE_SW_PS_MARGIN_LEFT           )   /* left indent (only handles LTR) */
RTCW("line"             , rtf_control_line              , 0                                 )   /* ctrl-return character */
COCW("linex"            , rtf_control_ignore            , 0                                 )
RTCW("loch"             , rtf_control_loch              , 0                                 )
RTCW("lquote"           , rtf_control_insert_char       , UCH_LEFT_SINGLE_QUOTATION_MARK    )   /* insert left quote into text */
COCW("ltrch"            , rtf_control_ignore            , 0                                 )
COCW("ltrpar"           , rtf_control_ignore            , 0                                 )
COCW("ltrsect"          , rtf_control_ignore            , 0                                 )
RTCW("mac"              , rtf_control_warning_charset   , 0                                 )   /* Apple Mac */
RTCW("margb"            , rtf_control_page_margin       , STYLE_SW_PS_PARA_END              )   /* bottom margin */
RTCW("margl"            , rtf_control_page_margin       , STYLE_SW_PS_MARGIN_LEFT           )   /* left margin */
RTCW("margr"            , rtf_control_page_margin       , STYLE_SW_PS_MARGIN_RIGHT          )   /* right margin */
RTCW("margt"            , rtf_control_page_margin       , STYLE_SW_PS_PARA_START            )   /* top margin */
RTCW("mmathPr"          , rtf_destination_mmathPr       , 0                                 )   /* mmathPr (destination) */
COCW("noqfpromote"      , rtf_control_ignore            , 0                                 )
COCW("nowidctlpar"      , rtf_control_ignore            , 0                                 )
RTCW("paperh"           , rtf_control_paper_size        , 1 /*y*/                           )   /* paper height */
RTCW("paperw"           , rtf_control_paper_size        , 0 /*x*/                           )   /* paper width */
RTCW("par"              , rtf_control_par               , 0                                 )   /* end of paragraph */
COCW("pararsid"         , rtf_control_ignore            , 0                                 )
RTCW("pard"             , rtf_control_pard              , 0                                 )   /* default paragraph */
RTCW("pc"               , rtf_control_warning_charset   , 0                                 )   /* IBM PC CP437 */
RTCW("pca"              , rtf_control_warning_charset   , 0                                 )   /* IBM PC CP850 */
RTCW("pict"             , rtf_destination_pict          , 0                                 )   /* pict (destination) */
COCW("plain"            , rtf_control_ignore            , 0                                 )
RTCW("qc"               , rtf_control_style             , STYLE_SW_PS_JUSTIFY               )   /* centered */
RTCW("qj"               , rtf_control_style             , STYLE_SW_PS_JUSTIFY               )   /* justified */
RTCW("ql"               , rtf_control_style             , STYLE_SW_PS_JUSTIFY               )   /* left aligned */
RTCW("qr"               , rtf_control_style             , STYLE_SW_PS_JUSTIFY               )   /* right aligned */
RTCW("rdblquote"        , rtf_control_insert_char       , UCH_RIGHT_DOUBLE_QUOTATION_MARK   )   /* insert right double quote into text */
RTCW("red"              , rtf_control_colortbl_rgb      , offsetof32(RGB, r)                )   /* red - only in colortbl */
RTCW("ri"               , rtf_control_style             , STYLE_SW_PS_MARGIN_RIGHT          )   /* right indent */
RTCW("rin"              , rtf_control_style             , STYLE_SW_PS_MARGIN_RIGHT          )   /* right indent (only handles LTR) */
RTCW("row"              , rtf_control_row               , 0                                 )   /* termination of row */
RTCW("rquote"           , rtf_control_insert_char       , UCH_RIGHT_SINGLE_QUOTATION_MARK   )   /* insert right quote into text */
RTCW("rtf"              , rtf_control_rtf               , 0                                 )   /* RTF Version */
COCW("rtlch"            , rtf_control_ignore            , 0                                 )
RTCW("s"                , rtf_control_s                 , 0                                 )   /* style no */
RTCW("sa"               , rtf_control_style             , STYLE_SW_PS_PARA_END              )   /* space after */
RTCW("sb"               , rtf_control_style             , STYLE_SW_PS_PARA_START            )   /* space before */
RTCW("sectd"            , rtf_control_sectd             , 0                                 )   /* default section */
COCW("sftnbj"           , rtf_control_ignore            , 0                                 )
RTCW("sl"               , rtf_control_style             , STYLE_SW_RS_HEIGHT                )   /* line spacing */
COCW("slmult"           , rtf_control_ignore            , 0                                 )
COCW("snext"            , rtf_control_ignore            , 0                                 )
COCW("spriority"        , rtf_control_ignore            , 0                                 )
COCW("sqformat"         , rtf_control_ignore            , 0                                 )
COCW("stshfbi"          , rtf_control_ignore            , 0                                 )
COCW("stshfdbch"        , rtf_control_ignore            , 0                                 )
COCW("stshfhich"        , rtf_control_ignore            , 0                                 )
COCW("stshfloch"        , rtf_control_ignore            , 0                                 )
RTCW("stylesheet"       , rtf_destination_stylesheet    , 0                                 )   /* stylesheet (destination) */
RTCW("tab"              , rtf_control_tab               , 0                                 )   /* tab */
COCW("themelang"        , rtf_control_ignore            , 0                                 )
COCW("themelangcs"      , rtf_control_ignore            , 0                                 )
COCW("themelangfe"      , rtf_control_ignore            , 0                                 )
RTCW("tqc"              , rtf_control_tq                , TAB_CENTRE                        )   /* centered tab */
RTCW("tqdec"            , rtf_control_tq                , TAB_DECIMAL                       )   /* decimal tab */
RTCW("tqr"              , rtf_control_tq                , TAB_RIGHT                         )   /* flush-right tab */
RTCW("trleft"           , rtf_control_trleft            , 0                                 )   /* leftmost edge of table */
RTCW("trowd"            , rtf_control_trowd             , 0                                 )   /* set table row defaults */
RTCW("tx"               , rtf_control_tx                , 0                                 )   /* tab position in twips from the left margin */
RTCW("u"                , rtf_control_u                 , 0                                 )   /* hex value for Unicode code point */
RTCW("uc"               , rtf_control_uc                , 0                                 )   /* chars to follow Unicode code point */
RTCW("ul"               , rtf_control_style             , STYLE_SW_FS_UNDERLINE             )   /* underline */
RTCW("uld"              , rtf_control_style             , STYLE_SW_FS_UNDERLINE             )   /* dotted underline */
RTCW("uldb"             , rtf_control_style             , STYLE_SW_FS_UNDERLINE             )   /* double underline */
RTCW("ulw"              , rtf_control_style             , STYLE_SW_FS_UNDERLINE             )   /* word underline */
RTCW("up"               , rtf_control_style             , STYLE_SW_FS_SUPERSCRIPT           )   /* superscript */
RTCW("vertal"           , rtf_control_style             , STYLE_SW_PS_JUSTIFY_V             )   /* text bottom-aligned */
RTCW("vertalc"          , rtf_control_style             , STYLE_SW_PS_JUSTIFY_V             )   /* text centered vertically */
RTCW("vertalj"          , rtf_control_style             , STYLE_SW_PS_JUSTIFY_V             )   /* text justified vertically */
RTCW("vertalt"          , rtf_control_style             , STYLE_SW_PS_JUSTIFY_V             )   /* text is top-aligned (default) */
COCW("widctlpar"        , rtf_control_ignore            , 0                                 )
COCW("widowctrl"        , rtf_control_ignore            , 0                                 )
COCW("wrapdefault"      , rtf_control_ignore            , 0                                 )
RTCW("{"                , rtf_control_insert_char       , CH_LEFT_CURLY_BRACKET             )   /* insert char into text */
RTCW("}"                , rtf_control_insert_char       , CH_RIGHT_CURLY_BRACKET            )   /* insert char into text */
RTCW("~"                , rtf_control_insert_char       , UCH_NO_BREAK_SPACE                )   /* non-breaking space */
};

static S32            skipping;

static const ARRAY_INIT_BLOCK ar_in_blk = aib_init(32, sizeof32(BYTE), TRUE); /* SKS 20feb2012 initialiser was missing! */

#define rtf_array_add_bytes(p_h_data, n_bytes, p_data) \
    al_array_add(p_h_data, BYTE, n_bytes, &ar_in_blk, p_data)

#define rtf_array_add_NULLCH(p_h_data) \
    rtf_array_add_bytes(p_h_data, 1, empty_string)

/******************************************************************************
*
* Compare RTF control words
*
******************************************************************************/

typedef struct RTF_CONTROL_WORD_TABLE_SEARCH
{
    PC_A7STR a7str_control_word;
    U32 control_word_length;
}
RTF_CONTROL_WORD_TABLE_SEARCH, * P_RTF_CONTROL_WORD_TABLE_SEARCH; typedef const RTF_CONTROL_WORD_TABLE_SEARCH * PC_RTF_CONTROL_WORD_TABLE_SEARCH;

#define RTF_CONTROL_WORD_LENGTH_PRIMARY_KEY 1

#if RISCOS
#define rtf_control_word_table_compare short_memcmp32
#else
#define rtf_control_word_table_compare memcmp /* use native function */
#endif /* OS */

PROC_BSEARCH_PROTO(static, compare_rtf_control_word, RTF_CONTROL_WORD_TABLE_SEARCH, RTF_CONTROL_WORD)
{
    BSEARCH_KEY_VAR_DECL(PC_RTF_CONTROL_WORD_TABLE_SEARCH, key);
    BSEARCH_DATUM_VAR_DECL(PC_RTF_CONTROL_WORD, datum);

    PC_A7STR a7str_control_word_key;
    PC_A7STR a7str_control_word_datum;

#if defined(RTF_CONTROL_WORD_LENGTH_PRIMARY_KEY)
    const U32 control_word_length_key   = key->control_word_length;
    const U32 control_word_length_datum = datum->control_word_length;

    if(control_word_length_key != control_word_length_datum)
        return((int) control_word_length_key - (int) control_word_length_datum);

    a7str_control_word_key = key->a7str_control_word;
    a7str_control_word_datum = datum->a7str_control_word;

    return(rtf_control_word_table_compare(a7str_control_word_key, a7str_control_word_datum, control_word_length_key));
#else
    {
    PC_A7STR p_control_word_1;
    PC_A7STR p_control_word_2;

    p_control_word_1 = a7str_control_word_key = key->a7str_control_word;
    p_control_word_2 = a7str_control_word_datum = datum->a7str_control_word;

    for(;;)
    {
        /* NB. case sensitive (we sorted the table that way) */
        int c1 = *p_control_word_1++;
        int c2 = *p_control_word_2++;

        if(c1 == c2)
        {
            if(CH_NULL != c1)
                continue;

            /* ended together -> equal */
            /*key->end_offset = PtrDiffBytesU32(p_control_word_1, key->p_control_word);*/
            return(0);
        }

        if(CH_NULL == c2)
        {   /* control_word_2 (table) ended before control_word_1 (test) -  match if parameter present */
            if(/*"C"*/isdigit(c1) || (CH_HYPHEN_MINUS == c1))
            {
                /*--p_control_word_1;*/
                /*key->end_offset = PtrDiffBytesU32(p_control_word_1, key->p_control_word);*/
                return(0);
            }
        }

        /* no match */
        /*key->end_offset = 0;*/
        return(c1 - c2);
    }
    }
#endif
}

/******************************************************************************
*
* Compare RTF control words for qsort
*
******************************************************************************/

PROC_QSORT_PROTO(static, compare_rtf_control_word_qsort, RTF_CONTROL_WORD)
{
    int res;

    QSORT_ARG1_VAR_DECL(PC_RTF_CONTROL_WORD, p_rtf_control_word_1);
    QSORT_ARG2_VAR_DECL(PC_RTF_CONTROL_WORD, p_rtf_control_word_2);

    PC_A7STR a7str_control_word_1;
    PC_A7STR a7str_control_word_2;

#if defined(RTF_CONTROL_WORD_LENGTH_PRIMARY_KEY)
    const U32 control_word_length_1 = p_rtf_control_word_1->control_word_length;
    const U32 control_word_length_2 = p_rtf_control_word_2->control_word_length;

    if(control_word_length_1 != control_word_length_2)
        return((int) control_word_length_1 - (int) control_word_length_2);

    a7str_control_word_1 = p_rtf_control_word_1->a7str_control_word;
    a7str_control_word_2 = p_rtf_control_word_2->a7str_control_word;

    res = rtf_control_word_table_compare(a7str_control_word_1, a7str_control_word_2, control_word_length_1);
#else
    a7str_control_word_1 = p_rtf_control_word_1->a7str_control_word;
    a7str_control_word_2 = p_rtf_control_word_2->a7str_control_word;

    res = /*"C"*/strcmp(a7str_control_word_1, a7str_control_word_2);
#endif

    myassert0x((res != 0) || (p_rtf_control_word_1 == p_rtf_control_word_2), TEXT("RTF control word table has identical entries"));

    return(res);
}

static void
rtf_msg_startup_sort(void)
{
    U32 i;

    for(i = 0; i < elemof(rtf_control_words); ++i)
    {
        rtf_control_words[i].control_word_length = strlen32(rtf_control_words[i].a7str_control_word);
    }

    qsort(&rtf_control_words[0], elemof(rtf_control_words), sizeof(rtf_control_words[0]), compare_rtf_control_word_qsort);
}

/******************************************************************************
*
* Lookup colour in table
*
* Return index of element as colour table entry
*
******************************************************************************/

_Check_return_
static ARRAY_INDEX
find_colour_in_table_load(
    _InRef_     PC_ARRAY_HANDLE p_h_colour_table,
    _InRef_     PC_RGB p_rgb,
    _OutRef_    P_BOOL p_found)
{
    const ARRAY_INDEX n_elements = array_elements(p_h_colour_table);
    ARRAY_INDEX index;
    PC_RGB p_table_rgb = array_range(p_h_colour_table, RGB, 0, n_elements);

    for(index = 0; index < n_elements; ++index, ++p_table_rgb)
    {
        if((p_table_rgb->r == p_rgb->r) && (p_table_rgb->g == p_rgb->g) && (p_table_rgb->b == p_rgb->b))
        {
            *p_found = TRUE;
            return(index);
        }
    }

    *p_found = FALSE;
    return(0);
}

/*
initialise a LOAD_CELL_FOREIGN block
*/

static void inline
load_cell_foreign_init(
    _InoutRef_  P_RTF_LOAD_INFO p_rtf_load_info,
    _OutRef_    P_LOAD_CELL_FOREIGN p_load_cell_foreign,
    _In_z_      PC_USTR_INLINE ustr_inline_contents)
{
    const P_DOCU p_docu = p_docu_from_docno(p_rtf_load_info->docno);
    zero_struct_ptr(p_load_cell_foreign);
    consume_bool(object_data_from_position(p_docu, &p_load_cell_foreign->object_data, &p_rtf_load_info->position, P_OBJECT_POSITION_NONE));
    /* turns out this is abused */
    if(OBJECT_ID_TEXT == p_load_cell_foreign->object_data.object_position_start.object_id)
    {
        p_load_cell_foreign->object_data.object_position_end.object_id = p_load_cell_foreign->object_data.object_position_start.object_id;
        p_load_cell_foreign->object_data.object_position_end.data = p_load_cell_foreign->object_data.object_position_start.data;
        p_load_cell_foreign->object_data.object_position_start.data = 0;
    }
    p_load_cell_foreign->original_slr.col = p_rtf_load_info->position.slr.col - p_rtf_load_info->load_position.slr.col;
    p_load_cell_foreign->original_slr.row = p_rtf_load_info->position.slr.row - p_rtf_load_info->load_position.slr.row;
    p_load_cell_foreign->data_type = OWNFORM_DATA_TYPE_TEXT;
    p_load_cell_foreign->ustr_inline_contents = ustr_inline_contents;
}

/******************************************************************************
*
* Actually do the loading
*
******************************************************************************/

_Check_return_
static STATUS
rtf_load_load_file(
    _InoutRef_  P_RTF_LOAD_INFO p_rtf_load_info,
    P_ARRAY_HANDLE p_h_data)
{
    STATUS status = STATUS_OK;

    p_rtf_load_info->bracket_level = 0;

    skipping = 0;

    /* Initialise static array handles */
    p_rtf_load_info->h_colour_table_load = 0;
    p_rtf_load_info->h_rtf_font_table = 0;
    p_rtf_load_info->h_stylesheet_load = 0;
    p_rtf_load_info->h_da_list = 0;  /* region list */
    p_rtf_load_info->h_footnotes = 0;
    p_rtf_load_info->h_table_widths = 0;
    p_rtf_load_info->grid_colour = 1;
    p_rtf_load_info->tab_type = TAB_LEFT;
    p_rtf_load_info->_trleft = DEFAULT_TABLE_INDENT;

    p_rtf_load_info->destination = RTF_DESTINATION_NORMAL;

    state_style_init(&p_rtf_load_info->state_style);

    zero_struct(p_rtf_load_info->colortbl_rgb);
    p_rtf_load_info->fonttbl_h_rtf_font_name_tstr = 0;
    p_rtf_load_info->fonttbl_font_number = RTF_FONT_NUMBER_INVALID; /* do get \f0 */
    p_rtf_load_info->stylesheet_h_style_name_tstr = 0;
    p_rtf_load_info->stylesheet_style_number = 0; /* \s0 default is OK */

    {
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(ARRAY_HANDLE), 1);
    status_return(al_array_alloc_zero(&p_rtf_load_info->h_da_list, &array_init_block));
    status_return(al_array_alloc_zero(&p_rtf_load_info->h_footnotes, &array_init_block));
    } /*block*/

    {
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(RTF_TABLE), 1);
    status_return(al_array_alloc_zero(&p_rtf_load_info->h_table_widths, &array_init_block));
    } /*block*/

    p_rtf_load_info->p_u8 = array_basec(p_h_data, U8Z);

    status = rtf_load_document_style(p_rtf_load_info);

    while(status_ok(status))
    {
        U8 rtf_char = rtf_get_char(p_rtf_load_info);

        switch(rtf_char)
        {
        case CH_NULL:
            assert(0 == p_rtf_load_info->bracket_level);
            p_rtf_load_info->bracket_level = -1; /* the one time we actually expect to reach the end! */
            break;

        case CH_LEFT_CURLY_BRACKET:
            {
            ARRAY_HANDLE h_content = 0;

            p_rtf_load_info->bracket_level++; /*->1*/

            assert(rtf_peek_char_off(p_rtf_load_info, 0) == CH_BACKWARDS_SLASH);    /* expecting \rtf1 */
            assert(rtf_peek_char_off(p_rtf_load_info, 1) != CH_ASTERISK);           /* \* can't be at top level! */

            status = rtf_load_process_group(p_rtf_load_info, &h_content);

            /* shouldn't have accumulated any content at this level */
            assert(0 == array_elements(&h_content));
            al_array_dispose(&h_content);

            break;
            }

        default: default_unhandled();
#if CHECKING
        case LF:
        case CR:
            /* These are ignored */
#endif
            /* top level - should only encounter an opening bracket - discard everything else */
            break;
        }

        if(p_rtf_load_info->bracket_level < 0)
            break;
    }

    assert(0 == style_selector_any(&p_rtf_load_info->state_style.style.selector));
    style_dispose(&p_rtf_load_info->state_style.style);

    status_return(status);

    return(rtf_load_regions_to_document(p_rtf_load_info));
}

/*
* Turn all regions in h_da_list into docu_areas in our document.
*/

_Check_return_
static STATUS
rtf_load_region_to_document(
    _InoutRef_  P_RTF_LOAD_INFO p_rtf_load_info,
    _InoutRef_  P_RTF_REGION p_rtf_region)
{
    const P_DOCU p_docu = p_docu_from_docno(p_rtf_load_info->docno);
    STATUS status = STATUS_OK;
    const P_STATE_STYLE p_state_style = &p_rtf_region->state_style;
    const P_STYLE p_style = &p_state_style->style;
    /* Apply this as a docu_area */
    STYLE_DOCU_AREA_ADD_PARM style_docu_area_add_parm;

    if(!style_selector_any(&p_style->selector))
        return(STATUS_OK);

    if(style_bit_test(p_style, STYLE_SW_NAME))
    {
        PCTSTR style_name = array_tstr(&p_style->h_style_name_tstr);
        STYLE_HANDLE style_handle = style_handle_from_name(p_docu, style_name);
        if(STYLE_HANDLE_NONE != style_handle)
        {
            STYLE_DOCU_AREA_ADD_HANDLE(&style_docu_area_add_parm, style_handle);
            status = style_docu_area_add(p_docu, &p_rtf_region->docu_area, &style_docu_area_add_parm);
        }
        al_array_dispose(&p_style->h_style_name_tstr);
        style_selector_bit_clear(&p_style->selector, STYLE_SW_NAME);
    }
    else
    {
        STYLE_DOCU_AREA_ADD_STYLE(&style_docu_area_add_parm, p_style);
        status = style_docu_area_add(p_docu, &p_rtf_region->docu_area, &style_docu_area_add_parm);
    }

    return(status);
}

#if 0

_Check_return_
static STATUS
rtf_coalesce_regions(
    _InoutRef_  P_RTF_LOAD_INFO p_rtf_load_info)
{
    STATUS status = STATUS_OK;
    ARRAY_INDEX i;

    for(i = 0; i < array_elements(&p_rtf_load_info->h_da_list); ++i)
    {
        const P_ARRAY_HANDLE p_sub_regions = array_ptr(&p_rtf_load_info->h_da_list, ARRAY_HANDLE, i);
        const ARRAY_INDEX n_sub_regions = array_elements(p_sub_regions);
        ARRAY_INDEX index_o = 0;
        ARRAY_INDEX index_i = 1;

        while(index_i < n_sub_regions)
        {
            P_RTF_REGION p_rtf_region_o = array_ptr(p_sub_regions, RTF_REGION, index_o);
            P_RTF_REGION p_rtf_region_i = array_ptr(p_sub_regions, RTF_REGION, index_i);

            /* if the two style regions apply to the same area and contain no style in common, merge them */
            if(!docu_area_equal(&p_rtf_region_o->docu_area, &p_rtf_region_i->docu_area))
            {
                tracef(TRACE_IO_RTF, "i%d DA[%d] DA[%d] NOT equal", i, index_o, index_i);
                index_i++;
                index_o = index_i - 1;
                continue;
            }

            tracef(TRACE_IO_RTF, "i%d DA[%d] DA[%d] equal", i, index_o, index_i);

            if(style_selector_test(&p_rtf_region_o->state_style.style.selector, &p_rtf_region_i->state_style.style.selector))
            {
                tracef(TRACE_IO_RTF, "i%d NO merge SSB[%d]0x%08X%08X with SSB[%d]0x%08X%08X", i,
                    index_o, p_rtf_region_o->state_style.style.selector.bitmap[1], p_rtf_region_o->state_style.style.selector.bitmap[0],
                    index_i, p_rtf_region_i->state_style.style.selector.bitmap[1], p_rtf_region_i->state_style.style.selector.bitmap[0]);
                index_i++;
                index_o = index_i - 1;
                continue;
            }

            tracef(TRACE_IO_RTF, "i%d merge SSB[%d]0x%08X%08X with SSB[%d]0x%08X%08X", i,
                index_o, p_rtf_region_o->state_style.style.selector.bitmap[1], p_rtf_region_o->state_style.style.selector.bitmap[0],
                index_i, p_rtf_region_i->state_style.style.selector.bitmap[1], p_rtf_region_i->state_style.style.selector.bitmap[0]);

            style_copy(&p_rtf_region_o->state_style.style, &p_rtf_region_i->state_style.style, &p_rtf_region_i->state_style.style.selector);
            state_style_init(&p_rtf_region_i->state_style);

            style_init(&p_rtf_region_i->state_style.style);

            tracef(TRACE_IO_RTF, "i%d       SSB[%d]0x%08X%08X", i,
                index_i, p_rtf_region_o->state_style.style.selector.bitmap[1], p_rtf_region_o->state_style.style.selector.bitmap[0]);

            index_i++;
        }
    }

    return(status);
}

#else

#define rtf_coalesce_regions(p_rtf_load_info) /*EMPTY*/

#endif

_Check_return_
static STATUS
rtf_load_regions_to_document(
    _InoutRef_  P_RTF_LOAD_INFO p_rtf_load_info)
{
    STATUS status = STATUS_OK;
    S32 regions_done = 0;
    ARRAY_INDEX i;

    rtf_coalesce_regions(p_rtf_load_info);

    i = array_elements(&p_rtf_load_info->h_da_list);

    while(--i >= 0) /* This MUST go backwards ! */
    {
        const P_ARRAY_HANDLE p_sub_regions = array_ptr(&p_rtf_load_info->h_da_list, ARRAY_HANDLE, i);
        const ARRAY_INDEX n_sub_regions = array_elements(p_sub_regions);
        ARRAY_INDEX index;

        for(index = 0; index < n_sub_regions; index++) /* forwards or backwards ? */
        {
            P_RTF_REGION p_rtf_region = array_ptr(p_sub_regions, RTF_REGION, index);

            trace_4(TRACE_IO_RTF, TEXT("i%d SSB[%d]0x%08X%08X load"),
                i, index, p_rtf_region->state_style.style.selector.bitmap[1], p_rtf_region->state_style.style.selector.bitmap[0]);

            if(status_ok(status))
                status = rtf_load_region_to_document(p_rtf_load_info, p_rtf_region);

            regions_done++;
        }

        { /* Update process status to show percentage decoded */
        S32 percent = 90 + (regions_done * 10 / p_rtf_load_info->doc_regions);
        p_rtf_load_info->process_status.data.percent.current = percent;
        process_status_reflect(&p_rtf_load_info->process_status);
        } /*block*/

        al_array_dispose(p_sub_regions);
    }

    al_array_dispose(&p_rtf_load_info->h_da_list);

    return(status);
}

/*
* recursively called procedure to handle group { } brackets in RTF files.
*/

_Check_return_
static STATUS
rtf_load_process_group(
    _InoutRef_  P_RTF_LOAD_INFO p_rtf_load_info,
    _InoutRef_  P_ARRAY_HANDLE p_h_dest_contents /*appended*/)
{
#if TRACE_ALLOWED
    PC_U8 group_start = p_rtf_load_info->p_u8 - 1; /* point to the opening curly bracket */
#endif
    STATUS       status = STATUS_OK;
    const S32 initial_bracket_level = p_rtf_load_info->bracket_level; /* incremented by caller, decremented by us */
    ARRAY_HANDLE h_sub_regions = 0; /* List of all regions started within this group */
    BOOL         ignorable_destination_group = FALSE;
    BOOL         awaiting_destination = FALSE;
    RTF_DESTINATION prev_destination = p_rtf_load_info->destination;
    STATE_STYLE  prev_state_style = p_rtf_load_info->state_style;

    {
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(2, sizeof32(RTF_REGION), 1);
    status_assert(status = al_array_alloc_zero(&h_sub_regions, &array_init_block));
    }

/* Note : regions need to be applied in reverse order to that in which they are decoded, i.e. sub_sections to
           this one with region changes need to be applied after any in this section */

    state_style_init(&p_rtf_load_info->state_style);

    status = style_duplicate(&p_rtf_load_info->state_style.style, &prev_state_style.style, &prev_state_style.style.selector);

    /* ignorable destination group? */
    if( (CH_BACKWARDS_SLASH == rtf_peek_char_off(p_rtf_load_info, 0)) &&
        (CH_ASTERISK        == rtf_peek_char_off(p_rtf_load_info, 1)) )
    {
        rtf_advance_char(p_rtf_load_info, 2);

        trace_3(TRACE_IO_RTF, TEXT("RTF(%d): Got ignorable destination group %.*s"), p_rtf_load_info->bracket_level, 12, report_sbstr(group_start));

        ignorable_destination_group = TRUE;

        awaiting_destination = TRUE;

        p_rtf_load_info->destination = RTF_DESTINATION_IGNORABLE;
        trace_2(TRACE_IO_RTF, TEXT("RTF(%d): Set RTF_DESTINATION_%d (IGNORABLE)"), p_rtf_load_info->bracket_level, p_rtf_load_info->destination);

        /* have seen \* immediately followed by CR,LF and then the next keyword - loop handles this */
    }

    if(ignorable_destination_group)
        skipping++;

    while(status_ok(status))
    {
        U8 rtf_char = rtf_get_char(p_rtf_load_info);

        if(rtf_char < CH_SPACE)
        {
            switch(rtf_char)
            {
            case CH_NULL:
                myassert3(TEXT("RTF(%d): EOF read before section %.*s terminated"), p_rtf_load_info->bracket_level, 12, report_sbstr(group_start));
                status = create_error(OB_RTF_ERR_MISMATCHED_BRACKETS);
                rtf_advance_char(p_rtf_load_info, -1);
                p_rtf_load_info->bracket_level = -1;
                break;

            default: default_unhandled();
#if CHECKING
            case LF:
            case CR:
                /* These are ignored */
#endif
                break;
            }
        }
        else
        {
            switch(rtf_char)
            {
            default:
                assert(u8_is_ascii7(rtf_char));
                if(0 != p_rtf_load_info->ignore_chars_count)
                {
                    trace_2(TRACE_IO_RTF, TEXT("RTF(%d): ignore_chars_count=") U32_TFMT, p_rtf_load_info->bracket_level, p_rtf_load_info->ignore_chars_count);
                    p_rtf_load_info->ignore_chars_count--;
                    break;
                }

                if(skipping)
                {   /* don't bother accumulating content for this group */
                    break;
                }

                switch(p_rtf_load_info->destination)
                {
                case RTF_DESTINATION_NORMAL:
                case RTF_DESTINATION_FLDRSLT:
                    if(status_ok(status = rtf_array_add_bytes(p_h_dest_contents, 1, &rtf_char)))
                    {
                        /* Keep a track of how far into the data we are. */
                        p_rtf_load_info->position.object_position.data = array_elements(p_h_dest_contents);
                        p_rtf_load_info->position.object_position.object_id = OBJECT_ID_TEXT;
                    }
                    break;

                case RTF_DESTINATION_COLORTBL:
                    status = rtf_destination_colortbl_char(p_rtf_load_info, rtf_char);
                    break;

                case RTF_DESTINATION_FONTTBL:
                    status = rtf_destination_fonttbl_char(p_rtf_load_info, rtf_char);
                    break;

                case RTF_DESTINATION_STYLESHEET:
                    status = rtf_destination_stylesheet_char(p_rtf_load_info, rtf_char);
                    break;

                default: default_unhandled();
#if CHECKING
                case RTF_DESTINATION_AUTHOR:
                case RTF_DESTINATION_FIELD:
                case RTF_DESTINATION_FLDINST:
                case RTF_DESTINATION_INFO:
                case RTF_DESTINATION_MMATHPR:
                case RTF_DESTINATION_PICT:
                case RTF_DESTINATION_IGNORABLE:
#endif
                    break;
                }
                break;

            case CH_LEFT_CURLY_BRACKET:
                p_rtf_load_info->bracket_level++;

                status = rtf_load_process_group(p_rtf_load_info, p_h_dest_contents);
                break;

            case CH_RIGHT_CURLY_BRACKET:
                p_rtf_load_info->bracket_level--;
                break;

            case CH_BACKWARDS_SLASH:
                {
                BOOL ignorable_destination = ignorable_destination_group;
                U32 parameter_offset;
                QUICK_BLOCK_WITH_BUFFER(quick_block, 32);
                quick_block_with_buffer_setup(quick_block);

                status = rtf_load_obtain_control_word(p_rtf_load_info, &quick_block, &parameter_offset, &ignorable_destination);

                if(status_ok(status))
                {
                    PC_A7STR a7str_control_word = quick_block_str(&quick_block);
                    PC_A7STR a7str_parameter = parameter_offset ? a7str_control_word + parameter_offset : "";
                    PC_RTF_CONTROL_WORD p_rtf_control_word;
                    RTF_CONTROL_WORD_TABLE_SEARCH rtf_control_word_table_search;

                    rtf_control_word_table_search.a7str_control_word = a7str_control_word;
                    rtf_control_word_table_search.control_word_length = parameter_offset ? (parameter_offset - 1) : strlen32(rtf_control_word_table_search.a7str_control_word);

                    p_rtf_control_word = (PC_RTF_CONTROL_WORD)
                        bsearch(&rtf_control_word_table_search,
                                &rtf_control_words, elemof(rtf_control_words), sizeof(rtf_control_words[0]),
                                compare_rtf_control_word);

                    if(skipping)
                    {
                        trace_5(TRACE_IO_RTF, TEXT("RTF(%d): Skipping control word \\%s %s in %.*s"), p_rtf_load_info->bracket_level, report_sbstr(a7str_control_word), report_sbstr(a7str_parameter), 12, report_sbstr(group_start));
                    }
                    else if(NULL == p_rtf_control_word)
                    {
                        if(ignorable_destination)
                        {
                            trace_5(TRACE_IO_RTF, TEXT("RTF(%d): Ignoring unhandled ignorable destination control word \\%s %s in %.*s"), p_rtf_load_info->bracket_level, report_sbstr(a7str_control_word), report_sbstr(a7str_parameter), 12, report_sbstr(group_start));
                        }
                        else
                        {
                            trace_5(TRACE_IO_RTF, TEXT("RTF(%d): Ignoring unhandled control word \\%s %s in %.*s"), p_rtf_load_info->bracket_level, report_sbstr(a7str_control_word), report_sbstr(a7str_parameter), 12, report_sbstr(group_start));
                        }
                    }
                    else
                    {
                        if(ignorable_destination)
                        {
                            trace_5(TRACE_IO_RTF, TEXT("RTF(%d): Ignoring ignorable destination control word \\%s %s in %.*s"), p_rtf_load_info->bracket_level, report_sbstr(a7str_control_word), report_sbstr(a7str_parameter), 12, report_sbstr(group_start));

                            status =
                                (rtf_control_ignore) (
                                    p_rtf_load_info, a7str_control_word, a7str_parameter,
                                    p_h_dest_contents, &h_sub_regions,
                                    p_rtf_control_word->arg);
                        }
                        else
                        {
                            trace_5(TRACE_IO_RTF, TEXT("RTF(%d): Processing control word \\%s %s in %.*s"), p_rtf_load_info->bracket_level, report_sbstr(a7str_control_word), report_sbstr(a7str_parameter), 12, report_sbstr(group_start));

                            status =
                                (* p_rtf_control_word->p_control_proc) (
                                    p_rtf_load_info, a7str_control_word, a7str_parameter,
                                    p_h_dest_contents, &h_sub_regions,
                                    p_rtf_control_word->arg);
                        }
                    }
                }

                quick_block_dispose(&quick_block);
                break;
                }
            }
        }

        if(p_rtf_load_info->bracket_level < initial_bracket_level)
            break;
    }

    if(ignorable_destination_group)
        skipping--;

    if(status_ok(status))
    {
        status = STATUS_OK;

        /* If h_sub_regions contains any regions, save them to h_da_list */
        if(array_elements(&h_sub_regions) != 0)
        {
            const ARRAY_INDEX n_sub_regions = array_elements(&h_sub_regions);
            ARRAY_INDEX i;
            P_RTF_REGION p_rtf_region = array_range(&h_sub_regions, RTF_REGION, 0, n_sub_regions);

            /* first, close any open-ended regions */
            for(i = 0; i < n_sub_regions; ++i, ++p_rtf_region)
            {
                if(p_rtf_region->closed)
                    continue;

                p_rtf_region->closed = 1;

                p_rtf_region->docu_area.br = p_rtf_load_info->position;
                if(p_rtf_region->docu_area.tl.slr.row == p_rtf_region->docu_area.br.slr.row)
                    p_rtf_region->docu_area.br.slr.row++;
                if(p_rtf_region->docu_area.tl.slr.col == p_rtf_region->docu_area.br.slr.col)
                    p_rtf_region->docu_area.br.slr.col++;
                if((OBJECT_ID_NONE == p_rtf_region->docu_area.tl.object_position.object_id) && (0 == p_rtf_region->docu_area.tl.slr.col))
                    p_rtf_region->docu_area.br.slr.col += p_rtf_load_info->doc_cols;

                p_rtf_load_info->doc_regions++;
            }

            status = al_array_add(&p_rtf_load_info->h_da_list, ARRAY_HANDLE, 1, PC_ARRAY_INIT_BLOCK_NONE, &h_sub_regions);
        }
        else
            al_array_dispose(&h_sub_regions); /* Release zero-element handles */
    }

/*dump:;*/
    style_free_resources_all(&p_rtf_load_info->state_style.style);

    p_rtf_load_info->state_style = prev_state_style;

    if(p_rtf_load_info->destination != prev_destination)
    {
        trace_2(TRACE_IO_RTF, TEXT("RTF(%d): Leaving RTF_DESTINATION_%d"), p_rtf_load_info->bracket_level, p_rtf_load_info->destination);
        p_rtf_load_info->destination = prev_destination;
        trace_2(TRACE_IO_RTF, TEXT("RTF(%d): Back to RTF_DESTINATION_%d"), p_rtf_load_info->bracket_level, p_rtf_load_info->destination);
    }

    return(status);
}

/******************************************************************************
*
* \author (destination) (but non-ignorable) : discard all data
*
******************************************************************************/

PROC_RTF_CONTROL_PROTO(rtf_destination_author)
{
    func_ignore_parms();

    p_rtf_load_info->destination = RTF_DESTINATION_AUTHOR;
    trace_2(TRACE_IO_RTF, TEXT("RTF(%d): Set RTF_DESTINATION_%d (AUTHOR)"), p_rtf_load_info->bracket_level, p_rtf_load_info->destination);

    return(STATUS_OK);
}

/******************************************************************************
*
* \colortbl (destination)
*
******************************************************************************/

PROC_RTF_CONTROL_PROTO(rtf_destination_colortbl)
{
    func_ignore_parms();

    p_rtf_load_info->destination = RTF_DESTINATION_COLORTBL;
    trace_2(TRACE_IO_RTF, TEXT("RTF(%d): Set RTF_DESTINATION_%d (COLORTBL)"), p_rtf_load_info->bracket_level, p_rtf_load_info->destination);

    zero_struct(p_rtf_load_info->colortbl_rgb);

    /* should have a semicolon to denote 'auto' entry zero */

    return(STATUS_OK);
}

_Check_return_
static STATUS
rtf_destination_colortbl_element_complete(
    _InoutRef_  P_RTF_LOAD_INFO p_rtf_load_info)
{
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(4, sizeof32(RGB), FALSE);
    STATUS status;

    status = al_array_add(&p_rtf_load_info->h_colour_table_load, RGB, 1, &array_init_block, &p_rtf_load_info->colortbl_rgb);

    zero_struct(p_rtf_load_info->colortbl_rgb);

    return(status);
}

_Check_return_
static STATUS
rtf_destination_colortbl_char(
    _InoutRef_  P_RTF_LOAD_INFO p_rtf_load_info,
    _InVal_     U8 rtf_char)
{
    switch(rtf_char)
    {
    case CH_SEMICOLON:
        return(rtf_destination_colortbl_element_complete(p_rtf_load_info));

    default: default_unhandled();
        return(STATUS_OK);
    }
}

/******************************************************************************
*
* \field: (destination) insert dynamic field result
*
******************************************************************************/

PROC_RTF_CONTROL_PROTO(rtf_destination_field)
{
    func_ignore_parms();

    p_rtf_load_info->destination = RTF_DESTINATION_FIELD;
    trace_2(TRACE_IO_RTF, TEXT("RTF(%d): Set RTF_DESTINATION_%d (FIELD)"), p_rtf_load_info->bracket_level, p_rtf_load_info->destination);

    return(STATUS_OK);
}

/******************************************************************************
*
* \fldinst (destination) (ignorable) : discard all data
*
******************************************************************************/

PROC_RTF_CONTROL_PROTO(rtf_destination_fldinst)
{
    func_ignore_parms();

    p_rtf_load_info->destination = RTF_DESTINATION_FLDINST;
    trace_2(TRACE_IO_RTF, TEXT("RTF(%d): Set RTF_DESTINATION_%d (FLDINST)"), p_rtf_load_info->bracket_level, p_rtf_load_info->destination);

    return(STATUS_OK);
}

/******************************************************************************
*
* \fldrslt: (destination) decode static field result
*
******************************************************************************/

PROC_RTF_CONTROL_PROTO(rtf_destination_fldrslt)
{
    func_ignore_parms();

    p_rtf_load_info->destination = RTF_DESTINATION_FLDRSLT;
    trace_2(TRACE_IO_RTF, TEXT("RTF(%d): Set RTF_DESTINATION_%d (FLDRSLT)"), p_rtf_load_info->bracket_level, p_rtf_load_info->destination);

    return(STATUS_OK);
}

/******************************************************************************
*
* \fonttbl (destination)
*
******************************************************************************/

PROC_RTF_CONTROL_PROTO(rtf_destination_fonttbl)
{
    func_ignore_parms();

    p_rtf_load_info->destination = RTF_DESTINATION_FONTTBL;
    trace_2(TRACE_IO_RTF, TEXT("RTF(%d): Set RTF_DESTINATION_%d (FONTTBL)"), p_rtf_load_info->bracket_level, p_rtf_load_info->destination);

    return(STATUS_OK);
}

_Check_return_
static STATUS
rtf_destination_fonttbl_element_complete(
    _InoutRef_  P_RTF_LOAD_INFO p_rtf_load_info)
{
    STATUS status;

    assert(RTF_FONT_NUMBER_INVALID != p_rtf_load_info->fonttbl_font_number);
    assert(0 != array_elements(&p_rtf_load_info->fonttbl_h_rtf_font_name_tstr));

    {
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(8, sizeof32(TCHAR), FALSE);
    TCHAR tchar = CH_NULL; /* terminate name */
    status = al_array_add(&p_rtf_load_info->fonttbl_h_rtf_font_name_tstr, TCHAR, 1, &array_init_block, &tchar);
    } /*block*/

    if(status_ok(status))
    {
        SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(RTF_FONT_TABLE), FALSE);
        P_RTF_FONT_TABLE p_rtf_font_table;

        if(NULL != (p_rtf_font_table = al_array_extend_by(&p_rtf_load_info->h_rtf_font_table, RTF_FONT_TABLE, 1, &array_init_block, &status)))
        {
            p_rtf_font_table->font_number = p_rtf_load_info->fonttbl_font_number;
            p_rtf_font_table->h_rtf_font_name_tstr = p_rtf_load_info->fonttbl_h_rtf_font_name_tstr; /* donate */
            p_rtf_load_info->fonttbl_h_rtf_font_name_tstr = 0;
        }
    }

    al_array_dispose(&p_rtf_load_info->fonttbl_h_rtf_font_name_tstr);
    p_rtf_load_info->fonttbl_font_number = RTF_FONT_NUMBER_INVALID;

    return(status);
}

_Check_return_
static STATUS
rtf_destination_fonttbl_char(
    _InoutRef_  P_RTF_LOAD_INFO p_rtf_load_info,
    _InVal_     U8 rtf_char)
{
    switch(rtf_char)
    {
    case CH_SEMICOLON:
        return(rtf_destination_fonttbl_element_complete(p_rtf_load_info));

    default:
        {
        SC_ARRAY_INIT_BLOCK array_init_block = aib_init(8, sizeof32(TCHAR), FALSE);

        TCHAR tchar = rtf_char;

        assert(u8_is_ascii7(rtf_char));

        return(al_array_add(&p_rtf_load_info->fonttbl_h_rtf_font_name_tstr, TCHAR, 1, &array_init_block, &tchar));
        }
    }
}

/******************************************************************************
*
* \info (destination) (but non-ignorable) : discard all data
*
******************************************************************************/

PROC_RTF_CONTROL_PROTO(rtf_destination_info)
{
    func_ignore_parms();

    p_rtf_load_info->destination = RTF_DESTINATION_INFO;
    trace_2(TRACE_IO_RTF, TEXT("RTF(%d): Set RTF_DESTINATION_%d (INFO)"), p_rtf_load_info->bracket_level, p_rtf_load_info->destination);

    return(STATUS_OK);
}

/******************************************************************************
*
* \mmathPr (destination) (but non-ignorable) : discard all data
*
******************************************************************************/

PROC_RTF_CONTROL_PROTO(rtf_destination_mmathPr)
{
    func_ignore_parms();

    p_rtf_load_info->destination = RTF_DESTINATION_MMATHPR;
    trace_2(TRACE_IO_RTF, TEXT("RTF(%d): Set RTF_DESTINATION_%d (MMATHPR)"), p_rtf_load_info->bracket_level, p_rtf_load_info->destination);

    return(STATUS_OK);
}

/******************************************************************************
*
* \pict (destination) (but non-ignorable) : discard all data
*
******************************************************************************/

PROC_RTF_CONTROL_PROTO(rtf_destination_pict)
{
    func_ignore_parms();

    p_rtf_load_info->destination = RTF_DESTINATION_PICT;
    trace_2(TRACE_IO_RTF, TEXT("RTF(%d): Set RTF_DESTINATION_%d (PICT)"), p_rtf_load_info->bracket_level, p_rtf_load_info->destination);

    return(STATUS_OK);
}

/******************************************************************************
*
* \stylesheet (destination)
*
******************************************************************************/

PROC_RTF_CONTROL_PROTO(rtf_destination_stylesheet)
{
    func_ignore_parms();

    p_rtf_load_info->destination = RTF_DESTINATION_STYLESHEET;
    trace_2(TRACE_IO_RTF, TEXT("RTF(%d): Set RTF_DESTINATION_%d (STYLESHEET)"), p_rtf_load_info->bracket_level, p_rtf_load_info->destination);

    /* doesn't inherit any state_style */
    style_free_resources_all(&p_rtf_load_info->state_style.style);
    state_style_init(&p_rtf_load_info->state_style);

    /* \s<n> is optional */
    p_rtf_load_info->stylesheet_style_number = 0;

    return(STATUS_OK);
}

_Check_return_
static STATUS
rtf_destination_stylesheet_element_complete(
    _InoutRef_  P_RTF_LOAD_INFO p_rtf_load_info)
{
    SC_ARRAY_INIT_BLOCK stylesheet_init_block = aib_init(1, sizeof32(RTF_STYLESHEET), FALSE);
    STATUS status;

    {
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(8, sizeof32(TCHAR), FALSE);
    TCHAR tchar = CH_NULL; /* terminate name */
    status = al_array_add(&p_rtf_load_info->stylesheet_h_style_name_tstr, TCHAR, 1, &array_init_block, &tchar);
    } /*block*/

    if(status_ok(status))
    {
        PCTSTR tstr_style_name;

        if(status_ok(status = al_tstr_set(&p_rtf_load_info->state_style.style.h_style_name_tstr, array_tstr(&p_rtf_load_info->stylesheet_h_style_name_tstr))))
            style_bit_set(&p_rtf_load_info->state_style.style, STYLE_SW_NAME);

        /* If stylename starts Base.., change it since not all attributes will
            be set, and it may override our document basestyle. */

        tstr_style_name = array_tstr(&p_rtf_load_info->state_style.style.h_style_name_tstr);
        if(0 == tstrnicmp(tstr_style_name, TEXT("base"), 4))
        {
            PTSTR tstr_style_name_wr = de_const_cast(PTSTR, tstr_style_name);
            tstr_style_name_wr[0] = 'R';
            tstr_style_name_wr[1] = 'T';
            tstr_style_name_wr[2] = 'F';
            tstr_style_name_wr[3] = CH_UNDERSCORE;
        }

        /* Now create the style */
        status = style_handle_add(p_docu_from_docno(p_rtf_load_info->docno), &p_rtf_load_info->state_style.style);
        /* status = style_handle */
        if(status_ok(status))
            state_style_init(&p_rtf_load_info->state_style); /* donated */

        if(status_ok(status))
        {   /* Put style number and name into h_stylesheet_load */
            P_RTF_STYLESHEET p_rtf_stylesheet;

            if(NULL != (p_rtf_stylesheet = al_array_extend_by(&p_rtf_load_info->h_stylesheet_load, RTF_STYLESHEET, 1, &stylesheet_init_block, &status)))
            {
                p_rtf_stylesheet->style_number = p_rtf_load_info->stylesheet_style_number;
                p_rtf_stylesheet->h_style_name_tstr = p_rtf_load_info->stylesheet_h_style_name_tstr; /* donate */
                p_rtf_load_info->stylesheet_h_style_name_tstr = 0;
            }
        }
    }

    style_free_resources_all(&p_rtf_load_info->state_style.style);
    state_style_init(&p_rtf_load_info->state_style);

    al_array_dispose(&p_rtf_load_info->stylesheet_h_style_name_tstr);
    p_rtf_load_info->stylesheet_style_number = 0;

    return(status);
}

_Check_return_
static STATUS
rtf_destination_stylesheet_char(
    _InoutRef_  P_RTF_LOAD_INFO p_rtf_load_info,
    _InVal_     U8 rtf_char)
{
    switch(rtf_char)
    {
    case CH_SEMICOLON:
        return(rtf_destination_stylesheet_element_complete(p_rtf_load_info));

    default:
        {
        SC_ARRAY_INIT_BLOCK array_init_block = aib_init(8, sizeof32(TCHAR), FALSE);

        TCHAR tchar = rtf_char;

        assert(u8_is_ascii7(rtf_char));

        return(al_array_add(&p_rtf_load_info->stylesheet_h_style_name_tstr, TCHAR, 1, &array_init_block, &tchar));
        }
    }
}

/******************************************************************************
*
* Scan through an RTF file to find the number of rows for docu_area_insert
*
******************************************************************************/

_Check_return_
static STATUS
rtf_load_find_document_dimensions(
    _InoutRef_  P_RTF_LOAD_INFO p_rtf_load_info,
    _InRef_     P_ARRAY_HANDLE p_h_data,
    _OutRef_    P_DOCU_AREA p_docu_area)
{
    STATUS status = STATUS_OK;
    S32 cur_cols = 0;
    S32 max_cols = 0;
    S32 max_rows = 0;

    docu_area_init(p_docu_area);

    p_rtf_load_info->p_u8 = array_basec(p_h_data, U8Z);

    p_rtf_load_info->bracket_level = 0;

    while(status_ok(status))
    {
        U8 rtf_char = rtf_get_char(p_rtf_load_info);

        if(rtf_char < CH_SPACE)
        {
            switch(rtf_char)
            {
            case CH_NULL:
                if(p_rtf_load_info->bracket_level > 0)
                {
                    myassert1(TEXT("RTF(%d): EOF read before document terminated"), p_rtf_load_info->bracket_level);
                    status = create_error(OB_RTF_ERR_MISMATCHED_BRACKETS);
                }
                rtf_advance_char(p_rtf_load_info, -1);
                p_rtf_load_info->bracket_level = -1;
                break;

            default:
                /* Ignore all other stuff */
                assert(rtf_char < 0x7F);
                break;
            }
        }
        else
        {
            switch(rtf_char)
            {
            default:
                /* Ignore all other stuff */
                assert(rtf_char < 0x7F);
                break;

            case CH_LEFT_CURLY_BRACKET:
                p_rtf_load_info->bracket_level++;

                if( (CH_BACKWARDS_SLASH == rtf_peek_char_off(p_rtf_load_info, 0)) &&
                    (CH_ASTERISK        == rtf_peek_char_off(p_rtf_load_info, 1)) )
                {   /* ignorable destination group */
                    trace_3(TRACE_IO_RTF, TEXT("RTF(%d): Got ignorable destination group %.*s"), p_rtf_load_info->bracket_level, 12, report_sbstr(p_rtf_load_info->p_u8 - 1));

                    rtf_advance_char(p_rtf_load_info, 2);

                    /* have seen \* immediately followed by CR,LF and then the next keyword - loop handles this */
                }
                break;

            case CH_RIGHT_CURLY_BRACKET:
                p_rtf_load_info->bracket_level--;
                if(p_rtf_load_info->bracket_level < 0)
                {
                    myassert2(TEXT("RTF(%d): too many closing curly brackets - %u bytes unused"), p_rtf_load_info->bracket_level, strlen32(p_rtf_load_info->p_u8));
                    status = create_error(OB_RTF_ERR_MISMATCHED_BRACKETS);
                }
                break;

            case CH_BACKWARDS_SLASH:
                {
                BOOL ignorable_destination = FALSE;
                U32 parameter_offset;
                QUICK_BLOCK_WITH_BUFFER(quick_block, 32);
                quick_block_with_buffer_setup(quick_block);

                status = rtf_load_obtain_control_word(p_rtf_load_info, &quick_block, &parameter_offset, &ignorable_destination);

                if(status_ok(status))
                {
                    PC_A7STR a7str_control_word = quick_block_str(&quick_block);
                    const U32 control_word_length = parameter_offset ? (parameter_offset - 1) : strlen32(a7str_control_word);

                    if((3 == control_word_length) && (0 == memcmp32(a7str_control_word, "par", 3)))
                        max_rows++;

                    else if((3 == control_word_length) && (0 == memcmp32(a7str_control_word, "row", 3)))
                    {
                        if(max_cols < cur_cols)
                            max_cols = cur_cols;
                        cur_cols = 0;
                        max_rows++;
                    }

                    else if((4 == control_word_length) && (0 == memcmp32(a7str_control_word, "cell", 4)))
                        cur_cols++;

                    else if((8 == control_word_length) && (0 == memcmp32(a7str_control_word, "footnote", 8)))
                        max_rows++;
                }

                quick_block_dispose(&quick_block);
                break;
                }
            }
        }

        if(p_rtf_load_info->bracket_level < 0)
            break;
    }

    if(max_rows == 0)
        max_rows = 1;

    if(max_cols == 0)
        max_cols = 1;

    p_docu_area->tl.object_position.object_id = OBJECT_ID_NONE;
    p_docu_area->tl.slr.col = 0;
    p_docu_area->tl.slr.row = 0;
    p_docu_area->br.object_position.object_id = OBJECT_ID_NONE;
    p_docu_area->br.slr.col = (COL) max_cols;
    p_docu_area->br.slr.row = (ROW) max_rows;

    return(status);
}

/******************************************************************************
*
* Loads the next RTF control word
*
******************************************************************************/

_Check_return_
static STATUS
rtf_load_obtain_control_word_special(
    _InoutRef_  P_RTF_LOAD_INFO p_rtf_load_info,
    _InoutRef_  P_QUICK_BLOCK p_quick_block /*appended,terminated*/,
    _OutRef_    P_U32 p_parameter_offset)
{
    U8Z u8z[8];
    U8 rtf_char = rtf_get_char(p_rtf_load_info);

    u8z[0] = rtf_char;

    u8z[1] = CH_NULL; /* terminate the RTF control word */

    if(CH_APOSTROPHE == rtf_char) /* hex character => only two more chars (hex digits) required */
    {
        *p_parameter_offset = 2; /* parameter is appended after that terminator */

        rtf_char = rtf_get_char(p_rtf_load_info);
        u8z[2] = rtf_char; /*MSN*/

        assert(!/*"C"*/iscntrl(rtf_char));
        if(CH_SPACE > rtf_char)
        {
            rtf_advance_char(p_rtf_load_info, -1);
            return(STATUS_FAIL);
        }

        rtf_char = rtf_get_char(p_rtf_load_info);
        u8z[3] = rtf_char; /*LSN*/

        assert(!/*"C"*/iscntrl(rtf_char));
        if(CH_SPACE > rtf_char)
        {
            rtf_advance_char(p_rtf_load_info, -1);
            return(STATUS_FAIL);
        }

        rtf_char = rtf_peek_char_off(p_rtf_load_info, 0);
        u8z[4] = CH_NULL; /* terminate the parameter */

        if(CH_SPACE >= rtf_char)
        {   /* consume an optional single space (or line sep) as part of the control word (but not added to it) */
            if(CH_NULL != rtf_char)
                rtf_advance_char(p_rtf_load_info, 1);
        }

        return(quick_block_bytes_add(p_quick_block, u8z, 5));
    }

    *p_parameter_offset = 0;

    /* NONE of these others have parameters or take an end delimiter */

    u8z[2] = CH_NULL; /* terminate the absent parameter in case someone looks at it */

    return(quick_block_bytes_add(p_quick_block, u8z, 3));
}

_Check_return_
static STATUS
rtf_load_obtain_control_word(
    _InoutRef_  P_RTF_LOAD_INFO p_rtf_load_info,
    _InoutRef_  P_QUICK_BLOCK p_quick_block /*appended,terminated*/,
    _OutRef_    P_U32 p_parameter_offset,
    _InoutRef_  P_BOOL p_ignorable_destination)
{
    /* p_document_start points to first character after the backslash */
#if TRACE_ALLOWED
    const PC_U8 control_word_start = p_rtf_load_info->p_u8;
#endif
    U8 rtf_char = *p_rtf_load_info->p_u8;

    *p_parameter_offset = 0;

    if( (CH_ASTERISK        == rtf_char) &&
        (CH_BACKWARDS_SLASH == p_rtf_load_info->p_u8[1]) )
    {   /* this is an ignorable destination */
        trace_3(TRACE_IO_RTF, TEXT("RTF(%d): Got ignorable destination %.*s"), p_rtf_load_info->bracket_level, 12, report_sbstr(control_word_start - 1));

        *p_ignorable_destination = TRUE;

        rtf_advance_char(p_rtf_load_info, 2); /* skip the *\ to the start of the associated destination control word */

        rtf_char = *p_rtf_load_info->p_u8;

        while((LF == rtf_char) || (CR == rtf_char)) /* it has been seen... */
        {
            rtf_advance_char(p_rtf_load_info, 1);

            rtf_char = *p_rtf_load_info->p_u8;
        }
    }

    assert(!/*"C"*/iscntrl(rtf_char));
    if(CH_SPACE > rtf_char)
        return(STATUS_FAIL);

    /* some characters need escaping, e.g. as \\, \{, \} */
    /* some are special control symbols e.g. \_, \~, \* */
    if(!/*"C"*/isalpha(rtf_char))
        return(rtf_load_obtain_control_word_special(p_rtf_load_info, p_quick_block, p_parameter_offset));

    /* all normal RTF control words are in lowercase ASCII a-z */
    /* however many newer RTF control words contain uppercase ASCII A-Z */
    /* accumulate [a-zA-Z] RTF control word together with optional numeric parameter and terminate */
    for(;;)
    {
        rtf_char = rtf_get_char(p_rtf_load_info);

        if(/*"C"*/isalpha(rtf_char))
        {
            if(!quick_block_byte_add_fast(p_quick_block, rtf_char))
                status_return(quick_block_byte_add(p_quick_block, rtf_char));
            continue;
        }

        status_return(quick_block_nullch_add(p_quick_block)); /* terminate the RTF control word */

        if(rtf_char <= CH_SPACE)
        {   /* consume an optional single space (or line sep) as part of the control word (but not added to it) */
            assert((CH_SPACE == rtf_char) || (LF == rtf_char) || (CR == rtf_char));
            if(CH_NULL == rtf_char)
                rtf_advance_char(p_rtf_load_info, -1);
            break;
        }

        if(!/*"C"*/isdigit(rtf_char) && (CH_MINUS_SIGN__BASIC != rtf_char))
        {
            rtf_advance_char(p_rtf_load_info, -1); /* leaves us pointing to next character to be processed */
            break;
        }

        /* accumulate (optionally signed) numeric parameter for RTF control word, appended after that terminator */
        *p_parameter_offset = quick_block_bytes(p_quick_block);

        do  {
            status_return(quick_block_byte_add(p_quick_block, rtf_char));

            rtf_char = rtf_get_char(p_rtf_load_info);
        }
        while(/*"C"*/isdigit(rtf_char));

        if(CH_SPACE >= rtf_char)
        {   /* consume an optional single space (or line sep) as part of the control word after the numeric parameter (but not added to it) */
            assert((CH_SPACE == rtf_char) || (LF == rtf_char) || (CR == rtf_char));
            if(CH_NULL == rtf_char)
                rtf_advance_char(p_rtf_load_info, -1);
            break;
        }

        rtf_advance_char(p_rtf_load_info, -1); /* leaves us pointing to next character to be processed */

        break;
    }

    return(quick_block_nullch_add(p_quick_block)); /* terminate the parameter (may be empty) */
}

/******************************************************************************
*
* Process an RTF control word
*
******************************************************************************/

_Check_return_
static STATUS
rtf_load_process_control_word(
    _InoutRef_  P_RTF_LOAD_INFO p_rtf_load_info,
    _In_z_      PC_A7STR a7str_control_word,
    _InVal_     U32 parameter_offset,
    P_ARRAY_HANDLE p_h_cell_contents,
    P_ARRAY_HANDLE p_h_sub_regions,
    _In_z_      PC_A7STR container_control_word,
    BOOL ignorable_destination)
{
    STATUS status;
    PC_A7STR a7str_parameter = parameter_offset ? a7str_control_word + parameter_offset : "";
    PC_RTF_CONTROL_WORD p_rtf_control_word;
    RTF_CONTROL_WORD_TABLE_SEARCH rtf_control_word_table_search;

#if !TRACE_ALLOWED
    UNREFERENCED_PARAMETER(container_control_word);
#endif

    rtf_control_word_table_search.a7str_control_word = a7str_control_word;
    rtf_control_word_table_search.control_word_length = parameter_offset ? (parameter_offset - 1) : strlen32(rtf_control_word_table_search.a7str_control_word);

    p_rtf_control_word = (PC_RTF_CONTROL_WORD)
        bsearch(&rtf_control_word_table_search,
                &rtf_control_words, elemof(rtf_control_words), sizeof(rtf_control_words[0]),
                compare_rtf_control_word);

    if(NULL == p_rtf_control_word)
    {
        if(ignorable_destination)
            trace_4(TRACE_IO_RTF, TEXT("RTF(%d): Ignoring unhandled ignorable destination control word \\%s %s in \\%s"), p_rtf_load_info->bracket_level, report_sbstr(a7str_control_word), report_sbstr(a7str_parameter), report_sbstr(container_control_word));
        else
            trace_4(TRACE_IO_RTF, TEXT("RTF(%d): Ignoring unhandled control word \\%s %s in \\%s"), p_rtf_load_info->bracket_level, report_sbstr(a7str_control_word), report_sbstr(a7str_parameter), report_sbstr(container_control_word));
        return(STATUS_OK);
    }

    assert(NULL != * p_rtf_control_word->p_control_proc);

    trace_4(TRACE_IO_RTF, TEXT("RTF(%d): Processing control word \\%s %s in \\%s"), p_rtf_load_info->bracket_level, report_sbstr(a7str_control_word), report_sbstr(a7str_parameter), report_sbstr(container_control_word));

    status =
        (* p_rtf_control_word->p_control_proc) (
            p_rtf_load_info, a7str_control_word, a7str_parameter,
            p_h_cell_contents, p_h_sub_regions,
            p_rtf_control_word->arg);

    return(status);
}

/******************************************************************************
*
* update document's page info if not inserting
*
******************************************************************************/

_Check_return_
static STATUS
update_from_phys_page_def(
    _InoutRef_  P_RTF_LOAD_INFO p_rtf_load_info,
    _InRef_     PC_PHYS_PAGE_DEF p_phys_page_def)
{
    if(!p_rtf_load_info->inserting)
    {
        const P_DOCU p_docu = p_docu_from_docno(p_rtf_load_info->docno);

        p_docu->phys_page_def = *p_phys_page_def;

        page_def_from_phys_page_def(&p_docu->page_def, &p_docu->phys_page_def, p_docu->paper_scale);
        page_def_validate(&p_docu->page_def);
    }

    return(STATUS_OK);
}

/******************************************************************************
*
* \marg[x]<n>
*
******************************************************************************/

PROC_RTF_CONTROL_PROTO(rtf_control_page_margin)
{
    const STYLE_BIT_NUMBER style_bit_number = (STYLE_BIT_NUMBER) arg;
    S32 parameter = fast_strtol(a7str_parameter, NULL);
    const P_DOCU p_docu = p_docu_from_docno(p_rtf_load_info->docno);
    PHYS_PAGE_DEF phys_page_def = p_docu->phys_page_def;

    func_ignore_parms();

    switch(style_bit_number)
    {
    default: default_unhandled();
    case STYLE_SW_PS_MARGIN_LEFT:  phys_page_def.margin_left   = parameter; break;
    case STYLE_SW_PS_MARGIN_RIGHT: phys_page_def.margin_right  = parameter; break;
    case STYLE_SW_PS_PARA_START:   phys_page_def.margin_top    = parameter; break;
    case STYLE_SW_PS_PARA_END:     phys_page_def.margin_bottom = parameter; break;
    }

    return(update_from_phys_page_def(p_rtf_load_info, &phys_page_def));
}

/******************************************************************************
*
* \paper[x]<n>
*
******************************************************************************/

PROC_RTF_CONTROL_PROTO(rtf_control_paper_size)
{
    S32 parameter = fast_strtol(a7str_parameter, NULL);
    const P_DOCU p_docu = p_docu_from_docno(p_rtf_load_info->docno);
    PHYS_PAGE_DEF phys_page_def = p_docu->phys_page_def;

    func_ignore_parms();

    switch(arg)
    {
    case 0 /*x*/: phys_page_def.size_x = parameter; break;
    default: default_unhandled();
    case 1 /*y*/: phys_page_def.size_y = parameter; break;
    }

    return(update_from_phys_page_def(p_rtf_load_info, &phys_page_def));
}

/******************************************************************************
*
* \landscape
*
******************************************************************************/

PROC_RTF_CONTROL_PROTO(rtf_control_landscape)
{
    const P_DOCU p_docu = p_docu_from_docno(p_rtf_load_info->docno);
    PHYS_PAGE_DEF phys_page_def = p_docu->phys_page_def;

    func_ignore_parms();

    phys_page_def.landscape = 1;

    return(update_from_phys_page_def(p_rtf_load_info, &phys_page_def));
}

_Check_return_
static STATUS
rtf_load_hefo(
    _InoutRef_  P_RTF_LOAD_INFO p_rtf_load_info,
    _In_z_      PC_A7STR a7str_control_word,
    P_HEADFOOT_DEF p_headfoot_def)
{
    const P_DOCU p_docu = p_docu_from_docno(p_rtf_load_info->docno);
    STATUS status;
    const S32 initial_bracket_level = p_rtf_load_info->bracket_level;
    P_ARRAY_HANDLE p_h_hefo_text = &p_headfoot_def->headfoot.h_data;
    ARRAY_HANDLE hefo_region_list;

    {
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(RTF_REGION), 1);
    status_assert(status = al_array_alloc_zero(&hefo_region_list, &array_init_block));
    } /*block*/

    while(status_ok(status))
    {
        U8 rtf_char = rtf_get_char(p_rtf_load_info);

        if(rtf_char < CH_SPACE)
        {
            switch(rtf_char)
            {
            case CH_NULL:
                myassert2(TEXT("RTF(%d): EOF read before \\%s terminated"), p_rtf_load_info->bracket_level, report_sbstr(a7str_control_word));
                status = create_error(OB_RTF_ERR_MISMATCHED_BRACKETS);
                rtf_advance_char(p_rtf_load_info, -1);
                p_rtf_load_info->bracket_level = -1;
                break;

            default: default_unhandled();
#if CHECKING
            case LF:
            case CR:
                /* These are ignored */
#endif
                break;
            }
        }
        else
        {
            switch(rtf_char)
            {
            default:
                assert(u8_is_ascii7(rtf_char));
                if(0 != p_rtf_load_info->ignore_chars_count)
                {
                    trace_2(TRACE_IO_RTF, TEXT("RTF(%d): ignore_chars_count=") U32_TFMT, p_rtf_load_info->bracket_level, p_rtf_load_info->ignore_chars_count);
                    p_rtf_load_info->ignore_chars_count--;
                    break;
                }

                if(p_rtf_load_info->inserting)
                    break;

                if(status_ok(status = rtf_array_add_bytes(p_h_hefo_text, 1, &rtf_char)))
                    p_rtf_load_info->position.object_position.data = array_elements(p_h_hefo_text);
                break;

            case CH_LEFT_CURLY_BRACKET:  p_rtf_load_info->bracket_level++; break;
            case CH_RIGHT_CURLY_BRACKET: p_rtf_load_info->bracket_level--; break;

            case CH_BACKWARDS_SLASH:
                {
                BOOL ignorable_destination = FALSE;
                PC_A7STR container_control_word = a7str_control_word;
                U32 parameter_offset;
                QUICK_BLOCK_WITH_BUFFER(quick_block, 32);
                quick_block_with_buffer_setup(quick_block);

                status = rtf_load_obtain_control_word(p_rtf_load_info, &quick_block, &parameter_offset, &ignorable_destination);

                if(status_ok(status))
                {
                    PC_A7STR inner_control_word = quick_block_str(&quick_block);

                    if(0 == /*"C"*/strcmp(inner_control_word, "par")) /* no paragraphs in hefo ! */
                    {
                        trace_3(TRACE_IO_RTF, TEXT("RTF(%d): Discarding control word \\%s in \\%s"), p_rtf_load_info->bracket_level, report_sbstr(inner_control_word), report_sbstr(a7str_control_word));
                    }
                    else
                    {
                        status = rtf_load_process_control_word(p_rtf_load_info, inner_control_word, parameter_offset,
                                                               p_h_hefo_text, &hefo_region_list,
                                                               container_control_word, ignorable_destination);
                    }
                }

                quick_block_dispose(&quick_block);
                break;
                }
            }
        }

        if(p_rtf_load_info->bracket_level < initial_bracket_level)
            break;
    }

    if(status_ok(status))
        status = rtf_array_add_NULLCH(p_h_hefo_text);

    if(status_ok(status) && !p_rtf_load_info->inserting)
    { /* do sub-object style update */
    DATA_REF               data_ref;
    OBJECT_POSITION_UPDATE object_position_update;
    OBJECT_POSITION        object_position;
    ARRAY_INDEX            index;

    object_position_init(&object_position);

    data_ref.data_space = p_headfoot_def->data_space;
    object_position_update.data_ref = data_ref;
    object_position_update.object_position = object_position;
    object_position_update.data_update = ustr_inline_strlen(ustr_inline_from_h_ustr(p_h_hefo_text));
    style_docu_area_position_update(p_docu, &p_headfoot_def->headfoot.h_style_list, &object_position_update);

    for(index = 0; index < array_elements(&hefo_region_list); index++) /* 4wards or backwards ? */
    {
        P_RTF_REGION p_rtf_region = array_ptr(&hefo_region_list, RTF_REGION, index);
        STYLE_DOCU_AREA_ADD_PARM style_docu_area_add_parm;

        if(!p_rtf_region->closed)
        {
            p_rtf_region->docu_area.br = p_rtf_region->docu_area.tl;
            p_rtf_region->docu_area.br.slr.col++;
            p_rtf_region->docu_area.br.slr.row++;
            p_rtf_region->docu_area.br.object_position.object_id = OBJECT_ID_TEXT;
            p_rtf_region->docu_area.br.object_position.data = p_rtf_load_info->position.object_position.data;
        }

        /* Now apply this as a docu_area */
        if(style_bit_test(&p_rtf_region->state_style.style, STYLE_SW_NAME))
        {
            STYLE_HANDLE style_handle = style_handle_from_name(p_docu, array_tstr(&p_rtf_region->state_style.style.h_style_name_tstr));
            STYLE_DOCU_AREA_ADD_HANDLE(&style_docu_area_add_parm, style_handle);
            style_docu_area_add_parm.p_array_handle = &p_headfoot_def->headfoot.h_style_list;
            style_docu_area_add_parm.data_space     = p_headfoot_def->data_space;
            status_return(style_docu_area_add(p_docu, &p_rtf_region->docu_area, &style_docu_area_add_parm));
            al_array_dispose(&p_rtf_region->state_style.style.h_style_name_tstr);
        }
        else
        {
            STYLE_DOCU_AREA_ADD_STYLE(&style_docu_area_add_parm, &p_rtf_region->state_style.style);
            style_docu_area_add_parm.p_array_handle = &p_headfoot_def->headfoot.h_style_list;
            style_docu_area_add_parm.data_space     = p_headfoot_def->data_space;

            status_return(style_docu_area_add(p_docu, &p_rtf_region->docu_area, &style_docu_area_add_parm));
        }
    }
    } /*block*/

    al_array_dispose(&hefo_region_list);

    return(STATUS_OK);
}

/******************************************************************************
*
* \header{x} (destination)
*
******************************************************************************/

PROC_RTF_CONTROL_PROTO(rtf_control_header)
{
    const P_DOCU p_docu = p_docu_from_docno(p_rtf_load_info->docno);
    const enum RTF_HEFO_TYPES rtf_hefo_type = (enum RTF_HEFO_TYPES) arg;
    PC_HEADFOOT_SIZES p_headfoot_sizes = &headfoot_sizes_default_header;
    P_HEADFOOT_DEF p_headfoot_def;
    P_PAGE_HEFO_BREAK p_page_hefo_break;
    BIT_NUMBER bit_number;
    DATA_SPACE data_space;
    STATUS status;

    func_ignore_parms();

    if(NULL == (p_page_hefo_break = p_page_hefo_break_from_row_exact(p_docu, p_rtf_load_info->position.slr.row)))
        if(NULL == (p_page_hefo_break = p_page_hefo_break_new_for_row(p_docu, p_rtf_load_info->position.slr.row, &status)))
            return(status);

    switch(rtf_hefo_type)
    {
    default: default_unhandled();
#if CHECKING
    case RTF_HEFO_ALL:  /* what should we do about this ? */
    case RTF_HEFO_RIGHT:
#endif
        p_headfoot_def = &p_page_hefo_break->header_odd;
        bit_number = PAGE_HEFO_HEADER_ODD;
        data_space = DATA_HEADER_ODD;
        break;

    case RTF_HEFO_LEFT:
        p_headfoot_def = &p_page_hefo_break->header_even;
        bit_number = PAGE_HEFO_HEADER_EVEN;
        data_space = DATA_HEADER_EVEN;
        break;

    case RTF_HEFO_FIRST:
        p_headfoot_def = &p_page_hefo_break->header_first;
        bit_number = PAGE_HEFO_HEADER_FIRST;
        data_space = DATA_HEADER_FIRST;
        break;
    }

    if(0 == page_hefo_selector_bit_test(&p_page_hefo_break->selector, bit_number))
    {
        status_return(headfoot_init(p_docu, p_headfoot_def, data_space));

        page_hefo_selector_bit_set(&p_page_hefo_break->selector, bit_number);

        p_headfoot_def->headfoot_sizes = *p_headfoot_sizes;
    }

    status_return(rtf_load_hefo(p_rtf_load_info, a7str_control_word, p_headfoot_def));

    p_rtf_load_info->position.object_position.data = array_elements(p_h_dest_contents);

    return(STATUS_OK);
}

/******************************************************************************
*
* \footer{x}
*
******************************************************************************/

PROC_RTF_CONTROL_PROTO(rtf_control_footer)
{
    const P_DOCU p_docu = p_docu_from_docno(p_rtf_load_info->docno);
    const enum RTF_HEFO_TYPES rtf_hefo_type = (enum RTF_HEFO_TYPES) arg;
    PC_HEADFOOT_SIZES p_headfoot_sizes = &headfoot_sizes_default_footer;
    P_HEADFOOT_DEF p_headfoot_def;
    P_PAGE_HEFO_BREAK p_page_hefo_break;
    BIT_NUMBER bit_number;
    DATA_SPACE data_space;
    STATUS status;

    func_ignore_parms();

    if(p_rtf_load_info->inserting)
        return(STATUS_OK);

    if(NULL == (p_page_hefo_break = p_page_hefo_break_from_row_exact(p_docu, p_rtf_load_info->position.slr.row)))
        if(NULL == (p_page_hefo_break = p_page_hefo_break_new_for_row(p_docu, p_rtf_load_info->position.slr.row, &status)))
            return(status);

    switch(rtf_hefo_type)
    {
    default: default_unhandled();
#if CHECKING
    case RTF_HEFO_ALL:  /* what should we do about this ? */
    case RTF_HEFO_RIGHT:
#endif
        p_headfoot_def = &p_page_hefo_break->footer_odd;
        bit_number = PAGE_HEFO_FOOTER_ODD;
        data_space = DATA_FOOTER_ODD;
        break;

    case RTF_HEFO_LEFT:
        p_headfoot_def = &p_page_hefo_break->footer_even;
        bit_number = PAGE_HEFO_FOOTER_EVEN;
        data_space = DATA_FOOTER_EVEN;
        break;

    case RTF_HEFO_FIRST:
        p_headfoot_def = &p_page_hefo_break->footer_first;
        bit_number = PAGE_HEFO_FOOTER_FIRST;
        data_space = DATA_FOOTER_FIRST;
        break;
    }

    if(0 == page_hefo_selector_bit_test(&p_page_hefo_break->selector, bit_number))
    {
        status_return(headfoot_init(p_docu, p_headfoot_def, data_space));

        page_hefo_selector_bit_set(&p_page_hefo_break->selector, bit_number);

        p_headfoot_def->headfoot_sizes = *p_headfoot_sizes;
    }

    status_return(rtf_load_hefo(p_rtf_load_info, a7str_control_word, p_headfoot_def));

    p_rtf_load_info->position.object_position.data = array_elements(p_h_dest_contents);

    return(STATUS_OK);
}

/******************************************************************************
*
* \par: New paragraph - save current cell and increment pointer
*
******************************************************************************/

PROC_RTF_CONTROL_PROTO(rtf_control_par)
{
    STATUS status = STATUS_OK;

    func_ignore_parms();

    if(p_rtf_load_info->in_table)
        return(rtf_control_line(p_rtf_load_info, empty_string, empty_string,
                                p_h_dest_contents, p_h_sub_regions,
                                0));

    al_array_empty(&p_rtf_load_info->h_table_widths);

    if(0 != array_elements(p_h_dest_contents))
    {
        if(status_ok(status = rtf_array_add_NULLCH(p_h_dest_contents)))
        {
            LOAD_CELL_FOREIGN load_cell_foreign;
            load_cell_foreign_init(p_rtf_load_info, &load_cell_foreign, ustr_inline_from_h_ustr(p_h_dest_contents));
            status = insert_cell_contents_foreign(p_docu_from_docno(p_rtf_load_info->docno), OBJECT_ID_TEXT, T5_MSG_LOAD_CELL_FOREIGN, &load_cell_foreign);
        }
    }

    /* Update current cell */
    p_rtf_load_info->position.slr.row++;
    object_position_init(&p_rtf_load_info->position.object_position);

    al_array_dispose(p_h_dest_contents);

    status_return(status);

    { /* Update process status to show percentage loaded */
    S32 percent = (p_rtf_load_info->position.slr.row - p_rtf_load_info->load_position.slr.row) * 90 / p_rtf_load_info->doc_rows;
    p_rtf_load_info->process_status.data.percent.current = percent;
    process_status_reflect(&p_rtf_load_info->process_status);
    } /*block*/

    /* If we are in a table, change the col widths */
    status_return(rtf_control_intbl(p_rtf_load_info, empty_string, empty_string,
                                    p_h_dest_contents, p_h_sub_regions,
                                    1 /* don't do top/bottom borders */));
    return(status);
}

/******************************************************************************
*
* \row: New row paragraph - save current cell and increment pointer.
*
******************************************************************************/

PROC_RTF_CONTROL_PROTO(rtf_control_row)
{
    STATUS status = STATUS_OK;

    func_ignore_parms();

    if(0 != array_elements(p_h_dest_contents))
    {
        if(status_ok(rtf_array_add_NULLCH(p_h_dest_contents)))
        {
            LOAD_CELL_FOREIGN load_cell_foreign;
            load_cell_foreign_init(p_rtf_load_info, &load_cell_foreign, ustr_inline_from_h_ustr(p_h_dest_contents));
            status = insert_cell_contents_foreign(p_docu_from_docno(p_rtf_load_info->docno), OBJECT_ID_TEXT, T5_MSG_LOAD_CELL_FOREIGN, &load_cell_foreign);
        }
    }

    /* Update current cell */
    p_rtf_load_info->position.slr.col = p_rtf_load_info->load_position.slr.col;
    p_rtf_load_info->position.slr.row++;
    object_position_init(&p_rtf_load_info->position.object_position);

    al_array_dispose(p_h_dest_contents);

    { /* Update process status to show percentage loaded */
    S32 percent = (p_rtf_load_info->position.slr.row - p_rtf_load_info->load_position.slr.row) * 90 / p_rtf_load_info->doc_rows;
    p_rtf_load_info->process_status.data.percent.current = percent;
    process_status_reflect(&p_rtf_load_info->process_status);
    } /*block*/

    return(status);
}

/******************************************************************************
*
* \cell: new cell - save current cell and increment pointer.
*
******************************************************************************/

PROC_RTF_CONTROL_PROTO(rtf_control_cell)
{
    STATUS status = STATUS_OK;

    func_ignore_parms();

    if(0 != array_elements(p_h_dest_contents))
    {
        if(status_ok(status = rtf_array_add_NULLCH(p_h_dest_contents)))
        {
            LOAD_CELL_FOREIGN load_cell_foreign;
            load_cell_foreign_init(p_rtf_load_info, &load_cell_foreign, ustr_inline_from_h_ustr(p_h_dest_contents));
            status = insert_cell_contents_foreign(p_docu_from_docno(p_rtf_load_info->docno), OBJECT_ID_TEXT, T5_MSG_LOAD_CELL_FOREIGN, &load_cell_foreign);
        }
    }

    /* Update current cell */
    p_rtf_load_info->position.slr.col++;
    p_rtf_load_info->position.object_position.data = 0;
    p_rtf_load_info->position.object_position.object_id = OBJECT_ID_NONE;

    al_array_dispose(p_h_dest_contents);

    return(status);
}

/******************************************************************************
*
* \red<N>, \green<N>, \blue<N> : NB ONLY IN COLORTBL
*
******************************************************************************/

PROC_RTF_CONTROL_PROTO(rtf_control_colortbl_rgb)
{
    U32 offset = arg;
    S32 parameter = fast_strtol(a7str_parameter, NULL);

    func_ignore_parms();

    assert(RTF_DESTINATION_COLORTBL == p_rtf_load_info->destination);

    if((U32) parameter < 256)
        PtrPutByteOff(&p_rtf_load_info->colortbl_rgb, offset, (BYTE) parameter);

    return(STATUS_OK);
}

/******************************************************************************
*
* \rtf: ignore this control word
*
******************************************************************************/

PROC_RTF_CONTROL_PROTO(rtf_control_rtf)
{
    func_ignore_parms();

    return(STATUS_OK);
}

/******************************************************************************
*
* ignore this control word
*
******************************************************************************/

PROC_RTF_CONTROL_PROTO(rtf_control_ignore)
{
    func_ignore_parms();

    trace_2(TRACE_IO_RTF, TEXT("RTF(%d): Ignoring control word \\%s"), p_rtf_load_info->bracket_level, report_sbstr(a7str_control_word));

    return(STATUS_OK);
}

/******************************************************************************
*
* \mac etc: ignore this control word and give warning
*
******************************************************************************/

PROC_RTF_CONTROL_PROTO(rtf_control_warning_charset)
{
    func_ignore_parms();

    trace_2(TRACE_IO_RTF, TEXT("RTF(%d): Unhandled character set \\%s"), p_rtf_load_info->bracket_level, report_sbstr(a7str_control_word));

    return(STATUS_OK);
}

/******************************************************************************
*
* Add a region to the sub_regions list
*
******************************************************************************/

_Check_return_
static STATUS
rtf_load_start_region(
    _InoutRef_  P_RTF_LOAD_INFO p_rtf_load_info,
    P_ARRAY_HANDLE p_h_sub_regions,
    _InVal_     STYLE_BIT_NUMBER style_bit_number,
    _InVal_     S32 parameter,
    P_STATE_STYLE p_current_state_style)
{
    /* Check that an 'opening' region doesn't already exist to be closed, otherwise add region to h_sub_regions */
    S32 index;
    P_RTF_REGION p_rtf_region;
    STATUS status;

    if(NULL == p_h_sub_regions)
    {
        assert0();
        return(STATUS_OK);
    }

    for(index = 0; index < array_elements(p_h_sub_regions); index ++)
    {
        p_rtf_region = array_ptr(p_h_sub_regions, RTF_REGION, index);

        /* if this parameter already set in p_current_state_style, ignore it */
        if(status_ok(rtf_load_state_style_similar_bit(p_rtf_load_info, p_current_state_style, style_bit_number, parameter)))
        {
            /* ok, this attribute is already set, so ignore it. */
            return(STATUS_OK);
        }

        /* if this region is the same as 'style_bit_number', close it & return */
        if(   !p_rtf_region->closed
           && status_ok(rtf_load_check_reverse_attribute(&p_rtf_region->state_style, style_bit_number, parameter)))
        {
            /* MATCH !! */
            /* make this exclusive ! */

            p_rtf_region->docu_area.br = p_rtf_load_info->position;
            if(p_rtf_region->docu_area.tl.slr.row == p_rtf_region->docu_area.br.slr.row)
                p_rtf_region->docu_area.br.slr.row++;

            p_rtf_region->docu_area.br.slr.col = p_rtf_region->docu_area.tl.slr.col+1;

            p_rtf_region->closed = 1;

            p_rtf_load_info->doc_regions++;

            /* put current state style back */
            return(rtf_load_state_style_set_bit(p_rtf_load_info, p_current_state_style, style_bit_number, parameter));
        }
    }

    /* Not a closing => create a new region */
    if(NULL == (p_rtf_region = al_array_extend_by(p_h_sub_regions, RTF_REGION, 1, PC_ARRAY_INIT_BLOCK_NONE, &status)))
        return(status);

    state_style_init(&p_rtf_region->state_style);
    p_rtf_region->state_style.style.h_style_name_tstr = 0;
    docu_area_init(&p_rtf_region->docu_area);
    p_rtf_region->docu_area.tl = p_rtf_load_info->position;
    p_rtf_region->docu_area.br = p_rtf_load_info->position;
    p_rtf_region->closed = 0;

    /* update current_state_style */
    if(STYLE_SW_NAME == style_bit_number)
        assert0();

    status_return(rtf_load_state_style_set_bit(p_rtf_load_info, p_current_state_style, style_bit_number, parameter));

    return(rtf_load_state_style_set_bit(p_rtf_load_info, &p_rtf_region->state_style, style_bit_number, parameter));
}

/******************************************************************************
*
* Check that the attribute is actually the same as that in p_style.
*
******************************************************************************/

_Check_return_
static STATUS
rtf_load_state_style_similar_bit(
    _InoutRef_  P_RTF_LOAD_INFO p_rtf_load_info,
    P_STATE_STYLE p_state_style,
    _InVal_     STYLE_BIT_NUMBER style_bit_number,
    _InVal_     S32 parameter)
{
    const P_STYLE p_style = &p_state_style->style;
    BOOL match = FALSE;
    BOOL found;

    if(!style_bit_test(p_style, style_bit_number))
        return(STATUS_FAIL);

    switch(style_bit_number)
    {
    case STYLE_SW_CS_WIDTH:
        match = (p_style->col_style.width == parameter);
        break;

    case STYLE_SW_RS_HEIGHT:
        match = (p_style->row_style.height == parameter);
        break;

    case STYLE_SW_FS_SIZE_Y:
        match = (p_style->font_spec.size_y == (parameter * PIXITS_PER_POINT) / 2);
        break;

    case STYLE_SW_FS_NAME:
        {
        PCTSTR font_name_tstr = array_tstr(&p_style->font_spec.h_app_name_tstr);
        STATUS name_index = /*create_error*/(fontmap_app_index_from_app_font_name(font_name_tstr));
        match = ((S32) name_index == parameter);
        break;
        }

    case STYLE_SW_FS_UNDERLINE:
        match = (p_style->font_spec.underline == (parameter ? 1 : 0));
        break;

    case STYLE_SW_FS_BOLD:
        match = (p_style->font_spec.bold == (parameter ? 1 : 0));
        break;

    case STYLE_SW_FS_ITALIC:
        match = (p_style->font_spec.italic == (parameter ? 1 : 0));
        break;

    case STYLE_SW_FS_SUPERSCRIPT:
        match = (p_style->font_spec.superscript == (parameter ? 1 : 0));
        break;

    case STYLE_SW_FS_SUBSCRIPT:
        match = (p_style->font_spec.subscript == (parameter ? 1 : 0));
        break;

    case STYLE_SW_PS_MARGIN_LEFT:
        match = (p_style->para_style.margin_left == MAX(32, parameter));
        break;

    case STYLE_SW_PS_MARGIN_RIGHT:
        match = (p_style->para_style.margin_right == MAX(32, parameter));
        break;

    case STYLE_SW_PS_MARGIN_PARA:
        match = (p_style->para_style.margin_para == parameter);
        break;

    case STYLE_SW_PS_PARA_START:
        match = (p_style->para_style.para_start == parameter);
        break;

    case STYLE_SW_PS_PARA_END:
        match = (p_style->para_style.para_end == parameter);
        break;

    case STYLE_SW_PS_JUSTIFY:
        match = (p_style->para_style.justify == parameter);
        break;

    case STYLE_SW_PS_JUSTIFY_V:
        match = (p_style->para_style.justify_v == parameter);
        break;

    case STYLE_SW_PS_GRID_LEFT:
        match = (p_style->para_style.grid_left == (parameter ? 1 : 0));
        break;

    case STYLE_SW_PS_GRID_TOP:
        match = (p_style->para_style.grid_top == (parameter ? 1 : 0));
        break;

    case STYLE_SW_PS_GRID_RIGHT:
        match = (p_style->para_style.grid_right == (parameter ? 1 : 0));
        break;

    case STYLE_SW_PS_GRID_BOTTOM:
        match = (p_style->para_style.grid_bottom == (parameter ? 1 : 0));
        break;

    case STYLE_SW_PS_RGB_BACK:
        match = (find_colour_in_table_load(&p_rtf_load_info->h_colour_table_load, &p_style->para_style.rgb_back, &found) == parameter);
        break;

    case STYLE_SW_FS_COLOUR:
        match = (find_colour_in_table_load(&p_rtf_load_info->h_colour_table_load, &p_style->font_spec.colour, &found) == parameter);
        break;

    case STYLE_SW_PS_RGB_GRID_LEFT:
        match = (find_colour_in_table_load(&p_rtf_load_info->h_colour_table_load, &p_style->para_style.rgb_grid_left, &found) == parameter);
        break;

    case STYLE_SW_PS_RGB_GRID_TOP:
        match = (find_colour_in_table_load(&p_rtf_load_info->h_colour_table_load, &p_style->para_style.rgb_grid_top, &found) == parameter);
        break;

    case STYLE_SW_PS_RGB_GRID_RIGHT:
        match = (find_colour_in_table_load(&p_rtf_load_info->h_colour_table_load, &p_style->para_style.rgb_grid_right, &found) == parameter);
        break;

    case STYLE_SW_PS_RGB_GRID_BOTTOM:
        match = (find_colour_in_table_load(&p_rtf_load_info->h_colour_table_load, &p_style->para_style.rgb_grid_bottom, &found) == parameter);
        break;

    default: default_unhandled();
        break;
    }

    if(!match)
        return(STATUS_FAIL);

    return(STATUS_OK);
}

/******************************************************************************
*
* Check that the attribute is actually the reverse of that in the style
*
******************************************************************************/

_Check_return_
static STATUS
rtf_load_check_reverse_attribute(
    _InRef_     PC_STATE_STYLE p_state_style,
    _InVal_     S32 attrib,
    _InVal_     S32 parameter)
{
    const PC_STYLE p_style = &p_state_style->style;
    BOOL match = FALSE;

    if(!style_bit_test(p_style, attrib))
        return(STATUS_FAIL);

    switch(attrib)
    {
    case STYLE_SW_FS_UNDERLINE:
        match = (p_style->font_spec.underline == (1 - (parameter ? 1 : 0)));
        break;

    case STYLE_SW_FS_BOLD:
        match = (p_style->font_spec.bold == (1 - (parameter ? 1 : 0)));
        break;

    case STYLE_SW_FS_ITALIC:
        match = (p_style->font_spec.italic == (1 - (parameter ? 1 : 0)));
        break;

    case STYLE_SW_FS_SUPERSCRIPT:
        match = (p_style->font_spec.superscript == (1 - (parameter ? 1 : 0)));
        break;

    case STYLE_SW_FS_SUBSCRIPT:
        match = (p_style->font_spec.subscript == (1 - (parameter ? 1 : 0)));
        break;

    default:
        break;
    }

    return(match ? STATUS_OK : STATUS_FAIL);
}

/******************************************************************************
*
* Set style_bit_number in a style to a parameter
*
******************************************************************************/

_Check_return_
static STATUS
rtf_load_state_style_set_bit(
    _InoutRef_  P_RTF_LOAD_INFO p_rtf_load_info,
    _InoutRef_  P_STATE_STYLE p_state_style,
    _InVal_     STYLE_BIT_NUMBER style_bit_number,
    _InVal_     S32 parameter)
{
    const P_STYLE p_style = &p_state_style->style;
    STATUS status = STATUS_OK;

    switch(style_bit_number)
    {
    case STYLE_SW_NAME:
        {
        const P_DOCU p_docu = p_docu_from_docno(p_rtf_load_info->docno);
        P_RTF_STYLESHEET p_rtf_stylesheet = NULL;
        P_STYLE p_local_style;
        const ARRAY_INDEX n_elements = array_elements(&p_rtf_load_info->h_stylesheet_load);
        ARRAY_INDEX index;
        P_RTF_STYLESHEET this_p_rtf_stylesheet = array_range(&p_rtf_load_info->h_stylesheet_load, RTF_STYLESHEET, 0, n_elements);

        for(index = 0; index < n_elements; ++index, ++this_p_rtf_stylesheet)
        {
            if(this_p_rtf_stylesheet->style_number != (U32) parameter)
                continue;

            p_rtf_stylesheet = this_p_rtf_stylesheet;
            break;
        }

        PTR_ASSERT(p_rtf_stylesheet);

        p_local_style = p_style_from_handle(p_docu, style_handle_from_name(p_docu, array_tstr(&p_rtf_stylesheet->h_style_name_tstr)));

        /* Extract all switches set in this style */
        style_selector_bit_clear(&p_local_style->selector, STYLE_SW_NAME);
        if(style_bit_test(p_style, STYLE_SW_FS_NAME) && style_bit_test(p_local_style, STYLE_SW_FS_NAME))
            font_spec_name_dispose(&p_style->font_spec);

        if(status_ok(status = style_duplicate(p_style, p_local_style, &p_local_style->selector))) /* we claim the resources!! */
            style_bit_set(p_local_style, STYLE_SW_NAME);
        break;
        }

    case STYLE_SW_FS_UNDERLINE:
        p_style->font_spec.underline = (U8) (parameter ? 1 : 0);
        break;

    case STYLE_SW_FS_BOLD:
        p_style->font_spec.bold = (U8) (parameter ? 1 : 0);
        break;

    case STYLE_SW_FS_ITALIC:
        p_style->font_spec.italic = (U8) (parameter ? 1 : 0);
        break;

    case STYLE_SW_FS_SUPERSCRIPT:
        p_style->font_spec.superscript = (U8) (parameter ? 1 : 0);
        break;

    case STYLE_SW_FS_SUBSCRIPT:
        p_style->font_spec.subscript = (U8) (parameter ? 1 : 0);
        break;

    case STYLE_SW_FS_NAME:
        {
        PCTSTR font_name_tstr = NULL;

        if(style_bit_test(p_style, STYLE_SW_FS_NAME)) /* may be overwriting font for current_state_style => dispose of old name first */
            font_spec_name_dispose(&p_style->font_spec);

        {
        const ARRAY_INDEX n_elements = array_elements32(&p_rtf_load_info->h_rtf_font_table);
        ARRAY_INDEX i;
        P_RTF_FONT_TABLE p_rtf_font_table = array_range(&p_rtf_load_info->h_rtf_font_table, RTF_FONT_TABLE, 0, n_elements);

        for(i = 0; i < n_elements; ++i, ++p_rtf_font_table)
        {
            if(p_rtf_font_table->font_number == (U32) parameter)
                font_name_tstr = array_tstr(&p_rtf_font_table->h_rtf_font_name_tstr);
        }
        } /*block*/

        if(NULL == font_name_tstr)
        {
            status = ERR_HOST_FONT_NOT_FOUND;
        }
        else if(ERR_HOST_FONT_NOT_FOUND == (status = fontmap_font_spec_from_host_base_name(&p_style->font_spec, font_name_tstr)))
        {
            if(ERR_TYPE5_FONT_NOT_FOUND == (status = fontmap_app_index_from_app_font_name(font_name_tstr)))
            {
                /* look it up in our table */
                U32 index;

                for(index = 0; index < elemof32(rtf_font_map); ++index)
                {
                    if(0 == tstrcmp(rtf_font_map[index].rtf_font_name, font_name_tstr))
                    {
                        STATUS app_index = fontmap_app_index_from_app_font_name(rtf_font_map[index].app_font_name);
                        status = fontmap_font_spec_from_app_index(&p_style->font_spec, app_index);
                        break;
                    }
                }
            }
        }

        if(ERR_TYPE5_FONT_NOT_FOUND == status) /* ok, we tried our best ! */
            status = font_spec_name_alloc(&p_style->font_spec, TEXT("Helvetica"));
        else
        {
            PTR_ASSERT(font_name_tstr);
            status = font_spec_name_alloc(&p_style->font_spec, font_name_tstr);
        }

        break;
        }

    case STYLE_SW_FS_SIZE_Y:
        p_style->font_spec.size_y = (parameter * PIXITS_PER_POINT) / 2;
        break;

    case STYLE_SW_CS_WIDTH:
        p_style->col_style.width = parameter;
        break;

    case STYLE_SW_RS_HEIGHT_FIXED:
        p_style->row_style.height_fixed = (U8) (parameter ? 1 : 0);
        break;

    case STYLE_SW_PS_JUSTIFY:
        p_style->para_style.justify = (U8) parameter;
        break;

    case STYLE_SW_PS_JUSTIFY_V:
        p_style->para_style.justify_v = (U8) parameter;
        break;

    case STYLE_SW_RS_HEIGHT:
        p_style->row_style.height = parameter;
        if(p_style->row_style.height == 0)
            p_style->row_style.height = -p_style->row_style.height;
        break;

    case STYLE_SW_PS_MARGIN_LEFT:
        p_style->para_style.margin_left = MAX(32, parameter);
        break;

    case STYLE_SW_PS_MARGIN_RIGHT:
        p_style->para_style.margin_right = MAX(32, parameter);
        break;

    case STYLE_SW_PS_MARGIN_PARA:
        p_style->para_style.margin_para = parameter;
        break;

    case STYLE_SW_PS_PARA_START:
        p_style->para_style.para_start = parameter;
        break;

    case STYLE_SW_PS_PARA_END:
        p_style->para_style.para_end = parameter;
        break;

    case STYLE_SW_PS_GRID_TOP:
        p_style->para_style.grid_top = (U8) (parameter ? 1 : 0);
        rtf_load_decode_colour(p_rtf_load_info, &p_style->para_style.rgb_grid_top, p_rtf_load_info->grid_colour);
        style_bit_set(p_style, STYLE_SW_PS_RGB_GRID_TOP);
        break;

    case STYLE_SW_PS_GRID_BOTTOM:
        p_style->para_style.grid_bottom = (U8) (parameter ? 1 : 0);
        rtf_load_decode_colour(p_rtf_load_info, &p_style->para_style.rgb_grid_bottom, p_rtf_load_info->grid_colour);
        style_bit_set(p_style, STYLE_SW_PS_RGB_GRID_BOTTOM);
        break;

    case STYLE_SW_PS_GRID_LEFT:
        p_style->para_style.grid_left = (U8) (parameter ? 1 : 0);
        rtf_load_decode_colour(p_rtf_load_info, &p_style->para_style.rgb_grid_left, p_rtf_load_info->grid_colour);
        style_bit_set(p_style, STYLE_SW_PS_RGB_GRID_LEFT);
        break;

    case STYLE_SW_PS_GRID_RIGHT:
        p_style->para_style.grid_right = (U8) (parameter ? 1 : 0);
        rtf_load_decode_colour(p_rtf_load_info, &p_style->para_style.rgb_grid_right, p_rtf_load_info->grid_colour);
        style_bit_set(p_style, STYLE_SW_PS_RGB_GRID_RIGHT);
        break;

    case STYLE_SW_PS_BORDER:
        p_style->para_style.border = (U8) (parameter ? 1 : 0);
        break;

    case STYLE_SW_FS_COLOUR:
        rtf_load_decode_colour(p_rtf_load_info, &p_style->font_spec.colour, parameter);
        break;

    case STYLE_SW_PS_RGB_BACK:
        rtf_load_decode_colour(p_rtf_load_info, &p_style->para_style.rgb_back, parameter);
        break;

    default: default_unhandled();
        break;
    }

    if((style_bit_number != STYLE_SW_NAME) && status_ok(status))
        style_bit_set(p_style, style_bit_number);

    return(status);
}

/******************************************************************************
*
* Convert RTF color table entry number to RGB
*
******************************************************************************/

static void
rtf_load_decode_colour(
    _InoutRef_  P_RTF_LOAD_INFO p_rtf_load_info,
    _OutRef_    P_RGB p_rgb,
    _InVal_     S32 colour)
{
    if(array_index_is_valid(&p_rtf_load_info->h_colour_table_load, colour))
    {
        *p_rgb = *array_ptrc_no_checks(&p_rtf_load_info->h_colour_table_load, RGB, colour);
        return;
    }

    myassert2(TEXT("Colour number %d out of range %d"), colour, array_elements(&p_rtf_load_info->h_colour_table_load));
    zero_struct_ptr(p_rgb);
}

/******************************************************************************
*
* \chdate
*
******************************************************************************/

PROC_RTF_CONTROL_PROTO(rtf_control_chdate)
{
    func_ignore_parms();

    status_return(inline_array_from_data(p_h_dest_contents, IL_DATE, IL_TYPE_USTR, USTR_TEXT("dd Mmmm yyyy"), 0));

    p_rtf_load_info->position.object_position.data = array_elements(p_h_dest_contents);
    p_rtf_load_info->position.object_position.object_id = OBJECT_ID_TEXT;

    return(STATUS_OK);
}

/******************************************************************************
*
* \chpgn
*
******************************************************************************/

PROC_RTF_CONTROL_PROTO(rtf_control_chpgn)
{
    func_ignore_parms();

    status_return(inline_array_from_data(p_h_dest_contents, IL_PAGE_Y, IL_TYPE_USTR, USTR_TEXT("0"), 0));

    p_rtf_load_info->position.object_position.data = array_elements(p_h_dest_contents);
    p_rtf_load_info->position.object_position.object_id = OBJECT_ID_TEXT;

    return(STATUS_OK);
}

/******************************************************************************
*
* \line
*
******************************************************************************/

PROC_RTF_CONTROL_PROTO(rtf_control_line)
{
    func_ignore_parms();

    status_return(inline_array_from_data(p_h_dest_contents, IL_RETURN, IL_TYPE_NONE, NULL, 0));

    p_rtf_load_info->position.object_position.data = array_elements(p_h_dest_contents);
    p_rtf_load_info->position.object_position.object_id = OBJECT_ID_TEXT;

    return(STATUS_OK);
}

/******************************************************************************
*
* \s<n>: insert style (or define style number for stylesheet builder)
*
******************************************************************************/

PROC_RTF_CONTROL_PROTO(rtf_control_s)
{
    S32 parameter = fast_strtol(a7str_parameter, NULL);
    U32 style_number = (U32) parameter;
    P_RTF_STYLESHEET p_rtf_stylesheet = NULL;
    P_RTF_REGION p_rtf_region;
    S32 index;
    STATUS status;

    func_ignore_parms();

    switch(p_rtf_load_info->destination)
    {
    case RTF_DESTINATION_STYLESHEET:
        p_rtf_load_info->stylesheet_style_number = style_number;
        return(STATUS_OK);

    default:
        break;
    }

    for(index = 0; index < array_elements(&p_rtf_load_info->h_stylesheet_load); index++)
    {
        P_RTF_STYLESHEET this_p_rtf_stylesheet = array_ptr(&p_rtf_load_info->h_stylesheet_load, RTF_STYLESHEET, index);

        if(this_p_rtf_stylesheet->style_number == style_number)
        {
            p_rtf_stylesheet = this_p_rtf_stylesheet;
            break;
        }
    }

    if(NULL == p_rtf_stylesheet)
    {
        myassert1(TEXT("Used style number %u not defined in stylesheet"), style_number);
        return(STATUS_FAIL);
    }

    /* apply style p_rtf_stylesheet->name */
    if(NULL == (p_rtf_region = al_array_extend_by(p_h_sub_regions, RTF_REGION, 1, PC_ARRAY_INIT_BLOCK_NONE, &status)))
        return(status);

    state_style_init(&p_rtf_region->state_style);
    docu_area_init(&p_rtf_region->docu_area);
    p_rtf_region->docu_area.tl = p_rtf_load_info->position;
    p_rtf_region->closed = 0;

    style_bit_set(&p_rtf_region->state_style.style, STYLE_SW_NAME);
    status_assert(al_tstr_set(&p_rtf_region->state_style.style.h_style_name_tstr, array_tstr(&p_rtf_stylesheet->h_style_name_tstr)));

    return(rtf_load_state_style_set_bit(p_rtf_load_info, &p_rtf_load_info->state_style, STYLE_SW_NAME, (S32) style_number));
}

/******************************************************************************
*
* \tab
*
******************************************************************************/

PROC_RTF_CONTROL_PROTO(rtf_control_tab)
{
    func_ignore_parms();

    status_return(inline_array_from_data(p_h_dest_contents, IL_TAB, IL_TYPE_NONE, NULL, 0));

    p_rtf_load_info->position.object_position.data = array_elements(p_h_dest_contents);
    p_rtf_load_info->position.object_position.object_id = OBJECT_ID_TEXT;

    return(STATUS_OK);
}

/******************************************************************************
*
* \tq{xxx}
*
******************************************************************************/

PROC_RTF_CONTROL_PROTO(rtf_control_tq)
{
    const TAB_TYPE tab_type = (TAB_TYPE) arg;

    func_ignore_parms();

    p_rtf_load_info->tab_type = tab_type;

    return(STATUS_OK);
}

/******************************************************************************
*
* \tx
*
******************************************************************************/

PROC_RTF_CONTROL_PROTO(rtf_control_tx)
{
    P_RTF_REGION p_rtf_region = NULL;
    SC_ARRAY_INIT_BLOCK tab_init_block = aib_init(1, sizeof32(TAB_INFO), FALSE);
    S32 index;
    P_STATE_STYLE p_state_style = &p_rtf_load_info->state_style;
    P_TAB_INFO p_dst_tab_info;
    STATUS status;

    func_ignore_parms();

    if(NULL != p_h_sub_regions) /* otherwise, we are building stylesheet */
    {  /* look for already present tablist */
        p_state_style = NULL;

        for(index = 0; index < array_elements(p_h_sub_regions); index ++)
        {
            p_rtf_region = array_ptr(p_h_sub_regions, RTF_REGION, index);

            if(style_bit_test(&p_rtf_region->state_style.style, STYLE_SW_PS_TAB_LIST))
            {
                p_state_style = &p_rtf_region->state_style;
                break;
            }
        }

        if(NULL == p_state_style) /* if none already present ... */
        {
            if(NULL == (p_rtf_region = al_array_extend_by(p_h_sub_regions, RTF_REGION, 1, PC_ARRAY_INIT_BLOCK_NONE, &status)))
                return(status);

            state_style_init(&p_rtf_region->state_style);
            docu_area_init(&p_rtf_region->docu_area);
            p_rtf_region->docu_area.tl = p_rtf_load_info->position;
            p_rtf_region->closed = 0;
            p_state_style = &p_rtf_region->state_style;
        }
    }

    if(!style_bit_test(&p_state_style->style, STYLE_SW_PS_TAB_LIST))
    {
        style_bit_set(&p_state_style->style, STYLE_SW_PS_TAB_LIST);
        p_state_style->style.para_style.h_tab_list = 0;
    }

    if(NULL == (p_dst_tab_info = al_array_extend_by(&p_state_style->style.para_style.h_tab_list, TAB_INFO, 1, &tab_init_block, &status)))
        return(status);

    {
    S32 parameter = fast_strtol(a7str_parameter, NULL);
    PIXIT insert_tab_position = parameter;
    p_dst_tab_info->type   = p_rtf_load_info->tab_type;
    p_dst_tab_info->offset = insert_tab_position;
    } /*block*/

    p_rtf_load_info->tab_type = TAB_LEFT;

    return(STATUS_OK);
}

/******************************************************************************
*
* Insert a character, either as a byte or inline
*
******************************************************************************/

_Check_return_
static STATUS
rtf_insert_ucs4(
    _InoutRef_  P_RTF_LOAD_INFO p_rtf_load_info,
    P_ARRAY_HANDLE p_h_dest_contents,
    _In_        UCS4 ucs4)
{
    STATUS status;

#if USTR_IS_SBSTR /* try hard not to make inlines - convert to native */
    if(!ucs4_is_sbchar(ucs4))
        ucs4 = ucs4_to_sbchar_try_with_codepage(ucs4, get_system_codepage());
#endif

    if(ucs4_is_sbchar(ucs4))
    {
        SBCHAR sbchar = (SBCHAR) ucs4 & 0xFF;

        status = rtf_array_add_bytes(p_h_dest_contents, 1, &sbchar);
    }
    else
    {
        status_return(ucs4_validate(ucs4));

        {
#if USTR_IS_SBSTR
        UTF8B utf8_buffer[8];
        U32 bytes_of_char = utf8_char_encode(utf8_bptr(utf8_buffer), elemof32(utf8_buffer), ucs4);
        status = inline_array_from_data(p_h_dest_contents, IL_UTF8, IL_TYPE_ANY, utf8_buffer, bytes_of_char);
#else
        UCHARB uchar_buffer[8];
        U32 bytes_of_char = uchars_char_encode(uchars_bptr(uchar_buffer), elemof32(uchar_buffer), ucs4);
        status = rtf_array_add_bytes(p_h_dest_contents, bytes_of_char, uchar_buffer);
#endif
        } /*block*/
    }

    p_rtf_load_info->position.object_position.data = array_elements(p_h_dest_contents);
    p_rtf_load_info->position.object_position.object_id = OBJECT_ID_TEXT;

    return(status);
}

/******************************************************************************
*
* Insert a specific character
*
******************************************************************************/

PROC_RTF_CONTROL_PROTO(rtf_control_insert_char)
{
    UCS4 ucs4 = (UCS4) arg;

    func_ignore_parms();

    return(rtf_insert_ucs4(p_rtf_load_info, p_h_dest_contents, ucs4));
}

/******************************************************************************
*
* \ansi
*
******************************************************************************/

PROC_RTF_CONTROL_PROTO(rtf_control_ansi)
{
    func_ignore_parms();

    return(STATUS_OK);
}

/******************************************************************************
*
* \ansicpg<n>
*
******************************************************************************/

PROC_RTF_CONTROL_PROTO(rtf_control_ansicpg)
{
    S32 parameter = fast_strtol(a7str_parameter, NULL);

    func_ignore_parms();

    p_rtf_load_info->_ansicpg = (U32) parameter;

    return(STATUS_OK);
}

/******************************************************************************
*
* \dbch 
*
******************************************************************************/

PROC_RTF_CONTROL_PROTO(rtf_control_dbch)
{
    func_ignore_parms();

    p_rtf_load_info->_loch_hich_dbch = 2;

    return(STATUS_OK);
}

/******************************************************************************
*
* \hich 
*
******************************************************************************/

PROC_RTF_CONTROL_PROTO(rtf_control_hich)
{
    func_ignore_parms();

    p_rtf_load_info->_loch_hich_dbch = 1;

    return(STATUS_OK);
}

/******************************************************************************
*
* \loch 
*
******************************************************************************/

PROC_RTF_CONTROL_PROTO(rtf_control_loch)
{
    func_ignore_parms();

    p_rtf_load_info->_loch_hich_dbch = 0;

    return(STATUS_OK);
}

/******************************************************************************
*
* \'xx: insert a hex char from set, translating from RTF file encoding
*
******************************************************************************/

_Check_return_
static inline STATUS
rtf_insert_sbchar(
    _InoutRef_  P_RTF_LOAD_INFO p_rtf_load_info,
    P_ARRAY_HANDLE p_h_dest_contents,
    _In_        SBCHAR sbchar,
    _InVal_     SBCHAR_CODEPAGE sbchar_codepage)
{
    UCS4 ucs4 = sbchar;

    if(!ucs4_is_ascii7(ucs4) && (0 != sbchar_codepage))
        /* map top bit set character from whatever codepage the file was encoded with */
        ucs4 = ucs4_from_sbchar_with_codepage((SBCHAR) ucs4 & 0xFF, sbchar_codepage);

    return(rtf_insert_ucs4(p_rtf_load_info, p_h_dest_contents, ucs4));
}

PROC_RTF_CONTROL_PROTO(rtf_control_hex_char)
{
    U32 u32 = (U32) strtoul(a7str_parameter, NULL, 16);

    func_ignore_parms();

#if CHECKING
    assert(u32 < 256);

    switch(p_rtf_load_info->_loch_hich_dbch)
    {
    default:
    case 2: /* \dbch */
        break;

    case 1: /* \hich */
        assert(0 != (u32 & 0x80));
        break;

    case 0: /* \loch */
        assert(0 == (u32 & 0x80));
        break;
    }
#endif

    if(0 != p_rtf_load_info->ignore_chars_count)
    {   /* immediately following a Unicode code point - ignore */
        trace_2(TRACE_IO_RTF, TEXT("RTF(%d): ignore_chars_count=") U32_TFMT, p_rtf_load_info->bracket_level, p_rtf_load_info->ignore_chars_count);
        p_rtf_load_info->ignore_chars_count--;
        return(STATUS_OK);
    }

    return(rtf_insert_sbchar(p_rtf_load_info, p_h_dest_contents, (SBCHAR) u32 & 0xFF, p_rtf_load_info->_ansicpg));
}

/******************************************************************************
*
* \u<n>: insert a Unicode character
*
******************************************************************************/

PROC_RTF_CONTROL_PROTO(rtf_control_u)
{
    S32 parameter = fast_strtol(a7str_parameter, NULL); /* look for signed 16-bit decimal value */
    UCS4 ucs4;

    func_ignore_parms();

    p_rtf_load_info->ignore_chars_count = p_rtf_load_info->_uc; /* multibyte sequence may follow - \uc<N> characters to be ignored */

    if(parameter < 0)
        parameter = 65536 + parameter;

    ucs4 = (UCS4) parameter;

    status_return(ucs4_validate(ucs4));

    assert(!ucs4_is_C1(ucs4)); /* hopefully no C1 to complicate things */

    return(rtf_insert_ucs4(p_rtf_load_info, p_h_dest_contents, ucs4));
}

/******************************************************************************
*
* \uc<n>: 
*
******************************************************************************/

PROC_RTF_CONTROL_PROTO(rtf_control_uc)
{
    S32 parameter = fast_strtol(a7str_parameter, NULL);

    func_ignore_parms();

    assert((1 == parameter) || (2 == parameter));
    if((1 == parameter) || (2 == parameter))
        p_rtf_load_info->_uc = (U32) parameter;
    else
        p_rtf_load_info->_uc = 0;

    return(STATUS_OK);
}

/******************************************************************************
*
* \pard: Default paragraph style apply
*
******************************************************************************/

PROC_RTF_CONTROL_PROTO(rtf_control_pard)
{
    STATUS status;
    S32 index;
    STYLE_SELECTOR selector;

    func_ignore_parms();

    p_rtf_load_info->in_table = FALSE;

    style_selector_clear(&selector);
    style_selector_bit_set(&selector, STYLE_SW_NAME);
    style_selector_bit_set(&selector, STYLE_SW_FS_COLOUR);
    style_selector_bit_set(&selector, STYLE_SW_FS_ITALIC);
    style_selector_bit_set(&selector, STYLE_SW_FS_BOLD);
    style_selector_bit_set(&selector, STYLE_SW_FS_UNDERLINE);
    style_selector_bit_set(&selector, STYLE_SW_FS_SUPERSCRIPT);
    style_selector_bit_set(&selector, STYLE_SW_FS_SUBSCRIPT);
    style_selector_bit_set(&selector, STYLE_SW_PS_MARGIN_LEFT);
    style_selector_bit_set(&selector, STYLE_SW_PS_MARGIN_RIGHT);
    style_selector_bit_set(&selector, STYLE_SW_PS_MARGIN_PARA);
    style_selector_bit_set(&selector, STYLE_SW_PS_TAB_LIST);
    style_selector_bit_set(&selector, STYLE_SW_PS_JUSTIFY);
    style_selector_bit_set(&selector, STYLE_SW_FS_NAME);
    style_selector_bit_set(&selector, STYLE_SW_FS_SIZE_Y);
    style_selector_bit_set(&selector, STYLE_SW_RS_HEIGHT);
    style_selector_bit_set(&selector, STYLE_SW_PS_GRID_LEFT);
    style_selector_bit_set(&selector, STYLE_SW_PS_GRID_TOP);
    style_selector_bit_set(&selector, STYLE_SW_PS_GRID_RIGHT);
    style_selector_bit_set(&selector, STYLE_SW_PS_GRID_BOTTOM);
    style_selector_bit_set(&selector, STYLE_SW_PS_RGB_BACK);
    /* any others ? */

    /* ought to close only paragraph-dependent regions */
    /*                     -------------------         */

    if(array_elements(p_h_sub_regions) != 0)
    {
        /* close relevant open-ended regions */
        for(index = 0; index < array_elements(p_h_sub_regions); index++)
        {
            P_RTF_REGION p_rtf_region = array_ptr(p_h_sub_regions, RTF_REGION, index);

            if(!p_rtf_region->closed && style_selector_test(&p_rtf_region->state_style.style.selector, &selector))
            {
                if(style_bit_test(&p_rtf_region->state_style.style, STYLE_SW_NAME) && !tstrcmp(array_tstr(&p_rtf_region->state_style.style.h_style_name_tstr), TEXT("RTF Default Section")))
                { /*EMPTY*/ } /* don't close */
                else
                {
                    /* make this exclusive ! */

                    p_rtf_region->docu_area.br = p_rtf_load_info->position;
                    if(p_rtf_region->docu_area.tl.slr.row == p_rtf_region->docu_area.br.slr.row)
                        p_rtf_region->docu_area.br.slr.row++;

                    if((OBJECT_ID_TEXT != p_rtf_region->docu_area.tl.object_position.object_id) && p_rtf_region->docu_area.tl.slr.col == 0)
                        p_rtf_region->docu_area.br.slr.col += p_rtf_load_info->doc_cols;
                    else
                    {
                        if(p_rtf_region->docu_area.tl.slr.col >= p_rtf_region->docu_area.br.slr.col)
                            p_rtf_region->docu_area.br.slr.col = p_rtf_region->docu_area.tl.slr.col + 1;
                    }

                    p_rtf_region->closed = 1;

                    p_rtf_load_info->doc_regions++;
                }
            }
        }
    }

    if(status_ok(status = rtf_load_state_style_set_bit(p_rtf_load_info, &p_rtf_load_info->state_style, STYLE_SW_FS_BOLD, 0)))
    if(status_ok(status = rtf_load_state_style_set_bit(p_rtf_load_info, &p_rtf_load_info->state_style, STYLE_SW_FS_ITALIC, 0)))
    if(status_ok(status = rtf_load_state_style_set_bit(p_rtf_load_info, &p_rtf_load_info->state_style, STYLE_SW_FS_UNDERLINE, 0)))
    if(status_ok(status = rtf_load_state_style_set_bit(p_rtf_load_info, &p_rtf_load_info->state_style, STYLE_SW_FS_SUPERSCRIPT, 0)))
    if(status_ok(status = rtf_load_state_style_set_bit(p_rtf_load_info, &p_rtf_load_info->state_style, STYLE_SW_FS_SUBSCRIPT, 0)))
    if(status_ok(status = rtf_load_state_style_set_bit(p_rtf_load_info, &p_rtf_load_info->state_style, STYLE_SW_FS_SIZE_Y, 20))) /* \fs20 */
    if(status_ok(status = rtf_load_state_style_set_bit(p_rtf_load_info, &p_rtf_load_info->state_style, STYLE_SW_PS_MARGIN_LEFT, 32)))
    if(status_ok(status = rtf_load_state_style_set_bit(p_rtf_load_info, &p_rtf_load_info->state_style, STYLE_SW_PS_MARGIN_RIGHT, 32)))
    if(status_ok(status = rtf_load_state_style_set_bit(p_rtf_load_info, &p_rtf_load_info->state_style, STYLE_SW_PS_MARGIN_PARA, 0)))
                 status = rtf_load_state_style_set_bit(p_rtf_load_info, &p_rtf_load_info->state_style, STYLE_SW_PS_JUSTIFY, SF_JUSTIFY_LEFT);

    return(status);
}

/******************************************************************************
*
* \sectd: default section style apply
*
******************************************************************************/

PROC_RTF_CONTROL_PROTO(rtf_control_sectd)
{
    P_RTF_REGION p_rtf_region;
    STATUS status;

    func_ignore_parms();

    if(NULL == (p_rtf_region = al_array_extend_by(p_h_sub_regions, RTF_REGION, 1, PC_ARRAY_INIT_BLOCK_NONE, &status)))
        return(status);

    state_style_init(&p_rtf_region->state_style);
    docu_area_init(&p_rtf_region->docu_area);
    p_rtf_region->docu_area.tl = p_rtf_load_info->position;
    p_rtf_region->closed = 0;

    style_bit_set(&p_rtf_region->state_style.style, STYLE_SW_NAME);
    return(al_tstr_set(&p_rtf_region->state_style.style.h_style_name_tstr, TEXT("RTF Default Section")));
}

/******************************************************************************
*
* \deff<n>
*
* NB usually before fonts defined!
*
******************************************************************************/

PROC_RTF_CONTROL_PROTO(rtf_control_deff)
{
    S32 parameter = fast_strtol(a7str_parameter, NULL);

    func_ignore_parms();

    p_rtf_load_info->_deff = (U32) parameter;

    return(STATUS_OK);
}

/******************************************************************************
*
* \xxx{<n>}: standard style application
*
******************************************************************************/

PROC_RTF_CONTROL_PROTO(rtf_control_style)
{
    S32 parameter = 1;

    func_ignore_parms();

    if(CH_NULL != *a7str_parameter)
        parameter = fast_strtol(a7str_parameter, NULL);

    if((STYLE_SW_PS_JUSTIFY == (STYLE_BIT_NUMBER) arg) && (parameter == 1)) /* Special case for justification */
    {
        switch(a7str_control_word[1]) /*"q"*/
        {
        case 'l': parameter = SF_JUSTIFY_LEFT;   break;
        case 'c': parameter = SF_JUSTIFY_CENTRE; break;
        case 'r': parameter = SF_JUSTIFY_RIGHT;  break;
        case 'j': parameter = SF_JUSTIFY_BOTH;   break;
        default: default_unhandled(); break;
        }
    }

    if((STYLE_SW_PS_JUSTIFY_V == (STYLE_BIT_NUMBER) arg) && (parameter == 1)) /* Special case for vertical justification */
    {
        switch(a7str_control_word[6]) /*"vertal"*/
        {
        case 't':     parameter = SF_JUSTIFY_V_TOP;    break;
        case 'c':     parameter = SF_JUSTIFY_V_CENTRE; break;
        case CH_NULL: parameter = SF_JUSTIFY_V_BOTTOM; break;
      /*case 'j':*/
        default: default_unhandled(); break;
        }
    }

    switch(p_rtf_load_info->destination)
    {
    case RTF_DESTINATION_NORMAL:
    case RTF_DESTINATION_FLDRSLT:
        /* This means we've been called within the document */
        return(rtf_load_start_region(p_rtf_load_info, p_h_sub_regions, (STYLE_BIT_NUMBER) arg, parameter, &p_rtf_load_info->state_style));

    case RTF_DESTINATION_STYLESHEET:
        /* This means we've been called by the stylesheet builder */
        return(rtf_load_state_style_set_bit(p_rtf_load_info, &p_rtf_load_info->state_style, (STYLE_BIT_NUMBER) arg, parameter));

    default: default_unhandled();
        return(STATUS_OK);
    }
}

/******************************************************************************
*
* \f<n>: insert font (or define font number in \fonttbl or \stylesheet builders)
*
******************************************************************************/

PROC_RTF_CONTROL_PROTO(rtf_control_f)
{
    S32 parameter = fast_strtol(a7str_parameter, NULL);

    func_ignore_parms();

    switch(p_rtf_load_info->destination)
    {
    case RTF_DESTINATION_NORMAL:
    case RTF_DESTINATION_FLDRSLT:
        return(rtf_control_style(p_rtf_load_info, a7str_control_word, a7str_parameter, p_h_dest_contents, p_h_sub_regions, arg));

    case RTF_DESTINATION_FONTTBL:
        p_rtf_load_info->fonttbl_font_number = (U32) parameter;
        return(STATUS_OK);

    case RTF_DESTINATION_STYLESHEET:
        return(rtf_load_state_style_set_bit(p_rtf_load_info, &p_rtf_load_info->state_style, (STYLE_BIT_NUMBER) arg, parameter));

    default: default_unhandled();
        return(STATUS_OK);
    }
}

/******************************************************************************
*
* Set up default paragraph style
*
******************************************************************************/

_Check_return_
static STATUS
rtf_load_document_style(
    _InoutRef_  P_RTF_LOAD_INFO p_rtf_load_info)
{
    const P_DOCU p_docu = p_docu_from_docno(p_rtf_load_info->docno);
    STYLE style;
    DOCU_AREA docu_area;

    if(!p_rtf_load_info->inserting)
    {   /* Set up default paper size and margins (RTF Spec 1.9.1) if fully loading otherwise leave those in current document unaltered */
        PHYS_PAGE_DEF phys_page_def = p_docu->phys_page_def;

        ustr_xstrkpy(ustr_bptr(phys_page_def.ustr_name), sizeof32(phys_page_def.ustr_name), USTR_TEXT("RTF Page"));

        phys_page_def.size_x = 12240;
        phys_page_def.size_y = 15840;

        phys_page_def.margin_left    = 1440;
        phys_page_def.margin_right   = 1440;
        phys_page_def.margin_top     = 1440;
        phys_page_def.margin_bottom  = 1440;

        phys_page_def.landscape      = 0;
        phys_page_def.margin_oe_swap = 0;

        status_consume(update_from_phys_page_def(p_rtf_load_info, &phys_page_def));
    }

    style_init(&style);

    style.font_spec.bold = 0;
    style_bit_set(&style, STYLE_SW_FS_BOLD);
    style.font_spec.italic = 0;
    style_bit_set(&style, STYLE_SW_FS_ITALIC);
    style.font_spec.underline = 0;
    style_bit_set(&style, STYLE_SW_FS_UNDERLINE);
    style.font_spec.superscript = 0;
    style_bit_set(&style, STYLE_SW_FS_SUPERSCRIPT);
    style.font_spec.subscript = 0;
    style_bit_set(&style, STYLE_SW_FS_SUBSCRIPT);
    style.font_spec.size_y = (20 * PIXITS_PER_POINT) / 2; /* \fs20 */
    style_bit_set(&style, STYLE_SW_FS_SIZE_Y);

    style.para_style.justify = SF_JUSTIFY_LEFT;
    style_bit_set(&style, STYLE_SW_PS_JUSTIFY);
    style.para_style.justify_v = SF_JUSTIFY_V_TOP;
    style_bit_set(&style, STYLE_SW_PS_JUSTIFY_V);
    style.para_style.margin_left = 32;
    style_bit_set(&style, STYLE_SW_PS_MARGIN_LEFT);
    style.para_style.margin_right = 32;
    style_bit_set(&style, STYLE_SW_PS_MARGIN_RIGHT);
    style.para_style.margin_para = 0;
    style_bit_set(&style, STYLE_SW_PS_MARGIN_PARA);

    status_return(al_tstr_set(&style.h_style_name_tstr, TEXT("RTF Default Paragraph")));
    style_bit_set(&style, STYLE_SW_NAME);
    status_return(style_handle_add(p_docu, &style));

    /*style_init(&style);*/
    status_return(al_tstr_set(&style.h_style_name_tstr, TEXT("RTF Default Section")));
    style_bit_set(&style, STYLE_SW_NAME);
    status_return(style_handle_add(p_docu, &style));

#if 0
    if(p_rtf_load_info->doc_cols > 1)
    {
        docu_area_init(&docu_area);
        docu_area.whole_col = 1;
        docu_area.whole_row = 0;
        docu_area.tl.object_position.object_id = OBJECT_ID_NONE;

        style_init(&style);
        status_return(rtf_load_set_bit_in_style(p_docu, &style, STYLE_SW_CS_WIDTH, 0));

        docu_area.tl.slr.col = 1;
        docu_area.br.slr.col = p_rtf_load_info->doc_cols;
        docu_area.br.object_position.object_id = OBJECT_ID_NONE;

        {
        STYLE_DOCU_AREA_ADD_PARM style_docu_area_add_parm;
        STYLE_DOCU_AREA_ADD_STYLE(&style_docu_area_add_parm, &style);
        status_return(style_docu_area_add(p_docu, &docu_area, &style_docu_area_add_parm));
        } /*block*/
    }
#endif

    style_init(&style);

    docu_area_init(&docu_area);
    docu_area_set_whole_col(&docu_area);
    docu_area_set_whole_row(&docu_area);

    {
    STYLE_DOCU_AREA_ADD_PARM style_docu_area_add_parm;
    STYLE_DOCU_AREA_ADD_HANDLE(&style_docu_area_add_parm, style_handle_from_name(p_docu, TEXT("RTF Default Section")));
    status_return(style_docu_area_add(p_docu, &docu_area, &style_docu_area_add_parm));
    } /*block*/

    {
    STYLE_DOCU_AREA_ADD_PARM style_docu_area_add_parm;
    STYLE_DOCU_AREA_ADD_HANDLE(&style_docu_area_add_parm, style_handle_from_name(p_docu, TEXT("RTF Default Paragraph")));
    status_return(style_docu_area_add(p_docu, &docu_area, &style_docu_area_add_parm));
    } /*block*/

    if(!p_rtf_load_info->inserting)
        status_return(rtf_load_set_width(p_rtf_load_info));

    /* no style_free_resources since resources have been claimed by the region (I think!) */
    return(STATUS_OK);
}

_Check_return_
static STATUS
rtf_load_set_width(
    _InoutRef_  P_RTF_LOAD_INFO p_rtf_load_info)
{
    const P_DOCU p_docu = p_docu_from_docno(p_rtf_load_info->docno);
    STYLE style;
    DOCU_AREA docu_area;

    style_init(&style);

    style.col_style.width = p_docu->page_def.size_x - (p_docu->page_def.margin_left + p_docu->page_def.margin_right);
    style_bit_set(&style, STYLE_SW_CS_WIDTH);

    docu_area_init(&docu_area);
    docu_area_set_whole_col(&docu_area);
    docu_area.whole_row = 0;
    docu_area.tl.slr.col = 0;
    docu_area.tl.object_position.object_id = OBJECT_ID_NONE;
    docu_area.br.slr.col = 1;
    docu_area.br.object_position.object_id = OBJECT_ID_NONE;

    {
    STYLE_DOCU_AREA_ADD_PARM style_docu_area_add_parm;
    STYLE_DOCU_AREA_ADD_STYLE(&style_docu_area_add_parm, &style);
    return(style_docu_area_add(p_docu, &docu_area, &style_docu_area_add_parm));
    } /*block*/
}

/******************************************************************************
*
* \clbrdr{x}: add new table cell attributes for this cell.
*
******************************************************************************/

PROC_RTF_CONTROL_PROTO(rtf_control_clbrdr)
{
    const STYLE_BIT_NUMBER style_bit_number = (STYLE_BIT_NUMBER) arg;
    STATUS status;
    P_RTF_TABLE p_rtf_table = NULL;

    func_ignore_parms();

    if(0 != array_elements(&p_rtf_load_info->h_table_widths))
        p_rtf_table = array_ptr(&p_rtf_load_info->h_table_widths, RTF_TABLE, array_elements(&p_rtf_load_info->h_table_widths) - 1);

    PTR_ASSERT(p_rtf_table); /* keep next statement happy for CA */

    if((0 == array_elements(&p_rtf_load_info->h_table_widths)) || (p_rtf_table->width != 0))
    {
        /* This assumes that if the width has been set, the cell description is complete (i.e. \cellxn last) */
        if(NULL == (p_rtf_table = al_array_extend_by(&p_rtf_load_info->h_table_widths, RTF_TABLE, 1, PC_ARRAY_INIT_BLOCK_NONE, &status)))
            return(status);

        p_rtf_table->width = 0;
        p_rtf_table->top    = RTF_BORDER_DONT_CARE;
        p_rtf_table->bottom = RTF_BORDER_DONT_CARE;
        p_rtf_table->left   = RTF_BORDER_DONT_CARE;
        p_rtf_table->right  = RTF_BORDER_DONT_CARE;
    }

    switch(style_bit_number)
    {
    default: default_unhandled();                                      break;
    case STYLE_SW_PS_GRID_TOP:    p_rtf_table->top    = RTF_BORDER_ON; break;
    case STYLE_SW_PS_GRID_BOTTOM: p_rtf_table->bottom = RTF_BORDER_ON; break;
    case STYLE_SW_PS_GRID_LEFT:   p_rtf_table->left   = RTF_BORDER_ON; break;
    case STYLE_SW_PS_GRID_RIGHT:  p_rtf_table->right  = RTF_BORDER_ON; break;
    }

    return(STATUS_OK);
}

/******************************************************************************
*
* \cellx: add some width to the table width handle.
*
******************************************************************************/

PROC_RTF_CONTROL_PROTO(rtf_control_cellx)
{
    S32 parameter = fast_strtol(a7str_parameter, NULL);
    STATUS status;
    P_RTF_TABLE p_rtf_table = NULL;
    S32 width, tv;

    func_ignore_parms();

    width = parameter;

    for(tv = 0; tv < array_elements(&p_rtf_load_info->h_table_widths); tv++)
        width -= array_ptr(&p_rtf_load_info->h_table_widths, RTF_TABLE, tv)->width;

    if(array_elements(&p_rtf_load_info->h_table_widths))
        p_rtf_table = array_ptr(&p_rtf_load_info->h_table_widths, RTF_TABLE, array_elements(&p_rtf_load_info->h_table_widths)-1);

    if((NULL == p_rtf_table) || (p_rtf_table->width != 0))
    {
        if(NULL == (p_rtf_table = al_array_extend_by(&p_rtf_load_info->h_table_widths, RTF_TABLE, 1, PC_ARRAY_INIT_BLOCK_NONE, &status)))
            return(status);

        p_rtf_table->left = RTF_BORDER_DONT_CARE;
        p_rtf_table->top = RTF_BORDER_DONT_CARE;
        p_rtf_table->right = RTF_BORDER_DONT_CARE;
        p_rtf_table->bottom = RTF_BORDER_DONT_CARE;
    }

    p_rtf_table->width = width;

    return(STATUS_OK);
}

/******************************************************************************
*
* \trowd: delete all current table widths (more arriving!)
*
******************************************************************************/

PROC_RTF_CONTROL_PROTO(rtf_control_trowd)
{
    func_ignore_parms();

    al_array_shrink_by(&p_rtf_load_info->h_table_widths, -array_elements(&p_rtf_load_info->h_table_widths));

    return(STATUS_OK);
}

/******************************************************************************
*
* \trleft: set left table indent
*
******************************************************************************/

PROC_RTF_CONTROL_PROTO(rtf_control_trleft)
{
    S32 parameter = fast_strtol(a7str_control_word + 7, NULL); /* 7, since we require non-negative value (strlen("trleft-") == 6)**/

    func_ignore_parms();

    p_rtf_load_info->_trleft = parameter;

    return(STATUS_OK);
}

/******************************************************************************
*
* \intbl: apply region to get table widths correct
*
******************************************************************************/

PROC_RTF_CONTROL_PROTO(rtf_control_intbl)
{
    S32                        curr_col;
    STYLE                      style;
    DOCU_AREA                  docu_area;
    P_RTF_TABLE                p_rtf_table;

    func_ignore_parms();

    if(array_elements(&p_rtf_load_info->h_table_widths) == 0)
        return(STATUS_OK);

    docu_area_init(&docu_area);

    p_rtf_load_info->in_table = TRUE;
    docu_area.tl.slr.row = p_rtf_load_info->position.slr.row;
    docu_area.br.slr.row = docu_area.tl.slr.row + 1;

/* table indent (first column in Wordz) */

    docu_area.tl.slr.col = 0;
    docu_area.br.slr.col = 1;

    style_init(&style);
    style.col_style.width = p_rtf_load_info->_trleft;
    style_bit_set(&style, STYLE_SW_CS_WIDTH);

    {
    STYLE_DOCU_AREA_ADD_PARM style_docu_area_add_parm;
    STYLE_DOCU_AREA_ADD_STYLE(&style_docu_area_add_parm, &style);
    status_return(style_docu_area_add(p_docu_from_docno(p_rtf_load_info->docno), &docu_area, &style_docu_area_add_parm));
    } /*block*/

 /* Now table widths */

    for(curr_col = 0; curr_col < array_elements(&p_rtf_load_info->h_table_widths); curr_col++)
    {
        p_rtf_table = array_ptr(&p_rtf_load_info->h_table_widths, RTF_TABLE, curr_col);

        docu_area.tl.slr.col = (COL) curr_col+1;
        docu_area.br.slr.col = (COL) curr_col+2;
        style_init(&style);

        style.col_style.width = p_rtf_table->width;
        style_bit_set(&style, STYLE_SW_CS_WIDTH);

#if 0
        if(0 == style_bit_number)
        {
            if(p_rtf_table->top != RTF_BORDER_DONT_CARE)
            {
                status_return(rtf_load_set_bit_in_style(p_docu, &style, STYLE_SW_PS_GRID_TOP, p_rtf_table->top));
            }
            if(p_rtf_table->bottom != RTF_BORDER_DONT_CARE)
            {
                status_return(rtf_load_set_bit_in_style(p_docu, &style, STYLE_SW_PS_GRID_BOTTOM, p_rtf_table->bottom));
            }
        }
        if(p_rtf_table->left != RTF_BORDER_DONT_CARE)
        {
            status_return(rtf_load_set_bit_in_style(p_docu, &style, STYLE_SW_PS_GRID_LEFT, p_rtf_table->left));
        }
        if(p_rtf_table->right != RTF_BORDER_DONT_CARE)
        {
            status_return(rtf_load_set_bit_in_style(p_docu, &style, STYLE_SW_PS_GRID_RIGHT, p_rtf_table->right));
        }
#endif
        {
        STYLE_DOCU_AREA_ADD_PARM style_docu_area_add_parm;
        STYLE_DOCU_AREA_ADD_STYLE(&style_docu_area_add_parm, &style);
        status_return(style_docu_area_add(p_docu_from_docno(p_rtf_load_info->docno), &docu_area, &style_docu_area_add_parm));
        } /*block*/
    }

    if(p_rtf_load_info->position.slr.col == 0)
    {
        p_rtf_load_info->position.slr.col++;
        object_position_init(&p_rtf_load_info->position.object_position);
    }

    return(STATUS_OK);
}

/******************************************************************************
*
* \brdrcf<n>: change global grid colour
*
******************************************************************************/

PROC_RTF_CONTROL_PROTO(rtf_control_brdrcf)
{
    S32 parameter = fast_strtol(a7str_parameter, NULL);

    func_ignore_parms();

    p_rtf_load_info->grid_colour = parameter;

    return(STATUS_OK);
}

/******************************************************************************
*
* \footnote: (destination) store footnote to go at end
*
******************************************************************************/

PROC_RTF_CONTROL_PROTO(rtf_control_footnote)
{
    STATUS status;
    const S32 initial_bracket_level = p_rtf_load_info->bracket_level;
    P_ARRAY_HANDLE p_h_footnote_text;
    S32            ftn_number;
    SC_ARRAY_INIT_BLOCK array_init_block_region = aib_init(2, sizeof32(RTF_REGION), 1);
    U8Z            nbr_array[20];
    P_RTF_REGION   p_rtf_region;
    P_ARRAY_HANDLE p_array_handle;

    func_ignore_parms();

    if(NULL == (p_array_handle = al_array_extend_by_ARRAY_HANDLE(&p_rtf_load_info->h_da_list, 1, PC_ARRAY_INIT_BLOCK_NONE, &status)))
        return(status);

    if(NULL == (p_rtf_region = al_array_extend_by(p_array_handle, RTF_REGION, 1, &array_init_block_region, &status)))
        return(status);

    docu_area_init(&p_rtf_region->docu_area);
    state_style_init(&p_rtf_region->state_style);
    style_bit_set(&p_rtf_region->state_style.style, STYLE_SW_FS_SUPERSCRIPT);
    p_rtf_region->state_style.style.font_spec.superscript = 1;
    p_rtf_region->docu_area.tl = p_rtf_load_info->position;

    ftn_number = array_elements(&p_rtf_load_info->h_footnotes)+1;
    consume_int(xsnprintf(nbr_array, sizeof32(nbr_array), S32_FMT, ftn_number));

    status_return(rtf_array_add_bytes(p_h_dest_contents, strlen32(nbr_array), nbr_array));

    p_rtf_load_info->position.object_position.data += strlen32(nbr_array);
    p_rtf_region->docu_area.br = p_rtf_load_info->position;
    p_rtf_region->docu_area.br.slr.col++;
    p_rtf_region->docu_area.br.slr.row++;

    if(NULL == (p_h_footnote_text = al_array_extend_by_ARRAY_HANDLE(&p_rtf_load_info->h_footnotes, 1, PC_ARRAY_INIT_BLOCK_NONE, &status)))
        return(status);

    /* Stick number at start of footnote text. */
    status_return(rtf_array_add_bytes(p_h_footnote_text, strlen32(nbr_array), nbr_array));

    {
    U8 sep_chars[2] = { CH_FULL_STOP, CH_SPACE };
    status_return(rtf_array_add_bytes(p_h_footnote_text, 2, sep_chars));
    } /*block*/

    while(status_ok(status))
    {
        U8 rtf_char = rtf_get_char(p_rtf_load_info);

        if(rtf_char < CH_SPACE)
        {
            switch(rtf_char)
            {
            case CH_NULL:
                myassert2(TEXT("RTF(%d): EOF read before \\%s terminated"), p_rtf_load_info->bracket_level, report_sbstr(a7str_control_word));
                status = create_error(OB_RTF_ERR_MISMATCHED_BRACKETS);
                rtf_advance_char(p_rtf_load_info, -1);
                p_rtf_load_info->bracket_level = -1;
                break;

            default: default_unhandled();
#if CHECKING
            case LF:
            case CR:
                /* These are ignored */
#endif
                break;
            }
        }
        else
        {
            switch(rtf_char)
            {
            default:
                /* Just accumulate text for this destination */
                assert(u8_is_ascii7(rtf_char));
                if(0 != p_rtf_load_info->ignore_chars_count)
                {
                    trace_2(TRACE_IO_RTF, TEXT("RTF(%d): ignore_chars_count=") U32_TFMT, p_rtf_load_info->bracket_level, p_rtf_load_info->ignore_chars_count);
                    p_rtf_load_info->ignore_chars_count--;
                    break;
                }

                status = rtf_array_add_bytes(p_h_footnote_text, 1, &rtf_char);
                break;

            case CH_LEFT_CURLY_BRACKET:  p_rtf_load_info->bracket_level++; break;
            case CH_RIGHT_CURLY_BRACKET: p_rtf_load_info->bracket_level--; break;

            case CH_BACKWARDS_SLASH:
                {
                BOOL ignorable_destination = FALSE;
                U32 parameter_offset;
                QUICK_BLOCK_WITH_BUFFER(quick_block, 32);
                quick_block_with_buffer_setup(quick_block);

                if(status_ok(status = rtf_load_obtain_control_word(p_rtf_load_info, &quick_block, &parameter_offset, &ignorable_destination)))
                {
#if TRACE_ALLOWED
                    PC_A7STR inner_control_word = quick_block_str(&quick_block);

                    trace_3(TRACE_IO_RTF, TEXT("RTF(%d): Discarding control word \\%s in \\%s"), p_rtf_load_info->bracket_level, report_sbstr(inner_control_word), report_sbstr(a7str_control_word));
#endif
                }

                quick_block_dispose(&quick_block);
                break;
                }
            }
        }

        if(p_rtf_load_info->bracket_level < initial_bracket_level)
            break;
    }

    if(status_ok(status))
        status = rtf_array_add_NULLCH(p_h_footnote_text);

    /* Ought to try to put a footnote marker in the text */

    /* All the footnote text is added later */
    return(status);
}

/******************************************************************************
*
* Output any footnotes
*
******************************************************************************/

_Check_return_
static STATUS
rtf_load_finalise_footnotes(
    _InoutRef_  P_RTF_LOAD_INFO p_rtf_load_info)
{
    ARRAY_INDEX i;

    for(i = 0; i < array_elements(&p_rtf_load_info->h_footnotes); ++i)
    {
        P_ARRAY_HANDLE p_h_footnote_text = array_ptr(&p_rtf_load_info->h_footnotes, ARRAY_HANDLE, i);

        {
        LOAD_CELL_FOREIGN load_cell_foreign;
        load_cell_foreign_init(p_rtf_load_info, &load_cell_foreign, ustr_inline_from_h_ustr(p_h_footnote_text));
        status_assert(insert_cell_contents_foreign(p_docu_from_docno(p_rtf_load_info->docno), OBJECT_ID_TEXT, T5_MSG_LOAD_CELL_FOREIGN, &load_cell_foreign));
        } /*block*/

        p_rtf_load_info->position.slr.row++;
        object_position_init(&p_rtf_load_info->position.object_position);

        al_array_dispose(p_h_footnote_text);
    }

    al_array_dispose(&p_rtf_load_info->h_footnotes);

    return(STATUS_OK);
}

/******************************************************************************
*
* Called to insert an RTF file into a Fireworkz document
*
******************************************************************************/

T5_MSG_PROTO(static, rtf_msg_insert_foreign, _InRef_ P_MSG_INSERT_FOREIGN p_msg_insert_foreign)
{
    STATUS status = STATUS_OK;
    RTF_LOAD_INFO rtf_load_info;
    P_RTF_LOAD_INFO p_rtf_load_info;
    P_POSITION p_position = &p_msg_insert_foreign->position;
    PCTSTR p_pathname = p_msg_insert_foreign->filename;
    ARRAY_HANDLE h_data = p_msg_insert_foreign->array_handle;
    BOOL data_allocated = FALSE;
    DOCU_AREA docu_area_to_insert;
    POSITION position_actual;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    docu_area_init(&docu_area_to_insert);

    zero_struct(rtf_load_info);

    p_rtf_load_info = &rtf_load_info;

    rtf_load_info.p_msg_insert_foreign = p_msg_insert_foreign;

    rtf_load_info.docno = docno_from_p_docu(p_docu);

    rtf_load_info.inserting = p_msg_insert_foreign->insert;

    process_status_init(&rtf_load_info.process_status);

    if(0 != h_data)
    {
        rtf_load_info.process_status.flags.foreground = TRUE;
        rtf_load_info.process_status.reason.type = UI_TEXT_TYPE_RESID;
        rtf_load_info.process_status.reason.text.resource_id = MSG_STATUS_CONVERTING; /* that's what we're about to do */
        process_status_begin(p_docu, &rtf_load_info.process_status, PROCESS_STATUS_PERCENT);
    }
    else
    {
        if(status_ok(status = file_memory_load(p_docu, &h_data, p_pathname, &rtf_load_info.process_status, 8)))
        {
            data_allocated = TRUE;
        }
    }

    if(status_ok(status))
        status = rtf_array_add_NULLCH(&h_data); /* Terminate 'cos we might walk off the end */

    if(status_ok(status))
    {
        /* Find size of docu_area to insert */
        if(status_ok(status = rtf_load_find_document_dimensions(p_rtf_load_info, &h_data, &docu_area_to_insert)))
        {
            p_rtf_load_info->doc_cols = docu_area_to_insert.br.slr.col - docu_area_to_insert.tl.slr.col;
            p_rtf_load_info->doc_rows = docu_area_to_insert.br.slr.row - docu_area_to_insert.tl.slr.row;

            if(status_ok(status = cells_docu_area_insert(p_docu, p_position, &docu_area_to_insert, &position_actual)))
            {
                rtf_load_info.position = rtf_load_info.load_position = position_actual;

                status = format_col_row_extents_set(p_docu, MAX(p_rtf_load_info->doc_cols, n_cols_logical(p_docu)), n_rows(p_docu));

                if(status_ok(status))
                    status = rtf_load_load_file(p_rtf_load_info, &h_data);

                if(status_ok(status))
                    status = rtf_load_finalise_footnotes(p_rtf_load_info);
            }
        }
    }

    al_array_dispose(&rtf_load_info.h_table_widths);

    {
    const ARRAY_INDEX n_elements = array_elements32(&rtf_load_info.h_rtf_font_table);
    ARRAY_INDEX i;
    P_RTF_FONT_TABLE p_rtf_font_table = array_range(&rtf_load_info.h_rtf_font_table, RTF_FONT_TABLE, 0, n_elements);

    for(i = 0; i < n_elements; ++i, ++p_rtf_font_table)
    {
        al_array_dispose(&p_rtf_font_table->h_rtf_font_name_tstr);
    }
    al_array_dispose(&rtf_load_info.h_rtf_font_table);
    } /*block*/

    al_array_dispose(&rtf_load_info.h_colour_table_load);

    {
    const ARRAY_INDEX n_elements = array_elements32(&rtf_load_info.h_stylesheet_load);
    ARRAY_INDEX i;
    P_RTF_STYLESHEET p_rtf_stylesheet = array_range(&rtf_load_info.h_stylesheet_load, RTF_STYLESHEET, 0, n_elements);

    for(i = 0; i < n_elements; ++i, ++p_rtf_stylesheet)
    {
        al_array_dispose(&p_rtf_stylesheet->h_style_name_tstr);
    }
    al_array_dispose(&rtf_load_info.h_stylesheet_load);
    } /*block*/

    if(0 != rtf_load_info.h_footnotes)
    {
        ARRAY_INDEX index = array_elements(&rtf_load_info.h_footnotes);

        while(--index >= 0)
            al_array_dispose(array_ptr(&rtf_load_info.h_footnotes, ARRAY_HANDLE, index));

        al_array_dispose(&rtf_load_info.h_footnotes);
    }

    if(0 != rtf_load_info.h_da_list)
    {
        ARRAY_INDEX index = array_elements(&rtf_load_info.h_da_list);
        myassert0(TEXT("RTF: region list not released"));
        while(--index >= 0)
        {
            const P_ARRAY_HANDLE p_region_list = array_ptr(&rtf_load_info.h_da_list, ARRAY_HANDLE, index);
            S32 region_idx;
            myassert0(TEXT("RTF: region list not empty"));
            for(region_idx = 0; region_idx < array_elements(p_region_list); region_idx++)  /* PARANOIA!! */
            {
                const P_RTF_REGION p_rtf_region = array_ptr(p_region_list, RTF_REGION, region_idx);
                style_free_resources_all(&p_rtf_region->state_style.style);
            }
            al_array_dispose(p_region_list);
        }
        al_array_dispose(&rtf_load_info.h_da_list);
    }

    process_status_end(&rtf_load_info.process_status);

    if(data_allocated) al_array_dispose(&h_data);

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
        rtf_msg_startup_sort();
        return(resource_init(OBJECT_ID_FL_RTF, MSG_WEAK, P_BOUND_RESOURCES_OBJECT_ID_FL_RTF));

    case T5_MSG_IC__EXIT1:
        return(resource_close(OBJECT_ID_FL_RTF));

    default:
        return(STATUS_OK);
    }
}

OBJECT_PROTO(extern, object_fl_rtf);
OBJECT_PROTO(extern, object_fl_rtf)
{
    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        return(rtf_msg_initclose(p_docu, t5_message, (PC_MSG_INITCLOSE) p_data));

    case T5_MSG_INSERT_FOREIGN:
        return(rtf_msg_insert_foreign(p_docu, t5_message, (P_MSG_INSERT_FOREIGN) p_data));

    default:
        return(STATUS_OK);
    }
}

/* end of fl_rtf.c */
