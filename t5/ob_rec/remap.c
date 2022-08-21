/* remap.c */

/* Copyright (C) 1994-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Build remap control structures for database reorganisation */

#include "common/gflags.h"

#include "ob_rec/ob_rec.h"

SC_ARRAY_INIT_BLOCK array_init_block_remap = aib_init(1, sizeof32(REMAP_ENTRY), FALSE);

static void
remap_entry_init(
    P_REMAP_ENTRY p_remap_entry,
    P_TABLEDEF p_table,
    _InRef_     PC_FIELDDEF p_fielddef)
{
    S32 sort_order;

    switch(p_fielddef->keyorder)
    {
    default: sort_order = SORT_NULL; break;
    case +1: sort_order = SORT_AZ;   break;
    case -1: sort_order = SORT_ZA;   break;
    }

    p_remap_entry->p_table      = p_table;
    p_remap_entry->field_id     = p_fielddef->id;
    p_remap_entry->sort_order   = sort_order;
    p_remap_entry->new_field_id = p_fielddef->id;
}

/*
add remap entries for given table to remap array
*/

_Check_return_
static STATUS
remap_entries_add(
    P_TABLEDEF p_table,
    /*inout*/ P_ARRAY_HANDLE p_handle)
{
    STATUS status = STATUS_OK;
    P_REMAP_ENTRY p_remap_entry;
    ARRAY_INDEX i;

    if(NULL == (p_remap_entry = al_array_extend_by(p_handle, REMAP_ENTRY, array_elements32(&p_table->h_fielddefs), &array_init_block_remap, &status)))
        return(status);

    /* Create the default AAAAA,BBBBB 1::1 mapping */

    /* Set up entries for table's fields */
    for(i = 0; i < array_elements(&p_table->h_fielddefs); ++i, ++p_remap_entry)
        remap_entry_init(p_remap_entry, p_table, array_ptrc(&p_table->h_fielddefs, FIELDDEF, i));

    return(status);
}

_Check_return_
static STATUS
remap_from_new_field_id(
    P_TABLEDEF p_table,
    P_ARRAY_HANDLE p_handle_in,
    P_ARRAY_HANDLE p_handle_out)
{
    STATUS status = STATUS_OK;
    P_REMAP_ENTRY p_remap_in = array_base(p_handle_in, REMAP_ENTRY);
    P_REMAP_ENTRY p_remap_out;
    ARRAY_INDEX j;

    if(NULL == (p_remap_out = al_array_extend_by(p_handle_out, REMAP_ENTRY, array_elements32(&p_table->h_fielddefs), &array_init_block_remap, &status)))
        return(status);

    /* Set up the remapping */
    for(j = 0; j < array_elements(&p_table->h_fielddefs); j++)
    {
        PC_FIELDDEF p_fielddef = array_ptrc(&p_table->h_fielddefs, FIELDDEF, j);
        ARRAY_INDEX i;

        p_remap_out[j] = p_remap_in[j]; /* A quick way to initialise it */

        /* Find the remap entry containing fielddef.id in its new id slot */
        for(i = 0; i < array_elements(&p_table->h_fielddefs); i++)
        {
            if(p_remap_in[i].new_field_id == p_fielddef->id)
            {
                /* output the remap entry (p_table, field_id) into the jth slot of a new remap table */
                p_remap_out[j] = p_remap_in[i];
                break;
            }
        }
    }

    return(status);
}

/*
HIGH LEVEL FUNCTIONS - These expect to "do" the entire operation including the data copying
*/

/*
Given an opendb and a substitute table build a remap array to create a database in the substitute structure
Then remap the data from the original database into it using the field-id ordering of the substitute table

The hidden status which exists per field is NOT stored in the datapower file therefore it gets lost in this process!

This also allows addtion & deletion of fields complete with properties
*/

