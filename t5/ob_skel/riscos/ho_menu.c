/* riscos/ho_menu.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* RISC OS specific menu routines for Fireworkz */

/* RCM May 1992 */

#include "common/gflags.h"

#if RISCOS

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#include "ob_skel/xp_skelr.h"

#include "ob_toolb/xp_toolb.h"

/*
internal routines
*/

static U32
menu_size(
    P_MENU_ROOT p_menu_root,
    _In_z_      PC_SBSTR menu_title);

static WimpMenu *
menu_build(
    _DocuRef_   P_DOCU p_docu,
    P_MENU_ROOT p_menu_root,
    _In_z_      PC_SBSTR menu_title);

/******************************************************************************
*
* The machine independent menu structure maintained by sk_menu is
* converted into its RISC OS equivalent by menu_build().
*
* It writes its structures into an al_ptr_alloc'ed block,
* menus are placed at the start, working up,
* indirected entry strings placed at the end, working down.
*
******************************************************************************/

static
struct MENU_BUILD_AREA
{
    P_U8 area_start;
    U32 area_size;

    P_U8 next_menu;     /* place next menu here, then inc this */
    P_U8 last_string;   /* last string starts here, so dec, then place new string */
}
menu_build_area;

static BOOL disable[MENU_ACTIVE_STATE_COUNT];

_Check_return_
_Ret_valid_
static P_MENU_ROOT
p_menu_root_from_si_handle(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_U16 p_si_handle)
{
    MENU_ROOT_ID menu_root_id = (MENU_ROOT_ID) *p_si_handle;

    switch(menu_root_id)
    {
    default:
        {
        NOTE_MENU_QUERY note_menu_query;
        note_menu_query.menu_root_id = MENU_ROOT_DOCU;
        status_consume(object_call_id(OBJECT_ID_NOTE, p_docu, T5_MSG_NOTE_MENU_QUERY, &note_menu_query));
        menu_root_id = note_menu_query.menu_root_id;
        } /*block*/

        if(MENU_ROOT_DOCU == menu_root_id)
        {
            P_MENU_ROOT p_menu_root = p_docu->p_menu_root;

            if(NULL != p_menu_root)
                return(p_menu_root);

            p_menu_root = p_docu_from_config()->p_menu_root;
            PTR_ASSERT(p_menu_root);
            return(p_menu_root);
        }

        /*FALLTHRU*/

    case MENU_ROOT_ICON:
    case MENU_ROOT_FUNC:
    case MENU_ROOT_CHART:
        return(sk_menu_root(p_docu_from_config_wr(), menu_root_id));
    }
}

static void
ho_menu_init_for_docu(
    _DocuRef_   P_DOCU p_docu)
{
    MENU_OTHER_INFO menu_other_info;
    BOOL paste_valid;

    zero_struct(menu_other_info);

    /* ask our friends out there to fillin as much of this structure as they want */
    status_assert(object_call_all(p_docu, T5_MSG_MENU_OTHER_INFO, &menu_other_info));

    /* can only paste to certain target objects */
    paste_valid = ((OBJECT_ID_CELLS == p_docu->focus_owner) || (OBJECT_ID_NOTE == p_docu->focus_owner));
    if(paste_valid)
        paste_valid = (local_clip_data_query() != 0);

    /* now setup the disable array so that items can be enabled and disabled */
    disable[MENU_PASTE] = !paste_valid;

    disable[MENU_SELECTION]       = (!menu_other_info.cells_selection) && (menu_other_info.note_selection_count == 0);
    disable[MENU_CELLS_SELECTION] = (!menu_other_info.cells_selection);

    disable[MENU_PICTURES]            = (menu_other_info.note_picture_count   == 0);
    disable[MENU_PICTURE_SELECTION]   = (menu_other_info.note_selection_count == 0);
    disable[MENU_PICTURE_OR_BACKDROP] = (menu_other_info.note_selection_count != 1) && (menu_other_info.note_behind_count != 1);

    disable[MENU_CARET_IN_CELLS] = (OBJECT_ID_CELLS != p_docu->focus_owner);
    disable[MENU_CARET_IN_TEXT]  = (OBJECT_ID_CELLS != p_docu->focus_owner) && (OBJECT_ID_HEADER != p_docu->focus_owner) && (OBJECT_ID_FOOTER != p_docu->focus_owner);

    disable[MENU_DATABASE] = !menu_other_info.focus_in_database;

    /* for Database->Create: disallow creates inside an existing database and if not in cells area */
    if(menu_other_info.focus_in_database)
        disable[MENU_CARET_IN_CELLS_NOT_DATABASE] = TRUE;
    else
        disable[MENU_CARET_IN_CELLS_NOT_DATABASE] = disable[MENU_CARET_IN_CELLS];

    /* disallow inserts into the text in databases */
    if(menu_other_info.focus_in_database || menu_other_info.focus_in_database_template)
        disable[MENU_CARET_IN_TEXT_NOT_DATABASE]  = TRUE;
    else
        disable[MENU_CARET_IN_TEXT_NOT_DATABASE]  = disable[MENU_CARET_IN_TEXT];

    disable[MENU_DATABASE_TEMPLATE] = !menu_other_info.focus_in_database_template;
}

