/* ui_misc.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#ifndef         __xp_skeld_h
#include "ob_skel/xp_skeld.h"
#endif

#if RISCOS
#include "ob_skel/xp_skelr.h"
#endif

_Check_return_
static STATUS
query_close_linked(
    _DocuRef_   P_DOCU p_docu);

_Ret_ _Check_return_
static PC_UI_TEXT
get_ui_text_product_ui_id(void)
{
    static UI_TEXT ui_text_product_ui_id = { UI_TEXT_TYPE_TSTR_PERM };

    ui_text_product_ui_id.text.tstr = product_ui_id();

    return(&ui_text_product_ui_id);
}

#if WINDOWS && 1

#define INFO_WINDOW_TIMER_TIMEOUT 50 * 100  /* up for ~5 seconds */

_Check_return_
extern STATUS
t5_cmd_info(
    _DocuRef_   P_DOCU p_docu)
{
    const P_VIEW p_view = p_view_from_viewno_caret(p_docu);
    const HOST_WND hwnd = VIEW_NOT_NONE(p_view) ? p_view->main[WIN_BACK].hwnd : HOST_WND_NONE;

    splash_window_create(hwnd, INFO_WINDOW_TIMER_TIMEOUT);

    return(STATUS_OK);
}

#else

/******************************************************************************
*
* Display the program info box
*
******************************************************************************/

#if RISCOS
#define INFO_FIELDS_LABEL_H ( 8 * (PIXITS_PER_INCH / 10))
#define INFO_FIELDS_DATA_H  (30 * (PIXITS_PER_INCH / 10))
#define INFO_FIELDS_DATA_V  (48 * PIXITS_PER_RISCOS)
#else
#define INFO_FIELDS_LABEL_H DIALOG_SYSCHARSL_H(16)
#define INFO_FIELDS_DATA_H  DIALOG_SYSCHARSL_H(32)
#define INFO_FIELDS_DATA_V  DIALOG_STDEDIT_V
#endif

enum INFO_CONTROL_IDS
{
    INFO_ID_NAME_LABEL = 40,
    INFO_ID_NAME,
    INFO_ID_AUTHOR_LABEL,
    INFO_ID_AUTHOR,
    INFO_ID_EXTRA_BUMPH,
    INFO_ID_PICTURE,
    INFO_ID_WEB,
    INFO_ID_VERSION_LABEL,
    INFO_ID_VERSION,
    INFO_ID_USER_LABEL,
    INFO_ID_USER,
    INFO_ID_ORGAN,
    INFO_ID_REGNO_LABEL,
    INFO_ID_REGNO
};

#define FRAMED_BOX_INFO_FIELDS FRAMED_BOX_TROUGH
#define P_RGB_INFO_FIELDS      NULL

static const DIALOG_CONTROL
info_name_label =
{
    INFO_ID_NAME_LABEL, DIALOG_MAIN_GROUP,
    { DIALOG_CONTROL_PARENT, INFO_ID_NAME, DIALOG_CONTROL_SELF, INFO_ID_NAME },
    { 0, 0, INFO_FIELDS_LABEL_H, 0 },
    { DRT(LTLB, TEXTLABEL) }
};

static const DIALOG_CONTROL_DATA_TEXTLABEL
info_name_label_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_INFO_NAME_LABEL) };

static const DIALOG_CONTROL
info_name =
{
    INFO_ID_NAME, DIALOG_MAIN_GROUP,
    { INFO_ID_NAME_LABEL, DIALOG_CONTROL_PARENT },
    { DIALOG_LABELGAP_H, 0, INFO_FIELDS_DATA_H, INFO_FIELDS_DATA_V },
    { DRT(RTLT, TEXTFRAME) }
};

static /*poked*/ DIALOG_CONTROL_DATA_TEXTFRAME
info_name_data = { { UI_TEXT_TYPE_NONE }, { 0, 1 /*centre_text*/, 0, FRAMED_BOX_INFO_FIELDS }, P_RGB_INFO_FIELDS };

static const DIALOG_CONTROL
info_author_label =
{
    INFO_ID_AUTHOR_LABEL, DIALOG_MAIN_GROUP,
    { INFO_ID_NAME_LABEL, INFO_ID_AUTHOR, INFO_ID_NAME_LABEL, INFO_ID_AUTHOR },
    { 0 },
    { DRT(LTRB, TEXTLABEL) }
};

static const DIALOG_CONTROL_DATA_TEXTLABEL
info_author_label_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_INFO_AUTHOR_LABEL) };

static const DIALOG_CONTROL
info_author =
{
    INFO_ID_AUTHOR, DIALOG_MAIN_GROUP,
    { INFO_ID_NAME, INFO_ID_NAME, INFO_ID_NAME },
    { 0, DIALOG_SMALLSPACING_V, 0, INFO_FIELDS_DATA_V },
    { DRT(LBRT, TEXTFRAME) }
};

static DIALOG_CONTROL_DATA_TEXTFRAME
info_author_data = { { UI_TEXT_TYPE_NONE }, { 0, 1 /*centre_text*/, 0, FRAMED_BOX_INFO_FIELDS }, P_RGB_INFO_FIELDS };

static const DIALOG_CONTROL
info_extra_bumph =
{
    INFO_ID_EXTRA_BUMPH, DIALOG_MAIN_GROUP,
    { INFO_ID_AUTHOR, INFO_ID_AUTHOR, INFO_ID_AUTHOR },
    { 0, DIALOG_SMALLSPACING_V, 0, INFO_FIELDS_DATA_V },
    { DRT(LBRT, TEXTFRAME) }
};

static DIALOG_CONTROL_DATA_TEXTFRAME
info_extra_bumph_data = { { UI_TEXT_TYPE_NONE }, { 0, 1 /*centre_text*/, 0, FRAMED_BOX_INFO_FIELDS }, P_RGB_INFO_FIELDS };

static const DIALOG_CONTROL
info_version_label =
{
    INFO_ID_VERSION_LABEL, DIALOG_MAIN_GROUP,
    { INFO_ID_AUTHOR_LABEL, INFO_ID_VERSION, INFO_ID_AUTHOR_LABEL, INFO_ID_VERSION },
    { 0 },
    { DRT(LTRB, TEXTLABEL) }
};

static const DIALOG_CONTROL_DATA_TEXTLABEL
info_version_label_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_INFO_VERSION_LABEL) };

static const DIALOG_CONTROL
info_version_sans_extra =
{
    INFO_ID_VERSION, DIALOG_MAIN_GROUP,
    { INFO_ID_AUTHOR, INFO_ID_AUTHOR, INFO_ID_AUTHOR },
    { 0, DIALOG_SMALLSPACING_V, 0, INFO_FIELDS_DATA_V },
    { DRT(LBRT, TEXTFRAME) }
};

static const DIALOG_CONTROL
info_version =
{
    INFO_ID_VERSION, DIALOG_MAIN_GROUP,
    { INFO_ID_EXTRA_BUMPH, INFO_ID_EXTRA_BUMPH, INFO_ID_EXTRA_BUMPH },
    { 0, DIALOG_SMALLSPACING_V, 0, INFO_FIELDS_DATA_V },
    { DRT(LBRT, TEXTFRAME) }
};

static /*poked*/ DIALOG_CONTROL_DATA_TEXTFRAME
info_version_data = { { UI_TEXT_TYPE_NONE }, { 0, 1 /*centre_text*/, 0, FRAMED_BOX_INFO_FIELDS }, P_RGB_INFO_FIELDS };

#if RISCOS

static const DIALOG_CONTROL
info_picture =
{
    INFO_ID_PICTURE, DIALOG_MAIN_GROUP,
    { INFO_ID_NAME, INFO_ID_NAME, INFO_ID_WEB },
    { DIALOG_SMALLSPACING_H, 0, 0 /*120 * PIXITS_PER_RISCOS*/, 96 * PIXITS_PER_RISCOS },
    { DRT(RTRT, STATICPICTURE) }
};

static const DIALOG_CONTROL_DATA_STATICPICTURE
info_picture_data = { { OBJECT_ID_SKEL, "!fireworkz" } };

#endif /* RISCOS */

