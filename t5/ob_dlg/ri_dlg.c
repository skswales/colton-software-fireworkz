/* ri_dlg.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* dialog UI handling (RISC OS) */

/* SKS May 1992 */

#include "common/gflags.h"

#include "ob_dlg/ui_dlgin.h"

#define USE_ACORN_UPDOWN

/*
callback routines
*/

static
mlec_event_proto(dialog_riscos_mlec_event_handler, rc, handle, p_eventdata);

/*
internal structure
*/

typedef struct DIALOG_CONTROL_REDRAW
{
    DIALOG_RISCOS_REDRAW_WINDOW dialog_riscos_redraw_window;

    P_DIALOG p_dialog;
    P_DIALOG_ICTL_GROUP p_ictl_group;
    P_DIALOG_ICTL p_dialog_ictl;
    P_DIALOG_ICTL_EDIT_XX p_dialog_ictl_edit_xx;

    PC_DIALOG_CONTROL_DATA p_dialog_control_data;
}
DIALOG_CONTROL_REDRAW, * P_DIALOG_CONTROL_REDRAW;

/*
internal routines
*/

OBJECT_PROTO(extern, object_dialog);

_Check_return_
_Ret_maybenull_
static P_RI_LBOX_HANDLE
dialog_riscos_event_lbn_scan_ictls_for_owner(
    /*out*/ P_P_DIALOG p_p_dialog,
    /*out*/ P_P_DIALOG_ICTL p_p_dialog_ictl,
    RI_LBOX_HANDLE lbox_handle,
    _InVal_     CLIENT_HANDLE client_handle);

_Check_return_
static STATUS
dialog_riscos_Key_Pressed(
    P_DIALOG p_dialog,
    _In_        KMAP_CODE ch);

_Check_return_
static STATUS
dialog_riscos_mlec_event_Mouse_Click(
    P_DIALOG p_dialog,
    P_DIALOG_ICTL p_dialog_ictl,
    P_DIALOG_ICTL_EDIT_XX p_dialog_ictl_edit_xx,
    _In_        const WimpMouseClickEvent * const p_mouse_click);

_Check_return_
static POINTER_SHAPE
dialog_riscos_mlec_event_determine_pshape(
    P_DIALOG p_dialog,
    P_DIALOG_ICTL p_dialog_ictl,
    P_DIALOG_ICTL_EDIT_XX p_dialog_ictl_edit_xx,
    _In_        const WimpMouseClickEvent * const p_mouse_click);

static void
dialog_riscos_mlec_enull(
    P_DIALOG p_dialog,
    P_DIALOG_ICTL p_dialog_ictl);

static void
dialog_riscos_mlec_update(
    P_DIALOG p_dialog,
    P_DIALOG_ICTL_EDIT_XX p_dialog_ictl_edit_xx,
    _InRef_     PC_BBox p_update_box,
    _InVal_     BOOL later);

static void
dialog_riscos_null_event_do(
    P_DIALOG p_dialog);

_Check_return_
static STATUS
dialog_riscos_redraw_controls_in(
    P_DIALOG_CONTROL_REDRAW p_dialog_control_redraw);

static void
dialog_riscos_redraw_core(
    _Inout_     WimpRedrawWindowBlock * const p_redraw_window_block,
    _InVal_     H_DIALOG h_dialog,
    _InVal_     BOOL is_update_now);

static DIALOG_CONTROL_ID
dialog_riscos_scan_controls_in(
    P_DIALOG p_dialog,
    P_DIALOG_ICTL_GROUP p_ictl_group,
    _InVal_     wimp_i icon_handle);

_Check_return_
static STATUS
part_control_redraw(
    P_DIALOG_CONTROL_REDRAW p_dialog_control_redraw,
    _In_        FRAMED_BOX_STYLE b,
    _InRef_     PC_DIALOG_WIMP_I i,
    _InVal_     DIALOG_CONTROL_TYPE dialog_control_type);

_Check_return_
_Ret_maybenull_
static inline _kernel_oserror *
wimp_get_icon_state_with_bitset(
    _InVal_     wimp_w window_handle,
    _InVal_     wimp_i icon_handle,
    _Out_       WimpIconBlockWithBitset * const p_icon_block)
{
    _kernel_oserror * e;
    WimpGetIconStateBlock b;

    b.window_handle = window_handle;
    b.icon_handle = icon_handle;

    e = wimp_get_icon_state(&b);

    *p_icon_block = * (WimpIconBlockWithBitset *) &b.icon;

    return(e);
}

static
struct DIALOG_NULL_STATICS
{
    WimpGetPointerInfoBlock pointer_info;
    MONOTIME pacemaker;
}
dialog_null_statics;

static BOOL
dragging_file_icon = FALSE;

extern void
dialog_riscos_big_mods(
    P_DIALOG p_dialog,
    _InVal_     BOOL start)
{
    if(start)
    {
        assert(!p_dialog->riscos.accumulate_box);
        p_dialog->riscos.accumulate_box = 1;
        p_dialog->riscos.invalid_bbox.xmin = p_dialog->riscos.invalid_bbox.ymin = S16_MAX;
        p_dialog->riscos.invalid_bbox.xmax = p_dialog->riscos.invalid_bbox.ymax = S16_MIN;
        p_dialog->big_mods = 1;
    }
    else
    {
        assert(p_dialog->riscos.accumulate_box);
        p_dialog->riscos.accumulate_box = 0;
        p_dialog->big_mods = 0;
        if( (p_dialog->riscos.invalid_bbox.xmin < p_dialog->riscos.invalid_bbox.xmax) &&
            (p_dialog->riscos.invalid_bbox.ymin < p_dialog->riscos.invalid_bbox.ymax) )
        {
            void_WrapOsErrorReporting(wimp_force_redraw_BBox(p_dialog->hwnd, &p_dialog->riscos.invalid_bbox));
        }
    }
}

/******************************************************************************
*
* dialog box coordinates are in twips and upside down
* and are inc,exc,exc,inc not inc,inc,exc,exc
* so convert and round the box out to RISC OS pixels
*
******************************************************************************/

extern void
dialog_riscos_box_from_pixit_rect(
    _OutRef_    P_GDI_BOX p_box,
    _InRef_     PC_PIXIT_RECT p_rect)
{
    const S32 dx = host_modevar_cache_current.dx;
    const S32 dy = host_modevar_cache_current.dy;

    /* bl */
    p_box->x0 = +idiv_floor(p_rect->tl.x, dx * PIXITS_PER_RISCOS) * dx;
    p_box->y0 = -idiv_floor(p_rect->br.y, dy * PIXITS_PER_RISCOS) * dy;
    /* one would naturally add a dy to convert from exc to inc, but a dy in the opposite direction
     * is needed to shift the zero line down to the first visible raster of a window
     * also draw yourself a diagram - if the br is exclusive, it ought not to draw in the pixel that maps to
    */

    /* tr */
    p_box->x1 = +idiv_floor(p_rect->br.x, dx * PIXITS_PER_RISCOS) * dx;
    p_box->y1 = -idiv_floor(p_rect->tl.y, dy * PIXITS_PER_RISCOS) * dy; /* ditto, from inc to exc ... */
}

static const RESOURCE_BITMAP_ID radio[2] =
{
    { OBJECT_ID_SKEL, BITMAP_NAME_RADIO_OFF },
    { OBJECT_ID_SKEL, BITMAP_NAME_RADIO_ON }
};

static const RESOURCE_BITMAP_ID check[2] =
{
    { OBJECT_ID_SKEL, BITMAP_NAME_CHECK_OFF },
    { OBJECT_ID_SKEL, BITMAP_NAME_CHECK_ON }
};

static const RESOURCE_BITMAP_ID tristate[3] =
{
    { OBJECT_ID_SKEL, BITMAP_NAME_TRISTATE_OFF },
    { OBJECT_ID_SKEL, BITMAP_NAME_TRISTATE_ON },
    { OBJECT_ID_SKEL, BITMAP_NAME_TRISTATE_DONT_CARE }
};

static const RESOURCE_BITMAP_ID dec = { OBJECT_ID_SKEL, SKEL_ID_BM_DEC };

static const RESOURCE_BITMAP_ID inc = { OBJECT_ID_SKEL, SKEL_ID_BM_INC };

static const RESOURCE_BITMAP_ID dropdown = { OBJECT_ID_SKEL, BITMAP_NAME_COMBO };

static void
dialog_riscos_cache_common_bitmap(
    P_DIALOG_RISCOS_RESOURCE_BITMAP_COMMON p,
    _InRef_     PC_RESOURCE_BITMAP_ID pc)
{
    p->handle = resource_bitmap_find_defaulting(pc);
    resource_bitmap_gdi_size_query(p->handle, &p->size);
}

extern void
dialog_riscos_cache_common_bitmaps(
    P_DIALOG p_dialog)
{
    dialog_riscos_cache_common_bitmap(&p_dialog->riscos.bitmap_radio_off, &radio[0]);
    dialog_riscos_cache_common_bitmap(&p_dialog->riscos.bitmap_radio_on,  &radio[1]);
    dialog_riscos_cache_common_bitmap(&p_dialog->riscos.bitmap_check_off, &check[0]);
    dialog_riscos_cache_common_bitmap(&p_dialog->riscos.bitmap_check_on,  &check[1]);
    dialog_riscos_cache_common_bitmap(&p_dialog->riscos.bitmap_tristate_off,       &tristate[0]);
    dialog_riscos_cache_common_bitmap(&p_dialog->riscos.bitmap_tristate_on,        &tristate[1]);
    dialog_riscos_cache_common_bitmap(&p_dialog->riscos.bitmap_tristate_dont_care, &tristate[2]);
    dialog_riscos_cache_common_bitmap(&p_dialog->riscos.bitmap_dec, &dec);
    dialog_riscos_cache_common_bitmap(&p_dialog->riscos.bitmap_inc, &inc);
    dialog_riscos_cache_common_bitmap(&p_dialog->riscos.bitmap_dropdown, &dropdown);
}

extern void
dialog_riscos_free_cached_bitmaps(
    P_DIALOG p_dialog)
{
    resource_bitmap_lose(&p_dialog->riscos.bitmap_radio_on.handle);
    resource_bitmap_lose(&p_dialog->riscos.bitmap_radio_off.handle);
    resource_bitmap_lose(&p_dialog->riscos.bitmap_check_on.handle);
    resource_bitmap_lose(&p_dialog->riscos.bitmap_check_off.handle);
    resource_bitmap_lose(&p_dialog->riscos.bitmap_tristate_on.handle);
    resource_bitmap_lose(&p_dialog->riscos.bitmap_tristate_off.handle);
    resource_bitmap_lose(&p_dialog->riscos.bitmap_tristate_dont_care.handle);
    resource_bitmap_lose(&p_dialog->riscos.bitmap_inc.handle);
    resource_bitmap_lose(&p_dialog->riscos.bitmap_dec.handle);
    resource_bitmap_lose(&p_dialog->riscos.bitmap_dropdown.handle);
}

PROC_DIALOG_EVENT_PROTO(static, lbn_event_dialog)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    return(object_call_DIALOG(dialog_message, p_data));
}

_Check_return_
static STATUS
dialog_riscos_combobox_dropdown(
    P_DIALOG p_dialog,
    P_DIALOG_ICTL p_dialog_ictl)
{
    BBox abs_bbox;

    assert((p_dialog_ictl->dialog_control_type == DIALOG_CONTROL_COMBO_TEXT) || (p_dialog_ictl->dialog_control_type == DIALOG_CONTROL_COMBO_S32));

    { /* read coordinates of parent icon */
    GDI_POINT gdi_org;
    WimpIconBlockWithBitset icon;

    host_gdi_org_from_screen(&gdi_org, p_dialog->hwnd); /* window work area ABS origin */

    void_WrapOsErrorReporting(wimp_get_icon_state_with_bitset(p_dialog->hwnd, p_dialog_ictl->riscos.dwi[0].icon_handle, &icon));
    abs_bbox.xmin = icon.bbox.xmin + gdi_org.x;
    abs_bbox.ymin = icon.bbox.ymin + gdi_org.y;
    abs_bbox.xmax = icon.bbox.xmax + gdi_org.x;
    abs_bbox.ymax = icon.bbox.ymax + gdi_org.y;
    } /*block*/

    { /* RISC OS 3-like: off to the right, ensuring same width available */
    int width = BBox_width(&abs_bbox);
    abs_bbox.xmin = abs_bbox.xmax + (p_dialog->riscos.bitmap_dropdown.size.cx / 2); /* SKS after 1.03 17jul93 make halfway though the dropdown icon */
    abs_bbox.xmax = abs_bbox.xmin + width;
    } /*block*/
    {
    int height = (int) (p_dialog_ictl->p_dialog_control_data.combo_text->combo_xx.dropdown_size / PIXITS_PER_RISCOS);
    if(!height)
        height = PIXITS_PER_INCH / PIXITS_PER_RISCOS;
    abs_bbox.ymin = abs_bbox.ymax - height;
    } /*block*/

    /* ensure we have a list box structure created for our use */
    if(RI_LBOX_HANDLE_NONE == p_dialog_ictl->data.combo_xx.list_xx.riscos.lbox)
    {
        RI_LBOX_NEW _ri_lbox_new;

        zero_struct(_ri_lbox_new);

        _ri_lbox_new.bbox = abs_bbox;

        _ri_lbox_new.caption = p_dialog_ictl->p_dialog_control_data.combo_text->combo_xx.dropdown_title;

        _ri_lbox_new.p_docu = p_docu_from_docno(p_dialog->docno);
        _ri_lbox_new.p_proc_dialog_event = lbn_event_dialog;
        _ri_lbox_new.client_handle = (CLIENT_HANDLE) p_dialog->h_dialog;

        _ri_lbox_new.bits.disable_double = TRUE;

        _ri_lbox_new.p_ui_control = p_dialog_ictl->data.combo_xx.list_xx.p_ui_control_s32;
        _ri_lbox_new.p_ui_source = p_dialog_ictl->data.combo_xx.list_xx.p_ui_source;

        switch(p_dialog_ictl->dialog_control_type)
        {
        default: default_unhandled();
#if CHECKING
        case DIALOG_CONTROL_COMBO_TEXT:
#endif
            _ri_lbox_new.ui_data_type = UI_DATA_TYPE_TEXT;
            break;

        case DIALOG_CONTROL_COMBO_S32:
            _ri_lbox_new.ui_data_type = UI_DATA_TYPE_S32;
            break;
        }

        status_return(ri_lbox_new(&p_dialog_ictl->data.combo_xx.list_xx.riscos.lbox, &_ri_lbox_new));

        /* initially reflect selection in list box iff state is one of those represented */
        if(p_dialog_ictl->state.list_text.itemno >= 0)
            (void) ri_lbox_selection_set_from_itemno(p_dialog_ictl->data.combo_xx.list_xx.riscos.lbox, p_dialog_ictl->state.list_text.itemno);
    }

    ri_lbox_make_child(p_dialog_ictl->data.combo_xx.list_xx.riscos.lbox, p_dialog->hwnd, p_dialog_ictl->riscos.dwi[1].icon_handle);

    { /* current RISC OS 3 convention is to have this as a free-floating top-level menu */
    GDI_POINT tl;
    tl.x = abs_bbox.xmin;
    tl.y = abs_bbox.ymax;
    ri_lbox_show_submenu(p_dialog_ictl->data.combo_xx.list_xx.riscos.lbox, RI_LBOX_SUBMENU_OF_COMPLEX_MENU, &tl);
    } /*block*/

    ri_lbox_focus_set(p_dialog_ictl->data.combo_xx.list_xx.riscos.lbox);
    return(STATUS_OK);
}

static void
dialog_riscos_current_move(
    P_DIALOG p_dialog,
    _InVal_     DIALOG_CONTROL_ID current_id,
    _InVal_     STATUS movement)
{
    DIALOG_CMD_CTL_FOCUS_SET dialog_cmd_ctl_focus_set;
    msgclr(dialog_cmd_ctl_focus_set);
    dialog_cmd_ctl_focus_set.h_dialog = p_dialog->h_dialog;
    dialog_cmd_ctl_focus_set.dialog_control_id = dialog_current_move(p_dialog, current_id, movement);
    if(dialog_cmd_ctl_focus_set.dialog_control_id)
        status_assert(object_call_DIALOG(DIALOG_CMD_CODE_CTL_FOCUS_SET, &dialog_cmd_ctl_focus_set));
}

/******************************************************************************
*
* RISC OS dialog box event handler
*
******************************************************************************/

_Check_return_
static int
dialog_riscos_dbox_event_Close_Window(
    _InVal_     H_DIALOG h_dialog);

_Check_return_
static int
dialog_riscos_dbox_event_Open_Window_Request(
    _InVal_     H_DIALOG h_dialog,
    _In_        const WimpOpenWindowRequestEvent * const p_open_window_request);

_Check_return_
static int
dialog_riscos_dbox_event_Redraw_Window_Request(
    _InVal_     H_DIALOG h_dialog,
    _In_        const WimpRedrawWindowRequestEvent * const p_redraw_window_request);

_Check_return_
static int
dialog_riscos_dbox_event_Mouse_Click(
    _InVal_     H_DIALOG h_dialog,
    _In_        const WimpMouseClickEvent * const p_mouse_click);

_Check_return_
static int
dialog_riscos_dbox_event_Key_Pressed(
    _InVal_     H_DIALOG h_dialog,
    _In_        const WimpKeyPressedEvent * const p_key_pressed);

_Check_return_
static int
dialog_riscos_dbox_event_User_Drag_Box(
    _InVal_     H_DIALOG h_dialog,
    _In_        const WimpUserDragBoxEvent * const p_user_drag_box);

_Check_return_
static int
dialog_riscos_dbox_event_Pointer_Entering_Window(
    _InVal_     H_DIALOG h_dialog);

_Check_return_
static int
dialog_riscos_dbox_event_Pointer_Leaving_Window(
    _InVal_     H_DIALOG h_dialog);

_Check_return_
static int
dialog_riscos_dbox_event_User_Message(
    _InVal_     H_DIALOG h_dialog,
    _InRef_     PC_WimpMessage p_wimp_message);

_Check_return_
extern int
dialog_riscos_dbox_event_handler(
    _InVal_     int event_code,
    _In_        const WimpPollBlock * const p_event_data,
    P_ANY handle)
{
    trace_3(TRACE_RISCOS_HOST, TEXT("%s: %s handle=") PTR_XTFMT, __Tfunc__, report_wimp_event(event_code, p_event_data), handle);

    switch(event_code)
    {
    case Wimp_ERedrawWindow:
        return(dialog_riscos_dbox_event_Redraw_Window_Request((H_DIALOG) handle, &p_event_data->redraw_window_request));

    case Wimp_EOpenWindow:
        return(dialog_riscos_dbox_event_Open_Window_Request((H_DIALOG) handle, &p_event_data->open_window_request));

    case Wimp_ECloseWindow:
        return(dialog_riscos_dbox_event_Close_Window((H_DIALOG) handle));

    case Wimp_EPointerLeavingWindow:
        return(dialog_riscos_dbox_event_Pointer_Leaving_Window((H_DIALOG) handle));

    case Wimp_EPointerEnteringWindow:
        return(dialog_riscos_dbox_event_Pointer_Entering_Window((H_DIALOG) handle));

    case Wimp_EMouseClick:
        return(dialog_riscos_dbox_event_Mouse_Click((H_DIALOG) handle, &p_event_data->mouse_click));

    case Wimp_EUserDrag:
        return(dialog_riscos_dbox_event_User_Drag_Box((H_DIALOG) handle, &p_event_data->user_drag_box));

    case Wimp_EKeyPressed:
        return(dialog_riscos_dbox_event_Key_Pressed((H_DIALOG) handle, &p_event_data->key_pressed));

    case Wimp_EUserMessage:
    case Wimp_EUserMessageRecorded:
        return(dialog_riscos_dbox_event_User_Message((H_DIALOG) handle, &p_event_data->user_message));

    default:
        break;
    }

    return(FALSE /*unprocessed*/);
}

_Check_return_
static int
dialog_riscos_dbox_event_Close_Window(
    _InVal_     H_DIALOG h_dialog)
{
    /* send a Cancel to the user's processor */
    P_DIALOG p_dialog = p_dialog_from_h_dialog(h_dialog);
    DIALOG_CMD_COMPLETE_DBOX dialog_cmd_complete_dbox;
    msgclr(dialog_cmd_complete_dbox);
    dialog_cmd_complete_dbox.h_dialog = p_dialog->h_dialog;
    dialog_cmd_complete_dbox.completion_code = DIALOG_COMPLETION_CANCEL;
    status_assert(object_call_DIALOG(DIALOG_CMD_CODE_COMPLETE_DBOX, &dialog_cmd_complete_dbox));
    return(TRUE /*processed*/);
}

_Check_return_
static int
dialog_riscos_dbox_event_Open_Window_Request(
    _InVal_     H_DIALOG h_dialog,
    _In_        const WimpOpenWindowRequestEvent * const p_open_window_request)
{
    WimpOpenWindowRequestEvent open_window_request = *p_open_window_request; /* open may adjust coords */
    WimpGetPointerInfoBlock pointer_info;
    void_WrapOsErrorReporting(winx_open_window(&open_window_request));
    void_WrapOsErrorReporting(wimp_get_pointer_info(&pointer_info));
    if(pointer_info.window_handle == open_window_request.window_handle)
    {
        /* fake ourselves a pointer entering window event cos Window Manager sometimes isn't up to it */
        DIALOG_RISCOS_EVENT_POINTER_ENTER dialog_riscos_event_pointer_enter;
        dialog_riscos_event_pointer_enter.h_dialog = h_dialog;
        dialog_riscos_event_pointer_enter.enter = 1;
        status_assert(object_call_DIALOG(DIALOG_RISCOS_EVENT_CODE_POINTER_ENTER, &dialog_riscos_event_pointer_enter));
    }
    return(TRUE /*processed*/);
}

_Check_return_
static int
dialog_riscos_dbox_event_Redraw_Window_Request(
    _InVal_     H_DIALOG h_dialog,
    _In_        const WimpRedrawWindowRequestEvent * const p_redraw_window_request)
{
    WimpRedrawWindowBlock redraw_window_block;
    int wimp_more;

    redraw_window_block.window_handle = p_redraw_window_request->window_handle;

    if(NULL != WrapOsErrorReporting(wimp_redraw_window(&redraw_window_block, &wimp_more)))
        wimp_more = 0;

    while(0 != wimp_more)
    {
        dialog_riscos_redraw_core(&redraw_window_block, h_dialog, 0);

        if(NULL != WrapOsErrorReporting(wimp_get_rectangle(&redraw_window_block, &wimp_more)))
            wimp_more = 0;
    }

    return(TRUE /*processed*/);
}

_Check_return_
static int
dialog_riscos_dbox_event_Mouse_Click(
    _InVal_     H_DIALOG h_dialog,
    _In_        const WimpMouseClickEvent * const p_mouse_click)
{
    /* send ourselves the mouse click event */
    DIALOG_RISCOS_EVENT_MOUSE_CLICK dialog_riscos_event_mouse_click;
    dialog_riscos_event_mouse_click.h_dialog = h_dialog;
    dialog_riscos_event_mouse_click.mouse_click = *p_mouse_click;
    status_assert(object_call_DIALOG(DIALOG_RISCOS_EVENT_CODE_MOUSE_CLICK, &dialog_riscos_event_mouse_click));
    return(TRUE /*processed*/);
}

_Check_return_
static int
dialog_riscos_dbox_event_Key_Pressed(
    _InVal_     H_DIALOG h_dialog,
    _In_        const WimpKeyPressedEvent * const p_key_pressed)
{
    /* send ourselves the key press event */
    DIALOG_RISCOS_EVENT_KEY_PRESSED dialog_riscos_event_key_pressed;
    dialog_riscos_event_key_pressed.h_dialog = h_dialog;
    dialog_riscos_event_key_pressed.key_pressed = *p_key_pressed;
    status_assert(object_call_DIALOG(DIALOG_RISCOS_EVENT_CODE_KEY_PRESSED, &dialog_riscos_event_key_pressed));
    return(dialog_riscos_event_key_pressed.processed);
}

