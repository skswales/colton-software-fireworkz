/* muldiv.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* SKS July 1991 */

#include "common/gflags.h"

#include "ob_skel/flags.h"

#if RISCOS && !defined(MULDIV_PORTABLE)
#error You require assembler implementation s.muldivas
#endif

#ifndef          __muldiv_h
#include "cmodules/muldiv.h"
#endif

#include <signal.h>

static struct _muldiv_statics
{
	S32 remainder;
	S32 overflow;
}
muldiv_;

/******************************************************************************
*
* careful 32-bit a1 * 32-bit a2 / 32-bit a3
*
******************************************************************************/

#define tbs 0x80000000U

extern S32
muldiv(S32 a1, S32 a2, S32 a3)
{
	S32 a4;
	S32 v1, v2, v3, v5, v6;
	S32 c, z;
	S32 v4;

	v1 = a1; /* save a */
	v2 = a2; /* save b */
	v3 = a3; /* save c */

/* first, the double-length product of a1 * a2 into a4(hi) & a3(lo)
 * uses v6 as hi-word of a during shifts
 * uses v5 as lo-word of a during shifts
 * destroying v5, v6, a2
*/
	v5 = a1;
	if(v5 < 0)
		v5 = -v5; /* abs a */
	v6 = 0;       /* hi-a */

	if(a2 < 0)
		a2 = -a2;  /* abs-b */

	a3 = 0; /* lo-res */
	a4 = 0; /* hi-res */

	do  {
		/* shift bits out of bottom of b */
		c    = ((a2 & 1) /*!= 0*/);
		a2 >>= 1; /* never has tbs so no ASR worries */
		if(c)
			{
			/* add current shifted 64-bit a to 64-bit result */
			/* will be carry from lo-word iff both tbs */
			c   = ((a3 & tbs) && (v5 & tbs)); /* 1|0 */
			a3 += v5;
			a4 += v6;
			a4 += c;
			}

		/* shift current 64-bit a */
		c    = ((v5 & tbs) != 0); /* 1|0 */
		v5 <<= 1;
		v6 <<= 1;
		v6  += c;
		}
	while(a2); /* [run out of bits in b to multiply by] */

/* now the 64*32 bit divide
 * dividend in a4(hi) and a3(lo)
 * remainder ends up in a3; quotient in v6
 * uses a2(hi) and a1(lo) to hold the (shifted) divisor;
 *      v5 for the current bit in the quotient
*/
	a1 = v3;
	if(a1 < 0)
		a1 = -a1; /* abs c */

	if(a1 == 0)
		raise(SIGFPE);

	a2 = 0;

	v6 = 0; /* becomes quotient */
	v4 = 0; /* quotient-hi */

	v5 = 0;

	while((a2 & tbs) == 0)
		{
		/* compare [a2, a1] against [a4, a3] */
		c = ((U32) a2 >= (U32) a4);
		z = ((U32) a2 == (U32) a4);
		if(z) /* CMPEQ */
			c = ((U32) a1 >= (U32) a3);

		if(c)
			break;

		/* left shift the divisor */
		c    = ((a1 & tbs) != 0); /* 1|0 */
		a1 <<= 1;
		a2 <<= 1;
		a2  += c;

		++v5;
		}

	do  {
		/* compare [a2, a1] against [a4, a3] */
		c = ((U32) a2 >= (U32) a4);
		z = ((U32) a2 == (U32) a4);
		if(z) /* CMPEQ */
			{
			c = ((U32) a1 >= (U32) a3);
			z = ((U32) a1 == (U32) a3);
			}

		if(!c || z) /* ~BHI -> doLS */
			{
			if(v5 <= 31)
				v6 += (1 << v5);
			else if(v5 >= 32)
				v4 += (1 << (v5 - 32));

			/* will subtraction of words cause borrow? */
			c   = ((U32) a3 < (U32) a1); /* 1/0 */
			a3 -= a1;
			a4 -= a2;
			a4 += c; /* borrow, really */
			}

		c    = ((a2 & 1) /*!= 0*/);
		a2 >>= 1; /* ASR, but never has tbs */
		a1 = (S32) ((U32) a1 >> 1); /* RRX */
		if(c)
			a1 |= tbs;
		}
	while(--v5 >= 0);

/* now all we need to do is sort out the signs. */

	v2 = v1 ^ v2; /* v2 := the sign of a * b */
	if((v2 ^ v3) & tbs)
		/* divisor & dividend have opposite signs - negate the quotient */
		{
		v6 = -v6;
		v4 = -v4;
		}

	if(v2 < 0)
		/* divident was negative - negate the remainder */
		a3 = -a3;

	/* stash the remainder */
	muldiv_.remainder = a3;

	/* stash the overflow */
	muldiv_.overflow = v4;

	return(v6);
}

/******************************************************************************
*
* and the remainder thereof
*
******************************************************************************/

extern S32
muldiv_rem(void)
{
	return(muldiv_.remainder);
}

/******************************************************************************
*
* and the overflow thereof
*
******************************************************************************/

extern S32
muldiv_overflow(void)
{
	return(muldiv_.overflow);
}

/******************************************************************************
*
* Skeleton functions that do nothing currently.
*
******************************************************************************/

extern S32
muldiv_ceil(S32 a, S32 b, S32 c)
{
	IGNOREPARM(a);
	IGNOREPARM(b);
	IGNOREPARM(c);
	return(0);
}

extern S32
muldiv_floor(S32 a, S32 b, S32 c)
{
	IGNOREPARM(a);
	IGNOREPARM(b);
	IGNOREPARM(c);
	return(0);
}

extern S32
muldiv_round_floor(S32 a, S32 b, S32 c)
{
	IGNOREPARM(a);
	IGNOREPARM(b);
	IGNOREPARM(c);
	return(0);
}

extern S32
muldiv_limiting(S32 a, S32 b, S32 c)
{
	IGNOREPARM(a);
	IGNOREPARM(b);
	IGNOREPARM(c);
	return(0);
}

/* end of muldiv.c */
