/* sk_menu.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Menus for Fireworkz */

/* RCM May 1992 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#if RISCOS
#include "ob_skel/xp_skelr.h"
#endif

#define MENU_SEP_CH          CH_FULL_STOP
#define MENU_SEP_TSTR   TEXT(".")

static
struct MENU_ROOT_TABLE_ENTRY
{
    PCTSTR name;
    U32 name_length;
#define MRTE_INIT(name) \
    { name, (elemof32(name)-1) }
    MENU_ROOT menu_root;
}
menu_root_table[] =
{
    MRTE_INIT(TEXT("$")),
    MRTE_INIT(TEXT("IconBar")),
    MRTE_INIT(TEXT("AutoFunction")),
    MRTE_INIT(TEXT("Function")),
    MRTE_INIT(TEXT("Chart")),
    MRTE_INIT(TEXT("ChartEdit"))
};

static void
recursive_menu_dispose(
    _DocuRef_maybenone_ P_DOCU p_docu,
    P_MENU_ROOT p_menu_root)
{
    ARRAY_INDEX i;

    for(i = 0; i < array_elements(&p_menu_root->h_entry_list); ++i)
    {
        P_MENU_ENTRY p_menu_entry = array_ptr(&p_menu_root->h_entry_list, MENU_ENTRY, i);

        if(p_menu_entry->key_code && DOCU_NOT_NONE(p_docu))
            command_array_handle_dispose_from_key_code(p_docu, p_menu_entry->key_code);     /* undefine key binding */

        /*tstr_clr(&p_menu_entry->tstr_entry_text); alloc_block*/
        al_array_dispose(&p_menu_entry->h_command);                 /* kill command macro string */
        al_array_dispose(&p_menu_entry->h_command2);                /* kill command macro string */

        recursive_menu_dispose(p_docu, &p_menu_entry->sub_menu);    /* kill any sub_menu */
    }

    al_array_dispose(&p_menu_root->h_entry_list);
}

#if defined(MENU_SAVE_ENABLED)

_Check_return_
static STATUS
recursive_menu_save(
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format,
    _InRef_     PC_CONSTRUCT_TABLE p_construct_table,
    _InVal_     ARGLIST_HANDLE arglist_handle,
    P_MENU_ROOT p_menu_root,
    _In_z_      PCTSTR root_name)
{
    const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 6);
    ARRAY_INDEX i;
    S32 root_name_end = tstrlen32(root_name);
    STATUS status = STATUS_OK;

    /* for each entry in this menu - save text and command, and its submenu (if any) */

    for(i = 0; i < array_elements(&p_menu_root->h_entry_list); ++i)
    {
        P_MENU_ENTRY p_menu_entry = array_ptr(&p_menu_root->h_entry_list, MENU_ENTRY, i);

        p_args[0].val.tstr = root_name;
        p_args[1].val.tstr = p_menu_entry->tstr_entry_text;
        p_args[2].val.tstr = key_of_name_from_key_code(p_menu_entry->key_code);

        p_args[3].val.raw    = p_menu_entry->h_command;  /* loan handle temporarily for output */
        if(!p_menu_entry->h_command)
            p_args[3].type   = ARG_TYPE_NONE;

        p_args[4].val.raw    = p_menu_entry->h_command2; /* loan handle temporarily for output */
        if(!p_menu_entry->h_command2)
            p_args[4].type   = ARG_TYPE_NONE;

        p_args[5].val.u8n    = p_menu_entry->flags.active_state;
        if(p_menu_entry->flags.active_state == MENU_ACTIVE_ALWAYS)
            p_args[5].type   = ARG_TYPE_NONE;

        status = ownform_save_arglist(arglist_handle, OBJECT_ID_SKEL, p_construct_table, p_of_op_format);

        p_args[3].type       = ARG_TYPE_RAW; /* reset type */
        p_args[3].val.raw    = 0; /* reclaim handle */

        p_args[4].type       = ARG_TYPE_RAW; /* reset type */
        p_args[4].val.raw    = 0; /* reclaim handle */

        p_args[5].type       = ARG_TYPE_S32; /* reset type */

        status_break(status);

        (void) tstrcat(tstrcat(root_name, p_menu_entry->tstr_entry_text), MENU_SEP_TSTR); /* temporarily append separator and <entry name> */

        status = recursive_menu_save(p_of_op_format, p_construct_table, arglist_handle, &p_menu_entry->sub_menu, root_name);

        root_name[root_name_end] = CH_NULL; /* undo damage ready for next entry/return */

        status_break(status);
    }

    return(status);
}

