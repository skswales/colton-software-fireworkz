/* riscos/ho_print.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* RISC OS specific print routines for Fireworkz */

/* RCM Aug 1992 */

#include "common/gflags.h"

#if RISCOS

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#define EXPOSE_RISCOS_FLEX 1
#define EXPOSE_RISCOS_SWIS 1

#include "ob_skel/xp_skelr.h"

#include "ob_skel/ho_print.h"

#define NEW_PRINT_ORIGIN TRUE  /*>>>using TRUE seems to stop pictures from printing!!! - investigate why */

/*
internal types
*/

typedef struct print_transmatstr
{
    int xx, xy, yx, yy;
}
print_transmatstr;

typedef struct print_positionstr
{
    int dx, dy;
}
print_positionstr;

typedef struct POSITION_ENTRY
{
    print_positionstr position[2];
}
POSITION_ENTRY, * P_POSITION_ENTRY;

typedef struct OSERROR_STASH
{
    STATUS status;
    _kernel_oserror e;
}
OSERROR_STASH, * P_OSERROR_STASH;

/*
internal routines
*/

_Check_return_
static STATUS
host_print_download_fonts(
    P_OSERROR_STASH p_oserror_stash,
    _DocuRef_   P_DOCU p_docu);

_Check_return_
static STATUS
print_one_printer_page(
    P_OSERROR_STASH p_oserror_stash,
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ARRAY_HANDLE h_page_list,
    _InVal_     ARRAY_INDEX page_index,
    _InVal_     S32 copies,
    P_GR_BOX p_inputBB,
    print_transmatstr * p_transmat,
    _InVal_     ARRAY_HANDLE h_position_list,
    P_PRINTER_PERCENTAGE p_printer_percentage);

static void
print_one_document_page(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ARRAY_HANDLE h_page_list,
    _InVal_     ARRAY_INDEX page_ident,
    _InRef_     PC_GDI_BOX clip,
    P_PRINTER_PERCENTAGE p_printer_percentage);

static void
print_percentage_band_inc(
    P_PRINTER_PERCENTAGE p_printer_percentage,
    _InVal_     S32 frac_upper,
    _InVal_     S32 frac_lower);

static void
skel_rect_from_print_box(
    _InoutRef_  P_SKEL_RECT p_skel_rect /*page_num untouched*/,
    _InRef_     PC_REDRAW_CONTEXT p_context,
    _InRef_     PC_GDI_BOX p_print_box);

_Check_return_
static STATUS
the_gorey_bit(
    _DocuRef_   P_DOCU p_docu,
    P_PRINT_CTRL p_print_ctrl,
    P_GR_BOX p_inputBB,
    print_transmatstr * p_transmat,
    P_ARRAY_HANDLE p_h_position_list);

#define WHITE 0xFFFFFF00

/******************************************************************************
*
* Accumulate errors
*
* The error to register is given by p_os_error, status holds the tag of a
* message into which the error string should be inserted when reperr is called
*
******************************************************************************/

_Check_return_
static STATUS
_kernel_swi_stash_error(
    _In_        int swi_code,
    _kernel_swi_regs * r,
    P_OSERROR_STASH p_oserror_stash)
{
    _kernel_oserror * p_kernel_oserror = _kernel_swi(swi_code, r, r);

    /* do nothing if no-error fed in, or one already stashed */

    if(p_kernel_oserror && status_ok(p_oserror_stash->status))
    {
        p_oserror_stash->status = ERR_PRINT_FAILED;
        p_oserror_stash->e = *p_kernel_oserror;
        trace_1(TRACE_OUT | TRACE_ANY, TEXT("*** ERROR in print: %s"), p_oserror_stash->e.errmess);
    }

    return(p_oserror_stash->status);
}

/******************************************************************************
*
* 'Quality' printout
*
* This routine reports any os errors found
*
******************************************************************************/

