/* project.c */

/* Copyright (C) 1994-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#include "common/gflags.h"

#include "ob_rec/ob_rec.h"

#ifndef          __sk_slot_h
#include "ob_cells/sk_slot.h"
#endif

#ifndef         __xp_skeld_h
#include "ob_skel/xp_skeld.h"
#endif

/*
internal functions
*/

_Check_return_
static STATUS
rec_docu_changed(
    _DocuRef_   P_DOCU p_docu);

_Check_return_
static STATUS
rec_projector_changed(
    P_REC_PROJECTOR p_rec_projector);

_Check_return_
static STATUS
rec_sheet_col_auto_width(
    P_REC_PROJECTOR p_rec_projector);

_Check_return_
static STATUS
rec_slot_changed(
    _DocuRef_   P_DOCU p_docu,
    P_SLR p_slr);

static void
rec_uref_del_dependency(
    P_REC_PROJECTOR p_rec_projector);

_Check_return_
static STATUS
rec_data_ref_changed(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DATA_REF p_data_ref);

extern void
rec_event_null(
    /*_Inout_*/ P_ANY p_data /* P_NULL_EVENT_BLOCK p_null_event_block*/)
{
    IGNOREPARM(p_data);

#if DPLIB
    if(status_fail(dplib_client_updates()))
    {
        /* F***ed up so might as well not bother again... */
        trace_0(TRACE_OUT | TRACE_ANY, TEXT("rec_event_null - *** null_events_stop(DOCNO_NONE)"));
        null_events_stop(DOCNO_NONE, T5_EVENT_NULL, object_rec, (CLIENT_HANDLE) 0);
    }
#endif
}

static void
rec_observe_changes(
    _DocuRef_   P_DOCU p_docu,
    P_ANY server,
    P_ANY handle)
{
    P_REC_INSTANCE_DATA p_rec_instance = p_object_instance_data_REC(p_docu);
    ARRAY_INDEX i;

    for(i = 0; i < array_elements(&p_rec_instance->array_handle); i++)
    {
        P_REC_PROJECTOR p_rec_projector = array_ptr(&p_rec_instance->array_handle, REC_PROJECTOR, i);
        STATUS status;

        if(!p_rec_projector->opendb.dbok)
            continue;

#if DPLIB
        status = dplib_changes_in_p_opendb(&p_rec_projector->opendb, server, handle);
#else
        status = STATUS_OK; /* simulate no change observed */
#endif

        if(status == STATUS_DONE)
            rec_update_projector_adjust(p_rec_projector);
    }
}

extern void
rec_inform_documents_of_changes(
    P_ANY server,
    P_ANY handle)
{
    DOCNO docno = DOCNO_NONE;

    while(DOCNO_NONE != (docno = docno_enum_docs(docno)))
        rec_observe_changes(p_docu_from_docno(docno), server, handle);
}

_Check_return_
extern STATUS
rec_check_for_file_attachment(
    _DocuRef_   P_DOCU p_docu,
    _In_z_      PCTSTR filename)
{
    P_REC_INSTANCE_DATA p_rec_instance = p_object_instance_data_REC(p_docu);
    ARRAY_INDEX i;
    STATUS status = STATUS_OK;

    for(i = 0; i < array_elements(&p_rec_instance->array_handle); i++)
    {
        P_REC_PROJECTOR p_rec_projector = array_ptr(&p_rec_instance->array_handle, REC_PROJECTOR, i);

        if(!p_rec_projector->opendb.dbok)
            continue;

        if(0 == tstricmp(p_rec_projector->opendb.db.name, filename))
        {
            status = STATUS_FAIL;
            break;
        }
    }

    return(status);
}

static void
rec_frames_dispose(
    P_ARRAY_HANDLE p_array_handle /*REC_FRAMES[]*/)
{
    ARRAY_INDEX i = array_elements(p_array_handle);

    while(--i >= 0)
    {
        P_REC_FRAME p_rec_frame = array_ptr(p_array_handle, REC_FRAME, i);

        al_array_dispose(&p_rec_frame->h_title_text_ustr);
    }

    al_array_dispose(p_array_handle);
}

extern void
rec_kill_projector(
    _InoutRef_  P_P_REC_PROJECTOR p_p_rec_projector)
{
    P_REC_PROJECTOR p_rec_projector = *p_p_rec_projector;

    if(NULL != p_rec_projector)
    {
        const P_DOCU p_docu = p_docu_from_docno(p_rec_projector->docno);
        P_REC_INSTANCE_DATA p_rec_instance = p_object_instance_data_REC(p_docu);
        ARRAY_INDEX index = array_indexof_element(&p_rec_instance->array_handle, REC_PROJECTOR, p_rec_projector);
        *p_p_rec_projector = NULL;
        rec_frames_dispose(&p_rec_projector->h_rec_frames);
        al_array_delete_at(&p_rec_instance->array_handle, -1, index); /* delete */
    }
}

_Check_return_
extern STATUS
rec_instance_add_alloc(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_P_REC_PROJECTOR p_p_rec_projector)
{
    P_REC_INSTANCE_DATA p_rec_instance = p_object_instance_data_REC(p_docu);
    P_REC_PROJECTOR p_rec_projector;
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(*p_rec_projector), TRUE);
    STATUS status;

    if(NULL == (*p_p_rec_projector = p_rec_projector = al_array_extend_by(&p_rec_instance->array_handle, REC_PROJECTOR, 1, &array_init_block, &status)))
        return(status);

    /* Better initialise it some */
    p_rec_projector->docno = docno_from_p_docu(p_docu);

    docu_area_init(&p_rec_projector->rec_docu_area);

    p_rec_projector->uref_handle = UREF_HANDLE_INVALID;

    return(status);
}

/* This reads the size of the projected area then it buggers about with it iff adaptive rows */

_Check_return_
static STATUS
rec_get_table_size(
    P_REC_PROJECTOR p_rec_projector,
    P_SLR p_size)
{
    p_size->col = p_rec_projector->rec_docu_area.br.slr.col -  p_rec_projector->rec_docu_area.tl.slr.col;
    p_size->row = p_rec_projector->rec_docu_area.br.slr.row -  p_rec_projector->rec_docu_area.tl.slr.row;

    if(p_size->col <= 0)
        /* No way can this work */
        p_size->col = 1;

    if(p_size->row <= 0)
        /* No way can this work */
        p_size->row = 1;

    if(p_rec_projector->adaptive_rows)
    {
        if(!p_rec_projector->opendb.recordspec.ncards)
            p_size->row = 1;
        else
        {
            if(PROJECTOR_TYPE_CARD == p_rec_projector->projector_type)
                p_size->row = ( (p_rec_projector->opendb.recordspec.ncards + p_size->col - 1) / p_size->col);
            else
                p_size->row = p_rec_projector->opendb.recordspec.ncards;
        }
    }

    p_rec_projector->rec_docu_area.br.slr = p_rec_projector->rec_docu_area.tl.slr;
    p_rec_projector->rec_docu_area.br.slr.col += p_size->col;
    p_rec_projector->rec_docu_area.br.slr.row += p_size->row;

    p_rec_projector->start_offset = 0;

    return(STATUS_OK);
}

