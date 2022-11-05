/* sk_table.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Tables for Fireworkz skeleton */

/* SKS April 1992 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

/*
construct argument types, common ones exported
*/

static const ARG_TYPE
args_bool_mandatory[] =
{
    ARG_TYPE_BOOL | ARG_MANDATORY,
    ARG_TYPE_NONE
};

static const ARG_TYPE
args_u8n_mandatory[] =
{
    ARG_TYPE_U8N | ARG_MANDATORY,
    ARG_TYPE_NONE
};

static const ARG_TYPE
args_s32[] =
{
    ARG_TYPE_S32,
    ARG_TYPE_NONE
};

static const ARG_TYPE
args_s32_mandatory[] =
{
    ARG_TYPE_S32 | ARG_MANDATORY,
    ARG_TYPE_NONE
};

static const ARG_TYPE
args_ctypetable[] =
{
    ARG_TYPE_U8N | ARG_MANDATORY, /* character */
    ARG_TYPE_U8N, /* ctype byte */
    ARG_TYPE_U8N, /* lower case */
    ARG_TYPE_U8N, /* upper case */
    ARG_TYPE_U8N, /* sort order byte */
    ARG_TYPE_NONE
};

static const ARG_TYPE
args_uiload[] =
{
    ARG_TYPE_S32 | ARG_MANDATORY,
    ARG_TYPE_USTR | ARG_MANDATORY,
    ARG_TYPE_USTR,
    ARG_TYPE_NONE
};

static const ARG_TYPE
args_bind_file_type[] =
{
    ARG_TYPE_S32 | ARG_MANDATORY,   /* load, save, insert mask */
    ARG_TYPE_X32 | ARG_MANDATORY,   /* file type */
    ARG_TYPE_USTR | ARG_MANDATORY,  /* descriptive string displayed (for saver / loader) */
    ARG_TYPE_TSTR,                  /* optional extra string appended for saver / loader (Windows) */
    ARG_TYPE_TSTR,                  /* optional wildcard spec for saver / loader (Windows) */
    ARG_TYPE_NONE
};

static const ARG_TYPE
args_object_bind_save[] =
{
    ARG_TYPE_S32 | ARG_MANDATORY,   /* object id */
    ARG_TYPE_X32 | ARG_MANDATORY,   /* file type */
    ARG_TYPE_TSTR,                  /* optional clipboard format to register with Windows clipboard */
    ARG_TYPE_NONE
};

static const ARG_TYPE
args_object_bind_load[] =
{
    ARG_TYPE_S32 | ARG_MANDATORY,   /* object id */
    ARG_TYPE_X32 | ARG_MANDATORY,   /* file type */
    ARG_TYPE_TSTR | ARG_MANDATORY,  /* file extension */
    ARG_TYPE_TSTR,                  /* optional template file to use */
    ARG_TYPE_NONE
};

static const ARG_TYPE
args_object_bind_construct[] =
{
    ARG_TYPE_S32 | ARG_MANDATORY,   /* object id */
    ARG_TYPE_U8C | ARG_MANDATORY,   /* constructor */
    ARG_TYPE_NONE
};

static const ARG_TYPE
args_s32_s32[] =
{
    ARG_TYPE_S32,
    ARG_TYPE_S32,
    ARG_TYPE_NONE
};

static const ARG_TYPE
args_rgb[] =
{
    ARG_TYPE_S32 | ARG_MANDATORY,
    ARG_TYPE_S32 | ARG_MANDATORY,
    ARG_TYPE_S32 | ARG_MANDATORY,
    ARG_TYPE_S32,

    ARG_TYPE_NONE
};

static const ARG_TYPE
args_s32_s32_s32_s32[] =
{
    ARG_TYPE_S32,
    ARG_TYPE_S32,
    ARG_TYPE_S32,
    ARG_TYPE_S32,
    ARG_TYPE_NONE
};

static const ARG_TYPE
args_ustr[] =
{
    ARG_TYPE_USTR,
    ARG_TYPE_NONE
};

static const ARG_TYPE
args_tstr_s32_tstr[] =
{
    ARG_TYPE_TSTR,
    ARG_TYPE_S32,
    ARG_TYPE_TSTR,
    ARG_TYPE_NONE
};

static const ARG_TYPE
args_cmd_of_cell[] =
{
    ARG_TYPE_U8C,
    ARG_TYPE_U8C,
#if 1
    ARG_TYPE_COL,
    ARG_TYPE_ROW,
#else
    ARG_TYPE_S32, /* NB NOT COL */
    ARG_TYPE_S32, /* NB NOT ROW */
#endif
    ARG_TYPE_USTR,
    ARG_TYPE_USTR,
    ARG_TYPE_USTR,
    ARG_TYPE_NONE
};

static const ARG_TYPE
args_cmd_box[] =
{
    ARG_TYPE_BOOL,
    ARG_TYPE_BOOL,

    ARG_TYPE_BOOL,
    ARG_TYPE_BOOL,
    ARG_TYPE_BOOL,

    ARG_TYPE_BOOL,
    ARG_TYPE_BOOL,
    ARG_TYPE_BOOL,

    ARG_TYPE_S32,
    ARG_TYPE_X32,

    ARG_TYPE_NONE
};

static const ARG_TYPE
args_cmd_define_key[] =
{
    ARG_TYPE_TSTR | ARG_MANDATORY,
    ARG_TYPE_USTR | ARG_MANDATORY,
    ARG_TYPE_NONE
};

static const ARG_TYPE
args_cmd_define_key_raw[] =
{
    ARG_TYPE_TSTR | ARG_MANDATORY,
    ARG_TYPE_RAW,
    ARG_TYPE_NONE
};

static const ARG_TYPE
args_cmd_current_posn[] =
{
    ARG_TYPE_COL | ARG_MANDATORY, /* col */
    ARG_TYPE_ROW | ARG_MANDATORY, /* row */
    ARG_TYPE_S32, /* data */
    ARG_TYPE_S32, /* object_id */
    ARG_TYPE_S32, /* more_data */
    ARG_TYPE_NONE
};

static const ARG_TYPE
args_cmd_view_control[] =
{
    ARG_TYPE_S32, /* scale percentage */
    ARG_TYPE_S32,
    ARG_TYPE_BOOL,
    ARG_TYPE_BOOL,
    ARG_TYPE_BOOL,
    ARG_TYPE_BOOL,
    ARG_TYPE_BOOL,
    ARG_TYPE_S32,
    ARG_TYPE_BOOL,
    ARG_TYPE_S32,
    ARG_TYPE_NONE
};

static const ARG_TYPE
args_cmd_view_scale[] =
{
    ARG_TYPE_S32, /* scale percentage */
    ARG_TYPE_NONE
};

static const ARG_TYPE
args_cmd_view_scroll[] =
{
    ARG_TYPE_S32, /* x page 1  */
    ARG_TYPE_S32, /* x pixit 1 */
    ARG_TYPE_S32, /* y page 1  */
    ARG_TYPE_S32, /* y pixit 1 */

    ARG_TYPE_S32, /* x page 2  */
    ARG_TYPE_S32, /* x pixit 2 */
    ARG_TYPE_S32, /* y page 2  */
    ARG_TYPE_S32, /* y pixit 2 */

    ARG_TYPE_NONE
};

