/* ob_file.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* File service module for Fireworkz */

/* SKS April 1992 */

#include "common/gflags.h"

#include "ob_file/ob_file.h"

#if WINDOWS
#include "commdlg.h"

#include "cderr.h"

__pragma(warning(push))
__pragma(warning(disable:4917)) /* a GUID can only be associated with a class, interface or namespace */
__pragma(warning(disable:4820)) /* padding added after data member */
__pragma(warning(disable:4668)) /* not defined as a preprocessor macro */
__pragma(warning(disable:4115)) /* named type definition in parentheses */
__pragma(warning(disable:4201)) /* nonstandard extension used : nameless struct/union */
__pragma(warning(disable:4255)) /* no function prototype given: converting '()' to '(void) (Windows SDK 6.0A) */
//#define _WIN32_IE 0x0200        /* don't use new features requiring IE Web Desktop */
#include "shlobj.h"
__pragma(warning(pop))

#endif /* WINDOWS */

#if RISCOS
#define MSG_WEAK &rb_file_msg_weak
extern PC_U8 rb_file_msg_weak;
#endif

#define P_BOUND_RESOURCES_OBJECT_ID_FILE DONT_LOAD_RESOURCES

/*
internal routines
*/

static CONSTRUCT_TABLE
object_construct_table[] =
{                                                                                                   /*   fi ti mi ur up xi md mf nn cp sm ba fo */

    { "NewDocument",            NULL,                       T5_CMD_NEW_DOCUMENT },
#if WINDOWS
    { "OpenDocument",           NULL,                       T5_CMD_OPEN_DOCUMENT },
    { "CloseDocumentReq",       NULL,                       T5_CMD_CLOSE_DOCUMENT_REQ },

    { "InsertFile",             NULL,                       T5_MSG_INSERT_FILE_WINDOWS },
    { "InsertPicture",          NULL,                       T5_MSG_INSERT_FILE_WINDOWS_PICTURE },
#endif /* WINDOWS */

    { NULL,                     NULL,                       T5_EVENT_NONE } /* end of table */
};

static struct OB_FILE_STATICS
{
    TCHARZ error_buffer[256]; /* primarily for RISC OS errors (so must be >= 252) */

#if WINDOWS
    OPENFILENAME openfilename;
    DWORD nFilterIndex[2];
    TCHARZ szDirName[BUF_MAX_PATHSTRING];
    TCHARZ szCustomFilter[64]; /* space for GetOpenFileName to stash custom filespec */
#endif /* WINDOWS */
}
ob_file_statics;

#if WINDOWS

_Check_return_
extern STATUS /*n_filters*/
windows_filter_list_create(
    _InoutRef_  P_QUICK_TBLOCK p_filter_quick_tblock,
    _InVal_     S32 filter_mask,
    _InRef_     PC_ARRAY_HANDLE p_h_save_filetype /* SAVE_FILETYPE[] */)
{
    /* build filters as description,CH_NULL,wildcard spec,CH_NULL sets */
    STATUS status = STATUS_OK;
    const BOOL fEnumerating = (0 == array_elements32(p_h_save_filetype));
    ARRAY_INDEX array_index = fEnumerating ? ENUMERATE_BOUND_FILETYPES_START : 0;
    S32 n_filters = 0;

    for(;;)
    {
        T5_FILETYPE t5_filetype;
        BOOL fFound_description, fFound_extension;
        PC_USTR ustr_description;
        PC_USTR ustr_extension_srch;

        if(fEnumerating)
        {
            t5_filetype = enumerate_bound_filetypes(&array_index, filter_mask);

            if(ENUMERATE_BOUND_FILETYPES_START == array_index)
                break;
        }
        else
        {
            PC_SAVE_FILETYPE p_save_filetype;

            if(!array_index_is_valid(p_h_save_filetype, array_index))
                break;

            p_save_filetype = array_ptrc(p_h_save_filetype, SAVE_FILETYPE, array_index);

            t5_filetype = p_save_filetype->t5_filetype;

            ++array_index;
        }

        ustr_description = description_ustr_from_t5_filetype(t5_filetype, &fFound_description);
        ustr_extension_srch = extension_srch_ustr_from_t5_filetype(t5_filetype, &fFound_extension);

        status_break(status = quick_tblock_ustr_add_n(p_filter_quick_tblock, ustr_description, strlen_with_NULLCH));
        status_break(status = quick_tblock_ustr_add_n(p_filter_quick_tblock, ustr_extension_srch, strlen_with_NULLCH));

        ++n_filters;
    }

    /* list ends with one final CH_NULL */
    if(status_ok(status))
        status = quick_tblock_nullch_add(p_filter_quick_tblock);

    if(status_fail(status))
    {
        quick_tblock_dispose(p_filter_quick_tblock);
        return(status);
    }

    return(n_filters);
}

