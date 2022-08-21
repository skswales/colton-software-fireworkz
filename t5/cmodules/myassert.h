/* myassert.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* NB Designed for multiple inclusion */

#if RISCOS && defined(CHECKING_PEDANTIC)
/* RISC OS can check parameters for matching format #ifdef CHECKING_PEDANTIC */
#pragma check_printf_formats
#endif

_Check_return_
extern BOOL __cdecl
__myasserted(
    _In_z_      PCTSTR tstr_function,
    _In_z_      PCTSTR tstr_file,
    _InVal_     U32 line_no,
    _In_z_ _Printf_format_string_ PCTSTR format,
    /**/        ...);

_Check_return_
extern BOOL __cdecl
__myasserted_msg(
    _In_z_      PCTSTR tstr_function,
    _In_z_      PCTSTR tstr_file,
    _InVal_     U32 line_no,
    _In_z_      PCTSTR message,
    _In_z_ _Printf_format_string_ PCTSTR format,
    /**/        ...);

#if RISCOS && defined(CHECKING_PEDANTIC)
#pragma no_check_printf_formats
#endif

_Check_return_
extern BOOL
__myasserted_EQ(
    _In_z_      PCTSTR tstr_function,
    _In_z_      PCTSTR tstr_file,
    _InVal_     U32 line_no,
    _InVal_     U32 val1,
    _InVal_     U32 val2);

_Check_return_
extern BOOL
__vmyasserted(
    _In_z_      PCTSTR tstr_function,
    _In_z_      PCTSTR tstr_file,
    _InVal_     U32 line_no,
    _In_opt_z_  PCTSTR message,
    _In_z_ _Printf_format_string_ PCTSTR format,
    /**/        va_list args);

_Check_return_
extern BOOL
__bool_assert(
    _InVal_     BOOL bool_val,
    _In_z_      PCTSTR tstr_function,
    _In_z_      PCTSTR tstr_file,
    _InVal_     U32 line_no,
    _In_z_      PCTSTR str);

extern void
__hard_assert(
    _InVal_     BOOL hard_mode);

_Check_return_
extern STATUS
__status_assert(
    _InVal_     STATUS status,
    _In_z_      PCTSTR tstr_function,
    _In_z_      PCTSTR tstr_file,
    _InVal_     U32 line_no,
    _In_z_      PCTSTR str);

#if RISCOS

/* __WrapOsErrorChecking() & WrapOsErrorReporting() are defined in xp_skelr.h */

#elif WINDOWS

_Check_return_
extern BOOL
__WrapOsBoolChecking(
    _InVal_     BOOL res,
    _In_z_      PCTSTR tstr_function,
    _In_z_      PCTSTR tstr_file,
    _In_        int line_no,
    _In_z_      PCTSTR str);

/*ncr*/
extern BOOL
WrapOsBoolReporting(
    _InVal_     BOOL res);

#endif /* OS */

#ifndef myDebugBreak
#define myDebugBreak() /*EMPTY*/
#if CHECKING
#undef myDebugBreak
#if WINDOWS
#if defined(_X86_) || (defined(_AMD64_) && !defined(_WIN64))
#define myDebugBreak()  __asm { int 3 } /* break at this level when possible for ease of debugging rather than in kernel32.dll */
#else
#define myDebugBreak()  DebugBreak()
#endif /* _X86_ || _AMD64_ */
#else /* !WINDOWS */
#define myDebugBreak() * BAD_POINTER_X(P_U32, 0) = (U32) BAD_POINTER_X(P_U32, 0)
#endif /* OS */
#endif /* CHECKING */
#endif /* myDebugBreak */

#ifndef __crash_and_burn_here
#define __crash_and_burn_here() myDebugBreak()
#endif

#ifdef myassert0
#undef myassert0
#endif

#ifdef myassert1
#undef myassert1
#endif

#ifdef myassert2
#undef myassert2
#endif

#ifdef myassert3
#undef myassert3
#endif

#ifdef myassert4
#undef myassert4
#endif

#ifdef myassert5
#undef myassert5
#endif

#ifdef myassert6
#undef myassert6
#endif

#ifdef myassert7
#undef myassert7
#endif

#ifdef myassert8
#undef myassert8
#endif

#ifdef myassert9
#undef myassert9
#endif

#ifdef myassert10
#undef myassert10
#endif

#ifdef myassert11
#undef myassert11
#endif

#ifdef myassert12
#undef myassert12
#endif

#ifdef myassert13
#undef myassert13
#endif

#ifdef myassert0x
#undef myassert0x
#endif

#ifdef myassert1x
#undef myassert1x
#endif

#ifdef myassert2x
#undef myassert2x
#endif

#ifdef myassert3x
#undef myassert3x
#endif

#ifdef myassert4x
#undef myassert4x
#endif

#ifdef assert_EQ
#undef assert_EQ
#endif

#ifdef bool_assert
#undef bool_assert
#endif

#ifdef hard_assert
#undef hard_assert
#endif

