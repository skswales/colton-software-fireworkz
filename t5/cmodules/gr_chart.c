/* gr_chart.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Charting module interface */

/* SKS May 1991 */

#include "common/gflags.h"

#include "ob_chart/ob_chart.h"

#include "cmodules/collect.h"

/*
internal functions
*/

#ifdef GR_CLONE

_Check_return_
static STATUS
gr_chart_clone(
    _InVal_     GR_CHART_HANDLE dst_ch,
    _InVal_     GR_CHART_HANDLE src_ch,
    _InVal_     S32 non_core_too);

static void
gr_chart_clone_noncore_pict_lose_refs(
    _InVal_     GR_CHART_HANDLE ch);

#endif

static S32
gr_chart_initialised = FALSE;

/*
a list of charts
*/

static LIST_BLOCK
gr_charts;

/*
exported variables
*/

const GR_CHART_OBJID
gr_chart_objid_anon = GR_CHART_OBJID_INIT_NAME(GR_CHART_OBJNAME_ANON);

const GR_CHART_OBJID
gr_chart_objid_chart = GR_CHART_OBJID_INIT_NAME(GR_CHART_OBJNAME_CHART);

const GR_CHART_OBJID
gr_chart_objid_legend = GR_CHART_OBJID_INIT_NAME(GR_CHART_OBJNAME_LEGEND);

/*
preferred defaults - exported solely for check in chtIO
*/

/*const-to-them*/ GR_CHART_HANDLE
gr_chart_preferred_ch = GR_CHART_HANDLE_NONE;

/******************************************************************************
*
* add a data source to a chart
*
******************************************************************************/

_Check_return_
extern STATUS
gr_chart_add(
    /*inout*/ P_GR_CHART_HANDLE chp,
    _InRef_     P_PROC_GR_CHART_TRAVEL proc,
    P_ANY ext_handle,
    P_GR_INT_HANDLE p_int_handle_out)
{
    return(gr_chart_insert(chp, proc, ext_handle, p_int_handle_out, GR_DATASOURCE_HANDLE_NONE));
}

/******************************************************************************
*
* add a datasource as the labels datasource
* even with multiple axes there is only one
* if punter tries to add more then let him
* but its really ignored by us (e.g. add range)
*
******************************************************************************/

_Check_return_
extern STATUS
gr_chart_add_labels(
    /*inout*/ P_GR_CHART_HANDLE chp,
    _InRef_     P_PROC_GR_CHART_TRAVEL proc,
    P_ANY ext_handle,
    P_GR_INT_HANDLE p_int_handle_out)
{
    const P_GR_CHART cp = p_gr_chart_from_chart_handle(*chp);

    if(cp->core.category_datasource.dsh != GR_DATASOURCE_HANDLE_NONE)
    {
        myassert0(TEXT("gr_chart_add_labels: chart already has labels"));
        *p_int_handle_out = GR_DATASOURCE_HANDLE_NONE;
    }
    else
        (void) gr_chart_datasource_insert(cp, proc, ext_handle, p_int_handle_out, GR_DATASOURCE_HANDLE_START);

    return(STATUS_OK);
}

/******************************************************************************
*
* add some live text to the chart
*
******************************************************************************/

_Check_return_
extern STATUS
gr_chart_add_text(
    /*inout*/ P_GR_CHART_HANDLE chp,
    _InRef_     P_PROC_GR_CHART_TRAVEL proc,
    P_ANY ext_handle,
    P_GR_INT_HANDLE p_int_handle_out)
{
    return(gr_chart_insert(chp, proc, ext_handle, p_int_handle_out, GR_DATASOURCE_HANDLE_TEXTS));
}

/******************************************************************************
*
* allow clients to reregister
*
******************************************************************************/

_Check_return_
extern STATUS
gr_chart_change_handle(
    _InVal_     GR_CHART_HANDLE ch,
    _InVal_     GR_DATASOURCE_HANDLE int_handle,
    P_ANY ext_handle)
{
    const P_GR_CHART cp = p_gr_chart_from_chart_handle_maybenull(ch);

    trace_2(TRACE_MODULE_GR_CHART, TEXT("gr_chart_update_handle(") ENUM_XTFMT TEXT(", ") ENUM_XTFMT TEXT(")"), ch, int_handle);

    if((GR_DATASOURCE_HANDLE_NONE != int_handle) && (NULL != cp))
    {
        P_GR_DATASOURCE p_gr_datasource;

        if(cp->core.category_datasource.dsh == int_handle)
        {
            cp->core.category_datasource.ext_handle = ext_handle;
            return(STATUS_DONE);
        }

        if(NULL != (p_gr_datasource = gr_chart_datasource_p_from_h(cp, int_handle)))
        {
            p_gr_datasource->ext_handle = ext_handle;
            return(STATUS_DONE);
        }
    }

    return(STATUS_OK);
}

