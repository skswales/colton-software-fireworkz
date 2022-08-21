/* sk_docno.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/******************************************************************************
*
* routines that manage documents and document references for Fireworkz
*
* Note: P_DOCUs are semi-permanent; they should not be stored in any
* long-term data structure; they may become invalid whenever documents
* are allocated and deallocated
*
* DOCNOs are safe; the p_docu_from_docno() call is fast
*
* MRJC March 1992
*
******************************************************************************/

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

/*
internal routines
*/

_Check_return_
static BOOL /* TRUE=can reuse entry */
docno_void_entry(
    _DocuRef_   P_DOCU p_docu);

/*
document table variables
*/

#if CHECKING /* otherwise it is exposed for the inline access function */
static
#endif
P_DOCU g_docu_table[256];

static U32 g_docu_table_limit = 1; /* excl. DOCNO_NONE entry is valid (albeit always NONE) */

static S32 docno_reuse_hold_count; /* document re-use hold */

/******************************************************************************
*
* remove all traces of a file when it is closed and the handle becomes invalid
*
******************************************************************************/

static void
docno_close_issue_close(
    _DocuRef_   P_DOCU p_docu)
{
    DOCU_ASSERT(p_docu);

    if(!p_docu->flags.init_close)
        return;

    p_docu->flags.init_close = 0;

    p_docu->flags.document_active = 0;

    /* tell all the hangers on */
    status_assert(uref_event(p_docu, T5_MSG_UREF_CLOSE1, P_DATA_NONE));
    {
    MSG_INITCLOSE msg_initclose;
    msg_initclose.t5_msg_initclose_message = T5_MSG_IC__CLOSE1;
    status_assert(maeve_event(p_docu, T5_MSG_INITCLOSE, &msg_initclose));
    status_assert(maeve_service_event(p_docu, T5_MSG_INITCLOSE, &msg_initclose));
    } /*block*/

    status_assert(uref_event(p_docu, T5_MSG_UREF_CLOSE2, P_DATA_NONE));
    {
    MSG_INITCLOSE msg_initclose;
    msg_initclose.t5_msg_initclose_message = T5_MSG_IC__CLOSE2;
    status_assert(maeve_event(p_docu, T5_MSG_INITCLOSE, &msg_initclose));
    status_assert(maeve_service_event(p_docu, T5_MSG_INITCLOSE, &msg_initclose));
    } /*block*/
}

static void
docno_close_issue_close_thunk(
    _DocuRef_   P_DOCU p_docu)
{
    if(!p_docu->flags.init_close_thunk)
        return;

    p_docu->flags.init_close_thunk = 0;

    { /* no point sending a uref_event - there must be no uref handler to have got this far */
    MSG_INITCLOSE msg_initclose;
    msg_initclose.t5_msg_initclose_message = T5_MSG_IC__CLOSE_THUNK;
    status_assert(maeve_event(p_docu, T5_MSG_INITCLOSE, &msg_initclose));
    status_assert(maeve_service_event(p_docu, T5_MSG_INITCLOSE, &msg_initclose));
    maeve_event_close_thunk(p_docu); /* tidy up */
    } /*block*/

    reportf(/*trace_4(TRACE_APP_SKEL,*/
            TEXT("docno_close_issue_close_thunk freed docno: ") S32_TFMT TEXT(", path(%s), leaf(%s), extension(%s)"),
            (S32) p_docu->docno,
            report_tstr(p_docu->docu_name.path_name),
            report_tstr(p_docu->docu_name.leaf_name),
            report_tstr(p_docu->docu_name.extension));

    /* free name resources */
    name_dispose(&p_docu->docu_name);
}

