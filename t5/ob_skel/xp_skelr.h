/* xp_skelr.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Defines the RISC OS specific exports for ob_skel */

/* Do not include unless you want a whole host of very RISC OS specific stuff */

#ifndef __xp_skelr_h
#define __xp_skelr_h

#if RISCOS

/* #define EXPOSE_RISCOS_xxx to expose various bits of RISC OS interfaces */

#ifndef __xp_skel_h /* e.g. when used directly from WindLibC */
#define ARRAY_HANDLE unsigned int
#define HOST_FONT int
#endif

#ifndef __kernel_h
__pragma(warning(push)) __pragma(warning(disable:4255)) /* no function prototype given: converting () to (void) */
#include "kernel.h" /* C: */
__pragma(warning(pop))
#endif

#ifndef __os_h
#define __os_h /* Fireworkz does not directly use os.h so suppress it (WindLibC will have included it already) */
#endif

#define os_error _kernel_oserror /* doesn't matter whether typedef'd or #define'd by os.h */

#ifdef EXPOSE_RISCOS_SWIS

#ifndef __swis_h
#include "swis.h" /* C: */
#endif

#endif /* EXPOSE_RISCOS_SWIS */

#if defined(__xp_skel_h)

_Check_return_
static inline STATUS
create_error_from_kernel_oserror(
    _In_        const _kernel_oserror * const e)
{
    return(create_error_from_tstr(e->errmess));
}

#endif /* __xp_skel_h */

#ifndef                   __filetype_h
#include "cmodules/coltsoft/filetype.h"
#endif

#ifndef          __riscos_osfile_h
#include "cmodules/riscos/osfile.h"
#endif

#ifdef EXPOSE_RISCOS_BBC

#if CROSS_COMPILE
#include "bbc.h" /* csnf...RISC_OSLib */
#else
#include "RISC_OSLib:bbc.h" /* RISC_OSLib: */
#endif

#endif /* EXPOSE_RISCOS_BBC */

#define bbc_move(x, y) \
    os_plot(0x00 /*bbc_SolidBoth*/ + 4 /*bbc_MoveCursorAbs*/, x, y)

#if defined(NORCROFT_INLINE_SWIX)
_Check_return_
_Ret_maybenull_
static inline _kernel_oserror *
os_writeN(
    _In_reads_(count) const char * s,
    _InVal_     U32 count)
{
    return(
        _swix(OS_WriteN, _INR(0, 1),
        /*in*/  s, count) );
}
#else
_Check_return_
_Ret_maybenull_
extern _kernel_oserror *
os_writeN(
    _In_reads_(count) const char * s,
    _InVal_     U32 count);
#endif

#if defined(NORCROFT_INLINE_SWIX)
_Check_return_
_Ret_maybenull_
static inline _kernel_oserror *
os_plot(
    _InVal_     int code,
    _InVal_     int x,
    _InVal_     int y)
{
    return(
        _swix(OS_Plot, _INR(0, 2),
        /*in*/  code, x, y) );
}
#else
_Check_return_
_Ret_maybenull_
extern _kernel_oserror *
os_plot(
    _InVal_     int code,
    _InVal_     int x,
    _InVal_     int y);
#endif

_Check_return_
_Ret_maybenull_
extern _kernel_oserror *
riscos_vdu_define_graphics_window(
    _In_        int x1,
    _In_        int y1,
    _In_        int x2,
    _In_        int y2);

extern void
riscos_hourglass_off(void);

extern void
riscos_hourglass_on(void);

extern void
riscos_hourglass_percent(
    _In_        S32 percent);

extern void
riscos_hourglass_smash(void);

extern void /* don't use - hourglass is set up automatically by wimp_poll_coltsoft() */
riscos_hourglass_start(
    _In_        int after_cs);

#ifndef __wimp_h
__pragma(warning(push)) __pragma(warning(disable:4255)) /* no function prototype given: converting '()' to '(void)' */
#include "wimp.h" /* C: tboxlibs */
__pragma(warning(pop))
#endif /* __wimp_h */

#include "wimplib.h" /* C: tboxlibs */

/* our additional helper structures and functions to go with tboxlibs */

typedef WimpMessage * P_WimpMessage; typedef const WimpMessage * PC_WimpMessage;

typedef BBox * P_BBox; typedef const BBox * PC_BBox;

