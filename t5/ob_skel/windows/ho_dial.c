/* windows/ho_dial.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

/* exports from Dial Solutions Draw DLLs */

#include "external/Dial_Solutions/drawfile.h"

/* exports needed for Dial Solutions Draw DLLs */

extern
#if defined(__cplusplus)
"C"
#endif
void
host_gdiplus_startup(void);

extern
#if defined(__cplusplus)
"C"
#endif
void
host_gdiplus_shutdown(void);

static STATUS drawfile_status = STATUS_FAIL;

static HMODULE dsff9_lib;

static HMODULE dsfntmap_lib;

static HMODULE dsaff_lib;

_Check_return_
static STATUS
load_drawfile_dlls(void);

_Check_return_
static STATUS
_ensure_drawfile_dlls(void)
{
    if(drawfile_status != STATUS_FAIL)
    {
        return(drawfile_status);
    }

    host_gdiplus_startup(); /* New DLLs require GDI+ */

    drawfile_status = load_drawfile_dlls();

    return(drawfile_status);
}

_Check_return_
static inline STATUS
ensure_drawfile_dlls(void)
{
    if(drawfile_status != STATUS_FAIL)
    {
        return(drawfile_status);
    }

    return(_ensure_drawfile_dlls());
}

/*---
Draw_PathBbox : calculates given path's bounding box
---*/

/*ncr*/
static BOOL (WINAPI *
p_proc_Draw_PathBbox) (
    HPLONG path,
    unsigned long fill_style,
    unsigned long thickness,
    draw_dashstr far *dash,
    draw_capjoinspec far *cjspec,
    HPDRAWBOX bbox,
    draw_error far *e);

/*ncr*/
extern BOOL FAR PASCAL
Draw_PathBbox(
    HPLONG path,
    unsigned long fill_style,
    unsigned long thickness,
    draw_dashstr far *dash,
    draw_capjoinspec far *cjspec,
    HPDRAWBOX bbox,
    draw_error far *e)
{
    if(status_ok(ensure_drawfile_dlls()) && (NULL != p_proc_Draw_PathBbox))
        return((* p_proc_Draw_PathBbox) (path, fill_style, thickness, dash, cjspec, bbox, e));

    zero_struct_ptr(e);
    return(FALSE);
}

/*---
Draw_GetAccurateTextBbox : calculates the bounding box for the given text object
---*/

static void (WINAPI *
p_proc_Draw_GetAccurateTextBbox) (
    HWND w,draw_diag far *d,long offset,HPDRAWMATRIX tx,HPDRAWMATRIX inv_tx,draw_box far *bb);

extern void FAR PASCAL
Draw_GetAccurateTextBbox(
    HWND w,draw_diag far *d,long offset,HPDRAWMATRIX tx,HPDRAWMATRIX inv_tx,draw_box far *bb)
{
    if(status_ok(ensure_drawfile_dlls()) && (NULL != p_proc_Draw_GetAccurateTextBbox))
        (* p_proc_Draw_GetAccurateTextBbox) (w, d, offset, tx, inv_tx, bb);
}

/*---
Draw_InverseTransform : calculates the inverse transformation of 't' and returns it in 'inv'
                        't' and 'inv' can be the same pointer with no side effects
---*/

static void (WINAPI *
p_proc_Draw_InverseTransform) (
    HPDRAWMATRIX t,
    HPDRAWMATRIX inv);

extern void FAR PASCAL
Draw_InverseTransform(
    HPDRAWMATRIX t,
    HPDRAWMATRIX inv)
{
    if(status_ok(ensure_drawfile_dlls()) && (NULL != p_proc_Draw_InverseTransform))
        (* p_proc_Draw_InverseTransform) (t, inv);
}

/*---
Draw_RenderDiag: draws the given drawfile on the given device context
                 r : pointer to rectangle to redraw in device context units
                 t : transformation matrix which maps the drawfile onto the device context
                 e : any returned errors (not properly implemented yet, just use function return value
                     to determine success or not)
---*/

/*ncr*/
static BOOL (WINAPI *
p_proc_Draw_RenderDiag) (
    draw_diag far *d,
    HDC dc,
    LPRECT r,
    HPDRAWMATRIX t,
    draw_error far *e);

/*ncr*/
extern BOOL FAR PASCAL
Draw_RenderDiag(
    draw_diag far *d,
    HDC dc,
    LPRECT r,
    HPDRAWMATRIX t,
    draw_error far *e)
{
    if(status_ok(ensure_drawfile_dlls()) && (NULL != p_proc_Draw_RenderDiag))
        return((* p_proc_Draw_RenderDiag) (d, dc, r, t, e));

    zero_struct_ptr(e);
    return(FALSE);
}