#endif /* MENU_SAVE_ENABLED */

static BOOL
search_for_entry(
    P_MENU_ROOT p_menu_root,
    _In_z_      PCTSTR tstr_search_name,
    /*out*/ P_P_MENU_ENTRY p_p_menu_entry)
{
    const ARRAY_INDEX n_elements = array_elements(&p_menu_root->h_entry_list);
    ARRAY_INDEX i;
    P_MENU_ENTRY p_menu_entry = array_range(&p_menu_root->h_entry_list, MENU_ENTRY, 0, n_elements);

    PTR_ASSERT(p_p_menu_entry);

    for(i = 0; i < n_elements; ++i, ++p_menu_entry)
    {
        PCTSTR tstr_entry_text = p_menu_entry->tstr_entry_text;

        if(NULL == tstr_entry_text)
            continue;

        if(0 == tstrcmp(tstr_entry_text, tstr_search_name))
        {
            *p_p_menu_entry = p_menu_entry;
            return(TRUE);
        }
    }

    *p_p_menu_entry = NULL;
    return(FALSE);
}

_Check_return_
static BOOL
menu_name_compare(
    _In_z_      PCTSTR tstr_a,
    _In_reads_(n_chars) PCTCH tchars_b,
    _InVal_     U32 n_chars)
{
    BOOL found = FALSE;
    U32 i = 0;

    for(;;) /* bit like tstrncmp() but catering for & escaping */
    {
        TCHAR a, b;

        a = *tstr_a++;
        if(CH_AMPERSAND == a)
            a = *tstr_a++;

        b = *tchars_b++;
        if(i++ >= n_chars)
        {
            found = (CH_NULL == a);
            break;
        }
        if(CH_AMPERSAND == b)
        {
            b = *tchars_b++;
            if(i++ >= n_chars)
            {
                found = (CH_NULL == a);
                break;
            }
        }

        if(a != b)
            break;
    }

    return(found);
}

_Check_return_
static BOOL
search_for_root(
    _DocuRef_   P_DOCU p_docu,
    _In_z_      PCTSTR tstr_menu_name,
    _OutRef_    P_P_MENU_ROOT p_p_menu_found)
{
    P_MENU_ROOT p_menu_root = P_MENU_ROOT_NONE;

    *p_p_menu_found = P_MENU_ROOT_NONE;

    if(NULL == tstr_menu_name)
        return(FALSE);

    {
    const U32 menu_name_length = tstrlen32(tstr_menu_name);
    MENU_ROOT_ID menu_root_id;

    for(menu_root_id = MENU_ROOT_DOCU; menu_root_id < elemof32(menu_root_table); ENUM_INCR(MENU_ROOT_ID, menu_root_id))
    {
        const U32 mrte_name_length = menu_root_table[menu_root_id].name_length;

        if(mrte_name_length > menu_name_length)
            continue; /* this entry is too long to possibly match */

        if(0 != memcmp(tstr_menu_name, menu_root_table[menu_root_id].name, mrte_name_length * sizeof32(TCHAR)))
            continue; /* failed to match at all */

        if( (tstr_menu_name[mrte_name_length] == CH_NULL) ||
            (tstr_menu_name[mrte_name_length] == MENU_SEP_CH) )
        {
            /* good match (NULL -> entire match; MENU_SEP_CH -> matched as prefix) */
            p_menu_root = sk_menu_root(p_docu, menu_root_id);

            /* skip what we matched, leaving pointing to either terminator */
            tstr_menu_name += mrte_name_length;
            break;
        }
    }
    } /*block */

    if(P_MENU_ROOT_NONE == p_menu_root)
    {
        assert0();
        return(0);
    }

    if(MENU_SEP_CH == *tstr_menu_name)
    {
        while(MENU_SEP_CH == *tstr_menu_name++)
        {
            PCTSTR tstr_menu_name_start = tstr_menu_name;
            U32 menu_name_length;
            ARRAY_INDEX element;
            BOOL found;

            while( (MENU_SEP_CH != *tstr_menu_name) && (CH_NULL != *tstr_menu_name) )
                tstr_menu_name++;

            if(tstr_menu_name_start == tstr_menu_name)
                return(FALSE);                  /* zero length name */

            menu_name_length = PtrDiffElemU32(tstr_menu_name, tstr_menu_name_start);

            /* search menu for entry */
            for(element = 0, found = FALSE; element < array_elements(&p_menu_root->h_entry_list); element++)
            {
                const P_MENU_ENTRY p_menu_entry = array_ptr(&p_menu_root->h_entry_list, MENU_ENTRY, element);
                const PCTSTR tstr_menu_entry_name = p_menu_entry->tstr_entry_text;

                if(NULL == tstr_menu_entry_name)
                    continue;

                found = menu_name_compare(tstr_menu_entry_name, tstr_menu_name_start, menu_name_length);

                if(found)
                {
                    p_menu_root = &p_menu_entry->sub_menu;
                    break;
                }
            }

            if(!found)
                return(FALSE);
        }
    }

    *p_p_menu_found = p_menu_root;

    return(TRUE);
}

