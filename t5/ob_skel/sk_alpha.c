/* sk_alpha.c */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Fireworkz' routines for character classification */

/* MRJC January 1, 1993 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#if RISCOS
#include "ob_skel/xp_skelr.h"
#endif

#if defined(HAS_REFERENCE_CTYPE)
#include "reference_ctype.h"
#endif

/* B0 = RISC OS __ctype byte */
/* B1 = upper-case-mapping(char) */
/* B2 = lower-case-mapping(char) */
/* B3 = sort order/compare byte - added 05mar96 (default is compare as lower case, lose accents etc) */

/*const-to-you*/ U32
t5__ctype_sbchar[256]; /* this table is set up first */

/*const-to-you*/ U32
t5__ctype[256]; /* and then it is copied here, which may get overrides applied */

static inline void
sk_alpha_ctype_sbchar_BIC_ORR(
    _InVal_     U32 i,
    _InVal_     U32 clear_bits,
    _InVal_     U32 set_bits)
{
    U32 u32;

    u32 = t5__ctype_sbchar[i];
    u32 = (u32 & ~clear_bits);
    u32 = u32 | set_bits;
    t5__ctype_sbchar[i] = u32;
}

/* start by mapping each character to itself, ignoring case and flags for now */

static void
sk_alpha_reset(
    _InVal_     U32 i)
{   /*              B3          B2          B1          B0 */
    U32 u32 = (i << 24) | (i << 16) | (i <<  8) | (0 <<  0);
    t5__ctype_sbchar[i] = u32;
}

static void
sk_alpha_reset_range(
    _InVal_     U32 stt,
    _InVal_     U32 end /*inclusive*/)
{
    U32 i;

    for(i = stt; i <= end; ++i)
        sk_alpha_reset(i);
}

static void
sk_alpha_set_flag(
    _InVal_     U32 i,
    _InVal_     U32 flag_bits)
{
    sk_alpha_ctype_sbchar_BIC_ORR(i, 0, flag_bits & 0xFFU);
}

static void
sk_alpha_set_as_lower(
    _InVal_     U32 i,
    _InVal_     U32 upper_byte)
{
    sk_alpha_ctype_sbchar_BIC_ORR(i, 0xFFU <<  8, (upper_byte <<  8 /*B1*/) | T5__L);
}

static void
sk_alpha_set_as_upper(
    _InVal_     U32 i,
    _InVal_     U32 lower_byte)
{
    sk_alpha_ctype_sbchar_BIC_ORR(i, 0xFFU << 16, (lower_byte << 16 /*B2*/) | T5__U);
}

static void
sk_alpha_set_sortbyte(
    _InVal_     U32 i,
    _InVal_     U32 sort_byte)
{
    sk_alpha_ctype_sbchar_BIC_ORR(i, 0xFFU << 24, sort_byte << 24 /*B3*/);
}

/*
0000..007F C0 Controls and Basic Latin (ASCII 7-bit characters)
*/

static void
sk_alpha_C0_Controls_and_Basic_Latin(void)
{
    U32 i;

    /* whitespace characters */
    for(i = 0x09 /*TAB*/; i <= 0x0D /*|M*/; ++i)
        sk_alpha_set_flag(i, T5__S);

    sk_alpha_set_flag(0x20 /*SPC*/, T5__S);

    /* punctuation characters */
    for(i = 0x21 /*!*/; i <= 0x2F /*'/'*/; ++i)
        sk_alpha_set_flag(i, T5__P);

    for(i = 0x3A /*:*/; i <= 0x40 /*@*/; ++i)
        sk_alpha_set_flag(i, T5__P);

    for(i = 0x5B /*[*/; i <= 0x60 /*`*/; ++i)
        sk_alpha_set_flag(i, T5__P);

    for(i = 0x7B /*{*/; i <= 0x7E /*~*/; ++i)
        sk_alpha_set_flag(i, T5__P);

    /* blank characters */
    sk_alpha_set_flag(0x20 /*SPC*/, T5__B);

    /* ASCII decimal digit characters */
    for(i = 0x30 /*0*/; i <= 0x39 /*9*/; ++i)
        sk_alpha_set_flag(i, T5__N);

    /* C0 control characters */
    for(i = 0x00; i <= 0x1F; ++i)
        sk_alpha_set_flag(i, T5__C);

    sk_alpha_set_flag(0x7F /*DEL*/, T5__C);

    /* ASCII hex digit characters */
    for(i = 0x41 /*A*/; i <= 0x46 /*F*/; ++i)
        sk_alpha_set_flag(i, T5__X);

    for(i = 0x61 /*a*/; i <= 0x66 /*f*/; ++i)
        sk_alpha_set_flag(i, T5__X);

    /* ASCII upper-case characters: trivial lower-case mapping */
    for(i = 0x41 /*A*/; i <= 0x5A /*Z*/; ++i)
        sk_alpha_set_as_upper(i, (i - (U32) 'A') + 'a');

    /* ASCII lower-case characters: trivial upper-case mapping */
    for(i = 0x61 /*a*/; i <= 0x7A /*z*/; ++i)
        sk_alpha_set_as_lower(i, (i - (U32) 'a') + 'A');

    /* trivial sort byte mapping for ASCII characters - map upper-case as lower-case */
    for(i = 0x41 /*A*/; i <= 0x5A /*Z*/; ++i)
        sk_alpha_set_sortbyte(i, (i - (U32) 'A') + 'a');
}

/*
0080..009F C1 Controls
*/

static void
sk_alpha_C1_Controls(void)
{
    U32 i;

    /* C1 control characters */
    for(i = 0x80; i <= 0x9F; ++i)
        sk_alpha_set_flag(i, T5__C);
}

/*
These characters are present at different code points in
different code pages but have the same attributes in each
*/

/*
single characters
*/

static void
sk_alpha_Euro_Currency_Sign(
    _InVal_     U32 i)
{
    sk_alpha_reset(i);
    sk_alpha_set_flag(i /*EURO CURRENCY SIGN*/, T5__P);
}

static void
sk_alpha_Latin_Capital_Letter_Y_With_Diaeresis(
    _InVal_     U32 i)
{
    sk_alpha_reset(i);
    sk_alpha_set_as_upper(i /*LATIN CAPITAL LETTER Y WITH DIAERESIS*/, 0xFF /*Latin Small Letter Y With Diaeresis*/);
    sk_alpha_set_sortbyte(i, 0x79 /*y*/);
}

/*
pairs of upper-case and lower-case characters that sort together
*/

static void
sk_alpha_upper_lower_sort(
    _InVal_     U32 i_capital,
    _InVal_     U32 i_small,
    _InVal_     U32 sort_byte)
{
    sk_alpha_reset(i_capital);
    sk_alpha_set_as_upper(i_capital, i_small);
    sk_alpha_set_sortbyte(i_capital, sort_byte);

    sk_alpha_reset(i_small);
    sk_alpha_set_as_lower(i_small, i_capital);
    sk_alpha_set_sortbyte(i_small, sort_byte);
}

#define sk_alpha_Latin_Letter_A_With_Diacritical_Mark(i_capital, i_small) \
    sk_alpha_upper_lower_sort(i_capital /*LATIN CAPITAL LETTER A WITH x*/, i_small /*LATIN SMALL LETTER A WITH x*/, 's')

#define sk_alpha_Latin_Letter_E_With_Diacritical_Mark(i_capital, i_small) \
    sk_alpha_upper_lower_sort(i_capital /*LATIN CAPITAL LETTER E WITH x*/, i_small /*LATIN SMALL LETTER E WITH x*/, 'e')

