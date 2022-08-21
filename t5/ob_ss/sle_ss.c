/* sle_ss.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* SLE */

/* David De Vorchik (diz) Mar-93 Created / 25-Mar-94 Windows version */

#include "common/gflags.h"

#include "ob_ss/ob_ss.h"

#include "ob_toolb/xp_toolb.h"

#if RISCOS
#include "ob_skel/xp_skeld.h"

#include "ob_dlg/xp_dlgr.h"

#define RISCOS_FONTY_SLE 1
#endif

#if RISCOS

#define X_OFFSET_PIXITS (PIXITS_PER_RISCOS * 8)
#define Y_OFFSET_PIXITS (PIXITS_PER_RISCOS * 8)

#if defined(RISCOS_FONTY_SLE)
static PIXIT CHAR_WIDTH_PIXITS = (PIXITS_PER_RISCOS * 16 /* OS units */);
#else
#define CHAR_WIDTH_PIXITS   (PIXITS_PER_RISCOS * 16 /* OS units */)
#endif
#define CHAR_HEIGHT_PIXITS  (PIXITS_PER_RISCOS * 32)

#define CARET_HEIGHT_RISCOS (                32 +                     8)
#define CARET_HEIGHT_PIXITS (CHAR_HEIGHT_PIXITS + PIXITS_PER_RISCOS * 8)

#elif WINDOWS

#define TEXT_X          3

#define RIGHT_INDENT    0 /*(4 * PIXITS_PER_INCH / PixelsPerInch.cx)*/

#endif /* OS */

#define IL_SLE_ARGUMENT_OFF_FUNC_NUMBER 0   /* function number for displaying information */
#define IL_SLE_ARGUMENT_OFF_ARG_NUMBER  2   /* index for argument within the string */
#define IL_SLE_ARGUMENT_OFF_ARG_USTR    4   /* textual string displayed when inline encountered */

#define il_sle_argument_data_off(__ptr_type, ustr_inline, off, add) ( \
    PtrAddBytes(__ptr_type, inline_data_ptr_off(__ptr_type, ustr_inline, off), add) )

OBJECT_PROTO(extern, object_sle);

/*
callback functions
*/

PROC_EVENT_PROTO(static, ob_sle_drag_after_click_filter);

/*
internal functions
*/

_Check_return_
static STATUS
sle_delete_char(
    P_SLE_INFO_BLOCK p_sle_info_block,
    _InVal_     BOOL right);

static void
sle_display_caret(
    _InoutRef_  P_SLE_INFO_BLOCK p_sle_info_block);

_Check_return_
static STATUS
sle_grab_focus(
    P_SLE_INFO_BLOCK p_sle_info_block);

_Check_return_
static STATUS
sle_index_compile(
    _InoutRef_  P_SLE_INFO_BLOCK p_sle_info_block,
    _InVal_     BOOL auto_highlight,
    _In_        BOOL refresh,
    _InVal_     BOOL scroll_it);

static void
sle_refresh_field(
    P_SLE_INFO_BLOCK p_sle_info_block);

static void
sle_selection_clear(
    P_SLE_INFO_BLOCK p_sle_info_block);

_Check_return_
static STATUS
sle_selection_delete(
    P_SLE_INFO_BLOCK p_sle_info_block);

#if WINDOWS

static S32
sle_index_from_pixel_x(
    P_SLE_INFO_BLOCK p_sle_info_block,
    _HwndRef_   HWND hwnd,
    _In_        int x);

static S32
sle_search_string(
    _HdcRef_    HDC hdc,
    _In_reads_(end) PCTSTR tstr,
    _InVal_     S32 stt,
    _InVal_     S32 end,
    _In_        int x);

#endif /* WINDOWS */

static const ARG_TYPE
ss_args_string[] = { ARG_TYPE_USTR, ARG_TYPE_NONE };

static CONSTRUCT_TABLE
object_construct_table[] =
{                                                                                                   /*   fi ti mi ur up xi md mf nn cp sm ba fo */


                                                                                                    /*   fi                            reject if file insertion */
                                                                                                    /*      ti                         reject if template insertion */
                                                                                                    /*         mi                      maybe interactive */
                                                                                                    /*            ur                   unrecordable */
                                                                                                    /*               up                unrepeatable */
                                                                                                    /*                  xi             exceptional inline */
                                                                                                    /*                     md          modify document */
                                                                                                    /*                        mf       memory froth */
                                                                                                    /*                           nn    supress newline on save */
                                                                                                    /*                              cp check for protection */

    { "EditFormula",            NULL,                       T5_CMD_SS_EDIT_FORMULA,                     { 0, 0, 0, 0, 0, 0, 0, 1, 0, 1 } },

    { "CancelFormula",          NULL,                       T5_CMD_ESCAPE },
    { "EnterFormula",           NULL,                       T5_CMD_RETURN },

    { NULL,                     NULL,                       T5_EVENT_NONE }
};

SC_ARRAY_INIT_BLOCK array_init_block_sle = aib_init(64, sizeof32(U8), FALSE);

/*
stuff for editing line focus handler
*/

static DOCNO docno_ss_edit = DOCNO_NONE;

static SLR slr_range_anchor;

static SLR old_slr[2];

static S32 abs_anchor = 0;

#define sle_selection_lhs_index(p_sle_info_block) \
    MIN(p_sle_info_block->selection_stt_index, p_sle_info_block->selection_end_index)

#define sle_selection_rhs_index(p_sle_info_block) \
    MAX(p_sle_info_block->selection_stt_index, p_sle_info_block->selection_end_index)

_Check_return_
static STATUS
ss_range_drag_output(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_SLR p_slr_to,
    _InVal_     S32 abs)
{
    SLR slr[2];
    EV_SLR ev_slr[2];
    UCHARZ ustr_buf[BUF_EV_LONGNAMLEN + BUF_EV_INTNAMLEN]; /* SKS 18jul93 buffer[50] was a bit tiny */
    U32 len;
    T5_PASTE_EDITLINE t5_paste_editline;
    QUICK_UBLOCK quick_ublock;

    slr[0] = slr_range_anchor;
    slr[1] = *p_slr_to;

    /* SKS 15apr93 sort into sensible range order */
    if(slr[0].col > slr[1].col)
    {
        COL temp_col;
        temp_col   = slr[0].col;
        slr[0].col = slr[1].col;
        slr[1].col =   temp_col;
    }

    if(slr[0].row > slr[1].row)
    {
        ROW temp_row;
        temp_row   = slr[0].row;
        slr[0].row = slr[1].row;
        slr[1].row =   temp_row;
    }

    if(slr[0].col == old_slr[0].col)
    if(slr[0].row == old_slr[0].row)
    if(slr[1].col == old_slr[1].col)
    if(slr[1].row == old_slr[1].row)
        return(STATUS_OK);

    old_slr[0] = slr[0];
    old_slr[1] = slr[1];

    if(abs >= 0)
        abs_anchor = abs;

    ev_slr_from_slr(p_docu, &ev_slr[0], &slr[0]);
    ev_slr[0].abs_col = ev_slr[0].abs_row = UBF_PACK(abs_anchor);

    ev_slr_from_slr(p_docu, &ev_slr[1], &slr[1]);
    ev_slr[1].docno = EV_DOCNO_PACK(docno_ss_edit); /* SKS 18jul93 don't repeat extref gumf */
    ev_slr[1].abs_col = ev_slr[1].abs_row = UBF_PACK(abs_anchor);

    {
    const U32 elemof_buffer = elemof32(ustr_buf);
    SS_DECOMPILER_OPTIONS ss_decompiler_options = g_ss_decompiler_options;

    if(global_preferences.ss_alternate_formula_style)
    {   /* Alternate formula style (Excel-ise) the decompiler output */
        g_ss_decompiler_options.lf = 0; /* prune these out for now */
        g_ss_decompiler_options.cr = 0;
        g_ss_decompiler_options.initial_formula_equals = 1;
        g_ss_decompiler_options.range_colon_separator = 1;
        g_ss_decompiler_options.upper_case_function = 1;
        g_ss_decompiler_options.upper_case_slr = 1;
        g_ss_decompiler_options.zero_args_function_parentheses = 1;
    }

    len  = ev_dec_slr_ustr_buf(ustr_bptr(ustr_buf) , elemof_buffer , docno_ss_edit, &ev_slr[0]);

    if(g_ss_decompiler_options.range_colon_separator)
        len += xsnprintf(PtrAddBytes(P_U8Z, ustr_buf, len), elemof_buffer - len, "%c", CH_COLON);

    len += ev_dec_slr_ustr_buf(ustr_bptr(ustr_buf + len), elemof_buffer- len, docno_ss_edit, &ev_slr[1]);

    g_ss_decompiler_options = ss_decompiler_options;
    } /*block*/

    quick_ublock_setup_fill_from_ubuf(&quick_ublock, uchars_bptr(ustr_buf), len);

    t5_paste_editline.p_quick_ublock = &quick_ublock;
    t5_paste_editline.select = TRUE;
    t5_paste_editline.special_select = TRUE;

    return(object_call_id(OBJECT_ID_SLE, p_docu_from_docno(docno_ss_edit), T5_MSG_PASTE_EDITLINE, &t5_paste_editline));
}

/******************************************************************************
*
* catch clicks and drags when in editing line
*
******************************************************************************/

_Check_return_
static STATUS
ob_sle_event_click_filter(
    _DocuRef_ P_DOCU p_docu,
    P_SKELEVENT_CLICK_FILTER p_skelevent_click_filter)
{
    STATUS status = STATUS_OK;

    p_skelevent_click_filter->processed_level = SKELEVENT_CLICK_FILTER_SUPPRESS;

    switch(p_skelevent_click_filter->t5_message)
    {
    case T5_EVENT_CLICK_LEFT_SINGLE:
    case T5_EVENT_CLICK_RIGHT_SINGLE:
        {
        const T5_MESSAGE t5_message_right = T5_EVENT_CLICK_RIGHT_SINGLE;
        const T5_MESSAGE t5_message_effective = right_message_if_ctrl(p_skelevent_click_filter->t5_message, t5_message_right, &p_skelevent_click_filter->data.click[SKELEVENT_CLICK_FILTER_SLOTAREA].skelevent_click);
        const BOOL abs = (t5_message_right == t5_message_effective);
        SKEL_POINT tl;
        SLR slr;

        if(p_skelevent_click_filter->data.click[SKELEVENT_CLICK_FILTER_SLOTAREA].in_area
        && status_ok(slr_owner_from_skel_point(p_docu, &slr, &tl,
                                               &p_skelevent_click_filter->data.click[SKELEVENT_CLICK_FILTER_SLOTAREA].skelevent_click.skel_point,
                                               ON_ROW_EDGE_GO_UP)))
        {
            EV_SLR ev_slr;
            UCHARZ ustr_buf[BUF_EV_LONGNAMLEN + BUF_EV_INTNAMLEN];
            U32 len;
            T5_PASTE_EDITLINE t5_paste_editline;
            QUICK_UBLOCK quick_ublock;

            p_skelevent_click_filter->processed_level = SKELEVENT_CLICK_FILTER_SLOTAREA;

            slr_range_anchor = slr;

            ev_slr_from_slr(p_docu, &ev_slr, &slr);
            if(abs)
                ev_slr.abs_col = ev_slr.abs_row = 1;

            {
            const U32 elemof_buffer = elemof32(ustr_buf);
            SS_DECOMPILER_OPTIONS ss_decompiler_options = g_ss_decompiler_options;

            if(global_preferences.ss_alternate_formula_style)
            {   /* Alternate formula style (Excel-ise) the decompiler output */
                g_ss_decompiler_options.lf = 0; /* prune these out for now */
                g_ss_decompiler_options.cr = 0;
                g_ss_decompiler_options.initial_formula_equals = 1;
                g_ss_decompiler_options.range_colon_separator = 1;
                g_ss_decompiler_options.upper_case_function = 1;
                g_ss_decompiler_options.upper_case_slr = 1;
                g_ss_decompiler_options.zero_args_function_parentheses = 1;
            }

            len = ev_dec_slr_ustr_buf(ustr_bptr(ustr_buf), elemof_buffer, docno_ss_edit, &ev_slr);

            g_ss_decompiler_options = ss_decompiler_options;
            } /*block*/

            quick_ublock_setup_fill_from_ubuf(&quick_ublock, uchars_bptr(ustr_buf), len);

            t5_paste_editline.p_quick_ublock = &quick_ublock;
            t5_paste_editline.select = TRUE;
            t5_paste_editline.special_select = TRUE;

            status = object_call_id(OBJECT_ID_SLE, p_docu_from_docno(docno_ss_edit), T5_MSG_PASTE_EDITLINE, &t5_paste_editline);
        }

        break;
        }

    case T5_EVENT_CLICK_LEFT_DRAG:
    case T5_EVENT_CLICK_RIGHT_DRAG:
        {
        const T5_MESSAGE t5_message_right = T5_EVENT_CLICK_RIGHT_DRAG;
        const T5_MESSAGE t5_message_effective = right_message_if_ctrl(p_skelevent_click_filter->t5_message, t5_message_right, &p_skelevent_click_filter->data.click[SKELEVENT_CLICK_FILTER_SLOTAREA].skelevent_click);
        const BOOL abs = (t5_message_right == t5_message_effective);
        SLR slr;
        SKEL_POINT tl;

        if(p_skelevent_click_filter->data.click[SKELEVENT_CLICK_FILTER_SLOTAREA].in_area
        && status_ok(slr_owner_from_skel_point(p_docu, &slr, &tl,
                                               &p_skelevent_click_filter->data.click[SKELEVENT_CLICK_FILTER_SLOTAREA].skelevent_click.skel_point,
                                               ON_ROW_EDGE_GO_UP)))
        {
            old_slr[0].col = (COL) -1;
            old_slr[0].row = (ROW) -1;
            old_slr[1] = old_slr[0];

            /* direct drags to other (non-maeve) proc */
            p_skelevent_click_filter->processed_level = SKELEVENT_CLICK_FILTER_SLOTAREA;
            p_skelevent_click_filter->data.click[SKELEVENT_CLICK_FILTER_SLOTAREA].redraw_tag_and_event.p_proc_event = ob_sle_drag_after_click_filter;

            host_drag_start(NULL);

            status = ss_range_drag_output(p_docu, &slr, abs);
        }
        break;
        }

    case T5_EVENT_POINTER_MOVEMENT:
        host_set_pointer_shape(POINTER_FORMULA_LINE);
        break;

    case T5_EVENT_POINTER_ENTERS_WINDOW:
        host_set_pointer_shape(POINTER_FORMULA_LINE);

        p_skelevent_click_filter->processed_level = SKELEVENT_CLICK_FILTER_SLOTAREA;
        p_skelevent_click_filter->data.click[SKELEVENT_CLICK_FILTER_SLOTAREA].redraw_tag_and_event.p_proc_event = ob_sle_drag_after_click_filter;
        break;

    case T5_EVENT_POINTER_LEAVES_WINDOW:
        break;

    default:
        break;
    }

    return(status);
}

MAEVE_SERVICES_EVENT_PROTO(static, maeve_services_ob_sle_click_filter)
{
    UNREFERENCED_PARAMETER_InRef_(p_maeve_services_block);

    switch(t5_message)
    {
    case T5_EVENT_CLICK_FILTER:
        return(ob_sle_event_click_filter(p_docu, (P_SKELEVENT_CLICK_FILTER) p_data));

    default:
        return(STATUS_OK);
    }
}

PROC_EVENT_PROTO(static, ob_sle_drag_after_click_filter)
{
    STATUS status = STATUS_OK;

    /* NB. message is directed straight here; not sent via maeve */
    switch(t5_message)
    {
    case T5_EVENT_CLICK_DRAG_FINISHED:
    case T5_EVENT_CLICK_DRAG_MOVEMENT:
        {
        P_SKELEVENT_CLICK p_skelevent_click = (P_SKELEVENT_CLICK) p_data;
        SLR slr;
        SKEL_POINT tl;

        if(status_ok(slr_owner_from_skel_point(p_docu, &slr, &tl, &p_skelevent_click->skel_point, ON_ROW_EDGE_GO_UP)))
            status = ss_range_drag_output(p_docu, &slr, -1);
        break;
        }

    case T5_EVENT_CLICK_DRAG_ABORTED:
        break;

    case T5_EVENT_POINTER_MOVEMENT:
        break;

    default:
        break;
    }

    return(status);
}

/******************************************************************************
*
* stops click catcher
*
******************************************************************************/

