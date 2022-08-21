/* ui_dlgin.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* dialog manager internal header */

/* SKS May 1992 */

#include "ob_dlg/xp_dlg.h"

#include "ob_dlg/resource/resource.h"

#if RISCOS
/*#define EDIT_XX_SINGLE_LINE_WIMP 1*/

#include "ob_dlg/xp_dlgr.h"

#include "ob_mlec/xp_mlec.h"

#include "cmodules/mlec.h"

#include "ob_dlg/ri_lbox.h"
#endif

#if WINDOWS
__pragma(warning(push))
__pragma(warning(disable:4255)) /* no function prototype given: converting '()' to '(void) (Windows SDK 6.0A) */
#include "uxtheme.h"
__pragma(warning(pop))
#endif

#ifndef         __xp_skeld_h
#include "ob_skel/xp_skeld.h"
#endif

#include "ob_file/xp_file.h"

typedef H_DIALOG * P_H_DIALOG;

/*
internal structure kept per control per dialog
*/

typedef struct DIALOG_ICTL_GROUP
{
    ARRAY_HANDLE handle; /* group of nested DIALOG_ICTLs */
    DIALOG_CONTROL_ID min_dialog_control_id; /* incl */
    DIALOG_CONTROL_ID max_dialog_control_id; /* incl */
}
DIALOG_ICTL_GROUP, * P_DIALOG_ICTL_GROUP, ** P_P_DIALOG_ICTL_GROUP; typedef const DIALOG_ICTL_GROUP * PC_DIALOG_ICTL_GROUP;

typedef struct DIALOG_ICTL_EDIT_XX
{
    UBF readonly            : 1; /* bits copied from DIALOG_CONTROL_DATA_EDIT_XX_BITS */
    UBF multiline           : 1;
    UBF h_scroll            : 1;
    UBF v_scroll            : 1;
    UBF always_update_later : 1;

    UBF reserved            : 8*sizeof(int) - (5*1 + DIALOG_BORDER_STYLE_BITS);

    UBF border_style        : DIALOG_BORDER_STYLE_BITS;

    DIALOG_CONTROL_ID dialog_control_id; /* backref out to control */

    H_DIALOG h_dialog; /* backref out to parent dialog */
    const BITMAP_WORD * p_bitmap_validation;

#if RISCOS
    struct DIALOG_ICTL_EDIT_XX_RISCOS
    {
#ifdef EDIT_XX_SINGLE_LINE_WIMP
        P_U8 slec_buffer;
        U32 slec_buffer_size;
#endif
        MLEC mlec;
        GDI_POINT scroll; /* need to store scrolls here for redraws and reopens */
    } riscos;
#elif WINDOWS
    struct DIALOG_ICTL_EDIT_XX_WINDOWS
    {
        SIZE gdi_size; /* in pixels, for WM_MEASUREITEM */
    } windows;
#endif
}
DIALOG_ICTL_EDIT_XX, * P_DIALOG_ICTL_EDIT_XX, ** P_P_DIALOG_ICTL_EDIT_XX;

typedef struct DIALOG_ICTL_LIST_XX
{
    PC_UI_SOURCE p_ui_source;

    PC_UI_CONTROL_S32 p_ui_control_s32; /* iff _S32 type */

#if RISCOS
    struct DIALOG_ICTL_DATA_LIST_XX_RISCOS
    {
        RI_LBOX_HANDLE lbox;
    } riscos;
#endif
}
DIALOG_ICTL_LIST_XX;

#if RISCOS
typedef struct DIALOG_WIMP_I
{
    wimp_i icon_handle;
    BOOL redrawn_by_wimp;
}
DIALOG_WIMP_I, * P_DIALOG_WIMP_I; typedef const DIALOG_WIMP_I * PC_DIALOG_WIMP_I;
#endif

struct DIALOG_ICTL_DATA_GROUPBOX
{
    DIALOG_ICTL_GROUP ictls;
    CLIENT_HANDLE client_handle;
    P_PROC_DIALOG_EVENT p_proc_client;
};

union DIALOG_ICTL_DATA
{
    struct DIALOG_ICTL_DATA_GROUPBOX groupbox;