#define sk_alpha_Latin_Letter_G_With_Diacritical_Mark(i_capital, i_small) \
    sk_alpha_upper_lower_sort(i_capital /*LATIN CAPITAL LETTER G WITH x*/, i_small /*LATIN SMALL LETTER G WITH x*/, 'g')

#define sk_alpha_Latin_Letter_H_With_Diacritical_Mark(i_capital, i_small) \
    sk_alpha_upper_lower_sort(i_capital /*LATIN CAPITAL LETTER H WITH x*/, i_small /*LATIN SMALL LETTER H WITH x*/, 'h')

#define sk_alpha_Latin_Letter_I_With_Diacritical_Mark(i_capital, i_small) \
    sk_alpha_upper_lower_sort(i_capital /*LATIN CAPITAL LETTER I WITH x*/, i_small /*LATIN SMALL LETTER I WITH x*/, 'i')

#define sk_alpha_Latin_Letter_J_With_Diacritical_Mark(i_capital, i_small) \
    sk_alpha_upper_lower_sort(i_capital /*LATIN CAPITAL LETTER J WITH x*/, i_small /*LATIN SMALL LETTER J WITH x*/, 'j')

#define sk_alpha_Latin_Letter_L_With_Diacritical_Mark(i_capital, i_small) \
    sk_alpha_upper_lower_sort(i_capital /*LATIN CAPITAL LETTER L WITH x*/, i_small /*LATIN SMALL LETTER L WITH x*/, 'l')

#define sk_alpha_Latin_Letter_R_With_Diacritical_Mark(i_capital, i_small) \
    sk_alpha_upper_lower_sort(i_capital /*LATIN CAPITAL LETTER R WITH x*/, i_small /*LATIN SMALL LETTER R WITH x*/, 'r')

#define sk_alpha_Latin_Letter_S_With_Diacritical_Mark(i_capital, i_small) \
    sk_alpha_upper_lower_sort(i_capital /*LATIN CAPITAL LETTER S WITH x*/, i_small /*LATIN SMALL LETTER S WITH x*/, 's')

#define sk_alpha_Latin_Letter_T_With_Diacritical_Mark(i_capital, i_small) \
    sk_alpha_upper_lower_sort(i_capital /*LATIN CAPITAL LETTER T WITH x*/, i_small /*LATIN SMALL LETTER T WITH x*/, 't')

#define sk_alpha_Latin_Letter_Y_With_Diacritical_Mark(i_capital, i_small) \
    sk_alpha_upper_lower_sort(i_capital /*LATIN CAPITAL LETTER Y WITH x*/, i_small /*LATIN SMALL LETTER Y WITH x*/, 'y')

#define sk_alpha_Latin_Letter_Z_With_Diacritical_Mark(i_capital, i_small) \
    sk_alpha_upper_lower_sort(i_capital /*LATIN CAPITAL LETTER Z WITH x*/, i_small /*LATIN SMALL LETTER Z WITH x*/, 'z')

static void
sk_alpha_Latin_Ligature_OE(
    _InVal_     U32 i_capital,
    _InVal_     U32 i_small)
{
    sk_alpha_reset(i_capital);
    sk_alpha_set_as_upper(i_capital /*LATIN CAPITAL LIGATURE OE*/, i_small /*LATIN SMALL LIGATURE OE*/);
    sk_alpha_set_sortbyte(i_capital /*LATIN CAPITAL LIGATURE OE*/, i_small /*LATIN SMALL LIGATURE OE*/);

    sk_alpha_reset(i_small);
    sk_alpha_set_as_lower(i_small /*LATIN SMALL LIGATURE OE*/, i_capital /*LATIN CAPITAL LIGATURE OE*/);
}

/**********************
*
* ISO 8859-1 & variants
*
**********************/

/*
00A0..00FF ISO 8859-1 (Latin-1 Supplement)
This range is common to both Acorn Extended Latin-1 and Windows-1252.
*/

static void
sk_alpha_ISO_8859_1_A0_BF(void)
{
    U32 i;

    /* ISO 8859-1 whitespace characters */
    sk_alpha_set_flag(0xA0 /*NO-BREAK SPACE*/, T5__S);

    /* ISO 8859-1 punctuation characters */
    for(i = 0xA1 /*¡*/; i <= 0xBF /**/; ++i)
        sk_alpha_set_flag(i, T5__P);
}

static void
sk_alpha_ISO_8859_1_C0_FF(void)
{
    U32 i;

    /* ISO 8859-1 punctuation characters */
    sk_alpha_set_flag(0xD7 /*×*/, T5__P);
    sk_alpha_set_flag(0xF7 /*÷*/, T5__P);

    sk_alpha_set_flag(0xDF /*ß*/, T5__L);

    /* ISO 8859-1 upper-case characters: trivial lower-case mapping */
    for(i = 0xC0 /*À*/; i <= 0xDE /*Þ*/; ++i)
        if(i != 0xD7 /*×*/)
            sk_alpha_set_as_upper(i,        (i - (U32) 'A') + 'a');

    /* ISO 8859-1 lower-case characters: trivial upper-case mapping */
    for(i = 0xE0 /*à*/; i <= 0xFE /*þ*/; ++i)
        if(i != 0xF7 /*÷*/)
            sk_alpha_set_as_lower(i,        (i - (U32) 'a') + 'A');

    sk_alpha_set_as_lower(0xFF /*ÿ*/,       0x59 /*Y*/);

    /* ISO 8859-1 sort byte mapping - map upper-case as lower-case, without accents as appropriate */

    for(i = 0xC0 /*À*/; i <= 0xC6 /*Æ*/; ++i)
        sk_alpha_set_sortbyte(i,            0x61 /*a*/);

    sk_alpha_set_sortbyte(0xC7 /*Ç*/,       0x63 /*c*/);

    for(i = 0xC8 /*È*/; i <= 0xCB /*Ë*/; ++i)
        sk_alpha_set_sortbyte(i,            0x65 /*e*/);

    for(i = 0xCC /*Ì*/; i <= 0xCF /*Ï*/; ++i)
        sk_alpha_set_sortbyte(i,            0x69 /*i*/);

    sk_alpha_set_sortbyte(0xD0 /*Ð*/,       0x64 /*d*/);
    sk_alpha_set_sortbyte(0xD1 /*Ñ*/,       0x6E /*n*/);

    for(i = 0xD2 /*Ò*/; i <= 0xD6 /*Ö*/; ++i)
        sk_alpha_set_sortbyte(i,            0x6F /*o*/);

    /* omitting 0xD7 */ /*×*/

    sk_alpha_set_sortbyte(0xD8 /*Ø*/,       0x6F /*o*/);

    for(i = 0xD9 /*Ù*/; i <= 0xDC /*Ü*/; ++i)
        sk_alpha_set_sortbyte(i,            0x75 /*u*/);

    sk_alpha_set_sortbyte(0xDD /*Ý*/,       0x79 /*y*/);
    sk_alpha_set_sortbyte(0xDE /*Þ*/,       0xFE /*þ*/);
    sk_alpha_set_sortbyte(0xDF /*ß*/,       0x73 /*s*/);

    /* ISO 8859-1 sort byte mapping - map lower-case as lower-case, without accents as appropriate */

    for(i = 0xE0 /*à*/; i <= 0xE6 /*æ*/; ++i)
        sk_alpha_set_sortbyte(i,            0x61 /*a*/);

    sk_alpha_set_sortbyte(0xE7 /*ç*/,       0x63 /*c*/);

    for(i = 0xE8 /*è*/; i <= 0xEB /*ë*/; ++i)
        sk_alpha_set_sortbyte(i,            0x65 /*e*/);

    for(i = 0xEC /*ì*/; i <= 0xEF /*ï*/; ++i)
        sk_alpha_set_sortbyte(i,            0x69 /*i*/);

    sk_alpha_set_sortbyte(0xF0 /*ð*/,       0x64 /*d*/);
    sk_alpha_set_sortbyte(0xF1 /*ñ*/,       0x6E /*n*/);

    for(i = 0xF2 /*ò*/; i <= 0xF6 /*ö*/; ++i)
        sk_alpha_set_sortbyte(i,            0x6F /*o*/);

    /* omitting 0xF7 */ /*÷*/

    sk_alpha_set_sortbyte(0xF8 /*ø*/,       0x6F /*o*/);

    for(i = 0xF9 /*ù*/; i <= 0xFC /*ü*/; ++i)
        sk_alpha_set_sortbyte(i,            0x75 /*u*/);

    sk_alpha_set_sortbyte(0xFD /*ý*/,       0x79 /*y*/);
    sk_alpha_set_sortbyte(0xFF /*ÿ*/,       0x79 /*y*/);
}

