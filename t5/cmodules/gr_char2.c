/* gr_char2.c */

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
internal routines
*/

static GR_DATASOURCE_NO
gr_series_n_datasources_req(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx);

#ifndef GR_SERIES_DESC_INCR
#define GR_SERIES_DESC_INCR 4
#endif

/* no GR_SERIES_DESC_DECR - series descriptor never shrinks */

#ifndef GR_DATASOURCES_DESC_INCR
#define GR_DATASOURCES_DESC_INCR 4
#endif

#ifndef GR_DATASOURCES_DESC_DECR
#define GR_DATASOURCES_DESC_DECR 5
#endif

/******************************************************************************
*
* reallocate the data sources we have now amongst the series for this chart
*
******************************************************************************/

_Check_return_
extern STATUS
gr_chart_add_series(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_AXES_IDX axes_idx,
    _InVal_     S32 init)
{
    /* add after this one */
    const GR_SERIES_IDX series_idx = cp->axes[axes_idx].series.end_idx;
    P_GR_SERIES serp;
    STATUS status;

    assert((axes_idx == 0) || (axes_idx == 1));

    /* descriptor doesn't already exist? */
    if(series_idx >= ((axes_idx == 0) ? cp->axes[1].series.stt_idx : array_elements(&cp->series.mh)))
    {
        SC_ARRAY_INIT_BLOCK array_init_block = aib_init(GR_SERIES_DESC_INCR, sizeof32(*serp), FALSE);

        if(NULL == al_array_extend_by(&cp->series.mh, GR_SERIES, 1, &array_init_block, &status))
            return(status);

        if(axes_idx == 0)
        {
            /* have to move existing defined descriptors from higher axes up (maybe 0) */
            serp = getserp(cp, cp->axes[1].series.stt_idx);

            memmove32(serp + 1 /* UP */, serp, sizeof32(*serp) * ((array_elements(&cp->series.mh) - 1) - cp->axes[1].series.stt_idx));

            /* overlay axes start higher up now (even during single axes set preparation) - move together */
            ++cp->axes[1].series.stt_idx;
            ++cp->axes[1].series.end_idx;
        }
        else
            serp = getserp(cp, array_elements(&cp->series.mh) - 1);

        /* zap this newly created descriptor */
        zero_struct_ptr(serp);
    }

    /* if not clearly specified the initialise this new element from a lower friend, if available, or defaults */
    serp = getserp(cp, series_idx);

    if(!serp->internal_bits.descriptor_ok && init) /* won't be set if brand new */
    {
        if(cp->axes[axes_idx].series.stt_idx == cp->axes[axes_idx].series.end_idx)
        {
            /* first addition to axes set */
            serp->sertype = cp->axes[axes_idx].sertype;
            serp->chart_type = cp->axes[axes_idx].chart_type;
        }
        else
        {
            /* clone some aspects of its friend */
            memcpy32(serp, serp-1, offsetof32(GR_SERIES, GR_SERIES_CLONE_END));

            zero_struct(serp->lbr);

            /* dup picture refs */
            gr_fillstyleb_ref_add(&serp->style.pdrop_fillb);
            gr_fillstyleb_ref_add(&serp->style.point_fillb);
        }
    }

    serp->datasources.n_req = gr_series_n_datasources_req(cp, series_idx);

    cp->axes[axes_idx].series.end_idx += 1;

    cp->axes[axes_idx].cache.n_series = cp->axes[axes_idx].series.end_idx - cp->axes[axes_idx].series.stt_idx;

    cp->series.n_in_use = cp->axes[0].cache.n_series + cp->axes[1].cache.n_series;

    return(1);
}

/******************************************************************************
*
* dispose of info owned by this series
*
******************************************************************************/

extern void
gr_chart_free_series(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx)
{
    /* delete series pictures */
    const P_GR_SERIES serp = getserp(cp, series_idx);

    gr_fillstyleb_pict_delete(&serp->style.pdrop_fillb);
    gr_fillstyleb_pict_delete(&serp->style.point_fillb);

    /* delete all point info from this series */
    gr_point_list_delete(cp, series_idx, GR_LIST_PDROP_FILLSTYLEB);
    gr_point_list_delete(cp, series_idx, GR_LIST_PDROP_FILLSTYLEC);
    gr_point_list_delete(cp, series_idx, GR_LIST_PDROP_LINESTYLE);

    gr_point_list_delete(cp, series_idx, GR_LIST_POINT_FILLSTYLEB);
    gr_point_list_delete(cp, series_idx, GR_LIST_POINT_FILLSTYLEC);
    gr_point_list_delete(cp, series_idx, GR_LIST_POINT_LINESTYLE);
    gr_point_list_delete(cp, series_idx, GR_LIST_POINT_TEXTSTYLE);

    gr_point_list_delete(cp, series_idx, GR_LIST_POINT_BARCHSTYLE);
    gr_point_list_delete(cp, series_idx, GR_LIST_POINT_BARLINECHSTYLE);
    gr_point_list_delete(cp, series_idx, GR_LIST_POINT_LINECHSTYLE);
    gr_point_list_delete(cp, series_idx, GR_LIST_POINT_PIECHDISPLSTYLE);
    gr_point_list_delete(cp, series_idx, GR_LIST_POINT_PIECHLABELSTYLE);
    gr_point_list_delete(cp, series_idx, GR_LIST_POINT_SCATCHSTYLE);
}