_Check_return_
extern STATUS
host_print_document(
    _DocuRef_   P_DOCU p_docu,
    P_PRINT_CTRL p_print_ctrl)
{
    OSERROR_STASH oserror_stash;
    _kernel_swi_regs rs;
    GR_BOX inputBB[1];
    print_transmatstr transmat;
    ARRAY_INDEX outer_copies = p_print_ctrl->flags.collate ? p_print_ctrl->copies : 1;
    ARRAY_INDEX inner_copies = p_print_ctrl->flags.collate ? 1 : p_print_ctrl->copies;
    ARRAY_HANDLE h_position_list = 0 /* keep dataflower happy */;
    int currjob = 0 /* keep dataflower happy */;
    int oldjob;
    int font_downloading = 0x0;

    trace_0(TRACE_APP_PRINT, TEXT("host_print_document - in"));

    oserror_stash.status = STATUS_OK; /* extremely unlikely with printing! */
    oserror_stash.e.errmess[0] = CH_NULL;

    /* open an output stream onto "printer:" */
    rs.r[0] = 0x8F; /* OpenOut, Ignore File$Path, give err if a dir.! */
    rs.r[1] = (int) "printer:";
    if(NULL != WrapOsErrorReporting(_kernel_swi(OS_Find, &rs, &rs)))
        return(STATUS_FAIL);

    /* check for RISC OS 2.00 FileSwitch bug (returning a zero handle) */
    if((currjob = rs.r[0]) == 0)
    {
        reperr_null(ERR_PRINT_WONT_OPEN);
        return(STATUS_FAIL);
    }

    { /* SKS 18feb2014 add some headroom for printing like PipeDream */
    void * blkp;

    flex_forbid_shrink(TRUE);

    if(NULL != (blkp = _al_ptr_alloc(64*1024, &oserror_stash.status)))
        al_ptr_dispose(&blkp);
    } /*block*/

    /* diz 21 Jul 1993.  Cos' printer drivers get sick when GetInfo called during
     * a job context we must do it outside and then bosch the flag a bit
     * further down.
     */

    if(status_ok(oserror_stash.status))
        if(NULL == _kernel_swi(PDriver_Info, &rs, &rs))
            font_downloading = rs.r[3] & (1<<29 /* declare font supported */);

    /* calculate inputBB (x0,y0,x1,y1) transmat (xx,xy,yx,yy) */

    /*   for one up  array[0..0] of position */
    /*   for two up  array[0..1] of position */
    /*   this would make for easy(?!#) label printing at a later date */

    if(status_ok(oserror_stash.status))
        oserror_stash.status = the_gorey_bit(p_docu, p_print_ctrl, inputBB, &transmat, &h_position_list);

    if(status_ok(oserror_stash.status))
    {
        /* select filehandle as the current print job */
        rs.r[0] = currjob;
        rs.r[1] = 0; /* no title string for job */
        if(status_ok(_kernel_swi_stash_error(PDriver_SelectJob, &rs, &oserror_stash)))
        {
            oldjob = rs.r[0];

            /* if no font downloading do this, or if downloading is supported
             * then only perform the rest if the download worked */

            if((font_downloading == 0x0) || (status_ok(host_print_download_fonts(&oserror_stash, p_docu))))
            {
                { /* print n collated sets */
                ARRAY_INDEX outer_copy_idx;
                for(outer_copy_idx = 0; outer_copy_idx < outer_copies; ++outer_copy_idx)
                {
                    const ARRAY_INDEX page_count = array_elements(&p_print_ctrl->h_page_list);
                    ARRAY_INDEX page_index;
                    const ARRAY_INDEX docu_pages_per_printer_page = array_elements(&h_position_list);
                    PRINTER_PERCENTAGE printer_percentage;

                    print_percentage_initialise(p_docu, &printer_percentage, page_count);

                    for(page_index = 0; page_index < page_count; page_index += docu_pages_per_printer_page)
                    {
                        (void) print_one_printer_page(&oserror_stash, p_docu, p_print_ctrl->h_page_list, page_index, inner_copies,
                                                      inputBB, &transmat, h_position_list, &printer_percentage);
                        status_assert(oserror_stash.status);
                        status_break(oserror_stash.status);

                        print_percentage_page_inc(&printer_percentage);
                    }

                    print_percentage_finalise(&printer_percentage);

                    status_break(oserror_stash.status);
                }
                } /*block*/
            }

            /* if error occurred, abort our print job else end our print job normally */
            rs.r[0] = currjob;
            if(status_ok(oserror_stash.status))
            {
                status_consume(_kernel_swi_stash_error(PDriver_EndJob, &rs, &oserror_stash));
            }
            else
            {
                void_WrapOsErrorChecking(_kernel_swi(PDriver_AbortJob, &rs, &rs));
            }

            /* in either case, reselect print job (if any) that was active on entry */
            rs.r[0] = oldjob;
            status_consume(_kernel_swi_stash_error(PDriver_SelectJob, &rs, &oserror_stash));
        }

        al_array_dispose(&h_position_list);
    }

    /* close output stream */
    rs.r[0] = 0;
    rs.r[1] = currjob;
    status_consume(_kernel_swi_stash_error(OS_Find, &rs, &oserror_stash));

    if(oserror_stash.status == ERR_PRINT_FAILED)
    {
        reperr(oserror_stash.status, oserror_stash.e.errmess);
        return(STATUS_FAIL); /* don't report error further out, but we have failed */
    }

    flex_forbid_shrink(FALSE);

    trace_0(TRACE_APP_PRINT, TEXT("host_print_document - out"));
    return(oserror_stash.status);
}

/******************************************************************************
*
* SKS 1.03 18mar93 download fonts and kerning info to
* PostScript printers after job selected for first time
*
******************************************************************************/

typedef struct DOWNLOAD_FONTS_INFO
{
    P_OSERROR_STASH p_oserror_stash;
    P_DOCU p_docu;
    STYLE_SELECTOR selector;
    S32 bold;
    S32 italic;
    ARRAY_HANDLE h_typefaces; /* -> array_handle of ARRAY_HANDLE_TSTR */
}
DOWNLOAD_FONTS_INFO, * P_DOWNLOAD_FONTS_INFO;

_Check_return_
static STATUS
host_print_document_download_fonts_list(
    P_DOWNLOAD_FONTS_INFO p_download_fonts_info,
    P_ARRAY_HANDLE p_h_style_list)
{
    ARRAY_INDEX style_docu_area_idx;

    for(style_docu_area_idx = 0; style_docu_area_idx < array_elements(p_h_style_list); ++style_docu_area_idx)
    {
        P_STYLE_DOCU_AREA p_style_docu_area = array_ptr(p_h_style_list, STYLE_DOCU_AREA, style_docu_area_idx);
        STYLE style;
        P_STYLE p_style;

        if(p_style_docu_area->deleted)
            continue;

        if(p_style_docu_area->style_handle)
        {
            /* dereference */
            p_style = &style;
            style_init(p_style);
            style_struct_from_handle(p_download_fonts_info->p_docu, p_style, p_style_docu_area->style_handle, &p_download_fonts_info->selector);
        }
        else
            p_style = p_style_docu_area->p_style_effect; /* <<< 06sep94 wot says stu */

        if(!p_download_fonts_info->bold && style_bit_test(p_style, STYLE_SW_FS_BOLD))
            p_download_fonts_info->bold = p_style->font_spec.bold;

        if(!p_download_fonts_info->italic && style_bit_test(p_style, STYLE_SW_FS_ITALIC))
            p_download_fonts_info->italic = p_style->font_spec.italic;

        if(style_bit_test(p_style, STYLE_SW_FS_NAME))
        {
            const ARRAY_HANDLE_TSTR h_typeface_tstr = p_style->font_spec.h_app_name_tstr;
            STATUS found = 0;
            ARRAY_INDEX i;

            for(i = 0; i < array_elements(&p_download_fonts_info->h_typefaces); ++i)
            {
                P_ARRAY_HANDLE_TSTR p_h_tstr_typeface = array_ptr(&p_download_fonts_info->h_typefaces, ARRAY_HANDLE_TSTR, i);

                if(0 == tstricmp(array_tstr(&h_typeface_tstr), array_tstr(p_h_tstr_typeface)))
                {
                    found = 1;
                    break;
                }
            }

            if(!found)
            {   /* add typeface to list */
                SC_ARRAY_INIT_BLOCK array_init_block = aib_init(4, sizeof32(ARRAY_HANDLE), 0);
                if(status_fail(al_array_add(&p_download_fonts_info->h_typefaces, ARRAY_HANDLE, 1, &array_init_block, &h_typeface_tstr)))
                    return(p_download_fonts_info->p_oserror_stash->status = status_nomem());
            }
        }
    }

    return(STATUS_OK);
}

