/* ob_dlg3.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* dialog UI handling */

/* SKS April 1992 */

#include "common/gflags.h"

#include "ob_dlg/ui_dlgin.h"

/******************************************************************************
*
* find who wants to handle messages to this control
*
******************************************************************************/

_Check_return_
_Ret_maybenull_
static P_PROC_DIALOG_EVENT
dialog_find_handler_in(
    _InRef_     PC_DIALOG_ICTL_GROUP p_ictl_group,
    _InVal_     DIALOG_CONTROL_ID dialog_control_id,
    _OutRef_    P_CLIENT_HANDLE p_client_handle,
    _InoutRef_  P_S32 p_this_branch /*IN=0*/ /*OUT*/)
{
    ARRAY_INDEX i;

    *p_client_handle = (CLIENT_HANDLE) NULL;

    /* now uses cached dialog control id ranges */
    if((p_ictl_group->max_dialog_control_id < dialog_control_id) || (p_ictl_group->min_dialog_control_id > dialog_control_id))
        return(NULL);

    for(i = 0; i < n_ictls_from_group(p_ictl_group); ++i)
    {
        const P_DIALOG_ICTL p_dialog_ictl = p_dialog_ictl_from(p_ictl_group, i);

        switch(p_dialog_ictl->dialog_control_type)
        {
        default:
            /* check at this level, get one of parents to fill in details */
            if(p_dialog_ictl->dialog_control_id == dialog_control_id)
            {
                *p_this_branch = 1;
                return(NULL);
            }
            break;

        case DIALOG_CONTROL_GROUPBOX:
            {
            P_PROC_DIALOG_EVENT p_proc_client;

            /* check at this level */
            if(p_dialog_ictl->dialog_control_id == dialog_control_id)
                *p_this_branch = 1;
            else
            /* recurse into subgroups */
            if(NULL != (p_proc_client = dialog_find_handler_in(&p_dialog_ictl->data.groupbox.ictls, dialog_control_id, p_client_handle, p_this_branch)))
                return(p_proc_client);

#if defined(DIALOG_GROUPBOX_CAN_HAVE_HANDLER)
            /* if control just found in this branch of the tree but hasn't yet found a handler, try supplying this one */
            if(*p_this_branch)
            {
                *p_client_handle = p_dialog_ictl->data.groupbox.client_handle;
                return(p_dialog_ictl->data.groupbox.p_proc_client /*maybe NULL*/);
            }
#endif

            break;
            }
        }
    }

    return(NULL);
}

_Check_return_
_Ret_maybenull_
extern P_PROC_DIALOG_EVENT
dialog_find_handler(
    _InRef_     PC_DIALOG p_dialog,
    _InVal_     DIALOG_CONTROL_ID dialog_control_id,
    _OutRef_    P_CLIENT_HANDLE p_client_handle)
{
    P_PROC_DIALOG_EVENT p_proc_client;
    S32 this_branch = 0;

    if(NULL != (p_proc_client = dialog_find_handler_in(&p_dialog->ictls, dialog_control_id, p_client_handle, &this_branch)))
        return(p_proc_client);

    /* fallback is entire dialog */
    return(dialog_main_handler(p_dialog, p_client_handle));
}

/******************************************************************************
*
* create one or more controls in a dialog
*
******************************************************************************/

static void
dialog_ictl_edit_xx_init(
    P_DIALOG p_dialog,
    P_DIALOG_ICTL p_dialog_ictl,
    _InRef_     PC_DIALOG_CONTROL_DATA_EDIT_XX pcd_edit_xx)
{
    P_DIALOG_ICTL_EDIT_XX p_dialog_ictl_edit_xx = p_dialog_ictl_edit_xx_from(p_dialog_ictl);

    if(NULL != p_dialog_ictl_edit_xx)
    {
        p_dialog_ictl_edit_xx->dialog_control_id = p_dialog_ictl->dialog_control_id;
        p_dialog_ictl_edit_xx->h_dialog = p_dialog->h_dialog;
        p_dialog_ictl_edit_xx->p_bitmap_validation = pcd_edit_xx->p_bitmap_validation; /* maybe NULL, defaults filled in later */

        p_dialog_ictl_edit_xx->read_only = pcd_edit_xx->bits.read_only;
        p_dialog_ictl_edit_xx->multiline = pcd_edit_xx->bits.multiline;
        p_dialog_ictl_edit_xx->always_update_later = pcd_edit_xx->bits.always_update_later;
        p_dialog_ictl_edit_xx->border_style = pcd_edit_xx->bits.border_style;
    }
}

_Check_return_
extern STATUS
dialog_ictls_bbox_in(
    P_DIALOG p_dialog,
    _InRef_     PC_DIALOG_ICTL_GROUP p_ictl_group,
    _InoutRef_  P_PIXIT_RECT p_rect)
{
    ARRAY_INDEX i;

    for(i = 0; i < n_ictls_from_group(p_ictl_group); ++i)
    {
        const P_DIALOG_ICTL p_dialog_ictl = p_dialog_ictl_from(p_ictl_group, i);
        PIXIT_RECT pixit_rect;

        dialog_control_rect(p_dialog, p_dialog_ictl->dialog_control_id, &pixit_rect);

        switch(p_dialog_ictl->dialog_control_type)
        {
        default:
            gr_box_union((P_GR_BOX) p_rect, (P_GR_BOX) p_rect, (P_GR_BOX) &pixit_rect);
            break;

        case DIALOG_CONTROL_GROUPBOX:
            {
            const BOOL logical_group = p_dialog_ictl->p_dialog_control->bits.logical_group || !p_dialog_ictl->p_dialog_control_data.groupbox || p_dialog_ictl->p_dialog_control_data.groupbox->bits.logical_group;

            if(!logical_group)
                gr_box_union((P_GR_BOX) p_rect, (P_GR_BOX) p_rect, (P_GR_BOX) &pixit_rect);

            status_return(dialog_ictls_bbox_in(p_dialog, &p_dialog_ictl->data.groupbox.ictls, p_rect));

            break;
            }
        }
    }

    return(STATUS_OK);
}

/******************************************************************************
*
* loop down over groups rebinding cached ranges
*
******************************************************************************/

