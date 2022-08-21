/* gr_chtio.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Charting module i/o */

/* SKS August 1993 for Fireworkz */

#include "common/gflags.h"

#include "ob_chart/ob_chart.h"

#include "cmodules/collect.h"

/*
internal structure
*/

typedef struct CHART_LOAD_INSTANCE
{
    ARRAY_HANDLE h_mapping_list;
    P_CHART_HEADER this_p_chart_header;

    LIST_BLOCK fillstyleb_translation_table;
}
CHART_LOAD_INSTANCE, * P_CHART_LOAD_INSTANCE;

#define chart_load_instance_goto(p_of_ip_format) \
    collect_goto_item(CHART_LOAD_INSTANCE, &(p_of_ip_format)->object_data_list, OBJECT_ID_CHART)

typedef struct CHART_LOAD_MAP
{
    P_CHART_HEADER p_chart_header;
    ARRAY_INDEX extref;
}
CHART_LOAD_MAP, * P_CHART_LOAD_MAP;

typedef struct CHART_SAVE_INSTANCE
{
    ARRAY_HANDLE h_mapping_list;

    U32 fillstyleb_translation_ekey;
    LIST_BLOCK fillstyleb_translation_table;
}
CHART_SAVE_INSTANCE, * P_CHART_SAVE_INSTANCE;

#define chart_save_instance_goto(p_of_op_format) \
    collect_goto_item(CHART_SAVE_INSTANCE, &(p_of_op_format)->object_data_list, OBJECT_ID_CHART)

typedef struct CHART_SAVE_MAP
{
    P_ANY object_data_ref;
    P_CHART_HEADER p_chart_header;
}
CHART_SAVE_MAP, * P_CHART_SAVE_MAP;

#define RISCOS_DRAW_FILE_ID TEXT("RISC OS")

/*
internal functions
*/

_Check_return_
static STATUS
chart_save_guts(
    _InoutRef_  P_OF_OP_FORMAT f,
    P_CHART_HEADER p_chart_header);

_Check_return_
static STATUS
chart_save_id_now(
    _ChartRef_  P_GR_CHART cp,
    _InoutRef_  P_OF_OP_FORMAT f);

/******************************************************************************
*
* maintain a translation list between cache handles and stored fillstylebs
*
******************************************************************************/

T5_CMD_PROTO(static, chart_load_pict_trans)
{
    STATUS status = STATUS_OK;
    const P_CHART_LOAD_INSTANCE p_chart_load_instance = chart_load_instance_goto(p_t5_cmd->p_of_ip_format);
    GR_CACHE_HANDLE cah;
    LIST_ITEMNO key;
    S32 res;

    IGNOREPARM_DocuRef_(p_docu);
    PTR_ASSERT(p_chart_load_instance);

    switch(t5_message)
    {
    case T5_CMD_CHART_IO_PICT_TRANS_REF:
        {
        const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 3);
        TCHARZ picture_namebuf[BUF_MAX_PATHSTRING];
        PCTSTR input_filename = p_t5_cmd->p_of_ip_format->input_filename;
        PCTSTR tstr_in_name = p_args[2].val.tstr;
        T5_FILETYPE t5_filetype;

        /* use external handle as key into list */
        key = (LIST_ITEMNO) p_args[0].val.s32;

        if((res = file_find_on_path_or_relative(picture_namebuf, elemof32(picture_namebuf), tstr_in_name, input_filename)) <= 0)
            return(res ? res : create_error(FILE_ERR_NOTFOUND));

        t5_filetype = host_t5_filetype_from_file(picture_namebuf);

        if((res = gr_cache_entry_ensure(&cah, picture_namebuf, t5_filetype)) <= 0)
            return(res ? res : status_nomem());

        break;
        }

    default: default_unhandled();
#if CHECKING
    case T5_CMD_CHART_IO_PICT_TRANS_EMB:
#endif
        {
        const P_ARGLIST_ARG p_args = p_arglist_args(&p_t5_cmd->arglist_handle, 3);

        /* use external handle as key into list */
        key = (LIST_ITEMNO) p_args[0].val.s32;

        status_return(gr_cache_embedded(&cah, &p_args[2].val.raw)); /* NB may steal handle */
        break;
        }
    }

    /* stick cache handle on the list */
    if(NULL == collect_add_entry_elem(GR_CACHE_HANDLE, &p_chart_load_instance->fillstyleb_translation_table, &cah, key, &status))
        return(status);

    return(STATUS_OK);
}

static S32
gr_fillstyleb_translate_pict_for_load(
    /*inout*/ P_GR_FILLSTYLEB fillstyleb,
    _InoutRef_  P_CHART_LOAD_INSTANCE p_chart_load_instance)
{
    LIST_ITEMNO key;
    P_GR_CACHE_HANDLE cahp;

    /* use external handle as key into list */
    key = (LIST_ITEMNO) fillstyleb->pattern; /* small non-zero integer here! */

    if(!key)
        return(0); /* auto-picture, leave bits alone */

    cahp = collect_goto_item(GR_CACHE_HANDLE, &p_chart_load_instance->fillstyleb_translation_table, key);

    if(!cahp)
    {
        fillstyleb->pattern = GR_FILL_PATTERN_NONE;

        fillstyleb->bits.pattern = 0;
        fillstyleb->bits.notsolid = 0; /* force to be solid */

        return(0);
    }

    fillstyleb->pattern = (GR_FILL_PATTERN_HANDLE) *cahp;

    return(1);
}

_Check_return_
static U32
gr_fillstyleb_translate_pict_for_save(
    _InRef_opt_ PC_GR_FILLSTYLEB fillstyleb,
    _InoutRef_  P_CHART_SAVE_INSTANCE p_chart_save_instance)
{
    LIST_ITEMNO key;
    P_U32 p_u32;

    if(!fillstyleb)
        return(0);

    /* use cache handle as key into list */
    key = (LIST_ITEMNO) fillstyleb->pattern;

    if(!key)
        return(0);

    if((p_u32 = collect_goto_item(U32, &p_chart_save_instance->fillstyleb_translation_table, key)) == NULL)
        return(0);

    return(*p_u32);
}

/*
exported for point list enumerators to call us back when we call them!
*/

_Check_return_
extern STATUS
gr_fillstyleb_make_key_for_save(
    _InRef_     PC_GR_FILLSTYLEB fillstyleb,
    P_ANY client_handle)
{
    const P_CHART_SAVE_INSTANCE p_chart_save_instance = (P_CHART_SAVE_INSTANCE) client_handle;
    LIST_ITEMNO key;
    P_U32 p_u32;
    STATUS status;

    /* use cache handle as key into list */
    key = (LIST_ITEMNO) fillstyleb->pattern;

    if(0 == key)
        return(STATUS_OK);

    if((p_u32 = collect_goto_item(U32, &p_chart_save_instance->fillstyleb_translation_table, key)) == NULL)
    {
        U32 u32 = p_chart_save_instance->fillstyleb_translation_ekey++;

        if(NULL == collect_add_entry_elem(U32, &p_chart_save_instance->fillstyleb_translation_table, &u32, key, &status))
            return(status);
    }

    return(STATUS_DONE);
}

/******************************************************************************
*
* need to create an external id from cache handle translation list
* has to be output before any constructs that have fill patterns
*
******************************************************************************/

_Check_return_
static STATUS
gr_fillstyleb_table_make_for_save(
    _ChartRef_  P_GR_CHART cp,
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format)
{
    const P_CHART_SAVE_INSTANCE p_chart_save_instance = chart_save_instance_goto(p_of_op_format);
    PTSTR tstr_out_name;
    S32 plotidx;
    GR_SERIES_IDX series_idx;
    U32 ekey;

    PTR_ASSERT(p_chart_save_instance);

    /* all picture references start going out from this value, rather than as their current cache handles */
    p_chart_save_instance->fillstyleb_translation_ekey = 1;

    list_init(&p_chart_save_instance->fillstyleb_translation_table);

    tstr_out_name = p_of_op_format->output.u.file.filename;
    if(tstr_out_name && !file_is_rooted(tstr_out_name))
        tstr_out_name = NULL;

    /* note refs to pictures */
    status_return(gr_fillstyleb_make_key_for_save(&cp->chart.areastyleb, p_chart_save_instance));

    for(plotidx = 0; plotidx < (cp->d3.bits.use ? GR_CHART_N_PLOTAREAS : 1); ++plotidx)
        status_return(gr_fillstyleb_make_key_for_save(&cp->plotarea.area[plotidx].areastyleb, p_chart_save_instance));

    if(cp->legend.bits.on)
        status_return(gr_fillstyleb_make_key_for_save(&cp->legend.areastyleb, p_chart_save_instance));

    for(series_idx = 0; series_idx < array_elements(&cp->series.mh); ++series_idx)
    {
        /* note refs to series pictures */
        const P_GR_SERIES serp = getserp(cp, series_idx);

        status_return(gr_fillstyleb_make_key_for_save(&serp->style.pdrop_fillb, p_chart_save_instance)); /* this may be overenthusiastic */
        status_return(gr_fillstyleb_make_key_for_save(&serp->style.point_fillb, p_chart_save_instance));

        /* note refs to all point info from this series */
        status_return(gr_point_list_fillstyleb_enum_for_save(cp, series_idx, GR_LIST_PDROP_FILLSTYLEB, p_chart_save_instance)); /* this may be overenthusiastic */
        status_return(gr_point_list_fillstyleb_enum_for_save(cp, series_idx, GR_LIST_POINT_FILLSTYLEB, p_chart_save_instance));
    }

    /* loop over the list saving correspondences */
    for(ekey = 1; ekey < p_chart_save_instance->fillstyleb_translation_ekey; ++ekey)
    {
        P_U32 p_u32;
        LIST_ITEMNO key;

        for(p_u32 = collect_first(U32, &p_chart_save_instance->fillstyleb_translation_table, &key);
            p_u32;
            p_u32 = collect_next(U32, &p_chart_save_instance->fillstyleb_translation_table, &key))
        {
            GR_CACHE_HANDLE cah;
            TCHARZ name_buffer[BUF_MAX_PATHSTRING];
            PTSTR picture_name;
            BOOL embedded;

            if(ekey != *p_u32)
                continue;

            cah = (GR_CACHE_HANDLE) key;

            if(gr_cache_name_query(cah, name_buffer, elemof32(name_buffer)))
            {
                P_FILE_PATHENUM path;
                PTSTR tstr_path;
                PTSTR pathelem;

                picture_name = name_buffer;

                /* loop over path to find minimalist reference (like localise_filename but on steroids) */
                status_assert(file_combine_path(&tstr_path, tstr_out_name, file_get_search_path()));

                for(pathelem = file_path_element_first(&path, tstr_path); NULL != pathelem; pathelem = file_path_element_next(&path))
                {
                    U32 pathlen = tstrlen32(pathelem);

                    if(0 == tstrnicmp(picture_name, pathelem, pathlen))
                    {
                        picture_name += pathlen;
                        break;
                    }
                }

                file_path_element_close(&path);

                tstr_clr(&tstr_path);

                embedded = FALSE;
            }
            else
            {
                picture_name = NULL;

                embedded = TRUE;
            }

            {
            const OBJECT_ID object_id = OBJECT_ID_CHART;
            const T5_MESSAGE t5_message = embedded ? T5_CMD_CHART_IO_PICT_TRANS_EMB : T5_CMD_CHART_IO_PICT_TRANS_REF;
            PC_CONSTRUCT_TABLE p_construct_table;
            ARGLIST_HANDLE arglist_handle;
            STATUS status;

            if(status_ok(status = arglist_prepare_with_construct(&arglist_handle, object_id, t5_message, &p_construct_table)))
            {
                const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 3);

                p_args[0].val.s32 = ekey;

                p_args[1].val.tstr = RISCOS_DRAW_FILE_ID;

                if(embedded)
                    p_args[2].val.raw = gr_cache_search(cah); /* loan handle for output */
                else
                    p_args[2].val.tstr = picture_name;

                status = ownform_save_arglist(arglist_handle, object_id, p_construct_table, p_of_op_format);

                p_args[2].val.raw = 0; /* reclaim handle */

                arglist_dispose(&arglist_handle);
            }

            status_return(status);
            } /*block*/

            /* stop this loop, go find another key to look up */
            break;
        }
    }

    return(STATUS_OK);
}

#define BUF_MAX_LOADSAVELINE 1024

/******************************************************************************
*
* maintain a current id during load and save ops
* and some state dervived therefrom for direct pokers
*
******************************************************************************/

enum GR_CONSTRUCT_OBJ_TYPE
{
    GR_STR_NONE = 0,
    GR_STR_CHART,    /* poke field in cp */
    GR_STR_AXES,     /* poke field in cp->axes[axes_idx] */
    GR_STR_AXIS,     /* poke field in cp->axes[axes_idx].axis[axis_idx] */
    GR_STR_SERIES    /* poke field in cp->...series.mh[series_idx]  */
};

