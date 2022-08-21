/* ui_ss.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#include "common/gflags.h"

#include "ob_ss/ob_ss.h"

#include "ob_toolb/xp_toolb.h"

static const T5_TOOLBAR_TOOL_DESC
ss_tools[] =
{
    { USTR_TEXT("FILL_DOWN"),
        OBJECT_ID_SS, T5_CMD_REPLICATE_DOWN,
        OBJECT_ID_SS, SS_ID_BM_TOOLBAR_FILL_DN, 0,
        { T5_TOOLBAR_TOOL_TYPE_COMMAND, 0 },
        UI_TEXT_INIT_RESID(SS_MSG_STATUS_FILL_DOWN), T5_CMD_REPLICATE_UP },

    { USTR_TEXT("FILL_RIGHT"),
        OBJECT_ID_SS, T5_CMD_REPLICATE_RIGHT,
        OBJECT_ID_SS, SS_ID_BM_TOOLBAR_FILL_RT, 0,
        { T5_TOOLBAR_TOOL_TYPE_COMMAND, 0 },
        UI_TEXT_INIT_RESID(SS_MSG_STATUS_FILL_RIGHT), T5_CMD_REPLICATE_LEFT },

    { USTR_TEXT("MAKE_TEXT"),
        OBJECT_ID_SS, T5_CMD_SS_MAKE_TEXT,
        OBJECT_ID_SS, SS_ID_BM_TOOLBAR_MAKE_TEXT, 0,
        { T5_TOOLBAR_TOOL_TYPE_COMMAND, 0 },
        UI_TEXT_INIT_RESID(SS_MSG_STATUS_MAKE_TEXT) },

    { USTR_TEXT("MAKE_NUMBER"),
        OBJECT_ID_SS, T5_CMD_SS_MAKE_NUMBER,
        OBJECT_ID_SS, SS_ID_BM_TOOLBAR_MAKE_NUMBER, 0,
        { T5_TOOLBAR_TOOL_TYPE_COMMAND, 0 },
        UI_TEXT_INIT_RESID(SS_MSG_STATUS_MAKE_NUMBER) },

    { USTR_TEXT("MAKE_CONSTANT"),
        OBJECT_ID_SKEL, T5_CMD_SNAPSHOT,
        OBJECT_ID_SS, SS_ID_BM_TOOLBAR_MAKE_CONSTANT, 0,
        { T5_TOOLBAR_TOOL_TYPE_COMMAND, 0 },
        UI_TEXT_INIT_RESID(SS_MSG_STATUS_MAKE_CONSTANT) },

    { USTR_TEXT("PLUS"),
        OBJECT_ID_SS, T5_CMD_INSERT_OPERATOR_PLUS,
        OBJECT_ID_SS, SS_ID_BM_TOOLBAR_PLUS, 0,
        { T5_TOOLBAR_TOOL_TYPE_COMMAND, 0 },
        UI_TEXT_INIT_RESID(SS_MSG_STATUS_PLUS) },

    { USTR_TEXT("MINUS"),
        OBJECT_ID_SS, T5_CMD_INSERT_OPERATOR_MINUS,
        OBJECT_ID_SS, SS_ID_BM_TOOLBAR_MINUS, 0,
        { T5_TOOLBAR_TOOL_TYPE_COMMAND, 0 },
        UI_TEXT_INIT_RESID(SS_MSG_STATUS_MINUS) },

    { USTR_TEXT("TIMES"),
        OBJECT_ID_SS, T5_CMD_INSERT_OPERATOR_TIMES,
        OBJECT_ID_SS, SS_ID_BM_TOOLBAR_TIMES, 0,
        { T5_TOOLBAR_TOOL_TYPE_COMMAND, 0 },
        UI_TEXT_INIT_RESID(SS_MSG_STATUS_TIMES) },

    { USTR_TEXT("DIVIDE"),
        OBJECT_ID_SS, T5_CMD_INSERT_OPERATOR_DIVIDE,
        OBJECT_ID_SS, SS_ID_BM_TOOLBAR_DIVIDE, 0,
        { T5_TOOLBAR_TOOL_TYPE_COMMAND, 0 },
        UI_TEXT_INIT_RESID(SS_MSG_STATUS_DIVIDE) },

    { USTR_TEXT("AUTO_SUM"),
        OBJECT_ID_SS, T5_CMD_ACTIVATE_MENU_AUTO_FUNCTION,
        OBJECT_ID_SS, SS_ID_BM_TOOLBAR_AUTO_SUM, 0,
        { T5_TOOLBAR_TOOL_TYPE_COMMAND, 1/*im*/ },
        UI_TEXT_INIT_RESID(SS_MSG_STATUS_AUTO_SUM), T5_CMD_SS_FUNCTIONS_SHORT },

    { USTR_TEXT("CHART"),
        OBJECT_ID_CHART, T5_CMD_ACTIVATE_MENU_CHART,
        OBJECT_ID_SS, SS_ID_BM_TOOLBAR_CHART, 0,
        { T5_TOOLBAR_TOOL_TYPE_COMMAND, 1/*im*/ },
        UI_TEXT_INIT_RESID(SS_MSG_STATUS_CHART) },

    { USTR_TEXT("FUNCTION"),
        OBJECT_ID_SS, T5_CMD_ACTIVATE_MENU_FUNCTION_SELECTOR,
        OBJECT_ID_SS, SS_ID_BM_TOOLBAR_FUNCTION, 0,
        { T5_TOOLBAR_TOOL_TYPE_COMMAND, 1/*im*/ },
        UI_TEXT_INIT_RESID(SS_MSG_STATUS_FORMULA_FUNCTION) },

    { USTR_TEXT("FORMULA_CANCEL"),
        OBJECT_ID_SLE, T5_CMD_ESCAPE,
        OBJECT_ID_SS, SS_ID_BM_TOOLBAR_FORM_CAN, 0,
        { T5_TOOLBAR_TOOL_TYPE_COMMAND, 0 },
        UI_TEXT_INIT_RESID(SS_MSG_STATUS_FORMULA_CANCEL) },

    { USTR_TEXT("FORMULA_ENTER"),
        OBJECT_ID_SLE, T5_CMD_RETURN,
        OBJECT_ID_SS, SS_ID_BM_TOOLBAR_FORM_ENT, 0,
        { T5_TOOLBAR_TOOL_TYPE_COMMAND, 0 },
        UI_TEXT_INIT_RESID(SS_MSG_STATUS_FORMULA_ENTER) },

    { USTR_TEXT("FORMULA_LINE"),
        OBJECT_ID_SS, T5_EVENT_NONE,
        OBJECT_ID_SS, 0, 0,
        { T5_TOOLBAR_TOOL_TYPE_USER, 0 },
        { UI_TEXT_TYPE_NONE } }
};