extern void
docno_close(
    _InoutRef_  P_DOCNO p_docno)
{
    DOCNO docno;

    if(DOCNO_NONE == (docno = *p_docno))
        return;

    *p_docno = DOCNO_NONE;

    if(IS_DOCU_NONE(p_docu_from_docno(docno)))
    {
        trace_1(TRACE_APP_SKEL, TEXT("docno_close docno: ") S32_TFMT TEXT(" already gone"), (S32) docno);
        return; /* already gone! */
    }

    trace_1(TRACE_APP_SKEL, TEXT("docno_close docno: ") S32_TFMT, (S32) docno);

    docno_close_issue_close(p_docu_from_docno(docno));

    { /* look for any documents (including this one) that can be released as a result of the close */
    S32 docs_remaining = 0;
    DOCNO i = (DOCNO) g_docu_table_limit;

    while(--i > DOCNO_NONE)
    {
        const P_P_DOCU p_p_docu = &g_docu_table[i];
        const P_DOCU p_docu = *p_p_docu;

        if(IS_DOCU_NONE(p_docu))
            continue;

        if(!docno_void_entry(p_docu))
        {
            docs_remaining += 1;
            continue;
        }

        docno_close_issue_close_thunk(p_docu);

        alloc_block_dispose(&p_docu->general_string_alloc_block);

        al_ptr_dispose(P_P_ANY_PEDANTIC(p_p_docu));

        *p_p_docu = P_DOCU_NONE;
    }

#if 0 /* no longer needed now we use g_docu_table[] */
    /* when there are none left, blow away document table */
    if(0 == docs_remaining)
    { /*EMPTY*/ }
#endif
    } /*block*/

    /* try to give some core back... */
    arglist_cache_reduce();
#if RISCOS
    alloc_tidy_up();
#endif
}

extern void
docno_close_all(void)
{
    { /* SKS 27sep95 introduce separate phase to take down real documents first ie. before the config doc etc! */
    DOCNO docno = DOCNO_SKIP_CONFIG; /* SKS 09sep14 ensure that the config doc is skipped in this first phase */

    while(DOCNO_NONE != (docno = docno_enum_docs(docno)))
    {
        DOCNO docno_i = docno;
        docno_close(&docno_i);
    }
    } /*block*/

    {
    DOCNO docno = DOCNO_SKIP_CONFIG; /* SKS 09sep14 ensure that the config doc is skipped in this second phase too */

    while(DOCNO_NONE != (docno = docno_enum_thunks(docno)))
    {
        DOCNO docno_i = docno;
        docno_close(&docno_i);
    }
    } /*block*/

    { /* SKS 09sep14 ensure that the config doc goes last of all (it is no longer just a thunk) */
    DOCNO docno = DOCNO_CONFIG;
    docno_close(&docno);
    } /*block*/
}

/******************************************************************************
*
* enumerate all valid documents
* start by calling with DOCNO_NONE (or DOCNO_SKIP_CONFIG)
* ends by returning DOCNO_NONE
*
******************************************************************************/

_Check_return_
extern DOCNO /* DOCNO_NONE == end */
docno_enum_docs(
    _InVal_     DOCNO docno_in)
{
    DOCNO res = DOCNO_NONE;
    DOCNO docno = (DOCNO) ((docno_in == DOCNO_NONE) ? DOCNO_FIRST : (docno_in + 1));

    for(; docno < g_docu_table_limit; ++docno)
    {
        const P_DOCU p_docu = p_docu_from_docno(docno);

        if(docno_void_entry(p_docu))
            continue;

        if(!p_docu->flags.has_data)
            continue;

        res = docno;
        break;
    }

    return(res);
}

/******************************************************************************
*
* enumerate all thunks
*
* start by calling with DOCNO_NONE
* ends by returning DOCNO_NONE
*
******************************************************************************/

_Check_return_
extern DOCNO /* DOCNO_NONE == end */
docno_enum_thunks(
    _InVal_     DOCNO docno_in)
{
    DOCNO res = DOCNO_NONE;
    DOCNO docno = (DOCNO) ((docno_in == DOCNO_NONE) ? DOCNO_FIRST : (docno_in + 1));

    for(; docno < g_docu_table_limit; ++docno)
    {
        const P_DOCU p_docu = p_docu_from_docno(docno);

        if(docno_void_entry(p_docu))
            continue;

        res = docno;
        break;
    }

    return(res);
}

/******************************************************************************
*
* establish a document number for the name
*
* --out--
* DOCNO_NONE = unable to allocate a document
*
******************************************************************************/

_Check_return_
static STATUS
docno_establish_issue_init_thunk(
    _DocuRef_   P_DOCU p_docu)
{
    MSG_INITCLOSE msg_initclose;
    msg_initclose.t5_msg_initclose_message = T5_MSG_IC__INIT_THUNK;
    return(maeve_service_event(p_docu, T5_MSG_INITCLOSE, &msg_initclose));
}