/*
ISO 8859-1
*/

static void
sk_alpha_ISO_8859_1(void)
{
    sk_alpha_C0_Controls_and_Basic_Latin();

    sk_alpha_C1_Controls();

    sk_alpha_ISO_8859_1_A0_BF();

    sk_alpha_ISO_8859_1_C0_FF();
}

/*
Windows-28591
*/

#define sk_alpha_Windows_28591() \
    sk_alpha_ISO_8859_1()

#if WINDOWS

/*
Windows-1252 range C1 0x80..0x9F extras.
Names in CAPS are their Unicode code point equivalents.
*/

static void
sk_alpha_Windows_1252_C1(void)
{
    U32 i;

    /* Windows-1252 characters in C1: reset for reuse */
    sk_alpha_Euro_Currency_Sign(0x80);
    sk_alpha_reset_range(0x82U, 0x8CU);
    sk_alpha_reset(0x8E);
    sk_alpha_reset_range(0x91U, 0x9CU);
    sk_alpha_reset_range(0x9EU, 0x9FU);

    /* Windows-1252 punctuation characters */
    sk_alpha_set_flag(0x82, T5__P); /*SINGLE LOW-9 QUOTATION MARK*/

    /*DOUBLE LOW-9 QUOTATION MARK*/
    /*HORIZONTAL ELLIPSIS*/
    /*DAGGER*/
    /*DOUBLE DAGGER*/
    /*MODIFIER LETTER CIRCUMFLEX ACCENT*/
    /*PER MILLE SIGN*/
    for(i = 0x84; i <= 0x89; ++i)
        sk_alpha_set_flag(i, T5__P);

    sk_alpha_set_flag(0x8B, T5__P); /*SINGLE LEFT-POINTING ANGLE QUOTATION MARK*/

    /*LEFT SINGLE QUOTATION MARK*/
    /*RIGHT SINGLE QUOTATION MARK*/
    /*LEFT DOUBLE QUOTATION MARK*/
    /*RIGHT DOUBLE QUOTATION MARK*/
    /*BULLET*/
    /*EN DASH*/
    /*EM DASH*/
    /*SMALL TILDE*/
    /*TRADE MARK SIGN*/
    for(i = 0x91; i <= 0x99; ++i)
        sk_alpha_set_flag(i, T5__P);

    sk_alpha_set_flag(0x9B, T5__P); /*SINGLE RIGHT-POINTING ANGLE QUOTATION MARK*/

    sk_alpha_Latin_Ligature_OE(0x8C /*LATIN CAPITAL LIGATURE OE*/, 0x9C /*LATIN SMALL LIGATURE OE*/);

    sk_alpha_Latin_Letter_S_With_Diacritical_Mark(0x8A /*LATIN CAPITAL LETTER S WITH CARON*/, 0x9A /*LATIN SMALL LETTER S WITH CARON*/);

    sk_alpha_Latin_Letter_Z_With_Diacritical_Mark(0x8E /*LATIN CAPITAL LETTER Z WITH CARON*/, 0x9A /*LATIN SMALL LETTER Z WITH CARON*/);

    sk_alpha_Latin_Capital_Letter_Y_With_Diaeresis(0x9F);

    /* Windows-1252 lower-case characters: upper-case mapping */
    sk_alpha_set_as_lower(0x83 /*LATIN SMALL LETTER F WITH HOOK*/, 0x83 /*same*/);
    sk_alpha_set_sortbyte(0x83 /*LATIN SMALL LETTER F WITH HOOK*/, 0x66 /*f*/);
}

/*
Windows-1252
*/

static void
sk_alpha_Windows_1252(void)
{
    sk_alpha_ISO_8859_1();

    sk_alpha_Windows_1252_C1();
}

#endif /* WINDOWS */

#if RISCOS

/*
Acorn Extended Latin-N C1 range 0x80..0x9F extras.
Settable one (or two) at a time as their presence at these code points varies between Latin-N variants.
Names in CAPS are their Unicode code point equivalents.
*/

static void
sk_alpha_AEL_C1_80(void)
{
    sk_alpha_Euro_Currency_Sign(0x80);
}

static void
sk_alpha_AEL_C1_81_82(void)
{
    sk_alpha_upper_lower_sort(0x81 /*LATIN CAPITAL LETTER W WITH CIRCUMFLEX*/, 0x82 /*LATIN SMALL LETTER W WITH CIRCUMFLEX*/, 0x77 /*W*/);
}

static void
sk_alpha_AEL_C1_85_86(void)
{
    sk_alpha_Latin_Letter_Y_With_Diacritical_Mark(0x85 /*LATIN CAPITAL LETTER Y WITH CIRCUMFLEX*/, 0x86 /*LATIN SMALL LETTER Y WITH CIRCUMFLEX*/);
}

static void
sk_alpha_AEL_C1_8C_99(void)
{
    U32 i;

    sk_alpha_reset_range(0x8CU, 0x99U);

    for(i = 0x8C /*HORIZONTAL ELLIPSIS*/; i <= 0x99 /*MINUS SIGN*/; ++i)
        sk_alpha_set_flag(i, T5__P);
}

static void
sk_alpha_AEL_C1_9A_9B(void)
{
    sk_alpha_Latin_Ligature_OE(0x9A /*LATIN CAPITAL LIGATURE OE*/, 0x9B /*LATIN SMALL LIGATURE OE*/);
}

static void
sk_alpha_AEL_C1_9C_9D(void)
{
    sk_alpha_reset_range(0x9CU, 0x9DU);
    sk_alpha_set_flag(0x9C /*DAGGER*/, T5__P);
    sk_alpha_set_flag(0x9D /*DOUBLE DAGGER*/, T5__P);
}

static void
sk_alpha_AEL_C1_9E_9F(void)
{
    sk_alpha_reset_range(0x9EU, 0x9FU);
    sk_alpha_set_flag(0x9E /*LATIN SMALL LIGATURE FI*/, T5__L);
    sk_alpha_set_flag(0x9F /*LATIN SMALL LIGATURE FL*/, T5__L);
}

/*
Acorn Extended Latin-1
*/

static void
sk_alpha_Alphabet_Latin1_C1(void)
{
    /* Acorn Extended Latin-1 characters in C1 */
    sk_alpha_AEL_C1_80();
    sk_alpha_AEL_C1_81_82();
    /* 0x83..0x84 still absent */
    sk_alpha_AEL_C1_85_86();
    /* 0x87..0x8B still absent */
    sk_alpha_AEL_C1_8C_99();
    sk_alpha_AEL_C1_9A_9B();
    sk_alpha_AEL_C1_9C_9D();
    sk_alpha_AEL_C1_9E_9F();
}

static void
sk_alpha_Alphabet_Latin1(void)
{
    sk_alpha_ISO_8859_1();

    sk_alpha_Alphabet_Latin1_C1();
}

#endif /* RISCOS */

