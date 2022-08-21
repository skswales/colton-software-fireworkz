/* dbread.c */

/* Copyright (C) 1994-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#include "common/gflags.h"

#include "ob_rec/ob_rec.h"

_Check_return_
extern STATUS
set_table(
    P_OPENDB p_opendb,
    P_U8 basename);

/* HIGH LEVEL DATABASE FUNCTIONS */

/* Compare two tables to see if the second is a strict subset of the first */

_Check_return_
extern BOOL
is_table_contained_by_table(
    _InRef_     PC_TABLEDEF p_table1,
    _InRef_     PC_TABLEDEF p_table2)
{
    ARRAY_INDEX i;

    if(array_elements(&p_table1->h_fielddefs) < array_elements(&p_table2->h_fielddefs))
        return(FALSE);

    for(i = 0; i < array_elements(&p_table2->h_fielddefs); i++)
    {
        PC_FIELDDEF p_fielddef_1 = array_ptrc(&p_table1->h_fielddefs, FIELDDEF, i);
        PC_FIELDDEF p_fielddef_2 = array_ptrc(&p_table2->h_fielddefs, FIELDDEF, i);

        if(p_fielddef_1->type != p_fielddef_2->type)
            return(FALSE);

        if(0 != tstricmp(p_fielddef_1->name, p_fielddef_2->name))
            return(FALSE);

        /* What about the field id --- surely not! */
    }

    return(TRUE);
}

/* Compare two tables to see if they are the same */

_Check_return_
extern BOOL
is_table_same_as_table(
    _InRef_     PC_TABLEDEF p_table1,
    _InRef_     PC_TABLEDEF p_table2)
{
    if(array_elements(&p_table1->h_fielddefs) != array_elements(&p_table2->h_fielddefs))
        return(FALSE);

    return(is_table_contained_by_table(p_table1, p_table2));
}

_Check_return_
extern STATUS
switch_database(
    P_OPENDB p_opendb,
    _In_z_      PCTSTR tempfile)
{
    STATUS status = STATUS_OK;
    DB_ID old_db_id = p_opendb->db.id; /* preserve because of uref */
    TCHARZ scrapfile[BUF_MAX_PATHSTRING];
    ARRAY_HANDLE h_old_querys;

    /* It would be nice to retain the querys */
    close_cursor(&p_opendb->table) ; /* Closeing the cursor first seems to help */
    h_old_querys = p_opendb->search.h_query;
    {
    ARRAY_INDEX i = array_elements(&p_opendb->search.h_query);

    while(--i >= 0)
    {
        P_QUERY p_query = array_ptr(&p_opendb->search.h_query, QUERY, i);
#if DPLIB
        status_assert(dplib_remove_query(p_query, TRUE));
#else
        IGNOREPARM(p_query);
#endif
    }
    } /*block*/
    p_opendb->search.h_query = 0;

    status_return(close_database(p_opendb, status));

    /* do the renames:
    rename p_opendb->db.name AS temporary name in same dir
    rename tempname AS p_opendb->db.name - this may not be on the same disc so rename may fail!
    */

    {
    QUICK_TBLOCK_WITH_BUFFER(aqtb_tempname, 80);
    quick_tblock_with_buffer_setup(aqtb_tempname);

    file_dirname(scrapfile, p_opendb->db.name);
    if(status_ok(status = file_tempname(scrapfile, TEXT("db"), NULL, 0, &aqtb_tempname)))
    {
        tstr_xstrkpy(scrapfile, elemof32(scrapfile), quick_tblock_tstr(&aqtb_tempname));
        status = file_rename(p_opendb->db.name, scrapfile);
    }

    quick_tblock_dispose(&aqtb_tempname);
    } /*block*/

    if(status_ok(status))
        /* The first rename has been sucessful */
        if(status_fail(status = file_rename(tempfile, p_opendb->db.name)))
            /* The second rename has failed */
            (void) file_rename(scrapfile, p_opendb->db.name); /* put the first one back */

    /* open the database */
    status = open_database(p_opendb, p_opendb->db.name);

    p_opendb->db.id = old_db_id; /* Must preserve the id because were still attached via a uref */

    if(status_ok(status))
        (void) file_remove(scrapfile);

    status_return(ensure_cursor_whole(&p_opendb->table));

    status = record_view_recnum(p_opendb, 0);

    p_opendb->search.h_query = h_old_querys; /* Weeble */

    return(status);
}