static const ARG_TYPE
args_cmd_note[] =
{
    ARG_TYPE_U8C,       /* object type D (DrawFile) */
    ARG_TYPE_U8C,
    ARG_TYPE_S32,       /* layer */
    ARG_TYPE_S32,       /* mounting 1            2            3 4 5        */
    ARG_TYPE_S32,       /*          tl pixit.x   tl pixit.x   tl pixit.x   */
    ARG_TYPE_S32,       /*          tl pixit.y   tl pixit.y   tl pixit.y   */
    ARG_TYPE_S32,       /*          tl col       tl col       width        */
    ARG_TYPE_S32,       /*          tl row       tl row       height       */
    ARG_TYPE_S32,       /*          width        br pixit.x   scale_to_fit */
    ARG_TYPE_S32,       /*          height       br pixit.y   all_pages    */
    ARG_TYPE_S32,       /*          -            br col       -            */
    ARG_TYPE_S32,       /*          -            br row       -            */
    ARG_TYPE_S32,       /* ref */
    ARG_TYPE_S32,       /* don't print */
    ARG_TYPE_S32,       /* scale x 16.16 */
    ARG_TYPE_S32,       /* scale y 16.16 */
    ARG_TYPE_S32,       /* width  (from 2.01) */
    ARG_TYPE_S32,       /* height (from 2.01) */
    ARG_TYPE_NONE
};

static const ARG_TYPE
args_ustr_mandatory[] =
{
    ARG_TYPE_USTR | ARG_MANDATORY,
    ARG_TYPE_NONE
};

static const ARG_TYPE
args_ustr_mandorblk[] =
{
    ARG_TYPE_USTR | ARG_MANDATORY_OR_BLANK,
    ARG_TYPE_NONE
};

static const ARG_TYPE
args_tstr_mandatory[] =
{
    ARG_TYPE_TSTR | ARG_MANDATORY,
    ARG_TYPE_NONE
};

static const ARG_TYPE
args_tstr_mandorblk[] =
{
    ARG_TYPE_TSTR | ARG_MANDATORY_OR_BLANK,
    ARG_TYPE_NONE
};

static const ARG_TYPE
args_cmd_button[] =
{
    ARG_TYPE_TSTR | ARG_MANDATORY,
    ARG_TYPE_S32, /* expanded from BOOL */
    ARG_TYPE_NONE
};

static const ARG_TYPE
args_cmd_load[] =
{
    ARG_TYPE_TSTR | ARG_MANDATORY,
    ARG_TYPE_BOOL | ARG_OPTIONAL,
    ARG_TYPE_NONE
};

/* no args_cmd_save */

static const ARG_TYPE
args_cmd_save_as[] =
{
    ARG_TYPE_TSTR | ARG_MANDATORY,          /* filename */
    ARG_TYPE_X32,                           /* t5_filetype */
    ARG_TYPE_BOOL,                          /* save_selection */
    ARG_TYPE_NONE
};

static const ARG_TYPE
args_cmd_save_as_intro[] =
{
    ARG_TYPE_X32,                           /* t5_filetype */ /* can't be ARG_MANDATORY as it's on a toolbar button that has no way of passing args */
    ARG_TYPE_NONE
};

static const ARG_TYPE
args_cmd_save_as_template[] =
{
    ARG_TYPE_TSTR | ARG_MANDATORY,          /* filename */
    ARG_TYPE_S32  | ARG_MANDATORY,          /* SAVING_TEMPLATE_xxx */
    ARG_TYPE_TSTR,                          /* style_name, iff SAVING_TEMPLATE_ONE_STYLE */
    ARG_TYPE_NONE
};

/* no args_cmd_save_as_template_intro */

static const ARG_TYPE
args_cmd_save_as_foreign[] =
{
    ARG_TYPE_TSTR | ARG_MANDATORY,          /* filename */
    ARG_TYPE_X32  | ARG_MANDATORY,          /* t5_filetype */
    ARG_TYPE_BOOL,                          /* save_selection */
    ARG_TYPE_NONE
};

static const ARG_TYPE
args_cmd_save_as_foreign_intro[] =
{
    ARG_TYPE_X32 | ARG_MANDATORY,           /* t5_filetype */
    ARG_TYPE_TSTR,                          /* suggested leafname */
    ARG_TYPE_NONE
};

static const ARG_TYPE
args_cmd_save_as_drawfile[] =
{
    ARG_TYPE_TSTR | ARG_MANDATORY,          /* filename */
    ARG_TYPE_TSTR,                          /* page range (may be absent/empty -> all pages) */
    ARG_TYPE_NONE
};

static const ARG_TYPE
args_cmd_save_as_drawfile_intro[] =
{
    ARG_TYPE_TSTR,                          /* suggested leafname */
    ARG_TYPE_NONE
};

static const ARG_TYPE
args_cmd_save_picture[] =
{
    ARG_TYPE_TSTR | ARG_MANDATORY_OR_BLANK, /* filename */
    ARG_TYPE_X32  | ARG_MANDATORY,          /* t5_filetype */
    ARG_TYPE_NONE
};

/* no args_cmd_save_picture_intro */

static const ARG_TYPE
args_cmd_of_block[] =
{
    /* NB. these args are NOT COL,ROW as COL,ROW arg processing during loading offsets using the original_docu_area set up by this construct */
    ARG_TYPE_S32 | ARG_MANDATORY, /* tl.col */
    ARG_TYPE_S32 | ARG_MANDATORY, /* tl.row */
    ARG_TYPE_S32,                 /* tl.object_data */
    ARG_TYPE_S32 | ARG_MANDATORY, /* tl.object_id */

    ARG_TYPE_S32 | ARG_MANDATORY, /* br.col */
    ARG_TYPE_S32 | ARG_MANDATORY, /* br.row */
    ARG_TYPE_S32,                 /* br.object_data */
    ARG_TYPE_S32 | ARG_MANDATORY, /* br.object_id */

    ARG_TYPE_S32 | ARG_MANDATORY, /* n_src_cols */
    ARG_TYPE_S32 | ARG_MANDATORY, /* n_src_rows */

    ARG_TYPE_U8C,
    ARG_TYPE_U8C,
    ARG_TYPE_USTR,

    ARG_TYPE_U8C,
    ARG_TYPE_U8C,
    ARG_TYPE_USTR,

    ARG_TYPE_NONE
};

static const ARG_TYPE
args_cmd_of_region[] =
{
    ARG_TYPE_S32,       /* tl.col */
    ARG_TYPE_S32,       /* tl.row */
    ARG_TYPE_S32,       /* tl.object_data */
    ARG_TYPE_S32,       /* tl.object_id */

    ARG_TYPE_S32,       /* br.col */
    ARG_TYPE_S32,       /* br.row */
    ARG_TYPE_S32,       /* br.object_data */
    ARG_TYPE_S32,       /* br.object_id */

    ARG_TYPE_BOOL,      /* whole col */
    ARG_TYPE_BOOL,      /* whole row */

    ARG_TYPE_USTR,

    ARG_TYPE_NONE
};

