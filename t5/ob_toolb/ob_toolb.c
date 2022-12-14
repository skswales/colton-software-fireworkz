/* ob_toolb.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Toolbar object module for Fireworkz */

/* SKS December 1993 */

#include "common/gflags.h"

#include "ob_toolb/xp_toolb.h"

#include "ob_toolb/ob_toolb.h"

#if RISCOS
#define EXPOSE_RISCOS_FONT 1
#define EXPOSE_RISCOS_SWIS 1

#include "ob_skel/xp_skelr.h"
#endif

#if RISCOS
#if defined(BOUND_MESSAGES_OBJECT_ID_TOOLB)
extern PC_U8 rb_toolb_msg_weak;
#define P_BOUND_MESSAGES_OBJECT_ID_TOOLB &rb_toolb_msg_weak
#else
#define P_BOUND_MESSAGES_OBJECT_ID_TOOLB LOAD_MESSAGES_FILE
#endif
#else
#define P_BOUND_MESSAGES_OBJECT_ID_TOOLB DONT_LOAD_MESSAGES_FILE
#endif

#define P_BOUND_RESOURCES_OBJECT_ID_TOOLB DONT_LOAD_RESOURCES

#if RISCOS
#define LAYER_TOOLBAR LAYER_PAPER_ABOVE
#endif

/*
internal routines
*/

OBJECT_PROTO(extern, object_toolbar);

_Check_return_
_Ret_maybenull_
static P_T5_TOOLBAR_DOCU_TOOL_DESC
find_docu_tool_uchars(
    _DocuRef_   P_DOCU p_docu,
    _In_reads_(name_len) PC_UCHARS name,
    _InVal_     U32 name_len);

_Check_return_
_Ret_maybenull_
static P_T5_TOOLBAR_DOCU_TOOL_DESC
find_docu_tool(
    _DocuRef_   P_DOCU p_docu,
    _In_z_      PC_USTR name)
{
    return(find_docu_tool_uchars(p_docu, name, ustrlen32(name)));
}

static void
schedule_tool_redraw(
    _DocuRef_   P_DOCU p_docu,
    _In_z_      PC_USTR name,
    _InVal_     S32 require_count);

static void
schedule_toolbar_redraw(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InVal_     ARRAY_INDEX view_row_idx);

static void
status_line_view_dispose(
    _ViewRef_   P_VIEW p_view);

_Check_return_
static STATUS
status_line_view_new(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view);

static void
toolbar_view_dispose(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view);

_Check_return_
static STATUS
toolbar_view_new(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InVal_     BOOL for_view_new);

#if RISCOS
#define T5_TOOLBAR_LM           ( 8 * PIXITS_PER_RISCOS)
#define T5_TOOLBAR_RM           (-2 * PIXITS_PER_RISCOS)
#define T5_TOOLBAR_TM           ( 4 * PIXITS_PER_RISCOS)
#define T5_TOOLBAR_BM           ( 4 * PIXITS_PER_RISCOS)

#define T5_TOOLBAR_TOOL_SEP_H   (12 * PIXITS_PER_RISCOS)
#define T5_TOOLBAR_TOOL_SEP_H_THEMED T5_TOOLBAR_TOOL_SEP_H
#define T5_TOOLBAR_ROW_SEP_V    ( 4 * PIXITS_PER_RISCOS)

#define T5_TOOLBAR_LARGE_WIDTH  (16384 * PIXITS_PER_RISCOS)
#elif WINDOWS
/* Defined in terms of Windows screen pixels - these become scaled by DPI in transforms, which is what we want */
#define T5_TOOLBAR_LM           ( 8 * PIXITS_PER_PIXEL)
#define T5_TOOLBAR_RM           ( 0 * PIXITS_PER_PIXEL)
#define T5_TOOLBAR_TM           ( 2 * PIXITS_PER_PIXEL)
#define T5_TOOLBAR_BM           ( 2 * PIXITS_PER_PIXEL)

#define T5_TOOLBAR_TOOL_SEP_H   ( 6 * PIXITS_PER_PIXEL) /* from IDG */
#define T5_TOOLBAR_TOOL_SEP_H_THEMED (12 * PIXITS_PER_PIXEL) /* better sized gap when using themed separator */
#define T5_TOOLBAR_ROW_SEP_V    ( 2 * PIXITS_PER_PIXEL)
#endif /* OS */

#if RISCOS
#define TOOL_BUTTON_STD_WIDTH   (48 * PIXITS_PER_RISCOS) /* core 15 @ dy=2, 8 @ dy=4 */
#define TOOL_BUTTON_THIN_WIDTH  (28 * PIXITS_PER_RISCOS) /* core  6 @ dx=2 */
#define TOOL_BUTTON_STD_HEIGHT  (48 * PIXITS_PER_RISCOS) /* core 16 @ dx=2 */
#endif

/* ------------------------------------------------------------------------------------------------------- */

static ARRAY_HANDLE toolbar_requested_array_handle;  /* [] of T5_TOOLBAR_REQUESTED_DESC */

static ARRAY_HANDLE toolbar_disallowed_array_handle; /* [] of T5_TOOLBAR_DISALLOWED_DESC */

static ARRAY_HANDLE toolbar_candidates_array_handle; /* [] of PC_T5_TOOLBAR_TOOL_DESC */

#if RISCOS
static PIXIT status_line_font_height_pixits;
#elif WINDOWS
static GDI_COORD status_line_font_height_pixels;

       GDI_COORD status_line_height_pixels = 0; /* ideal value for open, don't reset to zero */
#endif

#if RISCOS
static GDI_COORD toolbar_height_riscos = 0; /* ideal value for open, don't reset to zero */
#else
       GDI_COORD toolbar_height_pixels = 0; /* ideal value for open, don't reset to zero */
#endif

/*
construct argument types
*/

static const ARG_TYPE
args_cmd_toolbar_tool[] =
{
    ARG_TYPE_S32 | ARG_MANDATORY,       /* which toolbar */
    ARG_TYPE_USTR | ARG_MANDATORY_OR_BLANK, /* id name : BLANK -> separator */
    ARG_TYPE_NONE
};

/*
construct table
*/

static CONSTRUCT_TABLE
object_construct_table[] =
{                                                                                                   /*   fi ti mi ur up xi md mf nn cp sm ba fo */

    { "Tool",                   args_cmd_toolbar_tool,          T5_CMD_TOOLBAR_TOOL },
    { "ToolPossible",           args_cmd_toolbar_tool,          T5_CMD_COMMENT }, /* there just to comment the Choices18 file for the advanced hacker */

    { "StatusLine",             NULL,                           T5_CMD_REMOVED },

    { NULL,                     NULL,                           T5_EVENT_NONE } /* end of table */
};

/*
message handlers
*/

T5_MSG_PROTO(static, toolbar_msg_view_destroy, _InRef_ P_T5_MSG_VIEW_DESTROY_BLOCK p_t5_msg_view_destroy_block)
{
    const P_VIEW p_view = p_t5_msg_view_destroy_block->p_view;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    status_line_view_dispose(p_view);

    toolbar_view_dispose(p_docu, p_view);

    return(STATUS_OK);
}

T5_MSG_PROTO(static, toolbar_msg_view_new, _InRef_ P_T5_MSG_VIEW_NEW_BLOCK p_t5_msg_view_new_block)
{
    const P_VIEW p_view = p_t5_msg_view_new_block->p_view;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    status_return(status_line_view_new(p_docu, p_view));

    return(toolbar_view_new(p_docu, p_view, TRUE));
}

#if WINDOWS

static HBRUSH toolbar_hBrush; /* this is set by paint_toolbar() */

__pragma(warning(push))
__pragma(warning(disable:4255)) /* no function prototype given: converting '()' to '(void) (Windows SDK 6.0A) */
#include <Uxtheme.h>
__pragma(warning(pop))

#include <vssym32.h>

#pragma comment(lib, "Uxtheme.lib")

static HTHEME g_hTheme;

static BOOL
themed_toolbar_UIToolButtonDraw_32bpp_DISABLED(
    HTHEME hTheme,
    HDC hDC, PCRECT pRect,
    HBITMAP hBitmap, int bmx, int bmy, int iImageIndex,
    UINT iStateId)
{
    const int iPartId = TP_BUTTON;
    const HDC hdcMem = CreateCompatibleDC(hDC);
    HBITMAP hbmOld = SelectBitmap(hdcMem, hBitmap);
    RECT icon_rect;
    consume(HRESULT, DrawThemeBackground(hTheme, hDC, iPartId, iStateId, pRect, NULL));
    if(SUCCEEDED(GetThemeBackgroundContentRect(hTheme, hDC, iPartId, iStateId, pRect, &icon_rect)))
    {   /* centre icon in its slot */
        LONG content_cx = icon_rect.right - icon_rect.left;
        LONG content_cy = icon_rect.bottom - icon_rect.top;
        static BLENDFUNCTION bf = { AC_SRC_OVER, 0, 96 /*alpha*/, AC_SRC_ALPHA };
        if(bmx < content_cx)
            icon_rect.left += (content_cx - bmx) / 2;
        if(bmy < content_cy)
            icon_rect.top  += (content_cy - bmy) / 2;
        icon_rect.right  = icon_rect.left + bmx;
        icon_rect.bottom = icon_rect.top  + bmy;
        void_WrapOsBoolChecking(AlphaBlend(hDC, icon_rect.left, icon_rect.top, icon_rect.right - icon_rect.left, icon_rect.bottom - icon_rect.top, hdcMem, iImageIndex * bmx, 0, bmx, bmy, bf));
    }
    SelectBitmap(hdcMem, hbmOld);
    DeleteDC(hdcMem);
    return(TRUE);
}

static BOOL
themed_toolbar_UIToolButtonDraw(
    HTHEME hTheme, BOOL f32bpp,
    HDC hDC, PCRECT pRect,
    HBITMAP hBitmap, int bmx, int bmy, int iImageIndex, UINT uStateIn)
{
    const int iPartId = TP_BUTTON;
    int iStateId;
    HIMAGELIST hImageList;
    int i;

    if(uStateIn & BUTTONGROUP_DISABLED)
    {
        iStateId = TS_DISABLED;
    }
    else if(uStateIn & BUTTONGROUP_MOUSEDOWN)
    {
        iStateId = TS_PRESSED;
    }
    else if(uStateIn & BUTTONGROUP_MOUSEOVER)
    {
        if(uStateIn & BUTTONGROUP_DOWN)
            iStateId = TS_HOTCHECKED;
        else
            iStateId = TS_HOT;
    }
    else
    {
        if(uStateIn & BUTTONGROUP_DOWN)
            iStateId = TS_CHECKED;
        else
            iStateId = TS_NORMAL;
    }

    if( f32bpp && (uStateIn & BUTTONGROUP_DISABLED) )
        return(themed_toolbar_UIToolButtonDraw_32bpp_DISABLED(hTheme, hDC, pRect, hBitmap, bmx, bmy, iImageIndex, iStateId));

    if(NULL == (hImageList = ImageList_Create(bmx, bmy, f32bpp ? ILC_COLOR32 : (ILC_COLOR | ILC_MASK), 20, 20)))
        return(FALSE);

    ImageList_SetBkColor(hImageList, CLR_NONE);

    if(f32bpp)
    {
        i = ImageList_Add(hImageList, hBitmap, NULL);
    }
    else
    {
        /* NB ImageList_AddMasked trashes hBitmapCopy */
        HBITMAP hBitmapCopy;

        if(NULL == (hBitmapCopy = CopyImage(hBitmap, IMAGE_BITMAP, 0, 0, 0)))
        {
            i = -1;
        }
        else
        {
            i = ImageList_AddMasked(hImageList, hBitmapCopy, RGB(192, 192, 192) /* LTGRAY is the 4-bpp image colour key */);

            DeleteBitmap(hBitmapCopy);
        }
    }

    if(i < 0)
    {
        ImageList_Destroy(hImageList);
        return(FALSE);
    }

    {
    RECT icon_rect;
    consume(HRESULT, DrawThemeBackground(hTheme, hDC, iPartId, iStateId, pRect, NULL));
    if(SUCCEEDED(GetThemeBackgroundContentRect(hTheme, hDC, iPartId, iStateId, pRect, &icon_rect)))
    {   /* centre icon in its slot */
        LONG content_cx = icon_rect.right - icon_rect.left;
        LONG content_cy = icon_rect.bottom - icon_rect.top;
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
    }
    } /*block*/

    ImageList_Destroy(hImageList);

    return(TRUE);
}

static BOOL
toolbar_UIToolButtonDrawTDD(
    HTHEME hTheme, BOOL f32bpp,
    HDC hDC, PCRECT pRect,
    HBITMAP hBmp, int bmx, int bmy, int iImageIndex, UINT uStateIn,
    LPTOOLDISPLAYDATA pTDD)
{
    UINT uState;

    if( (NULL != hTheme) && themed_toolbar_UIToolButtonDraw(hTheme, f32bpp, hDC, pRect, hBmp, bmx, bmy, iImageIndex, uStateIn) )
        return(TRUE);

    /* fallback is old implementation */
    //assert(!f32bpp);
    uState = uStateIn;
    uState |= (PRESERVE_WHITE); /* | PRESERVE_DKGRAY | PRESERVE_BLACK); need these to map for high-contrast */ /* LTGRAY is colour key */
    return(UIToolButtonDrawTDD(hDC, pRect->left, pRect->top, pRect->right - pRect->left, pRect->bottom - pRect->top,
                               hBmp, bmx, bmy, iImageIndex, uState, pTDD));
}

static void
fill_pixit_rect(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_RECT p_pixit_rect,
    HBRUSH hBrush)
{
    RECT rect;

    if(!status_done(RECT_limited_from_pixit_rect_and_context(&rect, p_pixit_rect, p_redraw_context)))
        return;

    if(g_hTheme)
    {
        consume(HRESULT, DrawThemeBackground(g_hTheme, p_redraw_context->windows.paintstruct.hdc, 0, 0, &rect, NULL));
        return;
    }

    FillRect(p_redraw_context->windows.paintstruct.hdc, &rect, hBrush);
}

/* fill the separator to the right of this tool (if present) */

static void
do_paint_separator(
    _InRef_     PC_T5_TOOLBAR_VIEW_TOOL_DESC p_t5_toolbar_view_tool_desc,
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context)
{
    RECT rect;
    PIXIT_RECT rhs_extra_rect = p_t5_toolbar_view_tool_desc->pixit_rect;
    rhs_extra_rect.tl.x = rhs_extra_rect.br.x;
    rhs_extra_rect.br.x += p_t5_toolbar_view_tool_desc->rhs_extra_div_20 * 20;
    assert(0 != p_t5_toolbar_view_tool_desc->rhs_extra_div_20);

    if(!status_done(RECT_limited_from_pixit_rect_and_context(&rect, &rhs_extra_rect, p_redraw_context)))
        return;

    if(g_hTheme)
    {
        FillRect(p_redraw_context->windows.paintstruct.hdc, &rect, toolbar_hBrush);
        rect.left = ((rect.right + rect.left) / 2) - 1; /* centre the separator */
        consume(HRESULT, DrawThemeBackground(g_hTheme, p_redraw_context->windows.paintstruct.hdc, TP_SEPARATOR, 0, &rect, NULL));
        return;
    }

    if(p_redraw_context->windows.paintstruct.fErase)
        FillRect(p_redraw_context->windows.paintstruct.hdc, &rect, toolbar_hBrush);
}

#endif /* OS */

T5_MSG_PROTO(static, msg_toolbar_tool_disable, _InRef_ PC_T5_TOOLBAR_TOOL_DISABLE p_t5_toolbar_tool_disable)
{
    const P_T5_TOOLBAR_DOCU_TOOL_DESC p_t5_toolbar_docu_tool_desc = find_docu_tool(p_docu, p_t5_toolbar_tool_disable->name);

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(NULL == p_t5_toolbar_docu_tool_desc)
        return(create_error(TOOLB_ERR_UNKNOWN_CONTROL));

    {
    U32 old_disable_count = bitmap_count(p_t5_toolbar_docu_tool_desc->disable_state, N_BITS_ARG(32));
    U32 new_disable_count;
    if(p_t5_toolbar_tool_disable->disabled)
        bitmap_bit_set(p_t5_toolbar_docu_tool_desc->disable_state, p_t5_toolbar_tool_disable->disable_id, N_BITS_ARG(32));
    else
        bitmap_bit_clear(p_t5_toolbar_docu_tool_desc->disable_state, p_t5_toolbar_tool_disable->disable_id, N_BITS_ARG(32));
    new_disable_count = bitmap_count(p_t5_toolbar_docu_tool_desc->disable_state, N_BITS_ARG(32));
    if(new_disable_count != old_disable_count)
        schedule_tool_redraw(p_docu, p_t5_toolbar_tool_disable->name, -1);
    return(STATUS_OK);
    } /*block*/
}

_Check_return_
extern STATUS
_tool_enable(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_T5_TOOLBAR_TOOL_ENABLE p_t5_toolbar_tool_enable)
{
    const P_T5_TOOLBAR_DOCU_TOOL_DESC p_t5_toolbar_docu_tool_desc = find_docu_tool(p_docu, p_t5_toolbar_tool_enable->name);

    if(NULL == p_t5_toolbar_docu_tool_desc)
        return(create_error(TOOLB_ERR_UNKNOWN_CONTROL));

    {
    U32 old_enable_count = bitmap_count(p_t5_toolbar_docu_tool_desc->enable_state, N_BITS_ARG(32));
    U32 new_enable_count;
    if(p_t5_toolbar_tool_enable->enabled)
        bitmap_bit_set(p_t5_toolbar_docu_tool_desc->enable_state, p_t5_toolbar_tool_enable->enable_id, N_BITS_ARG(32));
    else
        bitmap_bit_clear(p_t5_toolbar_docu_tool_desc->enable_state, p_t5_toolbar_tool_enable->enable_id, N_BITS_ARG(32));
    new_enable_count = bitmap_count(p_t5_toolbar_docu_tool_desc->enable_state, N_BITS_ARG(32));
    if(new_enable_count != old_enable_count)
        schedule_tool_redraw(p_docu, p_t5_toolbar_tool_enable->name, +1);
    return(STATUS_OK);
    } /*block*/
}

T5_MSG_PROTO(static, msg_toolbar_tool_enable, _InRef_ PC_T5_TOOLBAR_TOOL_ENABLE p_t5_toolbar_tool_enable)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    return(_tool_enable(p_docu, p_t5_toolbar_tool_enable));
}

T5_MSG_PROTO(static, msg_toolbar_tool_enable_query, _InoutRef_ P_T5_TOOLBAR_TOOL_ENABLE_QUERY p_t5_toolbar_tool_enable_query)
{
    const P_T5_TOOLBAR_DOCU_TOOL_DESC p_t5_toolbar_docu_tool_desc = find_docu_tool_uchars(p_docu, p_t5_toolbar_tool_enable_query->name, p_t5_toolbar_tool_enable_query->name_len);

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(NULL == p_t5_toolbar_docu_tool_desc)
    {
        p_t5_toolbar_tool_enable_query->enabled = FALSE;

        return(create_error(TOOLB_ERR_UNKNOWN_CONTROL));
    }

    p_t5_toolbar_tool_enable_query->enabled = tool_enabled_query(p_t5_toolbar_docu_tool_desc);

    return(STATUS_OK);
}

