/* fileutil.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* File handling module (utils section) */

/* SKS June 1991 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#if RISCOS
#define EXPOSE_RISCOS_SWIS 1

#include "ob_skel/xp_skelr.h"
#endif

/*
internal functions
*/

_Check_return_
_Ret_z_
static PTSTR
file__make_usable_dir(
    _In_z_      PCTSTR dirname,
    _Out_writes_z_(elemof_buffer) PTSTR tstr_buf,
    _InVal_     U32 elemof_buffer);

typedef struct FILEUTIL_STATICS
{
#define N_SPECIAL_PATHS 3 /*std,net,exe*/
    PTSTR path[N_SPECIAL_PATHS]; /* alloc_block_tstr_set() */

    PTSTR search_path; /* alloc_block_tstr_set() */
}
FILEUTIL_STATICS;

static FILEUTIL_STATICS fileutil_statics;

/******************************************************************************
*
* obtain a path composed of the current filename and search path
*
******************************************************************************/

_Check_return_
extern STATUS
file_combine_path(
    _OutRef_    P_PTSTR aa,
    _In_opt_z_  PCTSTR currentfilename,
    _In_opt_z_  PCTSTR search_path)
{
    TCHARZ destpath[BUF_MAX_PATHSTRING];
    STATUS status = STATUS_OK;
    U32 tot_len = 0;

    file_get_cwd(destpath, currentfilename);

    /* size it up */
    if(CH_NULL != *destpath)
        tot_len = tstrlen32(destpath);

    if(NULL != search_path)
    {
        if(CH_NULL != *destpath)
            tot_len += tstrlen32(FILE_PATH_SEP_TSTR);

        tot_len += tstrlen32(search_path);
    }

    if(0 == tot_len)
    {
        *aa = NULL;
        return(STATUS_OK);
    }

    status_return(tstr_set_n(aa, NULL, tot_len));

    tot_len++; /* remember CH_NULL on buffer too for strkat_s */

    /* copy over elements */
    if(CH_NULL != *destpath)
        tstr_xstrkat(*aa, tot_len, destpath);

    if(NULL != search_path)
    {
        if(CH_NULL != *destpath)
            tstr_xstrkat(*aa, tot_len, FILE_PATH_SEP_TSTR);

        tstr_xstrkat(*aa, tot_len, search_path);
    }

    return(status);
}

_Check_return_
extern STATUS
file_derive_name(
    _In_z_      PCTSTR dir,
    _In_z_      PCTSTR leafname,
    _In_z_      PCTSTR suffix,
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*appended,terminated*/,
    _InVal_     STATUS status_isafile)
{
    U32 prefix_len = tstrlen32(leafname);
    U32 suffix_len = tstrlen32(suffix);
    U32 n_chars = 255;
    TCHARZ filename[BUF_MAX_PATHSTRING];

    if(!file_is_dir(dir))
        return(create_error(FILE_ERR_NOTADIR));

#if RISCOS
    /* presume systems prior to RISC OS 4 have limited name length */
    if(host_os_version_query() < RISCOS_4_0)
        n_chars = 10;
#endif

    assert(n_chars >= prefix_len);
    assert(n_chars >  suffix_len);

    if( prefix_len > n_chars - suffix_len)
        prefix_len = n_chars - suffix_len;

    tstr_xstrkpy(filename, elemof32(filename), dir);
    tstr_xstrnkat(filename, elemof32(filename), leafname, prefix_len); /* copy a limited amount over */
    tstr_xstrkat(filename, elemof32(filename), suffix);

    if(file_is_file(filename))
    {
        if(status_isafile)
        {
            PCTSTR format = resource_lookup_tstr(status_isafile);
            TCHARZ error_buffer[BUF_MAX_PATHSTRING];
            consume_int(tstr_xsnprintf(error_buffer, elemof32(error_buffer), format, filename));
            return(file_error_set(error_buffer));
        }

        return(create_error(FILE_ERR_ISAFILE));
    }

    return(quick_tblock_tchars_add(p_quick_tblock, filename, tstrlen32p1(filename) /*CH_NULL*/));
}

/******************************************************************************
*
* return the directory part of a filename with correct termination
*
* e.g.
*   c:\windev\lib\lwindll.lib -> c:\windev\lib\
*   adfs::4.$.cmodules.c.file -> adfs::4.$.cmodules.c.
*
******************************************************************************/

extern PTSTR
file_dirname(
    PTSTR destpath,
    PCTSTR srcfilename)
{
    return(file_separatename(destpath, NULL, srcfilename));
}

/******************************************************************************
*
* return the extension part of a filename
*
* e.g.
*   file.c -> c
*   fixx_z -> z
*
******************************************************************************/

_Check_return_
_Ret_maybenull_z_
extern PTSTR
file_extension(
    _In_z_      PCTSTR filename)
{
    return(file_extension_ch(filename, FILE_EXT_SEP_CH)); /* scan backwards only over the leafname for native extension character */
}

_Check_return_
_Ret_maybenull_z_
extern PTSTR
file_extension_ch(
    _In_z_      PCTSTR filename,
    _InVal_     TCHAR ch)
{
    PCTSTR leafname = file_leafname(filename);
    PCTSTR ptr = tstrrchr(leafname, ch); /* scan backwards only over the leafname for this specific character */

    if(ptr)
        return((PTSTR) ptr + 1);

    return(NULL);
}

/******************************************************************************
*
* --in--
*
*   object enumeration structure from file_find_first
*
* --out--
*
*   resources freed
*
******************************************************************************/

extern void
file_find_close(
    /*inout*/ P_P_FILE_OBJENUM pp)
{
    P_FILE_OBJENUM p = *pp;

    if(p)
    {
#if WINDOWS
        if(INVALID_HANDLE_VALUE != p->win32_find_handle)
        {
            FindClose(p->win32_find_handle);
            p->win32_find_handle = INVALID_HANDLE_VALUE;
        }
#endif

        file_path_element_close(&p->pathenum);

        al_ptr_dispose(P_P_ANY_PEDANTIC(pp));
    }
}

