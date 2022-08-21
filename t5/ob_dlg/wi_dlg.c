/* wi_dlg.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1994-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* dialog UI handling (Windows) */

/* SKS Jan 1994 */

#include "common/gflags.h"

#include "ob_dlg/ui_dlgin.h"

#if WINDOWS

#include "external/Microsoft/InsideOLE2/BTTNCURP/bttncur.h"

#include <htmlhelp.h>

#pragma comment(lib, "uxtheme.lib")

#include <vssym32.h>

/*
internal types
*/

typedef const DRAWITEMSTRUCT * LPCDRAWITEMSTRUCT; /* windows.h inadequate */

typedef struct WINDOWS_CTL_MAP
{
    DIALOG_CONTROL_ID dialog_control_id;
    HWND hwnd;
}
WINDOWS_CTL_MAP, * P_WINDOWS_CTL_MAP;

#define PM_CALLHELP (WM_USER + 0x100)
/* wParam and lParam can be used to pass context information for dialog filter proc. tosser Windows uses WM_USER + 0,1 for it's own dialog messsages */

/*
internal routines
*/

_Check_return_
static DIALOG_CONTROL_ID
control_id_from_windows_control_id(
    _InRef_     PC_DIALOG p_dialog,
    _InVal_     UINT windows_control_id);

_Check_return_
static STATUS
dialog_windows_dlgtemplate_prepare(
    _InoutRef_  P_DIALOG p_dialog,
    _InRef_     PC_PIXIT_RECT p_pixit_rect,
    _InRef_opt_ PC_GDI_POINT p_gdi_tl);

_Check_return_
static BOOL
dialog_onInitDialog(
    _HwndRef_   HWND hwnd,
    _HwndRef_   HWND hwndFocus,
    _In_        LPARAM lParam);

_Check_return_
static BOOL
dialog_onCommand(
    _HwndRef_   HWND hwnd,
    _In_        int id,
    _HwndRef_   HWND hwndCtl,
    _InVal_     UINT codeNotify);

static void
dialog_onDestroy(
    _HwndRef_   HWND hwnd);

static void
dialog_onSetFont(
    _HwndRef_   HWND hwnd,
    _In_        HFONT hfont,
    _InVal_     BOOL fRedraw);

static void
dialog_onDrawItem(
    _HwndRef_   HWND hwnd,
    _In_        const DRAWITEMSTRUCT * const pDrawItem);

static void
dialog_onMeasureItem(
    _HwndRef_   HWND hwnd,
    _In_        MEASUREITEMSTRUCT * const pMeasureItem);

static LRESULT
dialog_onNotify(
    _HwndRef_   HWND hwnd,
    _In_        int wParam,
    _In_reads_bytes_c_(sizeof32(NMUPDOWN)) NMHDR * p_nmhdr);

static void
dialog_onThemeChanged(
    _HwndRef_   HWND hwnd);

static HHOOK g_hhook; /* nasty */

#define WINDOWS_CTL_ID_STT 256

_Check_return_
static DIALOG_CONTROL_ID
control_id_from_windows_control_id(
    _InRef_     PC_DIALOG p_dialog,
    _InVal_     UINT windows_control_id)
{
    ARRAY_INDEX map_index;

    if((windows_control_id == IDOK) || (windows_control_id == IDCANCEL))
        return((DIALOG_CONTROL_ID) windows_control_id);

    if(windows_control_id < WINDOWS_CTL_ID_STT)
        return(0);

    map_index = ((ARRAY_INDEX) windows_control_id) - WINDOWS_CTL_ID_STT;

    if(!array_index_is_valid(&p_dialog->windows.h_windows_ctl_map, map_index))
        return(0);

    return(array_ptr(&p_dialog->windows.h_windows_ctl_map, WINDOWS_CTL_MAP, map_index)->dialog_control_id);
}

static void
convert_colour_using_tint(
    _InoutRef_  P_RGB p_rgb,
    _InVal_     S32 tint_in /*0...100*/)
{
    RGB rgb = *p_rgb;
    U8 tint = (U8) tint_in;
    p_rgb->r = (U8) ((rgb.r * tint + 0xFF * (100 - tint)) / 100);
    p_rgb->g = (U8) ((rgb.g * tint + 0xFF * (100 - tint)) / 100);
    p_rgb->b = (U8) ((rgb.b * tint + 0xFF * (100 - tint)) / 100);
}

_Check_return_
static STATUS
dialog_windows_help_contents(
    _InoutRef_  P_DIALOG p_dialog)
{
    if(!WrapOsBoolChecking(NULL != HtmlHelp(p_dialog->hwnd, p_dialog->windows.help_filename, HH_DISPLAY_TOC, 0)))
        return(create_error(ERR_HELP_FAILURE));

    p_dialog->windows.help_engine_used = TRUE;

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_windows_help_topic(
    _InoutRef_  P_DIALOG p_dialog)
{
    TCHARZ filename[BUF_MAX_PATHSTRING];
    PCTSTR tstr_help_topic;
    
    if(NULL == (tstr_help_topic = resource_lookup_tstr_no_default(p_dialog->help_topic_resource_id)))
        return(dialog_windows_help_contents(p_dialog));

    tstr_xstrkpy(filename, elemof32(filename), p_dialog->windows.help_filename);
    tstr_xstrkat(filename, elemof32(filename), TEXT("::/html/"));
    tstr_xstrkat(filename, elemof32(filename), tstr_help_topic);

    if(!WrapOsBoolChecking(NULL != HtmlHelp(p_dialog->hwnd, filename, HH_DISPLAY_TOPIC, 0)))
        return(create_error(ERR_HELP_FAILURE));

    p_dialog->windows.help_engine_used = TRUE;

    return(STATUS_OK);
}

_Check_return_
extern STATUS
dialog_windows_help(
    _InoutRef_  P_DIALOG p_dialog)
{
    {
    PCTSTR filename = leafname_helpfile_tstr;
    TCHARZ help_filename[BUF_MAX_PATHSTRING];

    if(file_find_on_path(help_filename, elemof32(help_filename), filename) <= 0)
        return(create_error(ERR_HELP_FAILURE));

    tstr_clr(&p_dialog->windows.help_filename);
    status_return(tstr_set(&p_dialog->windows.help_filename, help_filename));
    } /*block*/

    if(p_dialog->windows.help_filename)
    {
        if(p_dialog->help_topic_resource_id)
            return(dialog_windows_help_topic(p_dialog));
        else
            return(dialog_windows_help_contents(p_dialog));
    }

    return(STATUS_OK);
}

static H_DIALOG bastard_windows_h_dialog; /* Windows 3.1 inadequacy */

#define h_dialog_from_hwnd(hwnd) ( \
    (H_DIALOG) GetWindowLongPtr(hwnd, DWLP_USER))

/******************************************************************************
*
* normal modal dialog box handling via DialogBox
*
******************************************************************************/

_Check_return_
static BOOL
modal_dialog_pm_callhelp(
    _HwndRef_   HWND hwnd)
{
    const H_DIALOG h_dialog = h_dialog_from_hwnd(hwnd);
    const P_DIALOG p_dialog = p_dialog_from_h_dialog(h_dialog);
    status_assert(dialog_windows_help(p_dialog));
    return(FALSE);
}

_Check_return_
extern INT_PTR CALLBACK
modal_dialog_handler(
    _HwndRef_   HWND hwnd,
    _In_        UINT uiMsg,
    _In_        WPARAM wParam,
    _In_        LPARAM lParam)
{
    switch(uiMsg)
    {
    HANDLE_DLGMSG(hwnd, WM_INITDIALOG,      dialog_onInitDialog); /* MUST be created via DialogBoxIndirectParam to get handle thru here */
    HANDLE_DLGMSG(hwnd, WM_DESTROY,         dialog_onDestroy);
    HANDLE_DLGMSG(hwnd, WM_SETFONT,         dialog_onSetFont);
    HANDLE_DLGMSG(hwnd, WM_MEASUREITEM,     dialog_onMeasureItem);
    HANDLE_DLGMSG(hwnd, WM_DRAWITEM,        dialog_onDrawItem);

    HANDLE_MSG(hwnd,    WM_NOTIFY,          dialog_onNotify);
    HANDLE_MSG(hwnd,    WM_THEMECHANGED,    dialog_onThemeChanged);

    case WM_COMMAND:
        return(dialog_onCommand(hwnd, (int) LOWORD(wParam), (HWND) lParam, (UINT) HIWORD(wParam)));

    case PM_CALLHELP:
        return(modal_dialog_pm_callhelp(hwnd));

    default:
        return(FALSE); /* no DefDialogProc to call on Windows; we return to dialog processing */
    }
}

/******************************************************************************
*
* modeless dialog box handling via CreateDialog
*
******************************************************************************/

_Check_return_
extern INT_PTR CALLBACK
modeless_dialog_handler(
    _HwndRef_   HWND hwnd,
    _In_        UINT uiMsg,
    _In_        WPARAM wParam,
    _In_        LPARAM lParam)
{
    switch(uiMsg)
    {
    HANDLE_DLGMSG(hwnd, WM_INITDIALOG,      dialog_onInitDialog); /* MUST be created via CreateDialogIndirectParam to get handle thru here */
    HANDLE_DLGMSG(hwnd, WM_DESTROY,         dialog_onDestroy);
    HANDLE_DLGMSG(hwnd, WM_SETFONT,         dialog_onSetFont);
    HANDLE_DLGMSG(hwnd, WM_MEASUREITEM,     dialog_onMeasureItem);
    HANDLE_DLGMSG(hwnd, WM_DRAWITEM,        dialog_onDrawItem);

    HANDLE_MSG(hwnd,    WM_NOTIFY,          dialog_onNotify);
    HANDLE_MSG(hwnd,    WM_THEMECHANGED,    dialog_onThemeChanged);

    case WM_COMMAND:
        return(dialog_onCommand(hwnd, (int) LOWORD(wParam), (HWND) lParam, (UINT) HIWORD(wParam)));

    default:
        return(DefWindowProc(hwnd, uiMsg, wParam, lParam));
    }
}

static void
dialog_ictls_make_lists(
    _InoutRef_  P_DIALOG p_dialog,
    P_DIALOG_ICTL_GROUP p_ictl_group)
{
    ARRAY_INDEX i;

    for(i = 0; i < n_ictls_from_group(p_ictl_group); ++i)
    {
        const P_DIALOG_ICTL p_dialog_ictl = p_dialog_ictl_from(p_ictl_group, i);

        switch(p_dialog_ictl->dialog_control_type)
        {
        default:
            break;

        case DIALOG_CONTROL_GROUPBOX:
            dialog_ictls_make_lists(p_dialog, &p_dialog_ictl->data.groupbox.ictls);
            break;

        case DIALOG_CONTROL_LIST_S32:
        case DIALOG_CONTROL_LIST_TEXT:
        case DIALOG_CONTROL_COMBO_S32:
        case DIALOG_CONTROL_COMBO_TEXT:
            dialog_windows_build_list(p_dialog, p_dialog_ictl);
            break;
        }
    }
}

static BOOL bMouseOverButton;

static void
SubclassedDialogControl_onLButtonDown(
    _HwndRef_   HWND hwnd,
    _InoutRef_  P_DIALOG_ICTL p_dialog_ictl)
{
    p_dialog_ictl->bits.mouse_down = 1;
    InvalidateRect(hwnd, NULL, FALSE);
}

static void
SubclassedDialogControl_onLButtonUp(
    _HwndRef_   HWND hwnd,
    _InoutRef_  P_DIALOG_ICTL p_dialog_ictl)
{
    p_dialog_ictl->bits.mouse_down = 0;
    InvalidateRect(hwnd, NULL, FALSE);
}

static void
SubclassedDialogControl_onMouseMove(
    _HwndRef_   HWND hwnd,
    _InoutRef_  P_DIALOG_ICTL p_dialog_ictl)
{
    if(!host_set_tracking_for_window(hwnd))
        return;

    if(0 == p_dialog_ictl->bits.mouse_over)
    {
        p_dialog_ictl->bits.mouse_over = 1;
        InvalidateRect(hwnd, NULL, FALSE);
    }
}

static void
SubclassedDialogControl_onMouseLeave(
    _HwndRef_   HWND hwnd,
    _InoutRef_  P_DIALOG_ICTL p_dialog_ictl)
{
    if(1 == p_dialog_ictl->bits.mouse_over)
    {
        p_dialog_ictl->bits.mouse_over = 0;
        InvalidateRect(hwnd, NULL, FALSE);
    }
}

LRESULT CALLBACK
SubclassedDialogControlProc(
    _HwndRef_   HWND hwnd,
    _In_        UINT message,
    _In_        WPARAM wParam,
    _In_        LPARAM lParam)
{
    const H_DIALOG h_dialog = bastard_windows_h_dialog; /* Windows inadequacy */
    const P_DIALOG p_dialog = p_dialog_from_h_dialog(h_dialog);
    const DIALOG_CONTROL_ID dialog_control_id = (DIALOG_CONTROL_ID) GetWindowLong(hwnd, GWLP_USERDATA);
    P_DIALOG_ICTL p_dialog_ictl;

    if((0 == dialog_control_id) || (NULL == p_dialog))
    {
        assert0();
        return(0L);
    }

    p_dialog_ictl = p_dialog_ictl_from_control_id(p_dialog, dialog_control_id);

    switch(message)
    {
    case WM_LBUTTONDOWN:
        SubclassedDialogControl_onLButtonDown(hwnd, p_dialog_ictl);
        break;

    case WM_LBUTTONUP:
        SubclassedDialogControl_onLButtonUp(hwnd, p_dialog_ictl);
        break;

    case WM_MOUSEMOVE:
        SubclassedDialogControl_onMouseMove(hwnd, p_dialog_ictl);
        break;

    case WM_MOUSELEAVE:
        SubclassedDialogControl_onMouseLeave(hwnd, p_dialog_ictl);
        break;

    default:
        break;
    }

    /* Any messages we don't fully process must be passed onto the original window function */
    return(CallWindowProc(p_dialog_ictl->windows.prev_proc, hwnd, message, wParam, lParam));
}

static void
dialog_ictls_subclass(
    _InoutRef_  P_DIALOG p_dialog,
    P_DIALOG_ICTL_GROUP p_ictl_group)
{
    ARRAY_INDEX i;

    for(i = 0; i < n_ictls_from_group(p_ictl_group); ++i)
    {
        const P_DIALOG_ICTL p_dialog_ictl = p_dialog_ictl_from(p_ictl_group, i);

        switch(p_dialog_ictl->dialog_control_type)
        {
        default:
            break;

        case DIALOG_CONTROL_GROUPBOX:
            dialog_ictls_subclass(p_dialog, &p_dialog_ictl->data.groupbox.ictls);
            break;

        case DIALOG_CONTROL_PUSHPICTURE:
        case DIALOG_CONTROL_RADIOPICTURE:
        case DIALOG_CONTROL_CHECKPICTURE:
        case DIALOG_CONTROL_TRIPICTURE:
            {
            const HWND hwnd = GetDlgItem(p_dialog->hwnd, p_dialog_ictl->windows.wid);
            consume(LONG, SetWindowLong(hwnd, GWLP_USERDATA, (LONG) p_dialog_ictl->dialog_control_id));
            p_dialog_ictl->windows.prev_proc = (WNDPROC) SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR) SubclassedDialogControlProc);
            break;
            }

        case DIALOG_CONTROL_BUMP_S32:
        case DIALOG_CONTROL_BUMP_F64:
            {
            const HWND hwnd = GetDlgItem(p_dialog->hwnd, p_dialog_ictl->data.bump_xx.windows.spin_wid);
            const int range = 100000000;
            SendMessage(hwnd, UDM_SETRANGE32, (WPARAM) -range, (LPARAM) range);
            SendMessage(hwnd, UDM_SETPOS32, 0, (LPARAM) 0);
            break;
        }
        }
    }
}

