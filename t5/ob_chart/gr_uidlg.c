/* gr_uidlg.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Glue to stick UI into gr_chart */

/* SKS April 1993 */

/*#include "flags.h"*/

extern void
gr_uidlg_satisfy_compiler(void);

extern void
gr_uidlg_satisfy_compiler(void)
{
}

#if 0

/******************************************************************************
*
* fill style dialog processing
*
******************************************************************************/

typedef struct _FILLSTYLE_ENTRY
    {
    GR_CACHE_HANDLE cah;
    U8 leafname[BUF_MAX_LEAFNAME];
}
FILLSTYLE_ENTRY;
typedef struct _FILLSTYLE_ENTRY * P_FILLSTYLE_ENTRY;

static LIST_BLOCK
fillstyle_list;

static GR_FILL_PATTERN_HANDLE
gr_chart_edit_riscos_fillstyle_pattern_query(const LIST_ITEMNO new_fillstyle_key)
{
    P_FILLSTYLE_ENTRY entryp;

    if((entryp = list_gotoitemcontents(&fillstyle_list, new_fillstyle_key)) != NULL)
        return((GR_FILL_PATTERN_HANDLE) entryp->cah);

    return(GR_FILL_PATTERN_NONE);
}

static void
gr_chart_edit_encode_selected_fillstyle(P_GR_CHART_EDIT cep, H_DIALOG h_dialog, P_GR_FILLSTYLE /*const*/ fillstyle, const LIST_ITEMNO fillstyle_key)
{
    /* encode icons from current state (ok, ought to
     * use tristates but RISC OS doesn't have the concept)
    */
    P_GR_CHART cp = gr_chart_cp_from_ch(cep->ch);
    P_FILLSTYLE_ENTRY /*const*/ entryp;
    U8 leafname[BUF_MAX_LEAFNAME];
    STATUS enable;

    assert(cp);

    *leafname = '\0';

    if((entryp = list_gotoitemcontents(&fillstyle_list, fillstyle_key)) != NULL)
        (void) strcpy(leafname, entryp->leafname);

    {
    UI_TEXT ui_text;
    ui_text.type = UI_TEXT_TYPE_P_U8_TEMP;
    ui_text.text.p_u8 = leafname;
    ui_dlg_set_edit(h_dialog, GR_CHART_EDIT_TEM_FILLSTYLE_ICON_DRAW_NAME, &ui_text);
    }

    gr_ui_setcheck(h_dialog, GR_CHART_EDIT_TEM_FILLSTYLE_ICON_SOLID, (fillstyle->pattern == GR_FILL_PATTERN_NONE) || !fillstyle->bits.notsolid);
    gr_ui_setcheck(h_dialog, GR_CHART_EDIT_TEM_FILLSTYLE_ICON_PATTERN, (fillstyle->pattern != GR_FILL_PATTERN_NONE) && fillstyle->bits.pattern);

    gr_ui_setcheck(h_dialog, GR_CHART_EDIT_TEM_FILLSTYLE_ICON_ISOTROPIC, fillstyle->bits.isotropic);
    gr_ui_setcheck(h_dialog, GR_CHART_EDIT_TEM_FILLSTYLE_ICON_RECOLOUR,  !fillstyle->bits.norecolour);

    /* fade out picture icons once picture fill deselected */
    enable = gr_ui_getcheck(h_dialog, GR_CHART_EDIT_TEM_FILLSTYLE_ICON_PATTERN);

    gr_ui_ctl_enable(h_dialog, GR_CHART_EDIT_TEM_FILLSTYLE_ICON_DRAW_NAME, enable);
    gr_ui_ctl_enable(h_dialog, GR_CHART_EDIT_TEM_FILLSTYLE_ICON_DRAW_INC, enable);
    gr_ui_ctl_enable(h_dialog, GR_CHART_EDIT_TEM_FILLSTYLE_ICON_DRAW_DEC, enable);
    gr_ui_ctl_enable(h_dialog, GR_CHART_EDIT_TEM_FILLSTYLE_ICON_ISOTROPIC, enable);
    gr_ui_ctl_enable(h_dialog, GR_CHART_EDIT_TEM_FILLSTYLE_ICON_RECOLOUR, enable);
}