/*---
Draw_RenderRange: draws the given range of objects on the given device context
                  r,t,e    : as for Draw_RenderDiag
                  from, to : offsets to first and last object to draw
                             -can use DRAWOBJ_FIRST and DRAWOBJ_LAST as arguments
                  Note that no checking is done on 'from' or 'to' so it is up to the caller
                  to make sure they are valid offsets
---*/

/*ncr*/
static BOOL (WINAPI *
p_proc_Draw_RenderRange) (
    draw_diag far *d,
    HDC dc,
    LPRECT r,
    HPDRAWMATRIX t,
    long from,
    long to,
    draw_error far *e);

/*ncr*/
extern BOOL FAR PASCAL
Draw_RenderRange(
    draw_diag far *d,
    HDC dc,
    LPRECT r,
    HPDRAWMATRIX t,
    long from,
    long to,
    draw_error far *e)
{
    if(status_ok(ensure_drawfile_dlls()) && (NULL != p_proc_Draw_RenderRange))
        return((* p_proc_Draw_RenderRange) (d, dc, r, t, from, to, e));

    zero_struct_ptr(e);
    return(FALSE);
}

/*---
Draw_SetColourMapping called as follows:
 ...(DRAW_COLOURDITHER,(COLORREF)0)  ==> matches colours using dithering
                                         uses colours in drawfile
 ...(DRAW_COLOURNEAREST,(COLORREF)0) ==> matches colours to nearest in palette
                                         uses colours in drawfile
 ...(DRAW_COLOURDITHER | DRAW_COLOURFIXED,RGB(r,g,b))
                                     ==> matches colours using dithering
                                         renders all objects in given colour
 ...(DRAW_COLOURNEAREST | DRAW_COLOURFIXED,RGB(r,g,b))
                                     ==> matches colours to nearest in palette
                                         renders all objects in given colour
 ...({nearest/fixed/dither} | DRAW_COLOURNOFILL,colour)
                                     ==> does not fill paths regardless of what is stored in path
 ...((nearest/fixed/dither} | DRAW_COLOURNOEDGE,colour)
                                     ==> does not draw path outline regardless of what is stored in path
N.B. always call this before calling Draw_RenderDiag or Draw_RenderRange because another
     application may change the mapping
---*/

static BOOL (WINAPI *
p_proc_Draw_SetColourMapping) (
    UINT maptype,
    COLORREF fixed);

extern void FAR PASCAL
Draw_SetColourMapping(
    UINT maptype,
    COLORREF fixed)
{
    if(status_ok(ensure_drawfile_dlls()) && (NULL != p_proc_Draw_SetColourMapping))
        (* p_proc_Draw_SetColourMapping) (maptype, fixed);
}

/*---
Draw_SetFontTables : sets up the offsets to the font table objects for the given draw file
                     use before calling Draw_RenderRange
---*/

/*ncr*/
static BOOL (WINAPI *
p_proc_Draw_SetFontTables) (
    draw_diag far *d);

/*ncr*/
extern BOOL FAR PASCAL
Draw_SetFontTables(
    draw_diag far *d)
{
    if(status_ok(ensure_drawfile_dlls()) && (NULL != p_proc_Draw_SetFontTables))
        return((* p_proc_Draw_SetFontTables) (d));

    return(FALSE);
}

/*---
Draw_VerifyDiag : checks for errors in the given drawfile
                  parent : parent window handle
                  rebind : currently ignored
                  AddWinFontTable : if TRUE, adds a windows font table object to the drawfile
                                    (of type draw_OBJWINFONTLIST)
                  UpdateFontMap   : if TRUE, then the user will be asked to select fonts which
                                    have the same characteristics as any risc os fonts which are
                                    not in 'Fontmap.INI'
---*/

/*ncr*/
static BOOL (WINAPI *
p_proc_Draw_VerifyDiag) (
    HWND parent,
    draw_diag far *d,
    BOOL rebind,
    BOOL AddWinFontTable,
    BOOL UpdateFontMap,
    draw_error far *e);

/*ncr*/
extern BOOL FAR PASCAL
Draw_VerifyDiag(
    HWND parent,
    draw_diag far *d,
    BOOL rebind,
    BOOL AddWinFontTable,
    BOOL UpdateFontMap,
    draw_error far *e)
{
    if(status_ok(ensure_drawfile_dlls()) && (NULL != p_proc_Draw_VerifyDiag))
        return((* p_proc_Draw_VerifyDiag) (parent, d, rebind, AddWinFontTable, UpdateFontMap, e));

    zero_struct_ptr(e);
    return(FALSE);
}