/******************************************************************************
*
* DialogBox message handlers
*
******************************************************************************/

/******************************************************************************
*
* WM_INITDIALOG - hwnd is the handle of the dialog
*
******************************************************************************/

_Check_return_
static BOOL
dialog_onInitDialog(
    _HwndRef_   HWND hwnd,
    _HwndRef_   HWND hwndFocus,
    _In_        LPARAM lParam)
{
    const H_DIALOG h_dialog = (H_DIALOG) lParam;
    const P_DIALOG p_dialog = p_dialog_from_h_dialog(h_dialog);

    consume(LONG_PTR, SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR) lParam));

    UNREFERENCED_PARAMETER_HwndRef_(hwndFocus);

    p_dialog->hwnd = hwnd;

    if(g_nColours >= 256)
        p_dialog->windows.hTheme_Button = OpenThemeData(hwnd, VSCLASS_BUTTON);

    { /* loop over control id map and find the hwnds for all our controls */
    ARRAY_INDEX map_index;
    for(map_index = 0; map_index < array_elements(&p_dialog->windows.h_windows_ctl_map); ++map_index)
    {
        P_WINDOWS_CTL_MAP p_windows_ctl_map = array_ptr(&p_dialog->windows.h_windows_ctl_map, WINDOWS_CTL_MAP, map_index);
        const UINT windows_control_id = ((UINT) map_index) + WINDOWS_CTL_ID_STT;
        p_windows_ctl_map->hwnd = GetDlgItem(p_dialog->hwnd, windows_control_id);
    }
    p_dialog->windows.hwnd_idok = GetDlgItem(p_dialog->hwnd, IDOK);
    p_dialog->windows.hwnd_idcancel = GetDlgItem(p_dialog->hwnd, IDCANCEL);
    } /*block*/

    /* Windows should have already done WM_SETFONT itself to this window and all the controls */
    p_dialog->windows.hfont = dialog_statics.windows.hfont;

#if TRACE_ALLOWED
    {
    RECT rect = { 0, 0, 4, 8 };
    MapDialogRect(hwnd, &rect);
    trace_2(TRACE_APP_DIALOG, TEXT("MapDialogRect(0, 0, 4, 8) -> h_du=%d, v_du=%d"), rect.right, rect.bottom);
    } /*block*/
#endif

    if(p_dialog->modeless)
    if(HOST_WND_NONE == p_dialog->windows.hwnd_parent)
    {
        if(NULL != host_get_prog_icon_large())
            (void) /*old HICON*/ SendMessage(hwnd, WM_SETICON, ICON_BIG,   (LPARAM) host_get_prog_icon_large());
        if(NULL != host_get_prog_icon_small())
            (void) /*old HICON*/ SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM) host_get_prog_icon_small());
    }

    /* subclass all necessary buttons */
    dialog_ictls_subclass(p_dialog, &p_dialog->ictls);

    /* loop over these new control representations encoding them and enabling them as appropriate */
    dialog_ictls_make_lists(p_dialog, &p_dialog->ictls);
    status_assert(dialog_ictls_encode_in(p_dialog, &p_dialog->ictls, 0));
    status_assert(dialog_ictls_enable_in(p_dialog, &p_dialog->ictls));

    { /* tell client that dialog processing is about to start */
    CLIENT_HANDLE client_handle;
    const P_PROC_DIALOG_EVENT p_proc_client = dialog_main_handler(p_dialog, &client_handle);

    if(NULL != p_proc_client)
    {
        STATUS status;
        DIALOG_MSG_PROCESS_START dialog_msg_process_start;
        msgclr(dialog_msg_process_start);
        dialog_msg_process_start.h_dialog = p_dialog->h_dialog;
        dialog_msg_process_start.client_handle = client_handle;
        status_assert(status = dialog_call_client(p_dialog, DIALOG_MSG_CODE_PROCESS_START, &dialog_msg_process_start, p_proc_client));
    }
    } /*block*/

    return(TRUE);
}

#define WINDOWS_GROUPBOX_TM 4 /* dialog units */

/* definitions from atlwin.h */

#pragma pack(push, 1)

struct DLGTEMPLATEEX
{
	WORD dlgVer;
	WORD signature;
	DWORD helpID;
	DWORD exStyle;
	DWORD style;
	WORD cDlgItems;
	short x;
	short y;
	short cx;
	short cy;

	// Everything else in this structure is variable length,
	// and therefore must be determined dynamically

	// sz_Or_Ord menu;			// name or ordinal of a menu resource
	// sz_Or_Ord windowClass;	// name or ordinal of a window class
	// WCHAR title[titleLen];	// title string of the dialog box
	// short pointsize;			// only if DS_SETFONT is set
	// short weight;			// only if DS_SETFONT is set
	// short bItalic;			// only if DS_SETFONT is set
	// WCHAR font[fontLen];		// typeface name, if DS_SETFONT is set
};

struct DLGITEMTEMPLATEEX
{
	DWORD helpID;
	DWORD exStyle;
	DWORD style;
	short x;
	short y;
	short cx;
	short cy;
	DWORD id;

	// Everything else in this structure is variable length,
	// and therefore must be determined dynamically

	// sz_Or_Ord windowClass;	// name or ordinal of a window class
	// sz_Or_Ord title;			// title string or ordinal of a resource
	// WORD extraCount;			// bytes following creation data
};

#pragma pack(pop)

/******************************************************************************
*
* build a block of memory in a HGLOBAL for CreateDialogIndirectParam/DialogBoxIndirectParam
*
******************************************************************************/

typedef struct DLGTEMPLATE_HELPER
{
    P_DIALOG p_dialog;
    struct DLGTEMPLATEEX * p_dlgtemplate; /* points to header */
    P_BYTE p_byte; /* points to current allocation */
    U32 size;
    int windows_control_id; /* for mapping */

    struct DLGTEMPLATEEX dlgtemplateex;
}
DLGTEMPLATE_HELPER, * P_DLGTEMPLATE_HELPER;

/*
class ID values
*/

#define DTIC_BUTTON 0x0080
#define DTIC_EDIT 0x0081
#define DTIC_STATIC 0x0082
#define DTIC_LISTBOX 0x0083
#define DTIC_SCROLLBAR 0x0084
#define DTIC_COMBOBOX 0x0085

typedef struct DLGITEMHELPER
{
    struct DLGITEMTEMPLATEEX di;
    WORD diClass;
    PCTSTR diClassName;
    PC_UI_TEXT diCaption;
}
DLGITEMHELPER, * P_DLGITEMHELPER;

static const WORD dialog_windows_empty_string = 0x0000; /* Unicode string */

_Check_return_
_Ret_notnull_
static P_BYTE
dialog_windows_memcpy32(
    _Out_writes_bytes_(n_bytes) P_ANY p_out,
    _In_reads_bytes_(n_bytes) PC_ANY p_in,
    _InVal_     U32 n_bytes)
{
    assert(0 != n_bytes);
    memcpy32(p_out, p_in, n_bytes);
    return(PtrAddBytes(P_BYTE, p_out, n_bytes));
}

#define dialog_windows_tstrlen32p1_bytes(tstr) /* NB result in bytes */ \
    (tstrlen32p1(tstr) * sizeof32(WCHAR))

_Check_return_
_Ret_notnull_
static P_BYTE
dialog_windows_wstr_from_tstr(
    _Out_       P_BYTE p_byte,
    _In_z_      PCTSTR tstr)
{
    WCHAR * p_out = (WCHAR *) p_byte;
    U32 b = 0;
    for(;;)
    {
        WORD c = *tstr++;
        *p_out++ = (WCHAR) (c & 0xFF);
        b += sizeof32(c);
        if(0 == c)
            break;
    }
    return(p_byte + b);
}

