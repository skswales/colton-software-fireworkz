/* export.c */

/* Copyright (C) 1994-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#include "common/gflags.h"

#include "ob_rec/ob_rec.h"

#include "ob_skel/ff_io.h"

#include "ob_skel/xp_skeld.h"

#if RISCOS
#include "ob_dlg/xp_dlgr.h"
#endif

enum EXPORT_CONTROL_IDS
{
    EXPORT_ID_FORMAT_GROUP = 337,
    EXPORT_ID_FORMAT_DP,
    EXPORT_ID_FORMAT_CSV,
    EXPORT_ID_SAVE,
    EXPORT_ID_PICT,
    EXPORT_ID_NAME
};

/* Define some controls in a dialog box for obtaining a search request */

static const DIALOG_CONTROL
export_format_group =
{
    EXPORT_ID_FORMAT_GROUP, DIALOG_MAIN_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { 0, 0, DIALOG_STDGROUP_RM, DIALOG_STDGROUP_BM },
    { DRT(LTRB, GROUPBOX) }
};

static const DIALOG_CONTROL_DATA_GROUPBOX
export_format_group_data = { UI_TEXT_INIT_RESID(REC_MSG_EXPORT_FORMAT), { 0, 0, 0, FRAMED_BOX_GROUP } };

static const DIALOG_CONTROL
export_format_dp =
{
    EXPORT_ID_FORMAT_DP, EXPORT_ID_FORMAT_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { DIALOG_STDGROUP_LM, DIALOG_STDGROUP_TM, DIALOG_CONTENTS_CALC, DIALOG_STDRADIO_V },
    { DRT(LTLT, RADIOBUTTON) }
};

static const DIALOG_CONTROL_DATA_RADIOBUTTON
export_format_dp_data = { { 0 }, FILETYPE_DATAPOWER, UI_TEXT_INIT_RESID(REC_MSG_EXPORT_DP) };

static const DIALOG_CONTROL
export_format_csv =
{
    EXPORT_ID_FORMAT_CSV, EXPORT_ID_FORMAT_GROUP,
    { EXPORT_ID_FORMAT_DP, EXPORT_ID_FORMAT_DP },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDRADIO_V },
    { DRT(LBLT, RADIOBUTTON) }
};

static const DIALOG_CONTROL_DATA_RADIOBUTTON
export_format_csv_data = { { 0 }, FILETYPE_CSV, UI_TEXT_INIT_RESID(REC_MSG_EXPORT_CSV) };

/* A group box to contain the save thingy */

