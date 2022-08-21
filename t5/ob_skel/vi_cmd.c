/* vi_cmd.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* View specific commands for Fireworkz */

/* RCM July 1992 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#include "ob_toolb/xp_toolb.h"

#ifndef         __xp_skeld_h
#include "ob_skel/xp_skeld.h"
#endif

#if RISCOS
#include "ob_skel/xp_skelr.h"
#endif

/*
internal structure
*/

typedef struct VIEW_CREATE
{
    DOCNO docno;
    PIXIT_POINT tl;
    PIXIT_SIZE size;

    P_VIEW p_view; /*OUT*/
}
VIEW_CREATE, * P_VIEW_CREATE;

/*
internal routines
*/

static void
calc_paneextent(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view);

static void
view_set_zoomfactor(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InVal_     S32 scalet,
    _InVal_     S32 scaleb);

/* SKS after 1.03 18May93 - makes binding margin additive onto the left margin unless the alternate switch is set */

_Check_return_
extern PIXIT
margin_left_from(
    P_PAGE_DEF p_page_def,
    _InVal_     PAGE page_num_y)
{
    PIXIT margin;

    if(p_page_def->margin_bind && p_page_def->margin_oe_swap)
    {
        PIXIT max_lm_rm = MAX(p_page_def->margin_left, p_page_def->margin_right);

        if(page_num_y & 1)
            margin = max_lm_rm;
        else
            margin = MAX(max_lm_rm, p_page_def->margin_bind);
    }
    else
        margin = p_page_def->margin_left + p_page_def->margin_bind;

    return(margin);
}

_Check_return_
extern PIXIT
margin_right_from(
    P_PAGE_DEF p_page_def,
    _InVal_     PAGE page_num_y)
{
    PIXIT margin;

    if(p_page_def->margin_bind && p_page_def->margin_oe_swap)
    {
        PIXIT max_lm_rm = MAX(p_page_def->margin_left, p_page_def->margin_right);

        if(page_num_y & 1)
            margin = MAX(max_lm_rm, p_page_def->margin_bind);
        else
            margin = max_lm_rm;
    }
    else
        margin = p_page_def->margin_right;

    return(margin);
}

/******************************************************************************
*
* set the given pane as current
*
******************************************************************************/

T5_CMD_PROTO(extern, t5_cmd_current_pane)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 1);
    const P_VIEW p_view = p_view_from_viewno_caret(p_docu);

    IGNOREPARM_InVal_(t5_message);

    if(!IS_VIEW_NONE(p_view))
        p_view->cur_pane = ENUM_PACK(PANE_ID, p_args[0].val.s32);

    return(STATUS_OK);
}

/******************************************************************************
*
* set the given view as current
*
******************************************************************************/

T5_CMD_PROTO(extern, t5_cmd_current_view)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 1);
    VIEWNO viewno = (VIEWNO) p_args[0].val.s32;
    P_VIEW p_view = p_view_from_viewno(p_docu, viewno);

    IGNOREPARM_InVal_(t5_message);

    if(!IS_VIEW_NONE(p_view))
        p_view = p_view_from_viewno_caret(p_docu);

    return(command_set_current_view(p_docu, p_view)); /* in case we're recording a replayed sequence */
}

extern void
view_destroy(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view)
{
    DOCU_ASSERT(p_docu);
    VIEW_ASSERT(p_view);

    { /* inform interested parties that the view is just about to be destroyed */
    T5_MSG_VIEW_DESTROY_BLOCK t5_msg_view_destroy;
    t5_msg_view_destroy.p_view = p_view;
    status_assert(object_call_all_accumulate(p_docu, T5_MSG_VIEW_DESTROY, &t5_msg_view_destroy));
    status_assert(maeve_event(p_docu, T5_MSG_VIEW_DESTROY, &t5_msg_view_destroy));
    } /*block*/

    /* host specific window destruction */
    host_view_destroy(p_docu, p_view);

    /* mark view as unused */
    p_view->flags.deleted = 1;

    --p_docu->n_views;

    status_assert(maeve_event(p_docu, T5_MSG_DOCU_VIEWCOUNT, P_DATA_NONE));
}

/******************************************************************************
*
* find a space in the documents view table,
* or add an entry to create a space
*
******************************************************************************/

/* this proc knows about the handle 0 avoidance, rest of knowledge is in view.h */

_Check_return_
static STATUS
viewno_find_entry(
    _DocuRef_   P_DOCU p_docu,
    _OutRef_    P_VIEWNO p_viewno)
{
    *p_viewno = VIEWNO_NONE;

    if(array_elements(&p_docu->h_view_table))
    {
        const ARRAY_INDEX n_views = array_elements(&p_docu->h_view_table);
        ARRAY_INDEX viewix;
        P_VIEW p_view = array_range(&p_docu->h_view_table, VIEW, 0, n_views);

        for(viewix = 0; viewix < n_views; ++viewix)
        {
            if(!view_void_entry(p_view))
                continue;

            zero_struct_ptr(p_view);
            *p_viewno = p_view->viewno = (VIEWNO) (array_indexof_element(&p_docu->h_view_table, VIEW, p_view) + VIEWNO_FIRST);
            return(STATUS_OK);
        }
    }

    if(array_elements(&p_docu->h_view_table) >= VIEWNO_MAX - VIEWNO_FIRST)
        return(status_nomem());

    {
    STATUS status;
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(VIEW), TRUE);
    const P_VIEW p_view = al_array_extend_by(&p_docu->h_view_table, VIEW, 1, &array_init_block, &status);

    if(NULL == p_view)
        return(status);

    trace_2(TRACE_APP_SKEL,
            TEXT("documents view table realloced, now: ") U32_TFMT TEXT(" entries, ") U32_TFMT TEXT(" bytes"),
            array_elements32(&p_docu->h_view_table),
            array_elements32(&p_docu->h_view_table) * sizeof32(*p_view));

    *p_viewno = p_view->viewno = (VIEWNO) (array_indexof_element(&p_docu->h_view_table, VIEW, p_view) + VIEWNO_FIRST);
    return(STATUS_OK);
    } /*block*/
}

/******************************************************************************
*
* create a new view structure
*
******************************************************************************/

_Check_return_
static STATUS
view_new_create(
    /*inout*/ P_VIEW_CREATE p_view_create)
{
    const P_DOCU p_docu = p_docu_from_docno(p_view_create->docno);
    VIEWNO viewno;
    STATUS status;

    p_view_create->p_view = NULL;

    if(status_ok(status = viewno_find_entry(p_docu, &viewno)))
    {
        const P_VIEW p_view = array_ptr(&p_docu->h_view_table, VIEW, (ARRAY_INDEX) viewno - VIEWNO_FIRST); /* handle 0 avoidance */

        ++p_docu->n_views;

        p_view->docno = p_view_create->docno;

        zero_array(p_view->main);
        zero_array(p_view->edge);
        zero_array(p_view->pane);

        p_view->cur_pane = WIN_PANE;

        p_view->scalet = 1;
        p_view->scaleb = 1;

        view_cache_info(p_view);

        calc_paneextent(p_docu, p_view);

        if(status_fail(status = host_view_init(p_docu, p_view, &p_view_create->tl, &p_view_create->size)))
        {
            status_assertc(status);
            view_destroy(p_docu, p_view);
            return(status);
        }

        { /* inform interested parties of view creation */
        T5_MSG_VIEW_NEW_BLOCK t5_msg_view_new;

        t5_msg_view_new.p_view = p_view;
        t5_msg_view_new.status = STATUS_OK;

        status_assert(object_call_all(p_docu, T5_MSG_VIEW_NEW, &t5_msg_view_new));
        status_assert(maeve_event(p_docu, T5_MSG_VIEW_NEW, &t5_msg_view_new));

        /* someone out there can't cope? */
        if(status_fail(status = t5_msg_view_new.status))
        {
            status_assertc(status);
            view_destroy(p_docu, p_view);
            return(status);
        }
        } /*block*/

        status_assert(maeve_event(p_docu, T5_MSG_DOCU_VIEWCOUNT, P_DATA_NONE));

        host_view_show(p_docu, p_view);

        p_view_create->p_view = p_view;
    }

    return(status);
}

_Check_return_
extern VIEWNO
viewno_from_p_view_fn(
    _ViewRef_maybenone_ PC_VIEW p_view)
{
    if(IS_VIEW_NONE(p_view))
        return(VIEWNO_NONE);

    return(viewno_from_p_view(p_view));
}

