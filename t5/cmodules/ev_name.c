/* ev_name.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Manage the list of named resources for the evaluator */

/* MRJC January 1991 / May 1992 */

#include "common/gflags.h"

#include "ob_ss/ob_ss.h"

/*
internal functions
*/

PROC_BSEARCH_PROTO(static, proc_compare_custom_def, EV_CUSTOM, EV_CUSTOM);

PROC_ELEMENT_IS_DELETED_PROTO(static, custom_def_is_deleted);

PROC_BSEARCH_PROTO(static, proc_compare_name_def, EV_NAME, EV_NAME);

PROC_ELEMENT_IS_DELETED_PROTO(static, name_def_is_deleted);

/*
resource lists
*/

DEPTABLE custom_def_deptable;   /* custom function definition table */

DEPTABLE name_def_deptable;     /* name definition table */

static EV_HANDLE next_custom_handle = 0;

static EV_HANDLE next_name_handle = 0;

#define CUSTOM_INC      2

#define NAME_INC        5

/******************************************************************************
*
* set up name definition
*
******************************************************************************/

static void
name_set_def(
    _InoutRef_  P_EV_NAME p_ev_name,
    _InRef_     PC_SS_DATA p_ss_data,
    _InVal_     BOOL undefined)
{
    if(PtrGetByte(p_ev_name->ustr_name_id) == CH_QUESTION_MARK)
    {
        ss_data_set_data_id(&p_ev_name->def_data, DATA_ID_FIELD);
        p_ev_name->def_data.arg.h_name = p_ev_name->handle;
        p_ev_name->flags.undefined = 0;
    }
    else
    {
        status_assert(ss_data_resource_copy(&p_ev_name->def_data, p_ss_data));
        p_ev_name->flags.undefined = undefined;
        if(ss_data_is_array(&p_ev_name->def_data))
            data_ensure_constant(&p_ev_name->def_data);
    }

    ev_todo_add_name_dependents(p_ev_name->handle);
}

/******************************************************************************
*
* lookup custom id in custom function definition table
*
* --out--
* <0  custom function not in definition table
* >=0 entry number of custom function
*
******************************************************************************/

_Check_return_
extern ARRAY_INDEX
custom_def_from_handle(
    _InVal_     EV_HANDLE ev_handle)
{
    ARRAY_INDEX res = -1;
    EV_CUSTOM temp;

    custom_list_sort();

    temp.handle = ev_handle;

    if(array_elements(&custom_def_deptable.h_table))
    {
        const PC_EV_CUSTOM p_ev_custom = (PC_EV_CUSTOM)
            bsearch(&temp, array_basec(&custom_def_deptable.h_table, EV_CUSTOM), (U32) custom_def_deptable.sorted, sizeof(EV_CUSTOM), proc_compare_custom_def);

        if(NULL != p_ev_custom)
            res = array_indexof_element(&custom_def_deptable.h_table, EV_CUSTOM, p_ev_custom);
    }

    return(res);
}

/******************************************************************************
*
* sort custom function definition table
*
******************************************************************************/

static inline void
custom_list_sort_checkuse(void)
{
    /* check through custom function definition table to see if the deletion of
     * a custom use has rendered the definition record useless
     */
    const ARRAY_INDEX custom_table_elements = array_elements(&custom_def_deptable.h_table);
    ARRAY_INDEX custom_num;

    for(custom_num = 0; custom_num < custom_table_elements; ++custom_num)
    {
        const P_EV_CUSTOM p_ev_custom = array_ptr(&custom_def_deptable.h_table, EV_CUSTOM, custom_num);

        if(p_ev_custom->flags.to_be_deleted)
            continue;

        /* custom reference must be undefined before we can delete it */
        if(!p_ev_custom->flags.undefined)
            continue;

        if(search_for_custom_use(p_ev_custom->handle) < 0)
        {
            trace_1(TRACE_MODULE_EVAL, TEXT("custom_list_sort deleting undefined custom: %s"), report_ustr(ustr_bptr(p_ev_custom->ustr_custom_id)));

            {
            const P_SS_DOC p_ss_doc = ev_p_ss_doc_from_docno(ev_slr_docno(&p_ev_custom->owner));

            if(P_SS_DOC_NONE != p_ss_doc)
                p_ss_doc->custom_ref_count -= 1;
            } /*block*/

            p_ev_custom->flags.to_be_deleted = 1;
            custom_def_deptable.flags.to_be_deleted = 1;
            custom_def_deptable.mindel = MIN(custom_def_deptable.mindel, custom_num);
        }
    }

    custom_def_deptable.flags.checkuse = 0;
}

