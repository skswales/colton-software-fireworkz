/* report.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2013-2015 Stuart Swales */

/* SKS February 2013 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#if RISCOS
#include "ob_skel/xp_skelr.h"
#endif

#if RISCOS
#define LOW_MEMORY_LIMIT  0x00008000U /* Don't look at data in zero page on RISC OS*/
#define HIGH_MEMORY_LIMIT 0xFFFFFFFCU /* 32-bit RISC OS 5 */
#endif

static BOOL
g_report_enabled = TRUE;

extern void
report_enable(
    _InVal_     BOOL enable)
{
    g_report_enabled = enable;
}

_Check_return_
extern BOOL
report_enabled(void)
{
    return(g_report_enabled);
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

    if(!g_report_enabled) return;

    va_start(args, format);
    vreportf(format, args);
    va_end(args);
}

/******************************************************************************
*
* vreportf routine
*
******************************************************************************/

static TCHARZ report_buffer[4096];
static U32    report_buffer_offset = 0;

extern void
vreportf(
    _In_z_ _Printf_format_string_ PCTSTR format,
    /**/        va_list args)
{
    PCTSTR tail;
    BOOL wants_continuation;

    if(!g_report_enabled) return;

    if(format[0] == CH_VERTICAL_LINE)
    {   /* this one is a continuation - doesn't matter if it's already been flushed */
        format++;
    }
    else if(0 != report_buffer_offset)
    {   /* this one is a not a continuation - flush anything pending */
        report_buffer_offset = 0;
        report_output(report_buffer);
    }

    tail = format + tstrlen32(format);
    wants_continuation = (tail != format) && (tail[-1] == CH_VERTICAL_LINE);

#if WINDOWS
    consume_int(_vsntprintf_s(report_buffer + report_buffer_offset, elemof32(report_buffer) - report_buffer_offset, _TRUNCATE, format, args));
#else /* C99 CRT */
    consume_int(vsnprintf(report_buffer + report_buffer_offset, elemof32(report_buffer) - report_buffer_offset, format, args));
#endif

    if(wants_continuation)
    {
        report_buffer_offset += tstrlen32(report_buffer + report_buffer_offset);
        if(report_buffer[report_buffer_offset - 1] == CH_VERTICAL_LINE) /* may have been truncated - if so, just output */
        {
            report_buffer[--report_buffer_offset] = CH_NULL; /* otherwise retract back over the terminal continuation char */
            return;
        }
    }

    report_buffer_offset = 0;
    report_output(report_buffer);
}

/******************************************************************************
*
* write a string to the reporter module (or file, or whatever else is suitable)
*
******************************************************************************/

#if RISCOS

#ifndef __swis_h
#include "swis.h" /*C:*/
#endif

#define XReport_Text0 (XOS_Bit | /*Report_Text0*/ 0x54C80)

extern int __swi(XReport_Text0) __swi_XReport_Text0(const char * s);

#define XRPCEmu_Debug (XOS_Bit | /*RPCEmu_Debug*/ 0x56AC2)

extern int __swi(XRPCEmu_Debug) __swi_XRPCEmu_Debug(const char * s);

#endif /* RISCOS */

