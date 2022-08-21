/* view.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* RCM Dec 1991 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#ifndef         __vi_edge_h
#include "ob_skel/vi_edge.h"
#endif

#if RISCOS
#include "ob_skel/xp_skelr.h"
#endif

/*
reverse
*/

typedef struct CTRL_SKEL_VIEW
{
    CTRL_VIEW_SLICE slice;
}
CTRL_SKEL_VIEW; typedef const CTRL_SKEL_VIEW * PC_CTRL_SKEL_VIEW;

/*
colour control structure passed to send_redraw_event_to_skel()
*/

typedef struct REDRAW_COLOURS
{
    PC_RGB p_desk_colour;
    PC_RGB p_margin_colour;
    PC_RGB p_paper_colour;
}
REDRAW_COLOURS, * P_REDRAW_COLOURS;

/*
describe a page
*/

typedef struct PAGE_INFO
{
    HEADFOOT_BOTH headfoot_both;
    PAGE_DEF page_def;
    PAGE_NUM page_num;

    PIXIT_RECT paper_area;
    PIXIT_RECT print_area;
    PIXIT_RECT cells_area;

    PIXIT_RECT header_area;
    PIXIT_RECT footer_area;

    PIXIT_RECT col_area;
    PIXIT_RECT row_area;
}
PAGE_INFO, * P_PAGE_INFO;

typedef const CTRL_VIEW_SKEL * PC_CTRL_VIEW_SKEL;

typedef const REDRAW_TAG_AND_EVENT * PC_REDRAW_TAG_AND_EVENT;

_Check_return_
static STATUS
send_redraw_event_to_skel(
    _DocuRef_   P_DOCU p_docu,
    P_VIEWEVENT_REDRAW p_viewevent_redraw,
    _InRef_     PC_CTRL_VIEW_SKEL p_ctrl_view_skel,
    P_REDRAW_COLOURS p_redraw_colours);

static void
try_calling(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_SKELEVENT_REDRAW p_skelevent_redraw,
    _InRef_     PC_PIXIT_RECT p_work_area,
    _InRef_     PC_PIXIT_RECT p_clip_area,
    _InRef_     PC_REDRAW_TAG_AND_EVENT p_redraw_tag_and_event
    CHECKING_ONLY_ARG(PCTSTR area));

/******************************************************************************
*
* back_window : x and y both uncut, no redraw handler
* pane_window : x and y both cut, redraws are printable area relative
* ruler_horz  : x cut but not y, redraws are printable area relative
* border_vert : y cut but not x, redraws are printable area relative
* border_horz : x cut but not y, redraws are printable area relative
*
******************************************************************************/

/* now installed dynamically */

static /*poked*/ CTRL_VIEW_SKEL
ctrl_view_skel_back_window =
{
    { FALSE, FALSE },

    { UPDATE_BACK_WINDOW },
    { UPDATE_BACK_WINDOW },
    { UPDATE_BACK_WINDOW },
    { UPDATE_BACK_WINDOW },
    { UPDATE_BACK_WINDOW },
    { UPDATE_BACK_WINDOW },
    { UPDATE_BACK_WINDOW },

    { UPDATE_BACK_WINDOW },
    { UPDATE_BACK_WINDOW },
    { UPDATE_BACK_WINDOW },
    { UPDATE_BACK_WINDOW }
};

static const CTRL_VIEW_SKEL
ctrl_view_skel_border_horz =
{
    { TRUE,  FALSE },

    { UPDATE_BORDER_HORZ },
    { UPDATE_BORDER_HORZ },
    { UPDATE_BORDER_HORZ },
    { UPDATE_BORDER_HORZ, edge_window_event_border_horz },
    { UPDATE_BORDER_HORZ },
    { UPDATE_BORDER_HORZ },
    { UPDATE_BORDER_HORZ },

    { UPDATE_BORDER_HORZ },
    { UPDATE_BORDER_HORZ },
    { UPDATE_BORDER_HORZ },
    { UPDATE_BORDER_HORZ }
};

static const CTRL_VIEW_SKEL
ctrl_view_skel_border_vert =
{
    { FALSE, TRUE  },

    { UPDATE_BORDER_VERT },
    { UPDATE_BORDER_VERT },
    { UPDATE_BORDER_VERT },
    { UPDATE_BORDER_VERT, edge_window_event_border_vert },
    { UPDATE_BORDER_VERT },
    { UPDATE_BORDER_VERT },
    { UPDATE_BORDER_VERT },

    { UPDATE_BORDER_VERT },
    { UPDATE_BORDER_VERT },
    { UPDATE_BORDER_VERT },
    { UPDATE_BORDER_VERT }
};

/* now installed dynamically */

static /*poked*/ CTRL_VIEW_SKEL
ctrl_view_skel_ruler_horz =
{
    { TRUE,  FALSE },

    { UPDATE_RULER_HORZ },
    { UPDATE_RULER_HORZ },
    { UPDATE_RULER_HORZ },
    { UPDATE_RULER_HORZ },
    { UPDATE_RULER_HORZ },
    { UPDATE_RULER_HORZ },
    { UPDATE_RULER_HORZ },

    { UPDATE_RULER_HORZ },
    { UPDATE_RULER_HORZ },
    { UPDATE_RULER_HORZ },
    { UPDATE_RULER_HORZ }
};

/* now installed dynamically */

static /*poked*/ CTRL_VIEW_SKEL
ctrl_view_skel_ruler_vert =
{
    { FALSE, TRUE  },

    { UPDATE_RULER_VERT },
    { UPDATE_RULER_VERT },
    { UPDATE_RULER_VERT },
    { UPDATE_RULER_VERT },
    { UPDATE_RULER_VERT },
    { UPDATE_RULER_VERT },
    { UPDATE_RULER_VERT },

    { UPDATE_RULER_VERT },
    { UPDATE_RULER_VERT },
    { UPDATE_RULER_VERT },
    { UPDATE_RULER_VERT }
};

static const CTRL_SKEL_VIEW
ctrl_skel_view[/*modified_redraw_tag*/] =
{
    { { TRUE,  FALSE } } /* UPDATE_BORDER_HORZ        */,
    { { FALSE, TRUE  } } /* UPDATE_BORDER_VERT        */,
    { { TRUE,  FALSE } } /* UPDATE_RULER_HORZ         */,
    { { FALSE, TRUE  } } /* UPDATE_RULER_VERT         */,
    { { TRUE,  TRUE  } } /* UPDATE_PANE_MARGIN_HEADER */,
    { { TRUE,  TRUE  } } /* UPDATE_PANE_MARGIN_FOOTER */,
    { { TRUE,  TRUE  } } /* UPDATE_PANE_MARGIN_COL    */,
    { { TRUE,  TRUE  } } /* UPDATE_PANE_MARGIN_ROW    */,
    { { TRUE,  TRUE  } } /* UPDATE_PANE_CELLS_AREA    */,
    { { TRUE,  TRUE  } } /* UPDATE_PANE_PRINT_AREA    */,
    { { TRUE,  TRUE  } } /* UPDATE_PANE_PAPER         */,
    { { FALSE, FALSE } } /* UPDATE_BACK_WINDOW        */
};

#define p_ctrl_skel_view_from_redraw_tag(redraw_tag) ( \
    &ctrl_skel_view[(uintptr_t) (redraw_tag) - (uintptr_t) UPDATE_BORDER_HORZ] )

#if defined(NOTE_LAYER) && 0
static LAYER update_now_caller_layer = LAYER_CELLS;
#endif

_Check_return_
static STATUS
viewevent_click_call(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    _InRef_     PC_SKELEVENT_CLICK p_skelevent_click_master,
    _InRef_     PC_PIXIT_RECT p_area,
    const REDRAW_TAG_AND_EVENT * p_redraw_tag_and_event_in,
    /*out*/ P_REDRAW_TAG_AND_EVENT p_redraw_tag_and_event_out)
{
    P_PROC_EVENT p_proc_event = p_redraw_tag_and_event_in->p_proc_event;
    SKELEVENT_CLICK skelevent_click;

    if(NULL == p_proc_event)
        return(STATUS_OK);

    if((p_skelevent_click_master->skel_point.pixit_point.x <  p_area->tl.x) ||
       (p_skelevent_click_master->skel_point.pixit_point.x >= p_area->br.x) ||
       (p_skelevent_click_master->skel_point.pixit_point.y <  p_area->tl.y) ||
       (p_skelevent_click_master->skel_point.pixit_point.y >= p_area->br.y) )
        return(STATUS_OK);

    skelevent_click = *p_skelevent_click_master;

    skelevent_click.click_context.pixit_origin.x += p_area->tl.x;
    skelevent_click.click_context.pixit_origin.y += p_area->tl.y;

    skelevent_click.skel_point.pixit_point.x -= p_area->tl.x;
    skelevent_click.skel_point.pixit_point.y -= p_area->tl.y;

    skelevent_click.work_skel_rect.tl.pixit_point.x = 0;
    skelevent_click.work_skel_rect.tl.pixit_point.y = 0;
    skelevent_click.work_skel_rect.br.pixit_point.x = p_area->br.x - p_area->tl.x;
    skelevent_click.work_skel_rect.br.pixit_point.y = p_area->br.y - p_area->tl.y;

    skelevent_click.work_window = p_redraw_tag_and_event_in->redraw_tag;

    status_return((* p_proc_event) (p_docu, t5_message, &skelevent_click));

    if(skelevent_click.processed)
    {
        *p_redraw_tag_and_event_out = *p_redraw_tag_and_event_in;
        return(STATUS_DONE);
    }

    return(STATUS_OK);
}

static void
adjust_redraw_origin(
    _InoutRef_  P_SKELEVENT_REDRAW p_skelevent_redraw,
    _InVal_     S32 by_x,
    _InVal_     S32 by_y)
{
    /* move origin by (by_x, by_y) */
    p_skelevent_redraw->redraw_context.pixit_origin.x += by_x;
    p_skelevent_redraw->redraw_context.pixit_origin.y += by_y;

    /* and the origin relative clip rectangle by (-by_x, -by_y) */
    p_skelevent_redraw->clip_skel_rect.tl.pixit_point.x -= by_x;
    p_skelevent_redraw->clip_skel_rect.tl.pixit_point.y -= by_y;
    p_skelevent_redraw->clip_skel_rect.br.pixit_point.x -= by_x;
    p_skelevent_redraw->clip_skel_rect.br.pixit_point.y -= by_y;
}

_Check_return_
_Ret_maybenone_
extern P_VIEW
docu_enum_views(
    _DocuRef_   PC_DOCU p_docu,
    _InoutRef_  P_VIEWNO p_viewno /* start me with VIEWNO_NONE */)
{
    if(*p_viewno < VIEWNO_FIRST)
        *p_viewno = VIEWNO_FIRST;
    else
        *p_viewno += 1;

    while(array_index_is_valid(&p_docu->h_view_table, *p_viewno - VIEWNO_FIRST)) /* handle 0 avoidance */
    {
        const P_VIEW p_view = array_ptr_no_checks(&p_docu->h_view_table, VIEW, *p_viewno - VIEWNO_FIRST); /* handle 0 avoidance */

        if(view_void_entry(p_view))
        {
            *p_viewno += 1;
            continue;
        }

        return(p_view);
    }

    *p_viewno = VIEWNO_NONE;
    return(P_VIEW_NONE);
}

static void
page_info_fillin_other(
    _InoutRef_  P_PAGE_INFO p_page_info)
{
    p_page_info->header_area = p_page_info->print_area;
    p_page_info->footer_area = p_page_info->print_area;
    p_page_info->col_area = p_page_info->print_area;
    p_page_info->row_area = p_page_info->print_area;

    p_page_info->header_area.br.y = p_page_info->header_area.tl.y + p_page_info->headfoot_both.header.margin;

    p_page_info->footer_area.tl.y = p_page_info->footer_area.br.y - p_page_info->headfoot_both.footer.margin;

    p_page_info->col_area.tl.x += p_page_info->page_def.margin_row;
    p_page_info->col_area.tl.y += p_page_info->headfoot_both.header.margin;
    p_page_info->col_area.br.y  = p_page_info->col_area.tl.y + p_page_info->page_def.margin_col;

    p_page_info->row_area.tl.y += p_page_info->page_def.margin_col + p_page_info->headfoot_both.header.margin;
    p_page_info->row_area.br.x  = p_page_info->row_area.tl.x + p_page_info->page_def.margin_row;
    p_page_info->row_area.br.y -= p_page_info->headfoot_both.footer.margin;
}

static void
page_info_fillin_print_area(
    _InoutRef_  P_PAGE_INFO p_page_info,
    _OutRef_    P_PIXIT_POINT p_pa_shift)
{
    p_page_info->print_area = p_page_info->paper_area;

    p_pa_shift->x = margin_left_from(&p_page_info->page_def, p_page_info->page_num.y);
    p_pa_shift->y = p_page_info->page_def.margin_top;

    p_page_info->print_area.tl.x += p_pa_shift->x;
    p_page_info->print_area.tl.y += p_pa_shift->y;
    p_page_info->print_area.br.x -= margin_right_from(&p_page_info->page_def, p_page_info->page_num.y);
    p_page_info->print_area.br.y -= p_page_info->page_def.margin_bottom;
}

static void
page_info_fillin_cells_area(
    _InoutRef_  P_PAGE_INFO p_page_info,
    _OutRef_    P_PIXIT_POINT p_sa_shift)
{
    p_page_info->cells_area = p_page_info->print_area;

    p_sa_shift->x = p_page_info->page_def.margin_row;
    p_sa_shift->y = p_page_info->page_def.margin_col + p_page_info->headfoot_both.header.margin;

    p_page_info->cells_area.tl.x += p_sa_shift->x;
    p_page_info->cells_area.tl.y += p_sa_shift->y;
/*  p_page_info->cells_area.br.x -= 0; */
    p_page_info->cells_area.br.y -= p_page_info->headfoot_both.footer.margin;
}

static void
page_info_shift(
    _InoutRef_  P_PAGE_INFO p_page_info,
    _InRef_     PC_PIXIT_POINT p_pa_shift,
    _InRef_opt_ PC_PIXIT_POINT p_sa_shift)
{
    PIXIT_POINT shift = *p_pa_shift;

    if(NULL != p_sa_shift)
    {
        shift.x += p_sa_shift->x;
        shift.y += p_sa_shift->y;
    }

    p_page_info->paper_area.tl.x -= shift.x;
    p_page_info->paper_area.tl.y -= shift.y;
    p_page_info->paper_area.br.x -= shift.x;
    p_page_info->paper_area.br.y -= shift.y;

    p_page_info->print_area.tl.x -= shift.x;
    p_page_info->print_area.tl.y -= shift.y;
    p_page_info->print_area.br.x -= shift.x;
    p_page_info->print_area.br.y -= shift.y;

    p_page_info->cells_area.tl.x -= shift.x;
    p_page_info->cells_area.tl.y -= shift.y;
    p_page_info->cells_area.br.x -= shift.x;
    p_page_info->cells_area.br.y -= shift.y;
}