static BOOL
ss_edit_focus_release(
    P_SLE_INFO_BLOCK p_sle_info_block)
{
    if(p_sle_info_block->has_focus)
    {
        const P_DOCU p_docu = p_docu_from_docno(p_sle_info_block->docno);
        const P_SLE_INSTANCE_DATA p_sle_instance = p_object_instance_data_SLE(p_docu);
        assert(p_sle_instance->maeve_click_filter_handle != 0);
        maeve_services_event_handler_del_handle(p_sle_instance->maeve_click_filter_handle);
        p_sle_instance->maeve_click_filter_handle = 0;
        host_reenter_window();
        docno_ss_edit = DOCNO_NONE;
        p_sle_info_block->has_focus = FALSE;
        return(TRUE);
    }
    else
        return(FALSE);
}

/* Revoke the focus back to the world, including releasing our grip of the status line */

_Check_return_
static STATUS
sle_giveup_focus(
    P_SLE_INFO_BLOCK p_sle_info_block)
{
    STATUS status = STATUS_OK;

    if(p_sle_info_block->status_line_set)
    {
        p_sle_info_block->status_line_set = FALSE;

        status_line_clear(p_docu_from_docno(p_sle_info_block->docno), STATUS_LINE_LEVEL_SS_FORMULA_LINE);
    }

    if(ss_edit_focus_release(p_sle_info_block))
    {
        {
        const P_DOCU p_docu = p_docu_from_docno(p_sle_info_block->docno);
        caret_show_claim(p_docu, p_docu->focus_owner_old, FALSE);
        } /*block*/

        status = sle_index_compile(p_sle_info_block, TRUE, FALSE, TRUE);
    }

    return(status);
}

static void
sle_dispose(
    P_SLE_INFO_BLOCK p_sle_info_block)
{
    al_array_dispose(&p_sle_info_block->h_ustr_inline);

    ss_edit_focus_release(p_sle_info_block);
}

/* called when ss object gets the focus for its editing line; starts catching of clicks and drags */

_Check_return_
static STATUS
ss_edit_focus_claim(
    _DocuRef_   P_DOCU p_docu)
{
    const P_SLE_INSTANCE_DATA p_sle_instance = p_object_instance_data_SLE(p_docu);
    const P_SLE_INFO_BLOCK p_sle_info_block = &p_sle_instance->ss_editor;
    STATUS status;

    assert(p_sle_instance->maeve_click_filter_handle == 0);

    if(status_ok(status = maeve_services_event_handler_add(maeve_services_ob_sle_click_filter)))
    {
        p_sle_instance->maeve_click_filter_handle = status;

        p_sle_info_block->has_focus = TRUE;

        host_reenter_window();

        docno_ss_edit = docno_from_p_docu(p_docu);

        p_docu->focus_owner_old = p_docu->focus_owner;

        caret_show_claim(p_docu, OBJECT_ID_SLE, FALSE);

        status = STATUS_OK;
    }

    return(status);
}

/* Evaluate the formula line and place its contents back into the document */

_Check_return_
static STATUS
ss_formula_eval_then_msg(
    _DocuRef_   P_DOCU p_docu_in,
    _InVal_     T5_MESSAGE t5_message)
{
    P_DOCU p_docu = p_docu_in;
    const P_SLE_INSTANCE_DATA p_sle_instance = p_object_instance_data_SLE(p_docu);
    const P_SLE_INFO_BLOCK p_sle_info_block = &p_sle_instance->ss_editor;
    ARGLIST_HANDLE arglist_handle;
    U32 uchars_n;
    P_USTR ustr;
    STATUS status;
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock, elemof32("function(and....some....very....parpy....arguments)"));
    quick_ublock_with_buffer_setup(quick_ublock);

    /* copy into a quick buf removing any inlines present in the string */
    uchars_n = ustr_inline_strlen32p1(ustr_inline_from_h_ustr(&p_sle_info_block->h_ustr_inline)); /*CH_NULL*/

    if(NULL == (ustr = quick_ublock_extend_by(&quick_ublock, uchars_n, &status)))
        return(status);

    (void) ustr_inline_copy_strip(ustr, uchars_n, ustr_inline_from_h_ustr(&p_sle_info_block->h_ustr_inline));

    {
    const DOCNO docno = docno_from_p_docu(p_docu);

    if(status_ok(status = arglist_prepare(&arglist_handle, ss_args_string)))
    {
        const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 1);
        p_args[0].val.ustr = ustr;
        status = execute_command(p_docu, T5_CMD_NEW_EXPRESSION, &arglist_handle, OBJECT_ID_SS);
        arglist_dispose(&arglist_handle);
    }

    p_docu = p_docu_from_docno(docno); /* reload after command */
    } /*block*/

    quick_ublock_dispose(&quick_ublock);

    status_return(status);

    status_return(sle_giveup_focus(p_sle_info_block));

    status_return(ss_formula_reflect_contents(p_docu));

#if 0
    /* this isn't needed because of the focus change in giveup_focus */
    cur_change_after(p_docu);
#endif

    if(T5_EVENT_NONE == t5_message)
         return(STATUS_OK);

    return(object_skel(p_docu, t5_message, P_DATA_NONE));
}

T5_CMD_PROTO(static, ss_formula_eval_then_msg_for_message)
{
    UNREFERENCED_PARAMETER_InRef_(p_t5_cmd);

    switch(t5_message)
    {
    default: default_unhandled();
    case T5_CMD_RETURN:
        return(ss_formula_eval_then_msg(p_docu, T5_EVENT_NONE));

    case T5_CMD_TAB_LEFT:
        return(ss_formula_eval_then_msg(p_docu, T5_MSG_MOVE_LEFT_CELL));

    case T5_CMD_TAB_RIGHT:
        return(ss_formula_eval_then_msg(p_docu, T5_MSG_MOVE_RIGHT_CELL));

    case T5_CMD_CURSOR_UP:
        return(ss_formula_eval_then_msg(p_docu, T5_MSG_MOVE_UP_CELL));

    case T5_CMD_CURSOR_DOWN:
        return(ss_formula_eval_then_msg(p_docu, T5_MSG_MOVE_DOWN_CELL));
    }
}

_Check_return_
static STATUS
ss_formula_reject(
    _DocuRef_   P_DOCU p_docu)
{
    const P_SLE_INSTANCE_DATA p_sle_instance = p_object_instance_data_SLE(p_docu);
    const P_SLE_INFO_BLOCK p_sle_info_block = &p_sle_instance->ss_editor;

    status_return(sle_giveup_focus(p_sle_info_block));

    return(ss_formula_reflect_contents(p_docu));
}

/* Activate the specified SLE.  This code also performs selection stuff as required
 * the caret is placed at the end of the editing line.
 */

_Check_return_
static STATUS
sle_editline_activate(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  PC_QUICK_UBLOCK p_quick_ublock,
    _InVal_     BOOL select,
    _InVal_     BOOL psuedo_stuff)
{
    STATUS status = STATUS_OK;

    if(docno_ss_edit != DOCNO_NONE)
        status = STATUS_FAIL;
    else
    {
        if(OBJECT_ID_SLE != p_docu->focus_owner)
            /* check protection before activating the editing line */
            status = check_protection_simple(p_docu, FALSE);

        if(status_ok(status))
            status = sle_change_text(p_docu, p_quick_ublock, select, psuedo_stuff);

        if(status_ok(status))
        {
            const P_SLE_INSTANCE_DATA p_sle_instance = p_object_instance_data_SLE(p_docu);
            const P_SLE_INFO_BLOCK p_sle_info_block = &p_sle_instance->ss_editor;

            status_return(sle_grab_focus(p_sle_info_block));

            status_return(sle_index_compile(p_sle_info_block, TRUE, TRUE, TRUE));

            sle_display_caret(p_sle_info_block);
        }
    }

    return(status);
}

/* Process key events directed at the editing line.  These may
 * have direct impact on the way the widget works, including
 * special keys that will drop the focus owner.
 *
 * Key operations can also modify the status line in that they
 * will change the help information that is currently being shown.
 */

T5_CMD_PROTO(static, sle_key_command)
{
    const P_SLE_INSTANCE_DATA p_sle_instance = p_object_instance_data_SLE(p_docu);
    const P_SLE_INFO_BLOCK p_sle_info_block = &p_sle_instance->ss_editor;
    STATUS status = STATUS_OK;
    BOOL auto_highlight = TRUE;

    UNREFERENCED_PARAMETER_InRef_(p_t5_cmd);

    switch(t5_message)
    {
    case T5_CMD_DELETE_CHARACTER_LEFT:
        auto_highlight = FALSE;

        /*FALLTHRU*/

    case T5_CMD_DELETE_CHARACTER_RIGHT:
        if(p_sle_info_block->selection)
        {
            status = sle_selection_delete(p_sle_info_block);
            sle_refresh_field(p_sle_info_block);
        }
        else
            status = sle_delete_char(p_sle_info_block, (T5_CMD_DELETE_CHARACTER_RIGHT == t5_message));
        break;

    case T5_CMD_DELETE_LINE:
        p_sle_info_block->selection = TRUE;
        p_sle_info_block->selection_stt_index = 0;
        p_sle_info_block->selection_end_index = array_elements(&p_sle_info_block->h_ustr_inline) - 1 /*CH_NULL*/;
        status = sle_selection_delete(p_sle_info_block);
        sle_refresh_field(p_sle_info_block);
        break;

    case T5_CMD_DOCUMENT_BOTTOM:
        p_sle_info_block->selection = TRUE;
        p_sle_info_block->selection_stt_index = p_sle_info_block->caret_index;
        p_sle_info_block->selection_end_index = array_elements(&p_sle_info_block->h_ustr_inline) - 1 /*CH_NULL*/;
        status = sle_selection_delete(p_sle_info_block);
        sle_refresh_field(p_sle_info_block);
        break;

    case T5_CMD_DOCUMENT_TOP:
    case T5_CMD_LINE_START:
        sle_selection_clear(p_sle_info_block);
        p_sle_info_block->caret_index = 0;
        break;

    case T5_CMD_LINE_END:
        sle_selection_clear(p_sle_info_block);
        p_sle_info_block->caret_index = array_elements(&p_sle_info_block->h_ustr_inline) - 1 /*CH_NULL*/;
        break;

    default: default_unhandled();
#if CHECKING
    case T5_CMD_CURSOR_LEFT:
#endif
        if(p_sle_info_block->selection)
        {
            p_sle_info_block->caret_index = sle_selection_lhs_index(p_sle_info_block);
            auto_highlight = FALSE;
        }
        else if(p_sle_info_block->caret_index > 0)
        {
            PC_USTR_INLINE ustr_inline = ustr_inline_from_h_ustr(&p_sle_info_block->h_ustr_inline);
            S32 charcount = inline_b_bytecount_off(ustr_inline, p_sle_info_block->caret_index);
            p_sle_info_block->caret_index -= charcount;
        }

        sle_selection_clear(p_sle_info_block);
        break;

    case T5_CMD_CURSOR_RIGHT:
        if(p_sle_info_block->selection)
        {
            p_sle_info_block->caret_index = sle_selection_rhs_index(p_sle_info_block);
        }
        else if(p_sle_info_block->caret_index < array_elements(&p_sle_info_block->h_ustr_inline) - 1 /*CH_NULL*/)
        {
            PC_USTR_INLINE ustr_inline = ustr_inline_from_h_ustr(&p_sle_info_block->h_ustr_inline);
            S32 charcount = inline_bytecount_off(ustr_inline, p_sle_info_block->caret_index);
            p_sle_info_block->caret_index += charcount;
        }

        sle_selection_clear(p_sle_info_block);
        break;
    }

    if(status_ok(status))
        if(status_ok(status = sle_index_compile(p_sle_info_block, auto_highlight, FALSE, TRUE)))
            sle_display_caret(p_sle_info_block);

    return(status);
}

/* Paste a piece of text in to the specified SLE.  This text can then be activated with either
 * selection (pseudo if required).  The caret position is also updated, and the field refreshed.
 */

_Check_return_
static STATUS
sle_paste_text(
    P_SLE_INFO_BLOCK p_sle_info_block,
    _InoutRef_  PC_QUICK_UBLOCK p_quick_ublock,
    _InVal_     BOOL select,
    _InVal_     BOOL psuedo_select)
{
    const U32 size_add = quick_ublock_bytes(p_quick_ublock);
    S32 caret_index_on_entry = p_sle_info_block->selection ? sle_selection_lhs_index(p_sle_info_block) : p_sle_info_block->caret_index;
    BOOL refresh = FALSE;

    if(p_sle_info_block->selection)
    {
        status_return(sle_selection_delete(p_sle_info_block));
        refresh = TRUE;
    }

    if(0 != size_add)
    {
        STATUS status;
        P_U8 p_u8;
        
        if(NULL == (p_u8 = al_array_insert_before_U8(&p_sle_info_block->h_ustr_inline, size_add, &array_init_block_sle, &status, p_sle_info_block->caret_index)))
            return(status);

        memcpy32(p_u8, quick_ublock_uchars(p_quick_ublock), size_add);

        p_sle_info_block->caret_index += size_add;
    }

    if(refresh)
        sle_refresh_field(p_sle_info_block);

    if(select)
    {
        p_sle_info_block->caret_index = caret_index_on_entry;
        p_sle_info_block->selection = TRUE;
        p_sle_info_block->selection_stt_index = caret_index_on_entry;
        p_sle_info_block->selection_end_index = caret_index_on_entry + size_add;
        p_sle_info_block->pseudo_selection = psuedo_select;
    }
    else
    {
        /* position the caret on the first inline or at the end of the inserted string */
        S32 caret_index = caret_index_on_entry;
        S32 max_caret_index = caret_index_on_entry + size_add;
        PC_USTR_INLINE buffer = ustr_inline_from_h_ustr(&p_sle_info_block->h_ustr_inline);

        while(caret_index < max_caret_index)
        {
            S32 next = inline_bytecount_off(buffer, caret_index);

            if(next > 1)
                break;

            caret_index += next;
        }

        p_sle_info_block->caret_index = caret_index;
    }

    return(STATUS_OK);
}

/* Given an index into the string position, the caret position is
 * trimmed to the end of the SLE and the index information is recompiled.
 */

_Check_return_
static STATUS
sle_position_caret(
    P_SLE_INFO_BLOCK p_sle_info_block,
    _InVal_     S32 desired_caret_index)
{
    const S32 len = array_elements(&p_sle_info_block->h_ustr_inline) - 1 /*CH_NULL*/;
    const S32 limited_caret_index = MIN(desired_caret_index, len);

    p_sle_info_block->caret_index = limited_caret_index;

    sle_selection_clear(p_sle_info_block);

    status_return(sle_index_compile(p_sle_info_block, TRUE, FALSE, TRUE));

    sle_display_caret(p_sle_info_block);

    return(STATUS_OK);
}

/* Skip the selection within the editing line
 * Only relevant when the pseudo selection is active, it will force a refresh of the editing line.
 */

_Check_return_
static STATUS
sle_skip_selection(
    P_SLE_INFO_BLOCK p_sle_info_block)
{
    STATUS status = STATUS_OK;

    if(p_sle_info_block->selection && p_sle_info_block->pseudo_selection)
    {
        p_sle_info_block->selection = FALSE;
        p_sle_info_block->pseudo_selection = FALSE;
        p_sle_info_block->caret_index = sle_selection_rhs_index(p_sle_info_block);

        status_return(sle_index_compile(p_sle_info_block, TRUE, TRUE, TRUE));

        sle_display_caret(p_sle_info_block);
    }

    return(status);
}

/*
main events
*/

MAEVE_EVENT_PROTO(static, maeve_event_ob_sle)
{
    UNREFERENCED_PARAMETER(p_data);
    UNREFERENCED_PARAMETER_InRef_(p_maeve_block);

    switch(t5_message)
    {
    case T5_MSG_FOCUS_CHANGED:
    case T5_MSG_SELECTION_NEW:
        return(ss_backcontrols_enable(p_docu));

    default:
        return(STATUS_OK);
    }
}

/* Initialise the SLE block.  This code attempts to clear out the
 * instance data for the block.  It is called when the
 * document that owns the editor is created.
 *
 * It must also be called prior to any other activities with
 * the SLE as it initialises the memory being used.
 */

_Check_return_
extern STATUS
sle_create(
    _DocuRef_   P_DOCU p_docu)
{
    const P_SLE_INSTANCE_DATA p_sle_instance = p_object_instance_data_SLE(p_docu);
    const P_SLE_INFO_BLOCK p_sle_info_block = &p_sle_instance->ss_editor;
    U8 u8 = CH_NULL;

    zero_struct_ptr(p_sle_info_block);

    p_sle_info_block->docno = docno_from_p_docu(p_docu);

    /* Ensure there is a buffer with a single CH_NULL character in it! THIS CH_NULL MUST BE PRESERVED */
    return(al_array_add(&p_sle_info_block->h_ustr_inline, BYTE, 1, &array_init_block_sle, &u8));
}

