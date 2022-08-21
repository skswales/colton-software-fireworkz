/* dbquery.c */

/* Copyright (C) 1994-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#include "common/gflags.h"

#include "ob_rec/ob_rec.h"

_Check_return_
extern STATUS
open_query(
    P_OPENDB p_opendb,
    QUERY_ID query_id)
{
#if DPLIB
    return(dplib_create_query(p_opendb, query_id));
#else
    return(create_error(ERR_NYI));
#endif
}

_Check_return_
extern STATUS
close_query(
    P_OPENDB p_opendb,
    P_QUERY p_query)
{
    /* Shut it down, dispose of all the search_pattern */
    STATUS status;
    ARRAY_INDEX i = p_query - array_ptr(&p_opendb->search.h_query, QUERY, 0);

    {
    ARRAY_INDEX j = array_elements(&p_query->h_search_pattern);

    while(--j >= 0)
    {
        P_SEARCH_FIELD_PATTERN p_search_pattern = array_ptr(&p_query->h_search_pattern, SEARCH_FIELD_PATTERN, j);

        al_array_dispose(&p_search_pattern->h_text_ustr);
    }
    } /*block*/

    al_array_dispose(&p_query->h_search_pattern);
    al_array_dispose(&p_query->h_name_ustr);

#if DPLIB
    status = dplib_remove_query(p_query, TRUE);
#else
    status = STATUS_OK;
#endif

    al_array_delete_at(&p_opendb->search.h_query, -1, i); /* SKS 22nov95 remove effectively */

    if(!array_elements(&p_opendb->search.h_query))
        al_array_dispose(&p_opendb->search.h_query);

    return(status);
}

extern void
wipe_query(
    P_QUERY p_query)
{
    p_query->p_expr    = NULL;
    p_query->p_qfields = NULL;
    p_query->p_query   = NULL;

    p_query->h_name_ustr = 0;

    p_query->h_search_pattern = 0;
    p_query->search_type = 0;
    p_query->search_andor = 0;
    p_query->search_exclude = 0;

    p_query->query_id = (QUERY_ID)-1; /* Mark as free */
}

/* locate a query out of the array of them, using its id */

_Check_return_
_Ret_maybenull_
extern P_QUERY
p_query_from_p_opendb(
    P_OPENDB p_opendb,
    QUERY_ID query_id)
{
    ARRAY_INDEX i = array_elements(&p_opendb->search.h_query);

    while(--i >= 0)
    {
        P_QUERY p_query = array_ptr(&p_opendb->search.h_query, QUERY, i);

        if(p_query->query_id == query_id)
            return(p_query);
    }

    return(NULL);
}

/* locate a query out of the array of them, using its id */

_Check_return_
_Ret_maybenull_
static P_QUERY
p_query_from_p_opendb_by_parent(
    P_OPENDB p_opendb,
    QUERY_ID parent_id)
{
    ARRAY_INDEX i = array_elements(&p_opendb->search.h_query);

    while(--i >= 0)
    {
        P_QUERY p_query = array_ptr(&p_opendb->search.h_query, QUERY, i);

        if(p_query->parent_id == parent_id)
            return(p_query);
    }

    return(NULL);
}

/* generate a new query id */

extern QUERY_ID
unique_query_id(
    P_ARRAY_HANDLE p_h_query)
{
    S32 trial_id = 0;
    ARRAY_INDEX i = 0;

    while(i < array_elements(p_h_query))
    {
        P_QUERY p_query = array_ptr(p_h_query, QUERY, i);

        if(p_query->query_id == (QUERY_ID) trial_id)
        {
            trial_id++;
            i = 0; /* Start again ! */
            continue;
        }

        i++;
    }

    return((QUERY_ID) trial_id);
}

/* (used to prevent orphan formation) */

_Check_return_
extern BOOL
has_query_got_offspring(
    P_OPENDB p_opendb,
    QUERY_ID parent_id)
{
    P_QUERY p_query = p_query_from_p_opendb_by_parent(p_opendb, parent_id);

    return(NULL != p_query);
}

_Check_return_
extern STATUS
copy_search_pattern(
    P_ARRAY_HANDLE p_h_target,
    P_ARRAY_HANDLE p_h_source)
{
    ARRAY_INDEX i;

    status_return(al_array_duplicate(p_h_target, p_h_source));

    for(i = 0; i < array_elements(p_h_target); ++i)
        array_ptr(p_h_target, SEARCH_FIELD_PATTERN, i)->h_text_ustr = 0; /* ensure no cockups on failure */

    for(i = 0; i < array_elements(p_h_target); ++i)
    {
        P_SEARCH_FIELD_PATTERN p_source_pattern = array_ptr(p_h_source, SEARCH_FIELD_PATTERN, i);
        P_SEARCH_FIELD_PATTERN p_target_pattern = array_ptr(p_h_target, SEARCH_FIELD_PATTERN, i);

        if(0 == p_source_pattern->h_text_ustr)
            continue;

        status_return(al_array_duplicate(&p_target_pattern->h_text_ustr, &p_source_pattern->h_text_ustr));
    }

    return(STATUS_OK);
}

/* end of dbquery.c */