_Check_return_
extern STATUS
drop_projector_area(
    P_REC_PROJECTOR p_rec_projector)
{
    STATUS status = STATUS_OK;
    DOCU_AREA docu_area = p_rec_projector->rec_docu_area;
    const P_DOCU p_docu = p_docu_from_docno(p_rec_projector->docno);

    status_assert(maeve_event(p_docu, T5_MSG_CELL_MERGE, P_DATA_NONE));

    lock_rec_projector(p_rec_projector);

    if(OBJECT_ID_REC_FLOW != p_docu->object_id_flow)
    {
        cells_block_delete(p_docu,
                           docu_area.tl.slr.col,
                           docu_area.br.slr.col,
                           docu_area.tl.slr.row,
                           docu_area.br.slr.row - docu_area.tl.slr.row);
    }
    else
    {
        /* SKS 10apr95 - tell dependents about delete like a cells_block_delete would have done */
        UREF_PARMS uref_parms;
        uref_parms.source.region.tl.col = docu_area.tl.slr.col;
        uref_parms.source.region.tl.row = docu_area.tl.slr.row;
        uref_parms.source.region.br.col = docu_area.br.slr.col;
        uref_parms.source.region.br.row = docu_area.br.slr.row;
        uref_parms.source.region.whole_col = 0;
        uref_parms.source.region.whole_row = 0;
        if((docu_area.tl.slr.col == 0) && (docu_area.br.slr.col >= n_cols_logical(p_docu)))
            uref_parms.source.region.whole_row = 1;
        uref_event(p_docu, T5_MSG_UREF_DELETE, &uref_parms);
    }

    unlock_rec_projector(p_rec_projector);

    return(status);
}

_Check_return_
static STATUS
make_projector_area(
    _DocuRef_   P_DOCU p_docu,
    P_REC_PROJECTOR p_rec_projector,
    _InVal_     BOOL loading)
{
    STATUS status;
    SLR size;
    S32 old_rows = p_rec_projector->rec_docu_area.br.slr.row - p_rec_projector->rec_docu_area.tl.slr.row;

    for(;;)
    {
        status_break(status = rec_get_table_size(p_rec_projector, &size));

        if(OBJECT_ID_REC_FLOW != p_docu->object_id_flow)
        {
            SLR slr_tl = p_rec_projector->rec_docu_area.tl.slr;

            if(!loading)
            {
                status_break(status =
                    cells_block_insert(p_docu,
                                       p_rec_projector->rec_docu_area.tl.slr.col,
                                       p_rec_projector->rec_docu_area.tl.slr.col + size.col,
                                       p_rec_projector->rec_docu_area.tl.slr.row,
                                       size.row /*-1*/,
                                       0));
            }

            p_rec_projector->rec_docu_area.tl.slr = slr_tl;

            p_rec_projector->rec_docu_area.br.slr.col = p_rec_projector->rec_docu_area.tl.slr.col + size.col;

            /* size.row is a suggested row count but if adaptive rows and loading we need to use the projector size ??? */
            if((p_rec_projector->adaptive_rows) && loading)
            {
                /* Also watch out for "old" documents where the size is unknown and hence zero */
                if(old_rows > 0)
                    size.row = old_rows;
            }

            p_rec_projector->rec_docu_area.br.slr.row = p_rec_projector->rec_docu_area.tl.slr.row + size.row;

            size.col += p_rec_projector->rec_docu_area.tl.slr.col;
            size.row += p_rec_projector->rec_docu_area.tl.slr.row;

            size.col = MAX(n_cols_logical(p_docu), size.col);
            size.row = MAX(n_rows(p_docu), size.row);
        }
        else
        {
            size.col += p_rec_projector->rec_docu_area.tl.slr.col;
            size.row += p_rec_projector->rec_docu_area.tl.slr.row;
        }

        status_break(status = format_col_row_extents_set(p_docu, size.col, size.row));

        break;
        /*NOTREACHED*/
    }

    return status;
}

_Check_return_
extern STATUS
rec_insert_projector_and_attach_with_styles(
    P_REC_PROJECTOR p_rec_projector,
    _InVal_     BOOL loading)
{
    const P_DOCU p_docu = p_docu_from_docno(p_rec_projector->docno);

    status_return(make_projector_area(p_docu, p_rec_projector, loading));

    /* Set up a suitable docu_area */
    object_position_init(&p_rec_projector->rec_docu_area.tl.object_position);
    object_position_init(&p_rec_projector->rec_docu_area.br.object_position);

    p_rec_projector->rec_docu_area.whole_col = 0;
    p_rec_projector->rec_docu_area.whole_row = 1; /* SKS 25jun95 databases MUST control everything in the rows they occupy */

    if(OBJECT_ID_REC_FLOW == p_docu->object_id_flow)
    {
        p_rec_projector->rec_docu_area.whole_col = 1;
        p_rec_projector->rec_docu_area.whole_row = 1;
    }

    status_return(rec_uref_add_dependency(p_rec_projector));

    status_return(rec_style_database_create(p_docu, p_rec_projector));
    status_return(rec_style_all_fields_create(p_docu, p_rec_projector));
    status_return(rec_style_all_titles_create(p_docu, p_rec_projector));

    status_return(rec_style_styles_apply(p_docu, p_rec_projector));

    {
    STYLE style;
    STYLE_DOCU_AREA_ADD_PARM style_docu_area_add_parm;

    STYLE_DOCU_AREA_ADD_IMPLIED(&style_docu_area_add_parm, NULL, OBJECT_ID_REC, T5_EXT_STYLE_RECORDZ, 0, REGION_MAIN+1);
    style_docu_area_add_parm.type = STYLE_DOCU_AREA_ADD_TYPE_IMPLIED;
    style_docu_area_add_parm.data.p_style = &style;
    style_docu_area_add_parm.internal = TRUE;

    style_init(&style);
    style_bit_set(&style, STYLE_SW_PS_NEW_OBJECT);
    style.para_style.new_object = OBJECT_ID_REC;
    status_assert(style_docu_area_add(p_docu, &p_rec_projector->rec_docu_area, &style_docu_area_add_parm));
    } /*block*/

    {
    STYLE style;
    STYLE_DOCU_AREA_ADD_PARM style_docu_area_add_parm;

    STYLE_DOCU_AREA_ADD_IMPLIED(&style_docu_area_add_parm, NULL, OBJECT_ID_REC, T5_EXT_STYLE_RECORDZ, 0, REGION_MAIN); /* SKS changed to REGION_LOWER PMF puts is back again */
    style_docu_area_add_parm.type = STYLE_DOCU_AREA_ADD_TYPE_IMPLIED;
    style_docu_area_add_parm.data.p_style = &style;
    style_docu_area_add_parm.internal = TRUE;

    style_init(&style);
    style_bit_set(&style, STYLE_SW_CS_WIDTH);
    style.col_style.width = 0;
    status_assert(style_docu_area_add(p_docu, &p_rec_projector->rec_docu_area, &style_docu_area_add_parm));

    style_init(&style);
    style_bit_set(&style, STYLE_SW_CS_COL_NAME);
    style.col_style.h_numform = 0;
    status_assert(style_docu_area_add(p_docu, &p_rec_projector->rec_docu_area, &style_docu_area_add_parm));

    if(PROJECTOR_TYPE_CARD == p_rec_projector->projector_type) /* SKS 27mar95 */
    {
        style_init(&style);
        style_bit_set(&style, STYLE_SW_RS_HEIGHT);
        style.row_style.height = 0;
        status_assert(style_docu_area_add(p_docu, &p_rec_projector->rec_docu_area, &style_docu_area_add_parm));
    }
    } /*block*/

    if(PROJECTOR_TYPE_CARD != p_rec_projector->projector_type)
    {
        /* SKS 07jul95 see if it's first time ever gone into sheet */
        STYLE_HANDLE style_base = style_handle_base(&p_docu->h_style_docu_area);

        if(style_base)
        {
            const PC_STYLE p_style = array_ptrc(&p_docu->h_style_list, STYLE, style_base);

            if(style_bit_test(p_style, STYLE_SW_RS_HEIGHT))
            {
                /* SKS 07jul95 vvv horrid special value embedded in 1.22 template files; use this to trigger transition */
                if(3159 == p_style->row_style.height)
                {
                    STYLE style = *p_style;
                    style_selector_clear(&style.selector);
                    style_bit_set(&style, STYLE_SW_RS_HEIGHT);
                    style.row_style.height = 336; /* wot, yet another yukky constant??? or should we turn off fixed height bit - I think not */
                    style_handle_modify(p_docu, style_base, &style, &style.selector, &style.selector);
                }
            }
        }

        status_assert(rec_sheet_col_auto_width(p_rec_projector));
    }

    status_return(rec_recompute_card_size(p_rec_projector));

    docu_modify(p_docu);

    return(STATUS_OK);
}

