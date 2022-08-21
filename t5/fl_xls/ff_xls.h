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

typedef enum _BIFF_function_number
{
    BIFF_FN_Count = 0,
    BIFF_FN_If = 1,
    BIFF_FN_Isna = 2,
    BIFF_FN_Iserror = 3,
    BIFF_FN_Sum = 4,
    BIFF_FN_Average = 5,
    BIFF_FN_Min = 6,
    BIFF_FN_Max = 7,
    BIFF_FN_Row = 8,
    BIFF_FN_Column = 9,
    BIFF_FN_Na = 10,
    BIFF_FN_Npv = 11,
    BIFF_FN_Stdev = 12,
    BIFF_FN_Dollar = 13,
    BIFF_FN_Fixed = 14,
    BIFF_FN_Sin = 15,
    BIFF_FN_Cos = 16,
    BIFF_FN_Tan = 17,
    BIFF_FN_Atan = 18,
    BIFF_FN_Pi = 19,
    BIFF_FN_Sqrt = 20,
    BIFF_FN_Exp = 21,
    BIFF_FN_Ln = 22,
    BIFF_FN_Log10 = 23,
    BIFF_FN_Abs = 24,
    BIFF_FN_Int = 25,
    BIFF_FN_Sign = 26,
    BIFF_FN_Round = 27,
    BIFF_FN_Lookup = 28,
    BIFF_FN_Index = 29,
    BIFF_FN_Rept = 30,
    BIFF_FN_Mid = 31,
    BIFF_FN_Len = 32,
    BIFF_FN_Value = 33,
    BIFF_FN_True = 34,
    BIFF_FN_False = 35,
    BIFF_FN_And = 36,
    BIFF_FN_Or = 37,
    BIFF_FN_Not = 38,
    BIFF_FN_Mod = 39,
    BIFF_FN_Dcount = 40,
    BIFF_FN_Dsum = 41,
    BIFF_FN_Daverage = 42,
    BIFF_FN_Dmin = 43,
    BIFF_FN_Dmax = 44,
    BIFF_FN_Dstdev = 45,
    BIFF_FN_Var = 46,
    BIFF_FN_Dvar = 47,
    BIFF_FN_Text = 48,
    BIFF_FN_Linest = 49,
    BIFF_FN_Trend = 50,
    BIFF_FN_Logest = 51,
    BIFF_FN_Growth = 52,
    BIFF_FN_Goto = 53,
    BIFF_FN_Halt = 54,
    BIFF_FN_Pv = 56,
    BIFF_FN_Fv = 57,
    BIFF_FN_Nper = 58,
    BIFF_FN_Pmt = 59,
    BIFF_FN_Rate = 60,
    BIFF_FN_Mirr = 61,
    BIFF_FN_Irr = 62,
    BIFF_FN_Rand = 63,
    BIFF_FN_Match = 64,
    BIFF_FN_Date = 65,
    BIFF_FN_Time = 66,
    BIFF_FN_Day = 67,
    BIFF_FN_Month = 68,
    BIFF_FN_Year = 69,
    BIFF_FN_Weekday = 70,
    BIFF_FN_Hour = 71,
    BIFF_FN_Minute = 72,
    BIFF_FN_Second = 73,
    BIFF_FN_Now = 74,
    BIFF_FN_Areas = 75,
    BIFF_FN_Rows = 76,
    BIFF_FN_Columns = 77,
    BIFF_FN_Offset = 78,
    BIFF_FN_Absref = 79,
    BIFF_FN_Relref = 80,
    BIFF_FN_Argument = 81,
    BIFF_FN_Search = 82,
    BIFF_FN_Transpose = 83,
    BIFF_FN_Error = 84,
    BIFF_FN_Step = 85,
    BIFF_FN_Type = 86,
    BIFF_FN_Echo = 87,
    BIFF_FN_SetName = 88,
    BIFF_FN_Caller = 89,
    BIFF_FN_Deref = 90,
    BIFF_FN_Windows = 91,
    BIFF_FN_Series = 92,
    BIFF_FN_Documents = 93,
    BIFF_FN_ActiveCell = 94,
    BIFF_FN_Selection = 95,
    BIFF_FN_Result = 96,
    BIFF_FN_Atan2 = 97,
    BIFF_FN_Asin = 98,
    BIFF_FN_Acos = 99,
    BIFF_FN_Choose = 100,
    BIFF_FN_Hlookup = 101,
    BIFF_FN_Vlookup = 102,
    BIFF_FN_Links = 103,
    BIFF_FN_Input = 104,
    BIFF_FN_Isref = 105,
    BIFF_FN_GetFormula = 106,
    BIFF_FN_GetName = 107,
    BIFF_FN_SetValue = 108,
    BIFF_FN_Log = 109,
    BIFF_FN_Exec = 110,
    BIFF_FN_Char = 111,
    BIFF_FN_Lower = 112,
    BIFF_FN_Upper = 113,
    BIFF_FN_Proper = 114,
    BIFF_FN_Left = 115,
    BIFF_FN_Right = 116,
    BIFF_FN_Exact = 117,
    BIFF_FN_Trim = 118,
    BIFF_FN_Replace = 119,
    BIFF_FN_Substitute = 120,
    BIFF_FN_Code = 121,
    BIFF_FN_Names = 122,
    BIFF_FN_Directory = 123,
    BIFF_FN_Find = 124,
    BIFF_FN_Cell = 125,
    BIFF_FN_Iserr = 126,
    BIFF_FN_Istext = 127,
    BIFF_FN_Isnumber = 128,
    BIFF_FN_Isblank = 129,
    BIFF_FN_T = 130,
    BIFF_FN_N = 131,
    BIFF_FN_Fopen = 132,
    BIFF_FN_Fclose = 133,
    BIFF_FN_Fsize = 134,
    BIFF_FN_Freadln = 135,
    BIFF_FN_Fread = 136,
    BIFF_FN_Fwriteln = 137,
    BIFF_FN_Fwrite = 138,
    BIFF_FN_Fpos = 139,
    BIFF_FN_Datevalue = 140,
    BIFF_FN_Timevalue = 141,
    BIFF_FN_Sln = 142,
    BIFF_FN_Syd = 143,
    BIFF_FN_Ddb = 144,
    BIFF_FN_GetDef = 145,
    BIFF_FN_Reftext = 146,
    BIFF_FN_Textref = 147,
    BIFF_FN_Indirect = 148,
    BIFF_FN_Register = 149,
    BIFF_FN_Call = 150,
    BIFF_FN_AddBar = 151,
    BIFF_FN_AddMenu = 152,
    BIFF_FN_AddCommand = 153,
    BIFF_FN_EnableCommand = 154,
    BIFF_FN_CheckCommand = 155,
    BIFF_FN_RenameCommand = 156,
    BIFF_FN_ShowBar = 157,
    BIFF_FN_DeleteMenu = 158,
    BIFF_FN_DeleteCommand = 159,
    BIFF_FN_GetChartItem = 160,
    BIFF_FN_DialogBox = 161,
    BIFF_FN_Clean = 162,
    BIFF_FN_Mdeterm = 163,
    BIFF_FN_Minverse = 164,
    BIFF_FN_Mmult = 165,
    BIFF_FN_Files = 166,
    BIFF_FN_Ipmt = 167,
    BIFF_FN_Ppmt = 168,
    BIFF_FN_Counta = 169,
    BIFF_FN_CancelKey = 170,
    BIFF_FN_Initiate = 175,
    BIFF_FN_Request = 176,
    BIFF_FN_Poke = 177,
    BIFF_FN_Execute = 178,
    BIFF_FN_Terminate = 179,
    BIFF_FN_Restart = 180,
    BIFF_FN_Help = 181,
    BIFF_FN_GetBar = 182,
    BIFF_FN_Product = 183,
    BIFF_FN_Fact = 184,
    BIFF_FN_GetCell = 185,
    BIFF_FN_GetWorkspace = 186,
    BIFF_FN_GetWindow = 187,
    BIFF_FN_GetDocument = 188,
    BIFF_FN_Dproduct = 189,
    BIFF_FN_Isnontext = 190,
    BIFF_FN_GetNote = 191,
    BIFF_FN_Note = 192,
    BIFF_FN_Stdevp = 193,
    BIFF_FN_Varp = 194,
    BIFF_FN_Dstdevp = 195,
    BIFF_FN_Dvarp = 196,
    BIFF_FN_Trunc = 197,
    BIFF_FN_Islogical = 198,
    BIFF_FN_Dcounta = 199,
    BIFF_FN_DeleteBar = 200,
    BIFF_FN_Unregister = 201,
    BIFF_FN_Usdollar = 204,
    BIFF_FN_Findb = 205,
    BIFF_FN_Searchb = 206,
    BIFF_FN_Replaceb = 207,
    BIFF_FN_Leftb = 208,
    BIFF_FN_Rightb = 209,
    BIFF_FN_Midb = 210,
    BIFF_FN_Lenb = 211,
    BIFF_FN_Roundup = 212,
    BIFF_FN_Rounddown = 213,
    BIFF_FN_Asc = 214,
    BIFF_FN_Dbcs = 215,
    BIFF_FN_Rank = 216,
    BIFF_FN_Address = 219,
    BIFF_FN_Days360 = 220,
    BIFF_FN_Today = 221,
    BIFF_FN_Vdb = 222,
    BIFF_FN_Median = 227,
    BIFF_FN_Sumproduct = 228,
    BIFF_FN_Sinh = 229,
    BIFF_FN_Cosh = 230,
    BIFF_FN_Tanh = 231,
    BIFF_FN_Asinh = 232,
    BIFF_FN_Acosh = 233,
    BIFF_FN_Atanh = 234,
    BIFF_FN_Dget = 235,
    BIFF_FN_CreateObject = 236,
    BIFF_FN_Volatile = 237,
    BIFF_FN_LastError = 238,
    BIFF_FN_CustomUndo = 239,
    BIFF_FN_CustomRepeat = 240,
    BIFF_FN_FormulaConvert = 241,
    BIFF_FN_GetLinkInfo = 242,
    BIFF_FN_TextBox = 243,
    BIFF_FN_Info = 244,
    BIFF_FN_Group = 245,
    BIFF_FN_GetObject = 246,
    BIFF_FN_Db = 247,
    BIFF_FN_Pause = 248,
    BIFF_FN_Resume = 251,
    BIFF_FN_Frequency = 252,
    BIFF_FN_AddToolbar = 253,
    BIFF_FN_DeleteToolbar = 254,
    BIFF_FN_ResetToolbar = 256,
    BIFF_FN_Evaluate = 257,
    BIFF_FN_GetToolbar = 258,
    BIFF_FN_GetTool = 259,
    BIFF_FN_SpellingCheck = 260,
    BIFF_FN_ErrorType = 261,
    BIFF_FN_AppTitle = 262,
    BIFF_FN_WindowTitle = 263,
    BIFF_FN_SaveToolbar = 264,
    BIFF_FN_EnableTool = 265,
    BIFF_FN_PressTool = 266,
    BIFF_FN_RegisterId = 267,
    BIFF_FN_GetWorkbook = 268,
    BIFF_FN_Avedev = 269,
    BIFF_FN_Betadist = 270,
    BIFF_FN_Gammaln = 271,
    BIFF_FN_Betainv = 272,
    BIFF_FN_Binomdist = 273,
    BIFF_FN_Chidist = 274,
    BIFF_FN_Chiinv = 275,
    BIFF_FN_Combin = 276,
    BIFF_FN_Confidence = 277,
    BIFF_FN_Critbinom = 278,
    BIFF_FN_Even = 279,
    BIFF_FN_Expondist = 280,
    BIFF_FN_Fdist = 281,
    BIFF_FN_Finv = 282,
    BIFF_FN_Fisher = 283,
    BIFF_FN_Fisherinv = 284,
    BIFF_FN_Floor = 285,
    BIFF_FN_Gammadist = 286,
    BIFF_FN_Gammainv = 287,
    BIFF_FN_Ceiling = 288,
    BIFF_FN_Hypgeomdist = 289,
    BIFF_FN_Lognormdist = 290,
    BIFF_FN_Loginv = 291,
    BIFF_FN_Negbinomdist = 292,
    BIFF_FN_Normdist = 293,
    BIFF_FN_Normsdist = 294,
    BIFF_FN_Norminv = 295,
    BIFF_FN_Normsinv = 296,
    BIFF_FN_Standardize = 297,
    BIFF_FN_Odd = 298,
    BIFF_FN_Permut = 299,
    BIFF_FN_Poisson = 300,
    BIFF_FN_Tdist = 301,
    BIFF_FN_Weibull = 302,
    BIFF_FN_Sumxmy2 = 303,
    BIFF_FN_Sumx2my2 = 304,
    BIFF_FN_Sumx2py2 = 305,
    BIFF_FN_Chitest = 306,
    BIFF_FN_Correl = 307,
    BIFF_FN_Covar = 308,
    BIFF_FN_Forecast = 309,
    BIFF_FN_Ftest = 310,
    BIFF_FN_Intercept = 311,
    BIFF_FN_Pearson = 312,
    BIFF_FN_Rsq = 313,
    BIFF_FN_Steyx = 314,
    BIFF_FN_Slope = 315,
    BIFF_FN_Ttest = 316,
    BIFF_FN_Prob = 317,
    BIFF_FN_Devsq = 318,
    BIFF_FN_Geomean = 319,
    BIFF_FN_Harmean = 320,
    BIFF_FN_Sumsq = 321,
    BIFF_FN_Kurt = 322,
    BIFF_FN_Skew = 323,
    BIFF_FN_Ztest = 324,
    BIFF_FN_Large = 325,
    BIFF_FN_Small = 326,
    BIFF_FN_Quartile = 327,
    BIFF_FN_Percentile = 328,
    BIFF_FN_Percentrank = 329,
    BIFF_FN_Mode = 330,
    BIFF_FN_Trimmean = 331,
    BIFF_FN_Tinv = 332,
    BIFF_FN_MovieCommand = 334,
    BIFF_FN_GetMovie = 335,
    BIFF_FN_Concatenate = 336,
    BIFF_FN_Power = 337,
    BIFF_FN_PivotAddData = 338,
    BIFF_FN_GetPivotTable = 339,
    BIFF_FN_GetPivotField = 340,
    BIFF_FN_GetPivotItem = 341,
    BIFF_FN_Radians = 342,
    BIFF_FN_Degrees = 343,
    BIFF_FN_Subtotal = 344,
    BIFF_FN_Sumif = 345,
    BIFF_FN_Countif = 346,
    BIFF_FN_Countblank = 347,
    BIFF_FN_ScenarioGet = 348,
    BIFF_FN_OptionsListsGet = 349,
    BIFF_FN_Ispmt = 350,
    BIFF_FN_Datedif = 351,
    BIFF_FN_Datestring = 352,
    BIFF_FN_Numberstring = 353,
    BIFF_FN_Roman = 354,
    BIFF_FN_OpenDialog = 355,
    BIFF_FN_SaveDialog = 356,
    BIFF_FN_ViewGet = 357,
    BIFF_FN_GetPivotData = 358,
    BIFF_FN_Hyperlink = 359,
    BIFF_FN_Phonetic = 360,
    BIFF_FN_AverageA = 361,
    BIFF_FN_MaxA = 362,
    BIFF_FN_MinA = 363,
    BIFF_FN_StDevPA = 364,
    BIFF_FN_VarPA = 365,
    BIFF_FN_StDevA = 366,
    BIFF_FN_VarA = 367
}
BIFF_function_number;

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
