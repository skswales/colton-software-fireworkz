/* ob_rec2.c */

/* Copyright (C) 1994-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#include "common/gflags.h"

#include "ob_rec/ob_rec.h"

extern STYLE_SELECTOR style_selector_ob_rec;

/* Private functions */

static S32
record_number_from_row(
    _InRef_     PC_SLR p_slr,
    P_REC_PROJECTOR p_rec_projector);

static FIELD_ID
field_id_from_col(
    _InRef_     PC_SLR p_slr,
    P_REC_PROJECTOR p_rec_projector);

/* DATA DESCRIPTORS */

/* This routine defines what a compound name is

   it is eg

   ?mountains.height

*/

_Check_return_
_Ret_maybenone_
static P_REC_PROJECTOR
p_rec_projector_from_table_name(
    _DocuRef_   P_DOCU p_docu,
    _In_reads_(namlen) PC_U8 table_name,
    _InVal_     U32 namlen)
{
    P_REC_INSTANCE_DATA p_rec_instance = p_object_instance_data_REC(p_docu);
    const ARRAY_INDEX n_rec_projectors = array_elements(&p_rec_instance->array_handle);
    ARRAY_INDEX i;
    P_REC_PROJECTOR p_rec_projector = array_range(&p_rec_instance->array_handle, REC_PROJECTOR, 0, n_rec_projectors);

    for(i = 0; i < n_rec_projectors; ++i, ++p_rec_projector)
        if(0 == /*"C"*/strncmp(p_rec_projector->opendb.table.name, table_name, namlen))
            if(CH_NULL == p_rec_projector->opendb.table.name[namlen])
                return(p_rec_projector);

    return(P_DATA_NONE);
}

_Check_return_
_Ret_maybenone_
static P_REC_PROJECTOR
p_rec_projector_and_fielddef_from_field_name(
    _DocuRef_   P_DOCU p_docu,
    P_U8 p_u8_field_name,
    P_FIELDDEF * p_p_fielddef)
{
    P_REC_INSTANCE_DATA p_rec_instance = p_object_instance_data_REC(p_docu);
    const ARRAY_INDEX n_rec_projectors = array_elements(&p_rec_instance->array_handle);
    ARRAY_INDEX i;
    P_REC_PROJECTOR p_rec_projector = array_range(&p_rec_instance->array_handle, REC_PROJECTOR, 0, n_rec_projectors);

    for(i = 0; i < n_rec_projectors; ++i, ++p_rec_projector)
        if(P_DATA_NONE != (*p_p_fielddef = p_fielddef_from_name(&p_rec_projector->opendb.table.h_fielddefs, p_u8_field_name)))
            return(p_rec_projector);

    return(P_DATA_NONE);
}

_Check_return_
extern STATUS
rec_data_ref_from_name(
    _DocuRef_   P_DOCU p_docu,
    P_U8 p_u8_name,
    _In_        S32 recno,
    _OutRef_    P_DATA_REF p_data_ref)
{
    STATUS status = STATUS_OK;
    P_U8 p_u8_dot;

    p_data_ref->data_space = DATA_NONE;

    ++p_u8_name; /* skip '?' */

    p_u8_dot = strchr(p_u8_name, CH_FULL_STOP);

    if(NULL == p_u8_dot)
    {
        /* There is only a single name. let's assume it is the FIELD name for some database */
        P_U8 p_u8_field_name = p_u8_name;
        P_REC_PROJECTOR p_rec_projector;
        P_FIELDDEF p_fielddef;

        if(P_DATA_NONE == (p_rec_projector = p_rec_projector_and_fielddef_from_field_name(p_docu, p_u8_field_name, &p_fielddef)))
            status = STATUS_FAIL;
        else
        {
            p_data_ref->data_space = DATA_DB_FIELD;
            p_data_ref->arg.db_field.projector_type = p_rec_projector->projector_type;
            p_data_ref->arg.db_field.db_id = p_rec_projector->opendb.db.id;
            p_data_ref->arg.db_field.field_id = p_fielddef->id;
            p_data_ref->arg.db_field.record = recno;
            status = STATUS_OK;
        }
    }
    else
    {
        /* We need to bind the name to a database attached to the document */
        P_U8 p_u8_table_name = p_u8_name;
        P_U8 p_u8_field_name = p_u8_dot + 1;
        U32 namlen = PtrDiffBytesU32(p_u8_dot, p_u8_name);
        P_REC_PROJECTOR p_rec_projector;
        P_FIELDDEF p_fielddef;

        if(P_DATA_NONE == (p_rec_projector = p_rec_projector_from_table_name(p_docu, p_u8_table_name, namlen)))
            status = STATUS_FAIL;
        else if(P_DATA_NONE == (p_fielddef = p_fielddef_from_name(&p_rec_projector->opendb.table.h_fielddefs, p_u8_field_name)))
            status = STATUS_FAIL;
        else
        {
            p_data_ref->data_space = DATA_DB_FIELD;
            p_data_ref->arg.db_field.projector_type = p_rec_projector->projector_type;
            p_data_ref->arg.db_field.db_id = p_rec_projector->opendb.db.id;
            p_data_ref->arg.db_field.field_id = p_fielddef->id;
            p_data_ref->arg.db_field.record = recno;
            status = STATUS_OK;
        }
    }

    return(status);
}

/* Construct a recordz type data_ref stucture.

   ------------------------------------------------------
   N.B. This should be the ONLY way to convert SLRs to
   data_refs containing db, record and field information.

   Therefore it should be the only client of

   p_rec_projector_from_slr(p_docu, p_slr)

   All other parts of the code should make use of

   p_rec_projector_from_db_id(p_docu, db_id)

   Please report any others to the supervisor.
   ------------------------------------------------------

   This uses the P_SLR into the P_DOCU to identify the rec_projector
   structure (if any) which owns this point of the document.

   Then it uses these to get at the opendb, to determine the format
   and hence calculate the record number and field_id.

   it returns the PROJECTOR_TYPE_...

*/