/******************************************************************************
*
* insert a data source in a chart AFTER a given entry
*
******************************************************************************/

extern GR_DATASOURCE_HANDLE
gr_chart_datasource_insert(
    _ChartRef_  P_GR_CHART cp,
    _InRef_     P_PROC_GR_CHART_TRAVEL ext_proc,
    P_ANY ext_handle,
    P_GR_INT_HANDLE p_int_handle_out,
    _InVal_     GR_INT_HANDLE int_handle_after)
{
    P_GR_DATASOURCE i_p_gr_datasource;
    P_GR_DATASOURCE p_gr_datasource;
    GR_DATASOURCE_NO before_ds;
    GR_CHART_OBJID id;
    STATUS status;

    /*
    an internal handle for uniquely binding data sources to series
    */
    static GR_DATASOURCE_HANDLE gr_datasource_handle_gen = (GR_DATASOURCE_HANDLE) 0x02233220;

    *p_int_handle_out = GR_DATASOURCE_HANDLE_NONE;

    if(int_handle_after == GR_DATASOURCE_HANDLE_START)
    {
        /* replace the category data source */
        p_gr_datasource = &cp->core.category_datasource;
        id = gr_chart_objid_chart;
    }
    else if(int_handle_after == GR_DATASOURCE_HANDLE_TEXTS)
    {
        /* create new text object */
        LIST_ITEMNO key = gr_text_key_for_new(cp);
        P_GR_TEXT t;
        P_GR_TEXT_GUTS gutsp;

        if(status_fail(status = gr_text_new(cp, key, NULL, NULL)))
            return(GR_DATASOURCE_HANDLE_NONE);

        t = gr_text_search_key(cp, key);
        PTR_ASSERT(t);

        t->bits.live_text = 1;

        gutsp.p_gr_text = t + 1;

        p_gr_datasource = gutsp.p_gr_datasource;

        gr_chart_objid_from_text(key, &id);
    }
    else
    {
        SC_ARRAY_INIT_BLOCK array_init_block = aib_init(GR_DATASOURCES_DESC_INCR, sizeof32(*p_gr_datasource), FALSE);

        if(int_handle_after == GR_DATASOURCE_HANDLE_NONE)
            /* insert AT the end */
            before_ds = array_elements(&cp->core.datasources.mh);
        else if(int_handle_after == cp->core.category_datasource.dsh)
            /* if inserting after (non null) category data source then insert BEFORE first real data source */
            before_ds = 0;
        else
        {
            /* find the entry to insert BEFORE */
            if((p_gr_datasource = gr_chart_datasource_p_from_h(cp, int_handle_after)) == NULL)
                return(GR_DATASOURCE_HANDLE_NONE);

            i_p_gr_datasource = array_base(&cp->core.datasources.mh, GR_DATASOURCE);
            before_ds = PtrDiffElemU32(p_gr_datasource, i_p_gr_datasource);
            before_ds += 1;
        }

        if(NULL == al_array_extend_by(&cp->core.datasources.mh, GR_DATASOURCE, 1, &array_init_block, &status))
        {
            status_assert(status);
            return(GR_DATASOURCE_HANDLE_NONE);
        }

        /* remember we are now adding BEFORE the given element */
        p_gr_datasource = array_ptr(&cp->core.datasources.mh, GR_DATASOURCE, before_ds);

        /* move rest of descriptors up to make way */
        memmove32(p_gr_datasource + 1 /* UP */,
                  p_gr_datasource,
                  PtrDiffBytesU32(array_ptr(&cp->core.datasources.mh, GR_DATASOURCE, array_elements(&cp->core.datasources.mh)), (p_gr_datasource + 1)));

        cp->bits.realloc_series = 1;

        /* SKS after PD 4.12 30mar92 - not yet allocated to series */
        id = gr_chart_objid_anon;
    }

    /* no prior inheritance; clear out descriptor */
    zero_struct_ptr(p_gr_datasource);

    /* invent a unique non-repeating handle for this datasource */
    ENUM_INCR(GR_DATASOURCE_HANDLE, gr_datasource_handle_gen);

    p_gr_datasource->dsh = gr_datasource_handle_gen;
    p_gr_datasource->ext_proc = ext_proc;
    p_gr_datasource->ext_handle = ext_handle;
    p_gr_datasource->id = id;

    *p_int_handle_out = p_gr_datasource->dsh;

    trace_4(TRACE_MODULE_GR_CHART, TEXT("gr_datasource_insert(") PTR_XTFMT TEXT(", (%s,") PTR_XTFMT TEXT("), ") ENUM_XTFMT, cp, report_procedure_name(report_proc_cast(ext_proc)), ext_handle, p_gr_datasource->dsh);
    return(p_gr_datasource->dsh);
}