typedef struct GR_CONSTRUCT_TABLE_ENTRY
{
#if 1
    enum GR_CONSTRUCT_OBJ_TYPE obj_type;
#else
    UBF obj_type : 8; /* packed GR_CONSTRUCT_OBJ_TYPE */
    UBF reserved : 24;
#endif

    U32 offset;

    T5_MESSAGE t5_message;
}
GR_CONSTRUCT_TABLE_ENTRY, * P_GR_CONSTRUCT_TABLE_ENTRY; typedef const GR_CONSTRUCT_TABLE_ENTRY * PC_GR_CONSTRUCT_TABLE_ENTRY; 

#define gr_contab_entry(obj, offset, t5_message) \
    { obj, /*0,*/ offset, t5_message }

static /*sorted*/ GR_CONSTRUCT_TABLE_ENTRY
chart_construct_table[] =
{
    /*
    gr_chart
    */

    gr_contab_entry(GR_STR_CHART,  offsetof32(GR_CHART, axes_idx_max), T5_CMD_CHART_IO_AXES_MAX),

    gr_contab_entry(GR_STR_CHART,  offsetof32(GR_CHART, core) + offsetof32(GR_CHART_CORE, layout) + offsetof32(GR_CHART_LAYOUT, width),   T5_CMD_CHART_IO_CORE_LAYOUT), /* and height too */
    gr_contab_entry(GR_STR_CHART,  offsetof32(GR_CHART, core) + offsetof32(GR_CHART_CORE, layout) + offsetof32(GR_CHART_LAYOUT, margins), T5_CMD_CHART_IO_CORE_MARGINS),

    gr_contab_entry(GR_STR_CHART,  offsetof32(GR_CHART, legend) + offsetof32(GR_CHART_LEGEND, bits),         T5_CMD_CHART_IO_LEGEND_BITS),
    gr_contab_entry(GR_STR_CHART,  offsetof32(GR_CHART, legend) + offsetof32(GR_CHART_LEGEND, posn),         T5_CMD_CHART_IO_LEGEND_POSN), /* and size too */

    gr_contab_entry(GR_STR_CHART,  offsetof32(GR_CHART, d3) + offsetof32(GR_CHART_D3, bits),                 T5_CMD_CHART_IO_D3_BITS),

    gr_contab_entry(GR_STR_CHART,  offsetof32(GR_CHART, d3) + offsetof32(GR_CHART_D3, pitch),                T5_CMD_CHART_IO_D3_PITCH),
    gr_contab_entry(GR_STR_CHART,  offsetof32(GR_CHART, d3) + offsetof32(GR_CHART_D3, roll),                 T5_CMD_CHART_IO_D3_ROLL),

    gr_contab_entry(GR_STR_CHART,  offsetof32(GR_CHART, barch) + offsetof32(GR_CHART_BARCH, slot_overlap_percentage), T5_CMD_CHART_IO_BARCH_SLOT_2D_OVERLAP),

    gr_contab_entry(GR_STR_CHART,  offsetof32(GR_CHART, linech) + offsetof32(GR_CHART_LINECH, slot_shift_percentage), T5_CMD_CHART_IO_LINECH_SLOT_2D_SHIFT),

    /*
    gr_axes
    */

    gr_contab_entry(GR_STR_AXES,   offsetof32(GR_AXES, bits),      T5_CMD_CHART_IO_AXES_BITS),
    gr_contab_entry(GR_STR_AXES,   offsetof32(GR_AXES, sertype),   T5_CMD_CHART_IO_AXES_SERIES_TYPE), /* default series type */
    gr_contab_entry(GR_STR_AXES,   offsetof32(GR_AXES, chart_type), T5_CMD_CHART_IO_AXES_CHART_TYPE),  /* default series chart type */

    gr_contab_entry(GR_STR_AXES,   offsetof32(GR_AXES, series) + offsetof32(GR_AXES_SERIES, start_series), T5_CMD_CHART_IO_AXES_START_SERIES),

    /*
    gr_axis
    */

    gr_contab_entry(GR_STR_AXIS,   offsetof32(GR_AXIS, bits),                                            T5_CMD_CHART_IO_AXIS_BITS),
    gr_contab_entry(GR_STR_AXIS,   offsetof32(GR_AXIS, punter) + offsetof32(GR_MINMAX_NUMBER, min),        T5_CMD_CHART_IO_AXIS_PUNTER_MIN),
    gr_contab_entry(GR_STR_AXIS,   offsetof32(GR_AXIS, punter) + offsetof32(GR_MINMAX_NUMBER, max),        T5_CMD_CHART_IO_AXIS_PUNTER_MAX),

    gr_contab_entry(GR_STR_AXIS,   offsetof32(GR_AXIS, major) + offsetof32(GR_AXIS_TICKS, bits),   T5_CMD_CHART_IO_AXIS_MAJOR_BITS),
    gr_contab_entry(GR_STR_AXIS,   offsetof32(GR_AXIS, major) + offsetof32(GR_AXIS_TICKS, punter), T5_CMD_CHART_IO_AXIS_MAJOR_PUNTER),

    gr_contab_entry(GR_STR_AXIS,   offsetof32(GR_AXIS, minor) + offsetof32(GR_AXIS_TICKS, bits),   T5_CMD_CHART_IO_AXIS_MINOR_BITS),
    gr_contab_entry(GR_STR_AXIS,   offsetof32(GR_AXIS, minor) + offsetof32(GR_AXIS_TICKS, punter), T5_CMD_CHART_IO_AXIS_MINOR_PUNTER),

    /*
    gr_series
    */

    gr_contab_entry(GR_STR_SERIES, offsetof32(GR_SERIES, bits),                                                 T5_CMD_CHART_IO_SERIES_BITS),
    gr_contab_entry(GR_STR_SERIES, offsetof32(GR_SERIES, style) + offsetof32(GR_SERIES_STYLE, pie_start_heading), T5_CMD_CHART_IO_SERIES_PIE_HEADING),
    gr_contab_entry(GR_STR_SERIES, offsetof32(GR_SERIES, sertype),                                              T5_CMD_CHART_IO_SERIES_SERIES_TYPE),
    gr_contab_entry(GR_STR_SERIES, offsetof32(GR_SERIES, chart_type),                                           T5_CMD_CHART_IO_SERIES_CHART_TYPE),

    /*
    harder to poke, more generic types
    */

    gr_contab_entry(GR_STR_NONE, 0, T5_CMD_CHART_IO_FILLSTYLEB),
    gr_contab_entry(GR_STR_NONE, 0, T5_CMD_CHART_IO_FILLSTYLEC),
    gr_contab_entry(GR_STR_NONE, 0, T5_CMD_CHART_IO_LINESTYLE),
    gr_contab_entry(GR_STR_NONE, 0, T5_CMD_CHART_IO_TEXTSTYLE),

    gr_contab_entry(GR_STR_NONE, 0, T5_CMD_CHART_IO_BARCHSTYLE),
    gr_contab_entry(GR_STR_NONE, 0, T5_CMD_CHART_IO_BARLINECHSTYLE),
    gr_contab_entry(GR_STR_NONE, 0, T5_CMD_CHART_IO_LINECHSTYLE),
    gr_contab_entry(GR_STR_NONE, 0, T5_CMD_CHART_IO_PIECHDISPLSTYLE),
    gr_contab_entry(GR_STR_NONE, 0, T5_CMD_CHART_IO_PIECHLABELSTYLE),
    gr_contab_entry(GR_STR_NONE, 0, T5_CMD_CHART_IO_SCATCHSTYLE),

    gr_contab_entry(GR_STR_NONE, 0, T5_CMD_CHART_IO_TEXTPOS),
};

/* no context needed for qsort() */

PROC_QSORT_PROTO(static, proc_qsort_chart_construct_table, GR_CONSTRUCT_TABLE_ENTRY)
{
    QSORT_ARG1_VAR_DECL(PC_GR_CONSTRUCT_TABLE_ENTRY, pgce1);
    QSORT_ARG2_VAR_DECL(PC_GR_CONSTRUCT_TABLE_ENTRY, pgce2);

    if(pgce1->t5_message == pgce2->t5_message)
        return(0);

    return((pgce1->t5_message > pgce2->t5_message) ? +1 : -1);
}

PROC_BSEARCH_PROTO(static, proc_bsearch_chart_construct_table, T5_MESSAGE, GR_CONSTRUCT_TABLE_ENTRY)
{
    BSEARCH_KEY_VAR_DECL(PC_T5_MESSAGE, key);
    const T5_MESSAGE t5_message_key = *key;
    BSEARCH_DATUM_VAR_DECL(PC_GR_CONSTRUCT_TABLE_ENTRY, datum);
    const T5_MESSAGE t5_message_datum = datum->t5_message;

    if(t5_message_key == t5_message_datum)
        return(0);

    return((t5_message_key > t5_message_datum) ? +1 : -1);
}

static P_GR_CHART
chart_load_p_gr_chart;

static GR_CHART_OBJID
chart_load_save_objid;

static GR_AXES_IDX
chart_load_save_use_axes_idx;

static GR_AXIS_IDX
chart_load_save_use_axis_idx;

static GR_SERIES_IDX
chart_load_save_use_series_idx;

static BOOL
chart_load_save_use_series_idx_ok;

/*
a three state object:
    0 - id construct has been output
    1 - id construct awaiting output
    2 - perform id construct output
*/

static S32
chart_save_id_out_pending;

_Check_return_
static STATUS
chart_io_id_changed(
    _ChartRef_  P_GR_CHART cp,
    _InVal_     BOOL in_save)
{
    chart_load_save_use_axes_idx = 0;
    chart_load_save_use_axis_idx = 0;
    chart_load_save_use_series_idx = 0;
    chart_load_save_use_series_idx_ok = 0;

    /* now we have to work out what this 'selected' for direct pokers */
    switch(chart_load_save_objid.name)
    {
    default:
#if CHECKING
    case GR_CHART_OBJNAME_LEGDSERIES:
    case GR_CHART_OBJNAME_LEGDPOINT:
        assert0();

        /*FALLTHRU*/

    case GR_CHART_OBJNAME_CHART:
    case GR_CHART_OBJNAME_PLOTAREA:
    case GR_CHART_OBJNAME_LEGEND:
    case GR_CHART_OBJNAME_TEXT:
#endif
        break;

    case GR_CHART_OBJNAME_SERIES:
    case GR_CHART_OBJNAME_POINT:
    case GR_CHART_OBJNAME_DROPSERIES:
    case GR_CHART_OBJNAME_DROPPOINT:
    case GR_CHART_OBJNAME_BESTFITSER:
        {
        GR_SERIES_IDX series_idx;

        if(in_save)
            series_idx = gr_series_idx_from_external(cp, chart_load_save_objid.no);
        else
        {
            P_GR_SERIES serp;

            /* SKS after PD 4.12 26mar92 - series are loaded contiguously into axes set 0 and
             * eventually split with explicit split point to preserve overlay series styles
            */
            series_idx = (GR_SERIES_IDX) chart_load_save_objid.no - 1;

            if(series_idx >= cp->axes[0].series.end_idx)
            {
                status_return(gr_chart_add_series(cp, 0, 1 /*init if new*/));

                /* NB. convert using fn as allocation may have changed */
                series_idx = gr_series_idx_from_external(cp, chart_load_save_objid.no);

                serp = getserp(cp, series_idx);

                /* don't auto-reinitialise descriptors created by loading */
                serp->internal_bits.descriptor_ok = 1;
            }
        }

        chart_load_save_use_axes_idx = gr_axes_idx_from_series_idx(cp, series_idx);
        chart_load_save_use_series_idx = series_idx;
        chart_load_save_use_series_idx_ok = 1;
        break;
        }

    case GR_CHART_OBJNAME_AXIS:
    case GR_CHART_OBJNAME_AXISGRID:
    case GR_CHART_OBJNAME_AXISTICK:
        /* can poke unused axes ok */

        chart_load_save_use_axis_idx = gr_axes_idx_from_external(cp, chart_load_save_objid.no, &chart_load_save_use_axes_idx);
        break;
    }

    return(STATUS_OK);
}