_Check_return_
extern STATUS
remap_remap(
    P_OPENDB p_opendb,
    P_TABLEDEF p_table,
    _In_z_      PCTSTR filename)
{
    STATUS status;
    ARRAY_HANDLE handle = 0;

    for(;;) /* loop for structure */
    {
        ARRAY_INDEX i;

        /* build a remap array according to the substitute table structure */
        status_break(status = remap_entries_add(p_table, &handle));

        /* create the file with the new structure */
        status_break(status = remap_database_structure(p_opendb, filename, &handle));

        /* Make the remapping array refer to the table in the source database */
        for(i = 0; i < array_elements(&handle); i++)
        {
            P_REMAP_ENTRY p_remap_entry = array_ptr(&handle, REMAP_ENTRY, i);

            p_remap_entry->p_table = &p_opendb->table;
        }

        /* Do the data copy operation */
        status = remap_records(p_opendb, filename, &handle, NULL); /* transmogrify the data */

        break;
        /*NOTREACHED*/
    }

    al_array_dispose(&handle);

    return(status);
}

/*
Given 2 databases of the similar structure append records from one
*/

_Check_return_
extern STATUS
remap_similar(
    P_OPENDB p_opendb_1,
    P_OPENDB p_opendb_2,
    _In_z_      PCTSTR filename)
{
    STATUS status;
    ARRAY_HANDLE handle1 = 0;
    ARRAY_HANDLE handle2 = 0;

    for(;;) /* loop for structure */
    {
        status_break(status = remap_sim(&p_opendb_1->table, &p_opendb_2->table, &handle1, &handle2));

        /* create the file with the new structure using any layouts from the first */
        status_break(status = remap_database_structure(p_opendb_1, filename, &handle1));

        status_break(status = remap_records(p_opendb_1, filename, &handle1, NULL)); /* transmogrify the data */

        status = remap_records(p_opendb_2, filename, &handle2, NULL); /* transmogrify the data again */

        break;
        /*NOTREACHED*/
    }

    al_array_dispose(&handle1);
    al_array_dispose(&handle2);

    return(status);
}

/*
Given 2 databases of the same structure append records from one
*/

_Check_return_
extern STATUS
remap_append(
    P_OPENDB p_opendb_1,
    P_OPENDB p_opendb_2,
    _In_z_      PCTSTR filename)
{
    STATUS status;
    ARRAY_HANDLE handle = 0;

    for(;;)
    {
        /* build the default null remap array */
        status = remap_entries_add(&p_opendb_1->table, &handle);

        /* create the file with the new structure using any layouts from the first  */
        status_break(status = remap_database_structure(p_opendb_1, filename, &handle));

        status_break(status = remap_records(p_opendb_1, filename, &handle, NULL)); /* transmogrify the data */

        al_array_dispose(&handle);

        /* build the default null remap array for the second database */
        status_break(status = remap_entries_add(&p_opendb_2->table, &handle));

        status = remap_records(p_opendb_2, filename, &handle, NULL); /* transmogrify the data again */

        break;
        /*NOTREACHED*/
    }

    al_array_dispose(&handle);

    return(status);
}

/*
Perform the sort operation on a database by reconstructing it with the fields in sort-key order
The remap array is passed in!
*/

_Check_return_
extern STATUS
remap_sort(
    P_OPENDB p_opendb,
    P_ARRAY_HANDLE p_remap_array_handle,
    _In_z_      PCTSTR filename)
{
    STATUS status;

    /* It so happens that the result of this in datapower is that the fields get reordered in the database */
    status = remap_database_structure(p_opendb, filename, p_remap_array_handle); /* create the file with the new structure */

    /* reconstruct the remap due to the fact that sorting changes the order of the fields in the output file */
    if(status_ok(status))
    {
        ARRAY_HANDLE newhandle = 0;
        OPENDB opendb_2;

        zero_struct(opendb_2);
        status = open_database(&opendb_2, filename);
        status = remap_from_new_field_id(&opendb_2.table, p_remap_array_handle, &newhandle);
        status = close_database(&opendb_2, status);

        al_array_dispose(p_remap_array_handle);

        *p_remap_array_handle = newhandle;
    }

    /* Now actually move the data */
    if(status_ok(status))
    {
        REMAP_OPTIONS remap_options;
        /* Force the remapping to non-sequential for better file size & speed - Ask Neil */
        remap_options.insert_order = REMAP_OPT_INS_NUL;
        status = remap_records(p_opendb, filename, p_remap_array_handle, &remap_options);
    }

    return(status);
}

