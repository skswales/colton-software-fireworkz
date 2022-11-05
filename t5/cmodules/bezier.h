/* bezier.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#ifndef __bezier_h
#define __bezier_h

/*
exported functions
*/

extern void
bezier_arc_90(
    _InRef_     PC_DRAW_POINT centre,
    _InVal_     DRAW_COORD radius,
    _InVal_     F64 start_angle,
    _OutRef_    P_DRAW_POINT start,
    _OutRef_    P_DRAW_POINT end,
    _OutRef_    P_DRAW_POINT control1,
    _OutRef_    P_DRAW_POINT control2);

extern void
bezier_arc_90_aligned(
    _InVal_     DRAW_COORD radius,
    _OutRef_    P_DRAW_POINT start,
    _OutRef_    P_DRAW_POINT end,
    _OutRef_    P_DRAW_POINT control1,
    _OutRef_    P_DRAW_POINT control2);

extern void
bezier_arc_circle(
    _InRef_     PC_DRAW_POINT centre,
    _InVal_     DRAW_COORD radius,
    _Out_cap_c_(13) P_DRAW_POINT p /*[13]*/);

extern U32
bezier_arc_segment(
    _InVal_     U32 segment,
    _OutRef_    P_DRAW_POINT end,
    _OutRef_    P_DRAW_POINT control1,
    _OutRef_    P_DRAW_POINT control2);

extern U32
bezier_arc_start(
    _InRef_     PC_DRAW_POINT centre,
    _InVal_     DRAW_COORD radius,
    _InVal_     F64 start_angle,
    _InVal_     F64 end_angle,
    _OutRef_    P_DRAW_POINT start,
    _OutRef_    P_DRAW_POINT end,
    _OutRef_    P_DRAW_POINT control1,
    _OutRef_    P_DRAW_POINT control2);

#endif /* __bezier_h */

/* end of bezier.h */