_Check_return_
static abstract_menu_handle
ho_menu_event_maker_core(
    _DocuRef_   P_DOCU p_docu,
    P_MENU_ROOT p_menu_root)
{
    PC_U8Z p_menu_title;

    zero_array(disable);

    disable[MENU_ACTIVE_NEVER] = TRUE;

    if(!IS_DOCU_NONE(p_docu))
    {
        ho_menu_init_for_docu(p_docu);
    }

    if(!event_is_menu_being_recreated()) /* SKS 09apr93 - only blow menu away as needed */
        ho_menu_dispose();

    p_menu_title = p_menu_root->name[0] ? p_menu_root->name : product_id();

    if(!menu_build_area.area_start)
    {
        /* scan document menu tree to determine size of RISC OS menu equivalent */
        STATUS status;

        menu_build_area.area_size = menu_size(p_menu_root, p_menu_title);

        menu_build_area.area_start = al_ptr_alloc_bytes(P_U8, menu_build_area.area_size, &status); /* then allocate space */
        status_assert(status);

        if(!menu_build_area.area_start)
            return(NULL);
    }
    else
        assert(menu_build_area.area_size == menu_size(p_menu_root, p_menu_title)); /* but make sure we reuse the same stuff */

    menu_build_area.next_menu   = menu_build_area.area_start;
    menu_build_area.last_string = menu_build_area.area_start + menu_build_area.area_size;

    memset32(menu_build_area.area_start, 0, menu_build_area.area_size); /* clear out each time prior to build */

    /* and construct the RISC OS menu structure */
    return(event_return_real_menu(menu_build(p_docu, p_menu_root, p_menu_title)));
}

_Check_return_
extern abstract_menu_handle
ho_menu_event_maker(
    void * handle)
{
    U16 si_handle;
    P_VIEW p_view;
    const P_DOCU p_docu = viewid_unpack((U32) handle, &p_view, &si_handle);

    if(!IS_DOCU_NONE(p_docu) && !IS_VIEW_NONE(p_view))
        command_set_current_view(p_docu, p_view);

    return(ho_menu_event_maker_core(p_docu, p_menu_root_from_si_handle(p_docu, &si_handle)));
}

static BOOL
ho_menu_event_core(
    _DocuRef_   P_DOCU p_docu,
    P_MENU_ROOT p_menu_root,
    /*_In_z_*/  char * hit,
    _InVal_     BOOL submenurequest)
{
    P_MENU_ENTRY p_menu_entry = NULL;
    ARRAY_INDEX index;

    while((index = *hit++) != 0)
    {
        if(--index < array_elements(&p_menu_root->h_entry_list))
            p_menu_entry = array_ptr(&p_menu_root->h_entry_list, MENU_ENTRY, index);
        else
        {
            p_menu_entry = NULL; /* wimp's view and our view of menu are inconsistent! */
            assert0();
            break;
        }

        p_menu_root = &p_menu_entry->sub_menu; /* branch to follow iff *hit != 0 */
    }

    if(NULL != p_menu_entry)
    {   /* found a leaf (probably) or non-leaf with a command, so execute it */
        ARRAY_HANDLE h_command = p_menu_entry->h_command;

        if(submenurequest || !h_command)
            h_command = p_menu_entry->h_command2;

        if(h_command)
            status_consume(command_array_handle_execute(docno_from_p_docu(p_docu), h_command)); /* error already reported */
    }

    return(TRUE);
}

