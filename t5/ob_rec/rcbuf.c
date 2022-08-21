/* rcbuf.c */

/* Copyright (C) 1994-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* A read-edit buffer for database transactions */

#include "common/gflags.h"

#include "ob_rec/ob_rec.h"

#include "cmodules/mrofmun.h"

extern void
rec_cache_dispose(
    _DocuRef_   P_DOCU p_docu)
{
    P_REC_INSTANCE_DATA p_rec_instance = p_object_instance_data_REC(p_docu);
    ARRAY_INDEX i;

    trace_0(TRACE_APP_DPLIB, TEXT("rec cache free"));

    for(i = 0; i < array_elements(&p_rec_instance->h_cache); i++)
    {
        P_RCBUFFER p_rcbuffer = array_ptr(&p_rec_instance->h_cache, RCBUFFER, i);

        al_array_dispose(&p_rcbuffer->h_text_ustr);
    }

    al_array_dispose(&p_rec_instance->h_cache);
}

/* Enquire after the existance of a cache entry for this data ref

   Returns STATUS_DONE if found the data for this data_ref
           STATUS_OK   if not found
*/

_Check_return_
extern STATUS
rec_cache_enquire(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DATA_REF p_data_ref,
    _OutRef_    P_ARRAY_HANDLE_USTR p_h_text_ustr,
    _OutRef_    P_S32 p_state)
{
    P_REC_INSTANCE_DATA p_rec_instance = p_object_instance_data_REC(p_docu);
    ARRAY_INDEX i;

    for(i = 0; i < array_elements(&p_rec_instance->h_cache); i++)
    {
        P_RCBUFFER p_rcbuffer = array_ptr(&p_rec_instance->h_cache, RCBUFFER, i);

        if(0 == p_rcbuffer->h_text_ustr)
            continue;

        if(data_refs_equal(&p_rcbuffer->data_ref, p_data_ref))
        {
            *p_h_text_ustr = p_rcbuffer->h_text_ustr;
            *p_state = (S32) p_rcbuffer->modified;
            return(STATUS_DONE);
        }
    }

    *p_h_text_ustr = 0;
    *p_state = 0;
    return(STATUS_OK);
}

/* Set/clear the modified bit in cache for this data ref */

_Check_return_
extern STATUS
rec_cache_modified(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DATA_REF p_data_ref,
    _InVal_     BOOL modflag)
{
    P_REC_INSTANCE_DATA p_rec_instance = p_object_instance_data_REC(p_docu);
    ARRAY_INDEX i;

    for(i = 0; i < array_elements(&p_rec_instance->h_cache); i++)
    {
        P_RCBUFFER p_rcbuffer = array_ptr(&p_rec_instance->h_cache, RCBUFFER, i);

        if(0 == p_rcbuffer->h_text_ustr)
            continue;

        if(data_refs_equal(&p_rcbuffer->data_ref, p_data_ref))
        {
            p_rcbuffer->modified = modflag;
            return(STATUS_DONE);
        }
    }

    return(STATUS_OK);
}

/******************************************************************************
*
* Copy potentially edited text
* held in the cache back into
* the database
*
******************************************************************************/

_Check_return_
extern STATUS
rec_cache_purge(
    _DocuRef_   P_DOCU p_docu)
{
    P_REC_INSTANCE_DATA p_rec_instance = p_object_instance_data_REC(p_docu);
    ARRAY_INDEX i;
    STATUS status = STATUS_OK;

    trace_0(TRACE_APP_DPLIB, TEXT("rec cache purge"));

    for(i = 0; i < array_elements(&p_rec_instance->h_cache); i++)
    {
        P_RCBUFFER p_rcbuffer = array_ptr(&p_rec_instance->h_cache, RCBUFFER, i);
        ARRAY_HANDLE h_text_ustr = p_rcbuffer->h_text_ustr;

        if(0 == h_text_ustr)
            continue;

        p_rcbuffer->h_text_ustr = 0; /* Pretend we've dropped it */

        if(p_rcbuffer->modified)
        {
            /* Yes, so construct the write-data-back-to-object guff and call it */
            QUICK_UBLOCK quick_ublock;
            quick_ublock_setup_using_array(&quick_ublock, h_text_ustr);

            trace_2(TRACE_APP_DPLIB, TEXT("rec cache purging : record =") S32_TFMT TEXT(" field =") S32_TFMT, p_rcbuffer->data_ref.arg.db_field.record, p_rcbuffer->data_ref.arg.db_field.field_id);

            rec_object_write_text(p_docu, &p_rcbuffer->data_ref, &quick_ublock, FILETYPE_TEXT);

            p_rcbuffer->modified = 0;
        }

        al_array_dispose(&h_text_ustr); /* Drop it */
    }

    return(status);
}

