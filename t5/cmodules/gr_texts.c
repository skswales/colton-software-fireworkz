/* gr_texts.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* SKS June 1993 split here */

#include "common/gflags.h"

#include "ob_chart/ob_chart.h"

#include "cmodules/collect.h"

/******************************************************************************
*
* add in any text objects as a grouped layer in the chart
*
******************************************************************************/

_Check_return_
extern STATUS
gr_text_addin(
    _ChartRef_  P_GR_CHART cp)
{
    struct GR_CHART_TEXT * text = &cp->text;
    GR_DIAG_OFFSET textsStart;
    P_GR_TEXT t;
    LIST_ITEMNO key;
    STATUS status = STATUS_OK;

    status_return(gr_chart_group_new(cp, &textsStart, gr_chart_objid_anon));

    for(t = collect_first(GR_TEXT, &text->lbr, &key);
        t;
        t = collect_next(GR_TEXT, &text->lbr, &key))
    {
        GR_CHART_OBJID id;
        GR_TEXTSTYLE textstyle;
        P_GR_TEXT_GUTS gutsp;
        GR_CHART_VALUE value;
        PC_USTR ustr;

        /* ignore ones we aren't using */
        if(t->bits.unused)
            continue;

        gr_chart_objid_from_text(key, &id);

        gr_chart_objid_textstyle_query(cp, id, &textstyle);

        gutsp.p_gr_text = t + 1;

        if(t->bits.live_text)
        {
            value.req_type = GR_CHART_VALUE_REQ_NONE;

            status_consume(gr_travel_dsp(gutsp.p_gr_datasource, 0, &value));

            ustr = ustr_bptr(value.data.text);
        }
        else
            ustr = gutsp.ustr;

        status_break(status = gr_chart_text_new(cp, id, &t->box, ustr, &textstyle));
    }

    gr_chart_group_end(cp, &textsStart);

    return(status);
}

/******************************************************************************
*
* query/set the position and size of a text object
*
******************************************************************************/

extern void
gr_text_box_query(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     LIST_ITEMNO key,
    _OutRef_    P_GR_BOX p_box)
{
    P_GR_TEXT t;

    if(NULL != (t = gr_text_search_key(cp, key)))
        *p_box = t->box;
    else
        gr_box_make_null(p_box);
}

_Check_return_
extern STATUS
gr_text_box_set(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     LIST_ITEMNO key,
    _InRef_     PC_GR_BOX p_box)
{
    P_GR_TEXT t;

    if(NULL != (t = gr_text_search_key(cp, key)))
    {
        gr_box_sort(&t->box, p_box); /* always ensure sorted */
    }

    return(STATUS_OK);
}

extern void
gr_text_delete(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     LIST_ITEMNO key)
{
    P_GR_TEXT t = gr_text_search_key(cp, key);

    PTR_ASSERT(t);

    if(t->bits.live_text)
    {
        P_GR_TEXT_GUTS gutsp;

        /* kill client dep unless it was him who killed us */
        gutsp.p_gr_text = t + 1;

        if(gutsp.p_gr_datasource->dsh != GR_DATASOURCE_HANDLE_NONE)
            status_consume(gr_travel_dsp_null(gutsp.p_gr_datasource, 0));
    }

#if 0
    if(cp->core.ceh)
    {
        P_GR_CHART_EDIT cep = gr_chart_edit_cep_from_ceh(cp->core.ceh);

        if(cep->selection.id.name == GR_CHART_OBJNAME_TEXT)
            if(cep->selection.id.no == key)
                gr_chart_edit_selection_clear(cep);
    }
#endif

    /* don't change key numbering of subsequent entries in either list */
    collect_subtract_entry(&cp->text.lbr, key);
    collect_subtract_entry(&cp->text.style.lbr, key);
}

/* SKS after PD 4.12 27mar92 - needed for live text reload mechanism */

static LIST_ITEMNO gr_text_key_to_use = 0;

extern LIST_ITEMNO
gr_text_key_for_new(
    _ChartRef_  P_GR_CHART cp)
{
    P_GR_TEXT t;
    LIST_ITEMNO key;

    /* SKS after PD 4.12 27mar92 - needed for live text reload mechanism */
    if(gr_text_key_to_use != 0)
    {
        key = gr_text_key_to_use;
        gr_text_key_to_use = 0;
        return(key);
    }

    /* add to end of list or reuse dead one */
    key = list_numitem(&cp->text.lbr);

    if(key)
    {
        --key;

        if(NULL != (t = gr_text_search_key(cp, key)))
            if(!t || t->bits.unused)
                return(key);
    }

    return(++key);
}

/* SKS after PD 4.12 27mar92 - needed for live text reload mechanism */

extern void
gr_chart_text_order_set(
    _InVal_     GR_CHART_HANDLE ch,
    _InVal_     S32 key)
{
    UNREFERENCED_PARAMETER_InVal_(ch);

    gr_text_key_to_use = key;
}

/* create text object near (x, y) */

_Check_return_
extern STATUS
gr_text_new(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     LIST_ITEMNO key,
    _In_opt_z_  PC_U8Z szText,
    _InRef_opt_ PC_GR_POINT point)
{
    U32 text_lenp1 = szText ? strlen32p1(szText) /*CH_NULL*/ : 0;
    P_GR_TEXT t;
    U32 n_bytes = sizeof32(*t) + (szText ? text_lenp1 : sizeof32(GR_DATASOURCE));
    STATUS status;

    if(NULL == (t = collect_add_entry_bytes(GR_TEXT, &cp->text.lbr, P_DATA_NONE, n_bytes, key, &status)))
        return(status);

    zero_struct_ptr(t);

    if(point)
    {
        t->box.x0 = point->x;
        t->box.y0 = point->y;
    }
    else
    {
        t->box.x0 = ((S32) key * cp->text.style.base.size_y);
        t->box.y0 = ((S32) key * cp->text.style.base.size_y * 12) / 10;
        t->box.y0 = t->box.y0 % cp->core.layout.height;
        t->box.y0 = cp->core.layout.height - t->box.y0;
    }
    t->box.x1 = t->box.x0 + (szText ? (text_lenp1-1) : 1) * SYSCHARWIDTH_PIXIT;
    t->box.y1 = t->box.y0 + (cp->text.style.base.size_y * 3) / 2;

    if(szText)
        /* set contents to desired string */
        memcpy32((t + 1), szText, text_lenp1);
    else
        t->bits.live_text = 1;

    return(STATUS_OK);
}

/* end of gr_texts.c */
