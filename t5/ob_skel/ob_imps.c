/* ob_imps.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Fireworkz implied style object */

/* MRJC 1994 */

#include "common/gflags.h"

#ifndef          __sk_slot_h
#include "ob_cells/sk_slot.h"
#endif

/******************************************************************************
*
* answer implied style queries
*
******************************************************************************/

T5_MSG_PROTO(static, implied_style_ext_style_cell_type, P_IMPLIED_STYLE_QUERY p_implied_style_query)
{
    const PC_STYLE_DOCU_AREA p_style_docu_area = p_implied_style_query->p_style_docu_area;
    const P_CELL p_cell = p_cell_from_slr(p_docu, &p_implied_style_query->position.slr);

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if((NULL != p_cell) && (object_id_from_cell(p_cell) == p_implied_style_query->arg))
    {
        const PC_STYLE p_style_from = p_style_from_docu_area(p_docu, p_style_docu_area);
        PTR_ASSERT(p_style_from);
        if(NULL != p_style_from)
            style_copy(p_implied_style_query->p_style, p_style_from, &style_selector_all);
    }

    return(STATUS_OK);
}

T5_MSG_PROTO(static, implied_style_ext_style_cell_current, P_IMPLIED_STYLE_QUERY p_implied_style_query)
{
    const PC_STYLE_DOCU_AREA p_style_docu_area = p_implied_style_query->p_style_docu_area;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(slr_equal(&p_implied_style_query->position.slr, &p_docu->cur.slr))
    {
        const PC_STYLE p_style_from = p_style_from_docu_area(p_docu, p_style_docu_area);
        PTR_ASSERT(p_style_from);
        if(NULL != p_style_from)
            style_copy(p_implied_style_query->p_style, p_style_from, &style_selector_all);
    }

    return(STATUS_OK);
}

T5_MSG_PROTO(static, implied_style_ext_style_stripe_cols, P_IMPLIED_STYLE_QUERY p_implied_style_query)
{
    const PC_STYLE_DOCU_AREA p_style_docu_area = p_implied_style_query->p_style_docu_area;
    const U32 arg = p_implied_style_query->arg;
    const int offset = (int) (arg & 0xFF);
    const int repeat = (int) ((arg & 0xFF000000) >> 24) + 2;
    const COL col = p_style_docu_area->docu_area.whole_row ? 0 : p_style_docu_area->docu_area.tl.slr.col;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    /* I had this idea that it should be docu_area tl relative to preserve appearance as it was moved around */
    if(0 == ((int) (p_implied_style_query->position.slr.col - col) - offset) % repeat)
    {
        const PC_STYLE p_style_from = p_style_from_docu_area(p_docu, p_style_docu_area);
        PTR_ASSERT(p_style_from);
        if(NULL != p_style_from)
            style_copy(p_implied_style_query->p_style, p_style_from, &style_selector_all);
    }

    return(STATUS_OK);
}

T5_MSG_PROTO(static, implied_style_ext_style_stripe_rows, P_IMPLIED_STYLE_QUERY p_implied_style_query)
{
    const PC_STYLE_DOCU_AREA p_style_docu_area = p_implied_style_query->p_style_docu_area;
    const U32 arg = p_implied_style_query->arg;
    const int offset = (int) (arg & 0xFF);
    const int repeat = (int) ((arg & 0xFF000000) >> 24) + 2;
    const ROW row = p_style_docu_area->docu_area.whole_col ? 0 : p_style_docu_area->docu_area.tl.slr.row;

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    /* I had this idea that it should be docu_area tl relative to preserve appearance as it was moved around */
    if(0 == ((int) (p_implied_style_query->position.slr.row - row) - offset) % repeat)
    {
        const PC_STYLE p_style_from = p_style_from_docu_area(p_docu, p_style_docu_area);
        PTR_ASSERT(p_style_from);
        if(NULL != p_style_from)
            style_copy(p_implied_style_query->p_style, p_style_from, &style_selector_all);
    }

    return(STATUS_OK);
}

OBJECT_PROTO(extern, object_implied_style);
OBJECT_PROTO(extern, object_implied_style)
{
    switch(t5_message)
    {
    case T5_EXT_STYLE_CELL_TYPE:
        return(implied_style_ext_style_cell_type(p_docu, t5_message, (P_IMPLIED_STYLE_QUERY) p_data));

    case T5_EXT_STYLE_CELL_CURRENT:
        return(implied_style_ext_style_cell_current(p_docu, t5_message, (P_IMPLIED_STYLE_QUERY) p_data));

    case T5_EXT_STYLE_STRIPE_COLS:
        return(implied_style_ext_style_stripe_cols(p_docu, t5_message, (P_IMPLIED_STYLE_QUERY) p_data));

    case T5_EXT_STYLE_STRIPE_ROWS:
        return(implied_style_ext_style_stripe_rows(p_docu, t5_message, (P_IMPLIED_STYLE_QUERY) p_data));

    default:
        return(STATUS_OK);
    }
}

/* end of ob_imps.c */
