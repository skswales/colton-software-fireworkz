/* ob_rec.c */

/* Copyright (C) 1994-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Record data object module for Fireworkz */

/* PMF 1994 */

#include "common/gflags.h"

#include "ob_rec/ob_rec.h"

#if RISCOS
#include "ob_skel/xp_skelr.h"
#endif

#include "ob_toolb/xp_toolb.h"

/*
exported data
*/

extern STYLE_SELECTOR style_selector_ob_rec;

extern DRAG_STATE drag_state;

#if RISCOS
#define MSG_WEAK &rb_rec_msg_weak
extern PC_U8 rb_rec_msg_weak;
extern P_ANY rb_rec_spr_22_weak;
extern P_ANY rb_rec_spr_24_weak;
static BOUND_RESOURCES BOUND_RESOURCES_OBJECT_ID_REC = { NULL, &rb_rec_spr_22_weak, &rb_rec_spr_24_weak};
#define P_BOUND_RESOURCES_OBJECT_ID_REC &BOUND_RESOURCES_OBJECT_ID_REC
#else
#define P_BOUND_RESOURCES_OBJECT_ID_REC NULL
#endif

/* ----------------------------------------------------------------------- */

STYLE_SELECTOR style_selector_ob_rec;

DRAG_STATE drag_state;

static const ARG_TYPE
args_cmd_goto[] =
{
    ARG_TYPE_S32 | ARG_MANDATORY,
    ARG_TYPE_NONE
};

static const ARG_TYPE
args_cmd_table[] =
{
    ARG_TYPE_S32 | ARG_MANDATORY,       /* unique ID */
    ARG_TYPE_TSTR | ARG_MANDATORY,      /* filename  */
    ARG_TYPE_S32 | ARG_MANDATORY,       /* card type */
    ARG_TYPE_COL | ARG_MANDATORY,       /* tl.x */
    ARG_TYPE_ROW | ARG_MANDATORY,       /* tl.y */
    ARG_TYPE_S32 | ARG_MANDATORY,       /* number of columns */
    ARG_TYPE_S32 | ARG_MANDATORY,       /* number of rows occupied */
    ARG_TYPE_S32,                       /* new adaptive flag */
    ARG_TYPE_NONE
};

static const ARG_TYPE
args_cmd_frame[] =
{
    ARG_TYPE_S32 | ARG_MANDATORY,       /* unique ID of parent */
    ARG_TYPE_S32 | ARG_MANDATORY,       /* ordinal     */
    ARG_TYPE_S32 | ARG_MANDATORY,       /* field id    */
    ARG_TYPE_USTR | ARG_MANDATORY,      /* field name  */
    ARG_TYPE_S32 | ARG_MANDATORY,       /* x           */
    ARG_TYPE_S32 | ARG_MANDATORY,       /* y           */
    ARG_TYPE_S32 | ARG_MANDATORY,       /* w           */
    ARG_TYPE_S32 | ARG_MANDATORY,       /* h           */
    ARG_TYPE_S32 | ARG_MANDATORY,       /* z           */
    ARG_TYPE_NONE
};

static const ARG_TYPE
args_cmd_title[] =
{
    ARG_TYPE_S32 | ARG_MANDATORY,       /* unique ID of parent */
    ARG_TYPE_S32 | ARG_MANDATORY,       /* ordinal     */
    ARG_TYPE_S32 | ARG_MANDATORY,       /* field id    */
    ARG_TYPE_USTR | ARG_MANDATORY_OR_BLANK, /* field title */
    ARG_TYPE_S32 | ARG_MANDATORY,       /* x           */
    ARG_TYPE_S32 | ARG_MANDATORY,       /* y           */
    ARG_TYPE_S32 | ARG_MANDATORY,       /* w           */
    ARG_TYPE_S32 | ARG_MANDATORY,       /* h           */
    ARG_TYPE_BOOL | ARG_OPTIONAL,       /* title_show  */
    ARG_TYPE_NONE
};

static const ARG_TYPE
args_cmd_query[] =
{
    ARG_TYPE_USTR | ARG_MANDATORY,      /* textual name */
    ARG_TYPE_S32 | ARG_MANDATORY,       /* query  id */
    ARG_TYPE_S32 | ARG_MANDATORY,       /* parent id */
    ARG_TYPE_S32 | ARG_MANDATORY,       /* type */
    ARG_TYPE_S32 | ARG_MANDATORY,       /* exclude */
    ARG_TYPE_S32 | ARG_MANDATORY,       /* andor */
    ARG_TYPE_NONE
};

static const ARG_TYPE
args_cmd_pattern[] =
{
    ARG_TYPE_USTR | ARG_MANDATORY,      /* text */
    ARG_TYPE_S32 | ARG_MANDATORY,       /* field  id */
    ARG_TYPE_S32 | ARG_MANDATORY,       /* prefix length */
    ARG_TYPE_S32 | ARG_MANDATORY,       /* suffix length */
    ARG_TYPE_S32 | ARG_MANDATORY,       /* operator */
    ARG_TYPE_NONE
};

/*
construct table
*/

static CONSTRUCT_TABLE
object_construct_table[] =
{                                                                                                   /*   fi ti mi ur up xi md mf nn cp sm ba fo */

    { "DBChartMenu",            NULL,                       T5_CMD_ACTIVATE_MENU_CHART,                 { 0, 0, 0, 0, 0, 0, 0, 1, 0 }},

    { "DBStyleExtra",           NULL,                       T5_CMD_STYLE_BUTTON }, /* needed to redirect back to skel */

    { "DBStyle",                NULL,                       T5_CMD_STYLE_RECORDZ },
    { "DBSearch",               NULL,                       T5_CMD_SEARCH_RECORDZ },
    { "DBView",                 NULL,                       T5_CMD_VIEW_RECORDZ },
    { "DBCreate",               NULL,                       T5_CMD_CREATE_RECORDZ },
    { "DBClose",                NULL,                       T5_CMD_CLOSE_RECORDZ },
    { "DBAppend",               NULL,                       T5_CMD_APPEND_RECORDZ },
    { "DBSort",                 NULL,                       T5_CMD_SORT_RECORDZ },
    { "DBProp",                 NULL,                       T5_CMD_PROP_RECORDZ },
    { "DBLayout",               NULL,                       T5_CMD_LAYOUT_RECORDZ },
    { "DBAdd",                  NULL,                       T5_CMD_ADD_RECORDZ },
    { "DBCopy",                 NULL,                       T5_CMD_COPY_RECORDZ },
    { "DBDelete",               NULL,                       T5_CMD_DELETE_RECORDZ },
    { "DBExport",               NULL,                       T5_CMD_EXPORT_RECORDZ },

    { "DBFirst",                NULL,                       T5_CMD_RECORDZ_ERSTE },
    { "DBRewind",               NULL,                       T5_CMD_RECORDZ_REWIND },
    { "DBPrevious",             NULL,                       T5_CMD_RECORDZ_PREV },
    { "DBNext",                 NULL,                       T5_CMD_RECORDZ_NEXT },
    { "DBFastForward",          NULL,                       T5_CMD_RECORDZ_FFWD },
    { "DBLast",                 NULL,                       T5_CMD_RECORDZ_LETSTE },

    { "DBGoto",                 args_cmd_goto,              T5_CMD_RECORDZ_GOTO_RECORD },
    { "DBGotoIntro",            NULL,                       T5_CMD_RECORDZ_GOTO_RECORD_INTRO },

    { "DBTable",                args_cmd_table,             T5_CMD_RECORDZ_IO_TABLE  },
    { "DBFrame",                args_cmd_frame,             T5_CMD_RECORDZ_IO_FRAME  },
    { "DBTitle",                args_cmd_title,             T5_CMD_RECORDZ_IO_TITLE  },
    { "DBQuery",                args_cmd_query,             T5_CMD_RECORDZ_IO_QUERY  },
    { "DBPattern",              args_cmd_pattern,           T5_CMD_RECORDZ_IO_PATTERN },

    { NULL,                     NULL,                       T5_EVENT_NONE } /* end of table */
};

