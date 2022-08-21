/* riscos/ho_event.c */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* SKS May 1993 */

#include "common/gflags.h"

#if RISCOS

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#define EXPOSE_RISCOS_SWIS 1

#include "ob_skel/xp_skelr.h"

#include "ob_skel/prodinfo.h"

#define ICONBAR_SPRITE_MIN_H (34*2)
#define ICONBAR_SPRITE_MIN_V (17*4)

_Check_return_
static BOOL
preprocess_mouse_click(
    _Inout_     WimpPollBlock * const block);

#include "ob_skel/riscos/event.c"

#include "ob_skel/riscos/wimpt.c"

/* we must do some preprocessing on all mouse button events */

_Check_return_
static BOOL
preprocess_mouse_click(
    _Inout_     WimpPollBlock * const block)
{
    BOOL double_click_type = FALSE;

    {
    int window_button_type;

    if(block->mouse_click.icon_handle == -1)
    {
        /* due to Neil being an idiot we cannot retrieve the work area flags for a window on RISC OS 2 so they'd better be standard */
        const WimpWindowWithBitset * p_wimp_wind = win_wimp_wind_query(block->mouse_click.window_handle);
        window_button_type = ButtonType_DoubleClickDrag;
        if(NULL != p_wimp_wind)
            window_button_type = p_wimp_wind->work_flags.bits.button_type; /* but the cunning cache of winx_create_window may help */
    }
    else
    {
        WimpGetIconStateBlock icon_state_block;
        icon_state_block.window_handle = block->mouse_click.window_handle;
        icon_state_block.icon_handle = block->mouse_click.icon_handle;
        void_WrapOsErrorReporting(wimp_get_icon_state(&icon_state_block));
        window_button_type = (* (WimpIconFlagsBitset *) &icon_state_block.icon.flags).button_type;
    }

    switch(window_button_type)
    {
    case ButtonType_DoubleClickDrag:
        double_click_type = TRUE;
        break;

    default:
        break;
    }
    } /*block*/

    switch(block->mouse_click.buttons)
    {
    case Wimp_MouseButtonMenu: /* 0x002 ALWAYS 'menu' */
        event_.single_click.mce = block->mouse_click;
        break;

    case Wimp_MouseButtonSelect: /* 0x004 Double 'select' */
        if(double_click_type && (event_.single_click.mce.buttons == Wimp_MouseButtonSingleSelect))
        {
            event_.triple_click.mce = block->mouse_click;
            event_.triple_click.t = monotime();
            break;
        }

        block->mouse_click.buttons = Wimp_MouseButtonSingleSelect; /* force a single click instead of a double */

        /*FALLTHRU*/

    case Wimp_MouseButtonSingleSelect: /* 0x400 Single 'select' */
        event_.single_click.mce = block->mouse_click;
        break;

    case Wimp_MouseButtonAdjust: /* 0x001 Double 'adjust' */
        if(double_click_type && (event_.single_click.mce.buttons == Wimp_MouseButtonSingleAdjust))
        {
            event_.triple_click.mce = block->mouse_click;
            event_.triple_click.t = monotime();
            break;
        }

        block->mouse_click.buttons = Wimp_MouseButtonSingleAdjust; /* force a single click instead of a double */

        /*FALLTHRU*/

    case Wimp_MouseButtonSingleAdjust: /* 0x100 Single 'adjust' */
        event_.single_click.mce = block->mouse_click;
        break;

    case Wimp_MouseButtonDragSelect:
    case Wimp_MouseButtonDragAdjust:
        {
        /* watch out for PDM making up drag events when the pointer reappears in another window */
        if(event_.single_click.mce.mouse_x == block->mouse_click.mouse_x)
        if(event_.single_click.mce.mouse_y == block->mouse_click.mouse_y)
        if(event_.single_click.mce.window_handle == block->mouse_click.window_handle)
        if(event_.single_click.mce.icon_handle   == block->mouse_click.icon_handle)
        {
            WimpGetPointerInfoBlock pointer_info;
            void_WrapOsErrorReporting(wimp_get_pointer_info(&pointer_info));
            if(event_.single_click.mce.window_handle != pointer_info.window_handle)
                return(STATUS_OK); /* get another event, this one was not good */
        }
        break;
        }

    default:
        break;
    }

    if(block->mouse_click.buttons & (Wimp_MouseButtonSingleSelect | Wimp_MouseButtonSingleAdjust))
    {
        if(double_click_type && event_.triple_click.mce.buttons)
        {
            int buttons = event_.triple_click.mce.buttons;
            event_.triple_click.mce.buttons = 0; /* whenever we get the opportunity, eat this value */
            if( ((block->mouse_click.buttons & Wimp_MouseButtonSingleSelect) && (buttons & Wimp_MouseButtonSelect)) ||
                ((block->mouse_click.buttons & Wimp_MouseButtonSingleAdjust) && (buttons & Wimp_MouseButtonAdjust)) )
            {
                /* triple click requires three of the same buttons clicked in the same place within a given time limit */

                if( (block->mouse_click.window_handle == event_.triple_click.mce.window_handle) &&
                    (block->mouse_click.icon_handle   == event_.triple_click.mce.icon_handle  ) )
                {
                    /* read double click cancel distance */
                    int cancel_distance;
                    int dx = event_.triple_click.mce.mouse_x - block->mouse_click.mouse_x;
                    int dy = event_.triple_click.mce.mouse_y - block->mouse_click.mouse_y;

                    cancel_distance  = (_kernel_osbyte(161, 0x16, 0) & 0xFF00) >> 8;
                    cancel_distance ^= 32;

                    if((dx <= cancel_distance) && (dx >= -cancel_distance) && (dy < cancel_distance) && (dy >= -cancel_distance))
                    {
                        /* read RISC OS 3 double click timeout */
                        int timeout;

                        timeout  = (_kernel_osbyte(161, 0xDF, 0) & 0xFF00) >> 8;
                        timeout ^= 10;
                        timeout *= 10; /*cs*/

                        /* conditionally mutate event */
                        if(monotime_diff(event_.triple_click.t + timeout) <= 0)
                        {
                            /* use the cached position! */
                            block->mouse_click.mouse_x = event_.triple_click.mce.mouse_x;
                            block->mouse_click.mouse_y = event_.triple_click.mce.mouse_y;

                            block->mouse_click.buttons = (buttons & Wimp_MouseButtonAdjust) ? Wimp_MouseButtonTripleAdjust : Wimp_MouseButtonTripleSelect;
                        }
                    }

                }
            }
        }
    }

    return(STATUS_DONE);
}

/******************************************************************************
*
* get and process events - returning only if foreground null events are wanted.
*
* centralises null processing, modeless dialog, accelerator translation and message dispatch
*
******************************************************************************/

