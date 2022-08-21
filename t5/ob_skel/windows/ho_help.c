/* windows/ho_help.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1994-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Host specific help file handling */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#include <htmlhelp.h>

#pragma comment(lib, "htmlhelp.lib")

/* Display the specified contents page from the help file specified.
 * The help file specified is search for in the path sequences and
 * is then opened local to the specified window
*/

_Check_return_
static STATUS
ho_help_contents(
    _InRef_opt_ HWND hwnd,
    _In_z_      PCTSTR filename_in)
{
    TCHARZ filename[BUF_MAX_PATHSTRING];

    if(file_find_on_path(filename, elemof32(filename), file_get_resources_path(), filename_in) <= 0)
        return(create_error(ERR_HELP_FAILURE));

    if(!WrapOsBoolChecking(NULL != HtmlHelp(hwnd, filename, HH_DISPLAY_TOC, 0)))
        return(create_error(ERR_HELP_FAILURE));

    return(STATUS_OK);
}

/* Open the help file (search for on the path) and display
 * the search results for the specified keyword.
 * If more than one result (or no result) is located then
 * a list box is displayed showing the various options.
*/

_Check_return_
static STATUS
ho_help_search_keyword(
    _InRef_opt_ HWND hwnd,
    _In_z_      PCTSTR filename_in,
    _In_z_      PCTSTR keyword)
{
    TCHARZ filename[BUF_MAX_PATHSTRING];

    if(file_find_on_path(filename, elemof32(filename), file_get_resources_path(), filename_in) <= 0)
        return(create_error(ERR_HELP_FAILURE));

    if((NULL == keyword) || (CH_NULL == *keyword))
    {
        HH_FTS_QUERY query;
        zero_struct(query);
        query.cbStruct = sizeof(HH_FTS_QUERY);

        if(!WrapOsBoolChecking(NULL != HtmlHelp(hwnd, filename, HH_DISPLAY_SEARCH, (DWORD_PTR) &query)))
            return(create_error(ERR_HELP_FAILURE));

        return(STATUS_OK);
    }

    if(!WrapOsBoolChecking(NULL != HtmlHelp(hwnd, filename, HH_DISPLAY_TOPIC, 0)))
        return(create_error(ERR_HELP_FAILURE));

    {
    HH_AKLINK link = { sizeof(HH_AKLINK) };
    /*zero_struct(link);*/
    link.pszKeywords = keyword;
    link.fIndexOnFail = TRUE;

    if(!WrapOsBoolChecking(NULL != HtmlHelp(hwnd, filename, HH_KEYWORD_LOOKUP, (DWORD_PTR) &link)))
        return(create_error(ERR_HELP_FAILURE));
    } /*block*/

    return(STATUS_OK);
}

_Check_return_
static STATUS
ho_help_url(
    _InRef_opt_ HWND hwnd,
    _In_z_      PCTSTR url)
{
    SHELLEXECUTEINFO sei;

    zero_struct(sei);
    sei.cbSize = sizeof32(sei);
    sei.hwnd = hwnd;
    sei.lpVerb = TEXT("Open");
    sei.lpFile = url;
    sei.nShow = SW_SHOW;

    if(!WrapOsBoolChecking(ShellExecuteEx(&sei)))
        return(create_error(ERR_HELP_URL_FAILURE));

    return(STATUS_OK);
}

T5_CMD_PROTO(extern, t5_cmd_help)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 1); /* NB just the common span */
    const P_VIEW p_view = p_view_from_viewno_caret(p_docu);
    const HOST_WND hwnd = !IS_VIEW_NONE(p_view) ? p_view->main[WIN_BACK].hwnd : HOST_WND_NONE;

    switch(t5_message)
    {
    default: default_unhandled();
    case T5_CMD_HELP_CONTENTS:
        return(ho_help_contents(hwnd, p_args[0].val.tstr));

    case T5_CMD_HELP_SEARCH_KEYWORD:
        return(ho_help_search_keyword(hwnd, p_args[0].val.tstr, pc_arglist_arg(&p_t5_cmd->arglist_handle, 1)->val.tstr));

    case T5_CMD_HELP_URL:
        return(ho_help_url(hwnd, p_args[0].val.tstr));
    }
}

/* end of windows/ho_help.c */