/* Add/modify cache for this data ref */

_Check_return_
extern STATUS
rec_cache_update(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DATA_REF p_data_ref,
    _InRef_     PC_ARRAY_HANDLE_USTR p_h_text_ustr,
    _InVal_     BOOL modflag)
{
    STATUS status;
    P_REC_INSTANCE_DATA p_rec_instance = p_object_instance_data_REC(p_docu);
    ARRAY_INDEX i;
    P_RCBUFFER p_rcbuffer_use = P_DATA_NONE;
    ARRAY_INDEX free_slot = -1;

    for(i = 0; i < array_elements(&p_rec_instance->h_cache); i++)
    {
        P_RCBUFFER p_rcbuffer = array_ptr(&p_rec_instance->h_cache, RCBUFFER, i);

        if(0 == p_rcbuffer->h_text_ustr)
        {
            free_slot = i;
            continue;
        }

        if(data_refs_equal(&p_rcbuffer->data_ref, p_data_ref))
        {
            p_rcbuffer_use = p_rcbuffer;
            break;
        }
    }

    if(P_DATA_NONE == p_rcbuffer_use)
        if(-1 != free_slot)
            p_rcbuffer_use = array_ptr(&p_rec_instance->h_cache, RCBUFFER, free_slot);

    if(P_DATA_NONE == p_rcbuffer_use)
    {
        SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(*p_rcbuffer_use), FALSE);

        if(NULL == (p_rcbuffer_use = al_array_extend_by(&p_rec_instance->h_cache, RCBUFFER, 1, &array_init_block, &status)))
            return(status);
    }

    p_rcbuffer_use->h_text_ustr = *p_h_text_ustr;
    p_rcbuffer_use->modified = modflag;
    p_rcbuffer_use->data_ref = *p_data_ref;
    return(STATUS_DONE);
}

_Check_return_
static STATUS
rec_change_overwrite_binary(
    P_EDITREC p_editrec,
    _InRef_     PC_QUICK_UBLOCK p_quick_ublock_in, /* actually binary content here */
    _InVal_     T5_FILETYPE t5_filetype)
{
    const U32 len = quick_ublock_bytes(p_quick_ublock_in);
    STATUS status;

#if TRACE_ALLOWED
    switch(t5_filetype)
    {
    case FILETYPE_DRAW:
        myassert1x((len & 3) == 0, TEXT("Bad size ") U32_XTFMT TEXT(" for Drawfile - not a multiple of 4"), len);
        break;
    }
#endif

    if(NULL != (p_editrec->p_u8 = al_ptr_alloc_bytes(P_U8, len, &status))) /* SKS 07may95 a fiver says this fixes it! (NB NO +1 !!!) */
    {
        memcpy32(p_editrec->p_u8, quick_ublock_uchars(p_quick_ublock_in), len);
        p_editrec->length = len;
        p_editrec->t5_filetype = t5_filetype;
    }

    return(status);
}

