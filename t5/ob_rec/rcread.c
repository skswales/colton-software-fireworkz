/* rcread.c */

/* Copyright (C) 1994-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#include "common/gflags.h"

#include "ob_rec/ob_rec.h"

_Check_return_
extern STATUS
rec_read_resource(
    /*out*/ P_REC_RESOURCE p_rec_resource,
    P_REC_PROJECTOR p_rec_projector,
    _InRef_     PC_DATA_REF p_data_ref,
    _InVal_     BOOL plain)
{
    STATUS status = STATUS_OK;
    P_FIELDDEF p_fielddef = p_fielddef_from_field_id(&p_rec_projector->opendb.table.h_fielddefs, field_id_from_rec_data_ref(p_data_ref));

    zero_struct_ptr(p_rec_resource);

    ev_data_set_blank(&p_rec_resource->ev_data); /* SKS 15apr96 be pedantic */

    if(P_DATA_NONE == p_fielddef)
        return(STATUS_FAIL);

    /* Notice the type of data_ref here */
    if(p_data_ref->data_space == DATA_DB_TITLE)
    {
        reperr_null(ERR_NYI);

        if(status_ok(status = ss_string_make_ustr(&p_rec_resource->ev_data, p_fielddef->name)))
            p_rec_resource->t5_filetype = FILETYPE_TEXT;
    }
    else
    {
        /* Put the current cursor to the correct position and get the data */
        S32 record = get_record_from_rec_data_ref(p_data_ref);

        if(status_ok(status = ensure_cursor_current(&p_rec_projector->opendb, record)))
        {
            if(record <= p_rec_projector->opendb.recordspec.ncards)
                status = field_get_value(p_rec_resource, p_fielddef, plain);
            else
                status = STATUS_FAIL;
        }
     }

    return(status);
}

/******************************************************************************
*
* convert a cell to its output text
*
******************************************************************************/

_Check_return_ _Success_(status_ok(return))
extern STATUS
rec_object_convert_to_output_text(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock               /*appended,WRONGLY terminated*/ /* text out */,
    _OutRef_    P_PIXIT p_pixit_text_width                  /* width of text out */,
    _OutRef_    P_PIXIT p_pixit_text_height                 /* height of text out */,
    _OutRef_    P_PIXIT p_pixit_base_line                   /* base line position out */,
    _OutRef_opt_ P_FONTY_HANDLE p_fonty_handle              /* fonty handle out */,
    _InRef_     PC_STYLE p_style                            /* style in */,
    P_REC_PROJECTOR p_rec_projector,
    _InRef_     PC_DATA_REF p_data_ref                      /* Who am it really in */)
{
    STATUS status;
    P_DOCU p_docu;

    status_return(rec_text_from_data_ref(p_quick_ublock, p_style, p_rec_projector, p_data_ref));

    p_docu = p_docu_from_docno(p_rec_projector->docno);

    if(status_ok(status = fonty_handle_from_font_spec(&p_style->font_spec, p_docu->flags.draft_mode)))
    {
        const PC_UCHARS uchars = quick_ublock_uchars(p_quick_ublock);
        const U32 uchars_n = quick_ublock_bytes(p_quick_ublock);
        const FONTY_HANDLE fonty_handle = status;
        FONTY_CHUNK fonty_chunk;

        if(NULL != p_fonty_handle)
            *p_fonty_handle = fonty_handle;

        fonty_chunk_info_read_uchars(p_docu, &fonty_chunk, fonty_handle, uchars, uchars_n, 0 /* trail_spaces */);

        *p_pixit_text_width = fonty_chunk.width;
        *p_pixit_text_height = style_leading_from_style(p_style, &p_style->font_spec, p_docu->flags.draft_mode);
        *p_pixit_base_line = fonty_base_line(leading, p_style->font_spec.size_y, fonty_chunk.ascent);
    }

    return(status);
}

/******************************************************************************
*
* get the text for an object and
* pass it through numform
*
******************************************************************************/

_Check_return_
extern STATUS
rec_text_from_data_ref(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended,WRONGLY terminated*/ /* text out */,
    _InRef_     PC_STYLE p_style,
    P_REC_PROJECTOR p_rec_projector,
    _InRef_     PC_DATA_REF p_data_ref)
{
    REC_RESOURCE rec_resource;
    STATUS status;

    if(status_done(status = rec_read_resource(&rec_resource, p_rec_projector, p_data_ref, TRUE)))
    {
        NUMFORM_PARMS numform_parms;
        /*zero_struct(numform_parms);*/
        numform_parms.ustr_numform_numeric   = array_ustr(&p_style->para_style.h_numform_nu);
        numform_parms.ustr_numform_datetime  = array_ustr(&p_style->para_style.h_numform_dt);
        numform_parms.ustr_numform_texterror = array_ustr(&p_style->para_style.h_numform_se);
        numform_parms.p_numform_context = get_p_numform_context(p_docu_from_docno(p_rec_projector->docno));
        if(status_ok(status = numform(p_quick_ublock, P_QUICK_TBLOCK_NONE, &rec_resource.ev_data, &numform_parms)))
            status = STATUS_DONE;
    }
    else if(status_ok(status))
        /* The p_quick_block needs to return 'null' */
        status = STATUS_DONE;

    ss_data_free_resources(&rec_resource.ev_data);

    return(status);
}

/* end of rcread.c */