extern void
page_limits_from_page(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_PIXIT_POINT p_pixit_point,
    _InVal_     REDRAW_TAG redraw_tag,
    _InRef_     PC_PAGE_NUM p_page_num)
{
    PIXIT_RECT work_rect, clip_rect;

    DOCU_ASSERT(p_docu);
    PTR_ASSERT(p_pixit_point);
    PTR_ASSERT(p_page_num);

    /* pass DISPLAY_DESK_AREA as display_mode, as this mode has all header/footer/col/row margins available */
    pixit_rect_from_page(p_docu, &work_rect, &clip_rect, DISPLAY_DESK_AREA, redraw_tag, p_page_num);

    p_pixit_point->x = work_rect.br.x - work_rect.tl.x;
    p_pixit_point->y = work_rect.br.y - work_rect.tl.y;
}

static void
paint_desk_strips(
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_PIXIT_POINT p_grid_step,
    _InRef_     PC_PIXIT_RECT p_covered,
    _InRef_     PC_RGB p_desk_colour)
{
    if(p_grid_step->y)
    {
        PIXIT_RECT h_desk_strip;

        h_desk_strip.tl.x = 0;
        h_desk_strip.br.x = p_grid_step->x ? p_grid_step->x : p_covered->br.x;

        /* paint the strip of desk above the paper */
        h_desk_strip.tl.y = 0;
        h_desk_strip.br.y = p_covered->tl.y;
        host_paint_rectangle_filled(p_redraw_context, &h_desk_strip, p_desk_colour);

        /* paint the strip of desk below the paper */
        h_desk_strip.tl.y = p_covered->br.y;
        h_desk_strip.br.y = p_grid_step->y;
        host_paint_rectangle_filled(p_redraw_context, &h_desk_strip, p_desk_colour);
    }

    if(p_grid_step->x)
    {
        PIXIT_RECT v_desk_strip;

        v_desk_strip.tl.y = p_covered->tl.y;
        v_desk_strip.br.y = p_covered->br.y;

        /* paint the strip of desk to the left of the paper */
        v_desk_strip.tl.x = 0;
        v_desk_strip.br.x = p_covered->tl.x;
        host_paint_rectangle_filled(p_redraw_context, &v_desk_strip, p_desk_colour);

        /* paint the strip of desk to the right of the paper */
        v_desk_strip.tl.x = p_covered->br.x;
        v_desk_strip.br.x = p_grid_step->x;
        host_paint_rectangle_filled(p_redraw_context, &v_desk_strip, p_desk_colour);
    }
}

#if RISCOS
#define desk_margin_left(display_mode) ((DISPLAY_WORK_AREA == (display_mode)) ? (4 * PIXITS_PER_RISCOS) : (12 * PIXITS_PER_RISCOS))
#define desk_margin_top(display_mode)  ((DISPLAY_WORK_AREA == (display_mode)) ? (4 * PIXITS_PER_RISCOS) : (12 * PIXITS_PER_RISCOS))
#elif WINDOWS && 1
#define desk_margin_left(display_mode) ((DISPLAY_WORK_AREA == (display_mode)) ? (2 * PIXITS_PER_PIXEL) : (6 * PIXITS_PER_PIXEL))
#define desk_margin_top(display_mode)  ((DISPLAY_WORK_AREA == (display_mode)) ? (2 * PIXITS_PER_PIXEL) : (6 * PIXITS_PER_PIXEL))
#elif WINDOWS
#define desk_margin_left(display_mode) (8 * PIXITS_PER_PIXEL)
#define desk_margin_top(display_mode)  (8 * PIXITS_PER_PIXEL)
#endif


/*ncr*/
extern BOOL
pixit_rect_from_page(
    _DocuRef_   P_DOCU p_docu,
    P_PIXIT_RECT p_work_rect,
    P_PIXIT_RECT p_clip_rect,
    _InVal_     S32 display_mode,
    _InVal_     REDRAW_TAG redraw_tag,
    _InRef_     PC_PAGE_NUM p_page_num)
{
    BOOL visible = TRUE; /* area is usually visible, cleared if display_mode means it isn't */
    const PC_CTRL_SKEL_VIEW p_ctrl_skel_view = p_ctrl_skel_view_from_redraw_tag(redraw_tag);
    PIXIT desk_lm, desk_tm;
    PAGE_INFO page_info;
    PIXIT_POINT pa_shift, sa_shift;
    PIXIT_RECT display_area;

    page_info.page_def = p_docu->page_def;
    page_info.page_num = *p_page_num;

    if(p_ctrl_skel_view->slice.x)
        desk_lm = desk_margin_left(display_mode);
    else
    {
        desk_lm = 0;
        page_info.page_def.margin_left = page_info.page_def.margin_right = page_info.page_def.margin_bind = page_info.page_def.margin_row = 0;
        page_info.page_def.size_x = HUGENUMBER; /* not p_viewevent_redraw->work_area.rect.br.x; actual window width */
    }

    if(p_ctrl_skel_view->slice.y)
    {
        desk_tm = desk_margin_top(display_mode);
        headfoot_both_from_page_y(p_docu, &page_info.headfoot_both, page_info.page_num.y);
    }
    else
    {
        desk_tm = 0;
        page_info.page_def.margin_top = page_info.page_def.margin_bottom = page_info.page_def.margin_col = 0;
        page_info.page_def.size_y = HUGENUMBER; /* not p_viewevent_redraw->work_area.rect.br.y; actual window height */
        page_info.headfoot_both.header.margin = page_info.headfoot_both.footer.margin = 0;
    }

    page_info.paper_area.tl.x = desk_lm;
    page_info.paper_area.tl.y = desk_tm;
    page_info.paper_area.br.x = page_info.paper_area.tl.x + page_info.page_def.size_x;
    page_info.paper_area.br.y = page_info.paper_area.tl.y + page_info.page_def.size_y;

    page_info_fillin_print_area(&page_info, &pa_shift);
    page_info_fillin_cells_area(&page_info, &sa_shift);

    switch(display_mode)
    {
    default: default_unhandled();
#if CHECKING
    case DISPLAY_DESK_AREA:
    case DISPLAY_PRINT_AREA:
#endif
        {
        if(display_mode == DISPLAY_PRINT_AREA)
        {
            page_info_shift(&page_info, &pa_shift, NULL /*no sa adjust*/);

            display_area = page_info.print_area;
        }
        else
            display_area = page_info.paper_area;

        page_info_fillin_other(&page_info);

        switch(redraw_tag)
        {
        case UPDATE_PANE_MARGIN_HEADER:
            *p_work_rect = page_info.header_area;
            *p_clip_rect = page_info.header_area;
            break;

        case UPDATE_PANE_MARGIN_FOOTER:
            *p_work_rect = page_info.footer_area;
            *p_clip_rect = page_info.footer_area;
            break;

        case UPDATE_PANE_MARGIN_COL:
            *p_work_rect = page_info.col_area;
            *p_clip_rect = page_info.col_area;
            break;

        case UPDATE_PANE_MARGIN_ROW:
            *p_work_rect = page_info.row_area;
            *p_clip_rect = page_info.row_area;
            break;

        case UPDATE_PANE_PRINT_AREA:
        case UPDATE_PANE_PAPER:
        case UPDATE_RULER_HORZ:
        case UPDATE_RULER_VERT:
            /* earlier we set display_area = paper_area or display_area = print_area */
            *p_work_rect = page_info.paper_area;
            *p_clip_rect = display_area;
            break;

        default: default_unhandled();
#if CHECKING
        case UPDATE_BACK_WINDOW:
        case UPDATE_BORDER_HORZ:
        case UPDATE_BORDER_VERT:
        case UPDATE_PANE_CELLS_AREA:
#endif
            *p_work_rect = page_info.cells_area;
            *p_clip_rect = page_info.cells_area;
            break;
        }

        break;
        }

    case DISPLAY_WORK_AREA:
        {
        page_info_shift(&page_info, &pa_shift, &sa_shift);

        switch(redraw_tag)
        {
        case UPDATE_PANE_MARGIN_HEADER:
        case UPDATE_PANE_MARGIN_FOOTER:
        case UPDATE_PANE_MARGIN_COL:
        case UPDATE_PANE_MARGIN_ROW:
            /* area is NOT SWITCHED ON - but we could give its limits as if it was */
            visible = FALSE;
            break;

        case UPDATE_PANE_PRINT_AREA:
        case UPDATE_PANE_PAPER:
        case UPDATE_RULER_HORZ:
        case UPDATE_RULER_VERT:
            *p_work_rect = page_info.paper_area;
            *p_clip_rect = page_info.cells_area;
            break;

        default: default_unhandled();
#if CHECKING
        case UPDATE_BACK_WINDOW:
        case UPDATE_BORDER_HORZ:
        case UPDATE_BORDER_VERT:
        case UPDATE_PANE_CELLS_AREA:
#endif
            *p_work_rect = page_info.cells_area;
            *p_clip_rect = page_info.cells_area;
            break;
        }

        break;
        }
    }

    return(visible);
}

_Check_return_
static STATUS
viewevent_click_process(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    _InRef_     PC_VIEW_POINT p_view_point_in,
    _InRef_     PC_CTRL_VIEW_SKEL p_ctrl_view_skel,
    _InRef_     PC_SKELEVENT_CLICK p_skelevent_click_in,
    /*out*/ P_REDRAW_TAG_AND_EVENT p_redraw_tag_and_event)
{
    const P_VIEW p_view = p_view_point_in->p_view;
    SKELEVENT_CLICK skelevent_click_master;
    PIXIT_POINT grid_step, desk_margin;
    PAGE_INFO page_info;
    PIXIT_POINT pa_shift, sa_shift;
    PIXIT_POINT page_pix;
    STATUS status;

    page_info.page_def = p_docu->page_def;

    VIEW_ASSERT(p_view);

    view_grid_step(p_docu, &grid_step, &desk_margin, p_view->display_mode, p_ctrl_view_skel->slice.x, p_ctrl_view_skel->slice.y);

    if(p_ctrl_view_skel->slice.x)
    {
        assert(grid_step.x);
        page_info.page_num.x = p_view_point_in->pixit_point.x / grid_step.x;
        page_pix.x = page_info.page_num.x * grid_step.x;
    }
    else
    {
        page_info.page_num.x = 0;
        page_pix.x = 0;
    }

    if(p_ctrl_view_skel->slice.y)
    {
        assert(grid_step.y);
        page_info.page_num.y = p_view_point_in->pixit_point.y / grid_step.y;
        page_pix.y = page_info.page_num.y * grid_step.y;
    }
    else
    {
        page_info.page_num.y = cur_page_y(p_docu);
        page_pix.y = 0;
    }

    p_redraw_tag_and_event->p_proc_event = NULL; /* in case we fail to match an area */

    skelevent_click_master = *p_skelevent_click_in; /* copying click_context and data */

    skelevent_click_master.click_context.pixit_origin.x += page_pix.x;
    skelevent_click_master.click_context.pixit_origin.y += page_pix.y;

    skelevent_click_master.skel_point.page_num = page_info.page_num;
    skelevent_click_master.skel_point.pixit_point.x = p_view_point_in->pixit_point.x - page_pix.x;
    skelevent_click_master.skel_point.pixit_point.y = p_view_point_in->pixit_point.y - page_pix.y;

    skelevent_click_master.work_skel_rect.tl.page_num = page_info.page_num;
    skelevent_click_master.work_skel_rect.br.page_num = page_info.page_num;

    skelevent_click_master.processed = 0;

    if(!p_ctrl_view_skel->slice.x)
    {
        page_info.page_def.margin_left = page_info.page_def.margin_right = page_info.page_def.margin_bind = page_info.page_def.margin_row = 0;
        page_info.page_def.size_x = HUGENUMBER; /* so we get a sensible print_area.tl.x and print_area.br.x (ought to be actual window width, but we don't have it) */
    }

    if(!p_ctrl_view_skel->slice.y)
    {
        page_info.page_def.margin_top = page_info.page_def.margin_bottom = page_info.page_def.margin_col = 0;
        page_info.page_def.size_y = HUGENUMBER; /* so we get a sensible print_area.tl.y and print_area.br.y (ought to be actual window height, but we don't have it) */
        page_info.headfoot_both.header.margin = page_info.headfoot_both.footer.margin = 0;
    }
    else
        headfoot_both_from_page_y(p_docu, &page_info.headfoot_both, page_info.page_num.y);

    page_info.paper_area.tl = desk_margin;
    page_info.paper_area.br.x = page_info.paper_area.tl.x + page_info.page_def.size_x;
    page_info.paper_area.br.y = page_info.paper_area.tl.y + page_info.page_def.size_y;

    page_info_fillin_print_area(&page_info, &pa_shift);
    page_info_fillin_cells_area(&page_info, &sa_shift);

    switch(p_view->display_mode)
    {
    default: default_unhandled();
#if CHECKING
    case DISPLAY_DESK_AREA:
#endif
        break;

    case DISPLAY_PRINT_AREA:
        page_info_shift(&page_info, &pa_shift, NULL /*no sa adjust*/);
        break;

    case DISPLAY_WORK_AREA:
        page_info_shift(&page_info, &pa_shift, &sa_shift);
        break;
    }

    { /* tell hangers on about all the relevant areas (expandable when needed) */
    SKELEVENT_CLICK_FILTER skelevent_click_filter;
    PC_PIXIT_RECT p_area;

    skelevent_click_filter.processed_level = SKELEVENT_CLICK_FILTER_UNPROCESSED;
    skelevent_click_filter.t5_message = t5_message;

    p_area = &page_info.cells_area;

    skelevent_click_filter.data.click[SKELEVENT_CLICK_FILTER_SLOTAREA].in_area =
           (skelevent_click_master.skel_point.pixit_point.x >= p_area->tl.x)
        && (skelevent_click_master.skel_point.pixit_point.x <  p_area->br.x)
        && (skelevent_click_master.skel_point.pixit_point.y >= p_area->tl.y)
        && (skelevent_click_master.skel_point.pixit_point.y <  p_area->br.y);

    skelevent_click_filter.data.click[SKELEVENT_CLICK_FILTER_SLOTAREA].skelevent_click = skelevent_click_master;

    skelevent_click_filter.data.click[SKELEVENT_CLICK_FILTER_SLOTAREA].skelevent_click.click_context.pixit_origin.x += p_area->tl.x;
    skelevent_click_filter.data.click[SKELEVENT_CLICK_FILTER_SLOTAREA].skelevent_click.click_context.pixit_origin.y += p_area->tl.y;

    skelevent_click_filter.data.click[SKELEVENT_CLICK_FILTER_SLOTAREA].skelevent_click.skel_point.pixit_point.x -= p_area->tl.x;
    skelevent_click_filter.data.click[SKELEVENT_CLICK_FILTER_SLOTAREA].skelevent_click.skel_point.pixit_point.y -= p_area->tl.y;

    skelevent_click_filter.data.click[SKELEVENT_CLICK_FILTER_SLOTAREA].skelevent_click.work_skel_rect.tl.pixit_point.x = 0;
    skelevent_click_filter.data.click[SKELEVENT_CLICK_FILTER_SLOTAREA].skelevent_click.work_skel_rect.tl.pixit_point.y = 0;
    skelevent_click_filter.data.click[SKELEVENT_CLICK_FILTER_SLOTAREA].skelevent_click.work_skel_rect.br.pixit_point.x = p_area->br.x - p_area->tl.x;
    skelevent_click_filter.data.click[SKELEVENT_CLICK_FILTER_SLOTAREA].skelevent_click.work_skel_rect.br.pixit_point.y = p_area->br.y - p_area->tl.y;

    skelevent_click_filter.data.click[SKELEVENT_CLICK_FILTER_SLOTAREA].skelevent_click.work_window = p_ctrl_view_skel->cells_area.redraw_tag;

    skelevent_click_filter.data.click[SKELEVENT_CLICK_FILTER_SLOTAREA].redraw_tag_and_event = p_ctrl_view_skel->cells_area;

    status_assert(maeve_service_event(p_docu, T5_EVENT_CLICK_FILTER, &skelevent_click_filter));

    switch(skelevent_click_filter.processed_level)
    {
    default: default_unhandled();
#if CHECKING
    case SKELEVENT_CLICK_FILTER_UNPROCESSED:
#endif
        break;

    case SKELEVENT_CLICK_FILTER_SUPPRESS:
        return(0);

    case SKELEVENT_CLICK_FILTER_SLOTAREA:
        *p_redraw_tag_and_event = skelevent_click_filter.data.click[skelevent_click_filter.processed_level].redraw_tag_and_event;
        return(1);
    }
    } /*block*/

    if(T5_EVENT_FILEINSERT_DOINSERT == t5_message) /* SKS 08jan95 more of a chance for ob_rec to intercept file loads */
    if((status = viewevent_click_call(p_docu, T5_EVENT_FILEINSERT_DOINSERT_1, &skelevent_click_master, &page_info.cells_area, &p_ctrl_view_skel->cells_area, p_redraw_tag_and_event)) != 0)
        return(status);

    if((status = viewevent_click_call(p_docu, t5_message, &skelevent_click_master, &page_info.cells_area, &p_ctrl_view_skel->cells_area_above, p_redraw_tag_and_event)) != 0)
        return(status);

    if((status = viewevent_click_call(p_docu, t5_message, &skelevent_click_master, &page_info.print_area, &p_ctrl_view_skel->print_area_above, p_redraw_tag_and_event)) != 0)
        return(status);

    if((status = viewevent_click_call(p_docu, t5_message, &skelevent_click_master, &page_info.paper_area, &p_ctrl_view_skel->paper_above, p_redraw_tag_and_event)) != 0)
        return(status);

    if((status = viewevent_click_call(p_docu, t5_message, &skelevent_click_master, &page_info.cells_area, &p_ctrl_view_skel->cells_area, p_redraw_tag_and_event)) != 0)
        return(status);

    switch(p_view->display_mode)
    {
    case DISPLAY_WORK_AREA:
        break;

    default:
        page_info_fillin_other(&page_info);

        if((status = viewevent_click_call(p_docu, t5_message, &skelevent_click_master, &page_info.header_area, &p_ctrl_view_skel->margin_header, p_redraw_tag_and_event)) != 0)
            return(status);

        if((status = viewevent_click_call(p_docu, t5_message, &skelevent_click_master, &page_info.footer_area, &p_ctrl_view_skel->margin_footer, p_redraw_tag_and_event)) != 0)
            return(status);

        if((status = viewevent_click_call(p_docu, t5_message, &skelevent_click_master, &page_info.col_area, &p_ctrl_view_skel->margin_col, p_redraw_tag_and_event)) != 0)
            return(status);

        if((status = viewevent_click_call(p_docu, t5_message, &skelevent_click_master, &page_info.row_area, &p_ctrl_view_skel->margin_row, p_redraw_tag_and_event)) != 0)
            return(status);

        break;
    }

    if((status = viewevent_click_call(p_docu, t5_message, &skelevent_click_master, &page_info.cells_area, &p_ctrl_view_skel->cells_area_below, p_redraw_tag_and_event)) != 0)
        return(status);

    if((status = viewevent_click_call(p_docu, t5_message, &skelevent_click_master, &page_info.print_area, &p_ctrl_view_skel->print_area_below, p_redraw_tag_and_event)) != 0)
        return(status);

    if((status = viewevent_click_call(p_docu, t5_message, &skelevent_click_master, &page_info.paper_area, &p_ctrl_view_skel->paper_below, p_redraw_tag_and_event)) != 0)
        return(status);

    return(0);
}

