/* t5_glue.c - included by each product.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#if !defined(__prodinfo_h)
/* ignore */
#else

/*#include "common/gflags.h"*/

/*#ifndef    __skel_flags_h*/
/*#include "ob_skel/flags.h"*/
/*#endif*/

OBJECT_PROTO(extern, object_cells);
OBJECT_PROTO(extern, object_cells_edit);
OBJECT_PROTO(extern, object_chart);
OBJECT_PROTO(extern, object_dialog);
OBJECT_PROTO(extern, object_draw);
OBJECT_PROTO(extern, object_draw_io);
OBJECT_PROTO(extern, object_file);
OBJECT_PROTO(extern, object_hefo);
OBJECT_PROTO(extern, object_implied_style);
OBJECT_PROTO(extern, object_mailshot);
OBJECT_PROTO(extern, object_mlec);
OBJECT_PROTO(extern, object_note);
OBJECT_PROTO(extern, object_rec);
OBJECT_PROTO(extern, object_recb);
OBJECT_PROTO(extern, object_recn);
OBJECT_PROTO(extern, object_ruler);
OBJECT_PROTO(extern, object_skel);
OBJECT_PROTO(extern, object_skel_split);
OBJECT_PROTO(extern, object_sle);
OBJECT_PROTO(extern, object_spelb);
OBJECT_PROTO(extern, object_spell);
OBJECT_PROTO(extern, object_ss);
OBJECT_PROTO(extern, object_story);
OBJECT_PROTO(extern, object_text);
OBJECT_PROTO(extern, object_toolbar);

OBJECT_PROTO(extern, object_fl_ascii);
OBJECT_PROTO(extern, object_fl_csv);
OBJECT_PROTO(extern, object_fl_fwp);
OBJECT_PROTO(extern, object_fl_lotus123);
OBJECT_PROTO(extern, object_fl_pdtx);
OBJECT_PROTO(extern, object_fl_pdss);
OBJECT_PROTO(extern, object_fl_rtf);
OBJECT_PROTO(extern, object_fl_xls);

OBJECT_PROTO(extern, object_fs_ascii);
OBJECT_PROTO(extern, object_fs_csv);
OBJECT_PROTO(extern, object_fs_lotus123);
OBJECT_PROTO(extern, object_fs_ole);
OBJECT_PROTO(extern, object_fs_rtf);
OBJECT_PROTO(extern, object_fs_xls);