static const DIALOG_CONTROL
info_web =
{
    INFO_ID_WEB, DIALOG_MAIN_GROUP,
    { INFO_ID_VERSION, INFO_ID_VERSION, DIALOG_CONTROL_SELF, INFO_ID_VERSION },
    { DIALOG_SMALLSPACING_H, 0, INFO_FIELDS_LABEL_H, 0 },
    { DRT(RTLB, PUSHBUTTON) }
};

static const DIALOG_CONTROL_ID
info_web_argmap[] = { INFO_ID_WEB };

static const DIALOG_CONTROL_DATA_PUSH_COMMAND
info_web_command = { T5_CMD_HELP_URL, OBJECT_ID_SKEL, NULL, info_web_argmap, { 0, 0, 0, 1 /*lookup_arglist*/ } };

static const DIALOG_CONTROL_DATA_PUSHBUTTON
info_web_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_INFO_WEB_BUTTON), &info_web_command };

static const DIALOG_CONTROL
info_user_label =
{
    INFO_ID_USER_LABEL, DIALOG_MAIN_GROUP,
    { INFO_ID_VERSION_LABEL, INFO_ID_USER, INFO_ID_VERSION_LABEL, INFO_ID_USER },
    { 0 },
    { DRT(LTRB, TEXTLABEL) }
};

static const DIALOG_CONTROL_DATA_TEXTLABEL
info_user_label_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_INFO_USER_LABEL) };

static const DIALOG_CONTROL
info_user =
{
    INFO_ID_USER, DIALOG_MAIN_GROUP,
    { INFO_ID_VERSION, INFO_ID_VERSION, INFO_ID_VERSION },
    { 0, DIALOG_SMALLSPACING_V, 0, INFO_FIELDS_DATA_V },
    { DRT(LBRT, TEXTFRAME) }
};

static /*poked*/ DIALOG_CONTROL_DATA_TEXTFRAME
info_user_data = { { UI_TEXT_TYPE_NONE }, { 0, 1 /*centre_text*/, 0, FRAMED_BOX_INFO_FIELDS }, P_RGB_INFO_FIELDS };

static const DIALOG_CONTROL
info_organ =
{
    INFO_ID_ORGAN, DIALOG_MAIN_GROUP,
    { INFO_ID_USER, INFO_ID_USER, INFO_ID_USER },
    { 0, DIALOG_SMALLSPACING_V, 0, INFO_FIELDS_DATA_V },
    { DRT(LBRT, TEXTFRAME) }
};

static /*poked*/ DIALOG_CONTROL_DATA_TEXTFRAME
info_organ_data = { { UI_TEXT_TYPE_NONE }, { 0, 1 /*centre_text*/, 0, FRAMED_BOX_INFO_FIELDS }, P_RGB_INFO_FIELDS };

static const DIALOG_CONTROL
info_regno_label =
{
    INFO_ID_REGNO_LABEL, DIALOG_MAIN_GROUP,
    { INFO_ID_VERSION_LABEL, INFO_ID_REGNO, INFO_ID_VERSION_LABEL, INFO_ID_REGNO },
    { 0 },
    { DRT(LTRB, TEXTLABEL) }
};

static const DIALOG_CONTROL_DATA_TEXTLABEL
info_regno_label_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_INFO_REGNO_LABEL) };

static const DIALOG_CONTROL
info_regno =
{
    INFO_ID_REGNO, DIALOG_MAIN_GROUP,
#if WINDOWS || 1
    { INFO_ID_ORGAN, INFO_ID_ORGAN, INFO_ID_ORGAN },
    { 0, DIALOG_SMALLSPACING_V, 0, INFO_FIELDS_DATA_V },
    { DRT(LBRT, TEXTFRAME) }
#else
    { DIALOG_CONTROL_SELF, INFO_ID_ORGAN, INFO_ID_ORGAN },
    { DIALOG_SYSCHARS_H("  6000 0000 0000 0000  "), DIALOG_SMALLSPACING_V, 0, INFO_FIELDS_DATA_V },
    { DRT(RBRT, TEXTFRAME) }
#endif
};

static /*poked*/ DIALOG_CONTROL_DATA_TEXTFRAME
info_regno_data = { { UI_TEXT_TYPE_NONE }, { 0, 1 /*centre_text*/, 0, FRAMED_BOX_INFO_FIELDS }, P_RGB_INFO_FIELDS };

static const DIALOG_CTL_CREATE
info_ctl_create_sans_extra[] =
{
    { { &dialog_main_group }, NULL },

    { { &info_name_label }, &info_name_label_data },
    { { &info_name }, &info_name_data },
    { { &info_author_label }, &info_author_label_data },
    { { &info_author }, &info_author_data },

    { { &info_version_label }, &info_version_label_data },
    { { &info_version_sans_extra }, &info_version_data },

#if RISCOS
    { { &info_picture }, &info_picture_data },
#endif
    { { &info_web }, &info_web_data },

    { { &info_user_label }, &info_user_label_data },
    { { &info_user }, &info_user_data },
    { { &info_organ }, &info_organ_data },
    { { &info_regno_label }, &info_regno_label_data },
    { { &info_regno }, &info_regno_data }
};

static const DIALOG_CTL_CREATE
info_ctl_create[] =
{
    { { &dialog_main_group }, NULL },

    { { &info_name_label }, &info_name_label_data },
    { { &info_name }, &info_name_data },
    { { &info_author_label }, &info_author_label_data },
    { { &info_author }, &info_author_data },

    { { &info_extra_bumph }, &info_extra_bumph_data },

    { { &info_version_label }, &info_version_label_data },
    { { &info_version }, &info_version_data },

#if RISCOS
    { { &info_picture }, &info_picture_data },
#endif
    { { &info_web }, &info_web_data },

    { { &info_user_label }, &info_user_label_data },
    { { &info_user }, &info_user_data },
    { { &info_organ }, &info_organ_data },
    { { &info_regno_label }, &info_regno_label_data },
    { &info_regno, &info_regno_data }
};

_Check_return_
static STATUS
dialog_info_preprocess_command(
    _InoutRef_  P_DIALOG_MSG_PREPROCESS_COMMAND p_dialog_msg_preprocess_command)
{
    arg_dispose(&p_dialog_msg_preprocess_command->arglist_handle, 0);

    p_arglist_arg(&p_dialog_msg_preprocess_command->arglist_handle, 0)->type = ARG_TYPE_TSTR;

    return(arg_alloc_tstr(&p_dialog_msg_preprocess_command->arglist_handle, 0, resource_lookup_tstr_no_default(MSG_DIALOG_INFO_WEB_URL)));
}

PROC_DIALOG_EVENT_PROTO(static, dialog_event_info)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    switch(dialog_message)
    {
    case DIALOG_MSG_CODE_PREPROCESS_COMMAND:
        return(dialog_info_preprocess_command((P_DIALOG_MSG_PREPROCESS_COMMAND) p_data));

    default:
        return(STATUS_OK);
    }
}

static void
cmd_info_init(
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock)
{
    info_name_data.caption = *get_ui_text_product_ui_id();

    info_author_data.caption.type = UI_TEXT_TYPE_RESID;
    info_author_data.caption.text.resource_id = MSG_COPYRIGHT;

#if RISCOS
    if(has_real_database)
    {
        info_extra_bumph_data.caption.type = UI_TEXT_TYPE_RESID;
        info_extra_bumph_data.caption.text.resource_id = MSG_PORTIONS_R_COMP;
    }
#elif WINDOWS
    info_extra_bumph_data.caption.type = UI_TEXT_TYPE_RESID;
    info_extra_bumph_data.caption.text.resource_id = MSG_PORTIONS_DIAL_SOLUTIONS;
#endif

    info_version_data.caption.type = UI_TEXT_TYPE_NONE;

    if(status_ok(resource_lookup_quick_tblock(p_quick_tblock, MSG_SKEL_VERSION)))
    {
#if WINDOWS && defined(_M_X64)
        if(status_ok(quick_tblock_tstr_add(p_quick_tblock, TEXT(" 64-bit"))))
#endif
        if(status_ok(quick_tblock_tchar_add(p_quick_tblock, CH_SPACE)))
        if(status_ok(quick_tblock_tchar_add(p_quick_tblock, UCH_LEFT_PARENTHESIS)))
        if(status_ok(resource_lookup_quick_tblock(p_quick_tblock, MSG_SKEL_DATE)))
        if(status_ok(quick_tblock_tchar_add(p_quick_tblock, UCH_RIGHT_PARENTHESIS)))
        if(status_ok(quick_tblock_nullch_add(p_quick_tblock)))
        {
            info_version_data.caption.type = UI_TEXT_TYPE_TSTR_PERM;
            info_version_data.caption.text.tstr = quick_tblock_tstr(p_quick_tblock);
        }
    }

    info_user_data.caption.type = UI_TEXT_TYPE_TSTR_PERM;
    info_user_data.caption.text.tstr = user_id();

    info_organ_data.caption.type = UI_TEXT_TYPE_TSTR_PERM;
    info_organ_data.caption.text.tstr = user_organ_id();

    info_regno_data.caption.type = UI_TEXT_TYPE_TSTR_PERM;
    info_regno_data.caption.text.tstr = registration_number();
}