/* Scan the world looking for instances of this filename */

_Check_return_
static STATUS
rec_open_already_anywhere(
    _In_z_      PCTSTR filename)
{
    DOCNO docno = DOCNO_NONE;

    while(DOCNO_NONE != (docno = docno_enum_docs(docno)))
    {
        const P_DOCU p_docu = p_docu_from_docno(docno);
        STATUS status = rec_check_for_file_attachment(p_docu, filename);

        if(status_fail(status))
        {
            PCTSTR format = resource_lookup_tstr(REC_ERR_ALREADY_OPEN_IN);
            TCHARZ error_buffer[BUF_MAX_PATHSTRING];
            consume_int(tstr_xsnprintf(error_buffer, elemof32(error_buffer), format, filename));
            return(file_error_set(error_buffer));
        }
    }

    return(STATUS_OK);
}

/*  This is a combination of
    dplib_read_database() to open the file & extract the tabledefs
    and
    open_table() to find the tabledef for the given table and to get its fields data set up
*/

_Check_return_
extern STATUS
open_database(
    P_OPENDB p_opendb,
    _In_z_      PCTSTR filename)
{
    TCHARZ buffer[BUF_MAX_PATHSTRING];
    STATUS status;

    p_opendb->dbok = FALSE;
    p_opendb->tableok = FALSE;

    drop_search_suggestions(&p_opendb->search);

    {
    ARRAY_INDEX i = array_elements(&p_opendb->search.h_query);
    while(--i >= 0)
        wipe_query(array_ptr(&p_opendb->search.h_query, QUERY, i));
    } /*block*/

    p_opendb->recordspec.recno      = -1;
    p_opendb->recordspec.ncards     = -1;
    p_opendb->recordspec.lastvalid  = -1;
    p_opendb->recordspec.h_editrecs =  0;

    p_opendb->db.id = (DB_ID) -1;
    p_opendb->db.h_tabledefs = NULL;
    p_opendb->db.n_tables = 0;
    p_opendb->db.private_dp_handle = NULL;

#if DPLIB
    status = rec_oserror_set(os_canonicalise(de_const_cast(PTSTR, filename), buffer, elemof32(buffer)));
#else
    tstr_xstrkpy(buffer, elemof32(buffer), filename);
    status = STATUS_OK;
#endif

    if(status_ok(status))
    {
        filename = buffer;

        tstr_xstrkpy(p_opendb->db.name, elemof32(p_opendb->db.name), filename);

        status = rec_open_already_anywhere(filename);
    }

#if DPLIB
    if(status_ok(status))
        status = dplib_read_database(&p_opendb->db, filename);
#endif

    if(status_ok(status))
    {
        p_opendb->dbok = TRUE;

        p_opendb->db.id = 0xFFFFFFFFU;

        status = open_table(&p_opendb->db, &p_opendb->table, file_leafname(filename));

        if(status_ok(status))
            p_opendb->tableok =TRUE;
    }

    return(status_fail(status) ? close_database(p_opendb, status) : status);
}

/* This strips hidden fields - if you want the whole thing use clone_database_structure */

_Check_return_
extern STATUS
export_current_subset(
    P_OPENDB p_opendb,
    _In_z_      PCTSTR filename2)
{
    if(!p_opendb->dbok || !p_opendb->tableok)
        return(create_error(REC_ERR_DB_NOT_OK));

    /* Create a new datapower file based on the structure of the existing file */
#if DPLIB
    return(dplib_export(filename2, p_opendb));
#else
    return(create_error(ERR_NYI));
#endif
}

_Check_return_
extern STATUS
clone_database_structure(
    P_OPENDB p_opendb,
    _In_z_      PCTSTR filename2)
{
    if(!p_opendb->dbok || !p_opendb->tableok)
        return(create_error(REC_ERR_DB_NOT_OK));

    /* Create a new datapower file based on the structure of the existing file */
#if DPLIB
    return(dplib_clone_structure(filename2, &p_opendb->table));
#else
    return(create_error(ERR_NYI));
#endif
}