_Check_return_
extern WM_EVENT
wm_event_get(
    _InVal_     BOOL fgNullEventsWanted)
{
    static BOOL last_event_was_null = FALSE;

    int event_code;
    WimpPollBlock event_data;
    BOOL peek, bgScheduledEventsWanted, bgNullEventsWanted, bgNullTestWanted;
    S32 res;

    /* start exit sequence; anyone interested in setting up bgNulls
     * better do their thinking in the null query handlers
    */
    bgScheduledEventsWanted = (scheduled_events_do_query() == SCHEDULED_EVENTS_REQUIRED);

    if(last_event_was_null)
    {
        /* only ask our null query handlers at the end of each real
         * requested null event whether they want to continue having nulls
         * or not; this saves wasted time querying after every single event
         * null or otherwise; so we must request a single test null event
         * to come through to query the handlers whenever some idle time
         * turns up
         */
        bgNullEventsWanted = (null_events_do_query() == NULL_EVENTS_REQUIRED);
        trace_1(TRACE_APP_WM_EVENT, TEXT("wm_event_get: last_event_was_null: query handlers returns bgNullEventsWanted = %s"), report_boolstring(bgNullEventsWanted));
        bgNullTestWanted   = FALSE;
    }
    else
    {
        bgNullEventsWanted = FALSE;
        bgNullTestWanted   = TRUE /*Null_HandlersPresent()*/;
    }

    peek = (fgNullEventsWanted || bgScheduledEventsWanted || bgNullEventsWanted || bgNullTestWanted);

    {
    /* note that other parts of RISC_OSLib etc may be trying
     * to enable nulls independently using event/win so take care
    */
    int poll_mask = Wimp_Poll_NullMask;
    BOOL other_clients_want_nulls = FALSE;

    /* enable null events to ourselves if needed here */
    if(peek || other_clients_want_nulls)
        poll_mask &= ~Wimp_Poll_NullMask;

    wimpt_poll_coltsoft(poll_mask, &event_data, &event_code); /* loops itself till a good event is read */

    if(event_code != Wimp_ENull)
        if(winx_menu_process(event_code, &event_data))
            return(WM_EVENT_PROCESSED); /* can't think of 'owt better a ce moment */

    /* res == 0 only if we got a peek at a null event and we wanted it! */
    res = ((event_code != Wimp_ENull) || !peek);

    /* if we have a real event, process it and look again */

    /* always allow other clients to have a look in at nulls if they want */
    if((event_code != Wimp_ENull)  ||  other_clients_want_nulls)
    {
        consume_bool(event_do_process(event_code, &event_data));
    }
    } /*block*/

    if(res)
    {
        last_event_was_null = FALSE;

        return(WM_EVENT_PROCESSED);
    }

    /* only allow our processors a look in now at nulls they wanted */

    last_event_was_null = TRUE;

    /* perform the initial idle time test if some idle time has come */
    if(bgNullTestWanted)
    {
        bgNullEventsWanted = (null_events_do_query() == NULL_EVENTS_REQUIRED);
        trace_1(TRACE_APP_WM_EVENT, TEXT("wm_event_get: bgNullTestWanted: query handlers returns NULL_EVENTS_%s"), bgNullEventsWanted ? "REQUIRED" : "OFF");
    }

    /* processor is idle, dispatch scheduled & null events appropriately */

    if(bgScheduledEventsWanted)
        (void) scheduled_events_do_events();

    if(bgNullEventsWanted)
        (void) null_events_do_events();

    if(fgNullEventsWanted)
    {
        /* we have our desired fg idle, so return */
        trace_0(TRACE_APP_WM_EVENT, TEXT("wm_event_get: return WM_EVENT_FG_NULL"));
        return(WM_EVENT_FG_NULL);
    }

#if 1
    return(WM_EVENT_PROCESSED);
#else
    /* check bg null requirements to loop again and again */
    if(bgNullEventsWanted || bgScheduledEventsWanted)
        return(WM_EVENT_PROCESSED);

    /* exiting strangely */
    trace_0(TRACE_APP_WM_EVENT, TEXT("wm_event_get: return WM_EVENT_EXIT"));
    return(WM_EVENT_EXIT);
#endif
}

static wimp_i iconbar_handle;

static STATUS thesaurus_loaded_state;

/* allow a single message filter to be installed (used for draft printing) */

static P_PROC_RISCOS_MESSAGE_FILTER g_p_proc_message_filter = NULL;
static CLIENT_HANDLE g_message_filter_client_handle = 0;

extern void
host_message_filter_register(
    _In_        P_PROC_RISCOS_MESSAGE_FILTER p_proc_message_filter,
    CLIENT_HANDLE client_handle)
{
    g_p_proc_message_filter = p_proc_message_filter;
    g_message_filter_client_handle = client_handle;
}

extern void
host_message_filter_add(
    const int * const wimp_messages)
{
    _kernel_swi_regs rs;
    _kernel_oserror * e;

    rs.r[0] = (int) wimp_messages;
    e = _kernel_swi(0x400F6 /*Wimp_AddMessages*/, &rs, &rs);
}

extern void
host_message_filter_remove(
    const int * const wimp_messages)
{
    _kernel_swi_regs rs;
    _kernel_oserror * e;

    rs.r[0] = (int) wimp_messages;
    e = _kernel_swi(0x400F7 /*Wimp_RemoveMessages*/, &rs, &rs);
}

static void
general_message_PreQuit(
    _In_        const WimpMessage * const p_wimp_message)
{
    WimpMessage msg = *p_wimp_message;
    const WimpPreQuitMessage * const p_wimp_message_prequit = (const WimpPreQuitMessage *) &msg.data;
    S32 count;
    int kill_this_task;

    kill_this_task = p_wimp_message_prequit->flags & Wimp_MPreQuit_flags_killthistask;

    count = docs_modified();

    /* if any modified, it gets hard */
    if(count)
    {
        /* First, acknowledge the message to stop it going any further */
        trace_0(TRACE_RISCOS_HOST, TEXT("acknowledging PREQUIT message: "));
        msg.hdr.your_ref = p_wimp_message->hdr.my_ref;
        void_WrapOsErrorReporting(wimp_send_message(Wimp_EUserMessageAcknowledge, &msg, p_wimp_message->hdr.sender, BAD_WIMP_I, NULL));

        /* And then tell the user. */
        if(status_ok(query_quit(P_DOCU_NONE, count)))
        {
            WimpKeyPressedEvent key_pressed;

            if(kill_this_task)
                exit(EXIT_SUCCESS);

            /* Start up the closedown sequence again if OK and all tasks are to die. (not RISC OS 2) */
            /* We assume that the sender is the Task Manager,
             * so that Ctrl+Shift+F12 is the closedown key sequence.
            */
            void_WrapOsErrorReporting(wimp_get_caret_position(&key_pressed.caret));
            key_pressed.key_code = 16 /*Sh*/ + 32 /*Ctl*/ + 0x1CC /*F12*/;
            void_WrapOsErrorReporting(wimp_send_message(Wimp_EKeyPressed, &key_pressed, p_wimp_message->hdr.sender, BAD_WIMP_I, NULL));
        }
    }
}

