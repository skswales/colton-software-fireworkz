/* sk_name.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Name routines for Fireworkz */

/* MRJC December 1991 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

/******************************************************************************
*
* compare two names
*
******************************************************************************/

_Check_return_
extern int
name_compare(
    _InRef_     PC_DOCU_NAME p_docu_name1,
    _InRef_     PC_DOCU_NAME p_docu_name2,
    _InVal_     BOOL add_extension)
{
    int res;
    QUICK_TBLOCK_WITH_BUFFER(quick_tblock_1, 128);
    QUICK_TBLOCK_WITH_BUFFER(quick_tblock_2, 128);
    quick_tblock_with_buffer_setup(quick_tblock_1);
    quick_tblock_with_buffer_setup(quick_tblock_2);

    status_assert(name_make_wholename(p_docu_name1, &quick_tblock_1, add_extension));
    status_assert(name_make_wholename(p_docu_name2, &quick_tblock_2, add_extension));

    res = tstricmp(quick_tblock_tstr(&quick_tblock_1), quick_tblock_tstr(&quick_tblock_2));

    quick_tblock_dispose(&quick_tblock_1);
    quick_tblock_dispose(&quick_tblock_2);

    return(res);
}

/******************************************************************************
*
* dispose of the content of a name block
*
******************************************************************************/

extern void
name_dispose(
    _InoutRef_  P_DOCU_NAME p_docu_name)
{
    tstr_clr(&p_docu_name->path_name);
    tstr_clr(&p_docu_name->leaf_name);
    tstr_clr(&p_docu_name->extension);
    p_docu_name->flags.path_name_supplied = 0;
}

/******************************************************************************
*
* donate a copy of filename resources, disposing of existing content
*
******************************************************************************/

extern void
name_donate(
    _InoutRef_  P_DOCU_NAME p_docu_name_out,
    _InRef_     PC_DOCU_NAME p_docu_name_in)
{
    name_dispose(p_docu_name_out);

    *p_docu_name_out = *p_docu_name_in;
}

/******************************************************************************
*
* make a copy of filename resources
*
******************************************************************************/

_Check_return_
extern STATUS
name_dup(
    _OutRef_    P_DOCU_NAME p_docu_name_out,
    _InRef_     PC_DOCU_NAME p_docu_name_in)
{
    STATUS status = STATUS_OK;

    name_init(p_docu_name_out);

    if(status_ok(status = tstr_set(&p_docu_name_out->path_name, p_docu_name_in->path_name)))
    if(status_ok(status = tstr_set(&p_docu_name_out->leaf_name, p_docu_name_in->leaf_name)))
    if(status_ok(status = tstr_set(&p_docu_name_out->extension, p_docu_name_in->extension)))
    {
        p_docu_name_out->flags = p_docu_name_in->flags;
        return(status);
    }

    name_dispose(p_docu_name_out);
    return(status);
}

/******************************************************************************
*
* ensure that a name has a path
*
******************************************************************************/

_Check_return_
extern STATUS
name_ensure_path(
    _InoutRef_  P_DOCU_NAME p_docu_name_out,
    _InRef_     PC_DOCU_NAME p_docu_name_in)
{
    STATUS status = STATUS_OK;

    if(NULL == p_docu_name_out->path_name)
    {
        status = tstr_set(&p_docu_name_out->path_name, p_docu_name_in->path_name);
        p_docu_name_out->flags.path_name_supplied = 0;
    }

    return(status);
}

/******************************************************************************
*
* initialise a name block
*
******************************************************************************/

extern void
name_init(
    _OutRef_    P_DOCU_NAME p_docu_name)
{
    zero_struct_ptr(p_docu_name);
    /*p_docu_name->path_name = NULL;*/
    /*p_docu_name->leaf_name = NULL;*/
    /*p_docu_name->extension = NULL;*/
    /*p_docu_name->flags.path_name_supplied = 0;*/
}

/******************************************************************************
*
* make a whole name from its parts
*
******************************************************************************/

