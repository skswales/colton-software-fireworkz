/* image_convert.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2015-2018 Stuart Swales */

/* Module that handles conversion for image file cache */

#include "common/gflags.h"

#include "cmodules/typepack.h"

#define EXPOSE_RISCOS_SWIS 1

#if RISCOS
#include "ob_skel/xp_skelr.h"

#define PicConvert_BMPtoFF9 0x00048E00
#endif

#if WINDOWS
#include "ob_skel/ho_gdip_image.h"
#endif

#if !defined(_tremove)
#define _tremove    remove
#endif

#if RISCOS

static TCHARZ
ChangeFSI_name_buffer[256];

_Check_return_
static BOOL
ChangeFSI_available(void)
{
    static int available = -1;

    if(available < 0)
    {
        const char * var_name = "ChangeFSI$Dir";

        available = 0;

        if(NULL == _kernel_getenv(var_name, ChangeFSI_name_buffer, elemof32(ChangeFSI_name_buffer)))
        {
            tstr_xstrkat(ChangeFSI_name_buffer, elemof32(ChangeFSI_name_buffer), ".ChangeFSI");

            if(file_is_file(ChangeFSI_name_buffer))
                available = 1;
        }
    }

    return(available);
}

#endif /* RISCOS */

/******************************************************************************
*
* query of RISC OS filetypes that might sensibly be converted
*
******************************************************************************/

_Check_return_
extern BOOL
image_convert_can_convert(
    _InVal_     T5_FILETYPE t5_filetype)
{
#if RISCOS
    if(!ChangeFSI_available())
        return(FALSE);

    switch(t5_filetype)
    {
    case FILETYPE_WV_V10:
    case FILETYPE_WV_V12:
    case FILETYPE_ICO:
    case FILETYPE_TSTEP_128W:
    case FILETYPE_RAYSHADE:
    case FILETYPE_CCIR_601:
    case FILETYPE_TCLEAR:
    case FILETYPE_DEGAS:
    case FILETYPE_GIF:
    case FILETYPE_PCX:
    case FILETYPE_QRT:
    case FILETYPE_MTV:
    case FILETYPE_BMP:
    case FILETYPE_TGA:
    case FILETYPE_CUR:
    case FILETYPE_TSTEP_800S:
    case FILETYPE_PNG:
    case FILETYPE_PCD:
    case FILETYPE_JPEG:
    case FILETYPE_PROARTISAN:
    case FILETYPE_WATFORD_DFA:
    case FILETYPE_WAP:
    case FILETYPE_TIFF:
        return(TRUE);

    default:
        break;
    }
#else
    switch(t5_filetype)
    {
    case FILETYPE_ICO:
    case FILETYPE_GIF:
    case FILETYPE_WMF:
    case FILETYPE_PNG:
    case FILETYPE_JPEG:
    case FILETYPE_TIFF:
    case FILETYPE_WINDOWS_EMF:
        return(TRUE);

    default:
        break;
    }
#endif /* RISCOS */

    return(FALSE);
}

/******************************************************************************
*
* convert some image data via a file
*
* --out--
* -ve = error
*   0 = converted file exists, caller must delete after reading
*
******************************************************************************/

_Check_return_
extern STATUS
image_convert_do_convert_data(
    _OutRef_    P_PTSTR p_converted_name,
    _OutRef_    P_T5_FILETYPE p_t5_filetype_converted,
    _In_reads_(n_bytes) PC_BYTE p_data,
    _InVal_     U32 n_bytes,
    _InVal_     T5_FILETYPE t5_filetype)
{
    STATUS status;
#if WINDOWS
    GdipImage gdip_image = NULL;
#endif
    QUICK_TBLOCK_WITH_BUFFER(quick_tblock, 256);
    quick_tblock_with_buffer_setup(quick_tblock);

    *p_converted_name = NULL;
    *p_t5_filetype_converted = FILETYPE_UNDETERMINED;

#if RISCOS
    /* first save the data to a temp file */
    if(status_ok(status = file_tempname_null(TEXT("cd"), NULL, 0, &quick_tblock)))
    {
        _kernel_osfile_block osfile_block;

        osfile_block.load  = (int) t5_filetype;
        osfile_block.exec  = 0;
        osfile_block.start = (int) p_data;
        osfile_block.end   = osfile_block.start + (int) n_bytes;

        if(_kernel_ERROR == _kernel_osfile(OSFile_SaveStamp, quick_tblock_tstr(&quick_tblock), &osfile_block))
            status = file_error_set(_kernel_last_oserror()->errmess);
    }

    /* and then convert that */
    if(status_ok(status))
        status = image_convert_do_convert_file(p_converted_name, p_t5_filetype_converted, quick_tblock_tstr(&quick_tblock), t5_filetype);

    // <<< (void) _tremove(quick_tblock_tstr(&quick_tblock));
#elif WINDOWS
    /* simply save the data to a temp file, converting on-the-fly */
    if(status_ok(status = file_tempname_null(TEXT("cv"), FILE_EXT_SEP_TSTR TEXT("bmp"), 0, &quick_tblock)))
        status = tstr_set(p_converted_name, quick_tblock_tstr(&quick_tblock));

    if(status_ok(status))
         status = GdipImage_New(&gdip_image);

    if(status_ok(status))
    {
        BOOL ok;

        assert(NULL != gdip_image);

        ok = GdipImage_Load_Memory(gdip_image, p_data, n_bytes, t5_filetype);

        if(ok)
            ok = GdipImage_SaveAs_BMP(gdip_image, _wstr_from_tstr(quick_tblock_tstr(&quick_tblock)));

        if(ok)
            *p_t5_filetype_converted = FILETYPE_BMP;
        else
            status = STATUS_FAIL;

        GdipImage_Dispose(&gdip_image);
    }
#endif /* OS */

    quick_tblock_dispose(&quick_tblock);

    return(status);
}