extern void
execute_tool(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_maybenone_ P_VIEW p_view,
    _InRef_     PC_T5_TOOLBAR_DOCU_TOOL_DESC p_t5_toolbar_docu_tool_desc,
    _InVal_     S32 alternate /* expanded from BOOL */)
{
    STATUS status;
    const PC_T5_TOOLBAR_TOOL_DESC p_t5_toolbar_tool_desc = p_t5_toolbar_docu_tool_desc->p_t5_toolbar_tool_desc;
    T5_MESSAGE t5_message = p_t5_toolbar_tool_desc->t5_message;
    OBJECT_ID object_id = p_t5_toolbar_tool_desc->command_object_id;

    if(alternate)
    {
        if(0 == p_t5_toolbar_tool_desc->t5_message_alternate)
        {
            reperr_null(ERR_NO_ALTERNATE_COMMAND);
            return;
        }

        t5_message = p_t5_toolbar_tool_desc->t5_message_alternate;
    }

    switch(ENUM_UNPACK(T5_TOOLBAR_TOOL_TYPE, p_t5_toolbar_tool_desc->bits.type))
    {
    default:
        break;

    case T5_TOOLBAR_TOOL_TYPE_COMMAND:
        {
        const ARGLIST_HANDLE arglist_handle = 0; /* toolbar commands can't have any args! */

        if(p_t5_toolbar_tool_desc->bits.set_view && VIEW_NOT_NONE(p_view))
            status_assert(command_set_current_view(p_docu, p_view));

        /*if(p_t5_toolbar_tool_desc->bits.set_interactive)
            command_set_interactive();*/

        if(status_fail(status = object_load(object_id)))
        {
            reperr_null(status);
            return;
        }

        status_consume(execute_command_reperr(p_docu, t5_message, &arglist_handle, object_id));
        break;
        }

    case T5_TOOLBAR_TOOL_TYPE_STATE:
        {
        /* propose a state change to control's owner */
        T5_TOOLBAR_TOOL_STATE_CHANGE t5_toolbar_tool_state_change;
        assert(t5_message);
        t5_toolbar_tool_state_change.name = p_t5_toolbar_tool_desc->name;
        t5_toolbar_tool_state_change.current_state.state = p_t5_toolbar_docu_tool_desc->state.state;
        t5_toolbar_tool_state_change.proposed_state.state = (t5_toolbar_tool_state_change.current_state.state != 1); /* 0,2 -> 1; 1 -> 0 */
        if(status_fail(status = object_call_id(object_id, p_docu, t5_message, &t5_toolbar_tool_state_change)))
        {
            reperr_null(status);
            return;
        }
        break;
        }
    }
}

/* NB equal hash is a *guarantee* of equal length for fast subsequent comparision */

_Check_return_
static inline U32
trivial_name_hash(
    _In_reads_(name_len) PC_UCHARS name,
    _InVal_     U32 name_len)
{
    U32 name_hash = name_len;
    if(0 != name_len)
    {   /* NB do NOT pollute the low byte of the hash - see above guarantee */
        name_hash ^= (((U32) PtrGetByteOff(name,          0)) << 16);
        name_hash ^= (((U32) PtrGetByteOff(name, name_len-1)) << 24);
    }
    return(name_hash);
}

_Check_return_
_Ret_maybenull_
static P_T5_TOOLBAR_DOCU_TOOL_DESC
find_docu_tool_uchars(
    _DocuRef_   P_DOCU p_docu,
    _In_reads_(name_len) PC_UCHARS name,
    _InVal_     U32 name_len)
{
    const U32 name_hash = trivial_name_hash(name, name_len);
    const ARRAY_INDEX n_elements = array_elements(&p_docu->h_toolbar);
    ARRAY_INDEX i;
    P_T5_TOOLBAR_DOCU_TOOL_DESC p_t5_toolbar_docu_tool_desc = array_range(&p_docu->h_toolbar, T5_TOOLBAR_DOCU_TOOL_DESC, 0, n_elements);

    for(i = 0; i < n_elements; ++i, ++p_t5_toolbar_docu_tool_desc)
    {
        if(name_hash != p_t5_toolbar_docu_tool_desc->name_hash)
            continue;

        if(0 != memcmp32(p_t5_toolbar_docu_tool_desc->p_t5_toolbar_tool_desc->name, name, name_len))
            continue;

        return(p_t5_toolbar_docu_tool_desc);
    }

    return(NULL);
}

_Check_return_
static STATUS
new_view_tool(
    _InoutRef_  P_ARRAY_HANDLE p_h_toolbar_view_row_desc /*extended*/,
    _InVal_     ARRAY_INDEX docu_tool_index,
    _InVal_     ARRAY_INDEX row,
    _InVal_     BOOL separator)
{
    P_T5_TOOLBAR_VIEW_ROW_DESC p_t5_toolbar_view_row_desc;
    ARRAY_INDEX delta = (row + 1) - array_elements(p_h_toolbar_view_row_desc); /* ensure there is an entry for this row */
    STATUS status = STATUS_OK;

    if(delta > 0)
    {
        SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(*p_t5_toolbar_view_row_desc), TRUE);

        if(separator)
            return(status);

        if(NULL == (p_t5_toolbar_view_row_desc = al_array_extend_by(p_h_toolbar_view_row_desc, T5_TOOLBAR_VIEW_ROW_DESC, delta, &array_init_block, &status)))
            return(status); /* we have a half-formed toolbar */
    }

    p_t5_toolbar_view_row_desc = array_ptr(p_h_toolbar_view_row_desc, T5_TOOLBAR_VIEW_ROW_DESC, row);

    if(separator)
    {
        const U32 row_tools = array_elements32(&p_t5_toolbar_view_row_desc->h_toolbar_view_tool_desc);
        if(0 != row_tools)
        {
            const P_T5_TOOLBAR_VIEW_TOOL_DESC p_t5_toolbar_view_tool_desc = array_ptr(&p_t5_toolbar_view_row_desc->h_toolbar_view_tool_desc, T5_TOOLBAR_VIEW_TOOL_DESC, row_tools - 1);
            p_t5_toolbar_view_tool_desc->separator = TRUE;
        }
    }
    else
    {
        P_T5_TOOLBAR_VIEW_TOOL_DESC p_t5_toolbar_view_tool_desc;
        SC_ARRAY_INIT_BLOCK array_init_block = aib_init(4, sizeof32(*p_t5_toolbar_view_tool_desc), TRUE);

        if(NULL == (p_t5_toolbar_view_tool_desc = al_array_extend_by(&p_t5_toolbar_view_row_desc->h_toolbar_view_tool_desc, T5_TOOLBAR_VIEW_TOOL_DESC, 1, &array_init_block, &status)))
            return(status); /* we have a half-formed toolbar */

        p_t5_toolbar_view_tool_desc->docu_tool_index = docu_tool_index;
    }

    return(status);
}

T5_MSG_PROTO(static, msg_toolbar_tool_nobble, _InRef_ PC_T5_TOOLBAR_TOOL_NOBBLE p_t5_toolbar_tool_nobble)
{
    const PC_USTR name = p_t5_toolbar_tool_nobble->name;
    const P_T5_TOOLBAR_DOCU_TOOL_DESC p_t5_toolbar_docu_tool_desc = find_docu_tool(p_docu, name);

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(NULL == p_t5_toolbar_docu_tool_desc)
        return(/*create_error*/(TOOLB_ERR_UNKNOWN_CONTROL));

    if(p_t5_toolbar_docu_tool_desc->nobble_state == p_t5_toolbar_tool_nobble->nobbled)
        return(STATUS_OK);

    p_t5_toolbar_docu_tool_desc->nobble_state = p_t5_toolbar_tool_nobble->nobbled;

    {
    VIEWNO viewno = VIEWNO_NONE;
    P_VIEW p_view;

    while(P_VIEW_NONE != (p_view = docu_enum_views(p_docu, &viewno)))
    {
        if(p_view->toolbar.nobble_scheduled_event_id)
            continue;

        p_view->toolbar.nobble_scheduled_event_id = viewid_pack(p_docu, p_view, 0xFFFFU);

        trace_1(TRACE__SCHEDULED, TEXT("msg_toolbar_tool_nobble - *** scheduled_event_after(docno=") DOCNO_TFMT TEXT(", n)"), docno_from_p_docu(p_docu));
        status_assert(scheduled_event_after(docno_from_p_docu(p_docu), T5_EVENT_SCHEDULED, object_toolbar, p_view->toolbar.nobble_scheduled_event_id, MONOTIMEDIFF_VALUE_FROM_MS(250)));
    }
    } /*block*/

    return(STATUS_OK);
}

T5_MSG_PROTO(static, msg_toolbar_tool_query, _InoutRef_ P_T5_TOOLBAR_TOOL_QUERY p_t5_toolbar_tool_query)
{
    const P_T5_TOOLBAR_DOCU_TOOL_DESC p_t5_toolbar_docu_tool_desc = find_docu_tool(p_docu, p_t5_toolbar_tool_query->name);

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(NULL == p_t5_toolbar_docu_tool_desc)
        return(create_error(TOOLB_ERR_UNKNOWN_CONTROL));

    switch(ENUM_UNPACK(T5_TOOLBAR_TOOL_TYPE, p_t5_toolbar_docu_tool_desc->p_t5_toolbar_tool_desc->bits.type))
    {
    default:
        break;

    case T5_TOOLBAR_TOOL_TYPE_STATE:
        p_t5_toolbar_tool_query->state.state = p_t5_toolbar_docu_tool_desc->state.state;
        break;
    }

    return(STATUS_OK);
}

_Check_return_
static BOOL
intersect_pixit_rect(
    _OutRef_    P_PIXIT_RECT p_pixit_rect_clip,
    _InRef_     PC_PIXIT_RECT p_pixit_rect_1,
    _InRef_     PC_PIXIT_RECT p_pixit_rect_2)
{
    p_pixit_rect_clip->tl.x = MAX(p_pixit_rect_1->tl.x, p_pixit_rect_2->tl.x);
    p_pixit_rect_clip->tl.y = MAX(p_pixit_rect_1->tl.y, p_pixit_rect_2->tl.y);
    p_pixit_rect_clip->br.x = MIN(p_pixit_rect_1->br.x, p_pixit_rect_2->br.x);
    p_pixit_rect_clip->br.y = MIN(p_pixit_rect_1->br.y, p_pixit_rect_2->br.y);

    if((p_pixit_rect_clip->tl.x >= p_pixit_rect_clip->br.x) || (p_pixit_rect_clip->tl.y >= p_pixit_rect_clip->br.y))
        return(FALSE);

    return(TRUE);
}

#if RISCOS

static BOOL g_Win31 = FALSE;

static const U32 desktop_default_palette_16[16*2] =
{   /*BBGGRR00*/
    0xFFFFFF00U,0xFFFFFF00U,
    0xDDDDDD00U,0xDDDDDD00U,
    0xBBBBBB00U,0xBBBBBB00U,
    0x99999900U,0x99999900U,
    0x77777700U,0x77777700U,
    0x55555500U,0x55555500U,
    0x33333300U,0x33333300U,
    0x00000000U,0x00000000U,

    0x88440000U,0x88440000U,
    0xEEEE0000u,0xEEEE0000U,
    0x00DD0000U,0x00DD0000U,
    0x0000DD00u,0x0000DD00U,
    0xCCEEEE00U,0xCCEEEE00U,
    0x00884400U,0x00884400U,
    0x00CCFF00U,0x00CCFF00U,
    0xFFCC0000U,0xFFCC0000U
};

static const U32 desktop_greyed_palette_16[16*2] =
{   /*BBGGRR00*/
    0xFFFFFF00U,0xFFFFFF00U,
    0xEEEEEE00U,0xEEEEEE00U,
    0xDDDDDD00U,0xDDDDDD00U,
    0xCCCCCC00U,0xCCCCCC00U,
    0xBBBBBB00U,0xBBBBBB00U,
    0xAAAAAA00U,0xAAAAAA00U,
    0x99999900U,0x99999900U,
    0x88888800U,0x88888800U,

    0xCC995500U,0xCC995500U,
    0xF6F64400U,0xF6F64400U,
    0xCCEECC00U,0xCCEECC00U,
    0xCCCCEE00U,0xCCCCEE00U,
    0xE4F6F600U,0xE4F6F600U,
    0x88CC8800U,0x88CC8800U,
    0xCCE4FF00U,0xCCE4FF00U,
    0xFFE44400U,0xFFE44400U
};

static const U32 desktop_default_palette_256[256*2] =
{   /*BBGGRR00*/
    0x00000000U,0x00000000U,
    0x11111100U,0x11111100U,
    0x22222200U,0x22222200U,
    0x33333300U,0x33333300U,

    0x00004400U,0x00004400U,
    0x11115500U,0x11115500U,
    0x22226600U,0x22226600U,
    0x33337700U,0x33337700U,

    0x44000000U,0x44000000U,
    0x55111100U,0x55111100U,
    0x66222200U,0x66222200U,
    0x77333300U,0x77333300U,

    0x44004400U,0x44004400U,
    0x55115500U,0x55115500U,
    0x66226600U,0x66226600U,
    0x77337700U,0x77337700U,

    0x00008800U,0x00008800U,
    0x11119900U,0x11119900U,
    0x2222AA00U,0x2222AA00U,
    0x3333BB00U,0x3333BB00U,

    0x0000CC00U,0x0000CC00U,
    0x1111DD00U,0x1111DD00U,
    0x2222EE00U,0x2222EE00U,
    0x3333FF00U,0x3333FF00U,

    0x44008800U,0x44008800U,
    0x55119900U,0x55119900U,
    0x6622AA00U,0x6622AA00U,
    0x7733BB00U,0x7733BB00U,

    0x4400CC00U,0x4400CC00U,
    0x5511DD00U,0x5511DD00U,
    0x6622EE00U,0x6622EE00U,
    0x7733FF00U,0x7733FF00U,

    0x00440000U,0x00440000U,
    0x11551100U,0x11551100U,
    0x22662200U,0x22662200U,
    0x33773300U,0x33773300U,

    0x00444400U,0x00444400U,
    0x11555500U,0x11555500U,
    0x22666600U,0x22666600U,
    0x33777700U,0x33777700U,

    0x44440000U,0x44440000U,
    0x55551100U,0x55551100U,
    0x66662200U,0x66662200U,
    0x77773300U,0x77773300U,

    0x44444400U,0x44444400U,
    0x55555500U,0x55555500U,
    0x66666600U,0x66666600U,
    0x77777700U,0x77777700U,

    0x00448800U,0x00448800U,
    0x11559900U,0x11559900U,
    0x2266AA00U,0x2266AA00U,
    0x3377BB00U,0x3377BB00U,

    0x0044CC00U,0x0044CC00U,
    0x1155DD00U,0x1155DD00U,
    0x2266EE00U,0x2266EE00U,
    0x3377FF00U,0x3377FF00U,

    0x44448800U,0x44448800U,
    0x55559900U,0x55559900U,
    0x6666AA00U,0x6666AA00U,
    0x7777BB00U,0x7777BB00U,

    0x4444CC00U,0x4444CC00U,
    0x5555DD00U,0x5555DD00U,
    0x6666EE00U,0x6666EE00U,
    0x7777FF00U,0x7777FF00U,

    0x00880000U,0x00880000U,
    0x11991100U,0x11991100U,
    0x22AA2200U,0x22AA2200U,
    0x33BB3300U,0x33BB3300U,

    0x00884400U,0x00884400U,
    0x11995500U,0x11995500U,
    0x22AA6600U,0x22AA6600U,
    0x33BB7700U,0x33BB7700U,

    0x44880000U,0x44880000U,
    0x55991100U,0x55991100U,
    0x66AA2200U,0x66AA2200U,
    0x77BB3300U,0x77BB3300U,

    0x44884400U,0x44884400U,
    0x55995500U,0x55995500U,
    0x66AA6600U,0x66AA6600U,
    0x77BB7700U,0x77BB7700U,

    0x00888800U,0x00888800U,
    0x11999900U,0x11999900U,
    0x22AAAA00U,0x22AAAA00U,
    0x33BBBB00U,0x33BBBB00U,

    0x0088CC00U,0x0088CC00U,
    0x1199DD00U,0x1199DD00U,
    0x22AAEE00U,0x22AAEE00U,
    0x33BBFF00U,0x33BBFF00U,

    0x44888800U,0x44888800U,
    0x55999900U,0x55999900U,
    0x66AAAA00U,0x66AAAA00U,
    0x77BBBB00U,0x77BBBB00U,

    0x4488CC00U,0x4488CC00U,
    0x5599DD00U,0x5599DD00U,
    0x66AAEE00U,0x66AAEE00U,
    0x77BBFF00U,0x77BBFF00U,

    0x00CC0000U,0x00CC0000U,
    0x11DD1100U,0x11DD1100U,
    0x22EE2200U,0x22EE2200U,
    0x33FF3300U,0x33FF3300U,

    0x00CC4400U,0x00CC4400U,
    0x11DD5500U,0x11DD5500U,
    0x22EE6600U,0x22EE6600U,
    0x33FF7700U,0x33FF7700U,

    0x44CC0000U,0x44CC0000U,
    0x55DD1100U,0x55DD1100U,
    0x66EE2200U,0x66EE2200U,
    0x77FF3300U,0x77FF3300U,

    0x44CC4400U,0x44CC4400U,
    0x55DD5500U,0x55DD5500U,
    0x66EE6600U,0x66EE6600U,
    0x77FF7700U,0x77FF7700U,

    0x00CC8800U,0x00CC8800U,
    0x11DD9900U,0x11DD9900U,
    0x22EEAA00U,0x22EEAA00U,
    0x33FFBB00U,0x33FFBB00U,

    0x00CCCC00U,0x00CCCC00U,
    0x11DDDD00U,0x11DDDD00U,
    0x22EEEE00U,0x22EEEE00U,
    0x33FFFF00U,0x33FFFF00U,

    0x44CC8800U,0x44CC8800U,
    0x55DD9900U,0x55DD9900U,
    0x66EEAA00U,0x66EEAA00U,
    0x77FFBB00U,0x77FFBB00U,

    0x44CCCC00U,0x44CCCC00U,
    0x55DDDD00U,0x55DDDD00U,
    0x66EEEE00U,0x66EEEE00U,
    0x77FFFF00U,0x77FFFF00U,

    0x88000000U,0x88000000U,
    0x99111100U,0x99111100U,
    0xAA222200U,0xAA222200U,
    0xBB333300U,0xBB333300U,

    0x88004400U,0x88004400U,
    0x99115500U,0x99115500U,
    0xAA226600U,0xAA226600U,
    0xBB337700U,0xBB337700U,

    0xCC000000U,0xCC000000U,
    0xDD111100U,0xDD111100U,
    0xEE222200U,0xEE222200U,
    0xFF333300U,0xFF333300U,

    0xCC004400U,0xCC004400U,
    0xDD115500U,0xDD115500U,
    0xEE226600U,0xEE226600U,
    0xFF337700U,0xFF337700U,

    0x88008800U,0x88008800U,
    0x99119900U,0x99119900U,
    0xAA22AA00U,0xAA22AA00U,
    0xBB33BB00U,0xBB33BB00U,

    0x8800CC00U,0x8800CC00U,
    0x9911DD00U,0x9911DD00U,
    0xAA22EE00U,0xAA22EE00U,
    0xBB33FF00U,0xBB33FF00U,

    0xCC008800U,0xCC008800U,
    0xDD119900U,0xDD119900U,
    0xEE22AA00U,0xEE22AA00U,
    0xFF33BB00U,0xFF33BB00U,

    0xCC00CC00U,0xCC00CC00U,
    0xDD11DD00U,0xDD11DD00U,
    0xEE22EE00U,0xEE22EE00U,
    0xFF33FF00U,0xFF33FF00U,

    0x88440000U,0x88440000U,
    0x99551100U,0x99551100U,
    0xAA662200U,0xAA662200U,
    0xBB773300U,0xBB773300U,

    0x88444400U,0x88444400U,
    0x99555500U,0x99555500U,
    0xAA666600U,0xAA666600U,
    0xBB777700U,0xBB777700U,

    0xCC440000U,0xCC440000U,
    0xDD551100U,0xDD551100U,
    0xEE662200U,0xEE662200U,
    0xFF773300U,0xFF773300U,

    0xCC444400U,0xCC444400U,
    0xDD555500U,0xDD555500U,
    0xEE666600U,0xEE666600U,
    0xFF777700U,0xFF777700U,

    0x88448800U,0x88448800U,
    0x99559900U,0x99559900U,
    0xAA66AA00U,0xAA66AA00U,
    0xBB77BB00U,0xBB77BB00U,

    0x8844CC00U,0x8844CC00U,
    0x9955DD00U,0x9955DD00U,
    0xAA66EE00U,0xAA66EE00U,
    0xBB77FF00U,0xBB77FF00U,

    0xCC448800U,0xCC448800U,
    0xDD559900U,0xDD559900U,
    0xEE66AA00U,0xEE66AA00U,
    0xFF77BB00U,0xFF77BB00U,

    0xCC44CC00U,0xCC44CC00U,
    0xDD55DD00U,0xDD55DD00U,
    0xEE66EE00U,0xEE66EE00U,
    0xFF77FF00U,0xFF77FF00U,

    0x88880000U,0x88880000U,
    0x99991100U,0x99991100U,
    0xAAAA2200U,0xAAAA2200U,
    0xBBBB3300U,0xBBBB3300U,

    0x88884400U,0x88884400U,
    0x99995500U,0x99995500U,
    0xAAAA6600U,0xAAAA6600U,
    0xBBBB7700U,0xBBBB7700U,

    0xCC880000U,0xCC880000U,
    0xDD991100U,0xDD991100U,
    0xEEAA2200U,0xEEAA2200U,
    0xFFBB3300U,0xFFBB3300U,

    0xCC884400U,0xCC884400U,
    0xDD995500U,0xDD995500U,
    0xEEAA6600U,0xEEAA6600U,
    0xFFBB7700U,0xFFBB7700U,

    0x88888800U,0x88888800U,
    0x99999900U,0x99999900U,
    0xAAAAAA00U,0xAAAAAA00U,
    0xBBBBBB00U,0xBBBBBB00U,

    0x8888CC00U,0x8888CC00U,
    0x9999DD00U,0x9999DD00U,
    0xAAAAEE00U,0xAAAAEE00U,
    0xBBBBFF00U,0xBBBBFF00U,

    0xCC888800U,0xCC888800U,
    0xDD999900U,0xDD999900U,
    0xEEAAAA00U,0xEEAAAA00U,
    0xFFBBBB00U,0xFFBBBB00U,

    0xCC88CC00U,0xCC88CC00U,
    0xDD99DD00U,0xDD99DD00U,
    0xEEAAEE00U,0xEEAAEE00U,
    0xFFBBFF00U,0xFFBBFF00U,

    0x88CC0000U,0x88CC0000U,
    0x99DD1100U,0x99DD1100U,
    0xAAEE2200U,0xAAEE2200U,
    0xBBFF3300U,0xBBFF3300U,

    0x88CC4400U,0x88CC4400U,
    0x99DD5500U,0x99DD5500U,
    0xAAEE6600U,0xAAEE6600U,
    0xBBFF7700U,0xBBFF7700U,

    0xCCCC0000U,0xCCCC0000U,
    0xDDDD1100U,0xDDDD1100U,
    0xEEEE2200U,0xEEEE2200U,
    0xFFFF3300U,0xFFFF3300U,

    0xCCCC4400U,0xCCCC4400U,
    0xDDDD5500U,0xDDDD5500U,
    0xEEEE6600U,0xEEEE6600U,
    0xFFFF7700U,0xFFFF7700U,

    0x88CC8800U,0x88CC8800U,
    0x99DD9900U,0x99DD9900U,
    0xAAEEAA00U,0xAAEEAA00U,
    0xBBFFBB00U,0xBBFFBB00U,

    0x88CCCC00U,0x88CCCC00U,
    0x99DDDD00U,0x99DDDD00U,
    0xAAEEEE00U,0xAAEEEE00U,
    0xBBFFFF00U,0xBBFFFF00U,

    0xCCCC8800U,0xCCCC8800U,
    0xDDDD9900U,0xDDDD9900U,
    0xEEEEAA00U,0xEEEEAA00U,
    0xFFFFBB00U,0xFFFFBB00U,

    0xCCCCCC00U,0xCCCCCC00U,
    0xDDDDDD00U,0xDDDDDD00U,
    0xEEEEEE00U,0xEEEEEE00U,
    0xFFFFFF00U,0xFFFFFF00U
};