/******************************************************************************
*
* use external handle to dial up reference to underlying chart structure
*
******************************************************************************/

_Check_return_
_Ret_valid_
extern P_GR_CHART
p_gr_chart_from_chart_handle(
    _InVal_     GR_CHART_HANDLE ch)
{
    const LIST_ITEMNO key = (LIST_ITEMNO) ch;
    const P_GR_CHART cp = key ? collect_goto_item(GR_CHART, &gr_charts, key) : NULL;
    myassert1x((NULL != cp) || !key, TEXT("p_gr_chart_from_chart_handle: failed to find chart handle " ENUM_XTFMT), ch);
    PTR_ASSERT(cp);
    return(cp);
}

_Check_return_
_Ret_maybenull_
extern P_GR_CHART
p_gr_chart_from_chart_handle_maybenull(
    _InVal_     GR_CHART_HANDLE ch)
{
    const LIST_ITEMNO key = (LIST_ITEMNO) ch;
    const P_GR_CHART cp = key ? collect_goto_item(GR_CHART, &gr_charts, key) : NULL;
    return(cp);
}

/******************************************************************************
*
* damage a datasource
*
******************************************************************************/

extern void
gr_chart_damage(
    _InVal_     GR_CHART_HANDLE ch,
    _InVal_     GR_DATASOURCE_HANDLE int_handle)
{
    const P_GR_CHART cp = p_gr_chart_from_chart_handle_maybenull(ch);
    P_GR_DATASOURCE p_gr_datasource;
    GR_AXES_IDX axes_idx;
    GR_SERIES_IDX series_idx;
    P_GR_SERIES serp;

    trace_2(TRACE_MODULE_GR_CHART, TEXT("gr_chart_damage(") ENUM_XTFMT TEXT(", ") ENUM_XTFMT TEXT(")"), ch, int_handle);

    if(NULL == cp)
        return;

    cp->core.modified = 1;

    if(int_handle == GR_DATASOURCE_HANDLE_NONE)
    {
        assert(int_handle != GR_DATASOURCE_HANDLE_NONE);
        return;
    }

    if(int_handle == cp->core.category_datasource.dsh)
    {
        * (int *) &cp->core.category_datasource.valid = 0;

        for(axes_idx = 0; axes_idx <= cp->axes_idx_max; ++axes_idx)
        {
            for(series_idx = cp->axes[axes_idx].series.stt_idx; series_idx < cp->axes[axes_idx].series.end_idx; ++series_idx)
            {
                /* all series in chart depend on this! */
                serp = getserp(cp, series_idx);

                * (int *) &serp->valid = 0;
            }
        }

        return;
    }

    if(NULL != (p_gr_datasource = gr_chart_datasource_p_from_h(cp, int_handle)))
    {
        * (int *) &p_gr_datasource->valid = 0;

        switch(p_gr_datasource->id.name)
        {
        case GR_CHART_OBJNAME_TEXT:
            /* internal reformat of text object needed */
            break;

        case GR_CHART_OBJNAME_SERIES:
            {
            series_idx = gr_series_idx_from_external(cp, p_gr_datasource->id.no);

            serp = getserp(cp, series_idx);

            * (int *) &serp->valid = 0;

            break;
            }

        default: default_unhandled();
#if CHECKING
        case GR_CHART_OBJNAME_ANON:
#endif
            /* SKS after PD 4.12 30mar92 - note that damages can come through to a datasource even when not yet assigned to a series */
            break;
        }

        return;
    }

    PTR_ASSERT(p_gr_datasource);
}

/******************************************************************************
*
* dispose of a chart
*
******************************************************************************/

static void
gr_chart_dispose_core(
    _ChartRef_  P_GR_CHART cp)
{
    gr_diag_diagram_dispose(&cp->core.p_gr_diag);

    al_array_dispose(&cp->core.datasources.mh);

    tstr_clr(&cp->core.currentfilename);
    tstr_clr(&cp->core.currentdrawname);

    zero_struct(cp->core);
}

static void
gr_chart_dispose_noncore(
    _ChartRef_  P_GR_CHART cp)
{
    S32 plotidx;
    GR_SERIES_IDX series_idx;

    /* lose refs to pictures */
    gr_fillstyleb_pict_delete(&cp->chart.areastyleb);

    for(plotidx = 0; plotidx < GR_CHART_N_PLOTAREAS; ++plotidx)
        gr_fillstyleb_pict_delete(&cp->plotarea.area[plotidx].areastyleb);

    gr_fillstyleb_pict_delete(&cp->legend.areastyleb);

    /* delete text and text styles from chart */
    gr_chart_list_delete(cp, GR_LIST_CHART_TEXT);
    gr_chart_list_delete(cp, GR_LIST_CHART_TEXT_TEXTSTYLE);

    for(series_idx = 0; series_idx < array_elements(&cp->series.mh); ++series_idx)
        gr_chart_free_series(cp, series_idx);

    al_array_dispose(&cp->series.mh);

    assert_EQ(offsetof32(GR_CHART, core), 0); /* else we'd need another memset of noncore info */
    memset32(PtrAddBytes(P_BYTE, &cp->core, sizeof32(cp->core)), 0, sizeof32(*cp) - sizeof32(cp->core));
}

