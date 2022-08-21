/* no-sal.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2012-2019 Stuart Swales */

/* SKS 2012 */

/*
Some SAL2.0 fakes to SAL1.1 - only defined as needed
*/

#ifndef _In_reads_

#define _In_reads_(size)                                _In_count_(size)
#define _In_reads_opt_(size)                            _In_opt_count_(size)

#define _In_reads_or_z_(size)                           _In_z_count_(size)

#define _In_reads_bytes_(size)                          _In_bytecount_(size)
#define _In_reads_bytes_opt_(size)                      _In_opt_bytecount_(size)

#define _Out_writes_(size)                              _Out_cap_(size)
#define _Out_writes_opt_(size)                          _Out_opt_cap_(size)

#define _Out_writes_to_(size,count)                     _Out_cap_(size)
#define _Out_writes_to_opt_(size,count)                 _Out_opt_cap_(size)

#define _Out_writes_all_(size)                          _Out_cap_post_count_(size,size)

#define _Out_writes_z_(size)                            _Out_z_cap_(size)
#define _Out_writes_opt_z_(size)                        _Out_opt_z_cap_(size)

#define _Out_writes_bytes_(size)                        _Out_bytecap_(size)
#define _Out_writes_bytes_opt_(size)                    _Out_opt_bytecap_(size)

#define _Out_writes_bytes_all_(size)                    _Out_bytecap_post_bytecount_(size,size)

#define _Inout_updates_(size)                           _Inout_cap_(size)
#define _Inout_updates_opt_(size)                       _Inout_opt_cap_(size)

#define _Inout_updates_z_(size)                         _Inout_z_cap_(size)

#define _Inout_updates_bytes_(size)                     _Inout_bytecap_(size)
#define _Inout_updates_bytes_opt_(size)                 _Inout_opt_bytecap_(size)

#define _Ret_maybenull_z_                               _Ret_opt_z_

#define _Ret_writes_(size)                              _Ret_count_(size)
#define _Ret_writes_maybenull_(size)                    _Ret_opt_count_(size)

#define _Ret_writes_to_(size, count)                    _Ret_count_(size)
#define _Ret_writes_to_maybenull_(size, count)          _Ret_opt_count_(size)

#define _Ret_writes_z_                                  _Ret_z_

#define _Ret_writes_bytes_(size)                        _Ret_bytecap_(size)
#define _Ret_writes_bytes_maybenull_(size)              _Ret_opt_bytecap_(size)

#define _Ret_writes_bytes_to_(size, count)              _Ret_bytecap_(size)
#define _Ret_writes_bytes_to_maybenull_(size, count)    _Ret_opt_bytecap_(size)

#endif /* _In_reads_ */

/*
Some more macro defs so that we can compile code
with these VC2008-era SAL1.1 decorations on
on VC2005 (SAL 1.0 support abandoned here)
or indeed with CC Norcroft on RISC OS!
*/

#ifndef _Success_

#define _Check_return_

#define _Success_(expr)

#define _Pre_valid_
#define _Pre_notnull_
#define _Pre_maybenull_

#define _Post_invalid_

#define _In_
#define _In_bytecount_(countvar)
#define _In_bytecount_c_(_const_expr)
#define _In_bytecount_x_(complexexpr)
#define _In_count_(countvar)
#define _In_count_c_(_const_expr)
#define _In_count_x_(complexexpr)
#define _In_opt_
#define _In_opt_bytecount_(countvar)
#define _In_opt_count_(countvar)
#define _In_opt_count_c_(_const_expr)
#define _In_opt_count_x_(complexexpr)
#define _In_opt_z_
#define _In_z_
#define _In_z_bytecount_(countvar)
#define _In_z_count_(countvar)

#define _Inout_
#define _Inout_bytecap_(countvar)
#define _Inout_bytecap_c_(_const_expr)
#define _Inout_bytecap_x_(complexexpr)
#define _Inout_bytecount_(countvar)
#define _Inout_bytecount_c_(_const_expr)
#define _Inout_bytecount_x_(complexexpr)
#define _Inout_cap_(countvar)
#define _Inout_cap_c_(_const_expr)
#define _Inout_cap_x_(complexexpr)
#define _Inout_count_(countvar)
#define _Inout_count_c_(_const_expr)
#define _Inout_count_x_(complexexpr)
#define _Inout_opt_
#define _Inout_z_
#define _Inout_z_bytecap_(countvar)
#define _Inout_z_cap_(countvar)

#define _Out_
#define _Out_bytecap_(countvar)
#define _Out_bytecap_c_(_const_expr)
#define _Out_bytecap_x_(complexvar)
#define _Out_bytecapcount_(countvar)
#define _Out_bytecapcount_c_(_const_expr)
#define _Out_bytecapcount_x_(complexvar)
#define _Out_bytecap_post_bytecount_(countvar1,countvar2)
#define _Out_cap_(countvar)
#define _Out_cap_c_(_const_expr)
#define _Out_cap_x_(complexexpr)
#define _Out_capcount_(countvar)
/*#define _Out_capcount_c_(_const_expr)*/
#define _Out_capcount_x_(complexexpr)
#define _Out_cap_post_count_(countvar1,countvar2)
#define _Out_opt_
#define _Out_opt_bytecap_(countvar) 
#define _Out_opt_cap_(countvar)
#define _Out_opt_z_bytecap_(countvar)
#define _Out_opt_z_cap_(countvar)
#define _Out_z_bytecap_(countvar)
#define _Out_z_bytecap_c_(_const_expr)
#define _Out_z_cap_(countvar)

#define _Printf_format_string_

#define _Ret_valid_
#define _Ret_notnull_
#define _Ret_maybenull_

#define _Ret_
#define _Ret_bytecount_(size)
#define _Ret_bytecap_(size)
#define _Ret_count_(size)
#define _Ret_opt_
#define _Ret_opt_bytecount_(size)
#define _Ret_opt_bytecap_(size)
#define _Ret_opt_count_(size)
#define _Ret_opt_z_
#define _Ret_z_

#endif /* _Success_ */

/* end of no-sal.h */
