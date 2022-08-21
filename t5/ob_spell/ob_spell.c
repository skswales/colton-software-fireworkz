/* ob_spell.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Spell object module for Fireworkz */

/* RCM June 1992 */

#include "common/gflags.h"

#include "ob_spell/ob_spell.h"

#include "cmodules/collect.h"

#define IMMEDIATE_UPDATE FALSE

#if RISCOS
#if defined(BOUND_MESSAGES_OBJECT_ID_SPELL)
extern PC_U8 rb_spell_msg_weak;
#define P_BOUND_MESSAGES_OBJECT_ID_SPELL &rb_spell_msg_weak
#else
#define P_BOUND_MESSAGES_OBJECT_ID_SPELL LOAD_MESSAGES_FILE
#endif
#else
#define P_BOUND_MESSAGES_OBJECT_ID_SPELL DONT_LOAD_MESSAGES_FILE
#endif

/*
internal routines
*/

_Check_return_
static STATUS
opendicts(void);

_Check_return_
static STATUS
closedict(
    _InVal_     DICTIONARY_ID dictionary_id,
    _InVal_     BOOL allow_write);

_Check_return_
static BOOL
skip_list_find_word(
    _DocuRef_   P_DOCU p_docu,
    _In_z_      PC_USTR ustr);

T5_CMD_PROTO(static, t5_cmd_spell_dictionary_add_word);
T5_CMD_PROTO(static, t5_cmd_spell_dictionary_delete_word);

T5_MSG_PROTO(static, t5_msg_spell_word_check, _InoutRef_ P_WORD_CHECK p_word_check);

/*
construct argument types
*/

static const ARG_TYPE
args_cmd_dictionary_add_word[] =
{
    ARG_TYPE_USTR | ARG_MANDATORY,
    ARG_TYPE_NONE
};

static const ARG_TYPE
args_cmd_dictionary_delete_word[] =
{
    ARG_TYPE_USTR | ARG_MANDATORY,
    ARG_TYPE_NONE
};

/*
construct table
*/

static CONSTRUCT_TABLE
object_construct_table[] =
{                                                                                                   /*   fi ti mi ur up xi md mf nn cp sm ba fo */

    { "Dictionary",             NULL,                               T5_CMD_SPELL_DICTIONARY },
    { "DictAddWord",            args_cmd_dictionary_add_word,       T5_CMD_SPELL_DICTIONARY_ADD_WORD },
    { "DictDeleteWord",         args_cmd_dictionary_delete_word,    T5_CMD_SPELL_DICTIONARY_DELETE_WORD },
    { "SpellCheck",             NULL,                               T5_CMD_SPELL_CHECK },

    { NULL,                     NULL,                               T5_EVENT_NONE } /* end of table */
};

static
struct OB_SPELL_STATICS
{
    struct OB_SPELL_STATICS_DICTIONARY_ENTRY
    {
        STATUS status;
        DICT_NUMBER dict_number;
        PCTSTR name;
    } dictionary[1 + DICTIONARIES_N_REAL];
}
ob_spell_ =
{
    { /*dictionary[]*/
    { STATUS_FAIL, DICTIONARY_NONE, NULL },
    { STATUS_FAIL, DICTIONARY_NONE, TEXT("Dicts") FILE_DIR_SEP_TSTR TEXT("User") },
    { STATUS_FAIL, DICTIONARY_NONE, TEXT("Dicts") FILE_DIR_SEP_TSTR TEXT("UserC") },
    { STATUS_FAIL, DICTIONARY_NONE, TEXT("Dicts") FILE_DIR_SEP_TSTR TEXT("Master") },
    { STATUS_FAIL, DICTIONARY_NONE, TEXT("Dicts") FILE_DIR_SEP_TSTR TEXT("MasterC") }
    } /*dictionary[]*/
};

/******************************************************************************
*
* spell object event handler
*
******************************************************************************/

_Check_return_
static STATUS
spell_msg_startup(void)
{
    status_consume(register_object_construct_table(OBJECT_ID_SPELL, object_construct_table, FALSE /* no inlines */));

    return(opendicts()); /* SKS 02aug95 - look, if you've loaded the spell module you are going to use it! */
}

