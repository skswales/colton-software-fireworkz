/* ob_spelb.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Spell (bound) object module */

/* SKS Sep 1993 */

#include "common/gflags.h"

#include "ob_spelb/ob_spelb.h"

#ifndef         __xp_skeld_h
#include "ob_skel/xp_skeld.h"
#endif

#if RISCOS
#define MSG_WEAK &rb_spelb_msg_weak
extern PC_U8 rb_spelb_msg_weak;
#endif
#define P_BOUND_RESOURCES_OBJECT_ID_SPELB NULL

/*
construct argument types
*/

static const ARG_TYPE
spelb_args_bool[] =
{
    ARG_TYPE_BOOL | ARG_MANDATORY,
    ARG_TYPE_NONE
};

/*
construct table
*/

static CONSTRUCT_TABLE
object_construct_table[] =
{                                                                                                   /*   fi ti mi ur up xi md mf nn cp sm ba fo */

    { "ChoicesAutoCheck",       spelb_args_bool,            T5_CMD_CHOICES_SPELL_AUTO_CHECK  },
    { "ChoicesWriteUser",       spelb_args_bool,            T5_CMD_CHOICES_SPELL_WRITE_USER  },

    

    { NULL,                     NULL,                       T5_EVENT_NONE } /* end of table */
};

enum CHOICES_SPELB_CONTROL_IDS
{
    CHOICES_SPELB_ID_GROUP = (OBJECT_ID_SPELB * 1000) + 164,
    CHOICES_SPELB_ID_AUTO_CHECK,
    /*CHOICES_SPELB_ID_DICT_GROUP,*/
    /*CHOICES_SPELB_ID_LOAD_MASTER,*/
    /*CHOICES_SPELB_ID_LOAD_USER,*/
    CHOICES_SPELB_ID_WRITE_USER
};

static /*poked*/ DIALOG_CONTROL
choices_spell_group =
{
    CHOICES_SPELB_ID_GROUP, DIALOG_MAIN_GROUP,
    { 0, 0, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { DIALOG_GROUPSPACING_H, 0, DIALOG_STDGROUP_RM, DIALOG_STDGROUP_BM },
    { DRT(RTRB, GROUPBOX) }
};

static const DIALOG_CONTROL_DATA_GROUPBOX
choices_spell_group_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_CHOICES_SPELB_GROUP), { 0, 0, 0, FRAMED_BOX_GROUP } };

static const DIALOG_CONTROL
choices_spell_auto_check =
{
    CHOICES_SPELB_ID_AUTO_CHECK, CHOICES_SPELB_ID_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { DIALOG_STDGROUP_LM, DIALOG_STDGROUP_TM, DIALOG_CONTENTS_CALC, DIALOG_STDCHECK_V },
    { DRT(LTLT, CHECKBOX), 1 }
};

static /*poked*/ DIALOG_CONTROL_DATA_CHECKBOX
choices_spell_auto_check_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_CHOICES_SPELB_AUTO_CHECK) };

#if 0

static const DIALOG_CONTROL
choices_spell_dict_group =
{
    CHOICES_SPELB_ID_DICT_GROUP, CHOICES_SPELB_ID_GROUP,

    { CHOICES_SPELB_ID_AUTO_CHECK, CHOICES_SPELB_ID_AUTO_CHECK, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },

    { 0, DIALOG_STDSPACING_V, DIALOG_STDGROUP_RM, DIALOG_STDGROUP_BM },

    { DRT(LBRB, GROUPBOX) }
};

static const DIALOG_CONTROL_DATA_GROUPBOX
choices_spell_dict_group_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_CHOICES_SPELB_DICT_GROUP), { 0, 0, 0, FRAMED_BOX_GROUP } };

static const DIALOG_CONTROL
choices_spell_load_master =
{
    CHOICES_SPELB_ID_LOAD_MASTER, CHOICES_SPELB_ID_DICT_GROUP,

    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },

    { DIALOG_STDGROUP_LM, DIALOG_STDGROUP_TM, DIALOG_CONTENTS_CALC, DIALOG_STDCHECK_V },

    { DRT(LTLT, CHECKBOX), 1 }
};

static /*poked*/ DIALOG_CONTROL_DATA_CHECKBOX
choices_spell_load_master_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_CHOICES_SPELB_LOAD_MASTER } };

