/* ri_lbox.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* List box for RISC OS */

/* SKS March 1992 */

#include "common/gflags.h"

#include "ob_dlg/ui_dlgin.h"

#if RISCOS

/*
list box control
*/

typedef struct RI_LBOX
{
    /* private to ri_lbox.c */
    RI_LBOX_HANDLE ri_lbox_handle;

    P_U8 caption;

    S32 selected_item;

    UI_DATA_TYPE ui_data_type;
    const void /*UI_CONTROL*/ * p_ui_control;
    PC_UI_SOURCE p_ui_source;

    S32 n_items;
    U32 max_item_len;

    wimp_w window_handle;

    wimp_w parent_window_handle;
    wimp_i parent_icon_handle;

    struct RI_LBOX_BITS
    {
        UBF disabled                : 1;
        UBF always_show_selection   : 1;
        UBF deferred_ensure_visible : 1;
        UBF disable_double          : 1;
    } bits;

    DOCNO docno;

    P_PROC_DIALOG_EVENT p_proc_dialog_event;
    CLIENT_HANDLE       client_handle;

    WimpCaret stolen_focus_caretstr;

    BBox prev_extent;
}
RI_LBOX, * P_RI_LBOX, ** P_P_RI_LBOX;

/*
internal routines
*/

static void
ri_lbox_extent_set(
    P_RI_LBOX p_lbox,
    _In_        const WimpOpenWindowBlock * const p_open_window_block);

static void
ri_lbox_focus_lose(
    P_RI_LBOX p_lbox);

_Check_return_
static STATUS
ri_lbox_focus_notify(
    RI_LBOX_HANDLE handle);

static void
ri_lbox_item_update(
    P_RI_LBOX p_lbox,
    _InVal_     S32 stt_itemno,
    _InVal_     S32 end_itemno,
    _InVal_     S32 later);

_Check_return_
static STATUS
ri_lbox_redraw_core(
    P_RI_LBOX p_lbox,
    _In_        const WimpRedrawWindowBlock * const p_redraw_window_block);

static void
ri_lbox_selection_send_to_client(
    RI_LBOX_HANDLE handle,
    _InVal_     DIALOG_MESSAGE click_type);

#define RI_LBOX_UPDATE_NOW   0
#define RI_LBOX_UPDATE_LATER 1

static ARRAY_HANDLE ri_lboxes;

static U32 ri_lboxes_handle_gen = 0x28000000;

_Check_return_
_Ret_maybenull_
static P_RI_LBOX
ri_lbox_p_from_h(
    RI_LBOX_HANDLE handle)
{
    ARRAY_INDEX i = array_elements(&ri_lboxes);

    while(--i >= 0)
    {
        P_RI_LBOX p_ri_lbox = array_ptr(&ri_lboxes, RI_LBOX, i);

        if(handle == p_ri_lbox->ri_lbox_handle)
            return(p_ri_lbox);
    }

    return(NULL);
}

/******************************************************************************
*
* ensure that the current item is fully visible,
* scrolling the list box to make it so if not already
*
******************************************************************************/

static const GDI_SIZE lbox_mesh_size = { 1 /* uninteresting */, RI_LBOX_ITEM_HEIGHT };

static void
ri_lbox_current_ensure_visible(
    P_RI_LBOX p_lbox,
    _InVal_     STATUS page_up_down)
{
    union wimp_window_state_open_window_block_u window_u;
    GDI_COORD window_height;
    GDI_RECT items;

    if(p_lbox->selected_item == RI_LBOX_SELECTION_NONE)
        return;

    if(!p_lbox->window_handle)
        return;

    {
    GDI_POINT gdi_org;
    GDI_POINT lbox_mesh_origin;
    GDI_RECT  window_rect;

    /* compute the position in absolute screen coordinates of the work area origin (as per host_gdi_org_from_screen()) */
    window_u.window_state_block.window_handle = p_lbox->window_handle;
    void_WrapOsErrorReporting(wimp_get_window_state(&window_u.window_state_block));
    gdi_org.x = work_area_origin_x_from_visible_area_and_scroll(&window_u.window_state_block); /* window w.a. ABS origin */
    gdi_org.y = work_area_origin_y_from_visible_area_and_scroll(&window_u.window_state_block);

    window_height = BBox_height(&window_u.window_state_block.visible_area);

    /* currently origins same */
    lbox_mesh_origin = gdi_org;

    window_rect.tl.x = +(window_u.window_state_block.visible_area.xmin - lbox_mesh_origin.x);
    window_rect.tl.y = -(window_u.window_state_block.visible_area.ymax - lbox_mesh_origin.y);
    window_rect.br.x = +(window_u.window_state_block.visible_area.xmax - lbox_mesh_origin.x);
    window_rect.br.y = -(window_u.window_state_block.visible_area.ymin - lbox_mesh_origin.y);

    /* compute the indices (tl incl,br excl) of items the whole window intersects */
    gdi_rect_clip_mesh(&items, &window_rect, &lbox_mesh_size);
    } /*block*/

    if((p_lbox->selected_item == (items.tl.y - 1)) && !page_up_down)
    {
        /* ensure currently totally invisible off top item aligned at top by adjusting scroll offset of window to top of item */

        if( window_u.window_state_block.yscroll == (-(items.tl.y    ) * RI_LBOX_ITEM_HEIGHT))
        {
            window_u.window_state_block.yscroll  = (-(items.tl.y - 1) * RI_LBOX_ITEM_HEIGHT);

            /* NB. not moving position of window or place in window stack so just wimp_ not win_ */
            void_WrapOsErrorReporting(wimp_open_window(&window_u.open_window_block));
        }
    }

    else if(p_lbox->selected_item == items.tl.y)
    {
        /* ensure possibly partially visible top item aligned at top by adjusting scroll offset of window to top of item */

        if( window_u.window_state_block.yscroll != (-(items.tl.y) * RI_LBOX_ITEM_HEIGHT))
        {
            window_u.window_state_block.yscroll  = (-(items.tl.y) * RI_LBOX_ITEM_HEIGHT);

            /* NB. not moving position of window or place in window stack so just wimp_ not win_ */
            void_WrapOsErrorReporting(wimp_open_window(&window_u.open_window_block));
        }
    }

    else if(p_lbox->selected_item == (items.br.y - 1))
    {
        /* ensure possibly partially visible bottom item aligned at bottom by adjusting scroll offset of window to bottom of item */

        if( window_u.window_state_block.yscroll != (+window_height + (-(items.br.y - 1 + 1) * RI_LBOX_ITEM_HEIGHT)))
        {
            window_u.window_state_block.yscroll  = (+window_height + (-(items.br.y - 1 + 1) * RI_LBOX_ITEM_HEIGHT));

            /* NB. not moving position of window or place in window stack so just wimp_ not win_ */
            void_WrapOsErrorReporting(wimp_open_window(&window_u.open_window_block));
        }
    }

    else if((p_lbox->selected_item == items.br.y) && !page_up_down)
    {
        /* ensure currently totally invisible off bottom item aligned at bottom by adjusting scroll offset of window to bottom of item */

        if( window_u.window_state_block.yscroll == (+window_height + (-(items.br.y - 1 + 1) * RI_LBOX_ITEM_HEIGHT)))
        {
            window_u.window_state_block.yscroll  = (+window_height + (-(items.br.y     + 1) * RI_LBOX_ITEM_HEIGHT));

            /* NB. not moving position of window or place in window stack so just wimp_ not win_ */
            void_WrapOsErrorReporting(wimp_open_window(&window_u.open_window_block));
        }
    }

    else if((p_lbox->selected_item < items.tl.y) || (p_lbox->selected_item >= items.br.y))
    {
        /* ensure currently invisible item centred in window by adjusting scroll offset of window */

        window_u.window_state_block.yscroll = ((-(p_lbox->selected_item) * RI_LBOX_ITEM_HEIGHT) - (RI_LBOX_ITEM_HEIGHT / 2) + (window_height / 2));

        /* NB. not moving position of window or place in window stack so just wimp_ not win_ */
        void_WrapOsErrorReporting(wimp_open_window(&window_u.open_window_block));
    }
}

