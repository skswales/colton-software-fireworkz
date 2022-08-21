/* ob_cells.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Event interface to skeleton from host for Fireworkz */

/* MRJC December 1991 / made cells object February 1994 */

#include "common/gflags.h"

#ifndef    __cells_ob_cells_h
#include "ob_cells/ob_cells.h"
#endif

#include "ob_toolb/xp_toolb.h"

#if RISCOS
#define MSG_WEAK &rb_cells_msg_weak
extern PC_U8 rb_cells_msg_weak;
#endif
#define P_BOUND_RESOURCES_OBJECT_ID_CELLS NULL

/*
internal routines
*/

OBJECT_PROTO(extern, object_cells);

OBJECT_PROTO(static, object_cells_default);

static void
caret_position_set_show(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     BOOL scroll);

static void
cells_reflect_focus_change(
    _DocuRef_   P_DOCU p_docu);

static void
cells_reflect_position(
    _DocuRef_   P_DOCU p_docu);

_Check_return_
static STATUS
cells_reflect_selection_new(
    _DocuRef_   P_DOCU p_docu);

_Check_return_
static STATUS
cursor_left(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message);

_Check_return_ _Success_(status_ok(return))
static STATUS
logical_move(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_SKEL_POINT p_skel_point,
    _InVal_     enum OBJECT_LOGICAL_MOVE_ACTION action,
    _OutRef_opt_ P_BOOL p_use_both_xy);

_Check_return_
static S32
row_has_height(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ROW row);

_Check_return_
static STATUS
spell_auto_check(
    _DocuRef_   P_DOCU p_docu);

enum CARET_ACTIONS
{
    CARET_TO_LEFT,
    CARET_STAY,
    CARET_TO_RIGHT
};

static ARRAY_HANDLE g_h_local_clip_data = 0;

static BOOL g_clip_data_from_cut_operation = 0;

static DOCU_AREA g_clip_data_docu_area; /* for OBJECT_ID_CELLS */

#if WINDOWS
static UINT cf_fireworkz = 0;
#endif

/******************************************************************************
*
* caret_position_after_command
*
* RISC OS: considered on each generic event exit
* Windows: processed using a null event handler
*
******************************************************************************/

#if WINDOWS

#define DCP_NULL_CLIENT_HANDLE ((CLIENT_HANDLE) 0x00000001)

_Check_return_
static STATUS
docu_caret_position_after_command_null(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     P_NULL_EVENT_BLOCK p_null_event_block)
{
    UNREFERENCED_PARAMETER_InRef_(p_null_event_block);

    trace_1(TRACE_OUT | TRACE_ANY, TEXT("docu_caret_position_after_command_null(docno=%d): null_event"), docno_from_p_docu(p_docu));

    assert(p_docu->flags.caret_position_after_command);

    status_assert(maeve_event(p_docu, T5_MSG_AFTER_SKEL_COMMAND, P_DATA_NONE));

    assert(!p_docu->flags.caret_position_after_command);

    return(STATUS_OK);
}

PROC_EVENT_PROTO(static, null_event_docu_caret_position_after_command)
{
    switch(t5_message)
    {
    case T5_EVENT_NULL:
        return(docu_caret_position_after_command_null(p_docu, (P_NULL_EVENT_BLOCK) p_data));

    default: default_unhandled();
        return(STATUS_OK);
    }
}

#endif /* OS */

static void
docu_flags_caret_position_after_command_clear(
    _DocuRef_   P_DOCU p_docu)
{
    if(!p_docu->flags.caret_position_after_command)
        return;

    p_docu->flags.caret_position_after_command = 0;

#if WINDOWS
    trace_1(TRACE_OUT | TRACE_ANY, TEXT("docu_flags_caret_position_after_command_clear(docno=%d) - *** null_events_stop()"), docno_from_p_docu(p_docu));
    null_events_stop(docno_from_p_docu(p_docu), T5_EVENT_NULL, null_event_docu_caret_position_after_command, DCP_NULL_CLIENT_HANDLE);
#endif
}

static void
docu_flags_caret_position_after_command_set(
    _DocuRef_   P_DOCU p_docu)
{
    if(p_docu->flags.caret_position_after_command)
        return;

    p_docu->flags.caret_position_after_command = 1;

#if WINDOWS
    trace_1(TRACE_OUT | TRACE_ANY, TEXT("docu_flags_caret_position_after_command_set(docno=%d) - *** null_events_start()"), docno_from_p_docu(p_docu));
    status_assert(null_events_start(docno_from_p_docu(p_docu), T5_EVENT_NULL, null_event_docu_caret_position_after_command, DCP_NULL_CLIENT_HANDLE));
#endif
}

static void
caret_position_after_command(
    _DocuRef_   P_DOCU p_docu)
{
    if(p_docu->flags.caret_position_later)
        return;

    docu_flags_caret_position_after_command_set(p_docu);
}

T5_MSG_PROTO(static, cells_msg_check_protection, P_CHECK_PROTECTION p_check_protection)
{
    DOCU_AREA docu_area;
    STATUS status = STATUS_OK;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(p_check_protection->use_docu_area)
        docu_area = p_check_protection->docu_area;
    else if(p_docu->mark_info_cells.h_markers)
    {
        docu_area_from_markers_first(p_docu, &docu_area);
    }
    else
        docu_area_from_position(&docu_area, &p_docu->cur);

    if(docu_area_is_cell_or_less(&docu_area))
    {
        /* ask object if it's editable */
        OBJECT_EDITABLE object_editable;
        object_editable.editable = TRUE;
        (void) cell_data_from_docu_area_tl(p_docu, &object_editable.object_data, &docu_area);
        if(status_ok(cell_call_id(object_editable.object_data.object_id, p_docu, T5_MSG_OBJECT_EDITABLE, &object_editable, NO_CELLS_EDIT)))
            if(!object_editable.editable)
                status = ERR_READ_ONLY;
    }

    if(status_ok(status))
    {
        STYLE style;
        STYLE_SELECTOR selector;
        STYLE_SELECTOR selector_fuzzy;

        style_init(&style);
        style_selector_clear(&selector);
        style_selector_bit_set(&selector, STYLE_SW_PS_PROTECT);
        style_selector_clear(&selector_fuzzy);

        style_of_area(p_docu, &style, &selector_fuzzy, &selector, &docu_area);

        if((style_bit_test(&style, STYLE_SW_PS_PROTECT) && style.para_style.protect)
           ||
           style_selector_bit_test(&selector_fuzzy, STYLE_SW_PS_PROTECT))
        {
            status = ERR_AREA_PROTECTED;
        }
    }

    /* report as required */
    if(status_fail(status) && p_check_protection->status_line_message)
    {
        UI_TEXT ui_text;
        ui_text.type = UI_TEXT_TYPE_RESID;
        ui_text.text.resource_id = status;
        status_line_set(p_docu, STATUS_LINE_LEVEL_AUTO_CLEAR, &ui_text);
        host_bleep();
    }

    return(status);
}

/******************************************************************************
*
* prepare an array handle suitable to host system-global clipboard data if wanted and needed
*
******************************************************************************/

_Check_return_
extern STATUS
clip_data_array_handle_prepare(
    _OutRef_    P_ARRAY_HANDLE p_h_clip_data,
    _OutRef_    P_BOOL p_f_local_clip_data)
{
    static /*poked*/ ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(BYTE), FALSE);

#if WINDOWS && defined(USE_GLOBAL_CLIPBOARD)
#define GLOBAL_CLIPBOARD_SAVE_OUTPUT_BUFFER_INC 512 /* smaller if going to Windows clipboard */
    array_init_block.size_increment = GLOBAL_CLIPBOARD_SAVE_OUTPUT_BUFFER_INC;

    array_init_block.use_alloc = ALLOC_USE_GLOBAL_ALLOC; /* will become owned by Windows clipboard */

    *p_f_local_clip_data = FALSE;
#else
    array_init_block.size_increment = 4096; /* as #define SAVE_OUTPUT_BUFFER_INC 4096 in of_save.c */

    *p_f_local_clip_data = TRUE;
#endif

    return(al_array_alloc_zero(p_h_clip_data, &array_init_block));
}

/******************************************************************************
*
* donate handle as application-local or system-global clipboard data
*
******************************************************************************/

extern void
clip_data_set(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     BOOL f_local_clip_data,
    _In_        ARRAY_HANDLE h_clip_data,
    _InVal_     BOOL clip_data_from_cut_operation,
    _InVal_     OBJECT_ID object_id)
{
    if(f_local_clip_data)
    {
        global_clip_data_dispose();
        local_clip_data_set(h_clip_data, clip_data_from_cut_operation);
    }
    else
    {
        local_clip_data_dispose();
        global_clip_data_set(p_docu, h_clip_data, clip_data_from_cut_operation, object_id);
    }
}

/******************************************************************************
*
* dispose of system-global clipboard data
*
******************************************************************************/

extern void
global_clip_data_dispose(void)
{
#if defined(USE_GLOBAL_CLIPBOARD)
    host_release_global_clipboard(FALSE);
#endif
}

/******************************************************************************
*
* donate handle as system-global clipboard data
*
******************************************************************************/

extern void
global_clip_data_set(
    _DocuRef_   P_DOCU p_docu,
    _In_        ARRAY_HANDLE h_clip_data,
    _InVal_     BOOL clip_data_from_cut_operation,
    _InVal_     OBJECT_ID object_id)
{
#if WINDOWS && defined(USE_GLOBAL_CLIPBOARD)
    global_clip_data_dispose();

    g_clip_data_from_cut_operation = clip_data_from_cut_operation;

#if CHECKING
    if(0 != h_clip_data)
    {
#if WINDOWS
        {
        PCTSTR leafname = TEXT("clip_set.fwk");
        const U32 leafname_len = tstrlen32(leafname);
        TCHARZ tempPathBuffer[MAX_PATH];
        /* Gets the temp path env string (no guarantee it's a valid path). */
        DWORD dwRetVal = GetTempPath(elemof32(tempPathBuffer), tempPathBuffer);
        /* Check that there is space to append our leafname */
        if((dwRetVal != 0) && ((dwRetVal + leafname_len) < elemof32(tempPathBuffer)))
        {
            tstr_xstrkat(tempPathBuffer, elemof32(tempPathBuffer), leafname);
            status_assert(clip_data_save(tempPathBuffer, &h_clip_data));
        }
        } /*block*/
#elif RISCOS
        status_assert(clip_data_save(TEXT("$.Temp.clip_set/fwk"), &h_clip_data));
#endif
    }
#endif /* CHECKING */

    /* Put on Windows clipboard best-format-first after first acquiring clipboard (emptying establishes ownership) */
    if(host_acquire_global_clipboard(p_docu, p_view_from_viewno_caret(p_docu)))
    {
        { /* render(ed by saver) as Fireworkz already */
        HGLOBAL hMem;
        al_array_trim(&h_clip_data);
        hMem = al_array_steal_hglobal(&h_clip_data);
        if(WrapOsBoolChecking(NULL != SetClipboardData(cf_fireworkz, hMem)))
        { /*EMPTY*/ }
        } /*block*/

        if(!clip_data_from_cut_operation || (OBJECT_ID_CELLS != object_id))
        {
            /* defer rendering in other formats until requested for copy - can't do for cut!
             * NB won't be precise if this doc is modified before paste in other app!
             */

            /* Always offer CF_TEXT */
            /* NB NULL data incorrectly triggers non-success */
            (void) SetClipboardData(CF_TEXT, NULL);

            { /* Then loop over all clipboard formats that we've registered */
            ARRAY_INDEX i;

            for(i = 0; i < array_elements(&installed_save_objects_handle); ++i)
            {
                P_INSTALLED_SAVE_OBJECT p_installed_save_object = array_ptr(&installed_save_objects_handle, INSTALLED_SAVE_OBJECT, i);

                if(0 != p_installed_save_object->uClipboardFormat)
                    (void) SetClipboardData(p_installed_save_object->uClipboardFormat, NULL);
            }
            } /*block*/
        }

        CloseClipboard();

        return;
    }

    /*clip_issue_change();*/
#else
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(object_id);
#endif /* USE_GLOBAL_CLIPBOARD */

    /* no usable global clipboard - save to local clipboard as before */
    local_clip_data_set(h_clip_data, clip_data_from_cut_operation);
}

/******************************************************************************
*
* dispose of application-local clipboard data
*
******************************************************************************/

extern void
local_clip_data_dispose(void)
{
    al_array_dispose(&g_h_local_clip_data);
}

/******************************************************************************
*
* return application-local clipboard data
*
******************************************************************************/

_Check_return_
extern ARRAY_HANDLE
local_clip_data_query(void)
{
    return(g_h_local_clip_data);
}

/******************************************************************************
*
* donate handle as application-local clipboard data
*
******************************************************************************/

extern void
local_clip_data_set(
    _InVal_     ARRAY_HANDLE h_clip_data,
    _InVal_     BOOL clip_data_from_cut_operation)
{
    local_clip_data_dispose();

    g_h_local_clip_data = h_clip_data;

    g_clip_data_from_cut_operation = clip_data_from_cut_operation;

#if CHECKING
    if(0 != g_h_local_clip_data)
    {
#if WINDOWS
        {
        PCTSTR leafname = TEXT("clip_set.fwk");
        const U32 leafname_len = tstrlen32(leafname);
        TCHARZ tempPathBuffer[MAX_PATH];
        /* Gets the temp path env string (no guarantee it's a valid path). */
        DWORD dwRetVal = GetTempPath(elemof32(tempPathBuffer), tempPathBuffer);
        /* Check that there is space to append our leafname */
        if((dwRetVal != 0) && ((dwRetVal + leafname_len) < elemof32(tempPathBuffer)))
        {
            tstr_xstrkat(tempPathBuffer, elemof32(tempPathBuffer), leafname);
            status_assert(clip_data_save(tempPathBuffer, &g_h_local_clip_data));
        }
        } /*block*/
#elif RISCOS
        status_assert(clip_data_save(TEXT("$.Temp.clip_set/fwk"), &g_h_local_clip_data));
#endif
    }
#endif /* CHECKING */

    /*clip_issue_change();*/
}

#ifdef UNDEF

/******************************************************************************
*
* issue a clipboard change message
*
******************************************************************************/

static void
clip_issue_change(void)
{
    /* no longer needed maeve_event(spam, T5_MSG_CLIPBOARD_NEW, P_DATA_NONE); */
}

#endif /* UNDEF */

#if WINDOWS && defined(USE_GLOBAL_CLIPBOARD)

_Check_return_
static STATUS
load_foreign_from_array_handle(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_ARRAY_HANDLE p_array_handle,
    _InVal_     T5_FILETYPE t5_filetype)
{
    STATUS status = STATUS_OK;
    const OBJECT_ID object_id = object_id_from_t5_filetype(t5_filetype);
    MSG_INSERT_FOREIGN msg_insert_foreign;
    zero_struct(msg_insert_foreign);

    if(OBJECT_ID_NONE == object_id)
        return(create_error(ERR_UNKNOWN_FILETYPE));

    cur_change_before(p_docu);

    msg_insert_foreign.filename = NULL;
    msg_insert_foreign.t5_filetype = t5_filetype;
    msg_insert_foreign.scrap_file = TRUE;
    msg_insert_foreign.insert = TRUE;
    msg_insert_foreign.ctrl_pressed = FALSE;
    /***msg_insert_foreign.of_ip_format.flags.insert = 1;*/
    msg_insert_foreign.position = p_docu->cur;
    msg_insert_foreign.array_handle = *p_array_handle;
    status = object_call_id_load(p_docu, T5_MSG_INSERT_FOREIGN, &msg_insert_foreign, object_id);

    cur_change_after(p_docu);

    return(status);
}

_Check_return_
static UINT
determine_windows_clipboard_filetype(
    _OutRef_    P_T5_FILETYPE p_t5_filetype)
{
    UINT uFormat = 0; /* start enumeration */

    *p_t5_filetype = FILETYPE_TEXT;

    while(0 != (uFormat = EnumClipboardFormats(uFormat)))
    {
        if(cf_fireworkz == uFormat)
        {
            assert0(); /* should have already explicitly got this one! */
            *p_t5_filetype = FILETYPE_T5_FIREWORKZ;
            break;
        }

        /* use the first one that in the list that we can handle as that's the donor's preferred export format */
        if(CF_TEXT == uFormat)
        {
            *p_t5_filetype = FILETYPE_TEXT;
            break;
        }

        if(0xC000 <= uFormat)
        {
            TCHARZ buffer[256];
            int len = GetClipboardFormatName(uFormat, buffer, elemof32(buffer));

            if(0 != len)
            {
                buffer[len] = CH_NULL; /* so trusting... (not) */
                if(0 == tstricmp(TEXT("Biff8"), buffer))
                {
                    *p_t5_filetype = FILETYPE_MS_XLS;
                    break;
                }
                if(0 == tstricmp(TEXT("Biff5"), buffer))
                {
                    *p_t5_filetype = FILETYPE_MS_XLS;
                    break;
                }
                if(0 == tstricmp(TEXT("Biff4"), buffer))
                {
                    *p_t5_filetype = FILETYPE_MS_XLS;
                    break;
                }
                if(0 == tstricmp(TEXT("Biff3"), buffer))
                {
                    *p_t5_filetype = FILETYPE_MS_XLS;
                    break;
                }
                if(0 == tstricmp(TEXT("Biff"), buffer))
                {
                    *p_t5_filetype = FILETYPE_MS_XLS;
                    break;
                }
                if(0 == tstricmp(TEXT("Wk1"), buffer))
                {
                    *p_t5_filetype = FILETYPE_LOTUS123;
                    break;
                }
                if(0 == tstricmp(TEXT("Csv"), buffer))
                {
                    *p_t5_filetype = FILETYPE_CSV;
                    break;
                }
                if(0 == tstricmp(TEXT("Rich Text Format"), buffer))
                {
                    *p_t5_filetype = FILETYPE_RTF;
                    break;
                }
            }
        }
    }

    if(0 == uFormat)
        void_WrapOsBoolChecking(0 == GetLastError());

    return(uFormat);
}

static void
report_clipboard_format(
    _In_z_  PCTSTR routine,
    _InVal_ UINT uFormat)
{
    TCHARZ tszFormatName[128];

    if(CF_TEXT == uFormat)
        tstr_xstrkpy(tszFormatName, elemof32(tszFormatName), TEXT("CF_TEXT"));
    else if(!WrapOsBoolChecking(GetClipboardFormatName(uFormat, tszFormatName, elemof32(tszFormatName))))
        tszFormatName[0] = CH_NULL;

    reportf(TEXT("%s: GetClipboardFormatName(uFormat=") U32_XTFMT TEXT("): %s"), report_tstr(routine), (U32) uFormat, report_tstr(tszFormatName));
}