extern void
custom_list_sort(void)
{
    if(custom_def_deptable.flags.checkuse /* && !custom_def_deptable.flags.delhold */) /* SKS -delhold 08aug2015 as unused in Fz */
        custom_list_sort_checkuse();

    consume_bool(tree_sort(&custom_def_deptable, custom_def_is_deleted, proc_compare_custom_def));

    trace_2(TRACE_APP_MEMORY_USE,
            TEXT("custom definition table is: ") U32_TFMT TEXT(" elements") U32_TFMT TEXT(" bytes"),
            array_elements32(&custom_def_deptable.h_table), array_size32(&custom_def_deptable.h_table) * array_element_size32(&custom_def_deptable.h_table));
}

/******************************************************************************
*
* given a document and a custom,
* ensure this [document]custom
* combination exists in the resource table
*
* --out--
* <0  new custom couldn't be inserted
* >=0 entry number of name
*
******************************************************************************/

_Check_return_
extern ARRAY_INDEX
ensure_custom_in_list(
    _InVal_     EV_DOCNO owner_ev_docno,
    _In_z_      PC_USTR ustr_custom_name)
{
    STATUS status;
    EV_HANDLE h_custom;
    P_EV_CUSTOM p_ev_custom;
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(CUSTOM_INC, sizeof32(EV_CUSTOM), TRUE);
    P_EV_CUSTOM_ARGS p_ev_custom_args;

    if((h_custom = find_custom_in_list(owner_ev_docno, ustr_custom_name)) >= 0)
        return(h_custom);

    /* SKS 05apr99 allocate large args struct separate from custom defn array (otherwise easily reaches 64k!) */
    if(NULL == (p_ev_custom_args = al_ptr_calloc_elem(EV_CUSTOM_ARGS, 1, &status)))
        return(status);

    if(NULL == (p_ev_custom = al_array_extend_by(&custom_def_deptable.h_table, EV_CUSTOM, 1, &array_init_block, &status)))
    {
        al_ptr_free(p_ev_custom_args);
        return(status);
    }

    p_ev_custom->args = p_ev_custom_args; /* donate args struct to custom function */

    p_ev_custom->handle = next_custom_handle++;
    p_ev_custom->owner.docno = EV_DOCNO_PACK(owner_ev_docno);

    /* this stops ev_uref removing a custom reference
     * from the table when it doesn't yet belong anywhere
     */
    p_ev_custom->owner.col = EV_COL_PACK(EV_MAX_COL - 1);
    p_ev_custom->owner.row =            (EV_MAX_ROW - 1);
    p_ev_custom->flags.undefined = 1;
    ustr_xstrkpy(ustr_bptr(p_ev_custom->ustr_custom_id), sizeof32(p_ev_custom->ustr_custom_id), ustr_custom_name);

    {
    const P_SS_DOC p_ss_doc = ev_p_ss_doc_from_docno(ev_slr_docno(&p_ev_custom->owner));

    if(P_SS_DOC_NONE != p_ss_doc)
        p_ss_doc->custom_ref_count += 1;
    } /*block*/

    return(array_indexof_element(&custom_def_deptable.h_table, EV_CUSTOM, p_ev_custom));
}