/******************************************************************************
*
* dispose of a list box
*
******************************************************************************/

_Check_return_
extern STATUS
ri_lbox_dispose(
    _InoutRef_  P_RI_LBOX_HANDLE p_handle)
{
    RI_LBOX_HANDLE handle = *p_handle;

    if(handle)
    {
        P_RI_LBOX p_lbox = ri_lbox_p_from_h(handle);

        if(NULL != p_lbox)
        {
            ri_lbox_focus_lose(p_lbox);

            /* free resources owned by this list box */
            void_WrapOsErrorReporting(winx_dispose_window(&p_lbox->window_handle));

            tstr_clr(&p_lbox->caption);

            /* free this descriptor */
            al_array_delete_at(&ri_lboxes, -1, array_indexof_element(&ri_lboxes, RI_LBOX, p_lbox));

            if(!array_elements(&ri_lboxes))
                al_array_dispose(&ri_lboxes);

            *p_handle = 0;
        }
    }

    return(STATUS_OK);
}

extern void
ri_lbox_enable(
    _InVal_     RI_LBOX_HANDLE handle,
    _InVal_     BOOL enable)
{
    P_RI_LBOX p_lbox = ri_lbox_p_from_h(handle);

    if(p_lbox->bits.disabled != UBF_PACK(!enable))
    {
        p_lbox->bits.disabled = UBF_PACK(!enable);

        /* queue a redraw for the entire window */
        ri_lbox_item_update(p_lbox, 0, p_lbox->n_items, RI_LBOX_UPDATE_LATER);
    }
}

/******************************************************************************
*
* RISC OS list box event handler
*
******************************************************************************/

_Check_return_
static int
ri_lbox_event_Close_Window_Request(
    RI_LBOX_HANDLE handle,
    _In_        const WimpCloseWindowRequestEvent * const p_close_window_request);

_Check_return_
static int
ri_lbox_event_Open_Window_Request(
    RI_LBOX_HANDLE handle,
    _In_        const WimpOpenWindowRequestEvent * const p_open_window_request);

_Check_return_
static int
ri_lbox_event_Redraw_Window_Request(
    RI_LBOX_HANDLE handle,
    _In_        const WimpRedrawWindowRequestEvent * const p_redraw_window_request);

_Check_return_
static int
ri_lbox_event_Mouse_Click(
    RI_LBOX_HANDLE handle,
    _In_        const WimpMouseClickEvent * const p_mouse_click);

_Check_return_
static int
ri_lbox_event_Key_Pressed(
    RI_LBOX_HANDLE handle,
    _In_        const WimpKeyPressedEvent * const p_key_pressed);

_Check_return_
static int
ri_lbox_event_Lose_Caret(
    RI_LBOX_HANDLE handle);

_Check_return_
static int
ri_lbox_event_handler(
    _InVal_     int event_code,
    _In_        const WimpPollBlock * const p_event_data,
    P_ANY handle)
{
    trace_3(TRACE_RISCOS_HOST, TEXT("%s: %s handle=") PTR_XTFMT, __Tfunc__, report_wimp_event(event_code, p_event_data), handle);

    switch(event_code)
    {
    case Wimp_ERedrawWindow:
        return(ri_lbox_event_Redraw_Window_Request((RI_LBOX_HANDLE) handle, &p_event_data->redraw_window_request));

    case Wimp_EOpenWindow:
        return(ri_lbox_event_Open_Window_Request((RI_LBOX_HANDLE) handle, &p_event_data->open_window_request));

    case Wimp_ECloseWindow:
        return(ri_lbox_event_Close_Window_Request((RI_LBOX_HANDLE) handle, &p_event_data->close_window_request));

    case Wimp_EMouseClick:
        return(ri_lbox_event_Mouse_Click((RI_LBOX_HANDLE) handle, &p_event_data->mouse_click));

    case Wimp_EKeyPressed:
        return(ri_lbox_event_Key_Pressed((RI_LBOX_HANDLE) handle, &p_event_data->key_pressed));

    case Wimp_ELoseCaret:
    case Wimp_EGainCaret:
        return(ri_lbox_event_Lose_Caret((RI_LBOX_HANDLE) handle));

    default:
        return(FALSE /*unprocessed*/);
    }
}

/******************************************************************************
*
* handle closing our workspace window
*
******************************************************************************/

_Check_return_
static int
ri_lbox_event_Close_Window_Request(
    RI_LBOX_HANDLE handle,
    _In_        const WimpCloseWindowRequestEvent * const p_close_window_request)
{
    P_RI_LBOX p_lbox = ri_lbox_p_from_h(handle);

    if(p_lbox->p_proc_dialog_event)
    {
        /* send owner a message */
        DIALOG_MSG_LBN_DESTROY dialog_riscos_lbn_destroy;
        dialog_riscos_lbn_destroy.client_handle = p_lbox->client_handle;
        dialog_riscos_lbn_destroy.lbox_handle   = handle;
        dialog_riscos_lbn_destroy.processed = FALSE;
        (* p_lbox->p_proc_dialog_event) (p_docu_from_docno(p_lbox->docno), DIALOG_MSG_CODE_LBN_DESTROY, &dialog_riscos_lbn_destroy);
        return(dialog_riscos_lbn_destroy.processed);
    }

    UNREFERENCED_PARAMETER_CONST(p_close_window_request);

    return(FALSE /*unprocessed*/);
}

/******************************************************************************
*
* handle opening our workspace window
*
******************************************************************************/