#if 0
static void (WINAPI *
p_proc_Draw_GetLogFont) (
    int n,
    LOGFONT far *lf);

extern void FAR PASCAL
Draw_GetLogFont(
    int n,
    LOGFONT far *lf)
{
    if(status_ok(ensure_drawfile_dlls()) && (NULL != p_proc_Draw_GetLogFont))
        (* p_proc_Draw_GetLogFont) (n, lf);
}
#endif

/*
Load Draw file rendering DLLs
*/

enum DSAPPLIB_FNS /* 0xE0 / 4 */
{
    DSAPPLIB_FN_LoadDLL,
    DSAPPLIB_FN_LoadFN,
    DSAPPLIB_FN_GlobalFree0,
    DSAPPLIB_FN_Alloc,
    DSAPPLIB_FN_AllocB,
    DSAPPLIB_FN_AllocCopy,
    DSAPPLIB_FN_AllocCopyZS,
    DSAPPLIB_FN_AllocLen,
    DSAPPLIB_FN_AllocEnd,
    DSAPPLIB_FN_Free,
    DSAPPLIB_FN_Free0,
    DSAPPLIB_FN_ReAlloc,
    DSAPPLIB_FN_ReAllocB,
    DSAPPLIB_FN_ReportError,
    DSAPPLIB_FN_Say,
    DSAPPLIB_FN_Ask,
    DSAPPLIB_FN_ReportLastError,
    DSAPPLIB_FN_tracef,
    DSAPPLIB_FN_DegToRad,
    DSAPPLIB_FN_RadToDeg,
    DSAPPLIB_FN_PointInBox,
    DSAPPLIB_FN_SetInvalidBBox,
    DSAPPLIB_FN_BoxInBox,
    DSAPPLIB_FN_BoxUnion,
    DSAPPLIB_FN_NormaliseBox,
    DSAPPLIB_FN_BoxesOverlap,
    DSAPPLIB_FN_WALen,
    DSAPPLIB_FN_SetIdentityMatrix,
    DSAPPLIB_FN_NextToken,
    DSAPPLIB_FN_LoadStringV,
    DSAPPLIB_FN_GetParentDir,
    DSAPPLIB_FN_MakeDir,
    DSAPPLIB_FN_GetLeafNameA,
    DSAPPLIB_FN_GetLeafNameW,
    DSAPPLIB_FN_FileExists,
    DSAPPLIB_FN_AllocLoad,
    DSAPPLIB_FN_GlobalAllocLoad,
    DSAPPLIB_FN_SaveMem,
    DSAPPLIB_FN_MakeUniqueFileName,
    DSAPPLIB_FN_GetWindowsVersion,
    DSAPPLIB_FN_OpenDBox,
    DSAPPLIB_FN_CloseW,
    DSAPPLIB_FN_DrawRectangle,
    DSAPPLIB_FN_Pause,
    DSAPPLIB_FN_AlreadyRunning,
    DSAPPLIB_FN_TraceLastError,
    DSAPPLIB_FN_IsAsciiText,
    DSAPPLIB_FN_ConvertAsciiToUnicode,
    DSAPPLIB_FN_MemCopy,
    DSAPPLIB_FN_MemMove,
    DSAPPLIB_FN_ConvertUnicodeToAscii,
    DSAPPLIB_FN_LoadStandardDLL,
    DSAPPLIB_FN_UnloadStandardDLL,
    DSAPPLIB_FN_logfile_create_19DC,
    DSAPPLIB_FN_logfile_printf_1A0E
};

static FARPROC dsapplib_functions[256];

/*
exported for use by aligator.c
*/

_Check_return_
_Ret_writes_to_maybenull_(n_bytes, 0) /* may be NULL */
extern P_BYTE
_dsapplib_ptr_alloc(
    _InVal_     U32 n_bytes,
    _OutRef_    P_STATUS p_status)
{
    void * (PASCAL * p_proc_Alloc) (U32) = NULL;
    P_BYTE ptr = NULL;

    if(status_ok(*p_status = ensure_drawfile_dlls()) && (NULL != (* (FARPROC *) &p_proc_Alloc = dsapplib_functions[DSAPPLIB_FN_Alloc])))
    {
        ptr = (P_BYTE) (* p_proc_Alloc)(n_bytes);

        *p_status = (NULL != ptr) ? STATUS_OK : STATUS_NOMEM;
    }

    return(ptr);
}