static U32 desktop_greyed_palette_256[256*2];

static void
host_disable_bbggrrxx(
    _InoutRef_  P_U32 p_bbggrrxx)
{
    static const RGB rgb_d = RGB_INIT(221, 221, 221);
    U32 bbggrrxx = *p_bbggrrxx;
    RGB rgb;

    rgb_set(&rgb,
        (bbggrrxx >>  8) & 0xFF,
        (bbggrrxx >> 16) & 0xFF,
        (bbggrrxx >> 24) & 0xFF);

    host_disable_rgb(&rgb, &rgb_d, 3);

    bbggrrxx =
        (rgb.b << 24) |
        (rgb.g << 16) |
        (rgb.r <<  8) ;

  /*reportf("%.08X -> %.08X", *p_bbggrrxx, bbggrrxx);*/

    *p_bbggrrxx = bbggrrxx;
}

static void
generate_greyed_tool_palette(void)
{
    U32 i;
    U32 bbggrrxx;

#if 0
    for(i = 0; i < 16; ++i)
    {
        const U32 p_idx = i << 1;

        bbggrrxx = desktop_default_palette_16[p_idx];

        host_disable_bbggrrxx(&bbggrrxx);

        desktop_greyed_palette_16[p_idx + 0] = bbggrrxx;
        desktop_greyed_palette_16[p_idx + 1] = bbggrrxx; /* second is just a copy of first */
    }
#endif

    for(i = 0; i < 256; ++i)
    {
        const U32 p_idx = i << 1;

        bbggrrxx = desktop_default_palette_256[p_idx];

        host_disable_bbggrrxx(&bbggrrxx);

        desktop_greyed_palette_256[p_idx + 0] = bbggrrxx;
        desktop_greyed_palette_256[p_idx + 1] = bbggrrxx; /* second is just a copy of first */
    }
}

static void
do_paint_tool(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InRef_     PC_T5_TOOLBAR_VIEW_TOOL_DESC p_t5_toolbar_view_tool_desc,
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context)
{
    PIXIT_RECT tool_rect = p_t5_toolbar_view_tool_desc->pixit_rect;
    const P_T5_TOOLBAR_DOCU_TOOL_DESC p_t5_toolbar_docu_tool_desc = array_ptr(&p_docu->h_toolbar, T5_TOOLBAR_DOCU_TOOL_DESC, p_t5_toolbar_view_tool_desc->docu_tool_index);
    const PC_T5_TOOLBAR_TOOL_DESC p_t5_toolbar_tool_desc = p_t5_toolbar_docu_tool_desc->p_t5_toolbar_tool_desc;
    BOOL enabled = tool_enabled_query(p_t5_toolbar_docu_tool_desc);

    PC_SBSTR resource_id = p_t5_toolbar_tool_desc->resource_id;
    GDI_BOX control_screen_outer;
    GDI_BOX control_screen_inner;
    int b = g_Win31 ? FRAMED_BOX_W31_BUTTON_OUT : FRAMED_BOX_BUTTON_OUT;
    BOOL needs_depression = 0;

    {
    GDI_RECT gdi_rect;
    status_consume(gdi_rect_from_pixit_rect_and_context(&gdi_rect, &tool_rect, p_redraw_context));
    control_screen_outer.x0 = gdi_rect.tl.x;
    control_screen_outer.y0 = gdi_rect.br.y;
    control_screen_outer.x1 = gdi_rect.br.x;
    control_screen_outer.y1 = gdi_rect.tl.y;
    } /*block*/

    switch(ENUM_UNPACK(T5_TOOLBAR_TOOL_TYPE, p_t5_toolbar_tool_desc->bits.type))
    {
    case T5_TOOLBAR_TOOL_TYPE_STATE:
        {
        switch(p_t5_toolbar_docu_tool_desc->state.state)
        {
        default:
        case FALSE: /* button is up */
            break;

        case TRUE: /* button is down */
            if(p_t5_toolbar_tool_desc->resource_id_on)
            {
                resource_id = p_t5_toolbar_tool_desc->resource_id_on;
                needs_depression = 0;
            }
            else
                needs_depression = 1;

            b = g_Win31 ? FRAMED_BOX_W31_BUTTON_IN : FRAMED_BOX_BUTTON_IN;
            break;
        }

        break;
        }

    case T5_TOOLBAR_TOOL_TYPE_USER:
        b = FRAMED_BOX_PLAIN;

        if(tool_rect.br.x >= p_view->backwindow_pixit_rect.br.x - T5_TOOLBAR_RM)
        {
            tool_rect.br.x = p_view->backwindow_pixit_rect.br.x - T5_TOOLBAR_RM;

            {
            GDI_RECT gdi_rect;
            status_consume(gdi_rect_from_pixit_rect_and_context(&gdi_rect, &tool_rect, p_redraw_context));
            control_screen_outer.x0 = gdi_rect.tl.x;
            control_screen_outer.y0 = gdi_rect.br.y;
            control_screen_outer.x1 = gdi_rect.br.x;
            control_screen_outer.y1 = gdi_rect.tl.y;
            } /*block*/
        }
        break;

    default:
        resource_id = p_t5_toolbar_tool_desc->resource_id;
        break;
    }

    /* border */
    if(!enabled)
        b |= FRAMED_BOX_DISABLED; /* NB. also needed below */
#if 0
    {
    static FONT_SPEC font_spec;
    host_paint_fonty_text_in_framed_box(p_redraw_context,
        &tool_rect, USTR_TEXT(" "), 1,
        b, 1, 7, &font_spec);
    }
#else
    host_framed_box_paint_frame(&control_screen_outer, b);

    control_screen_inner = control_screen_outer;
    host_framed_box_trim_frame(&control_screen_inner, b);

    {                                             
    RGB core_rgb;
    FRAMED_BOX_STYLE disabled     = b &  FRAMED_BOX_DISABLED;
    FRAMED_BOX_STYLE border_style = b & ~disabled;

    switch(border_style)
    {
    case FRAMED_BOX_BUTTON_IN:
    case FRAMED_BOX_W31_BUTTON_IN:
        core_rgb = rgb_stash[0x02]; /*btn grey*/
#if 0
        if(disabled)
            core_rgb = rgb_stash[0x00];
#endif
        break;

    default:
        core_rgb = rgb_stash[0x01]; /*lt grey*/
#if 0
        if(disabled)
            core_rgb = rgb_stash[0x00];
#endif
        break;
    }

    host_framed_box_paint_core(&control_screen_inner, &core_rgb);
    } /*block*/

    if(resource_id)
    {
        RESOURCE_BITMAP_ID resource_bitmap_id;
        RESOURCE_BITMAP_HANDLE resource_bitmap_handle;

        resource_bitmap_id.object_id = p_t5_toolbar_tool_desc->resource_object_id;
        resource_bitmap_id.bitmap_name = resource_id;

        resource_bitmap_handle = resource_bitmap_find(&resource_bitmap_id);

        if(0 == resource_bitmap_handle.i)
            resource_bitmap_handle = resource_bitmap_find_system(&resource_bitmap_id);

        if(0 != resource_bitmap_handle.i)
        {
            P_SCB p_scb = resource_bitmap_handle.p_scb;
            BOOL has_mask = (p_scb->offset_to_mask != p_scb->offset_to_data);
            union SCB_BUFFER_U
            {
                SCB scb;
                U8 buffer[sizeof(SCB) + 2048 + 6*256];
            } fish;

           /* all toolbar buttons with sprite contents get small margin applied inside the frame */
           control_screen_inner.x0 += 4;
           control_screen_inner.y0 += 4;
           control_screen_inner.x1 -= 4;
           control_screen_inner.y1 -= 4;

            if(!enabled || (needs_depression && !has_mask))
            {
                const U32 source_palette_bytes = p_scb->offset_to_data - sizeof32(*p_scb);
                const U32 * replacement_palette = NULL;
                U32 replacement_palette_bytes = 0;
                U32 bpp;

                host_modevar_cache_query_bpp(p_scb->mode, &bpp);

                switch(bpp)
                {
                case 8:
                    replacement_palette = enabled ? desktop_default_palette_256 : desktop_greyed_palette_256;
                    replacement_palette_bytes = sizeof(desktop_greyed_palette_256);
                    break;

                case 4:
                    replacement_palette = enabled ? desktop_default_palette_16 : desktop_greyed_palette_16;
                    replacement_palette_bytes = sizeof(desktop_greyed_palette_16);
                    break;

                default:
                    break;
                }

                if(0 != replacement_palette_bytes)
                {
                    assert(p_scb->offset_to_next + (replacement_palette_bytes - source_palette_bytes) <= sizeof32(fish.buffer));
                    if(p_scb->offset_to_next + (replacement_palette_bytes - source_palette_bytes) > sizeof32(fish.buffer))
                        replacement_palette_bytes = 0;
                }

                if(0 != replacement_palette_bytes)
                {
                    U32 * p;
                    U32 c2 = replacement_palette[(2)*2]; /* colour 2 in whichever palette */

                    /* copy header */
                    memcpy32(&fish.scb, p_scb, sizeof32(*p_scb));

                    /* insert new palette */
                    memcpy32(&fish.buffer[sizeof32(*p_scb)], replacement_palette, replacement_palette_bytes);

                    /* copy image data (and mask) */
                    memcpy32(&fish.buffer[sizeof32(*p_scb) + replacement_palette_bytes],
                             (p_scb + 1) + source_palette_bytes,
                             p_scb->offset_to_next - (sizeof32(*p_scb) + source_palette_bytes));

                    p_scb = &fish.scb;
                    p_scb->offset_to_next += (replacement_palette_bytes - source_palette_bytes);
                    p_scb->offset_to_data += (replacement_palette_bytes - source_palette_bytes);
                    assert((U32) p_scb->offset_to_data == sizeof32(*p_scb) + replacement_palette_bytes);
                    p_scb->offset_to_mask += (replacement_palette_bytes - source_palette_bytes);

                    if(needs_depression)
                    {
                        p = (U32 *) (p_scb + 1);

                        /* simply map colour 1 to colour 2 for depressed button effect */
                        p[(1)*2] = c2;
                        p[(2)*2] = c2; /* copy 1st colour to 2nd colour */
                    }
                }
            }

            host_paint_sprite_abs(p_redraw_context, &control_screen_inner, p_scb, PAINT_SPRITE_SCALE_INTEGRAL);
        }

        resource_bitmap_lose(&resource_bitmap_handle);
    }
    else
    {
        switch(ENUM_UNPACK(T5_TOOLBAR_TOOL_TYPE, p_t5_toolbar_tool_desc->bits.type))
        {
        default:
            break;

        case T5_TOOLBAR_TOOL_TYPE_USER:
            {
            T5_TOOLBAR_TOOL_USER_REDRAW t5_toolbar_tool_user_redraw;
            RECT_FLAGS rect_flags;
            zero_struct_fn(t5_toolbar_tool_user_redraw);
            t5_toolbar_tool_user_redraw.client_handle = p_t5_toolbar_view_tool_desc->client_handle;
            t5_toolbar_tool_user_redraw.p_t5_toolbar_tool_desc = p_t5_toolbar_tool_desc;
            t5_toolbar_tool_user_redraw.viewno = p_view->viewno;
            t5_toolbar_tool_user_redraw.redraw_context = *p_redraw_context;
            t5_toolbar_tool_user_redraw.control_outer_pixit_rect = tool_rect;
            {
            GDI_RECT gdi_rect;
            gdi_rect.tl.x = control_screen_inner.x0;
            gdi_rect.br.y = control_screen_inner.y0;
            gdi_rect.br.x = control_screen_inner.x1;
            gdi_rect.tl.y = control_screen_inner.y1;
            pixit_rect_from_screen_rect_and_context(&t5_toolbar_tool_user_redraw.control_inner_pixit_rect, &gdi_rect, &t5_toolbar_tool_user_redraw.redraw_context);
            } /*block*/
            t5_toolbar_tool_user_redraw.enabled = enabled;
            RECT_FLAGS_CLEAR(rect_flags);
            if(host_set_clip_rectangle(&t5_toolbar_tool_user_redraw.redraw_context, &t5_toolbar_tool_user_redraw.control_inner_pixit_rect, rect_flags))
            {
                status_consume(object_call_id(p_t5_toolbar_tool_desc->command_object_id, p_docu, T5_MSG_TOOLBAR_TOOL_USER_REDRAW, &t5_toolbar_tool_user_redraw));
                host_restore_clip_rectangle(p_redraw_context);
            }
            break;
            }
        }
    }
#endif
}

#elif WINDOWS

static void
do_paint_tool(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InRef_     PC_T5_TOOLBAR_VIEW_TOOL_DESC p_t5_toolbar_view_tool_desc,
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context)
{
    PIXIT_RECT tool_rect = p_t5_toolbar_view_tool_desc->pixit_rect;
    const P_T5_TOOLBAR_DOCU_TOOL_DESC p_t5_toolbar_docu_tool_desc = array_ptr(&p_docu->h_toolbar, T5_TOOLBAR_DOCU_TOOL_DESC, p_t5_toolbar_view_tool_desc->docu_tool_index);
    const PC_T5_TOOLBAR_TOOL_DESC p_t5_toolbar_tool_desc = p_t5_toolbar_docu_tool_desc->p_t5_toolbar_tool_desc;
    BOOL enabled = tool_enabled_query(p_t5_toolbar_docu_tool_desc);

    UINT resource_id = p_t5_toolbar_tool_desc->resource_id;
    RECT rect;
    UINT uState;

    UNREFERENCED_PARAMETER_ViewRef_(p_view);

    (void) RECT_limited_from_pixit_rect_and_context(&rect, &tool_rect, p_redraw_context);

    switch(ENUM_UNPACK(T5_TOOLBAR_TOOL_TYPE, p_t5_toolbar_tool_desc->bits.type))
    {
    case T5_TOOLBAR_TOOL_TYPE_STATE:
        {
        switch(p_t5_toolbar_docu_tool_desc->state.state)
        {
        default:
        case FALSE: /* button is up */
            uState = ATTRIBUTEBUTTON_OFF;
            break;

        case TRUE: /* button is down */
            if(p_t5_toolbar_tool_desc->resource_id_on)
                resource_id = p_t5_toolbar_tool_desc->resource_id_on;

            uState = ATTRIBUTEBUTTON_ON;
            break;

        case INDETERMINATE:
            uState = ATTRIBUTEBUTTON_INDETERMINATE;
            break;
        }

        break;
        }

    default:
        resource_id = p_t5_toolbar_tool_desc->resource_id;
        uState = COMMANDBUTTON_NORMAL;
        break;
    }

    if(!enabled)
    {
        uState |= BUTTONGROUP_DISABLED;
    }
    else if(2 == p_t5_toolbar_view_tool_desc->button_down)
    {
        uState |= BUTTONGROUP_MOUSEDOWN;
    }
    else if(1 == p_t5_toolbar_view_tool_desc->button_down)
    {
        /*if(0 == (uState & ATTRIBUTEBUTTON_ON))*/
        uState |= BUTTONGROUP_MOUSEOVER;
    }

    if(resource_id)
    {
        BOOL f32bpp = FALSE;
        BOOL bitmap_found;
        RESOURCE_BITMAP_ID resource_bitmap_id;
        RESOURCE_BITMAP_HANDLE resource_bitmap_handle;
        GDI_SIZE bm_grid_size;
        UINT index;

        resource_bitmap_id.object_id = p_t5_toolbar_tool_desc->resource_object_id;
        resource_bitmap_id.bitmap_id = resource_id;

        if(g_hTheme && (g_nColours > 256)) /* still prefer 4-bit icons for 8-bit modes even if themed */
        if(resource_bitmap_id.bitmap_id & T5_RESOURCE_COMMON_BMP_BIT)
        {   /* Promote to using 32-bpp low/high-dpi icon set */
            /* NB this only applies to multi-image stores, which the toolbar buttons currently are */
            resource_bitmap_id.bitmap_id += 2;
            f32bpp = TRUE;
        }

        bitmap_found = resource_bitmap_find_new(&resource_bitmap_id, &resource_bitmap_handle, &bm_grid_size, &index);

        if(!bitmap_found)
        {   /* start again from a blank button */
            uState = BUTTONGROUP_BLANK;

            if(!enabled)
            {
                uState |= BUTTONGROUP_DISABLED;
            }
            else if(2 == p_t5_toolbar_view_tool_desc->button_down)
            {
                uState |= BUTTONGROUP_MOUSEDOWN;
            }
            else if(1 == p_t5_toolbar_view_tool_desc->button_down)
            {
                uState |= BUTTONGROUP_MOUSEOVER;
            }
        }

        (void) toolbar_UIToolButtonDrawTDD(g_hTheme, f32bpp, p_redraw_context->windows.paintstruct.hdc, &rect, resource_bitmap_handle.i, bm_grid_size.cx, bm_grid_size.cy, index, uState, &tdd);

        resource_bitmap_lose(&resource_bitmap_handle);
    }
    /* else ... User controls on Windows must be separate windows */
}

#endif /* OS */

