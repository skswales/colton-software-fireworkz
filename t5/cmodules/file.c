/* file.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1990-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* File handling module (stream section) */

/* SKS February 1990 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#if RISCOS
#define EXPOSE_RISCOS_SWIS 1

#include "ob_skel/xp_skelr.h"
#endif

#if WINDOWS

#include "sys/types.h"
#include "sys/stat.h"

__pragma(warning(push)) __pragma(warning(disable:4820)) /* padding added after data member */
#include "io.h"
__pragma(warning(pop))
#endif

/*
internal functions
*/

_Check_return_
static STATUS
file__closefile(
    _InoutRef_  FILE_HANDLE file_handle);

#if TRACE_ALLOWED

_Check_return_
static STATUS
file__flushbuffer(
    _InoutRef_  FILE_HANDLE file_handle,
    _In_z_      PCTSTR caller);

#else

_Check_return_
static STATUS
file___flushbuffer(
    _InoutRef_  FILE_HANDLE file_handle);

#define file__flushbuffer(f, caller) \
    file___flushbuffer(file_handle)

#endif

_Check_return_
static STATUS
file__getpos(
    _InoutRef_  FILE_HANDLE file_handle,
    _Out_       filepos_t * const p_cur_pos);

_Check_return_
static STATUS
file__make_output_buffer(
    _InoutRef_  FILE_HANDLE file_handle);

_Check_return_
static STATUS
file__read(
    _Out_writes_bytes_(bytestoread_in) P_ANY ptr,
    _InVal_     U32 bytestoread_in,
    _OutRef_    P_U32 p_bytesread,
    _InoutRef_  FILE_HANDLE file_handle);

_Check_return_
static STATUS
file__seek(
    _InoutRef_  FILE_HANDLE file_handle,
    _InVal_     S64 offset64,
    _InVal_     S32 origin);

_Check_return_
static STATUS
file__set_error(
    _InoutRef_  FILE_HANDLE file_handle,
    _InVal_     STATUS status);

_Check_return_
static STATUS
file__write(
    _In_reads_bytes_(bytestowrite) PC_ANY ptr,
    _InVal_     U32 bytestowrite,
    _InoutRef_  FILE_HANDLE file_handle);

#if RISCOS

_Check_return_
static STATUS
file__obtain_error_string(
    _In_opt_    const _kernel_oserror * const e);

#endif /* OS */

/*
helper macros
*/

/*
private fields in file handle flags
*/

#define _FILE_READ       0x0001
#define _FILE_WRITE      0x0002
#define _FILE_CLOSED     0x0004
#ifndef _FILE_EOFREAD
#define _FILE_EOFREAD    0x0008
#endif
#define _FILE_BUFDIRTY   0x0010
#define _FILE_USERBUFFER 0x0020 /* user allocated buffer, not us */
#define _FILE_UNBUFFERED 0x0040 /* user specified no buffering */
#define _FILE_HASUNGOTCH 0x0080

/* ------------------------------------------------------------------------- */

static BOOL file__initialised = FALSE;

/*
a linked list of all open files
*/

#define _FILE_LIST_END ((FILE_HANDLE) 0x0000)

static FILE_HANDLE file_list;

/*
default buffer size to allocate for single byte access
*/

static U32 file__defbufsiz = FILE_DEFBUFSIZ;

/*
hold an error string from the filing system
*/

static PTSTR file__errorbuffer;
static U32   file__errorbufelem;
static PTSTR file__errorptr;

/******************************************************************************
*
* set up buffering
*
* --in--
*
*   file_handle == 0 -> set default buffer size
*   file_handle != 0 -> set buffer on open file
*
******************************************************************************/

_Check_return_
extern STATUS
file_buffer(
    _InoutRef_opt_ FILE_HANDLE file_handle,
    _In_opt_    void * buffer,
    _InVal_     U32 bufsize)
{
    STATUS status;
    U32 usable_bufsize = bufsize;

    trace_3(TRACE_MODULE_FILE, TEXT("file_buffer(") PTR_XTFMT TEXT(", ") PTR_XTFMT TEXT(", ") U32_TFMT TEXT(")"), file_handle, buffer, bufsize);

    if(!file_handle)
    {
        trace_1(TRACE_MODULE_FILE, TEXT("file_buffer -- NULL file_handle sets up default buffer size of ") U32_TFMT, bufsize);
        file__defbufsiz = bufsize;
        return(STATUS_OK);
    }

    if(file_handle->magic != _FILE_MAGIC_WORD)
        return(create_error(FILE_ERR_BADHANDLE));

    status = STATUS_OK;

    if(buffer)
    {
        myassert1x(bufsize != 0, TEXT("file_buffer with buffer ") PTR_XTFMT TEXT(" but bufsize 0"), buffer);

        /* free buffer if any and we allocated it */
        if(!(file_handle->flags & _FILE_USERBUFFER))
            al_ptr_dispose(&file_handle->bufbase);

        file_handle->flags |= _FILE_USERBUFFER;
    }
    else
    {
        /* consider user has done file_buffer(f, b, sizeof(b)) then
         * file_buffer(f, NULL, q) - we have to assume that he's doing
         * this for a good reason e.g. the buffer is going out of scope
         * so failure to alloc can leave file in an unbuffered and error state
        */
        file_handle->flags = file_handle->flags & ~_FILE_USERBUFFER;

        if(bufsize)
        {
            if(NULL == (buffer = al_ptr_alloc_bytes(void *, bufsize, &status)))
                status = file__set_error(file_handle, create_error(FILE_ERR_HANDLEUNBUFFERED));
        }

        if(!buffer)
            usable_bufsize = 0;
    }

    file_handle->count = -1;

    file_handle->bufbase = buffer;
    file_handle->bufsize = usable_bufsize;

    return(status);
}

/******************************************************************************
*
* clear error indicator on stream
*
******************************************************************************/

_Check_return_
extern STATUS
file_clearerror(
    _InoutRef_opt_ FILE_HANDLE file_handle)
{
    STATUS status;

    trace_1(TRACE_MODULE_FILE, TEXT("file_clearerror(") PTR_XTFMT TEXT(")"), file_handle);

    if(!file_handle || (file_handle->magic != _FILE_MAGIC_WORD))
        return(create_error(FILE_ERR_BADHANDLE));

    status = file_handle->error;
    file_handle->error = 0;

    trace_1(TRACE_MODULE_FILE, TEXT("file_clearerror returns old error ") S32_TFMT, status);
    return(status);
}

/******************************************************************************
*
* close stream, flushing data to disc
*
******************************************************************************/

_Check_return_
extern STATUS
t5_file_close(
    _InoutRef_  P_FILE_HANDLE p_file_handle)
{
    FILE_HANDLE file_handle;
    STATUS      status;

    trace_2(TRACE_MODULE_FILE, TEXT("t5_file_close(") PTR_XTFMT TEXT(" -> ") PTR_XTFMT TEXT(")"), p_file_handle, p_file_handle ? *p_file_handle : NULL);

    if(IS_PTR_NULL_OR_NONE(p_file_handle))
    {
        PTR_ASSERT(p_file_handle);
        return(create_error(FILE_ERR_BADHANDLE));
    }

    file_handle = *p_file_handle;

    *p_file_handle = NULL;

    /* if file already closed, don't worry */
    if(!file_handle)
        return(STATUS_OK);

    if(!file_handle || (file_handle->magic != _FILE_MAGIC_WORD))
        return(create_error(FILE_ERR_BADHANDLE));

    { /* search for file and delink from list - first time round always awkward */
    FILE_HANDLE pp = (FILE_HANDLE) &file_list;
    FILE_HANDLE cp =                file_list;

    while(cp != _FILE_LIST_END)
    {
        if(cp == file_handle)
            break;

        pp = cp;
        cp = pp->next;
    }

    if(cp == _FILE_LIST_END)
        return(create_error(FILE_ERR_BADHANDLE));

    /* take care in delinking */
    if(pp == (FILE_HANDLE) &file_list)
        file_list = file_handle->next;
    else
        pp->next = file_handle->next;
    } /*block*/

    if(_FILE_HANDLE_NULL != file_handle->handle)
        status = file__closefile(file_handle);
    else
        /* nop if already closed on fs but good */
        status = STATUS_OK;

    /* free buffer if any and we allocated it */
    if(!(file_handle->flags & _FILE_USERBUFFER))
        al_ptr_dispose(&file_handle->bufbase);

    /* free file descriptor */
    al_ptr_dispose(P_P_ANY_PEDANTIC(&file_handle));

    trace_3(TRACE_MODULE_FILE, TEXT("t5_file_close(") PTR_XTFMT TEXT(" -> ") PTR_XTFMT TEXT(") yields ") S32_TFMT, p_file_handle, file_handle, status);
    return(status);
}