/******************************************************************************
*
* Convert a VIEW_POINT to a SKEL_POINT
*
* i.e. calculate the page number & pixit offset and adjust the context origin
*
******************************************************************************/

static S32
skel_point_from_view_point_and_redraw_tag(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_VIEW_POINT p_view_point_in,
    _OutRef_    P_SKEL_POINT p_skel_point_out,
    _InoutRef_  P_PIXIT_POINT p_origin,
    _InVal_     REDRAW_TAG redraw_tag)
{
    const P_VIEW p_view = p_view_point_in->p_view;
    const PC_CTRL_SKEL_VIEW p_ctrl_skel_view = p_ctrl_skel_view_from_redraw_tag(redraw_tag);
    PIXIT_POINT grid_step, desk_margin;
    PAGE_NUM page_num;
    PIXIT_POINT page_pix;
    PIXIT_RECT redraw_rect, clip_rect;

    view_grid_step(p_docu, &grid_step, &desk_margin, p_view->display_mode, p_ctrl_skel_view->slice.x, p_ctrl_skel_view->slice.y);

    if(p_ctrl_skel_view->slice.x)
    {
        S32 rhs_page = last_page_x(p_docu) - 1;
        assert(grid_step.x);
        page_num.x = p_view_point_in->pixit_point.x / grid_step.x;
        page_num.x = MIN(page_num.x, rhs_page);
        page_pix.x = page_num.x * grid_step.x;
    }
    else
    {
        page_num.x = 0;
        page_pix.x = 0;
    }

    if(p_ctrl_skel_view->slice.y)
    {
        assert(grid_step.y);
        page_num.y = p_view_point_in->pixit_point.y / grid_step.y;
        page_pix.y = page_num.y * grid_step.y;
    }
    else
    {
        page_num.y = cur_page_y(p_docu);
        page_pix.y = 0;
    }

    p_origin->x += page_pix.x;
    p_origin->y += page_pix.y;

    p_skel_point_out->page_num = page_num;
    p_skel_point_out->pixit_point.x = p_view_point_in->pixit_point.x - page_pix.x;
    p_skel_point_out->pixit_point.y = p_view_point_in->pixit_point.y - page_pix.y;

    /* get work_rect, clip_rect unused */
    if(pixit_rect_from_page(p_docu, &redraw_rect, &clip_rect, p_view->display_mode, redraw_tag, &page_num))
    {
        p_origin->x                     += redraw_rect.tl.x;
        p_origin->y                     += redraw_rect.tl.y;

        p_skel_point_out->pixit_point.x -= redraw_rect.tl.x;
        p_skel_point_out->pixit_point.y -= redraw_rect.tl.y;

        return(TRUE);
    }

    /* area specified isn't visible (i.e. wrong display_mode) */
    return(FALSE);
}

static S32
trim_clip_rectangle(
    _InoutRef_  P_SKELEVENT_REDRAW p_skelevent_redraw,
    _InVal_     PIXIT tl_x,
    _InVal_     PIXIT tl_y,
    _InVal_     PIXIT br_x,
    _InVal_     PIXIT br_y,
    _OutRef_    P_PIXIT_RECT p_pixit_rect)
{
    RECT_FLAGS rect_flags;

    assert(p_skelevent_redraw->clip_skel_rect.tl.page_num.x == p_skelevent_redraw->clip_skel_rect.br.page_num.x);
    assert(p_skelevent_redraw->clip_skel_rect.tl.page_num.y == p_skelevent_redraw->clip_skel_rect.br.page_num.y);

    p_pixit_rect->tl.x =
        p_skelevent_redraw->clip_skel_rect.tl.pixit_point.x = MAX(p_skelevent_redraw->clip_skel_rect.tl.pixit_point.x, tl_x);

    p_pixit_rect->tl.y =
        p_skelevent_redraw->clip_skel_rect.tl.pixit_point.y = MAX(p_skelevent_redraw->clip_skel_rect.tl.pixit_point.y, tl_y);

    p_pixit_rect->br.x =
        p_skelevent_redraw->clip_skel_rect.br.pixit_point.x = MIN(p_skelevent_redraw->clip_skel_rect.br.pixit_point.x, br_x);

    p_pixit_rect->br.y =
        p_skelevent_redraw->clip_skel_rect.br.pixit_point.y = MIN(p_skelevent_redraw->clip_skel_rect.br.pixit_point.y, br_y);

    RECT_FLAGS_CLEAR(rect_flags);
    return(host_set_clip_rectangle(&p_skelevent_redraw->redraw_context, p_pixit_rect, rect_flags));
}

/******************************************************************************
*
* The following event handlers receive events from the host
* and pass them to the appropriate skeleton event handlers,
* having translated the 'pixit space' coordinates into 'page space'
*
******************************************************************************/

static void
viewevent_leave_call(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    _InRef_     PC_REDRAW_TAG_AND_EVENT p_redraw_tag_and_event)
{
    if(p_redraw_tag_and_event->p_proc_event)
    {
        SKELEVENT_CLICK skelevent_click;

#if CHECKING
        memset32(&skelevent_click, '\xBF', sizeof32(skelevent_click));
#endif

        skelevent_click.work_window = p_redraw_tag_and_event->redraw_tag;

        status_assert((* p_redraw_tag_and_event->p_proc_event) (p_docu, t5_message, &skelevent_click));
    }
}

static REDRAW_TAG_AND_EVENT click_drag; /* stash so that T5_EVENT_CLICK_DRAG_xxxx can be directed to correct place */

_Check_return_
static STATUS
view_default_event(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    /*_Inout_*/ P_ANY p_data,
    _InRef_     PC_CTRL_VIEW_SKEL p_ctrl_view_skel)
{
    switch(t5_message)
    {
    case T5_EVENT_CLICK_LEFT_SINGLE:
    case T5_EVENT_CLICK_LEFT_DOUBLE:
    case T5_EVENT_CLICK_LEFT_TRIPLE:
    case T5_EVENT_CLICK_LEFT_DRAG:
    case T5_EVENT_CLICK_RIGHT_SINGLE:
    case T5_EVENT_CLICK_RIGHT_DOUBLE:
    case T5_EVENT_CLICK_RIGHT_TRIPLE:
    case T5_EVENT_CLICK_RIGHT_DRAG:
    case T5_EVENT_POINTER_ENTERS_WINDOW:
    case T5_EVENT_POINTER_MOVEMENT:
    case T5_EVENT_FILEINSERT_DOINSERT:
        {
        P_VIEWEVENT_CLICK p_viewevent_click = (P_VIEWEVENT_CLICK) p_data;
        SKELEVENT_CLICK skelevent_click;

        skelevent_click.click_context = p_viewevent_click->click_context;

        if(T5_EVENT_FILEINSERT_DOINSERT == t5_message)
        {
            skelevent_click.data.fileinsert.filename = p_viewevent_click->data.fileinsert.filename;
            skelevent_click.data.fileinsert.t5_filetype = p_viewevent_click->data.fileinsert.t5_filetype;
            skelevent_click.data.fileinsert.safesource = p_viewevent_click->data.fileinsert.safesource;
        }
#if WINDOWS && 1
        else if(T5_EVENT_POINTER_MOVEMENT == t5_message)
        {
            /* why did I have this as a special case ? */
            host_set_pointer_shape(POINTER_DEFAULT);
        }
#endif
        else if(T5_EVENT_POINTER_ENTERS_WINDOW == t5_message)
        {
            host_set_pointer_shape(POINTER_DEFAULT);
        }

        /* stashing stuff in click_drag so T5_EVENT_CLICK_DRAG_xxxx can be directed to correct place */
        return(viewevent_click_process(p_docu, t5_message, &p_viewevent_click->view_point, p_ctrl_view_skel, &skelevent_click, &click_drag));
        }

    case T5_EVENT_POINTER_LEAVES_WINDOW:
        {
        P_VIEWEVENT_CLICK p_viewevent_click = (P_VIEWEVENT_CLICK) p_data;

        {
        SKELEVENT_CLICK_FILTER skelevent_click_filter;

        skelevent_click_filter.processed_level = SKELEVENT_CLICK_FILTER_UNPROCESSED;
        skelevent_click_filter.t5_message = t5_message;
        skelevent_click_filter.data.click[SKELEVENT_CLICK_FILTER_SLOTAREA].in_area = (NULL != p_ctrl_view_skel->cells_area.p_proc_event);
        skelevent_click_filter.data.click[SKELEVENT_CLICK_FILTER_SLOTAREA].redraw_tag_and_event = p_ctrl_view_skel->cells_area;

        status_assert(maeve_service_event(p_docu, T5_EVENT_CLICK_FILTER, &skelevent_click_filter));

        switch(skelevent_click_filter.processed_level)
        {
        default: default_unhandled();
#if CHECKING
        case SKELEVENT_CLICK_FILTER_UNPROCESSED:
#endif
            break;

        case SKELEVENT_CLICK_FILTER_SUPPRESS:
            return(STATUS_OK);

        case SKELEVENT_CLICK_FILTER_SLOTAREA:
            /* now redundant: click_drag = skelevent_click_filter.data.click[SKELEVENT_CLICK_FILTER_SLOTAREA].redraw_tag_and_event; */
            break;
        }
        } /*block*/

        /* SKS 13aug93 - just broadcast it as a click (it isn't of course, but this is convenient and pragmatic) */
        viewevent_leave_call(p_docu, t5_message, &p_ctrl_view_skel->cells_area_above);
        viewevent_leave_call(p_docu, t5_message, &p_ctrl_view_skel->print_area_above);
        viewevent_leave_call(p_docu, t5_message, &p_ctrl_view_skel->paper_above);
        viewevent_leave_call(p_docu, t5_message, &p_ctrl_view_skel->cells_area);

        switch(p_viewevent_click->view_point.p_view->display_mode)
        {
        case DISPLAY_WORK_AREA: /* that's the point of the fudge */
            break;

        default:
            viewevent_leave_call(p_docu, t5_message, &p_ctrl_view_skel->margin_header);
            viewevent_leave_call(p_docu, t5_message, &p_ctrl_view_skel->margin_footer);
            viewevent_leave_call(p_docu, t5_message, &p_ctrl_view_skel->margin_col);
            viewevent_leave_call(p_docu, t5_message, &p_ctrl_view_skel->margin_row);
            break;
        }

        viewevent_leave_call(p_docu, t5_message, &p_ctrl_view_skel->cells_area_below);
        viewevent_leave_call(p_docu, t5_message, &p_ctrl_view_skel->print_area_below);
        viewevent_leave_call(p_docu, t5_message, &p_ctrl_view_skel->paper_below);

        return(STATUS_OK);
        }

    case T5_EVENT_CLICK_DRAG_STARTED:
    case T5_EVENT_CLICK_DRAG_MOVEMENT:
    case T5_EVENT_CLICK_DRAG_FINISHED:
    case T5_EVENT_CLICK_DRAG_ABORTED:
        {
        P_VIEWEVENT_CLICK p_viewevent_click = (P_VIEWEVENT_CLICK) p_data;
        SKELEVENT_CLICK skelevent_click;

        UNREFERENCED_PARAMETER_InRef_(p_ctrl_view_skel);

        skelevent_click.click_context = p_viewevent_click->click_context;

        skelevent_click.data.drag.p_reason_data = p_viewevent_click->data.drag.p_reason_data;
        skelevent_click.data.drag.pixit_delta = p_viewevent_click->data.drag.pixit_delta;

        skel_point_from_view_point_and_redraw_tag
                (p_docu,
                 &p_viewevent_click->view_point        /* in : view_point        */,
                 &skelevent_click.skel_point           /* out: skel_point        */,
                 &skelevent_click.click_context.pixit_origin /* & adjusted origin */,
                 click_drag.redraw_tag                 /* in                     */);

        if(click_drag.p_proc_event)
            return((* click_drag.p_proc_event) (p_docu, t5_message, &skelevent_click));

        return(STATUS_OK);
        }

    default:
        return(STATUS_OK);
    }
}

