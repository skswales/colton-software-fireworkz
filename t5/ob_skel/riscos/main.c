/* riscos/main.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* main() and other initialisation routines for Fireworkz */

/* RCM Apr 1992 */

#include "common/gflags.h"

#if RISCOS

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#include "ob_skel/prodinfo.h"

#define EXPOSE_RISCOS_SWIS
#include "ob_skel/xp_skelr.h"

#include <ctype.h> /* for tolower - only use is here before started fully */

#include <locale.h> /* for setlocale */

#ifndef ROOT_STACK_SIZE
#define ROOT_STACK_SIZE 16384
#endif

extern int __root_stack_size; /* keep compiler happy */

int __root_stack_size = ROOT_STACK_SIZE;

/*
from assembler riscos/ho_asm.c
*/

extern void
EscH(
    int * p_ctrlflag);

extern void
EventH(void);

/*
exported for CLib
*/

extern int
main(
    int argc,
    char * argv[]);

#if defined(BOUND_MESSAGES_OBJECT_ID_SKEL)
extern PC_U8 rb_skel_msg_weak;
#define P_BOUND_MESSAGES_OBJECT_ID_SKEL &rb_skel_msg_weak
#else
#define P_BOUND_MESSAGES_OBJECT_ID_SKEL LOAD_MESSAGES_FILE
#endif

#define P_BOUND_RESOURCES_OBJECT_ID_SKEL LOAD_RESOURCES

#if defined(PROFILING)

#include "cmodules/profile.h"

/******************************************************************************
*
* called by atexit() on exit(rc)
*
******************************************************************************/

PROC_ATEXIT_PROTO(static, profile_atexit)
{
    profile_off();
    profile_output(PROFILE_SORT_BY_COUNT, "$.Temp.pro_count", ",");
  /*profile_output(PROFILE_SORT_BY_NAME,  "$.Temp.pro_name",  ",");*/
  /*profile_output(PROFILE_SORT_BY_ADDR,  "$.Temp.pro_addr",  ",");*/
}

static void
profile_setup(void)
{
    profile_init();
    profile_on();
    atexit(profile_atexit);
}

#endif /* PROFILING */

/*
Read details from Info file - variables are now set up by Loader
*/

static void
get_user_info(void)
{
    TCHARZ var_name[BUF_MAX_PATHSTRING];

    if(NULL != _kernel_getenv(make_var_name(var_name, elemof32(var_name), "$User1"), __user_name, elemof32(__user_name)))
        /*EMPTY*//*__user_name[0] = CH_NULL*/;

#define SPECIAL_VERSION_PREFIX "+++"
    if(0 == memcmp32(__user_name, SPECIAL_VERSION_PREFIX, sizeof32(SPECIAL_VERSION_PREFIX)-1))
    {
        /* Remove the +++ prefix (caters for Iyonix bundle user's Info) */
        U32 len = strlen32(__user_name + sizeof32(SPECIAL_VERSION_PREFIX)-1);
        memmove32(__user_name, __user_name + sizeof32(SPECIAL_VERSION_PREFIX)-1, len);
        __user_name[len] = CH_NULL;
    }

    if(NULL != _kernel_getenv(make_var_name(var_name, elemof32(var_name), "$User2"), __organisation_name, elemof32(__organisation_name)))
        /*EMPTY*//*__organisation_name[0] = CH_NULL*/;

    if(NULL != _kernel_getenv(make_var_name(var_name, elemof32(var_name), "$RegNo"), __registration_number, elemof32(__registration_number)))
        /*EMPTY*//*__registration_number[0] = CH_NULL*/;
}

_Check_return_
static STATUS
import_this_foreign_file(
    _DocuRef_   P_DOCU cur_p_docu,
    _In_z_      PCTSTR filename)
{
    const T5_FILETYPE t5_filetype = t5_filetype_from_filename(filename);

    return(load_foreign_file_rl(cur_p_docu, filename, t5_filetype));
}

/*
no longer does anything on pass 1
*/

