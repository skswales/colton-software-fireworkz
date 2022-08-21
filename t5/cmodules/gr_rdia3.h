/* gr_rdia3.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Header file for the gr_rdia3 module */

/* SKS July 1992 */

/*
internally exported functions from gr_rdia3.c
*/

/*
RISC OS Draw styles
*/

extern GR_COLOUR
gr_colour_from_riscDraw(
    _InVal_     DRAW_COLOUR riscDraw);

extern DRAW_COLOUR
gr_colour_to_riscDraw(
    _InVal_     GR_COLOUR colour);

/*
RISC OS Draw path objects
*/

_Check_return_
extern STATUS
gr_riscdiag_path_close(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag);

_Check_return_
extern STATUS
gr_riscdiag_path_curveto(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _InVal_     DRAW_COORD cpx1,
    _InVal_     DRAW_COORD cpy1,
    _InVal_     DRAW_COORD cpx2,
    _InVal_     DRAW_COORD cpy2,
    _InVal_     DRAW_COORD    x,
    _InVal_     DRAW_COORD    y);

_Check_return_
extern STATUS
gr_riscdiag_path_lineto(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _InVal_     DRAW_COORD x,
    _InVal_     DRAW_COORD y);

_Check_return_
extern STATUS
gr_riscdiag_path_moveto(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _InVal_     DRAW_COORD x,
    _InVal_     DRAW_COORD y);

_Check_return_
_Ret_writes_maybenull_(extraBytes)
extern P_BYTE /* -> path guts */
gr_riscdiag_path_new(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _OutRef_    P_DRAW_DIAG_OFFSET pPathStart,
    _InRef_opt_ PC_GR_LINESTYLE linestyle,
    _InRef_opt_ PC_GR_FILLSTYLEC fillstylec,
    _InVal_     U32 extraBytes,
    _OutRef_    P_STATUS p_status);

_Check_return_
_Ret_valid_
extern P_BYTE
gr_riscdiag_path_query_guts(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _InVal_     DRAW_DIAG_OFFSET pathStart);

_Check_return_
extern STATUS
gr_riscdiag_path_term(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag);

/*
RISC OS Draw sprite objects
*/

/*
RISC OS Draw string objects
*/

_Check_return_
extern STATUS
gr_riscdiag_string_new_sbchars(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _OutRef_    P_DRAW_DIAG_OFFSET pTextStart,
    _InRef_     PC_DRAW_POINT point,
    _In_reads_(sbchars_n) PC_SBCHARS sbchars,
    _InVal_     U32 sbchars_n,
    _InRef_     PC_GR_TEXTSTYLE p_gr_textstyle,
    _InVal_     GR_COLOUR fg,
    _In_opt_    const GR_COLOUR * const bg,
    _InRef_     PC_GR_RISCDIAG lookup_gr_riscdiag);

_Check_return_
extern STATUS
gr_riscdiag_string_new_uchars(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _OutRef_    P_DRAW_DIAG_OFFSET pTextStart,
    _InRef_     PC_DRAW_POINT point,
    _In_reads_(uchars_n) PC_UCHARS uchars,
    _InVal_     U32 uchars_n,
    _InRef_     PC_GR_TEXTSTYLE p_gr_textstyle,
    _InVal_     GR_COLOUR fg,
    _In_opt_    const GR_COLOUR * const bg,
    _InRef_     PC_GR_RISCDIAG lookup_gr_riscdiag);

/*
RISC OS Draw pseudo objects
*/

_Check_return_
extern STATUS
gr_riscdiag_circle_new(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _OutRef_    P_DRAW_DIAG_OFFSET pCircleStart,
    _InRef_     PC_DRAW_POINT pOrigin,
    _InVal_     DRAW_COORD radius,
    _InRef_     PC_GR_LINESTYLE linestyle,
    _InRef_     PC_GR_FILLSTYLEC fillstylec);