_Check_return_
extern T5_FILETYPE
windows_filter_list_get_t5_filetype_from_filter_index(
    _InVal_     S32 filter_index,
    _InVal_     S32 filter_mask,
    _InRef_     PC_ARRAY_HANDLE p_h_save_filetype /* SAVE_FILETYPE[] */)
{
    const BOOL fEnumerating = (0 == array_elements32(p_h_save_filetype));
    ARRAY_INDEX array_index = fEnumerating ? ENUMERATE_BOUND_FILETYPES_START : 0;
    S32 n_filters = 1; /* NB filter index returned by GetSaveFileName() is one-based */

    for(;;)
    {
        T5_FILETYPE t5_filetype;

        if(fEnumerating)
        {
            t5_filetype = enumerate_bound_filetypes(&array_index, filter_mask);

            if(ENUMERATE_BOUND_FILETYPES_START == array_index)
                break;
        }
        else
        {
            PC_SAVE_FILETYPE p_save_filetype;

            if(!array_index_is_valid(p_h_save_filetype, array_index))
                break;

            p_save_filetype = array_ptrc(p_h_save_filetype, SAVE_FILETYPE, array_index);

            t5_filetype = p_save_filetype->t5_filetype;

            ++array_index;
        }

        if(n_filters == filter_index)
            return(t5_filetype);

        ++n_filters;
    }

    return(FILETYPE_UNDETERMINED);
}

#if (NTDDI_VERSION >= NTDDI_VISTA)

/* Hook procedure not needed on Vista to centre dialogue box with NULL hwndOwner */

#else /* XP */

/* Hook procedure */

static void
OpenFileHook_onNotify(
    _HwndRef_   HWND hwnd,
    _InRef_     LPOFNOTIFY lpon)
{
    switch(lpon->hdr.code)
    {
    case CDN_INITDONE:
        { /* centre the file open box on the screen */
        RECT window_rect_dialog, window_rect_desktop;
        int width_dialog, height_dialog;
        int left_dialog, top_dialog;

        void_WrapOsBoolChecking(GetWindowRect(hwnd, &window_rect_dialog));
        width_dialog  = window_rect_dialog.right  - window_rect_dialog.left;
        height_dialog = window_rect_dialog.bottom - window_rect_dialog.top;

        if(NULL == ob_file_statics.openfilename.hwndOwner)
        {
            void_WrapOsBoolChecking(GetWindowRect(GetDesktopWindow(), &window_rect_desktop));

            left_dialog = (window_rect_desktop.right  - window_rect_desktop.left - width_dialog ) / 2;
            top_dialog  = (window_rect_desktop.bottom - window_rect_desktop.top  - height_dialog) / 2;

            void_WrapOsBoolChecking(MoveWindow(hwnd, left_dialog, top_dialog, width_dialog, height_dialog, TRUE));
        }

        break;
        }

    default:
        break;
    }
}

extern UINT_PTR CALLBACK
OpenFileHook(
    _HwndRef_   HWND hdlg,      /* handle to child dialog window NOT the 'Open File' window!!! */
    _In_        UINT uiMsg,     /* message identifier */
    _In_        WPARAM wParam,  /* message parameter */
    _In_        LPARAM lParam)  /* message parameter */
{
    const HWND hwnd = GetParent(hdlg); /* *This* is the 'Open File' window handle */

    UNREFERENCED_PARAMETER(wParam);

    switch(uiMsg)
    {
    case WM_NOTIFY:
        OpenFileHook_onNotify(hwnd, (LPOFNOTIFY) lParam);
        break;

    default:
        break;
    }

    return(0); /* allow default action */
}

#endif /* NTDDI_VERSION */