static void
dialog_ictls_branch_recache(
    P_DIALOG_ICTL_GROUP p_ictl_group)
{
    ARRAY_INDEX i;

    p_ictl_group->max_dialog_control_id = 0;
    p_ictl_group->min_dialog_control_id = U16_MAX;

    for(i = 0; i < n_ictls_from_group(p_ictl_group); ++i)
    {
        const P_DIALOG_ICTL p_dialog_ictl = p_dialog_ictl_from(p_ictl_group, i);

        if( p_ictl_group->max_dialog_control_id < p_dialog_ictl->dialog_control_id)
            p_ictl_group->max_dialog_control_id = p_dialog_ictl->dialog_control_id;
        if( p_ictl_group->min_dialog_control_id > p_dialog_ictl->dialog_control_id)
            p_ictl_group->min_dialog_control_id = p_dialog_ictl->dialog_control_id;

        switch(p_dialog_ictl->dialog_control_type)
        {
        default:
            break;

        case DIALOG_CONTROL_GROUPBOX:
            { /* recurse into and accumulate from subgroups */
            dialog_ictls_branch_recache(&p_dialog_ictl->data.groupbox.ictls);

            if( p_ictl_group->max_dialog_control_id < p_dialog_ictl->data.groupbox.ictls.max_dialog_control_id)
                p_ictl_group->max_dialog_control_id = p_dialog_ictl->data.groupbox.ictls.max_dialog_control_id;
            if( p_ictl_group->min_dialog_control_id > p_dialog_ictl->data.groupbox.ictls.min_dialog_control_id)
                p_ictl_group->min_dialog_control_id = p_dialog_ictl->data.groupbox.ictls.min_dialog_control_id;

            break;
            }
        }
    }
}

static void
dialog_ictls_branch_uncache(
    P_DIALOG_ICTL_GROUP p_ictl_group)
{
    ARRAY_INDEX i;

    p_ictl_group->min_dialog_control_id = 0;
    p_ictl_group->max_dialog_control_id = U16_MAX;

    for(i = 0; i < n_ictls_from_group(p_ictl_group); ++i)
    {
        const P_DIALOG_ICTL p_dialog_ictl = p_dialog_ictl_from(p_ictl_group, i);

        switch(p_dialog_ictl->dialog_control_type)
        {
        default:
            break;

        case DIALOG_CONTROL_GROUPBOX:
            /* recurse into subgroups */
            dialog_ictls_branch_uncache(&p_dialog_ictl->data.groupbox.ictls);
            break;
        }
    }
}

