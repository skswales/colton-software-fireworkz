/* import.c */

/* Copyright (C) 1994-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#include "common/gflags.h"

#define EXPOSE_RISCOS_FLEX 1

#include "ob_rec/ob_rec.h"

/*poked*/ NUMFORM_PARMS numform_parms_for_dplib =
{
    NULL,
    USTR_TEXT("0.#########") /*p_numform_numeric*/,
    USTR_TEXT("DD.MM.yyyy HH:MM:SS;DD.MM.yyyy;HH:MM:SS") /*p_numform_datetime*/,
    USTR_TEXT("@") /*p_numform_texterror*/
};

_Check_return_
extern STATUS
rec_import_file(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DATA_REF p_data_ref,
    _In_z_      PCTSTR filename,
    _InVal_     T5_FILETYPE t5_filetype)
{
    STATUS status;
    ARRAY_HANDLE h_data;

    /* load the file */
    if(status_ok(status = file_memory_load(p_docu, &h_data, filename, NULL, 0)))
    {
        /* write it to the record */
        QUICK_UBLOCK quick_ublock;
        quick_ublock_setup_using_array(&quick_ublock, h_data);

        status = rec_object_write_text(p_docu, p_data_ref, &quick_ublock, t5_filetype);

        al_array_dispose(&h_data);
    }

    return(status);
}

_Check_return_
extern STATUS
rec_link_file(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DATA_REF p_data_ref,
    _In_z_      PCTSTR filename,
    _InVal_     T5_FILETYPE t5_filetype)
{
    STATUS status;
    TCHARZ tstr_buf[BUF_MAX_PATHSTRING];

    IGNOREPARM_InVal_(t5_filetype);

    if((status = file_find_on_path(tstr_buf, elemof32(tstr_buf), filename)) > 0 )
    {
        QUICK_UBLOCK quick_ublock;
        quick_ublock_setup_fill_from_ubuf(&quick_ublock, uchars_bptr(tstr_buf) /*T==U*/, elemof32(tstr_buf));

        status = rec_object_write_text(p_docu, p_data_ref, &quick_ublock, FILETYPE_LINKED_FILE);
    }
    else if(0 == status) /* SKS 12may95 */
        status = create_error(FILE_ERR_NOTFOUND);

    return(status);
}

/* Routine to create an empty database of a certain structure */

_Check_return_
static STATUS
manufacture_database(
    _In_z_      PCTSTR filename,
    _In_        S32 n_flds,
    /*_Inout_*/ P_ANY p_data)
{
    STATUS status = STATUS_OK;
    TABLEDEF table;
    BOOL has_titles = TRUE;
    S32 i;

    make_fake_table(&table);

    for(i = 0; i < n_flds; i++)
        status_break(status = add_new_field_to_table(&table, NULL, FIELD_TEXT));

    if(status_ok(status))
    {
        for(i = 0; i < n_flds; i++)
        {
            P_ARRAY_HANDLE p_array_handle = (P_ARRAY_HANDLE) p_data;
            P_EV_DATA p_ev_data = array_ptr(p_array_handle, EV_DATA, i);

            switch(p_ev_data->did_num)
            {
            default:
                has_titles = FALSE;
                break;

            case RPN_DAT_BLANK:
            case RPN_DAT_STRING:
                break;
            }
        }

        for(i = 0; i < n_flds; i++)
        {
            P_ARRAY_HANDLE p_array_handle = (P_ARRAY_HANDLE) p_data;
            P_EV_DATA p_ev_data = array_ptr(p_array_handle, EV_DATA, i);
            P_FIELDDEF p_fielddef;
            U8Z buffer[sizeof32(p_fielddef->name)];

            status = STATUS_OK;

            if(P_DATA_NONE == (p_fielddef = p_fielddef_from_number(&table.h_fielddefs, i+1)))
            {
                status = STATUS_FAIL;
                break;
            }

            consume_int(xsnprintf(buffer, elemof32(buffer), "Field" S32_FMT, i+1));

            if(has_titles)
                switch(p_ev_data->did_num)
                {
                default:
                    break;

                case RPN_DAT_STRING:
                    if(p_ev_data->arg.string.size)
                    {
                        U32 len = p_ev_data->arg.string.size;
                        if(len >= elemof32(buffer)-1)
                            len = elemof32(buffer)-1;
                        memcpy32(buffer, p_ev_data->arg.string.uchars, len);
                        buffer[len] = CH_NULL;
                    }
                    break;
                }

            if(P_DATA_NONE == p_fielddef_from_name(&table.h_fielddefs, buffer))
                /* not a duplicate */
                xstrkpy(p_fielddef->name, elemof32(p_fielddef->name), buffer);
        }

#if DPLIB
        status = dplib_create_from_prototype(filename, &table);
#endif
    }

    status_return(drop_table(&table, status));

    return(has_titles);
}

