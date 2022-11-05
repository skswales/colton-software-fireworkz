/* xustring.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1990-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Library module for UCHARS-based string handling */

/* SKS Apr 2014 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#ifndef          __xustring_h
#include "cmodules/xustring.h"
#endif

/******************************************************************************
*
* append to an aligator UCHARS-based string
*
******************************************************************************/

_Check_return_
extern STATUS
al_ustr_append(
    _InoutRef_  P_ARRAY_HANDLE p_array_handle_ustr,
    _In_z_      PC_USTR ustr)
{
    U32 uchars_n = ustrlen32p1(ustr); /*CH_NULL*/
    U32 nullch_char = 0;
    P_U8 p_u8;
    STATUS status;

#if CHECKING
    /* Watch out for inlines in USTRs! */
    if(contains_inline(ustr, uchars_n - 1))
    {
        assert0(); 
        /* "<<al_ustr_append - CONTAINS INLINES>>" */
        uchars_n = ustr_inline_strlen32p1((PC_USTR_INLINE) ustr); /*CH_NULL*/
    }
#endif

    if(0 != array_elements32(p_array_handle_ustr))
        if(CH_NULL == *array_ptrc(p_array_handle_ustr, U8, array_elements32(p_array_handle_ustr) - 1))
            nullch_char = 1;

    if(NULL == (p_u8 = al_array_extend_by_U8(p_array_handle_ustr, uchars_n - nullch_char, &array_init_block_uchars, &status)))
        return(status);

    p_u8 -= nullch_char; /* retract pointer over the CH_NULL iff present */

    memcpy32(p_u8, ustr, uchars_n);

    return(STATUS_OK);
}

/******************************************************************************
*
* do an aligator UCHARS-based string assignment
*
******************************************************************************/

_Check_return_
extern STATUS
al_ustr_set(
    _OutRef_    P_ARRAY_HANDLE_USTR p_array_handle_ustr,
    _In_z_      PC_USTR ustr)
{
    U32 uchars_n = ustrlen32p1(ustr); /*CH_NULL*/

    *p_array_handle_ustr = 0;

#if CHECKING
    /* Watch out for inlines in USTRs! */
    if(contains_inline(ustr, uchars_n - 1))
    {
        assert0(); 
        /* "<<al_ustr_append - CONTAINS INLINES>>" */
        uchars_n = ustr_inline_strlen32p1((PC_USTR_INLINE) ustr); /*CH_NULL*/
    }
#endif

    return(al_array_add(p_array_handle_ustr, _UCHARS, uchars_n, &array_init_block_uchars, ustr));
}

/******************************************************************************
*
* do a UCHARS-based string assignment
*
******************************************************************************/

_Check_return_
extern STATUS
ustr_set(
    _OutRef_    P_P_USTR aa,
    _In_opt_z_  PC_USTR ustr)
{
    STATUS status;
    P_USTR ustr_wr;
    U32 uchars_n;

    if(NULL == ustr)
    {
        *aa = NULL;
        return(STATUS_OK);
    }

    uchars_n = ustrlen32p1(ustr); /*CH_NULL*/

#if CHECKING
    /* Watch out for inlines in USTRs! */
    if(contains_inline(ustr, uchars_n - 1))
    {
        assert0(); 
        /* "<<ustr_set - CONTAINS INLINES>>" */
        uchars_n = ustr_inline_strlen32p1((PC_USTR_INLINE) ustr);
    }
#endif

    if(NULL == (*aa = ustr_wr = al_ptr_alloc_bytes(P_USTR, uchars_n, &status)))
        return(status);

    memcpy32(ustr_wr, ustr, uchars_n);

    return(STATUS_DONE);
}

_Check_return_
extern STATUS
ustr_set_n(
    _OutRef_    P_P_USTR aa,
    _In_reads_opt_(uchars_n) PC_UCHARS uchars,
    _InVal_     U32 uchars_n)
{
    STATUS status;
    P_USTR ustr_wr;

    if(0 == uchars_n)
    {
        *aa = NULL;
        return(STATUS_OK);
    }

#if CHECKING
    /* Watch out for inlines in USTRs! */
    if((NULL != uchars) && contains_inline(uchars, uchars_n))
    {
        assert0(); 
        /* "<<ustr_set_n - CONTAINS INLINES>>" */
    }
#endif

    if(NULL == (*aa = ustr_wr = al_ptr_alloc_bytes(P_USTR, uchars_n + 1 /*CH_NULL*/, &status)))
        return(status);

    if(NULL == uchars)
    {   /* NULL == uchars allows client to allocate for a string of uchars_n bytes (and the CH_NULL) */
        PtrPutByte(ustr_wr, CH_NULL); /* allows append e.g. ustr_xstrkat() */
        PtrPutByteOff(ustr_wr, uchars_n, CH_NULL); /* in case client forgets it */
    }
    else
    {
        memcpy32(ustr_wr, uchars, uchars_n);
        PtrPutByteOff(ustr_wr, uchars_n, CH_NULL);
        assert(uchars_n <= ustrlen32(ustr_wr));
    }

    return(STATUS_DONE);
}