static _kernel_oserror *
_kernel_fwrite0(
    _In_        int file_handle,
    _In_z_      PC_U8Z p_u8)
{
    U8 ch;

    while((ch = *p_u8++) != CH_NULL)
        if(_kernel_ERROR == _kernel_osbput(ch, file_handle))
            return(_kernel_last_oserror());

    return(NULL);
}

static void
general_message_SaveDesktop(
    _In_        const WimpMessage * const p_wimp_message)
{
    const int file_handle = p_wimp_message->data.save_desktop.file_handle;
    _kernel_oserror * e = NULL;
    U8Z var_name[BUF_MAX_PATHSTRING];
    TCHARZ main_app[BUF_MAX_PATHSTRING];
    PTSTR p_main_app = main_app;
    TCHARZ user_path[BUF_MAX_PATHSTRING];
    PTSTR p_user_path = user_path;

    if(NULL != _kernel_getenv(make_var_name(var_name, "$Dir"), main_app, elemof32(main_app)))
        main_app[0] = CH_NULL;

    if(NULL != _kernel_getenv(make_var_name(var_name, "$UserPath"), user_path, elemof32(user_path)))
        user_path[0] = CH_NULL;

    /* Ignore PRM guideline about caching vars at startup; final seen one is most relevant */

    /* write out commands to desktop save file that will restart application */
    if(0 != strlen(p_user_path))
    {
        /* need to boot main app first */
        if(NULL == (e = _kernel_fwrite0(file_handle, "Filer_Boot ")))
        if(NULL == (e = _kernel_fwrite0(file_handle, p_main_app)))
            e = _kernel_fwrite0(file_handle, "\x0A");

        p_main_app = p_user_path;

        p_user_path += strlen(p_user_path);
        assert(p_user_path[-1] == CH_COMMA);
        assert(p_user_path[-2] == CH_FULL_STOP);
        p_user_path[-2] = CH_NULL;
    }

    if(NULL == e)
    {
        /* now run app */
        if(NULL == (e = _kernel_fwrite0(file_handle, "Run ")))
        if(NULL == (e = _kernel_fwrite0(file_handle, p_main_app)))
            e = _kernel_fwrite0(file_handle, "\x0A");
    }

    if(NULL != e)
    {
        void_WrapOsErrorReporting(e);

        { /* ack the message to stop desktop save and remove file */
        WimpMessage msg = *p_wimp_message;
        trace_0(TRACE_RISCOS_HOST, TEXT("acknowledging SaveDesktop message: "));
        msg.hdr.your_ref = p_wimp_message->hdr.my_ref;
        void_WrapOsErrorReporting(wimp_send_message(Wimp_EUserMessageAcknowledge, &msg, p_wimp_message->hdr.sender, BAD_WIMP_I, NULL));
        } /*block*/
    }
}

static void
general_message_Shutdown(
    _In_        const WimpMessage * const p_wimp_message)
{
    IGNOREPARM_InRef_(p_wimp_message);

    g_silent_shutdown = TRUE; /* suppress error reporting for uninterrupted processing */

    status_consume(save_all_modified_docs_for_shutdown());

    exit(EXIT_SUCCESS);
}

/* allow this module to kill off duplicate instances when they start up */

extern BOOL g_kill_duplicates = FALSE;

static void
general_message_TaskInitialise(
    _In_        const WimpMessage * const p_wimp_message)
{
    if(g_kill_duplicates)
    {
        const WimpTaskInitMessage * const p_wimp_message_task_init = (const WimpTaskInitMessage *) &p_wimp_message->data;
        const char * taskname = p_wimp_message_task_init->taskname;
        trace_1(TRACE_RISCOS_HOST, TEXT("Wimp_MTaskInitialise for %s"), taskname);
        if(0 == /*"C"*/strcmp(product_ui_id(), taskname))
        {
            static int seen_my_birth = FALSE;
            if(seen_my_birth)
            {
                WimpMessage msg;
                trace_1(TRACE_RISCOS_HOST, TEXT("Another %s is trying to start! I'll kill it"), taskname);
                msg.hdr.size = sizeof32(msg.hdr);
                msg.hdr.your_ref = 0; /* fresh msg */
                msg.hdr.action_code = Wimp_MQuit;
                void_WrapOsErrorReporting(wimp_send_message(Wimp_EUserMessage, &msg, p_wimp_message->hdr.sender, BAD_WIMP_I, NULL));
            }
            else
            {
                trace_0(TRACE_RISCOS_HOST, TEXT("witnessing our own birth"));
                seen_my_birth = TRUE;
            }
        }
    }
}

_Check_return_
static inline BOOL
claim_broadcast_t5_file(
    _InVal_     T5_FILETYPE t5_filetype)
{
    switch(g_product_id)
    {
    case PRODUCT_ID_FPROWORKZ:
        return(TRUE); /* accept all of the t5 filetypes */

    case PRODUCT_ID_FIREWORKZ:
        if(FILETYPE_T5_RECORDZ == t5_filetype)
            return(FALSE); /* don't claim Recordz on broadcast (can still drag to icon) */

        return(TRUE); /* accept all of the other t5 filetypes */

    default:
        return(FALSE); /* accept none of the t5 filetypes */
    }
}

_Check_return_
static inline T5_FILETYPE
claim_broadcast_foreign_file_as(
    _InVal_     T5_FILETYPE t5_filetype,
    _In_z_      PCTSTR filename)
{
    /* accept any foreign filetype if it is currently set up to use our app */
    U8Z var_name[BUF_MAX_PATHSTRING];
    TCHARZ var_value[BUF_MAX_PATHSTRING];
    BOOL claim = FALSE;

    consume_int(xsnprintf(var_name, elemof32(var_name), "Alias$@RunType_%.3X", t5_filetype));

    if(NULL != _kernel_getenv(var_name, var_value, elemof32(var_value)))
        return(FILETYPE_UNDETERMINED);

    if(NULL != strstr(var_value, "!Fireworkz.!Run"))
        claim = TRUE;

    reportf("%s : %s - claim=%s", var_name, var_value, report_boolstring(claim));

    if(claim)
        return(t5_filetype);

    switch(t5_filetype)
    {
    case FILETYPE_TEXT:
    case FILETYPE_DOS:
    case FILETYPE_DATA:
    case FILETYPE_UNTYPED:
        {
        T5_FILETYPE t_t5_filetype = t5_filetype_from_extension(file_extension(filename)); /* thing/ext? */

        if(FILETYPE_UNDETERMINED == t_t5_filetype)
            return(FILETYPE_UNDETERMINED);

        return(claim_broadcast_foreign_file_as(t_t5_filetype, filename));
        }

    default:
        return(FILETYPE_UNDETERMINED);
    }
}

