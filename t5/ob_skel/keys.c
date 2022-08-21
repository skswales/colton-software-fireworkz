/* keys.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#if RISCOS
#define EXPOSE_RISCOS_SWIS 1

#include "ob_skel/xp_skelr.h"
#endif

#define MAX_KEY_EVENT_CACHE 24

static
struct KEY_EVENT_CACHE
{
    DOCNO docno;

    S32 fn_key;

    QUICK_UBLOCK_WITH_BUFFER(quick_ublock, (MAX_KEY_EVENT_CACHE + 1) + 1 /*CH_NULL*/);
}
key_event_cache =
{
    DOCNO_NONE,

    0
};

extern void
host_key_cache_init(void)
{
    quick_ublock_with_buffer_setup(key_event_cache.quick_ublock);
}

static void
host_key_cache_empty(void)
{
    /* empty our cache */
    quick_ublock_dispose(&key_event_cache.quick_ublock);

    key_event_cache.fn_key = 0;

    key_event_cache.docno = DOCNO_NONE;
}

extern void
host_key_buffer_flush(void)
{
    host_key_cache_empty();

#if RISCOS
    (void) _kernel_osbyte(15, 1 /*flush current input buffer*/, 0);
#else

#endif
}

_Check_return_
extern STATUS
host_key_cache_emit_events(void)
{
    return(host_key_cache_emit_events_for(DOCNO_NONE));
}

/******************************************************************************
*
* emit key events iff:
* docno is the same the one in the cache or
* docno == DOCNO_NONE
*
******************************************************************************/

_Check_return_
static STATUS
host_key_cache_emit_event_apply_style_handle(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     OBJECT_ID object_id,
    _InVal_     ARRAY_HANDLE h_commands)
{
    /* h_commands is the style_handle, n_events is irrelevant as ^F1 etc. to apply a style wouldn't want to be autorepeated */
    STATUS status = STATUS_OK;
    STYLE style;
    PC_CONSTRUCT_TABLE p_construct_table;
    ARGLIST_HANDLE arglist_handle;
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 32);
    quick_ublock_with_buffer_setup(quick_ublock);

    style_init(&style);

    style.style_handle = (STYLE_HANDLE) h_commands;
    style_bit_set(&style, STYLE_SW_HANDLE);

    if(status_ok(status = arglist_prepare_with_construct(&arglist_handle, object_id, T5_CMD_STYLE_APPLY, &p_construct_table)))
    {
        if(status_ok(status = style_ustr_inline_from_struct(p_docu, &quick_ublock, &style)))
        {
            const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 1);
            p_args[0].val.ustr_inline = quick_ublock_ustr_inline(&quick_ublock);
            status_consume(execute_command_reperr(p_docu, T5_CMD_STYLE_APPLY, &arglist_handle, object_id));
            quick_ublock_dispose(&quick_ublock);
        }

        arglist_dispose(&arglist_handle);
    }

    return(status);
}

_Check_return_
static STATUS
host_key_cache_emit_event_commands(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    _InVal_     OBJECT_ID object_id,
    _InVal_     ARRAY_HANDLE h_commands,
    _InVal_     U32 n_events)
{
    STATUS status = STATUS_OK;

    if(0 != h_commands)
    {
        /* execute set of commands n_events times. don't execute subsequent ones if any fails */
        const DOCNO docno = docno_from_p_docu(p_docu);
        U32 i;
        for(i = 0; i < n_events; ++i)
            status_break(status = command_array_handle_execute(docno, h_commands));
        if(status_fail(status))
            status = STATUS_OK; /* error already reported */
        return(status);
    }

    /* send all n_events raw commands at once e.g. arrow left */
    status = execute_command_n(p_docu, t5_message, object_id, n_events);

    if(status_fail(status))
    {
        if(status != STATUS_FAIL)
            reperr_null(status);

        status = STATUS_OK;
    }

    return(status);
}

