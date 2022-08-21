/* rcio.c */

/* Copyright (C) 1994-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#include "common/gflags.h"

#include "ob_rec/ob_rec.h"

static DB_ID db_id_gen = 0x0DB00000;

static DB_ID db_id_map_from;
static DB_ID db_id_map_to;

static QUERY_ID current_query = (QUERY_ID) -1;

/* For rec_load_block/slot/finish etc */

static ARRAY_HANDLE static_h_matrix;
static COL static_matrix_cols;
static ROW static_matrix_rows;

T5_CMD_PROTO(extern, rec_io_table)
{
    STATUS status;
    const P_ARGLIST_ARG p_args = p_arglist_args(&p_t5_cmd->arglist_handle, 8);
    DB_ID db_id = (DB_ID) p_args[0].val.s32;
    PCTSTR filename = p_args[1].val.tstr;
    PROJECTOR_TYPE projector_type = (PROJECTOR_TYPE) p_args[2].val.s32;
    SLR slr;
    SLR size;
    BOOL adaptive_rows;
    P_REC_PROJECTOR p_rec_projector;

    IGNOREPARM_InVal_(t5_message);

    db_id_map_from = db_id;

    slr.col  = p_args[3].val.col;
    slr.row  = p_args[4].val.row;

    size.col = p_args[5].val.col;
    size.row = p_args[6].val.row; /* 0 for old files which had adaptive rows. fixed up later */

    if(arg_is_present(p_args, 7))
        adaptive_rows = p_args[7].val.fBool;
    else
        adaptive_rows = (size.row == 0);

    status_return(rec_instance_add_alloc(p_docu, &p_rec_projector));

    if(status_ok(status = rec_open_database(&p_rec_projector->opendb, filename)))
    {
        /* hide all fields to start with */
        ARRAY_INDEX i;

        for(i = 0; i < array_elements(&p_rec_projector->opendb.table.h_fielddefs); i++)
            array_ptr(&p_rec_projector->opendb.table.h_fielddefs, FIELDDEF, i)->hidden = TRUE;

        db_id_map_to = p_rec_projector->opendb.db.id;

        p_rec_projector->projector_type = projector_type;

        p_rec_projector->adaptive_rows = adaptive_rows;

        p_rec_projector->rec_docu_area.tl.slr = slr;
        p_rec_projector->rec_docu_area.br.slr.col = p_rec_projector->rec_docu_area.tl.slr.col + size.col;
        p_rec_projector->rec_docu_area.br.slr.row = p_rec_projector->rec_docu_area.tl.slr.row + size.row;

        status = rec_insert_projector_and_attach_with_styles(p_rec_projector, TRUE /*loading*/);
    }

    if(status_fail(status))
    {
        /* kill off what we've allocated so far */
        status = close_database(&p_rec_projector->opendb, status);
        rec_kill_projector(&p_rec_projector);
    }

    return(status);
}

T5_CMD_PROTO(extern, rec_io_frame)
{
    STATUS status = STATUS_OK;
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 9);
    DB_ID db_id = (DB_ID) p_args[0].val.s32;
    S32 j = p_args[1].val.s32;
    FIELD_ID field_id = (FIELD_ID) p_args[2].val.x32;
    PCTSTR fieldname = p_args[3].val.tstr;
    S32 x = p_args[4].val.s32;
    S32 y = p_args[5].val.s32;
    S32 w = p_args[6].val.s32;
    S32 h = p_args[7].val.s32;
    S32 z = p_args[8].val.s32;
    P_REC_PROJECTOR p_rec_projector;

    IGNOREPARM_InVal_(t5_message);

    IGNOREPARM(fieldname);
    IGNOREPARM(j);

    if(db_id != db_id_map_from)
        return(create_error(REC_ERR_BAD_DB_LOAD));

    p_rec_projector = p_rec_projector_from_db_id(p_docu, db_id_map_to);

    {
    P_FIELDDEF p_fielddef = p_fielddef_from_field_id(&p_rec_projector->opendb.table.h_fielddefs, field_id);

    if(P_DATA_NONE != p_fielddef)
    {
        P_REC_FRAME p_rec_frame;
        SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(*p_rec_frame), 1);

        if(NULL == (p_rec_frame = al_array_extend_by(&p_rec_projector->h_rec_frames, REC_FRAME, 1, &array_init_block, &status)))
            return(status);

        p_rec_frame->field_id = p_fielddef->id;

        p_rec_frame->pixit_rect_field.tl.x = x;
        p_rec_frame->pixit_rect_field.tl.y = y;
        p_rec_frame->pixit_rect_field.br.x = x + w;
        p_rec_frame->pixit_rect_field.br.y = y + h;

        p_rec_frame->field_width = z;

        p_fielddef->hidden = FALSE; /* field visible now */

        /* Default the title frame and mark it as hidden */
        set_default_title_rect(p_rec_projector, p_rec_frame);

        p_rec_frame->title_show = FALSE;

        p_rec_frame->h_title_text_ustr = 0;

        rec_recompute_card_size(p_rec_projector);
    }
    } /*block*/

    return(status);
}

