/* windows/ho_menu.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Menu handling for Windows version of Fireworkz */

/* Convert the menu structure from our own internal format
 * into something that can be handled by the host platform
 * we are running on.  And provide simple command execution.
 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#include "common/res_defs.h"

#include "ob_toolb/xp_toolb.h"

/*
internal functions
*/

static BOOL
execute_menu_command(
    _DocuRef_   P_DOCU p_docu,
    P_MENU_ROOT p_menu_root,
    HMENU hmenu,
    _InVal_     UINT command,
    _InVal_     BOOL slide_off);

static HMENU
make_popup_from_menu_root(
    P_MENU_ROOT p_menu_root);

_Check_return_
static STATUS
make_host_menu(
    _DocuRef_   P_DOCU p_docu,
    P_MENU_ROOT p_menu_root,
    _InVal_     BOOL popup,
    _In_        UINT * p_command,
    /*inout*/ HMENU * p_hmenu);

#define COMMAND_BASE_INDEX 0

static BOOL
disable[MENU_ACTIVE_STATE_COUNT];

typedef struct HO_MENU_DESC
{
    struct HO_MENU_DESC_RANGE
    {
        UINT start;
        UINT end;
    }
    command_range;

    HMENU hmenu;
}
HO_MENU_DESC, * P_HO_MENU_DESC; typedef const HO_MENU_DESC * PC_HO_MENU_DESC;

_Check_return_
extern STATUS
ho_menu_msg_close2(
    _DocuRef_   P_DOCU p_docu)
{
    ARRAY_INDEX i;

    for(i = 0; i < array_elements(&p_docu->ho_menu_data); ++i)
    {
        const P_HO_MENU_DESC p_ho_menu_desc = array_ptr(&p_docu->ho_menu_data, HO_MENU_DESC, i);

        if(NULL == p_ho_menu_desc->hmenu)
            continue;

        DestroyMenu(p_ho_menu_desc->hmenu);
    }

    al_array_dispose(&p_docu->ho_menu_data);

    return(STATUS_OK);
}

/* Internal function used to create the menu structure required for
 * a document.  This is called by host functions as the view
 * is created.  It therefore must ensure that the document doesn't
 * already have a menu structure associated with it.
 */

_Check_return_
extern STATUS
ho_create_docu_menus(
    _DocuRef_   P_DOCU p_docu)
{
    STATUS status = STATUS_OK;
    P_HO_MENU_DESC p_ho_menu_desc;
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(*p_ho_menu_desc), 1);
    const P_DOCU p_docu_config = p_docu_from_config_wr(); /* SKS 21jan95 */

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    if(p_docu_config->ho_menu_data)
        return(status);

    if(NULL == al_array_extend_by(&p_docu_config->ho_menu_data, HO_MENU_DESC, MENU_ROOT_MAX, &array_init_block, &status))
        return(status);

    {
    UINT command = IDM_START; /* SKS 20mar95 was 0 but that's confusingly low c.f. IDOK */
    const MENU_ROOT_ID i_max = (MENU_ROOT_ID) array_elements(&p_docu_config->ho_menu_data);
    MENU_ROOT_ID i;

    for(i = MENU_ROOT_ID_FIRST; i < i_max; ENUM_INCR(MENU_ROOT_ID, i))
    {
        P_MENU_ROOT p_menu_root;
        BOOL pop_up;

        if(MENU_ROOT_ICON_BAR == i)
        {   /* this is only for RISC OS */
            continue;
        }

        if(NULL == (p_menu_root = sk_menu_root(p_docu_config, i)))
            continue;

        pop_up = (i != MENU_ROOT_DOCU) && (i != MENU_ROOT_CHART_EDIT);

        p_ho_menu_desc = array_ptr(&p_docu_config->ho_menu_data, HO_MENU_DESC, i);
        p_ho_menu_desc->command_range.start = command;
        p_ho_menu_desc->hmenu = NULL;
        status = make_host_menu(p_docu_config, p_menu_root, pop_up, &command, &p_ho_menu_desc->hmenu);
        p_ho_menu_desc->command_range.end = command;
        status_break(status);
        command++;
    }
    } /*block*/

    return(status);
}

/* Given the index for a menu attempt to make a popup of it, this also
 * involves ensuring that there is a suitable menu already in existance
 * for the structure.
 */

