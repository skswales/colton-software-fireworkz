/* report.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2013-2022 Stuart Swales */

/* SKS February 2013 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#if RISCOS
#include "ob_skel/xp_skelr.h"
#endif

#if RISCOS
#define LOW_MEMORY_LIMIT  0x00008000U /* Don't look at data in zero page on RISC OS */
#define HIGH_MEMORY_LIMIT 0xFFFFFFFCU /* 32-bit RISC OS 5 */
#endif

#if WINDOWS
#define REPORT_USE_PERFORMANCE_COUNTER 1
#endif

static struct REPORT_STATICS
{
    BOOL enabled;

#if RISCOS
    BOOL try_reporter;
    BOOL try_rpcemu_report;
#endif

    PTSTR buffer;
    U32 buffer_offset;
    U32 elemof_buffer;

    BOOL timing_enabled;

#if WINDOWS && defined(REPORT_USE_PERFORMANCE_COUNTER)
    LARGE_INTEGER timing_startup;
    F64 timing_frequency;
#else
    MONOTIME timing_startup;
    S32 timing_frequency;
#endif
}
g_report =
{
    FALSE

#if RISCOS
,   TRUE
,   TRUE
#endif
};

extern void
report_enable(
    _InVal_     BOOL enable)
{
    if( enable && (NULL == g_report.buffer) )
    {
        const U32 elemof_buffer = 1024; /* limit for PipeDream */ /*4096;*/
#if RISCOS /* avoid our own redirection */
#undef malloc
#endif
        if(NULL == (g_report.buffer = malloc(elemof_buffer * sizeof(TCHAR))))
            return;
        g_report.elemof_buffer = elemof_buffer;
    }

    g_report.enabled = enable;
}

_Check_return_
extern BOOL
report_enabled(void)
{
    return(g_report.enabled);
}

_Check_return_
extern BOOL
reporting_is_enabled(void)
{
    if(!g_report.enabled)
        return(FALSE);

#if RISCOS
    if( !g_report.try_reporter &&
        !g_report.try_rpcemu_report &&
        (0 == stderr->__file) /* not redirected? */ )
    {
        return(FALSE);
    }
#endif

    return(TRUE);
}

extern void
report_timing_enable(
    _InVal_     BOOL enable)
{
    g_report.timing_enabled = enable;

    if( enable && (0 == g_report.timing_frequency) )
    {
#if WINDOWS && defined(REPORT_USE_PERFORMANCE_COUNTER)
        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);
        g_report.timing_frequency = (F64) frequency.QuadPart;
        QueryPerformanceCounter(&g_report.timing_startup);
#else
        g_report.timing_frequency = MONOTIME_TICKS_PER_SECOND;
        g_report.timing_startup = monotime();
#endif
    }
}

_Check_return_
extern BOOL
report_timing_enabled(void)
{
    return(g_report.timing_enabled);
}

/******************************************************************************
*
* reportf routine - useful for instrumenting release code
*
******************************************************************************/

extern void __cdecl
reportf(
    _In_z_ _Printf_format_string_ PCTSTR format,
    /**/        ...)
{
    va_list args;

    if(!g_report.enabled) return;

    va_start(args, format);
    vreportf(format, args);
    va_end(args);
}

/******************************************************************************
*
* vreportf routine
*
******************************************************************************/