T5_CMD_PROTO(extern, rec_io_title)
{
    STATUS status = STATUS_OK;
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 8);
    DB_ID db_id = (DB_ID) p_args[0].val.s32;
    S32 j = p_args[1].val.s32;
    FIELD_ID field_id = (FIELD_ID) p_args[2].val.x32;
    PC_USTR field_title = p_args[3].val.ustr; /* NOT fieldname */
    S32 x = p_args[4].val.s32;
    S32 y = p_args[5].val.s32;
    S32 w = p_args[6].val.s32;
    S32 h = p_args[7].val.s32;
    BOOL title_show = TRUE;
    P_REC_PROJECTOR p_rec_projector;
    ARRAY_INDEX i;

    IGNOREPARM_InVal_(t5_message);

    /* SKS 25jul95 save hidden titles, reload that state. cater for old visible titles (ie missing arg) */
    if(arg_is_present(p_args, 8))
        title_show = p_args[8].val.fBool;

    IGNOREPARM(j);

    if(db_id != db_id_map_from)
        return(create_error(REC_ERR_BAD_DB_LOAD));

    p_rec_projector = p_rec_projector_from_db_id(p_docu, db_id_map_to);

    for(i = 0; i < array_elements(&p_rec_projector->h_rec_frames); i++)
    {
        P_REC_FRAME p_rec_frame = array_ptr(&p_rec_projector->h_rec_frames, REC_FRAME, i);

        if(p_rec_frame->field_id != field_id)
            continue;

        p_rec_frame->pixit_rect_title.tl.x = x;
        p_rec_frame->pixit_rect_title.tl.y = y;
        p_rec_frame->pixit_rect_title.br.x = x + w;
        p_rec_frame->pixit_rect_title.br.y = y + h;

        p_rec_frame->title_show = title_show;

        al_array_dispose(&p_rec_frame->h_title_text_ustr);
        status = al_ustr_set(&p_rec_frame->h_title_text_ustr, field_title);

        break;
    }

    rec_recompute_card_size(p_rec_projector);

    return(status);
}

T5_CMD_PROTO(extern, rec_io_pattern)
{
    STATUS status;
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 5);
    PC_USTR ustr_text = p_args[0].val.ustr;
    FIELD_ID field_id = (FIELD_ID) p_args[1].val.x32;
    P_REC_PROJECTOR p_rec_projector = p_rec_projector_from_db_id(p_docu, db_id_map_to);
    S32 i;
    P_QUERY p_current_query;
    P_SEARCH_FIELD_PATTERN p_search_pattern;

    IGNOREPARM_InVal_(t5_message);

    myassert0x(ustr_text != NULL, TEXT("Null text pointer"));
    myassert0x(ustrlen32(ustr_text), TEXT("Empty string"));

    status_assert(i = fieldnumber_from_field_id(&p_rec_projector->opendb.table, field_id));

    i--; /* field numbers go 1... the array of searches goes 0... */

    p_current_query = p_query_from_p_opendb(&p_rec_projector->opendb, current_query);

    myassert0x(p_current_query->h_search_pattern != 0, TEXT("Null pattern handle"));

    p_search_pattern = array_ptr(&p_current_query->h_search_pattern, SEARCH_FIELD_PATTERN, i);

    p_search_pattern->field_id                 = field_id;

    p_search_pattern->sop_info.prefix_length   = p_args[2].val.s32;
    p_search_pattern->sop_info.suffix_length   = p_args[3].val.s32;
    p_search_pattern->sop_info.search_operator = p_args[4].val.s32;

    al_array_dispose(&p_search_pattern->h_text_ustr);
    status_assert(status = al_ustr_set(&p_search_pattern->h_text_ustr, ustr_text));

    return(status);
}

T5_CMD_PROTO(extern, rec_io_query)
{
    STATUS status;
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 6);
    PC_USTR ustr_query_name = p_args[0].val.ustr;
    QUERY_ID query_id   = (QUERY_ID) p_args[1].val.s32;
    QUERY_ID parent_id  = (QUERY_ID) p_args[2].val.s32;
    P_REC_PROJECTOR p_rec_projector = p_rec_projector_from_db_id(p_docu, db_id_map_to);
    P_QUERY p_query;

    IGNOREPARM_InVal_(t5_message);

    if(NULL != p_query_from_p_opendb(&p_rec_projector->opendb, query_id))
    {
        myassert0(TEXT("Duplicate query ids"));
        return(STATUS_FAIL);
    }

    { /* allocate a new one */
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(*p_query), TRUE);
    if(NULL == (p_query = al_array_extend_by(&p_rec_projector->opendb.search.h_query, QUERY, 1, &array_init_block, &status)))
         return(status);
    p_query->query_id = query_id;
    p_query->parent_id = parent_id;
    p_query->search_type    = p_args[3].val.s32;
    p_query->search_exclude = p_args[4].val.fBool;
    p_query->search_andor   = p_args[5].val.fBool;
    status_return(status = al_ustr_set(&p_query->h_name_ustr, ustr_query_name));
    } /*block*/

    {
    ARRAY_INDEX i;
    ARRAY_HANDLE h_search_pattern = 0;

    for(i = 0; i < array_elements(&p_rec_projector->opendb.table.h_fielddefs); i++)
    {
        SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(SEARCH_FIELD_PATTERN), TRUE);
        P_SEARCH_FIELD_PATTERN p_search_pattern;
        if(NULL == (p_search_pattern = al_array_extend_by(&h_search_pattern, SEARCH_FIELD_PATTERN, 1, &array_init_block, &status)))
            break;
        status_break(status = al_ustr_set(&p_search_pattern->h_text_ustr, ""));
        p_search_pattern->field_id = array_ptrc(&p_rec_projector->opendb.table.h_fielddefs, FIELDDEF, i)->id;
        p_search_pattern->sop_info.prefix_length = 0;
        p_search_pattern->sop_info.suffix_length = 0;
        p_search_pattern->sop_info.search_operator = REC_MSG_OPERATOR_DONTCARE;
    }

    if(status_ok(status))
        p_query->h_search_pattern = h_search_pattern;
    } /*block*/

    current_query = query_id;

    return(status);
}