_Check_return_
extern STATUS
ho_menu_popup(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InVal_     S32 menu_root)
{
    STATUS status = STATUS_FAIL;
    const PC_DOCU p_docu_config = p_docu_from_config();

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);

    if(array_index_is_valid(&p_docu_config->ho_menu_data, menu_root))
    {
        const PC_HO_MENU_DESC p_ho_menu_desc = array_ptrc(&p_docu_config->ho_menu_data, HO_MENU_DESC, menu_root);

        if(p_ho_menu_desc->hmenu)
        {
            UINT extra_flags = TPM_LEFTBUTTON; /* SKS 29feb95 - stop Mr Uninitialized! */
            POINT point;

            if(IsLButtonDown())
                extra_flags = TPM_LEFTBUTTON;
            if(IsRButtonDown())
                extra_flags = TPM_RIGHTBUTTON;

            GetCursorPos(&point);

            (void) TrackPopupMenu(p_ho_menu_desc->hmenu, TPM_LEFTALIGN | extra_flags, point.x, point.y, 0, p_view->main[WIN_BACK].hwnd, NULL);

            return(status);
        }
    }

    return(status);
}

/* Attach the menu bar to the specified view.  When this is called the
 * host copy of the bar may not exist yet, so we must attempt to
 * created, it.  This is quite a simple process of calling the
 * menu create function below and then putting the HMENU into the
 * view structure.
 */

extern HMENU
ho_get_menu_bar(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view)
{
    const PC_DOCU p_docu_config = p_docu_from_config();
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_ViewRef_(p_view);
    return(array_ptr(&p_docu_config->ho_menu_data, HO_MENU_DESC, MENU_ROOT_DOCU)->hmenu);
}

/* Given a document change its menu bar to a new one.  This allows
 * editing objects to change their menus to reflect their state.
 */

extern void
ho_prefer_menu_bar(
    _DocuRef_   P_DOCU p_docu_in,
    _InVal_     S32 menu_id)
{
    const PC_DOCU p_docu_config = p_docu_from_config();

    if(array_index_is_valid(&p_docu_config->ho_menu_data, menu_id))
    {
        const PC_HO_MENU_DESC p_ho_menu_desc = array_ptrc(&p_docu_config->ho_menu_data, HO_MENU_DESC, menu_id);

        if(p_ho_menu_desc->hmenu)
        {
            VIEWNO viewno = VIEWNO_NONE;
            P_VIEW p_view;
            while(P_VIEW_NONE != (p_view = docu_enum_views(p_docu_in, &viewno)))
                SetMenu(p_view->main[WIN_BACK].hwnd, p_ho_menu_desc->hmenu);
        }
    }
}

/* Build a menu structure given the menu root.  This is quite
 * important as not only do we convert the internal structure
 * of the tree to something that can be handled by the
 * host layers, but we also asign the action codes.
 */

_Check_return_
static STATUS
make_host_menu(
    _DocuRef_   P_DOCU p_docu,
    P_MENU_ROOT p_menu_root,
    _InVal_     BOOL popup,
    _In_        UINT * p_command,
    /*inout*/ HMENU * p_hmenu)
{
    ARRAY_INDEX entry_count = array_elements(&p_menu_root->h_entry_list);
    ARRAY_INDEX index;
    HMENU hmenu = *p_hmenu;
    STATUS status = STATUS_OK;

    if(0 == entry_count)
        return(STATUS_OK);

    if(NULL == hmenu)
    {
        if(popup)
            hmenu = CreatePopupMenu();
        else
            hmenu = CreateMenu();
    }

    if(NULL == hmenu)
        return(status_nomem());

    for(index = 0; index < entry_count; ++index)
    {
        UINT item_flags = MF_ENABLED;
        P_MENU_ENTRY p_menu_entry = array_ptr(&p_menu_root->h_entry_list, MENU_ENTRY, index);
        UINT command;
        TCHARZ buffer[64];
        PCTSTR text = NULL;

        if(p_menu_entry->sub_menu.h_entry_list)
        {
            HMENU hmenu_local = NULL; /* a local one */
            item_flags |= MF_POPUP;
            status_break(status = make_host_menu(p_docu, &p_menu_entry->sub_menu, TRUE, p_command, &hmenu_local));
            command = (UINT) (UINT_PTR) hmenu_local; /* store this HMENU in slideoff's entry */
        }
        else
            command = (*p_command += 1);

        if(NULL == p_menu_entry->tstr_entry_text)
            item_flags |= MF_SEPARATOR;
        else
        {
            PCTSTR tstr_text_lhs = p_menu_entry->tstr_entry_text;
            PCTSTR tstr_text_rhs = key_ui_name_from_key_code(p_menu_entry->key_code, 0 /*short*/);

            buffer[0] = CH_NULL;
            ui_string_to_text(p_docu, buffer, elemof32(buffer), tstr_text_lhs);

            if(tstr_text_rhs)
            {
                tstr_xstrkat(buffer, elemof32(buffer), TEXT("\t"));
                tstr_xstrkat(buffer, elemof32(buffer), tstr_text_rhs);
            }

            text = buffer;
            item_flags |= MF_STRING;
        }

        if(!AppendMenu(hmenu, item_flags, command, text))
        {
            status = status_nomem();
            break;
        }
    }

    if(status_fail(status))
    {
        DestroyMenu(hmenu);
        hmenu = NULL;
    }

    *p_hmenu = hmenu;
    return(status);
}

