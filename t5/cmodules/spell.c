/* spell.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1988-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/******************************************************************************
*
* spelling checker for PipeDream
*
* MRJC May 1988
* March 1989 updated to use sparse lists
* July 1989 updated for moveable indexes
* April 1990 internationalisation
* Sept 1991 full event handling added
* November 1992 new style & aligator
*
******************************************************************************/

#include "common/gflags.h"

#include "ob_spell/ob_spell.h"

#if !defined(SPELL_OFF)

#if !defined(__CHAR_UNSIGNED__)
#error chars must be unsigned (see coltsoft/coltsoft.h)
#endif

/*
internal structure
*/

__pragma(pack(push, 2)) /* WINDOWS */

#define NAM_SIZE     100                        /* maximum size of names */
#define EXT_SIZE     500                        /* size of disk extend */
#define ESC_CHAR     '|'                        /* escape character */
#define MIN_CHAR      32                        /* minimum character offset */
#define MAX_CHAR      64                        /* maximum character offset */
#define MIN_TOKEN     64                        /* minium value of token */
#define MAX_TOKEN    256                        /* maximum value of token */
#define MAX_INDEX     96                        /* maximum elements in index level */
#define MAX_ENDLEN    15                        /* maximum ending length */
#define BUF_MAX_ENDLEN (MAX_ENDLEN + 1)

#define KEYSTR "[Colton Soft]"

/*
cached block structure
*/

typedef struct CACHEBLOCK
{
    S32 usecount;
    DICT_NUMBER dict_number;
    S32 lettix;
    S32 diskaddress;
    S32 diskspace;
}
CACHEBLOCK, * P_CACHEBLOCK;

#define cacheblock_ptr(p, i) ( \
    (P_U8) ((p) + 1) + (i) )

/*
structure of an index to a dictionary
*/

typedef struct LETTER
{
    union LETTER_P
    {
        LIST_ITEMNO cacheno;
        S32 disk;
    } p;
    S16 blklen;
    U8 letflags;
}
LETTER, * P_LETTER;

/* letter flags */

#define LET_CACHED 0x80
#define LET_LOCKED 0x40
#define LET_ONE    0x20
#define LET_TWO    0x10
#define LET_WRITE     8

/*
structure of the index of dictionaries
*/

typedef struct DICT
{
    FILE_HANDLE file_handle_dict;       /* handle of dictionary file */
    ARRAY_HANDLE h_index;               /* handle of index */
    S32 dictsize;                       /* size of dictionary on disk */
    P_ANY dictbuf;                      /* pointer to disk buffer */
    LIST_BLOCK dict_end_list;           /* list block for ending list */
    U8 char_offset;                     /* character offset */
    U8 token_start;                     /* number of first token */
    U8 man_token_start;                 /* manually supplied token start */
    U8 _spare;
    S32 index_offset;                   /* offset of index start */
    S32 data_offset;                    /* offset of data start */
    S32 n_index;                        /* number of index entries */
    U8 n_index_1;                       /* number of first level elements */
    U8 n_index_2;                       /* number of second level elements */
    U8 letter_1[MAX_INDEX];             /* mapping of chars for 1st letter */
    U8 letter_2[MAX_INDEX];             /* mapping of chars for 2nd letter */
    U8 case_map[256];                   /* case equivalence map */
    U8 dictflags;                       /* dictionary flags */
    U8 _spare_;
    ARRAY_HANDLE_TSTR h_dict_filename;  /* dictionary filename */
    ARRAY_HANDLE_USTR h_dict_name;      /* dictionary name */
}
DICT, * P_DICT, ** P_P_DICT;

#define P_DICT_NONE _P_DATA_NONE(P_DICT)

/*
tokenised word structure
*/

typedef struct TOKWORD
{
    S32 len;
    S32 lettix;
    S32 tail;

    S32 fail;                           /* reason for failure of search */
    S32 findpos;                        /* offset from start of data of root */
    S32 matchc;                         /* characters at start of root matched */
    S32 match;                          /* whole root matched ? */
    S32 matchcp;                        /* characters at start of previous root matched */
    S32 matchp;                         /* whole of previous root matched ? */

    U8 body[MAX_WORD];
    U8 bodyd[MAX_WORD];
    U8 bodydp[MAX_WORD];
}
TOKWORD, * P_TOKWORD;

/*
ending structure
*/

typedef struct ENDING
{
    U8 len;
    U8 alpha;
    U8 pos;
    U8 ending[BUF_MAX_ENDLEN];
}
ENDING, * P_ENDING; typedef const ENDING * PC_ENDING;

/*
internal ending structure
*/

typedef struct ENDING_I
{
    U8 len;
    U8 alpha;
    U8 ending[1];
}
ENDING_I, * P_ENDING_I;

_Check_return_
_Ret_valid_
static inline P_ENDING_I
ending_list_goto_item(
    _InoutRef_  P_LIST_BLOCK p_ending_list,
    _InVal_     LIST_ITEMNO itemno)
{
    P_ENDING_I p_ending_i = list_gotoitemcontents(ENDING_I, p_ending_list, itemno);
    assert(!IS_P_DATA_NONE(p_ending_i));
    return(p_ending_i);
}

/*
insert codes
*/

#define INS_WORD        1
#define INS_TOKENCUR    2
#define INS_TOKENPREV   3
#define INS_STARTLET    4

__pragma(pack(pop)) /* WINDOWS */

/*
internal variables
*/

static ARRAY_HANDLE h_dict_table = 0;           /* handle of dictionary table */

static P_LIST_BLOCK p_list_block_cache = NULL;  /* list of cached blocks */

_Check_return_
_Ret_notnull_
static inline P_CACHEBLOCK
cacheblock_goto_item(
    _In_        LIST_ITEMNO item)
{
    P_CACHEBLOCK p_cacheblock = list_gotoitemcontents(CACHEBLOCK, p_list_block_cache, item);
    assert(!IS_P_DATA_NONE(p_cacheblock));
    return(p_cacheblock);
}

static S32 cache_lock = 0;                      /* ignore full events for a mo */

/*
internal function declarations
*/

_Check_return_
static BOOL
badcharsin(
    _InoutRef_  P_DICT p_dict,
    _In_z_      PC_SBSTR str);

_Check_return_
static STATUS
char_ordinal_1(
    _InoutRef_  P_DICT p_dict,
    _InVal_     U8 ch);

#if WINDOWS

_Check_return_
static STATUS
def_file_position(
    _InoutRef_  FILE_HANDLE def_file_handle);

#endif /* OS */

_Check_return_
static STATUS
delreins(
    _InoutRef_  P_DICT p_dict,
    _In_z_      PC_SBSTR word,
    P_TOKWORD p_tokword);

#define dict_number(p_dict) ((DICT_NUMBER) \
    array_indexof_element(&h_dict_table, DICT, p_dict) )

_Check_return_
static inline STATUS
dict_seek_set(
    _InRef_     P_DICT p_dict,
    _InVal_     S32 filepos_lo)
{
    return(file_seek(p_dict->file_handle_dict, filepos_lo, NULL /*hi*/, SEEK_SET));
}

_Check_return_
static inline STATUS
dict_validate(
    _OutRef_    P_P_DICT p_p_dict,
    _InVal_     DICT_NUMBER dict_number)
{
    *p_p_dict = P_DICT_NONE;

    if(array_index_is_valid(&h_dict_table, dict_number))
    {
        *p_p_dict = array_ptr_no_checks(&h_dict_table, DICT, dict_number);

        assert(0 != (*p_p_dict)->h_index);
        if(0 != (*p_p_dict)->h_index)
            return(STATUS_OK);
    }

    return(create_error(SPELL_ERR_BADDICT));
}

static S32
endmatch(
    _InoutRef_  P_DICT p_dict,
    _In_z_      PC_SBSTR word,
    _In_opt_z_  PC_U8Z mask,
    _InVal_     S32 updown);

_Check_return_
static STATUS
ensuredict(
    _InoutRef_  P_DICT p_dict);

_Check_return_
static STATUS
fetchblock(
    _InoutRef_  P_DICT p_dict,
    _InVal_     S32 lettix);

_Check_return_
static STATUS
freecache(
    _InVal_     S32 lettix);

_Check_return_
static STATUS
get_dict_entry(
    _OutRef_    P_P_DICT p_p_dict);

static S32
initmatch(
    _InoutRef_  P_DICT p_dict,
    P_U8 wordout,
    _InVal_     S32 sizeof_wordout,
    _In_z_      PC_SBSTR wordin,
    _In_opt_z_  PC_U8Z mask);

#define iswordc_us(p_dict, ch) ( \
    (p_dict)->case_map[(ch)] )

_Check_return_
static STATUS
killcache(
    _In_        LIST_ITEMNO cacheno);

_Check_return_
static STATUS
load_dict_def(
    _InoutRef_  P_DICT p_dict);

_Check_return_
static STATUS
load_dict_def_now(
    _InoutRef_  P_DICT p_dict,
    _InoutRef_  FILE_HANDLE def_file,
    _InVal_     S32 keylen);

_Check_return_
static STATUS
lookupword(
    _InoutRef_  P_DICT p_dict,
    P_TOKWORD p_tokword,
    _InVal_     S32 needpos);

_Check_return_
_Ret_maybenull_
static P_TOKWORD
makeindex(
    _InoutRef_  P_DICT p_dict,
    P_TOKWORD p_tokword,
    _In_z_      PC_SBSTR word);

static S32
matchword(
    _InoutRef_  P_DICT p_dict,
    _In_opt_z_  PC_U8Z mask,
    _In_z_      PC_SBSTR word);

_Check_return_
static STATUS
nextword(
    _InoutRef_  P_DICT p_dict,
    P_U8Z word);

static S32
ordinal_char_1(
    _InoutRef_  P_DICT p_dict,
    _InVal_     S32 ord);

static S32
ordinal_char_2(
    _InoutRef_  P_DICT p_dict,
    _InVal_     S32 ord);

static S32
ordinal_char_3(
    _InoutRef_  P_DICT p_dict,
    _InVal_     S32 ord);

_Check_return_
static STATUS
prevword(
    _InoutRef_  P_DICT p_dict,
    P_U8Z word);

_Check_return_
static STATUS
read_def_line(
    _InoutRef_  FILE_HANDLE def_file,
    P_U8 buffer);

_Check_return_
static STATUS
read_def_line_ensure(
    _InoutRef_  FILE_HANDLE def_file,
    P_U8 buffer);

static void
release_dict_entry(
    _InoutRef_  P_DICT p_dict);

static S32
setabval(
    _InoutRef_  P_DICT p_dict,
    P_TOKWORD p_tokword,
    _InVal_     S32 lo_hi);

_Check_return_
static int
strnicmp_us(
    _InoutRef_  P_DICT p_dict,
    _In_        PC_SBSTR word1,
    _In_        PC_SBSTR word2,
    _In_        S32 len);

static void
stuffcache(
    _InoutRef_  P_DICT p_dict);

static void
tokenise(
    _InoutRef_  P_DICT p_dict,
    P_TOKWORD p_tokword,
    _InVal_     S32 rootlen);

static S32
tolower_us(
    _InoutRef_  P_DICT p_dict,
    _InVal_     S32 ch);

#define toupper_us(p_dict, ch) ( \
    (p_dict)->case_map[(ch)] \
        ? (p_dict)->case_map[(ch)] \
        : (ch) )

_Check_return_
static STATUS
writeblock(
    P_CACHEBLOCK p_cacheblock);

_Check_return_
static STATUS
writeindex(
    _InoutRef_  P_DICT p_dict,
    _InVal_     S32 lettix);

/******************************************************************************
*
* add a word to a dictionary
*
* --out--
* >0 word added
*  0 word exists
* <0 error
*
******************************************************************************/