/******************************************************************************
*
* convert column string into column number
*
* --out--
* number of bytes scanned
*
******************************************************************************/

_Check_return_
extern S32
stox(
    _In_z_      PC_USTR ustr,
    _OutRef_    P_S32 p_col)
{
    S32 n_scanned = 0;
    S32 col = 0;
    int c;

    profile_ensure_frame();

    c = PtrGetByte(ustr); ustr_IncByte(ustr);

    if((c >= 'A') && (c <= 'Z'))
        col = (c - 'A');
    else if((c >= 'a') && (c <= 'z'))
        col = (c - 'a');
    else
    {
        *p_col = 0;
        return(0);
    }

    n_scanned = 1;

    for(;;)
    {
        c = PtrGetByte(ustr); ustr_IncByte(ustr);

        if((c >= 'A') && (c <= 'Z'))
            c -= 'A';
        else if((c >= 'a') && (c <= 'z'))
            c -= 'a';
        else
            break;

        if(col > ((S32_MAX / 26) - 1))
            break; /* would overflow on increasing one place */
        col += 1;
        col *= 26;

        if(col > (S32_MAX - (S32) c))
            break; /* would overflow on adding in this component */
        col += c;

        ++n_scanned;
    }

    *p_col = col;

    return(n_scanned);
}

/******************************************************************************
*
* convert column to a string
*
* --out--
* length of resulting string
*
******************************************************************************/

static const S32
nth_letter[] =
{
    (S32)26,                         /* need this entry as loop looks at nlp-1 */

    (S32)26 +
        (S32)26*26,

    (S32)26 +
        (S32)26*26 +
            (S32)26*26*26,

    (S32)26 +
        (S32)26*26 +
            (S32)26*26*26 +
                (S32)26*26*26*26,

    (S32)26 +
        (S32)26*26 +
            (S32)26*26*26 +
                (S32)26*26*26*26 +
                    (S32)26*26*26*26*26,

    (S32)26 +
        (S32)26*26 +
            (S32)26*26*26 +
                (S32)26*26*26*26 +
                    (S32)26*26*26*26*26 +
                        (S32)26*26*26*26*26*26
};

static const S32
nth_power[] =
{
    (S32)26,                        /*  5 bits */ /* dummy entry - never used but keep for indexing consistency */
    (S32)26*26,                     /* 10 bits */
    (S32)26*26*26,                  /* 15 bits */
    (S32)26*26*26*26,               /* 19 bits */
    (S32)26*26*26*26*26,            /* 24 bits */
    (S32)26*26*26*26*26*26          /* 29 bits */
};

/*ncr*/
extern U32 /*number of bytes in converted buffer*/
xtos_ustr_buf(
    _Out_writes_z_(elemof_buffer) P_USTR ustr_buf,
    _InVal_     U32 elemof_buffer,
    _InVal_     S32 x,
    _InVal_     BOOL upper_case)
{
    U32 outidx = 0;
    BOOL force;
    PC_S32 nlp;
    PC_S32 npp;
    S32 col, nl, np;
    S32 digit[1 + 1 + elemof32(nth_power)];
    U32 digit_n = 0;
    U32 i;

    profile_ensure_frame();

    assert(elemof_buffer >= 2); /* at least one digit and terminator please! */

    col = x;

    if(col < 0)
    {
        col = 0 - col;

        PtrPutByteOff(ustr_buf, outidx++, CH_MINUS_SIGN__BASIC);
    }

    if(col >= 26 /*nth_letter[0]*/)
    {
        if(col >= nth_letter[1])
        {
            force = FALSE;
            nlp = nth_letter + elemof32(nth_letter) - 1;
            npp = nth_power  + elemof32(nth_power)  - 1;

            do  {
                if(force || (col >= *nlp))
                {
                    nl = *(nlp - 1);
                    np = *npp;
                    col -= nl;
                    digit[digit_n++] = col / np - 1;
                    col = col % np + nl;
                    force = TRUE;
                }

                --nlp;
                --npp;
            }
            while(nlp > nth_letter);        /* don't ever loop with nth_letter[0] */
        }

        /* nl == 0 */
        digit[digit_n++] = col / 26 - 1;
        col = col % 26;
    }

    digit[digit_n++] = col;

    /* make the string */
    for(i = 0; (i < elemof_buffer-1) && (i < digit_n); ++i)
    {
        PtrPutByteOff(ustr_buf, outidx++, (char) (digit[i] + (upper_case ? 'A' : 'a')));
    }

    PtrPutByteOff(ustr_buf, outidx, CH_NULL);

    return(outidx);
}