_Check_return_
static STATUS
host_print_document_download_fonts_hefo(
    P_DOWNLOAD_FONTS_INFO p_download_fonts_info,
    P_PAGE_HEFO_BREAK p_page_hefo_break,
    _In_        DATA_SPACE data_space)
{
    BIT_NUMBER bit_number;
    P_HEADFOOT_DEF p_headfoot_def;

    switch(data_space)
    {
    default: default_unhandled();
#if CHECKING
    case DATA_HEADER_ODD:
#endif
        bit_number     =           PAGE_HEFO_HEADER_ODD;
        p_headfoot_def = &p_page_hefo_break->header_odd;
        break;

    case DATA_HEADER_EVEN:
        bit_number     =           PAGE_HEFO_HEADER_EVEN;
        p_headfoot_def = &p_page_hefo_break->header_even;
        break;

    case DATA_HEADER_FIRST:
        bit_number     =           PAGE_HEFO_HEADER_FIRST;
        p_headfoot_def = &p_page_hefo_break->header_first;
        break;

    case DATA_FOOTER_ODD:
        bit_number     =           PAGE_HEFO_FOOTER_ODD;
        p_headfoot_def = &p_page_hefo_break->footer_odd;
        break;

    case DATA_FOOTER_EVEN:
        bit_number     =           PAGE_HEFO_FOOTER_EVEN;
        p_headfoot_def = &p_page_hefo_break->footer_even;
        break;

    case DATA_FOOTER_FIRST:
        bit_number     =           PAGE_HEFO_FOOTER_FIRST;
        p_headfoot_def = &p_page_hefo_break->footer_first;
        break;
    }

    if(!page_hefo_selector_bit_test(&p_page_hefo_break->selector, bit_number))
        return(STATUS_OK);

    return(host_print_document_download_fonts_list(p_download_fonts_info, &p_headfoot_def->headfoot.h_style_list));
}

