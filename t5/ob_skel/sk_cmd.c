/* sk_cmd.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Command routines */

/* SKS April 1992 */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#include "cmodules/collect.h"

/*
internal types
*/

/*
define association between a key code, an internal text representation and a display representation
*/

typedef struct KMAP_ASSOC
{
    KMAP_CODE   kmap_code;
    T5_MESSAGE  t5_message;
    TCHARZ      key_name[8];   /* this language-independent name goes out to command file */
    U16         resource_id;
    U8          long_prefix_index;
    U8          menu_prefix_index;
}
KMAP_ASSOC, * P_KMAP_ASSOC; typedef const KMAP_ASSOC * PC_KMAP_ASSOC;

#define PREFIX_S  1
#define PREFIX_C  2
#define PREFIX_CS 3

/*
table index type
*/

typedef U8 KMAP_ASSOC_INDEX; typedef KMAP_ASSOC_INDEX * P_KMAP_ASSOC_INDEX; typedef const KMAP_ASSOC_INDEX * PC_KMAP_ASSOC_INDEX;

/*
internal routines
*/

static S32
execution_contexts_handle_in_use(
    _InVal_     ARRAY_HANDLE handle);

_Check_return_
static STATUS
execution_context_pull(void);

_Check_return_
static STATUS
execution_context_push(void);

_Check_return_
static PC_KMAP_ASSOC
kmap_assoc_code_lookup(
    _In_        KMAP_CODE code);

PROC_BSEARCH_PROTO(static, kmap_assoc_code_search, KMAP_CODE, KMAP_ASSOC);

PROC_BSEARCH_PROTO_Z(static, kmap_assoc_index_text_search, TCHARZ, KMAP_ASSOC_INDEX);

_Check_return_
extern STATUS
check_protection_selection(
    _DocuRef_   P_DOCU p_docu)
{
    if(p_docu->flags.read_only)
        return(ERR_READ_ONLY);

    if(p_docu->mark_info_cells.h_markers)
    {
        CHECK_PROTECTION check_protection;
        docu_area_normalise(p_docu, &check_protection.docu_area, p_docu_area_from_markers_first(p_docu));
        check_protection.status_line_message = FALSE;
        check_protection.use_docu_area = TRUE;
        return(object_skel(p_docu, T5_MSG_CHECK_PROTECTION, &check_protection));
    }

    return(check_protection_simple(p_docu, FALSE));
}

_Check_return_
extern STATUS
check_protection_simple(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     BOOL status_line_message)
{
    if(p_docu->flags.read_only)
    {
        STATUS status = ERR_READ_ONLY;
        if(status_line_message)
        {
            UI_TEXT ui_text;
            ui_text.type = UI_TEXT_TYPE_RESID;
            ui_text.text.resource_id = status;
            status_line_set(p_docu, STATUS_LINE_LEVEL_AUTO_CLEAR, &ui_text);
            host_bleep();
        }
        return(status);
    }

    {
    CHECK_PROTECTION check_protection;
    check_protection.status_line_message = status_line_message;
    check_protection.use_docu_area = FALSE;
    return(object_skel(p_docu, T5_MSG_CHECK_PROTECTION, &check_protection));
    } /*block*/
}

extern void
docu_modify(
    _DocuRef_   P_DOCU p_docu)
{
    assert(p_docu->modified >= 0);

    if(!p_docu->flags.allow_modified_change)
        return;

    assert(!p_docu->flags.read_only);

    if(0 == p_docu->modified++)
        /* SKS 04apr95/13oct95 register time of *first* update as new time this session - will be updated on save */
        ss_local_time_to_ss_date(&p_docu->file_ss_date);

#if 0
    status_assert(maeve_event(p_docu, T5_MSG_DOCU_MODCHANGE, P_DATA_NONE));
#endif

    if(p_docu->modified == 1)
        status_assert(maeve_event(p_docu, T5_MSG_DOCU_MODSTATUS, P_DATA_NONE));
}

extern void
docu_modify_clear(
    _DocuRef_   P_DOCU p_docu)
{
    assert(p_docu->modified >= 0);

    if(!p_docu->flags.allow_modified_change)
        return;

    if(!p_docu->modified)
        return;

    p_docu->modified = 0;

#if 0
    status_assert(maeve_event(p_docu, T5_MSG_DOCU_MODCHANGE, P_DATA_NONE));
#endif

    status_assert(maeve_event(p_docu, T5_MSG_DOCU_MODSTATUS, P_DATA_NONE));
}

#ifndef p_command_recorder_of_op_format
/*
a pointer to the macro file currently being recorded, NULL if none
*/

P_OF_OP_FORMAT
p_command_recorder_of_op_format = NULL;
#endif

/*
the current execution context
*/

struct EXECUTION_CONTEXT_CORE
{
    DOCNO        docno;

    struct EXECUTION_CONTEXT_CORE_LAST_CMD
    {
        ARGLIST_HANDLE arglist_handle;
        UBF            interactive : 1;
        OBJECT_ID      object_id;
        T5_MESSAGE     t5_message;
    } last_cmd;

    POSITION     offset;
    UBF          use_offset : 1;
    ARRAY_HANDLE replay_handle;
};

static
struct CURRENT_EXECUTION_CONTEXT
{
    struct EXECUTION_CONTEXT_CORE core;

    struct CURRENT_EXECUTION_CONTEXT_COMMAND_RECORDER
    {
        OF_OP_FORMAT   of_op_format;
        ARRAY_HANDLE   mem_tmp;   /* temporary reception area */
        P_ARRAY_HANDLE p_mem_dst; /* where to receive the in-core file when it is completed */
    } command_recorder;
}
current_execution_context;

/*
execution contexts may be stacked
*/

typedef struct STACKED_EXECUTION_CONTEXT
{
    struct EXECUTION_CONTEXT_CORE core;
}
STACKED_EXECUTION_CONTEXT, * P_STACKED_EXECUTION_CONTEXT;

static ARRAY_HANDLE
stacked_execution_contexts;

/*
the key association - not particularly sorted in source, sorted by code at runtime, use index below for text lookup
*/