_Check_return_
extern STATUS
dialog_ictls_create(
    P_DIALOG p_dialog,
    _InVal_     U32 n_ctls,
    _In_reads_(n_ctls) P_DIALOG_CTL_CREATE p_dialog_ctl_create)
{
    U32 i;
    STATUS status = STATUS_OK;

    assert(n_ctls > 0);

#if RISCOS
    dialog_riscos_cache_common_bitmaps(p_dialog);
#endif

    dialog_ictls_branch_uncache(&p_dialog->ictls);  /* blow the control id caches (forces full searching) */

    /* first pass creates the control structures */
    for(i = 0; i < n_ctls; ++i)
    {
        PC_DIALOG_CONTROL p_dialog_control = p_dialog_ctl_create[i].p_dialog_control.p_dialog_control;
        PC_DIALOG_CONTROL_DATA p_dialog_control_data;
        P_DIALOG_ICTL_GROUP p_ictl_group;
        P_DIALOG_ICTL p_dialog_ictl;
        BOOL suppress;

        if(NULL == p_dialog_control)
            continue;

        p_dialog_control_data.p_any = p_dialog_ctl_create[i].p_dialog_control_data;

#if CHECKING
        /* ensure that a control of the same id is not already there */
        p_dialog_ictl = p_dialog_ictl_from_control_id(p_dialog, p_dialog_control->dialog_control_id);
        myassert1x(!p_dialog_ictl, TEXT("duplicate definition of control id ") U32_TFMT, (U32) p_dialog_control->dialog_control_id);
#endif

        /* create a descriptor for this control in this dialog or in a subgroup */
        if(p_dialog_control->parent_dialog_control_id == DIALOG_CONTROL_WINDOW)
        {
            p_ictl_group = &p_dialog->ictls;
            suppress = 0;
        }
        else
        {
            P_DIALOG_ICTL parent_p_dialog_ictl = p_dialog_ictl_from_control_id(p_dialog, p_dialog_control->parent_dialog_control_id);

            PTR_ASSERT(parent_p_dialog_ictl);
            myassert3x(parent_p_dialog_ictl && (parent_p_dialog_ictl->dialog_control_type == DIALOG_CONTROL_GROUPBOX),
                      TEXT("stupid parent for control ") S32_TFMT TEXT(": parent_p_dialog_ictl ") PTR_XTFMT TEXT(", type ") S32_TFMT,
                      (U32) p_dialog_control->dialog_control_id, parent_p_dialog_ictl, (S32) (parent_p_dialog_ictl ? parent_p_dialog_ictl->dialog_control_type : 0));

            p_ictl_group = &parent_p_dialog_ictl->data.groupbox.ictls;
            suppress = !parent_p_dialog_ictl->bits.enabled;
        }

#if RISCOS
        dialog_riscos_tweak_edit_controls(p_ictl_group, 0);
#endif

        if(NULL == (p_dialog_ictl = al_array_extend_by(&p_ictl_group->handle, DIALOG_ICTL, 1, &dialog_statics.ictls_init_block, &status)))
            return(status);

#if RISCOS
        dialog_riscos_tweak_edit_controls(p_ictl_group, 1);
#endif

        p_dialog_ictl->p_dialog_control = p_dialog_control;
        p_dialog_ictl->p_dialog_control_data = p_dialog_control_data;
        p_dialog_ictl->dialog_control_id = p_dialog_control->dialog_control_id;
        p_dialog_ictl->dialog_control_type = UBF_UNPACK(DIALOG_CONTROL_TYPE, p_dialog_control->bits.packed_dialog_control_type);

        /* propogate enable suppression state at create time too */
        p_dialog_ictl->bits.enabled = 1;
        p_dialog_ictl->bits.enable_suppressed = UBF_PACK(suppress);

#if RISCOS
        p_dialog_ictl->riscos.dwi[0].icon_handle = BAD_WIMP_I;
        p_dialog_ictl->riscos.dwi[1].icon_handle = BAD_WIMP_I;
        p_dialog_ictl->riscos.dwi[2].icon_handle = BAD_WIMP_I;
#endif

        switch(p_dialog_ictl->dialog_control_type)
        {
        default: default_unhandled(); break;

        case DIALOG_CONTROL_GROUPBOX:
            p_dialog_ictl->state.radiobutton = DIALOG_RADIOSTATE_NONE;

            p_dialog_ictl->data.groupbox.ictls.min_dialog_control_id = 0; /* create without knowledge of contents 'cos they aren't in it yet! */
            p_dialog_ictl->data.groupbox.ictls.max_dialog_control_id = U16_MAX;

            if(p_dialog_control_data.groupbox)
            {
#if defined(DIALOG_GROUPBOX_CAN_HAVE_HANDLER)
                if(p_dialog_control_data.groupbox->bits.has_client)
                {
                    p_dialog_ictl->data.groupbox.client_handle = p_dialog_control_data.groupboxx->client_handle;
                    p_dialog_ictl->data.groupbox.p_proc_client = p_dialog_control_data.groupboxx->p_proc_client;
                }
#endif

                status_assert(status = ui_text_copy(&p_dialog_ictl->caption, &p_dialog_control_data.groupbox->caption));

#if RISCOS
                if(status_ok(status))
                    status_assert(status = dialog_riscos_remove_escape(p_dialog_ictl, &p_dialog_ictl->caption));
#endif
            }
            break;

        case DIALOG_CONTROL_STATICTEXT:
        case DIALOG_CONTROL_STATICFRAME:
            break;

        case DIALOG_CONTROL_STATICPICTURE:
#if RISCOS
            p_dialog_ictl->data.staticpicture.riscos.resource_bitmap_handle = resource_bitmap_find_defaulting(&p_dialog_control_data.staticpicture->picture_bitmap_id);
#endif
            break;

        case DIALOG_CONTROL_PUSHBUTTON:
#if RISCOS
            p_dialog_ictl->riscos.hot_key = (U8) p_dialog_ictl->p_dialog_control_data.pushbutton->push_xx.hot_key;
#endif
            break;

        case DIALOG_CONTROL_PUSHPICTURE:
#if RISCOS
            p_dialog_ictl->riscos.hot_key = (U8) p_dialog_ictl->p_dialog_control_data.pushpicture->push_xx.hot_key;

            p_dialog_ictl->data.pushpicture.riscos.resource_bitmap_handle = resource_bitmap_find(&p_dialog_control_data.pushpicture->picture_bitmap_id);
#endif
            break;

        case DIALOG_CONTROL_RADIOBUTTON:
            /* radiobuttons no longer allowed in the root */
            assert(p_ictl_group != &p_dialog->ictls);
            p_dialog_ictl->state.radiobutton = DIALOG_RADIOSTATE_NONE;

            status_assert(status = ui_text_copy(&p_dialog_ictl->caption, &p_dialog_control_data.radiobutton->caption));

#if RISCOS
            if(status_ok(status))
                status_assert(status = dialog_riscos_remove_escape(p_dialog_ictl, &p_dialog_ictl->caption));
#endif
            break;

        case DIALOG_CONTROL_RADIOPICTURE:
            {
#if RISCOS
            PC_RESOURCE_BITMAP_ID p_bitmap_id_off = p_dialog_ictl->p_dialog_control_data.radiopicture->p_bitmap_id_offon;
            PC_RESOURCE_BITMAP_ID p_bitmap_id_on = NULL;

            if((NULL != p_bitmap_id_off) && p_dialog_ictl->p_dialog_control_data.radiopicture->bits.has_n_bmp)
                p_bitmap_id_on = p_bitmap_id_off + 1;

            if(NULL == p_bitmap_id_on)
                p_bitmap_id_on = p_bitmap_id_off;

            p_dialog_ictl->data.radiopicture.riscos.resource_bitmap_handle_off = resource_bitmap_find_defaulting(p_bitmap_id_off);
            p_dialog_ictl->data.radiopicture.riscos.resource_bitmap_handle_on  = resource_bitmap_find_defaulting(p_bitmap_id_on);
#endif

            /* radiobuttons no longer allowed in the root */
            assert(p_ictl_group != &p_dialog->ictls);
            p_dialog_ictl->state.radiobutton = DIALOG_RADIOSTATE_NONE;
            break;
            }

        case DIALOG_CONTROL_CHECKBOX:
            status_assert(status = ui_text_copy(&p_dialog_ictl->caption, &p_dialog_control_data.checkbox->caption));

#if RISCOS
            if(status_ok(status))
                status_assert(status = dialog_riscos_remove_escape(p_dialog_ictl, &p_dialog_ictl->caption));
#endif
            break;

        case DIALOG_CONTROL_CHECKPICTURE:
            {
#if RISCOS
            PC_RESOURCE_BITMAP_ID p_bitmap_id_off = p_dialog_ictl->p_dialog_control_data.checkpicture->p_bitmap_id_offon;
            PC_RESOURCE_BITMAP_ID p_bitmap_id_on = NULL;

            if((NULL != p_bitmap_id_off) && p_dialog_ictl->p_dialog_control_data.checkpicture->bits.has_n_bmp)
                p_bitmap_id_on = p_bitmap_id_off + 1;

            if(NULL == p_bitmap_id_on)
                p_bitmap_id_on = p_bitmap_id_off;

            p_dialog_ictl->data.checkpicture.riscos.resource_bitmap_handle_off = resource_bitmap_find_defaulting(p_bitmap_id_off);
            p_dialog_ictl->data.checkpicture.riscos.resource_bitmap_handle_on  = resource_bitmap_find_defaulting(p_bitmap_id_on);
#endif
            break;
            }

#ifdef DIALOG_HAS_TRISTATE
        case DIALOG_CONTROL_TRISTATE:
            status_assert(status = ui_text_copy(&p_dialog_ictl->caption, &p_dialog_control_data.checkbox->caption));

#if RISCOS
            if(status_ok(status))
                status_assert(status = dialog_riscos_remove_escape(p_dialog_ictl, &p_dialog_ictl->caption));

            p_dialog_ictl->data.tristate.riscos.caption_i.i = BAD_WIMP_I;
#endif
            break;

        case DIALOG_CONTROL_TRIPICTURE:
            {
#if RISCOS
            PC_RESOURCE_BITMAP_ID p_bitmap_id_off = p_dialog_ictl->p_dialog_control_data.tripicture->p_bitmap_id_offondontcare;
            PC_RESOURCE_BITMAP_ID p_bitmap_id_on = NULL;
            PC_RESOURCE_BITMAP_ID p_bitmap_id_dontcare = NULL;

            if((NULL != p_bitmap_id_off) && p_dialog_ictl->p_dialog_control_data.tripicture->bits.has_n_bmp)
            {
                p_bitmap_id_on = p_bitmap_id_off + 1;
                p_bitmap_id_dontcare = p_bitmap_id_off + 2;
            }

            if(NULL == p_bitmap_id_on)
                p_bitmap_id_on = p_bitmap_id_off;

            if(NULL == p_bitmap_id_dontcare)
                p_bitmap_id_dontcare = p_bitmap_id_off;

            p_dialog_ictl->data.tripicture.riscos.resource_bitmap_handle_off = resource_bitmap_find_defaulting(p_bitmap_id_dontcare);
            p_dialog_ictl->data.tripicture.riscos.resource_bitmap_handle_on  = resource_bitmap_find_defaulting(p_bitmap_id_dontcare);
            p_dialog_ictl->data.tripicture.riscos.resource_bitmap_handle_dont_care = resource_bitmap_find_defaulting(p_bitmap_id_dontcare);
#endif
            break;
            }
#endif

        case DIALOG_CONTROL_EDIT:
            dialog_ictl_edit_xx_init(p_dialog, p_dialog_ictl, &p_dialog_control_data.edit->edit_xx);

            if(!p_dialog_ictl->data.edit.edit_xx.p_bitmap_validation)
                p_dialog_ictl->data.edit.edit_xx.p_bitmap_validation = p_dialog_ictl->data.edit.edit_xx.multiline
                                                                      ? dialog_statics.bitmap_validation_edit_multiline
                                                                      : dialog_statics.bitmap_validation_edit;
            break;

        case DIALOG_CONTROL_BUMP_S32:
        case DIALOG_CONTROL_BUMP_F64:
            {
            const PC_DIALOG_CONTROL_DATA_BUMP_XX pcd_bump_xx = p_dialog_control_data.bump_xx;
            P_BITMAP p_bitmap_validation_default;

            switch(p_dialog_ictl->dialog_control_type)
            {
            default: default_unhandled();
#if CHECKING
            case DIALOG_CONTROL_BUMP_S32:
#endif
                p_bitmap_validation_default = dialog_statics.bitmap_validation_s32;
                break;

            case DIALOG_CONTROL_BUMP_F64:
                p_bitmap_validation_default = dialog_statics.bitmap_validation_f64;
                break;
            }

            dialog_ictl_edit_xx_init(p_dialog, p_dialog_ictl, &pcd_bump_xx->edit_xx);

            if(!p_dialog_ictl->data.bump_xx.edit_xx.p_bitmap_validation)
                p_dialog_ictl->data.bump_xx.edit_xx.p_bitmap_validation = p_bitmap_validation_default;
            break;
            }

        case DIALOG_CONTROL_LIST_S32:
        case DIALOG_CONTROL_LIST_TEXT:
            p_dialog_ictl->state.list_text.itemno = DIALOG_CTL_STATE_LIST_ITEM_NONE;
            break;

        case DIALOG_CONTROL_COMBO_TEXT:
        case DIALOG_CONTROL_COMBO_S32:
            dialog_ictl_edit_xx_init(p_dialog, p_dialog_ictl, &p_dialog_control_data.combo_text->combo_xx.edit_xx);

            if(!p_dialog_ictl->data.combo_xx.edit_xx.p_bitmap_validation)
                p_dialog_ictl->data.combo_xx.edit_xx.p_bitmap_validation = (p_dialog_ictl->dialog_control_type == DIALOG_CONTROL_COMBO_S32)
                                                                         ? dialog_statics.bitmap_validation_s32
                                                                         : dialog_statics.bitmap_validation_edit;

            p_dialog_ictl->state.combo_text.itemno = DIALOG_CTL_STATE_LIST_ITEM_NONE;
            break;

        case DIALOG_CONTROL_USER:
            p_dialog_ictl->data.user.border_style = p_dialog_control_data.user->bits.border_style;
            break;
        }

        status_return(status);

#if RISCOS
        /* create any associated EDIT_XX slec/mlec as necessary */
        status_return(dialog_riscos_ictl_edit_xx_create(p_dialog_ictl));
#else
        status = STATUS_OK;
#endif
    }

    dialog_ictls_branch_recache(&p_dialog->ictls); /* now cache fully again */

    /* second pass sets up the state if required and informs the punter */
    for(i = 0; i < n_ctls; ++i)
    {
        PC_DIALOG_CONTROL p_dialog_control = p_dialog_ctl_create[i].p_dialog_control.p_dialog_control;
        PC_DIALOG_CONTROL_DATA p_dialog_control_data;
        P_DIALOG_ICTL p_dialog_ictl;
        BOOL state_set = 1;
        DIALOG_MSG_CTL_CREATE_STATE dialog_msg_ctl_create_state;

        if(NULL == p_dialog_control)
            continue;

        p_dialog_control_data.p_any = p_dialog_ctl_create[i].p_dialog_control_data;

        p_dialog_ictl = p_dialog_ictl_from_control_id(p_dialog, p_dialog_control->dialog_control_id);
        PTR_ASSERT(p_dialog_ictl);
        assert(p_dialog_ictl->p_dialog_control == p_dialog_control);
        assert(p_dialog_ictl->p_dialog_control_data.p_any == p_dialog_control_data.p_any);

        /* ask client to fill in source for list controls */
        switch(p_dialog_ictl->dialog_control_type)
        {
        default:
            break;

        case DIALOG_CONTROL_LIST_S32:
        case DIALOG_CONTROL_LIST_TEXT:
        case DIALOG_CONTROL_COMBO_S32:
        case DIALOG_CONTROL_COMBO_TEXT:
            {
            P_PROC_DIALOG_EVENT p_proc_client;
            DIALOG_MSG_CTL_FILL_SOURCE dialog_msg_ctl_fill_source;
            msgclr(dialog_msg_ctl_fill_source);

            if(NULL != (p_proc_client = dialog_find_handler(p_dialog, p_dialog_ictl->dialog_control_id, &dialog_msg_ctl_fill_source.client_handle)))
            {
                DIALOG_MSG_CTL_HDR_from_dialog_ictl(dialog_msg_ctl_fill_source, p_dialog, p_dialog_ictl);

                dialog_msg_ctl_fill_source.p_ui_source = NULL;
                dialog_msg_ctl_fill_source.p_ui_control_s32 = NULL;

                switch(p_dialog_ictl->dialog_control_type)
                {
                default:
                    break;

                case DIALOG_CONTROL_LIST_S32:
                    dialog_msg_ctl_fill_source.p_ui_control_s32 = p_dialog_ictl->p_dialog_control_data.list_s32->p_ui_control_s32;
                    break;

                case DIALOG_CONTROL_COMBO_S32:
                    dialog_msg_ctl_fill_source.p_ui_control_s32 = p_dialog_ictl->p_dialog_control_data.combo_s32->p_ui_control_s32;
                    break;
                }

                status_assert(status = dialog_call_client(p_dialog, DIALOG_MSG_CODE_CTL_FILL_SOURCE, &dialog_msg_ctl_fill_source, p_proc_client));

                switch(p_dialog_ictl->dialog_control_type)
                {
                default: default_unhandled();
#if CHECKING
                case DIALOG_CONTROL_LIST_S32:
                case DIALOG_CONTROL_LIST_TEXT:
#endif
                    p_dialog_ictl->data.list_xx.list_xx.p_ui_source = dialog_msg_ctl_fill_source.p_ui_source;
                    p_dialog_ictl->data.list_xx.list_xx.p_ui_control_s32 = dialog_msg_ctl_fill_source.p_ui_control_s32;
                    break;

                case DIALOG_CONTROL_COMBO_S32:
                case DIALOG_CONTROL_COMBO_TEXT:
                    p_dialog_ictl->data.combo_xx.list_xx.p_ui_source = dialog_msg_ctl_fill_source.p_ui_source;
                    p_dialog_ictl->data.combo_xx.list_xx.p_ui_control_s32 = dialog_msg_ctl_fill_source.p_ui_control_s32;
                    break;
                }
            }

            break;
            }
        }

        msgclr(dialog_msg_ctl_create_state);
        DIALOG_MSG_CTL_HDR_from_dialog_ictl(dialog_msg_ctl_create_state, p_dialog, p_dialog_ictl);
        zero_struct(dialog_msg_ctl_create_state.state_set.state);

        switch(p_dialog_ictl->dialog_control_type)
        {
        default: default_unhandled();
#if CHECKING
        case DIALOG_CONTROL_RADIOBUTTON:
        case DIALOG_CONTROL_RADIOPICTURE:
        case DIALOG_CONTROL_STATICPICTURE:
        case DIALOG_CONTROL_PUSHPICTURE:
#endif
            state_set = 0;
            break;

        case DIALOG_CONTROL_USER:
            __analysis_assume(p_dialog_control_data.user);
            dialog_msg_ctl_create_state.state_set.state.user = p_dialog_control_data.user->state;
            break;

        case DIALOG_CONTROL_STATICTEXT:
        case DIALOG_CONTROL_STATICFRAME:
            __analysis_assume(p_dialog_control_data.statictext);
            dialog_msg_ctl_create_state.state_set.state.statictext = p_dialog_control_data.statictext->caption;
            break;

        case DIALOG_CONTROL_PUSHBUTTON:
            __analysis_assume(p_dialog_control_data.pushbutton);
            dialog_msg_ctl_create_state.state_set.state.pushbutton = p_dialog_control_data.pushbutton->caption;
            break;

        case DIALOG_CONTROL_GROUPBOX:
            dialog_msg_ctl_create_state.state_set.state = p_dialog_ictl->state;
            break;

        case DIALOG_CONTROL_CHECKBOX:
            __analysis_assume(p_dialog_control_data.checkbox);
            dialog_msg_ctl_create_state.state_set.state.checkbox = p_dialog_control_data.checkbox->init_state;
            break;

        case DIALOG_CONTROL_CHECKPICTURE:
            __analysis_assume(p_dialog_control_data.checkpicture);
            dialog_msg_ctl_create_state.state_set.state.checkbox = p_dialog_control_data.checkpicture->init_state;
            break;

#ifdef DIALOG_HAS_TRISTATE
        case DIALOG_CONTROL_TRISTATE:
            dialog_msg_ctl_create_state.state_set.state.tristate = p_dialog_control_data.tristate->init_state;
            break;

        case DIALOG_CONTROL_TRIPICTURE:
            dialog_msg_ctl_create_state.state_set.state.tristate = p_dialog_control_data.tripicture->init_state;
            break;
#endif

        case DIALOG_CONTROL_EDIT:
            __analysis_assume(p_dialog_control_data.edit);
            dialog_msg_ctl_create_state.state_set.state.edit.ui_text = p_dialog_control_data.edit->state;
            break;

        case DIALOG_CONTROL_BUMP_S32:
            __analysis_assume(p_dialog_control_data.bump_s32);
            dialog_msg_ctl_create_state.state_set.state.bump_s32 = p_dialog_control_data.bump_s32->state;
            break;

        case DIALOG_CONTROL_BUMP_F64:
            __analysis_assume(p_dialog_control_data.bump_f64);
            dialog_msg_ctl_create_state.state_set.state.bump_f64 = p_dialog_control_data.bump_f64->state;
            break;

        case DIALOG_CONTROL_LIST_S32:
            if(p_dialog_control_data.list_s32)
                dialog_msg_ctl_create_state.state_set.state.list_s32.s32 = p_dialog_control_data.list_s32->state;
            break;

        case DIALOG_CONTROL_LIST_TEXT:
            if(p_dialog_control_data.list_text)
                dialog_msg_ctl_create_state.state_set.state.list_text.ui_text = p_dialog_control_data.list_text->state;
            break;

        case DIALOG_CONTROL_COMBO_S32:
            if(p_dialog_control_data.combo_s32)
                dialog_msg_ctl_create_state.state_set.state.combo_s32.s32 = p_dialog_control_data.combo_s32->state;
            break;

        case DIALOG_CONTROL_COMBO_TEXT:
            if(p_dialog_control_data.combo_text)
                dialog_msg_ctl_create_state.state_set.state.combo_text.ui_text = p_dialog_control_data.combo_text->state;
            break;
        }

        status = STATUS_OK;

        if(state_set)
        {
            P_PROC_DIALOG_EVENT p_proc_client;

            dialog_msg_ctl_create_state.state_set.h_dialog = p_dialog->h_dialog;
            dialog_msg_ctl_create_state.state_set.dialog_control_id = p_dialog_ictl->dialog_control_id;
            dialog_msg_ctl_create_state.state_set.bits = 0;

            dialog_msg_ctl_create_state.processed = 0;

            /* inform client of control state creation */
            if(NULL != (p_proc_client = dialog_find_handler(p_dialog, p_dialog_ictl->dialog_control_id, &dialog_msg_ctl_create_state.client_handle)))
            {
                status_assert(status = dialog_call_client(p_dialog, DIALOG_MSG_CODE_CTL_CREATE_STATE, &dialog_msg_ctl_create_state, p_proc_client));
            }

            if(!dialog_msg_ctl_create_state.processed && status_ok(status))
                status_assert(status = object_call_DIALOG(DIALOG_CMD_CODE_CTL_STATE_SET, &dialog_msg_ctl_create_state.state_set));
        }

        /* inform client of control creation */
        if(status_ok(status))
        {
            P_PROC_DIALOG_EVENT p_proc_client;
            DIALOG_MSG_CTL_CREATE dialog_msg_ctl_create;
            msgclr(dialog_msg_ctl_create);

            if(NULL != (p_proc_client = dialog_find_handler(p_dialog, p_dialog_ictl->dialog_control_id, &dialog_msg_ctl_create.client_handle)))
            {
                DIALOG_MSG_CTL_HDR_from_dialog_ictl(dialog_msg_ctl_create, p_dialog, p_dialog_ictl);

                status_assert(status = dialog_call_client(p_dialog, DIALOG_MSG_CODE_CTL_CREATE, &dialog_msg_ctl_create, p_proc_client));

                if(status_ok(status))
                    p_dialog_ictl->bits.msg_ctl_create_sent = 1;
            }
        }

        if(status_fail(status))
            return(status);

        switch(p_dialog_ictl->dialog_control_type)
        {
        case DIALOG_CONTROL_PUSHBUTTON:
        case DIALOG_CONTROL_PUSHPICTURE:
            __analysis_assume(p_dialog_ictl->p_dialog_control_data.pushbutton);
            if(p_dialog_ictl->p_dialog_control_data.pushbutton->push_xx.help_id_offset)
                p_dialog->help_dialog_control_id = p_dialog_ictl->dialog_control_id;

            if((p_dialog_ictl->dialog_control_id == IDOK) || p_dialog_ictl->p_dialog_control_data.pushbutton->push_xx.def_pushbutton)
            {
                DIALOG_CMD_CTL_SET_DEFAULT dialog_cmd_ctl_set_default;
                msgclr(dialog_cmd_ctl_set_default);
                dialog_cmd_ctl_set_default.h_dialog = p_dialog->h_dialog;
                dialog_cmd_ctl_set_default.dialog_control_id = p_dialog_ictl->dialog_control_id;
                status_assert(object_call_DIALOG(DIALOG_CMD_CODE_CTL_SET_DEFAULT, &dialog_cmd_ctl_set_default));
            }
            break;

        default:
            break;
        }
    }

    return(STATUS_OK);
}