extern void
vreportf(
    _In_z_ _Printf_format_string_ PCTSTR format,
    /**/        va_list args)
{
    const U32 elemof_report_buffer = g_report.elemof_buffer;
    PCTSTR tail;
    BOOL wants_continuation, needs_newline;
    int len;

    if(!g_report.enabled) return;

    if(NULL == g_report.buffer) return;

    if(format[0] == CH_VERTICAL_LINE)
    {   /* this one is a continuation - doesn't matter if it's already been flushed */
        format++;
    }
    else if(0 != g_report.buffer_offset)
    {   /* this one is a not a continuation - flush anything pending */
        g_report.buffer_offset = 0;
        report_output(g_report.buffer);
    }

    tail = format + tstrlen32(format);

    if(tail == format)
        return;

    wants_continuation = (tail[-1] == CH_VERTICAL_LINE);

    needs_newline = (tail[-1] != '\n');

    if( (0 == g_report.buffer_offset) && g_report.timing_enabled )
    {   /* prefix new report with number of milliseconds since timing turned on */
#if WINDOWS && defined(REPORT_USE_PERFORMANCE_COUNTER)
        LARGE_INTEGER now;
        F64 seconds;
        QueryPerformanceCounter(&now);
        seconds = (now.QuadPart - g_report.timing_startup.QuadPart) / /*F64*/ g_report.timing_frequency;
        /* report to microsecond precision */
        len = _sntprintf_s(g_report.buffer + g_report.buffer_offset, elemof_report_buffer - g_report.buffer_offset, _TRUNCATE, TEXT("[%12.6f] "), seconds);
        g_report.buffer_offset += len;
#else /* C99 CRT */
        MONOTIMEDIFF delta = monotime_diff(g_report.timing_startup);
        div_t d = div(delta, g_report.timing_frequency);
        const int seconds = d.quot;
        const int frac_seconds = d.rem;
#if MONOTIME_TICKS_PER_SECOND == 100
        /* report to centisecond precision */
        PCTSTR format = "[%5d.%.02d] ";
#elif MONOTIME_TICKS_PER_SECOND == 1000
        /* report to millisecond precision */
        PCTSTR format = "[%5d.%.03d] ";
#endif

        len = snprintf(g_report.buffer + g_report.buffer_offset, elemof_report_buffer - g_report.buffer_offset, format, seconds, frac_seconds);
        if(len < 0)
            len = 0;
        g_report.buffer_offset += len;
#endif
    }

#if WINDOWS
    len = _vsntprintf_s(g_report.buffer + g_report.buffer_offset, elemof_report_buffer - g_report.buffer_offset, _TRUNCATE, format, args);
    if(-1 == len)
        len = tstrlen32(g_report.buffer); /* limit to what actually was achieved */
#else /* C99 CRT */
    len = vsnprintf(g_report.buffer + g_report.buffer_offset, elemof_report_buffer - g_report.buffer_offset, format, args);
    if(len < 0)
        len = 0;
    else if((U32) len >= elemof_report_buffer)
        len = strlen32(g_report.buffer); /* limit to what actually was achieved */
#endif

    if(0 == len)
        return;

    if(wants_continuation)
    {
        if(CH_VERTICAL_LINE == g_report.buffer[g_report.buffer_offset + (len - 1)]) /* may have been truncated losing trailing chars - if so, just output */
        {
            g_report.buffer_offset += len;

            g_report.buffer[--g_report.buffer_offset] = CH_NULL; /* otherwise retract back over the terminal continuation char and leave for next time */
            return;
        }
    }

    if(needs_newline)
    {   /* this saves the overhead of an extra OutputDebugString on Windows */
        g_report.buffer_offset += len;

        if(g_report.buffer_offset+1 < elemof_report_buffer)
        {
            g_report.buffer[g_report.buffer_offset+0] = '\n';
            g_report.buffer[g_report.buffer_offset+1] = CH_NULL;
        }
    }

    g_report.buffer_offset = 0;
    report_output(g_report.buffer);
}

/******************************************************************************
*
* write a string to the reporter module (or file, or whatever else is suitable)
*
******************************************************************************/

#if RISCOS

#if defined(__CC_NORCROFT)
#ifndef __swis_h
#include "swis.h" /*C:*/
#endif

#define XReport_Text0 (XOS_Bit | /*Report_Text0*/ 0x54C80)
#define XRPCEmu_Debug (XOS_Bit | /*RPCEmu_Debug*/ 0x56AC2)

extern int __swi(XReport_Text0) __swi_XReport_Text0(const char * s);
extern int __swi(XRPCEmu_Debug) __swi_XRPCEmu_Debug(const char * s);

#else

extern int /*__swi(XReport_Text0)*/ __swi_XReport_Text0(const char * s);
extern int /*__swi(XRPCEmu_Debug)*/ __swi_XRPCEmu_Debug(const char * s);

#endif

#endif /* RISCOS */