static const T5_TOOLBAR_TOOL_DESC
object_tools[] =
{
    { "DB_TOP",
        OBJECT_ID_REC, T5_CMD_RECORDZ_ERSTE,
        OBJECT_ID_REC, REC_ID_BM_DB_TOP, 0,
        { T5_TOOLBAR_TOOL_TYPE_COMMAND, 0 },
        UI_TEXT_INIT_RESID(REC_MSG_STATUS_DB_TOP), T5_CMD_RECORDZ_LETSTE },

    { "DB_REWIND",
        OBJECT_ID_REC, T5_CMD_RECORDZ_REWIND,
        OBJECT_ID_REC, REC_ID_BM_DB_REWIND, 0,
        { T5_TOOLBAR_TOOL_TYPE_COMMAND, 0 },
        UI_TEXT_INIT_RESID(REC_MSG_STATUS_DB_REWIND), T5_CMD_RECORDZ_FFWD },

    { "DB_PREV",
        OBJECT_ID_REC, T5_CMD_RECORDZ_PREV,
        OBJECT_ID_REC, REC_ID_BM_DB_PREV, 0,
        { T5_TOOLBAR_TOOL_TYPE_COMMAND,  0/*im*/, 0/*sv*/, 0/*th*/, 1/*ar*/ },
        UI_TEXT_INIT_RESID(REC_MSG_STATUS_DB_PREV), T5_CMD_RECORDZ_NEXT },

    { "DB_GOTO",
        OBJECT_ID_REC, T5_CMD_RECORDZ_GOTO_RECORD_INTRO,
        OBJECT_ID_REC, REC_ID_BM_DB_GOTO, 0,
        { T5_TOOLBAR_TOOL_TYPE_COMMAND, 0 },
        UI_TEXT_INIT_RESID(REC_MSG_STATUS_DB_GOTO) },

    { "DB_NEXT",
        OBJECT_ID_REC, T5_CMD_RECORDZ_NEXT,
        OBJECT_ID_REC, REC_ID_BM_DB_NEXT, 0,
        { T5_TOOLBAR_TOOL_TYPE_COMMAND, 0/*im*/, 0/*sv*/, 0/*th*/, 1/*ar*/ },
        UI_TEXT_INIT_RESID(REC_MSG_STATUS_DB_NEXT), T5_CMD_RECORDZ_PREV },

    { "DB_FAST",
        OBJECT_ID_REC, T5_CMD_RECORDZ_FFWD,
        OBJECT_ID_REC, REC_ID_BM_DB_FAST, 0,
        { T5_TOOLBAR_TOOL_TYPE_COMMAND, 0 },
        UI_TEXT_INIT_RESID(REC_MSG_STATUS_DB_FAST), T5_CMD_RECORDZ_REWIND },

    { "DB_BOTTOM",
        OBJECT_ID_REC, T5_CMD_RECORDZ_LETSTE,
        OBJECT_ID_REC, REC_ID_BM_DB_BOTTOM, 0,
        { T5_TOOLBAR_TOOL_TYPE_COMMAND, 0 },
        UI_TEXT_INIT_RESID(REC_MSG_STATUS_DB_BOTTOM), T5_CMD_RECORDZ_ERSTE },

    { "DB_ADD",
        OBJECT_ID_REC, T5_CMD_ADD_RECORDZ,
        OBJECT_ID_REC, REC_ID_BM_DB_ADD, 0,
        { T5_TOOLBAR_TOOL_TYPE_COMMAND, 0 },
        UI_TEXT_INIT_RESID(REC_MSG_STATUS_DB_ADD) },

    { "DB_DEL",
        OBJECT_ID_REC, T5_CMD_DELETE_RECORDZ,
        OBJECT_ID_REC, REC_ID_BM_DB_DEL, 0,
        { T5_TOOLBAR_TOOL_TYPE_COMMAND, 0 },
        UI_TEXT_INIT_RESID(REC_MSG_STATUS_DB_DEL) },

    { "DB_COPY",
        OBJECT_ID_REC, T5_CMD_COPY_RECORDZ,
        OBJECT_ID_REC, REC_ID_BM_DB_COPY, 0,
        { T5_TOOLBAR_TOOL_TYPE_COMMAND, 0 },
        UI_TEXT_INIT_RESID(REC_MSG_STATUS_DB_COPY) },

    { "DB_LAYOUT",
        OBJECT_ID_REC, T5_CMD_LAYOUT_RECORDZ,
        OBJECT_ID_REC, REC_ID_BM_DB_LAYOUT, 0,
        { T5_TOOLBAR_TOOL_TYPE_COMMAND, 0 },
        UI_TEXT_INIT_RESID(REC_MSG_STATUS_DB_LAYOUT) },

    { "DB_PROP",
        OBJECT_ID_REC, T5_CMD_PROP_RECORDZ,
        OBJECT_ID_REC, REC_ID_BM_DB_PROP, 0,
        { T5_TOOLBAR_TOOL_TYPE_COMMAND, 0 },
        UI_TEXT_INIT_RESID(REC_MSG_STATUS_DB_PROP) },

    { "DB_SORT",
        OBJECT_ID_REC, T5_CMD_SORT_RECORDZ,
        OBJECT_ID_SKEL, SKEL_ID_BM_SORT, 0,
        { T5_TOOLBAR_TOOL_TYPE_COMMAND, 0 },
        UI_TEXT_INIT_RESID(MSG_STATUS_SORT) },

    { "DB_SEARCH",
        OBJECT_ID_REC, T5_CMD_SEARCH_RECORDZ,
        OBJECT_ID_SKEL, SKEL_ID_BM_SEARCH, 0,
        { T5_TOOLBAR_TOOL_TYPE_COMMAND, 0 },
        UI_TEXT_INIT_RESID(REC_MSG_STATUS_SEARCH_EXTEND), T5_CMD_VIEW_RECORDZ },

    { "DB_STYLE",
        OBJECT_ID_REC, T5_CMD_STYLE_RECORDZ,
        OBJECT_ID_SKEL, SKEL_ID_BM_STYLE, 0,
        { T5_TOOLBAR_TOOL_TYPE_COMMAND, 0 },
        UI_TEXT_INIT_RESID(REC_MSG_STATUS_STYLE_EXTEND), T5_CMD_STYLE_BUTTON },

    { "DB_CHART",
        OBJECT_ID_REC, T5_CMD_ACTIVATE_MENU_CHART,
        OBJECT_ID_REC, REC_ID_BM_DB_CHART, 0,
        { T5_TOOLBAR_TOOL_TYPE_COMMAND, 1/*im*/ },
        UI_TEXT_INIT_RESID(REC_MSG_STATUS_DB_CHART) }
};

/******************************************************************************
*
* calculate x,y offset
*
* NB identical to ss_offset_from_style
*
******************************************************************************/

static void
rec_offset_from_style(
    _OutRef_    P_PIXIT_POINT p_offset,
    _InRef_     PC_STYLE p_style,
    _InRef_     PC_PIXIT p_text_width,
    _InRef_     PC_PIXIT p_text_height,
    _InRef_     PC_SKEL_RECT p_skel_rect_object)
{
    p_offset->x = p_style->para_style.margin_left;
    p_offset->y = p_style->para_style.para_start;

    switch(p_style->para_style.justify)
    {
    default:
        break;

    case SF_JUSTIFY_CENTRE:
    case SF_JUSTIFY_RIGHT:
        {
        PIXIT format_width = p_style->col_style.width
                           - p_style->para_style.margin_right
                           - p_style->para_style.margin_left
                           - *p_text_width;

        if(SF_JUSTIFY_CENTRE == p_style->para_style.justify)
            format_width /= 2;

        if(format_width > 0)
            p_offset->x += format_width;

        break;
        }
    }

    switch(p_style->para_style.justify_v)
    {
    default:
        break;

    case SF_JUSTIFY_V_CENTRE:
    case SF_JUSTIFY_V_BOTTOM:
        {
        PIXIT vertical_space = p_skel_rect_object->br.pixit_point.y
                             - p_skel_rect_object->tl.pixit_point.y
                             - p_style->para_style.para_start
                             - p_style->para_style.para_end
                             - *p_text_height;

        if(SF_JUSTIFY_V_CENTRE == p_style->para_style.justify_v)
            vertical_space /= 2;

        if(vertical_space > 0)
            p_offset->y += vertical_space;

        break;
        }
    }
}

_Check_return_
static STATUS
rec_read_text_draft(
    _DocuRef_   P_DOCU p_docu,
    P_OBJECT_READ_TEXT_DRAFT p_object_read_text_draft)
{
    DATA_REF data_ref;
    P_REC_PROJECTOR p_rec_projector;
    FONTY_HANDLE fonty_handle;
    PIXIT text_height, text_width, base_line;
    STYLE style;
    STATUS status = STATUS_OK;

    assert(p_object_read_text_draft->object_data.data_ref.data_space == DATA_SLOT);

    if(NULL != (p_rec_projector = rec_data_ref_from_slr(p_docu, &p_object_read_text_draft->object_data.data_ref.arg.slr, &data_ref)))
    {
        P_FIELDDEF p_fielddef = p_fielddef_from_field_id(&p_rec_projector->opendb.table.h_fielddefs, (FIELD_ID) data_ref.arg.db_field.field_id);
        QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 64);
        quick_ublock_with_buffer_setup(quick_ublock);

        rec_style_for_data_ref(p_docu, &p_object_read_text_draft->object_data.data_ref, &style, &style_selector_para_text);

        myassert1x(data_ref.data_space == DATA_DB_FIELD, TEXT("Not a DATA_DB_FIELD ") S32_TFMT, data_ref.data_space);

        if(P_DATA_NONE == p_fielddef)
            return(STATUS_FAIL);

        if(!p_fielddef->hidden)
        {
            REC_FRAME rec_frame;

            status_return(status = get_frame_by_field_id(p_rec_projector, p_fielddef->id, &rec_frame));

            switch(p_fielddef->type)
            {
            case FIELD_FILE:
            case FIELD_PICTURE:
                assert0();
                status = STATUS_FAIL;
                break;

            default:
                {
                if(status_ok(status = rec_object_convert_to_output_text(&quick_ublock,
                                                                        &text_width,
                                                                        &text_height,
                                                                        &base_line,
                                                                        &fonty_handle,
                                                                        &style,
                                                                        p_rec_projector, &data_ref)))
                {
                    if(0 != quick_ublock_bytes(&quick_ublock))
                    {
                        U8 effects[PLAIN_EFFECT_COUNT];
                        PIXIT_POINT offset;
                        const PC_FONT_CONTEXT p_font_context = p_font_context_from_fonty_handle(fonty_handle);
                        QUICK_UBLOCK_WITH_BUFFER(quick_ublock_plain, 100);
                        quick_ublock_with_buffer_setup(quick_ublock_plain);

                        rec_offset_from_style(&offset, &style, &text_width, &text_height, &p_object_read_text_draft->skel_rect_object);

                        if(status_ok(status))
                            status = plain_text_spaces_out(&quick_ublock_plain,
                                                           chars_from_pixit(offset.x, p_font_context->space_width));

                        zero_array(effects);

                        if(status_ok(status))
                            status = plain_text_effects_update(&quick_ublock_plain, effects, &style.font_spec);

                        if(status_ok(status))
                            status = quick_ublock_uchars_add(&quick_ublock_plain, quick_ublock_uchars(&quick_ublock), quick_ublock_bytes(&quick_ublock) - 1);

                        if(status_ok(status))
                            status = plain_text_effects_off(&quick_ublock_plain, effects);

                        if(status_ok(status))
                            status = plain_text_segment_out(&p_object_read_text_draft->h_plain_text, &quick_ublock_plain);

                        quick_ublock_dispose(&quick_ublock_plain);
                    }
                }

                break;
                }
            }
        }

        quick_ublock_dispose(&quick_ublock);

        /* chuck it all away if we failed */
        if(status_fail(status))
            plain_text_dispose(&p_object_read_text_draft->h_plain_text);
    }

    return(status);
}