T5_CMD_PROTO(extern, chart_io_gr_construct_load)
{
    STATUS status = STATUS_OK;
    const P_GR_CHART cp = chart_load_p_gr_chart;
    P_GR_CONSTRUCT_TABLE_ENTRY p_con;
    const GR_CHART_OBJID id = chart_load_save_objid;
    P_ANY p0;

    IGNOREPARM_DocuRef_(p_docu);

    p_con = (P_GR_CONSTRUCT_TABLE_ENTRY)
        bsearch(&t5_message, chart_construct_table, elemof(chart_construct_table), sizeof(chart_construct_table[0]), proc_bsearch_chart_construct_table);

    PTR_ASSERT(p_con);

    switch(p_con->obj_type)
    {
    default: default_unhandled();
#if CHECKING
    case GR_STR_NONE:
#endif
        p0 = NULL;
        break;

    case GR_STR_CHART:
        PTR_ASSERT(cp);
        p0 = cp;
        if(!p0)
            return(STATUS_OK);
        break;

    case GR_STR_AXES:
        PTR_ASSERT(cp);
        p0 = cp ? &cp->axes[chart_load_save_use_axes_idx] : NULL;
        if(!p0)
            return(STATUS_OK);
        break;

    case GR_STR_AXIS:
        PTR_ASSERT(cp);
        p0 = cp ? &cp->axes[chart_load_save_use_axes_idx].axis[chart_load_save_use_axis_idx] : NULL;
        if(!p0)
            return(STATUS_OK);
        break;

    case GR_STR_SERIES:
        PTR_ASSERT(cp);
        assert(chart_load_save_use_series_idx_ok);
        p0 = (cp && chart_load_save_use_series_idx_ok) ? getserp(cp, chart_load_save_use_series_idx) : NULL;
        if(!p0)
            return(STATUS_OK);
        break;
    }

    if(p0)
        p0 = (P_ANY) ((P_U8) p0 + p_con->offset);

    switch(t5_message)
    {
    default: default_unhandled();
    case T5_CMD_CHART_IO_AXES_MAX:
    case T5_CMD_CHART_IO_LEGEND_BITS:
    case T5_CMD_CHART_IO_D3_BITS:
    case T5_CMD_CHART_IO_BARCH_SLOT_2D_OVERLAP:
    case T5_CMD_CHART_IO_LINECH_SLOT_2D_SHIFT:
    case T5_CMD_CHART_IO_AXES_BITS:
    case T5_CMD_CHART_IO_AXES_SERIES_TYPE:
    case T5_CMD_CHART_IO_AXES_CHART_TYPE:
    case T5_CMD_CHART_IO_AXES_START_SERIES:
    case T5_CMD_CHART_IO_AXIS_BITS:
    case T5_CMD_CHART_IO_AXIS_MAJOR_BITS:
    case T5_CMD_CHART_IO_AXIS_MINOR_BITS:
    case T5_CMD_CHART_IO_SERIES_BITS:
    case T5_CMD_CHART_IO_SERIES_SERIES_TYPE:
    case T5_CMD_CHART_IO_SERIES_CHART_TYPE:
        {
        const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 1);
        P_S32 p_s32 = (P_S32) p0;
        assert(n_arglist_args(&p_t5_cmd->arglist_handle) == 1);
        assert(p_args[0].type == (ARG_TYPE_S32 | ARG_MANDATORY));
        PTR_ASSERT(p_s32);
        if(NULL != p_s32)
            * p_s32 = p_args[0].val.s32;
        break;
        }

    case T5_CMD_CHART_IO_CORE_LAYOUT:
        {
        const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 2);
        P_S32 p_s32 = (P_S32) p0;
        assert(n_arglist_args(&p_t5_cmd->arglist_handle) == 2);
        assert(p_args[0].type == (ARG_TYPE_S32 | ARG_MANDATORY));
        assert(p_args[1].type == (ARG_TYPE_S32 | ARG_MANDATORY));
        PTR_ASSERT(p_s32);
        if(NULL != p_s32)
        {
            p_s32[0] = p_args[0].val.s32;
            p_s32[1] = p_args[1].val.s32;
        }
        break;
        }

    case T5_CMD_CHART_IO_CORE_MARGINS:
    case T5_CMD_CHART_IO_LEGEND_POSN:
        {
        const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 4);
        P_S32 p_s32 = (P_S32) p0;
        assert(n_arglist_args(&p_t5_cmd->arglist_handle) == 4);
        assert(p_args[0].type == (ARG_TYPE_S32 | ARG_MANDATORY));
        assert(p_args[1].type == (ARG_TYPE_S32 | ARG_MANDATORY));
        assert(p_args[2].type == (ARG_TYPE_S32 | ARG_MANDATORY));
        assert(p_args[3].type == (ARG_TYPE_S32 | ARG_MANDATORY));
        PTR_ASSERT(p_s32);
        if(NULL != p_s32)
        {
            p_s32[0] = p_args[0].val.s32;
            p_s32[1] = p_args[1].val.s32;
            p_s32[2] = p_args[2].val.s32;
            p_s32[3] = p_args[3].val.s32;
        }
        break;
        }

    case T5_CMD_CHART_IO_D3_PITCH:
    case T5_CMD_CHART_IO_D3_ROLL:
    case T5_CMD_CHART_IO_AXIS_PUNTER_MIN:
    case T5_CMD_CHART_IO_AXIS_PUNTER_MAX:
    case T5_CMD_CHART_IO_AXIS_MAJOR_PUNTER:
    case T5_CMD_CHART_IO_AXIS_MINOR_PUNTER:
    case T5_CMD_CHART_IO_SERIES_PIE_HEADING:
        {
        const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 1);
        P_F64 p_f64 = (P_F64) p0;
        assert(n_arglist_args(&p_t5_cmd->arglist_handle) == 1);
        assert(p_args[0].type == (ARG_TYPE_F64 | ARG_MANDATORY));
        PTR_ASSERT(p_f64);
        if(NULL != p_f64)
            * p_f64 = p_args[0].val.f64;
        break;
        }

    case T5_CMD_CHART_IO_FILLSTYLEB:
        {
        const P_CHART_LOAD_INSTANCE p_chart_load_instance = chart_load_instance_goto(p_t5_cmd->p_of_ip_format);
        const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 2);
        GR_FILLSTYLEB style;

        gr_chart_objid_fillstyleb_query(cp, id, &style);

        assert(n_arglist_args(&p_t5_cmd->arglist_handle) == 2);
        * (P_U32) &style.bits = p_args[0].val.s32;
        style.pattern = (GR_FILL_PATTERN_HANDLE) p_args[1].val.s32;

        PTR_ASSERT(p_chart_load_instance);
        gr_fillstyleb_translate_pict_for_load(&style, p_chart_load_instance);

        status = gr_chart_objid_fillstyleb_set(cp, id, &style);

        break;
        }

    case T5_CMD_CHART_IO_FILLSTYLEC:
        {
        const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 4);
        GR_FILLSTYLEC style;

        gr_chart_objid_fillstylec_query(cp, id, &style);

        style.fg.manual = 1;
        style.fg.visible = 1;

        assert(n_arglist_args(&p_t5_cmd->arglist_handle) == 4);
        style.fg.red   = p_args[0].val.u8n;
        style.fg.green = p_args[1].val.u8n;
        style.fg.blue  = p_args[2].val.u8n;
        if(arg_is_present(p_args, 3))
            style.fg.visible = !p_args[3].val.fBool;

        status = gr_chart_objid_fillstylec_set(cp, id, &style);

        break;
        }

    case T5_CMD_CHART_IO_LINESTYLE:
        {
        const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 6);
        GR_LINESTYLE style;

        gr_chart_objid_linestyle_query(cp, id, &style);

        assert(n_arglist_args(&p_t5_cmd->arglist_handle) == 6);
        * (P_S32) &style.pattern = p_args[0].val.s32;
        style.width = p_args[1].val.s32;

        style.fg.manual = 1;
        style.fg.visible = 1;

        style.fg.red   = p_args[2].val.u8n;
        style.fg.green = p_args[3].val.u8n;
        style.fg.blue  = p_args[4].val.u8n;
        if(arg_is_present(p_args, 5))
            style.fg.visible = !p_args[5].val.fBool;

        status = gr_chart_objid_linestyle_set(cp, id, &style);

        break;
        }

    case T5_CMD_CHART_IO_TEXTSTYLE:
        {
        const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 9);
        GR_TEXTSTYLE style;

        gr_chart_objid_textstyle_query(cp, id, &style);

        assert(n_arglist_args(&p_t5_cmd->arglist_handle) == 9);
        style.height = (U16) p_args[0].val.s32; /* 16 bits limits to 45 inch or so */
        style.width  = (U16) p_args[1].val.s32;
        tstr_xstrkpy(style.tstrFontName, elemof32(style.tstrFontName), fontmap_remap(p_args[2].val.tstr));
        style.bold   = p_args[3].val.fBool;
        style.italic = p_args[4].val.fBool;

        style.fg.manual = 1;
        style.fg.visible = 1;

        style.fg.red   = p_args[5].val.u8n;
        style.fg.green = p_args[6].val.u8n;
        style.fg.blue  = p_args[7].val.u8n;
        if(arg_is_present(p_args, 8))
            style.fg.visible = !p_args[8].val.fBool;

        status = gr_chart_objid_textstyle_set(cp, id, &style);

        break;
        }

    case T5_CMD_CHART_IO_BARCHSTYLE:
        {
        const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 2);
        GR_BARCHSTYLE style;

        gr_chart_objid_barchstyle_query(cp, id, &style);

        assert(n_arglist_args(&p_t5_cmd->arglist_handle) == 2);
        assert(p_args[0].type == (ARG_TYPE_S32 | ARG_MANDATORY));
        assert(p_args[1].type == (ARG_TYPE_F64 | ARG_MANDATORY));
        * (P_S32) &style.bits = p_args[0].val.s32;
        style.slot_width_percentage = p_args[1].val.f64;

        status = gr_chart_objid_barchstyle_set(cp, id, &style);

        break;
        }

    case T5_CMD_CHART_IO_BARLINECHSTYLE:
        {
        const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 2);
        GR_BARLINECHSTYLE style;

        gr_chart_objid_barlinechstyle_query(cp, id, &style);

        assert(n_arglist_args(&p_t5_cmd->arglist_handle) == 2);
        assert(p_args[0].type == (ARG_TYPE_S32 | ARG_MANDATORY));
        assert(p_args[1].type == (ARG_TYPE_F64 | ARG_MANDATORY));
        * (P_S32) &style.bits = p_args[0].val.s32;
        style.slot_depth_percentage = p_args[1].val.f64;

        status = gr_chart_objid_barlinechstyle_set(cp, id, &style);

        break;
        }

    case T5_CMD_CHART_IO_LINECHSTYLE:
        {
        const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 2);
        GR_LINECHSTYLE style;

        gr_chart_objid_linechstyle_query(cp, id, &style);

        assert(n_arglist_args(&p_t5_cmd->arglist_handle) == 2);
        assert(p_args[0].type == (ARG_TYPE_S32 | ARG_MANDATORY));
        assert(p_args[1].type == (ARG_TYPE_F64 | ARG_MANDATORY));
        * (P_S32) &style.bits = p_args[0].val.s32;
        style.slot_width_percentage = p_args[1].val.f64;

        status = gr_chart_objid_linechstyle_set(cp, id, &style);

        break;
        }

    case T5_CMD_CHART_IO_PIECHDISPLSTYLE:
        {
        const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 2);
        GR_PIECHDISPLSTYLE style;

        gr_chart_objid_piechdisplstyle_query(cp, id, &style);

        assert(n_arglist_args(&p_t5_cmd->arglist_handle) == 2);
        assert(p_args[0].type == (ARG_TYPE_S32 | ARG_MANDATORY));
        assert(p_args[1].type == (ARG_TYPE_F64 | ARG_MANDATORY));
        * (P_S32) &style.bits = p_args[0].val.s32;
        style.radial_displacement = p_args[1].val.f64;

        status = gr_chart_objid_piechdisplstyle_set(cp, id, &style);

        break;
        }

    case T5_CMD_CHART_IO_PIECHLABELSTYLE:
        {
        const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 1);
        GR_PIECHLABELSTYLE style;

        gr_chart_objid_piechlabelstyle_query(cp, id, &style);

        assert(n_arglist_args(&p_t5_cmd->arglist_handle) == 1);
        assert(p_args[0].type == (ARG_TYPE_S32 | ARG_MANDATORY));
        * (P_S32) &style.bits = p_args[0].val.s32;

        status = gr_chart_objid_piechlabelstyle_set(cp, id, &style);

        break;
        }

    case T5_CMD_CHART_IO_SCATCHSTYLE:
        {
        const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 2);
        GR_SCATCHSTYLE style;

        gr_chart_objid_scatchstyle_query(cp, id, &style);

        assert(n_arglist_args(&p_t5_cmd->arglist_handle) == 2);
        assert(p_args[0].type == (ARG_TYPE_S32 | ARG_MANDATORY));
        assert(p_args[1].type == (ARG_TYPE_F64 | ARG_MANDATORY));
        * (P_S32) &style.bits = p_args[0].val.s32;
        style.width_percentage = p_args[1].val.f64;

        status = gr_chart_objid_scatchstyle_set(cp, id, &style);

        break;
        }

    case T5_CMD_CHART_IO_TEXTPOS:
        {
        const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 4);
        GR_BOX box;

        gr_text_box_query(cp, id.no, &box);

        assert(n_arglist_args(&p_t5_cmd->arglist_handle) == 4);
        assert(p_args[0].type == (ARG_TYPE_S32 | ARG_MANDATORY));
        assert(p_args[1].type == (ARG_TYPE_S32 | ARG_MANDATORY));
        assert(p_args[2].type == (ARG_TYPE_S32 | ARG_MANDATORY));
        assert(p_args[3].type == (ARG_TYPE_S32 | ARG_MANDATORY));
        box.x0 = p_args[0].val.s32;
        box.y0 = p_args[1].val.s32;
        box.x1 = p_args[2].val.s32;
        box.y1 = p_args[3].val.s32;

        status = gr_text_box_set(cp, id.no, &box);

        break;
        }
    }

    return(status);
}