_Check_return_
extern STATUS
name_make_wholename(
    _InRef_     PC_DOCU_NAME p_docu_name,
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*appended,terminated*/,
    _InVal_     BOOL add_extension)
{
    STATUS status = STATUS_OK;

    if(NULL != p_docu_name->path_name)
        status = quick_tblock_tstr_add(p_quick_tblock, p_docu_name->path_name);

    PTR_ASSERT(p_docu_name->leaf_name);
    if(status_ok(status))
        status = quick_tblock_tstr_add(p_quick_tblock, p_docu_name->leaf_name);

    if((NULL != p_docu_name->extension) && add_extension)
    {
        if(status_ok(status))
            status = quick_tblock_tchar_add(p_quick_tblock, FILE_EXT_SEP_CH);
        if(status_ok(status))
            status = quick_tblock_tstr_add(p_quick_tblock, p_docu_name->extension);
    }

    if(status_ok(status))
        status = quick_tblock_nullch_add(p_quick_tblock); /* no client wants this unterminated */

    return(status);
}

/******************************************************************************
*
* read a name from a string and split it into leaf and pathname
*
******************************************************************************/

_Check_return_
extern STATUS /* number of chars read including delimiter */
name_read_ustr(
    _OutRef_    P_DOCU_NAME p_docu_name,
    _In_z_      PC_USTR ustr_in,
    _In_        U8 delimiter /* in; 0=auto */)
{
    PC_USTR ustr = ustr_in;
    TCHARZ tstr_buffer[BUF_MAX_PATHSTRING];

#if USTR_IS_SBSTR
    U32 dst_idx = 0;

    while((CH_NULL != PtrGetByte(ustr)) && (PtrGetByte(ustr) != delimiter) && (dst_idx < elemof32(tstr_buffer)))
        tstr_buffer[dst_idx++] = *ustr++;

    if(PtrGetByte(ustr) != delimiter)
    {
        name_init(p_docu_name);
        return(0);
    }

    tstr_buffer[dst_idx++] = CH_NULL; /* JAD : Delimit wholename, or leafname is junk */
#else
    U32 ustr_n_bytes = 0;

    while((CH_NULL != PtrGetByte(ustr)) && (PtrGetByte(ustr) != delimiter))
    {
        const U32 bytes_of_char = ustr_bytes_of_char(ustr);
        ustr_IncBytes(ustr, bytes_of_char);
        ustr_n_bytes += bytes_of_char;
    }

    if(PtrGetByte(ustr) != delimiter)
    {
        name_init(p_docu_name);
        return(0);
    }

    consume(U32, tstr_buf_from_ustr(tstr_buffer, elemof32(tstr_buffer), ustr_in, ustr_n_bytes));
#endif

    if(PtrGetByte(ustr) != delimiter)
    {
        name_init(p_docu_name);
        return(0);
    }

    status_return(name_read_tstr(p_docu_name, tstr_buffer));

    /* skip delimiter too for success count */
    ustr_IncBytes(ustr, ustr_bytes_of_char(ustr));

    return(PtrDiffBytesS32(ustr, ustr_in));
}

_Check_return_
extern STATUS
name_read_tstr(
    _OutRef_    P_DOCU_NAME p_docu_name,
    _In_z_      PCTSTR tstr_in)
{
    STATUS status = STATUS_OK;
    PCTSTR wholename = tstr_in;
    PCTSTR tstr_extension;
    PCTSTR leafname;

    StrSkipSpaces(wholename);

    name_init(p_docu_name);

    tstr_extension = file_extension(wholename);

    if(file_is_rooted(wholename))
    {
        leafname = file_leafname(wholename);

        status = tstr_set_n(&p_docu_name->path_name, wholename, PtrDiffElemU32(leafname, wholename)); /* including dir sep ch */

        p_docu_name->flags.path_name_supplied = 1;
    }
    else
    {
        leafname = wholename;
    }

    if(status_ok(status))
    {
        if(NULL != tstr_extension)
            status = tstr_set_n(&p_docu_name->leaf_name, leafname, PtrDiffElemU32(tstr_extension - 1, leafname));
        else
            status = tstr_set(&p_docu_name->leaf_name, leafname);
    }

    if(status_ok(status))
        status = tstr_set(&p_docu_name->extension, tstr_extension); /* maybe NULL */

    if(status_fail(status))
        name_dispose(p_docu_name);

    return(status);
}