_Check_return_
static int
dialog_riscos_dbox_event_User_Drag_Box(
    _InVal_     H_DIALOG h_dialog,
    _In_        const WimpUserDragBoxEvent * const p_user_drag_box)
{
    /* send ourselves the drag ended event */
    DIALOG_RISCOS_EVENT_USER_DRAG dialog_riscos_event_userdrag;
    dialog_riscos_event_userdrag.h_dialog = h_dialog;
    dialog_riscos_event_userdrag.user_drag_box = *p_user_drag_box;
    status_assert(object_call_DIALOG(DIALOG_RISCOS_EVENT_CODE_USER_DRAG, &dialog_riscos_event_userdrag));
    return(TRUE /*processed*/);
}

_Check_return_
static int
dialog_riscos_dbox_event_Pointer_Entering_Window(
    _InVal_     H_DIALOG h_dialog)
{
    DIALOG_RISCOS_EVENT_POINTER_ENTER dialog_riscos_event_pointer_enter;
    dialog_riscos_event_pointer_enter.h_dialog = h_dialog;
    dialog_riscos_event_pointer_enter.enter = TRUE;
    status_assert(object_call_DIALOG(DIALOG_RISCOS_EVENT_CODE_POINTER_ENTER, &dialog_riscos_event_pointer_enter));
    return(TRUE /*processed*/);
}

_Check_return_
static int
dialog_riscos_dbox_event_Pointer_Leaving_Window(
    _InVal_     H_DIALOG h_dialog)
{
    DIALOG_RISCOS_EVENT_POINTER_ENTER dialog_riscos_event_pointer_enter;
    dialog_riscos_event_pointer_enter.h_dialog = h_dialog;
    dialog_riscos_event_pointer_enter.enter = FALSE;
    status_assert(object_call_DIALOG(DIALOG_RISCOS_EVENT_CODE_POINTER_ENTER, &dialog_riscos_event_pointer_enter));
    return(TRUE /*processed*/);
}

_Check_return_
static int
dialog_riscos_dbox_event_User_Message(
    _InVal_     H_DIALOG h_dialog,
    _InRef_     PC_WimpMessage p_wimp_message)
{
    switch(p_wimp_message->hdr.action_code)
    {
    case Wimp_MHelpRequest:
        {
        P_DIALOG p_dialog = p_dialog_from_h_dialog(h_dialog);

        if((int) p_wimp_message->data.help_request.icon_handle >= -1) /* don't reply to system icons help requests */
        {
            if(DOCNO_NONE != p_dialog->docno)
                reply_to_help_request(p_docu_from_docno(p_dialog->docno), p_wimp_message);
        }

        return(TRUE /*processed*/);
        }

    case Wimp_MDataLoad:    /* File dragged from directory display, dropped on our window */
    case Wimp_MDataSave:    /* File dragged from another application, dropped on our window */
        {
        P_DIALOG p_dialog = p_dialog_from_h_dialog(h_dialog);
        DIALOG_CONTROL_ID dialog_control_id;
        BOOL processed = FALSE;

        if((dialog_control_id = dialog_riscos_scan_controls_in(p_dialog, &p_dialog->ictls, p_wimp_message->data.data_load.destination_icon)) != 0)
        {
            const P_DIALOG_ICTL p_dialog_ictl = p_dialog_ictl_from_control_id(p_dialog, dialog_control_id);
            P_DIALOG_ICTL_EDIT_XX p_dialog_ictl_edit_xx = p_dialog_ictl_edit_xx_from(p_dialog_ictl);

            if((NULL != p_dialog_ictl_edit_xx) && (NULL != p_dialog_ictl_edit_xx->riscos.mlec))
            {
                STATUS status = mlec__User_Message(p_dialog_ictl_edit_xx->riscos.mlec, p_wimp_message);

                processed = status_done(status);
            }
        }

        return(processed);
        }

    default:
        return(FALSE /*unprocessed*/);
    }
}

_Check_return_
extern STATUS
dialog_riscos_file_icon_drag(
    _InVal_     H_DIALOG h_dialog,
    _In_        WimpIconBlockWithBitset * const p_icon)
{
    P_DIALOG p_dialog;
    WimpDragBox dr;
    int try_drag_sprite;

    if(NULL == (p_dialog = p_dialog_from_h_dialog(h_dialog)))
        return(create_error(DIALOG_ERR_UNKNOWN_DIALOG));

    dr.wimp_window = p_dialog->hwnd; /* needed for event direction */

    dr.dragging_box = p_icon->bbox;

    { /* Find ABS screen origin of containing window's work area */
    GDI_POINT gdi_org;
    host_gdi_org_from_screen(&gdi_org, p_dialog->hwnd); /* window work area ABS origin */
    dr.dragging_box.xmin += gdi_org.x;
    dr.dragging_box.ymin += gdi_org.y;
    dr.dragging_box.xmax += gdi_org.x;
    dr.dragging_box.ymax += gdi_org.y;
    } /*block*/

    try_drag_sprite = p_icon->flags.bits.sprite && !p_icon->flags.bits.text;

    if(try_drag_sprite) /* Have a look at user's choice too */
        try_drag_sprite = ((_kernel_osbyte(161, 28, 0) >> 8 /*R2*/) & (1 << 1));

    if(try_drag_sprite)
    {
        assert(!(p_icon->flags.bits.indirect));

        void_WrapOsErrorReporting(winx_drag_a_sprite_start(
                                (1 << 0) |
                                (1 << 2) |
                                (0 << 4) /*box is whole screen*/ |
                                (1 << 6) /*box applies to pointer*/ |
                                (1 << 7) /*drop shadow*/,
                                1 /*wimp sprite pool*/,
                                p_icon->data.s,
                                &dr));
    }
    else
    {
        WimpGetPointerInfoBlock mouse;
        int x_limit = host_modevar_cache_current.gdi_size.cx;
        int y_limit = host_modevar_cache_current.gdi_size.cy;

        dr.drag_type = Wimp_DragBox_DragFixedDash;

        /* Get ABS pointer position to allow icon to be dragged partially off-screen */
        void_WrapOsErrorReporting(wimp_get_pointer_info(&mouse));

        dr.parent_box.xmin = dr.dragging_box.xmin - mouse.x; /* Expanded parent by box overlap */
        dr.parent_box.ymin = dr.dragging_box.ymin - mouse.y;
        dr.parent_box.xmax = dr.dragging_box.xmax - mouse.x + x_limit;
        dr.parent_box.ymax = dr.dragging_box.ymax - mouse.y + y_limit;

        void_WrapOsErrorReporting(winx_drag_box(&dr));
    }

    dragging_file_icon = TRUE;

    return(STATUS_OK);
}

extern void
dialog_riscos_file_icon_setup(
    _Inout_     WimpIconBlockWithBitset * const p_icon,
    _InVal_     T5_FILETYPE t5_filetype)
{
    /* fills in sprite_name with name of a sprite representing the filetype from the Window Manager Sprite Pool */
    riscos_filesprite(p_icon->data.s, 12, t5_filetype);

    p_icon->flags.bits.indirect    = 0;

    p_icon->flags.bits.sprite      = 1;
    p_icon->flags.bits.horz_centre = 1;
    p_icon->flags.bits.vert_centre = 1;
}

static void
dialog_riscos_dbox_modify_open_type_in(
    P_DIALOG_ICTL_GROUP p_ictl_group,
    _InoutRef_  P_S32 p_type)
{
    ARRAY_INDEX i;

    for(i = 0; i < n_ictls_from_group(p_ictl_group); ++i)
    {
        const P_DIALOG_ICTL p_dialog_ictl = p_dialog_ictl_from(p_ictl_group, i);

        switch(p_dialog_ictl->dialog_control_type)
        {
#ifdef MRJC_SWITCHED_OFF
            default:
#endif
            case DIALOG_CONTROL_LIST_TEXT:
            case DIALOG_CONTROL_LIST_S32:
            case DIALOG_CONTROL_COMBO_TEXT:
            case DIALOG_CONTROL_COMBO_S32:
                /* these can/do open list box as a subwindow, so dialog can't itself be a true menu (but gets to be complex) */
                *p_type = DIALOG_RISCOS_NOT_MENU;
                break;

            case DIALOG_CONTROL_GROUPBOX:
                dialog_riscos_dbox_modify_open_type_in(&p_dialog_ictl->data.groupbox.ictls, p_type);
                break;

#ifndef MRJC_SWITCHED_OFF
            default:
                *p_type = DIALOG_RISCOS_NOT_MENU;
                break;
#endif
        }
    }
}

extern void
dialog_riscos_dbox_modify_open_type(
    P_DIALOG p_dialog,
    _InoutRef_  P_S32 p_type)
{
    dialog_riscos_dbox_modify_open_type_in(&p_dialog->ictls, p_type);
}

/******************************************************************************
*
* client has told us about an unknown click in his window
*
******************************************************************************/

_Check_return_
static STATUS
dialog_riscos_event_mouse_click_core(
    P_DIALOG_RISCOS_EVENT_MOUSE_CLICK p_dialog_riscos_event_mouse_click)
{
    const WimpMouseClickEvent * const p_mouse_click = &p_dialog_riscos_event_mouse_click->mouse_click;
    const P_DIALOG p_dialog = p_dialog_from_h_dialog(p_dialog_riscos_event_mouse_click->h_dialog);
    DIALOG_CONTROL_ID dialog_control_id;
    STATUS status = STATUS_OK;

    if(NULL == p_dialog)
    {
        assert0();
        return(STATUS_OK);
    }

    if((dialog_control_id = dialog_riscos_scan_controls_in(p_dialog, &p_dialog->ictls, p_mouse_click->icon_handle)) != 0)
    {
        const P_DIALOG_ICTL p_dialog_ictl = p_dialog_ictl_from_control_id(p_dialog, dialog_control_id);
        const wimp_i hit_icon_handle = p_mouse_click->icon_handle;

        PTR_ASSERT(p_dialog_ictl);

        if(!p_dialog_ictl->bits.disabled && !p_dialog_ictl->bits.nobbled)
        {
            switch(p_dialog_ictl->dialog_control_type)
            {
            default:
#if CHECKING
            case DIALOG_CONTROL_LIST_TEXT:
            case DIALOG_CONTROL_LIST_S32:
                /* these controls live in separate windows so we shouln't have got events for them here */

            case DIALOG_CONTROL_GROUPBOX:
                /* not expected here */
                assert0();

                /*FALLTHRU*/

            case DIALOG_CONTROL_STATICPICTURE:
            case DIALOG_CONTROL_STATICTEXT:
            case DIALOG_CONTROL_STATICFRAME:
#endif
                break;

            case DIALOG_CONTROL_PUSHBUTTON:
            case DIALOG_CONTROL_PUSHPICTURE:
                switch(p_mouse_click->buttons)
                {
                case Wimp_MouseButtonSingleSelect:
                case Wimp_MouseButtonSingleAdjust:
                    status = dialog_click_pushbutton(p_dialog, p_dialog_ictl, (p_mouse_click->buttons == Wimp_MouseButtonSingleAdjust), 0);
                    break;
                }
                break;

            case DIALOG_CONTROL_RADIOBUTTON:
            case DIALOG_CONTROL_RADIOPICTURE:
                switch(p_mouse_click->buttons)
                {
                case Wimp_MouseButtonSingleSelect:
                case Wimp_MouseButtonSingleAdjust:
                    status = dialog_click_radiobutton(p_dialog, p_dialog_ictl);
                    break;
                }
                break;

            case DIALOG_CONTROL_CHECKBOX:
            case DIALOG_CONTROL_CHECKPICTURE:
                switch(p_mouse_click->buttons)
                {
                case Wimp_MouseButtonSingleSelect:
                case Wimp_MouseButtonSingleAdjust:
                    status = dialog_click_checkbox(p_dialog, p_dialog_ictl);
                    break;
                }
                break;

#ifdef DIALOG_HAS_TRISTATE
            case DIALOG_CONTROL_TRISTATE:
                switch(p_mouse_click->buttons)
                {
                case Wimp_MouseButtonSingleSelect:
                case Wimp_MouseButtonSingleAdjust:
                    status = dialog_click_tristate(p_dialog, p_dialog_ictl);
                    break;
                }
                break;
#endif

            case DIALOG_CONTROL_EDIT:
                status = dialog_riscos_mlec_event_Mouse_Click(p_dialog, p_dialog_ictl, &p_dialog_ictl->data.edit.edit_xx, p_mouse_click);
                break;

            case DIALOG_CONTROL_BUMP_S32:
            case DIALOG_CONTROL_BUMP_F64:
                if(hit_icon_handle == p_dialog_ictl->riscos.dwi[0].icon_handle)
                    status = dialog_riscos_mlec_event_Mouse_Click(p_dialog, p_dialog_ictl, &p_dialog_ictl->data.bump_xx.edit_xx, p_mouse_click);
                else if((hit_icon_handle == p_dialog_ictl->riscos.dwi[1].icon_handle) || (hit_icon_handle == p_dialog_ictl->riscos.dwi[2].icon_handle))
                {
                    switch(p_mouse_click->buttons)
                    {
                    case Wimp_MouseButtonSingleSelect:
                    case Wimp_MouseButtonSingleAdjust:
                        {
                        BOOL adjust_clicked = (p_mouse_click->buttons == Wimp_MouseButtonSingleAdjust); /* NB. get 0 or 1 answer for subsequent bitwise EOR */
                        BOOL inc_clicked    = (hit_icon_handle == p_dialog_ictl->riscos.dwi[1].icon_handle);
                        status = dialog_click_bump_xx(p_dialog, p_dialog_ictl, inc_clicked ^ adjust_clicked);
                        break;
                        }
                    }
                }
                break;

            case DIALOG_CONTROL_COMBO_TEXT:
            case DIALOG_CONTROL_COMBO_S32:
                if(hit_icon_handle == p_dialog_ictl->riscos.dwi[0].icon_handle)
                    status = dialog_riscos_mlec_event_Mouse_Click(p_dialog, p_dialog_ictl, &p_dialog_ictl->data.combo_xx.edit_xx, p_mouse_click);
                else if(hit_icon_handle == p_dialog_ictl->riscos.dwi[1].icon_handle)
                {
                    switch(p_mouse_click->buttons)
                    {
                    case Wimp_MouseButtonSingleSelect:
                    case Wimp_MouseButtonMenu:
                 /* case Wimp_MouseButtonSingleAdjust: */
                        status = dialog_riscos_combobox_dropdown(p_dialog, p_dialog_ictl);
                        break;
                    }
                }
                break;

            case DIALOG_CONTROL_USER:
                {
                DIALOG_MSG_CTL_USER_MOUSE dialog_msg_ctl_user_mouse;
                P_PROC_DIALOG_EVENT p_proc_client;

                msgclr(dialog_msg_ctl_user_mouse);

                switch(p_mouse_click->buttons)
                {
                default:
                    myassert1(TEXT("wierd button click for USER: ") U32_XTFMT, p_mouse_click->buttons);
                    return(STATUS_OK);

                case Wimp_MouseButtonSingleSelect:
                    dialog_msg_ctl_user_mouse.click = DIALOG_MSG_USER_MOUSE_CLICK_LEFT_SINGLE;
                    break;

                case Wimp_MouseButtonSingleAdjust:
                    dialog_msg_ctl_user_mouse.click = DIALOG_MSG_USER_MOUSE_CLICK_RIGHT_SINGLE;
                    break;

                case Wimp_MouseButtonDragSelect:
                    dialog_msg_ctl_user_mouse.click = DIALOG_MSG_USER_MOUSE_CLICK_LEFT_DRAG;
                    break;

                case Wimp_MouseButtonDragAdjust:
                    dialog_msg_ctl_user_mouse.click = DIALOG_MSG_USER_MOUSE_CLICK_RIGHT_DRAG;
                    break;

                case Wimp_MouseButtonSelect:
                    dialog_msg_ctl_user_mouse.click = DIALOG_MSG_USER_MOUSE_CLICK_LEFT_DOUBLE;
                    break;

                case Wimp_MouseButtonAdjust:
                    dialog_msg_ctl_user_mouse.click = DIALOG_MSG_USER_MOUSE_CLICK_RIGHT_DOUBLE;
                    break;
                }

                /* always processed */
                status = STATUS_OK;

                if(NULL != (p_proc_client = dialog_find_handler(p_dialog, p_dialog_ictl->dialog_control_id, &dialog_msg_ctl_user_mouse.client_handle)))
                {
                    DIALOG_MSG_CTL_HDR_from_dialog_ictl(dialog_msg_ctl_user_mouse, p_dialog, p_dialog_ictl);

                    dialog_msg_ctl_user_mouse.riscos.window_handle = p_dialog->hwnd;
                    void_WrapOsErrorReporting(wimp_get_icon_state_with_bitset(dialog_msg_ctl_user_mouse.riscos.window_handle, hit_icon_handle, &dialog_msg_ctl_user_mouse.riscos.icon));

                    status = dialog_call_client(p_dialog, DIALOG_MSG_CODE_CTL_USER_MOUSE, &dialog_msg_ctl_user_mouse, p_proc_client);
                }

                break;
                }
            }
        }
    }

    return(status);
}

_Check_return_
static STATUS
dialog_riscos_event_mouse_click(
    P_DIALOG_RISCOS_EVENT_MOUSE_CLICK p_dialog_riscos_event_mouse_click)
{
    STATUS status = dialog_riscos_event_mouse_click_core(p_dialog_riscos_event_mouse_click);

    status_assert(status);

    if(status_fail(status))
    {
        /* report error locally */
        reperr_null(status);

        status = STATUS_FAIL;
    }

    return(status);
}

_Check_return_
static STATUS
dialog_riscos_event_key_pressed(
    P_DIALOG_RISCOS_EVENT_KEY_PRESSED p_dialog_riscos_event_key_pressed)
{
    P_DIALOG p_dialog;
    STATUS status;

    if(NULL == (p_dialog = p_dialog_from_h_dialog(p_dialog_riscos_event_key_pressed->h_dialog)))
    {
        assert0();
        return(STATUS_OK);
    }

    status_assert(status = dialog_riscos_Key_Pressed(p_dialog, kmap_convert(p_dialog_riscos_event_key_pressed->key_pressed.key_code)));

    p_dialog_riscos_event_key_pressed->processed = status_done(status);

    return(status);
}

/*
DIALOG_RISCOS_EVENT_CODE_POINTER_ENTER
*/

_Check_return_
static STATUS
dialog_riscos_event_pointer_enter(
    P_DIALOG_RISCOS_EVENT_POINTER_ENTER p_dialog_riscos_event_pointer_enter)
{
    const H_DIALOG h_dialog = p_dialog_riscos_event_pointer_enter->h_dialog;
    const P_DIALOG p_dialog = p_dialog_from_h_dialog(h_dialog);
    S32 enter = p_dialog_riscos_event_pointer_enter->enter;

    if(NULL == p_dialog)
    {
        assert0();
        return(STATUS_OK);
    }

    dialog_null_statics.pointer_info.window_handle = 0; /* invalidate mouse state cache */

    if(enter)
    {
        /* start tracking. won't ever occur if a drag is active */
        if(!p_dialog->has_nulls)
        {
            trace_1(TRACE_OUT | TRACE_ANY, TEXT("dialog_riscos_event_pointer_enter(docno=%d) - start tracking - *** null_events_start()"), p_dialog->docno);
            if(status_ok(status_wrap(null_events_start(p_dialog->docno, (T5_MESSAGE) DIALOG_CMD_CODE_NULL_EVENT, object_dialog, (CLIENT_HANDLE) h_dialog))))
                p_dialog->has_nulls = 1;
        }

        p_dialog->had_pointer = 1;

        /* and give ourseleves a NULL process immediately */
        dialog_riscos_null_event_do(p_dialog);
    }
    else if(p_dialog->had_pointer)
    {
        /* after a winx_drag_box() the Window Manager sends us a PTRLEAVE, which we ignore at this moment, else revert to normal */
        if(!winx_drag_active())
        {
            host_set_pointer_shape(POINTER_DEFAULT);

            if(DOCNO_NONE != p_dialog->docno)
                status_line_clear(p_docu_from_docno(p_dialog->docno), STATUS_LINE_LEVEL_DIALOG_CONTROLS);

            /* and end tracking */
            if(p_dialog->has_nulls)
            {
                p_dialog->has_nulls = 0;
                trace_1(TRACE_OUT | TRACE_ANY, TEXT("dialog_riscos_event_pointer_enter(docno=%d) - stop tracking - *** null_events_stop()"), p_dialog->docno);
                null_events_stop(p_dialog->docno, (T5_MESSAGE) DIALOG_CMD_CODE_NULL_EVENT, object_dialog, (CLIENT_HANDLE) h_dialog);
            }

            p_dialog->had_pointer = 0;
        }
    }

    return(STATUS_OK);
}

/******************************************************************************
*
* client has asked us to redraw any bits of window and controls we own
*
******************************************************************************/

_Check_return_
static STATUS
dialog_riscos_event_redraw_window(
    P_DIALOG_RISCOS_EVENT_REDRAW_WINDOW p_dialog_riscos_event_redraw_window)
{
    DIALOG_CONTROL_REDRAW dialog_control_redraw;
    STATUS status;

    if(NULL == (dialog_control_redraw.p_dialog = p_dialog_from_h_dialog(p_dialog_riscos_event_redraw_window->h_dialog)))
    {
        assert0();
        return(STATUS_OK);
    }

    if(NULL == (dialog_control_redraw.p_dialog = p_dialog_from_h_dialog(dialog_control_redraw.p_dialog->h_dialog)))
    {
        assert0();
        return(STATUS_OK);
    }

    dialog_control_redraw.dialog_riscos_redraw_window.redraw_context = p_dialog_riscos_event_redraw_window->redraw_context;

    dialog_control_redraw.dialog_riscos_redraw_window.redraw_window_block.window_handle = p_dialog_riscos_event_redraw_window->redraw_context.riscos.hwnd;

    {
    GDI_RECT gdi_rect;
    if(!status_done(gdi_rect_from_pixit_rect_and_context(&gdi_rect, &p_dialog_riscos_event_redraw_window->area_rect, &p_dialog_riscos_event_redraw_window->redraw_context)))
    {
        assert0();
        return(STATUS_OK);
    }
    dialog_control_redraw.dialog_riscos_redraw_window.redraw_window_block.visible_area.xmin = gdi_rect.tl.x;
    dialog_control_redraw.dialog_riscos_redraw_window.redraw_window_block.visible_area.ymin = gdi_rect.br.y;
    dialog_control_redraw.dialog_riscos_redraw_window.redraw_window_block.visible_area.xmax = gdi_rect.br.x;
    dialog_control_redraw.dialog_riscos_redraw_window.redraw_window_block.visible_area.ymax = gdi_rect.tl.y;
    } /*block*/

    dialog_control_redraw.dialog_riscos_redraw_window.redraw_window_block.redraw_area = * (const BBox *) &p_dialog_riscos_event_redraw_window->redraw_context.riscos.host_machine_clip_box;

    dialog_control_redraw.p_ictl_group = &dialog_control_redraw.p_dialog->ictls;

    status = dialog_riscos_redraw_controls_in(&dialog_control_redraw);

    return(status);
}

/*
DIALOG_RISCOS_EVENT_CODE_USER_DRAG
*/