extern void
t5_glue_objects(void)
{
    /* NB. object ordering here is irrelevant to startup order */

#if          defined(BIND_OB_CELLS)
    object_install(OBJECT_ID_CELLS, object_cells);
#endif

#if          defined(BIND_OB_CELLS_EDIT)
    object_install(OBJECT_ID_CELLS_EDIT, object_cells_edit);
#endif

#if          defined(BIND_OB_CHART)
    object_install(OBJECT_ID_CHART, object_chart);
#endif

#if          defined(BIND_OB_DIALOG)
    object_install(OBJECT_ID_DIALOG, object_dialog);
#endif

#if          defined(BIND_OB_DRAW)
    object_install(OBJECT_ID_DRAW, object_draw);
#endif

#if          defined(BIND_OB_DRAW_IO)
    object_install(OBJECT_ID_DRAW_IO, object_draw_io);
#endif

#if          defined(BIND_OB_FILE)
    object_install(OBJECT_ID_FILE, object_file);
#endif

#if          defined(BIND_OB_HEFO)
    object_install(OBJECT_ID_HEFO, object_hefo);
#endif

#if          defined(BIND_OB_IMPLIED_STYLE)
    object_install(OBJECT_ID_IMPLIED_STYLE, object_implied_style);
#endif

#if          defined(BIND_OB_MAILSHOT)
    object_install(OBJECT_ID_MAILSHOT, object_mailshot);
#endif

#if          defined(BIND_OB_MLEC)
    object_install(OBJECT_ID_MLEC, object_mlec);
#endif

#if          defined(BIND_OB_NOTE)
    object_install(OBJECT_ID_NOTE, object_note);
#endif

#if          defined(BIND_OB_REC)
    object_install(OBJECT_ID_REC, object_rec);
#endif

#if          defined(BIND_OB_RECB)
    object_install(OBJECT_ID_RECB, object_recb);
#endif

#if          defined(BIND_OB_RECN)
    object_install(OBJECT_ID_RECN, object_recn);
#endif

#if          defined(BIND_OB_RULER)
    object_install(OBJECT_ID_RULER, object_ruler);
#endif

#if          defined(BIND_OB_SKEL_SPLIT)
    object_install(OBJECT_ID_SKEL_SPLIT, object_skel_split);
#endif

#if          defined(BIND_OB_SLE)
    object_install(OBJECT_ID_SLE, object_sle);
#endif

#if          defined(BIND_OB_SPELB)
    object_install(OBJECT_ID_SPELB, object_spelb);
#endif

#if          defined(BIND_OB_SPELL)
    object_install(OBJECT_ID_SPELL, object_spell);
#endif

#if          defined(BIND_OB_STORY)
    object_install(OBJECT_ID_STORY, object_story);
#endif

#if          defined(BIND_OB_SS)
    object_install(OBJECT_ID_SS, object_ss);
#endif

#if          defined(BIND_OB_TEXT)
    object_install(OBJECT_ID_TEXT, object_text);
#endif

#if          defined(BIND_OB_TOOLBAR)
    object_install(OBJECT_ID_TOOLBAR, object_toolbar);
#endif

#if          defined(BIND_FL_ASCII)
    object_install(OBJECT_ID_FL_ASCII, object_fl_ascii);
#endif

#if          defined(BIND_FL_CSV)
    object_install(OBJECT_ID_FL_CSV, object_fl_csv);
#endif

#if          defined(BIND_FL_FWP)
    object_install(OBJECT_ID_FL_FWP, object_fl_fwp);
#endif

#if          defined(BIND_FL_LOTUS123)
    object_install(OBJECT_ID_FL_LOTUS123, object_fl_lotus123);
#endif

#if          defined(BIND_FL_PDSS)
    object_install(OBJECT_ID_FL_PDSS, object_fl_pdss);
#endif

#if          defined(BIND_FL_PDTX)
    object_install(OBJECT_ID_FL_PDTX, object_fl_pdtx);
#endif

#if          defined(BIND_FL_RTF)
    object_install(OBJECT_ID_FL_RTF, object_fl_rtf);
#endif

#if          defined(BIND_FL_XLS)
    object_install(OBJECT_ID_FL_XLS, object_fl_xls);
#endif

#if          defined(BIND_FS_ASCII)
    object_install(OBJECT_ID_FS_ASCII, object_fs_ascii);
#endif

#if          defined(BIND_FS_CSV)
    object_install(OBJECT_ID_FS_CSV, object_fs_csv);
#endif

#if          defined(BIND_FS_LOTUS123)
    object_install(OBJECT_ID_FS_LOTUS123, object_fs_lotus123);
#endif

#if          defined(BIND_FS_OLE)
    object_install(OBJECT_ID_FS_OLE, object_fs_ole);
#endif

#if          defined(BIND_FS_RTF)
    object_install(OBJECT_ID_FS_RTF, object_fs_rtf);
#endif

#if          defined(BIND_FS_XLS)
    object_install(OBJECT_ID_FS_XLS, object_fs_xls);
#endif

    object_install(OBJECT_ID_SKEL, object_skel);
}