/******************************************************************************
*
* --in--
*
*   object enumeration structure to initialise from path and pattern
*
* --out--
*
*   NULL    no matching objects
*   else    first matching object
*
******************************************************************************/

extern P_FILE_OBJINFO
file_find_first(
    /*out*/ P_P_FILE_OBJENUM pp,
    _In_z_      PCTSTR path,
    _In_z_      PCTSTR pattern)
{
    return(file_find_first_subdir(pp, path, pattern, NULL));
}

extern P_FILE_OBJINFO
file_find_first_subdir(
    /*out*/ P_P_FILE_OBJENUM pp,
    _In_z_      PCTSTR path,
    _In_z_      PCTSTR pattern,
    _In_opt_z_  PCTSTR subdir)
{
    STATUS status;
    P_FILE_OBJENUM p;

    if(NULL == (*pp = p = al_ptr_calloc_elem(FILE_OBJENUM, 1, &status)))
    {
        status_assert(status);
        return(NULL);
    }

    if(NULL == file_path_element_first(&p->pathenum, path))
    {
        al_ptr_dispose(P_P_ANY_PEDANTIC(pp));
        return(NULL);
    }

    /* initialise object enumeration structure */

    if(subdir)
        tstr_xstrkpy(p->subdir, elemof32(p->subdir), subdir);
    else
        *p->subdir = CH_NULL;

    tstr_xstrkpy(p->pattern, elemof32(p->pattern), pattern);

#if WINDOWS
    p->win32_find_handle = INVALID_HANDLE_VALUE;
#endif

    /* have path element, requires validation */
    p->state = 1;

    return(file_find_next(pp));
}

/******************************************************************************
*
* --in--
*
*   object enumeration structure from file_find_first
*
* --out--
*
*   NULL    no more matching objects
*   else    next matching object
*
*   current drive/directory corrupt
*
*   Note that if other processes in the program set the drive
*   or directory between calls then wrong results will be returned
*
******************************************************************************/

#if WINDOWS