/*
REMAP-MAKE FUNCTIONS
*/

/*
Given two table structures compute a two remappings such that similar fields can be combined.

Each resulting remapping will have the same number of entries, which will be at most the sum of
the number of fields in the two input tables. If any combinable fields exist it will be less.

The first remapping table will contain entries for fields in the first table plus additional fields from the second.

The second remapping table will contain entries for fields in the second table plus additional fields from the first.

Thus given ABCDEF and ABSTUCG we find that the field ABC are in both tables but DEF and STUG are only found in one table.

The resulting database then needs ABCDEFSTUG

the first table will end up as

ABCDEFSTUG where the common fields ABC are copied from table1's definitions

and the second

ABCDEFSTUG where the common fields ABC are copied from table2's definitions
*/

_Check_return_
extern STATUS
remap_sim(
    P_TABLEDEF p_table_a,
    P_TABLEDEF p_table_b,
    P_ARRAY_HANDLE p_handle_a,
    P_ARRAY_HANDLE p_handle_b)
{
    STATUS status = STATUS_OK;
    ARRAY_INDEX a, b;

    *p_handle_a = 0;
    *p_handle_b = 0;

    /* for all the fields in table_a, add a remap to handle_a */
    for(a = 0; a < array_elements(&p_table_a->h_fielddefs); a++)
    {
        PC_FIELDDEF p_fielddef_a = array_ptrc(&p_table_a->h_fielddefs, FIELDDEF, a);
        P_REMAP_ENTRY p_remap_entry;
        if(NULL == (p_remap_entry = al_array_extend_by(p_handle_a, REMAP_ENTRY, 1, &array_init_block_remap, &status)))
            break;
        remap_entry_init(p_remap_entry, p_table_a, p_fielddef_a);
    }

    /* scan for non-compatible fields from table_b and add them to handle_a */
    if(status_ok(status))
        for(b = 0; b < array_elements(&p_table_b->h_fielddefs); b++)
        {
            PC_FIELDDEF p_fielddef_b = array_ptrc(&p_table_b->h_fielddefs, FIELDDEF, b);
            BOOL similar = FALSE;

            for(a = 0; a < array_elements(&p_table_a->h_fielddefs); a++)
            {
                PC_FIELDDEF p_fielddef_a = array_ptrc(&p_table_a->h_fielddefs, FIELDDEF, a);

                if(p_fielddef_a->type != p_fielddef_b->type)
                    continue;

                if(0 != tstricmp(p_fielddef_a->name, p_fielddef_b->name))
                    continue;

                similar = TRUE;
                break;
            }

            if(!similar)
            {
                P_REMAP_ENTRY p_remap_entry;
                if(NULL == (p_remap_entry = al_array_extend_by(p_handle_a, REMAP_ENTRY, 1, &array_init_block_remap, &status)))
                    break;
                remap_entry_init(p_remap_entry, p_table_b, p_fielddef_b);
            }
        }

    /* scan fields from table_a outputting to handle_b - if there is a similar field from table_b add that otherwise add from table_a */
    if(status_ok(status))
        for(a = 0; a < array_elements(&p_table_a->h_fielddefs); a++)
        {
            PC_FIELDDEF p_fielddef_a = array_ptrc(&p_table_a->h_fielddefs, FIELDDEF, a);
            BOOL similar = FALSE;

            for(b = 0; b < array_elements(&p_table_b->h_fielddefs); b++)
            {
                PC_FIELDDEF p_fielddef_b = array_ptrc(&p_table_b->h_fielddefs, FIELDDEF, b);

                if(p_fielddef_a->type != p_fielddef_b->type)
                    continue;

                if(0 != tstricmp(p_fielddef_a->name, p_fielddef_b->name))
                    continue;

                similar = TRUE;
                break;
            }

            if(similar)
            {
                PC_FIELDDEF p_fielddef_b = array_ptrc(&p_table_b->h_fielddefs, FIELDDEF, b);
                P_REMAP_ENTRY p_remap_entry;
                if(NULL == (p_remap_entry = al_array_extend_by(p_handle_b, REMAP_ENTRY, 1, &array_init_block_remap, &status)))
                    break;
                remap_entry_init(p_remap_entry, p_table_b, p_fielddef_b);
            }
            else
            {
                P_REMAP_ENTRY p_remap_entry;
                if(NULL == (p_remap_entry = al_array_extend_by(p_handle_b, REMAP_ENTRY, 1, &array_init_block_remap, &status)))
                    break;
                remap_entry_init(p_remap_entry, p_table_a, p_fielddef_a);
            }
        }

    /* scan for non-compatible fields from table_b and add them to handle_b */
    if(status_ok(status))
        for(b = 0; b < array_elements(&p_table_b->h_fielddefs); b++)
        {
            PC_FIELDDEF p_fielddef_b = array_ptrc(&p_table_b->h_fielddefs, FIELDDEF, b);
            BOOL similar = FALSE;

            for(a = 0; a < array_elements(&p_table_a->h_fielddefs); a++)
            {
                PC_FIELDDEF p_fielddef_a = array_ptrc(&p_table_a->h_fielddefs, FIELDDEF, a);

                if(p_fielddef_a->type != p_fielddef_b->type)
                    continue;

                if(0 != tstricmp(p_fielddef_a->name, p_fielddef_b->name))
                    continue;

                similar = TRUE;
                break;
            }

            if(!similar)
            {
                P_REMAP_ENTRY p_remap_entry;
                if(NULL == (p_remap_entry = al_array_extend_by(p_handle_b, REMAP_ENTRY, 1, &array_init_block_remap, &status)))
                    break;
                remap_entry_init(p_remap_entry, p_table_b, p_fielddef_b);
            }
        }

    /* There that was easy wasn't it */
    return(status);
}