static BOOL
g_host_xfer_loading_via_scrap_file = FALSE;

static
struct HOST_XFER_LOAD_STATICS
{
    U8Z scrap_name_buffer[32]; /* needed to delete it on done() */

    /* obtained from the Window Manager */
    int scrap_my_ref;
}
host_xfer_load_statics;

static
struct HOST_XFER_SAVE_STATICS
{
    /* provided by the caller */
    P_PROC_HOST_XFER_SAVE p_proc_host_xfer_save;
    CLIENT_HANDLE client_handle;

    /* obtained from the Window Manager */
    int my_ref;
}
host_xfer_save_statics;

static void
general_message_DataSaveAck(
    _In_        const WimpMessage * const p_wimp_message)
{
    WimpMessage msg = *p_wimp_message; /* take a copy so that client proc can access filename safely */

    if(msg.hdr.your_ref == host_xfer_save_statics.my_ref)
    {
        host_xfer_set_saved_file_is_safe(msg.data.data_save_ack.estimated_size >= 0); /* -1 => Scrap file transfer */

        if((* host_xfer_save_statics.p_proc_host_xfer_save) (msg.data.data_save_ack.leaf_name, host_xfer_save_statics.client_handle))
        {
            /* saved OK - turn message around into a DataLoad */
            msg.hdr.your_ref = msg.hdr.my_ref;
            msg.hdr.action_code = Wimp_MDataLoad;

            void_WrapOsErrorReporting(wimp_send_message(Wimp_EUserMessageRecorded, &msg, p_wimp_message->hdr.sender, BAD_WIMP_I, NULL));
        }
        /* else error - should have been reported */
    }
}

static void
general_message_DataOpen(
    _In_        const WimpMessage * const p_wimp_message)
{
    /* File double-clicked in dirviewer */
    PCTSTR filename = p_wimp_message->data.data_load.leaf_name;
    T5_FILETYPE t5_filetype = (T5_FILETYPE) p_wimp_message->data.data_load.file_type;

    switch(t5_filetype)
    {
    case FILETYPE_T5_FIREWORKZ:
    case FILETYPE_T5_WORDZ:
    case FILETYPE_T5_RESULTZ:
    case FILETYPE_T5_RECORDZ:
    case FILETYPE_T5_TEMPLATE:
    case FILETYPE_T5_COMMAND:
        if(!claim_broadcast_t5_file(t5_filetype))
            return;

        break;

    default:
        t5_filetype = claim_broadcast_foreign_file_as(t5_filetype, filename);

        if(FILETYPE_UNDETERMINED == t5_filetype)
            return;

        break;
    }

    /* need host_xfer_load_file_setup(m) at top of recognized cases to
     * stop null events in load allowing unacked message bounce thereby
     * allowing Filer to try to invoke another copy of application
     * to load this file ... sadly prevents scrap file execution!
    */
    host_xfer_load_file_setup(p_wimp_message);

    switch(t5_filetype)
    {
    case FILETYPE_T5_FIREWORKZ:
    case FILETYPE_T5_WORDZ:
    case FILETYPE_T5_RESULTZ:
    case FILETYPE_T5_RECORDZ:
        (void) load_this_file(P_DOCU_NONE, T5_CMD_LOAD, filename);
        break;

    case FILETYPE_T5_TEMPLATE:
        (void) load_this_file(P_DOCU_NONE, T5_CMD_LOAD_TEMPLATE, filename);
        break;

    case FILETYPE_T5_COMMAND:
        (void) load_this_file(P_DOCU_NONE, T5_CMD_EXECUTE, filename);
        break;

    default:
        (void) load_foreign_file(P_DOCU_NONE, t5_filetype, filename);
        break;
    }

    host_xfer_load_file_done();
}

static void
general_message_PrintTypeOdd(
    _In_        const WimpMessage * const p_wimp_message)
{
    /* printer broadcast to find someone to print this odd filetype */
    PCTSTR filename = p_wimp_message->data.data_load.leaf_name;
    const T5_FILETYPE t5_filetype = (T5_FILETYPE) p_wimp_message->data.data_load.file_type;

    switch(t5_filetype)
    {
    case FILETYPE_T5_FIREWORKZ:
    case FILETYPE_T5_WORDZ:
    case FILETYPE_T5_RESULTZ:
    case FILETYPE_T5_RECORDZ:
    case FILETYPE_T5_TEMPLATE:
        if(claim_broadcast_t5_file(t5_filetype))
        {
            host_xfer_print_file_done(p_wimp_message, -1); /* print 'all done' */
            status_consume(load_and_print_this_file(filename, NULL));
        }

        break;

    default:
        break;
    }
}

static void
general_message_PaletteChange(void)
{
    /* recache palette-dependent variables */
    host_invalidate_cache(HIC_PURGE);
    host_palette_cache_reset();/* which does host_rgb_stash(); */
}

static void
general_message_ModeChange(void)
{
    /* recache mode-dependent variables */
    host_invalidate_cache(HIC_PURGE);
    host_modevar_cache_reset();
    host_palette_cache_reset(); /* which does host_rgb_stash(); */
    host_recache_mode_dependent_vars();
}

#define Message_ThesaurusStarting 0x821C0
#define Message_ThesaurusDying    0x821C1
#define Message_ThesaurusQuery    0x821C2
#define Message_ThesaurusReceive  0x821C3
#define Message_ThesaurusSend     0x821C4

