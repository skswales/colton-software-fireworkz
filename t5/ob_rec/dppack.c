/* dppack.c */

/* Copyright (C) 1994-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Routines to output DP Chunky Packed */

/* PMF June 94 */

#include "common/gflags.h"

#include "ob_rec/ob_rec.h"

_Check_return_
static inline U32
align__4(
    _InVal_     U32 size)
{
    return((size + 3) & ~3);
}

_Check_return_
static STATUS
make_header_chunk(
    P_ARRAY_HANDLE p_array_handle,
    _In_        S32 first_frame,
    _In_        S32 n_flds)
{
    STATUS status;
    U32 size = align__4(sizeof32(framespackstr));
    framespackstr * p_header_chunk;

    if(NULL == (p_header_chunk = (framespackstr *) al_array_extend_by_BYTE(p_array_handle, size, PC_ARRAY_INIT_BLOCK_NONE, &status)))
        return(status);

    /* Fill it in */
    p_header_chunk->chunk.id     = chunk_Frames; /* chunk identifier    */
    p_header_chunk->chunk.offset = (int) size;   /* chunk size          */

    /* Set up a bounding box for the frames  */
    p_header_chunk->bbox.x0 = 0;
    p_header_chunk->bbox.x1 = DP_DEFAULT_FRAME_LHS + DP_DEFAULT_FRAME_WIDTH + DP_DEFAULT_FRAME_RHS;

    p_header_chunk->bbox.y0 = (int) (-((n_flds * DP_DEFAULT_FRAME_SPACING) + DP_DEFAULT_FRAME_TOP + DP_DEFAULT_FRAME_BOTTOM ) ); /* bottom */
    p_header_chunk->bbox.y1 = 0; /* Top (zero=top, below =>-ve */

    /* Neil says set them to zero */
    p_header_chunk->desc.dx  = 0; /* scroll position of frames*/;
    p_header_chunk->desc.dy  = 0;
    p_header_chunk->desc.scx = 0;
    p_header_chunk->desc.scy = 0;

    p_header_chunk->frames_bbox = p_header_chunk->bbox; /* Neil says make'em the same */

    /*   It's probably best to set the 'Snap to frames' bit in the layout
         flags, in case the user starts fiddling with the layout.
    */
    p_header_chunk->flags = frames_FrameSnap;

    p_header_chunk->background = DP_DEFAULT_BACKGROUND_COLOUR;

    p_header_chunk->head_entry = (frame_id) first_frame;                  /* identifier of first frame in entry order */

    return(status);
}

_Check_return_
static STATUS
make_frame_chunk(
    P_ARRAY_HANDLE p_array_handle,
    _In_        S32 frame_id, fieldptr p_field,
    _In_        S32 next_id,
    _In_        S32 title_id,
    _In_        S32 yth,
    P_U8 fieldnamebuffer)
{
    STATUS status;
    S32 name_len = strlen(fieldnamebuffer);
    U32 size = align__4(sizeof32(framepackstr) + name_len);
    framepackstr * p_frame_chunk;

    if(NULL == (p_frame_chunk = (framepackstr *) al_array_extend_by_BYTE(p_array_handle, size, PC_ARRAY_INIT_BLOCK_NONE, &status)))
        return(status);

    p_frame_chunk->chunk.id     = chunk_Frame;     /* chunk identifier */
    p_frame_chunk->chunk.offset = (int) size;      /* chunk size       */

    p_frame_chunk->id           = (int) frame_id;  /* identifier of this frame                 */
    p_frame_chunk->link_entry   = (int) next_id;   /* identifier of next frame in entry order  */
    p_frame_chunk->link_title   = (int) title_id;  /* identifier of title frame for this field */

    if(NULL != p_field)
    {
        p_frame_chunk->fieldid = field_getid(p_field); /* field this frame was originally linked to  */

        /*   Field body frames should have the following flags set:

                   frame_Editable
                   frame_DotBorder
                   frame_Resizable; unless it's a boolean or file field

        */
        p_frame_chunk->flags = (frameflags) (frame_Editable | frame_DotBorder);

        if( field_type(p_field) != FT_BOOL )
            p_frame_chunk->flags = (frameflags) (frame_Resizable | p_frame_chunk->flags);

        /* Set up a suitable frame bounding box  */

        p_frame_chunk->box.x0 = DP_DEFAULT_FRAME_LHS;
        p_frame_chunk->box.x1 = DP_DEFAULT_FRAME_WIDTH + p_frame_chunk->box.x0;

        p_frame_chunk->box.y0 = (int) (-DP_DEFAULT_FRAME_TOP - DP_DEFAULT_FRAME_SPACING * yth);
        p_frame_chunk->box.y1 = ((DP_DEFAULT_LINE_SPACE * 1) + DP_DEFAULT_FRAME_TM - DP_DEFAULT_FRAME_BM) + p_frame_chunk->box.y0; /* Times 1 for one line of text */

        /* filetype of frame contents  */
        p_frame_chunk->filetype = filetype_Text;
    }
    else
    { /* Title frames should have all frame flags clear. */
        p_frame_chunk->flags = (frameflags) 0;
        p_frame_chunk->fieldid      = NULL;

        /* Set up a suitable frame bounding box  */

        p_frame_chunk->box.x0 = DP_DEFAULT_TIT_LHS;
        p_frame_chunk->box.x1 = DP_DEFAULT_TIT_WIDTH + p_frame_chunk->box.x0;

        p_frame_chunk->box.y0 = (int) (-DP_DEFAULT_FRAME_TOP - DP_DEFAULT_FRAME_SPACING * yth);
        p_frame_chunk->box.y1 = ((DP_DEFAULT_LINE_SPACE * 1) + DP_DEFAULT_FRAME_TM - DP_DEFAULT_FRAME_BM) + p_frame_chunk->box.y0; /* Times 1 for one line of text */

        /* filetype of frame contents  */
        p_frame_chunk->filetype = filetype_Text;
    }

    p_frame_chunk->textbody_offset = 0;                    /* offset in the chunk of the text */
    p_frame_chunk->textbody_size   = 0;                    /* size in the chunk of the text   */

    p_frame_chunk->stype = stype_Field;

    {
    P_U8 p_u8 = (P_U8) p_frame_chunk + sizeof32(framepackstr);

    if(name_len == 0)
    {
        p_frame_chunk->textbody_offset = 0;                 /* offset in the chunk of the text */
        p_frame_chunk->textbody_size   = 0;                 /* size in the chunk of the text   */
    }
    else
    {
        p_frame_chunk->textbody_offset = (int) (((S32) p_u8)-((S32) p_frame_chunk)); /* offset in the chunk of the text */
        p_frame_chunk->textbody_size   = (int) name_len;                             /* size in the chunk of the text   */

        memcpy32(p_u8, fieldnamebuffer, name_len);
    }
    } /*block*/

    return(STATUS_OK);
}