_Check_return_
extern DOCNO
docno_establish_docno_from_name(
    _InRef_     PC_DOCU_NAME p_docu_name)
{
    DOCNO docno;

    /* either use given docno, or get a new one */
    if(DOCNO_NONE == (docno = docno_find_name(p_docu_name)))
    {
        /* we didn't find a match for the name - allocate a new document for the reference */
        U32 ab_size = 512;
        STATUS status = STATUS_OK;
        P_DOCU p_docu;

        { /* scan g_docu_table[] for a free slot */
        P_P_DOCU p_p_docu = NULL;
        DOCNO i = (DOCNO) g_docu_table_limit;

        while(--i > DOCNO_NONE)
        {
            P_P_DOCU this_p_p_docu = &g_docu_table[i];

            p_docu = *this_p_p_docu;

            if(IS_DOCU_NONE(p_docu) || docno_void_entry(p_docu))
            {
                p_p_docu = this_p_p_docu;
                break;
            }
        }

        if(NULL == p_p_docu)
        {   /* Need to 'allocate' a slot for a new document at the end of g_docu_table[] */
            if(g_docu_table_limit >= DOCNO_MAX)
                return(DOCNO_NONE);

            /* NB the first one allocated will be &g_docu_table[1], thereby avoiding DOCNO_NONE */
            p_p_docu = &g_docu_table[g_docu_table_limit++];

            *p_p_docu = P_DOCU_NONE;
        }

        if(IS_DOCU_NONE(*p_p_docu))
        {   /* Need to allocate a new document structure in this slot */
            if(NULL == (p_docu = al_ptr_alloc_elem(DOCU, 1, &status)))
            {
                status_assert(status);
                return(DOCNO_NONE);
            }
            *p_p_docu = p_docu;
        }

        memset32(*p_p_docu, 0, sizeof32(DOCU));

        docno = (DOCNO) (p_p_docu - &g_docu_table[0]); /* return index of this slot in g_docu_table[] */
        } /*block*/

        p_docu = p_docu_from_docno(docno);

        p_docu->docno = docno;

        if(1 == docno)
            ab_size = 2048; /* SKS 24oct95 optimize for Config doc */

        ab_size -= ALLOCBLOCK_OVH;

        if(status_ok(status = alloc_block_create(&p_docu->general_string_alloc_block, ab_size)))
        if(status_ok(status = name_dup(&p_docu->docu_name, p_docu_name))) /* copy in our name */
        {
            reportf(/*trace_4(TRACE_APP_SKEL,*/
                   TEXT("docno_establish_docno_from_name established docno: ") S32_TFMT TEXT(", path(%s), leaf(%s), extension(%s)"),
                   (S32) p_docu->docno,
                   report_tstr(p_docu->docu_name.path_name),
                   report_tstr(p_docu->docu_name.leaf_name),
                   report_tstr(p_docu->docu_name.extension));

            /* p_docu->h_uref_table = 0; already zero */

            /* p_docu->flags.has_data = 0; already zero */

            /* initialise skeleton services for this document */
            if(status_ok(status = docno_establish_issue_init_thunk(p_docu)))
                p_docu->flags.init_close_thunk = 1;
        }

        if(status_fail(status))
        {
            name_dispose(&p_docu->docu_name); /* 'cos we might never have got to init_close_thunk ok */
            docno_close(&docno);
        }
    }

    return(docno);
}

/******************************************************************************
*
* search document table for an entry that matches the name supplied
*
******************************************************************************/

_Check_return_
extern DOCNO
docno_find_name(
    _InRef_     PC_DOCU_NAME p_docu_name)
{
    DOCNO res = DOCNO_NONE;
    DOCNO docno;

    PTR_ASSERT(p_docu_name->leaf_name);

    for(docno = DOCNO_FIRST; docno < g_docu_table_limit; ++docno)
    {
        const P_DOCU p_docu = p_docu_from_docno(docno);

        if(docno_void_entry(p_docu))
            continue;

        if(NULL == p_docu->docu_name.leaf_name)
            continue;

        /* if wholenames (excluding extensions) match */
        if(0 == name_compare(&p_docu->docu_name, p_docu_name, FALSE))
        {
            res = docno;
            break;
        }

        /* or ignore pathnames if both not supplied originally */
        /* either pathname not explicit */
        if( (!p_docu->docu_name.flags.path_name_supplied) ||
            (!p_docu_name->flags.path_name_supplied)      )
        {
            if(0 == tstricmp(p_docu->docu_name.leaf_name, p_docu_name->leaf_name))
            {
                res = docno;
                break;
            }
        }
    }

    return(res);
}

/******************************************************************************
*
* ensure that a given document is in the list
*
******************************************************************************/