_Check_return_
static int
ri_lbox_event_Open_Window_Request(
    RI_LBOX_HANDLE handle,
    _In_        const WimpOpenWindowRequestEvent * const p_open_window_request)
{
    P_RI_LBOX p_lbox = ri_lbox_p_from_h(handle);
    /* ensure extent of window large enough to span current list */
    WimpOpenWindowRequestEvent open_window_request = *p_open_window_request; /* open may adjust coords */

    ri_lbox_extent_set(p_lbox, &open_window_request);

    winx_open_window(&open_window_request);

    if(p_lbox->bits.deferred_ensure_visible)
    {
        p_lbox->bits.deferred_ensure_visible = 0;

        ri_lbox_current_ensure_visible(p_lbox, 1);
    }

    return(TRUE /*processed*/);
}

/******************************************************************************
*
* handle redrawing our workspace window
*
******************************************************************************/

_Check_return_
static int
ri_lbox_event_Redraw_Window_Request(
    RI_LBOX_HANDLE handle,
    _In_        const WimpRedrawWindowRequestEvent * const p_redraw_window_request)
{
    P_RI_LBOX p_lbox = ri_lbox_p_from_h(handle);
    WimpRedrawWindowBlock redraw_window_block;
    int wimp_more = 0;

    redraw_window_block.window_handle = p_redraw_window_request->window_handle;

    if(NULL != WrapOsErrorReporting(wimp_redraw_window(&redraw_window_block, &wimp_more)))
        wimp_more = 0;

    while(0 != wimp_more)
    {
        host_invalidate_cache(HIC_REDRAW_LOOP_START);

        (void) ri_lbox_redraw_core(p_lbox, &redraw_window_block);

        if(NULL != WrapOsErrorReporting(wimp_get_rectangle(&redraw_window_block, &wimp_more)))
            wimp_more = 0;
    }

    return(TRUE /*processed*/);
}

/******************************************************************************
*
* handle mouse clicks in our workspace
*
******************************************************************************/

_Check_return_
static int
ri_lbox_event_Mouse_Click(
    RI_LBOX_HANDLE handle,
    _In_        const WimpMouseClickEvent * const p_mouse_click)
{
    P_RI_LBOX p_lbox = ri_lbox_p_from_h(handle);
    GDI_POINT item;
    DIALOG_MESSAGE click_type;

    if(p_lbox->bits.disabled)
        return(TRUE /*processed*/);

    switch(p_mouse_click->buttons)
    {
    default:
        return(TRUE /*processed*/);

    case Wimp_MouseButtonSelect:
    case Wimp_MouseButtonAdjust:
        if(!p_lbox->bits.disable_double)
        {
            click_type = DIALOG_MSG_CODE_LBN_DBLCLK;
            break;
        }

        /*FALLTHRU*/

    case Wimp_MouseButtonSingleSelect:
    case Wimp_MouseButtonSingleAdjust:
        click_type = DIALOG_MSG_CODE_LBN_SGLCLK;
        break;
    }

    {
    GDI_POINT gdi_org;
    GDI_POINT lbox_mesh_origin;
    GDI_POINT test_point;

    /* compute the position in absolute screen coordinates of the work area origin */
    host_gdi_org_from_screen(&gdi_org, p_mouse_click->window_handle); /* window w.a. ABS origin */

    /* currently origins same */
    lbox_mesh_origin = gdi_org;

    test_point.x = +(p_mouse_click->mouse_x - lbox_mesh_origin.x);
    test_point.y = -(p_mouse_click->mouse_y - lbox_mesh_origin.y + host_modevar_cache_current.dy); /* RISC OS tells us bl of pixel; make tl */

    /* compute the indices of the item the button went down in */
    gdi_point_mesh_hit(&item, &test_point, &lbox_mesh_size);
    } /*block*/

    /* ignore clicks off either end (NB. don't set back to RI_LBOX_SELECTION_NONE) */
    if((item.y >= 0) && (item.y < p_lbox->n_items))
    {
        /* SKS 02feb93 */
        if(click_type == DIALOG_MSG_CODE_LBN_DBLCLK)
        {
            /* try to counter any selection movement funnies due to either source changing or client having mucked about with sel'n */
            if(item.y != p_lbox->selected_item)
                click_type = DIALOG_MSG_CODE_LBN_SGLCLK;
        }

        ri_lbox_focus_set(handle);
        ri_lbox_focus_notify(handle);
        (void) ri_lbox_selection_set_from_itemno(handle, item.y);
        ri_lbox_current_ensure_visible(p_lbox, 0);
        ri_lbox_selection_send_to_client(handle, click_type);
    }

    return(TRUE /*processed*/);
}

/******************************************************************************
*
* handle key presses in our workspace
*
******************************************************************************/