static const DIALOG_CONTROL
choices_spell_load_user =
{
    CHOICES_SPELB_ID_LOAD_USER, CHOICES_SPELB_ID_DICT_GROUP,

    { CHOICES_SPELB_ID_LOAD_MASTER, CHOICES_SPELB_ID_LOAD_MASTER, CHOICES_SPELB_ID_LOAD_MASTER },

    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDCHECK_V },

    { DRT(LBRT, CHECKBOX), 1 }
};

static /*poked*/ DIALOG_CONTROL_DATA_CHECKBOX
choices_spell_load_user_data = { { 0 },  UI_TEXT_INIT_RESID(MSG_DIALOG_CHOICES_SPELB_LOAD_USER } };

#endif

static const DIALOG_CONTROL
choices_spell_write_user =
{
    CHOICES_SPELB_ID_WRITE_USER, CHOICES_SPELB_ID_GROUP,
    { CHOICES_SPELB_ID_AUTO_CHECK, CHOICES_SPELB_ID_AUTO_CHECK },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDCHECK_V },
    { DRT(LBLT, CHECKBOX), 1 }
};

static /*poked*/ DIALOG_CONTROL_DATA_CHECKBOX
choices_spell_write_user_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_CHOICES_SPELB_WRITE_USER) };

static const DIALOG_CTL_CREATE
choices_spell_ctl_create[] =
{
    { &choices_spell_group,       &choices_spell_group_data },
    { &choices_spell_auto_check,  &choices_spell_auto_check_data },
#if 0
    { &choices_spell_dict_group,  &choices_spell_dict_group_data },
    { &choices_spell_load_master, &choices_spell_load_master_data },
    { &choices_spell_load_user,   &choices_spell_load_user_data },
#endif
    { &choices_spell_write_user,  &choices_spell_write_user_data }
};

T5_MSG_PROTO(static, spelb_choices_query, _InoutRef_ P_CHOICES_QUERY_BLOCK p_choices_query_block)
{
    IGNOREPARM_DocuRef_(p_docu);
    IGNOREPARM_InVal_(t5_message);

    choices_spell_auto_check_data.init_state  = (U8) global_preferences.spell_auto_check;
#if 0
    choices_spell_load_master_data.init_state = (U8) global_preferences.spell_load_master;
    choices_spell_load_user_data.init_state   = (U8) global_preferences.spell_load_user;
#endif
    choices_spell_write_user_data.init_state  = (U8) global_preferences.spell_write_user;

    choices_spell_group.relative_dialog_control_id[0] = p_choices_query_block->tr_dialog_control_id;
    choices_spell_group.relative_dialog_control_id[1] = p_choices_query_block->tr_dialog_control_id;

    p_choices_query_block->tr_dialog_control_id =
    p_choices_query_block->br_dialog_control_id = CHOICES_SPELB_ID_GROUP;

    return(al_array_add(&p_choices_query_block->ctl_create, DIALOG_CTL_CREATE, elemof32(choices_spell_ctl_create), PC_ARRAY_INIT_BLOCK_NONE, choices_spell_ctl_create));
}

T5_MSG_PROTO(static, spelb_choices_set, P_CHOICES_SET_BLOCK p_choices_set_block)
{
    STATUS status;
    ARGLIST_HANDLE arglist_handle;

    IGNOREPARM_InVal_(t5_message);

    if(status_ok(status = arglist_prepare(&arglist_handle, spelb_args_bool)))
    {
        const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 1);

        p_args[0].val.fBool = ui_dlg_get_check(p_choices_set_block->h_dialog, CHOICES_SPELB_ID_AUTO_CHECK);
        status_consume(execute_command_reperr(p_docu, T5_CMD_CHOICES_SPELL_AUTO_CHECK, &arglist_handle, OBJECT_ID_SPELB));

        p_args[0].val.fBool = ui_dlg_get_check(p_choices_set_block->h_dialog, CHOICES_SPELB_ID_WRITE_USER);
        status_consume(execute_command_reperr(p_docu, T5_CMD_CHOICES_SPELL_WRITE_USER, &arglist_handle, OBJECT_ID_SPELB));

        arglist_dispose(&arglist_handle);
    }

    return(status);
}