static BOOL
file_find_windows_result_convert(
    P_FILE_OBJINFO oip)
{
    /* enumeration functions can return silly '.' and '..' */
    if(oip->win32_find_data.cFileName[0] == CH_FULL_STOP)
        return(FALSE);

    oip->type = (oip->win32_find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? FILE_OBJECT_DIRECTORY : FILE_OBJECT_FILE;
    oip->t5_filetype = FILETYPE_UNDETERMINED; /* lazy git can't be bothered */
    return(TRUE);
}

#endif

extern P_FILE_OBJINFO
file_find_next(
    /*inout*/ P_P_FILE_OBJENUM pp)
{
    P_FILE_OBJENUM p = *pp;
    PTSTR dirname;
    TCHARZ tstr_buf[BUF_MAX_PATHSTRING];

    /* loop finding objects */
    for(;;)
    {
        if(p->state == 3)
        {
            /* keep looking in current dir */

            {
#if RISCOS
            _kernel_osgbpb_block gbpbblk;

            if(p->riscos_dirindex < 0)
            {
                /* already exhausted current dir, restart with new path element */
                p->state = 0;
                continue;
            }

            dirname = file__make_usable_dir(p->pathenum->res, tstr_buf, elemof32(tstr_buf));

            if(*p->subdir) /* not empty? */
            {
                if(dirname == p->pathenum->res)
                    dirname = strcpy(tstr_buf, dirname);

                (void) strcat(strcat(dirname, FILE_DIR_SEP_TSTR), p->subdir);
                /* dirname IS valid, cos we checked it when p->state was 2 */
            }

            gbpbblk.dataptr  = &p->objinfo.fileinfo;            /* r2 Result buffer               */
            gbpbblk.nbytes   = 1;                               /* r3 One object                  */
            gbpbblk.fileptr  = p->riscos_dirindex;              /* r4 Offset to item in directory */
            gbpbblk.buf_len  = sizeof32(p->objinfo.fileinfo);   /* r5 Result buffer length        */
            gbpbblk.wild_fld = p->pattern;                      /* r6 Wildcarded name to match    */

            if(_kernel_ERROR == _kernel_osgbpb(OSGBPB_ReadDirEntriesInfo /*10*/, (unsigned) dirname, &gbpbblk))
            {
                /* error, restart with new path element */
                p->state = 0;
                continue;
            }

            p->riscos_dirindex = gbpbblk.fileptr;

            /* loop if returned none matched this time */
            if(gbpbblk.nbytes <= 0)
                continue;

            p->objinfo.type = p->objinfo.fileinfo._type;
            p->objinfo.t5_filetype = (p->objinfo.type == FILE_OBJECT_DIRECTORY)
                                ? FILETYPE_DIRECTORY
                                : (T5_FILETYPE) (((U32) p->objinfo.fileinfo.load >> 8) & 0xFFFU);
#elif WINDOWS
            if(!FindNextFile(p->win32_find_handle, &p->objinfo.win32_find_data))
            {
                /* no more found (or error), restart with new path element */
                FindClose(p->win32_find_handle);
                p->win32_find_handle = INVALID_HANDLE_VALUE;

                p->state = 0;
                continue;
            }

            if(!file_find_windows_result_convert(&p->objinfo))
                continue;
#endif
            } /*block*/

            return(&p->objinfo);
        }

        if(p->state == 2)
        {
            /* got a good dir, start search */
            dirname = file__make_usable_dir(p->pathenum->res, tstr_buf, elemof32(tstr_buf));

            if(*p->subdir) /* not empty? */
            {
                if(dirname == p->pathenum->res)
                {
                    tstr_xstrkpy(tstr_buf, elemof32(tstr_buf), dirname);
                    dirname = tstr_buf;
                }

                tstr_xstrkat(dirname, elemof32(tstr_buf), FILE_DIR_SEP_TSTR);
                tstr_xstrkat(dirname, elemof32(tstr_buf), p->subdir);

                if(!file_is_dir(dirname))
                {
                    /* subdir invalid, get new path element */
                    p->state = 0;
                    continue;
                }
            }

#if RISCOS
            /* start hunting for files in this directory */
            p->riscos_dirindex = 0;
            p->state = 3;
            continue;

#elif WINDOWS
            dirname = file__make_usable_dir(p->pathenum->res, tstr_buf, elemof32(tstr_buf));

            if(*p->subdir) /* not empty? */
            {
                if(dirname == p->pathenum->res)
                {
                    tstr_xstrkpy(tstr_buf, elemof32(tstr_buf), dirname);
                    dirname = tstr_buf;
                }

                tstr_xstrkat(dirname, elemof32(tstr_buf), FILE_DIR_SEP_TSTR);
                tstr_xstrkat(dirname, elemof32(tstr_buf), p->subdir);
                /* dirname IS valid, cos we checked it when p->state was 2 */
            }

            tstr_xstrkat(dirname, elemof32(tstr_buf), FILE_DIR_SEP_TSTR);
            tstr_xstrkat(dirname, elemof32(tstr_buf), p->pattern);

            p->win32_find_handle = FindFirstFile(dirname, &p->objinfo.win32_find_data);

            if(INVALID_HANDLE_VALUE == p->win32_find_handle)
            {
                /* nothing found (or error), restart with new path element */
                p->state = 0;
                continue;
            }

            p->state = 3;

            if(!file_find_windows_result_convert(&p->objinfo))
                continue;

            return(&p->objinfo);

#else
#error file_find_next not implemented on this system
#endif
        }

        /* path element requires validation? */
        if(p->state == 1)
        {
            if(file_is_dir(p->pathenum->res))
                /* valid dir found, restart search */
                p->state = 2;
            else
                /* dir invalid, get new path element */
                p->state = 0;

            continue;
        }

        /* require new path element? */
        if(p->state == 0)
        {
            if(NULL != file_path_element_next(&p->pathenum))
                /* possible dir found, requires validation */
                p->state = 1;
            else
                /* no more paths to search - close enumeration */
                break;

            continue;
        }

        myassert1x(p->state == 0, TEXT("bad object enumeration state ") S32_TFMT, p->state);
        break;
    }

    file_find_close(pp);

    return(NULL);
}

/******************************************************************************
*
* return the directory in which the last object
* returned by file_find_first/file_find_next was found
*
* --in--
*
*   object enumeration structure from file_find_first
*
******************************************************************************/

extern PTSTR
file_find_query_dirname(
    _InRef_     P_P_FILE_OBJENUM pp,
    _Out_writes_z_(elemof_buffer) PTSTR destpath,
    _InVal_     U32 elemof_buffer)
{
    P_FILE_OBJENUM p = *pp;
    PTSTR dirname = file__make_usable_dir(p->pathenum->res, destpath, elemof_buffer);

    if(dirname == p->pathenum->res)
    {
        tstr_xstrkpy(destpath, elemof_buffer, dirname);
        dirname = destpath;
    }

    assert(dirname == destpath);

    if(*p->subdir) /* not empty? */
    {
        tstr_xstrkat(dirname, elemof_buffer, FILE_DIR_SEP_TSTR);
        tstr_xstrkat(dirname, elemof_buffer, p->subdir);
    }

    return(destpath);
}

_Check_return_
extern STATUS
file_objenum_fullname(
    P_P_FILE_OBJENUM pp,
    P_FILE_OBJINFO oip,
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*appended,terminated*/)
{
    TCHARZ buffer[BUF_MAX_PATHSTRING];
    file_find_query_dirname(pp, buffer, elemof32(buffer));
    tstr_xstrkat(buffer, elemof32(buffer), FILE_DIR_SEP_TSTR);
    status_return(quick_tblock_tstr_add(p_quick_tblock, buffer));
    return(file_objinfo_name(oip, p_quick_tblock));
}

_Check_return_
extern STATUS
file_objinfo_name(
    _InRef_     PC_FILE_OBJINFO oip,
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*appended,terminated*/)
{
    PCTSTR name;

#if RISCOS
    name = oip->fileinfo.name;
#elif WINDOWS
    name = oip->win32_find_data.cFileName;
#endif

    return(quick_tblock_tchars_add(p_quick_tblock, name, tstrlen32p1(name) /*CH_NULL*/));
}

_Check_return_
extern T5_FILETYPE
file_objinfo_filetype(
    _InRef_     PC_FILE_OBJINFO oip)
{
    return(oip->t5_filetype);
}

_Check_return_
extern U32
file_objinfo_size(
    _InRef_     PC_FILE_OBJINFO oip)
{
#if RISCOS
    return((oip->type == FILE_OBJECT_DIRECTORY) ? (U32) 0 : (U32) oip->fileinfo.size);
#elif WINDOWS
    return((oip->type == FILE_OBJECT_DIRECTORY) ? (U32) 0 : (U32) oip->win32_find_data.nFileSizeLow);
#endif
}

_Check_return_
extern STATUS
file_objinfo_type(
    _InRef_     PC_FILE_OBJINFO oip)
{
    return(oip->type);
}

/******************************************************************************
*
* try to find a file anywhere on the current path/cwd
*
* --out--
*
*   -ve: error
*     0: not found
*   +ve: found, filename set to found filename
*
******************************************************************************/

_Check_return_
extern STATUS
file_find_on_path(
    _Out_writes_z_(elemof_buffer) PTSTR filename,
    _InVal_     U32 elemof_buffer,
    _In_z_      PCTSTR srcfilename)
{
    S32 res = 0;
    P_FILE_PATHENUM path;
    PTSTR pathelem;
    U32 frt = file_reference_type(srcfilename);

    *filename = CH_NULL;

    /* SKS 20may96 - if filename completely the wrong sort of OS, just use its leafname */
#if RISCOS
    if(FILE_REFERENCE_WINDOWS == frt)
    {
        /* SKS 20may96 - hack for Dave Woods et al - try to load files even when they've got wrong OS/wrong abs references */
        srcfilename = file_leafname_windows(srcfilename);
    }
#else
    if(FILE_REFERENCE_RISCOS == frt)
    {
        /* SKS 20may96 - hack for Dave Woods et al - try to load files even when they've got wrong OS/wrong abs references */
        srcfilename = file_leafname_riscos(srcfilename);
    }
#endif
    else if(file_is_rooted(srcfilename))
    {
        /* ALWAYS copy in! */
        tstr_xstrkpy(filename, elemof_buffer, srcfilename);
        if((res = file_is_file(filename)) != 0)
            return(res);
        srcfilename = file_leafname(srcfilename); /* SKS 21may96 continues the above hack */
    }

    for(pathelem = file_path_element_first(&path, file_get_search_path()); NULL != pathelem; pathelem = file_path_element_next(&path))
    {
        if(file_is_dir(pathelem))
        {
            tstr_xstrkpy(filename, elemof_buffer, pathelem);
            tstr_xstrkat(filename, elemof_buffer, srcfilename);

#if RISCOS
            { /* may have come over from Windows with a DOS file extension, so no point trying with that, is there? */
            PTSTR extn = file_extension_ch(filename, CH_FULL_STOP);

            if(NULL != extn)
                *extn = CH_NULL;
            } /*block*/
#endif

            if((res = file_is_file(filename)) != 0)
                break;
        }
    }

    file_path_element_close(&path);

    trace_3(TRACE_MODULE_FILE, TEXT("file_find_on_path(%s) yields filename \")%s\" & %s"),
            report_tstr(srcfilename), report_tstr(filename), report_boolstring(res > 0));
    return(res);
}

_Check_return_
extern STATUS
file_find_on_path_or_relative(
    _Out_writes_z_(elemof_buffer) PTSTR filename,
    _InVal_     U32 elemof_buffer,
    _In_z_      PCTSTR srcfilename,
    _In_opt_z_  PCTSTR currentfilename)
{
    S32 res = 0;
    PTSTR tstr_path;
    P_FILE_PATHENUM path;
    PTSTR pathelem;
    U32 frt;

    if(NULL == currentfilename)
        return(file_find_on_path(filename, elemof_buffer, srcfilename));

    *filename = CH_NULL;

    frt = file_reference_type(srcfilename);

    /* SKS 20may96 - if filename completely the wrong sort of OS, just use its leafname */
#if RISCOS
    if(FILE_REFERENCE_WINDOWS == frt)
    {
        /* SKS 20may96 - hack for Dave Woods et al - try to load files even when they've got wrong OS/wrong abs references */
        srcfilename = file_leafname_windows(srcfilename);
    }
#else
    if(FILE_REFERENCE_RISCOS == frt)
    {
        /* SKS 20may96 - hack for Dave Woods et al - try to load files even when they've got wrong OS/wrong abs references */
        srcfilename = file_leafname_riscos(srcfilename);
    }
#endif
    else if(file_is_rooted(srcfilename))
    {
        /* ALWAYS copy in! */
        tstr_xstrkpy(filename, elemof_buffer, srcfilename);
        if((res = file_is_file(filename)) != 0)
            return(res);
        srcfilename = file_leafname(srcfilename); /* SKS 21may96 continues the above hack */
    }

    status_assert(file_combine_path(&tstr_path, currentfilename, file_get_search_path()));

    for(pathelem = file_path_element_first(&path, tstr_path); NULL != pathelem; pathelem = file_path_element_next(&path))
    {
        if(file_is_dir(pathelem))
        {
            tstr_xstrkpy(filename, elemof_buffer, pathelem);
            tstr_xstrkat(filename, elemof_buffer, srcfilename);

#if RISCOS
            { /* may have come over from Windows with a DOS file extension, so no point trying with that, is there? */
            PTSTR leaf = file_leafname(filename);
            PTSTR extn = strrchr(leaf, CH_FULL_STOP);

            if(NULL != extn)
                *extn = CH_NULL;
            } /*block*/
#endif

            if((res = file_is_file(filename)) != 0)
                break;
        }
    }

    file_path_element_close(&path);

    tstr_clr(&tstr_path);

    trace_4(TRACE_MODULE_FILE, TEXT("file_find_on_path_or_relative(%s, %s) yields filename \")%s\" & %s"),
            report_tstr(srcfilename), report_tstr(currentfilename), report_tstr(filename), report_boolstring(res > 0));
    return(res);
}

#if defined(UNUSED_KEEP_ALIVE) /* currently unused */

/******************************************************************************
*
* try to find a dir. anywhere on the current path/cwd
*
* --out--
*
*   -ve: error
*     0: not found
*   +ve: found, filename set to found dirname
*
******************************************************************************/

_Check_return_
extern STATUS
file_find_dir_on_path(
    _Out_writes_z_(elemof_buffer) PTSTR filename,
    _InVal_     U32 elemof_buffer,
    _In_z_      PCTSTR srcfilename,
    _In_opt_z_  PCTSTR currentfilename)
{
    S32 res = 0;
    PTSTR tstr_path;
    P_FILE_PATHENUM path;
    PTSTR pathelem;

    if(file_is_rooted(srcfilename))
    {   /* ALWAYS copy in! */
        tstr_xstrkpy(filename, elemof_buffer, srcfilename);
        return(file_is_dir(filename));
    }

    status_assert(file_combine_path(&tstr_path, currentfilename, file_get_search_path()));

    for(pathelem = file_path_element_first(&path, tstr_path); NULL != pathelem; pathelem = file_path_element_next(&path))
    {
        if(file_is_dir(pathelem))
        {
            tstr_xstrkpy(filename, elemof_buffer, pathelem);
            tstr_xstrkat(filename, elemof_buffer, srcfilename);

            if((res = file_is_dir(filename)) != 0)
                break;
        }
    }

    file_path_element_close(&path); /* SKS 1.03 17mar93 */

    tstr_clr(&tstr_path);

    trace_2(TRACE_MODULE_FILE, TEXT("file_find_dir_on_path() yields dirname \")%s\" & %s"), report_tstr(filename), report_boolstring(res > 0));
    return(res);
}

#endif /* UNUSED_KEEP_ALIVE */

/******************************************************************************
*
* obtain a directory prefix from current filename
*
******************************************************************************/

extern PTSTR
file_get_cwd(
    PTSTR destpath,
    PCTSTR currentfilename)
{
    PCTSTR namep = currentfilename;
    PTSTR res;
    U32 n_chars = 0;

    *destpath = CH_NULL;

    if(namep)
    {
        PCTSTR leafp = file_leafname(namep);
        n_chars = PtrDiffElemU32(leafp, namep);
    }

    if(n_chars)
        tstr_xstrnkpy(destpath, BUF_MAX_PATHSTRING, namep, n_chars); /* no -1 as we're not bounding the output buffer */

    res = *destpath ? destpath : NULL;

    trace_3(TRACE_MODULE_FILE,
            TEXT("file_get_cwd(%s) yields cwd = \"%s\", has cwd = %s"),
            report_tstr(currentfilename), report_tstr(res), report_boolstring(*destpath));
    return(res);
}

/******************************************************************************
*
* obtain a directory prefix from current filename or first valid path element
*
******************************************************************************/

extern PTSTR
file_get_prefix(
    _Out_writes_z_(elemof_buffer) PTSTR destpath,
    _InVal_     U32 elemof_buffer,
    _In_opt_z_  PCTSTR currentfilename)
{
    PTSTR tstr_path;
    P_FILE_PATHENUM path;
    PTSTR pathelem;

    *destpath = CH_NULL;

    status_assert(file_combine_path(&tstr_path, currentfilename, file_get_search_path()));

    for(pathelem = file_path_element_first(&path, tstr_path); NULL != pathelem; pathelem = file_path_element_next(&path))
    {
        if(file_is_dir(pathelem))
        {
            tstr_xstrkpy(destpath, elemof_buffer, pathelem);
            break;
        }
    }

    file_path_element_close(&path); /* SKS 1.03 17mar93 */

    tstr_clr(&tstr_path);

    trace_1(TRACE_MODULE_FILE, TEXT("file_get_prefix yields %s"), destpath);
    return(*destpath ? destpath : NULL);
}

/******************************************************************************
*
* --out--
*
*   current path being used for searches
*
******************************************************************************/

extern PCTSTR
file_get_search_path(void)
{
    return(fileutil_statics.search_path);
}

/******************************************************************************
*
* determine whether a dir exists
*
******************************************************************************/

_Check_return_
extern BOOL
file_is_dir(
    _In_z_      PCTSTR dirname)
{
    TCHARZ buffer[MAX_PATHSTRING];

    dirname = file__make_usable_dir(de_const_cast(PTSTR /*broken promise*/, dirname), buffer, elemof32(buffer));

    {
#if RISCOS
    _kernel_swi_regs rs;
    rs.r[0] = OSFile_ReadNoPath;
    rs.r[1] = (int) dirname;
    if(NULL != _kernel_swi(OS_File, &rs, &rs))
        return(FALSE);
    return((rs.r[0] == OSFile_ObjectType_Dir) || (rs.r[0] == OSFile_ObjectType_Image));
#elif WINDOWS
    DWORD dword = GetFileAttributes(dirname);
    if(INVALID_FILE_ATTRIBUTES == dword)
    {
        const DWORD dwLastError = GetLastError();
        UNREFERENCED_PARAMETER_CONST(dwLastError);
        return(FALSE);
    }
    return((dword & FILE_ATTRIBUTE_DIRECTORY) != 0);
#else
    return(FALSE);
#endif
    } /*block*/
}

/******************************************************************************
*
* determine whether a file exists
*
******************************************************************************/

_Check_return_
extern BOOL
file_is_file(
    _In_z_      PCTSTR filename)
{
#if RISCOS
    _kernel_swi_regs rs;
    rs.r[0] = OSFile_ReadNoPath;
    rs.r[1] = (int) filename;
    if(NULL != _kernel_swi(OS_File, &rs, &rs))
        return(FALSE);
    return(rs.r[0] == OSFile_ObjectType_File);
#elif WINDOWS
    DWORD dword = GetFileAttributes(filename);
    if(INVALID_FILE_ATTRIBUTES == dword)
    {
        const DWORD dwLastError = GetLastError();
        UNREFERENCED_PARAMETER_CONST(dwLastError);
        return(FALSE);
    }
    return((dword & FILE_ATTRIBUTE_DIRECTORY) == 0);
#else
    return(FALSE);
#endif
}

/******************************************************************************
*
* determine whether a file is read only
*
******************************************************************************/

_Check_return_
extern BOOL
file_is_read_only(
    _In_z_      PCTSTR filename)
{
#if RISCOS
    _kernel_swi_regs rs;
    rs.r[0] = OSFile_ReadNoPath;
    rs.r[1] = (int) filename;
    if(NULL != _kernel_swi(OS_File, &rs, &rs))
        return(FALSE);
    if(rs.r[0] != OSFile_ObjectType_File)
        return(FALSE);
    if((rs.r[5] & OSFile_ObjectAttribute_write) != 0) /* writable bit set? */
        return(FALSE);
    return(TRUE);
#elif WINDOWS
    DWORD dword = GetFileAttributes(filename);
    if(INVALID_FILE_ATTRIBUTES == dword)
    {
        const DWORD dwLastError = GetLastError();
        UNREFERENCED_PARAMETER_CONST(dwLastError);
        return(FALSE);
    }
    return((dword & FILE_ATTRIBUTE_READONLY) != 0);
#else
    return(FALSE);
#endif
}

/*
test for valid dos drive prefix
*/

#define file__DosDriveNumber(d) ( \
    (/*"C"*/isalpha(*d) && (*(d+1) == ':')) ? (/*"C"*/tolower(*d) - ('a'-1)) : -1 )