static void
make_textstylepack(
    textstylepack * p_textstylepack,
    _InVal_     BOOL is_title,
    styletype stype)
{
    p_textstylepack->type     = display_Text;
    p_textstylepack->fontname = 0; /* is (and must be) filled in later on */

    switch(stype)
    {
    case stype_BText:
        p_textstylepack->type = display_Graphic;
        p_textstylepack->xpts = DP_DEFAULT_FONT_WIDTH * 16;
        p_textstylepack->ypts = DP_DEFAULT_FONT_HEIGHT * 16;
        break;

    case stype_FText:
        p_textstylepack->type = display_Text;
        p_textstylepack->xpts = DP_DEFAULT_BIG_FONT_WIDTH * 16;
        p_textstylepack->ypts = DP_DEFAULT_BIG_FONT_HEIGHT * 16;
        break;

    default:
        p_textstylepack->type = display_Text;
        p_textstylepack->xpts = DP_DEFAULT_FONT_WIDTH * 16;
        p_textstylepack->ypts = DP_DEFAULT_FONT_HEIGHT * 16;
        break;
    }

    p_textstylepack->line.spacing  = DP_DEFAULT_LINE_SPACE;
    p_textstylepack->line.baseline = p_textstylepack->line.spacing / 4;
    p_textstylepack->line.calctype = calc_Scaling; /* use calcvalue as a percentage */
    p_textstylepack->line.calcvalue= 120; /* 120% please */

    /* Frame margins should be 1600 millipoints all round. */
    p_textstylepack->margin.x0 = DP_DEFAULT_FRAME_LM;
    p_textstylepack->margin.y0 = DP_DEFAULT_FRAME_BM;
    p_textstylepack->margin.x1 = DP_DEFAULT_FRAME_RM;
    p_textstylepack->margin.y1 = DP_DEFAULT_FRAME_TM;

    if(is_title)
    {
        p_textstylepack->flags = (styleflags) (style_ExpandX | style_ExpandY);
        p_textstylepack->align = align_Right;
    }
    else
    {
        p_textstylepack->flags = (styleflags) 0;
        p_textstylepack->align = align_Left;
    }

    p_textstylepack->textcolour  = DP_DEFAULT_TEXT_FORE_COLOUR;  /* Black */
    p_textstylepack->backcolour  = DP_DEFAULT_TEXT_BACK_COLOUR;  /* White */
    p_textstylepack->bordercolour= DP_DEFAULT_BORDER_COLOUR;

    p_textstylepack->border.type   = NULL;   /* pointer to border name */

    p_textstylepack->border.margin = 10;
    p_textstylepack->border.scale  = 1;

    p_textstylepack->handle  = NULL;
    p_textstylepack->handle2 = NULL;
}

