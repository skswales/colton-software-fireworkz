/* ed_card.c */

/* Copyright (C) 1994-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#include "common/gflags.h"

#include "ob_rec/ob_rec.h"

#include "ob_skel/xp_skeld.h"

#include "ob_skel/ff_io.h"

#if RISCOS

#include "ob_skel/xp_skelr.h"

#include "ob_dlg/ri_lbox.h"

static RI_LBOX_HANDLE lbox;
static UI_SOURCE ui_source;
#endif

extern DRAG_STATE drag_state;

#define CARD_EAR_WIDTH  128
#define CARD_EAR_HEIGHT 128

/* Drag modes */
enum CARD_DRAG_MODES
{
    CARD_DRAG_L = 1,
    CARD_DRAG_R,
    CARD_DRAG_T,
    CARD_DRAG_B,
    CARD_DRAG_TR,
    CARD_DRAG_BR,
    CARD_DRAG_TL,
    CARD_DRAG_BL,
    CARD_DRAG_M
};

typedef S32 CARD_DRAG_MODE;

/*
internal functions
*/

_Check_return_
static STATUS
rec_ole_resource(
    P_REC_PROJECTOR p_rec_projector,
    _InRef_     PC_DATA_REF p_data_ref);

/* awful static data */

typedef struct CARD_DRAG_BAGGAGE
{
   S32 reasoncode; /* MUST be first word (for drag code to work) */
   S32 drag_mode;
   SLR slr;
   SKEL_RECT skel_rect_container;
   SKEL_POINT skel_point_start;
}
CARD_DRAG_BAGGAGE;

static CARD_DRAG_BAGGAGE card_drag_baggage = { CB_CODE_PASS_TO_OBJECT + OBJECT_ID_REC };

static void
pixit_rect_deltas_calc(
    _DocuRef_   P_DOCU p_docu,
    P_PIXIT_RECT p_pixit_rect_deltas,
    _In_        S32 delta_x,
    _In_        S32 delta_y)
{
    P_REC_INSTANCE_DATA p_rec_instance = p_object_instance_data_REC(p_docu);
    PIXIT_SIZE pixit_size;
    PIXIT_RECT pixit_rect_deltas;
    PIXIT_RECT pixit_rect;
    BOOL calc_br_x = TRUE;
    BOOL calc_br_y = TRUE;

    pixit_size.cx = skel_rect_width(&p_rec_instance->frame.skel_rect);
    pixit_size.cy = skel_rect_height(&p_rec_instance->frame.skel_rect);

    pixit_rect_deltas.tl.x = 0;
    pixit_rect_deltas.tl.y = 0;
    pixit_rect_deltas.br.x = 0;
    pixit_rect_deltas.br.y = 0;

    {
    SNAP_TO_CLICK_STOP_MODE snap_mode = SNAP_TO_CLICK_STOP_ROUND;

    switch(card_drag_baggage.drag_mode)
    {
    case CARD_DRAG_M:
        {
        /* Move the top-left and the bottom right */
        pixit_rect_deltas.tl.x = delta_x;
        pixit_rect_deltas.tl.y = delta_y;
        pixit_rect_deltas.br.x = delta_x;
        pixit_rect_deltas.br.y = delta_y;

        if(!host_ctrl_pressed())
            snap_mode = SNAP_TO_CLICK_STOP_ROUND_COARSE; /* SKS 15jun95 made it only coarse for movement not resize */

        break;
        }

    case CARD_DRAG_L: pixit_rect_deltas.tl.x = delta_x; break;
    case CARD_DRAG_R: pixit_rect_deltas.br.x = delta_x; break;
    case CARD_DRAG_T: pixit_rect_deltas.tl.y = delta_y; break;
    case CARD_DRAG_B: pixit_rect_deltas.br.y = delta_y; break;

    case CARD_DRAG_TL:
        /* Move the top left */
        pixit_rect_deltas.tl.x = delta_x;
        pixit_rect_deltas.tl.y = delta_y;
        break;

    case CARD_DRAG_BR:
        /* Move the bottom right */
        pixit_rect_deltas.br.x = delta_x;
        pixit_rect_deltas.br.y = delta_y;
        break;

    case CARD_DRAG_BL:
        /* Move the bottom left */
        pixit_rect_deltas.tl.x = delta_x;
        pixit_rect_deltas.br.y = delta_y;
        break;

    case CARD_DRAG_TR:
        /* Move the top right */
        pixit_rect_deltas.br.x = delta_x;
        pixit_rect_deltas.tl.y = delta_y;
        break;
    }

    /* ho ho ho its christmas so let's screw up the drag coordinates down in the guts says Stuart */
    pixit_rect.tl.x = p_rec_instance->frame.skel_rect.tl.pixit_point.x + pixit_rect_deltas.tl.x;
    pixit_rect.tl.y = p_rec_instance->frame.skel_rect.tl.pixit_point.y + pixit_rect_deltas.tl.y;
    pixit_rect.br.x = p_rec_instance->frame.skel_rect.br.pixit_point.x + pixit_rect_deltas.br.x;
    pixit_rect.br.y = p_rec_instance->frame.skel_rect.br.pixit_point.y + pixit_rect_deltas.br.y;

    if(pixit_rect.tl.x > pixit_rect.br.x)
    {
        PIXIT temp = pixit_rect.tl.x;
        pixit_rect.tl.x = pixit_rect.br.x;
        pixit_rect.br.x = temp;
    }

    if(pixit_rect.tl.y > pixit_rect.br.y)
    {
        PIXIT temp = pixit_rect.tl.y;
        pixit_rect.tl.y = pixit_rect.br.y;
        pixit_rect.br.y = temp;
    }

    pixit_rect.tl.x = skel_ruler_snap_to_click_stop(p_docu, 1, pixit_rect.tl.x, snap_mode);

    /*if(pixit_rect.tl.x < card_drag_baggage.skel_rect_container.tl.pixit_point.x)
    {
        PIXIT dx = pixit_rect.tl.x - card_drag_baggage.skel_rect_container.tl.pixit_point.x;
        pixit_rect.tl.x += dx;
        pixit_rect.br.x += dx;
        calc_br_x = FALSE;
    }*/

    pixit_rect.tl.y = skel_ruler_snap_to_click_stop(p_docu, 0, pixit_rect.tl.y, snap_mode);

    /*if(pixit_rect.tl.y < card_drag_baggage.skel_rect_container.tl.pixit_point.y)
    {
        PIXIT dy = pixit_rect.tl.y - card_drag_baggage.skel_rect_container.tl.pixit_point.y;
        pixit_rect.tl.y += dy;
        pixit_rect.br.y += dy;
        calc_br_y = FALSE;
    }*/

    if(CARD_DRAG_M == card_drag_baggage.drag_mode)
    {
        pixit_rect.br.x = pixit_rect.tl.x + pixit_size.cx;
        pixit_rect.br.y = pixit_rect.tl.y + pixit_size.cy;
    }
    else
    {
        if(calc_br_x)
        {
            pixit_rect.br.x = skel_ruler_snap_to_click_stop(p_docu, 1, pixit_rect.br.x, snap_mode);

            /*if(pixit_rect.br.x > card_drag_baggage.skel_rect_container.br.pixit_point.x)
            {
                PIXIT dx = pixit_rect.br.x - card_drag_baggage.skel_rect_container.tl.pixit_point.x;
                pixit_rect.tl.x -= dx;
                pixit_rect.br.x -= dx;
            }*/
        }

        if(calc_br_y)
        {
            pixit_rect.br.y = skel_ruler_snap_to_click_stop(p_docu, 0, pixit_rect.br.y, snap_mode);

            /*if(pixit_rect.br.y > card_drag_baggage.skel_rect_container.br.pixit_point.y)
            {
                PIXIT dy = pixit_rect.br.y - card_drag_baggage.skel_rect_container.br.pixit_point.y;
                pixit_rect.tl.y -= dy;
                pixit_rect.br.y -= dy;
            }*/
        }
    }
    } /*block*/

    pixit_rect_deltas.tl.x = pixit_rect.tl.x - p_rec_instance->frame.skel_rect.tl.pixit_point.x;
    pixit_rect_deltas.tl.y = pixit_rect.tl.y - p_rec_instance->frame.skel_rect.tl.pixit_point.y;
    pixit_rect_deltas.br.x = pixit_rect.br.x - p_rec_instance->frame.skel_rect.br.pixit_point.x;
    pixit_rect_deltas.br.y = pixit_rect.br.y - p_rec_instance->frame.skel_rect.br.pixit_point.y;

    *p_pixit_rect_deltas = pixit_rect_deltas;
}