_Check_return_
static BOOL
general_message_ThesaurusSend_for_docu(
    _DocuRef_   P_DOCU p_docu,
    _In_        const WimpMessage * const p_wimp_message)
{
    const PC_SBSTR sbstr_replace_string = p_wimp_message->data.bytes;
    OBJECT_STRING_REPLACE object_string_replace;
    SCAN_BLOCK scan_block;
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 100);
    quick_ublock_with_buffer_setup(quick_ublock);

    if((OBJECT_ID_CELLS != p_docu->focus_owner) && (OBJECT_ID_REC_FLOW != p_docu->focus_owner))
        return(FALSE);

    if(!p_docu->mark_info_cells.h_markers)
        return(FALSE);

    if( !status_done(cells_scan_init(p_docu, &scan_block, SCAN_DOWN, SCAN_MARKERS, NULL, OBJECT_ID_NONE)) ||
        !status_done(cells_scan_next(p_docu, &object_string_replace.object_data, &scan_block)) )
        return(FALSE);

    if(OBJECT_ID_NONE == object_string_replace.object_data.object_id)
        return(FALSE);

    status_assert(quick_ublock_sbstr_add(&quick_ublock, sbstr_replace_string));

    object_string_replace.p_quick_ublock = &quick_ublock;
    object_string_replace.copy_capitals = 1;

    status_assert(maeve_event(p_docu, T5_MSG_SELECTION_CLEAR, P_DATA_NONE));

    docu_modify(p_docu);

    status_assert(object_call_id(object_string_replace.object_data.object_id,
                                 p_docu,
                                 T5_MSG_OBJECT_STRING_REPLACE,
                                 &object_string_replace));

    quick_ublock_dispose(&quick_ublock);

    {
    WimpMessage msg = *p_wimp_message;
    trace_0(TRACE_RISCOS_HOST, TEXT("acknowledging Message_ThesaurusSend: "));
    msg.hdr.your_ref = p_wimp_message->hdr.my_ref;
    void_WrapOsErrorReporting(wimp_send_message(Wimp_EUserMessageAcknowledge, &msg, p_wimp_message->hdr.sender, BAD_WIMP_I, NULL));
    } /*block*/

    return(TRUE);
}

_Check_return_
static BOOL
general_message_ThesaurusSend(
    _In_        const WimpMessage * const p_wimp_message)
{
    DOCNO docno = DOCNO_NONE;

    while(DOCNO_NONE != (docno = docno_enum_docs(docno)))
    {
        const P_DOCU p_docu = p_docu_from_docno(docno);

        if(p_docu->flags.is_current && p_docu->flags.has_input_focus)
            return(general_message_ThesaurusSend_for_docu(p_docu, p_wimp_message));
    }

    return(FALSE);
}

static BOOL
general_message(
    _In_        const WimpMessage * const p_wimp_message)
{
    switch(p_wimp_message->hdr.action_code)
    {
    case Wimp_MQuit:
        exit(EXIT_SUCCESS);
        break;

    case Wimp_MPreQuit:
        general_message_PreQuit(p_wimp_message);
        break;

    case Wimp_MSaveDesktop:
        general_message_SaveDesktop(p_wimp_message);
        break;

    case Wimp_MTaskInitialise:
        general_message_TaskInitialise(p_wimp_message);
        break;

    case Wimp_MShutDown:
        general_message_Shutdown(p_wimp_message);
        break;

    case Wimp_MDataSaveAck:
        general_message_DataSaveAck(p_wimp_message);
        break;

    case Wimp_MDataOpen:
        general_message_DataOpen(p_wimp_message);
        break;

    case Wimp_MPrintTypeOdd:
        general_message_PrintTypeOdd(p_wimp_message);
        break;

    case Wimp_MPaletteChange:
        general_message_PaletteChange();
        break;

    case Wimp_MModeChange:
        general_message_ModeChange();
        break;

    case Message_ThesaurusStarting:
        thesaurus_loaded_state = 1;
        break;

    case Message_ThesaurusDying:
        thesaurus_loaded_state = 0;
        break;

    case Message_ThesaurusSend:
        return(general_message_ThesaurusSend(p_wimp_message));

    default:
        return(FALSE);
    }

    return(TRUE);
}

static BOOL
general_message_bounced(
    _In_        const WimpMessage * const p_wimp_message)
{
    switch(p_wimp_message->hdr.action_code)
    {
    default: default_unhandled(); return(FALSE);

    case Message_ThesaurusQuery:
        thesaurus_loaded_state = 0;
        break;
    }

    return(TRUE);
}

static BOOL
general_event_handler(
    _InVal_     int event_code,
    _In_        const WimpPollBlock * const p_event_data,
    P_ANY handle)
{
    IGNOREPARM(handle);

    trace_2(TRACE_RISCOS_HOST, TEXT("%s: %s"), __Tfunc__, report_wimp_event(event_code, p_event_data));

    switch(event_code)
    {
    case Wimp_EUserMessage:
    case Wimp_EUserMessageRecorded:
        if(NULL != g_p_proc_message_filter)
            if((* g_p_proc_message_filter) (event_code, p_event_data, g_message_filter_client_handle))
                return(TRUE /*processed*/);

        return(general_message(&p_event_data->user_message));

    case Wimp_EUserMessageAcknowledge:
        if(NULL != g_p_proc_message_filter)
            if((* g_p_proc_message_filter) (event_code, p_event_data, g_message_filter_client_handle))
                return(TRUE /*processed*/);

        return(general_message_bounced(&p_event_data->user_message_acknowledge));

    default:
        break;
    }

    return(FALSE /*unprocessed*/);
}

/******************************************************************************
*
* Icon bar event handler
*
* Receives click and load events from the icon bar (ie from document_template icons)
*
******************************************************************************/

static void
iconbar_event_mouse_click_Select(void)
{
    if(host_ctrl_pressed())
    { /*EMPTY*/ } /* reserved */
    else if(host_shift_pressed())
    {
        TCHARZ filename[BUF_MAX_PATHSTRING];

        /* open the user's Choices directory viewer */
        tstr_xstrkpy(filename, elemof32(filename), TEXT("<Choices$Write>") FILE_DIR_SEP_TSTR TEXT("Fireworkz") FILE_DIR_SEP_TSTR);
        tstr_xstrkat(filename, elemof32(filename), TEXT("X"));

        filer_opendir(filename);
    }
    else
        (void) load_this_file(P_DOCU_NONE, T5_CMD_LOAD_TEMPLATE, NULL);
}

static void
iconbar_event_mouse_click_Adjust(void)
{
    if(host_ctrl_pressed())
    { /*EMPTY*/ } /* reserved */
    else if(host_shift_pressed())
    { /*EMPTY*/ } /* reserved */
    else
    {
        of_load_prepare_first_template();

        (void) load_this_file(P_DOCU_NONE, T5_CMD_LOAD_TEMPLATE, NULL);
    }
}

static BOOL
iconbar_mouse_click(
    _In_        const WimpMouseClickEvent * const p_mouse_click)
{
    if(p_mouse_click->buttons == Wimp_MouseButtonSingleSelect)
        iconbar_event_mouse_click_Select();
    else if(p_mouse_click->buttons == Wimp_MouseButtonSingleAdjust)
        iconbar_event_mouse_click_Adjust();

    /* done something, so... */
    return(TRUE);
}