/******************************************************************************
*
* determine whether a filename is 'rooted' sufficiently
* to open not relative to cwd or path
*
******************************************************************************/

_Check_return_
extern BOOL
file_is_rooted(
    _In_z_      PCTSTR filename)
{
    BOOL res;

#if RISCOS
    res = (NULL != strpbrk(filename, ":$&%@\\"));
#elif WINDOWS
    /* rooted WINDOWS filenames are either
     *  '\[dir\]*leaf'
     * or
     *  '<drive>:[\][dir\]*leaf'
     *  where drive name always one a-zA-Z
     * or UNC names
     *  '\\computername\mount\[dir\]*leaf'
    */
    if( (*filename == FILE_DIR_SEP_CH)
#ifdef                FILE_DIR_SEP_CH2
    ||  (*filename == FILE_DIR_SEP_CH2)
#endif
        )
        res = TRUE;
    else
        res = (file__DosDriveNumber(filename) != -1);
#endif

    trace_2(TRACE_MODULE_FILE,
            TEXT("file_is_rooted(%s): %s"), filename, report_boolstring(res));
    return(res);
}

/******************************************************************************
*
* return the leafname part of a filename
*
******************************************************************************/

_Check_return_
_Ret_z_
extern PTSTR
file_leafname(
    _In_z_      PCTSTR filename)
{
    PCTSTR leaf = filename + tstrlen32(filename);  /* point to CH_NULL */

    /* loop back over filename looking for a directory separator or a root delimiter */
    while(leaf > filename)
    {
        TCHAR ch = *--leaf;

        if( (ch == FILE_ROOT_CH)
         || (ch == FILE_DIR_SEP_CH)
#ifdef             FILE_DIR_SEP_CH2
         || (ch == FILE_DIR_SEP_CH2)
#endif
            )
        {
            /* FILE_DIR_SEP_CH & FILE_ROOT_CH are not multibyte chars,
             * so AnsiNext unneccessary
            */
            ++leaf;
            break;
        }
    }

    return(de_const_cast(PTSTR, leaf));
}