_Check_return_
static STATUS
host_key_cache_emit_event_keys(
    _DocuRef_   P_DOCU p_docu)
{
    STATUS status = STATUS_OK;
    SKELEVENT_KEYS skelevent_keys;

    skelevent_keys.p_quick_ublock = &key_event_cache.quick_ublock;
    skelevent_keys.processed = 0;
    status = object_skel(p_docu, T5_EVENT_KEYS, &skelevent_keys);

    host_key_cache_empty();

    return(status);
}

_Check_return_
extern STATUS
host_key_cache_emit_events_for(
    _InVal_     DOCNO docno)
{
    P_DOCU p_docu;

    if(DOCNO_NONE == key_event_cache.docno)
        return(STATUS_OK);

    p_docu = p_docu_from_docno(key_event_cache.docno);

    if((DOCNO_NONE != docno) && (key_event_cache.docno != docno))
        return(STATUS_OK);

    if(key_event_cache.fn_key)
    {
        const U32 n_events = quick_ublock_bytes(&key_event_cache.quick_ublock);
        const OBJECT_ID object_id = OBJECT_ID_SKEL;
        T5_MESSAGE t5_message;
        ARRAY_HANDLE h_commands = command_array_handle_from_key_code(p_docu, (KMAP_CODE) key_event_cache.fn_key, &t5_message);

        host_key_cache_empty();

        if(t5_message == T5_CMD_STYLE_APPLY_STYLE_HANDLE)
            return(host_key_cache_emit_event_apply_style_handle(p_docu, object_id, h_commands));

        return(host_key_cache_emit_event_commands(p_docu, t5_message, object_id, h_commands, n_events));
    }

    return(host_key_cache_emit_event_keys(p_docu));
}

/******************************************************************************
*
* stick this key event in the cache
*
******************************************************************************/

_Check_return_
extern STATUS
host_key_cache_event(
    _InVal_     DOCNO docno,
    _InVal_     S32 keycode,
    _InVal_     BOOL fn_key,
    _InVal_     BOOL emit_if_none_in_buffer)
{
    STATUS status;
    P_U8 p_u8;

    if(0 == key_event_cache.quick_ublock.ub_static_buffer_size)
        quick_ublock_with_buffer_setup(key_event_cache.quick_ublock);

    if(0 != quick_ublock_bytes(&key_event_cache.quick_ublock))
    {
        STATUS emit = 0;

        if(key_event_cache.docno != docno)
        {
            /* iff keys were for another document emit them */
            emit = 1;

            trace_2(TRACE_APP_HOST, TEXT("key cache belongs to docno ") S32_TFMT TEXT(" not ") S32_TFMT TEXT(" -> emit"), key_event_cache.docno, docno);
        }
        else if(fn_key)
        {
            /* iff fn key and cache is not the same fn key then emit */
            emit = (key_event_cache.fn_key != keycode);

            if(emit)
            {
                if(key_event_cache.fn_key)
                {
                    trace_2(TRACE_APP_HOST, TEXT("key cache function key - cached ") U32_XTFMT TEXT(" not function key ") U32_XTFMT TEXT(" -> emit"), key_event_cache.fn_key, keycode);
                }
                else
                    trace_1(TRACE_APP_HOST, TEXT("key cache function key - cached normal keys not function key ") U32_XTFMT TEXT(" -> emit"), keycode);
            }
        }
        else if(key_event_cache.fn_key)
        {
            emit = 1;

            trace_2(TRACE_APP_HOST, TEXT("key cache function key - cached ") U32_XTFMT TEXT(" not normal key ") U32_XTFMT TEXT(" -> emit"), key_event_cache.fn_key, keycode);
        }

        if(emit)
            status_return(host_key_cache_emit_events());
    }

    key_event_cache.docno = docno;

    p_u8 = (P_U8) quick_ublock_extend_by(&key_event_cache.quick_ublock, 1, &status);
    PTR_ASSERT(p_u8);
    status_assert(status);

    if(fn_key)
    {
        key_event_cache.fn_key = keycode;

        trace_3(TRACE_OUT | TRACE_APP_HOST, TEXT("key cache function key ") U32_XTFMT TEXT(" for docno ") S32_TFMT TEXT(", now size ") S32_TFMT, key_event_cache.fn_key, key_event_cache.docno, quick_ublock_bytes(&key_event_cache.quick_ublock));
    }
    else
    {
        /* only real keys are stored on list in cache */
        *p_u8 = (U8) keycode;
        trace_3(TRACE_APP_HOST, TEXT("key cache normal key ") U32_XTFMT TEXT(" for docno ") S32_TFMT TEXT(", now size ") S32_TFMT, *p_u8, key_event_cache.docno, quick_ublock_bytes(&key_event_cache.quick_ublock));
    }

    if((quick_ublock_bytes(&key_event_cache.quick_ublock) > MAX_KEY_EVENT_CACHE)
    || (emit_if_none_in_buffer && !host_keys_in_buffer()) )
    {
        /* emit cache if 'too many' keys buffered or no keys just over the horizon */
        assert(quick_ublock_bytes(&key_event_cache.quick_ublock) <= (MAX_KEY_EVENT_CACHE + 1));
        trace_v0(TRACE_APP_HOST, (quick_ublock_bytes(&key_event_cache.quick_ublock) > MAX_KEY_EVENT_CACHE)
                ? TEXT("key cache MAX_KEY_EVENT_CACHE exceeded -> emit")
                : TEXT("key cache no keys in buffer -> emit"));

        status_return(host_key_cache_emit_events());
    }

    return(STATUS_OK);
}