_Check_return_
static STATUS
make_style_chunk(
    P_ARRAY_HANDLE p_array_handle, styletype stype,
    _InVal_     BOOL is_title)
{
    STATUS status;
    char font_name[24];
    char border_name[16];

    textstylepack * p_textstylepack;
    stylepackstr * p_style_chunk;
    S32 font_len, border_len, size;

    xstrkpy(font_name, elemof32(font_name), is_title ? DP_FONT_OBLIQUE : DP_FONT_MEDIUM);

    font_len = 1 + strlen(font_name);

    (void) strcpy(border_name, ".");

    border_len = 1 + strlen(border_name);

    size = align__4(sizeof32(stylepackstr) + font_len + border_len);

    if(NULL == (p_style_chunk = (stylepackstr *) al_array_extend_by_BYTE(p_array_handle, size, PC_ARRAY_INIT_BLOCK_NONE, &status)))
        return(status);

    p_style_chunk->chunk.id     = chunk_Style;
    p_style_chunk->chunk.offset = (int)size;

    p_textstylepack = &p_style_chunk->style;

    make_textstylepack(p_textstylepack, is_title, stype);

    p_style_chunk->stype = stype;

    {
    P_U8 p_u8 = (P_U8) p_style_chunk + sizeof32(stylepackstr);

    /* A Font name */
    p_textstylepack->fontname = (int) (((S32) p_u8)-((S32) p_style_chunk));  /* set offset to font name from start of chunk */
    (void) strcpy(p_u8, font_name);
    p_u8 += font_len;

    /* A border name */
    p_textstylepack->border.type = (bordertype) (((S32) p_u8)-((S32) p_style_chunk));  /* set offset to font name from start of chunk */

    (void) strcpy(p_u8, border_name);
    p_u8 += border_len;
    p_style_chunk->/*int*/end_offset = (int) (((S32) p_u8)-((S32) p_style_chunk));
    } /*block*/

    return(STATUS_OK);
}

/******************************************************************************
*
* Low level routine to write the layout, fields and frames to an open datapower file.
*
******************************************************************************/

_Check_return_
extern STATUS
write_frames_chunky(
    fields_file fp,
    _In_        int rootno,
    fieldsptr p_fields,
    _In_        S32 n_flds)
{
    STATUS status = STATUS_OK;
    int relid = ROOT_LAYOUT /* As if by magic the shopkeeper appeared */;
    char relname[40];
    S32 i;
    ARRAY_HANDLE array_handle;

    (void) strcpy(relname, "Fireworkz Layout");

    {
    P_U8 p_u8 = al_array_alloc_U8(&array_handle, 4, &array_init_block_u8, &status); /* The extra 4 are for the pseudoflexing */
    if(NULL == p_u8)
        return(status);
    * (P_S32) p_u8 = (((S32) p_u8) + 4); /* psuedoflexor */
    } /*block*/

    /* Output the chunks, reallocing enough space as we go */
    status_return(make_header_chunk(&array_handle, FRAME_ID_START, n_flds));

    for(i = 0; i < 4; i++)
        status_return(make_style_chunk(&array_handle, (styletype) (stype_FText+i), FALSE));

    /* Use p_fields to navigate the fields list
       should change it to for i= 0 to n_flds ... get fielddef...
    */
    {
    fieldptr p_field;
    fieldptr p_next_field;
    S32 yth = 0;
    S32 this_frame_id = FRAME_ID_START;
    S32 next_frame_id;
    S32 title_id;

    p_field = fields_head(p_fields);

    while(NULL != p_field)
    {
        if(field_getflags(p_field) & fflag_Surrogate)
            /* It was the surrogate key so dont create a frame */
            p_field = field_next(p_field);
        else
        {
            char buffer[64];
            P_U8 p_name = field_name(p_field);

            if(NULL != p_name)
                xstrkpy(buffer, elemof32(buffer), p_name);
            else
                buffer[0] = CH_NULL;

            p_next_field = field_next(p_field);

            if(NULL != p_next_field)
            {
#if TITS_OUT
                title_id      = this_frame_id + 1;
                next_frame_id = this_frame_id + 2;
#else
                title_id      = 0;
                next_frame_id = this_frame_id + 1;
#endif
            }
            else
            {
                title_id      = this_frame_id + 1;
                next_frame_id = 0;
            }

            status_return(make_frame_chunk(&array_handle, this_frame_id, p_field, next_frame_id, title_id, yth, buffer));
            status_return(make_style_chunk(&array_handle, stype_None, FALSE));

#if TITS_OUT
            if(strlen(buffer)) /* Suppress titles for nameless ones */
            {
                status_return(make_frame_chunk(&array_handle, title_id, NULL, 0, 0, yth, buffer));
                status_return(make_style_chunk(&array_handle, stype_None, TRUE));
            }
#endif

            p_field = p_next_field;
            this_frame_id = next_frame_id;

            yth++;
        }
    } /* endwhile */
    } /*block*/

    { /* Psuedoflexor it */
    char ** anchor = array_base(&array_handle, char *);
    int size = (int) (array_elements(&array_handle) - 4);
    P_U8 p_u8 = array_base(&array_handle, U8);
    _kernel_oserror * e;
    * (P_S32) p_u8 = (((S32) p_u8) + 4); /* re-psuedoflexor */
    e = fields_savelayout(fp, rootno, relid, relname, NULL, 0, anchor, size);
    if(NULL != e)
        status = rec_oserror_set(e);
    } /*block*/

    al_array_dispose(&array_handle);

    return(status);
}

/* end of dppack.c */