_Check_return_
static STATUS
load_from_windows_clipboard(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     BOOL clip_data_from_cut_operation)
{
    STATUS status = STATUS_OK;
    UINT aFormatPriorityList[1];
    int res;
    UINT uFormat;
    T5_FILETYPE t5_filetype = FILETYPE_TEXT;

    if(!host_open_global_clipboard(p_docu, p_view_from_viewno_caret(p_docu)))
        return(STATUS_OK); /* can't lock Windows clipboard for our use */

    /* try to get Fireworkz format */
    aFormatPriorityList[0] = cf_fireworkz;
    res = GetPriorityClipboardFormat(aFormatPriorityList, elemof(aFormatPriorityList));
    if(cf_fireworkz == (UINT) res)
    {   /* Windows clipboard contains Fireworkz format data*/
        uFormat = cf_fireworkz;
        t5_filetype = FILETYPE_T5_FIREWORKZ;
    }
    else if(0 == res)
    {   /* Windows clipboard is empty */
        uFormat = 0;
        t5_filetype = FILETYPE_TEXT;
    }
    else
    {   /* Windows clipboard contains some other format data */
        uFormat = determine_windows_clipboard_filetype(&t5_filetype);
    }

    if(0 != uFormat)
    {
        HANDLE handle = GetClipboardData(uFormat);

        if(NULL != handle)
        {
            P_BYTE p_clip_data = (P_BYTE) GlobalLock(handle);

            if(NULL != p_clip_data)
            {
                SIZE_T n = GlobalSize(handle);
                SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(BYTE), FALSE);
                ARRAY_HANDLE array_handle = 0;
                P_BYTE core;

                if(CF_TEXT == uFormat)
                    n = lstrlen((PCTSTR) p_clip_data);

                if(NULL != (core = al_array_alloc_BYTE(&array_handle, (U32) n, &array_init_block, &status)))
                {
                    memcpy32(core, p_clip_data, (U32) n);

#if WINDOWS
                    {
                    PCTSTR leafname = TEXT("clip_get.dat");
                    const U32 leafname_len = tstrlen32(leafname);
                    TCHARZ tempPathBuffer[MAX_PATH];
                    /* Gets the temp path env string (no guarantee it's a valid path). */
                    DWORD dwRetVal = GetTempPath(elemof32(tempPathBuffer), tempPathBuffer);
                    /* Check that there is space to append our leafname */
                    if((dwRetVal != 0) && ((dwRetVal + leafname_len) < elemof32(tempPathBuffer)))
                    {
                        tstr_xstrkat(tempPathBuffer, elemof32(tempPathBuffer), leafname);
                        status_assert(clip_data_save(tempPathBuffer, &array_handle));
                    }
                    } /*block*/
#endif /* OS */

                    if(cf_fireworkz == uFormat)
                    {
                        report_clipboard_format(TEXT("load_from_windows_clipboard"), uFormat);
                        reportf(TEXT("load_from_windows_clipboard: size=") UINTPTR_XTFMT, n);
                        status = load_ownform_from_array_handle(p_docu, &array_handle, P_POSITION_NONE, clip_data_from_cut_operation);
                    }
                    else
                    {
                        report_clipboard_format(TEXT("load_from_windows_clipboard"), uFormat);
                        reportf(TEXT("load_from_windows_clipboard: size=") UINTPTR_XTFMT, n);
                        status = load_foreign_from_array_handle(p_docu, &array_handle, t5_filetype);
                    }

                    al_array_dispose(&array_handle);
                }

                GlobalUnlock(handle);
            }
        }
    }

    trace_0(TRACE_WINDOWS_HOST, TEXT("load_from_windows_clipboard: CloseClipboard"));
    CloseClipboard();

    return(status);
}

_Check_return_
extern STATUS
clip_render_format_for_filetype(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     UINT uFormat,
    _InVal_     T5_FILETYPE t5_filetype)
{
    STATUS status = STATUS_OK;
    P_DOCU_AREA p_docu_area = &g_clip_data_docu_area;
    /* see clip_data_array_handle_prepare() */
    ARRAY_HANDLE h_global_clip_data = 0; /* will become owned by Windows clipboard */
    static /*poked*/ ARRAY_INIT_BLOCK array_init_block = aib_init(GLOBAL_CLIPBOARD_SAVE_OUTPUT_BUFFER_INC, sizeof32(BYTE), FALSE);
    array_init_block.use_alloc = ALLOC_USE_GLOBAL_ALLOC;

    status = al_array_alloc_zero(&h_global_clip_data, &array_init_block);

#if CHECKING || 1
    report_clipboard_format(TEXT("clip_render_format_for_filetype"), uFormat);
#endif

    if(status_ok(status))
        status = save_foreign_to_array_from_docu_area(p_docu, &h_global_clip_data, t5_filetype, p_docu_area);

    if(status_ok(status))
    {
        switch(uFormat)
        {
        case CF_TEXT:
            status = al_array_add(&h_global_clip_data, BYTE, 1, PC_ARRAY_INIT_BLOCK_NONE, empty_string);
            break;

        default:
            break;
        }
    }

    if(status_ok(status))
    {
        HGLOBAL hMem;
        al_array_trim(&h_global_clip_data);
        hMem = al_array_steal_hglobal(&h_global_clip_data);
        if(WrapOsBoolChecking(NULL != SetClipboardData(uFormat, hMem)))
        { /*EMPTY*/ }
    }

    return(status);
}

_Check_return_
extern STATUS
clip_render_format(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     UINT uFormat)
{
    T5_FILETYPE t5_filetype = FILETYPE_UNDETERMINED;

#if CHECKING || 1
    report_clipboard_format(TEXT("clip_render_format"), uFormat);
#endif

    switch(uFormat)
    {
    case CF_TEXT:
        t5_filetype = FILETYPE_TEXT;
        break;

    default:
        { /* loop over all Windows clipboard formats that we've registered */
        ARRAY_INDEX i;

        for(i = 0; i < array_elements(&installed_save_objects_handle); ++i)
        {
            P_INSTALLED_SAVE_OBJECT p_installed_save_object = array_ptr(&installed_save_objects_handle, INSTALLED_SAVE_OBJECT, i);

            if(p_installed_save_object->uClipboardFormat == uFormat)
            {
                t5_filetype = p_installed_save_object->t5_filetype;
                break;
            }
        }

        break;
        } /*block*/
    }

    if(FILETYPE_UNDETERMINED == t5_filetype)
        return(STATUS_FAIL);

    return(clip_render_format_for_filetype(p_docu, uFormat, t5_filetype));
}

_Check_return_
extern STATUS
clip_render_all_formats(
    _DocuRef_   P_DOCU p_docu)
{
    STATUS status;
    ARRAY_INDEX i;

    /* Always render CF_TEXT */
    status = clip_render_format_for_filetype(p_docu, CF_TEXT, FILETYPE_TEXT);

    /* Then loop over all Windows clipboard formats that we've registered */
    for(i = 0; status_ok(status) && (i < array_elements(&installed_save_objects_handle)); ++i)
    {
        P_INSTALLED_SAVE_OBJECT p_installed_save_object = array_ptr(&installed_save_objects_handle, INSTALLED_SAVE_OBJECT, i);
        const UINT uFormat = p_installed_save_object->uClipboardFormat;

        if(0 != uFormat)
        {
#if CHECKING || 1
            report_clipboard_format(TEXT("clip_render_all_formats"), uFormat);
#endif
            status_return(clip_render_format_for_filetype(p_docu, uFormat, p_installed_save_object->t5_filetype));
        }
    }

    return(status);
}

#endif /* WINDOWS && USE_GLOBAL_CLIPBOARD */

/******************************************************************************
*
* save docu_area to array to probably stick on clipboard
*
******************************************************************************/

_Check_return_
static STATUS
cells_save_clip_data(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     P_ARRAY_HANDLE p_h_clip_data,
    _InRef_     PC_DOCU_AREA p_docu_area)
{
    STATUS status;
    OF_TEMPLATE of_template = { 0 };
    BOOL do_cur_change = FALSE;

    of_template.used_styles = 1;

    if(docu_area_is_frag(p_docu_area))
        of_template.data_class = DATA_SAVE_CHARACTER;
    else
    {
        of_template.data_class = DATA_SAVE_MANY;
        do_cur_change = TRUE;
    }

    if(do_cur_change)
        cur_change_before(p_docu);

    status = save_ownform_to_array_from_docu_area(p_docu, p_h_clip_data, &of_template, p_docu_area);

    if(do_cur_change)
        cur_change_after(p_docu);

    return(status);
}

/******************************************************************************
*
* join current cell to next cell
*
******************************************************************************/

_Check_return_
static STATUS
join_cells(
    _DocuRef_   P_DOCU p_docu)
{
    STATUS status = STATUS_OK;
    OBJECT_POSITION_SET object_position_set;
    DOCU_AREA docu_area;

    docu_area_from_position(&docu_area, &p_docu->cur);
    consume_bool(cell_data_from_position(p_docu, &object_position_set.object_data, &docu_area.tl));
    object_position_set.action = OBJECT_POSITION_SET_START;
    if(status_done(cell_call_id(object_position_set.object_data.object_id,
                                p_docu,
                                T5_MSG_OBJECT_POSITION_SET,
                                &object_position_set,
                                NO_CELLS_EDIT)))
    {
        /* special case to clear fragment status on blank objects */
        if(object_position_at_start(&docu_area.tl.object_position))
            object_position_init(&docu_area.tl.object_position);

        if(docu_area.br.slr.row < n_rows(p_docu))
        {
            POSITION br_pos = docu_area.tl; /* SKS 04feb93 - try going to next ***row*** */
            br_pos.slr.row += 1;

            consume_bool(cell_data_from_position(p_docu, &object_position_set.object_data, &br_pos));
            object_position_set.action = OBJECT_POSITION_SET_START;
            if(status_done(cell_call_id(object_position_set.object_data.object_id,
                                        p_docu,
                                        T5_MSG_OBJECT_POSITION_SET,
                                        &object_position_set,
                                        NO_CELLS_EDIT)))
            {
                docu_area.br.object_position = object_position_set.object_data.object_position_start;
                docu_area.br.slr.row += 1;
            }
            else
                docu_area.br.object_position.object_id = OBJECT_ID_NONE;

            status = cells_docu_area_delete(p_docu, &docu_area, FALSE, FALSE);
            caret_position_after_command(p_docu);
        }
    }

    return(status);
}

/******************************************************************************
*
* if there is a selection, delete it
*
* doesn't copy selection to clipboard, that's higher-level's choice
*
******************************************************************************/

_Check_return_
static STATUS /* STATUS_DONE = did it */
delete_selection(
    _DocuRef_   P_DOCU p_docu)
{
    STATUS status = STATUS_OK;

    if(p_docu->mark_info_cells.h_markers)
    {
        DOCU_AREA docu_area;

        docu_area_from_markers_first(p_docu, &docu_area);

        status_assert(maeve_event(p_docu, T5_MSG_SELECTION_CLEAR, P_DATA_NONE));

        if(status_ok(status))
        {
            status = cells_docu_area_delete(p_docu, &docu_area, docu_area_spans_across_table(p_docu, &docu_area), TRUE);
            caret_position_after_command(p_docu);
        }

        if(status_ok(status))
            status = STATUS_DONE;
    }

    return(status);
}

/*
* delete character to the right of p_docu->cur
*/

_Check_return_
static STATUS
cells_cmd_delete_character_right(
    _DocuRef_   P_DOCU p_docu)
{
    STATUS status = delete_selection(p_docu); /* doesn't copy selection to clipboard */

    if(STATUS_OK == status)
    {
        DOCU_AREA docu_area;
        OBJECT_POSITION_SET object_position_set;

        /* save start and end coordinates */
        docu_area_from_position(&docu_area, &p_docu->cur);

        consume_bool(cell_data_from_position(p_docu, &object_position_set.object_data, &docu_area.tl));
        object_position_set.action = OBJECT_POSITION_SET_FORWARD;

        if(status_done(cell_call_id(object_position_set.object_data.object_id,
                                    p_docu,
                                    T5_MSG_OBJECT_POSITION_SET,
                                    &object_position_set,
                                    NO_CELLS_EDIT)))
        {
            /* delete single character in cell */
            docu_area.br.object_position = object_position_set.object_data.object_position_start;
            status = cells_docu_area_delete(p_docu, &docu_area, FALSE, TRUE);
            caret_position_after_command(p_docu);
        }
        else
        {
            if(cell_in_table(p_docu, &p_docu->cur.slr))
            {
                if(OBJECT_ID_NONE == p_docu->cur.object_position.object_id)
                    status = cells_block_blank_make(p_docu, p_docu->cur.slr.col, p_docu->cur.slr.col + 1, p_docu->cur.slr.row, 1);
            }
            else
                status = join_cells(p_docu);
        }
    }
    /* otherwise DONE or error */

    return(status);
}

static STATUS
cells_cmd_delete_character_left(
    _DocuRef_   P_DOCU p_docu)
{
    STATUS status = delete_selection(p_docu); /* doesn't copy selection to clipboard */

    if(STATUS_OK == status)
    {
        BOOL in_table = cell_in_table(p_docu, &p_docu->cur.slr);

        if(OBJECT_ID_NONE != p_docu->cur.object_position.object_id)
        {
            if(!in_table || p_docu->cur.object_position.data != 0)
                if(status_done(cursor_left(p_docu, T5_CMD_CURSOR_LEFT)))
                    status = cells_cmd_delete_character_right(p_docu);
        }
        else
        {
            if(in_table)
                status = cells_block_blank_make(p_docu, p_docu->cur.slr.col, p_docu->cur.slr.col + 1, p_docu->cur.slr.row, 1);
        }
    }
    /* otherwise DONE or error */

    return(status);
}

/*
routines for keyboard selection
*/

static void
drop_anchor(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    _OutRef_    P_BOOL p_key_selection,
    _OutRef_    P_BOOL p_extend_selection)
{
    *p_extend_selection = FALSE;

    switch(t5_message)
    {
    case T5_CMD_SHIFT_WORD_LEFT:
    case T5_CMD_SHIFT_WORD_RIGHT:
    case T5_CMD_SHIFT_CURSOR_LEFT:
    case T5_CMD_SHIFT_CURSOR_RIGHT:
    case T5_CMD_SHIFT_LINE_START:
    case T5_CMD_SHIFT_LINE_END:
    case T5_CMD_SHIFT_CURSOR_UP:
    case T5_CMD_SHIFT_CURSOR_DOWN:
    case T5_CMD_SHIFT_PAGE_UP:
    case T5_CMD_SHIFT_PAGE_DOWN:
    case T5_CMD_SHIFT_DOCUMENT_TOP:
    case T5_CMD_SHIFT_DOCUMENT_BOTTOM:
        *p_key_selection = TRUE;
        break;

    default:
        *p_key_selection = FALSE;
        break;
    }

    if(*p_key_selection)
    {
        *p_extend_selection = (p_docu->mark_info_cells.h_markers != 0);

        if(!*p_extend_selection)
        {
            docu_area_init(&p_docu->anchor_mark.docu_area);
            p_docu->anchor_mark.docu_area.tl = p_docu->cur;
            p_docu->anchor_mark.docu_area.br = p_docu->cur;
        }
    }
}

static void
move_ship(
    _DocuRef_   P_DOCU p_docu,
    P_POSITION p_position,
    _InVal_     BOOL extend_selection)
{
    p_docu->anchor_mark.docu_area.br = *p_position;

    if(!extend_selection)
        status_assert(anchor_to_markers_new(p_docu));

    anchor_to_markers_finish(p_docu);
    markers_show(p_docu);
}

/******************************************************************************
*
* --in--
* data_ref in OBJECT_POSITION_FIND
* skel_point
*
* --out--
* OBJECT_POSITION_FIND set up for actual
* position from skel_point
*
******************************************************************************/

static void
position_set_from_skel_point(
    _DocuRef_   P_DOCU p_docu,
    P_OBJECT_POSITION_FIND p_object_position_find,
    _InRef_opt_ PC_SKEL_POINT p_skel_point,
    _InRef_     PC_SLR p_slr,
    _InVal_     BOOL allow_cells_edit)
{
    skel_rect_from_slr(p_docu, &p_object_position_find->skel_rect, p_slr);
    (void) cell_data_from_slr(p_docu, &p_object_position_find->object_data, p_slr);

    /* send message to object to get position */
    if(NULL != p_skel_point)
        p_object_position_find->pos = *p_skel_point;
    else
        p_object_position_find->pos = p_object_position_find->skel_rect.tl;

    if(status_fail(cell_call_id(p_object_position_find->object_data.object_id,
                                p_docu,
                                T5_MSG_OBJECT_POSITION_FROM_SKEL_POINT,
                                p_object_position_find,
                                allow_cells_edit)))
    {
        p_object_position_find->object_data.object_position_start.object_id = OBJECT_ID_NONE;
        p_object_position_find->pos = p_object_position_find->skel_rect.tl;
        p_object_position_find->caret_height = 0;
    }
}

/******************************************************************************
*
* a rather grizzly key processing routine
* which contains special case handling
* that could be generalised but isn't
* MRJC 18.10.94
*
******************************************************************************/

_Check_return_
static STATUS
process_keys(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    /*_Inout_*/ P_ANY p_data,
    P_OBJECT_DATA p_object_data,
    _InVal_     UCS4 key_ch)
{
    STATUS status = STATUS_OK;

    /* did we manage to set a sub-object position on this cell ? */
    if(OBJECT_ID_NONE == p_docu->cur.object_position.object_id)
    {
        /* No. All non-SS objects here do in-cell editing */
        if(OBJECT_ID_SS != p_object_data->object_id)
            p_object_data->object_id = OBJECT_ID_CELLS_EDIT;
        /* if it's an SS object and non-blank it must be a formula (otherwise we'd have a sub-object position),
         * otherwise use in-cell editing unless forbidden to or we typed equals sign to introduce a formula
         */
        else if((P_DATA_NONE == p_object_data->u.p_object) && (global_preferences.ss_edit_in_cell) && (CH_EQUALS_SIGN != key_ch))
            p_object_data->object_id = OBJECT_ID_CELLS_EDIT;
        else
            p_object_data->object_id = OBJECT_ID_SLE;
    }
    else
        p_object_data->object_id = p_docu->cur.object_position.object_id;

    status = cell_call_id(p_object_data->object_id, p_docu, t5_message, p_data, NO_CELLS_EDIT);

    if(status_ok(status))
    {
        if((p_docu->focus_owner == OBJECT_ID_CELLS) && (OBJECT_ID_NONE != p_object_data->object_position_end.object_id))
        {
            p_docu->cur.object_position = p_object_data->object_position_end;
            status = spell_auto_check(p_docu);
            caret_position_after_command(p_docu);
        }
    }

    return(status);
}

_Check_return_
static STATUS
cells_msg_cur_change_before(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_REGION p_region)
{
    status_assert(maeve_event(p_docu, T5_MSG_CELL_MERGE, P_DATA_NONE));

    {
    CACHES_DISPOSE caches_dispose;
    if(P_DATA_NONE != p_region)
        caches_dispose.region = *p_region;
    else
        region_from_two_slrs(&caches_dispose.region, &p_docu->cur.slr, &p_docu->cur.slr, TRUE);
    caches_dispose.data_space = DATA_NONE;
    status_assert(maeve_event(p_docu, T5_MSG_CACHES_DISPOSE, &caches_dispose));
    } /*block*/

    return(STATUS_OK);
}

_Check_return_
static STATUS
cells_msg_cur_change_after(
    _DocuRef_   P_DOCU p_docu)
{
    if(p_docu->focus_owner == OBJECT_ID_CELLS)
        caret_position_after_command(p_docu);
    else
        caret_show_claim(p_docu, p_docu->focus_owner, TRUE);

    return(STATUS_OK);
}

_Check_return_
static STATUS
cells_msg_docu_colrow(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_SLR p_slr_old)
{
    /* cause redraw of current and old cells if style is different */
    if(style_at_or_above_class(p_docu, &p_docu->h_style_docu_area, REGION_UPPER) && !slr_equal(&p_docu->cur.slr, p_slr_old))
    {
        RECT_FLAGS rect_flags;
        RECT_FLAGS_CLEAR(rect_flags);

        /* include bottom and right grid owned by next cell */
        rect_flags.extend_right_by_1 = 1;
        rect_flags.extend_down_by_1  = 1;

        if(slr_in_range(p_docu, p_slr_old))
        {
            CACHES_DISPOSE caches_dispose;
            SKEL_RECT skel_rect;
            region_from_two_slrs(&caches_dispose.region, p_slr_old, p_slr_old, TRUE);
            caches_dispose.data_space = DATA_SLOT;
            status_assert(maeve_event(p_docu, T5_MSG_CACHES_DISPOSE, &caches_dispose));
            skel_rect_from_slr(p_docu, &skel_rect, p_slr_old);
            view_update_later(p_docu, UPDATE_PANE_CELLS_AREA, &skel_rect, rect_flags);
        }

        {
        CACHES_DISPOSE caches_dispose;
        region_from_two_slrs(&caches_dispose.region, &p_docu->cur.slr, &p_docu->cur.slr, TRUE);
        caches_dispose.data_space = DATA_SLOT;
        status_assert(maeve_event(p_docu, T5_MSG_CACHES_DISPOSE, &caches_dispose));
        } /*block*/

        {
        SKEL_RECT skel_rect;
        skel_rect_from_slr(p_docu, &skel_rect, &p_docu->cur.slr);
        view_update_later(p_docu, UPDATE_PANE_CELLS_AREA, &skel_rect, rect_flags);
        } /*block*/
    }

    p_docu->old = p_docu->cur;

    cells_reflect_position(p_docu);

    return(STATUS_OK);
}