_Check_return_
static STATUS
chart_construct_save(
    _ChartRef_  P_GR_CHART cp,
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format,
    _InVal_     T5_MESSAGE t5_message)
{
    P_GR_CONSTRUCT_TABLE_ENTRY p_con;
    const GR_CHART_OBJID id = chart_load_save_objid;
    P_ANY p0;
    const OBJECT_ID object_id = OBJECT_ID_CHART;
    PC_CONSTRUCT_TABLE p_construct_table;
    ARGLIST_HANDLE arglist_handle;

    p_con = (P_GR_CONSTRUCT_TABLE_ENTRY)
        bsearch(&t5_message, chart_construct_table, elemof(chart_construct_table), sizeof(chart_construct_table[0]), proc_bsearch_chart_construct_table);

    PTR_ASSERT(p_con);

    switch(p_con->obj_type)
    {
    default: default_unhandled();
#if CHECKING
    case GR_STR_NONE:
#endif
        p0 = NULL;
        break;

    case GR_STR_CHART:
        assert(!chart_save_id_out_pending);
        PTR_ASSERT(cp);
        p0 = cp;
        if(!p0)
            return(STATUS_OK);
        break;

    case GR_STR_AXES:
        assert(!chart_save_id_out_pending);
        PTR_ASSERT(cp);
        p0 = cp ? &cp->axes[chart_load_save_use_axes_idx] : NULL;
        if(!p0)
            return(STATUS_OK);
        break;

    case GR_STR_AXIS:
        assert(!chart_save_id_out_pending);
        PTR_ASSERT(cp);
        p0 = cp ? &cp->axes[chart_load_save_use_axes_idx].axis[chart_load_save_use_axis_idx] : NULL;
        if(!p0)
            return(STATUS_OK);
        break;

    case GR_STR_SERIES:
        assert(!chart_save_id_out_pending);
        PTR_ASSERT(cp);
        assert(chart_load_save_use_series_idx_ok);
        p0 = (cp && chart_load_save_use_series_idx_ok) ? getserp(cp, chart_load_save_use_series_idx) : NULL;
        if(!p0)
            return(STATUS_OK);
        break;
    }

    if(p0)
        p0 = (P_ANY) ((P_U8) p0 + p_con->offset);

    status_return(arglist_prepare_with_construct(&arglist_handle, object_id, t5_message, &p_construct_table));

    switch(t5_message)
    {
    default: default_unhandled();
    case T5_CMD_CHART_IO_AXES_MAX:
    case T5_CMD_CHART_IO_LEGEND_BITS:
    case T5_CMD_CHART_IO_D3_BITS:
    case T5_CMD_CHART_IO_BARCH_SLOT_2D_OVERLAP:
    case T5_CMD_CHART_IO_LINECH_SLOT_2D_SHIFT:
    case T5_CMD_CHART_IO_AXES_BITS:
    case T5_CMD_CHART_IO_AXES_SERIES_TYPE:
    case T5_CMD_CHART_IO_AXES_CHART_TYPE:
    case T5_CMD_CHART_IO_AXES_START_SERIES:
    case T5_CMD_CHART_IO_AXIS_BITS:
    case T5_CMD_CHART_IO_AXIS_MAJOR_BITS:
    case T5_CMD_CHART_IO_AXIS_MINOR_BITS:
    case T5_CMD_CHART_IO_SERIES_BITS:
    case T5_CMD_CHART_IO_SERIES_SERIES_TYPE:
    case T5_CMD_CHART_IO_SERIES_CHART_TYPE:
        {
        const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 1);
        PC_S32 p_s32 = (PC_S32) p0;
        assert(n_arglist_args(&arglist_handle) == 1);
        assert(p_args[0].type == (ARG_TYPE_S32 | ARG_MANDATORY));
        PTR_ASSERT(p_s32);
        if(NULL != p_s32)
            p_args[0].val.s32 = * p_s32;
        break;
        }

    case T5_CMD_CHART_IO_CORE_LAYOUT:
        {
        const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 2);
        PC_S32 p_s32 = (PC_S32) p0;
        assert(n_arglist_args(&arglist_handle) == 2);
        assert(p_args[0].type == (ARG_TYPE_S32 | ARG_MANDATORY));
        assert(p_args[1].type == (ARG_TYPE_S32 | ARG_MANDATORY));
        PTR_ASSERT(p_s32);
        if(NULL != p_s32)
        {
            p_args[0].val.s32 = p_s32[0];
            p_args[1].val.s32 = p_s32[1];
        }
        break;
        }

    case T5_CMD_CHART_IO_CORE_MARGINS:
    case T5_CMD_CHART_IO_LEGEND_POSN:
        {
        const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 4);
        PC_S32 p_s32 = (PC_S32) p0;
        assert(n_arglist_args(&arglist_handle) == 4);
        assert(p_args[0].type == (ARG_TYPE_S32 | ARG_MANDATORY));
        assert(p_args[1].type == (ARG_TYPE_S32 | ARG_MANDATORY));
        assert(p_args[2].type == (ARG_TYPE_S32 | ARG_MANDATORY));
        assert(p_args[3].type == (ARG_TYPE_S32 | ARG_MANDATORY));
        PTR_ASSERT(p_s32);
        if(NULL != p_s32)
        {
            p_args[0].val.s32 = p_s32[0];
            p_args[1].val.s32 = p_s32[1];
            p_args[2].val.s32 = p_s32[2];
            p_args[3].val.s32 = p_s32[3];
        }
        break;
        }

    case T5_CMD_CHART_IO_D3_PITCH:
    case T5_CMD_CHART_IO_D3_ROLL:
    case T5_CMD_CHART_IO_AXIS_PUNTER_MIN:
    case T5_CMD_CHART_IO_AXIS_PUNTER_MAX:
    case T5_CMD_CHART_IO_AXIS_MAJOR_PUNTER:
    case T5_CMD_CHART_IO_AXIS_MINOR_PUNTER:
    case T5_CMD_CHART_IO_SERIES_PIE_HEADING:
        {
        const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 1);
        PC_F64 p_f64 = (PC_F64) p0;
        assert(n_arglist_args(&arglist_handle) == 1);
        assert(p_args[0].type == (ARG_TYPE_F64 | ARG_MANDATORY));
        PTR_ASSERT(p_f64);
        if(NULL != p_f64)
            p_args[0].val.f64 = * p_f64;
        break;
        }

    case T5_CMD_CHART_IO_FILLSTYLEB:
        {
        const P_CHART_SAVE_INSTANCE p_chart_save_instance = chart_save_instance_goto(p_of_op_format);
        const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 2);
        GR_FILLSTYLEB style;
        BOOL using_default = gr_chart_objid_fillstyleb_query(cp, id, &style);

        if(using_default || !style.bits.manual)
            goto cancel_construct;

        PTR_ASSERT(p_chart_save_instance);
        assert(n_arglist_args(&arglist_handle) == 2);
        p_args[0].val.s32 = * (PC_S32) &style.bits;
        p_args[1].val.s32 = gr_fillstyleb_translate_pict_for_save(&style, p_chart_save_instance);

        break;
        }

    case T5_CMD_CHART_IO_FILLSTYLEC:
        {
        const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 4);
        GR_FILLSTYLEC style;
        BOOL using_default = gr_chart_objid_fillstylec_query(cp, id, &style);

        if(using_default || !style.fg.manual)
            goto cancel_construct;

        assert(n_arglist_args(&arglist_handle) == 4);
        p_args[0].val.u8n = (U8) style.fg.red;
        p_args[1].val.u8n = (U8) style.fg.green;
        p_args[2].val.u8n = (U8) style.fg.blue;
        if(style.fg.visible)
            arg_dispose(&arglist_handle, 3);
        else
            p_args[3].val.fBool = 1;

        break;
        }

    case T5_CMD_CHART_IO_LINESTYLE:
        {
        const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 6);
        GR_LINESTYLE style;
        BOOL using_default = gr_chart_objid_linestyle_query(cp, id, &style);

        if(using_default || !style.fg.manual)
            goto cancel_construct;

        assert(n_arglist_args(&arglist_handle) == 6);
        p_args[0].val.s32 = * (P_S32) &style.pattern;
        p_args[1].val.s32 = style.width;

        p_args[2].val.u8n = (U8) style.fg.red;
        p_args[3].val.u8n = (U8) style.fg.green;
        p_args[4].val.u8n = (U8) style.fg.blue;
        if(style.fg.visible)
            arg_dispose(&arglist_handle, 5);
        else
            p_args[5].val.fBool = 1;

        break;
        }

    case T5_CMD_CHART_IO_TEXTSTYLE:
        {
        const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 9);
        GR_TEXTSTYLE style;
        BOOL using_default = gr_chart_objid_textstyle_query(cp, id, &style);

        if(using_default || !style.fg.manual)
            goto cancel_construct;

        assert(n_arglist_args(&arglist_handle) == 9);
        p_args[0].val.s32 = style.height;
        p_args[1].val.s32 = style.width;
        status_assert(arg_alloc_tstr(&arglist_handle, 2, style.tstrFontName));
        p_args[3].val.fBool = style.bold;
        p_args[4].val.fBool = style.italic;

        p_args[5].val.u8n = (U8) style.fg.red;
        p_args[6].val.u8n = (U8) style.fg.green;
        p_args[7].val.u8n = (U8) style.fg.blue;
        if(style.fg.visible)
            arg_dispose(&arglist_handle, 8);
        else
            p_args[8].val.fBool = 1;

        break;
        }

    case T5_CMD_CHART_IO_BARCHSTYLE:
        {
        const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 2);
        GR_BARCHSTYLE style;
        BOOL using_default = gr_chart_objid_barchstyle_query(cp, id, &style);

        if(using_default || !style.bits.manual)
            goto cancel_construct;

        assert(n_arglist_args(&arglist_handle) == 2);
        assert(p_args[0].type == (ARG_TYPE_S32 | ARG_MANDATORY));
        assert(p_args[1].type == (ARG_TYPE_F64 | ARG_MANDATORY));
        p_args[0].val.s32 = * (PC_S32) &style.bits;
        p_args[1].val.f64 = style.slot_width_percentage;

        break;
        }

    case T5_CMD_CHART_IO_BARLINECHSTYLE:
        {
        const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 2);
        GR_BARLINECHSTYLE style;
        BOOL using_default = gr_chart_objid_barlinechstyle_query(cp, id, &style);

        if(using_default || !style.bits.manual)
            goto cancel_construct;

        assert(n_arglist_args(&arglist_handle) == 2);
        assert(p_args[0].type == (ARG_TYPE_S32 | ARG_MANDATORY));
        assert(p_args[1].type == (ARG_TYPE_F64 | ARG_MANDATORY));
        p_args[0].val.s32 = * (PC_S32) &style.bits;
        p_args[1].val.f64 = style.slot_depth_percentage;

        break;
        }

    case T5_CMD_CHART_IO_LINECHSTYLE:
        {
        const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 2);
        GR_LINECHSTYLE style;
        BOOL using_default = gr_chart_objid_linechstyle_query(cp, id, &style);

        if(using_default || !style.bits.manual)
            goto cancel_construct;

        assert(n_arglist_args(&arglist_handle) == 2);
        assert(p_args[0].type == (ARG_TYPE_S32 | ARG_MANDATORY));
        assert(p_args[1].type == (ARG_TYPE_F64 | ARG_MANDATORY));
        p_args[0].val.s32 = * (PC_S32) &style.bits;
        p_args[1].val.f64 = style.slot_width_percentage;

        break;
        }

    case T5_CMD_CHART_IO_PIECHDISPLSTYLE:
        {
        const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 2);
        GR_PIECHDISPLSTYLE style;
        BOOL using_default = gr_chart_objid_piechdisplstyle_query(cp, id, &style);

        if(using_default || !style.bits.manual)
            goto cancel_construct;

        assert(n_arglist_args(&arglist_handle) == 2);
        assert(p_args[0].type == (ARG_TYPE_S32 | ARG_MANDATORY));
        assert(p_args[1].type == (ARG_TYPE_F64 | ARG_MANDATORY));
        p_args[0].val.s32 = * (PC_S32) &style.bits;
        p_args[1].val.f64 = style.radial_displacement;

        break;
        }

    case T5_CMD_CHART_IO_PIECHLABELSTYLE:
        {
        const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 1);
        GR_PIECHLABELSTYLE style;
        BOOL using_default = gr_chart_objid_piechlabelstyle_query(cp, id, &style);

        if(using_default || !style.bits.manual)
            goto cancel_construct;

        assert(n_arglist_args(&arglist_handle) == 1);
        assert(p_args[0].type == (ARG_TYPE_S32 | ARG_MANDATORY));
        p_args[0].val.s32 = * (PC_S32) &style.bits;

        break;
        }

    case T5_CMD_CHART_IO_SCATCHSTYLE:
        {
        const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 2);
        GR_SCATCHSTYLE style;
        BOOL using_default = gr_chart_objid_scatchstyle_query(cp, id, &style);

        if(using_default || !style.bits.manual)
            goto cancel_construct;

        assert(n_arglist_args(&arglist_handle) == 2);
        assert(p_args[0].type == (ARG_TYPE_S32 | ARG_MANDATORY));
        assert(p_args[1].type == (ARG_TYPE_F64 | ARG_MANDATORY));
        p_args[0].val.s32 = * (PC_S32) &style.bits;
        p_args[1].val.f64 = style.width_percentage;

        break;
        }

    case T5_CMD_CHART_IO_TEXTPOS:
        {
        const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 4);
        GR_BOX box;

        gr_text_box_query(cp, id.no, &box);

        assert(n_arglist_args(&arglist_handle) == 4);
        assert(p_args[0].type == (ARG_TYPE_S32 | ARG_MANDATORY));
        assert(p_args[1].type == (ARG_TYPE_S32 | ARG_MANDATORY));
        assert(p_args[2].type == (ARG_TYPE_S32 | ARG_MANDATORY));
        assert(p_args[3].type == (ARG_TYPE_S32 | ARG_MANDATORY));
        p_args[0].val.s32 = box.x0;
        p_args[1].val.s32 = box.y0;
        p_args[2].val.s32 = box.x1;
        p_args[3].val.s32 = box.y1;

        break;
        }
    }

    /* need to flush id change first? */
    if(chart_save_id_out_pending)
        status_return(chart_save_id_now(cp, p_of_op_format));

    { /* output completed construct */
    STATUS status = ownform_save_arglist(arglist_handle, object_id, p_construct_table, p_of_op_format);
    arglist_dispose(&arglist_handle);
    return(status);
    } /*block*/