_Check_return_
_Ret_maybenull_
static P_REC_PROJECTOR
old_make_data_ref(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_SLR p_slr,
    _OutRef_    P_DATA_REF p_data_ref,
    _In_        S32 fieldnumber)
{
    P_REC_PROJECTOR p_rec_projector;

    if(NULL == (p_rec_projector = p_rec_projector_from_slr(p_docu, p_slr)))
    {
        /*zero_struct_ptr(p_data_ref);*/
        p_data_ref->data_space = DATA_NONE;
        return(NULL);
    }

    /* Ok we have a valid database table, fill in the type and the owning database id */
    p_data_ref->data_space = DATA_DB_FIELD;
    p_data_ref->arg.db_field.projector_type = p_rec_projector->projector_type;
    p_data_ref->arg.db_field.db_id = p_rec_projector->opendb.db.id;

    /* Now, dependant on the format of the projector fill in the remaining two fields ie record number and field id */
    if(PROJECTOR_TYPE_CARD == p_rec_projector->projector_type)
    {
        S32 n_fields = array_elements(&p_rec_projector->opendb.table.h_fielddefs);

        if( fieldnumber > n_fields)
            fieldnumber = n_fields;

        p_data_ref->arg.db_field.field_id = array_ptrc(&p_rec_projector->opendb.table.h_fielddefs, FIELDDEF, (fieldnumber-1))->id;
    }
    else
    {
        p_data_ref->arg.db_field.field_id = field_id_from_col(p_slr, p_rec_projector); /* What about the column? */

        if(p_data_ref->arg.db_field.field_id == (FIELD_ID) (-1))
            p_data_ref->data_space = DATA_NONE;
    }

    p_data_ref->arg.db_field.record = record_number_from_row(p_slr, p_rec_projector);

    return(p_rec_projector);
}

/******************************************************************************
*
* a front-end to the data_ref builder
*
******************************************************************************/

extern P_REC_PROJECTOR
rec_data_ref_from_slr(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_SLR p_slr,
    _OutRef_    P_DATA_REF p_data_ref)
{
    /* The "1" as the last argument here results in the data_ref pointing at the first field if it is
       a card view. Because this is also used elsewhere the slr->records-type-data_ref conversion
       is consistent and so the sheet-in-cells-editing can be applied to the card view to edit the
       first field.
    */
    return(old_make_data_ref(p_docu, p_slr, p_data_ref, 1));
}

extern P_REC_PROJECTOR
rec_data_ref_from_slr_and_fn(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_SLR p_slr,
    _In_        S32 fieldnumber,
    _OutRef_    P_DATA_REF p_data_ref)
{
    /* The last argument here is used to get to a specific numbered field or the LAST field by setting fieldnumber to 7FFFFFFF
       (eg by ed_rec when finding the last field in a card )
    */
    return(old_make_data_ref(p_docu, p_slr, p_data_ref, fieldnumber));
}

/******************************************************************************
*
* Use the skel point to choose a
* frame or a title
* if PROJECTOR_TYPE_CARD
*
******************************************************************************/

extern P_REC_PROJECTOR
rec_data_ref_from_slr_and_skel_point(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_SLR p_slr,
    _InRef_     PC_SKEL_POINT p_skel_point,
    _OutRef_    P_DATA_REF p_data_ref)
{
    P_REC_PROJECTOR p_rec_projector;

    p_data_ref->data_space = DATA_NONE;

    if(NULL == (p_rec_projector = p_rec_projector_from_slr(p_docu, p_slr)))
        return(NULL);

    p_data_ref->arg.db_field.projector_type = p_rec_projector->projector_type; /* same offset in all types of DATA_DATABASE_* */

    if(PROJECTOR_TYPE_CARD != p_rec_projector->projector_type)
    {
        p_data_ref->data_space = DATA_DB_FIELD;
        p_data_ref->arg.db_title.db_id    = p_rec_projector->opendb.db.id;
        p_data_ref->arg.db_field.record   = record_number_from_row(p_slr, p_rec_projector);
        p_data_ref->arg.db_field.field_id = field_id_from_col(p_slr, p_rec_projector);

        if(p_data_ref->arg.db_field.field_id == (FIELD_ID) -1)
        {
            p_data_ref->data_space = DATA_NONE;
            return(NULL);
        }

        return(p_rec_projector);
    }

    {
    S32 record = record_number_from_row(p_slr, p_rec_projector);
    PIXIT_POINT pixit_point;
    SKEL_RECT skel_rect;
    ARRAY_INDEX i;

    skel_rect_from_slr(p_docu, &skel_rect, p_slr); /* Find the basic skel_rect */

    pixit_point.x = p_skel_point->pixit_point.x - skel_rect.tl.pixit_point.x;
    pixit_point.y = p_skel_point->pixit_point.y - skel_rect.tl.pixit_point.y;

    /* Now use the skel_point to locate a field or title. SKS 25jul95 deprocedurized and made to loop from front to back */
    i = array_elements(&p_rec_projector->h_rec_frames);

    while(--i >= 0)
    {
        P_REC_FRAME p_rec_frame = array_ptr(&p_rec_projector->h_rec_frames, REC_FRAME, i);

        if(rec_pixit_point_in_pixit_rect(&pixit_point, &p_rec_frame->pixit_rect_field))
        {
            p_data_ref->data_space = DATA_DB_FIELD;
            p_data_ref->arg.db_title.db_id    = p_rec_projector->opendb.db.id;
            p_data_ref->arg.db_field.record   = record;
            p_data_ref->arg.db_field.field_id = p_rec_frame->field_id;
            return(p_rec_projector);
        }

        if(p_rec_frame->title_show && rec_pixit_point_in_pixit_rect(&pixit_point, &p_rec_frame->pixit_rect_title))
        {
            p_data_ref->data_space = DATA_DB_TITLE;
            p_data_ref->arg.db_title.db_id    = p_rec_projector->opendb.db.id;
            p_data_ref->arg.db_title.record   = record;
            p_data_ref->arg.db_title.field_id = p_rec_frame->field_id;
            return(p_rec_projector);
        }
    }
    } /*block*/

    p_data_ref->data_space = DATA_NONE;
    return(NULL);
}

/* This returns a pointer to an array element and so should only be used with caution in case things move */