_Check_return_
static STATUS
cells_msg_after_skel_command(
    _DocuRef_   P_DOCU p_docu)
{
    if(p_docu->flags.caret_position_after_command)
    {
        docu_flags_caret_position_after_command_clear(p_docu);

        if(p_docu->focus_owner == OBJECT_ID_CELLS)
            caret_position_set_show(p_docu, TRUE);
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
cells_event_redraw_after(
    _DocuRef_   P_DOCU p_docu)
{
    if(p_docu->flags.caret_position_later || p_docu->flags.caret_scroll_later)
        if(p_docu->focus_owner == OBJECT_ID_CELLS)
            caret_position_set_show(p_docu, p_docu->flags.caret_scroll_later);

    return(STATUS_OK);
}

/*
main events
*/

MAEVE_EVENT_PROTO(static, maeve_event_ob_cells)
{
    UNREFERENCED_PARAMETER_InRef_(p_maeve_block);

    switch(t5_message)
    {
    case T5_MSG_SELECTION_NEW:
        return(cells_reflect_selection_new(p_docu));

    case T5_MSG_FOCUS_CHANGED:
        cells_reflect_focus_change(p_docu);
        cells_reflect_position(p_docu);
        break;

    case T5_MSG_CUR_CHANGE_BEFORE:
        return(cells_msg_cur_change_before(p_docu, (PC_REGION) p_data));

    case T5_MSG_CUR_CHANGE_AFTER:
        return(cells_msg_cur_change_after(p_docu));

    case T5_MSG_DOCU_COLROW:
        return(cells_msg_docu_colrow(p_docu, (PC_SLR) p_data));

    case T5_MSG_DOCU_CARETMOVE:
        p_docu->old.object_position = p_docu->cur.object_position;
        break;

    case T5_MSG_AFTER_SKEL_COMMAND:
        return(cells_msg_after_skel_command(p_docu));

    case T5_EVENT_REDRAW_AFTER:
        return(cells_event_redraw_after(p_docu));

    default:
        break;
    }

    return(STATUS_OK);
}

/******************************************************************************
*
* set and show new caret position
*
******************************************************************************/

static void
caret_position_set_show(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     BOOL scroll)
{
    OBJECT_DATA object_data;
    SKEL_RECT skel_rect_cur;
    BOOL found_caret = FALSE, can_edit_object = TRUE;

    if(!p_docu->flags.document_active)
        return;

    { /* check that we haven't left cur.slr in space */
    SLR extent;
    extent.col = n_cols_logical(p_docu);
    extent.row = n_rows(p_docu);
    p_docu->cur.slr.col = MIN(p_docu->cur.slr.col,  extent.col - 1);
    p_docu->cur.slr.row = MIN(p_docu->cur.slr.row, extent.row - 1);
    } /*block*/

    /* the object_id read here will be: 1) the active object (e.g. CELLS_EDIT if editing)
                                     or 2) the stored object (e.g. TEXT or SS) if there's an object
                                     or 3) the 'new type' object if cell is blank
    */

    consume_bool(cell_data_from_position(p_docu, &object_data, &p_docu->cur));

    if(status_fail(check_protection_simple(p_docu, FALSE)))
        can_edit_object = FALSE;

    /* try to set caret sub-object position */
    if((OBJECT_ID_NONE == p_docu->cur.object_position.object_id) && can_edit_object)
    {
        OBJECT_POSITION_SET object_position_set;
        object_position_set.object_data = object_data;
        object_position_set.action = OBJECT_POSITION_SET_START;
        if(status_done(cell_call_id(object_position_set.object_data.object_id,
                                    p_docu,
                                    T5_MSG_OBJECT_POSITION_SET,
                                    &object_position_set,
                                    can_edit_object)))
            p_docu->cur.object_position = object_position_set.object_data.object_position_start;
        else
            p_docu->cur.object_position.object_id = OBJECT_ID_NONE;
    }

    skel_rect_from_slr(p_docu, &skel_rect_cur, &p_docu->cur.slr);

    /* use object position to get pixit position of caret
    */
    if(skel_rect_cur.tl.pixit_point.x < skel_rect_cur.br.pixit_point.x
       &&
       object_present(object_data.object_id)
       &&
       can_edit_object)
    {
        OBJECT_POSITION_FIND object_position_find;
        object_position_find.object_data = object_data;
        object_position_find.skel_rect = skel_rect_cur;
        object_position_find.object_data.object_position_start = p_docu->cur.object_position;

        if(status_ok(cell_call_id(object_data.object_id,
                                  p_docu,
                                  T5_MSG_SKEL_POINT_FROM_OBJECT_POSITION,
                                  &object_position_find,
                                  can_edit_object)))
        {
            p_docu->caret_height = object_position_find.caret_height;
            p_docu->caret = object_position_find.pos;
            edge_set_from_skel_point(&p_docu->caret_x, &p_docu->caret, x);
            found_caret = 1;
        }
    }

    if(!found_caret)
    {
        /* invisible object - switch off caret */
        p_docu->caret = skel_rect_cur.tl;
        edge_set_from_skel_point(&p_docu->caret_x, &p_docu->caret, x);
        p_docu->caret_height = 0;
    }

    /* check for change of current cell */
    if(!slr_equal(&p_docu->cur.slr, &p_docu->old.slr))
    {
        SLR slr_old = p_docu->old.slr;
        status_assert(maeve_event(p_docu, T5_MSG_DOCU_COLROW, &slr_old));
    }
    /* issue caret move message */
    else if(position_compare(&p_docu->cur, &p_docu->old))
        status_assert(maeve_event(p_docu, T5_MSG_DOCU_CARETMOVE, P_DATA_NONE));

    /* SKS 02jun93 after 1.03 - do prior to showing caret so we can actually scroll the window there */
    if(p_docu->flags.new_extent)
        docu_set_and_show_extent_all_views(p_docu);

    view_show_caret(p_docu, UPDATE_PANE_CELLS_AREA, &p_docu->caret, p_docu->mark_info_cells.h_markers ? 0 : p_docu->caret_height);

    if(p_docu->flags.caret_scroll_later || scroll)
    {
        PIXIT top = 0, bottom = 0;

        if((p_docu->caret_height == 0) || (OBJECT_ID_REC_FLOW == p_docu->object_id_flow))
        {
            top = (skel_rect_cur.tl.page_num.y == p_docu->caret.page_num.y)
                        ? skel_rect_cur.tl.pixit_point.y - p_docu->caret.pixit_point.y
                        : 0;
            bottom = (skel_rect_cur.br.page_num.y == p_docu->caret.page_num.y)
                        ? skel_rect_cur.br.pixit_point.y - p_docu->caret.pixit_point.y
                        : page_height_from_row(p_docu, p_docu->cur.slr.row);
        }
        else
            bottom = p_docu->caret_height;

        view_scroll_caret(p_docu,
                          UPDATE_PANE_CELLS_AREA,
                          &p_docu->caret,
                          skel_rect_cur.tl.pixit_point.x - p_docu->caret.pixit_point.x,
                          skel_rect_cur.br.pixit_point.x - p_docu->caret.pixit_point.x,
                          top,
                          bottom);
    }

    p_docu->flags.caret_position_later = p_docu->flags.caret_scroll_later = 0;

#if TRACE_ALLOWED
    if_constant(tracing(TRACE_APP_CLICK))
    {
        static S32 count = 0;
        trace_1(TRACE_APP_CLICK, TEXT("caret_position_set_show, count: ") S32_TFMT, count++);
    }
#endif
}

/******************************************************************************
*
* move the caret to a position in the document
*
******************************************************************************/

static void
caret_to_slr(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_SLR p_slr,
    _InRef_opt_ PC_SKEL_POINT p_skel_point,
    _InVal_     BOOL caret_x_save /* preserve caret x offset*/,
    _InVal_     BOOL keep_selection,
    _InVal_     BOOL key_selection,
    _InVal_     BOOL extend_selection)
{
    OBJECT_POSITION_FIND object_position_find;
    SKEL_EDGE skel_edge_caret_x = p_docu->caret_x;

    if(!keep_selection)
        status_assert(maeve_event(p_docu, T5_MSG_SELECTION_CLEAR, P_DATA_NONE));

    if(!slr_equal(&p_docu->cur.slr, p_slr))
        cur_change_before(p_docu);

    position_set_from_skel_point(p_docu, &object_position_find, p_skel_point, p_slr, OK_CELLS_EDIT);

    /* set physical position */
    p_docu->cur.slr = *p_slr;
    p_docu->cur.object_position = object_position_find.object_data.object_position_start;

    caret_show_claim(p_docu, OBJECT_ID_CELLS, TRUE);

    if(caret_x_save)
        p_docu->caret_x = skel_edge_caret_x;

    if(key_selection)
        move_ship(p_docu, &p_docu->cur, extend_selection);
}

static void
cells_reflect_focus_change(
    _DocuRef_   P_DOCU p_docu)
{
    T5_TOOLBAR_TOOL_ENABLE t5_toolbar_tool_enable;

    t5_toolbar_tool_enable.enabled = (p_docu->focus_owner == OBJECT_ID_CELLS);
    t5_toolbar_tool_enable.enable_id = TOOL_ENABLE_CELLS_FOCUS;

    tool_enable(p_docu, &t5_toolbar_tool_enable, USTR_TEXT("PASTE"));

    tool_enable(p_docu, &t5_toolbar_tool_enable, USTR_TEXT("CHECK"));

    tool_enable(p_docu, &t5_toolbar_tool_enable, USTR_TEXT("SEARCH"));

    tool_enable(p_docu, &t5_toolbar_tool_enable, USTR_TEXT("TABLE"));
    tool_enable(p_docu, &t5_toolbar_tool_enable, USTR_TEXT("BOX"));

    tool_enable(p_docu, &t5_toolbar_tool_enable, USTR_TEXT("INSERT_DATE"));

    tool_enable(p_docu, &t5_toolbar_tool_enable, USTR_TEXT("STYLE"));
    tool_enable(p_docu, &t5_toolbar_tool_enable, USTR_TEXT("EFFECTS"));

    tool_enable(p_docu, &t5_toolbar_tool_enable, USTR_TEXT("BOLD"));
    tool_enable(p_docu, &t5_toolbar_tool_enable, USTR_TEXT("ITALIC"));
    tool_enable(p_docu, &t5_toolbar_tool_enable, USTR_TEXT("UNDERLINE"));
    tool_enable(p_docu, &t5_toolbar_tool_enable, USTR_TEXT("SUPERSCRIPT"));
    tool_enable(p_docu, &t5_toolbar_tool_enable, USTR_TEXT("SUBSCRIPT"));

    tool_enable(p_docu, &t5_toolbar_tool_enable, USTR_TEXT("JUSTIFY_LEFT"));
    tool_enable(p_docu, &t5_toolbar_tool_enable, USTR_TEXT("JUSTIFY_CENTRE"));
    tool_enable(p_docu, &t5_toolbar_tool_enable, USTR_TEXT("JUSTIFY_RIGHT"));
    tool_enable(p_docu, &t5_toolbar_tool_enable, USTR_TEXT("JUSTIFY_FULL"));

    tool_enable(p_docu, &t5_toolbar_tool_enable, USTR_TEXT("TAB_LEFT"));
    tool_enable(p_docu, &t5_toolbar_tool_enable, USTR_TEXT("TAB_CENTRE"));
    tool_enable(p_docu, &t5_toolbar_tool_enable, USTR_TEXT("TAB_RIGHT"));
    tool_enable(p_docu, &t5_toolbar_tool_enable, USTR_TEXT("TAB_DECIMAL"));

#if RISCOS
    /* nobble button if we have no thesaurus app loaded as yet */
    if(!thesaurus_loaded())
        t5_toolbar_tool_enable.enabled = FALSE;

    tool_enable(p_docu, &t5_toolbar_tool_enable, USTR_TEXT("THESAURUS"));
#endif
}

static void
cells_reflect_position(
    _DocuRef_   P_DOCU p_docu)
{
    switch(p_docu->focus_owner)
    {
    case OBJECT_ID_CELLS:
        if(last_page_x(p_docu) < 2)
        {
            ROW_ENTRY row_entry;

            row_entry_from_row(p_docu, &row_entry, p_docu->cur.slr.row);

            status_line_setf(p_docu, STATUS_LINE_LEVEL_INFORMATION_FOCUS(OBJECT_ID_CELLS),
                             MSG_STATUS_PAGE_ONE_NUM,
                             row_entry.rowtab.edge_top.page + 1);
        }
        else
        {
            SKEL_POINT skel_point;

            skel_point_from_slr_tl(p_docu, &skel_point, &p_docu->cur.slr);

            status_line_setf(p_docu, STATUS_LINE_LEVEL_INFORMATION_FOCUS(OBJECT_ID_CELLS),
                             MSG_STATUS_PAGE_XY_NUM,
                             skel_point.page_num.x + 1,
                             skel_point.page_num.y + 1);
        }

        break;

    default:
        status_line_clear(p_docu, STATUS_LINE_LEVEL_INFORMATION_FOCUS(OBJECT_ID_CELLS));
        break;
    }
}

_Check_return_
static STATUS
cells_reflect_selection_new(
    _DocuRef_   P_DOCU p_docu)
{
    MARK_INFO mark_info;

    if(status_fail(object_skel(p_docu, T5_MSG_MARK_INFO_READ, &mark_info)))
        mark_info.h_markers = 0;

    { /* because this is state, there must only be one controller */
    T5_TOOLBAR_TOOL_SET t5_toolbar_tool_set;
    t5_toolbar_tool_set.state.state = (mark_info.h_markers != 0);
    t5_toolbar_tool_set.name = USTR_TEXT("SELECTION");
    status_consume(object_call_id(OBJECT_ID_TOOLBAR, p_docu, T5_MSG_TOOLBAR_TOOL_SET, &t5_toolbar_tool_set));
    } /*block*/

    {
    T5_TOOLBAR_TOOL_ENABLE t5_toolbar_tool_enable;
    t5_toolbar_tool_enable.enabled = (p_docu->focus_owner == OBJECT_ID_CELLS) && (mark_info.h_markers != 0);
    t5_toolbar_tool_enable.enable_id = TOOL_ENABLE_CELLS_FOCUS_AND_SELECTION;
    tool_enable(p_docu, &t5_toolbar_tool_enable, USTR_TEXT("COPY"));
    tool_enable(p_docu, &t5_toolbar_tool_enable, USTR_TEXT("CUT"));
    tool_enable(p_docu, &t5_toolbar_tool_enable, USTR_TEXT("SORT"));
    } /*block*/

    return(STATUS_OK);
}

/******************************************************************************
*
* do auto width for an area
*
******************************************************************************/

#define AUTO_WIDTH_SPECIAL_COUNT 50

#define AUTO_WIDTH_TIME MONOTIMEDIFF_VALUE_FROM_SECONDS(3)

_Check_return_
static PIXIT
col_auto_width(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     COL col,
    _InVal_     ROW row_s,
    _InVal_     ROW row_e,
    _InVal_     BOOL allow_special)
{
    S32 n_rows = row_e - row_s;
    S32 n_rows_done = 0;
    S32 n_rows_to_do_min;
    ROW rows_done[AUTO_WIDTH_SPECIAL_COUNT];
    S32 rows_done_idx = 0;
    BOOL auto_width_special = 0;
    OBJECT_HOW_WIDE object_how_wide;
    STYLE style;
    SLR slr;
    MONOTIME time_started = monotime();

    if(allow_special && (n_rows > AUTO_WIDTH_SPECIAL_COUNT))
    {
        auto_width_special = 1;
        n_rows_to_do_min = AUTO_WIDTH_SPECIAL_COUNT;
    }
    else
        n_rows_to_do_min = n_rows;

    style_init(&style);
    style_bit_set(&style, STYLE_SW_CS_WIDTH);
    style.col_style.width = 0;

    slr.col = col;

    for(;;)
    {
        BOOL done_already = 0;

        if(n_rows_done >= n_rows_to_do_min) /* SKS 30mar95 new loop condition */
        {
            BOOL breakout = TRUE;

            if(auto_width_special)
            {
                if(n_rows_done >= n_rows)
                    breakout = TRUE;
                else
                    breakout = (monotime_diff(time_started) >= AUTO_WIDTH_TIME);
            }

            if(breakout)
                break;
        }

        if(auto_width_special)
        {
            S32 x;

            slr.row = row_s + (rand() % n_rows);

            for(x = 0; x < rows_done_idx; x += 1)
                if(slr.row == rows_done[x])
                {
                    done_already = 1;
                    break;
                }
        }
        else
            slr.row = row_s + n_rows_done;

        if(!done_already)
        {
            (void) cell_data_from_slr(p_docu, &object_how_wide.object_data, &slr);

            trace_2(TRACE_APP_SKEL, TEXT("auto_width col,row: ") COL_TFMT TEXT(",") ROW_TFMT, slr.col, slr.row);

            object_how_wide.width = 0;
            status_consume(cell_call_id(object_how_wide.object_data.object_id, p_docu, T5_MSG_OBJECT_HOW_WIDE, &object_how_wide, NO_CELLS_EDIT));
            style.col_style.width = MAX(style.col_style.width, object_how_wide.width);

            trace_2(TRACE_APP_SKEL, TEXT("object_how_wide col: ") COL_TFMT TEXT(", width: ") PIXIT_TFMT, col, object_how_wide.width);

            if(auto_width_special)
            {
                rows_done[rows_done_idx++] = slr.row;

                if(AUTO_WIDTH_SPECIAL_COUNT == rows_done_idx)
                    rows_done_idx = 0; /* wrap */
            }

            n_rows_done += 1;
        }
    }

    style.col_style.width = skel_ruler_snap_to_click_stop(p_docu, TRUE, style.col_style.width, SNAP_TO_CLICK_STOP_CEIL);
    if(style.col_style.width > p_docu->page_def.cells_usable_x)
        style.col_style.width = skel_ruler_snap_to_click_stop(p_docu, TRUE, p_docu->page_def.cells_usable_x, SNAP_TO_CLICK_STOP_FLOOR /* SKS 29jan95 */);
    style.col_style.width = MIN(p_docu->page_def.cells_usable_x, style.col_style.width);

    trace_2(TRACE_APP_SKEL, TEXT("col_auto_width col: ") COL_TFMT TEXT(", width: ") PIXIT_TFMT, col, style.col_style.width);

    return(style.col_style.width);
}

/******************************************************************************
*
* move the caret position down
*
******************************************************************************/

_Check_return_
static STATUS
cursor_down(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    _InVal_     S32 action)
{
    BOOL position_set = 0, extend_selection, key_selection, use_both_xy = 0;
    SKEL_POINT skel_point;

    drop_anchor(p_docu, t5_message, &key_selection, &extend_selection);

    if(status_ok(logical_move(p_docu, &skel_point, OBJECT_LOGICAL_MOVE_DOWN, &use_both_xy)))
        position_set = 1;
    CODE_ANALYSIS_ONLY(else use_both_xy = 0);

    if(!position_set)
    {
        if(p_docu->cur.slr.row + 1 < n_rows(p_docu))
        {
            ROW_ENTRY row_entry;

            skel_point_from_slr_tl(p_docu, &skel_point, &p_docu->cur.slr);
            row_entry_from_row(p_docu, &row_entry, p_docu->cur.slr.row + 1);

            skel_point_update_from_edge(&skel_point, &row_entry.rowtab.edge_top, y);
            position_set = 1;
        }
    }

    if(position_set)
    {
        switch(action)
        {
        case CARET_TO_LEFT:
        case CARET_TO_RIGHT:
            break;
        case CARET_STAY:
            if(!use_both_xy)
                skel_point_update_from_edge(&skel_point, &p_docu->caret_x, x);
            else
                edge_set_from_skel_point(&p_docu->caret_x, &skel_point, x);
            break;
        }

        {
        SLR slr;
        SKEL_POINT skel_point_tl;
        if(status_ok(slr_owner_from_skel_point(p_docu, &slr, &skel_point_tl, &skel_point, ON_ROW_EDGE_GO_DOWN)))
        {
            if(action == CARET_TO_RIGHT)
                skel_point.pixit_point.x += cell_width(p_docu, &slr);

            caret_to_slr(p_docu, &slr, &skel_point, action == CARET_STAY /* save caret_x */, key_selection, key_selection, extend_selection);
        }
        } /*block*/
    }

    return(STATUS_OK);
}

/******************************************************************************
*
* move cursor left; message tells how much
*
******************************************************************************/

_Check_return_
static STATUS
cursor_left(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message)
{
    STATUS status = STATUS_OK;
    BOOL extend_selection, key_selection;

    drop_anchor(p_docu, t5_message, &key_selection, &extend_selection);

    if(OBJECT_ID_NONE != p_docu->cur.object_position.object_id)
    {
        OBJECT_POSITION_SET object_position_set;

        consume_bool(cell_data_from_position(p_docu, &object_position_set.object_data, &p_docu->cur));

        switch(t5_message)
        {
        default: default_unhandled();
#if CHECKING
        case T5_CMD_CURSOR_LEFT:
        case T5_CMD_SHIFT_CURSOR_LEFT:
#endif
            object_position_set.action = OBJECT_POSITION_SET_BACK;
            break;

        case T5_CMD_WORD_LEFT:
        case T5_CMD_SHIFT_WORD_LEFT:
            object_position_set.action = OBJECT_POSITION_SET_PREV_WORD;
            break;
        }

        if(status_done(status = cell_call_id(object_position_set.object_data.object_id,
                                             p_docu,
                                             T5_MSG_OBJECT_POSITION_SET,
                                             &object_position_set,
                                             OK_CELLS_EDIT)))
        {
            p_docu->cur.object_position = object_position_set.object_data.object_position_start;

            status_assert(spell_auto_check(p_docu));

            if(key_selection)
                move_ship(p_docu, &p_docu->cur, extend_selection);
            else
            {
                status_assert(maeve_event(p_docu, T5_MSG_SELECTION_CLEAR, P_DATA_NONE));
                caret_position_after_command(p_docu);
            }
        }
    }

    if(!status_done(status))
    {
        /* look backwards for previous cell */
        SLR slr = p_docu->cur.slr;
        BOOL stop = FALSE;

        do  {
            slr.col -= 1;

            if(slr.col < 0)
            {
                slr.col = n_cols_logical(p_docu) - 1;
                slr.row -= 1;

                if(slr.row < 0)
                    stop = TRUE;
            }
        }
        while(!stop && (0 == cell_width(p_docu, &slr)));

        if(!stop)
        {
            SKEL_RECT skel_rect;
            skel_rect_from_slr(p_docu, &skel_rect, &slr);

            /* we achieved our objective only if we moved immediately above the current cell */
            if(slr.col == p_docu->cur.slr.col && slr.row == p_docu->cur.slr.row - 1)
                status = STATUS_DONE;

            caret_to_slr(p_docu, &slr, &skel_rect.br, FALSE /* save caret_x */, key_selection, key_selection, extend_selection);
        }
    }

    return(status);
}

/******************************************************************************
*
* move right; message gives degree
*
******************************************************************************/

_Check_return_
static STATUS
cursor_right(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message)
{
    STATUS status = STATUS_OK;
    BOOL extend_selection, key_selection;

    drop_anchor(p_docu, t5_message, &key_selection, &extend_selection);

    if(OBJECT_ID_NONE != p_docu->cur.object_position.object_id)
    {
        OBJECT_POSITION_SET object_position_set;

        consume_bool(cell_data_from_position(p_docu, &object_position_set.object_data, &p_docu->cur));

        switch(t5_message)
        {
        default: default_unhandled();
#if CHECKING
        case T5_CMD_CURSOR_RIGHT:
        case T5_CMD_SHIFT_CURSOR_RIGHT:
#endif
            object_position_set.action = OBJECT_POSITION_SET_FORWARD;
            break;

        case T5_CMD_WORD_RIGHT:
        case T5_CMD_SHIFT_WORD_RIGHT:
            object_position_set.action = OBJECT_POSITION_SET_NEXT_WORD;
            break;
        }

        if(status_done(status = cell_call_id(object_position_set.object_data.object_id,
                                             p_docu,
                                             T5_MSG_OBJECT_POSITION_SET,
                                             &object_position_set,
                                             OK_CELLS_EDIT)))
        {
            p_docu->cur.object_position = object_position_set.object_data.object_position_start;

            status_assert(spell_auto_check(p_docu));

            if(key_selection)
                move_ship(p_docu, &p_docu->cur, extend_selection);
            else
                status_assert(maeve_event(p_docu, T5_MSG_SELECTION_CLEAR, P_DATA_NONE));
                caret_position_after_command(p_docu);
        }
    }

    if(!status_done(status))
    {
        /* look forwards for next cell */
        SLR slr = p_docu->cur.slr;
        BOOL hit_end = FALSE;

        slr.col += 1;

        while(slr.col < n_cols_logical(p_docu) && (0 == cell_width(p_docu, &slr)))
            slr.col += 1;

        if(slr.col >= n_cols_logical(p_docu))
        {
            slr.row += 1;
            slr.col = 0;
            if(slr.row >= n_rows(p_docu))
                hit_end = TRUE;
        }

        if(!hit_end)
        {
            SKEL_POINT skel_point;
            skel_point_from_slr_tl(p_docu, &skel_point, &slr);

            caret_to_slr(p_docu, &slr, &skel_point, FALSE /* save caret_x */, key_selection, key_selection, extend_selection);
            status = STATUS_DONE;
        }

    }

    return(status);
}

/******************************************************************************
*
* move caret up
*
******************************************************************************/

_Check_return_
static STATUS
cursor_up(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    _InVal_     S32 action)
{
    BOOL position_set = 0, extend_selection, key_selection, use_both_xy = 0;
    SKEL_POINT skel_point = p_docu->caret;

    drop_anchor(p_docu, t5_message, &key_selection, &extend_selection);

    if(status_ok(logical_move(p_docu, &skel_point, OBJECT_LOGICAL_MOVE_UP, &use_both_xy)))
        position_set = 1;
    CODE_ANALYSIS_ONLY(else use_both_xy = 0);

    if(!position_set)
    {
        if(0 == p_docu->cur.slr.row)
        {
            skel_point.pixit_point.y = 0;
            skel_point.page_num.y = 0;
        }
        else
            skel_point_from_slr_tl(p_docu, &skel_point, &p_docu->cur.slr);
    }

    switch(action)
    {
    case CARET_TO_LEFT:
    case CARET_TO_RIGHT:
        break;
    case CARET_STAY:
        if(!use_both_xy)
            skel_point_update_from_edge(&skel_point, &p_docu->caret_x, x);
        else
            edge_set_from_skel_point(&p_docu->caret_x, &skel_point, x);
        break;
    }

    {
    SKEL_POINT skel_point_tl;
    SLR slr;
    if(status_ok(slr_owner_from_skel_point(p_docu, &slr, &skel_point_tl, &skel_point, position_set ? ON_ROW_EDGE_GO_DOWN : ON_ROW_EDGE_GO_UP)))
    {
        if(action == CARET_TO_RIGHT)
            skel_point.pixit_point.x += cell_width(p_docu, &slr);

        caret_to_slr(p_docu, &slr, &skel_point, action == CARET_STAY /* save caret_x */, key_selection, key_selection, extend_selection);
    }
    } /*block*/

    return(STATUS_OK);
}

/******************************************************************************
*
* perform a logical move
*
******************************************************************************/

_Check_return_ _Success_(status_ok(return))
static STATUS
logical_move(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_SKEL_POINT p_skel_point,
    _InVal_     enum OBJECT_LOGICAL_MOVE_ACTION action,
    _OutRef_opt_ P_BOOL p_use_both_xy)
{
    STATUS status = STATUS_FAIL;
    OBJECT_LOGICAL_MOVE object_logical_move;
    zero_struct(object_logical_move);

    consume_bool(cell_data_from_position(p_docu, &object_logical_move.object_data, &p_docu->cur));
    object_logical_move.action = action;
    if(status_ok(status = cell_call_id(object_logical_move.object_data.object_id, p_docu, T5_MSG_OBJECT_LOGICAL_MOVE, &object_logical_move, NO_CELLS_EDIT)))
    {
        *p_skel_point = object_logical_move.skel_point_out;
        if(NULL != p_use_both_xy)
            *p_use_both_xy = object_logical_move.use_both_xy;
    }

    return(status);
}

/******************************************************************************
*
* move whole cells at a time
*
******************************************************************************/

_Check_return_
static STATUS
cells_msg_move_cell_left(
    _DocuRef_   P_DOCU p_docu)
{
    SLR slr;

    cur_change_before(p_docu);

    slr = p_docu->cur.slr;

    do  {
        slr.col -= 1;
    }
    while((slr.col >= 0) && (0 == cell_width(p_docu, &slr)));

    if(slr.col >= 0)
        caret_to_slr(p_docu, &slr, NULL, FALSE, FALSE, FALSE, FALSE);

    cur_change_after(p_docu);

    return(STATUS_OK);
}

_Check_return_
static STATUS
cells_msg_move_cell_right(
    _DocuRef_   P_DOCU p_docu)
{
    SLR slr;

    cur_change_before(p_docu);

    slr = p_docu->cur.slr;

    do  {
        slr.col += 1;
    }
    while((slr.col < n_cols_logical(p_docu)) && (0 == cell_width(p_docu, &slr)));

    if(slr.col < n_cols_logical(p_docu))
        caret_to_slr(p_docu, &slr, NULL, FALSE, FALSE, FALSE, FALSE);

    cur_change_after(p_docu);

    return(STATUS_OK);
}

_Check_return_
static STATUS
cells_msg_move_cell_up(
    _DocuRef_   P_DOCU p_docu)
{
    SLR slr;

    cur_change_before(p_docu);

    slr = p_docu->cur.slr;

    do  {
        slr.row -= 1;
    }
    while((slr.row >= 0) && (0 == row_has_height(p_docu, slr.row)));

    if(slr.row >= 0)
        caret_to_slr(p_docu, &slr, NULL, FALSE, FALSE, FALSE, FALSE);

    cur_change_after(p_docu);

    return(STATUS_OK);
}

_Check_return_
static STATUS
cells_msg_move_cell_down(
    _DocuRef_   P_DOCU p_docu)
{
    SLR slr;

    cur_change_before(p_docu);

    slr = p_docu->cur.slr;

    do  {
        slr.row += 1;
    }
    while((slr.row < n_rows(p_docu)) && (0 == row_has_height(p_docu, slr.row)));

    if(slr.row < n_rows(p_docu))
        caret_to_slr(p_docu, &slr, NULL, FALSE, FALSE, FALSE, FALSE);

    cur_change_after(p_docu);

    return(STATUS_OK);
}

T5_CMD_PROTO(static, cells_cmd_page_up_down)
{
    SKEL_POINT skel_point = p_docu->caret;
    SKEL_POINT tl;
    SLR slr;
    BOOL key_selection, extend_selection;

    UNREFERENCED_PARAMETER_InVal_(p_t5_cmd);

    drop_anchor(p_docu, t5_message, &key_selection, &extend_selection);

    view_scroll_pane(p_docu, t5_message, &skel_point);

    if(status_ok(slr_owner_from_skel_point(p_docu, &slr, &tl, &skel_point, ON_ROW_EDGE_GO_UP)))
        caret_to_slr(p_docu, &slr, &skel_point, FALSE, key_selection, key_selection, extend_selection);

    return(STATUS_OK);
}

T5_CMD_PROTO(static, cells_cmd_line_start)
{
    SKEL_POINT skel_point;
    BOOL extend_selection, key_selection;

    UNREFERENCED_PARAMETER_InVal_(p_t5_cmd);

    drop_anchor(p_docu, t5_message, &key_selection, &extend_selection);

    if(status_ok(logical_move(p_docu, &skel_point, OBJECT_LOGICAL_MOVE_LEFT, NULL)))
    {
        SKEL_POINT tl;
        SLR slr;
        skel_point.pixit_point.y = p_docu->caret.pixit_point.y;
        skel_point.page_num.y = p_docu->caret.page_num.y;
        if(status_ok(slr_owner_from_skel_point(p_docu, &slr, &tl, &skel_point, ON_ROW_EDGE_GO_DOWN)))
            caret_to_slr(p_docu, &slr, &skel_point, FALSE, key_selection, key_selection, extend_selection);
    }

    return(STATUS_OK);
}

T5_CMD_PROTO(static, cells_cmd_line_end)
{
    SKEL_POINT skel_point;
    BOOL extend_selection, key_selection;

    UNREFERENCED_PARAMETER_InVal_(p_t5_cmd);

    drop_anchor(p_docu, t5_message, &key_selection, &extend_selection);

    if(status_ok(logical_move(p_docu, &skel_point, OBJECT_LOGICAL_MOVE_RIGHT, NULL)))
    {
        SKEL_POINT tl;
        SLR slr;
        skel_point.pixit_point.y = p_docu->caret.pixit_point.y;
        skel_point.page_num.y = p_docu->caret.page_num.y;
        if(status_ok(slr_owner_from_skel_point(p_docu, &slr, &tl, &skel_point, ON_ROW_EDGE_GO_DOWN)))
            caret_to_slr(p_docu, &slr, &skel_point, FALSE, key_selection, key_selection, extend_selection);
    }

    return(STATUS_OK);
}

T5_CMD_PROTO(static, cells_cmd_document_top)
{
    SLR slr = p_docu->cur.slr;
    BOOL extend_selection, key_selection;

    UNREFERENCED_PARAMETER_InVal_(p_t5_cmd);

    drop_anchor(p_docu, t5_message, &key_selection, &extend_selection);

    slr.row = 0;
    caret_to_slr(p_docu, &slr, NULL, TRUE, key_selection, key_selection, extend_selection);

    return(STATUS_OK);
}

T5_CMD_PROTO(static, cells_cmd_document_bottom)
{
    SLR slr = p_docu->cur.slr;
    SKEL_RECT skel_rect;
    BOOL extend_selection, key_selection;

    UNREFERENCED_PARAMETER_InVal_(p_t5_cmd);

    drop_anchor(p_docu, t5_message, &key_selection, &extend_selection);

    slr.row = n_rows(p_docu) - 1;
    skel_rect_from_slr(p_docu, &skel_rect, &slr);
    caret_to_slr(p_docu, &slr, &skel_rect.br, TRUE, key_selection, key_selection, extend_selection);

    return(STATUS_OK);
}

T5_CMD_PROTO(static, cells_cmd_first_column)
{
    SLR slr = p_docu->cur.slr;

    UNREFERENCED_PARAMETER_InVal_(t5_message);
    UNREFERENCED_PARAMETER_InVal_(p_t5_cmd);

    slr.col = 0;
    caret_to_slr(p_docu, &slr, NULL, FALSE, FALSE, FALSE, FALSE);

    return(STATUS_OK);
}

T5_CMD_PROTO(static, cells_cmd_last_column)
{
    SLR slr = p_docu->cur.slr;

    UNREFERENCED_PARAMETER_InVal_(t5_message);
    UNREFERENCED_PARAMETER_InVal_(p_t5_cmd);

    slr.col = n_cols_logical(p_docu) - 1;
    caret_to_slr(p_docu, &slr, NULL, FALSE, FALSE, FALSE, FALSE);

    return(STATUS_OK);
}

/******************************************************************************
*
* delete/insert rows and columns
*
******************************************************************************/

_Check_return_
static STATUS
row_col_ins_del(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    _InRef_maybenone_ PC_S32 p_n_given /* P_DATA_NONE -> use selection */)
{
    STATUS status = STATUS_OK;
    const BOOL use_n_given = !IS_PTR_NONE(p_n_given);
    COL col_s, col_e;
    ROW row_s, row_e;
    DOCU_AREA docu_area;
    BOOL had_selection = 0;
    BOOL was_base = 0;

    if(p_docu->mark_info_cells.h_markers)
    {
        docu_area_from_markers_first(p_docu, &docu_area);
        status_assert(maeve_event(p_docu, T5_MSG_SELECTION_CLEAR, P_DATA_NONE));
        had_selection = 1;
    }
    else
    {
        P_STYLE_DOCU_AREA p_style_docu_area;
        STYLE_SELECTOR selector;

        style_selector_clear(&selector);
        style_selector_bit_set(&selector, STYLE_SW_CS_WIDTH);

        p_style_docu_area = style_effect_source_find(p_docu, &selector, &p_docu->cur, &p_docu->h_style_docu_area, TRUE);
        PTR_ASSERT(p_style_docu_area);
        docu_area = p_style_docu_area->docu_area;

        was_base = p_style_docu_area->base; /* SKS 07sep95 */
    }

    limits_from_docu_area(p_docu, &col_s, &col_e, &row_s, &row_e, &docu_area);

    switch(t5_message)
    {
    default: default_unhandled();
#if CHECKING
    case T5_CMD_ADD_ROWS:
    case T5_CMD_INSERT_ROW:
    case T5_CMD_DELETE_ROW:
#endif
        {
        ROW n_rows;

        if(!had_selection)
        {
            col_s = 0;
            col_e = all_cols(p_docu);
            row_s = p_docu->cur.slr.row;
        }
        else if(t5_message == T5_CMD_ADD_ROWS)
            row_s = row_e - 1;

        n_rows = use_n_given ? (ROW) *p_n_given : (had_selection ? (row_e - row_s) : 1);

        row_e = row_s + n_rows;
        break;
        }

    case T5_CMD_ADD_COLS:
    case T5_CMD_INSERT_COL:
    case T5_CMD_DELETE_COL:
        {
        COL n_cols;

        if(!had_selection)
            col_s = p_docu->cur.slr.col;
        else if(t5_message == T5_CMD_ADD_COLS)
            col_s = col_e - 1;

        /* SKS 07sep95 stop them deleting the first text column in letters etc. */
        if(was_base && (T5_CMD_DELETE_COL == t5_message) && (0 == col_s))
            return(create_error(ERR_CANT_DELETE_MAIN_COL));

        n_cols = use_n_given ? (COL) *p_n_given : (had_selection ? (col_e - col_s) : 1);

        col_e = col_s + n_cols;
        break;
        }
    }

    { /* +++ */
    STYLE style;
    style_init(&style);

    /* set up column style to apply for new columns */
    switch(t5_message)
    {
    default:
        break;

    case T5_CMD_INSERT_COL:
        style.para_style.margin_para = style_default_measurement(p_docu, STYLE_SW_PS_MARGIN_PARA);
        style_bit_set(&style, STYLE_SW_PS_MARGIN_PARA);
        style.para_style.margin_left = style_default_measurement(p_docu, STYLE_SW_PS_MARGIN_LEFT);
        style_bit_set(&style, STYLE_SW_PS_MARGIN_LEFT);
        style.para_style.margin_right = style_default_measurement(p_docu, STYLE_SW_PS_MARGIN_RIGHT);
        style_bit_set(&style, STYLE_SW_PS_MARGIN_RIGHT);
        style.para_style.h_tab_list = 0;
        style_bit_set(&style, STYLE_SW_PS_TAB_LIST);
        style.col_style.width = style_default_measurement(p_docu, STYLE_SW_CS_WIDTH);
        style_bit_set(&style, STYLE_SW_CS_WIDTH);
        break;

    /* when adding columns, duplicate the style of the existing column */
    case T5_CMD_ADD_COLS:
        {
        DOCU_AREA docu_area_read;

        docu_area_init(&docu_area_read);
        docu_area_read.tl.slr.col = col_s;
        docu_area_read.tl.slr.row = row_s;
        docu_area_read.br.slr.col = col_s + 1;
        docu_area_read.br.slr.row = row_e;

        {
        STYLE_SELECTOR selector_fuzzy_out;
        STYLE_SELECTOR selector_in;
        style_selector_clear(&selector_in);
        style_selector_bit_set(&selector_in, STYLE_SW_PS_MARGIN_PARA);
        style_selector_bit_set(&selector_in, STYLE_SW_PS_MARGIN_LEFT);
        style_selector_bit_set(&selector_in, STYLE_SW_PS_MARGIN_RIGHT);
        style_selector_bit_set(&selector_in, STYLE_SW_CS_WIDTH);

        style_of_area(p_docu, &style, &selector_fuzzy_out, &selector_in, &docu_area_read);
        } /*block*/

        style.para_style.h_tab_list = 0; /* but force an override for this one - SKS 07jul96 made it work */
        style_bit_set(&style, STYLE_SW_PS_TAB_LIST);

        break;
        }
    }

    switch(t5_message)
    {
    default: default_unhandled();
        break;
    case T5_CMD_ADD_ROWS:
        status = cells_block_insert(p_docu, col_s, col_e, row_s, row_e - row_s, TRUE);
        break;
    case T5_CMD_ADD_COLS:
        status = cells_column_insert(p_docu, col_s, col_e - col_s, row_s, row_e, TRUE);
        break;
    case T5_CMD_INSERT_ROW:
        status = cells_block_insert(p_docu, col_s, col_e, row_s, row_e - row_s, FALSE);
        break;
    case T5_CMD_INSERT_COL:
        status = cells_column_insert(p_docu, col_s, col_e - col_s, row_s, row_e, FALSE);
        break;
    case T5_CMD_DELETE_ROW:
        cells_block_delete(p_docu, col_s, col_e, row_s, row_e - row_s);
        break;
    case T5_CMD_DELETE_COL:
        status = cells_column_delete(p_docu, col_s, col_e - col_s, row_s, row_e);
        break;
    }

    /* when adding/inserting columns, loop over those created and
     * ensure that they have a column width and a grid set
     */
    if(status_ok(status))
    {
        switch(t5_message)
        {
        default:
            break;

        case T5_CMD_ADD_COLS:
            col_s += 1;
            col_e += 1;

            /*FALLTHRU*/

        case T5_CMD_INSERT_COL:
            {
            COL col;
            DOCU_AREA docu_area_set;

            docu_area_init(&docu_area_set);
            docu_area_set.tl.slr.row = row_s;
            docu_area_set.br.slr.row = row_e;

            for(col = col_s; col < col_e && status_ok(status); ++col)
            {
                STYLE_DOCU_AREA_ADD_PARM style_docu_area_add_parm;

                docu_area.tl.slr.col = col;
                docu_area.br.slr.col = col + 1;

                STYLE_DOCU_AREA_ADD_STYLE(&style_docu_area_add_parm, &style);
                status = style_docu_area_add(p_docu, &docu_area, &style_docu_area_add_parm);
            }

            break;
            }
        }
    }

    } /* +++ */ /*block*/

    docu_modify(p_docu);

    view_update_all(p_docu, UPDATE_PANE_MARGIN_COL);
    view_update_all(p_docu, UPDATE_BORDER_HORZ);
    view_update_all(p_docu, UPDATE_RULER_HORZ);

    return(status);
}

/******************************************************************************
*
* check if a row has a non-zero height
*
******************************************************************************/

_Check_return_
static S32
row_has_height(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ROW row)
{
    ROW_ENTRY row_entry, row_entry_next;
    row_entries_from_row(p_docu, &row_entry, &row_entry_next, row);
    return(skel_edge_compare(&row_entry.rowtab.edge_top, &row_entry_next.rowtab.edge_top));
}

/******************************************************************************
*
* delete selected text as a result of a key press operation;
* this routine objects if the selection covers more than one cell
*
******************************************************************************/

_Check_return_
static STATUS /* >0 if no op; no error */
selection_delete_auto(
    _DocuRef_   P_DOCU p_docu)
{
    STATUS status = STATUS_OK;

    if(p_docu->mark_info_cells.h_markers)
    {
        DOCU_AREA docu_area;
        const BOOL clip_data_from_cut_operation = TRUE;
        BOOL f_local_clip_data;
        ARRAY_HANDLE h_clip_data; /* may become owned by Windows clipboard */

        docu_area_from_markers_first(p_docu, &docu_area);

        status_return(clip_data_array_handle_prepare(&h_clip_data, &f_local_clip_data));

        status = cells_save_clip_data(p_docu, &h_clip_data, &docu_area);

        if(status_ok(status))
        {
            if( docu_area.whole_col ||
                docu_area.whole_row ||
                docu_area.br.slr.col > docu_area.tl.slr.col + 1 ||
                docu_area.br.slr.row > docu_area.tl.slr.row + 1
              )
            {
                UI_TEXT ui_text;
                ui_text.type = UI_TEXT_TYPE_RESID;
                ui_text.text.resource_id = MSG_STATUS_SELECTION_TOO_BIG;
                status_line_set(p_docu, STATUS_LINE_LEVEL_AUTO_CLEAR, &ui_text);
                host_bleep();
                al_array_dispose(&h_clip_data);
                status = STATUS_DONE;
            }
            else
            {
                status_assert(maeve_event(p_docu, T5_MSG_SELECTION_CLEAR, P_DATA_NONE));

                status = cells_docu_area_delete(p_docu, &docu_area, docu_area_spans_across_table(p_docu, &docu_area), TRUE);
                caret_position_after_command(p_docu);

                clip_data_set(p_docu, f_local_clip_data, h_clip_data, clip_data_from_cut_operation, OBJECT_ID_CELLS);
                g_clip_data_docu_area = docu_area;
            }
        }
        else
        {
            al_array_dispose(&h_clip_data);
        }
    }

    return(status);
}

/******************************************************************************
*
* call object to check spelling after some movement
*
******************************************************************************/

_Check_return_
static STATUS
spell_auto_check(
    _DocuRef_   P_DOCU p_docu)
{
    if(!global_preferences.spell_auto_check)
        return(STATUS_OK);

    {
    SPELL_AUTO_CHECK spell_auto_check;
    spell_auto_check.position_now = p_docu->cur;
    spell_auto_check.position_was = p_docu->old;
    (void) cell_data_from_slr(p_docu, &spell_auto_check.object_data, &p_docu->cur.slr);
    return(cell_call_id(spell_auto_check.object_data.object_id, p_docu, T5_MSG_SPELL_AUTO_CHECK, &spell_auto_check, NO_CELLS_EDIT));
    } /*block*/
}

/* ------------------------------------------------------------------------- */

T5_CMD_PROTO(static, cells_cmd_add_cr)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 1);
    const S32 n_rc_add = p_args[0].val.s32;
    return(row_col_ins_del(p_docu, t5_message, &n_rc_add));
}