static void
paint_tool(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InRef_     PC_T5_TOOLBAR_VIEW_TOOL_DESC p_t5_toolbar_view_tool_desc,
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_RECT p_pixit_rect_clip)
{
    PIXIT_RECT intersection_rect;

    if(intersect_pixit_rect(&intersection_rect, &p_t5_toolbar_view_tool_desc->pixit_rect, p_pixit_rect_clip))
    {
#if WINDOWS
        /* used to be able to omit this given rectangular buttons but Themes complicate matters */
        if(p_redraw_context->windows.paintstruct.fErase)
            if(g_hTheme /*&& IsThemeBackgroundPartiallyTransparent(g_hTheme, TP_BUTTON, TS_NORMAL)*/)
                fill_pixit_rect(p_redraw_context, &intersection_rect, toolbar_hBrush);
#endif

        do_paint_tool(p_docu, p_view, p_t5_toolbar_view_tool_desc, p_redraw_context);
    }

#if WINDOWS
    if(0 != p_t5_toolbar_view_tool_desc->rhs_extra_div_20)
        do_paint_separator(p_t5_toolbar_view_tool_desc, p_redraw_context);
#endif
}

extern void
redraw_tool(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InRef_     PC_T5_TOOLBAR_VIEW_TOOL_DESC p_t5_toolbar_view_tool_desc)
{

#if RISCOS

    SKEL_RECT skel_rect;
    RECT_FLAGS rect_flags;
    REDRAW_FLAGS redraw_flags;

    skel_rect.tl.pixit_point = p_t5_toolbar_view_tool_desc->pixit_rect.tl;
    skel_rect.tl.page_num.x = 0;
    skel_rect.tl.page_num.y = 0;
    skel_rect.br.pixit_point = p_t5_toolbar_view_tool_desc->pixit_rect.br;
    skel_rect.br.page_num.x = 0;
    skel_rect.br.page_num.y = 0;

    RECT_FLAGS_CLEAR(rect_flags);
    REDRAW_FLAGS_CLEAR(redraw_flags);
    view_update_now_single(p_docu, p_view, UPDATE_BACK_WINDOW, &skel_rect, rect_flags, redraw_flags, LAYER_TOOLBAR);

#elif WINDOWS

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    if(HOST_WND_NONE != p_view->main[WIN_TOOLBAR].hwnd)
    {
        GDI_RECT gdi_rect;
        RECT rect;
        status_consume(window_rect_from_pixit_rect(&gdi_rect, &p_t5_toolbar_view_tool_desc->pixit_rect, &p_view->host_xform[XFORM_TOOLBAR]));
        rect.left   = MAX(T5_GDI_MIN_X, gdi_rect.tl.x);
        rect.top    = MAX(T5_GDI_MIN_Y, gdi_rect.tl.y);
        rect.right  = MIN(T5_GDI_MAX_X, gdi_rect.br.x);
        rect.bottom = MIN(T5_GDI_MAX_Y, gdi_rect.br.y);

        if((rect.left < rect.right) && (rect.top < rect.bottom))
            InvalidateRect(p_view->main[WIN_TOOLBAR].hwnd, &rect, TRUE);
    }

#endif /* OS */

}

static STYLE_SELECTOR _status_line_style_selector; /* const after T5_MSG_IC__STARTUP */

_Check_return_
_Ret_valid_
static inline PC_STYLE_SELECTOR
p_status_line_style_selector(void)
{
    return(&_status_line_style_selector);
}

static void
status_line_style_startup(void)
{
    style_selector_copy(&_status_line_style_selector, &style_selector_font_spec);
    style_selector_bit_set(&_status_line_style_selector, STYLE_SW_PS_RGB_BACK);
}

static BOOL  recache_status_line_style = TRUE;

static STYLE _status_line_style;

_Check_return_
_Ret_valid_
static PC_STYLE
status_line_style_cache_do(
    _DocuRef_   PC_DOCU p_docu_config,
    _OutRef_    P_STYLE p_style_out)
{
    const PC_STYLE_SELECTOR p_desired_style_selector = p_status_line_style_selector();
    STYLE_HANDLE style_handle;
    STYLE style;

    CHECKING_ONLY(zero_struct(style)); /* sure makes it easier to debug! */
    style_init(&style);

    if(0 != (style_handle = style_handle_from_name(p_docu_config, STYLE_NAME_UI_STATUS_LINE)))
        style_struct_from_handle(p_docu_config, &style, style_handle, p_desired_style_selector);

    if(0 != style_selector_compare(p_desired_style_selector, &style.selector))
    {   /* fill in anything missing from the base UI style defaults */
        STYLE_SELECTOR style_selector_ensure;

        void_style_selector_bic(&style_selector_ensure, p_desired_style_selector, &style.selector);

        style_copy_defaults(p_docu_config, &style, &style_selector_ensure);
    }

    /* all callers need fully populated font_spec */
    font_spec_from_ui_style(&style.font_spec, &style);

    *p_style_out = style; /* style is now populated with everything we need */

    return(p_style_out);
}

_Check_return_
_Ret_valid_
static PC_STYLE
status_line_style_cache(
    _InoutRef_  P_STYLE p_style_ensured)
{
    if(!recache_status_line_style)
        return(p_style_ensured);

    recache_status_line_style = FALSE;

    return(status_line_style_cache_do(p_docu_from_config(), p_style_ensured));
}

_Check_return_
_Ret_valid_
static inline PC_STYLE
p_style_for_status_line(void)
{
    return(status_line_style_cache(&_status_line_style));
}

#if RISCOS

_Check_return_
static PIXIT
status_line_font_ascent(
    _In_        HOST_FONT host_font,
    _OutRef_    P_PIXIT p_descent)
{   /* NB go via OS units not millipoints */
    PIXIT ascent = host_font_ascent(host_font, (int) UCH_LATIN_CAPITAL_LETTER_A_WITH_CIRCUMFLEX); /* a tall Latin-1 character */
    PIXIT descent = host_font_descent(host_font, (int) 'y'); /* and one with a descender */

    *p_descent = descent;

    return(ascent);
}

#elif WINDOWS

_Check_return_
static inline COLORREF
simple_colorref_from_rgb(
    _InRef_     PC_RGB p_rgb)
{
    return(RGB(p_rgb->r, p_rgb->g, p_rgb->b));
}

#endif /* OS */

_Check_return_
_Ret_valid_
static inline PC_RGB
colour_of_status_line_background(
    _InRef_     PC_STYLE p_status_line_style)
{
    return(
        style_selector_bit_test(&p_status_line_style->selector, STYLE_SW_PS_RGB_BACK)
            ? &p_status_line_style->para_style.rgb_back
            : &rgb_stash[1]);
}

#if RISCOS

static void
paint_status_line_in_toolbar_riscos(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_RECT p_redraw_clip_pixit_rect)
{
    PIXIT_RECT status_line_rect = p_view->toolbar.status_line_pixit_rect;
    PIXIT_RECT clip_rect;

    if(intersect_pixit_rect(&clip_rect, &status_line_rect, p_redraw_clip_pixit_rect))
    {
        const PC_STYLE p_status_line_style = p_style_for_status_line();
        PC_RGB p_rgb_back = colour_of_status_line_background(p_status_line_style);
        PIXIT t_y = status_line_rect.tl.y + p_redraw_context->one_real_pixel.y;
        PIXIT b_y = status_line_rect.br.y - p_redraw_context->one_real_pixel.y;
        PIXIT_RECT pixit_rect;

        /* top line */
        pixit_rect = status_line_rect;
        pixit_rect.br.y = t_y;
        host_paint_rectangle_filled(p_redraw_context, &pixit_rect, &rgb_stash[4]);

        /* bottom line */
        pixit_rect = status_line_rect;
        pixit_rect.tl.y = b_y;
        host_paint_rectangle_filled(p_redraw_context, &pixit_rect, &rgb_stash[2]);

        /* core */
        pixit_rect = status_line_rect;
        pixit_rect.tl.y = t_y;
        pixit_rect.br.y = b_y;
        host_paint_rectangle_filled(p_redraw_context, &pixit_rect, p_rgb_back);

        if(!ui_text_is_blank(&p_docu->status_line_ui_text))
        {
            PC_USTR ustr = ui_text_ustr(&p_docu->status_line_ui_text);
            const U32 uchars_n = ustrlen32(ustr);

            pixit_rect = status_line_rect;
            pixit_rect.tl.x += T5_TOOLBAR_LM;

            pixit_rect.tl.y += 2 * p_redraw_context->one_real_pixel.y;
            pixit_rect.br.y -= 2 * p_redraw_context->one_real_pixel.y;

            {
            HOST_FONT host_font_redraw;
            HOST_FONT_SPEC host_font_spec;
            PIXIT height = pixit_rect.br.y - pixit_rect.tl.y;
            PIXIT base_line = idiv_ceil_u((height * 3), 4);
            status_assert(fontmap_host_font_spec_from_font_spec(&host_font_spec, &p_status_line_style->font_spec, FALSE));
            host_font_redraw = host_font_find(&host_font_spec, p_redraw_context);
            host_font_spec_dispose(&host_font_spec);
            pixit_rect.tl.y += base_line;
            host_fonty_text_paint_uchars_simple(p_redraw_context, &pixit_rect.tl, ustr, uchars_n, &p_status_line_style->font_spec.colour, p_rgb_back, host_font_redraw, TA_LEFT);
            host_font_dispose(&host_font_redraw, p_redraw_context);
            } /*block*/
        }
    }
}

/* TOOLBAR_ERASE_BACKGROUND is not defined - we let RISC OS paint the background in whatever style is current */

T5_MSG_PROTO(static, paint_toolbar_riscos, _InoutRef_ P_SKELEVENT_REDRAW p_skelevent_redraw)
{
    const P_VIEW p_view = p_skelevent_redraw->redraw_context.p_view;
    const PC_REDRAW_CONTEXT p_redraw_context = &p_skelevent_redraw->redraw_context;
    PIXIT_RECT redraw_clip_pixit_rect;
    PIXIT_RECT bg_rect;
    BOOL paint_toolbar = !global_preferences.disable_toolbar;
    BOOL paint_status_line = !global_preferences.disable_status_line;
    BOOL first_time = TRUE;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    redraw_clip_pixit_rect.tl = p_skelevent_redraw->clip_skel_rect.tl.pixit_point;
    redraw_clip_pixit_rect.br = p_skelevent_redraw->clip_skel_rect.br.pixit_point;

    bg_rect.tl.x = 0;
    bg_rect.tl.y = 0;
    bg_rect.br.x = 16384 * PIXITS_PER_RISCOS;

    if(paint_toolbar)
    {
        const ARRAY_INDEX view_rows = array_elements(&p_view->toolbar.h_toolbar_view_row_desc);
        ARRAY_INDEX view_row_idx = view_rows;

        bg_rect.br.y = T5_TOOLBAR_TM;

        while(--view_row_idx >= 0) /* going down rows ... */
        {
            const PC_T5_TOOLBAR_VIEW_ROW_DESC p_t5_toolbar_view_row_desc = array_ptrc(&p_view->toolbar.h_toolbar_view_row_desc, T5_TOOLBAR_VIEW_ROW_DESC, view_row_idx);

            if(0 == p_t5_toolbar_view_row_desc->height)
                continue;

            if(first_time)
                first_time = FALSE;
            else
                bg_rect.br.y += T5_TOOLBAR_ROW_SEP_V;

#if defined(TOOLBAR_ERASE_BACKGROUND)
            /* erase rows of pixels above controls */
            if(intersect_pixit_rect(&clip_rect, &bg_rect, &redraw_clip_pixit_rect))
                host_paint_rectangle_filled(p_redraw_context, &clip_rect, &rgb_stash[1]);
#endif

            /* step down to row of controls */
            bg_rect.tl.y = bg_rect.br.y;
            bg_rect.br.y = bg_rect.tl.y + p_t5_toolbar_view_row_desc->height;

#if defined(TOOLBAR_ERASE_BACKGROUND)
            { /* erase space to the left of controls in this row */
            const P_T5_TOOLBAR_VIEW_TOOL_DESC p_t5_toolbar_view_tool_desc = array_ptr(&p_t5_toolbar_view_row_desc->h_toolbar_view_tool_desc, T5_TOOLBAR_VIEW_TOOL_DESC, 0);
            PIXIT_RECT test_rect = bg_rect;
            test_rect.br.x = p_t5_toolbar_view_tool_desc->pixit_rect.left;
            if((test_rect.tl.x < test_rect.br.x) && intersect_pixit_rect(&clip_rect, &test_rect, &redraw_clip_pixit_rect))
                host_paint_rectangle_filled(p_redraw_context, &clip_rect, &rgb_stash[1]);
            } /*block*/
#endif

            { /* left-to-right please */
            const ARRAY_INDEX row_tools = array_elements(&p_t5_toolbar_view_row_desc->h_toolbar_view_tool_desc);
            ARRAY_INDEX row_tool_idx;
            P_T5_TOOLBAR_VIEW_TOOL_DESC p_t5_toolbar_view_tool_desc = array_range(&p_t5_toolbar_view_row_desc->h_toolbar_view_tool_desc, T5_TOOLBAR_VIEW_TOOL_DESC, 0, row_tools);

            for(row_tool_idx = 0; row_tool_idx < row_tools; ++row_tool_idx, ++p_t5_toolbar_view_tool_desc)
            {
                paint_tool(p_docu, p_view, p_t5_toolbar_view_tool_desc, p_redraw_context, &redraw_clip_pixit_rect);
            }
            } /*block*/

#if defined(TOOLBAR_ERASE_BACKGROUND)
            { /* erase space to the right of controls in this row */
            const ARRAY_INDEX row_tools = array_elements(&p_t5_toolbar_view_row_desc->h_toolbar_view_tool_desc);
            const P_T5_TOOLBAR_VIEW_TOOL_DESC p_t5_toolbar_view_tool_desc = array_ptr(&p_t5_toolbar_view_row_desc->h_toolbar_view_tool_desc, T5_TOOLBAR_VIEW_TOOL_DESC, row_tools - 1);
            PIXIT_RECT test_rect = bg_rect;
            test_rect.tl.y = p_t5_toolbar_view_tool_desc->pixit_rect.right;
            if((test_rect.tl.y < test_rect.br.x) && intersect_pixit_rect(&clip_rect, &test_rect, &redraw_clip_pixit_rect))
                host_paint_rectangle_filled(p_redraw_context, &clip_rect, &rgb_stash[1]);
            } /*block*/
#endif

            /* step down to next row */
            bg_rect.tl.y = bg_rect.br.y;
        }
    }

    if(paint_status_line)
        paint_status_line_in_toolbar_riscos(p_docu, p_view, p_redraw_context, &redraw_clip_pixit_rect);

#if defined(TOOLBAR_ERASE_BACKGROUND)
    bg_rect.br.y = bg_rect.tl.y + T5_TOOLBAR_BM;

    /* erase rows of pixels below controls, or status line if present */
    if(intersect_pixit_rect(&clip_rect, &bg_rect, &redraw_clip_pixit_rect))
        host_paint_rectangle_filled(p_redraw_context, &clip_rect, &rgb_stash[1]);
#endif

    return(STATUS_OK);
}

#endif /* RISCOS */

#if WINDOWS

static int sl_select = 1;

extern void
paint_status_line_windows(
    _HwndRef_   HWND hwnd,
    _InRef_     PPAINTSTRUCT p_paintstruct,
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view)
{
    const HDC hdc = p_paintstruct->hdc;
    REDRAW_CONTEXT_CACHE redraw_context_cache;
    REDRAW_CONTEXT redraw_context;
    const P_REDRAW_CONTEXT p_redraw_context = &redraw_context;
    const BOOL upper_status_line = (hwnd == p_view->main[WIN_UPPER_STATUS_LINE].hwnd);
    const PC_STYLE p_status_line_style = p_style_for_status_line();
    HBRUSH h_brush_fill = CreateSolidBrush(simple_colorref_from_rgb(colour_of_status_line_background(p_status_line_style)));
    RECT client_rect;
    RECT core_rect;
    int core_height;

    zero_struct_ptr_fn(p_redraw_context);

    zero_struct_fn(redraw_context_cache);
    p_redraw_context->p_redraw_context_cache = &redraw_context_cache;

    p_redraw_context->p_docu = p_docu;
    p_redraw_context->p_view = p_view;

    p_redraw_context->windows.paintstruct = *p_paintstruct;

    host_redraw_context_set_host_xform(p_redraw_context, &p_view->host_xform[XFORM_TOOLBAR]);

    host_redraw_context_fillin(p_redraw_context);

    host_paint_start(p_redraw_context);

    GetClientRect(hwnd, &client_rect);

    /* rect to be painted is paintstruct.rcPaint */
    core_rect = client_rect;

    if(upper_status_line)
    {
        RECT rect;

        /* top line */
        rect = client_rect;
        rect.bottom = rect.top + 1;
        FillRect(hdc, &rect, GetStockBrush(GRAY_BRUSH));
        core_rect.top = rect.bottom;

        /* bottom line */
        rect = client_rect;
        /*rect.top = rect.bottom - 1;*/
        /*FillRect(hdc, &core_rect, h_brush_fill);*/
        /*rect.bottom = rect.top;*/
        rect.top = rect.bottom - 1;
        FillRect(hdc, &rect, GetStockBrush(LTGRAY_BRUSH));
        core_rect.bottom = rect.top;
    } /*block*/

    core_height = core_rect.bottom - core_rect.top;
    FillRect(hdc, &core_rect, h_brush_fill);

    if(!ui_text_is_blank(&p_docu->status_line_ui_text))
    {
        STATUS status;
        HOST_FONT host_font_redraw = HOST_FONT_NONE;
        RECT text_rect = core_rect;
        PCTSTR pszString = ui_text_tstr(&p_docu->status_line_ui_text);
        UINT cString = tstrlen32(pszString);

        if(status_ok(status = fonty_handle_from_font_spec(&p_status_line_style->font_spec, FALSE)))
        {
            const FONTY_HANDLE fonty_handle = (FONTY_HANDLE) status;

            host_font_redraw = fonty_host_font_from_fonty_handle_redraw(p_redraw_context, fonty_handle, FALSE);
        }

        if(HOST_FONT_NONE != host_font_redraw)
        {
            SIZE PixelsPerInch;

            /* Get current pixel size e.g. 96 or 120 */
            PixelsPerInch.cx = GetDeviceCaps(hdc, LOGPIXELSX);
         /* PixelsPerInch.cy = GetDeviceCaps(hdc, LOGPIXELSY); not used here */

            host_font_select(host_font_redraw, p_redraw_context, FALSE /*fonty-controlled*/);

            (void) SetTextAlign(hdc, TA_LEFT | TA_TOP);

            (void) SetBkMode(hdc, TRANSPARENT);

            { /* DPI-aware */ /* scale output x-position */
            GDI_COORD text_x = text_rect.left + (GDI_COORD) MulDiv(T5_TOOLBAR_LM / PIXITS_PER_PIXEL, PixelsPerInch.cx, 96);
            GDI_COORD text_y = text_rect.top /*+ 1*/;
            SIZE size;
            TCHARZ tstr_sizing[] = TEXT("y") TEXT("\xC2"); /* UCH_LATIN_CAPITAL_LETTER_A_WITH_CIRCUMFLEX - a tall Latin-1 character, and one with a descender */
            void_WrapOsBoolChecking(
                GetTextExtentPoint32(hdc, tstr_sizing, tstrlen32(tstr_sizing), &size));
            text_y = ((core_rect.bottom - core_rect.top) - size.cy) /2;
            void_WrapOsBoolChecking(
                ExtTextOut(hdc, text_x, text_y, ETO_CLIPPED, &core_rect, pszString, cString, NULL));
            } /*block*/
        }
    }

    /*if(h_brush_fill)*/
        DeleteBrush(h_brush_fill);

    host_paint_end(p_redraw_context);
}