extern void
report_output(
    _In_z_      PCTSTR buffer)
{
    PCTSTR tail;
    BOOL needs_newline;
    static const TCHARZ newline_buffer[2] = TEXT("\n");

    if(!g_report.enabled) return;

    tail = buffer + tstrlen32(buffer);
    if(tail == buffer)
        return;

    needs_newline = (tail[-1] != '\n');

#if RISCOS
    if(g_report.try_reporter)
    {
        if(__swi_XReport_Text0(buffer) == (int) buffer)
        {   /* Reporter module always sticks a newline on all output */
            return;
        }
        g_report.try_reporter = FALSE;
    }

    if(g_report.try_rpcemu_report)
    {
        if(__swi_XRPCEmu_Debug(buffer) == (int) buffer)
        {
            if(!needs_newline)
                return;

            if(__swi_XRPCEmu_Debug(newline_buffer) == (int) newline_buffer)
                return;
        }
        g_report.try_rpcemu_report = FALSE;
    }

#if defined(__CC_NORCROFT)
    if(0 == stderr->__file) /* not redirected? */
    {
        trace_disable();
        return;
    }
#endif

    {
    PCTSTR ptr = buffer;
    U8 ch;

    while((ch = *ptr++) != CH_NULL)
        switch(ch)
        {
        case 0x0A: /* LF  */
        case 0x0D: /* CR  */
            fputc(0x0A, stderr);
            break;

        case 0x7F: /* DEL */
            fputc(CH_VERTICAL_LINE, stderr);
            fputc('?', stderr);
            break;

        default:
            if(ch <= 0x1F)
            {
                fputc(CH_VERTICAL_LINE, stderr);
                ch = ch + '@';
            }

            fputc(ch, stderr);
            break;
        }

    if(needs_newline)
        fputc(0x0A, stderr);
    } /*block*/
#elif WINDOWS
    /* Don't get too excited about debugging Unicode - I quote ...
     * OutputDebugStringW converts the specified string based on the current system locale information and
     * passes it to OutputDebugStringA to be displayed. As a result, some Unicode characters may not be displayed correctly.
     */
    OutputDebugString(buffer);

    if(needs_newline)
        OutputDebugString(newline_buffer);
#endif /* OS */
}

_Check_return_
_Ret_z_
extern PCTSTR
report_boolstring(
    _InVal_     BOOL t)
{
    return(t ? TEXT("True ") : TEXT("False"));
}

_Check_return_
_Ret_z_
extern PCTSTR
report_procedure_name(
    report_proc proc)
{
    PCTSTR name = TEXT("<<not found>>");
    union _deviant_proc
    {
        report_proc proc;
        uintptr_t value;
        PC_ANY ptr;
    } deviant;

    deviant.proc = proc;

#if RISCOS
    if( ! ( (deviant.value & (uintptr_t) 0x00000003U      ) ||
            (deviant.value < (uintptr_t) LOW_MEMORY_LIMIT ) ||
            (deviant.value > (uintptr_t) HIGH_MEMORY_LIMIT) ) )
    {
        PC_U32 z = (PC_U32) deviant.ptr;
        U32 w = *--z;
        if((w & 0xFFFF0000) == 0xFF000000) /* marker? */
        {
            name = ((PC_U8Z) z) - (w & 0xFFFF);
            return(name);
        }
    }
#endif
    {
    static TCHARZ buffer[16];
#if WINDOWS
    consume_int(_sntprintf_s(buffer, elemof32(buffer), _TRUNCATE, PTR_XTFMT, deviant.ptr));
#else /* C99 CRT */
    consume_int(snprintf(buffer, elemof32(buffer), PTR_XTFMT, deviant.ptr));
#endif
    name = buffer;
    } /*block*/

    return(name);
}

_Check_return_
_Ret_z_
static PCTSTR
report_bad_pointer(
    PC_ANY p_any)
{
    static TCHARZ bad_pointer_buffer[16];
#if WINDOWS
    consume_int(_sntprintf_s(bad_pointer_buffer, elemof32(bad_pointer_buffer), _TRUNCATE, TEXT("<<") PTR_XTFMT TEXT(">>"), p_any));
#else /* C99 CRT */
    consume_int(snprintf(bad_pointer_buffer, elemof32(bad_pointer_buffer), TEXT("<<") PTR_XTFMT TEXT(">>"), p_any));
#endif
    return(bad_pointer_buffer);
}