extern P_REC_FRAME
p_rec_frame_from_field_id(
    P_ARRAY_HANDLE p_array_handle /*REC_FRAME[]*/,
    _In_        FIELD_ID field_id)
{
    ARRAY_INDEX i = array_elements(p_array_handle);

    while(--i >= 0)
    {
        P_REC_FRAME p_rec_frame = array_ptr(p_array_handle, REC_FRAME, i);

        if(p_rec_frame->field_id == field_id)
            return(p_rec_frame);
    }

    return(P_DATA_NONE);
}

/******************************************************************************
*
* rec_slr_from_data_ref
*
* For those occasion where you need to
* find style using an slr etc
*
******************************************************************************/

_Check_return_ _Success_(status_ok(return))
extern STATUS
rec_slr_from_data_ref(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DATA_REF p_data_ref,
    _OutRef_    P_SLR p_slr)
{
    STATUS status;
    P_REC_PROJECTOR p_rec_projector = p_rec_projector_from_data_ref(p_docu, p_data_ref);

    if(NULL == p_rec_projector)
        return(STATUS_FAIL);

    *p_slr = p_rec_projector->rec_docu_area.tl.slr; /* Start it off with the top left of the projector */

    switch(p_rec_projector->projector_type)
    {
    default:
        p_slr->row = -1;
        p_slr->col = -1;
        status = STATUS_FAIL;
        break;

    case PROJECTOR_TYPE_SHEET:
        {
        FIELD_ID field_id = field_id_from_rec_data_ref(p_data_ref);
        ARRAY_INDEX i;

        status = STATUS_FAIL;

        for(i = 0; i < array_elements(&p_rec_projector->h_rec_frames); i++)
        {
            P_REC_FRAME p_rec_frame = array_ptr(&p_rec_projector->h_rec_frames, REC_FRAME, i);

            if(p_rec_frame->field_id == field_id)
            {
                p_slr->col += (COL) i;
                status = STATUS_OK;
                break;
            }
        }

        if(status_ok(status))
            p_slr->row += ((S32) get_record_from_rec_data_ref(p_data_ref)) - p_rec_projector->start_offset;

        break;
        }

    case PROJECTOR_TYPE_CARD:
        {
        S32 cards_across = p_rec_projector->rec_docu_area.br.slr.col - p_rec_projector->rec_docu_area.tl.slr.col;
        S32 record = get_record_from_rec_data_ref(p_data_ref);

        p_slr->row -= p_rec_projector->start_offset ; /* scroll offset is measured in rows not records so no need to allow for cards_across */
        p_slr->row += record / cards_across;

        p_slr->col += (COL) (record % cards_across);

        myassert2x(p_slr->row < n_rows(p_docu),         TEXT("Row calculation result out of range ") ROW_TFMT TEXT(" ") ROW_TFMT, p_slr->row, n_rows(p_docu));
        myassert2x(p_slr->col < n_cols_logical(p_docu), TEXT("Col calculation result out of range ") COL_TFMT TEXT(" ") COL_TFMT, p_slr->col, n_cols_logical(p_docu));

        status = STATUS_OK;

        break;
        }
    }

    return(status);
}

/* This locates the real SKEL_RECT for a data_ref in a docu.
   It locates the skel_rect for the slr and then adjusts it for the
   pixit_position within the slr of the data_ref-ed item.
*/

_Check_return_ _Success_(status_ok(return))
extern STATUS
rec_skel_rect_from_data_ref(
    P_REC_PROJECTOR p_rec_projector,
    _OutRef_    P_SKEL_RECT p_skel_rect_out,
    _InRef_     PC_DATA_REF p_data_ref)
{
    const P_DOCU p_docu = p_docu_from_docno(p_rec_projector->docno);
    REC_FRAME rec_frame;
    SLR slr;
    STATUS status = STATUS_OK;

    switch(p_data_ref->data_space)
    {
    default: /*default_unhandled();*/
        status = STATUS_FAIL;
        break;

    case DATA_SLOT:
        assert0();
        skel_rect_from_slr(p_docu, p_skel_rect_out, &p_data_ref->arg.slr);
        break;

    case DATA_DB_FIELD:
        status_assert(rec_slr_from_data_ref(p_docu, p_data_ref, &slr));

        skel_rect_from_slr(p_docu, p_skel_rect_out, &slr);

        if(PROJECTOR_TYPE_CARD == p_rec_projector->projector_type)
        {
            if(status_ok(status = get_frame_by_field_id(p_rec_projector, p_data_ref->arg.db_field.field_id, &rec_frame)))
            {
                p_skel_rect_out->br.pixit_point.x = p_skel_rect_out->tl.pixit_point.x + rec_frame.pixit_rect_field.br.x;
                p_skel_rect_out->br.pixit_point.y = p_skel_rect_out->tl.pixit_point.y + rec_frame.pixit_rect_field.br.y;
                p_skel_rect_out->tl.pixit_point.x += rec_frame.pixit_rect_field.tl.x;
                p_skel_rect_out->tl.pixit_point.y += rec_frame.pixit_rect_field.tl.y;
            }
        }

        break;

    case DATA_DB_TITLE:
        status_assert(rec_slr_from_data_ref(p_docu, p_data_ref, &slr));

        skel_rect_from_slr(p_docu, p_skel_rect_out, &slr);

        if(status_ok(status = get_frame_by_field_id(p_rec_projector, p_data_ref->arg.db_title.field_id, &rec_frame)))
        {
            p_skel_rect_out->br.pixit_point.x = p_skel_rect_out->tl.pixit_point.x + rec_frame.pixit_rect_title.br.x;
            p_skel_rect_out->br.pixit_point.y = p_skel_rect_out->tl.pixit_point.y + rec_frame.pixit_rect_title.br.y;
            p_skel_rect_out->tl.pixit_point.x += rec_frame.pixit_rect_title.tl.x;
            p_skel_rect_out->tl.pixit_point.y += rec_frame.pixit_rect_title.tl.y;
        }

        break;
    }

    return(status);
}

/* Some routines to mismagle stuff out of one sort of data_ref or me other */

_Check_return_
extern PROJECTOR_TYPE
projector_type_from_rec_data_ref(
    _InRef_     PC_DATA_REF p_data_ref)
{
    switch(p_data_ref->data_space)
    {
    case DATA_DB_TITLE:
        return(p_data_ref->arg.db_title.projector_type);

    case DATA_DB_FIELD:
        return(p_data_ref->arg.db_field.projector_type);

    default:
        myassert1(TEXT("Bad data_ref ") S32_TFMT, p_data_ref->data_space);
        return(PROJECTOR_TYPE_ERROR);
    }
}