static const DIALOG_CONTROL
export_save =
{
    EXPORT_ID_SAVE, DIALOG_MAIN_GROUP,
    { EXPORT_ID_FORMAT_GROUP, EXPORT_ID_FORMAT_GROUP, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { 0, DIALOG_STDSPACING_V, DIALOG_STDGROUP_RM, DIALOG_STDGROUP_BM },
    { DRT(LBRB, GROUPBOX) }
};

static const DIALOG_CONTROL_DATA_GROUPBOX
export_save_data = { UI_TEXT_INIT_RESID(REC_MSG_CREATE_FILE), { 0, 0, 0, FRAMED_BOX_GROUP } };

#define SAVE_OWNFORM_TOTAL_H        ((PIXITS_PER_INCH * 17) / 10)
#define SAVE_OWNFORM_PICT_OFFSET_H  ((SAVE_OWNFORM_TOTAL_H - 68 * PIXITS_PER_RISCOS) / 2)

static const DIALOG_CONTROL
export_pict =
{
    EXPORT_ID_PICT, EXPORT_ID_SAVE,
    { EXPORT_ID_NAME, DIALOG_CONTROL_PARENT },
    { -SAVE_OWNFORM_PICT_OFFSET_H, 7*DIALOG_STDSPACING_V, 68 * PIXITS_PER_RISCOS, 68 * PIXITS_PER_RISCOS },
    { DRT(LTLT, USER) }
};

static const RGB
export_pict_rgb = { 0, 0, 0, 1 /*transparent*/ };

static const DIALOG_CONTROL_DATA_USER
export_pict_data = { 0, { FRAMED_BOX_NONE /* border_style */ }, (P_RGB) &export_pict_rgb };

static const DIALOG_CONTROL
export_name =
{
    EXPORT_ID_NAME, EXPORT_ID_SAVE,
    { DIALOG_CONTROL_PARENT, EXPORT_ID_PICT },
    { DIALOG_STDGROUP_LM, DIALOG_STDSPACING_V, SAVE_OWNFORM_TOTAL_H, DIALOG_STDEDIT_V },
    { DRT(LBLT, EDIT), 1 }
};

static const DIALOG_CONTROL_DATA_EDIT
export_name_data = { { { FRAMED_BOX_EDIT } }, /* EDIT_XX */ { UI_TEXT_TYPE_NONE } /* state */ };

static const DIALOG_CONTROL
export_ok =
{
    IDOK, DIALOG_CONTROL_WINDOW,
    { DIALOG_CONTROL_SELF, EXPORT_ID_SAVE, EXPORT_ID_SAVE, DIALOG_CONTROL_SELF },
    { DIALOG_CONTENTS_CALC, DIALOG_STDSPACING_V, 0, DIALOG_DEFPUSHBUTTON_V },
    { DRT(RBRT, PUSHBUTTON), 1 }
};

static const DIALOG_CTL_CREATE
export_ctl_create[] =
{
    { &dialog_main_group },

    { &stdbutton_cancel, &stdbutton_cancel_data },
    { &export_ok,        &defbutton_ok_data },

    { &export_format_group,  &export_format_group_data },
    { &export_format_dp,     &export_format_dp_data },
    { &export_format_csv,    &export_format_csv_data },

    { &export_save,   &export_save_data },
    { &export_pict,   &export_pict_data },
    { &export_name,   &export_name_data }
};

static P_OPENDB export_p_opendb;

static UI_TEXT  export_filename; /* The filename */

static /*poked*/ NUMFORM_PARMS
numform_parms_for_csv =
{
    NULL,
    "0.#########" /*p_numform_numeric*/,
    "DD.MM.yyyy HH:MM:SS;DD.MM.yyyy;HH:MM:SS" /*p_numform_datetime*/,
    "@" /*p_numform_texterror*/
};

_Check_return_
static STATUS
csv_output_field_names(
    P_OPENDB p_opendb,
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    _In_        S32 n_out)
{
    STATUS status = STATUS_OK;
    ARRAY_INDEX f = 0;
    S32 j = 0;

    for(;; ++f /*NB*/)
    {
        PC_FIELDDEF p_fielddef = array_ptrc(&p_opendb->table.h_fielddefs, FIELDDEF, f);

        if(p_fielddef->hidden)
            continue;

        status_break(status = plain_write_a7char(p_ff_op_format, CH_QUOTATION_MARK));
        status_break(status = plain_write_tstr(p_ff_op_format, p_fielddef->name));
        status_break(status = plain_write_a7char(p_ff_op_format, CH_QUOTATION_MARK));

        if(++j == n_out)
        {
            status_break(status = plain_write_newline(p_ff_op_format));
            break;
        }

        status_break(status = plain_write_a7char(p_ff_op_format, CH_COMMA));
    }

    return(status);
}

_Check_return_
static STATUS
csv_output_record(
    P_OPENDB p_opendb,
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format,
    _In_        S32 r,
    _In_        S32 n_out)
{
    STATUS status = STATUS_OK;
    ARRAY_INDEX f = 0;
    S32 j = 0;

    status_return(record_view_recnum(p_opendb, r));

    for(;; ++f /*NB*/)
    {
        PC_FIELDDEF p_fielddef = array_ptrc(&p_opendb->table.h_fielddefs, FIELDDEF, f);

        if(p_fielddef->hidden)
            continue;

        switch(p_fielddef->type)
        {
        case FIELD_PICTURE:
        case FIELD_FILE:
            status = plain_write_tstr(p_ff_op_format, p_fielddef->name);
            break;

        default: default_unhandled();
#if CHECKING
        case FIELD_INTEGER:
        case FIELD_DATE:
        case FIELD_INTERVAL:
        case FIELD_BOOL:
        case FIELD_REAL:
        case FIELD_FORMULA:
        case FIELD_TEXT:
#endif
            {
            REC_RESOURCE rec_resource;

            if(status_ok(status = field_get_value(&rec_resource, p_fielddef, FALSE))) /* SKS 15apr96 bite the bullet and deal with inlines here */
            {
                PC_EV_DATA p_ev_data = &rec_resource.ev_data;

                switch(p_ev_data->did_num)
                {
                case RPN_DAT_BLANK:
                    break;

                case RPN_DAT_STRING:
                    {
                    PC_UCHARS uchars = p_ev_data->arg.string.uchars;
                    S32 size = p_ev_data->arg.string.size;

                    status = plain_write_a7char(p_ff_op_format, CH_QUOTATION_MARK);

                    while(status_ok(status) && (--size >= 0))
                    {
                        if(CH_QUOTATION_MARK == *uchars) /* SKS 01apr95 added string escaping */
                            status_break(status = plain_write_a7char(p_ff_op_format, CH_QUOTATION_MARK));

                        if(is_inline(uchars)) /* SKS 15apr96 */
                            status = plain_write_a7str(p_ff_op_format, "\\" "n"); /* backslash and n */
                        else
                            status = plain_write_uchars(p_ff_op_format, uchars++, 1);
                    }

                    if(status_ok(status))
                        status = plain_write_a7char(p_ff_op_format, CH_QUOTATION_MARK);

                    break;
                    }

                default:
                    {
                    PC_USTR ustr;
                    U32 size;
                    QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 40);
                    quick_ublock_with_buffer_setup(quick_ublock);

                    status_assert(numform(&quick_ublock, P_QUICK_TBLOCK_NONE, p_ev_data, &numform_parms_for_csv));

                    ustr = quick_ublock_ustr(&quick_ublock);
                    size = ustrlen32(ustr);

                    status = plain_write_uchars(p_ff_op_format, ustr, size);

                    quick_ublock_dispose(&quick_ublock);
                    break;
                    }
                }
            }

            ss_data_free_resources(&rec_resource.ev_data);

            break;
            }
        } /* end switch */

        status_break(status);

        if(++j == n_out)
        {
            status = plain_write_newline(p_ff_op_format);
            break;
        }

        status_break(status = plain_write_a7char(p_ff_op_format, CH_COMMA));

    } /* end for ... fields  */

    return(status);
}