_Check_return_
_Ret_z_
extern PCTSTR
report_tstr(
    _In_opt_z_  PCTSTR tstr)
{
    if(NULL == tstr)
        return(TEXT("<<NULL>>"));

#if CHECKING
    if(PTR_IS_NONE(tstr))
        return(TEXT("<<NONE>>"));
#endif

    if(
#if defined(HIGH_MEMORY_LIMIT)
        ((uintptr_t) tstr < (uintptr_t) LOW_MEMORY_LIMIT ) ||
        ((uintptr_t) tstr > (uintptr_t) HIGH_MEMORY_LIMIT) ||
#endif
        PTR_IS_BAD_POINTER(tstr) )
    {
        return(report_bad_pointer(tstr));
    }

#if !TSTR_IS_SBSTR
    /* Watch out for inlines in TSTRs if they're not WSTR */
    if(contains_inline(tstr, tstrlen32(tstr)))
    {
        assert0();
        return(TEXT("<<CONTAINS INLINES>>"));
    }
#endif

    return(tstr);
}

_Check_return_
_Ret_z_
extern PCTSTR
report_ustr(
    _In_opt_z_  PC_USTR ustr)
{
    if(NULL == ustr)
        return(TEXT("<<NULL>>"));

#if CHECKING
    if(PTR_IS_NONE(ustr))
        return(TEXT("<<NONE>>"));
#endif

    if(
#if defined(HIGH_MEMORY_LIMIT)
        ((uintptr_t) ustr < (uintptr_t) LOW_MEMORY_LIMIT ) ||
        ((uintptr_t) ustr > (uintptr_t) HIGH_MEMORY_LIMIT) ||
#endif
        PTR_IS_BAD_POINTER(ustr) )
    {
        return(report_bad_pointer(ustr));
    }

    /* Always watch out for inlines in USTRs! */
    if(contains_inline(ustr, ustrlen32(ustr)))
    {
        assert0();
        return(TEXT("<<CONTAINS INLINES>>"));
    }

    return(_tstr_from_ustr(ustr));
}

#if WINDOWS

_Check_return_
_Ret_z_
extern PCTSTR
report_wstr(
    _In_opt_z_  PCWSTR wstr)
{
    if(NULL == wstr)
        return(TEXT("<<NULL>>"));

#if CHECKING
    if(PTR_IS_NONE(wstr))
        return(TEXT("<<NONE>>"));
#endif

    if(
#if defined(HIGH_MEMORY_LIMIT)
        ((uintptr_t) wstr < (uintptr_t) LOW_MEMORY_LIMIT ) ||
        ((uintptr_t) wstr > (uintptr_t) HIGH_MEMORY_LIMIT) ||
#endif
        PTR_IS_BAD_POINTER(wstr) )
    {
        return(report_bad_pointer(wstr));
    }

    return(_tstr_from_wstr(wstr));
}

#endif

_Check_return_
_Ret_z_
extern PCTSTR
report_sbstr(
    _In_opt_z_  PC_SBSTR sbstr)
{
    if(NULL == sbstr)
        return(TEXT("<<NULL>>"));

#if CHECKING
    if(PTR_IS_NONE(sbstr))
        return(TEXT("<<NONE>>"));
#endif

    if(
#if defined(HIGH_MEMORY_LIMIT)
        ((uintptr_t) sbstr < (uintptr_t) LOW_MEMORY_LIMIT ) ||
        ((uintptr_t) sbstr > (uintptr_t) HIGH_MEMORY_LIMIT) ||
#endif
        PTR_IS_BAD_POINTER(sbstr) )
    {
        return(report_bad_pointer(sbstr));
    }

    /* Always watch out for inlines in SBSTR! */
    if(contains_inline(sbstr, strlen32(sbstr)))
    {
        assert0();
        return(TEXT("<<CONTAINS INLINES>>"));
    }

    return(_tstr_from_sbstr(sbstr));
}

#if RISCOS

/******************************************************************************
*
* R I S C   O S   s p e c i f i c   r e p o r t i n g   r o u t i n e s
*
******************************************************************************/

