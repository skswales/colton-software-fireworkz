/* mlec.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* MultiLine Edit Controls for RISC OS */

/* RCM May 1991 */

#include "common/gflags.h"

#if RISCOS

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#define EXPOSE_RISCOS_SWIS 1
#define EXPOSE_RISCOS_FLEX 1
#define EXPOSE_RISCOS_FONT 1

#include "ob_skel/xp_skelr.h"

#include "cmodules/mlec.h"

#include "cmodules/mleci.h"

#define TRACE_MODULE_MLEC 0

/*
callback function
*/

PROC_EVENT_PROTO(static, null_event_mlec_drag);

/*
internal routines
*/

_Check_return_
static STATUS
mlec__callback(
    mlec_event_reason_code message,
    /*_Inout_*/ MLEC mlec,
    /*_Inout_*/ P_ANY p_data);

#ifdef MLEC_PANE

static void
mlec__event_redraw_loop(
    /*_Inout_*/ MLEC mlec);

#endif

static void
mlec__cursor_down(
    /*_Inout_*/ MLEC mlec);

static void
mlec__cursor_left(
    /*_Inout_*/ MLEC mlec);

static void
mlec__cursor_right(
    /*_Inout_*/ MLEC mlec);

static void
mlec__cursor_up(
    /*_Inout_*/ MLEC mlec);

static void
mlec__cursor_lineend(
    /*_Inout_*/ MLEC mlec);

static void
mlec__cursor_linehome(
    /*_Inout_*/ MLEC mlec);

static void
mlec__cursor_tab_left(
    /*_Inout_*/ MLEC mlec);

static void
mlec__cursor_wordleft(
    /*_Inout_*/ MLEC mlec);

static void
mlec__cursor_wordright(
    /*_Inout_*/ MLEC mlec);

_Check_return_
static STATUS
mlec__insert_tab(
    /*_Inout_*/ MLEC mlec);

static void
mlec__insert_text_core(
    /*_Inout_*/ MLEC mlec,
    _In_z_      PC_U8Z text);

static void
mlec__delete_left(
    /*_Inout_*/ MLEC mlec);

static void
mlec__delete_right(
    /*_Inout_*/ MLEC mlec);

static void
mlec__delete_line(
    /*_Inout_*/ MLEC mlec);

static void
mlec__delete_lineend(
    /*_Inout_*/ MLEC mlec);

static void
mlec__delete_linehome(
    /*_Inout_*/ MLEC mlec);

static void
scroll_until_cursor_visible(
    /*_Inout_*/ MLEC mlec);

static void
show_caret(
    /*_Inout_*/ MLEC mlec);

static void
build_caretstr(
    /*_Inout_*/ MLEC mlec,
    WimpCaret * carrotp);

static void
move_cursor(
    /*_Inout_*/ MLEC mlec,
    _In_        int col,
    _In_        int row);

static void
word_left(
    /*_Inout_*/ MLEC mlec,
    mark_position * startp);

static void
word_limits(
    /*_Inout_*/ MLEC mlec,
    mark_position * startp,
    mark_position * endp);

static P_U8
render_line(
    /*_Inout_*/ MLEC mlec,
    _In_        int lineCol,
    _In_        int x,
    _In_        int y,
    _InRef_     PC_GDI_BOX p_screen,
    P_U8 ptr,
    P_U8 limit);

_Check_return_
static STATUS
checkspace_deletealltext(
    /*_Inout_*/ MLEC mlec,
    _In_        S32 size);

_Check_return_
static STATUS
checkspace_delete_selection(
    /*_Inout_*/ MLEC mlec,
    _In_        S32 size);

static void
force_redraw_eoline(
    /*_Inout_*/ MLEC mlec);

static void
force_redraw_eotext(
    /*_Inout_*/ MLEC mlec);

static void
mlec__issue_update(
    /*_Inout_*/ MLEC mlec);

static void
mlec__select_word(
    /*_Inout_*/ MLEC mlec);

static void
mlec__select_para(
    /*_Inout_*/ MLEC mlec);

static void
mlec__drag_start(
    /*_Inout_*/ MLEC mlec);

#ifdef MLEC_PANE

static void
mlec__drag_complete(
    /*_Inout_*/ MLEC mlec,
    BBox * dragboxp);

#endif

static void
clear_selection(
    /*_Inout_*/ MLEC mlec);

static void
delete_selection(
    /*_Inout_*/ MLEC mlec);

#define remove_selection(mlec) \
    if(mlec->selectvalid) { mlec__selection_delete(mlec); return; }

static BOOL
range_is_selection(
    /*_Inout_*/ MLEC mlec,
    marked_text * range);

static void
find_offset(
    /*_Inout_*/ MLEC mlec,
    mark_position * find,
    int * offsetp);

static void
mlec__update_loop(
    /*_Inout_*/ MLEC mlec,
    mark_position mark1,
    cursor_position mark2);

static void
show_selection(
    /*_Inout_*/ MLEC mlec,
    _InRef_     PC_GDI_POINT p_origin,
    _InRef_     PC_GDI_BOX screenBB,
    mark_position markstt,
    mark_position markend);

#if 0

_Check_return_
static int
mlec__atcursor_load(
    /*_Inout_*/ MLEC mlec,
    _In_z_      PCTSTR filename);

_Check_return_
static int
mlec__alltext_save(
    /*_Inout_*/ MLEC mlec,
    _In_z_      PCTSTR filename,
    _InVal_     T5_FILETYPE t5_filetype,
    P_U8 lineterm);

_Check_return_
static int
mlec__selection_save(
    /*_Inout_*/ MLEC mlec,
    _In_z_      PCTSTR filename,
    _InVal_     T5_FILETYPE t5_filetype,
    P_U8 lineterm);

#endif

#if defined(MLEC_CLIP)

_Check_return_
static STATUS
mlec__atcursor_paste(
    /*_Inout_*/ MLEC mlec);

_Check_return_
static STATUS
mlec__selection_copy(
    /*_Inout_*/ MLEC mlec);

_Check_return_
static STATUS
mlec__selection_cut(
    /*_Inout_*/ MLEC mlec);

#endif

static void
range_is_alltext(
    /*_Inout_*/ MLEC mlec,
    marked_text * range);

#if defined(MLEC_CLIP)

_Check_return_
static STATUS
text_in(
    /*_Inout_*/ MLEC mlec,
    _In_z_      PCTSTR filename,
    /*_Check_return_*/ STATUS (* openp) (
        _In_z_      PCTSTR filename,
        P_T5_FILETYPE p_t5_filetype,
        P_S32 filesizep,
        P_XFER_HANDLE xferhandlep),
    /*_Check_return_*/ STATUS (* readp) (
        P_XFER_HANDLE xferhandlep,
        P_U8 dataptr,
        int datasize),
    /*_Check_return_*/ STATUS (* closep) (
        P_XFER_HANDLE xferhandlep));

#endif

_Check_return_
static STATUS
text_out(
    /*_Inout_*/ MLEC mlec,
    P_XFER_HANDLE xferhandlep,
    marked_text range,
    P_U8 lineterm,
    /*_Check_return_*/ STATUS (* sizep) (
        P_XFER_HANDLE xferhandlep,
        int xfersize),
    /*_Check_return_*/ STATUS (* writep) (
        P_XFER_HANDLE xferhandlep,
        P_U8 dataptr,
        int datasize),
    /*_Check_return_*/ STATUS (* closep) (
        P_XFER_HANDLE xferhandlep));

#if 0

_Check_return_
static STATUS
file_read_open(
    _In_z_      PCTSTR filename,
    /*out*/ P_T5_FILETYPE p_t5_filetype,
    /*out*/ P_S32 filesizep,
    /*out*/ P_XFER_HANDLE xferhandlep);

_Check_return_
static STATUS
file_read_getblock(
    P_XFER_HANDLE xferhandlep,
    P_U8 dataptr,
    _In_        int datasize);

_Check_return_
static STATUS
file_read_close(
    P_XFER_HANDLE xferhandlep);

_Check_return_
static STATUS
file_write_open(
    /*out*/ P_XFER_HANDLE xferhandlep,
    _In_z_      PCTSTR filename,
    _InVal_     T5_FILETYPE t5_filetype);

_Check_return_
static STATUS
file_write_size(
    P_XFER_HANDLE xferhandlep,
    _In_        int xfersize);

_Check_return_
static STATUS
file_write_putblock(
    P_XFER_HANDLE xferhandlep,
    P_U8 dataptr,
    _In_        int datasize);

_Check_return_
static STATUS
file_write_close(
    P_XFER_HANDLE xferhandlep);

#endif

#if defined(MLEC_CLIP)

_Check_return_
static STATUS
paste_read_open(
    _In_z_      PCTSTR filename,
    /*out*/ P_T5_FILETYPE p_t5_filetype,
    /*out*/ P_S32 filesizep,
    /*out*/ P_XFER_HANDLE xferhandlep);

_Check_return_
static STATUS
paste_read_getblock(
    P_XFER_HANDLE xferhandlep,
    P_U8 dataptr,
    _In_        int datasize);

_Check_return_
static STATUS
paste_read_close(
    P_XFER_HANDLE xferhandlep);

_Check_return_
static STATUS
paste_write_open(
    /*out*/ P_XFER_HANDLE xferhandlep);

_Check_return_
static STATUS
paste_write_size(
    P_XFER_HANDLE xferhandlep,
    _In_        int xfersize);

_Check_return_
static STATUS
paste_write_putblock(
    P_XFER_HANDLE xferhandlep,
    P_U8 dataptr,
    _In_        int datasize);

_Check_return_
static STATUS
paste_write_close(
    P_XFER_HANDLE xferhandlep);

static MLEC paste = NULL;        /* The paste buffer is created automatically by paste_write_open() */
                                 /* when mlec__selection_copy() or mlec__selection_cut() are used/  */

#endif /* MLEC_CLIP */

_Check_return_
static STATUS
string_write_open(
    /*out*/ P_XFER_HANDLE xferhandlep,
    P_U8 buffptr,
    _In_        int buffsiz);

_Check_return_
static STATUS
string_write_size(
    P_XFER_HANDLE xferhandlep,
    _In_        int xfersize);

_Check_return_
static STATUS
string_write_putblock(
    P_XFER_HANDLE xferhandlep,
    P_U8 dataptr,
    _In_        int datasize);

_Check_return_
static STATUS
string_write_close(
    P_XFER_HANDLE xferhandlep);

_Check_return_
extern int
mlec_attribute_query(
    /*_In_*/    MLEC mlec,
    _InVal_     MLEC_ATTRIBUTE attribute)
{
    assert((attribute >= 0) && (attribute < MLEC_ATTRIBUTE_MAX));

    return(mlec->attributes[attribute]);
}

extern void
mlec_attribute_set(
    /*_Inout_*/ MLEC mlec,
    _InVal_     MLEC_ATTRIBUTE attribute,
    _InVal_     int value)
{
    assert((attribute >= 0) && (attribute < MLEC_ATTRIBUTE_MAX));

    mlec->attributes[attribute] = value;
}

static void
mlec__colrow_from_point(
    /*_Inout_*/ MLEC mlec,
    _InRef_     PC_GDI_POINT p_point,
    _OutRef_    P_S32 p_col,
    _OutRef_    P_S32 p_row)
{
    *p_col = (S32) ((+p_point->x - mlec->attributes[MLEC_ATTRIBUTE_MARGIN_LEFT] +(S32)mlec->charwidth/2) / mlec->charwidth);
    *p_row = (S32) ((-p_point->y - mlec->attributes[MLEC_ATTRIBUTE_MARGIN_TOP]  -1                     ) / mlec->attributes[MLEC_ATTRIBUTE_LINESPACE]);
}

/* yields window relative top left point */

static void
mlec__point_from_colrow(
    /*_Inout_*/ MLEC mlec,
    _OutRef_    P_GDI_POINT p_point,
    _InVal_     S32 col,
    _InVal_     S32 row)
{
    p_point->x = +(mlec->attributes[MLEC_ATTRIBUTE_MARGIN_LEFT] + (S32) col * mlec->charwidth);
    p_point->y = -(mlec->attributes[MLEC_ATTRIBUTE_MARGIN_TOP]  + (S32) row * mlec->attributes[MLEC_ATTRIBUTE_LINESPACE]);
}

_Check_return_
static HOST_FONT
mlec_get_host_font(void)
{
    HOST_FONT host_font;

    /*U32 size_x = 0;*/
    U32 size_y = 12;

    /* RISC OS font manager needs 16x fontsize */
    /*U32 x16_size_x = 16 * 0;*/
    U32 x16_size_y = 16 * size_y;

    /* c.f. host_font_find() in Fireworkz */
    _kernel_swi_regs rs;
    _kernel_oserror * p_kernel_oserror;

    rs.r[1] = (int) /*"\\E" "Latin1"*/ "\\F" "DejaVuSans.Mono";
    rs.r[2] = /*x16_size_x ? x16_size_x :*/ x16_size_y;
    rs.r[3] = x16_size_y;
    rs.r[4] = 0;
    rs.r[5] = 0;

    if(NULL == (p_kernel_oserror = (_kernel_swi(/*Font_FindFont*/ 0x040081, &rs, &rs))))
    {
        host_font = (HOST_FONT) rs.r[0];
        return(host_font);
    }

    rs.r[1] = (int) /*"\\E" "Latin1"*/ "\\F" "Corpus.Medium";
    rs.r[2] = /*x16_size_x ? x16_size_x :*/ x16_size_y;
    rs.r[3] = x16_size_y;
    rs.r[4] = 0;
    rs.r[5] = 0;

    if(NULL == (p_kernel_oserror = (_kernel_swi(/*Font_FindFont*/ 0x040081, &rs, &rs))))
    {
        host_font = (HOST_FONT) rs.r[0];
        return(host_font);
    }

    return(HOST_FONT_NONE);
}

/******************************************************************************
*
* Create the data structures for an mlec (multi-line edit control)
*
******************************************************************************/

_Check_return_
extern STATUS
mlec_create(
    _OutRef_    P_MLEC mlecp)
{
    static const RGB rgb_background = { 0xFF, 0xFF, 0xFF }; /* white */
    static const RGB rgb_foreground = { 0x00, 0x00, 0x00 }; /* black */

    STATUS status;
    MLEC mlec;
    int buffsiz = MLEC_DEFAULT_BUFSIZ;

    trace_0(TRACE_MODULE_MLEC, TEXT("mlec_create"));

    if(NULL != (mlec = al_ptr_calloc_elem(struct MLEC, 1, &status)))
    {
        if(flex_alloc((flex_ptr) &mlec->buffptr, buffsiz))
        {
            /* empty buffer */
            mlec->lower.start = 0;
            mlec->upper.end   = mlec->buffsiz = buffsiz;
            mlec->lower.end   = mlec->lower.start;
            mlec->upper.start = mlec->upper.end;
            mlec->maxcol      = 0;
            mlec->linecount   = 0;

            /* home cursor */
            mlec->cursor.lcol = mlec->cursor.pcol = mlec->cursor.row = 0;

            mlec->attributes[MLEC_ATTRIBUTE_MARGIN_LEFT]  = 4; /*2;*/ /*32;*/
            mlec->attributes[MLEC_ATTRIBUTE_MARGIN_TOP]   = 4; /*16;*/

            mlec->host_font = mlec_get_host_font();

            if(HOST_FONT_NONE != mlec->host_font)
                mlec->charwidth = 18; /* fixed pitch font */
            else
                mlec->charwidth = 16; /* System font */

            mlec->attributes[MLEC_ATTRIBUTE_CHARHEIGHT]  = 32;

            mlec->attributes[MLEC_ATTRIBUTE_LINESPACE]      = mlec->attributes[MLEC_ATTRIBUTE_CHARHEIGHT];
            mlec->attributes[MLEC_ATTRIBUTE_CARETHEIGHTPOS] = mlec->attributes[MLEC_ATTRIBUTE_LINESPACE] + 4;
            mlec->attributes[MLEC_ATTRIBUTE_CARETHEIGHTNEG] = 4;

            mlec->attributes[MLEC_ATTRIBUTE_BG_RGB] = * (PC_S32) &rgb_background;
            mlec->attributes[MLEC_ATTRIBUTE_FG_RGB] = * (PC_S32) &rgb_foreground;

            mlec->termwidth   = mlec->charwidth/4;
#if FALSE
            mlec->termwidth   = mlec->charwidth * 3; /*>>>made bigger for testing*/
#endif

            mlec->main        = window_NULL;
            mlec->pane        = window_NULL;     /* ie not attached */

#ifdef MLEC_PANE
            mlec->panemenu     = FALSE;
#endif
#if FALSE
         /* done by attach */
            mlec->paneposx    =  32;
            mlec->paneposy    = -32;
            mlec->panewidth   =  500;
            mlec->paneheight  =  250;
#endif
            mlec->callbackproc = NULL;
            mlec->callbackhand = NULL;

            /* No initial selection */
            mlec->selectvalid  = FALSE;
            mlec->selectEORcol = 7;     /*>>>wrong! should be realcolour_for_wimpcol(0) EOR realcolour_for_wimpcol(7) */

            trace_0(TRACE_MODULE_MLEC, TEXT(" - OK"));

            *mlecp = mlec;

            return(STATUS_OK);
        }
        else
            status = status_nomem();

        al_ptr_free(mlec);
    }

    trace_0(TRACE_MODULE_MLEC, TEXT(" - !!!***!!! F A I L E D !!!***!!!"));

    *mlecp = NULL;

    return(status); /* not enough memory for MLEC */
}