static void
rec_drag_update(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_SKEL_RECT p_skel_rect)
{
    RECT_FLAGS rect_flags;
    REDRAW_FLAGS redraw_flags;

    REDRAW_FLAGS_CLEAR(redraw_flags);
    /*redraw_flags.show_selection = TRUE;*/ /* Show selection but not text */

    drag_state.rec_static_flag = 1;

    RECT_FLAGS_CLEAR(rect_flags);
    view_update_now(p_docu, UPDATE_PANE_CELLS_AREA, p_skel_rect, rect_flags, redraw_flags, LAYER_SLOT);

    drag_state.rec_static_flag = 0;
}

_Check_return_
extern STATUS
rec_card_design_dragging_started(
    _DocuRef_   P_DOCU p_docu,
    P_SKELEVENT_CLICK p_skelevent_click)
{
    IGNOREPARM(p_skelevent_click);

    cur_change_before(p_docu);

    drag_state.flag = 1;

    drag_state.pixit_rect_deltas.tl.x = 0;
    drag_state.pixit_rect_deltas.tl.y = 0;
    drag_state.pixit_rect_deltas.br.x = 0;
    drag_state.pixit_rect_deltas.br.y = 0;

    rec_drag_update(p_docu, &card_drag_baggage.skel_rect_container); /* stick it on */

    return(STATUS_OK);
}

/* Determine if things have moved significantly
   Disallow movement outside of certain ranges
   Reject movement which puts the frame outside the page on which the card lives
   Reject movement which puts the frame outside the card
   Reject movement which puts the frame inside out
*/

_Check_return_
extern STATUS
rec_card_design_dragging_movement(
    _DocuRef_   P_DOCU p_docu,
    P_SKELEVENT_CLICK p_skelevent_click)
{
    PIXIT_RECT pixit_rect_deltas = drag_state.pixit_rect_deltas;

    if( (p_skelevent_click->skel_point.page_num.x == card_drag_baggage.skel_rect_container.tl.page_num.x) &&
        (p_skelevent_click->skel_point.page_num.y == card_drag_baggage.skel_rect_container.tl.page_num.y) )
    {
        pixit_rect_deltas_calc(p_docu, &pixit_rect_deltas,
            p_skelevent_click->skel_point.pixit_point.x - card_drag_baggage.skel_point_start.pixit_point.x,
            p_skelevent_click->skel_point.pixit_point.y - card_drag_baggage.skel_point_start.pixit_point.y);

        if(0 != memcmp32(&drag_state.pixit_rect_deltas, &pixit_rect_deltas, sizeof32(pixit_rect_deltas)))
        {
            /* Rub out old grey outline */
            rec_drag_update(p_docu, &card_drag_baggage.skel_rect_container);

            /* move to new position */
            drag_state.pixit_rect_deltas = pixit_rect_deltas;

            /* Draw new grey outline */
            rec_drag_update(p_docu, &card_drag_baggage.skel_rect_container);
        }
    }

    return(STATUS_OK);
}

