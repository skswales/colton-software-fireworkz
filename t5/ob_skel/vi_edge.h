/* vi_edge.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Border / Ruler edge window positioning */

#if RISCOS
#define BORDER_HORZ_PIXIT_HEIGHT    (11 * PIXITS_PER_PROGRAM_PIXEL_Y)
#elif WINDOWS
#define BORDER_HORZ_PIXIT_HEIGHT    (20 * PIXITS_PER_PIXEL)
#endif

#if RISCOS
#define BORDER_VERT_PIXIT_MIN_WIDTH (48 * PIXITS_PER_PIXEL)
#elif WINDOWS
#define BORDER_VERT_PIXIT_MIN_WIDTH (40 * PIXITS_PER_PIXEL)
#endif

#define BORDER_HORZ_TM_MIN          ( 3 * PIXITS_PER_PROGRAM_PIXEL_Y)
#define BORDER_HORZ_BM_MIN          ( 3 * PIXITS_PER_PROGRAM_PIXEL_Y)

#if MARKERS_INSIDE_RULER_HORZ
#if RISCOS
#define RULER_HORZ_PIXIT_HEIGHT     ( 8 * PIXITS_PER_PROGRAM_PIXEL_Y)
#elif WINDOWS
#define RULER_HORZ_PIXIT_HEIGHT     ( 8 * PIXITS_PER_PROGRAM_PIXEL_Y)
#endif
#else /* MARKERS_INSIDE_RULER_HORZ */
#if RISCOS
#define RULER_HORZ_PIXIT_HEIGHT     (14 * PIXITS_PER_PROGRAM_PIXEL_Y)
#elif WINDOWS
#define RULER_HORZ_PIXIT_HEIGHT     (14 * PIXITS_PER_PROGRAM_PIXEL_Y)
#endif
#endif /* MARKERS_INSIDE_RULER_HORZ */

#if MARKERS_INSIDE_RULER_VERT
#if RISCOS
#define RULER_VERT_PIXIT_WIDTH      (40 * PIXITS_PER_PIXEL)
#elif WINDOWS
#define RULER_VERT_PIXIT_WIDTH      (36 * PIXITS_PER_PIXEL)
#endif
#else /* MARKERS_INSIDE_RULER_VERT */
#if RISCOS
#define RULER_VERT_PIXIT_WIDTH      (40 * PIXITS_PER_PIXEL)
#elif WINDOWS
#define RULER_VERT_PIXIT_WIDTH      (36 * PIXITS_PER_PIXEL)
#endif
#endif /* MARKERS_INSIDE_RULER_VERT */

#if RISCOS
#define RULER_HORZ_SCALE_TOP_Y      (2 * PIXITS_PER_PROGRAM_PIXEL_Y)
#elif WINDOWS
#define RULER_HORZ_SCALE_TOP_Y      (3 * PIXITS_PER_PIXEL)
#endif

#if RISCOS
#define RULER_VERT_SCALE_LEFT_X     (4 * PIXITS_PER_PROGRAM_PIXEL_X)
#elif WINDOWS
#define RULER_VERT_SCALE_LEFT_X     (4 * PIXITS_PER_PIXEL)
#endif

_Check_return_
_Ret_valid_
static inline PC_RGB
colour_of_border(
    _InRef_     PC_STYLE p_border_style)
{
    return(
        style_bit_test(p_border_style, STYLE_SW_PS_RGB_BACK)
            ? &p_border_style->para_style.rgb_back
            : &rgb_stash[COLOUR_OF_BORDER]);
}

_Check_return_
_Ret_valid_
static inline PC_RGB
colour_of_ruler(
    _InRef_     PC_STYLE p_ruler_style)
{
    return(
        style_bit_test(p_ruler_style, STYLE_SW_PS_RGB_BACK)
            ? &p_ruler_style->para_style.rgb_back
            : &rgb_stash[COLOUR_OF_RULER]);
}

/* end of vi_edge.h */