/******************************************************************************
*
* Destroy the data structures of an mlec (multiline edit control)
*
******************************************************************************/

extern void
mlec_destroy(
    _InoutRef_ /*never NULL*/ P_MLEC mlecp)
{
    MLEC mlec;

    PTR_ASSERT(mlecp);

    if(NULL != (mlec = *mlecp))
    {
#ifdef MLEC_PANE
        mlec_detach(*mlecp);
#endif

        if(HOST_FONT_NONE != mlec->host_font)
            WrapOsErrorReporting(font_LoseFont(mlec->host_font));

        flex_dispose((flex_ptr) &mlec->buffptr);

        al_ptr_dispose(P_P_ANY_PEDANTIC(mlecp));
    }
}

/******************************************************************************
*
* Attach an mlec to a pair of windows.
*
* The mlec outputs its text to a 'pane' window attached to some other 'main' window
* and receives characters typed when the pane window has the input focus.
* This call registers the windows with the mlec and installs an event handler and
* an optional menu handler for the pane.
*
******************************************************************************/

#ifdef MLEC_PANE

_Check_return_
extern STATUS
mlec_attach(
    /*_Inout_*/ MLEC mlec,
    _InVal_     wimp_w main_win_handle,
    _InVal_     wimp_w pane_win_handle,
    _InRef_     PC_BBox paneWorkArea)
{
    /*pane_win_handle must be created with mlec__event_handler*/

    mlec->main       = main_win_handle;
    mlec->pane       = pane_win_handle;
#if FALSE
    mlec->paneposx   = paneBB.x0 - mainBB.x0;
    mlec->paneposy   = paneBB.y1 - mainBB.y1;
    mlec->panewidth  = paneBB.x1 - paneBB.x0;
    mlec->paneheight = paneBB.y1 - paneBB.y0;
#endif
    mlec->paneextent = *paneWorkArea;

    return(STATUS_OK);
}

#endif

/******************************************************************************
*
* Deregister the windows with the mlec and un-install the panes event handler
* and menu handler.
*
******************************************************************************/

#ifdef MLEC_PANE

extern void
mlec_detach(
    MLEC mlec)
{
    if(mlec->panemenu)
    {
        /* Remove attached the filler & process routines from the window */
        (void) event_register_window_menumaker(mlec->pane, NULL, NULL, NULL); /* ie remove attachment */

        mlec->panemenu = FALSE;
    }

    mlec->main = window_NULL;
    mlec->pane = window_NULL;   /* ie not attached */
}

#endif

/******************************************************************************
*
* Attach an event handler to an mlec.
*
* Events sent to an mlec pane window (open, redraw, mouse_clicks, key presses etc)
* are normally processed by mlec__event_handler. This routine allows an alternate
* handler to be installed to process SOME of these events, or some unknown events.
*
* Typically this is used to allow the owner of the 'main' window to open/resize it
* when the 'pane' window is openned/resized.
*
******************************************************************************/

extern void
mlec_attach_eventhandler(
    MLEC mlec,
    mlec_event_proc proc,
    P_ANY handle,
    _InVal_     BOOL add)
{
    if(add)
    {
        mlec->callbackproc = proc;
        mlec->callbackhand = handle;
    }
    else
    {
        mlec->callbackproc = NULL;
        mlec->callbackhand = NULL;
    }
}

/******************************************************************************
*
* buffptr   pointer to buffer that receives the text
* buffsize  its size
*
* NB The string returned is CH_NULL terminated (so max strlen actually buffsize-1).
*    The lineterm used (fourth param of text_out) is lineterm_LF.
*    If the buffer is too small, an error is returned, with buffer contents undefined.
*
******************************************************************************/

_Check_return_
extern STATUS
mlec_GetText(
    MLEC mlec,
    P_U8 buffptr,
    _In_        S32 buffsize)
{
    STATUS err;
    marked_text range;
    XFER_HANDLE handle;

    range_is_alltext(mlec, &range);

    if((err = string_write_open(&handle, buffptr, (int) buffsize)) >= 0)
        err = text_out(mlec, &handle, range, lineterm_LF, string_write_size, string_write_putblock, string_write_close);
    return(err);
}

_Check_return_
extern S32
mlec_GetTextLen(
    MLEC mlec)
{
    marked_text range;

    range_is_alltext(mlec, &range);

    /* WARNING the calculation assumes the line terminator specified by mlec_GetText will expand to ONE character */

    return(((S32) range.lower.end - range.lower.start) + ((S32) range.upper.end - range.upper.start));      /* excluding terminator */
}

_Check_return_
static STATUS
string_write_open(
    /*out*/ P_XFER_HANDLE xferhandlep,
    P_U8 buffptr,
    _In_        int buffsiz)
{
    xferhandlep->s.ptr = buffptr;
    xferhandlep->s.siz = buffsiz;
    xferhandlep->s.len = 0;

    return(0);
}

_Check_return_
static STATUS
string_write_size(
    P_XFER_HANDLE xferhandlep,
    _In_        int xfersize)
{
    /* xfersize is the total number of printable chars & lineterm chars that will be output */
    /* by text_out to string_write_putblock, it does NOT include any end-of-text char       */

    if(xferhandlep->s.siz > (xferhandlep->s.len + xfersize))    /* > not >= to allow for eot */
        return(0);

    return(create_error(MLEC_ERR_GETTEXT_BUFOVF));
}

_Check_return_
static STATUS
string_write_putblock(
    P_XFER_HANDLE xferhandlep,
    P_U8 dataptr,
    _In_        int datasize)
{
    if(xferhandlep->s.siz > (xferhandlep->s.len + datasize))
    {
        P_U8 ptr = &(xferhandlep->s.ptr[xferhandlep->s.len]);
        int   i;

        for(i = 0; i < datasize; i++)
            ptr[i] = *dataptr++;

        xferhandlep->s.len += datasize;
        return(0);
    }

    return(create_error(MLEC_ERR_GETTEXT_BUFOVF)); /* shouldn't happen, string_write_size should catch the problem */
}

_Check_return_
static STATUS
string_write_close(
    P_XFER_HANDLE xferhandlep)
{
    /* terminate the string */

    if(xferhandlep->s.siz > xferhandlep->s.len)
    {
        xferhandlep->s.ptr[xferhandlep->s.len] = CH_NULL;
        return(0);
    }

    return(create_error(MLEC_ERR_GETTEXT_BUFOVF)); /* shouldn't happen, string_write_size should catch the problem */
}

_Check_return_
extern STATUS
mlec_SetText(
    MLEC mlec,
    _In_z_      PC_U8Z text)
{
    trace_0(TRACE_MODULE_MLEC, TEXT("mlec_SetText"));

    /* current contents flushed only when we know the new text will fit */
    status_return(checkspace_deletealltext(mlec, strlen(text)));

    /* empty buffer */
    mlec->lower.end   = mlec->lower.start;
    mlec->upper.start = mlec->upper.end;
    mlec->maxcol      = 0;
    mlec->linecount   = 0;

    /* home cursor */
    mlec->cursor.lcol = mlec->cursor.pcol = mlec->cursor.row = 0;

    /* ditch selection */
    mlec->selectvalid = FALSE;

    force_redraw_eotext(mlec); /* cursor (lcol,pcol,row) now (0,0,0), so this redraws whole window */

    mlec__insert_text_core(mlec, text);

    mlec__cursor_texthome(mlec);

    return(STATUS_OK);
}

/*mlec_area_click*/

static mark_position stashed_for_update_markstt;
static mark_position stashed_for_update_markend;

#if CHECKING
static S32 threaded_through_update = FALSE;
#endif

extern void
mlec_area_update(
    MLEC mlec,
    _InRef_     PC_GDI_POINT p_origin,
    _InRef_     PC_GDI_BOX p_screen)
{
    assert(threaded_through_update);

    show_selection(mlec, p_origin, p_screen, stashed_for_update_markstt, stashed_for_update_markend);
}

#ifdef MLEC_PANE

/******************************************************************************
*
* Report an error.
*
* Normally errors in mlec routines are bubbled back to the caller, but with events, such as
* key strokes, menu paste/save or file-dropped-on-window there is no suitable caller,
* so the error must be reported from the event routine.
*
******************************************************************************/

static void
mlec_report_error(
    _InVal_     STATUS err)
{
    reperr(err);
}

/******************************************************************************
*
* Event handler for a multi-line edit control.
*
* This is attached to the editor display window, which is usually
* a pane window linked to a normal window or to a dialog box.
* The event handler processes mouse-clicks, key-presses, messages
* and redraw requests etc, sent to the pane window.
*
******************************************************************************/

/*ncr*/
extern BOOL
mlec__event_handler(
    _InVal_     event_code,
    _In_        const WimpPollBlock * const p_event_data,
    void * handle)
{
    MLEC mlec = (MLEC) handle;

    /* Process the event */
    switch(e->e)
    {
    case Wimp_EOpenWindow:
        trace_0(TRACE_MODULE_MLEC, TEXT("** Open_Window_Request on mlec pane window **"));
        if(mlec__callback(MLEC_CODE_OPEN, mlec, &p_event_data->open_window_request) == 0)
            void_WrapOsErrorReporting(wimp_open_window(&p_event_data->open_window_request));
        break;

    case Wimp_ERedrawWindow:
        mlec__event_redraw_loop(mlec);    /* redraw text & selection */
        break;

    case Wimp_ECloseWindow:
        trace_0(TRACE_MODULE_MLEC, TEXT("** Close_Window_Request on mlec pane window **"));
        if(mlec__callback(MLEC_CODE_CLOSE, mlec, &p_event_data->close_window_request) == 0)
            return(FALSE);
        break;

    case Wimp_EMouseClick:
        if(mlec__callback(MLEC_CODE_CLICK, mlec, &p_event_data->mouse_click) == 0)
            if((int) e->data.mouse_click.icon_handle == -1)      /* work area background */
            {
                GDI_POINT gdi_org;

                trace_3(TRACE_MODULE_MLEC, TEXT("** Mouse_Click on EditBox pane window at (%d,%d), state ") U32_XTFMT TEXT(" **"),
                        e->data.mouse_click.mouse_x, e->data.mouse_click.mouse_y, e->data.mouse_click.buttons);

                host_gdi_org_from_screen(&gdi_org, e->data.mouse_click.window_handle); /* window w.a. ABS origin */

                mlec__click_core(mlec, &gdi_org, &e->data.mouse_click);
            }
        break;

    case Wimp_EUserDrag:
        /* Returned when a 'User_Drag' operation (started by winx_drag_box) completes */
        mlec__drag_complete(mlec, &p_event_data->user_drag_box);
        break;

    case Wimp_EKeyPressed:
        {
        STATUS status;

        trace_1(TRACE_MODULE_MLEC, TEXT("** Wimp_EKeyPressed on EditBox pane window, key code=%d **"), p_event_data->key.chcode);

        status = mlec__key_core(mlec, ri_kmap_convert(p_event_data->key.chcode));

        /* Any error is the result of direct user interaction with the mlec, */
        /* so report the error here, cos there is no caller to return it to. */
        if(status_fail(status))
        {
            mlec_report_error(status);
            status = STATUS_DONE; /* prevent any further processing */
        }

        return(status_done(err));
        }

#if 0
    case Wimp_EUserMessage:
    case Wimp_EUserMessageRecorded:
        trace_1(TRACE_MODULE_MLEC, TEXT("action is %d"), p_event_data->msg.hdr.action);
        switch(e->data.msg.hdr.action)
        {
        case Wimp_MDataLoad:   /* File dragged from file viewer, dropped on our window */
        case Wimp_MDataOpen:   /* File double clicked in file viewer */
            {
            PCTSTR filename;

            host_xfer_load_file_setup(&filename);
            {
                int err = mlec__atcursor_load(mlec, filename);
                if(err < 0)
                    mlec_report_error(err);    /* Report the error here, cos there is no caller to return it to. */
            }
            break;
            }

        case Wimp_MDataSave:    /* File dragged from another application, dropped on our window */
            host_xfer_import_via_scrap();
            break;

        default:
            return(FALSE);
        }
        break;
#endif

    default:
        return(FALSE);
    }

    /* done something, so... */
    return(TRUE);
}

#endif

/*>>>split out key code and have mlec__key_press*/

/******************************************************************************
*
* Process mouse click, double-click and drag events.
*
******************************************************************************/

extern void
mlec__click_core(
    MLEC mlec,
    P_GDI_POINT p_origin,
    _In_        const WimpMouseClickEvent * const p_mouse_click)
{
    GDI_POINT point;
    int col, row;

    point.x = p_mouse_click->mouse_x - p_origin->x;       /* mouse position relative to */
    point.y = p_mouse_click->mouse_y - p_origin->y;       /* window (ie EditBox) origin */

    mlec__colrow_from_point(mlec, &point, &col, &row);

    trace_2(TRACE_MODULE_MLEC, TEXT("(%d,%d)"),col, row);

    /* <<< all should detect whether mlec is caret owner, and if not, behave as left click */

    /* Decode the mouse buttons */
    switch(p_mouse_click->buttons)
    {
    default:
        break;

    case Wimp_MouseButtonSingleSelect:  /* 0x400 Single 'select' */
        mlec__cursor_setpos(mlec, col, row);
        break;

    case Wimp_MouseButtonDragSelect:    /* 0x040 Long   'select' */
        mlec__cursor_setpos(mlec, col, row);
        mlec__drag_start(mlec);
        break;

    case Wimp_MouseButtonSelect:        /* 0x004 Double 'select' */
        mlec__cursor_setpos(mlec, col, row);
        mlec__select_word(mlec);
        break;

    case Wimp_MouseButtonTripleSelect:  /* 0x4000 Triple 'select' */
        mlec__cursor_setpos(mlec, col, row);
        mlec__select_para(mlec);
        break;

    case Wimp_MouseButtonSingleAdjust:  /* 0x100 Single 'adjust' */
        /* Alter selection, (will create one if needed, starting at cursor position) */
        mlec__selection_adjust(mlec, col, row);
        break;

    case Wimp_MouseButtonDragAdjust:    /* 0x010 Long   'adjust' */
        mlec__selection_adjust(mlec, col, row);
        mlec__drag_start(mlec);
        break;

    case Wimp_MouseButtonAdjust:        /* 0x001 Double 'adjust' */
        mlec__selection_adjust(mlec, col, row);
        mlec__select_word(mlec);
        break;
    }
}