_Check_return_
static int
ri_lbox_event_Key_Pressed(
    RI_LBOX_HANDLE handle,
    _In_        const WimpKeyPressedEvent * const p_key_pressed)
{
    P_RI_LBOX p_lbox = ri_lbox_p_from_h(handle);
    WimpGetWindowStateBlock window_state;
    S32 whole_items_in_window;
    KMAP_CODE kmap_code;

    if(p_lbox->bits.disabled)
        return(FALSE /*unprocessed*/);

    window_state.window_handle = p_key_pressed->caret.window_handle;
    void_WrapOsErrorReporting(wimp_get_window_state(&window_state));

    whole_items_in_window = BBox_height(&window_state.visible_area) / RI_LBOX_ITEM_HEIGHT;
    if(!whole_items_in_window)
        whole_items_in_window = 1;

    kmap_code = kmap_convert(p_key_pressed->key_code);

    switch(kmap_code)
    {
    case KMAP_FUNC_HOME:
    case KMAP_FUNC_END:
    case KMAP_FUNC_PAGE_DOWN:
    case KMAP_FUNC_PAGE_UP:
    case KMAP_FUNC_ARROW_LEFT:
    case KMAP_FUNC_ARROW_RIGHT:
    case KMAP_FUNC_ARROW_DOWN:
    case KMAP_FUNC_ARROW_UP:
        {
        S32 itemno = RI_LBOX_SELECTION_NONE;
        STATUS page_up_down = 0;

        switch(kmap_code)
        {
        /* may want sometime to emulate Windows-like ctrl-func to more current pos without altering selection */

        case KMAP_FUNC_HOME:
            /* go to the top of the list box (selecting the item) */
            itemno = 0;
            break;

        case KMAP_FUNC_END:
            /* go to the end of the list box (selecting the item) */
            itemno = p_lbox->n_items;
            break;

        case KMAP_FUNC_PAGE_DOWN:
            /* go down a whole windowful (selecting a new item) if possible */
            if(p_lbox->selected_item == RI_LBOX_SELECTION_NONE)
                itemno = p_lbox->n_items;
            else if(p_lbox->selected_item != p_lbox->n_items - 1)
                itemno = MIN(p_lbox->selected_item + whole_items_in_window, p_lbox->n_items - 1);
            page_up_down = 1;
            break;

        case KMAP_FUNC_PAGE_UP:
            /* go up a whole windowful (selecting a new item) if possible */
            if(p_lbox->selected_item == RI_LBOX_SELECTION_NONE)
                itemno = 0;
            else if(p_lbox->selected_item != 0)
                itemno = MAX(p_lbox->selected_item - whole_items_in_window, 0);
            page_up_down = 1;
            break;

        case KMAP_FUNC_ARROW_RIGHT:
        case KMAP_FUNC_ARROW_DOWN:
            /* go down an item (selecting it too) if possible */
            if(p_lbox->selected_item == RI_LBOX_SELECTION_NONE)
                itemno = p_lbox->n_items;
            else if(p_lbox->selected_item != p_lbox->n_items - 1)
                itemno = p_lbox->selected_item + 1;
            break;

        default: default_unhandled();
#if CHECKING
        case KMAP_FUNC_ARROW_LEFT:
        case KMAP_FUNC_ARROW_UP:
#endif
            /* go up an item (selecting it too) if possible */
            if(p_lbox->selected_item == RI_LBOX_SELECTION_NONE)
                itemno = 0;
            else if(p_lbox->selected_item != 0)
                itemno = p_lbox->selected_item - 1;
            break;
        }

        if((itemno != RI_LBOX_SELECTION_NONE) && p_lbox->n_items)
        {
            (void) ri_lbox_selection_set_from_itemno(handle, itemno);
            ri_lbox_current_ensure_visible(p_lbox, page_up_down);
            ri_lbox_selection_send_to_client(handle, DIALOG_MSG_CODE_LBN_SELCHANGE);
        }

        return(TRUE /*processed*/);
        }

    default:
        {
        /* if a letter, try to select an appropriate entry */
        if((kmap_code < 0x100) && (sbchar_isalnum(kmap_code) || (CH_UNDERSCORE == kmap_code)))
        {
            TCHARZ s[2];
            s[0] = (TCHAR) kmap_code;
            s[1] = CH_NULL;
            (void) ri_lbox_selection_set_from_tstr(handle, s);
            ri_lbox_current_ensure_visible(p_lbox, 1);
            ri_lbox_selection_send_to_client(handle, DIALOG_MSG_CODE_LBN_SELCHANGE);
            return(TRUE /*processed*/);
        }

        /* if attached as a control to another window, send unprocessed, unconverted keys to parent window NOW */
        if(p_lbox->p_proc_dialog_event)
        {
            DIALOG_MSG_LBN_KEY dialog_msg_lbn_key;
            dialog_msg_lbn_key.client_handle = p_lbox->client_handle;
            dialog_msg_lbn_key.lbox_handle   = handle;
            dialog_msg_lbn_key.selected_item = p_lbox->selected_item;
            dialog_msg_lbn_key.kmap_code     = kmap_code;
            (* p_lbox->p_proc_dialog_event) (p_docu_from_docno(p_lbox->docno), DIALOG_MSG_CODE_LBN_KEY, &dialog_msg_lbn_key);
            return((BOOL) dialog_msg_lbn_key.processed);
        }

        return(FALSE /*unprocessed*/);
        }
    }
}

/******************************************************************************
*
* handle caret gain/lose in our workspace
*
******************************************************************************/

_Check_return_
static int
ri_lbox_event_Lose_Caret(
    RI_LBOX_HANDLE handle)
{
    /* ensure window redrawn if needed */
    P_RI_LBOX p_lbox = ri_lbox_p_from_h(handle);

    if(!p_lbox->bits.always_show_selection && (p_lbox->selected_item >= 0))
        ri_lbox_item_update(p_lbox, p_lbox->selected_item, p_lbox->selected_item+1, RI_LBOX_UPDATE_NOW);

    return(FALSE /*unprocessed*/);
}

static void
ri_lbox_build_caretstr(
    P_RI_LBOX p_lbox,
    WimpCaret * p_caret)
{
    p_caret->window_handle = p_lbox->window_handle;
    p_caret->icon_handle = BAD_WIMP_I;
    p_caret->xoffset = 0;
    p_caret->yoffset = 0;
    p_caret->height = 0         /* caret height */
#if 0
                    | (1 << 24) /* VDU-5 type caret */
#endif
                    | (1 << 25) /* caret invisible */;
    p_caret->index  = 0;
}

static inline int
caret_memcmp(
    _In_reads_bytes_(n_bytes) PC_ANY src_any_1,
    _In_reads_bytes_(n_bytes) PC_ANY src_any_2,
    _InVal_     U32 n_bytes)
{
    PC_S32 src_1 = (PC_S32) src_any_1;
    PC_S32 src_2 = (PC_S32) src_any_2;
    const PC_S32 end_src_2 = PtrAddBytes(PC_S32, src_2, n_bytes);

    while(src_2 < end_src_2)
    {
        S32 a = *src_1++;
        S32 b = *src_2++;
        if(a != b)
            return(a - b);
    }

    return(0);
}

_Check_return_
extern STATUS
ri_lbox_focus_query(
    _InVal_     RI_LBOX_HANDLE handle)
{
    P_RI_LBOX p_lbox = ri_lbox_p_from_h(handle);
    WimpCaret cur_caret, des_caret;

    if(0 != p_lbox->window_handle)
    {
        ri_lbox_build_caretstr(p_lbox, &des_caret);

        void_WrapOsErrorReporting(winx_get_caret_position(&cur_caret));

        if(0 == caret_memcmp(&cur_caret, &des_caret, sizeof32(cur_caret)))
            return(STATUS_OK);
    }

    return(STATUS_FAIL);
}

/******************************************************************************
*
* put the input focus in the list box, invisible
*
******************************************************************************/

static void
ri_lbox_focus_lose(
    P_RI_LBOX p_lbox)
{
    if(0 != p_lbox->stolen_focus_caretstr.window_handle)
    {
        /* attempt to restore caret a la Window Manager */
        WimpGetWindowStateBlock window_state;

        window_state.window_handle = p_lbox->stolen_focus_caretstr.window_handle;
        if(NULL == wimp_get_window_state(&window_state))
        {
            if(0 != (WimpWindow_Open & window_state.flags))
                void_WrapOsErrorReporting(winx_set_caret_position(&p_lbox->stolen_focus_caretstr));
        }

        p_lbox->stolen_focus_caretstr.window_handle = 0;
    }
}

_Check_return_
static STATUS
ri_lbox_focus_notify(
    RI_LBOX_HANDLE handle)
{
    P_RI_LBOX p_lbox = ri_lbox_p_from_h(handle);

    if(p_lbox->p_proc_dialog_event)
    {
        /* send owner a message */
        DIALOG_MSG_LBN_FOCUS dialog_riscos_lbn_focus;
        dialog_riscos_lbn_focus.client_handle = p_lbox->client_handle;
        dialog_riscos_lbn_focus.lbox_handle = handle;
        (* p_lbox->p_proc_dialog_event) (p_docu_from_docno(p_lbox->docno), DIALOG_MSG_CODE_LBN_FOCUS, &dialog_riscos_lbn_focus);
    }

    return(STATUS_OK);
}

