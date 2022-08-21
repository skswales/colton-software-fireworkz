/* file.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1990-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* External interface to the file handling module (both stream & utils) */

/* SKS February 1990 */

#ifndef __file_h
#define __file_h

/*
macro definitions
*/

/* NB this needs to be bigger than the largest valid UCS-4 character and still positive */
#define EOF_READ 0x40000000 /* bit 30 set */

#if RISCOS
#define FILE_DEFBUFSIZ      4096 /* even bigger buffering to ease unbuffered I/O pain */ /* was 1024, then 2048 */

/* possibly limited by C library */
#define MAX_PATHSTRING      (1+255)

/* often limited by Window Manager message block */
#define MAX_FILENAME_WIMP   (1+211)
#define BUF_MAX_FILENAME_WIMP (MAX_FILENAME_WIMP + 1)

#define FILE_WILD_MULTIPLE_ALL_TSTR TEXT("*")

#define FILE_WILD_MULTIPLE               '*'
#define FILE_WILD_MULTIPLE_TSTR     TEXT("*")
#define FILE_WILD_SINGLE                 '#'
#define FILE_WILD_SINGLE_TSTR       TEXT("#")

/* : anywhere in RISC OS -> rooted */
#define FILE_ROOT_CH             ':'
#define FILE_DIR_SEP_CH          '.'
#define FILE_DIR_SEP_TSTR   TEXT(".")

/* no real file extensions on RISC OS but / has become conventional (8/3) on DOS floppies */
#define FILE_EXT_SEP_CH          '/'
#define FILE_EXT_SEP_TSTR   TEXT("/")

#define FILE_PATH_SEP_CH         ','
#define FILE_PATH_SEP_TSTR  TEXT(",")

#if 1
#define FILE_STDOUT_NAME    "vdu:"
#else
#define FILE_STDOUT_NAME    "rawvdu:"
#endif

#elif WINDOWS
#define FILE_DEFBUFSIZ      4096    /* was 512 */

/* something small */
#define MAX_PATHSTRING      259     /* BUF_MAX_PATHSTRING adds 1 for CH_NULL to defined MAX_PATH 260 */

#define FILE_WILD_MULTIPLE_ALL_TSTR TEXT("*.*")

#define FILE_WILD_MULTIPLE           '*'
#define FILE_WILD_MULTIPLE_TSTR TEXT("*")
#define FILE_WILD_SINGLE             '?'
#define FILE_WILD_SINGLE_TSTR   TEXT("?")

/* : anywhere OR first is \ on DOS -> rooted */
#define FILE_ROOT_CH             ':'
#define FILE_DIR_SEP_CH          '\\'
#define FILE_DIR_SEP_TSTR   TEXT("\\")
/* / can be used when talking to C library */
#define FILE_DIR_SEP_CH2         '/'

/* DOS has real extensions (8.3) */
#define FILE_EXT_SEP_CH          '.'
#define FILE_EXT_SEP_TSTR   TEXT(".")

#define FILE_PATH_SEP_CH         ';'
#define FILE_PATH_SEP_TSTR  TEXT(";")

#endif

#define BUF_MAX_PATHSTRING (MAX_PATHSTRING + 1)


/* sometimes we need to parse filenames from alien systems */

#define RISCOS_FILE_EXT_SEP_CH           '/' /* see notes above */
#define RISCOS_FILE_EXT_SEP_TSTR    TEXT("/")

#define RISCOS_FILE_DIR_SEP_CH           '.'
#define RISCOS_FILE_DIR_SEP_TSTR    TEXT(".")

#define WINDOWS_FILE_EXT_SEP_CH          '.'
#define WINDOWS_FILE_EXT_SEP_TSTR   TEXT(".")

#define WINDOWS_FILE_DIR_SEP_CH          '\\'
#define WINDOWS_FILE_DIR_SEP_TSTR   TEXT("\\")


/*
structure definitions
*/

/*
position within file
*/

typedef struct filepos_t
{
    union filepos_t_u
    {
        U64 u64;

        struct filepos_t_u_words
        {
            U32 lo;
            U32 hi;
        } words;
    } u;
}
filepos_t;

#define filelength_t filepos_t

/*
mode of opening for file
*/

#define file_open_read      0
#define file_open_write     1
#define file_open_readwrite 2

#define file_open_mode int