static void
gr_chart_edit_build_fillstyle_list(P_GR_CHART_EDIT cep, P_GR_CHART_OBJID id /*const*/)
{
    P_GR_CHART cp = gr_chart_cp_from_ch(cep->ch);
    GR_FILLSTYLE fillstyle;
    P_U8 /*const*/ pict_dir;
    GR_CACHE_HANDLE current_cah;
    LIST_ITEMNO current_key;
    U8 path[BUF_MAX_PATHSTRING];
    U8 fullname[BUF_MAX_FILENAME];
    P_FILE_OBJENUM enumstrp;
    P_FILE_OBJINFO infostrp;
    P_FILLSTYLE_ENTRY entryp;
    STATUS status;

    assert(cp);
    gr_chart_objid_fillstyle_query(cp, id, &fillstyle);

    current_cah = (GR_CACHE_HANDLE) fillstyle.pattern;
    current_key = -1;

    /* throw away any existing fillstyle_list, then (re)build it */
    collect_delete(&fillstyle_list);

    /* enumerate Markers or Pictures into a list and gr_cache_ensure_entry each of them */
    switch(id->name)
    {
        case GR_CHART_OBJNAME_SERIES:
        case GR_CHART_OBJNAME_POINT:
        {
            GR_SERIES_IX seridx = gr_series_ix_from_external(cp, id->no);
            P_GR_AXES /*const*/ axesp  = gr_axesp_from_seridx(cp, seridx);

            if( (axesp->charttype == GR_CHARTTYPE_LINE) ||
                (axesp->charttype == GR_CHARTTYPE_SCAT) )
            {
                P_GR_SERIES serp = getserp(cp, seridx);
                GR_CHARTTYPE charttype = serp->charttype;

                if( (charttype == GR_CHARTTYPE_NONE) ||
                    (charttype == axesp->charttype)  )
                {
                    pict_dir = "Markers";
                    break;
                }
            }
        }

        /* else deliberate drop thru ... */

        default:
            pict_dir = "Pictures";
            break;
    }

    /* enumerate all **files** in subdirectory pict_dir found in the
     * directories relative to the chart or listed in the applications path variable
    */

    file_combined_path(path, file_is_rooted(cp->core.currentfilename) ? cp->core.currentfilename : NULL);

    trace_1(TRACE_MODULE_GR_CHART, "[path='%s']\n", path);

    for(infostrp = file_find_first_subdir(&enumstrp, path, FILE_WILD_MULTIPLE_STR, pict_dir);
        infostrp;
        infostrp = file_find_next(&enumstrp))
    {
        if(file_objinfo_type(infostrp) == FILE_OBJECT_FILE)
        {
            U8 leafname[BUF_MAX_LEAFNAME];
            GR_CACHE_HANDLE new_cah;
            LIST_ITEMNO new_key;

            file_objinfo_name(infostrp, leafname);

            file_find_query_dirname(&enumstrp, fullname);

            (void) strcat(strcat(fullname, FILE_DIR_SEP_STR), leafname);

            trace_1(TRACE_MODULE_GR_CHART, "[fullname is '%s']\n", fullname);

            status = gr_cache_entry_ensure(&new_cah, fullname);

            /* create a list entry (add to end), returns a key */
            new_key = -1;

            if((entryp = collect_add_entry(&fillstyle_list, sizeof(*entryp), new_key)) != NULL)
            {
                /* note key if we added current picture to list. doesn't matter
                 * about matching GR_CACHE_HANDLE_NONE as that's trapped below
                */
                if(current_cah == new_cah)
                    current_key = new_key;

                entryp->cah = new_cah;
                strnkpy(entryp->leafname, leafname, sizeof(entryp->leafname)-1);
            }
            else
            {
                status = status_nomem();
                break;
            }
        }
    }

    /* Tutu said to ignore errors from enumeration */

    file_find_close(&enumstrp);     /* Not really needed, done by file_find_next in this case */

    /* if the current fillstyle pattern is not already there then insert at front of list */
    if((current_cah != GR_CACHE_HANDLE_NONE) && (current_key == -1))
    {
        gr_cache_name_query(&current_cah, fullname, sizeof(fullname)-1);

        current_key = 0;

        if((entryp = collect_insert_entry(&fillstyle_list, sizeof(*entryp), current_key)) != NULL)
        {
            entryp->cah = current_cah;
            (void) strcpy(entryp->leafname, file_leafname(fullname));
        }
    }
}

static void
fillstyle_redraw_core(GR_CACHE_HANDLE cah, const wimp_redrawstr * r, wimp_icon * picture)
{
    wimp_redrawstr passed_r;

    passed_r  = *r;

#if 0
    passed_r.box.x0 = orgx + picture->box.x0;
    passed_r.box.y0 = orgy + picture->box.y0;
    passed_r.box.x1 = orgx + picture->box.x1;
    passed_r.box.y1 = orgy + picture->box.y1;
    passed_r.scx    = 0;
    passed_r.scy    = 0;

    /* set our own graphics window */
    wimpt_safe(bbc_gwindow(passed_r.g.x0,              passed_r.g.y0,
                           passed_r.g.x1 - wimpt_dx(), passed_r.g.y1 - wimpt_dy()));

    passed_r.box.y1 = passed_r.box.y0;  /* D.Elworthy, your're a twat! */

    bbc_gcol(0, 128);   /* clear background to white */
    bbc_clg();

    diag = gr_cache_search(cah);

    if(!diag)
        /* draw file not currently loaded, so force it in */
        diag = gr_cache_loaded_ensure(cah);

    if(diag)
    {
        draw_box bound;
        intl     diag_wid, diag_hei;
        intl     icon_wid, icon_hei;
        double   scaleX, scaleY, scale;

        draw_queryBox(diag, &bound, TRUE);      /* diagram bounding box (in screen coords) */

        diag_wid = bound.x1 - bound.x0;
        diag_hei = bound.y1 - bound.y0;

        diag_wid = max(diag_wid, 16);
        diag_hei = max(diag_hei, 16);

        icon_wid = picture->box.x1 - picture->box.x0;
        icon_hei = picture->box.y1 - picture->box.y0;

        scaleX = (double) icon_wid / (double) diag_wid;
        scaleY = (double) icon_hei / (double) diag_hei;

        scale = min(scaleX, scaleY);

        if(!draw_render_diag(diag, (draw_redrawstr *) &passed_r, scale, &error))
        {
            /* There was an error: indicate that it happened and continue */
            if(myassert(0))
                myasserted(error.type == DrawOSError
                                      ? "fillstyle diagram render failed %.0s" S32_FMT " at &" U32_XFMT
                                      : "fillstyle diagram render failed %s%.0d at &" U32_XFMT,
                           error.err.os.errmess,
                           error.err.draw.code,
                           error.err.draw.location);
        }
    }
#endif
}