_Check_return_
static STATUS
view_event_back_window_redraw(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     P_VIEWEVENT_REDRAW p_viewevent_redraw)
{
    REDRAW_COLOURS redraw_colours;

    redraw_colours.p_desk_colour   = &rgb_stash[COLOUR_OF_DESKTOP];
    redraw_colours.p_margin_colour = NULL; /*&rgb_stash[COLOUR_OF_MARGIN];*/
    redraw_colours.p_paper_colour  = NULL; /*&rgb_stash[COLOUR_OF_PAPER];*/

    return(send_redraw_event_to_skel(p_docu, p_viewevent_redraw, &ctrl_view_skel_back_window, &redraw_colours));
}

PROC_EVENT_PROTO(extern, view_event_back_window)
{
    switch(t5_message)
    {
    case T5_EVENT_REDRAW:
        return(view_event_back_window_redraw(p_docu, (P_VIEWEVENT_REDRAW) p_data));

    default:
        return(view_default_event(p_docu, t5_message, p_data, &ctrl_view_skel_border_horz));
    }
}

_Check_return_
static STATUS
view_event_pane_window_redraw(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     P_VIEWEVENT_REDRAW p_viewevent_redraw)
{
    REDRAW_COLOURS redraw_colours;

    /* feed separate events to the skeleton for each page */
    /* paint desk between pages                           */

    redraw_colours.p_desk_colour   = &rgb_stash[COLOUR_OF_DESKTOP];
    redraw_colours.p_margin_colour = &rgb_stash[COLOUR_OF_MARGIN];
    redraw_colours.p_paper_colour  = &rgb_stash[COLOUR_OF_PAPER];

    /* cut x and y, cells drawn relative to paper margins */
    return(send_redraw_event_to_skel(p_docu, p_viewevent_redraw, &p_docu->ctrl_view_skel_pane_window, &redraw_colours));
}

PROC_EVENT_PROTO(extern, view_event_pane_window)
{
    switch(t5_message)
    {
    case T5_EVENT_REDRAW:
        return(view_event_pane_window_redraw(p_docu, (P_VIEWEVENT_REDRAW) p_data));

    case T5_CMD_TAB_LEFT:
    case T5_CMD_TAB_RIGHT:
        assert(p_docu->ctrl_view_skel_pane_window.cells_area.p_proc_event);
        return((* p_docu->ctrl_view_skel_pane_window.cells_area.p_proc_event) (p_docu, t5_message, p_data));

    case T5_EVENT_VISIBLEAREA_CHANGED:
        {
        P_VIEWEVENT_VISIBLEAREA_CHANGED p_viewevent_visiblearea_changed = (P_VIEWEVENT_VISIBLEAREA_CHANGED) p_data;
        const P_VIEW p_view = p_viewevent_visiblearea_changed->p_view;
        PIXIT_POINT grid_step, desk_margin;
        PAGE_NUM tl_page_num, br_page_num;

        VIEW_ASSERT(p_view);
        assert(p_viewevent_visiblearea_changed->visible_area.p_view == p_view);

        view_grid_step(p_docu, &grid_step, &desk_margin, p_view->display_mode, TRUE, TRUE);

        tl_page_num.x = p_viewevent_visiblearea_changed->visible_area.rect.tl.x / grid_step.x;
        tl_page_num.y = p_viewevent_visiblearea_changed->visible_area.rect.tl.y / grid_step.y;
        br_page_num.x = p_viewevent_visiblearea_changed->visible_area.rect.br.x / grid_step.x;
        br_page_num.y = p_viewevent_visiblearea_changed->visible_area.rect.br.y / grid_step.y;

        p_view->tl_visible_page = tl_page_num;

        p_view->br_visible_page.x = last_page_x(p_docu) - 1;
        p_view->br_visible_page.y = last_page_y(p_docu) - 1;

        p_view->br_visible_page.x = MIN(p_view->br_visible_page.x, br_page_num.x);
        p_view->br_visible_page.y = MIN(p_view->br_visible_page.y, br_page_num.y);
        }

        /* now tell lower layer of change */
        return((* p_docu->ctrl_view_skel_pane_window.cells_area.p_proc_event) (p_docu, t5_message, P_DATA_NONE));

    default:
        return(view_default_event(p_docu, t5_message, p_data, &p_docu->ctrl_view_skel_pane_window));
    }
}

_Check_return_
static STATUS
view_event_border_horz_redraw(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     P_VIEWEVENT_REDRAW p_viewevent_redraw)
{
    const PC_STYLE p_ruler_style = p_style_for_ruler_horz(); /* yes */
    REDRAW_COLOURS redraw_colours;

    redraw_colours.p_desk_colour   = &rgb_stash[COLOUR_OF_DESKTOP];
    redraw_colours.p_margin_colour = &rgb_stash[COLOUR_OF_MARGIN];
    redraw_colours.p_paper_colour  = colour_of_ruler(p_ruler_style);

    /* cut x but not y, borders drawn relative to paper margins */
    return(send_redraw_event_to_skel(p_docu, p_viewevent_redraw, &ctrl_view_skel_border_horz, &redraw_colours));
}

PROC_EVENT_PROTO(extern, view_event_border_horz)
{
    switch(t5_message)
    {
    case T5_EVENT_REDRAW:
        return(view_event_border_horz_redraw(p_docu, (P_VIEWEVENT_REDRAW) p_data));

    default:
        return(view_default_event(p_docu, t5_message, p_data, &ctrl_view_skel_border_horz));
    }
}

_Check_return_
static STATUS
view_event_border_vert_redraw(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     P_VIEWEVENT_REDRAW p_viewevent_redraw)
{
    const PC_STYLE p_ruler_style = p_style_for_ruler_vert(); /* yes */
    REDRAW_COLOURS redraw_colours;

    redraw_colours.p_desk_colour   = &rgb_stash[COLOUR_OF_DESKTOP];
    redraw_colours.p_margin_colour = &rgb_stash[COLOUR_OF_MARGIN];
    redraw_colours.p_paper_colour  = colour_of_ruler(p_ruler_style);

    /* cut y but not x, borders drawn relative to paper margins */
    return(send_redraw_event_to_skel(p_docu, p_viewevent_redraw, &ctrl_view_skel_border_vert, &redraw_colours));
}

PROC_EVENT_PROTO(extern, view_event_border_vert)
{
    switch(t5_message)
    {
    case T5_EVENT_REDRAW:
        return(view_event_border_vert_redraw(p_docu, (P_VIEWEVENT_REDRAW) p_data));

    default:
        return(view_default_event(p_docu, t5_message, p_data, &ctrl_view_skel_border_vert));
    }
}

_Check_return_
static STATUS
view_event_ruler_horz_redraw(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     P_VIEWEVENT_REDRAW p_viewevent_redraw)
{
    const PC_STYLE p_ruler_style = p_style_for_ruler_horz();
    REDRAW_COLOURS redraw_colours;

    redraw_colours.p_desk_colour   = &rgb_stash[COLOUR_OF_DESKTOP];
    redraw_colours.p_margin_colour = &rgb_stash[COLOUR_OF_MARGIN];
    redraw_colours.p_paper_colour  = colour_of_ruler(p_ruler_style);

    /* cut x but not y, ruler drawn relative to paper edges */
    return(send_redraw_event_to_skel(p_docu, p_viewevent_redraw, &ctrl_view_skel_ruler_horz, &redraw_colours));
}

PROC_EVENT_PROTO(extern, view_event_ruler_horz)
{
    switch(t5_message)
    {
    case T5_EVENT_REDRAW:
        return(view_event_ruler_horz_redraw(p_docu, (P_VIEWEVENT_REDRAW) p_data));

    default:
        return(view_default_event(p_docu, t5_message, p_data, &ctrl_view_skel_ruler_horz));
    }
}

_Check_return_
static STATUS
view_event_ruler_vert_redraw(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     P_VIEWEVENT_REDRAW p_viewevent_redraw)
{
    const PC_STYLE p_ruler_style = p_style_for_ruler_vert();
    REDRAW_COLOURS redraw_colours;

    redraw_colours.p_desk_colour   = &rgb_stash[COLOUR_OF_DESKTOP];
    redraw_colours.p_margin_colour = &rgb_stash[COLOUR_OF_MARGIN];
    redraw_colours.p_paper_colour  = colour_of_ruler(p_ruler_style);

    /* cut y but not x, borders drawn relative to paper edges */
    return(send_redraw_event_to_skel(p_docu, p_viewevent_redraw, &ctrl_view_skel_ruler_vert, &redraw_colours));
}

PROC_EVENT_PROTO(extern, view_event_ruler_vert)
{
    switch(t5_message)
    {
    case T5_EVENT_REDRAW:
        return(view_event_ruler_vert_redraw(p_docu, (P_VIEWEVENT_REDRAW) p_data));

    default:
        return(view_default_event(p_docu, t5_message, p_data, &ctrl_view_skel_ruler_vert));
    }
}

/******************************************************************************
*
* Cut the view space into page sized chunks, paint the background for each
* and pass one T5_REDRAW_EVENT per page to the skeleton layer
*
* This routine processes redraw events from a number of view event handlers, its
* slicing action is controlled by the CTRL_VIEW_SKEL struture and its background
* colours by the REDRAW_COLOURS structure
*
******************************************************************************/

