/* bezier.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Portions of original code provided to Colton Software by Acorn */

/* Based on ideas by David Seal. Ask him about the hard bits.
 *
 * The 'maximum angle' is the longest chunk we want implemented as a single
 * path element. This is assumed to be 90 degrees.
 *
 * All coordinates, radii and centres are passed in Draw units.
 * All angles are passed in radians, in double format.
 * Internal computation is done in doubles.
 *
 * Note that arcs which have start and end angles very close together will
 * appear as circles. This only happens when the difference in angles is less
 * than about 1e-12 degrees.
*/

#include "common/gflags.h"

#include "cmodules/bezier.h"

/*
Useful constants
*/

#define bezier_arc_pi      (3.141592653589793238462643)
#define bezier_arc_rad90   (1.570796326794896619231322) /* pi/2 */
#define bezier_arc_rad360  (6.283185307179586476925286) /* pi*2 */

/*
Useful constants for 90 degree arcs
*/
#define bezier_arc_k90   (0.552284750)
   /* 4 * (sqrt(2) - 1) / 3 */

#define bezier_arc_cos45 (0.707106781)
   /* 1/sqrt(2) */

/*
Data Group : static variables used in arc drawing
*/

static struct BEZIER_ARC_STATICS
{
    U32         segments;  /* Number of segments */
    DRAW_POINT  centre;    /* Centre of arc */
    DRAW_COORD  radius;    /* Radius of arc */
    F64         angle;     /* Current angle */
}
bezier_arc_;

/*
 Function    : bezier_arc_90
 Purpose     : find bezier control points for 90 degree arc
 Description : the control points are calculated as follows:
   1. Conceptually shift the centre to (0,0). The arc then runs from (X, Y) to
      (Y, -X), which can be found from the radius and start angle.
   2. Calculate the bezier control points as (X-kY, Y+kX) and (kX-Y, kY+X)
      where k = 4(SQR(2)-1)/3 [precalculated]
   3. Shift the control points to allow for the actual centre.
      The shifted versions of (X, Y) and (Y, -X) are returned as the start and
      end points.

   Note: when the arc is already axis aligned and centred on (0,0), use
         bezier_arc_90_aligned instead.
*/

extern void
bezier_arc_90(
    _InRef_     PC_DRAW_POINT centre,
    _InVal_     DRAW_COORD radius,
    _InVal_     F64 start_angle,
    _OutRef_    P_DRAW_POINT start,
    _OutRef_    P_DRAW_POINT end,
    _OutRef_    P_DRAW_POINT control1,
    _OutRef_    P_DRAW_POINT control2)
{
    /* Calculate X and Y, and multiples needed for Bezier */
    const F64 x = radius * cos(start_angle);
    const F64 y = radius * sin(start_angle);
    const F64 kx = bezier_arc_k90 * x;
    const F64 ky = bezier_arc_k90 * y;

    /* Find control points and end points, applying origin shift */
    start->x    = centre->x + (DRAW_COORD) +x;
    start->y    = centre->y + (DRAW_COORD) +y;
    end->x      = centre->x + (DRAW_COORD) -y;
    end->y      = centre->y + (DRAW_COORD) +x;

    control1->x = centre->x + (DRAW_COORD) (x  - ky);
    control1->y = centre->y + (DRAW_COORD) (y  + kx);
    control2->x = centre->x + (DRAW_COORD) (kx -  y);
    control2->y = centre->y + (DRAW_COORD) (ky +  x);
}

/*
 Function    : bezier_arc_90_aligned
 Purpose     : draw a centred 90 degrees arc, axis aligned
 Description : this is used to draw a 90 degree arc, centred on the positive
               y axis, with the arc centre at (0,0). Its main use is in drawing
               circles. The method is essentially that of bezier_arc_90, but
               with some calculations taken out. The start angle is taken as
               45 degrees, so it has a cos of 1/SQR(2) and a sine of 1/SQR(2).

               The start and end points are then (x,y) and (-x,y) [x=y].

               Since x = y (and hence kx = ky), the control points simplify to
               (x-kx, x+kx) and (kx-x), (kx+x)
*/

