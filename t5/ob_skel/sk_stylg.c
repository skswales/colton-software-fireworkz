/* sk_stylg.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Style routines for grid */

/* MRJC November 1992 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

/*
internal routines
*/

static void
style_slr_grid_get(
    _DocuRef_   P_DOCU p_docu,
    P_GRID_SLOT p_grid_slot,
    P_STYLE_DOCU_AREA p_style_docu_area,
    _InVal_     S32 stack_level);

/******************************************************************************
*
* get grid bits into the grid array for a region
*
******************************************************************************/

static void
style_grid_array_from_region(
    _DocuRef_   P_DOCU p_docu,
    P_GRID_BLOCK p_grid_block,
    _InRef_     PC_REGION p_region)
{
    P_STYLE_DOCU_AREA p_style_docu_area;
    ARRAY_INDEX style_docu_area_ix, stack_level = 0;
    S32 n_slots, n_slots_per_row;
    P_GRID_SLOT p_grid_slot;

    n_slots_per_row = p_grid_block->region.br.col - p_grid_block->region.tl.col;
    n_slots = n_slots_per_row * (p_region->br.row - p_region->tl.row);
    assert(p_grid_block->h_grid_array);
    p_grid_slot = array_ptr(&p_grid_block->h_grid_array, GRID_SLOT,
                            n_slots_per_row * (p_region->tl.row - p_grid_block->region.tl.row));

    /* look through region list backwards for intersecting regions */
    for(style_docu_area_ix = array_elements(&p_docu->h_style_docu_area) - 1,
        p_style_docu_area = array_ptr(&p_docu->h_style_docu_area, STYLE_DOCU_AREA, style_docu_area_ix);
        style_docu_area_ix >= 0;
        --style_docu_area_ix, --p_style_docu_area, stack_level += 1)
    {
        if(p_style_docu_area->is_deleted)
            continue;

        if(region_intersect_docu_area(&p_style_docu_area->docu_area, p_region))
        {
            STYLE_SELECTOR temp;

            if(style_docu_area_selector_and(p_docu, &temp, p_style_docu_area, &style_selector_para_grid))
            {
                S32 i;
                P_GRID_SLOT p_grid_slot_i;
                STYLE_SELECTOR selector;

                style_selector_clear(&selector);

                for(i = 0, p_grid_slot_i = p_grid_slot; i < n_slots; ++i, ++p_grid_slot_i)
                {
                    if( (p_grid_slot_i->slr.col >= 0) &&
                        (p_grid_slot_i->slr.row >= 0) &&
                        (p_grid_slot_i->slr.col < n_cols_logical(p_docu)) &&
                        (p_grid_slot_i->slr.row < n_rows(p_docu)) )
                    {
                        style_slr_grid_get(p_docu, p_grid_slot_i, p_style_docu_area, stack_level);
                        void_style_selector_or(&selector, &selector, &p_grid_slot_i->selector);
                    }
                }

                if(!style_selector_any(&selector))
                    break;
            }
        }
    }
}

/******************************************************************************
*
* initialise a portion of grid array
*
******************************************************************************/

static void
style_grid_array_init(
    P_GRID_BLOCK p_grid_block,
    _InRef_     PC_REGION p_region,
    _InVal_     PAGE_FLAGS page_flags)
{
    SLR slr;
    P_GRID_SLOT p_grid_slot;

    assert(p_grid_block->h_grid_array);
    p_grid_slot = array_ptr(&p_grid_block->h_grid_array, GRID_SLOT, (p_grid_block->region.br.col - p_grid_block->region.tl.col) *
                                                                      (p_region->tl.row - p_grid_block->region.tl.row));
    /* initialise grid array */
    slr.row = p_region->tl.row;
    while(slr.row < p_region->br.row)
    {
        slr.col = p_region->tl.col;
        while(slr.col < p_region->br.col)
        {
            S32 i;

            for(i = 0; i < IX_GRID_COUNT; ++i)
            {
                p_grid_slot->grid_element[i].level = S32_MAX;
                p_grid_slot->grid_element[i].rgb_level = S32_MAX;
            }

            if( ((slr.row     == p_grid_block->region.tl.row) && page_flags.first_row_on_page) ||
                ((slr.col     == p_grid_block->region.tl.col) && page_flags.first_col_on_page) ||
                ((slr.row + 1 == p_grid_block->region.br.row) && page_flags.last_row_on_page)  ||
                ((slr.col + 1 == p_grid_block->region.br.col) && page_flags.last_col_on_page)  )
            {
                p_grid_slot->slr.col = -1;
                p_grid_slot->slr.row = -1;
            }
            else
                p_grid_slot->slr = slr;

            style_selector_copy(&p_grid_slot->selector, &style_selector_para_grid);

            ++slr.col;
            ++p_grid_slot;
        }

        ++slr.row;
    }
}