_Check_return_
static STATUS
decode_command_line_options(
    _In_        int argc,
    char * argv[],
    _In_        int pass)
{
    STATUS status = STATUS_OK;
    int argi = 0;

    while(++argi < argc)
    {
        P_U8 arg = argv[argi];

        if(CH_HYPHEN_MINUS == *arg)
        {
            U8 c = *++arg;

            switch(/*"C"*/tolower(c)) /* ASCII, no remapping */
            {
            case 'c':
                /* Execute command file */
                arg = argv[++argi];
                if(pass == 2)
                    status = load_this_command_file_rl(P_DOCU_NONE, arg);
                break;

            case 'i':
                /* Import */
                arg = argv[++argi];
                if(pass == 2)
                    status = import_this_foreign_file(P_DOCU_NONE, arg);
                break;

            case 't':
                /* LoadTemplate */
                arg = argv[++argi];
                if(pass == 2)
                    status = load_this_template_file_rl(P_DOCU_NONE, arg);
                break;

            case 'o':
            case 'p':
                /* Print */
                arg = argv[++argi];
                if(pass == 2)
                    status = load_and_print_this_file_rl(arg, NULL);
                break;

            default:
                break;
            }
        }
        else
        {
            if(pass == 2)
                status = load_this_fireworkz_file_rl(P_DOCU_NONE, arg, FALSE /*fReadOnly*/);
        }

        status_break(status);
    }

    return(status);
}

static void
decode_run_options(void)
{
    TCHARZ var_name[BUF_MAX_PATHSTRING];
    TCHARZ run_options[BUF_MAX_PATHSTRING];
    PC_SBSTR p_u8 = run_options;

    if(NULL != _kernel_getenv(make_var_name(var_name, elemof32(var_name), "$RunOptions"), run_options, elemof32(run_options)))
        return;

    while(NULL != p_u8)
    {
        PC_SBSTR arg = p_u8;

        StrSkipSpaces(arg); /* skip leading spaces */

        if(CH_NULL == *arg)
            break;

        if((CH_HYPHEN_MINUS == arg[0]) && (CH_HYPHEN_MINUS == arg[1]))
        {
            U32 arg_len;
            PC_U8Z test_arg;
            U32 test_len;

            arg += 2; /* skip -- */
            arg_len = strlen32(arg);

            test_arg = "dynamic-area-limit=";
            test_len = strlen32(test_arg);

            if((arg_len >= test_len) && (0 == memcmp32(arg, test_arg, test_len)))
            {
                P_U8Z p_u8_skip;
                U32 u32;

                arg += test_len;

                u32 = fast_strtoul(arg, &p_u8_skip);

                if(u32 != U32_MAX)
                    g_dynamic_area_limit = (u32 * 1024) * 1024; /*MB*/

                arg = p_u8_skip; /* skip digits */

                goto past_arg;
            }

            test_arg = "kill-duplicates";
            test_len = strlen32(test_arg);

            if((arg_len >= test_len) && (0 == memcmp32(arg, test_arg, test_len)))
            {
                arg += test_len;

                g_kill_duplicates = TRUE;

                goto past_arg;
            }
        }
        else
        {
            ++arg;
        }

past_arg:;
        p_u8 = strchr(arg, CH_SPACE); /* point past this arg, may end with NULL */
    }
}

#include <setjmp.h>
#include <signal.h>

static BOOL event_loop_jmp_set = FALSE;
static jmp_buf event_loop_jmp_buf;

#if defined(__CC_NORCROFT)
#pragma no_check_stack
#endif

extern void
host_longjmp_to_event_loop(int val)
{
    if(event_loop_jmp_set)
        longjmp(event_loop_jmp_buf, val);
}

static void
t5_internal_stack_overflow_handler(int sig)
{
    host_longjmp_to_event_loop(sig);

    exit(EXIT_FAILURE);
}

#if defined(__CC_NORCROFT)
#pragma check_stack
#endif

/* Escape events may happen while printing is happening; the printer driver should handle it, and the application simply deals with the returned error. */

static void
t5_escape_handler(int sig)
{
#if defined(NO_SURRENDER)
    if(host_ctrl_pressed())
    {
        if(host_shift_pressed())
            trace_on();
        else
            raise(SIGTERM);         /* goodbye cruel world */
    }
#endif

    /* reinstall ourselves, as SIG_DFL will have been restored */
    (void) signal(sig, &t5_escape_handler);
}

static BOOL
t5_signal_handler_common(int sig);

static void
t5_signal_handler(int sig)
{
    if(t5_signal_handler_common(sig))
        /* give it your best shot else we come back and die soon */
        host_longjmp_to_event_loop(sig);

    exit(EXIT_FAILURE);
}

