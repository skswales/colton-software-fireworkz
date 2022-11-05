/* tx_cache.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Text formatting for Fireworkz */

/* MRJC September 1992 */

#include "common/gflags.h"

#include "ob_story/ob_story.h"

/*
callback routines
*/

PROC_UREF_EVENT_PROTO(static, text_cache_uref_event);

/*
internal routines
*/

static void
text_cache_dispose_entry(
    P_TEXT_CACHE p_text_cache,
    _InVal_     S32 chunks);

static void
text_cache_init_entry(
    P_TEXT_CACHE p_text_cache);

#define TEXT_CACHE_SIZE 40

static S32 next_client_handle = 1;

/******************************************************************************
*
* find a given client handle in text object cache
*
******************************************************************************/

_Check_return_
_Ret_maybenull_
static P_TEXT_CACHE
p_text_cache_from_client_handle(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     CLIENT_HANDLE client_handle)
{
    const ARRAY_INDEX txt_cache_elements = array_elements(&p_docu->h_text_cache);
    ARRAY_INDEX txt_ix;
    P_TEXT_CACHE p_text_cache = array_range(&p_docu->h_text_cache, TEXT_CACHE, 0, txt_cache_elements);

    for(txt_ix = 0; txt_ix < txt_cache_elements; ++txt_ix, ++p_text_cache)
    {
        if(p_text_cache->used && p_text_cache->client_handle == client_handle)
            return(p_text_cache);
    }

    return(NULL);
}

/******************************************************************************
*
* get a pointer to formatted text information for some data
*
******************************************************************************/

_Check_return_
_Ret_maybenull_
extern P_TEXT_CACHE
p_text_cache_from_data_ref(
    _DocuRef_   P_DOCU p_docu,
    P_TEXT_FORMAT_INFO p_text_format_info,
    P_INLINE_OBJECT p_inline_object)
{
    P_TEXT_CACHE p_text_cache = NULL;

    if(0 != array_elements(&p_docu->h_text_cache))
    {
        const ARRAY_INDEX txt_cache_elements = array_elements(&p_docu->h_text_cache);
        ARRAY_INDEX txt_cache_ix;
        P_TEXT_CACHE p_text_cache_i = array_range(&p_docu->h_text_cache, TEXT_CACHE, 0, txt_cache_elements);

        for(txt_cache_ix = 0; txt_cache_ix < txt_cache_elements; ++txt_cache_ix, ++p_text_cache_i)
        {
            if(!p_text_cache_i->used)
                continue;

            if(!data_refs_equal(&p_text_cache_i->data_ref, &p_inline_object->object_data.data_ref))
                continue;

            p_text_cache = p_text_cache_i;

            if_constant(tracing(TRACE_APP_FORMAT))
            {
                if(p_inline_object->object_data.data_ref.data_space == DATA_SLOT)
                    trace_3(TRACE_APP_FORMAT,
                            TEXT("*** text cache hit *** col: ") COL_TFMT TEXT(", row: ") ROW_TFMT TEXT(", h_chunks: ") U32_TFMT,
                            p_text_cache->data_ref.arg.slr.col, p_text_cache->data_ref.arg.slr.row,
                            p_text_cache->formatted_text.h_chunks);
                else
                    trace_2(TRACE_APP_FORMAT,
                            TEXT("*** HEFO text cache hit *** data_space: ") S32_TFMT TEXT(", row: ") ROW_TFMT,
                            (S32) p_text_cache->data_ref.data_space, p_text_cache->data_ref.arg.row);
            }

            break;
        }
    }

    if(NULL == p_text_cache)
    {
        ARRAY_INDEX size_now = array_elements(&p_docu->h_text_cache);

        if(size_now < TEXT_CACHE_SIZE)
        {
            SC_ARRAY_INIT_BLOCK array_init_block = aib_init(TEXT_CACHE_SIZE, sizeof32(TEXT_CACHE), TRUE);
            STATUS status;
            /* n here is linked to average number of cols in row... */
            if(NULL == al_array_extend_by(&p_docu->h_text_cache, TEXT_CACHE, TEXT_CACHE_SIZE - size_now, &array_init_block, &status))
            {
                status_assert(status);
                return(NULL);
            }
            al_array_auto_compact_set(&p_docu->h_text_cache);
        }

        p_text_cache = array_ptr(&p_docu->h_text_cache, TEXT_CACHE, (S32) rand() % TEXT_CACHE_SIZE);

        if(p_text_cache->used)
            text_cache_dispose_entry(p_text_cache, TRUE);

        text_cache_init_entry(p_text_cache);

        if(p_inline_object->object_data.data_ref.data_space == DATA_SLOT)
        {
            REGION region;
            STATUS status;

            region_from_two_slrs(&region, &p_inline_object->object_data.data_ref.arg.slr, &p_inline_object->object_data.data_ref.arg.slr, TRUE);

            if(status_ok(status = uref_add_dependency(p_docu, &region, text_cache_uref_event, next_client_handle, &p_text_cache->uref_handle, FALSE)))
                p_text_cache->client_handle = next_client_handle++;
        }

        p_text_cache->docno = docno_from_p_docu(p_docu);
        p_text_cache->used = 1;
        p_text_cache->data_ref = p_inline_object->object_data.data_ref;

        if_constant(tracing(TRACE_APP_FORMAT))
        {
            if(p_inline_object->object_data.data_ref.data_space == DATA_SLOT)
                trace_2(TRACE_APP_FORMAT,
                        TEXT("p_text_cache_from_data_ref: ") COL_TFMT TEXT(", row: ") ROW_TFMT,
                        p_text_cache->data_ref.arg.slr.col,
                        p_text_cache->data_ref.arg.slr.row);

#if TRACE_ALLOWED
            text_cache_size(p_docu);
#endif
        }
    }

    /* split text into segments */
    if(p_text_cache && !p_text_cache->formatted_text.h_segments)
    {
        if_constant(tracing(TRACE_APP_FORMAT))
        {
            trace_3(TRACE_APP_FORMAT,
                    TEXT("*** text_segment %s chunks col: ") COL_TFMT TEXT(", row: ") ROW_TFMT,
                    p_text_cache->formatted_text.h_chunks ? TEXT("uses existing") : TEXT("needs new"),
                    p_text_cache->data_ref.arg.slr.col, p_text_cache->data_ref.arg.slr.row);
        }

        if(status_fail(text_segment(p_docu, &p_text_cache->formatted_text, p_text_format_info, p_inline_object)))
        {
            text_cache_dispose_entry(p_text_cache, TRUE);
            p_text_cache = NULL;
        }
    }

    return(p_text_cache);
}