/* compute the auto widths and stash them in the frames */

_Check_return_
static STATUS
rec_sheet_col_auto_width(
    P_REC_PROJECTOR p_rec_projector)
{
    const P_DOCU p_docu = p_docu_from_docno(p_rec_projector->docno);
    ARRAY_INDEX i;
    PROCESS_STATUS process_status;

    process_status_init(&process_status);
    process_status.flags.foreground = TRUE;
    process_status.reason.type = UI_TEXT_TYPE_RESID;
    process_status.reason.text.resource_id = REC_MSG_STATUS_AUTO_WIDTHING;

    process_status_begin(p_docu, &process_status, PROCESS_STATUS_PERCENT);

    for(i = 0; i < array_elements(&p_rec_projector->h_rec_frames); i++)
    {
        P_REC_FRAME p_rec_frame = array_ptr(&p_rec_projector->h_rec_frames, REC_FRAME, i);

        process_status.data.percent.current = (100 * i) / array_elements(&p_rec_projector->h_rec_frames);
        process_status_reflect(&process_status);

        if(p_rec_frame->field_width > 0)
            continue;

        p_rec_frame->field_width = 0;

        if(p_rec_projector->opendb.recordspec.ncards > 0)
        {
            COL_AUTO_WIDTH col_auto_width;
            col_auto_width.col = p_rec_projector->rec_docu_area.tl.slr.col + (COL) i;
            col_auto_width.row_s = p_rec_projector->rec_docu_area.tl.slr.row;
            col_auto_width.row_e = p_rec_projector->rec_docu_area.br.slr.row;
            col_auto_width.allow_special = TRUE;
            col_auto_width.width = 0;
            status_consume(object_call_id(p_docu->object_id_flow, p_docu, T5_MSG_COL_AUTO_WIDTH, &col_auto_width));
            p_rec_frame->field_width = col_auto_width.width;
        }

        /* I think this should be made at least wide enough for the fieldname/title (SKS - very hacky!) */
        if(0 != array_elements(&p_rec_frame->h_title_text_ustr))
        {
            PC_USTR ustr = array_ustr(&p_rec_frame->h_title_text_ustr);
            PIXIT width = ui_width_from_ustr(ustr);
            width += DIALOG_SYSCHAR_H; /* allow for a bit either side */
            if( p_rec_frame->field_width < width)
                p_rec_frame->field_width = width;
        }
    }

    process_status_end(&process_status);

    return(STATUS_OK);
}

extern void
rec_sheet_stash_field_widths(
    P_REC_PROJECTOR p_rec_projector)
{
    ARRAY_INDEX i = array_elements(&p_rec_projector->h_rec_frames);

    while(--i >= 0)
    {
        P_REC_FRAME p_rec_frame = array_ptr(&p_rec_projector->h_rec_frames, REC_FRAME, i);
        SLR slr = p_rec_projector->rec_docu_area.tl.slr;
        slr.col = p_rec_projector->rec_docu_area.tl.slr.col + (COL) i;
        p_rec_frame->field_width = cell_width(p_docu_from_docno(p_rec_projector->docno), &slr); /* in ob_cells/sk_col.c */
    }
}

_Check_return_
extern STATUS
rec_recompute_card_size(
    P_REC_PROJECTOR p_rec_projector)
{
    STATUS status;

    if(PROJECTOR_TYPE_CARD == p_rec_projector->projector_type)
        status = get_card_size(p_rec_projector, &p_rec_projector->card_size);
    else
        status = rec_sheet_col_auto_width(p_rec_projector);

    return(status);
}

_Check_return_
static STATUS
rec_projector_kill_close(
    P_REC_PROJECTOR * p_p_rec_projector)
{
    P_REC_PROJECTOR p_rec_projector = *p_p_rec_projector;
    STATUS status = STATUS_OK;
    rec_uref_del_dependency(p_rec_projector);
    status = close_database(&p_rec_projector->opendb, status);
    rec_kill_projector(p_p_rec_projector);
    return(status);
}

extern void
lock_rec_projector(
    P_REC_PROJECTOR p_rec_projector)
{
    p_rec_projector->lock += 1;
}

extern void
unlock_rec_projector(
    P_REC_PROJECTOR p_rec_projector)
{
    p_rec_projector->lock -= 1;
}

_Check_return_
extern BOOL
is_rec_projector_locked(
    P_REC_PROJECTOR p_rec_projector)
{
  return (p_rec_projector->lock != 0);
}

extern void
rec_closedown(
    _DocuRef_   P_DOCU p_docu)
{
    P_REC_INSTANCE_DATA p_rec_instance = p_object_instance_data_REC(p_docu);

    rec_cache_dispose(p_docu);

    { /* Close all the attached databases */
    assert(0 == array_elements(&p_rec_instance->array_handle)); /* uref *should* have killed all of these */
    while(0 != array_elements(&p_rec_instance->array_handle))
    {   /* Close the topmost element */
        P_REC_PROJECTOR p_rec_projector = array_ptr(&p_rec_instance->array_handle, REC_PROJECTOR, array_elements(&p_rec_instance->array_handle) - 1);
        rec_projector_kill_close(&p_rec_projector);
    }
    al_array_dispose(&p_rec_instance->array_handle);
    } /*block*/
}