extern HOST_WND
sle_view_create(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view)
{
    HOST_WND hwnd;

#if RISCOS
    /* we create icon when given a POSN_SET */
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_ViewRef_(p_view);
    hwnd = HOST_WND_NONE;
#elif WINDOWS
    U32 packed_id = viewid_pack(p_docu, p_view, EVENT_HANDLER_SLE);

    void_WrapOsBoolChecking(HOST_WND_NONE != (
    hwnd =
        CreateWindowEx(
            0, window_class[APP_WINDOW_CLASS_SLE], NULL,
            WS_CHILD /*| WS_BORDER*/ | WS_VISIBLE,
            0, 0, 32, 8,
            p_view->main[WIN_TOOLBAR].hwnd, NULL, GetInstanceHandle(), &packed_id)));
#endif

    return(hwnd);
}

extern void
sle_view_destroy(
    _ViewRef_   P_VIEW p_view)
{
#if RISCOS && !defined(RISCOS_SLE_ICON)
    UNREFERENCED_PARAMETER_ViewRef_(p_view);
#elif RISCOS && defined(RISCOS_SLE_ICON)
    if(* (wimp_i *) &p_view->main[WIN_SLE].hwnd != BAD_WIMP_I)
        void_WrapOsErrorReporting(wimp_dispose_icon(p_view->main[WIN_TOOLBAR].hwnd, (wimp_i *) &p_view->main[WIN_SLE].hwnd));
#elif WINDOWS
    if(HOST_WND_NONE != p_view->main[WIN_SLE].hwnd)
        DestroyWindow(p_view->main[WIN_SLE].hwnd);
#endif
}

_Check_return_
extern STATUS
sle_tool_user_posn_set(
    _DocuRef_   P_DOCU p_docu,
    P_ANY /*T5_TOOLBAR_TOOL_USER_POSN_SET*/ p_data)
{
    const PC_T5_TOOLBAR_TOOL_USER_POSN_SET p_t5_toolbar_tool_user_posn_set = (PC_T5_TOOLBAR_TOOL_USER_POSN_SET) p_data;
    const PC_PIXIT_RECT p_pixit_rect = &p_t5_toolbar_tool_user_posn_set->pixit_rect;
    const P_VIEW p_view = p_view_from_viewno(p_docu, p_t5_toolbar_tool_user_posn_set->viewno);
    const P_SLE_INSTANCE_DATA p_sle_instance = p_object_instance_data_SLE(p_docu);
    const P_SLE_INFO_BLOCK p_sle_info_block = &p_sle_instance->ss_editor;
    VIEWNO viewno = p_sle_info_block->prefered_viewno;
    STATUS status;

    p_sle_info_block->pixit_rect = *p_pixit_rect;

#if WINDOWS
    p_sle_info_block->pixit_rect.br.x -= RIGHT_INDENT;
#endif

#if RISCOS && !defined(RISCOS_SLE_ICON)
    /* NB Here the Back window hosts the toolbar NOT in an icon */
#elif RISCOS && defined(RISCOS_SLE_ICON)
    void_WrapOsErrorReporting(wimp_dispose_icon(p_view->main[WIN_BACK].hwnd, (wimp_i *) &p_view->main[WIN_SLE].hwnd));

    {
    int width = (int) ((p_sle_info_block->pixit_rect.br.x - p_sle_info_block->pixit_rect.tl.x) / PIXITS_PER_RISCOS);
    WimpCreateIconBlockWithBitset icreate;
    zero_struct(icreate);
    icreate.window_handle = p_view->main[WIN_BACK].hwnd;
    icreate.icon.bbox.xmin = + (int) (p_sle_info_block->pixit_rect.tl.x / PIXITS_PER_RISCOS);
    icreate.icon.bbox.ymax = - (int) (p_sle_info_block->pixit_rect.tl.y / PIXITS_PER_RISCOS);
    icreate.icon.bbox.xmax = icreate.icon.bbox.xmin + width;
    icreate.icon.bbox.ymin = icreate.icon.bbox.ymax - 48;
    icreate.icon.flags.text = 1;
    icreate.icon.flags.redraw = 1;
    icreate.icon.flags.button_type = ButtonType_DoubleClickDrag;
    (void) strcpy(icreate.icon.data.t, "PinkDuckMan");
    if(icreate.icon.bbox.xmax > icreate.icon.bbox.xmin)
        void_WrapOsErrorReporting(wimp_create_icon_with_bitset(&icreate, (wimp_i *) &p_view->main[WIN_SLE].hwnd));
    } /*block*/
#elif WINDOWS
    {
    RECT client_rect, invalidate_rect, new_pos_rect;
    GetClientRect(p_view->main[WIN_SLE].hwnd, &client_rect);

    { /* convert to dpi-dependent pixels */
    SIZE PixelsPerInch;
    host_get_pixel_size(NULL /*screen*/, &PixelsPerInch); /* Get current pixel size for the screen e.g. 96 or 120 */

    new_pos_rect.left   = (int) idiv_floor_fn(p_sle_info_block->pixit_rect.tl.x * PixelsPerInch.cx, PIXITS_PER_INCH);
    new_pos_rect.top    = (int) idiv_floor_fn(p_sle_info_block->pixit_rect.tl.y * PixelsPerInch.cy, PIXITS_PER_INCH);
    new_pos_rect.right  = (int) idiv_ceil_fn( p_sle_info_block->pixit_rect.br.x * PixelsPerInch.cx, PIXITS_PER_INCH);
    new_pos_rect.bottom = (int) idiv_ceil_fn( p_sle_info_block->pixit_rect.br.y * PixelsPerInch.cy, PIXITS_PER_INCH);
    } /*block*/

    SetWindowPos(p_view->main[WIN_SLE].hwnd, NULL, new_pos_rect.left, new_pos_rect.top, new_pos_rect.right - new_pos_rect.left, new_pos_rect.bottom - new_pos_rect.top, SWP_NOZORDER);
    /*reportf(TEXT("SLE SWP %d %d %d %d"), new_pos_rect.left, new_pos_rect.top, new_pos_rect.right, new_pos_rect.bottom);*/

    invalidate_rect = client_rect;
    invalidate_rect.left = invalidate_rect.right - 2; /* have to redraw right-hand side when shrinking to left */
    if(invalidate_rect.right < (new_pos_rect.right - new_pos_rect.left)) /* have to redraw right-hand side when growing to right */
        invalidate_rect.right = (new_pos_rect.right - new_pos_rect.left);
    InvalidateRect(p_view->main[WIN_SLE].hwnd, &invalidate_rect, TRUE);
    /*reportf(TEXT("SLE IVR %d %d %d %d"), invalidate_rect.left, invalidate_rect.top, invalidate_rect.right, invalidate_rect.bottom);*/
    } /*block*/
#endif /* OS */

    p_sle_info_block->prefered_viewno = viewno_from_p_view(p_view);

    if(status_ok(status = sle_index_compile(&p_sle_instance->ss_editor, FALSE, FALSE, TRUE)))
        sle_display_caret(&p_sle_instance->ss_editor);

    p_sle_info_block->prefered_viewno = viewno;

    return(status);
}

static STYLE_SELECTOR _sle_style_selector; /* const after T5_MSG_IC__STARTUP */

_Check_return_
_Ret_valid_
static inline PC_STYLE_SELECTOR
p_sle_style_selector(void)
{
    return(&_sle_style_selector);
}

static void
sle_style_startup(void)
{
    style_selector_copy(&_sle_style_selector, &style_selector_font_spec);
    style_selector_bit_set(&_sle_style_selector, STYLE_SW_PS_RGB_BACK);
    style_selector_bit_set(&_sle_style_selector, STYLE_SW_PS_RGB_BORDER);
    style_selector_bit_set(&_sle_style_selector, STYLE_SW_PS_BORDER);
}

static BOOL  recache_sle_style = TRUE;

static STYLE _sle_style;

_Check_return_
_Ret_valid_
static PC_STYLE
sle_style_cache_do(
    _DocuRef_   PC_DOCU p_docu_config,
    _OutRef_    P_STYLE p_style_out)
{
    const PC_STYLE_SELECTOR p_desired_style_selector = p_sle_style_selector();
    STYLE_HANDLE style_handle;
    STYLE style;

    CHECKING_ONLY(zero_struct(style)); /* sure makes it easier to debug! */
    style_init(&style);

    if(0 != (style_handle = style_handle_from_name(p_docu_config, STYLE_NAME_UI_FORMULA_LINE)))
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
sle_style_cache(
    _InoutRef_  P_STYLE p_style_ensured)
{
    if(!recache_sle_style)
        return(p_style_ensured);

    recache_sle_style = FALSE;

    return(sle_style_cache_do(p_docu_from_config(), p_style_ensured));
}

_Check_return_
_Ret_valid_
static inline PC_STYLE
p_style_for_sle(void)
{
    return(sle_style_cache(&_sle_style));
}

#if WINDOWS

static HFONT
sle_select_font(
    _HdcRef_    HDC hdc,
    _OutRef_    P_BOOL p_delete_font,
    _InRef_     PC_STYLE p_sle_style)
{
    HOST_FONT host_font;
    HOST_FONT_SPEC host_font_spec;

    status_assert(fontmap_host_font_spec_from_font_spec(&host_font_spec, &p_sle_style->font_spec, FALSE));

    { /* Em = dpiY * point_size / 72 from MSDN */ /* convert to dpi-dependent pixels */ /* DPI-aware */
    SIZE PixelsPerInch;
    host_get_pixel_size(NULL /*screen*/, &PixelsPerInch); /* Get current pixel size for the screen e.g. 96 or 120 */

    host_font_spec.size_y = idiv_floor_u(host_font_spec.size_y * PixelsPerInch.cy, PIXITS_PER_INCH);
    } /*block*/

    host_font = host_font_find(&host_font_spec, P_REDRAW_CONTEXT_NONE);
    host_font_spec_dispose(&host_font_spec);

    if(HOST_FONT_NONE != host_font)
    {
        *p_delete_font = TRUE;
        return(SelectFont(hdc, host_font));
    }

    /* SKS ensure some font selected in the DC */
    *p_delete_font = FALSE;
    return(SelectFont(hdc, GetStockObject(ANSI_VAR_FONT)));
}

static void
sle_onCreate(
    _HwndRef_   HWND hwnd,
    _ViewRef_   P_VIEW p_view)
{
    /* validate immediately */
    p_view->main[WIN_SLE].hwnd = hwnd;
}

_Check_return_
static BOOL
wndproc_sle_onCreate(
    _HwndRef_   HWND hwnd,
    _InRef_     LPCREATESTRUCT lpCreateStruct)
{
    DOCNO docno;
    P_VIEW p_view;
    EVENT_HANDLER event_handler;
    PC_U32 p_u32 = (PC_U32) lpCreateStruct->lpCreateParams;
    U32 packed_id = *p_u32;

    SetProp(hwnd, TYPE5_PROPERTY_WORD, (HANDLE) (UINT_PTR) packed_id);

    if(DOCNO_NONE != (docno = resolve_hwnd(hwnd, &p_view, &event_handler)))
    {
        sle_onCreate(hwnd, p_view);
        return(TRUE);
    }

    return(FALSE);
}

static void
sle_onDestroy(
    _HwndRef_   HWND hwnd,
    _ViewRef_   P_VIEW p_view)
{
    assert(p_view->main[WIN_SLE].hwnd == hwnd);
    UNREFERENCED_PARAMETER_HwndRef_(hwnd);
    p_view->main[WIN_SLE].hwnd = NULL;
}

static void
wndproc_sle_onDestroy(
    _HwndRef_   HWND hwnd)
{
    DOCNO docno;
    P_VIEW p_view;
    EVENT_HANDLER event_handler;

    RemoveProp(hwnd, TYPE5_PROPERTY_WORD);

    if(DOCNO_NONE != (docno = resolve_hwnd(hwnd, &p_view, &event_handler)))
    {
        sle_onDestroy(hwnd, p_view);
        return;
    }

    FORWARD_WM_DESTROY(hwnd, DefWindowProc);
}

_Check_return_
static inline COLORREF
simple_colorref_from_rgb(
    _InRef_     PC_RGB p_rgb)
{
    return(RGB(p_rgb->r, p_rgb->g, p_rgb->b));
}

static void
sle_draw_empty_frame(
    _HdcRef_    HDC hdc,
    _InRef_     PCRECT p_client_rect,
    _OutRef_    PRECT p_core_rect,
    HBRUSH h_brush_fill,
    HBRUSH h_brush_stroke)
{
    RECT rect;

    if(NULL == h_brush_stroke)
    {
        *p_core_rect = *p_client_rect;
        FillRect(hdc, p_core_rect, h_brush_fill);
        return;
    }

    rect = *p_client_rect; rect.left += 1; rect.top += 1; rect.right -= 1; rect.bottom -= 1;
    *p_core_rect = rect;
    FillRect(hdc, &rect, h_brush_fill); /* core */

    rect = *p_client_rect; rect.bottom = rect.top + 1; FillRect(hdc, &rect, h_brush_stroke); /* top */
    rect = *p_client_rect; rect.top = rect.bottom - 1; FillRect(hdc, &rect, h_brush_stroke); /* bottom */

    rect = *p_client_rect; rect.right = rect.left + 1; rect.top += 1; rect.bottom -= 1; FillRect(hdc, &rect, h_brush_stroke); /* left */
    rect = *p_client_rect; rect.left = rect.right - 1; rect.top += 1; rect.bottom -= 1; FillRect(hdc, &rect, h_brush_stroke); /* right */
}

static void
sle_onPaint(
    _HwndRef_   HWND hwnd,
    _InRef_     PPAINTSTRUCT p_paintstruct,
    _DocuRef_   P_DOCU p_docu)
{
    const HDC hdc = p_paintstruct->hdc;
    const P_SLE_INSTANCE_DATA p_sle_instance = p_object_instance_data_SLE(p_docu);
    const P_SLE_INFO_BLOCK p_sle_info_block = &p_sle_instance->ss_editor;
    const PC_STYLE p_sle_style = p_style_for_sle();
    RECT client_rect;
    RECT core_rect;

    /* rect to be painted is paintstruct.rcPaint */

    GetClientRect(hwnd, &client_rect);
    /*reportf(TEXT("SLE RCP %d %d %d %d"), p_paintstruct->rcPaint.left, p_paintstruct->rcPaint.top, p_paintstruct->rcPaint.right, p_paintstruct->rcPaint.bottom);*/
    /*reportf(TEXT("SLE GCR %d %d %d %d"), client_rect.left, client_rect.top, client_rect.right, client_rect.bottom);*/

    {
    HBRUSH h_brush_fill = NULL;
    HBRUSH h_brush_stroke = NULL;
    BOOL border_specified = FALSE;

    if(style_bit_test(p_sle_style, STYLE_SW_PS_RGB_BACK))
        h_brush_fill = CreateSolidBrush(simple_colorref_from_rgb(&p_sle_style->para_style.rgb_back));

    if(style_bit_test(p_sle_style, STYLE_SW_PS_BORDER))
    {
        border_specified = TRUE;
        if((0 != p_sle_style->para_style.border) && style_bit_test(p_sle_style, STYLE_SW_PS_RGB_BORDER))
            h_brush_stroke = CreateSolidBrush(simple_colorref_from_rgb(&p_sle_style->para_style.rgb_border));
        /* else we are specifying that we do not want a border, so h_brush_stroke remains NULL */
    }

    sle_draw_empty_frame(hdc, &client_rect, &core_rect,
                         h_brush_fill ? h_brush_fill : GetStockBrush(WHITE_BRUSH),
                         border_specified ? h_brush_stroke : GetStockBrush(BLACK_BRUSH) /* see above */);

    if(h_brush_fill)
        DeleteBrush(h_brush_fill);
    if(h_brush_stroke)
        DeleteBrush(h_brush_stroke);
    } /*block*/

    if(array_elements(&p_sle_info_block->h_ustr_inline) > 1 /*CH_NULL*/)
    {
        BOOL font_needs_delete;
        HFONT h_font_old = sle_select_font(hdc, &font_needs_delete, p_sle_style);
        PC_USTR_INLINE ustr_inline = ustr_inline_from_h_ustr(&p_sle_info_block->h_ustr_inline);
        U32 offset_stt = 0;
        U32 offset_cur = 0;
        SIZE size;
        RECT invert_rect;
        POINT point;

        SetBkMode(hdc, TRANSPARENT);

        if(style_bit_test(p_sle_style, STYLE_SW_FS_COLOUR))
            SetTextColor(hdc, simple_colorref_from_rgb(&p_sle_style->font_spec.colour));
        else
            SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));

        SetTextAlign(hdc, TA_LEFT | TA_TOP | TA_UPDATECP);

        {
        TCHARZ tstr_sizing[] = TEXT("$y") TEXT("\xC2"); /* UCH_LATIN_CAPITAL_LETTER_A_WITH_CIRCUMFLEX is generally the tallest Latin-1 character */
        void_WrapOsBoolChecking(
            GetTextExtentPoint32(hdc, tstr_sizing, tstrlen32(tstr_sizing), &size));
        } /*block*/

        invert_rect = core_rect;
        invert_rect.left  = MIN(p_sle_info_block->display.selection_stt_pixels, p_sle_info_block->display.selection_end_pixels);
        invert_rect.right = MAX(p_sle_info_block->display.selection_stt_pixels, p_sle_info_block->display.selection_end_pixels);

        if(invert_rect.left == TEXT_X)
            invert_rect.left -= (TEXT_X - 1); /* invert all the way to the left-hand side for prettiness */

        invert_rect.left  -= p_sle_info_block->display.scroll_offset_pixels;
        invert_rect.right -= p_sle_info_block->display.scroll_offset_pixels;

        if(p_sle_info_block->selection && p_sle_info_block->pseudo_selection)
            FillRect(hdc, &invert_rect, GetStockBrush(LTGRAY_BRUSH));

        point.x = TEXT_X - p_sle_info_block->display.scroll_offset_pixels;
        point.y = ((core_rect.bottom - core_rect.top) - size.cy) /2;
        MoveToEx(hdc, point.x, point.y, NULL);

        for(;;)
        {
            if(is_inline_off(ustr_inline, offset_cur))
            {
                switch(inline_code_off(ustr_inline, offset_cur))
                {
                default: default_unhandled();
                    offset_cur += inline_bytecount_off(ustr_inline, offset_cur);
                    break;

                case IL_SLE_ARGUMENT:
                    if(offset_cur > offset_stt) /* flush up to current point */
                        status_assert(uchars_ExtTextOut(hdc, 0, 0, 0, NULL, uchars_AddBytes(ustr_inline, offset_stt), offset_cur - offset_stt, NULL));

                    { /* output body of inline */
                    PC_USTR ustr_arg = il_sle_argument_data_off(PC_USTR, ustr_inline, offset_cur, IL_SLE_ARGUMENT_OFF_ARG_USTR);
                    int len = ustrlen32(ustr_arg);
                    if(len)
                        status_assert(uchars_ExtTextOut(hdc, 0, 0, 0, NULL, ustr_arg, len, NULL));
                    } /*block*/

                    offset_cur += inline_bytecount_off(ustr_inline, offset_cur);
                    offset_stt = offset_cur;
                    break;
                }
            }
            else
            {
                if(CH_NULL == PtrGetByteOff(ustr_inline, offset_cur))
                    break;

                ++offset_cur; /* no need to worry about bytes_of_char as we're counting a span of bytes to output later */
            }
        }

        if(offset_cur > offset_stt) /* flush end segment */
            status_assert(uchars_ExtTextOut(hdc, 0, 0, 0, NULL, uchars_AddBytes(ustr_inline, offset_stt), offset_cur - offset_stt, NULL));

        if(p_sle_info_block->selection && !p_sle_info_block->pseudo_selection)
            InvertRect(hdc, &invert_rect);

        if(font_needs_delete)
            DeleteFont(SelectFont(hdc, h_font_old));
    }
}