_Check_return_
extern STATUS
ri_lbox_focus_set(
    _InVal_     RI_LBOX_HANDLE handle)
{
    P_RI_LBOX p_lbox = ri_lbox_p_from_h(handle);
    WimpCaret cur_caret, des_caret;

    if(p_lbox->window_handle)
    {
        ri_lbox_build_caretstr(p_lbox, &des_caret);

        void_WrapOsErrorReporting(winx_get_caret_position(&cur_caret));

        if(0 == caret_memcmp(&cur_caret, &des_caret, sizeof32(cur_caret)))
            return(STATUS_OK);

        if(!p_lbox->stolen_focus_caretstr.window_handle)
            memcpy32(&p_lbox->stolen_focus_caretstr, &cur_caret, sizeof32(cur_caret));

        riscos_claim_caret();

        if(WrapOsErrorReporting(winx_set_caret_position(&des_caret)))
            return(STATUS_FAIL);
    }

    return(STATUS_OK);
}

/******************************************************************************
*
* obtain the host Window Manager's handle for a list box
*
******************************************************************************/

extern HOST_WND
ri_lbox_get_host_handle(
    _InVal_     RI_LBOX_HANDLE handle)
{
    P_RI_LBOX p_lbox = ri_lbox_p_from_h(handle);

    return(p_lbox->window_handle);
}

/******************************************************************************
*
* ensure the list box is not displayed
*
******************************************************************************/

_Check_return_
extern STATUS
ri_lbox_hide(
    _InVal_     RI_LBOX_HANDLE handle)
{
    P_RI_LBOX p_lbox = ri_lbox_p_from_h(handle);

    if(0 != p_lbox->window_handle)
    {
        WimpGetWindowStateBlock window_state;

        window_state.window_handle = p_lbox->window_handle;
        if(NULL == WrapOsErrorReporting(wimp_get_window_state(&window_state)))
        {
            if(0 != (WimpWindow_Open & window_state.flags))
            {
                ri_lbox_focus_lose(p_lbox);

                winx_send_close_window_request(p_lbox->window_handle, FALSE);
            }
        }
    }

    return(STATUS_OK);
}

/******************************************************************************
*
* update for list box item
*
******************************************************************************/

static void
ri_lbox_item_update(
    P_RI_LBOX p_lbox,
    _InVal_     S32 stt_itemno,
    _InVal_     S32 end_itemno,
    _InVal_     S32 later)
{
    WimpRedrawWindowBlock redraw_window_block;

    /* this window, so all coords are work area relative */
    if(!p_lbox->window_handle)
        return;

    /* tl */
    redraw_window_block.visible_area.xmin =       -0x00800000; /* large -ve */
    redraw_window_block.visible_area.ymax = (int) -(stt_itemno * RI_LBOX_ITEM_HEIGHT);

    /* br */
    redraw_window_block.visible_area.xmax =       +0x00800000; /* large +ve */
    redraw_window_block.visible_area.ymin = (int) -(end_itemno * RI_LBOX_ITEM_HEIGHT);

    if(later)
    {
        void_WrapOsErrorReporting(wimp_force_redraw_BBox(p_lbox->window_handle, &redraw_window_block.visible_area));
    }
    else
    {
        int wimp_more = 0;

        redraw_window_block.window_handle = p_lbox->window_handle;

        if(NULL != WrapOsErrorReporting(wimp_update_window(&redraw_window_block, &wimp_more)))
            wimp_more = 0;

        while(0 != wimp_more)
        {
            (void) ri_lbox_redraw_core(p_lbox, &redraw_window_block);

            if(NULL != WrapOsErrorReporting(wimp_get_rectangle(&redraw_window_block, &wimp_more)))
                wimp_more = 0;
        }
    }
}

/******************************************************************************
*
* make the list box a child control of another window
*
******************************************************************************/

_Check_return_
extern STATUS
ri_lbox_make_child(
    _InVal_     RI_LBOX_HANDLE handle,
    _InVal_     wimp_w parent_window_handle,
    _InVal_     wimp_i parent_icon_handle)
{
    P_RI_LBOX p_lbox = ri_lbox_p_from_h(handle);

    assert(p_lbox->window_handle);
    assert(parent_window_handle > 0);

    p_lbox->parent_window_handle = parent_window_handle;
    p_lbox->parent_icon_handle = parent_icon_handle;

    winx_register_child_window(p_lbox->parent_window_handle, p_lbox->window_handle, TRUE);

    return(STATUS_OK);
}

/******************************************************************************
*
* create a list box
*
******************************************************************************/