_Check_return_
extern STATUS
t5_cmd_info(
    _DocuRef_   P_DOCU p_docu)
{
    STATUS status;
    U32 n_strip = 0;
    QUICK_TBLOCK_WITH_BUFFER(version_quick_tblock, 64);
    quick_tblock_with_buffer_setup(version_quick_tblock);

    cmd_info_init(&version_quick_tblock);

    if(CH_NULL == info_user_data.caption.text.tstr[0])
    {   /* omit the user info block */
        assert(UI_TEXT_TYPE_RESID == info_user_data.caption.type);
        n_strip = 5;
    }

    {
    DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
#if RISCOS
    if(!has_real_database)
    {
        dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, info_ctl_create_sans_extra, elemof32(info_ctl_create_sans_extra) - n_strip, MSG_DIALOG_INFO_CAPTION);
    }
    else
#endif
    {
        dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, info_ctl_create, elemof32(info_ctl_create) - n_strip, MSG_DIALOG_INFO_CAPTION);
    }
  /*dialog_cmd_process_dbox.help_topic_resource_id = 0;*/
    dialog_cmd_process_dbox.p_proc_client = dialog_event_info;
  /*dialog_cmd_process_dbox.client_handle = NULL;*/
    status = object_call_DIALOG_with_docu(p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox);
    } /*block*/

    quick_tblock_dispose(&version_quick_tblock);

    return(status);
}

#endif /* OS */

_Check_return_
extern STATUS
t5_cmd_quit(
    _DocuRef_   P_DOCU p_docu)
{
    if(status_ok(query_quit(p_docu, docs_modified())))
    {
#if WINDOWS
        PostQuitMessage(0);
#else
        exit(EXIT_SUCCESS);
#endif
    }

    return(STATUS_OK);
}

/******************************************************************************
*
* A twee little function that returns the number of modified documents within the system.
*
******************************************************************************/

extern S32
docs_modified(void)
{
    S32 res = 0;

    DOCNO docno = DOCNO_NONE;

    while(DOCNO_NONE != (docno = docno_enum_docs(docno)))
    {
        const P_DOCU p_docu = p_docu_from_docno_valid(docno);

        status_assert(maeve_event(p_docu, T5_MSG_CELL_MERGE, P_DATA_NONE)); /* flush */

        if(p_docu->modified)
            ++res;
    }

    return(res);
}

_Check_return_
static STATUS
modified_docno_save(
    _InVal_     DOCNO docno)
{
    STATUS status = STATUS_OK;
    const P_DOCU p_docu = p_docu_from_docno(docno);
    const OBJECT_ID object_id = OBJECT_ID_SKEL;
    const T5_MESSAGE t5_message = T5_CMD_SAVE;
    PC_CONSTRUCT_TABLE p_construct_table;
    ARGLIST_HANDLE arglist_handle;

    if(status_ok(status = arglist_prepare_with_construct(&arglist_handle, object_id, t5_message, &p_construct_table)))
    {
        /* may fail to save or be saved to unsafe receiver */
        if(status_ok(status = execute_command(p_docu, t5_message, &arglist_handle, object_id)))
            if(p_docu->modified)
                status = STATUS_CANCEL;

        arglist_dispose(&arglist_handle);
    }

    return(status);
}

#if WINDOWS
#define QUIT_QUERY_BUTTON_H     ((DIALOG_STDPUSHBUTTON_H) * 5) / 4 /* "Nicht speichern" */
#else
#define QUIT_QUERY_BUTTON_H     (DIALOG_PUSHBUTTONOVH_H + DIALOG_SYSCHARS_H("Discard "))
#endif
#if 1
#define QUIT_QUERY_BUTTON_GAP_H DIALOG_STDSPACING_H /*poked*/
#else
/*#define QUIT_QUERY_ACROSS_H   ((9 * QUIT_QUERY_BUTTON_H) / 2)*/
#define QUIT_QUERY_BUTTON_GAP_H ((QUIT_QUERY_ACROSS_H - 3 * QUIT_QUERY_BUTTON_H) / 2)
#endif

enum QUIT_QUERY_CONTROL_IDS
{
    QUIT_QUERY_ID_SAVE = IDOK,

    QUIT_QUERY_ID_TEXT_1 = 30,
    QUIT_QUERY_ID_TEXT_2,
    QUIT_QUERY_ID_TEXT_3,
    QUIT_QUERY_ID_DISCARD
};

static /*poked*/ DIALOG_CONTROL
quit_query_text_1 =
{
    QUIT_QUERY_ID_TEXT_1, DIALOG_MAIN_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { 0, DIALOG_STDTEXT_V + 0, 0/*QUIT_QUERY_ACROSS_H*/, DIALOG_STDTEXT_V },
    { DRT(LTLT, STATICTEXT) }
};

static /*poked*/ DIALOG_CONTROL_DATA_STATICTEXT
quit_query_text_1_data = { { UI_TEXT_TYPE_NONE }, { 0 /*left_text*/, 1 /*centre_text*/ } };

static const DIALOG_CONTROL
quit_query_text_2 =
{
    QUIT_QUERY_ID_TEXT_2, DIALOG_MAIN_GROUP,
    { QUIT_QUERY_ID_TEXT_1, QUIT_QUERY_ID_TEXT_1, QUIT_QUERY_ID_TEXT_1 },
    { 0, DIALOG_STDTEXT_V + DIALOG_STDSPACING_V, 0, DIALOG_STDTEXT_V },
    { DRT(LBRT, STATICTEXT) }
};

static const DIALOG_CONTROL_DATA_STATICTEXT
quit_query_text_2_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_QUIT_QUERY_SECOND), { 0 /*left_text*/, 1 /*centre_text*/ } };

static const DIALOG_CONTROL
quit_query_text_3 =
{
    QUIT_QUERY_ID_TEXT_3, DIALOG_MAIN_GROUP,
    { QUIT_QUERY_ID_TEXT_2, QUIT_QUERY_ID_TEXT_2, QUIT_QUERY_ID_TEXT_2 },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDTEXT_V },
    { DRT(LBRT, STATICTEXT) }
};

static const DIALOG_CONTROL_DATA_STATICTEXT
quit_query_text_3_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_QUIT_QUERY_SURE), { 0 /*left_text*/, 1 /*centre_text*/ } };

/* RISC OS: Discard, Cancel, [Save] */
/* Windows: [Save], Discard, Cancel */

static const DIALOG_CONTROL
SDC_query_save =
{
    QUIT_QUERY_ID_SAVE, DIALOG_CONTROL_WINDOW,
#if RISCOS
    { DIALOG_CONTROL_SELF, DIALOG_MAIN_GROUP, DIALOG_MAIN_GROUP },
    { QUIT_QUERY_BUTTON_H + (2 * DIALOG_DEFPUSHEXTRA_H), DIALOG_STDTEXT_V + DIALOG_STDSPACING_V, 0, DIALOG_DEFPUSHBUTTON_V },
    { DRT(RBRT, PUSHBUTTON), 1 /*tabstop*/ }
#else
    { DIALOG_MAIN_GROUP, DIALOG_MAIN_GROUP },
    { 0, DIALOG_STDTEXT_V + DIALOG_STDSPACING_V, QUIT_QUERY_BUTTON_H + (2 * DIALOG_DEFPUSHEXTRA_H), DIALOG_DEFPUSHBUTTON_V },
    { DRT(LBLT, PUSHBUTTON), 1 /*tabstop*/ }
#endif
};