#if RISCOS
typedef struct FILE_EDATA_RISCOS
{
    file_open_mode openmode;
    T5_FILETYPE t5_filetype;
    int file_date_loword, file_date_hiword; /* UTC representation of file modified date/time */
}
FILE_EDATA_RISCOS;
#elif WINDOWS
typedef struct FILE_EDATA_WINDOWS
{
    FILETIME file_date; /* UTC representation of file modified date/time */
}
FILE_EDATA_WINDOWS;
#endif /* OS */

typedef struct _FILE_HANDLE
{
    /* public (only for macros) */
    S32     count;      /* number of bytes left in buffer */
    P_U8    ptr;        /* pointer to current data item (char for loading) */
    S32     flags;

    /* keep your hands off these privates (or what?) */

    struct _FILE_HANDLE *   next;

#define _FILE_MAGIC_TYPE U32
#define _FILE_MAGIC_WORD 0x21147226

    _FILE_MAGIC_TYPE        magic;

#if RISCOS
#define _FILE_HANDLE_TYPE int
#define _FILE_HANDLE_NULL ((_FILE_HANDLE_TYPE)  0)
#elif WINDOWS
#define _FILE_HANDLE_TYPE HANDLE
#define _FILE_HANDLE_NULL INVALID_HANDLE_VALUE
#endif

    _FILE_HANDLE_TYPE       handle;             /* host filing system handle */

    STATUS                  error;              /* sticky error */
    STATUS                  ungotch;            /* room for one ungot character */
    P_ANY                   bufbase;            /* address of buffer */
    U32                     bufsize;            /* allocated size of that buffer */
    U32                     bufbytes;           /* amount of buffer valid (bufbytes - count) */
    filepos_t               bufpos;             /* file position of buffer */
    BOOL                    written_to;

    PTSTR                   tstr_filename;      /* copy of opened filename */

#if RISCOS
    FILE_EDATA_RISCOS       riscos;
#elif WINDOWS
    FILE_EDATA_WINDOWS      windows;
#endif
}
_FILE_HANDLE, * FILE_HANDLE, ** P_FILE_HANDLE;
#define __FILE_HANDLE_DEFINED

/*
types of object returned by objinfo
*/

enum FILE_OBJECT_TYPES
{
    FILE_OBJECT_NONE = 0,
    FILE_OBJECT_FILE = 1,
    FILE_OBJECT_DIRECTORY = 2
};

/*
this structure is also private to fileutil.c but needs to be made visible for typedef safety
*/

typedef struct FILE_PATHENUM
{
    PCTSTR ptr; /* current state */
    PTSTR path; /* tstr_set() */
    TCHARZ res[BUF_MAX_PATHSTRING];
}
FILE_PATHENUM, * P_FILE_PATHENUM, ** P_P_FILE_PATHENUM;

/*
as does this one
*/

#if RISCOS
#define _FILE_OBJECT_IS_FILE 1
#define _FILE_OBJECT_IS_DIR  2
#endif

typedef struct FILE_OBJINFO
{
    S32 type;
    T5_FILETYPE t5_filetype;
#if RISCOS
    struct FILE_OBJINFO_FILEINFO /* do not change this structure - OS_GBPB defined */
    {
        S32 load;
        S32 exec;
        U32 size;
        S32 attr;
        S32 _type;
        TCHARZ name[66];
    }
    fileinfo;
#elif WINDOWS
    WIN32_FIND_DATA win32_find_data;
#endif
}
FILE_OBJINFO, * P_FILE_OBJINFO; typedef const FILE_OBJINFO * PC_FILE_OBJINFO;

/*
and these too
*/

typedef struct FILE_OBJENUM
{
    TCHARZ pattern[BUF_MAX_PATHSTRING];
    TCHARZ subdir[BUF_MAX_PATHSTRING];
    S32 state;
    P_FILE_PATHENUM pathenum;
    FILE_OBJINFO objinfo;
#if RISCOS
    int riscos_dirindex;
#elif WINDOWS
    HANDLE win32_find_handle;
#endif
}
FILE_OBJENUM, * P_FILE_OBJENUM, ** P_P_FILE_OBJENUM;

/*
function declarations
*/

/*
file.c - annoyingly get DPlib clash so t5_ prefix where needed (close/flush/open)
*/

_Check_return_
extern STATUS
file_buffer(
    _InoutRef_opt_ FILE_HANDLE file_handle,
    _In_opt_    void * buffer,
    _InVal_     U32 bufsize);

