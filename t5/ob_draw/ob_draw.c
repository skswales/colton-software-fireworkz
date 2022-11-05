/* ob_draw.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Drawfile (and other image file) objects for Fireworkz */

/* RCM June 1992 */

#include "common/gflags.h"

#include "ob_draw/ob_draw.h"

#include "ob_skel/ff_io.h"

#include "cmodules/gr_diag.h"

#include "cmodules/collect.h"

#if WINDOWS
#include "ob_skel/ho_gdip_image.h"
#endif

/*
internal structure
*/

#define DRAWFILE_EXTENSION_TSTR FILE_EXT_SEP_TSTR TEXT("aff")

enum DRAWING_INFO_TAG
{
    DRAWING_EMBEDDED = 0,
    DRAWING_REFERENCE
};

typedef struct DRAWING_INFO
{
    enum DRAWING_INFO_TAG tag;

    PTSTR filename; /* either referenced filename, or the original filename, if embedded */

    T5_FILETYPE t5_filetype; /* of the original image file data */
    P_USTR ustr_type_name;
#define USTR_TYPE_NAME_RISCOS USTR_TEXT("RISC OS")
    /* for DrawFileReference actually means 'can be easily loaded on RISC OS Fireworkz without conversion' (Draw,Sprite,JPEG) but NOT for DrawFileEmbdedded where older versions would throw a fit */

    ARRAY_HANDLE h_unhandled_data;

    BOOL use_image_cache_handle;
    IMAGE_CACHE_HANDLE image_cache_handle;
    BOOL image_cache_handle_is_draw_representation; /* i.e. not the original */
}
DRAWING_INFO, * P_DRAWING_INFO;

typedef struct DRAW_LOAD_INSTANCE
{
    ARRAY_HANDLE h_mapping_list;
}
DRAW_LOAD_INSTANCE, * P_DRAW_LOAD_INSTANCE, ** P_P_DRAW_LOAD_INSTANCE;

typedef struct DRAW_LOAD_MAP
{
    S32         extref;
    ARRAY_INDEX intref; /* index into h_drawing_list */
}
DRAW_LOAD_MAP, * P_DRAW_LOAD_MAP; typedef const DRAW_LOAD_MAP * PC_DRAW_LOAD_MAP;

typedef struct DRAW_SAVE_INSTANCE
{
    ARRAY_HANDLE h_mapping_list;
}
DRAW_SAVE_INSTANCE, * P_DRAW_SAVE_INSTANCE;

typedef struct DRAW_SAVE_MAP
{
    P_ANY object_data_ref; /* index into h_drawing_list; extref is index in h_mapping_list */
}
DRAW_SAVE_MAP, * P_DRAW_SAVE_MAP;

#if RISCOS
#if defined(BOUND_MESSAGES_OBJECT_ID_DRAW)
extern PC_U8 rb_draw_msg_weak;
#define P_BOUND_MESSAGES_OBJECT_ID_DRAW &rb_draw_msg_weak
#else
#define P_BOUND_MESSAGES_OBJECT_ID_DRAW LOAD_MESSAGES_FILE
#endif
#else
#define P_BOUND_MESSAGES_OBJECT_ID_DRAW DONT_LOAD_MESSAGES_FILE
#endif

#define P_BOUND_RESOURCES_OBJECT_ID_DRAW DONT_LOAD_RESOURCES

/*
construct argument types
*/

static ARG_TYPE
args_image_file_embedded[] = /* new for 2.01 */
{
    ARG_TYPE_S32  | ARG_MANDATORY, /* extref */
    ARG_TYPE_USTR | ARG_MANDATORY_OR_BLANK, /* Image type_name (0xABC) */
    ARG_TYPE_RAW_DS | ARG_MANDATORY, /* Image file data */
    ARG_TYPE_TSTR | ARG_OPTIONAL, /* original leafname */
    ARG_TYPE_NONE
};

static ARG_TYPE
args_drawfile_embedded[] =
{
    ARG_TYPE_S32  | ARG_MANDATORY, /* extref */
    ARG_TYPE_USTR | ARG_MANDATORY_OR_BLANK, /* Draw type_name */
    ARG_TYPE_RAW_DS | ARG_MANDATORY, /* Draw file data */ /* for 2.01, this is usually best representation for backwards compatibility */
    ARG_TYPE_TSTR | ARG_OPTIONAL, /* original leafname */
    ARG_TYPE_NONE
};

static ARG_TYPE
args_drawfile_reference[] = /* really ImageFileReference but try very hard for backwards/forwards compatibility */
{
    ARG_TYPE_S32  | ARG_MANDATORY, /* extref */
    ARG_TYPE_USTR | ARG_MANDATORY_OR_BLANK, /* type_name */
    ARG_TYPE_TSTR | ARG_MANDATORY, /* filename */
    ARG_TYPE_NONE
};

/*
construct table
*/

static CONSTRUCT_TABLE
object_construct_table[] =
{                                                                                                   /*   fi ti mi ur up xi md mf nn cp sm ba fo */

    { "ImageFileEmbedded",      args_image_file_embedded,   T5_CMD_IMAGE_FILE_EMBEDDED,                 { 0, 0, 0, 0, 0, 0, 0, 1, 0 } },
    { "DrawFileEmbedded",       args_drawfile_embedded,     T5_CMD_DRAWFILE_EMBEDDED,                   { 0, 0, 0, 0, 0, 0, 0, 1, 0 } },
    { "DrawFileReference",      args_drawfile_reference,    T5_CMD_DRAWFILE_REFERENCE,                  { 0, 0, 0, 0, 0, 0, 0, 1, 0 } },

    { NULL,                     NULL,                       T5_EVENT_NONE } /* end of table */
};

static void
type_name_from_filetype(
    _Out_writes_z_(elemof_buffer) P_USTR buffer,
    _InVal_     U32 elemof_buffer,
    _InVal_     T5_FILETYPE t5_filetype,
    _InVal_     BOOL strict_for_embed)
{
    if(!strict_for_embed)
    {
        switch(t5_filetype)
        {
        case FILETYPE_SPRITE:
        case FILETYPE_JPEG:
            ustr_xstrkpy(buffer, elemof_buffer, USTR_TYPE_NAME_RISCOS);
            return;

        default:
            break;
        }
    }

    switch(t5_filetype)
    {
    case FILETYPE_DRAW:
        ustr_xstrkpy(buffer, elemof_buffer, USTR_TYPE_NAME_RISCOS);
        break;

    default:
        consume_int(ustr_xsnprintf(buffer, elemof_buffer, USTR_TEXT("0x%03X"), (int) t5_filetype));
        break;
    }
}

_Check_return_
static STATUS
ensure_draw_load_instance(
    _InRef_     P_OF_IP_FORMAT p_of_ip_format,
    _OutRef_    P_P_DRAW_LOAD_INSTANCE p_p_draw_load_instance)
{
    P_DRAW_LOAD_INSTANCE p_draw_load_instance = collect_goto_item(DRAW_LOAD_INSTANCE, &p_of_ip_format->object_data_list, OBJECT_ID_DRAW);
    STATUS status = STATUS_OK;

    if(NULL == p_draw_load_instance)
    {
        DRAW_LOAD_INSTANCE draw_load_instance;

        draw_load_instance.h_mapping_list = 0;

        p_draw_load_instance = collect_add_entry_elem(DRAW_LOAD_INSTANCE, &p_of_ip_format->object_data_list, &draw_load_instance, OBJECT_ID_DRAW, &status);
    }

    *p_p_draw_load_instance = p_draw_load_instance;

    return(status);
}

