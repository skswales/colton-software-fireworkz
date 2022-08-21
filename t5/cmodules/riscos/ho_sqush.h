/* ho_sqush.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#ifndef          __file_h
#include "cmodules/file.h"
#endif

_Check_return_
extern STATUS
host_squash_alloc(
    /*out*/ P_P_ANY p_p_any,
    _InVal_     U32 size,
    _InVal_     BOOL may_be_code);

extern void
host_squash_dispose(
    /*inout*/ P_P_ANY p_p_any);

_Check_return_
extern STATUS
host_squash_expand_from_file(
    _Out_writes_bytes_(expand_size) P_ANY p_any,
    _InoutRef_  FILE_HANDLE file_handle,
    _InVal_     U32 compress_size,
    _InVal_     U32 expand_size);

_Check_return_
extern STATUS
host_squash_load_data_file(
    /*out*/ P_P_ANY p_p_any,
    _InoutRef_  FILE_HANDLE file_handle);

/* end of ho_sqush.h */