static /*effectively const*/ struct KMAP_ASSOC
kmap_assoc_table[] =
{
    /*
    simple chars that are mutated into commands
    */

    { KMAP_ESCAPE,             T5_CMD_ESCAPE,                 TEXT("ESC") },

#if 0
    { '\x1F',                  T5_CMD_INSERT_FIELD_HARDH,     TEXT("NBH") },  /* Non-breaking Hyphen */
#endif
    { '\xAD',                  T5_CMD_INSERT_FIELD_SOFT_HYPHEN, TEXT("SHY") },  /* ISO 8859-1 Soft Hyphen */

    /*
    function keys              vvv this field is used to buffer up simple commands
    */

    { KMAP_FUNC_PRINT,         T5_EVENT_NONE,                 TEXT("Print"),       MSG_FUNC_PRINT,     0, 0 },
    { KMAP_BASE_FUNC + 1,      T5_EVENT_NONE,                 TEXT("F1"),          MSG_FUNC_F1,        0, 0 },
    { KMAP_BASE_FUNC + 2,      T5_EVENT_NONE,                 TEXT("F2"),          MSG_FUNC_F2,        0, 0 },
    { KMAP_BASE_FUNC + 3,      T5_EVENT_NONE,                 TEXT("F3"),          MSG_FUNC_F3,        0, 0 },
    { KMAP_BASE_FUNC + 4,      T5_EVENT_NONE,                 TEXT("F4"),          MSG_FUNC_F4,        0, 0 },
    { KMAP_BASE_FUNC + 5,      T5_EVENT_NONE,                 TEXT("F5"),          MSG_FUNC_F5,        0, 0 },
    { KMAP_BASE_FUNC + 6,      T5_EVENT_NONE,                 TEXT("F6"),          MSG_FUNC_F6,        0, 0 },
    { KMAP_BASE_FUNC + 7,      T5_EVENT_NONE,                 TEXT("F7"),          MSG_FUNC_F7,        0, 0 },
    { KMAP_BASE_FUNC + 8,      T5_EVENT_NONE,                 TEXT("F8"),          MSG_FUNC_F8,        0, 0 },
    { KMAP_BASE_FUNC + 9,      T5_EVENT_NONE,                 TEXT("F9"),          MSG_FUNC_F9,        0, 0 },
    { KMAP_BASE_FUNC + 10,     T5_EVENT_NONE,                 TEXT("F10"),         MSG_FUNC_F10,       0, 0 },
    { KMAP_BASE_FUNC + 11,     T5_EVENT_NONE,                 TEXT("F11"),         MSG_FUNC_F11,       0, 0 },
    { KMAP_BASE_FUNC + 12,     T5_EVENT_NONE,                 TEXT("F12"),         MSG_FUNC_F12,       0, 0 },
    { KMAP_FUNC_INSERT,        T5_EVENT_NONE,                 TEXT("Ins"),         MSG_FUNC_INSERT,    0, 0 },
#if RISCOS
    { KMAP_FUNC_DELETE,        T5_CMD_DELETE_CHARACTER_LEFT,  TEXT("Del"),         MSG_FUNC_DELETE,    0, 0 },
#else
    { KMAP_FUNC_DELETE,        T5_CMD_DELETE_CHARACTER_RIGHT, TEXT("Del"),         MSG_FUNC_DELETE,    0, 0 },
#endif
    { KMAP_FUNC_HOME,          T5_CMD_LINE_START,             TEXT("Home"),        MSG_FUNC_HOME,      0, 0 },
    { KMAP_FUNC_BACKSPACE,     T5_CMD_DELETE_CHARACTER_LEFT,  TEXT("Bsp"),         MSG_FUNC_BACKSPACE, 0, 0 },
    { KMAP_FUNC_RETURN,        T5_CMD_RETURN,                 TEXT("Ret"),         MSG_FUNC_RETURN,    0, 0 },
    { KMAP_FUNC_PAGE_DOWN,     T5_CMD_PAGE_DOWN,              TEXT("PgDn"),        MSG_FUNC_PAGE_DOWN, 0, 0 },
    { KMAP_FUNC_PAGE_UP,       T5_CMD_PAGE_UP,                TEXT("PgUp"),        MSG_FUNC_PAGE_UP,   0, 0 },
    { KMAP_FUNC_TAB,           T5_CMD_TAB_RIGHT,              TEXT("Tab"),         MSG_FUNC_TAB,       0, 0 },
#if RISCOS
    { KMAP_FUNC_END,           T5_CMD_DELETE_CHARACTER_RIGHT, TEXT("End"),         MSG_FUNC_END,       0, 0 },
#else
    { KMAP_FUNC_END,           T5_CMD_LINE_END,               TEXT("End"),         MSG_FUNC_END,       0, 0 },
#endif
    { KMAP_FUNC_ARROW_LEFT,    T5_CMD_CURSOR_LEFT,            TEXT("Left"),        MSG_FUNC_LEFT,      0, 0 },
    { KMAP_FUNC_ARROW_RIGHT,   T5_CMD_CURSOR_RIGHT,           TEXT("Right"),       MSG_FUNC_RIGHT,     0, 0 },
    { KMAP_FUNC_ARROW_DOWN,    T5_CMD_CURSOR_DOWN,            TEXT("Down"),        MSG_FUNC_DOWN,      0, 0 },
    { KMAP_FUNC_ARROW_UP,      T5_CMD_CURSOR_UP,              TEXT("Up"),          MSG_FUNC_UP,        0, 0 },

    { KMAP_FUNC_SPRINT,        T5_EVENT_NONE,                 TEXT("!Print"),      MSG_FUNC_PRINT,     PREFIX_S, PREFIX_S },
    { KMAP_BASE_SFUNC + 1,     T5_EVENT_NONE,                 TEXT("!F1"),         MSG_FUNC_F1,        PREFIX_S, PREFIX_S },
    { KMAP_BASE_SFUNC + 2,     T5_EVENT_NONE,                 TEXT("!F2"),         MSG_FUNC_F2,        PREFIX_S, PREFIX_S },
    { KMAP_BASE_SFUNC + 3,     T5_EVENT_NONE,                 TEXT("!F3"),         MSG_FUNC_F3,        PREFIX_S, PREFIX_S },
    { KMAP_BASE_SFUNC + 4,     T5_EVENT_NONE,                 TEXT("!F4"),         MSG_FUNC_F4,        PREFIX_S, PREFIX_S },
    { KMAP_BASE_SFUNC + 5,     T5_EVENT_NONE,                 TEXT("!F5"),         MSG_FUNC_F5,        PREFIX_S, PREFIX_S },
    { KMAP_BASE_SFUNC + 6,     T5_EVENT_NONE,                 TEXT("!F6"),         MSG_FUNC_F6,        PREFIX_S, PREFIX_S },
    { KMAP_BASE_SFUNC + 7,     T5_EVENT_NONE,                 TEXT("!F7"),         MSG_FUNC_F7,        PREFIX_S, PREFIX_S },
    { KMAP_BASE_SFUNC + 8,     T5_EVENT_NONE,                 TEXT("!F8"),         MSG_FUNC_F8,        PREFIX_S, PREFIX_S },
    { KMAP_BASE_SFUNC + 9,     T5_EVENT_NONE,                 TEXT("!F9"),         MSG_FUNC_F9,        PREFIX_S, PREFIX_S },
    { KMAP_BASE_SFUNC + 10,    T5_EVENT_NONE,                 TEXT("!F10"),        MSG_FUNC_F10,       PREFIX_S, PREFIX_S },
    { KMAP_BASE_SFUNC + 11,    T5_EVENT_NONE,                 TEXT("!F11"),        MSG_FUNC_F11,       PREFIX_S, PREFIX_S },
    { KMAP_BASE_SFUNC + 12,    T5_EVENT_NONE,                 TEXT("!F12"),        MSG_FUNC_F12,       PREFIX_S, PREFIX_S },
    { KMAP_FUNC_SINSERT,       T5_CMD_PASTE_AT_CURSOR,        TEXT("!Ins"),        MSG_FUNC_INSERT,    PREFIX_S, PREFIX_S },
    { KMAP_FUNC_SDELETE,       T5_CMD_SELECTION_CUT,          TEXT("!Del"),        MSG_FUNC_DELETE,    PREFIX_S, PREFIX_S },
    { KMAP_FUNC_SHOME,         T5_CMD_SHIFT_LINE_START,       TEXT("!Home"),       MSG_FUNC_HOME,      PREFIX_S, PREFIX_S },
    { KMAP_FUNC_SBACKSPACE,    T5_CMD_DELETE_CHARACTER_RIGHT, TEXT("!Bsp"),        MSG_FUNC_BACKSPACE, PREFIX_S, PREFIX_S },
    { KMAP_FUNC_SRETURN,       T5_EVENT_NONE,                 TEXT("!Ret"),        MSG_FUNC_RETURN,    PREFIX_S, PREFIX_S },
    { KMAP_FUNC_SPAGE_DOWN,    T5_CMD_SHIFT_PAGE_DOWN,        TEXT("!PgDn"),       MSG_FUNC_PAGE_DOWN, PREFIX_S, PREFIX_S },
    { KMAP_FUNC_SPAGE_UP,      T5_CMD_SHIFT_PAGE_UP,          TEXT("!PgUp"),       MSG_FUNC_PAGE_UP,   PREFIX_S, PREFIX_S },
    { KMAP_FUNC_STAB,          T5_CMD_TAB_LEFT,               TEXT("!Tab"),        MSG_FUNC_TAB,       PREFIX_S, PREFIX_S },
    { KMAP_FUNC_SEND,          T5_CMD_SHIFT_LINE_END,         TEXT("!End"),        MSG_FUNC_END,       PREFIX_S, PREFIX_S },
#if RISCOS
    { KMAP_FUNC_SARROW_LEFT,   T5_CMD_WORD_LEFT,              TEXT("!Left"),       MSG_FUNC_LEFT,      PREFIX_S, PREFIX_S },
    { KMAP_FUNC_SARROW_RIGHT,  T5_CMD_WORD_RIGHT,             TEXT("!Right"),      MSG_FUNC_RIGHT,     PREFIX_S, PREFIX_S },
#else
    { KMAP_FUNC_SARROW_LEFT,   T5_CMD_SHIFT_CURSOR_LEFT,      TEXT("!Left"),       MSG_FUNC_LEFT,      PREFIX_S, PREFIX_S },
    { KMAP_FUNC_SARROW_RIGHT,  T5_CMD_SHIFT_CURSOR_RIGHT,     TEXT("!Right"),      MSG_FUNC_RIGHT,     PREFIX_S, PREFIX_S },
#endif
    { KMAP_FUNC_SARROW_DOWN,   T5_CMD_SHIFT_CURSOR_DOWN,      TEXT("!Down"),       MSG_FUNC_PAGE_DOWN, 0, 0 },
    { KMAP_FUNC_SARROW_UP,     T5_CMD_SHIFT_CURSOR_UP,        TEXT("!Up"),         MSG_FUNC_PAGE_UP,   0, 0 },

    { KMAP_FUNC_CPRINT,        T5_EVENT_NONE,                 TEXT("^Print"),      MSG_FUNC_PRINT,     PREFIX_C, PREFIX_C },
    { KMAP_BASE_CFUNC + 1,     T5_EVENT_NONE,                 TEXT("^F1"),         MSG_FUNC_F1,        PREFIX_C, PREFIX_C },
    { KMAP_BASE_CFUNC + 2,     T5_EVENT_NONE,                 TEXT("^F2"),         MSG_FUNC_F2,        PREFIX_C, PREFIX_C },
    { KMAP_BASE_CFUNC + 3,     T5_EVENT_NONE,                 TEXT("^F3"),         MSG_FUNC_F3,        PREFIX_C, PREFIX_C },
    { KMAP_BASE_CFUNC + 4,     T5_EVENT_NONE,                 TEXT("^F4"),         MSG_FUNC_F4,        PREFIX_C, PREFIX_C },
    { KMAP_BASE_CFUNC + 5,     T5_EVENT_NONE,                 TEXT("^F5"),         MSG_FUNC_F5,        PREFIX_C, PREFIX_C },
    { KMAP_BASE_CFUNC + 6,     T5_EVENT_NONE,                 TEXT("^F6"),         MSG_FUNC_F6,        PREFIX_C, PREFIX_C },
    { KMAP_BASE_CFUNC + 7,     T5_EVENT_NONE,                 TEXT("^F7"),         MSG_FUNC_F7,        PREFIX_C, PREFIX_C },
    { KMAP_BASE_CFUNC + 8,     T5_EVENT_NONE,                 TEXT("^F8"),         MSG_FUNC_F8,        PREFIX_C, PREFIX_C },
    { KMAP_BASE_CFUNC + 9,     T5_EVENT_NONE,                 TEXT("^F9"),         MSG_FUNC_F9,        PREFIX_C, PREFIX_C },
    { KMAP_BASE_CFUNC + 10,    T5_EVENT_NONE,                 TEXT("^F10"),        MSG_FUNC_F10,       PREFIX_C, PREFIX_C },
    { KMAP_BASE_CFUNC + 11,    T5_EVENT_NONE,                 TEXT("^F11"),        MSG_FUNC_F11,       PREFIX_C, PREFIX_C },
    { KMAP_BASE_CFUNC + 12,    T5_EVENT_NONE,                 TEXT("^F12"),        MSG_FUNC_F12,       PREFIX_C, PREFIX_C },
    { KMAP_FUNC_CINSERT,       T5_CMD_SELECTION_COPY,         TEXT("^Ins"),        MSG_FUNC_INSERT,    PREFIX_C, PREFIX_C },
    { KMAP_FUNC_CDELETE,       T5_EVENT_NONE,                 TEXT("^Del"),        MSG_FUNC_DELETE,    PREFIX_C, PREFIX_C },
    { KMAP_FUNC_CHOME,         T5_CMD_DOCUMENT_TOP,           TEXT("^Home"),       MSG_FUNC_HOME,      PREFIX_C, PREFIX_C },
    { KMAP_FUNC_CBACKSPACE,    T5_EVENT_NONE,                 TEXT("^Bsp"),        MSG_FUNC_BACKSPACE, PREFIX_C, PREFIX_C },
    { KMAP_FUNC_CRETURN,       T5_CMD_INSERT_FIELD_RETURN,    TEXT("^Ret"),        MSG_FUNC_RETURN,    PREFIX_C, PREFIX_C },
    { KMAP_FUNC_CTAB,          T5_CMD_LAST_COLUMN,            TEXT("^Tab"),        MSG_FUNC_TAB,       PREFIX_C, PREFIX_C },
    { KMAP_FUNC_CEND,          T5_CMD_DOCUMENT_BOTTOM,        TEXT("^End"),        MSG_FUNC_END,       PREFIX_C, PREFIX_C },
#if RISCOS
    { KMAP_FUNC_CARROW_LEFT,   T5_CMD_LINE_START,             TEXT("^Left"),       MSG_FUNC_LEFT,      PREFIX_C, PREFIX_C },
    { KMAP_FUNC_CARROW_RIGHT,  T5_CMD_LINE_END,               TEXT("^Right"),      MSG_FUNC_RIGHT,     PREFIX_C, PREFIX_C },
#else
    { KMAP_FUNC_CARROW_LEFT,   T5_CMD_WORD_LEFT,              TEXT("^Left"),       MSG_FUNC_LEFT,      PREFIX_C, PREFIX_C },
    { KMAP_FUNC_CARROW_RIGHT,  T5_CMD_WORD_RIGHT,             TEXT("^Right"),      MSG_FUNC_RIGHT,     PREFIX_C, PREFIX_C },
#endif
    { KMAP_FUNC_CARROW_DOWN,   T5_CMD_DOCUMENT_BOTTOM,        TEXT("^Down"),       MSG_FUNC_DOWN,      PREFIX_C, PREFIX_C },
    { KMAP_FUNC_CARROW_UP,     T5_CMD_DOCUMENT_TOP,           TEXT("^Up"),         MSG_FUNC_UP,        PREFIX_C, PREFIX_C },

    { KMAP_FUNC_CSPRINT,       T5_EVENT_NONE,                 TEXT("^!Print"),     MSG_FUNC_PRINT,     PREFIX_CS, PREFIX_CS },
    { KMAP_BASE_CSFUNC + 1,    T5_EVENT_NONE,                 TEXT("^!F1"),        MSG_FUNC_F1,        PREFIX_CS, PREFIX_CS },
    { KMAP_BASE_CSFUNC + 2,    T5_EVENT_NONE,                 TEXT("^!F2"),        MSG_FUNC_F2,        PREFIX_CS, PREFIX_CS },
    { KMAP_BASE_CSFUNC + 3,    T5_EVENT_NONE,                 TEXT("^!F3"),        MSG_FUNC_F3,        PREFIX_CS, PREFIX_CS },
    { KMAP_BASE_CSFUNC + 4,    T5_EVENT_NONE,                 TEXT("^!F4"),        MSG_FUNC_F4,        PREFIX_CS, PREFIX_CS },
    { KMAP_BASE_CSFUNC + 5,    T5_EVENT_NONE,                 TEXT("^!F5"),        MSG_FUNC_F5,        PREFIX_CS, PREFIX_CS },
    { KMAP_BASE_CSFUNC + 6,    T5_EVENT_NONE,                 TEXT("^!F6"),        MSG_FUNC_F6,        PREFIX_CS, PREFIX_CS },
    { KMAP_BASE_CSFUNC + 7,    T5_EVENT_NONE,                 TEXT("^!F7"),        MSG_FUNC_F7,        PREFIX_CS, PREFIX_CS },
    { KMAP_BASE_CSFUNC + 8,    T5_EVENT_NONE,                 TEXT("^!F8"),        MSG_FUNC_F8,        PREFIX_CS, PREFIX_CS },
    { KMAP_BASE_CSFUNC + 9,    T5_EVENT_NONE,                 TEXT("^!F9"),        MSG_FUNC_F9,        PREFIX_CS, PREFIX_CS },
    { KMAP_BASE_CSFUNC + 10,   T5_EVENT_NONE,                 TEXT("^!F10"),       MSG_FUNC_F10,       PREFIX_CS, PREFIX_CS },
    { KMAP_BASE_CSFUNC + 11,   T5_EVENT_NONE,                 TEXT("^!F11"),       MSG_FUNC_F11,       PREFIX_CS, PREFIX_CS },
    { KMAP_BASE_CSFUNC + 12,   T5_EVENT_NONE,                 TEXT("^!F12"),       MSG_FUNC_F12,       PREFIX_CS, PREFIX_CS },
    { KMAP_FUNC_CSINSERT,      T5_EVENT_NONE,                 TEXT("^!Ins"),       MSG_FUNC_INSERT,    PREFIX_CS, PREFIX_CS },
    { KMAP_FUNC_CSDELETE,      T5_EVENT_NONE,                 TEXT("^!Del"),       MSG_FUNC_DELETE,    PREFIX_CS, PREFIX_CS },
    { KMAP_FUNC_CSHOME,        T5_CMD_SHIFT_DOCUMENT_TOP,     TEXT("^!Home"),      MSG_FUNC_HOME,      PREFIX_CS, PREFIX_CS },
    { KMAP_FUNC_CSBACKSPACE,   T5_EVENT_NONE,                 TEXT("^!Bsp"),       MSG_FUNC_BACKSPACE, PREFIX_CS, PREFIX_CS },
    { KMAP_FUNC_CSRETURN,      T5_EVENT_NONE,                 TEXT("^!Ret"),       MSG_FUNC_RETURN,    PREFIX_CS, PREFIX_CS },
    { KMAP_FUNC_CSTAB,         T5_CMD_FIRST_COLUMN,           TEXT("^!Tab"),       MSG_FUNC_TAB,       PREFIX_CS, PREFIX_CS },
    { KMAP_FUNC_CSEND,         T5_CMD_SHIFT_DOCUMENT_BOTTOM,  TEXT("^!End"),       MSG_FUNC_END,       PREFIX_CS, PREFIX_CS },
    { KMAP_FUNC_CSARROW_LEFT,  T5_CMD_SHIFT_WORD_LEFT,        TEXT("^!Left"),      MSG_FUNC_LEFT,      PREFIX_CS, PREFIX_CS },
    { KMAP_FUNC_CSARROW_RIGHT, T5_CMD_SHIFT_WORD_RIGHT,       TEXT("^!Right"),     MSG_FUNC_RIGHT,     PREFIX_CS, PREFIX_CS },
    { KMAP_FUNC_CSARROW_DOWN,  T5_CMD_SHIFT_DOCUMENT_BOTTOM,  TEXT("^!Down"),      MSG_FUNC_PAGE_DOWN, PREFIX_C,  PREFIX_C  },
    { KMAP_FUNC_CSARROW_UP,    T5_CMD_SHIFT_DOCUMENT_TOP,     TEXT("^!Up"),        MSG_FUNC_PAGE_UP,   PREFIX_C,  PREFIX_C  }
};