    struct DIALOG_ICTL_DATA_STATICTEXT
    {
        U32 foo;
    } statictext, staticframe;

    struct DIALOG_ICTL_DATA_STATICPICTURE
    {
#if RISCOS
        struct DIALOG_ICTL_DATA_STATICPICTURE_RISCOS
        {
            RESOURCE_BITMAP_HANDLE h_bitmap; /* copy owned by dialog */
        } riscos;
#else
        U32 foo;
#endif
    } staticpicture;

    struct DIALOG_ICTL_DATA_PUSHBUTTON
    {
        U32 foo;
    } pushbutton;

    struct DIALOG_ICTL_DATA_PUSHPICTURE
    {
#if RISCOS
        struct DIALOG_ICTL_DATA_PUSHPICTURE_RISCOS
        {
            RESOURCE_BITMAP_HANDLE h_bitmap; /* copy owned by dialog */
        } riscos;
#else
        U32 foo;
#endif
    } pushpicture;

    struct DIALOG_ICTL_DATA_RADIOBUTTON
    {
        U32 foo;
    } radiobutton;

    struct DIALOG_ICTL_DATA_RADIOPICTURE
    {
#if RISCOS
        struct DIALOG_ICTL_DATA_RADIOPICTURE_RISCOS
        {
            RESOURCE_BITMAP_HANDLE h_bitmap_on, h_bitmap_off;
        } riscos;
#else
        U32 foo;
#endif
    } radiopicture;

    struct DIALOG_ICTL_DATA_CHECKBOX
    {
        U32 foo;
    } checkbox;

    struct DIALOG_ICTL_DATA_CHECKPICTURE
    {
#if RISCOS
        struct DIALOG_ICTL_DATA_CHECKPICTURE_RISCOS
        {
            RESOURCE_BITMAP_HANDLE h_bitmap_on, h_bitmap_off;
        } riscos;
#else
        U32 foo;
#endif
    } checkpicture;

    struct DIALOG_ICTL_DATA_TRISTATE
    {
        U32 foo;
    } tristate;

    struct DIALOG_ICTL_DATA_TRIPICTURE
    {
#if RISCOS
        struct DIALOG_ICTL_DATA_TRIPICTURE_RISCOS
        {
            RESOURCE_BITMAP_HANDLE h_bitmap_on, h_bitmap_off, h_bitmap_dont_care;
        } riscos;
#else
        U32 foo;
#endif
    } tripicture;

    struct DIALOG_ICTL_DATA_EDIT
    {
        DIALOG_ICTL_EDIT_XX edit_xx;
    } edit;

    struct DIALOG_ICTL_DATA_BUMP_XX
    {
        DIALOG_ICTL_EDIT_XX edit_xx;
#if RISCOS
        struct DIALOG_ICTL_DATA_BUMP_XX_RISCOS
        {
            RESOURCE_BITMAP_HANDLE h_bitmap_inc, h_bitmap_dec;
        } riscos;
#elif WINDOWS
        struct DIALOG_ICTL_DATA_BUMP_XX_WINDOWS
        {
            UINT spin_wid;
        } windows;
#endif /* OS */
    } bump_xx;

    struct DIALOG_ICTL_DATA_LIST_XX
    {
        DIALOG_ICTL_LIST_XX list_xx;
    } list_xx;

    struct DIALOG_ICTL_DATA_COMBO_XX
    {
        DIALOG_ICTL_EDIT_XX edit_xx;
        DIALOG_ICTL_LIST_XX list_xx;
    } combo_xx;

    struct DIALOG_ICTL_DATA_USER
    {
        UBF border_style : DIALOG_BORDER_STYLE_BITS;

        UBF spare : 8 * sizeof(int) - DIALOG_BORDER_STYLE_BITS;
    } user;
};