_Check_return_
extern STATUS
gr_riscdiag_parallelogram_new(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _OutRef_    P_DRAW_DIAG_OFFSET pParaStart,
    _InRef_     PC_DRAW_POINT pOriginBL,
    _InRef_     PC_DRAW_POINT pOffsetBR,
    _InRef_     PC_DRAW_POINT pOffsetTR,
    _InRef_     PC_GR_LINESTYLE linestyle,
    _InRef_opt_ PC_GR_FILLSTYLEC fillstylec);

_Check_return_
extern STATUS
gr_riscdiag_piesector_new(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _OutRef_    P_DRAW_DIAG_OFFSET pPieStart,
    _InRef_     PC_DRAW_POINT pOrigin,
    _InVal_     DRAW_COORD radius,
    _InRef_     PC_F64 alpha,
    _InRef_     PC_F64 beta,
    _InRef_     PC_GR_LINESTYLE linestyle,
    _InRef_     PC_GR_FILLSTYLEC fillstylec);

_Check_return_
extern STATUS
gr_riscdiag_rectangle_new(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _OutRef_    P_DRAW_DIAG_OFFSET pRectStart,
    _InRef_     PC_DRAW_BOX pBox,
    _InRef_opt_ PC_GR_LINESTYLE linestyle,
    _InRef_opt_ PC_GR_FILLSTYLEC fillstylec);

_Check_return_
extern STATUS
gr_riscdiag_scaled_diagram_add(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _OutRef_    P_DRAW_DIAG_OFFSET pPictStart,
    _InRef_     PC_DRAW_BOX pBox,
    _In_reads_(diag_len) PC_BYTE p_diag,
    _InVal_     U32 diag_len,
    _InRef_     PC_GR_FILLSTYLEB fillstyleb,
    _InRef_opt_ PC_GR_FILLSTYLEC fillstylec);

_Check_return_
extern STATUS
gr_riscdiag_scale_diagram(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _InRef_     PC_GR_BOX pBox,
    _InRef_     PC_GR_FILLSTYLEB fillstyleb);

_Check_return_
extern STATUS
gr_riscdiag_trapezoid_new(
    _InoutRef_  P_GR_RISCDIAG p_gr_riscdiag,
    _OutRef_    P_DRAW_DIAG_OFFSET pRectStart,
    _InRef_     PC_DRAW_POINT pOriginBL,
    _InRef_     PC_DRAW_POINT pOffsetBR,
    _InRef_     PC_DRAW_POINT pOffsetTR,
    _InRef_     PC_DRAW_POINT pOffsetTL,
    _InRef_     PC_GR_LINESTYLE linestyle,
    _InRef_     PC_GR_FILLSTYLEC fillstylec);

/*
misc
*/

extern void
gr_riscdiag_host_font_dispose(
    _InoutRef_  P_HOST_FONT p_host_font);

_Check_return_
extern HOST_FONT
gr_riscdiag_host_font_from_textstyle(
    _InRef_     PC_GR_TEXTSTYLE style);

_Check_return_
extern STATUS
gr_riscdiag_host_font_spec_riscos_from_textstyle(
    _OutRef_    P_HOST_FONT_SPEC p_host_font_spec /* h_host_name_tstr as RISC OS eg Trinity.Bold.Italic */,
    _InRef_     PC_GR_TEXTSTYLE textstyle);

/*
end of internal exports from gr_rdia3.c
*/

static inline void
draw_box_from_gr_box(
    _OutRef_    P_DRAW_BOX p_draw_box,
    _InRef_     PC_GR_BOX p_gr_box)
{
    gr_box_xform((P_GR_BOX) p_draw_box, p_gr_box, &gr_riscdiag_riscDraw_from_pixit_xformer);
}

static inline void
draw_point_from_gr_point(
    _OutRef_    P_DRAW_POINT p_draw_point,
    _InRef_     PC_GR_POINT p_gr_point)
{
    gr_point_xform((P_GR_POINT) p_draw_point, p_gr_point, &gr_riscdiag_riscDraw_from_pixit_xformer);
}

/* end of gr_rdia3.h */