T5_CMD_PROTO(static, draw_file_load)
{
    const P_DRAW_INSTANCE_DATA p_draw_instance_data = p_object_instance_data_DRAW(p_docu);
    const P_OF_IP_FORMAT p_of_ip_format = p_t5_cmd->p_of_ip_format;
    P_DRAW_LOAD_INSTANCE p_draw_load_instance = NULL;
    P_DRAW_LOAD_MAP p_draw_load_map = NULL;
    P_DRAWING_INFO p_drawing_info = NULL;
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(*p_drawing_info), TRUE);
    BOOL subsequent_draw_data = FALSE;
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 3); /* NB just the common span */
    const S32 extref = p_args[0].val.s32;
    PC_USTR ustr_type_name = p_args[1].val.ustr; /* for referenced files we always ignore the incoming one now but older versions didn't */
    UCHARZ type_name_buffer[8];
    const PCTSTR filename = (T5_CMD_DRAWFILE_REFERENCE == t5_message) ? p_args[2].val.tstr : NULL;
    T5_FILETYPE t5_filetype;
    STATUS status = STATUS_NOMEM;

    status_return(ensure_draw_load_instance(p_of_ip_format, &p_draw_load_instance));

    if(T5_CMD_DRAWFILE_EMBEDDED == t5_message)
    {   /* is there already an entry created by an earlier ImageFileEmbedded? */
        ARRAY_INDEX i;

        for(i = 0; i < array_elements(&p_draw_load_instance->h_mapping_list); ++i)
        {
            P_DRAW_LOAD_MAP p_draw_load_map_test = array_ptr(&p_draw_load_instance->h_mapping_list, DRAW_LOAD_MAP, i);

            if(extref == p_draw_load_map_test->extref)
            {
                P_DRAWING_INFO p_drawing_info_test = array_ptr(&p_draw_instance_data->h_drawing_list, DRAWING_INFO, p_draw_load_map_test->intref);

                if(DRAWING_EMBEDDED == p_drawing_info_test->tag)
                {
                    p_draw_load_map = p_draw_load_map_test;
                    p_drawing_info = p_drawing_info_test;
                    if(p_drawing_info->use_image_cache_handle)
                    {   /* the earlier ImageFileEmbedded worked well, so ignore this subsequent DrawFileEmbedded */
                        return(STATUS_OK);
                    }
                    /* the earlier ImageFileEmbedded did not work well, so use the Draw data from this DrawFileEmbedded */
                    assert(0 != p_drawing_info->h_unhandled_data);
                    subsequent_draw_data = TRUE;
                    break;
                }
            }
        }
    }

    for(;;) /* loop for structure */
    {
        if(NULL == p_drawing_info)
        {
            if(NULL == (p_drawing_info = al_array_extend_by(&p_draw_instance_data->h_drawing_list, DRAWING_INFO, 1, &array_init_block, &status)))
                return(status);
        }

        if(NULL == p_draw_load_map)
        {
            SC_ARRAY_INIT_BLOCK array_init_block_draw_load_map = aib_init(1, sizeof32(*p_draw_load_map), TRUE);

            if(NULL == (p_draw_load_map = al_array_extend_by(&p_draw_load_instance->h_mapping_list, DRAW_LOAD_MAP, 1, &array_init_block_draw_load_map, &status)))
                break;

            p_draw_load_map->extref = extref;

            p_draw_load_map->intref = array_indexof_element(&p_draw_instance_data->h_drawing_list, DRAWING_INFO, p_drawing_info);
        }

        if(T5_CMD_DRAWFILE_REFERENCE == t5_message)
        {
            BOOL file_status;

            p_drawing_info->tag = DRAWING_REFERENCE;

            PTR_ASSERT(filename);
            status_break(status = alloc_block_tstr_set(&p_drawing_info->filename, filename, &p_docu->general_string_alloc_block))

            file_status = file_is_file(filename);

            if(file_status <= 0)
            {   /* report errors locally and keep loading the file, dependent notes OK */
                reperr(ERR_NOTFOUND_REFERENCED_PICTURE, filename);

                t5_filetype = FILETYPE_UNDETERMINED;

                ustr_type_name = USTR_TYPE_NAME_RISCOS; /* try to reload the next time if saved and then presented to an older version */
            }
            else
            {
                t5_filetype = t5_filetype_from_filename(filename);

                type_name_from_filetype(ustr_bptr(type_name_buffer), elemof32(type_name_buffer), t5_filetype, FALSE /*strict_for_embed*/);

                ustr_type_name = ustr_bptr(type_name_buffer);
            }

            p_drawing_info->t5_filetype = t5_filetype; /* remember the type of image file loaded this time (may vary!) */

            status_break(status = alloc_block_ustr_set(&p_drawing_info->ustr_type_name, ustr_type_name, &p_docu->general_string_alloc_block));

            if(image_cache_can_import_with_image_convert(t5_filetype))
            {
                status_break(status = image_cache_entry_ensure(&p_drawing_info->image_cache_handle, filename, t5_filetype));

                p_drawing_info->use_image_cache_handle = TRUE;

                if(file_status > 0)
                {
                    const ARRAY_HANDLE h_data = image_cache_loaded_ensure(p_drawing_info->image_cache_handle);

                    if(0 == h_data)
                    {   /* report errors locally and keep loading the file, dependent notes OK */
                        reperr(image_cache_error_query(p_drawing_info->image_cache_handle), filename);
                    }
                }
            }
        }
        else /* DRAWING_EMBEDDED */
        {
            const P_ARGLIST_ARG p_arg_2 = de_const_cast(P_ARGLIST_ARG, &p_args[2]); /* Draw or Image file data */
            P_ARGLIST_ARG p_arg_3;

            if(subsequent_draw_data)
            {
                /* try embedding the supplied Draw data representation so the user might see something */
                if(status_ok(image_cache_embedded(&p_drawing_info->image_cache_handle, &p_arg_2->val.raw, FILETYPE_DRAW)))
                {
                    p_drawing_info->use_image_cache_handle = TRUE;
                    p_drawing_info->image_cache_handle_is_draw_representation = TRUE;
                }
                /* otherwise can't embed the Draw data either */ /* keep loading the file, dependent notes OK */
            }
            else
            {
                p_drawing_info->tag = DRAWING_EMBEDDED;

                if(arg_present(&p_t5_cmd->arglist_handle, 3, &p_arg_3))
                {   /* SKS 13.10.99 preserve original embedded picture's filename */
                    PCTSTR tstr_original_filename = p_arg_3->val.tstr;
                    PTR_ASSERT(tstr_original_filename);
                    status_break(status = alloc_block_tstr_set(&p_drawing_info->filename, tstr_original_filename, &p_docu->general_string_alloc_block))
                }
                else
                {
                    p_drawing_info->filename = tstr_empty_string;
                }

                if((PtrGetByte(ustr_type_name) == '0') && (PtrGetByteOff(ustr_type_name, 1) == 'x'))
                {   /* Fireworkz can't accurately discriminate between all filetype variants */
                    /* hex filetype from command dominates */
                    t5_filetype = (T5_FILETYPE) strtoul(PtrAddBytes(const char *, ustr_type_name, 2), NULL, 16);
                }
                else
                {
                    if(ustr_compare_equals(ustr_type_name, USTR_TYPE_NAME_RISCOS))
                    {   /* pre-2.01 only allowed Draw files to be embedded */
                        t5_filetype = FILETYPE_DRAW;

                        ustr_type_name = USTR_TYPE_NAME_RISCOS;
                    }
                    else
                    {   /* this way you can set the type name blank to let Fireworkz have another go at it */
                        t5_filetype = t5_filetype_from_data(array_rangec(&p_arg_2->val.raw, BYTE, 0, array_elements32(&p_arg_2->val.raw)), array_elements32(&p_arg_2->val.raw));

                        type_name_from_filetype(ustr_bptr(type_name_buffer), elemof32(type_name_buffer), t5_filetype, TRUE /*strict_for_embed*/);

                        ustr_type_name = ustr_bptr(type_name_buffer);
                    }
                }

                p_drawing_info->t5_filetype = t5_filetype; /* remember the type of image data */

                status_break(status = alloc_block_ustr_set(&p_drawing_info->ustr_type_name, ustr_type_name, &p_docu->general_string_alloc_block));

                if(image_cache_can_import_with_image_convert(t5_filetype))
                {
                    if(status_ok(image_cache_embedded(&p_drawing_info->image_cache_handle, &p_arg_2->val.raw, t5_filetype)))
                    {
                        p_drawing_info->use_image_cache_handle = TRUE;
                    }
                    /* otherwise can't embed the Image data */ /* keep loading the file, dependent notes OK */
                }

                if(!p_drawing_info->use_image_cache_handle)
                {   /* either the Image file data was not importable on this system or the embed failed */
                    /* simply steal what we have been passed in and keep it around safely for the save */
                    p_drawing_info->h_unhandled_data = p_arg_2->val.raw;
                    p_arg_2->val.raw = 0; /* yum yum */
                }
            }
        }

        if(p_drawing_info->use_image_cache_handle)
        {
            image_cache_ref(p_drawing_info->image_cache_handle, 1);
        }

        return(STATUS_OK);
        /*NOTREACHED*/
    }

    if(NULL != p_drawing_info)
        al_array_shrink_by(&p_draw_instance_data->h_drawing_list, -1);

    if(NULL != p_draw_load_map)
        al_array_shrink_by(&p_draw_load_instance->h_mapping_list, -1);

    /* don't care about the load instance data, he'll be freed at load process end (and may already have valid entries) */

    /* report errors locally and keep loading the file, but dependent notes should blow */
    if(filename)
        reperr(status, filename);
    else
        reperr_null(status);

    return(STATUS_OK);
}