static const DIALOG_CONTROL_DATA_PUSHBUTTON
quit_query_save_data = { { QUERY_COMPLETION_SAVE }, UI_TEXT_INIT_RESID(MSG_QUERY_SAVE) };

static /*poked*/ DIALOG_CONTROL
SDC_query_discard =
{
    QUIT_QUERY_ID_DISCARD, DIALOG_CONTROL_WINDOW,
#if RISCOS
    { DIALOG_MAIN_GROUP, IDCANCEL, DIALOG_CONTROL_SELF, IDCANCEL },
    { 0, 0, QUIT_QUERY_BUTTON_H, 0 },
    { DRT(LTLB, PUSHBUTTON), 1 /*tabstop*/ }
#else
    { QUIT_QUERY_ID_SAVE, IDCANCEL, DIALOG_CONTROL_SELF, IDCANCEL },
    { QUIT_QUERY_BUTTON_GAP_H /*poked*/, 0, QUIT_QUERY_BUTTON_H, 0 },
    { DRT(RTLB, PUSHBUTTON), 1 /*tabstop*/ }
#endif
};

static const DIALOG_CONTROL_DATA_PUSHBUTTON
quit_query_discard_data = { { QUERY_COMPLETION_DISCARD }, UI_TEXT_INIT_RESID(MSG_QUERY_DISCARD) };

static /*poked*/ DIALOG_CONTROL
SDC_query_cancel =
{
    IDCANCEL, DIALOG_CONTROL_WINDOW,
#if RISCOS
    { QUIT_QUERY_ID_DISCARD, QUIT_QUERY_ID_SAVE, DIALOG_CONTROL_SELF, QUIT_QUERY_ID_SAVE },
    { QUIT_QUERY_BUTTON_GAP_H /*poked*/, -DIALOG_DEFPUSHEXTRA_V, QUIT_QUERY_BUTTON_H, -DIALOG_DEFPUSHEXTRA_V },
    { DRT(RTLB, PUSHBUTTON), 1 /*tabstop*/ }
#else
    { DIALOG_CONTROL_SELF, QUIT_QUERY_ID_SAVE, DIALOG_MAIN_GROUP, QUIT_QUERY_ID_SAVE },
    { QUIT_QUERY_BUTTON_H, -DIALOG_DEFPUSHEXTRA_V, 0, -DIALOG_DEFPUSHEXTRA_V },
    { DRT(RTRB, PUSHBUTTON), 1 /*tabstop*/ }
#endif
};

static const DIALOG_CTL_CREATE
quit_query_ctl_create[] =
{
    { { &dialog_main_group },   NULL },
    { { &quit_query_text_1 },   &quit_query_text_1_data  },
    { { &quit_query_text_2 },   &quit_query_text_2_data  },
    { { &quit_query_text_3 },   &quit_query_text_3_data  },

    { { &SDC_query_save },      &quit_query_save_data    },
    { { &SDC_query_discard },   &quit_query_discard_data },
    { { &SDC_query_cancel },    &stdbutton_cancel_data   }
};

/******************************************************************************
*
* returns status to say whether we can complete the requested closedown
*
******************************************************************************/

_Check_return_
static STATUS
query_quit_do_query(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     S32 n_modified)
{
    STATUS completion_code = QUERY_COMPLETION_DISCARD;
    STATUS status;
    PIXIT width_1, buttongap_h;
    QUICK_TBLOCK_WITH_BUFFER(quick_tblock_1, 32);
    QUICK_TBLOCK_WITH_BUFFER(quick_tblock_2, 32);
    quick_tblock_with_buffer_setup(quick_tblock_1);
    quick_tblock_with_buffer_setup(quick_tblock_2);

    /* ensure_memory_froth'ed */
    status_assert(quick_tblock_printf(&quick_tblock_1, resource_lookup_tstr((n_modified == 1) ? MSG_DIALOG_QUIT_QUERY_ONE : MSG_DIALOG_QUIT_QUERY_MANY), n_modified, product_ui_id()));
    status_assert(quick_tblock_nullch_add(&quick_tblock_1));

    quit_query_text_1_data.caption.type = UI_TEXT_TYPE_TSTR_PERM; /* well, we are modal */
    quit_query_text_1_data.caption.text.tstr = quick_tblock_tstr(&quick_tblock_1);

    /* try to make sensible width for the long caption */
    width_1 = ui_width_from_tstr(quit_query_text_1_data.caption.text.tstr);

    /* but not so small as to have buttons crashing into each other */
    if( width_1 < (PIXIT) ((3 * QUIT_QUERY_BUTTON_H) + (2 * DIALOG_DEFPUSHEXTRA_H) + (2 * DIALOG_STDSPACING_H)) )
        width_1 = (PIXIT) ((3 * QUIT_QUERY_BUTTON_H) + (2 * DIALOG_DEFPUSHEXTRA_H) + (2 * DIALOG_STDSPACING_H));

    quit_query_text_1.relative_offset[2] = width_1;

    /* centralise the middle button of the three */
    buttongap_h = ((width_1 - (PIXIT) ((3 * QUIT_QUERY_BUTTON_H) + (2 * DIALOG_DEFPUSHEXTRA_H))) / 2);
#if RISCOS
    SDC_query_cancel.relative_offset[0] = buttongap_h;
#else
    SDC_query_discard.relative_offset[0] = buttongap_h;
#endif

    {
    DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
    dialog_cmd_process_dbox_setup_ui_text(&dialog_cmd_process_dbox, quit_query_ctl_create, elemof32(quit_query_ctl_create), get_ui_text_product_ui_id());
#if RISCOS
    dialog_cmd_process_dbox.bits.use_riscos_menu = 1;
    dialog_cmd_process_dbox.riscos.menu = DIALOG_RISCOS_STANDALONE_MENU;
#endif
    dialog_cmd_process_dbox.bits.dialog_position_type = DIALOG_POSITION_CENTRE_MOUSE;
  /*dialog_cmd_process_dbox.p_proc_client = NULL;*/
    completion_code = status = object_call_DIALOG_with_docu(p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox);
    } /*block*/

    quick_tblock_dispose(&quick_tblock_1);
    quick_tblock_dispose(&quick_tblock_2);

    if(status_fail(status))
    {
        reperr_null(status);

        completion_code = STATUS_FAIL;
    }

    return(completion_code);
}

_Check_return_
extern STATUS
query_quit(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     S32 n_modified)
{
    STATUS completion_code = QUERY_COMPLETION_DISCARD;

    if(0 != n_modified)
    {
        BOOL do_query = TRUE;
        STATUS status;

        if(status_fail(status = ensure_memory_froth()))
        {
            QUIT_OBJECTION quit_objection;
            quit_objection.objections = 0;
            status_assert(maeve_service_event(P_DOCU_NONE, T5_MSG_QUIT_OBJECTION, &quit_objection));
            if(quit_objection.objections)
            {
                reperr_null(STATUS_NOMEM);
                return(STATUS_FAIL);
            }

            do_query = FALSE;
        }

        if(do_query)
            completion_code = query_quit_do_query(p_docu, n_modified);
    }

    /* QUERY_COMPLETION_DISCARD: quit application, losing data */
    /* QUERY_COMPLETION_SAVE:    save files, then quit application */
    /* STATUS_FAIL:              abandon closedown */

    {
    DOCNO docno = DOCNO_NONE;

    while((QUERY_COMPLETION_SAVE == completion_code) && ((docno = docno_enum_docs(docno)) != DOCNO_NONE))
    {
        /* If punter cancels any save, he means abort the shutdown */
        switch(query_save_modified(p_docu_from_docno_valid(docno)))
        {
        case QUERY_COMPLETION_SAVE:
            {
            STATUS status = modified_docno_save(docno);

            if(status_fail(status))
            {
                reperr_null(status);

                completion_code = STATUS_FAIL;
            }

            break;
            }

        case STATUS_FAIL:
            completion_code = STATUS_FAIL;
            break;

        default:
            break;
        }
    }
    } /*block*/

    /* if not aborted then all modified documents either saved or ignored - delete all documents (must be at least one) */
    if(STATUS_FAIL != completion_code)
    {
#if defined(USE_GLOBAL_CLIPBOARD)
        host_release_global_clipboard(TRUE); /* doing it now avoids any deferred clipboard rendering request */ /* harmless on RISC OS */
#endif /* USE_GLOBAL_CLIPBOARD */

        docno_close_all();

        return(STATUS_OK);
    }

    return(STATUS_FAIL);
}