_Check_return_
extern STATUS
spell_addword(
    _InVal_     DICT_NUMBER dict_number,
    _In_z_      PC_SBSTR word)
{
    static S32 spell_addword_nestf = 0;

    STATUS status, err;
    TOKWORD newword;
    S32 rootlen;
    U8 token_start;
    P_U8 newpos, ci;
    P_LIST_ITEM it;
    P_LETTER p_letter;
    P_DICT p_dict;

    status_return(dict_validate(&p_dict, dict_number));

    if(!makeindex(p_dict, &newword, word))
        return(create_error(SPELL_ERR_BADWORD));

    /* check if word exists and get position */
    status_return(status = lookupword(p_dict, &newword, TRUE));

    /* check if dictionary is read only */
    if(p_dict->dictflags & DICT_READONLY)
        return(create_error(SPELL_ERR_READONLY));

    if(status)
        return(STATUS_OK);

    token_start = p_dict->token_start;

    switch(newword.len)
    {
    /* single letter word */
    case 1:
        array_ptr(&p_dict->h_index, LETTER, newword.lettix)->letflags |= LET_ONE;
        break;

    /* two letter word */
    case 2:
        array_ptr(&p_dict->h_index, LETTER, newword.lettix)->letflags |= LET_TWO;
        break;

    /* all other words */
    default:
        {
        P_CACHEBLOCK p_cacheblock = NULL;
        S32 wordsize = 0;

        /* tokenise the word to be inserted */
        rootlen = (newword.fail == INS_STARTLET)
                  ? MAX(newword.matchc, 1)
                  : MAX(newword.matchcp, 1);
        tokenise(p_dict, &newword, rootlen);

        /* check if the root matches the current or previous words */
        rootlen = strlen32(newword.body);
        if(newword.fail == INS_WORD)
        {
            if(newword.match)
            {
                if(rootlen == newword.matchc)
                    newword.fail = INS_TOKENCUR;
            }
            else if(newword.matchp)
            {
                if(rootlen == newword.matchcp)
                    newword.fail = INS_TOKENPREV;
                else if(!spell_addword_nestf && (rootlen > newword.matchcp))
                {
                    P_U8 pos;
                    TOKWORD delword;
                    P_CACHEBLOCK p_cacheblock_t;

                    delword = newword;
                    xstrkpy(delword.bodyd, sizeof32(delword.bodyd), delword.bodydp);

                    p_cacheblock_t = cacheblock_goto_item(array_ptr(&p_dict->h_index, LETTER, delword.lettix)->p.cacheno);

                    pos = cacheblock_ptr(p_cacheblock_t, delword.findpos);

                    /* skip back to start of unit */
                    while(delword.findpos && (*--pos >= token_start))
                        --delword.findpos;

                    spell_addword_nestf = 1;
                    status_return(status = delreins(p_dict, word, &delword));
                    spell_addword_nestf = 0;
                    if(status)
                        break;
                }
            }
        }

        /* calculate space needed for word */
        switch(newword.fail)
        {
        /* 1 byte for root count,
           n bytes for body,
           1 byte for token */
        case INS_STARTLET:
            wordsize = 1 + rootlen - newword.matchc + 1;
            break;

        case INS_WORD:
            wordsize = 1 + rootlen - newword.matchcp + 1;
            break;

        /* 1 byte for token */
        case INS_TOKENPREV:
        case INS_TOKENCUR:
            wordsize = 1;
            break;
        }

        /* check we have a cache block */
        status_return(fetchblock(p_dict, newword.lettix));

        /* add word to cache block */
        err = STATUS_OK;

        cache_lock = 1;

        for(;;)
        {
            /* loop to get some memory */
            p_letter = array_ptr(&p_dict->h_index, LETTER, newword.lettix);

            if(NULL != (it =
                list_createitem(p_list_block_cache,
                                p_letter->p.cacheno,
                                p_letter->blklen +
                                wordsize +
                                sizeof32(CACHEBLOCK),
                                FALSE)))
            {
                p_cacheblock = list_itemcontents(CACHEBLOCK, it);
                break;
            }

            status_break(err = freecache(newword.lettix));
        }

        cache_lock = 0;

        IGNOREPARM(cache_lock);

        status_return(err);

        /* find place to insert new word */
        newpos = cacheblock_ptr(p_cacheblock, newword.findpos);
        switch(newword.fail)
        {
        case INS_TOKENCUR:
            /* skip to tokens of current word */
            while(*newpos < token_start)
            {
                ++newpos;
                ++newword.findpos;
            }
            break;

        case INS_TOKENPREV:
        case INS_WORD:
        case INS_STARTLET:
            break;
        }

        /* make space for new word */
        p_letter = array_ptr(&p_dict->h_index, LETTER, newword.lettix);
        err = (S32) p_letter->blklen - newword.findpos;
        memmove32(newpos + wordsize, newpos, p_letter->blklen - (S16) newword.findpos);

        p_letter->blklen = (S16) ((S32) p_letter->blklen + wordsize);

        /* move in word */
        switch(newword.fail)
        {
        case INS_STARTLET:
            *newpos++ = CH_NULL;
            ci = newword.body;
            while(*ci)
                *newpos++ = *ci++;
            *newpos++ = (U8) newword.tail;
            *newpos++ = (U8) newword.matchc;
            break;

            /*FALLTHRU*/ /* <<< err, it's not there */

        case INS_WORD:
            *newpos++ = (U8) newword.matchcp;
            ci = newword.body + newword.matchcp;
            while(*ci)
                *newpos++ = *ci++;

            /*FALLTHRU*/

        case INS_TOKENPREV:
        case INS_TOKENCUR:
            *newpos++ = (U8) newword.tail;
            break;
        }

        break;
        }
    }

    /* mark that it needs a write */
    array_ptr(&p_dict->h_index, LETTER, newword.lettix)->letflags |= LET_WRITE;

    return(1);
}

/******************************************************************************
*
* check if the word is in the dictionary
*
******************************************************************************/

_Check_return_
extern STATUS
spell_checkword(
    _InVal_     DICT_NUMBER dict_number,
    _In_z_      PC_SBSTR word)
{
    TOKWORD curword;
    P_DICT p_dict;

    status_return(dict_validate(&p_dict, dict_number));

    if(!makeindex(p_dict, &curword, word))
        return(create_error(SPELL_ERR_BADWORD));

    /* check if word exists and get position */
    return(lookupword(p_dict, &curword, FALSE));
}

/******************************************************************************
*
* close a dictionary
*
******************************************************************************/

_Check_return_
extern STATUS
spell_close(
    _InVal_     DICT_NUMBER dict_number)
{
    STATUS err = STATUS_OK;
    P_DICT p_dict;

    trace_0(TRACE_MODULE_SPELL, TEXT("spell_close"));

    status_return(dict_validate(&p_dict, dict_number));

    if(p_dict->file_handle_dict)
    {
        /* write out any modified part */
        err = ensuredict(p_dict);

        /* close file on media */
        status_accumulate(err, t5_file_close(&p_dict->file_handle_dict));
    }

    /* make sure no cache blocks left */
    stuffcache(p_dict);

    release_dict_entry(p_dict);

    return(err);
}

/******************************************************************************
*
* close the file associated with a dictionary;
* changes to the dictionary can be written to disk
* using spell_write_whole;
* use spell_close to close the dictionary completely
* and free all resources
*
******************************************************************************/

_Check_return_
extern STATUS
spell_close_file_only(
    _InVal_     DICT_NUMBER dict_number)
{
    STATUS err;
    P_DICT p_dict;

    trace_0(TRACE_MODULE_SPELL, TEXT("spell_close_file_only"));

    status_return(dict_validate(&p_dict, dict_number));

    assert(p_dict->file_handle_dict);

    /* free buffer if there is one */
    al_ptr_dispose(P_P_ANY_PEDANTIC(&p_dict->dictbuf));

    /* close file on media */
    err = t5_file_close(&p_dict->file_handle_dict);

    /* throw away file handle */
    p_dict->file_handle_dict = NULL;

    return(err);
}

#if WINDOWS /* RISC OS Fireworkz family does not use this - leave in Windows section so it still gets compiled sometimes */

/******************************************************************************
*
* create new dictionary
*
******************************************************************************/

_Check_return_
extern STATUS
spell_createdict(
    _In_z_      PCTSTR filename,
    _In_z_      PCTSTR def_name)
{
    STATUS status, err;
    S32 n_bytes, i;
    FILE_HANDLE def_file_handle = NULL;
    P_DICT p_dict;
    LETTER wix;
    U8Z buffer[255 + 1];

    /* first check to see if it exists */
    if(file_is_file(filename))
        return(create_error(SPELL_ERR_EXISTS));

    /* get a dictionary entry */
    status_return(err = get_dict_entry(&p_dict));

    /* dummy loop for structure */
    for(;;)
    {
        err = STATUS_OK;

        /* open definition file */
        status_break(err = t5_file_open(def_name, file_open_read, &def_file_handle, TRUE));

        /* position definition file */
        trace_0(TRACE_MODULE_SPELL, TEXT("spell_createdict about to def_file_position"));
        status_break(err = def_file_position(def_file_handle));
        trace_1(TRACE_MODULE_SPELL, TEXT("spell_createdict def_file_position returned: ") S32_TFMT, err);

        /* create a blank file */
        status_break(err = t5_file_open(filename, file_open_write, &p_dict->file_handle_dict, TRUE));

        trace_on();

        trace_0(TRACE_MODULE_SPELL, TEXT("spell_createdict about to write KEYSTR"));

        /* write out file identifier */
        n_bytes = strlen32(KEYSTR);
        status_break(err = file_write_bytes(KEYSTR, n_bytes, p_dict->file_handle_dict));

        /* copy across definition file */
        while((status = read_def_line(def_file_handle, buffer)) > 0)
        {
            trace_1(TRACE_MODULE_SPELL, TEXT("spell_createdict def line: %s"), report_sbstr(buffer));

            if(status_fail(err = file_write_bytes(buffer, (S32) status, p_dict->file_handle_dict)))
                goto error;
        }

        trace_1(TRACE_MODULE_SPELL, TEXT("spell_createdict after def file: ") S32_TFMT, status);

        /* stop on error */
        if(status_fail(status))
        {
            err = status;
            break;
        }

        /* write out definition end byte */
        status_break(err = file_putc(0, p_dict->file_handle_dict));

        /* write out dictionary flag byte */
        status_break(err = file_putc(0, p_dict->file_handle_dict));

        /* close file so far */
        status_break(err = t5_file_close(&p_dict->file_handle_dict));

        p_dict->file_handle_dict = 0;

        trace_1(TRACE_MODULE_SPELL, TEXT("spell_createdict err: ") S32_TFMT, err);

        /* re-open for update */
        status_break(err = t5_file_open(filename, file_open_readwrite, &p_dict->file_handle_dict, TRUE));

        /* process dictionary definition file */
        if(status_fail(status = load_dict_def(p_dict)))
        {
            err = status;
            break;
        }

        status_break(err = dict_seek_set(p_dict, p_dict->index_offset));

        /* get a blank structure */
        wix.p.disk   = 0;
        wix.blklen   = 0;
        wix.letflags = 0;

        /* write out blank structures */
        for(i = 0; i < p_dict->n_index; ++i)
        {
            if(status_fail(err = file_write_bytes(&wix, sizeof32(LETTER), p_dict->file_handle_dict)))
            {
                trace_1(TRACE_MODULE_SPELL, TEXT("spell_createdict failed to write out index entry ") S32_TFMT, i);
                goto error;
            }
        }

        err = t5_file_flush(p_dict->file_handle_dict);

        break;
        /*NOTREACHED*/
    }

error:

    trace_off();

    status_accumulate(err, t5_file_close(&p_dict->file_handle_dict));

    status_accumulate(err, t5_file_close(&def_file_handle));

    /* get rid of our entry */
    release_dict_entry(p_dict);

    if(status_fail(err))
    {
        status_assert(file_remove(filename)); /* have to ignore any close error - current takes precedence */
        return(err);
    }

    return(STATUS_OK);
}

#endif /* OS */

/******************************************************************************
*
* delete a word from a dictionary
*
******************************************************************************/

_Check_return_
extern STATUS
spell_deleteword(
    _InVal_     DICT_NUMBER dict_number,
    _In_z_      PC_SBSTR word)
{
    STATUS status;
    TOKWORD curword;
    S32 delsize, tokcount, addroot, blockbefore, i;
    U8 token_start, char_offset;
    P_U8 sp, p_data_end, p_data, endword, ci, co;
    P_CACHEBLOCK p_cacheblock;
    P_LETTER p_letter;
    P_DICT p_dict;

    status_return(dict_validate(&p_dict, dict_number));

    if(!makeindex(p_dict, &curword, word))
        return(create_error(SPELL_ERR_BADWORD));

    /* check if word exists and get position */
    status_return(status = lookupword(p_dict, &curword, TRUE));

    /* check if dictionary is read only */
    if(p_dict->dictflags & DICT_READONLY)
        return(create_error(SPELL_ERR_READONLY));

    if(!status)
        return(create_error(SPELL_ERR_WORDNOTFOUND));

    p_letter = array_ptr(&p_dict->h_index, LETTER, curword.lettix);
    p_letter->letflags |= LET_WRITE;

    /* deal with 1 and 2 letter words */
    if(curword.len == 1)
    {
        p_letter->letflags &= ~LET_ONE;
        return(STATUS_OK);
    }

    if(curword.len == 2)
    {
        p_letter->letflags &= ~LET_TWO;
        return(STATUS_OK);
    }

    /* check we have a cache block */
    status_return(fetchblock(p_dict, curword.lettix));

    token_start = p_dict->token_start;
    char_offset = p_dict->char_offset;

    /* after succesful find, the pointer points
    at the token of the word found */
    p_letter = array_ptr(&p_dict->h_index, LETTER, curword.lettix);
    p_cacheblock = cacheblock_goto_item(p_letter->p.cacheno);
    sp = cacheblock_ptr(p_cacheblock, 0);

    p_data = sp + curword.findpos;
    p_data_end = sp + p_letter->blklen;

    /* count the tokens */
    while(*p_data >= token_start)
        --p_data;

    ++p_data;
    tokcount = 0;
    while((*p_data >= token_start) && (p_data < p_data_end))
    {
        ++p_data;
        ++tokcount;
    }
    endword = p_data;

    /* calculate bytes to delete */
    if(tokcount == 1)
    {
        /* move to beginning of word */
        --p_data;
        while(*p_data >= char_offset)
            --p_data;

        /* last word in the block ? */
        if(endword == p_data_end)
            delsize = PtrDiffBytesS32(endword, p_data);
        else
        {
            if(*endword <= *p_data)
                delsize = PtrDiffBytesS32(endword, p_data);
            else
            {
                /* copy across the extra root required */
                addroot = ((S32) *endword - (S32) *p_data) + 1;
                delsize = PtrDiffBytesS32(endword, p_data) - addroot + 1;
                ci = p_data + addroot;
                co = endword + 1;
                for(i = 0; i < addroot; ++i)
                    *--co = *--ci;
            }
        }
    }
    else
    {
        delsize = 1;
        p_data = sp + curword.findpos;
    }

    p_letter = array_ptr(&p_dict->h_index, LETTER, curword.lettix);
    blockbefore = PtrDiffBytesS32(p_data, sp) + delsize;
    memmove32(p_data, p_data + delsize, p_letter->blklen - (S16) blockbefore);

    p_letter->letflags |= LET_WRITE;
    p_letter->blklen = (S16) ((S32) p_letter->blklen - delsize);
    return(STATUS_OK);
}

#if WINDOWS /* RISC OS Fireworkz family does not use this - leave in Windows section so it still gets compiled sometimes */

/******************************************************************************
*
* flush a dictionary
*
******************************************************************************/

_Check_return_
extern STATUS
spell_flush(
    _InVal_     DICT_NUMBER dict_number)
{
    STATUS err;
    P_DICT p_dict;

    trace_0(TRACE_MODULE_SPELL, TEXT("spell_flush"));

    status_return(dict_validate(&p_dict, dict_number));

    assert(p_dict->file_handle_dict);

    /* write out any modified part */
    err = ensuredict(p_dict);

    /* make sure no cache blocks left */
    stuffcache(p_dict);

    return(err);
}

#endif /* OS */

/******************************************************************************
*
* is the character upper case ?
*
******************************************************************************/

_Check_return_
extern STATUS
spell_isupper(
    _InVal_     DICT_NUMBER dict_number,
    _InVal_     S32 ch)
{
    P_DICT p_dict;
    S32 ch_upper;

    status_return(dict_validate(&p_dict, dict_number));

    ch_upper = toupper_us(p_dict, ch);

    return(ch_upper == ch);
}