/* Strictly a private routine
   if you need to get at a rec_projector you should have a db_id and use
   p_rec_projector_from_db_id()
*/

_Check_return_
extern P_REC_PROJECTOR
p_rec_projector_from_slr(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_SLR p_slr)
{
    P_REC_INSTANCE_DATA p_rec_instance = p_object_instance_data_REC(p_docu);
    ARRAY_INDEX i;

    /* Lets return one which has an overlap with the given slr */
    for(i = 0; i < array_elements(&p_rec_instance->array_handle); i++ )
    {
        P_REC_PROJECTOR p_rec_projector = array_ptr(&p_rec_instance->array_handle, REC_PROJECTOR, i);

        if(p_rec_projector->rec_docu_area.tl.slr.col > p_slr->col)
            continue;

        if(p_rec_projector->rec_docu_area.br.slr.col <= p_slr->col)
            continue;

        if(p_rec_projector->rec_docu_area.tl.slr.row > p_slr->row)
            continue;

        if(p_rec_projector->rec_docu_area.br.slr.row <= p_slr->row)
            continue;

        return(p_rec_projector);
    }

    return(NULL);
}

/* Find the min & max cell range */

_Check_return_
extern STATUS
max_rec_projector(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_REGION p_region)
{
    STATUS status = STATUS_OK;
    P_REC_INSTANCE_DATA p_rec_instance = p_object_instance_data_REC(p_docu);
    ARRAY_INDEX i;

    p_region->tl.col = MAX_COL;
    p_region->tl.row = MAX_ROW;

    p_region->br.col = 0;
    p_region->br.row = 0;

    p_region->whole_col = 0;
    p_region->whole_row = 0;

    for(i = 0; i < array_elements(&p_rec_instance->array_handle); ++i)
    {
        P_REC_PROJECTOR p_rec_projector = array_ptr(&p_rec_instance->array_handle, REC_PROJECTOR, i);

        p_region->tl.col = MIN(p_region->tl.col, p_rec_projector->rec_docu_area.tl.slr.col);
        p_region->tl.row = MIN(p_region->tl.row, p_rec_projector->rec_docu_area.tl.slr.row);

        p_region->br.col = MAX(p_region->br.col, p_rec_projector->rec_docu_area.br.slr.col);
        p_region->br.row = MAX(p_region->br.row, p_rec_projector->rec_docu_area.br.slr.row);

        status = STATUS_DONE; /* processed */
    }

    return(status);
}

/* Use this where you need a p_rec_projector from a data_ref (iff type DATA_DB_FIELD or DATA_DB_TITLE) */

_Check_return_
extern P_REC_PROJECTOR
p_rec_projector_from_data_ref(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DATA_REF p_data_ref)
{
    switch(p_data_ref->data_space)
    {
    case DATA_DB_FIELD:
    case DATA_DB_TITLE:
        assert_EQ(offsetof32(DATA_REF_ARG, db_field.db_id), offsetof32(DATA_REF_ARG, db_title.db_id));
        return(p_rec_projector_from_db_id(p_docu, p_data_ref->arg.db_field.db_id));

    default: default_unhandled();
        /* could be coded to work if necessary */
        return(NULL);
    }
}

_Check_return_
extern P_REC_PROJECTOR
p_rec_projector_from_db_id(
    _DocuRef_   P_DOCU p_docu,
    _In_        DB_ID db_id)
{
    P_REC_INSTANCE_DATA p_rec_instance = p_object_instance_data_REC(p_docu);
    const ARRAY_INDEX n_rec_projectors = array_elements(&p_rec_instance->array_handle);
    ARRAY_INDEX i;
    P_REC_PROJECTOR p_rec_projector = array_range(&p_rec_instance->array_handle, REC_PROJECTOR, 0, n_rec_projectors);

    for(i = 0; i < n_rec_projectors; ++i, ++p_rec_projector)
        if(p_rec_projector->opendb.db.id == db_id)
            return(p_rec_projector);

    return(NULL);
}

#if 0 /*unused*/

static void
trash_caches_for_docu(
    _DocuRef_   P_DOCU p_docu)
{
    CACHES_DISPOSE caches_dispose;

    caches_dispose.region.tl.col = 0;
    caches_dispose.region.tl.row = 0;
    caches_dispose.region.br.col = n_cols_logical(p_docu);
    caches_dispose.region.br.row = n_rows(p_docu);

    /* Lose both flavours */
    caches_dispose.data_space = DATA_DB_FIELD;
    status_assert(maeve_event(p_docu, T5_MSG_CACHES_DISPOSE, &caches_dispose));

    caches_dispose.data_space = DATA_DB_TITLE;
    status_assert(maeve_event(p_docu, T5_MSG_CACHES_DISPOSE, &caches_dispose));

    rec_claim_caret(p_docu);
}

#endif

/* Called in ob_rec after CREATE and APPEND and before and after LAYOUT and PROP
   These use this most indescriminate version becuase they, none of them, have
   a wholly valid projector to hand.
*/

_Check_return_
extern STATUS
rec_update_docu(
    _DocuRef_   P_DOCU p_docu)
{
    STATUS status = rec_docu_changed(p_docu);
    view_update_all(p_docu, UPDATE_PANE_CELLS_AREA);
    return(status);
}

/* Called in ob_rec by the GOTO commands in CARD MODE ONLY in preference to rec_update_all because
   we know the number of records will not have changed. Hence we dont need to adjust_projector() it, which is expensive.

   Also only redraws the field contents not all the titles and other shit
*/