/******************************************************************************
*
* find a data source descriptor from its handle
* this could do with being fast so make it a list?
*
******************************************************************************/

extern P_GR_DATASOURCE
gr_chart_datasource_p_from_h(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_DATASOURCE_HANDLE dsh)
{
    const ARRAY_INDEX n_datasources = array_elements(&cp->core.datasources.mh);
    P_GR_DATASOURCE base_p_gr_datasource = array_range(&cp->core.datasources.mh, GR_DATASOURCE, 0, n_datasources);
    P_GR_DATASOURCE p_gr_datasource = base_p_gr_datasource;
    ARRAY_INDEX i;
    P_LIST_BLOCK p_list_block;
    P_GR_TEXT t;
    LIST_ITEMNO key;

    for(i = 0; i < n_datasources; ++i, ++p_gr_datasource)
    {
        if(p_gr_datasource->dsh == dsh)
            return(p_gr_datasource);
    }

    p_list_block = &cp->text.lbr;

    for(t = collect_first(GR_TEXT, p_list_block, &key);
        t;
        t = collect_next(GR_TEXT, p_list_block, &key))
    {
        P_GR_TEXT_GUTS gutsp;

        if(!t->bits.live_text)
            continue;

        gutsp.p_gr_text = t + 1;

        p_gr_datasource = gutsp.p_gr_datasource;

        if(p_gr_datasource->dsh == dsh)
            return(p_gr_datasource);
    }

    myassert0(TEXT("No datasource found"));
    return(NULL);
}

/******************************************************************************
*
* remove a data source from a chart
*
******************************************************************************/

_Check_return_
extern STATUS
gr_chart_datasource_subtract_using_dsh(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_DATASOURCE_HANDLE dsh)
{
    P_GR_DATASOURCE p_gr_datasource;

    if(dsh == GR_DATASOURCE_HANDLE_NONE)
    {
        assert(dsh != GR_DATASOURCE_HANDLE_NONE);
        return(0);
    }

    if(cp->core.category_datasource.dsh == dsh)
    {
        cp->core.category_datasource.dsh = GR_DATASOURCE_HANDLE_NONE;
        return(1);
    }

    if((p_gr_datasource = gr_chart_datasource_p_from_h(cp, dsh)) == NULL)
        return(0);

    switch(p_gr_datasource->id.name)
    {
    default: default_unhandled();
#if CHECKING
    case GR_CHART_OBJNAME_ANON: /* unused datasource */
#endif
        break;

    case GR_CHART_OBJNAME_TEXT:
        /* prevent recall during deletion */
        p_gr_datasource->dsh = GR_DATASOURCE_HANDLE_NONE;
        gr_text_delete(cp, p_gr_datasource->id.no);
        break;

    case GR_CHART_OBJNAME_SERIES:
        {
        GR_CHART_OBJID id;
        GR_SERIES_IDX series_idx;
        P_GR_SERIES serp;
        GR_DATASOURCE_NO ds;
        ARRAY_INDEX i;

        p_gr_datasource->dsh = GR_DATASOURCE_HANDLE_NONE;

        /* kill client dep */
        status_consume(gr_travel_dsp_null(p_gr_datasource, 0));

        cp->bits.realloc_series = 1;

        id = p_gr_datasource->id; /* rembember what series and ds we were used by */

        /* compact down datasource descriptor array */
        i = array_indexof_element(&cp->core.datasources.mh, GR_DATASOURCE, p_gr_datasource);
        al_array_delete_at(&cp->core.datasources.mh, -1, i);

        /* remove from use in a series */
        series_idx = gr_series_idx_from_external(cp, id.no);
        serp = getserp(cp, series_idx);

        * (int *) &serp->valid = 0;

        assert(serp->datasources.dsh[id.subno] == dsh);
        serp->datasources.dsh[id.subno] = GR_DATASOURCE_HANDLE_NONE;

        /* loop over datasources in this series and see if it needs removing */
        for(ds = 0; ds < GR_SERIES_MAX_DATASOURCES; ++ds)
            if(serp->datasources.dsh[ds] != GR_DATASOURCE_HANDLE_NONE)
                break;

        if(ds == GR_SERIES_MAX_DATASOURCES)
        {
            GR_AXES_IDX axes_idx;

            /* SKS 02oct95 - why oh why oh why! */
            gr_chart_free_series(cp, series_idx);

            al_array_delete_at(&cp->series.mh, -1, series_idx);

            if(series_idx < cp->axes[0].series.end_idx)
            {
                cp->axes[0].series.end_idx -= 1;
                cp->axes[1].series.stt_idx -= 1;
            }

            cp->axes[1].series.end_idx -= 1;

            /* reassign ownership of ALL series on ALL axes */
            for(axes_idx = 0; axes_idx <= cp->axes_idx_max; ++axes_idx)
            {
                for(series_idx = cp->axes[axes_idx].series.stt_idx; series_idx < cp->axes[axes_idx].series.end_idx; ++series_idx)
                {
                    serp = getserp(cp, series_idx);

                    id.no = UBF_PACK(gr_series_external_from_idx(cp, series_idx));

                    for(ds = 0; ds < serp->datasources.n_req; ++ds)
                    {
                        if(serp->datasources.dsh[ds] != GR_DATASOURCE_HANDLE_NONE)
                        {
                            p_gr_datasource = gr_chart_datasource_p_from_h(cp, serp->datasources.dsh[ds]);
                            PTR_ASSERT(p_gr_datasource);
                            id.subno = UBF_PACK(ds);
                            p_gr_datasource->id = id;
                        }
                    }
                }
            }
        }

        break;
        }
    }

    return(1);
}