extern void
mlec__drag_core(
    MLEC mlec,
    _InRef_     PC_GDI_POINT p_origin,
    _In_        const WimpGetPointerInfoBlock * const p_pointer_info)
{
    GDI_POINT point;
    int col, row;

    point.x = p_pointer_info->x - p_origin->x;  /* mouse position relative to */
    point.y = p_pointer_info->y - p_origin->y;  /* window origin              */

    mlec__colrow_from_point(mlec, &point, &col, &row);

    mlec__selection_adjust(mlec, col, row);
}

/******************************************************************************
*
* returns:
*   < 0  error,    key should be treated as processed
*   = 0  no error, key not processed
*   > 0  no error, key processed
*
* The callers event handler should look like this:
*
*       status = mlec__key_core(mlec, kmap_code);
*
*       if(status < 0)                  report any error
*           mlec_report_error(status);
*
*       return(status != 0);            returns TRUE if key processed
*
******************************************************************************/

_Check_return_
extern STATUS
mlec__key_core(
    MLEC mlec,
    _In_        KMAP_CODE kmap_code)
{
    STATUS status;

    status_return(status = mlec__callback(MLEC_CODE_KEY, mlec, &kmap_code));
    if(status != STATUS_OK)
        return(STATUS_DONE);

    status = STATUS_DONE;

    switch(kmap_code)
    {
    case KMAP_FUNC_ARROW_LEFT:            mlec__cursor_left     (mlec); break;
    case KMAP_FUNC_ARROW_RIGHT:           mlec__cursor_right    (mlec); break;
    case KMAP_FUNC_ARROW_UP:              mlec__cursor_up       (mlec); break;
    case KMAP_FUNC_ARROW_DOWN:            mlec__cursor_down     (mlec); break;

    case KMAP_FUNC_HOME:
    case KMAP_FUNC_CARROW_LEFT:           mlec__cursor_linehome (mlec); break;
    case KMAP_FUNC_CARROW_RIGHT:          mlec__cursor_lineend  (mlec); break;
    case KMAP_FUNC_CHOME:
    case KMAP_FUNC_CARROW_UP:             mlec__cursor_texthome (mlec); break;
    case KMAP_FUNC_CARROW_DOWN:           mlec__cursor_textend  (mlec); break;

    case KMAP_FUNC_SARROW_LEFT:           mlec__cursor_wordleft (mlec); break;
    case KMAP_FUNC_SARROW_RIGHT:          mlec__cursor_wordright(mlec); break;

    case KMAP_FUNC_TAB:                   status_break(status = mlec__insert_tab(mlec)); status = STATUS_DONE; break;
    case KMAP_FUNC_STAB:                  mlec__cursor_tab_left (mlec); break;

    case KMAP_FUNC_END:                   mlec__delete_right    (mlec); break;      /* Copy and End are same key */
    case KMAP_FUNC_SEND:                  mlec__delete_lineend  (mlec); break;
    case KMAP_FUNC_CEND:                  mlec__delete_line     (mlec); break;
    case KMAP_FUNC_CSEND:                 mlec__delete_linehome (mlec); break;

    case KMAP_FUNC_BACKSPACE:             mlec__delete_left     (mlec); break;

#if defined(MLEC_CLIP)
    case KMAP_FUNC_SINSERT:               status_break(status = mlec__atcursor_paste(mlec)); status = STATUS_DONE; break;
    case KMAP_FUNC_CINSERT:               status_break(status = mlec__selection_copy(mlec)); status = STATUS_DONE; break;
#endif

    case KMAP_FUNC_DELETE:                mlec__delete_left     (mlec); break;
#if defined(MLEC_CLIP)
    case KMAP_FUNC_SDELETE:               status_break(status = mlec__selection_cut(mlec)); status = STATUS_DONE; break;
#endif
    case KMAP_CODE_ADDED_ALT | 'Z':       mlec__selection_clear (mlec); break;

    case KMAP_FUNC_RETURN:                if(STATUS_OK != (status = mlec__callback(MLEC_CODE_KEY_RETURN, mlec, NULL)))
                                              break;

        /*FALLTHRU*/

    case KMAP_FUNC_SRETURN:
    case KMAP_FUNC_CRETURN:
    case KMAP_FUNC_CSRETURN:              status_break(status = mlec__insert_newline(mlec)); status = STATUS_DONE; break;

    case KMAP_ESCAPE:                     status = mlec__callback(MLEC_CODE_KEY_ESCAPE, mlec, NULL); break;

    default:
        if(kmap_code >= 0x100)
            status = STATUS_OK; /* NOT processed */
        else
        {
            status_break(status = mlec__insert_char(mlec, (U8) kmap_code));
            status = STATUS_DONE;
        }
        break;
    }

    return((status <= 0) ? status : STATUS_DONE);
}

#ifdef MLEC_PANE

/******************************************************************************
*
* Redraw any invalid rectangles in the pane window
*
* This renders text and highlights any selection
*
******************************************************************************/

static void
mlec__event_redraw_loop(
    /*_Inout_*/ MLEC mlec)
{
    WimpRedrawWindowBlock redraw_window;
    int wimp_more;
    GDI_POINT gdi_org;

    trace_0(TRACE_MODULE_MLEC, TEXT("** mlec__event_redraw_loop called **"));

    /* Start the redraw */
    r.window_handle = mlec->pane;
    if(WrapOsErrorReporting(wimp_redraw_window(&r, &wimp_more)))
        wimp_more = 0;

    trace_4(TRACE_MODULE_MLEC, TEXT("wimp_redraw_window returns: (%d,%d,%d,%d) "),r.visible_area.x0,r.visible_area.y0,r.visible_area.x1,r.visible_area.y1);
    trace_2(TRACE_MODULE_MLEC, TEXT("(%d,%d) "),r.scx,r.scy);
    trace_4(TRACE_MODULE_MLEC, TEXT("(%d,%d,%d,%d)"),r.g.x0,r.g.y0,r.g.x1,r.g.y1);

    gdi_org.x = work_area_origin_x_from_visible_area_and_scroll(&r); /* window w.a. ABS origin */
    gdi_org.y = work_area_origin_y_from_visible_area_and_scroll(&r);

    /* Do the redraw loop */
    while(0 != wimp_more)
    {
        mlec__redraw_core(mlec, &gdi_org, (PC_GDI_BOX) &r.g /*screenBB*/);

        if(WrapOsErrorReporting(wimp_get_rectangle(&r, &wimp_more)))
            wimp_more = 0;
    }
}

#endif

/*
* possibly bg colour needs to have been set up prior to call
*/

extern void
mlec__redraw_core(
    MLEC mlec,
    _InRef_     PC_GDI_POINT p_origin,
    _InRef_     PC_GDI_BOX p_screen)
{
    /*screen*/      /* bounding box of a region of the screen that needs updating */
    GDI_BOX lineBB; /* partial bounding box of a line considered for rendering    */
    GDI_BOX cursor; /* all characters on cursor row, to the left of the cursor    */

    int lineCol, lineRow;

    P_U8 lower_start;
    P_U8 lower_row;
    P_U8 lower_end;
    P_U8 upper_start;
    P_U8 upper_end;

    P_U8 linestart;
    P_U8 lineend;
    P_U8 ptr;

    RGB rgb_foreground, rgb_background;

    /* these are only valid if mlec->selectvalid == TRUE */
    mark_position markstt = { 0, 0 }; /* Keep dataflower happy */
    mark_position markend = { 0, 0 };

    if(mlec->selectvalid)
    {
        if((mlec->selectanchor.row < mlec->cursor.row) ||
           ((mlec->selectanchor.row == mlec->cursor.row) && (mlec->selectanchor.col < mlec->cursor.pcol))
          )
        {
            markstt = mlec->selectanchor; markend.col = mlec->cursor.pcol; markend.row = mlec->cursor.row;
        }
        else
        {
            markstt.col = mlec->cursor.pcol; markstt.row = mlec->cursor.row; markend = mlec->selectanchor;
        }
    }

    lower_start = &mlec->buffptr[mlec->lower.start];                    /* first character                         */
    lower_row   = &mlec->buffptr[mlec->lower.end - mlec->cursor.pcol];  /* character at (0,cursor.row)             */
    lower_end   = &mlec->buffptr[mlec->lower.end];                      /* 1 byte past character to left of cursor */
    upper_start = &mlec->buffptr[mlec->upper.start];                    /* character to right of cursor            */
    upper_end   = &mlec->buffptr[mlec->upper.end];                      /* 1 byte past last character              */

    /* bounding box of characters 0..cursor.pcol (inc,exc) on row cursor.row */

    mlec__point_from_colrow(mlec, (P_GDI_POINT) &cursor.x0, 0, mlec->cursor.row);
    cursor.x1 = cursor.x0 + mlec->charwidth * mlec->cursor.pcol;
    cursor.y1 = cursor.y0;
    cursor.y0 = cursor.y1 - mlec->attributes[MLEC_ATTRIBUTE_CHARHEIGHT];

    cursor.x0 += p_origin->x;
    cursor.y0 += p_origin->y;
    cursor.x1 += p_origin->x;
    cursor.y1 += p_origin->y;

    * (P_S32) &rgb_background = mlec->attributes[MLEC_ATTRIBUTE_BG_RGB];
    * (P_S32) &rgb_foreground = mlec->attributes[MLEC_ATTRIBUTE_FG_RGB];

    if(HOST_FONT_NONE != mlec->host_font)
        status_assert(host_setfontcolours_for_mlec(&rgb_foreground, &rgb_background));
    else
    {
        if(!host_setfgcolour(&rgb_foreground))
            return;
    }

    {
    /* Render characters to the right of the cursor and the lines below it */

    lineCol = mlec->cursor.pcol;
    lineRow = mlec->cursor.row;

    lineBB.x0 = lineBB.x1 = cursor.x1;
    lineBB.y1 = cursor.y1;
    lineBB.y0 = lineBB.y1 - mlec->attributes[MLEC_ATTRIBUTE_CHARHEIGHT];

    ptr = upper_start;

    /* Skip lines that are above the current graphics window */
    while((lineBB.y0 > p_screen->y1) && (ptr < upper_end))
    {
        /* line is above the region to be redrawn, so skip it */
        while(ptr < upper_end)
        {
            if(CR == *ptr++)
            {
                lineCol = 0;
                lineRow++;

                lineBB.x0 = cursor.x0;
                lineBB.y1 = lineBB.y1 - mlec->attributes[MLEC_ATTRIBUTE_LINESPACE];
                lineBB.y0 = lineBB.y1 - mlec->attributes[MLEC_ATTRIBUTE_CHARHEIGHT];
                break;
            }
        }
    }

    /* Render lines that fall within the current graphics window */
    while((lineBB.y1 > p_screen->y0) && (ptr < upper_end))
    {
        /* line within redraw region, so render it */
        ptr = render_line(mlec, lineCol, lineBB.x0, lineBB.y1, p_screen, ptr, upper_end);

        lineCol = 0;
        lineRow++;

        lineBB.x0 = cursor.x0;
        lineBB.y1 = lineBB.y1 - mlec->attributes[MLEC_ATTRIBUTE_LINESPACE];
        lineBB.y0 = lineBB.y1 - mlec->attributes[MLEC_ATTRIBUTE_CHARHEIGHT];
    }

    /* remaining lines are below the graphics window */
    } /*block*/

    {
    /* Render characters to the left of the cursor and the lines above it */

    lineCol = 0;
    lineRow = mlec->cursor.row;

    lineBB.x0 = lineBB.x1 = cursor.x0;
    lineBB.y1 = cursor.y1;
    lineBB.y0 = lineBB.y1 - mlec->attributes[MLEC_ATTRIBUTE_CHARHEIGHT];

    linestart = lower_row;
    lineend   = lower_end;

    /* Skip lines that are below the current graphics window */
    while((lineBB.y1 < p_screen->y0) && (linestart >= lower_start))
    {
        /* line is below region to be redrawn, so skip it */
        lineend = --linestart;          /* point at terminator for previous line */
        while((linestart > lower_start) && (CR != *(linestart - 1)))
            --linestart;

        lineRow--;

        lineBB.y1 = lineBB.y1 + mlec->attributes[MLEC_ATTRIBUTE_LINESPACE];
        lineBB.y0 = lineBB.y1 - mlec->attributes[MLEC_ATTRIBUTE_CHARHEIGHT];
    }

    /* Render lines that fall within the current graphics window */
    while((lineBB.y0 < p_screen->y1) && (linestart >= lower_start))
    {
        /* line within redraw region, so render it */
        ptr = linestart;

        ptr = render_line(mlec, lineCol, lineBB.x0, lineBB.y1, p_screen, ptr, lineend);

        lineend = --linestart;          /* point at terminator for previous line */

        while((linestart > lower_start) && (CR != *(linestart - 1)))
            --linestart;

        lineRow--;

        lineBB.y1 = lineBB.y1 + mlec->attributes[MLEC_ATTRIBUTE_LINESPACE];
        lineBB.y0 = lineBB.y1 - mlec->attributes[MLEC_ATTRIBUTE_CHARHEIGHT];
    }

    /* remaining lines are above the graphics window */

    if(mlec->selectvalid)
        show_selection(mlec, p_origin, p_screen, markstt, markend);
    } /*block*/

#if FALSE
    {
    int x = orgx + mlec->attributes[MLEC_ATTRIBUTE_MARGIN_LEFT] + mlec->charwidth * mlec->maxcol;
    int y = orgy - mlec->attributes[MLEC_ATTRIBUTE_MARGIN_TOP]  - mlec->attributes[MLEC_ATTRIBUTE_LINESPACE] * (mlec->linecount + 1);      /* lowest scan line on last row */

    bbc_move(orgx, y - 1);
    bbc_drawby(200 * mlec->charwidth, 0);       /* draw line on first unused scanline */
    bbc_move(x, y);
    bbc_drawby(0, 200);
    } /*block*/
#endif
}

/******************************************************************************
*
* Cursor movement routines
*
* Each one should clear the selection
*
******************************************************************************/

static void
mlec__cursor_down(
    /*_Inout_*/ MLEC mlec)
{
    P_U8 buff = mlec->buffptr;
    BOOL found;
    int  i;

    clear_selection(mlec);

    /* Can only move down if current line has a terminator */
    for(found = FALSE, i = mlec->upper.start; i < mlec->upper.end && !found; i++)
        found = (buff[i] == CR);

    if(found)
    {
        /* move rest of line including terminator */
        while(CR != (buff[mlec->lower.end++] = buff[mlec->upper.start++]))
            /* null statement */;

        /* wrap cursor to beginning of next line */
        mlec->cursor.row++;
        mlec->cursor.pcol = 0;

        /* try moving to logical cursor position, must stop at eol or eot */
        while(
              (mlec->cursor.pcol < mlec->cursor.lcol) &&
              (mlec->upper.start < mlec->upper.end  ) &&
              (CR != buff[mlec->upper.start])
             )
        {
            buff[mlec->lower.end++] = buff[mlec->upper.start++];
            mlec->cursor.pcol++;
        }
    }
  /*else             */
  /*    on last line */

    scroll_until_cursor_visible(mlec);
}