_Check_return_
static STATUS
spell_msg_exit1(void)
{
    /* nothing to go wrong closing these read-only ones! */
    status_assert(closedict(DICTIONARY_MASTER, 0));
    status_assert(closedict(DICTIONARY_MASTER_CAPITALISED, 0));

    /* unfortunately by the time we get called the config document has been killed so we can't use that! */
    {
    STATUS status1;
    if(status_fail(status1 = closedict(DICTIONARY_USER, global_preferences.spell_write_user)))
        reperr_null(status1);
    } /*block*/

    {
    STATUS status1;
    if(status_fail(status1 = closedict(DICTIONARY_USER_CAPITALISED, global_preferences.spell_write_user)))
        reperr_null(status1);
    } /*block*/

    return(resource_close(OBJECT_ID_SPELL));
}

_Check_return_
static STATUS
spell_msg_init1(
    _DocuRef_   P_DOCU p_docu)
{
    status_return(object_instance_data_alloc(p_docu, OBJECT_ID_SPELL, sizeof32(SPELL_INSTANCE_DATA)));

    {
    const P_SPELL_INSTANCE_DATA p_spell_instance = p_object_instance_data_SPELL(p_docu);
    PTR_ASSERT(p_spell_instance);
    list_init(&p_spell_instance->skip_list);
    } /*block*/

    return(STATUS_OK);
}

_Check_return_
static STATUS
spell_msg_close1(
    _DocuRef_   P_DOCU p_docu)
{
    const P_SPELL_INSTANCE_DATA p_spell_instance = p_object_instance_data_SPELL(p_docu);

    /* care here, as failure during startup may have half-cocked resources */
    if(!IS_P_DATA_NONE(p_spell_instance))
    {
        collect_delete(&p_spell_instance->skip_list);
    }
    PTR_ASSERT(p_spell_instance);

    return(STATUS_OK);
}

T5_MSG_PROTO(static, spell_msg_initclose, _InRef_ PC_MSG_INITCLOSE p_msg_initclose)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__STARTUP:
        status_return(resource_init(OBJECT_ID_SPELL, P_BOUND_MESSAGES_OBJECT_ID_SPELL, NULL));

        /* SKS 03jan94 initialise statics for Windows restart */
        spell_startup();

        return(spell_msg_startup());

    case T5_MSG_IC__EXIT1:
        return(spell_msg_exit1());

    case T5_MSG_IC__INIT1: /* initialise object in new document */
        return(spell_msg_init1(p_docu));

    case T5_MSG_IC__CLOSE1:
        return(spell_msg_close1(p_docu));

    default:
        return(STATUS_OK);
    }
}

T5_MSG_PROTO(static, spell_msg_spell_isupper, _InRef_ PC_S32 p_s32)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(status_fail(ob_spell_.dictionary[DICTIONARY_USER].status))
        return(STATUS_FAIL);

    return(spell_isupper(ob_spell_.dictionary[DICTIONARY_USER].dict_number, *p_s32));
}

T5_MSG_PROTO(static, spell_msg_spell_iswordc, _InRef_ PC_S32 p_s32)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(status_fail(ob_spell_.dictionary[DICTIONARY_USER].status))
        return(STATUS_FAIL);

    return(spell_iswordc(ob_spell_.dictionary[DICTIONARY_USER].dict_number, *p_s32));
}

T5_MSG_PROTO(static, spell_msg_spell_tolower, _InRef_ PC_S32 p_s32)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(status_fail(ob_spell_.dictionary[DICTIONARY_USER].status))
        return(STATUS_FAIL);

    return(spell_tolower(ob_spell_.dictionary[DICTIONARY_USER].dict_number, *p_s32));
}

T5_MSG_PROTO(static, spell_msg_spell_toupper, _InRef_ PC_S32 p_s32)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(status_fail(ob_spell_.dictionary[DICTIONARY_USER].status))
        return(STATUS_FAIL);

    return(spell_toupper(ob_spell_.dictionary[DICTIONARY_USER].dict_number, *p_s32));
}