/******************************************************************************
*
* report whether a character is part
* of a valid spellcheck word
*
******************************************************************************/

_Check_return_
extern STATUS
spell_iswordc(
    _InVal_     DICT_NUMBER dict_number,
    _InVal_     S32 ch)
{
    P_DICT p_dict;

    status_return(dict_validate(&p_dict, dict_number));

    return(iswordc_us(p_dict, ch));
}

/******************************************************************************
*
* load a dictionary
*
* all the dictionary is loaded and locked into place
*
******************************************************************************/

_Check_return_
extern STATUS
spell_load(
    _InVal_     DICT_NUMBER dict_number)
{
    ARRAY_INDEX i;
    P_DICT p_dict;
    P_LETTER p_letter;

    status_return(dict_validate(&p_dict, dict_number));

    for(i = 0, p_letter = array_ptr(&p_dict->h_index, LETTER, i); i < p_dict->n_index; i += 1, p_letter += 1)
    {
        if(p_letter->blklen)
        {
            status_return(fetchblock(p_dict, i));
            p_letter->letflags |= LET_LOCKED;
        }
    }

    return(STATUS_OK);
}

/******************************************************************************
*
* return the next word in a dictionary
*
* --in--
* wordout must point to a character
* buffer at least MAX_WORD long
*
* --out--
* <0 error
* =0 end of dictionary
* >0 word returned
*
******************************************************************************/

_Check_return_
extern STATUS
spell_nextword(
    _InVal_     DICT_NUMBER dict_number,
    _Out_writes_z_(sizeof_wordout) P_U8 wordout,
    _InVal_     S32 sizeof_wordout,
    _In_z_      PC_SBSTR wordin,
    _In_opt_z_  PC_U8Z mask,
    _InoutRef_  P_S32 brkflg)
{
    STATUS status;
    STATUS gotw;
    P_DICT p_dict;

    trace_5(TRACE_MODULE_SPELL, TEXT("spell_nextword(") S32_TFMT TEXT(", ") PTR_XTFMT TEXT(", %s, %s, ") PTR_XTFMT TEXT(")"),
            dict_number, wordout, report_sbstr(wordin), report_sbstr(mask), brkflg);

    assert(sizeof_wordout != 0);
    wordout[0] = CH_NULL;

    status_return(dict_validate(&p_dict, dict_number));

    if(badcharsin(p_dict, wordin))
        return(create_error(SPELL_ERR_BADWORD));

    if(mask && badcharsin(p_dict, mask))
        return(create_error(SPELL_ERR_BADWORD));

    /* check for start of dictionary */
    if((gotw = initmatch(p_dict, wordout, sizeof_wordout, wordin, mask)) != 0)
        status_return(gotw = spell_checkword(dict_number, wordout));

    do
    {
        if(*brkflg)
            return(create_error(SPELL_ERR_ESCAPE));

        if(!gotw)
        {
            status_return(status = nextword(p_dict, wordout));
        }
        else
        {
            status = 1;
            gotw = 0;
        }
    }
    while(status &&
          matchword(p_dict, mask, wordout) &&
          ((status = !endmatch(p_dict, wordout, mask, 1)) != 0));

    /* return blank word at end */
    if(!status)
        *wordout = CH_NULL;

    trace_2(TRACE_MODULE_SPELL, TEXT("spell_nextword yields %s, status = ") S32_TFMT, report_sbstr(wordout), status);
    return(status);
}

/******************************************************************************
*
* open a dictionary
*
* --out--
* dictionary handle
*
******************************************************************************/

_Check_return_
extern STATUS /*DICT_NUMBER*/
spell_opendict(
    _In_z_      PCTSTR filename,
    _OutRef_    P_PCTSTR copy_right,
    _InVal_     BOOL load_and_close_file_after)
{
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(LETTER), TRUE);

    STATUS err;
    DICT_NUMBER dict_number;
    S32 i, nmemb;
    P_DICT p_dict;
    P_LETTER p_letter;

    *copy_right = CH_NULL;

#if CHECKING
    { /* trap U8s being signed */
    U8 ch = '\x80';
    assert(ch > 127);
    } /*block*/
#endif

    /* get a dictionary number */
    status_return(err = get_dict_entry(&p_dict));
    dict_number = (DICT_NUMBER) err;

    for(;;) /* loop for structure */
    {
        P_LETTER p_index;
        filelength_t filelength;

        /* look for the file */
        status_break(err = t5_file_open(filename, file_open_read, &p_dict->file_handle_dict, TRUE));

        /* take copy of name for reopening in setoptions */
        status_break(err = al_tstr_set(&p_dict->h_dict_filename, filename));

        /* load dictionary definition */
        status_break(err = load_dict_def(p_dict));

        if(NULL == (p_index = al_array_alloc(&p_dict->h_index, LETTER, p_dict->n_index, &array_init_block, &err)))
            break;

        /* load index */
        nmemb = p_dict->n_index;
        status_break(err = file_read_bytes_requested(p_index, sizeof32(LETTER) * nmemb, p_dict->file_handle_dict));

        /* read size of dictionary file */
        status_break(err = file_length(p_dict->file_handle_dict, &filelength));

        assert(0 == filelength.u.words.hi); /* dictionaries are small */
        assert((S32) filelength.u.words.lo >= 0);
        p_dict->dictsize = (S32) filelength.u.words.lo - p_dict->data_offset;

        /* if dictionary can be updated, re-open for update */
        if(!(p_dict->dictflags & DICT_READONLY) && !load_and_close_file_after)
        {
            status_assert(t5_file_close(&p_dict->file_handle_dict));

            al_ptr_dispose(P_P_ANY_PEDANTIC(&p_dict->dictbuf));

            /* look for the file */
            status_break(err = t5_file_open(filename, file_open_readwrite, &p_dict->file_handle_dict, TRUE));
        }

        break; /* out of loop for structure */
        /*NOTREACHED*/
    }

    if(status_fail(err))
    {
        if(p_dict->file_handle_dict)
            status_assert(t5_file_close(&p_dict->file_handle_dict));
        release_dict_entry(p_dict);
        return(err);
    }

    /* loop over index, masking off unwanted bits */
    for(i = 0, p_letter = array_ptr(&p_dict->h_index, LETTER, 0); i < p_dict->n_index; ++i, ++p_letter)
        p_letter->letflags &= (LET_ONE | LET_TWO);

    /* return copyright string */
    *copy_right = array_tstr(&p_dict->h_dict_name);

    if(load_and_close_file_after)
    {
        if(status_ok(err = spell_load(dict_number)))
            err = spell_close_file_only(dict_number);

        if(status_fail(err))
        {
            status_consume(spell_close(dict_number));
            return(err);
        }
    }

    trace_1(TRACE_MODULE_SPELL, TEXT("spell_opendict returns: ") S32_TFMT, dict_number);

    return(dict_number);
}

#if WINDOWS /* RISC OS Fireworkz family does not use this - leave in Windows section so it still gets compiled sometimes */

/******************************************************************************
*
* pack a dictionary
*
******************************************************************************/

_Check_return_
extern STATUS
spell_pack(
    _InVal_     DICT_NUMBER old_dict_number,
    _InVal_     DICT_NUMBER new_dict_number)
{
    STATUS err = STATUS_OK;
    S32 i, diskpoint;
    P_CACHEBLOCK p_cacheblock;
    P_DICT p_dict_old, p_dict_new;
    P_LETTER p_letter_in, p_letter_out;

    status_return(dict_validate(&p_dict_old, old_dict_number));
    status_return(dict_validate(&p_dict_new, new_dict_number));

    status_return(ensuredict(p_dict_old));

    diskpoint = p_dict_new->data_offset;

    for(i = 0; i < p_dict_old->n_index; ++i)
    {
        p_letter_in  = array_ptr(&p_dict_old->h_index, LETTER, i);
        p_letter_out = array_ptr(&p_dict_new->h_index, LETTER, i);

        /* if no block, copy index entries and continue */
        if(!p_letter_in->blklen)
        {
            *p_letter_out = *p_letter_in;
            p_letter_in->letflags &= LET_ONE | LET_TWO;
            p_letter_out->letflags |= LET_WRITE;
            continue;
        }

        status_break(err = fetchblock(p_dict_old, i));

        /* re-load index pointers */
        p_letter_in  = array_ptr(&p_dict_old->h_index, LETTER, i);
        p_letter_out = array_ptr(&p_dict_new->h_index, LETTER, i);

        /* clear input index flags */
        *p_letter_out = *p_letter_in;
        p_letter_in->letflags &= LET_ONE | LET_TWO;

        p_cacheblock = cacheblock_goto_item(p_letter_in->p.cacheno);

        /* output index takes over block read from input index */
        p_letter_in->p.disk = p_cacheblock->diskaddress;
        p_cacheblock->diskaddress = diskpoint;
        diskpoint += p_letter_out->blklen;
        diskpoint += sizeof32(S32);
        p_cacheblock->diskspace = p_letter_out->blklen;
        p_cacheblock->dict_number = new_dict_number;
        p_cacheblock->lettix = i;
        p_letter_out->letflags |= LET_WRITE;
    }

    p_dict_new->dictflags = p_dict_old->dictflags;

    return(err);
}

#endif /* OS */

/******************************************************************************
*
* return the previous word
*
******************************************************************************/

_Check_return_
extern STATUS
spell_prevword(
    _InVal_     DICT_NUMBER dict_number,
    _Out_writes_z_(sizeof_wordout) P_SBSTR wordout,
    _InVal_     S32 sizeof_wordout,
    _In_z_      PC_SBSTR wordin,
    _In_opt_z_  PC_U8Z mask,
    _InoutRef_  P_S32 brkflg)
{
    STATUS status;
    P_DICT p_dict;

    trace_5(TRACE_MODULE_SPELL, TEXT("spell_prevword(") S32_TFMT TEXT(", ") PTR_XTFMT TEXT(", %s, %s, ") PTR_XTFMT TEXT(")"),
            dict_number, wordout, report_sbstr(wordin), report_sbstr(mask), brkflg);

    assert(sizeof_wordout != 0);
    wordout[0] = CH_NULL;

    status_return(dict_validate(&p_dict, dict_number));

    if(badcharsin(p_dict, wordin))
        return(create_error(SPELL_ERR_BADWORD));

    if(mask && badcharsin(p_dict, mask))
        return(create_error(SPELL_ERR_BADWORD));

    initmatch(p_dict, wordout, sizeof_wordout, wordin, mask);

    do  {
        if(*brkflg)
            return(create_error(SPELL_ERR_ESCAPE));

        status_return(status = prevword(p_dict, wordout));
    }
    while(status &&
          matchword(p_dict, mask, wordout) &&
          ((status = !endmatch(p_dict, wordout, mask, 0)) != 0));

    /* return blank word at start */
    if(!status)
        *wordout = CH_NULL;

    trace_2(TRACE_MODULE_SPELL, TEXT("spell_prevword yields %s, status = ") S32_TFMT, report_sbstr(wordout), status);
    return(status);
}

#if WINDOWS /* RISC OS Fireworkz family does not use this - leave in Windows section so it still gets compiled sometimes */

/******************************************************************************
*
* set dictionary options
*
******************************************************************************/

_Check_return_
extern STATUS
spell_setoptions(
    _InVal_     DICT_NUMBER dict_number,
    _InVal_     S32 optionset,
    _InVal_     S32 optionmask)
{
    P_DICT p_dict;

    status_return(dict_validate(&p_dict, dict_number));

    /* may need to open for update! */
    if(p_dict->dictflags & DICT_READONLY)
    {
        if(t5_file_close(&p_dict->file_handle_dict))
            p_dict->file_handle_dict = NULL;
        else
            status_assert(t5_file_open(array_tstr(&p_dict->h_dict_filename), file_open_readwrite, &p_dict->file_handle_dict, TRUE));

        if(!p_dict->file_handle_dict)
        {
            /* if failed to open for update, reopen for reading */
            status_assert(t5_file_open(array_tstr(&p_dict->h_dict_filename), file_open_read, &p_dict->file_handle_dict, TRUE));
            return(create_error(SPELL_ERR_CANTWRITE));
        }
    }

    p_dict->dictflags &= (U8) optionmask;
    p_dict->dictflags |= (U8) optionset;

    status_return(dict_seek_set(p_dict, p_dict->index_offset - 1));

    status_return(file_putc((S32) p_dict->dictflags & DICT_READONLY, p_dict->file_handle_dict));

    return(STATUS_OK);
}

/******************************************************************************
*
* return statistics about spelling checker
*
******************************************************************************/

extern void
spell_stats(
    P_S32 cblocks,
    P_S32 largest,
    P_S32 totalmem)
{
    LIST_ITEMNO cacheno = 0;
    P_LIST_ITEM it;
    S32 blksiz;

    *cblocks = 0;
    *largest = 0;
    *totalmem = 0;

    if(NULL != (it = list_initseq(p_list_block_cache, &cacheno)))
    {
        do  {
            P_CACHEBLOCK p_cacheblock = list_itemcontents(CACHEBLOCK, it);
            P_DICT p_dict = array_ptr(&h_dict_table, DICT, p_cacheblock->dict_number);

            ++(*cblocks);
            blksiz = (S32) sizeof32(CACHEBLOCK) + array_ptr(&p_dict->h_index, LETTER, p_cacheblock->lettix)->blklen;
            *largest = MAX(*largest, blksiz);
            *totalmem += blksiz;
        }
        while(NULL != (it = list_nextseq(p_list_block_cache, &cacheno)));
    }
}

#endif /* OS */

extern void
spell_startup(void)
{
    /* SKS 03jan94 initialise statics for Windows restart */
    h_dict_table = 0;
    p_list_block_cache = NULL;
    cache_lock = 0;
}

#if WINDOWS /* RISC OS Fireworkz family does not use this - leave in Windows section so it still gets compiled sometimes */

/******************************************************************************
*
* compare two strings using the case and order
* information for a dictionary
*
******************************************************************************/

