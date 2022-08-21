/* gr_coord.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Coordinate manipulation */

/* SKS July 1991 */

#ifndef __gr_coord_h
#define __gr_coord_h

/*
structure
*/

/*
coordinates are large signed things
*/

typedef S32 GR_COORD; typedef GR_COORD * P_GR_COORD;

#define GR_COORD_MAX S32_MAX

#define GR_COORD_TFMT TEXT("%d")

/*
points, or simply pairs of coordinates
*/

typedef struct GR_POINT
{
    GR_COORD x, y;
}
GR_POINT, * P_GR_POINT; typedef const GR_POINT * PC_GR_POINT;

#define GR_POINT_TFMT \
    TEXT("x = ") GR_COORD_TFMT TEXT(", y = ") GR_COORD_TFMT

#define GR_POINT_ARGS(gr_point__ref) \
    (gr_point__ref).x, \
    (gr_point__ref).y

typedef struct GR_SIZE
{
    GR_COORD cx, cy;
}
GR_SIZE, * P_GR_SIZE; typedef const GR_SIZE * PC_GR_SIZE;

/*
boxes, or simply pairs of points
*/

typedef struct GR_BOX
{
    GR_COORD x0, y0, x1, y1;
}
GR_BOX, * P_GR_BOX; typedef const GR_BOX * PC_GR_BOX;

#define GR_BOX_TFMT \
    TEXT("x0 = ") GR_COORD_TFMT TEXT(", y0 = ") GR_COORD_TFMT TEXT("; ") \
    TEXT("x1 = ") GR_COORD_TFMT TEXT(", y1 = ") GR_COORD_TFMT

#define GR_BOX_ARGS(gr_box__ref) \
    (gr_box__ref).x0, \
    (gr_box__ref).y0, \
    (gr_box__ref).x1, \
    (gr_box__ref).y1

/*
ordered rectangles
*/

#if defined(UNUSED_KEEP_ALIVE)

typedef struct GR_RECT
{
    GR_POINT tl, br;
}
GR_RECT, * P_GR_RECT; typedef const GR_RECT * PC_GR_RECT;

#endif /* UNUSED_KEEP_ALIVE */

typedef S32 GR_SCALE; /* signed 16.16 fixed point number */

#define GR_SCALE_ZERO    0          /* 0.0 */
#define GR_SCALE_ONE     0x00010000 /* 1.0 */
#define GR_SCALE_MAX     0x7FFFFFFF /* maximum number in this representation */

#define GR_MAX_FOR_SCALE 0x7FFF     /* maximum number that can be converted to this representation */

typedef struct GR_SCALE_PAIR
{
    GR_SCALE x, y;
}
GR_SCALE_PAIR, * P_GR_SCALE_PAIR; typedef const GR_SCALE_PAIR * PC_GR_SCALE_PAIR;

/*
matrix transforms
*/

typedef struct GR_XFORMMATRIX
{
    GR_SCALE a, b;
    GR_SCALE c, d;
    GR_COORD e, f;
}
GR_XFORMMATRIX, * P_GR_XFORMMATRIX; typedef const GR_XFORMMATRIX * PC_GR_XFORMMATRIX;

/*
exported functions
*/

extern void
eliminate_common_factors(
    _InoutRef_  P_S32 p_numer,
    _InoutRef_  P_S32 p_denom);

/*
box operations
*/

_Check_return_
extern BOOL
gr_box_hit(
    _InRef_     PC_GR_BOX box,
    _InRef_     PC_GR_POINT point,
    _InRef_opt_ PC_GR_SIZE size /*NULL->exact hit required*/);

_Check_return_
extern S32
gr_box_intersection(
    _OutRef_    P_GR_BOX ibox,
    _InRef_     PC_GR_BOX abox,
    _InRef_     PC_GR_BOX bbox);

extern void
gr_box_make_bad(
    _OutRef_    P_GR_BOX abox);

extern void
gr_box_make_huge(
    _OutRef_    P_GR_BOX abox);