T5_MSG_PROTO(static, spell_msg_spell_valid1, _InRef_ PC_S32 p_s32)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(status_fail(ob_spell_.dictionary[DICTIONARY_USER].status))
        return(STATUS_FAIL);

    return(spell_valid_1(ob_spell_.dictionary[DICTIONARY_USER].dict_number, *p_s32));
}

OBJECT_PROTO(extern, object_spell);
OBJECT_PROTO(extern, object_spell)
{
    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        return(spell_msg_initclose(p_docu, t5_message, (PC_MSG_INITCLOSE) p_data));

    case T5_MSG_SPELL_ISUPPER:
        return(spell_msg_spell_isupper(p_docu, t5_message, (PC_S32) p_data));

    case T5_MSG_SPELL_ISWORDC:
        return(spell_msg_spell_iswordc(p_docu, t5_message, (PC_S32) p_data));

    case T5_MSG_SPELL_TOLOWER:
        return(spell_msg_spell_tolower(p_docu, t5_message, (PC_S32) p_data));

    case T5_MSG_SPELL_TOUPPER:
        return(spell_msg_spell_toupper(p_docu, t5_message, (PC_S32) p_data));

    case T5_MSG_SPELL_VALID1:
        return(spell_msg_spell_valid1(p_docu, t5_message, (PC_S32) p_data));

    case T5_MSG_SPELL_WORD_CHECK:
        return(t5_msg_spell_word_check(p_docu, t5_message, (P_WORD_CHECK) p_data));

    case T5_CMD_SPELL_CHECK:
        return(t5_cmd_spell_check(p_docu));

    case T5_CMD_SPELL_DICTIONARY:
        return(t5_cmd_spell_dictionary(p_docu));

    case T5_CMD_SPELL_DICTIONARY_ADD_WORD:
        return(t5_cmd_spell_dictionary_add_word(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_SPELL_DICTIONARY_DELETE_WORD:
        return(t5_cmd_spell_dictionary_delete_word(p_docu, t5_message, (PC_T5_CMD) p_data));

    default:
        return(STATUS_OK);
    }
}

/******************************************************************************
*
* <0 error
*  0 not found - skip it
*    not found - replaced, resubmit
*    word too long
*
******************************************************************************/

T5_MSG_PROTO(static, t5_msg_spell_word_check, _InoutRef_ P_WORD_CHECK p_word_check)
{
    STATUS result = STATUS_OK;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    PTR_ASSERT(p_word_check);

    result = CHECK_CONTINUE;

    if(0 != ustrlen32(p_word_check->ustr_word))
    {
        PC_U8Z p_u8 = (PC_U8Z) p_word_check->ustr_word;
        PC_U8Z src = p_u8;
        U8Z word[BUF_MAX_WORD];
        U32 dst_idx = 0;
        P_U8 hptr = NULL;

        /* skip forward over funny chars e.g. Rob's famous '[Section]' */
        while(*src && !ob_spell_valid_1(*src))
            ++src;

        if(*src && ob_spell_valid_1(*src))
        {
            while(ob_spell_iswordc(*src) && (dst_idx < MAX_WORD))
            {
                if((*src == CH_HYPHEN_MINUS) && (NULL == hptr))
                    hptr = &word[dst_idx]; /* register position of (first) hyphen */

                word[dst_idx++] = *src++;
            }
        }

        word[dst_idx] = CH_NULL;

        if((strlen32(word) != 0) && (strlen32(word) <= MAX_WORD))
        {
            BOOL search_capitals = 0;

            { /* SKS 04apr95 make work without user dicts. Assumes dicts to use all open at this point */
            DICTIONARY_ID dictionary_id = DICTIONARY_USER_CAPITALISED;
            if(status_fail(ob_spell_.dictionary[dictionary_id].status))
                dictionary_id = DICTIONARY_MASTER_CAPITALISED;
            if(status_ok(ob_spell_.dictionary[dictionary_id].status))
                if(spell_isupper(ob_spell_.dictionary[dictionary_id].dict_number, *word) > 0)
                    search_capitals = 1;
            } /*block*/

            /* search skip list and dictionaries */
            if(SPELL_ERR_BADWORD == (result = ob_spell_checkword(p_docu, (PC_USTR) word, TRUE, search_capitals, TRUE)))
                result = 0;

            if(result == 0)
            {
                S32 len = strlen32(word);

                if( (len > 1) && (word[len - 1] == CH_APOSTROPHE) )
                {
                    word[len - 1] = CH_NULL; /* poke out the trailing quote */

                    if(SPELL_ERR_BADWORD == (result = ob_spell_checkword(p_docu, (PC_USTR) word, TRUE, search_capitals, TRUE)))
                        result = 0;

                    word[len - 1] = CH_APOSTROPHE; /* poke it back */
                }
            }

            if(result == 0)
            {
                S32 len = strlen32(word);

                if( (len > 2) && (word[len - 2] == CH_APOSTROPHE) && ((word[len - 1] == 's') || (word[len - 1] == 'S')) )
                {
                    word[len - 2] = CH_NULL; /* poke out the trailing quote 's' */

                    if(SPELL_ERR_BADWORD == (result = ob_spell_checkword(p_docu, (PC_USTR) word, TRUE, search_capitals, TRUE)))
                        result = 0;

                    word[len - 2] = CH_APOSTROPHE; /* poke it back */
                }
            }

            if(result == 0)
            {
                if(hptr)
                {
                    P_U8 next;

                    /* hyphens are not accepted as valid first chars, but we may have a trailing hyphen */

                    *hptr = CH_NULL; /* poke out the hyphen */

                    if(SPELL_ERR_BADWORD == (result = ob_spell_checkword(p_docu, (PC_USTR) word, TRUE, search_capitals, TRUE)))
                        result = 0;

                    *hptr = CH_HYPHEN_MINUS; /* poke the hyphen back */

                    while(hptr && (result > 0))
                    {
                        P_U8 dst;

                        next = hptr + 1;

                        if(!*next)
                            break;

                        dst  = next;
                        hptr = NULL;
                        while(*dst)
                        {
                            if(*dst == CH_HYPHEN_MINUS)
                            {
                                hptr = dst;
                                break;
                            }
                            dst++;
                        }

                        if(next == hptr)
                        {
                            result = 0;
                            break;
                        }

                        if(hptr)
                            *hptr = CH_NULL;       /* poke out the hyphen */

                        if(SPELL_ERR_BADWORD == (result = ob_spell_checkword(p_docu, (PC_USTR) next, TRUE, FALSE, TRUE)))
                            result = 0;

                        if(hptr)
                            *hptr = CH_HYPHEN_MINUS; /* poke it back */
                    }
                }
            }

            if(status_ok(result))
            {
                if(result == 0)
                {
                    /* word not in dictionary */
                    p_docu->spelling_mistakes++;

                    if(p_word_check->mistake_query)
                    {
                        /* put up dbox querrying the mistuk */
                        DOCU_AREA docu_area;

                        docu_area_from_object_data(&docu_area, &p_word_check->object_data);

                        object_skel(p_docu, T5_MSG_SELECTION_MAKE, &docu_area);

                        p_docu->cur = docu_area.tl; /* wot de fook is dis says SKS, as the focus is anywhere */

                        caret_show_claim(p_docu, p_docu->focus_owner, TRUE);

                        result = ui_check_dbox(p_docu, p_word_check, ustr_bptr(word));
                    }
                    else
                        result = CHECK_CANCEL;  /* indicate iffy word to caller */
                }
                else
                    result = CHECK_CONTINUE;    /* word is in dictionary */
            }
        }
        else
            result = CHECK_CONTINUE; /*>>>WORD_TOO_LONG;*/
    }

    p_word_check->status = result;

    return(status_fail(result) ? result : STATUS_OK);
}

/******************************************************************************
*
* Add word to user dictionary
*
******************************************************************************/

T5_CMD_PROTO(static, t5_cmd_spell_dictionary_add_word)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 1);
    const PC_USTR ustr_word = p_args[0].val.ustr;
    STATUS status = STATUS_OK;

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if((NULL != ustr_word) && (ustrlen32(ustr_word) >= 2))
    {
        const PC_SBSTR sbstr_word = _sbstr_from_ustr(ustr_word);
        /* We ask the 'proper-word' user dictionary whether it thinks the word is capitalised, */
        /* the test looks at the initial letter of the word only.                              */
        /* We rather assume that both normal and capitialised dictionaries must exist.         */
        DICTIONARY_ID dictionary_id = DICTIONARY_USER_CAPITALISED;

        if(status_fail(ob_spell_.dictionary[dictionary_id].status))
        {
            dictionary_id = DICTIONARY_USER;

            if(status_fail(ob_spell_.dictionary[dictionary_id].status))
                status = create_error(SPELL_ERR_NO_USER_DICTS);
        }

        if(status_ok(status) && status_ok(status = spell_isupper(ob_spell_.dictionary[dictionary_id].dict_number, sbstr_word[0])))
        {
            /* if the 'proper' dictionary thinks the word is not upper cased, put it in the 'normal' dictionary */
            if(status == 0)
                dictionary_id = DICTIONARY_USER; /* seems to be lower case */

            assert(status_ok(ob_spell_.dictionary[dictionary_id].status));
            status = spell_addword(ob_spell_.dictionary[dictionary_id].dict_number, sbstr_word);
            /* returns: >0 word added, =0 word exists, <0 error */
#if FALSE
            if(status == 0)
                status = ALREADY_IN_DICTIONARY;
#endif
#if IMMEDIATE_UPDATE
            if((status > 0) && ob_spell_.cfg__flags_write_user_dict)
                status = spell_write_whole(ob_spell_.dictionary[dictionary_id].dict_number);
#endif
        }
    }

    return(status);
}