_Check_return_
static STATUS
rec_object_data_read(
    _DocuRef_   P_DOCU p_docu,
    P_OBJECT_DATA_READ p_object_data_read)
{
    DATA_REF data_ref;
    P_REC_PROJECTOR p_rec_projector;
    REC_RESOURCE rec_resource;
    STATUS status;

    if(NULL == (p_rec_projector = rec_data_ref_from_slr(p_docu, &p_object_data_read->object_data.data_ref.arg.slr, &data_ref)))
        return(create_error(REC_ERR_NO_DATABASE));

    if(PROJECTOR_TYPE_CARD == p_rec_projector->projector_type)
        return(STATUS_FAIL);

    status = rec_read_resource(&rec_resource, p_rec_projector, &data_ref, TRUE);

    p_object_data_read->ev_data = rec_resource.ev_data;

    return(status);
}

_Check_return_
static STATUS
rec_object_how_wide(
    _DocuRef_   P_DOCU p_docu,
    P_OBJECT_HOW_WIDE p_object_how_wide)
{
    DATA_REF data_ref;
    P_REC_PROJECTOR p_rec_projector;

    if(NULL == (p_rec_projector = rec_data_ref_from_slr(p_docu, &p_object_how_wide->object_data.data_ref.arg.slr, &data_ref)))
        return(create_error(REC_ERR_NO_DATABASE));

    myassert1x(data_ref.data_space == DATA_DB_FIELD, TEXT("Not a DATA_DB_FIELD ") S32_TFMT, data_ref.data_space);

    if(PROJECTOR_TYPE_CARD == p_rec_projector->projector_type)
    {
        PIXIT_POINT pixit_point;
        assert(p_object_how_wide->object_data.data_ref.data_space == DATA_SLOT);
        rec_object_pixit_size(p_docu, &pixit_point, &p_object_how_wide->object_data.data_ref.arg.slr, NULL);
        p_object_how_wide->width = pixit_point.x;
        return(STATUS_OK);
    }

    {
    P_FIELDDEF p_fielddef = p_fielddef_from_field_id(&p_rec_projector->opendb.table.h_fielddefs, (FIELD_ID) data_ref.arg.db_field.field_id);
    REC_FRAME rec_frame;
    STATUS status = STATUS_OK;

    if((P_DATA_NONE == p_fielddef) || p_fielddef->hidden)
        return(STATUS_FAIL);

    status_assert(get_frame_by_field_id(p_rec_projector, p_fielddef->id, &rec_frame));

    switch(p_fielddef->type)
    {
    case FIELD_FILE:
    case FIELD_PICTURE:
        p_object_how_wide->width = rec_frame.pixit_rect_field.br.x - rec_frame.pixit_rect_field.tl.x;
        break;

    default:
        {
        STYLE style;
        FONTY_HANDLE fonty_handle;
        PIXIT text_height, text_width, base_line;
        QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 64);
        quick_ublock_with_buffer_setup(quick_ublock);

        rec_style_for_data_ref(p_docu, &data_ref, &style, &style_selector_ob_rec);

        p_object_how_wide->width = 0;
        status = rec_object_convert_to_output_text(&quick_ublock, &text_width, &text_height, &base_line, &fonty_handle, &style, p_rec_projector, &data_ref);
        if(text_width)
            p_object_how_wide->width = text_width
                                     + style.para_style.margin_left
                                     + style.para_style.margin_right;
        quick_ublock_dispose(&quick_ublock);
        break;
        }
    }

    return(status);
    } /*block*/
}

_Check_return_
extern STATUS
dplib_ustr_inline_convert(
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock /*appended*/,
    _In_reads_(uchars_n) PC_USTR_INLINE ustr_inline,
    _InVal_     U32 uchars_n)
{
    STATUS status = STATUS_OK;
    U32 offset = 0;

    while((offset < uchars_n) && status_ok(status))
    {
        if(is_inline_off(ustr_inline, offset))
        {
            switch(inline_code_off(ustr_inline, offset))
            {
            case IL_RETURN:
                status = quick_ublock_a7char_add(p_quick_ublock, '\n'); /* inlines returns get converted to newlines */
                break;

            default:
                status = quick_ublock_a7char_add(p_quick_ublock, CH_SPACE); /* all other inlines simply get converted to spaces */
                break;
            }

            offset += inline_bytecount_off(ustr_inline, offset);
        }
        else
        {
            U32 bytes_of_char;
            UCS4 ucs4 = ustr_char_decode_off(ustr_inline, offset, /*ref*/bytes_of_char);

            assert(CH_NULL != ucs4);
            status = quick_ublock_ucs4_add(p_quick_ublock, ucs4);

            offset += bytes_of_char;
        }
    }

    return(status);
}

/* Revert to the whole file */

_Check_return_
extern STATUS
rec_revert_whole(
    P_REC_PROJECTOR p_rec_projector)
{
    drop_search_suggestions(&p_rec_projector->opendb.search); /* Drop the suggestions now, we are about to make some new ones */

    p_rec_projector->opendb.search.query_id        = RECORDZ_WHOLE_FILE;
    p_rec_projector->opendb.search.suggest_type    = SEARCH_TYPE_FILE;
    p_rec_projector->opendb.search.suggest_andor   = 2; /* Eh ? */
    p_rec_projector->opendb.search.suggest_exclude = FALSE;

    close_cursor(&p_rec_projector->opendb.table);

    return(ensure_cursor_whole(&p_rec_projector->opendb.table));
}

/*
Routine to change the "start_offset" scroll position of a table-database

You can change possition either relative to the current position (abs = FALSE, rn = amount to move by)

or move to a specific record number (abs = true, rn = record to move to)

The special value pair of abs, RECORDZ_GOTO_LAST means move such that the last record is in the last position in the table.

The special value pair of abs, RECORDZ_PAGE_DOWN means move onwards by the height of the table

The special value pair of abs, RECORDZ_PAGE_UP means move backwards by the height of the table

Other negative values may be allocated special meanings

NB The record number maps to a row offset in SHEET mode but in CARD mode you must take account of the card width

we may need to use record_view to goto the requested record
*/

_Check_return_
extern STATUS
rec_goto_record(
    _DocuRef_   P_DOCU p_docu,
    _In_        BOOL abs,
    _In_        S32 rn)
{
    STATUS status = STATUS_OK;
    DATA_REF data_ref;
    P_REC_PROJECTOR p_rec_projector;
    S32 cols;
    S32 rows;
    S32 records_per_row = 1;

    if(NULL == (p_rec_projector = rec_data_ref_from_slr(p_docu, &p_docu->cur.slr, &data_ref)))
        return(create_error(REC_ERR_NO_DATABASE));

    if(!p_rec_projector->opendb.dbok)
        return(create_error(REC_ERR_DB_NOT_OK));

    cur_change_before(p_docu);

    cols = p_rec_projector->rec_docu_area.br.slr.col -  p_rec_projector->rec_docu_area.tl.slr.col;
    rows = p_rec_projector->rec_docu_area.br.slr.row -  p_rec_projector->rec_docu_area.tl.slr.row;

    if(p_rec_projector->projector_type == PROJECTOR_TYPE_CARD)
        records_per_row = cols;

    if(p_rec_projector->adaptive_rows)
    {
        /* whole database exposed - SKS 08jan95 does cell moves */
        if(abs)
        {
            BOOL do_it = TRUE;
            SKELCMD_GOTO skelcmd_goto;
            COL col_s, col_e;
            ROW row_s, row_e;

            skelcmd_goto.slr.col = p_docu->cur.slr.col;

            limits_from_docu_area(p_docu, &col_s, &col_e, &row_s, &row_e, &p_rec_projector->rec_docu_area);

            if(rn == 0)
            {
                if(p_rec_projector->projector_type == PROJECTOR_TYPE_CARD)
                   skelcmd_goto.slr.col = col_s;
                skelcmd_goto.slr.row = row_s;
            }
            else if(rn > 0)
            {
                if(p_rec_projector->projector_type == PROJECTOR_TYPE_CARD)
                {
                    skelcmd_goto.slr.row = row_s + (rn / records_per_row);
                    skelcmd_goto.slr.col = col_s + (COL) (rn - records_per_row * (skelcmd_goto.slr.row - row_s));
                }
                else
                    skelcmd_goto.slr.row = row_s + rn;
            }
            else
                switch(rn)
                {
                case RECORDZ_GOTO_LAST:
                    if(p_rec_projector->projector_type == PROJECTOR_TYPE_CARD)
                        skelcmd_goto.slr.col = col_e - 1;
                    skelcmd_goto.slr.row = row_e - 1;
                    break;

                case RECORDZ_PAGE_UP:
                    do_it = FALSE;
                    status = object_skel(p_docu, T5_CMD_PAGE_UP, P_DATA_NONE);
                    break;

                case RECORDZ_PAGE_DOWN:
                    do_it = FALSE;
                    status = object_skel(p_docu, T5_CMD_PAGE_DOWN, P_DATA_NONE);
                    break;

                default:
                    do_it = FALSE;
                    break;
                }

            if(do_it)
            {
                skelcmd_goto.keep_selection = FALSE;
                skel_point_from_slr_tl(p_docu, &skelcmd_goto.skel_point, &skelcmd_goto.slr);
                status = object_skel(p_docu, T5_MSG_GOTO_SLR, &skelcmd_goto);
            }
        }
        else
        {
            if(rn > 0)
                status = object_skel(p_docu, T5_MSG_MOVE_DOWN_CELL, P_DATA_NONE);
            else if(rn < 0)
                status = object_skel(p_docu, T5_MSG_MOVE_UP_CELL, P_DATA_NONE);
        }
    }
    else
    {
        /* OK we know how many records per row there are, so we can calculate the required offset */
        S32 scroll_offset_max = (records_per_row - 1 + p_rec_projector->opendb.recordspec.ncards) / records_per_row - rows;
        S32 scroll_offset;

        if(abs)
            /* let's deal with the special cases of page up/page down by converting them to relative moves by rows */
            switch(rn)
            {
            case RECORDZ_PAGE_UP:
                abs = FALSE;
                rn  = -rows;
                break;

            case RECORDZ_PAGE_DOWN:
                abs = FALSE;
                rn  = +rows;
                break;

            default:
                break;
            }

        /* We may need to adjust (and range check) the scroll offset */
        if(abs)
        {
            if(rn >= 0)
                 scroll_offset = rn / records_per_row;
            else
            {
                if(rn == RECORDZ_GOTO_LAST)
                    scroll_offset = scroll_offset_max;
                else
                {
                    assert0();
                    scroll_offset = p_rec_projector->start_offset;
                }
            }
        }
        else
            scroll_offset = p_rec_projector->start_offset + rn;

        if( scroll_offset > scroll_offset_max)
            scroll_offset = scroll_offset_max;

        if( scroll_offset < 0)
            scroll_offset = 0;

        if(p_rec_projector->start_offset != scroll_offset)
        {
            p_rec_projector->start_offset = scroll_offset;

            if(p_rec_projector->projector_type == PROJECTOR_TYPE_CARD)
                rec_update_card_contents(p_rec_projector);
            else
                rec_update_projector(p_rec_projector);
        }
    }

    cur_change_after(p_docu);

    return(status);
}