/******************************************************************************
*
* given a document and a name,
* ensure this [document]name
* combination exists in the resource table
*
* --out--
* <0  new name couldn't be inserted
* >=0 entry number of name
*
******************************************************************************/

_Check_return_
extern ARRAY_INDEX
ensure_name_in_list(
    _InVal_     EV_DOCNO owner_ev_docno,
    _In_z_      PC_USTR ustr_name)
{
    EV_HANDLE name_num;
    P_EV_NAME p_ev_name;
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(NAME_INC, sizeof32(EV_NAME), TRUE);
    STATUS status;

    if((name_num = find_name_in_list(owner_ev_docno, ustr_name)) >= 0)
        return(name_num);

    if(NULL == (p_ev_name = al_array_extend_by(&name_def_deptable.h_table, EV_NAME, 1, &array_init_block, &status)))
        return(status);

    p_ev_name->handle = next_name_handle++;
    p_ev_name->owner.docno = EV_DOCNO_PACK(owner_ev_docno);
    /* col, row need to be large enough not to be caught by accident */
    p_ev_name->owner.col = EV_COL_PACK(EV_MAX_COL - 1);
    p_ev_name->owner.row =            (EV_MAX_ROW - 1);
    ustr_xstrkpy(ustr_bptr(p_ev_name->ustr_name_id), sizeof32(p_ev_name->ustr_name_id), ustr_name);

    {
    SS_DATA ss_data;
    ss_data_set_blank(&ss_data);
    name_set_def(p_ev_name, &ss_data, 1);
    } /*block*/

    {
    const P_SS_DOC p_ss_doc = ev_p_ss_doc_from_docno(ev_slr_docno(&p_ev_name->owner));

    if(P_SS_DOC_NONE != p_ss_doc)
        p_ss_doc->name_ref_count += 1;
    } /*block*/

    return(array_indexof_element(&name_def_deptable.h_table, EV_NAME, p_ev_name));
}

#if defined(UNUSED)

/* Interestingly, we never implemented these for custom */

/******************************************************************************
*
* switch name deletions off
*
******************************************************************************/

extern void
ev_name_del_hold(void)
{
    name_def_deptable.flags.delhold = 1;
}

/******************************************************************************
*
* allow name deletions again
*
******************************************************************************/

extern void
ev_name_del_release(void)
{
    name_def_deptable.flags.delhold = 0;
}

#endif /* UNUSED */

/******************************************************************************
*
* define a name with the given argument
*
* --in--
* set undefine to make the name
* 'undefined'
*
* --out--
* < 0 error
* >=0 OK
*
******************************************************************************/

_Check_return_
extern STATUS
ev_name_make(
    _In_z_      PC_USTR ustr_name_id,
    _InVal_     EV_DOCNO ev_docno /* document owning definition */,
    _In_opt_z_  PC_USTR ustr_name_def,
    _InVal_     S32 undefine,
    _In_opt_z_  PC_USTR ustr_description)
{
    S32 res = 0;

    if(undefine)
    {
        EV_HANDLE name_num = find_name_in_list(ev_docno, ustr_name_id);

        if(name_num >= 0)
        {
            const P_EV_NAME p_ev_name = array_ptr(&name_def_deptable.h_table, EV_NAME, name_num);
            ev_todo_add_name_dependents(p_ev_name->handle);
            ss_data_free_resources(&p_ev_name->def_data);
            p_ev_name->flags.undefined = 1;
        }
        else
            res = create_error(EVAL_ERR_NAMEUNDEF);
    }
    else
    {
        SS_DATA ss_data;
        DOCU_NAME docu_name;

        name_init(&docu_name);
        ss_data_set_blank(&ss_data);

        /* check name of name is OK */
        status_return(ident_validate(ustr_name_id, FALSE));

        if(NULL != ustr_name_def)
        {
            if((res = ss_recog_constant(&ss_data, ustr_name_def)) <= 0)
            if((res = recog_slr_range(&ss_data, &docu_name, ev_docno, ustr_name_def)) == 0)
                res = create_error(EVAL_ERR_ODF_NA);
        }
        else
            /* allow blank definitions */
            res = 1;

        if(res > 0)
        {
            EV_HANDLE name_handle;
            SS_STRINGC ss_stringc;

            ss_stringc.uchars = ustr_name_id; /* loan */
            ss_stringc.size = ustrlen32(ustr_name_id);
            res = name_make(&name_handle, ev_docno, &ss_stringc, &ss_data, ustr_description);

            /* name_make has copied resources */
            ss_data_free_resources(&ss_data);
        }

        name_dispose(&docu_name);
    }

    return(res);
}