_Check_return_
static inline int
BBox_width(
    _InRef_     PC_BBox p_bbox)
{
    return(p_bbox->xmax - p_bbox->xmin);
}

_Check_return_
static inline int
BBox_height(
    _InRef_     PC_BBox p_bbox)
{
    return(p_bbox->ymax - p_bbox->ymin);
}

#define BBox_from_gdi_rect(bbox__ref, gdi_rect /*const*//*ref*/) \
    (bbox__ref).xmin = (gdi_rect).tl.x, \
    (bbox__ref).ymin = (gdi_rect).br.y, \
    (bbox__ref).xmax = (gdi_rect).br.x, \
    (bbox__ref).ymax = (gdi_rect).tl.y

#if defined(__xp_skel_h)

static inline void
host_framed_BBox_trim(
    _InoutRef_  P_BBox p_bbox,
    _In_        FRAMED_BOX_STYLE border_style)
{
    host_framed_box_trim_frame((P_GDI_BOX) p_bbox, border_style);
}

#endif /* __xp_skel_h */

/*
Icons
*/

typedef int wimp_i; /* abstract icon handle, actually 32-bit value */
#define BAD_WIMP_I  ((wimp_i) -1)

typedef struct WimpIconFlagsBitset /* flags bitset for ease of manipulation */
{
    UBF text          : 1;
    UBF sprite        : 1;
    UBF border        : 1;
    UBF horz_centre   : 1;

    UBF vert_centre   : 1;
    UBF filled        : 1;
    UBF font          : 1;
    UBF needs_help    : 1;

    UBF indirect      : 1;
    UBF right_justify : 1;
    UBF esg_no_cancel : 1;
    UBF halve_sprite  : 1;

    UBF button_type   : 4;

    UBF esg           : 4; /* as per RO5 PRM */

    UBF numbers_ltor  : 1;
    UBF selected      : 1;
    UBF disabled      : 1;
    UBF deleted       : 1;

    UBF fg_colour     : 4;

    UBF bg_colour     : 4;
}
WimpIconFlagsBitset;

typedef union WimpIconFlagsWithBitset /* contains bitset for ease of manipulation */
{
    U32 u32;
    WimpIconFlagsBitset bits;
}
WimpIconFlagsWithBitset;

typedef struct WimpIconBlockWithBitset /* contains bitset for ease of manipulation */
{
    BBox            bbox;
    WimpIconFlagsWithBitset flags;
    WimpIconData    data;
}
WimpIconBlockWithBitset /* analogous to WimpIconBlock */, * P_WimpIconBlockWithBitset; typedef const WimpIconBlockWithBitset * PC_WimpIconBlockWithBitset;

/*
Windows
*/

typedef int wimp_w; /* abstract window handle, actually 32-bit value */
#define NULL_WIMP_W ((wimp_w) 0)
#define ICONBAR_WIMP_W ((wimp_w) -2) /* the window handle that icon bar events appear to come from */

typedef struct WimpWindowFlagsBitset /* flags bitset for ease of manipulation */
{
    UBF old_bit_0         : 1;
    UBF moveable          : 1;
    UBF old_bit_2         : 1;
    UBF old_bit_3         : 1;

    UBF redrawn_by_wimp   : 1;
    UBF pane              : 1;
    UBF trespass          : 1;
    UBF old_bit_7         : 1;

    UBF scroll_autorepeat : 1;
    UBF scroll_debounced  : 1;
    UBF real_colours      : 1;
    UBF back_windows      : 1;

    UBF hot_keys          : 1;
    UBF force_to_screen   : 1;
    UBF ignore_extent_r   : 1;
    UBF ignore_extent_b   : 1;

    UBF open              : 1;
    UBF top               : 1;
    UBF full_size         : 1;
    UBF full_size_request : 1;

    UBF has_input_focus   : 1;
    UBF force_to_screen_on_next_open : 1;
    UBF reserved_bit_22   : 1;
    UBF reserved_bit_23   : 1;

    UBF has_back          : 1;
    UBF has_close         : 1;
    UBF has_title         : 1;
    UBF has_toggle_size   : 1;

    UBF has_vert_scroll   : 1;
    UBF has_adjust_size   : 1;
    UBF has_horz_scroll   : 1;
    UBF flags_are_new     : 1; /* new is a C++ keyword */
}
WimpWindowFlagsBitset;

