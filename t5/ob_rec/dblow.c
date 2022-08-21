/* dblow.c */

/* Copyright (C) 1994-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* THE LOW LEVEL FUNCTIONS FOR DATABASE ACCESS

   ALL VERY ENGINE-DEPENDANT

   DO NOT CALL THESE

   ONLY CALL FUNCTIONS FROM dbread.c
*/

#include "common/gflags.h"

#include "ob_rec/ob_rec.h"

#include "windflex.h" /* from windlib */

/*
imported variables
*/

extern WimpWindowWithBitset * Template_Password3;

extern void * EncryptBuffer;

/*
exported variables
*/

static struct { WimpWindowWithBitset wind; WimpIconBlockWithBitset icons[4]; } dummy_password_window;

WimpWindowWithBitset * Template_Password3 = &dummy_password_window.wind;

picturestr DefaultPicture;

/*
internal functions
*/

_Check_return_
static FIELD_TYPE
dplib_translate_type_to_db(
    _In_        S32 type);

_Check_return_
static S32
dplib_translate_type_to_datapower(
    _In_        FIELD_TYPE ft);

_Check_return_
static S32
dplib_translate_cardno_to_db(
    _In_        cardnum c);

_Check_return_
static cardnum
dplib_translate_cardno_from_db(
    _In_        S32 r);

_Check_return_
static STATUS
init_field_info(
    fieldinfostr * p_field_info_str,
    _InRef_     PC_FIELDDEF p_fielddef);

_Check_return_
static STATUS
dplib_new_file_layout(
    _In_z_      PCTSTR filename,
    fieldsptr p_fields,
    _In_        S32 n_flds,
    cached_layout ccl);

/* LOW LEVEL DATABASE FUNCTIONS */

_Check_return_
extern STATUS
dplib_init(void)
{
    if(!EncryptBuffer)
    {
        /* Neil says er... 512 no er 1K no better make it 2K  "J.... H C...." */
        windflex_alloc(&EncryptBuffer, 2048);

        if(!EncryptBuffer)
            return(status_nomem());
    }

    /* Read DateFormat and Holidays files */
    return(rec_oserror_set(date_init() /* this allocs a block which it then doesn't free */));
}

static void
dplib_delayed_initialise(void)
{
    static BOOL dplib_has_started = FALSE;

    if(!dplib_has_started)
    {
        dplib_has_started = TRUE; /* Once & only once */

        if(global_preferences.rec_dp_client)
        {
            (void) ClientServer_Initialise(); /* Create Econet event handler for client update requests */

            /* Kick null events into life as we need to poll Neil for data changes from Datapower server */
            trace_0(TRACE_OUT | TRACE_ANY, TEXT("dplib_delayed_initialise - *** null_events_start(DOCNO_NONE)"));
            status_assert(null_events_start(DOCNO_NONE, T5_EVENT_NULL, object_rec, (CLIENT_HANDLE) 0));
        }
    }
}

/* read_database()

1.  Opens the database file given by filename
2.  Fills in the DB structure given by p_db
    As part of this it creates a memory block holding the table defs collection.
    However, the fields collection for each table is not set up.
*/

_Check_return_
extern STATUS
dplib_read_database(
    P_DB p_db,
    _In_z_      PCTSTR filename)
{
    STATUS status = STATUS_OK;
    fields_file fp;

    dplib_delayed_initialise();

    status_return(rec_oserror_set(fields_open_file(&fp, de_const_cast(char *, filename), (os_filetype) -1 /* -1 for the filetype => dont care ??? */)));

    {
    S32 count = 0; /* Datapower only lets us have one ? or is infinity 28 ? */
    PTSTR name = file_leafname(filename); /* A name for the table ? */

    {
    P_TABLEDEF p_tabledef;
    SC_ARRAY_INIT_BLOCK aib = aib_init(1, sizeof32(*p_tabledef), TRUE);
    if(NULL != (p_tabledef = al_array_extend_by(&p_db->h_tabledefs, TABLEDEF, 1, &aib, &status)))
    {
        p_tabledef->id = (TABLE_ID) def_DataPower;
        tstr_xstrkpy(p_tabledef->name, elemof32(p_tabledef->name), name);
        count++;
    }
    } /*block*/

    p_db->n_tables = count; /* Datapower only lets us have one ? */

    if(p_db->n_tables > 0)
    {
        p_db->private_dp_handle = (P_ANY) fp;
    }
    else
    {
        status_return(rec_oserror_set(fields_close_file(fp, NULL))); /* the file has to be closed */

        status = create_error(REC_ERR_CANT_OPEN_DB);
    }
    } /*block*/

    return(status);
}

/******************************************************************************
*
* Create a new datapower file of a particular structure
* given a remapping array and a filename
*
******************************************************************************/

_Check_return_
extern STATUS
dplib_remap_file(
    _In_z_      PCTSTR filename,
    P_ARRAY_HANDLE p_handle,
    P_ANY private_dp_layout)
{
    fieldsptr p_fields;
    S32 counted_fields = 0;
    ARRAY_INDEX i;

    status_return(rec_oserror_set(fields_new(&p_fields, NULL)));

    /* For all remap entries ... */
    for(i = 0; i < array_elements(p_handle); i++)
    {
        P_REMAP_ENTRY p_remap = array_ptr(p_handle, REMAP_ENTRY, i);
        P_TABLEDEF p_table = p_remap->p_table;
        P_FIELDDEF p_fielddef;
        fieldinfostr field_info_str;
        fieldptr p_field;

        if(NULL == p_table) /* Some entries may be blank */
            continue;

        if(p_remap->field_id == -1)
        {
            field_new(p_fields, &p_field, NULL); /* A new field, with a new id */

            {
            P_FIELDDEF p_fielddef_t = p_fielddef_from_field_id(&p_table->h_fielddefs, p_remap->field_id);
            p_fielddef_t->id = field_getid(p_field); /* remember the new field id */
            p_remap->field_id = p_fielddef_t->id;
            } /*block*/

            p_fielddef = p_fielddef_from_field_id(&p_table->h_fielddefs, p_remap->field_id);
        }
        else
        {
            p_fielddef = p_fielddef_from_field_id(&p_table->h_fielddefs, p_remap->field_id);

            if(P_DATA_NONE != p_fielddef)
                field_new(p_fields, &p_field, p_fielddef->p_field); /* Copy from this field,  therefore id preserved? */
            else
            {
                myassert0(TEXT("Oh dear the remap failed to find a field id"));
                break;
            }
        }

        init_field_info(&field_info_str, p_fielddef);

        field_set(p_field, &field_info_str, finfo_All);

        switch(p_remap->sort_order)
        {
        default: break;
        case SORT_NULL: field_setflags(p_field, (fieldflags)   0, fflag_KeyMask); break;
        case SORT_AZ:   field_setflags(p_field, fflag_PrimKeyAsc, fflag_KeyMask); break;
        case SORT_ZA:   field_setflags(p_field, fflag_PrimKeyDsc, fflag_KeyMask); break;
        }

        p_remap->new_field_id = field_getid(p_field); /* remember the new field id */

        counted_fields ++;
    } /* End for all remap entries */

    return(dplib_new_file_layout(filename, p_fields, counted_fields, private_dp_layout));
}

/* Create a new datapower file

   The structure is defined by the field list given...

   The layout in the output file will be the default one created
   by write_frames_chunky.
*/

_Check_return_
static STATUS
dplib_new_file_layout(
    _In_z_      PCTSTR filename,
    fieldsptr p_fields,
    _In_        S32 n_flds,
    cached_layout ccl)
{
    _kernel_oserror * e;
    STATUS status = STATUS_OK;
    fields_file fp;
    int rootno;

    dplib_delayed_initialise();

    for(;;)
    {
        if(NULL != (e = fields_open_file(&fp, de_const_cast(char *, filename), filetype_PiscesDB)))
            break;

        if(NULL != (e = fields_new_rootno(fp, &rootno)))
            break;

        if(NULL != p_fields)
            fields_setfile(p_fields, fp, rootno);

        status_break(status = write_frames_chunky(fp, rootno, p_fields, n_flds));

        e = fields_copylayouts(fp, rootno, p_fields, NULL, TRUE, ccl); /* ccl used to get layout info propagated, NULL is ok */

        e = fields_flush(fp, e);

        if(NULL != p_fields)
            fields_setfile(p_fields, NULL, 0); /* And unlink again */

        e = fields_delete(p_fields, e);

        e = fields_close_file(fp, e);

        break;
        /*NOTREACHED*/
    }

    return(e ? rec_oserror_set(e) : status);
}