/* Scan the menu structure looking for the command to be executed.
 * This is a result of WM_COMMAND message.  We must decode the
 * command and perform a linear search of the tree to locate the item.
 */

/*ncr*/
extern BOOL
ho_execute_menu_command(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _In_        UINT command,
    _InVal_     BOOL slide_off)
{
    const PC_DOCU p_docu_config = p_docu_from_config();
    const MENU_ROOT_ID i_max = (MENU_ROOT_ID) array_elements(&p_docu_config->ho_menu_data);
    MENU_ROOT_ID i;

    for(i = MENU_ROOT_ID_FIRST; i < i_max; ENUM_INCR(MENU_ROOT_ID, i))
    {
        const PC_HO_MENU_DESC p_ho_menu_desc = array_ptr(&p_docu_config->ho_menu_data, HO_MENU_DESC, i);

        if(NULL == p_ho_menu_desc->hmenu)
            continue;

        if((command >= p_ho_menu_desc->command_range.start) && (command <= p_ho_menu_desc->command_range.end))
        {
            P_MENU_ROOT p_menu_root = sk_menu_root(p_docu_config, i);
            PTR_ASSERT(p_menu_root);
            return(execute_menu_command(p_docu, p_menu_root, p_ho_menu_desc->hmenu, command, slide_off));
        }
    }

    UNREFERENCED_PARAMETER_ViewRef_(p_view);
    return(FALSE);
}

/* Having pre-scanned the menu looking for the command to execute,
 * now traverse the desired menu looking for a suitable command
 * to be executed and then executing that function.
 */

static BOOL
execute_menu_command(
    _DocuRef_   P_DOCU p_docu,
    P_MENU_ROOT p_menu_root,
    HMENU hmenu,
    _InVal_     UINT command,
    _InVal_     BOOL slide_off)
{
    ARRAY_INDEX index = array_elements(&p_menu_root->h_entry_list);

    while(--index >= 0)
    {
        P_MENU_ENTRY p_menu_entry = array_ptr(&p_menu_root->h_entry_list, MENU_ENTRY, index);

        if(p_menu_entry->sub_menu.h_entry_list)
        {
            if(execute_menu_command(p_docu, &p_menu_entry->sub_menu, GetSubMenu(hmenu, (int) index), command, slide_off))
                return(TRUE);

            continue;
        }

        if(GetMenuItemID(hmenu, (int) index) == command)
        {
            ARRAY_HANDLE h_command = p_menu_entry->h_command;

            if(slide_off)
                h_command = p_menu_entry->h_command2;

            if(h_command)
                status_consume(command_array_handle_execute(docno_from_p_docu(p_docu), h_command)); /* error already reported */

            return(TRUE);
        }
    }

    return(FALSE);
}

/* Discard host menu structure */

extern void
ho_menu_dispose(void)
{
    const P_DOCU p_docu_config = p_docu_from_config_wr();
    ARRAY_INDEX i;

    for(i = 0; i < array_elements(&p_docu_config->ho_menu_data); ++i)
    {
        const P_HO_MENU_DESC p_ho_menu_desc = array_ptr(&p_docu_config->ho_menu_data, HO_MENU_DESC, i);

        if(NULL == p_ho_menu_desc->hmenu)
            continue;

        DestroyMenu(p_ho_menu_desc->hmenu); /* it is recursive */
        p_ho_menu_desc->hmenu = NULL;
    }

    al_array_dispose(&p_docu_config->ho_menu_data);
}

/* Enquire with the application to see which menu items should be
 * enabled and disabled.  Stored with each item is an index into the
 * disable table, this flags which options should be allowed and
 * not and is generated in this function and used by the enable
 * disable calls.
 */