extern void
gr_chart_dispose(
    _InoutRef_  P_GR_CHART_HANDLE chp)
{
    GR_CHART_HANDLE ch;
    P_GR_CHART cp;
    LIST_ITEMNO key;

    trace_2(TRACE_MODULE_GR_CHART, TEXT("gr_chart_dispose(") PTR_XTFMT TEXT("->") ENUM_XTFMT TEXT(")"), chp, chp ? *chp : 0);

    ch = *chp;
    *chp = GR_CHART_HANDLE_NONE;
    if(GR_CHART_HANDLE_NONE == ch)
        return;

    /* dispose of substructure (any dependent editors must be dealt with separately) */
    cp = p_gr_chart_from_chart_handle(ch);
    myassert1x(!cp->core.ceh, TEXT("chart editor for chart ") UINTPTR_XTFMT TEXT(" is still open!"), (uintptr_t) ch);

    gr_chart_dispose_noncore(cp);

    gr_chart_dispose_core(cp);

    /* reconvert ch explicitly for subtract */
    key = (LIST_ITEMNO) ch;

    trace_1(TRACE_MODULE_GR_CHART, TEXT("gr_chart_dispose: collect_subtract_entry ") S32_TFMT TEXT(" from gr_charts list"), key);
    collect_subtract_entry(&gr_charts, key);
    collect_compress(&gr_charts); /* SKS after 1.05 28oct93 */
}

/******************************************************************************
*
* initialisation for the chart module and friends
*
******************************************************************************/

static void
gr_chart_initialise(void)
{
    /* enumerate marker and fill files and add cache entries (but don't actually cache) */

    gr_chart_initialised = 1;
}

/******************************************************************************
*
* insert a data source in a chart AFTER a given entry
*
******************************************************************************/

_Check_return_
extern STATUS
gr_chart_insert(
    /*inout*/ P_GR_CHART_HANDLE chp,
    _InRef_     P_PROC_GR_CHART_TRAVEL proc,
    P_ANY ext_handle,
    _OutRef_    P_GR_INT_HANDLE p_int_handle_out,
    _InVal_     GR_INT_HANDLE int_handle_after)
{
    const P_GR_CHART cp = p_gr_chart_from_chart_handle_maybenull(*chp);
    GR_DATASOURCE_HANDLE dsh;

    *p_int_handle_out = GR_DATASOURCE_HANDLE_NONE;

    if(NULL == cp)
        return(0);

    dsh = gr_chart_datasource_insert(cp, proc, ext_handle, p_int_handle_out, int_handle_after);

    return(dsh != GR_DATASOURCE_HANDLE_NONE);
}

/******************************************************************************
*
* modify this chart and save if possible
*
******************************************************************************/

extern void
gr_chart_modify_and_rebuild(
    _InVal_     GR_CHART_HANDLE ch)
{
    const P_GR_CHART cp = p_gr_chart_from_chart_handle_maybenull(ch);

    if(NULL == cp)
        return;

    cp->core.modified = 1;

    gr_chart_diagram_ensure(ch);
}

/******************************************************************************
*
* ask chart for its filename if any
*
******************************************************************************/

_Check_return_
extern STATUS
gr_chart_name_query(
    _InVal_     GR_CHART_HANDLE ch,
    _Out_writes_z_(elemof_buffer) PTSTR tstr_buf,
    _InVal_     U32 elemof_buffer)
{
    const P_GR_CHART cp = p_gr_chart_from_chart_handle(ch);

    tstr_xstrkpy(tstr_buf, elemof_buffer, cp->core.currentfilename);

    return(STATUS_DONE);
}

_Check_return_
extern STATUS
gr_chart_name_set(
    _InVal_     GR_CHART_HANDLE ch,
    _In_z_      PCTSTR filename)
{
    const P_GR_CHART cp = p_gr_chart_from_chart_handle(ch);

    tstr_clr(&cp->core.currentfilename);

    status_return(tstr_set(&cp->core.currentfilename, filename));

    return(STATUS_DONE);
}

/******************************************************************************
*
* create a chart
*
******************************************************************************/

static const GR_CHART_LAYOUT
gr_chart_layout_default =
{
    (GR_PIXIT) (FP_PIXITS_PER_MM * (60.0 + 15.0 + 15.0)), /* w */
    (GR_PIXIT) (FP_PIXITS_PER_MM * (60.0 + 10.0 + 10.0)), /* h */

    { (GR_PIXIT) (FP_PIXITS_PER_MM *  15.0), /* LM */
      (GR_PIXIT) (FP_PIXITS_PER_MM *  10.0), /* BM */
      (GR_PIXIT) (FP_PIXITS_PER_MM *  15.0), /* RM */
      (GR_PIXIT) (FP_PIXITS_PER_MM *  10.0)  /* TM */ }

    /* dependent fields filled in by user of this structure */
};