/******************************************************************************
*
* release a text_cache entry
*
******************************************************************************/

static void
text_cache_dispose_entry(
    P_TEXT_CACHE p_text_cache,
    _InVal_     S32 chunks)
{
    formatted_text_dispose_segments(&p_text_cache->formatted_text);

    if(chunks)
    {
        if(p_text_cache->uref_handle)
            uref_del_dependency(p_text_cache->docno, p_text_cache->uref_handle);
        (void) formatted_text_dispose_chunks(&p_text_cache->formatted_text, 0);
        p_text_cache->data_ref.data_space = DATA_NONE;
        p_text_cache->used = 0;
    }

}

/******************************************************************************
*
* initialise a text cache entry
*
******************************************************************************/

static void
text_cache_init_entry(
    P_TEXT_CACHE p_text_cache)
{
    p_text_cache->data_ref.data_space = DATA_NONE;
    formatted_text_init(&p_text_cache->formatted_text);
    p_text_cache->used = 0;
    p_text_cache->uref_handle = 0;
    p_text_cache->client_handle = 0;
    p_text_cache->docno = DOCNO_NONE;
}

/******************************************************************************
*
* free unused entries in the text cache
*
******************************************************************************/

PROC_ELEMENT_IS_DELETED_PROTO(static, text_cache_entry_is_unused)
{
    const P_TEXT_CACHE p_text_cache = (P_TEXT_CACHE) p_any;

    return(!p_text_cache->used);
}

static void
text_cache_garbage_collect(
    _DocuRef_   P_DOCU p_docu)
{
    /* garbage collect text cache */
    AL_GARBAGE_FLAGS al_garbage_flags;
    AL_GARBAGE_FLAGS_CLEAR(al_garbage_flags);
    al_garbage_flags.remove_deleted = 1;
    al_garbage_flags.shrink = 1;
    al_array_auto_compact_set(&p_docu->h_text_cache);
    consume(S32, al_array_garbage_collect(&p_docu->h_text_cache, 0, text_cache_entry_is_unused, al_garbage_flags));
}

/******************************************************************************
*
* release any formatted text for a region
*
******************************************************************************/