/*
linked group save query box
*/

#define SAVE_LINKED_QUERY_BUTTON_H     (DIALOG_PUSHBUTTONOVH_H + DIALOG_SYSCHARS_H("Discard group "))
#define SAVE_LINKED_QUERY_ACROSS_H     DIALOG_SYSCHARS_H("...One or more documents in the group have been modified...")
#define SAVE_LINKED_QUERY_BUTTON_GAP_H ((SAVE_LINKED_QUERY_ACROSS_H - 3 * SAVE_LINKED_QUERY_BUTTON_H) / 2)

enum SAVE_LINKED_QUERY_CONTROL_IDS
{
    SAVE_LINKED_QUERY_ID_SAVE_ALL = IDOK,

    SAVE_LINKED_QUERY_ID_TEXT_1 = 30,
    SAVE_LINKED_QUERY_ID_DISCARD_ALL
};

static const DIALOG_CONTROL
save_linked_query_text_1 =
{
    SAVE_LINKED_QUERY_ID_TEXT_1, DIALOG_CONTROL_WINDOW,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { 0, 0, SAVE_LINKED_QUERY_ACROSS_H, DIALOG_STDTEXT_V },
    { DRT(LTLT, STATICTEXT) }
};

static const DIALOG_CONTROL_DATA_STATICTEXT
save_linked_query_text_1_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_SAVE_QUERY_LINKED), { 0 /*left_text*/, 1 /*centre_text*/ } };