static const ARG_TYPE
args_cmd_of_implied[] =
{
    ARG_TYPE_S32,                       /* tl.col */
    ARG_TYPE_S32,                       /* tl.row */
    ARG_TYPE_S32,                       /* tl.object_data */
    ARG_TYPE_S32,                       /* tl.object_id */

    ARG_TYPE_S32,                       /* br.col */
    ARG_TYPE_S32,                       /* br.row */
    ARG_TYPE_S32,                       /* br.object_data */
    ARG_TYPE_S32,                       /* br.object_id */

    ARG_TYPE_BOOL,                      /* whole col */
    ARG_TYPE_BOOL,                      /* whole row */

    ARG_TYPE_S32 | ARG_MANDATORY,       /* object_id */
    ARG_TYPE_S32 | ARG_MANDATORY,       /* message no */

    ARG_TYPE_S32,                       /* arg */
    ARG_TYPE_U8N | ARG_MANDATORY,       /* region_class */

    ARG_TYPE_USTR,

    ARG_TYPE_NONE
};

static const ARG_TYPE
args_cmd_of_style[] =
{
    ARG_TYPE_TSTR | ARG_MANDATORY,
    ARG_TYPE_USTR,
    ARG_TYPE_NONE
};

static const ARG_TYPE
args_cmd_of_version[] =
{
    ARG_TYPE_TSTR | ARG_MANDATORY,
    ARG_TYPE_TSTR | ARG_MANDATORY,
    ARG_TYPE_TSTR,
    ARG_TYPE_TSTR,
    ARG_TYPE_TSTR,
    ARG_TYPE_TSTR,
    ARG_TYPE_NONE
};

static const ARG_TYPE
args_cmd_of_flow[] =
{
    ARG_TYPE_S32 | ARG_MANDATORY, /* object id of flow object */
    ARG_TYPE_NONE
};

static const ARG_TYPE
args_cmd_backdrop[] =
{
    ARG_TYPE_S32 | ARG_MANDATORY,  /* mounting     */
    ARG_TYPE_BOOL | ARG_MANDATORY, /* scale_to_fit */
    ARG_TYPE_S32 | ARG_MANDATORY,  /* all_pages    */
    ARG_TYPE_BOOL | ARG_MANDATORY, /* print        */
    ARG_TYPE_NONE
};

static const ARG_TYPE
args_cmd_docu_area[] =
{
    ARG_TYPE_S32 | ARG_MANDATORY,       /* tl.col */
    ARG_TYPE_S32 | ARG_MANDATORY,       /* tl.row */
    ARG_TYPE_S32,                       /* tl.object_data */
    ARG_TYPE_S32,                       /* tl.object_id */

    ARG_TYPE_S32 | ARG_MANDATORY,       /* br.col */
    ARG_TYPE_S32 | ARG_MANDATORY,       /* br.row */
    ARG_TYPE_S32,                       /* br.object_data */
    ARG_TYPE_S32,                       /* br.object_id */

    ARG_TYPE_BOOL,                      /* whole col */
    ARG_TYPE_BOOL,                      /* whole row */

    ARG_TYPE_NONE
};

static const ARG_TYPE
args_cmd_menu_add[] =
{
    ARG_TYPE_TSTR | ARG_MANDATORY, /* adding to this menu */
    ARG_TYPE_TSTR,  /* textual entry name */
    ARG_TYPE_TSTR,  /* RHS of entry string */
    ARG_TYPE_USTR,  /* macro script for this entry */
    ARG_TYPE_USTR,  /* another optional macro script for this entry */
    ARG_TYPE_U8N,   /* optional */
    ARG_TYPE_NONE
};

static const ARG_TYPE
args_cmd_menu_add_raw[] =
{
    ARG_TYPE_TSTR | ARG_MANDATORY, /* adding to this menu */
    ARG_TYPE_TSTR,  /* textual entry name */
    ARG_TYPE_TSTR,  /* RHS of entry string */
    ARG_TYPE_RAW,   /* macro script for this entry */
    ARG_TYPE_RAW,   /* another optional macro script for this entry */
    ARG_TYPE_U8N,   /* optional */
    ARG_TYPE_NONE
};

static const ARG_TYPE
args_cmd_menu_delete[] =
{
    ARG_TYPE_TSTR | ARG_MANDATORY, /* adding to this menu */
    ARG_TYPE_TSTR,  /* textual entry name */
    ARG_TYPE_NONE
};

static const ARG_TYPE
args_cmd_menu_name[] =
{
    ARG_TYPE_TSTR | ARG_MANDATORY, /* adding to this menu */
    ARG_TYPE_TSTR | ARG_MANDATORY, /* textual entry name */
    ARG_TYPE_NONE
};

static const ARG_TYPE
args_cmd_paper[] =
{
    ARG_TYPE_USTR,
    ARG_TYPE_S32,
    ARG_TYPE_F64,
    ARG_TYPE_F64,
    ARG_TYPE_F64,
    ARG_TYPE_F64,
    ARG_TYPE_F64,
    ARG_TYPE_F64,
    ARG_TYPE_F64,
    ARG_TYPE_BOOL,
    ARG_TYPE_F64,
    ARG_TYPE_F64,
    ARG_TYPE_F64,
    ARG_TYPE_S32,
    ARG_TYPE_BOOL,
    ARG_TYPE_NONE
};

static const ARG_TYPE
args_cmd_paper_scale[] =
{
    ARG_TYPE_S32,
    ARG_TYPE_NONE
};

static const ARG_TYPE
args_cmd_print[] =
{
    ARG_TYPE_S32,   /* copies */
    ARG_TYPE_S32,   /* 0=all/1=range */ /* you might think this could be BOOL but it is constructed from a RADIOBUTTON state */
    ARG_TYPE_S32,   /* range y0 */
    ARG_TYPE_S32,   /* range y1 */
    ARG_TYPE_TSTR,  /* range  */
    ARG_TYPE_S32,   /* 0=both/1=odd/2=even */
    ARG_TYPE_BOOL,  /* 1=reverse*/
    ARG_TYPE_BOOL,  /* 0=sorted/1=collate */
    ARG_TYPE_BOOL,  /* 0=normal/1=two up */
    ARG_TYPE_NONE
};

static const ARG_TYPE
args_cmd_print_quality[] =
{
    ARG_TYPE_BOOL,  /* 0=normal/1=draft */
    ARG_TYPE_NONE
};

static const ARG_TYPE
args_cmd_search[] =
{
    ARG_TYPE_USTR,
    ARG_TYPE_BOOL,
    ARG_TYPE_BOOL,
    ARG_TYPE_BOOL,
    ARG_TYPE_USTR,
    ARG_TYPE_BOOL,
    ARG_TYPE_S32,
    ARG_TYPE_NONE
};

static const ARG_TYPE
args_cmd_sort[] =
{
    ARG_TYPE_S32, ARG_TYPE_BOOL,
    ARG_TYPE_S32, ARG_TYPE_BOOL,
    ARG_TYPE_S32, ARG_TYPE_BOOL,
    ARG_TYPE_S32, ARG_TYPE_BOOL,
    ARG_TYPE_S32, ARG_TYPE_BOOL,
    ARG_TYPE_NONE
};

#if WINDOWS

static const ARG_TYPE
args_cmd_help_search_keyword[] =
{
    ARG_TYPE_TSTR | ARG_MANDATORY_OR_BLANK,
    ARG_TYPE_TSTR | ARG_MANDATORY_OR_BLANK,
    ARG_TYPE_NONE
};

#endif