static void
wndproc_sle_onPaint(
    _HwndRef_   HWND hwnd)
{
    DOCNO docno;
    P_VIEW p_view;
    EVENT_HANDLER event_handler;
    PAINTSTRUCT paintstruct;

    if(!BeginPaint(hwnd, &paintstruct))
        return;

    hard_assert(TRUE);

    if(DOCNO_NONE != (docno = resolve_hwnd(hwnd, &p_view, &event_handler)))
    {
        sle_onPaint(hwnd, &paintstruct, p_docu_from_docno_valid(docno));
    }

    hard_assert(FALSE);

    EndPaint(hwnd, &paintstruct);
}

static void
sle_onLButtonDown(
    _HwndRef_   HWND hwnd,
    _InVal_     BOOL fDoubleClick,
    _InVal_     int x,
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view)
{
    const P_SLE_INSTANCE_DATA p_sle_instance = p_object_instance_data_SLE(p_docu);
    const P_SLE_INFO_BLOCK p_sle_info_block = &p_sle_instance->ss_editor;
    STATUS status = STATUS_OK;

    if(p_docu->mark_info_cells.h_markers) /* SKS after 1.08b2 15jul94 make consistent with F2 EditFormula */
    {
        host_bleep();
        return;
    }

    sle_view_prefer(p_docu, p_view);

    for(;;) /* loop for structure */
    {
        if(!p_sle_info_block->has_focus)
            status_break(status = object_call_id(OBJECT_ID_SLE, p_docu, T5_CMD_SS_EDIT_FORMULA, P_DATA_NONE));

        if(fDoubleClick)
        {
            p_sle_info_block->selection = TRUE;
            p_sle_info_block->pseudo_selection = FALSE;
            p_sle_info_block->selection_stt_index = 0;
            p_sle_info_block->selection_end_index = array_elements(&p_sle_info_block->h_ustr_inline) - 1 /*CH_NULL*/;

            status_break(status = sle_index_compile(p_sle_info_block, FALSE, TRUE, FALSE));

            sle_display_caret(p_sle_info_block);
        }
        else
        {
            S32 caret_index;

            if(OBJECT_ID_SLE != p_docu->focus_owner)
                status_break(status = check_protection_simple(p_docu, FALSE)); /* check protection before activating the editing line */

            caret_index = sle_index_from_pixel_x(p_sle_info_block, hwnd, MAX((p_sle_info_block->display.scroll_offset_pixels + x) - TEXT_X, 0));

            if(p_sle_info_block->selection)
                sle_selection_clear(p_sle_info_block);

            p_sle_info_block->caret_index = caret_index;
            p_sle_info_block->selection_stt_index = caret_index;

            status = sle_grab_focus(p_sle_info_block);
        }

        break; /* end of loop for structure */
        /*NOTREACHED*/
    }

    if(status_fail(status))
    {
        reperr_null(status);
        return;
    }

    SetCapture(hwnd);
    p_sle_info_block->captured = TRUE;
}

static void
wndproc_sle_onLButtonDown(
    _HwndRef_   HWND hwnd,
    _InVal_     BOOL fDoubleClick,
    _InVal_     int x,
    _InVal_     int y,
    _InVal_     UINT keyFlags)
{
    DOCNO docno;
    P_VIEW p_view;
    EVENT_HANDLER event_handler;

    if(DOCNO_NONE != (docno = resolve_hwnd(hwnd, &p_view, &event_handler)))
    {
        if((docno_ss_edit == DOCNO_NONE) || (docno_ss_edit == docno))
        {
            sle_onLButtonDown(hwnd, fDoubleClick, x, p_docu_from_docno_valid(docno), p_view);
            return;
        }
    }

    FORWARD_WM_LBUTTONDOWN(hwnd, fDoubleClick, x, y, keyFlags, DefWindowProc);
}

static void
sle_onLButtonUp(
    _DocuRef_   P_DOCU p_docu)
{
    const P_SLE_INSTANCE_DATA p_sle_instance = p_object_instance_data_SLE(p_docu);
    const P_SLE_INFO_BLOCK p_sle_info_block = &p_sle_instance->ss_editor;

    if(p_sle_info_block->captured)
    {
        p_sle_info_block->captured = FALSE;
        ReleaseCapture();
    }
}

static void
wndproc_sle_onLButtonUp(
    _HwndRef_   HWND hwnd,
    _InVal_     int x,
    _InVal_     int y,
    _InVal_     UINT keyFlags)
{
    DOCNO docno;
    P_VIEW p_view;
    EVENT_HANDLER event_handler;

    if(DOCNO_NONE != (docno = resolve_hwnd(hwnd, &p_view, &event_handler)))
    {
        if((docno_ss_edit == DOCNO_NONE) || (docno_ss_edit == docno))
        {
            sle_onLButtonUp(p_docu_from_docno_valid(docno));
            return;
        }
    }

    FORWARD_WM_LBUTTONUP(hwnd, x, y, keyFlags, DefWindowProc);
}

static void
wndproc_sle_onMouseLeave(
    _HwndRef_   HWND hwnd)
{
    DOCNO docno;
    P_VIEW p_view;
    EVENT_HANDLER event_handler;

    host_clear_tracking_for_window(hwnd);

    if(DOCNO_NONE != (docno = resolve_hwnd(hwnd, &p_view, &event_handler)))
    {
        trace_0(TRACE_APP_CLICK, TEXT("proc_event_sle T5_EVENT_POINTER_LEAVES_WINDOW"));
        status_line_clear(p_docu_from_docno_valid(docno), STATUS_LINE_LEVEL_BACKWINDOW_CONTROLS);
        return;
    }

    /* no FORWARD_WM_MOUSELEAVE() */
}

static void
sle_onMouseMove(
    _HwndRef_   HWND hwnd,
    _InVal_     int x,
    _DocuRef_   P_DOCU p_docu)
{
    if(host_set_tracking_for_window(hwnd))
    {
        trace_0(TRACE_APP_CLICK, TEXT("proc_event_sle T5_EVENT_POINTER_ENTERS_WINDOW"));
        status_line_clear(p_docu, STATUS_LINE_LEVEL_BACKWINDOW_CONTROLS);
    }

    if((docno_ss_edit != DOCNO_NONE) && (docno_ss_edit != docno_from_p_docu(p_docu)))
        return;

    {
    const P_SLE_INSTANCE_DATA p_sle_instance = p_object_instance_data_SLE(p_docu);
    const P_SLE_INFO_BLOCK p_sle_info_block = &p_sle_instance->ss_editor;

    if(p_sle_info_block->captured)
    {
        HWND hwnd_capture = GetCapture();

        if(HOST_WND_NONE != hwnd_capture)
        {
            p_sle_info_block->pseudo_selection = FALSE;
            p_sle_info_block->selection = TRUE;

            p_sle_info_block->selection_end_index = sle_index_from_pixel_x(p_sle_info_block, hwnd_capture, MAX((p_sle_info_block->display.scroll_offset_pixels + x) - TEXT_X, 0));

            if(status_ok(status_wrap(sle_index_compile(p_sle_info_block, TRUE, TRUE, FALSE))))
                sle_display_caret(p_sle_info_block);
        }
    }

    {
    UI_TEXT ui_text;
    ui_text.type = UI_TEXT_TYPE_RESID;
    ui_text.text.resource_id = SS_MSG_STATUS_FORMULA_LINE;
    status_line_set(p_docu, STATUS_LINE_LEVEL_BACKWINDOW_CONTROLS, &ui_text);
    } /*block*/
    } /*block*/
}

static void
wndproc_sle_onMouseMove(
    _HwndRef_   HWND hwnd,
    _InVal_     int x,
    _InVal_     int y,
    _InVal_     UINT keyFlags)
{
    DOCNO docno;
    P_VIEW p_view;
    EVENT_HANDLER event_handler;

    if(DOCNO_NONE != (docno = resolve_hwnd(hwnd, &p_view, &event_handler)))
    {
        sle_onMouseMove(hwnd, x, p_docu_from_docno_valid(docno));
        return;
    }

    FORWARD_WM_MOUSEMOVE(hwnd, x, y, keyFlags, DefWindowProc);
}

extern LRESULT CALLBACK
wndproc_sle(
    _HwndRef_   HWND hwnd,
    _In_        UINT message,
    _In_        WPARAM wParam,
    _In_        LPARAM lParam)
{
    switch(message)
    {
    HANDLE_MSG(hwnd, WM_CREATE,         wndproc_sle_onCreate);
    HANDLE_MSG(hwnd, WM_DESTROY,        wndproc_sle_onDestroy);
    HANDLE_MSG(hwnd, WM_PAINT,          wndproc_sle_onPaint);
    HANDLE_MSG(hwnd, WM_LBUTTONDOWN,    wndproc_sle_onLButtonDown);
    HANDLE_MSG(hwnd, WM_LBUTTONDBLCLK,  wndproc_sle_onLButtonDown);
    HANDLE_MSG(hwnd, WM_LBUTTONUP,      wndproc_sle_onLButtonUp);
    HANDLE_MSG(hwnd, WM_MOUSELEAVE,     wndproc_sle_onMouseLeave);
    HANDLE_MSG(hwnd, WM_MOUSEMOVE,      wndproc_sle_onMouseMove);

    default:
        return(DefWindowProc(hwnd, message, wParam, lParam));
    }
}

_Check_return_
static STATUS
sle_register_wndclass(void)
{
    if(NULL == g_hInstancePrev)
    {   /* register SLE window class */
        WNDCLASS wndclass;
        zero_struct(wndclass);
        wndclass.style = CS_DBLCLKS;
        wndclass.lpfnWndProc = (WNDPROC) wndproc_sle;
      /*wndclass.cbClsExtra = 0;*/
      /*wndclass.cbWndExtra = 0;*/
        wndclass.hInstance = GetInstanceHandle();
        wndclass.hCursor = LoadCursor(NULL, IDC_IBEAM);
        wndclass.hbrBackground = GetStockBrush(HOLLOW_BRUSH);
      /*wndclass.lpszMenuName = NULL;*/
        wndclass.lpszClassName = window_class[APP_WINDOW_CLASS_SLE];
        if(!WrapOsBoolChecking(RegisterClass(&wndclass)))
            return(status_nomem());
    }

    return(STATUS_OK);
}

/* Given a point within the SLE convert it to a text position.
 * Called as part of the editing code to handle the processing of clicks.
 */

static S32
sle_index_from_pixel_x(
    P_SLE_INFO_BLOCK p_sle_info_block,
    _HwndRef_   HWND hwnd,
    _In_        int x)
{
    S32 index = 0;

    x = MIN(x, p_sle_info_block->display.string_end_pixels);

    if(array_elements(&p_sle_info_block->h_ustr_inline) > 1 /*CH_NULL*/)
    {
        STATUS status = STATUS_OK;
        QUICK_TBLOCK_WITH_BUFFER(quick_tblock, elemof32("a.quick.buf.to.hold.text"));
        quick_tblock_with_buffer_setup(quick_tblock);

        { /* copy to on-screen representation: failure mostly irrelevant */
        PC_USTR_INLINE ustr_inline = ustr_inline_from_h_ustr(&p_sle_info_block->h_ustr_inline);
        U32 offset = 0;

        while(status_ok(status))
        {
            if(is_inline_off(ustr_inline, offset))
            {
                if(IL_SLE_ARGUMENT == inline_code_off(ustr_inline, offset))
                    status = quick_tblock_ustr_add(&quick_tblock, il_sle_argument_data_off(PC_USTR, ustr_inline, offset, IL_SLE_ARGUMENT_OFF_ARG_USTR));

                offset += inline_bytecount_off(ustr_inline, offset);
            }
            else
            {
                U32 bytes_of_char;
                UCS4 ucs4 = ustr_char_decode_off((PC_USTR) ustr_inline, offset, /*ref*/bytes_of_char);

                if(CH_NULL == ucs4)
                    break;

                status = quick_tblock_ucs4_add(&quick_tblock, ucs4);

                offset += bytes_of_char;
            }
        }
        } /*block*/

        {
        const HDC hdc = GetDC(hwnd);
        if(NULL != hdc)
        {
            const PC_STYLE p_sle_style = p_style_for_sle();
            BOOL font_needs_delete;
            HFONT h_font_old = sle_select_font(hdc, &font_needs_delete, p_sle_style);
            PCTSTR tstr = quick_tblock_tchars(&quick_tblock);
            S32 end = (S32) quick_tblock_chars(&quick_tblock);
            index = sle_search_string(hdc, tstr, 0, end, x);
            void_WrapOsBoolChecking(1 == ReleaseDC(hwnd, hdc));
            if(font_needs_delete)
                DeleteFont(SelectFont(hdc, h_font_old));
        }
        } /*block*/

        quick_tblock_dispose(&quick_tblock);
    }

    {
    S32 real_index = 0;
    S32 display_index = 0;

    if(array_elements(&p_sle_info_block->h_ustr_inline) > 1 /*CH_NULL*/)
    {
        PC_USTR_INLINE ustr_inline = ustr_inline_from_h_ustr(&p_sle_info_block->h_ustr_inline);

        while(display_index < index)
        {
            U32 bytes_of_thing;

            if(is_inline_off(ustr_inline, real_index))
            {
                bytes_of_thing = inline_bytecount_off(ustr_inline, real_index);

                if(IL_SLE_ARGUMENT == inline_code_off(ustr_inline, real_index))
                {
                    display_index += ustrlen32(il_sle_argument_data_off(PC_USTR, ustr_inline, real_index, IL_SLE_ARGUMENT_OFF_ARG_USTR));

                    if(display_index > index)
                        break;
                }
            }
            else
            {
                UCS4 ucs4 = ustr_char_decode_off((PC_USTR) ustr_inline, real_index, /*ref*/bytes_of_thing);

                if(CH_NULL == ucs4)
                    break;

                ++display_index;
            }

            real_index += bytes_of_thing;
        }
    }

    return(real_index);
    } /*block*/
}