_Check_return_
extern STATUS
gr_chart_new(
    /*out*/ P_GR_CHART_HANDLE chp,
    P_ANY ext_handle,
    _InVal_     S32 new_untitled)
{
    static LIST_ITEMNO cpkey_gen = 0x32461000; /* NB. not tbs! */

    static U32 nextUntitledNumber = 1;

    STATUS status;
    GR_CHART_HANDLE ch;
    P_GR_CHART cp;
    LIST_ITEMNO key;
    TCHARZ filename[BUF_MAX_PATHSTRING];
    GR_AXES_IDX axes_idx;
    GR_AXIS_IDX axis_idx;

    if(!gr_chart_initialised)
        gr_chart_initialise();

    gr_chart_validate_block((P_ANY) 1, "in>new"); /* full validation enable */
    gr_chart_validate_heap("in>new");

    *chp = GR_CHART_HANDLE_NONE;

    /* add to list of charts */
    key = cpkey_gen++;

    if(NULL == (cp = collect_add_entry_elem(GR_CHART, &gr_charts, P_DATA_NONE, key, &status)))
        return(status);

    /* convert ch explicitly */
    ch = (GR_CHART_HANDLE) key;

    *chp = ch;

    /* empty the descriptor */
    zero_struct_ptr(cp);

    cp->core.ch = ch;

    cp->core.ext_handle = ext_handle;

    /* NB. there is only one of these per chart */
    cp->core.category_datasource.dsh = GR_DATASOURCE_HANDLE_NONE;

    /* initial size of chart */
    cp->core.layout = gr_chart_layout_default;

    cp->core.layout.size.x = cp->core.layout.width  - (cp->core.layout.margins.left   + cp->core.layout.margins.right);
    cp->core.layout.size.y = cp->core.layout.height - (cp->core.layout.margins.bottom + cp->core.layout.margins.top  );

    /* SKS after PD 4.12 25mar92 - must reallocate series to chart on first gr_chart_build() even if no datasources have been added */
    cp->bits.realloc_series = 1;

    cp->d3.droop = 10.0; /* DEGREES */
    cp->d3.turn  = 10.0;

    cp->barch.slot_overlap_percentage = 0.0; /* no overlap */
    cp->linech.slot_shift_percentage = 0.0; /* complete overlap */

    /* normal axes start out life as bar */
    cp->axes[0].sertype = GR_CHART_SERIES_PLAIN;
    cp->axes[0].chart_type = GR_CHART_TYPE_BAR;

    /* overlay axes always starts out life as line */
    cp->axes[1].sertype = GR_CHART_SERIES_PLAIN;
    cp->axes[1].chart_type = GR_CHART_TYPE_LINE;

    for(axes_idx = 0; axes_idx <= GR_AXES_IDX_MAX; ++axes_idx)
    {
        const P_GR_AXES axesp = &cp->axes[axes_idx];

        axesp->style.barch.slot_width_percentage = 100.0; /* fill slots widthways */
        axesp->style.barlinech.slot_depth_percentage =  75.0;
        axesp->style.linech.slot_width_percentage =  20.0; /* fractionally fill slots widthways */
        axesp->style.piechdispl.radial_displacement =   0.0;
        axesp->style.scatch.width_percentage =  20.0;

        /* ensure options saved */
        axesp->style.barch.bits.manual = 1;
        axesp->style.barlinech.bits.manual = 1;
        axesp->style.linech.bits.manual = 1;
        axesp->style.piechdispl.bits.manual = 1;
        axesp->style.piechlabel.bits.manual = 1;
        axesp->style.scatch.bits.manual = 1;

        for(axis_idx = 0; axis_idx < 3; ++axis_idx)
        {
            const P_GR_AXIS axisp = &axesp->axis[axis_idx];

            axisp->bits.incl_zero = 1; /* this is RJM's fault */

            axisp->punter.min =  0.0;
            axisp->punter.max = 10.0;

            axisp->major.punter =  1.0;
            axisp->major.bits.tick = GR_AXIS_TICK_POSITION_FULL;

            axisp->minor.punter =  0.2;
            axisp->minor.bits.tick = GR_AXIS_TICK_POSITION_NONE;
        }
    }

    cp->axes[1].axis[X_AXIS_IDX].bits.lzr = GR_AXIS_POSITION_BZT_TOP; /* bzt for category or x-axis */
    cp->axes[1].axis[Y_AXIS_IDX].bits.lzr = GR_AXIS_POSITION_LZR_RIGHT;

    gr_colour_set_BLACK(  cp->chart.borderstyle.fg);
                        /*cp->chart.borderstyle.width = 4;*/ /* 0.2 point */
                          cp->chart.borderstyle.fg.manual = 1;
    gr_colour_set_WHITE(  cp->chart.areastylec.fg);
                          cp->chart.areastylec.fg.manual = 1;

                          cp->chart.areastyleb.bits.manual = 1;
                          cp->chart.areastyleb.bits.norecolour = 1;

    gr_colour_set_MIDGRAY(cp->plotarea.area[0].borderstyle.fg);
                          cp->plotarea.area[0].borderstyle.fg.manual = 1;
#if WINDOWS && 1 /* <<< tmp fudge? */
    gr_colour_set_RGB(cp->plotarea.area[0].areastylec.fg, rgb_stash[1].r, rgb_stash[1].g, rgb_stash[1].b); /* lightest grey */
#else
    gr_colour_set_VLTGRAY(cp->plotarea.area[0].areastylec.fg);
#endif
                          cp->plotarea.area[0].areastylec.fg.manual = 1;

                          cp->plotarea.area[0].areastyleb.bits.norecolour = 1;
                          cp->plotarea.area[0].areastyleb.bits.norecolour = 1;

    cp->plotarea.area[1].borderstyle.fg = cp->plotarea.area[0].borderstyle.fg;
    cp->plotarea.area[2].borderstyle.fg = cp->plotarea.area[0].borderstyle.fg;

    gr_colour_set_LTGRAY( cp->plotarea.area[1].areastylec.fg);
                          cp->plotarea.area[1].areastylec.fg.manual = 1;
                          cp->plotarea.area[1].areastyleb.bits.manual = 1;

    gr_colour_set_LTGRAY( cp->plotarea.area[2].areastylec.fg);
                          cp->plotarea.area[2].areastylec.fg.manual = 1;
                          cp->plotarea.area[2].areastyleb.bits.manual = 1;

    gr_colour_set_BLACK(  cp->legend.borderstyle.fg);
                          cp->legend.borderstyle.fg.manual = 1;
    gr_colour_set_WHITE(  cp->legend.areastylec.fg);
                          cp->legend.areastylec.fg.manual = 1;

                          cp->legend.areastyleb.bits.norecolour = 1;
                          cp->legend.areastyleb.bits.norecolour = 1;

    gr_colour_set_BLACK(  cp->text.style.base.fg);
                          cp->text.style.base.fg.manual = 1;

    {
    PC_USTR ustr = resource_lookup_ustr(CHART_MSG_DEFAULT_FONT);
    PIXIT height, width;
    height = (PIXIT) fast_ustrtoul(ustr, &ustr) * GR_PIXITS_PER_POINT;
    ustr_IncByte(ustr);
    width = (PIXIT) fast_ustrtoul(ustr, &ustr) * GR_PIXITS_PER_POINT;
    ustr_IncByte(ustr);
    cp->text.style.base.size_y = height;
    cp->text.style.base.size_x = width;
    tstr_xstrkpy(cp->text.style.base.tstrFontName, elemof32(cp->text.style.base.tstrFontName), _tstr_from_ustr(ustr));
    } /*block*/

    if(new_untitled)
        consume_int(tstr_xsnprintf(filename, elemof32(filename),
                                   resource_lookup_tstr(CHART_MSG_DEFAULT_CHARTZD),
                                   nextUntitledNumber++));
    else
        filename[0] = CH_NULL;

#ifdef GR_CHART_SAVES_ONLY_DRAWFILE
    tstr_clr(&cp->core.currentdrawname);
    status_assert(tstr_set(&cp->core.currentdrawname, filename));
#endif

    tstr_clr(&cp->core.currentfilename);
    status_assert(tstr_set(&cp->core.currentfilename, filename));

    gr_chart_validate_heap("new>out");

    return(1);
}