extern void
paint_toolbar_windows(
    _HwndRef_   HWND hwnd,
    _InRef_     PPAINTSTRUCT p_paintstruct,
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view)
{
    /* toolbar redraw starts here */
    /*const HDC hdc = p_paintstruct->hdc;*/
    REDRAW_CONTEXT_CACHE redraw_context_cache;
    REDRAW_CONTEXT redraw_context;
    const P_REDRAW_CONTEXT p_redraw_context = &redraw_context;
    const ARRAY_INDEX view_rows = array_elements(&p_view->toolbar.h_toolbar_view_row_desc);
    ARRAY_INDEX view_row_idx = view_rows;
    /*SIZE PixelsPerInch;*/
    PIXIT_RECT windows_clip_pixit_rect;
    PIXIT_RECT bg_rect;
    PIXIT_RECT clip_rect;
    BOOL first_time = TRUE;

    zero_struct_ptr_fn(p_redraw_context);

    zero_struct_fn(redraw_context_cache);
    p_redraw_context->p_redraw_context_cache = &redraw_context_cache;

    p_redraw_context->p_docu = p_docu;
    p_redraw_context->p_view = p_view;

    p_redraw_context->windows.paintstruct = *p_paintstruct;

    /* rect to be painted is paintstruct.rcPaint */

    /* Get current pixel size e.g. 96 or 120 */
    /*PixelsPerInch.cx = GetDeviceCaps(hdc, LOGPIXELSX);*/
    /*PixelsPerInch.cy = GetDeviceCaps(hdc, LOGPIXELSY);*/

    /*p_redraw_context->gdi_org.x = 0;*/
    /*p_redraw_context->gdi_org.y = 0;*/

    /*p_redraw_context->pixit_origin.x = 0;*/
    /*p_redraw_context->pixit_origin.y = 0;*/

    p_redraw_context->display_mode = p_view->display_mode;

    p_redraw_context->border_width.x = p_redraw_context->border_width.y = p_docu->page_def.grid_size;

    host_redraw_context_set_host_xform(p_redraw_context, &p_view->host_xform[XFORM_TOOLBAR]);

    host_redraw_context_fillin(p_redraw_context);

    host_paint_start(p_redraw_context);

    if(g_nColours >= 256)
        g_hTheme = OpenThemeData(hwnd, VSCLASS_TOOLBAR);

    /* set this so paint_tool() can retrieve a consistent value */
    if(g_hTheme)
        toolbar_hBrush = GetThemeSysColorBrush(g_hTheme, COLOR_3DFACE);
    else
        toolbar_hBrush = GetSysColorBrush(COLOR_BTNFACE);

    {
    GDI_RECT gdi_rect;
    RECT client_rect;
    GetClientRect(hwnd, &client_rect);
    gdi_rect.tl.x = client_rect.left;
    gdi_rect.tl.y = client_rect.top;
    gdi_rect.br.x = client_rect.right;
    gdi_rect.br.y = client_rect.bottom;
    pixit_rect_from_window_rect(&bg_rect, &gdi_rect, &p_redraw_context->host_xform);
    } /*block*/

    {
    GDI_RECT gdi_rect;
    gdi_rect.tl.x = redraw_context.windows.paintstruct.rcPaint.left;
    gdi_rect.tl.y = redraw_context.windows.paintstruct.rcPaint.top;
    gdi_rect.br.x = redraw_context.windows.paintstruct.rcPaint.right;
    gdi_rect.br.y = redraw_context.windows.paintstruct.rcPaint.bottom;
    pixit_rect_from_window_rect(&windows_clip_pixit_rect, &gdi_rect, &p_redraw_context->host_xform);
    } /*block*/

    bg_rect.br.y = T5_TOOLBAR_TM;

    while(--view_row_idx >= 0) /* going down rows ... */
    {
        const PC_T5_TOOLBAR_VIEW_ROW_DESC p_t5_toolbar_view_row_desc = array_ptrc(&p_view->toolbar.h_toolbar_view_row_desc, T5_TOOLBAR_VIEW_ROW_DESC, view_row_idx);

        if(0 == p_t5_toolbar_view_row_desc->height)
            continue;

        if(first_time)
            first_time = FALSE;
        else
            bg_rect.br.y += T5_TOOLBAR_ROW_SEP_V;

        /* erase rows of pixels above controls */
        if(intersect_pixit_rect(&clip_rect, &bg_rect, &windows_clip_pixit_rect))
            fill_pixit_rect(p_redraw_context, &clip_rect, toolbar_hBrush);

        /* step down to row of controls */
        bg_rect.tl.y = bg_rect.br.y;
        bg_rect.br.y = bg_rect.tl.y + p_t5_toolbar_view_row_desc->height;

        { /* erase space to the left of controls in this row */
        const P_T5_TOOLBAR_VIEW_TOOL_DESC p_t5_toolbar_view_tool_desc = array_ptr(&p_t5_toolbar_view_row_desc->h_toolbar_view_tool_desc, T5_TOOLBAR_VIEW_TOOL_DESC, 0);
        PIXIT_RECT test_rect = bg_rect;
        test_rect.br.x = p_t5_toolbar_view_tool_desc->pixit_rect.tl.x;
        if((test_rect.tl.x < test_rect.br.x) && intersect_pixit_rect(&clip_rect, &test_rect, &windows_clip_pixit_rect))
            fill_pixit_rect(p_redraw_context, &clip_rect, toolbar_hBrush);
        } /*block*/

        { /* left-to-right please */
        const ARRAY_INDEX row_tools = array_elements(&p_t5_toolbar_view_row_desc->h_toolbar_view_tool_desc);
        ARRAY_INDEX row_tool_idx;
        P_T5_TOOLBAR_VIEW_TOOL_DESC p_t5_toolbar_view_tool_desc = array_range(&p_t5_toolbar_view_row_desc->h_toolbar_view_tool_desc, T5_TOOLBAR_VIEW_TOOL_DESC, 0, row_tools);

        for(row_tool_idx = 0; row_tool_idx < row_tools; ++row_tool_idx, ++p_t5_toolbar_view_tool_desc)
        {
            paint_tool(p_docu, p_view, p_t5_toolbar_view_tool_desc, &redraw_context, &windows_clip_pixit_rect);
        }
        } /*block*/

        { /* erase space to the right of controls in this row */
        const ARRAY_INDEX row_tools = array_elements(&p_t5_toolbar_view_row_desc->h_toolbar_view_tool_desc);
        const P_T5_TOOLBAR_VIEW_TOOL_DESC p_t5_toolbar_view_tool_desc = array_ptr(&p_t5_toolbar_view_row_desc->h_toolbar_view_tool_desc, T5_TOOLBAR_VIEW_TOOL_DESC, row_tools - 1);
        PIXIT_RECT test_rect = bg_rect;
        test_rect.tl.x = p_t5_toolbar_view_tool_desc->pixit_rect.br.x;
        if((test_rect.tl.x < test_rect.br.x) && intersect_pixit_rect(&clip_rect, &test_rect, &windows_clip_pixit_rect))
            fill_pixit_rect(p_redraw_context, &clip_rect, toolbar_hBrush);
        } /*block*/

        /* step down to next row */
        bg_rect.tl.y = bg_rect.br.y;
    }

    /* erase rows of pixels below controls, or status line if present */
    bg_rect.br.y = bg_rect.tl.y + T5_TOOLBAR_BM;
    if(intersect_pixit_rect(&clip_rect, &bg_rect, &windows_clip_pixit_rect))
        fill_pixit_rect(p_redraw_context, &clip_rect, toolbar_hBrush);

    if(g_hTheme)
    {
        CloseThemeData(g_hTheme);
        g_hTheme = NULL;
    }

    host_paint_end(p_redraw_context);
}

#endif /* WINDOWS */

_Check_return_
_Ret_maybenull_
extern P_T5_TOOLBAR_VIEW_TOOL_DESC
tool_from_point(
    _ViewRef_   P_VIEW p_view,
    _InRef_     PC_PIXIT_POINT p_pixit_point)
{
    const ARRAY_INDEX view_rows = array_elements(&p_view->toolbar.h_toolbar_view_row_desc);
    ARRAY_INDEX view_row_idx;
    PC_T5_TOOLBAR_VIEW_ROW_DESC p_t5_toolbar_view_row_desc = array_rangec(&p_view->toolbar.h_toolbar_view_row_desc, T5_TOOLBAR_VIEW_ROW_DESC, 0, view_rows);

    if(global_preferences.disable_toolbar)
        return(NULL);

#if RISCOS
    if(p_pixit_point->x >= p_view->backwindow_pixit_rect.br.x - T5_TOOLBAR_RM)
        return(NULL);
#endif

    for(view_row_idx = 0; view_row_idx < view_rows; ++view_row_idx, ++p_t5_toolbar_view_row_desc)
    {
        const ARRAY_INDEX row_tools = array_elements(&p_t5_toolbar_view_row_desc->h_toolbar_view_tool_desc);
        ARRAY_INDEX row_tool_idx;
        P_T5_TOOLBAR_VIEW_TOOL_DESC p_t5_toolbar_view_tool_desc = array_range(&p_t5_toolbar_view_row_desc->h_toolbar_view_tool_desc, T5_TOOLBAR_VIEW_TOOL_DESC, 0, row_tools);

        for(row_tool_idx = 0; row_tool_idx < row_tools; ++row_tool_idx, ++p_t5_toolbar_view_tool_desc)
        {
            if(p_pixit_point->x <  p_t5_toolbar_view_tool_desc->pixit_rect.tl.x)
                continue;
            if(p_pixit_point->x >= p_t5_toolbar_view_tool_desc->pixit_rect.br.x)
                continue;
            if(p_pixit_point->y <  p_t5_toolbar_view_tool_desc->pixit_rect.tl.y)
                continue;
            if(p_pixit_point->y >= p_t5_toolbar_view_tool_desc->pixit_rect.br.y)
                continue;

            return(p_t5_toolbar_view_tool_desc);
        }
    }

    return(NULL);
}

static void
schedule_tool_redraw_for_view(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _In_z_      PC_USTR name,
    _InVal_     S32 require_count)
{
    const U32 name_len = ustrlen32(name);
    const U32 name_hash = trivial_name_hash(name, name_len);
    const ARRAY_INDEX view_rows = array_elements(&p_view->toolbar.h_toolbar_view_row_desc);
    ARRAY_INDEX view_row_idx;
    PC_T5_TOOLBAR_VIEW_ROW_DESC p_t5_toolbar_view_row_desc = array_range(&p_view->toolbar.h_toolbar_view_row_desc, T5_TOOLBAR_VIEW_ROW_DESC, 0, view_rows);

    if(p_view->flags.view_not_yet_open)
        return; /* trivial optimisation for views being created to not go flickerama */

    for(view_row_idx = 0; view_row_idx < view_rows; ++view_row_idx, ++p_t5_toolbar_view_row_desc)
    {
        const ARRAY_INDEX row_tools = array_elements(&p_t5_toolbar_view_row_desc->h_toolbar_view_tool_desc);
        ARRAY_INDEX row_tool_idx;
        P_T5_TOOLBAR_VIEW_TOOL_DESC p_t5_toolbar_view_tool_desc = array_range(&p_t5_toolbar_view_row_desc->h_toolbar_view_tool_desc, T5_TOOLBAR_VIEW_TOOL_DESC, 0, row_tools);

        for(row_tool_idx = 0; row_tool_idx < row_tools; ++row_tool_idx, ++p_t5_toolbar_view_tool_desc)
        {
            const PC_T5_TOOLBAR_DOCU_TOOL_DESC p_t5_toolbar_docu_tool_desc = array_ptr(&p_docu->h_toolbar, T5_TOOLBAR_DOCU_TOOL_DESC, p_t5_toolbar_view_tool_desc->docu_tool_index);

            if(name_hash != p_t5_toolbar_docu_tool_desc->name_hash)
                continue;

            if(0 != memcmp32(p_t5_toolbar_docu_tool_desc->p_t5_toolbar_tool_desc->name, name, name_len))
                continue;

            p_t5_toolbar_view_tool_desc->redraw_count += require_count;

            schedule_toolbar_redraw(p_docu, p_view, view_row_idx);
            return;
        }
    }
}

static void
schedule_tool_redraw(
    _DocuRef_   P_DOCU p_docu,
    _In_z_      PC_USTR name,
    _InVal_     S32 require_count)
{
    VIEWNO viewno = VIEWNO_NONE;
    P_VIEW p_view;

    while(P_VIEW_NONE != (p_view = docu_enum_views(p_docu, &viewno)))
        schedule_tool_redraw_for_view(p_docu, p_view, name, require_count);
}

static void
schedule_toolbar_redraw(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InVal_     ARRAY_INDEX view_row_idx)
{
    const P_T5_TOOLBAR_VIEW_ROW_DESC p_t5_toolbar_view_row_desc = array_ptr(&p_view->toolbar.h_toolbar_view_row_desc, T5_TOOLBAR_VIEW_ROW_DESC, view_row_idx);

    if(p_view->flags.view_not_yet_open)
        return; /* trivial optimisation for views being created to not go flickerama */

    if(p_t5_toolbar_view_row_desc->scheduled_event_id)
        return;

    p_t5_toolbar_view_row_desc->scheduled_event_id = viewid_pack(p_docu, p_view, (U16) view_row_idx);

    trace_1(TRACE__SCHEDULED, TEXT("schedule_toolbar_redraw - *** scheduled_event_after(docno=") DOCNO_TFMT TEXT(", n)"), docno_from_p_docu(p_docu));
    status_assert(scheduled_event_after(docno_from_p_docu(p_docu), T5_EVENT_SCHEDULED, object_toolbar, p_t5_toolbar_view_row_desc->scheduled_event_id, MONOTIMEDIFF_VALUE_FROM_MS(100)));
}

T5_MSG_PROTO(static, scheduled_toolbar_redraw, P_SCHEDULED_EVENT_BLOCK p_scheduled_event_block)
{
    U16 u16_row;
    P_VIEW p_view;
    const PC_DOCU p_docu_for_unpack = viewid_unpack((U32) p_scheduled_event_block->client_handle, &p_view, &u16_row);

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    assert(p_docu_for_unpack == p_docu);
    UNREFERENCED_PARAMETER_CONST(p_docu_for_unpack);

    /* check for this on any of the toolbar events - it's of much greater priority */
    if(p_view->toolbar.nobble_scheduled_event_id)
    {
        /* are we using somebody else's event? If so, we'd better cancel our own! */
        if(u16_row != 0xFFFFU)
            scheduled_event_remove(docno_from_p_docu(p_docu), T5_EVENT_SCHEDULED, object_toolbar, p_view->toolbar.nobble_scheduled_event_id);

        p_view->toolbar.nobble_scheduled_event_id = 0;

        if(status_done(toolbar_view_new(p_docu, p_view, FALSE)))
            return(STATUS_OK);

        if(u16_row == 0xFFFFU)
            return(STATUS_OK);
    }

    if(u16_row < (U16) array_elements32(&p_view->toolbar.h_toolbar_view_row_desc)) /* note that row here is unsigned */
    {
        const P_T5_TOOLBAR_VIEW_ROW_DESC p_t5_toolbar_view_row_desc = array_ptr(&p_view->toolbar.h_toolbar_view_row_desc, T5_TOOLBAR_VIEW_ROW_DESC, u16_row);

        myassert3x(p_t5_toolbar_view_row_desc->scheduled_event_id == p_scheduled_event_block->client_handle,
            TEXT("row ") U32_TFMT TEXT(" scheduled_event_id ") UINTPTR_XTFMT TEXT(" != client_handle ") UINTPTR_XTFMT,
            (U32) u16_row, (uintptr_t) p_t5_toolbar_view_row_desc->scheduled_event_id, p_scheduled_event_block->client_handle);

        p_t5_toolbar_view_row_desc->scheduled_event_id = 0;

        { /* find tools on this row that need redraw */
        const ARRAY_INDEX row_tools = array_elements(&p_t5_toolbar_view_row_desc->h_toolbar_view_tool_desc);
        ARRAY_INDEX row_tool_idx;
        P_T5_TOOLBAR_VIEW_TOOL_DESC p_t5_toolbar_view_tool_desc = array_range(&p_t5_toolbar_view_row_desc->h_toolbar_view_tool_desc, T5_TOOLBAR_VIEW_TOOL_DESC, 0, row_tools);

        for(row_tool_idx = 0; row_tool_idx < row_tools; ++row_tool_idx, ++p_t5_toolbar_view_tool_desc)
        {
            if(0 == p_t5_toolbar_view_tool_desc->redraw_count)
                continue;

            p_t5_toolbar_view_tool_desc->redraw_count = 0;

            redraw_tool(p_docu, p_view, p_t5_toolbar_view_tool_desc);
        }
        } /*block*/
    }

    return(STATUS_OK);
}

T5_MSG_PROTO(static, msg_toolbar_tool_set, _InRef_ PC_T5_TOOLBAR_TOOL_SET p_t5_toolbar_tool_set)
{
    const P_T5_TOOLBAR_DOCU_TOOL_DESC p_t5_toolbar_docu_tool_desc = find_docu_tool(p_docu, p_t5_toolbar_tool_set->name);
    BOOL same = TRUE;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(NULL == p_t5_toolbar_docu_tool_desc)
        return(create_error(TOOLB_ERR_UNKNOWN_CONTROL));

    switch(ENUM_UNPACK(T5_TOOLBAR_TOOL_TYPE, p_t5_toolbar_docu_tool_desc->p_t5_toolbar_tool_desc->bits.type))
    {
    default:
        break;

    case T5_TOOLBAR_TOOL_TYPE_STATE:
        same = (p_t5_toolbar_docu_tool_desc->state.state == p_t5_toolbar_tool_set->state.state);
        if(!same)
            p_t5_toolbar_docu_tool_desc->state.state = p_t5_toolbar_tool_set->state.state;
        break;
    }

    if(!same)
        schedule_tool_redraw(p_docu, p_t5_toolbar_tool_set->name, 0x04000000);

    return(STATUS_OK);
}

#if RISCOS

static void
status_line_view_update(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view)
{
    SKEL_RECT skel_rect;
    RECT_FLAGS rect_flags;
    REDRAW_FLAGS redraw_flags;

    skel_rect.tl.pixit_point = p_view->toolbar.status_line_pixit_rect.tl;
    skel_rect.tl.page_num.x = 0;
    skel_rect.tl.page_num.y = 0;
    skel_rect.br.pixit_point = p_view->toolbar.status_line_pixit_rect.br;
    skel_rect.br.page_num.x = 0;
    skel_rect.br.page_num.y = 0;

    /* try to avoid repainting the top and bottom lines */
    skel_rect.tl.pixit_point.y += (PIXITS_PER_RISCOS) << host_modevar_cache_current.YEigFactor;
    skel_rect.br.pixit_point.y -= (PIXITS_PER_RISCOS) << host_modevar_cache_current.YEigFactor;

    RECT_FLAGS_CLEAR(rect_flags);
    REDRAW_FLAGS_CLEAR(redraw_flags);
    view_update_now_single(p_docu, p_view, UPDATE_BACK_WINDOW, &skel_rect, rect_flags, redraw_flags, LAYER_TOOLBAR);
}

#elif WINDOWS

static void
status_line_view_update(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view)
{
    HOST_WND hwnd;
    RECT rect;

    /* try to avoid repainting the top and bottom lines */
    rect.top = 1;
    rect.top = 1;

    if(HOST_WND_NONE != (hwnd = p_view->main[WIN_UPPER_STATUS_LINE].hwnd))
    {
    }
    else if(HOST_WND_NONE != (hwnd = p_view->main[WIN_LOWER_STATUS_LINE].hwnd))
    {
    }
    else
        return;

    GetClientRect(hwnd, &rect);

    /* try to avoid repainting the top and bottom lines */
    rect.top += 1;
    rect.bottom -= 1;

    InvalidateRect(hwnd, &rect, TRUE);

    if(p_view->viewno == p_docu->viewno_caret)
        UpdateWindow(hwnd);
}

#endif /* OS */

T5_MSG_PROTO(static, t5_msg_status_line_set, _InRef_ PC_UI_TEXT p_ui_text)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    ui_text_dispose(&p_docu->status_line_ui_text);

    status_assert(ui_text_copy(&p_docu->status_line_ui_text, p_ui_text));

    if(!global_preferences.disable_status_line)
    {
        VIEWNO viewno = VIEWNO_NONE;
        P_VIEW p_view;

        while(P_VIEW_NONE != (p_view = docu_enum_views(p_docu, &viewno)))
        {
            status_line_view_update(p_docu, p_view);
        }
    }

    return(STATUS_OK);
}

_Check_return_
extern BOOL
tool_enabled_query(
    _InRef_     PC_T5_TOOLBAR_DOCU_TOOL_DESC p_t5_toolbar_docu_tool_desc)
{
    const U32 disable_count = bitmap_count(p_t5_toolbar_docu_tool_desc->disable_state, N_BITS_ARG(32));
    const U32 enable_count = bitmap_count(p_t5_toolbar_docu_tool_desc->enable_state, N_BITS_ARG(32));
    return(enable_count > disable_count); /* default state is disabled */
}

static void
status_line_view_dispose(
    _ViewRef_   P_VIEW p_view)
{
#if RISCOS
    UNREFERENCED_PARAMETER_ViewRef_(p_view);
#elif WINDOWS
    if(HOST_WND_NONE != p_view->main[WIN_UPPER_STATUS_LINE].hwnd)
    {
        DestroyWindow(p_view->main[WIN_UPPER_STATUS_LINE].hwnd);
        p_view->main[WIN_UPPER_STATUS_LINE].hwnd = NULL;
    }

    if(HOST_WND_NONE != p_view->main[WIN_LOWER_STATUS_LINE].hwnd)
    {
        DestroyWindow(p_view->main[WIN_LOWER_STATUS_LINE].hwnd);
        p_view->main[WIN_LOWER_STATUS_LINE].hwnd = NULL;
    }
#endif /* OS */
}

