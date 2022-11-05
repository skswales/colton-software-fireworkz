/* cn_pnpoly.c */

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
 
/* See Sunday, Dan (2001), Inclusion of a Point in a Polygon, https://geomalgorithms.com/a03-_inclusion.html. */

#include "common/gflags.h"

/******************************************************************************
*
* cn_PnPoly(): crossing number test for a point in a polygon
*      Input:   P = a point,
*               V[] = vertex points of a polygon V[n+1] with V[n]=V[0]
*      Return:  0 = outside, 1 = inside
*
* This code is patterned after [Franklin, 2000]
*
******************************************************************************/

_Check_return_
extern BOOL
cn_PnPoly(
    _InRef_     PC_GR_POINT P,
    _In_reads_(N) PC_GR_POINT V,
    _InVal_     U32 N /*elemof(V[])*/)
{
    int cn = 0; /* the crossing number counter */
    const U32 n = N - 1;
    U32 i;

    /* loop through all edges of the polygon */
    for(i = 0; i < n; i++)
    {   /* edge from V[i] to V[i+1] */
        if( ((V[i].y <= P->y) && (V[i+1].y  > P->y)) || /* an upward crossing */
            ((V[i].y  > P->y) && (V[i+1].y <= P->y)) )  /* a downward crossing */
       {    /* compute the actual edge-ray intersect x-coordinate */
            const GR_COORD edge_dx = (V[i+1].x - V[i].x);
            const GR_COORD edge_dy = (V[i+1].y - V[i].y);
            const GR_COORD dx = (P->x - V[i].x);
            const GR_COORD dy = (P->y - V[i].y);
            F64 vt = (F64) dy / edge_dy;

            if(dx < vt * edge_dx) /* P.x < intersect */
                ++cn; /* a valid crossing of y=P.y right of P.x */
        }
    }

    return(cn&1); /* 0 if even (out), and 1 if odd (in) */
}

/* end of cn_pnpoly.c */