static void
mlec__cursor_left(
    /*_Inout_*/ MLEC mlec)
{
    clear_selection(mlec);

    if(mlec->lower.end > mlec->lower.start)
    {
        P_U8 buff = mlec->buffptr;
        char  ch;

        ch = buff[--mlec->upper.start] = buff[--mlec->lower.end];

        if(ch == CR)
        {
            int i;
            /* wrap cursor to end of previous line */

            /* which means finding the line length */
            mlec->cursor.pcol = 0;
            for(i = mlec->lower.end; i > mlec->lower.start; mlec->cursor.pcol++)
            {
                if(buff[--i] == CR)
                    break;
            }

            --mlec->cursor.row;
        }
        else
        {
            /* ordinary character */
            --mlec->cursor.pcol;
        }
        mlec->cursor.lcol = mlec->cursor.pcol;
    }
  /*else                 */
  /*    at start of text */

    scroll_until_cursor_visible(mlec);
}

static void
mlec__cursor_right(
    /*_Inout_*/ MLEC mlec)
{
    clear_selection(mlec);

    if(mlec->upper.start < mlec->upper.end)
    {
        P_U8 buff = mlec->buffptr;
        char  ch;

        ch = buff[mlec->lower.end++] = buff[mlec->upper.start++];

        if(ch == CR)
        {
            /* wrap cursor to beginning of next line */
            mlec->cursor.pcol = 0;
            mlec->cursor.row++;
        }
        else
        {
            /* ordinary character */
            mlec->cursor.pcol++;
        }
        mlec->cursor.lcol = mlec->cursor.pcol;
    }
  /*else               */
  /*    at end of text */

    scroll_until_cursor_visible(mlec);
}

static void
mlec__cursor_up(
    /*_Inout_*/ MLEC mlec)
{
    clear_selection(mlec);

    if(mlec->cursor.row > 0)
    {
        P_U8 buff = mlec->buffptr;
        int   i;

        /* move rest of line including terminator */
        while(CR != (buff[--mlec->upper.start] = buff[--mlec->lower.end]))
            /* null statement */;

        /* wrap cursor to end of previous line */

        /* which means finding the line length */
        mlec->cursor.pcol = 0;
        for(i = mlec->lower.end; i > mlec->lower.start; mlec->cursor.pcol++)
        {
            if(buff[--i] == CR)
                break;
        }

        --mlec->cursor.row;

        /* try moving to logical cursor postion */
        while(mlec->cursor.pcol > mlec->cursor.lcol)
        {
            buff[--mlec->upper.start] = buff[--mlec->lower.end];
            --mlec->cursor.pcol;
        }
    }
  /*else            */
  /*    on top line */

    scroll_until_cursor_visible(mlec);
}

static void
mlec__cursor_lineend(
    /*_Inout_*/ MLEC mlec)
{
    P_U8 buff = mlec->buffptr;

    clear_selection(mlec);

    if(
       (mlec->upper.start < mlec->upper.end) &&
       (CR != buff[mlec->upper.start])
       )
    {
        while(
              (mlec->upper.start < mlec->upper.end) &&
              (CR != buff[mlec->upper.start])
             )
        {
            buff[mlec->lower.end++] = buff[mlec->upper.start++];
            mlec->cursor.pcol++;
        }
        mlec->cursor.lcol = mlec->cursor.pcol;
    }
  /*else                              */
  /*    at end of text or end of line */

    scroll_until_cursor_visible(mlec);
}

static void
mlec__cursor_linehome(
    /*_Inout_*/ MLEC mlec)
{
    clear_selection(mlec);

    if(mlec->cursor.pcol > 0)
    {
        P_U8 buff = mlec->buffptr;

        /* move cursor to start of line */
        while(mlec->cursor.pcol > 0)
        {
            buff[--mlec->upper.start] = buff[--mlec->lower.end];
            --mlec->cursor.pcol;
        }
        mlec->cursor.lcol = mlec->cursor.pcol;  /* snap logical col to physical col */
    }
  /*else                 */
  /*    at start of line */

    scroll_until_cursor_visible(mlec);
}

/******************************************************************************
*
* Move the cursor left to the previous tab position
*
* ie move 1..(TAB_MASK+1) places
*
******************************************************************************/

static void
mlec__cursor_tab_left(
    /*_Inout_*/ MLEC mlec)
{
    clear_selection(mlec);

    if(mlec->cursor.pcol > 0)
    {
        P_U8 buff = mlec->buffptr;

        /* move left (at least one) to tab-stop */
        do  {
            buff[--mlec->upper.start] = buff[--mlec->lower.end];
            --mlec->cursor.pcol;
        }
        while(mlec->cursor.pcol & TAB_MASK);

        mlec->cursor.lcol = mlec->cursor.pcol;  /* snap logical col to physical col */
    }
  /*else                 */
  /*    at start of line */

    scroll_until_cursor_visible(mlec);
}

extern void
mlec__cursor_textend(
    MLEC mlec)
{
    clear_selection(mlec);

    if(mlec->upper.start < mlec->upper.end)
    {
        P_U8 buff = mlec->buffptr;

        while(mlec->upper.start < mlec->upper.end)
        {
            if(CR == (buff[mlec->lower.end++] = buff[mlec->upper.start++]))
            {
                mlec->cursor.pcol = 0;
                mlec->cursor.row++;
            }
            else
            {
                /* ordinary character */
                mlec->cursor.pcol++;
            }
        }
        mlec->cursor.lcol = mlec->cursor.pcol;
    }
  /*else               */
  /*    at end of text */

    scroll_until_cursor_visible(mlec);
}

extern void
mlec__cursor_texthome(
    MLEC mlec)
{
    clear_selection(mlec);

    if(mlec->lower.end > mlec->lower.start)
    {
        /* SKS 09jan93: while we're at it let's free any excess core too */
        int excess = mlec->upper.start - mlec->lower.end; /*+ve*/

        (void) flex_midextend((flex_ptr) &mlec->buffptr, mlec->lower.end + excess, -excess);

        mlec->lower.end = mlec->lower.start;
        mlec->upper.end -= excess;
        mlec->upper.start = mlec->lower.end;

        mlec->cursor.lcol = mlec->cursor.pcol = 0;
        mlec->cursor.row  = 0;
    }
  /*else                 */
  /*    at start of text */

    scroll_until_cursor_visible(mlec);
}

extern void
mlec__cursor_getpos(
    MLEC mlec,
    _OutRef_    P_S32 colp,
    _OutRef_    P_S32 rowp)
{
    *colp = mlec->cursor.pcol;
    *rowp = mlec->cursor.row;        /* NB returns physical cursor posn. */
}

extern void
mlec__cursor_setpos(
    MLEC mlec,
    _InVal_     S32 col,
    _InVal_     S32 row)
{
    trace_2(TRACE_MODULE_MLEC, TEXT("(%d,%d)"),col,row);

    clear_selection(mlec);

    move_cursor(mlec, col, row);                                /* Cursor moved as close as possible */
                                                                /* to (col,row). Sets lcol = pcol    */
    scroll_until_cursor_visible(mlec);
}

static void
mlec__cursor_wordleft(
    /*_Inout_*/ MLEC mlec)
{
    mark_position wordstart;

    word_left(mlec, &wordstart);

    if(mlec->cursor.pcol == wordstart.col)
    {
        /* cursor at start of line */
        mlec__cursor_left(mlec);        /* will wrap cursor to end of previous line */
    }
    else
        mlec__cursor_setpos(mlec, wordstart.col, wordstart.row);
}

static void
mlec__cursor_wordright(
    /*_Inout_*/ MLEC mlec)
{
    mark_position wordstart, wordend;

    word_limits(mlec, &wordstart, &wordend);

    if(mlec->cursor.pcol == wordend.col)
    {
        /* cursor at eoline */
        mlec__cursor_right(mlec);       /* will wrap cursor to beginning of next line */
    }
    else
        mlec__cursor_setpos(mlec, wordend.col, wordend.row);
}

/******************************************************************************
*
* Character and block text insertion routines
*
******************************************************************************/

/******************************************************************************
*
* Insert the supplied character at the cursor position, deleting any previously selected text.
*
* The action of non printable characters is somewhat undefined.
*
* Aside: To prevent screen redraw problems, this implementation replaces non printable
*        characters with a dot, but this is subject to change.
*
******************************************************************************/

_Check_return_
extern STATUS
mlec__insert_char(
    MLEC mlec,
    _InVal_     U8 ch)
{
    P_U8 buff;

    status_return(checkspace_delete_selection(mlec, sizeof32(char)));

    buff = mlec->buffptr;

    force_redraw_eoline(mlec);     /* will execute later on */

    buff[mlec->lower.end++] = ch;
    mlec->cursor.pcol++;

    mlec->cursor.lcol = mlec->cursor.pcol;  /* snap logical col to physical col */

    mlec__issue_update(mlec);

    scroll_until_cursor_visible(mlec);

    return(STATUS_OK);
}

_Check_return_
extern STATUS
mlec__insert_newline(
    MLEC mlec)
{
    P_U8 buff;

    status_return(checkspace_delete_selection(mlec, sizeof32(char)));

    buff = mlec->buffptr;

    force_redraw_eotext(mlec);      /* will execute later on */

    buff[mlec->lower.end++] = CR;
    mlec->cursor.lcol = mlec->cursor.pcol = 0;  /* snap logical col and physical col to start of next line */
    mlec->cursor.row++;
    mlec->linecount++;

    mlec__issue_update(mlec);

    scroll_until_cursor_visible(mlec);

    return(STATUS_OK);
}

/******************************************************************************
*
* Insert spaces upto the next tab position
*
* ie insert 1..(TAB_MASK+1) spaces
*
******************************************************************************/

_Check_return_
static STATUS
mlec__insert_tab(
    /*_Inout_*/ MLEC mlec)
{
    P_U8 buff;

    status_return(checkspace_delete_selection(mlec, 1+TAB_MASK));

    buff = mlec->buffptr;

    force_redraw_eoline(mlec);      /* will execute later on */

    do  {
        buff[mlec->lower.end++] = CH_SPACE;
        mlec->cursor.pcol++;
    }
    while(mlec->cursor.pcol & TAB_MASK);

    mlec->cursor.lcol = mlec->cursor.pcol;  /* snap logical col to physical col */

    mlec__issue_update(mlec);

    scroll_until_cursor_visible(mlec);

    return(STATUS_OK);
}

/******************************************************************************
*
* Insert the supplied text at the cursor position, deleting any previously selected text.
* The supplied text should be CH_NULL-terminated, and may contain
* CR, LF, CRLF or LFCR as line-break sequences.
*
******************************************************************************/

static void
mlec__insert_text_core(
    /*_Inout_*/ MLEC mlec,
    _In_z_      PC_U8Z text)
{
    U8   ch;
    P_U8 buff = mlec->buffptr;
    int  col  = mlec->cursor.pcol;
    int  row  = mlec->cursor.row;
    int  lend = mlec->lower.end;

    while((ch = *text++) != 0)
    {
        switch(ch)
        {
        case CR:
        case LF:
            if(*text == (ch ^ (CR ^ LF)))
                ++text;
            ch = CR; /*>>>would zero be better???*/
            col = 0; row++; mlec->linecount++;
            break;

        default:
            col++;
            break;
        }

        buff[lend++] = ch;
    }

    mlec->cursor.lcol = mlec->cursor.pcol  = col;
    mlec->cursor.row  = row;

    mlec->lower.end   = lend;

    mlec__issue_update(mlec);
}

_Check_return_
extern STATUS
mlec__insert_text(
    MLEC mlec,
    _In_z_      PC_U8Z text)
{
    status_return(checkspace_delete_selection(mlec, strlen(text)));

    force_redraw_eotext(mlec);      /* will execute later on */

    mlec__insert_text_core(mlec, text);

    scroll_until_cursor_visible(mlec);

    return(STATUS_OK);
}

/******************************************************************************
*
* Character and line deletion routines
*
******************************************************************************/

static void
mlec__delete_left(
    /*_Inout_*/ MLEC mlec)
{
    remove_selection(mlec);                     /* if got selection, delete it, scroll_until_cursor_visible, then quit */
                                                /* else delete char left of cursor                                     */
    if(mlec->lower.end > mlec->lower.start)
    {
        P_U8 buff = mlec->buffptr;
        int   i;

        if(CR == buff[--mlec->lower.end])
        {
            /* cursor moves to what was the end of the previous line */

            /* which means finding its length */
            mlec->cursor.pcol = 0;
            for(i = mlec->lower.end; i > mlec->lower.start; mlec->cursor.pcol++)
            {
                if(buff[--i] == CR)
                    break;
            }

            --mlec->cursor.row;
            force_redraw_eotext(mlec);
            --mlec->linecount;
        }
        else
        {
            /* ordinary character */
            --mlec->cursor.pcol;
            force_redraw_eoline(mlec);
        }

        mlec->cursor.lcol = mlec->cursor.pcol;

        mlec__issue_update(mlec);
    }
  /*else                 */
  /*    at start of text */

    scroll_until_cursor_visible(mlec);
}

static void
mlec__delete_right(
    /*_Inout_*/ MLEC mlec)
{
    remove_selection(mlec);                     /* if got selection, delete it, scroll_until_cursor_visible, then quit */
                                                /* else delete char right of cursor                                    */
    if(mlec->upper.start < mlec->upper.end)
    {
        P_U8 buff = mlec->buffptr;

        if(CR == buff[mlec->upper.start++])
        {
            force_redraw_eotext(mlec);
            --mlec->linecount;
        }
        else
        {
            /* ordinary character */
            force_redraw_eoline(mlec);
        }

        mlec->cursor.lcol = mlec->cursor.pcol;  /* cos lcol may have been past old eol */

        mlec__issue_update(mlec);
    }
  /*else               */
  /*    at end of text */

    scroll_until_cursor_visible(mlec);
}

static void
mlec__delete_line(
    /*_Inout_*/ MLEC mlec)
{
    remove_selection(mlec);                     /* if got selection, delete it, scroll_until_cursor_visible, then quit */
                                                /* else delete cursor line                                             */
    {
    P_U8 buff = mlec->buffptr;
    BOOL  done = FALSE;

    if(mlec->cursor.pcol > 0)
    {
        done = TRUE;

        /* skip cursor to start of line, chucking chars on floor as we go */
        mlec->lower.end   -= mlec->cursor.pcol;
        mlec->cursor.lcol  = mlec->cursor.pcol = 0;
    }

    while(mlec->upper.start < mlec->upper.end)
    {
        done = TRUE;

        if(CR == buff[mlec->upper.start++])     /* chuck char on floor   */
        {
            --mlec->linecount;
            break;                              /* finish if it was a CR */
        }
    }

    if(done)
    {
        force_redraw_eotext(mlec);

        mlec->cursor.lcol = mlec->cursor.pcol;

        mlec__issue_update(mlec);
    }
  /*else                              */
  /*    no text or on empty last line */
    } /*block*/

    scroll_until_cursor_visible(mlec);
}

static void
mlec__delete_lineend(
    /*_Inout_*/ MLEC mlec)
{
    remove_selection(mlec);                     /* if got selection, delete it, scroll_until_cursor_visible, then quit */
                                                /* else delete from cursor to end-of-line                              */
    {
    P_U8 buff = mlec->buffptr;
    BOOL  done = FALSE;

    while(
          (mlec->upper.start < mlec->upper.end) &&
          (CR != buff[mlec->upper.start])
         )
    {
        done = TRUE;
        mlec->upper.start++;    /* chuck char on floor */
    }

    if(done)
    {
        force_redraw_eoline(mlec);

        mlec->cursor.lcol = mlec->cursor.pcol; /* be paranoid, should already be snapped */

        mlec__issue_update(mlec);
    }
  /*else                              */
  /*    at end of text or end of line */
    } /*block*/

    scroll_until_cursor_visible(mlec);
}