_Check_return_
static STATUS
status_line_view_new(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view)
{
    const PC_STYLE p_status_line_style = p_style_for_status_line();

#if RISCOS

    UNREFERENCED_PARAMETER_ViewRef_(p_docu);
    UNREFERENCED_PARAMETER_ViewRef_(p_view);

    status_line_font_height_pixits = 0;

    {
    HOST_FONT host_font_formatting;
    HOST_FONT_SPEC host_font_spec;
    status_assert(fontmap_host_font_spec_from_font_spec(&host_font_spec, &p_status_line_style->font_spec, FALSE));
    host_font_formatting = host_font_find(&host_font_spec, P_REDRAW_CONTEXT_NONE);
    host_font_spec_dispose(&host_font_spec);
    if(HOST_FONT_NONE != host_font_formatting)
    {
        PIXIT descent;
        PIXIT ascent = status_line_font_ascent(host_font_formatting, &descent);
        status_line_font_height_pixits = ascent + descent;
    }
    host_font_dispose(&host_font_formatting, P_REDRAW_CONTEXT_NONE);
    } /*block*/

    if(0 == status_line_font_height_pixits)
        return(STATUS_OK);

    if(global_preferences.disable_status_line)
        return(STATUS_OK);

#elif WINDOWS
    RECT client_rect;
    int nWidth, nHeight;

    GetClientRect(p_view->main[WIN_BACK].hwnd, &client_rect);

    nWidth = client_rect.right - client_rect.left;

    status_line_height_pixels = 0;

    status_line_font_height_pixels = 0;

    {
    const HWND hwnd = NULL; /* screen */
    const HDC hdc = GetDC(hwnd);
    if(NULL != hdc)
    {
        BOOL font_needs_delete;
        HFONT h_font_old;
        HOST_FONT host_font_formatting;
        HOST_FONT_SPEC host_font_spec;

        status_assert(fontmap_host_font_spec_from_font_spec(&host_font_spec, &p_status_line_style->font_spec, FALSE));

        { /* Em = dpiY * point_size / 72 from MSDN */ /* convert to dpi-dependent pixels */ /* DPI-aware */
        GDI_SIZE PixelsPerInch;
        host_get_pixel_size(NULL /*screen*/, NULL /*cx*/, &PixelsPerInch.cy); /* Get current pixel size for the screen e.g. 96 or 120 */

        host_font_spec.size_y = idiv_floor_u(host_font_spec.size_y * PixelsPerInch.cy, PIXITS_PER_INCH);
        } /*block*/

        host_font_formatting = host_font_find(&host_font_spec, P_REDRAW_CONTEXT_NONE);
        host_font_spec_dispose(&host_font_spec);
        if(HOST_FONT_NONE != host_font_formatting)
        {
            font_needs_delete = TRUE;
            h_font_old = SelectFont(hdc, host_font_formatting);
        }
        else
        {   /* SKS ensure we have some font selected in the DC */
            font_needs_delete = FALSE;
            h_font_old = SelectFont(hdc, GetStockObject(ANSI_VAR_FONT));
        }

        { /* dpi-dependent pixels */ 
        TEXTMETRIC textmetric;
        void_WrapOsBoolChecking(GetTextMetrics(hdc, &textmetric));
        trace_2(TRACE_APP_DIALOG, TEXT("GetTextMetrics() yields average width=%d, height=%d"), textmetric.tmAveCharWidth, textmetric.tmHeight);

        status_line_font_height_pixels = textmetric.tmHeight + textmetric.tmExternalLeading;
        } /*block*/

        if(font_needs_delete)
            host_font_dispose(&host_font_formatting, P_REDRAW_CONTEXT_NONE);

        void_WrapOsBoolChecking(1 == ReleaseDC(hwnd, hdc));
    }
    } /*block*/

    if(0 == status_line_font_height_pixels)
        return(STATUS_OK);
    
    /* dpi-dependent pixels */
    if(sl_select == 1)
    {   /* upper status line */
        status_line_height_pixels = status_line_font_height_pixels + 6; /* remember ideal height to open with */
    }
    else if(sl_select >= 2)
    {   /* lower status line */
        status_line_height_pixels = status_line_font_height_pixels + 6; /* remember ideal height to open with */
    }

    if(global_preferences.disable_status_line)
        return(STATUS_OK);

    if(sl_select == 1)
    {   /* upper status line */
        nHeight = status_line_height_pixels;

        {
        U32 packed_viewid = viewid_pack(p_docu, p_view, EVENT_HANDLER_STATUS_LINE);
        void_WrapOsBoolChecking(HOST_WND_NONE != (
        p_view->main[WIN_UPPER_STATUS_LINE].hwnd =
            CreateWindowEx(WS_EX_ACCEPTFILES | WS_EX_NOPARENTNOTIFY, window_class[APP_WINDOW_CLASS_STATUS_LINE], NULL,
                           WS_CHILD | WS_VISIBLE,
                           0, 0 /*needs repositioning*/,
                           nWidth, nHeight,
                           p_view->main[WIN_BACK].hwnd, NULL, GetInstanceHandle(), &packed_viewid)));
        } /*block*/
    }
    else if(sl_select >= 2)
    {   /* lower status line */
        nHeight = status_line_height_pixels;

        {
        U32 packed_viewid = viewid_pack(p_docu, p_view, EVENT_HANDLER_STATUS_LINE);
        void_WrapOsBoolChecking(HOST_WND_NONE != (
        p_view->main[WIN_LOWER_STATUS_LINE].hwnd =
            CreateWindowEx(WS_EX_ACCEPTFILES | WS_EX_NOPARENTNOTIFY, window_class[APP_WINDOW_CLASS_STATUS_LINE], NULL,
                           WS_CHILD | WS_VISIBLE,
                           0, client_rect.bottom - nHeight,
                           nWidth, nHeight,
                           p_view->main[WIN_BACK].hwnd, NULL, GetInstanceHandle(), &packed_viewid)));
        } /*block*/
    }
#endif /* OS */

    return(STATUS_OK);
}

_Check_return_
static STATUS
toolbar_candidate_range_added(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ARRAY_INDEX stt,
    _InVal_     ARRAY_INDEX end)
{
    STATUS status = STATUS_OK;

    if((end - stt) > 0)
    {
        SC_ARRAY_INIT_BLOCK array_init_block = aib_init(4, sizeof32(T5_TOOLBAR_DOCU_TOOL_DESC), TRUE);
        ARRAY_INDEX i;
        P_T5_TOOLBAR_DOCU_TOOL_DESC p_t5_toolbar_docu_tool_desc = al_array_extend_by(&p_docu->h_toolbar, T5_TOOLBAR_DOCU_TOOL_DESC, (end - stt), &array_init_block, &status);

        if(NULL == p_t5_toolbar_docu_tool_desc)
            return(status);

        for(i = stt; i < end; ++i, ++p_t5_toolbar_docu_tool_desc)
        {
            const PC_T5_TOOLBAR_TOOL_DESC p_t5_toolbar_tool_desc = * array_ptrc(&toolbar_candidates_array_handle, T5_TOOLBAR_TOOL_DESC *, i);

            p_t5_toolbar_docu_tool_desc->p_t5_toolbar_tool_desc = p_t5_toolbar_tool_desc;

            /* store hash to save derefs during lookup */
            p_t5_toolbar_docu_tool_desc->name_hash = trivial_name_hash(p_t5_toolbar_tool_desc->name, ustrlen32(p_t5_toolbar_tool_desc->name));
        }
    }

    return(status);
}

static void
toolbar_view_dispose_toolbar_handles(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view)
{
    const ARRAY_INDEX view_rows = array_elements(&p_view->toolbar.h_toolbar_view_row_desc);
    ARRAY_INDEX view_row_idx;
    P_T5_TOOLBAR_VIEW_ROW_DESC p_t5_toolbar_view_row_desc = array_range(&p_view->toolbar.h_toolbar_view_row_desc, T5_TOOLBAR_VIEW_ROW_DESC, 0, view_rows);

    for(view_row_idx = 0; view_row_idx < view_rows; ++view_row_idx, ++p_t5_toolbar_view_row_desc)
    {
        if(p_t5_toolbar_view_row_desc->scheduled_event_id)
        {
            trace_1(TRACE__SCHEDULED, TEXT("toolbar_view_dispose_toolbar_handles - scheduled_event_remove(docno=") DOCNO_TFMT TEXT(")"), docno_from_p_docu(p_docu));
            scheduled_event_remove(docno_from_p_docu(p_docu), T5_EVENT_SCHEDULED, object_toolbar, p_t5_toolbar_view_row_desc->scheduled_event_id);
            p_t5_toolbar_view_row_desc->scheduled_event_id = 0;
        }

        al_array_dispose(&p_t5_toolbar_view_row_desc->h_toolbar_view_tool_desc);
    }

    al_array_dispose(&p_view->toolbar.h_toolbar_view_row_desc);
}

static void
toolbar_view_dispose(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view)
{
    const ARRAY_INDEX view_rows = array_elements(&p_view->toolbar.h_toolbar_view_row_desc);
    ARRAY_INDEX view_row_idx;
    P_T5_TOOLBAR_VIEW_ROW_DESC p_t5_toolbar_view_row_desc = array_range(&p_view->toolbar.h_toolbar_view_row_desc, T5_TOOLBAR_VIEW_ROW_DESC, 0, view_rows);

    for(view_row_idx = 0; view_row_idx < view_rows; ++view_row_idx, ++p_t5_toolbar_view_row_desc)
    {
        const ARRAY_INDEX row_tools = array_elements(&p_t5_toolbar_view_row_desc->h_toolbar_view_tool_desc);
        ARRAY_INDEX row_tool_idx;
        P_T5_TOOLBAR_VIEW_TOOL_DESC p_t5_toolbar_view_tool_desc = array_range(&p_t5_toolbar_view_row_desc->h_toolbar_view_tool_desc, T5_TOOLBAR_VIEW_TOOL_DESC, 0, row_tools);

        for(row_tool_idx = 0; row_tool_idx < row_tools; ++row_tool_idx, ++p_t5_toolbar_view_tool_desc)
        {
            const P_T5_TOOLBAR_DOCU_TOOL_DESC p_t5_toolbar_docu_tool_desc = array_ptr(&p_docu->h_toolbar, T5_TOOLBAR_DOCU_TOOL_DESC, p_t5_toolbar_view_tool_desc->docu_tool_index);
            const PC_T5_TOOLBAR_TOOL_DESC p_t5_toolbar_tool_desc = p_t5_toolbar_docu_tool_desc->p_t5_toolbar_tool_desc;

            switch(ENUM_UNPACK(T5_TOOLBAR_TOOL_TYPE, p_t5_toolbar_tool_desc->bits.type))
            {
            case T5_TOOLBAR_TOOL_TYPE_USER:
                if(p_t5_toolbar_view_tool_desc->client_handle)
                {
                    T5_TOOLBAR_TOOL_USER_VIEW_DELETE t5_toolbar_tool_user_view_delete;
                    t5_toolbar_tool_user_view_delete.client_handle = p_t5_toolbar_view_tool_desc->client_handle;
                    t5_toolbar_tool_user_view_delete.p_t5_toolbar_tool_desc = p_t5_toolbar_tool_desc;
                    t5_toolbar_tool_user_view_delete.viewno = viewno_from_p_view(p_view);
                    status_assert(object_call_id(p_t5_toolbar_tool_desc->command_object_id, p_docu, T5_MSG_TOOLBAR_TOOL_USER_VIEW_DELETE, &t5_toolbar_tool_user_view_delete));
                }
                break;

            default:
                break;
            }
        }
    }

    toolbar_view_dispose_toolbar_handles(p_docu, p_view);

#if WINDOWS
    if(HOST_WND_NONE != p_view->main[WIN_TOOLBAR].hwnd)
    {
        DestroyWindow(p_view->main[WIN_TOOLBAR].hwnd);
        p_view->main[WIN_TOOLBAR].hwnd = NULL;
    }
#endif
}

_Check_return_
static STATUS
toolbar_view_new_toolbar_handles(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_ARRAY_HANDLE p_h_toolbar_view_row_desc)
{
    STATUS status = STATUS_OK;
    ARRAY_INDEX i;

    *p_h_toolbar_view_row_desc = 0; /* T5_TOOLBAR_VIEW_ROW_DESC->array_handle[] of T5_TOOLBAR_VIEW_TOOL_DESC */

    for(i = 0; i < array_elements(&toolbar_requested_array_handle); ++i)
    {
        const PC_T5_TOOLBAR_REQUESTED_TOOL_DESC p_t5_toolbar_requested_tool_desc = array_ptr(&toolbar_requested_array_handle, T5_TOOLBAR_REQUESTED_TOOL_DESC, i);
        const PC_USTR ustr_name = ustr_bptrc(p_t5_toolbar_requested_tool_desc->name);

        if(CH_NULL != PtrGetByte(ustr_name))
        {
            const P_T5_TOOLBAR_DOCU_TOOL_DESC p_t5_toolbar_docu_tool_desc = find_docu_tool(p_docu, ustr_name);
            ARRAY_INDEX docu_tool_index;

            if(NULL == p_t5_toolbar_docu_tool_desc) /* may have rubbish in choices file! */
                continue;

            if(p_t5_toolbar_docu_tool_desc->nobble_state != 0)
                continue;

            docu_tool_index = array_indexof_element(&p_docu->h_toolbar, T5_TOOLBAR_DOCU_TOOL_DESC, p_t5_toolbar_docu_tool_desc);

            status = new_view_tool(p_h_toolbar_view_row_desc, docu_tool_index, p_t5_toolbar_requested_tool_desc->row, FALSE);
        }
        else
        {
            status = new_view_tool(p_h_toolbar_view_row_desc, 0, p_t5_toolbar_requested_tool_desc->row, TRUE);
        }

        status_break(status);
    }

    if(status_fail(status))
        al_array_dispose(p_h_toolbar_view_row_desc);

    return(status);
}