_Check_return_
extern STATUS
spell_strnicmp(
    _InVal_     DICT_NUMBER dict_number,
    _In_z_      PC_SBSTR word1,
    _In_z_      PC_SBSTR word2,
    _InVal_     S32 len)
{
    P_DICT p_dict;

    status_return(dict_validate(&p_dict, dict_number));

    return(strnicmp_us(p_dict, word1, word2, len));
}

#endif /* OS */

/******************************************************************************
*
* convert a character to lower case
* using the dictionary's mapping
*
******************************************************************************/

_Check_return_
extern STATUS
spell_tolower(
    _InVal_     DICT_NUMBER dict_number,
    _InVal_     S32 ch)
{
    P_DICT p_dict;

    status_return(dict_validate(&p_dict, dict_number));

    return(tolower_us(p_dict, ch));
}

/******************************************************************************
*
* convert a character to upper case
* using the dictionary's mapping
*
******************************************************************************/

_Check_return_
extern STATUS
spell_toupper(
    _InVal_     DICT_NUMBER dict_number,
    _InVal_     S32 ch)
{
    P_DICT p_dict;

    status_return(dict_validate(&p_dict, dict_number));

    return(toupper_us(p_dict, ch));
}

#if WINDOWS /* RISC OS Fireworkz family does not use this - leave in Windows section so it still gets compiled sometimes */

/******************************************************************************
*
* unlock a dictionary
*
******************************************************************************/

_Check_return_
extern STATUS
spell_unlock(
    _InVal_     DICT_NUMBER dict_number)
{
    S32 i, n_index;
    P_LETTER p_letter;
    P_DICT p_dict;

    status_return(dict_validate(&p_dict, dict_number));

    for(i = 0, p_letter = array_ptr(&p_dict->h_index, LETTER, 0), n_index = p_dict->n_index;
        i < n_index;
        ++i, ++p_letter)
        p_letter->letflags &= ~LET_LOCKED;

    return(STATUS_OK);
}

#endif /* OS */

/******************************************************************************
*
* write out whole dictionary, index, data and all
* in pre-packed form
*
******************************************************************************/

_Check_return_
extern STATUS
spell_write_whole(
    _InVal_     DICT_NUMBER dict_number)
{
    STATUS status = STATUS_OK;
    P_DICT p_dict;
    BOOL modified;
    P_LETTER p_letter;
    ARRAY_INDEX i;

    status_return(dict_validate(&p_dict, dict_number));

    /* check if some part of the dictionary has been modified */
    for(i = 0, modified = 0, p_letter = array_ptr(&p_dict->h_index, LETTER, i); i < p_dict->n_index; i += 1, p_letter += 1)
        if(p_letter->letflags & LET_WRITE)
            modified = 1;

    if(modified)
    {
        S32 diskpoint;

        /* whole dictionary must be in memory - ensure that it is */
        status_return(spell_load(dict_number));

        if(!p_dict->file_handle_dict)
            status_assert(t5_file_open(array_tstr(&p_dict->h_dict_filename), file_open_readwrite, &p_dict->file_handle_dict, TRUE));

        if(!p_dict->file_handle_dict)
            return(create_error(SPELL_ERR_CANTWRITE));

        diskpoint = p_dict->data_offset;

        /* loop over data, setting optimal disk addresses */
        for(i = 0, p_letter = array_ptr(&p_dict->h_index, LETTER, i); i < p_dict->n_index; i += 1, p_letter += 1)
        {
            if(p_letter->blklen)
            {
                P_CACHEBLOCK p_cacheblock = cacheblock_goto_item(p_letter->p.cacheno);

                p_cacheblock->diskaddress = diskpoint;
                diskpoint += p_letter->blklen;
                diskpoint += sizeof32(S32);
                p_cacheblock->diskspace = p_letter->blklen;
                p_letter->letflags |= LET_WRITE;
            }
        }

        /* now write it out */
        status = ensuredict(p_dict);
    }

    return(status);
}

/******************************************************************************
*
* say whether a character is valid
* as the first letter of a word
*
* --out--
* =0 character is invalid
*
******************************************************************************/

_Check_return_
extern STATUS
spell_valid_1(
    _InVal_     DICT_NUMBER dict_number,
    _InVal_     S32 ch)
{
    P_DICT p_dict;

    status_return(dict_validate(&p_dict, dict_number));

    return(char_ordinal_1(p_dict, (U8) toupper_us(p_dict, ch)) >= 0);
}

/******************************************************************************
*
* ensure string contains only U8s valid for wild match
*
******************************************************************************/

_Check_return_
static BOOL
badcharsin(
    _InoutRef_  P_DICT p_dict,
    _In_z_      PC_SBSTR str)
{
    U8 ch;

    while((ch = *str++) != 0)
        if(!(iswordc_us(p_dict, (S32) ch) ||
             (ch == SPELL_WILD_SINGLE)    ||
             (ch == SPELL_WILD_MULTIPLE)))
            return(TRUE);

    return(FALSE);
}

/******************************************************************************
*
* convert a character to an ordinal number
* giving the index for the first character
* position
*
* --out--
* < 0 character not valid in first position
* >=0 ordinal number (offset by char_offset)
*
******************************************************************************/

_Check_return_
static STATUS
char_ordinal_1(
    _InoutRef_  P_DICT p_dict,
    _InVal_     U8 ch)
{
    U8 i;

    for(i = 0; i < p_dict->n_index_1; ++i)
        if(p_dict->letter_1[i] == ch)
            return((S32) i + (S32) p_dict->char_offset);

    return(SPELL_ERR_BADWORD);
}

/******************************************************************************
*
* convert a character to an ordinal number
* giving the index for the second character
* position
*
* --out--
* < 0 character not valid in 2nd position
* >=0 ordinal number (offset by char_offset)
*
******************************************************************************/

_Check_return_
static STATUS
char_ordinal_2(
    _InoutRef_  P_DICT p_dict,
    _InVal_     U8 ch)
{
    U8 i;

    for(i = 0; i < p_dict->n_index_2; ++i)
        if(p_dict->letter_2[i] == ch)
            return((S32) i + (S32) p_dict->char_offset);

    return(SPELL_ERR_BADWORD);
}

/******************************************************************************
*
* convert a character to an ordinal number
* giving the index for the third character
* position
*
* --out--
* < 0 character not valid in 3rd position
* >=0 ordinal number (offset by char_offset)
*
******************************************************************************/

_Check_return_
static STATUS
char_ordinal_3(
    _InoutRef_  P_DICT p_dict,
    _InVal_     U8 ch)
{
    U8 i;

    for(i = 0; i < p_dict->n_index_2; ++i)
        if(p_dict->letter_2[i] == ch)
            return(p_dict->man_token_start ? (S32) ch : (S32) i + (S32) p_dict->char_offset);

    return(SPELL_ERR_BADWORD);
}

/******************************************************************************
*
* compare two strings for sort routine
*
******************************************************************************/

#if WINDOWS && 0

PROC_QSORT_S_PROTO(static, compar, DICT, PC_SBSTR)
{
    const P_DICT p_dict_compar = (P_DICT) context;
    QSORT_ARG2_VAR_DECL(P_PC_SBSTR, word_1);
    QSORT_ARG2_VAR_DECL(P_PC_SBSTR, word_2);

    return(strnicmp_us(p_dict_compar, *word_1, *word_2, -1));
}

#else

static P_DICT p_dict_compar; /* dictionary compar needs */

PROC_QSORT_PROTO(static, compar, PC_SBSTR)
{
    QSORT_ARG1_VAR_DECL(P_PC_SBSTR, word_1);
    QSORT_ARG2_VAR_DECL(P_PC_SBSTR, word_2);

    return(strnicmp_us(p_dict_compar, *word_1, *word_2, -1));
}

#endif /* OS */

/******************************************************************************
*
* compare two endings for alphabetic value for sort routine
*
******************************************************************************/

/* no context needed for qsort() */

PROC_QSORT_PROTO(static, compar_ending_alpha, ENDING)
{
    QSORT_ARG1_VAR_DECL(PC_ENDING, p_ending_1);
    QSORT_ARG2_VAR_DECL(PC_ENDING, p_ending_2);

    const PC_SBSTR word_ending_1 = p_ending_1->ending;
    const PC_SBSTR word_ending_2 = p_ending_2->ending;

    return(/*"C"*/strcmp(word_ending_1, word_ending_2));
}

/******************************************************************************
*
* compare two endings for length value for sort routine
* secondary sort on original order in definition file
*
* (sort on length is in reverse order)
*
******************************************************************************/

/* no context needed for qsort() */

PROC_QSORT_PROTO(static, compar_ending_pos, ENDING)
{
    QSORT_ARG1_VAR_DECL(PC_ENDING, p_end1);
    QSORT_ARG2_VAR_DECL(PC_ENDING, p_end2);

    return(p_end1->len == p_end2->len
                ? (p_end1->pos == p_end2->pos
                    ? 0
                    : (p_end1->pos > p_end2->pos
                        ? 1
                        : -1))
                : (p_end1->len > p_end2->len
                    ? -1
                    : 1));
}

/******************************************************************************
*
* take a tokenised and indexed word from
* a word structure, and return the real word
*
******************************************************************************/

static S32
decodeword(
    _InoutRef_  P_DICT p_dict,
    P_U8 word,
    P_TOKWORD p_tokword,
    _InVal_     S32 len)
{
    P_U8 ci;
    P_U8 co;

    *(word + 0) = (U8)
                  tolower_us(p_dict,
                             ordinal_char_1(p_dict,
                                            p_tokword->lettix / p_dict->n_index_2 +
                                            p_dict->char_offset));

    if(len == 1)
    {
        *(word + 1) = CH_NULL;
        return(1);
    }

    *(word + 1) = (U8)
                  tolower_us(p_dict,
                             ordinal_char_2(p_dict,
                                            p_tokword->lettix % p_dict->n_index_2 +
                                            p_dict->char_offset));

    if(len == 2)
    {
        *(word + 2) = CH_NULL;
        return(2);
    }

    /* decode body */
    ci = p_tokword->bodyd;
    co = word + 2;
    while(*ci)
        *co++ = (U8) tolower_us(p_dict, ordinal_char_3(p_dict, *ci++));

    /* decode ending */
    ci = ending_list_goto_item(&p_dict->dict_end_list, (LIST_ITEMNO) p_tokword->tail)->ending;

    while(*ci)
        *co++ = (U8) tolower_us(p_dict, ordinal_char_3(p_dict, *ci++));
    *co = CH_NULL;

    return(strlen32(word));
}

#if WINDOWS

/******************************************************************************
*
* if the definition file is a dictionary, find out
* and position the file pointer for reading the
* definition file
*
******************************************************************************/

_Check_return_
static STATUS
def_file_position(
    _InoutRef_  FILE_HANDLE def_file_handle)
{
    U8 keystr[sizeof32(KEYSTR)];
    S32 keylen, n_bytes;

    trace_0(TRACE_MODULE_SPELL, TEXT("def_file_position"));

    /* position to start of file */
    status_return(file_rewind(def_file_handle));

    /* read key string to determine if it's a dictionary */
    n_bytes = strlen32(KEYSTR);
    *keystr = CH_NULL;
    status_return(file_read_bytes_requested(keystr, n_bytes, def_file_handle));

    if(0 == memcmp32(keystr, KEYSTR, strlen32(KEYSTR)))
        keylen = strlen32(KEYSTR);
    else
        keylen = 0;

    status_return(file_seek(def_file_handle, keylen, NULL /*hi*/, SEEK_SET));

    trace_1(TRACE_MODULE_SPELL, TEXT("def_file_position keylen: ") S32_TFMT, keylen);

    return(STATUS_OK);
}

#endif

/******************************************************************************
*
* delete a cache block from the list,
* adjusting cache numbers for the deletion
*
******************************************************************************/

static void
deletecache(
    _In_        LIST_ITEMNO cacheno)
{
    LIST_ITEMNO i;

    /* remove cache block */
    trace_2(TRACE_MODULE_SPELL, TEXT("deleting cache block: %d, %d items on list"),
            cacheno, list_numitem(p_list_block_cache));
    PTR_ASSERT(p_list_block_cache);
    list_deleteitems(p_list_block_cache, cacheno, (LIST_ITEMNO) 1);

    /* adjust cache numbers below */
    for(i = list_atitem(p_list_block_cache); i < list_numitem(p_list_block_cache); ++i)
    {
        P_CACHEBLOCK p_cacheblock = cacheblock_goto_item(i);
        P_DICT p_dict = array_ptr(&h_dict_table, DICT, p_cacheblock->dict_number);

        trace_2(TRACE_MODULE_SPELL,
                TEXT("p_cacheblock: ") PTR_XTFMT TEXT(", ixp: ") PTR_XTFMT,
                p_cacheblock,
                array_ptr(&p_dict->h_index, LETTER, p_cacheblock->lettix));

        array_ptr(&p_dict->h_index, LETTER, p_cacheblock->lettix)->p.cacheno = i;

        trace_2(TRACE_MODULE_SPELL,
                TEXT("deletecache adjusted: %d, numitems: %d"),
                i,
                list_numitem(p_list_block_cache));
    }

    if(!list_numitem(p_list_block_cache))
    {
        list_free(p_list_block_cache);
        al_ptr_dispose(P_P_ANY_PEDANTIC(&p_list_block_cache));
    }
}

/******************************************************************************
*
* delete a word unit from the dictionary -
* the root and all the endings, then insert
* the word we were trying to insert but
* couldn't because it was alphabetically in
* the middle of the unit, then re-insert all
* the words in the deleted unit
*
******************************************************************************/