/******************************************************************************
*
* detect whether there are keys buffered
*
* RISC OS: note that these can be for ANY task, not just ours
* WINDOWS: ask specifically about key messages for our task
*
******************************************************************************/

_Check_return_
extern BOOL
host_keys_in_buffer(void)
{
#if RISCOS
    _kernel_swi_regs rs;
    int carry;

#if 0 /* only needed if we will do ReadC in an escape enabled state */
    (void) _kernel_swi_c(OS_ReadEscapeState, &rs, &rs, &carry); /* When we do get() we'll get ESC */

    if(carry != 0)
    {
        trace_0(TRACE_OUT | TRACE_ANY, TEXT("host_keys_in_buffer returns TRUE due to ESCAPE set"));
        return(TRUE);
    }
#endif

    rs.r[0] = 152;      /* Examine keyboard buffer */
    rs.r[1] = 0;
    rs.r[2] = 0;
    (void) _kernel_swi_c(OS_Byte, &rs, &rs, &carry);

#if TRACE_ALLOWED
    if(carry == 0)
    {
        trace_0(TRACE_OUT | TRACE_RISCOS_HOST, TEXT("host_keys_in_buffer returns TRUE"));
    }
#endif

    return(carry == 0);
#elif WINDOWS
    MSG msg;

    if(PeekMessage(&msg, 0, WM_KEYDOWN, WM_KEYDOWN, PM_NOREMOVE | PM_NOYIELD))
        return(1);

    return(0);
#endif
}

#define CTRL_H ('H' & 0x1F)    /* Backspace key */
#define CTRL_M ('M' & 0x1F)    /* Return/Enter keys */
#define CTRL_Z ('Z' & 0x1F)    /* Last Ctrl+x key */

#if RISCOS

_Check_return_
static KMAP_CODE
cs_mutate_key(
    KMAP_CODE kmap_code)
{
    const KMAP_CODE shift_added = host_shift_pressed() ? KMAP_CODE_ADDED_SHIFT : 0;
    const KMAP_CODE ctrl_added  = host_ctrl_pressed()  ? KMAP_CODE_ADDED_CTRL  : 0;

#if CHECKING
    switch(kmap_code)
    {
    case KMAP_FUNC_DELETE:
    case KMAP_FUNC_HOME:
    case KMAP_FUNC_BACKSPACE:
    case KMAP_FUNC_RETURN:
        break;

    default: default_unhandled();
        break;
    }
#endif

    kmap_code |= (shift_added | ctrl_added);

    return(kmap_code);
}