/*
sorted index of the above kmap_assoc_table
*/

static P_KMAP_ASSOC_INDEX
kmap_assoc_index_text;

/******************************************************************************
*
* assign a array handle of commands to a key code
*
******************************************************************************/

_Check_return_
extern STATUS
command_array_handle_assign_to_key_code(
    _DocuRef_   P_DOCU p_docu,
    _In_        KMAP_CODE code,
    _InVal_     ARRAY_HANDLE h_commands)
{
    LIST_ITEMNO key = code;
    P_LISTED_KEY_DEF p_key = collect_goto_item(LISTED_KEY_DEF, &p_docu->defined_keys, key);
    STATUS status;

    if(NULL != p_key)
    {
        /* see if it is in use a ce moment */
        if(execution_contexts_handle_in_use(p_key->definition_handle))
            return(create_error(ERR_KEY_DEF_IN_USE));

        al_array_dispose(&p_key->definition_handle);
    }
    else
    {
        if(NULL == (p_key = collect_add_entry_elem(LISTED_KEY_DEF, &p_docu->defined_keys, P_DATA_NONE, key, &status)))
            return(status);

        zero_struct_ptr(p_key);
    }

    status = al_array_duplicate(&p_key->definition_handle, &h_commands);

    if(status_fail(status))
        collect_subtract_entry(&p_docu->defined_keys, key);

    return(STATUS_OK);
}