_Check_return_
static STATUS
rec_change_overwrite_text(
    P_EDITREC p_editrec,
    _InRef_     PC_QUICK_UBLOCK p_quick_ublock_in)
{
    PC_USTR ustr = quick_ublock_ustr(p_quick_ublock_in);
    U32 len = ustrlen32(ustr);
    STATUS status;
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 40);
    quick_ublock_with_buffer_setup(quick_ublock);

    if(contains_inline(ustr, len))
    {
        PC_USTR_INLINE ustr_inline = (PC_USTR_INLINE) ustr;
        dplib_ustr_inline_convert(&quick_ublock, ustr_inline, ustr_inline_strlen32(ustr_inline));
        status_assert(quick_ublock_nullch_add(&quick_ublock));
        ustr = quick_ublock_ustr(&quick_ublock);
        len = ustrlen32(ustr);
    }

    if(NULL != (p_editrec->p_u8 = al_ptr_alloc_bytes(P_U8Z, len + 1, &status)))
    {
        memcpy32(p_editrec->p_u8, ustr, len + 1);
        p_editrec->length = len; /* NB length of contents does NOT include CH_NULL */
        p_editrec->t5_filetype = FILETYPE_TEXT;
    }

    quick_ublock_dispose(&quick_ublock);

    return(status);
}

/* returns STATUS_DONE if formatting has placed the o/p in out
           STATUS_OK   if you should just use the input buffer
*/

_Check_return_
static STATUS
rec_object_format_text(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_QUICK_UBLOCK p_quick_ublock_in,
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock_out /*appended,terminated*/)
{
    STATUS status = STATUS_OK;
    PC_USTR ustr_text_in = quick_ublock_ustr(p_quick_ublock_in);
    EV_DATA ev_data_out;
    ARRAY_HANDLE h_mrofmun;
    STYLE_HANDLE style_handle_autoformat;

    ev_data_set_blank(&ev_data_out);

    if(status_ok(mrofmun_get_list(p_docu, &h_mrofmun)))
        status_assert(autoformat(&ev_data_out, &style_handle_autoformat, ustr_text_in, &h_mrofmun));

    switch(ev_data_out.did_num)
    {
    case RPN_DAT_REAL:
    case RPN_DAT_BOOL8:
    case RPN_DAT_WORD8:
    case RPN_DAT_WORD16:
    case RPN_DAT_WORD32:
    case RPN_DAT_DATE:
        numform_parms_for_dplib.p_numform_context = get_p_numform_context(p_docu);
        (void) numform(p_quick_ublock_out, P_QUICK_TBLOCK_NONE, &ev_data_out, &numform_parms_for_dplib);
        status = STATUS_DONE;
        break;

    default:
        break;
    }

    ss_data_free_resources(&ev_data_out);

    return(status);
}

_Check_return_
static STATUS
rec_change_overwrite_eval(
    P_EDITREC p_editrec,
    _InRef_     PC_QUICK_UBLOCK p_quick_ublock_in,
    _DocuRef_   P_DOCU p_docu)
{
    STATUS status = STATUS_OK;
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 40);
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock_out, 40);
    quick_ublock_with_buffer_setup(quick_ublock);
    quick_ublock_with_buffer_setup(quick_ublock_out);

    status = rec_object_format_text(p_docu, p_quick_ublock_in, &quick_ublock_out);

    if(status_ok(status))
    {
        PC_USTR ustr;
        U32 len;

        if(status_done(status))
            ustr = quick_ublock_ustr(&quick_ublock_out);
        else
            ustr = quick_ublock_ustr(p_quick_ublock_in);

        len = ustrlen32(ustr);

        if(contains_inline(ustr, len))
        {
            PC_USTR_INLINE ustr_inline = (PC_USTR_INLINE) ustr;
            dplib_ustr_inline_convert(&quick_ublock, ustr_inline, ustr_inline_strlen32(ustr_inline));
            status_assert(quick_ublock_nullch_add(&quick_ublock));
            ustr = quick_ublock_ustr(&quick_ublock);
            len = ustrlen32(ustr);
        }

        if(NULL != (p_editrec->p_u8 = al_ptr_alloc_bytes(P_U8Z, len + 1, &status)))
        {
            memcpy32(p_editrec->p_u8, ustr, len + 1);
            p_editrec->length = len; /* NB length of contents does NOT include CH_NULL */
            p_editrec->t5_filetype = FILETYPE_TEXT;
        }
    }

    quick_ublock_dispose(&quick_ublock);
    quick_ublock_dispose(&quick_ublock_out);

    return(status);
}