PROC_ENSURE_DOCNO_PROTO(static, docno_ensure_doc_in_list)
{
    const ARRAY_INDEX n_docs = array_elements(&p_docu_dep_sup->h_docnos);
    ARRAY_INDEX i;
    P_DOCNO p_docno = array_range(&p_docu_dep_sup->h_docnos, DOCNO, 0, n_docs);

    for(i = 0; i < n_docs; ++i, ++p_docno)
        if(ev_docno == *p_docno)
            break;

    if(i == n_docs)
    {
        STATUS status;
        SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(DOCNO), FALSE);

        if(NULL == (p_docno = al_array_extend_by(&p_docu_dep_sup->h_docnos, DOCNO, 1, &array_init_block, &status)))
            return(status);

        *p_docno = ev_docno;
    }

    return(array_elements(&p_docu_dep_sup->h_docnos));
}

/******************************************************************************
*
* read a document number from an external reference in the form [......]
*
******************************************************************************/

_Check_return_
extern STATUS /* n processed */
docno_from_id(
    _DocuRef_   PC_DOCU p_docu,
    _OutRef_    P_DOCNO p_docno,
    _In_z_      PC_USTR ustr_id,
    _InVal_     BOOL ensure)
{
    STATUS status = STATUS_OK;
    PC_USTR ustr = ustr_id;
    DOCU_NAME docu_name;

    name_init(&docu_name);

    ustr_SkipSpaces(ustr);

    if(PtrGetByte(ustr) == CH_LEFT_SQUARE_BRACKET)
        if(status_ok(status = name_read_ustr(&docu_name, ustr_AddBytes(ustr, 1), CH_RIGHT_SQUARE_BRACKET)))
            ustr_IncBytes(ustr, 1 + status);

    *p_docno = DOCNO_NONE;

    if(status_ok(status))
    {
        if(name_is_blank(&docu_name))
            *p_docno = docno_from_p_docu(p_docu);
        else
            *p_docno = docno_find_name(&docu_name);
    }

    if(ensure && *p_docno == DOCNO_NONE)
        *p_docno = docno_establish_docno_from_name(&docu_name);

    name_dispose(&docu_name);

    return(status_ok(status) ? PtrDiffBytesS32(ustr, ustr_id) : status);
}

/******************************************************************************
*
* ask all objects for dependent documents
*
******************************************************************************/

_Check_return_
extern S32
docno_get_dependent_docs(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_ARRAY_HANDLE p_h_docnos)
{
    DOCU_DEP_SUP docu_dep_sup;

    docu_dep_sup.h_docnos = 0;
    docu_dep_sup.p_proc_ensure_docno = docno_ensure_doc_in_list;

    status_consume(object_call_all(p_docu, T5_MSG_DOCU_DEPENDENTS, &docu_dep_sup));

    *p_h_docnos = docu_dep_sup.h_docnos;

    return(array_elements(&docu_dep_sup.h_docnos));
}

/******************************************************************************
*
* ask all objects for linked documents
*
******************************************************************************/

_Check_return_
extern S32
docno_get_linked_docs(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_ARRAY_HANDLE p_h_docnos)
{
    DOCU_DEP_SUP docu_dep_sup;

    docu_dep_sup.h_docnos = 0;
    docu_dep_sup.p_proc_ensure_docno = docno_ensure_doc_in_list;

    if(status_ok(docno_ensure_doc_in_list(&docu_dep_sup, docno_from_p_docu(p_docu))))
    {
        ARRAY_INDEX i = 0;

        do  {
            const DOCNO docno = *array_ptr(&docu_dep_sup.h_docnos, DOCNO, i);

            status_consume(object_call_all(p_docu_from_docno(docno), T5_MSG_DOCU_DEPENDENTS, &docu_dep_sup));

            status_consume(object_call_all(p_docu_from_docno(docno), T5_MSG_DOCU_SUPPORTERS, &docu_dep_sup));

            i += 1;
        }
        while(i < array_elements(&docu_dep_sup.h_docnos));
    }

    *p_h_docnos = docu_dep_sup.h_docnos;

    return(array_elements(&docu_dep_sup.h_docnos));
}

/******************************************************************************
*
* ask all objects for supporting documents
*
******************************************************************************/

_Check_return_
extern S32
docno_get_supporting_docs(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_ARRAY_HANDLE p_h_docnos)
{
    DOCU_DEP_SUP docu_dep_sup;

    docu_dep_sup.h_docnos = 0;
    docu_dep_sup.p_proc_ensure_docno = docno_ensure_doc_in_list;

    status_consume(object_call_all(p_docu, T5_MSG_DOCU_SUPPORTERS, &docu_dep_sup));

    *p_h_docnos = docu_dep_sup.h_docnos;

    return(array_elements(&docu_dep_sup.h_docnos));
}