_Check_return_
_Ret_z_
extern PCTSTR
report_wimp_event_code(
    _InVal_     int event_code)
{
    static TCHARZ default_eventstring[10];

    switch(event_code)
    {
    case Wimp_ENull:                    return(TEXT("Null_Reason_Code"));
    case Wimp_ERedrawWindow:            return(TEXT("Redraw_Window_Request"));
    case Wimp_EOpenWindow:              return(TEXT("Open_Window_Request"));
    case Wimp_ECloseWindow:             return(TEXT("Close_Window_Request"));
    case Wimp_EPointerLeavingWindow:    return(TEXT("Pointer_Leaving_Window"));
    case Wimp_EPointerEnteringWindow:   return(TEXT("Pointer_Entering_Window"));
    case Wimp_EMouseClick:              return(TEXT("Mouse_Click"));
    case Wimp_EUserDrag:                return(TEXT("User_Drag_Box"));
    case Wimp_EKeyPressed:              return(TEXT("Key_Pressed"));
    case Wimp_EMenuSelection:           return(TEXT("Menu_Selection"));
    case Wimp_EScrollRequest:           return(TEXT("Scroll_Request"));
    case Wimp_ELoseCaret:               return(TEXT("Lose_Caret"));
    case Wimp_EGainCaret:               return(TEXT("Gain_Caret"));
    case Wimp_EPollWordNonZero:         return(TEXT("Poll_Word_Non_Zero"));
    /* 14..16 not defined */
    case Wimp_EUserMessage:             return(TEXT("User_Message"));
    case Wimp_EUserMessageRecorded:     return(TEXT("User_Message_Recorded"));
    case Wimp_EUserMessageAcknowledge:  return(TEXT("User_Message_Acknowledge"));

    default:
        consume_int(snprintf(default_eventstring, elemof32(default_eventstring), U32_XTFMT, (U32) event_code));
        return(default_eventstring);
    }
}

_Check_return_
_Ret_opt_z_
static PTSTR
report_wimp_get_buffer(
    _InoutRef_  P_PTSTR p_buffer,
    _InVal_     U32 elemof_buffer)
{
    if(NULL == *p_buffer)
    {
        STATUS status;
        *p_buffer = al_ptr_alloc_elem(TCHAR, elemof_buffer, &status);
    }

    return(*p_buffer);
}