/******************************************************************************
*
* dispose of grid_block resources
*
******************************************************************************/

extern void
style_grid_block_dispose(
    P_GRID_BLOCK p_grid_block)
{
    al_array_dispose(&p_grid_block->h_grid_array);
}

/******************************************************************************
*
* start a new grid block for a given region
*
******************************************************************************/

_Check_return_
extern STATUS
style_grid_block_for_region_new(
    _DocuRef_   P_DOCU p_docu,
    P_GRID_BLOCK p_grid_block,
    _InRef_     PC_REGION p_region,
    _In_        PAGE_FLAGS page_flags)
{
    p_grid_block->region = *p_region;

    /* grid has to consider adjacent cells... */
    p_grid_block->region.tl.col -= 1;
    p_grid_block->region.br.col += 1;
    p_grid_block->region.tl.row -= 1;
    p_grid_block->region.br.row += 1;

    assert(p_grid_block->region.br.col - p_grid_block->region.tl.col > 2);
    assert(p_grid_block->region.br.row - p_grid_block->region.tl.row > 2);

    {
    STATUS status;
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(GRID_SLOT), FALSE);
    if(NULL == al_array_alloc(&p_grid_block->h_grid_array,
                              GRID_SLOT,
                              (p_grid_block->region.br.col - p_grid_block->region.tl.col)
                              *
                              (p_grid_block->region.br.row - p_grid_block->region.tl.row),
                              &array_init_block, &status))
        return(status);
    } /*block*/

    style_grid_array_init(p_grid_block, &p_grid_block->region, page_flags);
    style_grid_array_from_region(p_docu, p_grid_block, &p_grid_block->region);

    return(STATUS_OK);
}

/******************************************************************************
*
* initialise grid block
*
******************************************************************************/

extern void
style_grid_block_init(
    P_GRID_BLOCK p_grid_block)
{
    p_grid_block->h_grid_array = 0;
}

/******************************************************************************
*
* array of slot information for working out grids:
*
*   L     M      R
*     |       |
*   0 |   1   |  2   T
*     |       |
* ----+-------+----
*     |       |
*   3 |   4   |  5   M
*     |       |
* ----+-------+-----
*     |       |
*     |   7   |      B
*     |       |
*
******************************************************************************/

/*
define offsets from current slot in grid array
*/

#define SLOT_LT (-n_slots_per_row - 1)          /* left top */
#define SLOT_MT  -n_slots_per_row               /* middle top */
#define SLOT_RT (-n_slots_per_row + 1)          /* right top */
#define SLOT_LM  -1                             /* left middle */
#define SLOT_MM   0                             /* middle middle */
#define SLOT_RM   1                             /* right middle */
#define SLOT_MB   n_slots_per_row               /* middle bottom */

/******************************************************************************
*
* compare two grid line styles
*
******************************************************************************/

_Check_return_
static inline BOOL
style_grid_line_style_compare(
    _InRef_     PC_GRID_LINE_STYLE p_grid_line_style1,
    _InRef_     PC_GRID_LINE_STYLE p_grid_line_style2)
{
    if(p_grid_line_style1->border_line_flags.border_style != p_grid_line_style2->border_line_flags.border_style)
        return(TRUE);

    return(rgb_compare_not_equals(&p_grid_line_style1->rgb, &p_grid_line_style2->rgb));
}