static BOOL
fillstyle_event_handler(dbox d, P_ANY event, P_ANY handle)
{
    wimp_eventstr * e = event;
    LIST_ITEMNO * keyp = handle;

    UNREFERENCED_PARAMETER(d);

    switch(e->e)
    {
    case wimp_EREDRAW:
        {
        fillstyle_entrycp entryp;
        GR_CACHE_HANDLE   cah;
        wimp_redrawstr    redraw_window_block;
        wimp_icon         picture;
        int wimp_more;

        /* get the window relative bbox of the picture icon */
        redraw_window_block.w = e->data.redraw.w;
        wimp_get_icon_info(redraw_window_block.w, GR_CHART_EDIT_TEM_FILLSTYLE_ICON_DRAW_PICT, &picture);

        cah = 0;

        if((entryp = list_gotoitemcontents(&fillstyle_list, *keyp)) != NULL)
            cah = entryp->cah;

        /* only redrawing required is of the 'picture' (a draw file identified by cah), */
        /* which must be scaled to fit within the limits of its icon                    */
        if(NULL != WrapOsErrorReporting(wimp_redraw_window(&redraw_window_block, &more)))
            more = FALSE;

        while(more)
        {
            fillstyle_redraw_core(cah, &redraw_window_block, &picture);

            if(NULL != WrapOsErrorReporting(wimp_get_rectangle(&redraw_window_block, &more)))
                wimp_more = 0;
        }
        }
        break;

    default:
        return(FALSE);
    }

    return(TRUE);
}