extern void
gr_box_make_null(
    _OutRef_    P_GR_BOX abox);

extern void
gdi_rect_clip_mesh(
    _OutRef_    P_GDI_RECT p_rect,
    _InRef_     PC_GDI_RECT p_clip_rect,
    _InRef_     PC_GDI_SIZE meshsize);

extern P_GR_BOX
gr_box_rotate(
    _OutRef_    P_GR_BOX xbox,
    _InRef_     PC_GR_BOX abox,
    _InRef_     PC_GR_POINT spoint,
    _InVal_     F64 angle);

extern P_GR_BOX
gr_box_scale(
    _OutRef_    P_GR_BOX xbox,
    _InRef_     PC_GR_BOX abox,
    _InRef_     PC_GR_POINT spoint,
    _InRef_     PC_GR_SCALE_PAIR scale);

extern P_GR_BOX
gr_box_sort(
    _OutRef_    P_GR_BOX sbox,
    _InRef_     PC_GR_BOX abox);

extern P_GR_BOX
gr_box_translate(
    _OutRef_    P_GR_BOX xbox,
    _InRef_     PC_GR_BOX abox,
    _InRef_     PC_GR_POINT spoint);

extern P_GR_BOX
gr_box_union(
    _OutRef_    P_GR_BOX ubox,
    _InRef_     PC_GR_BOX abox,
    _InRef_     PC_GR_BOX bbox);

extern P_GR_BOX
gr_box_xform(
    _OutRef_    P_GR_BOX xbox,
    _InRef_     PC_GR_BOX abox,
    _InRef_     PC_GR_XFORMMATRIX xform);

/*
coordinate operations
*/

_Check_return_
static inline GR_COORD
gr_coord_from_f64(
    _InVal_     F64 d)
{
    if(d >= +GR_COORD_MAX)
        return(+GR_COORD_MAX);
    if(d <= -GR_COORD_MAX)
        return(-GR_COORD_MAX);
    return((GR_COORD) floor(d));
}

_Check_return_
extern GR_COORD
gr_coord_scale(
    _InVal_     GR_COORD coord,
    _InVal_     GR_SCALE scale);

_Check_return_
extern GR_COORD
gr_coord_scale_inverse(
    _InVal_     GR_COORD coord,
    _InVal_     GR_SCALE scale);

extern void
gr_coord_sort(
    _InoutRef_  P_GR_COORD acoord,
    _InoutRef_  P_GR_COORD bcoord);

/*
point operations
*/

extern void
gdi_point_mesh_hit(
    _OutRef_    P_GDI_POINT meshpoint,
    _InRef_     PC_GDI_POINT testpoint,
    _InRef_     PC_GDI_SIZE meshsize);

extern P_GR_POINT
gr_point_rotate(
    _OutRef_    P_GR_POINT xpoint,
    _InRef_     PC_GR_POINT apoint,
    _InRef_     PC_GR_POINT spoint,
    _InVal_     F64 angle);

extern P_GR_POINT
gr_point_scale(
    _OutRef_    P_GR_POINT xpoint,
    _InRef_     PC_GR_POINT apoint,
    _InRef_     PC_GR_POINT spoint,
    _InRef_     PC_GR_SCALE_PAIR scale);

extern P_GR_POINT
gr_point_translate(
    _OutRef_    P_GR_POINT xpoint,
    _InRef_     PC_GR_POINT apoint,
    _InRef_     PC_GR_POINT spoint);

extern P_GR_POINT
gr_point_xform(
    _OutRef_    P_GR_POINT xpoint,
    _InRef_     PC_GR_POINT apoint,
    _InRef_     PC_GR_XFORMMATRIX xform);

_Check_return_
extern BOOL
cn_PnPoly(
    _InRef_     PC_GR_POINT P,
    _In_reads_(N) PC_GR_POINT V,
    _InVal_     U32 N);

_Check_return_
extern U32
wn_PnPoly(
    _InRef_     PC_GR_POINT P,
    _In_reads_(N) PC_GR_POINT V,
    _InVal_     U32 N);