#define FRAME_MIN_HEIGHT 256
#define FRAME_MIN_WIDTH 256

static void
enforce_sanity_on_pixit_rect(
    P_PIXIT_RECT p_pixit_rect)
{
    PIXIT_POINT size;
    size.x = p_pixit_rect->br.x - p_pixit_rect->tl.x;
    size.y = p_pixit_rect->br.y - p_pixit_rect->tl.y;
    if( size.x < FRAME_MIN_WIDTH)
        size.x = FRAME_MIN_WIDTH;
    if( size.y < FRAME_MIN_HEIGHT)
        size.y = FRAME_MIN_HEIGHT;
    if( p_pixit_rect->tl.x < 0)
        p_pixit_rect->tl.x = 0;
    if( p_pixit_rect->tl.y < 0)
        p_pixit_rect->tl.y = 0;
    p_pixit_rect->br.x = p_pixit_rect->tl.x + size.x;
    p_pixit_rect->br.y = p_pixit_rect->tl.y + size.y;
}

_Check_return_
extern STATUS
rec_card_design_dragging_finished(
    _DocuRef_   P_DOCU p_docu,
    P_SKELEVENT_CLICK p_skelevent_click)
{
    P_REC_INSTANCE_DATA p_rec_instance = p_object_instance_data_REC(p_docu);
    PIXIT_RECT pixit_rect_frame;

    IGNOREPARM(p_skelevent_click);

    /* Rub out old grey outline */
    rec_drag_update(p_docu, &card_drag_baggage.skel_rect_container);

    drag_state.flag = 0;

    /* update dropped frame's location */
    p_rec_instance->frame.skel_rect.tl.pixit_point.x += drag_state.pixit_rect_deltas.tl.x;
    p_rec_instance->frame.skel_rect.tl.pixit_point.y += drag_state.pixit_rect_deltas.tl.y;
    p_rec_instance->frame.skel_rect.br.pixit_point.x += drag_state.pixit_rect_deltas.br.x;
    p_rec_instance->frame.skel_rect.br.pixit_point.y += drag_state.pixit_rect_deltas.br.y;

    { /* Find the top left of the containing cell */
    PIXIT_POINT tl;

    tl.x = card_drag_baggage.skel_rect_container.tl.pixit_point.x;
    tl.y = card_drag_baggage.skel_rect_container.tl.pixit_point.y;

    /* Calculate the frame in relative coordinates */
    pixit_rect_frame.tl.x = p_rec_instance->frame.skel_rect.tl.pixit_point.x - tl.x;
    pixit_rect_frame.tl.y = p_rec_instance->frame.skel_rect.tl.pixit_point.y - tl.y;
    pixit_rect_frame.br.x = p_rec_instance->frame.skel_rect.br.pixit_point.x - tl.x;
    pixit_rect_frame.br.y = p_rec_instance->frame.skel_rect.br.pixit_point.y - tl.y;

    /* Sanitise the rectangle to make sure it is of a minimum size and that non of the corners are at negative coordinates */
    enforce_sanity_on_pixit_rect(&pixit_rect_frame);

    /* Update the instance data's copy of the new sanistised position */
    p_rec_instance->frame.skel_rect.tl.pixit_point.x = pixit_rect_frame.tl.x + tl.x;
    p_rec_instance->frame.skel_rect.tl.pixit_point.y = pixit_rect_frame.tl.y + tl.y;
    p_rec_instance->frame.skel_rect.br.pixit_point.x = pixit_rect_frame.br.x + tl.x;
    p_rec_instance->frame.skel_rect.br.pixit_point.y = pixit_rect_frame.br.y + tl.y;
    } /*block*/

    {
    P_REC_PROJECTOR p_rec_projector = p_rec_projector_from_data_ref(p_docu, &p_rec_instance->frame.data_ref);

    { /* Write the new position into the frame */
    P_REC_FRAME p_rec_frame = p_rec_frame_from_field_id(&p_rec_projector->h_rec_frames, field_id_from_rec_data_ref(&p_rec_instance->frame.data_ref));

    if(p_rec_instance->frame.data_ref.data_space == DATA_DB_TITLE)
        memcpy32(&p_rec_frame->pixit_rect_title, &pixit_rect_frame, sizeof32(pixit_rect_frame));
    else
        memcpy32(&p_rec_frame->pixit_rect_field, &pixit_rect_frame, sizeof32(pixit_rect_frame));
    } /*block*/

    reformat_from_row(p_docu, p_rec_projector->rec_docu_area.tl.slr.row, REFORMAT_Y);

    rec_recompute_card_size(p_rec_projector);

    /* Brute force method: We need to rub out the drag box and then redraw all the cards */
    trash_caches_for_projector(p_rec_projector);

    rec_update_projector(p_rec_projector);
    } /*block*/

    cur_change_after(p_docu);

    docu_modify(p_docu);

    return(STATUS_OK);
}