/******************************************************************************
*
* how many data sources does a series of this type require?
*
******************************************************************************/

static GR_DATASOURCE_NO
gr_series_n_datasources_req(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx)
{
    const P_GR_SERIES serp = getserp(cp, series_idx);
    GR_DATASOURCE_NO n_req;

    switch(serp->sertype)
    {
    default:
#if CHECKING
        myassert1(TEXT("gr_series_datasources.n_req(type = ") S32_TFMT TEXT(" unknown)"), serp->sertype);

        /*FALLTHRU*/

    case GR_CHART_SERIES_PLAIN:
#endif
        n_req = 1;
        break;

    case GR_CHART_SERIES_POINT:
    case GR_CHART_SERIES_PLAIN_ERROR1:
        n_req = 2;
        break;

    case GR_CHART_SERIES_POINT_ERROR1:
    case GR_CHART_SERIES_PLAIN_ERROR2:
    /*case GR_CHART_SERIES_HILOCLOSE:*/
        n_req = 3;
        break;

    case GR_CHART_SERIES_POINT_ERROR2:
    /*case GR_CHART_SERIES_HILOOPENCLOSE:*/
        n_req = 4;
        break;
    }

    return(n_req);
}

#if CHECKING

_Check_return_
_Ret_z_
extern PCTSTR
gr_chart_object_name_from_id_quick(
    _InVal_     GR_CHART_OBJID id)
{
    static UCHARZ ustr_buf[BUF_MAX_GR_CHART_OBJID_REPR];

    gr_chart_object_name_from_id(NULL, id, ustr_bptr(ustr_buf), elemof32(ustr_buf));

    return(_tstr_from_ustr(ustr_bptr(ustr_buf)));
}

#endif

/******************************************************************************
*
* find the 'parent' of an object for selection porpoises
*
******************************************************************************/

_Check_return_
extern BOOL
gr_chart_objid_find_parent(
    _InoutRef_  P_GR_CHART_OBJID p_id,
    _InVal_     BOOL deepest)
{
    GR_CHART_OBJID_NAME new_name;

    switch(p_id->name)
    {
    case GR_CHART_OBJNAME_POINT:
        if(deepest)
            return(FALSE);

        /*FALLTHRU*/

    case GR_CHART_OBJNAME_LEGDSERIES:
    case GR_CHART_OBJNAME_LEGDPOINT:
        new_name = GR_CHART_OBJNAME_SERIES;
        break;

    case GR_CHART_OBJNAME_DROPPOINT:
        if(deepest)
            return(FALSE);

        new_name = GR_CHART_OBJNAME_DROPSERIES;
        break;

    /* note that AXISGRID and AXISTICK aren't considered as AXIS is easy to hit */

    default:
        return(FALSE);
    }

    p_id->name = new_name;
    p_id->has_subno = 0;
    p_id->subno = 0; /* no longer just for debugging clarity, needed for diagram searches too */
    return(TRUE);
}

_Check_return_
extern BOOL
gr_chart_objid_find_parent_for_search(
    _InoutRef_  P_GR_CHART_OBJID p_id)
{
    switch(p_id->name)
    {
    case GR_CHART_OBJNAME_LEGDSERIES:
    case GR_CHART_OBJNAME_LEGDPOINT:
        return(FALSE);

    default:
        return(gr_chart_objid_find_parent(p_id, FALSE));
    }
}