extern U32
gr_chart_order_query(
    _InVal_     GR_CHART_HANDLE ch,
    _InVal_     GR_INT_HANDLE int_handle)
{
    const P_GR_CHART cp = p_gr_chart_from_chart_handle_maybenull(ch);
    GR_DATASOURCE_HANDLE dsh = int_handle;
    P_GR_DATASOURCE p_gr_datasource;

    if(NULL == cp)
        return(0);

    if(GR_DATASOURCE_HANDLE_NONE == dsh)
        return(0);

    if(dsh == cp->core.category_datasource.dsh)
        return(1);

    if((p_gr_datasource = gr_chart_datasource_p_from_h(cp, dsh)) == NULL)
        return(0);

    switch(p_gr_datasource->id.name)
    {
    case GR_CHART_OBJNAME_TEXT:
        {
        /* SKS after PD 4.12 27mar92 - needed for live text reload mechanism */
        const P_LIST_BLOCK p_list_block = &cp->text.lbr;
        LIST_ITEMNO key;
        P_GR_TEXT t;

        for(t = collect_first(GR_TEXT, p_list_block, &key);
            t;
            t = collect_next(GR_TEXT, p_list_block, &key))
        {
            P_GR_TEXT_GUTS gutsp;

            if(!t->bits.live_text)
                continue;

            gutsp.p_gr_text = t + 1;

            if(p_gr_datasource == gutsp.p_gr_datasource) /* datasource_p_from_h yielded this ptr */
                return(key);
        }

        myassert0(TEXT("No text"));
        return(0);
        }

    default: default_unhandled();
#if CHECKING
    case GR_CHART_OBJNAME_SERIES:
#endif
        {
#if CHECKING
        const GR_SERIES_IDX series_idx = gr_series_idx_from_external(cp, p_gr_datasource->id.no);
        const P_GR_SERIES serp = getserp(cp, series_idx);
        assert(p_gr_datasource->id.subno < GR_SERIES_MAX_DATASOURCES);
        assert(serp->datasources.dsh[p_gr_datasource->id.subno] == dsh);
#endif
        /* invert some sort of ordered key to pass back */
        return(((U32) p_gr_datasource->id.no + 1) * 0x100 + p_gr_datasource->id.subno);
        }
    }
}