T5_CMD_PROTO(static, t5_cmd_spell_dictionary_delete_word)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 1);
    const PC_USTR ustr_word = p_args[0].val.ustr;
    STATUS status = STATUS_OK;

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if((NULL != ustr_word) && (ustrlen32(ustr_word) >= 3))
    {
        PC_SBSTR sbstr_word = _sbstr_from_ustr(ustr_word);
        DICTIONARY_ID dictionary_id = DICTIONARY_USER_CAPITALISED;

        if(('*' == sbstr_word[0]) && ('\t' == sbstr_word[1]))
            sbstr_word += 2; /* skip <star><tab> - see set_wordlist_entry() */

        if(status_fail(ob_spell_.dictionary[dictionary_id].status))
        {
            dictionary_id = DICTIONARY_USER;

            if(status_fail(ob_spell_.dictionary[dictionary_id].status))
                status = create_error(SPELL_ERR_NO_USER_DICTS);
        }

        if(status_ok(status) && status_ok(status = spell_isupper(ob_spell_.dictionary[dictionary_id].dict_number, sbstr_word[0])))
        {
            if(status == 0)
                dictionary_id = DICTIONARY_USER;

            status = spell_deleteword(ob_spell_.dictionary[dictionary_id].dict_number, sbstr_word);

#if IMMEDIATE_UPDATE
            if(status_ok(status) && ob_spell_.cfg__flags_write_user_dict)
                status = spell_write_whole(ob_spell_.dictionary[dictionary_id].dict_number);
#endif
        }
    }

    return(status);
}