_Check_return_
static STATUS
csv_output_database(
    P_OPENDB p_opendb,
    _InoutRef_  P_FF_OP_FORMAT p_ff_op_format)
{
    STATUS status = STATUS_OK;
    S32 n_out = 0;

    numform_parms_for_csv.p_numform_context = get_p_numform_context(P_DOCU_NONE);

    { /* work out how many fields to output */
    ARRAY_INDEX i;

    for(i = 0; i < array_elements(&p_opendb->table.h_fielddefs); ++i)
    {
        if(array_ptrc(&p_opendb->table.h_fielddefs, FIELDDEF, i)->hidden)
            continue;

        ++n_out;
    }
    } /*block*/

    status_return(csv_output_field_names(p_opendb, p_ff_op_format, n_out));

    status_return(ensure_cursor_current(p_opendb, 0));

    {
    S32 r;

    for(r = 0; r < p_opendb->recordspec.ncards; r++)
        status_break(status = csv_output_record(p_opendb, p_ff_op_format, r, n_out));
    } /*block*/

    return(status);
}

_Check_return_
static STATUS
do_export_db(
    _In_z_      PCTSTR filename,
    _InVal_     T5_FILETYPE t5_filetype)
{
    STATUS status;

    ui_text_dispose(&export_filename);
    status_return(ui_text_alloc_from_tstr(&export_filename, filename));

    filename = ui_text_tstr(&export_filename);

    switch(t5_filetype)
    {
    case FILETYPE_CSV:
        {
        P_DOCU p_docu = P_DOCU_NONE; /* we don't really care here */
        FF_OP_FORMAT ff_op_format = { OP_OUTPUT_INVALID };

        if(status_ok(status = foreign_initialise_save(p_docu, &ff_op_format, NULL, filename, t5_filetype, NULL)))
        {
            status = csv_output_database(export_p_opendb, &ff_op_format);

            status = foreign_finalise_save(&p_docu, &ff_op_format, status);
        }
        break;
        }

    default: default_unhandled();
#if CHECKING
    case FILETYPE_DATAPOWER:
#endif
        status = export_current_subset(export_p_opendb, (P_U8) filename);
        break;
    }

    status_assert(status);
    return(status);
}

static BOOL
proc_export_db(
    _In_z_      PCTSTR filename /*low lifetime*/,
    CLIENT_HANDLE client_handle)
{
    const H_DIALOG h_dialog = (H_DIALOG) client_handle;
    TCHARZ save_name[BUF_MAX_PATHSTRING];
    STATUS status;

    tstr_xstrkpy(save_name, elemof32(save_name), filename);

    if(status_fail(status = do_export_db(save_name, (T5_FILETYPE) ui_dlg_get_radio(h_dialog, EXPORT_ID_FORMAT_GROUP))))
        reperr(status, save_name);
    else if(client_handle)
    {
        DIALOG_CMD_COMPLETE_DBOX dialog_cmd_complete_dbox;
        msgclr(dialog_cmd_complete_dbox);
        dialog_cmd_complete_dbox.h_dialog = h_dialog;
        dialog_cmd_complete_dbox.completion_code = DIALOG_COMPLETION_OK;
        status_assert(call_dialog(DIALOG_CMD_CODE_COMPLETE_DBOX, &dialog_cmd_complete_dbox));
    }

    return(TRUE);
}

