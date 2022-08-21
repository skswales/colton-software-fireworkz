/* riscos/colourpick.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2014-2018 Stuart Swales */

/* RISC OS specific colour picker routines for Fireworkz */

/* See RISC OS PRM 5a-555 onwards */

#include "common/gflags.h"

#if RISCOS

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#define EXPOSE_RISCOS_FLEX 1
#define EXPOSE_RISCOS_SWIS 1

#include "ob_skel/xp_skelr.h"

/*
internal types
*/

enum COLOUR_MODEL
{
    COLOUR_MODEL_RGB  = 0,
    COLOUR_MODEL_CMYK = 1,
    COLOUR_MODEL_HSV  = 2
};

typedef struct COLOUR_DESCRIPTOR
{
    /* 0*/  U8 zero;
    /* 1*/  U8 red;
    /* 2*/  U8 green;
    /* 3*/  U8 blue;

    /* 4*/  U32 extra_bytes; /* 0 or 4+3*4 for RGB/HSV, 4+4*4 for CMYK */

    /* 8*/  enum COLOUR_MODEL colour_model;

    /*12*/  
    union COLOUR_MODEL_DATA
    {
        struct COLOUR_MODEL_DATA_RGB
        {
            /* 0*/  U32 red;
            /* 4*/  U32 green;
            /* 8*/  U32 blue;
        } rgb;

        struct COLOUR_MODEL_DATA_CMYK
        {
            /* 0*/  U32 cyan;
            /* 4*/  U32 magenta;
            /* 8*/  U32 yellow;
            /*12*/  U32 key;
        } cmyk;

        struct COLOUR_MODEL_DATA_HSV
        {
            /* 0*/  U32 hue;
            /* 4*/  U32 saturation;
            /* 8*/  U32 value;
        } hsv;
    } data;
}
COLOUR_DESCRIPTOR, * P_COLOUR_DESCRIPTOR; typedef const COLOUR_DESCRIPTOR * PC_COLOUR_DESCRIPTOR;

enum COLOUR_PICKER_TYPE
{
    COLOUR_PICKER_NORMAL  = 0,
    COLOUR_PICKER_MENU    = 1,
    COLOUR_PICKER_TOOLBOX = 2,
    COLOUR_PICKER_SUBMENU = 3
};

typedef struct COLOUR_PICKER_BLOCK
{
    /* 0*/  int flags;
    /* 4*/  const char * title;
    /* 8*/  int tl_x;
    /*12*/  U32 reserved_80000000;
    /*16*/  U32 reserved_7FFFFFFF;
    /*20*/  int tl_y;
    /*24*/  int reserved_0[2];
    /*32*/  COLOUR_DESCRIPTOR colour_descriptor;
}
COLOUR_PICKER_BLOCK, * P_COLOUR_PICKER_BLOCK;

static COLOUR_PICKER_BLOCK colour_picker_block;

typedef struct COLOURPICKER_CALLBACK
{
    wimp_w parent_window_handle;
    RGB rgb;

    int dialogue_handle;
    wimp_w window_handle;
}
COLOURPICKER_CALLBACK, * P_COLOURPICKER_CALLBACK;

static void
colourpicker_close_dialogue(
    P_COLOURPICKER_CALLBACK p_colourpicker_callback);

/*
mostly no processing to do in this event handler - all are handled by ColourPicker module using Wimp filters
*/

/* this one is an exception - event being issued by winx layers when we are a child and our parent is closing */

_Check_return_
static int
colourpicker_event_close_window(
    P_COLOURPICKER_CALLBACK p_colourpicker_callback,
    _In_        const WimpCloseWindowRequestEvent * const p_close_window_request)
{
    UNREFERENCED_PARAMETER_InRef_(p_close_window_request);

    colourpicker_close_dialogue(p_colourpicker_callback);

    return(TRUE /*processed*/);
}

_Check_return_
static int
colourpicker_event_handler(
    _InVal_     int event_code,
    _In_        const WimpPollBlock * const p_event_data,
    P_ANY handle)
{
    reportf(/*trace_3(TRACE_RISCOS_HOST,*/ TEXT("%s: %s handle=") PTR_XTFMT, __Tfunc__, report_wimp_event(event_code, p_event_data), handle);

    UNREFERENCED_PARAMETER_InRef_(p_event_data);

    switch(event_code)
    {
    case Wimp_ECloseWindow:
        return(colourpicker_event_close_window((P_COLOURPICKER_CALLBACK) handle, &p_event_data->close_window_request));

    default:
        break;
    }

    return(TRUE /*processed*/); /* all are handled by ColourPicker module using Wimp filters */
}

