/* wn_pnpoly.c */

/* SKS November 2015 translated to C and Colton Software types / style for use in Fireworkz */

/******************************************************************************
*
* Copyright 2000 softSurfer, 2012 Dan Sunday
* This code may be freely used, distributed and modified for any purpose
* providing that this copyright notice is included with it.
* SoftSurfer makes no warranty for this code, and cannot be held
* liable for any real or imagined damage resulting from its use.
* Users of this code must verify correctness for their application.
*
******************************************************************************/
 
/* See Sunday, Dan (2001), Inclusion of a Point in a Polygon, http://geomalgorithms.com/a03-_inclusion.html. */

#include "common/gflags.h"

/******************************************************************************
*
* isLeft(): tests if a point is Left|On|Right of an infinite line.
*    Input:  three points P0, P1, and P2
*    Return: >0 for P2 left of the line through P0 and P1
*            =0 for P2  on the line
*            <0 for P2  right of the line
*
*    See: Algorithm 1 "Area of Triangles and Polygons"
*
******************************************************************************/

_Check_return_
static inline BOOL
isLeft(
    _InRef_     PC_GR_POINT P0,
    _InRef_     PC_GR_POINT P1,
    _InRef_     PC_GR_POINT P2)
{
    return((P1->x - P0->x) * (P2->y - P0->y) - (P2->x -  P0->x) * (P1->y - P0->y));
}

/******************************************************************************
*
* wn_PnPoly(): winding number test for a point in a polygon
*      Input:   P = a point,
*               V[] = vertex points of a polygon V[n+1] with V[n]=V[0]
*      Return:  wn = the winding number (=0 only when P is outside)
*
******************************************************************************/

_Check_return_
extern U32
wn_PnPoly(
    _InRef_     PC_GR_POINT P,
    _In_reads_(N) PC_GR_POINT V,
    _InVal_     U32 N /*elemof(V[])*/)
{
    U32 wn = 0; /* the winding number counter */
    const U32 n = N - 1;
    U32 i;

    /* loop through all edges of the polygon */
    for(i = 0; i < n; i++)
    {   /* edge from V[i] to V[i+1] */
        if(V[i].y <= P->y)
        {                                           /* start y <= P.y */
            if(V[i+1].y  > P->y)                    /* an upward crossing */
                if(isLeft(&V[i], &V[i+1], P) > 0)   /* P left of edge */
                    ++wn;                           /* have a valid up intersect */
        }
        else
        {                                           /* start y > P.y (no test needed) */
            if(V[i+1].y <= P->y)                    /* a downward crossing */
                if(isLeft(&V[i], &V[i+1], P) < 0)   /* P right of edge */
                    --wn;                           /* have a valid down intersect */
        }
    }

    return(wn);
}

/* end of wn_pnpoly.c */
