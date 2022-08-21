/* mlec2.c */

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

#include "ob_skel/xp_skelr.h"

#include "cmodules/debug.h"

#include "cmodules/file.h"

#include "cmodules/mlec.h"

#include "cmodules/mleci.h"

#if defined(SUPPORT_PANEMENU) && (SUPPORT_PANEMENU > 0)

/* Pane window menu structure */

#define MENU_ROOT_SAVE      1
#define                         MENU_SAVE_FILE        1
#define                         MENU_SAVE_SELECTION   2
#define MENU_ROOT_SELECTION 2
#define                         MENU_SELECTION_CLEAR  1
#define                         MENU_SELECTION_COPY   2
#define                         MENU_SELECTION_CUT    3
#define                         MENU_SELECTION_DELETE 4
#define MENU_ROOT_PASTE     3

static menu mlec_menu_root      = NULL;
static menu mlec_menu_save      = NULL;
static menu mlec_menu_selection = NULL;

/******************************************************************************
*
* mlec__event_menu_maker  - constructs a static menu skeleton
* mlec__event_menu_filler - ticks/shades skeleton when 'menu' is pressed
* mlec__event_menu_proc   - processes menu actions
*
******************************************************************************/

static void
mlec__event_menu_maker(
    const char * menu_title)
{
    if(!mlec_menu_root)
    {
        mlec_menu_root      = menu_new(menu_title ,
                                       resource_lookup_string(MLEC_MSG_MENUBDY));

        mlec_menu_save      = menu_new(resource_lookup_string(MLEC_MSG_MENUHDR_SAVE),
                                       resource_lookup_string(MLEC_MSG_MENUBDY_SAVE));
        mlec_menu_selection = menu_new(resource_lookup_string(MLEC_MSG_MENUHDR_SELECTION),
                                       resource_lookup_string(MLEC_MSG_MENUBDY_SELECTION));

        menu_submenu(mlec_menu_root, MENU_ROOT_SAVE     , mlec_menu_save);
        menu_submenu(mlec_menu_root, MENU_ROOT_SELECTION, mlec_menu_selection);
    }
}

menu
mlec__event_menu_filler(
    void *handle)
{
    MLEC mlec = (MLEC) handle;

    if(mlec_menu_root)
    {
        BOOL fade = !mlec->selectvalid;

        menu_setflags(mlec_menu_root, MENU_ROOT_PASTE, FALSE, (NULL == paste));    /*>>>Paste NYA, so fade it*/

        if(mlec_menu_save)
        {
            menu_setflags(mlec_menu_save, MENU_SAVE_SELECTION, FALSE, fade);
        }

        if(mlec_menu_selection)
        {
            /* Copy,Cut & Delete only allowed for a valid selection */

            menu_setflags(mlec_menu_selection, MENU_SELECTION_CLEAR , FALSE, fade);
            menu_setflags(mlec_menu_selection, MENU_SELECTION_COPY  , FALSE, fade);
            menu_setflags(mlec_menu_selection, MENU_SELECTION_CUT   , FALSE, fade);
            menu_setflags(mlec_menu_selection, MENU_SELECTION_DELETE, FALSE, fade);
        }
    }

    return(mlec_menu_root);
}

BOOL
mlec__event_menu_proc(
    void *handle,
    P_U8 hit,
    _InVal_     BOOL submenurequest)
{
    MLEC mlec = (MLEC) handle;
    int err = 0;

    UNREFERENCED_PARAMETER(submenurequest);

    switch(*hit++)
    {
    case MENU_ROOT_SAVE:
        switch(*hit++)
        {
        case MENU_SAVE_FILE:
            saveas(FILETYPE_TEXT, "TextFile", 1024,
                   saveas_saveall,               /* file save */
                   0,                            /* ram xfer */
                   0,                            /* print */
                   (void*) mlec);
            break;

        case MENU_SAVE_SELECTION:
            saveas(FILETYPE_TEXT, "Selection", 1024,
                   saveas_saveselection,
                   0,
                   0,
                   (void*) mlec);
            break;
        }
        break;

    case MENU_ROOT_SELECTION:
        switch(*hit++)
        {
        case MENU_SELECTION_CLEAR:
            mlec__selection_clear(mlec);
            break;

        case MENU_SELECTION_COPY:
            err = mlec__selection_copy(mlec);
            break;

        case MENU_SELECTION_CUT:
            err = mlec__selection_cut(mlec);
            break;

        case MENU_SELECTION_DELETE:
            mlec__selection_delete(mlec);
            break;
        }
        break;

    case MENU_ROOT_PASTE:
        err = mlec__atcursor_paste(mlec);
        break;
    }

    /* Any error is the result of direct user interaction with the mlec, */
    /* so report the error here, cos there is no caller to return it to. */
    if(err < 0)
        mlec__report_error(mlec, err);

    return(TRUE);
}

/******************************************************************************
*
* saveas_saveall       - SaveAll       } to the filing system
* saveas_saveselection - SaveSelection }
*
* Returns
*   TRUE if save was successful
*   FALSE if not, having reported the error
*
******************************************************************************/

BOOL
saveas_saveall(
    const char * filename,
    void * handle)
{
    /* Reorder params and bend types as needed */

    MLEC mlec = (MLEC) handle;
    int err;

    if((err = mlec__alltext_save(mlec, (P_U8) filename, filetype_TEXT, lineterm_LF)) < 0)
    {
        report_error(mlec, err);        /* Report the error here, cos there is no caller to return it to. */
        return(FALSE);
    }

    return(TRUE);
}

BOOL
saveas_saveselection(
    const char * filename,
    void * handle)
{
    /* Reorder params and bend types as needed */

    MLEC mlec = (MLEC) handle;
    int err;

    if((err = mlec__selection_save(mlec, (P_U8) filename, filetype_TEXT, lineterm_LF)) < 0)
    {
        report_error(mlec, err);        /* Report the error here, cos there is no caller to return it to. */
        return(FALSE);
    }

    return(TRUE);
}

_Check_return_
extern STATUS
mlec_attach_panemenu(
    MLEC mlec,
    const char * menu_title)
{
    if(menu_title && (mlec != paste))
    {
        /* Construct the menu skeleton, and attach the filler & process routines to the window */
        mlec__event_menu_maker(menu_title);

        (void) event_register_window_menumaker(pane_win_handle,
                                    mlec__event_menu_filler,    /* Fade/tick the menu entries       */
                                    mlec__event_menu_proc,      /* Decode and action the menu entry */
                                    (void *) mlec);
        mlec->panemenu = TRUE;
    }

    return(STATUS_OK);
}

#else /* SUPPORT_PANEMENU */

extern void
__mlec2(void);

extern void
__mlec2(void)
{
}

#endif /* SUPPORT_PANEMENU */

#endif /* RISCOS */

/* end of mlec2.c */
