/* xp_file.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* File service object module header */

/* SKS April 1992 */

#ifndef __xp_file_h
#define __xp_file_h

/*
error numbers (sadly these need exposing to other modules)
*/

#define FILE_ERR__PRIVATE_BASE  ((STATUS_ERR_INCREMENT * OBJECT_ID_FILE) - 100)

#define FILE_ERR__PRIVATE(n)    (FILE_ERR__PRIVATE_BASE - (n))

#define FILE_ERR_LOADSTRFAIL    FILE_ERR__PRIVATE(0)
#define FILE_ERR_FINDRESFAIL    FILE_ERR__PRIVATE(1)
#define FILE_ERR_LOADRESFAIL    FILE_ERR__PRIVATE(2)

/* abstract file handle */
#ifndef __FILE_HANDLE_DEFINED
typedef struct __FILE_HANDLE * FILE_HANDLE;
#endif

#if WINDOWS

/*
exports from ob_file.c
*/

_Check_return_
extern STATUS
windows_filter_list_add(
    _InoutRef_  P_QUICK_TBLOCK p_filter_quick_tblock,
    _In_z_      PCTSTR tstr_filter_text,
    _In_z_      PCTSTR tstr_filter_wildcard_srch);

_Check_return_
extern STATUS
windows_filter_list_finish(
    _InoutRef_  P_QUICK_TBLOCK p_filter_quick_tblock,
    _In_        STATUS status);

_Check_return_
extern STATUS /*n_filters*/
windows_filter_list_create(
    _InoutRef_  P_QUICK_TBLOCK p_filter_quick_tblock,
    _InVal_     S32 filter_mask,
    _InRef_     PC_ARRAY_HANDLE p_h_save_filetype /* SAVE_FILETYPE[] */);

_Check_return_
extern S32
windows_filter_list_get_filter_index_from_t5_filetype(
    _InVal_     T5_FILETYPE t5_filetype_in,
    _InVal_     S32 filter_mask,
    _InRef_     PC_ARRAY_HANDLE p_h_save_filetype /* SAVE_FILETYPE[] */);

_Check_return_
extern T5_FILETYPE
windows_filter_list_get_t5_filetype_from_filter_index(
    _InVal_     S32 filter_index,
    _InVal_     S32 filter_mask,
    _InRef_     PC_ARRAY_HANDLE p_h_save_filetype /* SAVE_FILETYPE[] */);

#endif

_Check_return_
extern BOOL
GetPersonalDirectoryName(
    _Out_writes_z_(elemof_buffer) PTSTR p_dir_name /*filled*/,
    _InVal_     U32 elemof_buffer);

_Check_return_
extern BOOL
GetCommonAppDataDirectoryName(
    _Out_writes_z_(elemof_buffer) PTSTR p_dir_name /*filled*/,
    _InVal_     U32 elemof_buffer);

_Check_return_
extern BOOL
GetUserAppDataDirectoryName(
    _Out_writes_z_(elemof_buffer) PTSTR p_dir_name /*filled*/,
    _InVal_     U32 elemof_buffer);

_Check_return_
extern BOOL
GetCommonTemplatesDirectoryName(
    _Out_writes_z_(elemof_buffer) PTSTR p_dir_name /*filled*/,
    _InVal_     U32 elemof_buffer);

_Check_return_
extern BOOL
GetUserTemplatesDirectoryName(
    _Out_writes_z_(elemof_buffer) PTSTR p_dir_name /*filled*/,
    _InVal_     U32 elemof_buffer);

#endif /* __xp_file_h */

/* end of xp_file.h */