_Check_return_
extern STATUS
rec_add_record(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     BOOL add_blank)
{
    DATA_REF data_ref;
    P_REC_PROJECTOR p_rec_projector;
    STATUS status;

    if(NULL == (p_rec_projector = rec_data_ref_from_slr(p_docu, &p_docu->cur.slr, &data_ref)))
        return(create_error(REC_ERR_NO_DATABASE));

    if(!p_rec_projector->opendb.dbok)
        return(create_error(REC_ERR_DB_NOT_OK));

    status_return(table_check_access(&p_rec_projector->opendb.table, ACCESS_FUNCTION_ADD));

    cur_change_before(p_docu);

    myassert1x(data_ref.data_space == DATA_DB_FIELD, TEXT("Not DATA_DB_FIELD ") S32_TFMT, data_ref.data_space);

    status = record_view_recnum(&p_rec_projector->opendb, data_ref.arg.db_field.record);

    if(status_ok(status))
        status = record_add_commence(&p_rec_projector->opendb, TRUE);

    if(status_ok(status))
    {
        S32 new_recno  = -1;
        S32 new_ncards = -1;

        if(status_ok(status = record_edit_confirm(&p_rec_projector->opendb, add_blank, TRUE /*adding*/)))
        {
            /* Try and follow the little darling */
            if(status_ok(record_current(&p_rec_projector->opendb, &new_recno, &new_ncards)))
                p_rec_projector->opendb.recordspec.ncards = new_ncards;
            else
                new_recno = -1; /* Lost track of it - this is likely because the new record is not in the subset */
        }
        else
            status = record_edit_cancel(&p_rec_projector->opendb, TRUE);

        rec_update_projector_adjust_goto(p_rec_projector, new_recno);
    }

    cur_change_after(p_docu);

    return(status);
}

_Check_return_
extern STATUS
rec_delete_record(
    _DocuRef_   P_DOCU p_docu)
{
    STATUS status;
    DATA_REF data_ref;
    P_REC_PROJECTOR p_rec_projector;
    S32 old_recno;

    if(NULL == (p_rec_projector = rec_data_ref_from_slr(p_docu, &p_docu->cur.slr, &data_ref)))
        return(create_error(REC_ERR_NO_DATABASE));

    status_return(table_check_access(&p_rec_projector->opendb.table, ACCESS_FUNCTION_DELETE));

#if RISCOS
    {
    int errflags_out;
    _kernel_oserror e;
    e.errnum = 0;
    resource_lookup_tstr_buffer(e.errmess, elemof32(e.errmess), REC_MSG_DELETE_QUESTION);
    consume(_kernel_oserror *, wimp_reporterror_rf(&e, Wimp_ReportError_OK | Wimp_ReportError_Cancel, &errflags_out, NULL, 4));
    if(errflags_out != Wimp_ReportError_OK)
        return(STATUS_OK);
    } /*block*/
#endif

    cur_change_before(p_docu);

    old_recno = p_rec_projector->opendb.recordspec.recno;

    myassert1x(data_ref.data_space == DATA_DB_FIELD, TEXT("Not DATA_DB_FIELD ") S32_TFMT, data_ref.data_space);

    status = record_view_recnum(&p_rec_projector->opendb, data_ref.arg.db_field.record);

#if DPLIB
    if(status_ok(status))
        if(p_rec_projector->opendb.recordspec.ncards != 0)
            status = dplib_record_delete(&p_rec_projector->opendb);
#endif

    rec_update_projector_adjust_goto(p_rec_projector, MAX(0, old_recno - 1));

    cur_change_after(p_docu);

    return(status);
}

static BOOL
is_data_ref_editable_field_type(
    P_REC_PROJECTOR p_rec_projector,
    _InRef_     PC_DATA_REF p_data_ref)
{
    PC_FIELDDEF p_fielddef;

    if(p_data_ref->data_space == DATA_DB_TITLE)
        return(TRUE);

    assert(DATA_DB_FIELD == p_data_ref->data_space);

    if(P_DATA_NONE == (p_fielddef = p_fielddef_from_field_id(&p_rec_projector->opendb.table.h_fielddefs, p_data_ref->arg.db_field.field_id)))
        return(FALSE);

    switch(p_fielddef->type)
    {
    case FIELD_FORMULA:
    case FIELD_FILE:
    case FIELD_PICTURE:
        /* you can't textually edit these */
        return(FALSE);

    default:
        break;
    }

    return(TRUE);
}

_Check_return_
static STATUS
rec_object_position_from_skel_point(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    P_OBJECT_POSITION_FIND p_object_position_find)
{
    DATA_REF data_ref;
    P_REC_PROJECTOR p_rec_projector;

    assert(p_object_position_find->object_data.data_ref.data_space == DATA_SLOT);

    if(NULL == (p_rec_projector = rec_data_ref_from_slr_and_skel_point(p_docu, &p_object_position_find->object_data.data_ref.arg.slr, &p_object_position_find->pos, &data_ref)))
    {
        /* Let's put it in the first or the last field */
        if(rec_data_ref_from_slr(p_docu, &p_object_position_find->object_data.data_ref.arg.slr, &data_ref))
        {
            /* we should look at the SKEL_POINT p_object_position_find->pos vs the SKEL_RECT of the SLR, and decide! */
            SKEL_RECT skel_rect_frame;

            skel_rect_from_slr(p_docu, &skel_rect_frame, &p_object_position_find->object_data.data_ref.arg.slr);

            if(p_object_position_find->pos.pixit_point.y > skel_rect_frame.br.pixit_point.y )
            {
                /* try for the last field instead */
                P_REC_PROJECTOR p_rec_projector_last = rec_data_ref_from_slr_and_fn(p_docu, &p_object_position_find->object_data.data_ref.arg.slr, 0x7FFFFFFF, &data_ref);

                if(NULL != p_rec_projector_last)
                    p_rec_projector = p_rec_projector_last;
            }
        }
    }

    if(!p_rec_projector || (PROJECTOR_TYPE_CARD != p_rec_projector->projector_type))
        return(STATUS_FAIL);

    {
    OBJECT_DATA object_data_rec = p_object_position_find->object_data;
    OBJECT_DATA object_data;
    SKEL_RECT skel_rect_frame;

    object_data_rec.data_ref = data_ref;

    if(status_fail(rec_skel_rect_from_data_ref(p_rec_projector, &skel_rect_frame, &data_ref)))
    {
        set_rec_object_position_field_number(&p_object_position_find->object_data.object_position_end,   -1);
        set_rec_object_position_field_number(&p_object_position_find->object_data.object_position_start, -1);
        return(STATUS_OK); /* don't fail further message processing if we couldn't do it */
    }

    if(is_data_ref_editable_field_type(p_rec_projector, &data_ref) && status_ok(rec_text_from_card(p_docu, &object_data, &object_data_rec)))
    {
        TEXT_MESSAGE_BLOCK text_message_block;

        rec_text_message_block_init(p_docu, &text_message_block, p_object_position_find, &skel_rect_frame, &object_data);

        if(status_ok(object_call_id(OBJECT_ID_STORY, p_docu, t5_message, &text_message_block)))
        {
            set_rec_object_position_from_data_ref(p_docu, &p_object_position_find->object_data.object_position_end,   &data_ref);
            set_rec_object_position_from_data_ref(p_docu, &p_object_position_find->object_data.object_position_start, &data_ref);
            return(STATUS_DONE);
        }
    }

    /* not editable or failed to get text from card and process correctly */
    set_rec_object_position_from_data_ref(p_docu, &p_object_position_find->object_data.object_position_end,   &data_ref);
    set_rec_object_position_from_data_ref(p_docu, &p_object_position_find->object_data.object_position_start, &data_ref);
    return(STATUS_DONE);
    } /*block*/
}

_Check_return_
static STATUS
rec_skel_point_from_object_position(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    P_OBJECT_POSITION_FIND p_object_position_find)
{
    DATA_REF data_ref;
    P_REC_PROJECTOR p_rec_projector;

    assert(p_object_position_find->object_data.data_ref.data_space == DATA_SLOT);

    if(NULL == (p_rec_projector = rec_data_ref_from_slr(p_docu, &p_object_position_find->object_data.data_ref.arg.slr, &data_ref)))
        return(STATUS_FAIL);

    /* The data ref generated here will always be a DATA_DB_FIELD style refering to the first field */

    if(PROJECTOR_TYPE_CARD != p_rec_projector->projector_type)
        return(STATUS_FAIL);

    if(OBJECT_ID_REC != p_object_position_find->object_data.object_id)
        return(STATUS_FAIL);

    {
    S32 fnum = -1;

    if(p_object_position_find->object_data.object_position_start.object_id ==  OBJECT_ID_REC)
        fnum = get_rec_object_position_field_number(&p_object_position_find->object_data.object_position_start);

    if(fnum == -1)
        return(STATUS_FAIL);
    } /*block*/

    /* Reset the DATA_REF from the OBJECT_POSITION. This is necessary because the SLR to DATA_REF conversion
       Has yielded correct record number and card type information but the field_id and data_space will
       always point us at the first field in the card.
    */
    set_rec_data_ref_from_object_position(p_docu, &data_ref, &p_object_position_find->object_data.object_position_start);

    {
    OBJECT_DATA object_data_rec = p_object_position_find->object_data;
    OBJECT_DATA object_data;
    SKEL_RECT skel_rect_frame;

    object_data_rec.data_ref = data_ref;

    if(status_fail(rec_skel_rect_from_data_ref(p_rec_projector, &skel_rect_frame, &data_ref)))
        return(STATUS_OK); /* don't fail further message processing if we couldn't do it */

    if(status_ok(rec_text_from_card(p_docu, &object_data, &object_data_rec)))
    {
        TEXT_MESSAGE_BLOCK text_message_block;

        rec_text_message_block_init(p_docu, &text_message_block, p_object_position_find, &skel_rect_frame, &object_data);

        if(status_ok(object_call_id(OBJECT_ID_STORY, p_docu, t5_message, &text_message_block)))
            return(STATUS_DONE);
    }
    } /*block*/

    return(STATUS_OK); /* don't fail further message processing if we couldn't do it */
}