static void
iconbar_message_DataSave(
    const WimpMessage * const p_wimp_message /*DataSave*/)
{
    /* File dragged from another application, dropped on our icon */
    const T5_FILETYPE t5_filetype = (T5_FILETYPE) p_wimp_message->data.data_save.file_type;

    switch(t5_filetype)
    {
    case FILETYPE_DIRECTORY:
    case FILETYPE_APPLICATION:
        reperr(FILE_ERR_ISADIR, p_wimp_message->data.data_save.leaf_name);
        break;

    default:
        /* can be complicated ... see PipeDream */
        if_constant(p_command_recorder_of_op_format)
        {
            reperr_null(ERR_NOTWHILSTRECORDING);
            break;
        }

        host_xfer_import_file_via_scrap(p_wimp_message);
        break;
    }
}

static void
iconbar_message_DataLoad(
    const WimpMessage * const p_wimp_message /*DataLoad*/)
{
    /* File dragged from file viewer, dropped on our icon */
    TCHARZ filename[256];
    T5_FILETYPE t5_filetype = (T5_FILETYPE) p_wimp_message->data.data_load.file_type;
    T5_MESSAGE t5_message = T5_CMD_LOAD; /* keep dataflower happy */

    tstr_xstrkpy(filename, elemof32(filename), p_wimp_message->data.data_load.leaf_name); /* low-lifetime name */

    host_xfer_load_file_setup(p_wimp_message);

    switch(t5_filetype)
    {
    case FILETYPE_DIRECTORY:
    case FILETYPE_APPLICATION:
    case FILETYPE_T5_FIREWORKZ:
    case FILETYPE_T5_WORDZ:
    case FILETYPE_T5_RESULTZ:
    case FILETYPE_T5_RECORDZ:
    case FILETYPE_T5_TEMPLATE:
    case FILETYPE_T5_COMMAND:
        break;

    case FILETYPE_TEXT:
        { /* SKS 10dec94 allow fred/fwk files of type Text (eg unmapped on NFS) to be detected - but does not scan these for recognisable headers */
        T5_FILETYPE t_t5_filetype = t5_filetype_from_extension(file_extension(filename));

        if(FILETYPE_UNDETERMINED != t_t5_filetype)
            t5_filetype = t_t5_filetype;

        break;
        }

    case FILETYPE_DOS:
    case FILETYPE_DATA:
    case FILETYPE_UNTYPED:
        {
        T5_FILETYPE t_t5_filetype = t5_filetype_from_extension(file_extension(filename)); /* thing/ext? */

        if(FILETYPE_UNDETERMINED == t_t5_filetype)
            t_t5_filetype = t5_filetype_from_file_header(filename); /* no, so scan for recognisable headers */

        if(FILETYPE_UNDETERMINED != t_t5_filetype)
            t5_filetype = t_t5_filetype;

        break;
        }

    default:
        break;
    }

    switch(t5_filetype)
    {
    case FILETYPE_DIRECTORY:
    case FILETYPE_APPLICATION:
    case FILETYPE_T5_FIREWORKZ:
    case FILETYPE_T5_WORDZ:
    case FILETYPE_T5_RESULTZ:
    case FILETYPE_T5_RECORDZ:
        t5_message = T5_CMD_LOAD;
        break;

    case FILETYPE_T5_TEMPLATE:
        t5_message = T5_CMD_LOAD_TEMPLATE;
        break;

    case FILETYPE_T5_COMMAND:
        t5_message = T5_CMD_EXECUTE;
        break;

    default:
        t5_message = T5_CMD_LOAD_FOREIGN;
        break;
    }

    switch(t5_filetype)
    {
    case FILETYPE_DIRECTORY:
    case FILETYPE_APPLICATION:
        (void) load_this_dir(filename);
        break;

    default:
        if(T5_CMD_LOAD_FOREIGN == t5_message)
            (void) load_foreign_file(P_DOCU_NONE, t5_filetype, filename);
        else
            (void) load_this_file(P_DOCU_NONE, t5_message, filename);
        break;
    }

    host_xfer_load_file_done();
}

static void
iconbar_message_HelpRequest(
    const WimpMessage * const p_wimp_message /*HelpRequest*/)
{
    WimpMessage msg = *p_wimp_message;
    PCTSTR format = resource_lookup_tstr(MSG_HELP_ICONBAR);
    QUICK_TBLOCK quick_tblock;
    _quick_tblock_setup(&quick_tblock, msg.data.bytes, sizeof32(msg.data.bytes));

    if(status_ok(quick_tblock_printf(&quick_tblock, format, product_id())) && status_ok(quick_tblock_nullch_add(&quick_tblock)))
    {
        msg.hdr.size  = sizeof32(msg.hdr);
        msg.hdr.size += (int) strlen32p1(msg.data.bytes); /*CH_NULL*/
        msg.hdr.size  = (msg.hdr.size + (4-1)) & ~(4-1);

        msg.hdr.your_ref = p_wimp_message->hdr.my_ref;
        msg.hdr.action_code = Wimp_MHelpReply;

        trace_2(0, TEXT("helpreply is ") S32_TFMT TEXT(" long, %s"), msg.hdr.size, quick_tblock_tstr(&quick_tblock));

        void_WrapOsErrorReporting(wimp_send_message(Wimp_EUserMessage, &msg, p_wimp_message->hdr.sender, BAD_WIMP_I, NULL));
    }

    quick_tblock_dispose(&quick_tblock);
}

static BOOL
iconbar_message(
    _In_        const WimpMessage * const p_wimp_message)
{
    switch(p_wimp_message->hdr.action_code)
    {
    case Wimp_MDataSave:
        iconbar_message_DataSave(p_wimp_message);
        /* done something, so... */
        return(TRUE);

    case Wimp_MDataLoad:
        iconbar_message_DataLoad(p_wimp_message);
        /* done something, so... */
        return(TRUE);

    case Wimp_MHelpRequest:
        iconbar_message_HelpRequest(p_wimp_message);
        /* done something, so... */
        return(TRUE);

    default:
        break;
    }

    return(FALSE /*unprocessed*/);
}

static BOOL
iconbar_event_handler(
    _InVal_     int event_code,
    _In_        const WimpPollBlock * const p_event_data,
    P_ANY handle)
{
    IGNOREPARM(handle);

    trace_2(TRACE_RISCOS_HOST, TEXT("%s: %s"), __Tfunc__, report_wimp_event(event_code, p_event_data));

    switch(event_code)
    {
    case Wimp_EMouseClick:
        return(iconbar_mouse_click(&p_event_data->mouse_click));

    case Wimp_EUserMessage:
    case Wimp_EUserMessageRecorded:
        return(iconbar_message(&p_event_data->user_message));

    default:
        break;
    }

    return(FALSE /*unprocessed*/);
}