_Check_return_
static STATUS
dialog_windows_dlgtemplate_prepare_controls_in(
    _InoutRef_  P_DLGTEMPLATE_HELPER p_dh,
    P_DIALOG_ICTL_GROUP p_dialog_ictls_from_group,
    _In_        UINT pass)
{
    const P_DIALOG p_dialog = p_dh->p_dialog;
    STATUS status = STATUS_OK;
    ARRAY_INDEX i;
    const WORD unicode_class_marker = 0xFFFF;
    const WORD no_creation_data = 0x0000;

    for(i = 0; i < n_ictls_from_group(p_dialog_ictls_from_group); ++i)
    {
        const P_DIALOG_ICTL p_dialog_ictl = p_dialog_ictl_from(p_dialog_ictls_from_group, i);
        const PC_DIALOG_CONTROL_DATA p_dialog_control_data = p_dialog_ictl->p_dialog_control_data;
        PCTSTR tstr;
        U32 b;

        {
        DLGITEMHELPER cd[2];
        P_DLGITEMHELPER pcd = &cd[0];
        PIXIT_RECT pixit_rect;
        WORD n_cw;
        UINT n_id, j;

        dialog_control_rect(p_dialog, p_dialog_ictl->dialog_control_id, &pixit_rect);

        switch(p_dialog_ictl->dialog_control_type)
        {
        case DIALOG_CONTROL_COMBO_S32:
        case DIALOG_CONTROL_COMBO_TEXT:
            assert(p_dialog_control_data.combo_s32);
            pixit_rect.br.y += p_dialog_control_data.combo_s32->combo_xx.dropdown_size;
            break;

        default:
            break;
        }

        cd[0].di.helpID = 0;
        cd[0].di.exStyle = 0;

        /* simple transform, used to all be in multiples of PIXITS_PER_WDU but no longer... */
        cd[0].di.x  = (short) (pixit_rect.tl.x / PIXITS_PER_WDU_H);
        cd[0].di.y  = (short) (pixit_rect.tl.y / PIXITS_PER_WDU_V);
        cd[0].di.cx = (short) ((pixit_rect.br.x - pixit_rect.tl.x) / PIXITS_PER_WDU_H);
        cd[0].di.cy = (short) ((pixit_rect.br.y - pixit_rect.tl.y) / PIXITS_PER_WDU_V);

        /* transform according to desired origin within window */
        cd[0].di.x  = (short) (cd[0].di.x - p_dialog->windows.wdu_offset_tl.x);
        cd[0].di.y  = (short) (cd[0].di.y - p_dialog->windows.wdu_offset_tl.y);

        /* leaves di.style to be ORed into and di.id empty */
        cd[0].di.style = WS_CHILD | WS_VISIBLE;
        cd[0].diClass = DTIC_BUTTON;
        cd[0].diCaption = NULL;
        cd[0].diClassName = NULL;

        cd[1] = cd[0];

        /* however we generally have one control per ictl. windows_control_id is reset at start of pass 2 so don't worry */
        n_cw = 1;
        n_id = 1;
        p_dialog_ictl->windows.wid = (p_dh->windows_control_id)++;
        pcd->di.id = /*(WORD)*/ p_dialog_ictl->windows.wid;

        switch(p_dialog_ictl->dialog_control_type)
        {
        case DIALOG_CONTROL_GROUPBOX:
            if(p_dialog_ictl->p_dialog_control->bits.logical_group || !p_dialog_control_data.groupbox || p_dialog_control_data.groupbox->bits.logical_group)
            {
                p_dialog_ictl->windows.wid = 0; /* no need to reset pcd->di.id 'cos we're deleting it completely */
                n_cw--;
                n_id--;
                (p_dh->windows_control_id)--;
            }
            else
            {
                pcd->diCaption = &p_dialog_ictl->caption;
                pcd->di.style |= BS_GROUPBOX;
                /* pcd->di.y += WINDOWS_GROUPBOX_TM; */
                /* pcd->di.cy -= WINDOWS_GROUPBOX_TM; */
                if(pass == 2)
                trace_6(TRACE_APP_DIALOG, TEXT("GROUPBOX text=\"%s\", id=%d, x=") U32_TFMT TEXT(", y=") U32_TFMT TEXT(", w=") U32_TFMT TEXT(", h=") U32_TFMT TEXT(" (BUTTON)"),
                    report_tstr(ui_text_tstr(&p_dialog_ictl->caption)), pcd->di.id, (U32) pcd->di.x, (U32) pcd->di.y, (U32) pcd->di.cx, (U32) pcd->di.cy);
            }
            break;

        case DIALOG_CONTROL_STATICFRAME:
            assert(p_dialog_control_data.staticframe);
            assert(p_dialog_control_data.staticframe->bits.border_style);
            {
                pcd->diClass = DTIC_STATIC;
                pcd->di.style |= SS_SUNKEN;

                if(pass == 2)
                trace_5(TRACE_APP_DIALOG, TEXT("STATIC id=%d, x=") U32_TFMT TEXT(", y=") U32_TFMT TEXT(", w=") U32_TFMT TEXT(", h=") U32_TFMT TEXT(" (sunken frame)"),
                    pcd->di.id, (U32) pcd->di.x, (U32) pcd->di.y, (U32) pcd->di.cx, (U32) pcd->di.cy);

                pcd++;
                n_cw++;
                n_id++;

                p_dialog_ictl->windows.wid = (p_dh->windows_control_id)++; /* override one stored in dialog_ictl 'cos we don't want to know the frame's id (but must still allocate one) */
                pcd->di.id = (WORD) p_dialog_ictl->windows.wid;

                pcd->di.x += 1;
                pcd->di.y += 1;
                pcd->di.cx -= 2;
                pcd->di.cy -= 2;
            } /*block*/

            /*FALLTHRU*/

        case DIALOG_CONTROL_STATICTEXT:
            pcd->diClass = DTIC_STATIC;
            pcd->diCaption = &p_dialog_ictl->state.statictext;
            assert(p_dialog_control_data.statictext);
            if(p_dialog_control_data.statictext->bits.centre_text)
                pcd->di.style |= SS_CENTER;
            else if(p_dialog_control_data.statictext->bits.left_text)
                pcd->di.style |= SS_LEFTNOWORDWRAP;
            else
                pcd->di.style |= SS_RIGHT;

            /* trivial attempt to vertically centre caption */
            /* at least this way they also line up with checkbox text etc. on same line */
            pcd->di.style |= SS_CENTERIMAGE;

            if(pass == 2)
            trace_7(TRACE_APP_DIALOG, TEXT("%sTEXT text=\"%s\", id=%d, x=") U32_TFMT TEXT(", y=") U32_TFMT TEXT(", w=") U32_TFMT TEXT(", h=") U32_TFMT TEXT(" (STATIC)"),
                p_dialog_control_data.statictext->bits.centre_text ? TEXT("C") : (p_dialog_control_data.statictext->bits.left_text ? TEXT("L") : TEXT("R")),
                report_tstr(ui_text_tstr(pcd->diCaption)), pcd->di.id, (U32) pcd->di.x, (U32) pcd->di.y, (U32) pcd->di.cx, (U32) pcd->di.cy);
            break;

        case DIALOG_CONTROL_PUSHBUTTON:
            if((p_dialog_ictl->dialog_control_id == IDOK) || (p_dialog_ictl->dialog_control_id == IDCANCEL))
            {
                p_dialog_ictl->windows.wid = pcd->di.id = (WORD) p_dialog_ictl->dialog_control_id;
                n_id--;
                (p_dh->windows_control_id)--;
            }
            pcd->diCaption = &p_dialog_ictl->state.pushbutton;
            if((p_dialog_ictl->dialog_control_id == IDOK) || p_dialog_control_data.pushbutton->push_xx.def_pushbutton)
                pcd->di.style |= BS_DEFPUSHBUTTON;
            else
                pcd->di.style |= BS_PUSHBUTTON;

            if(pass == 2)
            trace_6(TRACE_APP_DIALOG, TEXT("BUTTON text=\"%s\", id=%d, x=") U32_TFMT TEXT(", y=") U32_TFMT TEXT(", w=") U32_TFMT TEXT(", h=") U32_TFMT,
                report_tstr(ui_text_tstr(pcd->diCaption)), pcd->di.id, (U32) pcd->di.x, (U32) pcd->di.y, (U32) pcd->di.cx, (U32) pcd->di.cy);
            break;

        case DIALOG_CONTROL_PUSHPICTURE:
            if((p_dialog_ictl->dialog_control_id == IDOK) || (p_dialog_ictl->dialog_control_id == IDCANCEL))
            {
                pcd->di.id = (WORD) p_dialog_ictl->dialog_control_id;
                n_id--;
                (p_dh->windows_control_id)--;
            }
            assert(p_dialog_control_data.pushpicture);
            assert(!p_dialog_control_data.pushpicture->push_xx.def_pushbutton);
            pcd->di.style |= BS_OWNERDRAW;

            if(pass == 2)
            trace_5(TRACE_APP_DIALOG, TEXT("BUTTON OWNERDRAW id=%d, x=") U32_TFMT TEXT(", y=") U32_TFMT TEXT(", w=") U32_TFMT TEXT(", h=") U32_TFMT TEXT(" (PUSHPICTURE)"),
                pcd->di.id, (U32) pcd->di.x, (U32) pcd->di.y, (U32) pcd->di.cx, (U32) pcd->di.cy);
            break;

        case DIALOG_CONTROL_RADIOBUTTON:
            pcd->diCaption = &p_dialog_ictl->caption;
            pcd->di.style |= BS_RADIOBUTTON;

            if(pass == 2)
            trace_6(TRACE_APP_DIALOG, TEXT("RADIOBUTTON text=\"%s\", id=%d, x=") U32_TFMT TEXT(", y=") U32_TFMT TEXT(", w=") U32_TFMT TEXT(", h=") U32_TFMT,
                report_tstr(ui_text_tstr(pcd->diCaption)), pcd->di.id, (U32) pcd->di.x, (U32) pcd->di.y, (U32) pcd->di.cx, (U32) pcd->di.cy);
            break;

        case DIALOG_CONTROL_CHECKBOX:
            pcd->diCaption = &p_dialog_ictl->caption;
            pcd->di.style |= BS_CHECKBOX;

            if(pass == 2)
            trace_6(TRACE_APP_DIALOG, TEXT("CHECKBOX text=\"%s\", id=%d, x=") U32_TFMT TEXT(", y=") U32_TFMT TEXT(", w=") U32_TFMT TEXT(", h=") U32_TFMT,
                report_tstr(ui_text_tstr(pcd->diCaption)), pcd->di.id, (U32) pcd->di.x, (U32) pcd->di.y, (U32) pcd->di.cx, (U32) pcd->di.cy);
            break;

#ifdef DIALOG_HAS_TRISTATE
        case DIALOG_CONTROL_TRISTATE:
            pcd->diCaption = &p_dialog_ictl->caption;
            pcd->di.style |= BS_3STATE;

            if(pass == 2)
            trace_6(TRACE_APP_DIALOG, TEXT("STATE3 text=\"%s\", id=%d, x=") U32_TFMT TEXT(", y=") U32_TFMT TEXT(", w=") U32_TFMT TEXT(", h=") U32_TFMT,
                report_tstr(ui_text_tstr(pcd->diCaption)), pcd->di.id, (U32) pcd->di.x, (U32) pcd->di.y, (U32) pcd->di.cx, (U32) pcd->di.cy);
            break;
#endif

        case DIALOG_CONTROL_RADIOPICTURE:
            pcd->di.style |= BS_OWNERDRAW;
            break;

        case DIALOG_CONTROL_CHECKPICTURE:
            pcd->di.style |= BS_OWNERDRAW;
            break;

        case DIALOG_CONTROL_TRIPICTURE:
            pcd->di.style |= BS_OWNERDRAW;
            break;

        case DIALOG_CONTROL_EDIT:
            pcd->diClass = DTIC_EDIT;
            pcd->di.style |= WS_BORDER;
            pcd->di.style |= ES_LEFT;
            assert(p_dialog_control_data.edit);
            if(p_dialog_control_data.edit->edit_xx.bits.multiline)
                pcd->di.style |= ES_MULTILINE | ES_AUTOVSCROLL;
            else
            {
                pcd->di.style |= ES_AUTOHSCROLL;
#define EDIT_EDIT_HEIGHT_HACK_WDU 0 /* bump it down a fraction to see if we can align with labels */
                /* it now often lines up perfectly on XP @96dpi, still fraction out on Vista @120 dpi */
                pcd->di.y  += EDIT_EDIT_HEIGHT_HACK_WDU;
                pcd->di.cy -= EDIT_EDIT_HEIGHT_HACK_WDU;
            }
            if(p_dialog_control_data.edit->edit_xx.bits.readonly)
                pcd->di.style |= ES_READONLY;

            if(pass == 2)
            trace_5(TRACE_APP_DIALOG, TEXT("EDIT id=%d, x=") U32_TFMT TEXT(", y=") U32_TFMT TEXT(", w=") U32_TFMT TEXT(", h=") U32_TFMT,
                pcd->di.id, (U32) pcd->di.x, (U32) pcd->di.y, (U32) pcd->di.cx, (U32) pcd->di.cy);
            break;

        case DIALOG_CONTROL_LIST_S32:
        case DIALOG_CONTROL_LIST_TEXT:
            pcd->diClass = DTIC_LISTBOX;
            pcd->di.style |= WS_BORDER | WS_VSCROLL;
            pcd->di.style |= LBS_NOINTEGRALHEIGHT | LBS_NOTIFY | LBS_HASSTRINGS /*| LBS_OWNERDRAWFIXED*/;
            if(p_dialog_control_data.list_text && p_dialog_control_data.list_text->list_xx.bits.force_v_scroll)
                pcd->di.style |= LBS_DISABLENOSCROLL;
            if(p_dialog_control_data.list_text && p_dialog_control_data.list_text->list_xx.bits.tab_position)
                pcd->di.style |= LBS_USETABSTOPS;

            if(pass == 2)
            trace_5(TRACE_APP_DIALOG, TEXT("LISTBOX id=%d, x=") U32_TFMT TEXT(", y=") U32_TFMT TEXT(", w=") U32_TFMT TEXT(", h=") U32_TFMT,
                pcd->di.id, (U32) pcd->di.x, (U32) pcd->di.y, (U32) pcd->di.cx, (U32) pcd->di.cy);
            break;

        case DIALOG_CONTROL_COMBO_S32:
        case DIALOG_CONTROL_COMBO_TEXT:
            pcd->diClass = DTIC_COMBOBOX;
            pcd->di.style |= WS_BORDER | WS_VSCROLL;
            pcd->di.style |= CBS_AUTOHSCROLL | CBS_HASSTRINGS /*| CBS_OWNERDRAWFIXED*/;
            if(p_dialog_control_data.combo_text && p_dialog_control_data.combo_text->combo_xx.list_xx.bits.force_v_scroll)
                pcd->di.style |= CBS_DISABLENOSCROLL;
            if(p_dialog_control_data.combo_text && p_dialog_control_data.combo_text->combo_xx.edit_xx.bits.readonly)
                pcd->di.style |= CBS_DROPDOWNLIST;
            else
                pcd->di.style |= CBS_DROPDOWN;

            if(pass == 2)
            trace_5(TRACE_APP_DIALOG, TEXT("COMBOBOX id=%d, x=") U32_TFMT TEXT(", y=") U32_TFMT TEXT(", w=") U32_TFMT TEXT(", h=") U32_TFMT,
                pcd->di.id, (U32) pcd->di.x, (U32) pcd->di.y, (U32) pcd->di.cx, (U32) pcd->di.cy);
            break;

        case DIALOG_CONTROL_BUMP_S32:
        case DIALOG_CONTROL_BUMP_F64:
            pcd->diClass = DTIC_EDIT;
            pcd->di.style |= WS_BORDER;
            pcd->di.style |= ES_LEFT | ES_AUTOHSCROLL;
#define BUMP_EDIT_HEIGHT_HACK_WDU EDIT_EDIT_HEIGHT_HACK_WDU /* bump it down a fraction to see if we can align with labels */
            /* it now lines up perfectly on XP @96dpi, still fraction out on Vista @120 dpi */
            pcd->di.y  += BUMP_EDIT_HEIGHT_HACK_WDU;
            pcd->di.cy -= BUMP_EDIT_HEIGHT_HACK_WDU;

            if(pass == 2)
            trace_5(TRACE_APP_DIALOG, TEXT("EDIT id=%d, x=") U32_TFMT TEXT(", y=") U32_TFMT TEXT(", w=") U32_TFMT TEXT(", h=") U32_TFMT TEXT(" (BUMP)"),
                pcd->di.id, (U32) pcd->di.x, (U32) pcd->di.y, (U32) pcd->di.cx, (U32) pcd->di.cy);

            pcd++;
            n_cw++;
            n_id++;

            p_dialog_ictl->data.bump_xx.windows.spin_wid = (p_dh->windows_control_id)++;
            pcd->diClassName = UPDOWN_CLASS;
            pcd->di.style |= UDS_AUTOBUDDY | UDS_ALIGNRIGHT | UDS_ARROWKEYS;
            pcd->di.id = (WORD) p_dialog_ictl->data.bump_xx.windows.spin_wid;
            pcd->di.x  = (short) (pcd->di.x + pcd->di.cx);
            pcd->di.y  = pcd[-1].di.y; /* match hacked edit control height */
            pcd->di.cy = pcd[-1].di.cy;

            if(pass == 2)
            trace_5(TRACE_APP_DIALOG, TEXT("UPDOWN id=%d, x=") U32_TFMT TEXT(", y=") U32_TFMT TEXT(", w=") U32_TFMT TEXT(", h=") U32_TFMT TEXT(" (BUMP)"),
                pcd->di.id, (U32) pcd->di.x, (U32) pcd->di.y, (U32) pcd->di.cx, (U32) pcd->di.cy);
            break;

        case DIALOG_CONTROL_USER:
            pcd->di.style |= BS_OWNERDRAW;

            if(pass == 2)
            trace_5(TRACE_APP_DIALOG, TEXT("USER id=%d, x=") U32_TFMT TEXT(", y=") U32_TFMT TEXT(", w=") U32_TFMT TEXT(", h=") U32_TFMT TEXT(" (BUTTON, OWNERDRAW)"),
                pcd->di.id, (U32) pcd->di.x, (U32) pcd->di.y, (U32) pcd->di.cx, (U32) pcd->di.cy);
            break;

        default: default_unhandled();
            n_cw = 0;
            n_id = 0;
            break;
        }

        if(pass == 1)
            p_dh->dlgtemplateex.cDlgItems = (WORD) (p_dh->dlgtemplateex.cDlgItems + n_cw);

        if(pass == 2)
        {
            WINDOWS_CTL_MAP windows_ctl_map;
            SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(windows_ctl_map), FALSE);
            for(j = 0; j < n_id; ++j)
            { /* for each real Windows control that will be created, make a mapping entry (except for OK & Cancel) */
                windows_ctl_map.dialog_control_id = p_dialog_ictl->dialog_control_id;
                windows_ctl_map.hwnd = NULL;
                status_break(status = al_array_add(&p_dh->p_dialog->windows.h_windows_ctl_map, WINDOWS_CTL_MAP, 1, &array_init_block, &windows_ctl_map));
            }
        }

        if(pass == 2)
            if(n_cw)
            {
                if(i == 0)
                    cd[0].di.style |= WS_GROUP;

                if(p_dialog_ictl->p_dialog_control->bits.tabstop)
                    cd[0].di.style |= WS_TABSTOP;
            }

        for(j = 0; j < n_cw; ++j)
        {   /* Ensure each DLGITEMTEMPLATEEX is DWORD aligned */
            if(pass == 2)
            {
                if((intptr_t) p_dh->p_byte & (sizeof32(DWORD)-1))
                {
                    const U32 excess = (U32) (intptr_t) p_dh->p_byte & (sizeof32(DWORD)-1);
                    b = sizeof32(DWORD) - excess;
                    p_dh->p_byte += b;
                }
            }
            else
            {
                if((DWORD) p_dh->size & (sizeof32(DWORD)-1))
                {
                    const U32 excess = (DWORD) p_dh->size & (sizeof32(DWORD)-1);
                    b = sizeof32(DWORD) - excess;
                    p_dh->size += b;
                }
            }

            /* DLGITEMTEMPLATEEX */
            b = sizeof32(cd[j].di);
            if(pass == 2)
                p_dh->p_byte = dialog_windows_memcpy32(p_dh->p_byte, &cd[j].di, b);
            else
                p_dh->size += b;

            /* Class ID/Name */
            tstr = cd[j].diClassName;
            if(NULL != tstr)
            {
                if(pass == 2)
                    p_dh->p_byte = dialog_windows_wstr_from_tstr(p_dh->p_byte, tstr);
                else
                    p_dh->size += dialog_windows_tstrlen32p1_bytes(tstr);
            }
            else
            {
                b = sizeof32(unicode_class_marker);
                if(pass == 2)
                    p_dh->p_byte = dialog_windows_memcpy32(p_dh->p_byte, &unicode_class_marker, b);
                else
                    p_dh->size += b;

                b = sizeof32(cd[j].diClass);
                if(pass == 2)
                    p_dh->p_byte = dialog_windows_memcpy32(p_dh->p_byte, &cd[j].diClass, b);
                else
                    p_dh->size += b;
            }

            /* Caption */
            tstr = ui_text_tstr(cd[j].diCaption);
            if(pass == 2)
                p_dh->p_byte = dialog_windows_wstr_from_tstr(p_dh->p_byte, tstr);
            else
                p_dh->size += dialog_windows_tstrlen32p1_bytes(tstr);

            /* Creation Data */
            b = sizeof32(no_creation_data);
            if(pass == 2)
                p_dh->p_byte = dialog_windows_memcpy32(p_dh->p_byte, &no_creation_data, b);
            else
                p_dh->size += b;
        }
        } /*block*/

        switch(p_dialog_ictl->dialog_control_type)
        {
        default:
            break;

        case DIALOG_CONTROL_GROUPBOX:
            status = dialog_windows_dlgtemplate_prepare_controls_in(p_dh, &p_dialog_ictl->data.groupbox.ictls, pass);
            break;
        }

        status_break(status);
    }

    return(status);
}