_Check_return_
static STATUS
dialog_export_ctl_create_state(
    _InoutRef_  P_DIALOG_MSG_CTL_CREATE_STATE p_dialog_msg_ctl_create_state)
{
    switch(p_dialog_msg_ctl_create_state->dialog_control_id)
    {
    case EXPORT_ID_NAME:
        p_dialog_msg_ctl_create_state->state_set.state.edit.ui_text = export_filename;
        p_dialog_msg_ctl_create_state->state_set.bits |= DIALOG_STATE_SET_DONT_MSG;
        break;

    default:
        break;
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_export_ctl_state_change(
    _InRef_     PC_DIALOG_MSG_CTL_STATE_CHANGE p_dialog_msg_ctl_state_change)
{
    switch(p_dialog_msg_ctl_state_change->dialog_control_id)
    {
    case EXPORT_ID_FORMAT_GROUP:
        {
        DIALOG_CMD_CTL_ENCODE dialog_cmd_ctl_encode;
        msgclr(dialog_cmd_ctl_encode);
        dialog_cmd_ctl_encode.h_dialog   = p_dialog_msg_ctl_state_change->h_dialog;
        dialog_cmd_ctl_encode.control_id = EXPORT_ID_PICT;
        dialog_cmd_ctl_encode.bits       = 0;
        status_assert(call_dialog(DIALOG_CMD_CODE_CTL_ENCODE, &dialog_cmd_ctl_encode));
        break;
        }

    default:
        break;
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_export_process_start(
    _InRef_     PC_DIALOG_MSG_PROCESS_START p_dialog_msg_process_start)
{
    return(ui_dlg_set_radio(p_dialog_msg_process_start->h_dialog, EXPORT_ID_FORMAT_GROUP, FILETYPE_CSV /*DATAPOWER*/));
}

_Check_return_
static STATUS
dialog_export_ctl_pushbutton(
    _InoutRef_  P_DIALOG_MSG_CTL_PUSHBUTTON p_dialog_msg_ctl_pushbutton)
{
    switch(p_dialog_msg_ctl_pushbutton->dialog_control_id)
    {
    case IDOK:
        {
        PCTSTR filename;

        ui_text_dispose(&export_filename);
        ui_dlg_get_edit(p_dialog_msg_ctl_pushbutton->h_dialog, EXPORT_ID_NAME, &export_filename);

        filename = ui_text_tstr(&export_filename);

        if(!file_is_rooted(filename))
        {
            reperr_null(ERR_SAVE_DRAG_TO_DIRECTORY);
            return(STATUS_OK);
        }

        proc_export_db(filename, (CLIENT_HANDLE) p_dialog_msg_ctl_pushbutton->h_dialog);
        break;
        }

    default:
        break;
    }

    return(STATUS_OK);
}

#if RISCOS

_Check_return_
static STATUS
dialog_export_ctl_user_redraw(
    _InRef_     PC_DIALOG_MSG_CTL_USER_REDRAW p_dialog_msg_ctl_user_redraw)
{
    const T5_FILETYPE t5_filetype = (T5_FILETYPE) ui_dlg_get_radio(p_dialog_msg_ctl_user_redraw->h_dialog, EXPORT_ID_FORMAT_GROUP);
    WimpIconBlockWithBitset icon;

    zero_struct(icon);

    host_ploticon_setup_bbox(&icon, &p_dialog_msg_ctl_user_redraw->control_inner_box, &p_dialog_msg_ctl_user_redraw->redraw_context);

    /* fills in icon with name of a sprite representing the filetype from the Window Manager Sprite Pool */
    dialog_riscos_file_icon_setup(&icon, t5_filetype);

    host_ploticon(&icon);

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_export_ctl_user_mouse(
    _InRef_     PC_DIALOG_MSG_CTL_USER_MOUSE p_dialog_msg_ctl_user_mouse)
{
    switch(p_dialog_msg_ctl_user_mouse->click)
    {
    case DIALOG_MSG_USER_MOUSE_CLICK_LEFT_DRAG:
        {
        const T5_FILETYPE t5_filetype = (T5_FILETYPE) ui_dlg_get_radio(p_dialog_msg_ctl_user_mouse->h_dialog, EXPORT_ID_FORMAT_GROUP);
        WimpIconBlockWithBitset icon;

        ui_text_dispose(&export_filename);
        ui_dlg_get_edit(p_dialog_msg_ctl_user_mouse->h_dialog, EXPORT_ID_NAME, &export_filename);

        zero_struct(icon);

        icon.bbox = p_dialog_msg_ctl_user_mouse->riscos.icon.bbox;

        /* fills in icon with name of a sprite representing the filetype from the Window Manager Sprite Pool */
        dialog_riscos_file_icon_setup(&icon, t5_filetype);

        status_return(dialog_riscos_file_icon_drag(p_dialog_msg_ctl_user_mouse->h_dialog, &icon));

        break;
        }

    default:
        break;
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_export_riscos_drag_ended(
    _InoutRef_  P_DIALOG_MSG_RISCOS_DRAG_ENDED p_dialog_msg_riscos_drag_ended)
{
    /* try saving file to target window */
    if(p_dialog_msg_riscos_drag_ended->mouse.window_handle != -1)
    {
        const T5_FILETYPE t5_filetype = (T5_FILETYPE) ui_dlg_get_radio(p_dialog_msg_riscos_drag_ended->h_dialog, EXPORT_ID_FORMAT_GROUP);

        consume_bool(
            host_xfer_save_file(
                ui_text_tstr(&export_filename),
                t5_filetype,
                42 /*estimated_size*/,
                proc_export_db,
                (CLIENT_HANDLE) p_dialog_msg_riscos_drag_ended->h_dialog,
                &p_dialog_msg_riscos_drag_ended->mouse));
    }

    return(STATUS_OK);
}

#endif /* OS */

PROC_DIALOG_EVENT_PROTO(static, dialog_event_export)
{
    IGNOREPARM_DocuRef_(p_docu);

    switch(dialog_message)
    {
    case DIALOG_MSG_CODE_CTL_CREATE_STATE:
        return(dialog_export_ctl_create_state((P_DIALOG_MSG_CTL_CREATE_STATE) p_data));

    case DIALOG_MSG_CODE_CTL_STATE_CHANGE:
        return(dialog_export_ctl_state_change((PC_DIALOG_MSG_CTL_STATE_CHANGE) p_data));

    case DIALOG_MSG_CODE_PROCESS_START:
        return(dialog_export_process_start((PC_DIALOG_MSG_PROCESS_START) p_data));

    case DIALOG_MSG_CODE_CTL_PUSHBUTTON:
        return(dialog_export_ctl_pushbutton((P_DIALOG_MSG_CTL_PUSHBUTTON) p_data));

#if RISCOS
    case DIALOG_MSG_CODE_CTL_USER_REDRAW:
        return(dialog_export_ctl_user_redraw((PC_DIALOG_MSG_CTL_USER_REDRAW) p_data));

    case DIALOG_MSG_CODE_CTL_USER_MOUSE:
        return(dialog_export_ctl_user_mouse((PC_DIALOG_MSG_CTL_USER_MOUSE) p_data));

    case DIALOG_MSG_CODE_RISCOS_DRAG_ENDED:
        return(dialog_export_riscos_drag_ended((P_DIALOG_MSG_RISCOS_DRAG_ENDED) p_data));
#endif /* RISCOS */

    default:
        return(STATUS_OK);
    }
}

_Check_return_
extern STATUS
t5_cmd_db_export(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DATA_REF p_data_ref)
{
    STATUS status;

    {
    P_REC_PROJECTOR p_rec_projector = p_rec_projector_from_db_id(p_docu, p_data_ref->arg.db_field.db_id);

    if(!p_rec_projector->opendb.dbok)
        return(create_error(REC_ERR_DATABASE_NOT_OPEN));

    status_return(table_check_access(&p_rec_projector->opendb.table, ACCESS_FUNCTION_SAVE));

    export_p_opendb = &p_rec_projector->opendb;
    } /*block*/

    export_filename.type = UI_TEXT_TYPE_RESID;
    export_filename.text.resource_id = REC_MSG_EXPORT_LEAFNAME;

    {
    DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
    dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, export_ctl_create, elemof32(export_ctl_create), 0);
    /*dialog_cmd_process_dbox.caption.type = UI_TEXT_TYPE_RESID;*/
    dialog_cmd_process_dbox.caption.text.resource_id = REC_MSG_EXPORT_TITLE;
    dialog_cmd_process_dbox.p_proc_client = dialog_event_export;
    status = call_dialog_with_docu(p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox);
    } /*block*/

    ui_text_dispose(&export_filename);

    return(status);
}

/* end of export.c */