_Check_return_
extern STATUS
file_close_date(
    _InoutRef_  P_FILE_HANDLE p_file_handle,
    _OutRef_    P_EV_DATE p_ev_date)
{
    FILE_HANDLE file_handle;
    STATUS status, status1;

    trace_2(TRACE_MODULE_FILE, TEXT("file_close_date(") PTR_XTFMT TEXT(" -> ") PTR_XTFMT TEXT(")"), p_file_handle, p_file_handle ? *p_file_handle : NULL);

    /* if file already closed, whinge */
    if((NULL == p_file_handle) || (NULL == (file_handle = *p_file_handle)) || (file_handle->magic != _FILE_MAGIC_WORD))
    {
        ev_date_init(p_ev_date);
        return(create_error(FILE_ERR_BADHANDLE));
    }

    status = file__closefile(file_handle);

    status_assert(file_date(file_handle, p_ev_date));

    status1 = t5_file_close(p_file_handle);

    if(status_ok(status))
        status = status1;

    return(status);
}

_Check_return_
extern STATUS
file_create_directory(
    _In_z_      PCTSTR dirname)
{
#if RISCOS
    _kernel_osfile_block osfile_block;

    CODE_ANALYSIS_ONLY(zero_struct(osfile_block));

    if(_kernel_ERROR == _kernel_osfile(OSFile_CreateDir, dirname, &osfile_block))
    {
        _kernel_oserror * e = _kernel_last_oserror();
        if(NULL != e)
            return(file_error_set(e->errmess));
        return(FILE_ERR_MKDIR_FAILED);
    }
#elif WINDOWS
    if(!CreateDirectory(dirname, NULL))
    {
        DWORD dwLastError = GetLastError();
        if(dwLastError != ERROR_ALREADY_EXISTS)
            return(FILE_ERR_MKDIR_FAILED);
    }
#else
    _mkdir(dirname);
#endif /* OS */

    return(STATUS_OK);
}

_Check_return_
extern STATUS
file_date(
    _InoutRef_opt_ FILE_HANDLE file_handle,
    _OutRef_    P_EV_DATE p_ev_date)
{
    S32 year, month, day;
    S32 hours, minutes, seconds;

    if((NULL == file_handle) || (file_handle->magic != _FILE_MAGIC_WORD))
    {
        ev_date_init(p_ev_date);
        return(create_error(FILE_ERR_BADHANDLE));
    }

    {
#if RISCOS
    /* return cached date */
    _kernel_swi_regs rs;
    int five_byte_time[2];
    U8Z result_buffer[256];
    int sscanf_result;

    five_byte_time[0] = file_handle->riscos.file_date_loword;
    five_byte_time[1] = file_handle->riscos.file_date_hiword;

    rs.r[0] = (int) five_byte_time;
    rs.r[1] = (int) result_buffer;
    rs.r[2] = sizeof32(result_buffer);
    rs.r[3] = (int) "%CE%YR.%MN.%DY %24:%MI:%SE.%CS"; /* returns local time representation of the given UTC */
    if(NULL != WrapOsErrorChecking(_kernel_swi(OS_ConvertDateAndTime, &rs, &rs)))
    {
        ev_date_init(p_ev_date);
        return(create_error(FILE_ERR_BADTIMESTAMP));
    }

    sscanf_result = sscanf(result_buffer, "%d.%d.%d %d:%d:%d.", &year, &month, &day, &hours, &minutes, &seconds);
    if(6 != sscanf_result)
    {
        assert(6 == sscanf_result);
        ev_date_init(p_ev_date);
        return(create_error(FILE_ERR_BADTIMESTAMP));
    }
#elif WINDOWS
    /* return cached date */
    FILETIME filetime;
    SYSTEMTIME systemtime;

    (void) FileTimeToLocalFileTime(&file_handle->windows.file_date, &filetime);

    if(!WrapOsBoolChecking(FileTimeToSystemTime(&filetime, &systemtime)))
    {
        ev_date_init(p_ev_date);
        return(create_error(FILE_ERR_BADTIMESTAMP));
    }

    seconds = systemtime.wSecond;
    minutes = systemtime.wMinute;
    hours = systemtime.wHour;

    day = systemtime.wDay;
    month = systemtime.wMonth;
    year = systemtime.wYear;
#else
    struct _stat s;
    struct tm * p;

    if(_FILE_HANDLE_NULL == file_handle->handle)
    {
        ev_date_init(p_ev_date);
        return(create_error(FILE_ERR_BADTIMESTAMP));
    }

    (void) _fstat(file_handle->handle, &s);

    p = localtime(&s.st_mtime);

    seconds = p->tm_sec;
    minutes = p->tm_min;
    hours = p->tm_hour;

    day = p->tm_mday;
    month = p->tm_mon;
    year = p->tm_year;

    year += 1900;
    month += 1;
#endif
    } /*block*/

    (void) ss_ymd_to_dateval(&p_ev_date->date, year, month, day);
    (void) ss_hms_to_timeval(&p_ev_date->time, hours, minutes, seconds);

    return(STATUS_OK);
}

/******************************************************************************
*
* return the last error encountered on a given stream
* the error is not cleared -- use file_clearerr()
*
******************************************************************************/

_Check_return_
extern STATUS
file_error(
    _InoutRef_opt_ FILE_HANDLE file_handle)
{
    STATUS status;

    trace_1(TRACE_MODULE_FILE, TEXT("file_error(") PTR_XTFMT TEXT(")"), file_handle);

    if(!file_handle || (file_handle->magic != _FILE_MAGIC_WORD))
        return(create_error(FILE_ERR_BADHANDLE));

    file_handle->flags &= ~_FILE_EOFREAD;
    status = file_handle->error;

    trace_2(TRACE_MODULE_FILE, TEXT("file_error(") PTR_XTFMT TEXT(") returns error ") S32_TFMT, file_handle, status);
    return(status);
}

extern void
file_error_buffer_set(
    _In_reads_(error_bufelem) PTSTR error_buffer,
    _InVal_     U32 error_bufelem)
{
    file__errorbuffer = error_buffer;
    file__errorbufelem = error_bufelem;
}

/******************************************************************************
*
* read the current error associated with this module if any
* clears error if set; error message only valid until next call to module
*
******************************************************************************/

_Check_return_
_Ret_maybenull_z_
extern PCTSTR
file_error_get(void)
{
    PCTSTR errorstr;

    errorstr = file__errorptr;
    file__errorptr = NULL;

    return(errorstr);
}

/******************************************************************************
*
* remember errors to get back later
*
******************************************************************************/

_Check_return_
extern STATUS
file_error_set(
    _In_opt_z_  PCTSTR errorstr)
{
    if(errorstr  &&  !file__errorptr  &&  file__errorbuffer)
        tstr_xstrkpy(file__errorptr = file__errorbuffer, file__errorbufelem, errorstr);
#if TRACE_ALLOWED
    else if(file__errorptr)
        trace_0(TRACE_MODULE_FILE, TEXT("*** ERROR LOST ***"));
#endif

    return(create_error(FILE_ERR_ERROR_RQ));
}

extern void
file_finalise(void)
{
    FILE_HANDLE file_handle;

    trace_0(TRACE_MODULE_FILE, TEXT("file_finalise() -- closedown"));

    /* restart each time as file descriptors get deleted */
    while((file_handle = file_list) != _FILE_LIST_END)
        (void) t5_file_close(&file_handle);
}

/******************************************************************************
*
* ensure all data flushed to disc on this stream
*
******************************************************************************/

_Check_return_
extern STATUS
t5_file_flush(
    _InoutRef_opt_ FILE_HANDLE file_handle)
{
    STATUS status = STATUS_OK;

    trace_1(TRACE_MODULE_FILE, TEXT("t5_file_flush(") PTR_XTFMT TEXT(")"), file_handle);

    if(!file_handle || (file_handle->magic != _FILE_MAGIC_WORD))
        return(create_error(FILE_ERR_BADHANDLE));

    /* flush internal buffers to file */
    status_return(file__flushbuffer( file_handle, TEXT("t5_file_flush")));

    /* flush external buffers to file -- no need to flush closed files! */
    if(!(file_handle->flags & _FILE_CLOSED))
    {
#if RISCOS
        _kernel_swi_regs rs;

        rs.r[0] = OSArgs_Flush;
        rs.r[1] = file_handle->handle;

        status = file__obtain_error_string(_kernel_swi(OS_Args, &rs, &rs));
#elif WINDOWS
        /* no flushing to be done */
#else
        if(fflush(file_handle->handle))
            status = file__set_error(file_handle, create_error(FILE_ERR_CANTWRITE));
#endif
    }

    trace_2(TRACE_MODULE_FILE, TEXT("t5_file_flush(") PTR_XTFMT TEXT(") yields ") S32_TFMT, file_handle, status);
    return(status);
}