_Check_return_
static STATUS
dialog_riscos_event_user_drag(
    P_DIALOG_RISCOS_EVENT_USER_DRAG p_dialog_riscos_event_user_drag)
{
    const H_DIALOG h_dialog = p_dialog_riscos_event_user_drag->h_dialog;
    P_DIALOG p_dialog;

    if(NULL == (p_dialog = p_dialog_from_h_dialog(h_dialog)))
    {
        assert0();
        return(STATUS_OK);
    }

    if(dragging_file_icon)
    {
        CLIENT_HANDLE client_handle;
        const P_PROC_DIALOG_EVENT p_proc_client = dialog_main_handler(p_dialog, &client_handle);

        dragging_file_icon = FALSE;

        if(NULL != p_proc_client)
        {
            DIALOG_MSG_RISCOS_DRAG_ENDED dialog_msg_riscos_drag_ended;

            dialog_msg_riscos_drag_ended.h_dialog = p_dialog->h_dialog;
            dialog_msg_riscos_drag_ended.client_handle = client_handle;

            void_WrapOsErrorReporting(wimp_get_pointer_info(&dialog_msg_riscos_drag_ended.mouse));

            status_assert(dialog_call_client(p_dialog, DIALOG_MSG_CODE_RISCOS_DRAG_ENDED, &dialog_msg_riscos_drag_ended, p_proc_client));
        }

        return(STATUS_OK);
    }

    /* give ourselves a NULL process immediately to reset pointer etc. */
    dialog_riscos_null_event_do(p_dialog);

    /* don't seem to have to tell mlec about drag end */

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_riscos_event_resize(
    P_DIALOG_RISCOS_EVENT_RESIZE p_dialog_riscos_event_resize)
{
    const H_DIALOG h_dialog = p_dialog_riscos_event_resize->h_dialog;
    P_DIALOG p_dialog;
    PIXIT_SIZE pixit_size;
    BITMAP(changed, 4);

    bitmap_clear(changed, N_BITS_ARG(4));

    if(NULL == (p_dialog = p_dialog_from_h_dialog(h_dialog)))
    {
        assert0();
        return(STATUS_OK);
    }

    pixit_size.cx = p_dialog_riscos_event_resize->size.x * PIXITS_PER_RISCOS;
    pixit_size.cy = p_dialog_riscos_event_resize->size.y * PIXITS_PER_RISCOS;

    if(p_dialog->pixit_size.cx != pixit_size.cx)
    {
        p_dialog->pixit_size.cx = pixit_size.cx;

        bitmap_bit_set(changed, DIALOG_RELATIVE_BIT_R, N_BITS_ARG(4));
    }

    if(p_dialog->pixit_size.cy != pixit_size.cy)
    {
        p_dialog->pixit_size.cy = pixit_size.cy;

        bitmap_bit_set(changed, DIALOG_RELATIVE_BIT_B, N_BITS_ARG(4));
    }

    if(bitmap_any(changed, N_BITS_ARG(4)))
    {
        dialog_control_rect_changed(p_dialog, DIALOG_CONTROL_WINDOW, changed);
    }

    return(STATUS_OK);
}

/* ------------------------------------------------------------------------------------------------- */

_Check_return_
static STATUS
dialog_riscos_event_lbn_click(
    _InVal_     DIALOG_MESSAGE dialog_message,
    /*_Inout_*/ P_ANY p_data)
{
    P_DIALOG_MSG_LBN_CLK p_dialog_msg_lbn_clk = (P_DIALOG_MSG_LBN_CLK) p_data;
    S32 selected_item = p_dialog_msg_lbn_clk->selected_item;
    P_DIALOG p_dialog;
    P_DIALOG_ICTL p_dialog_ictl;
    P_RI_LBOX_HANDLE p_ri_lbox_handle;

    /* find where he comes from */
    p_ri_lbox_handle = dialog_riscos_event_lbn_scan_ictls_for_owner(&p_dialog, &p_dialog_ictl, p_dialog_msg_lbn_clk->lbox_handle, p_dialog_msg_lbn_clk->client_handle);

    if(NULL == p_ri_lbox_handle)
    {
        assert0();
        return(STATUS_OK);
    }

    switch(dialog_message)
    {
    default: default_unhandled();
#if CHECKING
    case DIALOG_MSG_CODE_LBN_SGLCLK:
    case DIALOG_MSG_CODE_LBN_SELCHANGE:
#endif
        {
        /* reflect selection from list box into state, and for combos, the edit part of the control */
        DIALOG_CMD_CTL_STATE_SET dialog_cmd_ctl_state_set;

        msgclr(dialog_cmd_ctl_state_set);

        dialog_cmd_ctl_state_set.h_dialog = p_dialog->h_dialog;
        dialog_cmd_ctl_state_set.dialog_control_id = p_dialog_ictl->dialog_control_id;
        dialog_cmd_ctl_state_set.bits = DIALOG_STATE_SET_ALTERNATE;

        dialog_cmd_ctl_state_set.state.list_text.itemno = selected_item;

        status_assert(object_call_DIALOG(DIALOG_CMD_CODE_CTL_STATE_SET, &dialog_cmd_ctl_state_set));

#if 1 /* SKS 15apr93 see if this keeps MRJC happy. no it can't */
        switch(p_dialog_ictl->dialog_control_type)
        {
        default:
            break;

        case DIALOG_CONTROL_COMBO_S32:
        case DIALOG_CONTROL_COMBO_TEXT:
            if(dialog_message == DIALOG_MSG_CODE_LBN_SGLCLK)
            {
#if CHECKING
                {
                P_DIALOG db_p_dialog;
                P_DIALOG_ICTL db_p_dialog_ictl;
                P_RI_LBOX_HANDLE db_p_ri_lbox_handle;

                /* find where he comes from */
                db_p_ri_lbox_handle = dialog_riscos_event_lbn_scan_ictls_for_owner(&db_p_dialog, &db_p_dialog_ictl, p_dialog_msg_lbn_clk->lbox_handle, p_dialog_msg_lbn_clk->client_handle);

                assert(db_p_ri_lbox_handle == p_ri_lbox_handle);
                } /*block*/
#endif

                /* on the assumption that this hasn't moved! */
                ri_lbox_dispose(p_ri_lbox_handle);
            }
            break;
        }
#endif

        break;
        }

    case DIALOG_MSG_CODE_LBN_DBLCLK:
        {
        switch(p_dialog_ictl->dialog_control_type)
        {
        default: default_unhandled();
#if CHECKING
        case DIALOG_CONTROL_LIST_S32:
        case DIALOG_CONTROL_LIST_TEXT:
#endif
            { /* a double click means OK to the containing dialog */
            DIALOG_CMD_DEFPUSHBUTTON dialog_cmd_defpushbutton;
            dialog_cmd_defpushbutton.h_dialog = p_dialog->h_dialog;
            dialog_cmd_defpushbutton.double_dialog_control_id = p_dialog_ictl->dialog_control_id;
            status_assert(object_call_DIALOG(DIALOG_CMD_CODE_DEFPUSHBUTTON, &dialog_cmd_defpushbutton));
            break;
            }

        case DIALOG_CONTROL_COMBO_S32:
        case DIALOG_CONTROL_COMBO_TEXT:
#if CHECKING
            {
            P_DIALOG db_p_dialog;
            P_DIALOG_ICTL db_p_dialog_ictl;
            P_RI_LBOX_HANDLE db_p_ri_lbox_handle;

            /* find where he comes from */
            db_p_ri_lbox_handle = dialog_riscos_event_lbn_scan_ictls_for_owner(&db_p_dialog, &db_p_dialog_ictl, p_dialog_msg_lbn_clk->lbox_handle, p_dialog_msg_lbn_clk->client_handle);

            assert(db_p_ri_lbox_handle == p_ri_lbox_handle);
            } /*block*/
#endif

            /* on the assumption that this hasn't moved! */
            ri_lbox_dispose(p_ri_lbox_handle);
            break;
        }

        break;
        }
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_riscos_event_lbn_destroy(
    /*_Inout_*/ P_ANY p_data)
{
    P_DIALOG_MSG_LBN_DESTROY p_dialog_msg_lbn_destroy = (P_DIALOG_MSG_LBN_DESTROY) p_data;
    P_DIALOG p_dialog;
    P_DIALOG_ICTL p_dialog_ictl;
    P_RI_LBOX_HANDLE p_ri_lbox_handle;

    /* find where he comes from */
    p_ri_lbox_handle = dialog_riscos_event_lbn_scan_ictls_for_owner(&p_dialog, &p_dialog_ictl, p_dialog_msg_lbn_destroy->lbox_handle, p_dialog_msg_lbn_destroy->client_handle);

    if(NULL == p_ri_lbox_handle)
    {
        assert0();
        return(STATUS_OK);
    }

    ri_lbox_dispose(p_ri_lbox_handle);

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_riscos_event_lbn_focus(
    /*_Inout_*/ P_ANY p_data)
{
    P_DIALOG_MSG_LBN_FOCUS p_dialog_msg_lbn_focus = (P_DIALOG_MSG_LBN_FOCUS) p_data;
    P_DIALOG p_dialog;
    P_DIALOG_ICTL p_dialog_ictl;
    P_RI_LBOX_HANDLE p_ri_lbox_handle;

    /* find where he comes from */
    p_ri_lbox_handle = dialog_riscos_event_lbn_scan_ictls_for_owner(&p_dialog, &p_dialog_ictl,p_dialog_msg_lbn_focus->lbox_handle, p_dialog_msg_lbn_focus->client_handle);

    if(NULL == p_ri_lbox_handle)
    {
        assert0();
        return(STATUS_OK);
    }

    dialog_current_set(p_dialog, p_dialog_ictl->dialog_control_id, 0);

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_riscos_event_lbn_key(
    /*_Inout_*/ P_ANY p_data)
{
    P_DIALOG_MSG_LBN_KEY p_dialog_msg_lbn_key = (P_DIALOG_MSG_LBN_KEY) p_data;
    P_DIALOG p_dialog;
    P_DIALOG_ICTL p_dialog_ictl;
    P_RI_LBOX_HANDLE p_ri_lbox_handle;
    STATUS status;

    /* find where he comes from */
    p_ri_lbox_handle = dialog_riscos_event_lbn_scan_ictls_for_owner(&p_dialog, &p_dialog_ictl,p_dialog_msg_lbn_key->lbox_handle, p_dialog_msg_lbn_key->client_handle);

    if(NULL == p_ri_lbox_handle)
    {
        assert0();
        return(STATUS_OK);
    }

    status_assert(status = dialog_riscos_Key_Pressed(p_dialog, p_dialog_msg_lbn_key->kmap_code));

    p_dialog_msg_lbn_key->processed = status_done(status);

    return(STATUS_OK);
}

/******************************************************************************
*
* scan DIALOG_ICTLs to find the list box that owns this handle
*
******************************************************************************/

_Check_return_
_Ret_maybenull_
static P_DIALOG_ICTL
dialog_riscos_event_lbn_scan_ictls_for_owner_in(
    P_DIALOG_ICTL_GROUP p_dialog_ictl_group,
    RI_LBOX_HANDLE handle)
{
    ARRAY_INDEX i;

    for(i = 0; i < n_ictls_from_group(p_dialog_ictl_group); ++i)
    {
        P_DIALOG_ICTL p_dialog_ictl = p_dialog_ictl_from(p_dialog_ictl_group, i);

        switch(p_dialog_ictl->dialog_control_type)
        {
        default:
            continue;

        case DIALOG_CONTROL_GROUPBOX:
            if(NULL != (p_dialog_ictl = dialog_riscos_event_lbn_scan_ictls_for_owner_in(&p_dialog_ictl->data.groupbox.ictls, handle)))
                return(p_dialog_ictl);
            break;

        case DIALOG_CONTROL_LIST_S32:
        case DIALOG_CONTROL_LIST_TEXT:
            if(p_dialog_ictl->data.list_xx.list_xx.riscos.lbox == handle)
                return(p_dialog_ictl);
            break;

        case DIALOG_CONTROL_COMBO_S32:
        case DIALOG_CONTROL_COMBO_TEXT:
            if(p_dialog_ictl->data.combo_xx.list_xx.riscos.lbox == handle)
                return(p_dialog_ictl);
        }
    }

    return(NULL);
}

_Check_return_
_Ret_maybenull_
static P_RI_LBOX_HANDLE
dialog_riscos_event_lbn_scan_ictls_for_owner(
    /*out*/ P_P_DIALOG p_p_dialog,
    /*out*/ P_P_DIALOG_ICTL p_p_dialog_ictl,
    RI_LBOX_HANDLE lbox_handle,
    _InVal_     CLIENT_HANDLE client_handle)
{
    const H_DIALOG h_dialog = (H_DIALOG) client_handle;
    P_DIALOG p_dialog = p_dialog_from_h_dialog(h_dialog);
    P_DIALOG_ICTL p_dialog_ictl;
    P_RI_LBOX_HANDLE p_ri_lbox_handle;

    /* scan DIALOG_ICTLs for handle (will only find combo or list) */
    if(NULL != (p_dialog_ictl = dialog_riscos_event_lbn_scan_ictls_for_owner_in(&p_dialog->ictls, lbox_handle)))
    {
        switch(p_dialog_ictl->dialog_control_type)
        {
        default: default_unhandled();
#if CHECKING
        case DIALOG_CONTROL_LIST_S32:
        case DIALOG_CONTROL_LIST_TEXT:
#endif
            p_ri_lbox_handle = &p_dialog_ictl->data.list_xx.list_xx.riscos.lbox;
            break;

        case DIALOG_CONTROL_COMBO_S32:
        case DIALOG_CONTROL_COMBO_TEXT:
            p_ri_lbox_handle = &p_dialog_ictl->data.combo_xx.list_xx.riscos.lbox;
            break;
        }
    }
    else
    {
        /* there is a wee failure */
        assert0();
        p_ri_lbox_handle = NULL;
    }

    *p_p_dialog = p_dialog;
    *p_p_dialog_ictl = p_dialog_ictl;

    return(p_ri_lbox_handle);
}

#define DIALOG_MESSAGE_LBN_OFFSET(dialog_message) ( \
    ((U32) (dialog_message) - (U32) DIALOG_MSG_CODE_LBN_STT) )

_Check_return_
static STATUS
dialog_riscos_event_lbn(
    _InVal_     DIALOG_MESSAGE dialog_message,
    P_ANY p_data)
{
    switch(DIALOG_MESSAGE_LBN_OFFSET(dialog_message))
    {

    case DIALOG_MESSAGE_LBN_OFFSET(DIALOG_MSG_CODE_LBN_DESTROY):
        return(dialog_riscos_event_lbn_destroy(p_data));

    case DIALOG_MESSAGE_LBN_OFFSET(DIALOG_MSG_CODE_LBN_DBLCLK):
    case DIALOG_MESSAGE_LBN_OFFSET(DIALOG_MSG_CODE_LBN_SGLCLK):
    case DIALOG_MESSAGE_LBN_OFFSET(DIALOG_MSG_CODE_LBN_SELCHANGE):
        return(dialog_riscos_event_lbn_click(dialog_message, p_data));

    case DIALOG_MESSAGE_LBN_OFFSET(DIALOG_MSG_CODE_LBN_FOCUS):
        return(dialog_riscos_event_lbn_focus(p_data));

    case DIALOG_MESSAGE_LBN_OFFSET(DIALOG_MSG_CODE_LBN_KEY):
        return(dialog_riscos_event_lbn_key(p_data));

    default: default_unhandled();
        return(STATUS_OK);
    }
}

#define DIALOG_MESSAGE_RISCOS_EVENT_OFFSET(dialog_message) ( \
    ((U32) (dialog_message) - (U32) DIALOG_RISCOS_EVENT_CODE_STT) )

_Check_return_
extern STATUS
dialog_riscos_event(
    _InVal_     DIALOG_MESSAGE dialog_message,
    P_ANY p_data)
{
    switch(DIALOG_MESSAGE_RISCOS_EVENT_OFFSET(dialog_message))
    {
    case DIALOG_MESSAGE_RISCOS_EVENT_OFFSET(DIALOG_RISCOS_EVENT_CODE_REDRAW_WINDOW):
        return(dialog_riscos_event_redraw_window(p_data));

    case DIALOG_MESSAGE_RISCOS_EVENT_OFFSET(DIALOG_RISCOS_EVENT_CODE_MOUSE_CLICK):
        return(dialog_riscos_event_mouse_click(p_data));

    case DIALOG_MESSAGE_RISCOS_EVENT_OFFSET(DIALOG_RISCOS_EVENT_CODE_KEY_PRESSED):
        return(dialog_riscos_event_key_pressed(p_data));

    case DIALOG_MESSAGE_RISCOS_EVENT_OFFSET(DIALOG_RISCOS_EVENT_CODE_USER_DRAG):
        return(dialog_riscos_event_user_drag(p_data));

    case DIALOG_MESSAGE_RISCOS_EVENT_OFFSET(DIALOG_RISCOS_EVENT_CODE_RESIZE):
        return(dialog_riscos_event_resize(p_data));

    case DIALOG_MESSAGE_RISCOS_EVENT_OFFSET(DIALOG_RISCOS_EVENT_CODE_POINTER_ENTER):
        return(dialog_riscos_event_pointer_enter(p_data));

    default:
        return(dialog_riscos_event_lbn(dialog_message, p_data));
    }
}

_Check_return_
static STATUS
dialog_riscos_icon_create(
    P_DIALOG p_dialog,
    _In_        const WimpIconBlockWithBitset * const p_icon,
    _OutRef_    P_DIALOG_WIMP_I p_i)
{
    WimpCreateIconBlockWithBitset icreate;

    icreate.window_handle = p_dialog->hwnd;
    icreate.icon = *p_icon;

    p_i->icon_handle = BAD_WIMP_I;

    p_i->redrawn_by_wimp = (icreate.icon.flags.bits.redraw == 0);

    if( (p_icon->bbox.xmin < p_icon->bbox.xmax) &&
        (p_icon->bbox.ymin < p_icon->bbox.ymax) )
        if(WrapOsErrorReporting(wimp_create_icon_with_bitset(&icreate, &p_i->icon_handle)))
            return(status_nomem());

    return(STATUS_OK);
}

/* Gee thanks a bundle Neil - deleting an icon doesn't cause a redraw at all */

static _kernel_oserror *
dialog_riscos_icon_delete_redraw(
    _InVal_     wimp_w window_handle,
    _Inout_     wimp_i * const p_icon_handle)
{
    WimpIconBlockWithBitset icon;
    _kernel_oserror * e;

    if(BAD_WIMP_I == *p_icon_handle)
        return(NULL);

    if(NULL == (e = wimp_get_icon_state_with_bitset(window_handle, *p_icon_handle, &icon)))
    {
        if(NULL == (e = wimp_force_redraw_BBox(window_handle, &icon.bbox)))
            e = wimp_dispose_icon(window_handle, p_icon_handle);
    }

    return(e);
}

static void
dialog_riscos_icon_delete(
    P_DIALOG p_dialog,
    _InoutRef_  P_DIALOG_WIMP_I p_i)
{
    /* don't blow Window Manager's little brain with lots of rectangles on view creation/deletion */
    if(p_dialog->big_mods)
    {
        void_WrapOsErrorReporting(wimp_dispose_icon(p_dialog->hwnd, &p_i->icon_handle));
        return;
    }

    void_WrapOsErrorReporting(dialog_riscos_icon_delete_redraw(p_dialog->hwnd, &p_i->icon_handle)); /* itself copes with BAD_WIMP_I */
}

typedef struct wimp__handles_and_flags
{
    wimp_w window_handle;
    wimp_i icon_handle;
    WimpIconFlagsWithBitset flags_v;
    WimpIconFlagsWithBitset flags_m;
}
wimp__handles_and_flags;

static void
dialog_riscos_icon_enable(
    P_DIALOG p_dialog,
    _InRef_     PC_DIALOG_WIMP_I p_i,
    _InVal_     BOOL disabled)
{
    WimpIconBlockWithBitset info;

    void_WrapOsErrorReporting(wimp_get_icon_state_with_bitset(p_dialog->hwnd, p_i->icon_handle, &info));

    if((disabled && !info.flags.bits.disabled) || (!disabled && info.flags.bits.disabled))
    {
        wimp__handles_and_flags b;
        _kernel_swi_regs rs;

        b.window_handle = p_dialog->hwnd;
        b.icon_handle = p_i->icon_handle;

        b.flags_v.u32 = 0; /*eor*/
        b.flags_v.bits.disabled = 1; /*eor*/

        b.flags_m.u32 = 0; /*bic*/

        rs.r[0] = 0;
        rs.r[1] = (int) &b;

        void_WrapOsErrorReporting(_kernel_swi(/*Wimp_SetIconState*/ 0x000400CD, &rs, &rs));
    }
}

/******************************************************************************
*
* NB. recreates icon but DOES NOT redraw it
*
******************************************************************************/

_Check_return_
extern STATUS
dialog_riscos_icon_recreate(
    P_DIALOG p_dialog,
    _In_        const WimpIconBlockWithBitset * const p_icon,
    _InoutRef_  P_DIALOG_WIMP_I p_i)
{
    if(p_i->icon_handle == BAD_WIMP_I)
        return(STATUS_OK);

    void_WrapOsErrorReporting(wimp_dispose_icon(p_dialog->hwnd, &p_i->icon_handle));

    if( (p_icon->bbox.xmin < p_icon->bbox.xmax) &&
        (p_icon->bbox.ymin < p_icon->bbox.ymax) )
    {
        WimpCreateIconBlockWithBitset icreate;

        icreate.window_handle = p_dialog->hwnd;
        icreate.icon = *p_icon;

        if(WrapOsErrorReporting(wimp_create_icon_with_bitset(&icreate, &p_i->icon_handle)))
            return(status_nomem());
    }

    return(STATUS_OK);
}

_Check_return_ _Success_(status_ok(return))
extern STATUS
dialog_riscos_icon_recreate_prepare(
    P_DIALOG p_dialog,
    _Out_       WimpIconBlockWithBitset * const p_icon,
    _InRef_     PC_DIALOG_WIMP_I p_i)
{
    if(p_i->icon_handle != BAD_WIMP_I)
        void_WrapOsErrorReporting(wimp_get_icon_state_with_bitset(p_dialog->hwnd, p_i->icon_handle, p_icon));

    return(STATUS_OK);
}

_Check_return_
extern STATUS
dialog_riscos_icon_recreate_with(
    P_DIALOG p_dialog,
    _Inout_     WimpIconBlockWithBitset * const p_icon,
    _InoutRef_  P_DIALOG_WIMP_I p_i,
    RESOURCE_BITMAP_HANDLE h_bitmap)
{
    if(h_bitmap.i & RESOURCE_BITMAP_HANDLE_RISCOS_BODGE)
    {
        p_icon->data.is.sprite = (P_U8) (h_bitmap.i & ~RESOURCE_BITMAP_HANDLE_RISCOS_BODGE);
    }
    else
    {
        p_icon->data.is.sprite = h_bitmap.p_u8;
    }

    if(p_i->icon_handle != BAD_WIMP_I)
    {
        dialog_riscos_icon_delete(p_dialog, p_i);

        status_return(dialog_riscos_icon_create(p_dialog, p_icon, p_i));
    }

    return(STATUS_OK);
}

static void
dialog_riscos_icon_redraw_for_enable(
    P_DIALOG p_dialog,
    _InRef_     PC_DIALOG_WIMP_I p_i)
{
    WimpIconBlockWithBitset icon;

    void_WrapOsErrorReporting(wimp_get_icon_state_with_bitset(p_dialog->hwnd, p_i->icon_handle, &icon));

    if(p_dialog->riscos.accumulate_box)
    {
        if( p_dialog->riscos.invalid_bbox.xmin > icon.bbox.xmin)
            p_dialog->riscos.invalid_bbox.xmin = icon.bbox.xmin;
        if( p_dialog->riscos.invalid_bbox.ymin > icon.bbox.ymin)
            p_dialog->riscos.invalid_bbox.ymin = icon.bbox.ymin;
        if( p_dialog->riscos.invalid_bbox.xmax < icon.bbox.xmax)
            p_dialog->riscos.invalid_bbox.xmax = icon.bbox.xmax;
        if( p_dialog->riscos.invalid_bbox.ymax < icon.bbox.ymax)
            p_dialog->riscos.invalid_bbox.ymax = icon.bbox.ymax;
    }
    else
    {
        void_WrapOsErrorReporting(wimp_force_redraw_BBox(p_dialog->hwnd, &icon.bbox));
    }
}

extern void
dialog_riscos_icon_redraw_for_encode(
    P_DIALOG p_dialog,
    _InRef_     PC_DIALOG_WIMP_I p_i,
    _In_        FRAMED_BOX_STYLE border_style,
    _InVal_     S32 encode_bits)
{
    WimpIconBlockWithBitset icon;

    if(p_i->icon_handle == BAD_WIMP_I)
        return;

    if(NULL == wimp_get_icon_state_with_bitset(p_dialog->hwnd, p_i->icon_handle, &icon))
    {
        WimpRedrawWindowBlock redraw_window_block;

        redraw_window_block.visible_area = icon.bbox;

        host_framed_BBox_trim(&redraw_window_block.visible_area, border_style);

        if(encode_bits & DIALOG_ENCODE_UPDATE_NOW)
        {
            int wimp_more;

            redraw_window_block.window_handle = p_dialog->hwnd;

            if(NULL != WrapOsErrorReporting(wimp_update_window(&redraw_window_block, &wimp_more)))
                wimp_more = 0;
    
            while(0 != wimp_more)
            {
                dialog_riscos_redraw_core(&redraw_window_block, p_dialog->h_dialog, 1 /*is_update_now*/);

                if(NULL != WrapOsErrorReporting(wimp_get_rectangle(&redraw_window_block, &wimp_more)))
                    wimp_more = 0;
            }
        }
        else if(p_dialog->riscos.accumulate_box)
        {
            if( p_dialog->riscos.invalid_bbox.xmin > redraw_window_block.visible_area.xmin)
                p_dialog->riscos.invalid_bbox.xmin = redraw_window_block.visible_area.xmin;
            if( p_dialog->riscos.invalid_bbox.ymin > redraw_window_block.visible_area.ymin)
                p_dialog->riscos.invalid_bbox.ymin = redraw_window_block.visible_area.ymin;
            if( p_dialog->riscos.invalid_bbox.xmax < redraw_window_block.visible_area.xmax)
                p_dialog->riscos.invalid_bbox.xmax = redraw_window_block.visible_area.xmax;
            if( p_dialog->riscos.invalid_bbox.ymax < redraw_window_block.visible_area.ymax)
                p_dialog->riscos.invalid_bbox.ymax = redraw_window_block.visible_area.ymax;
        }
        else
        {
            void_WrapOsErrorReporting(wimp_force_redraw_BBox(p_dialog->hwnd, &redraw_window_block.visible_area));
        }
    }
}

static BOOL
dialog_riscos_icon_sprite_setup(
    WimpIconBlockWithBitset * p_icon,
    RESOURCE_BITMAP_HANDLE h_bitmap)
{
    if(!h_bitmap.i)
        return(0);

    p_icon->flags.bits.indirect = 1;

    if(h_bitmap.i & RESOURCE_BITMAP_HANDLE_RISCOS_BODGE)
    {
        p_icon->data.is.sprite = (P_U8) (h_bitmap.i & ~RESOURCE_BITMAP_HANDLE_RISCOS_BODGE);
        p_icon->data.is.sprite_area = (void *) 1; /* Window Manager's sprite area */
        p_icon->data.is.sprite_name_length = 1;
    }
    else
    {
        p_icon->data.is.sprite = h_bitmap.p_u8;
        p_icon->data.is.sprite_area = (void *) 1; /* Window Manager's sprite area - shouldn't be needed */
        p_icon->data.is.sprite_name_length = 0;
    }

    return(1);
}

/*ncr*/
extern BOOL
dialog_riscos_icon_text_setup(
    WimpIconBlockWithBitset * p_icon,
    _In_opt_z_  PC_U8Z p_u8)
{
    BOOL not_null = (NULL != p_u8);

    p_icon->flags.bits.indirect = 1;

    p_icon->data.it.buffer = de_const_cast(char *, not_null ? p_u8 : empty_string);
    /*p_icon->data.it.validation = NULL; DO NOT MODIFY THIS SETTING*/
    p_icon->data.it.buffer_size = strlen32p1(p_icon->data.it.buffer); /*CH_NULL*/

    return(not_null);
}

/******************************************************************************
*
* create the icons for a control in this view
*
******************************************************************************/

_Check_return_
static STATUS
dialog_riscos_ictl_create_here(
    P_DIALOG p_dialog,
    P_DIALOG_ICTL p_dialog_ictl,
    P_PIXIT_RECT p_pixit_rect)
{
    STATUS status = STATUS_OK;
    WimpIconBlockWithBitset a_icon, b_icon, c_icon;
    P_DIALOG_WIMP_I p_i_a = NULL; /* receives icon 1 */
    P_DIALOG_WIMP_I p_i_b = NULL; /* receives icon 2 */
    P_DIALOG_WIMP_I p_i_c = NULL; /* receives icon 3 */
    BOOL captioned;

    zero_struct(a_icon);

    /* use same back colour as work area (remember to clear this out if changing colour) */
    a_icon.flags.bits.bg_colour = '\x01';
    a_icon.flags.bits.fg_colour = '\x07';

    a_icon.flags.bits.horz_centre = 1;
    a_icon.flags.bits.vert_centre = 1;

    a_icon.flags.bits.button_type = ButtonType_Click;

    dialog_riscos_box_from_pixit_rect((P_GDI_BOX) &a_icon.bbox, p_pixit_rect);

    /* transform according to desired origin within window */
    a_icon.bbox.xmin -= p_dialog->riscos.gdi_offset_tl.x;
    a_icon.bbox.ymin -= p_dialog->riscos.gdi_offset_tl.y;
    a_icon.bbox.xmax -= p_dialog->riscos.gdi_offset_tl.x;
    a_icon.bbox.ymax -= p_dialog->riscos.gdi_offset_tl.y;

    b_icon = a_icon; /* make copies of these */
    c_icon = a_icon;

    switch(p_dialog_ictl->dialog_control_type)
    {
    default: default_unhandled();
#if CHECKING
        break;

    case DIALOG_CONTROL_GROUPBOX:
#endif
        {
        BOOL logical_group = !p_dialog_ictl->p_dialog_control || p_dialog_ictl->p_dialog_control->bits.logical_group || !p_dialog_ictl->p_dialog_control_data.groupbox || p_dialog_ictl->p_dialog_control_data.groupbox->bits.logical_group;

        if(!logical_group)
        {
            /* first icon is the clickless group border */
            p_i_a = &p_dialog_ictl->riscos.dwi[0];

            a_icon.flags.bits.button_type = ButtonType_Never;

            a_icon.flags.bits.border = 1;

            a_icon.flags.bits.indirect = 1;
            a_icon.flags.bits.text = 1;
            a_icon.data.it.buffer = "";
            a_icon.data.it.validation = "R4"; /* Channel */
            a_icon.data.it.buffer_size = 1;

            /* second icon is the (optional) clickless group name as a title to the group */
            captioned = dialog_riscos_icon_text_setup(&b_icon, p_dialog_ictl->riscos.caption);

            if(captioned)
            {
                GDI_POINT size, prev_size;

                p_i_b = &p_dialog_ictl->riscos.dwi[1];

                b_icon.flags.bits.button_type = ButtonType_Never;

                b_icon.flags.bits.text = 1;
                b_icon.flags.bits.sprite = 1; /* T,S,I -> Window Manager sorts out nicely with patterned backgrounds */ /* Bug: Sprite also drawn if in WM SA */

                b_icon.flags.bits.horz_centre = 0;

                b_icon.bbox.xmin += 16;
                b_icon.bbox.xmax -= 16;

                size.x = 8 + (GDI_COORD) (ui_width_from_tstr(p_dialog_ictl->riscos.caption) / PIXITS_PER_RISCOS);
                {
                GDI_COORD icon_width = BBox_width(&b_icon.bbox);
                if( size.x > icon_width)
                    size.x = icon_width;
                } /*block*/
                size.y = 32;

                prev_size = size;

                size.y += 2 * 4; /* SKS after 1.05 27oct93 needed to make them acceptably large */

                /* adjust as needed */
                b_icon.bbox.xmin -= (size.x - prev_size.x) / 2;

                b_icon.bbox.xmax = b_icon.bbox.xmin + size.x;
                b_icon.bbox.ymin = b_icon.bbox.ymax - size.y;

                /* margin between top of title and top of group rectangle */
                a_icon.bbox.ymax = b_icon.bbox.ymin + BBox_height(&b_icon.bbox) / 2;
            }
        }

        break;
        }

    case DIALOG_CONTROL_STATICTEXT:
        {
        /* icon is the clickless static text */
        p_i_a = &p_dialog_ictl->riscos.dwi[0];

        a_icon.flags.bits.button_type = ButtonType_Never;

        a_icon.flags.bits.redraw = 1;

        if(!p_dialog_ictl->p_dialog_control_data.statictext->bits.centre_text)
        {
            a_icon.flags.bits.horz_centre = 0;

            if(!p_dialog_ictl->p_dialog_control_data.statictext->bits.left_text)
                a_icon.flags.bits.right_just = 1;
        }

        /* always create so we can put in initial null text */
        dialog_riscos_icon_text_setup(&a_icon, p_dialog_ictl->riscos.caption);

        break;
        }

    case DIALOG_CONTROL_STATICFRAME:
        {
        /* icon is the clickless static text */
        p_i_a= &p_dialog_ictl->riscos.dwi[0];

        /* framed border clickless icon */
        a_icon.flags.bits.button_type = ButtonType_Never;

        /* could assert border style == FRAMED_BOX_TROUGH but it always is... */
        a_icon.flags.bits.border = 1;

        a_icon.flags.bits.indirect = 1;
        a_icon.flags.bits.text = 1;
        a_icon.data.it.buffer = "";
        a_icon.data.it.validation = "R2"; /* Slab in */
        a_icon.data.it.buffer_size = 1;

        if(!p_dialog_ictl->p_dialog_control_data.statictext->bits.centre_text)
        {
            a_icon.flags.bits.horz_centre = 0;

            if(!p_dialog_ictl->p_dialog_control_data.statictext->bits.left_text)
                a_icon.flags.bits.right_just = 1;
        }

        /* always create so we can put in initial null text */
        dialog_riscos_icon_text_setup(&a_icon, p_dialog_ictl->riscos.caption);

        break;
        }

    case DIALOG_CONTROL_STATICPICTURE:
        {
        /* icon is the clickless static picture */
        p_i_a = &p_dialog_ictl->riscos.dwi[0];

        a_icon.flags.bits.button_type = ButtonType_Never;

        a_icon.flags.bits.sprite = 1;

        dialog_riscos_icon_sprite_setup(&a_icon, p_dialog_ictl->data.staticpicture.riscos.h_bitmap);

        break;
        }

    case DIALOG_CONTROL_PUSHBUTTON:
        {
        p_i_a = &p_dialog_ictl->riscos.dwi[0];

        a_icon.flags.bits.redraw = 1;

        dialog_riscos_icon_text_setup(&a_icon, p_dialog_ictl->riscos.caption);

        if(p_dialog_ictl->p_dialog_control_data.pushbutton->push_xx.auto_repeat)
            a_icon.flags.bits.button_type = ButtonType_Repeat;
        else
        {
            a_icon.flags.bits.text = 1;

            a_icon.flags.bits.redraw = 0;
            a_icon.flags.bits.border = 1;
            a_icon.flags.bits.filled = 1;

            if((p_dialog_ictl->dialog_control_id == IDOK) || p_dialog_ictl->p_dialog_control_data.pushbutton->push_xx.def_pushbutton)
                a_icon.data.it.validation = "R6,3"; /* Default action */
            else
                a_icon.data.it.validation = "R5,3"; /* Action button */
        }

        break;
        }

    case DIALOG_CONTROL_PUSHPICTURE:
        {
        p_i_a = &p_dialog_ictl->riscos.dwi[0];

        a_icon.flags.bits.redraw = 1;

        dialog_riscos_icon_sprite_setup(&a_icon, p_dialog_ictl->data.pushpicture.riscos.h_bitmap);

        if(p_dialog_ictl->p_dialog_control_data.pushpicture->push_xx.auto_repeat)
            a_icon.flags.bits.button_type = ButtonType_Repeat;

        break;
        }

    case DIALOG_CONTROL_RADIOBUTTON:
        {
        /* first icon is state indicator, */
        p_i_a = &p_dialog_ictl->riscos.dwi[0];

        a_icon.flags.bits.sprite = 1;

        /* second icon is (optional) caption */
        captioned = dialog_riscos_icon_text_setup(&b_icon, p_dialog_ictl->riscos.caption);

        if(captioned)
        {
            if(p_dialog_ictl->p_dialog_control_data.radiobutton->bits.left_text)
                a_icon.bbox.xmin = a_icon.bbox.xmax - p_dialog->riscos.bitmap_radio_off.size.cx;
            else
                a_icon.bbox.xmax = a_icon.bbox.xmin + p_dialog->riscos.bitmap_radio_off.size.cx;
        }

        dialog_riscos_icon_sprite_setup(&a_icon, p_dialog->riscos.bitmap_radio_off.handle);

        if(captioned)
        {
            p_i_b = &p_dialog_ictl->riscos.dwi[1];

            b_icon.flags.bits.redraw = 1; /* you might well think you should use the wimp but don't 'cos it looks shite */

            b_icon.flags.bits.horz_centre = 0;

            if(p_dialog_ictl->p_dialog_control_data.radiobutton->bits.left_text)
                b_icon.bbox.xmax = a_icon.bbox.xmin;
            else
                b_icon.bbox.xmin = a_icon.bbox.xmax; /* let renderer sort out fine positioning */
        }

        break;
        }

    case DIALOG_CONTROL_RADIOPICTURE:
        {
        /* icon is state indicator */
        p_i_a = &p_dialog_ictl->riscos.dwi[0];

        a_icon.flags.bits.redraw = 1;

        assert(p_dialog_ictl->data.radiopicture.riscos.h_bitmap_off.i);
        assert(p_dialog_ictl->data.radiopicture.riscos.h_bitmap_on.i);

        dialog_riscos_icon_sprite_setup(&a_icon, p_dialog_ictl->data.radiopicture.riscos.h_bitmap_off);

        break;
        }

    case DIALOG_CONTROL_CHECKBOX:
        {
        /* first icon is state indicator */
        p_i_a = &p_dialog_ictl->riscos.dwi[0];

        a_icon.flags.bits.sprite = 1;

        /* second icon is (optional) caption */
        captioned = dialog_riscos_icon_text_setup(&b_icon, p_dialog_ictl->riscos.caption);

        if(captioned)
        {
            if(p_dialog_ictl->p_dialog_control_data.checkbox->bits.left_text)
                a_icon.bbox.xmin = a_icon.bbox.xmax - p_dialog->riscos.bitmap_check_off.size.cx;
            else
                a_icon.bbox.xmax = a_icon.bbox.xmin + p_dialog->riscos.bitmap_check_off.size.cx;
        }

        dialog_riscos_icon_sprite_setup(&a_icon, p_dialog->riscos.bitmap_check_off.handle);

        if(captioned)
        {
            p_i_b = &p_dialog_ictl->riscos.dwi[1];

            b_icon.flags.bits.redraw = 1; /* you might well think you should use the wimp but don't 'cos it looks shite */

            b_icon.flags.bits.horz_centre = 0;

            if(p_dialog_ictl->p_dialog_control_data.checkbox->bits.left_text)
                b_icon.bbox.xmax = a_icon.bbox.xmin;
            else
                b_icon.bbox.xmin = a_icon.bbox.xmax; /* let renderer sort out fine positioning */
        }

        break;
        }

    case DIALOG_CONTROL_CHECKPICTURE:
        {
        /* icon is state indicator */
        p_i_a = &p_dialog_ictl->riscos.dwi[0];

        a_icon.flags.bits.redraw = 1;

        assert(p_dialog_ictl->data.checkpicture.riscos.h_bitmap_off.i);
        assert(p_dialog_ictl->data.checkpicture.riscos.h_bitmap_on.i);

        dialog_riscos_icon_sprite_setup(&a_icon, p_dialog_ictl->data.checkpicture.riscos.h_bitmap_off);

        break;
    }

#ifdef DIALOG_HAS_TRISTATE
    case DIALOG_CONTROL_TRISTATE:
        {
        /* first icon is state indicator */
        p_i_a = &p_dialog_ictl->riscos.i;

        a_icon.flags.bits.sprite = 1;

        /* second icon is (optional) caption */
        captioned = dialog_riscos_icon_text_setup(&b_icon, p_dialog_ictl->riscos.caption);

        if(captioned)
        {
            if(p_dialog_ictl->p_dialog_control_data.tristate->bits.left_text)
                a_icon.bbox.xmin = a_icon.bbox.xmax - p_dialog->riscos.bitmap_tristate_on.size.x;
            else
                a_icon.bbox.xmax = a_icon.bbox.xmin + p_dialog->riscos.bitmap_tristate_on.size.x;
        }

        dialog_riscos_icon_sprite_setup(&a_icon, p_dialog->riscos.bitmap_tristate_off.handle);

        if(captioned)
        {
            p_i_b = &p_dialog_ictl->data.tristate.riscos.caption_i;

            b_icon.flags.redraw = 1; /* you might well think you should use the wimp but don't 'cos it looks shite */

            b_icon.flags.horz_centre = 0;

            if(p_dialog_ictl->p_dialog_control_data.tristate->bits.left_text)
                b_icon.bbox.xmax = a_icon.bbox.xmin;
            else
                b_icon.bbox.xmin = a_icon.bbox.xmax; /* let renderer sort out fine positioning */
        }

        break;
        }

    case DIALOG_CONTROL_TRIPICTURE:
        {
        /* icon is state indicator */
        p_i_a = &p_dialog_ictl->riscos.i;

        a_icon.flags.bits.redraw = 1;

        assert(p_dialog_ictl->data.tripicture.riscos.h_bitmap_off.i);
        assert(p_dialog_ictl->data.tripicture.riscos.h_bitmap_on.i);
        assert(p_dialog_ictl->data.tripicture.riscos.h_bitmap_dont_care.i);

        dialog_riscos_icon_sprite_setup(&a_icon, p_dialog_ictl->data.tripicture.riscos.h_bitmap_off);

        break;
    }
#endif

    case DIALOG_CONTROL_EDIT:
        {
        p_i_a = &p_dialog_ictl->riscos.dwi[0];

        a_icon.flags.bits.redraw = 1;

        if(p_dialog_ictl->data.edit.edit_xx.readonly)
            a_icon.flags.bits.button_type = ButtonType_Never;
        else
            a_icon.flags.bits.button_type = ButtonType_DoubleClickDrag;

#ifdef EDIT_XX_SINGLE_LINE_WIMP
        if(0 != p_dialog_ictl->data.edit.edit_xx.riscos.slec_buffer_size)
        {
            a_icon.flags.bits.redraw = 0;

            a_icon.flags.bits.horz_centre = 0;

            a_icon.flags.bits.border = 1;
            a_icon.flags.bits.filled = 1;

            a_icon.flags.bits.bg_colour = '\x00';

            a_icon.flags.bits.indirect = 1;
            a_icon.flags.bits.text = 1;
            a_icon.data.it.buffer = p_dialog_ictl->data.edit.edit_xx.riscos.slec_buffer;
            a_icon.data.it.validation = NULL;
            a_icon.data.it.buffer_length = p_dialog_ictl->data.edit.edit_xx.riscos.slec_buffer_size;

            if(!p_dialog_ictl->data.edit.edit_xx.readonly)
                a_icon.flags.button_type = ButtonType_Writable;
        }
#endif

        break;
        }

    case DIALOG_CONTROL_BUMP_S32:
    case DIALOG_CONTROL_BUMP_F64:
        {
        /* first icon is increment (rightmost of set) */
        p_i_a = &p_dialog_ictl->riscos.dwi[1];

        a_icon.flags.bits.button_type = ButtonType_Repeat;

        a_icon.flags.bits.redraw = 0;

        a_icon.bbox.xmin = a_icon.bbox.xmax - p_dialog->riscos.bitmap_inc.size.cx;

        a_icon.flags.bits.indirect = 1;
        a_icon.flags.bits.text = 1;
        a_icon.flags.bits.sprite = 1;
        a_icon.data.ist.buffer = "";
        a_icon.data.ist.validation = "R5;Sup,pup"; /* Action button; up sprites */ /* NewLook or suitable substitutes must be loaded */
        a_icon.data.ist.buffer_size = 1;

        /* second icon is decrement (to the left of increment) */
        p_i_b = &p_dialog_ictl->riscos.dwi[2];

        b_icon.flags.bits.button_type = ButtonType_Repeat;

        b_icon.flags.bits.redraw = 0;

        b_icon.bbox.xmax = a_icon.bbox.xmin; /* maybe tiny amount of horizontal overlap iff 2-D */

        b_icon.bbox.xmin = b_icon.bbox.xmax - p_dialog->riscos.bitmap_dec.size.cx;

        b_icon.flags.bits.indirect = 1;
        b_icon.flags.bits.text = 1;
        b_icon.flags.bits.sprite = 1;
        b_icon.data.ist.buffer = "";
        b_icon.data.ist.validation = "R5;Sdown,pdown"; /* Action button; down sprites */ /* NewLook or suitable substitutes must be loaded */
        b_icon.data.ist.buffer_size = 1;

        /* third icon is editable field (to the left of the bump pair) */
        p_i_c = &p_dialog_ictl->riscos.dwi[0];

        c_icon.flags.bits.redraw = 1;

        c_icon.bbox.xmax = b_icon.bbox.xmin - (int) (DIALOG_BUMPGAP_H / PIXITS_PER_RISCOS); /* small gap from icons to field */

        if(p_dialog_ictl->data.bump_xx.edit_xx.readonly)
            c_icon.flags.bits.button_type = ButtonType_Never;
        else
            c_icon.flags.bits.button_type = ButtonType_DoubleClickDrag;

#ifdef EDIT_XX_SINGLE_LINE_WIMP
        if(0 != p_dialog_ictl->data.bump_xx.edit_xx.riscos.slec_buffer_size)
        {
            c_icon.flags.bits.redraw = 0;

            c_icon.flags.bits.horz_centre = 0;

            c_icon.flags.bits.border = 1;
            c_icon.flags.bits.filled = 1;

            c_icon.flags.bits.bg_colour = '\x00';

            c_icon.flags.bits.indirect = 1;
            c_icon.flags.bits.text = 1;
            c_icon.data.it.buffer = p_dialog_ictl->data.edit.edit_xx.riscos.slec_buffer;
            c_icon.data.it.validation = (p_dialog_ictl->dialog_control_type == DIALOG_CONTROL_BUMP_S32) ? "A0-9+-" : "A0-9.E+-";
            c_icon.data.it.buffer_length = p_dialog_ictl->data.edit.edit_xx.riscos.slec_buffer_size;

            if(!p_dialog_ictl->data.bump_xx.edit_xx.readonly)
                c_icon.flags.button_type = ButtonType_Writable;
        }
#endif

        break;
        }

    case DIALOG_CONTROL_LIST_TEXT:
    case DIALOG_CONTROL_LIST_S32:
        {
        RI_LBOX_NEW _ri_lbox_new;

        /* framed border clickless icon over which the list box window lives */
        a_icon.flags.bits.button_type = ButtonType_Never;

        a_icon.flags.bits.border = 1;

        a_icon.flags.bits.indirect = 1;
        a_icon.flags.bits.text = 1;
        a_icon.flags.bits.sprite = 1;
        a_icon.data.ist.buffer = "";
        a_icon.data.ist.validation = "R2"; /* Slab in */
        a_icon.data.ist.buffer_size = 1;

        status_return(dialog_riscos_icon_create(p_dialog, &a_icon, &p_dialog_ictl->riscos.dwi[0]));

        zero_struct(_ri_lbox_new);

        assert_EQ((RI_LBOX_ITEM_HEIGHT * PIXITS_PER_RISCOS), DIALOG_STDLISTITEM_V);

        {
        GDI_POINT gdi_org;
        host_gdi_org_from_screen(&gdi_org, p_dialog->hwnd); /* window work area ABS origin */
        _ri_lbox_new.bbox.xmin = a_icon.bbox.xmin + gdi_org.x;
        _ri_lbox_new.bbox.ymin = a_icon.bbox.ymin + gdi_org.y;
        _ri_lbox_new.bbox.xmax = a_icon.bbox.xmax + gdi_org.x;
        _ri_lbox_new.bbox.ymax = a_icon.bbox.ymax + gdi_org.y;
        } /*block*/

        host_framed_BBox_trim(&_ri_lbox_new.bbox, FRAMED_BOX_TROUGH_LBOX);

        /* trim off another bit for the window border (even though that's white ...) */
        /* (ok, so it'll be wodgy changing down resolution but what the hell ...) */
        _ri_lbox_new.bbox.xmin += host_modevar_cache_current.dx;
        _ri_lbox_new.bbox.ymin += host_modevar_cache_current.dy;
        _ri_lbox_new.bbox.xmax -= host_modevar_cache_current.dx;
        _ri_lbox_new.bbox.ymax -= host_modevar_cache_current.dy;

        _ri_lbox_new.caption = NULL;
        _ri_lbox_new.bits.allow_non_multiple = 1;
        _ri_lbox_new.bits.dont_shrink_to_fit = 1;
        _ri_lbox_new.bits.always_show_selection = 1;
        _ri_lbox_new.bits.trespass = 1;
        _ri_lbox_new.bits.disable_frame = 1; /* well, white ... */
        _ri_lbox_new.p_docu = p_docu_from_docno(p_dialog->docno);
        _ri_lbox_new.p_proc_dialog_event = lbn_event_dialog;
        _ri_lbox_new.client_handle = (CLIENT_HANDLE) p_dialog->h_dialog;

        _ri_lbox_new.bits.force_v_scroll = p_dialog_ictl->p_dialog_control_data.list_text && p_dialog_ictl->p_dialog_control_data.list_text->list_xx.bits.force_v_scroll;
        _ri_lbox_new.bits.disable_double = p_dialog_ictl->p_dialog_control_data.list_text && p_dialog_ictl->p_dialog_control_data.list_text->list_xx.bits.disable_double;
        _ri_lbox_new.p_ui_control = p_dialog_ictl->data.list_xx.list_xx.p_ui_control_s32;
        _ri_lbox_new.p_ui_source = p_dialog_ictl->data.list_xx.list_xx.p_ui_source;

        _ri_lbox_new.ui_data_type = (p_dialog_ictl->dialog_control_type == DIALOG_CONTROL_LIST_S32)
                                       ? UI_DATA_TYPE_S32
                                       : UI_DATA_TYPE_TEXT;

        assert(!p_dialog_ictl->data.list_xx.list_xx.riscos.lbox);
        ri_lbox_new(&p_dialog_ictl->data.list_xx.list_xx.riscos.lbox, &_ri_lbox_new);

        ri_lbox_make_child(p_dialog_ictl->data.list_xx.list_xx.riscos.lbox, p_dialog->hwnd, p_dialog_ictl->riscos.dwi[0].icon_handle);

        /* always open as modeless window; under our control */
#if 0 /* allow winx_open_window to open it */
        ri_lbox_show(p_dialog_ictl->data.list_xx.lbox);
#endif

        break;
        }

    case DIALOG_CONTROL_COMBO_TEXT:
    case DIALOG_CONTROL_COMBO_S32:
        {
        /* first icon is dropdown button (always at right) */
        p_i_a = &p_dialog_ictl->riscos.dwi[1];

        a_icon.bbox.xmin = a_icon.bbox.xmax - p_dialog->riscos.bitmap_dropdown.size.cx;
        a_icon.bbox.ymin = a_icon.bbox.ymax - p_dialog->riscos.bitmap_dropdown.size.cy;

        a_icon.flags.bits.indirect = 1;
        a_icon.flags.bits.text = 1;
        a_icon.flags.bits.sprite = 1;
        a_icon.data.ist.buffer = "";
        a_icon.data.ist.validation = "R5;Sgright,pgright"; /* Action button; gright sprites */ /* NewLook or suitable substitutes must be loaded */
        a_icon.data.ist.buffer_size = 1;

        /* second icon is state indicator to the left of the dropdown button */
        p_i_b = &p_dialog_ictl->riscos.dwi[0];

        b_icon.flags.bits.redraw = 1;

        b_icon.bbox.xmax = a_icon.bbox.xmin;

        if(p_dialog_ictl->data.combo_xx.edit_xx.readonly)
            b_icon.flags.bits.button_type = ButtonType_Never;
        else
            b_icon.flags.bits.button_type = ButtonType_DoubleClickDrag;

#ifdef EDIT_XX_SINGLE_LINE_WIMP
        if(0 != p_dialog_ictl->data.combo_xx.edit_xx.riscos.slec_buffer_size)
        {
            b_icon.flags.bits.redraw = 0;

            b_icon.flags.bits.horz_centre = 0;

            b_icon.flags.bits.border = 1;
            b_icon.flags.bits.filled = 1;

            b_icon.flags.bits.bg_colour = '\x00';

            b_icon.flags.bits.indirect = 1;
            b_icon.flags.bits.text = 1;
            b_icon.data.it.buffer = p_dialog_ictl->data.edit.edit_xx.riscos.slec_buffer;
            b_icon.data.it.validation = NULL;
            b_icon.data.it.buffer_length = p_dialog_ictl->data.edit.edit_xx.riscos.slec_buffer_size;

            if(p_dialog_ictl->data.combo_xx.edit_xx.readonly)
                b_icon.flags.bits.button_type = ButtonType_Writable;
        }
#endif

        break;
        }

    case DIALOG_CONTROL_USER:
        {
        p_i_a = &p_dialog_ictl->riscos.dwi[0];

        a_icon.flags.bits.button_type = ButtonType_DoubleClickDrag;

        a_icon.flags.bits.redraw = 1;

        break;
        }
    }

    status_return(status);

    if(NULL != p_i_a)
        status_return(dialog_riscos_icon_create(p_dialog, &a_icon, p_i_a));

    if(NULL != p_i_b)
        status_return(dialog_riscos_icon_create(p_dialog, &b_icon, p_i_b));

    if(NULL != p_i_c)
        status_return(dialog_riscos_icon_create(p_dialog, &c_icon, p_i_c));

    {
    P_DIALOG_ICTL_EDIT_XX p_dialog_ictl_edit_xx = p_dialog_ictl_edit_xx_from(p_dialog_ictl);

    if((NULL != p_dialog_ictl_edit_xx) && (NULL != p_dialog_ictl_edit_xx->riscos.mlec))
    {
        S32 caretheightpos, caretheightneg, charheight, topmargin;
        WimpIconBlockWithBitset icon;

        PTR_ASSERT(p_dialog_ictl_edit_xx);

        void_WrapOsErrorReporting(wimp_get_icon_state_with_bitset(p_dialog->hwnd, p_dialog_ictl->riscos.dwi[0].icon_handle, &icon));

        host_framed_BBox_trim(&icon.bbox, p_dialog_ictl_edit_xx->border_style);

        charheight = mlec_attribute_query(p_dialog_ictl_edit_xx->riscos.mlec, MLEC_ATTRIBUTE_CHARHEIGHT);

        /* set up an appropriate top margin for vertical centring of characters (not lines) if not multiline */
        if(!p_dialog_ictl_edit_xx->multiline)
        {
            topmargin = ((GDI_COORD) BBox_height(&icon.bbox) - charheight) / 2;

            topmargin = (topmargin + ((GDI_COORD) host_modevar_cache_current.dy - 1)) & ~((GR_COORD) host_modevar_cache_current.dy - 1);

            if( topmargin < 0)
                topmargin = 0;
        }
        else
            topmargin = 4;

        mlec_attribute_set(p_dialog_ictl_edit_xx->riscos.mlec, MLEC_ATTRIBUTE_MARGIN_TOP, topmargin);

        /* set up an appropriate caret height if needs be */
        if(!p_dialog_ictl_edit_xx->readonly)
        {
            caretheightpos = (7 * charheight / 8);
            caretheightneg = (1 * charheight / 8);

            if(topmargin)
            {
                caretheightpos += 4;

                if(((GDI_COORD) icon.bbox.ymax - icon.bbox.ymin) > caretheightpos + caretheightneg)
                    caretheightneg += 4;
            }

            mlec_attribute_set(p_dialog_ictl_edit_xx->riscos.mlec, MLEC_ATTRIBUTE_CARETHEIGHTPOS, caretheightpos);
            mlec_attribute_set(p_dialog_ictl_edit_xx->riscos.mlec, MLEC_ATTRIBUTE_CARETHEIGHTNEG, caretheightneg);
        }
    }
    } /*block*/

    return(STATUS_OK);
}

_Check_return_
extern STATUS
dialog_riscos_ictl_edit_xx_create(
    P_DIALOG_ICTL p_dialog_ictl)
{
    STATUS status = STATUS_OK;
    P_DIALOG_ICTL_EDIT_XX p_dialog_ictl_edit_xx;

    if(NULL != (p_dialog_ictl_edit_xx = p_dialog_ictl_edit_xx_from(p_dialog_ictl)))
    {
#ifdef EDIT_XX_SINGLE_LINE_WIMP
        if(!p_dialog_ictl_edit_xx->multiline)
        {
            p_dialog_ictl_edit_xx->riscos.slec_buffer_size = 256; /* could tune down for BUMP_XXs */
            if(NULL == (p_dialog_ictl_edit_xx->riscos.slec_buffer = al_ptr_alloc_bytes(P_U8, p_dialog_ictl_edit_xx->riscos.slec_buffer_size, &status)))
                p_dialog_ictl_edit_xx->riscos.slec_buffer_size = 0;
        }
        else
#endif
        {
            status_return(mlec_create(&p_dialog_ictl_edit_xx->riscos.mlec));
            mlec_attach_eventhandler(p_dialog_ictl_edit_xx->riscos.mlec, dialog_riscos_mlec_event_handler, p_dialog_ictl_edit_xx, TRUE);
        }
    }

    return(status);
}

extern void
dialog_riscos_ictl_edit_xx_destroy(
    P_DIALOG p_dialog,
    P_DIALOG_ICTL p_dialog_ictl)
{
    P_DIALOG_ICTL_EDIT_XX p_dialog_ictl_edit_xx;

    if(NULL != (p_dialog_ictl_edit_xx = p_dialog_ictl_edit_xx_from(p_dialog_ictl)))
    {
#ifdef EDIT_XX_SINGLE_LINE_WIMP
        if(0 != p_dialog_ictl_edit_xx->riscos.slec_buffer_size)
        {
            /* remove the buffer from the associated writeable icon first! */
            const P_DIALOG_WIMP_I p_i = &p_dialog_ictl->riscos.dwi[0];
            WimpIconBlockWithBitset icon;

            status_assert(dialog_riscos_icon_recreate_prepare(p_dialog, &icon, p_i));
            dialog_riscos_icon_text_setup(&icon, NULL);
            status_assert(dialog_riscos_icon_recreate(p_dialog, &icon, p_i));

            al_ptr_dispose(P_P_ANY_PEDANTIC(&p_dialog_ictl_edit_xx->riscos.slec_buffer));
            p_dialog_ictl_edit_xx->riscos.slec_buffer_size = 0;
        }
#else
        UNREFERENCED_PARAMETER(p_dialog);
#endif

        if(NULL != p_dialog_ictl_edit_xx->riscos.mlec)
            mlec_destroy(&p_dialog_ictl_edit_xx->riscos.mlec);
    }
}

/******************************************************************************
*
* reflect the enabled state in this view on a control
*
******************************************************************************/

_Check_return_
extern STATUS
dialog_riscos_ictl_enable_here(
    P_DIALOG p_dialog,
    P_DIALOG_ICTL p_dialog_ictl)
{
    P_DIALOG_WIMP_I   p_icone[3 + 1];
    P_DIALOG_WIMP_I   p_iconr[3 + 1];
    P_DIALOG_WIMP_I * ppie = p_icone;
    P_DIALOG_WIMP_I * ppir = p_iconr;
    P_DIALOG_WIMP_I * ppie_t = ppie;
    P_DIALOG_WIMP_I * ppir_t = ppir;
    int i;

    for(i = 0; i < 3; ++i)
    {
        const P_DIALOG_WIMP_I p_i = &p_dialog_ictl->riscos.dwi[i];

        if(BAD_WIMP_I == p_i->icon_handle)
            continue;

        if(p_i->redrawn_by_wimp)
            *ppie++ = p_i;
        else
            *ppir++ = p_i;
    }

    switch(p_dialog_ictl->dialog_control_type)
    {
    default: default_unhandled();
#if CHECKING
    case DIALOG_CONTROL_GROUPBOX:
    case DIALOG_CONTROL_STATICTEXT:
    case DIALOG_CONTROL_STATICFRAME:
    case DIALOG_CONTROL_STATICPICTURE:
    case DIALOG_CONTROL_PUSHBUTTON:
    case DIALOG_CONTROL_PUSHPICTURE:
    case DIALOG_CONTROL_RADIOBUTTON:
    case DIALOG_CONTROL_RADIOPICTURE:
    case DIALOG_CONTROL_CHECKBOX:
    case DIALOG_CONTROL_CHECKPICTURE:
    case DIALOG_CONTROL_TRISTATE:
    case DIALOG_CONTROL_TRIPICTURE:
    case DIALOG_CONTROL_EDIT:
    case DIALOG_CONTROL_BUMP_S32:
    case DIALOG_CONTROL_BUMP_F64:
    case DIALOG_CONTROL_USER:
#endif
        break;

    case DIALOG_CONTROL_LIST_TEXT:
    case DIALOG_CONTROL_LIST_S32:
        if(p_dialog_ictl->data.list_xx.list_xx.riscos.lbox)
            ri_lbox_enable(p_dialog_ictl->data.list_xx.list_xx.riscos.lbox, !p_dialog_ictl->bits.disabled);
        break;

    case DIALOG_CONTROL_COMBO_TEXT:
    case DIALOG_CONTROL_COMBO_S32:
        if(p_dialog_ictl->data.combo_xx.list_xx.riscos.lbox)
            ri_lbox_enable(p_dialog_ictl->data.combo_xx.list_xx.riscos.lbox, !p_dialog_ictl->bits.disabled);
        break;
    }

    assert(&p_icone[elemof32(p_icone)] >= ppie);
    assert(&p_iconr[elemof32(p_iconr)] >= ppir);

    while(ppie_t < ppie)
        dialog_riscos_icon_enable(p_dialog, *ppie_t++, p_dialog_ictl->bits.disabled);

    while(ppir_t < ppir)
        dialog_riscos_icon_redraw_for_enable(p_dialog, *ppir_t++);

    return(STATUS_OK);
}

_Check_return_
extern STATUS
dialog_riscos_ictl_focus_query(
    P_DIALOG p_dialog,
    P_DIALOG_ICTL p_dialog_ictl)
{
    {
    P_DIALOG_ICTL_EDIT_XX p_dialog_ictl_edit_xx = p_dialog_ictl_edit_xx_from(p_dialog_ictl);

    if(NULL != p_dialog_ictl_edit_xx)
    {
        WimpCaret caretstr;

        void_WrapOsErrorReporting(winx_get_caret_position(&caretstr));

        if((caretstr.window_handle == p_dialog->hwnd) && (caretstr.icon_handle == p_dialog_ictl->riscos.dwi[0].icon_handle))
            return(STATUS_OK);
    }
    } /*block*/

    switch(p_dialog_ictl->dialog_control_type)
    {
    case DIALOG_CONTROL_LIST_TEXT:
    case DIALOG_CONTROL_LIST_S32:
        if(p_dialog_ictl->data.list_xx.list_xx.riscos.lbox)
            if(status_ok(ri_lbox_focus_query(p_dialog_ictl->data.list_xx.list_xx.riscos.lbox)))
                return(STATUS_OK);

        break;

    case DIALOG_CONTROL_COMBO_TEXT:
    case DIALOG_CONTROL_COMBO_S32:
        if(p_dialog_ictl->data.combo_xx.list_xx.riscos.lbox)
            if(status_ok(ri_lbox_focus_query(p_dialog_ictl->data.combo_xx.list_xx.riscos.lbox)))
                return(STATUS_OK);

        break;

    default:
        break;
    }

    return(STATUS_FAIL);
}

/******************************************************************************
*
* create controls in a repr for a group of controls
*
******************************************************************************/

_Check_return_
extern STATUS
dialog_riscos_ictls_create_in(
    P_DIALOG p_dialog,
    _InRef_     PC_DIALOG_ICTL_GROUP p_ictl_group)
{
    ARRAY_INDEX i;

    for(i = 0; i < n_ictls_from_group(p_ictl_group); ++i)
    {
        const P_DIALOG_ICTL p_dialog_ictl = p_dialog_ictl_from(p_ictl_group, i);
        PIXIT_RECT pixit_rect;

        dialog_control_rect(p_dialog, p_dialog_ictl->dialog_control_id, &pixit_rect);

#ifdef DIALOG_COORD_DEBUG
        myassert3x(pixit_rect.tl.x < pixit_rect.br.x-1, TEXT("dialog_control_id ") U32_TFMT TEXT(" pixit_rect tl.x ") PIXIT_TFMT TEXT(" br.x ") PIXIT_TFMT, (U32) p_dialog_ictl->dialog_control_id, pixit_rect.tl.x, pixit_rect.br.x);
        myassert3x(pixit_rect.tl.y < pixit_rect.br.y-1, TEXT("dialog_control_id ") U32_TFMT TEXT(" pixit_rect tl.y ") PIXIT_TFMT TEXT(" br.y ") PIXIT_TFMT, (U32) p_dialog_ictl->dialog_control_id, pixit_rect.tl.y, pixit_rect.br.y);
#endif

        status_return(dialog_riscos_ictl_create_here(p_dialog, p_dialog_ictl, &pixit_rect));

        if(p_dialog_ictl->dialog_control_type == DIALOG_CONTROL_GROUPBOX)
            /* create subcontrols AFTER group icons 'cos they go on top */
            status_return(dialog_riscos_ictls_create_in(p_dialog, &p_dialog_ictl->data.groupbox.ictls));
    }

    return(STATUS_OK);
}

/******************************************************************************
*
* process a key press, depending on what kind on control we are in
*
******************************************************************************/

static DIALOG_CONTROL_ID
dialog_riscos_key_pressed_hot_key_in(
    P_DIALOG_ICTL_GROUP p_ictl_group,
    _InVal_     U8 kh)
{
    ARRAY_INDEX i;

    for(i = 0; i < n_ictls_from_group(p_ictl_group); ++i)
    {
        const P_DIALOG_ICTL p_dialog_ictl = p_dialog_ictl_from(p_ictl_group, i);

        if(p_dialog_ictl->riscos.hot_key == kh)
            return(p_dialog_ictl->dialog_control_id);

        if(p_dialog_ictl->dialog_control_type == DIALOG_CONTROL_GROUPBOX)
        {
            const DIALOG_CONTROL_ID dialog_control_id = dialog_riscos_key_pressed_hot_key_in(&p_dialog_ictl->data.groupbox.ictls, kh);

            if(0 != dialog_control_id)
                return(dialog_control_id);
        }
    }

    return(0);
}

_Check_return_
static STATUS
dialog_riscos_help_contents(
    _InoutRef_  P_DIALOG p_dialog)
{
    UNREFERENCED_PARAMETER_InRef_(p_dialog);

    return(ho_help_contents(NULL));
}

_Check_return_
static STATUS
dialog_riscos_help_topic(
    _InoutRef_  P_DIALOG p_dialog)
{
    TCHARZ url[BUF_MAX_PATHSTRING];
    PCTSTR tstr_help_topic;
    
    if(NULL == (tstr_help_topic = resource_lookup_tstr_no_default(p_dialog->help_topic_resource_id)))
        return(dialog_riscos_help_contents(p_dialog));

    tstr_xstrkpy(url, elemof32(url), prefix_uri_userguide_content_tstr);
    tstr_xstrkat(url, elemof32(url), tstr_help_topic);

    return(ho_help_url(url));
}

_Check_return_
static STATUS
dialog_riscos_help(
    _InoutRef_  P_DIALOG p_dialog)
{
    if(p_dialog->help_topic_resource_id)
        return(dialog_riscos_help_topic(p_dialog));

    return(dialog_riscos_help_contents(p_dialog));
}

_Check_return_
static STATUS
dialog_riscos_Key_Pressed_hot_key(
    P_DIALOG p_dialog,
    _InVal_     U8 kh)
{
    /* scan dialog for this hot key */
    const DIALOG_CONTROL_ID dialog_control_id = dialog_riscos_key_pressed_hot_key_in(&p_dialog->ictls, kh);
    P_DIALOG_ICTL p_dialog_ictl;
    STATUS status = STATUS_OK;

    if(!dialog_control_id)
        return(STATUS_OK /*NOT processed*/);

    p_dialog_ictl = p_dialog_ictl_from_control_id(p_dialog, dialog_control_id);
    PTR_ASSERT(p_dialog_ictl);

    if(!p_dialog_ictl->bits.disabled && !p_dialog_ictl->bits.nobbled)
    {
        switch(p_dialog_ictl->dialog_control_type)
        {
        case DIALOG_CONTROL_PUSHBUTTON:
        case DIALOG_CONTROL_PUSHPICTURE:
            status = dialog_click_pushbutton(p_dialog, p_dialog_ictl, FALSE, 0);
            break;

        case DIALOG_CONTROL_RADIOBUTTON:
        case DIALOG_CONTROL_RADIOPICTURE:
            status = dialog_click_radiobutton(p_dialog, p_dialog_ictl);
            break;

        case DIALOG_CONTROL_CHECKBOX:
        case DIALOG_CONTROL_CHECKPICTURE:
            status = dialog_click_checkbox(p_dialog, p_dialog_ictl);
            break;

#ifdef DIALOG_HAS_TRISTATE
        case DIALOG_CONTROL_TRISTATE:
        case DIALOG_CONTROL_TRIPICTURE:
            status = dialog_click_tristate(p_dialog, p_dialog_ictl);
            break;
#endif

        default:
            /* try to make next control with tabstop get focus */
            dialog_riscos_current_move(p_dialog, p_dialog_ictl->dialog_control_id, 1);
            break;
        }
    }

    UNREFERENCED_PARAMETER(status);

    return(STATUS_DONE /*processed*/);
}

_Check_return_
static STATUS
dialog_riscos_Key_Pressed(
    P_DIALOG p_dialog,
    _In_        KMAP_CODE kmap_code)
{
    P_DIALOG_ICTL p_dialog_ictl = NULL;
    P_DIALOG_ICTL_EDIT_XX p_dialog_ictl_edit_xx = NULL;
    STATUS status;

    /* key event and current control? */
    if(0 != p_dialog->current_dialog_control_id)
    {
        P_PROC_DIALOG_EVENT p_proc_client;
        DIALOG_MSG_CTL_KEY dialog_msg_ctl_key;
        msgclr(dialog_msg_ctl_key);

        p_dialog_ictl = p_dialog_ictl_from_control_id(p_dialog, p_dialog->current_dialog_control_id);
        PTR_ASSERT(p_dialog_ictl);

        p_dialog_ictl_edit_xx = p_dialog_ictl_edit_xx_from(p_dialog_ictl);
        assert((NULL == p_dialog_ictl_edit_xx) || !p_dialog_ictl_edit_xx->readonly);

        /* send to control first off */
        if(NULL != (p_proc_client = dialog_find_handler(p_dialog, p_dialog_ictl->dialog_control_id, &dialog_msg_ctl_key.client_handle)))
        {
            DIALOG_MSG_CTL_HDR_from_dialog_ictl(dialog_msg_ctl_key, p_dialog, p_dialog_ictl);

            dialog_msg_ctl_key.processed = 0;

            dialog_msg_ctl_key.key_code = kmap_code;

            if(status_fail(status = dialog_call_client(p_dialog, DIALOG_MSG_CODE_CTL_KEY, &dialog_msg_ctl_key, p_proc_client)))
            {
                reperr_null(status);
                return(STATUS_DONE); /* stop any further processing */
            }

            if(dialog_msg_ctl_key.processed)
                return(STATUS_DONE);
        }
    }

    status = STATUS_DONE; /* assume processed till shown otherwise */

    switch(kmap_code)
    {
    case KMAP_ESCAPE:
        {
        DIALOG_CMD_COMPLETE_DBOX dialog_cmd_complete_dbox;
        msgclr(dialog_cmd_complete_dbox);
        dialog_cmd_complete_dbox.h_dialog = p_dialog->h_dialog;
        dialog_cmd_complete_dbox.completion_code = DIALOG_COMPLETION_CANCEL;
        status_assert(object_call_DIALOG(DIALOG_CMD_CODE_COMPLETE_DBOX, &dialog_cmd_complete_dbox));
        break;
        }

    case KMAP_FUNC_SRETURN:
    case KMAP_FUNC_CRETURN:
    case KMAP_FUNC_CSRETURN:
        if((NULL != p_dialog_ictl_edit_xx) && (NULL != p_dialog_ictl_edit_xx->riscos.mlec) && p_dialog_ictl_edit_xx->multiline)
            status = mlec__Key_Pressed(p_dialog_ictl_edit_xx->riscos.mlec, kmap_code);
        break;

    case KMAP_FUNC_RETURN:
        {
        DIALOG_CMD_DEFPUSHBUTTON dialog_cmd_defpushbutton;
        dialog_cmd_defpushbutton.h_dialog = p_dialog->h_dialog;
        dialog_cmd_defpushbutton.double_dialog_control_id = 0;
        status_assert(object_call_DIALOG(DIALOG_CMD_CODE_DEFPUSHBUTTON, &dialog_cmd_defpushbutton));
        break;
        }

    case KMAP_FUNC_TAB:
        /* move input focus along to next item with a tabstop set */
        dialog_riscos_current_move(p_dialog, p_dialog->current_dialog_control_id, +1);
        break;

    case KMAP_FUNC_STAB:
        /* move input focus along to prev item with a tabstop set */
        dialog_riscos_current_move(p_dialog, p_dialog->current_dialog_control_id, -1);
        break;

    case KMAP_FUNC_ARROW_DOWN:
    case KMAP_FUNC_ARROW_UP:
#if 1
        if((NULL != p_dialog_ictl_edit_xx) && p_dialog_ictl_edit_xx->riscos.mlec && p_dialog_ictl_edit_xx->multiline)
        {
            status = mlec__Key_Pressed(p_dialog_ictl_edit_xx->riscos.mlec, kmap_code);
            break;
        }
#else
        if(NULL != p_dialog_ictl_edit_xx)
        {
            if(!p_dialog_ictl_edit_xx->multiline)
                kmap_code = (kmap_code == KMAP_FUNC_ARROW_UP)
                   ? KMAP_FUNC_ARROW_LEFT
                   : KMAP_FUNC_ARROW_RIGHT;

            status = mlec__Key_Pressed(p_dialog_ictl_edit_xx->riscos.mlec, kmap_code);
            break;
        }
#endif

        switch(p_dialog_ictl->dialog_control_type)
        {
        case DIALOG_CONTROL_BUMP_S32:
        case DIALOG_CONTROL_BUMP_F64:
            status_assert(dialog_click_bump_xx(p_dialog, p_dialog_ictl, (kmap_code == KMAP_FUNC_ARROW_UP)));
            break;

        default:
            /* move input focus along to next/prev control */
            dialog_riscos_current_move(p_dialog, p_dialog->current_dialog_control_id, (kmap_code == KMAP_FUNC_ARROW_UP) ? -1 : +1);
            break;
        }

        break;

    case KMAP_FUNC_ARROW_LEFT:
    case KMAP_FUNC_ARROW_RIGHT:
        if((NULL != p_dialog_ictl_edit_xx) && p_dialog_ictl_edit_xx->riscos.mlec)
        {
            status = mlec__Key_Pressed(p_dialog_ictl_edit_xx->riscos.mlec, kmap_code);
        }
        break;

    case (KMAP_BASE_FUNC    + 0x01): /* F1 */
        status_break(status = dialog_riscos_help(p_dialog));
        status = STATUS_DONE;
        break;

    default:
        if((NULL != p_dialog_ictl_edit_xx) && p_dialog_ictl_edit_xx->riscos.mlec)
        {
            /* early validation prevents much state thrash */
            if(kmap_code < 0x100)
            {
                if(p_dialog_ictl_edit_xx->p_bitmap_validation)
                    if(bitmap_bit_test((PC_BITMAP) p_dialog_ictl_edit_xx->p_bitmap_validation, kmap_code, N_BITS_ARG(256)))
                        status_break(status = mlec__insert_char(p_dialog_ictl_edit_xx->riscos.mlec, (U8) kmap_code));

                return(STATUS_DONE /*processed*/);
            }
            else if(status_ok(status = mlec__Key_Pressed(p_dialog_ictl_edit_xx->riscos.mlec, kmap_code)))
                return(STATUS_DONE /*processed*/);
        }

        if(0 != (kmap_code & KMAP_CODE_ADDED_ALT))
        {
            U8 kh = (U8) kmap_code & 0xFF;

            status = dialog_riscos_Key_Pressed_hot_key(p_dialog, kh);
            break;
        }

        if((kmap_code < 0x100) && sbchar_isalpha(kmap_code))
        {
            status = dialog_riscos_Key_Pressed_hot_key(p_dialog, sbchar_toupper(kmap_code));
            break;
        }

        status = STATUS_OK; /* NOT processed */
        break;
    }

    if(status_fail(status))
    {
        reperr_null(status);
        return(STATUS_DONE); /* stop any further processing */
    }

    return(status); /* STATUS_OK == NOT processed; status_done() == processed */
}

/******************************************************************************
*
* view has got a null event
*
******************************************************************************/

static void
dialog_riscos_null_event_status(
    P_DIALOG p_dialog)
{
    /* track pointer shape over controls */
    WimpGetPointerInfoBlock pointer_info;
    const WimpMouseClickEvent * const p_mouse_click = (const WimpMouseClickEvent *) &pointer_info;
    POINTER_SHAPE pshape;
    UI_TEXT ui_text;
    DIALOG_CONTROL_ID dialog_control_id;

    void_WrapOsErrorReporting(wimp_get_pointer_info(&pointer_info));
    pointer_info.button_state = 0;

    pshape = POINTER_DEFAULT;
    ui_text.type = UI_TEXT_TYPE_NONE;

    if((dialog_control_id = dialog_riscos_scan_controls_in(p_dialog, &p_dialog->ictls, pointer_info.icon_handle)) != 0)
    {
        const P_DIALOG_ICTL p_dialog_ictl = p_dialog_ictl_from_control_id(p_dialog, dialog_control_id);
        const wimp_i hit_icon_handle = pointer_info.icon_handle;

        PTR_ASSERT(p_dialog_ictl);

        if(p_dialog_ictl->p_dialog_control->bits.has_help)
            ui_text = ((PC_DIALOG_CONTROLH) p_dialog_ictl->p_dialog_control)->interactive_help;

        if(!p_dialog_ictl->bits.disabled && !p_dialog_ictl->bits.nobbled)
        {
            switch(p_dialog_ictl->dialog_control_type)
            {
            default:
#if CHECKING
            case DIALOG_CONTROL_LIST_TEXT:
            case DIALOG_CONTROL_LIST_S32:
                /* these controls live in separate windows so we won't get to track them here */

            case DIALOG_CONTROL_GROUPBOX:
                assert0();

                /*FALLTHRU*/

            case DIALOG_CONTROL_STATICPICTURE:
            case DIALOG_CONTROL_STATICTEXT:
            case DIALOG_CONTROL_STATICFRAME:
            case DIALOG_CONTROL_PUSHBUTTON:
            case DIALOG_CONTROL_PUSHPICTURE:
            case DIALOG_CONTROL_RADIOBUTTON:
            case DIALOG_CONTROL_RADIOPICTURE:
            case DIALOG_CONTROL_CHECKBOX:
            case DIALOG_CONTROL_CHECKPICTURE:
            case DIALOG_CONTROL_TRISTATE:
            case DIALOG_CONTROL_TRIPICTURE:
#endif
                break;

            case DIALOG_CONTROL_USER:
                {
                P_PROC_DIALOG_EVENT p_proc_client;
                DIALOG_MSG_CTL_USER_POINTER_QUERY dialog_msg_ctl_user_pointer_query;
                msgclr(dialog_msg_ctl_user_pointer_query);

                if(NULL != (p_proc_client = dialog_find_handler(p_dialog, p_dialog_ictl->dialog_control_id, &dialog_msg_ctl_user_pointer_query.client_handle)))
                {
                    DIALOG_MSG_CTL_HDR_from_dialog_ictl(dialog_msg_ctl_user_pointer_query, p_dialog, p_dialog_ictl);

                    dialog_msg_ctl_user_pointer_query.pointer_shape = POINTER_DEFAULT;

                    dialog_msg_ctl_user_pointer_query.riscos.window_handle = p_dialog->hwnd;
                    void_WrapOsErrorReporting(wimp_get_icon_state_with_bitset(dialog_msg_ctl_user_pointer_query.riscos.window_handle, hit_icon_handle, &dialog_msg_ctl_user_pointer_query.riscos.icon));

                    if(status_ok(dialog_call_client(p_dialog, DIALOG_MSG_CODE_CTL_USER_POINTER_QUERY, &dialog_msg_ctl_user_pointer_query, p_proc_client)))
                        pshape = dialog_msg_ctl_user_pointer_query.pointer_shape;
                }

                break;
                }

            case DIALOG_CONTROL_EDIT:
                pshape = dialog_riscos_mlec_event_determine_pshape(p_dialog, p_dialog_ictl, &p_dialog_ictl->data.edit.edit_xx, p_mouse_click);
                break;

            case DIALOG_CONTROL_BUMP_S32:
            case DIALOG_CONTROL_BUMP_F64:
                if(hit_icon_handle == p_dialog_ictl->riscos.dwi[0].icon_handle)
                    pshape = dialog_riscos_mlec_event_determine_pshape(p_dialog, p_dialog_ictl, &p_dialog_ictl->data.bump_xx.edit_xx, p_mouse_click);
                break;

            case DIALOG_CONTROL_COMBO_TEXT:
            case DIALOG_CONTROL_COMBO_S32:
                if(hit_icon_handle == p_dialog_ictl->riscos.dwi[0].icon_handle)
                    pshape = dialog_riscos_mlec_event_determine_pshape(p_dialog, p_dialog_ictl, &p_dialog_ictl->data.combo_xx.edit_xx, p_mouse_click);
                else if(hit_icon_handle == p_dialog_ictl->riscos.dwi[1].icon_handle)
                    pshape = POINTER_MENU;
                break;
            }
        }
    }

    host_set_pointer_shape(pshape);

    if(DOCNO_NONE != p_dialog->docno)
        status_line_set(p_docu_from_docno(p_dialog->docno), STATUS_LINE_LEVEL_DIALOG_CONTROLS, &ui_text);
}

/******************************************************************************
*
* fire through null events iff pointer state (buttons or position) change
* or every half-second or so to catch any changes that happen under our feet
*
******************************************************************************/

extern void
dialog_riscos_null_event(
    P_DIALOG p_dialog)
{
    WimpGetPointerInfoBlock pointer_info;
    MONOTIME now;

    void_WrapOsErrorReporting(wimp_get_pointer_info(&pointer_info));

    now = monotime();

    if(0 == memcmp32(&dialog_null_statics.pointer_info, &pointer_info, sizeof32(pointer_info)))
#if TRACE_ALLOWED
        if(now - dialog_null_statics.pacemaker < MONOTIMEDIFF_VALUE_FROM_MS(2500))
#else
        if(now - dialog_null_statics.pacemaker < MONOTIMEDIFF_VALUE_FROM_MS(500))
#endif
            return;

    dialog_null_statics.pointer_info = pointer_info;
    dialog_null_statics.pacemaker = now;

    dialog_riscos_null_event_do(p_dialog);
}

static void
dialog_riscos_null_event_do(
    P_DIALOG p_dialog)
{
    if(winx_drag_active() == p_dialog->hwnd)
    {
        if(dragging_file_icon)
        {
        }
        else
        {
            P_DIALOG_ICTL p_dialog_ictl;
            P_DIALOG_ICTL_EDIT_XX p_dialog_ictl_edit_xx;

            /* send to current control, probably mlec */
            assert(p_dialog->current_dialog_control_id);

            p_dialog_ictl = p_dialog_ictl_from_control_id(p_dialog, p_dialog->current_dialog_control_id);
            PTR_ASSERT(p_dialog_ictl);

            if(NULL != (p_dialog_ictl_edit_xx = p_dialog_ictl_edit_xx_from(p_dialog_ictl)))
            {
                UNREFERENCED_PARAMETER(p_dialog_ictl_edit_xx);
                dialog_riscos_mlec_enull(p_dialog, p_dialog_ictl);
                return;
            }
        }
    }

    dialog_riscos_null_event_status(p_dialog);
}

static void
dialog_riscos_redraw_control_bump_xx(
    P_DIALOG_CONTROL_REDRAW p_dialog_control_redraw,
    _InRef_     PC_DIALOG_ICTL p_dialog_ictl,
    _InVal_     DIALOG_CONTROL_TYPE dialog_control_type)
{
    FRAMED_BOX_STYLE b;

    b = p_dialog_control_redraw->p_dialog_ictl_edit_xx->border_style;
    part_control_redraw(p_dialog_control_redraw, b, &p_dialog_ictl->riscos.dwi[0], dialog_control_type);

    b = FRAMED_BOX_BUTTON_OUT;
    part_control_redraw(p_dialog_control_redraw, b, &p_dialog_ictl->riscos.dwi[1], (DIALOG_CONTROL_TYPE) (dialog_control_type | 0x100));
    part_control_redraw(p_dialog_control_redraw, b, &p_dialog_ictl->riscos.dwi[2], (DIALOG_CONTROL_TYPE) (dialog_control_type | 0x200));
}

static void
dialog_riscos_redraw_control_combo_xx(
    P_DIALOG_CONTROL_REDRAW p_dialog_control_redraw,
    _InRef_     PC_DIALOG_ICTL p_dialog_ictl,
    _InVal_     DIALOG_CONTROL_TYPE dialog_control_type)
{
    FRAMED_BOX_STYLE b;

    b = p_dialog_control_redraw->p_dialog_ictl_edit_xx->border_style;
    part_control_redraw(p_dialog_control_redraw, b, &p_dialog_ictl->riscos.dwi[0], dialog_control_type);
}

_Check_return_
static STATUS
dialog_riscos_redraw_control(
    P_DIALOG_CONTROL_REDRAW p_dialog_control_redraw)
{
    const P_DIALOG_ICTL p_dialog_ictl = p_dialog_control_redraw->p_dialog_ictl;
    const DIALOG_CONTROL_TYPE dialog_control_type = p_dialog_ictl->dialog_control_type;
    const P_DIALOG_WIMP_I p_i = &p_dialog_ictl->riscos.dwi[0];
    FRAMED_BOX_STYLE b = FRAMED_BOX_NONE;

    switch(dialog_control_type)
    {
    default: default_unhandled();
#if CHECKING
    case DIALOG_CONTROL_GROUPBOX:
    case DIALOG_CONTROL_STATICFRAME:
    case DIALOG_CONTROL_LIST_S32:
    case DIALOG_CONTROL_LIST_TEXT:
#endif
        break;

    case DIALOG_CONTROL_STATICTEXT:
        part_control_redraw(p_dialog_control_redraw, b, p_i, dialog_control_type);
        break;

    case DIALOG_CONTROL_PUSHBUTTON:
    case DIALOG_CONTROL_PUSHPICTURE:
        if(!p_dialog_control_redraw->p_dialog_control_data.pushbutton->push_xx.not_dlg_framed)
            b = (p_dialog_control_redraw->p_dialog->default_dialog_control_id == p_dialog_ictl->dialog_control_id) ? FRAMED_BOX_DEFBUTTON_OUT : FRAMED_BOX_BUTTON_OUT;
        part_control_redraw(p_dialog_control_redraw, b, p_i, dialog_control_type);
        break;

    case DIALOG_CONTROL_RADIOBUTTON:
    case DIALOG_CONTROL_CHECKBOX:
#ifdef DIALOG_HAS_TRISTATE
    case DIALOG_CONTROL_TRISTATE:
#endif
        part_control_redraw(p_dialog_control_redraw, b, &p_dialog_ictl->riscos.dwi[1], dialog_control_type);
        break;

    case DIALOG_CONTROL_RADIOPICTURE:
        b = (p_dialog_ictl->bits.radiobutton_active) ? FRAMED_BOX_BUTTON_IN : FRAMED_BOX_BUTTON_OUT;
        part_control_redraw(p_dialog_control_redraw, b, p_i, dialog_control_type);
        break;

    case DIALOG_CONTROL_CHECKPICTURE:
        b = (p_dialog_ictl->state.checkbox == DIALOG_BUTTONSTATE_ON) ? FRAMED_BOX_BUTTON_IN : FRAMED_BOX_BUTTON_OUT;
        part_control_redraw(p_dialog_control_redraw, b, p_i, dialog_control_type);
        break;

#ifdef DIALOG_HAS_TRISTATE
    case DIALOG_CONTROL_TRIPICTURE:
        b = (p_dialog_ictl->state.tristate == DIALOG_TRISTATE_ON) ? FRAMED_BOX_BUTTON_IN : FRAMED_BOX_BUTTON_OUT;
        part_control_redraw(p_dialog_control_redraw, b, p_i, dialog_control_type);
        break;
#endif

    case DIALOG_CONTROL_EDIT:
        b = p_dialog_control_redraw->p_dialog_ictl_edit_xx->border_style;
        part_control_redraw(p_dialog_control_redraw, b, p_i, dialog_control_type);
        break;

    case DIALOG_CONTROL_BUMP_F64:
        dialog_riscos_redraw_control_bump_xx(p_dialog_control_redraw, p_dialog_ictl, DIALOG_CONTROL_BUMP_S32);
        break;

    case DIALOG_CONTROL_BUMP_S32:
        dialog_riscos_redraw_control_bump_xx(p_dialog_control_redraw, p_dialog_ictl, dialog_control_type);
        break;

    case DIALOG_CONTROL_COMBO_S32:
        dialog_riscos_redraw_control_combo_xx(p_dialog_control_redraw, p_dialog_ictl, DIALOG_CONTROL_COMBO_TEXT);
        break;

    case DIALOG_CONTROL_COMBO_TEXT:
        dialog_riscos_redraw_control_combo_xx(p_dialog_control_redraw, p_dialog_ictl, dialog_control_type);
        break;

    case DIALOG_CONTROL_USER:
        b = p_dialog_ictl->data.user.border_style;
        part_control_redraw(p_dialog_control_redraw, b, p_i, dialog_control_type);
        break;
    }

    return(STATUS_OK);
}

/******************************************************************************
*
* redraw a group of controls in a view
*
******************************************************************************/

_Check_return_
static STATUS
dialog_riscos_redraw_controls_in(
    P_DIALOG_CONTROL_REDRAW p_dialog_control_redraw)
{
    DIALOG_CONTROL_REDRAW dialog_control_redraw = *p_dialog_control_redraw;
    S32 pass, i;

    for(pass = 1; pass <= 2; ++pass)
    {
        for(i = 0; i < n_ictls_from_group(p_dialog_control_redraw->p_ictl_group); ++i)
            /* NB. always use passed structure (pass 2 pokes handles!) */
        {
            dialog_control_redraw.p_dialog_ictl = p_dialog_ictl_from(p_dialog_control_redraw->p_ictl_group, i);

            assert(dialog_control_redraw.p_dialog_ictl->dialog_control_id == dialog_control_redraw.p_dialog_ictl->dialog_control_id);

            if(pass == 1)
            {
                dialog_control_redraw.p_dialog_control_data = dialog_control_redraw.p_dialog_ictl->p_dialog_control_data;
                dialog_control_redraw.p_dialog_ictl_edit_xx = p_dialog_ictl_edit_xx_from(dialog_control_redraw.p_dialog_ictl);
                if(dialog_control_redraw.p_dialog_ictl_edit_xx)
                dialog_control_redraw.p_dialog_ictl_edit_xx = p_dialog_ictl_edit_xx_from(dialog_control_redraw.p_dialog_ictl);

                dialog_riscos_redraw_control(&dialog_control_redraw);
            }
            else /* if(pass == 2) */
            {
                if(dialog_control_redraw.p_dialog_ictl->dialog_control_type == DIALOG_CONTROL_GROUPBOX)
                {
                    dialog_control_redraw.p_ictl_group = &dialog_control_redraw.p_dialog_ictl->data.groupbox.ictls;

                    status_return(dialog_riscos_redraw_controls_in(&dialog_control_redraw));
                }
            }
        }
    }

    return(STATUS_OK);
}

/* send ourselves each redraw/update event */

/* dialog equivalent of host_redraw_context_set_host_xform() */

static void
dialog_host_redraw_context_set_host_xform(
    _InoutRef_  P_HOST_XFORM p_host_xform /*updated*/)
{
    p_host_xform->riscos.d_x = host_modevar_cache_current.dx;
    p_host_xform->riscos.d_y = host_modevar_cache_current.dy;

    p_host_xform->riscos.eig_x = host_modevar_cache_current.XEig;
    p_host_xform->riscos.eig_y = host_modevar_cache_current.YEig;
}

static void
dialog_riscos_redraw_core(
    _Inout_     WimpRedrawWindowBlock * const p_redraw_window_block,
    _InVal_     H_DIALOG h_dialog,
    _InVal_     BOOL is_update_now)
{
    DIALOG_RISCOS_EVENT_REDRAW_WINDOW dialog_riscos_event_redraw_window;
    REDRAW_CONTEXT_CACHE redraw_context_cache;
    const P_REDRAW_CONTEXT p_redraw_context = &dialog_riscos_event_redraw_window.redraw_context;

    zero_struct(dialog_riscos_event_redraw_window);

    zero_struct(redraw_context_cache);
    p_redraw_context->p_redraw_context_cache = &redraw_context_cache;

    host_invalidate_cache(HIC_REDRAW_LOOP_START);

    dialog_riscos_event_redraw_window.h_dialog = h_dialog;
    dialog_riscos_event_redraw_window.is_update_now = is_update_now;

    p_redraw_context->riscos.hwnd = p_redraw_window_block->window_handle;

    p_redraw_context->riscos.host_machine_clip_box = * (PC_GDI_BOX) &p_redraw_window_block->redraw_area;

    p_redraw_context->host_xform.scale.t.x = 1;
    p_redraw_context->host_xform.scale.t.y = 1;
    p_redraw_context->host_xform.scale.b.x = 1;
    p_redraw_context->host_xform.scale.b.y = 1;

    p_redraw_context->display_mode = DISPLAY_PRINT_AREA;

    p_redraw_context->border_width.x = PIXITS_PER_RISCOS << p_redraw_context->host_xform.riscos.d_x;
    p_redraw_context->border_width.y = PIXITS_PER_RISCOS << p_redraw_context->host_xform.riscos.d_y;
    p_redraw_context->border_width_2.x = 2 * p_redraw_context->border_width.x;
    p_redraw_context->border_width_2.y = 2 * p_redraw_context->border_width.y;

    dialog_host_redraw_context_set_host_xform(&p_redraw_context->host_xform);

    p_redraw_context->gdi_org.x = work_area_origin_x_from_visible_area_and_scroll(p_redraw_window_block); /* window work area ABS origin */
    p_redraw_context->gdi_org.y = work_area_origin_y_from_visible_area_and_scroll(p_redraw_window_block);

    host_redraw_context_fillin(p_redraw_context);

    {
    GDI_RECT gdi_rect;
    gdi_rect.tl.x = p_redraw_window_block->redraw_area.xmin;
    gdi_rect.br.y = p_redraw_window_block->redraw_area.ymin;
    gdi_rect.br.x = p_redraw_window_block->redraw_area.xmax;
    gdi_rect.tl.y = p_redraw_window_block->redraw_area.ymax;
    pixit_rect_from_screen_rect_and_context(&dialog_riscos_event_redraw_window.area_rect, &gdi_rect, p_redraw_context);
    } /*block*/

    status_assert(object_call_DIALOG(DIALOG_RISCOS_EVENT_CODE_REDRAW_WINDOW, &dialog_riscos_event_redraw_window));
}

/******************************************************************************
*
* the Window Manager has kindly helped us out in detecting which rectangle
* was hit and doing double click/drag/autorepeat on them appropriately
*
******************************************************************************/

static DIALOG_CONTROL_ID
dialog_riscos_scan_controls_in(
    P_DIALOG p_dialog,
    P_DIALOG_ICTL_GROUP p_ictl_group,
    _InVal_     wimp_i hit_icon_handle)
{
    ARRAY_INDEX i;
    DIALOG_CONTROL_ID dialog_control_id;
    BOOL is_hit = 0;

    if(hit_icon_handle == BAD_WIMP_I)
        return(0);

    i = n_ictls_from_group(p_ictl_group);

    while(--i >= 0)
    {
        const P_DIALOG_ICTL p_dialog_ictl = p_dialog_ictl_from(p_ictl_group, i);
        int n_test;

        switch(p_dialog_ictl->dialog_control_type)
        {
        default: default_unhandled();
#if CHECKING
        case DIALOG_CONTROL_STATICTEXT:
        case DIALOG_CONTROL_STATICFRAME:
        case DIALOG_CONTROL_PUSHBUTTON:
        case DIALOG_CONTROL_PUSHPICTURE:
        case DIALOG_CONTROL_RADIOPICTURE:
        case DIALOG_CONTROL_CHECKPICTURE:
        case DIALOG_CONTROL_TRIPICTURE:
        case DIALOG_CONTROL_EDIT:
        case DIALOG_CONTROL_USER:
#endif
            n_test = 1;
            break;

        case DIALOG_CONTROL_STATICPICTURE:
            n_test = 0;
            break;

        case DIALOG_CONTROL_GROUPBOX:
            if((dialog_control_id = dialog_riscos_scan_controls_in(p_dialog, &p_dialog_ictl->data.groupbox.ictls, hit_icon_handle)) != 0)
                return(dialog_control_id);

            n_test = 0;
            break;

        case DIALOG_CONTROL_RADIOBUTTON:
        case DIALOG_CONTROL_CHECKBOX:
        case DIALOG_CONTROL_TRISTATE:
        case DIALOG_CONTROL_COMBO_TEXT:
        case DIALOG_CONTROL_COMBO_S32:
            n_test = 2;
            break;

        case DIALOG_CONTROL_BUMP_S32:
        case DIALOG_CONTROL_BUMP_F64:
            n_test = 3;
            break;

        case DIALOG_CONTROL_LIST_TEXT:
        case DIALOG_CONTROL_LIST_S32:
            /* these controls live in separate windows so we won't get to track them here */
            n_test = 0;
            break;
        }

        while(--n_test >= 0)
            if(hit_icon_handle == p_dialog_ictl->riscos.dwi[n_test].icon_handle)
            {
                is_hit = TRUE;
                break;
            }

        if(is_hit)
            return(p_dialog_ictl->dialog_control_id);
    }

    /* indicate no hit detected at this level */
    return(0);
}

/******************************************************************************
*
* feed click event to mlec, who will set caret into parent window
*
******************************************************************************/

_Check_return_
static STATUS
dialog_riscos_mlec_event_Mouse_Click(
    P_DIALOG p_dialog,
    P_DIALOG_ICTL p_dialog_ictl,
    P_DIALOG_ICTL_EDIT_XX p_dialog_ictl_edit_xx,
    _In_        const WimpMouseClickEvent * const p_mouse_click)
{
    if(p_dialog_ictl_edit_xx->riscos.mlec)
    {
        if(!p_dialog_ictl_edit_xx->readonly)
        {
            GDI_POINT mlec_origin_abs;

            if(p_mouse_click->buttons == 0)
                return(STATUS_OK);

            {
            GDI_POINT gdi_org;
            WimpIconBlockWithBitset icon;
            GDI_POINT mlec_origin_rel;

            host_gdi_org_from_screen(&gdi_org, p_dialog->hwnd); /* window work area ABS origin */

            void_WrapOsErrorReporting(wimp_get_icon_state_with_bitset(p_dialog->hwnd, p_dialog_ictl->riscos.dwi[0].icon_handle, &icon));

            host_framed_BBox_trim(&icon.bbox, p_dialog_ictl_edit_xx->border_style);

            mlec_origin_rel.x = icon.bbox.xmin - p_dialog_ictl_edit_xx->riscos.scroll.x;
            mlec_origin_rel.y = icon.bbox.ymax - p_dialog_ictl_edit_xx->riscos.scroll.y;

            mlec_origin_abs.x = gdi_org.x + mlec_origin_rel.x;
            mlec_origin_abs.y = gdi_org.y + mlec_origin_rel.y;
            } /*block*/

            switch(p_mouse_click->buttons)
            {
            case Wimp_MouseButtonSingleSelect:
            case Wimp_MouseButtonSingleAdjust:
                dialog_current_set(p_dialog, p_dialog_ictl->dialog_control_id, 1);
                break;

            default:
                break;
            }

            mlec__Mouse_Click(p_dialog_ictl_edit_xx->riscos.mlec, &mlec_origin_abs, p_mouse_click);
        }
    }

    return(STATUS_OK);
}

_Check_return_
static POINTER_SHAPE
dialog_riscos_mlec_event_determine_pshape(
    P_DIALOG p_dialog,
    P_DIALOG_ICTL p_dialog_ictl,
    P_DIALOG_ICTL_EDIT_XX p_dialog_ictl_edit_xx,
    _In_        const WimpMouseClickEvent * const p_mouse_click)
{
    UNREFERENCED_PARAMETER(p_dialog);
    UNREFERENCED_PARAMETER(p_dialog_ictl);

    if(p_dialog_ictl_edit_xx->riscos.mlec)
    {
        if(!p_dialog_ictl_edit_xx->readonly)
        {
            if(p_mouse_click->buttons == 0)
                return(POINTER_CARET);
        }
    }

    return(POINTER_DEFAULT);
}

static void
dialog_riscos_mlec_enull(
    P_DIALOG p_dialog,
    P_DIALOG_ICTL p_dialog_ictl)
{
    P_DIALOG_ICTL_EDIT_XX p_dialog_ictl_edit_xx = p_dialog_ictl_edit_xx_from(p_dialog_ictl);
    WimpGetPointerInfoBlock pointer_info;
    GDI_POINT gdi_org;
    WimpIconBlockWithBitset icon;
    GDI_POINT mlec_origin_rel, mlec_origin_abs;

    if(NULL == p_dialog_ictl_edit_xx)
        return;

    if(NULL == p_dialog_ictl_edit_xx->riscos.mlec)
        return;

    if(NULL != WrapOsErrorReporting(wimp_get_pointer_info(&pointer_info)))
        return;

    host_gdi_org_from_screen(&gdi_org, p_dialog->hwnd); /* window work area ABS origin */

    void_WrapOsErrorReporting(wimp_get_icon_state_with_bitset(p_dialog->hwnd, p_dialog_ictl->riscos.dwi[0].icon_handle, &icon));

    host_framed_BBox_trim(&icon.bbox, p_dialog_ictl_edit_xx->border_style);

    /* The origin of the text is (icon.bbox.xmin,icon.bbox.ymax), then we scroll it. */

    mlec_origin_rel.x = icon.bbox.xmin - p_dialog_ictl_edit_xx->riscos.scroll.x;
    mlec_origin_rel.y = icon.bbox.ymax - p_dialog_ictl_edit_xx->riscos.scroll.y;

    mlec_origin_abs.x = gdi_org.x + mlec_origin_rel.x;
    mlec_origin_abs.y = gdi_org.y + mlec_origin_rel.y;

    mlec__drag_core(p_dialog_ictl_edit_xx->riscos.mlec, &mlec_origin_abs, &pointer_info);
}

/******************************************************************************
*
* mlec calls this routine to get its client to help with certain events,
* such as text redrawing, placing the caret, key processing etc.
*
******************************************************************************/

_Check_return_
_Ret_valid_
static P_DIALOG
dialog_riscos_mlec_event_common(
    P_DIALOG_ICTL_EDIT_XX p_dialog_ictl_edit_xx,
    /*out*/ P_P_DIALOG_ICTL p_p_dialog_ictl,
    /*out*/ WimpIconBlockWithBitset * p_icon)
{
    const P_DIALOG p_dialog = p_dialog_from_h_dialog(p_dialog_ictl_edit_xx->h_dialog);
    const P_DIALOG_ICTL p_dialog_ictl = p_dialog_ictl_from_control_id(p_dialog, p_dialog_ictl_edit_xx->dialog_control_id);

    *p_p_dialog_ictl = p_dialog_ictl;

    void_WrapOsErrorReporting(wimp_get_icon_state_with_bitset(p_dialog->hwnd, p_dialog_ictl->riscos.dwi[0].icon_handle, p_icon));

    return(p_dialog);
}

static STATUS
dialog_riscos_mlec_CODE_KEY(
    P_DIALOG_ICTL_EDIT_XX p_dialog_ictl_edit_xx,
    P_KMAP_CODE p_kmap_code)
{
    UNREFERENCED_PARAMETER(p_dialog_ictl_edit_xx);

    switch(*p_kmap_code)
    {
    case KMAP_CODE_ADDED_ALT | 'U':
        *p_kmap_code = KMAP_FUNC_CEND;    /* delete line */
        break;

    case KMAP_CODE_ADDED_ALT | 'X':
        *p_kmap_code = KMAP_FUNC_SDELETE; /* cut to clipboard */
        break;

    case KMAP_CODE_ADDED_ALT | 'C':
        *p_kmap_code = KMAP_FUNC_CINSERT; /* copy to clipboard */
        break;

    case KMAP_CODE_ADDED_ALT | 'V':
        *p_kmap_code = KMAP_FUNC_SINSERT; /* paste from clipboard */
        break;

    default:
        break;
    }

    return(STATUS_OK);
}

/* The mlec is trying to tell us that part of its text needs repainting. */
/* We must call wimp_force_redraw for each edit control in each view,    */
/* taking into account the scroll state for each one.                    */

static STATUS
dialog_riscos_mlec_CODE_UPDATELATER(
    P_DIALOG_ICTL_EDIT_XX p_dialog_ictl_edit_xx,
    const WimpRedrawWindowBlock * const p_redraw_window_block)
{
    ARRAY_INDEX i = array_elements(&dialog_statics.handles);

    while(--i >= 0)
    {
        const P_DIALOG p_dialog = array_ptr(&dialog_statics.handles, DIALOG, i);

        if(p_dialog->h_dialog == p_dialog_ictl_edit_xx->h_dialog)
            dialog_riscos_mlec_update(p_dialog, p_dialog_ictl_edit_xx, &p_redraw_window_block->visible_area, 1);
    }

    return(STATUS_OK);
}

/* The mlec is trying to tell us that part of its text needs highlighting by EORing.  */
/* We must call wimp_update_window() for each edit control in each view.              */
/* From within the update loop, call mlec_area_update(), then mlec will do it for us. */
/* We could just call wimp_force_redraw() for each view instead, but this flickers.   */

static STATUS
dialog_riscos_mlec_CODE_UPDATENOW(
    P_DIALOG_ICTL_EDIT_XX p_dialog_ictl_edit_xx,
    const WimpRedrawWindowBlock * const p_redraw_window_block)
{
    ARRAY_INDEX i = array_elements(&dialog_statics.handles);

    while(--i >= 0)
    {
        const P_DIALOG p_dialog = array_ptr(&dialog_statics.handles, DIALOG, i);

        if(p_dialog->h_dialog == p_dialog_ictl_edit_xx->h_dialog)
            dialog_riscos_mlec_update(p_dialog, p_dialog_ictl_edit_xx, &p_redraw_window_block->visible_area, p_dialog_ictl_edit_xx->always_update_later);
    }

    return(STATUS_OK);
}

static STATUS
dialog_riscos_mlec_CODE_PLACECARET(
    P_DIALOG_ICTL_EDIT_XX p_dialog_ictl_edit_xx,
    const WimpCaret * const p_caret)
{
    WimpCaret carrot = *p_caret;
    WimpIconBlockWithBitset icon;
    P_DIALOG_ICTL p_dialog_ictl;
    const P_DIALOG p_dialog = dialog_riscos_mlec_event_common(p_dialog_ictl_edit_xx, &p_dialog_ictl, &icon);

    if(NULL != p_dialog)
    {
        if(!p_dialog_ictl_edit_xx->readonly)
        {
            WimpCaret current;
            GDI_POINT mlec_origin_rel;

            void_WrapOsErrorReporting(winx_get_caret_position(&current));

            if((current.window_handle == p_dialog->hwnd) && (p_dialog->current_dialog_control_id == p_dialog_ictl->dialog_control_id))
            {
                trace_0(TRACE_RISCOS_HOST, TEXT("window and control own input focus "));

                /* Take the WimpCaret passed by mlec, add the origin (icon.box.x0,icon.box.y1), then scroll it. */
                host_framed_BBox_trim(&icon.bbox, p_dialog_ictl_edit_xx->border_style);

                mlec_origin_rel.x = icon.bbox.xmin - p_dialog_ictl_edit_xx->riscos.scroll.x;
                mlec_origin_rel.y = icon.bbox.ymax - p_dialog_ictl_edit_xx->riscos.scroll.y;

                carrot.window_handle = p_dialog->hwnd;
                carrot.xoffset += mlec_origin_rel.x;
                carrot.yoffset += mlec_origin_rel.y;

                if(
                   (current.icon_handle != carrot.icon_handle) || /* test whether caret is in some other icon too */
                   (current.xoffset != carrot.xoffset) ||
                   (current.yoffset != carrot.yoffset) ||
                   (current.height != carrot.height) /* cos this field holds caret (in)visible bit */ )
                {
                    trace_3(TRACE_RISCOS_HOST, TEXT(" place caret (") S32_TFMT TEXT(",") S32_TFMT TEXT(",") S32_TFMT TEXT(")"), carrot.xoffset, carrot.yoffset, carrot.height);
                    void_WrapOsErrorReporting(winx_set_caret_position(&carrot));
                }
                else
                {
                    trace_0(TRACE_RISCOS_HOST, TEXT(" no action (caret already positioned)"));
                }
            }
            else
            {
                trace_0(TRACE_RISCOS_HOST, TEXT(" no action (focus belongs elsewhere)"));
            }
        }
    }

    return(STATUS_OK);
}

static STATUS
dialog_riscos_mlec_CODE_PASTEATCARET(
    P_DIALOG_ICTL_EDIT_XX p_dialog_ictl_edit_xx,
    P_ANY p_eventdata)
{
    WimpIconBlockWithBitset icon;
    P_DIALOG_ICTL p_dialog_ictl;
    const P_DIALOG p_dialog = dialog_riscos_mlec_event_common(p_dialog_ictl_edit_xx, &p_dialog_ictl, &icon);

    UNREFERENCED_PARAMETER(p_eventdata);

    if(NULL != p_dialog)
    {
        if(!p_dialog_ictl_edit_xx->readonly)
        {
            /* initiate paste from external clipboard data source */
            static const T5_FILETYPE t5_filetypes[] =
            {
                FILETYPE_TEXT
            };

            WimpCaret carrot;
            WimpCaret current;
            GDI_POINT mlec_origin_rel;

            void_WrapOsErrorReporting(winx_get_caret_position(&current));

            if((current.window_handle == p_dialog->hwnd) && (p_dialog->current_dialog_control_id == p_dialog_ictl->dialog_control_id))
            {
                trace_0(TRACE_RISCOS_HOST, TEXT("window and control own input focus "));
                carrot = current;

                /* Take the WimpCaret passed by mlec, add the origin (icon.box.x0,icon.box.y1), then scroll it. */
                host_framed_BBox_trim(&icon.bbox, p_dialog_ictl_edit_xx->border_style);

                mlec_origin_rel.x = icon.bbox.xmin - p_dialog_ictl_edit_xx->riscos.scroll.x;
                mlec_origin_rel.y = icon.bbox.ymax - p_dialog_ictl_edit_xx->riscos.scroll.y;

                carrot.xoffset += mlec_origin_rel.x;
                carrot.yoffset += mlec_origin_rel.y;

                host_paste_from_global_clipboard(p_dialog->hwnd, p_dialog_ictl->riscos.dwi[0].icon_handle,
                                                 carrot.xoffset, carrot.yoffset,
                                                 t5_filetypes, elemof32(t5_filetypes));

                return(STATUS_DONE);
            }
            else
            {
                trace_0(TRACE_RISCOS_HOST, TEXT(" no action (focus belongs elsewhere)"));
            }
        }
    }

    return(STATUS_OK);
}

/* query the scroll and size of the primary view of this control */

static STATUS
dialog_riscos_mlec_CODE_QUERYSCROLL(
    P_DIALOG_ICTL_EDIT_XX p_dialog_ictl_edit_xx,
    P_MLEC_QUERYSCROLL p_mlec_queryscroll)
{
    WimpIconBlockWithBitset icon;
    P_DIALOG_ICTL p_dialog_ictl;
    P_DIALOG p_dialog;

    if(NULL != (p_dialog = dialog_riscos_mlec_event_common(p_dialog_ictl_edit_xx, &p_dialog_ictl, &icon)))
    {
        host_framed_BBox_trim(&icon.bbox, p_dialog_ictl_edit_xx->border_style);

        p_mlec_queryscroll->scroll = p_dialog_ictl_edit_xx->riscos.scroll;

        p_mlec_queryscroll->visible.x = BBox_width(&icon.bbox);
        p_mlec_queryscroll->visible.y = BBox_height(&icon.bbox);

        p_mlec_queryscroll->use = 1;
    }

    return(STATUS_OK);
}

/* scroll the primary view of this control */

static STATUS
dialog_riscos_mlec_CODE_DOSCROLL(
    P_DIALOG_ICTL_EDIT_XX p_dialog_ictl_edit_xx,
    P_MLEC_DOSCROLL p_mlec_doscroll)
{
    WimpIconBlockWithBitset icon;
    P_DIALOG_ICTL p_dialog_ictl;
    P_DIALOG p_dialog;

    if(NULL != (p_dialog = dialog_riscos_mlec_event_common(p_dialog_ictl_edit_xx, &p_dialog_ictl, &icon)))
    {
        if( (p_dialog_ictl_edit_xx->riscos.scroll.x != p_mlec_doscroll->scroll.x) ||
            (p_dialog_ictl_edit_xx->riscos.scroll.y != p_mlec_doscroll->scroll.y) )
        {
            host_framed_BBox_trim(&icon.bbox, p_dialog_ictl_edit_xx->border_style);

            void_WrapOsErrorReporting(wimp_force_redraw_BBox(p_dialog->hwnd, &icon.bbox));

            p_dialog_ictl_edit_xx->riscos.scroll = p_mlec_doscroll->scroll;
        }
    }

    return(STATUS_OK);
}

/* start a drag in the primary view of this control */

static STATUS
dialog_riscos_mlec_CODE_STARTDRAG(
    P_DIALOG_ICTL_EDIT_XX p_dialog_ictl_edit_xx,
    WimpDragBox * p_dragstr)
{
    WimpIconBlockWithBitset icon;
    P_DIALOG_ICTL p_dialog_ictl;
    P_DIALOG p_dialog;

    if(NULL != (p_dialog = dialog_riscos_mlec_event_common(p_dialog_ictl_edit_xx, &p_dialog_ictl, &icon)))
    {
        GDI_POINT gdi_org;

        /* drag boxes specified ABS even if given a window */
        host_gdi_org_from_screen(&gdi_org, p_dialog->hwnd); /* window work area ABS origin */

        host_framed_BBox_trim(&icon.bbox, p_dialog_ictl_edit_xx->border_style);

        p_dragstr->wimp_window = p_dialog->hwnd;
        p_dragstr->parent_box.xmin = icon.bbox.xmin + gdi_org.x;
        p_dragstr->parent_box.ymin = icon.bbox.ymin + gdi_org.y;
        p_dragstr->parent_box.xmax = icon.bbox.xmax + gdi_org.x;
        p_dragstr->parent_box.ymax = icon.bbox.ymax + gdi_org.y;

        return(MLEC_EVENT_DOSTARTDRAG);
    }

    return(STATUS_OK);
}

/* the drag has been started, so set up our null handler for the primary view */

static STATUS
dialog_riscos_mlec_CODE_STARTEDDRAG(
    P_DIALOG_ICTL_EDIT_XX p_dialog_ictl_edit_xx,
    P_ANY p_eventdata)
{
#if 0
    const P_DIALOG p_dialog = p_dialog_from_h_dialog(p_dialog_ictl_edit_xx->h_dialog);
    const H_DIALOG h_dialog = p_dialog->h_current;

    assert(p_dialog->h_current);

    if(status_fail(status_wrap(null_events_start(p_docu_from_docno(p_dialog->docno), DIALOG_CMD_CODE_NULL_EVENT, dialog, (P_ANY) h_dialog))))
    {
        /*winx_drag_abort()*/;
    }
    else
    {
        p_dialog = p_dialog_from_h_dialog(h_dialog);

        p_dialog->has_nulls = 1;
    }
#else
    UNREFERENCED_PARAMETER(p_dialog_ictl_edit_xx);
#endif

    UNREFERENCED_PARAMETER(p_eventdata);

    return(STATUS_OK);
}

/* the edit control's contents have changed, so read and validate it iff not caused by this bit of code */

static STATUS
dialog_riscos_mlec_CODE_UPDATE(
    P_DIALOG_ICTL_EDIT_XX p_dialog_ictl_edit_xx,
    P_ANY p_eventdata)
{
    WimpIconBlockWithBitset icon;
    P_DIALOG_ICTL p_dialog_ictl;
    P_DIALOG p_dialog;

    UNREFERENCED_PARAMETER(p_eventdata);

    if(NULL != (p_dialog = dialog_riscos_mlec_event_common(p_dialog_ictl_edit_xx, &p_dialog_ictl, &icon)))
    {
        if(0 == p_dialog_ictl->bits.in_update)
        {
            UI_TEXT ui_text;
            DIALOG_CMD_CTL_STATE_SET dialog_cmd_ctl_state_set;
            STATUS status;

            status_assert(ui_text_from_ictl_edit_xx(&ui_text, p_dialog_ictl_edit_xx));

            /* state change will want to rejig views iff chars rejected */
            p_dialog_ictl->bits.force_update = (ui_text_validate(&ui_text, p_dialog_ictl_edit_xx->p_bitmap_validation) != 0);

            msgclr(dialog_cmd_ctl_state_set);

            switch(p_dialog_ictl->dialog_control_type)
            {
            default: default_unhandled();
#if CHECKING
            case DIALOG_CONTROL_EDIT:
#endif
                dialog_cmd_ctl_state_set.state.edit.ui_text = ui_text;
                break;

            case DIALOG_CONTROL_BUMP_F64:
                {
                PC_USTR i_ustr = ui_text_ustr(&ui_text);
                PC_USTR e_ustr;
                SS_RECOG_CONTEXT ss_recog_context;
                ss_recog_context_push(&ss_recog_context);
                errno = 0;
                dialog_cmd_ctl_state_set.state.bump_f64 = ui_strtod(i_ustr, &e_ustr);
                if(errno)
                    dialog_cmd_ctl_state_set.state.bump_f64 = 0.0;
                ss_recog_context_pull(&ss_recog_context);
                break;
                }

            case DIALOG_CONTROL_BUMP_S32:
                {
                PC_USTR i_ustr = ui_text_ustr(&ui_text);
                P_USTR e_ustr;
                errno = 0;
                dialog_cmd_ctl_state_set.state.bump_s32 = ui_strtol(i_ustr, &e_ustr, 10);
                if(errno)
                    dialog_cmd_ctl_state_set.state.bump_s32 = 0;
                break;
                }

            case DIALOG_CONTROL_COMBO_TEXT:
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
            {
                status_assert(status);
                reperr_null(status);
                status = STATUS_OK;
            }

            p_dialog_ictl->bits.force_update = 0;

            ui_text_dispose(&ui_text);
        }
    }

    return(STATUS_OK);
}

static
mlec_event_proto(dialog_riscos_mlec_event_handler, rc, handle, p_eventdata)
{
    switch(rc)
    {
    case MLEC_CODE_KEY:
        return(dialog_riscos_mlec_CODE_KEY((P_DIALOG_ICTL_EDIT_XX) handle, (P_KMAP_CODE) p_eventdata));

    case MLEC_CODE_UPDATELATER:
        return(dialog_riscos_mlec_CODE_UPDATELATER((P_DIALOG_ICTL_EDIT_XX) handle, (const WimpRedrawWindowBlock *) p_eventdata));

    case MLEC_CODE_UPDATENOW:
        return(dialog_riscos_mlec_CODE_UPDATENOW((P_DIALOG_ICTL_EDIT_XX) handle, (const WimpRedrawWindowBlock *) p_eventdata));

    case MLEC_CODE_PLACECARET:
        return(dialog_riscos_mlec_CODE_PLACECARET((P_DIALOG_ICTL_EDIT_XX) handle, (const WimpCaret *) p_eventdata));

    case MLEC_CODE_PASTEATCARET:
        return(dialog_riscos_mlec_CODE_PASTEATCARET((P_DIALOG_ICTL_EDIT_XX) handle, p_eventdata));

    case MLEC_CODE_QUERYSCROLL:
        return(dialog_riscos_mlec_CODE_QUERYSCROLL((P_DIALOG_ICTL_EDIT_XX) handle, (P_MLEC_QUERYSCROLL) p_eventdata));

    case MLEC_CODE_DOSCROLL:
        return(dialog_riscos_mlec_CODE_DOSCROLL((P_DIALOG_ICTL_EDIT_XX) handle, (P_MLEC_DOSCROLL) p_eventdata));

    case MLEC_CODE_STARTDRAG:
        return(dialog_riscos_mlec_CODE_STARTDRAG((P_DIALOG_ICTL_EDIT_XX) handle, (WimpDragBox *) p_eventdata));

    case MLEC_CODE_STARTEDDRAG:
        return(dialog_riscos_mlec_CODE_STARTEDDRAG((P_DIALOG_ICTL_EDIT_XX) handle, p_eventdata));

    case MLEC_CODE_UPDATE:
        return(dialog_riscos_mlec_CODE_UPDATE((P_DIALOG_ICTL_EDIT_XX) handle, p_eventdata));

    default:
        return(STATUS_OK);
    }
}

static void
dialog_riscos_mlec_update(
    P_DIALOG p_dialog,
    P_DIALOG_ICTL_EDIT_XX p_dialog_ictl_edit_xx,
    _InRef_     PC_BBox p_update_bbox,
    _InVal_     BOOL later)
{
    const P_DIALOG_ICTL p_dialog_ictl = p_dialog_ictl_from_control_id(p_dialog, p_dialog_ictl_edit_xx->dialog_control_id);
    WimpIconBlockWithBitset icon;
    GDI_POINT mlec_origin_rel;
    WimpRedrawWindowBlock redraw_window_block;

    void_WrapOsErrorReporting(wimp_get_icon_state_with_bitset(p_dialog->hwnd, p_dialog_ictl->riscos.dwi[0].icon_handle, &icon));

    host_framed_BBox_trim(&icon.bbox, p_dialog_ictl_edit_xx->border_style);

    /* The origin of the text is (icon.bbox.xmin,icon.bbox.ymax), then we scroll it. */

    mlec_origin_rel.x = icon.bbox.xmin - p_dialog_ictl_edit_xx->riscos.scroll.x;
    mlec_origin_rel.y = icon.bbox.ymax - p_dialog_ictl_edit_xx->riscos.scroll.y;

    redraw_window_block.visible_area.xmin = (mlec_origin_rel.x + p_update_bbox->xmin);
    redraw_window_block.visible_area.ymin = (mlec_origin_rel.y + p_update_bbox->ymin);
    redraw_window_block.visible_area.xmax = (mlec_origin_rel.x + p_update_bbox->xmax);
    redraw_window_block.visible_area.ymax = (mlec_origin_rel.y + p_update_bbox->ymax);

    /* clip to the icon border */
    if( redraw_window_block.visible_area.xmin < icon.bbox.xmin)
        redraw_window_block.visible_area.xmin = icon.bbox.xmin;
    if( redraw_window_block.visible_area.ymin < icon.bbox.ymin)
        redraw_window_block.visible_area.ymin = icon.bbox.ymin;
    if( redraw_window_block.visible_area.xmax > icon.bbox.xmax)
        redraw_window_block.visible_area.xmax = icon.bbox.xmax;
    if( redraw_window_block.visible_area.ymax > icon.bbox.ymax)
        redraw_window_block.visible_area.ymax = icon.bbox.ymax;

    if(later)
    {
        void_WrapOsErrorReporting(wimp_force_redraw_BBox(p_dialog->hwnd, &redraw_window_block.visible_area));
    }
    else
    {
        int wimp_more = 0;

        redraw_window_block.window_handle = p_dialog->hwnd;

        if(NULL != WrapOsErrorReporting(wimp_update_window(&redraw_window_block, &wimp_more)))
            wimp_more = 0;

        while(0 != wimp_more)
        {
            GDI_POINT gdi_org;
            GDI_POINT mlec_origin_abs;

            gdi_org.x = work_area_origin_x_from_visible_area_and_scroll(&redraw_window_block); /* window work area ABS origin */
            gdi_org.y = work_area_origin_y_from_visible_area_and_scroll(&redraw_window_block);

            mlec_origin_abs.x = gdi_org.x + mlec_origin_rel.x;
            mlec_origin_abs.y = gdi_org.y + mlec_origin_rel.y;

            /* no need to clip the screen redraw area to the icon boundary */
            /* because we clipped the rectangle fed to wimp_update_window()  */

            if(NULL != p_dialog_ictl_edit_xx->riscos.mlec)
                mlec_area_update(p_dialog_ictl_edit_xx->riscos.mlec, &mlec_origin_abs, (PC_GDI_BOX) &redraw_window_block.redraw_area);

            if(NULL != WrapOsErrorReporting(wimp_get_rectangle(&redraw_window_block, &wimp_more)))
                wimp_more = 0;
        }
    }
}

/******************************************************************************
*
* ensure dropdown part of this combobox is shown
*
******************************************************************************/

static void
dialog_riscos_redraw_gwindow_restore(
    P_DIALOG_RISCOS_REDRAW_WINDOW p_dialog_riscos_redraw_window)
{
    const PC_BBox redraw_area = &p_dialog_riscos_redraw_window->redraw_window_block.redraw_area;

    void_WrapOsErrorChecking(
        riscos_vdu_define_graphics_window(
            redraw_area->xmin,
            redraw_area->ymin,
            redraw_area->xmax - 1,
            redraw_area->ymax - 1));
}

/******************************************************************************
*
* remove any ampersand characters from caption strings as per Windows
*
******************************************************************************/

_Check_return_
extern STATUS
dialog_riscos_remove_escape(
    _InoutRef_  P_DIALOG_ICTL p_dialog_ictl,
    _InRef_     PC_UI_TEXT p_ui_text)
{
    SBCHAR hot_u8 = 0;

    if(ui_text_is_blank(p_ui_text))
    {
        p_dialog_ictl->riscos.caption = NULL;
        p_dialog_ictl->riscos.hot_key = hot_u8;
        return(STATUS_OK);
    }

    status_return(ui_text_copy_as_sbstr(&p_dialog_ictl->riscos.caption, p_ui_text));

    {
    P_SBSTR out = p_dialog_ictl->riscos.caption;
    PC_SBSTR in = out;
    SBCHAR ch;
    BOOL in_escape = 0;

    do  {
        ch = *in++;

        if(in_escape)
        {
            if(CH_AMPERSAND != ch)
                hot_u8 = sbchar_toupper(ch);
        }
        else
        {
            if(CH_AMPERSAND == ch)
            {
                in_escape = 1;
                continue;
            }
        }

        *out++ = ch;
    }
    while(CH_NULL != ch);
    } /*block*/

    return(hot_u8);
}

/******************************************************************************
*
* because we pass a substructure address as our handle to mlec
* then we need to register/deregister around realloc of parent ictl
*
******************************************************************************/

extern void
dialog_riscos_tweak_edit_controls(
    P_DIALOG_ICTL_GROUP p_ictl_group,
    _InVal_     BOOL add)
{
    const ARRAY_INDEX n_ictls = n_ictls_from_group(p_ictl_group);
    ARRAY_INDEX i;
    P_DIALOG_ICTL p_dialog_ictl = p_dialog_ictls_from_group(p_ictl_group, n_ictls);

    for(i = 0; i < n_ictls; ++i, ++p_dialog_ictl)
    {
        const P_DIALOG_ICTL_EDIT_XX p_dialog_ictl_edit_xx = p_dialog_ictl_edit_xx_from(p_dialog_ictl);

        if(NULL == p_dialog_ictl_edit_xx)
            continue;

        if(!p_dialog_ictl_edit_xx->riscos.mlec)
            continue;

        mlec_attach_eventhandler(p_dialog_ictl_edit_xx->riscos.mlec, dialog_riscos_mlec_event_handler, p_dialog_ictl_edit_xx, add);
    }
}

/******************************************************************************
*
* redraw the control as directed
*
******************************************************************************/

_Check_return_
_Ret_maybenull_
static inline _kernel_oserror *
swi_wimp_setcolour(int colour)
{
    _kernel_swi_regs rs;
    rs.r[0] = colour;
    return(_kernel_swi(/*Wimp_SetColour*/ 0x000400E6, &rs, &rs));
}

_Check_return_
static STATUS
part_control_redraw(
    P_DIALOG_CONTROL_REDRAW p_dialog_control_redraw,
    _In_        FRAMED_BOX_STYLE b,
    _InRef_     PC_DIALOG_WIMP_I p_i,
    _InVal_     DIALOG_CONTROL_TYPE dialog_control_type)
{
    P_DIALOG p_dialog;
    P_DIALOG_ICTL p_dialog_ictl;
    GDI_BOX control_screen_border, control_screen_clipped, control_screen;
    WimpIconBlockWithBitset icon;
    BOOL disabled;
    BOOL plot_edit;
    BOOL plot_sprite;
    BOOL plot_text;
    BOOL plot_user;
    RGB rgb;
    P_RGB p_fill_colour;
    S32 fill_colour, text_colour, normal_fill_colour;

    if((p_i->icon_handle == BAD_WIMP_I) || p_i->redrawn_by_wimp)
        return(STATUS_OK);

    p_dialog = p_dialog_control_redraw->p_dialog;
    p_dialog_ictl = p_dialog_control_redraw->p_dialog_ictl;

    disabled = p_dialog_ictl->bits.disabled;

    void_WrapOsErrorReporting(wimp_get_icon_state_with_bitset(p_dialog->hwnd, p_i->icon_handle, &icon));

    assert(icon.flags.bits.redraw);

    /* now make control's full box into absolute screen coordinates */
    control_screen_border.x0 = p_dialog_control_redraw->dialog_riscos_redraw_window.redraw_context.gdi_org.x + icon.bbox.xmin;
    control_screen_border.y0 = p_dialog_control_redraw->dialog_riscos_redraw_window.redraw_context.gdi_org.y + icon.bbox.ymin;
    control_screen_border.x1 = p_dialog_control_redraw->dialog_riscos_redraw_window.redraw_context.gdi_org.x + icon.bbox.xmax;
    control_screen_border.y1 = p_dialog_control_redraw->dialog_riscos_redraw_window.redraw_context.gdi_org.y + icon.bbox.ymax;

    /* does clip box returned from Window Manager intersect this control? */
    if(gr_box_intersection((P_GR_BOX) &control_screen_clipped,
                           (PC_GR_BOX) &control_screen_border,
                           (PC_GR_BOX) &p_dialog_control_redraw->dialog_riscos_redraw_window.redraw_window_block.redraw_area) <= 0)
        return(STATUS_OK);

    /* border */
    if(disabled)
        b |= FRAMED_BOX_DISABLED; /* NB. also needed below */
    host_framed_box_paint_frame(&control_screen_border, b);

    plot_edit = 0;
    plot_sprite = 0;
    plot_text = 0;
    plot_user = 0;
    p_fill_colour = NULL;
    fill_colour = -1;
    text_colour = icon.flags.bits.fg_colour;
    normal_fill_colour = icon.flags.bits.bg_colour;

    switch(dialog_control_type)
    {
    default: default_unhandled(); return(STATUS_OK);
#if CHECKING
    case DIALOG_CONTROL_LIST_S32:
    case DIALOG_CONTROL_COMBO_S32:
        assert0();

        /*FALLTHRU*/

    case DIALOG_CONTROL_GROUPBOX:
    case DIALOG_CONTROL_GROUPBOX_PX100:
    case DIALOG_CONTROL_LIST_TEXT:
        return(STATUS_OK);
#endif

    case DIALOG_CONTROL_EDIT:
    case DIALOG_CONTROL_BUMP_S32:
    case DIALOG_CONTROL_COMBO_TEXT:
        plot_edit = 1;
        text_colour = 0x07;
        fill_colour = 0x00;
        break;

    case DIALOG_CONTROL_BUMP_S32_PX100:
    case DIALOG_CONTROL_BUMP_S32_PX200:
        plot_sprite = 1;
        fill_colour = normal_fill_colour;
        break;

    case DIALOG_CONTROL_COMBO_TEXT + 0x100:
        plot_sprite = 1;
        break;

    case DIALOG_CONTROL_STATICTEXT:
        plot_text = 1;
        break;

    case DIALOG_CONTROL_STATICFRAME:
        plot_text = 1;
        p_fill_colour = p_dialog_control_redraw->p_dialog_control_data.staticframe->p_back_colour;
        break;

    case DIALOG_CONTROL_PUSHBUTTON:
        plot_text = 1;
        fill_colour = normal_fill_colour;
        break;

    case DIALOG_CONTROL_PUSHPICTURE:
        plot_sprite = 1;
        p_fill_colour = p_dialog_control_redraw->p_dialog_control_data.pushpicture->p_back_colour;
        if(NULL == p_fill_colour)
            fill_colour = normal_fill_colour;
        break;

    case DIALOG_CONTROL_RADIOBUTTON:
        if(!p_dialog_control_redraw->p_dialog_control_data.radiobutton->bits.left_text)
            icon.bbox.xmin += (int) (6 /*DIALOG_RADIOGAP_H / PIXITS_PER_RISCOS*/);

        plot_text = 1;
        /*fill_colour = normal_fill_colour;*/
        break;

    case DIALOG_CONTROL_RADIOPICTURE:
        plot_sprite = 1;

        fill_colour = p_dialog_ictl->bits.radiobutton_active ? 2 : normal_fill_colour;
        break;

    case DIALOG_CONTROL_CHECKBOX:
        if(!p_dialog_control_redraw->p_dialog_control_data.checkbox->bits.left_text)
            icon.bbox.xmin += (int) (6 /*DIALOG_CHECKGAP_H / PIXITS_PER_RISCOS*/);

        plot_text = 1;
        /*fill_colour = normal_fill_colour;*/
        break;

    case DIALOG_CONTROL_CHECKPICTURE:
        plot_sprite = 1;

        fill_colour = (p_dialog_ictl->state.checkbox == DIALOG_BUTTONSTATE_ON) ? 2 : normal_fill_colour;
        break;

#ifdef DIALOG_HAS_TRISTATE
    case DIALOG_CONTROL_TRISTATE:
        if(!p_dialog_control_redraw->p_dialog_control_data.tristate->bits.left_text)
            icon.box.x0 += DIALOG_CHECKGAP_H / PIXITS_PER_RISCOS;

        plot_text = 1;
        /*fill_colour = normal_fill_colour;*/
        break;
#endif

    case DIALOG_CONTROL_TRIPICTURE:
        plot_sprite = 1;

        fill_colour = (p_dialog_ictl->state.tristate == DIALOG_BUTTONSTATE_ON) ? 2 : normal_fill_colour;
        break;

    case DIALOG_CONTROL_USER:
        plot_user = 1;

        {
        PC_RGB p_rgb = p_dialog_ictl->p_dialog_control_data.user->p_back_colour;

        if(NULL != p_rgb)
            rgb = *p_rgb;
        else
        {
            if(p_dialog_ictl->p_dialog_control_data.user->bits.state_is_rgb)
                rgb = p_dialog_ictl->state.user.rgb;
            if(p_dialog_ictl->p_dialog_control_data.user->bits.state_is_rgb_stash_index)
                rgb = rgb_stash[p_dialog_ictl->state.user.u32];
            else
                rgb_set(&rgb, 0xFF, 0xFF, 0xFF);
        }

        if(p_dialog_ictl->bits.disabled)
        {
            /* bias towards white (ha ha maybe convert to HSV, average V and reconvert to RGB) */
            rgb.r = (U8) ((0x200 + (S32) rgb.r) / 3);
            rgb.g = (U8) ((0x200 + (S32) rgb.g) / 3);
            rgb.b = (U8) ((0x200 + (S32) rgb.b) / 3);
        }
        } /*block*/

        p_fill_colour = &rgb;
        break;
    }

    /* contents */
    host_framed_BBox_trim(&icon.bbox, b);

    /* now make control's inner box into absolute screen coordinates */
    control_screen.x0 = p_dialog_control_redraw->dialog_riscos_redraw_window.redraw_context.gdi_org.x + icon.bbox.xmin;
    control_screen.y0 = p_dialog_control_redraw->dialog_riscos_redraw_window.redraw_context.gdi_org.y + icon.bbox.ymin;
    control_screen.x1 = p_dialog_control_redraw->dialog_riscos_redraw_window.redraw_context.gdi_org.x + icon.bbox.xmax;
    control_screen.y1 = p_dialog_control_redraw->dialog_riscos_redraw_window.redraw_context.gdi_org.y + icon.bbox.ymax;

    if(gr_box_intersection((P_GR_BOX) &control_screen_clipped,
                           (PC_GR_BOX) &control_screen,
                           (PC_GR_BOX) &p_dialog_control_redraw->dialog_riscos_redraw_window.redraw_window_block.redraw_area) <= 0)
        return(STATUS_OK);

    /* set the graphics window up */
    void_WrapOsErrorChecking(
        riscos_vdu_define_graphics_window(
            control_screen_clipped.x0,
            control_screen_clipped.y0,
            control_screen_clipped.x1 - 1,
            control_screen_clipped.y1 - 1));

    if(fill_colour != -1)
    {
        void_WrapOsErrorReporting(swi_wimp_setcolour(0x80 | (U8) (disabled ? 0 : fill_colour))); /* stuff caching */
        host_invalidate_cache(HIC_BG);
        host_clg();
    }
    else if(p_fill_colour)
    {
        if(host_setbgcolour(p_fill_colour))
            host_clg();
    }

    if(plot_sprite)
    {
        assert(!icon.flags.bits.text);
        assert(!icon.flags.bits.sprite);
        assert(!icon.flags.bits.border);
        assert(!icon.flags.bits.disabled);

        icon.flags.bits.sprite = 1;
        icon.flags.bits.disabled = disabled;

        host_ploticon(&icon);
    }

    if(plot_text && icon.flags.bits.indirect)
    {
        P_U8 p_u8 = icon.data.it.buffer;
        S32 len = strlen(p_u8);

        if(len)
        {
#if 0
            GR_POINT point;
            S32 spare;
#endif

            if(disabled)
                text_colour = DISABLED_BLACK_ON_GREY;

#if 0
            void_WrapOsErrorReporting(swi_wimp_setcolour(text_colour));
            host_invalidate_cache(HIC_FG);

            assert(icon.flags.bits.vert_centre);
            {
                spare = (control_screen.y1 - control_screen.y0) - 32;
                point.y = control_screen.y1 - spare/2 - host_modevar_cache_current.dy;
            }

            if(icon.flags.bits.horz_centre)
            {
                spare = (control_screen.x1 - control_screen.x0) - 16 * len;
                point.x = control_screen.x0 + spare/2;
            }
            else if(icon_flags.bits.right_just)
                point.x = control_screen.x1 - 16 * len;
            else
                point.x = control_screen.x0;

            void_WrapOsErrorChecking(
                bbc_move(point.x, point.y));

            void_WrapOsErrorChecking(bbc_write0(p_u8));
#endif

#if 1
            {
            int horz_shift = 6; /* just get right for 2 OS unit per pixel modes */

            if(icon.flags.bits.horz_centre)
            { /*EMPTY*/ }
            else if(icon.flags.bits.right_just)
                icon.bbox.xmax += horz_shift;
            else
                icon.bbox.xmin -= horz_shift; /* Window Manager plots text too far right */
            } /*block*/

            icon.flags.bits.fg_colour = UBF_PACK(text_colour);

            assert(!icon.flags.bits.text);
            assert(!icon.flags.bits.sprite);
            assert(!icon.flags.bits.border);
            assert(!icon.flags.bits.disabled);

            icon.flags.bits.text = 1;

            host_ploticon(&icon);
#endif
        }
    }

    if(plot_edit)
    {
        /* dispatch redraw events to edit parts of controls after clipping at this level
         * and setting graphics window etc
        */
        P_DIALOG_ICTL_EDIT_XX p_dialog_ictl_edit_xx = p_dialog_control_redraw->p_dialog_ictl_edit_xx;
        GDI_POINT mlec_origin_abs;

        if(disabled)
            text_colour = DISABLED_BLACK_ON_WHITE;

        /* origin of text in EDIT control in absolute screen coordinates */
        mlec_origin_abs.x = control_screen.x0 - p_dialog_ictl_edit_xx->riscos.scroll.x;
        mlec_origin_abs.y = control_screen.y1 - p_dialog_ictl_edit_xx->riscos.scroll.y;

        if(NULL != p_dialog_ictl_edit_xx->riscos.mlec)
        {
            mlec_attribute_set(p_dialog_ictl_edit_xx->riscos.mlec, MLEC_ATTRIBUTE_FG_RGB, * (PC_S32) &rgb_stash[text_colour]);

            mlec__redraw_core(p_dialog_ictl_edit_xx->riscos.mlec, &mlec_origin_abs, &control_screen_clipped); /* <<< can't produce an error!? */
        }
    }

    if(plot_user)
    {
        /* dispatch redraw events to USER buttons after clipping at this level
         * and setting graphics window etc
        */
        P_PROC_DIALOG_EVENT p_proc_client;
        DIALOG_MSG_CTL_USER_REDRAW dialog_msg_ctl_user_redraw;
        msgclr(dialog_msg_ctl_user_redraw);

        if(NULL != (p_proc_client = dialog_find_handler(p_dialog, p_dialog_ictl->dialog_control_id, &dialog_msg_ctl_user_redraw.client_handle)))
        {
            /*const P_REDRAW_CONTEXT p_redraw_context = &dialog_msg_ctl_user_redraw.redraw_context;*/

            DIALOG_MSG_CTL_HDR_from_dialog_ictl(dialog_msg_ctl_user_redraw, p_dialog, p_dialog_ictl);

            dialog_msg_ctl_user_redraw.redraw_context = p_dialog_control_redraw->dialog_riscos_redraw_window.redraw_context;

            /*host_paint_start(p_redraw_context);*/

            {
            GDI_RECT gdi_rect;
            gdi_rect.tl.x = control_screen.x0;
            gdi_rect.br.y = control_screen.y0;
            gdi_rect.br.x = control_screen.x1;
            gdi_rect.tl.y = control_screen.y1;
            pixit_rect_from_screen_rect_and_context(&dialog_msg_ctl_user_redraw.control_inner_box, &gdi_rect, &dialog_msg_ctl_user_redraw.redraw_context);
            } /*block*/

            dialog_msg_ctl_user_redraw.enabled = !disabled;

            status_assert(dialog_call_client(p_dialog, DIALOG_MSG_CODE_CTL_USER_REDRAW, &dialog_msg_ctl_user_redraw, p_proc_client));

            /*host_paint_end(p_redraw_context);*/
        }
    }

    dialog_riscos_redraw_gwindow_restore(&p_dialog_control_redraw->dialog_riscos_redraw_window);

    return(STATUS_OK);
}

_Check_return_
extern PIXIT
ui_width_from_tstr_host(
    _In_z_      PCTSTR tstr)
{
    static int use_wimp_text_op = 1; /* SKS 15apr95 attempts to make it work on !NewDesk machines too */

    const U32 tchars_n = tstrlen32(tstr);
    PIXIT width;

    if((0 != tchars_n) && use_wimp_text_op)
    {
        _kernel_oserror * e;
        _kernel_swi_regs rs;
        rs.r[0] = 1;
        rs.r[1] = (int) tstr;
        rs.r[2] = (int) tchars_n;

        if(use_wimp_text_op < 2)
            consume(_kernel_oserror *, e = wimp_text_op(&rs)); /* allow silent failure first time */
        else
            void_WrapOsErrorChecking(e = wimp_text_op(&rs)); /* but report any subsequent failure */

        if(NULL == e)
        {
            if(use_wimp_text_op == 1) use_wimp_text_op = 2;
            width = rs.r[0]; /* OS units */
            width += 4; /* just a little bodge */
            width *= PIXITS_PER_RISCOS;
            return(width);
        }

        use_wimp_text_op = 0;
    }

    width = DIALOG_SYSCHARSL_H(tchars_n);
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

    /* always dynamically limit to a portion of the host's work area */
#define UI_LIST_LIMIT_NUMERATOR     3
#define UI_LIST_LIMIT_DENOMINATOR   5

    host_work_area_gdi_size_query(&screen_gdi_size); /* NB pixels */

    max_elements = muldiv64(screen_gdi_size.cy, UI_LIST_LIMIT_NUMERATOR, UI_LIST_LIMIT_DENOMINATOR) / (DIALOG_STDLISTITEM_V / PIXITS_PER_RISCOS);

    p_pixit_size->cx = 0;

    if(show_elements > max_elements)
    {
        show_elements = max_elements;
        p_pixit_size->cx += DIALOG_STDLISTEXTRA_H;
    }

    p_pixit_size->cy = show_elements * DIALOG_STDLISTITEM_V;
}

/* end of ri_dlg.c */