/******************************************************************************
*
* ask chart whether category labels ought
* to be provided (they are optional even so)
*
******************************************************************************/

_Check_return_
extern STATUS
gr_chart_query_labelling(
    _InVal_     GR_CHART_HANDLE ch)
{
    P_GR_CHART cp = p_gr_chart_from_chart_handle_maybenull(ch);

    if(NULL != cp)
        return(cp->axes[0].chart_type != GR_CHART_TYPE_SCAT);

    /* return preferred state if no chart to enquire about */

    if(gr_chart_preferred_ch)
    {
        cp = p_gr_chart_from_chart_handle(gr_chart_preferred_ch);

        return(cp->axes[0].chart_type != GR_CHART_TYPE_SCAT);
    }

    /* hardwired state is bar chart so, yes, we will be labelling */

    return(1);
}

/******************************************************************************
*
* ask chart whether category labels have been provided
*
******************************************************************************/

_Check_return_
extern STATUS
gr_chart_query_labels(
    _InVal_     GR_CHART_HANDLE ch)
{
    const P_GR_CHART cp = p_gr_chart_from_chart_handle_maybenull(ch);

    return(cp ? (cp->core.category_datasource.dsh != GR_DATASOURCE_HANDLE_NONE) : 0);
}

/******************************************************************************
*
* remove a data source from a chart
*
******************************************************************************/

_Check_return_
extern STATUS
gr_chart_subtract(
    _InVal_     GR_CHART_HANDLE ch,
    /*inout*/ P_GR_INT_HANDLE p_int_handle)
{
    const P_GR_CHART cp = p_gr_chart_from_chart_handle(ch);
    const GR_DATASOURCE_HANDLE dsh = *p_int_handle;

    if(dsh == GR_DATASOURCE_HANDLE_NONE)
        return(0);

    *p_int_handle = GR_DATASOURCE_HANDLE_NONE;

    return(gr_chart_datasource_subtract_using_dsh(cp, dsh));
}

_Check_return_
extern STATUS
gr_chart_query_modified(
    _InVal_     GR_CHART_HANDLE ch)
{
    const P_GR_CHART cp = p_gr_chart_from_chart_handle(ch);

    PTR_ASSERT(cp);

    return(cp->core.modified);
}

#ifdef GR_CLONE

/******************************************************************************
*
* new chart using preferred state
*
******************************************************************************/

_Check_return_
extern STATUS
gr_chart_preferred_new(
    /*out*/ P_GR_CHART_HANDLE chp,
    P_ANY ext_handle)
{
    status_return(gr_chart_new(chp, ext_handle, 1));

    if(!gr_chart_preferred_ch)
        return(1);

#ifdef GR_CLONE
    return(gr_chart_clone(*chp, gr_chart_preferred_ch, 1 /*non-core info too*/));
#else
    return(status_nomem());
#endif
}

/******************************************************************************
*
* query the existence of a preferred state
*
******************************************************************************/

_Check_return_
extern STATUS
gr_chart_preferred_query(void)
{
    return(gr_chart_preferred_ch != 0);
}

/******************************************************************************
*
* save preferred state into a file
*
******************************************************************************/

_Check_return_
extern STATUS
gr_chart_preferred_save(
    _In_z_      PCTSTR filename)
{
#if 0
    if(gr_chart_preferred_ch)
        return(gr_chart_save_chart_without_dialog(gr_chart_preferred_ch, filename));
#else
    IGNOREPARM(filename);
#endif

    assert0();
    return(1);
}

/******************************************************************************
*
* copy chart data as preferred state
*
******************************************************************************/