#define Wimp_MColourPickerColourChoice         0x47700
#define Wimp_MColourPickerColourChanged        0x47701
#define Wimp_MColourPickerCloseDialogueRequest 0x47702
#define Wimp_MColourPickerOpenParentRequest    0x47703
#define Wimp_MColourPickerResetColourRequest   0x47704

static BOOL
colourpicker_message_ColourPickerColourChoice(
    _InRef_     PC_WimpMessage p_wimp_message,
    P_COLOURPICKER_CALLBACK p_colourpicker_callback)
{
    const PC_COLOUR_DESCRIPTOR p_colour_descriptor = (PC_COLOUR_DESCRIPTOR) &p_wimp_message->data.words[2];

    assert(p_colourpicker_callback->dialogue_handle == p_wimp_message->data.words[0]);

    p_colourpicker_callback->rgb.transparent = u8_from_int(0 != (p_wimp_message->data.words[1] & 1));

    p_colourpicker_callback->rgb.r = p_colour_descriptor->red;
    p_colourpicker_callback->rgb.g = p_colour_descriptor->green;
    p_colourpicker_callback->rgb.b = p_colour_descriptor->blue;

    return(TRUE);
}

static BOOL
colourpicker_message_ColourPickerCloseDialogueRequest(
    _InRef_     PC_WimpMessage p_wimp_message,
    P_COLOURPICKER_CALLBACK p_colourpicker_callback)
{
    UNREFERENCED_PARAMETER_InRef_(p_wimp_message);

    assert(p_colourpicker_callback->dialogue_handle == p_wimp_message->data.words[0]);

    colourpicker_close_dialogue(p_colourpicker_callback);

    return(TRUE);
}

static BOOL
colourpicker_message_ColourPickerResetColourRequest(
    _InRef_     PC_WimpMessage p_wimp_message,
    P_COLOURPICKER_CALLBACK p_colourpicker_callback)
{
    UNREFERENCED_PARAMETER_InRef_(p_wimp_message);

    assert(p_colourpicker_callback->dialogue_handle == p_wimp_message->data.words[0]);

    {
    _kernel_swi_regs rs;
    _kernel_oserror * e;

    rs.r[0] = 0
        | (1 << 6) /* update from RGB triplet */;
    rs.r[1] = p_colourpicker_callback->dialogue_handle;
    rs.r[2] = (int) &colour_picker_block;
    e = WrapOsErrorReporting(_kernel_swi(0x47704 /*ColourPicker_UpdateDialogue*/, &rs, &rs));
    } /*block*/

    return(TRUE);
}

static BOOL
colourpicker_message(
    _InRef_     PC_WimpMessage p_wimp_message,
    P_COLOURPICKER_CALLBACK p_colourpicker_callback)
{
    switch(p_wimp_message->hdr.action_code)
    {
    case Wimp_MColourPickerColourChoice:
        return(colourpicker_message_ColourPickerColourChoice(p_wimp_message, p_colourpicker_callback));

    case Wimp_MColourPickerCloseDialogueRequest:
        return(colourpicker_message_ColourPickerCloseDialogueRequest(p_wimp_message, p_colourpicker_callback));

    case Wimp_MColourPickerResetColourRequest:
        return(colourpicker_message_ColourPickerResetColourRequest(p_wimp_message, p_colourpicker_callback));

    default:
        return(FALSE); /* we don't want it - pass it on to the real handler */
    }
}

/* the list of messages that we are specifically interested in */

static const int
colourpicker_wimp_messages[] =
{
    Wimp_MColourPickerColourChoice,
    Wimp_MColourPickerCloseDialogueRequest,
    Wimp_MColourPickerResetColourRequest,
    0
};

static BOOL
colourpicker_message_filter(
    _InVal_     int event_code,
    _In_        const WimpPollBlock * const p_event_data,
    CLIENT_HANDLE client_handle)
{
    switch(event_code)
    {
    case Wimp_EUserMessage:
    case Wimp_EUserMessageRecorded:
        reportf(/*trace_2(TRACE_RISCOS_HOST,*/ TEXT("%s: %s"), __Tfunc__, report_wimp_event(event_code, p_event_data));
        return(colourpicker_message(&p_event_data->user_message, (P_COLOURPICKER_CALLBACK) client_handle));

    default:
        return(FALSE); /* we don't want it - pass it on to the real handler */
    }
}