_Check_return_
extern BOOL
ho_menu_event_proc(
    void * handle,
    char * hit,
    _InVal_     BOOL submenurequest)
{
    U16 si_handle;
    P_VIEW p_view;
    const P_DOCU p_docu = viewid_unpack((U32) handle, &p_view, &si_handle);

    if(!IS_DOCU_NONE(p_docu) && !IS_VIEW_NONE(p_view))
        command_set_current_view(p_docu, p_view);

    return(ho_menu_event_core(p_docu, p_menu_root_from_si_handle(p_docu, &si_handle), hit, submenurequest));
}

/******************************************************************************
*
* Scan the menu structure and return the size of
* the equivalent RISC OS wimp menu structure.
*
******************************************************************************/

static U32
menu_size(
    P_MENU_ROOT p_menu_root,
    _In_z_      PC_SBSTR p_menu_title)
{
    U32 max_entry_len = strlen(p_menu_title);
    U32 padded_entry_count = 0;
    U32 size = sizeof32(WimpMenu) - sizeof32(WimpMenuItem); /* allocate space for menu header */ /* NB there is always one item declared */
    ARRAY_INDEX index;

    if(!array_elements(&p_menu_root->h_entry_list))
        return(0);

    assert(max_entry_len <= 12); /* see menu_build */

    for(index = 0; index < array_elements(&p_menu_root->h_entry_list); index++)
    {
        P_MENU_ENTRY p_menu_entry = array_ptr(&p_menu_root->h_entry_list, MENU_ENTRY, index);
        PC_SBSTR text_lhs, text_rhs;
        U32 text_lhs_len, text_rhs_len, entry_len;

        if(NULL == p_menu_entry->tstr_entry_text)
            continue; /* ignore separator entries */

        text_lhs = p_menu_entry->tstr_entry_text;
        text_rhs = key_ui_name_from_key_code(p_menu_entry->key_code, 0 /*short*/);
        text_lhs_len = strlen32(text_lhs);
        text_rhs_len = text_rhs ? strlen32(text_rhs) : 0;

        size += sizeof32(WimpMenuItem); /* size of one entry. if text needs indirecting, size increased later on */

        if(strchr(text_lhs, CH_AMPERSAND))
            --text_lhs_len;

        entry_len = text_lhs_len;

        if(text_rhs)
        {
            /* we will show the key short cut in the menu, as "<menu text><1 or more spaces><key text>" */
            /* spaces are added between the two text pieces, so that the key text is right justified    */

            entry_len += 1 /*space*/ + text_rhs_len;

            padded_entry_count++; /* count number of padded entries */
        }
        else
        {
            /* no key associated with command, so we will show just the menu text */

            if(entry_len > 12)
                size += entry_len + 1 /*term*/; /* must be indirected, so allocate extra space */
        }

        max_entry_len = MAX(max_entry_len, entry_len);

        if(p_menu_entry->sub_menu.h_entry_list)
            size += menu_size(&p_menu_entry->sub_menu, text_lhs); /* size-up this entry's submenu */
    }

    /* if padded entries need indirecting, allocate extra space */
    if(max_entry_len > 12)
        size += padded_entry_count * (max_entry_len + 1 /*term*/);

    return(size);
}

/******************************************************************************
*
* Scan the menu structure and build an equivalent
* RISC OS wimp menu structure in menu_build_area.
*
******************************************************************************/

static BOOL
menu_build_disable_query(
    _DocuRef_   P_DOCU p_docu,
    P_MENU_ENTRY p_menu_entry)
{
    PC_U8Z p_command = array_basec(&p_menu_entry->h_command, U8Z);
    BOOL disabled = disable[p_menu_entry->flags.active_state];

    if(P_DATA_NONE != p_command)
    {
        const U32 n_bytes = sizeof32("Button:")-1;
        if(0 == /*"C"*/strncmp("Button:", p_command, n_bytes))
        {
            T5_TOOLBAR_TOOL_ENABLE_QUERY t5_toolbar_tool_enable_query;
            t5_toolbar_tool_enable_query.name = p_command + n_bytes;
            status_consume(object_call_id(OBJECT_ID_TOOLBAR, p_docu, T5_MSG_TOOLBAR_TOOL_ENABLE_QUERY, &t5_toolbar_tool_enable_query));
            disabled = !t5_toolbar_tool_enable_query.enabled;
        }
    }

    return(disabled);
}