/******************************************************************************
*
* set the faint grid line style
*
******************************************************************************/

#define COLOUR_OF_FAINT_GRID 1 /* restore this!!! */

static inline void
grid_line_style_set_faint_grid(
    _InoutRef_  P_GRID_LINE_STYLE p_grid_line_style)
{
    assert(p_grid_line_style->border_line_flags.border_style == SF_BORDER_NONE);
    p_grid_line_style->border_line_flags.border_style = SF_BORDER_THIN;
    assert(p_grid_line_style->border_line_flags.border_style < SF_BORDER_COUNT);
    p_grid_line_style->rgb = rgb_stash[COLOUR_OF_FAINT_GRID]; /* maybe read from UI.FaintGrid style in future */
}

/******************************************************************************
*
* process a horizontal line segment from a grid block
*
******************************************************************************/

static void
style_grid_from_grid_slot_h(
    _OutRef_    P_GRID_LINE_STYLE p_grid_line_style,
    P_GRID_SLOT p_grid_slot,
    _InVal_     S32 n_slots_per_row,
    _In_        GRID_FLAGS grid_flags)
{
    zero_struct_ptr(p_grid_line_style);

    if(!grid_flags.outer_edge)
    {
        P_GRID_SLOT p_grid_slot_lt = &p_grid_slot[SLOT_LT];
        P_GRID_SLOT p_grid_slot_mt = &p_grid_slot[SLOT_MT];
        P_GRID_SLOT p_grid_slot_lm = &p_grid_slot[SLOT_LM];
        P_GRID_SLOT p_grid_slot_mm = &p_grid_slot[SLOT_MM];
        S32 level = p_grid_slot_mm->grid_element[IX_GRID_TOP].level;

        p_grid_line_style->border_line_flags.border_style = p_grid_slot_mm->grid_element[IX_GRID_TOP].type;
        assert(p_grid_line_style->border_line_flags.border_style < SF_BORDER_COUNT);

        p_grid_line_style->rgb = p_grid_slot_mm->grid_element[IX_GRID_TOP].rgb;

        if(p_grid_slot_mt->grid_element[IX_GRID_BOTTOM].level < level)
        {
            p_grid_line_style->border_line_flags.border_style = p_grid_slot_mt->grid_element[IX_GRID_BOTTOM].type;
            assert(p_grid_line_style->border_line_flags.border_style < SF_BORDER_COUNT);
            level = p_grid_slot_mt->grid_element[IX_GRID_BOTTOM].level;
        }

        if(p_grid_slot_mt->grid_element[IX_GRID_BOTTOM].rgb_level < p_grid_slot_mm->grid_element[IX_GRID_TOP].rgb_level)
            p_grid_line_style->rgb = p_grid_slot_mt->grid_element[IX_GRID_BOTTOM].rgb;

        if( grid_flags.faint_grid && (p_grid_line_style->border_line_flags.border_style == SF_BORDER_NONE) )
        {
            grid_line_style_set_faint_grid(p_grid_line_style);
            level = S32_MAX;
        }
        else
        {
            p_grid_line_style->border_line_flags.add_lw_to_r = 1;
        }

        if( ((p_grid_slot_lt->grid_element[IX_GRID_RIGHT].level < level) &&
             (p_grid_slot_lt->grid_element[IX_GRID_RIGHT].type != SF_BORDER_NONE))
           ||
            ((p_grid_slot_lt->grid_element[IX_GRID_BOTTOM].level < level) &&
             (p_grid_slot_lt->grid_element[IX_GRID_BOTTOM].type != SF_BORDER_NONE))
           ||
            ((p_grid_slot_mt->grid_element[IX_GRID_LEFT].level < level) &&
             (p_grid_slot_mt->grid_element[IX_GRID_LEFT].type != SF_BORDER_NONE))
           ||
            ((p_grid_slot_lm->grid_element[IX_GRID_TOP].level < level) &&
             (p_grid_slot_lm->grid_element[IX_GRID_TOP].type != SF_BORDER_NONE)) )
        {
            p_grid_line_style->border_line_flags.add_lw_to_l = 1;
        }
    }
    else
    {
        P_GRID_SLOT p_grid_slot_mb = &p_grid_slot[SLOT_MB];
        P_GRID_SLOT p_grid_slot_lm = &p_grid_slot[SLOT_LM];
        P_GRID_SLOT p_grid_slot_mm = &p_grid_slot[SLOT_MM];
        S32 level = p_grid_slot_mm->grid_element[IX_GRID_BOTTOM].level;

        p_grid_line_style->border_line_flags.border_style = p_grid_slot_mm->grid_element[IX_GRID_BOTTOM].type;
        assert(p_grid_line_style->border_line_flags.border_style < SF_BORDER_COUNT);
        p_grid_line_style->rgb = p_grid_slot_mm->grid_element[IX_GRID_BOTTOM].rgb;
        p_grid_line_style->border_line_flags.add_lw_to_r = 1;

        if(p_grid_slot_mb->grid_element[IX_GRID_TOP].level < level)
        {
            p_grid_line_style->border_line_flags.border_style = p_grid_slot_mb->grid_element[IX_GRID_TOP].type;
            assert(p_grid_line_style->border_line_flags.border_style < SF_BORDER_COUNT);
            level = p_grid_slot_mb->grid_element[IX_GRID_TOP].level;
        }

        if(p_grid_slot_mb->grid_element[IX_GRID_TOP].rgb_level < p_grid_slot_mm->grid_element[IX_GRID_BOTTOM].rgb_level)
            p_grid_line_style->rgb = p_grid_slot_mb->grid_element[IX_GRID_TOP].rgb;

        if( grid_flags.faint_grid && (p_grid_line_style->border_line_flags.border_style == SF_BORDER_NONE) )
        {
            grid_line_style_set_faint_grid(p_grid_line_style);
            level = S32_MAX;
        }
        else
        {
            p_grid_line_style->border_line_flags.add_lw_to_r = 1;
        }

        if( ((p_grid_slot_lm->grid_element[IX_GRID_RIGHT].level < level) &&
             (p_grid_slot_lm->grid_element[IX_GRID_RIGHT].type != SF_BORDER_NONE))
           ||
            ((p_grid_slot_lm->grid_element[IX_GRID_BOTTOM].level < level) &&
             (p_grid_slot_lm->grid_element[IX_GRID_BOTTOM].type != SF_BORDER_NONE)) )
        {
            p_grid_line_style->border_line_flags.add_lw_to_l = 1;
        }
    }
}