typedef struct DIALOG_ICTL
{
    PC_DIALOG_CONTROL p_dialog_control;
    PC_DIALOG_CONTROL_DATA p_dialog_control_data;

    UI_TEXT caption;

    DIALOG_CONTROL_ID dialog_control_id; /* cache here */

    DIALOG_CONTROL_TYPE dialog_control_type; /* cache here */

    struct DIALOG_ICTL_BITS
    {
        /* four bits - must be at first nibble of bits */
        UBF valid_l_t_b_r          : 4;

        /* four bits - must be at second nibble of bits */
        UBF support_valid_l_t_r_b  : 4;

        UBF valid_rect             : 1;
        UBF msg_ctl_create_sent    : 1;
        UBF disabled               : 1; /* !enabled || enable_suppressed */
        UBF enabled                : 1;
        UBF enable_suppressed      : 1; /* a group higher up has turned us off */
        UBF enable_suppress_change : 1; /* enable_suppressed was changed */
        UBF radiobutton_active     : 1; /* which one of this group of radiobuttons is active */
        UBF force_update           : 1; /* transient set over self callback primarily for mlec */
        UBF nobbled                : 1; /* another disabling bit, but not reflected in control (for style editor) */

        UBF in_update              : 2;

        UBF mouse_over             : 1;
        UBF mouse_down             : 1;

        UBF spare : 8*sizeof(int) - (2*4 + 9*1 + 2 + 2*1);
    } bits;

    union DIALOG_ICTL_DATA data;

    PIXIT_RECT pixit_rect; /* relative to root window */

    DIALOG_CTL_STATE state;

#if RISCOS
    struct DIALOG_ICTL_RISCOS
    {
        U8 hot_key;
        U8 _spare[3];

        P_SBSTR caption; /* locked-down SBSTR U8 Latin-N copy for RISC OS Window Manager, hot key stripped */

        DIALOG_WIMP_I dwi[3];
    } riscos;
#elif WINDOWS
    struct DIALOG_ICTL_WINDOWS
    {
        UINT wid;
        WNDPROC prev_proc; /* when subclassed */
    } windows;
#endif
}
DIALOG_ICTL, * P_DIALOG_ICTL, ** P_P_DIALOG_ICTL; typedef const DIALOG_ICTL * PC_DIALOG_ICTL;

/*
internal structure to maintain a dialog box
*/

#if RISCOS
typedef struct DIALOG_RISCOS_RESOURCE_BITMAP_COMMON
{
    RESOURCE_BITMAP_HANDLE handle;
    GDI_SIZE size;
}
DIALOG_RISCOS_RESOURCE_BITMAP_COMMON, * P_DIALOG_RISCOS_RESOURCE_BITMAP_COMMON;
#endif

typedef struct DIALOG
{
    H_DIALOG h_dialog; /* quick ref back to ourselves */

    P_PROC_DIALOG_EVENT p_proc_client; /* callback event procedure */
    CLIENT_HANDLE client_handle; /* client's handle to this dialog*/

    STATUS help_topic_resource_id;

    S32 completion_code;

    DIALOG_ICTL_GROUP ictls; /* top-level DIALOG_ICTLs */

    DIALOG_CONTROL_ID current_dialog_control_id; /* which control has the input focus, 0 if none */
    DIALOG_CONTROL_ID default_dialog_control_id;
    DIALOG_CONTROL_ID help_dialog_control_id;

    HOST_WND hwnd;

    DOCNO docno; /* which document to send events to */
    U8 _spare[3];

    int change_level; /* indicate when recursive change processes have got back to the top */

    BOOL had_pointer;
    BOOL has_nulls;
    BOOL msg_create_sent;
    BOOL modeless;
    BOOL stolen_focus;
    BOOL stolen_focus_from_doc;
    BOOL big_mods;

    MAEVE_HANDLE maeve_handle;

    struct DIALOG_STOLEN_FOCUS
    {
        S32 maeve_handle;
        DOCNO docno;
        U8 _spare[3];
        S32 focus_owner;
        P_PROC_EVENT focus_owner_p_proc_event;
    } stolen_focus_;

    UI_TEXT caption; /* dialog owned */

    PIXIT_SIZE pixit_size;

#if RISCOS
    struct DIALOG_RISCOS
    {
        P_SBSTR caption; /* locked-down SBSTR U8 Latin-N copy for RISC OS Window Manager */

        GDI_POINT gdi_offset_tl;

        BBox invalid_bbox;

        WimpCaret stolen_focus_caretstr;

        DIALOG_RISCOS_RESOURCE_BITMAP_COMMON bitmap_radio_off, bitmap_radio_on;
        DIALOG_RISCOS_RESOURCE_BITMAP_COMMON bitmap_check_off, bitmap_check_on;
        DIALOG_RISCOS_RESOURCE_BITMAP_COMMON bitmap_tristate_off, bitmap_tristate_on, bitmap_tristate_dont_care;
        DIALOG_RISCOS_RESOURCE_BITMAP_COMMON bitmap_dec, bitmap_inc;
        DIALOG_RISCOS_RESOURCE_BITMAP_COMMON bitmap_dropdown;

        BOOL accumulate_box;
    } riscos;
#elif WINDOWS
    struct DIALOG_WINDOWS
    {
        HGLOBAL dlgtemplate; /* disposed of immediately */
        ARRAY_HANDLE h_windows_ctl_map; /* [] of WINDOWS_CTL_MAP */
        HWND hwnd_parent;
        HWND hwnd_idok;
        HWND hwnd_idcancel;
        POINT wdu_offset_tl;
        HFONT hfont;
        HHOOK dlg_filter_hook;
        UINT windows_control_id_pressed;
        BOOL help_engine_used;
        PTSTR help_filename;
        HTHEME hTheme_Button;
    } windows;
#endif /* OS */
}
DIALOG, * P_DIALOG, ** P_P_DIALOG; typedef const DIALOG * PC_DIALOG;

