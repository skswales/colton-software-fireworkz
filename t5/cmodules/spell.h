/* spell.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1988-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* External header file for spellcheck */

/* MRJC May 1988 / November 1992 new style */

#ifndef __spell_h
#define __spell_h

typedef S32 DICT_NUMBER;

/*
function declarations
*/

_Check_return_
extern STATUS
spell_addword(
    _InVal_     DICT_NUMBER dict_number,
    _In_z_      PC_SBSTR word);

_Check_return_
extern STATUS
spell_checkword(
    _InVal_     DICT_NUMBER dict_number,
    _In_z_      PC_SBSTR word);

_Check_return_
extern STATUS
spell_close(
    _InVal_     DICT_NUMBER dict_number);

_Check_return_
extern STATUS
spell_close_file_only(
    _InVal_     DICT_NUMBER dict_number);

_Check_return_
extern STATUS
spell_createdict(
    _In_z_      PCTSTR filename,
    _In_z_      PCTSTR def_name);

_Check_return_
extern STATUS
spell_deleteword(
    _InVal_     DICT_NUMBER dict_number,
    _In_z_      PC_SBSTR word);

_Check_return_
extern STATUS
spell_flush(
    _InVal_     DICT_NUMBER dict_number);

_Check_return_
extern STATUS
spell_isupper(
    _InVal_     DICT_NUMBER dict_number,
    _InVal_     S32 ch);

_Check_return_
extern STATUS
spell_iswordc(
    _InVal_     DICT_NUMBER dict_number,
    _InVal_     S32 ch);

_Check_return_
extern STATUS
spell_load(
    _InVal_     DICT_NUMBER dict_number);

_Check_return_
extern STATUS
spell_nextword(
    _InVal_     DICT_NUMBER dict_number,
    _Out_writes_z_(sizeof_wordout) P_SBSTR wordout,
    _InVal_     S32 sizeof_wordout,
    _In_z_      PC_SBSTR wordin,
    _In_opt_z_  PC_U8Z mask,
    _InoutRef_  P_S32 brkflg);

_Check_return_
extern STATUS /*DICT_NUMBER*/
spell_opendict(
    _In_z_      PCTSTR filename,
    _OutRef_    P_PCTSTR copy_right,
    _InVal_     BOOL load_and_close_file_after);

_Check_return_
extern STATUS
spell_pack(
    _InVal_     DICT_NUMBER old_dict_number,
    _InVal_     DICT_NUMBER new_dict_number);

_Check_return_
extern STATUS
spell_prevword(
    _InVal_     DICT_NUMBER dict_number,
    _Out_writes_z_(sizeof_wordout) P_SBSTR wordout,
    _InVal_     S32 sizeof_wordout,
    _In_z_      PC_SBSTR wordin,
    _In_opt_z_  PC_U8Z mask,
    _InoutRef_  P_S32 brkflg);

_Check_return_
extern STATUS
spell_setoptions(
    _InVal_     DICT_NUMBER dict_number,
    _InVal_     S32 optionset,
    _InVal_     S32 optionmask);

extern void
spell_stats(
    P_S32 cblocks,
    P_S32 largest,
    P_S32 totalmem);

extern void
spell_startup(void);

_Check_return_
extern STATUS
spell_strnicmp(
    _InVal_     DICT_NUMBER dict_number,
    _In_z_      PC_SBSTR word1,
    _In_z_      PC_SBSTR word2,
    _InVal_     S32 len);

_Check_return_
extern STATUS
spell_tolower(
    _InVal_     DICT_NUMBER dict_number,
    _InVal_     S32 ch);

_Check_return_
extern STATUS
spell_toupper(
    _InVal_     DICT_NUMBER dict_number,
    _InVal_     S32 ch);

_Check_return_
extern STATUS
spell_unlock(
    _InVal_     DICT_NUMBER dict_number);

_Check_return_
extern STATUS
spell_valid_1(
    _InVal_     DICT_NUMBER dict_number,
    _InVal_     S32 ch);

_Check_return_
extern STATUS
spell_write_whole(
    _InVal_     DICT_NUMBER dict_number);

/*
wildcard characters
*/

#define SPELL_WILD_SINGLE   '?'
#define SPELL_WILD_MULTIPLE '*'

/*
dictionary flags
*/

#define DICT_WRITEINDEX 0x80
#define DICT_READONLY   0x40

/*
maximum length of word (MAX_CHAR - 1 + 2)
*/

#define MAX_WORD 65
#define BUF_MAX_WORD (MAX_WORD + 1)

#endif /* __spell_h */

/* end of spell.h */