/******************************************************************************
*
* dispose of the array handle that is associated with a key code
* NB. also removes the key code entry
*
******************************************************************************/

extern void
command_array_handle_dispose_from_key_code(
    _DocuRef_   P_DOCU p_docu,
    _In_        KMAP_CODE code)
{
    LIST_ITEMNO key = code;
    P_LISTED_KEY_DEF p_key = collect_goto_item(LISTED_KEY_DEF, &p_docu->defined_keys, key);

    if(NULL != p_key)
    {
        al_array_dispose(&p_key->definition_handle);

        collect_subtract_entry(&p_docu->defined_keys, key);
    }
}

/******************************************************************************
*
* execute commands from an array - primarily
* for execution of commands on function keys but
* can also be used for sequences off buttons etc.
*
******************************************************************************/

_Check_return_
extern STATUS
command_array_handle_execute(
    _InVal_     DOCNO docno,
    _InVal_     ARRAY_HANDLE h_commands)
{
    P_DOCU p_docu;
    POSITION offset;
    STATUS status;
    OF_IP_FORMAT of_ip_format = OF_IP_FORMAT_INIT;

    status_return(execution_context_push());

    current_execution_context.core.docno         = docno;
    current_execution_context.core.replay_handle = h_commands;

    /* <<< commands executed relative to current document,view,pos when these are determinable */

    p_docu = p_docu_from_docno(current_execution_context.core.docno);
    offset = current_execution_context.core.offset;

/* where does p_docu->cur come into this ? */

    if(status_ok(status = ownform_initialise_load(p_docu, &offset, &of_ip_format, NULL, &h_commands)))
    {
        status = load_ownform_command_file(&of_ip_format);

        p_docu = p_docu_from_docno(current_execution_context.core.docno); /* NB may be P_DOCU_NONE id command has destroyed the document! */

        status_accumulate(status, ownform_finalise_load(p_docu, &of_ip_format));

        status_assertc(status);
    }

    status_assert(execution_context_pull());

    if(status_fail(status))
    {
        if(status != STATUS_FAIL)
            reperr_null(status);

        status = STATUS_FAIL;
    }

    return(status);
}

/******************************************************************************
*
* lookup the array handle that is associated with a key code
*
******************************************************************************/

extern ARRAY_HANDLE
command_array_handle_from_key_code(
    _DocuRef_   P_DOCU p_docu,
    _In_        KMAP_CODE code,
    /*out*/ P_T5_MESSAGE p_t5_message)
{
    {
    LIST_ITEMNO key = code;
    P_LISTED_KEY_DEF p_key;

    if( (NULL != (p_key = collect_goto_item(LISTED_KEY_DEF, &p_docu->defined_keys,                  key)))  ||
        (NULL != (p_key = collect_goto_item(LISTED_KEY_DEF, &p_docu_from_config_wr()->defined_keys, key)))  )
    {
        return(p_key->definition_handle);
    }
    } /*block*/

    if(NULL != p_t5_message)
    {
        PC_KMAP_ASSOC pka = kmap_assoc_code_lookup(code);

        if(pka && pka->t5_message)
            *p_t5_message = pka->t5_message;
        else
        {
            /* try a style */
            if(code >= 0x100)
            {
                STYLE_HANDLE style_handle = STYLE_HANDLE_ENUM_START;
                P_STYLE p_style;

                while(style_enum_styles(p_docu, &p_style, &style_handle) >= 0)
                {
                    if(style_bit_test(p_style, STYLE_SW_KEY))
                    {
                        if(p_style->style_key == code)
                        {
                            *p_t5_message = T5_CMD_STYLE_APPLY_STYLE_HANDLE;
                            return(style_handle);
                        }
                    }
                }
            }

            *p_t5_message = T5_EVENT_NONE;
        }
    }

    return(0);
}

/******************************************************************************
*
* record a command to the current output
*
******************************************************************************/

_Check_return_
extern STATUS
command_record(
    _InVal_     ARGLIST_HANDLE arglist_handle,
    _InVal_     OBJECT_ID object_id,
    _InRef_     PC_CONSTRUCT_TABLE p_construct_table)
{
#if 0
    STATUS status;

    assert(p_command_recorder_of_op_format);

    if(status_fail(status = ownform_save_arglist(arglist_handle, object_id, p_construct_table, p_command_recorder_of_op_format)))
    {
        command_recorder_abort();
        return(status);
    }
#else
    UNREFERENCED_PARAMETER_InVal_(arglist_handle);
    UNREFERENCED_PARAMETER_InVal_(object_id);
    UNREFERENCED_PARAMETER_InRef_(p_construct_table);
#endif

    return(STATUS_OK);
}

#if 0

/******************************************************************************
*
* start recording commands to real file
*
******************************************************************************/