/******************************************************************************
*
* search internal resource list for a custom function
*
* --out--
* <0  custom not found
* >=0 entry number of custom
*
******************************************************************************/

_Check_return_
extern EV_HANDLE
find_custom_in_list(
    _InVal_     EV_DOCNO owner_ev_docno,
    _In_z_      PC_USTR ustr_custom_name)
{
    const ARRAY_INDEX n_elements = array_elements(&custom_def_deptable.h_table);
    ARRAY_INDEX i;
    PC_EV_CUSTOM p_ev_custom = array_rangec(&custom_def_deptable.h_table, EV_CUSTOM, 0, n_elements);

    for(i = 0; i < n_elements; ++i, ++p_ev_custom)
    {
        if(p_ev_custom->flags.to_be_deleted)
            continue;

        if(owner_ev_docno != ev_slr_docno(&p_ev_custom->owner))
            continue;

        if(ustr_compare_equals_nocase(ustr_bptrc(p_ev_custom->ustr_custom_id), ustr_custom_name))
            return(i);
    }

    return(EVAL_ERR_CUSTOMUNDEF);
}

/******************************************************************************
*
* search name definition table for a name
*
* --out--
* <0  name not found
* >=0 entry number of name
*
******************************************************************************/

_Check_return_
extern EV_HANDLE
find_name_in_list(
    _InVal_     EV_DOCNO owner_ev_docno,
    _In_z_      PC_USTR ustr_name)
{
    const ARRAY_INDEX n_elements = array_elements(&name_def_deptable.h_table);
    ARRAY_INDEX i;
    PC_EV_NAME p_ev_name = array_rangec(&name_def_deptable.h_table, EV_NAME, 0, n_elements);

    for(i = 0; i < n_elements; ++i, ++p_ev_name)
    {
        if(p_ev_name->flags.to_be_deleted)
            continue;

        if(owner_ev_docno != ev_slr_docno(&p_ev_name->owner))
            continue;

        if(ustr_compare_equals_nocase(ustr_bptrc(p_ev_name->ustr_name_id), ustr_name))
            return(i);
    }

    return(EVAL_ERR_NAMEUNDEF);
}

/******************************************************************************
*
* given a name id, update the target document's name reference count
*
******************************************************************************/

static void
name_ref_count_update(
    _InRef_     PC_EV_NAME p_ev_name,
    _InVal_     S32 update)
{
    switch(ss_data_get_data_id(&p_ev_name->def_data))
    {
    case DATA_ID_SLR:
        {
        const P_SS_DOC p_ss_doc = ev_p_ss_doc_from_docno(ev_slr_docno(&p_ev_name->def_data.arg.slr));

        PTR_ASSERT(p_ss_doc);
        if(P_SS_DOC_NONE != p_ss_doc)
        {
            p_ss_doc->name_ref_count += update;
            trace_2(TRACE_MODULE_EVAL,
                    TEXT("name_ref_count_update docno: ") U32_TFMT TEXT(", ref now: ") S32_TFMT,
                    (U32) ev_slr_docno(&p_ev_name->def_data.arg.slr), p_ss_doc->name_ref_count);
        }

        break;
        }

    case DATA_ID_RANGE:
        {
        const P_SS_DOC p_ss_doc = ev_p_ss_doc_from_docno(ev_slr_docno(&p_ev_name->def_data.arg.range.s));

        PTR_ASSERT(p_ss_doc);
        if(P_SS_DOC_NONE != p_ss_doc)
        {
            p_ss_doc->name_ref_count += update;
            trace_2(TRACE_MODULE_EVAL,
                    TEXT("name_ref_count_update docno: ") U32_TFMT TEXT(", ref now: ") S32_TFMT,
                    (U32) ev_slr_docno(&p_ev_name->def_data.arg.range.s), p_ss_doc->name_ref_count);
        }

        break;
        }
    }
}