static void
mlec__delete_linehome(
    /*_Inout_*/ MLEC mlec)
{
    remove_selection(mlec);                     /* if got selection, delete it, scroll_until_cursor_visible, then quit */
                                                /* else delete from cursor to start-of-line                            */
    if(mlec->cursor.pcol > 0)
    {
        /* skip cursor to start of line, chucking chars on floor as we go */
        mlec->lower.end   -= mlec->cursor.pcol;
        mlec->cursor.lcol  = mlec->cursor.pcol = 0;

        force_redraw_eoline(mlec);

        mlec__issue_update(mlec);
    }
  /*else                 */
  /*    at start of line */

    scroll_until_cursor_visible(mlec);
}

/******************************************************************************
*
* Claim the input focus for this edit control and place the caret at its cursor position.
*
******************************************************************************/

void
mlec_claim_focus(
    MLEC mlec)
{
    WimpCaret carrot; /* nyeeeer, whats up doc? */

    trace_0(TRACE_MODULE_MLEC, TEXT("mlec_claim_focus - "));

    build_caretstr(mlec, &carrot);

    if(mlec__callback(MLEC_CODE_CLAIMFOCUS, mlec, &carrot) != 0)
        return;

    /*CONSTANTCONDITION*/
    if_pane(mlec)
    {
        WimpCaret current;

        void_WrapOsErrorReporting(wimp_get_caret_position(&current));

        /* The the caret isn't exactly where we want it (and in the correct state), make it so. */

        if((current.window_handle != mlec->pane) ||
           (current.xoffset != carrot.xoffset)   ||
           (current.yoffset != carrot.yoffset)   ||
           (current.height != carrot.height)        /* 'cos this field holds caret (in)visible bit */
          )
        {
            void_WrapOsErrorReporting(wimp_set_caret_position_block(&carrot));
            trace_3(TRACE_MODULE_MLEC, TEXT("place caret (%d,%d,%d)"),carrot.xoffset,carrot.yoffset,carrot.height);
        }
        else
        {
            trace_0(TRACE_MODULE_MLEC, TEXT(" no action (we own input focus, caret already positioned)"));
        }
    }
}

static void
scroll_until_cursor_visible(
    /*_Inout_*/ MLEC mlec)
{
    trace_0(TRACE_MODULE_MLEC, TEXT("scroll_until_cursor_visible - "));

#ifdef MLEC_PANE
    /* May need to enlarge the pane window (change its extent) to fit the text. */
    /*CONSTANTCONDITION*/
    if_pane(mlec)
    {
        BOOL change = FALSE;
        GDI_POINT extent;

        mlec__point_from_colrow(mlec, &extent, mlec->maxcol, mlec->linecount + 1);

        trace_4(TRACE_MODULE_MLEC, TEXT("window extent (%d,%d,%d,%d)"),mlec->paneextent.x0,mlec->paneextent.y0,mlec->paneextent.x1,mlec->paneextent.y1);
        trace_1(TRACE_MODULE_MLEC, TEXT("extenty=%d"),extent.y);

        if(mlec->paneextent.x1 < extent.x)
        {
            mlec->paneextent.x1 = extent.x;
            change = TRUE;
        }

        if(mlec->paneextent.y0 > extent.y)       /* NB -ve numbers */
        {
            mlec->paneextent.y0 = extent.y;
            change = TRUE;
        }

        if(change)
        {
            WimpRedrawWindowBlock blk;

            blk.w   = mlec->pane;
            blk.box = mlec->paneextent;
            void_WrapOsErrorReporting(wimp_set_extent(&blk));

            mlec__callback(MLEC_CODE_IsWorkAreaChanged, mlec, &blk);
        }
    }
#endif

    { /* May need to scroll the window, to keep the cursor (pcol,row) visible. */
    GDI_BOX curshape;
    union wimp_window_state_open_window_u window_u;
    MLEC_QUERYSCROLL mlec_queryscroll;

    mlec__point_from_colrow(mlec, (P_GDI_POINT) &curshape.x0, mlec->cursor.pcol, mlec->cursor.row);
    curshape.x1 = curshape.x0 + 2;
    curshape.x0 = curshape.x0 - 2;
    curshape.y1 = curshape.y0;
    curshape.y0 = curshape.y1 - mlec->attributes[MLEC_ATTRIBUTE_CHARHEIGHT];

    /* it might be worth expanding the curshape box - a few characters wider and a few pixels taller */

    trace_2(TRACE_MODULE_MLEC, TEXT("cursor is (%d,%d)"), curshape.x0, curshape.y1);

    zero_struct(mlec_queryscroll);
    /*mlec_queryscroll.use = 0;*/

    /*CONSTANTCONDITION*/
    if_pane(mlec)
    {
        window_u.window_state.window_handle = mlec->pane;
        if(NULL == wimp_get_window_state(&window_u.window_state))
        {
            trace_4(TRACE_MODULE_MLEC, TEXT("wimp_get_window_state returns: visible area=(%d,%d, %d,%d)"),window_u.window_state.visible_area.xmin,window_u.window_state.visible_area.ymin,window_u.window_state.visible_area.xmax,window_u.window_state.visible_area.ymax);
            trace_2(TRACE_MODULE_MLEC, TEXT("scroll offset=(%d,%d)"), window_u.window_state.xscroll,window_u.window_state.yscroll);

            mlec_queryscroll.scroll.x  = window_u.window_state.xscroll;
            mlec_queryscroll.scroll.y  = window_u.window_state.yscroll;

            mlec_queryscroll.visible.x = (PIXIT) BBox_width(&window_u.window_state.visible_area);
            mlec_queryscroll.visible.y = (PIXIT) BBox_height(&window_u.window_state.visible_area);

            mlec_queryscroll.use       = 1;
        }
    }
    else
    {
        /* Not attached to a pane, so call client. If the client is maintaining its own view(s) onto this mlec */
        /* it may wish to scroll it/them (probably just the view with the input focus).                        */

        if(mlec__callback(MLEC_CODE_SCROLL, mlec, &curshape) == 0)
            /* SKS default mechanism for scrolling a la main mlec panes */
            mlec__callback(MLEC_CODE_QUERYSCROLL, mlec, &mlec_queryscroll);
    }

    if(mlec_queryscroll.use)
    {
        BOOL done = FALSE;

        while(mlec_queryscroll.scroll.x > curshape.x0)
        {
            mlec_queryscroll.scroll.x -= 4 * mlec->charwidth;
            done = TRUE;
        }

        while((mlec_queryscroll.scroll.x + mlec_queryscroll.visible.x) < curshape.x1)
        {
            mlec_queryscroll.scroll.x += 4 * mlec->charwidth;
            done = TRUE;
        }

        while(mlec_queryscroll.scroll.y < curshape.y1)
        {
            mlec_queryscroll.scroll.y += mlec->attributes[MLEC_ATTRIBUTE_LINESPACE];
            done = TRUE;
        }

        while((mlec_queryscroll.scroll.y - mlec_queryscroll.visible.y) > curshape.y0)
        {
            mlec_queryscroll.scroll.y -= mlec->attributes[MLEC_ATTRIBUTE_LINESPACE];
            done = TRUE;
        }

        if(done)
        {
            /*CONSTANTCONDITION*/
            if_pane(mlec)
            {
#if FALSE
                mlec__event_redraw_loop(mlec);    /* redraw all invalid rectangles BEFORE scrolling window */
                                            /* cos wimp fails to scroll the invalid rectangle list   */
                /*>>>causes a lot of flicker when cursor up/down causes a scroll */
#endif

                window_u.open_window.xscroll = (int) mlec_queryscroll.scroll.x;
                window_u.open_window.yscroll = (int) mlec_queryscroll.scroll.y;

                void_WrapOsErrorReporting(wimp_open_window(&window_u.open_window));
            }
            else
            {
                MLEC_DOSCROLL mlec_doscroll;

                mlec_doscroll.scroll = mlec_queryscroll.scroll;

                status_assert(mlec__callback(MLEC_CODE_DOSCROLL, mlec, &mlec_doscroll));
            }
        }
    }
    } /*block*/

    show_caret(mlec);   /* now show the caret/selection */
}

static void
show_caret(
    /*_Inout_*/ MLEC mlec)
{
    WimpCaret carrot; /* nyeeeer, whats up doc? */

    trace_0(TRACE_MODULE_MLEC, TEXT("show_caret - "));

    build_caretstr(mlec, &carrot);

    if(mlec__callback(MLEC_CODE_PLACECARET, mlec, &carrot) != 0)
        return;

    /*CONSTANTCONDITION*/
    if_pane(mlec)
    {
        WimpCaret current;

        void_WrapOsErrorReporting(wimp_get_caret_position(&current));

        if(current.window_handle == mlec->pane)
        {
            trace_0(TRACE_MODULE_MLEC, TEXT("we own input focus "));

            if((current.xoffset != carrot.xoffset) ||
               (current.yoffset != carrot.yoffset) ||
               (current.height != carrot.height)    /* 'cos this field holds caret (in)visible bit */
              )
            {
                void_WrapOsErrorReporting(wimp_set_caret_position_block(&carrot));
                trace_3(TRACE_MODULE_MLEC, TEXT("place caret (%d,%d,%d)"),carrot.xoffset,carrot.yoffset,carrot.height);
            }
            else
            {
                trace_0(TRACE_MODULE_MLEC, TEXT(" no action (caret already positioned)"));
            }
        }
        else
        {
            trace_0(TRACE_MODULE_MLEC, TEXT(" no action (focus belongs elsewhere)"));
        }
    }
}

/******************************************************************************
*
* Do the donkey work of building the structure needed to claim the caret for
* our pane window, placed at the cursor physical position (cursor(pcol,row)).
* The caret is set as invisible if there is a selection.
*
******************************************************************************/

static void
build_caretstr(
    /*_Inout_*/ MLEC mlec,
    WimpCaret * carrotp)
{
    GDI_POINT caretoffset;

    mlec__point_from_colrow(mlec, &caretoffset, mlec->cursor.pcol, mlec->cursor.row);

    /* put caret at baseline */
    caretoffset.y -= (mlec->attributes[MLEC_ATTRIBUTE_CHARHEIGHT] * 7) / 8;

    carrotp->window_handle = mlec->pane;
    carrotp->icon_handle = (wimp_i) -1;
    carrotp->xoffset = caretoffset.x;
    carrotp->yoffset = caretoffset.y - mlec->attributes[MLEC_ATTRIBUTE_CARETHEIGHTNEG];
    carrotp->height = mlec->attributes[MLEC_ATTRIBUTE_CARETHEIGHTPOS] + mlec->attributes[MLEC_ATTRIBUTE_CARETHEIGHTNEG];
    carrotp->index  = 0;

    if(HOST_FONT_NONE == mlec->host_font)
        carrotp->height |= 0x01000000;  /* VDU 5 style caret if no font */

    if(mlec->selectvalid)
        carrotp->height |= 0x02000000;  /* caret made invisible, if we have a selection */
}

static void
move_cursor(
    /*_Inout_*/ MLEC mlec,
    _In_        int col,
    _In_        int row)
{
    P_U8 buff = mlec->buffptr;

    if(mlec->cursor.row > row)
    {
        /* move cursor to (0,row), or (0,0) if row<0 */

        while((mlec->cursor.row > row) &&
              (mlec->cursor.row > 0)
             )
        {
            /* move rest of line including terminator of previous line */
            while(CR != (buff[--mlec->upper.start] = buff[--mlec->lower.end]))
                /* null statement */;

            --mlec->cursor.row; /* now at end of previous line, column position unknown */
        }

        /* somewhere on required line (or line 0 if row < 0), find column 0 */
        while((mlec->lower.end > mlec->lower.start) &&
              (CR != buff[mlec->lower.end-1])
             )
        {
            buff[--mlec->upper.start] = buff[--mlec->lower.end];
        }

        mlec->cursor.pcol = 0;
    }

    while(
          (mlec->cursor.row  < row)             &&
          (mlec->upper.start < mlec->upper.end)
         )
    {
        if(CR == (buff[mlec->lower.end++] = buff[mlec->upper.start++]))
        {
            /* wrap cursor to beginning of next line */
            mlec->cursor.pcol = 0;
            mlec->cursor.row++;
        }
        else
        {
            /* ordinary character */
            mlec->cursor.pcol++;
        }
    }

    if(mlec->cursor.row == row)
    {
        if(col < 0)
            col = 0;

        while(mlec->cursor.pcol > col)
        {
            buff[--mlec->upper.start] = buff[--mlec->lower.end];
            --mlec->cursor.pcol;
        }

        while(
              (mlec->cursor.pcol < col)             &&
              (mlec->upper.start < mlec->upper.end) &&
              (CR != buff[mlec->upper.start])
             )
        {
            buff[mlec->lower.end++] = buff[mlec->upper.start++];
            mlec->cursor.pcol++;
        }
    }

    mlec->cursor.lcol = mlec->cursor.pcol;
}

/******************************************************************************
*
* Scan left from the cursor position, looking for the start of a word.
* If the cursor is already at the start, find the start of the previous word.
*
******************************************************************************/

static void
word_left(
    /*_Inout_*/ MLEC mlec, mark_position * startp)
{
    P_U8 sptr = &mlec->buffptr[mlec->lower.end];       /* 1 byte past character to left of cursor */
    int scol = mlec->cursor.pcol;

    while((scol > 0) && (CH_SPACE == *(sptr-1)))
    {
        --scol; --sptr;
    }

    while((scol > 0) && (CH_SPACE != *(sptr-1)))
    {
        --scol; --sptr;
    }

    startp->col = scol;
    startp->row = mlec->cursor.row;

    trace_1(TRACE_MODULE_MLEC, TEXT("word start %d"), scol);
}

/******************************************************************************
*
* Scan either side of cursor position, return limits of a para.
*
******************************************************************************/

static void
para_limits(
    /*_Inout_*/ MLEC mlec, mark_position * startp, mark_position * endp)
{
    P_U8 upper_start = &mlec->buffptr[mlec->upper.start];                    /* character to right of cursor            */
    P_U8 upper_end   = &mlec->buffptr[mlec->upper.end];                      /* 1 byte past last character              */
    P_U8 eptr = upper_start;
    int  ecol;

    ecol = mlec->cursor.pcol;

    if((eptr < upper_end) && (CR != *eptr))
    {
        while((eptr < upper_end) && (CR != *eptr))
        {
            ecol++; eptr++;
        }
    }

    while((eptr < upper_end) && (CR != *eptr))
    {
        ecol++; eptr++;
    }

    startp->col = 0;
    startp->row = endp->row = mlec->cursor.row;
    endp->col   = ecol;

    trace_2(TRACE_MODULE_MLEC, TEXT("para limits %d..%d"), 0, ecol);
}

/******************************************************************************
*
* Scan either side of cursor position, return limits of a word.
*
******************************************************************************/

static void
word_limits(
    /*_Inout_*/ MLEC mlec, mark_position * startp, mark_position * endp)
{
  /*P_U8 lower_start = &mlec->buffptr[mlec->lower.start];*/                  /* first character                         */
    P_U8 lower_end   = &mlec->buffptr[mlec->lower.end];                      /* 1 byte past character to left of cursor */
    P_U8 upper_start = &mlec->buffptr[mlec->upper.start];                    /* character to right of cursor            */
    P_U8 upper_end   = &mlec->buffptr[mlec->upper.end];                      /* 1 byte past last character              */

    P_U8 sptr = lower_end;
    P_U8 eptr = upper_start;
    int scol, ecol;

    scol = ecol = mlec->cursor.pcol;

    if((eptr < upper_end) && (CR != *eptr) && (CH_SPACE != *eptr))
    {
        while((eptr < upper_end) && (CR != *eptr) && (CH_SPACE != *eptr))
        {
            ecol++; eptr++;
        }
    }
    else
    {
        while((scol > 0) && (CH_SPACE == *(sptr-1)))
        {
            --scol; --sptr;
        }
    }

    while((scol > 0) && (CH_SPACE != *(sptr-1)))
    {
        --scol; --sptr;
    }

    while((eptr < upper_end) && (CR != *eptr) && (CH_SPACE == *eptr))
    {
        ecol++; eptr++;
    }

    startp->col = scol;
    startp->row = endp->row = mlec->cursor.row;
    endp->col   = ecol;

    trace_2(TRACE_MODULE_MLEC, TEXT("word limits %d..%d"), scol, ecol);
}