/******************************************************************************
*
* process a vertical line segment from a grid block
*
******************************************************************************/

static void
style_grid_from_grid_slot_v(
    _OutRef_    P_GRID_LINE_STYLE p_grid_line_style,
    P_GRID_SLOT p_grid_slot,
    _InVal_     S32 n_slots_per_row,
    _In_        GRID_FLAGS grid_flags)
{
    zero_struct_ptr(p_grid_line_style);

    if(!grid_flags.outer_edge)
    {
        P_GRID_SLOT p_grid_slot_lt = &p_grid_slot[SLOT_LT];
        P_GRID_SLOT p_grid_slot_mt = &p_grid_slot[SLOT_MT];
        P_GRID_SLOT p_grid_slot_lm = &p_grid_slot[SLOT_LM];
        P_GRID_SLOT p_grid_slot_mm = &p_grid_slot[SLOT_MM];
        S32 level = p_grid_slot_mm->grid_element[IX_GRID_LEFT].level;

        p_grid_line_style->border_line_flags.border_style = p_grid_slot_mm->grid_element[IX_GRID_LEFT].type;
        p_grid_line_style->rgb = p_grid_slot_mm->grid_element[IX_GRID_LEFT].rgb;

        if(p_grid_slot_lm->grid_element[IX_GRID_RIGHT].level < level)
        {
            p_grid_line_style->border_line_flags.border_style = p_grid_slot_lm->grid_element[IX_GRID_RIGHT].type;
            level = p_grid_slot_lm->grid_element[IX_GRID_RIGHT].level;
        }

        if(p_grid_slot_lm->grid_element[IX_GRID_RIGHT].rgb_level < p_grid_slot_mm->grid_element[IX_GRID_LEFT].rgb_level)
            p_grid_line_style->rgb = p_grid_slot_lm->grid_element[IX_GRID_RIGHT].rgb;

        if( grid_flags.faint_grid && (p_grid_line_style->border_line_flags.border_style == SF_BORDER_NONE) )
        {
            grid_line_style_set_faint_grid(p_grid_line_style);
            level = S32_MAX;
        }
        else
        {
            p_grid_line_style->border_line_flags.add_lw_to_b = 1;
        }

        /* check if any of the grid lines that meet at this point have higher priority */
        if( ((p_grid_slot_lt->grid_element[IX_GRID_RIGHT].level < level) &&
             (p_grid_slot_lt->grid_element[IX_GRID_RIGHT].type != SF_BORDER_NONE))
           ||
            ((p_grid_slot_lt->grid_element[IX_GRID_BOTTOM].level < level) &&
             (p_grid_slot_lt->grid_element[IX_GRID_BOTTOM].type != SF_BORDER_NONE))
           ||
            ((p_grid_slot_mt->grid_element[IX_GRID_LEFT].level < level) &&
             (p_grid_slot_mt->grid_element[IX_GRID_LEFT].type != SF_BORDER_NONE))
           ||
            ((p_grid_slot_lm->grid_element[IX_GRID_TOP].level < level) &&
             (p_grid_slot_lm->grid_element[IX_GRID_TOP].type != SF_BORDER_NONE)) )
        {
            p_grid_line_style->border_line_flags.add_lw_to_t = 1;
        }
    }
    else
    {
        P_GRID_SLOT p_grid_slot_rt = &p_grid_slot[SLOT_RT];
        P_GRID_SLOT p_grid_slot_mt = &p_grid_slot[SLOT_MT];
        P_GRID_SLOT p_grid_slot_rm = &p_grid_slot[SLOT_RM];
        P_GRID_SLOT p_grid_slot_mm = &p_grid_slot[SLOT_MM];
        S32 level = p_grid_slot_mm->grid_element[IX_GRID_RIGHT].level;

        p_grid_line_style->border_line_flags.border_style = p_grid_slot_mm->grid_element[IX_GRID_RIGHT].type;
        p_grid_line_style->rgb = p_grid_slot_mm->grid_element[IX_GRID_RIGHT].rgb;

        if(p_grid_slot_rm->grid_element[IX_GRID_LEFT].level < level)
        {
            p_grid_line_style->border_line_flags.border_style = p_grid_slot_rm->grid_element[IX_GRID_LEFT].type;
            level = p_grid_slot_rm->grid_element[IX_GRID_LEFT].level;
        }

        if(p_grid_slot_rm->grid_element[IX_GRID_LEFT].rgb_level < p_grid_slot_mm->grid_element[IX_GRID_RIGHT].rgb_level)
            p_grid_line_style->rgb = p_grid_slot_rm->grid_element[IX_GRID_LEFT].rgb;

        if( grid_flags.faint_grid && (p_grid_line_style->border_line_flags.border_style == SF_BORDER_NONE) )
        {
            grid_line_style_set_faint_grid(p_grid_line_style);
            level = S32_MAX;
        }
        else
        {
            p_grid_line_style->border_line_flags.add_lw_to_b = 1;
        }

        if( ((p_grid_slot_mt->grid_element[IX_GRID_RIGHT].level < level) &&
             (p_grid_slot_mt->grid_element[IX_GRID_RIGHT].type != SF_BORDER_NONE))
           ||
            ((p_grid_slot_mt->grid_element[IX_GRID_BOTTOM].level < level) &&
             (p_grid_slot_mt->grid_element[IX_GRID_BOTTOM].type != SF_BORDER_NONE))
           ||
            ((p_grid_slot_rt->grid_element[IX_GRID_LEFT].level < level) &&
             (p_grid_slot_rt->grid_element[IX_GRID_LEFT].type != SF_BORDER_NONE)) )
        {
            p_grid_line_style->border_line_flags.add_lw_to_t = 1;
        }
    }
}