/******************************************************************************
*
* get a byte from stream
*
******************************************************************************/

_Check_return_
extern STATUS
file_getbyte(
    _InoutRef_opt_ FILE_HANDLE file_handle)
{
    if(!file_handle || (file_handle->magic != _FILE_MAGIC_WORD))
        return(create_error(FILE_ERR_BADHANDLE));

    return(file_getc(file_handle));
}

/******************************************************************************
*
* read the current seqptr of stream
*
******************************************************************************/

_Check_return_
extern STATUS
file_getpos(
    _InoutRef_opt_ FILE_HANDLE file_handle,
    _Out_       filepos_t * const p_cur_pos)
{
    trace_2(TRACE_MODULE_FILE, TEXT("file_getpos(") PTR_XTFMT TEXT(" to ") PTR_XTFMT TEXT(")"), file_handle, p_cur_pos);

    if(!file_handle || (file_handle->magic != _FILE_MAGIC_WORD))
    {
        p_cur_pos->u.u64 = 0;
        return(create_error(FILE_ERR_BADHANDLE));
    }

    return(file__getpos(file_handle, p_cur_pos));
}

/******************************************************************************
*
* read a LF/CR/LF,CR/CR,LF/EOF terminated line into a buffer
*
******************************************************************************/

_Check_return_
extern STATUS
file_gets(
    _Out_writes_z_(bufsize) char * buffer,
    _InVal_     U32 bufsize,
    _InoutRef_opt_ FILE_HANDLE file_handle)
{
    U32 usable_bufsize;
    U32 count = 0;

    if(bufsize < 2)
    {
        assert(bufsize >= 2);
        return(STATUS_OK);
    }

    usable_bufsize = bufsize - 1; /* always CH_NULL-terminated, so leave space */

    *buffer = CH_NULL;

    if(!file_handle || (file_handle->magic != _FILE_MAGIC_WORD))
        return(create_error(FILE_ERR_BADHANDLE));

    do  {
        U8 res, newres;
        STATUS status;

        status_return(status = file_getc(file_handle));

        /* EOF terminating a line is ok normally, especially if chars read */
        if(EOF_READ == status)
            return((count != 0) ? count : EOF_READ);

        res = (U8) status;

        if((res == LF)  ||  (res == CR))
        {
            /* got line terminator, read ahead */
            status_return(status = file_getc(file_handle));

            /* that EOF will terminate next line immediately */
            if(EOF_READ == status)
                return(count);

            newres = (U8) status;

            /* if not got alternate line terminator, put it back */
            if(res != (newres ^ LF ^ CR))
                status_return(file_ungetc(newres, file_handle));

            break;
        }

        buffer[count++] = res;
        buffer[count  ] = CH_NULL; /* keep terminated */
    }
    while(count < usable_bufsize);

    return(count);
}

#if RISCOS

_Check_return_
extern STATUS
file_get_risc_os_filetype(
    _InoutRef_opt_ FILE_HANDLE file_handle)
{
    if(!file_handle || (file_handle->magic != _FILE_MAGIC_WORD))
        return(create_error(FILE_ERR_BADHANDLE));

    return(file_handle->riscos.t5_filetype);
}

#endif /* RISCOS */

/******************************************************************************
*
* initialise module entry point
*
******************************************************************************/

extern void
file_startup(void)
{
    file__initialised = TRUE;
}

/******************************************************************************
*
* return the length of a file
*
******************************************************************************/

_Check_return_
extern STATUS
file_length(
    _InoutRef_opt_ FILE_HANDLE file_handle,
    _Out_       filelength_t * const p_file_length)
{
    STATUS status = STATUS_OK;

    p_file_length->u.u64 = 0;

    trace_1(TRACE_MODULE_FILE, TEXT("file_length(") PTR_XTFMT TEXT(")"), file_handle);

    if(!file_handle || (file_handle->magic != _FILE_MAGIC_WORD))
        return(create_error(FILE_ERR_BADHANDLE));

    /* ensure anything written to output buffer is included in the length */
    status_return(file__flushbuffer( file_handle, TEXT("file_length")));

    {
#if RISCOS
    _kernel_swi_regs rs;

    rs.r[0] = OSArgs_ReadEXT;
    rs.r[1] = file_handle->handle;

    if(status_ok(status = file__obtain_error_string(_kernel_swi(OS_Args, &rs, &rs))))
        p_file_length->u.words.lo = rs.r[2];
#elif WINDOWS
    BY_HANDLE_FILE_INFORMATION info;

    if(WrapOsBoolChecking(GetFileInformationByHandle(file_handle->handle, &info)))
    {
        p_file_length->u.words.lo = info.nFileSizeLow;
        p_file_length->u.words.hi = info.nFileSizeHigh;
    }
#else
    filepos_t cur_pos;

    status_return(file_getpos(file_handle, &cur_pos));

    if(status_ok(status = file_seek(file_handle, 0, NULL, SEEK_END)))
        status = file_tell(file_handle, &p_file_length->u.words.lo, &p_file_length->u.words.hi);

    status_accumulate(status, file_setpos(file_handle, &cur_pos));
#endif
    } /*block*/

    trace_2(TRACE_MODULE_FILE, TEXT("file_length(") PTR_XTFMT TEXT(") yields ") U64_TFMT, file_handle, p_file_length->u.u64);
    return(status);
}

/******************************************************************************
*
* open a file:
*
* --out--
*
*   -ve:   error in open
*     0:   file could not be opened
*   +ve:   file opened
*
******************************************************************************/

_Check_return_
extern STATUS
t5_file_open(
    _In_z_      PCTSTR filename,
    _InVal_     file_open_mode openmode,
    _OutRef_    P_FILE_HANDLE p_file_handle,
    _In_        BOOL must_open /* forced TRUE iff write */)
{
#if RISCOS
    static const int
    openatts[] =
    {
        OSFind_OpenRead     | OSFind_UseNoPath | OSFind_EnsureNoDir,
        OSFind_CreateUpdate | OSFind_UseNoPath,
        OSFind_OpenUpdate   | OSFind_UseNoPath
    };
#elif WINDOWS
    static const DWORD
    openaccess[] =
    {
        GENERIC_READ,
        GENERIC_READ | GENERIC_WRITE,
        GENERIC_READ | GENERIC_WRITE
    };
    static const DWORD
    openshare[] =
    {
        FILE_SHARE_READ,
        0,
        0
    };
    static const DWORD
    opencreate[] =
    {
        OPEN_EXISTING,
        CREATE_ALWAYS,
        OPEN_EXISTING
    };
#else
    static const char *
    openatts[] =
    {
        "rb",
        "w+b",
        "r+b"
    };
#endif /* OS */

    FILE_HANDLE file_handle;
    STATUS status;

    *p_file_handle = 0;

#if WINDOWS
    if((U32) openmode >= elemof32(openaccess))
        return(status_check());
#else
    if((U32) openmode >= elemof32(openatts))
        return(status_check());
#endif

    if(openmode == file_open_write)
        must_open = TRUE;

    trace_3(TRACE_MODULE_FILE, TEXT("t5_file_open(%s, ") S32_TFMT TEXT(", ") PTR_XTFMT TEXT(")"), filename, openmode, p_file_handle);

    /* would the name be too long to put in the allocated structure? */
#if RISCOS
    if(tstrlen32(filename) >= sizeof32(file_handle->riscos.filename))
        return(create_error(FILE_ERR_NAMETOOLONG));
#endif

    if(NULL == (*p_file_handle = file_handle = al_ptr_calloc_elem(_FILE_HANDLE, 1, &status)))
        return(status);

    status = STATUS_DONE; /* not STATUS_OK */

    /* always initialise file to look silly */
    file_handle->count = -1;  /* nothing buffered; forces getc/putc to fns */
    file_handle->flags = (openmode == file_open_read)
                          ?  _FILE_READ
                          : (_FILE_READ | _FILE_WRITE);
    file_handle->magic = _FILE_MAGIC_WORD;

    file_handle->bufsize = file__defbufsiz; /* to start with, but no buffer initially allocated */

#if RISCOS
    {
    _kernel_swi_regs rs;

    (void) strcpy(file_handle->riscos.filename, filename);

    file_handle->riscos.t5_filetype = FILETYPE_UNTYPED;

    rs.r[0] = OSFile_ReadNoPath; /* SKS after 1.05 25oct93 - see if this works better than OSFile_ReadInfo wrt disc swaps on A3000/RISC OS 3.10 */
    rs.r[1] = (int) filename;

    if(0 == _kernel_swi(OS_File, &rs, &rs)) /* ignore errors (especially for device streams) */
    {
        switch(rs.r[0])
        {
        case OSFile_ObjectType_None:
            if(openmode != file_open_write)
                status = 0;
            break;

        case OSFile_ObjectType_File:
            {
            if(openmode == file_open_read)
            {
                if((rs.r[5] & OSFile_ObjectAttribute_read) == 0)
                    status = create_error(FILE_ERR_NO_ACCESS_READ);
            }
            else
            {
                if((rs.r[5] & OSFile_ObjectAttribute_locked) != 0)
                    status = create_error(FILE_ERR_LOCKED);
                else if((rs.r[5] & OSFile_ObjectAttribute_write) == 0)
                    status = create_error(FILE_ERR_NO_ACCESS_WRITE);
            }

            if(status_ok(status))
            {
                file_handle->riscos.t5_filetype = (T5_FILETYPE) (((U32) rs.r[2] >> 8) & 0xFFFU);

                file_handle->riscos.file_date_loword = rs.r[3];
                file_handle->riscos.file_date_hiword = rs.r[2] & 0xFF;
            }

            break;
            }

        default: default_unhandled();
#if CHECKING
        case OSFile_ObjectType_Dir:
        case OSFile_ObjectType_Image:
#endif
            rs.r[2] = rs.r[0];
            rs.r[0] = OSFile_MakeError;
            status = file__obtain_error_string(_kernel_swi(OS_File, &rs, &rs));
            break;
        }
    }

    if(status == 1)
    {
        rs.r[0] = openatts[openmode];
        rs.r[1] = (int) filename;

        if(status_ok(status = file__obtain_error_string(_kernel_swi(OS_Find, &rs, &rs))))
        {
            file_handle->handle = rs.r[0];
            status = (_FILE_HANDLE_NULL != file_handle->handle);
        }
    }
    } /*block*/
#elif WINDOWS
    file_handle->handle = CreateFile(filename, openaccess[openmode], openshare[openmode], NULL, opencreate[openmode], FILE_ATTRIBUTE_NORMAL, NULL);

    status = (_FILE_HANDLE_NULL != file_handle->handle);

    if(status == 1)
    {
        BY_HANDLE_FILE_INFORMATION info;

        GetFileInformationByHandle(file_handle->handle, &info);

        file_handle->windows.file_date = info.ftLastWriteTime;
    }
#else
    file_handle->handle = fopen(filename, openatts[openmode]);

    status = (_FILE_HANDLE_NULL != file_handle->handle);
#endif

    if((status == 0)  &&  must_open)
        status = create_error(FILE_ERR_CANTOPEN);

    if(status <= 0)
        al_ptr_dispose(P_P_ANY_PEDANTIC(p_file_handle));
    else
    {
        if(!file__initialised)
            file_startup();

        /* link file onto head of list */
        file_handle->next = file_list;
        file_list = file_handle;
    }

    trace_2(TRACE_MODULE_FILE, TEXT("t5_file_open yields ") PTR_XTFMT TEXT(", ") S32_TFMT, file_handle, status);
    return(status);
}