_Check_return_
extern STATUS
gr_chart_realloc_series(
    _ChartRef_  P_GR_CHART cp)
{
    GR_CHART_OBJID id;
    GR_SERIES_IDX series_idx;
    P_GR_SERIES serp;
    GR_DATASOURCE_NO ds;
    GR_AXES_IDX axes_idx;
    GR_SERIES_IDX old_n_series_0, old_n_series_1;
    ARRAY_INDEX n_datasources;
    P_GR_DATASOURCE p_gr_datasource, last_p_gr_datasource;

    cp->bits.realloc_series = 0;

    gr_chart_objid_from_series_no(1, &id);

    /* clear out data source allocation of all axes but preserve series attributes
     * and as we'll be reshuffling data sources between series destroy series caches
    */
    for(series_idx = 0; series_idx < array_elements(&cp->series.mh); ++series_idx)
    {
        serp = getserp(cp, series_idx);

        * (int *) &serp->valid = 0;

        serp->datasources.n = 0;

        for(ds = 0; ds < GR_SERIES_MAX_DATASOURCES; ++ds)
            serp->datasources.dsh[ds] = GR_DATASOURCE_HANDLE_NONE;
    }

    /* ensure first axes set always start at zero! */
    assert(cp->axes[0].series.stt_idx == 0);

    /* remember where we had got to (relatively speaking) */
    old_n_series_0 = cp->axes[0].series.end_idx;
    old_n_series_1 = cp->axes[1].series.end_idx - cp->axes[1].series.stt_idx;

    /* 'remove' series from axes sets */
    for(axes_idx = 0; axes_idx <= GR_AXES_IDX_MAX; ++axes_idx)
    {
        cp->axes[axes_idx].series.end_idx = cp->axes[axes_idx].series.stt_idx;
        cp->axes[axes_idx].cache.n_series = 0;
    }

    cp->series.n_in_use = 0;

    /* loop over existing series on main axis set refilling
     * them from the data source array or padding with NONE
    */
    series_idx = 0;

    n_datasources = array_elements(&cp->core.datasources.mh);
    p_gr_datasource = array_range(&cp->core.datasources.mh, GR_DATASOURCE, 0, n_datasources);
    last_p_gr_datasource = p_gr_datasource + n_datasources;

    /* SKS after PD 4.12 25mar92 - must allocate at least one series even if this would be completely unfilled */
    do
    {
        /* whether to bomb the new descriptor or leave as is: vvv */
        status_return(gr_chart_add_series(cp, 0, (series_idx >= old_n_series_0)));

        serp = getserp(cp, series_idx);

        /* pour as many datasources in as it will take */
        do  {
            GR_DATASOURCE_HANDLE dsh;

            if(p_gr_datasource == last_p_gr_datasource)
                dsh = GR_DATASOURCE_HANDLE_NONE;
            else
            {
                id.no = UBF_PACK(gr_series_external_from_idx(cp, series_idx));
                id.subno = UBF_PACK(serp->datasources.n);

                dsh = p_gr_datasource->dsh;
                p_gr_datasource->id = id;
                p_gr_datasource++;
            }

            assert(serp->datasources.n < GR_SERIES_MAX_DATASOURCES);
            serp->datasources.dsh[serp->datasources.n++] = dsh;
        }
        while(serp->datasources.n < serp->datasources.n_req);

        ++series_idx;
    }
    while(p_gr_datasource != last_p_gr_datasource);

    /* if only one axes set then all series are on that */
    if(cp->axes_idx_max != 0)
    {
        /* loop stripping data sources and therefore series from main axes till a respectable balance is achieved */
        while((cp->axes[0].cache.n_series - cp->axes[1].cache.n_series) > 1)
        {
            /* find first free descriptor off end of overlay axes */
            GR_SERIES_IDX new_series_idx = cp->axes[1].series.end_idx;
            GR_SERIES_IDX n_series_strip = 0;

            /* concoct new series for overlay axes */
            status_return(gr_chart_add_series(cp, 1, ((new_series_idx - cp->axes[1].series.end_idx) >= old_n_series_1)));

            /* count number of series we'd have to fully move over from main axes set
             * to satisfy this one new series on the overlay axes
             * note that we never have partially filled series on the overlay axes at this stage
             * all partially filled series are kept on the main axes
            */
            series_idx = cp->axes[0].series.end_idx;
            ds = 0;
            while(--series_idx >= cp->axes[0].series.stt_idx)
            {
                serp = getserp(cp, series_idx);

                ds += serp->datasources.n;

                if(ds <= serp->datasources.n_req)
                    ++n_series_strip;

                if(ds >= serp->datasources.n_req)
                    break;
            }

            serp = getserp(cp, new_series_idx);

            /* if couldn't fully satisfy request from series on main axes, stop */
            /* if number moved from main axes would swing the balance to the overlay side, stop */
            if( (ds < serp->datasources.n_req)                                               ||
                (cp->axes[1].cache.n_series > (cp->axes[0].cache.n_series - n_series_strip)) )
            {
                /* apologies go to the allocator - remove this descriptor and stop */
                cp->axes[1].series.end_idx -= 1;
                cp->axes[1].cache.n_series -= 1;
                cp->series.n_in_use -= 1;
                break;
            }
            else
            {
                /* strip from some ds point in this series in the main axes set */
                GR_SERIES_IDX stop_at_series_idx = cp->axes[0].series.end_idx - n_series_strip;
                /* loop moving data sources upstream through the overlay descriptors! */
                GR_SERIES_IDX src_series_idx = cp->axes[1].series.end_idx - 1;
                GR_SERIES_IDX dst_series_idx = cp->axes[1].series.end_idx;
                GR_DATASOURCE_NO src_ds = 0;
                GR_DATASOURCE_NO dst_ds = 0;
                P_GR_SERIES src_serp;
                P_GR_SERIES dst_serp;

                p_gr_datasource = last_p_gr_datasource;

                for(;;)
                {
                    /* step back one ds; when run out, step back one descriptor,
                     * possibly jumping over the mid-axis gap
                    */
                    if(dst_ds == 0)
                    {
                        if(dst_series_idx == cp->axes[1].series.stt_idx)
                            dst_series_idx = cp->axes[0].series.end_idx;

                        --dst_series_idx;

                        dst_serp = getserp(cp, dst_series_idx);
                        dst_ds = dst_serp->datasources.n_req;
                    }

                    --dst_ds;

                    if(src_ds == 0)
                    {
                        if(src_series_idx == cp->axes[1].series.stt_idx)
                            src_series_idx = cp->axes[0].series.end_idx;

                        --src_series_idx;

                        src_serp = getserp(cp, src_series_idx);
                        src_ds = src_serp->datasources.n_req;
                    }

                    --src_ds;

                    /* move over single datasource per loop */
                    dst_serp = getserp(cp, dst_series_idx);
                    src_serp = getserp(cp, src_series_idx);
                    dst_serp->datasources.dsh[dst_ds] = src_serp->datasources.dsh[src_ds];
                    dst_serp->datasources.n++;
                    src_serp->datasources.dsh[src_ds] = GR_DATASOURCE_HANDLE_NONE;
                    src_serp->datasources.n--;

                    if(dst_ds)
                        /* keep looping if dst not satisfied, esp. within last strip series */
                        continue;

                    if(src_series_idx == stop_at_series_idx)
                        break;
                }
            }

            /* possibly several series removed from the main axes set */
            cp->axes[0].series.end_idx -= n_series_strip;
            cp->axes[0].cache.n_series -= n_series_strip;
            cp->series.n_in_use        -= n_series_strip;

            /* one more series added to the overlay axes set by allocator */
        }
    }

    /* SKS after PD 4.12 26mar92 - added overlay reloading helper stuff */
    cp->axes[0].series.start_series = 0;
    if(cp->axes_idx_max != 0)
    {
        if(cp->axes[1].series.start_series >= 0) /* auto allocation */
        {
            if(cp->axes[1].cache.n_series != 0)
                cp->axes[1].series.start_series = gr_series_external_from_idx(cp, cp->axes[1].series.stt_idx);
            else
                cp->axes[1].series.start_series = 0;
        }
    }
    else
        cp->axes[1].series.start_series = 0;

#if 0 /* SKS after PD 4.12 26mar92 - removed, this should only be set on loading and cloning */
    /* all descriptors on main axes set are now ok
    */
    for(series_idx = cp->axes[0].series.stt_idx; series_idx < cp->axes[0].series.end_idx; ++series_idx)
    {
        serp = getserp(cp, series_idx);

        serp->internal_bits.descriptor_ok = 1;
    }
#endif

    /* can now go back over the datasources and reassign series ownership:
     * note that all series on main axes will be correctly assigned still
    */
    for(series_idx = cp->axes[1].series.stt_idx; series_idx < cp->axes[1].series.end_idx; ++series_idx)
    {
        serp = getserp(cp, series_idx);

#if 0 /* SKS after PD 4.12 26mar92 - removed, this should only be set on loading and cloning */
        serp->internal_bits.descriptor_ok = 1;
#endif

        id.no = UBF_PACK(gr_series_external_from_idx(cp, series_idx));

        for(ds = 0; ds < serp->datasources.n_req; ++ds)
        {
            if(serp->datasources.dsh[ds] != GR_DATASOURCE_HANDLE_NONE)
            {
                p_gr_datasource = gr_chart_datasource_p_from_h(cp, serp->datasources.dsh[ds]);
                PTR_ASSERT(p_gr_datasource);
                id.subno = UBF_PACK(ds);
                p_gr_datasource->id = id;
            }
        }
    }

    return(1);
}