/* Attempt to copy the string given by p_u8 into the handles table to overwrite the data from the file */

_Check_return_
static STATUS
rec_incorporate_changes_eat(
    _DocuRef_   P_DOCU p_docu,
    P_OPENDB p_opendb,
    _InRef_     PC_DATA_REF p_data_ref,
    _InRef_     PC_QUICK_UBLOCK p_quick_ublock_in,
    _InVal_     T5_FILETYPE t5_filetype)
{
    FIELD_ID field_id = field_id_from_rec_data_ref(p_data_ref);
    ARRAY_INDEX i;
    STATUS status;

    status_return(status = new_recordspec_handle_table(&p_opendb->recordspec, array_elements(&p_opendb->table.h_fielddefs)));

    /* For all fields do... see if the cache has one for us */
    for(i = 0; i < array_elements(&p_opendb->table.h_fielddefs); i++)
    {
        P_FIELDDEF p_fielddef = array_ptr(&p_opendb->table.h_fielddefs, FIELDDEF, i);
        P_EDITREC p_editrec = array_ptr(&p_opendb->recordspec.h_editrecs, EDITREC, i);

        status = STATUS_OK;

        if(p_fielddef->id == field_id)
        {
            switch(p_fielddef->type)
            {
            case FIELD_FILE:
            case FIELD_PICTURE: /* actually binary content here */
                status = rec_change_overwrite_binary(p_editrec, p_quick_ublock_in, t5_filetype);
                break;

            case FIELD_TEXT:
                status = rec_change_overwrite_text(p_editrec, p_quick_ublock_in);
                break;

            default: default_unhandled();
#if CHECKING
            case FIELD_INTEGER:
            case FIELD_DATE:
            case FIELD_INTERVAL:
            case FIELD_REAL:
            case FIELD_FORMULA:
            case FIELD_BOOL:
#endif
                status = rec_change_overwrite_eval(p_editrec, p_quick_ublock_in, p_docu);
                break;
            }
        }
        else
        {
            DATA_REF data_ref = *p_data_ref; /* set up a data_ref for this field */
            ARRAY_HANDLE_USTR h_text_ustr;
            S32 modified;

            set_field_id_for_rec_data_ref(&data_ref, p_fielddef->id);

            status = rec_cache_enquire(p_docu, &data_ref, &h_text_ustr, &modified);

            if(status_done(status) && modified)
            {
                QUICK_UBLOCK quick_ublock;
                quick_ublock_setup_using_array(&quick_ublock, h_text_ustr);

                switch(p_fielddef->type)
                {
                case FIELD_FILE:
                case FIELD_PICTURE: /* actually binary content here */
                    status = rec_change_overwrite_binary(p_editrec, &quick_ublock, t5_filetype);
                    break;

                case FIELD_TEXT:
                    status = rec_change_overwrite_text(p_editrec, &quick_ublock);
                    break;

                default: default_unhandled();
#if CHECKING
                case FIELD_INTEGER:
                case FIELD_DATE:
                case FIELD_INTERVAL:
                case FIELD_REAL:
                case FIELD_FORMULA:
                case FIELD_BOOL:
#endif
                    status = rec_change_overwrite_eval(p_editrec, &quick_ublock, p_docu);
                    break;
                }

                rec_cache_modified(p_docu, &data_ref, 0);
            }
        }
    }

    return(status);
}

/* Write the given text back to the database in the field/record specified by the data ref */