/******************************************************************************
*
* dispose of a group of dialog box controls
*
******************************************************************************/

extern void
dialog_ictls_dispose_in(
    /*inout*/ P_DIALOG p_dialog,
    /*inout*/ P_DIALOG_ICTL_GROUP p_ictl_group)
{
    ARRAY_INDEX i;

    for(i = 0; i < n_ictls_from_group(p_ictl_group); ++i)
    {
        const P_DIALOG_ICTL p_dialog_ictl = p_dialog_ictl_from(p_ictl_group, i);

        /* dispose of a group's contents first */
        if(p_dialog_ictl->dialog_control_type == DIALOG_CONTROL_GROUPBOX)
            dialog_ictls_dispose_in(p_dialog, &p_dialog_ictl->data.groupbox.ictls);

        /* send a MSG_CTL_DISPOSE iff MSG_CTL_CREATE sent */
        if(p_dialog_ictl->bits.msg_ctl_create_sent)
        {
            P_PROC_DIALOG_EVENT p_proc_client;
            DIALOG_MSG_CTL_DISPOSE dialog_msg_ctl_dispose;
            msgclr(dialog_msg_ctl_dispose);

            p_dialog_ictl->bits.msg_ctl_create_sent = 0;

            if(NULL != (p_proc_client = dialog_find_handler(p_dialog, p_dialog_ictl->dialog_control_id, &dialog_msg_ctl_dispose.client_handle)))
            {
                DIALOG_MSG_CTL_HDR_from_dialog_ictl(dialog_msg_ctl_dispose, p_dialog, p_dialog_ictl);
                dialog_msg_ctl_dispose.p_dialog_control_data = p_dialog_ictl->p_dialog_control_data.p_any;
                status_assert(dialog_call_client(p_dialog, DIALOG_MSG_CODE_CTL_DISPOSE, &dialog_msg_ctl_dispose, p_proc_client));
            }
        }

        ui_text_dispose(&p_dialog_ictl->caption);

#if RISCOS
        tstr_clr(&p_dialog_ictl->riscos.caption);
#endif

        switch(p_dialog_ictl->dialog_control_type)
        {
        default: default_unhandled();
#if CHECKING
        case DIALOG_CONTROL_USER:
        case DIALOG_CONTROL_EDIT:
#endif
            break;

        case DIALOG_CONTROL_GROUPBOX:
            break;

        case DIALOG_CONTROL_STATICTEXT:
        case DIALOG_CONTROL_STATICFRAME:
            break;

        case DIALOG_CONTROL_STATICPICTURE:
#if RISCOS
            resource_bitmap_lose(&p_dialog_ictl->data.staticpicture.riscos.resource_bitmap_handle);
#endif
            break;

        case DIALOG_CONTROL_PUSHBUTTON:
            break;

        case DIALOG_CONTROL_PUSHPICTURE:
#if RISCOS
            resource_bitmap_lose(&p_dialog_ictl->data.pushpicture.riscos.resource_bitmap_handle);
#endif
            break;

        case DIALOG_CONTROL_RADIOBUTTON:
            break;

        case DIALOG_CONTROL_RADIOPICTURE:
#if RISCOS
            resource_bitmap_lose(&p_dialog_ictl->data.radiopicture.riscos.resource_bitmap_handle_on);
            resource_bitmap_lose(&p_dialog_ictl->data.radiopicture.riscos.resource_bitmap_handle_off);
#endif
            break;

        case DIALOG_CONTROL_CHECKBOX:
            break;

        case DIALOG_CONTROL_CHECKPICTURE:
#if RISCOS
            resource_bitmap_lose(&p_dialog_ictl->data.checkpicture.riscos.resource_bitmap_handle_on);
            resource_bitmap_lose(&p_dialog_ictl->data.checkpicture.riscos.resource_bitmap_handle_off);
#endif
            break;

#ifdef DIALOG_HAS_TRISTATE
        case DIALOG_CONTROL_TRISTATE:
            break;

        case DIALOG_CONTROL_TRIPICTURE:
#if RISCOS
            resource_bitmap_lose(&p_dialog_ictl->data.tripicture.riscos.resource_bitmap_handle_on);
            resource_bitmap_lose(&p_dialog_ictl->data.tripicture.riscos.resource_bitmap_handle_off);
            resource_bitmap_lose(&p_dialog_ictl->data.tripicture.riscos.resource_bitmap_handle_dont_care);
#endif
            break;
#endif

        case DIALOG_CONTROL_BUMP_S32:
        case DIALOG_CONTROL_BUMP_F64:
            break;

        case DIALOG_CONTROL_LIST_TEXT:
        case DIALOG_CONTROL_LIST_S32:
#if RISCOS
            status_assert(ri_lbox_dispose(&p_dialog_ictl->data.list_xx.list_xx.riscos.lbox));
#endif
            break;

        case DIALOG_CONTROL_COMBO_TEXT:
        case DIALOG_CONTROL_COMBO_S32:
#if RISCOS
            status_assert(ri_lbox_dispose(&p_dialog_ictl->data.combo_xx.list_xx.riscos.lbox));
#endif
            break;
        }

#if RISCOS
        /* no need to detach handler */
        dialog_riscos_ictl_edit_xx_destroy(p_dialog, p_dialog_ictl);
#endif

        switch(p_dialog_ictl->dialog_control_type)
        {
        default:
            break;

        case DIALOG_CONTROL_STATICTEXT:
        case DIALOG_CONTROL_STATICFRAME:
        case DIALOG_CONTROL_PUSHBUTTON:
            ui_text_dispose(&p_dialog_ictl->state.pushbutton);
            break;

        case DIALOG_CONTROL_EDIT:
        case DIALOG_CONTROL_LIST_TEXT:
        case DIALOG_CONTROL_COMBO_TEXT:
            ui_text_dispose(&p_dialog_ictl->state.list_text.ui_text);
            break;
        }
    }

    al_array_dispose(&p_ictl_group->handle);
}