/******************************************************************************
*
* pay a visit to the owner of the data we
* are using in this chart and
* ask him what the category label is, inventing one if he can't
*
******************************************************************************/

extern void
gr_travel_categ_label(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_CHART_ITEMNO item,
    _OutRef_    P_GR_CHART_VALUE pValue)
{
    gr_travel_dsh_label(cp, cp->core.category_datasource.dsh, item, pValue);

    if(pValue->type != GR_CHART_VALUE_TEXT)
    {
        /* invent a category label, based on item number */
        pValue->type = GR_CHART_VALUE_TEXT;
        consume_int(xsnprintf(pValue->data.text, sizeof32(pValue->data.text), U32_FMT, item + 1));
    }
}

/******************************************************************************
*
* pay a visit to the owner of the data we
* are using in this datasource and
* ask him about it.
*
******************************************************************************/

_Check_return_
extern STATUS
gr_travel_dsh(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_DATASOURCE_HANDLE dsh,
    _InVal_     GR_CHART_ITEMNO item,
    _InoutRef_  P_GR_CHART_VALUE pValue /* req_type must be set */)
{
    P_GR_DATASOURCE p_gr_datasource;
    STATUS status;

    pValue->type = GR_CHART_VALUE_NONE;

    if(dsh == GR_DATASOURCE_HANDLE_NONE)
        return(0);

    if((p_gr_datasource = &cp->core.category_datasource)->dsh != dsh)
        if((p_gr_datasource = gr_chart_datasource_p_from_h(cp, dsh)) == NULL)
            return(STATUS_FAIL);

    myassert3x(p_gr_datasource->ext_proc != NULL, TEXT("gr_travel about to call NULL travel proc: chart ") PTR_XTFMT TEXT(" dsh ") UINTPTR_XTFMT TEXT(" item ") S32_TFMT, cp, (uintptr_t) dsh, item);

    if(status_fail(status = (* p_gr_datasource->ext_proc) (p_gr_datasource->ext_handle, item, pValue)))
        pValue->type = GR_CHART_VALUE_NONE;

    return(status);
}