T5_MSG_PROTO(static, draw_msg_note_delete, /*_Inout_*/ P_ANY p_data)
{
    const P_DRAW_INSTANCE_DATA p_draw_instance_data = p_object_instance_data_DRAW(p_docu);
    ARRAY_INDEX intref = (ARRAY_INDEX) (intptr_t) p_data; /* position in h_drawing_list */

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(array_index_is_valid(&p_draw_instance_data->h_drawing_list, intref))
    {
        const P_DRAWING_INFO p_drawing_info = array_ptr_no_checks(&p_draw_instance_data->h_drawing_list, DRAWING_INFO, intref);

        if(p_drawing_info->use_image_cache_handle)
        {
            p_drawing_info->use_image_cache_handle = FALSE;

            image_cache_ref(p_drawing_info->image_cache_handle, 0);
        }

        al_array_dispose(&p_drawing_info->h_unhandled_data);
    }

    return(STATUS_OK);
}

T5_MSG_PROTO(static, draw_note_donate, P_NOTE_OBJECT_SNAPSHOT p_note_object_snapshot)
{
    const P_DRAW_INSTANCE_DATA p_draw_instance_data = p_object_instance_data_DRAW(p_docu);
    const P_GR_DIAG p_gr_diag = (P_GR_DIAG) p_note_object_snapshot->p_more_info;
    P_GR_RISCDIAG p_gr_riscdiag = p_gr_diag->p_gr_riscdiag;
    P_DRAWING_INFO p_drawing_info;
    ARRAY_HANDLE array_handle = 0;
    STATUS status = STATUS_OK;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    {
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(*p_drawing_info), TRUE);
    if(NULL == (p_drawing_info = al_array_extend_by(&p_draw_instance_data->h_drawing_list, DRAWING_INFO, 1, &array_init_block, &status)))
        return(status);
    } /*block*/

    for(;;) /* loop for structure */
    {
        {
        PC_DRAW_FILE_HEADER pDrawFileHdr;
        if(NULL != (pDrawFileHdr = gr_riscdiag_diagram_lock(p_gr_riscdiag)))
        {
            SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(BYTE), FALSE);
            status = al_array_add(&array_handle, BYTE, p_gr_riscdiag->draw_diag.length, &array_init_block, pDrawFileHdr);
            gr_riscdiag_diagram_unlock(p_gr_riscdiag);
            status_break(status);
        }
        } /*block*/

        p_drawing_info->tag = DRAWING_EMBEDDED;

        status_break(status = alloc_block_ustr_set(&p_drawing_info->ustr_type_name, USTR_TYPE_NAME_RISCOS, &p_docu->general_string_alloc_block));

        status_break(status = image_cache_embedded(&p_drawing_info->image_cache_handle, &array_handle, FILETYPE_DRAW));

        p_drawing_info->use_image_cache_handle = TRUE;

        image_cache_ref(p_drawing_info->image_cache_handle, 1);

        {
        NOTE_UPDATE_OBJECT_INFO note_update_object_info;
        note_update_object_info.p_note_info = p_note_object_snapshot->p_note_info;
        note_update_object_info.object_id = OBJECT_ID_DRAW;
        note_update_object_info.object_data_ref = (P_ANY) (intptr_t) array_indexof_element(&p_draw_instance_data->h_drawing_list, DRAWING_INFO, p_drawing_info);
        status_assert(object_call_id(OBJECT_ID_NOTE, p_docu, T5_MSG_NOTE_UPDATE_OBJECT_INFO, &note_update_object_info));
        } /*block*/

        break; /* out of loop for structure */
    }

    al_array_dispose(&array_handle);

    if(status_fail(status))
        al_array_shrink_by(&p_draw_instance_data->h_drawing_list, -1);

    return(status);
}