/******************************************************************************
*
* find who wants to handle messages to this dialog
*
******************************************************************************/

_Check_return_
_Ret_maybenull_
extern P_PROC_DIALOG_EVENT
dialog_main_handler(
    _InRef_     PC_DIALOG p_dialog,
    _OutRef_    P_CLIENT_HANDLE p_client_handle)
{
    *p_client_handle= p_dialog->client_handle;

    return(p_dialog->p_proc_client);
}

/******************************************************************************
*
* search list of dialogs using handle
*
******************************************************************************/

_Check_return_
extern P_DIALOG
p_dialog_from_h_dialog(
    _InVal_     H_DIALOG h_dialog)
{
    ARRAY_INDEX i = array_elements(&dialog_statics.handles);

    while(--i >= 0)
    {
        P_DIALOG p_dialog = array_ptr(&dialog_statics.handles, DIALOG, i);

        if(p_dialog->h_dialog == h_dialog)
            return(p_dialog);
    }

    return(NULL);
}

extern P_DIALOG_ICTL_EDIT_XX
p_dialog_ictl_edit_xx_from(
    P_DIALOG_ICTL p_dialog_ictl)
{
    switch(p_dialog_ictl->dialog_control_type)
    {
    default:
        return(NULL);

    case DIALOG_CONTROL_EDIT:
        return(&p_dialog_ictl->data.edit.edit_xx);

    case DIALOG_CONTROL_BUMP_S32:
    case DIALOG_CONTROL_BUMP_F64:
        return(&p_dialog_ictl->data.bump_xx.edit_xx);

    case DIALOG_CONTROL_COMBO_S32:
    case DIALOG_CONTROL_COMBO_TEXT:
        return(&p_dialog_ictl->data.combo_xx.edit_xx);
    }
}