_Check_return_
extern STATUS
rec_update_card_contents(
    P_REC_PROJECTOR p_rec_projector)
{
    STATUS status = rec_projector_changed(p_rec_projector);
    COL col;

    myassert0x(p_rec_projector->projector_type == PROJECTOR_TYPE_CARD, TEXT("Not a card display"));

    for(col = p_rec_projector->rec_docu_area.tl.slr.col; col < p_rec_projector->rec_docu_area.br.slr.col; col++)
    {
        ROW row;

        for(row = p_rec_projector->rec_docu_area.tl.slr.row; row < p_rec_projector->rec_docu_area.br.slr.row; row++)
        {
            S32 frame;

            for(frame = 0; frame < array_elements(&p_rec_projector->h_rec_frames); frame++)
            {
                P_REC_FRAME p_rec_frame = array_ptr(&p_rec_projector->h_rec_frames, REC_FRAME, frame);
                const P_DOCU p_docu = p_docu_from_docno(p_rec_projector->docno);
                SKEL_RECT skel_rect;
                SLR slr;

                slr.row = row;
                slr.col = col;
                skel_rect_from_slr(p_docu, &skel_rect, &slr); /* Find the basic skel_rect */

                skel_rect.br.pixit_point.x  = skel_rect.tl.pixit_point.x + p_rec_frame->pixit_rect_field.br.x;
                skel_rect.br.pixit_point.y  = skel_rect.tl.pixit_point.y + p_rec_frame->pixit_rect_field.br.y;
                skel_rect.tl.pixit_point.x += p_rec_frame->pixit_rect_field.tl.x;
                skel_rect.tl.pixit_point.y += p_rec_frame->pixit_rect_field.tl.y;

#if 1
                {
                /* This results in a very smooth update for any number of cards * fields */
                RECT_FLAGS rect_flags;
                REDRAW_FLAGS redraw_flags;

                RECT_FLAGS_CLEAR(rect_flags);
                rect_flags.reduce_left_by_1  = 1; /* maybe should be _by_2 */
                rect_flags.reduce_up_by_1    = 1;
              /*rect_flags.reduce_right_by_1 = 1;*/
              /*rect_flags.reduce_down_by_1  = 1;*/

                REDRAW_FLAGS_CLEAR(redraw_flags);
                redraw_flags.show_selection = TRUE;
                redraw_flags.show_content = TRUE;

                view_update_now(p_docu, UPDATE_PANE_CELLS_AREA, &skel_rect, rect_flags, redraw_flags, LAYER_SLOT);
                } /*block*/
#else
                /* This results in a very smooth update for up to 16 cards * fields but then runs out
                   of rectangles in the riscos wimp and forces a whole screeen update */

                view_update_later(p_docu, UPDATE_PANE_CELLS_AREA, &skel_rect, rect_flags);
#endif
            }
        }
    }

    return(status);
}

extern void
trash_caches_for_projector(
    P_REC_PROJECTOR p_rec_projector)
{
    const P_DOCU p_docu = p_docu_from_docno(p_rec_projector->docno);
    CACHES_DISPOSE caches_dispose;

    region_from_docu_area_max(&caches_dispose.region, &p_rec_projector->rec_docu_area);

    /* Lose both flavours */
    caches_dispose.data_space = DATA_DB_FIELD;
    status_assert(maeve_event(p_docu, T5_MSG_CACHES_DISPOSE, &caches_dispose));

    caches_dispose.data_space = DATA_DB_TITLE;
    status_assert(maeve_event(p_docu, T5_MSG_CACHES_DISPOSE, &caches_dispose));

/*    rec_claim_caret(p_docu); */
}

/*
Called in ob_rec by the GOTO commands for SHEET mode in preference to rec_update_projector_adjust() because we know
the number of records will not have changed. Hence we dont need to adaptive_projector_adjust(), which is expensive.
*/

_Check_return_
extern STATUS
rec_update_projector(
    P_REC_PROJECTOR p_rec_projector)
{
    STATUS status = rec_projector_changed(p_rec_projector);
    const P_DOCU p_docu = p_docu_from_docno(p_rec_projector->docno);
    SKEL_RECT skel_rect;
    RECT_FLAGS rect_flags;

    skel_rect_from_docu_area(p_docu, &skel_rect, &p_rec_projector->rec_docu_area);

    RECT_FLAGS_CLEAR(rect_flags);
    view_update_later(p_docu, UPDATE_PANE_CELLS_AREA, &skel_rect, rect_flags);

    return(status);
}

_Check_return_
static STATUS
adaptive_projector_adjust(
    P_REC_PROJECTOR p_rec_projector)
{
    STATUS status = drop_projector_area(p_rec_projector);

    /* Ensure we have some form of cursor and reread the number of records */
    status = ensure_cursor_current(&p_rec_projector->opendb, 0);

    if(status_ok(status))
        status = rec_insert_projector_and_attach_with_styles(p_rec_projector, FALSE);

    return(status);
}

_Check_return_
extern STATUS
rec_update_projector_adjust(
    P_REC_PROJECTOR p_rec_projector)
{
    if(p_rec_projector->adaptive_rows)
        adaptive_projector_adjust(p_rec_projector);

    return(rec_update_projector(p_rec_projector));
}

_Check_return_
extern STATUS
rec_update_projector_adjust_goto(
    P_REC_PROJECTOR p_rec_projector,
    _In_        S32 recno)
{
    if(p_rec_projector->adaptive_rows)
        adaptive_projector_adjust(p_rec_projector);

    if(recno >= 0)
        rec_goto_record(p_docu_from_docno(p_rec_projector->docno), TRUE, recno);

    return(rec_update_projector(p_rec_projector));
}

#if 0 /*unused*/

static void
trash_caches_for_slot(
    _DocuRef_   P_DOCU p_docu,
    P_SLR p_slr)
{
    CACHES_DISPOSE caches_dispose;

    caches_dispose.region.tl.col = p_slr->col;
    caches_dispose.region.tl.row = p_slr->row;
    caches_dispose.region.br.col = caches_dispose.region.tl.col + 1;
    caches_dispose.region.br.row = caches_dispose.region.tl.row + 1;

    /* Lose both flavours */
    caches_dispose.data_space = DATA_DB_FIELD;
    status_assert(maeve_event(p_docu, T5_MSG_CACHES_DISPOSE, &caches_dispose));

    caches_dispose.data_space = DATA_DB_TITLE;
    status_assert(maeve_event(p_docu, T5_MSG_CACHES_DISPOSE, &caches_dispose));

    rec_claim_caret(p_docu);
}

#endif

/* Called by ed_rec after a drag aborted */

_Check_return_
extern STATUS
rec_update_cell(
    _DocuRef_   P_DOCU p_docu,
    P_SLR p_slr)
{
    STATUS status = rec_slot_changed(p_docu, p_slr);
    SKEL_RECT skel_rect;
    RECT_FLAGS rect_flags;

    skel_rect_from_slr(p_docu, &skel_rect, p_slr); /* Find the basic skel_rect */

    RECT_FLAGS_CLEAR(rect_flags);
    view_update_later(p_docu, UPDATE_PANE_CELLS_AREA, &skel_rect, rect_flags);

    return(status);
}

/* Called after editing a field */

_Check_return_
extern STATUS
rec_update_data_ref(
    P_REC_PROJECTOR p_rec_projector,
    _InRef_     PC_DATA_REF p_data_ref)
{
    const P_DOCU p_docu = p_docu_from_docno(p_rec_projector->docno);
    STATUS status;
    BOOL has_formula = FALSE;
    ARRAY_INDEX i;

    for(i = 0; i < array_elements(&p_rec_projector->opendb.table.h_fielddefs); i++)
        if(FIELD_FORMULA == array_ptrc(&p_rec_projector->opendb.table.h_fielddefs, FIELDDEF, i)->type)
        {
            has_formula = TRUE;
            break;
        }

    if(has_formula)
    {
        SLR slr;

        if(status_ok(status = rec_slr_from_data_ref(p_docu, p_data_ref, &slr)))
        {
            if(PROJECTOR_TYPE_CARD == p_rec_projector->projector_type)
                status = rec_update_cell(p_docu, &slr);
            else
                status = rec_update_projector(p_rec_projector);
        }
    }
    else
    {
        status = rec_data_ref_changed(p_docu, p_data_ref);

        {
        SKEL_RECT skel_rect;
        RECT_FLAGS rect_flags;

        rec_skel_rect_from_data_ref(p_rec_projector, &skel_rect, p_data_ref);

        RECT_FLAGS_CLEAR(rect_flags);
        view_update_later(p_docu, UPDATE_PANE_CELLS_AREA, &skel_rect, rect_flags);
        } /*block*/
    }

    return(status);
}