_Check_return_
_Ret_z_
extern PCTSTR
report_wimp_event(
    _InVal_     int event_code,
    _In_        const void * const p_event_data)
{
    const WimpPollBlock * const event_data = (const WimpPollBlock * const) p_event_data;
    char tempbuffer[256];

    const U32 elemof_messagebuffer = 512;
    static PTSTR messagebuffer;

    if(NULL == report_wimp_get_buffer(&messagebuffer, elemof_messagebuffer))
        return(report_tstr(NULL));

    tstr_xstrkpy(messagebuffer, elemof_messagebuffer, report_wimp_event_code(event_code));

    switch(event_code)
    {
    case Wimp_EOpenWindow:
        consume_int(
            snprintf(tempbuffer, elemof32(tempbuffer),
                     TEXT(": window ") UINTPTR_XTFMT TEXT("; coords (") INT_TFMT TEXT(", ") INT_TFMT TEXT(", ") INT_TFMT TEXT(", ") INT_TFMT TEXT("); scroll (") INT_TFMT TEXT(", ") INT_TFMT TEXT("); behind ") UINTPTR_XTFMT,
                     (uintptr_t) event_data->open_window_request.window_handle,
                     event_data->open_window_request.visible_area.xmin, event_data->open_window_request.visible_area.ymin,
                     event_data->open_window_request.visible_area.xmax, event_data->open_window_request.visible_area.ymax,
                     event_data->open_window_request.xscroll,           event_data->open_window_request.yscroll,
                     (uintptr_t) event_data->open_window_request.behind));
        break;

    case Wimp_ERedrawWindow:
    case Wimp_ECloseWindow:
    case Wimp_EPointerLeavingWindow:
    case Wimp_EPointerEnteringWindow:
        consume_int(
            snprintf(tempbuffer, elemof32(tempbuffer),
                     TEXT(": window ") UINTPTR_XTFMT,
                     (uintptr_t) event_data->close_window_request.window_handle));
        break;

    case Wimp_EMouseClick:
        consume_int(
            snprintf(tempbuffer, elemof32(tempbuffer),
                     TEXT(": window ") UINTPTR_XTFMT TEXT("; icon ") INT_TFMT TEXT("; mouse x ") INT_TFMT TEXT(" y ") INT_TFMT TEXT("; buttons ") U32_XTFMT,
                     (uintptr_t) event_data->mouse_click.window_handle,
                     event_data->mouse_click.icon_handle,
                     event_data->mouse_click.mouse_x, event_data->mouse_click.mouse_y,
                     event_data->mouse_click.buttons));
        break;

    case Wimp_EKeyPressed:
        consume_int(
            snprintf(tempbuffer, elemof32(tempbuffer),
                     TEXT(": window ") UINTPTR_XTFMT TEXT("; icon ") INT_TFMT TEXT("; x ") INT_TFMT TEXT(" y ") INT_TFMT TEXT("; height ") U32_XTFMT TEXT(" index ") INT_TFMT TEXT("; key_code ") U32_XTFMT,
                     (uintptr_t) event_data->key_pressed.caret.window_handle,
                     event_data->key_pressed.caret.icon_handle,
                     event_data->key_pressed.caret.xoffset, event_data->key_pressed.caret.yoffset,
                     event_data->key_pressed.caret.height,  event_data->key_pressed.caret.index,
                     (U32) event_data->key_pressed.key_code));
        break;

    case Wimp_EScrollRequest:
        consume_int(
            snprintf(tempbuffer, elemof32(tempbuffer),
                     TEXT(": window ") UINTPTR_XTFMT TEXT("; coords (") INT_TFMT TEXT(", ") INT_TFMT TEXT(", ") INT_TFMT TEXT(", ") INT_TFMT TEXT("); scroll (") INT_TFMT TEXT(", ") INT_TFMT TEXT("); dir'n (") INT_TFMT TEXT(", ") INT_TFMT TEXT(")"),
                     (uintptr_t) event_data->scroll_request.open.window_handle,
                     event_data->scroll_request.open.visible_area.xmin, event_data->scroll_request.open.visible_area.ymin,
                     event_data->scroll_request.open.visible_area.xmax, event_data->scroll_request.open.visible_area.ymax,
                     event_data->scroll_request.open.xscroll,           event_data->scroll_request.open.yscroll,
                     event_data->scroll_request.xscroll,                event_data->scroll_request.yscroll));
        break;

    case Wimp_ELoseCaret:
    case Wimp_EGainCaret:
        consume_int(
            snprintf(tempbuffer, elemof32(tempbuffer),
                     TEXT(": window ") UINTPTR_XTFMT TEXT("; icon ") INT_TFMT TEXT(" x ") INT_TFMT TEXT(" y ") INT_TFMT TEXT("; height ") U32_XTFMT TEXT(" index ") INT_TFMT,
                     (uintptr_t) event_data->lose_caret.window_handle,
                     event_data->lose_caret.icon_handle,
                     event_data->lose_caret.xoffset, event_data->lose_caret.yoffset,
                     event_data->lose_caret.height,  event_data->lose_caret.index));
        break;

    case Wimp_EUserMessage:
    case Wimp_EUserMessageRecorded:
    case Wimp_EUserMessageAcknowledge:
        consume_int(
            snprintf(tempbuffer, elemof32(tempbuffer),
                     TEXT(": %s"), report_wimp_message(&event_data->user_message, FALSE)));
        break;

    case Wimp_ENull:
    case Wimp_EUserDrag:
    case Wimp_EMenuSelection:

    default:
        *tempbuffer = CH_NULL;
        break;
    }

    if(*tempbuffer)
        tstr_xstrkat(messagebuffer, elemof_messagebuffer, tempbuffer);

    return(messagebuffer);
}