#if USTR_IS_SBSTR

/******************************************************************************
*
* case insensitive lexical comparison of leading chars of two counted UCHAR sequences
*
* NB uses t5_sortbyte() for collation
*
******************************************************************************/

_Check_return_
extern int /* 0:EQ, +ve:arg1>arg2, -ve:arg1<arg2 */
uchars_compare_t5_nocase(
    _In_reads_(uchars_n_a) PC_UCHARS uchars_a,
    _InVal_     U32 uchars_n_a,
    _In_reads_(uchars_n_b) PC_UCHARS uchars_b,
    _InVal_     U32 uchars_n_b)
{
    int res;
    PC_U8 str_a = (PC_U8) uchars_a;
    PC_U8 str_b = (PC_U8) uchars_b;
    U32 limit = MIN(uchars_n_a, uchars_n_b);
    U32 i;
    U32 remain_a, remain_b;

    profile_ensure_frame();

    assert(strlen_without_NULLCH > uchars_n_a); /* we will read to the bitter end - these are not valid */
    assert(strlen_without_NULLCH > uchars_n_b);

    for(i = 0; i < limit; ++i)
    {
        int c_a = *str_a++;
        int c_b = *str_b++;

        res = c_a - c_b;

        if(0 != res)
        {   /* retry with case folding */
            c_a = t5_sortbyte(c_a);
            c_b = t5_sortbyte(c_b);

            res = c_a - c_b;

            if(0 != res)
                return(res);
        }
    }

    /* matched up to the comparison limit */

    /* which counted sequence has the greater number of chars left over? */
    remain_a = uchars_n_a - limit;
    remain_b = uchars_n_b - limit;

    /*if(remain_a == remain_b)
        return(0);*/ /* ended together at the specified finite lengths -> equal */

    res = (int) remain_a - (int) remain_b;

    return(res);
}

/******************************************************************************
*
* this routine is a quick hack from PD3 stringcmp
* uses wild ^? single and ^# multiple, ^^ == ^
* needs rewriting long-term for generality!
*
* leading and trailing spaces are insignificant
*
* NB uses t5_sortbyte() for collation
*
******************************************************************************/

_Check_return_
extern int /* 0:EQ, +ve:arg1>arg2, -ve:arg1<arg2 */
uchars_compare_t5_nocase_wild(
    _In_reads_(uchars_n_a) PC_UCHARS uchars_a,
    _InVal_     U32 uchars_n_a,
    _In_reads_(uchars_n_b) PC_UCHARS uchars_b,
    _InVal_     U32 uchars_n_b)
{
    PC_U8 ptr1 = (PC_U8) uchars_a;
    PC_U8 ptr2 = (PC_U8) uchars_b;
    PC_U8 end_y = ptr1 + uchars_n_a;
    PC_U8 end_x = ptr2 + uchars_n_b;
    PC_U8 x;
    PC_U8 y;
    PC_U8 ox;
    PC_U8 oy;
    int sb_x, sb_y;
    U8 ch;
    BOOL wild_x;
    int pos_res;

    profile_ensure_frame();

    assert(strlen_without_NULLCH > uchars_n_a); /* we will read to the bitter end - these are not valid */
    assert(strlen_without_NULLCH > uchars_n_b);

    /* skip leading and trailing spaces */
    while((ptr1 < end_y) && (CH_SPACE == PtrGetByte(ptr1)))
        PtrIncBytes(PC_U8, ptr1, 1);

    while((end_y > ptr1) && (CH_SPACE == PtrGetByteOff(end_y, -1)))
        PtrDecBytes(PC_U8, end_y, 1);

    y = ptr1;

    /* skip leading and trailing spaces in template string */
    while((ptr2 < end_x) && (CH_SPACE == PtrGetByte(ptr2)))
        PtrIncBytes(PC_U8, ptr2, 1);

    while((end_x > ptr2) && (CH_SPACE == PtrGetByteOff(end_x, -1)))
        PtrDecBytes(PC_U8, end_x, 1);

    /* must skip leading hilites in template string for final rejection */
    x = ptr2 - 1;

STAR:
    /* skip a char in template string */
    ch = *++x;
    /*trace_1(0, TEXT("uchars_compare_nocase_wild STAR (x skipped): x -> '%s'"), x);*/

    wild_x = (ch == CH_CIRCUMFLEX_ACCENT);
    if(wild_x)
        ++x;

    oy = y;

    /* loop1: */
    for(;;)
    {
        /* skip a char in second string */
        ch = *++oy;
        /*trace_1(0, TEXT("uchars_compare_nocase_wild loop1 (oy skipped): oy -> '%s'"), oy);*/

        ox = x;

        /* loop3: */
        for(;;)
        {
            if(wild_x)
                switch(*x)
                {
                case CH_NUMBER_SIGN:
                    /*trace_0(0, TEXT("uchars_compare_nocase_wild loop3: ^# found in template string: goto STAR to skip it & hilites"));*/
                    goto STAR;

                case CH_CIRCUMFLEX_ACCENT:
                    /*trace_0(0, TEXT("uchars_compare_nocase_wild loop3: ^^ found in template string: match as ^"));*/
                    wild_x = FALSE;

                default:
                    break;
                }

            /*trace_3(0, TEXT("uchars_compare_nocase_wild loop3: x -> '%s', y -> '%s', wild_x %s"), x, y, report_boolstring(wild_x));*/

            /* are we at end of y string? */
            if(y == end_y)
            {
                /*trace_1(0, TEXT("uchars_compare_nocase_wild: end of y string: returns ") S32_TFMT, (*x == CH_NULL) ? 0 : -1);*/
                if(x == end_x)
                    return(0);       /* equal */
                else
                    return(-1);      /* first is bigger */
            }

            /* see if characters at x and y match */
            sb_x = (int) t5_sortbyte(*x);
            sb_y = (int) t5_sortbyte(*y);
            pos_res = sb_y - sb_x;

            if(0 != pos_res)
            {
                /* single character wildcard at x? */
                if(!wild_x  ||  (*x != CH_QUOTATION_MARK)  ||  (*y == CH_SPACE))
                {
                    y = oy;
                    x = ox;

                    if(x == ptr2)
                    {
                        /*trace_1(0, TEXT("uchars_compare_nocase_wild: returns ") S32_TFMT, pos_res);*/
                        return(pos_res);
                    }

                    /*trace_0(0, TEXT("uchars_compare_nocase_wild: chars differ: restore ptrs & break to loop1"));*/
                    break;
                }
            }

            /* characters at x and y match, so increment x and y */
            /*trace_0(0, TEXT("uchars_compare_nocase_wild: chars at x & y match: ++x, ++y & keep in loop3"));*/
            ch = *++x;

            wild_x = (ch == CH_CIRCUMFLEX_ACCENT);
            if(wild_x)
                ++x;

            ch = *++y;
        }
    }
}