cancel_construct:;
    arglist_dispose(&arglist_handle);
    return(STATUS_OK);
}

T5_MSG_PROTO(static, chart_note_ensure_saved, _InoutRef_ P_NOTE_ENSURE_SAVED p_note_ensure_saved)
{
    const P_OF_OP_FORMAT p_of_op_format = p_note_ensure_saved->p_of_op_format;
    const P_CHART_HEADER p_chart_header = (P_CHART_HEADER) p_note_ensure_saved->object_data_ref;
    const OBJECT_ID object_id = OBJECT_ID_CHART;
    P_CHART_SAVE_INSTANCE p_chart_save_instance = chart_save_instance_goto(p_of_op_format);
    P_CHART_SAVE_MAP p_chart_save_map;
    STATUS status = STATUS_OK;

    IGNOREPARM_DocuRef_(p_docu);
    IGNOREPARM_InVal_(t5_message);

    if(NULL != p_chart_save_instance)
    {
        ARRAY_INDEX i = array_elements(&p_chart_save_instance->h_mapping_list);

        while(--i >= 0)
        {
            p_chart_save_map = array_ptr(&p_chart_save_instance->h_mapping_list, CHART_SAVE_MAP, i);

            if(p_chart_save_map->object_data_ref == p_note_ensure_saved->object_data_ref)
            {
                /* already saved */
                p_note_ensure_saved->extref = i;
                return(STATUS_OK);
            }
        }
    }
    else
    {
        if(NULL == (p_chart_save_instance = collect_add_entry_elem(CHART_SAVE_INSTANCE, &p_of_op_format->object_data_list, P_DATA_NONE, object_id, &status)))
            return(status);

        zero_struct_ptr(p_chart_save_instance); /* SKS 23feb2012 this was missing! */

        p_chart_save_instance->h_mapping_list = 0;
    }

    {
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(*p_chart_save_map), FALSE);
    if(NULL == (p_chart_save_map = al_array_extend_by(&p_chart_save_instance->h_mapping_list, CHART_SAVE_MAP, 1, &array_init_block, &status)))
        return(status);
    } /*block*/

    p_chart_save_map->object_data_ref = p_note_ensure_saved->object_data_ref;

    p_note_ensure_saved->extref = array_indexof_element(&p_chart_save_instance->h_mapping_list, CHART_SAVE_MAP, p_chart_save_map);

    {
    const T5_MESSAGE t5_message = T5_CMD_CHART_IO_1;
    PC_CONSTRUCT_TABLE p_construct_table;
    ARGLIST_HANDLE arglist_handle;
    if(status_ok(status = arglist_prepare_with_construct(&arglist_handle, object_id, t5_message, &p_construct_table)))
    {
        const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 1);
        p_args[0].val.s32 = p_note_ensure_saved->extref;
        status = ownform_save_arglist(arglist_handle, object_id, p_construct_table, p_of_op_format);
        arglist_dispose(&arglist_handle);
    }
    status_return(status);
}

#if 1
    status = chart_save_guts(p_of_op_format, p_chart_header);
#else
    { /* live text objects */
    ARRAY_INDEX i;

    for(i = 0; i < array_elements(&p_chart_header->h_elem); ++i)
    {
        const P_CHART_ELEMENT p_chart_element = array_ptr(&p_chart_header->h_elem, CHART_ELEMENT, i);

        switch(p_chart_element->bits.type)
        {
        default:
            continue;

        case CHART_RANGE_TXT:
            {
            S32 order = gr_chart_order_query(p_chart_header->ch, p_chart_element->gr_int_handle);
            break;
            }
        }
    }
    } /*block*/
#endif

    return(status);
}

_Check_return_
static STATUS
chart_save_chart(
    _ChartRef_  P_GR_CHART cp,
    _InoutRef_  P_OF_OP_FORMAT f)
{
    chart_load_save_objid = gr_chart_objid_chart;

    status_return(chart_save_id_now(cp, f));

    { /* blast over chart/plotarea/legend etc. direct pokers */
    T5_MESSAGE t5_message;
    for(t5_message = T5_CMD_CHART_IO_DIRECT_CHART_STT; t5_message < T5_CMD_CHART_IO_DIRECT_CHART_END; T5_MESSAGE_INCR(t5_message))
        status_return(chart_construct_save(cp, f, t5_message));
    } /*block*/

    status_return(chart_construct_save(cp, f, T5_CMD_CHART_IO_FILLSTYLEB));
    status_return(chart_construct_save(cp, f, T5_CMD_CHART_IO_FILLSTYLEC));
    status_return(chart_construct_save(cp, f, T5_CMD_CHART_IO_LINESTYLE));
    status_return(chart_construct_save(cp, f, T5_CMD_CHART_IO_TEXTSTYLE));

    return(STATUS_OK);
}

_Check_return_
static STATUS
chart_save_id(
    _ChartRef_  P_GR_CHART cp,
    _InoutRef_  P_OF_OP_FORMAT f)
{
    status_assert(chart_io_id_changed(cp, TRUE)); /* can't go wrong on save */

    switch(chart_save_id_out_pending)
    {
    case 0:
        chart_save_id_out_pending = 1;
        return(STATUS_OK);

    default:
    case 1:
        /* no-one used that id for anything so ignore it, keep this new id pending */
        return(STATUS_OK);

    case 2:
        chart_save_id_out_pending = 0;
        break;
    }

    {
    const OBJECT_ID object_id = OBJECT_ID_CHART;
    PC_CONSTRUCT_TABLE p_construct_table;
    ARGLIST_HANDLE arglist_handle;
    STATUS status;

    if(status_ok(status = arglist_prepare_with_construct(&arglist_handle, object_id, T5_CMD_CHART_IO_OBJID, &p_construct_table)))
    {
        const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 3);
        const GR_CHART_OBJID id = chart_load_save_objid;

        assert(p_args[2].type == ARG_TYPE_S32);
        if(id.has_subno)
            p_args[2].val.s32 = id.subno;
        else
            arg_dispose(&arglist_handle, 2);

        assert(p_args[1].type == ARG_TYPE_S32);
        if(id.has_no)
            p_args[1].val.s32 = id.no;
        else
            arg_dispose(&arglist_handle, 1);

        assert(p_args[0].type == (ARG_TYPE_S32 | ARG_MANDATORY));
        p_args[0].val.s32 = id.name;

        status = ownform_save_arglist(arglist_handle, object_id, p_construct_table, f);

        arglist_dispose(&arglist_handle);
    }

    return(status);
    } /*block*/
}

_Check_return_
static STATUS
chart_save_id_now(
    _ChartRef_  P_GR_CHART cp,
    _InoutRef_  P_OF_OP_FORMAT f)
{
    chart_save_id_out_pending = 2;

    return(chart_save_id(cp, f));
}

_Check_return_
static STATUS
chart_save_legend(
    _ChartRef_  P_GR_CHART cp,
    _InoutRef_  P_OF_OP_FORMAT f)
{
    STATUS status = STATUS_OK;

    if(cp->legend.bits.on)
    {
        chart_load_save_objid = gr_chart_objid_legend;

        if(status_ok(status = chart_save_id(cp, f)))
        if(status_ok(status = chart_construct_save(cp, f, T5_CMD_CHART_IO_FILLSTYLEB)))
        if(status_ok(status = chart_construct_save(cp, f, T5_CMD_CHART_IO_FILLSTYLEC)))
        if(status_ok(status = chart_construct_save(cp, f, T5_CMD_CHART_IO_LINESTYLE)))
                     status = chart_construct_save(cp, f, T5_CMD_CHART_IO_TEXTSTYLE);
    }

    return(status);
}

_Check_return_
static STATUS
chart_save_plotareas(
    _ChartRef_  P_GR_CHART cp,
    _InoutRef_  P_OF_OP_FORMAT f)
{
    STATUS status = STATUS_OK;

    gr_chart_objid_clear(&chart_load_save_objid);
    chart_load_save_objid.name = GR_CHART_OBJNAME_PLOTAREA;
    chart_load_save_objid.has_no = 1;

    {
    S32 plotidx;

    for(plotidx = 0; plotidx < (cp->d3.bits.use ? GR_CHART_N_PLOTAREAS : 1); ++plotidx)
    {
        chart_load_save_objid.no = (UBF) plotidx;

        status_break(status = chart_save_id(cp, f));
        status_break(status = chart_construct_save(cp, f, T5_CMD_CHART_IO_FILLSTYLEB));
        status_break(status = chart_construct_save(cp, f, T5_CMD_CHART_IO_FILLSTYLEC));
        status_break(status = chart_construct_save(cp, f, T5_CMD_CHART_IO_LINESTYLE));
    }
    } /*block*/

    return(status);
}

/******************************************************************************
*
* save out data on this series and its points
*
******************************************************************************/

typedef struct GR_CHART_SAVE_POINT_DATA_LIST
{
    GR_LIST_ID list_id;
    T5_MESSAGE t5_message;
    LIST_ITEMNO key;
    struct GR_CHART_SAVE_POINT_DATA_LIST_BITS
    {
        UBF def_save : 1;
        UBF cur_save : 1;
    } bits;
}
GR_CHART_SAVE_POINT_DATA_LIST, * P_GR_CHART_SAVE_POINT_DATA_LIST;

typedef struct GR_CHART_SAVE_POINT_DATA
{
    P_GR_CHART_SAVE_POINT_DATA_LIST list;
    U32 list_n;
}
GR_CHART_SAVE_POINT_DATA, * P_GR_CHART_SAVE_POINT_DATA;

static /*poked*/ struct GR_CHART_SAVE_POINT_DATA_LIST
chart_save_pdrop_data_list[] =
{
    { GR_LIST_PDROP_FILLSTYLEB,     T5_CMD_CHART_IO_FILLSTYLEB,     0, 0 },
    { GR_LIST_PDROP_FILLSTYLEC,     T5_CMD_CHART_IO_FILLSTYLEC,     0, 0 },
    { GR_LIST_PDROP_LINESTYLE,      T5_CMD_CHART_IO_LINESTYLE,      0, 0 }
};

static struct GR_CHART_SAVE_POINT_DATA
chart_save_pdrop_data = { chart_save_pdrop_data_list, elemof32(chart_save_pdrop_data_list) };

static /*poked*/ struct GR_CHART_SAVE_POINT_DATA_LIST
chart_save_point_data_list[] =
{
    { GR_LIST_POINT_FILLSTYLEB,      T5_CMD_CHART_IO_FILLSTYLEB,      0, 1 },
    { GR_LIST_POINT_FILLSTYLEC,      T5_CMD_CHART_IO_FILLSTYLEC,      0, 1 },
    { GR_LIST_POINT_LINESTYLE,       T5_CMD_CHART_IO_LINESTYLE,       0, 1 },
    { GR_LIST_POINT_TEXTSTYLE,       T5_CMD_CHART_IO_TEXTSTYLE,       0, 1 },

    { GR_LIST_POINT_BARCHSTYLE,      T5_CMD_CHART_IO_BARCHSTYLE,      0, 0 },
    { GR_LIST_POINT_BARLINECHSTYLE,  T5_CMD_CHART_IO_BARLINECHSTYLE,  0, 0 },
    { GR_LIST_POINT_LINECHSTYLE,     T5_CMD_CHART_IO_LINECHSTYLE,     0, 0 },
    { GR_LIST_POINT_PIECHDISPLSTYLE, T5_CMD_CHART_IO_PIECHDISPLSTYLE, 0, 0 },
    { GR_LIST_POINT_PIECHLABELSTYLE, T5_CMD_CHART_IO_PIECHLABELSTYLE, 0, 0 },
    { GR_LIST_POINT_SCATCHSTYLE,     T5_CMD_CHART_IO_SCATCHSTYLE,     0, 0 }
};

