/* ob_skel/flags.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* skeleton includes */

#ifndef __skel_flags_h
#define __skel_flags_h

#ifndef         __xp_skel_h
#include "ob_skel/xp_skel.h"
#endif

#ifndef         __xp_note_h
#include "ob_note/xp_note.h"
#endif

#ifndef    __cells_ob_cells_h
#include "ob_cells/ob_cells.h"
#endif

#ifndef          __ce_edit_h
#include "ob_cells/ce_edit.h"
#endif

/*
ff_load.c
*/

T5_CMD_PROTO(extern, t5_cmd_load_foreign);
T5_CMD_PROTO(extern, t5_cmd_bind_file_type);
T5_CMD_PROTO(extern, t5_cmd_object_bind_loader);

/*
fonty.c
*/

T5_CMD_PROTO(extern, t5_cmd_fontmap);
T5_CMD_PROTO(extern, t5_cmd_fontmap_end);
T5_CMD_PROTO(extern, t5_cmd_fontmap_remap);

/*
of_load.c
*/

T5_CMD_PROTO(extern, t5_cmd_load);
T5_CMD_PROTO(extern, t5_cmd_load_template);
T5_CMD_PROTO(extern, t5_cmd_object_bind_construct);

/*
sk_bord.c
*/

PROC_EVENT_PROTO(extern, edge_window_event_border_horz);
PROC_EVENT_PROTO(extern, edge_window_event_border_vert);

PROC_EVENT_PROTO(extern, proc_event_margin_col);
PROC_EVENT_PROTO(extern, proc_event_margin_row);

/*
sk_choic.c
*/

T5_CMD_PROTO(extern, t5_cmd_choices_auto_save);
T5_CMD_PROTO(extern, t5_cmd_choices_display_pictures);
T5_CMD_PROTO(extern, t5_cmd_choices_embed_inserted_files);
T5_CMD_PROTO(extern, t5_cmd_choices_kerning);
T5_CMD_PROTO(extern, t5_cmd_choices_dithering);
T5_CMD_PROTO(extern, t5_cmd_choices_status_line);
T5_CMD_PROTO(extern, t5_cmd_choices_toolbar);
T5_CMD_PROTO(extern, t5_cmd_choices_update_styles_from_choices);

T5_CMD_PROTO(extern, t5_cmd_choices_ascii_load_as_delimited);
T5_CMD_PROTO(extern, t5_cmd_choices_ascii_load_delimiter);
T5_CMD_PROTO(extern, t5_cmd_choices_ascii_load_as_paragraphs);

T5_CMD_PROTO(extern, t5_cmd_choices);

/*
sk_cmd.c
*/

T5_CMD_PROTO(extern, t5_cmd_define_key);

/*
sk_draft.c
*/

T5_MSG_PROTO(extern, t5_msg_draft_print, P_PRINT_CTRL p_print_ctrl);
T5_MSG_PROTO(extern, t5_msg_draft_print_to_file, P_DRAFT_PRINT_TO_FILE p_draft_print_to_file);

/*
sk_hefod.c
*/

T5_CMD_PROTO(extern, t5_cmd_insert_page_break);

/*
sk_print.c
*/

T5_CMD_PROTO(extern, t5_cmd_paper);
T5_CMD_PROTO(extern, t5_cmd_paper_scale);
T5_CMD_PROTO(extern, t5_cmd_print);
T5_CMD_PROTO(extern, t5_cmd_print_intro);
T5_CMD_PROTO(extern, t5_cmd_print_quality);

/*
sk_save.c
*/

T5_CMD_PROTO(extern, t5_cmd_save_clipboard);

_Check_return_
extern STATUS
skeleton_save_construct(
    _DocuRef_   P_DOCU p_docu,
    P_SAVE_CONSTRUCT_OWNFORM p_save_construct);

_Check_return_
extern STATUS
skeleton_save_style_handle(
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format,
    _InVal_     STYLE_HANDLE style_handle,
    _InVal_     BOOL part_save);