_Check_return_
static STATUS
insert_file_here(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_FILETYPE t5_filetype,
    _In_z_      PCTSTR filename)
{
    HCURSOR hcursor_old;
    STATUS status;
    const OBJECT_ID object_id = object_id_from_t5_filetype(t5_filetype);
    MSG_INSERT_FILE msg_insert_file;
    zero_struct(msg_insert_file);

    if(OBJECT_ID_NONE == object_id)
        return(create_error(ERR_UNKNOWN_FILETYPE));

    hcursor_old = SetCursor(LoadCursor(NULL, IDC_WAIT));

    cur_change_before(p_docu);

    msg_insert_file.filename = filename;
    msg_insert_file.t5_filetype = t5_filetype;
    msg_insert_file.insert = TRUE;
    msg_insert_file.ctrl_pressed = FALSE;
    /***msg_insert_file.of_ip_format.flags.insert = 1;*/
    msg_insert_file.position = p_docu->cur;
    skel_point_from_slr_tl(p_docu, &msg_insert_file.skel_point, &p_docu->cur.slr);
    status = object_call_id_load(p_docu, (OBJECT_ID_SKEL == object_id) ? T5_MSG_INSERT_OWNFORM : T5_MSG_INSERT_FOREIGN, &msg_insert_file, object_id);

    cur_change_after(p_docu);

    (void) SetCursor(hcursor_old);

    return(status);
}

#if (NTDDI_VERSION >= NTDDI_VISTA)
/* Is_Win_Vista_or_Later == TRUE */
#else

_Check_return_
static BOOL
Is_Win_Vista_or_Later(void) 
{
   OSVERSIONINFOEX osvi;
   DWORDLONG dwlConditionMask = 0;
   BYTE op = VER_GREATER_EQUAL;

   /* Initialize the OSVERSIONINFOEX structure. */
   ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
   osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
   osvi.dwMajorVersion = 6;
   osvi.dwMinorVersion = 0;

   /* Initialize the condition mask. */
   VER_SET_CONDITION(dwlConditionMask, VER_MAJORVERSION, op);
   VER_SET_CONDITION(dwlConditionMask, VER_MINORVERSION, op);

   /* Perform the test. */
   return(
       VerifyVersionInfo(
            &osvi,
            VER_MAJORVERSION | VER_MINORVERSION,
            dwlConditionMask));
}

#endif /* NTDDI_VERSION */

_Check_return_
static BOOL
file_is_zip(
    _In_z_      PCTSTR filename)
{
    PCTSTR tstr_extension = file_extension(filename);

    if(NULL == tstr_extension)
        return(FALSE);

    return(tstr_compare_equals_nocase(tstr_extension, TEXT("zip")));
}