/******************************************************************************
*
* scan all controls in dialog for the given control id
*
******************************************************************************/

extern P_DIALOG_ICTL
p_dialog_ictl_from_control_id(
    P_DIALOG p_dialog,
    _InVal_     DIALOG_CONTROL_ID dialog_control_id)
{
    return(p_dialog_ictl_from_control_id_in(&p_dialog->ictls, dialog_control_id, NULL));
}

/******************************************************************************
*
* scan a group of controls for the given control id
*
******************************************************************************/

extern P_DIALOG_ICTL
p_dialog_ictl_from_control_id_in(
    P_DIALOG_ICTL_GROUP p_ictl_group,
    _InVal_     DIALOG_CONTROL_ID dialog_control_id,
    /*out*/ P_P_DIALOG_ICTL_GROUP p_p_parent_ictls)
{
    ARRAY_INDEX i;

    if(NULL != p_p_parent_ictls)
        *p_p_parent_ictls = NULL;

    /* now uses cached control id ranges */
    if((p_ictl_group->max_dialog_control_id < dialog_control_id) || (p_ictl_group->min_dialog_control_id > dialog_control_id))
        return(NULL);

    for(i = 0; i < n_ictls_from_group(p_ictl_group); ++i)
    {
        P_DIALOG_ICTL p_dialog_ictl = p_dialog_ictl_from(p_ictl_group, i);

        /* check at this level */
        if(p_dialog_ictl->dialog_control_id == dialog_control_id)
        {
            if(NULL != p_p_parent_ictls)
                *p_p_parent_ictls = p_ictl_group;

            return(p_dialog_ictl);
        }

        switch(p_dialog_ictl->dialog_control_type)
        {
        default:
            break;

        case DIALOG_CONTROL_GROUPBOX:
            /* recurse into subgroups */
            if(NULL != (p_dialog_ictl = p_dialog_ictl_from_control_id_in(&p_dialog_ictl->data.groupbox.ictls, dialog_control_id, p_p_parent_ictls)))
                /* leaving *p_p_parent_ictls well alone, just found somewhere lower down tree */
                return(p_dialog_ictl);
            break;
        }
    }

    return(NULL);
}