/******************************************************************************
*
* accumulate horizontal grid segments
*
******************************************************************************/

extern COL
style_grid_from_grid_block_h(
    _OutRef_    P_GRID_LINE_STYLE p_grid_line_style,
    P_GRID_BLOCK p_grid_block,
    _InVal_     COL col,
    _InVal_     ROW row,
    _In_        GRID_FLAGS grid_flags)
{
    SLR offset;
    S32 n_slots_per_row;
    P_GRID_SLOT p_grid_slot;
    COL col_i = col;

    assert(p_grid_block->h_grid_array);

    n_slots_per_row = p_grid_block->region.br.col - p_grid_block->region.tl.col;

    offset.col = col - p_grid_block->region.tl.col;
    offset.row = row - p_grid_block->region.tl.row;

    p_grid_slot = array_ptr(&p_grid_block->h_grid_array, GRID_SLOT, (offset.row * n_slots_per_row) + offset.col);

    style_grid_from_grid_slot_h(p_grid_line_style, p_grid_slot, n_slots_per_row, grid_flags);
    assert(p_grid_line_style->border_line_flags.border_style < SF_BORDER_COUNT);

    /* step down rows looking for same line style */
    while(++col_i < p_grid_block->region.br.col - 1)
    {
        GRID_LINE_STYLE grid_line_style;

        p_grid_slot += 1;
        style_grid_from_grid_slot_h(&grid_line_style, p_grid_slot, n_slots_per_row, grid_flags);
        assert(grid_line_style.border_line_flags.border_style < SF_BORDER_COUNT);

        if(style_grid_line_style_compare(p_grid_line_style, &grid_line_style))
            break;
    }

    return(col_i);
}