extern void
set_rec_object_position_from_data_ref(
    _DocuRef_   P_DOCU p_docu,
    P_OBJECT_POSITION p_object_position,
    _InRef_     PC_DATA_REF p_data_ref)
{
    set_rec_object_position_data_space  (p_object_position, p_data_ref->data_space);
    set_rec_object_position_field_number(p_object_position, fieldnumber_from_rec_data_ref(p_docu, p_data_ref));
}

extern void
set_rec_data_ref_from_object_position(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_DATA_REF p_data_ref,
    _InRef_     PC_OBJECT_POSITION p_object_position)
{
    myassert1x( (p_data_ref->data_space == DATA_DB_FIELD ) || (p_data_ref->data_space == DATA_DB_TITLE), TEXT("Invalid data_space for rec_data_ref") S32_TFMT, p_data_ref->data_space);

    set_rec_data_ref_field_by_number(p_docu, p_data_ref, get_rec_object_position_field_number(p_object_position));
    p_data_ref->data_space = get_rec_object_position_data_space(p_object_position);
}

extern void
rec_object_position_copy(
    _InoutRef_  P_OBJECT_POSITION p_object_position_dst,
    _InRef_     PC_OBJECT_POSITION p_object_position_src)
{
    myassert1x(OBJECT_ID_REC == p_object_position_src->object_id,  TEXT(" Wrong object id ") S32_TFMT, p_object_position_src->object_id);
    p_object_position_dst->more_data = p_object_position_src->more_data;
}

_Check_return_
extern S32
rec_object_position_compare(
    _InRef_     PC_OBJECT_POSITION p_object_position_first,
    _InRef_     PC_OBJECT_POSITION p_object_position_second)
{
    S32 result;

    myassert1x(OBJECT_ID_REC == p_object_position_first->object_id,  TEXT(" Wrong object id ") S32_TFMT, p_object_position_first->object_id);
    myassert1x(OBJECT_ID_REC == p_object_position_second->object_id, TEXT(" Wrong object id ") S32_TFMT, p_object_position_second->object_id);

    /* The comparison must test for in order
       the field-ordinal ... to be replaced by comparison of true visible x,y position... and
       the data_space and
       the textual offset

       result is  0 if the same
                 +1 if first > second
                 -1 if first < second
    */

    result = get_rec_object_position_field_number(p_object_position_first) - get_rec_object_position_field_number(p_object_position_second);

    if(result == 0)
    {
        result = (S32) get_rec_object_position_data_space(p_object_position_first) - (S32) get_rec_object_position_data_space(p_object_position_second );

        if(result == 0 )
        {
            result =  p_object_position_first->data - p_object_position_second->data;
        }
    }

    if(result < 0) result = -1;
    if(result > 0) result =  1;
    return result;
}

extern S32
rec_object_position_at_start(
    _InRef_     PC_OBJECT_POSITION p_object_position)
{
    myassert1x(OBJECT_ID_REC == p_object_position->object_id, TEXT(" Wrong object id ") S32_TFMT, p_object_position->object_id);

    /* Insist it is the first field, titles don't come first ? */
    if( (get_rec_object_position_field_number(p_object_position) == 0) &&
         (get_rec_object_position_data_space(p_object_position) == DATA_DB_FIELD ) )
    {
        return(0 == p_object_position->data);
    }

    return FALSE;
}

extern void
rec_object_position_set_start(
    P_OBJECT_POSITION p_object_position)
{
    myassert1x(OBJECT_ID_REC == p_object_position->object_id, TEXT(" Wrong object id ") S32_TFMT, p_object_position->object_id);

    set_rec_object_position_data_space(p_object_position, DATA_DB_FIELD);
    set_rec_object_position_field_number(p_object_position, 0);
    p_object_position->data = 0;
}

extern void
set_rec_object_position_data_space(
    P_OBJECT_POSITION p_object_position,
    _In_        DATA_SPACE data_space)
{
    switch(p_object_position->object_id)
    {
    case OBJECT_ID_NONE:
        p_object_position->object_id = OBJECT_ID_REC;
        p_object_position->data = 0;

        /*FALLTHRU*/

    case OBJECT_ID_REC:
        p_object_position->more_data = (p_object_position->more_data & 0x0000FFFF) | (((U32) data_space << 16) & 0xFFFF0000);
        break;

    default:
        myassert1(TEXT("Very Bad news : Object Id in position is wrong ") S32_TFMT, p_object_position->object_id);
        break;
    }
}

extern DATA_SPACE
get_rec_object_position_data_space(
    _InRef_     PC_OBJECT_POSITION p_object_position)
{
    DATA_SPACE data_space = DATA_NONE;

    switch(p_object_position->object_id)
    {
    case OBJECT_ID_REC:
        data_space = (DATA_SPACE) (p_object_position->more_data >> 16);
        break;

    default:
        myassert1(TEXT("Very Bad news : Object Id in position is wrong ") S32_TFMT, p_object_position->object_id);
        break;
    }

    return data_space;
}

extern void
set_rec_object_position_field_number(
    P_OBJECT_POSITION p_object_position,
    _In_        S32 fieldnumber)
{
    switch(p_object_position->object_id)
    {
    case OBJECT_ID_NONE:
        p_object_position->object_id = OBJECT_ID_REC;
        p_object_position->data = 0;

        /*FALLTHRU*/

    case OBJECT_ID_REC:
        p_object_position->more_data = (p_object_position->more_data & 0xFFFF0000) | (fieldnumber & 0x0000FFFF);
        break;

    default:
        myassert1(TEXT("Object Id in position is wrong ") S32_TFMT, p_object_position->object_id);
        break;
    }
}

extern S32
get_rec_object_position_field_number(
    _InRef_     PC_OBJECT_POSITION p_object_position)
{
    S32 fieldnumber = -1;

    switch(p_object_position->object_id)
    {
    case OBJECT_ID_REC:
        fieldnumber = * (PC_S16) &p_object_position->more_data;
#if CHECKING
        if(p_object_position->more_data & 0x8000)
        {
            myassert1x(fieldnumber < 0, TEXT("Sign extension failed ") S32_TFMT, fieldnumber);
        }
#endif
        break;

    default:
        myassert1(TEXT("Object Id in position is wrong ") S32_TFMT, p_object_position->object_id);
        break;
    }

    return fieldnumber;
}