_Check_return_
static STATUS
delreins(
    _InoutRef_  P_DICT p_dict,
    _In_z_      PC_SBSTR word,
    P_TOKWORD p_tokword)
{
    STATUS err = STATUS_OK;
    P_U8Z deadwords[MAX_TOKEN - MIN_TOKEN + 1];
    U8 realword[BUF_MAX_WORD];
    S32 wordc = 0, i;
    U8 token_start;
    P_U8 p_data, p_data_end;
    P_CACHEBLOCK p_cacheblock;
    P_LETTER p_letter;
    S32 len;

    token_start = p_dict->token_start;
    p_letter = array_ptr(&p_dict->h_index, LETTER, p_tokword->lettix);
    p_cacheblock = cacheblock_goto_item(p_letter->p.cacheno);
    p_data = p_data_end = cacheblock_ptr(p_cacheblock, 0);

    p_data += p_tokword->findpos;
    p_data_end += p_letter->blklen;

    /* extract each word */
    cache_lock = 1;
    while((p_data < p_data_end) && (*p_data >= token_start))
    {
        p_tokword->tail = (S32) *p_data - (S32) token_start;
        len = decodeword(p_dict, realword, p_tokword, 0);
        if(NULL == (deadwords[wordc] = al_ptr_alloc_bytes(P_U8Z, len + 1, &err)))
            break;
        memcpy32(deadwords[wordc++], realword, len + 1);
        p_cacheblock = cacheblock_goto_item(p_letter->p.cacheno);
        ++p_data;
    }
    cache_lock = 0;

    status_return(err);

    /* check if any words are out of range */
    for(i = 0; i < wordc; ++i)
        if(strnicmp_us(p_dict, deadwords[i], word, -1) > 0)
            break;

    /* if none out of range, free memory and exit */
    if(i == wordc)
    {
        for(i = 0; i < wordc; ++i)
            al_ptr_dispose(P_P_ANY_PEDANTIC(&deadwords[i]));
        return(STATUS_OK);
    }

    /* delete the words */
    for(i = 0; i < wordc; ++i)
    {
        U8 word_buf[BUF_MAX_WORD];
        xstrkpy(word_buf, sizeof32(word_buf), deadwords[i]);
        status_return(spell_deleteword(dict_number(p_dict), word_buf));
    }

    /* add the new word */
    len = strlen32(word);
    if(NULL == (deadwords[wordc] = al_ptr_alloc_bytes(P_U8Z, len + 1, &err)))
        return(err);
    memcpy32(deadwords[wordc++], word, len + 1);

    /* and put back all the words we deleted */
    p_dict_compar = p_dict;
    qsort(deadwords, (U32) wordc, sizeof32(deadwords[0]), compar);
    for(i = 0; i < wordc; ++i)
    {
        U8 word_buf[BUF_MAX_WORD];
        xstrkpy(word_buf, sizeof32(word_buf), deadwords[i]);
        status_return(spell_addword(dict_number(p_dict), word_buf));
        al_ptr_dispose(P_P_ANY_PEDANTIC(&deadwords[i]));
    }

    return(1);
}

/******************************************************************************
*
* detect the end of possible matches
*
******************************************************************************/

static S32
endmatch(
    _InoutRef_  P_DICT p_dict,
    _In_z_      PC_SBSTR word,
    _In_opt_z_  PC_U8Z mask,
    _InVal_     S32 updown)
{
    STATUS status;
    S32 len;
    PC_U8Z ci;

    if(!mask || !*mask)
        return(0);

    len = 0;
    ci = mask;
    while(iswordc_us(p_dict, (S32) *ci))
    {
        ++ci;
        ++len;
    }

    if(!len)
        return(0);

    status = strnicmp_us(p_dict, mask, word, len);

    return(updown ? (status >= 0) ? 0 : 1
                  : (status <= 0) ? 0 : 1);
}

/******************************************************************************
*
* ensure that any modified parts of a
* dictionary are written out to the disk
*
******************************************************************************/

_Check_return_
static STATUS
ensuredict(
    _InoutRef_  P_DICT p_dict)
{
    STATUS err = STATUS_OK, allerr = STATUS_OK;
    S32 i;
    P_LETTER p_letter;

    trace_0(TRACE_MODULE_SPELL, TEXT("ensuredict"));

    /* work down the index and write out anything altered */
    for(i = 0, p_letter = array_ptr(&p_dict->h_index, LETTER, i); i < p_dict->n_index; i += 1, p_letter += 1)
    {
        trace_1(TRACE_MODULE_SPELL, TEXT("ensure letter: ") S32_TFMT, i);
        if(p_letter->letflags & LET_WRITE)
        {
            if(p_letter->letflags & LET_CACHED)
                err = killcache(p_letter->p.cacheno);
            else
            {
                /* mask flags to be written to disk */
                p_letter->letflags &= LET_ONE | LET_TWO;
                err = writeindex(p_dict, i);
            }

            if(status_fail(err))
                allerr = status_fail(allerr) ? allerr : err;
            else
                p_letter->letflags &= ~LET_WRITE;
        }
    }

    return(allerr);
}

/******************************************************************************
*
* fetch a block of the dictionary
*
******************************************************************************/

_Check_return_
static STATUS
fetchblock(
    _InoutRef_  P_DICT p_dict,
    _InVal_     S32 lettix)
{
    STATUS err = STATUS_OK;
    P_CACHEBLOCK p_cacheblock_new;
    P_LIST_ITEM it;
    S32 n_bytes;
    P_LETTER p_letter;

    /* check if it's already cached */
    if(array_ptr(&p_dict->h_index, LETTER, lettix)->letflags & LET_CACHED)
        return(STATUS_OK);

    trace_3(TRACE_MODULE_SPELL, TEXT("fetchblock dict: ") S32_TFMT TEXT(", letter: ") S32_TFMT TEXT(", p_list_block_cache: ") PTR_XTFMT,
            dict_number(p_dict), lettix, p_list_block_cache);

    /* allocate a list block if we don't have one */
    if(NULL == p_list_block_cache)
    {
        if(NULL != (p_list_block_cache = al_ptr_alloc_elem(LIST_BLOCK, 1, &err)))
        {
            list_init(p_list_block_cache);
            trace_0(TRACE_MODULE_SPELL, TEXT("fetchblock has allocated cache list block"));
        }
    }

    /* get a space to receive the block */
    it = NULL;
    if(status_ok(err))
    {
        assert(!IS_P_DATA_NONE(p_list_block_cache));
        cache_lock = 1;
        for(;;)
        {
            trace_0(TRACE_MODULE_SPELL, TEXT("fetchblock doing createitem"));
            if(NULL != (it =
                list_createitem(p_list_block_cache,
                                list_numitem(p_list_block_cache),
                                (S32) sizeof32(CACHEBLOCK) + (S32) array_ptr(&p_dict->h_index, LETTER, lettix)->blklen,
                                FALSE)))
                break;

            trace_0(TRACE_MODULE_SPELL, TEXT("fetchblock doing freecache"));
            status_break(err = freecache(-1));
        }
        cache_lock = 0;
    }

    status_return(err);

    PTR_ASSERT(it);
    p_cacheblock_new = list_itemcontents(CACHEBLOCK, it);
    p_letter = array_ptr(&p_dict->h_index, LETTER, lettix);

    /* read the data if there is any */
    if(p_letter->p.disk && p_letter->blklen /* MRJC 25.3.93 */)
    {
        /* position for the read */
        trace_0(TRACE_MODULE_SPELL, TEXT("fetchblock doing seek"));
        if(status_fail(err = dict_seek_set(p_dict, p_letter->p.disk)))
        {
            deletecache(list_atitem(p_list_block_cache));
            return(err);
        }

        /* read in the block */
        trace_0(TRACE_MODULE_SPELL, TEXT("fetchblock doing read"));
        n_bytes = (S32) p_letter->blklen + (S32) sizeof32(S32);
        if(status_fail(err = file_read_bytes_requested(&p_cacheblock_new->diskspace, n_bytes, p_dict->file_handle_dict)))
        {
            deletecache(list_atitem(p_list_block_cache));
            return(err);
        }

        p_cacheblock_new->diskaddress = p_letter->p.disk;
    }
    else
    {
        p_cacheblock_new->diskspace = 0;
        p_cacheblock_new->diskaddress = 0;
    }

    /* save parameters in cacheblock */
    p_cacheblock_new->usecount = 0;
    p_cacheblock_new->lettix = lettix;
    p_cacheblock_new->dict_number = dict_number(p_dict);

    /* move index pointer */
    p_letter->p.cacheno = list_atitem(p_list_block_cache);
    p_letter->letflags |= LET_CACHED;

    return(STATUS_OK);
}

/******************************************************************************
*
* free least used cache block
*
* --out--
* bytes freed
*
******************************************************************************/

_Check_return_
static STATUS
freecache(
    _InVal_     S32 lettix)
{
    LIST_ITEMNO cacheno = 0;
    LIST_ITEMNO minno = -1;
    P_LIST_ITEM it;
    P_CACHEBLOCK p_cacheblock;
    P_DICT p_dict;
    S32 mincount = 0x7FFFFFFF;
    S32 bytes_freed;

    if(NULL == p_list_block_cache)
        return(status_nomem());

    if(NULL != (it = list_initseq(p_list_block_cache, &cacheno)))
    {
        do  {
            P_DICT p_dict_t;
            P_CACHEBLOCK p_cacheblock_t = list_itemcontents(CACHEBLOCK, it);

            /* check if block is locked */
            p_dict_t = array_ptr(&h_dict_table, DICT, p_cacheblock_t->dict_number);
            if(lettix != p_cacheblock_t->lettix
               &&
               !(array_ptr(&p_dict_t->h_index, LETTER, p_cacheblock_t->lettix)->letflags & LET_LOCKED))
            {
                if(p_cacheblock_t->usecount < mincount)
                {
                    mincount = p_cacheblock_t->usecount;
                    minno = cacheno;
                }
            }
        }
        while(NULL != (it = list_nextseq(p_list_block_cache, &cacheno)));
    }

    if(minno < 0)
        return(status_nomem());

    p_cacheblock = cacheblock_goto_item(minno);
    p_dict = array_ptr(&h_dict_table, DICT, p_cacheblock->dict_number);
    bytes_freed = array_ptr(&p_dict->h_index, LETTER, p_cacheblock->lettix)->blklen;

#if TRACE_ALLOWED
    {
    S32 blocks, largest;
    S32 totalmem;

    trace_1(TRACE_MODULE_SPELL, TEXT("spell freecache has freed a block of: ") S32_TFMT TEXT(" bytes"),
            array_ptr(&p_dict->h_index, LETTER, p_cacheblock->lettix)->blklen);
    spell_stats(&blocks, &largest, &totalmem);
    trace_3(TRACE_MODULE_SPELL, TEXT("spell stats: blocks: ") S32_TFMT TEXT(", largest: ") S32_TFMT TEXT(", totalmem: %d"),
            blocks, largest, totalmem);
    } /*block*/
#endif

    trace_1(TRACE_MODULE_SPELL, TEXT("freecache freeing: %d"), minno);

    status_return(killcache(minno));

    return(bytes_freed);
}

/******************************************************************************
*
* check space in dictionary table
*
* --out--
* <0 if no space, otherwise index
* of free entry
*
******************************************************************************/

_Check_return_
static STATUS
get_dict_entry(
    _OutRef_    P_P_DICT p_p_dict)
{
    STATUS status = STATUS_OK;
    P_DICT p_dict = NULL;

    { /* look for a hole */
    const ARRAY_INDEX n_dicts = array_elements(&h_dict_table);
    ARRAY_INDEX i;
    P_DICT p_dict_t = array_range(&h_dict_table, DICT, 0, n_dicts);

    for(i = 0; i < n_dicts; ++i, ++p_dict_t)
    {
        if(0 == p_dict_t->h_index)
        {
            p_dict = p_dict_t;
            break;
        }
    }
    } /*block*/

    if(NULL == p_dict)
    {
        SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(DICT), TRUE /* need this */);
        p_dict = al_array_extend_by(&h_dict_table, DICT, 1, &array_init_block, &status);
    }

    if(NULL != p_dict)
    {
        list_init(&p_dict->dict_end_list);
        status = dict_number(p_dict);
    }

    *p_p_dict = p_dict;

    return(status);
}

/******************************************************************************
*
* initialise variables for match
*
* --out--
* =0 indicates we have no word
* >0 means we have a valid word
*
******************************************************************************/

static S32
initmatch(
    _InoutRef_  P_DICT p_dict,
    P_U8 wordout,
    _InVal_     S32 sizeof_wordout,
    _In_z_      PC_SBSTR wordin,
    _In_opt_z_  PC_U8Z mask)
{
    PC_U8Z ci;
    P_U8 co;

    /* if no wordin, initialise from mask */
    if(!*wordin)
    {
        ci = mask;
        co = wordout;
        do  {
            if(!ci                          ||
               !*ci                         ||
               (*ci == SPELL_WILD_MULTIPLE) ||
               (*ci == SPELL_WILD_SINGLE))
            {
                if(!ci || (ci == mask))
                {
                    *co++ = p_dict->letter_1[0];
                    *co = CH_NULL;

                    return((array_ptr(&p_dict->h_index, LETTER, 0)->letflags & LET_ONE) ? 1 : 0);
                }
                *co = CH_NULL;
                return(1);
            }
            *co++ = *ci;
        }
        while(*ci++);
    }
    else
        xstrkpy(wordout, sizeof_wordout, wordin);

    return(0);
}

/******************************************************************************
*
* remove a block from the cache chain,
* writing out to the dictionary if changed
*
******************************************************************************/

_Check_return_
static STATUS
killcache(
    _In_        LIST_ITEMNO cacheno)
{
    STATUS err, flush_err;
    BOOL write;
    P_CACHEBLOCK p_cacheblock;
    FILE_HANDLE dicthand;
    P_LETTER p_letter;
    P_DICT p_dict;

    err = STATUS_OK;
    p_cacheblock = cacheblock_goto_item(cacheno);
    p_dict = array_ptr(&h_dict_table, DICT, p_cacheblock->dict_number);

    trace_1(TRACE_MODULE_SPELL, TEXT("killcache cacheno: %d"), cacheno);

    /* write out block if altered */
    if((write = (array_ptr(&p_dict->h_index, LETTER, p_cacheblock->lettix)->letflags & LET_WRITE)) != 0)
        status_return(writeblock(p_cacheblock));

    /* clear flags in index */
    p_letter = array_ptr(&p_dict->h_index, LETTER, p_cacheblock->lettix);
    p_letter->letflags &= LET_ONE | LET_TWO;

    /* save disk address in index */
    p_letter->p.disk = p_cacheblock->diskaddress;

    /* write out index entry */
    dicthand = p_dict->file_handle_dict;
    if(write && status_fail(err = writeindex(p_dict, p_cacheblock->lettix)))
    {
        /* make the dictionary useless
         * if not properly updated
        */
        trace_1(TRACE_MODULE_SPELL, TEXT("write of index block: ") S32_TFMT TEXT(" failed"), p_cacheblock->lettix);
        status_assert(file_clearerror(dicthand));
        (void) file_rewind(dicthand);
        (void) file_putc(0, dicthand);
    }

    /* throw away the cache block */
    deletecache(cacheno);

    /* flush buffer */
    trace_0(TRACE_MODULE_SPELL, TEXT("flushing buffer"));
    if(status_fail(flush_err = t5_file_flush(dicthand)) && status_ok(err))
        err = flush_err;

    trace_0(TRACE_MODULE_SPELL, TEXT("buffer flushed"));
    return(err);
}