typedef union WimpWindowFlagsWithBitset /* contains bitset for ease of manipulation */
{
    U32 u32;
    WimpWindowFlagsBitset bits;
}
WimpWindowFlagsWithBitset;

typedef struct WimpWindowWithBitset /* contains bitset for ease of manipulation */
{
    BBox          visible_area;
    int           xscroll;
    int           yscroll;
    int           behind;
    WimpWindowFlagsWithBitset flags;
    char          title_fg;
    char          title_bg;
    char          work_fg;
    char          work_bg;
    char          scroll_outer;
    char          scroll_inner;
    char          highlight_bg;
    char          reserved;
    BBox          extent;
    WimpIconFlagsWithBitset title_flags;
    WimpIconFlagsWithBitset work_flags;
    void         *sprite_area;
    short         min_width;
    short         min_height;
    WimpIconData  titledata;
    int           nicons;
}
WimpWindowWithBitset; /* analogous to WimpWindow */

typedef struct WimpMenuItemWithBitset
{
    unsigned int  flags;

    union WimpMenuItem_submenu
    {
        WimpMenu *  m;  /* pointer to submenu */
        wimp_w      window_handle; /* or wimp_w dialogue box */
        int         i;  /* or -1 if no submenu */
    } submenu;

    WimpIconFlagsWithBitset icon_flags;
    WimpIconData  icon_data;
}
WimpMenuItemWithBitset;

typedef struct WimpCreateIconBlockWithBitset
{
    int      window_handle;
    WimpIconBlockWithBitset icon;
}
WimpCreateIconBlockWithBitset; /* analogous to WimpCreateIconBlock */

union wimp_window_state_open_window_block_u
{
    WimpGetWindowStateBlock window_state_block;
    WimpOpenWindowBlock     open_window_block; /* a sub-set thereof */
};

/* other useful button state bits not defined by tboxlibs wimp.h */
#define Wimp_MouseButtonDragAdjust      0x0010
/* NB Menu button NEVER produces 0x0020 or 0x0200 */
#define Wimp_MouseButtonDragSelect      0x0040
#define Wimp_MouseButtonSingleAdjust    0x0100
#define Wimp_MouseButtonSingleSelect    0x0400
#define Wimp_MouseButtonTripleAdjust    0x1000
#define Wimp_MouseButtonTripleSelect    0x4000

/*
sizes of wimp controlled areas of screen - slightly mode dependent
*/

#define wimp_iconbar_height         (96)

#define wimp_win_title_height(dy)   (40+(dy))
#define wimp_win_hscroll_height(dy) (40+(dy))
#define wimp_win_vscroll_width(dx)  (40+(dx))

/* created particularly for WimpOpenWindowBlock & WimpRedrawWindowBlock,
 * these can operate on any structure with corresponding
 * visible_area.xmin/ymax and xscroll/yscroll members
 */
#define work_area_origin_x_from_visible_area_and_scroll(ptr_ref) ( \
    (ptr_ref)->visible_area.xmin - (ptr_ref)->xscroll )

#define work_area_origin_y_from_visible_area_and_scroll(ptr_ref) ( \
    (ptr_ref)->visible_area.ymax - (ptr_ref)->yscroll )

typedef struct WimpPreQuitMessage
{
    int flags;
}
WimpPreQuitMessage;

#define Wimp_MPreQuit_flags_killthistask 0x01

typedef struct WimpDataRequestMessage
{
    wimp_w     destination_window_handle;
    int        destination_handle;
    int        destination_x;               /* abs */
    int        destination_y;
    int        flags;
    int        type[236/4 - 5];             /* list of acceptable file types, -1 at end */
}
WimpDataRequestMessage;

typedef struct WimpMenuWarningMessage
{
    union WimpMenuWarningMessage_submenu
    {
        WimpMenu * m;
        wimp_w     window_handle;
    } submenu;

    int             x;                      /* where to open the submenu */
    int             y;

    int             menu[10];               /* list of menu selection indices, -1 at end */
}
WimpMenuWarningMessage;

typedef struct WimpTaskInitMessage
{
    void * CAO;
    int    size;
    char   taskname[236 - 2*4];
}
WimpTaskInitMessage;