_Ret_writes_bytes_maybenull_(dwBytes)
static inline
void *
GlobalAlloc_GPTR_SAL(size_t dwBytes)
{
     return(GlobalAlloc(GPTR, dwBytes));
}

_Check_return_
static STATUS
dialog_windows_dlgtemplate_prepare(
    _InoutRef_  P_DIALOG p_dialog,
    _InRef_     PC_PIXIT_RECT p_pixit_rect,
    _InRef_opt_ PC_GDI_POINT p_gdi_tl)
{
    DLGTEMPLATE_HELPER dh;
    P_DLGTEMPLATE_HELPER p_dh = &dh;
    STATUS status = STATUS_OK; /* keep dataflower happy */
    SIZE PixelsPerInch;
    int pass = 1;
    PCTSTR tstr;
    U32 b;

    zero_struct(dh);

    p_dh->p_dialog = p_dialog;

    bastard_windows_h_dialog = p_dialog->h_dialog; /* Windows 3.1 inadequacy */

    p_dh->dlgtemplateex.dlgVer = 1;
    p_dh->dlgtemplateex.signature = 0xFFFF;
    p_dh->dlgtemplateex.helpID = 0;
    p_dh->dlgtemplateex.exStyle = 0;

    p_dh->dlgtemplateex.style = WS_VISIBLE;

    if(p_dialog->modeless)
        p_dh->dlgtemplateex.style |= (WS_POPUP | WS_SYSMENU | WS_CAPTION | WS_BORDER);
    else
        p_dh->dlgtemplateex.style |= (WS_POPUP | WS_SYSMENU | WS_CAPTION | DS_MODALFRAME);

    p_dh->dlgtemplateex.style |= DS_3DLOOK;
    /* NB this flag may be obsolete, but does affect the code below! */

    p_dh->dlgtemplateex.style |= DS_SETFONT /* let's have a font */;

    if(NULL != p_gdi_tl)
    {
        p_dh->dlgtemplateex.x = (short) p_gdi_tl->x / 2; /* NB still in WDU not pixels... */
        p_dh->dlgtemplateex.y = (short) p_gdi_tl->y / 2;

        p_dh->dlgtemplateex.style |= DS_ABSALIGN /* coordinates are screen-absolute */;
    }
    else
        p_dh->dlgtemplateex.style |= DS_CENTER;

    p_dialog->windows.wdu_offset_tl.x = p_pixit_rect->tl.x / PIXITS_PER_WDU_H; /* offset_tl stored in h/v dialog units */
    p_dialog->windows.wdu_offset_tl.y = p_pixit_rect->tl.y / PIXITS_PER_WDU_V;

    p_dh->dlgtemplateex.cx = (short) (pixit_rect_width(p_pixit_rect)  / PIXITS_PER_WDU_H);
    p_dh->dlgtemplateex.cy = (short) (pixit_rect_height(p_pixit_rect) / PIXITS_PER_WDU_V);

    p_dh->size = 0;

    trace_5(TRACE_APP_DIALOG, TEXT("DIALOGEX x=%d, y=%d%s, w=%d, h=%d"),
        p_dh->dlgtemplateex.x, p_dh->dlgtemplateex.y, (NULL != p_gdi_tl) ? TEXT(" (ABS x,y)") : tstr_empty_string,
        p_dh->dlgtemplateex.cx, p_dh->dlgtemplateex.cy);

    host_get_pixel_size(NULL /*screen*/, &PixelsPerInch); /* Get current pixel size for this dialog e.g. 96 or 120 */

    do  { /* reset each time round */
        p_dh->windows_control_id = WINDOWS_CTL_ID_STT;

        if(pass == 2)
        {
            if(NULL == (p_dialog->windows.dlgtemplate = GlobalAlloc_GPTR_SAL(p_dh->size)))
            {
                status = status_nomem();
                break;
            }

            p_dh->p_byte = (P_BYTE) /*GlobalLock*/(p_dialog->windows.dlgtemplate); /* GPTR -> GlobalLock just converts from HGLOBAL to pointer (same value) */

            p_dh->p_dlgtemplate = (struct DLGTEMPLATEEX *) p_dh->p_byte;
        }

        /* DLGTEMPLATEEX */
        b = sizeof32(p_dh->dlgtemplateex);
        if(pass == 2)
            p_dh->p_byte = dialog_windows_memcpy32(p_dh->p_byte, &p_dh->dlgtemplateex, b);
        else
            p_dh->size += b;

        /* Menu Name - always none */
        b = sizeof32(dialog_windows_empty_string);
        if(pass == 2)
            p_dh->p_byte = dialog_windows_memcpy32(p_dh->p_byte, &dialog_windows_empty_string, b);
        else
            p_dh->size += b;

        /* Class Name - always default */
        b = sizeof32(dialog_windows_empty_string);
        if(pass == 2)
            p_dh->p_byte = dialog_windows_memcpy32(p_dh->p_byte, &dialog_windows_empty_string, b);
        else
            p_dh->size += b;

        /* Caption */
        tstr = ui_text_tstr(&p_dialog->caption);
        if(pass == 2)
        {
            p_dh->p_byte = dialog_windows_wstr_from_tstr(p_dh->p_byte, tstr);
            trace_1(TRACE_APP_DIALOG, TEXT("CAPTION \"%s\""), report_tstr(tstr));
        }
        else
            p_dh->size += dialog_windows_tstrlen32p1_bytes(tstr);

        if(p_dh->dlgtemplateex.style & DS_SETFONT)
        {
            const WORD font_height = (WORD) MulDiv(-dialog_statics.windows.logfont.lfHeight, 72, PixelsPerInch.cy); /* NB Do the rounding, MulDiv() !!! */ /* DPI-aware */
            const WORD font_weight = (WORD) dialog_statics.windows.logfont.lfWeight;
            const BYTE font_italic = dialog_statics.windows.logfont.lfItalic;
            const BYTE font_charset = dialog_statics.windows.logfont.lfCharSet;

            b = sizeof32(font_height);
            if(pass == 2)
                p_dh->p_byte = dialog_windows_memcpy32(p_dh->p_byte, &font_height, b);
            else
                p_dh->size += b;

            b = sizeof32(font_weight);
            if(pass == 2)
                p_dh->p_byte = dialog_windows_memcpy32(p_dh->p_byte, &font_weight, b);
            else
                p_dh->size += b;

            b = sizeof32(font_italic);
            if(pass == 2)
                p_dh->p_byte = dialog_windows_memcpy32(p_dh->p_byte, &font_italic, b);
            else
                p_dh->size += b;

            b = sizeof32(font_charset);
            if(pass == 2)
                p_dh->p_byte = dialog_windows_memcpy32(p_dh->p_byte, &font_charset, b);
            else
                p_dh->size += b;

            tstr = dialog_statics.windows.logfont.lfFaceName;
            if(pass == 2)
            {
                p_dh->p_byte = dialog_windows_wstr_from_tstr(p_dh->p_byte, tstr);
                trace_2(TRACE_APP_DIALOG, TEXT("FONT %d, \"%s\""), font_height, report_tstr(tstr));
            }
            else
                p_dh->size += dialog_windows_tstrlen32p1_bytes(tstr);
        }

        status_break(status = dialog_windows_dlgtemplate_prepare_controls_in(p_dh, &p_dialog->ictls, pass));
    }
    while(++pass <= 2);

#if 0 /* Unnecessary with GPTR */
    if(NULL != p_dialog->windows.dlgtemplate)
        void_WrapOsBoolChecking(GlobalUnlock(p_dialog->windows.dlgtemplate));
#endif

    return(status);
}

extern void
dialog_windows_build_list(
    _InoutRef_  P_DIALOG p_dialog,
    P_DIALOG_ICTL p_dialog_ictl)
{
    HWND hwnd;
    BOOL combo_box;
    PC_UI_SOURCE p_ui_source;
    const void /*UI_CONTROL*/ * p_ui_control = NULL;
    UI_DATA_TYPE ui_data_type;

    switch(p_dialog_ictl->dialog_control_type)
    {
    default:
        return;

    case DIALOG_CONTROL_LIST_S32:
    case DIALOG_CONTROL_LIST_TEXT:
        if(!p_dialog_ictl->windows.wid) /* may be called to reencode prior to any id allocation e.g. spell dictionary dialog */
            return;
        hwnd  = GetDlgItem(p_dialog->hwnd, p_dialog_ictl->windows.wid);
        if(HOST_WND_NONE == hwnd)
            return;
        combo_box = FALSE;
        ListBox_ResetContent(hwnd);
        p_ui_source = p_dialog_ictl->data.list_xx.list_xx.p_ui_source;
        if(p_dialog_ictl->p_dialog_control_data.list_text && p_dialog_ictl->p_dialog_control_data.list_text->list_xx.bits.tab_position)
        {
            int tab = DIALOG_SYSCHARSL_H(p_dialog_ictl->p_dialog_control_data.list_text->list_xx.bits.tab_position) / PIXITS_PER_WDU_H;
            ListBox_SetTabStops(hwnd, 1, &tab);
        }
        break;

    case DIALOG_CONTROL_COMBO_S32:
    case DIALOG_CONTROL_COMBO_TEXT:
        if(!p_dialog_ictl->windows.wid)
            return;
        hwnd  = GetDlgItem(p_dialog->hwnd, p_dialog_ictl->windows.wid);
        if(HOST_WND_NONE == hwnd)
            return;
        combo_box = TRUE;
        ComboBox_ResetContent(hwnd);
        p_ui_source = p_dialog_ictl->data.combo_xx.list_xx.p_ui_source;
        break;
    }

    switch(p_dialog_ictl->dialog_control_type)
    {
    case DIALOG_CONTROL_LIST_S32:
        ui_data_type = UI_DATA_TYPE_S32;
        p_ui_control = p_dialog_ictl->data.list_xx.list_xx.p_ui_control_s32;
        break;

    case DIALOG_CONTROL_COMBO_S32:
        ui_data_type = UI_DATA_TYPE_S32;
        p_ui_control = p_dialog_ictl->data.combo_xx.list_xx.p_ui_control_s32;
        break;

    default:
    case DIALOG_CONTROL_LIST_TEXT:
    case DIALOG_CONTROL_COMBO_TEXT:
        ui_data_type = UI_DATA_TYPE_TEXT;
        break;
    }

    {
    S32 n_items = ui_data_n_items_query(ui_data_type, p_ui_source);
    S32 itemno;

    for(itemno = 0; itemno < n_items; ++itemno)
    {
        UI_TEXT ui_text;
        if(status_ok(ui_data_query_as_text(ui_data_type, p_ui_source, itemno, p_ui_control, &ui_text)))
        {
            PCTSTR tstr = ui_text_tstr(&ui_text);
            if(combo_box)
                ComboBox_AddString(hwnd, tstr);
            else
                ListBox_AddString(hwnd, tstr);
            ui_text_dispose(&ui_text);
        }
    }
    } /*block*/
}

extern void
dialog_windows_ictl_enable_here(
    _InoutRef_  P_DIALOG p_dialog,
    P_DIALOG_ICTL p_dialog_ictl)
{
    BOOL enabled = !p_dialog_ictl->bits.disabled;
    HWND hwnd;

    if(HOST_WND_NONE == p_dialog->hwnd)
        return;

    if(0 == p_dialog_ictl->windows.wid) /* may be logical group (etc) */
        return;

    hwnd = GetDlgItem(p_dialog->hwnd, p_dialog_ictl->windows.wid);

    switch(p_dialog_ictl->dialog_control_type)
    {
    case DIALOG_CONTROL_GROUPBOX:
        EnableWindow(hwnd, enabled);
        break;

    case DIALOG_CONTROL_STATICTEXT:
    case DIALOG_CONTROL_STATICFRAME:
        Static_Enable(hwnd, enabled);
        break;

    case DIALOG_CONTROL_PUSHBUTTON:
    case DIALOG_CONTROL_PUSHPICTURE:
    case DIALOG_CONTROL_RADIOBUTTON:
    case DIALOG_CONTROL_RADIOPICTURE:
    case DIALOG_CONTROL_CHECKBOX:
    case DIALOG_CONTROL_CHECKPICTURE:
    case DIALOG_CONTROL_TRISTATE:
    case DIALOG_CONTROL_TRIPICTURE:
    case DIALOG_CONTROL_USER:
        Button_Enable(hwnd, enabled);
        break;

    case DIALOG_CONTROL_EDIT:
        Edit_Enable(hwnd, enabled);
        break;

    case DIALOG_CONTROL_BUMP_S32:
    case DIALOG_CONTROL_BUMP_F64:
        Edit_Enable(hwnd, enabled);
        Button_Enable(GetDlgItem(p_dialog->hwnd, p_dialog_ictl->data.bump_xx.windows.spin_wid), enabled);
        break;

    case DIALOG_CONTROL_LIST_S32:
    case DIALOG_CONTROL_LIST_TEXT:
        ListBox_Enable(hwnd, enabled);
        break;

    case DIALOG_CONTROL_COMBO_S32:
    case DIALOG_CONTROL_COMBO_TEXT:
        ComboBox_Enable(hwnd, enabled);
        break;
    }
}

/******************************************************************************
*
* WM_COMMAND - hwnd is the handle of the dialog, id is the id of the control
*
******************************************************************************/

static void
dialog_onCommand_pushbutton_BN_CLICKED(
    _InoutRef_  P_DIALOG p_dialog,
    _InoutRef_  P_DIALOG_ICTL p_dialog_ictl)
{
    STATUS status = STATUS_OK;

    if(!p_dialog_ictl->bits.nobbled)
        status_assert(status = dialog_click_pushbutton(p_dialog, p_dialog_ictl, host_ctrl_pressed(), 0));

    if(status_fail(status))
    {   /* report error locally */
        reperr_null(status);
        status = STATUS_FAIL;
    }
}

/* edit control's contents have changed */

_Check_return_
static STATUS
dialog_onCommand_edit_EN_UPDATE(
    _InoutRef_  P_DIALOG p_dialog,
    _InoutRef_  P_DIALOG_ICTL p_dialog_ictl)
{
    STATUS status = STATUS_OK;
    P_DIALOG_ICTL_EDIT_XX p_dialog_ictl_edit_xx = p_dialog_ictl_edit_xx_from(p_dialog_ictl);
    PTR_ASSERT(p_dialog_ictl_edit_xx);

    /* the edit control's contents have changed, so read and validate it iff not caused by this bit of code*/
    if(NULL != p_dialog_ictl_edit_xx)
    {
        if(0 == p_dialog_ictl->bits.in_update)
        {
            HWND hwnd_edit = GetDlgItem(p_dialog->hwnd, p_dialog_ictl->windows.wid);
            DWORD caret = 0;
            DIALOG_CMD_CTL_STATE_SET dialog_cmd_ctl_state_set;
            UI_TEXT ui_text;

            msgclr(dialog_cmd_ctl_state_set);

            status_assert(ui_text_from_ictl_edit_xx(&ui_text, p_dialog, p_dialog_ictl));

            /* state change will want to rejig views iff chars rejected */
            p_dialog_ictl->bits.force_update = (ui_text_validate(&ui_text, p_dialog_ictl_edit_xx->p_bitmap_validation) != 0);

            if(p_dialog_ictl->bits.force_update)
            {
                caret = Edit_GetSel(hwnd_edit); /* EM_GETSEL */
                host_bleep();
            }

            dialog_cmd_ctl_state_set.state.edit.ui_text = ui_text;

            /* command ourselves with a state change, with interlock against killer recursion */
            dialog_cmd_ctl_state_set.h_dialog = p_dialog_ictl_edit_xx->h_dialog;
            dialog_cmd_ctl_state_set.dialog_control_id = p_dialog_ictl_edit_xx->dialog_control_id;
            dialog_cmd_ctl_state_set.bits = 0;

            p_dialog_ictl->bits.in_update += 1;
            status = object_call_DIALOG(DIALOG_CMD_CODE_CTL_STATE_SET, &dialog_cmd_ctl_state_set);
            p_dialog_ictl->bits.in_update -= 1;

            if(status_fail(status))
            {
                reperr_null(status);
                status = STATUS_OK;
            }

            if(p_dialog_ictl->bits.force_update)
                Edit_SetSel(hwnd_edit, HIWORD(caret), HIWORD(caret)); /* EM_SETSEL */

            p_dialog_ictl->bits.force_update = 0;

            ui_text_dispose(&ui_text);
        }
    }

    return(status);
}