/*
scale building
*/

#define gr_f64_from_scale(b) ( \
    ((F64) (b)) / GR_SCALE_ONE )

_Check_return_
static inline GR_SCALE
gr_scale_from_f64(
    _InVal_     F64 d)
{
    if(d >= +GR_MAX_FOR_SCALE)
        return(+GR_SCALE_MAX);
    if(d <= -GR_MAX_FOR_SCALE)
        return(-GR_SCALE_MAX);
    return((GR_SCALE) ((d) * GR_SCALE_ONE));
}

_Check_return_
extern GR_SCALE
gr_scale_from_s32_pair(
    _InVal_     S32 numerator,
    _InVal_     S32 denominator);

/*
transformation building
*/

/*ncr*/
extern P_GR_XFORMMATRIX
gr_xform_make_combination(
    _OutRef_    P_GR_XFORMMATRIX xform,
    _InRef_     PC_GR_XFORMMATRIX axform,
    _InRef_     PC_GR_XFORMMATRIX bxform);

/*ncr*/
extern P_GR_XFORMMATRIX
gr_xform_make_rotation(
    _OutRef_    P_GR_XFORMMATRIX xform,
    _InRef_     PC_GR_POINT spoint,
    _InVal_     F64 angle);

/*ncr*/
extern P_GR_XFORMMATRIX
gr_xform_make_scale(
    _OutRef_    P_GR_XFORMMATRIX xform,
    _InRef_     PC_GR_POINT spoint,
    _InVal_     GR_SCALE xscale,
    _InVal_     GR_SCALE yscale);

extern void
gr_xform_make_translation(
    _OutRef_    P_GR_XFORMMATRIX xform,
    _InRef_     PC_GR_POINT spoint);

/*
Draw analogues
*/

/*
box
*/

static inline void
draw_box_make_bad(
    _OutRef_    P_DRAW_BOX p_draw_box)
{
    gr_box_make_bad((const P_GR_BOX) p_draw_box);
}

static inline void
draw_box_sort(
    _OutRef_    P_DRAW_BOX sbox,
    _InRef_     PC_DRAW_BOX abox)
{
    (void) gr_box_sort((const P_GR_BOX) sbox, (const PC_GR_BOX) abox);
}

static inline void
draw_box_translate(
    _OutRef_    P_DRAW_BOX xbox,
    _InRef_     PC_DRAW_BOX abox,
    _InRef_     PC_DRAW_POINT spoint)
{
    (void) gr_box_translate((const P_GR_BOX) xbox, (const PC_GR_BOX) abox, (const PC_GR_POINT) spoint);
}

static inline void
draw_box_union(
    _OutRef_    P_DRAW_BOX ubox,
    _InRef_     PC_DRAW_BOX abox,
    _InRef_     PC_DRAW_BOX bbox)
{
    (void) gr_box_union((const P_GR_BOX) ubox, (const PC_GR_BOX) abox, (const PC_GR_BOX) bbox);
}

static inline void
draw_box_xform(
    _OutRef_    P_DRAW_BOX xbox,
    _InRef_     PC_DRAW_BOX abox,
    _InRef_     PC_GR_XFORMMATRIX xform)
{
    (void) gr_box_xform((const P_GR_BOX) xbox, (const PC_GR_BOX) abox, xform);
}

_Check_return_
static inline DRAW_COORD
draw_coord_scale(
    _InVal_     DRAW_COORD coord,
    _InVal_     GR_SCALE scale)
{
    return((DRAW_COORD) gr_coord_scale((GR_COORD) coord, scale));
}

/*
point operations
*/

static inline void
draw_point_xform(
    _OutRef_    P_DRAW_POINT xpoint,
    _InRef_     PC_DRAW_POINT apoint,
    _InRef_     PC_GR_XFORMMATRIX xform)
{
    gr_point_xform((const P_GR_POINT) xpoint, (const PC_GR_POINT) apoint, xform);
}

#endif /* __gr_coord_h */

/* end of gr_coord.h */