_Check_return_
extern STATUS
command_recorder_start_file(
    _In_z_      PCTSTR filename)
{
    if(NULL != p_command_recorder_of_op_format)
        return(create_error(ERR_ALREADYRECORDING));

    status_return(ownform_initialise_save(p_docu_config,
                                          NULL,
                                          FILETYPE_T5_COMMAND,
                                          &current_execution_context.command_recorder.of_op_format,
                                          NULL,
                                          filename));

    p_command_recorder_of_op_format = &current_execution_context.command_recorder.of_op_format;

    /* it's going to a file so no array handle to put anywhere */
    current_execution_context.command_recorder.p_mem_dst = NULL;

    return(STATUS_OK);
}

/******************************************************************************
*
* start recording commands to in-core file
*
******************************************************************************/

_Check_return_
extern STATUS
command_recorder_start_mem(
    P_ARRAY_HANDLE p_array_handle)
{
    if(NULL != p_command_recorder_of_op_format)
        return(create_error(ERR_ALREADYRECORDING));

    status_return(ownform_initialise_save(p_docu_config,
                                          NULL,
                                          FILETYPE_T5_COMMAND,
                                          &current_execution_context.command_recorder.of_op_format,
                                          &current_execution_context.command_recorder.mem_tmp,
                                          NULL));

    p_command_recorder_of_op_format = &current_execution_context.command_recorder.of_op_format;

    /* where to put the array handle of commands when all is well */
    current_execution_context.command_recorder.p_mem_dst = p_array_handle;

    return(STATUS_OK);
}

/******************************************************************************
*
* stop command recorder, discarding the output
*
******************************************************************************/

_Check_return_
extern STATUS
command_recorder_abort(void)
{
    P_OF_OP_FORMAT p_of_op_format = p_command_recorder_of_op_format;
    STATUS         status;

    /* switch command recorder off */
    p_command_recorder_of_op_format = NULL;

    if(NULL == p_of_op_format)
        /* command recorder was already switched off */
        return(STATUS_OK);

    status = ownform_finalise_save(NULL, p_of_op_format, STATUS_FAIL);

    if(current_execution_context.command_recorder.p_mem_dst)
    {
        /* recording to memory, can discard the handle now */
        al_array_dispose(&current_execution_context.command_recorder.mem_tmp);

        current_execution_context.command_recorder.p_mem_dst = NULL;

        /* I choose here not to destroy the punter's receiving handle */

        status_return(status);
    }
    else
    {
        /* recording to file, <<< may choose to remove the evidence */

        status_return(status);
    }

    return(STATUS_OK);
}

/******************************************************************************
*
* stop command recorder normally
*
******************************************************************************/

_Check_return_
extern STATUS
command_recorder_stop(void)
{
    P_OF_OP_FORMAT p_of_op_format = p_command_recorder_of_op_format;
    STATUS         status;

    /* switch command recorder off */
    p_command_recorder_of_op_format = NULL;

    if(NULL == p_of_op_format)
        /* command recorder was already switched off */
        return(STATUS_OK);

    status = ownform_finalise_save(NULL, p_of_op_format, STATUS_OK);

#if 0
    if(NULL != p_command_recorder_mem_dst)
    {
        /* recording to memory, can give him ownership of the handle now */
        if(status_fail(status))
            al_array_dispose(&command_recorder_mem_tmp);

        *p_command_recorder_mem_dst = command_recorder_mem_tmp;
        command_recorder_mem_tmp    = 0;
        p_command_recorder_mem_dst  = NULL;

        status_return(status);
    }
    else
    {
        /* recording to file, should be ok */

        status_return(status);
    }
#endif

    return(STATUS_OK);
}

#endif

/* note any sneaky changes in document number */

_Check_return_
extern STATUS
command_set_current_docu(
    _DocuRef_   P_DOCU p_docu)
{
    const DOCNO docno = docno_from_p_docu(p_docu);

    if(current_execution_context.core.docno != docno)
    {
        current_execution_context.core.docno = docno;

#if 0
        /* and record it */
        if(NULL != p_command_recorder_of_op_format)
        {
            const OBJECT_ID object_id = OBJECT_ID_SKEL;
            PC_CONSTRUCT_TABLE p_construct_table;
            ARGLIST_HANDLE arglist_handle;
            STATUS status;

            if(status_ok(status = arglist_prepare_with_construct(&arglist_handle, object_id, T5_CMD_CURRENT_DOCUMENT, &p_construct_table)))
            {
                const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 1);
                p_args[0].val.tstr = file_leafname(p_docu->name.leaf_name);
                status = command_record(arglist_handle, object_id, p_construct_table);
                arglist_dispose(&arglist_handle);
            }

            status_return(status);
        }
#endif

        return(STATUS_DONE);
    }

    return(STATUS_OK);
}

_Check_return_
extern STATUS
command_set_current_view(
    _DocuRef_   P_DOCU p_docu,
    _ViewRef_   P_VIEW p_view)
{
    BOOL change_view = 0;
    STATUS status;

    status_return(status = command_set_current_docu(p_docu));

    /* if changed document tell us what view we're getting (unless only zero or one views) */
    if(status == 1)
        change_view = 1;

    {
    VIEWNO viewno = (VIEWNO) (p_view ? viewno_from_p_view(p_view) : VIEWNO_NONE);

    if(p_docu->viewno_caret != viewno)
    {
        p_docu->viewno_caret = viewno;

        change_view = 1;
    }
    } /*block*/

#if 0
    if(change_view && p_docu->n_views && p_command_recorder_of_op_format)
    {
        const OBJECT_ID object_id = OBJECT_ID_SKEL;
        PC_CONSTRUCT_TABLE p_construct_table;
        ARGLIST_HANDLE arglist_handle;

        if(status_ok(status = arglist_prepare_with_construct(&arglist_handle, object_id, T5_CMD_CURRENT_VIEW, &p_construct_table)))
        {
            const P_ARGLIST_ARG p_args = p_arglist_args(&arglist_handle, 1);
            S32 view_id = view_return_extid(p_docu, p_view);
            p_args[0].val.s32 = view_id;
            status = command_record(arglist_handle, object_id, p_construct_table);
            arglist_dispose(&arglist_handle);
        }

        status_return(status);
    }
#endif

    if(change_view)
        caret_show_claim(p_docu, p_docu->focus_owner, FALSE);

    return(STATUS_OK);
}

static BOOL go_interactive = 0;

_Check_return_
extern BOOL
command_query_interactive(void)
{
    return(go_interactive);
}

extern void
command_set_interactive(void)
{
    go_interactive = 1;
}

_Check_return_
static BOOL
is_interactive(void)
{
    BOOL interactive = go_interactive;

    if(interactive)
        go_interactive = 0;

    return(interactive);
}

_Check_return_
static inline BOOL
cmd_interactive(
    _InRef_     PC_CONSTRUCT_TABLE p_construct_table)
{
    return(p_construct_table->bits.maybe_interactive ? is_interactive() : FALSE);
}

/******************************************************************************
*
* fire a given Fireworkz command at an object with given arglist
*
* NB only error return is STATUS_FAIL, all other errors
* have been reported before this procedure returns
*
******************************************************************************/

_Check_return_
static STATUS
execute_command_common(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_CONSTRUCT_TABLE p_construct_table,
    _InoutRef_  P_T5_CMD p_t5_cmd,
    _InVal_     OBJECT_ID object_id)
{
    STATUS status = STATUS_OK;

    if(p_construct_table->bits.ensure_frothy)
        status = ensure_memory_froth();

    if(status_ok(status))
        if(p_construct_table->bits.check_protect)
            if(DOCU_NOT_NONE(p_docu))
                status = check_protection_selection(p_docu);

    /* note any sneaky changes in document number */
    if(status_ok(status))
        status = command_set_current_docu(p_docu);

    if(status_ok(status))
    {
        const DOCNO docno = docno_from_p_docu(p_docu);

        if(!p_t5_cmd->interactive && p_construct_table->bits.modify_doc)
            docu_modify(p_docu);

        if(p_construct_table->bits.wrap_with_before_after)
            cur_change_before(p_docu);

        if(p_construct_table->bits.send_via_maeve_service)
            status = maeve_service_event(p_docu, p_construct_table->t5_message, p_t5_cmd);
        else if(p_construct_table->bits.send_to_focus_owner)
            status = docu_focus_owner_object_call(p_docu, p_construct_table->t5_message, p_t5_cmd);
        else
            status = object_call_id(object_id, p_docu, p_construct_table->t5_message, p_t5_cmd);

        p_docu = p_docu_from_docno(docno);

        if(p_construct_table->bits.wrap_with_before_after)
            cur_change_after(p_docu);
    }

#if 0
    /* if the command is recordable and recording is on then record it iff OK
     * note that repeated commands would go fully into the file a second time
    */
    if(!p_construct_table->bits.unrecordable && !interactive && p_command_recorder_of_op_format && status_ok(status))
        status = command_record(p_t5_cmd->arglist_handle, object_id, p_construct_table);
#endif

    return(status);
}