/**********************
*
* ISO 8859-2 & variants
*
**********************/

/*
ISO 8859-2 range 0xA0..0xBF.
*/

static void
sk_alpha_ISO_8859_2_A0_BF(void)
{
    sk_alpha_ISO_8859_1_A0_BF(); /* start with this and apply overrides */

    sk_alpha_Latin_Letter_A_With_Diacritical_Mark(0xA1 /*LATIN CAPITAL LETTER A WITH OGONEK*/, 0xB1 /*LATIN SMALL LETTER A WITH OGONEK*/);
    sk_alpha_Latin_Letter_L_With_Diacritical_Mark(0xA3 /*LATIN CAPITAL LETTER L WITH STROKE*/, 0xB3 /*LATIN SMALL LETTER L WITH STROKE*/);
    sk_alpha_Latin_Letter_L_With_Diacritical_Mark(0xA5 /*LATIN CAPITAL LETTER L WITH CARON*/, 0xB5 /*LATIN SMALL LETTER L WITH CARON*/);
    sk_alpha_Latin_Letter_S_With_Diacritical_Mark(0xA6 /*LATIN CAPITAL LETTER S WITH ACUTE*/, 0xB6 /*LATIN SMALL LETTER S WITH ACUTE*/);
    sk_alpha_Latin_Letter_S_With_Diacritical_Mark(0xA9 /*LATIN CAPITAL LETTER S WITH CARON*/, 0xB9 /*LATIN SMALL LETTER S WITH CARON*/);
    sk_alpha_Latin_Letter_S_With_Diacritical_Mark(0xAA /*LATIN CAPITAL LETTER S WITH CEDILLA*/, 0xBA /*LATIN SMALL LETTER S WITH CEDILLA*/);
    sk_alpha_Latin_Letter_T_With_Diacritical_Mark(0xAB /*LATIN CAPITAL LETTER T WITH CARON*/, 0xBB /*LATIN SMALL LETTER T WITH CARON*/);
    sk_alpha_Latin_Letter_Z_With_Diacritical_Mark(0xAC /*LATIN CAPITAL LETTER Z WITH ACUTE*/, 0xBC /*LATIN SMALL LETTER Z WITH ACUTE*/);
    sk_alpha_Latin_Letter_Z_With_Diacritical_Mark(0xAE /*LATIN CAPITAL LETTER Z WITH CARON*/, 0xBE /*LATIN SMALL LETTER Z WITH CARON*/);
    sk_alpha_Latin_Letter_Z_With_Diacritical_Mark(0xAF /*LATIN CAPITAL LETTER Z WITH DOT ABOVE*/, 0xBF /*LATIN SMALL LETTER Z WITH DOT ABOVE*/);
}

/*
ISO 8859-2 range 0xC0..0xFF.
*/

static void
sk_alpha_ISO_8859_2_C0_FF(void)
{
    U32 i;

    /* ISO 8859-2 punctuation characters */
    sk_alpha_set_flag(0xD7 /*×*/, T5__P);
    sk_alpha_set_flag(0xF7 /*÷*/, T5__P);

    sk_alpha_set_flag(0xFF /*DOT ABOVE*/, T5__P);

    sk_alpha_set_flag(0xDF /*ß*/, T5__L);

    /* ISO 8859-2 upper-case characters: trivial lower-case mapping */
    for(i = 0xC0; i <= 0xDE; ++i)
        if(i != 0xD7 /*×*/)
            sk_alpha_set_as_upper(i, (i - (U32) 'A') + 'a');

    /* ISO 8859-2 lower-case characters: trivial upper-case mapping */
    for(i = 0xE0; i <= 0xFE; ++i)
        if(i != 0xF7 /*÷*/)
            sk_alpha_set_as_lower(i, (i - (U32) 'a') + 'A');

    sk_alpha_Latin_Letter_R_With_Diacritical_Mark(0xC0 /*LATIN CAPITAL LETTER R WITH ACUTE*/, 0xE0 /*LATIN SMALL LETTER R WITH ACUTE*/);
    sk_alpha_Latin_Letter_L_With_Diacritical_Mark(0xC5 /*LATIN CAPITAL LETTER L WITH ACUTE*/, 0xE5 /*LATIN SMALL LETTER L WITH ACUTE*/);
    sk_alpha_Latin_Letter_R_With_Diacritical_Mark(0xD8 /*LATIN CAPITAL LETTER R WITH CARON*/, 0xF8 /*LATIN SMALL LETTER R WITH CARON*/);
    sk_alpha_Latin_Letter_Y_With_Diacritical_Mark(0xDD /*LATIN CAPITAL LETTER Y WITH ACUTE*/, 0xFD /*LATIN SMALL LETTER Y WITH ACUTE*/);
    sk_alpha_Latin_Letter_T_With_Diacritical_Mark(0xDE /*LATIN CAPITAL LETTER T WITH CEDILLA*/, 0xFE /*LATIN SMALL LETTER T WITH CEDILLA*/);

    /* ISO 8859-2 sort byte mapping - map upper-case as lower-case, without accents as appropriate */

    /* already 0xC0 */ /*LATIN CAPITAL LETTER R WITH ACUTE*/

    for(i = 0xC1; i <= 0xC4; ++i)
        sk_alpha_set_sortbyte(i, 0x61 /*a*/);

    /* already 0xC5 */ /*LATIN CAPITAL LETTER L WITH ACUTE*/

    for(i = 0xC6; i <= 0xC8; ++i)
        sk_alpha_set_sortbyte(i, 0x63 /*c*/);

    for(i = 0xC9; i <= 0xCC; ++i)
        sk_alpha_set_sortbyte(i, 0x65 /*e*/);

    for(i = 0xCD; i <= 0xCE; ++i)
        sk_alpha_set_sortbyte(i, 0x69 /*i*/);

    for(i = 0xCF; i <= 0xD0; ++i)
        sk_alpha_set_sortbyte(i, 0x64 /*d*/);

    for(i = 0xD1; i <= 0xD2; ++i)
        sk_alpha_set_sortbyte(i, 0x6E /*n*/);

    for(i = 0xD3; i <= 0xD6; ++i)
        sk_alpha_set_sortbyte(i, 0x6F /*o*/);

    /* omitting 0xD7 */ /*×*/

    /* already 0xD8 */ /*LATIN CAPITAL LETTER R WITH CARON*/

    for(i = 0xD9; i <= 0xDC; ++i)
        sk_alpha_set_sortbyte(i, 0x75 /*u*/);

    /* already 0xDD */ /*LATIN CAPITAL LETTER Y WITH ACUTE*/
    /* already 0xDE */ /*LATIN CAPITAL LETTER T WITH CEDILLA*/

    sk_alpha_set_sortbyte(0xDF /*ß*/, 0x73 /*s*/);

    /* ISO 8859-2 sort byte mapping - map lower-case as lower-case, without accents as appropriate */

    /* already 0xE0 */ /*LATIN SMALL LETTER R WITH ACUTE*/

    for(i = 0xE1; i <= 0xE4; ++i)
        sk_alpha_set_sortbyte(i, 0x61 /*a*/);

    /* already 0xE5 */ /*LATIN SMALL LETTER L WITH ACUTE*/

    for(i = 0xE6; i <= 0xE8; ++i)
        sk_alpha_set_sortbyte(i, 0x63 /*c*/);

    for(i = 0xE9; i <= 0xEC; ++i)
        sk_alpha_set_sortbyte(i, 0x65 /*e*/);

    for(i = 0xED; i <= 0xEE; ++i)
        sk_alpha_set_sortbyte(i, 0x69 /*i*/);

    for(i = 0xEF; i <= 0xF0; ++i)
        sk_alpha_set_sortbyte(i, 0x64 /*d*/);

    for(i = 0xF1; i <= 0xF2; ++i)
        sk_alpha_set_sortbyte(i, 0x6E /*n*/);

    for(i = 0xF3; i <= 0xF6; ++i)
        sk_alpha_set_sortbyte(i, 0x6F /*o*/);

    /* omitting 0xF7 */ /*÷*/

    /* already 0xF8 */ /*LATIN SMALL LETTER R WITH CARON*/

    for(i = 0xF9; i <= 0xFC; ++i)
        sk_alpha_set_sortbyte(i, 0x75 /*u*/);

    /* already 0xFD */ /*LATIN SMALL LETTER Y WITH ACUTE*/
    /* already 0xFE */ /*LATIN SMALL LETTER T WITH CEDILLA*/

    /* omitting 0xFF */ /*DOT ABOVE*/
}