/******************************************************************************
*
* pay a visit to the owner of the data we
* are using in this datasource and
* ask him what the label is
*
******************************************************************************/

extern void
gr_travel_dsh_label(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_DATASOURCE_HANDLE dsh,
    _InVal_     GR_CHART_ITEMNO item,
    _OutRef_    P_GR_CHART_VALUE pValue)
{
    pValue->req_type = GR_CHART_VALUE_REQ_TEXT;

    if(status_fail(gr_travel_dsh(cp, dsh, item, pValue)))
        return;

    switch(pValue->type)
    {
    case GR_CHART_VALUE_NONE:
        break;

    case GR_CHART_VALUE_TEXT:
        /* got label */
        break;

    case GR_CHART_VALUE_NUMBER:
        {
        NUMFORM_PARMS numform_parms;
        EV_DATA ev_data;
        QUICK_UBLOCK quick_ublock;
        quick_ublock_setup(&quick_ublock, pValue->data.text);

        pValue->type = GR_CHART_VALUE_TEXT;
        pValue->data.text[0] = CH_NULL;

        ev_data_set_real(&ev_data, pValue->data.number);

        zero_struct(numform_parms);
        if(fabs(ev_data.arg.fp) >= S32_MAX)
            numform_parms.ustr_numform_numeric = USTR_TEXT("0.00e+00");
        else if(fabs(ev_data.arg.fp) < 1.0)
            numform_parms.ustr_numform_numeric = USTR_TEXT("0.##");
        else
            numform_parms.ustr_numform_numeric =
                (floor(ev_data.arg.fp) == ev_data.arg.fp)
                    ? USTR_TEXT("#,##0")
                    : USTR_TEXT("#,##0.00");

        status_assert(numform(&quick_ublock, P_QUICK_TBLOCK_NONE, &ev_data, &numform_parms));

        quick_ublock_dispose_leaving_buffer_valid(&quick_ublock);

        break;
        }

    default:
        pValue->type = GR_CHART_VALUE_NONE;
        break;
    }
}

/******************************************************************************
*
* pay a visit to the owner of the data we
* are using in this datasource and
* ask him how many items there are in it.
*
******************************************************************************/

extern GR_CHART_ITEMNO
gr_travel_dsh_n_items(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_DATASOURCE_HANDLE dsh)
{
    P_GR_DATASOURCE p_gr_datasource;

    if(dsh == GR_DATASOURCE_HANDLE_NONE)
        return(0);

    if((p_gr_datasource = &cp->core.category_datasource)->dsh != dsh)
        if((p_gr_datasource = gr_chart_datasource_p_from_h(cp, dsh)) == NULL)
                return(STATUS_FAIL);

    if(!p_gr_datasource->valid.n_items)
    {
        GR_CHART_VALUE value;

        value.req_type = GR_CHART_VALUE_REQ_NONE;

        status_consume(gr_travel_dsp(p_gr_datasource, GR_CHART_ITEMNO_N_ITEMS, &value));

        if(value.type != GR_CHART_VALUE_N_ITEMS)
            return(0);

        p_gr_datasource->cache.n_items = value.data.n_items;
        p_gr_datasource->valid.n_items = 1;
    }

    return(p_gr_datasource->cache.n_items);
}

/******************************************************************************
*
* pay a visit to the owner of the data we
* are using in this datasource and
* ask him about its value, returning a number
* (zero if he returns a non-number).
*
******************************************************************************/