_Check_return_
extern STATUS
set_rec_data_ref_field_by_number(
    _DocuRef_   P_DOCU p_docu,
    _InoutRef_  P_DATA_REF p_data_ref,
    _In_        S32 fieldnumber)
{
    STATUS status;
    P_FIELDDEF p_fielddef;
    P_REC_PROJECTOR p_rec_projector;

    myassert1x( (p_data_ref->data_space == DATA_DB_FIELD ) || (p_data_ref->data_space == DATA_DB_TITLE), TEXT("Invalid data_space for rec_data_ref") S32_TFMT, p_data_ref->data_space);

    p_rec_projector = p_rec_projector_from_data_ref(p_docu, p_data_ref);
    if(NULL == p_rec_projector)
        return STATUS_FAIL;

    if(P_DATA_NONE != (p_fielddef = p_fielddef_from_number(&p_rec_projector->opendb.table.h_fielddefs, fieldnumber)))
    {
        status = STATUS_OK;
        set_field_id_for_rec_data_ref(p_data_ref, p_fielddef->id);
    }
    else
        status = STATUS_FAIL;

    return status;
}

_Check_return_
extern S32
fieldnumber_from_rec_data_ref(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DATA_REF p_data_ref)
{
    P_REC_PROJECTOR p_rec_projector = p_rec_projector_from_data_ref(p_docu, p_data_ref);

    if(NULL != p_rec_projector)
        return(fieldnumber_from_field_id(&p_rec_projector->opendb.table, field_id_from_rec_data_ref(p_data_ref)));

    return(-1);
}

/* Some routines to mismagle stuff out of one sort of data_ref or me other */

extern void
set_field_id_for_rec_data_ref(
    _InoutRef_  P_DATA_REF p_data_ref,
    _In_        FIELD_ID field_id)
{
    switch(p_data_ref->data_space)
    {
    case DATA_DB_TITLE:
        p_data_ref->arg.db_title.field_id = field_id;
        break;

    case DATA_DB_FIELD:
        p_data_ref->arg.db_field.field_id = field_id;
        break;

    default:
        myassert1(TEXT("Bad data_ref ") S32_TFMT, p_data_ref->data_space);
        break;
    }
}

_Check_return_
extern FIELD_ID
field_id_from_rec_data_ref(
    _InRef_     PC_DATA_REF p_data_ref)
{
    switch(p_data_ref->data_space)
    {
    case DATA_DB_TITLE:
        return(p_data_ref->arg.db_title.field_id);

    case DATA_DB_FIELD:
        return(p_data_ref->arg.db_field.field_id);

    default:
        myassert1(TEXT("Bad data_ref ") S32_TFMT, p_data_ref->data_space);
        return(0xFFFFFFFFU /* wot - not FIELD_ID_BAD? */);
    }
}

_Check_return_
extern DB_ID
db_id_from_rec_data_ref(
    _InRef_     PC_DATA_REF p_data_ref)
{
    switch(p_data_ref->data_space)
    {
    case DATA_DB_TITLE:
        return(p_data_ref->arg.db_title.db_id);

    case DATA_DB_FIELD:
        return(p_data_ref->arg.db_field.db_id);

    default:
        myassert1(TEXT("Bad data_ref ") S32_TFMT, p_data_ref->data_space);
        return(DB_ID_BAD);
    }
}

extern void
set_db_id_for_rec_data_ref(
    _InoutRef_  P_DATA_REF p_data_ref,
    _InVal_     DB_ID db_id)
{
    switch(p_data_ref->data_space)
    {
    case DATA_DB_TITLE:
        p_data_ref->arg.db_title.db_id = db_id;
        break;

    case DATA_DB_FIELD:
        p_data_ref->arg.db_field.db_id = db_id;
        break;

    default:
        myassert1(TEXT("Bad data_ref ") S32_TFMT, p_data_ref->data_space);
        break;
    }
}

extern void
set_record_for_rec_data_ref(
    _InoutRef_  P_DATA_REF p_data_ref,
    _InVal_     S32 record)
{
    switch(p_data_ref->data_space)
    {
    case DATA_DB_TITLE:
        p_data_ref->arg.db_title.record = record;
        break;

    case DATA_DB_FIELD:
        p_data_ref->arg.db_field.record = record;
        break;

    default:
        myassert1(TEXT("Bad data_ref ") S32_TFMT, p_data_ref->data_space);
        break;

    }
}

extern S32
get_record_from_rec_data_ref(
    _InRef_     PC_DATA_REF p_data_ref)
{
    switch(p_data_ref->data_space)
    {
    case DATA_DB_TITLE:
        return(p_data_ref->arg.db_title.record);

    case DATA_DB_FIELD:
        return(p_data_ref->arg.db_field.record);

    default:
        myassert1(TEXT("Bad data_ref ") S32_TFMT, p_data_ref->data_space);
        return(0);
    }
}

/* Extract all the info from a Recordz data_ref and make a field copy of it */

extern void
field_data_ref_from_rec_data_ref(
    _OutRef_    P_DATA_REF p_data_ref,
    _InRef_     PC_DATA_REF p_data_ref_in)
{
    p_data_ref->data_space = DATA_DB_FIELD;
    p_data_ref->arg.db_title.projector_type = projector_type_from_rec_data_ref(p_data_ref_in);
    set_db_id_for_rec_data_ref(p_data_ref, db_id_from_rec_data_ref(p_data_ref_in));
    set_record_for_rec_data_ref(p_data_ref, get_record_from_rec_data_ref(p_data_ref_in));
    set_field_id_for_rec_data_ref(p_data_ref, field_id_from_rec_data_ref(p_data_ref_in));
}

/* Extract all the info from a Recordz data_ref and make a title field copy of it */