extern STATUS
gr_chart_edit_selection_fillstyle_edit(P_GR_CHART_EDIT cep)
{
    intl         ok, persist, reflect_modify;
    P_GR_CHART cp;
    GR_FILLSTYLE fillstyle;
    LIST_ITEMNO fillstyle_key;
    fillstyle_entrycp entryp;
    STATUS disallow_piccie = 0;
    STATUS status = STATUS_OK;

    assert(cep);

    d = dbox_new_give_err(GR_CHART_EDIT_TEM_FILLSTYLE, status_nomem());
    if(!d)
        return(0);

    dbox_show(d);

    w = dbox_syshandle(d);

    {
    U8 title[BUF_MAX_GR_CHART_OBJID_REPR + 32];
    intl appendage = GR_CHART_MSG_EDIT_APPEND_FILL;

    switch(cep->selection.id.name)
    {
        case GR_CHART_OBJNAME_PLOTAREA:
            if(cep->selection.id.no)
                disallow_piccie = 1;
            else
                /* 'area area' sounds silly */
                appendage = 0;
            break;

        case GR_CHART_OBJNAME_CHART:
        case GR_CHART_OBJNAME_LEGEND:
            appendage = GR_CHART_MSG_EDIT_APPEND_AREA;
            break;

        default:
            break;
    }

    gr_chart_object_name_from_id(title, &cep->selection.id);

    if(appendage)
        (void) strcat(title, string_lookup(appendage));

    (void) strcat(title, string_lookup(GR_CHART_MSG_EDIT_APPEND_STYLE));

    win_settitle(w, title);
    }

    gr_ui_enable(h_dialog, GR_CHART_EDIT_TEM_FILLSTYLE_ICON_PATTERN, disallow_piccie);

    if(!disallow_piccie)
    {
        gr_chart_edit_build_fillstyle_list(cep, &cep->selection.id);

        dbox_raw_eventhandler(d, fillstyle_event_handler, &fillstyle_key);
    }

    reflect_modify = 0;

    do  {
        /* load chart structure up to local structure */
        cp = gr_chart_cp_from_ch(cep->ch);
        assert(cp);
        gr_chart_objid_fillstyle_query(cp, &cep->selection.id, &fillstyle);

    {
        LIST_ITEMNO key;

        fillstyle_key = 0;

        for(entryp = collect_first(&fillstyle_list, &key); entryp; entryp = collect_next( &fillstyle_list, &key))
            if((GR_CACHE_HANDLE) fillstyle.pattern == entryp->cah)
            {
                fillstyle_key = key;
                break;
            }
    }

        for(;;)
        {
            gr_chart_edit_encode_selected_fillstyle(cep, w, &fillstyle, fillstyle_key);

            res = 1;

            if((f = dbox_fillin(d)) == dbox_CLOSE)
                break;

            switch(f)
            {
                case GR_CHART_EDIT_TEM_FILLSTYLE_ICON_SOLID:
                    fillstyle.bits.notsolid = !win_getonoff(w, f);
                    break;

                case GR_CHART_EDIT_TEM_FILLSTYLE_ICON_PATTERN:
                    fillstyle.bits.pattern = win_getonoff(w, f);

                    fillstyle.pattern = fillstyle.bits.pattern
                                                ? gr_chart_edit_riscos_fillstyle_pattern_query(fillstyle_key)
                                                : GR_FILL_PATTERN_NONE;
                    break;

                case GR_CHART_EDIT_TEM_FILLSTYLE_ICON_ISOTROPIC:
                    fillstyle.bits.isotropic = win_getonoff(w, f);
                    break;

                case GR_CHART_EDIT_TEM_FILLSTYLE_ICON_RECOLOUR:
                    fillstyle.bits.norecolour  = !win_getonoff(w, f);
                    break;

                default:
                    if(win_adjustbumphit(&f, GR_CHART_EDIT_TEM_FILLSTYLE_ICON_DRAW_NAME)) /* inverts dirn. for click */
                    {                                                                /* with adjust button      */
                        LIST_ITEMNO last_key;
                        U8 leafname[MAX_LEAFNAME];

                        last_key = list_numitem(fillstyle_list.lbr);
                        if(!last_key)
                            break;

                        if((entryp = (f == GR_CHART_EDIT_TEM_FILLSTYLE_ICON_DRAW_INC
                                         ?  nlist_next
                                         :  nlist_prev)
                                    (&fillstyle_list, &fillstyle_key)) == NULL)
                        {
                            /* either: 'next' whilst showing last item or 'prev' whilst showing first item, */
                            /*         so wrap to other end of list                                         */
                            fillstyle_key = (f == GR_CHART_EDIT_TEM_FILLSTYLE_ICON_DRAW_INC ? 0 : last_key - 1);

                            if((entryp = list_gotoitemcontents(&fillstyle_list, fillstyle_key)) == NULL)
                            {
                                fillstyle_key = 0;
                                assert0();  /* SKS is more paranoid */
                                break;      /* not found - will never happen! (or so says RCM) */
                            }
                        }

                        /* show the pictures leafname, trigger a (later) redraw of picture icon */
                        (void) strcpy(leafname, entryp->leafname);

                        win_setfield(       w, GR_CHART_EDIT_TEM_FILLSTYLE_ICON_DRAW_NAME, leafname);
                        wimp_set_icon_state(w, GR_CHART_EDIT_TEM_FILLSTYLE_ICON_DRAW_PICT, 0, 0);
                    }
                    break;
            }

            /* check what the current pattern is set to */
            fillstyle.bits.pattern = win_getonoff(w, GR_CHART_EDIT_TEM_FILLSTYLE_ICON_PATTERN);

            fillstyle.pattern = fillstyle.bits.pattern
                                        ? gr_chart_edit_riscos_fillstyle_pattern_query(fillstyle_key)
                                        : GR_FILL_PATTERN_NONE;

            if(f == dbox_OK)
            {
                fillstyle.fg.manual = 1;

                res = gr_chart_objid_fillstyle_set(cp, &cep->selection.id, &fillstyle);

                if(res < 0)
                    gr_chart_edit_winge(res);

                gr_chart_modify_and_rebuild(&cep->ch);

                if(res < 0)
                    f = dbox_CLOSE;

                break;
            }
        }

        ok = (f == dbox_OK);

        persist = ok ? dbox_persist() : FALSE;
    }
    while(persist);

    dbox_dispose(&d);

    return(res);
}

/******************************************************************************
*
* use Acorn-supplied FontSelector code to select a text style for objects
*
******************************************************************************/

typedef struct _GR_CHART_EDIT_RISCOS_FONTSELECT_CALLBACK_INFO
    {
    GR_CHART_EDIT_HANDLE ceh;
    GR_CHART_OBJID      id;

    GR_TEXTSTYLE        style;
}
GR_CHART_EDIT_RISCOS_FONTSELECT_CALLBACK_INFO;
typedef         GR_CHART_EDIT_RISCOS_FONTSELECT_CALLBACK_INFO * P_GR_CHART_EDIT_RISCOS_FONTSELECT_CALLBACK_INFO;

extern void
gr_chart_edit_fontselect_kill(P_GR_CHART_EDIT cep)
{
    if(cep)
        cep->riscos.processing_fontselector = 0;

    fontselect_closewindows();
}