static void
install_t5_signal_handlers(void)
{
    (void) signal(SIGABRT,    &t5_signal_handler);
    (void) signal(SIGFPE,     &t5_signal_handler);
    (void) signal(SIGILL,     &t5_signal_handler);
    (void) signal(SIGINT,     &t5_escape_handler);
    (void) signal(SIGSEGV,    &t5_signal_handler);
    (void) signal(SIGTERM,    &t5_signal_handler);
    (void) signal(SIGSTAK,    &t5_internal_stack_overflow_handler);
    (void) signal(SIGUSR1,    &t5_signal_handler);
    (void) signal(SIGUSR2,    &t5_signal_handler);
    (void) signal(SIGOSERROR, &t5_signal_handler);
}

/* keep defaults for these in case of msgs death */

static const U8Z
error_fatal_str[] =
    "error_fatal:%s has gone wrong (%s). "
    "Click Continue to quit, losing data";

static const U8Z
error_serious_str[] =
    "error_serious:%s has gone wrong (%s). "
    "Click Continue to try to resume or Cancel to quit, losing data.";

static PC_U8Z
tag_from_signal_number(int sig)
{
    static const PC_U8Z
    tag_table[] =
    {
        "signal0:Unknown",
        "signal1:Abnormal termination",
        "signal2:Arithmetic exception",
        "signal3:Illegal instruction",
        "signal4:Escape",
        "signal5:Address exception",
        "signal6:Termination request",
        "signal7:Stack overflow"
    };

    if(SIGOSERROR == sig)
        return("signal10:OS error");

    if((U32) sig >= elemof32(tag_table))
        sig = 0;

    return(tag_table[sig]);
}

static BOOL
t5_signal_handler_common(int sig)
{
    _kernel_oserror * e;
    _kernel_oserror err;
    const char * cause = NULL;
    BOOL must_die;
    BOOL jump_back = FALSE;
    int button_clicked;

    must_die = host_must_die_query() || (host_task_handle() == 0);
    host_must_die_set(TRUE); /* trap errors in lookup/sprintf etc. */

    // reportf("t5_signal_handler_common(sig=%d)", sig);

    switch(sig)
    {
    case SIGOSERROR:
        e = _kernel_last_oserror();
        if(NULL != e)
        {
            err.errnum = e->errnum;
            cause = e->errmess;
        }
        break;

    case SIGSTAK:
        /* I prefer my message... */
        break;

    default:
        e = _kernel_last_oserror();
        // reportf("t5_signal_handler_common: e=&%.8X, &%.8X", (uintptr_t) e, e ? e->errnum : 0);
        if( (NULL != e) && (0 != (e->errnum & (1U << 31))) )
        {   /* Only use the error if its already got top bit set */
            err.errnum = e->errnum;
            cause = e->errmess;
        }
        break;
    }

    if(NULL == cause)
    {
        err.errnum = sig;
        cause = string_for_object(tag_from_signal_number(sig), OBJECT_ID_SKEL);
    }

    err.errnum |= (1U << 31); /* Force a helpful Quit button for the users on RISC OS 3.5+ */
    consume_int(snprintf(err.errmess, elemof32(err.errmess),
                         string_for_object(must_die ? error_fatal_str : error_serious_str, OBJECT_ID_SKEL),
                         product_ui_id(),
                         cause));

    if(0 == host_task_handle())
    {
        _kernel_swi_regs rs;
        report_output(err.errmess);
        rs.r[0] = (int) &err;
        consume(_kernel_oserror *, _kernel_swi(OS_GenerateError, &rs, &rs));
    }
    else if(must_die)
    {
        consume(_kernel_oserror *, wimp_reporterror_rf(&err, Wimp_ReportError_OK, &button_clicked, NULL, 3 /*STOP*/));
        /* ignore button_clicked - we must die */
    }
    else
    {
        consume(_kernel_oserror *, wimp_reporterror_rf(&err, Wimp_ReportError_OK | Wimp_ReportError_Cancel, &button_clicked, NULL, 3 /*STOP*/));
        jump_back = (1 == button_clicked);
        if(jump_back)
            host_must_die_set(must_die); /* only time this is restored */
    }

    /* reinstall ourselves, as SIG_DFL will have been restored (as defined by the ANSI spec) */
    /* helps to trap any further errors during closedown rather than giving postmortem dump */
    switch(sig)
    {
    case SIGSTAK:
        break;

    default:
        (void) signal(sig, &t5_signal_handler);
        break;
    }

    return(jump_back);
}