/*
main events
*/

#if defined(MENU_SAVE_ENABLED)

_Check_return_
static STATUS
sk_menu_msg_pre_save_2(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format)
{
    STATUS status = STATUS_OK;

    if(p_of_op_format->of_template.menu_struct && p_docu->p_menu_root)
    {
        OBJECT_ID         object_id = OBJECT_ID_SKEL;
        PC_CONSTRUCT_TABLE p_construct_table;
        ARGLIST_HANDLE    arglist_handle;
        U8Z               root_name[1024]; /*>>> make this a handle based character array, so it can grow */
        MENU_ROOT_ID menu_root_id;

        status_return(arglist_prepare_with_construct(&arglist_handle, object_id, T5_CMD_MENU_ADD, &p_construct_table));

        for(menu_root_id = 1; menu_root_id < elemof32(menu_root_table); ++menu_root_id)
        {
            (void) strcpy(root_name, menu_root_table[menu_root_id].name);
            status_break(status = recursive_menu_save(p_of_op_format, p_construct_table, arglist_handle, sk_menu_root(p_docu, menu_root_id), root_name));
        }

        if(status_ok(status))
        {
            menu_root_id = MENU_ROOT_DOCU;
            (void) strcpy(root_name, menu_root_table[menu_root_id].name);
            status = recursive_menu_save(p_of_op_format, p_construct_table, arglist_handle, p_docu->p_menu_root, root_name);
        }

        arglist_dispose(&arglist_handle);
    }

    return(status);
}

#endif /* MENU_SAVE_ENABLED */

#if defined(MENU_SAVE_ENABLED)

_Check_return_
static STATUS
sk_menu_msg_save(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     P_MSG_SAVE p_msg_save)
{
    switch(p_msg_save->t5_message)
    {
    case T5_MSG_PRE_SAVE_2:
        status_return(sk_menu_msg_pre_save_2(p_docu, p_msg_save));

#if 0
        /* pass this sideways */
        return(sk_cmd_msg_pre_save_2(p_docu, p_msg_save));
#else
        return(STATUS_OK);
#endif

    default:
        return(STATUS_OK);
    }
}

MAEVE_EVENT_PROTO(static, maeve_event_sk_menu)
{
    switch(t5_message)
    {
    case T5_MSG_SAVE:
        return(sk_menu_msg_save(p_docu, DATA_FROM_MB(MSG_SAVE, p_maeve_block));

    default:
        return(STATUS_OK);
    }
}

#endif /* MENU_SAVE_ENABLED */

/*
exported services hook
*/

_Check_return_
static STATUS
sk_menu_msg_init1(
    _DocuRef_   P_DOCU p_docu)
{
#if defined(MENU_SAVE_ENABLED)
    return(maeve_event_handler_add(p_docu, maeve_event_sk_menu, (CLIENT_HANDLE) 0));
#else
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    return(STATUS_OK);
#endif /* MENU_SAVE_ENABLED */
}

_Check_return_
static STATUS
sk_menu_msg_close1(
    _DocuRef_   P_DOCU p_docu)
{
#if defined(MENU_SAVE_ENABLED)
    maeve_event_handler_del(p_docu, maeve_event_sk_menu, (CLIENT_HANDLE) 0);
#endif /* MENU_SAVE_ENABLED */

    if(NULL != p_docu->p_menu_root)
    {
        recursive_menu_dispose(p_docu, p_docu->p_menu_root);

        al_ptr_dispose(P_P_ANY_PEDANTIC(&p_docu->p_menu_root));
    }

    if(DOCNO_CONFIG == docno_from_p_docu(p_docu))
    {
        MENU_ROOT_ID menu_root_id;

        for(menu_root_id = MENU_ROOT_ICON_BAR; menu_root_id < elemof32(menu_root_table); ENUM_INCR(MENU_ROOT_ID, menu_root_id))
            recursive_menu_dispose(p_docu, sk_menu_root(p_docu, menu_root_id));

        ho_menu_dispose();
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
maeve_services_sk_menu_msg_initclose(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_MSG_INITCLOSE p_msg_initclose)
{
    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__INIT1:
        status_return(sk_menu_msg_init1(p_docu));
        break;

    case T5_MSG_IC__CLOSE1:
        status_return(sk_menu_msg_close1(p_docu));
        break;

    default:
        break;
    }

    /* pass these sideways */
    return(sk_cmd_direct_msg_initclose(p_docu, p_msg_initclose));
}

MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_sk_menu);
MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_sk_menu)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InRef_(p_maeve_services_block);

    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        status_return(maeve_services_sk_menu_msg_initclose(p_docu, (PC_MSG_INITCLOSE) p_data));

    default:
        return(STATUS_OK);
    }
}