/* -------------------------------------------------------------------------
 * Enable/disable buttons on the toolbar that belong to us.
 * ------------------------------------------------------------------------- */

_Check_return_
extern STATUS
ss_backcontrols_enable(
    _DocuRef_   P_DOCU p_docu)
{
    T5_TOOLBAR_TOOL_ENABLE t5_toolbar_tool_enable;

    t5_toolbar_tool_enable.enable_id = 0; /* very simple usage */

    t5_toolbar_tool_enable.enabled = (OBJECT_ID_CELLS == p_docu->focus_owner) && (0 != p_docu->mark_info_cells.h_markers);
    tool_enable(p_docu, &t5_toolbar_tool_enable, USTR_TEXT("FILL_RIGHT"));
    tool_enable(p_docu, &t5_toolbar_tool_enable, USTR_TEXT("FILL_DOWN"));

    t5_toolbar_tool_enable.enabled = (OBJECT_ID_CELLS == p_docu->focus_owner) || (OBJECT_ID_SLE == p_docu->focus_owner);
    tool_enable(p_docu, &t5_toolbar_tool_enable, USTR_TEXT("AUTO_SUM"));
    tool_enable(p_docu, &t5_toolbar_tool_enable, USTR_TEXT("PLUS"));
    tool_enable(p_docu, &t5_toolbar_tool_enable, USTR_TEXT("MINUS"));
    tool_enable(p_docu, &t5_toolbar_tool_enable, USTR_TEXT("TIMES"));
    tool_enable(p_docu, &t5_toolbar_tool_enable, USTR_TEXT("DIVIDE"));
    tool_enable(p_docu, &t5_toolbar_tool_enable, USTR_TEXT("FUNCTION"));

    t5_toolbar_tool_enable.enabled = (OBJECT_ID_CELLS == p_docu->focus_owner);
    tool_enable(p_docu, &t5_toolbar_tool_enable, USTR_TEXT("CHART"));
    tool_enable(p_docu, &t5_toolbar_tool_enable, USTR_TEXT("MAKE_TEXT"));
    tool_enable(p_docu, &t5_toolbar_tool_enable, USTR_TEXT("MAKE_NUMBER"));
    tool_enable(p_docu, &t5_toolbar_tool_enable, USTR_TEXT("MAKE_CONSTANT"));

    t5_toolbar_tool_enable.enabled = (OBJECT_ID_SLE == p_docu->focus_owner);
    tool_enable(p_docu, &t5_toolbar_tool_enable, USTR_TEXT("FORMULA_CANCEL"));
    tool_enable(p_docu, &t5_toolbar_tool_enable, USTR_TEXT("FORMULA_ENTER"));

    t5_toolbar_tool_enable.enabled = (OBJECT_ID_CELLS == p_docu->focus_owner) || (OBJECT_ID_SLE == p_docu->focus_owner);
    tool_enable(p_docu, &t5_toolbar_tool_enable, USTR_TEXT("FORMULA_LINE"));

    return(STATUS_OK);
}