_Check_return_
static STATUS
opendict(
    _InVal_     DICTIONARY_ID dictionary_id)
{
    TCHARZ filename[BUF_MAX_PATHSTRING];
    PCTSTR copy_right;
    STATUS status;

    if(status_ok(ob_spell_.dictionary[dictionary_id].status))
        return(STATUS_OK);

    if((status = file_find_on_path(filename, elemof32(filename), file_get_search_path(), ob_spell_.dictionary[dictionary_id].name)) <= 0)
    {
        if(status == 0)
            status = create_error(FILE_ERR_NOTFOUND);
        ob_spell_.dictionary[dictionary_id].status = status;
    }
    else if(status_ok(status = spell_opendict(filename, &copy_right, TRUE)))
    {
        ob_spell_.dictionary[dictionary_id].status = STATUS_DONE;
        ob_spell_.dictionary[dictionary_id].dict_number = (DICT_NUMBER) status;
        status = STATUS_OK;
    }

    return(status);
}

_Check_return_
static STATUS
opendicts(void)
{
    { /* SKS 15may95 report errors better here */
    DICTIONARY_ID dictionary_id = DICTIONARY_MASTER;
    STATUS status = opendict(dictionary_id);
    if(status_fail(status))
    {
        reperr(status, ob_spell_.dictionary[dictionary_id].name);
        return(STATUS_CANCEL);
    }
    } /*block*/

    {
    DICTIONARY_ID dictionary_id = DICTIONARY_MASTER_CAPITALISED;
    STATUS status = opendict(dictionary_id);
    if(status_fail(status))
    {
        reperr(status, ob_spell_.dictionary[dictionary_id].name);
        return(STATUS_CANCEL);
    }
    } /*block*/

    { /* SKS 04apr95 allow for none, one or two user dicts */
    STATUS status = opendict(DICTIONARY_USER);
    if(status_ok(status))
        status = opendict(DICTIONARY_USER_CAPITALISED);
    if(FILE_ERR_NOTFOUND != status)
        return(status);
    } /*block*/

    return(STATUS_OK);
}