_Check_return_
static STATUS
ob_rec_save_projector(
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format,
    P_REC_PROJECTOR p_rec_projector)
{
    STATUS status;
    const OBJECT_ID object_id = OBJECT_ID_REC;
    ARGLIST_HANDLE arglist_handle;
    PC_CONSTRUCT_TABLE p_construct_table;

    if(status_ok(status = arglist_prepare_with_construct(&arglist_handle, object_id, T5_CMD_RECORDZ_IO_TABLE, &p_construct_table)))
    {
        const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 8);
        TCHARZ canonical_filename[BUF_MAX_PATHSTRING];
        p_args[0].val.s32    = p_rec_projector->opendb.db.id;
        (void) os_canonicalise(p_of_op_format->output.u.file.filename, canonical_filename, sizeof32(canonical_filename));
        p_args[1].val.tstr   = localise_filename(canonical_filename, p_rec_projector->opendb.db.name);
        p_args[2].val.s32    = p_rec_projector->projector_type;
        p_args[3].val.s32    = p_rec_projector->rec_docu_area.tl.slr.col;
        p_args[4].val.s32    = p_rec_projector->rec_docu_area.tl.slr.row;
        p_args[5].val.s32    = p_rec_projector->rec_docu_area.br.slr.col - p_rec_projector->rec_docu_area.tl.slr.col;
        p_args[6].val.s32    = p_rec_projector->rec_docu_area.br.slr.row - p_rec_projector->rec_docu_area.tl.slr.row;
        p_args[7].val.fBool  = p_rec_projector->adaptive_rows;
        status = ownform_save_arglist(arglist_handle, object_id, p_construct_table, p_of_op_format);
        arglist_dispose(&arglist_handle);
    }

    status_return(status);

    if(PROJECTOR_TYPE_SHEET == p_rec_projector->projector_type)
        rec_sheet_stash_field_widths(p_rec_projector);

    { /* Now we should save out any frame constructs for this table */
    ARRAY_INDEX frame_index;

    for(frame_index = 0; frame_index < array_elements(&p_rec_projector->h_rec_frames); ++frame_index)
    {
        PC_REC_FRAME p_rec_frame = array_ptrc(&p_rec_projector->h_rec_frames, REC_FRAME, frame_index);
        PC_FIELDDEF p_fielddef = p_fielddef_from_field_id(&p_rec_projector->opendb.table.h_fielddefs, p_rec_frame->field_id);

        if(status_ok(status = arglist_prepare_with_construct(&arglist_handle, object_id, T5_CMD_RECORDZ_IO_FRAME, &p_construct_table)))
        {
            const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 9);
            p_args[0].val.s32   = p_rec_projector->opendb.db.id;
            p_args[1].val.s32   = frame_index;
            p_args[2].val.s32   = p_rec_frame->field_id;
            p_args[3].val.tstr  = (P_DATA_NONE != p_fielddef) ? p_fielddef->name : tstr_empty_string;
            p_args[4].val.s32   = p_rec_frame->pixit_rect_field.tl.x;
            p_args[5].val.s32   = p_rec_frame->pixit_rect_field.tl.y;
            p_args[6].val.s32   = p_rec_frame->pixit_rect_field.br.x - p_rec_frame->pixit_rect_field.tl.x;
            p_args[7].val.s32   = p_rec_frame->pixit_rect_field.br.y - p_rec_frame->pixit_rect_field.tl.y;
            p_args[8].val.s32   = p_rec_frame->field_width;
            status = ownform_save_arglist(arglist_handle, object_id, p_construct_table, p_of_op_format);
            arglist_dispose(&arglist_handle);
        }

        status_break(status);

        /* now do the field titles */
        if(status_ok(status = arglist_prepare_with_construct(&arglist_handle, object_id, T5_CMD_RECORDZ_IO_TITLE, &p_construct_table)))
        {
            const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 9);
            p_args[0].val.s32   = p_rec_projector->opendb.db.id;
            p_args[1].val.s32   = frame_index;
            p_args[2].val.s32   = p_rec_frame->field_id;
            p_args[3].val.ustr  = array_ustr(&p_rec_frame->h_title_text_ustr);
            p_args[4].val.s32   = p_rec_frame->pixit_rect_title.tl.x;
            p_args[5].val.s32   = p_rec_frame->pixit_rect_title.tl.y;
            p_args[6].val.s32   = p_rec_frame->pixit_rect_title.br.x - p_rec_frame->pixit_rect_title.tl.x;
            p_args[7].val.s32   = p_rec_frame->pixit_rect_title.br.y - p_rec_frame->pixit_rect_title.tl.y;
            p_args[8].val.fBool = p_rec_frame->title_show;
            status = ownform_save_arglist(arglist_handle, object_id, p_construct_table, p_of_op_format);
            arglist_dispose(&arglist_handle);
        }

        status_break(status);
    }

    status_return(status);
    } /*block*/

    { /* Now we should save out any query constructs for this table */
    ARRAY_INDEX query_index;

    for(query_index = 0; query_index < array_elements(&p_rec_projector->opendb.search.h_query); ++query_index)
    {
        P_QUERY p_query = array_ptr(&p_rec_projector->opendb.search.h_query, QUERY, query_index);
        ARRAY_INDEX query_frame_index;

        if(status_ok(status = arglist_prepare_with_construct(&arglist_handle, object_id, T5_CMD_RECORDZ_IO_QUERY, &p_construct_table)))
        {
            const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 6);
            p_args[0].val.ustr  = array_ustr(&p_query->h_name_ustr); /* textual name */
            p_args[1].val.s32   = (S32) p_query->query_id;
            p_args[2].val.s32   = (S32) p_query->parent_id; /* parent id (or -1) */
            p_args[3].val.s32   = p_query->search_type;
            p_args[4].val.s32   = p_query->search_exclude;
            p_args[5].val.s32   = p_query->search_andor;
            status = ownform_save_arglist(arglist_handle, object_id, p_construct_table, p_of_op_format);
            arglist_dispose(&arglist_handle);
        }

        status_break(status);

        for(query_frame_index = 0; query_frame_index < array_elements(&p_rec_projector->h_rec_frames); ++query_frame_index)
        {
            P_SEARCH_FIELD_PATTERN p_search_pattern = array_ptr(&p_query->h_search_pattern, SEARCH_FIELD_PATTERN, query_frame_index);

            if(0 != array_elements(&p_search_pattern->h_text_ustr))
            {
                PC_USTR ustr_search_text = array_ustr(&p_search_pattern->h_text_ustr);

                if(0 != ustrlen32(ustr_search_text))
                {
                    if(status_ok(status = arglist_prepare_with_construct(&arglist_handle, object_id, T5_CMD_RECORDZ_IO_PATTERN, &p_construct_table)))
                    {
                        const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 5);
                        p_args[0].val.ustr  = ustr_search_text;
                        p_args[1].val.s32   = p_search_pattern->field_id;
                        p_args[2].val.s32   = p_search_pattern->sop_info.prefix_length;
                        p_args[3].val.s32   = p_search_pattern->sop_info.suffix_length;
                        p_args[4].val.s32   = p_search_pattern->sop_info.search_operator;
                        status = ownform_save_arglist(arglist_handle, object_id, p_construct_table, p_of_op_format);
                        arglist_dispose(&arglist_handle);
                    }
                }
            }

            status_break(status);
        }
    }

    status_return(status);
    } /*block*/

    return(STATUS_OK);
}