/******************************************************************************
*
* Set up file loading from a DataSave message
*
* Always goes via a Scrap file
*
******************************************************************************/

#define SCRAP_NAME_LEAF product_family_id()

extern void
host_xfer_import_file_via_scrap(
    _In_        const WimpMessage * const p_wimp_message /*DataSave*/)
{
    WimpMessage msg = *p_wimp_message;
    char buffer[16];
    _kernel_swi_regs rs;

    /* first check that the preferred variable exists */
    xstrkpy(host_xfer_load_statics.scrap_name_buffer, elemof32(host_xfer_load_statics.scrap_name_buffer), "<" "Wimp$ScrapDir");

    rs.r[0] = (int) host_xfer_load_statics.scrap_name_buffer + 1;
    rs.r[1] = (int) buffer;
    rs.r[2] = sizeof32(buffer);
    rs.r[3] = 0;
    rs.r[4] = 3;
    void_WrapOsErrorChecking(_kernel_swi(OS_ReadVarVal, &rs, &rs));

    if(rs.r[2] == 0)
    {
        /* nope, so check that the older variable exists */
        xstrkpy(host_xfer_load_statics.scrap_name_buffer, elemof32(host_xfer_load_statics.scrap_name_buffer), "<" "Wimp$Scrap");

        rs.r[0] = (int) host_xfer_load_statics.scrap_name_buffer + 1;
        rs.r[1] = (int) buffer;
        rs.r[2] = sizeof32(buffer);
        rs.r[3] = 0;
        rs.r[4] = 3;
        void_WrapOsErrorChecking(_kernel_swi(OS_ReadVarVal, &rs, &rs));

        if(rs.r[2] == 0)
        {
            reperr_null(ERR_RISCOS_NO_SCRAP);
            return;
        }

        xstrkat(host_xfer_load_statics.scrap_name_buffer, elemof32(host_xfer_load_statics.scrap_name_buffer), ">");
    }
    else
    {
        xstrkat(host_xfer_load_statics.scrap_name_buffer, elemof32(host_xfer_load_statics.scrap_name_buffer), ">.");
        xstrkat(host_xfer_load_statics.scrap_name_buffer, elemof32(host_xfer_load_statics.scrap_name_buffer), SCRAP_NAME_LEAF);
    }

    msg.hdr.size = sizeof32(msg.hdr) + sizeof32(WimpDataSaveAckMessage);
    msg.hdr.your_ref = p_wimp_message->hdr.my_ref;
    msg.hdr.action_code = Wimp_MDataSaveAck;

    xstrkpy(msg.data.data_save_ack.leaf_name, elemof32(msg.data.data_save_ack.leaf_name), host_xfer_load_statics.scrap_name_buffer);

    msg.data.data_save_ack.estimated_size = -1; /* any file sent to us will NOT be safe with us */

    void_WrapOsErrorReporting(wimp_send_message(Wimp_EUserMessage, &msg, p_wimp_message->hdr.sender, BAD_WIMP_I, NULL));

    host_xfer_load_statics.scrap_my_ref = msg.hdr.my_ref;
}

/******************************************************************************
*
* Set up file loading from a DataLoad/DataOpen message
*
* Sends an acknowledge back to the original application
*
******************************************************************************/

extern void
host_xfer_load_file_setup(
    _In_        const WimpMessage * const p_wimp_message /*DataLoad/DataOpen*/)
{
    WimpMessage msg = *p_wimp_message;

    msg.hdr.size = sizeof32(msg.hdr);
    msg.hdr.your_ref = p_wimp_message->hdr.my_ref;
    msg.hdr.action_code = Wimp_MDataLoadAck;

    /* Is this DataLoad/DataOpen coming from our host_xfer_import_file_via_scrap()? */
    g_host_xfer_loading_via_scrap_file =
        (host_xfer_load_statics.scrap_my_ref != 0) &&
        (host_xfer_load_statics.scrap_my_ref == p_wimp_message->hdr.your_ref);

    host_xfer_load_statics.scrap_my_ref = 0;

    void_WrapOsErrorReporting(wimp_send_message(Wimp_EUserMessage, &msg, p_wimp_message->hdr.sender, BAD_WIMP_I, NULL));
}

/******************************************************************************
*
* Delete any Scrap file used in file loading
*
******************************************************************************/

extern void
host_xfer_load_file_done(void)
{
    if(g_host_xfer_loading_via_scrap_file)
    {
        _kernel_swi_regs rs;

        g_host_xfer_loading_via_scrap_file = FALSE;

        rs.r[0] = OSFile_Delete; /* doesn't complain if not there */
        rs.r[1] = (int) host_xfer_load_statics.scrap_name_buffer;
        void_WrapOsErrorChecking(_kernel_swi(OS_File, &rs, &rs));
    }
}

_Check_return_
extern BOOL
host_xfer_loaded_file_is_safe(void)
{
    return(!g_host_xfer_loading_via_scrap_file);
}

extern void
host_xfer_print_file_done(
    _In_        const WimpMessage * const p_wimp_message,
    int type)
{
    WimpMessage msg = *p_wimp_message;

    msg.hdr.size = sizeof32(WimpMessage);
    msg.hdr.your_ref = p_wimp_message->hdr.my_ref;
    msg.hdr.action_code = Wimp_MPrintTypeKnown;

    msg.data.data_save.file_type = type;

    void_WrapOsErrorReporting(wimp_send_message(Wimp_EUserMessage, &msg, p_wimp_message->hdr.sender, BAD_WIMP_I, NULL));
}

_Check_return_
extern BOOL
host_xfer_save_file(
    _In_z_      PCTSTR filename,
    _InVal_     T5_FILETYPE t5_filetype,
    int estimated_size,
    P_PROC_HOST_XFER_SAVE p_proc_host_xfer_save,
    CLIENT_HANDLE client_handle,
    _In_        const WimpGetPointerInfoBlock * const p_pointer_info)
{
    WimpMessage msg;

    host_xfer_save_statics.p_proc_host_xfer_save = p_proc_host_xfer_save;
    host_xfer_save_statics.client_handle = client_handle;

    msg.hdr.size = sizeof32(WimpDataSaveMessage);
    msg.hdr.sender = p_pointer_info->window_handle;
    msg.hdr.my_ref = 0; /* will be filled in by the Window Manager */
    msg.hdr.your_ref = 0;
    msg.hdr.action_code = Wimp_MDataSave;

    msg.data.data_save.destination_window = p_pointer_info->window_handle;
    msg.data.data_save.destination_icon = p_pointer_info->icon_handle;
    msg.data.data_save.destination_x = p_pointer_info->x;
    msg.data.data_save.destination_y = p_pointer_info->y;
    msg.data.data_save.estimated_size = estimated_size; /* in Fireworkz this is never known so 42 is supplied */
    msg.data.data_save.file_type = (int) t5_filetype;

    xstrkpy(msg.data.data_save.leaf_name, elemof32(msg.data.data_save.leaf_name), file_leafname(filename));

    void_WrapOsErrorReporting(wimp_send_message(Wimp_EUserMessage, &msg, p_pointer_info->window_handle, p_pointer_info->icon_handle, NULL));

    host_xfer_save_statics.my_ref = msg.hdr.my_ref; /* note the my_ref of this message which has been filled in by the Window Manager for protocol validation */

    return(TRUE);
}