_Check_return_
static STATUS
closedict(
    _InVal_     DICTIONARY_ID dictionary_id,
    _InVal_     BOOL allow_write)
{
    STATUS status = STATUS_OK;

    if(status_ok(ob_spell_.dictionary[dictionary_id].status))
    {
        DICT_NUMBER dict_number = ob_spell_.dictionary[dictionary_id].dict_number;

        if(allow_write)
            status = spell_write_whole(dict_number);

        status_accumulate(status, spell_close(dict_number));

        ob_spell_.dictionary[dictionary_id].status = STATUS_FAIL;
        ob_spell_.dictionary[dictionary_id].dict_number = DICTIONARY_NONE;
    }

    return(status);
}

/******************************************************************************
*
* search_all_dicts: 0=search user dicts, 1=search master and user dicts
*
* <0 error
*  0 not found
* >0 found: 1 in lower case user dictionary
*            2 in upper case user dictionary
*            3 in lower case master dictionary
*            4 in upper case master dictionary
*            5 in skip list
*
******************************************************************************/

_Check_return_
extern STATUS
ob_spell_checkword(
    _DocuRef_   P_DOCU p_docu,
    _In_z_      PC_USTR word,
    _InVal_     BOOL search_skip_list,
    _InVal_     BOOL search_capitals,
    _InVal_     BOOL search_all_dicts)
{
    STATUS status;
    DICTIONARY_ID dictionary_id;

    dictionary_id = DICTIONARY_MASTER;

    if(status_ok(ob_spell_.dictionary[dictionary_id].status) && search_all_dicts)
    {
        status = spell_checkword(ob_spell_.dictionary[dictionary_id].dict_number, _sbstr_from_ustr(word));

        /* 24.3.95 MRJC: a word may be bad for an individual dictionary, after all */
        if(SPELL_ERR_BADWORD != status)
            status_return(status);

        if(status > 0)
            return(dictionary_id);
    }

    if(search_skip_list && skip_list_find_word(p_docu, word))
        return(DICTIONARY_SKIP_LIST);

    dictionary_id = 0;

    while(++dictionary_id <= DICTIONARIES_N_REAL)
    {
        if(status_fail(ob_spell_.dictionary[dictionary_id].status))
            continue;

        switch(dictionary_id)
        {
        default: default_unhandled();
#if CHECKING
        case DICTIONARY_MASTER: /* already checked */
#endif
            break;

        case DICTIONARY_MASTER_CAPITALISED:
            if(!search_all_dicts)
                break;
          /*else*/

            /*FALLTHRU*/

        case DICTIONARY_USER_CAPITALISED:
            if(!search_capitals)
                break;
          /*else*/

            /*FALLTHRU*/

        case DICTIONARY_USER:
            status = spell_checkword(ob_spell_.dictionary[dictionary_id].dict_number, _sbstr_from_ustr(word));

            /* 24.3.95 MRJC: a word may be bad for an individual dictionary, after all */
            if(SPELL_ERR_BADWORD != status)
                status_return(status);

            if(status > 0)
                return(dictionary_id);

            break;
        }
    }

    return(0);
}