_Check_return_
extern S32
view_return_extid(
    _DocuRef_   PC_DOCU p_docu,
    _ViewRef_   PC_VIEW p_view)
{
    IGNOREPARM_DocuRef_(p_docu);
    return((S32) viewno_from_p_view_fn(p_view));
}

/******************************************************************************
*
* view control dialog
*
******************************************************************************/

enum VIEW_CONTROL_IDS
{
    VIEW_CONTROL_ID_STT = 30,

    VIEW_CONTROL_ID_NEW_VIEW,

    VIEW_CONTROL_ID_RULER_BORDER_GROUP = 40,
    VIEW_CONTROL_ID_BORDER_HORZ,
    VIEW_CONTROL_ID_BORDER_VERT,
    VIEW_CONTROL_ID_RULER_HORZ,
    VIEW_CONTROL_ID_RULER_VERT,

    VIEW_CONTROL_ID_ZOOM_GROUP = 50,
    VIEW_CONTROL_ID_ZOOM_BUMP,
    VIEW_CONTROL_ID_ZOOM_FIT_H,
    VIEW_CONTROL_ID_ZOOM_FIT_V,
    VIEW_CONTROL_ID_ZOOM_1,
    VIEW_CONTROL_ID_ZOOM_2,
    VIEW_CONTROL_ID_ZOOM_3,
    VIEW_CONTROL_ID_ZOOM_4,
    VIEW_CONTROL_ID_ZOOM_5,

    VIEW_CONTROL_ID_DISPLAY_GROUP = 60,
    VIEW_CONTROL_ID_DISPLAY_1,
    VIEW_CONTROL_ID_DISPLAY_2,
    VIEW_CONTROL_ID_DISPLAY_3,

    VIEW_CONTROL_ID_SPLIT_H = 70,
    VIEW_CONTROL_ID_SPLIT_V,

    VIEW_CONTROL_ID_END
};

#if RISCOS
#define VIEW_CONTROL_ZOOM_BUTTONS_H ((16 * 60 * PIXITS_PER_RISCOS) / 8)
#elif WINDOWS
#define VIEW_CONTROL_ZOOM_BUTTONS_H ((18 * 14 * PIXITS_PER_WDU_H) / 8)
#endif
#define VIEW_CONTROL_ZOOM_BUTTONS_V DIALOG_STDPUSHBUTTON_V

#if RISCOS
#define VIEW_CONTROL_ZOOM_BUTTONS_FIT_H DIALOG_PUSHBUTTONOVH_H + DIALOG_SYSCHARSL_H(6)
#elif WINDOWS
#define VIEW_CONTROL_ZOOM_BUTTONS_FIT_H VIEW_CONTROL_ZOOM_BUTTONS_H
#endif

static const DIALOG_CONTROL
view_control_ruler_border_group =
{
    VIEW_CONTROL_ID_RULER_BORDER_GROUP, DIALOG_MAIN_GROUP,
    { DIALOG_CONTROL_PARENT, VIEW_CONTROL_ID_ZOOM_GROUP, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { 0, DIALOG_STDSPACING_V },
    { DRT(LBRB, GROUPBOX) }
};

static const DIALOG_CONTROL
view_control_border_horz =
{
    VIEW_CONTROL_ID_BORDER_HORZ, VIEW_CONTROL_ID_RULER_BORDER_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { 0, 0, DIALOG_CONTENTS_CALC, DIALOG_STDCHECK_V },
    { DRT(LTLT, CHECKBOX) }
};

static const DIALOG_CONTROL_DATA_CHECKBOX
view_control_border_horz_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_VIEW_BORDER_HORZ) };

static const DIALOG_CONTROL
view_control_border_vert =
{
    VIEW_CONTROL_ID_BORDER_VERT, VIEW_CONTROL_ID_RULER_BORDER_GROUP,
    { VIEW_CONTROL_ID_BORDER_HORZ, VIEW_CONTROL_ID_BORDER_HORZ },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDCHECK_V },
    { DRT(LBLT, CHECKBOX) }
};

static const DIALOG_CONTROL_DATA_CHECKBOX
view_control_border_vert_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_VIEW_BORDER_VERT) };

static const DIALOG_CONTROL
view_control_ruler_horz =
{
    VIEW_CONTROL_ID_RULER_HORZ, VIEW_CONTROL_ID_RULER_BORDER_GROUP,
    { VIEW_CONTROL_ID_BORDER_VERT, VIEW_CONTROL_ID_BORDER_VERT },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDCHECK_V },
    { DRT(LBLT, CHECKBOX) }
};

static const DIALOG_CONTROL_DATA_CHECKBOX
view_control_ruler_horz_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_VIEW_RULER_HORZ) };

static const DIALOG_CONTROL
view_control_ruler_vert =
{
    VIEW_CONTROL_ID_RULER_VERT, VIEW_CONTROL_ID_RULER_BORDER_GROUP,
    { VIEW_CONTROL_ID_RULER_HORZ, VIEW_CONTROL_ID_RULER_HORZ },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDCHECK_V },
    { DRT(LBLT, CHECKBOX) }
};

static const DIALOG_CONTROL_DATA_CHECKBOX
view_control_ruler_vert_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_VIEW_RULER_VERT) };

/*
* ZOOM group
*/

static const DIALOG_CONTROL
view_control_zoom_group =
{
    VIEW_CONTROL_ID_ZOOM_GROUP, DIALOG_MAIN_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { 0, 0, DIALOG_STDGROUP_RM, DIALOG_STDGROUP_BM },
    { DRT(LTRB, GROUPBOX) }
};

static const DIALOG_CONTROL_DATA_GROUPBOX
view_control_zoom_group_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_VIEW_ZOOM), { 0, 0, 0, FRAMED_BOX_GROUP } };

/*
* zoom bump
*/

static const UI_CONTROL_S32
view_control_zoom_bump_control = { 10, 1600, 5 };

static const DIALOG_CONTROL_DATA_BUMP_S32
view_control_zoom_bump_data = { { { { FRAMED_BOX_EDIT } } /*EDIT_XX*/, &view_control_zoom_bump_control } /* BUMP_XX */ };

static const DIALOG_CONTROL
view_control_zoom_bump =
{
    VIEW_CONTROL_ID_ZOOM_BUMP, VIEW_CONTROL_ID_ZOOM_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { DIALOG_STDGROUP_LM, DIALOG_STDGROUP_TM, DIALOG_BUMP_H(4), DIALOG_STDBUMP_V },
    { DRT(LTLT, BUMP_S32), 1 }
};

/*
* zoom fit
*/
static const DIALOG_CONTROL
view_control_zoom_fit_h =
{
    VIEW_CONTROL_ID_ZOOM_FIT_H, VIEW_CONTROL_ID_ZOOM_GROUP,
    { DIALOG_CONTROL_SELF, DIALOG_CONTROL_SELF/*VIEW_CONTROL_ID_ZOOM_BUMP*/, VIEW_CONTROL_ID_ZOOM_FIT_V, VIEW_CONTROL_ID_ZOOM_BUMP },
    { VIEW_CONTROL_ZOOM_BUTTONS_FIT_H, DIALOG_STDPUSHBUTTON_V, 0, 0 },
    { DRT(RBLB, PUSHBUTTON), 1 }
};

static const DIALOG_CONTROL_DATA_PUSHBUTTON
view_control_zoom_fit_h_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_VIEW_SCALE_FIT_H) };

static const DIALOG_CONTROL
view_control_zoom_fit_v =
{
    VIEW_CONTROL_ID_ZOOM_FIT_V, VIEW_CONTROL_ID_ZOOM_GROUP,
    { DIALOG_CONTROL_SELF, VIEW_CONTROL_ID_ZOOM_FIT_H, VIEW_CONTROL_ID_ZOOM_5, VIEW_CONTROL_ID_ZOOM_FIT_H },
    { VIEW_CONTROL_ZOOM_BUTTONS_FIT_H, 0, 0, 0 },
    { DRT(RTRB, PUSHBUTTON), 1 }
};

static const DIALOG_CONTROL_DATA_PUSHBUTTON
view_control_zoom_fit_v_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_VIEW_SCALE_FIT_V) };

static const DIALOG_CONTROL_DATA_PUSHBUTTON
view_control_zoom_n_data[5] =
{
    { { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_VIEW_SCALE_1) },
    { { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_VIEW_SCALE_2) },
    { { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_VIEW_SCALE_3) },
    { { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_VIEW_SCALE_4) },
    { { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_VIEW_SCALE_5) }
};

/*
* zoom buttons
*/