_Check_return_
static STATUS
dialog_onCommand_edit(
    _InoutRef_  P_DIALOG p_dialog,
    _InoutRef_  P_DIALOG_ICTL p_dialog_ictl,
    _InVal_     UINT codeNotify)
{
    switch(codeNotify)
    {
    case EN_ERRSPACE:
        return(status_nomem());

    case EN_MAXTEXT:
        return(status_nomem());

    case EN_SETFOCUS:
        p_dialog->current_dialog_control_id = p_dialog_ictl->dialog_control_id;
        return(STATUS_OK);

    case EN_UPDATE:
        return(dialog_onCommand_edit_EN_UPDATE(p_dialog, p_dialog_ictl));

    default:
        return(STATUS_OK);
    }
}

/* edit control's contents have changed */

_Check_return_
static STATUS
dialog_onCommand_bump_xx_EN_UPDATE(
    _InoutRef_  P_DIALOG p_dialog,
    _InoutRef_  P_DIALOG_ICTL p_dialog_ictl)
{
    STATUS status = STATUS_OK;
    P_DIALOG_ICTL_EDIT_XX p_dialog_ictl_edit_xx = p_dialog_ictl_edit_xx_from(p_dialog_ictl);
    PTR_ASSERT(p_dialog_ictl_edit_xx);

    /* the edit control's contents have changed, so read and validate it iff not caused by this bit of code*/
    if(NULL != p_dialog_ictl_edit_xx)
    {
        if(0 == p_dialog_ictl->bits.in_update)
        {
            DIALOG_CMD_CTL_STATE_SET dialog_cmd_ctl_state_set;
            UI_TEXT ui_text;

            msgclr(dialog_cmd_ctl_state_set);

            status_assert(ui_text_from_ictl_edit_xx(&ui_text, p_dialog, p_dialog_ictl));

            /* state change will want to rejig views iff chars rejected */
            p_dialog_ictl->bits.force_update = (ui_text_validate(&ui_text, p_dialog_ictl_edit_xx->p_bitmap_validation) != 0);

            switch(p_dialog_ictl->dialog_control_type)
            {
            default: default_unhandled();
#if CHECKING
            case DIALOG_CONTROL_BUMP_F64:
#endif
                {
                PC_USTR i_ustr = ui_text_ustr(&ui_text);
                PC_USTR e_ustr;
                SS_RECOG_CONTEXT ss_recog_context;
                ss_recog_context_push(&ss_recog_context);
                errno = 0;
                dialog_cmd_ctl_state_set.state.bump_f64 = ui_strtod(i_ustr, &e_ustr);
                ss_recog_context_pull(&ss_recog_context);
                break;
                }

            case DIALOG_CONTROL_BUMP_S32:
                {
                PC_USTR i_ustr = ui_text_ustr(&ui_text);
                P_USTR e_ustr;
                errno = 0;
                dialog_cmd_ctl_state_set.state.bump_s32 = ui_strtol(i_ustr, &e_ustr, 0);
                if(errno)
                    dialog_cmd_ctl_state_set.state.bump_s32 = 0;
                break;
                }
            }

            /* command ourselves with a state change, with interlock against killer recursion */
            dialog_cmd_ctl_state_set.h_dialog = p_dialog_ictl_edit_xx->h_dialog;
            dialog_cmd_ctl_state_set.dialog_control_id = p_dialog_ictl_edit_xx->dialog_control_id;
            dialog_cmd_ctl_state_set.bits = 0;

            p_dialog_ictl->bits.in_update += 1;
            status = object_call_DIALOG(DIALOG_CMD_CODE_CTL_STATE_SET, &dialog_cmd_ctl_state_set);
            p_dialog_ictl->bits.in_update -= 1;

            if(status_fail(status))
            {   /* report error locally */
                reperr_null(status);
                status = STATUS_OK;
            }

            p_dialog_ictl->bits.force_update = 0;

            ui_text_dispose(&ui_text);
        }
    }

    return(status);
}

_Check_return_
static STATUS
dialog_onCommand_bump_xx(
    _InoutRef_  P_DIALOG p_dialog,
    _InoutRef_  P_DIALOG_ICTL p_dialog_ictl,
    _InVal_     UINT codeNotify)
{
    switch(codeNotify)
    {
    case EN_ERRSPACE:
        return(status_nomem());

    case EN_MAXTEXT:
        return(status_nomem());

    case EN_SETFOCUS:
        p_dialog->current_dialog_control_id = p_dialog_ictl->dialog_control_id;
        return(STATUS_OK);

    case EN_UPDATE:
        return(dialog_onCommand_bump_xx_EN_UPDATE(p_dialog, p_dialog_ictl));

    default:
        return(STATUS_OK);
    }
}

_Check_return_
static STATUS
dialog_onCommand_list_xx_LBN_DBLCLK(
    _InoutRef_  P_DIALOG p_dialog,
    _InoutRef_  P_DIALOG_ICTL p_dialog_ictl)
{
    if(!p_dialog_ictl->p_dialog_control_data.list_text || !p_dialog_ictl->p_dialog_control_data.list_text->list_xx.bits.disable_double)
    {   /* a double click means OK to the containing dialog */
        DIALOG_CMD_DEFPUSHBUTTON dialog_cmd_defpushbutton;
        dialog_cmd_defpushbutton.h_dialog = p_dialog->h_dialog;
        dialog_cmd_defpushbutton.double_dialog_control_id = p_dialog_ictl->dialog_control_id;
        status_assert(object_call_DIALOG(DIALOG_CMD_CODE_DEFPUSHBUTTON, &dialog_cmd_defpushbutton));
    }

    return(STATUS_DONE); /* get caller to exit early */
}