/*
ISO 8859-2
*/

static void
sk_alpha_ISO_8859_2(void)
{
    sk_alpha_C0_Controls_and_Basic_Latin();

    sk_alpha_C1_Controls();

    sk_alpha_ISO_8859_2_A0_BF();

    sk_alpha_ISO_8859_2_C0_FF();
}

/*
Windows-28592
*/

#define sk_alpha_Windows_28592() \
    sk_alpha_ISO_8859_2()

#if WINDOWS

/*
Windows-1250
*/

static void
sk_alpha_Windows_1250(void)
{
    sk_alpha_ISO_8859_2();

    /* seems easiest to add Windows-1252 C1 then and sort out the differences, resetting and redefining character attributes as needed */
    sk_alpha_Windows_1252_C1();

    /* Windows-1250 additional missing characters in C1: flag as control characters */
    sk_alpha_reset(0x83);
    sk_alpha_set_flag(0x83, T5__C);

    sk_alpha_reset(0x88);
    sk_alpha_set_flag(0x88, T5__C);

    sk_alpha_reset(0x98);
    sk_alpha_set_flag(0x98, T5__C);

    /*
    The designer of Windows-1250 made some strange choices.
    We must move fifteen entries set by ISO 8859-2.
    Some move down into C1, some move within 0xA0..0xBF.
    */

    /*
    Ten moves from standard ISO 8859-2 0xA0..0xBF down into C1.
    These moves allow some ISO 8859-1 punctuation characters to be used in Windows-1250.
    */

    /* LATIN CAPITAL LETTER S WITH CARON */
    /* LATIN SMALL LETTER S WITH CARON */
    sk_alpha_reset(0xA9 /*ISO 8859-2*/);
    sk_alpha_reset(0xB9 /*ISO 8859-2*/);
    /* move to */
    /*sk_alpha_reset(0x8A);*/ /*Windows-1250*/
    /*sk_alpha_reset(0x9A);*/ /*Windows-1250*/

    sk_alpha_Latin_Letter_S_With_Diacritical_Mark(0x8A /*LATIN CAPITAL LETTER S WITH CARON*/, 0x9A /*LATIN SMALL LETTER S WITH CARON*/);

    /* LATIN CAPITAL LETTER S WITH ACUTE */
    /* LATIN SMALL LETTER S WITH ACUTE */
    sk_alpha_reset(0xA6 /*ISO 8859-2*/);
    sk_alpha_reset(0xB6 /*ISO 8859-2*/);
    /* move to */
    /*sk_alpha_reset(0x8C);*/ /*Windows-1250*/
    /*sk_alpha_reset(0x9C);*/ /*Windows-1250*/

    sk_alpha_Latin_Letter_S_With_Diacritical_Mark(0x8C /*LATIN CAPITAL LETTER S WITH ACUTE*/, 0x9C /*LATIN SMALL LETTER S WITH ACUTE*/);

    /* LATIN CAPITAL LETTER T WITH CARON */
    /* LATIN SMALL LETTER T WITH CARON */
    sk_alpha_reset(0xAB /*ISO 8859-2*/);
    sk_alpha_reset(0xBB /*ISO 8859-2*/);
    /* move to */
    /*sk_alpha_reset(0x8D);*/ /*Windows-1250*/
    /*sk_alpha_reset(0x9D);*/ /*Windows-1250*/

    sk_alpha_Latin_Letter_T_With_Diacritical_Mark(0x8D /*LATIN CAPITAL LETTER T WITH CARON*/, 0x9D /*LATIN SMALL LETTER T WITH CARON*/);

    /* LATIN CAPITAL LETTER Z WITH CARON */
    /* LATIN SMALL LETTER Z WITH CARON */
    sk_alpha_reset(0xAE /*ISO 8859-2*/);
    sk_alpha_reset(0xBE /*ISO 8859-2*/);
    /* move to */
    /*sk_alpha_reset(0x8E);*/ /*Windows-1250*/
    /*sk_alpha_reset(0x9E);*/ /*Windows-1250*/

    sk_alpha_Latin_Letter_Z_With_Diacritical_Mark(0x8E /*LATIN CAPITAL LETTER Z WITH CARON*/, 0x9E /*LATIN SMALL LETTER Z WITH CARON*/);

    /* LATIN CAPITAL LETTER Z WITH ACUTE */
    /* LATIN SMALL LETTER Z WITH ACUTE */
    sk_alpha_reset(0xAC /*ISO 8859-2*/);
    sk_alpha_reset(0xAC /*ISO 8859-2*/);
    /* move to */
    /*sk_alpha_reset(0x8F);*/ /*Windows-1250*/
    /*sk_alpha_reset(0x9F);*/ /*Windows-1250*/

    sk_alpha_Latin_Letter_Z_With_Diacritical_Mark(0x8F /*LATIN CAPITAL LETTER Z WITH ACUTE*/, 0x9F /*LATIN SMALL LETTER Z WITH ACUTE*/);

    /*
    Five moves within 0xA0..0xBF.
    */

    /*CARON*/
    sk_alpha_reset(0xB7 /*ISO 8859-2*/);
    /* moves to */
    sk_alpha_reset(0xA1 /*Windows-1250*/); /*replacing LATIN CAPITAL LETTER A WITH OGONEK in ISO 8859-2*/

    sk_alpha_set_flag(0xA1, T5__P);

    /* LATIN CAPITAL LETTER A WITH OGONEK */
    /* LATIN SMALL LETTER A WITH OGONEK */
    sk_alpha_reset(0xA1 /*ISO 8859-2*/);
    sk_alpha_reset(0xB1 /*ISO 8859-2*/);
    /* move to */
    /*sk_alpha_reset(0xA5);*/ /*Windows-1250*/ /*replacing LATIN CAPITAL LETTER L WITH CARON in ISO 8859-2*/
    /*sk_alpha_reset(0xB9);*/ /*Windows-1250*/ /*replacing SUPERSCRIPT ONE in ISO 8859-2*/

    sk_alpha_Latin_Letter_A_With_Diacritical_Mark(0xA5 /*LATIN CAPITAL LETTER A WITH OGONEK*/, 0xB9 /*LATIN SMALL LETTER A WITH OGONEK*/);

    /* LATIN CAPITAL LETTER L WITH CARON */
    /* LATIN SMALL LETTER L WITH CARON */
    /*sk_alpha_reset(0xA5);*/ /*ISO 8859-2*/ /* already set just above! */
    sk_alpha_reset(0xB5 /*ISO 8859-2*/);
    /* move to */
    /*sk_alpha_reset(0xBC);*/ /*Windows-1250*/ /*replacing LATIN SMALL LETTER Z WITH ACUTE in ISO 8859-2*/
    /*sk_alpha_reset(0xBE);*/ /*Windows-1250*/ /*replacing LATIN SMALL LETTER Z WITH CARON in ISO 8859-2*/

    sk_alpha_Latin_Letter_L_With_Diacritical_Mark(0xBC /*LATIN CAPITAL LETTER L WITH CARON*/, 0xBE /*LATIN SMALL LETTER L WITH CARON*/);

    /*
    Ten characters reinstated from ISO 8859-1 !!!
    */

    sk_alpha_set_flag(0xA6 /*¦*/, T5__P);
    sk_alpha_set_flag(0xA9 /*©*/, T5__P);
    sk_alpha_set_flag(0xAB /*«*/, T5__P);
    sk_alpha_set_flag(0xAC /*¬*/, T5__P);
    sk_alpha_set_flag(0xAE /*®*/, T5__P);
    sk_alpha_set_flag(0xB1 /*±*/, T5__P);
    sk_alpha_set_flag(0xB5 /*µ*/, T5__P);
    sk_alpha_set_flag(0xB6 /*¶*/, T5__P);
    sk_alpha_set_flag(0xB7 /*·*/, T5__P);
    sk_alpha_set_flag(0xBB /*»*/, T5__P);
}