static const DIALOG_CONTROL
view_control_zoom_n[5] =
{
    { VIEW_CONTROL_ID_ZOOM_1, VIEW_CONTROL_ID_ZOOM_GROUP, { VIEW_CONTROL_ID_ZOOM_BUMP, VIEW_CONTROL_ID_ZOOM_BUMP },
  { 0, DIALOG_STDSPACING_V, VIEW_CONTROL_ZOOM_BUTTONS_H, VIEW_CONTROL_ZOOM_BUTTONS_V }, { DRT(LBLT, PUSHBUTTON) } },

    { VIEW_CONTROL_ID_ZOOM_2, VIEW_CONTROL_ID_ZOOM_GROUP, { VIEW_CONTROL_ID_ZOOM_1, VIEW_CONTROL_ID_ZOOM_1, DIALOG_CONTROL_SELF, VIEW_CONTROL_ID_ZOOM_1 },
   { 0, 0, VIEW_CONTROL_ZOOM_BUTTONS_H, 0 }, { DRT(RTLB, PUSHBUTTON) } },

    { VIEW_CONTROL_ID_ZOOM_3, VIEW_CONTROL_ID_ZOOM_GROUP, { VIEW_CONTROL_ID_ZOOM_2, VIEW_CONTROL_ID_ZOOM_2, DIALOG_CONTROL_SELF, VIEW_CONTROL_ID_ZOOM_2 },
  { 0, 0, VIEW_CONTROL_ZOOM_BUTTONS_H, 0 }, { DRT(RTLB, PUSHBUTTON) } },

    { VIEW_CONTROL_ID_ZOOM_4, VIEW_CONTROL_ID_ZOOM_GROUP, { VIEW_CONTROL_ID_ZOOM_3, VIEW_CONTROL_ID_ZOOM_3, DIALOG_CONTROL_SELF, VIEW_CONTROL_ID_ZOOM_3 },
  { 0, 0, VIEW_CONTROL_ZOOM_BUTTONS_H, 0 }, { DRT(RTLB, PUSHBUTTON) } },

    { VIEW_CONTROL_ID_ZOOM_5, VIEW_CONTROL_ID_ZOOM_GROUP, { VIEW_CONTROL_ID_ZOOM_4, VIEW_CONTROL_ID_ZOOM_4, DIALOG_CONTROL_SELF, VIEW_CONTROL_ID_ZOOM_4 },
  { 0, 0, VIEW_CONTROL_ZOOM_BUTTONS_H, 0 }, { DRT(RTLB, PUSHBUTTON) } }
};

/*
* display group
*/

static const DIALOG_CONTROL
view_control_display_group =
{
    VIEW_CONTROL_ID_DISPLAY_GROUP, DIALOG_MAIN_GROUP,
    { VIEW_CONTROL_ID_RULER_BORDER_GROUP, VIEW_CONTROL_ID_RULER_BORDER_GROUP, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { 0, DIALOG_STDSPACING_V, DIALOG_STDGROUP_RM, DIALOG_STDGROUP_BM },
    { DRT(LBRB, GROUPBOX) }
};

static const DIALOG_CONTROL_DATA_GROUPBOX
view_control_display_group_data = { UI_TEXT_INIT_RESID(MSG_DIALOG_VIEW_DISPLAY), { 0, 0, 0, FRAMED_BOX_GROUP } };

static const DIALOG_CONTROL
view_control_display_1 =
{
    VIEW_CONTROL_ID_DISPLAY_1, VIEW_CONTROL_ID_DISPLAY_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { DIALOG_STDGROUP_LM, DIALOG_STDGROUP_TM, DIALOG_CONTENTS_CALC, DIALOG_STDRADIO_V },
    { DRT(LTLT, RADIOBUTTON) }
};

static const DIALOG_CONTROL_DATA_RADIOBUTTON
view_control_display_1_data = { { 0 }, DISPLAY_DESK_AREA, UI_TEXT_INIT_RESID(MSG_DIALOG_VIEW_DISPLAY_1) };

static const DIALOG_CONTROL
view_control_display_2 =
{
    VIEW_CONTROL_ID_DISPLAY_2, VIEW_CONTROL_ID_DISPLAY_GROUP,
    { VIEW_CONTROL_ID_DISPLAY_1, VIEW_CONTROL_ID_DISPLAY_1, DIALOG_CONTROL_SELF, VIEW_CONTROL_ID_DISPLAY_1 },
    { DIALOG_STDSPACING_H, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(RTLB, RADIOBUTTON) }
};

static const DIALOG_CONTROL_DATA_RADIOBUTTON
view_control_display_2_data = { { 0 }, DISPLAY_PRINT_AREA, UI_TEXT_INIT_RESID(MSG_DIALOG_VIEW_DISPLAY_2) };

static const DIALOG_CONTROL
view_control_display_3 =
{
    VIEW_CONTROL_ID_DISPLAY_3, VIEW_CONTROL_ID_DISPLAY_GROUP,
    { VIEW_CONTROL_ID_DISPLAY_2, VIEW_CONTROL_ID_DISPLAY_2, DIALOG_CONTROL_SELF, VIEW_CONTROL_ID_DISPLAY_2 },
    { DIALOG_STDSPACING_H, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(RTLB, RADIOBUTTON) }
};

static const DIALOG_CONTROL_DATA_RADIOBUTTON
view_control_display_3_data = { { 0 }, DISPLAY_WORK_AREA, UI_TEXT_INIT_RESID(MSG_DIALOG_VIEW_DISPLAY_3) };

static const DIALOG_CONTROL
view_control_split_h =
{
    VIEW_CONTROL_ID_SPLIT_H, DIALOG_MAIN_GROUP,
    { VIEW_CONTROL_ID_DISPLAY_GROUP, VIEW_CONTROL_ID_DISPLAY_GROUP },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDCHECK_V },
    { DRT(LBLT, CHECKBOX) }
};

static const DIALOG_CONTROL_DATA_CHECKBOX
view_control_split_h_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_VIEW_SPLIT_H) };

static const DIALOG_CONTROL
view_control_split_v =
{
    VIEW_CONTROL_ID_SPLIT_V, DIALOG_MAIN_GROUP,
    { VIEW_CONTROL_ID_SPLIT_H, VIEW_CONTROL_ID_SPLIT_H, DIALOG_CONTROL_SELF, VIEW_CONTROL_ID_SPLIT_H },
    { DIALOG_STDSPACING_H, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(RTLB, CHECKBOX) }
};

static const DIALOG_CONTROL_DATA_CHECKBOX
view_control_split_v_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_VIEW_SPLIT_V) };

static const DIALOG_CONTROL_ID
view_control_ok_data_argmap[] =
{
#define ARG_VIEW_CONTROL_ZOOM           0
    VIEW_CONTROL_ID_ZOOM_BUMP,
#define ARG_VIEW_CONTROL_DISPLAY        1
    VIEW_CONTROL_ID_DISPLAY_GROUP,
#define ARG_VIEW_CONTROL_RULER_HORZ     2
    VIEW_CONTROL_ID_RULER_HORZ,
#define ARG_VIEW_CONTROL_RULER_VERT     3
    VIEW_CONTROL_ID_RULER_VERT,
#define ARG_VIEW_CONTROL_BORDER_HORZ    4
    VIEW_CONTROL_ID_BORDER_HORZ,
#define ARG_VIEW_CONTROL_BORDER_VERT    5
    VIEW_CONTROL_ID_BORDER_VERT,
#define ARG_VIEW_CONTROL_SPLIT_H        6
    VIEW_CONTROL_ID_SPLIT_H,
#define ARG_VIEW_CONTROL_SPLIT_H_POS    7
    0,
#define ARG_VIEW_CONTROL_SPLIT_V        8
    VIEW_CONTROL_ID_SPLIT_V,
#define ARG_VIEW_CONTROL_SPLIT_V_POS    9
    0
#define ARG_VIEW_CONTROL_N_ARGS         10
};

static const DIALOG_CONTROL_DATA_PUSH_COMMAND
view_control_ok_command = { T5_CMD_VIEW_CONTROL, OBJECT_ID_SKEL, NULL, view_control_ok_data_argmap, { 0 /*set_view*/, 0, 0, 1 /*lookup_arglist*/ } };

static const DIALOG_CONTROL_DATA_PUSHBUTTON
view_control_ok_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_OK), &view_control_ok_command };