T5_MSG_PROTO(static, spelb_choices_save, _InoutRef_ P_OF_OP_FORMAT p_of_op_format)
{
    STATUS status = STATUS_OK;
    const OBJECT_ID object_id = OBJECT_ID_SPELB;
    PC_CONSTRUCT_TABLE p_construct_table;
    ARGLIST_HANDLE arglist_handle;

    IGNOREPARM_DocuRef_(p_docu);
    IGNOREPARM_InVal_(t5_message);

    if(status_ok(status = arglist_prepare_with_construct(&arglist_handle, object_id, T5_CMD_CHOICES_SPELL_AUTO_CHECK, &p_construct_table)))
    {
        const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 1);
        p_args[0].val.fBool = global_preferences.spell_auto_check;
        status = ownform_save_arglist(arglist_handle, object_id, p_construct_table, p_of_op_format);
        arglist_dispose(&arglist_handle);
    }

    status_return(status);

    if(status_ok(status = arglist_prepare_with_construct(&arglist_handle, object_id, T5_CMD_CHOICES_SPELL_WRITE_USER, &p_construct_table)))
    {
        const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 1);
        p_args[0].val.fBool = global_preferences.spell_write_user;
        status = ownform_save_arglist(arglist_handle, object_id, p_construct_table, p_of_op_format);
        arglist_dispose(&arglist_handle);
    }

    return(status);
}

T5_CMD_PROTO(static, spelb_choices_spell_auto_check)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 1);

    IGNOREPARM_DocuRef_(p_docu);

    if(global_preferences.spell_auto_check != p_args[0].val.fBool)
    {
        global_preferences.spell_auto_check = p_args[0].val.fBool;
        issue_choice_changed(t5_message);
    }

    return(STATUS_OK);
}

T5_CMD_PROTO(static, spelb_choices_spell_write_user)
{
    STATUS status = STATUS_OK;
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 1);

    IGNOREPARM_DocuRef_(p_docu);

    if(global_preferences.spell_write_user != p_args[0].val.fBool)
    {
        global_preferences.spell_write_user = p_args[0].val.fBool;
        issue_choice_changed(t5_message);
    }

    /* default is on, so only tell spell module if it's loaded, otherwise it's irrelevant */
    /*if(object_present(OBJECT_ID_SPELL))*/
        /*status = object_call_id(OBJECT_ID_SPELL, p_docu, T5_CMD_CHOICES_SPELL_WRITE_USER, de_const_cast(P_ANY, p_t5_cmd));*/

    return(status);
}

T5_MSG_PROTO(static, spelb_msg_initclose, _InRef_ PC_MSG_INITCLOSE p_msg_initclose)
{
    IGNOREPARM_DocuRef_(p_docu);
    IGNOREPARM_InVal_(t5_message);

    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__STARTUP:
        status_return(resource_init(OBJECT_ID_SPELB, MSG_WEAK, P_BOUND_RESOURCES_OBJECT_ID_SPELB));

        return(register_object_construct_table(OBJECT_ID_SPELB, object_construct_table, FALSE /* no inlines */));

    case T5_MSG_IC__STARTUP_CONFIG:
        return(load_object_config_file(OBJECT_ID_SPELB));

    case T5_MSG_IC__EXIT1:
        return(resource_close(OBJECT_ID_SPELB));

    default:
        return(STATUS_OK);
    }
}

OBJECT_PROTO(extern, object_spelb);
OBJECT_PROTO(extern, object_spelb)
{
    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        return(spelb_msg_initclose(p_docu, t5_message, (PC_MSG_INITCLOSE) p_data));

    case T5_MSG_CHOICES_QUERY:
        return(spelb_choices_query(p_docu, t5_message, (P_CHOICES_QUERY_BLOCK) p_data));

    case T5_MSG_CHOICES_SET:
        return(spelb_choices_set(p_docu, t5_message, (P_CHOICES_SET_BLOCK) p_data));

    case T5_MSG_CHOICES_SAVE:
        return(spelb_choices_save(p_docu, t5_message, (P_OF_OP_FORMAT) p_data));

    case T5_CMD_CHOICES_SPELL_AUTO_CHECK:
        return(spelb_choices_spell_auto_check(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_CHOICES_SPELL_WRITE_USER:
        return(spelb_choices_spell_write_user(p_docu, t5_message, (PC_T5_CMD) p_data));

    default:
        return(STATUS_OK);
    }
}

/* end of ob_spelb.c */