_Check_return_
extern STATUS
ss_formula_reflect_contents(
    _DocuRef_   P_DOCU p_docu)
{
    P_EV_CELL p_ev_cell;
    STATUS status = STATUS_OK;
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 128);
    quick_ublock_with_buffer_setup(quick_ublock);

    if(P_DATA_NONE != (p_ev_cell = p_ev_cell_object_from_slr(p_docu, &p_docu->cur.slr)))
        status = ev_cell_decode_ui(&quick_ublock, p_ev_cell, docno_from_p_docu(p_docu));

    if(status_ok(status))
        status = sle_change_text(p_docu, &quick_ublock, FALSE, FALSE);

    quick_ublock_dispose(&quick_ublock);

    return(status);
}

/*
only receives a selection of sideways-passed messages from ob_ss
*/

_Check_return_
static STATUS
ui_ss_msg_startup(void)
{
    T5_TOOLBAR_TOOLS t5_toolbar_tools;
    t5_toolbar_tools.n_tool_desc = elemof32(ss_tools);
    t5_toolbar_tools.p_t5_toolbar_tool_desc = ss_tools;
    return(object_call_id(OBJECT_ID_TOOLBAR, P_DOCU_NONE, T5_MSG_TOOLBAR_TOOLS, &t5_toolbar_tools));
}

_Check_return_
static STATUS
ss_msg_initclose(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_MSG_INITCLOSE p_msg_initclose)
{
    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__STARTUP:
        return(ui_ss_msg_startup());

    case T5_MSG_IC__INIT1:
        return(sle_create(p_docu));

    default:
        return(STATUS_OK);
    }
}

_Check_return_
static STATUS
ui_ss_toolbar_tool_user_view_new(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_T5_TOOLBAR_TOOL_USER_VIEW_NEW p_t5_toolbar_tool_user_view_new)
{
    const P_VIEW p_view = p_view_from_viewno(p_docu, p_t5_toolbar_tool_user_view_new->viewno);
    consume(HOST_WND, sle_view_create(p_docu, p_view));
    p_t5_toolbar_tool_user_view_new->client_handle = 42;
    return(STATUS_OK);
}

_Check_return_
static STATUS
ui_ss_toolbar_tool_user_view_delete(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_T5_TOOLBAR_TOOL_USER_VIEW_DELETE p_t5_toolbar_tool_user_view_delete)
{
    sle_view_destroy(p_view_from_viewno(p_docu, p_t5_toolbar_tool_user_view_delete->viewno));
    return(STATUS_OK);
}

_Check_return_
static STATUS
ui_ss_toolbar_tool_user_size_query(
    _InoutRef_  P_T5_TOOLBAR_TOOL_USER_SIZE_QUERY p_t5_toolbar_tool_user_size_query)
{
    p_t5_toolbar_tool_user_size_query->pixit_size.cx = 84 * PIXITS_PER_INCH; /* initial guess */
    /* leave y size as supplied by toolbar */
    return(STATUS_OK);
}

PROC_EVENT_PROTO(extern, proc_event_ui_ss_direct)
{
    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        return(ss_msg_initclose(p_docu, (PC_MSG_INITCLOSE) p_data));

    case T5_MSG_TOOLBAR_TOOL_USER_VIEW_NEW:
        return(ui_ss_toolbar_tool_user_view_new(p_docu, (P_T5_TOOLBAR_TOOL_USER_VIEW_NEW) p_data));

    case T5_MSG_TOOLBAR_TOOL_USER_VIEW_DELETE:
        return(ui_ss_toolbar_tool_user_view_delete(p_docu, (PC_T5_TOOLBAR_TOOL_USER_VIEW_DELETE) p_data));

    case T5_MSG_TOOLBAR_TOOL_USER_SIZE_QUERY:
        return(ui_ss_toolbar_tool_user_size_query((P_T5_TOOLBAR_TOOL_USER_SIZE_QUERY) p_data));

    case T5_MSG_TOOLBAR_TOOL_USER_POSN_SET:
        return(sle_tool_user_posn_set(p_docu, p_data));

    case T5_MSG_TOOLBAR_TOOL_USER_REDRAW:
        return(sle_tool_user_redraw(p_docu, p_data));

    case T5_MSG_TOOLBAR_TOOL_USER_MOUSE:
        return(sle_tool_user_mouse(p_docu, p_data));

    default: default_unhandled();
        return(STATUS_OK);
    }
}

/* end of ui_ss.c */