_Check_return_
extern STATUS
dplib_export(
    _In_z_      PCTSTR filename,
    P_OPENDB p_opendb)
{
    STATUS status = STATUS_OK;
    TABLEDEF fake_table;
    ARRAY_INDEX n_fields = 0;
    ARRAY_INDEX i;
    P_FIELDDEF p_fielddef_dst = NULL /*keep dataflower happy*/;
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(*p_fielddef_dst), TRUE);

    /* create a table and allocate space for all entries which are not hidden */
    make_fake_table(&fake_table);

    for(i = 0; i < array_elements(&p_opendb->table.h_fielddefs); i++)
    {
        PC_FIELDDEF p_fielddef = array_ptrc(&p_opendb->table.h_fielddefs, FIELDDEF, i);

        if(p_fielddef->hidden)
            continue;

        ++n_fields;
    }

    if(0 == n_fields)
        status = create_error(REC_ERR_NO_VISIBLE_FIELDS);
    else
        p_fielddef_dst = al_array_extend_by(&fake_table.h_fielddefs, FIELDDEF, n_fields, &array_init_block, &status);

    if(status_ok(status))
    {
        for(i = 0; i < array_elements(&p_opendb->table.h_fielddefs); i++)
        {
            PC_FIELDDEF p_fielddef_src = array_ptrc(&p_opendb->table.h_fielddefs, FIELDDEF, i);

            if(p_fielddef_src->hidden)
                continue;

            memcpy32(p_fielddef_dst, p_fielddef_src, sizeof32(*p_fielddef_src));

            ++p_fielddef_dst;
        }

        status = remap_remap(p_opendb, &fake_table, filename);
    }

    return(drop_table(&fake_table, status));
}

/******************************************************************************
*
* Create a new datapower file
*
* Given complete table and a filename manufacture a field list and
* use the dplib_new_file_layout call to get a file constructed
*
******************************************************************************/

_Check_return_
extern STATUS
dplib_clone_structure(
    _In_z_      PCTSTR filename,
    P_TABLEDEF p_table)
{
    fieldsptr p_fields;

    status_return(rec_oserror_set(fields_new(&p_fields, (fieldsptr) p_table->h_fields)));

    return(dplib_new_file_layout(filename, p_fields, array_elements(&p_table->h_fielddefs), p_table->private_dp_layout));
}

_Check_return_
extern STATUS
dplib_copy_fields(
    P_TABLEDEF p_table_dst,
    P_TABLEDEF p_table_src)
{
    p_table_dst->h_fields = NULL;

    return(rec_oserror_set(fields_new((fieldsptr *) &p_table_dst->h_fields, (fieldsptr) p_table_src->h_fields)));
}

/******************************************************************************
*
* Create a new datapower file
*
* using the table given which is a prototype table
* where the non-datapower bits have been constructed
*
******************************************************************************/

_Check_return_
extern STATUS
dplib_create_from_prototype(
    _In_z_      PCTSTR filename,
    P_TABLEDEF p_table)
{
    /* Create in-core fields structures */
    status_return(rec_oserror_set(fields_new((fieldsptr *) &p_table->h_fields, NULL /*nothing to copy from*/)));

    { /* For all fields do create a field in the eyes of dplib from the pro-forma fielddef */
    ARRAY_INDEX i;

    for(i = 0; i < array_elements(&p_table->h_fielddefs); i++)
    {
        P_FIELDDEF p_fielddef = array_ptr(&p_table->h_fielddefs, FIELDDEF, i);
        fieldptr p_field;
        fieldinfostr field_info_str;

        /* Get datapower to create a field and set up the attributes */
        field_new((fieldsptr) p_table->h_fields, &p_field, NULL /*nothing to copy from*/);

        /* THIS NEEDS TO DERIVE STUFF FROM THE FIELDDEF */
        init_field_info(&field_info_str, p_fielddef);

        field_set(p_field, &field_info_str, finfo_All);

        p_fielddef->p_field = p_field;
    }
    } /*block*/

    return(dplib_clone_structure(filename, p_table)); /* p_table is now a completed table */
}

/* LOW LEVEL FIELD FUNCTIONS */

/* Routine to make a field_info_str from a FIELDDEF */

_Check_return_
static STATUS
init_field_info(
    fieldinfostr * p_field_info_str,
    _InRef_     PC_FIELDDEF p_fielddef)
{
    xstrkpy(p_field_info_str->name, elemof32(p_field_info_str->name), p_fielddef->name);

    xstrkpy(p_field_info_str->default_formula, elemof32(p_field_info_str->default_formula), p_fielddef->default_formula); /* default (value if formula)         */
    xstrkpy(p_field_info_str->check_formula,   elemof32(p_field_info_str->check_formula),   p_fielddef->check_formula);   /* expression which must be true      */
    xstrkpy(p_field_info_str->value_list,      elemof32(p_field_info_str->value_list),      p_fielddef->value_list);      /* list of allowed values             */

    p_field_info_str->type = (fieldtype) dplib_translate_type_to_datapower(p_fielddef->type);

    switch(p_fielddef->keyorder)
    {
    default:  p_field_info_str->flags = (fieldflags) 0;   break;
    case +1:  p_field_info_str->flags = fflag_PrimKeyAsc; break;
    case -1:  p_field_info_str->flags = fflag_PrimKeyDsc; break;
    }

#if 0
    if(p_fielddef->compulsory) /* SKS 19jun95 compulsory bit means you can't add records so never apply it */
        p_field_info_str->flags |= fflag_NotNull;
#endif

    p_field_info_str->flagmask  = (fieldflags) -1; /* for field_set() */
    p_field_info_str->reference = NULL; /* reference to another field */

/* I suspect that the ffflag_Thingy state should also be type-related

typedef enum field_fflags
{
    ffflag_Decimal     = 1 << 0,
    ffflag_Thousands   = 1 << 1,
    ffflag_NegBrackets = 1 << 2,
    ffflag_NegRed      = 1 << 3,
    ffflag_Interval    = 1 << 4
} field_fflags;

What do you think?
*/

    p_field_info_str->format.fflags = ffflag_Decimal; /* field format (decimal format) */

    switch(p_fielddef->type)
    {
    default:
        p_field_info_str->format.decimal = 0; /* field format (decimal places) */
        break;

    case FIELD_FORMULA: /* SKS 03mar95 */
    case FIELD_REAL:
        p_field_info_str->format.decimal = 9; /* field format (decimal places) */
        break;
    }

    p_field_info_str->format.prefix[0] = CH_NULL; /* eg $ £  */
    p_field_info_str->format.suffix[0] = CH_NULL; /* M K % etc */

    return(STATUS_OK);
}

_Check_return_
extern STATUS
dplib_create_field(
    P_TABLEDEF p_table,
    P_FIELDDEF p_fielddef)
{
    fieldinfostr field_info_str;

    if(NULL == p_table->h_fields)
        status_return(rec_oserror_set(fields_new((fieldsptr *) &p_table->h_fields, NULL))); /* Invent the collection if null */

    field_new((fieldsptr) p_table->h_fields, (fieldptr *) &p_fielddef->p_field, NULL); /* Invent a field */

    init_field_info(&field_info_str, p_fielddef);

    field_set((fieldptr) p_fielddef->p_field, &field_info_str, finfo_All);

    p_fielddef->id = field_getid((fieldptr) p_fielddef->p_field);

    return(STATUS_OK);
}

_Check_return_
static STATUS
dplib_add_field_to_table(
    P_TABLEDEF p_table,
    fieldptr p_field)
{
    STATUS status;
    fieldinfostr field_info_str;
    P_FIELDDEF p_fielddef;
    SC_ARRAY_INIT_BLOCK aib = aib_init(1, sizeof32(*p_fielddef), TRUE);

    if(NULL == (p_fielddef = al_array_extend_by(&p_table->h_fielddefs, FIELDDEF, 1, &aib, &status)))
        return(status);

    wipe_fielddef(p_fielddef);

    status_return(rec_oserror_set(field_get(p_field, &field_info_str)));

    {
    fieldflags flags = field_getflags(p_field);
    P_U8 p_u8_name = field_name(p_field);

    if(NULL != p_u8_name)
        xstrkpy(p_fielddef->name, elemof32(p_fielddef->name), p_u8_name);

    p_fielddef->id       = (FIELD_ID) field_getid(p_field);
    p_fielddef->length   = -1;
    p_fielddef->type     = (FIELD_TYPE) dplib_translate_type_to_db(field_type(p_field));
    p_fielddef->parentid = p_table->id;

    xstrkpy(p_fielddef->default_formula, elemof32(p_fielddef->default_formula), field_info_str.default_formula);
    xstrkpy(p_fielddef->check_formula,   elemof32(p_fielddef->check_formula),   field_info_str.check_formula);
    xstrkpy(p_fielddef->value_list,      elemof32(p_fielddef->value_list),      field_info_str.value_list);

    p_fielddef->readable  = ((flags & fflag_Access_R) != 0);
    p_fielddef->writeable = ((flags & fflag_Access_W) != 0);

    if(p_fielddef->type  == FIELD_FORMULA)
        p_fielddef->writeable = FALSE; /* I should hope not */

    { /* SKS 26mar96 stop punters editing fields on Protected discs etc */
    itemaccess ia = fields_getaccess(p_table->private_dp_layout);

    if(0 == (ia & itemaccess_Edit))
        p_fielddef->writeable = FALSE;

    if(fields_readonly(p_table->private_dp_layout))
        p_fielddef->writeable = FALSE;
    } /*block*/

    p_fielddef->compulsory = ((flags & fflag_NotNull) == fflag_NotNull);

    switch(flags & fflag_KeyMask)
    {
    default:                p_fielddef->keyorder =  0; break;
    case fflag_PrimKeyAsc:  p_fielddef->keyorder = +1; break;
    case fflag_PrimKeyDsc:  p_fielddef->keyorder = -1; break;
    }

    p_fielddef->p_field = (P_ANY) p_field;
    } /*block*/

    return(status);
}