_Check_return_
extern STATUS
skel_save_version(
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format);

/*
sk_table.c
*/

/* KEEP THESE CORRESPONDING WITH ARG TABLES IN sk_table.c */

/*
offsets in args_region[]
*/

enum ARG_DOCU_AREA
{
    ARG_DOCU_AREA_TL_COL = 0,
    ARG_DOCU_AREA_TL_ROW,
    ARG_DOCU_AREA_TL_DATA,
    ARG_DOCU_AREA_TL_OBJECT_ID,

    ARG_DOCU_AREA_BR_COL,
    ARG_DOCU_AREA_BR_ROW,
    ARG_DOCU_AREA_BR_DATA,
    ARG_DOCU_AREA_BR_OBJECT_ID,

    ARG_DOCU_AREA_WHOLE_COL,
    ARG_DOCU_AREA_WHOLE_ROW,

    ARG_DOCU_AREA_DEFN,

    ARG_DOCU_AREA_N_ARGS
};

enum ARG_IMPLIED
{
    ARG_IMPLIED_TL_COL = 0,         /* numbers here must correspond with ARG_DOCU_AREA */
    ARG_IMPLIED_TL_ROW,
    ARG_IMPLIED_TL_DATA,
    ARG_IMPLIED_TL_OBJECT_ID,

    ARG_IMPLIED_BR_COL,
    ARG_IMPLIED_BR_ROW,
    ARG_IMPLIED_BR_DATA,
    ARG_IMPLIED_BR_OBJECT_ID,

    ARG_IMPLIED_WHOLE_COL,
    ARG_IMPLIED_WHOLE_ROW,

    ARG_IMPLIED_OBJECT_ID,
    ARG_IMPLIED_MESSAGE,
    ARG_IMPLIED_ARG,
    ARG_IMPLIED_REGION_CLASS,

    ARG_IMPLIED_STYLE,

    ARG_IMPLIED_N_ARGS
};

/*
ui_load.c
*/

T5_CMD_PROTO(extern, t5_cmd_new_document_default);
T5_CMD_PROTO(extern, t5_cmd_new_document_intro);

/*
ui_save.c
*/

T5_CMD_PROTO(extern, t5_cmd_object_bind_saver);
T5_CMD_PROTO(extern, t5_cmd_save);
T5_CMD_PROTO(extern, t5_cmd_save_as);
T5_CMD_PROTO(extern, t5_cmd_save_as_intro);
T5_CMD_PROTO(extern, t5_cmd_save_as_template);
T5_CMD_PROTO(extern, t5_cmd_save_as_template_intro);
T5_CMD_PROTO(extern, t5_cmd_save_as_foreign);
T5_CMD_PROTO(extern, t5_cmd_save_as_foreign_intro);
T5_CMD_PROTO(extern, t5_cmd_save_as_drawfile);
T5_CMD_PROTO(extern, t5_cmd_save_as_drawfile_intro);
T5_CMD_PROTO(extern, t5_cmd_save_picture);
T5_CMD_PROTO(extern, t5_cmd_save_picture_intro);

/*
view.c
*/

PROC_EVENT_PROTO(extern, view_event_back_window);
PROC_EVENT_PROTO(extern, view_event_border_horz);
PROC_EVENT_PROTO(extern, view_event_border_vert);
PROC_EVENT_PROTO(extern, view_event_pane_window);
PROC_EVENT_PROTO(extern, view_event_ruler_horz);
PROC_EVENT_PROTO(extern, view_event_ruler_vert);