_Check_return_
extern STATUS
rec_card_design_dragging_aborted(
    _DocuRef_   P_DOCU p_docu,
    P_SKELEVENT_CLICK p_skelevent_click)
{
    IGNOREPARM(p_skelevent_click);

    /* Rub out old grey outline */
    rec_drag_update(p_docu, &card_drag_baggage.skel_rect_container);

    drag_state.flag = 0;

    /* Brute force method: We only need to rub out the drag box */
    /*view_update_all(p_docu, UPDATE_PANE_CELLS_AREA);*/

    /*rec_update_cell(p_docu, &card_drag_baggage.slr);*/

    cur_change_after(p_docu);

    return(STATUS_OK);
}

_Check_return_
extern STATUS
rec_card_design_drag(
    _DocuRef_   P_DOCU p_docu,
    P_SKELEVENT_CLICK p_skelevent_click)
{
    STATUS status = STATUS_OK;
    P_REC_INSTANCE_DATA p_rec_instance = p_object_instance_data_REC(p_docu);
    SLR slr;
    SKEL_POINT tl;
    PIXIT_POINT pixit_point;

    if(!p_rec_instance->frame.on)
        return(STATUS_OK);

    if(status_fail(slr_owner_from_skel_point(p_docu, &slr, &tl, &p_skelevent_click->skel_point, ON_ROW_EDGE_GO_UP)))
        return(STATUS_OK);

    pixit_point.x = p_skelevent_click->skel_point.pixit_point.x - tl.pixit_point.x;
    pixit_point.y = p_skelevent_click->skel_point.pixit_point.y - tl.pixit_point.y;

    /* If there is an active frame, see if a click hits it or its ears */
    if(status_ok(status /*what we hit*/ = card_mode_pixit(p_docu, &p_rec_instance->frame.data_ref, &pixit_point, &p_skelevent_click->click_context.one_program_pixel)))
    {
        p_skelevent_click->processed = 1;

        card_drag_baggage.drag_mode = status;

        card_drag_baggage.slr = slr;

        card_drag_baggage.skel_point_start = p_skelevent_click->skel_point;

        skel_rect_from_slr(p_docu, &card_drag_baggage.skel_rect_container, &card_drag_baggage.slr); /* make the skel rect of the whole cell */

        host_drag_start(&card_drag_baggage);
    }

    return(STATUS_OK);
}

_Check_return_
extern STATUS
rec_card_design_double_click(
    _DocuRef_   P_DOCU p_docu,
    P_SKELEVENT_CLICK p_skelevent_click)
{
    SLR slr;
    SKEL_POINT tl;
    DATA_REF data_ref;
    P_REC_PROJECTOR p_rec_projector;
    SKEL_RECT skel_rect;
    RECT_FLAGS rect_flags;
    RECT_FLAGS_CLEAR(rect_flags);

    if(status_fail(slr_owner_from_skel_point(p_docu, &slr, &tl, &p_skelevent_click->skel_point, ON_ROW_EDGE_GO_UP)))
        return(STATUS_OK);

    if(NULL == (p_rec_projector = rec_data_ref_from_slr(p_docu, &slr, &data_ref)))
        return(STATUS_OK);

    if(PROJECTOR_TYPE_CARD != p_rec_projector->projector_type)
        return(STATUS_OK);

    /* This is reliant upon the preceeding single click to have set up this field id */
    set_rec_data_ref_from_object_position(p_docu, &data_ref, &p_docu->cur.object_position);

    /* Construct the skel_rect of the frame */
    if(status_ok(rec_skel_rect_from_data_ref(p_rec_projector, &skel_rect, &data_ref)))
    {
        P_FIELDDEF p_fielddef = p_fielddef_from_field_id(&p_rec_projector->opendb.table.h_fielddefs, data_ref.arg.db_field.field_id);
        P_REC_INSTANCE_DATA p_rec_instance = p_object_instance_data_REC(p_docu);

        status_line_setf(p_docu, STATUS_LINE_LEVEL_AUTO_CLEAR, (DATA_DB_TITLE == data_ref.data_space) ? REC_MSG_STATUS_LINE_TITLE : REC_MSG_STATUS_LINE_FIELD, p_fielddef->name);

        p_skelevent_click->processed = 1;

#if RISCOS
        /* CTRL key down then OLE it ??? */
        if(host_ctrl_pressed())
        {
            switch(p_fielddef->type)
            {
            case FIELD_FILE:
            case FIELD_PICTURE:
                return(rec_ole_resource(p_rec_projector, &data_ref));

            default:
                break;
            }

            return(STATUS_OK);
        }
#endif

        p_rec_instance->frame.on = 1;
        p_rec_instance->frame.data_ref = data_ref;
        p_rec_instance->frame.skel_rect = skel_rect;

        /* Make room for the ears */
        skel_rect.tl.pixit_point.x -= CARD_EAR_WIDTH;
        skel_rect.tl.pixit_point.y -= CARD_EAR_HEIGHT;

        skel_rect.br.pixit_point.x += CARD_EAR_WIDTH;
        skel_rect.br.pixit_point.y += CARD_EAR_HEIGHT;

        view_update_later(p_docu, UPDATE_PANE_CELLS_AREA, &skel_rect, rect_flags);

#if TITLES_TOO
        if(DATA_DB_FIELD == data_ref.data_space)
        {
            data_ref.data_space = DATA_DB_TITLE;

            if(status_ok(rec_skel_rect_from_data_ref(p_rec_projector, &skel_rect, &data_ref)))
            {
                /* Make room for the ears */
                skel_rect.tl.pixit_point.x -= CARD_EAR_WIDTH;
                skel_rect.tl.pixit_point.y -= CARD_EAR_HEIGHT;

                skel_rect.br.pixit_point.x += CARD_EAR_WIDTH;
                skel_rect.br.pixit_point.y += CARD_EAR_HEIGHT;

                view_update_later(p_docu, UPDATE_PANE_CELLS_AREA, &skel_rect, rect_flags);
            }
        }
#endif
    }

    return(STATUS_OK);
}