#endif /* WINDOWS */

#if RISCOS

/*
Acorn Extended Latin-2
*/

static void
sk_alpha_Alphabet_Latin2_C1(void)
{
    /* Acorn Extended Latin-2 characters in C1 */
    sk_alpha_AEL_C1_80();
    /* 0x81..0x8B still absent */
    sk_alpha_AEL_C1_8C_99();
    sk_alpha_AEL_C1_9A_9B();
    sk_alpha_AEL_C1_9C_9D();
    sk_alpha_AEL_C1_9E_9F();
}

static void
sk_alpha_Alphabet_Latin2(void)
{
    sk_alpha_ISO_8859_2();

    sk_alpha_Alphabet_Latin2_C1();
}

#endif /* RISCOS */

/**********************
*
* ISO 8859-3 & variants
*
**********************/

/*
ISO 8859-3 range 0xA0..0xFF.
*/

static void
sk_alpha_ISO_8859_3_A0_BF(void)
{
    sk_alpha_ISO_8859_1_A0_BF(); /* start with this and apply overrides */

    sk_alpha_reset(0xA5); /* not present in ISO 8859-3 */
    sk_alpha_reset(0xAE); /* not present in ISO 8859-3 */
    sk_alpha_reset(0xBE); /* not present in ISO 8859-3 */

    sk_alpha_Latin_Letter_H_With_Diacritical_Mark(0xA1 /*LATIN CAPITAL LETTER H WITH STROKE*/, 0xB1 /*LATIN SMALL LETTER H WITH STROKE*/);
    sk_alpha_Latin_Letter_H_With_Diacritical_Mark(0xA6 /*LATIN CAPITAL LETTER H WITH CIRCUMFLEX*/, 0xB6 /*LATIN SMALL LETTER H WITH CIRCUMFLEX*/);
    sk_alpha_Latin_Letter_I_With_Diacritical_Mark(0xA9 /*LATIN CAPITAL LETTER I WITH DOT ABOVE*/, 0xB9 /*LATIN SMALL LETTER I WITH DOT ABOVE*/);
    sk_alpha_Latin_Letter_S_With_Diacritical_Mark(0xAA /*LATIN CAPITAL LETTER S WITH CEDILLA*/, 0xBA /*LATIN SMALL LETTER S WITH CEDILLA*/);
    sk_alpha_Latin_Letter_G_With_Diacritical_Mark(0xAB /*LATIN CAPITAL LETTER G WITH BREVE*/, 0xBB /*LATIN SMALL LETTER G WITH BREVE*/);
    sk_alpha_Latin_Letter_J_With_Diacritical_Mark(0xAC /*LATIN CAPITAL LETTER J WITH CIRCUMFLEX*/, 0xBC /*LATIN SMALL LETTER J WITH CIRCUMFLEX*/);
    sk_alpha_Latin_Letter_Z_With_Diacritical_Mark(0xAF /*LATIN CAPITAL LETTER Z WITH DOT ABOVE*/, 0xBF /*LATIN SMALL LETTER Z WITH DOT ABOVE*/);
}

static void
sk_alpha_ISO_8859_3_C0_FF(void)
{
    U32 i;

    /* ISO 8859-3 punctuation characters */
    sk_alpha_set_flag(0xD7 /*×*/, T5__P);
    sk_alpha_set_flag(0xF7 /*÷*/, T5__P);

    sk_alpha_set_flag(0xDF /*ß*/, T5__L);

    /* ISO 8859-3 upper-case characters: trivial lower-case mapping */
    for(i = 0xC0; i <= 0xDE; ++i)
    {
        switch(i)
        {
        case 0xC3: /* not present in ISO 8859-3 */
        case 0xD0: /* not present in ISO 8859-3 */
        case 0xD7: /*×*/
            break;

        default:
            sk_alpha_set_as_upper(i, (i - (U32) 'A') + 'a');
            break;
        }
    }

    /* ISO 8859-3 lower-case characters: trivial upper-case mapping */
    for(i = 0xE0; i <= 0xFE; ++i)
    {
        switch(i)
        {
        case 0xE3: /* not present in ISO 8859-3 */
        case 0xE0: /* not present in ISO 8859-3 */
        case 0xF7: /*÷*/
            break;

        default:
            sk_alpha_set_as_lower(i, (i - (U32) 'a') + 'A');
            break;
        }
    }

    /* ISO 8859-3 sort byte mapping - map upper-case as lower-case, without accents as appropriate */

    for(i = 0xC0; i <= 0xC4; ++i)
        if(i != 0xC3) /* not present in ISO 8859-3 */
            sk_alpha_set_sortbyte(i, 0x61 /*a*/);

    for(i = 0xC5; i <= 0xC7; ++i)
        sk_alpha_set_sortbyte(i, 0x63 /*c*/);

    for(i = 0xC8; i <= 0xCB; ++i)
        sk_alpha_set_sortbyte(i, 0x65 /*e*/);

    for(i = 0xCC; i <= 0xCF; ++i)
        sk_alpha_set_sortbyte(i, 0x69 /*i*/);

    /*0xD0*/ /* not present in ISO 8859-3 */

    sk_alpha_set_sortbyte(0xD1, 0x6E /*n*/);

    for(i = 0xD2; i <= 0xD4; ++i)
        sk_alpha_set_sortbyte(i, 0x6F /*o*/);

    sk_alpha_set_sortbyte(0xD5, 0x67 /*g*/);

    sk_alpha_set_sortbyte(0xD6, 0x6F /*o*/);

    /* omitting 0xD7 */ /*×*/

    sk_alpha_set_sortbyte(0xD8, 0x67 /*g*/);

    for(i = 0xD9; i <= 0xDD; ++i)
        sk_alpha_set_sortbyte(i, 0x75 /*u*/);

    sk_alpha_set_sortbyte(0xDE, 0x73 /*s*/);

    /* ISO 8859-3 sort byte mapping - map lower-case as lower-case, without accents as appropriate */

    for(i = 0xE0; i <= 0xE4; ++i)
        if(i != 0xE3) /* not present in ISO 8859-3 */
            sk_alpha_set_sortbyte(i, 0x61 /*a*/);

    for(i = 0xE5; i <= 0xE7; ++i)
        sk_alpha_set_sortbyte(i, 0x63 /*c*/);

    for(i = 0xE8; i <= 0xEB; ++i)
        sk_alpha_set_sortbyte(i, 0x65 /*e*/);

    for(i = 0xEC; i <= 0xEF; ++i)
        sk_alpha_set_sortbyte(i, 0x69 /*i*/);

    /*0xF0*/ /* not present in ISO 8859-3 */

    sk_alpha_set_sortbyte(0xF1, 0x6E /*n*/);

    for(i = 0xF2; i <= 0xF4; ++i)
        sk_alpha_set_sortbyte(i, 0x6F /*o*/);

    sk_alpha_set_sortbyte(0xF5, 0x67 /*g*/);

    sk_alpha_set_sortbyte(0xF6, 0x6F /*o*/);

    /* omitting 0xF7 */ /*÷*/

    sk_alpha_set_sortbyte(0xF8, 0x67 /*g*/);

    for(i = 0xF9; i <= 0xFD; ++i)
        sk_alpha_set_sortbyte(i, 0x75 /*u*/);

    sk_alpha_set_sortbyte(0xFE, 0x73 /*s*/);
}

