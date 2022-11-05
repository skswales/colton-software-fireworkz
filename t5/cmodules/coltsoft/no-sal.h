/* no-sal.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2012-2022 Stuart Swales */

/*
Some SAL2.0 fakery - only defined as needed
*/

#ifndef _In_reads_

#define _In_reads_(size)
#define _In_reads_opt_(size)

#define _In_reads_or_z_(size)

#define _In_reads_bytes_(size)
#define _In_reads_bytes_opt_(size)

#define _Out_writes_(size)
#define _Out_writes_opt_(size)

#define _Out_writes_to_(size,count)
#define _Out_writes_to_opt_(size,count)

#define _Out_writes_all_(size)

#define _Out_writes_z_(size)
#define _Out_writes_opt_z_(size)

#define _Out_writes_bytes_(size)
#define _Out_writes_bytes_opt_(size)

#define _Out_writes_bytes_all_(size)

#define _Inout_updates_(size)
#define _Inout_updates_opt_(size)

#define _Inout_updates_z_(size)

#define _Inout_updates_bytes_(size)
#define _Inout_updates_bytes_opt_(size)

#define _Ret_maybenull_z_

#define _Ret_writes_(size)
#define _Ret_writes_maybenull_(size)

#define _Ret_writes_to_(size, count)
#define _Ret_writes_to_maybenull_(size, count)

#define _Ret_writes_z_

#define _Ret_writes_bytes_(size)
#define _Ret_writes_bytes_maybenull_(size)

#define _Ret_writes_bytes_to_(size, count)
#define _Ret_writes_bytes_to_maybenull_(size, count)

#endif /* _In_reads_ */

/*
Some SAL1.1 fakery - only defined as needed
*/

#ifndef _Success_

#define _Check_return_

#define _Success_(expr)

#define _Pre_valid_
#define _Pre_maybenull_

#define _Post_invalid_

#define _In_
#define _In_opt_
#define _In_opt_z_
#define _In_z_

#define _Inout_
#define _Inout_bytecap_(countvar)
#define _Inout_bytecap_c_(_const_expr)

#define _Inout_opt_
#define _Inout_z_

#define _Out_
#define _Out_bytecap_(countvar)
#define _Out_bytecap_x_(complexvar)
#define _Out_bytecapcount_(countvar)
#define _Out_bytecapcount_x_(complexvar)
#define _Out_cap_(countvar)
#define _Out_cap_c_(_const_expr)
#define _Out_opt_

#define _Printf_format_string_

#define _Ret_valid_
#define _Ret_notnull_
#define _Ret_maybenull_

#define _Ret_
#define _Ret_z_
#define _Ret_opt_z_

#endif /* _Success_ */

/* end of no-sal.h */