extern void
title_data_ref_from_rec_data_ref(
    _OutRef_    P_DATA_REF p_data_ref,
    _InRef_     PC_DATA_REF p_data_ref_in)
{
    p_data_ref->data_space = DATA_DB_TITLE;
    p_data_ref->arg.db_title.projector_type = projector_type_from_rec_data_ref(p_data_ref_in);
    set_db_id_for_rec_data_ref(p_data_ref, db_id_from_rec_data_ref(p_data_ref_in));
    set_record_for_rec_data_ref(p_data_ref, get_record_from_rec_data_ref(p_data_ref_in));
    set_field_id_for_rec_data_ref(p_data_ref, field_id_from_rec_data_ref(p_data_ref_in));
}

extern void
rec_style_for_data_ref(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DATA_REF p_data_ref,
    _OutRef_    P_STYLE p_style,
    _InRef_     PC_STYLE_SELECTOR p_style_selector)
{
    /* set up effects of which we want details */
    style_init(p_style);

    switch(p_data_ref->data_space)
    {
    case DATA_DB_TITLE:
    case DATA_DB_FIELD:
        {
        POSITION position;
        status_break(rec_slr_from_data_ref(p_docu, p_data_ref, &position.slr));
        position.object_position.object_id = OBJECT_ID_REC;

        set_rec_object_position_from_data_ref(p_docu, &position.object_position, p_data_ref);

        style_from_position(p_docu,
                            p_style,
                            p_style_selector,
                            &position,
                            &p_docu->h_style_docu_area,
                            position_in_docu_area,
                            FALSE);
         break;
         }

     case DATA_SLOT:
         style_from_slr(p_docu, p_style, p_style_selector, &p_data_ref->arg.slr);
         break;

     default:
         myassert1(TEXT("Bad data_ref ") S32_TFMT, p_data_ref->data_space);
         break;
    }
}

_Check_return_
extern P_FIELDDEF
p_fielddef_from_data_ref(
    _DocuRef_   P_DOCU p_docu,
    _InRef_     PC_DATA_REF p_data_ref)
{
    P_REC_PROJECTOR p_rec_projector = p_rec_projector_from_data_ref(p_docu, p_data_ref);

    switch(p_data_ref->data_space)
    {
    case DATA_DB_FIELD:
    case DATA_DB_TITLE:
        if(NULL != p_rec_projector)
            return(p_fielddef_from_field_id(&p_rec_projector->opendb.table.h_fielddefs, field_id_from_rec_data_ref(p_data_ref)));
         break;

    default:
        myassert1x(p_data_ref->data_space == DATA_DB_FIELD, TEXT("Not DATA_DB_FIELD ") S32_TFMT, p_data_ref->data_space);
        break;
    }

    return(P_DATA_NONE);
}

extern P_FIELDDEF
p_fielddef_from_field_id(
    _InRef_     PC_ARRAY_HANDLE p_array_handle /*FIELDDEF[]*/,
    _In_        FIELD_ID field_id)
{
    ARRAY_INDEX i = array_elements(p_array_handle);

    while(--i >= 0)
    {
        P_FIELDDEF p_fielddef = array_ptr(p_array_handle, FIELDDEF, i);

        if(p_fielddef->id == field_id)
            return(p_fielddef);
    }

    return(P_DATA_NONE);
}

extern P_FIELDDEF
p_fielddef_from_name(
    _InRef_     PC_ARRAY_HANDLE p_array_handle /*FIELDDEF[]*/,
    _In_z_      PC_U8Z p_field_name)
{
    ARRAY_INDEX i = array_elements(p_array_handle);

    while(--i >= 0)
    {
        P_FIELDDEF p_fielddef = array_ptr(p_array_handle, FIELDDEF, i);

        if(0 == tstricmp(p_field_name, p_fielddef->name))
            return(p_fielddef);
    }

    return(P_DATA_NONE);
}

_Check_return_
_Ret_maybenone_
extern P_FIELDDEF
p_fielddef_from_number(
    _InRef_     PC_ARRAY_HANDLE p_array_handle /*FIELDDEF[]*/,
    _InVal_     S32 nth_field)
{
    const S32 field_idx = nth_field - 1;

    if(!array_index_valid(p_array_handle, field_idx))
        return(P_DATA_NONE);

    return(array_ptr_no_checks(p_array_handle, FIELDDEF, field_idx));
}

/* Note that this is really operating on pixit points !!! */

_Check_return_
extern BOOL
rec_pixit_point_in_pixit_rect(
    _InRef_     PC_PIXIT_POINT p_pixit_point,
    _InRef_     PC_PIXIT_RECT p_pixit_rect)
{
    if(p_pixit_rect->tl.x > p_pixit_point->x)
        return(FALSE); /* no overlap */
    if(p_pixit_rect->tl.y > p_pixit_point->y)
        return(FALSE); /* no overlap */

    if(p_pixit_rect->br.x <= p_pixit_point->x)
        return(FALSE); /* no overlap */
    if(p_pixit_rect->br.y <= p_pixit_point->y)
        return(FALSE); /* no overlap */

    return(TRUE);
}

static S32
record_number_from_row(
    _InRef_     PC_SLR p_slr,
    P_REC_PROJECTOR p_rec_projector)
{
    S32 record;

    switch(p_rec_projector->projector_type)
    {
    case PROJECTOR_TYPE_CARD:
    {
        S32 cards_across, rows_down; /* We need to calculate a record number using both row & column info */

        cards_across = p_rec_projector->rec_docu_area.br.slr.col - p_rec_projector->rec_docu_area.tl.slr.col;

        rows_down    = p_slr->row - p_rec_projector->rec_docu_area.tl.slr.row;

        rows_down   += p_rec_projector->start_offset;

        record = (rows_down * cards_across) +   p_slr->col - p_rec_projector->rec_docu_area.tl.slr.col;
        break;
    }

    default:
    case PROJECTOR_TYPE_SHEET:
        record   = p_slr->row - p_rec_projector->rec_docu_area.tl.slr.row ; /* Adjust the base row position */

        record  += p_rec_projector->start_offset;

        break;
    }

    return record;
}

