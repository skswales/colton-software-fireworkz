/* mlec.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* MultiLine Edit Controls for RISC OS */

/* RCM May 1991 */

#ifndef __mlec_h
#define __mlec_h

#ifndef MLEC_OFF

/*
Exported types
*/

typedef struct MLEC * MLEC, ** P_MLEC;

#define MLEC_ATTRIBUTE_LINESPACE 0
#define MLEC_ATTRIBUTE_CHARHEIGHT 1
#define MLEC_ATTRIBUTE_CARETHEIGHTPOS 2
#define MLEC_ATTRIBUTE_CARETHEIGHTNEG 3
#define MLEC_ATTRIBUTE_MARGIN_LEFT 4
#define MLEC_ATTRIBUTE_MARGIN_TOP 5
#define MLEC_ATTRIBUTE_BG_RGB 6
#define MLEC_ATTRIBUTE_FG_RGB 7
#define MLEC_ATTRIBUTE_MAX 8

#define MLEC_ATTRIBUTE int

/*
Exported routines
*/

_Check_return_
extern int
mlec_attribute_query(
    /*_In_*/    MLEC mlec,
    _InVal_     MLEC_ATTRIBUTE attribute);

extern void
mlec_attribute_set(
    /*_Inout_*/ MLEC mlec,
    _InVal_     MLEC_ATTRIBUTE attribute,
    _InVal_     int value);

_Check_return_
extern STATUS
mlec_create(
    _OutRef_    P_MLEC mlecp);

extern void
mlec_destroy(
    _InoutRef_ /*never NULL*/ P_MLEC mlecp);

#ifdef MLEC_PANE

_Check_return_
extern STATUS
mlec_attach(
    /*_Inout_*/ MLEC mlec,
    _InVal_     wimp_w main_win_handle,
    _InVal_     wimp_w pane_win_handle,
    _InRef_     PC_BBox paneWorkArea);

extern void
mlec_detach(
    MLEC mlec);

#endif

extern void
mlec_claim_focus(
    MLEC mlec);

/*   mlec_attach_eventhandler - see further down this file */

_Check_return_
extern STATUS
mlec_GetText(
    MLEC mlec, char * buffptr,
    _In_        S32 buffsize);

_Check_return_
extern S32
mlec_GetTextLen(
    MLEC mlec);

_Check_return_
extern STATUS
mlec_SetText(
    MLEC mlec,
    _In_z_      PC_U8Z text);

/*ncr*/
extern BOOL
mlec__event_handler(
    _InVal_     int event_code,
    _In_        const WimpPollBlock * const p_event_data,
    void * handle);

_Check_return_
extern STATUS
mlec__insert_char(
    MLEC mlec,
    _InVal_     U8 ch);

_Check_return_
extern STATUS
mlec__insert_text(
    MLEC mlec,
    _In_z_      PC_U8Z text);

_Check_return_
extern STATUS
mlec__insert_newline(
    MLEC mlec);

extern void
mlec__cursor_getpos(
    MLEC mlec,
    _OutRef_    P_S32 colp,
    _OutRef_    P_S32 rowp);

extern void
mlec__cursor_setpos(
    MLEC mlec,
    _InVal_     S32 col,
    _InVal_     S32 row);

extern void
mlec__cursor_textend(
    MLEC mlec);

extern void
mlec__cursor_texthome(
    MLEC mlec);

_Check_return_
extern STATUS
mlec_attach_panemenu(
    MLEC mlec,
    const char * menu_title);

extern void
mlec_area_update(
    MLEC mlec,
    _InRef_     PC_GDI_POINT p_origin,
    _InRef_     PC_GDI_BOX p_screen);

extern void
mlec__click_core(
    MLEC mlec,
    P_GDI_POINT p_origin,
    _In_        const WimpMouseClickEvent * const p_mouse_click);

extern void
mlec__drag_core(
    MLEC mlec,
    _InRef_     PC_GDI_POINT p_origin,
    _In_        const WimpGetPointerInfoBlock * const p_pointer_info);

_Check_return_
extern STATUS
mlec__key_core(
    MLEC mlec,
    _In_        KMAP_CODE kmap_code);

extern void
mlec__redraw_core(
    MLEC mlec,
    _InRef_     PC_GDI_POINT p_origin,
    _InRef_     PC_GDI_BOX p_screen);