typedef struct WimpIconizeMessage
{
    wimp_w window_handle;
    int    window_owner;
    char   title[20];
}
WimpIconizeMessage;

typedef struct WimpWindowInfoMessage
{
    wimp_w window_handle;
    int    reserved_0;
    char   sprite[8]; /* doesn't include ic_ */
    char   title[20];
}
WimpWindowInfoMessage;

extern int g_current_wm_version;

_Check_return_
_Ret_maybenull_
static inline _kernel_oserror *
wimp_create_icon_with_bitset(
    _In_        const WimpCreateIconBlockWithBitset * const icreate,
    _Out_       wimp_i * const p_icon_handle)
{
    _kernel_swi_regs rs;
    _kernel_oserror * e;

    rs.r[1] = (int) icreate;

    e = _kernel_swi(/*Wimp_CreateIcon*/ 0x000400C2, &rs, &rs);

    *p_icon_handle = (NULL == e) ? (wimp_i) rs.r[0] : BAD_WIMP_I;

    return(e);
}

_Check_return_
_Ret_maybenull_
extern _kernel_oserror *
wimp_dispose_icon(
    _InVal_     wimp_w window_handle,
    _Inout_     wimp_i * const p_icon_handle);

_Check_return_
_Ret_maybenull_
static inline _kernel_oserror *
wimp_force_redraw_BBox(
    _InVal_     wimp_w window_handle,
    _InRef_     PC_BBox p_bbox)
{
    return(
        wimp_force_redraw(
            window_handle,
            p_bbox->xmin,
            p_bbox->ymin,
            p_bbox->xmax,
            p_bbox->ymax));
}

_Check_return_
_Ret_maybenull_
extern
_kernel_oserror *wimp_poll_coltsoft    (int mask,
                                        WimpPollBlock *block,
                                        int *pollword,
                                        int *event_code);

_Check_return_
_Ret_valid_
extern _kernel_oserror *
wimp_reporterror_rf(
    _In_        _kernel_oserror * e,
    _InVal_     int errflags_in,
    _Out_       int * const p_button_clicked,
    _In_opt_z_  const char * message,
    _InVal_     int error_level);

_Check_return_
_Ret_valid_
extern _kernel_oserror *
wimp_reporterror_simple(
    _In_        _kernel_oserror * e); /* always just Continue(OK) button */

_Check_return_
_Ret_maybenull_
static inline _kernel_oserror *
wimp_set_caret_position_block(
    _In_        const WimpCaret * const c)
{
    return(
        wimp_set_caret_position(
            c->window_handle,
            c->icon_handle,
            c->xoffset,
            c->yoffset,
            c->height,
            c->index));
}

/* NB winx_set_caret_position is preferred */

#if TRACE_ALLOWED
static inline _kernel_oserror *
report_wimp_send_message(
    int code,
    void *block,
    int handle,
    int icon,
    int *th)
{
    trace_3(TRACE_RISCOS_HOST, TEXT("wimp_send_message: %s to ") UINTPTR_XTFMT TEXT(", icon %d"), report_wimp_event(code, block), handle, icon);
    return((wimp_send_message)(code, block, handle, icon, th));
}
#define wimp_send_message(c,b,h,i,t) report_wimp_send_message(c,b,h,i,t)
#endif /* TRACE_ALLOWED */

/* These are useful for wrapping up calls to system functions
 * and distinguishing our own errors from those emanating from
 * the Database Engine which uses the RISC_OSLib wimpt_xxx API.
 */

/*ncr*/
extern _kernel_oserror *
__WrapOsErrorChecking(
    _In_opt_    _kernel_oserror * const p_kernel_oserror,
    _In_z_      PCTSTR tstr_function,
    _In_z_      PCTSTR tstr_file,
    _In_        int line_no,
    _In_z_      PCTSTR tstr);

/*ncr*/
static inline _kernel_oserror *
WrapOsErrorReporting(
    _In_opt_    _kernel_oserror * p_kernel_oserror)
{
    if(NULL != p_kernel_oserror)
        (void) wimp_reporterror_simple(p_kernel_oserror);

    return(p_kernel_oserror);
}

static inline void
void_WrapOsErrorReporting(
    _In_opt_    _kernel_oserror * e)
{
    if(NULL != e)
        (void) wimp_reporterror_simple(e);
}