_Check_return_
static STATUS
host_print_download_fonts(
    P_OSERROR_STASH p_oserror_stash,
    _DocuRef_   P_DOCU p_docu)
{
    _kernel_swi_regs rs;
    DOWNLOAD_FONTS_INFO download_fonts_info;

    download_fonts_info.p_oserror_stash = p_oserror_stash;
    download_fonts_info.p_docu = p_docu;
    download_fonts_info.bold = 0;
    download_fonts_info.italic = 0;
    download_fonts_info.h_typefaces = 0;

    style_selector_clear(&download_fonts_info.selector);
    style_selector_bit_set(&download_fonts_info.selector, STYLE_SW_FS_BOLD);
    style_selector_bit_set(&download_fonts_info.selector, STYLE_SW_FS_ITALIC);
    style_selector_bit_set(&download_fonts_info.selector, STYLE_SW_FS_NAME);

    if(status_ok(host_print_document_download_fonts_list(&download_fonts_info, &p_docu->h_style_docu_area)))
    {
        P_PAGE_HEFO_BREAK p_page_hefo_break;
        ROW row = -1;

        while(NULL != (p_page_hefo_break = page_hefo_break_enum(p_docu, &row)))
        {
            status_break(host_print_document_download_fonts_hefo(&download_fonts_info, p_page_hefo_break, DATA_HEADER_ODD  ));
            status_break(host_print_document_download_fonts_hefo(&download_fonts_info, p_page_hefo_break, DATA_HEADER_EVEN ));
            status_break(host_print_document_download_fonts_hefo(&download_fonts_info, p_page_hefo_break, DATA_HEADER_FIRST));
            status_break(host_print_document_download_fonts_hefo(&download_fonts_info, p_page_hefo_break, DATA_FOOTER_ODD  ));
            status_break(host_print_document_download_fonts_hefo(&download_fonts_info, p_page_hefo_break, DATA_FOOTER_EVEN ));
            status_break(host_print_document_download_fonts_hefo(&download_fonts_info, p_page_hefo_break, DATA_FOOTER_FIRST));
        }
    }

    trace_on();

    if(status_ok(download_fonts_info.p_oserror_stash->status))
    {
        /* download each distinct font we may use */
        ARRAY_INDEX i;
        FONT_SPEC font_spec;
        HOST_FONT_SPEC host_font_spec;

        zero_struct(host_font_spec); /*host_font_spec.h_host_name_tstr = 0;*/

        for(i = 0; i < array_elements(&download_fonts_info.h_typefaces); ++i)
        {
            zero_struct(font_spec);

            font_spec.h_app_name_tstr = *array_ptr(&download_fonts_info.h_typefaces, ARRAY_HANDLE_TSTR, i);

            font_spec.size_y = 12 * PIXITS_PER_POINT; /* must give **something** to the FontManager! */

            if(status_ok(fonty_handle_from_font_spec(&font_spec, p_docu->flags.draft_mode)))
            {
                host_font_spec_dispose(&host_font_spec);
                status_assert(fontmap_host_font_spec_from_font_spec(&host_font_spec, &font_spec, FALSE));
                rs.r[0] = 0;
                rs.r[1] = (int) array_tstr(&host_font_spec.h_host_name_tstr);
                rs.r[2] = (1 << 1 /*kerning - ON*/) | (0 << 0 /*prevent download - OFF*/); /* SKS 1.04 26mar93 use bitwise not illogical OR */
                trace_2(0, TEXT("PDriver_DeclareFont %s %s"), array_tstr(&font_spec.h_app_name_tstr), rs.r[1]);
                status_break(_kernel_swi_stash_error(PDriver_DeclareFont, &rs, p_oserror_stash));
            }

            font_spec.bold = 1;
            if(download_fonts_info.bold && status_ok(fonty_handle_from_font_spec(&font_spec, p_docu->flags.draft_mode)))
            {
                host_font_spec_dispose(&host_font_spec);
                status_assert(fontmap_host_font_spec_from_font_spec(&host_font_spec, &font_spec, FALSE));
                rs.r[0] = 0;
                rs.r[1] = (int) array_tstr(&host_font_spec.h_host_name_tstr);
                rs.r[2] = (1 << 1 /*kerning - ON*/) | (0 << 0 /*prevent download - OFF*/);
                trace_2(0, TEXT("PDriver_DeclareFont %s %s B"), array_tstr(&font_spec.h_app_name_tstr), rs.r[1]);
                status_break(_kernel_swi_stash_error(PDriver_DeclareFont, &rs, p_oserror_stash));
            }
            font_spec.bold = 0;

            font_spec.italic = 1;
            if(download_fonts_info.italic && status_ok(fonty_handle_from_font_spec(&font_spec, p_docu->flags.draft_mode)))
            {
                host_font_spec_dispose(&host_font_spec);
                status_assert(fontmap_host_font_spec_from_font_spec(&host_font_spec, &font_spec, FALSE));
                rs.r[0] = 0;
                rs.r[1] = (int) array_tstr(&host_font_spec.h_host_name_tstr);
                rs.r[2] = (1 << 1 /*kerning - ON*/) | (0 << 0 /*prevent download - OFF*/);
                trace_2(0, TEXT("PDriver_DeclareFont %s %s I"), array_tstr(&font_spec.h_app_name_tstr), rs.r[1]);
                status_break(_kernel_swi_stash_error(PDriver_DeclareFont, &rs, p_oserror_stash));
            }

            font_spec.bold = 1;
            if(download_fonts_info.bold && download_fonts_info.italic && status_ok(fonty_handle_from_font_spec(&font_spec, p_docu->flags.draft_mode)))
            {
                host_font_spec_dispose(&host_font_spec);
                status_assert(fontmap_host_font_spec_from_font_spec(&host_font_spec, &font_spec, FALSE));
                rs.r[0] = 0;
                rs.r[1] = (int) array_tstr(&host_font_spec.h_host_name_tstr);
                rs.r[2] = (1 << 1 /*kerning - ON*/) | (0 << 0 /*prevent download - OFF*/);
                trace_2(0, TEXT("PDriver_DeclareFont %s %s BI"), array_tstr(&font_spec.h_app_name_tstr), rs.r[1]);
                status_break(_kernel_swi_stash_error(PDriver_DeclareFont, &rs, p_oserror_stash));
            }
        }

        host_font_spec_dispose(&host_font_spec);

        /* end of list */
        trace_0(0, TEXT("End of PDriver_DeclareFont"));
        rs.r[0] = 0;
        rs.r[1] = 0;
        status_consume(_kernel_swi_stash_error(PDriver_DeclareFont, &rs, p_oserror_stash));

        fonty_cache_trash(P_REDRAW_CONTEXT_NONE);
    }

    trace_off();

    al_array_dispose(&download_fonts_info.h_typefaces);

    return(p_oserror_stash->status);
}

/******************************************************************************
*
* On one piece of printer paper, print as many document pages as there are positions specified by h_position_list.
*
******************************************************************************/