extern void
bezier_arc_90_aligned(
    _InVal_     DRAW_COORD radius,
    _OutRef_    P_DRAW_POINT start,
    _OutRef_    P_DRAW_POINT end,
    _OutRef_    P_DRAW_POINT control1,
    _OutRef_    P_DRAW_POINT control2)
{
    /* Calculate X, and multiple needed for Bezier */
    const F64 x  = radius * bezier_arc_cos45;
    const F64 kx = bezier_arc_k90 * x;
    const DRAW_COORD ix = (DRAW_COORD) x;

    /* Find control points and end points, applying origin shift */
    start->x = +ix;
    start->y = +ix;
    end->x   = -ix;
    end->y   = +ix;
    control1->x = (DRAW_COORD) (x - kx);
    control1->y = (DRAW_COORD) (x + kx);
    control2->x = -control1->x;
    control2->y = +control1->y;
}

/*
 Function    : bezier_arc_circle
 Purpose     : calculate (an approximate) complete circle of arcs
 Description : generates a circle as four arcs of 90 degree each. These are
               placed in a point buffer, implemented as an array of coords,
               which is to be used as follows:
               move to point 0
               curve to 1, 2, 3 (i.e. 1,2 are control, 3 is end)
               curve to 4, 5, 6
               curve to 7, 8, 9
               curve to 10, 11, 12
               The buffer thus needs to be large enough for 13 coordinates.

   To get the points, we proceed as follows: find the control points and start
   and end points for an axis aligned arc based on (0,0).
   The buffer points are then generated by applying centre shifts to the
   points generated, as follows:
    let (a, b) be control point 1 [control point 2 will be (-a,b)]
    let (s, t) be start point [end point will be (-s, s), s = t]
    point 0  = start
    point 1  = (a,b),   point 2  = (-a,b)
    point 3  = end [(-s, s)]
    point 4  = (-b,a),  point 5  = (-b,-a)
    point 6  = (-s,-s)
    point 7  = (-a,-b), point 8  = (a,-b)
    point 9  = (s,-s)
    point 10 = (b,-a),  point 11 = (b,a)
    point 12 = point 0
*/

extern void
bezier_arc_circle(
    _InRef_     PC_DRAW_POINT centre,
    _InVal_     DRAW_COORD radius,
    _Out_cap_c_(13) P_DRAW_POINT p /*[13]*/)
{
    DRAW_POINT start, end, control1, control2;

    /* Get initial parameters */
    bezier_arc_90_aligned(radius, &start, &end, &control1, &control2);

    /* Calculate points adding centre shifts */
    p[0].x  = p[9].x  = centre->x + start.x;
    p[3].x  = p[6].x  = centre->x - start.x;

    p[0].y  = p[3].y  = centre->y + start.y;
    p[6].y  = p[9].y  = centre->y - start.y;

    p[1].x  = p[8].x  = centre->x + control1.x;
    p[2].x  = p[7].x  = centre->x - control1.x;
    p[4].y  = p[11].y = centre->y + control1.x;
    p[5].y  = p[10].y = centre->y - control1.x;

    p[10].x = p[11].x = centre->x + control1.y;
    p[4].x  = p[5].x  = centre->x - control1.y;
    p[1].y  = p[2].y  = centre->y + control1.y;
    p[7].y  = p[8].y  = centre->y - control1.y;

    p[12]   = p[0];
}

/*
 Function    : bezier_arc_short
 Purpose     : calculate bezier points for an arc of less than 90 degrees
 Description : the bezier control points are calculated as follows:
  1. Conceptually centre the arc on 0,0; the end points are defined to
     be (X1, Y1), (X2, Y2) in this coordinate system. X1, X2, Y1, Y2 can be
     calculated from the parameters.
  2. The control points are then (X1-k.Y1, Y1+k.X1) and (X2+k.Y2, Y2-k.X2)
     where k is a positive constant obtained from:

     k = 4(SQR(2.(X1^2 + Y1^2).(X1^2 + Y1^2 + X1.X2 + Y1.Y2))
               - (X1^2 + Y1^2 + X1.X2 + Y1.Y2)) / 3(X1.Y2 - Y1.X2)
  3. Having obtained these control points, shift back to the true origin.
     (x1, y1) and (x2, y2) are returned as the start and end points

 Note that there is no check that the angle is less than the maximum.
*/