/******************************************************************************
*
* <0 error
*  0 not found
* >0 found in whichever dictionary_id
*
******************************************************************************/

_Check_return_
extern STATUS
ob_spell_nextword(
    _Out_writes_z_(sizeof_wordout) P_SBSTR wordout,
    _InVal_     S32 sizeof_wordout,
    _In_z_      PC_SBSTR wordin,
    _In_opt_z_  PC_U8Z mask,
    _InoutRef_  P_S32 brkflg,
    _InVal_     S32 search_all_dicts)
{
    STATUS status;
    U8Z copy_wordin[BUF_MAX_WORD];
    U8Z word[BUF_MAX_WORD];
    DICTIONARY_ID dictionary_id;
    S32 found = 0;

    xstrkpy(copy_wordin, sizeof32(copy_wordin), wordin);        /* must copy wordin, as wordout usually = wordin */
    *wordout = CH_NULL;

    dictionary_id = DICTIONARIES_N_REAL + 1;

    while(--dictionary_id > 0)
    {
        if(status_fail(ob_spell_.dictionary[dictionary_id].status))
            continue;

        switch(dictionary_id)
        {
        case DICTIONARY_MASTER:
        case DICTIONARY_MASTER_CAPITALISED:
            if(!search_all_dicts)
                break;
          /*else*/

            /*FALLTHRU*/

        default:
            status = spell_nextword(ob_spell_.dictionary[dictionary_id].dict_number, word, sizeof32(word), copy_wordin, mask, brkflg);

            /* 24.3.95 MRJC: dictionary may return bad word: that's cool */
            if(SPELL_ERR_BADWORD == status)
                status = 0;
            else
                status_return(status);

            if(status)
            {
                if(!*wordout || (*word && (/*"C"*/strcmp(wordout, word) > 0))) /* force assignment if wordout is empty */
                {
                    xstrkpy(wordout, sizeof_wordout, word);
                    found = dictionary_id;       /* indicate which dictionary contained the word, so we can case-change if needed */
                }
            }

            break;
        }
    }

    return(found);
}

/******************************************************************************
*
* <0 error
*  0 not found
* >0 found in whichever dictionary
*
******************************************************************************/

_Check_return_
extern STATUS
ob_spell_prevword(
    _Out_writes_z_(sizeof_wordout) P_SBSTR wordout,
    _InVal_     S32 sizeof_wordout,
    _In_z_      PC_SBSTR wordin,
    _In_opt_z_  PC_U8Z mask,
    _InoutRef_  P_S32 brkflg,
    _InVal_     S32 search_all_dicts)
{
    STATUS status;
    U8Z copy_wordin[BUF_MAX_WORD];
    U8Z word[BUF_MAX_WORD];
    DICTIONARY_ID dictionary_id;
    S32 found = 0;

    xstrkpy(copy_wordin, sizeof32(copy_wordin), wordin);        /* must copy wordin, as wordout usually = wordin */
    *wordout = CH_NULL;

    dictionary_id = DICTIONARIES_N_REAL + 1;

    while(--dictionary_id > 0)
    {
        if(status_fail(ob_spell_.dictionary[dictionary_id].status))
            continue;

        switch(dictionary_id)
        {
        case DICTIONARY_MASTER:
        case DICTIONARY_MASTER_CAPITALISED:
            if(!search_all_dicts)
                break;
          /*else*/

            /*FALLTHRU*/

        default:
            status = spell_prevword(ob_spell_.dictionary[dictionary_id].dict_number, word, sizeof32(word), copy_wordin, mask, brkflg);

            /* 24.3.95 MRJC: dictionary may return bad word: that's cool */
            if(SPELL_ERR_BADWORD == status)
                status = 0;
            else
                status_return(status);

            if(status)
            {
                if(!*wordout || (*word && (/*"C"*/strcmp(wordout, word) < 0))) /* force assignment if wordout is empty */
                {
                    xstrkpy(wordout, sizeof_wordout, word);
                    found = dictionary_id;       /* indicate which dictionary contained the word, so we can case-change if needed */
                }
            }

            break;
        }
    }

    return(found);
}