/*
internal functions as macros from ob_dlg.c
*/

#define n_ictls_from_group(p_ictl_group) \
    array_elements(&p_ictl_group->handle)

#define p_dialog_ictls_from_group(p_ictl_group, n_ictls) \
    array_range(&p_ictl_group->handle, DIALOG_ICTL, 0, n_ictls)

#define p_dialog_ictl_from(p_ictl_group, i) \
    array_ptr(&p_ictl_group->handle, DIALOG_ICTL, i)

#define DIALOG_MSG_HDR_from_dialog(msg_hdr__ref, p_dialog) \
    msg_hdr__ref.h_dialog = (p_dialog)->h_dialog

#define DIALOG_MSG_CTL_HDR_from_dialog_ictl(msg_ctl_hdr__ref, p_dialog, p_dialog_ictl) \
    msg_ctl_hdr__ref.h_dialog = (p_dialog)->h_dialog; \
    msg_ctl_hdr__ref.dialog_control_id = (p_dialog_ictl)->p_dialog_control->dialog_control_id; \
    msg_ctl_hdr__ref.p_dialog_control = (p_dialog_ictl)->p_dialog_control; \
    msg_ctl_hdr__ref.p_dialog_control_data = (p_dialog_ictl)->p_dialog_control_data.p_any

/*
internally exported routines from ob_dlg.c
*/

/*
internally exported routines from ob_dlg2.c
*/

_Check_return_
extern STATUS
dialog_call_client(
    P_DIALOG p_dialog,
    _InVal_     DIALOG_MESSAGE dialog_message,
    /*_Inout_*/ P_ANY p_data,
    _InRef_     P_PROC_DIALOG_EVENT p_proc_client);

_Check_return_
extern STATUS
dialog_click_bump_xx(
    P_DIALOG p_dialog,
    P_DIALOG_ICTL p_dialog_ictl,
    _InVal_     BOOL inc);

_Check_return_
extern STATUS
dialog_click_checkbox(
    P_DIALOG p_dialog,
    P_DIALOG_ICTL p_dialog_ictl);

_Check_return_
extern STATUS
dialog_click_pushbutton(
    _In_        P_DIALOG p_dialog,
    _In_        P_DIALOG_ICTL p_dialog_ictl,
    _InVal_     BOOL right_button,
    _InVal_     DIALOG_CONTROL_ID double_dialog_control_id);

_Check_return_
extern STATUS
dialog_click_radiobutton(
    P_DIALOG p_dialog,
    P_DIALOG_ICTL p_dialog_ictl);