_Check_return_
extern STATUS
execute_command(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    _InRef_opt_ PC_ARGLIST_HANDLE pc_arglist_handle,
    _InVal_     OBJECT_ID object_id)
{
    PC_CONSTRUCT_TABLE p_construct_table;
    T5_CMD cmd;

    if(!object_present(object_id))
    {
        myassert2(TEXT("execute_command(command=") S32_TFMT TEXT("): Object ") S32_TFMT TEXT(" not present"), t5_message, (S32) object_id);
        return(STATUS_OK);
    }

    if(NULL == (p_construct_table = construct_table_lookup_message(object_id, t5_message)))
    {
        myassert1(TEXT("execute_command(command=") S32_TFMT TEXT("): command not present"), t5_message);
        return(STATUS_OK);
    }

    cmd.arglist_handle = IS_P_DATA_NONE(pc_arglist_handle) ? 0 : *pc_arglist_handle; /* owned by caller; command may NOT steal it or its args */
    cmd.interactive = cmd_interactive(p_construct_table);
    cmd.count = 1;
    cmd.p_of_ip_format = NULL;

    return(execute_command_common(p_docu, p_construct_table, &cmd, object_id));
}

_Check_return_
extern STATUS
execute_command_reperr(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    _InRef_opt_ PC_ARGLIST_HANDLE pc_arglist_handle,
    _InVal_     OBJECT_ID object_id)
{
    STATUS status = execute_command(p_docu, t5_message, pc_arglist_handle, object_id);

    if(status_fail(status))
    {
        if(status != STATUS_FAIL)
            reperr_null(status);

        status = STATUS_FAIL;
    }

    return(status);
}

_Check_return_
extern STATUS
execute_command_n(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    _InVal_     OBJECT_ID object_id,
    _InVal_     U32 count)
{
    PC_CONSTRUCT_TABLE p_construct_table;
    T5_CMD cmd;

    if(!object_present(object_id))
    {
        myassert2(TEXT("execute_simple_command(command=") S32_TFMT TEXT("): Object ") S32_TFMT TEXT(" not present"), t5_message, (S32) object_id);
        return(STATUS_OK);
    }

    if(NULL == (p_construct_table = construct_table_lookup_message(object_id, t5_message)))
    {
        myassert1(TEXT("execute_simple_command(command=") S32_TFMT TEXT("): command not present"), t5_message);
        return(STATUS_OK);
    }

    cmd.arglist_handle = 0;
    cmd.interactive = cmd_interactive(p_construct_table);
    cmd.count = count;
    cmd.p_of_ip_format = NULL;

    return(execute_command_common(p_docu, p_construct_table, &cmd, object_id));
}

/******************************************************************************
*
* an object has just detected a CMD construct being loaded
* so has asked us to despatch it to its event proc with
* command recorder defuddling
*
******************************************************************************/

_Check_return_
extern STATUS
execute_loaded_command(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_CONSTRUCT_CONVERT p_construct_convert,
    _InVal_     OBJECT_ID object_id)
{
    T5_CMD cmd;

#if TRACE_ALLOWED
    trace_2(TRACE_APP_ARGLIST, TEXT("loading command '%s' object id ") S32_TFMT, report_sbstr(p_construct_convert->p_construct->a7str_construct_name), (S32) object_id);
    arglist_trace(&p_construct_convert->arglist_handle, TEXT("command args "));
#endif

    /* NB return errors to caller (probably loader) */
    cmd.arglist_handle = p_construct_convert->arglist_handle; /* owned by caller; command may NOT steal it or its args */
    cmd.interactive = cmd_interactive(p_construct_convert->p_construct);
    cmd.count = 1;
    cmd.p_of_ip_format = p_construct_convert->p_of_ip_format;

    return(execute_command_common(p_docu, p_construct_convert->p_construct, &cmd, object_id));
}

/******************************************************************************
*
* restore a stacked execution context as current
*
******************************************************************************/

_Check_return_
static STATUS
execution_context_pull(void)
{
    P_STACKED_EXECUTION_CONTEXT p;
    S32 tos = array_elements(&stacked_execution_contexts);

    /* take the top element off the stack */
    if(!tos--)
        return(STATUS_FAIL);

    p = array_ptr(&stacked_execution_contexts, STACKED_EXECUTION_CONTEXT, tos);

    /* must dispose of this before reverting to stacked ones */
    arglist_dispose(&current_execution_context.core.last_cmd.arglist_handle);

    current_execution_context.core = p->core;

    if(current_execution_context.core.docno == DOCNO_NONE)
    { /*EMPTY*/ } /* <<< */

    al_array_shrink_by(&stacked_execution_contexts, -1);
    return(STATUS_OK);
}

/******************************************************************************
*
* push the current execution context on a stack
*
******************************************************************************/

_Check_return_
static STATUS
execution_context_push(void)
{
    STATUS status;
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(STACKED_EXECUTION_CONTEXT), 1);
    P_STACKED_EXECUTION_CONTEXT p;

    if(NULL == (p = al_array_extend_by(&stacked_execution_contexts, STACKED_EXECUTION_CONTEXT, 1, &array_init_block, &status)))
        return(status);

    p->core = current_execution_context.core;

    /* when stacked, new process does not inherit these */
    current_execution_context.core.last_cmd.t5_message     = T5_EVENT_NONE;
    current_execution_context.core.last_cmd.arglist_handle = 0;
    current_execution_context.core.replay_handle           = 0;

    return(STATUS_OK);
}

static void
execution_context_startup(void)
{
    current_execution_context.core.docno         = DOCNO_NONE;
    current_execution_context.core.use_offset    = 0;
    current_execution_context.core.replay_handle = 0;
}

/******************************************************************************
*
* signify whether an array handle is being used as a
* source of commands by the command replay system
*
******************************************************************************/

static S32
execution_contexts_handle_in_use(
    _InVal_     ARRAY_HANDLE handle)
{
    P_STACKED_EXECUTION_CONTEXT p;
    S32 i;

    if(!handle)
        return(0);

    if(handle == current_execution_context.core.replay_handle)
        return(1);

    for(i = 0; i < array_elements(&stacked_execution_contexts); ++i)
    {
        p = array_ptr(&stacked_execution_contexts, STACKED_EXECUTION_CONTEXT, i);

        if(p->core.replay_handle == handle)
            return(1);
    }

    return(0);
}

/******************************************************************************
*
* qsort callback to compare two table entries by code field when sorting table by code
*
******************************************************************************/

PROC_QSORT_PROTO(static, kmap_assoc_code_compare, KMAP_ASSOC)
{
    QSORT_ARG1_VAR_DECL(PC_KMAP_ASSOC, pka1);
    QSORT_ARG2_VAR_DECL(PC_KMAP_ASSOC, pka2);

    const KMAP_CODE kmap_code_1 = pka1->kmap_code;
    const KMAP_CODE kmap_code_2 = pka2->kmap_code;

    if(kmap_code_1 > kmap_code_2)
        return(+1);

    if(kmap_code_1 < kmap_code_2)
        return(-1);

    return(0);
}

/******************************************************************************
*
* lookup into table given key code using code index
*
******************************************************************************/

_Check_return_
static PC_KMAP_ASSOC
kmap_assoc_code_lookup(
    _In_        KMAP_CODE code)
{
    PC_KMAP_ASSOC pka = (PC_KMAP_ASSOC)
        bsearch(&code, kmap_assoc_table, elemof(kmap_assoc_table), sizeof(kmap_assoc_table[0]), kmap_assoc_code_search);

    if(!pka)
        return(NULL);

    /* found entry in code index corresponding to lookup code; return kmap_assoc_table entry */
    return(pka);
}

/******************************************************************************
*
* bsearch callback to compare code key with code field of a table entry
*
******************************************************************************/