static struct GR_CHART_SAVE_POINT_DATA
chart_save_point_data = { chart_save_point_data_list, elemof32(chart_save_point_data_list) };

static void
chart_save_points_init_save_bits(
    P_GR_CHART_SAVE_POINT_DATA p_gr_chart_save_point_data)
{
    U32 ix;

    /* clear bits down */
    for(ix = 0; ix < p_gr_chart_save_point_data->list_n; ++ix)
        p_gr_chart_save_point_data->list[ix].bits.cur_save = p_gr_chart_save_point_data->list[ix].bits.def_save;
}

static void
chart_save_points_set_save_bit(
    P_GR_CHART_SAVE_POINT_DATA p_gr_chart_save_point_data,
    _InVal_     T5_MESSAGE t5_message)
{
    U32 ix;

    /* set bit for saving */
    for(ix = 0; ix < p_gr_chart_save_point_data->list_n; ++ix)
        if(p_gr_chart_save_point_data->list[ix].t5_message == t5_message)
            p_gr_chart_save_point_data->list[ix].bits.cur_save = 1;
}

_Check_return_
static STATUS
chart_save_points(
    _ChartRef_  P_GR_CHART cp,
    _InoutRef_  P_OF_OP_FORMAT f,
    _InVal_     GR_SERIES_IDX series_idx,
    P_GR_CHART_SAVE_POINT_DATA p_gr_chart_save_point_data)
{
    LIST_ITEMNO max_key = gr_point_key_from_external(GR_CHART_OBJID_SUBNO_MAX) + 1; /* off the end of saveable range */
    S32 first = 1;

    { /* clear keys down, end jamming ones that aren't being saved */
    U32 ix;
    for(ix = 0; ix < p_gr_chart_save_point_data->list_n; ++ix)
        p_gr_chart_save_point_data->list[ix].key = p_gr_chart_save_point_data->list[ix].bits.cur_save ? 0 : max_key;
    } /*block*/

    /* loop over points together */
    for(;;)
    {
        LIST_ITEMNO key = max_key;

        { /* loop over all in list to find next common key */
        U32 ix;
        for(ix = 0; ix < p_gr_chart_save_point_data->list_n; ++ix)
        {
            if(p_gr_chart_save_point_data->list[ix].key != max_key)
            {
                P_ANY p_any;

                if(first)
                    p_any = _gr_point_list_first(cp, series_idx, &p_gr_chart_save_point_data->list[ix].key, p_gr_chart_save_point_data->list[ix].list_id);
                else
                    p_any = _gr_point_list_next(cp, series_idx, &p_gr_chart_save_point_data->list[ix].key, p_gr_chart_save_point_data->list[ix].list_id);

                if(NULL == p_any)
                {
                    /* ensure key becomes end jammed if point data not found */
                    p_gr_chart_save_point_data->list[ix].key = max_key;
                }
                else
                {
                    key = MIN(key, p_gr_chart_save_point_data->list[ix].key);
                }
            }
        }
        } /*block*/

        first = 0;

        /* no more points */
        if(key >= max_key)
            break;

        chart_load_save_objid.subno = (U16) gr_point_external_from_key(key);

        status_return(chart_save_id(cp, f));

        { /* save out all points which have stopped at this key, restarting all else from this key unless end jammed */
        U32 ix;
        for(ix = 0; ix < p_gr_chart_save_point_data->list_n; ++ix)
        {
            if(p_gr_chart_save_point_data->list[ix].key != key)
            {
                if(p_gr_chart_save_point_data->list[ix].key != max_key)
                    p_gr_chart_save_point_data->list[ix].key = key;

                continue;
            }

            status_return(chart_construct_save(cp, f, p_gr_chart_save_point_data->list[ix].t5_message));
        }
        } /*block*/
    }

    return(1);
}

_Check_return_
static STATUS
chart_save_points_and_pdrops(
    _ChartRef_  P_GR_CHART cp,
    _InoutRef_  P_OF_OP_FORMAT f,
    _InVal_     GR_SERIES_IDX series_idx)
{
    chart_load_save_objid.has_subno = 1;

    chart_load_save_objid.name = GR_CHART_OBJNAME_POINT;
    status_return(chart_save_points(cp, f, series_idx, &chart_save_point_data));

    chart_load_save_objid.name = GR_CHART_OBJNAME_DROPPOINT;
    return(chart_save_points(cp, f, series_idx, &chart_save_pdrop_data));
}

_Check_return_
static STATUS
chart_save_series(
    _ChartRef_  P_GR_CHART cp,
    _InoutRef_  P_OF_OP_FORMAT f,
    _InVal_     GR_SERIES_IDX series_idx)
{
    gr_chart_objid_from_series_idx(cp, series_idx, &chart_load_save_objid);

    status_return(chart_save_id_now(cp, f));

    { /* blast over series direct pokers */
    T5_MESSAGE t5_message;
    for(t5_message = T5_CMD_CHART_IO_DIRECT_SERIES_STT; t5_message < T5_CMD_CHART_IO_DIRECT_SERIES_END; T5_MESSAGE_INCR(t5_message))
        status_return(chart_construct_save(cp, f, t5_message));
    } /*block*/

    status_return(chart_construct_save(cp, f, T5_CMD_CHART_IO_FILLSTYLEB));
    status_return(chart_construct_save(cp, f, T5_CMD_CHART_IO_FILLSTYLEC));
    status_return(chart_construct_save(cp, f, T5_CMD_CHART_IO_LINESTYLE));
    status_return(chart_construct_save(cp, f, T5_CMD_CHART_IO_TEXTSTYLE));

    {
    P_GR_SERIES serp;
    GR_CHART_TYPE chart_type;

    serp = getserp(cp, series_idx);

    chart_type = cp->axes[0].chart_type;

    switch(chart_type)
    {
    case GR_CHART_TYPE_PIE:
    case GR_CHART_TYPE_SCAT:
        break;

    default: default_unhandled();
#if CHECKING
    case GR_CHART_TYPE_BAR:
    case GR_CHART_TYPE_LINE:
#endif
        chart_type = cp->axes[chart_load_save_use_axes_idx].chart_type;

        if(chart_type != GR_CHART_TYPE_LINE)
            chart_type = GR_CHART_TYPE_BAR;

        if(serp->chart_type != GR_CHART_TYPE_NONE)
            chart_type = serp->chart_type;

        break;
    }

    chart_save_points_init_save_bits(&chart_save_point_data);

    switch(chart_type)
    {
    case GR_CHART_TYPE_PIE:
        status_return(chart_construct_save(cp, f, T5_CMD_CHART_IO_PIECHDISPLSTYLE));
        status_return(chart_construct_save(cp, f, T5_CMD_CHART_IO_PIECHLABELSTYLE));

        chart_save_points_set_save_bit(&chart_save_point_data, T5_CMD_CHART_IO_PIECHDISPLSTYLE);
        chart_save_points_set_save_bit(&chart_save_point_data, T5_CMD_CHART_IO_PIECHLABELSTYLE);
        break;

    case GR_CHART_TYPE_BAR:
        status_return(chart_construct_save(cp, f, T5_CMD_CHART_IO_BARCHSTYLE));
        status_return(chart_construct_save(cp, f, T5_CMD_CHART_IO_BARLINECHSTYLE));

        chart_save_points_set_save_bit(&chart_save_point_data, T5_CMD_CHART_IO_BARCHSTYLE);
        chart_save_points_set_save_bit(&chart_save_point_data, T5_CMD_CHART_IO_BARLINECHSTYLE);
        break;

    case GR_CHART_TYPE_LINE:
        status_return(chart_construct_save(cp, f, T5_CMD_CHART_IO_LINECHSTYLE));
        status_return(chart_construct_save(cp, f, T5_CMD_CHART_IO_BARLINECHSTYLE));

        chart_save_points_set_save_bit(&chart_save_point_data, T5_CMD_CHART_IO_LINECHSTYLE);
        chart_save_points_set_save_bit(&chart_save_point_data, T5_CMD_CHART_IO_BARLINECHSTYLE);
        break;

    case GR_CHART_TYPE_SCAT:
        status_return(chart_construct_save(cp, f, T5_CMD_CHART_IO_SCATCHSTYLE));

        chart_save_points_set_save_bit(&chart_save_point_data, T5_CMD_CHART_IO_SCATCHSTYLE);
        break;
    }

    /*
    drop lines
    */

    chart_save_points_init_save_bits(&chart_save_pdrop_data);

    switch(chart_type)
    {
    case GR_CHART_TYPE_PIE:
        break;

    case GR_CHART_TYPE_LINE:
        chart_load_save_objid.name = GR_CHART_OBJNAME_DROPSERIES;

        status_return(chart_save_id(cp, f));

        status_return(chart_construct_save(cp, f, T5_CMD_CHART_IO_FILLSTYLEB));
        status_return(chart_construct_save(cp, f, T5_CMD_CHART_IO_FILLSTYLEC));
        status_return(chart_construct_save(cp, f, T5_CMD_CHART_IO_LINESTYLE));

        chart_save_points_set_save_bit(&chart_save_pdrop_data, T5_CMD_CHART_IO_FILLSTYLEB);
        chart_save_points_set_save_bit(&chart_save_pdrop_data, T5_CMD_CHART_IO_FILLSTYLEC);
        chart_save_points_set_save_bit(&chart_save_pdrop_data, T5_CMD_CHART_IO_LINESTYLE);

        /*FALLTHRU*/

    default:
    case GR_CHART_TYPE_SCAT:
    case GR_CHART_TYPE_BAR:
        chart_load_save_objid.name = GR_CHART_OBJNAME_BESTFITSER;

        status_return(chart_save_id(cp, f)); /* SKS after 1.05 20oct93 - was missing in PD4 too */

        status_return(chart_construct_save(cp, f, T5_CMD_CHART_IO_LINESTYLE));
        break;
    }
    } /*block*/

    /*
    point data
    */

    return(chart_save_points_and_pdrops(cp, f, series_idx));
}

_Check_return_
static STATUS
chart_save_axis(
    _ChartRef_  P_GR_CHART cp,
    _InoutRef_  P_OF_OP_FORMAT f,
    _InVal_     GR_AXES_IDX axes_idx,
    _InVal_     GR_AXIS_IDX axis_idx)
{
    gr_chart_objid_from_axes_idx(cp, axes_idx, axis_idx, &chart_load_save_objid);

    status_return(chart_save_id_now(cp, f));

    { /* blast over axis direct pokers */
    T5_MESSAGE t5_message;
    for(t5_message = T5_CMD_CHART_IO_DIRECT_AXIS_STT; t5_message < T5_CMD_CHART_IO_DIRECT_AXIS_END; T5_MESSAGE_INCR(t5_message))
        status_return(chart_construct_save(cp, f, t5_message));
    } /*block*/

    status_return(chart_construct_save(cp, f, T5_CMD_CHART_IO_LINESTYLE));
    status_return(chart_construct_save(cp, f, T5_CMD_CHART_IO_TEXTSTYLE));

    {
    U32 mmix;

    for(mmix = GR_CHART_AXISTICK_MAJOR; mmix <= GR_CHART_AXISTICK_MINOR; ++mmix)
    {
        gr_chart_objid_from_axis_grid(&chart_load_save_objid, (mmix == GR_CHART_AXISTICK_MAJOR));

        status_return(chart_save_id(cp, f));
        status_return(chart_construct_save(cp, f, T5_CMD_CHART_IO_LINESTYLE));

        gr_chart_objid_from_axis_tick(&chart_load_save_objid, (mmix == GR_CHART_AXISTICK_MAJOR));

        status_return(chart_save_id(cp, f));
        status_return(chart_construct_save(cp, f, T5_CMD_CHART_IO_LINESTYLE));
    }
    } /*block*/

    return(1);
}

_Check_return_
static STATUS
chart_save_axes(
    _ChartRef_  P_GR_CHART cp,
    _InoutRef_  P_OF_OP_FORMAT f,
    _InVal_     GR_AXES_IDX axes_idx)
{
    gr_chart_objid_from_axes_idx(cp, axes_idx, 0, &chart_load_save_objid);

    status_return(chart_save_id_now(cp, f));

    { /* blast over axes direct pokers */
    T5_MESSAGE t5_message;
    for(t5_message = T5_CMD_CHART_IO_DIRECT_AXES_STT; t5_message < T5_CMD_CHART_IO_DIRECT_AXES_END; T5_MESSAGE_INCR(t5_message))
        status_return(chart_construct_save(cp, f, t5_message));
    } /*block*/

    { /* save default styles from axes */
    GR_CHART_TYPE chart_type = cp->axes[0].chart_type;

    switch(chart_type)
    {
    case GR_CHART_TYPE_PIE:
    case GR_CHART_TYPE_SCAT:
        break;

    default:
    case GR_CHART_TYPE_BAR:
    case GR_CHART_TYPE_LINE:
        chart_type = cp->axes[chart_load_save_use_axes_idx].chart_type;

        if(chart_type != GR_CHART_TYPE_LINE)
            chart_type = GR_CHART_TYPE_BAR;

        break;
    }

    switch(chart_type)
    {
    case GR_CHART_TYPE_PIE:
        status_return(chart_construct_save(cp, f, T5_CMD_CHART_IO_PIECHDISPLSTYLE));
        status_return(chart_construct_save(cp, f, T5_CMD_CHART_IO_PIECHLABELSTYLE));
        break;

    case GR_CHART_TYPE_BAR:
        status_return(chart_construct_save(cp, f, T5_CMD_CHART_IO_BARCHSTYLE));
        status_return(chart_construct_save(cp, f, T5_CMD_CHART_IO_BARLINECHSTYLE));
        break;

    case GR_CHART_TYPE_LINE:
        status_return(chart_construct_save(cp, f, T5_CMD_CHART_IO_LINECHSTYLE));
        status_return(chart_construct_save(cp, f, T5_CMD_CHART_IO_BARLINECHSTYLE));
        break;

    case GR_CHART_TYPE_SCAT:
        status_return(chart_construct_save(cp, f, T5_CMD_CHART_IO_SCATCHSTYLE));
        break;
    }
    } /*block*/

    return(STATUS_OK);
}