_Check_return_
extern STATUS
ob_rec_save(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format)
{
    STATUS status = STATUS_OK;

    if(p_of_op_format->of_template.data_class >= DATA_SAVE_WHOLE_DOC) /* SKS 20apr95 was MANY */
    {
        P_REC_INSTANCE_DATA p_rec_instance = p_object_instance_data_REC(p_docu);
        ARRAY_INDEX projector_index;

        for(projector_index = 0; projector_index < array_elements(&p_rec_instance->array_handle); ++projector_index)
        {
            P_REC_PROJECTOR p_rec_projector = array_ptr(&p_rec_instance->array_handle, REC_PROJECTOR, projector_index);

            if(!p_rec_projector->opendb.dbok)
                continue;

            if(!docu_area_in_docu_area(&p_of_op_format->save_docu_area, &p_rec_projector->rec_docu_area))
                continue;

            p_of_op_format->saved_database_constructs = 1; /* SKS 14apr96 */

            status_break(status = ob_rec_save_projector(p_of_op_format, p_rec_projector));
        }
    }

    return(status);
}

/* Called before the notelayer gets its oar in so that we can import files */

_Check_return_
extern STATUS
rec_event_fileinsert_doinsert_1(
    _DocuRef_   P_DOCU p_docu,
    P_SKELEVENT_CLICK p_skelevent_click)
{
    STATUS status = STATUS_OK;
    T5_FILETYPE t5_filetype = p_skelevent_click->data.fileinsert.t5_filetype;
    PCTSTR filename = p_skelevent_click->data.fileinsert.filename;
    /*S32 safesource  = p_skelevent_click->data.fileinsert.safesource;*/

    SLR slr;
    SKEL_POINT tl;
    DATA_REF data_ref;
    P_REC_PROJECTOR p_rec_projector;
    P_FIELDDEF p_fielddef;

    if(status_fail(slr_owner_from_skel_point(p_docu, &slr, &tl, &p_skelevent_click->skel_point, ON_ROW_EDGE_GO_UP)))
        return(STATUS_OK);

    if(NULL == (p_rec_projector = rec_data_ref_from_slr_and_skel_point(p_docu, &slr, &p_skelevent_click->skel_point, &data_ref)))
    {
        if(FILETYPE_DATAPOWER == t5_filetype)
            return(STATUS_OK); /* keep going in either case - import outside rec area or into new document */

        if(FILETYPE_CSV == t5_filetype)
            return(STATUS_OK); /* will fault itself it it needs */

        if(OBJECT_ID_REC_FLOW == p_docu->object_id_flow)
            return(create_error(REC_ERR_CANT_IMPORT));

        return(STATUS_OK);
    }

    /* return STATUS_DONE and p_skelevent_click->processed iff you've dealt with it */

    if(P_DATA_NONE == (p_fielddef = p_fielddef_from_field_id(&p_rec_projector->opendb.table.h_fielddefs, data_ref.arg.db_field.field_id)))
    {
        myassert0(TEXT("Bad data ref"));
        status = STATUS_FAIL;
    }
    else
    {
        switch(p_fielddef->type)
        {
        default:
            myassert0(TEXT("Field type not able to import data"));
            break;

        case FIELD_PICTURE:
            {
            BOOL allow_import = FALSE;

            switch(t5_filetype)
            {
            case FILETYPE_DRAW:
            case FILETYPE_SPRITE:
            case FILETYPE_WINDOWS_BMP:
                allow_import = TRUE;
                break;

            default:
                break;
            }

            if(!allow_import)
                break;
            }

            /*FALLTHRU*/

        case FIELD_FILE:
            /* Any filetype will do */

#if RISCOS
            { /* SKS 10nov96 take note of embed state. 13oct99 Ctrl *inverts* embed state */
            BOOL desire_embed = global_preferences.embed_inserted_files;

            if(host_ctrl_pressed()) desire_embed = !desire_embed;

            if(!desire_embed)
            {
                if(status_ok(status = rec_link_file(p_docu, &data_ref, filename, t5_filetype)))
                /* SKS 12may95 pass out failure codes from this */
                {
                    p_skelevent_click->processed = 1;
                    return(STATUS_DONE);
                }

                myassert0(TEXT("File rejected"));
                break;
            }
            } /*block*/
#endif

            if(status_ok(status = rec_import_file(p_docu, &data_ref, filename, t5_filetype)))
            {
                p_skelevent_click->processed = 1;
                return(STATUS_DONE);
            }

            myassert0(TEXT("File rejected"));
            break;
        }
    }

    IGNOREPARM(status);

    /* limited import currently allowed into running databases */
    if(FILETYPE_DATAPOWER == t5_filetype)
        return(STATUS_OK);

    return(create_error(REC_ERR_CANT_IMPORT));
}

_Check_return_
static STATUS
rec_load_fragment(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_OF_IP_FORMAT p_of_ip_format,
    _In_z_      PC_USTR_INLINE ustr_inline_frag_start)
{
    const OBJECT_ID object_id = OBJECT_ID_REC;
    LOAD_CELL_OWNFORM load_cell_ownform_stt_frag;
    zero_struct(load_cell_ownform_stt_frag);
    load_cell_ownform_stt_frag.ustr_inline_contents = ustr_inline_frag_start;
  /*load_cell_ownform_stt_frag.ustr_formula = NULL;*/

    load_cell_ownform_stt_frag.original_slr = p_of_ip_format->original_docu_area.tl.slr;

    /* call object to load in start fragment */
    return(insert_cell_contents_ownform(p_docu, object_id, T5_MSG_LOAD_CELL_OWNFORM, &load_cell_ownform_stt_frag, &p_of_ip_format->insert_position));
}