extern void
report_output(
    _In_z_      PCTSTR buffer)
{
    PCTSTR tail;
    BOOL may_need_newline;
    static const TCHARZ newline_buffer[2] = TEXT("\n");

#if RISCOS
    PCTSTR ptr;
    U8 ch;

    static int try_reporter = TRUE;
    static int try_rpcemu_report = TRUE;

    if(!g_report_enabled) return;

    if(try_reporter)
    {
        if(__swi_XReport_Text0(buffer) == (int) buffer)
        {   /* Reporter module always sticks a newline on all output */
            return;
        }
        try_reporter = FALSE;
    }

    tail = buffer + tstrlen32(buffer);
    may_need_newline = (tail == buffer) || (tail[-1] != '\n');

    if(try_rpcemu_report)
    {
        if(__swi_XRPCEmu_Debug(buffer) == (int) buffer)
        {
            if(!may_need_newline)
                return;

            if(__swi_XRPCEmu_Debug(newline_buffer) == (int) newline_buffer)
                return;
        }
        try_rpcemu_report = FALSE;
    }

#if defined(__CC_NORCROFT)
    if(0 == stderr->__file) /* not redirected? */
    {
        trace_disable();
        return;
    }
#endif

    ptr = buffer;

    while((ch = *ptr++) != CH_NULL)
        switch(ch)
        {
        case 0x0A: /* LF  */
        case 0x0D: /* CR  */
            fputc(0x0A, stderr);
            break;

        case 0x7F: /* DEL */
            fputc('|', stderr);
            fputc('?', stderr);
            break;

        default:
            if(ch <= 0x1F)
            {
                fputc('|', stderr);
                ch = ch + '@';
            }

            fputc(ch, stderr);
            break;
        }

    if(may_need_newline)
        fputc(0x0A, stderr);
#elif WINDOWS
    if(!g_report_enabled) return;

    tail = buffer + tstrlen32(buffer);
    may_need_newline = (tail == buffer) || (tail[-1] != '\n');

    OutputDebugString(buffer);

    if(may_need_newline)
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
extern PCTSTR
report_tstr(
    _In_opt_z_  PCTSTR tstr)
{
    if(NULL == tstr)
        return(TEXT("<<NULL>>"));

#if CHECKING
    if(IS_PTR_NONE_ANY(tstr))
        return(TEXT("<<NONE>>"));
#endif

    if(
#if defined(HIGH_MEMORY_LIMIT)
        ((uintptr_t) tstr < (uintptr_t) LOW_MEMORY_LIMIT ) ||
        ((uintptr_t) tstr > (uintptr_t) HIGH_MEMORY_LIMIT) ||
#endif
        IS_BAD_POINTER(tstr) )
    {
        static TCHARZ stringbuffer[16];
#if WINDOWS
        consume_int(_sntprintf_s(stringbuffer, elemof32(stringbuffer), _TRUNCATE, TEXT("<<") PTR_XTFMT TEXT(">>"), tstr));
#else /* C99 CRT */
        consume_int(snprintf(stringbuffer, elemof32(stringbuffer), TEXT("<<") PTR_XTFMT TEXT(">>"), tstr));
#endif
        return(stringbuffer);
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
    if(IS_PTR_NONE_ANY(ustr))
        return(TEXT("<<NONE>>"));
#endif

    if(
#if defined(HIGH_MEMORY_LIMIT)
        ((uintptr_t) ustr < (uintptr_t) LOW_MEMORY_LIMIT ) ||
        ((uintptr_t) ustr > (uintptr_t) HIGH_MEMORY_LIMIT) ||
#endif
        IS_BAD_POINTER(ustr) )
    {
        static TCHARZ stringbuffer[16];
#if WINDOWS
        consume_int(_sntprintf_s(stringbuffer, elemof32(stringbuffer), _TRUNCATE, TEXT("<<") PTR_XTFMT TEXT(">>"), ustr));
#else /* C99 CRT */
        consume_int(snprintf(stringbuffer, elemof32(stringbuffer), TEXT("<<") PTR_XTFMT TEXT(">>"), ustr));
#endif
        return(stringbuffer);
    }

    /* Always watch out for inlines in USTRs! */
    if(contains_inline(ustr, ustrlen32(ustr)))
    {
        assert0();
        return(TEXT("<<CONTAINS INLINES>>"));
    }

    return(_tstr_from_ustr(ustr));
}

_Check_return_
_Ret_z_
extern PCTSTR
report_wstr(
    _In_opt_z_  PCWSTR wstr)
{
    if(NULL == wstr)
        return(TEXT("<<NULL>>"));

#if CHECKING
    if(IS_PTR_NONE_ANY(wstr))
        return(TEXT("<<NONE>>"));
#endif

    if(
#if defined(HIGH_MEMORY_LIMIT)
        ((uintptr_t) wstr < (uintptr_t) LOW_MEMORY_LIMIT ) ||
        ((uintptr_t) wstr > (uintptr_t) HIGH_MEMORY_LIMIT) ||
#endif
        IS_BAD_POINTER(wstr) )
    {
        static TCHARZ stringbuffer[16];
#if WINDOWS
        consume_int(_sntprintf_s(stringbuffer, elemof32(stringbuffer), _TRUNCATE, TEXT("<<") PTR_XTFMT TEXT(">>"), wstr));
#else /* C99 CRT */
        consume_int(snprintf(stringbuffer, elemof32(stringbuffer), TEXT("<<") PTR_XTFMT TEXT(">>"), report_ptr_cast(wstr)));
#endif
        return(stringbuffer);
    }

    return(_tstr_from_wstr(wstr));
}

_Check_return_
_Ret_z_
extern PCTSTR
report_sbstr(
    _In_opt_z_  PC_SBSTR sbstr)
{
    if(NULL == sbstr)
        return(TEXT("<<NULL>>"));

#if CHECKING
    if(IS_PTR_NONE_ANY(sbstr))
        return(TEXT("<<NONE>>"));
#endif

    if(
#if defined(HIGH_MEMORY_LIMIT)
        ((uintptr_t) sbstr < (uintptr_t) LOW_MEMORY_LIMIT)  ||
        ((uintptr_t) sbstr > (uintptr_t) HIGH_MEMORY_LIMIT) ||
#endif
        IS_BAD_POINTER(sbstr) )
    {
        static TCHARZ stringbuffer[16];
#if WINDOWS
        consume_int(_sntprintf_s(stringbuffer, elemof32(stringbuffer), _TRUNCATE, TEXT("<<") PTR_XTFMT TEXT(">>"), sbstr));
#else /* C99 CRT */
        consume_int(snprintf(stringbuffer, elemof32(stringbuffer), TEXT("<<") PTR_XTFMT TEXT(">>"), sbstr));
#endif
        return(stringbuffer);
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
    case Wimp_ENull:                    return(TEXT("Wimp_ENull"));
    case Wimp_ERedrawWindow:            return(TEXT("Wimp_ERedrawWindow"));
    case Wimp_EOpenWindow:              return(TEXT("Wimp_EOpenWindow"));
    case Wimp_ECloseWindow:             return(TEXT("Wimp_ECloseWindow"));
    case Wimp_EPointerLeavingWindow:    return(TEXT("Wimp_EPointerLeavingWindow"));
    case Wimp_EPointerEnteringWindow:   return(TEXT("Wimp_EPointerEnteringWindow"));
    case Wimp_EMouseClick:              return(TEXT("Wimp_EMouseClick"));
    case Wimp_EUserDrag:                return(TEXT("Wimp_EUserDrag"));
    case Wimp_EKeyPressed:              return(TEXT("Wimp_EKeyPressed"));
    case Wimp_EMenuSelection:           return(TEXT("Wimp_EMenuSelection"));
    case Wimp_EScrollRequest:           return(TEXT("Wimp_EScrollRequest"));
    case Wimp_ELoseCaret:               return(TEXT("Wimp_ELoseCaret"));
    case Wimp_EGainCaret:               return(TEXT("Wimp_EGainCaret"));
    case Wimp_EPollWordNonZero:         return(TEXT("Wimp_EPollWordNonZero"));
    /* 14..16 not defined */
    case Wimp_EUserMessage:             return(TEXT("Wimp_EUserMessage"));
    case Wimp_EUserMessageRecorded:     return(TEXT("Wimp_EUserMessageRecorded"));
    case Wimp_EUserMessageAcknowledge:  return(TEXT("Wimp_EUserMessageAcknowledge"));

    default:
        consume_int(snprintf(default_eventstring, elemof32(default_eventstring), U32_XTFMT, (U32) event_code));
        return(default_eventstring);
    }
}

_Check_return_
_Ret_z_
extern PCTSTR
report_wimp_event(
    _InVal_     int event_code,
    _In_        const void * const p_event_data)
{
    const WimpPollBlock * const ed = (const WimpPollBlock * const) p_event_data;
    char tempbuffer[256];

    static TCHARZ messagebuffer[512];

    tstr_xstrkpy(messagebuffer, elemof32(messagebuffer), report_wimp_event_code(event_code));

    switch(event_code)
    {
    case Wimp_EOpenWindow:
        consume_int(
            snprintf(tempbuffer, elemof32(tempbuffer),
                     TEXT(": window " UINTPTR_XTFMT "; coords (") INT_TFMT TEXT(", ") INT_TFMT TEXT(", ") INT_TFMT TEXT(", ") INT_TFMT TEXT("); scroll (") INT_TFMT TEXT(", ") INT_TFMT TEXT("); behind ") UINTPTR_XTFMT,
                     ed->open_window_request.window_handle,
                     ed->open_window_request.visible_area.xmin, ed->open_window_request.visible_area.ymin,
                     ed->open_window_request.visible_area.xmax, ed->open_window_request.visible_area.ymax,
                     ed->open_window_request.xscroll,           ed->open_window_request.yscroll,
                     ed->open_window_request.behind));
        break;

    case Wimp_ERedrawWindow:
    case Wimp_ECloseWindow:
    case Wimp_EPointerLeavingWindow:
    case Wimp_EPointerEnteringWindow:
        consume_int(
            snprintf(tempbuffer, elemof32(tempbuffer),
                     TEXT(": window ") UINTPTR_XTFMT,
                     ed->close_window_request.window_handle));
        break;

    case Wimp_EMouseClick:
        consume_int(
            snprintf(tempbuffer, elemof32(tempbuffer),
                     TEXT(": window ") UINTPTR_XTFMT TEXT("; icon ") INT_TFMT TEXT("; mouse x ") INT_TFMT TEXT(" y ") INT_TFMT TEXT("; buttons ") INT_TFMT,
                     ed->mouse_click.window_handle,
                     ed->mouse_click.icon_handle,
                     ed->mouse_click.mouse_x, ed->mouse_click.mouse_y,
                     ed->mouse_click.buttons));
        break;

    case Wimp_EKeyPressed:
        consume_int(
            snprintf(tempbuffer, elemof32(tempbuffer),
                     TEXT(": window ") UINTPTR_XTFMT TEXT("; icon ") INT_TFMT TEXT("; x ") INT_TFMT TEXT(" y ") INT_TFMT TEXT("; height ") U32_XTFMT TEXT(" index ") INT_TFMT TEXT("; key_code ") INT_TFMT,
                     ed->key_pressed.caret.window_handle,
                     ed->key_pressed.caret.icon_handle,
                     ed->key_pressed.caret.xoffset, ed->key_pressed.caret.yoffset,
                     ed->key_pressed.caret.height,  ed->key_pressed.caret.index,
                     ed->key_pressed.key_code));
        break;

    case Wimp_EScrollRequest:
        consume_int(
            snprintf(tempbuffer, elemof32(tempbuffer),
                     TEXT(": window ") UINTPTR_XTFMT TEXT("; coords (") INT_TFMT TEXT(", ") INT_TFMT TEXT(", ") INT_TFMT TEXT(", ") INT_TFMT TEXT("); scroll (") INT_TFMT TEXT(", ") INT_TFMT TEXT("); dir'n (") INT_TFMT TEXT(", ") INT_TFMT TEXT(")"),
                     ed->scroll_request.open.window_handle,
                     ed->scroll_request.open.visible_area.xmin, ed->scroll_request.open.visible_area.ymin,
                     ed->scroll_request.open.visible_area.xmax, ed->scroll_request.open.visible_area.ymax,
                     ed->scroll_request.open.xscroll,           ed->scroll_request.open.yscroll,
                     ed->scroll_request.xscroll,                ed->scroll_request.yscroll));
        break;

    case Wimp_ELoseCaret:
    case Wimp_EGainCaret:
        consume_int(
            snprintf(tempbuffer, elemof32(tempbuffer),
                     TEXT(": window ") UINTPTR_XTFMT TEXT("; icon ") INT_TFMT TEXT(" x ") INT_TFMT TEXT(" y ") INT_TFMT TEXT("; height ") U32_XTFMT TEXT(" index ") INT_TFMT,
                     ed->lose_caret.window_handle,
                     ed->lose_caret.icon_handle,
                     ed->lose_caret.xoffset, ed->lose_caret.yoffset,
                     ed->lose_caret.height,  ed->lose_caret.index));

        break;

    case Wimp_EUserMessage:
    case Wimp_EUserMessageRecorded:
    case Wimp_EUserMessageAcknowledge:
        consume_int(
            snprintf(tempbuffer, elemof32(tempbuffer),
                     TEXT(": %s"), report_wimp_message(&ed->user_message, FALSE)));
        break;

    case Wimp_ENull:
    case Wimp_EUserDrag:
    case Wimp_EMenuSelection:

    default:
        *tempbuffer = CH_NULL;
        break;
    }

    if(*tempbuffer)
        tstr_xstrkat(messagebuffer, elemof32(messagebuffer), tempbuffer);

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
    case Wimp_MQuit:            return(TEXT("Wimp_MQuit"));
    case Wimp_MDataSave:        return(TEXT("Wimp_MDataSave"));
    case Wimp_MDataSaveAck:     return(TEXT("Wimp_MDataSaveAck"));
    case Wimp_MDataLoad:        return(TEXT("Wimp_MDataLoad"));
    case Wimp_MDataLoadAck:     return(TEXT("Wimp_MDataLoadAck"));
    case Wimp_MDataOpen:        return(TEXT("Wimp_MDataOpen"));
    case Wimp_MRAMFetch:        return(TEXT("Wimp_MRAMFetch"));
    case Wimp_MRAMTransmit:     return(TEXT("Wimp_MRAMTransmit"));
    case Wimp_MPreQuit:         return(TEXT("Wimp_MPreQuit"));
    case Wimp_MPaletteChange:   return(TEXT("Wimp_MPaletteChange"));
    case Wimp_MSaveDesktop:     return(TEXT("Wimp_MSaveDesktop"));
    /*Wimp_MDeviceClaim*/
    /*Wimp_MDeviceInUse*/
    case Wimp_MDataSaved:       return(TEXT("Wimp_MDataSaved"));
    case Wimp_MShutDown:        return(TEXT("Wimp_MShutDown"));
    /*Wimp_MClaimEntity*/
    /*Wimp_MDataRequest*/
    /*Wimp_MDragging*/
    /*Wimp_MDragClaim*/
    case Wimp_MAppControl:      return(TEXT("Wimp_MAppControl"));

    case Wimp_MHelpRequest:     return(TEXT("Wimp_MHelpRequest"));
    case Wimp_MHelpReply:       return(TEXT("Wimp_MHelpReply"));

    case Wimp_MMenuWarning:     return(TEXT("Wimp_MMenuWarning"));
    case Wimp_MModeChange:      return(TEXT("Wimp_MModeChange"));
    case Wimp_MTaskInitialise:  return(TEXT("Wimp_MTaskInitialise"));
    case Wimp_MTaskCloseDown:   return(TEXT("Wimp_MTaskCloseDown"));
    case Wimp_MSlotSize:        return(TEXT("Wimp_MSlotSize"));
    case Wimp_MSetSlot:         return(TEXT("Wimp_MSetSlot"));
    case Wimp_MTaskNameRq:      return(TEXT("Wimp_MTaskNameRq"));
    case Wimp_MTaskNameIs:      return(TEXT("Wimp_MTaskNameIs"));
    case Wimp_MTaskStarted:     return(TEXT("Wimp_MTaskStarted"));
    case Wimp_MMenusDeleted:    return(TEXT("Wimp_MMenusDeleted"));
    case Wimp_MIconize:         return(TEXT("Wimp_MIconize"));
    case Wimp_MWindowClosed:    return(TEXT("Wimp_MWindowClosed"));
    case Wimp_MWindowInfo:      return(TEXT("Wimp_MWindowInfo"));
    case Wimp_MSwap:            return(TEXT("Wimp_MSwap"));
    case Wimp_MToolsChanged:    return(TEXT("Wimp_MToolsChanged"));
    case Wimp_MFontChanged:     return(TEXT("Wimp_MFontChanged"));

    case Wimp_MPrintFile:       return(TEXT("Wimp_MPrintFile"));
    case Wimp_MWillPrint:       return(TEXT("Wimp_MWillPrint"));
    case Wimp_MPrintSave:       return(TEXT("Wimp_MPrintSave"));
    case Wimp_MPrintInit:       return(TEXT("Wimp_MPrintInit"));
    case Wimp_MPrintError:      return(TEXT("Wimp_MPrintError"));
    case Wimp_MPrintTypeOdd:    return(TEXT("Wimp_MPrintTypeOdd"));
    case Wimp_MPrintTypeKnown:  return(TEXT("Wimp_MPrintTypeKnown"));
    case Wimp_MSetPrinter:      return(TEXT("Wimp_MSetPrinter"));

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
    const WimpMessage * const m = (const WimpMessage * const) p_wimp_message;
    PCTSTR format =
        sending
            ? TEXT("%s, size ") INT_TFMT TEXT(", your_ref ") U32_XTFMT
            : TEXT("%s, size ") INT_TFMT TEXT(", your_ref ") U32_XTFMT TEXT(" from task " U32_XTFMT ", my(his)_ref ") U32_XTFMT;
    TCHARZ tempbuffer[256];

    static TCHARZ messagebuffer[512];

    consume_int(
        snprintf(messagebuffer, elemof32(messagebuffer), format,
                 report_wimp_message_action(m->hdr.action_code),
                 m->hdr.size, m->hdr.your_ref,
                 m->hdr.sender, m->hdr.my_ref));

    switch(m->hdr.action_code)
    {
    case Wimp_MDataLoadAck:
        if(m->hdr.size <= 44)
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
                     m->data.data_load.destination_window,
                     m->data.data_load.destination_icon,
                     m->data.data_load.destination_x, m->data.data_load.destination_y,
                     m->data.data_load.estimated_size,
                     m->data.data_load.file_type,
                     m->data.data_load.leaf_name));
        break;

    case Wimp_MRAMFetch:
        consume_int(
            snprintf(tempbuffer, elemof32(tempbuffer),
                     TEXT(": buffer ") PTR_XTFMT TEXT("; size ") U32_XTFMT,
                     m->data.ram_fetch.buffer,
                     m->data.ram_fetch.buffer_size));
        break;

    case Wimp_MRAMTransmit:
        consume_int(
            snprintf(tempbuffer, elemof32(tempbuffer),
                     TEXT(": buffer ") PTR_XTFMT TEXT("; nbytes ") U32_XTFMT,
                     m->data.ram_transmit.buffer,
                     m->data.ram_transmit.nbytes));
        break;

    case Wimp_MHelpRequest:
        consume_int(
            snprintf(tempbuffer, elemof32(tempbuffer),
                     TEXT(": window ") UINTPTR_XTFMT TEXT("; icon ") INT_TFMT TEXT("; mouse x ") INT_TFMT TEXT(" y ") INT_TFMT TEXT("; buttons ") INT_TFMT,
                     m->data.help_request.window_handle,
                     m->data.help_request.icon_handle,
                     m->data.help_request.mouse_x, m->data.help_request.mouse_y,
                     m->data.help_request.buttons));
        break;

    case Wimp_MHelpReply:
        xstrkpy(tempbuffer, elemof32(tempbuffer), ": ");
        xstrkat(tempbuffer, elemof32(tempbuffer), m->data.help_reply.text);
        break;

    default:
        *tempbuffer = CH_NULL;
        break;
    }

    if(*tempbuffer)
        tstr_xstrkat(messagebuffer, elemof32(messagebuffer), tempbuffer);

    return(messagebuffer);
}

#endif /* RISCOS */

/* end of report.c */