/*
main events
*/

/* detect when document we want to feed dialog events to goes away */

_Check_return_
static STATUS
dialog_msg_close1(
    _InVal_     H_DIALOG h_dialog)
{
    const P_DIALOG p_dialog = p_dialog_from_h_dialog(h_dialog);

    /* document we will be feeding commands to is going AWOL */
    p_dialog->docno = DOCNO_NONE;

    { /* complete dbox */
    DIALOG_CMD_COMPLETE_DBOX dialog_cmd_complete_dbox;
    msgclr(dialog_cmd_complete_dbox);
    dialog_cmd_complete_dbox.h_dialog = p_dialog->h_dialog;
    dialog_cmd_complete_dbox.completion_code = DIALOG_COMPLETION_CANCEL;
    status_assert(object_call_DIALOG(DIALOG_CMD_CODE_COMPLETE_DBOX, &dialog_cmd_complete_dbox));
    } /*block*/

    return(STATUS_OK);
}

_Check_return_
static STATUS
maeve_dialog_msg_initclose(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message_dummy,
    _InRef_     PC_MSG_INITCLOSE p_msg_initclose,
    _InRef_     PC_MAEVE_BLOCK p_maeve_block)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message_dummy); /* dummy for APCS */

    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__CLOSE1:
        {
        const H_DIALOG h_dialog = (H_DIALOG) p_maeve_block->client_handle;
        maeve_event_handler_del_handle(p_docu, p_maeve_block->maeve_handle);
        return(dialog_msg_close1(h_dialog));
        }

    default:
        return(STATUS_OK);
    }
}