static inline void
host_gdi_org_from_screen(
    _OutRef_    P_GDI_POINT p_gdi_org,
    _InVal_     HOST_WND window_handle)
{
    WimpGetWindowStateBlock window_state;
    window_state.window_handle = window_handle;
    void_WrapOsErrorReporting(wimp_get_window_state(&window_state));
    p_gdi_org->x = work_area_origin_x_from_visible_area_and_scroll(&window_state); /* window w.a. ABS origin */
    p_gdi_org->y = work_area_origin_y_from_visible_area_and_scroll(&window_state);
}

#ifdef EXPOSE_RISCOS_FONT

_Check_return_
_Ret_maybenull_
extern _kernel_oserror *
font_SetFont(
    _In_        HOST_FONT host_font);

_Check_return_
_Ret_maybenull_
extern _kernel_oserror *
font_LoseFont(
    _In_        HOST_FONT host_font);

/* Font_Paint options */
#define FONT_PAINT_JUSTIFY          0x000001 /* justify text */
#define FONT_PAINT_RUBOUT           0x000002 /* rub-out box required */
                                 /* 0x000004    not used */
                                 /* 0x000008    not used */
#define FONT_PAINT_OSCOORDS         0x000010 /* OS coords supplied (otherwise 1/72000 inch) */
#define FONT_PAINT_RUBOUTBLOCK      0x000020
#define FONT_PAINT_USE_LENGTH       0x000080 /*r7*/
#define FONT_PAINT_USE_HANDLE       0x000100 /*r0*/
#define FONT_PAINT_KERNING          0x000200

/* Font_ScanString options */
#define FONT_SCANSTRING_USE_R5      0x000020 /*r5*/
#define FONT_SCANSTRING_USE_R6      0x000040 /*r6*/
#define FONT_SCANSTRING_USE_R7      0x000080 /*r7*/
#define FONT_SCANSTRING_USE_R0      0x000100 /*r0*/
#define FONT_SCANSTRING_KERNING     0x000200
#define FONT_SCANSTRING_FIND        0x020000

#define FONT_SCANSTRING_USE_HANDLE  FONT_SCANSTRING_USE_R0
#define FONT_SCANSTRING_USE_LENGTH  FONT_SCANSTRING_USE_R7
#define FONT_SCANSTRING_GET_BBOX    (FONT_SCANSTRING_USE_R5 | 0x040000)

/*#include "font.h"*/ /* RISC_OSLib: */

#ifndef __font_h
#define __font_h

typedef int font; /* abstract font handle */

#endif

/* end font.h */

#endif /* EXPOSE_RISCOS_FONT */

#ifndef __menu_h
#define __menu_h

/* Suppress RISC_OSLib's menu system */

#ifndef menu__str_defined
#define menu__str_defined
typedef struct menu__str * abstract_menu_handle; /* abstract menu handle */
#endif

#endif /* __menu_h */

/* end of menu.h */

#ifndef __event_h
#define __event_h

/* Process the given event with default action
*/

/*ncr*/
extern BOOL
event_do_process(
    _In_        int event_code,
    _Inout_     WimpPollBlock * const p_event_data);

/* -------- Attaching menus. -------- */

typedef struct event__submenu * event_submenu;
/* A system-dependent representation of a user interface object. Interfaces
 * for user interface objects that contain a "syshandle" system hook function
 * returning an int, in fact return one of these. This is implicit to reduce
 * interdependencies between the definition modules.
*/

typedef BOOL (* event_menu_proc) (
    void * handle,
    char * hit,
    BOOL submenurequest);

typedef abstract_menu_handle (* event_menu_maker) (
    void * handle);

extern void
event_popup_menu(
    event_menu_maker,
    event_menu_proc,
    void * handle);

/*ncr*/
extern BOOL
event_register_window_menumaker(
    _InVal_     wimp_w window_handle,
    event_menu_maker,
    event_menu_proc,
    void * handle);

/*ncr*/
extern BOOL
event_register_iconbar_menumaker(
    _InVal_     wimp_i icon_handle,
    event_menu_maker,
    event_menu_proc,
    void * handle);
/* Rather than attaching a menu to a window, a procedure can be attached
 * that contructs a menu. The procedure will be called whenever the menu is
 * invoked by the user, allowing setting of flags etc. at the last minute. The
 * menu will be destroyed at the end of the operation. The menumaker and the
 * menu event proc share the same data handle.
*/