_Check_return_
extern STATUS
file_clearerror(
    _InoutRef_opt_ FILE_HANDLE file_handle);

_Check_return_
extern STATUS
t5_file_close(
    _InoutRef_  P_FILE_HANDLE p_file_handle);

_Check_return_
extern STATUS
file_close_date(
    _InoutRef_  P_FILE_HANDLE p_file_handle,
    _OutRef_    P_SS_DATE p_ss_date);

_Check_return_
extern STATUS
file_create_directory(
    _In_z_      PCTSTR dirname);

_Check_return_
extern STATUS
file_date(
    _InoutRef_opt_ FILE_HANDLE file_handle,
    _OutRef_    P_SS_DATE p_ss_date);

_Check_return_
extern STATUS
file_error(
    _InoutRef_opt_ FILE_HANDLE file_handle);

extern void
file_error_buffer_set(
    _In_reads_(error_bufelem) PTSTR error_buffer,
    _InVal_     U32 error_bufelem);

_Check_return_
_Ret_maybenull_z_
extern PCTSTR
file_error_get(void);

_Check_return_
extern STATUS
file_error_set(
    _In_opt_z_  PCTSTR error);

#if WINDOWS

_Check_return_
extern STATUS
file_error_set_from_last_error(
    _InVal_     DWORD dwLastError,
    _InVal_     STATUS status_message,
    _In_z_      PCTSTR filename);

#endif /* WINDOWS */

extern void
file_finalise(void);

_Check_return_
extern STATUS
t5_file_flush(
    _InoutRef_opt_ FILE_HANDLE file_handle);

_Check_return_
extern STATUS
file_getbyte(
    _InoutRef_opt_ FILE_HANDLE file_handle);

_Check_return_
extern STATUS
file_getpos(
    _InoutRef_opt_ FILE_HANDLE file_handle,
    _Out_       filepos_t * const p_cur_pos);

_Check_return_
extern STATUS
file_gets(
    _Out_writes_z_(bufsize) P_U8 buffer,
    _InVal_     U32 bufsize,
    _InoutRef_opt_ FILE_HANDLE file_handle);

extern void
file_startup(void);

_Check_return_
extern STATUS
file_length(
    _InoutRef_opt_ FILE_HANDLE file_handle,
    _Out_       filelength_t * const p_file_length);

_Check_return_
extern STATUS
t5_file_open(
    _In_z_      PCTSTR filename,
    _InVal_     file_open_mode openmode,
    _OutRef_    P_FILE_HANDLE p_file_handle,
    _In_        BOOL must_open); /* forced TRUE iff write */

_Check_return_
extern STATUS
file_pad(
    _InoutRef_opt_ FILE_HANDLE file_handle,
    _InVal_     U32 alignpower,
    _InVal_     U8 pad_byte);

_Check_return_
extern STATUS
file_putbyte(
    _InVal_     STATUS c,
    _InoutRef_opt_ FILE_HANDLE file_handle);

#if defined(UNUSED_KEEP_ALIVE)

_Check_return_ _Success_(status_ok(return))
extern STATUS
file_read(
    _Out_bytecap_x_(size*nmemb) P_ANY ptr,
    _InVal_     U32 size,
    _InVal_     U32 nmemb,
    _InoutRef_opt_ FILE_HANDLE file_handle);

#endif /* UNUSED_KEEP_ALIVE */

_Check_return_ _Success_(status_ok(return))
extern STATUS
file_read_bytes(
    _Out_writes_bytes_(bytestoread) P_ANY ptr,
    _InVal_     U32 bytestoread,
    _OutRef_    P_U32 p_bytesread,
    _InoutRef_opt_ FILE_HANDLE file_handle);

_Check_return_ _Success_(status_ok(return))
extern STATUS
file_read_bytes_requested(
    _Out_writes_bytes_(bytestoread) P_ANY ptr,
    _InVal_     U32 bytestoread,
    _InoutRef_opt_ FILE_HANDLE file_handle);

_Check_return_
extern STATUS
file_rewind(
    _InoutRef_opt_ FILE_HANDLE file_handle);

_Check_return_
extern STATUS
file_seek(
    _InoutRef_opt_ FILE_HANDLE file_handle,
    _InVal_     S32 offset_lo,
    _InRef_opt_ PC_S32 p_offset_hi,
    _InVal_     S32 origin);