/* apply a region containing just row height unfixed */

_Check_return_
static STATUS
cells_cmd_auto_height__virtual_row_table(
    _DocuRef_   P_DOCU p_docu)
{
    STATUS status = STATUS_OK;
    SLR slr;
    PIXIT row_height = 0;

    slr.row = p_docu->cur.slr.row;
    for(slr.col = 0; slr.col < n_cols_logical(p_docu); ++slr.col)
    {
        PIXIT this_row_height;
        OBJECT_HOW_BIG object_how_big;
        (void) cell_data_from_slr(p_docu, &object_how_big.object_data, &slr);
        skel_rect_from_slr(p_docu, &object_how_big.skel_rect, &slr);
        format_object_how_big(p_docu, &object_how_big);
        this_row_height = skel_rect_height(&object_how_big.skel_rect);
        row_height = MAX(row_height, this_row_height);
    }

    if(row_height)
    {
        POSITION position = p_docu->cur;
        STYLE style;
        style_init(&style);
        style_bit_set(&style, STYLE_SW_RS_HEIGHT);
        style.row_style.height = row_height;
        position.slr.row = 0;
        style_effect_source_modify(p_docu, &p_docu->h_style_docu_area, &position, NULL, &style);
    }

    return(status);
}

_Check_return_
static STATUS
cells_cmd_auto_height__normal_row_table(
    _DocuRef_   P_DOCU p_docu)
{
    STATUS status = STATUS_OK;
    P_STYLE_DOCU_AREA p_style_docu_area;
    STYLE_SELECTOR selector;

    style_selector_clear(&selector);
    style_selector_bit_set(&selector, STYLE_SW_RS_HEIGHT);
    style_selector_bit_set(&selector, STYLE_SW_RS_HEIGHT_FIXED);

    if(NULL != (p_style_docu_area = style_effect_source_find(p_docu, &selector, &p_docu->cur, &p_docu->h_style_docu_area, FALSE)))
    {
        const P_STYLE p_style = p_style_from_docu_area(p_docu, p_style_docu_area);

        /* try very hard to remove an existing area rather than add a new one */
        if(!p_docu->mark_info_cells.h_markers
           &&
           !p_style_docu_area->docu_area.whole_col
           &&
           p_style_docu_area->docu_area.tl.slr.row == p_docu->cur.slr.row
           &&
           p_style_docu_area->docu_area.br.slr.row == p_docu->cur.slr.row + 1
           &&
           !style_selector_compare(&p_style->selector, &selector))
        {
            style_docu_area_remove(p_docu,
                                   &p_docu->h_style_docu_area,
                                   array_indexof_element(&p_docu->h_style_docu_area, STYLE_DOCU_AREA, p_style_docu_area));
        }
        else
        {
            DOCU_AREA docu_area;
            STYLE style;
            STYLE_DOCU_AREA_ADD_PARM style_docu_area_add_parm;
            STYLE_DOCU_AREA_ADD_STYLE(&style_docu_area_add_parm, &style);

            style_init(&style);
            style_bit_set(&style, STYLE_SW_RS_HEIGHT_FIXED);
            style.row_style.height_fixed = 0;

            if(p_docu->mark_info_cells.h_markers)
                docu_area_from_markers_first(p_docu, &docu_area);
            else
                docu_area_from_position_max(&docu_area, &p_docu->cur);

            status = style_docu_area_add(p_docu, &docu_area, &style_docu_area_add_parm);
        }
    }

    return(status);
}