/******************************************************************************
*
* set up untitled name
*
******************************************************************************/

/*ncr*/
extern BOOL
name_preprocess_docu_name_flags_for_rename(
    _InoutRef_  P_DOCU_NAME p_docu_name)
{
    BOOL is_loaded_from_path = 0;

    trace_2(TRACE_APP_PD4, TEXT("name_preprocess_docu_name_flags_for_rename(path(%s) leaf(%s))"), report_tstr(p_docu_name->path_name), report_tstr(p_docu_name->leaf_name));

    if((NULL != p_docu_name->path_name) && file_is_rooted(p_docu_name->path_name))
    {
        STATUS status;
        QUICK_TBLOCK_WITH_BUFFER(quick_tblock, BUF_MAX_PATHSTRING);
        quick_tblock_with_buffer_setup(quick_tblock);

        p_docu_name->flags.path_name_supplied = 1;

        if(status_ok(status = quick_tblock_tstr_add_n(&quick_tblock, p_docu_name->path_name, strlen_with_NULLCH)))
        {
            PTCH dir_name = quick_tblock_tchars_wr(&quick_tblock);
            U32 trail_offset = quick_tblock_chars(&quick_tblock) - 1 /*CH_NULL*/ - 1;

            if(dir_name[trail_offset] == FILE_DIR_SEP_CH) /* overwrite trailing '.' but NOT ':' */
                dir_name[trail_offset] = CH_NULL;

            { /* strip off trailing Library from dir_name if present */
            PTSTR leaf_ptr = file_leafname(dir_name);
            QUICK_TBLOCK_WITH_BUFFER(quick_tblock_library, 8);
            quick_tblock_with_buffer_setup(quick_tblock_library);
            if( (NULL != leaf_ptr) &&
                status_ok(status = resource_lookup_quick_tblock(&quick_tblock_library, MSG_CUSTOM_LIBRARY)) &&
                status_ok(status = quick_tblock_nullch_add(&quick_tblock_library)) )
            {
                if(0 == tstricmp(leaf_ptr, quick_tblock_tstr(&quick_tblock_library)))
                    *leaf_ptr = CH_NULL;
            }
            quick_tblock_dispose(&quick_tblock_library);
            } /*block*/

            if(dir_name[trail_offset] == CH_NULL) /* restore trailing '.' iff it was removed */
                dir_name[trail_offset] = FILE_DIR_SEP_CH;

            reportf(TEXT("dir_name: %s"), dir_name);

            { /* scan to see if the remainder of the dir_name is one of our path elements */
            P_FILE_PATHENUM path;
            PCTSTR pathelem;

            for(pathelem = file_path_element_first(&path, file_get_search_path()); pathelem; pathelem = file_path_element_next(&path))
            {
                reportf(TEXT("compare with pathelem: %s"), pathelem);

                if((0 == tstricmp(dir_name, pathelem))  /*&&  file_is_dir(pathelem)*/)
                {
                    reportf(TEXT("is_loaded_from_path at pathelem: %s"), pathelem);
                    is_loaded_from_path = 1;
                    p_docu_name->flags.path_name_supplied = 0;
                    break;
                }
            }

            file_path_element_close(&path);
            } /*block*/
        }

        quick_tblock_dispose(&quick_tblock);
    }
    else
    {
        p_docu_name->flags.path_name_supplied = 0;
    }

    reportf(TEXT("is_loaded_from_path = %s, p_docu_name->flags.path_name_supplied = %s"), report_boolstring(is_loaded_from_path), report_boolstring(p_docu_name->flags.path_name_supplied));
    return(is_loaded_from_path);
}