_Check_return_
extern STATUS
get_card_size(
    P_REC_PROJECTOR p_rec_projector,
    /*out*/ P_PIXIT_POINT p_pixit_point)
{
    ARRAY_INDEX i;

    if(NULL == p_rec_projector)
        return STATUS_FAIL;

    myassert1x(p_rec_projector != NULL, TEXT("Bad rec projector ") PTR_TFMT, p_rec_projector);

    assert(p_rec_projector->projector_type == PROJECTOR_TYPE_CARD);

    /* Set up some minimum values */
    p_pixit_point->x = MINIMUM_CARD_WIDTH;
    p_pixit_point->y = MINIMUM_CARD_HEIGHT;

    /* Look at all the fields and find the maximum extents of x and y */
    for(i = 0; i < array_elements(&p_rec_projector->h_rec_frames); i++)
    {
        P_REC_FRAME p_rec_frame = array_ptr(&p_rec_projector->h_rec_frames, REC_FRAME, i);
        PC_FIELDDEF p_fielddef = p_fielddef_from_field_id(&p_rec_projector->opendb.table.h_fielddefs, p_rec_frame->field_id);

        if(P_DATA_NONE == p_fielddef)
            break;

        if(strlen(p_fielddef->value_list))
            p_pixit_point->x = MAX(p_pixit_point->x, p_rec_frame->pixit_rect_field.br.x + DEFAULT_GRIGHT_SIZE);
        else
            p_pixit_point->x = MAX(p_pixit_point->x, p_rec_frame->pixit_rect_field.br.x);

        p_pixit_point->y = MAX(p_pixit_point->y, p_rec_frame->pixit_rect_field.br.y);

        /* Also take into account the titles */
        p_pixit_point->x = MAX(p_pixit_point->x, p_rec_frame->pixit_rect_title.br.x);
        p_pixit_point->y = MAX(p_pixit_point->y, p_rec_frame->pixit_rect_title.br.y);
    }

    p_pixit_point->x += REC_DEFAULT_FRAME_RM;
    p_pixit_point->y += REC_DEFAULT_FRAME_BM;

    {
    const PC_DOCU p_docu = p_docu_from_docno(p_rec_projector->docno);
    p_pixit_point->x = skel_ruler_snap_to_click_stop(p_docu, 1, p_pixit_point->x, SNAP_TO_CLICK_STOP_ROUND);
    p_pixit_point->y = skel_ruler_snap_to_click_stop(p_docu, 0, p_pixit_point->y, SNAP_TO_CLICK_STOP_ROUND);
    } /*block*/

    return(STATUS_OK);
}

_Check_return_
extern STATUS
get_frame_by_field_id(
    P_REC_PROJECTOR p_rec_projector,
    _In_        FIELD_ID fid,
    P_REC_FRAME p_rec_output)
{
    ARRAY_INDEX i;

    profile_ensure_frame();

    for(i = 0; i < array_elements(&p_rec_projector->h_rec_frames); i++)
    {
        P_REC_FRAME p_rec_frame = array_ptr(&p_rec_projector->h_rec_frames, REC_FRAME, i);

        if(p_rec_frame->field_id == fid )
        {
            *p_rec_output = *p_rec_frame;
            return(STATUS_OK);
        }
    }

    return(STATUS_FAIL);
}

static FIELD_ID
field_id_from_col(
    _InRef_     PC_SLR p_slr,
    P_REC_PROJECTOR p_rec_projector)
{
    S32 col = p_slr->col - p_rec_projector->rec_docu_area.tl.slr.col;

    profile_ensure_frame();

    if(array_index_valid(&p_rec_projector->h_rec_frames, col))
    {
        P_REC_FRAME p_rec_frame = array_ptr_no_checks(&p_rec_projector->h_rec_frames, REC_FRAME, col);
        return(p_rec_frame->field_id);
    }

  return((FIELD_ID)-1);
}

/******************************************************************************
*
* calculate pixit size of object
*
******************************************************************************/

_Check_return_
extern STATUS
rec_object_pixit_size(
    _DocuRef_   P_DOCU p_docu,
    /*out*/ P_PIXIT_POINT p_pixit_point,
    P_SLR p_slr,
    _InRef_opt_ PC_STYLE p_style_in)
{
    PC_STYLE p_style = p_style_in;
    STYLE style;
    DATA_REF data_ref;
    P_REC_PROJECTOR p_rec_projector;

    if(NULL == (p_rec_projector = rec_data_ref_from_slr(p_docu, p_slr, &data_ref)))
        return(create_error(REC_ERR_NO_DATABASE));

    if(PROJECTOR_TYPE_CARD == p_rec_projector->projector_type)
    {
        *p_pixit_point = p_rec_projector->card_size;
        return(STATUS_OK);
    }

    if(IS_P_STYLE_NONE(p_style))
    {
        STYLE_SELECTOR selector;

        style_selector_copy(&selector, &style_selector_para_leading);
        style_selector_bit_set(&selector, STYLE_SW_CS_WIDTH);
        style_selector_bit_set(&selector, STYLE_SW_PS_PARA_START);
        style_selector_bit_set(&selector, STYLE_SW_PS_PARA_END);

        /* find out size of cell */
        style_init(&style);
        style_from_slr(p_docu, &style, &selector, p_slr);
        p_style = &style;
    }

    p_pixit_point->x = p_style->col_style.width;
    p_pixit_point->y = style_leading_from_style(p_style, &p_style->font_spec, p_docu->flags.draft_mode)
                     + p_style->para_style.para_start
                     + p_style->para_style.para_end;

    return(STATUS_OK);
}