/* read the datapower-bit-mask for a given dbread-function
   unknown functions will yeidl the all-bit-mask
*/

static itemaccess
dplib_itemaccess_bit(
    _InVal_     ACCESS_FUNCTION access_function)
{
    switch(access_function)
    {
    default                       : return(itemaccess_All);

    case ACCESS_FUNCTION_NONE     : return(itemaccess_None);
    case ACCESS_FUNCTION_ALL      : return(itemaccess_All);

    case ACCESS_FUNCTION_FULL     : return(itemaccess_Full);
    case ACCESS_FUNCTION_READ     : return(itemaccess_Browse);
    case ACCESS_FUNCTION_ADD      : return(itemaccess_Add);
    case ACCESS_FUNCTION_EDIT     : return(itemaccess_Edit);
    case ACCESS_FUNCTION_DELETE   : return(itemaccess_Delete);
    case ACCESS_FUNCTION_SEARCH   : return(itemaccess_Search);
    case ACCESS_FUNCTION_SORT     : return(itemaccess_Sort);
    case ACCESS_FUNCTION_LAYOUT   : return(itemaccess_Layouts);
    case ACCESS_FUNCTION_SAVE     : return(itemaccess_Save);
    case ACCESS_FUNCTION_PRINT    : return(itemaccess_Print);
    case ACCESS_FUNCTION_PASSWORD : return(itemaccess_Password);
    }
}

/* Pass in an function_function and I'll tell you if it is allowed
   NB This is a one-at-a-time call.
   You CAN NOT combine access_function descriptors by or-ing them togehter!
*/

_Check_return_
extern STATUS
dplib_check_access_function(
    P_TABLEDEF p_table,
    _InVal_     ACCESS_FUNCTION access_function)
{
    itemaccess table_ia = fields_getaccess(p_table->private_dp_layout);
    itemaccess function_ia = dplib_itemaccess_bit(access_function);

    if((table_ia & function_ia) != 0)
        return(STATUS_OK);

    switch(access_function)
    {
    default: default_unhandled();
#if CHECKING
    case ACCESS_FUNCTION_READ:
#endif
        return(create_error(REC_ERR_NO_ACCESS_READ));

    case ACCESS_FUNCTION_ADD:
        return(create_error(REC_ERR_NO_ACCESS_ADD));

    case ACCESS_FUNCTION_EDIT:
        return(create_error(REC_ERR_NO_ACCESS_EDIT));

    case ACCESS_FUNCTION_DELETE:
        return(create_error(REC_ERR_NO_ACCESS_DELETE));

    case ACCESS_FUNCTION_LAYOUT:
        return(create_error(REC_ERR_NO_ACCESS_LAYOUT));

    case ACCESS_FUNCTION_SAVE:
        return(create_error(REC_ERR_NO_ACCESS_SAVE));

    case ACCESS_FUNCTION_SEARCH:
        return(create_error(REC_ERR_NO_ACCESS_SEARCH));

    case ACCESS_FUNCTION_SORT:
        return(create_error(REC_ERR_NO_ACCESS_SORT));
    }
}

/*
Scan the database for field definitions associated with this table.
Build them into a fields collection memory block with the handle in h_fields of p_table.
Checks for consistancy of the number of fields
*/

_Check_return_
static STATUS
dplib_loadfields(
    cached_layout ccl,
    fieldsptr * fields,
    char ***anchor,
    int * size)
{
    int rootno;
    int relid;

    status_return(rec_oserror_set(fields_loadlayout(ccl, &rootno, &relid, NULL, anchor, size)));

    /* read field definitions from the file */
    status_return(rec_oserror_set(fields_loadfields(fields_layoutfp(ccl), rootno, fields)));

    /* open and processes password dbox if necessary */
    return(rec_oserror_set(fields_getfileaccess(fields_layoutfp(ccl), rootno, *fields)));
}

_Check_return_
extern STATUS
dplib_read_fields_for_table(
    P_DB p_db,
    P_TABLEDEF p_table)
{
    cached_layout layout;

    status_return(rec_oserror_set(fields_findlayout_withaccess((fields_file) p_db->private_dp_handle, def_DataPower, &layout)));

    /* if there was a password and the user said cancel this returns 1 (layout of NULL)
       it returns 0 if there was no password or if the password was given correctly
    */
    if(NULL == layout)
        return(REC_ERR_NO_ACCESS_TO_DATABASE);

    {
    STATUS status;
    fieldsptr fieldsp;
    char ** anchor;
    int size;

    p_table->private_dp_layout = layout;

    /* insist that browse access exists */
    status_return(dplib_check_access_function(p_table, ACCESS_FUNCTION_READ));

    /* now get the field for this layout */
    if(status_ok(status = dplib_loadfields(layout, &fieldsp, &anchor, &size)))
    {
        fieldptr p_field;

        p_table->h_fields = (P_ANY) fieldsp;

        /* Navigate the datapower fields list building up a proper fields collection
           This should produce the fields collection in the 'entry' order not
           the order in the file because the latter is used to determine the sort-order
           when a database is (re-)constructed.
        */

        /* For all fields on the list do...create a fielddef */
        for(p_field = fields_head((fieldsptr)fieldsp); p_field; p_field = field_next(p_field))
            if(0 == (field_getflags(p_field) & fflag_Surrogate))
                status_break(status = dplib_add_field_to_table(p_table, p_field));
    }
    else
    {
        status_assert(rec_oserror_set(fields_close_file(fields_layoutfp(layout), NULL))); /* deletes fields if no more references to this file */

        p_table->h_fields = NULL;
    }

    return(status);
    } /*block*/
}

_Check_return_
extern STATUS
dplib_fields_delete_for_table(
    P_TABLEDEF p_table)
{
    STATUS status = STATUS_OK;

    if(p_table->h_fields)
    {
        status = rec_oserror_set(fields_delete((fieldsptr) p_table->h_fields, NULL));

        p_table->h_fields = NULL;
    }

    return(status);
}

_Check_return_
extern STATUS
dplib_drop_fields_for_table(
    P_TABLEDEF p_table)
{
    STATUS status = STATUS_OK;

    /* deletes fields if no more references to this file */
    if(p_table->private_dp_layout)
    {
        _kernel_oserror * e = dplib_remove_cursor(p_table);

        e = fields_close_file(fields_layoutfp(p_table->private_dp_layout), e);

        status = rec_oserror_set(e);

        p_table->h_fields = NULL;

        p_table->private_dp_layout = NULL;
    }

    return(status);
}

_Check_return_
static STATUS
rec_set_binary(
    P_REC_RESOURCE p_rec_resource,
    char *** p_anchor,
    _In_        int offset,
    _In_        int size)
{
    P_BYTE p_byte = "\xDD"; /* a pointer to SOMETHING at least (easy to spot in debug) */
    char * text;

    if(0 != size)
    {
        STATUS status;

        if(NULL == (p_byte = al_ptr_alloc_bytes(P_BYTE, size, &status)))
            return(status);

        p_rec_resource->ev_data.local_data = 1;
    }

    p_rec_resource->ev_data.did_num = RPN_DAT_STRING; /* eek! */
    p_rec_resource->ev_data.arg.db_blob.data = p_byte;
    p_rec_resource->ev_data.arg.db_blob.size = size;

    text = **p_anchor;
    text += offset;

    memcpy32(p_byte, text, size);

    return((0 != size) ? STATUS_DONE : STATUS_OK);
}

/* This routine expects to take text containing LF as line-break chars and converts them to Fireworkz inlines or plain text */