extern void
dsapplib_ptr_free(
    _Pre_maybenull_ _Post_invalid_ P_ANY p_any)
{
    void (PASCAL * p_proc_Free) (void *) = NULL;

    if(NULL == p_any)
        return;

    /* no need to ensure_drawfile_dlls() */
    if(NULL != (* (FARPROC *) &p_proc_Free = dsapplib_functions[DSAPPLIB_FN_Free]))
        (* p_proc_Free)(p_any);
}

_Check_return_
_Ret_writes_to_maybenull_(n_bytes, 0) /* may be NULL */
extern P_BYTE
_dsapplib_ptr_realloc(
    _Pre_maybenull_ _Post_invalid_ P_ANY p_any,
    _InVal_     U32 n_bytes,
    _OutRef_    P_STATUS p_status)
{
    void * (PASCAL * p_proc_ReAlloc) (void *, U32) = NULL;
    P_BYTE ptr = NULL;

    if(status_ok(*p_status = ensure_drawfile_dlls()) && (NULL != (* (FARPROC *) &p_proc_ReAlloc = dsapplib_functions[DSAPPLIB_FN_ReAlloc])))
    {
        ptr = (P_BYTE) (* p_proc_ReAlloc)(p_any, n_bytes);

        *p_status = (NULL != ptr) ? STATUS_OK : STATUS_NOMEM;
    }

    return(ptr);
}

enum DSAFF_FNS
{
    DSAFF_FN_Terminate = 0,
    DSAFF_FN_Draw_RenderDiag,
    DSAFF_FN_Draw_GetDiagBbox,
    DSAFF_FN_Draw_TransformPoint,
    DSAFF_FN_Draw_AllocBuffer,
    DSAFF_FN_Draw_LockBuffer,
    DSAFF_FN_Draw_UnlockBuffer,
    DSAFF_FN_Draw_FreeBuffer,
    DSAFF_FN_Draw_ExtendBuffer,
    DSAFF_FN_Draw_Stroke,
    DSAFF_FN_DrawPath_Init,
    DSAFF_FN_DrawPath_End,
    DSAFF_FN_Draw_SetColourMapping,
    DSAFF_FN_Draw_MatrixMultiply,
    DSAFF_FN_Draw_TransformPath,
    DSAFF_FN_Draw_PathBbox,
    DSAFF_FN_Draw_FirstRenderable,
    DSAFF_FN_Draw_SetPathBbox,
    DSAFF_FN_ColorRefToDraw,
    DSAFF_FN_Path_HeaderSize,
    DSAFF_FN_Draw_RenderDiagInMetaFile,
    DSAFF_FN_Draw_TransformX,
    DSAFF_FN_Draw_TransformY,
    DSAFF_FN_DrawToColorRef,
    DSAFF_FN_IsValidMemmove,
    DSAFF_FN_Draw_VerifyDiag,
    DSAFF_FN_Draw_ObjectHasBbox,
    DSAFF_FN_Draw_SetLogFontSize,
    DSAFF_FN_Draw_SetTextBbox,
    DSAFF_FN_Draw_TextDescent,
    DSAFF_FN_Draw_SetFontTables,
    DSAFF_FN_Draw_GetLogFont,
    DSAFF_FN_Draw_RenderRange,
    DSAFF_FN_Draw_InverseTransform,
    DSAFF_FN_Draw_SetAccurateTextBbox,
    DSAFF_FN_Draw_GetRiscOSToWindowsTextWidthFactor,
    DSAFF_FN_Draw_GetAccurateTextBbox,
    DSAFF_FN_Draw_SetSpriteBbox,
    DSAFF_FN_Draw_FlattenPath,
    DSAFF_FN_Draw_SetMetaFileDPI,
    DSAFF_FN_Draw_GetTextSlab,
    DSAFF_FN_Draw_GetTextBaseline,
    DSAFF_FN_Draw_GetSpriteSlab,
    DSAFF_FN_Draw_TransformPoints,
    DSAFF_FN_DrawText_SetFontTablePointers,
    DSAFF_FN_round,
    DSAFF_FN_Draw_Render,
    DSAFF_FN_Draw_DoubleToFptyp,
    DSAFF_FN_Draw_FptypToDouble,
    DSAFF_FN_DrawText_Init,
    DSAFF_FN_Draw_ConvertDouble
};

static FARPROC dsaff_functions[256];