#ifdef status_wrap
#undef status_wrap
#endif

#ifdef status_assert
#undef status_assert
#endif

#ifdef status_assertc
#undef status_assertc
#endif

#ifdef WrapOsBoolChecking
#undef WrapOsBoolChecking
#endif

#ifdef void_WrapOsBoolChecking
#undef void_WrapOsBoolChecking
#endif

#ifdef WrapOsErrorChecking
#undef WrapOsErrorChecking
#endif

#ifdef void_WrapOsErrorChecking
#undef void_WrapOsErrorChecking
#endif

#if CHECKING

#define myassert0(str) do { \
    if(__myasserted(__Tfunc__, __TFILE__, __LINE__, TEXT("%s"), (str))) \
        __crash_and_burn_here(); } while_constant(0)
#define myassert1(format, arg1) do { \
    if(__myasserted(__Tfunc__, __TFILE__, __LINE__, (format), arg1)) \
        __crash_and_burn_here(); } while_constant(0)
#define myassert2(format, arg1, arg2) do { \
    if(__myasserted(__Tfunc__, __TFILE__, __LINE__, (format), arg1, arg2)) \
        __crash_and_burn_here(); } while_constant(0)
#define myassert3(format, arg1, arg2, arg3) do { \
    if(__myasserted(__Tfunc__, __TFILE__, __LINE__, (format), arg1, arg2, arg3)) \
        __crash_and_burn_here(); } while_constant(0)
#define myassert4(format, arg1, arg2, arg3, arg4) do { \
    if(__myasserted(__Tfunc__, __TFILE__, __LINE__, (format), arg1, arg2, arg3, arg4)) \
        __crash_and_burn_here(); } while_constant(0)
#define myassert5(format, arg1, arg2, arg3, arg4, arg5) do { \
    if(__myasserted(__Tfunc__, __TFILE__, __LINE__, (format), arg1, arg2, arg3, arg4, arg5)) \
        __crash_and_burn_here(); } while_constant(0)
#define myassert6(format, arg1, arg2, arg3, arg4, arg5, arg6) do { \
    if(__myasserted(__Tfunc__, __TFILE__, __LINE__, (format), arg1, arg2, arg3, arg4, arg5, arg6)) \
        __crash_and_burn_here(); } while_constant(0)
#define myassert7(format, arg1, arg2, arg3, arg4, arg5, arg6, arg7) do { \
    if(__myasserted(__Tfunc__, __TFILE__, __LINE__, (format), arg1, arg2, arg3, arg4, arg5, arg6, arg7)) \
        __crash_and_burn_here(); } while_constant(0)
#define myassert8(format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8) do { \
    if(__myasserted(__Tfunc__, __TFILE__, __LINE__, (format), arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8)) \
        __crash_and_burn_here(); } while_constant(0)
#define myassert9(format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9) do { \
    if(__myasserted(__Tfunc__, __TFILE__, __LINE__, (format), arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9)) \
        __crash_and_burn_here(); } while_constant(0)
#define myassert10(format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10) do { \
    if(__myasserted(__Tfunc__, __TFILE__, __LINE__, (format), arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10)) \
        __crash_and_burn_here(); } while_constant(0)
#define myassert11(format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11) do { \
    if(__myasserted(__Tfunc__, __TFILE__, __LINE__, (format), arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11)) \
        __crash_and_burn_here(); } while_constant(0)
#define myassert12(format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12) do { \
    if(__myasserted(__Tfunc__, __TFILE__, __LINE__, (format), arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12)) \
        __crash_and_burn_here(); } while_constant(0)
#define myassert13(format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13) do { \
    if(__myasserted(__Tfunc__, __TFILE__, __LINE__, (format), arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13)) \
        __crash_and_burn_here(); } while_constant(0)

#define myassert0x(exp, str) do { \
    if_constant(!(exp)) \
        myassert0(      str); \
    _Analysis_assume_(exp); } while_constant(0)
#define myassert1x(exp, format, arg1) do { \
    if_constant(!(exp)) \
        myassert1(      format, arg1); \
    _Analysis_assume_(exp); } while_constant(0)
#define myassert2x(exp, format, arg1, arg2) do { \
    if_constant(!(exp)) \
        myassert2(      format, arg1, arg2); \
    _Analysis_assume_(exp); } while_constant(0)
#define myassert3x(exp, format, arg1, arg2, arg3) do { \
    if_constant(!(exp)) \
        myassert3(      format, arg1, arg2, arg3); \
    _Analysis_assume_(exp); } while_constant(0)
#define myassert4x(exp, format, arg1, arg2, arg3, arg4) do { \
    if_constant(!(exp)) \
        myassert4(      format, arg1, arg2, arg3, arg4); \
    _Analysis_assume_(exp); } while_constant(0)

#define assert_EQ(exp1, exp2) do { \
    if(__myasserted_EQ(__Tfunc__, __TFILE__, __LINE__, (U32) (exp1), (U32) (exp2))) \
        __crash_and_burn_here(); } while_constant(0)