_Check_return_
extern STATUS
file_setpos(
    _InoutRef_opt_ FILE_HANDLE file_handle,
    _In_        const filepos_t * const p_new_pos);

_Check_return_
extern STATUS
file_tell(
    _InoutRef_opt_ FILE_HANDLE file_handle,
    _OutRef_    P_U32 p_cur_pos_lo,
    _OutRef_    P_U32 p_cur_pos_hi);

_Check_return_
extern STATUS
file_ungetc(
    _InVal_     STATUS c,
    _InoutRef_opt_ FILE_HANDLE file_handle);

#if defined(UNUSED_KEEP_ALIVE)

_Check_return_
extern STATUS
file_write(
    _In_reads_bytes_x_(size*nmemb) PC_ANY ptr,
    _InVal_     U32 size,
    _InVal_     U32 nmemb,
    _InoutRef_opt_ FILE_HANDLE file_handle);

#endif /* UNUSED_KEEP_ALIVE */

_Check_return_
extern STATUS
file_write_bytes(
    _In_reads_bytes_(bytestowrite) PC_ANY ptr,
    _InVal_     U32 bytestowrite,
    _InoutRef_opt_ FILE_HANDLE file_handle);

#if RISCOS

_Check_return_
extern STATUS
file_get_risc_os_filetype(
    _InoutRef_opt_ FILE_HANDLE file_handle);

_Check_return_
extern STATUS
file_set_risc_os_filetype(
    _InoutRef_opt_ FILE_HANDLE file_handle,
    _InVal_     T5_FILETYPE t5_filetype);

#endif /* OS */

/*
fileutil.c
*/

extern void
file_build_paths(void);

_Check_return_
extern STATUS
file_combine_path(
    _OutRef_    P_PTSTR aa,
    _In_opt_z_  PCTSTR currentfilename,
    _In_opt_z_  PCTSTR search_path);

_Check_return_
extern STATUS
file_derive_name(
    _In_z_      PCTSTR dir,
    _In_z_      PCTSTR leafname,
    _In_z_      PCTSTR suffix,
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*appended,terminated*/,
    _InVal_     STATUS status_isafile);

extern PTSTR
file_dirname(
    PTSTR destpath,
    PCTSTR srcfilename);

_Check_return_
_Ret_maybenull_z_
extern PTSTR
file_extension(
    _In_z_      PCTSTR filename);

_Check_return_
_Ret_maybenull_z_
extern PTSTR
file_extension_ch(
    _In_z_      PCTSTR filename,
    _InVal_     TCHAR ch);

extern void
file_find_close(
    /*inout*/ P_P_FILE_OBJENUM pp);

extern P_FILE_OBJINFO
file_find_first(
    /*out*/ P_P_FILE_OBJENUM pp,
    _In_z_      PCTSTR path,
    _In_z_      PCTSTR pattern);

extern P_FILE_OBJINFO
file_find_first_subdir(
    /*out*/ P_P_FILE_OBJENUM pp,
    _In_z_      PCTSTR path,
    _In_z_      PCTSTR pattern,
    _In_opt_z_  PCTSTR subdir);

extern P_FILE_OBJINFO
file_find_next(
    /*inout*/ P_P_FILE_OBJENUM pp);

_Check_return_
extern STATUS
file_find_on_path(
    _Out_writes_z_(elemof_buffer) PTSTR filename,
    _InVal_     U32 elemof_buffer,
    _In_z_      PCTSTR path,
    _In_z_      PCTSTR srcfilename);

_Check_return_
extern STATUS
file_find_on_path_or_relative(
    _Out_writes_z_(elemof_buffer) PTSTR filename,
    _InVal_     U32 elemof_buffer,
    _In_z_      PCTSTR path,
    _In_z_      PCTSTR srcfilename,
    _In_opt_z_  PCTSTR currentfilename);

extern PTSTR
file_find_query_dirname(
    _InRef_     P_P_FILE_OBJENUM pp,
    _Out_writes_z_(elemof_buffer) PTSTR destpath,
    _InVal_     U32 elemof_buffer);

extern PTSTR
file_get_cwd(
    PTSTR destpath,
    PCTSTR currentfilename);

extern PTSTR
file_get_prefix(
    _Out_writes_z_(elemof_buffer) PTSTR destpath,
    _InVal_     U32 elemof_buffer,
    _In_opt_z_  PCTSTR currentfilename);