static const DIALOG_CONTROL
save_linked_query_save_all =
{
    SAVE_LINKED_QUERY_ID_SAVE_ALL, DIALOG_CONTROL_WINDOW,
    { SAVE_LINKED_QUERY_ID_TEXT_1, SAVE_LINKED_QUERY_ID_TEXT_1 },
    { 0, DIALOG_STDSPACING_V, SAVE_LINKED_QUERY_BUTTON_H, DIALOG_DEFPUSHBUTTON_V },
    { DRT(LBLT, PUSHBUTTON), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_PUSHBUTTON
save_linked_query_save_all_data = { { LINKED_QUERY_COMPLETION_SAVE_ALL }, UI_TEXT_INIT_RESID(MSG_DIALOG_SAVE_ALL) };

static const DIALOG_CONTROL
save_linked_query_discard_all =
{
    SAVE_LINKED_QUERY_ID_DISCARD_ALL, DIALOG_CONTROL_WINDOW,
    { SAVE_LINKED_QUERY_ID_SAVE_ALL, SAVE_LINKED_QUERY_ID_SAVE_ALL, DIALOG_CONTROL_SELF, SAVE_LINKED_QUERY_ID_SAVE_ALL },
    { SAVE_LINKED_QUERY_BUTTON_GAP_H, -DIALOG_DEFPUSHEXTRA_V, SAVE_LINKED_QUERY_BUTTON_H, -DIALOG_DEFPUSHEXTRA_V },
    { DRT(RTLB, PUSHBUTTON), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_PUSHBUTTON
save_linked_query_discard_all_data = { { LINKED_QUERY_COMPLETION_DISCARD_ALL }, UI_TEXT_INIT_RESID(MSG_DIALOG_DISCARD_ALL) };

static const DIALOG_CONTROL
save_linked_query_cancel =
{
    IDCANCEL, DIALOG_CONTROL_WINDOW,
    { DIALOG_CONTROL_SELF, SAVE_LINKED_QUERY_ID_DISCARD_ALL, SAVE_LINKED_QUERY_ID_TEXT_1, SAVE_LINKED_QUERY_ID_DISCARD_ALL },
    { SAVE_LINKED_QUERY_BUTTON_H, 0, 0, 0 },
    { DRT(RTRB, PUSHBUTTON), 1 /*tabstop*/ }
};

static const DIALOG_CTL_CREATE
save_linked_query_ctl_create[] =
{
    { { &save_linked_query_text_1 },      &save_linked_query_text_1_data },

    { { &save_linked_query_save_all },    &save_linked_query_save_all_data },
    { { &save_linked_query_discard_all }, &save_linked_query_discard_all_data },
    { { &save_linked_query_cancel },      &stdbutton_cancel_data }
};

_Check_return_
extern STATUS
query_save_linked(
    _DocuRef_   P_DOCU p_docu)
{
    STATUS status;

    {
    DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
    dialog_cmd_process_dbox_setup_ui_text(&dialog_cmd_process_dbox, save_linked_query_ctl_create, elemof32(save_linked_query_ctl_create), get_ui_text_product_ui_id());
#if RISCOS
    dialog_cmd_process_dbox.bits.use_riscos_menu = 1;
    dialog_cmd_process_dbox.riscos.menu = DIALOG_RISCOS_STANDALONE_MENU;
#endif
    dialog_cmd_process_dbox.bits.dialog_position_type = DIALOG_POSITION_CENTRE_MOUSE;
  /*dialog_cmd_process_dbox.p_proc_client = NULL;*/
    status = object_call_DIALOG_with_docu(p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox);
    } /*block*/

    /* LINKED_QUERY_COMPLETION_DISCARD_ALL */
    /* LINKED_QUERY_COMPLETION_SAVE_ALL */
    /* STATUS_FAIL: abandon closure */

    if(status_fail(status))
    {
        reperr_null(status);

        status = STATUS_FAIL;
    }

    return(status);
}

#ifndef MAX_FILENAME_FOR_SAVE_QUERY
/* subjective value */
#define MAX_FILENAME_FOR_SAVE_QUERY 64 /*44*/ /*48*/
#endif

/*#define CLOSE_QUERY_ACROSS_H     MAX(3 * CLOSE_QUERY_BUTTON_H + 2 * DIALOG_STDSPACING_H, ((MAX_FILENAME_FOR_SAVE_QUERY + sizeof32("''?")) * DIALOG_SYSCHAR_H))*/
#if 1
#define CLOSE_QUERY_BUTTON_H     QUIT_QUERY_BUTTON_H /* reuse these buttons */
#elif WINDOWS
#define CLOSE_QUERY_BUTTON_H     DIALOG_STDPUSHBUTTON_H
#else
#define CLOSE_QUERY_BUTTON_H     (DIALOG_PUSHBUTTONOVH_H + DIALOG_SYSCHARS_H("Discard "))
#endif
/*#define CLOSE_QUERY_BUTTON_GAP_H ((CLOSE_QUERY_ACROSS_H - 3 * CLOSE_QUERY_BUTTON_H) / 2)*/

static const DIALOG_CONTROL
close_query_text_1 =
{
    QUIT_QUERY_ID_TEXT_1, DIALOG_MAIN_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT, QUIT_QUERY_ID_TEXT_2 },
    { 0, DIALOG_STDTEXT_V + 0, 0, DIALOG_STDTEXT_V },
    { DRT(LTRT, STATICTEXT) }
};

static const DIALOG_CONTROL_DATA_STATICTEXT
close_query_text_1_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_CLOSE_QUERY_1), { 0 /*left_text*/, 1 /*centre_text*/ } };

static /*poked*/ DIALOG_CONTROL
close_query_text_2 =
{
    QUIT_QUERY_ID_TEXT_2, DIALOG_MAIN_GROUP,
    { QUIT_QUERY_ID_TEXT_1, QUIT_QUERY_ID_TEXT_1 },
    { 0, DIALOG_STDSPACING_V, 0 /*CLOSE_QUERY_ACROSS_H*/, DIALOG_STDTEXT_V },
    { DRT(LBLT, STATICTEXT) }
};

static /*poked*/ DIALOG_CONTROL_DATA_STATICTEXT
close_query_text_2_data = { { UI_TEXT_TYPE_NONE }, { 0 /*left_text*/, 1 /*centre_text*/ } };

#if 0
static const DIALOG_CONTROL
close_query_save =
{
    QUIT_QUERY_ID_SAVE, DIALOG_CONTROL_WINDOW,
    { QUIT_QUERY_ID_TEXT_2, QUIT_QUERY_ID_TEXT_2 },
    { 0, DIALOG_STDTEXT_V + DIALOG_STDSPACING_V, CLOSE_QUERY_BUTTON_H, DIALOG_DEFPUSHBUTTON_V },
    { DRT(LBLT, PUSHBUTTON), 1 /*tabstop*/ }
};
#endif

static const DIALOG_CONTROL_DATA_PUSHBUTTON
close_query_save_data = { { QUERY_COMPLETION_SAVE }, UI_TEXT_INIT_RESID(MSG_QUERY_SAVE) };

#if 0
static /*poked*/ DIALOG_CONTROL
close_query_discard =
{
    QUIT_QUERY_ID_DISCARD, DIALOG_CONTROL_WINDOW,
    { QUIT_QUERY_ID_SAVE, QUIT_QUERY_ID_SAVE, DIALOG_CONTROL_SELF, QUIT_QUERY_ID_SAVE },
    { CLOSE_QUERY_BUTTON_GAP_H /*poked*/, -DIALOG_DEFPUSHEXTRA_V, CLOSE_QUERY_BUTTON_H, -DIALOG_DEFPUSHEXTRA_V },
    { DRT(RTLB, PUSHBUTTON), 1 /*tabstop*/ }
};
#endif

static const DIALOG_CONTROL_DATA_PUSHBUTTON
close_query_discard_data = { { QUERY_COMPLETION_DISCARD }, UI_TEXT_INIT_RESID(MSG_QUERY_DISCARD) };

#if 0
static const DIALOG_CONTROL
close_query_cancel =
{
    IDCANCEL, DIALOG_CONTROL_WINDOW,
    { DIALOG_CONTROL_SELF, QUIT_QUERY_ID_DISCARD, QUIT_QUERY_ID_TEXT_2, QUIT_QUERY_ID_DISCARD },
    { CLOSE_QUERY_BUTTON_H, 0, 0, 0 },
    { DRT(RTRB, PUSHBUTTON), 1 /*tabstop*/ }
};
#endif

static const DIALOG_CTL_CREATE
close_query_ctl_create[] =
{
    { { &dialog_main_group },   NULL },
    { { &close_query_text_1 },  &close_query_text_1_data  },
    { { &close_query_text_2 },  &close_query_text_2_data  },

    { { &SDC_query_save },      &close_query_save_data    },
    { { &SDC_query_discard },   &close_query_discard_data },
    { { &SDC_query_cancel },    &stdbutton_cancel_data    }
};

_Check_return_
extern STATUS
query_save_modified(
    _DocuRef_   P_DOCU p_docu)
{
    STATUS status;
    TCHARZ mess_tstr_buf[BUF_MAX_PATHSTRING];
    PIXIT width_2, buttongap_h;

    status_assert(maeve_event(p_docu, T5_MSG_CELL_MERGE, P_DATA_NONE)); /* flush */

    if(!p_docu->modified)
        return(QUERY_COMPLETION_DISCARD);

    {
    PTSTR filename;
    U32 s;
    QUICK_TBLOCK_WITH_BUFFER(quick_tblock, 64);
    quick_tblock_with_buffer_setup(quick_tblock);

    status_assert(name_make_wholename(&p_docu->docu_name, &quick_tblock, TRUE));

    filename = quick_tblock_tchars_wr(&quick_tblock);

    s = tstrlen32(filename);

    if(s > MAX_FILENAME_FOR_SAVE_QUERY)
    {
        PTSTR p2 = filename + s; /* point to end of filename */
        PTSTR p, q;
        BOOL new_dotty = FALSE;

#if RISCOS
        if(NULL != (p = strchr(filename, CH_DOLLAR_SIGN)))
        {
            ++p; /* skip root */
            new_dotty = TRUE;
        }
#else
        if(FILE_DIR_SEP_CH == filename[0] && FILE_DIR_SEP_CH == filename[1])
        {
            if(NULL != (p = tstrchr(filename + 2, FILE_DIR_SEP_CH)))
            if(NULL != (p = tstrchr(p + 1, FILE_DIR_SEP_CH)))
            {
                ++p; /* skip all of UNC root */
                new_dotty = TRUE;
            }
        }
        else if(NULL != (p = tstrchr(filename, FILE_ROOT_CH)))
        {
            ++p; /* skip root */
            if(FILE_DIR_SEP_CH == *p) /* skip this on Windows if present */
                ++p;
            new_dotty = TRUE;
        }
#endif
        if(new_dotty)
        {
            U32 s1;
            *p++ = CH_FULL_STOP;
            *p++ = CH_FULL_STOP;
            *p++ = CH_FULL_STOP;
            s1 = PtrDiffElemU32(p, filename); /* length of bit to preserve */
            p2 -= MAX_FILENAME_FOR_SAVE_QUERY;
            p2 += s1; /* can now consider using this much */
            /* if we can find a directory separator in this bit then skip to it */
            if(NULL != (q = tstrchr(p2, FILE_DIR_SEP_CH)))
            {
                p2 = q;
#if RISCOS
                ++p2; /* skip over it too */
#endif
            }
            memmove32(p, p2, sizeof32(*p2) * (tstrlen32(p2) + 1)); /* copy it down */
        }
        else
        {
            /* skip less important leading part of name */
            filename = p2;
            filename -= MAX_FILENAME_FOR_SAVE_QUERY;

            filename[0] = CH_FULL_STOP;
            filename[1] = CH_FULL_STOP;
            filename[2] = CH_FULL_STOP;
        }
    }

    consume_int(tstr_xsnprintf(mess_tstr_buf, elemof32(mess_tstr_buf),
                               resource_lookup_tstr(MSG_DIALOG_CLOSE_QUERY_2),
                               filename));

    quick_tblock_dispose(&quick_tblock);
    } /*block*/

    close_query_text_2_data.caption.type = UI_TEXT_TYPE_TSTR_PERM; /* well, we are modal */
    close_query_text_2_data.caption.text.tstr = mess_tstr_buf;

    /* try to make sensible width for the long caption */
    width_2 = ui_width_from_tstr(close_query_text_2_data.caption.text.tstr);

    /* but not so small as to have buttons crashing into each other */
    if( width_2 < (PIXIT) ((3 * CLOSE_QUERY_BUTTON_H) + (2 * DIALOG_DEFPUSHEXTRA_H) + (2 * DIALOG_STDSPACING_H)) )
        width_2 = (PIXIT) ((3 * CLOSE_QUERY_BUTTON_H) + (2 * DIALOG_DEFPUSHEXTRA_H) + (2 * DIALOG_STDSPACING_H));

    close_query_text_2.relative_offset[2] = width_2;

    /* centralise the middle button of the three */
    buttongap_h = ((width_2 - (PIXIT) ((3 * CLOSE_QUERY_BUTTON_H) + (2 * DIALOG_DEFPUSHEXTRA_H))) / 2);
#if RISCOS
    SDC_query_cancel.relative_offset[0] = buttongap_h; 
#else
    SDC_query_discard.relative_offset[0] = buttongap_h;
#endif

    {
    DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
    dialog_cmd_process_dbox_setup_ui_text(&dialog_cmd_process_dbox, close_query_ctl_create, elemof32(close_query_ctl_create), get_ui_text_product_ui_id());
    dialog_cmd_process_dbox.caption.type = UI_TEXT_TYPE_TSTR_PERM;
#if RISCOS
    dialog_cmd_process_dbox.bits.use_riscos_menu = 1;
    dialog_cmd_process_dbox.riscos.menu = DIALOG_RISCOS_STANDALONE_MENU;
#endif
    dialog_cmd_process_dbox.bits.dialog_position_type = DIALOG_POSITION_CENTRE_MOUSE;
    /*dialog_cmd_process_dbox.p_proc_client = NULL;*/
    status = object_call_DIALOG_with_docu(p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox);
    } /*block*/

    /* QUERY_COMPLETION_DISCARD: discard file */
    /* QUERY_COMPLETION_SAVE:    save file then discard */
    /* STATUS_FAIL:              abandon closure */

    if(status_fail(status))
    {
        reperr_null(status);

        status = STATUS_FAIL;
    }

    return(status);
}

_Check_return_
extern STATUS
save_all_modified_docs_for_shutdown(void)
{
    STATUS completion_code = STATUS_OK;
    DOCNO docno = DOCNO_NONE;

    while(/*status_ok(completion_code) &&*/ (DOCNO_NONE != (docno = docno_enum_docs(docno))))
    {
        STATUS status = modified_docno_save(docno);

        if(status_fail(status))
        {
            reperr_null(status); /* may be no time to report (but still log it) */

            completion_code = STATUS_FAIL;
        }
    }

    /* if not aborted then all modified documents saved */
    if(STATUS_FAIL != completion_code)
        return(STATUS_OK);

    return(STATUS_FAIL);
}

extern void
process_close_request(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _In_        BOOL closing_a_doc,
    _In_        BOOL closing_a_view,
    _InVal_     BOOL opening_a_directory)
{
    STATUS status = STATUS_OK;
    BOOL closing_docs = 0;
    ARRAY_HANDLE h_linked = 0;
    S32 n_linked = 0;

    status_line_auto_clear(p_docu);

    if(closing_a_view && (p_docu->n_views == 1))
        closing_a_doc = 1;

    if(closing_a_doc)
    {
        S32 n_docs_not_thunks = 0;
        S32 n_docs_not_thunks_modified = 0;

        n_linked = docno_get_linked_docs(p_docu, &h_linked);

        { /* count the number of real documents (those not just thunks) present (and count modified too) in the group */
        ARRAY_INDEX i;
        P_DOCNO p_docno = array_range(&h_linked, DOCNO, 0, n_linked);

        for(i = 0; i < n_linked; ++i, ++p_docno)
        {
            if(!p_docu_from_docno(*p_docno)->flags.has_data)
                continue;

            n_docs_not_thunks += 1;

            if(p_docu_from_docno(*p_docno)->modified)
                n_docs_not_thunks_modified += 1;
        }
        } /*block*/

        if(n_docs_not_thunks > 1)
        {
            switch(query_close_linked(p_docu))
            {
            case LINKED_QUERY_COMPLETION_CLOSE_ALL:
                closing_docs = 1;
                closing_a_doc = 0;
                closing_a_view = 0;
                break;

            case LINKED_QUERY_COMPLETION_CLOSE_DOC:
                closing_docs = 0;
                closing_a_doc = 1;
                break;

            default:
                closing_docs = 0;
                closing_a_doc = 0;
                closing_a_view = 0;
                break;
            }
        }

        if(closing_docs)
        {
            if(n_docs_not_thunks_modified)
            {
                switch(query_save_linked(p_docu))
                {
                case LINKED_QUERY_COMPLETION_SAVE_ALL:
                    {
                    ARRAY_INDEX i;
                    P_DOCNO p_docno = array_range(&h_linked, DOCNO, 0, n_linked);

                    for(i = 0; i < n_linked; ++i, ++p_docno)
                    {
                        if(!p_docu_from_docno(*p_docno)->flags.has_data) /* SKS after 1.19/11 28jan95 - used to consider thunks too! */
                            continue;

                        status_break(status = modified_docno_save(*p_docno));
                    }

                    break;
                    }

                case LINKED_QUERY_COMPLETION_DISCARD_ALL:
                    break;

                default:
                    closing_docs = 0;
                    closing_a_doc = 0;
                    closing_a_view = 0;
                    break;
                }
            }
        }
        else if(closing_a_doc)
        {
            if(p_docu->modified)
            {
                switch(query_save_modified(p_docu))
                {
                case QUERY_COMPLETION_SAVE:
                    status = modified_docno_save(docno_from_p_docu(p_docu));
                    break;

                case STATUS_FAIL:
                    closing_docs = 0;
                    closing_a_doc = 0;
                    closing_a_view = 0;
                    break;

                default:
                    break;
                }
            }
        }
    }

    if(status_ok(status))
        if(opening_a_directory)
        {
            QUICK_TBLOCK_WITH_BUFFER(quick_tblock, 64);
            quick_tblock_with_buffer_setup(quick_tblock);

            if(status_ok(status = name_make_wholename(&p_docu->docu_name, &quick_tblock, FALSE)))
            {
#if RISCOS
                filer_opendir(quick_tblock_tstr(&quick_tblock));
#endif
            }

            quick_tblock_dispose(&quick_tblock);
        }

    if(status_ok(status))
    {
        if(closing_docs)
        {
            ARRAY_INDEX i;
            P_DOCNO p_docno = array_range(&h_linked, DOCNO, 0, n_linked);

            for(i = 0; i < n_linked; ++i, ++p_docno)
                docno_close(p_docno);
        }
        else if(closing_a_view)
        {
            SKELCMD_CLOSE_VIEW skelcmd_close_view;
            skelcmd_close_view.p_view = p_view;
            object_skel(p_docu, T5_MSG_SKELCMD_VIEW_DESTROY, &skelcmd_close_view);
        }
    }

    if(status_fail(status))
        reperr_null(status);

    al_array_dispose(&h_linked);
}

/*
linked group close query box
*/

#define CLOSE_LINKED_QUERY_BUTTON_H     (DIALOG_PUSHBUTTONOVH_H + DIALOG_SYSCHARS_H("Close group"))
#define CLOSE_LINKED_QUERY_ACROSS_H     DIALOG_SYSCHARS_H("...This document is part of a linked group...")
#define CLOSE_LINKED_QUERY_BUTTON_GAP_H ((CLOSE_LINKED_QUERY_ACROSS_H - 3 * CLOSE_LINKED_QUERY_BUTTON_H) / 2)

enum CLOSE_LINKED_QUERY_CONTROL_IDS
{
    CLOSE_LINKED_QUERY_ID_CLOSE_ALL = IDOK,

    CLOSE_LINKED_QUERY_ID_TEXT_1 = 30,
    CLOSE_LINKED_QUERY_ID_CLOSE_DOC
};

static const DIALOG_CONTROL
close_linked_query_text_1 =
{
    CLOSE_LINKED_QUERY_ID_TEXT_1, DIALOG_CONTROL_WINDOW,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { 0, 0, CLOSE_LINKED_QUERY_ACROSS_H, DIALOG_STDTEXT_V },
    { DRT(LTLT, STATICTEXT) }
};

static const DIALOG_CONTROL_DATA_STATICTEXT
close_linked_query_text_1_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_CLOSE_QUERY_LINKED), { 0 /*left_text*/, 1 /*centre_text*/ } };