_Check_return_
static STATUS
send_redraw_event_to_skel(
    _DocuRef_   P_DOCU p_docu,
    P_VIEWEVENT_REDRAW p_viewevent_redraw,
    _InRef_     PC_CTRL_VIEW_SKEL p_ctrl_view_skel,
    P_REDRAW_COLOURS p_redraw_colours)
{
    const P_VIEW p_view = p_viewevent_redraw->p_view;
    PIXIT_POINT grid_step, desk_margin;
    PAGE_NUM tl_page_num, br_page_num;
    PAGE_NUM inner_pagestep, outer_pagestep;
    PAGE_INFO page_info;
    S32 outer_count, inner_count;

    view_grid_step(p_docu, &grid_step, &desk_margin, p_view->display_mode, p_ctrl_view_skel->slice.x, p_ctrl_view_skel->slice.y);

    if(p_ctrl_view_skel->slice.x)
    {
        assert(grid_step.x);
        tl_page_num.x = p_viewevent_redraw->area.rect.tl.x / grid_step.x;
        br_page_num.x = (p_viewevent_redraw->area.rect.br.x == p_viewevent_redraw->area.rect.tl.x)
                      ? tl_page_num.x
                      : p_viewevent_redraw->area.rect.br.x / grid_step.x;
    }
    else
        tl_page_num.x = br_page_num.x = 0;

    if(p_ctrl_view_skel->slice.y)
    {
        assert(grid_step.y);
        tl_page_num.y = p_viewevent_redraw->area.rect.tl.y / grid_step.y;
        br_page_num.y = (p_viewevent_redraw->area.rect.br.y == p_viewevent_redraw->area.rect.tl.y)
                      ? tl_page_num.y
                      : p_viewevent_redraw->area.rect.br.y / grid_step.y;
    }
    else
        tl_page_num.y = br_page_num.y = cur_page_y(p_docu);

    page_info.page_def = p_docu->page_def;
    page_info.page_num = tl_page_num;

    if(!p_ctrl_view_skel->slice.x)
    {
        page_info.page_def.margin_left = page_info.page_def.margin_right = page_info.page_def.margin_bind = page_info.page_def.margin_row = 0;
        page_info.page_def.size_x = HUGENUMBER; /* not p_viewevent_redraw->work_area.rect.br.x; actual window width */
    }

    if(!p_ctrl_view_skel->slice.y)
    {
        page_info.page_def.margin_col = page_info.page_def.margin_top = page_info.page_def.margin_bottom = 0;
        page_info.page_def.size_y = HUGENUMBER; /* not p_viewevent_redraw->area.rect.br.y; actual window height */
    }

    if(!p_ctrl_view_skel->slice.x && !p_ctrl_view_skel->slice.y)
    {
        /* only one to do - possibly optimise this */ /*EMPTY*/
    }

    inner_pagestep.x = 1;
    inner_pagestep.y = 0;

    outer_pagestep.x = tl_page_num.x - (br_page_num.x + 1);
    outer_pagestep.y = 1;

    for(outer_count = tl_page_num.y; outer_count <= br_page_num.y; ++outer_count)
    {
        if(p_ctrl_view_skel->slice.y) /* SKS - this therefore knows outer loop is down y pages */
            headfoot_both_from_page_y(p_docu, &page_info.headfoot_both, page_info.page_num.y);
        else
            page_info.headfoot_both.header.margin = page_info.headfoot_both.footer.margin = 0;

        for(inner_count = tl_page_num.x; inner_count <= br_page_num.x; ++inner_count)
        {
            SKELEVENT_REDRAW skelevent_redraw;
            PIXIT_POINT pa_shift, sa_shift;
            PIXIT_RECT display_area;

            skelevent_redraw.flags = p_viewevent_redraw->flags;
            skelevent_redraw.redraw_context = p_viewevent_redraw->redraw_context;

            skelevent_redraw.clip_skel_rect.tl.page_num = page_info.page_num;
            skelevent_redraw.clip_skel_rect.tl.pixit_point = p_viewevent_redraw->area.rect.tl;

            skelevent_redraw.clip_skel_rect.br.page_num = page_info.page_num;
            skelevent_redraw.clip_skel_rect.br.pixit_point = p_viewevent_redraw->area.rect.br;

            adjust_redraw_origin(&skelevent_redraw, page_info.page_num.x * grid_step.x, page_info.page_num.y * grid_step.y);

            page_info.paper_area.tl = desk_margin;
            page_info.paper_area.br.x = page_info.paper_area.tl.x + page_info.page_def.size_x;
            page_info.paper_area.br.y = page_info.paper_area.tl.y + page_info.page_def.size_y;

            page_info_fillin_print_area(&page_info, &pa_shift);
            page_info_fillin_cells_area(&page_info, &sa_shift);

            if((page_info.page_num.x >= last_page_x(p_docu)) || (page_info.page_num.y >= last_page_y(p_docu)))
            {
                PIXIT_RECT whole_page_pixit_rect;

                whole_page_pixit_rect.tl.x = 0;
                whole_page_pixit_rect.tl.y = 0;
                whole_page_pixit_rect.br.x = grid_step.x ? grid_step.x : (100 * PIXITS_PER_METRE);
                whole_page_pixit_rect.br.y = grid_step.y ? grid_step.y : (100 * PIXITS_PER_METRE);

                if(p_redraw_colours->p_desk_colour)
                    host_paint_rectangle_filled(&skelevent_redraw.redraw_context, &whole_page_pixit_rect, p_redraw_colours->p_desk_colour);
            }
            else
            {
                switch(p_view->display_mode)
                {
                default: default_unhandled();
#if CHECKING
                case DISPLAY_DESK_AREA:
                case DISPLAY_PRINT_AREA:
#endif
                    {
                    if(p_view->display_mode == DISPLAY_PRINT_AREA)
                    {
                        page_info_shift(&page_info, &pa_shift, NULL /*no sa adjust*/);

                        display_area = page_info.print_area;
                    }
                    else
                        display_area = page_info.paper_area;

                    page_info_fillin_other(&page_info);

                    if(skelevent_redraw.flags.show_content)
                    {
                        /* draw a section of desk, with the paper/printable area on top of it */
                        if(p_redraw_colours->p_desk_colour)
                            paint_desk_strips(&skelevent_redraw.redraw_context, &grid_step, &display_area, p_redraw_colours->p_desk_colour);

                        if(p_view->display_mode == DISPLAY_DESK_AREA)
                        {
                            /* draw printable area margins */

                            if(p_ctrl_view_skel->slice.y && p_redraw_colours->p_margin_colour)
                            {
                                PIXIT_RECT h_margin_strip;

                                h_margin_strip.tl.x = page_info.paper_area.tl.x;
                                h_margin_strip.br.x = page_info.paper_area.br.x;

                                /* paint top print margin */
                                h_margin_strip.tl.y = page_info.paper_area.tl.y;
                                h_margin_strip.br.y = page_info.print_area.tl.y;
                                host_paint_rectangle_filled(&skelevent_redraw.redraw_context, &h_margin_strip, p_redraw_colours->p_margin_colour);

                                /* paint bottom print margin */
                                h_margin_strip.tl.y = page_info.print_area.br.y;
                                h_margin_strip.br.y = page_info.paper_area.br.y;
                                host_paint_rectangle_filled(&skelevent_redraw.redraw_context, &h_margin_strip, p_redraw_colours->p_margin_colour);
                            }

                            if(p_ctrl_view_skel->slice.x && p_redraw_colours->p_margin_colour)
                            {
                                PIXIT_RECT v_margin_strip;

                                v_margin_strip.tl.y = page_info.print_area.tl.y;
                                v_margin_strip.br.y = page_info.print_area.br.y;

                                /* paint left print margin */
                                v_margin_strip.tl.x = page_info.paper_area.tl.x;
                                v_margin_strip.br.x = page_info.print_area.tl.x;
                                host_paint_rectangle_filled(&skelevent_redraw.redraw_context, &v_margin_strip, p_redraw_colours->p_margin_colour);

                                /* paint right print margin */
                                v_margin_strip.tl.x = page_info.print_area.br.x;
                                v_margin_strip.br.x = page_info.paper_area.br.x;
                                host_paint_rectangle_filled(&skelevent_redraw.redraw_context, &v_margin_strip, p_redraw_colours->p_margin_colour);
                            }
                        }

                        /* printable area */
                        if(p_redraw_colours->p_paper_colour)
                            host_paint_rectangle_filled(&skelevent_redraw.redraw_context, &page_info.print_area, p_redraw_colours->p_paper_colour);
                    }

                    break;
                    }

                case DISPLAY_WORK_AREA:
                    {
                    page_info_shift(&page_info, &pa_shift, &sa_shift);

                    display_area = page_info.cells_area;

                    if(skelevent_redraw.flags.show_content)
                    {
                        /* draw a section of desk, the paper on top of it shows just its cells area */
                        if(p_redraw_colours->p_desk_colour)
                            paint_desk_strips(&skelevent_redraw.redraw_context, &grid_step, &display_area, p_redraw_colours->p_desk_colour);

                        if(p_redraw_colours->p_paper_colour)
                            host_paint_rectangle_filled(&skelevent_redraw.redraw_context, &page_info.cells_area, p_redraw_colours->p_paper_colour);
                    }

                    break;
                    }
                }

                try_calling(p_docu, &skelevent_redraw, &page_info.paper_area,  &display_area,          &p_ctrl_view_skel->paper_below       CHECKING_ONLY_ARG(TEXT("paper_below")));
                try_calling(p_docu, &skelevent_redraw, &page_info.print_area,  &page_info.print_area,  &p_ctrl_view_skel->print_area_below  CHECKING_ONLY_ARG(TEXT("print_area_below")));
                try_calling(p_docu, &skelevent_redraw, &page_info.cells_area,  &page_info.cells_area,  &p_ctrl_view_skel->cells_area_below  CHECKING_ONLY_ARG(TEXT("cells_area_below")));

                try_calling(p_docu, &skelevent_redraw, &page_info.cells_area,  &page_info.cells_area,  &p_ctrl_view_skel->cells_area        CHECKING_ONLY_ARG(TEXT("cells_area")));

                switch(p_view->display_mode)
                {
                case DISPLAY_WORK_AREA:
                    break;

                default:
                    try_calling(p_docu, &skelevent_redraw, &page_info.header_area, &page_info.header_area, &p_ctrl_view_skel->margin_header CHECKING_ONLY_ARG(TEXT("margin_header")));
                    try_calling(p_docu, &skelevent_redraw, &page_info.footer_area, &page_info.footer_area, &p_ctrl_view_skel->margin_footer CHECKING_ONLY_ARG(TEXT("margin_footer")));
                    try_calling(p_docu, &skelevent_redraw, &page_info.col_area,    &page_info.col_area,    &p_ctrl_view_skel->margin_col    CHECKING_ONLY_ARG(TEXT("margin_col")));
                    try_calling(p_docu, &skelevent_redraw, &page_info.row_area,    &page_info.row_area,    &p_ctrl_view_skel->margin_row    CHECKING_ONLY_ARG(TEXT("margin_row")));
                    break;
                }

                try_calling(p_docu, &skelevent_redraw, &page_info.paper_area,  &display_area,          &p_ctrl_view_skel->paper_above       CHECKING_ONLY_ARG(TEXT("paper_above")));
                try_calling(p_docu, &skelevent_redraw, &page_info.print_area,  &page_info.print_area,  &p_ctrl_view_skel->print_area_above  CHECKING_ONLY_ARG(TEXT("print_area_above")));
                try_calling(p_docu, &skelevent_redraw, &page_info.cells_area,  &page_info.cells_area,  &p_ctrl_view_skel->cells_area_above  CHECKING_ONLY_ARG(TEXT("cells_area_above")));
            }

            host_restore_clip_rectangle(&skelevent_redraw.redraw_context);

            /* completed this page, so step one page right */
            page_info.page_num.x += inner_pagestep.x;
            page_info.page_num.y += inner_pagestep.y;
        }

        /* completed a horizontal strip of pages, so step down and left to beginning of next strip */
        page_info.page_num.x += outer_pagestep.x;
        page_info.page_num.y += outer_pagestep.y;
    }

    return(STATUS_OK);
}

/*
* exported for ho_print to paint one page
*/

extern void
view_redraw_page(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_SKELEVENT_REDRAW p_skelevent_redraw,
    P_PIXIT_RECT p_print_area)
{
    PC_CTRL_VIEW_SKEL p_ctrl_view_skel = &p_docu->ctrl_view_skel_pane_window;
    PAGE_INFO page_info;
    PIXIT_POINT sa_shift; /* used only as necessary store */

    page_info.page_def = p_docu->page_def;
    page_info.page_num = p_skelevent_redraw->clip_skel_rect.tl.page_num;

    headfoot_both_from_page_y(p_docu, &page_info.headfoot_both, page_info.page_num.y);

    page_info.print_area = *p_print_area;

    /* perform reverse of page_info_fillin_print_area() */
    page_info.paper_area       = page_info.print_area;
    page_info.paper_area.tl.x -= margin_left_from( &page_info.page_def, page_info.page_num.y);
    page_info.paper_area.tl.y -= page_info.page_def.margin_top;
    page_info.paper_area.br.x += margin_right_from(&page_info.page_def, page_info.page_num.y);
    page_info.paper_area.br.y += page_info.page_def.margin_bottom;

    page_info_fillin_cells_area(&page_info, &sa_shift);

    page_info_fillin_other(&page_info);

    try_calling(p_docu, p_skelevent_redraw, &page_info.paper_area,  &page_info.paper_area,  &p_ctrl_view_skel->paper_below      CHECKING_ONLY_ARG(TEXT("paper_below")));
    try_calling(p_docu, p_skelevent_redraw, &page_info.print_area,  &page_info.print_area,  &p_ctrl_view_skel->print_area_below CHECKING_ONLY_ARG(TEXT("print_area_below")));
    try_calling(p_docu, p_skelevent_redraw, &page_info.cells_area,  &page_info.cells_area,  &p_ctrl_view_skel->cells_area_below CHECKING_ONLY_ARG(TEXT("cells_area_below")));

    try_calling(p_docu, p_skelevent_redraw, &page_info.cells_area,  &page_info.cells_area,  &p_ctrl_view_skel->cells_area       CHECKING_ONLY_ARG(TEXT("cells_area")));

    try_calling(p_docu, p_skelevent_redraw, &page_info.header_area, &page_info.header_area, &p_ctrl_view_skel->margin_header    CHECKING_ONLY_ARG(TEXT("margin_header")));
    try_calling(p_docu, p_skelevent_redraw, &page_info.footer_area, &page_info.footer_area, &p_ctrl_view_skel->margin_footer    CHECKING_ONLY_ARG(TEXT("margin_footer")));
    try_calling(p_docu, p_skelevent_redraw, &page_info.col_area,    &page_info.col_area,    &p_ctrl_view_skel->margin_col       CHECKING_ONLY_ARG(TEXT("margin_col")));
    try_calling(p_docu, p_skelevent_redraw, &page_info.row_area,    &page_info.row_area,    &p_ctrl_view_skel->margin_row       CHECKING_ONLY_ARG(TEXT("margin_row")));

    try_calling(p_docu, p_skelevent_redraw, &page_info.paper_area,  &page_info.paper_area,  &p_ctrl_view_skel->paper_above      CHECKING_ONLY_ARG(TEXT("paper_above")));
    try_calling(p_docu, p_skelevent_redraw, &page_info.print_area,  &page_info.print_area,  &p_ctrl_view_skel->print_area_above CHECKING_ONLY_ARG(TEXT("print_area_above")));
    try_calling(p_docu, p_skelevent_redraw, &page_info.cells_area,  &page_info.cells_area,  &p_ctrl_view_skel->cells_area_above CHECKING_ONLY_ARG(TEXT("cells_area_above")));
}