extern PCTSTR
file_get_resources_path(void);

extern PCTSTR
file_get_search_path(void);

extern PCTSTR
file_get_templates_path(void);

_Check_return_
extern BOOL
file_is_dir(
    _In_z_      PCTSTR dirname);

_Check_return_
extern BOOL
file_is_file(
    _In_z_      PCTSTR filename);

_Check_return_
extern BOOL
file_is_read_only(
    _In_z_      PCTSTR filename);

_Check_return_
extern BOOL
file_is_rooted(
    _In_z_      PCTSTR filename);

_Check_return_
_Ret_z_
extern PTSTR
file_leafname(
    _In_z_      PCTSTR filename);

_Check_return_
_Ret_z_
extern PTSTR
file_leafname_riscos(
    _In_z_      PCTSTR filename);

_Check_return_
_Ret_z_
extern PTSTR
file_leafname_windows(
    _In_z_      PCTSTR filename);

_Check_return_
extern STATUS
file_objenum_fullname(
    P_P_FILE_OBJENUM p_p_file_objenum,
    P_FILE_OBJINFO oip,
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*appended,terminated*/);

_Check_return_
extern T5_FILETYPE
file_objinfo_filetype(
    _InRef_     PC_FILE_OBJINFO oip);

_Check_return_
extern STATUS
file_objinfo_name(
    _InRef_     PC_FILE_OBJINFO oip,
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*appended,terminated*/);

_Check_return_
_Ret_z_
extern PCTSTR /* low-lifetime*/
file_objinfo_name_ll(
    _InRef_     PC_FILE_OBJINFO oip);

_Check_return_
extern U32
file_objinfo_size(
    _InRef_     PC_FILE_OBJINFO oip);

_Check_return_
extern STATUS
file_objinfo_type(
    _InRef_     PC_FILE_OBJINFO oip);

extern void
file_path_element_close(
    /*inout*/ P_P_FILE_PATHENUM p);

extern PTSTR
file_path_element_first(
    /*out*/ P_P_FILE_PATHENUM p,
    PCTSTR path);

extern PTSTR
file_path_element_next(
    /*inout*/ P_P_FILE_PATHENUM p);

extern PCTSTR
file_path_query(
    _In_        UINT i);

_Check_return_
extern STATUS
file_path_set(
    PCTSTR path,
    _In_        UINT i);

#define FILE_REFERENCE_UNKNOWN 0
#define FILE_REFERENCE_RISCOS  1
#define FILE_REFERENCE_WINDOWS 2

extern U32
file_reference_type(
    _In_z_      PCTSTR filename);

_Check_return_
extern STATUS
file_remove(
    _In_z_      PCTSTR filename);

_Check_return_
extern STATUS
file_rename(
    _In_z_      PCTSTR filename_from,
    _In_z_      PCTSTR filename_to);

extern PTSTR
file_separatename(
    PTSTR destpath,
    PTSTR destfilename,
    PCTSTR srcfilename);

#define FILE_TEMPNAME_INITIAL_TRY 1

_Check_return_
extern STATUS
file_tempname(
    _In_z_      PCTSTR dir,
    _In_z_      PCTSTR prefix,
    _In_opt_z_  PCTSTR suffix,
    _InVal_     S32 flags,
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*appended,terminated*/);

_Check_return_
extern STATUS
file_tempname_null(
    _In_z_      PCTSTR prefix,
    _In_opt_z_  PCTSTR suffix,
    _InVal_     S32 flags,
    _InoutRef_  P_QUICK_TBLOCK p_quick_tblock /*appended,terminated*/);

_Check_return_
_Ret_maybenull_z_
extern PTSTR
file_wild(
    _In_z_      PCTSTR filename);

extern void
fileutil_shutdown(void);

/*
exported functions for sole use of macros
*/

/*
file.c
*/

_Check_return_
extern STATUS
file__eof(
    _InoutRef_  FILE_HANDLE file_handle);

_Check_return_
extern STATUS
file__filbuf(
    _InoutRef_  FILE_HANDLE file_handle);

_Check_return_
extern STATUS
file__flsbuf(
    _InVal_     STATUS c,
    _InoutRef_  FILE_HANDLE file_handle);

/*
functions as macros
*/

#ifndef EOF
#define EOF (-1)
#endif