_Check_return_
extern P_PROC_OBJECT
t5_glued_object(
    _InVal_     OBJECT_ID object_id)
{
    UNREFERENCED_PARAMETER_InVal_(object_id); /* there are times when this is really never used */

    myassert1x(IS_OBJECT_ID_VALID(object_id), TEXT("t5_glued_object(INVALID OBJECT_ID ") S32_TFMT TEXT(")"), object_id);

    switch(object_id)
    {
#if !defined(OBJECTS_LOADABLE) /* i.e. fully-bound */

#if  defined(LOADED_OB_CELLS)
        case OBJECT_ID_CELLS: return(object_cells);
#endif

#if  defined(LOADED_OB_CELLS_EDIT)
        case OBJECT_ID_CELLS_EDIT: return(object_cells_edit);
#endif

#if  defined(LOADED_OB_CHART)
        case OBJECT_ID_CHART: return(object_chart);
#endif

#if  defined(LOADED_OB_DIALOG)
        case OBJECT_ID_DIALOG: return(object_dialog);
#endif

#if  defined(LOADED_OB_DRAW)
        case OBJECT_ID_DRAW: return(object_draw);
#endif

#if  defined(LOADED_OB_DRAW_IO)
        case OBJECT_ID_DRAW_IO: return(object_draw_io);
#endif

#if  defined(LOADED_OB_FILE)
        case OBJECT_ID_FILE: return(object_file);
#endif

#if  defined(LOADED_OB_HEFO)
        case OBJECT_ID_HEFO: return(object_hefo);
#endif

#if  defined(LOADED_OB_IMPLIED_STYLE)
        case OBJECT_ID_IMPLIED_STYLE: return(object_implied_style);
#endif

#if  defined(LOADED_OB_MAILSHOT)
        case OBJECT_ID_MAILSHOT: return(object_mailshot);
#endif

#if  defined(LOADED_OB_MLEC)
        case OBJECT_ID_MLEC: return(object_mlec);
#endif

#if  defined(LOADED_OB_NOTE)
        case OBJECT_ID_NOTE: return(object_note);
#endif

#if  defined(LOADED_OB_REC)
        case OBJECT_ID_REC: return(object_rec);
#endif

#if  defined(LOADED_OB_RECB)
        case OBJECT_ID_RECB: return(object_recb);
#endif

#if  defined(LOADED_OB_RECN)
        case OBJECT_ID_RECN: return(object_recn);
#endif

#if  defined(LOADED_OB_RULER)
        case OBJECT_ID_RULER: return(object_ruler);
#endif

#if  defined(LOADED_OB_SKEL_SPLIT)
        case OBJECT_ID_SKEL_SPLIT: return(object_skel_split);
#endif

#if  defined(LOADED_OB_SLE)
        case OBJECT_ID_SLE: return(object_sle);
#endif

#if  defined(LOADED_OB_SPELB)
        case OBJECT_ID_SPELB: return(object_spelb);
#endif

#if  defined(LOADED_OB_SPELL)
        case OBJECT_ID_SPELL: return(object_spell);
#endif

#if  defined(LOADED_OB_STORY)
        case OBJECT_ID_STORY: return(object_story);
#endif

#if  defined(LOADED_OB_SS)
        case OBJECT_ID_SS: return(object_ss);
#endif

#if  defined(LOADED_OB_TEXT)
        case OBJECT_ID_TEXT: return(object_text);
#endif

#if  defined(LOADED_OB_TOOLBAR)
        case OBJECT_ID_TOOLBAR: return(object_toolbar);
#endif

#if  defined(LOADED_FL_ASCII)
        case OBJECT_ID_FL_ASCII: return(object_fl_ascii);
#endif

#if  defined(LOADED_FL_CSV)
        case OBJECT_ID_FL_CSV: return(object_fl_csv);
#endif

#if  defined(LOADED_FL_FWP)
        case OBJECT_ID_FL_FWP: return(object_fl_fwp);
#endif

#if  defined(LOADED_FL_LOTUS123)
        case OBJECT_ID_FL_LOTUS123: return(object_fl_lotus123);
#endif

#if  defined(LOADED_FL_PDSS)
        case OBJECT_ID_FL_PDSS: return(object_fl_pdss);
#endif

#if  defined(LOADED_FL_PDTX)
        case OBJECT_ID_FL_PDTX: return(object_fl_pdtx);
#endif

#if  defined(LOADED_FL_RTF)
        case OBJECT_ID_FL_RTF: return(object_fl_rtf);
#endif

#if  defined(LOADED_FL_XLS)
        case OBJECT_ID_FL_XLS: return(object_fl_xls);
#endif

#if  defined(LOADED_FS_ASCII)
        case OBJECT_ID_FS_ASCII: return(object_fs_ascii);
#endif

#if  defined(LOADED_FS_CSV)
        case OBJECT_ID_FS_CSV: return(object_fs_csv);
#endif

#if  defined(LOADED_FS_LOTUS123)
        case OBJECT_ID_FS_LOTUS123: return(object_fs_lotus123);
#endif

#if  defined(LOADED_FS_OLE)
        case OBJECT_ID_FS_OLE: return(object_fs_ole);
#endif

#if  defined(LOADED_FS_RTF)
        case OBJECT_ID_FS_RTF: return(object_fs_rtf);
#endif

#if  defined(LOADED_FS_XLS)
        case OBJECT_ID_FS_XLS: return(object_fs_xls);
#endif

#endif /* defined(OBJECTS_LOADABLE) */

        default:
            break;
    }

    return(NULL);
}

/*
is object available for use (either currently loaded or loadable)
*/