_Check_return_
static STATUS
rec_set_text(
    P_REC_RESOURCE p_rec_resource,
    char *** p_anchor,
    _In_        int offset,
    _In_        int size_in,
    _InVal_     BOOL plain)
{
    P_U8 p_u8 = "\xDD"; /* a pointer to SOMETHING at least (easy to spot in debug) */
    char * text;
    int size_needed = size_in;
    int count_newlines = 0;
    UCHARB_INLINE il_return_buffer[INLINE_OVH + 1];
    U32 il_return_len = 0;

    { /* must compute size required allowing for inline codes */
    int i;
    PC_U8Z p_u8_in = **p_anchor;
    p_u8_in += offset;
    for(i = 0; i < size_in; i++)
        if(LF == *p_u8_in++)
            count_newlines++;
    } /*block*/

    if(count_newlines)
    {
        if(plain)
        {
            strcpy(il_return_buffer, " "); /* SKS 15apr96 was "\\n" */
            il_return_len = strlen(il_return_buffer);
        }
        else
            il_return_len = inline_uchars_buf_from_data(ustr_inline_bptr(il_return_buffer), INLINE_OVH, IL_RETURN, IL_TYPE_NONE, NULL, 0);

        size_needed += (il_return_len - 1) * count_newlines; /* replacing 1 char with m chars n times */
    }

    if(0 != size_needed)
    {
        STATUS status;

        if(NULL == (p_rec_resource->ev_data.arg.string_wr.uchars = al_ptr_alloc_bytes(P_UCHARS, size_needed, &status)))
            return(status);

        p_u8 = (P_U8) p_rec_resource->ev_data.arg.string_wr.uchars;

        p_rec_resource->ev_data.local_data = 1;
    }

    p_rec_resource->ev_data.did_num = RPN_DAT_STRING;
    p_rec_resource->ev_data.arg.string.size = size_needed;

    text = **p_anchor;
    text += offset;

    /* now we need to copy the data */
    if(0 == count_newlines)
        memcpy32(p_u8, text, size_in);
    else
    {
        int i;
        PC_U8Z p_u8_in = text;

        for(i = 0; i < size_in; ++i)
        {
            if(LF == *p_u8_in)
            {
                memcpy32(p_u8, il_return_buffer, il_return_len);
                p_u8 += il_return_len;
            }
            else
                *p_u8++ = *p_u8_in;

            p_u8_in++;
        }
    }

    return((0 != size_needed) ? STATUS_DONE : STATUS_OK);
}

_Check_return_
extern STATUS
dplib_field_get_value(
    /*out*/ P_REC_RESOURCE p_rec_resource,
    _InRef_     PC_FIELDDEF p_fielddef,
    _InVal_     BOOL plain)
{
    fieldtype ftyp = field_type(p_fielddef->p_field); /* NB using DataPower ftyp_xxx throughout */
    fieldtype ftyp_result = ftyp;
    char buffer[sizeof32(field_value)];
    field_value * p_fv  = (P_ANY) &buffer[0];
    char * textptr = &buffer[0];
    char ** anchor = &textptr;
    int offset;
    int size;

    p_rec_resource->t5_filetype = FILETYPE_TEXT; /* sensible defaults */

    ev_data_set_blank(&p_rec_resource->ev_data);
    p_rec_resource->ev_data.local_data = 0;

    switch(ftyp)
    {
    default: default_unhandled();
#if CHECKING
    case ftyp_COMMENT:
    case ftyp_FILE:
    case ftyp_GRAPHIC:
#endif
        break;

    case ftyp_FORMULA:
        {
        status_return(rec_oserror_set(field_getexprtype(p_fielddef->p_field, &ftyp_result)));

        if(ftyp_INTERVAL == ftyp_result)
            ftyp_result = ftyp_TEXT;

        if(ftyp_TEXT != ftyp_result)
        {
            status_return(rec_oserror_set(field_getvalue(p_fielddef->p_field, p_fv)));

            if(field_isnull(ftyp_result, p_fv)) /* SKS 19aug96 for calculated fields that may yield NULL */
                return(STATUS_OK);
        }

        break;
        }

    case ftyp_BOOLEAN:
    case ftyp_DATE:
    case ftyp_REAL:
    case ftyp_INTEGER:
        {
        status_return(rec_oserror_set(field_getvalue(p_fielddef->p_field, p_fv)));

        if(field_isnull(ftyp, p_fv)) /* SKS now queries should this be ftyp_result too? */
            return(STATUS_OK);

        break;
        }

    case ftyp_INTERVAL:
        {
        status_return(rec_oserror_set(field_getvalue(p_fielddef->p_field, p_fv)));

        /* no NULL test 'cos Neil's a prat */
        break;
        }

    case ftyp_TEXT:
        break;
    }

    switch(ftyp_result)
    {
    default: default_unhandled();
#if CHECKING
    case ftyp_FILE:
    case ftyp_GRAPHIC:
#endif
        {
        if(plain)
            return(ss_string_make_ustr(&p_rec_resource->ev_data, p_fielddef->name));

        status_return(rec_oserror_set(field_getvalue_text(p_fielddef->p_field, NULL, (os_filetype *) &p_rec_resource->t5_filetype, &anchor, &offset, &size)));

        /* SKS 07may95 try to not kill ourselves with naff files we created in 1.21/02 */
        /* it's a pity about old DataPower though... it just explodes in flames */
        switch(p_rec_resource->t5_filetype)
        {
        case FILETYPE_DRAW:
            {
            int extra = size & 3;
            if(1 == extra) /* our mode of failure */
                size -= extra;
            break;
            }

        default:
            break;
        }

        return(rec_set_binary(p_rec_resource, &anchor, offset, size));
        }

    case ftyp_BOOLEAN:
        {
        S32 blather = (S32) p_fv->boolean;
        PC_U8Z p_u8;

        switch(blather)
        {
        case 0:  p_u8 = "No";  break;
        case 1:  p_u8 = "Yes"; break;
        default: return(STATUS_OK); /* any other value maps as NULL */
        }

        strcpy(buffer, p_u8);

        return(rec_set_text(p_rec_resource, &anchor, 0, strlen(buffer), FALSE)); /* SKS 07jul96 was str_hlp_make */
        }

    case ftyp_DATE:
        {
        S32 year  = (S32) date_year(p_fv->date);
        S32 month = (S32) date_month(p_fv->date);
        S32 day   = (S32) date_day(p_fv->date);
        S32 hour  = (S32) date_hour(p_fv->date);
        S32 mins  = (S32) date_minute(p_fv->date);
        S32 secs  = (S32) date_second(p_fv->date);
        BOOL null_date = ((day  == NULL_DAY)  && (month == NULL_MONTH)  && (year == NULL_YEAR));
        BOOL null_time = ((hour == NULL_HOUR) && (mins  == NULL_MINUTE) && (secs == NULL_SECOND));

        if(year == NULL_YEAR)
        {
            S32 temp_m, temp_d, temp_h, temp_min, temp_s;
            ss_local_time(&year, &temp_m, &temp_d, &temp_h, &temp_min, &temp_s);
        }
        if(month == NULL_MONTH)
            month = 1;
        if(day == NULL_DAY)
            day = 1;

        if(hour == NULL_HOUR)
            hour = 0;
        if(mins == NULL_MINUTE)
            mins = 0;
        if(secs == NULL_SECOND)
            secs = 0;

        ev_date_init(&p_rec_resource->ev_data.arg.ev_date);

        if(!null_date)
            (void) ss_ymd_to_dateval(&p_rec_resource->ev_data.arg.ev_date.date, year, month, day);

        if(!null_time)
            (void) ss_hms_to_timeval(&p_rec_resource->ev_data.arg.ev_date.time, hour, mins, secs);

        p_rec_resource->ev_data.did_num = RPN_DAT_DATE;

        return(STATUS_DONE);
        }

    case ftyp_REAL:
        {
        F64 f_number = (F64) p_fv->real;

        ev_data_set_real(&p_rec_resource->ev_data, f_number);

        return(STATUS_DONE);
        }

    case ftyp_INTEGER:
        {
        S32 i_number = (S32) p_fv->integer;

        ev_data_set_integer(&p_rec_resource->ev_data, i_number);

        return(STATUS_DONE);
        }

    case ftyp_INTERVAL:
        interval_write(buffer, &p_fv->interval.years);
        return(rec_set_text(p_rec_resource, &anchor, 0, strlen(buffer), FALSE));

    case ftyp_TEXT:
        status_return(rec_oserror_set(field_getvalue_text(p_fielddef->p_field, NULL, (os_filetype *) &p_rec_resource->t5_filetype, &anchor, &offset, &size)));
        return(rec_set_text(p_rec_resource, &anchor, offset, size, plain));
    }

    /*NOTREACHED*/
    /*return(STATUS_OK);*/
}

_Check_return_
extern STATUS
dplib_locate_record(
    P_TABLEDEF p_table,
    P_S32 p_recno,
    P_S32 p_ncards,
    P_S32 p_lastvalid)
{
    cardnum cardn = dplib_translate_cardno_from_db(*p_recno);
    status_return(rec_oserror_set(cursor_get(p_table->private_dp_cursor, &cardn, (int *) p_ncards, (int *) p_lastvalid, lock_View, NULL)));
    *p_recno = dplib_translate_cardno_to_db(cardn);
    return(STATUS_OK);
}