/******************************************************************************
*
* Routines to generate urefs for names and for cells
*
******************************************************************************/

/* Routine to generate urefs for names */

_Check_return_
static STATUS
rec_generate_name_uref(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DATA_REF p_data_ref)
{
    /* lob it at maeve
       as a result of which the message T5_MSG_DATA_REF_NAME_COMPARE will come through
       zero or more times to see if the name has changed
    */
    NAME_UREF name_uref;
    name_uref.docno    = docno_from_p_docu(p_docu);
    name_uref.data_ref = *p_data_ref;
    status_assert(maeve_event(p_docu, T5_MSG_NAME_UREF, &name_uref));
    return(STATUS_OK);
}

/* Routine to generate urefs for cells */

_Check_return_
static STATUS
rec_generate_uref(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_SLR p_slr_tl,
    _InRef_     PC_SLR p_slr_br)
{
   STATUS status = STATUS_OK;
   UREF_PARMS uref_parms;

   uref_parms.source.region.tl = *p_slr_tl;
   uref_parms.source.region.br = *p_slr_br;

   uref_parms.source.region.whole_col = uref_parms.source.region.whole_row = 0;

   uref_event(p_docu, T5_MSG_UREF_OVERWRITE, &uref_parms);

   return status;
}

/* Purpose : To generate UREFs for cells and Names */

_Check_return_
static STATUS
rec_data_ref_changed(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DATA_REF p_data_ref)
{
    status_return(rec_generate_name_uref(p_docu, p_data_ref));

    {
    SLR slr_tl;
    SLR slr_br;

    status_assert(rec_slr_from_data_ref(p_docu, p_data_ref, &slr_tl));
    slr_br.col = slr_tl.col + 1;
    slr_br.row = slr_tl.row + 1;

    return(rec_generate_uref(p_docu, &slr_tl, &slr_br));
    } /*block*/
}

/* Purpose : To generate UREFs for cells and Names */

_Check_return_
static STATUS
rec_docu_changed(
    _DocuRef_   P_DOCU p_docu)
{
    P_REC_INSTANCE_DATA p_rec_instance = p_object_instance_data_REC(p_docu);
    ARRAY_INDEX i = array_elements(&p_rec_instance->array_handle);

    /* Enumerate all the projectors */
    while(--i >= 0)
        rec_projector_changed(array_ptr(&p_rec_instance->array_handle, REC_PROJECTOR, i));

    return(STATUS_OK);
}

/* Purpose : To generate UREFs for cells and Names */

_Check_return_
static STATUS
rec_projector_changed(
    P_REC_PROJECTOR p_rec_projector)
{
    const P_DOCU p_docu = p_docu_from_docno(p_rec_projector->docno);
    STATUS status = rec_generate_uref(p_docu, &p_rec_projector->rec_docu_area.tl.slr, &p_rec_projector->rec_docu_area.br.slr);
    DATA_REF data_ref;

    if(PROJECTOR_TYPE_CARD == p_rec_projector->projector_type)
    {
        /* data_ref will be set up for the first field so if we just enumerate the frames field id into it we can span the whole thing */
        ARRAY_INDEX i;

        rec_data_ref_from_slr(p_docu, &p_rec_projector->rec_docu_area.tl.slr, &data_ref);

        for(i = 0; i < array_elements(&p_rec_projector->h_rec_frames); i++)
        {
            PC_REC_FRAME p_rec_frame = array_ptrc(&p_rec_projector->h_rec_frames, REC_FRAME, i);
            data_ref.arg.db_field.field_id = p_rec_frame->field_id;
            status = rec_generate_name_uref(p_docu, &data_ref);
        }
    }
    else
    {
        /* Walk across the top row getting data refs  */
        SLR slr = p_rec_projector->rec_docu_area.tl.slr;

        while(slr.col < p_rec_projector->rec_docu_area.br.slr.col)
        {
            rec_data_ref_from_slr(p_docu, &slr, &data_ref);
            status = rec_generate_name_uref(p_docu, &data_ref);
            slr.col++;
        }
     }

    return(status);
}

/* Purpose : To generate UREFs for cells and Names */

_Check_return_
static STATUS
rec_slot_changed(
    _DocuRef_   P_DOCU p_docu,
    P_SLR p_slr)
{
    STATUS status;
    DATA_REF data_ref;
    P_REC_PROJECTOR p_rec_projector;
    SLR slr_tl = *p_slr;
    SLR slr_br;

    slr_br.col = slr_tl.col + 1;
    slr_br.row = slr_tl.row + 1;

    status = rec_generate_uref(p_docu, &slr_tl, &slr_br);

    if(NULL != (p_rec_projector = rec_data_ref_from_slr(p_docu, p_slr, &data_ref)))
    {
        if(PROJECTOR_TYPE_CARD == p_rec_projector->projector_type)
        {
            ARRAY_INDEX i;

            for(i = 0; i < array_elements(&p_rec_projector->h_rec_frames); i++)
            {
                PC_REC_FRAME p_rec_frame = array_ptrc(&p_rec_projector->h_rec_frames, REC_FRAME, i);
                data_ref.arg.db_field.field_id = p_rec_frame->field_id;
                status = rec_generate_name_uref(p_docu, &data_ref);
            }
        }
        else
            status = rec_generate_name_uref(p_docu, &data_ref);
    }

    return(status);
}

/******************************************************************************
*
* callback from uref to update references
*
******************************************************************************/

