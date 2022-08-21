/* riscos/vfp_double.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2021 Stuart Swales */

#if RISCOS

typedef struct VFP_DOUBLE
{
    union VFP_DOUBLE_U
    {
        U32 u32[2];
        double d;
    } u;
}
VFP_DOUBLE, * P_VFP_DOUBLE; typedef const VFP_DOUBLE * PC_VFP_DOUBLE;

/*
vfp_double.c
*/

extern bool
vfp_context_ensure(void);

extern void
vfp_context_destroy(void);

/*
vfparith.s
*/

extern VFP_DOUBLE vfp_double_from_f64(PC_F64 p_f64);

extern void vfp_double_add_r(P_VFP_DOUBLE result, PC_VFP_DOUBLE a, PC_VFP_DOUBLE b);
extern void vfp_double_sub_r(P_VFP_DOUBLE result, PC_VFP_DOUBLE a, PC_VFP_DOUBLE b);
extern void vfp_double_mul_r(P_VFP_DOUBLE result, PC_VFP_DOUBLE a, PC_VFP_DOUBLE b);
extern void vfp_double_div_r(P_VFP_DOUBLE result, PC_VFP_DOUBLE a, PC_VFP_DOUBLE b);

extern VFP_DOUBLE vfp_double_add(PC_VFP_DOUBLE a, PC_VFP_DOUBLE b);
extern VFP_DOUBLE vfp_double_sub(PC_VFP_DOUBLE a, PC_VFP_DOUBLE b);
extern VFP_DOUBLE vfp_double_mul(PC_VFP_DOUBLE a, PC_VFP_DOUBLE b);
extern VFP_DOUBLE vfp_double_mla(PC_VFP_DOUBLE a, PC_VFP_DOUBLE b, PC_VFP_DOUBLE c); /* c + (a*b) */
extern VFP_DOUBLE vfp_double_div(PC_VFP_DOUBLE a, PC_VFP_DOUBLE b);

extern VFP_DOUBLE vfp_double_mul_sub_mul(PC_VFP_DOUBLE a, PC_VFP_DOUBLE b, PC_VFP_DOUBLE c, PC_VFP_DOUBLE d); /* (a*b) - (c*d) */

extern int        vfp_double_compare_zero(PC_VFP_DOUBLE a);

static inline void
vfp_double_copy_from_f64(
    _OutRef_    P_VFP_DOUBLE p_vfp_double_out,
    _InRef_     PC_F64 p_f64_in)
{
    PC_U32 p_u32_in = (PC_U32) p_f64_in;
    p_vfp_double_out->u.u32[1] = p_u32_in[0]; /* swap words */
    p_vfp_double_out->u.u32[0] = p_u32_in[1];
}

static inline void
vfp_double_copy_to_f64(
    _OutRef_    P_F64 p_f64_out,
    _InRef_     PC_VFP_DOUBLE p_vfp_double_in)
{
    P_U32 p_u32_out = (P_U32) p_f64_out;
    p_u32_out[0] = p_vfp_double_in->u.u32[1]; /* swap words */
    p_u32_out[1] = p_vfp_double_in->u.u32[0];
}

#define vfp_double_copy(p_vfp_double_out, p_vfp_double_in) \
    *(p_vfp_double_out) = *(p_vfp_double_in)

static inline void
ss_data_set_real_from_vfp_double(
    _OutRef_    P_SS_DATA p_ss_data,
    _InVal_     VFP_DOUBLE vfp_double)
{
    vfp_double_copy_to_f64(&p_ss_data->arg.fp, &vfp_double);
    ss_data_set_data_id(p_ss_data, DATA_ID_REAL);
}

#endif /* RISCOS */

/* end of riscos/vfp_double.h */