/*
ISO 8859-3
*/

static void
sk_alpha_ISO_8859_3(void)
{
    sk_alpha_C0_Controls_and_Basic_Latin();

    sk_alpha_C1_Controls();

    sk_alpha_ISO_8859_3_A0_BF();

    sk_alpha_ISO_8859_3_C0_FF();
}

/*
Windows-28593
*/

#define sk_alpha_Windows_28593() \
    sk_alpha_ISO_8859_3()


#if WINDOWS

#endif /* WINDOWS */

#if RISCOS

/*
Acorn Extended Latin-3
*/

static void
sk_alpha_Alphabet_Latin3(void)
{
    sk_alpha_ISO_8859_3();

    sk_alpha_Alphabet_Latin2_C1(); /* Acorn Extended Latin-3 characters in C1 same as Acorn Extended Latin-2 */
}

#endif /* RISCOS */

/**********************
*
* ISO 8859-4 & variants
*
**********************/

/*
ISO 8859-4 range 0xA0..0xFF.
*/

static void
sk_alpha_ISO_8859_4_A0_BF(void)
{
    sk_alpha_ISO_8859_1_A0_BF(); /* start with this and apply overrides */

    sk_alpha_Latin_Letter_A_With_Diacritical_Mark(0xA1 /*LATIN CAPITAL LETTER A WITH OGONEK*/, 0xB1 /*LATIN SMALL LETTER A WITH OGONEK*/);
    sk_alpha_Latin_Letter_R_With_Diacritical_Mark(0xA3 /*LATIN CAPITAL LETTER R WITH CEDILLA*/, 0xB3 /*LATIN SMALL LETTER R WITH CEDILLA*/);
    sk_alpha_Latin_Letter_I_With_Diacritical_Mark(0xA5 /*LATIN CAPITAL LETTER I WITH TILDE*/, 0xB5 /*LATIN SMALL LETTER I WITH TILDE*/);
    sk_alpha_Latin_Letter_L_With_Diacritical_Mark(0xA6 /*LATIN CAPITAL LETTER L WITH CEDILLA*/, 0xB6 /*LATIN SMALL LETTER L WITH CEDILLA*/);
    sk_alpha_Latin_Letter_S_With_Diacritical_Mark(0xA9 /*LATIN CAPITAL LETTER S WITH CARON*/, 0xB9 /*LATIN SMALL LETTER S WITH CARON*/);
    sk_alpha_Latin_Letter_E_With_Diacritical_Mark(0xAA /*LATIN CAPITAL LETTER E WITH MACRON*/, 0xBA /*LATIN SMALL LETTER E WITH MACRON*/);
    sk_alpha_Latin_Letter_G_With_Diacritical_Mark(0xAB /*LATIN CAPITAL LETTER G WITH CEDILLA*/, 0xBB /*LATIN SMALL LETTER G WITH CEDILLA*/);
    sk_alpha_Latin_Letter_T_With_Diacritical_Mark(0xAC /*LATIN CAPITAL LETTER T WITH STROKE*/, 0xBC /*LATIN SMALL LETTER T WITH STROKE*/);
    sk_alpha_Latin_Letter_Z_With_Diacritical_Mark(0xAE /*LATIN CAPITAL LETTER Z WITH CARON*/, 0xBE /*LATIN SMALL LETTER Z WITH CARON*/);

    /* oddballs */
    sk_alpha_set_flag(0xA2 /*LATIN SMALL LETTER KRA*/, T5__L);
    sk_alpha_set_as_upper(0xBD /*LATIN CAPITAL LETTER ENG*/, 0xBF /*LATIN SMALL LETTER ENG*/);
    sk_alpha_set_as_lower(0xBF /*LATIN SMALL LETTER ENG*/, 0xBD /*LATIN CAPITAL LETTER ENG*/);
    sk_alpha_set_sortbyte(0xBD /*LATIN CAPITAL LETTER ENG*/, 0xBF /*LATIN SMALL LETTER ENG*/);
}

static void
sk_alpha_ISO_8859_4_C0_FF(void)
{
    U32 i;

    /* ISO 8859-4 punctuation characters */
    sk_alpha_set_flag(0xD7 /*×*/, T5__P);
    sk_alpha_set_flag(0xF7 /*÷*/, T5__P);

    sk_alpha_set_flag(0xDF /*ß*/, T5__L);

    /* ISO 8859-4 lower-case characters: trivial upper-case mapping */
    for(i = 0xC0; i <= 0xDE; ++i)
        if(i != 0xD7 /*×*/)
            sk_alpha_set_as_upper(i,        (i - (U32) 'A') + 'a');

    /* ISO 8859-4 lower-case characters: trivial upper-case mapping */
    for(i = 0xE0; i <= 0xFE; ++i)
        if(i != 0xF7 /*÷*/)
            sk_alpha_set_as_lower(i,        (i - (U32) 'a') + 'A');

    /* ISO 8859-4 sort byte mapping - map upper-case as lower-case, without accents as appropriate */

    for(i = 0xC0; i <= 0xC6; ++i)
        sk_alpha_set_sortbyte(i, 0x61 /*a*/);

    sk_alpha_set_sortbyte(0xC7, 0x69 /*i*/);

    sk_alpha_set_sortbyte(0xC8, 0x63 /*c*/);

    for(i = 0xC9; i <= 0xCC; ++i)
        sk_alpha_set_sortbyte(i, 0x65 /*e*/);

    for(i = 0xCD; i <= 0xCF; ++i)
        sk_alpha_set_sortbyte(i, 0x69 /*i*/);

    sk_alpha_set_sortbyte(0xD0, 0x64 /*d*/);

    sk_alpha_set_sortbyte(0xD1, 0x6E /*n*/);

    sk_alpha_set_sortbyte(0xD2, 0x6F /*o*/);

    sk_alpha_set_sortbyte(0xD3, 0x6B /*k*/);

    for(i = 0xD4; i <= 0xD6; ++i)
        sk_alpha_set_sortbyte(i, 0x6F /*o*/);

    /* omitting 0xD7 */ /*×*/

    sk_alpha_set_sortbyte(0xD8, 0x6F /*o*/);

    for(i = 0xD9; i <= 0xDE; ++i)
        sk_alpha_set_sortbyte(i, 0x75 /*u*/);

    /* ISO 8859-4 sort byte mapping - map lower-case as lower-case, without accents as appropriate */

    for(i = 0xE0; i <= 0xE6; ++i)
        sk_alpha_set_sortbyte(i, 0x61 /*a*/);

    sk_alpha_set_sortbyte(0xE7, 0x69 /*i*/);

    sk_alpha_set_sortbyte(0xE8, 0x63 /*c*/);

    for(i = 0xE9; i <= 0xEC; ++i)
        sk_alpha_set_sortbyte(i, 0x65 /*e*/);

    for(i = 0xED; i <= 0xEF; ++i)
        sk_alpha_set_sortbyte(i, 0x69 /*i*/);

    sk_alpha_set_sortbyte(0xF0, 0x64 /*d*/);

    sk_alpha_set_sortbyte(0xF1, 0x6E /*n*/);

    sk_alpha_set_sortbyte(0xF2, 0x6F /*o*/);

    sk_alpha_set_sortbyte(0xF3, 0x6B /*k*/);

    for(i = 0xF4; i <= 0xF6; ++i)
        sk_alpha_set_sortbyte(i, 0x6F /*o*/);

    /* omitting 0xF7 */ /*÷*/

    sk_alpha_set_sortbyte(0xF8, 0x6F /*o*/);

    for(i = 0xF9; i <= 0xFE; ++i)
        sk_alpha_set_sortbyte(i, 0x75 /*u*/);
}