_Check_return_
static STATUS
load_drawfile_dlls(void)
{
    TCHARZ name[BUF_MAX_PATHSTRING];
    HMODULE df = NULL;
    BOOL (PASCAL * p_proc_Initialise) (void)  = NULL;
    BOOL (PASCAL * p_proc_LoadFunctions) (HMODULE hModule, FARPROC *);

	/* Load DLLs in sequence from local resource */
    if(status_done(file_find_on_path(name, elemof32(name), TEXT("DLL") FILE_DIR_SEP_TSTR TEXT("DSAppLib.dll"))))
    {
        df = LoadLibrary(name);
        if(NULL != df)
        {
            * (FARPROC *) &p_proc_Initialise = GetProcAddress(df, "Initialise");
            if(p_proc_Initialise) consume_bool(WrapOsBoolReporting(p_proc_Initialise()));

            * (FARPROC *) &p_proc_LoadFunctions = GetProcAddress(df, "LoadFunctions");
            if(p_proc_LoadFunctions) consume_bool(WrapOsBoolReporting(p_proc_LoadFunctions(df, dsapplib_functions)));
        }
    }

    if((NULL != df) && status_done(file_find_on_path(name, elemof32(name), TEXT("DLL") FILE_DIR_SEP_TSTR TEXT("DSFF9.dll"))))
    {
        dsff9_lib = df = LoadLibrary(name);
        if(NULL != df)
        {
            * (FARPROC *) &p_proc_Initialise = GetProcAddress(df, "Initialise");
            if(p_proc_Initialise) consume_bool(WrapOsBoolReporting(p_proc_Initialise()));
        }
    }

    if((NULL != df) && status_done(file_find_on_path(name, elemof32(name), TEXT("DLL") FILE_DIR_SEP_TSTR TEXT("DSFntMap.dll"))))
    {
        dsfntmap_lib = df = LoadLibrary(name);
        if(NULL != df)
        {
            * (FARPROC *) &p_proc_Initialise = GetProcAddress(df, "Initialise");
            if(p_proc_Initialise) consume_bool(WrapOsBoolReporting(p_proc_Initialise()));
        }
    }

    if((NULL != df) && status_done(file_find_on_path(name, elemof32(name), TEXT("DLL") FILE_DIR_SEP_TSTR TEXT("dsaff.dll"))))
    {
        dsaff_lib = df = LoadLibrary(name);
        if(NULL != df)
        {
            * (FARPROC *) &p_proc_Initialise = GetProcAddress(df, "Initialise");
            if(p_proc_Initialise) WrapOsBoolReporting(p_proc_Initialise());

            * (FARPROC *) &p_proc_LoadFunctions = GetProcAddress(df, "LoadFunctions");
            if(p_proc_LoadFunctions) consume_bool(WrapOsBoolReporting(p_proc_LoadFunctions(df, dsaff_functions)));
        }
    }

    if(dsaff_lib)
    {
        * (FARPROC *) &p_proc_Draw_PathBbox             = dsaff_functions[DSAFF_FN_Draw_PathBbox];
        * (FARPROC *) &p_proc_Draw_GetAccurateTextBbox  = dsaff_functions[DSAFF_FN_Draw_GetAccurateTextBbox];
        * (FARPROC *) &p_proc_Draw_InverseTransform     = dsaff_functions[DSAFF_FN_Draw_InverseTransform];
        * (FARPROC *) &p_proc_Draw_RenderDiag           = dsaff_functions[DSAFF_FN_Draw_RenderDiag];
        * (FARPROC *) &p_proc_Draw_RenderRange          = dsaff_functions[DSAFF_FN_Draw_RenderRange];
        * (FARPROC *) &p_proc_Draw_SetColourMapping     = dsaff_functions[DSAFF_FN_Draw_SetColourMapping];
        * (FARPROC *) &p_proc_Draw_SetFontTables        = dsaff_functions[DSAFF_FN_Draw_SetFontTables];
        * (FARPROC *) &p_proc_Draw_VerifyDiag           = dsaff_functions[DSAFF_FN_Draw_VerifyDiag];
    }

    if(NULL != p_proc_Draw_PathBbox)
        return(STATUS_OK);

    return(create_error(ERR_DRAWFILE_FAILURE));
}

extern void
ho_dial_msg_exit2(void)
{
    if(NULL != dsaff_lib)
    {
        HINSTANCE hInstance = dsaff_lib;
        dsaff_lib = NULL;
        FreeLibrary(hInstance);
    }

    if(NULL != dsfntmap_lib)
    {
        HINSTANCE hInstance = dsfntmap_lib;
        dsfntmap_lib = NULL;
        FreeLibrary(hInstance);
    }

    if(NULL != dsff9_lib)
    {
        HINSTANCE hInstance = dsff9_lib;
        dsff9_lib = NULL;
        FreeLibrary(hInstance);
    }

    host_gdiplus_shutdown();
}

/* end of windows/ho_dial.c */