PROC_UREF_EVENT_PROTO(static, proc_uref_event_ob_rec)
{
    DB_ID db_id = (DB_ID) p_uref_event_block->uref_id.client_handle;
    P_REC_PROJECTOR p_rec_projector = p_rec_projector_from_db_id(p_docu, db_id);

    myassert1x(p_rec_projector != NULL, TEXT("REC UREF for unknown database ") S32_TFMT, (S32) db_id);

    if(NULL == p_rec_projector)
        return(STATUS_OK);

    switch(p_uref_event_block->reason.code)
    {
    case DEP_DELETE:
        trash_caches_for_projector(p_rec_projector);

        if(!is_rec_projector_locked(p_rec_projector)) /* over slot_delete/insert pairs */
            rec_projector_kill_close(&p_rec_projector);

        break;

     case DEP_INFORM:
        break;

    case DEP_UPDATE:
        {
        DOCU_AREA new_docu_area;
        S32 old_width, new_width;
        S32 i, n;
        S32 start, end, delta;
        P_REC_FRAME p_rec_frames;

        /* The new docu area which we will own */
        docu_area_from_region(&new_docu_area, &p_uref_event_block->uref_id.region);

        old_width = p_rec_projector->rec_docu_area.br.slr.col - p_rec_projector->rec_docu_area.tl.slr.col;
        new_width =                  new_docu_area.br.slr.col -                  new_docu_area.tl.slr.col;

        p_rec_frames = array_base(&p_rec_projector->h_rec_frames, REC_FRAME);
        n = array_elements(&p_rec_projector->h_rec_frames);

        /* If the projected area is a sheet we may need to update the frames  */

        if( (p_rec_projector->projector_type == PROJECTOR_TYPE_SHEET) && ( new_width != old_width ) )
        { /* A column or columns may have been added or deleted */

            switch(t5_message)
            {
            case T5_MSG_UREF_UREF:
                {
                  /* The new region to occupy is  p_uref_event_block -> uref_id.region
                     The region over which the change has occured is  p_uref_event_block -> source.region
                     The change which has occured is p_uref_event_block -> target.slr
                  */

                  start = p_uref_event_block->uref_parms.source.region.tl.col - p_uref_event_block -> uref_id.region.tl.col;
                  delta = new_width - old_width;

                  /* If start is negative then the change is outside the table and just causing a move
                     It may also be true that start is > n in which case we must ignore it
                  */

                  if((start >= 0) && (start < n))
                  {
                      /* If delta is negative we are deleting columns and hence frames else adding! */
                      if(delta < 0)
                          al_array_delete_at(&p_rec_projector->h_rec_frames, delta, start);
                      else
                      {
                          STATUS status;
                          SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(REC_FRAME), TRUE);
                          P_REC_FRAME p_rec_frame = al_array_insert_before(&p_rec_projector->h_rec_frames, REC_FRAME, delta, &array_init_block, &status, start);

                          if(!IS_P_DATA_NONE(p_rec_frame))
                          {
                              for(i = 0; i < delta; ++i, ++p_rec_frame)
                                  p_rec_frame->field_id = (FIELD_ID) -1;
                          }
                       }
                  } /* endif start */

                break;
                } /* end case uref */

            case T5_MSG_UREF_DELETE:
                {
                  /* The region specified by the p_uref_event_block -> source.region is being deleted */

                  /* calculate the start & end frame number */
                  start = p_uref_event_block->uref_parms.source.region.tl.col - p_rec_projector->rec_docu_area.tl.slr.col;
                  end   = p_uref_event_block->uref_parms.source.region.br.col - p_rec_projector->rec_docu_area.tl.slr.col;

                  delta = start - end; /* Will be negative */

                  myassert1x(delta < 0, TEXT("Uref delete start > end ?") S32_TFMT, delta);

                  /* Check it is with the bounds of reason */
                  if((start >= 0) && (start <n ))
                 {
                     if(end >= n)
                    {
                        /* Nothing to move left */ /*EMPTY*/
                    }
                     else
                    {
                        myassert1x(end>=n, TEXT("This never happens in UREF_DELETE ") S32_TFMT, end);
                        /* Move down any which follow the deleted area */
                        for(i = end; i<n; i++ )
                        {
                            p_rec_frames[i+delta] = p_rec_frames[i]; /* delta is negative for deletion so this copeis down ie left  */
                        }
                    } /* endif end >= n */

                 } /* endif start */

                  break;
                } /* end case uref_delete */

            } /* end switch t5_message */

        } /* endif sheet and width changed */

        p_rec_projector->rec_docu_area = new_docu_area;

        trash_caches_for_projector(p_rec_projector);

        break;
        }
    }

    return(STATUS_OK);
}

_Check_return_
extern STATUS
rec_uref_add_dependency(
    P_REC_PROJECTOR p_rec_projector)
{
    STATUS status;
    REGION region;

    region_from_docu_area_max(&region, &p_rec_projector->rec_docu_area);

    /* remove any previous uref */
    rec_uref_del_dependency(p_rec_projector);

    status = uref_add_dependency(p_docu_from_docno(p_rec_projector->docno), &region, proc_uref_event_ob_rec, (S32) p_rec_projector->opendb.db.id /* client handle */, &p_rec_projector->uref_handle, FALSE);

    return(status);
}

static void
rec_uref_del_dependency(
    P_REC_PROJECTOR p_rec_projector)
{
    if(p_rec_projector->uref_handle != UREF_HANDLE_INVALID)
    {
        UREF_HANDLE uref_handle = p_rec_projector->uref_handle;
        p_rec_projector->uref_handle = UREF_HANDLE_INVALID;
        uref_del_dependency(p_rec_projector->docno, uref_handle);
    }
}

_Check_return_
static PIXIT
default_text_leading(
    _DocuRef_   P_DOCU p_docu,
    P_REC_PROJECTOR p_rec_projector)
{
    STYLE_SELECTOR selector;
    STYLE style;
    S32 default_text_height;

    style_selector_copy(&selector, &style_selector_para_leading);
    style_selector_bit_set(&selector, STYLE_SW_PS_PARA_START);
    style_selector_bit_set(&selector, STYLE_SW_PS_PARA_END);

    /* find out size of cell */
    style_init(&style);
    style_from_slr(p_docu, &style, &selector, &p_rec_projector->rec_docu_area.tl.slr);

    default_text_height = style_leading_from_style(&style, &style.font_spec, p_docu->flags.draft_mode) + style.para_style.para_start + style.para_style.para_end;

    return(default_text_height);
}

_Check_return_
extern STATUS
set_default_title_rect(
    P_REC_PROJECTOR p_rec_projector,
    P_REC_FRAME p_rec_frame)
{
    /* Create the default title rectangle for this field according to its type */

    p_rec_frame->pixit_rect_title.br.x = p_rec_frame->pixit_rect_field.tl.x - REC_DEFAULT_TITLE_FRAME_X;
    p_rec_frame->pixit_rect_title.br.x = skel_ruler_snap_to_click_stop(p_docu_from_docno(p_rec_projector->docno), 1, p_rec_frame->pixit_rect_title.br.x, SNAP_TO_CLICK_STOP_ROUND);
    p_rec_frame->pixit_rect_title.tl.x = p_rec_frame->pixit_rect_title.br.x - REC_DEFAULT_TITLE_FRAME_W;
    p_rec_frame->pixit_rect_title.tl.x = skel_ruler_snap_to_click_stop(p_docu_from_docno(p_rec_projector->docno), 1, p_rec_frame->pixit_rect_title.tl.x, SNAP_TO_CLICK_STOP_ROUND);

    p_rec_frame->pixit_rect_title.tl.y = p_rec_frame->pixit_rect_field.tl.y;
    p_rec_frame->pixit_rect_title.br.y = p_rec_frame->pixit_rect_field.br.y;

    return(STATUS_OK);
}

