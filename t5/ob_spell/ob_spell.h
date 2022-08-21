/* ob_spell.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Spell object module internal header */

/* RCM Nov 1992 */

#ifndef __ob_spell_h
#define __ob_spell_h

#include "cmodules/spell.h"

typedef U32 DICTIONARY_ID;

#define DICTIONARY_NONE ((DICTIONARY_ID) 0)
#define DICTIONARY_USER ((DICTIONARY_ID) 1)
#define DICTIONARY_USER_CAPITALISED ((DICTIONARY_ID) 2)
#define DICTIONARY_MASTER ((DICTIONARY_ID) 3)
#define DICTIONARY_MASTER_CAPITALISED ((DICTIONARY_ID) 4)
#define DICTIONARIES_N_REAL DICTIONARY_MASTER_CAPITALISED
#define DICTIONARY_SKIP_LIST ((DICTIONARY_ID) 5)

typedef struct SPELL_INSTANCE_DATA
{
    LIST_BLOCK skip_list;
}
SPELL_INSTANCE_DATA, * P_SPELL_INSTANCE_DATA;

_Check_return_
_Ret_valid_
static inline P_SPELL_INSTANCE_DATA
p_object_instance_data_SPELL(
    _InRef_     P_DOCU p_docu)
{
    const P_SPELL_INSTANCE_DATA p_spell_instance_data = (P_SPELL_INSTANCE_DATA)
        _p_object_instance_data(p_docu, OBJECT_ID_SPELL CODE_ANALYSIS_ONLY_ARG(sizeof32(SPELL_INSTANCE_DATA)));
    PTR_ASSERT(p_spell_instance_data);
    return(p_spell_instance_data);
}

#define TRACE_APP_TYPE5_SPELL TRACE_APP_PRINT

/*
internally exported routines from ob_spell.c
*/

_Check_return_
extern STATUS
ob_spell_checkword(
    _DocuRef_   P_DOCU p_docu,
    _In_z_      PC_USTR word,
    _InVal_     BOOL search_skip_list,
    _InVal_     BOOL search_capitals,
    _InVal_     BOOL search_all_dicts);

_Check_return_
extern STATUS
ob_spell_nextword(
    _Out_writes_z_(sizeof_wordout) P_SBSTR wordout,
    _InVal_     S32 sizeof_wordout,
    _In_z_      PC_SBSTR wordin,
    _In_opt_z_  PC_U8Z mask,
    _InoutRef_  P_S32 brkflg,
    _InVal_     S32 search_all_dicts);

_Check_return_
extern STATUS
ob_spell_prevword(
    _Out_writes_z_(sizeof_wordout) P_SBSTR wordout,
    _InVal_     S32 sizeof_wordout,
    _In_z_      PC_SBSTR wordin,
    _In_opt_z_  PC_U8Z mask,
    _InoutRef_  P_S32 brkflg,
    _InVal_     S32 search_all_dicts);

extern void
ob_spell_setcase(
    P_U8 wordout,
    _In_        DICTIONARY_ID dictionary_id);

_Check_return_
extern STATUS
ob_spell_isupper(
    _InVal_     U8 ch);

_Check_return_
extern STATUS
ob_spell_valid_1(
    _InVal_     U8 ch);

_Check_return_
extern STATUS
ob_spell_iswordc(
    _InVal_     U8 ch);

_Check_return_
extern STATUS
ob_spell_tolower(
    _InVal_     U8 ch);

_Check_return_
extern STATUS
skip_list_add_word(
    _DocuRef_   P_DOCU p_docu,
    _In_z_      PC_USTR ustr);

/*
internally exported routines from ui_check.c
*/

_Check_return_
extern STATUS
t5_cmd_spell_check(
    _DocuRef_   P_DOCU p_docu);

_Check_return_
extern STATUS
ui_check_dbox(
    _DocuRef_   P_DOCU p_docu,
    P_WORD_CHECK p_word_check,
    _In_z_      PC_USTR ustr);

/*
internally exported routine from ui_dict.c
*/

_Check_return_
extern STATUS
t5_cmd_spell_dictionary(
    _DocuRef_   P_DOCU p_docu);

/*
internally exported routines from guess.c
*/

extern void
guess_end(void);

_Check_return_
extern STATUS
guess_init(
    _In_z_      PC_USTR word);

_Check_return_
extern STATUS
guess_next(
    _DocuRef_   P_DOCU p_docu,
    /*out*/ P_U8Z word,
    _InVal_     S32 sizeof_word);

#define GUESS_REMAP  0  /* actually >=0 */
#define GUESS_CHAR  -1
#define GUESS_SKIP  -2

#define GUESS_INIT      1
#define GUESS_CYCLE     2
#define GUESS_DOUBLE    3
#define GUESS_REMOVE    4
#define GUESS_SWAP      5
#define GUESS_REPLACE   6
#define GUESS_FINISHED  7

#include "ob_spell/resource/resource.h"

#endif /* __ob_spell_h */

/* end of ob_spell.h */