static const DIALOG_CONTROL
view_control_new_view =
{
    VIEW_CONTROL_ID_NEW_VIEW, DIALOG_CONTROL_WINDOW,
    { DIALOG_CONTROL_SELF, IDCANCEL, IDCANCEL, IDCANCEL },
#if WINDOWS
    { DIALOG_STDPUSHBUTTON_H, 0, DIALOG_STDSPACING_H, 0 },
#else
    { DIALOG_CONTENTS_CALC, 0, DIALOG_STDSPACING_H, 0 },
#endif
    { DRT(RTLB, PUSHBUTTON), 1 }
};

static const DIALOG_CONTROL_DATA_PUSHBUTTON
view_control_new_view_data = { { 0 }, UI_TEXT_INIT_RESID(MSG_DIALOG_VIEW_NEW_VIEW), &view_control_ok_command };

static const DIALOG_CTL_CREATE
view_control_ctl_create[] =
{
    { &dialog_main_group },

    { &view_control_zoom_group,     &view_control_zoom_group_data },
    { &view_control_zoom_bump,      &view_control_zoom_bump_data },
    { &view_control_zoom_fit_h,     &view_control_zoom_fit_h_data },
    { &view_control_zoom_fit_v,      &view_control_zoom_fit_v_data },
    { &view_control_zoom_n[1-1],    &view_control_zoom_n_data[1-1] },
    { &view_control_zoom_n[2-1],    &view_control_zoom_n_data[2-1] },
    { &view_control_zoom_n[3-1],    &view_control_zoom_n_data[3-1] },
    { &view_control_zoom_n[4-1],    &view_control_zoom_n_data[4-1] },
    { &view_control_zoom_n[5-1],    &view_control_zoom_n_data[5-1] },

    { &view_control_ruler_border_group },
    { &view_control_border_horz,    &view_control_border_horz_data },
    { &view_control_border_vert,    &view_control_border_vert_data },
    { &view_control_ruler_horz,     &view_control_ruler_horz_data },
    { &view_control_ruler_vert,     &view_control_ruler_vert_data },

    { &view_control_display_group,  &view_control_display_group_data },
    { &view_control_display_1,      &view_control_display_1_data },
    { &view_control_display_2,      &view_control_display_2_data },
    { &view_control_display_3,      &view_control_display_3_data },

    { &view_control_split_h,        &view_control_split_h_data },
    { &view_control_split_v,        &view_control_split_v_data },

    { &defbutton_ok,                &view_control_ok_data },
    { &stdbutton_cancel,            &stdbutton_cancel_data },
    { &view_control_new_view,       &view_control_new_view_data }
};

typedef struct VIEW_CONTROL_CALLBACK
{
    S32 zoom_bump_state;
    S32 display_mode;
    BOOL horz_ruler_on;
    BOOL vert_ruler_on;
    BOOL horz_border_on;
    BOOL vert_border_on;
    BOOL split_h;
    BOOL split_v;
}
VIEW_CONTROL_CALLBACK, * P_VIEW_CONTROL_CALLBACK;

_Check_return_
static STATUS
dialog_view_control_process_start(
    _InRef_     PC_DIALOG_MSG_PROCESS_START p_dialog_msg_process_start)
{
    const P_VIEW_CONTROL_CALLBACK p_view_control_callback = (P_VIEW_CONTROL_CALLBACK) p_dialog_msg_process_start->client_handle;
    const H_DIALOG h_dialog = p_dialog_msg_process_start->h_dialog;

    status_return(ui_dlg_set_s32(h_dialog, VIEW_CONTROL_ID_ZOOM_BUMP, p_view_control_callback->zoom_bump_state));
    status_return(ui_dlg_set_radio(h_dialog, VIEW_CONTROL_ID_DISPLAY_GROUP, p_view_control_callback->display_mode));
    status_return(ui_dlg_set_check(h_dialog, VIEW_CONTROL_ID_BORDER_HORZ, p_view_control_callback->horz_border_on));
    status_return(ui_dlg_set_check(h_dialog, VIEW_CONTROL_ID_BORDER_VERT, p_view_control_callback->vert_border_on));
    status_return(ui_dlg_set_check(h_dialog, VIEW_CONTROL_ID_RULER_HORZ, p_view_control_callback->horz_ruler_on));
    status_return(ui_dlg_set_check(h_dialog, VIEW_CONTROL_ID_RULER_VERT, p_view_control_callback->vert_ruler_on));
    status_return(ui_dlg_set_check(h_dialog, VIEW_CONTROL_ID_SPLIT_H, p_view_control_callback->split_h));
    status_return(ui_dlg_set_check(h_dialog, VIEW_CONTROL_ID_SPLIT_V, p_view_control_callback->split_v));

    return(STATUS_OK);
}

/* calculates a scale factor such that a view with one pane at full size
 * covering the screen would have a page's horz. printable area visible
*/

_Check_return_
static S32 /* new zoom factor */
process_zoom_fit(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     H_DIALOG h_dialog,
    _InVal_     DIALOG_CONTROL_ID dialog_control_id)
{
    const P_VIEW p_view = p_view_from_viewno_caret(p_docu);
    DISPLAY_MODE display_mode = (DISPLAY_MODE) ui_dlg_get_radio(h_dialog, VIEW_CONTROL_ID_DISPLAY_GROUP);
    BOOL horz_ruler_on  = ui_dlg_get_check(h_dialog, VIEW_CONTROL_ID_RULER_HORZ);
    BOOL horz_border_on = ui_dlg_get_check(h_dialog, VIEW_CONTROL_ID_BORDER_HORZ);
    BOOL vert_ruler_on  = ui_dlg_get_check(h_dialog, VIEW_CONTROL_ID_RULER_VERT);
    BOOL vert_border_on = ui_dlg_get_check(h_dialog, VIEW_CONTROL_ID_BORDER_VERT);
    PIXIT_POINT page_size;
    GDI_SIZE screen_gdi_size;
    PIXIT_POINT screen_pixit_size;
    S32 new_zoom_factor;

    view_one_page_display_size_query(p_docu, display_mode, &page_size);

    host_work_area_gdi_size_query(&screen_gdi_size); /* NB pixels */

    /* always a vertical scroll bar at the right */
#if RISCOS
    screen_gdi_size.cx -= wimp_win_vscroll_width(4);
#elif WINDOWS
    screen_gdi_size.cx -= GetSystemMetrics(SM_CXVSCROLL);

    /* and accomodate left and right borders */
    screen_gdi_size.cx -= 2 * GetSystemMetrics(SM_CXBORDER);
#endif

    /* always a title bar at the top */
#if RISCOS
    screen_gdi_size.cy -= wimp_win_title_height(4);
#elif WINDOWS
    screen_gdi_size.cy -= GetSystemMetrics(SM_CYCAPTION);

    /* and on Windows, a menu bar */
    screen_gdi_size.cy -= GetSystemMetrics(SM_CYMENU);
#endif

    { /* then the toobar gets in the way too */
#if RISCOS
    screen_gdi_size.cy += p_view->margin.y1; /* NB margin is -ve */
#elif WINDOWS
    screen_gdi_size.cy -= (p_view->inner_frame_gdi_rect.tl.y - p_view->outer_frame_gdi_rect.tl.y);

    /* and on Windows, there may be a lower status bar */
    screen_gdi_size.cy -= (p_view->outer_frame_gdi_rect.br.y - p_view->inner_frame_gdi_rect.br.y);
#endif
    } /*block*/

    /* always a horizontal scroll bar at the bottom */
#if RISCOS
    screen_gdi_size.cy -= wimp_win_hscroll_height(4);
#elif WINDOWS
    screen_gdi_size.cy -= GetSystemMetrics(SM_CYHSCROLL);

    /* and accomodate top and bottom borders */
    screen_gdi_size.cy -= 2 * GetSystemMetrics(SM_CYBORDER);
#endif

#if RISCOS
    /* always the icon bar to get in the way too */
    screen_gdi_size.cy -= wimp_iconbar_height;
#endif


    { /* now go into pixits: this we do using a rectangle transform to ensure that the co-ordinates are correctly rounded. */
    GDI_POINT screen_gdi_br;
    HOST_XFORM host_xform;
    set_host_xform_for_view(&host_xform, p_view, FALSE, FALSE);
    screen_gdi_br.x = screen_gdi_size.cx;
#if RISCOS
    screen_gdi_br.y = - screen_gdi_size.cy; /* flip as size != point */
#else
    screen_gdi_br.y = screen_gdi_size.cy;
#endif
    pixit_point_from_window_point(&screen_pixit_size, &screen_gdi_br, &host_xform);
    } /*block*/

    if(horz_ruler_on || vert_ruler_on)
    {
        if(status_ok(object_load(OBJECT_ID_RULER)))
        {
            if(vert_ruler_on)
                screen_pixit_size.x -= view_ruler_pixit_size(p_view, FALSE);

            if(horz_ruler_on)
                screen_pixit_size.y -= view_ruler_pixit_size(p_view, TRUE);
        }
    }

    if(vert_border_on)
        screen_pixit_size.x -= view_border_pixit_size(p_view, FALSE);

    if(horz_border_on)
        screen_pixit_size.y -= view_border_pixit_size(p_view, TRUE);

    if(VIEW_CONTROL_ID_ZOOM_FIT_H == dialog_control_id)
    {
        new_zoom_factor = 5 * muldiv64_round_floor(screen_pixit_size.x, 100, 5 * page_size.x); /* round down to nearest 5% */
    }
    else /* (VIEW_CONTROL_ID_ZOOM_FIT_V == dialog_control_id) */
    {
        /*reportf(TEXT("sps.y = %d; ps.y = %d"), screen_pixit_size.y, page_size.y);*/
        new_zoom_factor = 5 * muldiv64_round_floor(screen_pixit_size.y, 100, 5 * page_size.y); /* round down to nearest 5% */
    }

    return(new_zoom_factor);
}