_Check_return_
_Ret_notnull_
extern P_MENU_ROOT
sk_menu_root(
    _DocuRef_   PC_DOCU p_docu,
    _InVal_     MENU_ROOT_ID menu_root_id)
{
    P_MENU_ROOT p_menu_root;

    switch(menu_root_id)
    {
    case MENU_ROOT_DOCU:
        assert(p_docu == p_docu_from_config()); /* hopefully... */
        p_menu_root = p_docu->p_menu_root;
        break;

    default:
        assert((U32) menu_root_id < (U32) elemof32(menu_root_table));
        p_menu_root = &menu_root_table[menu_root_id].menu_root;
        break;
    }

    return(p_menu_root);
}

_Check_return_
extern STATUS
t5_cmd_activate_menu(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message)
{
    const P_VIEW p_view = p_view_from_viewno_caret(p_docu);
    MENU_ROOT_ID menu_root_id = MENU_ROOT_DOCU;

    switch(T5_MESSAGE_CMD_OFFSET(t5_message))
    {
    default: default_unhandled();
        break;

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_ACTIVATE_MENU_AUTO_FUNCTION):
        menu_root_id = MENU_ROOT_AUTO_FUNCTION;
        break;

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_ACTIVATE_MENU_FUNCTION_SELECTOR):
        menu_root_id = MENU_ROOT_FUNCTION_SELECTOR;
        break;

    case T5_MESSAGE_CMD_OFFSET(T5_CMD_ACTIVATE_MENU_CHART):
        menu_root_id = MENU_ROOT_CHART;
        break;
    }

    return(ho_menu_popup(p_docu, p_view, menu_root_id));
}

#define ARG_MENU_ADD_NAME     0
#define ARG_MENU_ADD_ENTRY    1
#define ARG_MENU_ADD_RHS      2
#define ARG_MENU_ADD_COMMAND  3
#define ARG_MENU_ADD_COMMAND2 4
#define ARG_MENU_ACTIVE_STATE 5

T5_CMD_PROTO(extern, t5_cmd_menu_add)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 6);
    PCTSTR tstr_menu_name = p_args[ARG_MENU_ADD_NAME].val.tstr;
    BOOL raw_args = (t5_message == T5_CMD_MENU_ADD_RAW);
    PCTSTR tstr_entry_text;
    PCTSTR p_entry_key;
    ARRAY_HANDLE h_entry_command  = 0;
    ARRAY_HANDLE h_entry_command2 = 0;
    KMAP_CODE key_code = 0;
    STATUS status;
    PTSTR tstr_entry_text_copy = NULL;
    P_MENU_ROOT p_menu_root;
    MAEVE_HANDLE maeve_handle = 0;
    U8 active_state = MENU_ACTIVE_ALWAYS;

    /* Check the command arguments:                                */
    /*   0 - the name of the menu to be added to      (must exist) */
    /*   1 - the entry text                           (optional) */
    /*   2 - keyboard shortcut                        (optional)   */
    /*   3 - the command to execute on selection      (optional)   */
    /*   4 - the command to execute on slide to right (optional)   */
    /*   5 - entry active/greyed out state            (optional)   */
    /*                                                             */
    /* The entry text and additional entry text are copied to      */
    /* the global string table. The entry command is duplicated.   */