T5_MSG_PROTO(static, draw_msg_note_object_size_query, P_NOTE_OBJECT_SIZE p_note_object_size)
{
    const P_DRAW_INSTANCE_DATA p_draw_instance_data = p_object_instance_data_DRAW(p_docu);
    const P_DRAWING_INFO p_drawing_info = array_ptr(&p_draw_instance_data->h_drawing_list, DRAWING_INFO, (ARRAY_INDEX) (intptr_t) p_note_object_size->object_data_ref);

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(p_drawing_info->use_image_cache_handle)
    {
        ARRAY_HANDLE h_data;

#if WINDOWS
        GdipImage gdip_image = image_cache_search_gdip_image(p_drawing_info->image_cache_handle, &h_data);

        if(NULL != gdip_image)
        {
            SIZE image_size;
            GdipImage_GetImageSize(gdip_image, &image_size);
            p_note_object_size->pixit_size.cx = image_size.cx;
            p_note_object_size->pixit_size.cy = image_size.cy;
        }
        else
#endif /* OS */
        {   /* must be a Draw file */
            h_data = image_cache_search(p_drawing_info->image_cache_handle, FALSE);

            if(array_elements32(&h_data) >= sizeof32(DRAW_FILE_HEADER))
                host_read_drawfile_pixit_size(array_basec(&h_data, BYTE), &p_note_object_size->pixit_size);
        }

        p_note_object_size->processed = 1;
    }

    return(STATUS_OK);
}

T5_MSG_PROTO(static, draw_msg_note_load_intref_from_extref, P_NOTE_REF p_note_ref)
{
    const P_DRAW_LOAD_INSTANCE p_draw_load_instance = collect_goto_item(DRAW_LOAD_INSTANCE, &p_note_ref->p_of_ip_format->object_data_list, OBJECT_ID_DRAW);

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(NULL != p_draw_load_instance)
    {
        const ARRAY_INDEX mapping_list_elements = array_elements(&p_draw_load_instance->h_mapping_list);
        ARRAY_INDEX i;
        PC_DRAW_LOAD_MAP p_draw_load_map = array_rangec(&p_draw_load_instance->h_mapping_list, DRAW_LOAD_MAP, 0, mapping_list_elements);

        for(i = 0; i < mapping_list_elements; ++i, ++p_draw_load_map)
        {
            if(p_draw_load_map->extref != p_note_ref->extref)
                continue;

            p_note_ref->object_data_ref = (P_ANY) (uintptr_t) p_draw_load_map->intref;
            return(STATUS_OK);
        }
    }

    return(STATUS_FAIL);
}

T5_MSG_PROTO(static, draw_msg_note_ensure_embedded, P_NOTE_ENSURE_EMBEDDED p_note_ensure_embedded)
{
    const P_DRAW_INSTANCE_DATA p_draw_instance_data = p_object_instance_data_DRAW(p_docu);
    const P_DRAWING_INFO p_drawing_info = array_ptr(&p_draw_instance_data->h_drawing_list, DRAWING_INFO, (ARRAY_INDEX) (intptr_t) p_note_ensure_embedded->object_data_ref);

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(p_drawing_info->tag == DRAWING_REFERENCE)
    {
        if(p_drawing_info->use_image_cache_handle)
        {
            const ARRAY_HANDLE h_data = image_cache_loaded_ensure(p_drawing_info->image_cache_handle);

            if(0 == h_data)
                return(image_cache_error_query(p_drawing_info->image_cache_handle));

            p_drawing_info->tag = DRAWING_EMBEDDED;
        }
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
draw_msg_note_save_reference(
    _InRef_     P_DRAWING_INFO p_drawing_info,
    _InRef_     P_NOTE_ENSURE_SAVED p_note_ensure_saved,
    _InVal_     T5_MESSAGE t5_message)
{
    P_OF_OP_FORMAT p_of_op_format = p_note_ensure_saved->p_of_op_format;
    const OBJECT_ID object_id = OBJECT_ID_DRAW;
    PC_CONSTRUCT_TABLE p_construct_table;
    ARGLIST_HANDLE arglist_handle;
    STATUS status;

    if(status_ok(status = arglist_prepare_with_construct(&arglist_handle, object_id, t5_message, &p_construct_table)))
    {
        const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 3);

        PTR_ASSERT(p_drawing_info->ustr_type_name);

        p_args[0].val.s32 = p_note_ensure_saved->extref;
        p_args[1].val.ustr = p_drawing_info->ustr_type_name;
        p_args[2].val.tstr = localise_filename(p_of_op_format->output.u.file.filename, p_drawing_info->filename);

        status = ownform_save_arglist(arglist_handle, object_id, p_construct_table, p_of_op_format);

        arglist_dispose(&arglist_handle);
    }

    return(status);
}

_Check_return_
static STATUS
draw_msg_note_save_embedded(
    _InRef_     P_DRAWING_INFO p_drawing_info,
    _InRef_     P_NOTE_ENSURE_SAVED p_note_ensure_saved,
    _InVal_     T5_MESSAGE t5_message,
    _In_z_      PC_USTR ustr_type_name,
    _InVal_     ARRAY_HANDLE h_data)
{
    P_OF_OP_FORMAT p_of_op_format = p_note_ensure_saved->p_of_op_format;
    const OBJECT_ID object_id = OBJECT_ID_DRAW;
    PC_CONSTRUCT_TABLE p_construct_table;
    ARGLIST_HANDLE arglist_handle;
    STATUS status;

    if(status_ok(status = arglist_prepare_with_construct(&arglist_handle, object_id, t5_message, &p_construct_table)))
    {
        const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 4);

        p_args[0].val.s32 = p_note_ensure_saved->extref;
        p_args[1].val.ustr = ustr_type_name;
        p_args[2].val.raw = h_data;
        p_args[3].val.tstr = file_leafname(p_drawing_info->filename); /* only original leafname stored when embedded (covers handling a scrapfile's name) */

        status = ownform_save_arglist(arglist_handle, object_id, p_construct_table, p_of_op_format);

        p_args[2].val.raw = 0; /* reclaim handle */

        arglist_dispose(&arglist_handle);
    }

    return(status);
}