/******************************************************************************
*
* load the dictionary definition
*
* --out--
* <0 error
*
******************************************************************************/

_Check_return_
static STATUS
load_dict_def(
    _InoutRef_  P_DICT p_dict)
{
    U8 keystr[sizeof32(KEYSTR)];
    S32 keylen, n_bytes;

    /* position to start of dictionary */
    status_return(file_rewind(p_dict->file_handle_dict));

    /* read key string to determine dictionary type */
    n_bytes = strlen32(KEYSTR);
    *keystr = CH_NULL;
    status_return(file_read_bytes_requested(keystr, n_bytes, p_dict->file_handle_dict));

    keylen = strlen32(KEYSTR);
    if(0 != memcmp32(keystr, KEYSTR, strlen32(KEYSTR)))
        return(create_error(SPELL_ERR_BADDICT));

    return(load_dict_def_now(p_dict, p_dict->file_handle_dict, keylen));
}

/******************************************************************************
*
* process a dictionary definition given a handle and key length
*
******************************************************************************/

_Check_return_
static STATUS
load_dict_def_now(
    _InoutRef_  P_DICT p_dict,
    _InoutRef_  FILE_HANDLE def_file,
    _InVal_     S32 keylen)
{
    STATUS status;
    ENDING token[MAX_TOKEN - MIN_TOKEN];
    U8Z buffer[255 + 1];
    S32 i, end;
    U32 filepos_lo, filepos_hi;
    PC_U8 in;
    P_U8 out;
    P_ENDING p_ending;

    /* position dictionary */
    status_return(dict_seek_set(p_dict, keylen));

    /* read dictionary name */
    status_return(read_def_line_ensure(def_file, buffer));

    status_return(al_tstr_set(&p_dict->h_dict_name, _tstr_from_sbstr(buffer)));

    /* read character offset */
    status_return(read_def_line_ensure(def_file, buffer));

    p_dict->char_offset = (U8) atoi(buffer);

    if((p_dict->char_offset < MIN_CHAR) || (p_dict->char_offset >= MAX_CHAR))
        return(create_error(SPELL_ERR_BADDEFFILE));

    /* read token offset */
    status_return(read_def_line_ensure(def_file, buffer));

    p_dict->man_token_start = (U8) atoi(buffer);

    if(p_dict->man_token_start                     &&
       ((S32) p_dict->man_token_start <  MIN_TOKEN ||
        (S32) p_dict->man_token_start >= MAX_TOKEN) )
        return(create_error(SPELL_ERR_BADDEFFILE));

    /* read first letter list */
    status_return(status = read_def_line_ensure(def_file, buffer));
    if(status > MAX_INDEX)
        return(create_error(SPELL_ERR_BADDEFFILE));

    p_dict->n_index_1 = (U8) (status - 1);
    in = buffer;
    out = p_dict->letter_1;
    while(*in)
    {
        if(p_dict->man_token_start &&
           (*in >= p_dict->man_token_start ||
            *in <  p_dict->char_offset))
            return(create_error(SPELL_ERR_DEFCHARERR));

        *out++ = *in++;
    }

    /* read second letter list */
    status_return(status = read_def_line_ensure(def_file, buffer));
    if(status > MAX_INDEX)
        return(create_error(SPELL_ERR_DEFCHARERR));

    p_dict->n_index_2 = (U8) (status - 1);
    in = buffer;
    out = p_dict->letter_2;
    while(*in)
    {
        if(p_dict->man_token_start &&
           (*in >= p_dict->man_token_start ||
            *in <  p_dict->char_offset) )
            return(create_error(SPELL_ERR_DEFCHARERR));

        *out++ = *in++;
    }

    /* initialise case map */
    for(i = 0; i < 256; i++)
        p_dict->case_map[i] = 0;

    /* flag OK characters in case map */
    for(i = 0, in = buffer; *in; ++in)
    {
        const U8 case_map_idx = *in; 
        p_dict->case_map[case_map_idx] = case_map_idx;
    }

    /* read case equivalence list */
    status_return(status = read_def_line_ensure(def_file, buffer));
    if((U8) (status - 1) > p_dict->n_index_2)
        return(create_error(SPELL_ERR_DEFCHARERR));

    /* insert case equivalences */
    for(i = 0, in = buffer; *in; ++in, ++i)
    {
        const U8 case_map_idx = *in; 
        assert(case_map_idx <= 255);
        p_dict->case_map[case_map_idx] = p_dict->letter_2[i];
    }

    /* set number of index elements */
    p_dict->n_index = (S32) p_dict->n_index_1 * (S32) p_dict->n_index_2;

    if(0 == (p_dict->token_start = p_dict->man_token_start))
        /* set auto token_start */
        p_dict->token_start = p_dict->char_offset + p_dict->n_index_2;
    else
        /* check manual token start is high enough */
        if(p_dict->char_offset + p_dict->n_index_2 > p_dict->man_token_start)
            return(create_error(SPELL_ERR_DEFCHARERR));

    /* insert ending zero - null token */
    token[0].len       = 0;
    token[0].alpha     = 0;
    token[0].pos       = 255;
    token[0].ending[0] = CH_NULL;

    /* read endings list */
    for(end = 1;
        (status = read_def_line(def_file, buffer)) > 0 &&
        end < MAX_TOKEN - MIN_TOKEN;
        ++end)
    {
        P_U8 out;
        S32 i;

        /* read an ending and convert to internal form */
        for(i = 0, in = buffer, p_ending = &token[end], out = p_ending->ending;
            i < MAX_ENDLEN;
            ++i)
        {
            U8 u8 = *in++;
            STATUS ch;
            if(CH_NULL == u8)
                break;
            u8 = (U8) toupper_us(p_dict, u8);
            if((ch = char_ordinal_3(p_dict, u8)) <= 0)
                break;
            *out++ = (U8) ch;
        }

        if((0 == i) || (i >= MAX_ENDLEN))
            return(create_error(SPELL_ERR_BADDEFFILE));

        *out++ = CH_NULL;
        p_ending->len = (U8) strlen(p_ending->ending);
        p_ending->pos = (U8) end;
    }

    /* check we read some endings */
    if((status >= 0) && (end == 1))
        status = create_error(SPELL_ERR_BADDEFFILE);
    status_return(status);

    /* check that tokens and all fit into the space */
    if(p_dict->token_start + end >= MAX_TOKEN)
        return(create_error(SPELL_ERR_BADDEFFILE));

    /* sort into alphabetical order */
    qsort(token, (U32) end, sizeof(token[0]), compar_ending_alpha);

    /* add alphabetical numbers */
    for(i = 0, p_ending = token; i < end; ++i, ++p_ending)
        p_ending->alpha = (U8) i;

    /* sort into length/original position order */
    qsort(token, (U32) end, sizeof(token[0]), compar_ending_pos);

    /* insert into dictionary ending list */
    for(i = 0, p_ending = token; i < end; ++i, ++p_ending)
    {
        P_LIST_ITEM it;
        P_ENDING_I p_ending_i;
        U32 ending_bytes = strlen32p1(p_ending->ending) /*CH_NULL*/;
        U32 n_bytes;

        /* the ending array already has 1 byte reserved */
        n_bytes = (sizeof32(ENDING_I) - 1) + ending_bytes;

        if(NULL == (it =
            list_createitem(&p_dict->dict_end_list,
                            (LIST_ITEMNO) i,
                            n_bytes,
                            FALSE)))
            return(status_nomem());

        p_ending_i = list_itemcontents(ENDING_I, it);

        p_ending_i->len   = p_ending->len;
        p_ending_i->alpha = p_ending->alpha;
        memcpy32(p_ending_i->ending, p_ending->ending, ending_bytes);
    }

    /* read dictionary flags */
    status_return(status = file_getc(p_dict->file_handle_dict));

    p_dict->dictflags = (U8) status;

    /* read position of start of index */
    status_return(status = file_tell(p_dict->file_handle_dict, &filepos_lo, &filepos_hi));

    assert(0 == filepos_hi); /* dictionaries are small */

    /* set offset parameters */
    p_dict->index_offset = (S32) filepos_lo;
    p_dict->data_offset  = (S32) filepos_lo + p_dict->n_index * sizeof32(LETTER);

    return(STATUS_OK);
}

/******************************************************************************
*
* look up a word in the dictionary
*
* --out--
* <0 error
*  0 not found
* >0 found
*
******************************************************************************/

_Check_return_
static STATUS
lookupword(
    _InoutRef_  P_DICT p_dict,
    P_TOKWORD p_tokword,
    _InVal_     S32 needpos)
{
    P_U8 startlim, endlim, p_data, co, ci;
    S32 i, found, bodylen, updown;
    U8 token_start, char_offset;
    P_CACHEBLOCK p_cacheblock;
    P_LETTER p_letter;
    P_LIST_BLOCK p_ending_list;

    if(needpos)
    {
        p_tokword->fail = INS_WORD;
        p_tokword->findpos = p_tokword->matchc = p_tokword->match = p_tokword->matchcp = p_tokword->matchp = 0;
    }

    p_letter = array_ptr(&p_dict->h_index, LETTER, p_tokword->lettix);

    /* check one/two letter words */
    switch(p_tokword->len)
    {
    case 1:
        return((S32) p_letter->letflags & LET_ONE);

    case 2:
        return((S32) p_letter->letflags & LET_TWO);

    default:
        break;
    }

    /* is there a block defined ? */
    if(!p_letter->blklen)
        return(0);

    /* fetch the block if not in memory */
    status_return(fetchblock(p_dict, p_tokword->lettix));

    /* load some variables */
    token_start = p_dict->token_start;
    char_offset = p_dict->char_offset;
    p_ending_list = &p_dict->dict_end_list;

    /* search the block for the word */
    p_letter = array_ptr(&p_dict->h_index, LETTER, p_tokword->lettix);
    p_cacheblock = cacheblock_goto_item(p_letter->p.cacheno);
    ++p_cacheblock->usecount;

    /*
    do a binary search on the third letter
    */

    startlim = cacheblock_ptr(p_cacheblock, 0);
    endlim = cacheblock_ptr(p_cacheblock, p_letter->blklen);
    found = updown = 0;

    p_data = NULL;
    while((endlim - startlim) > 1)
    {
        P_U8 midpoint = p_data = startlim + (endlim - startlim) / 2;
        U8 ch;

        /* step back to first in block */
        while(*p_data)
            --p_data;

        ch = *(p_data + 1);

        if(ch == *p_tokword->body)
        {
            found = 1;
            break;
        }

        if(ch < *p_tokword->body)
        {
            updown = 1;
            startlim = midpoint;
        }
        else
        {
            updown = 0;
            endlim = midpoint;
        }
    }

    if(!found)
    {
        /* set insert position after this letter
        if we are inserting a higher letter */
        if(needpos)
        {
            if(updown)
            {
                endlim = cacheblock_ptr(p_cacheblock, p_letter->blklen);
                while(p_data < endlim)
                {
                    ++p_data;
                    if(!*p_data)
                        break;
                }
            }

            p_tokword->fail = INS_WORD;
            p_tokword->findpos = PtrDiffBytesS32(p_data, cacheblock_ptr(p_cacheblock, 0));
        }
        return(0);
    }

    /* search forward for word */
    endlim = cacheblock_ptr(p_cacheblock, p_letter->blklen);

    *p_tokword->bodyd = CH_NULL;
    while((p_data + 1) < endlim)
    {
        /* save previous body */
        if(needpos)
            xstrkpy(p_tokword->bodydp, sizeof32(p_tokword->bodydp), p_tokword->bodyd);

        /* take prefix count */
        co = p_tokword->bodyd + *p_data++;

        /* build body */
        while(*p_data < token_start)
            *co++ = *p_data++;

        /* mark end of body */
        bodylen = PtrDiffBytesS32(co, p_tokword->bodyd);
        *co = CH_NULL;

        /* compare bodies and stop search
         * if we are past the place
        */
        for(i = found = 0, ci = p_tokword->bodyd, co = p_tokword->body;
            i < bodylen;
            ++i, ++ci, ++co)
        {
            if(*ci != *co)
            {
                if(*ci > *co)
                    found = 1;
                else
                    found = -1;

                break;
            }
        }

        if(needpos)
        {
            p_tokword->matchcp = p_tokword->matchc;
            p_tokword->matchp = p_tokword->match;
            p_tokword->matchc = i;
            p_tokword->match = (i == bodylen && p_tokword->matchc >= p_tokword->matchcp) ? 1 : 0;
        }

        if(!found)
        {
            /* compare tokens */
            U8 ch;
            while(((ch = *p_data) >= token_start) && (p_data < endlim))
            {
                ++p_data;
                if(0 == /*"C"*/strcmp(ending_list_goto_item(p_ending_list, (LIST_ITEMNO) ch - (LIST_ITEMNO) token_start)->ending, p_tokword->body + bodylen))
                {
                    if(needpos)
                    {
                        p_tokword->fail = 0;
                        p_tokword->findpos = PtrDiffBytesS32(p_data, cacheblock_ptr(p_cacheblock, 0));
                        p_tokword->findpos -= 1;
                    }
                    return(1);
                }
            }
        }

        /* if bodies didn't compare */
        if(found
           ||
           (needpos && (p_tokword->matchc < p_tokword->matchcp)))
        {
            if(found >= 0)
            {
                /* step back to start of word */
                if(needpos)
                {
                    while(*--p_data >= char_offset)
                    { /*EMPTY*/ }

                    if(*p_data)
                        p_tokword->fail = INS_WORD;
                    else
                        p_tokword->fail = (p_tokword->matchc == 0)
                            ? INS_WORD
                            : INS_STARTLET;

                    p_tokword->findpos = PtrDiffBytesS32(p_data, cacheblock_ptr(p_cacheblock, 0));
                }

                return(0);
            }
            else
            {
                /* skip tokens, then move to next body */
                while((*p_data >= token_start) && (p_data < endlim))
                    ++p_data;
            }
        }

        /* hit the end of the letter ? */
        if(!*p_data)
            break;
    }

    /* at end of block */
    if(needpos)
    {
        p_tokword->matchcp = p_tokword->matchc;
        p_tokword->matchp = p_tokword->match;
        p_tokword->matchc = p_tokword->match = 0;
        xstrkpy(p_tokword->bodydp, sizeof32(p_tokword->bodydp), p_tokword->bodyd);

        p_tokword->fail = INS_WORD;
        p_tokword->findpos = PtrDiffBytesS32(p_data, cacheblock_ptr(p_cacheblock, 0));
    }

    return(0);
}