/*
ISO 8859-4
*/

static void
sk_alpha_ISO_8859_4(void)
{
    sk_alpha_C0_Controls_and_Basic_Latin();

    sk_alpha_C1_Controls();

    sk_alpha_ISO_8859_4_A0_BF();

    sk_alpha_ISO_8859_4_C0_FF();
}

/*
Windows-28594
*/

#define sk_alpha_Windows_28594() \
    sk_alpha_ISO_8859_4()


#if WINDOWS

#endif /* WINDOWS */

#if RISCOS

/*
Acorn Extended Latin-4
*/

static void
sk_alpha_Alphabet_Latin4(void)
{
    sk_alpha_ISO_8859_4();

    sk_alpha_Alphabet_Latin2_C1(); /* Acorn Extended Latin-4 characters in C1 same as Acorn Extended Latin-2 */
}

#endif /* RISCOS */

/***********************
*
* ISO 8859-15 & variants
*
***********************/

/*
ISO 8859-15
*/

static void
sk_alpha_ISO_8859_15(void)
{
    /* This is *very* close to ISO 8859-1 so derive from that */
    sk_alpha_ISO_8859_1();

    /* overrides in 0xA0..0xFF */
    sk_alpha_Euro_Currency_Sign(0xA4 /*EURO CURRENCY SIGN*/);
    sk_alpha_Latin_Letter_S_With_Diacritical_Mark(0xA6 /*LATIN CAPITAL LETTER S WITH CARON*/, 0xA8 /*LATIN SMALL LETTER S WITH CARON*/);
    sk_alpha_Latin_Letter_Z_With_Diacritical_Mark(0xB4 /*LATIN CAPITAL LETTER Z WITH CARON*/, 0xB8 /*LATIN SMALL LETTER Z WITH CARON*/);
    sk_alpha_Latin_Ligature_OE(0xBC /*LATIN CAPITAL LIGATURE OE*/, 0xBD /*LATIN SMALL LIGATURE OE*/);
    sk_alpha_Latin_Capital_Letter_Y_With_Diaeresis(0xBE /*LATIN CAPITAL LETTER Y WITH DIAERESIS*/);
}

/*
Windows-28605
*/

#define sk_alpha_Windows_28605() \
    sk_alpha_ISO_8859_15()

#if WINDOWS

#endif /* WINDOWS */

#if RISCOS

/*
Acorn Extended Latin-9
*/

static void
sk_alpha_Alphabet_Latin9_C1(void)
{
    /* Acorn Extended Latin-9 characters in C1 */
    /* 0x80 still absent */
    sk_alpha_AEL_C1_81_82();
    /* 0x83..0x84 still absent */
    sk_alpha_AEL_C1_85_86();
    /* 0x87..0x8B still absent */
    sk_alpha_AEL_C1_8C_99();
    /* 0x9A..0x9B still absent */
    sk_alpha_AEL_C1_9C_9D();
    sk_alpha_AEL_C1_9E_9F();
}

static void
sk_alpha_Alphabet_Latin9(void)
{
    sk_alpha_ISO_8859_15();

    sk_alpha_Alphabet_Latin9_C1();
}

#endif /* RISCOS */

extern void
sk_alpha_startup(void)
{
    /* start by mapping each character to itself, ignoring case and flags for now */
    sk_alpha_reset_range(0x00U, 0xFFU);

    switch(get_system_codepage())
    {
    default: default_unhandled();
        sk_alpha_ISO_8859_1();
        break;

#if WINDOWS

    case SBCHAR_CODEPAGE_WINDOWS_1252:
        sk_alpha_Windows_1252();
        break;

    case SBCHAR_CODEPAGE_WINDOWS_1250:
        sk_alpha_Windows_1250();
        break;

#endif /* WINDOWS */

    case SBCHAR_CODEPAGE_WINDOWS_28591:
        sk_alpha_Windows_28591();
        break;

    case SBCHAR_CODEPAGE_WINDOWS_28592:
        sk_alpha_Windows_28592();
        break;

    case SBCHAR_CODEPAGE_WINDOWS_28593:
        sk_alpha_Windows_28593();
        break;

    case SBCHAR_CODEPAGE_WINDOWS_28594:
        sk_alpha_Windows_28594();
        break;

    case SBCHAR_CODEPAGE_WINDOWS_28605:
        sk_alpha_Windows_28605();
        break;

#if RISCOS

    case SBCHAR_CODEPAGE_ALPHABET_LATIN1:
        sk_alpha_Alphabet_Latin1();
        break;

    case SBCHAR_CODEPAGE_ALPHABET_LATIN2:
        sk_alpha_Alphabet_Latin2();
        break;

    case SBCHAR_CODEPAGE_ALPHABET_LATIN3:
        sk_alpha_Alphabet_Latin3();
        break;

    case SBCHAR_CODEPAGE_ALPHABET_LATIN4:
        sk_alpha_Alphabet_Latin4();
        break;

    case SBCHAR_CODEPAGE_ALPHABET_LATIN9:
        sk_alpha_Alphabet_Latin9();
        break;

#endif /* OS */

    }

#if defined(HAS_REFERENCE_CTYPE)
    {
    U32 i;

    for(i = 0x00; i <= 0xFFU; ++i)
    {
        if(t5__ctype_sbchar[i] != __reference_ctype[i])
        {
            reportf(
                TEXT("t5__ctype[") U32_XTFMT TEXT("] ") U32_XTFMT TEXT(" != __reference_ctype[] ") U32_XTFMT,
                i, t5__ctype_sbchar[i], __reference_ctype[i]);
        }
    }
    } /*block*/
#endif

    /* run with a copy of the Latin-N table we just set up until any overrides come in */
    assert(sizeof32(t5__ctype) == sizeof32(t5__ctype_sbchar));
    memcpy32(t5__ctype, t5__ctype_sbchar, sizeof32(t5__ctype));
}

T5_CMD_PROTO(extern, t5_cmd_ctypetable)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 5);
    const U8 c = p_args[0].val.u8n;
    U32 ctb = 0, lcb = 0, ucb = 0, sob = 0;
    U32 bic_mask = 0;
    U32 value;

    IGNOREPARM_DocuRef_(p_docu);
    IGNOREPARM_InVal_(t5_message);

    if(arg_is_present(p_args, 1))
    {
        ctb = p_args[1].val.u8n;
        bic_mask |= 0x000000FFU;
    }

    if(arg_is_present(p_args, 2))
    {
        ucb = p_args[2].val.u8n;
        ucb = ucb << 8;
        bic_mask |= 0x0000FF00U;
    }

    if(arg_is_present(p_args, 3))
    {
        lcb = p_args[3].val.u8n;
        lcb = lcb << 16;
        bic_mask |= 0x00FF0000U;
    }

    if(arg_is_present(p_args, 4))
    {
        sob = p_args[4].val.u8n;
        sob = sob << 24;
        bic_mask |= 0xFF000000U;
    }

    value = t5__ctype[c];
    value = value & ~bic_mask;
    value = value | sob | lcb | ucb | ctb;
    t5__ctype[c] = value;

    return(STATUS_OK);
}

/* end of sk_alpha.c */