/******************************************************************************
*
* lookup id in names definition table
*
* --out--
* <0  name not in table
* >=0 entry number of name
*
******************************************************************************/

_Check_return_
extern ARRAY_INDEX
name_def_from_handle(
    _InVal_     EV_HANDLE ev_handle)
{
    ARRAY_INDEX res = -1;
    EV_NAME temp;

    name_list_sort();

    temp.handle = ev_handle;

    if(array_elements(&name_def_deptable.h_table))
    {
        const PC_EV_NAME p_ev_name = (PC_EV_NAME)
            bsearch(&temp, array_basec(&name_def_deptable.h_table, EV_NAME), (U32) name_def_deptable.sorted, sizeof(EV_NAME), proc_compare_name_def);

        if(NULL != p_ev_name)
            res = array_indexof_element(&name_def_deptable.h_table, EV_NAME, p_ev_name);
    }

    return(res);
}

/******************************************************************************
*
* free any resources that are owned by a name definition
*
******************************************************************************/

extern void
name_free_resources(
    _InoutRef_  P_EV_NAME p_ev_name)
{
    name_ref_count_update(p_ev_name, -1);
    ss_data_free_resources(&p_ev_name->def_data);
    ustr_clr(&p_ev_name->ustr_description);
    ss_data_set_blank(&p_ev_name->def_data);
}

/******************************************************************************
*
* sort name definition table
*
******************************************************************************/

static inline void
name_list_sort_checkuse(void)
{
    /* check through name definition table to see if the deletion of
     * a name use has rendered the definition record useless
     */
    const ARRAY_INDEX n_elements = array_elements(&name_def_deptable.h_table);
    ARRAY_INDEX name_num;

    for(name_num = 0; name_num < n_elements; ++name_num)
    {
        const P_EV_NAME p_ev_name = array_ptr(&name_def_deptable.h_table, EV_NAME, name_num);

        if(p_ev_name->flags.to_be_deleted)
            continue;

        /* reference must be undefined before we can delete it */
        if(!p_ev_name->flags.undefined)
            continue;

        if( (search_for_name_use(p_ev_name->handle) < 0)
            &&
            !ev_tell_name_clients(p_ev_name->handle, FALSE /* changed */) )
        {
            trace_1(TRACE_MODULE_EVAL, TEXT("name_list_sort deleting undefined name: %s"), report_ustr(ustr_bptr(p_ev_name->ustr_name_id)));

            name_free_resources(p_ev_name);

            {
            const P_SS_DOC p_ss_doc = ev_p_ss_doc_from_docno(ev_slr_docno(&p_ev_name->owner));

            if(P_SS_DOC_NONE != p_ss_doc)
                p_ss_doc->name_ref_count -= 1;
            } /*block*/

            p_ev_name->flags.to_be_deleted = 1;
            name_def_deptable.flags.to_be_deleted = 1;
            name_def_deptable.mindel = MIN(name_def_deptable.mindel, name_num);
        }
    }

    name_def_deptable.flags.checkuse = 0;
}