T5_CMD_PROTO(extern, t5_cmd_view_control_intro);
T5_CMD_PROTO(extern, t5_cmd_view_control);
T5_CMD_PROTO(extern, t5_cmd_view_scale_intro);
T5_CMD_PROTO(extern, t5_cmd_view_scale);
T5_CMD_PROTO(extern, t5_cmd_view_create);
T5_CMD_PROTO(extern, t5_cmd_view_new);
T5_CMD_PROTO(extern, t5_cmd_view_size);
T5_CMD_PROTO(extern, t5_cmd_view_posn);
T5_CMD_PROTO(extern, t5_cmd_view_maximize);
T5_CMD_PROTO(extern, t5_cmd_view_minimize);
T5_CMD_PROTO(extern, t5_cmd_view_scroll);
T5_CMD_PROTO(extern, t5_cmd_view_close_req);

T5_CMD_PROTO(extern, t5_cmd_current_pane);
T5_CMD_PROTO(extern, t5_cmd_current_view);

T5_CMD_PROTO(extern, t5_cmd_ruler_scale);

/*
sk_menu.c/ho_menu.c
*/

T5_CMD_PROTO(extern, t5_cmd_menu_add);
T5_CMD_PROTO(extern, t5_cmd_menu_delete);
T5_CMD_PROTO(extern, t5_cmd_menu_name);

#if RISCOS

#ifndef menu__str_defined
#define menu__str_defined
typedef struct menu__str * abstract_menu_handle; /* abstract menu handle */
#endif

_Check_return_
extern abstract_menu_handle
ho_menu_event_maker(
    P_ANY handle);

_Check_return_
extern BOOL
ho_menu_event_proc(
    P_ANY handle,
    P_U8 hit,
    _InVal_     BOOL submenurequest);

#elif WINDOWS

_Check_return_
extern STATUS
ho_menu_msg_close2(
    _DocuRef_   P_DOCU p_docu);

_Check_return_
extern STATUS
ho_create_docu_menus(
    _DocuRef_   P_DOCU p_docu);

extern HMENU
ho_get_menu_bar(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view);

extern HMENU
ho_get_menu_bar_for_icon(void);

/*ncr*/
extern BOOL
ho_execute_menu_command(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _In_        UINT command,
    _InVal_     BOOL slide_off);

extern void
host_onInitMenu(
    _InRef_     HMENU hmenu,
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view);

#endif

_Check_return_
extern STATUS
ho_menu_popup(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InVal_     S32 menu_root);

extern void
ho_menu_dispose(void);

/*
internally exported types for sole use of ho_menu.c
*/

typedef struct MENU_ROOT
{
    ARRAY_HANDLE h_entry_list;  /* list of MENU_ENTRYs (defined local to sk_menu.c) */
    TCHARZ name[12];
}
MENU_ROOT, * P_MENU_ROOT, ** P_P_MENU_ROOT;

#define P_MENU_ROOT_NONE _P_DATA_NONE(P_MENU_ROOT)

enum MENU_ACTIVE_STATE
{
    MENU_ACTIVE_ALWAYS                = 0,    /* always active */
    MENU_ACTIVE_NEVER                 = 1,    /* never active (greyed out) */
    MENU_SELECTION                    = 2,    /* grey if no selection */

    MENU_PASTE                        = 3,    /* grey if paste buffer empty */

    MENU_CARET_IN_CELLS               = 4,
    MENU_CARET_IN_TEXT                = 5,

    MENU_CELLS_SELECTION              = 6,

    MENU_PICTURES                     = 10,
    MENU_PICTURE_SELECTION            = 11,   /* grey if no selected pictures */
    MENU_PICTURE_OR_BACKDROP          = 12,   /* grey if no (selected pictures or backdrop) */

    MENU_spare_thing = 13,

    MENU_DATABASE                     = 14,   /* grey if not in database */

    MENU_CARET_IN_CELLS_NOT_DATABASE  = 15,
    MENU_CARET_IN_TEXT_NOT_DATABASE   = 16,

    MENU_DATABASE_TEMPLATE            = 17,   /* grey if not in empty database. i.e. Database->Create */

    MENU_ACTIVE_STATE_COUNT
};