#if 1

static S32
sle_search_string(
    _HdcRef_    HDC hdc,
    _In_reads_(end) PCTSTR tstr,
    _InVal_     S32 stt,
    _InVal_     S32 end,
    _In_        int x)
{
    int nMaxExtent = x;
    int nFit = 0;
    SIZE size;

    assert(0 == stt);
    UNREFERENCED_PARAMETER_InVal_(stt);

    if(WrapOsBoolChecking(GetTextExtentExPoint(hdc, tstr, end, nMaxExtent, &nFit, NULL /*pDx*/, &size)))
    {
        return(nFit);
    }

    return(0);
}

#else

/* This is a recursive binary search function that will find the character offset defined by the point specified */

static S32
sle_search_string(
    _HdcRef_    HDC hdc,
    _In_reads_(end) PCTSTR tstr,
    _InVal_     S32 stt,
    _InVal_     S32 end,
    _In_        int x)
{
    S32 mid;
    SIZE size;

    trace_3(TRACE_WINDOWS_HOST, TEXT("sle_search_string: tstr ") PTR_XTFMT TEXT(", stt ") S32_TFMT TEXT(", end ") S32_TFMT, tstr, stt, end);

    if(stt >= end)
        return(stt);

    mid = (stt + end) / 2;

    void_WrapOsBoolChecking(
        GetTextExtentPoint32(hdc, tstr, (int) mid, &size));

    if(x > size.cx)
        return(sle_search_string(hdc, tstr, mid + 1, end, x));

    if(x < size.cx)
        return(sle_search_string(hdc, tstr, stt, mid, x));

    return(mid);
}

#endif

#endif /* OS */

/* Change the text within the SLE.
 * Allows you to put text into the buffer and it will erase the entire contents of the SLE in one go.
 * The code makes an internal selection.  And given flags can perform psuedo and normal selection
 */

_Check_return_
extern STATUS
sle_change_text(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  PC_QUICK_UBLOCK p_quick_ublock,
    _InVal_     BOOL select /* MRJC 17.1.95: this is never TRUE */,
    _InVal_     BOOL psuedo_stuff)
{
    STATUS status = STATUS_OK;
    const P_SLE_INSTANCE_DATA p_sle_instance = p_object_instance_data_SLE(p_docu);
    const P_SLE_INFO_BLOCK p_sle_info_block = &p_sle_instance->ss_editor;
    U32 size_new = quick_ublock_bytes(p_quick_ublock); /*not CH_NULL-terminated*/
    U32 size_old = array_elements32(&p_sle_info_block->h_ustr_inline) - 1 /*CH_NULL*/;
    BOOL refresh = FALSE;

    /* SKS 12dec94 optimization for setting up same text to reduce flickerama */
    if((size_new != size_old) || (size_old && (0 != memcmp32(quick_ublock_uchars(p_quick_ublock), ustr_inline_from_h_ustr(&p_sle_info_block->h_ustr_inline), size_old))))
    {
        refresh = TRUE;

        p_sle_info_block->selection = TRUE;
        p_sle_info_block->selection_stt_index = 0;
        p_sle_info_block->selection_end_index = size_old;

        if(0 != quick_ublock_bytes(p_quick_ublock))
            status = sle_paste_text(&p_sle_instance->ss_editor, p_quick_ublock, select, psuedo_stuff);
        else if(p_sle_info_block->selection)
            status = sle_selection_delete(&p_sle_instance->ss_editor);
    }

#if 0 /* MRJC 17.1.95 caret is set up by sle_paste_text and sle_delete_selection */
    if(!select)
        p_sle_info_block->caret = size_new;
#endif

    status_return(status);

    return(sle_index_compile(&p_sle_instance->ss_editor, TRUE, refresh, TRUE));
}

/* Grab input focus for the SLE.  This can be called at any point
 * and with notice when the SLE is currently the owner and ignore
 * the request.
 */

_Check_return_
static STATUS
sle_grab_focus(
    P_SLE_INFO_BLOCK p_sle_info_block)
{
    if(!p_sle_info_block->has_focus)
        status_return(ss_edit_focus_claim(p_docu_from_docno(p_sle_info_block->docno)));

    status_return(sle_index_compile(p_sle_info_block, TRUE, FALSE, TRUE));

    sle_display_caret(p_sle_info_block);

    return(STATUS_OK);
}

/* Remove the currently active selection within the SLE forcing a redraw as required */

static void
sle_selection_clear(
    P_SLE_INFO_BLOCK p_sle_info_block)
{
    if(!p_sle_info_block->selection)
        return;

    p_sle_info_block->selection = FALSE;
    p_sle_info_block->pseudo_selection = FALSE;

    if(p_sle_info_block->status_line_set)
    {
        p_sle_info_block->status_line_set = FALSE;

        status_line_clear(p_docu_from_docno(p_sle_info_block->docno), STATUS_LINE_LEVEL_SS_FORMULA_LINE);
    }

    sle_refresh_field(p_sle_info_block);
}

_Check_return_
static STATUS
sle_delete_char(
    P_SLE_INFO_BLOCK p_sle_info_block,
    _InVal_     BOOL right)
{
    STATUS status = STATUS_OK;
    S32 current_size = array_elements(&p_sle_info_block->h_ustr_inline) - 1 /*CH_NULL*/;
    S32 caret_index = p_sle_info_block->caret_index;
    S32 stt_index = caret_index;
    S32 end_index = caret_index;
    P_USTR_INLINE ustr_inline = ustr_inline_from_h_ustr(&p_sle_info_block->h_ustr_inline);
    BOOL do_delete = FALSE;

    if(right)
    {
        if((caret_index >= 0) && (caret_index < current_size))
        {
            end_index += inline_bytecount_off(ustr_inline, caret_index);
            do_delete = TRUE;
        }
    }
    else
    {
        if((caret_index > 0) && (caret_index <= current_size))
        {
            stt_index -= inline_b_bytecount_off(ustr_inline, caret_index);
            do_delete = TRUE;
        }
    }

    if(do_delete)
    {
        S32 delta = stt_index - end_index;
        al_array_delete_at(&p_sle_info_block->h_ustr_inline, delta, stt_index);
        p_sle_info_block->caret_index = stt_index;
        sle_refresh_field(p_sle_info_block);
    }

    return(status);
}

_Check_return_
static STATUS
sle_selection_delete(
    P_SLE_INFO_BLOCK p_sle_info_block)
{
    S32 stt_index = sle_selection_lhs_index(p_sle_info_block);
    S32 end_index = sle_selection_rhs_index(p_sle_info_block);
    S32 delta = stt_index - end_index;
    al_array_delete_at(&p_sle_info_block->h_ustr_inline, delta, stt_index);
    p_sle_info_block->caret_index = stt_index;
    sle_selection_clear(p_sle_info_block);
    return(STATUS_OK);
}

static void
sle_refresh_status_line_for_selected_argument(
    _InoutRef_  P_SLE_INFO_BLOCK p_sle_info_block)
{
    PC_USTR_INLINE ustr_inline = ustr_inline_from_h_ustr(&p_sle_info_block->h_ustr_inline);
    const P_DOCU p_docu = p_docu_from_docno(p_sle_info_block->docno);
    PC_BYTE p_data = il_sle_argument_data_off(PC_BYTE, ustr_inline, p_sle_info_block->caret_index, 0);
    SS_FUNCTION_ARGUMENT_HELP ss_function_argument_help;
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock, elemof32("a.quick.buf.to.hold.some.text"));
    quick_ublock_with_buffer_setup(quick_ublock);

    ss_function_argument_help.help_id   = ((STATUS) p_data[0]) | ((STATUS) p_data[1] << 8);
    ss_function_argument_help.arg_index = (   (U32) p_data[2]) | (   (U32) p_data[3] << 8);
    ss_function_argument_help.p_quick_ublock = &quick_ublock;

    if(status_ok(object_call_id_load(p_docu, T5_MSG_SS_FUNCTION_ARGUMENT_HELP, &ss_function_argument_help, OBJECT_ID_SS_SPLIT)) && quick_ublock_bytes(&quick_ublock))
    {
        UI_TEXT ui_text;

        ui_text.type = UI_TEXT_TYPE_USTR_TEMP; /* status line must copy this */
        ui_text.text.ustr = quick_ublock_ustr(&quick_ublock);

        status_line_set(p_docu, STATUS_LINE_LEVEL_SS_FORMULA_LINE, &ui_text);

        p_sle_info_block->status_line_set = TRUE;
    }
    else if(p_sle_info_block->status_line_set)
    {
        p_sle_info_block->status_line_set = FALSE;

        status_line_clear(p_docu, STATUS_LINE_LEVEL_SS_FORMULA_LINE);
    }

    quick_ublock_dispose(&quick_ublock);
}

/* Convert internal index pointers into display values that can be used for showing the caret and performing general update */

_Check_return_
static STATUS
sle_index_compile(
    _InoutRef_  P_SLE_INFO_BLOCK p_sle_info_block,
    _InVal_     BOOL highlight,
    _In_        BOOL refresh,
    _InVal_     BOOL scroll_field)
{
    PC_USTR_INLINE ustr_inline = ustr_inline_from_h_ustr(&p_sle_info_block->h_ustr_inline);
    S32 index = 0;
    S32 vis_index = 0;
    S32 vis_caret_index = 0;
    S32 vis_selection_stt_index = 0;
    S32 vis_selection_end_index = 0;

#if RISCOS

    QUICK_UBLOCK_WITH_BUFFER(quick_ublock, elemof32("a.quick.buf.to.hold.some.text"));
    quick_ublock_with_buffer_setup(quick_ublock);

    for(;;)
    {
        U32 bytes_of_thing;
        S32 vis_size;

        if(is_inline_off(ustr_inline, index))
        {
            bytes_of_thing = inline_bytecount_off(ustr_inline, index);

            if(IL_SLE_ARGUMENT == inline_code_off(ustr_inline, index))
            {
                PC_USTR ustr_arg = il_sle_argument_data_off(PC_USTR, ustr_inline, index, IL_SLE_ARGUMENT_OFF_ARG_USTR);

                vis_size = ustrlen32(ustr_arg);

                status_return(quick_ublock_ustr_add(&quick_ublock, ustr_arg));

                if(!p_sle_info_block->selection && highlight && (p_sle_info_block->caret_index == index))
                {
                    p_sle_info_block->selection = TRUE;
                    p_sle_info_block->selection_stt_index = index;
                    p_sle_info_block->selection_end_index = index + inline_bytecount_off(ustr_inline, index);
                    refresh = TRUE;
                }
            }
            else
            {
                vis_size = 0;
            }
        }
        else
        {
            UCS4 ucs4 = uchars_char_decode_off(ustr_inline, index, /*ref*/bytes_of_thing);

            if(CH_NULL == ucs4)
                break;

            vis_size = 1;

            status_return(quick_ublock_ucs4_add(&quick_ublock, ucs4));
        }

        if(p_sle_info_block->selection)
        {
            if(p_sle_info_block->selection_stt_index == index)
                vis_selection_stt_index = vis_index;
            if(p_sle_info_block->selection_end_index == index)
                vis_selection_end_index = vis_index;
        }

        if(p_sle_info_block->caret_index == index)
            vis_caret_index = vis_index;

        index += bytes_of_thing;

        vis_index += vis_size;
    }

    /* repeat calculations to ensure that correct! */
    if(p_sle_info_block->selection)
    {
        if(p_sle_info_block->selection_stt_index == index)
            vis_selection_stt_index = vis_index;
        if(p_sle_info_block->selection_end_index == index)
            vis_selection_end_index = vis_index;
    }

    if(p_sle_info_block->caret_index == index)
        vis_caret_index = vis_index;

    /* ensure terminated */
    status_return(quick_ublock_nullch_add(&quick_ublock));

    { /* Host specific code for computing string lengths and scrolling the field */
    const P_DOCU p_docu = p_docu_from_docno(p_sle_info_block->docno);

    if(VIEWNO_NONE != p_sle_info_block->prefered_viewno)
    {
        const P_VIEW p_view = p_view_from_viewno(p_docu, p_sle_info_block->prefered_viewno);
        PC_U8Z p_u8 = quick_ublock_ustr(&quick_ublock);
        S32 len = strlen32(p_u8);

        vis_caret_index = MIN(len, vis_caret_index);
        vis_selection_stt_index = MIN(len, vis_selection_stt_index);
        vis_selection_end_index = MIN(len, vis_selection_end_index);

        UNREFERENCED_PARAMETER_ViewRef_(p_view);

        p_sle_info_block->display.caret_pixits = vis_caret_index * CHAR_WIDTH_PIXITS;

        if(p_sle_info_block->selection)
        {
            p_sle_info_block->display.selection_stt_pixits = vis_selection_stt_index * CHAR_WIDTH_PIXITS;
            p_sle_info_block->display.selection_end_pixits = vis_selection_end_index * CHAR_WIDTH_PIXITS;
        }

        if(scroll_field)
        {
            S32 string_end_index = len;
            PIXIT width_pixits = (p_sle_info_block->pixit_rect.br.x - p_sle_info_block->pixit_rect.tl.x - 2*2*PIXITS_PER_RISCOS /*l/r edges*/);
            S32 width_chars = width_pixits / (CHAR_WIDTH_PIXITS);
            S32 scroll_offset = p_sle_info_block->caret_index - (width_chars / 2);
            PIXIT scroll_offset_pixits;

            if((scroll_offset > 0) && ((string_end_index - scroll_offset) < width_chars))
                scroll_offset = string_end_index - width_chars;

            scroll_offset_pixits = MAX(scroll_offset, 0) * CHAR_WIDTH_PIXITS;

            if(p_sle_info_block->display.scroll_offset_pixits != scroll_offset_pixits)
            {
                p_sle_info_block->display.scroll_offset_pixits = scroll_offset_pixits;

                refresh = TRUE;
            }
        }
    }
    } /*block*/

    quick_ublock_dispose(&quick_ublock);

#elif WINDOWS

    QUICK_TBLOCK_WITH_BUFFER(quick_tblock, elemof32("a.quick.buf.to.hold.some.text"));
    quick_tblock_with_buffer_setup(quick_tblock);

    for(;;)
    {
        U32 bytes_of_thing;
        S32 vis_size;

        if(is_inline_off(ustr_inline, index))
        {
            bytes_of_thing = inline_bytecount_off(ustr_inline, index);

            if(IL_SLE_ARGUMENT == inline_code_off(ustr_inline, index))
            {
                PC_USTR ustr_arg = il_sle_argument_data_off(PC_USTR, ustr_inline, index, IL_SLE_ARGUMENT_OFF_ARG_USTR);

                vis_size = ustrlen32(ustr_arg);

                status_return(quick_tblock_ustr_add(&quick_tblock, ustr_arg));

                if(!p_sle_info_block->selection && highlight && (p_sle_info_block->caret_index == index))
                {
                    p_sle_info_block->selection = TRUE;
                    p_sle_info_block->selection_stt_index = index;
                    p_sle_info_block->selection_end_index = index + inline_bytecount_off(ustr_inline, index);
                    refresh = TRUE;
                }
            }
            else
            {
                vis_size = 0;
            }
        }
        else
        {
            UCS4 ucs4 = ustr_char_decode_off((PC_USTR) ustr_inline, index, /*ref*/bytes_of_thing);

            if(CH_NULL == ucs4)
                break;

            vis_size = 1;

            status_return(quick_tblock_ucs4_add(&quick_tblock, ucs4));
        }

        if(p_sle_info_block->selection)
        {
            if(p_sle_info_block->selection_stt_index == index)
                vis_selection_stt_index = vis_index;
            if(p_sle_info_block->selection_end_index == index)
                vis_selection_end_index = vis_index;
        }

        if(p_sle_info_block->caret_index == index)
            vis_caret_index = vis_index;

        index += bytes_of_thing;

        vis_index += vis_size;
    }

    /* repeat calculations to ensure that correct! */
    if(p_sle_info_block->selection)
    {
        if(p_sle_info_block->selection_stt_index == index)
            vis_selection_stt_index = vis_index;
        if(p_sle_info_block->selection_end_index == index)
            vis_selection_end_index = vis_index;
    }

    if(p_sle_info_block->caret_index == index)
        vis_caret_index = vis_index;

    /* ensure terminated */
    status_return(quick_tblock_nullch_add(&quick_tblock));

    { /* Host specific code for computing string lengths and scrolling the field */
    const P_DOCU p_docu = p_docu_from_docno(p_sle_info_block->docno);

    if(VIEWNO_NONE != p_sle_info_block->prefered_viewno)
    {
        const P_VIEW p_view = p_view_from_viewno(p_docu, p_sle_info_block->prefered_viewno);
        PCTSTR tstr = quick_tblock_tstr(&quick_tblock);
        S32 len = tstrlen32(tstr);

        vis_caret_index = MIN(len, vis_caret_index);
        vis_selection_stt_index = MIN(len, vis_selection_stt_index);
        vis_selection_end_index = MIN(len, vis_selection_end_index);

        {
        const HOST_WND hwnd = p_view->main[WIN_SLE].hwnd;
        const HDC hdc = GetDC(hwnd);

        if(NULL != hdc)
        {
            const PC_STYLE p_sle_style = p_style_for_sle();
            BOOL font_needs_delete;
            HFONT h_font_old = sle_select_font(hdc, &font_needs_delete, p_sle_style);
            SIZE size;

            void_WrapOsBoolChecking(
                GetTextExtentPoint32(hdc, tstr, (int) vis_caret_index, &size));
            p_sle_info_block->display.caret_pixels = TEXT_X + size.cx;

            if(p_sle_info_block->selection)
            {
                void_WrapOsBoolChecking(
                    GetTextExtentPoint32(hdc, tstr, (int) vis_selection_stt_index, &size));
                p_sle_info_block->display.selection_stt_pixels = TEXT_X + size.cx;

                void_WrapOsBoolChecking(
                    GetTextExtentPoint32(hdc, tstr, (int) vis_selection_end_index, &size));
                p_sle_info_block->display.selection_end_pixels = TEXT_X + size.cx;
            }

            if(scroll_field)
            {
                RECT client_rect;
                int scroll_offset_pixels;
                int width_pixels;

                GetClientRect(hwnd, &client_rect);
                width_pixels = MAX(client_rect.right - TEXT_X, 0);

                scroll_offset_pixels = p_sle_info_block->display.caret_pixels - (width_pixels / 2);

                void_WrapOsBoolChecking(
                    GetTextExtentPoint32(hdc, tstr, (int) len, &size));
                p_sle_info_block->display.string_end_pixels = TEXT_X + size.cx;

                if(((p_sle_info_block->display.string_end_pixels - scroll_offset_pixels) < width_pixels) && (scroll_offset_pixels > 0))
                    scroll_offset_pixels = p_sle_info_block->display.string_end_pixels - width_pixels;

                scroll_offset_pixels = MAX(scroll_offset_pixels, 0);

                if(p_sle_info_block->display.scroll_offset_pixels != scroll_offset_pixels)
                {
                    p_sle_info_block->display.scroll_offset_pixels = scroll_offset_pixels;

                    refresh = TRUE;
                }
            }

            if(font_needs_delete)
                DeleteFont(SelectFont(hdc, h_font_old));

            void_WrapOsBoolChecking(1 == ReleaseDC(hwnd, hdc));
        }
        } /*block*/

    }
    } /*block*/

    quick_tblock_dispose(&quick_tblock);

#endif /* OS */

    if(refresh)
    {
        sle_refresh_field(p_sle_info_block);

        if( p_sle_info_block->selection &&
            is_inline_off(ustr_inline, p_sle_info_block->caret_index) &&
            (IL_SLE_ARGUMENT == inline_code_off(ustr_inline, p_sle_info_block->caret_index)))
        {
            sle_refresh_status_line_for_selected_argument(p_sle_info_block);
        }
    }

    return(STATUS_OK);
}