/******************************************************************************
*
* return the leafname part of a RISC OS filename
*
******************************************************************************/

_Check_return_
_Ret_z_
extern PTSTR
file_leafname_riscos(
    _In_z_      PCTSTR filename)
{
    PCTSTR leaf = filename + tstrlen32(filename);  /* point to CH_NULL */

    /* loop back over filename looking for a directory separator or a root delimiter */
    while(leaf > filename)
    {
        TCHAR ch = *--leaf;

        if((ch == CH_COLON) || (ch == CH_FULL_STOP))
        {
            ++leaf;
            break;
        }
    }

    return(de_const_cast(PTSTR, leaf));
}

/******************************************************************************
*
* return the leafname part of a Windows filename
*
******************************************************************************/

_Check_return_
_Ret_z_
extern PTSTR
file_leafname_windows(
    _In_z_      PCTSTR filename)
{
    PCTSTR leaf = filename + tstrlen32(filename);  /* point to CH_NULL */

    /* loop back over filename looking for a directory separator or a root delimiter */
    while(leaf > filename)
    {
        TCHAR ch = *--leaf;

        if((ch == CH_COLON) || (ch == CH_BACKWARDS_SLASH) || (ch == CH_FORWARDS_SLASH))
        {
            ++leaf;
            break;
        }
    }

    return(de_const_cast(PTSTR, leaf));
}