static S32
chart_save_texts(
    _ChartRef_  P_GR_CHART cp,
    _InoutRef_  P_OF_OP_FORMAT f)
{
    LIST_ITEMNO key;
    P_GR_TEXT t;
    STATUS status = STATUS_OK;

    for(t = collect_first(GR_TEXT, &cp->text.lbr, &key);
        t;
        t = collect_next(GR_TEXT, &cp->text.lbr, &key))
    {
        if(t->bits.unused)
            continue;

        gr_chart_objid_from_text(key, &chart_load_save_objid);

        status_break(status = chart_save_id(cp, f));

        status_break(status = chart_construct_save(cp, f, T5_CMD_CHART_IO_TEXTPOS));
        status_break(status = chart_construct_save(cp, f, T5_CMD_CHART_IO_TEXTSTYLE));

        /* we have to add static texts; live texts will be added by punter reload */
        if(!t->bits.live_text)
        {
            const OBJECT_ID object_id = OBJECT_ID_CHART;
            PC_CONSTRUCT_TABLE p_construct_table;
            ARGLIST_HANDLE arglist_handle;

            if(status_ok(status = arglist_prepare_with_construct(&arglist_handle, object_id, T5_CMD_CHART_IO_TEXTCONTENTS, &p_construct_table)))
            {
                P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 1);
                PC_USTR ustr = (PC_USTR) (t + 1);

                assert(n_arglist_args(&arglist_handle) == 1);
                assert(p_args[0].type == (ARG_TYPE_USTR | ARG_MANDATORY));
                p_args[0].val.ustr = ustr;

                status = ownform_save_arglist(arglist_handle, object_id, p_construct_table, f);

                arglist_dispose(&arglist_handle);
            }

            status_break(status);
        }
    }

    return(status);
}

_Check_return_
static STATUS
chart_save_datasources_for_series_idx(
    _ChartRef_  P_GR_CHART cp,
    _InoutRef_  P_OF_OP_FORMAT f,
    _InVal_     GR_SERIES_IDX series_idx)
{
    GR_DATASOURCE_FOURSOME gr_datasource_foursome;

    gr_get_datasources(cp, series_idx, &gr_datasource_foursome, TRUE);

    {
    const OBJECT_ID object_id = OBJECT_ID_CHART;
    PC_CONSTRUCT_TABLE p_construct_table;
    ARGLIST_HANDLE arglist_handle;
    P_ARGLIST_ARG p_args;
    STATUS status;
    U32 state;

    status_return(arglist_prepare_with_construct(&arglist_handle, object_id, T5_CMD_CHART_IO_3, &p_construct_table));

    p_args = p_arglist_args(&arglist_handle, 2 + 4*4);

    p_args[0].val.fBool = FALSE; /* initially; this is modified lower down */
    p_args[1].val.s32   = getserp(cp, series_idx)->sertype;

    for(state = 0; state < 4; ++state)
    {
        U32 arglist_base_index = 2 + state * 4 /*c,r,c,r*/;
        GR_DATASOURCE_HANDLE dsh;
        P_GR_DATASOURCE p_gr_datasource;

        switch(state)
        {
        default:
        case 0: dsh = gr_datasource_foursome.value_y; break;
        case 1: dsh = gr_datasource_foursome.value_x; break;
        case 2: dsh = gr_datasource_foursome.error_y; break;
        case 3: dsh = gr_datasource_foursome.error_x; break;
        }

        if(dsh == GR_DATASOURCE_HANDLE_NONE)
        {
            arg_dispose(&arglist_handle, arglist_base_index + 0);
            arg_dispose(&arglist_handle, arglist_base_index + 1);
            arg_dispose(&arglist_handle, arglist_base_index + 2);
            arg_dispose(&arglist_handle, arglist_base_index + 3);
        }
        else
        {
            P_CHART_ELEMENT p_chart_element;

            p_gr_datasource = gr_chart_datasource_p_from_h(cp, dsh);
            PTR_ASSERT(p_gr_datasource);

            p_chart_element = (P_CHART_ELEMENT) p_gr_datasource->ext_handle;

            p_args[0].val.fBool = p_chart_element->bits.label_first_item;

            p_args[arglist_base_index + 0].val.col = p_chart_element->region.tl.col;
            p_args[arglist_base_index + 1].val.row = p_chart_element->region.tl.row;
            p_args[arglist_base_index + 2].val.col = p_chart_element->region.br.col;
            p_args[arglist_base_index + 3].val.row = p_chart_element->region.br.row;
        }
    }

    status = ownform_save_arglist(arglist_handle, object_id, p_construct_table, f);
    arglist_dispose(&arglist_handle);
    return(status);
    } /*block*/
}

_Check_return_
static STATUS
chart_save_datasources(
    _ChartRef_  P_GR_CHART cp,
    _InoutRef_  P_OF_OP_FORMAT f)
{
    STATUS status = STATUS_OK;

    /* category labels / scatter x common datasource? */
    if(cp->core.category_datasource.dsh != GR_DATASOURCE_HANDLE_NONE)
    {
        const P_GR_DATASOURCE p_gr_datasource = &cp->core.category_datasource;
        const P_CHART_ELEMENT p_chart_element = (P_CHART_ELEMENT) p_gr_datasource->ext_handle;

        {
        const OBJECT_ID object_id = OBJECT_ID_CHART;
        PC_CONSTRUCT_TABLE p_construct_table;
        ARGLIST_HANDLE arglist_handle;

        if(status_ok(status = arglist_prepare_with_construct(&arglist_handle, object_id, T5_CMD_CHART_IO_2, &p_construct_table)))
        {
            const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 5);

            assert(n_arglist_args(&arglist_handle) == 5);
            p_args[0].val.fBool = p_chart_element->bits.label_first_item;
            p_args[1].val.col = p_chart_element->region.tl.col;
            p_args[2].val.row = p_chart_element->region.tl.row;
            p_args[3].val.col = p_chart_element->region.br.col;
            p_args[4].val.row = p_chart_element->region.br.row;

            status = ownform_save_arglist(arglist_handle, object_id, p_construct_table, f);

            arglist_dispose(&arglist_handle);
        }

        status_return(status);
        } /*block*/
    }

    switch(cp->axes[0].chart_type)
    {
    case GR_CHART_TYPE_PIE:
        {
        /* just the one series, just Y data */
        status = chart_save_datasources_for_series_idx(cp, f, 0);
        break;
        }

    default: default_unhandled();
#if CHECKING
    case GR_CHART_TYPE_SCAT:
    case GR_CHART_TYPE_BAR:
    case GR_CHART_TYPE_LINE:
#endif
        {
        GR_AXES_IDX axes_idx;

        for(axes_idx = 0; axes_idx <= cp->axes_idx_max; ++axes_idx)
        {
            GR_SERIES_IDX series_idx;

            for(series_idx = cp->axes[axes_idx].series.stt_idx; series_idx < cp->axes[axes_idx].series.end_idx; ++series_idx)
                status_break(status = chart_save_datasources_for_series_idx(cp, f, series_idx));
        }

        break;
        }
    }

    return(status);
}

/******************************************************************************
*
* save out gr_chart's information
*
******************************************************************************/

_Check_return_
static STATUS
chart_save_guts(
    _InoutRef_  P_OF_OP_FORMAT f,
    P_CHART_HEADER p_chart_header)
{
    const P_GR_CHART cp = p_gr_chart_from_chart_handle(p_chart_header->ch);

    chart_save_id_out_pending = 0;

    status_return(gr_fillstyleb_table_make_for_save(cp, f));

    status_return(chart_save_datasources(cp, f));
    status_return(chart_save_chart(cp, f));
    status_return(chart_save_plotareas(cp, f));
    status_return(chart_save_legend(cp, f));

    switch(cp->axes[0].chart_type)
    {
    case GR_CHART_TYPE_PIE:
        {
        /* save axes 0 info */
        status_return(chart_save_axes(cp, f, 0));

        /* no axis info to save */

        /* just the one series */
        status_return(chart_save_series(cp, f, 0));

        break;
        }

    case GR_CHART_TYPE_SCAT:
        {
        GR_AXES_IDX axes_idx;

        for(axes_idx = 0; axes_idx <= cp->axes_idx_max; ++axes_idx)
        {
            GR_AXIS_IDX axis_idx;
            GR_SERIES_IDX series_idx;

            status_return(chart_save_axes(cp, f, axes_idx));

            /* output value X and Y axes */
            for(axis_idx = 0; axis_idx <= 1; ++axis_idx)
                status_return(chart_save_axis(cp, f, axes_idx, axis_idx));

            for(series_idx = cp->axes[axes_idx].series.stt_idx; series_idx < cp->axes[axes_idx].series.end_idx; ++series_idx)
                status_return(chart_save_series(cp, f, series_idx));

        }

        for(axes_idx = 1; axes_idx <= cp->axes_idx_max; ++axes_idx)
        {
            /* save out overlay split after all series done */
            gr_chart_objid_from_axes_idx(cp, axes_idx, 0, &chart_load_save_objid);

            status_return(chart_save_id_now(cp, f));

            status_return(chart_construct_save(cp, f, T5_CMD_CHART_IO_AXES_START_SERIES));
        }

        break;
        }

    default: default_unhandled();
#if CHECKING
    case GR_CHART_TYPE_BAR:
    case GR_CHART_TYPE_LINE:
#endif
        {
        GR_AXES_IDX axes_idx;

        for(axes_idx = 0; axes_idx <= cp->axes_idx_max; ++axes_idx)
        {
            GR_AXIS_IDX axis_idx;
            GR_SERIES_IDX series_idx;

            status_return(chart_save_axes(cp, f, axes_idx));

            /* output category X and 1 or 2 value Y axes.
             * ignore Z axes as they are currently irrelevant
            */
            for(axis_idx = 0; axis_idx <= 1; ++axis_idx)
            {
                /* it is pointless outputting the second, identical, category axis */
                if((axis_idx == 0) && (axes_idx == 1))
                    continue;

                status_return(chart_save_axis(cp, f, axes_idx, axis_idx));
            }

            for(series_idx = cp->axes[axes_idx].series.stt_idx; series_idx < cp->axes[axes_idx].series.end_idx; ++series_idx)
                status_return(chart_save_series(cp, f, series_idx));
        }

        for(axes_idx = 1; axes_idx <= cp->axes_idx_max; ++axes_idx)
        {
            /* save out overlay split after all series done */
            gr_chart_objid_from_axes_idx(cp, axes_idx, 0, &chart_load_save_objid);

            status_return(chart_save_id_now(cp, f));

            status_return(chart_construct_save(cp, f, T5_CMD_CHART_IO_AXES_START_SERIES));
        }

        break;
        }
    }

    return(chart_save_texts(cp, f));
}

T5_CMD_PROTO(static, t5_cmd_chart_io_1)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 1);
    P_CHART_LOAD_INSTANCE p_chart_load_instance = chart_load_instance_goto(p_t5_cmd->p_of_ip_format);
    P_CHART_LOAD_MAP p_chart_load_map = NULL;
    P_CHART_HEADER p_chart_header = NULL;
    STATUS status;

    IGNOREPARM_InVal_(t5_message);

    for(;;) /* loop for structure */
    {
        if(NULL == p_chart_load_instance)
        {
            if(NULL == (p_chart_load_instance = collect_add_entry_elem(CHART_LOAD_INSTANCE, &p_t5_cmd->p_of_ip_format->object_data_list, P_DATA_NONE, OBJECT_ID_CHART, &status)))
            {
                status = create_error(CHART_ERR_LOAD_NO_MEM);
                break;
            }

            p_chart_load_instance->h_mapping_list = 0;

            list_init(&p_chart_load_instance->fillstyleb_translation_table);
        }

        {
        SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(*p_chart_load_map), TRUE);

        if(NULL == (p_chart_load_map = al_array_extend_by(&p_chart_load_instance->h_mapping_list, CHART_LOAD_MAP, 1, &array_init_block, &status)))
        {
            assert(STATUS_NOMEM == status);
            status = create_error(CHART_ERR_LOAD_NO_MEM);
            break;
        }

        p_chart_load_map->extref = p_args[0].val.s32; /* extref */
        } /*block*/

        status_break(status = chart_new(&p_chart_header, 0, 0));

        p_chart_header->docno = docno_from_p_docu(p_docu);

        p_chart_load_map->p_chart_header = p_chart_header;

        p_chart_load_instance->this_p_chart_header = p_chart_header;

        chart_load_p_gr_chart = p_gr_chart_from_chart_handle(p_chart_header->ch);

        chart_modify(p_chart_header);

        return(STATUS_OK);
        /*NOTREACHED*/
    }

    if(NULL != p_chart_header)
        chart_dispose(&p_chart_header);

    if(NULL != p_chart_load_map)
        al_array_shrink_by(&p_chart_load_instance->h_mapping_list, -1);

    /* don't care about the load instance data, he'll be freed at load process end (and may already have valid entries) */

    /* report errors locally and keep loading the file, but dependent notes should blow */
    reperr_null(status);

    return(STATUS_OK);
}