static BOOL
colourpicker_open_dialogue(
    P_COLOURPICKER_CALLBACK p_colourpicker_callback)
{
    _kernel_swi_regs rs;
    _kernel_oserror * e;

    rs.r[0] = (int) COLOUR_PICKER_NORMAL;
    rs.r[1] = (int) &colour_picker_block;

    if(NULL != (e = WrapOsErrorReporting(_kernel_swi(0x47702 /*ColourPicker_OpenDialogue*/, &rs, &rs))))
    {
        p_colourpicker_callback->dialogue_handle = 0;
        p_colourpicker_callback->window_handle = 0;
        return(FALSE);
    }

    p_colourpicker_callback->dialogue_handle = rs.r[0];
    p_colourpicker_callback->window_handle = rs.r[1];

    reportf("got colourpicker window_handle " UINTPTR_XTFMT, p_colourpicker_callback->window_handle);

    winx_register_event_handler(p_colourpicker_callback->window_handle, BAD_WIMP_I, colourpicker_event_handler, p_colourpicker_callback);
    winx_register_child_window(p_colourpicker_callback->parent_window_handle, p_colourpicker_callback->window_handle, FALSE); /* send opens via queue */

    return(TRUE);
}

static BOOL
colourpicker_is_open(
    P_COLOURPICKER_CALLBACK p_colourpicker_callback)
{
    return(p_colourpicker_callback->window_handle > 0);
}

static void
colourpicker_process_dialogue(
    P_COLOURPICKER_CALLBACK p_colourpicker_callback)
{
    host_message_filter_register(colourpicker_message_filter, (CLIENT_HANDLE) p_colourpicker_callback);

    host_message_filter_add(colourpicker_wimp_messages);

    do { (void) wm_event_get(FALSE); } while(colourpicker_is_open(p_colourpicker_callback));

    host_message_filter_remove(colourpicker_wimp_messages);

    host_message_filter_register(NULL, (CLIENT_HANDLE) 0);
}

static void
colourpicker_close_dialogue(
    P_COLOURPICKER_CALLBACK p_colourpicker_callback)
{
    if(p_colourpicker_callback->window_handle > 0)
    {
        winx_deregister_child_window(p_colourpicker_callback->window_handle);
        winx_register_event_handler(p_colourpicker_callback->window_handle, BAD_WIMP_I, NULL, NULL);
        /* window will actually be destroyed in the ColourPicker_CloseDialogue SWI */
        p_colourpicker_callback->window_handle = 0;
    }

    if(p_colourpicker_callback->dialogue_handle)
    {
        _kernel_swi_regs rs;
        _kernel_oserror * e;
        rs.r[0] = 0;
        rs.r[1] = p_colourpicker_callback->dialogue_handle;
        e = WrapOsErrorReporting(_kernel_swi(0x47703 /*ColourPicker_CloseDialogue*/, &rs, &rs));
        p_colourpicker_callback->dialogue_handle = 0;
    }
}

extern BOOL
riscos_colour_picker(
    HOST_WND    parent_window_handle,
    _InoutRef_  P_RGB p_rgb)
{
    COLOURPICKER_CALLBACK colourpicker_callback;
    P_COLOURPICKER_CALLBACK p_colourpicker_callback = &colourpicker_callback;

    zero_struct(colourpicker_callback);
    colourpicker_callback.parent_window_handle = parent_window_handle;
    colourpicker_callback.rgb = *p_rgb;

colour_picker_block.flags = 0;
colour_picker_block.title = "Colour";

    { /* obtain current parent window position */
    WimpGetWindowStateBlock window_state;
    int parx, pary;

    window_state.window_handle = parent_window_handle;
    void_WrapOsErrorReporting(wimp_get_window_state(&window_state));
    parx = window_state.visible_area.xmin;
    pary = window_state.visible_area.ymax;

colour_picker_block.tl_x = parx + 108;
colour_picker_block.reserved_80000000 = 0x80000000U;
colour_picker_block.reserved_7FFFFFFF = 0x7FFFFFFFU;
colour_picker_block.tl_y = pary - 64;
    } /*block*/

colour_picker_block.reserved_0[0] = 0;
colour_picker_block.reserved_0[1] = 0;

colour_picker_block.colour_descriptor.zero = 0;
colour_picker_block.colour_descriptor.red = p_colourpicker_callback->rgb.r;
colour_picker_block.colour_descriptor.green = p_colourpicker_callback->rgb.g;
colour_picker_block.colour_descriptor.blue = p_colourpicker_callback->rgb.b;
colour_picker_block.colour_descriptor.extra_bytes = 0;
colour_picker_block.colour_descriptor.colour_model = COLOUR_MODEL_RGB;

    if(!colourpicker_open_dialogue(p_colourpicker_callback))
        return(FALSE);

    colourpicker_process_dialogue(p_colourpicker_callback);

    colourpicker_close_dialogue(p_colourpicker_callback);

reportf("RGB out: %d,%d,%d(%d)", colourpicker_callback.rgb.r, colourpicker_callback.rgb.g, colourpicker_callback.rgb.b, colourpicker_callback.rgb.transparent);
    *p_rgb = colourpicker_callback.rgb;
    return(TRUE);
}

#endif /* RISCOS */

/* end of colourpick.c */