/* The above return TRUE if the attachment succeeded, or FALSE
if it failed due to space allocation failing. */

_Check_return_
_Ret_maybenull_
extern _kernel_oserror *
event_create_menu(
    WimpMenu * m,
    int x,
    int y);

_Check_return_
_Ret_maybenull_
extern _kernel_oserror *
event_create_submenu(
    WimpMenu * m,
    int x,
    int y);
/* for win to zap recreatepending */

extern void
event_clear_current_menu(void);
/* It's not possible for the client of this interface to tell if a
 * registered menu tree is still active, or currently visible. This call
 * definitely clears away the current menu tree.
 * In Arthur terms: a null menu tree is registered with the Window Manager.
*/

_Check_return_
extern BOOL
event_is_menu_recreate_pending(void);
/* Callable inside your menu processors, it indicates whether the menu will be recreated
*/

_Check_return_
extern BOOL
event_is_menu_being_recreated(void);
/* Callable inside your menu makers, it indicates whether information
 * should be cached on the basis of the current pointer position
*/

_Check_return_
extern STATUS
event_read_submenudata(
    _Out_opt_   WimpMenu ** smp,
    _Out_opt_   int * const xp,
    _Out_opt_   int * const yp);
/* Read the submenu pointer and x, y position to open a submenu at */

_Check_return_
extern BOOL
event_query_submenudata_valid(void);
/* whether the above data can be read */

_Check_return_
extern abstract_menu_handle
event_return_real_menu(
    WimpMenu * m);
/* bodge for new world menus: return(event_return_real_menu(m)); in menu makers */

#endif /* __event_h */

/* end of event.h */

#ifdef EXPOSE_RISCOS_FLEX
#ifndef          __riscos_ho_flex_h
#include "cmodules/riscos/ho_flex.h"
#endif
#endif /* EXPOSE_RISCOS_FLEX */

#ifndef __wimpt_h
#define __wimpt_h

extern void
wimpt_fake_event(
    _InVal_     int event_code,
    _In_        const WimpPollBlock * const p_event_data);
/* The event_code & event_data are saved away, and will be yielded by the next call to
 * wimpt_poll, rather than calling wimp_poll. If the next call to Poll will not
 * allow it because of the mask then the fake is discarded. Multiple calls
 * without wimpt_poll calls will be ignored.
*/

#endif /* __wimpt_h */

/* end of wimpt.h */

/*
cs-winx.c
*/

typedef BOOL (* winx_event_handler) (
    _InVal_     int event_code,
    _In_        const WimpPollBlock * const p_event_data,
    P_ANY handle);

_Check_return_
extern BOOL
winx_adjustclicked(void);

extern void
winx_changedtitle(
    _InVal_     wimp_w window_handle);

_Check_return_
extern BOOL
winx_dispatch_event(
    _InVal_     int event_code,
    _In_        WimpPollBlock * const p_event_data);

_Check_return_
extern STATUS
winx_register_event_handler(
    _InVal_     wimp_w window_handle,
    _InVal_     wimp_i icon_handle,
    winx_event_handler proc,
    P_ANY handle);

_Check_return_
extern STATUS
winx_register_general_event_handler(
    winx_event_handler proc,
    P_ANY handle);

extern void
winx_claim_caret(void); /* and selection */

_Check_return_
extern BOOL
winx_owns_caret(void); /* and selection */

_Check_return_
_Ret_maybenull_
extern _kernel_oserror *
winx_get_caret_position(
    _Out_       WimpCaret * const caret);

_Check_return_
_Ret_maybenull_
extern _kernel_oserror *
winx_set_caret_position(
    _In_        const WimpCaret * const caret);

extern void
winx_claim_global_clipboard(
    P_PROC_GLOBAL_CLIPBOARD_DATA_DISPOSE p_proc_global_clipboard_data_dispose,
    P_PROC_GLOBAL_CLIPBOARD_DATA_DATAREQUEST p_proc_global_clipboard_data_DataRequest);

_Check_return_
_Ret_maybenull_
extern P_PROC_GLOBAL_CLIPBOARD_DATA_DATAREQUEST
winx_global_clipboard_owner(void);