static void
gr_chart_edit_fontselect_try_me(
    P_U8 /*const*/ font_name, const double * width, const double * height, P_GR_CHART_EDIT_RISCOS_FONTSELECT_CALLBACK_INFO i)
{
    P_GR_CHART_EDIT cep;
    intl            modify = 0;
    P_GR_TEXTSTYLE   cur_style = &i->style;
    GR_TEXTSTYLE    new_style = *cur_style;

    cep = gr_chart_edit_cep_from_ceh(i->ceh);
    /* note that chart editor may have gone away during FontSelector processing! */
    if(!cep)
        return;

    memclr(new_style.tstrFontName, sizeof(new_style.tstrFontName));
    strncat(new_style.tstrFontName, font_name, sizeof(new_style.tstrFontName)-1);

    new_style.width  = (GR_PIXIT) (*width  * GR_PIXITS_PER_POINT);
    new_style.height = (GR_PIXIT) (*height * GR_PIXITS_PER_POINT);

    modify = memcmp(&new_style, cur_style, sizeof(new_style));

    if(modify)
    {
        P_GR_CHART cp;

        cp = gr_chart_cp_from_ch(cep->ch);
        assert(cp);

        new_style.fg.manual = 1;
        *cur_style = new_style;
        gr_chart_edit_winge(gr_chart_objid_textstyle_set(cp, &i->id, cur_style));

        gr_chart_modify_and_rebuild(&cep->ch);
    }
}

static BOOL
gr_chart_edit_fontselect_unknown_fn(
    const char *   font_name, const double * width, const double * height, const wimp_eventstr * e, P_ANY try_handle, BOOL try_anyway)
{
    P_GR_CHART_EDIT_RISCOS_FONTSELECT_CALLBACK_INFO i = try_handle;
    BOOL close_windows = FALSE;

    if(try_anyway)
        gr_chart_edit_fontselect_try_me(font_name, width, height, i);
    else
        switch(e->e)
        {
            default:
                break;
        }

    return(close_windows);
}

extern STATUS
gr_chart_edit_selection_textstyle_edit(P_GR_CHART_EDIT cep)
{
    GR_CHART_EDIT_RISCOS_FONTSELECT_CALLBACK_INFO i;
    F64 width, height;
    U8 title[BUF_MAX_GR_CHART_OBJID_REPR + 32];

    assert(cep);

    /* winge about rubbish font manager every time */
    (void) fontxtra_ensure_usable_version();

    /* can have only one font selector per program */
    if(!fontselect_prepare_process())
    {
        /* that **nasty** piece of code relies on the first level monitoring of
         * the state of the font selector returning without further ado to its caller
         * and several milliseconds later the Window Manager will issue yet another
         * submenu open request. or so we hope. if the punter is miles too quick,
         * it will merely close the old font selector and not open a new one, so
         * he'll have to go and move over the right arrow again!
        */
        return(0);
    }

    event_clear_current_menu();

    i.id  = cep->selection.id;
    i.ceh = cep->ceh;

    {
    S32 appendage = 0;

    switch(i.id.name)
    {
        case GR_CHART_OBJNAME_PLOTAREA:
            i.id = gr_chart_objid_chart;

            /* deliberate drop thru ... */

        case GR_CHART_OBJNAME_CHART:
            appendage = GR_CHART_MSG_EDIT_APPEND_BASE;
            break;

        default:
            break;
    }

    gr_chart_object_name_from_id(title, &i.id);

    if(appendage)
        (void) strcat(title, string_lookup(appendage));

    switch(i.id.name)
    {
        case GR_CHART_OBJNAME_TEXT:
            appendage = GR_CHART_MSG_EDIT_APPEND_STYLE;
            break;

        default:
            appendage = GR_CHART_MSG_EDIT_APPEND_TEXTSTYLE;
            break;
    }

    if(appendage)
        (void) strcat(title, string_lookup(appendage));
    }

    {
    P_GR_CHART cp = gr_chart_cp_from_ch(cep->ch);
    assert(cp);
    gr_chart_objid_textstyle_query(cp, &i.id, &i.style);
    }

    width  = i.style.width  / (double) GR_PIXITS_PER_POINT;
    height = i.style.height / (double) GR_PIXITS_PER_POINT;

    cep->riscos.processing_fontselector = 1;

    fontselect_process(title,                                    /* win title   */
                       fontselect_SETFONT | fontselect_SETTITLE, /* flags       */
                       i.style.tstrFontName,                       /* font_name   */
                       &width, /* font width  */
                       &height, /* font height */
                       NULL, NULL,
                       gr_chart_edit_fontselect_unknown_fn, &i);

    /* we may be dead */
    cep = gr_chart_edit_cep_from_ceh(i.ceh);
    if(cep)
        cep->riscos.processing_fontselector = 0;

    return(0);
}

/******************************************************************************
*
* remove the selected text and its attributes
*
******************************************************************************/

extern STATUS
gr_chart_edit_selection_text_delete(P_GR_CHART_EDIT cep)
{
    P_GR_CHART cp;

    assert(cep);

    cp = gr_chart_cp_from_ch(cep->ch);
    assert(cp);

    gr_text_delete(cp, cep->selection.id.no);

    return(STATUS_OK);
}