static const ARG_TYPE
args_cmd_fontmap[] =
{
    ARG_TYPE_TSTR | ARG_MANDATORY,  /* PostScript (Fireworkz) name */
    ARG_TYPE_TSTR | ARG_MANDATORY,  /* RTF class name */
    ARG_TYPE_BOOL | ARG_OPTIONAL,   /* optional RTF class master flag */
    ARG_TYPE_TSTR | ARG_MANDATORY,  /* Host family name */
    ARG_TYPE_TSTR | ARG_OPTIONAL,   /* Host regular full name */
    ARG_TYPE_TSTR | ARG_OPTIONAL,   /* Host italic full name */
    ARG_TYPE_TSTR | ARG_OPTIONAL,   /* Host bold full name */
    ARG_TYPE_TSTR | ARG_OPTIONAL,   /* Host bold italic full name */
    ARG_TYPE_TSTR | ARG_OPTIONAL,   /* optional RISC OS regular full name */
    ARG_TYPE_TSTR | ARG_OPTIONAL,   /* optional RISC OS italic full name */
    ARG_TYPE_TSTR | ARG_OPTIONAL,   /* optional RISC OS bold full name */
    ARG_TYPE_TSTR | ARG_OPTIONAL,   /* optional RISC OS bold italic full name */
    ARG_TYPE_NONE
};

static const ARG_TYPE
args_cmd_fontmap_remap[] =
{
    ARG_TYPE_TSTR | ARG_MANDATORY,  /* newer, corrected PostScript (Fireworkz) name */
    ARG_TYPE_TSTR | ARG_MANDATORY,  /* older, incorrect PostScript (Fireworkz) name */
    ARG_TYPE_NONE
};

static const ARG_TYPE
args_ss_context[] =
{
    ARG_TYPE_USTR | ARG_MANDATORY,
    ARG_TYPE_USTR | ARG_MANDATORY,
    ARG_TYPE_USTR | ARG_MANDATORY,
    ARG_TYPE_USTR | ARG_MANDATORY,
    ARG_TYPE_USTR | ARG_MANDATORY,
    ARG_TYPE_USTR | ARG_MANDATORY,
    ARG_TYPE_USTR | ARG_MANDATORY,
    ARG_TYPE_USTR | ARG_MANDATORY,
    ARG_TYPE_USTR | ARG_MANDATORY,
    ARG_TYPE_USTR | ARG_OPTIONAL,

    ARG_TYPE_NONE
};