static void
text_cache_release_region(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     DATA_SPACE data_space,
    _InRef_opt_ PC_REGION p_region,
    _InVal_     S32 chunks)
{
    if(p_docu->h_text_cache)
    {
        const ARRAY_INDEX txt_cache_elements = array_elements(&p_docu->h_text_cache);
        ARRAY_INDEX txt_ix;
        P_TEXT_CACHE p_text_cache = array_range(&p_docu->h_text_cache, TEXT_CACHE, 0, txt_cache_elements);
        BOOL any_freed = FALSE;

        for(txt_ix = 0; txt_ix < txt_cache_elements; ++txt_ix, ++p_text_cache)
        {
            if(!p_text_cache->used)
                continue;

            if((NULL == p_region) || data_ref_in_region(p_docu, data_space, p_region, &p_text_cache->data_ref))
            {
                text_cache_dispose_entry(p_text_cache, chunks);
                any_freed = TRUE;
            }
        }

        if(any_freed)
        {
            text_cache_garbage_collect(p_docu);
#if TRACE_ALLOWED
            text_cache_size(p_docu);
#endif
        }

        trace_1(TRACE_APP_SKEL, TEXT("text_cache_release_region: %s"), p_region ? TEXT("PART") : TEXT("WHOLE"));
    }
}

/******************************************************************************
*
* calculate text cache size
*
******************************************************************************/

#if TRACE_ALLOWED

extern void
text_cache_size(
    _DocuRef_   P_DOCU p_docu)
{
    ARRAY_INDEX txt_cache_elements;
    ARRAY_INDEX txt_ix;
    P_TEXT_CACHE p_text_cache;
    S32 tot_size = 0;
    S32 entries = 0;

    if_constant(!trace_is_on() || !tracing(TRACE_APP_MEMORY_USE))
        return;

    txt_cache_elements = array_elements(&p_docu->h_text_cache);

    p_text_cache = array_range(&p_docu->h_text_cache, TEXT_CACHE, 0, txt_cache_elements);

    for(txt_ix = 0; txt_ix < txt_cache_elements; ++txt_ix, ++p_text_cache)
    {
        if(!p_text_cache->used)
            continue;

        tot_size += array_elements(&p_text_cache->formatted_text.h_segments) * sizeof32(SEGMENT);
        tot_size += array_elements(&p_text_cache->formatted_text.h_chunks) * sizeof32(CHUNK);
        ++entries;
    }

    tot_size += txt_cache_elements * sizeof32(TEXT_CACHE);

    trace_2(TRACE_APP_MEMORY_USE,
            TEXT("formatted text: ") S32_TFMT TEXT(" entries used, ") S32_TFMT TEXT(" bytes"),
            entries, tot_size);
}

#endif /* TRACE_ALLOWED */

/******************************************************************************
*
* handle uref events for the text cache
*
******************************************************************************/

PROC_UREF_EVENT_PROTO(static, text_cache_uref_event_dep_delete)
{
    /* dependency must be deleted */
    P_TEXT_CACHE p_text_cache = p_text_cache_from_client_handle(p_docu, p_uref_event_block->uref_id.client_handle);

    UNREFERENCED_PARAMETER_InVal_(uref_message);

    PTR_ASSERT(p_text_cache);
    if(NULL != p_text_cache)
        text_cache_dispose_entry(p_text_cache, TRUE);

    return(STATUS_OK);
}

PROC_UREF_EVENT_PROTO(static, text_cache_uref_event_dep_update)
{
    /* dependency region must be updated */
    P_TEXT_CACHE p_text_cache = p_text_cache_from_client_handle(p_docu, p_uref_event_block->uref_id.client_handle);

    /* find our entry */
    PTR_ASSERT(p_text_cache);
    if(NULL != p_text_cache)
        if(Uref_Dep_None != uref_match_slr(&p_text_cache->data_ref.arg.slr, uref_message, p_uref_event_block))
            text_cache_dispose_entry(p_text_cache, TRUE);

    return(STATUS_OK);
}

PROC_UREF_EVENT_PROTO(static, text_cache_uref_event)
{
#if TRACE_ALLOWED && 0
    uref_trace_reason(UBF_UNPACK(UREF_COMMS, p_uref_event_block->reason.code), TEXT("TEXT_CACHE_UREF"));
#endif

    switch(UBF_UNPACK(UREF_COMMS, p_uref_event_block->reason.code))
    {
    case Uref_Dep_Delete: /* dependency must be deleted */
        return(text_cache_uref_event_dep_delete(p_docu, uref_message, p_uref_event_block));

    case Uref_Dep_Update: /* dependency region must be updated */
    case Uref_Dep_Inform:
        return(text_cache_uref_event_dep_update(p_docu, uref_message, p_uref_event_block));

    default: default_unhandled();
        return(STATUS_OK);
    }
}