typedef struct MENU_ENTRY_FLAGS
{
    UBF active_state : 8;       /* fits an enum MENU_ACTIVE_STATE */
}
MENU_ENTRY_FLAGS;

typedef struct MENU_ENTRY
{
    PTSTR tstr_entry_text;
    ARRAY_HANDLE h_command;     /* macro script to execute if entry is selected - usually NULL if not a leaf entry */
    ARRAY_HANDLE h_command2;    /* macro script to execute if entry is slid off */
    KMAP_CODE key_code;         /* 0 if no key binding (or if binding attempt failed) */
    MENU_ROOT sub_menu;
    MENU_ENTRY_FLAGS flags;
}
MENU_ENTRY, * P_MENU_ENTRY, ** P_P_MENU_ENTRY;

/*
data stored on list for key definition
*/

typedef struct LISTED_KEY_DEF
{
    ARRAY_HANDLE definition_handle;
}
LISTED_KEY_DEF, * P_LISTED_KEY_DEF;

/*
Exported routines
*/

_Check_return_
extern STATUS
ho_create_icon_menu(void);

_Check_return_
_Ret_notnull_
extern P_MENU_ROOT
sk_menu_root(
    _DocuRef_   PC_DOCU p_docu,
    _InVal_     MENU_ROOT_ID menu_root_id);

/*PROC_EVENT_PROTO(extern, proc_event_sk_cmd_direct);*/

_Check_return_
extern STATUS
sk_cmd_direct_msg_initclose(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_MSG_INITCLOSE p_msg_initclose);

/*
ui_field.c
*/

T5_CMD_PROTO(extern, t5_cmd_insert_field_intro_date);
T5_CMD_PROTO(extern, t5_cmd_insert_field_intro_page);

/*
ui_page.c
*/

#define ARG_PAPER_NAME              0
#define ARG_PAPER_ORIENTATION       1
#define ARG_PAPER_HEIGHT            2
#define ARG_PAPER_WIDTH             3
#define ARG_PAPER_MARGIN_TOP        4
#define ARG_PAPER_MARGIN_BOTTOM     5
#define ARG_PAPER_MARGIN_LEFT       6
#define ARG_PAPER_MARGIN_RIGHT      7
#define ARG_PAPER_MARGIN_BIND       8
#define ARG_PAPER_MARGIN_OE_SWAP    9
#define ARG_PAPER_MARGIN_COL        10
#define ARG_PAPER_MARGIN_ROW        11
#define ARG_PAPER_GRID_SIZE         12
#define ARG_PAPER_LOADED_ID         13 /* NB. only present in config file (so far) */
#define ARG_PAPER_GRID_FAINT        14
#define ARG_PAPER_N_ARGS            15

T5_CMD_PROTO(extern, t5_cmd_paper_intro);
T5_CMD_PROTO(extern, t5_cmd_paper_scale_intro);

/*
vi_edge.c
*/

extern void
view_edge_windows_cache_info(
    _ViewRef_   P_VIEW p_view);

_Check_return_
extern STATUS
vi_edge_msg_post_save_1(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format);

extern void
startup_t5_personal(void);

extern void
host_initialise_file_paths(void);

#if WINDOWS

extern LRESULT CALLBACK
host_top_level_window_event_handler(
    _HwndRef_   HWND hwnd,
    _InVal_     UINT uiMsg,
    _InVal_     WPARAM wParam,
    _InVal_     LPARAM lParam);

/*
ho_dde.c
*/

extern BOOL g_started_for_dde;

extern void
ho_dde_msg_exit2(void);

extern void
host_dde_startup(void);

/*
ho_dial.c
*/

extern void
ho_dial_msg_exit2(void);

#endif /* WINDOWS */

/*
ho_help.c
*/

T5_CMD_PROTO(extern, t5_cmd_help);

/*
ho_print.c
*/

T5_CMD_PROTO(extern, t5_cmd_print_setup);

#endif /* __skel_flags_h */

/* end of flags.h */