extern void
ob_spell_setcase(
    P_U8 wordout,
    _In_        DICTIONARY_ID dictionary_id)
{
    switch(dictionary_id)
    {
    case DICTIONARY_USER_CAPITALISED:
        if(status_fail(ob_spell_.dictionary[dictionary_id].status))
            dictionary_id = DICTIONARY_MASTER_CAPITALISED;

        /*FALLTHRU*/

    case DICTIONARY_MASTER_CAPITALISED:
        {
        STATUS status = spell_toupper(ob_spell_.dictionary[dictionary_id].dict_number, wordout[0]);
        if(status_ok(status))
            wordout[0] = (U8) status;
        break;
        }

    default:
        break;
    }
}

_Check_return_
extern STATUS
ob_spell_isupper(
    _InVal_     U8 ch)
{
    DICTIONARY_ID dictionary_id = DICTIONARY_USER;
    if(status_fail(ob_spell_.dictionary[dictionary_id].status))
        dictionary_id = DICTIONARY_MASTER;
    return(spell_isupper(ob_spell_.dictionary[dictionary_id].dict_number, ch));
}

_Check_return_
extern STATUS
ob_spell_valid_1(
    _InVal_     U8 ch)
{
    DICTIONARY_ID dictionary_id = DICTIONARY_USER;
    if(status_fail(ob_spell_.dictionary[dictionary_id].status))
        dictionary_id = DICTIONARY_MASTER;
    return(spell_valid_1(ob_spell_.dictionary[dictionary_id].dict_number, ch));
}

_Check_return_
extern STATUS
ob_spell_iswordc(
    _InVal_     U8 ch)
{
    DICTIONARY_ID dictionary_id = DICTIONARY_USER;
    if(status_fail(ob_spell_.dictionary[dictionary_id].status))
        dictionary_id = DICTIONARY_MASTER;
    return(spell_iswordc(ob_spell_.dictionary[dictionary_id].dict_number, ch));
}

_Check_return_
extern STATUS
ob_spell_tolower(
    _InVal_     U8 ch)
{
    DICTIONARY_ID dictionary_id = DICTIONARY_USER;
    if(status_fail(ob_spell_.dictionary[dictionary_id].status))
        dictionary_id = DICTIONARY_MASTER;
    return(spell_tolower(ob_spell_.dictionary[dictionary_id].dict_number, ch));
}

_Check_return_
extern STATUS
skip_list_add_word(
    _DocuRef_   P_DOCU p_docu,
    _In_z_      PC_USTR ustr)
{
    const P_SPELL_INSTANCE_DATA p_spell_instance = p_object_instance_data_SPELL(p_docu);
    const U32 size = ustrlen32p1(ustr); /*CH_NULL*/
    STATUS status;

    PTR_ASSERT(p_spell_instance);

    consume_ptr(collect_add_entry_bytes(UCHARZ, &p_spell_instance->skip_list, ustr, size, (LIST_ITEMNO) -1 /* add, don't care about itemno */, &status));

    return(status);
}

_Check_return_
static BOOL
skip_list_find_word(
    _DocuRef_   P_DOCU p_docu,
    _In_z_      PC_USTR ustr)
{
    const U32 ustr_len = ustrlen32(ustr);
    const P_SPELL_INSTANCE_DATA p_spell_instance = p_object_instance_data_SPELL(p_docu);
    LIST_ITEMNO key;
    PC_USTR ustr_entry;

    PTR_ASSERT(p_spell_instance);

    for(ustr_entry = (PC_USTR) collect_first(UCHARZ, &p_spell_instance->skip_list, &key);
        ustr_entry;
        ustr_entry = (PC_USTR) collect_next(UCHARZ, &p_spell_instance->skip_list, &key))
    {
        const U32 entry_len = ustrlen32(ustr_entry);

        if(entry_len != ustr_len)
            continue;

        /* now requires precise match - caps variants must be added separately */
        if(0 != memcmp(ustr_entry, ustr, ustr_len))
            continue;

        return(TRUE); /* found */
    }

    return(FALSE); /* not found */
}

/* end of ob_spell.c */