_Check_return_
static STATUS
dialog_view_control_ctl_pushbutton(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_DIALOG_MSG_CTL_PUSHBUTTON p_dialog_msg_ctl_pushbutton)
{
    const DIALOG_CONTROL_ID dialog_control_id = p_dialog_msg_ctl_pushbutton->dialog_control_id;

    switch(dialog_control_id)
    {
    case VIEW_CONTROL_ID_NEW_VIEW:
        /* first of all, fire off a New View command, then apply commands to it just like OK does */
        return(execute_command(p_docu, T5_CMD_VIEW_NEW, _P_DATA_NONE(P_ARGLIST_HANDLE), OBJECT_ID_SKEL));

    case VIEW_CONTROL_ID_ZOOM_FIT_H:
    case VIEW_CONTROL_ID_ZOOM_FIT_V:
    case VIEW_CONTROL_ID_ZOOM_1:
    case VIEW_CONTROL_ID_ZOOM_2:
    case VIEW_CONTROL_ID_ZOOM_3:
    case VIEW_CONTROL_ID_ZOOM_4:
    case VIEW_CONTROL_ID_ZOOM_5:
        {
        S32 new_zoom_factor;

        if((dialog_control_id == VIEW_CONTROL_ID_ZOOM_FIT_H) || (dialog_control_id == VIEW_CONTROL_ID_ZOOM_FIT_V))
        {
            new_zoom_factor = process_zoom_fit(p_docu, p_dialog_msg_ctl_pushbutton->h_dialog, dialog_control_id);
        }
        else
        {
            /* read new zoom factor from messages file; up to us, and then punter, to keep icons & messages file in step */
            PC_USTR ustr = resource_lookup_ustr(((S32) dialog_control_id - VIEW_CONTROL_ID_ZOOM_1) + MSG_DIALOG_VIEW_SCALE_1);
            new_zoom_factor = (S32) fast_ustrtoul(ustr, NULL);
            assert(new_zoom_factor > 0);
        }

        /* reflect into dialog */
        return(ui_dlg_set_s32(p_dialog_msg_ctl_pushbutton->h_dialog, VIEW_CONTROL_ID_ZOOM_BUMP, new_zoom_factor));
        }

    default:
        return(STATUS_OK);
    }
}

PROC_DIALOG_EVENT_PROTO(static, dialog_event_view_control)
{
    switch(dialog_message)
    {
    case DIALOG_MSG_CODE_PROCESS_START:
        return(dialog_view_control_process_start((PC_DIALOG_MSG_PROCESS_START) p_data));

    case DIALOG_MSG_CODE_CTL_PUSHBUTTON:
        return(dialog_view_control_ctl_pushbutton(p_docu, (P_DIALOG_MSG_CTL_PUSHBUTTON) p_data));

    default:
        return(STATUS_OK);
    }
}

T5_CMD_PROTO(extern, t5_cmd_view_control_intro)
{
    const P_VIEW p_view = p_view_from_viewno_caret(p_docu);
    VIEW_CONTROL_CALLBACK view_control_callback;

    IGNOREPARM_InVal_(t5_message);
    IGNOREPARM_InRef_(p_t5_cmd);

    /* encode initial state of control(s) */
    view_control_callback.zoom_bump_state = muldiv64_ceil(100, p_view->scalet, p_view->scaleb);
    view_control_callback.display_mode = p_view->display_mode;
    view_control_callback.horz_ruler_on = p_view->flags.horz_ruler_on;
    view_control_callback.vert_ruler_on = p_view->flags.vert_ruler_on;
    view_control_callback.horz_border_on = p_view->flags.horz_border_on;
    view_control_callback.vert_border_on = p_view->flags.vert_border_on;
    view_control_callback.split_h = p_view->flags.vert_split_on;
    view_control_callback.split_v = p_view->flags.horz_split_on;

    { /* put up a dialog box and get the punter to choose something */
    DIALOG_CMD_PROCESS_DBOX dialog_cmd_process_dbox;
    dialog_cmd_process_dbox_setup(&dialog_cmd_process_dbox, view_control_ctl_create, elemof32(view_control_ctl_create), MSG_DIALOG_VIEW_CONTROL_HELP_TOPIC);
    /*dialog_cmd_process_dbox.caption.type = UI_TEXT_TYPE_RESID;*/
    dialog_cmd_process_dbox.caption.text.resource_id = MSG_DIALOG_VIEW_CONTROL;
    dialog_cmd_process_dbox.p_proc_client = dialog_event_view_control;
    dialog_cmd_process_dbox.client_handle = (CLIENT_HANDLE) &view_control_callback;
    return(object_call_DIALOG_with_docu(p_docu, DIALOG_CMD_CODE_PROCESS_DBOX, &dialog_cmd_process_dbox));
    } /*block*/
}

T5_CMD_PROTO(extern, t5_cmd_view_control)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, ARG_VIEW_CONTROL_N_ARGS);
    const P_VIEW p_view = p_view_from_viewno_caret(p_docu);
    BOOL reopen = 0;
    BOOL set_extent = 0;
    BOOL redisplay = 0;

    IGNOREPARM_InVal_(t5_message);
    IGNOREPARM_InRef_(p_t5_cmd);

#if 0 /* SKS 20may97 try this out on people... */
    if(1/*invent a first time thru on a view*/)
    {
        p_view->pane[WIN_PANE].lastopen.scx            =
        p_view->pane[WIN_PANE_SPLIT_VERT].lastopen.scx =
        p_view->pane[WIN_PANE_SPLIT_DIAG].lastopen.scx =
        p_view->pane[WIN_PANE_SPLIT_HORZ].lastopen.scx = (int) (DESK_LM + p_docu->page_def.margin_left);
    }