/* Show the caret at the current index within the editing line.
 * We are given a physical index into the string
 * and then the co-ordinates must be correctly scrolled
 * before the host call to show the caret is made.
 */

static void
sle_display_caret(
    _InoutRef_  P_SLE_INFO_BLOCK p_sle_info_block)
{
    const P_DOCU p_docu = p_docu_from_docno(p_sle_info_block->docno);

    if(VIEWNO_NONE != p_sle_info_block->prefered_viewno)
    {
        const P_VIEW p_view = p_view_from_viewno(p_docu, p_sle_info_block->prefered_viewno);

        if(p_sle_info_block->has_focus)
        {
#if RISCOS && !defined(RISCOS_SLE_ICON)
            const HOST_WND hwnd = p_view->main[WIN_BACK].hwnd;
            PIXIT pixit_x = p_sle_info_block->display.caret_pixits - p_sle_info_block->display.scroll_offset_pixits;
            PIXIT pixit_y = CARET_HEIGHT_PIXITS;
            int os_x, os_y;
            pixit_x += p_sle_info_block->pixit_rect.tl.x;
            pixit_y += p_sle_info_block->pixit_rect.tl.y;
            pixit_x += X_OFFSET_PIXITS;
            pixit_y += Y_OFFSET_PIXITS;
            os_x = pixit_x / PIXITS_PER_RISCOS;
            os_y = pixit_y / PIXITS_PER_RISCOS;
            os_y -= 4; /* bodge to align with System font */
            (void) host_show_caret(hwnd, 0, p_sle_info_block->selection ? 0 : (int) CARET_HEIGHT_RISCOS, (int) + os_x, (int) - os_y);
#elif WINDOWS
            const HOST_WND hwnd = p_view->main[WIN_SLE].hwnd;
            int x = (int) (p_sle_info_block->display.caret_pixels - p_sle_info_block->display.scroll_offset_pixels);
            int y = 0;
            RECT client_rect;
            GetClientRect(hwnd, &client_rect);
            client_rect.top += 1;
            client_rect.bottom -= 1;
            y += 1;
            (void) host_show_caret(hwnd, GetSystemMetrics(SM_CXBORDER), p_sle_info_block->selection ? 0 : MAX(client_rect.bottom - client_rect.top, 0), x, y);
#endif /* OS */
        }
    }
}

static void
sle_refresh_field(
    P_SLE_INFO_BLOCK p_sle_info_block)
{
    const P_DOCU p_docu = p_docu_from_docno(p_sle_info_block->docno);
    VIEWNO viewno = VIEWNO_NONE;
    P_VIEW p_view;

    while(P_VIEW_NONE != (p_view = docu_enum_views(p_docu, &viewno)))
    {
#if RISCOS && !defined(RISCOS_SLE_ICON)
        SKEL_RECT skel_rect;
        RECT_FLAGS rect_flags;

        skel_rect.tl.pixit_point = p_sle_info_block->pixit_rect.tl;
        skel_rect.tl.page_num.x = 0;
        skel_rect.tl.page_num.y = 0;
        skel_rect.br.pixit_point = p_sle_info_block->pixit_rect.br;
        skel_rect.br.page_num.x = 0;
        skel_rect.br.page_num.y = 0;

        RECT_FLAGS_CLEAR(rect_flags);
        view_update_later_single(p_docu, p_view, UPDATE_BACK_WINDOW, &skel_rect, rect_flags);
#elif WINDOWS
        const HOST_WND hwnd = p_view->main[WIN_SLE].hwnd;

        InvalidateRect(hwnd, NULL, TRUE);

        if(viewno == p_sle_info_block->prefered_viewno)
            UpdateWindow(hwnd);
#endif /* OS */
    }
}

extern void
sle_view_prefer(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view)
{
    const P_SLE_INSTANCE_DATA p_sle_instance = p_object_instance_data_SLE(p_docu);
    const P_SLE_INFO_BLOCK p_sle_info_block = &p_sle_instance->ss_editor;

    p_sle_info_block->prefered_viewno = viewno_from_p_view(p_view);
}

#if RISCOS && 0 /* ignore drags for the mo */

/* -------------------------------------------------------------------------
 * Track the mouse for the current SLE field.  This is used to perform
 * drag selections over the field, when a drag occurs a tracking flag
 * is set, then we must attempt to look at the block to see if we
 * are being drag selected, if so then we highlight.
 * ------------------------------------------------------------------------- */

_Check_return_
extern STATUS
sle_track_mouse(
    P_SLE_INFO_BLOCK p_sle_info_block)
{
    if(p_sle_info_block->track_mouse_for_selection)
    {
        if(winx_drag_active() == (wimp_w) p_sle_info_block->drag_window)
        {
            WimpGetWindowStateBlock window_state;
            WimpGetPointerInfoBlock pointer_info;
            S32 point;
            U32 stt = p_sle_info_block->selection_stt;
            U32 end = p_sle_info_block->selection_end;

            wimp_get_window_state((wimp_w) p_sle_info_block->drag_window, &window_state);

            void_WrapOsErrorReporting(wimp_get_pointer_info(&pointer_info));

            /* convert to character index and then ensure within a sensible range */

            point = (pointer_info.x - window_state.visible_area.xmin);
            point = ((point - p_sle_info_block->left_offset) + X_OFFSET / PIXITS_PER_RISCOS) / CHAR_WIDTH;
            point = sle_get_real_index(ustr_inline_from_h_ustr(&p_sle_info_block->array_handle), point);

            /* adjust the relevant end of the selection, selection ends re-ordered when
             * used, so don't worry about ordering at this point... */

            if(p_sle_info_block->right_edge)
            {
                p_sle_info_block->selection_end = point;
            }
            else
            {
                p_sle_info_block->selection_stt = point;
            }

             /* check for need to refresh the field - i.e. has it been modified? */

            if((p_sle_info_block->selection_stt != stt) || (p_sle_info_block->selection_end != end))
              status_return(sle_compile_index(p_sle_info_block, TRUE, TRUE, FALSE));

        }
        else
        {
            p_sle_info_block->track_mouse_for_selection = FALSE;
        }
    }

    return(STATUS_OK);
  }

#endif

#if RISCOS /* Not specific, but unused by Windows */

/* -------------------------------------------------------------------------
 * Begin dragging the editline.  This involves setting up a drag
 * function so that we can know when the mouse is released and therefore
 * we can stop dragging.
 *
 * Assumes the relevant viewer will have been prefered!
 * ------------------------------------------------------------------------- */

_Check_return_
static STATUS
sle_drag_editline(
    P_SLE_INFO_BLOCK p_sle_info_block,
    _InVal_     BOOL right)
{
#if 0
    WimpGetWindowStateBlock window_state;
    WimpDragBox drag_box;

    DIALOG_CMD_CTL_VIEW_PARENT_QUERY parent_query;
    DIALOG_CMD_CTL_VIEW_POSN_QUERY control_query;
    DIALOG_CMD_CTL_VIEW_SIZE_QUERY size_query;

    size_query.h_dialog_view = control_query.h_dialog_view = parent_query.h_dialog_view = p_sle_info_block->h_dialog_view;
    size_query.dialog_control_id = control_query.dialog_control_id = parent_query.dialog_control_id = p_sle_info_block->dialog_control_id;

    status_return(skel_call_dialog(DIALOG_CMD_CODE_CTL_VIEW_PARENT_QUERY, &parent_query));
    status_return(skel_call_dialog(DIALOG_CMD_CODE_CTL_VIEW_POSN_QUERY, &control_query));
    status_return(skel_call_dialog(DIALOG_CMD_CODE_CTL_VIEW_SIZE_QUERY, &size_query));

    p_sle_info_block->drag_window = parent_query.whandle;

    drag_box.wimp_window = (wimp_w) parent_query.whandle;
    drag_box.drag_type = Wimp_DragBox_DragPoint;
    wimp_get_window_state(drag_box.wimp_window, &window_state);
    drag_box.parent_box.xmin = window_state.visible_area.xmin +(control_query.inner_posn.x /PIXITS_PER_RISCOS);
    drag_box.parent_box.ymin = window_state.visible_area.ymax -((control_query.inner_posn.y +size_query.inner_size.y) /PIXITS_PER_RISCOS);
    drag_box.parent_box.xmax = window_state.visible_area.xmin +((control_query.inner_posn.x +size_query.inner_size.x) /PIXITS_PER_RISCOS);
    drag_box.parent_box.ymax = window_state.visible_area.ymax -(control_query.inner_posn.y /PIXITS_PER_RISCOS);
    void_WrapOsErrorReporting(winx_drag_box(&drag_box));

    p_sle_info_block->track_mouse_for_selection = TRUE;
    p_sle_info_block->right_edge = right;
    p_sle_info_block->left_offset = (control_query.inner_posn.x - p_sle_info_block->display.scroll_offset) /PIXITS_PER_RISCOS;
#endif

    UNREFERENCED_PARAMETER(p_sle_info_block);
    UNREFERENCED_PARAMETER_InVal_(right);
    return(STATUS_OK);
}

/* Convert from the display index to a real string index, coping with inline sequences */

static S32
sle_get_real_index(
    _In_reads_(index) PC_USTR_INLINE ustr_inline,
    _InVal_     S32 index)
{
    S32 real_index = 0;
    S32 display_index = 0;

    while(display_index < index)
    {
        U32 bytes_of_thing;

        if(is_inline_off(ustr_inline, real_index))
        {
            bytes_of_thing = inline_bytecount_off(ustr_inline, real_index);

            if(IL_SLE_ARGUMENT == inline_code_off(ustr_inline, real_index))
            {
                PC_USTR ustr_arg = il_sle_argument_data_off(PC_USTR, ustr_inline, real_index, IL_SLE_ARGUMENT_OFF_ARG_USTR);

                display_index += ustrlen32(ustr_arg);

                if(display_index >index)
                    return(real_index);
            }

        }
        else
        {
            UCS4 ucs4 = ustr_char_decode_off(ustr_inline, real_index, /*ref*/bytes_of_thing);

            if(CH_NULL == ucs4)
                break;

            display_index += 1;
        }

        real_index += bytes_of_thing;
    }

    return(real_index);
}

#endif /* RISCOS */

/*
SINGLE LEFT  => clear selection, insert caret
SINGLE_RIGHT => adjust selected region to that point (from current caret if no stt already)
DRAG_LEFT    => begin drag selection at that point
DRAG_RIGHT   => adjust selection from that point (picking up the nearest end)
*/