_Check_return_
extern STATUS
dialog_click_tristate(
    P_DIALOG p_dialog,
    P_DIALOG_ICTL p_dialog_ictl);

extern DIALOG_CONTROL_ID
dialog_control_id_of_group(
    P_DIALOG p_dialog,
    P_DIALOG_ICTL_GROUP p_group_ictls);

extern void
dialog_control_rect(
    P_DIALOG p_dialog,
    _InVal_     DIALOG_CONTROL_ID dialog_control_id,
    _OutRef_opt_ P_PIXIT_RECT p_rect /*NULL->ensure*/);

extern void
dialog_control_rect_changed(
    P_DIALOG p_dialog,
    _InVal_     DIALOG_CONTROL_ID dialog_control_id,
    _InRef_     PC_BITMAP p_bitmap);

extern DIALOG_CONTROL_ID
dialog_current_move(
    P_DIALOG p_dialog,
    _InVal_     DIALOG_CONTROL_ID current_dialog_control_id,
    _InVal_     STATUS movement);

extern void
dialog_current_set(
    P_DIALOG p_dialog,
    _InVal_     DIALOG_CONTROL_ID dialog_control_id,
    _InVal_     BOOL disallow_movement);

_Check_return_
extern STATUS
dialog_cmd_complete_dbox(
    P_DIALOG_CMD_COMPLETE_DBOX p_dialog_cmd_complete_dbox);

_Check_return_
extern STATUS
dialog_cmd_dispose_dbox(
    P_DIALOG_CMD_DISPOSE_DBOX p_dialog_cmd_dispose_dbox);

extern void
dialog_dbox_dispose(
    _InVal_     H_DIALOG h_dialog);

T5_MSG_PROTO(extern, dialog_dbox_process, P_DIALOG_CMD_PROCESS_DBOX p_dialog_cmd_process_dbox);

/*
internally exported routines from ob_dlg3.c
*/

_Check_return_
_Ret_maybenull_
extern P_PROC_DIALOG_EVENT
dialog_find_handler(
    _InRef_     PC_DIALOG p_dialog,
    _InVal_     DIALOG_CONTROL_ID dialog_control_id,
    _OutRef_    P_CLIENT_HANDLE p_client_handle);

_Check_return_
extern STATUS
dialog_ictls_bbox_in(
    P_DIALOG p_dialog,
    _InRef_     PC_DIALOG_ICTL_GROUP p_ictl_group,
    _InoutRef_  P_PIXIT_RECT p_rect);

_Check_return_
extern STATUS
dialog_ictls_create(
    P_DIALOG p_dialog,
    _InVal_     U32 n_ctls,
    _In_reads_(n_ctls) P_DIALOG_CTL_CREATE p_dialog_ctl_create);

extern void
dialog_ictls_dispose_in(
    P_DIALOG p_dialog,
    /*inout*/ P_DIALOG_ICTL_GROUP p_ictl_group);

_Check_return_
extern STATUS
dialog_ictls_enable_in(
    P_DIALOG p_dialog,
    P_DIALOG_ICTL_GROUP p_ictl_group);

_Check_return_
extern STATUS
dialog_ictls_encode_in(
    P_DIALOG p_dialog,
    P_DIALOG_ICTL_GROUP p_ictl_group,
    _InVal_     S32 bits);

_Check_return_
_Ret_maybenull_
extern P_PROC_DIALOG_EVENT
dialog_main_handler(
    _InRef_     PC_DIALOG p_dialog,
    _OutRef_    P_CLIENT_HANDLE p_client_handle);

_Check_return_
extern P_DIALOG
p_dialog_from_h_dialog(
    _InVal_     H_DIALOG h_dialog);

extern P_DIALOG_ICTL_EDIT_XX
p_dialog_ictl_edit_xx_from(
    P_DIALOG_ICTL p_dialog_ictl);

extern P_DIALOG_ICTL
p_dialog_ictl_from_control_id(
    P_DIALOG p_dialog,
    _InVal_     DIALOG_CONTROL_ID dialog_control_id);