/******************************************************************************
*
* convert an image file
*
* --out--
* -ve = error
*   0 = converted file exists, caller must delete after reading
*
******************************************************************************/

_Check_return_
extern STATUS
image_convert_do_convert_file(
    _OutRef_    P_PTSTR p_converted_name,
    _OutRef_    P_T5_FILETYPE p_t5_filetype_converted,
    _In_z_      PCTSTR source_file_name,
    _InVal_     T5_FILETYPE t5_filetype)
{
    STATUS status = STATUS_OK;
#if RISCOS
    PCTSTR mode = TEXT("S32,90,90");
#elif WINDOWS
    GdipImage gdip_image = NULL;
#endif
    QUICK_TBLOCK_WITH_BUFFER(quick_tblock, BUF_MAX_PATHSTRING);
    quick_tblock_with_buffer_setup(quick_tblock);

    *p_converted_name = NULL;
    *p_t5_filetype_converted = FILETYPE_UNDETERMINED;

#if RISCOS
    status = file_tempname_null(TEXT("cv"), NULL, 0, &quick_tblock);
#else
    status = file_tempname_null(TEXT("cv"), FILE_EXT_SEP_TSTR TEXT("bmp"), 0, &quick_tblock);
#endif /* OS */

    if(status_ok(status))
        status = tstr_set(p_converted_name, quick_tblock_tstr(&quick_tblock));

    quick_tblock_dispose(&quick_tblock);

    status_return(status);

#if RISCOS
    UNREFERENCED_PARAMETER_InVal_(t5_filetype);

    if(host_os_version_query() < RISCOS_4_0)
        mode = TEXT("28r");

    if( status_ok(status = quick_tblock_tstr_add(&quick_tblock, TEXT("WimpTask Run ")              )) &&
        status_ok(status = quick_tblock_tstr_add(&quick_tblock, ChangeFSI_name_buffer              )) &&
        status_ok(status = quick_tblock_tstr_add(&quick_tblock, TEXT(" ")                          )) &&
        status_ok(status = quick_tblock_tstr_add(&quick_tblock, source_file_name  /* <in file>  */ )) &&
        status_ok(status = quick_tblock_tstr_add(&quick_tblock, TEXT(" ")                          )) &&
        status_ok(status = quick_tblock_tstr_add(&quick_tblock, *p_converted_name /* <out file> */ )) &&
        status_ok(status = quick_tblock_tstr_add(&quick_tblock, TEXT(" ")                          )) &&
        status_ok(status = quick_tblock_tstr_add(&quick_tblock, mode              /* <mode>     */ )) &&
        status_ok(status = quick_tblock_tstr_add_n(&quick_tblock, TEXT(" -nomode -noscale") /* <options> */, strlen_with_NULLCH)) )
    { /*EMPTY*/ }

    if(status_ok(status))
    {   /* check that there will be enough memory in the Window Manager next slot to run ChangeFSI */
        _kernel_swi_regs rs;
        rs.r[0] = -1; /* read */
        rs.r[1] = -1; /* read next slot */
        _kernel_swi(Wimp_SlotSize, &rs, &rs);
#define MIN_NEXT_SLOT (512 * 1024)
        if(rs.r[1] < MIN_NEXT_SLOT)
        {
            rs.r[0] = -1; /* read */
            rs.r[1] = MIN_NEXT_SLOT; /* write next slot */
            _kernel_swi(Wimp_SlotSize, &rs, &rs); /* sorry for the override, but it does need some space to run! */
        }
    }

    if(status_ok(status))
    {
        reportf(quick_tblock_tstr(&quick_tblock));
        _kernel_oscli(quick_tblock_tstr(&quick_tblock));
    }

    if(status_ok(status))
        *p_t5_filetype_converted = FILETYPE_SPRITE;
#elif WINDOWS
    status = GdipImage_New(&gdip_image);

    if(status_ok(status))
    {
        BOOL ok;

        assert(NULL != gdip_image);

        ok = GdipImage_Load_File(gdip_image, _wstr_from_tstr(source_file_name), t5_filetype);

        if(ok)
            ok = GdipImage_SaveAs_BMP(gdip_image, _wstr_from_tstr(quick_tblock_tstr(&quick_tblock)));

        if(ok)
            *p_t5_filetype_converted = FILETYPE_BMP;
        else
            status = STATUS_FAIL;

        GdipImage_Dispose(&gdip_image);
    }
#endif /* OS */

    quick_tblock_dispose(&quick_tblock);

    return(status);
}

#if RISCOS

/* try loading the module - just the once, mind (remember the error too) */

_Check_return_
extern STATUS
image_convert_ensure_PicConvert(void)
{
    static STATUS status = STATUS_OK;

    TCHARZ command_buffer[BUF_MAX_PATHSTRING];
    PTSTR tstr = command_buffer;

    if(STATUS_OK != status)
        return(status);

    tstr_xstrkpy(tstr, elemof32(command_buffer), "%RMLoad ");
    tstr += tstrlen32(tstr);

    status_return(status = file_find_on_path(tstr, elemof32(command_buffer) - (tstr - command_buffer), file_get_resources_path(), TEXT("PicConvert")));

    if(STATUS_OK == status)
        return(status = create_error(FILE_ERR_NOTFOUND));

    if(_kernel_ERROR == _kernel_oscli(command_buffer))
        return(status = status_nomem());

    return(status = STATUS_DONE);
}

#endif /* RISCOS */

/* end of im_convert.c */