#ifndef _bool_assert_function_declared
#define _bool_assert_function_declared 1
_Check_return_
static inline BOOL
_bool_assert(
    _InVal_     BOOL bool_val,
    _In_z_      PCTSTR tstr_function,
    _In_z_      PCTSTR tstr_file,
    _In_        int line_no,
    _In_z_      PCTSTR tstr)
{
    if(bool_val)
        return(bool_val);

    return(__bool_assert(bool_val, tstr_function, tstr_file, line_no, tstr));
}
#endif /* _bool_assert_function_declared */

#define bool_assert(bool_expr) \
    consume_bool(_bool_assert(bool_expr, __Tfunc__, __TFILE__, __LINE__, TEXT(#bool_expr)))

#define hard_assert(hard_mode) \
    __hard_assert(hard_mode)

#ifndef _status_assert_function_declared
#define _status_assert_function_declared 1
_Check_return_
static inline STATUS
_status_assert(
    _InVal_     STATUS status,
    _In_z_      PCTSTR tstr_function,
    _In_z_      PCTSTR tstr_file,
    _In_        int line_no,
    _In_z_      PCTSTR tstr)
{
    if(status_ok(status) || (STATUS_CANCEL == status))
        return(status);

    return(__status_assert(status, tstr_function, tstr_file, line_no, tstr));
}
#endif /* _status_assert_function_declared */

#define status_wrap(status_expr) \
    _status_assert(status_expr, __Tfunc__, __TFILE__, __LINE__, TEXT(#status_expr))

#define status_assert(status_expr) \
    status_consume(_status_assert(status_expr, __Tfunc__, __TFILE__, __LINE__, TEXT(#status_expr)))

#define status_assertc(status_expr) \
    status_assert(status_expr)

#if RISCOS

#define WrapOsErrorChecking(f) \
    __WrapOsErrorChecking(f, __Tfunc__, __TFILE__, __LINE__, TEXT(#f))

#define void_WrapOsErrorChecking(f) \
    consume(_kernel_oserror *, __WrapOsErrorChecking(f, __Tfunc__, __TFILE__, __LINE__, TEXT(#f)))

#elif WINDOWS && !defined(CODE_ANALYSIS)

#define WrapOsBoolChecking(f) \
    __WrapOsBoolChecking(f, __Tfunc__, __TFILE__, __LINE__, TEXT(#f))

#define void_WrapOsBoolChecking(f) \
    consume_bool(__WrapOsBoolChecking(f, __Tfunc__, __TFILE__, __LINE__, TEXT(#f)))

#endif /* OS */

#else

#define myassert0(format)
#define myassert1(format, arg1)
#define myassert2(format, arg1, arg2)
#define myassert3(format, arg1, arg2, arg3)
#define myassert4(format, arg1, arg2, arg3, arg4)
#define myassert5(format, arg1, arg2, arg3, arg4, arg5)
#define myassert6(format, arg1, arg2, arg3, arg4, arg5, arg6)
#define myassert7(format, arg1, arg2, arg3, arg4, arg5, arg6, arg7)
#define myassert8(format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8)
#define myassert9(format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9)
#define myassert10(format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10)
#define myassert11(format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11)
#define myassert12(format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12)
#define myassert13(format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13)

#define myassert0x(exp, format) _Analysis_assume_(exp)
#define myassert1x(exp, format, arg1) _Analysis_assume_(exp)
#define myassert2x(exp, format, arg1, arg2) _Analysis_assume_(exp)
#define myassert3x(exp, format, arg1, arg2, arg3) _Analysis_assume_(exp)
#define myassert4x(exp, format, arg1, arg2, arg3, arg4) _Analysis_assume_(exp)

#define assert_EQ(exp1, exp2)

#define bool_assert(bool_expr)      consume_bool(bool_expr)

#define hard_assert(hard_mode)

#define status_wrap(status_expr)    (status_expr)
#define status_assert(status_expr)  status_consume(status_expr)
#define status_assertc(status_expr) /* rien - don't evaluate arg */

#endif /* CHECKING */

#if RISCOS

#ifndef WrapOsErrorChecking
#define WrapOsErrorChecking(f) (f)
#define void_WrapOsErrorChecking(f) (void) (f)
#endif

#elif WINDOWS

#ifndef WrapOsBoolChecking
#define WrapOsBoolChecking(f) (f)
#define void_WrapOsBoolChecking(f) (void) (f)
#endif

#endif /* OS */

/*
override normal C assert() usage
*/

#ifdef assert
#undef assert
#endif

#define assert(expr) \
    myassert0x(expr, TEXT(#expr))

#ifndef assert0
#define assert0() \
    myassert0(TEXT("assert(FALSE)"))
#endif

#ifndef default_unhandled
#define default_unhandled() \
    myassert0(TEXT("Unhandled value in switch")) /*then often...*/ /*FALLTHRU*/
#endif

/* end of myassert.h */