/******************************************************************************
*
* pad bytes out from seqptr to a power of two on stream
*
******************************************************************************/

_Check_return_
extern STATUS
file_pad(
    _InoutRef_opt_ FILE_HANDLE file_handle,
    _InVal_     U32 alignpower)
{
    U32 alignment, alignmask, res32;
    filepos_t cur_pos;

    if(0 == alignpower)
        return(STATUS_OK);

    alignment = (U32) 1U << alignpower;
    alignmask = alignment - 1;

    status_return(file_getpos(file_handle, &cur_pos)); /* validates file */

    res32 = cur_pos.u.words.lo; /* can safely ignore cur_pos.hi as we're only trying to align to a small boundary */
    assert(alignpower < 32);

    __assume(file_handle);

    trace_6(TRACE_MODULE_FILE,
            TEXT("file_pad(") PTR_XTFMT TEXT(", ") U32_TFMT TEXT("): alignment ") U32_TFMT TEXT(", mask ") U32_XTFMT TEXT(", pos ") U32_XTFMT TEXT(", needs ") U32_TFMT,
            file_handle, alignpower, alignment, alignmask, res32,
            ((res32 & alignmask) ? (alignment - (res32 & alignmask)) : 0));

    if(res32 & alignmask)
    {
        alignment = alignment - (res32 & alignmask);
        do  {
            trace_0(TRACE_MODULE_FILE, TEXT("file_pad outputting CH_NULL"));
            status_return(file_putc(CH_NULL, file_handle));
        }
        while(--alignment != 0);
    }

    return(STATUS_OK);
}

/******************************************************************************
*
* With buffered getc/putc we too make the restriction (a la ANSI)
* that getc and putc on the same stream are separated by a flush
* or a seek operation (use seek(file_handle, 0, SEEK_CUR))
*
******************************************************************************/

_Check_return_
extern STATUS
file_putbyte(
    _InVal_     STATUS c,
    _InoutRef_opt_ FILE_HANDLE file_handle)
{
    if(!file_handle || (file_handle->magic != _FILE_MAGIC_WORD))
        return(create_error(FILE_ERR_BADHANDLE));

    return(file_putc(c, file_handle));
}

#if defined(UNUSED_KEEP_ALIVE)

_Check_return_ _Success_(status_ok(return))
extern STATUS
file_read(
    P_ANY ptr,
    _InVal_     U32 size,
    _InVal_     U32 nmemb,
    _InoutRef_opt_ FILE_HANDLE file_handle)
{
    U32 bytesread;
    U32 membersread;

    if(!file_handle || (file_handle->magic != _FILE_MAGIC_WORD))
        return(create_error(FILE_ERR_BADHANDLE));

    /* trivial case? */
    if(!size  ||  !nmemb)
        return(0);

#if WINDOWS
    myassert4x(((U64) size * (U64) nmemb) <= U32_MAX, TEXT("file_read(") PTR_XTFMT TEXT(", ") U32_TFMT TEXT(", ") U32_TFMT TEXT(", ") PTR_XTFMT TEXT("): integer overflow size * nmemb"), ptr, size, nmemb, file_handle);
#endif

    /* always perform read ops from file, so lose buffer, reposition seqptr */
    status_return(file__flushbuffer(file_handle, TEXT("file_read")));

    status_return(file__read(ptr, size * nmemb, &bytesread, file_handle));

    membersread = /*(size == 1) ? bytesread :*/ (bytesread / size);

    trace_1(TRACE_MODULE_FILE, TEXT("file_read read ") U32_TFMT TEXT(" members"), (U32) membersread);
    status_assert((STATUS) membersread);
    return((STATUS) membersread);
}

#endif /* UNUSED_KEEP_ALIVE */

_Check_return_ _Success_(status_ok(return))
extern STATUS
file_read_bytes(
    _Out_writes_bytes_(bytestoread) P_ANY ptr,
    _InVal_     U32 bytestoread,
    _OutRef_    P_U32 p_bytesread,
    _InoutRef_opt_ FILE_HANDLE file_handle)
{
    *p_bytesread = 0;

    if(!file_handle || (file_handle->magic != _FILE_MAGIC_WORD))
        return(create_error(FILE_ERR_BADHANDLE));

    /* trivial case? */
    if(0 == bytestoread)
        return(0);

    /* always perform read ops from file, so lose buffer, reposition seqptr */
    status_return(file__flushbuffer(file_handle, TEXT("file_read_bytes")));

    return(file__read(ptr, bytestoread, p_bytesread, file_handle));
}

_Check_return_ _Success_(status_ok(return))
extern STATUS
file_read_bytes_requested(
    _Out_writes_bytes_(bytestoread) P_ANY ptr,
    _InVal_     U32 bytestoread,
    _InoutRef_opt_ FILE_HANDLE file_handle)
{
    U32 bytesread;
    STATUS status;

    status_return(status = file_read_bytes(ptr, bytestoread, &bytesread, file_handle));

    if(bytestoread != bytesread)
    {
        assert(bytestoread == bytesread);
        return(create_error(FILE_ERR_CANTREADREQUESTED));
    }

    return(status);
}

_Check_return_
extern STATUS
file_remove(
    _In_z_      PCTSTR filename)
{
#if RISCOS
    _kernel_swi_regs rs;
    rs.r[0] = OSFile_Delete;
    rs.r[1] = (int) filename;
    return(file__obtain_error_string(_kernel_swi(OS_File, &rs, &rs)));
#elif WINDOWS
    if(!WrapOsBoolChecking(DeleteFile(filename)))
        return(FILE_ERR_REMOVE_FAILED);

    return(STATUS_OK);
#endif /*OS */
}