/******************************************************************************
*
* called by atexit() on exit(rc)
*
******************************************************************************/

PROC_ATEXIT_PROTO(static, window_manager_atexit)
{
    void_WrapOsErrorReporting(wimp_close_down(host_task_handle()));
}

static BOOL g_host_must_die = FALSE;

_Check_return_
extern BOOL
host_must_die_query(void)
{
    return(g_host_must_die);
}

extern void
host_must_die_set(
    _InVal_     BOOL must_die)
{
    g_host_must_die = must_die;
}

static int g_host_task_handle;

_Check_return_
extern int
host_task_handle(void)
{
    return(g_host_task_handle);
}

static void
host_report_and_trace_enable(void)
{
    /* allow application to run without any report info */
    char env_value[BUF_MAX_PATHSTRING];

    report_enable(NULL == _kernel_getenv("Fireworkz$ReportEnable", env_value, elemof32(env_value)));

#if TRACE_ALLOWED
    /* Allow a version built with TRACE_ALLOWED to run on an end-user's system without outputting any trace info */
    if(NULL != _kernel_getenv("Fireworkz$TraceEnable", env_value, elemof32(env_value)))
    {
        trace_disable();
    }
    else
    {
        _kernel_stack_chunk *scptr;
        _kernel_swi_regs rs;
        unsigned long scsize;
        trace_on();
        trace_1(TRACE_OUT | TRACE_ANY, TEXT("main: sp ~= ") PTR_XTFMT, &scptr);
        scptr = _kernel_current_stack_chunk();
        scsize = scptr->sc_size;
        void_WrapOsErrorChecking(_kernel_swi(OS_GetEnv, &rs, &rs));
        trace_4(TRACE_OUT | TRACE_ANY, TEXT("main: stack chunk ") PTR_XTFMT TEXT(", size ") S32_TFMT TEXT(", top ") PTR_XTFMT TEXT(", slot end ") PTR_XTFMT, scptr, scsize, (char *) scptr + scsize, (char *) rs.r[1]);
    }
#endif /* TRACE_ALLOWED */
}

#if defined(UNUSED) || 0

static int
t5_stack_overflow_test(int i)
{
    unsigned char buf[256];
    unsigned int j;
    // *(int*)1 = i;
    for(j = 0; j < 256; ++j)
        buf[j] = i & 0xFF;
    return(buf[i & 0xFF] + t5_stack_overflow_test(i + 1));
}

#endif

/******************************************************************************
*
* Application starts here
*
******************************************************************************/

#ifdef XXM_LGPL_DYNAMIC_LINKING

__pragma(warning(push)) __pragma(warning(disable:4255)) /* converting () to (void) */
#include "../lgpl/xxM/xxM.h"
__pragma(warning(pop))

extern void
xxM_usr_init(void (* p_proc_entry) (void));

#endif /* XXM_LGPL_DYNAMIC_LINKING */

/* The list of messages that this application is interested in */

static const int
t5_wimp_messages[] =
{
    Wimp_MDataSave,
    Wimp_MDataSaveAck,
    Wimp_MDataLoad,
    Wimp_MDataLoadAck,
    Wimp_MDataOpen,
    Wimp_MPreQuit,
    Wimp_MPaletteChange,
    Wimp_MSaveDesktop,
    Wimp_MShutDown,
    Wimp_MClaimEntity,
    Wimp_MDataRequest,

    Wimp_MHelpRequest,

    Wimp_MMenuWarning,
    Wimp_MModeChange,
    Wimp_MTaskInitialise,
    Wimp_MWindowInfo,
    Wimp_MMenusDeleted,

    Wimp_MPrintFile,
    Wimp_MPrintSave,
    Wimp_MPrintError,
    Wimp_MPrintTypeOdd,

    0 /* terminate list */
};

int g_current_wm_version = 310;