/******************************************************************************
*
* set up untitled name
*
******************************************************************************/

_Check_return_
extern STATUS
name_set_untitled(
    _OutRef_    P_DOCU_NAME p_docu_name)
{
    return(name_set_untitled_with(p_docu_name, resource_lookup_tstr(MSG_FILENAME_UNTITLED)));
}

_Check_return_
extern STATUS
name_set_untitled_with(
    _OutRef_    P_DOCU_NAME p_docu_name,
    _In_z_      PCTSTR leafname)
{
    TCHARZ buffer[BUF_MAX_PATHSTRING];
    U32 name_len = tstrlen32(leafname);
    PCTSTR tstr_extension = file_extension(leafname);

    if(NULL != tstr_extension)
    {
        name_len = PtrDiffElemU32(tstr_extension - 1, leafname);

        if(0 == tstricmp(tstr_extension, extension_template_tstr))
            tstr_extension = extension_document_tstr; /* change fwt extension as fwk */
    }

#if !RISCOS
    if(NULL == tstr_extension)
        tstr_extension = extension_document_tstr;
#endif

    name_init(p_docu_name);

    {
    S32 i = 1;
    DOCU_NAME docu_name;
    name_init(&docu_name);
    docu_name.leaf_name = buffer; /* only temporary loan */
    docu_name.extension = de_const_cast(PTSTR, tstr_extension); /* ditto */
    for(;;)
    {
        tstr_xstrnkpy(buffer, elemof32(buffer), leafname, name_len);

        consume_int(tstr_xsnprintf(buffer + name_len, elemof32(buffer) - name_len,
                                   S32_TFMT,
                                   i++));

        if(docno_find_name(&docu_name) == DOCNO_NONE)
            break;
    }
    } /*block*/

    /* assign name (NB no path name set) */
    status_return(tstr_set(&p_docu_name->leaf_name, buffer));

    status_return(tstr_set(&p_docu_name->extension, tstr_extension));

    return(STATUS_OK);
}

/******************************************************************************
*
* write out a whole name

* if it has part or all of its path in common with another name, this is stripped out
*
******************************************************************************/

_Check_return_
extern U32 /* number of characters output */
name_write_ustr_buf(
    _Out_writes_z_(elemof_buffer) P_USTR ustr_buf,
    _InVal_     U32 elemof_buffer,
    _InoutRef_  P_DOCU_NAME p_docu_name_to /* name output */,
    _InRef_opt_ PC_DOCU_NAME p_docu_name_from /* can be NULL */,
    _InVal_     BOOL add_extension)
{
    assert(0 != elemof_buffer);

    /* strip out a pathname common to target and source documents */
    if(p_docu_name_to->flags.path_name_supplied)
    {
        PCTSTR tstr_name = tstr_empty_string;
        QUICK_TBLOCK_WITH_BUFFER(quick_tblock, 64);
        quick_tblock_with_buffer_setup(quick_tblock);

        if(status_ok(name_make_wholename(p_docu_name_to, &quick_tblock, add_extension)))
        {
            tstr_name = quick_tblock_tstr(&quick_tblock);

            if((NULL != p_docu_name_from) && (NULL != p_docu_name_from->path_name))
            {
                U32 path_len = tstrlen32(p_docu_name_from->path_name);

                if(0 == tstrnicmp(tstr_name, p_docu_name_from->path_name, path_len))
                    tstr_name += path_len;
            }
        }

        ustr_xstrkpy(ustr_buf, elemof_buffer, _ustr_from_tstr(tstr_name));

        quick_tblock_dispose(&quick_tblock);
    }
    else
        ustr_xstrkpy(ustr_buf, elemof_buffer, _ustr_from_tstr(p_docu_name_to->leaf_name));

    return(ustrlen32(ustr_buf));
}

/* end of sk_name.c */