extern P_DIALOG_ICTL
p_dialog_ictl_from_control_id_in(
    P_DIALOG_ICTL_GROUP p_ictl_group,
    _InVal_     DIALOG_CONTROL_ID dialog_control_id,
    /*out*/ P_P_DIALOG_ICTL_GROUP p_p_parent_ictls);

MAEVE_EVENT_PROTO(extern, maeve_event_dialog);

MAEVE_EVENT_PROTO(extern, maeve_event_dialog_stolen_focus);

#if RISCOS

_Check_return_
extern STATUS
ui_text_from_ictl_edit_xx(
    _OutRef_    P_UI_TEXT p_ui_text,
    P_DIALOG_ICTL_EDIT_XX p_dialog_ictl_edit_xx);

#elif WINDOWS

_Check_return_
extern STATUS
ui_text_from_ictl_edit_xx(
    _OutRef_    P_UI_TEXT p_ui_text,
    P_DIALOG p_dialog,
    P_DIALOG_ICTL p_dialog_ictl);

#endif /* OS */

_Check_return_
extern STATUS
ui_text_state_change(
    _InoutRef_  P_UI_TEXT p_ui_cur,
    _InRef_     PC_UI_TEXT p_ui_new,
    _OutRef_    P_BOOL p_changed);

#if RISCOS

#define DISABLED_BLACK_ON_GREY  '\x03' /* was 2, but 3 stands out better on grey dialog bg 1 */
#define DISABLED_BLACK_ON_WHITE '\x02'

typedef struct DIALOG_RISCOS_REDRAW_WINDOW
{
    REDRAW_CONTEXT redraw_context; /*IN*/
    WimpRedrawWindowBlock redraw_window_block; /*IN*/
}
DIALOG_RISCOS_REDRAW_WINDOW, * P_DIALOG_RISCOS_REDRAW_WINDOW;

/*
callback routines internally exported from ri_dlg.c
*/

_Check_return_
extern int
dialog_riscos_dbox_event_handler(
    _InVal_     int event_code,
    _In_        const WimpPollBlock * const p_event_data,
    P_ANY handle);

/*
internally exported routines from ri_dlg.c
*/

extern void
dialog_riscos_box_from_pixit_rect(
    _OutRef_    P_GDI_BOX p_box,
    _InRef_     PC_PIXIT_RECT p_rect);

extern void
dialog_riscos_big_mods(
    P_DIALOG p_dialog,
    _InVal_     BOOL start);

extern void
dialog_riscos_cache_common_bitmaps(
    P_DIALOG p_dialog);

extern void
dialog_riscos_dbox_modify_open_type(
    P_DIALOG p_dialog,
    _InoutRef_  P_S32 p_type);

_Check_return_
extern STATUS
dialog_riscos_event(
    _InVal_     DIALOG_MESSAGE dialog_message,
    P_ANY p_data);

extern void
dialog_riscos_free_cached_bitmaps(
    P_DIALOG p_dialog);

_Check_return_
extern STATUS
dialog_riscos_icon_recreate(
    P_DIALOG p_dialog,
    _In_        const WimpIconBlockWithBitset * const p_icon,
    _InoutRef_  P_DIALOG_WIMP_I p_i);

_Check_return_ _Success_(status_ok(return))
extern STATUS
dialog_riscos_icon_recreate_prepare(
    P_DIALOG p_dialog,
    _Out_       WimpIconBlockWithBitset * const p_icon,
    _InRef_     PC_DIALOG_WIMP_I p_i);

_Check_return_
extern STATUS
dialog_riscos_icon_recreate_with(
    P_DIALOG p_dialog,
    _Inout_     WimpIconBlockWithBitset * const p_icon,
    _InoutRef_  P_DIALOG_WIMP_I p_i,
    RESOURCE_BITMAP_HANDLE h_bitmap);

extern void
dialog_riscos_icon_redraw_for_encode(
    P_DIALOG p_dialog,
    _InRef_     PC_DIALOG_WIMP_I p_i,
    _In_        FRAMED_BOX_STYLE border_style,
    _InVal_     S32 encode_bits);