_Check_return_
extern STATUS
dplib_current_record(
    P_TABLEDEF p_table,
    P_S32 p_recno,
    P_S32 p_ncards)
{
    cardnum cardno;
    int numcards, lastvalid, foundall;

    *p_recno = -1;
    *p_ncards = -1;

    if(NULL == p_table->private_dp_cursor)
        return(STATUS_FAIL);

    status_return(rec_oserror_set(cursor_getinfo(p_table->private_dp_cursor, &cardno, &numcards, &lastvalid, &foundall)));

    *p_recno = dplib_translate_cardno_to_db(cardno);
    *p_ncards = numcards;

    return(STATUS_OK);
}

/* Enforce the NON-NULL cursor state */

_Check_return_
extern STATUS
dplib_create_cursor(
    P_TABLEDEF p_table,
    P_ANY p_q17)
{
    if(NULL == p_table->private_dp_cursor)
    {
        _kernel_oserror * e = cursor_open((fieldsptr) p_table->h_fields, (query)p_q17, lock_View, (cursor *) &p_table->private_dp_cursor);

        if(NULL != e)
        {
            p_table->private_dp_cursor = NULL; /* well, I don't trust it... */

            return(rec_oserror_set(e));
        }
    }

    return(STATUS_OK);
}

/* Enforce the NULL cursor state */

_Check_return_
_Ret_maybenull_
extern _kernel_oserror *
dplib_remove_cursor(
    P_TABLEDEF p_table)
{
    _kernel_oserror * e = NULL;

    if(NULL != p_table->private_dp_cursor)
    {
        e = cursor_close(p_table->private_dp_cursor, NULL);

        p_table->private_dp_cursor = NULL;
    }

    return(e);
}

static FIELD_TYPE
dplib_translate_type_to_db(
    _In_        S32 type)
{
    switch(type)
    {
    case FT_TEXT     : return(FIELD_TEXT);
    case FT_BOOL     : return(FIELD_BOOL);
    case FT_INTEGER  : return(FIELD_INTEGER);
    case FT_REAL     : return(FIELD_REAL);
    case FT_GRAPHIC  : return(FIELD_PICTURE);
    case FT_FILE     : return(FIELD_FILE);
    case FT_DATE     : return(FIELD_DATE);
    case FT_FORMULA  : return(FIELD_FORMULA);
    case FT_INTERVAL : return(FIELD_INTERVAL);
    case FT_COMMENT  : return 0;
    default: default_unhandled(); return(FIELD_TEXT);
    }
}

static S32
dplib_translate_type_to_datapower(
    _In_        FIELD_TYPE ft)
{
    switch(ft)
    {
    case FIELD_TEXT     : return(FT_TEXT);
    case FIELD_BOOL     : return(FT_BOOL);
    case FIELD_INTEGER  : return(FT_INTEGER);
    case FIELD_REAL     : return(FT_REAL);
    case FIELD_PICTURE  : return(FT_GRAPHIC);
    case FIELD_FILE     : return(FT_FILE);
    case FIELD_DATE     : return(FT_DATE);
    case FIELD_FORMULA  : return(FT_FORMULA);
    case FIELD_INTERVAL : return(FT_INTERVAL);
    default: default_unhandled(); return(FT_TEXT);
    }
}

static cardnum
dplib_translate_cardno_from_db(
    _In_        S32 r)
{
    switch(r)
    {
    case rec_First   : return(card_First);      /* go to first card                             */
    case rec_Last    : return(card_Last);       /* go to last card                              */
    case rec_Next    : return(card_Next);       /* go to next card                              */
    case rec_Previous: return(card_Previous);   /* go to previous card                          */
    case rec_Current : return(card_Current);    /* read current card                            */
    case rec_Dunno   : return(card_Dunno);      /* card number not known (+ve => card number n) */
    case rec_Finished: return(card_Finished);   /* on return => no more cards available         */
    default:           return((cardnum) r);
    }
}

static S32
dplib_translate_cardno_to_db(
    _In_        cardnum c)
{
    switch(c)
    {
    case card_First   : return(rec_First);      /* go to first card                             */
    case card_Last    : return(rec_Last);       /* go to last card                              */
    case card_Next    : return(rec_Next);       /* go to next card                              */
    case card_Previous: return(rec_Previous);   /* go to previous card                          */
    case card_Current : return(rec_Current);    /* read current card                            */
    case card_Dunno   : return(rec_Dunno);      /* card number not known (+ve => card number n) */
    case card_Finished: return(rec_Finished);   /* on return => no more cards available         */
    default:            return(c);
    }
}

_Check_return_
static STATUS
rebuild_query(
    P_OPENDB p_opendb,
    P_QUERY p_query,
    QUERY_ID query_id,
    _InVal_     BOOL substr,
    _InVal_     BOOL expr,
    _InVal_     BOOL all)
{
    QUERY_ID parent_id;
    BOOL andor, not;
    srch_op op;

    {
    P_QUERY p_query = p_query_from_p_opendb(p_opendb, query_id);

    parent_id = p_query->parent_id;

    switch(p_query->search_type)
    {
    case SEARCH_TYPE_FILE:   op = srch_Find;    break;
    case SEARCH_TYPE_EXTEND: op = srch_Add;     break;
    case SEARCH_TYPE_REFINE: op = srch_Refine;  break;
    default:                 op = srch_Find;    break;
    }

    andor = p_query->search_andor   ? 1 : 0;
    not   = p_query->search_exclude ? 1 : 0;
    } /*block*/

    /* do the recursion */
    if(parent_id != RECORDZ_WHOLE_FILE)
        status_return(rebuild_query(p_opendb, p_query, parent_id, substr, expr, all));

    p_opendb->search.query_id = query_id;

    if(search_buildexpr((searchexpr) p_query->p_expr, (framesptr) p_opendb, op, andor, not, substr, expr, all))
        return(STATUS_FAIL);

    return(STATUS_OK);
}

/* THIS WORKS BY USING THE CALL BACK FUNCTIONS IN EXTRAS

   search_buildexpr() uses the framesptr which you pass in to
   call you back for the enumeration of the fields and their
   contents.

   When opening the cursor you pass NULL (if show is FALSE)
   or the query (be it null or not)

  andor  =  [(0)OR,(1)AND] the fields
  not    =  [(0)Include,(1)Exclude] matching records
*/

_Check_return_
extern STATUS
dplib_create_query(
    P_OPENDB p_opendb,
    QUERY_ID query_id)
{
    fieldsptr fields = (fieldsptr) p_opendb->table.h_fields;
    BOOL substr = 0; /* [(0)direct,(1)Implicit *fred*]                  */
    BOOL expr   = 1; /* [(0)literal,(1)evaluable] data                  */
    BOOL all    = 0; /* [(0)particular,(1)any] field for the given data */
    P_QUERY p_query;
    _kernel_oserror * e = NULL;

    set_p_opendb_for_getting_at_things(p_opendb); /* A Global context variable YUK */

    p_query = p_query_from_p_opendb(p_opendb, query_id);
    p_query->p_expr = NULL;

    status_return(rec_oserror_set(search_new((searchexpr *) &p_query->p_expr)));

    status_return(rebuild_query(p_opendb, p_query, query_id, substr, expr, all));

    /* Discard existing cursor and query (if any) */
    status_return(rec_oserror_set(dplib_remove_cursor(&p_opendb->table)));

    dplib_remove_query(p_query_from_p_opendb(p_opendb, query_id), FALSE);

    if(search_isnull((searchexpr) p_query->p_expr))
    {
        e = search_delete((searchexpr *) &p_query->p_expr, e);
        query_id = RECORDZ_WHOLE_FILE;
        e = NULL;
    }
    else
    {
        e = fields_new_index((fieldsptr *) &p_query->p_qfields, fields, NULL);
        if(NULL == e)
            e = query_open((query *) &p_query->p_query, (fieldsptr) p_query->p_qfields, fields, (searchexpr) p_query->p_expr);
    }

    p_opendb->search.query_id = e ? RECORDZ_WHOLE_FILE : query_id;

    return(e ? rec_oserror_set(e) : STATUS_OK);
}

/* This removes the dplib specifix */

_Check_return_
extern STATUS
dplib_remove_query(
    P_QUERY p_query,
    _InVal_     BOOL delete_search_expr)
{
    _kernel_oserror * e = NULL;

    if(p_query->p_query)
    {
        e = query_close((query) p_query->p_query, e);
        p_query->p_query = NULL;
    }

    if(p_query->p_qfields)
    {
        e = fields_delete((fieldsptr) p_query->p_qfields, e);
        p_query->p_qfields = NULL;
    }

    if(p_query->p_expr && delete_search_expr)
    {
        e = search_delete((searchexpr *) &p_query->p_expr, e);
        p_query->p_expr = NULL;
    }

    return(e ? rec_oserror_set(e) : STATUS_OK);
}