_Check_return_
extern STATUS
ri_lbox_new(
    _OutRef_    P_RI_LBOX_HANDLE p_handle,
    /*const*/ P_RI_LBOX_NEW p_ri_lbox_new)
{
    P_RI_LBOX p_lbox;
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(*p_lbox), TRUE);
    RI_LBOX_HANDLE handle;
    WimpWindowWithBitset wind_defn;
    GDI_COORD items_extent, window_size_y;
    STATUS status;

    *p_handle = 0;

    if(NULL == (p_lbox = al_array_extend_by(&ri_lboxes, RI_LBOX, 1, &array_init_block, &status)))
        return(status);

    /* generate unique, non-zero handle */
    handle = ++ri_lboxes_handle_gen;

    p_lbox->ri_lbox_handle = handle;

    p_lbox->selected_item = RI_LBOX_SELECTION_NONE;
    p_lbox->bits.always_show_selection = p_ri_lbox_new->bits.always_show_selection;
    p_lbox->bits.disable_double = p_ri_lbox_new->bits.disable_double;

    p_lbox->docno         = docno_from_p_docu(p_ri_lbox_new->p_docu);
    p_lbox->ui_data_type  = p_ri_lbox_new->ui_data_type;
    p_lbox->p_ui_control  = p_ri_lbox_new->p_ui_control;
    p_lbox->p_ui_source   = p_ri_lbox_new->p_ui_source;
    p_lbox->p_proc_dialog_event = p_ri_lbox_new->p_proc_dialog_event;
    p_lbox->client_handle = p_ri_lbox_new->client_handle;

    /* cache n_items */
    p_lbox->n_items = ui_data_n_items_query(p_lbox->ui_data_type, p_lbox->p_ui_source);

    items_extent = p_lbox->n_items * RI_LBOX_ITEM_HEIGHT;

    if(!ui_text_is_blank(p_ri_lbox_new->caption))
        if(status_fail(status = ui_text_copy_as_sbstr(&p_lbox->caption, p_ri_lbox_new->caption)))
        {
            ri_lbox_dispose(&handle);
            return(status);
        }

    zero_struct(wind_defn);

    wind_defn.behind = (wimp_w) -1; /* open at the top of the window stack */

    wind_defn.flags.bits.flags_are_new = 1;
    wind_defn.flags.bits.moveable = 1;
    /* needed for RISC OS 3's bodge window open without allowing children access to parent info */
    wind_defn.flags.bits.trespass = p_ri_lbox_new->bits.trespass;
    wind_defn.title_fg = '\x07'; /* black  */
    wind_defn.title_bg = '\x02'; /* light grey */
    wind_defn.work_fg = '\x07'; /* black */
    wind_defn.work_bg = '\xFF'; /* transparent */
    wind_defn.scroll_outer = '\x03'; /* mid grey */
    wind_defn.scroll_inner = '\x01'; /* light grey */
    wind_defn.highlight_bg = wind_defn.title_bg;

    wind_defn.work_flags.bits.button_type = p_lbox->bits.disable_double ? ButtonType_Click : ButtonType_DoubleClickDrag;
    wind_defn.sprite_area = (void *) 1; /* Window Manager sprite area (needed to satisfy RISC PC) */
    wind_defn.min_width = 1;
    wind_defn.min_height = 1;

    wind_defn.visible_area = p_ri_lbox_new->bbox;

    if(p_ri_lbox_new->bits.pane)
        wind_defn.flags.bits.pane = 1;

    if(p_ri_lbox_new->bits.disable_frame)
        wind_defn.title_fg = '\x00'; /* next best thing to frame not present */

    if(p_lbox->caption)
    {
        wind_defn.flags.bits.has_title = 1;

        wind_defn.title_flags.bits.text        = 1;
        wind_defn.title_flags.bits.horz_centre = 1;
        wind_defn.title_flags.bits.indirect    = 1;

        wind_defn.titledata.it.buffer = p_lbox->caption;
        wind_defn.titledata.it.buffer_size = strlen32p1(wind_defn.titledata.it.buffer); /*CH_NULL*/

        /* adjust given window size y for title bar */
        wind_defn.visible_area.ymax -= wimp_win_title_height(host_modevar_cache_current.dy);

        if( wind_defn.visible_area.ymin > wind_defn.visible_area.ymax)
            wind_defn.visible_area.ymin = wind_defn.visible_area.ymax;
    }

    if(p_ri_lbox_new->bits.force_h_scroll)
    {
        wind_defn.flags.bits.has_horz_scroll = 1;

        /* adjust given window size y for horizontal scroll bar */
        wind_defn.visible_area.ymin += wimp_win_hscroll_height(host_modevar_cache_current.dy);

        if( wind_defn.visible_area.ymin > wind_defn.visible_area.ymax)
            wind_defn.visible_area.ymin = wind_defn.visible_area.ymax;

        p_lbox->max_item_len = 1;
    }

    window_size_y = BBox_height(&wind_defn.visible_area);

    if(!p_ri_lbox_new->bits.allow_non_multiple)
    {
        /* round window size up or down to next multiple */
        GDI_COORD diff     = window_size_y / RI_LBOX_ITEM_HEIGHT;
        GDI_COORD downsize = diff * RI_LBOX_ITEM_HEIGHT;

        diff = window_size_y - downsize;

        window_size_y = downsize + ((diff >= RI_LBOX_ITEM_HEIGHT / 2) ? RI_LBOX_ITEM_HEIGHT : 0);
    }

    window_size_y = MIN(window_size_y, host_modevar_cache_current.gdi_size.cy);

    if(p_ri_lbox_new->bits.force_v_scroll || (items_extent > window_size_y))
    {
        /* adjust given window size x for vertical scroll bar */
        wind_defn.visible_area.xmax -= wimp_win_vscroll_width(host_modevar_cache_current.dx);

        wind_defn.flags.bits.has_vert_scroll = 1;

        /* now we have a vertical scroll bar the Window Manager forces a minimum size upon us;
         * larger on RISC OS 3 than RISC OS 2, just for compatibility ...
        */
        if(window_size_y < 116)
        {
            if(p_ri_lbox_new->bits.allow_non_multiple)
                window_size_y = 116;
            else
                window_size_y = 120;

            assert_EQ(120, 3 * RI_LBOX_ITEM_HEIGHT);
        }
    }
    else if(!p_ri_lbox_new->bits.dont_shrink_to_fit)
    {
        if( window_size_y > items_extent)
            window_size_y = items_extent;
    }

    wind_defn.visible_area.ymin = wind_defn.visible_area.ymax - (int) window_size_y;

    /* make initial scrolls and extent */
    wind_defn.xscroll = 0;
    wind_defn.yscroll = 0;

    wind_defn.extent.xmin = - wind_defn.xscroll; /* top left */
    wind_defn.extent.ymax = - wind_defn.yscroll;

    wind_defn.extent.xmax = wind_defn.extent.xmin +     BBox_width(&wind_defn.visible_area); /* bottom right */
    wind_defn.extent.ymin = wind_defn.extent.ymax - MAX(BBox_height(&wind_defn.visible_area), (int) items_extent);

    p_lbox->prev_extent = wind_defn.extent;

    if(status_fail(status = winx_create_window(&wind_defn, &p_lbox->window_handle, ri_lbox_event_handler, (P_ANY) handle)))
    {
        ri_lbox_dispose(&handle);
        return(status);
    }

    *p_handle = handle;

    return(STATUS_OK);
}

/******************************************************************************
*
* list box RISC OS redraw core
*
******************************************************************************/