_Check_return_
static STATUS
toolbar_view_new(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InVal_     BOOL for_view_new)
{
    BOOL fThemedToolbar = FALSE;

    if(for_view_new)
    {
        status_return(toolbar_view_new_toolbar_handles(p_docu, &p_view->toolbar.h_toolbar_view_row_desc));
    }
    else
    {
        ARRAY_HANDLE array_handle;
        BOOL differ;

        status_return(toolbar_view_new_toolbar_handles(p_docu, &array_handle));

        /* compare the two */
        differ = FALSE;

        for(;;)
        {
            P_ARRAY_HANDLE p_array_handle_toolbar_1 = &array_handle;
            P_ARRAY_HANDLE p_array_handle_toolbar_2 = &p_view->toolbar.h_toolbar_view_row_desc;
            ARRAY_INDEX view_row_idx;

            if(array_elements(p_array_handle_toolbar_1) != array_elements(p_array_handle_toolbar_2))
            {
                differ = TRUE;
                break;
            }

            view_row_idx = array_elements(p_array_handle_toolbar_1);

            while(--view_row_idx >= 0)
            {
                const PC_T5_TOOLBAR_VIEW_ROW_DESC p_t5_toolbar_view_row_desc_1 = array_ptrc(p_array_handle_toolbar_1, T5_TOOLBAR_VIEW_ROW_DESC, view_row_idx);
                const PC_T5_TOOLBAR_VIEW_ROW_DESC p_t5_toolbar_view_row_desc_2 = array_ptrc(p_array_handle_toolbar_2, T5_TOOLBAR_VIEW_ROW_DESC, view_row_idx);
                const PC_ARRAY_HANDLE p_array_handle_row_1 = &p_t5_toolbar_view_row_desc_1->h_toolbar_view_tool_desc;
                const PC_ARRAY_HANDLE p_array_handle_row_2 = &p_t5_toolbar_view_row_desc_2->h_toolbar_view_tool_desc;
                const ARRAY_INDEX row_tools = array_elements(p_array_handle_row_1);
                ARRAY_INDEX row_tool_idx;
                PC_T5_TOOLBAR_VIEW_TOOL_DESC p_t5_toolbar_view_tool_desc_1;
                PC_T5_TOOLBAR_VIEW_TOOL_DESC p_t5_toolbar_view_tool_desc_2;

                if(row_tools != array_elements(p_array_handle_row_2))
                {
                    differ = TRUE;
                    break;
                }

                p_t5_toolbar_view_tool_desc_1 = array_range(p_array_handle_row_1, T5_TOOLBAR_VIEW_TOOL_DESC, 0, row_tools);
                p_t5_toolbar_view_tool_desc_2 = array_range(p_array_handle_row_2, T5_TOOLBAR_VIEW_TOOL_DESC, 0, row_tools);

                for(row_tool_idx = 0; row_tool_idx < row_tools; ++row_tool_idx, ++p_t5_toolbar_view_tool_desc_1, ++p_t5_toolbar_view_tool_desc_2)
                {
                    if(p_t5_toolbar_view_tool_desc_2->docu_tool_index != p_t5_toolbar_view_tool_desc_1->docu_tool_index)
                    {
                        differ = TRUE;
                        break;
                    }
                }

                if(differ)
                    break;
            }

            break; /* out of loop for structure */
            /*NOTREACHED*/
        }

        if(!differ)
        {
            al_array_dispose(&array_handle);
            return(STATUS_OK);
        }

        toolbar_view_dispose_toolbar_handles(p_docu, p_view);

        p_view->toolbar.h_toolbar_view_row_desc = array_handle; /* make a donation */
    }

    { /* work out the required height of the toolbar given the rows and their contents */
    const BOOL has_toolbar = !global_preferences.disable_toolbar;
#if RISCOS
    const BOOL has_status_line_in_toolbar = !global_preferences.disable_status_line;
#elif WINDOWS
    const BOOL has_status_line_in_toolbar = FALSE; /* it is a separate window */
#endif
    PIXIT_POINT cur_point = { 0, 0 };
    BOOL first_time = TRUE;
    PIXIT height = 0;

#if RISCOS
    toolbar_height_riscos = 0;
#elif WINDOWS
    toolbar_height_pixels = 0;
#endif

    if(has_toolbar)
    {
        const ARRAY_INDEX view_rows = array_elements(&p_view->toolbar.h_toolbar_view_row_desc);
        ARRAY_INDEX view_row_idx = view_rows;

#if WINDOWS
        if(g_nColours >= 256)
        {   /* do we need a real hwnd? haven't got the toolbar one yet */
            HTHEME hTheme = OpenThemeData(NULL/*p_view->main[WIN_BACK].hwnd*/, VSCLASS_TOOLBAR);

            if(hTheme)
            {
                CloseThemeData(hTheme);
                fThemedToolbar = TRUE;
            }
        }
#endif

        cur_point.x = T5_TOOLBAR_LM;
        cur_point.y = T5_TOOLBAR_TM;

        while(--view_row_idx >= 0) /* going down rows ... */
        {
            const P_T5_TOOLBAR_VIEW_ROW_DESC p_t5_toolbar_view_row_desc = array_ptr(&p_view->toolbar.h_toolbar_view_row_desc, T5_TOOLBAR_VIEW_ROW_DESC, view_row_idx);
            const ARRAY_INDEX row_tools = array_elements(&p_t5_toolbar_view_row_desc->h_toolbar_view_tool_desc);
            ARRAY_INDEX row_tool_idx;
            P_T5_TOOLBAR_VIEW_TOOL_DESC p_t5_toolbar_view_tool_desc = array_range(&p_t5_toolbar_view_row_desc->h_toolbar_view_tool_desc, T5_TOOLBAR_VIEW_TOOL_DESC, 0, row_tools);
            PIXIT_POINT this_point;

            if(first_time)
                first_time = FALSE;
            else if(height)
                cur_point.y += T5_TOOLBAR_ROW_SEP_V;

            height = 0;

            this_point = cur_point;

            for(row_tool_idx = 0; row_tool_idx < row_tools; ++row_tool_idx, ++p_t5_toolbar_view_tool_desc)
            {
                const P_T5_TOOLBAR_DOCU_TOOL_DESC p_t5_toolbar_docu_tool_desc = array_ptr(&p_docu->h_toolbar, T5_TOOLBAR_DOCU_TOOL_DESC, p_t5_toolbar_view_tool_desc->docu_tool_index);
                const PC_T5_TOOLBAR_TOOL_DESC p_t5_toolbar_tool_desc = p_t5_toolbar_docu_tool_desc->p_t5_toolbar_tool_desc;
                PIXIT_SIZE this_size;

#if WINDOWS
                this_size.cx = MulDiv(tdd.cxButton, 96, tdd.uDPI) * PIXITS_PER_PIXEL;
                this_size.cy = MulDiv(tdd.cyButton, 96, tdd.uDPI) * PIXITS_PER_PIXEL;

                if(p_t5_toolbar_tool_desc->bits.thin)
                    this_size.cx /= 2;

                /* a little more internal padding from button border to the button image these days, especially now we have themed buttons */
                this_size.cx += 2 * PIXITS_PER_PIXEL;
                this_size.cy += 2 * PIXITS_PER_PIXEL;
#else
                this_size.cx = p_t5_toolbar_tool_desc->bits.thin ? TOOL_BUTTON_THIN_WIDTH : TOOL_BUTTON_STD_WIDTH;
                this_size.cy = TOOL_BUTTON_STD_HEIGHT;
#endif

                switch(ENUM_UNPACK(T5_TOOLBAR_TOOL_TYPE, p_t5_toolbar_tool_desc->bits.type))
                {
                case T5_TOOLBAR_TOOL_TYPE_USER:
                    if(p_t5_toolbar_view_tool_desc->client_handle)
                    {
                        T5_TOOLBAR_TOOL_USER_SIZE_QUERY t5_toolbar_tool_user_size_query;
                        t5_toolbar_tool_user_size_query.client_handle = p_t5_toolbar_view_tool_desc->client_handle;
                        t5_toolbar_tool_user_size_query.p_t5_toolbar_tool_desc = p_t5_toolbar_tool_desc;
                        t5_toolbar_tool_user_size_query.pixit_size =  this_size;
                        t5_toolbar_tool_user_size_query.viewno = viewno_from_p_view(p_view);
                        status_assert(object_call_id(p_t5_toolbar_tool_desc->command_object_id, p_docu, T5_MSG_TOOLBAR_TOOL_USER_SIZE_QUERY, &t5_toolbar_tool_user_size_query));
                        this_size = t5_toolbar_tool_user_size_query.pixit_size;
                    }
                    break;

#ifdef ALWAYS_UNUSED /* never implemented */
                case T5_TOOLBAR_TOOL_TYPE_DROPDOWN:
                    this_size.cx = 160; /* <<< suss eventually */
                    this_size.cy = 22; /* Windows pixels - from IDG */
                    break;
#endif

                default:
                    break;
                }

                p_t5_toolbar_view_tool_desc->pixit_rect.tl.x = this_point.x;
                p_t5_toolbar_view_tool_desc->pixit_rect.tl.y = this_point.y;
                p_t5_toolbar_view_tool_desc->pixit_rect.br.x = p_t5_toolbar_view_tool_desc->pixit_rect.tl.x + this_size.cx;
                p_t5_toolbar_view_tool_desc->pixit_rect.br.y = p_t5_toolbar_view_tool_desc->pixit_rect.tl.y + this_size.cy;

                this_point.x = p_t5_toolbar_view_tool_desc->pixit_rect.br.x;

                if(p_t5_toolbar_view_tool_desc->separator)
                {
                    PIXIT tool_sep_h = fThemedToolbar ? T5_TOOLBAR_TOOL_SEP_H_THEMED : T5_TOOLBAR_TOOL_SEP_H;
                    p_t5_toolbar_view_tool_desc->rhs_extra_div_20 = (U8) (tool_sep_h / 20);
                    this_point.x += p_t5_toolbar_view_tool_desc->rhs_extra_div_20 * 20;
                }

                height = MAX(height, this_size.cy);
            }

            p_t5_toolbar_view_row_desc->height = height;

            cur_point.y += height;
        }

        if(has_status_line_in_toolbar)
            cur_point.y += T5_TOOLBAR_ROW_SEP_V;
        else
            cur_point.y += T5_TOOLBAR_BM;
    }

#if RISCOS
    /* always set up */
    p_view->toolbar.status_line_pixit_rect.tl.x = 0;
    p_view->toolbar.status_line_pixit_rect.tl.y = cur_point.y;
    p_view->toolbar.status_line_pixit_rect.br.x = T5_TOOLBAR_LARGE_WIDTH;
    p_view->toolbar.status_line_pixit_rect.br.y = p_view->toolbar.status_line_pixit_rect.tl.y;

    if(has_status_line_in_toolbar)
    {
        const PIXIT status_line_in_toolbar_height_pixits = status_line_font_height_pixits + 2 * PIXITS_PER_RISCOS;

        p_view->toolbar.status_line_pixit_rect.br.y += status_line_in_toolbar_height_pixits;

        cur_point.y = p_view->toolbar.status_line_pixit_rect.br.y;
    }
#endif

    /* remember ideal height to open with */
#if RISCOS
    toolbar_height_riscos = cur_point.y / PIXITS_PER_RISCOS;
#elif WINDOWS
    { /* convert to dpi-dependent pixels */ /* DPI-aware */
    GDI_SIZE PixelsPerInch;
    host_get_pixel_size(NULL /*screen*/, NULL /*cx*/, &PixelsPerInch.cy); /* Get current pixel size for the screen e.g. 96 or 120 */

    toolbar_height_pixels = idiv_ceil_u(cur_point.y * PixelsPerInch.cy, PIXITS_PER_INCH);
    } /*block*/
#endif

    /* dynamic toolbar doesn't cope well with vertical resizes - more long-term research needed */

#if RISCOS
    /* hack view structure (ugh! - relies on it being shown later to get used) */
    p_view->margin.y1 = - toolbar_height_riscos;

    if(for_view_new)
        view_install_main_window_handler(WIN_TOOLBAR, object_toolbar);
    else
    {
        SKEL_RECT skel_rect;
        RECT_FLAGS rect_flags;
        /*REDRAW_FLAGS redraw_flags;REDRAW_FLAGS_CLEAR(redraw_flags);*/

        skel_rect.tl.pixit_point.x = 0;
        skel_rect.tl.pixit_point.y = 0;
        skel_rect.tl.page_num.x = 0;
        skel_rect.tl.page_num.y = 0;
        skel_rect.br.pixit_point.x = T5_TOOLBAR_LARGE_WIDTH;
        skel_rect.br.pixit_point.y = cur_point.y;
        skel_rect.br.page_num.x = 0;
        skel_rect.br.page_num.y = 0;

        RECT_FLAGS_CLEAR(rect_flags);
        view_update_later_single(p_docu, p_view, UPDATE_BACK_WINDOW, &skel_rect, rect_flags);
    }
#elif WINDOWS
    if(for_view_new)
    {
        U32 packed_viewid = viewid_pack(p_docu, p_view, EVENT_HANDLER_TOOLBAR);
        int nWidth, nHeight;
        {
        RECT client_rect;
        GetClientRect(p_view->main[WIN_BACK].hwnd, &client_rect);
        nWidth = client_rect.right - client_rect.left;
        } /*block*/
        nHeight = !global_preferences.disable_toolbar ? toolbar_height_pixels : 0;
        void_WrapOsBoolChecking(HOST_WND_NONE != (
        p_view->main[WIN_TOOLBAR].hwnd =
            CreateWindowEx(WS_EX_ACCEPTFILES | WS_EX_NOPARENTNOTIFY, window_class[APP_WINDOW_CLASS_TOOLBAR], NULL,
                           WS_CHILD | WS_VISIBLE,
                           0, 0, nWidth, nHeight,
                           p_view->main[WIN_BACK].hwnd, NULL, GetInstanceHandle(), &packed_viewid)));
    }
    else if(HOST_WND_NONE != p_view->main[WIN_TOOLBAR].hwnd)
    {
        InvalidateRect(p_view->main[WIN_TOOLBAR].hwnd, NULL, TRUE);
    }
#endif
    } /*block*/

    { /* tell any user controls where we ended up */
    const ARRAY_INDEX view_rows = array_elements(&p_view->toolbar.h_toolbar_view_row_desc);
    ARRAY_INDEX view_row_idx = view_rows;

    while(--view_row_idx >= 0) /* going down rows ... */
    {
        const P_T5_TOOLBAR_VIEW_ROW_DESC p_t5_toolbar_view_row_desc = array_ptr(&p_view->toolbar.h_toolbar_view_row_desc, T5_TOOLBAR_VIEW_ROW_DESC, view_row_idx);
        const ARRAY_INDEX row_tools = array_elements(&p_t5_toolbar_view_row_desc->h_toolbar_view_tool_desc);
        ARRAY_INDEX row_tool_idx;
        P_T5_TOOLBAR_VIEW_TOOL_DESC p_t5_toolbar_view_tool_desc = array_range(&p_t5_toolbar_view_row_desc->h_toolbar_view_tool_desc, T5_TOOLBAR_VIEW_TOOL_DESC, 0, row_tools);

        for(row_tool_idx = 0; row_tool_idx < row_tools; ++row_tool_idx, ++p_t5_toolbar_view_tool_desc)
        {
            const P_T5_TOOLBAR_DOCU_TOOL_DESC p_t5_toolbar_docu_tool_desc = array_ptr(&p_docu->h_toolbar, T5_TOOLBAR_DOCU_TOOL_DESC, p_t5_toolbar_view_tool_desc->docu_tool_index);
            const PC_T5_TOOLBAR_TOOL_DESC p_t5_toolbar_tool_desc = p_t5_toolbar_docu_tool_desc->p_t5_toolbar_tool_desc;

            switch(ENUM_UNPACK(T5_TOOLBAR_TOOL_TYPE, p_t5_toolbar_tool_desc->bits.type))
            {
            case T5_TOOLBAR_TOOL_TYPE_USER:
                if(for_view_new)
                {
                    T5_TOOLBAR_TOOL_USER_VIEW_NEW t5_toolbar_tool_user_view_new;
                    t5_toolbar_tool_user_view_new.client_handle = 0;
                    t5_toolbar_tool_user_view_new.p_t5_toolbar_tool_desc = p_t5_toolbar_tool_desc;
                    t5_toolbar_tool_user_view_new.viewno = viewno_from_p_view(p_view);
                    status_assert(object_call_id(p_t5_toolbar_tool_desc->command_object_id, p_docu, T5_MSG_TOOLBAR_TOOL_USER_VIEW_NEW, &t5_toolbar_tool_user_view_new));
                    p_t5_toolbar_view_tool_desc->client_handle = t5_toolbar_tool_user_view_new.client_handle;
                    assert(p_t5_toolbar_view_tool_desc->client_handle == 42); /* this can no longer be significant due to dynamic toolbarism */
                }
                else
                    p_t5_toolbar_view_tool_desc->client_handle = 42; /* see! ^^^ */

                if(p_t5_toolbar_view_tool_desc->client_handle)
                {
                    {
                    T5_TOOLBAR_TOOL_USER_SIZE_QUERY t5_toolbar_tool_user_size_query;
                    t5_toolbar_tool_user_size_query.client_handle = p_t5_toolbar_view_tool_desc->client_handle;
                    t5_toolbar_tool_user_size_query.p_t5_toolbar_tool_desc = p_t5_toolbar_tool_desc;
                    t5_toolbar_tool_user_size_query.pixit_size.cx = p_t5_toolbar_view_tool_desc->pixit_rect.br.x - p_t5_toolbar_view_tool_desc->pixit_rect.tl.x;
                    t5_toolbar_tool_user_size_query.pixit_size.cy = p_t5_toolbar_view_tool_desc->pixit_rect.br.y - p_t5_toolbar_view_tool_desc->pixit_rect.tl.y;
                    t5_toolbar_tool_user_size_query.viewno = viewno_from_p_view(p_view);
                    status_assert(object_call_id(p_t5_toolbar_tool_desc->command_object_id, p_docu, T5_MSG_TOOLBAR_TOOL_USER_SIZE_QUERY, &t5_toolbar_tool_user_size_query));
                    p_t5_toolbar_view_tool_desc->pixit_rect.br.x = p_t5_toolbar_view_tool_desc->pixit_rect.tl.x + t5_toolbar_tool_user_size_query.pixit_size.cx;
                    p_t5_toolbar_view_tool_desc->pixit_rect.br.y = p_t5_toolbar_view_tool_desc->pixit_rect.tl.y + t5_toolbar_tool_user_size_query.pixit_size.cy;
                    } /*block*/

                    if(!global_preferences.disable_toolbar)
                    {
                    T5_TOOLBAR_TOOL_USER_POSN_SET t5_toolbar_tool_user_posn_set;
                    t5_toolbar_tool_user_posn_set.client_handle = p_t5_toolbar_view_tool_desc->client_handle;
                    t5_toolbar_tool_user_posn_set.p_t5_toolbar_tool_desc = p_t5_toolbar_tool_desc;
                    t5_toolbar_tool_user_posn_set.pixit_rect = p_t5_toolbar_view_tool_desc->pixit_rect;
                    t5_toolbar_tool_user_posn_set.viewno = viewno_from_p_view(p_view);
                    status_assert(object_call_id(p_t5_toolbar_tool_desc->command_object_id, p_docu, T5_MSG_TOOLBAR_TOOL_USER_POSN_SET, &t5_toolbar_tool_user_posn_set));
                    } /*block*/
                }

                break;

            default:
                break;
            }
        }
    }
    } /*block*/

    return(STATUS_DONE);
}

#if RISCOS

extern void
frame_windows_set_margins(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view)
{
    /* work out the required height of the toolbar given the rows and their contents */
    BOOL has_toolbar = !global_preferences.disable_toolbar;
    BOOL has_status_line_in_toolbar = !global_preferences.disable_status_line;
    PIXIT_POINT cur_point = { 0, 0 };
    BOOL first_time = TRUE;
    PIXIT height = 0;

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    if(has_toolbar)
    {
        const ARRAY_INDEX view_rows = array_elements(&p_view->toolbar.h_toolbar_view_row_desc);
        ARRAY_INDEX view_row_idx = view_rows;

        cur_point.y = T5_TOOLBAR_TM;

        while(--view_row_idx >= 0) /* going down rows ... */
        {
            const P_T5_TOOLBAR_VIEW_ROW_DESC p_t5_toolbar_view_row_desc = array_ptr(&p_view->toolbar.h_toolbar_view_row_desc, T5_TOOLBAR_VIEW_ROW_DESC, view_row_idx);

            if(first_time)
                first_time = FALSE;
            else if(height)
                cur_point.y += T5_TOOLBAR_ROW_SEP_V;

            height = p_t5_toolbar_view_row_desc->height;

            cur_point.y += height;
        }

        if(has_status_line_in_toolbar)
            cur_point.y += T5_TOOLBAR_ROW_SEP_V;
        else
            cur_point.y += T5_TOOLBAR_BM;
    }

    /* always set up */
    p_view->toolbar.status_line_pixit_rect.tl.x = 0;
    p_view->toolbar.status_line_pixit_rect.tl.y = cur_point.y;
    p_view->toolbar.status_line_pixit_rect.br.x = T5_TOOLBAR_LARGE_WIDTH;
    p_view->toolbar.status_line_pixit_rect.br.y = p_view->toolbar.status_line_pixit_rect.tl.y;

    if(has_status_line_in_toolbar)
    {
        const PIXIT status_line_in_toolbar_height_pixits = status_line_font_height_pixits + 2 * PIXITS_PER_RISCOS;

        p_view->toolbar.status_line_pixit_rect.br.y += status_line_in_toolbar_height_pixits;

        cur_point.y = p_view->toolbar.status_line_pixit_rect.br.y;
    }

    toolbar_height_riscos = cur_point.y / PIXITS_PER_RISCOS;

    /* hack view structure */
    if(p_view->margin.y1 == - toolbar_height_riscos)
        return;

    p_view->margin.y1 = - toolbar_height_riscos;

    { /* redraw back window where toolbar lives */
    BBox bbox;
    bbox.xmin = -0x1FFFFFFF; bbox.ymin = -0x1FFFFFFF;
    bbox.xmax =  0x1FFFFFFF; bbox.ymax =  0x1FFFFFFF;
    void_WrapOsErrorReporting(wimp_force_redraw_BBox(p_view->main[WIN_BACK].hwnd, &bbox));
    } /*block*/
}

#endif /* OS */

T5_CMD_PROTO(static, t5_cmd_button)
{
    /* poke a toolbar button, even in if isn't in a view. but DO take note of its enable state! */
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 2);
    const PC_USTR name = p_args[0].val.ustr;
    const P_T5_TOOLBAR_DOCU_TOOL_DESC p_t5_toolbar_docu_tool_desc = find_docu_tool(p_docu, name);
    S32 alternate = 0; /* expanded from BOOL */

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(arg_is_present(p_args, 1))
        alternate = p_args[1].val.s32;

    if(NULL == p_t5_toolbar_docu_tool_desc)
        return(create_error(TOOLB_ERR_UNKNOWN_CONTROL));

    if(0 == bitmap_count(p_t5_toolbar_docu_tool_desc->disable_state, N_BITS_ARG(32)))
        execute_tool(p_docu, P_VIEW_NONE, p_t5_toolbar_docu_tool_desc, alternate);

    return(STATUS_OK);
}

T5_CMD_PROTO(static, t5_cmd_toolbar_tool)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 2);
    const PC_USTR name = p_args[1].val.ustr;
    T5_TOOLBAR_REQUESTED_TOOL_DESC t5_toolbar_requested_tool_desc;
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(4, sizeof32(t5_toolbar_requested_tool_desc), 0);

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    t5_toolbar_requested_tool_desc.row = p_args[0].val.u8n;
    ustr_xstrkpy(ustr_bptr(t5_toolbar_requested_tool_desc.name), elemof32(t5_toolbar_requested_tool_desc.name), name);

    return(al_array_add(&toolbar_requested_array_handle, T5_TOOLBAR_REQUESTED_TOOL_DESC, 1, &array_init_block, &t5_toolbar_requested_tool_desc));
}

#if 0

T5_CMD_PROTO(static, t5_cmd_toolbar_tool_disallow)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 1);
    const PC_USTR name = p_args[0].val.ustr;
    T5_TOOLBAR_DISALLOWED_TOOL_DESC t5_toolbar_disallowed_tool_desc;
    SC_ARRAY_INIT_BLOCK array_init_block = ain_init(4, sizeof32(t5_toolbar_disallowed_tool_desc), 0);
    P_ARRAY_HANDLE p_array_handle = &toolbar_disallowed_array_handle;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    ustr_xstrkpy(t5_toolbar_disallowed_tool_desc.name, elemof32(t5_toolbar_disallowed_tool_desc.name), name);

    return(al_array_add(p_array_handle, 1, &array_init_block, &t5_toolbar_disallowed_tool_desc));
}

#endif

#if RISCOS

/* NB these get t5_message from the BACK_WINDOW_EVENT */

T5_MSG_PROTO(static, t5_msg_back_window_event_click, _InoutRef_ P_BACK_WINDOW_EVENT p_back_window_event)
{
    const PC_SKELEVENT_CLICK p_skelevent_click = p_back_window_event->p_data;
    const P_VIEW p_view = p_skelevent_click->click_context.p_view;
    P_T5_TOOLBAR_VIEW_TOOL_DESC p_t5_toolbar_view_tool_desc;

    status_assert(host_key_cache_emit_events());

    p_t5_toolbar_view_tool_desc = tool_from_point(p_view, &p_skelevent_click->skel_point.pixit_point);

    if(NULL != p_t5_toolbar_view_tool_desc)
    {
        const P_T5_TOOLBAR_DOCU_TOOL_DESC p_t5_toolbar_docu_tool_desc = array_ptr(&p_docu->h_toolbar, T5_TOOLBAR_DOCU_TOOL_DESC, p_t5_toolbar_view_tool_desc->docu_tool_index);

        if(tool_enabled_query(p_t5_toolbar_docu_tool_desc))
        {
            PC_T5_TOOLBAR_TOOL_DESC p_t5_toolbar_tool_desc = p_t5_toolbar_docu_tool_desc->p_t5_toolbar_tool_desc;

            switch(ENUM_UNPACK(T5_TOOLBAR_TOOL_TYPE, p_t5_toolbar_tool_desc->bits.type))
            {
            case T5_TOOLBAR_TOOL_TYPE_USER:
                {
                T5_TOOLBAR_TOOL_USER_MOUSE t5_toolbar_tool_user_mouse;
                t5_toolbar_tool_user_mouse.client_handle = p_t5_toolbar_view_tool_desc->client_handle;
                t5_toolbar_tool_user_mouse.p_t5_toolbar_tool_desc = p_t5_toolbar_tool_desc;
                t5_toolbar_tool_user_mouse.viewno = p_view->viewno;
                t5_toolbar_tool_user_mouse.skelevent_click = *p_skelevent_click;
                t5_toolbar_tool_user_mouse.t5_message = t5_message;
                t5_toolbar_tool_user_mouse.pixit_rect = p_t5_toolbar_view_tool_desc->pixit_rect;
                status_assert(object_call_id(p_t5_toolbar_tool_desc->command_object_id, p_docu, T5_MSG_TOOLBAR_TOOL_USER_MOUSE, &t5_toolbar_tool_user_mouse));
                break;
                }

            default:
                {
                const S32 alternate = (T5_EVENT_CLICK_RIGHT_SINGLE == t5_message) || (T5_EVENT_CLICK_RIGHT_DOUBLE == t5_message);
                execute_tool(p_docu, p_view, p_t5_toolbar_docu_tool_desc, alternate);
                break;
                }
            }
        }
    }

    p_back_window_event->processed = TRUE;
    return(STATUS_OK);
}