_Check_return_
extern STATUS
file_rename(
    _In_z_      PCTSTR filename_from,
    _In_z_      PCTSTR filename_to)
{
#if RISCOS
    _kernel_swi_regs rs;
    rs.r[0] = OSFSControl_Rename;
    rs.r[1] = (int) filename_from;
    rs.r[2] = (int) filename_to;
    return(file__obtain_error_string(_kernel_swi(OS_FSControl, &rs, &rs)));
#elif WINDOWS
    if(!WrapOsBoolChecking(MoveFile(filename_from, filename_to)))
        return(FILE_ERR_RENAME_FAILED);

    return(STATUS_OK);
#endif /*OS */
}

/******************************************************************************
*
* sets the file position indicator for the stream pointed to by stream to
* the beginning of the file. It is equivalent to
*          file_seek(stream, 0, SEEK_SET)
* except that the error indicator for the stream is also cleared.
*
******************************************************************************/

_Check_return_
extern STATUS
file_rewind(
    _InoutRef_opt_ FILE_HANDLE file_handle)
{
    if(!file_handle || (file_handle->magic != _FILE_MAGIC_WORD))
        return(create_error(FILE_ERR_BADHANDLE));

    /* stay close to ANSI */
    status_consume(file_clearerror(file_handle));

    status_return(file__flushbuffer(file_handle, TEXT("file_rewind")));

    return(file__seek(file_handle, 0, SEEK_SET));
}

/******************************************************************************
*
* sets the file position indicator for the stream pointed to by f.
* The new position is at the signed number of characters specified
* by offset away from the point specified by origin.
* The specified point is the beginning of the file for SEEK_SET, the
* current position in the file for SEEK_CUR, or end-of-file for SEEK_END.
* After a file_seek call, the next operation on an update stream
* may be either input or output.
*
******************************************************************************/

_Check_return_
extern STATUS
file_seek(
    _InoutRef_opt_ FILE_HANDLE file_handle,
    _InVal_     S32 offset_lo,
    _InRef_opt_ PC_S32 p_offset_hi,
    _InVal_     S32 origin)
{
    S64 offset64;

    if(!file_handle || (file_handle->magic != _FILE_MAGIC_WORD))
        return(create_error(FILE_ERR_BADHANDLE));

    if(NULL != p_offset_hi)
    {   /* DON'T sign-extend the low word */
        offset64 = (S64) (((U64) (*p_offset_hi) << 32) | (U32) offset_lo);
    }
    else
    {   /* DO sign-extend the low word */
        offset64 = (S64) offset_lo;
    }

    /* always lose buffer as a subsequent file_putc will need to
     * call file__flsbuf in order to set BUFDIRTY. Yes, it may be
     * tempting to reposition ptr within the buffer but DON'T DO IT!
    */
    status_return(file__flushbuffer(file_handle, TEXT("file_seek")));

    return(file__seek(file_handle, offset64, origin));
}

/******************************************************************************
*
* restore a saved (from file_getpos) seqptr on stream
*
******************************************************************************/

_Check_return_
extern STATUS
file_setpos(
    _InoutRef_opt_ FILE_HANDLE file_handle,
    _In_        const filepos_t * const pos)
{
    if(!file_handle || (file_handle->magic != _FILE_MAGIC_WORD))
        return(create_error(FILE_ERR_BADHANDLE));

    status_return(file__flushbuffer(file_handle, TEXT("file_setpos")));

    return(file__seek(file_handle, (S64) pos->u.u64, SEEK_SET));
}

#if RISCOS

/******************************************************************************
*
* set the file type of a stream (written on closing)
*
******************************************************************************/

_Check_return_
extern STATUS
file_set_risc_os_filetype(
    _InoutRef_opt_ FILE_HANDLE file_handle,
    _InVal_     T5_FILETYPE t5_filetype)
{
    if(!file_handle || (file_handle->magic != _FILE_MAGIC_WORD))
        return(create_error(FILE_ERR_BADHANDLE));

    file_handle->riscos.t5_filetype = t5_filetype;

    return(STATUS_OK);
}

#endif /* RISCOS */

_Check_return_
extern STATUS
file_tell(
    _InoutRef_opt_ FILE_HANDLE file_handle,
    _OutRef_    P_U32 p_cur_pos_lo,
    _OutRef_    P_U32 p_cur_pos_hi)
{
    filepos_t cur_pos;

    *p_cur_pos_lo = *p_cur_pos_hi = 0;

    if(!file_handle || (file_handle->magic != _FILE_MAGIC_WORD))
        return(create_error(FILE_ERR_BADHANDLE));

    /* return result either deduced from offset in buffer or real seq ptr */
    status_return(file__getpos(file_handle, &cur_pos));

    *p_cur_pos_lo = cur_pos.u.words.lo;
    *p_cur_pos_hi = cur_pos.u.words.hi;

    return(STATUS_OK);
}

/******************************************************************************
*
* one level of ungetc provided
*
******************************************************************************/

_Check_return_
extern STATUS
file_ungetc(
    _InVal_     STATUS c,
    _InoutRef_opt_ FILE_HANDLE file_handle)
{
    if(!file_handle || (file_handle->magic != _FILE_MAGIC_WORD))
        return(create_error(FILE_ERR_BADHANDLE));

    if(EOF_READ != c)
    {
        /* lose any buffer on this file */
        status_return(file__flushbuffer(file_handle, TEXT("file_ungetc")));

        /* step file ptr back in file one byte -- ANSI */
        status_return(file__seek(file_handle, -1, SEEK_CUR));

        file_handle->flags |= _FILE_HASUNGOTCH;
        file_handle->ungotch = c;
    }

    return(c);
}

/******************************************************************************
*
* return error if file handle not valid
*
******************************************************************************/

_Check_return_
extern STATUS
file_validate(
    _InoutRef_opt_ FILE_HANDLE file_handle);

_Check_return_
extern STATUS
file_validate(
    _InoutRef_opt_ FILE_HANDLE file_handle)
{
    if(!file_handle || (file_handle->magic != _FILE_MAGIC_WORD))
        return(create_error(FILE_ERR_BADHANDLE));

    return(STATUS_OK);
}

#if defined(UNUSED_KEEP_ALIVE)

/******************************************************************************
*
* writes, from the array pointed to by ptr up to nmemb members whose size
* is specified by size, to the file_handler specified by f. The file
* position indicator is advanced by the number of characters
* successfully written. If an error occurs, the resulting value of the file
* position indicator is indeterminate.
*
******************************************************************************/

_Check_return_
extern STATUS
file_write(
    _In_reads_bytes_x_(size*nmemb) const void * ptr,
    _InVal_     U32 size,
    _InVal_     U32 nmemb,
    _InoutRef_opt_ FILE_HANDLE file_handle)
{
    U32 bytestowrite;

    /* trivial call? */
    if(!size  ||  !nmemb)
        return(STATUS_OK);

#if WINDOWS
    assert(((U64) size * (U64) nmemb) <= U32_MAX);
#endif

    bytestowrite = size * nmemb;

    return(file_write_bytes(ptr, bytestowrite, file_handle));
}

#endif /* UNUSED_KEEP_ALIVE */

_Check_return_
extern STATUS
file_write_bytes(
    _In_reads_bytes_(bytestowrite) PC_ANY ptr,
    _InVal_     U32 bytestowrite,
    _InoutRef_opt_ FILE_HANDLE file_handle)
{
    U32 byteslefttowrite, bytestobuffer;

    if(!file_handle || (file_handle->magic != _FILE_MAGIC_WORD))
        return(create_error(FILE_ERR_BADHANDLE));

    /* trivial call? */
    if(0 == bytestowrite)
        return(STATUS_OK);

    byteslefttowrite = bytestowrite;

    /* if there's a buffer, write as much as we can to it */
    if(file_handle->count != -1)
    {
        bytestobuffer = MIN((U32) file_handle->count, byteslefttowrite);

        file_handle->count -= bytestobuffer;
        memcpy32(file_handle->ptr, ptr, bytestobuffer);
        file_handle->ptr += bytestobuffer;

        byteslefttowrite -= bytestobuffer;

        if(0 == byteslefttowrite)
            return(STATUS_OK); /* it did all fit in the current buffer */

        /* simple implementation: must flush out buffered data as we're going
         * to write direct to filing system at seqptr
        */
        status_return(file__flushbuffer(file_handle, TEXT("file_write_bytes")));

        /* next data to write will come from here */
        ptr = PtrAddBytes(PC_ANY, ptr, bytestobuffer);
    }

    if(0 == file_handle->bufsize)
    {
        /* it will all have to be written directly */
        bytestobuffer = 0;
    }
    else
    {
        /* will it all fit in one buffer? */
        if(byteslefttowrite < file_handle->bufsize)
        {
            bytestobuffer = byteslefttowrite;
            byteslefttowrite = 0;
        }
        else
        {
            /* write out as much as possible now, followed by possibly buffering remainder */
            U32 directbytes = byteslefttowrite;

            directbytes /= file_handle->bufsize;
            directbytes *= file_handle->bufsize;

            bytestobuffer = byteslefttowrite - directbytes;
            byteslefttowrite = directbytes;
        }
    }

    if(byteslefttowrite)
    {
        status_return(file__write(ptr, byteslefttowrite, file_handle));

        /* next data to write will come from here */
        ptr = PtrAddBytes(PC_ANY, ptr, byteslefttowrite);
    }

    if(bytestobuffer)
    {
        assert(file_handle->count == -1);
        status_return(file__make_output_buffer(file_handle));

        assert((U32) file_handle->count >= bytestobuffer);
        file_handle->count -= bytestobuffer;
        memcpy32(file_handle->ptr, ptr, bytestobuffer);
        file_handle->ptr += bytestobuffer;
    }

    return(STATUS_OK);
}