_Check_return_
extern BOOL
dplib_is_db_on_server(
    P_OPENDB p_opendb)
{
    file_desc * file = fields_filedesc((fields_file) p_opendb->db.private_dp_handle);

    return((NULL != file) && file->server);
}

/*
Stuff for telling the server to do an update! You'd think it would notice!
*/

_Check_return_
static STATUS
kick_server(
    P_OPENDB p_opendb)
{
    file_desc * file = fields_filedesc((fields_file) p_opendb->db.private_dp_handle);

    if((NULL != file) && file->server)
    {
        _kernel_oserror * e = client_updateserver(file->server, (file_desc *) file->handle, ROOT_DATA); /* this will NOT call us back */
        IGNOREPARM(e);
    }

    return(STATUS_OK);
}

/*
Stuff for doing update when ordered to by the server
*/

_Check_return_
extern STATUS
dplib_changes_in_p_opendb(
    P_OPENDB p_opendb,
    P_ANY server,
    P_ANY handle)
{
    fieldsptr p_fields = (fieldsptr) p_opendb->table.h_fields;

    if(fields_matchserverfile(p_fields, (serverptr) server, (void *) handle))
        return(STATUS_DONE); /* Things to be done */

    return(STATUS_OK); /* Ok but nothing to do */
}

static os_error *
dplib_update_callback(
    serverptr server,
    _In_        void * handle,
    _In_        int rootno)
{
    switch(rootno)
    {
    case -1: return wind_suspendfile(server, handle);   /* suspend server file (without notifying other clients) */
    case -2: return wind_resumefile(server, handle);    /* resume server file (without notifying other clients) */
    }

    rec_inform_documents_of_changes((P_ANY) server, (P_ANY) handle);
    return(NULL);
}

/*-------------------------------------------------------------------------------*
 * Call this on NULL events to respond to server prodding us to update ourselves *
 *-------------------------------------------------------------------------------*/

extern U32 * CRNet_CreateReceive;

_Check_return_
extern STATUS
dplib_client_updates(void)
{
    if(CRNet_CreateReceive)
    {
        _kernel_oserror * e = client_scan_serverfiles(dplib_update_callback);

        if(NULL == e)
            return(STATUS_OK);
    }

    return(STATUS_FAIL);
}

/*
SUSPEND AND RESUME CODE
*/

typedef struct _resumestr * resumeptr;

typedef struct _resumestr
{
    resumeptr link;
    serverptr server;
    file_desc * file;
    char filename[4];
}
resumestr;

static resumeptr ResumeHead = NULL;

_Check_return_
extern BOOL
wind_getresumefile(
    char * filename,
    serverptr * serverp,
    file_desc ** filep)
{
    resumeptr r;

    for(r = ResumeHead; NULL != r; r = r->link)
    {
        if(0 == tstricmp(filename, r->filename))
        {
            if(serverp)
                *serverp = r->server;

            if(filep)
                *filep = r->file;

            return(TRUE);
        }
    }

    return(FALSE);
}

extern char *
wind_getresumefilename(
    serverptr server,
    file_desc * file)
{
    resumeptr r;

    for(r = ResumeHead; NULL != r; r = r->link)
    {
        if((r->server == server) && (r->file == file))
        {
            return(r->filename);
        }
    }

    return(NULL);
}

/*
 - Called from DPLib file_open() to check whether a file is being resumed on a server elsewhere.
 - If it is, it won't check the filetype of the file, since the !Server on the other machine won't get a look in.
 */

_Check_return_
extern BOOL
wind_isresumefile_onserver(
    char *filename)
{
    serverptr server;

    return(wind_getresumefile(filename, &server, NULL) && (NULL != server));
}

os_error *
wind_loseresumefile(
    char * filename,
    os_error * e)
{
    resumeptr r;
    resumeptr * tail = &ResumeHead;

    for(r = *tail; NULL != r; tail = &r->link, r = *tail)
    {
        if(0 == tstricmp(filename, r->filename))
        {
            *tail = r->link;
            al_ptr_dispose((P_P_ANY)&r);
            return(NULL);
        }
    }

    return(e);
}

os_error *
wind_setresumefile(
    char * filename,
    serverptr server,
    file_desc * file)
{
    resumeptr r;

    for(r = ResumeHead; NULL != r; r = r->link)
    {
        if(0 == tstricmp(filename, r->filename))
            break;
    }

    if(NULL != r)
    {
        STATUS status;

        if(NULL == (r = al_ptr_alloc_bytes(resumestr *, (sizeof32(resumestr) - sizeof32(r->filename)) + strlen32(filename) + 1, &status)))
        {
            status_assert(status);
            return(NULL);
        }

        r->link = ResumeHead;
        ResumeHead = r;

        (void) strcpy(r->filename, filename);
    }

    r->server = server;
    r->file   = file;

    return(NULL);
}

/*---------------------------------------*
 * Suspend / resume references to a file *
 *---------------------------------------*/

extern os_error *
wind_suspend(
    char * filename)
{
    os_error * e = wind_suspend_allbut(NULL, filename); /* NB: if we're the server, we can't poll the Window Manager here */

    if(NULL != e)
    { /* complain */ /*EMPTY*/ }

    e = wind__suspend(filename); /* do this AFTER suspending the other clients */

    if(NULL != e)
    {
        e = wind_resume(NULL, filename, e); /* NB: if we're the server, we may poll the Window Manager here */

        if(NULL != e)
        { /* complain */ /*EMPTY*/ }
    }

    return(e);
}

/*
 - Suspend file, notifying all clients, possibly via the server.
 - Does not notify the calling client, so that it can be done last.
 */

extern os_error *
wind_suspend_allbut(
    serverptr client,
    char * filename)
{
    file_desc * file = file_find(filename);

    IGNOREPARM(client);

    if(file && file->server)
    {
        os_error * e = wind_setresumefile(file->pathname, file->server, (file_desc *) file->handle);
        if(NULL != e)
        { /* complain */ /*EMPTY*/ }
        return(client_updateserver(file->server, (file_desc *) file->handle, -1));
    }

    return(NULL);
}

/*
 - Suspend file locally, in response to a server request.
 - Does not notify the other clients, but does tell the server when it's finished.
 */

extern os_error *
wind_suspendfile(
    serverptr server,
    file_desc * handle)
{
    file_desc * file = file_findserver(server, handle);
    TCHARZ buffer[BUF_MAX_PATHSTRING];
    os_error * e = NULL;

    e =  rpc_send(server, CLIENT_startupdate, NULL, 0);
    if(NULL != e)
    { /* complain */ /*EMPTY*/ }

    e = rpc_recv(server, CLIENT_startupdate, NULL, 0); /* make sure the server still wants to play ball */
    if(NULL != e)
    { /* complain */ /*EMPTY*/ }

    if(file)
    {
        e = wind_setresumefile(file->pathname, file->server, (file_desc *) file->handle);
        if(NULL == e)
        {
            tstr_xstrkpy(buffer, elemof32(buffer), file->pathname);

            e = wind__suspend(buffer);
        }
    }

    if(!server_complain(rpc_send(server, CLIENT_endupdate, NULL, 0))) /* tell server we've finished */
        server_complain(rpc_senderror(server, CLIENT_endresult, e));  /* return error result */

    return(e);
}

/*
 - Suspend file without regard to other clients.
 - Note that if it fails at any stage, the whole lot must be undone.
 */

extern os_error *
wind__suspend(
    char * filename) /* suspend local files only */
{
    file_desc * file;

    /* for all documents do... so why doesn't it? */
    {
    os_error *e = NULL;
    /* e = layout_suspend(win, filename);  */
    if(NULL != e) return(wind__unsuspend(filename, e));
    } /*block*/

    file = file_find(filename); /* if file has not gone away, we're in trouble! */

    if(file /* && file->refcount > rpc_clientrefs(file) */)
        /* couldn't suspend the file as a remap was going on */
        return(wind__unsuspend(filename, make_oserror(1, "BadSus2", filename)));

    return(NULL);
}

extern os_error *
wind__unsuspend(
    char * filename,
    os_error * e)
{
    /* for all my documents do... layout_resume() */

    /* does nothing if not suspended */

    return(wind_loseresumefile(filename, e));
}

/*
 - Resume file, notifying all clients, possibly via the server.
 - Error on entry => the file was not saved successfully, so don't mark it unmodified.
 */

extern os_error *
wind_resume(
    winptr win,
    char * filename,
    os_error * e)
{
    serverptr server;
    file_desc * file;

    if(wind_getresumefile(filename, &server, &file) && server)
        rpc_complain(client_updateserver(server, file, -2)); /* doesn't resume us */
        /* We can't be a server, so no need to update our clients */

    return(wind__resume(win, filename, e));
}

/*
 - Client to resume file in response to a server request.
 - Doesn't notify the other clients, but does tell the server when it's finished.
 */