_Check_return_
static KMAP_CODE
convert_ctrl_key(
    _InVal_     int ch_chode)
{
    KMAP_CODE kmap_code;

    if(ch_chode <= CTRL_Z)
    {
        kmap_code = (ch_chode - 1 + 'A') | KMAP_CODE_ADDED_ALT; /* Uppercase Alt-letter by default */

        /* Watch out for useful CtrlChars not produced by Ctrl+letter */

        /* SKS after PD 4.11 10jan92 - if already in Alt-sequence then these go in as Alt-letters
         * without considering Ctrl-state (allows ^BM, ^PHB etc. from !Chars but not ^M, ^H still)
        */
        if(ch_chode == CTRL_M)
            kmap_code = RISCOS_EKEY_RETURN;
        else if((ch_chode == CTRL_H) && !host_ctrl_pressed())
            kmap_code = RISCOS_EKEY_BACKSPACE;
    }
    else
    {
        kmap_code = ch_chode;
    }

    /* transform simply produced Return (some of), Delete, Home and Backspace keys into function-like keys */
    switch(kmap_code)
    {
    default:
        if(kmap_code & KMAP_CODE_ADDED_ALT)
            if(host_shift_pressed())
                kmap_code |= KMAP_CODE_ADDED_SHIFT;
        break;

    case RISCOS_EKEY_DELETE:
        kmap_code = cs_mutate_key(KMAP_FUNC_DELETE);
        break;

    case RISCOS_EKEY_HOME:
        kmap_code = cs_mutate_key(KMAP_FUNC_HOME);
        break;

    case RISCOS_EKEY_BACKSPACE:
        kmap_code = cs_mutate_key(KMAP_FUNC_BACKSPACE);
        break;

    case RISCOS_EKEY_RETURN:
        kmap_code = cs_mutate_key(KMAP_FUNC_RETURN);
        break;
    }

    return(kmap_code);
}

/* convert RISC OS Fn keys to our internal representations */

_Check_return_
static KMAP_CODE
convert_function_key(
    _InVal_     int ch_chode /*from Key_Pressed*/)
{
    KMAP_CODE kmap_code = ch_chode ^ 0x180; /* map RISC OS F0 (0x180) to 0x00, F10 (0x1CA) to 0x4A etc. */

    KMAP_CODE shift_added = 0;
    KMAP_CODE ctrl_added  = 0;

    /* remap RISC OS shift and control bits to our definitions */
    if(0 != (kmap_code & 0x10))
    {
        kmap_code ^= 0x10;
        shift_added = KMAP_CODE_ADDED_SHIFT;
    }

    if(0 != (kmap_code & 0x20))
    {
        kmap_code ^= 0x20;
        ctrl_added = KMAP_CODE_ADDED_CTRL;
    }

    /* map F10-F15 range onto end of F0-F9 range ... */
    if((kmap_code >= 0x4A) && (kmap_code <= 0x4F))
    {
        kmap_code ^= (0x40 | KMAP_BASE_FUNC);
    }
    /* ... and map TAB-up arrow out of that range */
    else if((kmap_code >= 0x0A) && (kmap_code <= 0x0F))
    {
        kmap_code ^= (0x00 | KMAP_BASE_FUNC2);
    }
    else
    {
        kmap_code ^= KMAP_BASE_FUNC;
    }

    /* SKS 18aug94 distinguish between shift-arrow and page up/down 'cos Window Manager doesn't */
    if((kmap_code == KMAP_FUNC_ARROW_UP) || (kmap_code == KMAP_FUNC_ARROW_DOWN))
    {
        if(shift_added && !ctrl_added)
        {
            if(!host_shift_pressed())
            {
                kmap_code = (kmap_code == KMAP_FUNC_ARROW_UP) ? KMAP_FUNC_PAGE_UP : KMAP_FUNC_PAGE_DOWN;
                shift_added = 0;
            }
        }
    }

    kmap_code |= (shift_added | ctrl_added);

    return(kmap_code);
}