_Check_return_
static STATUS
print_one_printer_page(
    P_OSERROR_STASH p_oserror_stash,
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ARRAY_HANDLE h_page_list,
    _InVal_     ARRAY_INDEX page_index,
    _InVal_     S32 copies,
    P_GR_BOX p_inputBB,
    print_transmatstr * p_transmat,
    _InVal_     ARRAY_HANDLE h_position_list,
    P_PRINTER_PERCENTAGE p_printer_percentage)
{
    _kernel_swi_regs rs;
    GDI_BOX clip_box;
    S32 more;
    ARRAY_INDEX ident;
    const ARRAY_INDEX page_count = array_elements(&h_page_list);
    const ARRAY_INDEX docu_pages_per_printer_page = array_elements(&h_position_list);
    ARRAY_INDEX position_index;
    P_POSITION_ENTRY p_position_entry;

    assert(docu_pages_per_printer_page > 0);    /* better be >0 or cos we us it as a for-loop step value */
    if(docu_pages_per_printer_page <= 0)
        return(STATUS_OK);

    trace_0(TRACE_APP_PRINT, TEXT("print_one_printer_page - in"));

    /*>>>display percentage here*/

    /* give the printer driver a rectangle and position for each document page that is to be printed on this printer page */
    for(position_index = 0; position_index < docu_pages_per_printer_page; ++position_index)
    {
        /* when printing two_up (or more) there may be fewer document pages to be printed than available */
        /* paper areas, so only give a rectangle to the printer if an entry exists in h_page_list        */

        if((page_index + position_index) < page_count)
        {
            /*>>>could test for blank document page here */
            S32 page_ident = (S32)page_index + position_index;
            PAGE page_num_y = array_ptr(&h_page_list, PAGE_ENTRY, page_ident)->page.y;
            S32 pos_index = page_num_y & 1;

            p_position_entry = array_ptr(&h_position_list, POSITION_ENTRY, position_index);

            trace_0(TRACE_APP_PRINT, TEXT("Calling print_giverectangle with:"));
            trace_4(TRACE_APP_PRINT, TEXT("  inputBB=(") S32_TFMT TEXT(",") S32_TFMT TEXT(", ") S32_TFMT TEXT(",") S32_TFMT TEXT(")"),
                    p_inputBB[0].x0, p_inputBB[0].y0, p_inputBB[0].x1, p_inputBB[0].y1);
            trace_4(TRACE_APP_PRINT, TEXT("  transmat=(") S32_TFMT TEXT(",") S32_TFMT TEXT(", ") S32_TFMT TEXT(",") S32_TFMT TEXT(")"),
                    p_transmat->xx, p_transmat->xy, p_transmat->yx, p_transmat->yy);
            trace_3(TRACE_APP_PRINT, TEXT("  position[") S32_TFMT TEXT("]=(") S32_TFMT TEXT(",") S32_TFMT TEXT(")"),
                    pos_index, p_position_entry->position[pos_index].dx, p_position_entry->position[pos_index].dy);

            rs.r[0] = (int) page_ident /* page identity, passed back to us in print loop */;
            rs.r[1] = (int) &p_inputBB[0];
            rs.r[2] = (int) p_transmat;
            rs.r[3] = (int) &p_position_entry->position[pos_index];
            rs.r[4] = (int) WHITE /* background colour for this rectangle */;
            status_return(_kernel_swi_stash_error(PDriver_GiveRectangle, &rs, p_oserror_stash));
        }
    }

    rs.r[0] = (int) copies /* number of copies of this page */;
    rs.r[1] = (int) &clip_box;
    rs.r[2] = 0 /* page sequence number within document */;
    rs.r[3] = 0 /* page number string */;
    status_return(_kernel_swi_stash_error(PDriver_DrawPage, &rs, p_oserror_stash));
    more  = rs.r[0];
    ident = rs.r[2];

    while(more)
    {
        /* print the document page, requested (demanded) by the printer driver */
        host_invalidate_cache(HIC_REDRAW_LOOP_START);

        print_one_document_page(p_docu, h_page_list, ident, &clip_box, p_printer_percentage);

        rs.r[1] = (int) &clip_box;
        status_return(_kernel_swi_stash_error(PDriver_GetRectangle, &rs, p_oserror_stash));
        more  = rs.r[0];
        ident = rs.r[2];
    }

    trace_0(TRACE_APP_PRINT, TEXT("print_one_printer_page - out"));
    return(STATUS_OK);
}

/* printer equivalent of host_redraw_context_set_host_xform() */

static void
print_host_redraw_context_set_host_xform(
    _InoutRef_  P_HOST_XFORM p_host_xform /*updated*/)
{
    p_host_xform->riscos.d_x = 2; /* cast in stone somewhere! */
    p_host_xform->riscos.d_y = 2;

    p_host_xform->riscos.eig_x = 1;
    p_host_xform->riscos.eig_y = 1;
}

static void
print_one_document_page(
    _DocuRef_   P_DOCU p_docu,
    _InVal_     ARRAY_HANDLE h_page_list,
    _InVal_     ARRAY_INDEX page_ident,
    _InRef_     PC_GDI_BOX p_clip_box,
    P_PRINTER_PERCENTAGE p_printer_percentage)
{
    const PC_PAGE_ENTRY p_page_entry = array_ptrc(&h_page_list, PAGE_ENTRY, page_ident);
    REDRAW_CONTEXT_CACHE redraw_context_cache;
    SKELEVENT_REDRAW skelevent_redraw;
    const P_REDRAW_CONTEXT p_redraw_context = &skelevent_redraw.redraw_context;

    zero_struct(skelevent_redraw);

    zero_struct(redraw_context_cache);
    p_redraw_context->p_redraw_context_cache = &redraw_context_cache;

    skelevent_redraw.flags.show_content = TRUE;

    p_redraw_context->flags.printer = 1;

    p_redraw_context->host_xform.scale.t.x = 1;
    p_redraw_context->host_xform.scale.t.y = 1;
    p_redraw_context->host_xform.scale.b.x = 1;
    p_redraw_context->host_xform.scale.b.y = 1;

    p_redraw_context->display_mode = DISPLAY_PRINT_AREA;

    p_redraw_context->border_width.x = p_redraw_context->border_width.y = p_docu->page_def.grid_size;
    p_redraw_context->border_width_2.x = p_redraw_context->border_width_2.y = 2 * p_docu->page_def.grid_size;

    print_host_redraw_context_set_host_xform(&p_redraw_context->host_xform);

    host_redraw_context_fillin(p_redraw_context);

    if((p_page_entry->page.x >= 0) && (p_page_entry->page.y >= 0))
    {
        PIXIT_RECT print_area;
        GDI_BOX bigger_clip_box;

        print_area.tl.x = 0;
        print_area.tl.y = 0;
        print_area.br.x = p_docu->page_def.size_x - margin_left_from(&p_docu->page_def, p_page_entry->page.y) - margin_right_from(&p_docu->page_def, p_page_entry->page.y);
        print_area.br.y = p_docu->page_def.size_y - p_docu->page_def.margin_top  - p_docu->page_def.margin_bottom;

        p_redraw_context->gdi_org.x     = 0;
#if NEW_PRINT_ORIGIN
        p_redraw_context->gdi_org.y     = 0;
#else
        p_redraw_context->gdi_org.y     = (print_area.br.y-print_area.tl.y) / PIXITS_PER_RISCOS; /*>>>why isn't this zero?*/
                                                                      /*>>>try feeding different positions to print give_rectangle*/
#endif

        p_redraw_context->riscos.host_machine_clip_box = *p_clip_box;

        p_redraw_context->pixit_origin.x = 0;
        p_redraw_context->pixit_origin.y = 0;

        /* the printer driver lies through its teeth, so enlarge the clip rectangle it supplied by some small amount */
        bigger_clip_box.x0 = p_clip_box->x0 - 8;
        bigger_clip_box.y0 = p_clip_box->y0 - 8;
        bigger_clip_box.x1 = p_clip_box->x1 + 8;
        bigger_clip_box.y1 = p_clip_box->y1 + 8; /* was 16 */

        skel_rect_from_print_box(&skelevent_redraw.clip_skel_rect, p_redraw_context, &bigger_clip_box);
        skelevent_redraw.clip_skel_rect.tl.page_num = p_page_entry->page;
        skelevent_redraw.clip_skel_rect.br.page_num = p_page_entry->page;

        print_percentage_band_inc(p_printer_percentage, skelevent_redraw.clip_skel_rect.tl.pixit_point.y, (print_area.br.y - print_area.tl.y));

#if FALSE
                trace_0(TRACE_APP_PRINT, TEXT("Calling skeleton with:"));
                trace_2(TRACE_APP_PRINT, TEXT("  redraw page ") S32_TFMT TEXT(", ") S32_TFMT,
                        skelevent_redraw.clip_skel_rect.tl.page_num.x, skelevent_redraw.clip_skel_rect.tl.page_num.y);
                trace_4(TRACE_APP_PRINT, TEXT("  redraw area ") S32_TFMT TEXT(", ") S32_TFMT TEXT(", ") S32_TFMT TEXT(", ") S32_TFMT,
                        skelevent_redraw.clip_skel_rect.tl.pixit_point.x, skelevent_redraw.clip_skel_rect.tl.pixit_point.y,
                        skelevent_redraw.clip_skel_rect.br.pixit_point.x, skelevent_redraw.clip_skel_rect.br.pixit_point.y);
#endif
#if FALSE
            skelevent_redraw.clip_skel_rect.tl.pixit_point.x = MAX(skelevent_redraw.clip_skel_rect.tl.pixit_point.x, 0);
            skelevent_redraw.clip_skel_rect.tl.pixit_point.y = MAX(skelevent_redraw.clip_skel_rect.tl.pixit_point.y, 0);
#endif

            /* based on send_redrawevent_to_skel in c.view */
            /* from first switch statement, when p_view->display_mode is DISPLAY_PRINT_AREA */
        {
            /* in second switch statement, when p_view->display_mode is DISPLAY_PRINT_AREA */
            /* do the same header_area, footer_area, row_area, col_area, work_area calculations */
            /* so replace by a proc */

            host_paint_start(&skelevent_redraw.redraw_context);
            view_redraw_page(p_docu, &skelevent_redraw, &print_area);
            host_paint_end(&skelevent_redraw.redraw_context);
        }
    }
}