/******************************************************************************
*
* write out the whole name of a document; if
* it has part or all of its path in common with
* another document, this is stripped out
*
******************************************************************************/

_Check_return_
extern U32 /* number of chars output */
docno_name_write_ustr_buf(
    _Out_writes_z_(elemof_buffer) P_USTR ustr_buf,
    _InVal_     U32 elemof_buffer,
    _InVal_     DOCNO docno_to /* name output */,
    _InVal_     DOCNO docno_from /* common path */)
{
    P_DOCU p_docu_from;

    assert(DOCNO_NONE != docno_to);
    if(DOCNO_NONE == docno_to)
    {
        ustr_buf[0] = CH_NULL;
        return(0);
    }

    p_docu_from = p_docu_from_docno(docno_from);

    return(name_write_ustr_buf(ustr_buf, elemof_buffer, &p_docu_from_docno(docno_to)->docu_name, !IS_DOCU_NONE(p_docu_from) ? &p_docu_from->docu_name : NULL, FALSE));
}

/******************************************************************************
*
* create a new document structure, initialise it and any objects that are worried
*
******************************************************************************/

_Check_return_
static STATUS
docno_new_issue_init(
    _DocuRef_   P_DOCU p_docu)
{
    MSG_INITCLOSE msg_initclose;

    msg_initclose.t5_msg_initclose_message = T5_MSG_IC__INIT1;
    status_return(maeve_service_event(p_docu, T5_MSG_INITCLOSE, &msg_initclose));

    msg_initclose.t5_msg_initclose_message = T5_MSG_IC__INIT2;
           return(maeve_service_event(p_docu, T5_MSG_INITCLOSE, &msg_initclose));
}

_Check_return_
extern DOCNO
docno_new_create(
    _InRef_opt_ PC_DOCU_NAME p_docu_name /* NULL gets untitled */)
{
    STATUS status = STATUS_FAIL;
    DOCNO docno = DOCNO_NONE;
    DOCU_NAME docu_name;

    name_init(&docu_name);

    /* set up name */
    if(NULL == p_docu_name)
        status = name_set_untitled(&docu_name);
    else
    {
        status = name_dup(&docu_name, p_docu_name);

        if(status_ok(status))
            consume_bool(name_preprocess_docu_name_flags_for_rename(&docu_name));
    }

    if(status_ok(status))
    {
        /* NB. returned document has been memclr'd */
        if(DOCNO_NONE != (docno = docno_establish_docno_from_name(&docu_name)))
        {
            const P_DOCU p_docu = p_docu_from_docno(docno);

            /* initialise skeleton services for this document */
            p_docu->flags.document_active = 1;

            status = docno_new_issue_init(p_docu);

            if(status_fail(status))
                p_docu->flags.document_active = 0;
            else
                p_docu->flags.init_close = 1;
        }
    }

    name_dispose(&docu_name);

    if(status_fail(status))
    {
        docno_close(&docno);
        docno = DOCNO_NONE;
    }

    return(docno);
}

/******************************************************************************
*
* prevent re-use of possibly void document
* entries temporarily (used during compilation)
*
******************************************************************************/

extern void
docno_reuse_hold(void)
{
    docno_reuse_hold_count += 1;
}

/******************************************************************************
*
* release document reuse
*
******************************************************************************/

extern void
docno_reuse_release(void)
{
    docno_reuse_hold_count -= 1;
}

/******************************************************************************
*
* work out whether this entry in document tree is worth keeping
*
* --out--
* TRUE  = can reuse entry
* FALSE = entry still required
*
******************************************************************************/

_Check_return_
static BOOL /* TRUE=can reuse entry */
docno_void_entry(
    _DocuRef_   P_DOCU p_docu)
{
    if(IS_DOCU_NONE(p_docu))
        return(TRUE);

    /* does this document have an instance ? */
    if(p_docu->flags.has_data)
        return(FALSE);

    if(docno_reuse_hold_count && (NULL != p_docu->docu_name.leaf_name))
        return(FALSE);

    if(0 != p_docu->h_uref_table)
        return(FALSE);

    return(TRUE);
}

/*
main events
*/