_Check_return_
static STATUS
rec_object_position_set(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     T5_MESSAGE t5_message,
    P_OBJECT_POSITION_SET p_object_position_set)
{
    DATA_REF data_ref;
    P_REC_PROJECTOR p_rec_projector;

    assert(p_object_position_set->object_data.data_ref.data_space == DATA_SLOT);

    if(NULL == (p_rec_projector = rec_data_ref_from_slr(p_docu, &p_object_position_set->object_data.data_ref.arg.slr, &data_ref)))
        return(STATUS_FAIL);

    if(PROJECTOR_TYPE_CARD != p_rec_projector->projector_type)
        return(STATUS_FAIL);

    {
    OBJECT_DATA object_data_rec = p_object_position_set->object_data;
    OBJECT_DATA object_data;

    switch(p_object_position_set->action)
    {
    case OBJECT_POSITION_SET_START:
        /* For the start message just initialise as the first field */
        set_rec_object_position_from_data_ref(p_docu, &object_data_rec.object_position_end,   &data_ref);
        set_rec_object_position_from_data_ref(p_docu, &object_data_rec.object_position_start, &data_ref);
        object_data_rec.data_ref = data_ref;
        break;

    default:
#if CHECKING
        myassert1x(OBJECT_ID_REC == p_object_position_set->object_data.object_id, TEXT("Object position data NOT OBJECT_ID_REC ") S32_TFMT, p_object_position_set->object_data.object_id);
        { S32 fieldnumber = get_rec_object_position_field_number(&p_object_position_set->object_data.object_position_start);
        myassert1x(fieldnumber != (-1), TEXT("field number invalid ") S32_TFMT, fieldnumber); }
#endif
        set_rec_data_ref_from_object_position(p_docu, &data_ref, &p_object_position_set->object_data.object_position_start);
        object_data_rec.data_ref = data_ref;
        break;
    }

    if(status_ok(rec_text_from_card(p_docu, &object_data, &object_data_rec)))
    {
        p_object_position_set->object_data = object_data;

        if(status_ok(object_call_id(OBJECT_ID_STORY, p_docu, t5_message, p_object_position_set)))
        {
            rec_object_position_copy(&p_object_position_set->object_data.object_position_start, &object_data_rec.object_position_start);
            return(STATUS_DONE);
        }
    }
    } /*block*/

    return(STATUS_OK); /* don't fail further message processing if we couldn't do it */
}

static void
rec_buttons_init(
    _DocuRef_   P_DOCU p_docu)
{
    /* tools are all left un-enabled and nobbled in this document until we get the input focus */
    U32 i;

    for(i = 0; i < elemof32(object_tools); ++i)
    {
        T5_TOOLBAR_TOOL_NOBBLE t5_toolbar_tool_nobble;
        t5_toolbar_tool_nobble.nobbled = TRUE;
        t5_toolbar_tool_nobble.name = object_tools[i].name;
        status_consume(object_call_id(OBJECT_ID_TOOLBAR, p_docu, T5_MSG_TOOLBAR_TOOL_NOBBLE, &t5_toolbar_tool_nobble));
    }
}

_Check_return_
extern STATUS
rec_buttons_encode(
    _DocuRef_   P_DOCU p_docu)
{
    P_REC_INSTANCE_DATA p_rec_instance = p_object_instance_data_REC(p_docu);
    BOOL focus_in_database = FALSE;
    BOOL focus_in_database_template = FALSE;

    if((OBJECT_ID_CELLS == p_docu->focus_owner) || (OBJECT_ID_REC_FLOW == p_docu->focus_owner))
    {
        DATA_REF data_ref;
        P_REC_PROJECTOR p_rec_projector;

        if(NULL != (p_rec_projector = rec_data_ref_from_slr(p_docu, &p_docu->cur.slr, &data_ref)))
        {
            STATUS resource_id;
            S32 r;
            S32 n;

            record_current(&p_rec_projector->opendb, &r, &n);

            r = get_record_from_rec_data_ref(&data_ref);

            resource_id = ((p_rec_projector->opendb.search.query_id == RECORDZ_WHOLE_FILE) ? REC_MSG_STATUS_LINE_WHOLE : REC_MSG_STATUS_LINE_SUBSET) + (n == 1);

            status_line_setf(p_docu, STATUS_LINE_LEVEL_INFORMATION_FOCUS_2(OBJECT_ID_CELLS), resource_id, r + 1, n);

            focus_in_database = TRUE;
        }
        else if(OBJECT_ID_REC_FLOW == p_docu->object_id_flow)
        {
            static const UI_TEXT ui_text_e_d = UI_TEXT_INIT_RESID(REC_MSG_STATUS_LINE_EMPTY_DOCUMENT);
            static const UI_TEXT ui_text_e_t = UI_TEXT_INIT_RESID(REC_MSG_STATUS_LINE_EMPTY_TEMPLATE);

            status_line_set(p_docu, STATUS_LINE_LEVEL_INFORMATION_FOCUS_2(OBJECT_ID_CELLS), (NULL == p_docu->docu_name.path_name) ? &ui_text_e_t : &ui_text_e_d);

            focus_in_database_template = TRUE;
        }
    }

    if((p_rec_instance->focus_in_database != focus_in_database) || (p_rec_instance->focus_in_database_template != focus_in_database_template))
    {
        if(!focus_in_database && !focus_in_database_template)
            status_line_clear(p_docu, STATUS_LINE_LEVEL_INFORMATION_FOCUS_2(OBJECT_ID_CELLS));

        p_rec_instance->focus_in_database = focus_in_database;
        p_rec_instance->focus_in_database_template = focus_in_database_template;

        { /* SKS 28jul95 show database tools if focus_in_database or template. but only enable them when in full database */
        U32 i;

        for(i = 0; i < elemof32(object_tools); ++i)
        {
            T5_TOOLBAR_TOOL_NOBBLE t5_toolbar_tool_nobble;
            t5_toolbar_tool_nobble.nobbled = !focus_in_database && !focus_in_database_template;
            t5_toolbar_tool_nobble.name = object_tools[i].name;
            status_consume(object_call_id(OBJECT_ID_TOOLBAR, p_docu, T5_MSG_TOOLBAR_TOOL_NOBBLE, &t5_toolbar_tool_nobble));
        }

        for(i = 0; i < elemof32(object_tools); ++i)
        {
            T5_TOOLBAR_TOOL_ENABLE t5_toolbar_tool_enable;
            t5_toolbar_tool_enable.enable_id = TOOL_ENABLE_REC_FOCUS;
            t5_toolbar_tool_enable.enabled = focus_in_database;
            tool_enable(p_docu, &t5_toolbar_tool_enable, object_tools[i].name);
        }
        } /*block*/

#if 0
        {
        U32 i;

        static const PCTSTR
        enable_tools[] =
        {
        };

        for(i = 0; i < elemof32(enable_tools); ++i)
        {
            T5_TOOLBAR_TOOL_ENABLE t5_toolbar_tool_enable;
            t5_toolbar_tool_enable.enable_id = TOOL_ENABLE_REC_FOCUS;
            t5_toolbar_tool_enable.enabled = buttons_enabled;
            tool_enable(p_docu, &t5_toolbar_tool_enable, enable_tools[i]/*.name*/);
        }
        } /*block*/
#endif

        { /* when focus_in_database or template, turn off all these 'normal' tools */
        U32 i;

        static const PCTSTR
        nobble_tools[] =
        {
            TEXT("JUSTIFY_LEFT"),
            TEXT("JUSTIFY_CENTRE"),
            TEXT("JUSTIFY_RIGHT"),
            TEXT("JUSTIFY_FULL"),
            TEXT("STYLE"),
            TEXT("EFFECTS"),
            TEXT("BOLD"),
            TEXT("ITALIC"),
            TEXT("UNDERLINE"),
            TEXT("SUPERSCRIPT"),
            TEXT("SUBSCRIPT"),
            TEXT("CHECK"),
            TEXT("THESAURUS"),
            TEXT("SEARCH"),
            TEXT("SORT"),
            TEXT("INSERT_DATE"),
            TEXT("TABLE"),
            TEXT("BOX"),
            TEXT("TAB_LEFT"),
            TEXT("TAB_CENTRE"),
            TEXT("TAB_RIGHT"),
            TEXT("TAB_DECIMAL"),

            TEXT("FILL_DOWN"),
            TEXT("FILL_RIGHT"),
            TEXT("AUTO_SUM"),
            TEXT("CHART"),
            TEXT("PLUS"),
            TEXT("MINUS"),
            TEXT("TIMES"),
            TEXT("DIVIDE"),
            TEXT("MAKE_TEXT"),
            TEXT("MAKE_NUMBER"),
            TEXT("MAKE_CONSTANT"),
            TEXT("FUNCTION"),
            TEXT("FORMULA_CANCEL"),
            TEXT("FORMULA_ENTER"),
            TEXT("FORMULA_LINE")
        };

        for(i = 0; i < elemof32(nobble_tools); ++i)
        {
            T5_TOOLBAR_TOOL_NOBBLE t5_toolbar_tool_nobble;
            t5_toolbar_tool_nobble.nobbled = p_rec_instance->focus_in_database || p_rec_instance->focus_in_database_template;
            t5_toolbar_tool_nobble.name = nobble_tools[i]/*.name*/;
            status_consume(object_call_id(OBJECT_ID_TOOLBAR, p_docu, T5_MSG_TOOLBAR_TOOL_NOBBLE, &t5_toolbar_tool_nobble));
        }
        } /*block*/
    }

    return(STATUS_OK);
}

/*
main events
*/