T5_MSG_PROTO(static, draw_msg_note_ensure_saved, P_NOTE_ENSURE_SAVED p_note_ensure_saved)
{
    STATUS status;
    P_OF_OP_FORMAT p_of_op_format = p_note_ensure_saved->p_of_op_format;
    const OBJECT_ID object_id = OBJECT_ID_DRAW;
    const P_DRAW_INSTANCE_DATA p_draw_instance_data = p_object_instance_data_DRAW(p_docu);
    P_DRAW_SAVE_INSTANCE p_draw_save_instance = collect_goto_item(DRAW_SAVE_INSTANCE, &p_of_op_format->object_data_list, (LIST_ITEMNO) object_id);
    P_DRAW_SAVE_MAP p_draw_save_map;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(NULL != p_draw_save_instance)
    {
        const ARRAY_INDEX mapping_list_elements = array_elements(&p_draw_save_instance->h_mapping_list);
        ARRAY_INDEX i;

        p_draw_save_map = array_range(&p_draw_save_instance->h_mapping_list, DRAW_SAVE_MAP, 0, mapping_list_elements);

        for(i = 0; i < mapping_list_elements; ++i, ++p_draw_save_map)
        {
            if(p_draw_save_map->object_data_ref != p_note_ensure_saved->object_data_ref)
                continue;

            /* already saved */
            p_note_ensure_saved->extref = i;
            return(STATUS_OK);
        }
    }
    else
    {
        DRAW_SAVE_INSTANCE draw_save_instance;

        draw_save_instance.h_mapping_list = 0;

        if(NULL == (p_draw_save_instance = collect_add_entry_elem(DRAW_SAVE_INSTANCE, &p_of_op_format->object_data_list, &draw_save_instance, (LIST_ITEMNO) object_id, &status)))
            return(status);
    }

    {
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(*p_draw_save_map), FALSE);

    if(NULL == (p_draw_save_map = al_array_extend_by(&p_draw_save_instance->h_mapping_list, DRAW_SAVE_MAP, 1, &array_init_block, &status)))
        return(status);
    } /*block*/

    p_draw_save_map->object_data_ref = p_note_ensure_saved->object_data_ref;

    p_note_ensure_saved->extref = array_indexof_element(&p_draw_save_instance->h_mapping_list, DRAW_SAVE_MAP, p_draw_save_map);

    {
    const P_DRAWING_INFO p_drawing_info = array_ptr(&p_draw_instance_data->h_drawing_list, DRAWING_INFO, (ARRAY_INDEX) (intptr_t) p_draw_save_map->object_data_ref);

    if(DRAWING_REFERENCE == p_drawing_info->tag)
    {
        status = draw_msg_note_save_reference(p_drawing_info, p_note_ensure_saved, T5_CMD_DRAWFILE_REFERENCE);
    }
    else
    {
        ARRAY_HANDLE h_image_data = 0;
        ARRAY_HANDLE h_draw_data = 0;

        if(FILETYPE_DRAW == p_drawing_info->t5_filetype)
        {   /* just save DrawFileEmbedded */
            assert(0 == p_drawing_info->h_unhandled_data);
            assert(p_drawing_info->use_image_cache_handle);
            if(p_drawing_info->use_image_cache_handle)
                h_draw_data = image_cache_search(p_drawing_info->image_cache_handle, TRUE); /* obtain original Draw data handle for output (saves don't degrade) */

            if(0 != h_draw_data)
                status = draw_msg_note_save_embedded(p_drawing_info, p_note_ensure_saved, T5_CMD_DRAWFILE_EMBEDDED, USTR_TYPE_NAME_RISCOS, h_draw_data);
        }
        else
        {   /* save ImageFileEmbdedded, followed by DrawFileEmbedded if possible */
            if(0 != p_drawing_info->h_unhandled_data)
            {
                h_image_data = p_drawing_info->h_unhandled_data;

                if(p_drawing_info->use_image_cache_handle)
                    h_draw_data = image_cache_search(p_drawing_info->image_cache_handle, TRUE); /* obtain original Draw representation handle for output (saves don't degrade) */
            }
            else
            {
                assert(p_drawing_info->use_image_cache_handle);
                if(p_drawing_info->use_image_cache_handle)
                {
                    h_image_data = image_cache_search(p_drawing_info->image_cache_handle, TRUE); /* obtain original Image data handle for output (saves don't degrade) */
                    h_draw_data = image_cache_search(p_drawing_info->image_cache_handle, FALSE); /* obtain current Draw representation handle for output */
                }
            }

            if(0 != h_image_data)
                status = draw_msg_note_save_embedded(p_drawing_info, p_note_ensure_saved, T5_CMD_IMAGE_FILE_EMBEDDED, p_drawing_info->ustr_type_name, h_image_data);

            if(status_ok(status) && (0 != h_draw_data))
                status = draw_msg_note_save_embedded(p_drawing_info, p_note_ensure_saved, T5_CMD_DRAWFILE_EMBEDDED, USTR_TYPE_NAME_RISCOS, h_draw_data);
        }
    }
    } /*block*/

    return(status);
}

/******************************************************************************
*
* Insert an image file into the notelayer at the given position.
*
* embed == TRUE causes the data to be loaded and embedded into the document
*               (a later save operation causes the file data to be saved as part
*                of the Fireworkz file within a DrawFileEmbedded command).
*
* embed == FALSE causes a reference to the data to be held in the document
*               (a later save operation causes the filename to be saved as part
*                of the Fireworkz file within a DrawFileReference command).
*
******************************************************************************/