_Check_return_
static STATUS
sk_docno_msg_init_flow(
    _DocuRef_   P_DOCU p_docu,    
    _InRef_     P_OBJECT_ID p_object_id)
{
    STATUS status;

    assert(!p_docu->flags.flow_installed);

    if(status_ok(status = object_load(*p_object_id)))
    {
        p_docu->flags.flow_installed = 1;

        p_docu->object_id_flow = *p_object_id;

        /* send the init_flow message to the object */
        status_consume(object_call_id(p_docu->object_id_flow, p_docu, T5_MSG_INIT_FLOW, p_object_id));
    }

    return(status);
}

/* really belongs in sk_name.c but this saves an extra maeve handler */

_Check_return_
static STATUS
sk_docno_msg_docu_rename(
    _DocuRef_   P_DOCU p_docu,
    _In_z_      PCTSTR filename)
{
    STATUS status;
    DOCU_NAME docu_name;

    name_init(&docu_name);

    if(status_ok(status = name_read_tstr(&docu_name, filename)))
    {
        consume_bool(name_preprocess_docu_name_flags_for_rename(&docu_name));

        name_dispose(&p_docu->docu_name);
        p_docu->docu_name = docu_name;

        /* tell the world the name has changed */
        status_assert(maeve_event(p_docu, T5_MSG_DOCU_NAME, P_DATA_NONE));
    }

    return(status);
}

MAEVE_EVENT_PROTO(static, maeve_event_sk_docno)
{
    IGNOREPARM_InRef_(p_maeve_block);

    switch(t5_message)
    {
    case T5_MSG_INIT_FLOW: /* initialise flow object */
        return(sk_docno_msg_init_flow(p_docu, (P_OBJECT_ID) p_data));

    case T5_MSG_DOCU_RENAME:
        return(sk_docno_msg_docu_rename(p_docu, (PCTSTR) p_data));

    case T5_CMD_OF_END_OF_DATA:
        /* load supporting documents at end of load */
        load_supporting_documents(p_docu);
        return(STATUS_OK);

    default:
        return(STATUS_OK);
    }
}

/*
exported services hook
*/

MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_sk_docno);

_Check_return_
static STATUS
sk_docno_msg_init1(
    _DocuRef_   P_DOCU p_docu)
{
    p_docu->flags.flow_installed = 0;

    p_docu->object_id_flow = OBJECT_ID_NONE;

    return(STATUS_OK);
}

T5_MSG_PROTO(static, maeve_services_sk_docno_msg_initclose, _InRef_ PC_MSG_INITCLOSE p_msg_initclose)
{
    IGNOREPARM_InVal_(t5_message);

    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__INIT_THUNK:
        return(maeve_event_handler_add(p_docu, maeve_event_sk_docno, (CLIENT_HANDLE) 0));

    case T5_MSG_IC__INIT1:
        return(sk_docno_msg_init1(p_docu));

    case T5_MSG_IC__CLOSE_THUNK:
        maeve_event_handler_del(p_docu, maeve_event_sk_docno, (CLIENT_HANDLE) 0);
        return(STATUS_OK);

    default:
        return(STATUS_OK);
    }
}

MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_sk_docno)
{
    IGNOREPARM_InRef_(p_maeve_services_block);

    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        return(maeve_services_sk_docno_msg_initclose(p_docu, t5_message, (PC_MSG_INITCLOSE) p_data));

    default:
        return(STATUS_OK);
    }
}

_Check_return_
extern DOCNO
docno_from_p_docu(
    _DocuRef_   PC_DOCU p_docu)
{
#if CHECKING
    if(NULL == p_docu) /* until this release is proven kosher and the assert here NEVER triggers ... */
    {
        assert(NULL != p_docu);
        return(DOCNO_NONE);
    }
#endif

    if(IS_DOCU_NONE(p_docu))
        return(DOCNO_NONE);

    return(p_docu->docno);
}

#if CHECKING

_Check_return_
_Ret_maybenone_
extern P_DOCU
p_docu_from_docno(
    _InVal_     DOCNO docno)
{
    assert(docno < g_docu_table_limit);
    if(docno >= g_docu_table_limit)
        return(P_DOCU_NONE);

    /* zeroth entry is always P_DOCU_NONE */
    /* all entries are initialsed to P_DOCU_NONE */
    /* freed entries are always reset to P_DOCU_NONE */

    return(g_docu_table[docno]);
}

#endif

extern void
sk_docno_startup(void)
{
    U32 docno;

    for(docno = 0; docno < 256; ++docno)
        g_docu_table[docno] = P_DOCU_NONE;
}

/* end of sk_docno.c */