#endif

    if(ARG_TYPE_NONE != p_args[ARG_VIEW_CONTROL_ZOOM].type)
    {
        /* zoom to this value */
        S32 zoom = p_args[ARG_VIEW_CONTROL_ZOOM].val.s32;
        S32 zoomt = MIN(3200, MAX(5, zoom));
        S32 zoomb = 100;

        view_set_zoomfactor(p_docu, p_view, zoomt, zoomb);

        reopen = set_extent = redisplay = 1;
    }

    if(ARG_TYPE_NONE != p_args[ARG_VIEW_CONTROL_DISPLAY].type)
    {
        DISPLAY_MODE display_mode = (DISPLAY_MODE) p_args[ARG_VIEW_CONTROL_DISPLAY].val.s32;
        DISPLAY_MODE limit = DISPLAY_WORK_AREA;
        if( display_mode > limit)
            display_mode = limit;
        p_view->display_mode = display_mode;
        redisplay = reopen = set_extent = 1;
    }

    if(ARG_TYPE_NONE != p_args[ARG_VIEW_CONTROL_RULER_HORZ].type)
    {
        p_view->flags.horz_ruler_on = p_args[ARG_VIEW_CONTROL_RULER_HORZ].val.fBool;
        reopen = set_extent = 1;
    }

    if(ARG_TYPE_NONE != p_args[ARG_VIEW_CONTROL_RULER_VERT].type)
    {
        p_view->flags.vert_ruler_on = p_args[ARG_VIEW_CONTROL_RULER_VERT].val.fBool;
        reopen = set_extent = 1;
    }

    if(ARG_TYPE_NONE != p_args[ARG_VIEW_CONTROL_BORDER_HORZ].type)
    {
        p_view->flags.horz_border_on = p_args[ARG_VIEW_CONTROL_BORDER_HORZ].val.fBool;
        reopen = set_extent = 1;
    }

    if(ARG_TYPE_NONE != p_args[ARG_VIEW_CONTROL_BORDER_VERT].type)
    {
        p_view->flags.vert_border_on = p_args[ARG_VIEW_CONTROL_BORDER_VERT].val.fBool;
        reopen = set_extent = 1;
    }

    if(ARG_TYPE_NONE != p_args[ARG_VIEW_CONTROL_SPLIT_H].type)
    {
        p_view->flags.vert_split_on = p_args[ARG_VIEW_CONTROL_SPLIT_H].val.fBool;
        reopen = 1;
    }

    if(ARG_TYPE_NONE != p_args[ARG_VIEW_CONTROL_SPLIT_H_POS].type)
    {
        p_view->vert_split_pos = p_args[ARG_VIEW_CONTROL_SPLIT_H_POS].val.s32;
        reopen = 1;
    }

    if(ARG_TYPE_NONE != p_args[ARG_VIEW_CONTROL_SPLIT_V].type)
    {
        p_view->flags.horz_split_on = p_args[ARG_VIEW_CONTROL_SPLIT_V].val.fBool;
        reopen = 1;
    }

    if(ARG_TYPE_NONE != p_args[ARG_VIEW_CONTROL_SPLIT_V_POS].type)
    {
        p_view->horz_split_pos = p_args[ARG_VIEW_CONTROL_SPLIT_V_POS].val.s32;
        reopen = 1;
    }

    if(p_view->flags.horz_ruler_on || p_view->flags.vert_ruler_on)
        if(status_fail(object_load(OBJECT_ID_RULER)))
        {
            p_view->flags.horz_ruler_on = p_view->flags.vert_ruler_on = 0;
            reopen = set_extent = 1;
        }

    if(set_extent)
    {
        calc_paneextent(p_docu, p_view);

        host_set_extent_this_view(p_docu, p_view);
    }

    if(reopen)
    {
        if(p_view->horz_split_pos <= 0)
            p_view->horz_split_pos = PIXITS_PER_INCH;

        if(p_view->vert_split_pos <= 0)
            p_view->vert_split_pos = PIXITS_PER_INCH;

        host_view_reopen(p_docu, p_view);
    }

    if(redisplay)
    {
        view__update__later(p_docu, p_view);
        caret_show_claim(p_docu, p_docu->focus_owner, TRUE);
    }

    return(STATUS_OK);
}

/******************************************************************************
*
* Output a series of commands that on execution would recreate the given view
*
******************************************************************************/

_Check_return_
static STATUS
save_this_view(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format)
{
    STATUS status;

    if(view_void_entry(p_view))
        return(STATUS_OK);

    status_return(ownform_output_group_stt(p_of_op_format));

    {
    const OBJECT_ID object_id = OBJECT_ID_SKEL;
    PC_CONSTRUCT_TABLE p_construct_table;
    ARGLIST_HANDLE arglist_handle;
    PIXIT_POINT tl;
    PIXIT_SIZE size;

    host_view_query_posn(p_docu, p_view, &tl, &size);

    if(status_ok(status = arglist_prepare_with_construct(&arglist_handle, object_id, T5_CMD_VIEW_CREATE, &p_construct_table)))
    {
        const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 4);
        p_args[0].val.s32 = tl.x;
        p_args[1].val.s32 = tl.y;
        p_args[2].val.s32 = size.cx;
        p_args[3].val.s32 = size.cy;
        status = ownform_save_arglist(arglist_handle, object_id, p_construct_table, p_of_op_format);
        arglist_dispose(&arglist_handle);
    }

    status_return(status);
    } /*block*/

    {
    const OBJECT_ID object_id = OBJECT_ID_SKEL;
    PC_CONSTRUCT_TABLE p_construct_table;
    ARGLIST_HANDLE arglist_handle;

    if(status_ok(status = arglist_prepare_with_construct(&arglist_handle, object_id, T5_CMD_VIEW_CONTROL, &p_construct_table)))
    {
        const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, ARG_VIEW_CONTROL_N_ARGS);
        p_args[ARG_VIEW_CONTROL_ZOOM].val.s32           = muldiv64_ceil(100, p_view->scalet, p_view->scaleb);
        p_args[ARG_VIEW_CONTROL_DISPLAY].val.s32        = p_view->display_mode;
        p_args[ARG_VIEW_CONTROL_RULER_HORZ].val.fBool   = p_view->flags.horz_ruler_on;
        p_args[ARG_VIEW_CONTROL_RULER_VERT].val.fBool   = p_view->flags.vert_ruler_on;
        p_args[ARG_VIEW_CONTROL_BORDER_HORZ].val.fBool  = p_view->flags.horz_border_on;
        p_args[ARG_VIEW_CONTROL_BORDER_VERT].val.fBool  = p_view->flags.vert_border_on;
        p_args[ARG_VIEW_CONTROL_SPLIT_H].val.fBool      = p_view->flags.vert_split_on;
        p_args[ARG_VIEW_CONTROL_SPLIT_H_POS].val.s32    = p_view->vert_split_pos;
        p_args[ARG_VIEW_CONTROL_SPLIT_V].val.fBool      = p_view->flags.horz_split_on;
        p_args[ARG_VIEW_CONTROL_SPLIT_V_POS].val.s32    = p_view->horz_split_pos;
        status = ownform_save_arglist(arglist_handle, object_id, p_construct_table, p_of_op_format);
        arglist_dispose(&arglist_handle);
    }

    status_return(status);
    } /*block*/

    if(p_view_from_viewno_caret(p_docu) == p_view)
    {
#if RISCOS
        /* save preferred pane info unless default */
        if(p_view->cur_pane != WIN_PANE)
        {
            const OBJECT_ID object_id = OBJECT_ID_SKEL;
            PC_CONSTRUCT_TABLE p_construct_table;
            ARGLIST_HANDLE arglist_handle;

            if(status_ok(status = arglist_prepare_with_construct(&arglist_handle, object_id, T5_CMD_CURRENT_PANE, &p_construct_table)))
            {
                const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 1);
                p_args[0].val.s32 = (S32) p_view->cur_pane;
                status = ownform_save_arglist(arglist_handle, object_id, p_construct_table, p_of_op_format);
                arglist_dispose(&arglist_handle);
            }

            status_return(status);
        }
#endif

        /* save current position */
#if 0
        if(1 /*OBJECT_ID_CELLS == p_docu->focus_owner*/)
#endif
        {
            const OBJECT_ID object_id = OBJECT_ID_SKEL;
            PC_CONSTRUCT_TABLE p_construct_table;
            ARGLIST_HANDLE arglist_handle;

            if(status_ok(status = arglist_prepare_with_construct(&arglist_handle, object_id, T5_CMD_CURRENT_POSITION, &p_construct_table)))
            {
                const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 4);

                p_args[0].val.col = p_docu->cur.slr.col;
                p_args[1].val.row = p_docu->cur.slr.row;

                if(OBJECT_ID_NONE != p_docu->cur.object_position.object_id)
                {
                    p_args[2].val.s32 = p_docu->cur.object_position.data;
                    p_args[3].val.s32 = p_docu->cur.object_position.object_id;
                }
                else
                {
                    arg_dispose(&arglist_handle, 2);
                    arg_dispose(&arglist_handle, 3);
                }

                status = ownform_save_arglist(arglist_handle, object_id, p_construct_table, p_of_op_format);

                arglist_dispose(&arglist_handle);
            }

            status_return(status);
        }
    }

    return(ownform_output_group_end(p_of_op_format));
}

T5_CMD_PROTO(extern, t5_cmd_view_create)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 4);
    VIEW_CREATE view_create;
    P_VIEW p_view;

    IGNOREPARM_InVal_(t5_message);

    view_create.docno   = docno_from_p_docu(p_docu);
    view_create.tl.x    = p_args[0].val.s32;
    view_create.tl.y    = p_args[1].val.s32;
    view_create.size.cx = p_args[2].val.s32;
    view_create.size.cy = p_args[3].val.s32;

    status_return(view_new_create(&view_create));

    p_view = view_create.p_view;

    p_docu->viewno_caret = viewno_from_p_view(p_view);

    host_set_extent_this_view(p_docu, p_view);

    host_view_reopen(p_docu, p_view);

    view__update__later(p_docu, p_view);

    return(STATUS_OK);
}

/******************************************************************************
*
* Actioned by the menu entry 'New view'.
*
* Create and show a new view based on the current one.
*
******************************************************************************/