/******************************************************************************
*
* Takes a block construct and blasts a hole in the document, inserting fragments if present.
*
******************************************************************************/

_Check_return_
static STATUS
rec_cmd_of_block(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_CONSTRUCT_CONVERT p_construct_convert)
{
    P_OF_IP_FORMAT p_of_ip_format = p_construct_convert->p_of_ip_format;
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_construct_convert->arglist_handle, ARG_BLOCK_N_ARGS);
    PC_USTR_INLINE frag_start = NULL;
    DATA_REF data_ref;

    trace_0(TRACE_APP_DPLIB, TEXT("rec_cmd_of_block called"));

    al_array_dispose(&static_h_matrix); /* ensure we don't have one */

    /* NB. these args aren't COL,ROW as COL,ROW arg processing during loading offset using the original_docu_area set up here! */

    /* Top left position */
    p_of_ip_format->original_docu_area.tl.slr.col                   = p_args[ARG_BLOCK_TL_COL].val.col;
    p_of_ip_format->original_docu_area.tl.slr.row                   = p_args[ARG_BLOCK_TL_ROW].val.row;
    p_of_ip_format->original_docu_area.tl.object_position.data      = p_args[ARG_BLOCK_TL_DATA].val.s32;
    p_of_ip_format->original_docu_area.tl.object_position.object_id = p_args[ARG_BLOCK_TL_OBJECT_ID].val.object_id;

    /* Bottom right position */
    p_of_ip_format->original_docu_area.br.slr.col                   = p_args[ARG_BLOCK_BR_COL].val.col;
    p_of_ip_format->original_docu_area.br.slr.row                   = p_args[ARG_BLOCK_BR_ROW].val.row;
    p_of_ip_format->original_docu_area.br.object_position.data      = p_args[ARG_BLOCK_BR_DATA].val.s32;
    p_of_ip_format->original_docu_area.br.object_position.object_id = p_args[ARG_BLOCK_BR_OBJECT_ID].val.object_id;

    p_of_ip_format->original_docu_area.whole_col =
    p_of_ip_format->original_docu_area.whole_row = 0;

    /* Start fragment? */
    if(arg_is_present(p_args, ARG_BLOCK_FA))
        frag_start = p_args[ARG_BLOCK_FA].val.ustr_inline;

    if(frag_start && docu_area_is_frag(&p_of_ip_format->original_docu_area))
#if 1
    {
        return(rec_load_fragment(p_docu, p_of_ip_format, frag_start));
    }
#else
    { /* tried this but it only works from database to database */
        return(skeleton_load_construct(p_docu, p_construct_convert));
    }
#endif

    cur_change_before(p_docu);

    /* flag document has data */
    p_docu->flags.has_data = 1;

/*
We will soon be getting some data to deal with in the rec load_slot code

use p_of_ip_format->insert_pos.slr to decide which database to target

the number of incoming records to add is (p_of_ip_format->original_docu_area.br.slr.row - p_of_ip_format->original_docu_area.tl.slr.row)
each of which has (p_of_ip_format->original_docu_area.br.slr.col - p_of_ip_format->original_docu_area.tl.slr.col) fields

er we might not get them all though!

notes:

1 Flush the cache before hand
2 like add, revert to whole file
3 build and m,n array of array handles with SLRs

load_slot fills them in

on finish purge on a record by record basis, adding each record as you go

update the number of rows in the projector? NO
*/
    if(!rec_data_ref_from_slr(p_docu, &p_of_ip_format->insert_position.slr, &data_ref))
        return(create_error(REC_ERR_NO_DATABASE));

    trace_0(TRACE_APP_DPLIB, TEXT("issuing cur_change_before"));
    cur_change_before(p_docu);

    rec_revert_whole(p_rec_projector_from_db_id(p_docu, data_ref.arg.db_field.db_id));

    static_matrix_rows = (p_of_ip_format->original_docu_area.br.slr.row - p_of_ip_format->original_docu_area.tl.slr.row);
    static_matrix_cols = (p_of_ip_format->original_docu_area.br.slr.col - p_of_ip_format->original_docu_area.tl.slr.col);

    trace_2(TRACE_APP_DPLIB, TEXT("Load At row=") ROW_TFMT TEXT(" col=") COL_TFMT, p_of_ip_format->insert_position.slr.row, p_of_ip_format->insert_position.slr.col);
    trace_2(TRACE_APP_DPLIB, TEXT("Load Records") ROW_TFMT TEXT(" fields=") COL_TFMT, static_matrix_rows, static_matrix_cols);

    return(STATUS_OK);
}