typedef enum
    {
    GR_OBJECT_DRAG_REPOSITION = 0,
    GR_OBJECT_DRAG_RESIZE     = 1
    }
GR_OBJECT_DRAG_TYPE;

static GR_POINT drag_start_point;
static GR_POINT drag_curr_point;
static GR_OBJECT_DRAG_TYPE drag_type;

/* ------------------------------------------------------------------------- */
/*                                                                           */
/* Rectangle outline                                                         */
/*                                                                           */
/* tests for height or width zero, to prevent double plotting                */
/*                                                                           */

static void
displ_box(int x0,int y0, int x1,int y1)
{
  bbc_move(x0,y0);

  if(x0 == x1)
      bbc_draw(x0,y1);
  else if (y0 == y1)
      bbc_draw(x1,y0);
  else
  {
      os_plot(bbc_SolidExInit | bbc_DrawAbsFore, x1, y0);
      os_plot(bbc_SolidExInit | bbc_DrawAbsFore, x1, y1);
      os_plot(bbc_SolidExInit | bbc_DrawAbsFore, x0, y1);
      os_plot(bbc_SolidExInit | bbc_DrawAbsFore, x0, y0);
  }
}

static void
object_dragging_eor_bbox(P_GR_CHART_EDIT cep, P_GR_CHART cp, P_GR_POINT /*const*/ start_point, P_GR_POINT /*const*/ curr_point)
{
    GR_POINT moveby;
    GR_POINT upperleft, lowerright;
    GR_OSUNIT os_orgx, os_orgy;
    wimp_box os_outline;
    wimp_redrawstr r;
    int more;

    moveby.x = curr_point->x - start_point->x;
    moveby.y = curr_point->y - start_point->y;

    /* reposition adjusts all coords, resize adjusts (x1, y0) only */

    lowerright.x = cep->selection.box.x1 + moveby.x;
    lowerright.y = cep->selection.box.y0 + moveby.y;

    upperleft.x = cep->selection.box.x0;
    upperleft.y = cep->selection.box.y1;

    if(drag_type == GR_OBJECT_DRAG_REPOSITION)
    {
        upperleft.x += moveby.x;
        upperleft.y += moveby.y;
    }

    /* scale by chart zoom factor (pixit->pixit) */
    gr_coord_point_scale(addr_of_coord_point(lowerright), NULL, NULL, &cep->riscos.scale_from_diag16);
    gr_coord_point_scale(addr_of_coord_point(upperleft),  NULL, NULL, &cep->riscos.scale_from_diag16);

    /* plot origin (os_coords) */
    os_orgx = cp->core.editsave.open_box.x0 - cp->core.editsave.open_scx;
    os_orgy = cp->core.editsave.open_box.y0;

    /* add in display offset */
    os_orgx += cep->riscos.diagram_off_x;
    os_orgy += cep->riscos.diagram_off_y;

    /* bbox in absolute screen coords (os_coords) */
    os_outline.x0 = (int) (os_orgx + gr_riscos_from_pixit(upperleft.x ));
    os_outline.y0 = (int) (os_orgy + gr_riscos_from_pixit(lowerright.y));
    os_outline.x1 = (int) (os_orgx + gr_riscos_from_pixit(lowerright.x));
    os_outline.y1 = (int) (os_orgy + gr_riscos_from_pixit(upperleft.y ));

    r.w      = cep->riscos.w;
    r.box.x0 = -0x1FFFFFFF; r.box.y0 = -0x1FFFFFFF;
    r.box.x1 =  0x1FFFFFFF; r.box.y1 = 0; /* 0x1FFFFFFF; */     /* RCM says that SKS claimed zero was a good upper limit */

    if(new_wimpt_complain(wimp_update_wind(&r, &more)))
        more = FALSE;

    while(more)
    {
        bbc_gcol(3, 15); /*>>>eor in light-blue */

        displ_box(os_outline.x0, os_outline.y0, os_outline.x1, os_outline.y1);

        if(new_wimpt_complain(wimp_get_rectangle(&r, &more)))
            more = FALSE;
    }
}