T5_CMD_PROTO(extern, t5_cmd_view_new)
{
    P_VIEW p_sourceview = p_view_from_viewno_caret(p_docu);
    VIEW_CREATE view_create;
    P_VIEW p_view;

    IGNOREPARM_InVal_(t5_message);
    IGNOREPARM_InRef_(p_t5_cmd);

    view_create.docno = docno_from_p_docu(p_docu);

    if(!IS_VIEW_NONE(p_sourceview))
    {
        host_view_query_posn(p_docu, p_sourceview, &view_create.tl, &view_create.size);

#if RISCOS
        view_create.tl.y -= 48 * PIXITS_PER_RISCOS;
#endif
    }
    else
    {
        view_create.tl.x = 0;
        view_create.tl.y = 0;
    }

    status_return(view_new_create(&view_create));

    p_view = view_create.p_view;

    if(!IS_VIEW_NONE(p_sourceview))
        p_sourceview = p_view_from_viewno_caret(p_docu); /* cos view table may move */

    p_docu->viewno_caret = viewno_from_p_view(p_view);

    /* zoom factor */
    if(!IS_VIEW_NONE(p_sourceview))
    {
        __analysis_assume(p_sourceview);
        view_set_zoomfactor(p_docu, p_view, p_sourceview->scalet, p_sourceview->scaleb);

        p_view->flags.horz_split_on = p_sourceview->flags.horz_split_on;
        p_view->horz_split_pos      = p_sourceview->horz_split_pos;

        p_view->flags.vert_split_on = p_sourceview->flags.vert_split_on;
        p_view->vert_split_pos      = p_sourceview->vert_split_pos;

        p_view->display_mode = p_sourceview->display_mode;

        p_view->flags.horz_border_on = p_sourceview->flags.horz_border_on;
        p_view->flags.vert_border_on = p_sourceview->flags.vert_border_on;
        p_view->flags.horz_ruler_on  = p_sourceview->flags.horz_ruler_on;
        p_view->flags.vert_ruler_on  = p_sourceview->flags.vert_ruler_on;
    }
    else
    {
        view_set_zoomfactor(p_docu, p_view, 1, 1);

        p_view->flags.horz_split_on = 0;
        p_view->horz_split_pos = 1440;

        p_view->flags.vert_split_on = 0;
        p_view->vert_split_pos = 1440;

        p_view->display_mode = DISPLAY_DESK_AREA;

        p_view->flags.horz_border_on = 1;
        p_view->flags.vert_border_on = 1;
        p_view->flags.horz_ruler_on  = 0;
        p_view->flags.vert_ruler_on  = 0;
    }

    host_set_extent_this_view(p_docu, p_view);

    host_view_reopen(p_docu, p_view);

    view__update__later(p_docu, p_view);

    view_scroll_caret(p_docu, UPDATE_PANE_CELLS_AREA, &p_docu->caret, 0, 0, 0, p_docu->caret_height);

    return(STATUS_OK);
}

T5_CMD_PROTO(extern, t5_cmd_view_size)
{
    IGNOREPARM_DocuRef_(p_docu);
    IGNOREPARM_InVal_(t5_message);
    IGNOREPARM_InRef_(p_t5_cmd);
    return(STATUS_OK);
}

T5_CMD_PROTO(extern, t5_cmd_view_posn)
{
    IGNOREPARM_DocuRef_(p_docu);
    IGNOREPARM_InVal_(t5_message);
    IGNOREPARM_InRef_(p_t5_cmd);
    return(STATUS_OK);
}

T5_CMD_PROTO(extern, t5_cmd_view_maximize)
{
    IGNOREPARM_InVal_(t5_message);
    IGNOREPARM_InRef_(p_t5_cmd);

    host_view_maximize(p_docu, p_view_from_viewno_caret(p_docu));

    return(STATUS_OK);
}

T5_CMD_PROTO(extern, t5_cmd_view_minimize)
{
    IGNOREPARM_InVal_(t5_message);
    IGNOREPARM_InRef_(p_t5_cmd);

    host_view_minimize(p_docu, p_view_from_viewno_caret(p_docu));

    return(STATUS_OK);
}

T5_CMD_PROTO(extern, t5_cmd_view_scroll)
{
    IGNOREPARM_DocuRef_(p_docu);
    IGNOREPARM_InVal_(t5_message);
    IGNOREPARM_InRef_(p_t5_cmd);
    return(STATUS_OK);
}

T5_CMD_PROTO(extern, t5_cmd_view_close_req)
{
    IGNOREPARM_InVal_(t5_message);
    IGNOREPARM_InRef_(p_t5_cmd);

    status_return(maeve_event(p_docu, T5_MSG_CELL_MERGE, P_DATA_NONE));

    {
    const P_VIEW p_view = p_view_from_viewno_caret(p_docu);
    if(!IS_VIEW_NONE(p_view))
    {
#if RISCOS
        winx_send_close_window_request(p_view->main[WIN_BACK].hwnd, TRUE /*immediate*/);
#elif WINDOWS
        SendMessage(p_view->main[WIN_BACK].hwnd, WM_CLOSE, 0, 0L);
#endif
    }
    } /*block*/

    return(STATUS_OK);
}

static void
view_set_zoomfactor(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InVal_     S32 scalet,
    _InVal_     S32 scaleb)
{
#if TRUE && RISCOS
    { /* mash the pane scroll offsets so the document remains scrolled to approx. the same place */
    S32 x1 = (scalet * p_view->scaleb * p_view->pane[WIN_PANE           ].lastopen.xscroll) / (p_view->scalet * scaleb);
    S32 x2 = (scalet * p_view->scaleb * p_view->pane[WIN_PANE_SPLIT_HORZ].lastopen.xscroll) / (p_view->scalet * scaleb);
    S32 y1 = (scalet * p_view->scaleb * p_view->pane[WIN_PANE           ].lastopen.yscroll) / (p_view->scalet * scaleb);
    S32 y2 = (scalet * p_view->scaleb * p_view->pane[WIN_PANE_SPLIT_VERT].lastopen.yscroll) / (p_view->scalet * scaleb);

    p_view->pane[WIN_PANE           ].lastopen.xscroll = (int) x1;
    p_view->pane[WIN_PANE_SPLIT_VERT].lastopen.xscroll = (int) x1;

    p_view->pane[WIN_PANE_SPLIT_DIAG].lastopen.xscroll = (int) x2;
    p_view->pane[WIN_PANE_SPLIT_HORZ].lastopen.xscroll = (int) x2;

    p_view->pane[WIN_PANE           ].lastopen.yscroll = (int) y1;
    p_view->pane[WIN_PANE_SPLIT_HORZ].lastopen.yscroll = (int) y1;

    p_view->pane[WIN_PANE_SPLIT_DIAG].lastopen.yscroll = (int) y2;
    p_view->pane[WIN_PANE_SPLIT_VERT].lastopen.yscroll = (int) y2;
    } /*block*/
#endif

    p_view->scalet = scalet;
    p_view->scaleb = scaleb;

    eliminate_common_factors(&p_view->scalet, &p_view->scaleb);

    view_cache_info(p_view);

    status_assert(maeve_event(p_docu, T5_MSG_VIEW_ZOOMFACTOR, p_view));
}

extern void
view_set_zoom_from_wheel_delta(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InVal_     S32 delta)
{
    S32 current_zoom = muldiv64_ceil(100, p_view->scalet, p_view->scaleb);
    S32 divisor = (current_zoom >= 500) ? 1 : (current_zoom >= 250) ? 2 : (current_zoom >= 120) ? 5 : 10;
    S32 zoom = current_zoom + (delta / divisor);
    S32 zoomt = MIN(3200, MAX(5, zoom));
    S32 zoomb = 100;

    view_set_zoomfactor(p_docu, p_view, zoomt, zoomb);

    /*(set_extent)*/
    {
        calc_paneextent(p_docu, p_view);

        host_set_extent_this_view(p_docu, p_view);
    }

    /*(redisplay)*/
    {
        view__update__later(p_docu, p_view);
        caret_show_claim(p_docu, p_docu->focus_owner, TRUE);
    }
}

/******************************************************************************
*
* Set the window extents of all the windows in all the views of a document.
*
* Typically called when some property of the document length changes,
* e.g. the number of pages or the size of a page.
*
******************************************************************************/

