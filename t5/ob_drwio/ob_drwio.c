/* ob_drwio.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2001-2015 R W Colton */

/* Save as Drawfile object module for Fireworkz */

/* SKS July 2001 */

#include "common/gflags.h"

#include "ob_drwio/ob_drwio.h"

#include "ob_skel/ff_io.h"

#ifndef          __collect_h
#include "cmodules/collect.h"
#endif

#ifndef          __typepack_h
#include "cmodules/typepack.h"
#endif

#include "ob_file/xp_file.h"

#if WINDOWS
#include "commdlg.h"

#include "cderr.h"

#include "direct.h"
#endif

#if RISCOS
#if defined(BOUND_MESSAGES_OBJECT_ID_DRAW_IO)
extern PC_U8 rb_draw_io_msg_weak;
#define P_BOUND_MESSAGES_OBJECT_ID_DRAW_IO &rb_draw_io_msg_weak
#else
#define P_BOUND_MESSAGES_OBJECT_ID_DRAW_IO LOAD_MESSAGES_FILE
#endif
#else
#define P_BOUND_MESSAGES_OBJECT_ID_DRAW_IO DONT_LOAD_MESSAGES_FILE
#endif

#define P_BOUND_RESOURCES_OBJECT_ID_DRAW_IO DONT_LOAD_RESOURCES

/* ------------------------------------------------------------------------------------------------------- */

/*
construct argument types
*/

static const ARG_TYPE
args_cmd_hybrid_setup[] =
{
    ARG_TYPE_X32 | ARG_MANDATORY,               /* file type */
    ARG_TYPE_TSTR | ARG_MANDATORY_OR_BLANK,     /* page range (may be empty -> all pages) */
    ARG_TYPE_NONE
};

static CONSTRUCT_TABLE
object_construct_table[] =
{                                                                                                   /*   fi ti mi ur up xi md mf nn cp sm ba fo */
    { "HybridSetup",            args_cmd_hybrid_setup,      T5_CMD_OF_HYBRID_SETUP,                     { 1, 1, 0, 0, 0, 0, 0, 1, 0 } },

#if WINDOWS && defined(T5_CMD_SAVE_AS_METAFILE)
    { "SaveAsMetafileIntro",    NULL,                       T5_CMD_SAVE_AS_METAFILE_INTRO,              { 0, 0, 0, 0, 0, 0, 0, 1, 0 } },
    { "SaveAsMetafile",         args_cmd_save_as_drawfile,  T5_CMD_SAVE_AS_METAFILE,                    { 0, 0, 0, 0, 0, 0, 0, 1, 0 } },
#endif

    { NULL,                     NULL,                       T5_EVENT_NONE } /* end of table */
};

/******************************************************************************
*
* draw_io hybrid Draw setup
*
******************************************************************************/

T5_CMD_PROTO(static, t5_cmd_hybrid_setup)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, ARG_HYBRID_SETUP_N_ARGS);
    const T5_FILETYPE t5_filetype = p_args[ARG_HYBRID_SETUP_FILETYPE].val.t5_filetype;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    /* no matter how it was loaded, that's what type it should be if saved directly again (if possible - check during save) */
    p_docu->docu_preferred_filetype = t5_filetype;

    status_return(tstr_set(&p_docu->tstr_hybrid_draw_page_range, p_args[ARG_HYBRID_SETUP_PAGE_RANGE].val.tstr)); /* may be empty, never NULL */

    return(STATUS_OK);
}

/******************************************************************************
*
* draw_io render document as Draw data
*
******************************************************************************/

T5_MSG_PROTO(static, draw_io_msg_render_as_draw, _InoutRef_ P_MSG_RENDER_AS_DRAW p_msg_render_as_draw)
{
    STATUS status;
    PRINT_CTRL print_ctrl; zero_struct_fn(print_ctrl);

    UNREFERENCED_PARAMETER_InVal_(t5_message);

  /*print_ctrl.h_page_list = 0;*/

    print_ctrl.flags.landscape = p_docu->page_def.landscape;

    status = print_page_list_fill_from_tstr(&print_ctrl.h_page_list, p_msg_render_as_draw->tstr_page_range);

    if(status_ok(status))
        status = save_as_drawfile_host_print_document(p_docu, &print_ctrl, NULL /*returns data*/, p_msg_render_as_draw->t5_filetype);

    al_array_dispose(&print_ctrl.h_page_list);

    status_return(status);

    p_msg_render_as_draw->array_handle = (ARRAY_HANDLE) status; /* donate to caller */

    return(status);
}