#if RISCOS

static void
rec_popup_state_dispose(void)
{
    ri_lbox_dispose(&lbox);
    ui_source_dispose(&ui_source);
}

_Check_return_
static STATUS
dialog_riscos_event_lbn_click(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     DIALOG_MESSAGE dialog_message,
    /*_Inout_*/ P_ANY p_data)
{
    STATUS status = STATUS_OK;
    P_DIALOG_MSG_LBN_CLK p_dialog_msg_lbn_clk = (P_DIALOG_MSG_LBN_CLK) p_data;
    S32 selected_item = p_dialog_msg_lbn_clk->selected_item;

    switch(dialog_message)
    {
    default: default_unhandled();
#if CHECKING
    case DIALOG_MSG_CODE_LBN_SGLCLK:
    case DIALOG_MSG_CODE_LBN_SELCHANGE:
#endif
        {
        if(dialog_message == DIALOG_MSG_CODE_LBN_SGLCLK)
        {
            P_UI_TEXT p_ui_text = array_ptr(&ui_source.source.array_handle, UI_TEXT, selected_item);
            PC_USTR ustr = ui_text_ustr(p_ui_text);

            if(*ustr)
            {
                SKELEVENT_KEYS skelevent_keys;
                QUICK_UBLOCK quick_ublock;
                quick_ublock_setup_fill_from_const_ubuf(&quick_ublock, ustr, ustrlen32(ustr));

                skelevent_keys.p_quick_ublock = &quick_ublock;
                skelevent_keys.processed = 0;

                status =  object_skel(p_docu, T5_EVENT_KEYS, &skelevent_keys);
            }

            rec_popup_state_dispose();
        }

        break;
        }
    }

    return status;
}