static void
bezier_arc_short(
    _InRef_     PC_DRAW_POINT centre,
    _InVal_     DRAW_COORD radius,
    _InVal_     F64 start_angle,
    _InVal_     F64 end_angle,
    _OutRef_    P_DRAW_POINT start,
    _OutRef_    P_DRAW_POINT end,
    _OutRef_    P_DRAW_POINT control1,
    _OutRef_    P_DRAW_POINT control2)
{
    /* Calculate end points, based on (0,0) */
    const F64 x1 = radius * cos(start_angle);
    const F64 y1 = radius * sin(start_angle);
    const F64 x2 = radius * cos(end_angle);
    const F64 y2 = radius * sin(end_angle);
    F64 x1sq_plus_y1sq, sq_plus_x1x2_plus_y1y2, k;

    /* Calculate k */
    x1sq_plus_y1sq = x1*x1 + y1*y1;
    sq_plus_x1x2_plus_y1y2 = x1sq_plus_y1sq + x1*x2 + y1*y2;
    k = 4.0 * (sqrt(2.0 * x1sq_plus_y1sq * sq_plus_x1x2_plus_y1y2)
                  - sq_plus_x1x2_plus_y1y2) / (3.0 * (x1*y2 - y1*x2));

    /* Record start and end locations */
    start->x = centre->x + (DRAW_COORD) x1;
    start->y = centre->y + (DRAW_COORD) y1;
    end->x   = centre->x + (DRAW_COORD) x2;
    end->y   = centre->y + (DRAW_COORD) y2;

    /* Find control points, including origin shift */
    control1->x = centre->x + (DRAW_COORD) (x1 - k * y1);
    control1->y = centre->y + (DRAW_COORD) (y1 + k * x1);
    control2->x = centre->x + (DRAW_COORD) (x2 + k * y2);
    control2->y = centre->y + (DRAW_COORD) (y2 - k * x2);
}

/*
 Function    : bezier_arc_start
 Purpose     : start to draw an arc
 Returns     : number of segments including this one
 Description : calculates the start, and and control points for the first
               segment of an arc. This should be followed by repeated
               calls to bezier_arc_segment.
               Only one arc may be in progress at a time.
               The number of segments can be used for (e.g.) assigning
               memory for the path.
*/

extern U32
bezier_arc_start(
    _InRef_     PC_DRAW_POINT centre,
    _InVal_     DRAW_COORD radius,
    _InVal_     F64 start_angle,
    _InVal_     F64 end_angle_in,
    _OutRef_    P_DRAW_POINT start,
    _OutRef_    P_DRAW_POINT end,
    _OutRef_    P_DRAW_POINT control1,
    _OutRef_    P_DRAW_POINT control2)
{
    F64 end_angle = end_angle_in;

    bezier_arc_.segments = (U32) floor(((end_angle >= start_angle)
                                                ? (                    (end_angle - start_angle))
                                                : (bezier_arc_rad360 + (end_angle - start_angle))
                                         ) / bezier_arc_rad90);

    /* Find end of short segment */
    end_angle -= bezier_arc_.segments * bezier_arc_rad90;

    /* Check for a zero length arc */
    if(start_angle == end_angle)
    {
        /* Find arc points for first 90 degree chunk */
        bezier_arc_90(centre, radius, start_angle,
                      start, end, control1, control2);
        end_angle += bezier_arc_rad90;
    }
    else
    {
        /* Find arc points for first chunk of arc */
        bezier_arc_short(centre, radius, start_angle, end_angle,
                         start, end, control1, control2);
        bezier_arc_.segments += 1;
    }

    /* Record iterator globals */
    bezier_arc_.centre = *centre;
    bezier_arc_.radius = radius;
    bezier_arc_.angle  = end_angle;

    return(bezier_arc_.segments);
}

/*
 Function    : bezier_arc_segment
 Purpose     : draw next segment of an arc
 Returns     : next segment number
 Description : this should be called repeatedly, following bezier_arc_start.
               On the first call, the segment number should be 1, and
               thereafter it should be the segment number returned by the
               previous call.
               The routine returns zero when next called after the last
               segment has been drawn.
*/

extern U32
bezier_arc_segment(
    _InVal_     U32 segment,
    _OutRef_    P_DRAW_POINT end,
    _OutRef_    P_DRAW_POINT control1,
    _OutRef_    P_DRAW_POINT control2)
{
    DRAW_POINT start_dummy;

    if(segment >= bezier_arc_.segments)
    {
        end->x = 0;         end->y = 0;
        control1->x = 0;    control1->y = 0;
        control2->x = 0;    control2->y = 0;
        return(0);
    }

    /* Calculate arc */
    bezier_arc_90(&bezier_arc_.centre,
                   bezier_arc_.radius,
                   bezier_arc_.angle,
                  &start_dummy,
                   end, control1, control2);

    /* Next angle */
    bezier_arc_.angle += bezier_arc_rad90;

    return(segment + 1);
}

/* end of bezier.c */