_Check_return_
_Ret_z_
extern PCTSTR
report_wimp_message_action(
    _InVal_     int message_action)
{
    static TCHARZ default_actionstring[10];

    switch(message_action)
    {
    case Wimp_MQuit:            return(TEXT("Message_Quit"));
    case Wimp_MDataSave:        return(TEXT("Message_DataSave"));
    case Wimp_MDataSaveAck:     return(TEXT("Message_DataSaveAck"));
    case Wimp_MDataLoad:        return(TEXT("Message_DataLoad"));
    case Wimp_MDataLoadAck:     return(TEXT("Message_DataLoadAck"));
    case Wimp_MDataOpen:        return(TEXT("Message_DataOpen"));
    case Wimp_MRAMFetch:        return(TEXT("Message_RAMFetch"));
    case Wimp_MRAMTransmit:     return(TEXT("Message_RAMTransmit"));
    case Wimp_MPreQuit:         return(TEXT("Message_PreQuit"));
    case Wimp_MPaletteChange:   return(TEXT("Message_PaletteChange"));
    case Wimp_MSaveDesktop:     return(TEXT("Message_SaveDesktop"));
    /*Wimp_MDeviceClaim*/
    /*Wimp_MDeviceInUse*/
    case Wimp_MDataSaved:       return(TEXT("Message_DataSaved"));
    case Wimp_MShutDown:        return(TEXT("Message_ShutDown"));
    case Wimp_MClaimEntity:     return(TEXT("Message_ClaimEntity"));
    /*Wimp_MDataRequest*/
    /*Wimp_MDragging*/
    /*Wimp_MDragClaim*/
    case Wimp_MAppControl:      return(TEXT("Message_AppControl"));

    case Wimp_MFilerOpenDir:    return(TEXT("Message_FilerOpenDir"));
    case Wimp_MFilerCloseDir:   return(TEXT("Message_FilerCloseDir"));

    case Wimp_MHelpRequest:     return(TEXT("Message_HelpRequest"));
    case Wimp_MHelpReply:       return(TEXT("Message_HelpReply"));

    case Wimp_MMenuWarning:     return(TEXT("Message_MenuWarning"));
    case Wimp_MModeChange:      return(TEXT("Message_ModeChange"));
    case Wimp_MTaskInitialise:  return(TEXT("Message_TaskInitialise"));
    case Wimp_MTaskCloseDown:   return(TEXT("Message_TaskCloseDown"));
    case Wimp_MSlotSize:        return(TEXT("Message_SlotSize"));
    case Wimp_MSetSlot:         return(TEXT("Message_SetSlot"));
    case Wimp_MTaskNameRq:      return(TEXT("Message_TaskNameRq"));
    case Wimp_MTaskNameIs:      return(TEXT("Message_TaskNameIs"));
    case Wimp_MTaskStarted:     return(TEXT("Message_TaskStarted"));
    case Wimp_MMenusDeleted:    return(TEXT("Message_MenusDeleted"));
    case Wimp_MIconize:         return(TEXT("Message_Iconize"));
    case Wimp_MWindowClosed:    return(TEXT("Message_WindowClosed"));
    case Wimp_MWindowInfo:      return(TEXT("Message_WindowInfo"));
    case Wimp_MSwap:            return(TEXT("Message_Swap"));
    case Wimp_MToolsChanged:    return(TEXT("Message_ToolsChanged"));
    case Wimp_MFontChanged:     return(TEXT("Message_FontChanged"));

    case Wimp_MPrintFile:       return(TEXT("Message_PrintFile"));
    case Wimp_MWillPrint:       return(TEXT("Message_WillPrint"));
    case Wimp_MPrintSave:       return(TEXT("Message_PrintSave"));
    case Wimp_MPrintInit:       return(TEXT("Message_PrintInit"));
    case Wimp_MPrintError:      return(TEXT("Message_PrintError"));
    case Wimp_MPrintTypeOdd:    return(TEXT("Message_PrintTypeOdd"));
    case Wimp_MPrintTypeKnown:  return(TEXT("Message_PrintTypeKnown"));
    case Wimp_MSetPrinter:      return(TEXT("Message_SetPrinter"));

    default:
        consume_int(snprintf(default_actionstring, elemof32(default_actionstring), U32_XTFMT, (U32) message_action));
        return(default_actionstring);
    }
}