_Check_return_
static STATUS
ri_lbox_redraw_core(
    P_RI_LBOX p_lbox,
    _In_        const WimpRedrawWindowBlock * const p_redraw_window_block)
{
    GDI_POINT gdi_org;
    GDI_POINT item_origin;
    GDI_POINT lbox_mesh_origin;
    GDI_RECT items;
    LIST_ITEMNO itemno;
    BOOL has_focus;

    {
    WimpCaret caret;
    void_WrapOsErrorReporting(winx_get_caret_position(&caret));
    has_focus = (caret.window_handle == p_redraw_window_block->window_handle);
    } /*block*/

    {
    GDI_RECT clip_rect;

    /* compute the position in absolute screen coordinates of the work area origin */
    gdi_org.x = work_area_origin_x_from_visible_area_and_scroll(p_redraw_window_block);
    gdi_org.y = work_area_origin_y_from_visible_area_and_scroll(p_redraw_window_block);

    /* currently origins same */
    lbox_mesh_origin = gdi_org;

    clip_rect.tl.x = +(p_redraw_window_block->redraw_area.xmin - lbox_mesh_origin.x);
    clip_rect.tl.y = -(p_redraw_window_block->redraw_area.ymax - lbox_mesh_origin.y);
    clip_rect.br.x = +(p_redraw_window_block->redraw_area.xmax - lbox_mesh_origin.x);
    clip_rect.br.y = -(p_redraw_window_block->redraw_area.ymin - lbox_mesh_origin.y);

    /* compute the indices (tl incl, br excl) of items the clip rectangle intersects */
    gdi_rect_clip_mesh(&items, &clip_rect, &lbox_mesh_size);
    } /*block*/

    itemno = items.tl.y; /* topmost visible item */

    item_origin.x = lbox_mesh_origin.x;
    item_origin.y = lbox_mesh_origin.y - items.tl.y * lbox_mesh_size.cy; /* tl of topmost visible item */

    do  {
        int item_fg, item_bg;
        UI_TEXT ui_text;
        STATUS is_selected_item = (itemno >= 0) && (itemno < p_lbox->n_items) && (itemno == p_lbox->selected_item);

        item_fg = p_lbox->bits.disabled ? 0x01 /*light grey*/ : 0x07 /*black*/;
        item_bg = 0x00 /*white*/;

        if(is_selected_item && (p_lbox->bits.always_show_selection || has_focus))
        {
            /* swap colours around */
            item_fg ^= item_bg;
            item_bg ^= item_fg;
            item_fg ^= item_bg;
        }

        /* clear item background to item bg colour, print item text in item fg colour */

        if(host_setbgcolour(&rgb_stash[item_bg]))
        {
            void_WrapOsErrorChecking(
                bbc_move((int) item_origin.x, (int) item_origin.y - 1));

            void_WrapOsErrorChecking(
                os_plot(0x60 /*bbc_RectangleFill*/ + 3 /*bbc_DrawRelBack*/,
                        (int) +(BBox_width(&p_redraw_window_block->visible_area) - 1),
                        (int) -(lbox_mesh_size.cy - 1)));
        }

        /* may get stray pixels up top, and also at bottom, but more importantly, empty space when small n_items in larger list box */
        if((itemno >= 0) && (itemno < p_lbox->n_items))
        {
            status_return(ui_data_query_as_text(p_lbox->ui_data_type, p_lbox->p_ui_source, itemno, p_lbox->p_ui_control, &ui_text));

            if(!ui_text_is_blank(&ui_text))
            {
                P_SBSTR sbstr = de_const_cast(P_SBSTR, _sbstr_from_tstr(ui_text_tstr(&ui_text)));
                U32 len = strlen32(sbstr);

                {
                P_SBSTR p_tab = sbstr;

                while(NULL != (p_tab = strchr(p_tab, '\t')))
                    *p_tab++ = CH_SPACE;
                } /*block*/
#if 0
                if(host_setfgcolour(&rgb_stash[item_fg]))
                {
                    /* it's that wretched VDU 5 again! */
                    bbc_move(item_origin.x
                             + RI_LBOX_ITEM_LM   /* left margin of text within item in list box */,

                             item_origin.y
                             - RI_LBOX_ITEM_TM   /* top border of text within item */
                             - 4                 /* VDU 5 offset down from cell top */);

                    bbc_write0(p_u8);
                }
#endif

#if 1
                {
                WimpIconBlockWithBitset icon;

                icon.flags.u32 = 0;

                icon.flags.bits.text = 1;
                icon.flags.bits.indirect = 1;
                icon.flags.bits.fg_colour = item_fg;
                icon.flags.bits.bg_colour = item_bg; /* keep proportional font happy */
                icon.flags.bits.filled = 1; /* ditto? */

                icon.bbox.xmin = (item_origin.x + RI_LBOX_ITEM_LM /* left margin of text within item in list box */ - gdi_org.x - 6 /*bodge for PDM*/);
                icon.bbox.ymax = (item_origin.y - RI_LBOX_ITEM_TM /* top  border of text within item in list box */ - gdi_org.y + 4 /*bodge for PDM*/);
                icon.bbox.xmax = icon.bbox.xmin + 0x3FFF;
                icon.bbox.ymin = icon.bbox.ymax - lbox_mesh_size.cy;

                icon.data.it.buffer = sbstr;
                icon.data.it.validation = NULL;
                icon.data.it.buffer_size = len;

                host_ploticon(&icon);
                } /*block*/
#endif

                if(p_lbox->max_item_len)
                    p_lbox->max_item_len = MAX(p_lbox->max_item_len, len);
            }

            ui_text_dispose(&ui_text);
        }

        item_origin.y -= lbox_mesh_size.cy;
    }
    while(++itemno < items.br.y);

    return(STATUS_OK);
}

/******************************************************************************
*
* obtain the current selection in the list box
*
******************************************************************************/

_Check_return_
extern S32
ri_lbox_selection_get(
    _InVal_     RI_LBOX_HANDLE handle)
{
    P_RI_LBOX p_lbox = ri_lbox_p_from_h(handle);

    return(p_lbox->selected_item);
}

static void
ri_lbox_selection_send_to_client(
    RI_LBOX_HANDLE handle,
    _InVal_     DIALOG_MESSAGE click_type)
{
    P_RI_LBOX p_lbox = ri_lbox_p_from_h(handle);

    /* notify owner, who may close us down */
    if(p_lbox->p_proc_dialog_event)
    {
        DIALOG_MSG_LBN_CLK dialog_msg_lbn_clk;
        dialog_msg_lbn_clk.client_handle = p_lbox->client_handle;
        dialog_msg_lbn_clk.lbox_handle   = handle;
        dialog_msg_lbn_clk.selected_item = p_lbox->selected_item;
        (* p_lbox->p_proc_dialog_event) (p_docu_from_docno(p_lbox->docno), click_type, &dialog_msg_lbn_clk);
    }
}

/******************************************************************************
*
* attempt to set an item as current selection in list box
*
******************************************************************************/

/*ncr*/
extern S32
ri_lbox_selection_set_from_itemno(
    _InVal_     RI_LBOX_HANDLE handle,
    _In_        S32 selected_item)
{
    P_RI_LBOX p_lbox = ri_lbox_p_from_h(handle);
    S32 old_selected_item = p_lbox->selected_item;

    /* limit selections to ends of list box
     * NB. take care with zero-element list boxes
    */
    if(selected_item < 0)
        selected_item = RI_LBOX_SELECTION_NONE;

    if(selected_item >= p_lbox->n_items)
    {
        if(p_lbox->n_items == 0)
            selected_item = RI_LBOX_SELECTION_NONE;
        else
            selected_item = p_lbox->n_items - 1;
    }

    p_lbox->selected_item = selected_item;

    if(RI_LBOX_SELECTION_NONE != old_selected_item)
        ri_lbox_item_update(p_lbox, old_selected_item, old_selected_item + 1, RI_LBOX_UPDATE_LATER);

    if(RI_LBOX_SELECTION_NONE != selected_item)
        ri_lbox_item_update(p_lbox, selected_item, selected_item + 1, RI_LBOX_UPDATE_NOW);

    return(selected_item);
}

/******************************************************************************
*
* attempt to set the selection in a list box from the given string prefix
* searches from current point onwards, wrapping round
*
******************************************************************************/