PROC_BSEARCH_PROTO(static, kmap_assoc_code_search, KMAP_CODE, KMAP_ASSOC)
{
    BSEARCH_KEY_VAR_DECL(PC_KMAP_CODE, key);
    const KMAP_CODE key_code = *key;
    BSEARCH_DATUM_VAR_DECL(PC_KMAP_ASSOC, datum);
    const KMAP_CODE datum_code = datum->kmap_code;

    if(key_code > datum_code)
        return(+1);

    if(key_code < datum_code)
        return(-1);

    return(0);
}

/******************************************************************************
*
* qsort callback to compare two table entries by text field when creating text index
*
******************************************************************************/

/* no context needed for qsort() */

PROC_QSORT_PROTO(static, kmap_assoc_index_text_compare, KMAP_ASSOC_INDEX)
{
    QSORT_ARG1_VAR_DECL(PC_KMAP_ASSOC_INDEX, pka1);
    QSORT_ARG2_VAR_DECL(PC_KMAP_ASSOC_INDEX, pka2);

    PCTSTR text_1 = kmap_assoc_table[(unsigned int) (*pka1)].key_name;
    PCTSTR text_2 = kmap_assoc_table[(unsigned int) (*pka2)].key_name;

    return(tstrcmp(text_1, text_2));
}

/******************************************************************************
*
* lookup into table given text using text index
*
******************************************************************************/

_Check_return_
static PC_KMAP_ASSOC
kmap_assoc_index_text_lookup(
    _In_z_      PCTSTR text)
{
    PC_KMAP_ASSOC_INDEX pki = (PC_KMAP_ASSOC_INDEX)
        bsearch(text, kmap_assoc_index_text, elemof(kmap_assoc_table), sizeof(kmap_assoc_index_text[0]), kmap_assoc_index_text_search);

    if(!pki)
        return(NULL);

    /* found entry in text index corresponding to lookup text; return kmap_assoc_table entry */
    return(kmap_assoc_table + *pki);
}

/******************************************************************************
*
* bsearch callback to compare text key with text field of a table entry
*
******************************************************************************/

PROC_BSEARCH_PROTO_Z(static, kmap_assoc_index_text_search, TCHARZ, KMAP_ASSOC_INDEX)
{
    BSEARCH_KEY_VAR_DECL(PCTSTR, key_text);
    BSEARCH_DATUM_VAR_DECL(PC_KMAP_ASSOC_INDEX, datum);
    PCTSTR datum_text = kmap_assoc_table[*datum].key_name;

    return(tstrcmp(key_text, datum_text));
}

/******************************************************************************
*
* build a pair of indexes for the association
* of KMAP_CODEs and textual representations
*
******************************************************************************/

_Check_return_
static STATUS
kmap_assoc_indexes_create(void)
{
    STATUS status;
    const U32 n = elemof32(kmap_assoc_table);

    /* first sort table by code */
#if WINDOWS
    check_sorted
#else
    qsort
#endif
        (kmap_assoc_table, n, sizeof(kmap_assoc_table[0]), kmap_assoc_code_compare);

    /* initialise and then sort text index */
    if(NULL == (kmap_assoc_index_text = al_ptr_alloc_elem(KMAP_ASSOC_INDEX, n, &status)))
        return(status);

    {
    U32 i;
    for(i = 0; i < n; ++i)
        kmap_assoc_index_text[i] = (KMAP_ASSOC_INDEX) i;
    } /*block*/

    qsort(kmap_assoc_index_text, n, sizeof(kmap_assoc_index_text[0]), kmap_assoc_index_text_compare);

    return(STATUS_OK);
}

/******************************************************************************
*
* return the key code corresponding to a language-independent 'file' key name
*
******************************************************************************/

extern KMAP_CODE
key_code_from_key_of_name(
    _In_z_      PCTSTR key_of_name)
{
    PC_KMAP_ASSOC pka;
    PCTSTR tstr = key_of_name;
    KMAP_CODE kmap_code = 0;

    if(NULL != (pka = kmap_assoc_index_text_lookup(key_of_name)))
        return(pka->kmap_code);

    if(*tstr++ == CH_CIRCUMFLEX_ACCENT)
    {
        kmap_code = KMAP_CODE_ADDED_ALT;

        if(*tstr++ == CH_EXCLAMATION_MARK)
            kmap_code |= KMAP_CODE_ADDED_SHIFT;
        else
            --tstr;

        assert((*tstr >= 'A') && (*tstr <= 'Z'));
        if((*tstr >= 'A') && (*tstr <= 'Z'))
            kmap_code = (KMAP_CODE) (kmap_code | (KMAP_CODE) *tstr);
        else
            kmap_code = 0;
    }

    assert(kmap_code);
    return(kmap_code);
}

extern PCTSTR
key_of_name_from_key_code(
    _In_        KMAP_CODE key_code)
{
    static TCHARZ nasty_static_namebuf[32];

    PC_KMAP_ASSOC pka;
    PTSTR tstr;

    if(!key_code)
        return(NULL);

    if(NULL != (pka = kmap_assoc_code_lookup(key_code)))
        return(pka->key_name);

    tstr = nasty_static_namebuf;

    assert(key_code & KMAP_CODE_ADDED_ALT);
    if(key_code & KMAP_CODE_ADDED_ALT)
    {
        key_code &= ~KMAP_CODE_ADDED_ALT;
        *tstr++ = CH_CIRCUMFLEX_ACCENT;
    }

    if(key_code & KMAP_CODE_ADDED_SHIFT)
    {
        key_code &= ~KMAP_CODE_ADDED_SHIFT;
        *tstr++ = CH_EXCLAMATION_MARK;
    }

    assert((key_code >= 'A') && (key_code <= 'Z'));
    *tstr++ = (TCHAR) key_code;
    *tstr   = CH_NULL;

    return(nasty_static_namebuf);
}

extern PCTSTR /*low-lifetime*/
key_ui_name_from_key_code(
    _In_        KMAP_CODE key_code,
    _InVal_     BOOL long_name)
{
    static TCHARZ nasty_static_namebuf[32];

    PC_KMAP_ASSOC pka;
    STATUS prefix_shift;
    PTSTR tstr;
    U32 len = 0;

    if(!key_code)
        return(NULL);

    prefix_shift = long_name ? MSG_FUNC_LONG_SHIFT : MSG_FUNC_MENU_SHIFT;

    if(NULL != (pka = kmap_assoc_code_lookup(key_code)))
    {
        STATUS prefix_index = long_name ? pka->long_prefix_index : pka->menu_prefix_index;

        if(prefix_index)
        {
            resource_lookup_tstr_buffer(nasty_static_namebuf, elemof32(nasty_static_namebuf), prefix_shift + ((S32) pka->long_prefix_index - PREFIX_S));
            len += tstrlen32(nasty_static_namebuf);
        }

        resource_lookup_tstr_buffer(nasty_static_namebuf + len, elemof32(nasty_static_namebuf) - len, pka->resource_id);

        return(nasty_static_namebuf);
    }

    assert(key_code & KMAP_CODE_ADDED_ALT);

    resource_lookup_tstr_buffer(tstr = nasty_static_namebuf, elemof32(nasty_static_namebuf),
                                ((key_code & KMAP_CODE_ADDED_SHIFT) ? PREFIX_CS : PREFIX_C) +
                                (prefix_shift - PREFIX_S));
    tstr += tstrlen32(tstr);

    key_code &= ~(KMAP_CODE_ADDED_ALT | KMAP_CODE_ADDED_SHIFT);

    assert((key_code >= 'A') && (key_code <= 'Z'));
    if((key_code >= 'A') && (key_code <= 'Z'))
    {
        *tstr++ = (U8) key_code;
        *tstr   = CH_NULL;
        return(nasty_static_namebuf);
    }

    assert0();
    return(NULL);
}

_Check_return_
static STATUS
sk_cmd_direct_msg_close1(
    _DocuRef_   P_DOCU p_docu)
{
    {
    LIST_ITEMNO key;
    P_LISTED_KEY_DEF p_key;

    for(p_key = collect_first(LISTED_KEY_DEF, &p_docu->defined_keys, &key);
        p_key;
        p_key = collect_next(LISTED_KEY_DEF, &p_docu->defined_keys, &key))
    {
        al_array_dispose(&p_key->definition_handle);
    }

    collect_delete(&p_docu->defined_keys);
    } /*block*/

    { /* look up the stacked contexts for this document */
    const DOCNO docno = docno_from_p_docu(p_docu);
    ARRAY_INDEX i;

    for(i = 0; i < array_elements(&stacked_execution_contexts); ++i)
    {
        P_STACKED_EXECUTION_CONTEXT p = array_ptr(&stacked_execution_contexts, STACKED_EXECUTION_CONTEXT, i);

        if(p->core.docno == docno)
            p->core.docno = DOCNO_NONE;
    }
    } /*block*/

    return(STATUS_OK);
}

