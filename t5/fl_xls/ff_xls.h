/* ff_xls.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1994-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Common header file for Excel spreadsheet load & save modules */

/* SKS April 2014 */

#if defined(__cplusplus)
extern "C" {
#endif

#if RISCOS
/* #defines needed for xlcall.h to compile on RISC OS */
#define FAR /*nothing*/
#define _cdecl /*nothing*/
#define pascal /*nothing*/
#define LPSTR PC_U8Z
#endif

/*
If you get an error here, you must download the
Microsoft Excel 97 SDK files and expand into the
directory external/Microsoft/Excel97SDK
*/

#if defined(__cplusplus) && !defined(bool_defined)
#define bool fBool
#define bool_defined 1
#endif

/* NB Do not redistribute xlcall.h header file with Fireworkz source! */
#include "external/Microsoft/Excel97SDK/INCLUDE/xlcall.h"
/* NB If this #include fails, try
 * Build\w32\setup.cmd (Windows)
 * or
 * Build\setup-ro.sh (on Linux, for RISC OS)
 */

#ifndef xlfIf
#define xlfIf 1
#endif

#if defined(bool_defined)
#undef bool
#undef bool_defined
#endif

/*
Excel record types (see http://sc.openoffice.org/excelfileformat.pdf)
*/

typedef U16 XLS_OPCODE; typedef XLS_OPCODE * P_XLS_OPCODE;

#define X_DIMENSIONS_B2         0x00
#define X_BLANK_B2              0x01
#define X_INTEGER_B2            0x02
#define X_NUMBER_B2             0x03
#define X_LABEL_B2              0x04
#define X_BOOLERR_B2            0x05
#define X_FORMULA_B2_B5_B8      0x06
#define X_STRING_B2             0x07
#define X_ROW_B2                0x08
#define X_BOF_B2                0x09
#define X_EOF                   0x0A
#define X_INDEX_B2              0x0B
#define X_CALCCOUNT             0x0C
#define X_CALCMODE              0x0D
#define X_PRECISION             0x0E
#define X_REFMODE               0x0F

#define X_DELTA                 0x10
#define X_ITERATION             0x11
#define X_PROTECT               0x12
#define X_PASSWORD              0x13
#define X_HEADER                0x14
#define X_FOOTER                0x15
#define X_EXTERNCOUNT_B2_B7     0x16
#define X_EXTERNSHEET           0x17
#define X_DEFINEDNAME_B2_B5_B8  0x18
#define X_WINDOWPROTECT         0x19
#define X_VERTICALPAGEBREAKS    0x1A
#define X_HORIZONTALPAGEBREAKS  0x1B
#define X_NOTE                  0x1C
#define X_SELECTION             0x1D
#define X_FORMAT_B2_B3          0x1E
#define X_BUILTINFMTCOUNT_B2    0x1F

#define X_COLUMNDEFAULT_B2      0x20
#define X_ARRAY_B2              0x21
#define X_DATEMODE              0x22
#define X_EXTERNNAME_B2_B5_B8   0x23
#define X_COLWIDTH_B2           0x24
#define X_DEFAULTROWHEIGHT_B2   0x25
#define X_LEFTMARGIN            0x26
#define X_RIGHTMARGIN           0x27
#define X_TOPMARGIN             0x28
#define X_BOTTOMMARGIN          0x29
#define X_PRINTHEADERS          0x2A
#define X_PRINTGRIDLINES        0x2B
#define X_FILEPASS              0x2F

#define X_FONT_B2_B5_B8         0x31
#define X_TABLEOP_B2            0x36
#define X_TABLEOP2_B2           0x37
#define X_CONTINUE              0x3C
#define X_WINDOW1               0x3D
#define X_WINDOW2_B2            0x3E

#define X_BACKUP                0x40
#define X_PANE                  0x41
#define X_CODEPAGE              0x42
#define X_XF_B2                 0x43
#define X_IXFE_B2               0x44
#define X_EFONT_B2              0x45
#define X_PLS                   0x4D

#define X_DCONREF               0x51
#define X_DEFCOLWIDTH           0x55
#define X_BUILTINFMTCOUNT_B3_B4 0x56
#define X_XCT_B3_B8             0x59
#define X_CRN_B3_B8             0x5A
#define X_FILESHARING_B3_B8     0x5B
#define X_WRITEACCESS_B3_B8     0x5C
#define X_UNCALCED_B3_B8        0x5E
#define X_SAVERECALC_B3_B8      0x5F

#define X_OBJECTPROTECT_B3_B8   0x63

#define X_COLINFO_B3_B8         0x7D

#define X_GUTS_B3_B8            0x80
#define X_WSBOOL_B3_B8          0x81
#define X_GRIDSET_B3_B8         0x82
#define X_HCENTER_B3_B8         0x83
#define X_VCENTER_B3_B8         0x84
#define X_BOUNDSHEET_B5_B8      0x85
#define X_WRITEPROT_B3_B8       0x86
#define X_COUNTRY_B3_B8         0x8C
#define X_HIDEOBJ_B3_B8         0x8D
#define X_SHEETSOFFSET_B4       0x8E
#define X_SHEETHDR_B4           0x8F

#define X_SORT_B5_B8            0x90
#define X_PALETTE_B3_B8         0x92
#define X_STANDARDWIDTH_B4_B8   0x99
#define X_FNGROUPCOUNT          0x9C

#define X_SCL_B4_B8             0xA0
#define X_PAGESETUP_B4_B8       0xA1
#define X_GCW_B4_B7             0xAB

#define X_MULRK_B5_B8           0xBD
#define X_MULBLANK_B5_B8        0xBE

#define X_TOOLBARHDR            0xBF /* from BIFFVIEW */
#define X_TOOLBAREND            0xC0 /* from BIFFVIEW */
#define X_MMS                   0xC1

#define X_RSTRING_B5_B7         0xD6
#define X_DBCELL_B5_B8          0xD7
#define X_BOOKBOOL_B5_B8        0xDA
#define X_SCENPROTECT_B5_B8     0xDD

#define X_XF_B5_B8              0xE0
#define X_INTERFACEHDR          0xE1
#define X_INTERFACEEND          0xE2
#define X_MERGEDCELLS_B8        0xE5
#define X_BITMAP                0xE9
#define X_MSODRAWINGGROUP       0xEB
#define X_MSODRAWING            0xEC
#define X_MSODRAWINGSELECTION   0xED
#define X_PHONETIC_B8           0xEF

#define X_SST_B8                0xFC
#define X_LABELSST_B8           0xFD
#define X_EXTSST_B8             0xFF

#define X_TABID                 0x13D

#define X_LABELRANGES           0x15F

#define X_USESELFS_B8           0x160
#define X_DSF                   0x161

#define X_SUPBOOK_B8            0x1AE
#define X_PROT4REV              0x1AF

#define X_CONDFMT               0x1B0
#define X_CF                    0x1B1
#define X_DVAL                  0x1B2
#define X_REFRESHALL            0x1B7
#define X_HLINK                 0x1B8
#define X_PROT4REVPASS          0x1BC
#define X_DV                    0x1BE

#define X_EXCEL9FILE            0x1C0
#define X_RECALCID              0x1C1

#define X_DIMENSIONS_B3_B8      0x200
#define X_BLANK_B3_B8           0x201
#define X_NUMBER_B3_B8          0x203
#define X_LABEL_B3_B8           0x204
#define X_BOOLERR_B3_B8         0x205
#define X_FORMULA_B3            0x206
#define X_STRING_B3_B8          0x207
#define X_ROW_B3_B8             0x208
#define X_BOF_B3                0x209
#define X_INDEX_B3_B8           0x20B
#define X_DEFINEDNAME_B3_B4     0x218
#define X_ARRAY_B3_B8           0x221
#define X_EXTERNNAME_B3_B4      0x223
#define X_DEFAULTROWHEIGHT_B3_B8 0x225
#define X_FONT_B3_B4            0x231
#define X_TABLEOP_B3_B8         0x236
#define X_WINDOW2_B3_B8         0x23E
#define X_XF_B3                 0x243
#define X_RK_B3_B8              0x27E
#define X_STYLE_B3_B8           0x293

#define X_FORMULA_B4            0x406
#define X_BOF_B4                0x409
#define X_FORMAT_B4_B8          0x41E
#define X_XF_B4                 0x443
#define X_SHRFMLA_B5_B8         0x4BC

#define X_QUICKTIP_B8           0x800
#define X_BOF_B5_B8             0x809
#define X_SHEETLAYOUT_B8        0x862
#define X_BOOKEXT               0x863
#define X_SHEETPROTECTION_B8    0x867
#define X_RANGEPROTECTION       0x868
#define X_XFCRC                 0x87C
#define X_XFEXT                 0x87D
#define X_PLV12                 0x88B
#define X_COMPAT12              0x88C
#define X_TABLESTYLES           0x88E
#define X_STYLEEXT              0x892
#define X_THEME                 0x896
#define X_MTRSETTINGS           0x89A
#define X_COMPRESSPICTURES      0x89B
#define X_HEADERFOOTER          0x89C
#define X_FORCEFULLCALCULATION  0x8A3

/*
Excel Tokens (see http://sc.openoffice.org/excelfileformat.pdf)
*/

#define tNotUsed        0x00
#define tExp            0x01
#define tTbl            0x02
#define tAdd            0x03
#define tSub            0x04
#define tMul            0x05
#define tDiv            0x06
#define tPower          0x07
#define tConcat         0x08
#define tLT             0x09
#define tLE             0x0A
#define tEQ             0x0B
#define tGE             0x0C
#define tGT             0x0D
#define tNE             0x0E
#define tIsect          0x0F
#define tList           0x10
#define tRange          0x11
#define tUplus          0x12
#define tUminus         0x13
#define tPercent        0x14
#define tParen          0x15
#define tMissArg        0x16
#define tStr            0x17
#define tNlr            0x18
#define tAttr           0x19
#define tSheet          0x1A
#define tEndSheet       0x1B
#define tErr            0x1C
#define tBool           0x1D
#define tInt            0x1E
#define tNum            0x1F

#define tArrayR         0x20
#define tFuncR          0x21
#define tFuncVarR       0x22
#define tNameR          0x23
#define tRefR           0x24
#define tAreaR          0x25
#define tMemAreaR       0x26
#define tMemErrR        0x27
#define tMemNoMemR      0x28
#define tMemFuncR       0x29
#define tRefErrR        0x2A
#define tAreaErrR       0x2B
#define tRefNR          0x2C
#define tAreaNR         0x2D
#define tMemAreaNR      0x2E
#define tMemNoMemNR     0x2F
/* 0x30..0x37 not used */
#define tFuncCER        0x38
#define tNameXR         0x39
#define tRef3dR         0x3A
#define tArea3dR        0x3B
#define tRefErr3dR      0x3C
#define tAreaErr3dR     0x3D
/* 0x3E..0x3F not used */

#define tArrayV         0x40
#define tFuncV          0x41
#define tFuncVarV       0x42
#define tNameV          0x43
#define tRefV           0x44
#define tAreaV          0x45
#define tMemAreaV       0x46
#define tMemErrV        0x47
#define tMemNoMemV      0x48
#define tMemFuncV       0x49
#define tRefErrV        0x4A
#define tAreaErrV       0x4B
#define tRefNV          0x4C
#define tAreaNV         0x4D
#define tMemAreaNV      0x4E
#define tMemNoMemNV     0x4F
/* 0x50..0x57 not used */
#define tFuncCEV        0x58
#define tNameXV         0x59
#define tRef3dV         0x5A
#define tArea3dV        0x5B
#define tRefErr3dV      0x5C
#define tAreaErr3dV     0x5D
/* 0x5E..0x5F not used */

#define tArrayA         0x60
#define tFuncA          0x61
#define tFuncVarA       0x62
#define tNameA          0x63
#define tRefA           0x64
#define tAreaA          0x65
#define tMemAreaA       0x66
#define tMemErrA        0x67
#define tMemNoMemA      0x68
#define tMemFuncA       0x69
#define tRefErrA        0x6A
#define tAreaErrA       0x6B
#define tRefNA          0x6C
#define tAreaNA         0x6D
#define tMemAreaNA      0x6E
#define tMemNoMemNA     0x6F
/* 0x70..0x77 not used */
#define tFuncCEA        0x78
#define tNameXA         0x79
#define tRef3dA         0x7A
#define tArea3dA        0x7B
#define tRefErr3dA      0x7C
#define tAreaErr3dA     0x7D
/* 0x7E..0x7F not used */

/* XLS_COL and XLS_ROW are sized to accommodate largest DIMENSIONS record members */

typedef U16 XLS_COL; typedef XLS_COL * P_XLS_COL;

#define XLS_MAXCOL_BIFF2    256 /* BIFF2..BIFF7 8 bits */
#define XLS_MAXCOL_BIFF8    256 /* still just 8 bits */

typedef U32 XLS_ROW; typedef XLS_ROW * P_XLS_ROW;

#define XLS_MAXROW_BIFF2    16384 /* BIFF2..BIFF7 14 bits */
#define XLS_MAXROW_BIFF8    65536 /* BIFF8 16 bits */

#if defined(__cplusplus)
}
#endif

/* end of ff_xls.h */