_Check_return_
static STATUS
dialog_onCommand_list_xx_LBN_SELCHANGE(
    _InoutRef_  P_DIALOG p_dialog,
    _InoutRef_  P_DIALOG_ICTL p_dialog_ictl,
    _InVal_     UINT codeNotify)
{
    /* reflect selection from list box into state, and for combos, the edit part of the control */
    DIALOG_CMD_CTL_STATE_SET dialog_cmd_ctl_state_set;
    S32 selected_item = DIALOG_CTL_STATE_LIST_ITEM_NONE;

    if(LBN_SELCHANGE == codeNotify)
    {
        HWND hwnd_list = GetDlgItem(p_dialog->hwnd, p_dialog_ictl->windows.wid);
        int sel = ListBox_GetCurSel(hwnd_list);

        if(sel != LB_ERR)
            selected_item = (S32) sel;
    }

    msgclr(dialog_cmd_ctl_state_set);
    dialog_cmd_ctl_state_set.h_dialog = p_dialog->h_dialog;
    dialog_cmd_ctl_state_set.dialog_control_id = p_dialog_ictl->dialog_control_id;
    dialog_cmd_ctl_state_set.bits = DIALOG_STATE_SET_ALTERNATE;

    dialog_cmd_ctl_state_set.state.list_text.itemno = selected_item;

    status_assert(object_call_DIALOG(DIALOG_CMD_CODE_CTL_STATE_SET, &dialog_cmd_ctl_state_set));

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_onCommand_list_xx(
    _InoutRef_  P_DIALOG p_dialog,
    _InoutRef_  P_DIALOG_ICTL p_dialog_ictl,
    _InVal_     UINT codeNotify)
{
    switch(codeNotify)
    {
    case LBN_ERRSPACE:
        return(status_nomem());

    case LBN_SETFOCUS:
        p_dialog->current_dialog_control_id = p_dialog_ictl->dialog_control_id;
        return(STATUS_OK);

    case LBN_DBLCLK:
        return(dialog_onCommand_list_xx_LBN_DBLCLK(p_dialog, p_dialog_ictl));

    case LBN_SELCANCEL:
    case LBN_SELCHANGE:
        return(dialog_onCommand_list_xx_LBN_SELCHANGE(p_dialog, p_dialog_ictl, codeNotify));

    default:
        return(STATUS_OK);
    }
}

_Check_return_
static STATUS
dialog_onCommand_combo_xx_CBN_DBLCLK(
    _InoutRef_  P_DIALOG p_dialog,
    _InoutRef_  P_DIALOG_ICTL p_dialog_ictl)
{
    if(!p_dialog_ictl->p_dialog_control_data.combo_text->combo_xx.list_xx.bits.disable_double)
    {   /* a double click means OK to the containing dialog */
        DIALOG_CMD_DEFPUSHBUTTON dialog_cmd_defpushbutton;
        dialog_cmd_defpushbutton.h_dialog = p_dialog->h_dialog;
        dialog_cmd_defpushbutton.double_dialog_control_id = p_dialog_ictl->dialog_control_id;
        status_assert(object_call_DIALOG(DIALOG_CMD_CODE_DEFPUSHBUTTON, &dialog_cmd_defpushbutton));
    }

    return(STATUS_DONE); /* get caller to exit early */
}

_Check_return_
static STATUS
dialog_onCommand_combo_xx_CBN_SELCHANGE(
    _InoutRef_  P_DIALOG p_dialog,
    _InoutRef_  P_DIALOG_ICTL p_dialog_ictl /*,
    _InVal_     UINT codeNotify*/)
{
    /* reflect selection from list box into state, and for combos, the edit part of the control */
    DIALOG_CMD_CTL_STATE_SET dialog_cmd_ctl_state_set;
    S32 selected_item = DIALOG_CTL_STATE_LIST_ITEM_NONE;

    {
    HWND hwnd_combo = GetDlgItem(p_dialog->hwnd, p_dialog_ictl->windows.wid);
    int sel = ComboBox_GetCurSel(hwnd_combo);

    if(sel != LB_ERR)
        selected_item = (S32) sel;
    } /*block*/

    msgclr(dialog_cmd_ctl_state_set);
    dialog_cmd_ctl_state_set.h_dialog = p_dialog->h_dialog;
    dialog_cmd_ctl_state_set.dialog_control_id = p_dialog_ictl->dialog_control_id;
    dialog_cmd_ctl_state_set.bits = DIALOG_STATE_SET_ALTERNATE;

    dialog_cmd_ctl_state_set.state.list_text.itemno = selected_item;

    status_assert(object_call_DIALOG(DIALOG_CMD_CODE_CTL_STATE_SET, &dialog_cmd_ctl_state_set));

    return(STATUS_OK);
}

/* control's contents have changed */

_Check_return_
static STATUS
dialog_onCommand_combo_xx_CBN_EDITUPDATE(
    _InoutRef_  P_DIALOG p_dialog,
    _InoutRef_  P_DIALOG_ICTL p_dialog_ictl)
{
    STATUS status = STATUS_OK;
    P_DIALOG_ICTL_EDIT_XX p_dialog_ictl_edit_xx = p_dialog_ictl_edit_xx_from(p_dialog_ictl);
    PTR_ASSERT(p_dialog_ictl_edit_xx);

    /* the edit control's contents have changed, so read and validate it iff not caused by this bit of code*/
    if(NULL != p_dialog_ictl_edit_xx)
    {
        if(0 == p_dialog_ictl->bits.in_update)
        {
            DIALOG_CMD_CTL_STATE_SET dialog_cmd_ctl_state_set;
            UI_TEXT ui_text;

            msgclr(dialog_cmd_ctl_state_set);

            status_assert(ui_text_from_ictl_edit_xx(&ui_text, p_dialog, p_dialog_ictl));

            /* state change will want to rejig views iff chars rejected */

            p_dialog_ictl->bits.force_update = (ui_text_validate(&ui_text, p_dialog_ictl_edit_xx->p_bitmap_validation) != 0);

            switch(p_dialog_ictl->dialog_control_type)
            {
            default: default_unhandled();
#if CHECKING
            case DIALOG_CONTROL_COMBO_TEXT:
#endif
                dialog_cmd_ctl_state_set.state.combo_text.ui_text = ui_text;
                break;
            }

            /* command ourselves with a state change, with interlock against killer recursion */
            dialog_cmd_ctl_state_set.h_dialog = p_dialog_ictl_edit_xx->h_dialog;
            dialog_cmd_ctl_state_set.dialog_control_id = p_dialog_ictl_edit_xx->dialog_control_id;
            dialog_cmd_ctl_state_set.bits = 0;

            p_dialog_ictl->bits.in_update += 1;
            status = object_call_DIALOG(DIALOG_CMD_CODE_CTL_STATE_SET, &dialog_cmd_ctl_state_set);
            p_dialog_ictl->bits.in_update -= 1;

            if(status_fail(status))
            {   /* report error locally */
                reperr_null(status);
                status = STATUS_OK;
            }

            p_dialog_ictl->bits.force_update = 0;

            ui_text_dispose(&ui_text);
        }
    }

    return(status);
}

_Check_return_
static STATUS
dialog_onCommand_combo_xx(
    _InoutRef_  P_DIALOG p_dialog,
    _InoutRef_  P_DIALOG_ICTL p_dialog_ictl,
    _InVal_     UINT codeNotify)
{
    switch(codeNotify)
    {
    case CBN_ERRSPACE:
        return(status_nomem());

    case CBN_SETFOCUS:
        p_dialog->current_dialog_control_id = p_dialog_ictl->dialog_control_id;
        return(STATUS_OK);

    case CBN_DBLCLK:
        return(dialog_onCommand_combo_xx_CBN_DBLCLK(p_dialog, p_dialog_ictl));

    case CBN_SELCHANGE:
    case CBN_SELENDOK:
        return(dialog_onCommand_combo_xx_CBN_SELCHANGE(p_dialog, p_dialog_ictl));

    case CBN_EDITUPDATE:
        return(dialog_onCommand_combo_xx_CBN_EDITUPDATE(p_dialog, p_dialog_ictl));

    default:
        return(STATUS_OK);
    }
}

_Check_return_
static STATUS
dialog_onCommand_user_BN_CLICKED(
    _InoutRef_  P_DIALOG p_dialog,
    _InoutRef_  P_DIALOG_ICTL p_dialog_ictl)
{
    STATUS status = STATUS_OK;
    P_PROC_DIALOG_EVENT p_proc_client;
    DIALOG_MSG_CTL_USER_MOUSE dialog_msg_ctl_user_mouse;
    msgclr(dialog_msg_ctl_user_mouse);

    /* always processed */

    if(NULL != (p_proc_client = dialog_find_handler(p_dialog, p_dialog_ictl->dialog_control_id, &dialog_msg_ctl_user_mouse.client_handle)))
    {
        DIALOG_MSG_CTL_HDR_from_dialog_ictl(dialog_msg_ctl_user_mouse, p_dialog, p_dialog_ictl);

        if(IsRButtonDown())
            dialog_msg_ctl_user_mouse.click = DIALOG_MSG_USER_MOUSE_CLICK_RIGHT_SINGLE;
        else
            dialog_msg_ctl_user_mouse.click = DIALOG_MSG_USER_MOUSE_CLICK_LEFT_SINGLE;

        status_assert(status = dialog_call_client(p_dialog, DIALOG_MSG_CODE_CTL_USER_MOUSE, &dialog_msg_ctl_user_mouse, p_proc_client));
    }

    return(status);
}

_Check_return_
static STATUS
dialog_onCommand_user(
    _InoutRef_  P_DIALOG p_dialog,
    _InoutRef_  P_DIALOG_ICTL p_dialog_ictl,
    _InVal_     UINT codeNotify)
{
    switch(codeNotify)
    {
    case BN_CLICKED:
        return(dialog_onCommand_user_BN_CLICKED(p_dialog, p_dialog_ictl));

    default:
        return(STATUS_OK);
    }
}

_Check_return_
static BOOL
dialog_onCommand(
    _HwndRef_   HWND hwnd,
    _In_        int id,
    _HwndRef_   HWND hwndCtl,
    _InVal_     UINT codeNotify)
{
    const H_DIALOG h_dialog = h_dialog_from_hwnd(hwnd);
    const P_DIALOG p_dialog = p_dialog_from_h_dialog(h_dialog);
    const UINT windows_control_id = id;
    const DIALOG_CONTROL_ID dialog_control_id = control_id_from_windows_control_id(p_dialog, windows_control_id);
    const P_DIALOG_ICTL p_dialog_ictl = p_dialog_ictl_from_control_id(p_dialog, dialog_control_id);
    STATUS status = STATUS_OK;

    UNREFERENCED_PARAMETER_HwndRef_(hwndCtl);

    if(NULL != p_dialog_ictl) /* some of them don't have Cancel buttons, for instance */
    switch(p_dialog_ictl->dialog_control_type)
    {
    default:
        break;

    case DIALOG_CONTROL_PUSHBUTTON:
    case DIALOG_CONTROL_PUSHPICTURE:
        if(BN_CLICKED == codeNotify)
        {
            dialog_onCommand_pushbutton_BN_CLICKED(p_dialog, p_dialog_ictl);

            return(TRUE); /* handle IDOK and IDCANCEL button clicks as normal */
        }
        break;

    case DIALOG_CONTROL_RADIOBUTTON:
    case DIALOG_CONTROL_RADIOPICTURE:
        if(BN_CLICKED == codeNotify)
            if(!p_dialog_ictl->bits.nobbled)
                status_assert(status = dialog_click_radiobutton(p_dialog, p_dialog_ictl));
        break;

    case DIALOG_CONTROL_CHECKBOX:
    case DIALOG_CONTROL_CHECKPICTURE:
        if(BN_CLICKED == codeNotify)
            if(!p_dialog_ictl->bits.nobbled)
                status_assert(status = dialog_click_checkbox(p_dialog, p_dialog_ictl));
        break;

#ifdef DIALOG_HAS_TRISTATE
    case DIALOG_CONTROL_TRISTATE:
    case DIALOG_CONTROL_TRIPICTURE:
        if(BN_CLICKED == codeNotify)
            if(!p_dialog_ictl->bits.nobbled)
                status_assert(status = dialog_click_tristate(p_dialog, p_dialog_ictl));
        break;
#endif

    case DIALOG_CONTROL_EDIT:
        status = dialog_onCommand_edit(p_dialog, p_dialog_ictl, codeNotify);
        break;

    case DIALOG_CONTROL_BUMP_S32:
    case DIALOG_CONTROL_BUMP_F64:
        if(windows_control_id == p_dialog_ictl->windows.wid)
            status = dialog_onCommand_bump_xx(p_dialog, p_dialog_ictl, codeNotify);
        break;

    case DIALOG_CONTROL_LIST_S32:
    case DIALOG_CONTROL_LIST_TEXT:
        if(status_done(status = dialog_onCommand_list_xx(p_dialog, p_dialog_ictl, codeNotify)))
            return(TRUE);
        break;

    case DIALOG_CONTROL_COMBO_S32:
    case DIALOG_CONTROL_COMBO_TEXT:
        if(status_done(status = dialog_onCommand_combo_xx(p_dialog, p_dialog_ictl, codeNotify)))
            return(TRUE);
        break;

    case DIALOG_CONTROL_USER:
        status = dialog_onCommand_user(p_dialog, p_dialog_ictl, codeNotify);
        break;
    }

    if((windows_control_id == IDOK) || (windows_control_id == IDCANCEL))
    {
        DIALOG_CMD_COMPLETE_DBOX dialog_cmd_complete_dbox;
        msgclr(dialog_cmd_complete_dbox);
        dialog_cmd_complete_dbox.h_dialog = p_dialog->h_dialog;
        dialog_cmd_complete_dbox.completion_code = (windows_control_id == IDOK) ? DIALOG_COMPLETION_OK : DIALOG_COMPLETION_CANCEL;
        status_assert(object_call_DIALOG(DIALOG_CMD_CODE_COMPLETE_DBOX, &dialog_cmd_complete_dbox));
        return(TRUE);
    }

    if(status_fail(status))
    {   /* report error locally */
        reperr_null(status);
        status = STATUS_FAIL;
    }

    return(FALSE); /* unprocessed */
}

/******************************************************************************
*
* WM_DESTROY - hwnd is the handle of the dialog
*
******************************************************************************/

static void
dialog_onDestroy(
    _HwndRef_   HWND hwnd)
{
    const H_DIALOG h_dialog = h_dialog_from_hwnd(hwnd);

    if(0 == h_dialog)
    {   /* Does happen from time to time - I think iff DialogBoxIndirectParam() failed */
        assert0();
        return;
    }

    {
    const P_DIALOG p_dialog = p_dialog_from_h_dialog(h_dialog);

    if(p_dialog->windows.hTheme_Button)
    {
        CloseThemeData(p_dialog->windows.hTheme_Button);
        p_dialog->windows.hTheme_Button = NULL;
    }

    if(p_dialog->completion_code == 0)
        p_dialog->completion_code = DIALOG_COMPLETION_CANCEL;

    p_dialog->hwnd = 0;
    } /*block*/
}

/******************************************************************************
*
* WM_SETFONT - hwnd is the handle of the dialog, HFONT is the font handle
*
******************************************************************************/

static void
dialog_onSetFont(
    _HwndRef_   HWND hwnd,
    _In_        HFONT hfont,
    _InVal_     BOOL fRedraw)
{
    UNREFERENCED_PARAMETER_HwndRef_(hwnd);
    UNREFERENCED_PARAMETER(hfont);
    UNREFERENCED_PARAMETER_InVal_(fRedraw);
}

/******************************************************************************
*
* WM_DRAWITEM - hwnd is the handle of the dialog, pDrawItem->CtlID is the control id
*
******************************************************************************/

static BOOL
themed_dialog_UIToolButtonDraw(
    HTHEME hTheme,
    HDC hDC, PCRECT pRect,
    HBITMAP hBmp, int bmx, int bmy, int iImageIndex, UINT uStateIn)
{
    const int iPartId = BP_PUSHBUTTON;
    int iStateId;
    HIMAGELIST hImageList;
    HBITMAP hBitmapCopy;

    if(uStateIn & BUTTONGROUP_DISABLED)
    {
        iStateId = PBS_DISABLED;
    }
    else if(uStateIn & BUTTONGROUP_MOUSEDOWN)
    {
        iStateId = PBS_PRESSED;
    }
    else if(uStateIn & BUTTONGROUP_MOUSEOVER)
    {
        iStateId = PBS_HOT;
    }
    else
    {
        if(uStateIn & BUTTONGROUP_DOWN)
            iStateId = PBS_PRESSED;
        else
            iStateId = PBS_NORMAL;
    }

    if(NULL == (hImageList = ImageList_Create(bmx, bmy, ILC_COLOR | ILC_MASK, 20, 20)))
        return(FALSE);

    if(NULL == (hBitmapCopy = CopyImage(hBmp, IMAGE_BITMAP, 0, 0, 0)))
    {
        ImageList_Destroy(hImageList);
        return(FALSE);
    }

    ImageList_SetBkColor(hImageList, CLR_NONE);

    /* NB ImageList_AddMasked trashes hBitmapCopy */
    if(ImageList_AddMasked(hImageList, hBitmapCopy, RGB(192, 192, 192)) < 0) /* LTGRAY is the colour key */
    {
        DeleteBitmap(hBitmapCopy);
        ImageList_Destroy(hImageList);
        return(FALSE);
    }

    DeleteBitmap(hBitmapCopy);

    {
    RECT icon_rect;
    LONG content_cx, content_cy;
    DrawThemeBackground(hTheme, hDC, iPartId, iStateId, pRect, NULL);
    GetThemeBackgroundContentRect(hTheme, hDC, iPartId, iStateId, pRect, &icon_rect);
    /* centre icon in its slot */
    content_cx = icon_rect.right - icon_rect.left;
    content_cy = icon_rect.bottom - icon_rect.top;
    if(bmx < content_cx)
        icon_rect.left += (content_cx - bmx) / 2;
    if(bmy < content_cy)
        icon_rect.top  += (content_cy - bmy) / 2;
    icon_rect.right  = icon_rect.left + bmx;
    icon_rect.bottom = icon_rect.top  + bmy;
    if(uStateIn & BUTTONGROUP_DISABLED)
    {
        HICON hIcon = ImageList_GetIcon(hImageList, iImageIndex, ILD_TRANSPARENT);
        if(NULL != hIcon)
        {
            DrawState(hDC, GetStockBrush(WHITE_BRUSH), NULL, (LPARAM) hIcon, 0, icon_rect.left, icon_rect.top, bmx, bmy, DSS_DISABLED | DST_ICON);
            DestroyIcon(hIcon);
        }
    }
    else
    {
        DrawThemeIcon(hTheme, hDC, iPartId, iStateId, &icon_rect, hImageList, iImageIndex);
        //ImageList_Draw(hImageList, iImageIndex, hDC, icon_rect.left, icon_rect.top, ILD_TRANSPARENT);
    }
    } /*block*/

    ImageList_Destroy(hImageList);

    return(TRUE);
}

static BOOL
dialog_UIToolButtonDrawTDD(
    HTHEME hTheme,
    HDC hDC, PCRECT pRect,
    HBITMAP hBmp, int bmx, int bmy, int iImageIndex, UINT uStateIn,
    LPTOOLDISPLAYDATA pTDD)
{
    UINT uState;

    if(NULL != hTheme)
        if(themed_dialog_UIToolButtonDraw(hTheme, hDC, pRect, hBmp, bmx, bmy, iImageIndex, uStateIn))
            return(TRUE);

    /* fallback is old implementation */
    uState = uStateIn;
    uState |= (PRESERVE_WHITE); /* | PRESERVE_DKGRAY | PRESERVE_BLACK); need these to map for high-contrast */ /* LTGRAY is colour key */
    uState |= BUTTONGROUP_DIALOGUE_BOX_BUTTON;
    return(UIToolButtonDrawTDD(hDC, pRect->left, pRect->top, pRect->right - pRect->left, pRect->bottom - pRect->top,
                               hBmp, bmx, bmy, iImageIndex, uState, pTDD));
}

/* dialog equivalent of host_redraw_context_set_host_xform() */

static void
dialog_host_redraw_context_set_host_xform(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InoutRef_  P_HOST_XFORM p_host_xform /*updated*/)
{
    p_host_xform->windows.pixels_per_inch.x = GetDeviceCaps(p_redraw_context->windows.paintstruct.hdc, LOGPIXELSX);
    p_host_xform->windows.pixels_per_inch.y = GetDeviceCaps(p_redraw_context->windows.paintstruct.hdc, LOGPIXELSY);

    p_host_xform->windows.d.x = muldiv64(p_host_xform->windows.pixels_per_inch.x, INCHES_PER_METRE_MUL, INCHES_PER_METRE_DIV);
    p_host_xform->windows.d.y = muldiv64(p_host_xform->windows.pixels_per_inch.y, INCHES_PER_METRE_MUL, INCHES_PER_METRE_DIV);

    p_host_xform->windows.multiplier_of_pixels.x = PIXITS_PER_METRE * p_host_xform->scale.b.x;
    p_host_xform->windows.multiplier_of_pixels.y = PIXITS_PER_METRE * p_host_xform->scale.b.y;

    p_host_xform->windows.divisor_of_pixels.x = p_host_xform->windows.d.x * p_host_xform->scale.t.x;
    p_host_xform->windows.divisor_of_pixels.y = p_host_xform->windows.d.y * p_host_xform->scale.t.y;
}

static void
dialog_onDrawItem(
    _HwndRef_   HWND hwnd,
    _In_        const DRAWITEMSTRUCT * const pDrawItem)
{
    const H_DIALOG h_dialog = h_dialog_from_hwnd(hwnd);
    const P_DIALOG p_dialog = p_dialog_from_h_dialog(h_dialog);
    const UINT windows_control_id = pDrawItem->CtlID;
    DIALOG_CONTROL_ID dialog_control_id;
    P_DIALOG_ICTL p_dialog_ictl;
    RESOURCE_BITMAP_ID resource_bitmap_id;
    BOOL mouse_down = FALSE;
    BOOL mouse_over = FALSE;
    UINT uState = 0;
    BOOL plot = TRUE;
    HPALETTE old_hpalette;

    if(pDrawItem->CtlType != ODT_BUTTON)
        return;

    dialog_control_id = control_id_from_windows_control_id(p_dialog, windows_control_id);
    p_dialog_ictl = p_dialog_ictl_from_control_id(p_dialog, dialog_control_id);

    host_select_default_palette(pDrawItem->hDC, &old_hpalette);

    if(pDrawItem->itemAction == ODA_SELECT)
    {
        mouse_down = (pDrawItem->itemState & ODS_SELECTED) != 0;
    }
    else
    {
        if(p_dialog_ictl->bits.mouse_down)
            mouse_down = TRUE;
        else if(p_dialog_ictl->bits.mouse_over)
            mouse_over = TRUE;
    }

    if(p_dialog_ictl->bits.disabled || p_dialog_ictl->bits.nobbled)
    {
        mouse_down = FALSE;
        mouse_over = FALSE;
    }

    switch(p_dialog_ictl->dialog_control_type)
    {
    case DIALOG_CONTROL_PUSHPICTURE:
        resource_bitmap_id = p_dialog_ictl->p_dialog_control_data.pushpicture->picture_bitmap_id;
        uState = COMMANDBUTTON_NORMAL;
        if(p_dialog_ictl->bits.disabled)
            uState |= BUTTONGROUP_DISABLED;
        else if(mouse_down)
            uState |= BUTTONGROUP_MOUSEDOWN;
        else if(mouse_over)
            uState |= BUTTONGROUP_MOUSEOVER;
        break;

    case DIALOG_CONTROL_RADIOPICTURE:
        {
        PC_RESOURCE_BITMAP_ID p_resource_bitmap_id = p_dialog_ictl->p_dialog_control_data.radiopicture->p_bitmap_id_offon;
        if(p_dialog_ictl->bits.radiobutton_active && p_dialog_ictl->p_dialog_control_data.radiopicture->bits.has_n_bmp)
            p_resource_bitmap_id++;
        resource_bitmap_id = *p_resource_bitmap_id;
        uState = p_dialog_ictl->bits.radiobutton_active ? ATTRIBUTEBUTTON_ON : ATTRIBUTEBUTTON_OFF;
        if(p_dialog_ictl->bits.disabled)
            uState |= BUTTONGROUP_DISABLED;
        else if(mouse_down)
            uState |= BUTTONGROUP_MOUSEDOWN;
        else if(mouse_over)
            uState |= BUTTONGROUP_MOUSEOVER;
        break;
        }

    case DIALOG_CONTROL_CHECKPICTURE:
        {
        PC_RESOURCE_BITMAP_ID p_resource_bitmap_id = p_dialog_ictl->p_dialog_control_data.checkpicture->p_bitmap_id_offon;
        if((p_dialog_ictl->state.checkbox > 0) && p_dialog_ictl->p_dialog_control_data.checkpicture->bits.has_n_bmp)
            p_resource_bitmap_id++;
        resource_bitmap_id = *p_resource_bitmap_id;
        uState = (p_dialog_ictl->state.checkbox > 0) ? ATTRIBUTEBUTTON_ON : ATTRIBUTEBUTTON_OFF;
        if(p_dialog_ictl->bits.disabled)
            uState |= BUTTONGROUP_DISABLED;
        else if(mouse_down)
            uState |= BUTTONGROUP_MOUSEDOWN;
        else if(mouse_over)
            uState |= BUTTONGROUP_MOUSEOVER;
        break;
        }

#ifdef DIALOG_HAS_TRISTATE
    case DIALOG_CONTROL_TRIPICTURE:
        {
        PC_RESOURCE_BITMAP_ID p_resource_bitmap_id = p_dialog_ictl->p_dialog_control_data.tripicture->p_bitmap_id_offondontcare;
        if((p_dialog_ictl->state.checkbox > 0) && p_dialog_ictl->p_dialog_control_data.tripicture->bits.has_n_bmp)
            p_resource_bitmap_id++;
        if((p_dialog_ictl->state.checkbox > 1) && p_dialog_ictl->p_dialog_control_data.tripicture->bits.has_n_bmp)
            p_resource_bitmap_id++;
        resource_bitmap_id = *p_resource_bitmap_id;
        uState = (p_dialog_ictl->state.tristate == INDETERMINATE) ? ATTRIBUTEBUTTON_INDETERMINATE : ((p_dialog_ictl->state.tristate > 0) ? ATTRIBUTEBUTTON_ON : ATTRIBUTEBUTTON_OFF));
        if(p_dialog_ictl->bits.disabled)
            uState |= BUTTONGROUP_DISABLED;
        else if(mouse_down)
            uState |= BUTTONGROUP_MOUSEDOWN
        else if(mouse_over)
            uState |= BUTTONGROUP_MOUSEOVER;
        break;
        }
#endif

    default:
#if CHECKING
        DebugBreak();
    case DIALOG_CONTROL_USER:
#endif
        {
        PC_RGB p_rgb = p_dialog_ictl->p_dialog_control_data.user->p_back_colour;
        RGB rgb;

        if(NULL != p_rgb)
            rgb = *p_rgb;
        else
        {
            if(p_dialog_ictl->p_dialog_control_data.user->bits.state_is_rgb)
                rgb = p_dialog_ictl->state.user.rgb;
            else if(p_dialog_ictl->p_dialog_control_data.user->bits.state_is_rgb_stash_index)
            {
                host_rgb_stash_colour(pDrawItem->hDC, (UINT) p_dialog_ictl->state.user.u32); /* keep it up-to-date */
                rgb = rgb_stash[p_dialog_ictl->state.user.u32];
            }
            else
                rgb_set(&rgb, 0xFF, 0xFF, 0xFF);
        }

        if(p_dialog_ictl->bits.disabled)
            convert_colour_using_tint(&rgb, 30); /* lighten up a bit */

        {
        HBRUSH hbrush = CreateSolidBrush(PALETTERGB(rgb.r, rgb.g, rgb.b));
        if(NULL != hbrush)
        {
            FillRect(pDrawItem->hDC, &pDrawItem->rcItem, hbrush);
            DeleteBrush(hbrush);
        }
        } /*block*/
        FrameRect(pDrawItem->hDC, &pDrawItem->rcItem, GetStockBrush(BLACK_BRUSH));

        {
        P_PROC_DIALOG_EVENT p_proc_client;
        DIALOG_MSG_CTL_USER_REDRAW dialog_msg_ctl_user_redraw;
        msgclr(dialog_msg_ctl_user_redraw);

        if(NULL != (p_proc_client = dialog_find_handler(p_dialog, p_dialog_ictl->dialog_control_id, &dialog_msg_ctl_user_redraw.client_handle)))
        {
            REDRAW_CONTEXT_CACHE redraw_context_cache = { NULL };
            const P_REDRAW_CONTEXT p_redraw_context = &dialog_msg_ctl_user_redraw.redraw_context;

            zero_struct_ptr(p_redraw_context);
            p_redraw_context->p_redraw_context_cache = &redraw_context_cache;

            p_redraw_context->windows.paintstruct.hdc = pDrawItem->hDC;
            p_redraw_context->windows.paintstruct.fErase = FALSE;
            p_redraw_context->windows.paintstruct.rcPaint = pDrawItem->rcItem;
            /* rest of paintstruct is Windows' internal garbage */

            p_redraw_context->host_xform.scale.t.x = 1;
            p_redraw_context->host_xform.scale.t.y = 1;
            p_redraw_context->host_xform.scale.b.x = 1;
            p_redraw_context->host_xform.scale.b.y = 1;

            p_redraw_context->display_mode = DISPLAY_PRINT_AREA;

            if(DOCNO_NONE != p_dialog->docno)
            {
                p_redraw_context->border_width.x = p_redraw_context->border_width.y = p_docu_from_docno(p_dialog->docno)->page_def.grid_size;
                p_redraw_context->border_width_2.x = p_redraw_context->border_width_2.y = 2 * p_redraw_context->border_width.x;
            }

            dialog_host_redraw_context_set_host_xform(p_redraw_context, &p_redraw_context->host_xform);

            host_redraw_context_fillin(p_redraw_context);

            /* no origin worries */

            host_paint_start(p_redraw_context);

            p_redraw_context->windows.host_machine_clip_rect = p_redraw_context->windows.paintstruct.rcPaint;

            DIALOG_MSG_CTL_HDR_from_dialog_ictl(dialog_msg_ctl_user_redraw, p_dialog, p_dialog_ictl);

            { /* Convert the redraw rectangle to a suitable view rectangle */
            GDI_RECT gdi_rect;
            gdi_rect.tl.x = p_redraw_context->windows.paintstruct.rcPaint.left   + p_redraw_context->gdi_org.x;
            gdi_rect.tl.y = p_redraw_context->windows.paintstruct.rcPaint.top    + p_redraw_context->gdi_org.y;
            gdi_rect.br.x = p_redraw_context->windows.paintstruct.rcPaint.right  + p_redraw_context->gdi_org.x;
            gdi_rect.br.y = p_redraw_context->windows.paintstruct.rcPaint.bottom + p_redraw_context->gdi_org.y;
            pixit_rect_from_window_rect(&dialog_msg_ctl_user_redraw.control_inner_box, &gdi_rect, &p_redraw_context->host_xform);
            } /*block*/

            dialog_msg_ctl_user_redraw.enabled = !p_dialog_ictl->bits.disabled;

            status_assert(dialog_call_client(p_dialog, DIALOG_MSG_CODE_CTL_USER_REDRAW, &dialog_msg_ctl_user_redraw, p_proc_client));

            host_paint_end(p_redraw_context);
        }
        } /*block*/

        plot = 0;
        break;
        }
    }

    if(plot)
    {
        RESOURCE_BITMAP_HANDLE resource_bitmap_handle;
        GDI_SIZE bm_grid_size;
        UINT index;
        BOOL bitmap_found = resource_bitmap_find_new(&resource_bitmap_id, &resource_bitmap_handle, &bm_grid_size, &index);

        if(!bitmap_found)
        {
            uState = BUTTONGROUP_BLANK;
            if(p_dialog_ictl->bits.disabled)
                uState |= BUTTONGROUP_DISABLED;
            else if(mouse_down)
                uState |= BUTTONGROUP_MOUSEDOWN;
            else if(mouse_over)
                uState |= BUTTONGROUP_MOUSEOVER;
        }

#if 0
        switch(pDrawItem->itemAction)
        {
        case ODA_DRAWENTIRE: /* Redraw the entire control */
            /* therefore bttncur controls need the outer frame nobbling 'cos of corners */
            FrameRect(pDrawItem->hDC, &pDrawItem->rcItem, GetStockBrush(LTGRAY_BRUSH));
        default:
        case ODA_SELECT: /* Redraw to reflect current selection */
        case ODA_FOCUS: /* Redraw to reflect current focus */
            break;
        }
#endif

        consume_bool(dialog_UIToolButtonDrawTDD(p_dialog->windows.hTheme_Button, pDrawItem->hDC, &pDrawItem->rcItem, resource_bitmap_handle.i, bm_grid_size.cx, bm_grid_size.cy, index, uState, &tdd));

        if(resource_bitmap_handle.i)
            resource_bitmap_lose(&resource_bitmap_handle);
    }

#if 0
    /* Draw a focus rectangle if the button has the focus */
    if(pDrawItem->itemState & ODS_FOCUS)
    {
        RECT focus_rect = pDrawItem->rcItem;
        focus_rect.left   += size.cx / 8;
        focus_rect.top    += size.cy / 8;
        focus_rect.right  -= size.cx / 8;
        focus_rect.bottom -= size.cy / 8;
        DrawFocusRect(pDrawItem->hDC, &focus_rect);
    }
#endif

    host_select_old_palette(pDrawItem->hDC, &old_hpalette);
}