extern void
mlec__selection_adjust(
    MLEC mlec,
    _In_        int col,
    _In_        int row);

extern void
mlec__selection_clear(
    MLEC mlec);

extern void
mlec__selection_delete(
    MLEC mlec);

#define MLEC_CODE_NONE         0
#define MLEC_CODE_OPEN         1
#define MLEC_CODE_CLOSE        2
#define MLEC_CODE_KEY_RETURN   3
#define MLEC_CODE_KEY_ESCAPE   4
#define MLEC_CODE_CLICK        5
#define MLEC_CODE_IsWorkAreaChanged 6
#define MLEC_CODE_UPDATELATER  7
#define MLEC_CODE_PLACECARET   8
#define MLEC_CODE_CLAIMFOCUS   9
#define MLEC_CODE_RELEASEFOCUS 0x0A
#define MLEC_CODE_STARTDRAG    0x0B
#define MLEC_CODE_UPDATENOW    0x0C
#define MLEC_CODE_SCROLL       0x0D
#define MLEC_CODE_STARTEDDRAG  0x0E
#define MLEC_CODE_QUERYSCROLL  0x0F
#define MLEC_CODE_DOSCROLL     0x10
#define MLEC_CODE_UPDATE       0x11
#define MLEC_CODE_KEY          0x12

#define mlec_event_reason_code int

    /* from IsWorkAreaChanged */
#define mlec_event_workareachanged 0x60

#define MLEC_EVENT_DOSTARTDRAG     0xE0

#define mlec_event_return_code int

typedef struct MLEC_QUERYSCROLL
{
    GDI_POINT scroll;
    GDI_POINT visible;
    S32 use;
}
MLEC_QUERYSCROLL, * P_MLEC_QUERYSCROLL;

typedef struct MLEC_DOSCROLL
{
    GDI_POINT scroll;
}
MLEC_DOSCROLL, * P_MLEC_DOSCROLL;

typedef mlec_event_return_code (* mlec_event_proc) (
    mlec_event_reason_code rc,
    P_ANY handle,
    P_ANY p_eventdata);

#define mlec_event_proto(_proc_name, rc, handle, p_eventdata) \
mlec_event_return_code \
_proc_name( \
    mlec_event_reason_code rc, \
    P_ANY handle, \
    P_ANY p_eventdata) \

extern void
mlec_attach_eventhandler(
    MLEC mlec,
    mlec_event_proc proc,
    P_ANY handle,
    _InVal_     BOOL add);

#define MLEC_KEY_NOTPROCESSED STATUS_OK

#define RESOURCE_NUM_MLEC               30

/*
error definition
*/

#define MLEC_ERR_BASE                   (STATUS_ERR_INCREMENT * OBJECT_ID_MLEC)

#define MLEC_ERR(n)                     (MLEC_ERR_BASE - (n))

#define MLEC_ERR_NOSELECTION            MLEC_ERR(0)
#define MLEC_ERR_ERROR_RQ               MLEC_ERR(1) /* unused, but essential (see SKS) */
#define MLEC_ERR_NOPASTEBUFFER          MLEC_ERR(2)
#define MLEC_ERR_BUFFERWENT_AWOL        MLEC_ERR(3)
#define MLEC_ERR_INVALID_PASTE_OP       MLEC_ERR(4)
#define MLEC_ERR_GETTEXT_BUFOVF         MLEC_ERR(5)

#define MLEC_ERR_END                    MLEC_ERR(6)

/*
message definition (keep consistent with CModules.*.msg.mlec)
*/

#define MLEC_MSG_BASE                   (STATUS_MSG_INCREMENT * OBJECT_ID_MLEC)

#define MLEC_MSG(n)                     (MLEC_MSG_BASE + (n))

#define MLEC_MSG_MENUBDY                MLEC_MSG(1) /* NB menu_title is supplied by caller */
#define MLEC_MSG_MENUHDR_SAVE           MLEC_MSG(10)
#define MLEC_MSG_MENUBDY_SAVE           MLEC_MSG(11)
#define MLEC_MSG_MENUHDR_SELECTION      MLEC_MSG(20)
#define MLEC_MSG_MENUBDY_SELECTION      MLEC_MSG(21)

#endif /* MLEC_OFF */

#endif /* __mlec_h */

/* end of mlec.h */