_Check_return_
static STATUS
rec_cmd_of_end_of_data(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_CONSTRUCT_CONVERT p_construct_convert)
{
    P_OF_IP_FORMAT p_of_ip_format = p_construct_convert->p_of_ip_format;
    STATUS status = STATUS_OK;
    COL n_cols;
    S32 startfield;
    DATA_REF data_ref;
    P_REC_PROJECTOR p_rec_projector;
    P_RCBUFFER p_rcbuffer;
    S32 i, j, fieldnumber;
    ARRAY_INDEX ix;

    /*
    We just had some data to deal with in the rec load_slot code
    use p_of_ip_format->insert_pos.slr to decide which database to target
    */
    trace_2(TRACE_APP_DPLIB, TEXT("rec_cmd_of_end_of_data() for DB At row=") ROW_TFMT TEXT(" col=") COL_TFMT, p_of_ip_format->insert_position.slr.row, p_of_ip_format->insert_position.slr.col);

    if(NULL == (p_rec_projector = rec_data_ref_from_slr(p_docu, &p_of_ip_format->insert_position.slr, &data_ref)))
        return(create_error(REC_ERR_NO_DATABASE));

    if(!p_rec_projector->opendb.dbok)
        return(create_error(REC_ERR_DB_NOT_OK));

    status_return(table_check_access(&p_rec_projector->opendb.table, ACCESS_FUNCTION_ADD));

    myassert1x(data_ref.data_space == DATA_DB_FIELD, TEXT("Not DATA_DB_FIELD ") S32_TFMT, data_ref.data_space);

    if(PROJECTOR_TYPE_CARD == p_rec_projector->projector_type)
        startfield = 0;
    else
        startfield = (p_of_ip_format->insert_position.slr.col - p_rec_projector->rec_docu_area.tl.slr.col);

    n_cols = static_matrix_cols;

    for(i = 0 ; i < static_matrix_rows; i++)
    {
        /* add a record */
        S32 old_recno  = p_rec_projector->opendb.recordspec.recno;
        S32 old_ncards = p_rec_projector->opendb.recordspec.ncards;
        S32 new_recno  = -1;
        S32 new_ncards = -1;

        IGNOREPARM(old_recno);
        IGNOREPARM(old_ncards);

        status = record_view_recnum(&p_rec_projector->opendb, data_ref.arg.db_field.record);

        if(status_ok(status))
            status = record_add_commence(&p_rec_projector->opendb, TRUE);

        if(status_ok(status))
        {
            /* second argument (TRUE for add blank, FALSE for copy )
               last argument   (TRUE means adding, FALSE for editing ) */

            status = record_edit_confirm (&p_rec_projector->opendb, TRUE, TRUE );

            if(status_ok(status))
            {
                if(status_ok(record_current(&p_rec_projector->opendb, &new_recno, &new_ncards)))
                    p_rec_projector->opendb.recordspec.ncards = new_ncards;
                else
                    new_recno = -1;
            }
        }
        else
            status = record_edit_cancel(&p_rec_projector->opendb, TRUE);

        if(new_recno !=-1)
        {
            DATA_REF data_ref_mine;
            trace_1(TRACE_APP_DPLIB, TEXT("tracked record to ") S32_TFMT, new_recno);

            data_ref_mine = data_ref;
            data_ref_mine.arg.db_field.record = new_recno;

            /* for all fields write the data to the record */
            for(j = 0; j < n_cols; j++)
            {
                fieldnumber = startfield+j+1;

                if((U32) fieldnumber <= array_elements32(&p_rec_projector->opendb.table.h_fielddefs))
                {
                    set_rec_data_ref_field_by_number(p_docu, &data_ref_mine, fieldnumber);

                    ix = (i * n_cols) + j;

                    trace_1(TRACE_APP_DPLIB, TEXT("Row index is ") S32_TFMT, (i));
                    trace_1(TRACE_APP_DPLIB, TEXT("Col index is ") S32_TFMT, (j));
                    trace_1(TRACE_APP_DPLIB, TEXT("Array index is ") S32_TFMT, (ix));

                    p_rcbuffer = array_ptr(&static_h_matrix, RCBUFFER, ix);

                    if(0 != array_elements(&p_rcbuffer->h_text_ustr))
                    {
                        trace_1(TRACE_APP_DPLIB, TEXT("Data is %s"), array_ustr(&p_rcbuffer->h_text_ustr));

                        status = rec_cache_update(p_docu, &data_ref_mine, &p_rcbuffer->h_text_ustr, TRUE);
                        myassert0x(status_done(status), TEXT("wot happened"));
                        if(status_done(status))
                            /* the text is now in the cache */
                            p_rcbuffer->h_text_ustr = 0;
                    }
                    else
                        trace_0(TRACE_APP_DPLIB, TEXT("Data is null"));
                }
            } /* next field */

            /* force it to be output before starting on the next record */
            status = rec_cache_purge(p_docu);
        }
        else
            myassert0(TEXT("Lost track of the record"));
    } /* next record */

    cur_change_after(p_docu);

    { /* flush 'im down the plughole */
    ARRAY_INDEX i;
    for(i = 0; i < array_elements(&static_h_matrix); i++)
    {
        P_RCBUFFER p_rcbuffer = array_ptr(&static_h_matrix, RCBUFFER, i);
        al_array_dispose(&p_rcbuffer->h_text_ustr);
    }
    al_array_dispose(&static_h_matrix);
    } /*block*/

    static_matrix_rows = -1;
    static_matrix_cols = 0;

    return(status);
}

/******************************************************************************
*
* Decode construct information for ownform cell construct, then call relevant object id
*
******************************************************************************/

_Check_return_
static STATUS
rec_cmd_of_cell(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_CONSTRUCT_CONVERT p_construct_convert)
{
    P_OF_IP_FORMAT p_of_ip_format = p_construct_convert->p_of_ip_format;
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_construct_convert->arglist_handle, ARG_CELL_N_ARGS);
    LOAD_CELL_OWNFORM load_cell_ownform;
    OBJECT_ID object_id;
    POSITION position;
    DATA_REF data_ref;
    STATUS status = STATUS_OK;

    trace_0(TRACE_APP_DPLIB, TEXT("rec_cmd_of_cell called"));

    zero_struct(load_cell_ownform);

    if(status_fail(status = object_id_from_construct_id(p_args[ARG_CELL_OWNER].val.u8c, &object_id)))
    {
        if(status != STATUS_FAIL)
            return(status);
        status = STATUS_OK;
    }

    load_cell_ownform.data_type = OWNFORM_DATA_TYPE_TEXT;

    {
    U8 owner = p_args[ARG_CELL_DATA_TYPE].val.u8c;

    if(owner && status_fail(data_type_from_construct_id(owner, &load_cell_ownform.data_type)))
    {
        p_of_ip_format->flags.unknown_data_type = 1;
        return(STATUS_OK);
    }
    } /*block*/