/******************************************************************************
*
* WM_MEASUREITEM - hwnd is the handle of the control, pMeasureItem->CtlID is the id of the control
*
******************************************************************************/

static void
dialog_onMeasureItem(
    _HwndRef_   HWND hwnd,
    _In_        MEASUREITEMSTRUCT * const pMeasureItem)
{
    /*const H_DIALOG h_dialog = bastard_windows_h_dialog;*/ /* Windows 3.1 inadequacy */
    /*const P_DIALOG p_dialog = p_dialog_from_h_dialog(h_dialog);*/
    /*const UINT windows_control_id = (UINT) wParam;*/
    /*PMEASUREITEMSTRUCT pMeasureItemStruct = (PMEASUREITEMSTRUCT) lParam;*/
    /*const UINT windows_control_id = pMeasureItem->CtlID;*/ /* or the other id??? */
    /*DIALOG_CONTROL_ID dialog_control_id = control_id_from_windows_control_id(p_dialog, windows_control_id);*/
    /*const P_DIALOG_ICTL p_dialog_ictl = p_dialog_ictl_from_control_id(p_dialog, dialog_control_id);*/
    SIZE gdi_size = { 16, 16 }; /* Keep dataflower happy */

    UNREFERENCED_PARAMETER_HwndRef_(hwnd);

    pMeasureItem->itemWidth = gdi_size.cx;
    pMeasureItem->itemHeight = gdi_size.cy;
}

_Check_return_
_Ret_maybenull_
static P_DIALOG_ICTL
dialog_ictls_find_windows_control_id_in(
    _InoutRef_  P_DIALOG p_dialog,
    P_DIALOG_ICTL_GROUP p_ictl_group,
    _InVal_     UINT windows_control_id)
{
    ARRAY_INDEX i;

    for(i = 0; i < n_ictls_from_group(p_ictl_group); ++i)
    {
        P_DIALOG_ICTL p_dialog_ictl = p_dialog_ictl_from(p_ictl_group, i);

        if(windows_control_id == p_dialog_ictl->windows.wid)
            return(p_dialog_ictl);

        switch(p_dialog_ictl->dialog_control_type)
        {
        default:
            break;

        case DIALOG_CONTROL_GROUPBOX:
            {
            P_DIALOG_ICTL p_dialog_ictl_recurse = dialog_ictls_find_windows_control_id_in(p_dialog, &p_dialog_ictl->data.groupbox.ictls, windows_control_id);
            if(NULL != p_dialog_ictl_recurse)
                return(p_dialog_ictl_recurse);
            break;
            }

        case DIALOG_CONTROL_BUMP_S32:
        case DIALOG_CONTROL_BUMP_F64:
            {
            if(windows_control_id == p_dialog_ictl->data.bump_xx.windows.spin_wid)
                return(p_dialog_ictl);
            break;
        }
        }
    }

    return(NULL);
}

static void
dialog_onUdnDeltaPos(
    _HwndRef_   HWND hwnd,
    _In_        NMUPDOWN * pNmUpDown)
{
    const H_DIALOG h_dialog = h_dialog_from_hwnd(hwnd);
    const P_DIALOG p_dialog = p_dialog_from_h_dialog(h_dialog);
    const UINT windows_control_id = (UINT) pNmUpDown->hdr.idFrom;

    /* Have to scan for the up-down control that matches */
    const P_DIALOG_ICTL p_dialog_ictl = dialog_ictls_find_windows_control_id_in(p_dialog, &p_dialog->ictls, windows_control_id);

    if(NULL != p_dialog_ictl)
    {
        switch(p_dialog_ictl->dialog_control_type)
        {
        default: /*default_unhandled();*/
            break;

        case DIALOG_CONTROL_BUMP_S32:
        case DIALOG_CONTROL_BUMP_F64:
            {
          /*const BOOL adjust_clicked = host_ctrl_pressed();*/
            const BOOL dec_button = (pNmUpDown->iDelta < 0);
            status_assert(dialog_click_bump_xx(p_dialog, p_dialog_ictl, (!dec_button) /* ^ adjust_clicked */)); /* can't pass error back */
            break;
            }
        }
    }
}

/******************************************************************************
*
* WM_NOTIFY - hwnd is the handle of the dialog, pNmHdr->idFrom is the id of the control
*
******************************************************************************/

static LRESULT
dialog_onNotify(
    _HwndRef_   HWND hwnd,
    _In_        int wParam,
    _In_reads_bytes_c_(sizeof32(NMUPDOWN)) NMHDR * pNmHdr)
{
    UNREFERENCED_PARAMETER(wParam); /* The identifier of the common control sending the message */

    switch(pNmHdr->code)
    {
    case UDN_DELTAPOS:
        dialog_onUdnDeltaPos(hwnd, /*reinterpret_cast*/ (NMUPDOWN *) (pNmHdr));
        break;
    }

    return(0L);
}

/******************************************************************************
*
* WM_THEMECHANGED
*
******************************************************************************/