_Check_return_
extern STATUS
gr_travel_dsh_valof(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_DATASOURCE_HANDLE dsh,
    _InVal_     GR_CHART_ITEMNO item,
    _OutRef_    P_F64 evalue)
{
    GR_CHART_VALUE value;

    *evalue = 0.0;

    value.req_type = GR_CHART_VALUE_NUMBER;

    status_return(gr_travel_dsh(cp, dsh, item, &value));

    if(value.type == GR_CHART_VALUE_NUMBER)
    {
        *evalue = value.data.number;

        if(!isfinite(*evalue))/* temp hack to handle inf/NaN */
            return(STATUS_OK);

        return(STATUS_DONE);
    }

    return(STATUS_OK);
}

_Check_return_
extern STATUS
gr_travel_dsp(
    P_GR_DATASOURCE p_gr_datasource,
    _InVal_     GR_CHART_ITEMNO item,
    _InoutRef_  P_GR_CHART_VALUE pValue /* req_type must be set */)
{
    STATUS status;

    pValue->type = GR_CHART_VALUE_NONE;

    PTR_ASSERT(p_gr_datasource);
    myassert2x(p_gr_datasource->ext_proc != NULL, TEXT("gr_travel about to call NULL travel proc: p_gr_datasource ") PTR_XTFMT TEXT(" item ") S32_TFMT, p_gr_datasource, item);

    if(status_fail(status = (* p_gr_datasource->ext_proc) (p_gr_datasource->ext_handle, item, pValue)))
        pValue->type = GR_CHART_VALUE_NONE;

    return(status);
}

_Check_return_
extern STATUS
gr_travel_dsp_null(
    P_GR_DATASOURCE p_gr_datasource,
    _InVal_     GR_CHART_ITEMNO item)
{
    PTR_ASSERT(p_gr_datasource);
    myassert2x(p_gr_datasource->ext_proc != NULL, TEXT("gr_travel about to call NULL travel proc: p_gr_datasource ") PTR_XTFMT TEXT(" item ") S32_TFMT, p_gr_datasource, item);

    /* pValue == NULL special value to tell client we no longer want to access this item */
    return((* p_gr_datasource->ext_proc) (p_gr_datasource->ext_handle, item, NULL));
}

/******************************************************************************
*
* obtain the datasource handle for a ds in the given series
* unused series have DATASOURCE_NONE set anyway
*
******************************************************************************/

_Check_return_
extern GR_DATASOURCE_HANDLE
gr_travel_series_dsh_from_ds(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_DATASOURCE_NO ds)
{
    P_GR_SERIES serp;

    assert(array_index_is_valid(&cp->series.mh, series_idx));

    serp = getserp(cp, series_idx);

    return(serp->datasources.dsh[ds]);
}

/******************************************************************************
*
* pay a visit to the owner of the data we
* are using in this chart/axes/series and
* ask him what the label is, inventing one if he can't
*
******************************************************************************/

extern void
gr_travel_series_label(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _OutRef_    P_GR_CHART_VALUE pValue)
{
    P_GR_SERIES serp;
    const GR_DATASOURCE_NO ds = 0; /* always get series label from ds 0 in a series */

    assert(array_index_is_valid(&cp->series.mh, series_idx));

    serp = getserp(cp, series_idx);

    gr_travel_dsh_label(cp, serp->datasources.dsh[ds], GR_CHART_ITEMNO_LABEL, pValue);

    if(pValue->type != GR_CHART_VALUE_TEXT)
    {
        /* invent a label, based on series number */
        pValue->type = GR_CHART_VALUE_TEXT;
        consume_int(xsnprintf(pValue->data.text, sizeof32(pValue->data.text), "S" U32_FMT, gr_series_external_from_idx(cp, series_idx)));
    }
}

/******************************************************************************
*
* pay a visit to the owner of the data we
* are using in this chart/axes/series/ds and
* ask him how many items there are in it.
*
******************************************************************************/

_Check_return_
extern GR_CHART_ITEMNO
gr_travel_series_n_items(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx,
    _InVal_     GR_DATASOURCE_NO ds)
{
    P_GR_SERIES serp;

    assert(array_index_is_valid(&cp->series.mh, series_idx));

    serp = getserp(cp, series_idx);

    return(gr_travel_dsh_n_items(cp, serp->datasources.dsh[ds]));
}

/* scan all ds belonging to this series */

_Check_return_
extern GR_CHART_ITEMNO
gr_travel_series_n_items_total(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     GR_SERIES_IDX series_idx)
{
    P_GR_SERIES serp;
    GR_DATASOURCE_NO ds;
    GR_CHART_ITEMNO curnum, maxnum;

    assert(array_index_is_valid(&cp->series.mh, series_idx));

    serp = getserp(cp, series_idx);

    if(serp->valid.n_items_total)
        return(serp->cache.n_items_total);

    maxnum = 0;

    for(ds = 0; ds < serp->datasources.n; ++ds)
    {
        curnum = gr_travel_dsh_n_items(cp, serp->datasources.dsh[ds]);
        if(maxnum < curnum)
            maxnum = curnum;
    }

    serp->cache.n_items_total = maxnum;
    serp->valid.n_items_total = 1;

    return(maxnum);
}

/* end of gr_chart.c */