_Check_return_
extern wimp_w
winx_drag_active(void);

_Check_return_
_Ret_maybenull_
extern _kernel_oserror *
winx_drag_box(
    _In_opt_    /*const*/ WimpDragBox * const dr);

_Check_return_
_Ret_maybenull_
extern _kernel_oserror *
winx_drag_a_sprite_start(
    int flags,
    int sprite_area_id,
    _In_z_      const char * p_sprite_name,
    _In_        const WimpDragBox * const dr);

extern void
winx_drag_a_sprite_stop(void);

_Check_return_
extern void * /* NULL if not set */
winx_menu_get_handle(
    _InVal_     wimp_w window_handle,
    _InVal_     wimp_i icon_handle);

extern void
winx_menu_set_handle(
    _InVal_     wimp_w window_handle,
    _InVal_     wimp_i icon_handle,
    void * handle);

_Check_return_
extern BOOL
winx_register_child_window(
    _InVal_     wimp_w parent_window_handle,
    _InVal_     wimp_w child_window_handle,
    _InVal_     BOOL immediate /*for open_window_request*/);

/*ncr*/
extern BOOL
winx_deregister_child_window(
    _InVal_     wimp_w window_handle);

_Check_return_
extern STATUS
winx_create_window(
    WimpWindowWithBitset * wwp,
    wimp_w * p_window_handle,
    winx_event_handler proc,
    void * handle);

_Check_return_
_Ret_maybenull_
extern const WimpWindowWithBitset * /* low-lifetime too */
win_wimp_wind_query(
    _InVal_     wimp_w window_handle);

_Check_return_
_Ret_maybenull_
extern _kernel_oserror *
winx_close_window(
    _InVal_     wimp_w window_handle);

_Check_return_
_Ret_maybenull_
extern _kernel_oserror *
winx_dispose_window(
    _Inout_     wimp_w * const p_window_handle);

_Check_return_
_Ret_maybenull_
extern _kernel_oserror *
winx_open_window(
    _Inout_     WimpOpenWindowBlock * const p_open_window_block);

extern void
winx_open_child_windows(
    _InVal_     wimp_w window_handle,
    _In_        const WimpOpenWindowBlock * const p_open_window_block,
    _Out_       wimp_w * const p_new_behind);

extern void
winx_send_close_window_request(
    _InVal_     wimp_w window_handle,
    _InVal_     BOOL immediate);

extern void
winx_send_open_window_request(
    _InVal_     wimp_w window_handle,
    _InVal_     BOOL immediate,
    _In_        const WimpOpenWindowBlock * const p_open_window_block);

extern void
winx_send_front_window_request(
    _InVal_     wimp_w window_handle,
    _InVal_     BOOL immediate,
    _InRef_opt_ PC_BBox p_bbox);

_Check_return_
extern BOOL
winx_menu_process(
    _InVal_     int event_code,
    _In_        const WimpPollBlock * const p_event_data);

_Check_return_
_Ret_maybenull_
extern _kernel_oserror *
winx_create_submenu_child(
    _InVal_     wimp_w window_handle,
    _InVal_     int x,
    _InVal_     int y);

_Check_return_
_Ret_maybenull_
extern _kernel_oserror *
winx_create_submenu(
    _InVal_     wimp_w window_handle,
    _InVal_     int x,
    _InVal_     int y);

_Check_return_
_Ret_maybenull_
extern _kernel_oserror *
winx_create_menu(
    _InVal_     wimp_w window_handle,
    _InVal_     int x,
    _InVal_     int y);

extern void
winx_create_complex_menu(
    _InVal_     wimp_w window_handle);

_Check_return_
extern BOOL
winx_submenu_query_is_submenu(
    _InVal_     wimp_w window_handle);

_Check_return_
extern BOOL
winx_submenu_query_is_submenu_child(
    _InVal_     wimp_w window_handle);

_Check_return_
extern BOOL
winx_submenu_query_closed(void);

_Check_return_
extern BOOL
winx_submenu_query_closed_child(void);

/* end of cs-winx.h */

extern BOOL g_kill_duplicates;

typedef BOOL (* P_PROC_RISCOS_MESSAGE_FILTER) (
    _InVal_     int event_code,
    _In_        const WimpPollBlock * const p_event_data,
    CLIENT_HANDLE handle);