static void
try_calling(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_SKELEVENT_REDRAW p_skelevent_redraw,
    _InRef_     PC_PIXIT_RECT p_work_area,
    _InRef_     PC_PIXIT_RECT p_clip_area,
    _InRef_     PC_REDRAW_TAG_AND_EVENT p_redraw_tag_and_event
    CHECKING_ONLY_ARG(PCTSTR area))
{
    CHECKING_ONLY(UNREFERENCED_PARAMETER(area));

    if(p_redraw_tag_and_event->p_proc_event)
    {
        SKELEVENT_REDRAW skelevent_redraw = *p_skelevent_redraw;
        PIXIT_RECT trimmed_pixit_rect;
        S32 res;

        skelevent_redraw.work_window = p_redraw_tag_and_event->redraw_tag;
        skelevent_redraw.work_skel_rect.tl.pixit_point = p_work_area->tl;
        skelevent_redraw.work_skel_rect.br.pixit_point = p_work_area->br;

        skelevent_redraw.work_skel_rect.tl.page_num =
        skelevent_redraw.work_skel_rect.br.page_num = skelevent_redraw.clip_skel_rect.tl.page_num;

        skelevent_redraw.work_skel_rect.tl.pixit_point.x -= p_work_area->tl.x;
        skelevent_redraw.work_skel_rect.tl.pixit_point.y -= p_work_area->tl.y;
        skelevent_redraw.work_skel_rect.br.pixit_point.x -= p_work_area->tl.x;
        skelevent_redraw.work_skel_rect.br.pixit_point.y -= p_work_area->tl.y;

        adjust_redraw_origin(&skelevent_redraw, p_work_area->tl.x, p_work_area->tl.y);

        res =
        trim_clip_rectangle(&skelevent_redraw,
                               p_clip_area->tl.x - p_work_area->tl.x, p_clip_area->tl.y - p_work_area->tl.y,
                               p_clip_area->br.x - p_work_area->tl.x, p_clip_area->br.y - p_work_area->tl.y,
                               &trimmed_pixit_rect);

        if(res)
        {
#if defined(NOTE_LAYER) && 0 /*SKS - invent some damage mechanism*/
            if(p_redraw_tag_and_event->p_proc_event == object_skel)
            {
                RECT_FLAGS rect_flags;

                if(skelevent_redraw.flags.show_content || (update_now_caller_layer <= LAYER_BACK))
                {
                    /* call notelayer, to render notes behind the cells */
                    backlayer_event(p_docu, T5_EVENT_REDRAW, &skelevent_redraw);
                    skelevent_redraw.flags.show_content = TRUE;

                    /* restore the clip rectangle set by trim_clip_rectangle() */
                    RECT_FLAGS_CLEAR(rect_flags);
                    host_set_clip_rectangle(&skelevent_redraw.redraw_context, &trimmed_pixit_rect, rect_flags);
                }

                if(skelevent_redraw.flags.show_content || (update_now_caller_layer <= LAYER_CELLS))
                {
                    /* call skeleton to render cells */
                    (* p_redraw_tag_and_event->p_proc_event) (p_docu, T5_EVENT_REDRAW, &skelevent_redraw);
                    skelevent_redraw.flags.show_content = TRUE;

                    /* MUST restore the clip rectangle set by trim_clip_rectangle(), cos the skeleton clips to each cell */
                    RECT_FLAGS_CLEAR(rect_flags);
                    host_set_clip_rectangle(&skelevent_redraw.redraw_context, &trimmed_pixit_rect, rect_flags);
                }

                /* call notelayer, to render notes above the cells */
                proc_event_note(p_docu, T5_EVENT_REDRAW, &skelevent_redraw);
            }
            else
                (* p_redraw_tag_and_event->p_proc_event) (p_docu, T5_EVENT_REDRAW, &skelevent_redraw);
#else
            (* p_redraw_tag_and_event->p_proc_event) (p_docu, T5_EVENT_REDRAW, &skelevent_redraw);
#endif
        }
    }
}

static S32
view_point_from_skel_point(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    P_VIEW_POINT p_view_point_out,
    _InVal_     REDRAW_TAG redraw_tag,
    P_SKEL_POINT p_skel_point_in)
{
    const PC_CTRL_SKEL_VIEW p_ctrl_skel_view = p_ctrl_skel_view_from_redraw_tag(redraw_tag);
    PIXIT_POINT grid_step, desk_margin;
    PIXIT_RECT work_rect, clip_rect;

    p_view_point_out->p_view = p_view;
    p_view_point_out->pixit_point.x = 0;
    p_view_point_out->pixit_point.y = 0;

    view_grid_step(p_docu, &grid_step, &desk_margin, p_view->display_mode, p_ctrl_skel_view->slice.x, p_ctrl_skel_view->slice.y);

    /* need work_area as an origin for the point, must test against clip_rect, cos */
    /* display mode may mean that some of the area is hidden                       */
    if(pixit_rect_from_page(p_docu, &work_rect, &clip_rect, p_view->display_mode, redraw_tag, &p_skel_point_in->page_num))
    {
        p_view_point_out->pixit_point.x = p_skel_point_in->pixit_point.x + work_rect.tl.x;
        p_view_point_out->pixit_point.y = p_skel_point_in->pixit_point.y + work_rect.tl.y;

        if( (p_view_point_out->pixit_point.x >= clip_rect.tl.x) &&
            (p_view_point_out->pixit_point.x <  clip_rect.br.x) &&
            (p_view_point_out->pixit_point.y >= clip_rect.tl.y) &&
            (p_view_point_out->pixit_point.y <  clip_rect.br.y) )
        {
            p_view_point_out->pixit_point.x += grid_step.x * p_skel_point_in->page_num.x;
            p_view_point_out->pixit_point.y += grid_step.y * p_skel_point_in->page_num.y;

            return(TRUE);
        }
    }

    return(FALSE); /* area not visible because of display_mode OR point outside area! */
}

extern void
view_grid_step(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_PIXIT_POINT p_pixit_step,
    _OutRef_    P_PIXIT_POINT p_desk_margin,
    _InVal_     DISPLAY_MODE display_mode,
    _InVal_     BOOL slice_x,
    _InVal_     BOOL slice_y)
{
    DOCU_ASSERT(p_docu);
    PTR_ASSERT(p_pixit_step);
    PTR_ASSERT(p_desk_margin);

    if(slice_x)
    {
        PIXIT desk_lm = desk_margin_left(display_mode);
        PIXIT grid_paper_size_x = p_docu->page_def.size_x;
        PIXIT grid_print_area_x = grid_paper_size_x - margin_left_from(&p_docu->page_def, 0) - margin_right_from(&p_docu->page_def, 0);
        PIXIT grid_cells_area_x = grid_print_area_x - p_docu->page_def.margin_row;

        switch(display_mode)
        {
        default: default_unhandled();
#if CHECKING
        case DISPLAY_DESK_AREA:
#endif
            p_desk_margin->x = desk_lm;
            p_pixit_step->x = desk_lm + grid_paper_size_x;
            break;

        case DISPLAY_PRINT_AREA:
            p_desk_margin->x = desk_lm;
            p_pixit_step->x = desk_lm + grid_print_area_x;
            break;

        case DISPLAY_WORK_AREA:
            p_desk_margin->x = desk_lm;
            p_pixit_step->x = desk_lm + grid_cells_area_x;
            break;
        }
    }
    else
    {
        p_desk_margin->x = 0;
        p_pixit_step->x = 0;
    }

    if(slice_y)
    {
        PIXIT desk_tm = desk_margin_top(display_mode);
        PIXIT grid_paper_size_y = p_docu->page_def.size_y;
        PIXIT grid_print_area_y = grid_paper_size_y - p_docu->page_def.margin_top - p_docu->page_def.margin_bottom;
        PIXIT grid_cells_area_y;

        switch(display_mode)
        {
        default: default_unhandled();
#if CHECKING
        case DISPLAY_DESK_AREA:
#endif
            p_desk_margin->y = desk_tm;
            p_pixit_step->y = desk_tm + grid_paper_size_y;
            break;

        case DISPLAY_PRINT_AREA:
            p_desk_margin->y = desk_tm;
            p_pixit_step->y = desk_tm + grid_print_area_y;
            break;

        case DISPLAY_WORK_AREA:
            {
            PIXIT header_margin_min, footer_margin_min;

            hefod_margins_min(p_docu, &header_margin_min, &footer_margin_min);

            grid_cells_area_y = grid_print_area_y - p_docu->page_def.margin_col - header_margin_min - footer_margin_min;

            p_desk_margin->y = desk_tm;
            p_pixit_step->y = desk_tm + grid_cells_area_y;

            break;
            }
        }
    }
    else
    {
        p_desk_margin->y = 0;
        p_pixit_step->y = 0;
    }
}

static void
upcall_slicer(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InVal_     REDRAW_TAG redraw_tag,
    _InRef_     PC_SKEL_RECT p_skel_rect,
    _InVal_     RECT_FLAGS rect_flags_in,
    _InVal_     REDRAW_FLAGS redraw_flags)
{
    const PC_CTRL_SKEL_VIEW p_ctrl_skel_view = p_ctrl_skel_view_from_redraw_tag(redraw_tag);
    SKEL_RECT skel_rect = *p_skel_rect; /* take a copy so we can widdle on it; rect_flags is passed by value, so it's safe */
    RECT_FLAGS rect_flags = rect_flags_in;
    PAGE_NUM loop_tl, loop_br;
    PAGE_NUM page_num;
    PAGE_NUM tl_visible_page, br_visible_page;
    PIXIT_POINT grid_step, desk_margin;
    PIXIT_RECT redraw_rect, clip_rect;

    if(rect_flags.extend_right_window)
    {
        skel_rect.br.page_num.x = p_view->br_visible_page.x; /*>>>was last_page_x(p_docu) - 1;*/

        rect_flags.extend_right_window = FALSE;
        rect_flags.extend_right_page   = TRUE;
    }

    if(rect_flags.extend_down_window)
    {
        skel_rect.br.page_num.y = p_view->br_visible_page.y; /*>>>was last_page_y(p_docu) - 1;*/

        rect_flags.extend_down_window = FALSE;
        rect_flags.extend_down_page   = TRUE;
    }

    /* the caller wants pages skel_rect.tl.page_num.x .. skel_rect.br.page_num.x by            */
    /*                        skel_rect.tl.page_num.y .. skel_rect.br.page_num.y updating, but */
    /* we only call the host for the pages that are actually on screen for this view           */

    tl_visible_page.x = p_ctrl_skel_view->slice.x ? p_view->tl_visible_page.x : 0;
    tl_visible_page.y = p_ctrl_skel_view->slice.y ? p_view->tl_visible_page.y : skel_rect.tl.page_num.y;
    br_visible_page.x = p_ctrl_skel_view->slice.x ? p_view->br_visible_page.x : 0;
    br_visible_page.y = p_ctrl_skel_view->slice.y ? p_view->br_visible_page.y : skel_rect.br.page_num.y;

    loop_tl.x = MAX(skel_rect.tl.page_num.x, tl_visible_page.x);
    loop_tl.y = MAX(skel_rect.tl.page_num.y, tl_visible_page.y);
    loop_br.x = MIN(skel_rect.br.page_num.x, br_visible_page.x);
    loop_br.y = MIN(skel_rect.br.page_num.y, br_visible_page.y);

    if((loop_tl.x > loop_br.x) || (loop_tl.y > loop_br.y))
        /* no pages visible, so give up now! */
        return;

    view_grid_step(p_docu, &grid_step, &desk_margin, p_view->display_mode, p_ctrl_skel_view->slice.x, p_ctrl_skel_view->slice.y);

    for(page_num.y = loop_tl.y; page_num.y <= loop_br.y; ++page_num.y)
    {
        for(page_num.x = loop_tl.x; page_num.x <= loop_br.x; ++page_num.x)
        {
            /*>>>munge the clip_rect in as well???*/
            if(pixit_rect_from_page(p_docu, &redraw_rect, &clip_rect, p_view->display_mode, redraw_tag, &page_num))
            {
                /* take care! - redraw_rect is relative to page placement grid          */
                /*              skel_rect.tl/br.pixit_point are relative to redraw_rect */

                if((page_num.x == skel_rect.br.page_num.x) && !rect_flags.extend_right_page)
                    redraw_rect.br.x = MIN(redraw_rect.br.x, (redraw_rect.tl.x + skel_rect.br.pixit_point.x));  /* must do this */

                if(page_num.x == skel_rect.tl.page_num.x)
                    redraw_rect.tl.x += MAX(0, skel_rect.tl.pixit_point.x);                                     /* before this! */

                if((page_num.y == skel_rect.br.page_num.y) && !rect_flags.extend_down_page)
                    redraw_rect.br.y = MIN(redraw_rect.br.y, (redraw_rect.tl.y + skel_rect.br.pixit_point.y));  /* must do this */

                if(page_num.y == skel_rect.tl.page_num.y)
                    redraw_rect.tl.y += MAX(0, skel_rect.tl.pixit_point.y);                                     /* before this! */
#if FALSE
                if((redraw_rect.tl.x < redraw_rect.br.x) && (redraw_rect.tl.y < redraw_rect.br.y))
                /* 6/10/92, RCM took test out, cos it we need to allow tl==br with flags.extend_xxx to call host */
#endif
                {
                    VIEW_RECT view_rect;

                    view_rect.p_view = p_view;
                    view_rect.rect.tl.x  = grid_step.x * page_num.x + redraw_rect.tl.x;
                    view_rect.rect.tl.y  = grid_step.y * page_num.y + redraw_rect.tl.y;
                    view_rect.rect.br.x  = grid_step.x * page_num.x + redraw_rect.br.x;
                    view_rect.rect.br.y  = grid_step.y * page_num.y + redraw_rect.br.y;

                    host_update(p_docu, &view_rect, &rect_flags, redraw_flags, redraw_tag);
                }
            }
        }
    }
}

/******************************************************************************
*
* Show caret iff we have the input focus
*
******************************************************************************/

extern void
view_show_caret(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     REDRAW_TAG redraw_tag,
    P_SKEL_POINT p_skel_point,
    _InVal_     PIXIT height)
{
    P_VIEW p_view;

    DOCU_ASSERT(p_docu);
    PTR_ASSERT(p_skel_point);

    trace_6(TRACE_APP_CLICK,
            TEXT("stuff a caret at redraw_tag = ") ENUM_XTFMT TEXT(", page=(") S32_TFMT TEXT(",") S32_TFMT TEXT("), offset=(") S32_TFMT TEXT(",") S32_TFMT TEXT("), height=") S32_TFMT,
            redraw_tag, p_skel_point->page_num.x, p_skel_point->page_num.y,
            p_skel_point->pixit_point.x, p_skel_point->pixit_point.y, height);

    p_view = p_view_from_viewno_caret(p_docu);

    if(VIEW_NOT_NONE(p_view))
    {
        VIEW_POINT view_point;

        if(view_point_from_skel_point(p_docu, p_view, &view_point, redraw_tag, p_skel_point))
            host_main_show_caret(p_docu, &view_point, height);
#if RISCOS
        else
            host_main_hide_caret(p_docu, p_view); /* area not visible due to display_mode or size, so hide caret */
#endif
    }
}

/******************************************************************************
*
* Scroll the preferred view until the caret/cursor position is visible
*
* The caret is shown only if we have the input focus
*
* If TRUE, centre_x and centre_y force scrolling of the caret/cursor
* position to the centre of the window, else scrolling occurs only to
* keep it within the window
*
******************************************************************************/

extern void
view_scroll_caret(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     REDRAW_TAG redraw_tag,
    P_SKEL_POINT p_skel_point,
    _InVal_     PIXIT left,
    _InVal_     PIXIT right,
    _InVal_     PIXIT top,
    _InVal_     PIXIT bottom)
{
    P_VIEW p_view;
    DOCU_ASSERT(p_docu);

    p_view = p_view_from_viewno_caret(p_docu);

    if(VIEW_NOT_NONE(p_view))
    {
        VIEW_POINT view_point;

        if(view_point_from_skel_point(p_docu, p_view, &view_point, redraw_tag, p_skel_point))
            host_main_scroll_caret(p_docu, &view_point, left, right, top, bottom);
      /*else                                     */
      /*    area not visible due to display_mode */
    }
}