/******************************************************************************
*
* --in--
*
*   path enumeration structure from file_path_element_first
*
* --out--
*
*   resources freed
*
******************************************************************************/

extern void
file_path_element_close(
    /*inout*/ P_P_FILE_PATHENUM pp)
{
    /* thankfully simple */
    if((NULL != pp) && (NULL != *pp))
    {
        tstr_clr(&(*pp)->path);

        al_ptr_dispose(P_P_ANY_PEDANTIC(pp));
    }
}

/******************************************************************************
*
* --in--
*
*   path enumeration structure to initialise from path
*
* --out--
*
*   NULL    no more path elements
*   else    correctly terminated path element, might not exist
*
******************************************************************************/

extern PTSTR
file_path_element_first(
    /*out*/ P_P_FILE_PATHENUM pp,
    PCTSTR path)
{
    STATUS status;
    P_FILE_PATHENUM p;

    if(NULL == (*pp = p = al_ptr_calloc_elem(FILE_PATHENUM, 1, &status)))
    {
        status_assert(status);
        return(NULL);
    }

    /* initialise path enumeration structure */
    status_assert(tstr_set(&p->path, path));
    p->ptr = p->path;

    return(file_path_element_next(pp));
}

/******************************************************************************
*
* --in--
*
*   path enumeration structure from file_path_element_first
*
* --out--
*
*   NULL    no more path elements
*   else    correctly terminated path element, might not exist
*
******************************************************************************/

extern PTSTR
file_path_element_next(
    /*inout*/ P_P_FILE_PATHENUM pp)
{
    P_FILE_PATHENUM p = *pp;

    for(;;) /* loop needed as there may be empty path elements */
    {
        PCTSTR tstr = p->ptr;
        PTSTR tstr_res = p->res;
        TCHAR ch;
        BOOL is_blank = FALSE;

        *tstr_res = CH_NULL;

        /* strip off leading spaces */
        while((ch = *tstr++) == CH_SPACE)
        { /*EMPTY*/ }
        --tstr;

        /* path ended? */
        if(CH_NULL == *tstr)
        {
            file_path_element_close(pp);
            return(NULL);
        }

        /* loop till we find a path separator or eos. at least a CH_NULL will be copied */
        do  {
            ch = *tstr++;

            if(ch == FILE_PATH_SEP_CH)
                ch = CH_NULL;

            *tstr_res++ = ch;
        }
        while(ch);
        --tstr_res; /* point at that CH_NULL again */

        /* retract pointer as needed for safety, else leave pointing to next path element */
        if(*--tstr != CH_NULL)
            ++tstr;

        p->ptr = tstr;

        /* strip off any copied trailing spaces - at least a CH_NULL was copied, and tstr_res was pointing to it. ch exits as the last non-space in the string */
        for(;;)
        {
            if(tstr_res == p->res)
            {
                is_blank = TRUE;
                break;
            }
            ch = *--tstr_res;
            if(ch != CH_SPACE) /* leave tstr_res pointing **at** the last non-space */
                break;
            *tstr_res = CH_NULL;
        }

        if(is_blank)
            /* empty path element found; loop back to top */
            continue;

        /* ensure path correctly terminated */
        if((ch != FILE_ROOT_CH)
        && (ch != FILE_DIR_SEP_CH)
#ifdef            FILE_DIR_SEP_CH2
        && (ch != FILE_DIR_SEP_CH2)
#endif
            )
        {
            *++tstr_res = FILE_DIR_SEP_CH;
            *++tstr_res = CH_NULL;
        }
        /* otherwise got a correctly terminated end element */
        break;
    }

    return(p->res);
}