#if defined(VALIDATE_MAIN_ALLOCS) && defined(VALIDATE_MENU_ADD)
    alloc_validate((void *) 1, "menu_add");
#endif

    if(arg_is_present(p_args, ARG_MENU_ADD_ENTRY))
        tstr_entry_text = p_args[ARG_MENU_ADD_ENTRY].val.tstr;
    else
        tstr_entry_text = NULL;

    if(arg_is_present(p_args, ARG_MENU_ADD_RHS))
        p_entry_key = p_args[ARG_MENU_ADD_RHS].val.tstr;
    else
        p_entry_key = NULL;

    if(NULL != tstr_entry_text)
        status_return(alloc_block_tstr_set(&tstr_entry_text_copy, tstr_entry_text, &p_docu->general_string_alloc_block));

    if(arg_is_present(p_args, ARG_MENU_ADD_COMMAND))
    {
        if(raw_args)
            status = al_array_duplicate(&h_entry_command, &p_args[ARG_MENU_ADD_COMMAND].val.raw);
        else
            status = al_ustr_set(&h_entry_command, p_args[ARG_MENU_ADD_COMMAND].val.ustr);

        if(status_fail(status))
        {
            /*tstr_clr(&entry_text);*/
            return(status);
        }
    }

    if(arg_is_present(p_args, ARG_MENU_ADD_COMMAND2))
    {
        if(raw_args)
            status = al_array_duplicate(&h_entry_command2, &p_args[ARG_MENU_ADD_COMMAND2].val.raw);
        else
            status = al_ustr_set(&h_entry_command2, p_args[ARG_MENU_ADD_COMMAND2].val.ustr);

        if(status_fail(status))
        {
            /*tstr_clr(&entry_text);*/
            al_array_dispose(&h_entry_command);
            return(status);
        }
    }

    if(arg_is_present(p_args, ARG_MENU_ACTIVE_STATE))
        active_state = p_args[ARG_MENU_ACTIVE_STATE].val.u8n;

    /* if the following fails, free the duplicated command macro strings */
    for(;;)
    {
        P_MENU_ENTRY p_menu_entry;
        BOOL found;

        if(!p_docu->p_menu_root)
        {
#if defined(MENU_SAVE_ENABLED)
            /* SKS: we must endeavour to free menus when appropriate! */
            status_break(status = maeve_event_handler_add(p_docu, maeve_event_sk_menu, (CLIENT_HANDLE) 0));

            maeve_handle = (MAEVE_HANDLE) status;
#endif

            if(NULL == (p_docu->p_menu_root = al_ptr_calloc_elem(MENU_ROOT, 1, &status)))
                break;
        }

        if(!search_for_root(p_docu, tstr_menu_name, &p_menu_root))
            break;

        /* the 'add to' menu looks kosher, so check it to see if entry already exists */
        p_menu_entry = NULL;

        if(NULL != tstr_entry_text)
            found = search_for_entry(p_menu_root, tstr_entry_text, &p_menu_entry);
        else
            found = FALSE;

        if(found)
        {
            /* destroy old entry, but retain position in menu structure */
            if(p_menu_entry->key_code)
                command_array_handle_dispose_from_key_code(p_docu, p_menu_entry->key_code);       /* kill its key defn. */

            /*tstr_clr(&p_menu_entry->tstr_entry_text); alloc_block*/
            al_array_dispose(&p_menu_entry->h_command);                                         /* kill old command macro string */
            al_array_dispose(&p_menu_entry->h_command2);                                        /* kill old command macro string */
        }
        else
        {
            SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(MENU_ENTRY), TRUE);

            if(NULL == (p_menu_entry = al_array_extend_by(&p_menu_root->h_entry_list, MENU_ENTRY, 1, &array_init_block, &status)))
            {
                al_array_dispose(&h_entry_command);     /* free duplicated command macro string */
                al_array_dispose(&h_entry_command2);    /* free duplicated command macro string */
                break;
            }
        }

        /* define key binding here AFTER any undefines caused when (found == TRUE) */
        if(p_entry_key && (h_entry_command || h_entry_command2))
        {
            ARRAY_HANDLE h_command = h_entry_command2 ? h_entry_command2 : h_entry_command;
            if(0 != (key_code = key_code_from_key_of_name(p_entry_key)))
                if(status_fail(command_array_handle_assign_to_key_code(p_docu, key_code, h_command)))
                    key_code = 0;       /* brush quietly under the carpet! */
        }

        p_menu_entry->tstr_entry_text = tstr_entry_text_copy;
        p_menu_entry->h_command  = h_entry_command;
        p_menu_entry->h_command2 = h_entry_command2;
        p_menu_entry->key_code   = key_code;

        p_menu_entry->flags.active_state = active_state;

        return(STATUS_OK);
        /*NOTREACHED*/
    }

    /* failure - dispose of claimed items */
    maeve_event_handler_del_handle(p_docu, maeve_handle);

    /*tstr_clr(&entry_text);*/
    al_array_dispose(&h_entry_command);
    al_array_dispose(&h_entry_command2);

    return(STATUS_OK);
}