/******************************************************************************
*
* internal routines
*
******************************************************************************/

/******************************************************************************
*
* close file
*
******************************************************************************/

_Check_return_
static STATUS
file__closefile(
    _InoutRef_  FILE_HANDLE file_handle)
{
    STATUS status;

    trace_1(TRACE_MODULE_FILE, TEXT("file__closefile(") PTR_XTFMT TEXT(")"), file_handle);

    if(file_handle->flags & _FILE_CLOSED)
        /* no need to do anything; already closed on fs */
        return(STATUS_OK);

    status = t5_file_flush(file_handle);

    {
#if RISCOS
    _kernel_swi_regs rs;
    STATUS status1 = STATUS_OK;

    rs.r[0] = 0;
    rs.r[1] = file_handle->handle;

    if(status_ok(status1 = file__obtain_error_string(_kernel_swi(OS_Find, &rs, &rs))))
    {
        if(file_handle->written_to)
        {
            /* read current time UTC (really does work!) */
            int buffer[2];
            rs.r[0] = 14;
            rs.r[1] = (int) buffer;
            buffer[0] = 3;
            void_WrapOsErrorChecking(_kernel_swi(OS_Word, &rs, &rs));
            file_handle->riscos.file_date_loword = buffer[0];
            file_handle->riscos.file_date_hiword = buffer[1] & 0xFF;

            rs.r[0] = OSFile_WriteLoad;
            rs.r[1] = (int) file_handle->riscos.filename;
            rs.r[2] = 0xFFF00000 | ((int) file_handle->riscos.t5_filetype << 8) | file_handle->riscos.file_date_hiword;
            status1 = file__obtain_error_string(_kernel_swi(OS_File, &rs, &rs));

            if(status_ok(status1))
            {
                rs.r[0] = OSFile_WriteExec;
                rs.r[1] = (int) file_handle->riscos.filename;
                /* not r[2] */
                rs.r[3] = file_handle->riscos.file_date_loword;
                status1 = file__obtain_error_string(_kernel_swi(OS_File, &rs, &rs));
            }
        }
    }

    if(status_ok(status))
        status = status1;
#elif WINDOWS
    /* update this near to the close */
    BY_HANDLE_FILE_INFORMATION info;

    GetFileInformationByHandle(file_handle->handle, &info);

    file_handle->windows.file_date = info.ftLastWriteTime;

    if(!CloseHandle(file_handle->handle))
        status = create_error(FILE_ERR_CANTCLOSE);
#else
    if(fclose(file_handle->handle))
        status = create_error(FILE_ERR_CANTCLOSE);
#endif
    } /*block*/

    file_handle->handle = _FILE_HANDLE_NULL; /* SKS after 1.12 01nov94 */

    file_handle->flags |= _FILE_CLOSED;

    return(status);
}

/******************************************************************************
*
* fill a buffer on a file from the filing system at seqptr
* only called if nothing in buffer (count = -1)
*
******************************************************************************/

_Check_return_
static STATUS
file__fillbuffer(
    _InoutRef_  FILE_HANDLE file_handle)
{
    STATUS status;
    U32 bytesread;

    /* note where buffer belongs in the file */
    status_return(file__getpos(file_handle, &file_handle->bufpos));

    /* try to read as many bytes as possible */
    status = file__read(file_handle->bufbase, file_handle->bufsize, &bytesread, file_handle);

    if(status_fail(status))
    {
        file_handle->count = -1;
        file_handle->flags |=  _FILE_EOFREAD;
        return(status);
    }

    if(0 == bytesread)
    {
        file_handle->count = -1;
        file_handle->flags |=  _FILE_EOFREAD;
        return(EOF_READ); /* MUST be picked up by caller! */
    }

    if(bytesread != file_handle->bufsize)
        /* only set if EOF been read in this buffer */
        file_handle->flags |=  _FILE_EOFREAD;
    else
        file_handle->flags &= ~_FILE_EOFREAD;

    file_handle->count = (S32) bytesread; /* doesn't matter if bytesread < bufsize */
    file_handle->ptr = (P_U8) file_handle->bufbase;
    file_handle->bufbytes = (U32) bytesread;

    return(STATUS_OK);
}

/******************************************************************************
*
* if a file is buffered and bytes have been written to it then flush them to
* the filing system, but update seqptr in any case. file must be valid and open
*
******************************************************************************/

_Check_return_
static STATUS
#if TRACE_ALLOWED
file__flushbuffer(
    _InoutRef_  FILE_HANDLE file_handle,
    _In_z_      PCTSTR caller)
#else
file___flushbuffer(
    _InoutRef_  FILE_HANDLE file_handle)
#endif
{
    STATUS status = STATUS_OK;

    if(file_handle->error)
    {
        /* sticky error for ANSI closeness */
        trace_1(TRACE_MODULE_FILE, TEXT("file__flushbuffer returns sticky error ") S32_TFMT, file_handle->error);
        return(file_handle->error);
    }

    if(file_handle->count != -1)
    {
        /* we have a buffer, determine usage and dirtiness */
        U32 bytesused;
        
        trace_2(TRACE_MODULE_FILE, TEXT("file__flushbuffer(") PTR_XTFMT TEXT(") called by %s"), file_handle, caller);

        bytesused = file_handle->bufbytes - (U32) file_handle->count;

        /* zap buffer */
        file_handle->count = -1;

        if(bytesused)
        {
            if(file_handle->flags & _FILE_BUFDIRTY)
            {
                /* write used portion of buffer out to filing system
                 * at the correct place, updating seqptr
                */
                file_handle->flags &= ~_FILE_BUFDIRTY;
                status = file__write(file_handle->bufbase, bytesused, file_handle);
            }
            else
            {
                /* update filing system seqptr to the point
                 * at which the client has used the read-only buffer
                */
                filepos_t new_pos;
                new_pos.u.u64 = file_handle->bufpos.u.u64 + bytesused;
                status = (STATUS) file__seek(file_handle, (S64) new_pos.u.u64, SEEK_SET);
            }
        }
    }

#if TRACE_ALLOWED
    if(status_fail(status))
        trace_3(TRACE_MODULE_FILE, TEXT("*** file__flushbuffer(") PTR_XTFMT TEXT(") from %s returning error ") S32_TFMT TEXT(" ***"), file_handle, caller, status);
#endif

    return(status);
}

/******************************************************************************
*
* return position of seqptr.
*
* --in--
*
*   all parameters must be valid, file open on filing system
*
* --out--
*
*   <  0  error condition
*
******************************************************************************/

_Check_return_
static STATUS
file__getpos(
    _InoutRef_  FILE_HANDLE file_handle,
    _Out_       filepos_t * const p_cur_pos)
{
    filepos_t cur_pos = { 0 };

    /* answer easy if buffered:   pos of buffer in file
     *                          + n bytes actually buffered
     *                          - n bytes still to be used
    */
    if(file_handle->count != -1)
    {
        U32 offset_in_buffer = file_handle->bufbytes - (U32) file_handle->count;
        cur_pos.u.u64 = file_handle->bufpos.u.u64 + offset_in_buffer;
        *p_cur_pos = cur_pos;
        trace_2(TRACE_MODULE_FILE, TEXT("file__getpos(") PTR_XTFMT TEXT(") yields ") U64_TFMT, file_handle, cur_pos.u.u64);
        return(STATUS_OK);
    }

    p_cur_pos->u.words.lo = 0;
    p_cur_pos->u.words.hi = 0;

    { /* must ask filing system */
#if RISCOS
    _kernel_swi_regs rs;

    rs.r[0] = OSArgs_ReadPTR;
    rs.r[1] = file_handle->handle;

    if(status_fail(file__obtain_error_string(_kernel_swi(OS_Args, &rs, &rs))))
        return(file__set_error(file_handle, create_error(FILE_ERR_CANTREAD)));

    cur_pos.u.words.lo = rs.r[2];
#elif WINDOWS
    LARGE_INTEGER liDistanceToMove = { 0 };
    LARGE_INTEGER liCurrentFilePointer = { 0 };

    if(!WrapOsBoolChecking(SetFilePointerEx(file_handle->handle, liDistanceToMove, &liCurrentFilePointer, FILE_CURRENT)))
        return(file__set_error(file_handle, create_error(FILE_ERR_CANTREAD)));

    cur_pos.u.u64 = (U64) liCurrentFilePointer.QuadPart;
#else
    cur_pos.u.words.lo = ftell(file_handle->handle);

    if(cur_pos.u.words.lo == (U32) EOF)
        return(file__set_error(file_handle, create_error(FILE_ERR_CANTREAD)));
#endif
    } /*block*/

    *p_cur_pos = cur_pos;

    trace_2(TRACE_MODULE_FILE, TEXT("file__getpos(") PTR_XTFMT TEXT(") yields ") U64_TFMT, file_handle, cur_pos.u.u64);
    return(STATUS_OK);
}