T5_MSG_PROTO(static, t5_msg_back_window_event_click_drag, _InoutRef_ P_BACK_WINDOW_EVENT p_back_window_event)
{
    const PC_SKELEVENT_CLICK p_skelevent_click = p_back_window_event->p_data;
    const P_VIEW p_view = p_skelevent_click->click_context.p_view;
    P_T5_TOOLBAR_VIEW_TOOL_DESC p_t5_toolbar_view_tool_desc;

    status_assert(host_key_cache_emit_events());

    p_t5_toolbar_view_tool_desc = tool_from_point(p_view, &p_skelevent_click->skel_point.pixit_point);

    if(NULL != p_t5_toolbar_view_tool_desc)
    {
        const P_T5_TOOLBAR_DOCU_TOOL_DESC p_t5_toolbar_docu_tool_desc = array_ptr(&p_docu->h_toolbar, T5_TOOLBAR_DOCU_TOOL_DESC, p_t5_toolbar_view_tool_desc->docu_tool_index);

        if(tool_enabled_query(p_t5_toolbar_docu_tool_desc))
        {
            PC_T5_TOOLBAR_TOOL_DESC p_t5_toolbar_tool_desc = p_t5_toolbar_docu_tool_desc->p_t5_toolbar_tool_desc;

            if(p_t5_toolbar_tool_desc->bits.auto_repeat)
            {
                const S32 alternate = (T5_EVENT_CLICK_RIGHT_DRAG == t5_message);
                execute_tool(p_docu, p_view, p_t5_toolbar_docu_tool_desc, alternate);
            }
        }
    }

    p_back_window_event->processed = TRUE;
    return(STATUS_OK);
}

T5_MSG_PROTO(static, t5_msg_back_window_event_click_consume, _InoutRef_ P_BACK_WINDOW_EVENT p_back_window_event)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    p_back_window_event->processed = TRUE;

    return(STATUS_OK);
}

T5_MSG_PROTO(static, t5_msg_back_window_event_pointer_movement, _InoutRef_ P_BACK_WINDOW_EVENT p_back_window_event)
{
    const PC_SKELEVENT_CLICK p_skelevent_click = p_back_window_event->p_data;
    const P_VIEW p_view = p_skelevent_click->click_context.p_view;
    P_T5_TOOLBAR_VIEW_TOOL_DESC p_t5_toolbar_view_tool_desc;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    trace_v0(TRACE_APP_CLICK,
            (T5_EVENT_POINTER_ENTERS_WINDOW == t5_message)
                ? "t5_msg_back_window_event T5_EVENT_POINTER_ENTERS_WINDOW"
                : "t5_msg_back_window_event T5_EVENT_POINTER_MOVEMENT");

    p_t5_toolbar_view_tool_desc = tool_from_point(p_view, &p_skelevent_click->skel_point.pixit_point);

    if(NULL != p_t5_toolbar_view_tool_desc)
    {
        const P_T5_TOOLBAR_DOCU_TOOL_DESC p_t5_toolbar_docu_tool_desc = array_ptr(&p_docu->h_toolbar, T5_TOOLBAR_DOCU_TOOL_DESC, p_t5_toolbar_view_tool_desc->docu_tool_index);
        const PC_T5_TOOLBAR_TOOL_DESC p_t5_toolbar_tool_desc = p_t5_toolbar_docu_tool_desc->p_t5_toolbar_tool_desc;
        UI_TEXT ui_text = p_t5_toolbar_tool_desc->ui_text;
        /*status_consume(object_call_id(p_t5_toolbar_tool_desc->resource_object_id, p_docu, T5_MSG_STATUS_LINE_MESSAGE_QUERY, &ui_text));*/
        status_line_set(p_docu, STATUS_LINE_LEVEL_BACKWINDOW_CONTROLS, &ui_text);
    }
    else
        status_line_clear(p_docu, STATUS_LINE_LEVEL_BACKWINDOW_CONTROLS);

    p_back_window_event->processed = TRUE;

    return(STATUS_OK);
}

T5_MSG_PROTO(static, t5_msg_back_window_event_pointer_leaves_window, _InoutRef_ P_BACK_WINDOW_EVENT p_back_window_event)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);
    UNREFERENCED_PARAMETER_InoutRef_(p_back_window_event);

    trace_0(TRACE_APP_CLICK, TEXT("t5_msg_back_window_event T5_EVENT_POINTER_LEAVES_WINDOW"));

    status_line_clear(p_docu, STATUS_LINE_LEVEL_BACKWINDOW_CONTROLS);

    return(STATUS_OK);
}

T5_MSG_PROTO(static, t5_msg_back_window_event, _InoutRef_ P_BACK_WINDOW_EVENT p_back_window_event)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    switch(p_back_window_event->t5_message)
    {
    case T5_EVENT_CLICK_LEFT_SINGLE:
    case T5_EVENT_CLICK_LEFT_DOUBLE: /* on RISC OS we get a DOUBLE following a SINGLE */
    case T5_EVENT_CLICK_RIGHT_SINGLE:
    case T5_EVENT_CLICK_RIGHT_DOUBLE:
        return(t5_msg_back_window_event_click(p_docu, p_back_window_event->t5_message, p_back_window_event));

    case T5_EVENT_CLICK_LEFT_DRAG:
    case T5_EVENT_CLICK_RIGHT_DRAG:
        return(t5_msg_back_window_event_click_drag(p_docu, p_back_window_event->t5_message, p_back_window_event));

    case T5_EVENT_CLICK_LEFT_TRIPLE:
    case T5_EVENT_CLICK_RIGHT_TRIPLE:
    case T5_EVENT_FILEINSERT_DOINSERT:
    case T5_EVENT_CLICK_DRAG_STARTED:
    case T5_EVENT_CLICK_DRAG_MOVEMENT:
    case T5_EVENT_CLICK_DRAG_FINISHED:
    case T5_EVENT_CLICK_DRAG_ABORTED:
        return(t5_msg_back_window_event_click_consume(p_docu, p_back_window_event->t5_message, p_back_window_event));

    case T5_EVENT_POINTER_ENTERS_WINDOW:
    case T5_EVENT_POINTER_MOVEMENT:
        return(t5_msg_back_window_event_pointer_movement(p_docu, p_back_window_event->t5_message, p_back_window_event));

    case T5_EVENT_POINTER_LEAVES_WINDOW:
        return(t5_msg_back_window_event_pointer_leaves_window(p_docu, p_back_window_event->t5_message, p_back_window_event));

    default:
        return(STATUS_OK);
    }
}

T5_MSG_PROTO(static, t5_msg_back_window_event_resized, P_VIEWEVENT_VISIBLEAREA_CHANGED p_viewevent_visiblearea_changed)
{
    const P_VIEW p_view = p_viewevent_visiblearea_changed->visible_area.p_view;
    PIXIT_RECT pixit_rect = p_viewevent_visiblearea_changed->visible_area.rect;
    const ARRAY_INDEX view_rows = array_elements(&p_view->toolbar.h_toolbar_view_row_desc);
    ARRAY_INDEX view_row_idx;
    P_T5_TOOLBAR_VIEW_ROW_DESC p_t5_toolbar_view_row_desc = array_range(&p_view->toolbar.h_toolbar_view_row_desc, T5_TOOLBAR_VIEW_ROW_DESC, 0, view_rows);

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    for(view_row_idx = 0; view_row_idx < view_rows; ++view_row_idx, ++p_t5_toolbar_view_row_desc)
    {
        const ARRAY_INDEX row_tools = array_elements(&p_t5_toolbar_view_row_desc->h_toolbar_view_tool_desc);
        ARRAY_INDEX row_tool_idx;
        P_T5_TOOLBAR_VIEW_TOOL_DESC p_t5_toolbar_view_tool_desc = array_range(&p_t5_toolbar_view_row_desc->h_toolbar_view_tool_desc, T5_TOOLBAR_VIEW_TOOL_DESC, 0, row_tools);

        for(row_tool_idx = 0; row_tool_idx < row_tools; ++row_tool_idx, ++p_t5_toolbar_view_tool_desc)
        {
            const P_T5_TOOLBAR_DOCU_TOOL_DESC p_t5_toolbar_docu_tool_desc = array_ptr(&p_docu->h_toolbar, T5_TOOLBAR_DOCU_TOOL_DESC, p_t5_toolbar_view_tool_desc->docu_tool_index);
            const PC_T5_TOOLBAR_TOOL_DESC p_t5_toolbar_tool_desc = p_t5_toolbar_docu_tool_desc->p_t5_toolbar_tool_desc;

            switch(ENUM_UNPACK(T5_TOOLBAR_TOOL_TYPE, p_t5_toolbar_tool_desc->bits.type))
            {
            case T5_TOOLBAR_TOOL_TYPE_USER:
                if(p_t5_toolbar_view_tool_desc->client_handle)
                {
                    T5_TOOLBAR_TOOL_USER_POSN_SET t5_toolbar_tool_user_posn_set;
                    t5_toolbar_tool_user_posn_set.client_handle = p_t5_toolbar_view_tool_desc->client_handle;
                    t5_toolbar_tool_user_posn_set.p_t5_toolbar_tool_desc = p_t5_toolbar_tool_desc;
                    t5_toolbar_tool_user_posn_set.pixit_rect = p_t5_toolbar_view_tool_desc->pixit_rect;
                    t5_toolbar_tool_user_posn_set.viewno = viewno_from_p_view(p_view);
                    if(t5_toolbar_tool_user_posn_set.pixit_rect.br.x >= pixit_rect.br.x - T5_TOOLBAR_RM)
                        t5_toolbar_tool_user_posn_set.pixit_rect.br.x = pixit_rect.br.x - T5_TOOLBAR_RM;
                    if(t5_toolbar_tool_user_posn_set.pixit_rect.br.x < t5_toolbar_tool_user_posn_set.pixit_rect.tl.x)
                        t5_toolbar_tool_user_posn_set.pixit_rect.br.x = t5_toolbar_tool_user_posn_set.pixit_rect.tl.x;
                    status_assert(object_call_id(p_t5_toolbar_tool_desc->command_object_id, p_docu, T5_MSG_TOOLBAR_TOOL_USER_POSN_SET, &t5_toolbar_tool_user_posn_set));
                }
                break;

            default:
                break;
            }
        }
    }

    return(STATUS_OK);
}

#endif /* RISCOS */

T5_MSG_PROTO(static, t5_msg_toolbar_tools, _InRef_ PC_T5_TOOLBAR_TOOLS p_t5_toolbar_tools)
{
    STATUS status = STATUS_OK;
    ARRAY_INDEX i;
    const ARRAY_INDEX n = array_elements(&toolbar_candidates_array_handle);

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    for(i = 0; i < p_t5_toolbar_tools->n_tool_desc; ++i)
    {
        const PC_T5_TOOLBAR_TOOL_DESC p_t5_toolbar_tool_desc = p_t5_toolbar_tools->p_t5_toolbar_tool_desc + i;
        SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(PC_T5_TOOLBAR_TOOL_DESC), 1);
        status_break(status = al_array_add(&toolbar_candidates_array_handle, PC_T5_TOOLBAR_TOOL_DESC, 1, &array_init_block, &p_t5_toolbar_tool_desc));
    }

    { /* add these to the running. so long as they come in before a view is created they'll get shown */
    DOCNO docno = DOCNO_NONE;

    while(DOCNO_NONE != (docno = docno_enum_docs(docno)))
    {
        status_break(status = toolbar_candidate_range_added(p_docu_from_docno_valid(docno), n, n + p_t5_toolbar_tools->n_tool_desc));
    }
    } /*block*/

    return(status);
}

/******************************************************************************
*
* toolbar object event handler
*
******************************************************************************/

_Check_return_
static STATUS
toolbar_msg_startup_services(void)
{
#if WINDOWS
    status_return(toolbar_register_wndclass());

    status_return(status_line_register_wndclass());
#endif

    return(STATUS_OK);
}

_Check_return_
static STATUS
toolbar_msg_startup(void)
{
    status_return(resource_init(OBJECT_ID_TOOLBAR, P_BOUND_MESSAGES_OBJECT_ID_TOOLB, P_BOUND_RESOURCES_OBJECT_ID_TOOLB));

#if RISCOS
    generate_greyed_tool_palette();
#endif

    status_line_style_startup();

    return(register_object_construct_table(OBJECT_ID_TOOLBAR, object_construct_table, FALSE /* no inlines */));
}

_Check_return_
static STATUS
toolbar_msg_init_thunk(
    _DocuRef_   P_DOCU p_docu)
{
    /* build a state and enable state holding array from all current candidate tools */
    p_docu->h_toolbar = 0;

    p_docu->status_line_ui_text.type = UI_TEXT_TYPE_NONE;

    return(toolbar_candidate_range_added(p_docu, 0, array_elements(&toolbar_candidates_array_handle)));
}

_Check_return_
static STATUS
toolbar_msg_close_thunk(
    _DocuRef_   P_DOCU p_docu)
{
    al_array_dispose(&p_docu->h_toolbar);

    ui_text_dispose(&p_docu->status_line_ui_text);

    return(STATUS_OK);
}

_Check_return_
static STATUS
toolbar_msg_exit1(void)
{
    al_array_dispose(&toolbar_requested_array_handle);
    al_array_dispose(&toolbar_disallowed_array_handle);
    al_array_dispose(&toolbar_candidates_array_handle);

    /* status_line_style resources belong to style system */

    return(resource_close(OBJECT_ID_TOOLBAR));
}

T5_MSG_PROTO(static, toolbar_msg_initclose, _InRef_ PC_MSG_INITCLOSE p_msg_initclose)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__STARTUP_SERVICES:
        return(toolbar_msg_startup_services());

    case T5_MSG_IC__STARTUP:
        return(toolbar_msg_startup());

    case T5_MSG_IC__STARTUP_CONFIG:
        return(load_object_config_file(OBJECT_ID_TOOLBAR));

    case T5_MSG_IC__EXIT1:
        return(toolbar_msg_exit1());

    /* initialise object in new document thunk */
    case T5_MSG_IC__INIT_THUNK:
        /* needs to be active before and after clients on INITn/CLOSEn */
        return(toolbar_msg_init_thunk(p_docu));

    case T5_MSG_IC__CLOSE_THUNK:
        return(toolbar_msg_close_thunk(p_docu));

    default:
        return(STATUS_OK);
    }
}

_Check_return_
static STATUS
toolbar_msg_choice_changed_status_line_or_toolbar(
    _DocuRef_       P_DOCU p_docu)
{
    VIEWNO viewno = VIEWNO_NONE;
    P_VIEW p_view;

    while(P_VIEW_NONE != (p_view = docu_enum_views(p_docu, &viewno)))
        host_view_reopen(p_docu, p_view);

    return(STATUS_OK);
}

_Check_return_
static STATUS
toolbar_msg_choice_changed_ui_styles(
    _DocuRef_       P_DOCU p_docu)
{
    if(DOCNO_CONFIG == docno_from_p_docu(p_docu))
    {
        recache_status_line_style = TRUE;
    }

    return(toolbar_msg_choice_changed_status_line_or_toolbar(p_docu));
}

T5_MSG_PROTO(static, toolbar_msg_choice_changed, _InRef_ PC_MSG_CHOICE_CHANGED p_msg_choice_changed)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    switch(p_msg_choice_changed->t5_message)
    {
    case T5_CMD_CHOICES_STATUS_LINE:
    case T5_CMD_CHOICES_TOOLBAR:
        return(toolbar_msg_choice_changed_status_line_or_toolbar(p_docu));

    case T5_CMD_STYLE_FOR_CONFIG:
        return(toolbar_msg_choice_changed_ui_styles(p_docu));

    default:
        return(STATUS_OK);
    }
}

OBJECT_PROTO(extern, object_toolbar)
{
    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        return(toolbar_msg_initclose(p_docu, t5_message, (PC_MSG_INITCLOSE) p_data));

    case T5_MSG_CHOICE_CHANGED:
        return(toolbar_msg_choice_changed(p_docu, t5_message, (PC_MSG_CHOICE_CHANGED) p_data));

    case T5_MSG_VIEW_NEW:
        return(toolbar_msg_view_new(p_docu, t5_message, (P_T5_MSG_VIEW_NEW_BLOCK) p_data));

    case T5_MSG_VIEW_DESTROY:
        return(toolbar_msg_view_destroy(p_docu, t5_message, (P_T5_MSG_VIEW_DESTROY_BLOCK) p_data));

    case T5_MSG_STATUS_LINE_SET:
        return(t5_msg_status_line_set(p_docu, t5_message, (PC_UI_TEXT) p_data));

    case T5_MSG_TOOLBAR_TOOLS:
        return(t5_msg_toolbar_tools(p_docu, t5_message, (PC_T5_TOOLBAR_TOOLS) p_data));

    case T5_MSG_TOOLBAR_TOOL_QUERY:
        return(msg_toolbar_tool_query(p_docu, t5_message, (P_T5_TOOLBAR_TOOL_QUERY) p_data));

    case T5_MSG_TOOLBAR_TOOL_SET:
        return(msg_toolbar_tool_set(p_docu, t5_message, (PC_T5_TOOLBAR_TOOL_SET) p_data));

    case T5_MSG_TOOLBAR_TOOL_ENABLE:
        return(msg_toolbar_tool_enable(p_docu, t5_message, (PC_T5_TOOLBAR_TOOL_ENABLE) p_data));

    case T5_MSG_TOOLBAR_TOOL_ENABLE_QUERY:
        return(msg_toolbar_tool_enable_query(p_docu, t5_message, (P_T5_TOOLBAR_TOOL_ENABLE_QUERY) p_data));

    case T5_MSG_TOOLBAR_TOOL_DISABLE:
        return(msg_toolbar_tool_disable(p_docu, t5_message, (PC_T5_TOOLBAR_TOOL_DISABLE) p_data));

    case T5_MSG_TOOLBAR_TOOL_NOBBLE:
        return(msg_toolbar_tool_nobble(p_docu, t5_message, (PC_T5_TOOLBAR_TOOL_NOBBLE) p_data));

#if WINDOWS
    case T5_MSG_FRAME_WINDOWS_DESCRIBE:
        return(t5_msg_frame_windows_describe(p_docu, t5_message, (P_MSG_FRAME_WINDOW) p_data));

    case T5_MSG_FRAME_WINDOWS_POSN:
        return(t5_msg_frame_windows_posn(p_docu, t5_message, (P_MSG_FRAME_WINDOW) p_data));
#endif /* WINDOWS */

#if RISCOS
    case T5_MSG_BACK_WINDOW_EVENT:
        return(t5_msg_back_window_event(p_docu, t5_message, p_data));

    case T5_EVENT_REDRAW:
        return(paint_toolbar_riscos(p_docu, t5_message, p_data));

    case T5_EVENT_VISIBLEAREA_CHANGED:
        return(t5_msg_back_window_event_resized(p_docu, t5_message, p_data));
#endif

    case T5_EVENT_SCHEDULED:
        return(scheduled_toolbar_redraw(p_docu, t5_message, (P_SCHEDULED_EVENT_BLOCK) p_data));

    case T5_CMD_BUTTON:
        return(t5_cmd_button(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_TOOLBAR_TOOL:
        return(t5_cmd_toolbar_tool(p_docu, t5_message, (PC_T5_CMD) p_data));

    default:
        return(STATUS_OK);
    }
}

/* end of ob_toolb.c */