_Check_return_
static STATUS
cells_cmd_auto_height(
    _DocuRef_   P_DOCU p_docu)
{
    if(p_docu->flags.virtual_row_table)
        return(cells_cmd_auto_height__virtual_row_table(p_docu));
    else
        return(cells_cmd_auto_height__normal_row_table(p_docu));
}

_Check_return_
static STATUS
cells_cmd_auto_width(
    _DocuRef_   P_DOCU p_docu)
{
    STATUS status = STATUS_OK;
    DOCU_AREA docu_area;
    STYLE style;
    BOOL got_area = FALSE;

    style_init(&style);
    style_bit_set(&style, STYLE_SW_CS_WIDTH);

    if(p_docu->mark_info_cells.h_markers)
    {
        docu_area_from_markers_first(p_docu, &docu_area);
        got_area = TRUE;
    }
    else
    {
        P_STYLE_DOCU_AREA p_style_docu_area = style_effect_source_find(p_docu, &style.selector, &p_docu->cur, &p_docu->h_style_docu_area, TRUE);

        if(NULL != p_style_docu_area)
        {
            docu_area = p_style_docu_area->docu_area;
            docu_area.tl.slr.col = p_docu->cur.slr.col;
            docu_area.br.slr.col = p_docu->cur.slr.col + 1; /* limit it to the current column! */
            got_area = TRUE;
        }
    }

    if(got_area)
    {
        COL col_s, col_e, col;
        ROW row_s, row_e;

        limits_from_docu_area(p_docu, &col_s, &col_e, &row_s, &row_e, &docu_area);

        for(col = col_s; col < col_e; col += 1)
        {
            COL_AUTO_WIDTH col_auto_width;
            col_auto_width.col = col;
            col_auto_width.row_s = row_s;
            col_auto_width.row_e = row_e;
            col_auto_width.allow_special = p_docu->flags.virtual_row_table;
            status_consume(object_call_id(p_docu->object_id_flow, p_docu, T5_MSG_COL_AUTO_WIDTH, &col_auto_width));
            style.col_style.width = col_auto_width.width;

#if 0 /* check switched off 6.7.94 by request */
            if(style.col_style.width > p_docu->page_def.cells_usable_x / 2)
                status_line_set(p_docu, STATUS_LINE_LEVEL_AUTO_CLEAR, MSG_STATUS_AUTO_WIDTH_NOT_DONE);
            else
#endif

            if(style.col_style.width)
            {
                DOCU_AREA docu_area_apply;
                STYLE_DOCU_AREA_ADD_PARM style_docu_area_add_parm;
                STYLE_DOCU_AREA_ADD_STYLE(&style_docu_area_add_parm, &style);

                docu_area_apply = docu_area;
                docu_area_apply.tl.slr.col = col;
                docu_area_apply.br.slr.col = col + 1;
                docu_area_apply.whole_row = 0;

                status_break(status = style_docu_area_add(p_docu, &docu_area_apply, &style_docu_area_add_parm));
            }
        }
    }

    return(status);
}

T5_CMD_PROTO(static, cells_cmd_block_clear)
{
    DOCU_AREA docu_area;
    REGION region;
    STATUS status;

    UNREFERENCED_PARAMETER_InVal_(t5_message);
    UNREFERENCED_PARAMETER_InRef_(p_t5_cmd);

    if(p_docu->mark_info_cells.h_markers)
        docu_area_from_markers_first(p_docu, &docu_area);
    else
        docu_area_from_position_max(&docu_area, &p_docu->cur);

    region_from_docu_area_max(&region, &docu_area);
    status_assert(maeve_event(p_docu, T5_MSG_CUR_CHANGE_BEFORE, &region));

    {
    COL col_s, col_e;
    ROW row_s, row_e;

    limits_from_docu_area(p_docu, &col_s, &col_e, &row_s, &row_e, &docu_area);
    status = cells_block_blank_make(p_docu, col_s, col_e, row_s, row_e - row_s);
    } /*block*/

    cur_change_after(p_docu);

    return(status);
}

T5_CMD_PROTO(static, cells_cmd_sort)
{
    STATUS status;

    cur_change_before(p_docu);

    status = object_call_id_load(p_docu, t5_message, de_const_cast(P_T5_CMD, p_t5_cmd), OBJECT_ID_SKEL_SPLIT);

    cur_change_after(p_docu);

    return(status);
}

T5_CMD_PROTO(static, cells_cmd_box)
{
    DOCU_AREA docu_area;
    STATUS status;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(p_docu->mark_info_cells.h_markers)
        docu_area_from_markers_first(p_docu, &docu_area);
    else
        docu_area_from_position_max(&docu_area, &p_docu->cur);

    {
    BOX_APPLY box_apply;
    box_apply.p_array_handle = NULL;
    box_apply.data_space = DATA_SLOT;
    docu_area_normalise(p_docu, &box_apply.docu_area, &docu_area);
    box_apply.arglist_handle = p_t5_cmd->arglist_handle;
    status = object_skel(p_docu, T5_MSG_BOX_APPLY, &box_apply);
    } /*block*/

    return(status);
}

_Check_return_
static STATUS
cells_cmd_selection_clear(
    _DocuRef_   P_DOCU p_docu)
{
    status_assert(maeve_event(p_docu, T5_MSG_SELECTION_CLEAR, P_DATA_NONE));

    caret_show_claim(p_docu, OBJECT_ID_CELLS, FALSE);

    return(STATUS_OK);
}

_Check_return_
static STATUS
cells_cmd_selection_copy_or_cut(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message)
{
    const BOOL cut_operation = (T5_CMD_SELECTION_CUT == t5_message);
    STATUS status = STATUS_OK;

    if(p_docu->mark_info_cells.h_markers)
    {
        DOCU_AREA docu_area;
        const BOOL clip_data_from_cut_operation = cut_operation;
        BOOL f_local_clip_data;
        ARRAY_HANDLE h_clip_data; /* may become owned by Windows clipboard */

        docu_area_from_markers_first(p_docu, &docu_area);

        status_return(clip_data_array_handle_prepare(&h_clip_data, &f_local_clip_data));

        status = cells_save_clip_data(p_docu, &h_clip_data, &docu_area);

        if(status_ok(status))
        {
            clip_data_set(p_docu, f_local_clip_data, h_clip_data, clip_data_from_cut_operation, OBJECT_ID_CELLS); /* handle donated */
            g_clip_data_docu_area = docu_area;
        }
        else
        {
            al_array_dispose(&h_clip_data);
            return(status);
        }
    }

    if(cut_operation)
        status = delete_selection(p_docu);

    return(status);
}

_Check_return_
static STATUS
cells_cmd_selection_delete(
    _DocuRef_   P_DOCU p_docu)
{
    return(delete_selection(p_docu));
}

T5_CMD_PROTO(static, cells_cmd_insert_field)
{
    STATUS status;
    OBJECT_DATA object_data;

    (void) cell_data_from_slr(p_docu, &object_data, &p_docu->cur.slr);

    status = cell_call_id(object_data.object_id, p_docu, t5_message, de_const_cast(P_T5_CMD, p_t5_cmd), OK_CELLS_EDIT);

    docu_modify(p_docu);

    return(status);
}

T5_CMD_PROTO(static, cells_cmd_object_convert)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 1);
    const OBJECT_ID target_object = p_args[0].val.object_id;
    DOCU_AREA docu_area;
    REGION region;
    STATUS status = STATUS_OK;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(p_docu->mark_info_cells.h_markers)
        docu_area_from_markers_first(p_docu, &docu_area);
    else
        docu_area_from_position_max(&docu_area, &p_docu->cur);

    region_from_docu_area_max(&region, &docu_area);

    status_assert(maeve_event(p_docu, T5_MSG_CUR_CHANGE_BEFORE, &region));
#if 0 /* we can't do this cos it frees all the results we're trying to read */
    { /* tell dependents about it */
    UREF_PARMS uref_parms;
    region_from_docu_area_max(&uref_parms.source.region, &docu_area);
    uref_event(p_docu, T5_MSG_UREF_OVERWRITE, &uref_parms);
    } /*block*/
#endif

    {
    SCAN_BLOCK scan_block;
    OBJECT_READ_TEXT object_read_text;
    PROCESS_STATUS process_status;
    process_status_init(&process_status);
    process_status.flags.foreground = TRUE;
    process_status.reason.type = UI_TEXT_TYPE_RESID;
    process_status.reason.text.resource_id = MSG_STATUS_CONVERTING;
    process_status_begin(p_docu, &process_status, PROCESS_STATUS_PERCENT);

    if(status_done(cells_scan_init(p_docu, &scan_block, SCAN_DOWN, SCAN_AREA, &docu_area, OBJECT_ID_NONE)))
    {
        while(status_done(cells_scan_next(p_docu, &object_read_text.object_data, &scan_block)))
        {
            QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 500);
            quick_ublock_with_buffer_setup(quick_ublock);

            process_status.data.percent.current = cells_scan_percent(&scan_block);
            process_status_reflect(&process_status);

            object_read_text.p_quick_ublock = &quick_ublock;
            object_read_text.type = OBJECT_READ_TEXT_PLAIN;
            if(target_object != object_read_text.object_data.object_id)
            {
                status = object_call_id(object_read_text.object_data.object_id, p_docu, T5_MSG_OBJECT_READ_TEXT, &object_read_text);

                if(status_ok(status) && (0 != quick_ublock_bytes(object_read_text.p_quick_ublock)))
                {
                    status = quick_ublock_nullch_add(object_read_text.p_quick_ublock);

                    if(status_ok(status))
                    {
                        NEW_OBJECT_FROM_TEXT new_object_from_text;
                        new_object_from_text.data_ref = object_read_text.object_data.data_ref;
                        new_object_from_text.p_quick_ublock = &quick_ublock;
                        new_object_from_text.status = STATUS_OK;
                        new_object_from_text.please_redraw = FALSE;
                        new_object_from_text.please_uref_overwrite = TRUE;
                        new_object_from_text.please_mrofmun = TRUE;
                        status = object_call_id(target_object, p_docu, T5_MSG_NEW_OBJECT_FROM_TEXT, &new_object_from_text);
                    }
                }
            }

            quick_ublock_dispose(&quick_ublock);

            status_break(status);
        }
    }

    process_status_end(&process_status);
    } /*block*/

    status_consume(object_skel(p_docu, T5_MSG_REDRAW_REGION, &region));

    cur_change_after(p_docu);

    return(status);
}