_Check_return_
static KMAP_CODE
convert_standard_key(
    _InVal_     int ch_chode /*from Key_Pressed*/)
{
    KMAP_CODE kmap_code;

    if((ch_chode < 0x20) || (ch_chode == RISCOS_EKEY_DELETE))
    {
        kmap_code = convert_ctrl_key(ch_chode); /* RISC OS 'Hot Key' here we come */
    }
    else
    {
        kmap_code = ch_chode;
    }

    return(kmap_code);
}

/* Translate key from RISC OS Window manager into what application expects */

_Check_return_
extern KMAP_CODE
kmap_convert(
    _InVal_     int ch_chode /*from Key_Pressed*/)
{
    KMAP_CODE kmap_code;

    trace_1(0, TEXT("kmap_convert: ch_chode in ") U32_XTFMT, (U32) ch_chode);

    switch(ch_chode & ~0x000000FF)
    {
    /* 'normal' chars in 0..255 */
    case 0x0000:
        kmap_code = convert_standard_key(ch_chode);
        break;

    /* convert RISC OS Fn keys to our internal representations */
    case 0x0100:
        kmap_code = convert_function_key(ch_chode);
        break;

    default: default_unhandled();
        kmap_code = 0;
        break;
    }

    trace_1(0, TEXT("kmap_convert: kmap_code out ") U32_XTFMT, (U32) kmap_code);

    return(kmap_code);
}

#elif WINDOWS