static P_U8
render_line(
    /*_Inout_*/ MLEC mlec,
    _In_        int lineCol,
    _In_        int x,
    _In_        int y,
    _InRef_     PC_GDI_BOX p_screen,
    P_U8 ptr,
    P_U8 limit)
{
    int  charedge;
    BOOL scan = TRUE;
    P_U8 showptr;      /* ptr to first visible or partially visible character */
    int  showcnt = 0;  /* number of visible or partially visible characters */

    trace_2(TRACE_MODULE_MLEC, TEXT("render_line x=%d,y=%d "),x,y);
    trace_4(TRACE_MODULE_MLEC, TEXT("screen=(%d,%d, %d,%d) "),p_screen->x0,p_screen->y0,p_screen->x1,p_screen->y1);

    charedge = x + mlec->charwidth;     /* char covers x..charedge (inc..exc) */

    /* find first character not totally hidden */
    while((charedge <= p_screen->x0) && (ptr < limit))              /*>>>  <= ??? */
    {
        if(CR == *ptr++)
        {
            scan = FALSE;       /* ptr now points to first character on next line */
            break;
        }

        x = charedge;
        charedge += mlec->charwidth;
        lineCol++;
    }

    charedge = x;                       /* char covers charedge.. (inc..), N.B. screen->x1 is inclusive!! */
    showptr  = ptr;
    while((charedge </*=*/ p_screen->x1) && (ptr < limit) && scan)
    {
        if(CR == *ptr++)
        {
            scan = FALSE;       /* ptr now points to first character on next line */
            break;
        }

        charedge += mlec->charwidth;
        lineCol++;
        showcnt++;
    }

    if(showcnt > 0)
    {
        if(HOST_FONT_NONE != mlec->host_font)
        {
            STATUS status = STATUS_OK;
            const int base_line_shift = -24;
            _kernel_swi_regs rs;

            rs.r[0] = mlec->host_font;
            rs.r[1] = (int) showptr;
            rs.r[2] = FONT_PAINT_USE_LENGTH /*r7*/ | FONT_PAINT_USE_HANDLE /*r0*/ | FONT_PAINT_OSCOORDS;
            rs.r[3] = x;
            rs.r[4] = y + base_line_shift;
            rs.r[7] = showcnt;

            if(NULL != WrapOsErrorChecking(_kernel_swi(/*Font_Paint*/ 0x40086, &rs, &rs)))
                status = create_error(ERR_FONT_PAINT);
        }
        else
        {
            void_WrapOsErrorChecking(
                bbc_move(x, y-host_modevar_cache_current.dy));

            void_WrapOsErrorChecking(
                os_writeN(showptr, showcnt)); /* shouldn't have CtrlChar in it anyway */
        }
    }

    /* if not already there, advance ptr, to first character on next line */
    while((ptr < limit) && scan)
    {
        if(CR == *ptr++)
            break;

        lineCol++;
    }

    if(mlec->maxcol < lineCol)
    {
        mlec->maxcol = lineCol;
    }

    return(ptr);
}

_Check_return_
static STATUS
checkspace_deletealltext(
    /*_Inout_*/ MLEC mlec,
    _In_        S32 size)
{
    S32 shortfall = size - ((S32) mlec->upper.end - mlec->lower.start);

    /* Check buffer space, note use of lower.start..upper.end instead of lower.end..upper.start */
    /* we want to know if text will fit after current text is flushed */
    if(shortfall > 0)
    {
        trace_2(TRACE_MODULE_MLEC, TEXT("do flex_midextend by %d bytes, at position %d"), shortfall, mlec->upper.start);

        if(!flex_midextend((flex_ptr)&mlec->buffptr, mlec->upper.start, (int) shortfall))
            return(status_nomem());

        mlec->buffsiz     += (int) shortfall;
        mlec->upper.start += (int) shortfall;
        mlec->upper.end   += (int) shortfall;
    }

    return(STATUS_OK);
}

static S32
checkspace_delete_selection(
    /*_Inout_*/ MLEC mlec,
    _In_        S32 size)
{
    S32 shortfall = size - ((S32) mlec->upper.start - mlec->lower.end);

    if(shortfall > 0)
    {
        shortfall = ((shortfall + 3) & -4);     /* round up to a word */

        trace_2(TRACE_MODULE_MLEC, TEXT("do flex_midextend by %d bytes, at position %d"), shortfall, mlec->upper.start);

        if(!flex_midextend((flex_ptr)&mlec->buffptr, mlec->upper.start, (int) shortfall))
            return(status_nomem());

        mlec->buffsiz     += (int) shortfall;
        mlec->upper.start += (int) shortfall;
        mlec->upper.end   += (int) shortfall;
    }

    delete_selection(mlec);
    return(0); /*>>>not quite right, we don't consider the space the delete_selection will free*/
}

#if FALSE
void force_redraw(MLEC mlec)
{
    WimpRedrawWindowBlock redraw_window;

    redraw_window.window_handle =  mlec->pane;
    redraw_window.box.x0 = -0x1FFFFFFF; redraw_window.box.y0 = -0x1FFFFFFF;
    redraw_window.box.x1 =  0x1FFFFFFF; redraw_window.box.y1 =  0x1FFFFFFF;

    if(mlec__callback(MLEC_CODE_UPDATELATER, mlec, &redraw_window) == 0)
        /*CONSTANTCONDITION*/
        if_pane(mlec)
            void_WrapOsErrorReporting(wimp_force_redraw_BBox(mlec->pane, &redraw_window.box));
}
#endif

/******************************************************************************
*
* Mark an area of the pane window as invalid - from cursor position to end of line
*
* NB The window is redrawn later on, when our caller next does a Wimp_Poll.
*    Cursor position is (mlec->cursorpcol,mlec->cursor.row), mlec->cursor.lcol
*    is not used.
*
******************************************************************************/

static void
force_redraw_eoline(
    /*_Inout_*/ MLEC mlec)
{
    WimpRedrawWindowBlock redraw_eoln;

    /* invalidate right of cursor to eol */
    redraw_eoln.window_handle = mlec->pane;

    mlec__point_from_colrow(mlec, (P_GDI_POINT) &redraw_eoln.visible_area.xmin, mlec->cursor.pcol, mlec->cursor.row);
    redraw_eoln.visible_area.xmax = 0x1FFFFFFF;
    redraw_eoln.visible_area.ymax = redraw_eoln.visible_area.ymin;
    redraw_eoln.visible_area.ymin = redraw_eoln.visible_area.ymax - mlec->attributes[MLEC_ATTRIBUTE_LINESPACE]; /*>>>actually charheight??*/

    if(mlec__callback(MLEC_CODE_UPDATELATER, mlec, &redraw_eoln) != 0)
        return;

    /*CONSTANTCONDITION*/
    if_pane(mlec)
        void_WrapOsErrorReporting(wimp_force_redraw_BBox(mlec->pane, &redraw_eoln.visible_area));
}

/******************************************************************************
*
* Mark an area of the pane window as invalid - from cursor position to end of text
*
* NB the window is redrawn later on, when our caller next does a Wimp_Poll.
*    Cursor position is (mlec->cursorpcol,mlec->cursor.row), mlec->cursor.lcol
*    is not used.
*
******************************************************************************/

void
force_redraw_eotext(
    /*_Inout_*/ MLEC mlec)
{
    WimpRedrawWindowBlock redraw_eoln, redraw_eotx;

    /* invalidate right of cursor to eol */
    redraw_eoln.window_handle = mlec->pane;

    mlec__point_from_colrow(mlec, (P_GDI_POINT) &redraw_eoln.visible_area.xmin, mlec->cursor.pcol, mlec->cursor.row);
    redraw_eoln.visible_area.xmax = 0x1FFFFFFF;
    redraw_eoln.visible_area.ymax = redraw_eoln.visible_area.ymin;
    redraw_eoln.visible_area.ymin = redraw_eoln.visible_area.ymax - mlec->attributes[MLEC_ATTRIBUTE_LINESPACE]; /*>>>actually charheight??*/

    /* invalidate rows below cursor */
    redraw_eotx.window_handle = mlec->pane;

    redraw_eotx.visible_area.xmin =  mlec->attributes[MLEC_ATTRIBUTE_MARGIN_LEFT];    /* left   (inc) */
    redraw_eotx.visible_area.xmax =  0x1FFFFFFF;                                      /* right  (exc) */
    redraw_eotx.visible_area.ymax =  redraw_eoln.visible_area.ymin;                     /* top    (exc) */
    redraw_eotx.visible_area.ymin = -0x1FFFFFFF;                                      /* bottom (inc) */

    if(mlec__callback(MLEC_CODE_UPDATELATER, mlec, &redraw_eoln) == 0)
        /*CONSTANTCONDITION*/
        if_pane(mlec)
            void_WrapOsErrorReporting(wimp_force_redraw_BBox(mlec->pane, &redraw_eoln.visible_area));

    if(mlec__callback(MLEC_CODE_UPDATELATER, mlec, &redraw_eotx) == 0)
        /*CONSTANTCONDITION*/
        if_pane(mlec)
            void_WrapOsErrorReporting(wimp_force_redraw_BBox(mlec->pane, &redraw_eotx.visible_area));
}

static void
mlec__drag_start(
    /*_Inout_*/ MLEC mlec)
{
    WimpDragBox dragstr;

    dragstr.drag_type = Wimp_DragBox_DragPoint;

    /*CONSTANTCONDITION*/
    if_pane(mlec)
    {
        WimpGetWindowStateBlock window_state;
        window_state.window_handle = mlec->pane;
        void_WrapOsErrorReporting(wimp_get_window_state(&window_state));

        dragstr.wimp_window = mlec->pane; /* Needed by winx_drag_box(), so it can send Wimp_EUserDrag to us */
#if FALSE
        /* Window Manager ignores inner box on hidden drags */
        dragstr.box.x0 = mx;
        dragstr.box.y0 = my;
        dragstr.box.x1 = mx+30;
        dragstr.box.y1 = my+30;
#endif
        dragstr.parent_box = window_state.visible_area;
    }

    switch(mlec__callback(MLEC_CODE_STARTDRAG, mlec, &dragstr))
    {
    case STATUS_OK:
        return;

    case MLEC_EVENT_DOSTARTDRAG:
        break;

    default:
        /*CONSTANTCONDITION*/
        if_no_pane(mlec)
            return;
        break;
    }

    dragstr.parent_box.xmin -= mlec->charwidth;
    dragstr.parent_box.ymin -= mlec->attributes[MLEC_ATTRIBUTE_LINESPACE];
    dragstr.parent_box.xmax += mlec->charwidth;
    dragstr.parent_box.ymax += mlec->attributes[MLEC_ATTRIBUTE_LINESPACE];

    if(WrapOsErrorReporting(winx_drag_box(&dragstr))) /* NB winx_drag_box() NOT wimp_drag_box() */
        return;

    /*CONSTANTCONDITION*/
    if_pane(mlec)
    {
        trace_0(TRACE_OUT | TRACE_ANY, TEXT("mlec__drag_start - *** null_events_start(DOCNO_NONE)"));
        status_assert(null_events_start(DOCNO_NONE, T5_EVENT_NULL, null_event_mlec_drag, (CLIENT_HANDLE) mlec));
    }

    status_assert(mlec__callback(MLEC_CODE_STARTEDDRAG, mlec, &dragstr));
}

#ifdef MLEC_PANE

static void
mlec__drag_complete(
    /*_Inout_*/ MLEC mlec,
    BBox * dragboxp)
{
    IGNOREPARM(dragboxp);

    trace_0(TRACE_OUT | TRACE_ANY, TEXT("mlec__drag_complete - *** null_events_stop(DOCNO_NONE)"));
    null_events_stop(P_DOCU_NONE, T5_EVENT_NULL, null_event_mlec_drag, mlec);
}

#endif

/******************************************************************************
*
* Process callback from null engine
*
******************************************************************************/

_Check_return_
static STATUS
mlec_drag_null_event(
    P_NULL_EVENT_BLOCK p_data)
{
    const MLEC mlec = (MLEC) p_data->client_handle;
    WimpGetPointerInfoBlock pointer_info;
    GDI_POINT gdi_org;

    void_WrapOsErrorReporting(wimp_get_pointer_info(&pointer_info));

    host_gdi_org_from_screen(&gdi_org, mlec->pane); /* window w.a. ABS origin */

    mlec__drag_core(mlec, &gdi_org, &pointer_info);

    return(STATUS_OK);
}

PROC_EVENT_PROTO(static, null_event_mlec_drag)
{
    IGNOREPARM_DocuRef_(p_docu);

    switch(t5_message)
    {
    case T5_EVENT_NULL:
        return(mlec_drag_null_event((P_NULL_EVENT_BLOCK) p_data));

    default: default_unhandled();
        return(STATUS_OK);
    }
}

/******************************************************************************
*
* Move the cursor to (col,row), without clearing the selection.
*
* If a selection does not exist, one is created, with its
* anchor point at the old cursor position.
*
******************************************************************************/

extern void
mlec__selection_adjust(
    MLEC mlec,
    _In_        int col,
    _In_        int row)
{
    mark_position old;

    trace_2(TRACE_MODULE_MLEC, TEXT("adjust selection to (%d,%d)"),col,row);

    if(!mlec->selectvalid)
    {
        /* currently no selection, so start one at the cursor position */

        mlec->selectvalid      = TRUE;
        mlec->selectanchor.col = mlec->cursor.pcol;
        mlec->selectanchor.row = mlec->cursor.row;
    }

    old.col = mlec->cursor.pcol; old.row = mlec->cursor.row;

    /* move cursor (as close as possible) to (col,row) */
    move_cursor(mlec, col, row);
    col = mlec->cursor.pcol; row = mlec->cursor.row;        /* actual position */

    mlec__update_loop(mlec, old, mlec->cursor);

    if((mlec->selectanchor.col == mlec->cursor.pcol) && (mlec->selectanchor.row == mlec->cursor.row))
        mlec->selectvalid = FALSE;

    scroll_until_cursor_visible(mlec);
}

extern void
mlec__selection_clear(
    MLEC mlec)
{
    clear_selection(mlec);
    show_caret(mlec);   /* do show_caret, not scroll_until_cursor_visible, so the window won't scroll */
}

extern void
mlec__selection_delete(
    MLEC mlec)
{
    delete_selection(mlec);
    scroll_until_cursor_visible(mlec);
}

static void
mlec__select_para(
    /*_Inout_*/ MLEC mlec)
{
    mark_position parastart, paraend;

    para_limits(mlec, &parastart, &paraend);

    if(!mlec->selectvalid)
    {
        /* currently no selection, so set anchor to para start */

        mlec->selectvalid      = TRUE;
        mlec->selectanchor.col = parastart.col;     /* Safe, as para_limits returns valid character positions */
        mlec->selectanchor.row = parastart.row;

        mlec__update_loop(mlec, mlec->selectanchor, mlec->cursor);
    }
    else
    {
        if((mlec->selectanchor.row < mlec->cursor.row) ||
           ((mlec->selectanchor.row == mlec->cursor.row) && (mlec->selectanchor.col < mlec->cursor.pcol))
          )
        { /*EMPTY*/ }
        else
            paraend = parastart;
    }

    mlec__selection_adjust(mlec, paraend.col, paraend.row);
}