_Check_return_
static BOOL
main_init(
    int argc,
    char * argv[])
{
    static int ctrlflag;

    STATUS status;

    /* Trap ESCAPE as soon as possible */
    EscH(&ctrlflag);

    /* Block events from reaching C runtime system where they cause enormous chugging */
    EventH();

#ifdef PROFILING
    profile_setup();
#endif

    muldiv64_init(); /* very early indeed */

    host_os_version_determine(); /* very early indeed */

    install_t5_signal_handlers();

    host_report_and_trace_enable();

    /* set locale for isalpha etc. (ctype.h functions) */
    trace_1(TRACE_OUT | TRACE_ANY, TEXT("main: initial CTYPE locale is %s"), setlocale(LC_CTYPE, NULL));
    consume_ptr(setlocale(LC_CTYPE, "C")); /* standard ASCII NB NOT "ISO8859-1" (ISO Latin-1) */

    get_user_info();

    if(status_fail(startup_t5_application_1()))
        return(FALSE);

    status_assert(decode_command_line_options(argc, argv, 1));

    decode_run_options();

    /* Startup Window Manager interface */
    (void) wimp_initialise(
                310 /*wm_version*/,
                de_const_cast(char *, product_ui_id()),
                de_const_cast(int *, t5_wimp_messages),
                &g_current_wm_version,
                &g_host_task_handle);

    atexit(window_manager_atexit); /* will now need to close our task correctly */

#ifdef PROFILING
    /* Task ID may now have changed, so re-register */
    profile_taskchange();
#endif

    if(status_fail(aligator_init())) /* which sets an atexit() for flex dynamic area shutdown */
    {   /* Output in English - unlikely to be able to load messages file for error report! */
        reperr(ERR_OUTPUT_STRING, TEXT("Not enough memory, or not in *Desktop world"));
        return(FALSE);
    }

    /* These may need doing prior to any object startup */
    host_modevar_cache_reset(); /* needs allocator going */
    host_palette_cache_reset(); /* which does host_rgb_stash(); */
    host_recache_mode_dependent_vars();

    thesaurus_startup();

    file_startup();

    host_initialise_file_paths();

    /* Make error messages available for startup */
    resource_startup();

    if(status_fail(status = resource_init(OBJECT_ID_SKEL, P_BOUND_MESSAGES_OBJECT_ID_SKEL, P_BOUND_RESOURCES_OBJECT_ID_SKEL)))
    {
        reperr_null(status);
        return(FALSE);
    }

    /* Now try and startup some non-host specific stuff */
    if(status_fail(startup_t5_application_2()))
        return(FALSE);

    status = decode_command_line_options(argc, argv, 2);

    alloc_track_stop();

#ifdef XXM_LGPL_DYNAMIC_LINKING
    {
    STUB_DESC stub_desc;
    RUNTIME_INFO runtime_info;
    if( status_ok(host_load_stubs("<Fireworkz$Dir>.Resources.Neutral.xxMstubs", &stub_desc)) &&
        status_ok(host_load_module("<Fireworkz$Dir>.Resources.Neutral.xxMso", &runtime_info, 1, stub_desc.p_stub_data)) )
    {
        xxM_usr_init(runtime_info.p_module_entry);
        xxM_test_routine();
    }
    } /*block*/
#endif /* XXM_LGPL_DYNAMIC_LINKING */

    return(TRUE);
}

extern int
main(
    int argc,
    char * argv[])
{
    if(!main_init(argc, argv))
        return(EXIT_FAILURE);

    /* Set up a point we can longjmp back to on serious error */
    switch(setjmp(event_loop_jmp_buf))
    {
    case SIGSTAK:
        report_output("*** Stack overflow ***");

        if(!t5_signal_handler_common(SIGSTAK))
            return(EXIT_FAILURE);

        /*FALLTHRU*/

    default:
        /* returned here from exception - tidy up as necessary */
        status_assert(maeve_service_event(P_DOCU_NONE, T5_MSG_HAD_SERIOUS_ERROR, P_DATA_NONE));

        trace_0(TRACE_APP_SKEL, TEXT("main: Starting to poll for messages again after serious error"));
        break;

    case 0:
        event_loop_jmp_set = TRUE;

        trace_0(TRACE_APP_SKEL, TEXT("main: Starting to poll for messages"));
        /* __crash_and_burn_here(); */
        /* t5_stack_overflow_test(0); */
        break;
    }

    for(;;)
    {
        WM_EVENT res = wm_event_get(FALSE /*fgNullEventsWanted*/);

        if(WM_EVENT_PROCESSED != res)
            break;
    }

    /* atexit() works on RISC OS, so we don't need explicit tidying here */
    return(EXIT_SUCCESS);
}

#endif /* RISCOS */

/* end of riscos/main.c */