/******************************************************************************
*
* code to grok about in csv data
*
******************************************************************************/

static void
dispose_csv_record(
    /*inout*/ P_ARRAY_HANDLE p_array_handle)
{
    ARRAY_INDEX i = array_elements(p_array_handle);

    while(--i >= 0)
    {
        P_EV_DATA p_ev_data = array_ptr(p_array_handle, EV_DATA, i);

        ss_data_free_resources(p_ev_data);
    }

    al_array_dispose(p_array_handle);
}

#if DPLIB

_Check_return_
static STATUS
dplib_remap_import(
    _DocuRef_   P_DOCU p_docu,
    P_OPENDB p_opendb,
    P_CSV_READ_RECORD p_csv_read_record) /* fast import */
{
    STATUS status = STATUS_OK;
    ARRAY_HANDLE h_field_text;
    cursor crsr;
    fieldsptr p_fields = (fieldsptr) p_opendb->table.h_fields;
    _kernel_oserror * e = cursor_open(p_fields, NULL, lock_Update, &crsr);

    if(NULL != e)
        return(rec_oserror_set(e));

    {
    SC_ARRAY_INIT_BLOCK aib = aib_init(1, sizeof32(char *), TRUE);
    if(NULL == al_array_alloc(&h_field_text, char *, array_elements(&p_opendb->table.h_fielddefs), &aib, &status))
         return(status);
    } /*block*/

    numform_parms_for_dplib.p_numform_context = get_p_numform_context(P_DOCU_NONE);

    for(;;) /* keep going till no more records */
    {
        ARRAY_INDEX j;
        int new_surrogate_key;

        e = fields_newsurrogate(p_fields, &new_surrogate_key);
        if(NULL != e)
            break;

        fields_blank(p_fields);

        /* Set the fields from the input record */
        for(j = 0; j < array_elements(&p_opendb->table.h_fielddefs); ++j)
        {
            P_FIELDDEF p_fielddef = array_ptr(&p_opendb->table.h_fielddefs, FIELDDEF, j);
            char ** anchor = array_ptr(&h_field_text, char *, j);
            PC_EV_DATA p_ev_data;
            PC_U8 p_u8 = NULL;
            U32 size = 0;
            QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 40);
            quick_ublock_with_buffer_setup(quick_ublock);

            if(j >= array_elements(&p_csv_read_record->array_handle))
                continue;

            status = STATUS_OK;

            p_ev_data = array_ptrc(&p_csv_read_record->array_handle, EV_DATA, j);

            switch(p_ev_data->did_num)
            {
            default:
                break;

            case RPN_DAT_REAL:
            case RPN_DAT_BOOL8:
            case RPN_DAT_WORD8:
            case RPN_DAT_WORD16:
            case RPN_DAT_WORD32:
            case RPN_DAT_DATE:
                (void) numform(&quick_ublock, P_QUICK_TBLOCK_NONE, p_ev_data, &numform_parms_for_dplib);
                quick_ublock_nullch_strip(&quick_ublock);
                p_u8 = quick_ublock_uchars(&quick_ublock);
                size = quick_ublock_bytes(&quick_ublock);
                break;

            case RPN_DAT_STRING:
                p_u8 = p_ev_data->arg.string.uchars;
                size = p_ev_data->arg.string.size;
                break;
            }

            if(size)
            {
                if(flex_alloc((flex_ptr) anchor, size + 1))
                {
                    char * p_u8_dst = *anchor;
                    p_u8_dst[size] = CH_NULL;
                    memcpy32(p_u8_dst, p_u8, size);
                }
                else
                    size = 0;
            }

            e = field_setvalue_text(p_fielddef->p_field, filetype_Text, anchor, 0, (int) size);

            quick_ublock_dispose(&quick_ublock);

            if(NULL != e)
                break;
        }

        if(NULL == e)
        {
            fields_setsurrogate(p_fields, new_surrogate_key);

            e = cursor_insert(crsr, (recflags) 0);
        }

        for(j = 0; j < array_elements(&h_field_text); ++j)
        {
            char ** anchor = array_ptr(&h_field_text, char *, j);

            flex_dispose((flex_ptr) anchor);
        }

        dispose_csv_record(&p_csv_read_record->array_handle);

        if(NULL != e)
            break;

        if(!status_done(status = object_call_id(OBJECT_ID_FL_CSV, p_docu, T5_MSG_CSV_READ_RECORD, p_csv_read_record)))
            break;
    }

    al_array_dispose(&h_field_text);

    e = cursor_close(crsr, cursor_flush(crsr, e));

    return(e ? rec_oserror_set(e) : status);
}