/******************************************************************************
*
* accumulate vertical grid segments
*
******************************************************************************/

extern ROW
style_grid_from_grid_block_v(
    _OutRef_    P_GRID_LINE_STYLE p_grid_line_style,
    P_GRID_BLOCK p_grid_block,
    _InVal_     COL col,
    _InVal_     ROW row,
    _In_        GRID_FLAGS grid_flags)
{
    SLR offset;
    S32 n_slots_per_row;
    P_GRID_SLOT p_grid_slot;
    ROW row_i = row;

    assert(p_grid_block->h_grid_array);

    n_slots_per_row = p_grid_block->region.br.col - p_grid_block->region.tl.col;

    offset.col = col - p_grid_block->region.tl.col;
    offset.row = row - p_grid_block->region.tl.row;

    p_grid_slot = array_ptr(&p_grid_block->h_grid_array, GRID_SLOT, (offset.row * n_slots_per_row) + offset.col);

    style_grid_from_grid_slot_v(p_grid_line_style, p_grid_slot, n_slots_per_row, grid_flags);

    /* step down rows looking for same line style */
    while(++row_i < p_grid_block->region.br.row - 1)
    {
        GRID_LINE_STYLE grid_line_style;

        p_grid_slot += n_slots_per_row;
        style_grid_from_grid_slot_v(&grid_line_style, p_grid_slot, n_slots_per_row, grid_flags);
        if(style_grid_line_style_compare(p_grid_line_style, &grid_line_style))
            break;
    }

    return(row_i);
}

/******************************************************************************
*
* get grid effects from a style region record
* for a given slot
*
******************************************************************************/