_Check_return_
extern KMAP_CODE
kmap_convert(
    _InVal_     UINT vk)
{
    KMAP_CODE kmap_code = (KMAP_CODE) vk;
    KMAP_CODE shift_added = 0;
    KMAP_CODE ctrl_added  = 0;
    const BOOL shift_pressed = host_shift_pressed();
    const BOOL ctrl_pressed  = host_ctrl_pressed();

    trace_1(0, TEXT("kmap_convert: key in ") U32_XTFMT, (U32) vk);

    switch(vk)
    {
    case VK_ESCAPE:
        kmap_code = KMAP_ESCAPE;
        break;

    case VK_RETURN:
        kmap_code = KMAP_FUNC_RETURN;
        if(shift_pressed)
            shift_added = KMAP_CODE_ADDED_SHIFT;
        if(ctrl_pressed)
            ctrl_added = KMAP_CODE_ADDED_CTRL;
        break;

    /* Page up/down sequences */
    case VK_PRIOR:
        kmap_code = KMAP_FUNC_PAGE_UP;
        if(shift_pressed)
            shift_added = KMAP_CODE_ADDED_SHIFT;
        break;
    case VK_NEXT:
        kmap_code = KMAP_FUNC_PAGE_DOWN;
        if(shift_pressed)
            shift_added = KMAP_CODE_ADDED_SHIFT;
        break;

    case VK_TAB:
        kmap_code = KMAP_FUNC_TAB;
        if(shift_pressed)
            shift_added = KMAP_CODE_ADDED_SHIFT;
        if(ctrl_pressed)
            ctrl_added = KMAP_CODE_ADDED_CTRL;
        break;

    /* End/home */
    case VK_HOME:
        kmap_code = KMAP_FUNC_HOME;
        if(shift_pressed)
            shift_added = KMAP_CODE_ADDED_SHIFT;
        if(ctrl_pressed)
            ctrl_added = KMAP_CODE_ADDED_CTRL;
        break;
    case VK_END:
        kmap_code = KMAP_FUNC_END;
        if(shift_pressed)
            shift_added = KMAP_CODE_ADDED_SHIFT;
        if(ctrl_pressed)
            ctrl_added = KMAP_CODE_ADDED_CTRL;
        break;

    case VK_INSERT:
        kmap_code = KMAP_FUNC_INSERT;
        if(shift_pressed)
            shift_added = KMAP_CODE_ADDED_SHIFT;
        if(ctrl_pressed)
            ctrl_added = KMAP_CODE_ADDED_CTRL;
        break;

    /* Delete cases */
    case VK_DELETE:
        kmap_code = KMAP_FUNC_DELETE;
        if(shift_pressed)
            shift_added = KMAP_CODE_ADDED_SHIFT;
        if(ctrl_pressed)
            ctrl_added = KMAP_CODE_ADDED_CTRL;
        break;

    case VK_BACK:
        kmap_code = KMAP_FUNC_BACKSPACE;
        if(shift_pressed)
            shift_added = KMAP_CODE_ADDED_SHIFT;
        if(ctrl_pressed)
            ctrl_added = KMAP_CODE_ADDED_CTRL;
        break;

    /* Cursor key grobbling */
    case VK_LEFT:
        kmap_code = KMAP_FUNC_ARROW_LEFT;
        if(shift_pressed)
            shift_added = KMAP_CODE_ADDED_SHIFT;
        if(ctrl_pressed)
            ctrl_added = KMAP_CODE_ADDED_CTRL;
        break;

    case VK_RIGHT:
        kmap_code = KMAP_FUNC_ARROW_RIGHT;
        if(shift_pressed)
            shift_added = KMAP_CODE_ADDED_SHIFT;
        if(ctrl_pressed)
            ctrl_added = KMAP_CODE_ADDED_CTRL;
        break;

    case VK_UP:
        kmap_code = KMAP_FUNC_ARROW_UP;
        if(shift_pressed)
            shift_added = KMAP_CODE_ADDED_SHIFT;
        if(ctrl_pressed)
            ctrl_added = KMAP_CODE_ADDED_CTRL;
        break;

    case VK_DOWN:
        kmap_code = KMAP_FUNC_ARROW_DOWN;
        if(shift_pressed)
            shift_added = KMAP_CODE_ADDED_SHIFT;
        if(ctrl_pressed)
            ctrl_added = KMAP_CODE_ADDED_CTRL;
        break;

    /* Now check for sensible character ranges */
    default:
        if((vk >= VK_F1) && (vk <= VK_F15))
        {
            kmap_code = ((KMAP_CODE) vk - VK_F1) + ((KMAP_BASE_FUNC + 1) | KMAP_CODE_ADDED_FN);
            if(shift_pressed)
                shift_added = KMAP_CODE_ADDED_SHIFT;
            if(ctrl_pressed)
                ctrl_added = KMAP_CODE_ADDED_CTRL;
        }
        else
        {
            kmap_code = 0; /* unconverted, unless... */

            if(ctrl_pressed)
                if((vk >= 'A') && (vk <= 'Z')) /* WM_KEYthing messages are all uppercase VK_s */
                {
                    kmap_code = (KMAP_CODE) vk | KMAP_CODE_ADDED_ALT;
                    if(shift_pressed)
                        shift_added = KMAP_CODE_ADDED_SHIFT;
                }
        }

        break;
    }

    trace_1(0, TEXT("kmap_convert: kmap_code out ") U32_XTFMT, (U32) (kmap_code | shift_added | ctrl_added));

    return(kmap_code | shift_added | ctrl_added);
}

#endif /* OS */

#if RISCOS

_Check_return_
static int
host_is_key_pressed(
    _In_        int x)
{
    int r = _kernel_osbyte(0x81, x, 255);

    return((r & 0x0000FFFF) == 0x0000FFFF); /* X and Y both 255? */
}

_Check_return_
extern BOOL
host_shift_pressed(void)
{
    return(host_is_key_pressed(-1));
}

_Check_return_
extern BOOL
host_ctrl_pressed(void)
{
    return(host_is_key_pressed(-2));
}

#elif WINDOWS

_Check_return_
extern BOOL
host_shift_pressed(void)
{
    return(GetKeyState(VK_SHIFT) < 0);
}

_Check_return_
extern BOOL
host_ctrl_pressed(void)
{
    return(GetKeyState(VK_CONTROL) < 0);
}

#endif /* RISCOS */

/* end of keys.c */