#if 1
    /* already offset using COL,ROW arg processing */
    position.slr.col = p_args[ARG_CELL_COL].val.col;
    position.slr.row = p_args[ARG_CELL_ROW].val.row;
    position.object_position.object_id = OBJECT_ID_NONE;

    load_cell_ownform.original_slr.col = position.slr.col + (p_of_ip_format->original_docu_area.tl.slr.col /*- p_of_ip_format->insert_position.slr.col*/);
    load_cell_ownform.original_slr.row = position.slr.row + (p_of_ip_format->original_docu_area.tl.slr.row /*- p_of_ip_format->insert_position.slr.row*/);
    /* what about p_of_ip_format->insert_pos.slr.col,row ??? */
#else
    load_cell_ownform.original_slr.col = p_args[ARG_CELL_COL].val.col;
    load_cell_ownform.original_slr.row = p_args[ARG_CELL_ROW].val.row;

    position.slr.col = (load_cell_ownform.original_slr.col - p_of_ip_format->original_docu_area.tl.slr.col);
    position.slr.row = (load_cell_ownform.original_slr.row - p_of_ip_format->original_docu_area.tl.slr.row);
    position.object_position.object_id = OBJECT_ID_NONE;
#endif

    region_from_docu_area_max(&load_cell_ownform.region_saved, &p_of_ip_format->original_docu_area);

    load_cell_ownform.ustr_inline_contents = p_args[ARG_CELL_CONTENTS].val.ustr_inline;
    load_cell_ownform.ustr_formula         = p_args[ARG_CELL_FORMULA ].val.ustr;
    load_cell_ownform.tstr_mrofmun_style   = p_args[ARG_CELL_MROFMUN ].val.tstr;

    if(!load_cell_ownform.ustr_inline_contents && !load_cell_ownform.ustr_formula)
        return(STATUS_OK);

    /* Here we have a pointer to some text (null terminated string) (load_cell_ownform.p_contents) and
       use p_of_ip_format->insert_pos.slr to decide which database to target
       position.slr === the record/field to get the text
    */
    if(!rec_data_ref_from_slr(p_docu, &p_of_ip_format->insert_position.slr, &data_ref))
        return(create_error(REC_ERR_NO_DATABASE));

    /* Invent a vaque data ref for the item */
    set_record_for_rec_data_ref(&data_ref, position.slr.row);
    set_rec_data_ref_field_by_number(p_docu, &data_ref, position.slr.col);

    trace_2(TRACE_APP_DPLIB, TEXT("load item record") ROW_TFMT TEXT(" field") COL_TFMT, position.slr.row, position.slr.col);
    trace_1(TRACE_APP_DPLIB, TEXT("Data is %s"), load_cell_ownform.ustr_inline_contents);

    assert(position.slr.col >= 0);
    assert(position.slr.row >= 0);
    assert(position.slr.col < static_matrix_cols);
    assert(position.slr.row < static_matrix_rows);

    if(!static_h_matrix) /* SKS 26jun95 allocate on demand */
    {
        SC_ARRAY_INIT_BLOCK aib = aib_init(1, sizeof32(RCBUFFER), TRUE);
        if(NULL == al_array_alloc(&static_h_matrix, RCBUFFER, static_matrix_cols * static_matrix_rows, &aib, &status))
            return(status);
    }

    {
    ARRAY_INDEX i = (position.slr.row * static_matrix_cols) + position.slr.col;
    P_RCBUFFER p_rcbuffer = array_ptr(&static_h_matrix, RCBUFFER, i);
    assert(0 == p_rcbuffer->h_text_ustr);
    assert(load_cell_ownform.ustr_inline_contents);
    assert(!contains_inline(load_cell_ownform.ustr_inline_contents, ustr_inline_strlen32(load_cell_ownform.ustr_inline_contents)));
    status = al_ustr_set(&p_rcbuffer->h_text_ustr, (PC_USTR) load_cell_ownform.ustr_inline_contents);
    p_rcbuffer->data_ref = data_ref;
    p_rcbuffer->modified = TRUE;
    } /*block*/

    return(STATUS_OK);
}

_Check_return_
extern STATUS
rec_load_construct_ownform(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_CONSTRUCT_CONVERT p_construct_convert)
{
    const T5_MESSAGE t5_message = p_construct_convert->p_construct->t5_message;

    trace_0(TRACE_APP_DPLIB, TEXT("rec_load_construct_ownform called"));

    switch(t5_message)
    {
    case T5_CMD_OF_BLOCK:
        return(rec_cmd_of_block(p_docu, p_construct_convert));

    case T5_CMD_OF_END_OF_DATA:
        return(rec_cmd_of_end_of_data(p_docu, p_construct_convert));

    case T5_CMD_OF_BASE_REGION:
    case T5_CMD_OF_REGION:
    case T5_CMD_OF_BASE_IMPLIED_REGION:
    case T5_CMD_OF_IMPLIED_REGION:
        /* don't pass these on */
        return(STATUS_OK);

    case T5_CMD_OF_CELL:
        return(rec_cmd_of_cell(p_docu, p_construct_convert));

    default:
        return(skeleton_load_construct(p_docu, p_construct_convert));
    }
}

_Check_return_
extern STATUS
rec_open_database(
    P_OPENDB p_opendb,
    _In_z_      PCTSTR filename)
{
    /* Check that one is not already open, & close it if it is */
    if(p_opendb->dbok)
        status_return(close_database(p_opendb, STATUS_OK));

    status_return(open_database(p_opendb, filename));

    /* Set up a unique Recordz db_id for this opendb */
    p_opendb->db.id = db_id_gen++;

    /* Ensure we have a (whole database) cursor and position to the first position */
    return(ensure_cursor_current(p_opendb, 0));
}

/* This is called from the drag-in-a-datapower-file code when there is already a database */