/******************************************************************************
*
* Set up print rectangle, transformation matrices and print positions needed by print_giverectangle().
*
******************************************************************************/

_Check_return_
static STATUS
the_gorey_bit(
    _DocuRef_   P_DOCU p_docu,
    P_PRINT_CTRL p_print_ctrl,
    P_GR_BOX p_inputBB,
    print_transmatstr * p_transmat,
    P_ARRAY_HANDLE p_h_position_list)
{
    PIXIT_POINT size;
    SC_ARRAY_INIT_BLOCK array_init_block = aib_init(1, sizeof32(POSITION_ENTRY), TRUE);
    STATUS status = STATUS_OK;

    size.x = p_docu->page_def.size_x - margin_left_from(&p_docu->page_def, 0) - margin_right_from(&p_docu->page_def, 0);
    size.y = p_docu->page_def.size_y - p_docu->page_def.margin_top  - p_docu->page_def.margin_bottom;

    p_inputBB[0].x0 = 0;
    p_inputBB[0].x1 = +(size.x / PIXITS_PER_RISCOS) + 2 /* SKS fudge - plot a little more to the right! */;
#if NEW_PRINT_ORIGIN
    p_inputBB[0].y0 = -(size.y / PIXITS_PER_RISCOS);
    p_inputBB[0].y1 = 0;
#else
    p_inputBB[0].y0 = 0;
    p_inputBB[0].y1 = +(size.y / PIXITS_PER_RISCOS);
#endif

#if 0
    /* displace normal (right) page inputBB by binding margin */
    p_inputBB[0].x0 = p_inputBB[1].x0 - p_docu->page_def.margin_bind / PIXITS_PER_RISCOS;
    p_inputBB[0].y0 = p_inputBB[1].y0;
    p_inputBB[0].x1 = p_inputBB[1].x1 - p_docu->page_def.margin_bind / PIXITS_PER_RISCOS;
    p_inputBB[0].y1 = p_inputBB[1].y1;

    /* only if we are swapping do we use the thing we first thought of */
    if(!p_docu->page_def.margin_oe_swap)
        p_inputBB[1] = p_inputBB[0];
#endif

    *p_h_position_list = NULL;

    if(!p_print_ctrl->flags.two_up)
    {
        print_positionstr position[2];
        POSITION_ENTRY position_entry;
        S32 xform = 0x00010000;

        if(100 != p_docu->paper_scale) /* SKS 22apr96 only introduce inaccuracies if we really need to */
            xform = muldiv64(xform, p_docu->paper_scale, 100);

        if(!p_print_ctrl->flags.landscape)
        {
            p_transmat->xx = (int) +xform;
            p_transmat->xy = 0;
            p_transmat->yx = 0;
            p_transmat->yy = (int) +xform;

            position[0].dx = (int) muldiv64(margin_left_from(&p_docu->page_def, 0), p_docu->paper_scale, 100) * MILLIPOINTS_PER_PIXIT;
            position[0].dy = (int) p_docu->phys_page_def.margin_bottom * MILLIPOINTS_PER_PIXIT;

            position[1].dx = (int) muldiv64(margin_left_from(&p_docu->page_def, 1), p_docu->paper_scale, 100) * MILLIPOINTS_PER_PIXIT;
            position[1].dy = (int) p_docu->phys_page_def.margin_bottom * MILLIPOINTS_PER_PIXIT;
        }
        else
        {
            p_transmat->xx = 0;
            p_transmat->xy = (int) -xform;
            p_transmat->yx = (int) +xform;
            p_transmat->yy = 0;

            position[0].dx = (int) p_docu->phys_page_def.margin_bottom * MILLIPOINTS_PER_PIXIT;
            position[0].dy = (int) (p_docu->phys_page_def.size_x - muldiv64(margin_left_from(&p_docu->page_def, 0), p_docu->paper_scale, 100)) * MILLIPOINTS_PER_PIXIT;

            position[1].dx = (int) p_docu->phys_page_def.margin_bottom * MILLIPOINTS_PER_PIXIT;
            position[1].dy = (int) (p_docu->phys_page_def.size_x - muldiv64(margin_left_from(&p_docu->page_def, 1), p_docu->paper_scale, 100)) * MILLIPOINTS_PER_PIXIT;
        }

        position_entry.position[0] = position[0];
        position_entry.position[1] = position[1];

        status_return(al_array_add(p_h_position_list, POSITION_ENTRY, 1, &array_init_block, &position_entry));
    }
    else
    {
        PAPER printer_paper;
        S32 scale, scale1, scale2;
        print_positionstr position, position2;
        POSITION_ENTRY position_entry[2];

        if(status_fail(status = host_read_printer_paper_details(&printer_paper)))
            return(status);     /* probably some sort of "printer driver not present" error */

        if(!p_print_ctrl->flags.landscape)
        {
            scale1 = muldiv64(65536, ((printer_paper.y_size / 2) - printer_paper.tm - printer_paper.bm), printer_paper.print_area_x);
            scale2 = muldiv64(65536, printer_paper.print_area_x, printer_paper.print_area_y);
            scale  = MIN(scale1, scale2);

            p_transmat->xx = 0;
            p_transmat->xy = (int) -scale; /* 0xFFFF0000; */
            p_transmat->yx = (int) +scale; /* 0x00010000; */
            p_transmat->yy = 0;

            position.dx  = (int) (                                printer_paper.lm) * MILLIPOINTS_PER_PIXIT;
            position.dy  = (int) (printer_paper.print_area_y    + printer_paper.bm) * MILLIPOINTS_PER_PIXIT;
            assert(              (printer_paper.print_area_y    + printer_paper.bm) == (printer_paper.y_size - printer_paper.tm));
            position2.dx = position.dx;
            position2.dy = (int) (printer_paper.y_size / 2      - printer_paper.tm) * MILLIPOINTS_PER_PIXIT;
            assert(              (printer_paper.y_size / 2      - printer_paper.tm) == (printer_paper.print_area_y/2 + printer_paper.bm - printer_paper.tm));
        }
        else
        {
            scale1 = muldiv64(65536, ((printer_paper.y_size / 2) - printer_paper.tm - printer_paper.bm), printer_paper.print_area_x);
            scale2 = muldiv64(65536, printer_paper.print_area_x, printer_paper.print_area_y);
            scale  = MIN(scale1, scale2);

            p_transmat->xx = (int) +scale;   /* 0x00010000; */
            p_transmat->xy = 0;
            p_transmat->yx = 0;
            p_transmat->yy = (int) +scale;   /* 0x00010000; */

            position.dx  = (int) (                                printer_paper.lm) * MILLIPOINTS_PER_PIXIT;
          /*position.dy  =       (printer_paper.print_area_y    + printer_paper.bm) * MILLIPOINTS_PER_PIXIT;*/
            position.dy  = (int) (printer_paper.y_size / 2      + printer_paper.bm) * MILLIPOINTS_PER_PIXIT;

            position2.dx = position.dx;
          /*position2.dy =       (printer_paper.y_size / 2      - printer_paper.tm) * MILLIPOINTS_PER_PIXIT;*/
            position2.dy = (int) (                                printer_paper.bm) * MILLIPOINTS_PER_PIXIT;
        }

        position_entry[0].position[0] = position;
        position_entry[0].position[1] = position;
        position_entry[1].position[0] = position2;
        position_entry[1].position[1] = position2;

        status_return(al_array_add(p_h_position_list, POSITION_ENTRY, elemof32(position_entry), &array_init_block, position_entry));
    }

    return(STATUS_OK);
}