static void
dialog_onThemeChanged(
    _HwndRef_   HWND hwnd)
{
    const H_DIALOG h_dialog = h_dialog_from_hwnd(hwnd);
    const P_DIALOG p_dialog = p_dialog_from_h_dialog(h_dialog);

    if(NULL != p_dialog->windows.hTheme_Button)
        CloseThemeData(p_dialog->windows.hTheme_Button);

    p_dialog->windows.hTheme_Button = OpenThemeData(hwnd, VSCLASS_BUTTON);
}

/* ****** Context-Sensitive Help in a Dialog Box Through F1 ****** */

/*
Normally, the child control windows in a dialog box do not pass keystroke messages on to the dialog box window.
To detect F1 keystrokes in a dialog box, it is necessary to install a message filter:
*/

/*
--------------------------    FilterFunc -------------------------

   PARAMETERS:

      nCode : Specifies the type of message being processed. It will
              be either MSGF_DIALOGBOX, MSGF_MENU, or less than 0.

      wParam: specifies a NULL value

      lParam: a pointer to a MSG structure

   GLOBAL VARIABLES USED:

      pfnOldHook

   NOTES:

     If(nCode < 0), return DefHookProc() IMMEDIATELY.

     If(MSGF_DIALOGBOX==nCode), set the local variable ptrMsg to
     point to the message structure. If this message is an F1
     keystroke, ptrMsg->hwnd will contain the HWND for the dialog
     control that the user wants help information on. Post a private
     message to the application, then return 1L to indicate that
     this message was handled.

     When the application receives the private message, it can call
     WinHelp(). WinHelp() must NOT be called directly from the
     FilterFunc() routine.

     In this example, post a private PM_CALLHELP message to the
     dialog box. wParam and lParam can be used to pass context
     information.

     If the message is not an F1 keystroke, or if nCode is
     MSGF_MENU, we return 0L to indicate that we did not process
     this message.
*/

_Check_return_
extern LRESULT CALLBACK
FilterFunc(
    _In_        int nCode,
    _In_        WPARAM wParam,
    _In_        LPARAM lParam)
{
   if(nCode < 0)
      return(CallNextHookEx(g_hhook, nCode, wParam, lParam));

   if(MSGF_DIALOGBOX == nCode)
    {
        MSG * ptrMsg = (MSG *) lParam;

        if((WM_KEYDOWN == ptrMsg->message) && (VK_F1 == ptrMsg->wParam))
        {   /* Use PostMessage() here to post an application-defined message to the application. Here is one possible call: */
            (void) PostMessage(GetParent(ptrMsg->hwnd), PM_CALLHELP, (WPARAM) ptrMsg->hwnd, 0L);
            return(1L); /* Handled it */
        }

        return(0L); /* Did not handle it */
    }

    return(0L); /* Did not handle it */
}

/*--------------------- end FilterFunc  ----------------------------*/

/*
Note that FilterFunc returns a DWORD.  Note also that not all messages are passed to DefHookProc. The
message hook is for this application only, so it is known that no other application needs to see these messages.
However, DefHookProc MUST be called when the nCode parameter is less than zero.

Have the application respond to the private message by calling WinHelp.
*/

_Check_return_
extern STATUS
dialog_dbox_process_windows(
    _InoutRef_  P_DIALOG p_dialog,
    _InoutRef_  P_DIALOG_CMD_PROCESS_DBOX p_dialog_cmd_process_dbox,
    _InRef_     PC_PIXIT_RECT p_pixit_rect)
{
    DIALOG_POSITION_TYPE dialog_position_type = ENUM_UNPACK(DIALOG_POSITION_TYPE, p_dialog_cmd_process_dbox->bits.dialog_position_type);
    STATUS status = STATUS_OK;
    PIXIT_RECT pixit_rect = *p_pixit_rect;
    GDI_POINT gdi_tl;
    P_GDI_POINT p_gdi_tl = &gdi_tl;
    SIZE PixelsPerInch;

    /* add top and left margins (still in pixit space) */
    pixit_rect.tl.x -= DIALOG_BOX_LM;
    pixit_rect.tl.y -= DIALOG_BOX_TM;

    /* add right and bottom margins (still in pixit space) */
    pixit_rect.br.x += DIALOG_BOX_RM;
    pixit_rect.br.y += DIALOG_BOX_BM;

    if(DIALOG_POSITION_DEFAULT == dialog_position_type)
        dialog_position_type = DIALOG_POSITION_NEAR_MOUSE;

    if(p_dialog_cmd_process_dbox->bits.use_windows_hwnd)
    {
        p_dialog->windows.hwnd_parent = p_dialog_cmd_process_dbox->windows.hwnd;
    }
    else if((DOCNO_NONE != p_dialog->docno) && !p_dialog->modeless)
    {
        const P_DOCU p_docu = p_docu_from_docno(p_dialog->docno);
        const P_VIEW p_view = p_view_from_viewno_caret(p_docu);
        p_dialog->windows.hwnd_parent = !IS_VIEW_NONE(p_view) ? p_view->main[WIN_BACK].hwnd : HOST_WND_NONE;
    }

    if(HOST_WND_NONE == p_dialog->windows.hwnd_parent)
        dialog_position_type = DIALOG_POSITION_CENTRE_SCREEN;

    host_get_pixel_size(NULL /*screen*/, &PixelsPerInch); /* Get current pixel size for this dialog e.g. 96 or 120 */

#define pixels_from_pixit_x(pixit_x) ( \
    (pixit_x * PixelsPerInch.cx) / PIXITS_PER_INCH)

#define pixels_from_pixit_y(pixit_y) ( \
    (pixit_y * PixelsPerInch.cy) / PIXITS_PER_INCH)

    switch(dialog_position_type)
    {
    default: default_unhandled();
#if CHECKING
    case DIALOG_POSITION_CENTRE_SCREEN:
#endif
        p_gdi_tl = NULL;
        break;

    case DIALOG_POSITION_CENTRE_WINDOW:
        {
        RECT client_rect;
        GetClientRect(p_dialog->windows.hwnd_parent, &client_rect);
        gdi_tl.x = ((client_rect.right  - client_rect.left)     - pixels_from_pixit_x(pixit_rect_width(&pixit_rect)) ) / 2;
        gdi_tl.y = ((client_rect.bottom - client_rect.top )     - pixels_from_pixit_y(pixit_rect_height(&pixit_rect))) / 2;
        break;
        }

    case DIALOG_POSITION_NEAR_MOUSE:
        if(dialog_statics.noted_position)
        {
            gdi_tl = dialog_statics.noted_gdi_tl;
            /* leave dialog_statics.note_position alone; this will apply at the end of this dialog session */
            dialog_statics.noted_position = FALSE;
            dialog_statics.noted_gdi_tl.x = 0;
            dialog_statics.noted_gdi_tl.y = 0;
        }
        else
        {
            POINT cursor_pos;
            GetCursorPos(&cursor_pos);
            gdi_tl.x = cursor_pos.x - 4;
            gdi_tl.y = cursor_pos.y - 4;
        }
        break;
    }

    status_return(dialog_windows_dlgtemplate_prepare(p_dialog, p_pixit_rect, p_gdi_tl));

    p_dialog->windows.dlg_filter_hook = g_hhook = SetWindowsHookEx(WH_MSGFILTER, FilterFunc, GetInstanceHandle(), GetCurrentThreadId());

    if(!p_dialog->modeless)
    {
        LPCDLGTEMPLATE lpTemplate = (LPCDLGTEMPLATE) /*GlobalLock*/(p_dialog->windows.dlgtemplate); /* GPTR -> GlobalLock just converts from HGLOBAL to pointer (same value) */
        if(NULL == lpTemplate) /* analyze: VC */
            status = status_check();
        else if(-1 == DialogBoxIndirectParam(GetInstanceHandle(), lpTemplate, p_dialog->windows.hwnd_parent, (DLGPROC) modal_dialog_handler, (LPARAM) p_dialog->h_dialog))
            status = status_nomem();
    }
    else
    {
        LPCDLGTEMPLATE lpTemplate = (LPCDLGTEMPLATE) /*GlobalLock*/(p_dialog->windows.dlgtemplate); /* GPTR -> GlobalLock just converts from HGLOBAL to pointer (same value) */
        if(NULL == lpTemplate) /* analyze: VC */
            status = status_check();
        else if(HOST_WND_NONE == (p_dialog->hwnd = CreateDialogIndirectParam(GetInstanceHandle(), lpTemplate, p_dialog->windows.hwnd_parent, (DLGPROC) modeless_dialog_handler, (LPARAM) p_dialog->h_dialog)))
            status = status_nomem();
        else
        {
            InvalidateRect(p_dialog->hwnd, NULL, TRUE);
            UpdateWindow(p_dialog->hwnd);
        }
    }

    return(status);
}

extern void
dialog_windows_ui_len_init(void)
{
    { /* SKS 18mar2010 - obtain message font from system metrics */
    NONCLIENTMETRICS nonclientmetrics;
    nonclientmetrics.cbSize = (UINT) sizeof32(NONCLIENTMETRICS);
#if (WINVER >= 0x0600) /* keep size compatible with older OSes even if we can target newer */
    nonclientmetrics.cbSize -= sizeof32(nonclientmetrics.iPaddedBorderWidth);
#endif
    if(WrapOsBoolChecking(SystemParametersInfo(SPI_GETNONCLIENTMETRICS, nonclientmetrics.cbSize, &nonclientmetrics, 0)))
    {
        dialog_statics.windows.logfont = nonclientmetrics.lfMessageFont;
    }
    else
    {
        LOGFONT logfont;
        zero_struct(logfont);
        logfont.lfHeight = -10;
        logfont.lfWeight = FW_NORMAL;
        logfont.lfPitchAndFamily = DEFAULT_PITCH | FF_ROMAN;
        tstr_xstrkpy(logfont.lfFaceName, elemof32(logfont.lfFaceName), TEXT("MS Shell Dlg"));
        dialog_statics.windows.logfont = logfont;
    }
    trace_2(TRACE_APP_DIALOG, TEXT("*** Dialog (message) logfont is %d, %s ***"), dialog_statics.windows.logfont.lfHeight, report_tstr(dialog_statics.windows.logfont.lfFaceName));
    } /*block*/

    dialog_statics.windows.pixels_per_four_h_du = 8; /* only true for the System font - we fill these in properly below */
    dialog_statics.windows.pixels_per_eight_v_du = 16;

    if((NULL == dialog_statics.windows.hfont) && (CH_NULL != dialog_statics.windows.logfont.lfFaceName[0]))
    {
        void_WrapOsBoolChecking(HOST_FONT_NONE != (
        dialog_statics.windows.hfont = CreateFontIndirect(&dialog_statics.windows.logfont)));

        if(dialog_statics.windows.hfont)
        {
            const HWND hwnd = NULL; /* screen */
            const HDC hdc = GetDC(hwnd); /* only used here as an IC */
            assert(hdc);
            if(NULL != hdc)
            {
                HFONT old_hfont = SelectFont(hdc, dialog_statics.windows.hfont);
                SIZE size;
                TEXTMETRIC textmetric;
                S32 avgWidth, avgHeight;

                void_WrapOsBoolChecking(GetTextMetrics(hdc, &textmetric));
                trace_2(TRACE_APP_DIALOG, TEXT("GetTextMetrics() yields average width=%d, height=%d"), textmetric.tmAveCharWidth, textmetric.tmHeight);

                void_WrapOsBoolChecking(GetTextExtentPoint32(hdc,
                    TEXT("ABCDEFGHIJKLMNOPQRSTUVWXYZ")
                    TEXT("abcdefghijklmnopqrstuvwxyz"),
                    26 * 2, &size));
                avgWidth = (S32) (((size.cx / 26) + 1) / 2); /* See KB145994 - tmAveCharWidth is imprecise */
                avgHeight = (S32) textmetric.tmHeight; /* DPI-aware */
                trace_1(TRACE_APP_DIALOG, TEXT("GetTextExtentPoint32() yields average width=%d"), avgWidth);

                /* One horizontal base unit is equal to one-fourth of the average character width for the font. */
                dialog_statics.windows.pixels_per_four_h_du = avgWidth;

                /* One vertical base unit is equal to one-eighth of the average character height for the font */
                dialog_statics.windows.pixels_per_eight_v_du = avgHeight;

                consume(HFONT, SelectFont(hdc, old_hfont));

                void_WrapOsBoolChecking(1 == ReleaseDC(hwnd, hdc));
            }
        }
    }
}

_Check_return_
extern PIXIT
ui_width_from_tstr_host(
    _In_z_      PCTSTR tstr)
{
    const U32 tchars_n = tstrlen32(tstr);
    PIXIT width;

    assert(dialog_statics.windows.logfont.lfHeight);

    if(dialog_statics.windows.hfont)
    {
        const HWND hwnd = NULL; /* screen */
        const HDC hdc = GetDC(hwnd); /* only used here as an IC */
        assert(hdc);
        if(NULL != hdc)
        {
            HFONT old_hfont = SelectFont(hdc, dialog_statics.windows.hfont);
            SIZE size;

            void_WrapOsBoolChecking(
                GetTextExtentPoint32(hdc, tstr, (int) tchars_n, &size));

            consume(HFONT, SelectFont(hdc, old_hfont));

            void_WrapOsBoolChecking(1 == ReleaseDC(hwnd, hdc));

            /* return width rounded out plus a wee bit as multiples of dialog units */ /* DPI-aware */
            width = PIXITS_PER_WDU_H * (2 + muldiv64_ceil(size.cx, 4, dialog_statics.windows.pixels_per_four_h_du));
            return(width);
        }
    }

    width = (PIXIT) DIALOG_SYSCHARSL_H(tchars_n);
    return(width);
}

extern void
ui_list_size_estimate(
    _InVal_     S32 num_elements,
    _OutRef_    P_PIXIT_SIZE p_pixit_size)
{
    S32 show_elements = num_elements;
    S32 max_elements;
    GDI_SIZE screen_gdi_size;

    /* always give the idea that there should be *some* list elements */
    show_elements = MAX(3, show_elements);

    assert(dialog_statics.windows.logfont.lfHeight);

    /* always dynamically limit to a portion of the host's work area */
#define UI_LIST_LIMIT_NUMERATOR     3
#define UI_LIST_LIMIT_DENOMINATOR   5

    host_work_area_gdi_size_query(&screen_gdi_size); /* NB pixels */

    /* remembering to go from pixels via (multiples of) dialog units */
    max_elements = muldiv64(screen_gdi_size.cy, UI_LIST_LIMIT_NUMERATOR * 8, UI_LIST_LIMIT_DENOMINATOR * dialog_statics.windows.pixels_per_eight_v_du) / (DIALOG_STDLISTITEM_V / PIXITS_PER_WDU_V);
    trace_2(TRACE_APP_DIALOG, TEXT("work area size.cy=") S32_TFMT TEXT(", max_elements = ") S32_TFMT, screen_gdi_size.cy, max_elements);

    p_pixit_size->cx = 0;

    if(show_elements > max_elements)
    {
        show_elements = max_elements;
        p_pixit_size->cx += DIALOG_STDLISTEXTRA_H;
    }

    p_pixit_size->cy = show_elements * DIALOG_STDLISTITEM_V;
}

#endif /* WINDOWS */

/* end of wi_dlg.c */