#define ARG_MENU_DELETE_NAME    0
#define ARG_MENU_DELETE_ENTRY   1

T5_CMD_PROTO(extern, t5_cmd_menu_delete)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 2);
    PCTSTR tstr_menu_name = p_args[ARG_MENU_DELETE_NAME].val.tstr;
    PCTSTR tstr_entry_text;
    P_MENU_ROOT p_menu_root;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(arg_is_present(p_args, ARG_MENU_DELETE_ENTRY))
        tstr_entry_text = p_args[ARG_MENU_DELETE_ENTRY].val.tstr;
    else
        tstr_entry_text = NULL;

    /* allow MenuDelete <$>,<nothing> to clear whole menu tree */
    if(NULL == tstr_entry_text)
    {
        MENU_ROOT_ID menu_root_id;

        for(menu_root_id = MENU_ROOT_DOCU; menu_root_id < elemof32(menu_root_table); ENUM_INCR(MENU_ROOT_ID, menu_root_id))
        {
            if(0 == tstrcmp(tstr_menu_name, menu_root_table[menu_root_id].name))
            {
                recursive_menu_dispose(p_docu, sk_menu_root(p_docu, menu_root_id));
                return(STATUS_OK);      /* root wiped as requested */
            }
        }

        /* if not wiping root, MUST have entry name */
        return(STATUS_OK);      /*>>>missing parameters */
    }

    if(search_for_root(p_docu, tstr_menu_name, &p_menu_root))
    {
        /* the 'remove from' menu looks kosher, so check it to see if entry exists */
        P_MENU_ENTRY p_menu_entry = NULL;
        BOOL found = search_for_entry(p_menu_root, tstr_entry_text, &p_menu_entry);

        if(found)
        {
            ARRAY_INDEX index_last;
            P_MENU_ENTRY p_menu_entry_last;

            if(p_menu_entry->key_code)
                command_array_handle_dispose_from_key_code(p_docu, p_menu_entry->key_code);     /* undefine key binding */

            /*tstr_clr(&p_menu_entry->tstr_entry_text); alloc_block*/
            al_array_dispose(&p_menu_entry->h_command);                 /* kill command macro string */
            al_array_dispose(&p_menu_entry->h_command2);                /* kill command macro string */

            recursive_menu_dispose(p_docu, &p_menu_entry->sub_menu);    /* kill old sub_menu */

            /* cut unwanted entry from array */
            index_last        = array_elements(&p_menu_root->h_entry_list) - 1;
            p_menu_entry_last = array_ptr(&p_menu_root->h_entry_list, MENU_ENTRY, index_last);
            while(p_menu_entry < p_menu_entry_last)
            {
                *p_menu_entry = *(p_menu_entry + 1);
                p_menu_entry++;
            }

            al_array_shrink_by(&p_menu_root->h_entry_list, -1);

            return(STATUS_OK);          /* Menu entry (and its submenus) disposed of OK */
        }
      /*else                */
      /*    naff entry name */
    }
  /*else               */
  /*    naff menu name */

    return(STATUS_OK);  /*>>>naff menu name */
}

#define ARG_MENU_NAME_NAME    0
#define ARG_MENU_NAME_ENTRY   1

T5_CMD_PROTO(extern, t5_cmd_menu_name)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 2);
    PCTSTR tstr_menu_name = p_args[ARG_MENU_NAME_NAME].val.tstr;
    PCTSTR tstr_entry_text = p_args[ARG_MENU_NAME_ENTRY].val.tstr;
    P_MENU_ROOT p_menu_root;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(search_for_root(p_docu, tstr_menu_name, &p_menu_root))
        tstr_xstrkpy(p_menu_root->name, elemof32(p_menu_root->name), tstr_entry_text);

    return(STATUS_OK);
}

/* end of sk_menu.c */