_Check_return_
extern STATUS
remap_database_structure(
    P_OPENDB p_opendb,
    _In_z_      PCTSTR filename,
    P_ARRAY_HANDLE p_handle)
{
    if(!p_opendb->dbok || !p_opendb->tableok)
        return(create_error(REC_ERR_DB_NOT_OK));

    /* Create a new datapower file based on the structure of the existing file */
#if DPLIB
    return(dplib_remap_file(filename, p_handle, p_opendb->table.private_dp_layout));
#else
    return(create_error(ERR_NYI));
#endif
}

/* Remap the fields from one extant database to another */

_Check_return_
extern STATUS
remap_records(
    P_OPENDB p_opendb,
    _In_z_      PCTSTR filename,
    P_ARRAY_HANDLE p_handle,
    P_REMAP_OPTIONS p_options)
{
    STATUS status;
    OPENDB opendb_newfile;

    zero_struct(opendb_newfile);
    status_return(open_database(&opendb_newfile, filename));

    status = ensure_cursor_whole(&opendb_newfile.table);

    if(status_ok(status))
        status = ensure_cursor_whole(&p_opendb->table);

    if(status_ok(status))
        status = record_view_recnum(p_opendb, 0);

#if DPLIB
    if(status_ok(status))
        status = dplib_remap_records(p_opendb, &opendb_newfile, p_handle, p_options);
#endif

    return(close_database(&opendb_newfile, status));
}

/*
This terminates use of a database and flushes its resources
*/

_Check_return_
extern STATUS
close_database(
    P_OPENDB p_opendb,
    _InVal_     STATUS status_in)
{
    STATUS status = STATUS_OK;

    if(p_opendb->dbok)
    {
        p_opendb->search.query_id = RECORDZ_WHOLE_FILE;

        close_cursor(&p_opendb->table);

        drop_search_suggestions(&p_opendb->search);

        { /* Drop all active queries - before the table or else... */
        ARRAY_INDEX i = array_elements(&p_opendb->search.h_query);

        while(--i >= 0)
            close_query(p_opendb, array_ptr(&p_opendb->search.h_query, QUERY, i));

        al_array_dispose(&p_opendb->search.h_query);
        } /*block*/

        /* Drop any open table - this will drop the fields for you */
        status = drop_table(&p_opendb->table, status);

        /* Drop the table defs collection for the database */
        al_array_dispose(&p_opendb->db.h_tabledefs);

        p_opendb->tableok = FALSE;

        p_opendb->dbok = FALSE;
    }

    return(status_fail(status_in) ? status_in : status);
}

/* HIGH LEVEL TABLE FUNCTIONS */

_Check_return_
extern STATUS
open_table(
    P_DB p_db,
    P_TABLEDEF p_table,
    P_U8 p_table_name)
{
    STATUS status = STATUS_FAIL;
    ARRAY_INDEX i;

    for(i = 0; i < array_elements(&p_db->h_tabledefs); ++i)
    {
        P_TABLEDEF p_tabledef = array_ptr(&p_db->h_tabledefs, TABLEDEF, i);

        if(0 == tstricmp(p_table_name, p_tabledef->name))
        {
            *p_table = *p_tabledef;

            p_table->h_fields = NULL;
            p_table->h_fielddefs = NULL;

            p_table->private_dp_layout = NULL;
            p_table->private_dp_cursor = NULL;

#if DPLIB
            status = dplib_read_fields_for_table(p_db, p_table);
#else
            status = STATUS_OK;
#endif

            break;
        }
    }

    return(status);
}

/*
Drops the table from a database, frees the fields collection associated with it, kills the file
*/

_Check_return_
extern STATUS
drop_table(
    P_TABLEDEF p_table,
    _InVal_     STATUS status_in)
{
    STATUS status;

#if DPLIB
    status = dplib_drop_fields_for_table(p_table);
#else
    status = STATUS_OK;
#endif

    drop_fake_table(p_table);

    return(status_fail(status_in) ? status_in : status);
}