_Check_return_
static STATUS
notelayer_insert_image_file(
    _DocuRef_   P_DOCU p_docu,
    P_SKEL_POINT p_skel_point,
    _In_z_      PCTSTR filename,
    _InVal_     T5_FILETYPE t5_filetype,
    _InVal_     BOOL embed_file)
{
    const P_DRAW_INSTANCE_DATA p_draw_instance_data = p_object_instance_data_DRAW(p_docu);
    P_DRAWING_INFO p_drawing_info;
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(*p_drawing_info), TRUE);
    UCHARZ type_name_buffer[8];
    STATUS status = STATUS_OK;

    if(NULL == (p_drawing_info = al_array_extend_by(&p_draw_instance_data->h_drawing_list, DRAWING_INFO, 1, &array_init_block, &status)))
        return(status);

    p_drawing_info->tag = embed_file ? DRAWING_EMBEDDED : DRAWING_REFERENCE;

    p_drawing_info->t5_filetype = t5_filetype; /* remember the type of image data */

    type_name_from_filetype(ustr_bptr(type_name_buffer), elemof32(type_name_buffer), t5_filetype, embed_file /*strict_for_embed*/);

    for(;;) /* loop for structure */
    {
        status_break(status = alloc_block_ustr_set(&p_drawing_info->ustr_type_name, ustr_bptr(type_name_buffer), &p_docu->general_string_alloc_block));

        status_break(status = alloc_block_tstr_set(&p_drawing_info->filename, filename, &p_docu->general_string_alloc_block));

        if(!embed_file)
        {
            ARRAY_HANDLE h_data;

            if(!image_cache_entry_query(&p_drawing_info->image_cache_handle, filename))
            {
                BOOL file_status = file_is_file(filename);

                if(file_status == 0)
                {
                    status = create_error(ERR_NOTFOUND_REFERENCED_PICTURE);
                    break;
                }

                status_break(status = image_cache_entry_ensure(&p_drawing_info->image_cache_handle, filename, t5_filetype));
            }

            h_data = image_cache_loaded_ensure(p_drawing_info->image_cache_handle);

            if(0 == h_data)
                status_break(status = image_cache_error_query(p_drawing_info->image_cache_handle));

            p_drawing_info->use_image_cache_handle = TRUE;

            image_cache_ref(p_drawing_info->image_cache_handle, 1);
        }
        else
        {
            /* create a new entry and load it, then test for equivalence with other embedded cache members */
            ARRAY_HANDLE h_data;

            status_break(image_cache_entry_ensure(&p_drawing_info->image_cache_handle, filename, t5_filetype));

            h_data = image_cache_loaded_ensure(p_drawing_info->image_cache_handle);

            if(0 == h_data)
                status_break(status = image_cache_error_query(p_drawing_info->image_cache_handle));

            image_cache_embedded_updating_entry(&p_drawing_info->image_cache_handle);

            p_drawing_info->use_image_cache_handle = TRUE;

            image_cache_ref(p_drawing_info->image_cache_handle, 1);
        }

        /* if there is a selection, replace all the selected notes with the new drawing */
        /* else create a one new note                                                   */

        {
        S32 intref = array_indexof_element(&p_draw_instance_data->h_drawing_list, DRAWING_INFO, p_drawing_info);
        PIXIT_SIZE drawing_pixit_size = { 0, 0 };
        ARRAY_HANDLE h_data;

#if WINDOWS
        GdipImage gdip_image = image_cache_search_gdip_image(p_drawing_info->image_cache_handle, &h_data);

        if(NULL != gdip_image)
        {
            SIZE image_size;
            GdipImage_GetImageSize(gdip_image, &image_size);
            drawing_pixit_size.cx = image_size.cx;
            drawing_pixit_size.cy = image_size.cy;
        }
        else
#endif /* OS */
        {   /* must be a Draw file */
            h_data = image_cache_search(p_drawing_info->image_cache_handle, FALSE);

            if(array_elements32(&h_data) >= sizeof32(DRAW_FILE_HEADER))
                host_read_drawfile_pixit_size(array_basec(&h_data, BYTE), &drawing_pixit_size);
        }

        /*if(selected note)
            replace selected note(p_docu, OBJECT_ID_DRAW, (P_ANY) intref);
        else */
        { /* create a new note, contents referenced by intref */
            NOTE_INFO note_info;
            SKEL_RECT skel_rect;
            RECT_FLAGS rect_flags;
            RECT_FLAGS_CLEAR(rect_flags);

            zero_struct_fn(note_info);

            note_info.layer = LAYER_CELLS_AREA_ABOVE; /* always insert into front layer */

            note_info.pixit_size = drawing_pixit_size; /* width and height */

            note_info.gr_scale_pair.x = GR_SCALE_ONE;
            note_info.gr_scale_pair.y = GR_SCALE_ONE;

#if 0
            if(p_docu->mark_info_cells.h_markers)
            {
                DOCU_AREA docu_area;

                docu_area_normalise(p_docu, &docu_area, p_docu_area_from_markers_first(p_docu));

                note_info.note_pinning = NOTE_PIN_CELLS_TWIN;

                region_from_two_slrs(&note_info.region, &docu_area.tl.slr, &docu_area.br.slr, 1);

                skel_point_from_slr_tl(p_docu, &skel_rect.tl, &note_info.region.tl);
                skel_point_normalise(p_docu, &skel_rect.tl, UPDATE_PANE_CELLS_AREA);

                skel_point_from_slr_tl(p_docu, &skel_rect.br, &note_info.region.br);
                skel_point_normalise(p_docu, &skel_rect.br, UPDATE_PANE_CELLS_AREA);
            }
            else
#endif
            {
                SLR tl_slr;
                SKEL_POINT slot_skel_point;

                note_info.note_pinning = NOTE_PIN_CELLS_SINGLE;

                status_assert(slr_owner_from_skel_point(p_docu, &tl_slr, &slot_skel_point, p_skel_point, ON_ROW_EDGE_GO_DOWN));
                region_from_two_slrs(&note_info.region, &tl_slr, &tl_slr, 1);

                /* if the cell is on the same page, calc the offset from the cell's tl, else leave offset as 0,0 */
                if((slot_skel_point.page_num.x == p_skel_point->page_num.x) && (slot_skel_point.page_num.y == p_skel_point->page_num.y))
                {
                    note_info.offset_tl.x = p_skel_point->pixit_point.x - slot_skel_point.pixit_point.x;
                    note_info.offset_tl.y = p_skel_point->pixit_point.y - slot_skel_point.pixit_point.y;
                }

                skel_point_from_slr_tl(p_docu, &skel_rect.tl, &note_info.region.tl);
                skel_rect.tl.pixit_point.x += note_info.offset_tl.x;
                skel_rect.tl.pixit_point.y += note_info.offset_tl.y;
                skel_point_normalise(p_docu, &skel_rect.tl, UPDATE_PANE_CELLS_AREA);

                skel_rect.br = skel_rect.tl;
                skel_rect.br.pixit_point.x += note_info.pixit_size.cx;
                skel_rect.br.pixit_point.y += note_info.pixit_size.cy;
                skel_point_normalise(p_docu, &skel_rect.br, UPDATE_PANE_CELLS_AREA);
            }

            view_update_later(p_docu, UPDATE_PANE_CELLS_AREA, &skel_rect, rect_flags);

            note_info.object_id = OBJECT_ID_DRAW;
            note_info.object_data_ref = (P_ANY) (uintptr_t) intref;

            status_break(status = object_call_id_load(p_docu, T5_MSG_NOTE_NEW, &note_info, OBJECT_ID_NOTE));
        } /*block (was else)*/
        } /*block*/

        return(STATUS_OK);
        /*NOTREACHED*/
    }

    al_array_shrink_by(&p_draw_instance_data->h_drawing_list, -1);

    return(status);
}

_Check_return_
static STATUS
draw_msg_init1(
    _DocuRef_   P_DOCU p_docu)
{
    P_DRAW_INSTANCE_DATA p_draw_instance = p_object_instance_data_DRAW(p_docu);
    zero_struct_ptr(p_draw_instance);
    return(STATUS_OK);
}

_Check_return_
static STATUS
draw_msg_close2(
    _DocuRef_   P_DOCU p_docu)
{
    const P_DRAW_INSTANCE_DATA p_draw_instance_data = p_object_instance_data_DRAW(p_docu);
    al_array_dispose(&p_draw_instance_data->h_drawing_list);
    return(STATUS_OK);
}

T5_MSG_PROTO(static, draw_msg_initclose, _InRef_ PC_MSG_INITCLOSE p_msg_initclose)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__STARTUP:
        status_return(resource_init(OBJECT_ID_DRAW, P_BOUND_MESSAGES_OBJECT_ID_DRAW, P_BOUND_RESOURCES_OBJECT_ID_DRAW));

        return(register_object_construct_table(OBJECT_ID_DRAW, object_construct_table, TRUE));

    case T5_MSG_IC__EXIT1:
        return(resource_close(OBJECT_ID_DRAW));

    /* initialise object in new document */
    case T5_MSG_IC__INIT1:
        return(draw_msg_init1(p_docu));

    case T5_MSG_IC__CLOSE2:
        return(draw_msg_close2(p_docu));

    default:
        return(STATUS_OK);
    }
}