#endif /* USTR_IS_SBSTR */

/*
conversion routines
*/

/*ncr*/
extern U32
sbstr_buf_from_ustr(
    _Out_writes_z_(elemof_buffer) P_SBSTR buffer,
    _InVal_     U32 elemof_buffer,
    _InVal_     SBCHAR_CODEPAGE sbchar_codepage,
    _In_z_      PC_USTR ustr,
    _InVal_     U32 uchars_n /*strlen_with,without_NULLCH*/)
{
#if USTR_IS_SBSTR
    UNREFERENCED_PARAMETER_InVal_(sbchar_codepage);
    /* no conversion needed, don't waste any more time/space */
    return(xstrnkpy(buffer, elemof_buffer, ustr, ustrlen32_n(ustr, uchars_n)));
#else
    return(sbstr_buf_from_utf8str(buffer, elemof_buffer, sbchar_codepage, ustr, uchars_n));
#endif /* USTR_IS_SBSTR */
}

/*
low-lifetime conversion routines
*/

#if (TSTR_IS_SBSTR && USTR_IS_SBSTR) && (CHECKING && 0)

_Check_return_
_Ret_z_ /* never NULL */
extern PCTSTR /*low-lifetime*/
_tstr_from_ustr(
    _In_z_      PC_USTR ustr)
{
    if(NULL == ustr)
    {
        assert0();
        return(TEXT("<<tstr_from_ustr - NULL>>"));
    }

    if(PTR_IS_NONE(ustr))
    {
        assert0();
        return(TEXT("<<tstr_from_ustr - NONE>>"));
    }

    if(contains_inline(ustr, ustrlen32(ustr)))
    {
        assert0();
        return(TEXT("<<tstr_from_ustr - CONTAINS INLINES>>"));
    }

    return(_tstr_from_sbstr((PC_SBSTR) ustr));
}

_Check_return_
_Ret_z_ /* never NULL */
extern PC_USTR /*low-lifetime*/
_ustr_from_tstr(
    _In_z_      PCTSTR tstr)
{
    if(NULL == tstr)
        return(("<<ustr_from_tstr - NULL>>"));

    if(PTR_IS_NONE(tstr))
        return(("<<ustr_from_tstr - NONE>>"));

    return((PC_USTR) _sbstr_from_tstr(tstr));
}

#else /* (TSTR_IS_SBSTR && USTR_IS_SBSTR) && (CHECKING) */

/* either no conversion is required or host-specific implementation */

#endif /* (TSTR_IS_SBSTR && USTR_IS_SBSTR) && (CHECKING) */

/* end of xustring.c */