extern void
host_message_filter_register(
    _In_        P_PROC_RISCOS_MESSAGE_FILTER p_proc_message_filter,
    CLIENT_HANDLE client_handle);

extern void
host_message_filter_add(
    const int * const wimp_messages);

extern void
host_message_filter_remove(
    const int * const wimp_messages);

typedef struct HOST_MODEVAR_CACHE_ENTRY
{
    U32 mode_specifier; /* Number, Selector or Word */

    /* derived values */
    S32 dx;
    S32 dy;
    U32 bpp;

    GDI_SIZE gdi_size;

    /* raw values */
    U32 XEigFactor;
    U32 YEigFactor;
    U32 Log2BPP;

    U32 XWindLimit;
    U32 YWindLimit;

    ARRAY_HANDLE h_mode_selector; /*RISCOS_MODE_SELECTOR*/
}
HOST_MODEVAR_CACHE_ENTRY, * P_HOST_MODEVAR_CACHE_ENTRY; typedef const HOST_MODEVAR_CACHE_ENTRY * PC_HOST_MODEVAR_CACHE_ENTRY;

extern HOST_MODEVAR_CACHE_ENTRY
host_modevar_cache_current;

extern void
host_modevar_cache_reset(void);

extern void
host_modevar_cache_query_bpp(
    _InVal_     U32 mode_specifier,
    _OutRef_    P_U32 p_bpp);

extern void
host_modevar_cache_query_eig_factors(
    _InVal_     U32 mode_specifier,
    _OutRef_    P_U32 p_XEigFactor,
    _OutRef_    P_U32 p_YEigFactor);

extern void
host_disable_rgb(
    _InoutRef_  P_RGB p_rgb,
    _InRef_     PC_RGB p_rgb_d,
    _InVal_     S32 multiplier);

_Check_return_
extern BOOL
host_must_die_query(void);

extern void
host_must_die_set(
    _InVal_     BOOL must_die);

_Check_return_
extern int
host_platform_features_query(void);

_Check_return_
extern int
host_query_alphabet_number(void);

#define PLATFEAT_SYNCH_CODE_AREAS (1 << 0) /* Must tell OS when code areas change (by calling OS_SynchroniseCodeAreas) */

extern void
host_ploticon(
    _InRef_     PC_WimpIconBlockWithBitset p_icon);

extern void
host_ploticon_setup_bbox(
    _InoutRef_  P_WimpIconBlockWithBitset p_icon,
    _InRef_     PC_PIXIT_RECT p_pixit_rect,
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context);

_Check_return_
extern int
host_task_handle(void);

extern void
host_xfer_import_file_via_scrap(
    _InRef_     PC_WimpMessage p_wimp_message /*DataSave*/);

extern void
host_xfer_load_file_done(void); /* MUST be called if host_xfer_load_file_setup() has been called */

extern void
host_xfer_load_file_setup(
    _InRef_     PC_WimpMessage p_wimp_message /*DataLoad/DataOpen*/);

extern void
host_xfer_print_file_done(
    _InRef_     PC_WimpMessage p_wimp_message /*PrintTypeOdd*/,
    int type);

typedef /*_Check_return_*/ BOOL (* P_PROC_HOST_XFER_SAVE) (
    _In_z_      PCTSTR filename,
    _InVal_     T5_FILETYPE t5_filetype,
    _InVal_     CLIENT_HANDLE client_handle);

_Check_return_
extern BOOL
host_xfer_save_file(
    _In_z_      PCTSTR filename,
    _InVal_     T5_FILETYPE t5_filetype,
    int estimated_size,
    P_PROC_HOST_XFER_SAVE p_proc_host_xfer_save,
    CLIENT_HANDLE client_handle,
    _In_        const WimpGetPointerInfoBlock * const p_pointer_info);

_Check_return_
extern BOOL
host_xfer_save_file_for_DataRequest(
    _In_z_      PCTSTR filename,
    _InVal_     T5_FILETYPE t5_filetype,
    int estimated_size,
    P_PROC_HOST_XFER_SAVE p_proc_host_xfer_save,
    CLIENT_HANDLE client_handle,
    _In_        const WimpMessage * const p_wimp_message /*DataRequest*/);

#endif /* RISCOS */

#endif /* __xp_skelr_h */

/* end of xp_skelr.h */