_Check_return_
static STATUS
cells_cmd_paste_at_cursor(
    _DocuRef_   P_DOCU p_docu)
{
    DOCU_AREA docu_area;
    BOOL f_local_clip_data = TRUE;
    ARRAY_HANDLE h_clip_data = 0;
    STATUS status;

    docu_area_init(&docu_area);

#if 0
    cur_change_before(p_docu);
#endif

    if(p_docu->mark_info_cells.h_markers)
    {
        docu_area_from_markers_first(p_docu, &docu_area);

        status_return(clip_data_array_handle_prepare(&h_clip_data, &f_local_clip_data));

        status = cells_save_clip_data(p_docu, &h_clip_data, &docu_area);

        if(status_fail(status))
        {
            al_array_dispose(&h_clip_data);
            return(status);
        }
    }

    status = delete_selection(p_docu);

    if(status_ok(status))
    {
#if WINDOWS && defined(USE_GLOBAL_CLIPBOARD)
        status = load_from_windows_clipboard(p_docu, g_clip_data_from_cut_operation);
#else
        status = load_ownform_from_array_handle(p_docu, &g_h_local_clip_data, P_POSITION_NONE, g_clip_data_from_cut_operation);
#endif
    }

#if 0
    if(0 != h_clip_data)
    {   /* replace clipboard data with the saved data from above */
        clip_data_set(p_docu, f_local_clip_data, h_clip_data, TRUE /*clip_data_from_cut_operation*/);

        g_clip_data_docu_area = docu_area;
    }
#endif

    cur_change_after(p_docu);

    return(status);
}

_Check_return_
static STATUS
cells_cmd_style_region_clear(
    _DocuRef_   P_DOCU p_docu)
{
    if(p_docu->mark_info_cells.h_markers)
    {
        DOCU_AREA docu_area;
        docu_area_from_markers_first(p_docu, &docu_area);
        style_docu_area_clear(p_docu, &p_docu->h_style_docu_area, &docu_area, DATA_SLOT);
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
cells_cmd_style_region_count(
    _DocuRef_   P_DOCU p_docu)
{
    if(p_docu->mark_info_cells.h_markers)
    {
        DOCU_AREA docu_area;
        S32 count;
        docu_area_from_markers_first(p_docu, &docu_area);
        count = style_docu_area_count(&p_docu->h_style_docu_area, &docu_area);
        status_line_setf(p_docu, STATUS_LINE_LEVEL_AUTO_CLEAR, (1 == count) ? MSG_STATUS_REGION_COUNTED : MSG_STATUS_REGIONS_COUNTED, count);
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
cells_cmd_return(
    _DocuRef_   P_DOCU p_docu)
{
    const T5_MESSAGE t5_message = T5_CMD_RETURN;
    STATUS status;

    if(OBJECT_ID_REC_FLOW == p_docu->object_id_flow)
    {
        status_assert(cursor_down(p_docu, t5_message, CARET_TO_LEFT));
        return(STATUS_OK);
    }

    if(status_ok(status = spell_auto_check(p_docu)))
    {
        OBJECT_POSITION_SET object_position_set;
        BOOL object_splittable = 1, on_last_row;

        on_last_row = p_docu->cur.slr.row + 1 >= n_rows(p_docu);

        /* see if object will accept sub-object position */
        consume_bool(cell_data_from_position(p_docu, &object_position_set.object_data, &p_docu->cur));
        object_position_set.action = OBJECT_POSITION_SET_START;
        if(!status_done(cell_call_id(object_position_set.object_data.object_id,
                                     p_docu,
                                     T5_MSG_OBJECT_POSITION_SET,
                                     &object_position_set,
                                     NO_CELLS_EDIT)))
            object_splittable = 0;

        /* if object is not splittable, just do a cursor down */
        if(!object_splittable && !on_last_row)
            status_assert(cursor_down(p_docu, t5_message, CARET_TO_LEFT));
        else
        {
            if(status_ok(check_protection_simple(p_docu, TRUE)))
            {
                if(!object_splittable || cell_in_table(p_docu, &p_docu->cur.slr))
                {
                    if(on_last_row)
                    {
                        docu_modify(p_docu);
                        if(status_ok(cells_block_insert(p_docu, 0, all_cols(p_docu), p_docu->cur.slr.row + 1, 1, FALSE)))
                            status_assert(cursor_down(p_docu, t5_message, CARET_TO_LEFT));
                    }
                    else
                    {
                        status_assert(cursor_down(p_docu, t5_message, CARET_TO_LEFT));
                    }
                }
                else
                {
                    /* split object and insert row across document */
                    docu_modify(p_docu);
                    if(status_ok(cells_object_split(p_docu, &p_docu->cur)))
                        status_assert(cursor_down(p_docu, t5_message, CARET_TO_LEFT));
                }
            }
        }
    }

    return(status);
}

T5_MSG_PROTO(static, cells_msg_mark_info_read, _OutRef_ P_MARK_INFO p_mark_info)
{
    UNREFERENCED_PARAMETER_InRef_(t5_message);

    *p_mark_info = p_docu->mark_info_cells;

    return(STATUS_OK);
}

T5_CMD_PROTO(static, cells_cmd_toggle_marks)
{
    UNREFERENCED_PARAMETER_InRef_(t5_message);

    /* fire off appropriate command to focus owner, avoiding macro recording */
    return(docu_focus_owner_object_call(p_docu, (p_docu->mark_info_cells.h_markers ? T5_CMD_SELECTION_CLEAR : T5_CMD_SELECT_DOCUMENT), de_const_cast(P_T5_CMD, p_t5_cmd)));
}

T5_CMD_PROTO(static, cells_cmd_select_cell)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);
    UNREFERENCED_PARAMETER_InRef_(p_t5_cmd);

    docu_area_init(&p_docu->anchor_mark.docu_area);
    p_docu->anchor_mark.docu_area.tl = p_docu->cur;
    p_docu->anchor_mark.docu_area.tl.object_position.object_id = OBJECT_ID_NONE;
    p_docu->anchor_mark.docu_area.br = p_docu->anchor_mark.docu_area.tl;

    status_assert(anchor_to_markers_new(p_docu));
    anchor_to_markers_finish(p_docu);
    markers_show(p_docu);

    return(STATUS_OK);
}

T5_CMD_PROTO(static, cells_cmd_select_document)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);
    UNREFERENCED_PARAMETER_InRef_(p_t5_cmd);

    p_docu->anchor_mark.docu_area.whole_col =
    p_docu->anchor_mark.docu_area.whole_row = 1;
    p_docu->anchor_mark.docu_area.tl.slr.col = 0;
    p_docu->anchor_mark.docu_area.br.slr.col = -1;
    p_docu->anchor_mark.docu_area.tl.slr.row = 0;
    p_docu->anchor_mark.docu_area.br.slr.row = -1;
    p_docu->anchor_mark.docu_area.tl.object_position.object_id = OBJECT_ID_NONE;
    p_docu->anchor_mark.docu_area.br.object_position.object_id = OBJECT_ID_NONE;

    caret_show_claim(p_docu, OBJECT_ID_CELLS, FALSE);

    status_assert(anchor_to_markers_new(p_docu));
    anchor_to_markers_finish(p_docu);
    markers_show(p_docu);

    return(STATUS_OK);
}

T5_CMD_PROTO(static, cells_cmd_select_word)
{
    OBJECT_POSITION_SET object_position_set;
    BOOL extend_selection = (p_docu->mark_info_cells.h_markers != 0);

    UNREFERENCED_PARAMETER_InVal_(t5_message);
    UNREFERENCED_PARAMETER_InRef_(p_t5_cmd);

    if(extend_selection)
    {
        docu_area_from_markers_first(p_docu, &p_docu->anchor_mark.docu_area);
        p_docu->anchor_mark.docu_area.br.slr.col -= 1;
        p_docu->anchor_mark.docu_area.br.slr.row -= 1;
    }
    else
    {
        docu_area_init(&p_docu->anchor_mark.docu_area);
        p_docu->anchor_mark.docu_area.tl = p_docu->cur;
        p_docu->anchor_mark.docu_area.br = p_docu->cur;
    }

    if(cell_data_from_position(p_docu,
                                 &object_position_set.object_data,
                                 extend_selection ? &p_docu->anchor_mark.docu_area.br
                                                  : &p_docu->anchor_mark.docu_area.tl))
    {
        object_position_set.action = extend_selection ? OBJECT_POSITION_SET_NEXT_WORD : OBJECT_POSITION_SET_START_WORD;
        status_consume(cell_call_id(object_position_set.object_data.object_id, p_docu, T5_MSG_OBJECT_POSITION_SET, &object_position_set, NO_CELLS_EDIT));
        if(!extend_selection)
            p_docu->anchor_mark.docu_area.tl.object_position = object_position_set.object_data.object_position_start;

        object_position_set.action = OBJECT_POSITION_SET_END_WORD;
        status_consume(cell_call_id(object_position_set.object_data.object_id, p_docu, T5_MSG_OBJECT_POSITION_SET, &object_position_set, NO_CELLS_EDIT));
        p_docu->anchor_mark.docu_area.br.object_position = object_position_set.object_data.object_position_start;
        p_docu->cur.object_position = object_position_set.object_data.object_position_start;
    }
    else
    {
        p_docu->anchor_mark.docu_area.tl.object_position.object_id = OBJECT_ID_NONE;
        p_docu->anchor_mark.docu_area.br = p_docu->anchor_mark.docu_area.tl;
    }

    if(!extend_selection)
        status_assert(anchor_to_markers_new(p_docu));

    anchor_to_markers_finish(p_docu);
    markers_show(p_docu);

    return(STATUS_OK);
}

T5_CMD_PROTO(static, cells_cmd_setc)
{
    STATUS status = STATUS_OK;
    STATUS status_cell = STATUS_OK;

    UNREFERENCED_PARAMETER_InRef_(p_t5_cmd);

    if(p_docu->mark_info_cells.h_markers)
    {
        SCAN_BLOCK scan_block;
        OBJECT_SET_CASE object_set_case;
        DOCU_AREA docu_area;
        REGION region;

        docu_area_from_markers_first(p_docu, &docu_area);
        region_from_docu_area_max(&region, &docu_area);

        object_set_case.do_redraw = FALSE;

        if(status_done(cells_scan_init(p_docu, &scan_block, SCAN_DOWN, SCAN_MARKERS, NULL, OBJECT_ID_NONE)))
        {
            while(status_done(cells_scan_next(p_docu, &object_set_case.object_data, &scan_block)))
            {
                status_cell = cell_call_id(object_set_case.object_data.object_id, p_docu, t5_message, &object_set_case, OK_CELLS_EDIT);
                if(status_fail(status_cell) && (STATUS_FAIL != status_cell))
                    break;
            }
        }

        status_consume(object_skel(p_docu, T5_MSG_REDRAW_REGION, &region));
    }
    else
    {
        /* no selection - act on a single char and advance */
        OBJECT_POSITION_SET object_position_set;
        OBJECT_SET_CASE object_set_case;

        consume_bool(cell_data_from_position(p_docu, &object_position_set.object_data, &p_docu->cur));
        object_set_case.object_data = object_position_set.object_data;
        object_set_case.do_redraw = TRUE;

        object_position_set.action = OBJECT_POSITION_SET_FORWARD;
        if(status_done(cell_call_id(object_position_set.object_data.object_id,
                                    p_docu,
                                    T5_MSG_OBJECT_POSITION_SET,
                                    &object_position_set,
                                    OK_CELLS_EDIT)))
        {
            object_set_case.object_data.object_position_end = object_position_set.object_data.object_position_start;
            status_cell = cell_call_id(object_set_case.object_data.object_id, p_docu, t5_message, &object_set_case, OK_CELLS_EDIT);
            p_docu->cur.object_position = object_set_case.object_data.object_position_end;
        }
    }

    if(status_fail(status_cell) && (STATUS_FAIL != status_cell))
        status = status_cell;

    cur_change_after(p_docu);

    return(status);
}

T5_CMD_PROTO(static, cells_cmd_search)
{
    STATUS status;

    cur_change_before(p_docu);

    status = t5_cmd_search_do(p_docu, t5_message, p_t5_cmd);

    cur_change_after(p_docu);

    return(status);
}

T5_CMD_PROTO(static, cells_cmd_snapshot)
{
    DOCU_AREA docu_area;
    REGION region;
    STATUS status = STATUS_OK;

    UNREFERENCED_PARAMETER_InVal_(t5_message);
    UNREFERENCED_PARAMETER_InRef_(p_t5_cmd);

    if(p_docu->mark_info_cells.h_markers)
        docu_area_from_markers_first(p_docu, &docu_area);
    else
        docu_area_from_position_max(&docu_area, &p_docu->cur);

    region_from_docu_area_max(&region, &docu_area);
    status_assert(maeve_event(p_docu, T5_MSG_CUR_CHANGE_BEFORE, &region));

#if 0 /* we can't do this cos it frees all the results we're trying to read */
    { /* tell dependents about it */
    UREF_PARMS uref_parms;
    region_from_docu_area_max(&uref_parms.source.region, &docu_area);
    uref_event(p_docu, T5_MSG_UREF_OVERWRITE, &uref_parms);
    } /*block*/
#endif

    {
    SCAN_BLOCK scan_block;
    OBJECT_SNAPSHOT object_snapshot;
    PROCESS_STATUS process_status;
    process_status_init(&process_status);
    process_status.flags.foreground = TRUE;
    process_status.reason.type = UI_TEXT_TYPE_RESID;
    process_status.reason.text.resource_id = MSG_STATUS_CONVERTING;
    process_status_begin(p_docu, &process_status, PROCESS_STATUS_PERCENT);

    object_snapshot.do_uref_overwrite = TRUE;

    if(status_done(cells_scan_init(p_docu, &scan_block, SCAN_DOWN, SCAN_AREA, &docu_area, OBJECT_ID_NONE)))
    {
        while(status_done(cells_scan_next(p_docu, &object_snapshot.object_data, &scan_block)))
        {
            process_status.data.percent.current = cells_scan_percent(&scan_block);
            process_status_reflect(&process_status);

            status_break(status = cell_call_id(object_snapshot.object_data.object_id, p_docu, T5_MSG_OBJECT_SNAPSHOT, &object_snapshot, NO_CELLS_EDIT));
        }
    }

    process_status_end(&process_status);
    } /*block*/

    status_consume(object_skel(p_docu, T5_MSG_REDRAW_REGION, &region));

    cur_change_after(p_docu);

    return(status);
}

_Check_return_
static STATUS
cells_cmd_straddle_horz(
    _DocuRef_   P_DOCU p_docu)
{
    STATUS status = STATUS_OK;

    if(p_docu->mark_info_cells.h_markers)
    {
        DOCU_AREA docu_area_marked;
        COL col_s, col_e;
        ROW row_s, row_e;
        ARRAY_HANDLE h_col_info = 0;
        SKEL_POINT skel_point;
        SLR slr;

        docu_area_from_markers_first(p_docu, &docu_area_marked);
        limits_from_docu_area(p_docu, &col_s, &col_e, &row_s, &row_e, &docu_area_marked);

        slr.col = col_s;
        slr.row = row_s;
        skel_point_from_slr_tl(p_docu, &skel_point, &slr);

        status_return(skel_col_enum(p_docu, row_s, skel_point.page_num.x, (COL) -1, &h_col_info));

        if(0 != array_elements32(&h_col_info))
        {
            const PC_COL_INFO p_col_info_base = array_basec(&h_col_info, COL_INFO);
            const COL col_base = p_col_info_base->col;

            if((col_e - 1) < (col_base + array_elements(&h_col_info))
               &&
               (col_e - col_s) > 1)
            {
                const PC_COL_INFO p_col_info_s = p_col_info_base + (col_s - col_base);
                const PC_COL_INFO p_col_info_e = p_col_info_base + (col_e - col_base) - 1;
                STYLE style;
                DOCU_AREA docu_area_apply;
                STYLE_DOCU_AREA_ADD_PARM style_docu_area_add_parm;
                STYLE_DOCU_AREA_ADD_STYLE(&style_docu_area_add_parm, &style);

                style_init(&style);
                style_bit_set(&style, STYLE_SW_CS_WIDTH);
                style.col_style.width = 0;

                docu_area_apply = docu_area_marked;
                docu_area_apply.whole_row = 0;

                docu_area_apply.tl.slr.col = col_s + 1;
                docu_area_apply.br.slr.col = col_e;

                if(status_ok(status = style_docu_area_add(p_docu, &docu_area_apply, &style_docu_area_add_parm)))
                {
                    style.col_style.width = p_col_info_e->edge_right.pixit - p_col_info_s->edge_left.pixit;
                    docu_area_apply.tl.slr.col = col_s;
                    docu_area_apply.br.slr.col = col_s + 1;
                    STYLE_DOCU_AREA_ADD_STYLE(&style_docu_area_add_parm, &style);
                    status = style_docu_area_add(p_docu, &docu_area_apply, &style_docu_area_add_parm);
                }
            }
        }
    }

    return(status);
}

T5_CMD_PROTO(static, cells_cmd_style_apply)
{
    /* apply a new region */
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 1);
    PC_USTR_INLINE ustr_inline = p_args[0].val.ustr_inline; /* string of inlines */
    DOCU_AREA docu_area;
    STYLE_DOCU_AREA_ADD_PARM style_docu_area_add_parm;
    STYLE_DOCU_AREA_ADD_INLINE(&style_docu_area_add_parm, ustr_inline);

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(p_docu->mark_info_cells.h_markers)
        docu_area_from_markers_first(p_docu, &docu_area);
    else
    {
        docu_area_from_position(&docu_area, &p_docu->cur);
        style_docu_area_add_parm.caret = 1;
    }

    return(style_docu_area_add(p_docu, &docu_area, &style_docu_area_add_parm));
}

T5_CMD_PROTO(static, cells_cmd_style_apply_source)
{
    /* if there is a selection - redirect to normal STYLE_APPLY */
    if(p_docu->mark_info_cells.h_markers)
        return(cells_cmd_style_apply(p_docu, t5_message, p_t5_cmd));

    { /* modify source of style if no selection */
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 1);
    PC_USTR_INLINE ustr_inline = p_args[0].val.ustr_inline; /* string of inlines */
    style_effect_source_modify(p_docu, &p_docu->h_style_docu_area, &p_docu->cur, ustr_inline, NULL);
    return(STATUS_OK);
    } /*block*/
}