/*ncr*/
extern S32
ri_lbox_selection_set_from_tstr(
    _InVal_     RI_LBOX_HANDLE handle,
    _In_z_      PCTSTR tstr)
{
    P_RI_LBOX p_lbox = ri_lbox_p_from_h(handle);
    S32 itemno, stt_itemno, end_itemno;
    S32 pass;

    if(p_lbox->n_items == 0)
        return(ri_lbox_selection_set_from_itemno(handle, RI_LBOX_SELECTION_NONE));

    /* search from current point onwards */
    stt_itemno = p_lbox->selected_item;
    end_itemno = p_lbox->n_items;

    if(++stt_itemno <= 0)
    {
        stt_itemno = 0;
        pass = 2;
    }
    else
        pass = 1;

    do  {
        for(itemno = stt_itemno; itemno < end_itemno; ++itemno)
        {
            UI_TEXT ui_text;

            {
            STATUS status = ui_data_query_as_text(p_lbox->ui_data_type, p_lbox->p_ui_source, itemno, p_lbox->p_ui_control, &ui_text);
            if(status_fail(status))
                continue;
            } /*block*/

            {
            PCTSTR test_tstr = ui_text_tstr_no_default(&ui_text);
            BOOL differ = ((NULL == test_tstr) || !sbstr_compare_equals_nocase(test_tstr, tstr));
            ui_text_dispose(&ui_text);
            if(differ)
                continue;
            } /*block*/

            return(ri_lbox_selection_set_from_itemno(handle, itemno));
        }

        /* maybe restart unsuccessful loop ending at this start */
        end_itemno = stt_itemno;
        stt_itemno = 0;
    }
    while(++pass <= 2);

    return(ri_lbox_selection_set_from_itemno(handle, RI_LBOX_SELECTION_NONE));
}

/******************************************************************************
*
* recompute the list box window's extent
*
******************************************************************************/

static void
ri_lbox_extent_set(
    P_RI_LBOX p_lbox,
    _In_        const WimpOpenWindowBlock * const p_open_window_block)
{
    BBox extent_bbox;

    if(0 == p_lbox->window_handle)
        return;

    extent_bbox.xmin = p_lbox->prev_extent.xmin;
    extent_bbox.ymax = p_lbox->prev_extent.ymax;

    extent_bbox.xmax = extent_bbox.xmin + MAX(BBox_width(&p_open_window_block->visible_area),  (int) p_lbox->max_item_len * 16);
    extent_bbox.ymin = extent_bbox.ymax - MAX(BBox_height(&p_open_window_block->visible_area), (int) p_lbox->n_items * RI_LBOX_ITEM_HEIGHT);

    if( (p_lbox->prev_extent.xmax != extent_bbox.xmax) ||
        (p_lbox->prev_extent.ymin != extent_bbox.ymin) )
    {
        void_WrapOsErrorReporting(wimp_set_extent(p_lbox->window_handle, &extent_bbox));

        p_lbox->prev_extent.xmax = extent_bbox.xmax;
        p_lbox->prev_extent.ymin = extent_bbox.ymin;
    }
}

/******************************************************************************
*
* ensure the list box is visible
*
******************************************************************************/

_Check_return_
extern STATUS
ri_lbox_show(
    _InVal_     RI_LBOX_HANDLE handle)
{
    P_RI_LBOX p_lbox = ri_lbox_p_from_h(handle);
    WimpGetWindowStateBlock window_state;

    if(0 == p_lbox->window_handle)
        return(STATUS_OK);

    window_state.window_handle = p_lbox->window_handle;
    if(NULL == WrapOsErrorReporting(wimp_get_window_state(&window_state)))
    {
        if(0 == (WimpWindow_Open & window_state.flags))
        {
            p_lbox->bits.deferred_ensure_visible = 1;

            winx_send_front_window_request(p_lbox->window_handle, FALSE /*not immediate front*/, NULL);
        }
    }

    ri_lbox_current_ensure_visible(p_lbox, 0);

    return(STATUS_OK);
}

/******************************************************************************
*
* ensure the list box is visible
* and attached as the leaf submenu
*
******************************************************************************/

_Check_return_
extern STATUS
ri_lbox_show_submenu(
    _InVal_     RI_LBOX_HANDLE handle,
    _InVal_     S32 submenu,
    _InRef_opt_ PC_GDI_POINT p_tl /*NULL->lookup*/)
{
    P_RI_LBOX p_lbox = ri_lbox_p_from_h(handle);
    GDI_POINT tl;

    assert(p_lbox->window_handle);

    p_lbox->bits.deferred_ensure_visible = 1;

    if(p_tl)
        tl = *p_tl;
    else
        tl.x = tl.y = 0;

    if(submenu == RI_LBOX_SUBMENU_OF_COMPLEX_MENU)
    {
        PTR_ASSERT(p_tl);

        winx_create_submenu_child(p_lbox->window_handle, tl.x, tl.y);
    }
    else if(submenu)
    {
        if((NULL == p_tl) && event_query_submenudata_valid())
            status_assert(event_read_submenudata(NULL, &tl.x, &tl.y)); /* ask about menu position */

        winx_create_submenu(p_lbox->window_handle, tl.x, tl.y);
    }
    else
    {
        if(NULL == p_tl)
        {
            WimpGetPointerInfoBlock pointer_info;

            void_WrapOsErrorReporting(wimp_get_pointer_info(&pointer_info));

            tl.x = pointer_info.x - 32; /* try to be a bit into the window */
            tl.y = pointer_info.y + 32;
        }

        winx_create_menu(p_lbox->window_handle, tl.x, tl.y);
    }

    /*ri_lbox_current_ensure_visible(p_lbox);*/

    return(STATUS_OK);
}

/******************************************************************************
*
* items (unspecified) in a list box have been modified
*
* does this smell like it needs UREF-like handling
* for completeness?
*
******************************************************************************/

_Check_return_
extern STATUS
ri_lbox_source_modified(
    _InVal_     RI_LBOX_HANDLE handle)
{
    P_RI_LBOX p_lbox = ri_lbox_p_from_h(handle);
    S32 old_n_items = p_lbox->n_items;

    p_lbox->n_items = ui_data_n_items_query(p_lbox->ui_data_type, p_lbox->p_ui_source);

    if(old_n_items != p_lbox->n_items)
    {
        if(0 != p_lbox->window_handle)
        {
            union wimp_window_state_open_window_block_u window_u;

            window_u.window_state_block.window_handle = p_lbox->window_handle;
            void_WrapOsErrorReporting(wimp_get_window_state(&window_u.window_state_block));

            ri_lbox_extent_set(p_lbox, &window_u.open_window_block);

            /* queue an open so the sausage gets redrawn */
            winx_send_open_window_request(p_lbox->window_handle, FALSE, &window_u.open_window_block);
        }
    }

    ri_lbox_item_update(p_lbox, 0, MAX(old_n_items, p_lbox->n_items), RI_LBOX_UPDATE_LATER);

    return(STATUS_OK);
}

#endif /* RISCOS */

/* end of ri_lbox.c */