_Check_return_
static STATUS
open_document(
    _DocuRef_   P_DOCU cur_p_docu,
    _InVal_     BOOL insert_file,
    _InVal_     BOOL importing_picture)
{
    STATUS status;
    TCHARZ szFile[BUF_MAX_PATHSTRING];
    const S32 filter_mask = importing_picture ? BOUND_FILETYPE_READ_PICT : BOUND_FILETYPE_READ;
    const ARRAY_HANDLE h_save_filetype = 0;
    BOOL ofnResult;
    PTSTR loop_filename;
    T5_FILETYPE t5_filetype;
    QUICK_TBLOCK_WITH_BUFFER(filter_quick_tblock, 256);
    QUICK_TBLOCK_WITH_BUFFER(insert_file_title_quick_tblock, 48);
    QUICK_TBLOCK_WITH_BUFFER(pathname_quick_tblock, 32);
    quick_tblock_with_buffer_setup(filter_quick_tblock);
    quick_tblock_with_buffer_setup(insert_file_title_quick_tblock);
    quick_tblock_with_buffer_setup(pathname_quick_tblock);

    ob_file_statics.openfilename.lStructSize = sizeof32(ob_file_statics.openfilename);
    ob_file_statics.openfilename.hInstance = GetInstanceHandle();
    ob_file_statics.openfilename.lpstrCustomFilter = ob_file_statics.szCustomFilter;
    ob_file_statics.openfilename.nMaxCustFilter = elemof32(ob_file_statics.szCustomFilter);
    ob_file_statics.openfilename.nFileOffset = 0;
    ob_file_statics.openfilename.nFileExtension = 0;
    ob_file_statics.openfilename.lpstrDefExt = extension_document_tstr;
    ob_file_statics.openfilename.lCustData = 0;
    ob_file_statics.openfilename.lpTemplateName = NULL;

    ob_file_statics.openfilename.Flags =
        OFN_ALLOWMULTISELECT    |
        OFN_EXPLORER            |
        OFN_NOCHANGEDIR         |
        OFN_PATHMUSTEXIST       |
        OFN_FILEMUSTEXIST       |
      /*OFN_HIDEREADONLY        |*/   /* hides the 'Open as read only' check box */
        OFN_ENABLESIZING        ;

    if(!insert_file && !importing_picture)
        ob_file_statics.openfilename.Flags |= OFN_ALLOWMULTISELECT;

    /* build filters as description,CH_NULL,wildcard spec,CH_NULL sets */
    status_return(windows_filter_list_create(&filter_quick_tblock, filter_mask, &h_save_filetype));

    /* try to find some sensible place to start off - load relative to existing document */
    if(!ob_file_statics.szDirName[0]) /* SKS 10may94 now static buffer, first time only initialise */
    {
        if(0 == MyGetProfileString(TEXT("DefaultDirectory"), tstr_empty_string, ob_file_statics.szDirName, elemof32(ob_file_statics.szDirName)))
        {
            if(!GetPersonalDirectoryName(ob_file_statics.szDirName, elemof32(ob_file_statics.szDirName)))
            {
                if(GetModuleFileName(GetInstanceHandle(), ob_file_statics.szDirName, elemof32(ob_file_statics.szDirName)))
                {
                    file_dirname(ob_file_statics.szDirName, ob_file_statics.szDirName);
                }
            }
        }
    }

    { /* always ensure dir sep char trimmed off end unless root */
    U32 len = tstrlen32(ob_file_statics.szDirName);
    if(len > elemof32(TEXT("C:") FILE_DIR_SEP_TSTR)-1)
    {
        PTSTR tstr = ob_file_statics.szDirName + len;
        if(*--tstr == FILE_DIR_SEP_CH)
            *tstr = CH_NULL;
    }
    } /*block*/

    ob_file_statics.openfilename.lpstrInitialDir = ob_file_statics.szDirName;

    szFile[0] = CH_NULL;
    ob_file_statics.openfilename.lpstrFile = szFile;
    ob_file_statics.openfilename.nMaxFile = elemof32(szFile);

    ob_file_statics.openfilename.lpstrFileTitle = NULL;
    ob_file_statics.openfilename.nMaxFileTitle = 0;

    if(insert_file)
    {
        const STATUS resource_id = FILE_MSG_INSERT_FILE_CAPTION;
        status_assert(resource_lookup_quick_tblock(&insert_file_title_quick_tblock, resource_id));
        status_assert(quick_tblock_nullch_add(&insert_file_title_quick_tblock));
    }

    /* some bits of OPENFILENAME need setting up each time */
    ob_file_statics.openfilename.hwndOwner = NULL /*host_get_icon_hwnd()*/;
    {
    const P_VIEW p_view = p_view_from_viewno_caret(cur_p_docu);
    if(!IS_VIEW_NONE(p_view))
    {
        ob_file_statics.openfilename.hwndOwner = p_view->main[WIN_BACK].hwnd;
        if(HOST_WND_NONE != p_view->pane[p_view->cur_pane].hwnd)
            ob_file_statics.openfilename.hwndOwner = p_view->pane[p_view->cur_pane].hwnd;
    }
    } /*block*/
    if(NULL == ob_file_statics.openfilename.hwndOwner)
    {
#if (NTDDI_VERSION >= NTDDI_VISTA)
        /* Hook procedure not needed on Vista to centre dialogue box with NULL hwndOwner */
#else
        if(!Is_Win_Vista_or_Later())
        {
            ob_file_statics.openfilename.Flags |= OFN_ENABLEHOOK;
            ob_file_statics.openfilename.lpfnHook = OpenFileHook;
        }
#endif /* NTDDI_VERSION */
    }

    ob_file_statics.openfilename.lpstrFilter = quick_tblock_tstr(&filter_quick_tblock);
    ob_file_statics.openfilename.lpstrTitle = insert_file ? quick_tblock_tstr(&insert_file_title_quick_tblock) : NULL;
    { /* need first time thru initialisation of these ***individually*** in the right cases */
    DWORD * p_nFilterIndex;
    if(insert_file)
    {
        p_nFilterIndex = &ob_file_statics.nFilterIndex[1];
        if(!*p_nFilterIndex)
            *p_nFilterIndex = 1; /* assumes All files (*.*) at start (newer style) */
    }
    else
    {
        p_nFilterIndex = &ob_file_statics.nFilterIndex[0];
        if(!*p_nFilterIndex)
            *p_nFilterIndex = 2; /* assumes Fireworkz document immediately after start */
    }
    ob_file_statics.openfilename.nFilterIndex = *p_nFilterIndex;
    } /*block*/

    ofnResult = GetOpenFileName(&ob_file_statics.openfilename);

    quick_tblock_dispose(&filter_quick_tblock);
    quick_tblock_dispose(&insert_file_title_quick_tblock);

    if(!ofnResult)
    {
        switch(CommDlgExtendedError())
        {
        case 0: return(STATUS_CANCEL);
        case CDERR_STRUCTSIZE: return(STATUS_FAIL);
        case CDERR_INITIALIZATION: return(status_nomem());
        case CDERR_NOTEMPLATE: return(STATUS_FAIL);
        case CDERR_LOADSTRFAILURE: return(create_error(FILE_ERR_LOADSTRFAIL));
        case CDERR_FINDRESFAILURE: return(create_error(FILE_ERR_FINDRESFAIL));
        case CDERR_LOADRESFAILURE: return(create_error(FILE_ERR_LOADRESFAIL));
        case CDERR_LOCKRESFAILURE: return(create_error(FILE_ERR_LOCKRESFAIL));
        case CDERR_MEMALLOCFAILURE: return(status_nomem());
        case CDERR_MEMLOCKFAILURE: return(status_nomem());
        case CDERR_NOHOOK: return(STATUS_FAIL);
        case CDERR_REGISTERMSGFAIL: return(status_nomem());
        case FNERR_SUBCLASSFAILURE: return(status_nomem());
        case FNERR_INVALIDFILENAME: return(create_error(FILE_ERR_BADNAME));
        case FNERR_BUFFERTOOSMALL: return(status_nomem());
        default: return(STATUS_FAIL);
        }
    }

    if(0 == (ob_file_statics.openfilename.Flags & OFN_ALLOWMULTISELECT))
        loop_filename = ob_file_statics.openfilename.lpstrFile;
    else
        loop_filename = ob_file_statics.openfilename.lpstrFile + ob_file_statics.openfilename.nFileOffset;

    for(;;)
    {
        PCTSTR filename;

        if(0 == (ob_file_statics.openfilename.Flags & OFN_ALLOWMULTISELECT))
        {   /* single file */
            filename = loop_filename;
        }
        else if(FILE_DIR_SEP_CH == ob_file_statics.openfilename.lpstrFile[ob_file_statics.openfilename.nFileOffset - 1])
        {   /* single file */
            filename = ob_file_statics.openfilename.lpstrFile;
        }
        else
        {   /* omit the trailing CH_NULL (multiple files) */
            status_break(status = quick_tblock_tstr_add_n(&pathname_quick_tblock, ob_file_statics.openfilename.lpstrFile, ob_file_statics.openfilename.nFileOffset - 1));
            status_break(status = quick_tblock_tchar_add(&pathname_quick_tblock, FILE_DIR_SEP_CH));
            status_break(status = quick_tblock_tstr_add_n(&pathname_quick_tblock, loop_filename, strlen_with_NULLCH));
            filename = quick_tblock_tstr(&pathname_quick_tblock);
        }

        if(file_is_zip(filename))
        {
            reperr(FILE_ERR_ISAZIP, filename);
            status_break(status = STATUS_FAIL);
        }

        /* 95% job will have to do. would be better if Windows kept more state */
        if(!importing_picture)
            file_dirname(ob_file_statics.szDirName, filename);

        /* remember filter indexes separately for each dialog */
        if(!importing_picture)
            ob_file_statics.nFilterIndex[insert_file ? 1 : 0] = ob_file_statics.openfilename.nFilterIndex;

        t5_filetype = t5_filetype_from_filename(filename);

        if(importing_picture)
        {
            MSG_INSERT_FOREIGN msg_insert_foreign;
            zero_struct(msg_insert_foreign);
            msg_insert_foreign.filename = filename;
            msg_insert_foreign.t5_filetype = t5_filetype;
            msg_insert_foreign.insert = TRUE;
            msg_insert_foreign.ctrl_pressed = FALSE;
            /***msg_insert_foreign.of_ip_format.flags.insert = 1;*/
            msg_insert_foreign.position = cur_p_docu->cur; /*irrelevant, just do it*/
            skel_point_from_slr_tl(cur_p_docu, &msg_insert_foreign.skel_point, &cur_p_docu->cur.slr);
            status = object_call_id(OBJECT_ID_CHART, cur_p_docu, T5_MSG_CHART_EDIT_INSERT_PICTURE, &msg_insert_foreign);
            break;
        }

        if(insert_file)
        {
            status = insert_file_here(cur_p_docu, t5_filetype, filename);
            break;
        }

        switch(t5_filetype)
        {
        case FILETYPE_T5_FIREWORKZ:
        case FILETYPE_T5_WORDZ:
        case FILETYPE_T5_RESULTZ:
        case FILETYPE_T5_RECORDZ:
            {
            const BOOL fReadOnly = (0 != (ob_file_statics.openfilename.Flags & OFN_READONLY));
            status = load_this_fireworkz_file_rl(cur_p_docu, filename, fReadOnly);
            break;
            }

        case FILETYPE_T5_TEMPLATE:
            status = load_this_template_file_rl(cur_p_docu, filename);
            break;

        case FILETYPE_T5_COMMAND:
            status = load_this_command_file_rl(cur_p_docu, filename);
            break;

        default:
            status = load_foreign_file_rl(cur_p_docu, filename, t5_filetype);
            break;
        }

        status_break(status);

        if(0 == (ob_file_statics.openfilename.Flags & OFN_ALLOWMULTISELECT))
            break;

        loop_filename = loop_filename + tstrlen32p1(loop_filename); /* skip to next */

        quick_tblock_dispose(&pathname_quick_tblock);

        if(CH_NULL == *loop_filename)
            break;
    }

    quick_tblock_dispose(&pathname_quick_tblock);

    return(status);
}