static CONSTRUCT_TABLE
object_construct_table[] =
{                                                                                                   /*   fi ti mi ur up xi md mf nn cp sm ba fo */

    { "ChoicesAutoSave",        args_s32,                   T5_CMD_CHOICES_AUTO_SAVE },
    { "ChoicesDisplayPictures", args_bool_mandatory,        T5_CMD_CHOICES_DISPLAY_PICTURES },
    { "ChoicesDithering",       args_bool_mandatory,        T5_CMD_CHOICES_DITHERING },
    { "ChoicesEmbedPictures",   args_bool_mandatory,        T5_CMD_CHOICES_EMBED_INSERTED_FILES },
    { "ChoicesKerning",         args_bool_mandatory,        T5_CMD_CHOICES_KERNING },
    { "ChoicesStatusLine",      args_bool_mandatory,        T5_CMD_CHOICES_STATUS_LINE },
    { "ChoicesToolbar",         args_bool_mandatory,        T5_CMD_CHOICES_TOOLBAR },
    { "ChoicesUpdateStylesFromChoices", args_bool_mandatory, T5_CMD_CHOICES_UPDATE_STYLES_FROM_CHOICES },

    { "ChoicesASCIIDelimited",  args_bool_mandatory,        T5_CMD_CHOICES_ASCII_LOAD_AS_DELIMITED },
    { "ChoicesASCIIDelimiter",  args_s32_mandatory,         T5_CMD_CHOICES_ASCII_LOAD_DELIMITER },
    { "ChoicesASCIIParagraph",  args_bool_mandatory,        T5_CMD_CHOICES_ASCII_LOAD_AS_PARAGRAPHS },

    { "Version",                args_cmd_of_version,        T5_CMD_OF_VERSION },

    { "Flow",                   args_cmd_of_flow,           T5_CMD_OF_FLOW,                             { 1, 0 } },
    { "RowTable",               args_bool_mandatory,        T5_CMD_OF_ROWTABLE,                         { 1, 0 } },
    { "BaseSingleCol",          args_bool_mandatory,        T5_CMD_OF_BASE_SINGLE_COL,                  { 0, 0 } }, /* SKS 05nov95 - just sets state in IP_FORMAT_FLAGS (DOCU flags.base_single_col is set by auto-magic) */
    { "Block",                  args_cmd_of_block,          T5_CMD_OF_BLOCK,                            { 0, 1 } },
    { "R",                      args_cmd_of_region,         T5_CMD_OF_REGION,                           { 0, 1 } },
    { "BR",                     args_cmd_of_region,         T5_CMD_OF_BASE_REGION,                      { 0, 1 } },
    { "Imp",                    args_cmd_of_implied,        T5_CMD_OF_BASE_IMPLIED_REGION,              { 0, 0 } }, /* SKS 05nov95 retained same name so old files work but take on new attribute */
    { "ImpR",                   args_cmd_of_implied,        T5_CMD_OF_IMPLIED_REGION,                   { 0, 0 } }, /* SKS 05nov95 */
    { "S",                      args_cmd_of_cell,           T5_CMD_OF_CELL,                             { 0, 1 } },
    { "Style",                  args_cmd_of_style,          T5_CMD_OF_STYLE },
    { "StyleBase",              args_tstr_mandatory,        T5_CMD_OF_STYLE_BASE,                       { 0, 0 } },
    { "StyleCurrent",           args_tstr_mandatory,        T5_CMD_OF_STYLE_CURRENT,                    { 1, 0 } },
    { "StyleHeaderFooter",      args_tstr_mandatory,        T5_CMD_OF_STYLE_HEFO,                       { 1, 0 } },
    { "StyleText",              args_tstr_mandatory,        T5_CMD_OF_STYLE_TEXT,                       { 1, 0 } },
    { "CmdGroup",               args_ustr_mandorblk,        T5_CMD_OF_GROUP,                            { 0, 0, 0, 0, 0, 0, 0, 0, 1 } },
    { "StartOfData",            NULL,                       T5_CMD_OF_START_OF_DATA,                    { 0, 0 } },
    { "EndOfData",              NULL,                       T5_CMD_OF_END_OF_DATA,                      { 0, 0 } },

    { "U",                      args_ustr_mandatory,        (T5_MESSAGE) IL_UTF8,                       { 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0 } },

                                                                                                    /*   fi                                     reject if file insertion */
                                                                                                    /*      ti                                  reject if template insertion */
                                                                                                    /*         mi                               maybe interactive */
                                                                                                    /*            ur                            unrecordable */
                                                                                                    /*               up                         unrepeatable */
                                                                                                    /*                  xi                      exceptional inline */
                                                                                                    /*                     md                   modify document */
                                                                                                    /*                        mf                memory froth */
                                                                                                    /*                           nn             supress newline on save */
                                                                                                    /*                              cp          check for protection */
                                                                                                    /*                                 sm       send via maeve */
                                                                                                    /*                                    ba    wrap with CUR_CHANGE_BEFORE/CUR_CHANGE_AFTER */
                                                                                                    /*                                       fo send to focus owner */

                                                                                                    /*   fi ti mi ur up xi md mf nn cp sm ba fo */

    { "StyleName",              args_tstr_mandorblk,        (T5_MESSAGE) IL_STYLE_NAME,                 { 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 } },
    { "StyleKey",               args_tstr_mandorblk,        (T5_MESSAGE) IL_STYLE_KEY,                  { 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0 } },
    { "StyleHandle",            args_tstr_mandatory,        (T5_MESSAGE) IL_STYLE_HANDLE,               { 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0 } },
    { "StyleSearch",            args_s32_mandatory,         (T5_MESSAGE) IL_STYLE_SEARCH,               { 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 } },

    { "ColWidth",               args_s32_mandatory,         (T5_MESSAGE) IL_STYLE_CS_WIDTH,             { 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 } },
    { "ColName",                args_ustr_mandorblk,        (T5_MESSAGE) IL_STYLE_CS_COL_NAME,          { 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 } },

    { "RowHeight",              args_s32_mandatory,         (T5_MESSAGE) IL_STYLE_RS_HEIGHT,            { 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 } },
    { "RowHeightFix",           args_u8n_mandatory,         (T5_MESSAGE) IL_STYLE_RS_HEIGHT_FIXED,      { 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 } },
    { "RowUnbreakable",         args_u8n_mandatory,         (T5_MESSAGE) IL_STYLE_RS_UNBREAKABLE,       { 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 } },
    { "RowName",                args_ustr_mandorblk,        (T5_MESSAGE) IL_STYLE_RS_ROW_NAME,          { 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 } },

    { "MarginLeft",             args_s32_mandatory,         (T5_MESSAGE) IL_STYLE_PS_MARGIN_LEFT,       { 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 } },
    { "MarginRight",            args_s32_mandatory,         (T5_MESSAGE) IL_STYLE_PS_MARGIN_RIGHT,      { 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 } },
    { "MarginPara",             args_s32_mandatory,         (T5_MESSAGE) IL_STYLE_PS_MARGIN_PARA,       { 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 } },
    { "Tablist",                args_ustr_mandorblk,        (T5_MESSAGE) IL_STYLE_PS_TAB_LIST,          { 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0 } },
    { "Background",             args_rgb,                   (T5_MESSAGE) IL_STYLE_PS_RGB_BACK,          { 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0 } },
    { "ParaStart",              args_s32_mandatory,         (T5_MESSAGE) IL_STYLE_PS_PARA_START,        { 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 } },
    { "ParaEnd",                args_s32_mandatory,         (T5_MESSAGE) IL_STYLE_PS_PARA_END,          { 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 } },
    { "LineSpace",              args_s32_s32,               (T5_MESSAGE) IL_STYLE_PS_LINE_SPACE,        { 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0 } },
    { "Justify",                args_u8n_mandatory,         (T5_MESSAGE) IL_STYLE_PS_JUSTIFY,           { 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 } },
    { "JustifyV",               args_u8n_mandatory,         (T5_MESSAGE) IL_STYLE_PS_JUSTIFY_V,         { 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 } },
    { "NewObject",              args_u8n_mandatory,         (T5_MESSAGE) IL_STYLE_PS_NEW_OBJECT,        { 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 } },
    { "Numform",                args_ustr_mandorblk,        (T5_MESSAGE) IL_STYLE_PS_NUMFORM_NU,        { 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 } },
    { "NumformDT",              args_ustr_mandorblk,        (T5_MESSAGE) IL_STYLE_PS_NUMFORM_DT,        { 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 } },
    { "NumformSE",              args_ustr_mandorblk,        (T5_MESSAGE) IL_STYLE_PS_NUMFORM_SE,        { 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 } },

    { "BorderColour",           args_rgb,                   (T5_MESSAGE) IL_STYLE_PS_RGB_BORDER,        { 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0 } },
    { "Border",                 args_u8n_mandatory,         (T5_MESSAGE) IL_STYLE_PS_BORDER,            { 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 } },

    { "GridLeftColour",         args_rgb,                   (T5_MESSAGE) IL_STYLE_PS_RGB_GRID_LEFT,     { 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0 } },
    { "GridTopColour",          args_rgb,                   (T5_MESSAGE) IL_STYLE_PS_RGB_GRID_TOP,      { 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0 } },
    { "GridRightColour",        args_rgb,                   (T5_MESSAGE) IL_STYLE_PS_RGB_GRID_RIGHT,    { 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0 } },
    { "GridBottomColour",       args_rgb,                   (T5_MESSAGE) IL_STYLE_PS_RGB_GRID_BOTTOM,   { 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0 } },
    { "GridLeft",               args_u8n_mandatory,         (T5_MESSAGE) IL_STYLE_PS_GRID_LEFT,         { 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 } },
    { "GridTop",                args_u8n_mandatory,         (T5_MESSAGE) IL_STYLE_PS_GRID_TOP,          { 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 } },
    { "GridRight",              args_u8n_mandatory,         (T5_MESSAGE) IL_STYLE_PS_GRID_RIGHT,        { 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 } },
    { "GridBottom",             args_u8n_mandatory,         (T5_MESSAGE) IL_STYLE_PS_GRID_BOTTOM,       { 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 } },

    { "Protect",                args_u8n_mandatory,         (T5_MESSAGE) IL_STYLE_PS_PROTECT,           { 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 } },

    { "Typeface",               args_ustr_mandorblk,        (T5_MESSAGE) IL_STYLE_FS_NAME,              { 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 } },
    { "Fontx",                  args_s32_mandatory,         (T5_MESSAGE) IL_STYLE_FS_SIZE_X,            { 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 } },
    { "Fonty",                  args_s32_mandatory,         (T5_MESSAGE) IL_STYLE_FS_SIZE_Y,            { 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 } },
    { "Underline",              args_u8n_mandatory,         (T5_MESSAGE) IL_STYLE_FS_UNDERLINE,         { 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 } },
    { "Bold",                   args_u8n_mandatory,         (T5_MESSAGE) IL_STYLE_FS_BOLD,              { 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 } },
    { "Italic",                 args_u8n_mandatory,         (T5_MESSAGE) IL_STYLE_FS_ITALIC,            { 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 } },
    { "Superscript",            args_u8n_mandatory,         (T5_MESSAGE) IL_STYLE_FS_SUPERSCRIPT,       { 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 } },
    { "Subscript",              args_u8n_mandatory,         (T5_MESSAGE) IL_STYLE_FS_SUBSCRIPT,         { 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 } },
    { "Foreground",             args_rgb,                   (T5_MESSAGE) IL_STYLE_FS_COLOUR,            { 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0 } },

                                                                                                    /*   fi ti mi ur up xi md mf nn cp sm ba fo */
    { "CTypeTable",             args_ctypetable,            T5_CMD_CTYPETABLE },
    { "UILoad",                 args_uiload,                T5_CMD_NUMFORM_LOAD },
    { "NumformData",            args_ustr_mandorblk,        T5_CMD_NUMFORM_DATA,                        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
    { "SSContext",              args_ss_context,            T5_CMD_SS_CONTEXT,                          { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },

    { "SetInteractive",         NULL,                       T5_CMD_SET_INTERACTIVE },

    { "CursorLeft",             NULL,                       T5_CMD_CURSOR_LEFT,                         { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 } },
    { "CursorRight",            NULL,                       T5_CMD_CURSOR_RIGHT,                        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 } },
    { "CursorDown",             NULL,                       T5_CMD_CURSOR_DOWN,                         { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 } },
    { "CursorUp",               NULL,                       T5_CMD_CURSOR_UP,                           { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 } },
    { "WordLeft",               NULL,                       T5_CMD_WORD_LEFT,                           { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 } },
    { "WordRight",              NULL,                       T5_CMD_WORD_RIGHT,                          { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 } },
    { "PageDown",               NULL,                       T5_CMD_PAGE_DOWN,                           { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 } },
    { "PageUp",                 NULL,                       T5_CMD_PAGE_UP,                             { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 } },
    { "LineStart",              NULL,                       T5_CMD_LINE_START,                          { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 } },
    { "LineEnd",                NULL,                       T5_CMD_LINE_END,                            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 } },
    { "DocumentTop",            NULL,                       T5_CMD_DOCUMENT_TOP,                        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 } },
    { "DocumentBottom",         NULL,                       T5_CMD_DOCUMENT_BOTTOM,                     { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 } },

    { "ShiftCursorLeft",        NULL,                       T5_CMD_SHIFT_CURSOR_LEFT,                   { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 } },
    { "ShiftCursorRight",       NULL,                       T5_CMD_SHIFT_CURSOR_RIGHT,                  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 } },
    { "ShiftCursorDown",        NULL,                       T5_CMD_SHIFT_CURSOR_DOWN,                   { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 } },
    { "ShiftCursorUp",          NULL,                       T5_CMD_SHIFT_CURSOR_UP,                     { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 } },
    { "ShiftWordLeft",          NULL,                       T5_CMD_SHIFT_WORD_LEFT,                     { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 } },
    { "ShiftWordRight",         NULL,                       T5_CMD_SHIFT_WORD_RIGHT,                    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 } },
    { "ShiftPageDown",          NULL,                       T5_CMD_SHIFT_PAGE_DOWN,                     { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 } },
    { "ShiftPageUp",            NULL,                       T5_CMD_SHIFT_PAGE_UP,                       { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 } },
    { "ShiftLineStart",         NULL,                       T5_CMD_SHIFT_LINE_START,                    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 } },
    { "ShiftLineEnd",           NULL,                       T5_CMD_SHIFT_LINE_END,                      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 } },
    { "ShiftDocumentTop",       NULL,                       T5_CMD_SHIFT_DOCUMENT_TOP,                  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 } },
    { "ShiftDocumentBottom",    NULL,                       T5_CMD_SHIFT_DOCUMENT_BOTTOM,               { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 } },

    { "TabLeft",                NULL,                       T5_CMD_TAB_LEFT,                            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 } },
    { "TabRight",               NULL,                       T5_CMD_TAB_RIGHT,                           { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 } },
    { "FirstColumn",            NULL,                       T5_CMD_FIRST_COLUMN },
    { "LastColumn",             NULL,                       T5_CMD_LAST_COLUMN },
    { "DeleteLeft",             NULL,                       T5_CMD_DELETE_CHARACTER_LEFT,               { 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 1 } },
    { "DeleteRight",            NULL,                       T5_CMD_DELETE_CHARACTER_RIGHT,              { 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 1 } },
    { "DeleteLine",             NULL,                       T5_CMD_DELETE_LINE,                         { 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 1 } },
    { "Return",                 NULL,                       T5_CMD_RETURN,                              { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 } },
    { "Escape",                 NULL,                       T5_CMD_ESCAPE,                              { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 } },


                                                                                                    /*   fi ti mi ur up xi md mf nn cp sm ba fo */

    { "ToggleMarks",            NULL,                       T5_CMD_TOGGLE_MARKS,                        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 } },
    { "SelectDocument",         NULL,                       T5_CMD_SELECT_DOCUMENT,                     { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 } },
    { "SelectCell",             NULL,                       T5_CMD_SELECT_CELL,                         { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 } },
    { "SelectWord",             NULL,                       T5_CMD_SELECT_WORD,                         { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 } },

    { "SelectionClear",         NULL,                       T5_CMD_SELECTION_CLEAR,                     { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 } },
    { "SelectionCopy",          NULL,                       T5_CMD_SELECTION_COPY,                      { 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 } },
    { "SelectionCut",           NULL,                       T5_CMD_SELECTION_CUT,                       { 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 1 } },
    { "SelectionDelete",        NULL,                       T5_CMD_SELECTION_DELETE,                    { 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 1 } },
    { "Paste",                  NULL,                       T5_CMD_PASTE_AT_CURSOR,                     { 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 1 } },

    { "BlockClear",             NULL,                       T5_CMD_BLOCK_CLEAR,                         { 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0 } },

    { "DefineKey",              args_cmd_define_key,        T5_CMD_DEFINE_KEY,                          { 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0 } },
    { "DefineKeyRaw",           args_cmd_define_key_raw,    T5_CMD_DEFINE_KEY_RAW,                      { 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0 } },

    { "MenuName",               args_cmd_menu_name,         T5_CMD_MENU_NAME,                           { 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
    { "MenuAdd",                args_cmd_menu_add,          T5_CMD_MENU_ADD,                            { 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0 } },
    { "MenuAddRaw",             args_cmd_menu_add_raw,      T5_CMD_MENU_ADD_RAW,                        { 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0 } },
    { "MenuDelete",             args_cmd_menu_delete,       T5_CMD_MENU_DELETE,                         { 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0 } },

    { "CurrentDocument",        args_ustr_mandatory,        T5_CMD_CURRENT_DOCUMENT,                    { 1, 1 } },
    { "CurrentView",            args_s32_mandatory,         T5_CMD_CURRENT_VIEW,                        { 1, 1 } },
    { "CurrentPane",            args_s32_mandatory,         T5_CMD_CURRENT_PANE,                        { 1, 1 } },
    { "CurrentPosition",        args_cmd_current_posn,      T5_CMD_CURRENT_POSITION,                    { 1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 } },

    { "Execute",                args_ustr_mandorblk,        T5_CMD_EXECUTE },

    { "NewDocumentIntro",       NULL,                       T5_CMD_NEW_DOCUMENT_INTRO,                  { 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0 } },

    { "Load",                   args_cmd_load,              T5_CMD_LOAD,                                { 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0 } },
    { "LoadForeign",            args_tstr_s32_tstr,         T5_CMD_LOAD_FOREIGN ,                       { 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0 } },
    { "LoadTemplate",           args_tstr_mandatory,        T5_CMD_LOAD_TEMPLATE,                       { 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0 } },

    { "Save",                   NULL,                       T5_CMD_SAVE },
    { "SaveAs",                 args_cmd_save_as,           T5_CMD_SAVE_AS },
    { "SaveAsIntro",            args_cmd_save_as_intro,     T5_CMD_SAVE_AS_INTRO },
    { "SaveAsTemplate",         args_cmd_save_as_template,  T5_CMD_SAVE_AS_TEMPLATE },
    { "SaveAsTemplateIntro",    NULL,                       T5_CMD_SAVE_AS_TEMPLATE_INTRO },
    { "SaveAsForeign",          args_cmd_save_as_foreign,   T5_CMD_SAVE_AS_FOREIGN },
    { "SaveAsForeignIntro",     args_cmd_save_as_foreign_intro, T5_CMD_SAVE_AS_FOREIGN_INTRO },
    { "SaveAsDrawfile",         args_cmd_save_as_drawfile,  T5_CMD_SAVE_AS_DRAWFILE },
    { "SaveAsDrawfileIntro",    args_cmd_save_as_drawfile_intro, T5_CMD_SAVE_AS_DRAWFILE_INTRO },
    { "SavePicture",            args_cmd_save_picture,      T5_CMD_SAVE_PICTURE },
    { "SavePictureIntro",       NULL,                       T5_CMD_SAVE_PICTURE_INTRO },
    { "SaveClipboard",          args_ustr,                  T5_CMD_SAVE_CLIPBOARD },

                                                                                                    /*   fi ti mi ur up xi md mf nn cp sm ba fo */

    { "ViewControlIntro",       NULL,                       T5_CMD_VIEW_CONTROL_INTRO,                  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
    { "ViewControl",            args_cmd_view_control,      T5_CMD_VIEW_CONTROL,                        { 1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 } },
    { "ViewScaleIntro",         NULL,                       T5_CMD_VIEW_SCALE_INTRO,                    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
    { "ViewScale",              args_cmd_view_scale,        T5_CMD_VIEW_SCALE,                          { 1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 } },
    { "ViewCreate",             args_s32_s32_s32_s32,       T5_CMD_VIEW_CREATE,                         { 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0 } },
    { "ViewMaximize",           NULL,                       T5_CMD_VIEW_MAXIMIZE,                       { 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
    { "ViewMinimize",           NULL,                       T5_CMD_VIEW_MINIMIZE,                       { 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
    { "ViewNew",                NULL,                       T5_CMD_VIEW_NEW,                            { 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
    { "ViewPosition",           args_s32_s32,               T5_CMD_VIEW_POSN,                           { 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
    { "ViewScroll",             args_cmd_view_scroll,       T5_CMD_VIEW_SCROLL,                         { 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
    { "ViewSize",               args_s32_s32,               T5_CMD_VIEW_SIZE,                           { 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
    { "ViewCloseReq",           NULL,                       T5_CMD_VIEW_CLOSE_REQ,                      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },

    { "RulerScale",             args_s32_s32_s32_s32,       T5_CMD_RULER_SCALE,                         { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
    { "VRulerScale",            args_s32_s32_s32_s32,       T5_CMD_RULER_SCALE_V,                       { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },

    { "NoteSingle",             args_cmd_note,              T5_CMD_NOTE,                                { 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0 } },
    { "NoteTwin",               args_cmd_note,              T5_CMD_NOTE_TWIN,                           { 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0 } }, /* compatibility with older readers */
    { "NoteBackdrop",           args_cmd_note,              T5_CMD_NOTE_BACKDROP,                       { 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0 } }, /* exists so that backdrops can be rejected on file insert */
    { "NoteBack",               NULL,                       T5_CMD_NOTE_BACK,                           { 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0 } },
    { "NoteSwap",               NULL,                       T5_CMD_NOTE_SWAP,                           { 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0 } },
    { "NoteEmbed",              NULL,                       T5_CMD_NOTE_EMBED,                          { 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0 } },
    { "Backdrop",               args_cmd_backdrop,          T5_CMD_BACKDROP,                            { 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0 } },
    { "BackdropIntro",          NULL,                       T5_CMD_BACKDROP_INTRO,                      { 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0 } },

                                                                                                    /*   fi ti mi ur up xi md mf nn cp sm ba fo */

    { "PlainTextPrint",         NULL,                       T5_CMD_PLAIN_TEXT_TEMP,                     { 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0 } },
    { "PrintIntro",             NULL,                       T5_CMD_PRINT_INTRO,                         { 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0 } },
    { "PrintExtraIntro",        NULL,                       T5_CMD_PRINT_EXTRA_INTRO,                   { 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0 } },
    { "Print",                  args_cmd_print,             T5_CMD_PRINT,                               { 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0 } },
    { "PrintExtra",             args_cmd_print,             T5_CMD_PRINT_EXTRA,                         { 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0 } },
    { "PrintQuality",           args_cmd_print_quality,     T5_CMD_PRINT_QUALITY,                       { 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0 } },
    { "PrintSetup",             NULL,                       T5_CMD_PRINT_SETUP },

    { "StyleIntro",             NULL,                       T5_CMD_STYLE_BUTTON },
    { "StyleIntroForConfig",    NULL,                       T5_CMD_STYLE_FOR_CONFIG },
    { "StyleApply",             args_ustr,                  T5_CMD_STYLE_APPLY,                         { 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1 } },
    { "StyleApplySource",       args_ustr,                  T5_CMD_STYLE_APPLY_SOURCE,                  { 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1 } },
    { "Effects",                args_s32,                   T5_CMD_EFFECTS_BUTTON },

    { "StyleRegionClear",       NULL,                       T5_CMD_STYLE_REGION_CLEAR,                  { 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0 } },
    { "StyleRegionCount",       NULL,                       T5_CMD_STYLE_REGION_COUNT,                  { 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0 } },
    { "StyleRegionEdit",        NULL,                       T5_CMD_STYLE_REGION_EDIT,                   { 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0 } },

    { "CaseUpper",              NULL,                       T5_CMD_SETC_UPPER,                          { 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1 } },
    { "CaseLower",              NULL,                       T5_CMD_SETC_LOWER,                          { 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1 } },
    { "CaseIniCap",             NULL,                       T5_CMD_SETC_INICAP,                         { 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1 } },
    { "CaseSwap",               NULL,                       T5_CMD_SETC_SWAP,                           { 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1 } },

    { "InsertFieldIntroDate",   NULL,                       T5_CMD_INSERT_FIELD_INTRO_DATE,             { 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0 } },
    { "InsertFieldIntroTime",   NULL,                       T5_CMD_INSERT_FIELD_INTRO_TIME,             { 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0 } },
    { "InsertFieldIntroPage",   NULL,                       T5_CMD_INSERT_FIELD_INTRO_PAGE,             { 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0 } },

    { "InsertFieldDate",        args_ustr,                  T5_CMD_INSERT_FIELD_DATE,                   { 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 1 } },
    { "InsertFieldFileDate",    args_ustr,                  T5_CMD_INSERT_FIELD_FILE_DATE,              { 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 1 } },
    { "InsertFieldPageX",       args_ustr,                  T5_CMD_INSERT_FIELD_PAGE_X,                 { 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 1 } },
    { "InsertFieldPageY",       args_ustr,                  T5_CMD_INSERT_FIELD_PAGE_Y,                 { 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 1 } },
    { "InsertFieldName",        args_s32,                   T5_CMD_INSERT_FIELD_SS_NAME,                { 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 1 } },
    { "InsertFieldMailshotField", args_s32_mandatory,       T5_CMD_INSERT_FIELD_MS_FIELD,               { 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 1 } },

    { "InsertFieldWholename",   NULL,                       T5_CMD_INSERT_FIELD_WHOLENAME,              { 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 1 } },
    { "InsertFieldLeafname",    NULL,                       T5_CMD_INSERT_FIELD_LEAFNAME,               { 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 1 } },
    { "InsertFieldReturn",      NULL,                       T5_CMD_INSERT_FIELD_RETURN,                 { 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 1 } },
    { "InsertFieldSoftHyphen",  NULL,                       T5_CMD_INSERT_FIELD_SOFT_HYPHEN,            { 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 1 } },

                                                                                                    /*   fi ti mi ur up xi md mf nn cp sm ba fo */

    { "AddColumns",             args_s32,                   T5_CMD_COLS_ADD_AFTER,                      { 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 1 } },
    { "AddColumnsIntro",        NULL,                       T5_CMD_COLS_ADD_AFTER_INTRO,                { 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0 } },
    { "InsertColumns",          args_s32,                   T5_CMD_COLS_INSERT_BEFORE,                  { 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 1 } },
    { "InsertColumnsIntro",     NULL,                       T5_CMD_COLS_INSERT_BEFORE_INTRO,            { 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0 } },
    { "DeleteColumn",           NULL,                       T5_CMD_COL_DELETE,                          { 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 1 } },

    { "AddRows",                args_s32,                   T5_CMD_ROWS_ADD_AFTER,                      { 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 1 } },
    { "AddRowsIntro",           NULL,                       T5_CMD_ROWS_ADD_AFTER_INTRO,                { 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0 } },
    { "InsertRows",             args_s32,                   T5_CMD_ROWS_INSERT_BEFORE,                  { 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 1 } },
    { "InsertRowsIntro",        NULL,                       T5_CMD_ROWS_INSERT_BEFORE_INTRO,            { 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1 } },
    { "DeleteRow",              NULL,                       T5_CMD_ROW_DELETE,                          { 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 1 } },

    { "InsertTable",            args_s32_s32,               T5_CMD_INSERT_TABLE,                        { 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 1 } },
    { "InsertTableIntro",       NULL,                       T5_CMD_INSERT_TABLE_INTRO,                  { 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0 } },

    { "GoTo",                   args_ustr,                  T5_CMD_GOTO,                                { 0, 0, 0, 0, 0, 0, 0, 1 } },
    { "GoToIntro",              NULL,                       T5_CMD_GOTO_INTRO,                          { 0, 0, 0, 0, 0, 0, 0, 1 } },

    { "InsertPageBreak",        NULL,                       T5_CMD_INSERT_PAGE_BREAK,                   { 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0 } },
    { "WordCount",              NULL,                       T5_CMD_WORD_COUNT,                          { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 } },
    { "BoxIntro",               NULL,                       T5_CMD_BOX_INTRO,                           { 0, 0, 0, 0, 0, 0, 0, 1 } },
    { "Box",                    args_cmd_box,               T5_CMD_BOX,                                 { 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1 } },
    { "AutoWidth",              NULL,                       T5_CMD_AUTO_WIDTH,                          { 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0 } },
    { "AutoHeight",             NULL,                       T5_CMD_AUTO_HEIGHT,                         { 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0 } },
    { "SearchButton",           NULL,                       T5_CMD_SEARCH_BUTTON },
    { "SearchButtonAlternate",  NULL,                       T5_CMD_SEARCH_BUTTON_ALTERNATE },
    { "Search",                 args_cmd_search,            T5_CMD_SEARCH,                              { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 } },
    { "SearchIntro",            args_cmd_search,            T5_CMD_SEARCH_INTRO },
    { "Choices",                NULL,                       T5_CMD_CHOICES },
    { "Info",                   NULL,                       T5_CMD_INFO },
    { "Quit",                   NULL,                       T5_CMD_QUIT },
    { "Thesaurus",              NULL,                       T5_CMD_THESAURUS },

                                                                                                    /*   fi ti mi ur up xi md mf nn cp sm ba fo */

    { "AutoSave",               args_s32,                   T5_CMD_AUTO_SAVE },
    { "SortIntro",              NULL,                       T5_CMD_SORT_INTRO,                          { 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1 } },
    { "Sort",                   args_cmd_sort,              T5_CMD_SORT,                                { 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 1 } },
    { "Snapshot",               NULL,                       T5_CMD_SNAPSHOT,                            { 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 1 } },
    { "ObjectConvert",          args_s32_mandatory,         T5_CMD_OBJECT_CONVERT,                      { 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0 } },

    { "StraddleHorz",           NULL,                       T5_CMD_STRADDLE_HORZ,                       { 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0 } },

    { "Paper",                  args_cmd_paper,             T5_CMD_PAPER,                               { 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0 } },
    { "PaperIntro",             NULL,                       T5_CMD_PAPER_INTRO,                         { 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0 } },
    { "PaperScale",             args_cmd_paper_scale,       T5_CMD_PAPER_SCALE,                         { 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0 } },
    { "PaperScaleIntro",        NULL,                       T5_CMD_PAPER_SCALE_INTRO,                   { 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0 } },

    { "BindFileType",           args_bind_file_type,        T5_CMD_BIND_FILE_TYPE,                      { 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0 } },
    { "ObjectBindLoader",       args_object_bind_load,      T5_CMD_OBJECT_BIND_LOADER,                  { 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0 } },
    { "ObjectBindSaver",        args_object_bind_save,      T5_CMD_OBJECT_BIND_SAVER,                   { 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0 } },
    { "ObjectBindConstruct",    args_object_bind_construct, T5_CMD_OBJECT_BIND_CONSTRUCT,               { 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0 } },
    { "ObjectEnsure",           args_s32_mandatory,         T5_CMD_OBJECT_ENSURE,                       { 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0 } },

    { "Select",                 args_cmd_docu_area,         T5_CMD_SELECTION_MAKE,                      { 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0 } },
    { "ForceRecalc",            args_ustr,                  T5_CMD_FORCE_RECALC,                        { 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1 } },

    { "Button",                 args_cmd_button,            T5_CMD_BUTTON },

    { "Fontmap",                args_cmd_fontmap,           T5_CMD_FONTMAP },
    { "FontmapEnd",             NULL,                       T5_CMD_FONTMAP_END },
    { "FontmapRemap",           args_cmd_fontmap_remap,     T5_CMD_FONTMAP_REMAP },

    { "HelpContents",           args_tstr_mandorblk,        T5_CMD_HELP_CONTENTS },
#if WINDOWS
    { "HelpSearchKeyword",      args_cmd_help_search_keyword, T5_CMD_HELP_SEARCH_KEYWORD },
#endif
    { "HelpURL",                args_tstr_mandatory,        T5_CMD_HELP_URL },

    { "#",                      NULL,                       T5_CMD_COMMENT }, /* explicit comments are allowed in files e.g. config.txt */

    { "RulerTypeface",          NULL,                       T5_CMD_REMOVED },
    { "BorderTypeface",         NULL,                       T5_CMD_REMOVED },

    /*
    end of table
    */

    { "Trace",                  args_s32_mandatory,         T5_CMD_TRACE },
    { "NotYetImplemented",      NULL,                       T5_CMD_NYI },
    { "Test",                   NULL,                       T5_CMD_TEST },

    { NULL,                     NULL,                       T5_EVENT_NONE }
};

_Check_return_
extern STATUS
skeleton_table_startup(void)
{
    return(register_object_construct_table(OBJECT_ID_SKEL, object_construct_table, TRUE));
}

/* end of sk_table.c */