#endif

#if 0 /* only needed for slow import */

/* add new record at end of database */

_Check_return_
static STATUS
add_new_record(
    P_OPENDB p_opendb,
    /*_Inout_*/ P_ANY p_data)
{
    STATUS status;
    ARRAY_INDEX j;

    status_assert(status = record_view_recnum(p_opendb, 0)); /* don't quite know why he does this ... */

    if(status_ok(status = record_add_commence(p_opendb, TRUE)))
        status = record_edit_confirm(p_opendb, TRUE, TRUE);
    else
        record_edit_cancel(p_opendb, TRUE);

    status_return(status);

    status_assert(status = record_view_recnum(p_opendb, p_opendb->recordspec.ncards));

    status_assert(status = record_edit_commence(p_opendb));

    status_assert(status = new_recordspec_handle_table(&p_opendb->recordspec, array_elements(&p_opendb->table.h_fielddefs)));

    numform_parms_for_dplib.p_numform_context = get_p_numform_context(P_DOCU_NONE);

    for(j = 0; (j < array_elements(&p_opendb->table.h_fielddefs)) && status_ok(status); j++)
    {
        P_EDITREC p_editrec = array_ptr(&p_opendb->recordspec.h_editrecs, EDITREC, j);
        P_ARRAY_HANDLE p_array_handle = (P_ARRAY_HANDLE) p_data;
        P_EV_DATA p_ev_data;
        PC_U8 p_u8 = NULL;
        S32 size = 0;
        QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 40);
        quick_ublock_with_buffer_setup(quick_ublock);

        if(j >= array_elements(p_array_handle))
            continue;

        p_editrec->p_u8 = NULL;
        p_editrec->length = 0;
        p_editrec->t5_filetype = -1;

        p_ev_data = array_ptr(p_array_handle, EV_DATA, j);

        switch(p_ev_data->did_num)
        {
        default:
            break;

        case RPN_DAT_REAL:
        case RPN_DAT_BOOL8:
        case RPN_DAT_WORD8:
        case RPN_DAT_WORD16:
        case RPN_DAT_WORD32:
        case RPN_DAT_DATE:
            (void) numform(&quick_ublock, P_QUICK_TBLOCK_NONE, p_ev_data, &numform_parms_for_dplib);
            quick_ublock_nullch_strip(&quick_ublock);
            p_u8 = quick_ublock_uchars(&quick_ublock);
            size = quick_ublock_bytes(&quick_ublock);
            break;

        case RPN_DAT_STRING:
            p_u8 = p_ev_data->arg.stringc.data;
            size = p_ev_data->arg.stringc.size;
            break;
        }

        if(size)
        {
            U32 len = size + 1 /*CH_NULL*/;
            if(NULL != (p_editrec->p_u8 = al_ptr_alloc_bytes(P_U8Z, len, &status)))
            {
                memcpy32(p_editrec->p_u8, p_u8, size);
                p_editrec->p_u8[size] = CH_NULL;
                p_editrec->length = len;
                p_editrec->t5_filetype = FILETYPE_TEXT;
            }
        }

        quick_ublock_dispose(&quick_ublock);
    }

    if(status_ok(status))
        /* p_opendb->recordspec.h_editrecs is now setup. Lock onto the target record and write out the data */
        status = record_edit_confirm (p_opendb, FALSE, FALSE);

    if(status_fail(status))
        status_assert(record_edit_cancel(p_opendb, FALSE));

    (void) free_recordspec_handle_table(&p_opendb->recordspec);

    return(status);
}