static WimpMenu *
menu_build(
    _DocuRef_   P_DOCU p_docu,
    P_MENU_ROOT p_menu_root,
    _In_z_      PC_SBSTR p_menu_title)
{
    U32 max_entry_len;
    WimpMenu * p_wimp_menuhdr;
    WimpMenuItemWithBitset * p_wimp_menuentry;
    int pass;

    if(!array_elements(&p_menu_root->h_entry_list))
        return((WimpMenu *) -1);

    /* must allocate the menu header and body of 'entry_count' menu entries in one lump */
    p_wimp_menuhdr = (WimpMenu *) menu_build_area.next_menu;
    menu_build_area.next_menu += sizeof32(WimpMenu) - sizeof32(WimpMenuItem); /* NB there is always one item declared */

    /* menu colours, entry heights and gap widths - copied from cwimp.c.menu */
    p_wimp_menuhdr->title_fg    = 7;  /* title fore: black */
    p_wimp_menuhdr->title_bg    = 2;  /* title back: grey */
    p_wimp_menuhdr->work_fg     = 7;  /* entries fore */
    p_wimp_menuhdr->work_bg     = 0;  /* entries back */
    p_wimp_menuhdr->item_width  = 0;  /* now ignored */
    p_wimp_menuhdr->item_height = 44; /* OS units per entry */
    p_wimp_menuhdr->gap         = 0;  /* gap between entries, in OS units */

    p_wimp_menuentry = (WimpMenuItemWithBitset *) menu_build_area.next_menu;
    menu_build_area.next_menu += sizeof32(WimpMenuItem) * array_elements(&p_menu_root->h_entry_list);

    {
    BOOL had_escape = 0;
    int src = 0;
    int dst = 0;

    for(;;)
    {
        U8 ch = p_menu_title[src++];

        if(CH_AMPERSAND == ch) /* omit first & found */
            if(!had_escape)
            {
                had_escape = 1;
                continue;
            }

        p_wimp_menuhdr->title[dst++] = ch;

        if(0 == ch)
            break;

        if(dst >= 12) /* title clipped at 12 chars (can't be indirected on RISC OS 2) */
            break;
    }

    max_entry_len = dst;

    p_wimp_menuhdr->item_width = dst * 16; /* minimum value - 16 OS units per char */
    } /*block*/

    /* Menu entries with key short cuts are shown as "<menu text><1 or more spaces><key text>"   */
    /* spaces are added between the two text pieces, so that the key text is right justified     */
    /* first pass scans through all entries, to find the max_entry_len, so we can pad as needed. */

    for(pass = 1; pass <= 2; ++pass)
    {
        ARRAY_INDEX index;

        for(index = 0; index < array_elements(&p_menu_root->h_entry_list); ++index)
        {
            P_MENU_ENTRY p_menu_entry = array_ptr(&p_menu_root->h_entry_list, MENU_ENTRY, index);
            PCTSTR text_lhs, text_rhs;
            U32 text_lhs_len, text_rhs_len, entry_len;

            if(NULL == p_menu_entry->tstr_entry_text)
                continue; /* ignore separator entries */

            text_lhs = p_menu_entry->tstr_entry_text;
            text_rhs = key_ui_name_from_key_code(p_menu_entry->key_code, 0 /*short*/);
            text_lhs_len = tstrlen32(text_lhs);
            text_rhs_len = text_rhs ? tstrlen32(text_rhs) : 0;

            if(tstrchr(text_lhs, CH_AMPERSAND))
                --text_lhs_len;

            if(pass == 1)
            {
                entry_len = text_lhs_len;

                if(text_rhs)
                    entry_len += 1/*space*/ + text_rhs_len;

                max_entry_len = MAX(max_entry_len, entry_len);
            }
            else /*if(pass == 2)*/
            {
                U32 menu_text_len = text_rhs ? max_entry_len : text_lhs_len;
                int menu_text_wid = 16 + (menu_text_len * 16); /* in OS units, 16 per char */
                P_U8 p_dst;

                /* entry may need a dotted line to separate it from the entry below */
                if(array_index_valid(&p_menu_root->h_entry_list, index + 1))
                    if(NULL == array_ptr(&p_menu_root->h_entry_list, MENU_ENTRY, index + 1)->tstr_entry_text)
                        p_wimp_menuentry->flags |= WimpMenuItem_DottedLine;

                /* entry may need ticking for various reasons */
#if 0
                if(0)
                    p_wimp_menuentry->flags |= WimpMenuItem_Ticked;
#endif

                if(p_menu_entry->sub_menu.h_entry_list)
                    p_wimp_menuentry->submenu.m = menu_build(p_docu, &p_menu_entry->sub_menu, text_lhs);
                else
                    p_wimp_menuentry->submenu.i = -1;

                if(p_menu_entry->h_command2)
                {
                    p_wimp_menuentry->flags |= WimpMenuItem_GenerateSubWimpMenuWarning;
                    p_wimp_menuentry->submenu.i = 1;
                }

                /* each entry is a text icon (flags already set to zero) */
                p_wimp_menuentry->icon_flags.bits.text        = 1;
                p_wimp_menuentry->icon_flags.bits.vert_centre = 1;
                p_wimp_menuentry->icon_flags.bits.filled      = 1;
                p_wimp_menuentry->icon_flags.bits.fg_colour   = 7;

                /* entry may need to be disabled (greyed out) for various reasons */
                p_wimp_menuentry->icon_flags.bits.disabled = menu_build_disable_query(p_docu, p_menu_entry);

                p_wimp_menuhdr->item_width = MAX(p_wimp_menuhdr->item_width, menu_text_wid);

                if(menu_text_len > 12)
                {
                    P_U8 indirected_string;

                    menu_build_area.last_string -= (menu_text_len + 1 /*term*/); /* claim space for indirected text */

                    indirected_string = menu_build_area.last_string;

                    p_wimp_menuentry->icon_flags.bits.indirect = 1;
                    p_wimp_menuentry->icon_data.it.buffer = indirected_string;
                    p_wimp_menuentry->icon_data.it.validation = (char *) -1;
                    p_wimp_menuentry->icon_data.it.buffer_size = 100;

                    p_dst = indirected_string;
                }
                else
                    p_dst = p_wimp_menuentry->icon_data.t;

                {
                PC_U8Z p_src = text_lhs;
                BOOL had_escape = 0;
                U32 dst = 0;
                while(dst < text_lhs_len) /* entry text placed at lhs */
                {
                    U8 ch = *p_src++;
                    if(CH_AMPERSAND == ch) /* omit first & found */
                        if(!had_escape)
                        {
                            had_escape = 1;
                            continue;
                        }
                    *p_dst++ = ch;
                    ++dst;
                }
                } /*block*/

                if(text_rhs)
                {
                    U32 spaces_len = menu_text_len - text_lhs_len - text_rhs_len;
                    U32 j;

                    for(j = 0; j < spaces_len; ++j) /* if entry has a key short cut add spaces */
                        *p_dst++ = CH_SPACE;

                    for(j = 0; j < text_rhs_len; ++j) /* then show the key text on the rhs */
                        *p_dst++ = text_rhs[j];
                }

                if(menu_text_len != 12) /* only case without termination is when icon fills exactly */
                    *p_dst++ = CH_NULL;

                if((index + 1) == array_elements(&p_menu_root->h_entry_list)) /* mark last entry in submenu */
                    p_wimp_menuentry->flags |= WimpMenuItem_Last;

                ++p_wimp_menuentry;
            }
        }
    }

    return(p_wimp_menuhdr);
}

extern void
ho_menu_dispose(void)
{
    menu_build_area.area_size = 0;

    al_ptr_dispose(P_P_ANY_PEDANTIC(&menu_build_area.area_start));
}

_Check_return_
extern STATUS
ho_menu_popup(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view,
    _InVal_     S32 root_id)
{
    event_popup_menu(ho_menu_event_maker, ho_menu_event_proc, (P_ANY) (uintptr_t) viewid_pack(p_docu, p_view, (U16) root_id));

    return(STATUS_OK);
}

#endif /* RISCOS */

/* end of riscos/ho_menu.c */