static BOOL
g_host_xfer_saved_file_is_safe = TRUE;

_Check_return_
extern BOOL
host_xfer_saved_file_is_safe(void)
{
    return(g_host_xfer_saved_file_is_safe);
}

extern void
host_xfer_set_saved_file_is_safe(
    _InVal_     BOOL value)
{
    g_host_xfer_saved_file_is_safe = value;
}

/*
exported services hook
*/

MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_ho_event);

_Check_return_
static STATUS
ho_event_msg_startup(void)
{
    status_assert(winx_register_general_event_handler(general_event_handler, NULL));

    { /* Always place an icon (from Window Manager Sprite Pool) on the icon bar;
       * it needs an event handler, plus a menu and the menu's event handler */
    U8Z buffer[16];
    WimpCreateIconBlockWithBitset icreate;
    zero_struct(icreate);
    icreate.window_handle = (wimp_w) (-1) /* icon bar, right hand side */;
    icreate.icon.flags.bits.sprite      = 1;
    icreate.icon.flags.bits.horz_centre = 1;
    icreate.icon.flags.bits.button_type = ButtonType_Click;

    xstrkpy(buffer, elemof32(buffer), g_product_sprite_name);

    {
    RESOURCE_BITMAP_ID resource_bitmap_id;
    RESOURCE_BITMAP_HANDLE resource_bitmap_handle;
    GDI_SIZE size;
    resource_bitmap_id.object_id = OBJECT_ID_SKEL;
    resource_bitmap_id.bitmap_name = buffer;
    resource_bitmap_handle = resource_bitmap_find_system(&resource_bitmap_id);
    resource_bitmap_gdi_size_query(resource_bitmap_handle, &size);
    /*icreate.icon.bbox.xmin = 0;*/
    /*icreate.icon.bbox.ymin = 0;*/
    icreate.icon.bbox.xmax = MAX(size.cx, ICONBAR_SPRITE_MIN_H);
    icreate.icon.bbox.ymax = MAX(size.cy, ICONBAR_SPRITE_MIN_V);
    } /*block*/

    xstrkpy(icreate.icon.data.s, elemof32(icreate.icon.data.s), buffer);

    void_WrapOsErrorReporting(wimp_create_icon_with_bitset(&icreate, &iconbar_handle));
    } /*block*/

    status_assert(winx_register_event_handler(ICONBAR_WIMP_W, iconbar_handle, iconbar_event_handler, NULL));

    (void) event_register_iconbar_menumaker(iconbar_handle, ho_menu_event_maker, ho_menu_event_proc, (P_ANY) (uintptr_t) viewid_pack(P_DOCU_NONE, NULL, MENU_ROOT_ICON));

    return(STATUS_OK);
}

_Check_return_
static STATUS
ho_event_msg_exit2(void)
{
    (void) event_register_iconbar_menumaker(iconbar_handle, NULL, NULL, (P_ANY) (uintptr_t) viewid_pack(P_DOCU_NONE, NULL, MENU_ROOT_ICON));

    status_assert(winx_register_event_handler(ICONBAR_WIMP_W, iconbar_handle, NULL, NULL));

    void_WrapOsErrorReporting(wimp_dispose_icon((wimp_w) (-1) /* icon bar */, &iconbar_handle));

    return(winx_register_general_event_handler(NULL, NULL));
}

T5_MSG_PROTO(static, maeve_services_ho_event_msg_initclose, _InRef_ PC_MSG_INITCLOSE p_msg_initclose)
{
    IGNOREPARM_DocuRef_(p_docu);
    IGNOREPARM_InVal_(t5_message);

    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__STARTUP:
        return(ho_event_msg_startup());

    case T5_MSG_IC__EXIT2:
        return(ho_event_msg_exit2());

    default:
        return(STATUS_OK);
    }
}

MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_ho_event)
{
    IGNOREPARM_InRef_(p_maeve_services_block);

    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        return(maeve_services_ho_event_msg_initclose(p_docu, t5_message, (PC_MSG_INITCLOSE) p_data));

    default:
        return(STATUS_OK);
    }
}

_Check_return_
extern STATUS
thesaurus_loaded(void)
{
    return(thesaurus_loaded_state);
}

_Check_return_
extern STATUS
thesaurus_process_word(
    _In_z_      PC_USTR ustr)
{
    WimpMessage msg;

    if(!thesaurus_loaded())
        return(create_error(ERR_NO_THESAURUS));

    {
    QUICK_TBLOCK quick_tblock;
    quick_tblock_setup_without_clearing_tbuf(&quick_tblock, msg.data.bytes, elemof32(msg.data.bytes));
    status_return(quick_tblock_ustr_add_n(&quick_tblock, ustr, strlen_with_NULLCH));
    quick_tblock_dispose_leaving_buffer_valid(&quick_tblock);
    } /*block*/

    msg.hdr.size  = sizeof32(msg.hdr);
    msg.hdr.size += strlen32p1(msg.data.bytes);
    msg.hdr.size  = (msg.hdr.size + (4-1)) & ~(4-1);
    msg.hdr.my_ref = 0; /* fresh msg */
    msg.hdr.action_code = Message_ThesaurusReceive;

    void_WrapOsErrorReporting(wimp_send_message(Wimp_EUserMessage /*no ack required*/, &msg, 0 /*broadcast*/, BAD_WIMP_I, NULL));

    return(STATUS_OK);
}

extern void
thesaurus_startup(void)
{
    /* queue a message to Desktop Thesaurus to see if it's loaded; it will reply 'soon' */
    WimpMessage msg;

    msg.hdr.size = sizeof32(msg.hdr);
    msg.hdr.my_ref = 0; /* fresh msg */
    msg.hdr.action_code = Message_ThesaurusQuery;

    void_WrapOsErrorReporting(wimp_send_message(Wimp_EUserMessageRecorded, &msg, 0 /* broadcast */, BAD_WIMP_I, NULL));

    thesaurus_loaded_state = 1;
}

#endif /* RISCOS */

/* end of riscos/ho_event.c */