static void
mlec__select_word(
    /*_Inout_*/ MLEC mlec)
{
    mark_position wordstart, wordend;

    word_limits(mlec, &wordstart, &wordend);

    if(!mlec->selectvalid)
    {
        /* currently no selection, so set anchor to word start */

        mlec->selectvalid      = TRUE;
        mlec->selectanchor.col = wordstart.col;     /* Safe, as word_limits returns valid character positions */
        mlec->selectanchor.row = wordstart.row;

        mlec__update_loop(mlec, mlec->selectanchor, mlec->cursor);
    }
    else
    {
        if((mlec->selectanchor.row < mlec->cursor.row) ||
           ((mlec->selectanchor.row == mlec->cursor.row) && (mlec->selectanchor.col < mlec->cursor.pcol))
          )
        { /*EMPTY*/ }
        else
            wordend = wordstart;
    }

    mlec__selection_adjust(mlec, wordend.col, wordend.row);
}

static void
clear_selection(
    /*_Inout_*/ MLEC mlec)
{
    if(mlec->selectvalid)
    {
        mlec__update_loop(mlec, mlec->selectanchor, mlec->cursor);
        mlec->selectvalid = FALSE;
    }
}

static void
delete_selection(
    /*_Inout_*/ MLEC mlec)
{
    marked_text range;

    if(range_is_selection(mlec, &range))
    {
        /* Since it is defined that any attempt to move the cursor will either clear the selection, or retain */
        /* it and extend it to the cursor position, the cursor MUST lie within selectstart..selectend         */

        assert(range.lower.start  >= mlec->lower.start);
        assert(range.lower.start  <= mlec->lower.end);
        assert(range.lower.end    == mlec->lower.end);
        assert(range.upper.end    >= mlec->upper.start);
        assert(range.upper.end    <= mlec->upper.end);
        assert(range.upper.start  == mlec->upper.start);

        mlec->cursor.lcol  = mlec->cursor.pcol = range.markstt.col;
        mlec->cursor.row   = range.markstt.row;
        mlec->linecount   -= range.marklines;
        mlec->lower.end    = range.lower.start;
        mlec->upper.start  = range.upper.end;

        if(range.marklines == 0)
            force_redraw_eoline(mlec);
        else
            force_redraw_eotext(mlec);

        mlec->selectvalid = FALSE;

        mlec__issue_update(mlec);
    }
}

static void
range_is_alltext(
    /*_Inout_*/ MLEC mlec, marked_text * range)
{
    range->markstt.col = range->markstt.row = 0;
    range->marklines   = mlec->linecount;
    range->lower       = mlec->lower;
    range->upper       = mlec->upper;
}

static BOOL
range_is_selection(
    /*_Inout_*/ MLEC mlec, marked_text * range)
{
    if(mlec->selectvalid)
    {
        mark_position markstt, markend;

        if((mlec->selectanchor.row < mlec->cursor.row) ||
           ((mlec->selectanchor.row == mlec->cursor.row) && (mlec->selectanchor.col < mlec->cursor.pcol))
          )
        {
            markstt = mlec->selectanchor; markend.col = mlec->cursor.pcol; markend.row = mlec->cursor.row;
        }
        else
        {
            markstt.col = mlec->cursor.pcol; markstt.row = mlec->cursor.row; markend = mlec->selectanchor;
        }

        range->markstt = markstt;
        range->marklines = markend.row - markstt.row;

        /* NB if mark==cursor, find_offset returns mlec->upper.start */

        range->lower.end = mlec->lower.end;
        find_offset(mlec, &markstt, &range->lower.start);
        if(range->lower.start == mlec->upper.start)
            range->lower.start = mlec->lower.end;                      /* so cater for it here */

        range->upper.start = mlec->upper.start;
        find_offset(mlec, &markend, &range->upper.end);
        if(range->upper.end == mlec->lower.end)                        /* should never happen */
            range->lower.end = mlec->upper.start;

    /*>>>wrong - should whinge if no selection */
    }

    return(mlec->selectvalid);
}

/*
*
* NB if find(col,row) == cursor(col,row) this returns mlec->upper.start
*/

static void
find_offset(
    /*_Inout_*/ MLEC mlec,
    mark_position * find,
    int * offsetp)
{
    P_U8 buff = mlec->buffptr;

    if(find->col < 0)           /* The find->(col,row) values passed in should be valid as    */
        find->col = 0;          /* they are either cursor(pcol,row) or selectanchor(col,row), */

    if(find->row < 0)           /* but a little bomb proofing never hurt anyone!?.            */
    {
        find->row = 0;
        find->col = 0;
    }

    if((find->row == mlec->cursor.row) && (find->col < mlec->cursor.pcol))
    {
        /* easy case: cursor on required row, past required col */

        *offsetp = mlec->lower.end - mlec->cursor.pcol + find->col;
    }
    else
    {
        /* hard case: must identify and search appropriate buff_region */
        int col, row, off, lim;

        if(find->row < mlec->cursor.row)
        {
            /* search lower buff_region */

            col = 0;
            row = 0;
            off = mlec->lower.start;
            lim = mlec->lower.end;
        }
        else
        {
            /* search upper buff_region */

            col = mlec->cursor.pcol;
            row = mlec->cursor.row;
            off = mlec->upper.start;
            lim = mlec->upper.end;
        }

        while(
              (row < find->row) &&
              (off < lim)
             )
        {
            if(CR == buff[off++])
            {
                col = 0;
                row++;
            }
            else
            {
                col++;
            }
        }

        /* either found required row, or reached end-of-text */
        while(
              (col < find->col) &&
              (off < lim)       &&
              (CR != buff[off])
             )
        {
            col++;
            off++;
        }

        /* pass back closest col,row found, and its offset in the buffer */
        find->col = col; find->row = row; *offsetp = off;
    }
}

/******************************************************************************
*
* Update the specified region of the pane window
*
* This inverts the specified region
*
******************************************************************************/

static void
mlec__update_loop(
    /*_Inout_*/ MLEC mlec, mark_position mark1, cursor_position mark2)
{
    WimpRedrawWindowBlock redraw_window;
    mark_position markstt, markend;

    /* quit now if null region, as doing the wimp_update_window loop causes the caret to flicker */
    if((mark1.col == mark2.pcol) && (mark1.row == mark2.row))
        return;

    if((mark1.row < mark2.row) ||
       ((mark1.row == mark2.row) && (mark1.col < mark2.pcol))
      )
    {
        markstt = mark1; markend.col = mark2.pcol; markend.row = mark2.row;
    }
    else
    {
        markstt.col = mark2.pcol; markstt.row = mark2.row; markend = mark1;
    }

    trace_4(TRACE_MODULE_MLEC, TEXT("mlec__update_loop (%d,%d, %d,%d)"), markstt.col, markstt.row, markend.col, markend.row);

    redraw_window.window_handle =  mlec->pane;
    redraw_window.visible_area.xmin = -0x1FFFFFFF;
    redraw_window.visible_area.xmax =  0x1FFFFFFF;
    redraw_window.visible_area.ymax = - mlec->attributes[MLEC_ATTRIBUTE_MARGIN_TOP] - mlec->attributes[MLEC_ATTRIBUTE_LINESPACE] *  markstt.row;
    redraw_window.visible_area.ymin = - mlec->attributes[MLEC_ATTRIBUTE_MARGIN_TOP] - mlec->attributes[MLEC_ATTRIBUTE_LINESPACE] * (markend.row + 1);

    stashed_for_update_markstt = markstt;
    stashed_for_update_markend = markend;

#if CHECKING
    threaded_through_update = TRUE;
#endif

    if(mlec__callback(MLEC_CODE_UPDATENOW, mlec, &redraw_window) == 0)
        /*CONSTANTCONDITION*/
        if_pane(mlec)
        {
            int wimp_more;
            GDI_POINT gdi_org;

            if(WrapOsErrorReporting(wimp_update_window(&redraw_window, &wimp_more)))
                wimp_more = 0;

            trace_4(TRACE_MODULE_MLEC, TEXT("wimp_update_window returns: (%d,%d,%d,%d) "),redraw_window.visible_area.xmin,redraw_window.visible_area.ymin,redraw_window.visible_area.xmax,redraw_window.visible_area.ymax);
            trace_2(TRACE_MODULE_MLEC, TEXT("(%d,%d) "),redraw_window.xscroll,redraw_window.yscroll);
            trace_4(TRACE_MODULE_MLEC, TEXT("(%d,%d,%d,%d)"),redraw_window.redraw_area.xmin,redraw_window.redraw_area.ymin,redraw_window.redraw_area.xmax,redraw_window.redraw_area.ymax);

            gdi_org.x = work_area_origin_x_from_visible_area_and_scroll(&redraw_window); /* window w.a. ABS origin */
            gdi_org.y = work_area_origin_y_from_visible_area_and_scroll(&redraw_window);

            while(0 != wimp_more)
            {
                mlec_area_update(mlec, &gdi_org, (PC_GDI_BOX) &redraw_window.redraw_area);

                if(WrapOsErrorReporting(wimp_get_rectangle(&redraw_window, &wimp_more)))
                    wimp_more = 0;
            }
        }

#if CHECKING
    threaded_through_update = FALSE;
#endif
}

_Check_return_
static inline int /*colnum*/
colourtrans_ReturnColourNumber(
    _In_        unsigned int word)
{
    _kernel_swi_regs rs;
    rs.r[0] = word;
    return(_kernel_swi(ColourTrans_ReturnColourNumber, &rs, &rs) ? 0 : rs.r[0]);
}

typedef union RISCOS_PALETTE_U
{
    unsigned int word;

    struct RISCOS_PALETTE_U_BYTES
    {
        char gcol;
        char red;
        char green;
        char blue;
    } bytes;
}
RISCOS_PALETTE_U;

static void
host_set_EOR_for_mlec(void)
{
    RISCOS_PALETTE_U os_rgb_foreground;
    RISCOS_PALETTE_U os_rgb_background;

    os_rgb_foreground.bytes.gcol  = 0;
    os_rgb_foreground.bytes.red   = 0x00;
    os_rgb_foreground.bytes.green = 0x00;
    os_rgb_foreground.bytes.blue  = 0x00;

    os_rgb_background.bytes.gcol  = 0;
    os_rgb_background.bytes.red   = 0xFF;
    os_rgb_background.bytes.green = 0xFF;
    os_rgb_background.bytes.blue  = 0xFF;

    { /* New machines usually demand this mechanism */
    int colnum_foreground = colourtrans_ReturnColourNumber(os_rgb_foreground.word);
    int colnum_background = colourtrans_ReturnColourNumber(os_rgb_background.word);
    _kernel_swi_regs rs;
    rs.r[0] = 3;
    rs.r[1] = colnum_foreground ^ colnum_background;
    void_WrapOsErrorChecking(_kernel_swi(OS_SetColour, &rs, &rs));
    } /*block*/
}

static void
show_selection(
    /*_Inout_*/ MLEC mlec,
    _InRef_     PC_GDI_POINT p_origin,
    _InRef_     PC_GDI_BOX screenBB,
    mark_position markstt,
    mark_position markend)
{
    char * lower_start;
    char * lower_end;
    char * upper_start;
    char * upper_end;
    char * ptr;
    GDI_BOX lineBB;
    GDI_BOX cursor;

    int lineCol, lineRow;

    lower_start = &mlec->buffptr[mlec->lower.start];            /* first character                         */
    lower_end   = &mlec->buffptr[mlec->lower.end];              /* 1 byte past character to left of cursor */
    upper_start = &mlec->buffptr[mlec->upper.start];            /* character to right of cursor            */
    upper_end   = &mlec->buffptr[mlec->upper.end];              /* 1 byte past last character              */

    /* bounding box of characters 0..cursor.pcol (inc,exc) on row cursor.row */

    mlec__point_from_colrow(mlec, (P_GDI_POINT) &cursor.x0, 0, mlec->cursor.row);
    cursor.x1 = cursor.x0 + mlec->charwidth * mlec->cursor.pcol;
    cursor.y1 = cursor.y0;
    cursor.y0 = cursor.y1 - mlec->attributes[MLEC_ATTRIBUTE_CHARHEIGHT];

    cursor.x0 += p_origin->x;
    cursor.y0 += p_origin->y;
    cursor.x1 += p_origin->x;
    cursor.y1 += p_origin->y;

    host_set_EOR_for_mlec();

    {
    /* Consider characters to the right of the cursor and the lines below it */

    lineCol = mlec->cursor.pcol; lineRow = mlec->cursor.row;

    lineBB.x0 = lineBB.x1 = cursor.x1;
    lineBB.y1 = cursor.y1;
    lineBB.y0 = lineBB.y1 - mlec->attributes[MLEC_ATTRIBUTE_CHARHEIGHT];

    ptr    = upper_start;

    /*>>>could put in code to skip lines that are above the current graphics window - Is this comment obsolete??? */

    while((lineBB.y1 > screenBB->y0) && (ptr < upper_end))
    {
        lineBB.x1  = lineBB.x0;

        while(ptr < upper_end)
        {
            if(CR == *ptr++)
            {
                lineBB.x1 += mlec->termwidth;
                break;
            }
            lineBB.x1 += mlec->charwidth;
        }

        if((lineRow >= markstt.row) && (lineRow <= markend.row))
        {
            if(lineRow == markstt.row)
            {
                int stt = (p_origin->x + mlec->attributes[MLEC_ATTRIBUTE_MARGIN_LEFT]) + mlec->charwidth * markstt.col;

                if(lineBB.x0 < stt)
                    lineBB.x0 = stt;
            }

            if(lineRow == markend.row)
            {
                int end = (p_origin->x + mlec->attributes[MLEC_ATTRIBUTE_MARGIN_LEFT]) + mlec->charwidth * markend.col;

                if(lineBB.x1 > end)
                    lineBB.x1 = end;
            }

            if(lineBB.x0 < lineBB.x1)
            {
                /* NB lineBB is (L,B,R,T) edges which are (inc,inc,exc,exc), bbc_RectangleFill takes (inc,inc,inc,inc) */

                void_WrapOsErrorChecking(
                    bbc_move(lineBB.x0, lineBB.y0));

                void_WrapOsErrorChecking(
                    os_plot(0x60 /*bbc_RectangleFill*/ | 5 /*bbc_DrawAbsFore*/, lineBB.x1 - 1, lineBB.y1 - 1));
            }
        }

        lineCol = 0; lineRow++;

        lineBB.x0 = cursor.x0;
        lineBB.y1 = lineBB.y1 - mlec->attributes[MLEC_ATTRIBUTE_LINESPACE];
        lineBB.y0 = lineBB.y1 - mlec->attributes[MLEC_ATTRIBUTE_CHARHEIGHT];
    }

    /* remaining lines are below the graphics window */
    } /*block*/

    {
    /* Consider characters to the left of the cursor and the lines above it */

    lineCol = 0; lineRow = mlec->cursor.row;

    lineBB.x0 = lineBB.x1 = cursor.x0;      /* NB x0 & x1 the same ie no CR width */
    lineBB.y1 = cursor.y1;
    lineBB.y0 = lineBB.y1 - mlec->attributes[MLEC_ATTRIBUTE_CHARHEIGHT];

    ptr = lower_end;

    /*>>>could put in code to skip lines that are below the current graphics window */

    while((lineBB.y0 < screenBB->y1) && (ptr > lower_start))
    {
      /*lineBB.x1  = lineBB.x0;*/

        while(ptr > lower_start)
        {
            if(CR == *--ptr)
                break;

            lineBB.x1 += mlec->charwidth;
        }

        if((lineRow >= markstt.row) && (lineRow <= markend.row))
        {
            if(lineRow == markstt.row)
            {
                int stt = (p_origin->x + mlec->attributes[MLEC_ATTRIBUTE_MARGIN_LEFT]) + mlec->charwidth * markstt.col;

                if(lineBB.x0 < stt)
                    lineBB.x0 = stt;
            }

            if(lineRow == markend.row)
            {
                int end = (p_origin->x + mlec->attributes[MLEC_ATTRIBUTE_MARGIN_LEFT]) + mlec->charwidth * markend.col;

                if(lineBB.x1 > end)
                    lineBB.x1 = end;
            }

            if(lineBB.x0 < lineBB.x1)
            {
                /* NB lineBB is (L,B,R,T) edges which are (inc,inc,exc,exc), bbc_RectangleFill takes (inc,inc,inc,inc) */

                void_WrapOsErrorChecking(
                    bbc_move(lineBB.x0, lineBB.y0));

                void_WrapOsErrorChecking(
                    os_plot(0x60 /*bbc_RectangleFill*/ | 5 /*bbc_DrawAbsFore*/, lineBB.x1 - 1, lineBB.y1 - 1));
            }
        }

        lineCol = 0; --lineRow;

        lineBB.x0 = cursor.x0;
        lineBB.x1 = lineBB.x0 + mlec->termwidth; /* cos CR has width */
        lineBB.y1 = lineBB.y1 + mlec->attributes[MLEC_ATTRIBUTE_LINESPACE];
        lineBB.y0 = lineBB.y1 - mlec->attributes[MLEC_ATTRIBUTE_CHARHEIGHT];
    }

    /* remaining lines are above the graphics window */
    IGNOREPARM(lineCol);
    } /*block*/
}