/*
(re)construct the frames from the fields
create frames which have become visible (ie where a visible field has no extant frame)
kill frames which have no fielddef or which have become hidden
*/

_Check_return_
extern STATUS
reconstruct_frames_from_fields(
    P_REC_PROJECTOR p_rec_projector)
{
    STATUS status = STATUS_OK;
    ARRAY_INDEX i;
    const P_DOCU p_docu = p_docu_from_docno(p_rec_projector->docno);
    PIXIT text_leading = default_text_leading(p_docu, p_rec_projector);
    SNAP_TO_CLICK_STOP_MODE snap_mode = SNAP_TO_CLICK_STOP_ROUND_COARSE;
    ARRAY_HANDLE old_h_rec_frames = p_rec_projector->h_rec_frames;

    p_rec_projector->h_rec_frames = 0;

    /* run though the fields in the table. create a frame for each visible field in the new array */
    for(i = 0; i < array_elements(&p_rec_projector->opendb.table.h_fielddefs); i++)
    {
        PC_FIELDDEF p_fielddef = array_ptrc(&p_rec_projector->opendb.table.h_fielddefs, FIELDDEF, i);
        P_REC_FRAME p_rec_frame;
        SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(*p_rec_frame), TRUE);

        if(p_fielddef->hidden)
            continue;

        if(NULL == (p_rec_frame = al_array_extend_by(&p_rec_projector->h_rec_frames, REC_FRAME, 1, &array_init_block, &status)))
            break;

        p_rec_frame->field_id = p_fielddef->id;
    }

    /* run over the frames we just created, now that they all exist, ensuring that they are where they ought to be etc. */
    for(i = 0; i < array_elements(&p_rec_projector->h_rec_frames); i++)
    {
        P_REC_FRAME p_rec_frame = array_ptr(&p_rec_projector->h_rec_frames, REC_FRAME, i);
        PC_FIELDDEF p_fielddef = p_fielddef_from_field_id(&p_rec_projector->opendb.table.h_fielddefs, p_rec_frame->field_id);
        /* try to find corresponding frame in the old array and copy its attributes */
        P_REC_FRAME p_rec_frame_old = p_rec_frame_from_field_id(&old_h_rec_frames, p_rec_frame->field_id);

        p_rec_frame->field_width = -1; /* Means unknown - will auto_width later if needed */

        if(P_DATA_NONE != p_rec_frame_old)
        {
            /* rec_frame has 0 handle at the moment */
            memcpy32(p_rec_frame, p_rec_frame_old, sizeof32(*p_rec_frame));
            p_rec_frame_old->h_title_text_ustr = 0; /* simply stolen !!! */
        }
        else
        {
            PIXIT_POINT size;

            /* unhiding a frame (or a newly created frame) - did it have a sensible previous size? */
            size.x = p_rec_frame->pixit_rect_field.br.x - p_rec_frame->pixit_rect_field.tl.x;
            size.y = p_rec_frame->pixit_rect_field.br.y - p_rec_frame->pixit_rect_field.tl.y;

            p_rec_frame->pixit_rect_field.tl.x = REC_DEFAULT_FRAME_X;
            p_rec_frame->pixit_rect_field.tl.y = REC_DEFAULT_FRAME_Y;

            { /* try to align this frame with all the other extant frames */
            ARRAY_INDEX i;

            for(i = 0; i < array_elements(&p_rec_projector->h_rec_frames); i++)
            {
                PC_REC_FRAME this_p_rec_frame = array_ptrc(&p_rec_projector->h_rec_frames, REC_FRAME, i);
                if(this_p_rec_frame == p_rec_frame)
                    continue;
                if( p_rec_frame->pixit_rect_field.tl.x < this_p_rec_frame->pixit_rect_field.tl.x)
                    p_rec_frame->pixit_rect_field.tl.x = this_p_rec_frame->pixit_rect_field.tl.x; /* directly underneath the left hand side */
                if( p_rec_frame->pixit_rect_field.tl.y < this_p_rec_frame->pixit_rect_field.br.y)
                    p_rec_frame->pixit_rect_field.tl.y = this_p_rec_frame->pixit_rect_field.br.y + text_leading / 4; /* but spaced away slightly */
            }
            } /*block*/

            /* realign to current grid */
            p_rec_frame->pixit_rect_field.tl.x = skel_ruler_snap_to_click_stop(p_docu, 1, p_rec_frame->pixit_rect_field.tl.x, snap_mode);
            p_rec_frame->pixit_rect_field.tl.y = skel_ruler_snap_to_click_stop(p_docu, 0, p_rec_frame->pixit_rect_field.tl.y, snap_mode);

            if(!size.x || !size.y)
                switch(p_fielddef->type)
                {
                default:
                    size.x = REC_DEFAULT_TEXT_FRAME_H;
                    size.y = text_leading;
                    break;

                case FIELD_BOOL:
                    size.x = REC_DEFAULT_BOOL_FRAME_H;
                    size.y = text_leading;
                    break;

                case FIELD_FILE:
                case FIELD_PICTURE:
                    size.x = REC_DEFAULT_PICT_FRAME_H;
                    size.y = REC_DEFAULT_PICT_FRAME_V;
                    break;
                }

            p_rec_frame->pixit_rect_field.br.x = p_rec_frame->pixit_rect_field.tl.x + size.x;
            p_rec_frame->pixit_rect_field.br.y = p_rec_frame->pixit_rect_field.tl.y + size.y;

            /* while we're at it, show title too - we can't know any better given the structure of the code */
            set_default_title_rect(p_rec_projector, p_rec_frame);

            p_rec_frame->title_show = TRUE;
        }

        /* SKS 26jun95 when making a field with a title that is currently empty, copy the field name as default so we can see what is happening */
        if(0 == p_rec_frame->h_title_text_ustr)
        {
            PIXIT width = p_rec_frame->pixit_rect_title.br.x - p_rec_frame->pixit_rect_title.tl.x;
            status = al_ustr_set(&p_rec_frame->h_title_text_ustr, p_fielddef->name);
            p_rec_frame->pixit_rect_title.tl.y = p_rec_frame->pixit_rect_field.tl.y; /* and set it alongside its field */
            p_rec_frame->pixit_rect_title.br.x = p_rec_frame->pixit_rect_field.tl.x;
            p_rec_frame->pixit_rect_title.br.y = p_rec_frame->pixit_rect_field.br.y;
            if(width <= 0)
                width = PIXITS_PER_INCH;
            p_rec_frame->pixit_rect_title.tl.x = p_rec_frame->pixit_rect_title.br.x - width;
        }
    }

    if(status_fail(status))
        /* hang on to what we had - will be inconsistent with fielddefs in any case */
        memswap32(&p_rec_projector->h_rec_frames, &old_h_rec_frames, sizeof32(p_rec_projector->h_rec_frames));

    /* dispose of the old array */
    rec_frames_dispose(&old_h_rec_frames);

    return(status);
}

/* end of project.c */