_Check_return_
static STATUS
file__make_output_buffer(
    _InoutRef_  FILE_HANDLE file_handle)
{
    /* writeable file? */
    if(!(file_handle->flags & _FILE_WRITE))
        return(file__set_error(file_handle, create_error(FILE_ERR_ACCESSDENIED)));

    /* no buffer present? */
    if(!file_handle->bufbase)
    {
        assert(file_handle->bufsize);
        status_return(file_buffer(file_handle, NULL, file_handle->bufsize));
        assert(file_handle->bufsize);
    }

    /* buffer is empty, make it so we can write to it,
     * noting where it belongs in the file
    */
    status_return(file__getpos(file_handle, &file_handle->bufpos));

    file_handle->flags |= _FILE_BUFDIRTY; /* it is now dirty */
    file_handle->bufbytes = file_handle->bufsize; /* bytes of buffer available in total */

    file_handle->count = file_handle->bufbytes; /* bytes of buffer still available */
    file_handle->ptr = (P_U8) file_handle->bufbase;

    return(STATUS_OK);
}

#if RISCOS

/******************************************************************************
*
* if an os call yielded an error, try to remember it
*
******************************************************************************/

_Check_return_
static STATUS
file__obtain_error_string(
    _In_opt_    const _kernel_oserror * const e)
{
    return(e ? file_error_set(e->errmess) : STATUS_OK);
}

#endif /* RISCOS */

/******************************************************************************
*
* read a buffer from file at a given position. updates seqptr
*
* --in--
*
*   all parameters must be valid, file open
*
* --out--
*
*   <  0  error condition
*
******************************************************************************/

_Check_return_
static STATUS
file__read(
    _Out_writes_bytes_(bytestoread_in) P_ANY ptr,
    _InVal_     U32 bytestoread_in,
    _OutRef_    P_U32 p_bytesread,
    _InoutRef_  FILE_HANDLE file_handle)
{
    U32 bytestoread = bytestoread_in;

    *p_bytesread = 0;

    trace_4(TRACE_MODULE_FILE, TEXT("file__read(") PTR_XTFMT TEXT(", ") U32_TFMT TEXT(", ") PTR_XTFMT TEXT(") <- handle ") UINTPTR_XTFMT, ptr, bytestoread, file_handle, (uintptr_t) file_handle->handle);

    /* sticky error for ANSI closeness */
    if(file_handle->error)
    {
        trace_1(TRACE_MODULE_FILE, TEXT("file__read returns sticky error ") S32_TFMT, file_handle->error);
        return(file_handle->error);
    }

    {
#if RISCOS
    _kernel_osgbpb_block blk;

    blk.dataptr = ptr;
    blk.nbytes  = (int) bytestoread;

    if(_kernel_ERROR == _kernel_osgbpb(OSGBPB_ReadFromPTR, file_handle->handle, &blk))
    {
        return(file__obtain_error_string(_kernel_last_oserror()));
    }

    *p_bytesread = bytestoread - blk.nbytes;
#elif WINDOWS
    if(!ReadFile(file_handle->handle, ptr, bytestoread, (LPDWORD) p_bytesread, NULL))
        return(file__set_error(file_handle, create_error(FILE_ERR_CANTREAD)));
#else
    *p_bytesread = fread(ptr, 1, bytestoread, file_handle->handle);
#endif
    } /*block*/

    trace_1(TRACE_MODULE_FILE, TEXT("file__read read ") U32_TFMT TEXT(" bytes"), *p_bytesread);
    return(STATUS_OK);
}

/******************************************************************************
*
* internal seek; doesn't flush buffers
* assumed done prior to this
* file must be valid and open
*
******************************************************************************/

_Check_return_
static STATUS
file__seek(
    _InoutRef_  FILE_HANDLE file_handle,
    _InVal_     S64 offset64,
    _InVal_     S32 origin)
{
    STATUS status = STATUS_OK;
    filepos_t new_pos = { 0 };

    file_handle->flags &= ~(_FILE_EOFREAD | _FILE_HASUNGOTCH);

    {
#if RISCOS
    _kernel_swi_regs rs;
    filepos_t cur_pos = { 0 };

    switch(origin)
    {
    default: default_unhandled();
#if CHECKING
    case SEEK_SET:
#endif
        /* moving to given position */
        if(offset64 < 0) /* as per ERROR_NEGATIVE_SEEK */
            return(file__set_error(file_handle, create_error(FILE_ERR_INVALIDPOSITION)));
        new_pos.u.u64 = (U64) offset64;
        break;

    case SEEK_CUR:
        if(status_fail(status = file__getpos(file_handle, &cur_pos)))
            return(file__set_error(file_handle, status));

        if(offset64 > 0)
        {   /* moving forwards from current position */
            U64 pos_offset64 = (U64) offset64;
            new_pos.u.u64 = cur_pos.u.u64 + pos_offset64;
            /* check for overflow */
            if((new_pos.u.u64 < cur_pos.u.u64) || (new_pos.u.u64 < pos_offset64))
                return(file__set_error(file_handle, create_error(FILE_ERR_INVALIDPOSITION)));
        }
        else if(offset64 < 0)
        {   /* moving backwards from current position */
            U64 pos_offset64 = (U64) - offset64;
            /* check for underflow(trying to move before start of file) */
            if(cur_pos.u.u64 < pos_offset64) /* as per ERROR_NEGATIVE_SEEK */
                return(file__set_error(file_handle, create_error(FILE_ERR_INVALIDPOSITION)));
            new_pos.u.u64 = cur_pos.u.u64 - pos_offset64;
        }
        else /*if(0 == offset64)*/
            new_pos = cur_pos;
        break;

    case SEEK_END:
        if(status_fail(status = file_length(file_handle, &cur_pos)))
            return(file__set_error(file_handle, status));

        if(offset64 > 0)
        {   /* moving forwards beyond current end of file -> file will grow */
            U64 pos_offset64 = (U64) offset64;
            new_pos.u.u64 = cur_pos.u.u64 + pos_offset64;
            /* check for overflow */
            if((new_pos.u.u64 < cur_pos.u.u64) || (new_pos.u.u64 < pos_offset64))
                return(file__set_error(file_handle, create_error(FILE_ERR_INVALIDPOSITION)));
        }
        else if(offset64 < 0)
        {   /* moving backwards to a point within the file */
            U64 pos_offset64 = (U64) - offset64;
            /* check for underflow (trying to move before start of file) */
            if(cur_pos.u.u64 < pos_offset64) /* as per ERROR_NEGATIVE_SEEK */
                return(file__set_error(file_handle, create_error(FILE_ERR_INVALIDPOSITION)));
            new_pos.u.u64 = cur_pos.u.u64 - pos_offset64;
        }
        break;
    }

    assert((0 == new_pos.u.words.hi) || (-1 == new_pos.u.words.hi));

    rs.r[0] = OSArgs_SetPTR;
    rs.r[1] = file_handle->handle;
    rs.r[2] = (int) new_pos.u.words.lo;

    status = file__obtain_error_string(_kernel_swi(OS_Args, &rs, &rs));
#elif WINDOWS
    LARGE_INTEGER liDistanceToMove;
    LARGE_INTEGER liNewFilePointer;
    DWORD dwMoveMethod;

    liDistanceToMove.QuadPart = offset64;

    switch(origin)
    {
    default: default_unhandled();
#if CHECKING
    case SEEK_SET:
#endif
        dwMoveMethod = FILE_BEGIN;
        break;

    case SEEK_CUR:
        dwMoveMethod = FILE_CURRENT;
        break;

    case SEEK_END:
        dwMoveMethod = FILE_END;
        break;
    }

    if(!SetFilePointerEx(file_handle->handle, liDistanceToMove, &liNewFilePointer, dwMoveMethod))
    {
        switch(GetLastError())
        {
        default:
            return(file__set_error(file_handle, create_error(FILE_ERR_CANTREAD)));

        case ERROR_NEGATIVE_SEEK:
            return(file__set_error(file_handle, create_error(FILE_ERR_INVALIDPOSITION)));
        }
    }

    new_pos.u.u64 = (U64) liNewFilePointer.QuadPart;
#else
    if(fseek(file_handle->handle, (S32) offset, origin))
        return(file__set_error(file_handle, create_error(FILE_ERR_CANTREAD)));

#if CHECKING
    status = file__getpos(file_handle, &new_pos);
#endif
#endif
    } /*block*/

    trace_4(TRACE_MODULE_FILE, TEXT("file__seek(") PTR_XTFMT TEXT(", ") S64_TFMT TEXT(", ") S32_TFMT TEXT(") yields ") U64_TFMT, file_handle, offset64, origin, new_pos.u.u64);
    return(status);
}