/******************************************************************************
*
* set up index for a word
* this routine tries to be fast
*
* --in--
* pointer to a word block
*
* --out--
* index set in block,
* pointer to block returned
*
******************************************************************************/

_Check_return_
_Ret_maybenull_
static P_TOKWORD
makeindex(
    _InoutRef_  P_DICT p_dict,
    P_TOKWORD p_tokword,
    _In_z_      PC_SBSTR word)
{
    S32 len = strlen32(word);
    P_U8 co;

    /* we can use more than char_offset since two letters
    at the start of every word are stripped by the index */
    if(!len || (len >= (S32) p_dict->char_offset + 2))
        return(NULL);
    p_tokword->len = len;

    { /* process first letter */
    U8 u8 = *word++;
    STATUS ch;
    u8 = (U8) toupper_us(p_dict, u8);
    if((ch = char_ordinal_1(p_dict, u8)) < 0)
        return(NULL);
    p_tokword->lettix = ((S32) ch - (S32) p_dict->char_offset) * (S32) p_dict->n_index_2;
    } /*block*/

    /* process second letter */
    if(len > 1)
    {
        U8 u8 = *word++;
        STATUS ch;
        u8 = (U8) toupper_us(p_dict, u8);
        if((ch = char_ordinal_2(p_dict, u8)) < 0)
            return(NULL);
        p_tokword->lettix += ((S32) ch - (S32) p_dict->char_offset);
    }

    /* copy across body */
    co = p_tokword->body;
    while(*word)
    {
        U8 u8 = *word++;
        STATUS ch;
        u8 = (U8) toupper_us(p_dict, u8);
        if((ch = char_ordinal_3(p_dict, u8)) < 0)
            return(NULL);
        *co++ = (U8) ch;
    }
    *co++ = CH_NULL;

    /* clear tail index */
    p_tokword->tail = 0;

    return(p_tokword);
}

/******************************************************************************
*
* match two words with possible wildcards
* word1 must contain any wildcards
*
* --out--
* -1 word1 < word2
*  0 word1 = word2
* +1 word1 > word2
*
******************************************************************************/

static S32
matchword(
    _InoutRef_  P_DICT p_dict,
    _In_opt_z_  PC_U8Z mask,
    _In_z_      PC_SBSTR word)
{
    PC_U8Z maskp;
    PC_U8Z star;
    PC_SBSTR wordp;
    PC_SBSTR nextpos;

    if((NULL == mask) || (CH_NULL == PtrGetByte(mask)))
        return(0);

    maskp = star = mask;
    wordp = nextpos = word;

    for(;;)
    {
        /* loop1 */
        ++nextpos;

        /* loop3 */
        for(;;)
        {
            if(*maskp == SPELL_WILD_MULTIPLE)
            {
                nextpos = wordp;
                star = ++maskp;
                break;
            }

            if(toupper_us(p_dict, *maskp) != toupper_us(p_dict, *wordp))
            {
                if(CH_NULL == *wordp)
                    return(1);

                if(*maskp != SPELL_WILD_SINGLE)
                {
                    ++maskp;
                    wordp = nextpos;
                    if(star != mask)
                    {
                        maskp = star;
                        break;
                    }
                    else
                    {
                        U8 u8_mask = *maskp;
                        U8 u8_word = *wordp;
                        u8_mask = (U8) toupper_us(p_dict, u8_mask);
                        u8_word = (U8) toupper_us(p_dict, u8_word);
                        return((char_ordinal_2(p_dict, u8_mask) < char_ordinal_2(p_dict, u8_word))
                                 ? -1
                                 :  1);
                    }
                }
            }

            if(CH_NULL == *maskp++)
                return(0);
            ++wordp;
        }
    }
}

/******************************************************************************
*
* return the next word in a dictionary
*
* --out--
* <0 error
* =0 no more words
* >0 length of returned word
*
******************************************************************************/

_Check_return_
static STATUS
nextword(
    _InoutRef_  P_DICT p_dict,
    P_U8Z word)
{
    TOKWORD curword;
    S32 tokabval, nexthigher, curabval, tail, n_index;
    P_U8 p_data, p_data_end, co;
    U8 token_start;
    P_CACHEBLOCK p_cacheblock;
    P_LETTER p_letter;
    P_LIST_BLOCK p_ending_list;

    if(!makeindex(p_dict, &curword, word))
        return(create_error(SPELL_ERR_BADWORD));

    /* check if word exists and get position */
    status_return(lookupword(p_dict, &curword, TRUE));

    if((curword.len == 1) && (array_ptr(&p_dict->h_index, LETTER, curword.lettix)->letflags & LET_TWO))
        return(decodeword(p_dict, word, &curword, 2));

    n_index = p_dict->n_index;
    token_start = p_dict->token_start;
    p_ending_list = &p_dict->dict_end_list;

    do  {
        if(array_ptr(&p_dict->h_index, LETTER, curword.lettix)->blklen)
        {
            /* check we have a cache block */
            status_return(fetchblock(p_dict, curword.lettix));

            p_letter = array_ptr(&p_dict->h_index, LETTER, curword.lettix);
            p_cacheblock = cacheblock_goto_item(p_letter->p.cacheno);
            p_data = cacheblock_ptr(p_cacheblock, curword.findpos);
            p_data_end = cacheblock_ptr(p_cacheblock, p_letter->blklen);

            /* if we matched previous root */
            if(!curword.match && curword.matchp)
            {
                /* step back to previous unit */
                --p_data;

                xstrkpy(curword.bodyd, sizeof32(curword.bodyd), curword.bodydp);

                curword.match  = curword.matchp;
                curword.matchc = curword.matchcp;
                curword.matchp = curword.matchcp = 0;
            }
            /* build a body at the start of a block */
            else if(p_data < p_data_end && !*p_data)
            {
                /* skip null starter */
                ++p_data;
                co = curword.bodyd;
                while(*p_data < token_start)
                    *co++ = *p_data++;
                *co++ = CH_NULL;
            }

            curabval = -1;
            if(p_data < p_data_end)
            {
                tokenise(p_dict, &curword, curword.matchc);
                curabval = setabval(p_dict, &curword, 1);

                /* ensure we are on the tokens */
                while(*p_data >= token_start)
                    --p_data;

                while((*p_data < token_start) && (p_data < p_data_end))
                    ++p_data;
            }

            while(p_data < p_data_end)
            {
                /* find the next higher token */
                U8 token;
                tail = -1;
                nexthigher = MAX_TOKEN;
                while(((token = *p_data) >= token_start) && (p_data < p_data_end))
                {
                    ++p_data;
                    tokabval = ending_list_goto_item(p_ending_list, (LIST_ITEMNO) token - (LIST_ITEMNO) token_start)->alpha;
                    if(tokabval > curabval)
                    {
                        if(tokabval < nexthigher)
                        {
                            tail = (S32) token - (S32) token_start;
                            nexthigher = tokabval;
                        }
                    }
                }

                if((tail >= 0) && (nexthigher > curabval))
                {
                    /* work out the real word from the decoded stuff */
                    curword.tail = tail;
                    return(decodeword(p_dict, word, &curword, 0));
                }

                /* if there were no tokens higher, go onto next root */
                if(++p_data >= p_data_end)
                    break;

                co = curword.bodyd + token;

                while((*p_data < token_start) && (p_data < p_data_end))
                    *co++ = *p_data++;

                *co = CH_NULL;
                curabval = -1;
            }
        }

        /* move onto the next letter */
        *curword.bodyd = *curword.bodydp = CH_NULL;
#if 0
        curword.tail   = curword.matchc = curword.matchcp =
        curword.match  = curword.matchp = curword.findpos = 0;
#else
        curword.matchc = 0;
        curword.findpos = 0;
#endif

        /* skip down the index till we find
        an entry with some words in it */
        if(++curword.lettix < n_index)
        {
            p_letter = array_ptr(&p_dict->h_index, LETTER, curword.lettix);
            if(p_letter->letflags & LET_ONE)
                return(decodeword(p_dict, word, &curword, 1));

            if(p_letter->letflags & LET_TWO)
                return(decodeword(p_dict, word, &curword, 2));
        }
    }
    while(curword.lettix < n_index);

    *word = CH_NULL;
    return(0);
}

/******************************************************************************
*
* given ordinal number for the first character
* position (including char_offset)
* return the upper case character for this ordinal
*
******************************************************************************/

static S32
ordinal_char_1(
    _InoutRef_  P_DICT p_dict,
    _InVal_     S32 ord)
{
    return(p_dict->letter_1[ord - p_dict->char_offset]);
}

/******************************************************************************
*
* given ordinal number for the second character
* position (including char_offset)
* return the upper case character for this ordinal
*
******************************************************************************/

static S32
ordinal_char_2(
    _InoutRef_  P_DICT p_dict,
    _InVal_     S32 ord)
{
    return(p_dict->letter_2[ord - p_dict->char_offset]);
}

/******************************************************************************
*
* given ordinal number for the third character
* position (including char_offset)
* return the upper case character for this ordinal
*
******************************************************************************/

static S32
ordinal_char_3(
    _InoutRef_  P_DICT p_dict,
    _InVal_     S32 ord)
{
    return(p_dict->man_token_start ? ord : p_dict->letter_2[ord - p_dict->char_offset]);
}

/******************************************************************************
*
* return the previous word
*
* --out--
* <0 error
* =0 no more words
* >0 length of word
*
******************************************************************************/

_Check_return_
static STATUS
prevword(
    _InoutRef_  P_DICT p_dict,
    P_U8Z word)
{
    TOKWORD curword;
    S32 tokabval, nextlower, token, curabval, tail, onroot;
    P_U8 p_data_start, p_data_end, p_data, co;
    U8 token_start, char_offset;
    P_CACHEBLOCK p_cacheblock;
    P_LETTER p_letter;
    P_LIST_BLOCK p_ending_list;

    if(!makeindex(p_dict, &curword, word))
        return(create_error(SPELL_ERR_BADWORD));

    /* check if word exists and get position */
    status_return(lookupword(p_dict, &curword, TRUE));

    if((curword.len == 2) && (array_ptr(&p_dict->h_index, LETTER, curword.lettix)->letflags & LET_ONE))
        return(decodeword(p_dict, word, &curword, 1));

    token_start = p_dict->token_start;
    char_offset = p_dict->char_offset;
    p_ending_list = &p_dict->dict_end_list;
    p_data = p_data_start = NULL;

    do  {
        if(array_ptr(&p_dict->h_index, LETTER, curword.lettix)->blklen)
        {
            /* check we have a cache block */
            status_return(fetchblock(p_dict, curword.lettix));

            p_letter = array_ptr(&p_dict->h_index, LETTER, curword.lettix);
            p_cacheblock = cacheblock_goto_item(p_letter->p.cacheno);
            p_data_start = cacheblock_ptr(p_cacheblock, 0);
            p_data = p_data_start + curword.findpos;
            p_data_end = p_data_start + p_letter->blklen;

            onroot = 0;
            curabval = MAX_TOKEN;
            if((p_data > p_data_start) && (curword.matchc || curword.matchcp))
            {
                /* if we didn't match this root,
                start from the one before */
                if(!curword.match && curword.matchcp)
                {
                    if(p_data != p_data_end)
                        while(*p_data >= char_offset)
                            --p_data;
                    if(p_data > p_data_start)
                    {
                        --p_data;
                        curword.matchc = curword.matchcp;
                        curword.match = curword.matchp;
                        curword.matchcp = curword.matchp = 0;
                        xstrkpy(curword.bodyd, sizeof32(curword.bodyd), curword.bodydp);
                    }
                }

                tokenise(p_dict, &curword, curword.matchc);
                curabval = setabval(p_dict, &curword, 0);

                /* move to the start of the root */
                while(*p_data >= char_offset)
                    --p_data;

                /* build a body at the start of a block */
                if(CH_NULL == *p_data)
                {
                    /* skip null starter */
                    ++p_data;
                    co = curword.bodyd;
                    while(*p_data < token_start)
                        *co++ = *p_data++;
                    *co++ = CH_NULL;
                    curword.matchcp = 0;
                }

                /* move on to the tokens */
                while((*p_data < token_start) && (p_data < p_data_end))
                    ++p_data;

                onroot = 1;
            }

            /* move back down the block */
            while((p_data > p_data_start) && onroot)
            {
                /* find the next lower token */
                tail = nextlower = -1;
                while(((token = *p_data) >= (S32) token_start) && (p_data < p_data_end))
                {
                    ++p_data;
                    tokabval = ending_list_goto_item(p_ending_list, (LIST_ITEMNO) token - token_start)->alpha;
                    if(tokabval < curabval)
                    {
                        if(tokabval > nextlower)
                        {
                            tail = token - token_start;
                            nextlower = tokabval;
                        }
                    }
                }

                /* did we find a suitable token ? */
                if((tail >= 0) && (nextlower < curabval))
                {
                    /* work out the real word from the decoded stuff */
                    curword.tail = tail;
                    return(decodeword(p_dict, word, &curword, 0));
                }

                /* if there were no tokens lower,
                go onto previous root */
                --p_data;
                while(*p_data >= char_offset)
                    --p_data;

                /* check for beginning of block,
                or no previous root */
                if((p_data == p_data_start) || (!curword.matchcp))
                    break;
                --p_data;
                while(*p_data >= token_start)
                    --p_data;
                ++p_data;

                curword.match = 0;
                xstrkpy(curword.bodyd, sizeof32(curword.bodyd), curword.bodydp);
                curabval = MAX_TOKEN;
            }
        }

        p_letter = array_ptr(&p_dict->h_index, LETTER, curword.lettix);
        if(p_data == p_data_start || !p_letter->blklen)
        {
            U8 last_ch;
            U8 max_word, i;

            /* if we are at the start of the block */
            do  {
                /* return the small words if there are some */
                if((curword.len > 2) && (p_letter->letflags & LET_TWO))
                    return(decodeword(p_dict, word, &curword, 2));
                if((curword.len > 1) && (p_letter->letflags & LET_ONE))
                    return(decodeword(p_dict, word, &curword, 1));

                if(--curword.lettix < 0)
                    break;

                p_letter = array_ptr(&p_dict->h_index, LETTER, curword.lettix);
            }
            while(!p_letter->blklen);

            /* quit if at beginning */
            if(curword.lettix < 0)
                break;

            /* build previous letter ready for search */
            decodeword(p_dict, word, &curword, 2);
            last_ch = p_dict->letter_2[p_dict->n_index_2 - 1];
            max_word = p_dict->char_offset + 1;
            for(i = 2, co = word + 2; i < max_word; ++i)
                *co++ = last_ch;
            *co = CH_NULL;

            /* set position to end of block */
            (void) makeindex(p_dict, &curword, word);
        }
        else
        {
            U8 last_ch;
            U8 max_word, i;

            /* if we are at the start of a letter in a block */
            co = word + 2;

            { /* set word to 'last of previous letter' */
            U8 u8 = *co;
            u8 = (U8) toupper_us(p_dict, u8);
            u8 = p_dict->letter_2[char_ordinal_2(p_dict, u8) - p_dict->char_offset - 1];
            *co++ = u8;
            } /*block*/

            last_ch  = p_dict->letter_2[p_dict->n_index_2 - 1];
            max_word = p_dict->char_offset + 1;

            for(i = 3; i < max_word; ++i)
                *co++ = last_ch;
            *co = CH_NULL;

            (void) makeindex(p_dict, &curword, word);
        }

        status_return(lookupword(p_dict, &curword, TRUE));
    }
    while(curword.lettix >= 0);

    *word = CH_NULL;
    return(0);
}