_Check_return_
static HRESULT
t5_SHGetSpecialFolderLocation(
    _In_        int nFolder,
    _Out_writes_z_(elemof_buffer) PTSTR p_dir_name /*filled*/,
    _InVal_     U32 elemof_buffer)
{
    HRESULT hresult;
    LPITEMIDLIST pidl;
    LPMALLOC lpMalloc;

    assert(0 != elemof_buffer);
    p_dir_name[0] = CH_NULL;

    if(NOERROR == (hresult = SHGetSpecialFolderLocation(NULL /*host_get_icon_hwnd()*/, nFolder, &pidl)))
    {
#if (NTDDI_VERSION >= NTDDI_VISTA)
        void_WrapOsBoolChecking(SHGetPathFromIDListEx(pidl, p_dir_name, elemof_buffer, GPFIDL_DEFAULT));
#else
        assert(elemof_buffer >= MAX_PATH); /* SHGetPathFromIDListEx() needs Vista or later */
        UNREFERENCED_PARAMETER_InVal_(elemof_buffer);

        if(!WrapOsBoolChecking(SHGetPathFromIDList(pidl, p_dir_name)))
            p_dir_name[0] = CH_NULL;
#endif /* NTDDI_VERSION */

        if(NOERROR == (hresult = SHGetMalloc(&lpMalloc)))
        {
            /* free the ID list */
            IMalloc_Free(lpMalloc, pidl);

            IMalloc_Release(lpMalloc);
        }
    }

    return(hresult);
}