extern void
view_scroll_pane(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    P_SKEL_POINT p_skel_point)
{
    const P_VIEW p_view = p_view_from_viewno_caret(p_docu);

    assert((t5_message == T5_CMD_PAGE_DOWN) || (t5_message == T5_CMD_PAGE_UP) || (t5_message == T5_CMD_SHIFT_PAGE_DOWN) || (t5_message == T5_CMD_SHIFT_PAGE_UP));

    if(VIEW_NOT_NONE(p_view))
    {
        REDRAW_TAG redraw_tag = UPDATE_PANE_CELLS_AREA;
        VIEW_POINT view_point;

        if(view_point_from_skel_point(p_docu, p_view, &view_point, redraw_tag, p_skel_point))
        {
            PIXIT_POINT origin;
            SKEL_POINT skel_point;

            host_scroll_pane(p_docu, p_view, t5_message, &view_point);

            origin.x = origin.y = 0;

            skel_point_from_view_point_and_redraw_tag(p_docu, &view_point, &skel_point, &origin, redraw_tag);

            *p_skel_point = skel_point;
        }
    }
}

/******************************************************************************
*
* Called from the skeleton to cause a redraw of the whole of window
*
******************************************************************************/

extern void
view_update_all(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     REDRAW_TAG redraw_tag)
{
    VIEWNO viewno = VIEWNO_NONE;
    P_VIEW p_view;

    while(P_VIEW_NONE != (p_view = docu_enum_views(p_docu, &viewno)))
        host_update_all(p_docu, p_view, redraw_tag);
}

/******************************************************************************
*
* Called from the skeleton to cause a redraw of an area of a window
*
******************************************************************************/

extern void
view_update_later(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     REDRAW_TAG redraw_tag,
    _InRef_     PC_SKEL_RECT p_skel_rect,
    _InVal_     RECT_FLAGS rect_flags)
{
    VIEWNO viewno = VIEWNO_NONE;
    P_VIEW p_view;
    REDRAW_FLAGS redraw_flags;
    REDRAW_FLAGS_CLEAR(redraw_flags);

#if TRUE
    trace_0(TRACE_APP_SKEL_DRAW, TEXT("view_update_later"));
    trace_4(TRACE_APP_SKEL_DRAW, TEXT("  top left     page (") S32_TFMT TEXT(",") S32_TFMT TEXT("), point (") S32_TFMT TEXT(",") S32_TFMT TEXT(")"),
        p_skel_rect->tl.page_num.x, p_skel_rect->tl.page_num.y, p_skel_rect->tl.pixit_point.x, p_skel_rect->tl.pixit_point.y);
    trace_4(TRACE_APP_SKEL_DRAW, TEXT("  bottom right page (") S32_TFMT TEXT(",") S32_TFMT TEXT("), point (") S32_TFMT TEXT(",") S32_TFMT TEXT(")"),
        p_skel_rect->br.page_num.x, p_skel_rect->br.page_num.y, p_skel_rect->br.pixit_point.x, p_skel_rect->br.pixit_point.y);
#endif

    while(P_VIEW_NONE != (p_view = docu_enum_views(p_docu, &viewno)))
        /* do a reverse bacon slicer on skel_rect, to turn pages into view space */
        upcall_slicer(p_docu, p_view, redraw_tag, p_skel_rect, rect_flags, redraw_flags);
}

extern void
view_update_later_single(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InVal_     REDRAW_TAG redraw_tag,
    _InRef_     PC_SKEL_RECT p_skel_rect,
    _InVal_     RECT_FLAGS rect_flags)
{
    REDRAW_FLAGS redraw_flags;
    REDRAW_FLAGS_CLEAR(redraw_flags);

    /* do a reverse bacon slicer on skel_rect, to turn pages into view space */
    upcall_slicer(p_docu, p_view, redraw_tag, p_skel_rect, rect_flags, redraw_flags);
}

extern void
view_update_now(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     REDRAW_TAG redraw_tag,
    _InRef_     PC_SKEL_RECT p_skel_rect,
    _InVal_     RECT_FLAGS rect_flags,
    _In_        REDRAW_FLAGS redraw_flags,
    _InVal_     LAYER layer)
{
    VIEWNO viewno = VIEWNO_NONE;
    P_VIEW p_view;

#if TRUE
    trace_0(TRACE_APP_SKEL_DRAW, TEXT("view_update_now"));
    trace_4(TRACE_APP_SKEL_DRAW, TEXT("  top left     page (") S32_TFMT TEXT(",") S32_TFMT TEXT("), point (") S32_TFMT TEXT(",") S32_TFMT TEXT(")"),
            p_skel_rect->tl.page_num.x, p_skel_rect->tl.page_num.y,
            p_skel_rect->tl.pixit_point.x, p_skel_rect->tl.pixit_point.y);
    trace_4(TRACE_APP_SKEL_DRAW, TEXT("  bottom right page (") S32_TFMT TEXT(",") S32_TFMT TEXT("), point (") S32_TFMT TEXT(",") S32_TFMT TEXT(")"),
            p_skel_rect->br.page_num.x, p_skel_rect->br.page_num.y,
            p_skel_rect->br.pixit_point.x, p_skel_rect->br.pixit_point.y);
#endif

    redraw_flags.update_now = 1;

#if defined(NOTE_LAYER) && 0
    update_now_caller_layer = layer;
#else
    UNREFERENCED_PARAMETER_InVal_(layer);
#endif

    while(P_VIEW_NONE != (p_view = docu_enum_views(p_docu, &viewno)))
        /* do a reverse bacon slicer on skel_rect, to turn pages into view space */
        upcall_slicer(p_docu, p_view, redraw_tag, p_skel_rect, rect_flags, redraw_flags);
}

extern void
view_update_now_single(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InVal_     REDRAW_TAG redraw_tag,
    _InRef_     PC_SKEL_RECT p_skel_rect,
    _InVal_     RECT_FLAGS rect_flags,
    _In_        REDRAW_FLAGS redraw_flags,
    _InVal_     LAYER layer)
{
    redraw_flags.update_now = 1;

#if defined(NOTE_LAYER) && 0
    update_now_caller_layer = layer;
#else
    UNREFERENCED_PARAMETER_InVal_(layer);
#endif

    /* do a reverse bacon slicer on skel_rect, to turn pages into view space */
    upcall_slicer(p_docu, p_view, redraw_tag, p_skel_rect, rect_flags, redraw_flags);
}

/******************************************************************************
*
* Called by the skeleton to cause a fast update of the pane
* containing the cursor, typically when typing in text
*
******************************************************************************/

static VIEWEVENT_REDRAW stash_viewevent_redraw;
static REDRAW_TAG       stash_work_redraw_tag;
static PIXIT_RECT       stash_work_area;
static PAGE_NUM         stash_page_num;
static PIXIT_RECT       stash_trimmed_pixit_rect;
#if FAST_UPDATE_CALLS_NOTELAYER && 0
static SKELEVENT_REDRAW stash_skelevent_redraw;
#endif

static void
fast_update_common(
    _DocuRef_   P_DOCU p_docu,
    P_VIEWEVENT_REDRAW p_viewevent_redraw,
    /*out*/ P_REDRAW_CONTEXT p_redraw_context)
{
    SKELEVENT_REDRAW skelevent_redraw;

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    skelevent_redraw.flags = p_viewevent_redraw->flags;

    skelevent_redraw.work_window = stash_work_redraw_tag;

    skelevent_redraw.work_skel_rect.tl.page_num = stash_page_num;
    skelevent_redraw.work_skel_rect.br.page_num = stash_page_num;
    skelevent_redraw.work_skel_rect.tl.pixit_point = stash_work_area.tl;
    skelevent_redraw.work_skel_rect.br.pixit_point = stash_work_area.br;

    skelevent_redraw.clip_skel_rect.tl.page_num = stash_page_num;
    skelevent_redraw.clip_skel_rect.br.page_num = stash_page_num;
    skelevent_redraw.clip_skel_rect.tl.pixit_point = p_viewevent_redraw->area.rect.tl;
    skelevent_redraw.clip_skel_rect.br.pixit_point = p_viewevent_redraw->area.rect.br;

    skelevent_redraw.redraw_context = p_viewevent_redraw->redraw_context;

#if FALSE
        /*>>>should adjust_redraw_origin() do this???*/
#else
        skelevent_redraw.work_skel_rect.tl.pixit_point.x -= stash_work_area.tl.x;
        skelevent_redraw.work_skel_rect.tl.pixit_point.y -= stash_work_area.tl.y;
        skelevent_redraw.work_skel_rect.br.pixit_point.x -= stash_work_area.tl.x;
        skelevent_redraw.work_skel_rect.br.pixit_point.y -= stash_work_area.tl.y;
#endif

    adjust_redraw_origin(&skelevent_redraw, stash_work_area.tl.x, stash_work_area.tl.y);

#if FAST_UPDATE_CALLS_NOTELAYER && 0
    stash_skelevent_redraw = skelevent_redraw;
#endif

    if(trim_clip_rectangle(&skelevent_redraw, 0, 0, stash_work_area.br.x - stash_work_area.tl.x, stash_work_area.br.y - stash_work_area.tl.y, &stash_trimmed_pixit_rect))
    {
#if FAST_UPDATE_CALLS_NOTELAYER && 0
        /* <<< SKS says we must invent a mechanism here */

        if(stash_work_redraw_tag == UPDATE_PANE_CELLS_AREA)
        {
            RECT_FLAGS rect_flags;

            /* call notelayer, to render notes behind the cells */
            /*backlayer_event(p_docu, T5_EVENT_REDRAW, &stash_skelevent_redraw)*/;

            /* restore the clip rectangle set by trim_clip_rectangle() (probably not changed by note layer) */
            RECT_FLAGS_CLEAR(rect_flags);
            host_set_clip_rectangle(&stash_skelevent_redraw.redraw_context, &stash_trimmed_pixit_rect, rect_flags);
        }
#else
        /*EMPTY*/
#endif
    }

    *p_redraw_context = skelevent_redraw.redraw_context;
}

_Check_return_
extern BOOL
view_update_fast_start(
    _DocuRef_   P_DOCU p_docu,
    /*out*/ P_REDRAW_CONTEXT p_redraw_context,
    _InVal_     REDRAW_TAG redraw_tag,
    _InRef_     PC_SKEL_RECT p_skel_rect,
    _InVal_     RECT_FLAGS rect_flags,
    _InVal_     LAYER layer)
{
    VIEWNO viewno = VIEWNO_NONE;
    REDRAW_FLAGS redraw_flags;
    S32 more;

    REDRAW_FLAGS_CLEAR(redraw_flags);

    UNREFERENCED_PARAMETER_InVal_(layer);
    DOCU_ASSERT(p_docu);
    PTR_ASSERT(p_skel_rect);

#if TRUE
    trace_0(TRACE_APP_SKEL_DRAW, TEXT("view_update_fast"));
    trace_4(TRACE_APP_SKEL_DRAW, TEXT("  top left     page (") S32_TFMT TEXT(",") S32_TFMT TEXT("), point (") S32_TFMT TEXT(",") S32_TFMT TEXT(")"),
            p_skel_rect->tl.page_num.x, p_skel_rect->tl.page_num.y, p_skel_rect->tl.pixit_point.x, p_skel_rect->tl.pixit_point.y);
    trace_4(TRACE_APP_SKEL_DRAW, TEXT("  bottom right page (") S32_TFMT TEXT(",") S32_TFMT TEXT("), point (") S32_TFMT TEXT(",") S32_TFMT TEXT(")"),
            p_skel_rect->br.page_num.x, p_skel_rect->br.page_num.y, p_skel_rect->br.pixit_point.x, p_skel_rect->br.pixit_point.y);
#endif

    assert(p_skel_rect->tl.page_num.x == p_skel_rect->br.page_num.x);
    assert(p_skel_rect->tl.page_num.y == p_skel_rect->br.page_num.y);
    assert(!rect_flags.extend_right_window);
    assert(!rect_flags.extend_down_window);

    { /* mark all views except the caret view for update_later */
    P_VIEW p_view;
    while(P_VIEW_NONE != (p_view = docu_enum_views(p_docu, &viewno)))
    {
        if(p_view->viewno != p_docu->viewno_caret)
        {   /* do a reverse bacon slicer on skel_rect, to turn pages into view space */
            upcall_slicer(p_docu, p_view, redraw_tag, p_skel_rect, rect_flags, redraw_flags);
        }
    }
    } /*block*/

    /* do a fast update of the caret view (if any) */
    if(p_docu->viewno_caret)
    {
        const P_VIEW p_view = p_view_from_viewno_caret(p_docu);
        const PC_CTRL_SKEL_VIEW p_ctrl_skel_view = p_ctrl_skel_view_from_redraw_tag(redraw_tag);
        PIXIT_POINT grid_step, desk_margin;
        PIXIT_RECT redraw_rect, clip_rect;
        VIEWEVENT_REDRAW viewevent_redraw;

        SKEL_RECT skel_rect = *p_skel_rect; /* take a copy so we can widdle on it */
                                            /* rect_flags is passed by value, so its safe */
        PAGE_NUM  page_num  = p_skel_rect->tl.page_num;

/*>>>mustn't allow rect_flags.extend_right_window or rect_flags.extend_down_window */
/*>>>it's safe to allow rect_flags.extend_right_page and rect_flags.extend_down_page */

        view_grid_step(p_docu, &grid_step, &desk_margin, p_view->display_mode, p_ctrl_skel_view->slice.x, p_ctrl_skel_view->slice.y);

        /*>>>munge the clip_rect in */
        if(pixit_rect_from_page(p_docu, &redraw_rect, &clip_rect, p_view->display_mode, redraw_tag, &skel_rect.tl.page_num))
        {
            /* take care! - redraw_rect is relative to page placement grid          */
            /*              skel_rect.tl/br.pixit_point are relative to redraw_rect */

            stash_page_num = page_num;

            stash_work_redraw_tag = redraw_tag;

            stash_work_area.tl.x = grid_step.x * page_num.x + redraw_rect.tl.x;
            stash_work_area.tl.y = grid_step.y * page_num.y + redraw_rect.tl.y;
            stash_work_area.br.x = grid_step.x * page_num.x + redraw_rect.br.x;
            stash_work_area.br.y = grid_step.y * page_num.y + redraw_rect.br.y;

            if(!rect_flags.extend_right_page)
                redraw_rect.br.x = MIN(redraw_rect.br.x, (redraw_rect.tl.x + skel_rect.br.pixit_point.x));      /* must do this */

            redraw_rect.tl.x += MAX(0, skel_rect.tl.pixit_point.x);                                             /* before this! */

            if(!rect_flags.extend_down_page)
                redraw_rect.br.y = MIN(redraw_rect.br.y, (redraw_rect.tl.y + skel_rect.br.pixit_point.y));      /* must do this */

            redraw_rect.tl.y += MAX(0, skel_rect.tl.pixit_point.y);                                             /* before this! */

            if((redraw_rect.tl.x < redraw_rect.br.x) && (redraw_rect.tl.y < redraw_rect.br.y))
            {
                VIEW_RECT view_rect;

                view_rect.p_view = p_view;
                view_rect.rect.tl.x = grid_step.x * page_num.x + redraw_rect.tl.x;
                view_rect.rect.tl.y = grid_step.y * page_num.y + redraw_rect.tl.y;
                view_rect.rect.br.x = grid_step.x * page_num.x + redraw_rect.br.x;
                view_rect.rect.br.y = grid_step.y * page_num.y + redraw_rect.br.y;

                if(TRUE == (more = host_update_fast_start(p_docu, &viewevent_redraw /*filled*/, redraw_tag, &view_rect, rect_flags)))
                {
                    stash_viewevent_redraw = viewevent_redraw;

                    fast_update_common(p_docu, &viewevent_redraw, p_redraw_context);
                }

                return(more);
            }
        }
    }

    return(FALSE);
}