/*
what sort of OS does the filename refer to?
*/

extern U32
file_reference_type(
    _In_z_      PCTSTR filename)
{
    /* rooted DOS filenames are either
     *  '\[dir\]*leaf'
     * or
     *  '<drive>:[\][dir\]*leaf'
     *  where drive name always one a-zA-Z
     * or UNC names
     *  '\\computername\mount\[dir\]*leaf'
    */
    if((*filename == CH_BACKWARDS_SLASH) || (*filename == CH_FORWARDS_SLASH))
        return(FILE_REFERENCE_WINDOWS);

    if(file__DosDriveNumber(filename) != -1)
        return(FILE_REFERENCE_WINDOWS);

    if(NULL != tstrchr(filename, ':')) /* A colon then means it's not Windows */
        return(FILE_REFERENCE_RISCOS);

    return(FILE_REFERENCE_UNKNOWN);
}

/******************************************************************************
*
* split up a pathname into a directory and filename parts
*
* --in--
*
*   NULL may be passed as either destpath or destfilename
*        if these are not wanted
*
* --out--
*
*   any non-existent component comes back as an empty string
*   destpath will be returned terminated
*
******************************************************************************/

extern PTSTR
file_separatename(
    PTSTR destpath,
    PTSTR destfilename,
    PCTSTR srcfilename)
{
    PCTSTR leaf = file_leafname(srcfilename);

    if(destfilename)
        tstr_xstrkpy(destfilename, BUF_MAX_PATHSTRING, leaf);

    if(destpath)
    {
        *destpath = CH_NULL;

        if(leaf != srcfilename)
            tstr_xstrnkpy(destpath, BUF_MAX_PATHSTRING, srcfilename, PtrDiffElemU32(leaf, srcfilename)); /* no -1 as we're not bounding the output buffer */
    }

    return((destpath && *destpath) ? destpath : NULL);
}

/* call like status = file_tempname(file_dirname(file_that_exists), "xf", ...); --- not an exact replacement for _tempnam */

_Check_return_
extern STATUS
file_tempname(
    _In_z_      PCTSTR dirname,
    _In_z_      PCTSTR prefix,
    _In_opt_z_  PCTSTR suffix,
    _InVal_     S32 flags,
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*appended,terminated*/)
{
    BOOL use_monotime = ((flags & FILE_TEMPNAME_INITIAL_TRY) == 0);
    /*U32 dirname_len = tstrlen32(dirname);*/
    U32 prefix_len = tstrlen32(prefix);
    const U32 min_prefix_len = (0 == prefix_len) ? 0 : 1;
    U32 suffix_len = (NULL != suffix) ? tstrlen32(suffix) : 0;
    const U32 min_places = 2; /* always leave some variability */
    U32 places = 4;
    U32 n_chars = 255;
    MONOTIME m = monotime();
    int i = 0;

    if(!file_is_dir(dirname))
        return(create_error(FILE_ERR_NOTADIR));

#if RISCOS
    /* presume systems prior to RISC OS 4 have limited name length */
    if(host_os_version_query() < RISCOS_4_0)
        n_chars = 10;
#endif

    /* leafname too long? */
    if(prefix_len + places + suffix_len > n_chars)
    {
        /* first try reducing number of places */
        /* ensure there's room for some variability! */
        if(n_chars >=          (prefix_len + min_places + suffix_len))
            places = n_chars - (prefix_len              + suffix_len);
        else
            places = min_places;
    }

    if(prefix_len + places + suffix_len > n_chars)
    {
        /* next try reducing length of prefix (even down to zero) */
        if(n_chars >=              (places + min_prefix_len + suffix_len))
            prefix_len = n_chars - (places                  + suffix_len);
        else
            prefix_len = min_prefix_len;
    }

    if(prefix_len + places + suffix_len > n_chars)
        return(create_error(FILE_ERR_NAMETOOLONG));

    for(;;)
    {
        TCHARZ filename[BUF_MAX_PATHSTRING];
        PTSTR tstr;
        int int_places = (int) places;

        tstr_xstrkpy(filename, elemof32(filename), dirname);
        tstr_xstrnkat(filename, elemof32(filename), prefix, prefix_len); /* copy a limited amount over */

        tstr = filename + tstrlen32(filename);

        if(use_monotime)
        {
            for(;;) /* loop till timer ticks over */
            {
                MONOTIME m_1 = monotime();

                if(m != m_1)
                {
                    m = m_1;
                    break;
                }
            }

            consume_int(tstr_xsnprintf(tstr, elemof32(filename) - PtrDiffElemU32(tstr, filename),
                                       TEXT("%*.*x"),
                                       int_places, int_places, (int) m));
        }
        else
        {
            consume_int(tstr_xsnprintf(tstr, elemof32(filename) - PtrDiffElemU32(tstr, filename),
                                       TEXT("%*.*d"),
                                       int_places, int_places, i++));
        }

        if(NULL != suffix)
            tstr_xstrkat(filename, elemof32(filename), suffix);

        if(file_is_file(filename))
        {
#if 1
            /* initial attempt failed, so now use next number as suffix */
#else
            use_monotime = TRUE; /* initial attempt failed, so now use monotime as suffix */
#endif
            continue;
        }

        return(quick_tblock_tchars_add(p_quick_tblock, filename, tstrlen32p1(filename) /*CH_NULL*/));
    }
}