T5_CMD_PROTO(static, t5_cmd_chart_io_2)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 5);
    const P_CHART_LOAD_INSTANCE p_chart_load_instance = chart_load_instance_goto(p_t5_cmd->p_of_ip_format);
    P_CHART_HEADER p_chart_header;
    CHART_SHAPEDESC chart_shapedesc;

    IGNOREPARM_DocuRef_(p_docu);
    IGNOREPARM_InVal_(t5_message);

    PTR_ASSERT(p_chart_load_instance);

    p_chart_header = p_chart_load_instance->this_p_chart_header;

    zero_struct(chart_shapedesc);
    chart_shapedesc.docno = docno_from_p_docu(p_docu);
    chart_shapedesc.bits.label_first_range = 1;
    chart_shapedesc.bits.label_first_item = p_args[0].val.u8n;
    status_return(chart_element_ensure(p_chart_header, 1));
    chart_shapedesc.region.tl.col = p_args[1].val.col;
    chart_shapedesc.region.tl.row = p_args[2].val.row;
    chart_shapedesc.region.br.col = p_args[3].val.col;
    chart_shapedesc.region.br.row = p_args[4].val.row;
    chart_shapedesc.bits.range_over_columns = (U8) ( (chart_shapedesc.region.br.row - chart_shapedesc.region.tl.row) >=
                                                     (chart_shapedesc.region.br.col - chart_shapedesc.region.tl.col) );
    return(chart_element(p_chart_header, &chart_shapedesc));
}

T5_CMD_PROTO(static, t5_cmd_chart_io_3)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 2 + 4*4);
    const P_CHART_LOAD_INSTANCE p_chart_load_instance = chart_load_instance_goto(p_t5_cmd->p_of_ip_format);
    P_CHART_HEADER p_chart_header;
    CHART_SHAPEDESC chart_shapedesc;
    GR_SERIES_TYPE sertype;

    IGNOREPARM_InVal_(t5_message);

    PTR_ASSERT(p_chart_load_instance);

    p_chart_header = p_chart_load_instance->this_p_chart_header;

    zero_struct(chart_shapedesc);
    chart_shapedesc.docno = docno_from_p_docu(p_docu);
    chart_shapedesc.bits.label_first_range = 0;
    chart_shapedesc.bits.label_first_item = p_args[0].val.u8n;
    status_return(chart_element_ensure(p_chart_header, 4)); /* overkill, but doesn't matter */
    sertype = p_args[1].val.s32;

    /* x data */
    chart_shapedesc.region.tl.col = 0;
    chart_shapedesc.region.tl.row = 0;
    chart_shapedesc.region.br.col = 0;
    chart_shapedesc.region.br.row = 0;

    if(arg_is_present(p_args, 2 + 1*4 + 0))
    {
        chart_shapedesc.region.tl.col = p_args[2 + 1*4 + 0].val.col;
        chart_shapedesc.region.tl.row = p_args[2 + 1*4 + 1].val.row;
        chart_shapedesc.region.br.col = p_args[2 + 1*4 + 2].val.col;
        chart_shapedesc.region.br.row = p_args[2 + 1*4 + 3].val.row;
    }

    chart_shapedesc.bits.range_over_columns = (U8) ( (chart_shapedesc.region.br.row - chart_shapedesc.region.tl.row) >=
                                                     (chart_shapedesc.region.br.col - chart_shapedesc.region.tl.col) );

    status_return(chart_element(p_chart_header, &chart_shapedesc));

    /* y data */
    chart_shapedesc.region.tl.col = 0;
    chart_shapedesc.region.tl.row = 0;
    chart_shapedesc.region.br.col = 0;
    chart_shapedesc.region.br.row = 0;

    if(arg_is_present(p_args, 2 + 0*4 + 0))
    {
        chart_shapedesc.region.tl.col = p_args[2 + 0*4 + 0].val.col;
        chart_shapedesc.region.tl.row = p_args[2 + 0*4 + 1].val.row;
        chart_shapedesc.region.br.col = p_args[2 + 0*4 + 2].val.col;
        chart_shapedesc.region.br.row = p_args[2 + 0*4 + 3].val.row;
    }

    chart_shapedesc.bits.range_over_columns = (U8) ( (chart_shapedesc.region.br.row - chart_shapedesc.region.tl.row) >=
                                                     (chart_shapedesc.region.br.col - chart_shapedesc.region.tl.col) );

    status_return(chart_element(p_chart_header, &chart_shapedesc));

    chart_shapedesc.region.tl.col = 0;
    chart_shapedesc.region.tl.row = 0;
    chart_shapedesc.region.br.col = 0;
    chart_shapedesc.region.br.row = 0;

    /* x error data */
    switch(sertype)
    {
    case GR_CHART_SERIES_POINT_ERROR2:
    case GR_CHART_SERIES_PLAIN_ERROR2:
        if(arg_is_present(p_args, 2 + 3*4 + 0))
        {
            chart_shapedesc.region.tl.col = p_args[2 + 3*4 + 0].val.col;
            chart_shapedesc.region.tl.row = p_args[2 + 3*4 + 1].val.row;
            chart_shapedesc.region.br.col = p_args[2 + 3*4 + 2].val.col;
            chart_shapedesc.region.br.row = p_args[2 + 3*4 + 3].val.row;
        }
        break;

    default:
        break;
    }

    chart_shapedesc.bits.range_over_columns = (U8) ( (chart_shapedesc.region.br.row - chart_shapedesc.region.tl.row) >=
                                                     (chart_shapedesc.region.br.col - chart_shapedesc.region.tl.col) );

    status_return(chart_element(p_chart_header, &chart_shapedesc));

    chart_shapedesc.region.tl.col = 0;
    chart_shapedesc.region.tl.row = 0;
    chart_shapedesc.region.br.col = 0;
    chart_shapedesc.region.br.row = 0;

    /* y error data */
    switch(sertype)
    {
    case GR_CHART_SERIES_POINT_ERROR1:
    case GR_CHART_SERIES_PLAIN_ERROR1:
    case GR_CHART_SERIES_POINT_ERROR2:
    case GR_CHART_SERIES_PLAIN_ERROR2:
        if(arg_is_present(p_args, 2 + 2*4 + 0))
        {
            chart_shapedesc.region.tl.col = p_args[2 + 2*4 + 0].val.col;
            chart_shapedesc.region.tl.row = p_args[2 + 2*4 + 1].val.row;
            chart_shapedesc.region.br.col = p_args[2 + 2*4 + 2].val.col;
            chart_shapedesc.region.br.row = p_args[2 + 2*4 + 3].val.row;
        }
        break;

    default:
        break;
    }

    chart_shapedesc.bits.range_over_columns = (U8) ( (chart_shapedesc.region.br.row - chart_shapedesc.region.tl.row) >=
                                                     (chart_shapedesc.region.br.col - chart_shapedesc.region.tl.col) );

    status_return(chart_element(p_chart_header, &chart_shapedesc));

    return(STATUS_OK);
}

T5_CMD_PROTO(static, t5_cmd_chart_io_objid)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 3);
    GR_CHART_OBJID id;

    IGNOREPARM_DocuRef_(p_docu);
    IGNOREPARM_InVal_(t5_message);

    gr_chart_objid_clear(&id);

    id.name = (UBF) p_args[0].val.s32;

    if(arg_is_present(p_args, 1))
    {
        id.no = (UBF) p_args[1].val.s32;
        id.has_no = 1;

        if(arg_is_present(p_args, 2))
        {
            id.subno = (UBF) p_args[2].val.s32;
            id.has_subno = 1;
        }
    }

    chart_load_save_objid = id;

    return(chart_io_id_changed(chart_load_p_gr_chart, FALSE));
}

T5_MSG_PROTO(static, chart_note_load_intref_from_extref, _InoutRef_ P_NOTE_REF p_note_ref)
{
    const P_CHART_LOAD_INSTANCE p_chart_load_instance = chart_load_instance_goto(p_note_ref->p_of_ip_format);

    IGNOREPARM_DocuRef_(p_docu);
    IGNOREPARM_InVal_(t5_message);

    if(NULL != p_chart_load_instance)
    {
        ARRAY_INDEX i = array_elements(&p_chart_load_instance->h_mapping_list);

        while(--i >= 0)
        {
            P_CHART_LOAD_MAP p_chart_load_map = array_ptr(&p_chart_load_instance->h_mapping_list, CHART_LOAD_MAP, i);

            if(p_chart_load_map->extref == p_note_ref->extref)
            {
                p_note_ref->object_data_ref = p_chart_load_map->p_chart_header;
                return(STATUS_OK);
            }
        }
    }

    return(STATUS_FAIL);
}

T5_MSG_PROTO(static, chart_load_ended, _InoutRef_ P_OF_IP_FORMAT p_of_ip_format)
{
    const P_CHART_LOAD_INSTANCE p_chart_load_instance = chart_load_instance_goto(p_of_ip_format);

    IGNOREPARM_DocuRef_(p_docu);
    IGNOREPARM_InVal_(t5_message);

    if(NULL != p_chart_load_instance)
        al_array_dispose(&p_chart_load_instance->h_mapping_list);

    return(STATUS_OK);
}

T5_MSG_PROTO(static, chart_io_direct_msg_initclose, _InRef_ PC_MSG_INITCLOSE p_msg_initclose)
{
    IGNOREPARM_DocuRef_(p_docu);
    IGNOREPARM_InVal_(t5_message);

    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__STARTUP:
        qsort(chart_construct_table, elemof(chart_construct_table), sizeof(chart_construct_table[0]), proc_qsort_chart_construct_table);
        return(STATUS_OK);

    default:
        return(STATUS_OK);
    }
}

_Check_return_
static STATUS
chart_io_save_ended(
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format)
{
    const P_CHART_SAVE_INSTANCE p_chart_save_instance = chart_save_instance_goto(p_of_op_format);

    if(NULL != p_chart_save_instance)
        al_array_dispose(&p_chart_save_instance->h_mapping_list);

    return(STATUS_OK);
}

T5_MSG_PROTO(static, chart_io_direct_msg_save, _InRef_ PC_MSG_SAVE p_msg_save)
{
    IGNOREPARM_DocuRef_(p_docu);
    IGNOREPARM_InVal_(t5_message);

    switch(p_msg_save->t5_msg_save_message)
    {
    case T5_MSG_SAVE__SAVE_ENDED:
        return(chart_io_save_ended(p_msg_save->p_of_op_format));

    default:
        return(STATUS_OK);
    }
}

OBJECT_PROTO(extern, object_chart_io_sideways)
{
    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        return(chart_io_direct_msg_initclose(p_docu, t5_message, (PC_MSG_INITCLOSE) p_data));

    case T5_MSG_NOTE_ENSURE_SAVED:
        return(chart_note_ensure_saved(p_docu, t5_message, (P_NOTE_ENSURE_SAVED) p_data));

    case T5_MSG_SAVE:
        return(chart_io_direct_msg_save(p_docu, t5_message, (PC_MSG_SAVE) p_data));

    case T5_MSG_NOTE_LOAD_INTREF_FROM_EXTREF:
        return(chart_note_load_intref_from_extref(p_docu, t5_message, (P_NOTE_REF) p_data));

    case T5_MSG_LOAD_ENDED:
        return(chart_load_ended(p_docu, t5_message, (P_OF_IP_FORMAT) p_data));

    case T5_CMD_CHART_IO_1:
        return(t5_cmd_chart_io_1(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_CHART_IO_2:
        return(t5_cmd_chart_io_2(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_CHART_IO_3:
        return(t5_cmd_chart_io_3(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_CHART_IO_OBJID:
        return(t5_cmd_chart_io_objid(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_CHART_IO_PICT_TRANS_REF:
    case T5_CMD_CHART_IO_PICT_TRANS_EMB:
        return(chart_load_pict_trans(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_CHART_IO_TEXTCONTENTS:
        assert0();
    default:
        return(STATUS_OK);
    }
}

/* end of gr_chtIO.c */