/* EMF we'd have to save data to array handle first then pass that to embed in post phase render */

/******************************************************************************
*
* ensure that it's all in memory, so we can skip over Draw data until we get to Fireworkz data
*
******************************************************************************/

_Check_return_
static STATUS
mutate_input_from_file_to_mem(
    _InoutRef_ P_OF_IP_FORMAT p_of_ip_format)
{
    STATUS status;
    const U32 n_bytes = p_of_ip_format->input.file.file_size_in_bytes;
    ARRAY_HANDLE array_handle;
    P_BYTE p_byte;

    if(NULL != (p_byte = al_array_alloc_BYTE(&array_handle, n_bytes, &array_init_block_u8, &status)))
        status = file_read_bytes_requested(p_byte, n_bytes, p_of_ip_format->input.file.file_handle);

    if(status_ok(status))
    {   /* mutate input from FILE to MEM for object scanning */
        (void) t5_file_close(&p_of_ip_format->input.file.file_handle);

        p_of_ip_format->input.mem.owned_array_handle = array_handle; /* donate - will be freed at end of load */
        p_of_ip_format->input.mem.p_array_handle = &p_of_ip_format->input.mem.owned_array_handle;
        p_of_ip_format->input.mem.array_offset = 0;
        p_of_ip_format->input.state = IP_INPUT_MEM;
    }
    else
    {
        al_array_dispose(&array_handle);
    }

    return(status);
}

T5_MSG_PROTO(static, draw_io_msg_setup_for_input_from_hybrid_draw, _InoutRef_ P_OF_IP_FORMAT p_of_ip_format)
{
    STATUS status;

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(0 != p_of_ip_format->input.bom_bytes)
        /* that would be really stupid */
        return(status_check());

    if(IP_INPUT_FILE == p_of_ip_format->input.state)
        status_return(mutate_input_from_file_to_mem(p_of_ip_format));

    status_return(status = find_fireworkz_data_in_drawfile(p_of_ip_format));

    /* abuse this to skip straight to the Fireworkz data on the subsequent input_rewind */
    p_of_ip_format->input.bom_bytes = (U32) status;

    return(input_rewind(&p_of_ip_format->input));
}

/******************************************************************************
*
* draw_io converter object event handler
*
******************************************************************************/

T5_MSG_PROTO(static, draw_io_msg_initclose, _InRef_ PC_MSG_INITCLOSE p_msg_initclose)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__STARTUP:
        status_return(resource_init(OBJECT_ID_DRAW_IO, P_BOUND_MESSAGES_OBJECT_ID_DRAW_IO, P_BOUND_RESOURCES_OBJECT_ID_DRAW_IO));

        return(register_object_construct_table(OBJECT_ID_DRAW_IO, object_construct_table, FALSE /* no inlines */));

    case T5_MSG_IC__EXIT1:
        return(resource_close(OBJECT_ID_DRAW_IO));

    default:
        return(STATUS_OK);
    }
}

OBJECT_PROTO(extern, object_draw_io);
OBJECT_PROTO(extern, object_draw_io)
{
    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        return(draw_io_msg_initclose(p_docu, t5_message, (PC_MSG_INITCLOSE) p_data));

    case T5_MSG_RENDER_AS_DRAW:
        return(draw_io_msg_render_as_draw(p_docu, t5_message, (P_MSG_RENDER_AS_DRAW) p_data));

    case T5_MSG_SETUP_FOR_INPUT_FROM_HYBRID_DRAW:
        return(draw_io_msg_setup_for_input_from_hybrid_draw(p_docu, t5_message, (P_OF_IP_FORMAT) p_data));

    case T5_CMD_OF_HYBRID_SETUP:
        return(t5_cmd_hybrid_setup(p_docu, t5_message, (PC_T5_CMD) p_data));

#if WINDOWS && defined(T5_CMD_SAVE_AS_METAFILE)
    case T5_CMD_SAVE_AS_METAFILE_INTRO:
        return(t5_cmd_save_as_drawfile_intro(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_SAVE_AS_METAFILE:
        return(t5_cmd_save_as_drawfile(p_docu, t5_message, (PC_T5_CMD) p_data));
#endif /* T5_CMD_SAVE_AS_METAFILE */

    default:
        return(STATUS_OK);
    }
}

/* end of ob_drwio.c */