T5_MSG_PROTO(static, draw_event_redraw, P_NOTE_OBJECT_REDRAW p_note_object_redraw)
{
    const P_DRAW_INSTANCE_DATA p_draw_instance_data = p_object_instance_data_DRAW(p_docu);

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(p_note_object_redraw->object_redraw.flags.show_content)
    {
        ARRAY_INDEX intref = (ARRAY_INDEX) (intptr_t) p_note_object_redraw->object_redraw.object_data.u.p_object; /* position in h_drawing_list (puke) */

        if(array_index_is_valid(&p_draw_instance_data->h_drawing_list, intref))
        {
            const P_DRAWING_INFO p_drawing_info = array_ptr_no_checks(&p_draw_instance_data->h_drawing_list, DRAWING_INFO, intref);

            if(p_drawing_info->use_image_cache_handle)
            {
                ARRAY_HANDLE h_data = 0;

#if WINDOWS
                {
                GdipImage gdip_image = image_cache_search_gdip_image(p_drawing_info->image_cache_handle, &h_data);

                if(NULL != gdip_image)
                {
                    host_paint_gdip_image(&p_note_object_redraw->object_redraw.redraw_context,
                        &p_note_object_redraw->object_redraw.pixit_rect_object,
                        gdip_image);
                    return(STATUS_OK);
                }
                } /*block*/
#endif /* OS */

                /* must be a Draw file */
                h_data = image_cache_search(p_drawing_info->image_cache_handle, FALSE);

                if(0 != array_elements32(&h_data))
                {
                    DRAW_DIAG draw_diag;
                    draw_diag.data = array_base(&h_data, BYTE);
                    draw_diag.length = array_elements32(&h_data);
                    host_paint_drawfile(&p_note_object_redraw->object_redraw.redraw_context,
                                        &p_note_object_redraw->object_redraw.pixit_rect_object.tl,
                                        &p_note_object_redraw->gr_scale_pair,
                                        &draw_diag, 0);
                    return(STATUS_OK);
                }

                return(ERR_NOTE_NOT_LOADED);
            }
        }
    }

    return(STATUS_OK);
}

T5_MSG_PROTO(static, draw_msg_load_ended, P_OF_IP_FORMAT p_of_ip_format)
{
    const P_DRAW_LOAD_INSTANCE p_draw_load_instance = collect_goto_item(DRAW_LOAD_INSTANCE, &p_of_ip_format->object_data_list, OBJECT_ID_DRAW);

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(NULL != p_draw_load_instance)
        al_array_dispose(&p_draw_load_instance->h_mapping_list);

    return(STATUS_OK);
}

T5_MSG_PROTO(static, draw_msg_load_construct_ownform, P_CONSTRUCT_CONVERT p_construct_convert)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    switch(p_construct_convert->p_construct->t5_message)
    {
    case T5_CMD_DRAWFILE_REFERENCE:
        {
        /* swift bit of reprocessing needed to load document relative references */
        PCTSTR input_filename = p_construct_convert->p_of_ip_format->input_filename;
        PCTSTR draw_filename = pc_arglist_arg(&p_construct_convert->arglist_handle, 2)->val.tstr;

        /*if(!file_is_rooted(draw_filename))*/ /* SKS 21may96 Dave Woods hack */
        {
            TCHARZ tstr_buf[BUF_MAX_PATHSTRING];

            STATUS status = 0;

            status = file_find_on_path_or_relative(tstr_buf, elemof32(tstr_buf), file_get_search_path(), draw_filename, input_filename);

            if(status == 0)
            {   /* if not found using supplied name, retry with/without an extension */
                TCHARZ tstr_buf_f[BUF_MAX_PATHSTRING];

                tstr_xstrkpy(tstr_buf_f, elemof32(tstr_buf_f), draw_filename);

                if(file_extension(draw_filename))
                {   /* strip existing extension for retry */
                    PTSTR tstr_extension = file_extension(tstr_buf_f);
                    PTR_ASSERT(tstr_extension);
                    *--tstr_extension = CH_NULL;
                }
                else
                {   /* add an extension for retry */
                    tstr_xstrkat(tstr_buf_f, elemof32(tstr_buf_f), DRAWFILE_EXTENSION_TSTR); /* just have to assume this. sorry */
                }

                status = file_find_on_path_or_relative(tstr_buf, elemof32(tstr_buf), file_get_search_path(), tstr_buf_f, input_filename);
            }

#if WINDOWS
            /* if still not found, try swapping sep chars from RISC OS */
            if(status == 0)
            {
                TCHARZ tstr_buf_f[BUF_MAX_PATHSTRING];
                UINT f_idx = 0;
                PCTSTR i = draw_filename;
                TCHAR c;

                do  {
                    c = *i++;
                    tstr_buf_f[f_idx++] = c;

                    if(c == RISCOS_FILE_DIR_SEP_CH) /* Swap RISC OS dir sep char for Windows? */
                        tstr_buf_f[f_idx-1] = WINDOWS_FILE_DIR_SEP_CH;
                    else
                    if(c == RISCOS_FILE_EXT_SEP_CH) /* Swap RISC OS 'ext' sep char for Windows? */
                        tstr_buf_f[f_idx-1] = WINDOWS_FILE_EXT_SEP_CH;
                }
                while(c != CH_NULL);

                status = file_find_on_path_or_relative(tstr_buf, elemof32(tstr_buf), file_get_search_path(), tstr_buf_f, input_filename);
            }
#endif /* OS */

            if(status_done(status))
            {
                arg_dispose_val(&p_construct_convert->arglist_handle, 2);

                /* if this fails, we are pretty well dead */
                status_return(arg_alloc_tstr(&p_construct_convert->arglist_handle, 2, tstr_buf));
            }
        }

        break;
        }

    default:
        break;
    }

    return(execute_loaded_command(p_docu, p_construct_convert, OBJECT_ID_DRAW));
}

T5_MSG_PROTO(static, draw_msg_insert_foreign, P_MSG_INSERT_FOREIGN p_msg_insert_foreign)
{
    /* if not scrapfile, we *may* keep a reference to the file */
    const BOOL file_is_not_safe = p_msg_insert_foreign->scrap_file;
    const BOOL ctrl_pressed = p_msg_insert_foreign->ctrl_pressed;
    BOOL embed_file = global_preferences.embed_inserted_files;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(!image_cache_can_import_with_image_convert(p_msg_insert_foreign->t5_filetype))
        return(create_error(ERR_UNKNOWN_FILETYPE));

    if(file_is_not_safe)
    {   /* if source is a scrapfile, we must embed, regardless of the checkbox/Ctrl key states */
        embed_file = TRUE;
    }
    else
    {   /* SKS 10/13oct99 take note of Ctrl key state too - inverts the preference */
        if(ctrl_pressed) embed_file = !embed_file;
    }

    return(notelayer_insert_image_file(p_docu, &p_msg_insert_foreign->skel_point,
                                       p_msg_insert_foreign->filename,
                                       p_msg_insert_foreign->t5_filetype,
                                       embed_file));
}

T5_MSG_PROTO(static, draw_msg_save, _InRef_ PC_MSG_SAVE p_msg_save)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    switch(p_msg_save->t5_msg_save_message)
    {
    case T5_MSG_SAVE__SAVE_ENDED:
        {
        const P_OF_OP_FORMAT p_of_op_format = p_msg_save->p_of_op_format;
        const P_DRAW_SAVE_INSTANCE p_draw_save_instance = collect_goto_item(DRAW_SAVE_INSTANCE, &p_of_op_format->object_data_list, OBJECT_ID_DRAW);

        if(NULL != p_draw_save_instance)
            al_array_dispose(&p_draw_save_instance->h_mapping_list);

        return(STATUS_OK);
        }

    default:
        return(STATUS_OK);
    }
}