/******************************************************************************
*
* read a line from the definition file
*
* --out--
* <0 error
* =0 EOF
* >0 line length
*
******************************************************************************/

_Check_return_
static STATUS
read_def_line(
    _InoutRef_  FILE_HANDLE def_file,
    P_U8 buffer)
{
    static S32 last_ch = 0;
    S32 ch, comment, hadcr, hadbar, leadspc;
    P_U8 out, trail;

    comment = hadcr = hadbar = leadspc = 0;
    out = trail = buffer;

    for(;;)
    {
        if(last_ch)
        {
            ch = last_ch;
            last_ch = 0;
        }
        else
        {
            STATUS status;
            if((status = file_getc(def_file)) == EOF_READ)
                break;
            status_return(status);
            ch = (S32) status;
        }

        /* nulls delimit the definition file
        when stored in the dictionary */
        if(!ch)
            break;

        if(ch == CR || ch == LF)
        {
            hadcr = 1;
            hadbar = comment = 0;
            continue;
        }

        /* stop on non-blank line and CR or LF */
        if(hadcr && trail != buffer)
        {
            /* save last character */
            last_ch = ch;
            break;
        }
        else
            hadcr = 0;

        /* check for comments */
        if(ch == ESC_CHAR)
        {
            if(!hadbar)
            {
                hadbar = 1;
                continue;
            }

            /* bona fide | character */
            hadbar = 0;
        }

        if(hadbar)
            comment = 1;

        if(comment)
            continue;

        /* strip out control characters */
        if(ch < CH_SPACE)
            continue;

        /* strip leading spaces */
        if(ch == CH_SPACE && !leadspc)
            continue;
        else
            leadspc = 1;

        /* finally, save wanted character */
        *out++ = (U8) ch;

        if(ch != CH_SPACE)
            trail = out;
    }

    if(trail != buffer)
        *trail++ = CH_NULL;

    return(PtrDiffBytesS32(trail, buffer));
}

/******************************************************************************
*
* read a def file line and ensure we got something
*
******************************************************************************/

_Check_return_
static STATUS
read_def_line_ensure(
    _InoutRef_  FILE_HANDLE def_file,
    P_U8 buffer)
{
    STATUS status;

    status = read_def_line(def_file, buffer);
    if(status == 0 || status_fail(status))
        return(create_error(SPELL_ERR_BADDEFFILE));

    return(status);
}

/******************************************************************************
*
* give back entry in dictionary table
*
******************************************************************************/

static void
release_dict_entry(
    _InoutRef_  P_DICT p_dict)
{
    list_free(&p_dict->dict_end_list);

    /* free memory used by index */
    al_array_dispose(&p_dict->h_index);

    /* free buffer if there is one */
    al_ptr_dispose(P_P_ANY_PEDANTIC(&p_dict->dictbuf));

    /* throw away file handle */
    p_dict->file_handle_dict = NULL;

    /* free names */
    al_array_dispose(&p_dict->h_dict_name);
    al_array_dispose(&p_dict->h_dict_filename);

    {
    ARRAY_INDEX i;

    for(i = 0; i < array_elements(&h_dict_table); ++i)
    {
        P_DICT p_dict = array_ptr(&h_dict_table, DICT, i);

        if(p_dict->file_handle_dict || p_dict->h_index)
            break;
    }

    if(i == array_elements(&h_dict_table))
        al_array_dispose(&h_dict_table);
    } /*block*/
}

/******************************************************************************
*
* compare two strings using the case and order
* information for a dictionary
*
******************************************************************************/

static S32
setabval_ncmp(
    P_U8 word1,
    P_U8 word2)
{
    S32 ch1, ch2;

    while((ch1 = *word1) == (ch2 = *word2))
    {
        word1 += 1;
        word2 += 1;

        if(!ch1)
            return(0);
    }

    return(ch1 > ch2 ? 1 : -1);
}

/******************************************************************************
*
* set alphabetic value of ending of tokenised word
*
******************************************************************************/

static S32
setabval(
    _InoutRef_  P_DICT p_dict,
    P_TOKWORD p_tokword,
    _InVal_     S32 lo_hi)
{
    U8 ending[BUF_MAX_WORD];
    P_ENDING_I p_ending_i;
    S32 abval = lo_hi ? -1 : S32_MAX;
    LIST_ITEMNO i;
    P_LIST_BLOCK p_ending_list;

    if(!p_tokword->matchc)
        return(abval);

    /* find entry for ending */
    p_ending_list = &p_dict->dict_end_list;
    p_ending_i = ending_list_goto_item(p_ending_list, (LIST_ITEMNO) p_tokword->tail - p_dict->token_start);

    /* no ending .. */
    if(!p_ending_i->len)
        return(p_tokword->match ? 0 : abval);

    /* if token is whole ending ... */
    if(p_tokword->matchc == (S32) strlen(p_tokword->body))
        return(p_ending_i->alpha);

    /* concoct whole ending */
    xstrkpy(ending, sizeof32(ending), p_tokword->body + p_tokword->matchc);
    xstrkat(ending, sizeof32(ending), p_ending_i->ending);

    /* work out the alphabetic value of the non-token ending
    that we have - either the next lower, or next higher value */
    for(i = 0; i < list_numitem(p_ending_list); ++i)
    {
        S32 abval_i;

        p_ending_i = ending_list_goto_item(p_ending_list, i);

        abval_i = (S32) p_ending_i->alpha;

        if(lo_hi)
        {
            if((setabval_ncmp(p_ending_i->ending, ending) <= 0) && (abval_i > abval))
                abval = abval_i;
        }
        else
        {
            if((setabval_ncmp(p_ending_i->ending, ending) >= 0) && (abval_i < abval))
                abval = abval_i;
        }
    }

    return(abval);
}

/******************************************************************************
*
* compare two strings using the case and order information for a dictionary
*
******************************************************************************/

_Check_return_
static int
strnicmp_us(
    _InoutRef_  P_DICT p_dict,
    _In_        PC_SBSTR word1,
    _In_        PC_SBSTR word2,
    _In_        S32 len)
{
    STATUS ch1, ch2;

    for(;;)
    {
        U8 u8_1 = *word1++;
        U8 u8_2 = *word2++;

        u8_1 = (U8) toupper_us(p_dict, u8_1);
        u8_2 = (U8) toupper_us(p_dict, u8_2);

        ch1 = char_ordinal_2(p_dict, u8_1);
        ch2 = char_ordinal_2(p_dict, u8_2);

        if(ch1 != ch2)
            break;

        /* detect when at end of string (NULL or len) */
        if(ch1 < 0 || !(--len))
            return(0);
    }

    return((ch1 > ch2) ? 1 : -1);
}

/******************************************************************************
*
* make sure that there are no cache
* blocks left for a given dictionary
*
******************************************************************************/

static void
stuffcache(
    _InoutRef_  P_DICT p_dict)
{
    LIST_ITEMNO i;
    P_CACHEBLOCK p_cacheblock;

    trace_0(TRACE_MODULE_SPELL, TEXT("stuffcache"));

    /* write out/remove any blocks from this dictionary */

    /* SKS after 1.04 26sep93 */
    i = list_numitem(p_list_block_cache);

    while(--i >= 0)
    {
        p_cacheblock = cacheblock_goto_item(i);

        if(p_cacheblock->dict_number == dict_number(p_dict))
            deletecache(i);
    }
}

/******************************************************************************
*
* tokenise a word
* this routine tries to be fast
*
* --in--
* pointer to a word block
*
* --out--
* tokenised word in block,
* pointer to block returned
*
******************************************************************************/

static void
tokenise(
    _InoutRef_  P_DICT p_dict,
    P_TOKWORD p_tokword,
    _InVal_     S32 rootlen)
{
    P_U8 endbody;
    U8 token_start;
    S32 maxtail;
    LIST_ITEMNO i;
    P_LIST_BLOCK p_ending_list;

    /* calculate maximum ending length */
    if(p_tokword->len > MAX(1, rootlen) + 2)
        maxtail = p_tokword->len - rootlen - 2;
    else
        maxtail = 0;

    /* find a suitable ending */
    endbody = p_tokword->body + MAX(0, p_tokword->len - 2);

    token_start = p_dict->token_start;
    p_ending_list = &p_dict->dict_end_list;

    for(i = 0; i < list_numitem(p_ending_list); ++i)
    {
        P_ENDING_I p_ending_i_cur = ending_list_goto_item(p_ending_list, i);

        if(maxtail < (S32) p_ending_i_cur->len)
            continue;

        if(0 == /*"C"*/strcmp(p_ending_i_cur->ending, endbody - p_ending_i_cur->len))
        {
            p_tokword->tail = i + token_start;
            *(endbody - p_ending_i_cur->len) = CH_NULL;
            break;
        }
    }
}

/******************************************************************************
*
* convert a character to lower case
* using the dictionary's mapping
*
******************************************************************************/

static S32
tolower_us(
    _InoutRef_  P_DICT p_dict,
    _InVal_     S32 ch)
{
    S32 i;

    if(ch >= CH_SPACE)
        for(i = CH_SPACE; i < 256; i++)
            if(i != ch && p_dict->case_map[i] == (U8) ch)
                return(i);

    return(ch);
}

/******************************************************************************
*
* write out an altered block to a dictionary
*
******************************************************************************/

_Check_return_
static STATUS
writeblock(
    P_CACHEBLOCK p_cacheblock)
{
    P_DICT p_dict;
    P_LETTER p_letter;

    trace_2(TRACE_MODULE_SPELL, TEXT("writeblock dict: ") S32_TFMT TEXT(", letter: ") S32_TFMT, p_cacheblock->dict_number, p_cacheblock->lettix);

    p_dict = array_ptr(&h_dict_table, DICT, p_cacheblock->dict_number);
    p_letter = array_ptr(&p_dict->h_index, LETTER, p_cacheblock->lettix);

    if(p_letter->blklen)
    {
        /* do we need to extend file ? */
        if((S32) p_letter->blklen > p_cacheblock->diskspace)
        {
            status_return(dict_seek_set(p_dict, p_dict->data_offset + p_dict->dictsize + p_letter->blklen + EXT_SIZE));

            status_return(file_putc(0, p_dict->file_handle_dict));

            p_cacheblock->diskaddress = p_dict->data_offset + p_dict->dictsize;
            p_dict->dictsize += (S32) p_letter->blklen + EXT_SIZE;
            p_cacheblock->diskspace += EXT_SIZE;
        }

        /* write out block */
        status_return(dict_seek_set(p_dict, p_cacheblock->diskaddress));

        status_return(file_write_bytes(&p_cacheblock->diskspace, p_letter->blklen + sizeof32(S32), p_dict->file_handle_dict));
    }

    p_letter->letflags &= ~LET_WRITE;

    return(STATUS_OK);
}

/******************************************************************************
*
* write out the index entry for a letter
*
******************************************************************************/

_Check_return_
static STATUS
writeindex(
    _InoutRef_  P_DICT p_dict,
    _InVal_     S32 lettix)
{
    P_LETTER p_letter = array_ptr(&p_dict->h_index, LETTER, lettix);

    /* update index entry for letter */
    status_return(dict_seek_set(p_dict, p_dict->index_offset + (sizeof32(LETTER) * lettix)));

    /* MRJC 25.3.93 - don't write out pointers to NULL blocks */
    if(!p_letter->blklen)
        p_letter->p.disk = 0;

    status_return(file_write_bytes(p_letter, sizeof32(LETTER), p_dict->file_handle_dict));

    return(STATUS_OK);
}

#endif /* SPELL_OFF */

/* end of spell.c */