T5_CMD_PROTO(static, cells_cmd_tab_lr)
{
    STATUS status;
    TAB_WANTED tab_wanted;
    zero_struct(tab_wanted);

    UNREFERENCED_PARAMETER_InRef_(p_t5_cmd);

    tab_wanted.t5_message = t5_message;
    consume_bool(cell_data_from_position(p_docu, &tab_wanted.object_data, &p_docu->cur));
    status_return(status = cell_call_id(tab_wanted.object_data.object_id, p_docu, T5_MSG_TAB_WANTED, &tab_wanted, NO_CELLS_EDIT));

    if(tab_wanted.want_inline_insert)
    {
        if(status_ok(check_protection_simple(p_docu, TRUE)))
            return(object_cells(p_docu, T5_CMD_INSERT_FIELD_TAB, P_DATA_NONE));
    }
    else if(!tab_wanted.processed)
    {
        switch(t5_message)
        {
        default: default_unhandled();
#if CHECKING
        case T5_CMD_TAB_LEFT:
#endif
            return(object_skel(p_docu, T5_MSG_MOVE_LEFT_CELL, P_DATA_NONE));

        case T5_CMD_TAB_RIGHT:
            return(object_skel(p_docu, T5_MSG_MOVE_RIGHT_CELL, P_DATA_NONE));
        }
    }

    return(status);
}

#if RISCOS

_Check_return_
static STATUS
cells_cmd_thesaurus(
    _DocuRef_   P_DOCU p_docu)
{
    STATUS status = STATUS_OK;

    if(!p_docu->mark_info_cells.h_markers)
        status_return(status = object_call_id(OBJECT_ID_CELLS, p_docu, T5_CMD_SELECT_WORD, P_DATA_NONE));

    if(p_docu->mark_info_cells.h_markers)
    {
        P_DOCU_AREA p_docu_area = p_docu_area_from_markers_first(p_docu);

        if(docu_area_is_frag(p_docu_area))
        {
            OBJECT_READ_TEXT object_read_text;

            if(cell_data_from_docu_area_tl(p_docu, &object_read_text.object_data, p_docu_area))
            {
                QUICK_UBLOCK quick_ublock = QUICK_UBLOCK_INIT_NULL(); /* force to array */

                object_read_text.p_quick_ublock = &quick_ublock;
                object_read_text.type = OBJECT_READ_TEXT_PLAIN;

                if(status_ok(status = cell_call_id(object_read_text.object_data.object_id, p_docu, T5_MSG_OBJECT_READ_TEXT, &object_read_text, NO_CELLS_EDIT)))
                    if(status_ok(status = quick_ublock_nullch_add(&quick_ublock)))
                        status = thesaurus_process_word(quick_ublock_ustr(&quick_ublock));

                quick_ublock_dispose(&quick_ublock);
                return(status);
            }
        }
    }

    host_bleep();

    return(status);
}

#endif /* RISCOS */

T5_MSG_PROTO(static, cells_event_click_left_single, _InoutRef_ P_SKELEVENT_CLICK p_skelevent_click)
{
    SKEL_POINT tl;
    SLR slr;
    OBJECT_DATA object_data;
    STATUS status;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    status_return(slr_owner_from_skel_point(p_docu, &slr, &tl, &p_skelevent_click->skel_point, ON_ROW_EDGE_GO_UP));

    (void) cell_data_from_slr(p_docu, &object_data, &slr);

    status_return(status = cell_call_id(object_data.object_id, p_docu, t5_message, p_skelevent_click, NO_CELLS_EDIT));

    if(!p_skelevent_click->processed)
    {
        p_skelevent_click->processed = 1;

        caret_to_slr(p_docu, &slr, &p_skelevent_click->skel_point, FALSE, FALSE, FALSE, FALSE);
    }

    return(status);
}

T5_MSG_PROTO(static, cells_event_click_left_double, _InoutRef_ P_SKELEVENT_CLICK p_skelevent_click)
{
    SKEL_POINT tl;
    SLR slr;
    OBJECT_DATA object_data;
    STATUS status;

    status_return(slr_owner_from_skel_point(p_docu, &slr, &tl, &p_skelevent_click->skel_point, ON_ROW_EDGE_GO_UP));

    (void) cell_data_from_slr(p_docu, &object_data, &slr);

    status_return(status = cell_call_id(object_data.object_id, p_docu, t5_message, p_skelevent_click, NO_CELLS_EDIT));

    if(!p_skelevent_click->processed)
    {
        p_skelevent_click->processed = 1;

        { /* ask object if it wants the double click */
        OBJECT_DOUBLE_CLICK object_double_click;
        OBJECT_ID object_id = OBJECT_ID_ENUM_START;
        object_double_click.processed = 0;
        data_ref_from_slr(&object_double_click.data_ref, &slr);

        while(status_ok(object_next(&object_id)))
        {
            status_break(status = cell_call_id(object_id, p_docu, T5_MSG_CLICK_LEFT_DOUBLE, &object_double_click, NO_CELLS_EDIT));

            if(object_double_click.processed)
                break;
        }

        if(!object_double_click.processed)
            status = execute_command(p_docu, T5_CMD_SELECT_WORD, _P_DATA_NONE(P_ARGLIST_HANDLE), OBJECT_ID_SKEL);
        } /*block*/
    }

    return(status);
}

T5_MSG_PROTO(static, cells_event_click_left_triple, _InoutRef_ P_SKELEVENT_CLICK p_skelevent_click)
{
    SKEL_POINT tl;
    SLR slr;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    status_return(slr_owner_from_skel_point(p_docu, &slr, &tl, &p_skelevent_click->skel_point, ON_ROW_EDGE_GO_UP));

    p_skelevent_click->processed = 1;

    return(execute_command(p_docu, T5_CMD_SELECT_CELL, _P_DATA_NONE(P_ARGLIST_HANDLE), OBJECT_ID_SKEL));
}

T5_MSG_PROTO(static, cells_event_click_left_drag, _InoutRef_ P_SKELEVENT_CLICK p_skelevent_click)
{
    SLR slr;
    OBJECT_DATA object_data;
    OBJECT_POSITION_FIND object_position_find;
    STATUS status;

    status_return(slr_owner_from_skel_point(p_docu, &slr, &object_position_find.skel_rect.tl, &p_skelevent_click->skel_point, ON_ROW_EDGE_GO_UP));

    (void) cell_data_from_slr(p_docu, &object_data, &slr);

    status_return(status = cell_call_id(object_data.object_id, p_docu, t5_message, p_skelevent_click, NO_CELLS_EDIT));

    if(!p_skelevent_click->processed)
    {
        p_skelevent_click->processed = 1;

        position_set_from_skel_point(p_docu, &object_position_find, &p_skelevent_click->skel_point, &slr, NO_CELLS_EDIT);

        p_docu->anchor_mark.docu_area.whole_col =
        p_docu->anchor_mark.docu_area.whole_row = 0;
        p_docu->anchor_mark.docu_area.tl.slr = slr;
        p_docu->anchor_mark.docu_area.tl.object_position = object_position_find.object_data.object_position_start;
        p_docu->anchor_mark.docu_area.br = p_docu->anchor_mark.docu_area.tl;

        status_assert(anchor_to_markers_new(p_docu));

        host_drag_start(NULL);
    }

    return(status);
}

T5_MSG_PROTO(static, cells_event_click_right, _InoutRef_ P_SKELEVENT_CLICK p_skelevent_click)
{
    SLR slr;
    OBJECT_POSITION_FIND object_position_find;
    STATUS status = STATUS_OK;

    if(p_docu->focus_owner == OBJECT_ID_CELLS)
    {
        /* find position of click */
        if(status_ok(slr_owner_from_skel_point(p_docu, &slr, &object_position_find.skel_rect.tl, &p_skelevent_click->skel_point, ON_ROW_EDGE_GO_UP)))
        {
            p_skelevent_click->processed = 1;

            /* set up anchor start position -
             * use current position if no selection
             */
            if(!p_docu->mark_info_cells.h_markers)
            {
                p_docu->anchor_mark.docu_area.whole_col =
                p_docu->anchor_mark.docu_area.whole_row = 0;
                p_docu->anchor_mark.docu_area.tl = p_docu->cur;
            }

            position_set_from_skel_point(p_docu,
                                         &object_position_find,
                                         &p_skelevent_click->skel_point,
                                         &slr,
                                         NO_CELLS_EDIT);

            p_docu->anchor_mark.docu_area.br.slr = slr;
            p_docu->anchor_mark.docu_area.br.object_position = object_position_find.object_data.object_position_start;

            trace_2(TRACE_APP_SKEL,
                    TEXT("CLICK_LEFT_DRAG col: ") COL_TFMT TEXT(", row: ") ROW_TFMT,
                    p_docu->anchor_mark.docu_area.tl.slr.col,
                    p_docu->anchor_mark.docu_area.tl.slr.row);

            /* put anchor block in list */
            if(!p_docu->mark_info_cells.h_markers)
                status_assert(anchor_to_markers_new(p_docu));
            else
                anchor_to_markers_update(p_docu);

            if(T5_EVENT_CLICK_RIGHT_SINGLE == t5_message)
                anchor_to_markers_finish(p_docu);

            markers_show(p_docu);

            if(T5_EVENT_CLICK_RIGHT_DRAG == t5_message)
                host_drag_start(NULL);
        }
    }

    return(status);
}

T5_MSG_PROTO(static, cells_event_click_right_double, _InoutRef_ P_SKELEVENT_CLICK p_skelevent_click)
{
    return(object_cells_default(p_docu, t5_message, p_skelevent_click));
}

T5_MSG_PROTO(static, cells_event_click_right_triple, _InoutRef_ P_SKELEVENT_CLICK p_skelevent_click)
{
    return(object_cells_default(p_docu, t5_message, p_skelevent_click));
}

T5_MSG_PROTO(static, cells_event_click_single, _InoutRef_ P_SKELEVENT_CLICK p_skelevent_click)
{
    const T5_MESSAGE t5_message_right = T5_EVENT_CLICK_RIGHT_SINGLE;
    const T5_MESSAGE t5_message_effective = right_message_if_shift(t5_message, t5_message_right, p_skelevent_click);

    if(t5_message_right == t5_message_effective)
        return(cells_event_click_right(p_docu, t5_message_effective, p_skelevent_click));

    return(cells_event_click_left_single(p_docu, t5_message_effective, p_skelevent_click));
}

T5_MSG_PROTO(static, cells_event_click_double, _InoutRef_ P_SKELEVENT_CLICK p_skelevent_click)
{
    const T5_MESSAGE t5_message_right = T5_EVENT_CLICK_RIGHT_DOUBLE;
    const T5_MESSAGE t5_message_effective = right_message_if_shift(t5_message, t5_message_right, p_skelevent_click);

    if(t5_message_right == t5_message_effective)
        return(cells_event_click_right_double(p_docu, t5_message_effective, p_skelevent_click));

    return(cells_event_click_left_double(p_docu, t5_message_effective, p_skelevent_click));
}

T5_MSG_PROTO(static, cells_event_click_triple, _InoutRef_ P_SKELEVENT_CLICK p_skelevent_click)
{
    const T5_MESSAGE t5_message_right = T5_EVENT_CLICK_RIGHT_TRIPLE;
    const T5_MESSAGE t5_message_effective = right_message_if_shift(t5_message, t5_message_right, p_skelevent_click);

    if(t5_message_right == t5_message_effective)
        return(cells_event_click_right_triple(p_docu, t5_message_effective, p_skelevent_click));

    return(cells_event_click_left_triple(p_docu, t5_message_effective, p_skelevent_click));
}

T5_MSG_PROTO(static, cells_event_click_drag, _InoutRef_ P_SKELEVENT_CLICK p_skelevent_click)
{
    const T5_MESSAGE t5_message_right = T5_EVENT_CLICK_RIGHT_DRAG;
    const T5_MESSAGE t5_message_effective = right_message_if_shift(t5_message, t5_message_right, p_skelevent_click);

    if(t5_message_right == t5_message_effective)
        return(cells_event_click_right(p_docu, t5_message_effective, p_skelevent_click));

    return(cells_event_click_left_drag(p_docu, t5_message_effective, p_skelevent_click));
}

T5_MSG_PROTO(static, cells_event_click_drags, _InoutRef_ P_SKELEVENT_CLICK p_skelevent_click)
{
    STATUS status = STATUS_OK;

    if(CB_CODE_NOREASON != * (PC_S32) p_skelevent_click->data.drag.p_reason_data)
    {
        const OBJECT_ID object_id = (OBJECT_ID) (* (PC_S32) p_skelevent_click->data.drag.p_reason_data - CB_CODE_PASS_TO_OBJECT);
        return(object_call_id(object_id, p_docu, t5_message, p_skelevent_click));
    }

    if(T5_EVENT_CLICK_DRAG_STARTED == t5_message)
        return(status); /* not interesting to us */

    if(T5_EVENT_CLICK_DRAG_ABORTED == t5_message)
    {
        status_assert(maeve_event(p_docu, T5_MSG_SELECTION_CLEAR, P_DATA_NONE));
        return(status);
    }

    if(p_docu->mark_info_cells.h_markers)
    {
        SLR slr;
        OBJECT_POSITION_FIND object_position_find;

        status_return(slr_owner_from_skel_point(p_docu, &slr, &object_position_find.skel_rect.tl, &p_skelevent_click->skel_point, ON_ROW_EDGE_GO_UP));

        position_set_from_skel_point(p_docu, &object_position_find, &p_skelevent_click->skel_point, &slr, NO_CELLS_EDIT);

        p_docu->anchor_mark.docu_area.br.slr = slr;
        p_docu->anchor_mark.docu_area.br.object_position = object_position_find.object_data.object_position_start;

        if(T5_EVENT_CLICK_DRAG_FINISHED == t5_message)
            anchor_to_markers_finish(p_docu);
        else
            anchor_to_markers_update(p_docu);

        markers_show(p_docu);
    }

    return(status);
}

T5_MSG_PROTO(static, cells_event_fileinsert_doinsert_1, _InoutRef_ P_SKELEVENT_CLICK p_skelevent_click)
{
    SKEL_POINT tl;
    SLR slr;
    OBJECT_DATA object_data;
    STATUS status;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    cur_change_before(p_docu);

    status_return(slr_owner_from_skel_point(p_docu, &slr, &tl, &p_skelevent_click->skel_point, ON_ROW_EDGE_GO_UP));

    (void) cell_data_from_slr(p_docu, &object_data, &slr);

    status = cell_call_id(object_data.object_id, p_docu, T5_EVENT_FILEINSERT_DOINSERT_1, p_skelevent_click, NO_CELLS_EDIT);

    cur_change_after(p_docu);

    if(p_skelevent_click->processed && status_ok(status))
        status = STATUS_DONE;

    return(status);
}

T5_MSG_PROTO(static, cells_event_fileinsert_doinsert, _InoutRef_ P_SKELEVENT_CLICK p_skelevent_click)
{
    const OBJECT_ID object_id = object_id_from_t5_filetype(p_skelevent_click->data.fileinsert.t5_filetype);
    STATUS status;
    MSG_INSERT_FILE msg_insert_file;
    zero_struct(msg_insert_file);

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(OBJECT_ID_NONE == object_id)
        return(create_error(ERR_UNKNOWN_FILETYPE));

    p_skelevent_click->processed = 1;

    cur_change_before(p_docu);

    msg_insert_file.filename = p_skelevent_click->data.fileinsert.filename;
    msg_insert_file.t5_filetype = p_skelevent_click->data.fileinsert.t5_filetype;
    msg_insert_file.scrap_file = !p_skelevent_click->data.fileinsert.safesource;
    msg_insert_file.insert = TRUE;
    msg_insert_file.ctrl_pressed = p_skelevent_click->click_context.ctrl_pressed;
    /***msg_insert_file.of_ip_format.flags.insert = 1;*/
    msg_insert_file.position = p_docu->cur;
    msg_insert_file.skel_point = p_skelevent_click->skel_point;
    status = object_call_id_load(p_docu, (OBJECT_ID_SKEL == object_id) ? T5_MSG_INSERT_OWNFORM : T5_MSG_INSERT_FOREIGN, &msg_insert_file, object_id);

    cur_change_after(p_docu);

    return(status);
}

T5_MSG_PROTO(static, cells_event_keys, _InRef_ P_SKELEVENT_KEYS p_skelevent_keys)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(!status_ok(check_protection_simple(p_docu, TRUE)))
        return(STATUS_OK);

    {
    STATUS status = selection_delete_auto(p_docu);
    status_return(status);
    if(status_done(status))
        return(STATUS_OK);
    } /*block*/

    assert(0 != quick_ublock_bytes(p_skelevent_keys->p_quick_ublock));
    {
    U32 bytes_of_char;
    UCS4 ucs4 = uchars_char_decode(quick_ublock_uchars(p_skelevent_keys->p_quick_ublock), /*ref*/bytes_of_char);
    OBJECT_KEYS object_keys;
    consume_bool(cell_data_from_position(p_docu, &object_keys.object_data, &p_docu->cur));
    object_keys.p_skelevent_keys = p_skelevent_keys;
    return(process_keys(p_docu, T5_MSG_OBJECT_KEYS, &object_keys, &object_keys.object_data, ucs4));
    } /*block*/
}

T5_MSG_PROTO(static, cells_msg_col_auto_width, _InoutRef_ P_COL_AUTO_WIDTH p_col_auto_width)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    p_col_auto_width->width =
        col_auto_width(p_docu, p_col_auto_width->col, p_col_auto_width->row_s, p_col_auto_width->row_e, p_col_auto_width->allow_special);

    return(STATUS_OK);
}

T5_MSG_PROTO(static, cells_msg_col_width_adjust, _InRef_ P_COL_WIDTH_ADJUST p_col_width_adjust)
{
    STATUS status;
    STYLE style;
    STYLE_DOCU_AREA_ADD_PARM style_docu_area_add_parm;
    DOCU_AREA docu_area;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    style_init(&style);

    STYLE_DOCU_AREA_ADD_STYLE(&style_docu_area_add_parm, &style);

    /* extract relevant docu_area */
    if(p_docu->mark_info_cells.h_markers)
        docu_area_from_markers_first(p_docu, &docu_area);
    else
    {
        P_STYLE_DOCU_AREA p_style_docu_area = style_effect_source_find(p_docu, &style.selector, &p_docu->cur, &p_docu->h_style_docu_area, TRUE);
        PTR_ASSERT(p_style_docu_area);
        if(NULL != p_style_docu_area)
            docu_area = p_style_docu_area->docu_area;
    }

    style_bit_set(&style, STYLE_SW_CS_WIDTH);
    style.col_style.width = p_col_width_adjust->width_left;

    docu_area.tl.slr.col = p_col_width_adjust->col;
    docu_area.br.slr.col = p_col_width_adjust->col + 1;

    status_return(status = style_docu_area_add(p_docu, &docu_area, &style_docu_area_add_parm));

    docu_modify(p_docu);

    if(p_col_width_adjust->col + 1 < n_cols_logical(p_docu))
    {
        style.col_style.width = p_col_width_adjust->width_right;

        docu_area.tl.slr.col += 1;
        docu_area.br.slr.col += 1;

        status_return(status = style_docu_area_add(p_docu, &docu_area, &style_docu_area_add_parm));
    }

    return(status);
}

T5_MSG_PROTO(static, cells_msg_object_at_current_position, _InoutRef_ P_OBJECT_AT_CURRENT_POSITION p_object_at_current_position)
{
    /* SKS 17apr95 no point sending T5_MSG_OBJECT_WANTS_LOAD to ce_edit is it? Find the real cell owner */
    const OBJECT_ID object_id = cell_owner_from_slr(p_docu, &p_docu->cur.slr);

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    {
    OBJECT_WANTS_LOAD object_wants_load;
    zero_struct(object_wants_load);
    status_consume(object_call_id(object_id, p_docu, T5_MSG_OBJECT_WANTS_LOAD, &object_wants_load));
    if(object_wants_load.object_wants_load)
        p_object_at_current_position->object_id = object_id;
    } /*block*/

    return(STATUS_OK);
}