extern void
gr_chart_edit_selected_object_drag_start(P_GR_CHART_EDIT cep, P_GR_POINT /*const*/ point, P_GR_POINT /*const*/ workareaoff)
{
    P_GR_CHART cp = gr_chart_cp_from_ch(cep->ch);
    GR_POINT resize_patch;

    UNREFERENCED_PARAMETER(workareaoff);

    assert(cp);

    /* bbox of object is in cep->selection.box, point clicked in point, both are in GR_PIXITs */

    trace_4(TRACE_MODULE_GR_CHART, "gr_chart_edit_selected_object_drag point (" S32_FMT "," S32_FMT "), workareaoff(" S32_FMT "," S32_FMT ")\n", point->x, point->y, workareaoff->x, workareaoff->y);

    /* treat the lower right hand corner of the bounding box as 'drag-to-resize', */
    /* the rest rest of the box as 'drag-to-reposition'                           */

    resize_patch.x = cep->selection.box.x1 - min(gr_pixit_from_riscos(16), ((cep->selection.box.x1 - cep->selection.box.x0) / 2));
    resize_patch.y = cep->selection.box.y0 + min(gr_pixit_from_riscos(8),  ((cep->selection.box.y1 - cep->selection.box.y0) / 2));

    drag_type = ((point->x >= resize_patch.x) && (point->y <= resize_patch.y))
              ? GR_OBJECT_DRAG_RESIZE
              : GR_OBJECT_DRAG_REPOSITION;

    if(status_ok(Null_EventProc(NULL, 0, gr_chart_edit_selected_object_drag_null_handler, (P_ANY) cp->core.ch, TRUE)))
    {
        wimp_dragstr dragstr;

        /* confine the drag to the chart edit window */
        dragstr.window    = cep->riscos.w;          /* Needed by win_drag_box, so it can send EUSERDRAG to us */
        dragstr.type      = wimp_USER_HIDDEN;
#if FALSE
        /* Window Manager ignores inner box on hidden drags */
        dragstr.box.x0    = mx;
        dragstr.box.y0    = my;
        dragstr.box.x1    = mx+30;
        dragstr.box.y1    = my+30;
#endif
        dragstr.parent.x0 = cp->core.editsave.open_box.x0;
        dragstr.parent.y0 = cp->core.editsave.open_box.y0;
        dragstr.parent.x1 = cp->core.editsave.open_box.x1;
        dragstr.parent.y1 = cp->core.editsave.open_box.y1;

        new_wimpt_complain(win_drag_box(&dragstr));     /* NB win_drag_box NOT wimp_drag_box */

        drag_start_point = *point;
        drag_curr_point  = *point;

        object_dragging_eor_bbox(cep, cp, &drag_start_point, &drag_curr_point);
    }
}

extern void
gr_chart_edit_selected_object_drag_complete(P_GR_CHART_EDIT cep, const wimp_box * dragboxp)
{
    P_GR_CHART cp = gr_chart_cp_from_ch(cep->ch);
    GR_POINT moveby;

    UNREFERENCED_PARAMETER(dragboxp);

    trace_0(TRACE_MODULE_GR_CHART, "[gr_chart_edit_selected_object_drag_complete]\n");

    assert(cp);

    object_dragging_eor_bbox(cep, cp, &drag_start_point, &drag_curr_point);

    moveby.x  = drag_curr_point.x - drag_start_point.x;
    moveby.y  = drag_curr_point.y - drag_start_point.y;

    (void) Null_EventHandler(gr_chart_edit_selected_object_drag_null_handler, (P_ANY) cp->core.ch, FALSE, 0);

    switch(cep->selection.id.name)
    {
        case GR_CHART_OBJNAME_TEXT:
        {
            GR_BOX box;

            gr_text_box_query(cp, cep->selection.id.no, &box);

            /* reposition adjusts all coords, resize adjusts (x1, y0) only */

            box.x1 += moveby.x;
            box.y0 += moveby.y;

            if(drag_type == GR_OBJECT_DRAG_REPOSITION)
            {
                box.x0 += moveby.x;
                box.y1 += moveby.y;
            }

            gr_text_box_set(cp, cep->selection.id.no, &box);
        }
            break;

        case GR_CHART_OBJNAME_LEGEND:
        {
            GR_BOX legendbox;

            /* reposition adjusts all coords, resize adjusts bottom right corner only */
            legendbox.x0 = cp->legend.posn.x;
            legendbox.y0 = cp->legend.posn.y;
            legendbox.x1 = cp->legend.posn.x + cp->legend.size.x;
            legendbox.y1 = cp->legend.posn.y + cp->legend.size.y;

            legendbox.x1 += moveby.x;
            legendbox.y0 += moveby.y;

            if(drag_type == GR_OBJECT_DRAG_REPOSITION)
            {
                legendbox.x0 += moveby.x;
                legendbox.y1 += moveby.y;
            }

            gr_box_sort(&legendbox, NULL);       /* in case scale in x or y was -ve */

            cp->legend.posn.x = legendbox.x0;
            cp->legend.posn.y = legendbox.y0;
            cp->legend.size.x = legendbox.x1 - legendbox.x0;
            cp->legend.size.y = legendbox.y1 - legendbox.y0;

            cp->legend.bits.manual = 1;
        }
            break;
    }

    gr_chart_modify_and_rebuild(&cep->ch);
}