_Check_return_
extern STATUS
remap_hidden_to_end(
    P_TABLEDEF p_table,
    P_ARRAY_HANDLE p_handle)
{
    STATUS status = STATUS_OK;
    S32 i;
    BOOL hidden;

    *p_handle = 0;

    if(NULL == al_array_extend_by(p_handle, REMAP_ENTRY, array_elements32(&p_table->h_fielddefs), &array_init_block_remap, &status))
        return(status);

    /* Set up the remapping */
    i = 0;

    /* add all the not-hidden ones first, then the hidden ones */
    for(hidden = 0; hidden <= 1; ++hidden)
    {
        ARRAY_INDEX j;

        for(j = 0; j < array_elements(&p_table->h_fielddefs); j++)
        {
            PC_FIELDDEF p_fielddef = array_ptrc(&p_table->h_fielddefs, FIELDDEF, j);

            if(hidden == p_fielddef->hidden)
            {
                remap_entry_init(array_ptr(p_handle, REMAP_ENTRY, i), p_table, p_fielddef);
                i++;
            }
        }
    }

    return(status);
}

/*
Copy the Fireworkz specific attributes of all fields from one table to another
Use field-id bindings...
*/

_Check_return_
extern STATUS
remap_copy_fireworkz_attributes(
    P_TABLEDEF p_table_dst,
    P_TABLEDEF p_table_src)
{
    ARRAY_INDEX i;

    for(i = 0; i < array_elements(&p_table_src->h_fielddefs); i++)
    {
        PC_FIELDDEF p_fielddef_src = array_ptrc(&p_table_src->h_fielddefs, FIELDDEF, i);
        P_FIELDDEF p_fielddef_dst = p_fielddef_from_field_id(&p_table_dst->h_fielddefs, p_fielddef_src->id);

        if(P_DATA_NONE != p_fielddef_dst)
            /* at present the only attribute we are interested in is the hidden bit */
            p_fielddef_dst->hidden = p_fielddef_src->hidden;
    }

    return(STATUS_OK);
}

/* end of remap.c */