T5_MSG_PROTO(static, cells_msg_redraw_region, _InRef_ PC_REGION p_region)
{
    REGION region_on_screen, region_clipped;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(region_intersect_region_out(&region_clipped, p_region, region_visible(p_docu, &region_on_screen)))
    {
        SKEL_RECT skel_rect;

        {
        DOCU_AREA docu_area;
        docu_area_from_region(&docu_area, &region_clipped);
        skel_rect_from_docu_area(p_docu, &skel_rect, &docu_area);
        } /*block*/

        {
        RECT_FLAGS rect_flags;
        RECT_FLAGS_CLEAR(rect_flags);
        view_update_later(p_docu, UPDATE_PANE_CELLS_AREA, &skel_rect, rect_flags);
        } /*block*/
    }

    return(STATUS_OK);
}

T5_MSG_PROTO(static, cells_msg_redraw_cells, _InRef_ PC_SLR p_slr)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(row_is_visible(p_docu, p_slr->row))
    {
        SKEL_RECT skel_rect;

        skel_rect_from_slr(p_docu, &skel_rect, p_slr);

        {
        RECT_FLAGS rect_flags;
        RECT_FLAGS_CLEAR(rect_flags);
        view_update_later(p_docu, UPDATE_PANE_CELLS_AREA, &skel_rect, rect_flags);
        } /*block*/
    }

    return(STATUS_OK);
}

T5_MSG_PROTO(static, cells_msg_caret_show_claim, P_CARET_SHOW_CLAIM p_caret_show_claim)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    caret_position_set_show(p_docu, p_caret_show_claim->scroll);

    return(STATUS_OK);
}

/******************************************************************************
*
* Return useful info, so that host menu system can grey out bits of the tree
*
******************************************************************************/

T5_MSG_PROTO(static, cells_msg_menu_other_info, _InoutRef_ P_MENU_OTHER_INFO p_menu_other_info)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(OBJECT_ID_CELLS == p_docu->focus_owner)
    {
        MARK_INFO mark_info;

        if(status_fail(object_skel(p_docu, T5_MSG_MARK_INFO_READ, &mark_info)))
            mark_info.h_markers = 0;

        p_menu_other_info->cells_selection = (0 != mark_info.h_markers);
    }

    return(STATUS_OK);
}

T5_MSG_PROTO(static, cells_msg_ruler_info, _InoutRef_ P_RULER_INFO p_ruler_info)
{
    ARRAY_HANDLE array_handle;
    ROW row;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(!p_docu->mark_info_cells.h_markers)
        row = p_docu->cur.slr.row;
    else
        limits_from_docu_area(p_docu, NULL, NULL, &row, NULL, &p_markers_first(p_docu)->docu_area);

    status_return(skel_col_enum(p_docu, row, (PAGE) -1, p_ruler_info->col, &array_handle));

    if(0 == array_elements(&array_handle))
        return(STATUS_FAIL);

    p_ruler_info->valid = TRUE;
    p_ruler_info->col_info = *array_basec(&array_handle, COL_INFO);

    return(STATUS_OK);
}

T5_MSG_PROTO(static, cells_msg_selection_make, _InRef_ PC_DOCU_AREA p_docu_area)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    p_docu->anchor_mark.docu_area = *p_docu_area;

    /* 'raw' markers are inclusive, unfortunately */
    p_docu->anchor_mark.docu_area.br.slr.col -= 1;
    p_docu->anchor_mark.docu_area.br.slr.row -= 1;

    status_assert(anchor_to_markers_new(p_docu));
    anchor_to_markers_finish(p_docu);
    markers_show(p_docu);

    return(STATUS_OK);
}

T5_MSG_PROTO(static, cells_msg_selection_replace, _InRef_ P_SELECTION_REPLACE p_selection_replace)
{
    STATUS status = STATUS_OK;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(p_docu->mark_info_cells.h_markers)
    {
        OBJECT_STRING_REPLACE object_string_replace;

        cur_change_before(p_docu);

        status_consume(object_data_from_docu_area_tl(p_docu, &object_string_replace.object_data, p_docu_area_from_markers_first(p_docu)));
        object_string_replace.p_quick_ublock = p_selection_replace->p_quick_ublock;
        object_string_replace.copy_capitals = p_selection_replace->copy_capitals;

        status_assert(maeve_event(p_docu, T5_MSG_SELECTION_CLEAR, P_DATA_NONE));

        if(status_ok(status = object_call_id(object_string_replace.object_data.object_id,
                                             p_docu,
                                             T5_MSG_OBJECT_STRING_REPLACE,
                                             &object_string_replace)))
            p_selection_replace->object_position_after = object_string_replace.object_position_after;

        cur_change_after(p_docu);
    }

    return(status);
}

T5_MSG_PROTO(static, cells_msg_selection_style, _InoutRef_ P_SELECTION_STYLE p_selection_style)
{
    DOCU_AREA docu_area;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(p_docu->mark_info_cells.h_markers)
        docu_area_from_markers_first(p_docu, &docu_area);
    else
        docu_area_from_position(&docu_area, &p_docu->cur);

    style_of_area(p_docu, &p_selection_style->style_out, &p_selection_style->selector_fuzzy_out, &p_selection_style->selector_in, &docu_area);

    return(STATUS_OK);
}

/******************************************************************************
*
* events for no specific document and for the panes of a document come here
*
******************************************************************************/

OBJECT_PROTO(static, object_cells_default)
{
    /* 12.8.94 MRJC call current object for default case */
    if(IS_DOCU_NONE(p_docu))
        return(STATUS_OK);

    if(!p_docu->flags.document_active)
        return(STATUS_OK);

    if(p_docu->focus_owner == OBJECT_ID_CELLS)
    {
        OBJECT_DATA object_data;
        (void) cell_data_from_slr(p_docu, &object_data, &p_docu->cur.slr);
        return(cell_call_id(object_data.object_id, p_docu, t5_message, p_data, NO_CELLS_EDIT));
    }

    return(STATUS_OK);
}

T5_MSG_PROTO(static, cells_msg_initclose, _InRef_ PC_MSG_INITCLOSE p_msg_initclose)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__STARTUP:
#if WINDOWS
        void_WrapOsBoolChecking(0 != (cf_fireworkz = RegisterClipboardFormat(TEXT("Colton Software Fireworkz"))));
#endif
        return(resource_init(OBJECT_ID_CELLS, MSG_WEAK, P_BOUND_RESOURCES_OBJECT_ID_CELLS));

    case T5_MSG_IC__EXIT1:
        local_clip_data_dispose();
        return(STATUS_OK);

    /* T5_MSG_IC__INIT1 moved to sk_root 8.2.94 MRJC */

    case T5_MSG_IC__CLOSE1:
        maeve_event_handler_del(p_docu, maeve_event_ob_cells, (CLIENT_HANDLE) 0);
        return(STATUS_OK);

    default:
        return(STATUS_OK);
    }
}

T5_MSG_PROTO(static, cells_msg_init_flow, _InRef_ P_OBJECT_ID p_object_id)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);
    UNREFERENCED_PARAMETER_InRef_(p_object_id);

    position_init_from_col_row(&p_docu->cur, 0, 0);
    p_docu->old = p_docu->cur;

    return(maeve_event_handler_add(p_docu, maeve_event_ob_cells, (CLIENT_HANDLE) 0));
}

T5_MSG_PROTO(static, cells_msg_goto_slr, _InRef_ P_SKELCMD_GOTO p_skelcmd_goto)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    caret_to_slr(p_docu, &p_skelcmd_goto->slr, &p_skelcmd_goto->skel_point, FALSE, (BOOL) p_skelcmd_goto->keep_selection, FALSE, FALSE);

    return(STATUS_OK);
}

T5_CMD_PROTO(static, object_cells_cmd)
{
    assert(T5_CMD__ACTUAL_END <= T5_CMD__END);

    switch(T5_MESSAGE_CMD_OFFSET(t5_message))
    {
    default:
        return(object_cells_default(p_docu, t5_message, de_const_cast(P_T5_CMD, p_t5_cmd)));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_SELECT_WORD):
        return(cells_cmd_select_word(p_docu, t5_message, p_t5_cmd));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_SELECT_CELL):
        return(cells_cmd_select_cell(p_docu, t5_message, p_t5_cmd));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_SELECT_DOCUMENT):
        return(cells_cmd_select_document(p_docu, t5_message, p_t5_cmd));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_TAB_LEFT):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_TAB_RIGHT):
        return(cells_cmd_tab_lr(p_docu, t5_message, p_t5_cmd));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_INSERT_FIELD_DATE):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_INSERT_FIELD_FILE_DATE):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_INSERT_FIELD_PAGE_X):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_INSERT_FIELD_PAGE_Y):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_INSERT_FIELD_SS_NAME):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_INSERT_FIELD_MS_FIELD):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_INSERT_FIELD_WHOLENAME):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_INSERT_FIELD_LEAFNAME):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_INSERT_FIELD_RETURN):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_INSERT_FIELD_SOFT_HYPHEN):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_INSERT_FIELD_TAB):
        return(cells_cmd_insert_field(p_docu, t5_message, p_t5_cmd));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_STYLE_APPLY_SOURCE):
        return(cells_cmd_style_apply_source(p_docu, t5_message, p_t5_cmd));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_STYLE_APPLY):
        return(cells_cmd_style_apply(p_docu, t5_message, p_t5_cmd));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_STYLE_REGION_COUNT):
        return(cells_cmd_style_region_count(p_docu));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_STYLE_REGION_CLEAR):
        return(cells_cmd_style_region_clear(p_docu));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_AUTO_WIDTH):
        return(cells_cmd_auto_width(p_docu));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_AUTO_HEIGHT):
        return(cells_cmd_auto_height(p_docu));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_STRADDLE_HORZ):
        return(cells_cmd_straddle_horz(p_docu));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_RETURN):
        return(cells_cmd_return(p_docu));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_DELETE_CHARACTER_LEFT):
        return(cells_cmd_delete_character_left(p_docu));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_DELETE_CHARACTER_RIGHT):
        return(cells_cmd_delete_character_right(p_docu));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_CURSOR_LEFT):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_WORD_LEFT):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_SHIFT_CURSOR_LEFT):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_SHIFT_WORD_LEFT):
        return(cursor_left(p_docu, t5_message));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_CURSOR_RIGHT):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_WORD_RIGHT):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_SHIFT_CURSOR_RIGHT):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_SHIFT_WORD_RIGHT):
        return(cursor_right(p_docu, t5_message));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_CURSOR_UP):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_SHIFT_CURSOR_UP):
        return(cursor_up(p_docu, t5_message, CARET_STAY));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_CURSOR_DOWN):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_SHIFT_CURSOR_DOWN):
        return(cursor_down(p_docu, t5_message, CARET_STAY));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_PAGE_DOWN):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_PAGE_UP):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_SHIFT_PAGE_DOWN):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_SHIFT_PAGE_UP):
        return(cells_cmd_page_up_down(p_docu, t5_message, p_t5_cmd));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_LINE_START):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_SHIFT_LINE_START):
        return(cells_cmd_line_start(p_docu, t5_message, p_t5_cmd));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_LINE_END):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_SHIFT_LINE_END):
        return(cells_cmd_line_end(p_docu, t5_message, p_t5_cmd));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_DOCUMENT_TOP):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_SHIFT_DOCUMENT_TOP):
        return(cells_cmd_document_top(p_docu, t5_message, p_t5_cmd));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_DOCUMENT_BOTTOM):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_SHIFT_DOCUMENT_BOTTOM):
        return(cells_cmd_document_bottom(p_docu, t5_message, p_t5_cmd));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_FIRST_COLUMN):
        return(cells_cmd_first_column(p_docu, t5_message, p_t5_cmd));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_LAST_COLUMN):
        return(cells_cmd_last_column(p_docu, t5_message, p_t5_cmd));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_TOGGLE_MARKS):
        return(cells_cmd_toggle_marks(p_docu, t5_message, p_t5_cmd));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_SELECTION_CLEAR):
        return(cells_cmd_selection_clear(p_docu));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_SELECTION_COPY):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_SELECTION_CUT):
        return(cells_cmd_selection_copy_or_cut(p_docu, t5_message));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_SELECTION_DELETE):
        return(cells_cmd_selection_delete(p_docu));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_PASTE_AT_CURSOR):
        return(cells_cmd_paste_at_cursor(p_docu));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_INSERT_COL):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_DELETE_COL):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_DELETE_ROW):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_INSERT_ROW):
        return(row_col_ins_del(p_docu, t5_message, P_S32_NONE));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_ADD_COLS):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_ADD_ROWS):
        return(cells_cmd_add_cr(p_docu, t5_message, p_t5_cmd));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_ADD_COLS_INTRO):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_ADD_ROWS_INTRO):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_INSERT_TABLE):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_INSERT_TABLE_INTRO):

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_SORT_INTRO):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_WORD_COUNT):
        return(object_call_id_load(p_docu, t5_message, de_const_cast(P_T5_CMD, p_t5_cmd), OBJECT_ID_SKEL_SPLIT));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_SETC_UPPER):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_SETC_LOWER):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_SETC_INICAP):
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_SETC_SWAP):
        return(cells_cmd_setc(p_docu, t5_message, p_t5_cmd));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_SEARCH):
        return(cells_cmd_search(p_docu, t5_message, p_t5_cmd));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_SEARCH_INTRO):
        return(t5_cmd_search_intro(p_docu, t5_message, p_t5_cmd));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_SEARCH_BUTTON_POSS_DB_QUERIES):
        return(t5_cmd_search_button_poss_db_queries(p_docu, t5_message, p_t5_cmd));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_SEARCH_BUTTON_POSS_DB_QUERY):
        return(t5_cmd_search_button_poss_db_query(p_docu, t5_message, p_t5_cmd));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_SORT):
        return(cells_cmd_sort(p_docu, t5_message, p_t5_cmd));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_BOX):
        return(cells_cmd_box(p_docu, t5_message, p_t5_cmd));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_OBJECT_CONVERT):
        return(cells_cmd_object_convert(p_docu, t5_message, p_t5_cmd));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_SNAPSHOT):
        return(cells_cmd_snapshot(p_docu, t5_message, p_t5_cmd));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_BLOCK_CLEAR):
        return(cells_cmd_block_clear(p_docu, t5_message, p_t5_cmd));

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_FORCE_RECALC):
        /* SKS quick fudge 18.10.93 to allow charts to get calced; just pass on to SS here */
        return(object_call_id(OBJECT_ID_SS, p_docu, t5_message, de_const_cast(P_T5_CMD, p_t5_cmd)));

#if RISCOS
    case T5_MESSAGE_CMD_OFFSET(T5_CMD_THESAURUS):
        return(cells_cmd_thesaurus(p_docu));
#endif
    }
}

OBJECT_PROTO(extern, object_cells);
OBJECT_PROTO(extern, object_cells)
{
    if(T5_MESSAGE_IS_CMD(t5_message))
        return(object_cells_cmd(p_docu, t5_message, (PC_T5_CMD) p_data));

    switch(t5_message)
    {
    default:
        return(object_cells_default(p_docu, t5_message, p_data));

    case T5_EVENT_CLICK_LEFT_SINGLE:
    case T5_EVENT_CLICK_RIGHT_SINGLE:
        return(cells_event_click_single(p_docu, t5_message, (P_SKELEVENT_CLICK) p_data));

    case T5_EVENT_CLICK_LEFT_DOUBLE:
    case T5_EVENT_CLICK_RIGHT_DOUBLE:
        return(cells_event_click_double(p_docu, t5_message, (P_SKELEVENT_CLICK) p_data));

    case T5_EVENT_CLICK_LEFT_TRIPLE:
    case T5_EVENT_CLICK_RIGHT_TRIPLE:
        return(cells_event_click_triple(p_docu, t5_message, (P_SKELEVENT_CLICK) p_data));

    case T5_EVENT_CLICK_LEFT_DRAG:
    case T5_EVENT_CLICK_RIGHT_DRAG:
        return(cells_event_click_drag(p_docu, t5_message, (P_SKELEVENT_CLICK) p_data));

    case T5_EVENT_CLICK_DRAG_STARTED:
    case T5_EVENT_CLICK_DRAG_FINISHED:
    case T5_EVENT_CLICK_DRAG_MOVEMENT:
    case T5_EVENT_CLICK_DRAG_ABORTED:
        return(cells_event_click_drags(p_docu, t5_message, (P_SKELEVENT_CLICK) p_data));

    case T5_EVENT_FILEINSERT_DOINSERT_1:
        return(cells_event_fileinsert_doinsert_1(p_docu, t5_message, (P_SKELEVENT_CLICK) p_data));

    case T5_EVENT_FILEINSERT_DOINSERT:
        return(cells_event_fileinsert_doinsert(p_docu, t5_message, (P_SKELEVENT_CLICK) p_data));

    case T5_EVENT_KEYS:
        return(cells_event_keys(p_docu, t5_message, (P_SKELEVENT_KEYS) p_data));

    case T5_EVENT_REDRAW:
        return(skel_event_redraw(p_docu, t5_message, (P_SKELEVENT_REDRAW) p_data));

    case T5_MSG_INITCLOSE:
        return(cells_msg_initclose(p_docu, t5_message, (PC_MSG_INITCLOSE) p_data));

    case T5_MSG_INIT_FLOW:
        return(cells_msg_init_flow(p_docu, t5_message, (P_OBJECT_ID) p_data));

    case T5_MSG_GOTO_SLR:
        return(cells_msg_goto_slr(p_docu, t5_message, (P_SKELCMD_GOTO) p_data));

    case T5_MSG_REDRAW_REGION:
        return(cells_msg_redraw_region(p_docu, t5_message, (PC_REGION) p_data));

    case T5_MSG_REDRAW_CELLS:
        return(cells_msg_redraw_cells(p_docu, t5_message, (PC_SLR) p_data));

    case T5_MSG_CARET_SHOW_CLAIM:
        return(cells_msg_caret_show_claim(p_docu, t5_message, (P_CARET_SHOW_CLAIM) p_data));

    case T5_MSG_OBJECT_AT_CURRENT_POSITION:
        return(cells_msg_object_at_current_position(p_docu, t5_message, (P_OBJECT_AT_CURRENT_POSITION) p_data));

    case T5_MSG_MARK_INFO_READ:
        return(cells_msg_mark_info_read(p_docu, t5_message, (P_MARK_INFO) p_data));

    case T5_MSG_SELECTION_STYLE:
        return(cells_msg_selection_style(p_docu,  t5_message, (P_SELECTION_STYLE) p_data));

    case T5_MSG_MENU_OTHER_INFO:
        return(cells_msg_menu_other_info(p_docu,  t5_message, (P_MENU_OTHER_INFO) p_data));

    case T5_MSG_RULER_INFO:
        return(cells_msg_ruler_info(p_docu, t5_message, (P_RULER_INFO) p_data));

    case T5_MSG_SELECTION_MAKE:
        return(cells_msg_selection_make(p_docu, t5_message, (PC_DOCU_AREA) p_data));

    case T5_MSG_CHECK_PROTECTION:
        return(cells_msg_check_protection(p_docu, t5_message, (P_CHECK_PROTECTION) p_data));

    case T5_MSG_COL_AUTO_WIDTH:
        return(cells_msg_col_auto_width(p_docu, t5_message, (P_COL_AUTO_WIDTH) p_data));

    case T5_MSG_SELECTION_REPLACE:
        return(cells_msg_selection_replace(p_docu, t5_message, (P_SELECTION_REPLACE) p_data));

    case T5_MSG_COL_WIDTH_ADJUST:
        return(cells_msg_col_width_adjust(p_docu, t5_message, (P_COL_WIDTH_ADJUST) p_data));

    case T5_MSG_MOVE_LEFT_CELL:
        return(cells_msg_move_cell_left(p_docu));

    case T5_MSG_MOVE_RIGHT_CELL:
        return(cells_msg_move_cell_right(p_docu));

    case T5_MSG_MOVE_UP_CELL:
        return(cells_msg_move_cell_up(p_docu));

    case T5_MSG_MOVE_DOWN_CELL:
        return(cells_msg_move_cell_down(p_docu));
    }
}

/* end of ob_cells.c */