static void
host_wm_initmenu_recurse(
    _DocuRef_maybenone_ PC_DOCU p_docu,
    P_MENU_ROOT p_menu_root,
    HMENU hmenu)
{
    ARRAY_INDEX index = array_elements(&p_menu_root->h_entry_list);

    while(--index >= 0)
    {
        P_MENU_ENTRY p_menu_entry = array_ptr(&p_menu_root->h_entry_list, MENU_ENTRY, index);
        PC_U8Z p_command = array_basec(&p_menu_entry->h_command, U8Z);
        UINT state = disable[p_menu_entry->flags.active_state] ? (MF_DISABLED | MF_GRAYED) : MF_ENABLED;

        if(P_DATA_NONE != p_command)
        {
            const PC_U8Z p_colon = strchr(p_command, CH_COLON);
            if(NULL != p_colon)
            {
                const U32 n_bytes_command = PtrDiffBytesU32(p_colon, p_command) + 1;
                const U32 n_bytes_test = sizeof32("Button:")-1;
                if( (n_bytes_command == n_bytes_test) && (0 == memcmp("Button:", p_command, n_bytes_test)) ) /* SKS eliminate strncmp() for Pentium build */
                {
                    const PC_U8Z p_button_name = PtrAddBytes(PC_USTR, p_command, n_bytes_command);
                    T5_TOOLBAR_TOOL_ENABLE_QUERY t5_toolbar_tool_enable_query;
                    t5_toolbar_tool_enable_query.name = p_button_name;
                    status_assert(object_call_id(OBJECT_ID_TOOLBAR, de_const_cast(P_DOCU, p_docu), T5_MSG_TOOLBAR_TOOL_ENABLE_QUERY, &t5_toolbar_tool_enable_query));
                    state = t5_toolbar_tool_enable_query.enabled ? MF_ENABLED : (MF_DISABLED | MF_GRAYED);
                }
            }
        }

        EnableMenuItem(hmenu, GetMenuItemID(hmenu, (int) index), state);

        if(p_menu_entry->sub_menu.h_entry_list)
            host_wm_initmenu_recurse(p_docu, &p_menu_entry->sub_menu, GetSubMenu(hmenu, (int) index));
    }
}

static void
host_wm_initmenu_for_docu(
    _DocuRef_   P_DOCU p_docu)
{
    MENU_OTHER_INFO menu_other_info;
    BOOL paste_valid;

    zero_struct(menu_other_info);

    /* ask our friends out there to fillin as much of this structure as they want */
    status_assert(object_call_all(p_docu, T5_MSG_MENU_OTHER_INFO, &menu_other_info));

    /* can only paste to certain target objects */
    paste_valid = ((OBJECT_ID_CELLS == p_docu->focus_owner) || (OBJECT_ID_NOTE == p_docu->focus_owner));
    if(paste_valid && (0 == local_clip_data_query()))
    {   /* accept lots of different kinds of data, so if there's any global clip data, say OK */
#if defined(USE_GLOBAL_CLIPBOARD)
        paste_valid = (0 != CountClipboardFormats());
#else
        paste_valid = FALSE;
#endif
    }

    /* now setup the disable array so that items can be enabled and disabled */
    disable[MENU_PASTE] = (!paste_valid);

    disable[MENU_SELECTION]       = (!menu_other_info.cells_selection) && (menu_other_info.note_selection_count == 0);
    disable[MENU_CELLS_SELECTION] = (!menu_other_info.cells_selection);

    disable[MENU_PICTURES]            = (menu_other_info.note_picture_count == 0);
    disable[MENU_PICTURE_SELECTION]   = (menu_other_info.note_selection_count == 0);
    disable[MENU_PICTURE_OR_BACKDROP] = (menu_other_info.note_selection_count != 1) && (menu_other_info.note_behind_count != 1);

    disable[MENU_CARET_IN_CELLS]      = (OBJECT_ID_CELLS != p_docu->focus_owner);
    disable[MENU_CARET_IN_TEXT]       = (OBJECT_ID_CELLS != p_docu->focus_owner) && (OBJECT_ID_HEADER != p_docu->focus_owner) && (OBJECT_ID_FOOTER != p_docu->focus_owner);
}

extern void
host_onInitMenu(
    _InRef_     HMENU hmenu,
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view)
{
    const PC_DOCU p_docu_config = p_docu_from_config();
    const MENU_ROOT_ID i_max = (MENU_ROOT_ID) array_elements(&p_docu_config->ho_menu_data);
    MENU_ROOT_ID i;

    UNREFERENCED_PARAMETER_ViewRef_(p_view);

    zero_array(disable);

    for(i = MENU_ROOT_ID_FIRST; i < i_max; ENUM_INCR(MENU_ROOT_ID, i))
    {
        const P_HO_MENU_DESC p_ho_menu_desc = array_ptr(&p_docu_config->ho_menu_data, HO_MENU_DESC, i);

        if(p_ho_menu_desc->hmenu == hmenu)
        {
            const P_MENU_ROOT p_menu_root = sk_menu_root(p_docu_config, i);

            if(!IS_DOCU_NONE(p_docu))
                host_wm_initmenu_for_docu(p_docu);

            DOCU_ASSERT(p_docu); /* currently all callers DO pass valid DOCU */
            __analysis_assume(p_docu);
            host_wm_initmenu_recurse(p_docu, p_menu_root, hmenu);

            break;
        }
    }
}

/* end of windows/ho_menu.c */