_Check_return_
extern STATUS
sle_tool_user_mouse(
    _DocuRef_   P_DOCU p_docu,
    P_ANY /*P_T5_TOOLBAR_TOOL_USER_MOUSE*/ p_data)
{
#if RISCOS
    const P_T5_TOOLBAR_TOOL_USER_MOUSE p_t5_toolbar_tool_user_mouse = (P_T5_TOOLBAR_TOOL_USER_MOUSE) p_data;
    const P_SKELEVENT_CLICK p_skelevent_click = &p_t5_toolbar_tool_user_mouse->skelevent_click;
    const P_VIEW p_view = p_view_from_viewno(p_docu, p_t5_toolbar_tool_user_mouse->viewno);
    const P_SLE_INSTANCE_DATA p_sle_instance = p_object_instance_data_SLE(p_docu);
    const P_SLE_INFO_BLOCK p_sle_info_block = &p_sle_instance->ss_editor;
    S32 new_caret_index = 0;
    BOOL refresh = FALSE;
    STATUS status = STATUS_OK;
    S32 stt_index, end_index, mid_index;

    if(p_docu->mark_info_cells.h_markers) /* SKS after 1.08b2 15jul94 make consistent with F2 EditFormula */
    {
        host_bleep();
        return(STATUS_OK);
    }

    sle_view_prefer(p_docu, p_view);

    if(!p_sle_info_block->has_focus)
        status_consume(object_call_id(OBJECT_ID_SLE, p_docu, T5_CMD_SS_EDIT_FORMULA, P_DATA_NONE));

    /* Ensure that the selection information is correctly ordered. */
    if(p_sle_info_block->selection)
    {
        stt_index = sle_selection_lhs_index(p_sle_info_block);
        end_index = sle_selection_rhs_index(p_sle_info_block);
        mid_index = (end_index - stt_index) / 2;
        p_sle_info_block->selection_stt_index = stt_index;
        p_sle_info_block->selection_end_index = end_index;
    }
    else
        stt_index = end_index = mid_index = 0; /* keep dataflower happy */

    /* <<< sle_view_preferer(p_sle_info_block, p_view->viewno); */

    { /* Find the caret in a OS specific manner! */
    PIXIT pixit_x = p_skelevent_click->skel_point.pixit_point.x;

    pixit_x -= p_t5_toolbar_tool_user_mouse->pixit_rect.tl.x;
    pixit_x -= X_OFFSET_PIXITS;

    pixit_x += p_sle_info_block->display.scroll_offset_pixits;

    new_caret_index = sle_get_real_index(ustr_inline_from_h_ustr(&p_sle_info_block->h_ustr_inline), (int) (pixit_x / CHAR_WIDTH_PIXITS));
    } /*block*/

    switch(p_t5_toolbar_tool_user_mouse->t5_message)
    {
    default:
        break;

    case T5_EVENT_CLICK_LEFT_SINGLE:
        if(OBJECT_ID_SLE != p_docu->focus_owner)
            /* check protection before activating the editing line */
            status_break(status = check_protection_simple(p_docu, FALSE));

        sle_selection_clear(p_sle_info_block);

        p_sle_info_block->caret_index = new_caret_index;
        status = sle_grab_focus(p_sle_info_block);
        break;

    case T5_EVENT_CLICK_RIGHT_SINGLE:
        if(p_sle_info_block->selection)
         {
             p_sle_info_block->pseudo_selection = FALSE;
             if(new_caret_index <= (stt_index + mid_index))
                 p_sle_info_block->selection_stt_index = new_caret_index;
             else
                 p_sle_info_block->selection_end_index = new_caret_index;
         }
        else
         {
             p_sle_info_block->selection = TRUE;
             p_sle_info_block->pseudo_selection = FALSE;
             p_sle_info_block->selection_stt_index = p_sle_info_block->caret_index;
             p_sle_info_block->selection_end_index = new_caret_index;
         }
        refresh = TRUE;
        break;

    case T5_EVENT_CLICK_LEFT_DRAG:
        sle_selection_clear(p_sle_info_block);

        p_sle_info_block->selection = TRUE;
        p_sle_info_block->pseudo_selection = FALSE;
        p_sle_info_block->selection_stt_index = new_caret_index;
        p_sle_info_block->selection_end_index = new_caret_index;
        status = sle_drag_editline(p_sle_info_block, TRUE);
        break;

    case T5_EVENT_CLICK_RIGHT_DRAG:
        if(p_sle_info_block->selection)
        {
            p_sle_info_block->pseudo_selection = FALSE;
            status = sle_drag_editline(p_sle_info_block, (new_caret_index >= (stt_index + mid_index)));
        }
        else
        {
            p_sle_info_block->selection = TRUE;
            p_sle_info_block->pseudo_selection = FALSE;
            p_sle_info_block->selection_stt_index = p_sle_info_block->caret_index;
            p_sle_info_block->selection_end_index = new_caret_index;
            status = sle_drag_editline(p_sle_info_block, TRUE);
        }
        break;
    }

    status_return(status);

    return(sle_index_compile(p_sle_info_block, TRUE, refresh, FALSE));
#else
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER(p_data);
    return(STATUS_OK);
#endif
}

_Check_return_
_Ret_valid_
static inline PC_RGB
colour_of_sle_back(
    _InRef_     PC_STYLE p_sle_style)
{
    return(
        style_bit_test(p_sle_style, STYLE_SW_PS_RGB_BACK)
            ? &p_sle_style->para_style.rgb_back
            : &rgb_stash[0]);
}

#if RISCOS && defined(RISCOS_FONTY_SLE)

_Check_return_
static HOST_FONT
sle_find_font(
    const char * name,
    U32 x16_size_x,
    U32 x16_size_y)
{
    HOST_FONT host_font = HOST_FONT_NONE;

    /* c.f. host_font_find() in Fireworkz */
    _kernel_swi_regs rs;
    _kernel_oserror * p_kernel_oserror;

    rs.r[1] = (int) name;
    rs.r[2] = x16_size_x;
    rs.r[3] = x16_size_y;
    rs.r[4] = 0;
    rs.r[5] = 0;

    if(NULL == (p_kernel_oserror = (_kernel_swi(/*Font_FindFont*/ 0x040081, &rs, &rs))))
        host_font = (HOST_FONT) rs.r[0];

    return(host_font);
}

static int
riscos_fonty_sle(
    _InoutRef_  P_FONT_SPEC p_font_spec)
{
    static int yes_or_no = -1;
    static FONT_SPEC font_spec;

    if(yes_or_no < 0)
    {
        HOST_FONT host_font;

        /*U32 size_x = 0;*/
        U32 size_y = 12;

        /* RISC OS font manager needs 16x fontsize */
        /*U32 x16_size_x = 16 * 0;*/
        U32 x16_size_y = 16 * size_y;

        yes_or_no = 0;

        /* Only use DejaVuSans.Mono if we have a Unicode Font Manager */
        host_font = sle_find_font("\\F" "DejaVuSans.Mono" "\\E" "UTF8", x16_size_y, x16_size_y);

        if(HOST_FONT_NONE != host_font)
        {
            host_font_dispose(&host_font, P_REDRAW_CONTEXT_NONE);

            host_font = sle_find_font( /*"\\E" "Latin1"*/ "\\F" "DejaVuSans.Mono", /*x16_size_x ? x16_size_x :*/ x16_size_y, x16_size_y);

            if(HOST_FONT_NONE != host_font)
            {
                if(status_ok(fontmap_font_spec_from_host_base_name(&font_spec, "DejaVuSans.Mono")))
                {
                    font_spec.size_y = size_y * PIXITS_PER_POINT;

                    yes_or_no = 1;
                }

                host_font_dispose(&host_font, P_REDRAW_CONTEXT_NONE);
            }
        }
    }

    if(yes_or_no)
        *p_font_spec = font_spec;

    return(yes_or_no);
}

#endif /* RISCOS && defined(RISCOS_FONTY_SLE) */

_Check_return_
extern STATUS
sle_tool_user_redraw(
    _DocuRef_   P_DOCU p_docu,
    P_ANY /*P_T5_TOOLBAR_TOOL_USER_REDRAW*/ p_data)
{
#if RISCOS && defined(RISCOS_FONTY_SLE)
    static const RGB rgb_g_paint = { 0x88, 0x88, 0x88, 0 }; /* colour to paint a greyed line of text */

    const P_T5_TOOLBAR_TOOL_USER_REDRAW p_t5_toolbar_tool_user_redraw = (P_T5_TOOLBAR_TOOL_USER_REDRAW) p_data;
    const PC_REDRAW_CONTEXT p_redraw_context = &p_t5_toolbar_tool_user_redraw->redraw_context;
    const P_SLE_INSTANCE_DATA p_sle_instance = p_object_instance_data_SLE(p_docu);
    const P_SLE_INFO_BLOCK p_sle_info_block = &p_sle_instance->ss_editor;
    const PC_STYLE p_sle_style = p_style_for_sle();
    const PC_RGB p_rgb_sle_back = colour_of_sle_back(p_sle_style);
    PIXIT plot_x_pixits = p_t5_toolbar_tool_user_redraw->control_outer_pixit_rect.tl.x;
    PIXIT plot_y_pixits = p_t5_toolbar_tool_user_redraw->control_outer_pixit_rect.tl.y;

    plot_x_pixits += X_OFFSET_PIXITS;
    plot_y_pixits += Y_OFFSET_PIXITS;

    plot_x_pixits -= p_sle_info_block->display.scroll_offset_pixits;

    host_paint_rectangle_filled(p_redraw_context, &p_t5_toolbar_tool_user_redraw->control_inner_pixit_rect, p_rgb_sle_back);

    if(array_elements(&p_sle_info_block->h_ustr_inline) > 1 /*CH_NULL*/)
    {
        PC_USTR_INLINE string = ustr_inline_from_h_ustr(&p_sle_info_block->h_ustr_inline);
        U32 stt_index = 0;
        U32 index = 0;
        U32 length = 0;
        PIXIT new_plot_x_pixits = plot_x_pixits;
        PC_RGB p_rgb_sle_text = p_t5_toolbar_tool_user_redraw->enabled ? &p_sle_style->font_spec.colour : &rgb_g_paint;
        PC_SBSTR sbstr;
        HOST_FONT host_font_redraw = HOST_FONT_NONE;
        STATUS status = STATUS_OK;
        PIXIT base_line = 0;

        /*if(0 != p_sle_style->font_spec.h_app_name_tstr)*/
        {
            FONT_SPEC font_spec = p_sle_style->font_spec;

            if(riscos_fonty_sle(&font_spec) && status_ok(status = fonty_handle_from_font_spec(&font_spec, FALSE)))
            {
                const FONTY_HANDLE fonty_handle = (FONTY_HANDLE) status;
                const PC_UCHARS uchars = USTR_TEXT("0");
                const U32 uchars_n = 1;
                FONTY_CHUNK fonty_chunk;
                PIXIT pixit_width_of_zero;
                PIXIT ascent;

                host_font_redraw = fonty_host_font_from_fonty_handle_redraw(p_redraw_context, fonty_handle, FALSE);

                fonty_chunk_info_read_uchars(p_docu, &fonty_chunk, fonty_handle, uchars, uchars_n, 0 /* trail_spaces */);

                pixit_width_of_zero = fonty_chunk.width;
                ascent = fonty_chunk.ascent;

                reportf(TEXT("width of zero=") PIXIT_TFMT TEXT(" in %s ") S32_TFMT, pixit_width_of_zero, array_tstr(&font_spec.h_app_name_tstr), font_spec.size_y);
                reportf(TEXT("ascent=") PIXIT_TFMT, ascent);

#if 0
                /* round formatting values to pixels */
                const PIXIT pixits_per_riscos_d_x = PIXITS_PER_RISCOS * p_redraw_context->host_xform.riscos.d_x;
                const PIXIT pixits_per_riscos_d_y = PIXITS_PER_RISCOS * p_redraw_context->host_xform.riscos.d_y;
                pixit_width_of_zero = pixits_per_riscos_d_x * muldiv64_round_floor(pixit_width_of_zero, 1, pixits_per_riscos_d_x);
                ascent = pixits_per_riscos_d_y * muldiv64_round_floor(ascent, 1, pixits_per_riscos_d_y);
                reportf(TEXT("width of zero=") PIXIT_TFMT TEXT(" in %s ") S32_TFMT, pixit_width_of_zero, array_tstr(&font_spec.h_app_name_tstr), font_spec.size_y);
                reportf(TEXT("ascent=") PIXIT_TFMT, ascent);
#endif

                CHAR_WIDTH_PIXITS = pixit_width_of_zero;
                base_line = ascent;
            }
            else
            {
                status = STATUS_OK;
            }
        }

        plot_y_pixits += base_line;

        for(;;)
        {
            U32 bytes_of_thing;

            if(is_inline_off(string, index))
            {
                bytes_of_thing = inline_bytecount_off(string, index);

                if(IL_SLE_ARGUMENT == inline_code_off(string, index))
                {
                    PIXIT_POINT pixit_point;

                    /* output any plain bits we've accumulated */
                    if(length)
                    {
                        pixit_point.x = new_plot_x_pixits;
                        pixit_point.y = plot_y_pixits;
                        sbstr = PtrAddBytes(PC_SBSTR, string, stt_index);
                        if(HOST_FONT_NONE == host_font_redraw)
                            host_paint_plain_text_counted(p_redraw_context, &pixit_point, sbstr, length, p_rgb_sle_text);
                        else
                            host_fonty_text_paint_uchars_simple(p_redraw_context, &pixit_point, sbstr, length, p_rgb_sle_text, p_rgb_sle_back, host_font_redraw, TA_LEFT);
                        new_plot_x_pixits += length * CHAR_WIDTH_PIXITS;
                    }

                    /* output body of inline */
                    pixit_point.x = new_plot_x_pixits;
                    pixit_point.y = plot_y_pixits;
                    sbstr = il_sle_argument_data_off(PC_SBSTR, string, index, IL_SLE_ARGUMENT_OFF_ARG_USTR);
                    length = strlen32(sbstr);
                    if(HOST_FONT_NONE == host_font_redraw)
                        host_paint_plain_text_counted(p_redraw_context, &pixit_point, sbstr, length, p_rgb_sle_text);
                    else
                        host_fonty_text_paint_uchars_simple(p_redraw_context, &pixit_point, sbstr, length, p_rgb_sle_text, p_rgb_sle_back, host_font_redraw, TA_LEFT);
                    new_plot_x_pixits += length * CHAR_WIDTH_PIXITS;

                    length = 0;
                }
            }
            else
            {
                bytes_of_thing = 1;

                if(CH_NULL == PtrGetByteOff(string, index))
                    break;

                length += bytes_of_thing; /* accumulate another plain bit */
            }

            index += bytes_of_thing;

            if(length == 0)
                stt_index = index;
        }

        /* output any trailing bits not already printed */
        if(length)
        {
            PIXIT_POINT pixit_point;
            pixit_point.x = new_plot_x_pixits;
            pixit_point.y = plot_y_pixits;
            sbstr = PtrAddBytes(PC_SBSTR, string, stt_index);
            if(HOST_FONT_NONE == host_font_redraw)
                host_paint_plain_text_counted(p_redraw_context, &pixit_point, sbstr, length, p_rgb_sle_text);
            else
                host_fonty_text_paint_uchars_simple(p_redraw_context, &pixit_point, sbstr, length, p_rgb_sle_text, p_rgb_sle_back, host_font_redraw, TA_LEFT);
        }
    }

    /* Is there a selection? if so then highlight that particular area of the string. */
    if(p_sle_info_block->selection)
    {
        static const RGB rgb_invert        = { 0x00, 0x00, 0x00, 0 };
        static const RGB rgb_pseudo_invert = { 0xDD, 0xDD, 0xDD, 0 };

        PIXIT_RECT region_box = p_t5_toolbar_tool_user_redraw->control_inner_pixit_rect;
        PIXIT stt_pixits = MIN(p_sle_info_block->display.selection_stt_pixits, p_sle_info_block->display.selection_end_pixits);
        PIXIT end_pixits = MAX(p_sle_info_block->display.selection_stt_pixits, p_sle_info_block->display.selection_end_pixits);

        if(stt_pixits == 0)
            stt_pixits -= X_OFFSET_PIXITS * PIXITS_PER_RISCOS; /* SKS 15apr93 stops little white bit at lhs showing */

        region_box.tl.x = plot_x_pixits + stt_pixits;
        region_box.br.x = plot_x_pixits + end_pixits;

        host_invert_rectangle_filled(p_redraw_context, &region_box, p_sle_info_block->pseudo_selection ? &rgb_pseudo_invert : &rgb_invert, p_rgb_sle_back);
    }

    /* host font handles belong to fonty session (upper redraw layers) */

#elif RISCOS
    static const RGB rgb_fill    = { 0xFF, 0xFF, 0xFF, 0 }; /* colour to paint background in */
    static const RGB rgb_paint   = { 0x00, 0x00, 0x00, 0 }; /* colour to paint text in */
    static const RGB rgb_g_paint = { 0x88, 0x88, 0x88, 0 }; /* colour to paint a greyed line of text */

    const P_T5_TOOLBAR_TOOL_USER_REDRAW p_t5_toolbar_tool_user_redraw = (P_T5_TOOLBAR_TOOL_USER_REDRAW) p_data;
    const PC_REDRAW_CONTEXT p_redraw_context = &p_t5_toolbar_tool_user_redraw->redraw_context;
    const P_SLE_INSTANCE_DATA p_sle_instance = p_object_instance_data_SLE(p_docu);
    const P_SLE_INFO_BLOCK p_sle_info_block = &p_sle_instance->ss_editor;
    PIXIT plot_x_pixits = p_t5_toolbar_tool_user_redraw->control_outer_pixit_rect.tl.x;
    PIXIT plot_y_pixits = p_t5_toolbar_tool_user_redraw->control_outer_pixit_rect.tl.y;

    plot_x_pixits += X_OFFSET_PIXITS;
    plot_y_pixits += Y_OFFSET_PIXITS;

    plot_x_pixits -= p_sle_info_block->display.scroll_offset_pixits;

    host_paint_rectangle_filled(p_redraw_context, &p_t5_toolbar_tool_user_redraw->control_inner_pixit_rect, &rgb_fill);

    if(array_elements(&p_sle_info_block->h_ustr_inline) > 1 /*CH_NULL*/)
    {
        PC_USTR_INLINE string = ustr_inline_from_h_ustr(&p_sle_info_block->h_ustr_inline);
        U32 stt_index = 0;
        U32 index = 0;
        U32 length = 0;
        PIXIT new_plot_x_pixits = plot_x_pixits;
        PC_RGB p_text_rgb = p_t5_toolbar_tool_user_redraw->enabled ? &rgb_paint : &rgb_g_paint;
        PC_SBSTR sbstr;

        for(;;)
        {
            U32 bytes_of_thing;

            if(is_inline_off(string, index))
            {
                bytes_of_thing = inline_bytecount_off(string, index);

                if(IL_SLE_ARGUMENT == inline_code_off(string, index))
                {
                    PIXIT_POINT pixit_point;

                    /* output any plain bits we've accumulated */
                    if(length)
                    {
                        pixit_point.x = new_plot_x_pixits;
                        pixit_point.y = plot_y_pixits;
                        sbstr = PtrAddBytes(PC_SBSTR, string, stt_index);
                        host_paint_plain_text_counted(p_redraw_context, &pixit_point, sbstr, length, p_text_rgb);
                        new_plot_x_pixits += length * CHAR_WIDTH_PIXITS;
                    }

                    /* output body of inline */
                    pixit_point.x = new_plot_x_pixits;
                    pixit_point.y = plot_y_pixits;
                    sbstr = il_sle_argument_data_off(PC_SBSTR, string, index, IL_SLE_ARGUMENT_OFF_ARG_USTR);
                    length = strlen32(sbstr);
                    host_paint_plain_text_counted(p_redraw_context, &pixit_point, sbstr, length, p_text_rgb);
                    new_plot_x_pixits += length * CHAR_WIDTH_PIXITS;

                    length = 0;
                }
            }
            else
            {
                bytes_of_thing = 1;

                if(CH_NULL == PtrGetByteOff(string, index))
                    break;

                length += bytes_of_thing; /* accumulate another plain bit */
            }

            index += bytes_of_thing;

            if(length == 0)
                stt_index = index;
        }

        /* output any trailing bits not already printed */
        if(length)
        {
            PIXIT_POINT pixit_point;
            pixit_point.x = new_plot_x_pixits;
            pixit_point.y = plot_y_pixits;
            sbstr = PtrAddBytes(PC_SBSTR, string, stt_index);
            host_paint_plain_text_counted(p_redraw_context, &pixit_point, sbstr, length, p_text_rgb);
        }
    }

    /* Is there a selection? if so then highlight that particular area of the string. */
    if(p_sle_info_block->selection)
    {
        static const RGB rgb_invert        = { 0x00, 0x00, 0x00, 0 };
        static const RGB rgb_pseudo_invert = { 0xDD, 0xDD, 0xDD, 0 };

        PIXIT_RECT region_box = p_t5_toolbar_tool_user_redraw->control_inner_pixit_rect;
        PIXIT stt_pixits = MIN(p_sle_info_block->display.selection_stt_pixits, p_sle_info_block->display.selection_end_pixits);
        PIXIT end_pixits = MAX(p_sle_info_block->display.selection_stt_pixits, p_sle_info_block->display.selection_end_pixits);

        if(stt_pixits == 0)
            stt_pixits -= X_OFFSET_PIXITS * PIXITS_PER_RISCOS; /* SKS 15apr93 stops little white bit at lhs showing */

        region_box.tl.x = plot_x_pixits + stt_pixits;
        region_box.br.x = plot_x_pixits + end_pixits;

        host_invert_rectangle_filled(p_redraw_context, &region_box, p_sle_info_block->pseudo_selection ? &rgb_pseudo_invert : &rgb_invert, &rgb_fill);
    }
#else
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER(p_data);
#endif
    return(STATUS_OK);
}