/******************************************************************************
*
* File load routines: LoadAtCursor
*
* File save routines: SaveAll & SaveSelection
*
* Selected area routines: Cut, Copy & Delete
*
******************************************************************************/

#if 0

_Check_return_
static int
mlec__atcursor_load(
    /*_Inout_*/ MLEC mlec,
    _In_z_      PCTSTR filename)
{
    trace_1(TRACE_MODULE_MLEC, TEXT("mlec__atcursor_load('%s')"), filename);

    return(text_in(mlec, filename, file_read_open, file_read_getblock, file_read_close));
}

_Check_return_
static int
mlec__alltext_save(
    /*_Inout_*/ MLEC mlec,
    _In_z_      PCTSTR filename,
    _InVal_     T5_FILETYPE t5_filetype,
    P_U8 lineterm)
{
    marked_text range;
    XFER_HANDLE handle;

    range_is_alltext(mlec, &range);

    status_return(file_write_open(&handle, filename, t5_filetype));

    return(text_out(mlec, &handle, range, lineterm, file_write_size, file_write_putblock, file_write_close));
}

_Check_return_
static int
mlec__selection_save(
    /*_Inout_*/ MLEC mlec,
    _In_z_      PCTSTR filename,
    _InVal_     T5_FILETYPE t5_filetype,
    P_U8 lineterm)
{
    marked_text range;
    XFER_HANDLE handle;

    if(range_is_selection(mlec, &range))
    {
        status_return(file_write_open(&handle, filename, t5_filetype));

        return(text_out(mlec, &handle, range, lineterm, file_write_size, file_write_putblock, file_write_close));
    }

    return(create_error(MLEC_ERR_NOSELECTION));
}

#endif

#if 0

_Check_return_
static STATUS
file_read_open(
    _In_z_      PCTSTR filename,
    /*out*/ P_T5_FILETYPE p_t5_filetype,
    /*out*/ P_S32 filesizep,
    /*out*/ P_XFER_HANDLE xferhandlep)
{
    STATUS status;
    filelength_t filelength;

    status_return(t5_file_open(filename, file_open_read, &xferhandlep->f, TRUE));

    if(status_fail(status = file_length(xferhandlep->f, &filelength)))
        t5_file_close(&xferhandlep->f);
    else
    {
        *p_t5_filetype = file_get_type(xferhandlep->f);
        *filesizep = (S32) filelength.u.words.lo;
    }

    return(status);        /*>>>what about filetype checks???*/
}

_Check_return_
static STATUS
file_read_getblock(
    P_XFER_HANDLE xferhandlep,
    P_U8 dataptr,
    _In_        int datasize)
{
    U32 bytesread;
    status_return(file_read_bytes(dataptr, datasize, &bytesread, xferhandlep->f));
    return((STATUS) bytesread);
}

_Check_return_
static STATUS
file_read_close(
    P_XFER_HANDLE xferhandlep)
{
    return(t5_file_close(&(xferhandlep->f)));
}

_Check_return_
static STATUS
file_write_open(
    /*out*/ P_XFER_HANDLE xferhandlep,
    _In_z_      PCTSTR filename,
    _InVal_     T5_FILETYPE t5_filetype)
{
    status_return(t5_file_open(filename, file_open_write, &xferhandlep->f, TRUE));

    return(file_set_risc_os_filetype(xferhandlep->f, t5_filetype));
}

_Check_return_
static STATUS
file_write_size(
    P_XFER_HANDLE xferhandlep,
    _In_        int xfersize)
{
    IGNOREPARM(xferhandlep);
    IGNOREPARM(xfersize);

    return(0);    /*>>>might be better to set the file extent to xfersize - ask Tutu */
}

_Check_return_
static STATUS
file_write_putblock(
    P_XFER_HANDLE xferhandlep,
    P_U8 dataptr,
    _In_        int datasize)
{
    return(file_write_bytes(dataptr, datasize, xferhandlep->f));
}

_Check_return_
static STATUS
file_write_close(
    P_XFER_HANDLE xferhandlep)
{
    return(t5_file_close(&(xferhandlep->f)));
}

#endif

#if defined(MLEC_CLIP)

_Check_return_
static STATUS
mlec__atcursor_paste(
    /*_Inout_*/ MLEC mlec)
{
    trace_0(TRACE_MODULE_MLEC, TEXT("mlec__atcursor_paste"));

    return(text_in(mlec, "", paste_read_open, paste_read_getblock, paste_read_close));
}

/******************************************************************************
*
* Copy selection to the paste buffer
*
* Doesn't clear the selection
*
******************************************************************************/

_Check_return_
static STATUS
mlec__selection_copy(
    /*_Inout_*/ MLEC mlec)
{
    STATUS err;
    marked_text range;
    XFER_HANDLE handle;

    reject_if_paste_buffer(mlec);

    if(range_is_selection(mlec, &range))
    {
        if((err = paste_write_open(&handle)) >= 0)
            err = text_out(mlec, &handle, range, NULL, paste_write_size, paste_write_putblock, paste_write_close);
        return(err);
        /*>>>should this clear the selection???*//*No, windows doesn't*/
    }

    return(create_error(MLEC_ERR_NOSELECTION));
}

/******************************************************************************
*
* Cut the selection into the paste buffer
*
* Same as Copy-then-Delete
*
******************************************************************************/

_Check_return_
static STATUS
mlec__selection_cut(
    /*_Inout_*/ MLEC mlec)
{
    STATUS err;

    if((err = mlec__selection_copy(mlec)) >= 0)
        mlec__selection_delete(mlec);           /* delete selection only if copy succeeds */

    return(err);
}

_Check_return_
static STATUS
paste_read_open(
    _In_z_      PCTSTR filename,
    /*out*/ P_T5_FILETYPE p_t5_filetype,
    /*out*/ P_S32 filesizep,
    /*out*/ P_XFER_HANDLE xferhandlep)
{
    marked_text range;

    IGNOREPARM(filename);

    xferhandlep->p = (MLEC) paste;

    if(!paste)
        return(create_error(MLEC_ERR_NOPASTEBUFFER));

    range_is_alltext(paste, &range);

    *p_t5_filetype = FILETYPE_TEXT;

    *filesizep = ((S32) range.lower.end - range.lower.start) +
                 ((S32) range.upper.end - range.upper.start);

    return(0);
}

/******************************************************************************
*
* NB The data size field is ignored, all the data is written to dataptr
*
******************************************************************************/

_Check_return_
static STATUS
paste_read_getblock(
    P_XFER_HANDLE xferhandlep,
    P_U8 dataptr,
    _In_        int datasize)
{
    MLEC mlec = (MLEC) xferhandlep->p;

    IGNOREPARM(datasize);

    if(mlec)
    {
        marked_text  range;
        char       * buff = mlec->buffptr;
        int          i;

        range_is_alltext(mlec, &range);

        for(i = range.lower.start; i < range.lower.end; i++)
            *dataptr++ = buff[i];

        for(i = range.upper.start; i < range.upper.end; i++)
            *dataptr++ = buff[i];

        return(0);
    }

    return(create_error(MLEC_ERR_BUFFERWENT_AWOL));
}

_Check_return_
static STATUS
paste_read_close(
    P_XFER_HANDLE xferhandlep)
{
    xferhandlep->p = NULL;
    return(0);
}

_Check_return_
static STATUS
paste_write_open(
    /*out*/ P_XFER_HANDLE xferhandlep)
{
    if(!paste)
        status_return(mlec_create(&paste)); /* probably STATUS_NOMEM */

    xferhandlep->p = paste;

    if(!paste)
        return(create_error(MLEC_ERR_NOPASTEBUFFER));

    return(mlec_SetText(paste, ""));
}

_Check_return_
static STATUS
paste_write_size(
    P_XFER_HANDLE xferhandlep,
    _In_        int xfersize)
{
    /* Since paste_write_open does mlec_SetText(paste, "") to clear all text (and selection!), */
    /* we can use either checkspace_deletealltext or checkspace_delete_selection               */

    return(checkspace_delete_selection((MLEC) xferhandlep->p, xfersize + 1));  /* plus 1 to allow room for terminator */
}

_Check_return_
static STATUS
paste_write_putblock(
    P_XFER_HANDLE xferhandlep,
    P_U8 dataptr,
    _In_        int datasize)
{
    MLEC mlec = (MLEC) xferhandlep->p;

    if(mlec)
    {
        P_U8 ptr = &mlec->buffptr[mlec->lower.end];
        int   i;

        for(i = 0; i < datasize; i++)
            ptr[i] = *dataptr++;
        ptr[i] = CH_NULL;

        return(mlec__insert_text(mlec, ptr));   /* space test WILL be successful */
                                                /*>>> could flex space move???   */
    }

    return(create_error(MLEC_ERR_BUFFERWENT_AWOL));
}

_Check_return_
static STATUS
paste_write_close(
    P_XFER_HANDLE xferhandlep)
{
    xferhandlep->p = NULL;
    return(0);
}

_Check_return_
static STATUS
text_in(
    /*_Inout_*/ MLEC mlec,
    _In_z_      PCTSTR filename,
    /*_Check_return_*/ STATUS (* openp) (
        _In_z_      PCTSTR filename,
        P_T5_FILETYPE p_t5_filetype,
        P_S32 filesizep,
        P_XFER_HANDLE xferhandlep),
    /*_Check_return_*/ STATUS (* readp) (
        P_XFER_HANDLE xferhandlep,
        P_U8 dataptr,
        int datasize),
    /*_Check_return_*/ STATUS (* closep) (
        P_XFER_HANDLE xferhandlep))
{
    STATUS err, errc;
    XFER_HANDLE handle;
    T5_FILETYPE t5_filetype;
    S32 filesize;

    if(status_ok(err = (* openp)(filename, &t5_filetype, &filesize, &handle)))
    {
        if(status_ok(err = checkspace_delete_selection(mlec, filesize + 1 /* plus 1 to allow room for terminator */)))
        {
            if(status_ok(err = (* readp)(&handle, &mlec->buffptr[mlec->lower.end], (int) filesize)))
            {
                P_U8 ptr = &mlec->buffptr[mlec->lower.end];
                ptr[filesize] = CH_NULL;
                mlec__insert_text(mlec, ptr); /* space test WILL be successful */
            }

            mlec__issue_update(mlec);
        }

        errc = (* closep)(&handle);

        if(status_ok(err))
            err = errc;
    }

    scroll_until_cursor_visible(mlec);  /* In case an error stops mlec__insert_text doing one for us */

    return(err);
}

#endif /* MLEC_CLIP */

_Check_return_
static STATUS
text_out(
    /*_Inout_*/ MLEC mlec,
    P_XFER_HANDLE xferhandlep,
    marked_text range,
    P_U8 lineterm,
    /*_Check_return_*/ STATUS (* sizep) (
        P_XFER_HANDLE xferhandlep,
        int xfersize),
    /*_Check_return_*/ STATUS (* writep) (
        P_XFER_HANDLE xferhandlep,
        P_U8 dataptr,
        int datasize),
    /*_Check_return_*/ STATUS (* closep) (
        P_XFER_HANDLE xferhandlep))
{
    STATUS err, errc;
    int xfersize = ((range.lower.end - range.lower.start) + (range.upper.end - range.upper.start));
                     /* carefull, size valid for (NULL == lineterm) or (sizeof32(lineterm) == 1) */
                     /* since only paste_write_open expects this to be valid we should be OK   */
                     /*>>>think about this, can we do calculation properly*/
    if(status_ok(err = (* sizep)(xferhandlep, xfersize)))
    {
        P_U8 buff = mlec->buffptr;

        if(lineterm)
        {
            /* output the text line-by-line, replacing our terminators by the caller-supplied sequence */
            int size;
            int linestart;
            int lineend;

            linestart = range.lower.start;

            while(linestart < range.lower.end)
            {
                BOOL term;

                lineend = linestart;

                for(term = FALSE; lineend < range.lower.end; lineend++)
                    if(buff[lineend] == CR)
                    {
                        term = TRUE;
                        break;
                    }

                if(((size = lineend - linestart) > 0) && status_ok(err))
                    err = (* writep)(xferhandlep, &buff[linestart], size);

                /* if we stopped at eol, output the supplied terminator char(s) */
                if(term && status_ok(err))
                {
                    err = (* writep)(xferhandlep, lineterm, strlen(lineterm));
                    lineend++;              /* not forgetting to skip our term */
                }

                linestart = lineend;
            }

            linestart = range.upper.start;

            while(linestart < range.upper.end)
            {
                BOOL term;

                lineend = linestart;

                for(term = FALSE; lineend < range.upper.end; lineend++)
                    if(buff[lineend] == CR)
                    {
                        term = TRUE;
                        break;
                    }

                if(((size = lineend - linestart) > 0) && status_ok(err))
                    err = (* writep)(xferhandlep, &buff[linestart], size);

                /* if we stopped at eol, output the supplied terminator char(s) */
                if(term && status_ok(err))
                {
                    err = (* writep)(xferhandlep, lineterm, strlen(lineterm));
                    lineend++;              /* not forgetting to skip our term */
                }

                linestart = lineend;
            }
        }
        else
        {
            /* not bothered about line terminator characters, so blast the data out in 2 (max) lumps */

            int size;

            if(((size = range.lower.end - range.lower.start) > 0) && status_ok(err))
                err = (* writep)(xferhandlep, &buff[range.lower.start], size);

            if(((size = range.upper.end - range.upper.start) > 0) && status_ok(err))
                err = (* writep)(xferhandlep, &buff[range.upper.start], size);
        }

        errc = (* closep) (xferhandlep);

        if(status_ok(err))
            err = errc;
    }

    return(err);
}

_Check_return_
static STATUS
mlec__callback(
    mlec_event_reason_code message,
    /*_Inout_*/ MLEC mlec,
    /*_Inout_*/ P_ANY p_data)
{
    if(!mlec->callbackproc)
        return(STATUS_OK);

    return((* mlec->callbackproc) (message, mlec->callbackhand, p_data));
}

static void
mlec__issue_update(
    /*_Inout_*/ MLEC mlec)
{
    mlec__callback(MLEC_CODE_UPDATE, mlec, NULL);
}

#endif /* RISCOS */

/* end of mlec.c */