_Check_return_
static STATUS
sk_cmd_direct_msg_exit1(void)
{
    arglist_dispose(&current_execution_context.core.last_cmd.arglist_handle);
    al_array_dispose(&stacked_execution_contexts);

    al_ptr_dispose(P_P_ANY_PEDANTIC(&kmap_assoc_index_text));

    return(STATUS_OK);
}

_Check_return_
static STATUS
sk_cmd_direct_msg_init1(
    _DocuRef_   P_DOCU p_docu)
{
    list_init(&p_docu->defined_keys);

    return(STATUS_OK);
}

_Check_return_
static STATUS
sk_cmd_direct_msg_startup(void)
{
    execution_context_startup();

    return(kmap_assoc_indexes_create());
}

/* replaces our old maeve handler */

_Check_return_
extern STATUS
sk_cmd_direct_msg_initclose(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_MSG_INITCLOSE p_msg_initclose)
{
    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__STARTUP:
        return(sk_cmd_direct_msg_startup());

    case T5_MSG_IC__EXIT1:
        return(sk_cmd_direct_msg_exit1());

    case T5_MSG_IC__INIT1:
        return(sk_cmd_direct_msg_init1(p_docu));

    case T5_MSG_IC__CLOSE1:
        return(sk_cmd_direct_msg_close1(p_docu));

    default:
        return(STATUS_OK);
    }
}

#if 0

/******************************************************************************
*
* output the defined keys list for a document
*
******************************************************************************/

_Check_return_
static STATUS
save_defined_keys(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_OF_OP_FORMAT p_of_op_format)
{
    LIST_ITEMNO key;
    P_LISTED_KEY_DEF p_key;
    const OBJECT_ID object_id = OBJECT_ID_SKEL;
    PC_CONSTRUCT_TABLE p_construct_table;
    ARGLIST_HANDLE arglist_handle;
    STATUS status;

    if(!collect_has_data(&p_docu->defined_keys))
        return(STATUS_OK);

    if(status_ok(status = arglist_prepare_with_construct(&arglist_handle, object_id, T5_CMD_DEFINE_KEY_RAW, &p_construct_table)))
    {
        const P_ARGLIST_ARG p_args = p_arglist_args(&p_t5_cmd->arglist_handle, 2);

        assert(p_args[0].type == (ARG_TYPE_USTR | ARG_MANDATORY));
        assert(p_args[1].type == ARG_TYPE_RAW);

        for(p_key = collect_first(LISTED_KEY_DEF, &p_docu->defined_keys, &key);
            p_key;
            p_key = collect_next(LISTED_KEY_DEF, &p_docu->defined_keys, &key))
        {
            if(!p_key->definition_handle)
                continue;

            p_args[0].val.tstr = key_of_name_from_key_code((KMAP_CODE) key);
            p_args[1].val.raw  = p_key->definition_handle; /* loan handle */

            status = ownform_save_arglist(arglist_handle, object_id, p_construct_table, p_of_op_format);

            p_args[1].val.raw    = 0; /* reclaim handle */

            status_break(status);
        }

        arglist_dispose(&arglist_handle);
    }

    return(status);
}

_Check_return_
static STATUS
sk_cmd_msg_save(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     P_MSG_SAVE p_msg_save)
{
    switch(p_msg_save->t5_message)
    {
    case T5_MSG_PRE_SAVE_2:
        return(save_defined_keys(p_docu, p_msg_save->p_of_op_format));

    default:
        return(STATUS_OK);
    }
}

PROC_EVENT_PROTO(extern, proc_event_sk_cmd_direct)
{
    switch(t5_message)
    {
#if 0
    case T5_MSG_SAVE:
        return(sk_cmd_msg_save(p_docu, (P_MSG_SAVE) p_data));
#endif

    default:
        return(STATUS_OK);
    }
}

#endif

/******************************************************************************
*
* set the given document as current
*
******************************************************************************/

T5_CMD_PROTO(extern, t5_cmd_current_document)
{
    /*DOCU_NAME docu_name;*/

    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);
    UNREFERENCED_PARAMETER_InRef_(p_t5_cmd);

    /*docu_name.leaf_name = p_args[0].val.tstr;*/
    /*docu_name.path_name = NULL;*/

    /* UNREFERENCED_PARAMETER(docu_name); <<< */

    return(STATUS_OK);
}

T5_CMD_PROTO(extern, t5_cmd_current_position)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 4);

    UNREFERENCED_PARAMETER_InVal_(t5_message);

    p_docu->cur.slr.col = p_args[0].val.col;
    p_docu->cur.slr.row = p_args[1].val.row;
    p_docu->cur.object_position.object_id = OBJECT_ID_NONE;

    if(arg_is_present(p_args, 2) && arg_is_present(p_args, 3))
    {
        p_docu->cur.object_position.data      = p_args[2].val.s32;
        p_docu->cur.object_position.more_data = -1;
        p_docu->cur.object_position.object_id = p_args[3].val.object_id;
    }

    caret_show_claim(p_docu, OBJECT_ID_CELLS, TRUE);

    /* SKS after 1.08b2 15jul94 added because otherwise we don't get formula reflected for 0,0 cell
     * (or much else that depends on this initial encoding!)
    */
    status_assert(maeve_event(p_docu, T5_MSG_DOCU_COLROW, &p_docu->old.slr));

    return(STATUS_OK);
}

/******************************************************************************
*
* define the command sequence associated with a key
*
* two args:
*   0: key ident (ARG_TYPE_USTR)
*   1: key value (ARG_TYPE_RAW or ARG_TYPE_USTR)
*
******************************************************************************/

T5_CMD_PROTO(extern, t5_cmd_define_key)
{
    const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 2);
    PCTSTR name = p_args[0].val.tstr;
    KMAP_CODE code = key_code_from_key_of_name(name);
    LIST_ITEMNO key = (LIST_ITEMNO) code;
    P_LISTED_KEY_DEF p_key;
    STATUS status = STATUS_OK;

    if(0 == code)
    {   /* key ident not found in table */
        assert(0 != code);
        return(STATUS_OK);
    }

    p_key = collect_goto_item(LISTED_KEY_DEF, &p_docu->defined_keys, key);

    if(NULL != p_key)
    {
        /* see if it is in use a ce moment */
        if(execution_contexts_handle_in_use(p_key->definition_handle))
            return(create_error(ERR_KEY_DEF_IN_USE));

        al_array_dispose(&p_key->definition_handle);
    }
    else
    {
        if(NULL == (p_key = collect_add_entry_elem(LISTED_KEY_DEF, &p_docu->defined_keys, P_DATA_NONE, key, &status)))
            return(status);

        zero_struct_ptr(p_key);
    }

    if(t5_message == T5_CMD_DEFINE_KEY_RAW)
    {
        ARRAY_HANDLE h_commands = p_args[1].val.raw;
        status = al_array_duplicate(&p_key->definition_handle, &h_commands);
    }
    else
    {
        PC_USTR ustr_commands = p_args[1].val.ustr;
        status = al_ustr_set(&p_key->definition_handle, ustr_commands);
    }

    if(status_fail(status))
        collect_subtract_entry(&p_docu->defined_keys, key);

    return(status);
}

/******************************************************************************
*
* set the next-command-is-interactive flag
*
******************************************************************************/

_Check_return_
extern STATUS
t5_cmd_set_interactive(void)
{
    go_interactive = 1;

    return(STATUS_OK);
}

#if 0

/******************************************************************************
*
* remove any associated command sequence from a given key
*
******************************************************************************/

T5_CMD_PROTO(extern, t5_cmd_undefine_key)
{
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    if(p_t5_cmd->interactive)
    {
        /* put up a dialog box with a list box of what keys are defined and get the punter to choose one */
        assert0();
    }
    else
    {
        const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 1);
        PCTSTR name = p_args[0].val.tstr;
        KMAP_CODE code = key_code_from_key_of_name(name);

        assert(code);
        if(!code)
            /* key ident not found in table */
            return(STATUS_OK);

        command_array_handle_dispose_from_key_code(p_docu, code);
    }

    return(STATUS_OK);
}

#endif

/* end of sk_cmd.c */