_Check_return_
static STATUS
rec_append_foreign(
    _InoutRef_  P_MSG_INSERT_FOREIGN p_msg_insert_foreign,
    P_REC_PROJECTOR p_rec_projector)
{
    STATUS status = STATUS_OK;
    OPENDB new_opendb;
    QUICK_TBLOCK_WITH_BUFFER(aqtb_buffer, 40);
    QUICK_TBLOCK_WITH_BUFFER(aqtb_tempname, 40);
    quick_tblock_with_buffer_setup(aqtb_buffer);
    quick_tblock_with_buffer_setup(aqtb_tempname);

    zero_struct(new_opendb);

    status_return(open_database(&new_opendb, p_msg_insert_foreign->filename));

    new_opendb.db.id = db_id_gen++; /* Set up a unique Recordz db_id for this opendb */

    /* Put the (whole database) cursor to the first position */
    if(status_fail(status = ensure_cursor_current(&new_opendb, 0)))
        return(close_database(&new_opendb, status));

    if(new_opendb.recordspec.ncards == 0)
        /* Eek! the database is empty */
        return(close_database(&new_opendb, status));

    status_assert(quick_tblock_tstr_add_n(&aqtb_buffer, p_rec_projector->opendb.db.name, strlen_with_NULLCH));

    file_dirname(quick_tblock_tchars_wr(&aqtb_buffer), p_rec_projector->opendb.db.name);

    status = file_tempname(quick_tblock_tstr(&aqtb_buffer), TEXT("db"), NULL, 0, &aqtb_tempname);

    /* If the table structures are "identical" we can just remap_append
       otherwise we shall have to do a remap_similar

       Actually in the case where is_table_contained_by_table returns true
       remap_similar and remap_append are degenerate.
    */

    if(is_table_contained_by_table(&p_rec_projector->opendb.table, &new_opendb.table))
        status = remap_append(&p_rec_projector->opendb, &new_opendb, quick_tblock_tstr(&aqtb_tempname));
    else
        status = remap_similar(&p_rec_projector->opendb, &new_opendb, quick_tblock_tstr(&aqtb_tempname));

    status = close_database(&new_opendb, status);

    if(status_ok(status))
        switch_database(&p_rec_projector->opendb, quick_tblock_tstr(&aqtb_tempname));

    quick_tblock_dispose(&aqtb_tempname);
    quick_tblock_dispose(&aqtb_buffer);

    if(status_ok(status))
        /* The number of fields may have increased so we may need to create some additional frames */
        status = reconstruct_frames_from_fields(p_rec_projector);

    rec_update_projector_adjust(p_rec_projector);

    return(status);
}

/* This is called from the drag-in-a-datapower-file code */

_Check_return_
extern STATUS
rec_msg_insert_foreign(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_MSG_INSERT_FOREIGN p_msg_insert_foreign)
{
    P_REC_PROJECTOR p_rec_projector;
    STATUS status;

    { /* Determine whether a projector exists for this already and if not then create a new one */
    DATA_REF data_ref;
    if(NULL != (p_rec_projector = rec_data_ref_from_slr(p_docu, &p_msg_insert_foreign->position.slr, &data_ref)))
        return(rec_append_foreign(p_msg_insert_foreign, p_rec_projector));
    } /*block*/

    if(p_msg_insert_foreign->scrap_file)
        return(create_error(REC_ERR_CANT_LOAD_SCRAP)); /* but we can import it ^^^ */

    status_return(rec_instance_add_alloc(p_docu, &p_rec_projector));

    if(status_ok(status = rec_open_database(&p_rec_projector->opendb, p_msg_insert_foreign->filename)))
    {
        if(status_ok(status = table_check_access(&p_rec_projector->opendb.table, ACCESS_FUNCTION_READ))) /* SKS 26jun95 check early */
        {
            RECORDZ_DEFAULTS recordz_defaults;
            SLR size;

            {
            P_RECB_INSTANCE_DATA p_recb_instance = p_object_instance_data_RECB(p_docu);

            if(!p_recb_instance->recordz_defaults_valid)
                p_recb_instance = p_object_instance_data_RECB(p_docu_from_config_wr());

            if(p_recb_instance->recordz_defaults_valid)
                recordz_defaults = p_recb_instance->recordz_defaults;
            else
            {
                recordz_defaults.projector_type = PROJECTOR_TYPE_SHEET;
                recordz_defaults.cols = 0;
                recordz_defaults.rows = 1;
            }
            } /*block*/

            p_rec_projector->projector_type = recordz_defaults.projector_type;

            if(PROJECTOR_TYPE_CARD == p_rec_projector->projector_type)
            {
                if(recordz_defaults.cols > 0)
                    size.col = (COL) recordz_defaults.cols;
                else
                    size.col = n_cols_logical(p_docu);
            }
            else
                size.col = (COL) array_elements(&p_rec_projector->opendb.table.h_fielddefs); /* none hidden */

            if(recordz_defaults.rows > 0)
            {
                size.row = recordz_defaults.rows;
                p_rec_projector->adaptive_rows = 0;
            }
            else
            {
                size.row = 0;
                p_rec_projector->adaptive_rows = 1;
            }

            if(OBJECT_ID_REC_FLOW == p_docu->object_id_flow)
            {
                /* rec_flow template? then we want to put it at A1 regardless */
                p_rec_projector->rec_docu_area.tl.slr.row = 0;
                p_rec_projector->rec_docu_area.tl.slr.col = 0;
            }
            else
                p_rec_projector->rec_docu_area.tl.slr = p_msg_insert_foreign->position.slr;

            p_rec_projector->rec_docu_area.br.slr.col = p_rec_projector->rec_docu_area.tl.slr.col + size.col;
            p_rec_projector->rec_docu_area.br.slr.row = p_rec_projector->rec_docu_area.tl.slr.row + size.row;

            if(status_ok(status = reconstruct_frames_from_fields(p_rec_projector)))
                status = rec_insert_projector_and_attach_with_styles(p_rec_projector, FALSE);
        }
    }

    if(status_fail(status))
    {
        /* kill off what we've allocated so far */
        status = close_database(&p_rec_projector->opendb, status);
        rec_kill_projector(&p_rec_projector);
    }

    return(status);
}

/* end of rcio.c */