/*ncr*/
extern BOOL
dialog_riscos_icon_text_setup(
    WimpIconBlockWithBitset * p_icon,
    _In_opt_z_  PC_U8Z p_u8);

_Check_return_
extern STATUS
dialog_riscos_ictl_edit_xx_create(
    P_DIALOG_ICTL p_dialog_ictl);

extern void
dialog_riscos_ictl_edit_xx_destroy(
    P_DIALOG p_dialog,
    P_DIALOG_ICTL p_dialog_ictl);

_Check_return_
extern STATUS
dialog_riscos_ictl_enable_here(
    P_DIALOG p_dialog,
    P_DIALOG_ICTL p_dialog_ictl);

_Check_return_
extern STATUS
dialog_riscos_ictl_focus_query(
    P_DIALOG p_dialog,
    P_DIALOG_ICTL p_dialog_ictl);

_Check_return_
extern STATUS
dialog_riscos_ictls_create_in(
    P_DIALOG p_dialog,
    _InRef_     PC_DIALOG_ICTL_GROUP p_ictl_group);

extern void
dialog_riscos_null_event(
    P_DIALOG p_dialog);

_Check_return_
extern STATUS
dialog_riscos_remove_escape(
    _InoutRef_  P_DIALOG_ICTL p_dialog_ictl,
    _InRef_     PC_UI_TEXT p_ui_text);

extern void
dialog_riscos_tweak_edit_controls(
    P_DIALOG_ICTL_GROUP p_ictl_group,
    _InVal_     BOOL add);

#endif /* RISCOS */

#if WINDOWS

/*
callback routines internally exported from wi_dlg.c
*/

_Check_return_
extern INT_PTR CALLBACK
modal_dialog_handler(
    _HwndRef_   HWND hwnd,
    _In_        UINT uiMsg,
    _In_        WPARAM wParam,
    _In_        LPARAM lParam);

_Check_return_
extern INT_PTR CALLBACK
modeless_dialog_handler(
    _HwndRef_   HWND hwnd,
    _In_        UINT uiMsg,
    _In_        WPARAM wParam,
    _In_        LPARAM lParam);

/*
internally exported routines from wi_dlg.c
*/

_Check_return_
extern STATUS
dialog_dbox_process_windows(
    _InoutRef_  P_DIALOG p_dialog,
    _InoutRef_  P_DIALOG_CMD_PROCESS_DBOX p_dialog_cmd_process_dbox,
    _InRef_     PC_PIXIT_RECT p_pixit_rect);

extern void
dialog_windows_build_list(
    _InoutRef_  P_DIALOG p_dialog,
    P_DIALOG_ICTL p_dialog_ictl);

_Check_return_
extern STATUS
dialog_windows_help(
    _InoutRef_  P_DIALOG p_dialog);

extern void
dialog_windows_ictl_enable_here(
    _InoutRef_  P_DIALOG p_dialog,
    P_DIALOG_ICTL p_dialog_ictl);

extern void
dialog_windows_ui_len_init(void);

#endif /* WINDOWS */

/*
internally exported variables
*/

/* dialog statics shared by ob_dlg modules */

typedef struct DIALOG_STATICS
{
    BITMAP(bitmap_validation_f64, 256);
    BITMAP(bitmap_validation_s32, 256);
    BITMAP(bitmap_validation_u8,  256);

    BITMAP(bitmap_validation_edit,           256);
    BITMAP(bitmap_validation_edit_multiline, 256);

    ARRAY_HANDLE handles;
    CLIENT_HANDLE handle_gen;

    ARRAY_INIT_BLOCK ictls_init_block;

    GDI_POINT noted_gdi_tl;
    BOOL note_position;
    BOOL noted_position;

#if WINDOWS
    struct DIALOG_STATICS_WINDOWS
    {
        LOGFONT logfont;
        HFONT hfont; /* used for sizing up */
        S32 pixels_per_four_h_du;
        S32 pixels_per_eight_v_du;
    } windows;
#endif
}
DIALOG_STATICS;

extern DIALOG_STATICS dialog_statics;

/* end of ui_dlgin.h */