#ifndef _FILE_EOFREAD
/* keep in step with file.h */
#define _FILE_EOFREAD 0x0008
#endif

/*
can only say for sure about EOF if read last buffer in and not yet at end
*/

#define file_eof(f) ( \
    ((f)->flags & _FILE_EOFREAD) \
        ? (((f)->count > 0) \
                ? STATUS_OK \
                : file__eof(f)) \
        : file__eof(f) )

#define file_getc(f) ( \
    (--(f)->count >= 0) \
        ? (*(f)->ptr++) \
        : file__filbuf(f) )

/* can improve by using these two */
#define file_getc_fast_ready(f) ( \
    (f)->count > 0 ) \

#define file_getc_fast(f) ( \
    --(f)->count \
        , (*(f)->ptr++) )

#define file_putc(c, f) ( \
    (--(f)->count >= 0) \
        ? (*(f)->ptr++ = (char)(c)) \
        : file__flsbuf((c), (f)) )

/* can improve by using these two */
#define file_putc_fast_ready(f) ( \
    (f)->count > 0 )

#define file_putc_fast(c, f) ( \
    --(f)->count \
        , (*(f)->ptr++ = (char)(c)) )

#define file_postooff(fileposp) ( \
    (S32) ((fileposp)->lo) )

#define FILE_PATH_USER      0U  /* User data */
#define FILE_PATH_ADMIN     1U  /* Admin data */
#define FILE_PATH_RESOURCES 2U  /* Program resources */
#define FILE_PATH_TEMPLATES 3U  /* Program templates */
#define FILE_PATH_N_PATHS   4U

#define RESOURCE_NUM_FILE 31

/*
error definition
*/

#define FILE_ERR_BASE   (STATUS_ERR_INCREMENT * OBJECT_ID_FILE)

#define FILE_ERR(n)     (FILE_ERR_BASE - (n))

#define FILE_ERR_CANTOPEN           FILE_ERR(0)
#define FILE_ERR_ERROR_RQ           FILE_ERR(1)
#define FILE_ERR_CANTREAD           FILE_ERR(2)
#define FILE_ERR_CANTWRITE          FILE_ERR(3)
#define FILE_ERR_CANTREADREQUESTED  FILE_ERR(4)
#define FILE_ERR_INVALIDPOSITION    FILE_ERR(5)
#define FILE_ERR_HANDLEUNBUFFERED   FILE_ERR(6)
#define FILE_ERR_spare_7            FILE_ERR(7)
#define FILE_ERR_DEVICEFULL         FILE_ERR(8)
#define FILE_ERR_CANTCLOSE          FILE_ERR(9) /* not on RISC OS, or on Windows */
#define FILE_ERR_BADHANDLE          FILE_ERR(10)
#define FILE_ERR_BADNAME            FILE_ERR(11) /* not on RISC OS */
#define FILE_ERR_NOTFOUND           FILE_ERR(12)
#define FILE_ERR_ISAFILE            FILE_ERR(13)
#define FILE_ERR_ISADIR             FILE_ERR(14)
#define FILE_ERR_NAMETOOLONG        FILE_ERR(15)
#define FILE_ERR_LOCKED             FILE_ERR(16) /* not on Windows */
#define FILE_ERR_NO_ACCESS_READ     FILE_ERR(17) /* not on Windows */
#define FILE_ERR_NO_ACCESS_WRITE    FILE_ERR(18)
#define FILE_ERR_ISAZIP             FILE_ERR(19)
#define FILE_ERR_NOTADIR            FILE_ERR(20)
#define FILE_ERR_BADTIMESTAMP       FILE_ERR(21)

#define FILE_ERR_FRAG_WHEN_OPENING      FILE_ERR(108)
#define FILE_ERR_FRAG_WHEN_CLOSING      FILE_ERR(109)
#define FILE_ERR_FRAG_WHILE_READING     FILE_ERR(110)
#define FILE_ERR_FRAG_WHILE_WRITING     FILE_ERR(111)
#define FILE_ERR_FRAG_WHILE_SEEKING     FILE_ERR(112)
#define FILE_ERR_FRAG_RENAMING          FILE_ERR(114)
#define FILE_ERR_FRAG_REMOVING          FILE_ERR(115)
#define FILE_ERR_FRAG_CREATE_DIRECTORY  FILE_ERR(116)

#endif /* __file_h */

/* end of file.h */