_Check_return_
_Ret_z_
extern PCTSTR
report_wimp_message(
    _In_        const void * const p_wimp_message,
    _InVal_     BOOL sending)
{
    const WimpMessage * const user_message = (const WimpMessage * const) p_wimp_message;
    PCTSTR format =
        sending
            ? TEXT("%s, size ") INT_TFMT TEXT(", your_ref ") U32_XTFMT TEXT(" ")
            : TEXT("%s, size ") INT_TFMT TEXT(", your_ref ") U32_XTFMT TEXT(" from task " U32_XTFMT ", my(his)_ref ") U32_XTFMT TEXT(" ");
    TCHARZ tempbuffer[256];

    const U32 elemof_messagebuffer = 512;
    static PTSTR messagebuffer;

    if(NULL == report_wimp_get_buffer(&messagebuffer, elemof_messagebuffer))
        return(report_tstr(NULL));

    consume_int(
        snprintf(messagebuffer, elemof_messagebuffer, format,
                 report_wimp_message_action(user_message->hdr.action_code),
                 user_message->hdr.size, user_message->hdr.your_ref,
                 user_message->hdr.sender, user_message->hdr.my_ref));

    switch(user_message->hdr.action_code)
    {
    case Wimp_MDataLoadAck:
        if(user_message->hdr.size <= 44)
        {
            xstrkpy(tempbuffer, elemof32(tempbuffer), TEXT(": runt")); /* e.g. SrcEdit, Draw */
            break;
        }

        /*FALLTHRU*/

    case Wimp_MDataSave:
    case Wimp_MDataSaveAck:
    case Wimp_MDataLoad:
  /*case Wimp_MDataLoadAck:*/
    case Wimp_MDataOpen:
        consume_int(
            snprintf(tempbuffer, elemof32(tempbuffer),
                     TEXT(": dest window ") UINTPTR_XTFMT TEXT("; icon ") INT_TFMT TEXT("; x ") INT_TFMT TEXT(" y ") INT_TFMT TEXT("; estimated_size ") U32_XTFMT TEXT("; type: 0x%.3X; file: %s"),
                     user_message->data.data_load.destination_window,
                     user_message->data.data_load.destination_icon,
                     user_message->data.data_load.destination_x, user_message->data.data_load.destination_y,
                     user_message->data.data_load.estimated_size,
                     user_message->data.data_load.file_type,
                     user_message->data.data_load.leaf_name));
        break;

    case Wimp_MRAMFetch:
        consume_int(
            snprintf(tempbuffer, elemof32(tempbuffer),
                     TEXT(": buffer ") PTR_XTFMT TEXT("; size ") U32_XTFMT,
                     user_message->data.ram_fetch.buffer,
                     user_message->data.ram_fetch.buffer_size));
        break;

    case Wimp_MRAMTransmit:
        consume_int(
            snprintf(tempbuffer, elemof32(tempbuffer),
                     TEXT(": buffer ") PTR_XTFMT TEXT("; nbytes ") U32_XTFMT,
                     user_message->data.ram_transmit.buffer,
                     user_message->data.ram_transmit.nbytes));
        break;

    case Wimp_MClaimEntity:
        consume_int(
            snprintf(tempbuffer, elemof32(tempbuffer),
                     TEXT(": ") U32_XTFMT,
                     user_message->data.words[0]));
        break;

    case Wimp_MDataRequest:
        consume_int(
            snprintf(tempbuffer, elemof32(tempbuffer),
                     TEXT(": dest window ") UINTPTR_XTFMT TEXT("; icon ") INT_TFMT TEXT("; x ") INT_TFMT TEXT(" y ") INT_TFMT TEXT("; flags ") U32_XTFMT TEXT("; type[0]: 0x%.3X"),
                     user_message->data.data_load.destination_window,
                     user_message->data.data_load.destination_icon,
                     user_message->data.data_load.destination_x, user_message->data.data_load.destination_y,
                     ((const WimpDataRequestMessage *) (&user_message->data))->flags,
                     ((const WimpDataRequestMessage *) (&user_message->data))->type[0]));
        break;

    case Wimp_MHelpRequest:
        consume_int(
            snprintf(tempbuffer, elemof32(tempbuffer),
                     TEXT(": window ") UINTPTR_XTFMT TEXT("; icon ") INT_TFMT TEXT("; mouse x ") INT_TFMT TEXT(" y ") INT_TFMT TEXT("; buttons ") INT_TFMT,
                     user_message->data.help_request.window_handle,
                     user_message->data.help_request.icon_handle,
                     user_message->data.help_request.mouse_x, user_message->data.help_request.mouse_y,
                     user_message->data.help_request.buttons));
        break;

    case Wimp_MHelpReply:
        xstrkpy(tempbuffer, elemof32(tempbuffer), ": ");
        xstrkat(tempbuffer, elemof32(tempbuffer), user_message->data.help_reply.text);
        break;

    default:
        *tempbuffer = CH_NULL;
        break;
    }

    if(*tempbuffer)
        tstr_xstrkat(messagebuffer, elemof_messagebuffer, tempbuffer);

    return(messagebuffer);
}

#endif /* RISCOS */

/* end of report.c */