static void
print_percentage_band_inc(
    P_PRINTER_PERCENTAGE p_printer_percentage,
    _InVal_     S32 frac_upper,
    _InVal_     S32 frac_lower)
{
    S32 sub_percentage = p_printer_percentage->percent_per_page * frac_upper / frac_lower;

    sub_percentage = MAX(sub_percentage, 0);
    sub_percentage = MIN(sub_percentage, p_printer_percentage->percent_per_page);

    print_percentage_reflect(p_printer_percentage, sub_percentage);
}

/* c.f. view_rect_from_screen_rect_and_context */

static void
skel_rect_from_print_box(
    _InoutRef_  P_SKEL_RECT p_skel_rect /*page_num untouched*/,
    _InRef_     PC_REDRAW_CONTEXT p_redraw_context,
    _InRef_     PC_GDI_BOX p_print_box)
{
    GDI_RECT gdi_rect;
    PIXIT_RECT pixit_rect;

    gdi_rect.tl.x = (p_print_box->x0 - p_redraw_context->gdi_org.x);
    gdi_rect.br.y = (p_print_box->y0 - p_redraw_context->gdi_org.y);
    gdi_rect.br.x = (p_print_box->x1 - p_redraw_context->gdi_org.x);
    gdi_rect.tl.y = (p_print_box->y1 - p_redraw_context->gdi_org.y);

    pixit_rect_from_window_rect(&pixit_rect, &gdi_rect, &p_redraw_context->host_xform);

    p_skel_rect->tl.pixit_point.x = pixit_rect.tl.x;
    p_skel_rect->tl.pixit_point.y = pixit_rect.tl.y;
    p_skel_rect->br.pixit_point.x = pixit_rect.br.x;
    p_skel_rect->br.pixit_point.y = pixit_rect.br.y;
}