extern void
drop_fake_table(
    P_TABLEDEF p_table)
{
    drop_fields_for_table(p_table);

    p_table->id = 0xFFFFFFFFU;
    p_table->name[0] = CH_NULL;
}

_Check_return_
extern STATUS
drop_fields_for_table(
    P_TABLEDEF p_table)
{
    STATUS status = STATUS_OK;

#if DPLIB
    /* This fixes a handle leak */
    dplib_fields_delete_for_table(p_table);
#endif

    al_array_dispose(&p_table->h_fielddefs);

    return(status);
}

_Check_return_
extern STATUS
make_fake_table(
    P_TABLEDEF p_fake_table)
{
    p_fake_table->id = INVENT_TABLE_ID;
    tstr_xstrkpy(p_fake_table->name, elemof32(p_fake_table->name), "Fake_Table");

    p_fake_table->h_fielddefs = 0;

    p_fake_table->h_fields = NULL;
    p_fake_table->private_dp_layout = NULL;
    p_fake_table->private_dp_cursor = NULL;

    return(STATUS_OK);
}

_Check_return_
extern STATUS
table_check_access(
    P_TABLEDEF p_table,
    _InVal_     ACCESS_FUNCTION access_function)
{
#if DPLIB
    return(dplib_check_access_function(p_table, access_function));
#else
    return(STATUS_OK);
#endif
}

/* Copy all the fields collection from one table to another */

_Check_return_
extern STATUS
table_copy_fields(
    P_TABLEDEF p_table_dst,
    P_TABLEDEF p_table_src)
{
    al_array_dispose(&p_table_dst->h_fielddefs);

    if(0 == array_elements(&p_table_src->h_fielddefs))
        return(STATUS_OK);

    status_return(al_array_duplicate(&p_table_dst->h_fielddefs, &p_table_src->h_fielddefs));

#if DPLIB
    return(dplib_copy_fields(p_table_dst, p_table_src));
#else
    return(STATUS_FAIL);
#endif
}

_Check_return_
extern STATUS
table_add_field(
    P_TABLEDEF p_table,
    P_FIELDDEF p_fielddef)
{
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(*p_fielddef), TRUE);

#if DPLIB
    if(NULL == p_fielddef->p_field)
        status_return(dplib_create_field(p_table, p_fielddef));
#endif

    status_return(al_array_add(&p_table->h_fielddefs, FIELDDEF, 1, &array_init_block, p_fielddef));

    return(STATUS_OK);
}

/* add a new field to end of table */

_Check_return_
extern STATUS
add_new_field_to_table(
    P_TABLEDEF p_table,
    _In_z_      PC_U8Z p_u8_name,
    _In_        FIELD_TYPE ft)
{
    STATUS status;
    P_FIELDDEF p_fielddef;
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(*p_fielddef), TRUE);
    U8Z name[32];

    if(NULL == (p_fielddef = al_array_extend_by(&p_table->h_fielddefs, FIELDDEF, 1, &array_init_block, &status)))
        return(status);

    wipe_fielddef(p_fielddef);

    p_fielddef->type = ft;
    p_fielddef->parentid = p_table->id;

    if(NULL == p_u8_name)
    {
        consume_int(xsnprintf(name, elemof32(name), "Field_" S32_FMT, array_elements(&p_table->h_fielddefs)));
        p_u8_name = name;
    }

    tstr_xstrkpy(p_fielddef->name, elemof32(p_fielddef->name), p_u8_name);

    return(status);
}

_Check_return_
extern STATUS
remove_field_from_table(
    P_TABLEDEF p_table,
    _In_        ARRAY_INDEX delete_at)
{
    al_array_delete_at(&p_table->h_fielddefs, -1, delete_at);
    return(STATUS_OK);
}

extern S32
fieldnumber_from_field_id(
    P_TABLEDEF p_table,
    _In_        FIELD_ID f_id)
{
    ARRAY_INDEX i;

    for(i = 0; i < array_elements(&p_table->h_fielddefs); ++i)
    {
        PC_FIELDDEF p_fielddef = array_ptrc(&p_table->h_fielddefs, FIELDDEF, i);

        if(f_id == p_fielddef->id)
            return(i + 1);
    }

    return(-1); /* see fieldnumber_from_rec_data_ref */
}