#endif

static inline void
rec_csv_quick_ublock_setup(
    _OutRef_    P_QUICK_UBLOCK p_quick_ublock /*set up*/,
    _InoutRef_  P_ARRAY_HANDLE p_array_handle)
{
    quick_ublock_setup_without_clearing_ubuf(p_quick_ublock, array_base(p_array_handle, _UCHARS), array_size32(p_array_handle));
}

#if 0

_Check_return_
extern STATUS
rec_import_csv(
    _DocuRef_   P_DOCU p_docu,
    P_RECORDZ_IMPORT p_recordz_import) /* not needed for a while */
{
    STATUS status;
    FF_IP_FORMAT ff_ip_format = FF_IP_FORMAT_INIT;
    ARRAY_HANDLE temp_contents_array_handle = 0;
    ARRAY_HANDLE temp_decode_array_handle = 0;
    CSV_READ_RECORD csv_read_record;
    BOOL created = FALSE;
    OPENDB opendb;

    zero_struct(opendb);

    /* We need to open the file for input, and derive an input stream */
    status_return(status = foreign_load_initialise(p_docu, &p_docu->cur, &ff_ip_format, p_recordz_import->input_filename, NULL));

    csv_read_record.p_ff_ip_format = &ff_ip_format;
    csv_read_record.array_handle = 0;
    csv_read_record.temp_contents_array_handle = 0;
    csv_read_record.temp_decode_array_handle = 0;

    { /* SKS 11oct95 make quick_block for temp contents and temp decode grow to size of max line used for quick reallocs */
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1024, sizeof32(U8), FALSE);
    if(status_ok(status = al_array_preallocate_zero(&csv_read_record.temp_contents_array_handle, &array_init_block)))
        status = al_array_preallocate_zero(&csv_read_record.temp_decode_array_handle, &array_init_block);
    } /*block*/

    rec_csv_quick_ublock_setup(&csv_read_record.temp_contents_quick_ublock, &csv_read_record.temp_contents_array_handle);
    rec_csv_quick_ublock_setup(&csv_read_record.temp_decode_quick_ublock, &csv_read_record.temp_decode_array_handle);

    for(;;)
    {
        BOOL do_create_record = TRUE;

        /* Read me a record */
        if(!status_done(status = object_call_id(OBJECT_ID_FL_CSV, p_docu, T5_MSG_CSV_READ_RECORD, &csv_read_record)))
            break;

        if(!created)
        {
            status_break(status = manufacture_database(p_recordz_import->output_filename, array_elements(&csv_read_record.array_handle), &csv_read_record.array_handle));

            do_create_record = TRUE; /*(status == 0); SKS 06jan95 punter can always delete 1st record*/

            status_break(status = rec_open_database(&opendb, p_recordz_import->output_filename));

            created = TRUE;
        }

        if(do_create_record)
            status = add_new_record(&opendb, &csv_read_record.array_handle);

        dispose_csv_record(&csv_read_record.array_handle);

        status_break(status);
    }

    dispose_csv_record(&csv_read_record.array_handle); /* needed if error in loop */

    al_array_dispose(&temp_decode_array_handle);
    al_array_dispose(&temp_contents_array_handle);

    status_accumulate(status, foreign_load_finalise(p_docu, &ff_ip_format));

    status_return(close_database(&opendb, status));

    { /* now send the file to ourselves now that it's converted */
    MSG_INSERT_FOREIGN msg_insert_foreign = p_recordz_import->msg_insert_foreign;
    msg_insert_foreign.filename = p_recordz_import->output_filename;
    msg_insert_foreign.t5_filetype = FILETYPE_DATAPOWER;
    msg_insert_foreign.scrap_file = FALSE; /* SKS 22sep95 even if CSV was scrap, this isn't!!! */
    return(rec_msg_insert_foreign(p_docu, &msg_insert_foreign));
    } /*block*/
}