T5_MSG_PROTO(static, ob_rec_msg_save, _InRef_ PC_MSG_SAVE p_msg_save)
{
    IGNOREPARM_InVal_(t5_message);

    switch(p_msg_save->t5_msg_save_message)
    {
    case T5_MSG_SAVE__DATA_SAVE_1: /* must be before cell data is saved for save_cell_ownform bodge to work */
        return(ob_rec_save(p_docu, p_msg_save->p_of_op_format));

    default:
        return(STATUS_OK);
    }
}

MAEVE_EVENT_PROTO(static, maeve_event_ob_rec)
{
    IGNOREPARM_InRef_(p_maeve_block);

    switch(t5_message)
    {
    case T5_MSG_SAVE:
        return(ob_rec_msg_save(p_docu, t5_message, (PC_MSG_SAVE) p_data));

    case T5_MSG_CUR_CHANGE_BEFORE:
        return(rec_cache_purge(p_docu)); /* Purge the editing buffer - we may be about to do something dangerous */

    case T5_MSG_CUR_CHANGE_AFTER:
    case T5_MSG_DOCU_COLROW:
    case T5_MSG_FOCUS_CHANGED:
        return(rec_buttons_encode(p_docu));

    default:
        return(STATUS_OK);
    }
}

/******************************************************************************
*
* rec object event handler
*
******************************************************************************/

T5_MSG_PROTO(static, rec_msg_initclose, _InRef_ PC_MSG_INITCLOSE p_msg_initclose)
{
    IGNOREPARM_InVal_(t5_message);

    switch(p_msg_initclose->t5_msg_initclose_message)
    {
    case T5_MSG_IC__STARTUP:
        status_return(resource_init(OBJECT_ID_REC, MSG_WEAK, P_BOUND_RESOURCES_OBJECT_ID_REC));

        drag_state.flag = 0;

#if DPLIB
        status_return(dplib_init()); /* Kick DPLIB info life */
#endif

        /* initialise bitmap constants */
        style_selector_copy(&style_selector_ob_rec, &style_selector_font_spec);
        void_style_selector_or(&style_selector_ob_rec, &style_selector_ob_rec, &style_selector_para_leading);
        style_selector_bit_set(&style_selector_ob_rec, STYLE_SW_CS_WIDTH);
        style_selector_bit_set(&style_selector_ob_rec, STYLE_SW_PS_MARGIN_LEFT);

        style_selector_bit_set(&style_selector_ob_rec, STYLE_SW_PS_MARGIN_PARA); /* 16.3.95 PMF add this to see if auto width better */

        style_selector_bit_set(&style_selector_ob_rec, STYLE_SW_PS_MARGIN_RIGHT);
        style_selector_bit_set(&style_selector_ob_rec, STYLE_SW_PS_NUMFORM_NU);
        style_selector_bit_set(&style_selector_ob_rec, STYLE_SW_PS_NUMFORM_DT);
        style_selector_bit_set(&style_selector_ob_rec, STYLE_SW_PS_NUMFORM_SE);
        style_selector_bit_set(&style_selector_ob_rec, STYLE_SW_PS_JUSTIFY);
        style_selector_bit_set(&style_selector_ob_rec, STYLE_SW_PS_JUSTIFY_V);
        style_selector_bit_set(&style_selector_ob_rec, STYLE_SW_PS_PARA_START);
        style_selector_bit_set(&style_selector_ob_rec, STYLE_SW_PS_PARA_END);
        style_selector_bit_set(&style_selector_ob_rec, STYLE_SW_PS_RGB_BORDER);
        style_selector_bit_set(&style_selector_ob_rec, STYLE_SW_PS_BORDER);

        {
        T5_TOOLBAR_TOOLS t5_toolbar_tools;
        t5_toolbar_tools.n_tool_desc = elemof32(object_tools);
        t5_toolbar_tools.p_t5_toolbar_tool_desc = object_tools;
        status_return(object_call_id(OBJECT_ID_TOOLBAR, P_DOCU_NONE, T5_MSG_TOOLBAR_TOOLS, &t5_toolbar_tools));
        } /*block*/

        return(register_object_construct_table(OBJECT_ID_REC, object_construct_table, TRUE /* commands requiring preprocessing */));

    case T5_MSG_IC__EXIT1:
        return(resource_close(OBJECT_ID_REC));

    case T5_MSG_IC__INIT1:
        {
        P_REC_INSTANCE_DATA p_rec_instance = p_object_instance_data_REC(p_docu);
        zero_struct_ptr(p_rec_instance);
        } /*block*/

        rec_buttons_init(p_docu);

        return(maeve_event_handler_add(p_docu, maeve_event_ob_rec, (CLIENT_HANDLE) 0));

    case T5_MSG_IC__CLOSE1:
        maeve_event_handler_del(p_docu, maeve_event_ob_rec, (CLIENT_HANDLE) 0);
        return(STATUS_OK);

    case T5_MSG_IC__CLOSE2:
        rec_closedown(p_docu);
        return(STATUS_OK);

    default:
        return(STATUS_OK);
    }
}