_Check_return_
extern STATUS
rec_object_write_text(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DATA_REF p_data_ref,
    _InRef_     PC_QUICK_UBLOCK p_quick_ublock,
    _InVal_     T5_FILETYPE t5_filetype)
{
    P_REC_PROJECTOR p_rec_projector = p_rec_projector_from_data_ref(p_docu, p_data_ref);

    if(DATA_DB_TITLE == p_data_ref->data_space)
    {
        /* changing a field title */
        FIELD_ID field_id = field_id_from_rec_data_ref(p_data_ref);
        P_REC_FRAME p_rec_frame = p_rec_frame_from_field_id(&p_rec_projector->h_rec_frames, field_id);
        STATUS status;

        if(P_DATA_NONE != p_rec_frame)
        {
            al_array_dispose(&p_rec_frame->h_title_text_ustr);
            status = al_ustr_set(&p_rec_frame->h_title_text_ustr, quick_ublock_ustr(p_quick_ublock));

            /* SKS 22jun95 changing one field title needs to change it in all the visible cards */
            rec_update_projector_adjust(p_rec_projector);
        }
        else
            status = STATUS_OK;

        return(status);
    }

    myassert1x(p_data_ref->data_space == DATA_DB_FIELD, TEXT("Not a DATA_DB_FIELD ") S32_TFMT, p_data_ref->data_space);

    {
    P_FIELDDEF p_fielddef = p_fielddef_from_data_ref(p_docu, p_data_ref);

    if(P_DATA_NONE == p_fielddef)
        return(STATUS_FAIL);

    if(!p_fielddef->writeable)
        return(STATUS_FAIL);
    } /*block*/

    /* We must read in the whole record, modify the field and write back to whole record */

    /* call record_view to make it more valid */
    status_return(record_view_recnum(&p_rec_projector->opendb, get_record_from_rec_data_ref(p_data_ref)));

    if(p_rec_projector->opendb.recordspec.ncards <= 0)
        /* Zero records so you can't edit... You'll have to add one first */
        return(STATUS_FAIL);

    if(p_rec_projector->opendb.recordspec.recno < 0)
        /* No record of that record number You'll have to add it */
        return(STATUS_FAIL);

    {
    S32 old_ncards = p_rec_projector->opendb.recordspec.ncards;
    S32 old_recno  = p_rec_projector->opendb.recordspec.recno;
    STATUS status;

    /* At this point we do the optimistic lock... */
    status = record_edit_commence(&p_rec_projector->opendb);

    if(status_fail(status))
        status = create_error(REC_ERR_RECORD_LOCKED);
    else
    {
        /* OK we got the lock so do it */
        S32 new_ncards;
        S32 new_recno;

        status = rec_incorporate_changes_eat(p_docu, &p_rec_projector->opendb, p_data_ref, p_quick_ublock, t5_filetype);

        /* This left the data attached to the p_opendb in the resocspec.h_handles array */

        if(status_ok(status)) /* SKS 22jun95 placed here from subroutine 'cos of naff error handling */
            status = record_view_recnum(&p_rec_projector->opendb, get_record_from_rec_data_ref(p_data_ref));

        if(status_ok(status))
            status = record_edit_confirm(&p_rec_projector->opendb, FALSE, FALSE);

        if(status_fail(status))
            (void) record_edit_cancel(&p_rec_projector->opendb, FALSE);

        (void) free_recordspec_handle_table(&p_rec_projector->opendb.recordspec);

        if(status_fail(record_current(&p_rec_projector->opendb, &new_recno, &new_ncards)))
            new_recno = -1;
        else
        {
            /* This is not enough...
               The record may no longer be part of the active query
               or the sort order may have moved them all about
            */
            if((new_recno == old_recno) && (new_ncards == old_ncards))
                rec_update_data_ref(p_rec_projector, p_data_ref);
            else
            {
                trace_0(TRACE_APP_DPLIB, TEXT("rec_write_text_to_field : changed"));
                rec_update_projector_adjust(p_rec_projector);
            }
        }
    }

    if(status_fail(status))
    {
        reperr_null(status);
        rec_update_data_ref(p_rec_projector, p_data_ref);
        status = STATUS_FAIL;
    }

    return(status);
    } /*block*/
}

/* end of rcbuf.c */