_Check_return_
extern BOOL
GetPersonalDirectoryName(
    _Out_writes_z_(elemof_buffer) PTSTR p_dir_name /*filled*/,
    _InVal_     U32 elemof_buffer)
{
    HRESULT hresult;

    if(NOERROR == (hresult = t5_SHGetSpecialFolderLocation(CSIDL_PERSONAL, p_dir_name, elemof_buffer)))
        return(TRUE);

    return(FALSE);
}

_Check_return_
extern BOOL
GetCommonAppDataDirectoryName(
    _Out_writes_z_(elemof_buffer) PTSTR p_dir_name /*filled*/,
    _InVal_     U32 elemof_buffer)
{
    HRESULT hresult;

    if(NOERROR == (hresult = t5_SHGetSpecialFolderLocation(CSIDL_COMMON_APPDATA, p_dir_name, elemof_buffer)))
        return(TRUE);

    return(FALSE);
}

_Check_return_
extern BOOL
GetUserAppDataDirectoryName(
    _Out_writes_z_(elemof_buffer) PTSTR p_dir_name /*filled*/,
    _InVal_     U32 elemof_buffer)
{
    HRESULT hresult;

    if(NOERROR == (hresult = t5_SHGetSpecialFolderLocation(CSIDL_APPDATA, p_dir_name, elemof_buffer)))
        return(TRUE);

    return(FALSE);
}