MAEVE_EVENT_PROTO(extern, maeve_event_dialog)
{
    UNREFERENCED_PARAMETER_InRef_(p_maeve_block);

    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        return(maeve_dialog_msg_initclose(p_docu, t5_message, (PC_MSG_INITCLOSE) p_data, p_maeve_block));

    default:
        return(STATUS_OK);
    }
}

_Check_return_
static STATUS
dialog_stolen_focus_msg_close1(
    _InVal_     H_DIALOG h_dialog)
{
    const P_DIALOG p_dialog = p_dialog_from_h_dialog(h_dialog);

    /* document we will be restoring focus to is going AWOL */
    p_dialog->stolen_focus = 0;

    return(STATUS_OK);
}

_Check_return_
static STATUS
maeve_dialog_stolen_focus_msg_initclose(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message_dummy,
    _InRef_     PC_MSG_INITCLOSE p_msg_initclose,
    _InRef_     PC_MAEVE_BLOCK p_maeve_block)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message_dummy); /* dummy for APCS */

    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__CLOSE1:
        {
        const H_DIALOG h_dialog = (H_DIALOG) p_maeve_block->client_handle;
        maeve_event_handler_del_handle(p_docu, p_maeve_block->maeve_handle);
        return(dialog_stolen_focus_msg_close1(h_dialog));
        }

    default:
        return(STATUS_OK);
    }
}

MAEVE_EVENT_PROTO(extern, maeve_event_dialog_stolen_focus)
{
    UNREFERENCED_PARAMETER_InRef_(p_maeve_block);

    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        return(maeve_dialog_stolen_focus_msg_initclose(p_docu, t5_message, (PC_MSG_INITCLOSE) p_data, p_maeve_block));

    default:
        return(STATUS_OK);
    }
}

/******************************************************************************
*
* extract the text from a control which has an edit control
*
******************************************************************************/

#if RISCOS

_Check_return_
extern STATUS
ui_text_from_ictl_edit_xx(
    _OutRef_    P_UI_TEXT p_ui_text,
    P_DIALOG_ICTL_EDIT_XX p_dialog_ictl_edit_xx)
{
    STATUS status = STATUS_OK;

    p_ui_text->type = UI_TEXT_TYPE_NONE;

    if(NULL != p_dialog_ictl_edit_xx->riscos.mlec)
    {
        S32 tstr_len = mlec_GetTextLen(p_dialog_ictl_edit_xx->riscos.mlec) + 1 /*CH_NULL*/;
        PTSTR tstr;

        if(NULL != (tstr = al_array_alloc_TCHAR(&p_ui_text->text.array_handle_tstr, tstr_len, &array_init_block_tchar, &status)))
        {
            p_ui_text->type = UI_TEXT_TYPE_TSTR_ARRAY; /*TSTR==SBSTR*/

            status_assert(mlec_GetText(p_dialog_ictl_edit_xx->riscos.mlec, tstr, tstr_len));

            tstr[tstr_len-1] = CH_NULL; /* paranoia */
        }
    }

    return(status);
}

#elif WINDOWS

_Check_return_
extern STATUS
ui_text_from_ictl_edit_xx(
    _OutRef_    P_UI_TEXT p_ui_text,
    P_DIALOG p_dialog,
    P_DIALOG_ICTL p_dialog_ictl)
{
    const HWND hwnd = GetDlgItem(p_dialog->hwnd, p_dialog_ictl->windows.wid);
    U32 tstr_len = GetWindowTextLength(hwnd) + 1 /*CH_NULL*/;
    STATUS status;
    PTSTR tstr;

    p_ui_text->type = UI_TEXT_TYPE_NONE;

    if(NULL != (tstr = al_array_alloc_TCHAR(&p_ui_text->text.array_handle_tstr, tstr_len, &array_init_block_tchar, &status)))
    {
        p_ui_text->type = UI_TEXT_TYPE_TSTR_ARRAY;

        GetWindowText(hwnd, tstr, (int) tstr_len);
    }

    return(status);
}

#endif /* OS */

_Check_return_
extern STATUS
ui_text_state_change(
    _InoutRef_  P_UI_TEXT p_ui_cur,
    _InRef_     PC_UI_TEXT p_ui_new,
    _OutRef_    P_BOOL p_changed)
{
    BOOL changed = (0 != ui_text_compare(p_ui_cur, p_ui_new, 0, 0));

    if(changed)
    {
        UI_TEXT ui_text;
        *p_changed = TRUE; /* in case we status_return() */
        status_return(ui_text_copy(&ui_text, p_ui_new));
        ui_text_dispose(p_ui_cur);
        *p_ui_cur = ui_text;
    }

    *p_changed = changed;
    return(STATUS_OK);
}

/* end of ob_dlg3.c */