extern os_error *
wind_resumefile(
    serverptr server,
    file_desc * file)
{
    char * filename = wind_getresumefilename(server, file);  /* must have been suspended earlier */
    os_error *e = (filename ? wind__resume(NULL, filename, NULL) : NULL);

    e = rpc_send(server, CLIENT_endupdate, NULL, 0); /* tell server we've finished */
    if(NULL != e)
    { /* complain */ /*EMPTY*/ }

    return(rpc_senderror(server, CLIENT_endresult, e)); /* return error result */
}

/*
 - Resume file locally, without regard to any clients/servers etc.
 */

extern os_error *
wind__resume(
    winptr win,
    char * filename,
    os_error * origerr)
{
    IGNOREPARM(win);

    /* For all my documents do... layout_resume() */

    return(wind_loseresumefile(filename, origerr));
}

/*
internal functions
*/

_Check_return_
static STATUS
dplib_remap_fields(
    P_OPENDB p_opendb_src,
    P_OPENDB p_opendb_dst,
    P_ARRAY_HANDLE p_handle);

_Check_return_
extern STATUS
dplib_record_delete(
    P_OPENDB p_opendb)
{
    /* delete card at cursor */
    _kernel_oserror * e = cursor_delete(p_opendb->table.private_dp_cursor, rec_Flush);
    return(e ? rec_oserror_set(e) : STATUS_OK);
}

/* EDITING STUFF */

_Check_return_
static STATUS
dplib_write_record(
    P_OPENDB p_opendb,
    _InVal_     BOOL edit_handles);

_Check_return_
static STATUS
dplib_notediting(
    P_OPENDB p_opendb,
    _InVal_     BOOL addingQ);

_Check_return_
static STATUS
dplib_check(
    P_OPENDB p_opendb);

/* The clear flag should be true for a new, blank record or FALSE for a copy of and extant record */

_Check_return_
extern STATUS
dplib_record_add_commence(
    P_OPENDB p_opendb,
    _InVal_     BOOL clear)
{
    STATUS status = STATUS_FAIL;
    fieldsptr fields = (fieldsptr) p_opendb->table.h_fields;

    fields_newsurrogate(fields, (int *)&p_opendb->recordspec.dplib_surrogate_key);

    if(clear)
    {
        /* also clears the surrogate key */
        if(NULL == fields_blank(fields))
        {
            fields_setsurrogate(fields, (int) p_opendb->recordspec.dplib_surrogate_key);

            status = STATUS_OK;
        }
    }
    else
        /* Allow formulae involving surrogate to change */
        status = dplib_write_record(p_opendb, FALSE);

    return(status);
}

/* To Start Editing */

_Check_return_
extern STATUS
dplib_record_edit_commence(
    P_OPENDB p_opendb)
{
    p_opendb->recordspec.dplib_surrogate_key = fields_getsurrogate((fieldsptr) p_opendb->table.h_fields); /* surrogate key for this card */

    status_return(dplib_check_access_function(&p_opendb->table, ACCESS_FUNCTION_EDIT));

    if(fields_readonly(p_opendb->table.private_dp_layout))
    { /* can't edit read only ... */
        myassert0(TEXT("Attempt to edit_commence on read only data"));
        return(create_error(REC_ERR_NO_ACCESS_FOR_ITEM));
    }

    /* Get a cursor with lock_Update or lock_Shared (if readonly) */
    return(rec_oserror_set(cursor_lock(p_opendb->table.private_dp_cursor, lock_Update)));
}

/* To Cancel an edit just call dplib_notediting() */

_Check_return_
extern STATUS
dplib_record_edit_cancel(
    P_OPENDB p_opendb,
    _InVal_     BOOL adding)
{
    return(dplib_notediting(p_opendb, adding));
}

/* To Confirm an edit */

_Check_return_
extern STATUS
dplib_record_edit_confirm(
    P_OPENDB p_opendb,
    _In_        BOOL blank,
    _InVal_     BOOL adding)
{
    STATUS status;

    if(p_opendb->recordspec.recno < 0)
        blank = TRUE;  /* Es macht geblankken - Keine rekorden  */

    /* Read back the original record: */
    if(blank)
    { /* if we were adding a new (blank) one */
        fields_blank((fieldsptr)p_opendb->table.h_fields);
    }
    else
    { /* if we were editing an existing record */
        recordptr r = cursor_record(p_opendb->table.private_dp_cursor);  /* locate the record which is in the cursor...*/
        myassert0x(r != NULL, TEXT("cursor_record returned NULL in edit_confirm"));
        status_return(rec_oserror_set(cursor_getrecord(p_opendb->table.private_dp_cursor, r)));
    }

    /* Call frames_write_record() to copy the frame contents into the fields (see below)  */
    status = dplib_write_record(p_opendb, !adding);

    if(status_ok(status))
        status = dplib_record_edit_finished(p_opendb, adding);
    else
    {
        myassert0(TEXT("write_record failed in edit_confirm"));
        dplib_notediting(p_opendb, adding);
    }

    return(status);
}

_Check_return_
extern STATUS
dplib_record_edit_finished(
    P_OPENDB p_opendb,
    _InVal_     BOOL adding)
{
    STATUS status = STATUS_FAIL;

    /* Call frames_check() to ensure that the record is valid (see below) */

    status = dplib_check(p_opendb);

    if(status_fail(status))
    {
        myassert0(TEXT("dplib_check failed in dplib_record_edit_finished"));

        status = create_error(REC_ERR_CHECK_FAILED);
    }
    else
    {
        /* Save the record. SKS 26mar96 adds some error checking... */
        if(adding)
        { /*  Adding: */
            status = rec_oserror_set(cursor_insert(p_opendb->table.private_dp_cursor, (recflags) (rec_Flush | rec_CheckLock)));

            myassert0x(status_ok(status), TEXT("cursor_insert failed in edit_finished while adding "));

            p_opendb->recordspec.dplib_surrogate_key |= SURROGATE_SAVED; /* we've added it now */
        }
        else
        { /* Editing: */ /* Note: in future this may change */
            status = rec_oserror_set(cursor_update(p_opendb->table.private_dp_cursor, rec_Flush));

            myassert0x(status_ok(status), TEXT("cursor_update failed in edit_finished"));
        }
    }

    /* Call layout_notediting()  - see below  */

    dplib_notediting(p_opendb, adding);

    /* To 'follow' the record to its new position:  direction 1 => forwards */
    if(status_ok(status))
    {
        if(NULL != cursor_findcursor(p_opendb->table.private_dp_cursor, p_opendb->table.private_dp_cursor, 1))
        {
            myassert0(TEXT("cursor_findcursor failed in edit_finished"));

            status = STATUS_FAIL;
        }
    }

    return(status);
}

/*
Copy all the records IN THE CURRENT QUERY (16 April 95 PMF) from one database to the other subject to the remapping array given
*/

_Check_return_
extern STATUS
dplib_remap_records(
    P_OPENDB p_opendb_src,
    P_OPENDB p_opendb_dst,
    P_ARRAY_HANDLE p_handle,
    P_REMAP_OPTIONS p_options)
{
    STATUS status = STATUS_OK;
    _kernel_oserror * e;
    fieldsptr p_newfields = (fieldsptr) p_opendb_dst->table.h_fields;
    fieldsptr p_oldfields = (fieldsptr) p_opendb_src->table.h_fields;
    cursor crsr1;
    cursor crsr2;
    P_ANY p_query_query = NULL /*no query*/;

    /* Ensure we have a cursor */
    if(p_opendb_src->search.query_id != RECORDZ_WHOLE_FILE)
    {
        P_QUERY p_query = p_query_from_p_opendb(p_opendb_src, p_opendb_src->search.query_id);
        PTR_ASSERT(p_query);
        p_query_query = p_query->p_query;
    }

    status_return(rec_oserror_set(cursor_open(p_oldfields, p_query_query, lock_Shared, &crsr1)));

    e = cursor_open(p_newfields, NULL, lock_Update, &crsr2); /* Possible that this NULL trashed the subset */

    if(NULL == e)
    {
        cardnum cardno;
        int ncards;
        int lastvalid;

#if 0
        /* What the F*** is this says PMF ? */
        e = cursor_writefields(crsr2, crsr1);
#endif

        for(cardno = card_First; ; cardno = card_Next)
        {
            int new_surrogate_key;
            recflags insert_order = (recflags) 0;

            e = cursor_get(crsr1, &cardno, &ncards, &lastvalid, lock_View, NULL);
            if(NULL != e)
                break;

            if(card_Finished == cardno)
                break;

            e = fields_newsurrogate(p_newfields, &new_surrogate_key);
            if(NULL != e)
                break;

            fields_blank(p_newfields);

            if(status_fail(status = dplib_remap_fields(p_opendb_src, p_opendb_dst, p_handle)))
            { /*EMPTY*/ } /* That record is lost or damaged */

            fields_setsurrogate(p_newfields, new_surrogate_key);

            if((NULL == p_options) || (p_options->insert_order == REMAP_OPT_INS_SEQ))
                insert_order = rec_Sequential;

            e = cursor_insert(crsr2, insert_order);
            if(NULL != e)
                break;
        }

        e = cursor_flush(crsr2, e);
        e = cursor_close(crsr2, e);
    }

    e = cursor_close(crsr1, e);

    return(e ? rec_oserror_set(e) : status);
}