#else

_Check_return_
extern STATUS
rec_import_csv(
    _DocuRef_   P_DOCU p_docu,
    P_RECORDZ_IMPORT p_recordz_import) /* new, fast method */
{
    STATUS status;
    FF_IP_FORMAT ff_ip_format = FF_IP_FORMAT_INIT;
    ARRAY_HANDLE temp_contents_array_handle = 0;
    ARRAY_HANDLE temp_decode_array_handle = 0;
    CSV_READ_RECORD csv_read_record;
    OPENDB opendb;

    zero_struct(opendb);

    /* We need to open the file for input, and derive an input stream */
    status_return(status = foreign_load_initialise(p_docu, &p_docu->cur, &ff_ip_format, p_recordz_import->input_filename, NULL));

    csv_read_record.p_ff_ip_format = &ff_ip_format;
    csv_read_record.array_handle = 0;
    csv_read_record.temp_contents_array_handle = 0;
    csv_read_record.temp_decode_array_handle = 0;

    { /* SKS 11oct95 make quick_block for temp contents and temp decode grow to size of max line used for quick reallocs */
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1024, sizeof32(U8), FALSE);
    if(status_ok(status = al_array_preallocate_zero(&csv_read_record.temp_contents_array_handle, &array_init_block)))
        status = al_array_preallocate_zero(&csv_read_record.temp_decode_array_handle, &array_init_block);
    } /*block*/

    rec_csv_quick_ublock_setup(&csv_read_record.temp_contents_quick_ublock, &csv_read_record.temp_contents_array_handle);
    rec_csv_quick_ublock_setup(&csv_read_record.temp_decode_quick_ublock, &csv_read_record.temp_decode_array_handle);

    if(status_ok(status))
    {
        /* Read me a record */
        if(status_done(status = object_call_id(OBJECT_ID_FL_CSV, p_docu, T5_MSG_CSV_READ_RECORD, &csv_read_record)))
            if(status_ok(status = manufacture_database(p_recordz_import->output_filename, array_elements(&csv_read_record.array_handle), &csv_read_record.array_handle)))
                status = rec_open_database(&opendb, p_recordz_import->output_filename);
    }

#if DPLIB
    if(status_ok(status))
        status = dplib_remap_import(p_docu, &opendb, &csv_read_record);
#endif

    dispose_csv_record(&csv_read_record.array_handle); /* needed if error in loop */

    al_array_dispose(&temp_decode_array_handle);
    al_array_dispose(&temp_contents_array_handle);

    status_accumulate(status, foreign_load_finalise(p_docu, &ff_ip_format));

    status_return(close_database(&opendb, status));

    p_recordz_import->msg_insert_foreign.filename = p_recordz_import->output_filename;

    return(rec_msg_insert_foreign(p_docu, &p_recordz_import->msg_insert_foreign));
}

#endif

/* end of import.c */