extern void
drop_search_suggestions(
    P_SEARCH p_search)
{
    p_search->query_id = RECORDZ_WHOLE_FILE;

    p_search->suggest_type = SEARCH_TYPE_FILE;
    p_search->suggest_andor = (2);
    p_search->suggest_exclude = FALSE;

    {
    ARRAY_INDEX i = array_elements(&p_search->h_suggest_pattern);

    while(--i >= 0)
    {
        P_SEARCH_FIELD_PATTERN p_search_pattern = array_ptr(&p_search->h_suggest_pattern, SEARCH_FIELD_PATTERN, i);
        al_array_dispose(&p_search_pattern->h_text_ustr);
    }

    al_array_dispose(&p_search->h_suggest_pattern);
    } /*block*/
}

/* Free any existing stuff
   make a new table with the correct number of slots in it
   CLEAR the slots to null
*/

_Check_return_
extern STATUS
new_recordspec_handle_table(
    P_RECORDSPEC p_recordspec,
    _In_        S32 n_required)
{
    STATUS status = STATUS_OK;
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(EDITREC), TRUE);
    ARRAY_INDEX n_current = array_elements(&p_recordspec->h_editrecs);

    if(n_current)
     {
         ARRAY_INDEX i = n_current;

         while(--i >= 0)
         {
             P_EDITREC p_editrec = array_ptr(&p_recordspec->h_editrecs, EDITREC, i);

             al_ptr_dispose(P_P_ANY_PEDANTIC(&p_editrec->p_u8));
         }

         if(n_current != n_required)
            /* Highly probable that they were the same */
            consume_ptr(al_array_extend_by(&p_recordspec->h_editrecs, EDITREC, n_required, &array_init_block, &status));
    }
    else
    {
        /* No table yet so just allocate */
        consume_ptr(al_array_alloc(&p_recordspec->h_editrecs, EDITREC, n_required, &array_init_block, &status));
    }

    return(status);
}

_Check_return_
extern STATUS
free_recordspec_handle_table(
    P_RECORDSPEC p_recordspec)
{
    ARRAY_INDEX i = array_elements(&p_recordspec->h_editrecs);

    while(--i >= 0)
    {
        P_EDITREC p_editrec = array_ptr(&p_recordspec->h_editrecs, EDITREC, i);

        al_ptr_dispose(P_P_ANY_PEDANTIC(&p_editrec->p_u8));
    }

    al_array_dispose(&p_recordspec->h_editrecs);

    return(STATUS_OK);
}

_Check_return_
extern STATUS
field_get_value(
    P_REC_RESOURCE p_rec_resource,
    _InRef_     PC_FIELDDEF p_fielddef,
    _InVal_     BOOL plain)
{
    if(!p_fielddef->readable)
        return(create_error(REC_ERR_FIELD_NOT_READABLE));

#if DPLIB
    return(dplib_field_get_value(p_rec_resource, p_fielddef, plain));
#else
    return(ERR_NYI);
#endif
}

_Check_return_
extern STATUS
record_view(
    P_OPENDB p_opendb)
{
#if DPLIB
    status_return(dplib_locate_record(&p_opendb->table, &p_opendb->recordspec.recno, &p_opendb->recordspec.ncards, &p_opendb->recordspec.lastvalid));

    if((U32) p_opendb->recordspec.recno > (U32) p_opendb->recordspec.ncards)
        return(STATUS_FAIL);

    return(STATUS_OK);
#else
    return(create_error(ERR_NYI));
#endif
}

_Check_return_
extern STATUS
record_view_recnum(
    P_OPENDB p_opendb,
    _In_        S32 recnum)
{
    p_opendb->recordspec.recno = recnum;
    p_opendb->recordspec.ncards = -1;
    p_opendb->recordspec.lastvalid = -1;

#if DPLIB
    status_return(dplib_locate_record(&p_opendb->table, &p_opendb->recordspec.recno, &p_opendb->recordspec.ncards, &p_opendb->recordspec.lastvalid));

    if(0 != p_opendb->recordspec.ncards)
        if((U32) p_opendb->recordspec.recno > (U32) p_opendb->recordspec.ncards)
            return(STATUS_FAIL); /* record not located */

    return(STATUS_OK);
#else
    return(create_error(ERR_NYI));
#endif
}

