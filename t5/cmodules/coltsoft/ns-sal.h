/* ns-sal.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2016-2021 Stuart Swales */

/*
Some non-standard SAL hacks
*/

#ifdef _In_reads_ /* SAL2.0 */
#define _In_reads_c_(size)                              _In_reads_(size)
#define _In_reads_x_(size)                              _In_reads_(size)
#define _In_reads_opt_c_(size)                          _In_reads_opt_(size)
#define _In_reads_bytes_c_(size)                        _In_reads_bytes_(size)
#define _In_reads_bytes_x_(size)                        _In_reads_bytes_(size)
#else /* none */
#define _In_reads_c_(size)
#define _In_reads_x_(size)
#define _In_reads_opt_c_(size)
#define _In_reads_bytes_c_(size)
#define _In_reads_bytes_x_(size)
#endif

#ifdef _Out_writes_ /* SAL2.0 */
#define _Out_writes_c_(size)                            _Out_writes_(size) 
#define _Out_writes_bytes_c_(size)                      _Out_writes_bytes_(size) 
#else /* none */
#define _Out_writes_c_(size)
#define _Out_writes_bytes_c_(size)
#endif

#ifdef _Inout_updates_ /* SAL2.0 */
#define _Inout_updates_x_(size)                         _Inout_updates_(size)
#define _Inout_updates_bytes_x_(size)                   _Inout_updates_bytes_(size)
#else /* none */
#define _Inout_updates_x_(size)
#define _Inout_updates_bytes_x_(size)
#endif

#ifdef _In_reads_ /* SAL2.0 */
#else /* none */
#include "cmodules/coltsoft/no-sal.h"
#endif

/* end of ns-sal.h */