extern void
docu_set_and_show_extent_all_views(
    _DocuRef_   P_DOCU p_docu)
{
    DOCU_ASSERT(p_docu);

    for(;;) /* NOT a loop for structure - see last comment */
    {
        VIEWNO viewno = VIEWNO_NONE;
        P_VIEW p_view;

        if(!p_docu->flags.new_extent)
            break;

        docu_flags_new_extent_clear(p_docu);

        while(P_VIEW_NONE != (p_view = docu_enum_views(p_docu, &viewno)))
        {
            calc_paneextent(p_docu, p_view);
            host_set_extent_this_view(p_docu, p_view);
            host_view_reopen(p_docu, p_view); /* NB. this may docu_flags_new_extent_set() (reason for outer loop) */
        }

        assert(0 == p_docu->flags.new_extent); /* to let me test that */
    }
}

/******************************************************************************
*
* give size of a 'page' as displayed
*
******************************************************************************/

extern void
view_one_page_display_size_query(
    _DocuRef_   P_DOCU p_docu,
    _In_        DISPLAY_MODE display_mode,
    /*out*/ P_PIXIT_POINT p_size)
{
    PIXIT_POINT grid_step, desk_margin;

    view_grid_step(p_docu, &grid_step, &desk_margin, display_mode, TRUE, TRUE);

    /* note extra desk_lm and desk_tm to give right-most and bottom-most pages a desk strip */
    p_size->x = grid_step.x + desk_margin.x;
    p_size->y = grid_step.y + desk_margin.y;
}

static void
calc_paneextent(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view)
{
    PIXIT_POINT grid_step, desk_margin;

    view_grid_step(p_docu, &grid_step, &desk_margin, p_view->display_mode, TRUE, TRUE);

    /* note extra desk_lm and desk_tm to give right-most and bottom-most pages a desk strip */
    p_view->paneextent.x = last_page_x(p_docu) * grid_step.x + desk_margin.x;
    p_view->paneextent.y = last_page_y(p_docu) * grid_step.y + desk_margin.y;
}

extern void
view_cache_info(
    _ViewRef_   P_VIEW p_view)
{
    set_host_xform_for_view(&p_view->host_xform[XFORM_BACK], p_view, FALSE, FALSE);
    set_host_xform_for_view(&p_view->host_xform[XFORM_HORZ], p_view, TRUE,  FALSE);
    set_host_xform_for_view(&p_view->host_xform[XFORM_VERT], p_view, FALSE, TRUE);
    set_host_xform_for_view(&p_view->host_xform[XFORM_PANE], p_view, TRUE,  TRUE);

    view_edge_windows_cache_info(p_view);
}

/*
main events
*/

static void
vi_cmd_msg_reformat(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DOCU_REFORMAT p_docu_reformat)
{
    if(p_docu_reformat->data_type == REFORMAT_ALL || p_docu_reformat->action == REFORMAT_HEFO_Y)
    {
        VIEWNO viewno = VIEWNO_NONE;
        P_VIEW p_view;

        while(P_VIEW_NONE != (p_view = docu_enum_views(p_docu, &viewno)))
        {
            VIEW_RECT view_rect;

            view_rect.p_view = p_view;
            view_rect.rect.tl.x = view_rect.rect.tl.y = 0;
            view_rect.rect.br = p_view->paneextent;

            host_all_update_later(p_docu, &view_rect);
        }
    }
}

static void
vi_cmd_msg_row_extent_changed(
    _DocuRef_   P_DOCU p_docu)
{
    VIEWNO viewno = VIEWNO_NONE;
    P_VIEW p_view;

    while(P_VIEW_NONE != (p_view = docu_enum_views(p_docu, &viewno)))
        host_recache_visible_row_range(p_docu, p_view);
}

_Check_return_
static STATUS
vi_cmd_msg_post_save_2(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format)
{
    if(p_of_op_format->of_template.views)
    {
        P_VIEW p_view_caret = p_view_from_viewno_caret(p_docu);
        VIEWNO viewno = VIEWNO_NONE;
        P_VIEW p_view;

        while(P_VIEW_NONE != (p_view = docu_enum_views(p_docu, &viewno)))
            if(p_view != p_view_caret)
                status_return(save_this_view(p_docu, p_view, p_of_op_format));

        /* save current view */
        if(!IS_VIEW_NONE(p_view_caret))
            status_return(save_this_view(p_docu, p_view_caret, p_of_op_format));
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
vi_cmd_msg_save(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_MSG_SAVE p_msg_save)
{
    switch(p_msg_save->t5_msg_save_message)
    {
    case T5_MSG_SAVE__POST_SAVE_1:
        return(vi_edge_msg_post_save_1(p_docu, p_msg_save->p_of_op_format));

    case T5_MSG_SAVE__POST_SAVE_2:
        return(vi_cmd_msg_post_save_2(p_docu, p_msg_save->p_of_op_format));

    default:
        return(STATUS_OK);
    }
}

MAEVE_EVENT_PROTO(static, maeve_event_vi_cmd)
{
    IGNOREPARM_InRef_(p_maeve_block);

    switch(t5_message)
    {
    case T5_MSG_REFORMAT:
        vi_cmd_msg_reformat(p_docu, (PC_DOCU_REFORMAT) p_data);
        return(STATUS_OK);

    case T5_MSG_ROW_EXTENT_CHANGED:
        vi_cmd_msg_row_extent_changed(p_docu);
        return(STATUS_OK);

    case T5_MSG_SAVE:
        return(vi_cmd_msg_save(p_docu, (PC_MSG_SAVE) p_data));

    default:
        return(STATUS_OK);
    }
}

static const CTRL_VIEW_SKEL
ctrl_view_skel_pane_window_template =
{
    { TRUE,  TRUE  },

    { UPDATE_PANE_PAPER         /*event_paper_below      - now installed dynamically (per document) */ },
    { UPDATE_PANE_PRINT_AREA    /*event_print_below      - now installed dynamically */ },
    { UPDATE_PANE_CELLS_AREA    /*event_cells_area_below - now installed dynamically */ },
    { UPDATE_PANE_CELLS_AREA,   object_skel },
    { UPDATE_PANE_PAPER         /*event_paper_above      - now installed dynamically */ },
    { UPDATE_PANE_PRINT_AREA    /*event_print_above      - now installed dynamically */ },
    { UPDATE_PANE_CELLS_AREA    /*event_cells_area_above - now installed dynamically */ },

    { UPDATE_PANE_MARGIN_HEADER /*event_header           - now installed dynamically */ },
    { UPDATE_PANE_MARGIN_FOOTER /*event_footer           - now installed dynamically */ },
    { UPDATE_PANE_MARGIN_COL,   proc_event_margin_col },
    { UPDATE_PANE_MARGIN_ROW,   proc_event_margin_row },
};

/*
exported services hook
*/

MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_vi_cmd);

_Check_return_
static STATUS
vi_cmd_msg_init1(
    _DocuRef_   P_DOCU p_docu)
{
    p_docu->ctrl_view_skel_pane_window = ctrl_view_skel_pane_window_template;
    p_docu->scale_info.loaded = FALSE;
    p_docu->vscale_info.loaded = FALSE;

    return(maeve_event_handler_add(p_docu, maeve_event_vi_cmd, (CLIENT_HANDLE) 0));
}

_Check_return_
static STATUS
vi_cmd_msg_close1(
    _DocuRef_   P_DOCU p_docu)
{
    VIEWNO viewno = VIEWNO_NONE;
    P_VIEW p_view;

    maeve_event_handler_del(p_docu, maeve_event_vi_cmd, (CLIENT_HANDLE) 0);

    while(P_VIEW_NONE != (p_view = docu_enum_views(p_docu, &viewno)))
        view_destroy(p_docu, p_view);

    assert(0 == p_docu->n_views);
    al_array_dispose(&p_docu->h_view_table);

    return(STATUS_OK);
}

T5_MSG_PROTO(static, maeve_services_vi_cmd_msg_initclose, _InRef_ PC_MSG_INITCLOSE p_msg_initclose)
{
    IGNOREPARM_InVal_(t5_message);

    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__INIT1:
        return(vi_cmd_msg_init1(p_docu));

    case T5_MSG_IC__CLOSE1:
        return(vi_cmd_msg_close1(p_docu));

    default:
        return(STATUS_OK);
    }
}

MAEVE_SERVICES_EVENT_PROTO(extern, maeve_services_event_vi_cmd)
{
    IGNOREPARM_InRef_(p_maeve_services_block);

    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        return(maeve_services_vi_cmd_msg_initclose(p_docu, t5_message, (PC_MSG_INITCLOSE) p_data));

    default:
        return(STATUS_OK);
    }
}

/* end of vi_cmd.c */