_Check_return_
extern STATUS
record_current(
    P_OPENDB p_opendb,
    P_S32 p_s32_record_out,
    P_S32 p_s32_ncards_out)
{
    S32 r = p_opendb->recordspec.recno;
    S32 n = p_opendb->recordspec.ncards;
    STATUS status;

#if DPLIB
    if(status_fail(status = dplib_current_record(&p_opendb->table, &r, &n)))
    {
        /* Just lie */
        r = p_opendb->recordspec.recno;
        n = p_opendb->recordspec.ncards;
    }
#else
    status = STATUS_OK;
#endif

    *p_s32_record_out = r;
    *p_s32_ncards_out = n;

    return(status);
}

_Check_return_
static STATUS
open_cursor(
    P_TABLEDEF p_table,
    P_ANY p_q17)
{
#if DPLIB
    return(dplib_create_cursor(p_table, p_q17));
#else
    return(create_error(ERR_NYI));
#endif
}

/* Ensure we have a cursor, either for whole database or current subset */

_Check_return_
extern STATUS
ensure_cursor_current(
    P_OPENDB p_opendb,
    _In_        S32 recnum)
{
    P_QUERY p_query = NULL /* no query*/;

    if(RECORDZ_WHOLE_FILE != p_opendb->search.query_id)
        p_query = p_query_from_p_opendb(p_opendb, p_opendb->search.query_id);

    status_return(open_cursor(&p_opendb->table, p_query ? p_query->p_query : NULL));

    return(record_view_recnum(p_opendb, recnum));
}

_Check_return_
extern STATUS
ensure_cursor_whole(
    P_TABLEDEF p_tabledef)
{
    return(open_cursor(p_tabledef, NULL));
}

_Check_return_
extern STATUS
close_cursor(
    P_TABLEDEF p_tabledef)
{
#if DPLIB
    return(rec_oserror_set(dplib_remove_cursor(p_tabledef)));
#else
    return(create_error(ERR_NYI));
#endif
}

/* Routines for editing/adding records

   Its all so orthogonal that this layer is trivial  !!!
*/

_Check_return_
extern STATUS
record_add_commence(
    P_OPENDB p_opendb,
    _InVal_     BOOL clear)
{
#if DPLIB
    return(dplib_record_add_commence(p_opendb, clear));
#else
    return(create_error(ERR_NYI));
#endif
}

_Check_return_
extern STATUS
record_edit_commence(
    P_OPENDB p_opendb)
{
#if DPLIB
    return(dplib_record_edit_commence(p_opendb));
#else
    return(create_error(ERR_NYI));
#endif
}

_Check_return_
extern STATUS
record_edit_confirm(
    P_OPENDB p_opendb,
    _InVal_     BOOL blank,
    _InVal_     BOOL adding)
{
#if DPLIB
    return(dplib_record_edit_confirm(p_opendb, blank, adding));
#else
    return(create_error(ERR_NYI));
#endif
}

_Check_return_
extern STATUS
record_edit_cancel(
    P_OPENDB p_opendb,
    _InVal_     BOOL adding)
{
#if DPLIB
    return(dplib_record_edit_cancel(p_opendb, adding));
#else
    return(create_error(ERR_NYI));
#endif
}

extern void
wipe_fielddef(
    P_FIELDDEF p_fielddef)
{
    zero_array(p_fielddef->name);

    p_fielddef->id = 0xFFFFFFFFU;
    p_fielddef->parentid = 0xFFFFFFFFU;
    p_fielddef->length = -1;
    p_fielddef->type = 0xFFFFFFFFU;
    p_fielddef->compulsory = FALSE;
    p_fielddef->hidden     = FALSE;
    p_fielddef->readable   = TRUE;
    p_fielddef->writeable  = TRUE;
    p_fielddef->keyorder   = 0;

    zero_array(p_fielddef->default_formula);
    zero_array(p_fielddef->check_formula);
    zero_array(p_fielddef->value_list);

#if DPLIB
    p_fielddef->p_field = NULL;
#endif
}

/* end of dbread.c */