_Check_return_
static STATUS
dialog_riscos_event_lbn_destroy(
    /*_Inout_*/ P_ANY p_data)
{
    P_DIALOG_MSG_LBN_DESTROY p_dialog_msg_lbn_destroy = (P_DIALOG_MSG_LBN_DESTROY) p_data;
    IGNOREPARM(p_dialog_msg_lbn_destroy);
    rec_popup_state_dispose();
    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_riscos_event_lbn_focus(
    /*_Inout_*/ P_ANY p_data)
{
    P_DIALOG_MSG_LBN_FOCUS p_dialog_msg_lbn_focus = (P_DIALOG_MSG_LBN_FOCUS) p_data;
    IGNOREPARM(p_dialog_msg_lbn_focus);
    return(STATUS_OK);
}

_Check_return_
static STATUS
dialog_riscos_event_lbn_key(
    /*_Inout_*/ P_ANY p_data)
{
    P_DIALOG_MSG_LBN_KEY p_dialog_msg_lbn_key = (P_DIALOG_MSG_LBN_KEY) p_data;
    p_dialog_msg_lbn_key->processed = 0;
    return(STATUS_OK);
}

PROC_DIALOG_EVENT_PROTO(static, lbn_event_popup)
{
    switch(dialog_message)
    {
    case DIALOG_MSG_CODE_LBN_DESTROY:
        return(dialog_riscos_event_lbn_destroy(p_data));

    case DIALOG_MSG_CODE_LBN_DBLCLK:
    case DIALOG_MSG_CODE_LBN_SGLCLK:
    case DIALOG_MSG_CODE_LBN_SELCHANGE:
        return(dialog_riscos_event_lbn_click(p_docu, dialog_message, p_data));

    case DIALOG_MSG_CODE_LBN_FOCUS:
        return(dialog_riscos_event_lbn_focus(p_data));

    case DIALOG_MSG_CODE_LBN_KEY:
        return(dialog_riscos_event_lbn_key(p_data));

    default:
        return(STATUS_OK);
    }
}

#endif /* RISCOS */

_Check_return_
static STATUS
rec_vl_popup(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _In_z_      PC_U8Z p_value_list,
    P_PIXIT_POINT p_pixit_point)
{
    STATUS status = STATUS_OK;
#if RISCOS
    static const UI_TEXT caption = UI_TEXT_INIT_RESID(REC_MSG_VALUE_CAPTION);
    int x = 0;
    int y = 0;
    GDI_POINT gdi_point;

    /* get rid of any that may still be in existance */
    rec_popup_state_dispose();

    window_point_from_pixit_point(&gdi_point, p_pixit_point, &p_view->host_xform[WIN_PANE]);

    for(;;) /* loop for structure */
    {
        PC_U8Z p_u8 = p_value_list;

        if(!ui_text_is_blank(&caption))
            x = (GDI_COORD) (ui_width_from_tstr(ui_text_tstr(&caption)) / PIXITS_PER_RISCOS);

        for(;;)
        {
            PC_U8Z p_u8_next = strchr(p_u8, CH_COMMA);
            P_UI_TEXT p_ui_text;
            P_U8 p_u8_alloc;
            U32 len;
            GDI_COORD width;

            if(NULL == (p_ui_text = al_array_extend_by_UI_TEXT(&ui_source.source.array_handle, 1, &array_init_block_ui_text, &status)))
                break;

            ui_source.type = UI_SOURCE_TYPE_ARRAY;

            len = (NULL != p_u8_next) ? (p_u8_next - p_u8) : strlen32(p_u8);

            if(NULL == (p_u8_alloc = al_ptr_alloc_bytes(P_U8Z, len + 1 /*CH_NULL*/, &status)))
                break;

            memcpy32(p_u8_alloc, p_u8, len);
            p_u8_alloc[len] = CH_NULL;

            p_ui_text->type = UI_TEXT_TYPE_TSTR_ALLOC;
            p_ui_text->text.tstr = p_u8_alloc;

            width = (GDI_COORD) (ui_width_from_tstr(p_ui_text->text.tstr) / PIXITS_PER_RISCOS);

            x = MAX(x, width);
            y += RI_LBOX_ITEM_HEIGHT;

            if(NULL == p_u8_next)
                break;

            p_u8 = p_u8_next + 1;
        }

        status_break(status);

        {
        const wimp_w hwnd = p_view->pane[p_view->cur_pane].hwnd;
        RI_LBOX_NEW _ri_lbox_new;
        GDI_POINT gdi_org;

        zero_struct(_ri_lbox_new);

        host_gdi_org_from_screen(&gdi_org, hwnd); /* window w.a. ABS origin */

        _ri_lbox_new.bbox.xmin = gdi_org.x + gdi_point.x;
        _ri_lbox_new.bbox.ymax = gdi_org.y + gdi_point.y;
        _ri_lbox_new.bbox.xmax = _ri_lbox_new.bbox.xmin + x;
        _ri_lbox_new.bbox.ymin = _ri_lbox_new.bbox.ymax - y;

        _ri_lbox_new.bbox.xmax += 2*8;
        _ri_lbox_new.bbox.ymin -= 2*4;
        _ri_lbox_new.bbox.ymax += 44;

        _ri_lbox_new.caption = &caption;

        _ri_lbox_new.p_docu = p_docu;
        _ri_lbox_new.p_proc_dialog_event = lbn_event_popup;
        _ri_lbox_new.client_handle = NULL;

        _ri_lbox_new.bits.disable_double = TRUE;

        _ri_lbox_new.p_ui_control = NULL;
        _ri_lbox_new.p_ui_source = &ui_source;

        _ri_lbox_new.ui_data_type = UI_DATA_TYPE_TEXT;

        status_break(status = ri_lbox_new(&lbox, &_ri_lbox_new));

        ri_lbox_make_child(lbox, hwnd, NULL);

        ri_lbox_show_submenu(lbox, RI_LBOX_SUBMENU_MENU, NULL);

        (void) ri_lbox_selection_set_from_itemno(lbox, 0);

        ri_lbox_focus_set(lbox);
        } /*block*/

        return(STATUS_OK);
        /*NOTREACHED*/
    }

    rec_popup_state_dispose();

#else
    IGNOREPARM_DocuRef_(p_docu);
    IGNOREPARM_ViewRef_(p_view);
    IGNOREPARM(p_value_list);
    IGNOREPARM(p_pixit_point);
#endif

    return(status);
}

_Check_return_
extern STATUS
rec_card_design_single_click(
    _DocuRef_   P_DOCU p_docu,
    P_SKELEVENT_CLICK p_skelevent_click)
{
    SLR slr;
    SKEL_POINT tl;
    P_REC_PROJECTOR p_rec_projector;
    DATA_REF data_ref;
    PIXIT_POINT pixit_point;

    if(status_fail(slr_owner_from_skel_point(p_docu, &slr, &tl, &p_skelevent_click->skel_point, ON_ROW_EDGE_GO_UP)))
        return(STATUS_OK);

    if(NULL == (p_rec_projector = rec_data_ref_from_slr(p_docu, &slr, &data_ref)))
        return(STATUS_OK);

    if(PROJECTOR_TYPE_CARD != p_rec_projector->projector_type)
        return(STATUS_OK); /* always let ce_edit handle sheet mode clicks */

    pixit_point.x = p_skelevent_click->skel_point.pixit_point.x - tl.pixit_point.x;
    pixit_point.y = p_skelevent_click->skel_point.pixit_point.y - tl.pixit_point.y;

    {
    P_REC_INSTANCE_DATA p_rec_instance = p_object_instance_data_REC(p_docu);

    if(p_rec_instance->frame.on)
    {
        SKEL_RECT skel_rect;
        SLR slr_f;
        RECT_FLAGS rect_flags;
        RECT_FLAGS_CLEAR(rect_flags);

        /* ought to check the slr vs the p_rec_instance->frame.data_ref to make sure it is the same one! */

        rec_slr_from_data_ref(p_docu, &p_rec_instance->frame.data_ref, &slr_f);

        if(slr_f.row == slr.row && slr_f.col == slr.col)
        {
            /* If there is an active frame, see if a click hits it or its ears */
            if(status_ok(card_mode_pixit(p_docu, &p_rec_instance->frame.data_ref, &pixit_point, &p_skelevent_click->click_context.one_program_pixel)))
            {
                p_skelevent_click->processed = 1; /* yep, we hit it (but that's not really interesting) */
                return(STATUS_OK);
            }
        }

        /* missed the active frame so cancel it */
        p_rec_instance->frame.on = 0;

        skel_rect = p_rec_instance->frame.skel_rect;

        /* Make room for the ears */
        skel_rect.tl.pixit_point.x -= CARD_EAR_WIDTH;
        skel_rect.tl.pixit_point.y -= CARD_EAR_HEIGHT;

        skel_rect.br.pixit_point.x += CARD_EAR_WIDTH;
        skel_rect.br.pixit_point.y += CARD_EAR_HEIGHT;

        view_update_later(p_docu, UPDATE_PANE_CELLS_AREA, &skel_rect, rect_flags);

#if TITLES_TOO
        if(DATA_DB_FIELD == p_rec_instance->frame.data_ref.data_space)
        {
            DATA_REF data_ref = p_rec_instance->frame.data_ref;

            data_ref.data_space = DATA_DB_TITLE;

            if(status_ok(rec_skel_rect_from_data_ref(p_rec_projector, &skel_rect, &data_ref)))
            { /* Make room for the ears */
                skel_rect.tl.pixit_point.x -= CARD_EAR_WIDTH;
                skel_rect.tl.pixit_point.y -= CARD_EAR_HEIGHT;

                skel_rect.br.pixit_point.x += CARD_EAR_WIDTH;
                skel_rect.br.pixit_point.y += CARD_EAR_HEIGHT;

                view_update_later(p_docu, UPDATE_PANE_CELLS_AREA, &skel_rect, rect_flags);
            }
        }
#endif

        /* and then revert to normal click stuff */
    }
    } /*block*/

    { /* click hits a frame in this card? SKS 25jul95 work from the front to the back */
    ARRAY_INDEX i = array_elements(&p_rec_projector->h_rec_frames);

    while(--i >= 0)
    {
        PC_REC_FRAME p_rec_frame = array_ptrc(&p_rec_projector->h_rec_frames, REC_FRAME, i);
        PC_FIELDDEF p_fielddef;

        if(rec_pixit_point_in_pixit_rect(&pixit_point, &p_rec_frame->pixit_rect_field))
            return(STATUS_OK); /* let ce_edit take care of higher levels of clicking/setting object position etc in field */

        if(rec_pixit_point_in_pixit_rect(&pixit_point, &p_rec_frame->pixit_rect_title))
            return(STATUS_OK); /* let ce_edit take care of higher levels of clicking/setting object position etc in title */

        p_fielddef = p_fielddef_from_field_id(&p_rec_projector->opendb.table.h_fielddefs, p_rec_frame->field_id);

        if(strlen(p_fielddef->value_list))
        {
            PIXIT_RECT pixit_rect_temp;

            pixit_rect_temp.tl.x = p_rec_frame->pixit_rect_field.br.x;
            pixit_rect_temp.tl.y = p_rec_frame->pixit_rect_field.tl.y;

            pixit_rect_temp.tl.x += p_skelevent_click->click_context.one_program_pixel.x;

            pixit_rect_temp.br = pixit_rect_temp.tl;

            pixit_rect_temp.br.x += DEFAULT_GRIGHT_SIZE;
            pixit_rect_temp.br.y += DEFAULT_GRIGHT_SIZE;

            if(rec_pixit_point_in_pixit_rect(&pixit_point, &pixit_rect_temp))
            {
                PC_FIELDDEF p_fielddef = p_fielddef_from_field_id(&p_rec_projector->opendb.table.h_fielddefs, p_rec_frame->field_id);
                SKEL_RECT skel_rect_frame;

                set_field_id_for_rec_data_ref(&data_ref, p_fielddef->id);

                /* Construct the skel_rect of the frame */
                if(status_fail(rec_skel_rect_from_data_ref(p_rec_projector, &skel_rect_frame, &data_ref)))
                    return(STATUS_OK);

                p_skelevent_click->processed = 1;

                { /* Let's mark the field so its contents get overwritten */
                DOCU_AREA docu_area_marker;

                docu_area_init(&docu_area_marker);
                docu_area_marker.tl.slr = slr;
                docu_area_marker.br.slr.col = docu_area_marker.tl.slr.col + 1;
                docu_area_marker.br.slr.row = docu_area_marker.tl.slr.row + 1;

                docu_area_marker.tl.object_position.object_id = OBJECT_ID_REC;
                set_rec_object_position_from_data_ref(p_docu, &docu_area_marker.tl.object_position, &data_ref);
                docu_area_marker.tl.object_position.data = 0;

                docu_area_marker.br.object_position = docu_area_marker.tl.object_position;
                docu_area_marker.br.object_position.data = 0x7FFFFFFF;

                status_consume(object_call_id(OBJECT_ID_CELLS, p_docu, T5_MSG_SELECTION_MAKE, &docu_area_marker));
                } /*block*/

#if RISCOS
                rec_vl_popup(p_docu, p_skelevent_click->click_context.p_view, p_fielddef->value_list, &skel_rect_frame.tl.pixit_point);
#endif
            }
        }

    }
    } /*block*/

    /* SKS 24jul95 - missed all the frames. must stop ce_edit from further processing or he'll do summats funny with the position */
    p_skelevent_click->processed = 1;
    return(STATUS_OK);
}

_Check_return_
extern STATUS
card_mode_pixit(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DATA_REF p_data_ref,
    _InRef_     PC_PIXIT_POINT p_pixit_point,
    _InRef_     PC_PIXIT_POINT p_one_program_pixel)
{
    PIXIT_RECT pixit_rect_frame;
    PIXIT_RECT_EARS ears;
    STATUS position = STATUS_FAIL;

    {
    P_REC_PROJECTOR p_rec_projector = p_rec_projector_from_data_ref(p_docu, p_data_ref);
    PC_REC_FRAME p_rec_frame;

    if(NULL == p_rec_projector)
        return(position);

    p_rec_frame = p_rec_frame_from_field_id(&p_rec_projector->h_rec_frames, field_id_from_rec_data_ref(p_data_ref));

    if(P_DATA_NONE == p_rec_frame)
        return(position);

    if(p_data_ref->data_space == DATA_DB_TITLE)
        pixit_rect_frame = p_rec_frame->pixit_rect_title;
    else
        pixit_rect_frame = p_rec_frame->pixit_rect_field;
    } /*block*/

    pixit_rect_get_ears(&ears, &pixit_rect_frame, p_one_program_pixel);

    if(POINT_INSIDE_RECTANGLE(p_pixit_point, ears.outer_bound))
    {
        STATUS note_position = PIXIT_RECT_EAR_COUNT;

        position = PIXIT_RECT_EAR_CENTRE; /* even if missed everything else! */

        while(--note_position >= 0)
            if(ears.ear_active[note_position])
                if(POINT_INSIDE_RECTANGLE(p_pixit_point, ears.ear[note_position]))
                {
                    position = note_position;
                    break;
                }
    }

    switch(position)
    {
    default: break;
    case PIXIT_RECT_EAR_TL:     position = CARD_DRAG_TL; break;
    case PIXIT_RECT_EAR_TC:     position = CARD_DRAG_T;  break;
    case PIXIT_RECT_EAR_TR:     position = CARD_DRAG_TR; break;
    case PIXIT_RECT_EAR_LC:     position = CARD_DRAG_L;  break;
    case PIXIT_RECT_EAR_CENTRE: position = CARD_DRAG_M;  break;
    case PIXIT_RECT_EAR_RC:     position = CARD_DRAG_R;  break;
    case PIXIT_RECT_EAR_BL:     position = CARD_DRAG_BL; break;
    case PIXIT_RECT_EAR_BC:     position = CARD_DRAG_B;  break;
    case PIXIT_RECT_EAR_BR:     position = CARD_DRAG_BR; break;
    }

    return(position);
}

_Check_return_
extern T5_FILETYPE
rec_resource_filetype(
    P_REC_RESOURCE p_rec_resource)
{
    T5_FILETYPE t5_filetype = p_rec_resource->t5_filetype;

    if(t5_filetype == FILETYPE_LINKED_FILE) /* filetype_Link */
    {
        TCHARZ picture_namebuf[BUF_MAX_PATHSTRING];
        TCHARZ buffer[BUF_MAX_PATHSTRING];

        if(p_rec_resource->ev_data.arg.string.size < elemof32(buffer))
        {
            tstr_xstrnkpy(buffer, elemof32(buffer), p_rec_resource->ev_data.arg.string.uchars, p_rec_resource->ev_data.arg.string.size);

            if(file_find_on_path(picture_namebuf, elemof32(picture_namebuf), buffer) > 0)
            {
                t5_filetype = host_t5_filetype_from_file(picture_namebuf);
            }
        }
    }

    return(t5_filetype);
}

/*
OLE functionality could be build into a separate object

The idea is to allow the user to fire up and edit files which are either
linked to or embedded by ctrl-double-clicking to launch them into their
associated editors and to catch them being resaved using an upcall handler.

If this were done we could use it for files/picture in a database AND
arbitrary things in the note layer.

This is what the latest Genesis has done for about a year and it
a> is very popular with the noise-makers and
b> works with all applications which I have met.

if linked:-
   launch the file (Filer_Run or WIMPTASK which works on RISC OS 2)
   note the fact that the file is active
   on upcalls indicating a save perform any updates

if embeded
   Save the file in a scrapfile
   Launch the file
   note the fact that the file is active
   on upcalls indicating a save reload the file, replacing the original and delete the scrapfile

The upcall(SAVE)handler notes the filename and the program polls this on wimp_events(NULL or otherwise)
and spots fielnames in which it has an interest.

<<< NB a small matter of writing a module to handle this!!!

The note of files being active is on a per document basis. On closing the document you
can delete any scrapfiles which it made.

Problems:

This can still leave scrapfiles lying about : tough but who cares.

Delayed Re-embeding into a record of a database may put it in the wrong record if things
have got shuffled about.
*/

_Check_return_
static STATUS
rec_ole_resource(
    P_REC_PROJECTOR p_rec_projector,
    _InRef_     PC_DATA_REF p_data_ref)
{
    STATUS status = STATUS_OK;
    REC_RESOURCE rec_resource;
    T5_FILETYPE t5_filetype;
    TCHARZ filename_buffer[BUF_MAX_PATHSTRING];

    status_return(rec_read_resource(&rec_resource, p_rec_projector, p_data_ref, FALSE));

    t5_filetype = rec_resource_filetype(&rec_resource);

    if(rec_resource.t5_filetype != FILETYPE_LINKED_FILE) /* filetype_Link */
    {
        /* We must go via a scrapfile */
        P_DOCU p_docu = p_docu_from_docno(p_rec_projector->docno);
        FF_OP_FORMAT ff_op_format = { OP_OUTPUT_INVALID };
        TCHARZ scrapfile[BUF_MAX_PATHSTRING];
        char * fscrap = tmpnam(scrapfile);

        if(NULL == fscrap)
            return(STATUS_FAIL);

        tstr_xstrkpy(filename_buffer, elemof32(filename_buffer), fscrap);

        /* Save the data to the scrapfile */
        if(status_ok(status = foreign_initialise_save(p_docu, &ff_op_format, NULL, filename_buffer, t5_filetype, NULL)))
        {
            assert(0 != rec_resource.ev_data.arg.db_blob.size);

            status = binary_write_bytes(&ff_op_format.of_op_format.output, rec_resource.ev_data.arg.db_blob.data, rec_resource.ev_data.arg.db_blob.size);

            status = foreign_finalise_save(&p_docu, &ff_op_format, status);
        }
    }
    else
    {
        /* Just go for it */
        memcpy32(filename_buffer, rec_resource.ev_data.arg.string.uchars, MIN(rec_resource.ev_data.arg.string.size, sizeof32(filename_buffer)));
        filename_buffer[rec_resource.ev_data.arg.string.size] = CH_NULL;
    }

    if(status_ok(status))
        filer_launch(filename_buffer);

    ss_data_free_resources(&rec_resource.ev_data);

    return(status);
}

/* end of ed_card.c */