T5_MSG_PROTO(static, draw_msg_save_picture, _InRef_ P_MSG_SAVE_PICTURE p_msg_save_picture)
{
    const P_DRAW_INSTANCE_DATA p_draw_instance_data = p_object_instance_data_DRAW(p_docu);
    const P_DRAWING_INFO p_drawing_info = array_ptr(&p_draw_instance_data->h_drawing_list, DRAWING_INFO, (ARRAY_INDEX) p_msg_save_picture->extra);

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(p_drawing_info->use_image_cache_handle)
    {
        ARRAY_HANDLE h_data;

        if(FILETYPE_DRAW == p_msg_save_picture->t5_filetype)
            h_data = image_cache_search(p_drawing_info->image_cache_handle, FALSE); /* get the Draw file data (possibly amended for this platform) */
        else
            h_data = image_cache_search(p_drawing_info->image_cache_handle, TRUE); /* get the original image file data */

        if(0 != array_elements32(&h_data))
        {
            const U32 n_bytes = array_elements32(&h_data);
            PC_BYTE p_data = array_rangec(&h_data, BYTE, 0, n_bytes);
            return(binary_write_bytes(&p_msg_save_picture->p_ff_op_format->of_op_format.output, p_data, n_bytes));
        }
    }
    else if(0 != array_elements32(&p_drawing_info->h_unhandled_data))
    {
        const U32 n_bytes = array_elements32(&p_drawing_info->h_unhandled_data);
        PC_BYTE p_data = array_rangec(&p_drawing_info->h_unhandled_data, BYTE, 0, n_bytes);
        return(binary_write_bytes(&p_msg_save_picture->p_ff_op_format->of_op_format.output, p_data, n_bytes));
    }

    return(STATUS_FAIL);
}

T5_MSG_PROTO(static, draw_msg_save_picture_filetypes_request, _InoutRef_ P_MSG_SAVE_PICTURE_FILETYPES_REQUEST p_msg_save_picture_filetypes_request)
{
    const P_DRAW_INSTANCE_DATA p_draw_instance_data = p_object_instance_data_DRAW(p_docu);
    const P_DRAWING_INFO p_drawing_info = array_ptr(&p_draw_instance_data->h_drawing_list, DRAWING_INFO, (ARRAY_INDEX) p_msg_save_picture_filetypes_request->extra);
    STATUS status;
    SAVE_FILETYPE save_filetype; zero_struct_fn(save_filetype);

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(FILETYPE_UNDETERMINED == p_drawing_info->t5_filetype)
        return(STATUS_OK);

    if(FILETYPE_DRAW != p_drawing_info->t5_filetype)
    {
        save_filetype.object_id = OBJECT_ID_DRAW;
        save_filetype.t5_filetype = p_drawing_info->t5_filetype;

        if(NULL != p_drawing_info->filename)
            status_return(ui_text_alloc_from_tstr(&save_filetype.suggested_leafname, file_leafname(p_drawing_info->filename)));

        if(status_fail(status = al_array_add(&p_msg_save_picture_filetypes_request->h_save_filetype, SAVE_FILETYPE, 1, PC_ARRAY_INIT_BLOCK_NONE, &save_filetype)))
        {
            ui_text_dispose(&save_filetype.suggested_leafname);
            return(status);
        }

        zero_struct_fn(save_filetype);
    }

    /* Indicate that we can save pictures as Drawfiles */
    save_filetype.object_id = OBJECT_ID_DRAW;
    save_filetype.t5_filetype = FILETYPE_DRAW;

    if(FILETYPE_DRAW == p_drawing_info->t5_filetype)
        if(NULL != p_drawing_info->filename)
            status_return(ui_text_alloc_from_tstr(&save_filetype.suggested_leafname, file_leafname(p_drawing_info->filename)));

    if(status_fail(status = al_array_add(&p_msg_save_picture_filetypes_request->h_save_filetype, SAVE_FILETYPE, 1, PC_ARRAY_INIT_BLOCK_NONE, &save_filetype)))
    {
        ui_text_dispose(&save_filetype.suggested_leafname);
        return(status);
    }

    return(STATUS_OK);
}

OBJECT_PROTO(extern, object_draw);
OBJECT_PROTO(extern, object_draw)
{
    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        return(draw_msg_initclose(p_docu, t5_message, (PC_MSG_INITCLOSE) p_data));

    case T5_EVENT_REDRAW:
        return(draw_event_redraw(p_docu, t5_message, (P_NOTE_OBJECT_REDRAW) p_data));

    case T5_MSG_NOTE_DELETE:
        return(draw_msg_note_delete(p_docu, t5_message, p_data));

    case T5_MSG_LOAD_ENDED:
        return(draw_msg_load_ended(p_docu, t5_message, (P_OF_IP_FORMAT) p_data));

    case T5_MSG_LOAD_CONSTRUCT_OWNFORM:
        return(draw_msg_load_construct_ownform(p_docu, t5_message, (P_CONSTRUCT_CONVERT) p_data));

    case T5_MSG_INSERT_FOREIGN:
        return(draw_msg_insert_foreign(p_docu, t5_message, (P_MSG_INSERT_FOREIGN) p_data));

    case T5_MSG_SAVE:
        return(draw_msg_save(p_docu, t5_message, (PC_MSG_SAVE) p_data));

    case T5_MSG_SAVE_PICTURE:
        return(draw_msg_save_picture(p_docu, t5_message, (P_MSG_SAVE_PICTURE) p_data));

    case T5_MSG_SAVE_PICTURE_FILETYPES_REQUEST:
        return(draw_msg_save_picture_filetypes_request(p_docu, t5_message, (P_MSG_SAVE_PICTURE_FILETYPES_REQUEST) p_data));

    case T5_MSG_NOTE_LOAD_INTREF_FROM_EXTREF:
        return(draw_msg_note_load_intref_from_extref(p_docu, t5_message, (P_NOTE_REF) p_data));

    case T5_MSG_NOTE_ENSURE_EMBEDED:
        return(draw_msg_note_ensure_embedded(p_docu, t5_message, (P_NOTE_ENSURE_EMBEDDED) p_data));

    case T5_MSG_NOTE_ENSURE_SAVED:
        return(draw_msg_note_ensure_saved(p_docu, t5_message, (P_NOTE_ENSURE_SAVED) p_data));

    case T5_MSG_NOTE_OBJECT_SIZE_QUERY:
        return(draw_msg_note_object_size_query(p_docu, t5_message,  (P_NOTE_OBJECT_SIZE) p_data));

    case T5_MSG_DRAWING_DONATE:
        return(draw_note_donate(p_docu, t5_message, (P_NOTE_OBJECT_SNAPSHOT) p_data));

    case T5_CMD_IMAGE_FILE_EMBEDDED:
    case T5_CMD_DRAWFILE_EMBEDDED:
    case T5_CMD_DRAWFILE_REFERENCE:
        return(draw_file_load(p_docu, t5_message, (PC_T5_CMD) p_data));

    default:
        return(STATUS_OK);
    }
}

/* end of ob_draw.c */