extern void
name_list_sort(void)
{
    if(name_def_deptable.flags.checkuse /* && !name_def_deptable.flags.delhold */) /* SKS -delhold 08aug2015 as unused in Fz */
        name_list_sort_checkuse();

    consume_bool(tree_sort(&name_def_deptable, name_def_is_deleted, proc_compare_name_def));

    trace_2(TRACE_APP_MEMORY_USE,
            TEXT("name definition table is: ") U32_TFMT TEXT(" elements") U32_TFMT TEXT(" bytes"),
            array_elements32(&name_def_deptable.h_table), array_size32(&name_def_deptable.h_table) * array_element_size32(&name_def_deptable.h_table));
}

/******************************************************************************
*
* make a name with a given value
*
******************************************************************************/

_Check_return_
extern S32
name_make(
    P_EV_HANDLE p_ev_handle,
    _InVal_     EV_DOCNO ev_docno,
    _InRef_     PC_SS_STRINGC p_name,
    _InRef_     PC_SS_DATA p_ss_data_in,
    _In_opt_z_  PC_USTR ustr_description)
{
    S32 res = 0;
    UCHARZ name_ustr_buf[BUF_EV_INTNAMLEN];

    if(p_name->size >= elemof32(name_ustr_buf))
        return(create_error(EVAL_ERR_BADIDENT));

    memcpy32(name_ustr_buf, p_name->uchars, p_name->size);
    name_ustr_buf[p_name->size] = CH_NULL;

    status_return(ident_validate(ustr_bptr(name_ustr_buf), FALSE));

    {
    EV_HANDLE name_num = ensure_name_in_list(ev_docno, ustr_bptr(name_ustr_buf));

    if(status_ok(name_num))
    {
        const P_EV_NAME p_ev_name = array_ptr(&name_def_deptable.h_table, EV_NAME, name_num);

        name_free_resources(p_ev_name);

        name_set_def(p_ev_name, p_ss_data_in, 0);

        name_ref_count_update(p_ev_name, 1);

        if(NULL != ustr_description)
            status_assert(res = ustr_set(&p_ev_name->ustr_description, ustr_description));

        *p_ev_handle = p_ev_name->handle;
    }
    else
        res = (STATUS) name_num;
    } /*block*/

    return(res);
}

/******************************************************************************
*
* compare custom ids
*
******************************************************************************/

PROC_BSEARCH_PROTO(static, proc_compare_custom_def, EV_CUSTOM, EV_CUSTOM)
{
    BSEARCH_KEY_VAR_DECL(PC_EV_CUSTOM, key);
    BSEARCH_DATUM_VAR_DECL(PC_EV_CUSTOM, datum);

    if(key->handle == datum->handle)
        return(0);

    return(key->handle > datum->handle ? 1 : -1);
}

/******************************************************************************
*
* return flag saying whether entry is deleted for aligator garbage collection
*
******************************************************************************/

PROC_ELEMENT_IS_DELETED_PROTO(static, custom_def_is_deleted)
{
    const PC_EV_CUSTOM p_ev_custom = (PC_EV_CUSTOM) p_any;

    return(p_ev_custom->flags.to_be_deleted);
}

/******************************************************************************
*
* compare names
*
******************************************************************************/

PROC_BSEARCH_PROTO(static, proc_compare_name_def, EV_NAME, EV_NAME)
{
    BSEARCH_KEY_VAR_DECL(PC_EV_NAME, key);
    BSEARCH_DATUM_VAR_DECL(PC_EV_NAME, datum);

    if(key->handle == datum->handle)
        return(0);

    return(key->handle > datum->handle ? 1 : -1);
}

/******************************************************************************
*
* return flag saying whether entry is deleted for aligator garbage collection
*
******************************************************************************/

PROC_ELEMENT_IS_DELETED_PROTO(static, name_def_is_deleted)
{
    const PC_EV_NAME p_ev_name = (PC_EV_NAME) p_any;

    return(p_ev_name->flags.to_be_deleted);
}

/* end of ev_name.c */