static const DIALOG_CONTROL
close_linked_query_close_all =
{
    CLOSE_LINKED_QUERY_ID_CLOSE_ALL, DIALOG_CONTROL_WINDOW,
    { CLOSE_LINKED_QUERY_ID_TEXT_1, CLOSE_LINKED_QUERY_ID_TEXT_1 },
    { 0, DIALOG_STDSPACING_V, CLOSE_LINKED_QUERY_BUTTON_H, DIALOG_DEFPUSHBUTTON_V },
    { DRT(LBLT, PUSHBUTTON), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_PUSHBUTTON
close_linked_query_close_all_data = { { LINKED_QUERY_COMPLETION_CLOSE_ALL }, UI_TEXT_INIT_RESID(MSG_DIALOG_CLOSE_ALL) };

static const DIALOG_CONTROL
close_linked_query_close_doc =
{
    CLOSE_LINKED_QUERY_ID_CLOSE_DOC, DIALOG_CONTROL_WINDOW,
    { CLOSE_LINKED_QUERY_ID_CLOSE_ALL, CLOSE_LINKED_QUERY_ID_CLOSE_ALL, DIALOG_CONTROL_SELF, CLOSE_LINKED_QUERY_ID_CLOSE_ALL },
    { CLOSE_LINKED_QUERY_BUTTON_GAP_H, -DIALOG_DEFPUSHEXTRA_V, CLOSE_LINKED_QUERY_BUTTON_H, -DIALOG_DEFPUSHEXTRA_V },
    { DRT(RTLB, PUSHBUTTON), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_PUSHBUTTON
close_linked_query_close_doc_data = { { LINKED_QUERY_COMPLETION_CLOSE_DOC }, UI_TEXT_INIT_RESID(MSG_DIALOG_CLOSE_DOCUMENT) };

static const DIALOG_CONTROL
close_linked_query_cancel =
{
    IDCANCEL, DIALOG_CONTROL_WINDOW,
    { DIALOG_CONTROL_SELF, CLOSE_LINKED_QUERY_ID_CLOSE_DOC, CLOSE_LINKED_QUERY_ID_TEXT_1, CLOSE_LINKED_QUERY_ID_CLOSE_DOC },
    { CLOSE_LINKED_QUERY_BUTTON_H, 0, 0, 0 },
    { DRT(RTRB, PUSHBUTTON), 1 /*tabstop*/ }
};

static const DIALOG_CTL_CREATE
close_linked_query_ctl_create[] =
{
    { { &close_linked_query_text_1 }, &close_linked_query_text_1_data },

    { { &close_linked_query_close_all }, &close_linked_query_close_all_data },
    { { &close_linked_query_close_doc }, &close_linked_query_close_doc_data },
    { { &close_linked_query_cancel }, &stdbutton_cancel_data }
};

_Check_return_
static STATUS
query_close_linked(
    _DocuRef_   P_DOCU p_docu)
{
    STATUS status;

    {
    DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
    dialog_cmd_process_dbox_setup_ui_text(&dialog_cmd_process_dbox, close_linked_query_ctl_create, elemof32(close_linked_query_ctl_create), get_ui_text_product_ui_id());
#if RISCOS
    dialog_cmd_process_dbox.bits.use_riscos_menu = 1;
    dialog_cmd_process_dbox.riscos.menu = DIALOG_RISCOS_STANDALONE_MENU;
#endif
    dialog_cmd_process_dbox.bits.dialog_position_type = DIALOG_POSITION_CENTRE_MOUSE;
    /*dialog_cmd_process_dbox.p_proc_client = NULL;*/
    status = object_call_DIALOG_with_docu(p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox);
    } /*block*/

    /* LINKED_QUERY_COMPLETION_CLOSE_DOC */
    /* LINKED_QUERY_COMPLETION_CLOSE_ALL */
    /* STATUS_FAIL:              abandon closure */

    if(STATUS_CANCEL == status)
        status = STATUS_FAIL;
    else if(status_fail(status))
    {
        reperr_null(status);

        status = STATUS_FAIL;
    }

    return(status);
}

/******************************************************************************
*
* return the ears surrounding an object
*
******************************************************************************/

extern void
pixit_rect_get_ears(
    _OutRef_    P_PIXIT_RECT_EARS p_pixit_rect_ears,
    _InRef_     PC_PIXIT_RECT p_pixit_rect,
    _InRef_     PC_PIXIT_POINT p_one_program_pixel)
{
    PIXIT l_x, c_x, r_x, t_y, c_y, b_y;
    PIXIT_POINT ear_size;

    p_pixit_rect_ears->ear[PIXIT_RECT_EAR_CENTRE] = *p_pixit_rect;

    l_x = p_pixit_rect->tl.x - 2* 2*p_one_program_pixel->x;
    r_x = p_pixit_rect->br.x - 3* 2*p_one_program_pixel->x;
    c_x = (l_x + r_x) / 2;
    t_y = p_pixit_rect->tl.y - 2* 1*p_one_program_pixel->y;
    b_y = p_pixit_rect->br.y - 3* 1*p_one_program_pixel->y;
    c_y = (t_y + b_y) / 2;

    ear_size.x = 5* 2*p_one_program_pixel->x;
    ear_size.y = 5* 1*p_one_program_pixel->y;

    /* left */
    p_pixit_rect_ears->ear[PIXIT_RECT_EAR_TL].tl.x = l_x;
    p_pixit_rect_ears->ear[PIXIT_RECT_EAR_TL].br.x = l_x + ear_size.x;
    p_pixit_rect_ears->ear[PIXIT_RECT_EAR_LC].tl.x = l_x;
    p_pixit_rect_ears->ear[PIXIT_RECT_EAR_LC].br.x = l_x + ear_size.x;
    p_pixit_rect_ears->ear[PIXIT_RECT_EAR_BL].tl.x = l_x;
    p_pixit_rect_ears->ear[PIXIT_RECT_EAR_BL].br.x = l_x + ear_size.x;

    /* right */
    p_pixit_rect_ears->ear[PIXIT_RECT_EAR_TR].tl.x = r_x;
    p_pixit_rect_ears->ear[PIXIT_RECT_EAR_TR].br.x = r_x + ear_size.x;
    p_pixit_rect_ears->ear[PIXIT_RECT_EAR_RC].tl.x = r_x;
    p_pixit_rect_ears->ear[PIXIT_RECT_EAR_RC].br.x = r_x + ear_size.x;
    p_pixit_rect_ears->ear[PIXIT_RECT_EAR_BR].tl.x = r_x;
    p_pixit_rect_ears->ear[PIXIT_RECT_EAR_BR].br.x = r_x + ear_size.x;

    /* top */
    p_pixit_rect_ears->ear[PIXIT_RECT_EAR_TL].tl.y = t_y;
    p_pixit_rect_ears->ear[PIXIT_RECT_EAR_TL].br.y = t_y + ear_size.y;
    p_pixit_rect_ears->ear[PIXIT_RECT_EAR_TC].tl.y = t_y;
    p_pixit_rect_ears->ear[PIXIT_RECT_EAR_TC].br.y = t_y + ear_size.y;
    p_pixit_rect_ears->ear[PIXIT_RECT_EAR_TR].tl.y = t_y;
    p_pixit_rect_ears->ear[PIXIT_RECT_EAR_TR].br.y = t_y + ear_size.y;

    /* bottom */
    p_pixit_rect_ears->ear[PIXIT_RECT_EAR_BL].tl.y = b_y;
    p_pixit_rect_ears->ear[PIXIT_RECT_EAR_BL].br.y = b_y + ear_size.y;
    p_pixit_rect_ears->ear[PIXIT_RECT_EAR_BC].tl.y = b_y;
    p_pixit_rect_ears->ear[PIXIT_RECT_EAR_BC].br.y = b_y + ear_size.y;
    p_pixit_rect_ears->ear[PIXIT_RECT_EAR_BR].tl.y = b_y;
    p_pixit_rect_ears->ear[PIXIT_RECT_EAR_BR].br.y = b_y + ear_size.y;

    /* centres */
    p_pixit_rect_ears->ear[PIXIT_RECT_EAR_TC].tl.x = c_x;
    p_pixit_rect_ears->ear[PIXIT_RECT_EAR_TC].br.x = c_x + ear_size.x;
    p_pixit_rect_ears->ear[PIXIT_RECT_EAR_BC].tl.x = c_x;
    p_pixit_rect_ears->ear[PIXIT_RECT_EAR_BC].br.x = c_x + ear_size.x;

    p_pixit_rect_ears->ear[PIXIT_RECT_EAR_LC].tl.y = c_y;
    p_pixit_rect_ears->ear[PIXIT_RECT_EAR_LC].br.y = c_y + ear_size.y;
    p_pixit_rect_ears->ear[PIXIT_RECT_EAR_RC].tl.y = c_y;
    p_pixit_rect_ears->ear[PIXIT_RECT_EAR_RC].br.y = c_y + ear_size.y;

    p_pixit_rect_ears->ear_active[PIXIT_RECT_EAR_CENTRE] = TRUE;

    p_pixit_rect_ears->ear_active[PIXIT_RECT_EAR_TL] = TRUE;
    p_pixit_rect_ears->ear_active[PIXIT_RECT_EAR_BR] = TRUE;
    p_pixit_rect_ears->ear_active[PIXIT_RECT_EAR_TR] = TRUE;
    p_pixit_rect_ears->ear_active[PIXIT_RECT_EAR_BL] = TRUE;

    p_pixit_rect_ears->ear_active[PIXIT_RECT_EAR_TC] = (p_pixit_rect_ears->ear[PIXIT_RECT_EAR_TC].br.x <= r_x);
    p_pixit_rect_ears->ear_active[PIXIT_RECT_EAR_BC] = (p_pixit_rect_ears->ear[PIXIT_RECT_EAR_TC].br.x <= r_x);

    p_pixit_rect_ears->ear_active[PIXIT_RECT_EAR_LC] = (p_pixit_rect_ears->ear[PIXIT_RECT_EAR_LC].br.y <= b_y);
    p_pixit_rect_ears->ear_active[PIXIT_RECT_EAR_RC] = (p_pixit_rect_ears->ear[PIXIT_RECT_EAR_RC].br.y <= b_y);

    p_pixit_rect_ears->outer_bound.tl = p_pixit_rect_ears->ear[PIXIT_RECT_EAR_TL].tl;
    p_pixit_rect_ears->outer_bound.br = p_pixit_rect_ears->ear[PIXIT_RECT_EAR_BR].br;
}

/* end of ui_misc.c */