extern void
rec_ext_style(
    _DocuRef_   P_DOCU p_docu,
    P_IMPLIED_STYLE_QUERY p_implied_style_query)
{
    PC_STYLE_DOCU_AREA p_style_docu_area = p_implied_style_query->p_style_docu_area;
    const PC_STYLE p_style_from = p_style_from_docu_area(p_docu, p_style_docu_area);
    P_STYLE p_style_to = p_implied_style_query->p_style;
    DATA_REF data_ref;
    P_REC_PROJECTOR p_rec_projector = rec_data_ref_from_slr(p_docu, &p_implied_style_query->position.slr, &data_ref);

    if(NULL == p_rec_projector)
    {
        /* SKS 25jun95 hide everything else that's in the rows under our control */
        if(style_bit_test(p_style_from, STYLE_SW_CS_WIDTH))
        {
            p_style_to->col_style.width = 0;
            style_bit_set(p_style_to, STYLE_SW_CS_WIDTH);
        }
    }
    else
    {
        if(style_bit_test(p_style_from, STYLE_SW_PS_NEW_OBJECT))
        {
            p_style_to->para_style.new_object = OBJECT_ID_REC;
            style_bit_set(p_style_to, STYLE_SW_PS_NEW_OBJECT);
        }

        switch(data_ref.arg.db_field.projector_type)
        {
        case PROJECTOR_TYPE_CARD:
            {
            BOOL do_cs_width  = style_bit_test(p_style_from, STYLE_SW_CS_WIDTH);
            BOOL do_rs_height = style_bit_test(p_style_from, STYLE_SW_RS_HEIGHT);

            if(do_cs_width || do_rs_height)
            {
                PIXIT_POINT pixit_point;

                if(status_ok(rec_object_pixit_size(p_docu, &pixit_point, &p_implied_style_query->position.slr, NULL)))
                {
                    if(do_cs_width)
                    {
                        p_style_to->col_style.width = pixit_point.x;
                        style_bit_set(p_style_to, STYLE_SW_CS_WIDTH);
                    }

                    if(do_rs_height)
                    {
                        p_style_to->row_style.height = pixit_point.y;
                        style_bit_set(p_style_to, STYLE_SW_RS_HEIGHT);
                    }
                }
            }

            break;
            }

        case PROJECTOR_TYPE_SHEET:
            {
            BOOL do_cs_width    = style_bit_test(p_style_from, STYLE_SW_CS_WIDTH);
            BOOL do_cs_col_name = style_bit_test(p_style_from, STYLE_SW_CS_COL_NAME);

            myassert1x(data_ref.data_space == DATA_DB_FIELD, TEXT("Not a DATA_DB_FIELD ") S32_TFMT, data_ref.data_space);

            if(do_cs_width || do_cs_col_name)
            {
                PC_REC_FRAME p_rec_frame = p_rec_frame_from_field_id(&p_rec_projector->h_rec_frames, data_ref.arg.db_field.field_id);

                if(do_cs_width && (P_DATA_NONE != p_rec_frame))
                {
                    if(data_ref.data_space == DATA_DB_TITLE)
                        p_style_to->col_style.width = p_rec_frame->pixit_rect_title.br.x - p_rec_frame->pixit_rect_title.tl.x;
                    else
                        p_style_to->col_style.width = p_rec_frame->field_width;

                    if(p_style_to->col_style.width <= 0)
                    {
                        PIXIT default_title_width = skel_ruler_snap_to_click_stop(p_docu, 1, REC_DEFAULT_TITLE_FRAME_W, SNAP_TO_CLICK_STOP_ROUND);
                        p_style_to->col_style.width = MAX(default_title_width, p_style_to->col_style.width);
                    }

                    style_bit_set(p_style_to, STYLE_SW_CS_WIDTH);
                }

                if(do_cs_col_name && (P_DATA_NONE != p_rec_frame))
                {
                    p_style_to->col_style.h_numform = p_rec_frame->h_title_text_ustr; /* update the name of the column */
                    style_bit_set(p_style_to, STYLE_SW_CS_COL_NAME);
                }
            }

            break;
            }
        }
    }
}

_Check_return_
extern STATUS
rec_ext_style_fields(
    _DocuRef_   P_DOCU p_docu,
    P_IMPLIED_STYLE_QUERY p_implied_style_query)
{
    DATA_REF data_ref;
    P_REC_PROJECTOR p_rec_projector = rec_data_ref_from_slr(p_docu, &p_implied_style_query->position.slr, &data_ref);

    if(NULL != p_rec_projector)
    {
        BOOL use_style = FALSE;

        switch(p_implied_style_query->arg)
        {
        default: default_unhandled(); break;

        case DB_IMPLIED_ARG_DATABASE:
            use_style = TRUE;
            break;

        case DB_IMPLIED_ARG_ALL_FIELDS:
            if(OBJECT_ID_REC == p_implied_style_query->position.object_position.object_id)
            {
                DATA_SPACE data_space = get_rec_object_position_data_space(&p_implied_style_query->position.object_position);
                use_style = (data_space == DATA_DB_FIELD);
            }
            break;

        case DB_IMPLIED_ARG_ALL_TITLES:
            if(OBJECT_ID_REC == p_implied_style_query->position.object_position.object_id)
            {
                DATA_SPACE data_space = get_rec_object_position_data_space(&p_implied_style_query->position.object_position);
                use_style = (data_space == DATA_DB_TITLE);
            }
            break;

        case DB_IMPLIED_ARG_FIELD:
            if(OBJECT_ID_REC == p_implied_style_query->position.object_position.object_id)
            {
                set_rec_data_ref_from_object_position(p_docu, &data_ref, &p_implied_style_query->position.object_position);

                if(data_ref.data_space == DATA_DB_FIELD)
                {
                    P_FIELDDEF p_fielddef = p_fielddef_from_field_id(&p_rec_projector->opendb.table.h_fielddefs, data_ref.arg.db_field.field_id);

                    if(P_DATA_NONE != p_fielddef)
                    {
                        const PC_STYLE p_style_from = p_style_from_docu_area(p_docu, p_implied_style_query->p_style_docu_area);
                        PCTSTR tstr_style_name = array_tstr(&p_style_from->h_style_name_tstr);
                        PCTSTR tstr_field_name;

                        if(*tstr_style_name++ != '\x03')
                            break;

                        tstr_field_name = tstrchr(tstr_style_name, '\x03');

                        if(!tstr_field_name)
                            break;

                        if(tstrnicmp(p_rec_projector->opendb.table.name, tstr_style_name, tstr_field_name - tstr_style_name))
                            break;

                        tstr_field_name++;

                        use_style = (0 == tstricmp(p_fielddef->name, tstr_field_name));
                    }
                }
            }
            break;
        }

        if(use_style)
        {
            const PC_STYLE p_style_from = p_style_from_docu_area(p_docu, p_implied_style_query->p_style_docu_area);
            PTR_ASSERT(p_style_from);
            if(NULL != p_style_from)
                style_copy(p_implied_style_query->p_style, p_style_from, &style_selector_all);
        }
    }

    return(STATUS_OK);
}

/* end of ob_rec2.c */