T5_MSG_PROTO(static, sle_event_keys, _InRef_ P_SKELEVENT_KEYS p_skelevent_keys)
{
    const P_SLE_INSTANCE_DATA p_sle_instance = p_object_instance_data_SLE(p_docu);
    const P_SLE_INFO_BLOCK p_sle_info_block = &p_sle_instance->ss_editor;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    status_return(sle_skip_selection(p_sle_info_block));

    status_return(sle_paste_text(p_sle_info_block, p_skelevent_keys->p_quick_ublock, 0, 0));

    status_return(sle_index_compile(p_sle_info_block, TRUE, TRUE, TRUE));

    sle_display_caret(p_sle_info_block);

    return(STATUS_OK);
}

_Check_return_
static STATUS
sle_msg_init_thunk(
    _DocuRef_   P_DOCU p_docu)
{
    status_return(object_instance_data_alloc(p_docu, OBJECT_ID_SLE, sizeof32(SLE_INSTANCE_DATA)));

    return(maeve_event_handler_add(p_docu, maeve_event_ob_sle, (CLIENT_HANDLE) 0));
}

_Check_return_
static STATUS
sle_msg_close1(
    _DocuRef_   P_DOCU p_docu)
{
    const P_SLE_INSTANCE_DATA p_sle_instance = p_object_instance_data_SLE(p_docu);
    const P_SLE_INFO_BLOCK p_sle_info_block = &p_sle_instance->ss_editor;

    sle_dispose(p_sle_info_block);

    return(STATUS_OK);
}

_Check_return_
static STATUS
sle_msg_close_thunk(
    _DocuRef_   P_DOCU p_docu)
{
    maeve_event_handler_del(p_docu, maeve_event_ob_sle, (CLIENT_HANDLE) 0);

    object_instance_data_dispose(p_docu, OBJECT_ID_SLE);

    return(STATUS_OK);
}

T5_MSG_PROTO(static, sle_msg_initclose, _InRef_ PC_MSG_INITCLOSE p_msg_initclose)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    switch(p_msg_initclose->t5_msg_initclose_message)
    {
#if WINDOWS
    case T5_MSG_IC__STARTUP_SERVICES:
        return(sle_register_wndclass());
#endif

    case T5_MSG_IC__STARTUP:
        /*resource_init(OBJECT_ID_SLE, MSG_WEAK, P_BOUND_RESOURCES_OBJECT_ID_SLE)*/
        sle_style_startup();
        return(register_object_construct_table(OBJECT_ID_SLE, object_construct_table, FALSE /* no inlines */));

    /* initialise object in new document thunk */
    case T5_MSG_IC__INIT_THUNK:
        return(sle_msg_init_thunk(p_docu));

    case T5_MSG_IC__CLOSE1: /* SKS 28apr93 - used to be CLOSE2 but we can't give away focus from SLE then */
        return(sle_msg_close1(p_docu));

    case T5_MSG_IC__CLOSE_THUNK:
        return(sle_msg_close_thunk(p_docu));

    default:
        return(STATUS_OK);
    }
}

_Check_return_
static STATUS
sle_msg_choice_changed_ui_styles(
    _DocuRef_       P_DOCU p_docu)
{
    if(DOCNO_CONFIG == docno_from_p_docu(p_docu))
    {
        recache_sle_style = TRUE;
    }

    return(STATUS_OK);
}

T5_MSG_PROTO(static, sle_msg_choice_changed, _InRef_ PC_MSG_CHOICE_CHANGED p_msg_choice_changed)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    switch(p_msg_choice_changed->t5_message)
    {
    case T5_CMD_STYLE_FOR_CONFIG:
        return(sle_msg_choice_changed_ui_styles(p_docu));

    default:
        return(STATUS_OK);
    }
}

T5_MSG_PROTO(static, sle_msg_caret_show_claim, _InRef_ P_CARET_SHOW_CLAIM p_caret_show_claim)
{
    const P_SLE_INSTANCE_DATA p_sle_instance = p_object_instance_data_SLE(p_docu);
    const P_SLE_INFO_BLOCK p_sle_info_block = &p_sle_instance->ss_editor;

    UNREFERENCED_PARAMETER_InVal_(t5_message);
    UNREFERENCED_PARAMETER_InRef_(p_caret_show_claim);

    sle_display_caret(p_sle_info_block);

    return(STATUS_OK);
}

T5_MSG_PROTO(static, sle_msg_docu_focus_query, _InoutRef_ P_DOCU_FOCUS_QUERY p_docu_focus_query)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(docno_ss_edit != DOCNO_NONE)
    {
        p_docu_focus_query->docno = docno_ss_edit;
        p_docu_focus_query->processed = 1;
    }

    return(STATUS_OK);
}

T5_MSG_PROTO(static, sle_msg_paste_editline, _InRef_ P_T5_PASTE_EDITLINE p_t5_paste_editline)
{
    STATUS status = STATUS_OK;

    if(T5_MSG_ACTIVATE_EDITLINE == t5_message)
    {   /* MRJC 19.1.95 */
        sle_view_prefer(p_docu, p_view_from_viewno_caret(p_docu));

        status = sle_editline_activate(p_docu, p_t5_paste_editline->p_quick_ublock, p_t5_paste_editline->select, p_t5_paste_editline->special_select);
    }
    else
    {
        const P_SLE_INSTANCE_DATA p_sle_instance = p_object_instance_data_SLE(p_docu);
        const P_SLE_INFO_BLOCK p_sle_info_block = &p_sle_instance->ss_editor;

        if(status_ok(status = sle_paste_text(p_sle_info_block, p_t5_paste_editline->p_quick_ublock, p_t5_paste_editline->select, p_t5_paste_editline->special_select)))
        if(status_ok(status = sle_index_compile(p_sle_info_block, TRUE, TRUE, TRUE)))
            sle_display_caret(p_sle_info_block);
    }

    return(status);
}

/* activate editing line with keys */

T5_MSG_PROTO(static, sle_msg_object_keys, _InoutRef_ P_OBJECT_KEYS p_object_keys)
{
    T5_PASTE_EDITLINE t5_paste_editline;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    p_object_keys->p_skelevent_keys->processed = 1;

    t5_paste_editline.p_quick_ublock = p_object_keys->p_skelevent_keys->p_quick_ublock; /* note that this is ***NOT*** CH_NULL-terminated */
    t5_paste_editline.select = FALSE;
    t5_paste_editline.special_select = FALSE;

    return(object_sle(p_docu, T5_MSG_ACTIVATE_EDITLINE, &t5_paste_editline));
}

/* Force the current cell contents into the editing line if not already editing */

T5_CMD_PROTO(static, sle_cmd_ss_edit_formula)
{
    STATUS status = STATUS_OK;

    UNREFERENCED_PARAMETER_InVal_(t5_message);
    UNREFERENCED_PARAMETER_InRef_(p_t5_cmd);

    { /* SKS after 1.07 15jul94 check for already editing */
    const P_SLE_INSTANCE_DATA p_sle_instance = p_object_instance_data_SLE(p_docu);
    const P_SLE_INFO_BLOCK p_sle_info_block = &p_sle_instance->ss_editor;
#if 0 /* MRJC 19.1.95 */
    sle_view_prefer(p_docu, p_view_from_viewno_caret(p_docu));
#endif
    if(p_sle_info_block->has_focus)
        return(STATUS_OK);
    } /*block*/

    if(p_docu->mark_info_cells.h_markers) /* can't edit in this state */
    {
        host_bleep();
        return(STATUS_OK);
    }

    cur_change_before(p_docu);

    {
    POSITION position = p_docu->cur;
    OBJECT_READ_TEXT object_read_text;
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 500);
    quick_ublock_with_buffer_setup(quick_ublock);

    zero_struct(object_read_text);
    object_read_text.p_quick_ublock = &quick_ublock;
    object_read_text.type = OBJECT_READ_TEXT_PLAIN;

    object_position_init(&position.object_position);
    status_consume(object_data_from_position(p_docu, &object_read_text.object_data, &position, P_OBJECT_POSITION_NONE));

    {
    SS_DECOMPILER_OPTIONS ss_decompiler_options = g_ss_decompiler_options;
    SS_RECOG_CONTEXT ss_recog_context;

    ss_recog_context_push(&ss_recog_context);

    if(global_preferences.ss_alternate_formula_style)
    {   /* Alternate formula style (Excel-ise) the decompiler output */
        g_ss_decompiler_options.lf = 0; /* prune these out for now */
        g_ss_decompiler_options.cr = 0;
        g_ss_decompiler_options.initial_formula_equals = 1;
        g_ss_decompiler_options.range_colon_separator = 1;
        g_ss_decompiler_options.upper_case_function = 1;
        g_ss_decompiler_options.upper_case_slr = 1;
        g_ss_decompiler_options.zero_args_function_parentheses = 1;
    }

    status = object_call_id(object_read_text.object_data.object_id, p_docu, T5_MSG_OBJECT_READ_TEXT, &object_read_text);

    ss_recog_context_pull(&ss_recog_context);

    g_ss_decompiler_options = ss_decompiler_options;
    } /*block*/

    if(status_ok(status))
    {
        T5_PASTE_EDITLINE t5_paste_editline;

        t5_paste_editline.p_quick_ublock = &quick_ublock;
        t5_paste_editline.select = FALSE;
        t5_paste_editline.special_select = FALSE;

        status = object_sle(p_docu, T5_MSG_ACTIVATE_EDITLINE, &t5_paste_editline);

        if(status_ok(status) && (0 != quick_ublock_bytes(&quick_ublock)))
        {
            if(OBJECT_ID_TEXT == p_docu->cur.object_position.object_id)
            {
                const P_SLE_INSTANCE_DATA p_sle_instance = p_object_instance_data_SLE(p_docu);
                const P_SLE_INFO_BLOCK p_sle_info_block = &p_sle_instance->ss_editor;

                status = sle_position_caret(p_sle_info_block, p_docu->cur.object_position.data);
            }
        }
    }

    quick_ublock_dispose(&quick_ublock);
    } /*block*/

    return(status);
}

T5_CMD_PROTO(static, sle_cmd_selection_clear)
{
    const P_SLE_INSTANCE_DATA p_sle_instance = p_object_instance_data_SLE(p_docu);
    const P_SLE_INFO_BLOCK p_sle_info_block = &p_sle_instance->ss_editor;

    UNREFERENCED_PARAMETER_InVal_(t5_message);
    UNREFERENCED_PARAMETER_InRef_(p_t5_cmd);

    status_return(sle_skip_selection(p_sle_info_block));

    sle_selection_clear(p_sle_info_block);

    return(STATUS_OK);
}

T5_CMD_PROTO(static, object_sle_cmd)
{
    assert(T5_CMD__ACTUAL_END <= T5_CMD__END);

    switch(T5_MESSAGE_CMD_OFFSET(t5_message))
    {
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_SS_EDIT_FORMULA):
        return(sle_cmd_ss_edit_formula(p_docu, t5_message, p_t5_cmd));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_SELECTION_CLEAR):
        return(sle_cmd_selection_clear(p_docu, t5_message, p_t5_cmd));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_DELETE_CHARACTER_LEFT):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_DELETE_CHARACTER_RIGHT):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_DELETE_LINE):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_DOCUMENT_BOTTOM):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_DOCUMENT_TOP):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_LINE_START):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_LINE_END):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_CURSOR_LEFT):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_CURSOR_RIGHT):
        return(sle_key_command(p_docu, t5_message, p_t5_cmd));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_ESCAPE):
        return(ss_formula_reject(p_docu));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_RETURN):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_TAB_LEFT):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_TAB_RIGHT):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_CURSOR_UP):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_CURSOR_DOWN):
        return(ss_formula_eval_then_msg_for_message(p_docu, t5_message, p_t5_cmd));

    default:
        return(STATUS_OK);
    }
}

OBJECT_PROTO(extern, object_sle)
{
    if(T5_MESSAGE_IS_CMD(t5_message))
        return(object_sle_cmd(p_docu, t5_message, (PC_T5_CMD) p_data));

    switch(t5_message)
    {
    case T5_EVENT_KEYS:
        return(sle_event_keys(p_docu, t5_message, (P_SKELEVENT_KEYS) p_data));

    case T5_MSG_INITCLOSE:
        return(sle_msg_initclose(p_docu, t5_message, (PC_MSG_INITCLOSE) p_data));

    case T5_MSG_CHOICE_CHANGED:
        return(sle_msg_choice_changed(p_docu, t5_message, (PC_MSG_CHOICE_CHANGED) p_data));

    case T5_MSG_CARET_SHOW_CLAIM:
        return(sle_msg_caret_show_claim(p_docu, t5_message, (P_CARET_SHOW_CLAIM) p_data));

    case T5_MSG_DOCU_FOCUS_QUERY:
        return(sle_msg_docu_focus_query(p_docu, t5_message, (P_DOCU_FOCUS_QUERY) p_data));

    case T5_MSG_ACTIVATE_EDITLINE:
    case T5_MSG_PASTE_EDITLINE:
        return(sle_msg_paste_editline(p_docu, t5_message, (P_T5_PASTE_EDITLINE) p_data));

    case T5_MSG_OBJECT_KEYS:
        return(sle_msg_object_keys(p_docu, t5_message, (P_OBJECT_KEYS) p_data));

    default:
        return(STATUS_OK);
    }
}

/* end of sle_ss.c */