static void
style_slr_grid_get(
    _DocuRef_   P_DOCU p_docu,
    P_GRID_SLOT p_grid_slot,
    P_STYLE_DOCU_AREA p_style_docu_area,
    _InVal_     S32 stack_level)
{
    if(slr_in_docu_area(&p_style_docu_area->docu_area, &p_grid_slot->slr))
    {
        STYLE_SELECTOR selector_temp;
        STYLE style;
        PC_STYLE p_style;

        {
        POSITION position;
        position.slr = p_grid_slot->slr;
        object_position_init(&position.object_position);
        if(style_implied_query(p_docu, &style, p_style_docu_area, &position))
            p_style = &style;
        else
            p_style = p_style_from_docu_area(p_docu, p_style_docu_area);
        } /*block*/

        if((NULL != p_style) && style_selector_and(&selector_temp, &p_style->selector, &p_grid_slot->selector))
        {
            if(style_selector_bit_test(&selector_temp, STYLE_SW_PS_RGB_GRID_LEFT))
            {
                p_grid_slot->grid_element[IX_GRID_LEFT].rgb = p_style->para_style.rgb_grid_left;
                p_grid_slot->grid_element[IX_GRID_LEFT].rgb_level = stack_level;
            }
            if(style_selector_bit_test(&selector_temp, STYLE_SW_PS_RGB_GRID_TOP))
            {
                p_grid_slot->grid_element[IX_GRID_TOP].rgb = p_style->para_style.rgb_grid_top;
                p_grid_slot->grid_element[IX_GRID_TOP].rgb_level = stack_level;
            }
            if(style_selector_bit_test(&selector_temp, STYLE_SW_PS_RGB_GRID_RIGHT))
            {
                p_grid_slot->grid_element[IX_GRID_RIGHT].rgb = p_style->para_style.rgb_grid_right;
                p_grid_slot->grid_element[IX_GRID_RIGHT].rgb_level = stack_level;
            }
            if(style_selector_bit_test(&selector_temp, STYLE_SW_PS_RGB_GRID_BOTTOM))
            {
                p_grid_slot->grid_element[IX_GRID_BOTTOM].rgb = p_style->para_style.rgb_grid_bottom;
                p_grid_slot->grid_element[IX_GRID_BOTTOM].rgb_level = stack_level;
            }

            if(style_selector_bit_test(&selector_temp, STYLE_SW_PS_GRID_LEFT))
            {
                p_grid_slot->grid_element[IX_GRID_LEFT].type = p_style->para_style.grid_left;
                p_grid_slot->grid_element[IX_GRID_LEFT].level = stack_level;
                assert(p_grid_slot->grid_element[IX_GRID_LEFT].type < SF_BORDER_COUNT);
            }
            if(style_selector_bit_test(&selector_temp, STYLE_SW_PS_GRID_TOP))
            {
                p_grid_slot->grid_element[IX_GRID_TOP].type = p_style->para_style.grid_top;
                p_grid_slot->grid_element[IX_GRID_TOP].level = stack_level;
                assert(p_grid_slot->grid_element[IX_GRID_TOP].type < SF_BORDER_COUNT);
            }
            if(style_selector_bit_test(&selector_temp, STYLE_SW_PS_GRID_RIGHT))
            {
                p_grid_slot->grid_element[IX_GRID_RIGHT].type = p_style->para_style.grid_right;
                p_grid_slot->grid_element[IX_GRID_RIGHT].level = stack_level;
                assert(p_grid_slot->grid_element[IX_GRID_RIGHT].type < SF_BORDER_COUNT);
            }
            if(style_selector_bit_test(&selector_temp, STYLE_SW_PS_GRID_BOTTOM))
            {
                p_grid_slot->grid_element[IX_GRID_BOTTOM].type = p_style->para_style.grid_bottom;
                p_grid_slot->grid_element[IX_GRID_BOTTOM].level = stack_level;
                assert(p_grid_slot->grid_element[IX_GRID_BOTTOM].type < SF_BORDER_COUNT);
            }

            /* clear bits we found */
            void_style_selector_bic(&p_grid_slot->selector, &p_grid_slot->selector, &selector_temp);
        }
    }
}

/* end of sk_stylg.c */