_Check_return_
static STATUS
file__set_error(
    _InoutRef_  FILE_HANDLE file_handle,
    _InVal_     STATUS status)
{
    if(status_fail(status) && !file_handle->error)
        file_handle->error = status;

    return(status);
}

/******************************************************************************
*
* write a buffer to file at a given position. updates seqptr
*
* --in--
*
*   all parameters must be valid, file open on filing system
*
******************************************************************************/

_Check_return_
static STATUS
file__write(
    _In_reads_bytes_(bytestowrite) PC_ANY ptr,
    _InVal_     U32 bytestowrite,
    _InoutRef_  FILE_HANDLE file_handle)
{
    U32 byteswritten;

    trace_4(TRACE_MODULE_FILE, TEXT("file__write(") PTR_XTFMT TEXT(", ") PTR_XTFMT TEXT("): n_bytes ") U32_TFMT TEXT(" -> handle ") UINTPTR_XTFMT, ptr, file_handle, bytestowrite, (uintptr_t) file_handle->handle);

    /* sticky error for ANSI closeness */
    if(file_handle->error)
    {
        trace_1(TRACE_MODULE_FILE, TEXT("file__write returns sticky error ") S32_TFMT, file_handle->error);
        return(file_handle->error);
    }

    file_handle->written_to = 1;

    {
#if RISCOS
    _kernel_osgbpb_block blk;

    blk.dataptr = (void *) ptr;
    blk.nbytes  = (int) bytestowrite;

    if(_kernel_ERROR == _kernel_osgbpb(OSGBPB_WriteAtPTR, file_handle->handle, &blk))
    {
        status_consume(file__set_error(file_handle, create_error(FILE_ERR_CANTWRITE)));
        return(file__obtain_error_string(_kernel_last_oserror()));
    }

    byteswritten = bytestowrite - (U32) blk.nbytes;
#elif WINDOWS
    errno = 0;

    if(!WriteFile(file_handle->handle, ptr, bytestowrite, (LPDWORD) &byteswritten, NULL))
    {
        switch(GetLastError())
        {
        /*case ENOSPC:
            return(file__set_error(file_handle, create_error(FILE_ERR_DEVICEFULL)));*/

        default:
            return(file__set_error(file_handle, create_error(FILE_ERR_CANTWRITE)));
        }
    }
#else
    size_t memberswritten;

    nmemb = fwrite(ptr, 1, bytestowrite, file_handle->handle);

    if(memberswritten != nmemb)
        return(file__set_error(file_handle, create_error(FILE_ERR_DEVICEFULL)));
#endif
    } /*block*/

    /* this case should never happen with a RISC OS filing system */
    /* pretty well defined to be the error we should get on MS-DOS */
    if(byteswritten != bytestowrite)
        return(file__set_error(file_handle, create_error(FILE_ERR_DEVICEFULL)));

    return(STATUS_OK);
}

/* ***                                                                      ***
*  ***                                                                      ***
*  *** procedures that are exported for file_getc/file_putc/file_eof macros ***
*  ***                                                                      ***
*  ***                                                                      *** */

/******************************************************************************
*
* routine called from file_getc on failure
*
******************************************************************************/

_Check_return_
extern STATUS
file__filbuf(
    _InoutRef_  FILE_HANDLE file_handle)
{
    U8     c;
    STATUS status;

    /* increment as count got decremented to -1 or -2
     * (do even in case of naff file - might repair damage at least)
    */
    file_handle->count += 1;

    if(!file_handle || (file_handle->magic != _FILE_MAGIC_WORD))
        return(create_error(FILE_ERR_BADHANDLE));

    if(file_handle->flags & _FILE_HASUNGOTCH)
    {
        c = (U8) file_handle->ungotch;

        /* guaranteed no buffered data in this state! */

        /* move file ptr forwards one byte in file -- ANSI clears ungot state */
        status_return(file__seek(file_handle, +1, SEEK_CUR));

        return(c);
    }

    if(!file_handle->bufbase)
    {
        if(file_handle->bufsize)
            status_return(file_buffer(file_handle, NULL, file_handle->bufsize));

        /* DO NOT combine these two! (side effects from file_buffer()) */

        if(!file_handle->bufsize)
        {   /* unbuffered i/o needed */
            U32 bytesread;
            status_return(file_read_bytes(&c, 1, &bytesread, file_handle));
            if(bytesread == 1)
                return(c);
            file_handle->flags |= _FILE_EOFREAD;
            return(EOF_READ);
        }
    }

    status = file__fillbuffer(file_handle);

    if(status_fail(status) || (EOF_READ == status))
        return(status);

    return(file_getc(file_handle));
}

/******************************************************************************
*
* routine called from file_putc on failure
*
* can be either
*   (i)  buffer has been filled and requires flushing, or
*   (ii) no buffer, so ensure we have an empty buffer we
*        can write to and make it dirty so it will be flushed
*
* NB. getc/putc MUST be seek etc. separated a la ANSI spec
*     or corruption of thy files wilst be the result
*
******************************************************************************/

_Check_return_
extern STATUS
file__flsbuf(
    _InVal_     STATUS c,
    _InoutRef_  FILE_HANDLE file_handle)
{
    /* increment as count got decremented to -1 or -2
     * (do even in case of naff file - might repair damage at least)
    */
    file_handle->count += 1;

    if(!file_handle || (file_handle->magic != _FILE_MAGIC_WORD))
        return(create_error(FILE_ERR_BADHANDLE));

    /* no buffered data? */
    if(file_handle->count == -1)
    {
        status_return(file__make_output_buffer(file_handle));

        if(!file_handle->bufsize)
        {
            /* explicitly unbuffered i/o needed */
            STATUS status = file__write(&c, 1, file_handle);

            return(status_fail(status) ? status : c);
        }

        file_handle->count -= 1;
        *file_handle->ptr++ = (U8) c; /* put the char in the buffer */
        return(c);
    }

    /* file has filled output buffer, clear out to disc and reset */
    status_return(file__flushbuffer(file_handle, TEXT("file__flsbuf")));

    /* we expect the putc to recurse in this case to the above case ... */
    return(file_putc(c, file_handle));
}

#if RISCOS

/******************************************************************************
*
* return end-of-file status
*
******************************************************************************/

_Check_return_
extern STATUS
file__eof(
    _InoutRef_  FILE_HANDLE file_handle)
{
    STATUS status;

    trace_1(TRACE_MODULE_FILE, TEXT("file__eof(") PTR_XTFMT TEXT(")"), file_handle);

    if(!file_handle || (file_handle->magic != _FILE_MAGIC_WORD))
        return(create_error(FILE_ERR_BADHANDLE));

    /* must flush internal buffers to do EOF check */
    status_return(file__flushbuffer( file_handle, TEXT("file_eof")));

    {
#if RISCOS
    _kernel_swi_regs rs;

    rs.r[0] = OSArgs_EOFCheck;
    rs.r[1] = file_handle->handle;

    if(status_fail(status = file__obtain_error_string(_kernel_swi(OS_Args, &rs, &rs))))
        (void) file__set_error(file_handle, create_error(FILE_ERR_CANTWRITE));
    else
        status = (rs.r[2] != 0) ? EOF : STATUS_OK; /* non-zero -> EOF */
#elif WINDOWS
    int res = _eof(file_handle->handle);

    if(res == -1)
        status = file__set_error(file_handle, create_error(FILE_ERR_BADHANDLE));
    else
        status = res ? EOF : STATUS_OK;
#else
    status = feof(file_handle->handle) ? EOF : STATUS_OK;
#endif
    } /*block*/

    trace_2(TRACE_MODULE_FILE, TEXT("file__eof(") PTR_XTFMT TEXT(") yields ") S32_TFMT, file_handle, status);
    return(status);
}

#endif

/* end of file.c */