_Check_return_
extern STATUS
gr_chart_preferred_set(
    _InVal_     GR_CHART_HANDLE ch)
{
    P_GR_CHART pcp;
    GR_SERIES_IDX series_idx;
    P_GR_SERIES serp;
    STATUS status;

    if(gr_chart_preferred_ch)
        gr_chart_dispose(&gr_chart_preferred_ch);

    status_return(status = gr_chart_new(&gr_chart_preferred_ch, &gr_chart_preferred_ch /*internal creation*/, 0));

#ifdef GR_CLONE
    if(status_fail(status = gr_chart_clone(gr_chart_preferred_ch, ch, 1 /*non-core info too*/)))
    {
        gr_chart_dispose(&gr_chart_preferred_ch);
        return(status);
    }

    gr_chart_clone_noncore_pict_lose_refs(gr_chart_preferred_ch);
#else
    IGNOREPARM(ch);
#endif

    pcp = p_gr_chart_from_chart_handle(gr_chart_preferred_ch);

    /* run over all series in preferred world and give them the ok */
    for(series_idx = 0; series_idx < array_elements(&pcp->series.mh); ++series_idx)
    {
        serp = getserp(pcp, series_idx);

        serp->internal_bits.descriptor_ok = 1;
    }

    return(status);
}

/******************************************************************************
*
* reflect preferred state into this chart
*
******************************************************************************/

_Check_return_
extern STATUS
gr_chart_preferred_use(
    _InVal_     GR_CHART_HANDLE ch)
{
    if(!gr_chart_preferred_ch)
        return(1);

#ifdef GR_CLONE
    return(gr_chart_clone(ch, gr_chart_preferred_ch, 0 /*leave non-core info alone*/));
#else
    IGNOREPARM(ch);

    return(status_nomem());
#endif
}

/*
helper routines for preferred state handling
*/

#define acu_stat(s) \
    { if(status_ok(status1)) status1 = (s); }