static
PROC_EVENT_PROTO(gr_chart_edit_selected_object_drag_null_handler)
{
    P_NULL_EVENT_BLOCK p_null_event_block = p_data;
    GR_CHART_HANDLE ch = p_null_event_block->client_handle;

    switch(p_null_event_block->rc)
    {
        case NULL_QUERY:
            return(NULL_EVENTS_REQUIRED);

        case NULL_EVENT:
        {
            P_GR_CHART cp = gr_chart_cp_from_ch(ch);
            WimpGetPointerInfoBlock pointer_info;
            GR_POINT point;

            trace_0(TRACE_MODULE_GR_CHART, "gr_chart_edit_selected_object_drag_null_handler, ");

            assert(cp);

            void_wimpt_complain(wimp_get_pointer_info(&pointer_info));

            gr_chart_edit_riscos_point_from_abs(cp, &point, pointer_info.x, pointer_info.y);

            trace_2(TRACE_MODULE_GR_CHART, "point (" S32_FMT "," S32_FMT ")\n", point.x, point.y);

            if((drag_curr_point.x != point.x) || (drag_curr_point.y != point.y))
            {
                P_GR_CHART_EDIT cep = gr_chart_edit_cep_from_ceh(cp->core.ceh);
                object_dragging_eor_bbox(cep, cp, &drag_start_point, &drag_curr_point); /* remove from old position */
                drag_curr_point = point;
                object_dragging_eor_bbox(cep, cp, &drag_start_point, &drag_curr_point); /* show at new position */
            }
          /*else            */
          /*    no movement */

            return(NULL_EVENT_COMPLETED);
        }

        default:
            return(0);
    }
}

extern STATUS
gr_chart_edit_selection_text_edit(P_GR_CHART_EDIT cep, BOOL submenurequest)
{
    P_GR_CHART cp;

    assert(cep);

    if(cep->selection.id.name != GR_CHART_OBJNAME_TEXT)
        return(0);

    cp = gr_chart_cp_from_ch(cep->ch);
    assert(cp);

    return(gr_chart_edit_text_editor_make(cp, cep->selection.id.no));
}

/******************************************************************************
*
* remove the editing window from the given text object in this chart
*
******************************************************************************/

extern void
gr_chart_edit_text_editor_kill(P_GR_CHART cp, LIST_ITEMNO key)
{
    P_GR_TEXT t;

    if((t = list_gotoitemcontents((P_LIST_BLOCK) &cp->text.lbr, key)) != NULL)
    {
        if(t->bits.being_edited)
        {
            t->bits.being_edited = 0;

            /* <<< kill kill kill */
        }
        else
            trace_1(TRACE_MODULE_GR_CHART, "[text object " U32_FMT " not being edited]\n", key);
    }
}

/******************************************************************************
*
* start up an editing window on the given text object in this chart
*
******************************************************************************/

static STATUS
gr_chart_edit_text_editor_make(P_GR_CHART cp, LIST_ITEMNO key)
{
    STATUS status = STATUS_OK;
    P_GR_TEXT t;

    if((t = list_gotoitemcontents((P_LIST_BLOCK) &cp->text.lbr, key)) == NULL)
        /* wot? */
        return(status);

    if(t->bits.being_edited)
        /* sounds ok ... */
        return(status);

    {
    GR_CHART_OBJID id;
    P_GR_TEXT_GUTS gutsp;
    GR_CHART_VALUE value;
    P_U8 p_u8;

    gr_chart_objid_from_text(key, &id);

    gutsp.p_gr_text = t + 1;

    if(t->bits.live_text)
    {
        gr_travel_dsp(cp, gutsp.p_gr_datasource, 0, &value);

        p_u8 = value.data.text;
    }
    else
        p_u8 = gutsp.textp;
    }

    if(status_ok(status))
        /*status = mlsubmenu_process(&mlsubmenu, submenu)*/;

    if(status_ok(status))
    {
        /* successful edit, copy over text descriptor and new text*/
        GR_TEXT temp = *t; /* save current header */
        P_UI_TEXT p_ui_text;
        size_t nBytes = ui_text_len(p_ui_text) + 1; /* + 1 for NULLCH */

        /* overwrite existing entry */
        if((t = collect_add_entry((P_LIST_BLOCK) &cp->text.lbr, sizeof(*t) + nBytes, key)) != NULL)
        {
            P_GR_TEXT_GUTS gutsp;

            /* copy over header */
            *t = temp;

            gutsp.mp = t + 1;

            /* validate as used after editing complete */
            t->bits.unused = 0;

            if(t->bits.live_text)
            {
                t->bits.live_text = 0;

                /* kill client dep */
                gr_travel_dsp(cp, gutsp.p_gr_datasource, 0, NULL);
            }

            /* copy over edited text */
            /*mlsubmenu_gettext(&mlsubmenu, gutsp.textp, nBytes);*/
        }
        else
            status = status_nomem();
    }

    return(status);
}

extern STATUS
gr_chart_edit_text_new_and_edit(P_GR_CHART cp, LIST_ITEMNO key, P_U8 /*const*/ text, P_GR_POINT point)
{
    P_GR_TEXT t;

    status_return(gr_text_new(cp, key, text, point));

    t = list_gotoitemcontents((P_LIST_BLOCK) &cp->text.lbr, key);
    assert(t);

    t->bits.unused = 1; /* as yet */

    return(gr_chart_edit_text_editor_make(cp, key));
}

#endif

/* end of gr_uidlg.c */