_Check_return_
extern BOOL
GetCommonTemplatesDirectoryName(
    _Out_writes_z_(elemof_buffer) PTSTR p_dir_name /*filled*/,
    _InVal_     U32 elemof_buffer)
{
    HRESULT hresult;

    if(NOERROR == (hresult = t5_SHGetSpecialFolderLocation(CSIDL_COMMON_TEMPLATES, p_dir_name, elemof_buffer)))
        return(TRUE);

    return(FALSE);
}

_Check_return_
extern BOOL
GetUserTemplatesDirectoryName(
    _Out_writes_z_(elemof_buffer) PTSTR p_dir_name /*filled*/,
    _InVal_     U32 elemof_buffer)
{
    HRESULT hresult;

    if(NOERROR == (hresult = t5_SHGetSpecialFolderLocation(CSIDL_TEMPLATES, p_dir_name, elemof_buffer)))
        return(TRUE);

    return(FALSE);
}

#endif /* WINDOWS */

/******************************************************************************
*
* file service module event handler
*
******************************************************************************/

T5_MSG_PROTO(static, file_msg_initclose, _InRef_ PC_MSG_INITCLOSE p_msg_initclose)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__STARTUP_SERVICES:
        file_error_buffer_set(ob_file_statics.error_buffer, elemof32(ob_file_statics.error_buffer));

        return(resource_init(OBJECT_ID_FILE, MSG_WEAK, P_BOUND_RESOURCES_OBJECT_ID_FILE));

    case T5_MSG_IC__STARTUP:
        return(register_object_construct_table(OBJECT_ID_FILE, object_construct_table, FALSE /* no inlines */));

    case T5_MSG_IC__SERVICES_EXIT2:
        file_finalise();
        fileutil_shutdown();
        return(resource_close(OBJECT_ID_FILE));

    default:
        return(STATUS_OK);
    }
}

T5_MSG_PROTO(static, file_msg_error_rq, _InoutRef_ P_MSG_ERROR_RQ p_msg_error_rq)
{
    PCTSTR p_error = file_error_get();

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(NULL == p_error)
    {   /* no error to be returned */
        assert0();
        return(STATUS_FAIL);
    }

    tstr_xstrkpy(p_msg_error_rq->tstr_buf, p_msg_error_rq->elemof_buffer, p_error);
    return(STATUS_OK);
}

T5_CMD_PROTO(static, file_cmd_new_document)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);
    UNREFERENCED_PARAMETER_InRef_(p_t5_cmd);

    /* pop up template list or load single template */
    return(load_this_template_file_rl(p_docu, NULL));
}

#if WINDOWS

T5_CMD_PROTO(static, file_cmd_close_document_req)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);
    UNREFERENCED_PARAMETER_InRef_(p_t5_cmd);

    process_close_request(p_docu, P_VIEW_NONE, TRUE /*closing_a_doc*/, FALSE, FALSE);

    return(STATUS_OK);
}

#endif /* WINDOWS */

OBJECT_PROTO(extern, object_file);
OBJECT_PROTO(extern, object_file)
{
    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        return(file_msg_initclose(p_docu, t5_message, (PC_MSG_INITCLOSE) p_data));

    case T5_MSG_ERROR_RQ:
        return(file_msg_error_rq(p_docu, t5_message, (P_MSG_ERROR_RQ) p_data));

    case T5_CMD_NEW_DOCUMENT:
        return(file_cmd_new_document(p_docu, t5_message, (PC_T5_CMD) p_data));

#if WINDOWS
    case T5_CMD_OPEN_DOCUMENT:
        return(open_document(p_docu, FALSE, FALSE));

    case T5_CMD_CLOSE_DOCUMENT_REQ:
        return(file_cmd_close_document_req(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_MSG_INSERT_FILE_WINDOWS:
        return(open_document(p_docu, TRUE, FALSE));

    case T5_MSG_INSERT_FILE_WINDOWS_PICTURE:
        return(open_document(p_docu, TRUE, TRUE));
#endif /* WINDOWS */

    default:
        return(STATUS_OK);
    }
}

/* end of ob_file.c */