_Check_return_
extern BOOL
object_available(
    _InVal_     OBJECT_ID object_id)
{
    myassert1x(IS_OBJECT_ID_VALID(object_id), TEXT("object_available(INVALID OBJECT_ID ") S32_TFMT TEXT(")"), object_id);

    if(object_present(object_id))
        return(TRUE);

    switch(object_id)
    {
#if  defined(LOADED_OB_CELLS) || defined(BIND_OB_CELLS)
        case OBJECT_ID_CELLS:
#endif

#if  defined(LOADED_OB_CELLS_EDIT) || defined(BIND_OB_CELLS_EDIT)
        case OBJECT_ID_CELLS_EDIT:
#endif

#if  defined(LOADED_OB_CHART) || defined(BIND_OB_CHART)
        case OBJECT_ID_CHART:
#endif

#if  defined(LOADED_OB_DIALOG) || defined(BIND_OB_DIALOG)
        case OBJECT_ID_DIALOG:
#endif

#if  defined(LOADED_OB_DRAW) || defined(BIND_OB_DRAW)
        case OBJECT_ID_DRAW:
#endif

#if  defined(LOADED_OB_DRAW_IO) || defined(BIND_OB_DRAW_IO)
        case OBJECT_ID_DRAW_IO:
#endif

#if defined(LOADED_OB_FILE) || defined(BIND_OB_FILE)
        case OBJECT_ID_FILE:
#endif

#if  defined(LOADED_OB_HEFO) || defined(BIND_OB_HEFO)
        case OBJECT_ID_HEFO:
#endif

#if  defined(LOADED_OB_IMPLIED_STYLE) || defined(BIND_OB_IMPLIED_STYLE)
        case OBJECT_ID_IMPLIED_STYLE:
#endif

#if  defined(LOADED_OB_MAILSHOT) || defined(BIND_OB_MAILSHOT)
        case OBJECT_ID_MAILSHOT:
#endif

#if  defined(LOADED_OB_MLEC) || defined(BIND_OB_MLEC)
        case OBJECT_ID_MLEC:
#endif

#if  defined(LOADED_OB_NOTE) || defined(BIND_OB_NOTE)
        case OBJECT_ID_NOTE:
#endif

#if  defined(LOADED_OB_REC) || defined(BIND_OB_REC)
        case OBJECT_ID_REC:
#endif

#if  defined(LOADED_OB_RECB) || defined(BIND_OB_RECB)
        case OBJECT_ID_RECB:
#endif

#if  defined(LOADED_OB_RECN) || defined(BIND_OB_RECN)
        case OBJECT_ID_RECN:
#endif

#if  defined(LOADED_OB_RULER) || defined(BIND_OB_RULER)
        case OBJECT_ID_RULER:
#endif

#if  defined(LOADED_OB_SKEL_SPLIT) || defined(BIND_OB_SKEL_SPLIT)
        case OBJECT_ID_SKEL_SPLIT:
#endif

#if  defined(LOADED_OB_SLE) || defined(BIND_OB_SLE)
        case OBJECT_ID_SLE:
#endif

#if  defined(LOADED_OB_SPELB) || defined(BIND_OB_SPELB)
        case OBJECT_ID_SPELB:
#endif

#if  defined(LOADED_OB_SPELL) || defined(BIND_OB_SPELL)
        case OBJECT_ID_SPELL:
#endif

#if  defined(LOADED_OB_STORY) || defined(BIND_OB_STORY)
        case OBJECT_ID_STORY:
#endif

#if  defined(LOADED_OB_SS) || defined(BIND_OB_SS)
        case OBJECT_ID_SS:
#endif

#if  defined(LOADED_OB_TEXT) || defined(BIND_OB_TEXT)
        case OBJECT_ID_TEXT:
#endif

#if  defined(LOADED_OB_TOOLBAR) || defined(BIND_OB_TOOLBAR)
        case OBJECT_ID_TOOLBAR:
#endif

#if  defined(LOADED_FL_ASCII) || defined(BIND_FL_ASCII)
        case OBJECT_ID_FL_ASCII:
#endif

#if  defined(LOADED_FL_CSV) || defined(BIND_FL_CSV)
        case OBJECT_ID_FL_CSV:
#endif

#if  defined(LOADED_FL_FWP) || defined(BIND_FL_FWP)
        case OBJECT_ID_FL_FWP:
#endif

#if  defined(LOADED_FL_PDSS) || defined(BIND_FL_PDSS)
        case OBJECT_ID_FL_PDSS:
#endif

#if  defined(LOADED_FL_PDTX) || defined(BIND_FL_PDTX)
        case OBJECT_ID_FL_PDTX:
#endif

#if  defined(LOADED_FL_RTF) || defined(BIND_FL_RTF)
        case OBJECT_ID_FL_RTF:
#endif

#if  defined(LOADED_FL_XLS) || defined(BIND_FL_XLS)
        case OBJECT_ID_FL_XLS:
#endif

#if  defined(LOADED_FS_ASCII) || defined(BIND_FS_ASCII)
        case OBJECT_ID_FS_ASCII:
#endif

#if  defined(LOADED_FS_CSV) || defined(BIND_FS_CSV)
        case OBJECT_ID_FS_CSV:
#endif

#if  defined(LOADED_FS_OLE) || defined(BIND_FS_OLE)
        case OBJECT_ID_FS_OLE:
#endif

#if  defined(LOADED_FS_RTF) || defined(BIND_FS_RTF)
        case OBJECT_ID_FS_RTF:
#endif

#if  defined(LOADED_FS_XLS) || defined(BIND_FS_XLS)
        case OBJECT_ID_FS_XLS:
#endif

#if  defined(LOADED_FF_LOTUS123) || defined(BIND_FF_LOTUS123)
        case OBJECT_ID_FF_LOTUS123:
#endif
            return(TRUE);

        default:
            break;
    }

    return(FALSE);
}

#endif /* __prodinfo_h */

/* end of t5_glue.c */