/*
main events
*/

static void
text_cache_msg_caches_dispose(
    _DocuRef_   P_DOCU p_docu,
    P_CACHES_DISPOSE p_caches_dispose)
{
    text_cache_release_region(p_docu, p_caches_dispose->data_space, &p_caches_dispose->region, TRUE);
}

static void
text_cache_msg_reformat(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DOCU_REFORMAT p_docu_reformat)
{
    REGION region;

    text_cache_release_region(p_docu, p_docu_reformat->data_space, region_from_docu_reformat(&region, p_docu_reformat), TRUE);
}

static void
text_cache_msg_row_move_y(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_ROW p_row)
{
    REGION region;

    region.tl.row = *p_row;
    region.br.row = MAX_ROW;
    region.whole_col = 0;
    region.whole_row = 1;

    text_cache_release_region(p_docu, DATA_SLOT, &region, FALSE);

    trace_1(TRACE_APP_SKEL, TEXT("ROW_MOVE_Y, row: ") ROW_TFMT, *p_row);
}

static void
text_cache_msg_style_docu_area_changed(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     P_STYLE_DOCU_AREA_CHANGED p_style_docu_area_changed)
{
    REGION region;
    PC_REGION p_region = &region;

    if(p_style_docu_area_changed->data_space == DATA_SLOT)
        region_from_docu_area_max(&region, &p_style_docu_area_changed->docu_area);
    else
        p_region = NULL;

    text_cache_release_region(p_docu, DATA_NONE /* 7.11.94: ob_rec data space not getting freed */
                                      /* p_style_docu_area_changed->data_space */, p_region, TRUE);
}

MAEVE_EVENT_PROTO(static, maeve_event_text_cache)
{
    UNREFERENCED_PARAMETER_InRef_(p_maeve_block);

    switch(t5_message)
    {
    case T5_MSG_REFORMAT:
        text_cache_msg_reformat(p_docu, (PC_DOCU_REFORMAT) p_data);
        return(STATUS_OK);

    case T5_MSG_ROW_MOVE_Y:
        text_cache_msg_row_move_y(p_docu, (PC_ROW) p_data);
        return(STATUS_OK);

    case T5_MSG_CACHES_DISPOSE:
        text_cache_msg_caches_dispose(p_docu, (P_CACHES_DISPOSE) p_data);
        return(STATUS_OK);

    case T5_MSG_STYLE_DOCU_AREA_CHANGED:
        text_cache_msg_style_docu_area_changed(p_docu, (P_STYLE_DOCU_AREA_CHANGED) p_data);
        return(STATUS_OK);

    default:
        return(STATUS_OK);
    }
}

/*
exported services hook
*/

MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_tx_cache);

_Check_return_
static STATUS
text_cache_msg_init1(
    _DocuRef_   P_DOCU p_docu)
{
    p_docu->h_text_cache = 0;

    return(maeve_event_handler_add(p_docu, maeve_event_text_cache, (CLIENT_HANDLE) 0));
}

_Check_return_
static STATUS
text_cache_msg_close1(
    _DocuRef_   P_DOCU p_docu)
{
    maeve_event_handler_del(p_docu, maeve_event_text_cache, (CLIENT_HANDLE) 0);

    text_cache_release_region(p_docu, DATA_NONE, NULL, TRUE);

    al_array_dispose(&p_docu->h_text_cache);

    return(STATUS_OK);
}

T5_MSG_PROTO(static, maeve_services_tx_cache_msg_initclose, _InRef_ PC_MSG_INITCLOSE p_msg_initclose)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__INIT1:
        return(text_cache_msg_init1(p_docu));

    case T5_MSG_IC__CLOSE1:
        return(text_cache_msg_close1(p_docu));

    default:
        return(STATUS_OK);
    }
}

MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_tx_cache)
{
    UNREFERENCED_PARAMETER_InRef_(p_maeve_services_block);

    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        return(maeve_services_tx_cache_msg_initclose(p_docu, t5_message, (PC_MSG_INITCLOSE) p_data));

    default:
        return(STATUS_OK);
    }
}

/* end of tx_cache.c */