/******************************************************************************
*
* read the current printer's name
*
******************************************************************************/

extern void
host_printer_name_query(
    _OutRef_    P_UI_TEXT p_ui_text)
{
    static TCHARZ printer_description[20]; /* so we can simply return pointer to name, copied by client */

    _kernel_swi_regs rs;

    p_ui_text->type = UI_TEXT_TYPE_RESID;
    p_ui_text->text.resource_id = MSG_DIALOG_PRINT_NO_RISCOS;

    if(NULL == WrapOsErrorChecking(_kernel_swi(PDriver_Info, &rs, &rs)))
    {
        tstr_xstrkpy(printer_description, elemof32(printer_description), (PCTSTR) rs.r[4]);

        p_ui_text->type = UI_TEXT_TYPE_TSTR_TEMP; /* still say it's temporary to force copy! */
        p_ui_text->text.tstr = printer_description;
    }
}

/******************************************************************************
*
* Read default paper details
*
* returns size, margins and printable area for a typical piece of A4 paper
*
******************************************************************************/

extern void
host_read_default_paper_details(
    P_PAPER p_paper)
{
    /* assume a sort of normal LaserWriter A4 */
    p_paper->x_size = 11901; /* 209.1 mm */
    p_paper->y_size = 16839; /* 297.0 mm */

    p_paper->lm = 358; /* 6.3 mm */
    p_paper->bm = 460; /* 8.1 mm */

    p_paper->rm = 358; /* 6.3 mm */
    p_paper->tm = 460; /* 8.1 mm */

    p_paper->print_area_x = p_paper->x_size - (p_paper->lm + p_paper->rm);
    p_paper->print_area_y = p_paper->y_size - (p_paper->tm + p_paper->bm);
}

/******************************************************************************
*
* Read paper details from currently selected printer
*
******************************************************************************/

_Check_return_
extern STATUS
host_read_printer_paper_details(
    /*out*/ P_PAPER p_paper)
{
    _kernel_swi_regs rs;
    BBox pixit_bbox;

    trace_0(TRACE_APP_PRINT, TEXT("host_read_printer_paper_details"));

    if(_kernel_swi(PDriver_PageSize, &rs, &rs))
    {
        trace_1(TRACE_OUT | TRACE_APP_PRINT, TEXT("error from pagesize: '%s'"), _kernel_last_oserror()->errmess);
        return(create_error(ERR_PRINT_PAPER_DETAILS));
    }

    trace_4(TRACE_APP_PRINT,
            TEXT("  page size       : (") S32_TFMT TEXT(" x ") S32_TFMT TEXT(" mp) (") S32_TFMT TEXT(" x ") S32_TFMT TEXT(" inches)"),
            rs.r[1] /*xsize*/, rs.r[2] /*ysize*/, rs.r[1] /*xsize*/ /72000, rs.r[2] /*ysize*/ /72000);

    trace_4(TRACE_APP_PRINT,
            TEXT("  printable area  : ") S32_TFMT TEXT(", ") S32_TFMT TEXT(", ") S32_TFMT TEXT(", ") S32_TFMT TEXT(" (x0,y0,x1,y1)"),
            rs.r[3] /*bbox.x0*/, rs.r[4] /*bbox.y0*/, rs.r[5] /*bbox.x1*/, rs.r[6] /*bbox.y1*/);

    p_paper->x_size = (S32) rs.r[1] /*xsize*/ / MILLIPOINTS_PER_PIXIT;
    p_paper->y_size = (S32) rs.r[2] /*ysize*/ / MILLIPOINTS_PER_PIXIT;

    /* round printable area inwards to pixit grid */
    pixit_bbox.xmin = div_round_ceil( rs.r[3] /*bbox.x0*/, MILLIPOINTS_PER_PIXIT);
    pixit_bbox.ymin = div_round_ceil( rs.r[4] /*bbox.y0*/, MILLIPOINTS_PER_PIXIT);
    pixit_bbox.xmax = div_round_floor(rs.r[5] /*bbox.x1*/, MILLIPOINTS_PER_PIXIT);
    pixit_bbox.ymax = div_round_floor(rs.r[6] /*bbox.y1*/, MILLIPOINTS_PER_PIXIT);

    p_paper->lm = pixit_bbox.xmin;
    p_paper->bm = pixit_bbox.ymin;
    p_paper->rm = p_paper->x_size - pixit_bbox.xmax;
    p_paper->tm = p_paper->y_size - pixit_bbox.ymax;

    p_paper->print_area_x = (PIXIT) BBox_width(&pixit_bbox);
    p_paper->print_area_y = (PIXIT) BBox_height(&pixit_bbox);

    trace_4(TRACE_APP_PRINT,
            TEXT("  page size       : (") S32_TFMT TEXT(" x ") S32_TFMT TEXT(" pixit) (") S32_TFMT TEXT(" x ") S32_TFMT TEXT(" inches)"),
            p_paper->x_size, p_paper->y_size, p_paper->x_size / PIXITS_PER_INCH, p_paper->y_size / PIXITS_PER_INCH);

    trace_4(TRACE_APP_PRINT,
            TEXT("  margins         : ") S32_TFMT TEXT(", ") S32_TFMT TEXT(", ") S32_TFMT TEXT(", ") S32_TFMT TEXT(" (lm,tm,rm,bm)"),
            p_paper->lm, p_paper->tm, p_paper->rm, p_paper->bm);

    trace_2(TRACE_APP_PRINT,
            TEXT("  printable area  : ") S32_TFMT TEXT(" x ") S32_TFMT,
            p_paper->print_area_x, p_paper->print_area_y);

    return(STATUS_OK);
}

#endif /* RISCOS */

/* end of ho_print.c */