_Check_return_
static STATUS
dplib_datacopy_single_field(
    P_FIELDDEF p_fielddef_src,
    P_FIELDDEF p_fielddef_dst)
{
     STATUS status = STATUS_FAIL;
     field_value value;

    if(NULL == p_fielddef_src->p_field)
        /* Its supposed to make a blank one */
        return(STATUS_OK);

    if(NULL == p_fielddef_dst->p_field)
        /* The field does not exist in the output */
        return(STATUS_OK);

    if(field_getvalue(p_fielddef_src->p_field, &value))
        return(STATUS_FAIL);

    if(fields_convert(field_type(p_fielddef_src->p_field), field_type(p_fielddef_dst->p_field), &value))
    {
        field_setvalue(p_fielddef_dst->p_field, value);
        return(STATUS_OK);
    }

    { /* get text */
    os_filetype filetype;
    char ** anchor;
    int offset;
    int size;
    _kernel_oserror * e = field_getvalue_text(p_fielddef_src->p_field, NULL, &filetype, &anchor, &offset, &size);

    if(NULL == e)
    {
        /* set text to the other */
        e = field_setvalue_text(p_fielddef_dst->p_field, filetype, anchor, offset, size);

#if RECORDZ_BAD_FIELD_TO_NULL
        if(NULL != e)
        {
            /* This is probably because you can't make a number out of this text or similar stupidity */
            if(size > 0)
            {
                switch(p_fielddef_dst->type)
                {
                case FIELD_INTEGER:
                case FIELD_REAL:
                    {
                    P_U8 p_u8 = (*anchor)+offset;
                    *p_u8++ = CH_DIGIT_ZERO;  /* The digit zero */
                    if(size > 1)
                    {
                        *p_u8++ = CH_NULL;  /* a null terminator */
                        size = 2;
                    }
                    break;
                    }

                default:
                    size = 0;  /* Just throw the least offensive thing you can   ie NULL */
                    break;
                }
            }

            e = field_setvalue_text(p_fielddef_dst->p_field, filetype, anchor, offset, size);
        }
#endif
    }

    if(NULL != e)
        status = rec_oserror_set(e);
    else
        status = STATUS_OK;
    } /*block*/

    return(status);
}

/* We will be returning to the code right after these messages

Yes, the 'duplicate record' error means that you are trying to insert a
record which has the same key as an existing record.

Note that if you are re-merging records back into the a new (empty) version
of the original database, eg. because a new field has been added, then the
output surrogate key values should be copied from the original records, in
other words the field copying procedure should surrogate field should copy
the surrogate field across as well as the others.

However, if you are inserting the records into a different database, then
the surrogate field values must be new versions, since otherwise it is quite
possible (and likely) that the new values (copied from the source records)
will clash with those of existing records in the destination file.

In this case you should use fields_newsurrogate() to read the next surrogate
value to use, and fields_setsurrogate to put it into the fields themselves.

DataPower does it by putting NULL_INTEGER into the surrogate key at first,
then copying the fields across, and then calling fields_newsurrogate() to
reset the value if it is still NULL_INTEGER (ie. has not been copied).

*/

_Check_return_
static STATUS
dplib_remap_fields(
    P_OPENDB p_opendb_src,
    P_OPENDB p_opendb_dst,
    P_ARRAY_HANDLE p_handle)
{
    STATUS status = STATUS_OK;
    ARRAY_INDEX i;
    ARRAY_INDEX j;

    /* for all elements in the mapping array do */
    i = 0;

    for(j = 0; j < array_elements(p_handle); j++)
    {
        P_REMAP_ENTRY p_remap = array_ptr(p_handle, REMAP_ENTRY, j);

        if(p_remap->p_table)
        {
            FIELDDEF fielddef_src;
            FIELDDEF fielddef_dst = *array_ptrc(&p_opendb_dst->table.h_fielddefs, FIELDDEF, i);

            /* Presume the field is non extant */
            fielddef_src.id = 0;
            fielddef_src.p_field = NULL;

            if(&p_opendb_src->table == p_remap->p_table)
            {
                S32 f = fieldnumber_from_field_id(&p_opendb_src->table, p_remap->field_id);

                if(f > 0)
                    /* No, we've found it in the source database */
                    fielddef_src = *array_ptrc(&p_opendb_src->table.h_fielddefs, FIELDDEF, (f-1));
            }

            status = dplib_datacopy_single_field(&fielddef_src, &fielddef_dst);

            if(status_fail(status) && (STATUS_FAIL != status))
                break;

            i++; /* If not blank step dst field number on */
        }
    }

    if(status_ok(status)) /* SKS for 1.30/01 well done lint */
        kick_server(p_opendb_dst);

    return(status);
}

/* Traverse the fields writing them back to a record */

_Check_return_
static STATUS
dplib_write_record(
    P_OPENDB p_opendb,
    _InVal_     BOOL edit_handles)
{
    STATUS status = STATUS_OK;
    ARRAY_INDEX i;

    /* put back the original surrogate */
    fields_setsurrogate(frames_fields((framesptr) p_opendb), (int) (p_opendb->recordspec.dplib_surrogate_key & ~SURROGATE_SAVED));

    /* for all fields do... */
    for(i = 0; i < array_elements(&p_opendb->table.h_fielddefs); i++)
    {
        P_FIELDDEF p_fielddef = array_ptr(&p_opendb->table.h_fielddefs, FIELDDEF, i);
        os_filetype filetype;
        char ** anchor;
        int offset;
        int size;
        BOOL doflag = TRUE;

        if(edit_handles)
        {
            P_EDITREC p_editrec = array_ptr(&p_opendb->recordspec.h_editrecs, EDITREC, i);

            filetype = (os_filetype) p_editrec->t5_filetype;
            anchor   = (char **) &p_editrec->p_u8;
            offset   = 0;
            size     = (int) p_editrec->length;
        }
        else
            doflag = !field_getvalue_text(p_fielddef->p_field, NULL, &filetype, &anchor, &offset, &size);

        if(doflag && (p_fielddef->type != FIELD_FORMULA))
        {
            /* This give the record of record-1's fields ??? the fields_blank() has failed/not been called ??? */
            if(*anchor) /* Watch out for null => no change */
            {
                status = rec_oserror_set(field_setvalue_text(p_fielddef->p_field, filetype, anchor, offset, size));
                myassert0x(status_ok(status), TEXT("field_setvalue_text failed in dplib_write_record"));
            }
        }
    }

    if(status_ok(status))
        kick_server(p_opendb);

    return(status);
}

/* Tidy up after cancelling/saving a record */

_Check_return_
static STATUS
dplib_notediting(
    P_OPENDB p_opendb,
    _InVal_     BOOL addingQ)
{
    STATUS status = STATUS_OK;
    _kernel_oserror * e = cursor_lock(p_opendb->table.private_dp_cursor, lock_View); /* release the lock */

    if(NULL != e)
        status = rec_oserror_set(e);

    /* finished editing the card */
    /*  layout->editing = FALSE; */
    if(addingQ)
    {
        status = STATUS_FAIL;

        if(0 == (p_opendb->recordspec.dplib_surrogate_key & SURROGATE_SAVED))
            if(NULL == fields_oldsurrogate((fieldsptr) p_opendb->table.h_fields, (int) p_opendb->recordspec.dplib_surrogate_key))
            {
                p_opendb->recordspec.dplib_surrogate_key |= SURROGATE_SAVED;
                status = STATUS_OK;
            }
    }

    if(status_ok(status))
        kick_server(p_opendb);

    /* remove caret from the record and mark unmodified */

    return(status);
}

/*
 - Check for errors in the visible part of the record.
 - Then do a more extensive check which includes the non-layout fields.
 - This also sets up sorted formula fields.
 */

_Check_return_
static STATUS
dplib_check(
    P_OPENDB p_opendb)
{
    STATUS status = STATUS_OK;
    ARRAY_INDEX i;

    for(i = 0; i < array_elements(&p_opendb->table.h_fielddefs); i++)
    {
        P_FIELDDEF p_fielddef = array_ptr(&p_opendb->table.h_fielddefs, FIELDDEF, i);

        if(field_check(p_fielddef->p_field))
            if(p_fielddef->type != FIELD_FORMULA)
            {
                status = STATUS_FAIL;
                break;
            }
    }

    if(status_ok(status))
        if(fields_setvalues_sort((fieldsptr) p_opendb->table.h_fields))
            status = STATUS_FAIL;

    return(status);
}

/* end of dblow.c */