_Check_return_
extern STATUS
file_tempname_null(
    _In_z_      PCTSTR prefix,
    _In_opt_z_  PCTSTR suffix,
    _InVal_     S32 flags,
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*appended,terminated*/)
{
    STATUS status = STATUS_OK;
    QUICK_TBLOCK_WITH_BUFFER(quick_tblock, 256);
    quick_tblock_with_buffer_setup(quick_tblock);

#if RISCOS
    { /* check that the preferred variable exists */
    char buffer[16];
    _kernel_swi_regs rs;

    rs.r[0] = (int) TEXT("Wimp$ScrapDir");
    rs.r[1] = (int) buffer;
    rs.r[2] = sizeof32(buffer);
    rs.r[3] = 0;
    rs.r[4] = 3;
    void_WrapOsErrorChecking(_kernel_swi(OS_ReadVarVal, &rs, &rs));

    if(rs.r[2] == 0)
        status = create_error(ERR_RISCOS_NO_SCRAP);
    } /*block*/

    if(status_ok(status))
        status = quick_tblock_tstr_add(&quick_tblock, TEXT("<") TEXT("Wimp$ScrapDir") TEXT(">") FILE_DIR_SEP_TSTR);
#elif WINDOWS
    {
    TCHAR buffer[BUF_MAX_PATHSTRING];
    size_t requiredSize;

    if(0 == _tgetenv_s(&requiredSize, buffer, elemof32(buffer), TEXT("TEMP")))
    {
        status = quick_tblock_tstr_add(&quick_tblock, buffer);
    }
    else
    {
        status = create_error(FILE_ERR_NOTADIR);
    }

    if(status_ok(status))
        status = quick_tblock_tstr_add(&quick_tblock, FILE_DIR_SEP_TSTR);
    } /*block*/
#endif

    if(status_ok(status))
        status = quick_tblock_tstr_add_n(&quick_tblock, product_family_id(), strlen_with_NULLCH);

    if(status_ok(status))
        if(!file_is_dir(quick_tblock_tstr(&quick_tblock)))
            status = file_create_directory(quick_tblock_tstr(&quick_tblock));

    if(status_ok(status))
    {
        quick_tblock_nullch_strip(&quick_tblock);
        status = quick_tblock_tstr_add_n(&quick_tblock, FILE_DIR_SEP_TSTR, strlen_with_NULLCH);
    }

    if(status_ok(status))
        status = file_tempname(quick_tblock_tstr(&quick_tblock), prefix, suffix, flags, p_quick_tblock);

    quick_tblock_dispose(&quick_tblock);

    return(status);
}

/******************************************************************************
*
* return a pointer to the first wild part
* of a filename, if it is wild, or NULL
*
******************************************************************************/

_Check_return_
_Ret_maybenull_z_
extern PTSTR
file_wild(
    _In_z_      PCTSTR filename)
{
    PCTSTR ptr = filename;
    TCHAR ch = *ptr;

    do  {
        if((ch == FILE_WILD_MULTIPLE)  ||  (ch == FILE_WILD_SINGLE))
        {
            trace_2(TRACE_MODULE_FILE,
                    TEXT("file_wild(%s): %s"), filename, ptr);
            return((PTSTR) ptr);
        }

        ch = *++ptr;
    }
    while(ch);

    trace_1(TRACE_MODULE_FILE, TEXT("file_wild(%s): NULL"), filename);
    return(NULL);
}

/******************************************************************************
*
* internal routines
*
******************************************************************************/

/******************************************************************************
*
* given a dirname, strip off unneccesary trailing dir sep ch, copying to a buffer
* if mods are needed. return ptr to either this copy or original
*
******************************************************************************/

_Check_return_
_Ret_z_
static PTSTR
file__make_usable_dir(
    _In_z_      PCTSTR dirname,
    _Out_writes_z_(elemof_buffer) PTSTR tstr_buf,
    _InVal_     U32 elemof_buffer)
{
    PCTSTR ptr;
    TCHAR ch;

    assert(0 != elemof_buffer);
    tstr_buf[0] = CH_NULL;

    ptr = dirname + tstrlen32(dirname);

    ch = *--ptr;

    if( (ch == FILE_DIR_SEP_CH)
#ifdef         FILE_DIR_SEP_CH2
    ||  (ch == FILE_DIR_SEP_CH2)
#endif
      ) {
#if WINDOWS
        /* must allow \ or c:\ */
        if(ptr != dirname)
            if((ptr != dirname + 2) || (file__DosDriveNumber(dirname) == -1))
#endif /* WINDOWS */
            {   /* need to strip off that carefully placed dir sep */
                U32 len = PtrDiffElemU32(ptr, dirname);
                tstr_xstrnkpy(tstr_buf, elemof_buffer, dirname, len); /* no -1 ... */
                dirname = tstr_buf;
            }
    }
#if RISCOS
    else if(ch == FILE_ROOT_CH)
    {   /* need to add CSD symbol on end */
        tstr_xstrkpy(tstr_buf, elemof_buffer, dirname);
        tstr_xstrkat(tstr_buf, elemof_buffer, "@");
        dirname = tstr_buf;
    }
#endif

    return((PTSTR) dirname);
}

extern PCTSTR
file_path_query(
    _In_        UINT i)
{
    assert(i < N_SPECIAL_PATHS);
    return(fileutil_statics.path[i]);
}

_Check_return_
extern STATUS
file_path_set(
    PCTSTR tstr,
    _In_        UINT i)
{
    assert(i < N_SPECIAL_PATHS);
    /*tstr_clr(&fileutil_statics.path[i]);*/
    fileutil_statics.path[i] = NULL;

    if(NULL == tstr)
        return(STATUS_OK);

    return(alloc_block_tstr_set(&fileutil_statics.path[i], tstr, &global_string_alloc_block));
}

/* loop over the places we've set up and build a full search path for those that need it */

extern void
file_build_path(void)
{
    TCHARZ full_path[BUF_MAX_PATHSTRING * 4];
    UINT i;

    full_path[0] = CH_NULL;

    for(i = FILE_PATH_STANDARD; i <= FILE_PATH_EXECUTABLE; ++i)
    {
        PCTSTR p_path = file_path_query(i);

        if(NULL == p_path)
            continue;

        if(NULL != tstrstr(full_path, p_path))
            continue;

        if(full_path[0])
            tstr_xstrkat(full_path, elemof32(full_path), FILE_PATH_SEP_TSTR);

        tstr_xstrkat(full_path, elemof32(full_path), p_path);
    }

    status_assert(alloc_block_tstr_set(&fileutil_statics.search_path, full_path, &global_string_alloc_block));
}

extern void
fileutil_shutdown(void)
{
}

/* end of fileutil.c */