_Check_return_
static STATUS
gr_chart_clone(
    _InVal_     GR_CHART_HANDLE dst_ch,
    _InVal_     GR_CHART_HANDLE src_ch,
    _InVal_     S32 non_core_too)
{
    const P_GR_CHART dst_cp = p_gr_chart_from_chart_handle(dst_ch);
    S32 plotidx;
    GR_SERIES_IDX series_idx;
    GR_AXES_IDX axes_idx;
    P_GR_SERIES dst_serp;
    STATUS status1 = STATUS_OK;

    /* blow away current non-core contents */
    gr_chart_dispose_noncore(dst_cp);

    {
    PC_GR_CHART src_cp = p_gr_chart_from_chart_handle(src_ch); /* restrict scope */
    S32 n_alloc;

    if(non_core_too)
    {
        dst_cp->core.layout = src_cp->core.layout;
        dst_cp->core.editsave = src_cp->core.editsave;
    }

    /* replicate current non-core contents from source */
    assert(offsetof32(GR_CHART, core) == 0); /* else we'd need another memcpy of noncore info */
    memcpy32(PtrAddBytes(P_BYTE, dst_cp, sizeof32(dst_cp->core)),
             (PC_BYTE) src_cp + sizeof32(dst_cp->core),
             sizeof32(*dst_cp) - sizeof32(dst_cp->core));

    /* dup series descriptor, packing down */
    dst_cp->axes[0].series.end_idx -= dst_cp->axes[0].series.stt_idx; /* make relative */
    dst_cp->axes[0].series.stt_idx = 0;
 /* dst_cp->axes[0].series.end_idx += dst_cp->axes[0].series.stt_idx; !* make abs again */

    dst_cp->axes[1].series.end_idx -= dst_cp->axes[1].series.stt_idx; /* make relative */
    dst_cp->axes[1].series.stt_idx = dst_cp->axes[0].series.end_idx;
    dst_cp->axes[1].series.end_idx += dst_cp->axes[1].series.stt_idx; /* make abs again */

    /* copy over in two sections such that destination is packed */
    n_alloc = 0;

    for(axes_idx = 0; axes_idx <= GR_AXES_MAX; ++axes_idx)
    {
        dst_cp->axes[axes_idx].cache.n_series = dst_cp->axes[axes_idx].series.end_idx - dst_cp->axes[axes_idx].series.stt_idx;

        n_alloc += dst_cp->axes[axes_idx].cache.n_series;
    }

    dst_cp->series.mh = 0;

    if(n_alloc)
    {
        SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(*dst_serp), TRUE);

        if(NULL != (dst_serp = al_array_alloc(&dst_cp->series.mh, GR_SERIES, n_alloc, &array_init_block)))
            return(status_nomem());

        for(axes_idx = 0; axes_idx <= GR_AXES_MAX; ++axes_idx)
        {
            P_GR_SERIES src_serp;

            src_serp = array_ptr(&src_cp->series.mh, GR_SERIES, src_cp->axes[axes_idx].series.stt_idx);
            dst_serp = array_ptr(&dst_cp->series.mh, GR_SERIES, dst_cp->axes[axes_idx].series.stt_idx);

            memcpy32(dst_serp, src_serp, sizeof32(*dst_serp) * (dst_cp->axes[axes_idx].series.end_idx - dst_cp->axes[axes_idx].series.stt_idx));
        }
    }
    } /*block*/

    /* have to properly duplicate the lists */

    /* have to properly dup the picture refs */
    gr_fillstyleb_ref_add(&dst_cp->chart.areastyleb);

    for(plotidx = 0; plotidx < GR_CHART_N_PLOTAREAS; ++plotidx)
        gr_fillstyleb_ref_add(&dst_cp->plotarea.area[plotidx].areastyleb);

    gr_fillstyleb_ref_add(&dst_cp->legend.areastyleb);

    /* dup text and text styles from this chart */
    acu_stat(gr_chart_list_duplic(dst_cp, GR_LIST_CHART_TEXT));
    acu_stat(gr_chart_list_duplic(dst_cp, GR_LIST_CHART_TEXT_TEXTSTYLE));

    for(series_idx = 0; series_idx < array_elements(&dst_cp->series.mh); ++series_idx)
    {
        /* dup refs to series pictures */
        dst_serp = getserp(dst_cp, series_idx);

        gr_fillstyleb_ref_add(&dst_serp->style.pdrop_fillb);
        gr_fillstyleb_ref_add(&dst_serp->style.point_fillb);

        /* dup all point info from this series - implicit dup of refs to point pictures */
        acu_stat(gr_point_list_duplic(dst_cp, series_idx, GR_LIST_PDROP_FILLSTYLEB));
        acu_stat(gr_point_list_duplic(dst_cp, series_idx, GR_LIST_PDROP_FILLSTYLEC));
        acu_stat(gr_point_list_duplic(dst_cp, series_idx, GR_LIST_PDROP_LINESTYLE));

        acu_stat(gr_point_list_duplic(dst_cp, series_idx, GR_LIST_POINT_FILLSTYLEB));
        acu_stat(gr_point_list_duplic(dst_cp, series_idx, GR_LIST_POINT_FILLSTYLEC));
        acu_stat(gr_point_list_duplic(dst_cp, series_idx, GR_LIST_POINT_LINESTYLE));
        acu_stat(gr_point_list_duplic(dst_cp, series_idx, GR_LIST_POINT_TEXTSTYLE));

        acu_stat(gr_point_list_duplic(dst_cp, series_idx, GR_LIST_POINT_BARCHSTYLE));
        acu_stat(gr_point_list_duplic(dst_cp, series_idx, GR_LIST_POINT_BARLINECHSTYLE));
        acu_stat(gr_point_list_duplic(dst_cp, series_idx, GR_LIST_POINT_LINECHSTYLE));
        acu_stat(gr_point_list_duplic(dst_cp, series_idx, GR_LIST_POINT_PIECHDISPLSTYLE));
        acu_stat(gr_point_list_duplic(dst_cp, series_idx, GR_LIST_POINT_PIECHLABELSTYLE));
        acu_stat(gr_point_list_duplic(dst_cp, series_idx, GR_LIST_POINT_SCATCHSTYLE));
    }

    dst_cp->bits.realloc_series = 1;

    status_assert(status1);

    /* if anything failed blow the new copied non-core contents away */
    /* but only when we have reached a consistent state with no stolen list_blkrefs etc. */
    if(status_fail(status1))
        gr_chart_dispose_noncore(dst_cp);

    return(status1);
}

#undef acu_stat

static void
gr_chart_clone_noncore_pict_lose_refs(
    _InVal_     GR_CHART_HANDLE ch)
{
    const P_GR_CHART cp = p_gr_chart_from_chart_handle(ch);
    S32 plotidx;
    GR_SERIES_IDX series_idx;

    /* have to properly dup the picture refs when really used */
    gr_fillstyleb_ref_lose(&cp->chart.areastyleb);

    for(plotidx = 0; plotidx < GR_CHART_N_PLOTAREAS; ++plotidx)
        gr_fillstyleb_ref_lose(&cp->plotarea.area[plotidx].areastyleb);

    gr_fillstyleb_ref_lose(&cp->legend.areastyleb);

    for(series_idx = 0; series_idx < array_elements(&cp->series.mh); ++series_idx)
    {
        /* lose refs to series pictures */
        const P_GR_SERIES serp = getserp(cp, series_idx);

        gr_fillstyle_ref_lose(&serp->style.pdrop_fill);
        gr_fillstyle_ref_lose(&serp->style.point_fill);

        /* lose refs to point pictures */
        gr_pdrop_list_fillstyle_reref(cp, series_idx, 0 /*lose*/);
        gr_point_list_fillstyle_reref(cp, series_idx, 0 /*lose*/);
    }
}

#endif /* GR_CLONE */

/* end of gr_chart.c */