OBJECT_PROTO(extern, object_rec);
OBJECT_PROTO(extern, object_rec)
{
    STATUS status = STATUS_OK;

    switch(t5_message)
    {
    case T5_MSG_INITCLOSE:
        return(rec_msg_initclose(p_docu, t5_message, (PC_MSG_INITCLOSE) p_data));

    case T5_EVENT_NULL:
        rec_event_null(p_data);
        break;

    case T5_CMD_RECORDZ_IO_PATTERN:
        return(rec_io_pattern(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_RECORDZ_IO_QUERY:
        return(rec_io_query(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_RECORDZ_IO_TABLE:
        return(rec_io_table(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_RECORDZ_IO_FRAME:
        return(rec_io_frame(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_RECORDZ_IO_TITLE:
        return(rec_io_title(p_docu, t5_message, (PC_T5_CMD) p_data));

    case T5_CMD_CREATE_RECORDZ:
        cur_change_before(p_docu);
        status = t5_cmd_db_create(p_docu);
        rec_update_docu(p_docu);
        cur_change_after(p_docu);
        break;

    case T5_CMD_LAYOUT_RECORDZ:
        {
        DATA_REF data_ref;

        if(!rec_data_ref_from_slr(p_docu, &p_docu->cur.slr, &data_ref))
            return(create_error(REC_ERR_NO_DATABASE));

        cur_change_before(p_docu);
        status = t5_cmd_db_layout(p_docu, &data_ref);
        rec_update_docu(p_docu);
        cur_change_after(p_docu);
        break;
        }

    case T5_CMD_PROP_RECORDZ:
        {
        DATA_REF data_ref;

        if(!rec_data_ref_from_slr(p_docu, &p_docu->cur.slr, &data_ref))
            return(create_error(REC_ERR_NO_DATABASE));

        cur_change_before(p_docu);
        status = t5_cmd_db_properties(p_docu, &data_ref);
        rec_update_docu(p_docu);
        cur_change_after(p_docu);
        break;
        }

    case T5_CMD_EXPORT_RECORDZ:
        {
        DATA_REF data_ref;

        if(!rec_data_ref_from_slr(p_docu, &p_docu->cur.slr, &data_ref))
            return(create_error(REC_ERR_NO_DATABASE));

        cur_change_before(p_docu);
        status = t5_cmd_db_export(p_docu, &data_ref);
        cur_change_after(p_docu);
        break;
        }

    case T5_MSG_RECORDZ_IMPORT:
        return(rec_import_csv(p_docu, p_data));

    case T5_CMD_CLOSE_RECORDZ:
        {
        DATA_REF data_ref;
        P_REC_PROJECTOR p_rec_projector;

        if(!rec_data_ref_from_slr(p_docu, &p_docu->cur.slr, &data_ref))
            return(create_error(REC_ERR_NO_DATABASE));

        if(OBJECT_ID_REC_FLOW == p_docu->object_id_flow)
        {
            /* SKS 04nov95 simply try to close the container as this is otherwise nonsensical */
            process_close_request(p_docu, p_view_from_viewno_caret(p_docu), TRUE, FALSE);
        }
        else
        {
            cur_change_before(p_docu);
            p_rec_projector = p_rec_projector_from_db_id(p_docu, data_ref.arg.db_field.db_id);
            drop_projector_area(p_rec_projector);
            status = close_database(&p_rec_projector->opendb, status);
            rec_kill_projector(&p_rec_projector);
            rec_update_docu(p_docu);
            cur_change_after(p_docu);
        }

        break;
        }

    case T5_CMD_SORT_RECORDZ:
        {
        DATA_REF data_ref;

        if(!rec_data_ref_from_slr(p_docu, &p_docu->cur.slr, &data_ref))
        {
            if(OBJECT_ID_REC_FLOW == p_docu->object_id_flow)
                return(create_error(REC_ERR_NO_DATABASE));

            return(STATUS_OK); /* possibly back to t5_cmd_sort_intro */
        }

        cur_change_before(p_docu);
        status = t5_cmd_db_sort(p_docu, &data_ref);
        cur_change_after(p_docu);
        break;
        }

    case T5_CMD_SEARCH_RECORDZ:
        {
        DATA_REF data_ref;

        if(!rec_data_ref_from_slr(p_docu, &p_docu->cur.slr, &data_ref))
        {
            if(OBJECT_ID_REC_FLOW == p_docu->object_id_flow)
                return(create_error(REC_ERR_NO_DATABASE));

            return(STATUS_OK); /* possibly back to t5_cmd_search_button_poss_db_query */
        }

        /* To Search from scratch we must drop the suggestions if any exist */
        drop_search_suggestions(&p_rec_projector_from_db_id(p_docu, data_ref.arg.db_field.db_id)->opendb.search);

        cur_change_before(p_docu);
        status = t5_cmd_db_search(p_docu, &data_ref);
        cur_change_after(p_docu);
        break;
        }

    case T5_CMD_VIEW_RECORDZ:
        {
        DATA_REF data_ref;

        if(!rec_data_ref_from_slr(p_docu, &p_docu->cur.slr, &data_ref))
        {
            if(OBJECT_ID_REC_FLOW == p_docu->object_id_flow)
                return(create_error(REC_ERR_NO_DATABASE));

            return(STATUS_OK); /* possibly back to t5_cmd_search_button_poss_db_queries */
        }

        cur_change_before(p_docu);
        status = t5_cmd_db_view(p_docu, &data_ref);
        cur_change_after(p_docu);
        break;
        }

    case T5_CMD_STYLE_BUTTON:
        return(object_call_id_load(p_docu, t5_message, p_data, OBJECT_ID_SKEL_SPLIT));

    case T5_CMD_STYLE_RECORDZ:
        {
        DATA_REF data_ref;

        if(!rec_data_ref_from_slr(p_docu, &p_docu->cur.slr, &data_ref))
        {
            if(OBJECT_ID_REC_FLOW == p_docu->object_id_flow)
                return(create_error(REC_ERR_NO_DATABASE));

            return(STATUS_OK); /* back to t5_cmd_style_intro */
        }

        set_rec_data_ref_from_object_position(p_docu, &data_ref, &p_docu->cur.object_position);

        status = t5_cmd_db_style(p_docu, &data_ref);
        break;
        }

    case T5_CMD_RECORDZ_GOTO_RECORD_INTRO:
        {
        DATA_REF data_ref;

        if(!rec_data_ref_from_slr(p_docu, &p_docu->cur.slr, &data_ref))
            return(create_error(REC_ERR_NO_DATABASE));

        status = t5_cmd_db_goto_record_intro(p_docu, &data_ref);
        break;
        }

    case T5_CMD_RECORDZ_GOTO_RECORD:
        {
        const PC_T5_CMD p_t5_cmd = (PC_T5_CMD) p_data;
        const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_t5_cmd->arglist_handle, 1);
        S32 r = p_args[0].val.s32;
        return(rec_goto_record(p_docu, TRUE, r - 1));
        }

    case T5_CMD_RECORDZ_NEXT:
        return(rec_goto_record(p_docu, FALSE, +1)); /* Relative, +1 => next */

    case T5_CMD_RECORDZ_PREV:
        return(rec_goto_record(p_docu, FALSE, -1));

    case T5_CMD_RECORDZ_FFWD:
        return(rec_goto_record(p_docu, TRUE, RECORDZ_PAGE_DOWN));

    case T5_CMD_RECORDZ_REWIND:
        return(rec_goto_record(p_docu, TRUE, RECORDZ_PAGE_UP));

    case T5_CMD_RECORDZ_ERSTE:
        return(rec_goto_record(p_docu, TRUE, 0)); /* Abs, 0 => first */

    case T5_CMD_RECORDZ_LETSTE:
        return(rec_goto_record(p_docu, TRUE, RECORDZ_GOTO_LAST)); /* Means go as far as possible */

    case T5_CMD_ADD_RECORDZ:
        {
        DATA_REF data_ref;

        if(!rec_data_ref_from_slr(p_docu, &p_docu->cur.slr, &data_ref))
            return(create_error(REC_ERR_NO_DATABASE));

        cur_change_before(p_docu);
        /* I think we should do a view whole file here 'cos the added record is blank and probably not in the subset */
        if(status_ok(status = rec_revert_whole(p_rec_projector_from_db_id(p_docu, data_ref.arg.db_field.db_id))))
            status = rec_add_record(p_docu, TRUE);
        rec_update_docu(p_docu);
        cur_change_after(p_docu);
        break;
        }

    case T5_CMD_COPY_RECORDZ:
        { /* The same as add but different */
        DATA_REF data_ref;

        if(!rec_data_ref_from_slr(p_docu, &p_docu->cur.slr, &data_ref))
            return(create_error(REC_ERR_NO_DATABASE));

        cur_change_before(p_docu);
        /* no need to revert to whole file as new record will be in current subset as well */
        status = rec_add_record(p_docu, FALSE);
        rec_update_docu(p_docu);
        cur_change_after(p_docu);
        break;
        }

    case T5_CMD_DELETE_RECORDZ:
        {
        DATA_REF data_ref;

        if(!rec_data_ref_from_slr(p_docu, &p_docu->cur.slr, &data_ref))
            return(create_error(REC_ERR_NO_DATABASE));

        cur_change_before(p_docu);
        status = rec_delete_record(p_docu);
        rec_update_docu(p_docu);
        cur_change_after(p_docu);
        break;
        }

    case T5_CMD_ACTIVATE_MENU_CHART:
        p_docu->flags.next_chart_unpinned = 1;
        return(t5_cmd_activate_menu(p_docu, t5_message));

#if 0
    case T5_MSG_STATUS_LINE_MESSAGE_QUERY:
        {
        P_UI_TEXT p_ui_text = (P_UI_TEXT) p_data;

        if(UI_TEXT_TYPE_RESID == p_ui_text->type)
        {
            BOOL is_rec_flow = (OBJECT_ID_REC_FLOW == p_docu->object_id_flow);
            DATA_REF data_ref;

            if(MSG_STATUS_APPLY_STYLE == p_ui_text->text.resource_id)
            {
                if(is_rec_flow || !rec_data_ref_from_slr(p_docu, &p_docu->cur.slr, &data_ref))
                    p_ui_text->text.resource_id = REC_MSG_STATUS_STYLE_EXTEND;
            }
        }

        break;
    }
#endif

    case T5_MSG_CELLS_EXTENT:
        {
        P_CELLS_EXTENT p_cells_extent = (P_CELLS_EXTENT) p_data;
        return(max_rec_projector(p_docu, &p_cells_extent->region));
        }

    case T5_MSG_OBJECT_POSITION_AT_START:
        {
        P_OBJECT_POSITION_COMPARE p_object_position_compare = (P_OBJECT_POSITION_COMPARE) p_data;
        p_object_position_compare->resultant_order = rec_object_position_at_start(&p_object_position_compare->first);
        break;
        }

    case T5_MSG_COMPARE_OBJECT_POSITIONS:
        {
        P_OBJECT_POSITION_COMPARE p_object_position_compare = (P_OBJECT_POSITION_COMPARE) p_data;
        /* this returns object_position_compare.resultant_order as per strcmp() */
        p_object_position_compare->resultant_order = rec_object_position_compare(&p_object_position_compare->first, &p_object_position_compare->second);
        break;
        }

    case T5_MSG_OBJECT_POSITION_SET_START:
        rec_object_position_set_start((P_OBJECT_POSITION) p_data);
        break;

    case T5_MSG_DATA_REF_TO_SLR:
        {
        P_DATA_REF_AND_SLR p_data_ref_and_slr = (P_DATA_REF_AND_SLR) p_data;
        status = rec_slr_from_data_ref(p_docu, &p_data_ref_and_slr->data_ref, &p_data_ref_and_slr->slr);
        break;
        }

    case T5_MSG_DATA_REF_FROM_SLR: /* This will always return the first field in a card */
        {
        P_DATA_REF_AND_SLR p_data_ref_and_slr = (P_DATA_REF_AND_SLR) p_data;

        if(!rec_data_ref_from_slr(p_docu, &p_data_ref_and_slr->slr, &p_data_ref_and_slr->data_ref))
            return(create_error(REC_ERR_NO_DATABASE));

        break;
        }

    case T5_MSG_DATA_REF_TO_POSITION:
        {
        P_DATA_REF_AND_POSITION p_data_ref_and_position = (P_DATA_REF_AND_POSITION) p_data;

        if(status_ok(status = rec_slr_from_data_ref(p_docu, &p_data_ref_and_position->data_ref, &p_data_ref_and_position->position.slr)))
        {
            S32 field_number = fieldnumber_from_rec_data_ref(p_docu, &p_data_ref_and_position->data_ref);
            p_data_ref_and_position->position.object_position.object_id = OBJECT_ID_REC;
            p_data_ref_and_position->position.object_position.data = 0 ; /* We can't fill in the sub-object-position given just the data_ref */
            set_rec_object_position_field_number(&p_data_ref_and_position->position.object_position, field_number);
        }
        else
            p_data_ref_and_position->position.object_position.object_id = OBJECT_ID_NONE;

        break;
        }

    case T5_MSG_DATA_REF_FROM_POSITION:
        {
        P_DATA_REF_AND_POSITION p_data_ref_and_position = (P_DATA_REF_AND_POSITION) p_data;

        if(!rec_data_ref_from_slr(p_docu, &p_data_ref_and_position->position.slr, &p_data_ref_and_position->data_ref))
        {
            p_data_ref_and_position->data_ref.data_space = DATA_NONE;
            return(create_error(REC_ERR_NO_DATABASE));
        }

        if(OBJECT_ID_REC == p_data_ref_and_position->position.object_position.object_id)
            set_rec_data_ref_field_by_number(p_docu, &p_data_ref_and_position->data_ref, get_rec_object_position_field_number(&p_data_ref_and_position->position.object_position));

        break;
        }

    case T5_MSG_DATA_REF_NAME_COMPARE:
        {
        P_DATA_REF_NAME_COMPARE p_data_ref_name_compare = (P_DATA_REF_NAME_COMPARE) p_data;
        DATA_REF data_ref_for_name;

        if(status_ok(status = rec_data_ref_from_name(p_docu_from_docno(p_data_ref_name_compare->docno),
                                                      p_data_ref_name_compare->p_compound_name,
                                                      0,
                                                      &data_ref_for_name)))
        {
           if((data_ref_for_name.arg.db_field.db_id  == p_data_ref_name_compare->data_ref.arg.db_field.db_id)
           && (data_ref_for_name.arg.db_field.field_id == p_data_ref_name_compare->data_ref.arg.db_field.field_id))
           {
                status = STATUS_OK;
                p_data_ref_name_compare->result = 0 ; /* They are the same */
           }
           else
           {
                status = STATUS_OK;
                p_data_ref_name_compare->result = 1 ; /* They are different */
           }
        }
        else
        {
            status = STATUS_OK;
            p_data_ref_name_compare->result = 1 ; /* They are different */
        }

        break;
        }

    case T5_MSG_OBJECT_DATA_READ:
        return(rec_object_data_read(p_docu, p_data));

    case T5_MSG_FIELD_DATA_N_RECORDS:
        {
        P_FIELD_DATA_QUERY p_field_data_query = (P_FIELD_DATA_QUERY) p_data;
        DATA_REF data_ref;
        P_REC_PROJECTOR p_rec_projector;

        if(status_ok(status = rec_data_ref_from_name(p_docu_from_docno(p_field_data_query->docno),
                                                     p_field_data_query->p_compound_name,
                                                     p_field_data_query->record_no,
                                                     &data_ref
                                                     )))
        {
            p_rec_projector = p_rec_projector_from_db_id(p_docu_from_docno(p_field_data_query->docno),data_ref.arg.db_field.db_id);
            p_field_data_query->record_no = p_rec_projector->opendb.recordspec.ncards;
        }

        break;
        }

    case T5_MSG_FIELD_DATA_READ:
        {
        P_FIELD_DATA_QUERY p_field_data_query = (P_FIELD_DATA_QUERY) p_data;
        DATA_REF data_ref;

        if(status_ok(status = rec_data_ref_from_name(p_docu_from_docno(p_field_data_query->docno),
                                                     p_field_data_query->p_compound_name,
                                                     p_field_data_query->record_no,
                                                     &data_ref)))
        {
            P_REC_PROJECTOR p_rec_projector = p_rec_projector_from_db_id(p_docu_from_docno(p_field_data_query->docno), data_ref.arg.db_field.db_id);
            REC_RESOURCE rec_resource;
            status = rec_read_resource(&rec_resource, p_rec_projector, &data_ref, TRUE);
            p_field_data_query->ev_data = rec_resource.ev_data;
        }

        break;
        }

    case T5_EXT_STYLE_RECORDZ:
        rec_ext_style(p_docu, (P_IMPLIED_STYLE_QUERY) p_data);
        break;

    case T5_EXT_STYLE_RECORDZ_FIELDS:
        rec_ext_style_fields(p_docu, (P_IMPLIED_STYLE_QUERY) p_data);
        break;

    case T5_MSG_OBJECT_EDITABLE:
        rec_object_editable(p_docu, (P_OBJECT_EDITABLE) p_data);
        break;

    case T5_MSG_OBJECT_HOW_BIG:
        {
        P_OBJECT_HOW_BIG p_object_how_big = (P_OBJECT_HOW_BIG) p_data;
        PIXIT_POINT pixit_point;

        assert(p_object_how_big->object_data.data_ref.data_space == DATA_SLOT);

        rec_object_pixit_size(p_docu, &pixit_point, &p_object_how_big->object_data.data_ref.arg.slr, NULL);

        p_object_how_big->skel_rect.br = p_object_how_big->skel_rect.tl;
        p_object_how_big->skel_rect.br.pixit_point.x += pixit_point.x;
        p_object_how_big->skel_rect.br.pixit_point.y += pixit_point.y;
        break;
        }

    case T5_MSG_OBJECT_HOW_WIDE:
        return(rec_object_how_wide(p_docu, (P_OBJECT_HOW_WIDE) p_data));

    case T5_EVENT_REDRAW:
        {
        const P_OBJECT_REDRAW p_object_redraw = (P_OBJECT_REDRAW) p_data;
        DATA_REF data_ref;
        P_REC_PROJECTOR p_rec_projector;

        myassert1x(p_object_redraw->object_data.data_ref.data_space == DATA_SLOT, TEXT("Not DATA_SLOT") S32_TFMT, p_object_redraw->object_data.data_ref.data_space);

        /* in a card view, the data_ref constructed here will be to the first field of the record */
        if(NULL == (p_rec_projector = rec_data_ref_from_slr(p_docu, &p_object_redraw->object_data.data_ref.arg.slr, &data_ref)))
            break;

        if(PROJECTOR_TYPE_CARD == data_ref.arg.db_field.projector_type)
            status = rec_card_redraw(p_object_redraw, p_rec_projector, &data_ref);
        else
            status = rec_sheet_redraw_field(p_object_redraw, p_rec_projector, &data_ref);

        break;
        }

    case T5_MSG_OBJECT_READ_TEXT_DRAFT: /* Convert to plain text,  similar to the redraw stuff above */
        return(rec_read_text_draft(p_docu, (P_OBJECT_READ_TEXT_DRAFT) p_data));

    /* MRJC 28.1.95: handle relevant inserts */
    case T5_CMD_FIELD_INS_DATE:
    case T5_CMD_FIELD_INS_FILE_DATE:
    case T5_CMD_FIELD_INS_RETURN:
        return(object_call_id(OBJECT_ID_STORY, p_docu, t5_message, p_data));

    /* Pass the buck to ed_rec */

    case T5_MSG_OBJECT_STRING_SEARCH:
    case T5_MSG_OBJECT_STRING_REPLACE:

    case T5_CMD_SETC_UPPER:
    case T5_CMD_SETC_LOWER:
    case T5_CMD_SETC_INICAP:
    case T5_CMD_SETC_SWAP:

    case T5_EVENT_CLICK_LEFT_SINGLE:
    case T5_EVENT_CLICK_LEFT_DOUBLE:
    case T5_EVENT_CLICK_LEFT_DRAG:
    case T5_EVENT_CLICK_DRAG_STARTED:
    case T5_EVENT_CLICK_DRAG_MOVEMENT:
    case T5_EVENT_CLICK_DRAG_ABORTED:
    case T5_EVENT_CLICK_DRAG_FINISHED:

    case T5_MSG_NEW_OBJECT_FROM_TEXT:
    case T5_MSG_OBJECT_READ_TEXT:

    case T5_MSG_OBJECT_KEYS:
    case T5_MSG_OBJECT_LOGICAL_MOVE:
    case T5_MSG_OBJECT_DELETE_SUB:
        return(proc_event_ed_rec_direct(p_docu, t5_message, p_data));

    case T5_EVENT_FILEINSERT_DOINSERT_1:
        return(rec_event_fileinsert_doinsert_1(p_docu, p_data));

    case T5_MSG_OBJECT_POSITION_FROM_SKEL_POINT:
        return(rec_object_position_from_skel_point(p_docu, t5_message, p_data));

    case T5_MSG_SKEL_POINT_FROM_OBJECT_POSITION:
        return(rec_skel_point_from_object_position(p_docu, t5_message, p_data));

    case T5_MSG_OBJECT_POSITION_SET:
        return(rec_object_position_set(p_docu, t5_message, p_data));

    case T5_MSG_TAB_WANTED:
        {
        P_TAB_WANTED p_tab_wanted = (P_TAB_WANTED) p_data;
        DATA_REF data_ref;
        P_REC_PROJECTOR p_rec_projector;

        if(NULL != (p_rec_projector = rec_data_ref_from_slr(p_docu, &p_tab_wanted->object_data.data_ref.arg.slr, &data_ref)))
        {
            if(PROJECTOR_TYPE_CARD == p_rec_projector->projector_type)
            {
                p_tab_wanted->processed = 1;

                switch(p_tab_wanted->t5_message)
                {
                case T5_CMD_TAB_LEFT:
                    return(proc_event_ed_rec_direct(p_docu, T5_MSG_FIELD_PREV, &p_tab_wanted->object_data));

                case T5_CMD_TAB_RIGHT:
                    return(proc_event_ed_rec_direct(p_docu, T5_MSG_FIELD_NEXT, &p_tab_wanted->object_data));

                default: default_unhandled(); break;
                }
            }
        }

        break;
        }

    case T5_MSG_INSERT_FOREIGN:
        return(rec_msg_insert_foreign(p_docu, p_data));

    case T5_MSG_LOAD_CELL_OWNFORM:
        cur_change_before(p_docu);
        proc_event_ed_rec_direct(p_docu, t5_message, p_data);
        cur_change_after(p_docu);
        break;

    case T5_MSG_SAVE_CELL_OWNFORM:
        /* cur_change_before(p_docu) ; removed 3/4/95 to avoid record commit on clipboard access eg when using value lists */
        proc_event_ed_rec_direct(p_docu, t5_message, p_data);
        /* cur_change_after(p_docu) ;  */
        break;

    case T5_MSG_LOAD_CONSTRUCT_OWNFORM:
        {
        P_CONSTRUCT_CONVERT p_construct_convert = (P_CONSTRUCT_CONVERT) p_data;

        switch(p_construct_convert->p_construct->t5_message)
        {
        case T5_CMD_RECORDZ_IO_TABLE:
            { /* swift bit of reprocessing needed to load document relative references */
            const PC_ARGLIST_ARG p_args = pc_arglist_args(&p_construct_convert->arglist_handle, 2);
            PCTSTR filename = p_args[1].val.tstr;
            PCTSTR input_filename = p_construct_convert->p_of_ip_format->input_filename;

            if(!file_is_rooted(filename))
            {
                TCHARZ tstr_buf[BUF_MAX_PATHSTRING];

                if(status_done(status = file_find_on_path_or_relative(tstr_buf, elemof32(tstr_buf), filename, input_filename)))
                {
                    arg_dispose_val(&p_construct_convert->arglist_handle, 1);

                    /* if this fails, we are pretty well dead */
                    status_return(arg_alloc_tstr(&p_construct_convert->arglist_handle, 1, tstr_buf));
                }
            }

            break;
            }

        default:
            break;
        }

        return(execute_loaded_command(p_docu, p_construct_convert, OBJECT_ID_REC));
        }

    case T5_MSG_OBJECT_WANTS_LOAD:
        {
        P_OBJECT_WANTS_LOAD p_object_wants_load = (P_OBJECT_WANTS_LOAD) p_data;
        p_object_wants_load->object_wants_load = 1 ; /* redirect skeleton table commands though rec_load_construct_ownform */
        break;
        }

    case T5_MSG_LOAD_CONSTRUCT_OWNFORM_DIRECTED:
        return(rec_load_construct_ownform(p_docu, p_data));

    /* END OF OWNFORM RELATED STUFF */
    /******************************************************************************/

    case T5_MSG_MENU_OTHER_INFO:
        {
        P_REC_INSTANCE_DATA p_rec_instance = p_object_instance_data_REC(p_docu);
        P_MENU_OTHER_INFO p_menu_other_info = (P_MENU_OTHER_INFO) p_data;
        p_menu_other_info->focus_in_database = p_rec_instance->focus_in_database;
        p_menu_other_info->focus_in_database_template = p_rec_instance->focus_in_database_template;
        break;
        }

    default:
        break;
    }

    return(status);
}

/* end of ob_rec.c */