_Check_return_
extern BOOL
view_update_fast_continue(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_REDRAW_CONTEXT p_redraw_context)
{
    BOOL more;

#if FAST_UPDATE_CALLS_NOTELAYER && 0
    /* <<< SKS says we must invent a mechanism here */
    if(stash_work_redraw_tag == UPDATE_PANE_CELLS_AREA)
    {
        RECT_FLAGS rect_flags;

        /* MUST restore the clip rectangle set by trim_clip_rectangle(), cos the skeleton clips to each cell */
        RECT_FLAGS_CLEAR(rect_flags);
        host_set_clip_rectangle(&stash_skelevent_redraw.redraw_context, &stash_trimmed_pixit_rect, rect_flags);

        /* call notelayer, to render notes above the cells */
        proc_event_note(p_docu, T5_EVENT_REDRAW, &stash_skelevent_redraw);
    }
#endif

    if(TRUE == (more = host_update_fast_continue(p_docu, &stash_viewevent_redraw)))
    {
        fast_update_common(p_docu, &stash_viewevent_redraw, p_redraw_context);
    }

    return(more);
}

extern void
view__update__later(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view)
{
    VIEW_RECT view_rect;

    DOCU_ASSERT(p_docu);
    VIEW_ASSERT(p_view);

    view_rect.p_view = p_view;
    view_rect.rect.tl.x = 0;
    view_rect.rect.tl.y = 0;
    view_rect.rect.br.x = 0x00FFFFFF;
    view_rect.rect.br.y = 0x00FFFFFF;

    host_all_update_later(p_docu, &view_rect);
}

extern void
view_visible_skel_rect(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InRef_     PC_PIXIT_RECT p_pixit_rect,
    /*out*/ P_SKEL_RECT p_skel_rect)
{
    PIXIT_POINT grid_step, desk_margin;
    PIXIT_RECT  redraw_rect, clip_rect;
    SKEL_RECT   skel_rect;

    DOCU_ASSERT(p_docu);
    VIEW_ASSERT(p_view);

    view_grid_step(p_docu, &grid_step, &desk_margin, p_view->display_mode, TRUE, TRUE);

    /* now convert pixit_rect to skel_rect */

    /* concider tl of rect */
    skel_rect.tl.page_num.x    = p_pixit_rect->tl.x / grid_step.x;
    skel_rect.tl.pixit_point.x = p_pixit_rect->tl.x - grid_step.x * skel_rect.tl.page_num.x;
    skel_rect.tl.page_num.y    = p_pixit_rect->tl.y / grid_step.y;
    skel_rect.tl.pixit_point.y = p_pixit_rect->tl.y - grid_step.y * skel_rect.tl.page_num.y;

    /* since we pass for UPDATE_PANE_CELLS_AREA, work_rect and clip_rect returned are identical */
    pixit_rect_from_page(p_docu, &redraw_rect, &clip_rect,
                         p_view->display_mode, UPDATE_PANE_CELLS_AREA, &skel_rect.tl.page_num);

    skel_rect.tl.pixit_point.x -= redraw_rect.tl.x;
    skel_rect.tl.pixit_point.y -= redraw_rect.tl.y;

    skel_rect.tl.pixit_point.x  = MAX(skel_rect.tl.pixit_point.x, 0);
    skel_rect.tl.pixit_point.y  = MAX(skel_rect.tl.pixit_point.y, 0);

    if(skel_rect.tl.pixit_point.x >= (redraw_rect.br.x - redraw_rect.tl.x))
    {
        skel_rect.tl.page_num.x++;
        skel_rect.tl.pixit_point.x = 0;
    }

    if(skel_rect.tl.pixit_point.y >= (redraw_rect.br.y - redraw_rect.tl.y))
    {
        skel_rect.tl.page_num.y++;
        skel_rect.tl.pixit_point.y = 0;
    }

    /* consider br of rect */
    skel_rect.br.page_num.x    = p_pixit_rect->br.x / grid_step.x;
    skel_rect.br.pixit_point.x = p_pixit_rect->br.x - grid_step.x * skel_rect.br.page_num.x;
    skel_rect.br.page_num.y    = p_pixit_rect->br.y / grid_step.y;
    skel_rect.br.pixit_point.y = p_pixit_rect->br.y - grid_step.y * skel_rect.br.page_num.y;

    /* since we pass for UPDATE_PANE_CELLS_AREA, work_rect and clip_rect returned are identical */
    pixit_rect_from_page(p_docu, &redraw_rect, &clip_rect,
                         p_view->display_mode, UPDATE_PANE_CELLS_AREA, &skel_rect.br.page_num);

    skel_rect.br.pixit_point.x -= redraw_rect.tl.x;
    skel_rect.br.pixit_point.y -= redraw_rect.tl.y;

    skel_rect.br.pixit_point.x  = MIN(skel_rect.br.pixit_point.x, (redraw_rect.br.x - redraw_rect.tl.x));
    skel_rect.br.pixit_point.y  = MIN(skel_rect.br.pixit_point.y, (redraw_rect.br.y - redraw_rect.tl.y));

    if(skel_rect.br.pixit_point.x < 0)
        skel_rect.br.page_num.x--;

    if(skel_rect.br.pixit_point.y < 0)
        skel_rect.br.page_num.y--;

    if( ((skel_rect.br.pixit_point.x < 0) || (skel_rect.br.pixit_point.y < 0)) &&
        (skel_rect.br.page_num.x >= 0) && (skel_rect.br.page_num.y >= 0) )
    {
        /* since we pass for UPDATE_PANE_CELLS_AREA, work_rect and clip_rect returned are identical */
        pixit_rect_from_page(p_docu, &redraw_rect, &clip_rect,
                             p_view->display_mode, UPDATE_PANE_CELLS_AREA, &skel_rect.br.page_num);

        if(skel_rect.br.pixit_point.x < 0)
            skel_rect.br.pixit_point.x = redraw_rect.br.x - redraw_rect.tl.x;

        if(skel_rect.br.pixit_point.y < 0)
            skel_rect.br.pixit_point.y = redraw_rect.br.y - redraw_rect.tl.y;
    }

#if TRUE
    /* add skel_rect to array, only if kosher! */
    if( ( (skel_rect.tl.page_num.x < skel_rect.br.page_num.x)
                                                                        ||
          ((skel_rect.tl.page_num.x == skel_rect.br.page_num.x)      &&
           (skel_rect.tl.pixit_point.x < skel_rect.br.pixit_point.x)
          )
        )
      &&
        ( (skel_rect.tl.page_num.y < skel_rect.br.page_num.y)
                                                                        ||
          ((skel_rect.tl.page_num.y == skel_rect.br.page_num.y)      &&
           (skel_rect.tl.pixit_point.y < skel_rect.br.pixit_point.y)
          )
        )
      )
#endif
    {
        *p_skel_rect = skel_rect;
    }
}

extern void
view_install_edge_window_handler(
    _InVal_     EDGE_ID edge_id,
    _InRef_     P_PROC_EVENT p_proc_event)
{
    switch(edge_id)
    {
    case WIN_RULER_HORZ:
        ctrl_view_skel_ruler_horz.paper_above.p_proc_event = p_proc_event;
        break;

    case WIN_RULER_VERT:
        ctrl_view_skel_ruler_vert.paper_above.p_proc_event = p_proc_event;
        break;

    default: default_unhandled();
        break;
    }
}

#if RISCOS

extern void
view_install_main_window_handler(
    _InVal_     U32 win_id,
    _InRef_     P_PROC_EVENT p_proc_event)
{
    switch(win_id)
    {
    case WIN_TOOLBAR:
        ctrl_view_skel_back_window.paper_above.p_proc_event = p_proc_event;
        break;

    default: default_unhandled();
        break;
    }
}

#endif

extern void
view_install_pane_window_handler(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     LAYER layer,
    _InRef_     P_PROC_EVENT p_proc_event)
{
    switch(layer)
    {
    case LAYER_PAPER_BELOW:
        p_docu->ctrl_view_skel_pane_window.paper_below.p_proc_event = p_proc_event;
        break;

    case LAYER_PRINT_AREA_BELOW:
        p_docu->ctrl_view_skel_pane_window.print_area_below.p_proc_event = p_proc_event;
        break;

    case LAYER_CELLS_AREA_BELOW:
        p_docu->ctrl_view_skel_pane_window.cells_area_below.p_proc_event = p_proc_event;
        break;

    case LAYER_CELLS_AREA_ABOVE:
        p_docu->ctrl_view_skel_pane_window.cells_area_above.p_proc_event = p_proc_event;
        break;

    case LAYER_PRINT_AREA_ABOVE:
        p_docu->ctrl_view_skel_pane_window.print_area_above.p_proc_event = p_proc_event;
        break;

    case LAYER_PAPER_ABOVE:
        p_docu->ctrl_view_skel_pane_window.paper_above.p_proc_event = p_proc_event;
        break;

    default: default_unhandled();
        break;
    }
}

extern void
view_install_pane_window_hefo_handlers(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     P_PROC_EVENT p_proc_event_header,
    _InRef_     P_PROC_EVENT p_proc_event_footer)
{
    p_docu->ctrl_view_skel_pane_window.margin_header.p_proc_event = p_proc_event_header;
    p_docu->ctrl_view_skel_pane_window.margin_footer.p_proc_event = p_proc_event_footer;
}

/* Pack a view ID
 *
 * The ID is packed into a 32 bit unsigned word.
 * It assumes that the document ID and the view ID
 * can be fit into eight bits each.
*/

_Check_return_
extern U32
viewid_pack(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InVal_     U16 event_handler_id)
{
    const DOCNO docno = docno_from_p_docu(p_docu);
    VIEWNO viewno = VIEWNO_NONE;
    if((docno != DOCNO_NONE) && VIEW_NOT_NONE(p_view))
        viewno = viewno_from_p_view(p_view);
    return((U32) (((U32) event_handler_id << 16) | ((U32) viewno << 8) | docno));
}

/* Unpack a packed view ID
 *
 * It assumes the above packing function has been called and decodes the
 * 32 bit unsigned quantity into something sensible in the form
 * of a P_DOCU and a P_VIEW and a U16 user id.
*/

_Check_return_
_Ret_maybenone_
extern P_DOCU
viewid_unpack(
    _InVal_     U32 packed_view_id,
    _OutRef_    P_P_VIEW p_p_view,
    _OutRef_opt_ P_U16 p_event_handler_id)
{
    const VIEWNO viewno = (VIEWNO) ((packed_view_id & 0xFF00) >> 8);
    const DOCNO docno = (DOCNO) (packed_view_id & 0xFF);
    const P_DOCU p_docu = p_docu_from_docno(docno);

    *p_p_view = (docno != DOCNO_NONE) ? p_view_from_viewno(p_docu, viewno) : P_VIEW_NONE;

    if(NULL != p_event_handler_id)
        *p_event_handler_id = (U16) ((packed_view_id & 0xFFFF0000) >> 16);

    return(p_docu);
}

_Check_return_
_Ret_maybenone_
extern P_VIEW
p_view_from_viewno(
    _DocuRef_   PC_DOCU p_docu,
    _InVal_     VIEWNO viewno)
{
    if(DOCU_NOT_NONE(p_docu))
    {
        ARRAY_INDEX view_table_index = viewno - VIEWNO_FIRST; /* handle 0 avoidance */

        if(array_index_is_valid(&p_docu->h_view_table, view_table_index))
        {
            const P_VIEW p_view = array_ptr_no_checks(&p_docu->h_view_table, VIEW, view_table_index);

            if(!view_void_entry(p_view))
                return(p_view);
        }
    }

    return(P_VIEW_NONE);
}

_Check_return_
_Ret_maybenone_
extern P_VIEW
p_view_from_viewno_caret(
    _DocuRef_   PC_DOCU p_docu)
{
    if(IS_DOCU_NONE(p_docu))
        return(P_VIEW_NONE);

    return(p_view_from_viewno(p_docu, p_docu->viewno_caret));
}

_Check_return_
extern S32
cur_page_y(
    _DocuRef_   P_DOCU p_docu)
{
    ROW_ENTRY row_entry;
    ROW rows_n = n_rows(p_docu) - 1;
    row_entry_from_row(p_docu, &row_entry, MIN(p_docu->cur.slr.row, rows_n));
    return(row_entry.rowtab.edge_top.page);
}

/* set up the common (non-host-specific) bits of a HOST_XFORM */

extern void
set_host_xform_for_view_common( /* only use in host-specific set_host_xform_for_view() routines */
    _OutRef_    P_HOST_XFORM p_host_xform,
    _ViewRef_   P_VIEW p_view,
    _InVal_     BOOL do_x_scale,
    _InVal_     BOOL do_y_scale)
{
    p_host_xform->scale.t.x = 1;
    p_host_xform->scale.t.y = 1;
    p_host_xform->scale.b.x = 1;
    p_host_xform->scale.b.y = 1;

    if(p_view->scalet != p_view->scaleb)
    {
        if(do_x_scale)
        {
            p_host_xform->scale.t.x = p_view->scalet;
            p_host_xform->scale.b.x = p_view->scaleb;
        }

        if(do_y_scale)
        {
            p_host_xform->scale.t.y = p_view->scalet;
            p_host_xform->scale.b.y = p_view->scaleb;
        }
    }

    p_host_xform->do_x_scale = do_x_scale;
    p_host_xform->do_y_scale = do_y_scale;
}

/* end of view.c */
