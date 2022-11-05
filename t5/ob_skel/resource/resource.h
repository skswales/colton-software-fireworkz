/* ob_skel/resource/resource.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#ifndef __ob_skel_resource_resource_h
#define __ob_skel_resource_resource_h

/* bitmap ids */
#if RISCOS
#define SKEL_ID_BM_TOOLBAR_NEW                  "doc_new" /* not actually present */
#define SKEL_ID_BM_TOOLBAR_VIEW                 "view_ctrl"
#define SKEL_ID_BM_TOOLBAR_SAVE                 "doc_save"
#define SKEL_ID_BM_TOOLBAR_PRINT                "doc_print"
#define SKEL_ID_BM_TOOLBAR_CUT                  "edit_cut"
#define SKEL_ID_BM_TOOLBAR_COPY                 "edit_copy"
#define SKEL_ID_BM_TOOLBAR_PASTE                "edit_paste"
#define SKEL_ID_BM_TOOLBAR_MARKS                "edit_marks0"
#define SKEL_ID_BM_TOOLBAR_MARKS_ON             "edit_marks1"

#define SKEL_ID_BM_TOOLBAR_STYLE                "styl_s"
#define SKEL_ID_BM_TOOLBAR_EFFECTS              "styl_e"
#define SKEL_ID_BM_TOOLBAR_BOLD                 "styl_b"
#define SKEL_ID_BM_TOOLBAR_ITALIC               "styl_i"
#define SKEL_ID_BM_TOOLBAR_UNDERLINE            "styl_u"
#define SKEL_ID_BM_TOOLBAR_SUPERSCRIPT_THIN     "thin_super"
#define SKEL_ID_BM_TOOLBAR_SUBSCRIPT_THIN       "thin_sub"

#define SKEL_ID_BM_TOOLBAR_J_LEFT               "j_l"
#define SKEL_ID_BM_TOOLBAR_J_CENTRE             "j_c"
#define SKEL_ID_BM_TOOLBAR_J_RIGHT              "j_r"
#define SKEL_ID_BM_TOOLBAR_J_FULL               "j_b"

#define SKEL_ID_BM_TOOLBAR_SEARCH               "edit_find"
#define SKEL_ID_BM_TOOLBAR_TABLE                "table"
#define SKEL_ID_BM_TOOLBAR_BOX                  "box"
#define SKEL_ID_BM_TOOLBAR_MARKER               "marker"
#define SKEL_ID_BM_TOOLBAR_SORT                 "sort"
#define SKEL_ID_BM_TOOLBAR_CHECK                "check"
#define SKEL_ID_BM_TOOLBAR_THESAURUS            "thesaurus"
#define SKEL_ID_BM_TOOLBAR_INSERT_DATE          "insert_date"

#define SKEL_ID_BM_TOOLBAR_TAB_LEFT             "tab+l"
#define SKEL_ID_BM_TOOLBAR_TAB_CENTRE           "tab+c"
#define SKEL_ID_BM_TOOLBAR_TAB_RIGHT            "tab+r"
#define SKEL_ID_BM_TOOLBAR_TAB_DECIMAL          "tab+d"

#define SKEL_ID_BM_BOLD                         SKEL_ID_BM_TOOLBAR_BOLD
#define SKEL_ID_BM_ITALIC                       SKEL_ID_BM_TOOLBAR_ITALIC
#define SKEL_ID_BM_UNDERLINE                    SKEL_ID_BM_TOOLBAR_UNDERLINE
#define SKEL_ID_BM_SUPERSCRIPT                  "styl_super"
#define SKEL_ID_BM_SUBSCRIPT                    "styl_sub"

#define SKEL_ID_BM_J_LEFT                       SKEL_ID_BM_TOOLBAR_J_LEFT
#define SKEL_ID_BM_J_CENTRE                     SKEL_ID_BM_TOOLBAR_J_CENTRE
#define SKEL_ID_BM_J_RIGHT                      SKEL_ID_BM_TOOLBAR_J_RIGHT
#define SKEL_ID_BM_J_FULL                       SKEL_ID_BM_TOOLBAR_J_FULL

#define SKEL_ID_BM_VJ_TOP                       "v_t"
#define SKEL_ID_BM_VJ_CENTRE                    "v_c"
#define SKEL_ID_BM_VJ_BOTTOM                    "v_b"

#define SKEL_ID_BM_PS_SINGLE                    "spac1"
#define SKEL_ID_BM_PS_ONEP5                     "spac1p5"
#define SKEL_ID_BM_PS_DOUBLE                    "spac2"
#define SKEL_ID_BM_PS_N                         "spacn"

#define SKEL_ID_BM_LINE_NONE                    "lnone"
#define SKEL_ID_BM_LINE_THIN                    "lthin"
#define SKEL_ID_BM_LINE_STD                     "lstd"
#define SKEL_ID_BM_LINE_STDD                    "lstdd"
#define SKEL_ID_BM_LINE_THICK                   "lthick"

#define SKEL_ID_BM_DEC                          "down"
#define SKEL_ID_BM_INC                          "up"
#elif WINDOWS
/* multi-bitmaps: 0x0n (+1 = high-dpi variant) */
#define SKEL_ID_BM_TOOLBAR_COM_BTN_ID           T5_RESOURCE_COMMON_BMP_BASE(OBJECT_ID_SKEL, 1)
#define SKEL_ID_BM_TOOLBAR_COM_BTN(n)           T5_RESOURCE_COMMON_BMP(SKEL_ID_BM_TOOLBAR_COM_BTN_ID, n)

#define SKEL_ID_BM_COM_BTN_ID                   T5_RESOURCE_COMMON_BMP_BASE(OBJECT_ID_SKEL, 5)
#define SKEL_ID_BM_COM_BTN(n)                   T5_RESOURCE_COMMON_BMP(SKEL_ID_BM_COM_BTN_ID, n)

#define SKEL_ID_BM_COM07X11_ID                  T5_RESOURCE_COMMON_BMP_BASE(OBJECT_ID_SKEL, 7)
#define SKEL_ID_BM_COM07X11(n)                  T5_RESOURCE_COMMON_BMP(SKEL_ID_BM_COM07X11_ID, n)

#define SKEL_ID_BM_TOOLBAR_VIEW                 SKEL_ID_BM_TOOLBAR_COM_BTN(0)
#define SKEL_ID_BM_TOOLBAR_NEW                  SKEL_ID_BM_TOOLBAR_COM_BTN(28)
#define SKEL_ID_BM_TOOLBAR_OPEN                 SKEL_ID_BM_TOOLBAR_COM_BTN(29)
#define SKEL_ID_BM_TOOLBAR_SAVE                 SKEL_ID_BM_TOOLBAR_COM_BTN(30)
#define SKEL_ID_BM_TOOLBAR_PRINT                SKEL_ID_BM_TOOLBAR_COM_BTN(31)
#define SKEL_ID_BM_TOOLBAR_CUT                  SKEL_ID_BM_TOOLBAR_COM_BTN(32)
#define SKEL_ID_BM_TOOLBAR_COPY                 SKEL_ID_BM_TOOLBAR_COM_BTN(33)
#define SKEL_ID_BM_TOOLBAR_PASTE                SKEL_ID_BM_TOOLBAR_COM_BTN(34)
#define SKEL_ID_BM_TOOLBAR_MARKS                SKEL_ID_BM_TOOLBAR_COM_BTN(23)
#define SKEL_ID_BM_TOOLBAR_MARKS_ON             SKEL_ID_BM_TOOLBAR_COM_BTN(24)

#define SKEL_ID_BM_TOOLBAR_STYLE                SKEL_ID_BM_TOOLBAR_COM_BTN(1)
#define SKEL_ID_BM_TOOLBAR_EFFECTS              SKEL_ID_BM_TOOLBAR_COM_BTN(2)
#define SKEL_ID_BM_TOOLBAR_BOLD                 SKEL_ID_BM_TOOLBAR_COM_BTN(3)
#define SKEL_ID_BM_TOOLBAR_ITALIC               SKEL_ID_BM_TOOLBAR_COM_BTN(4)
#define SKEL_ID_BM_TOOLBAR_UNDERLINE            SKEL_ID_BM_TOOLBAR_COM_BTN(5)
#define SKEL_ID_BM_TOOLBAR_SUPERSCRIPT_THIN     SKEL_ID_BM_TOOLBAR_COM_BTN(6)//SKEL_ID_BM_COM07X11(4)
#define SKEL_ID_BM_TOOLBAR_SUBSCRIPT_THIN       SKEL_ID_BM_TOOLBAR_COM_BTN(7)//SKEL_ID_BM_COM07X11(5)

#define SKEL_ID_BM_TOOLBAR_J_LEFT               SKEL_ID_BM_TOOLBAR_COM_BTN(8)
#define SKEL_ID_BM_TOOLBAR_J_CENTRE             SKEL_ID_BM_TOOLBAR_COM_BTN(9)
#define SKEL_ID_BM_TOOLBAR_J_RIGHT              SKEL_ID_BM_TOOLBAR_COM_BTN(10)
#define SKEL_ID_BM_TOOLBAR_J_FULL               SKEL_ID_BM_TOOLBAR_COM_BTN(11)

#define SKEL_ID_BM_TOOLBAR_TAB_LEFT             SKEL_ID_BM_COM07X11(0)
#define SKEL_ID_BM_TOOLBAR_TAB_CENTRE           SKEL_ID_BM_COM07X11(1)
#define SKEL_ID_BM_TOOLBAR_TAB_RIGHT            SKEL_ID_BM_COM07X11(2)
#define SKEL_ID_BM_TOOLBAR_TAB_DECIMAL          SKEL_ID_BM_COM07X11(3)

#define SKEL_ID_BM_TOOLBAR_SORT                 SKEL_ID_BM_TOOLBAR_COM_BTN(25)
#define SKEL_ID_BM_TOOLBAR_CHECK                SKEL_ID_BM_TOOLBAR_COM_BTN(27)
#define SKEL_ID_BM_TOOLBAR_THESAURUS            SKEL_ID_BM_TOOLBAR_COM_BTN(0)
#define SKEL_ID_BM_TOOLBAR_INSERT_DATE          SKEL_ID_BM_TOOLBAR_COM_BTN(26)

#define SKEL_ID_BM_TOOLBAR_SEARCH               SKEL_ID_BM_TOOLBAR_COM_BTN(12)
#define SKEL_ID_BM_TOOLBAR_TABLE                SKEL_ID_BM_TOOLBAR_COM_BTN(13)
#define SKEL_ID_BM_TOOLBAR_BOX                  SKEL_ID_BM_TOOLBAR_COM_BTN(14)
#define SKEL_ID_BM_TOOLBAR_MARKER               SKEL_ID_BM_TOOLBAR_COM_BTN(15)

#define SKEL_ID_BM_BOLD                         SKEL_ID_BM_COM_BTN(3)
#define SKEL_ID_BM_ITALIC                       SKEL_ID_BM_COM_BTN(4)
#define SKEL_ID_BM_UNDERLINE                    SKEL_ID_BM_COM_BTN(5)
#define SKEL_ID_BM_SUPERSCRIPT                  SKEL_ID_BM_COM_BTN(6)
#define SKEL_ID_BM_SUBSCRIPT                    SKEL_ID_BM_COM_BTN(7)

#define SKEL_ID_BM_J_LEFT                       SKEL_ID_BM_COM_BTN(8)
#define SKEL_ID_BM_J_CENTRE                     SKEL_ID_BM_COM_BTN(9)
#define SKEL_ID_BM_J_RIGHT                      SKEL_ID_BM_COM_BTN(10)
#define SKEL_ID_BM_J_FULL                       SKEL_ID_BM_COM_BTN(11)

#define SKEL_ID_BM_VJ_TOP                       SKEL_ID_BM_COM_BTN(16)
#define SKEL_ID_BM_VJ_CENTRE                    SKEL_ID_BM_COM_BTN(17)
#define SKEL_ID_BM_VJ_BOTTOM                    SKEL_ID_BM_COM_BTN(18)

#define SKEL_ID_BM_PS_SINGLE                    SKEL_ID_BM_COM_BTN(19)
#define SKEL_ID_BM_PS_ONEP5                     SKEL_ID_BM_COM_BTN(20)
#define SKEL_ID_BM_PS_DOUBLE                    SKEL_ID_BM_COM_BTN(21)
#define SKEL_ID_BM_PS_N                         SKEL_ID_BM_COM_BTN(22)

#define SKEL_ID_BM_LINE_NONE                    1225
#define SKEL_ID_BM_LINE_THIN                    1226
#define SKEL_ID_BM_LINE_STD                     1227
#define SKEL_ID_BM_LINE_STDD                    1228
#define SKEL_ID_BM_LINE_THICK                   1229

#define SKEL_ID_BM_RULER_TABL                   1250
#define SKEL_ID_BM_RULER_TABR                   1251
#define SKEL_ID_BM_RULER_TABC                   1252
#define SKEL_ID_BM_RULER_TABD                   1253
#define SKEL_ID_BM_RULER_MARL                   1254
#define SKEL_ID_BM_RULER_MARP                   1255
#define SKEL_ID_BM_RULER_MARR                   1256
#define SKEL_ID_BM_RULER_MARH                   1257
#define SKEL_ID_BM_RULER_MARF                   1258
#define SKEL_ID_BM_RULER_ROWB                   1259
#define SKEL_ID_BM_RULER_COLR                   1260
#define SKEL_ID_BM_RULER_COLS                   1261
#define SKEL_ID_BM_RULER_ROWS                   1262

#define SKEL_ID_BM_INC                          1216
#define SKEL_ID_BM_DEC                          1217
#endif /* OS */

/*
keep consistent with &.t5.ob_skel.resource.msg
*/

/* following on from STATUS_xxx we have...*/

#define ERR_OUTPUT_STRING                   -47 /* output the following string in error rather than lookup */
#define ERR_ERR_NOT_FOUND                   -48
#define ERR_MSG_NOT_FOUND                   -49
#define ERR_OUTPUT_SPRINTF                  -50 /* output the following sprintf format and args in error rather than lookup */
#define ERR_NYI                             -51
#define ERR_TOOMANYWINDOWS                  -52
#define ERR_NO_CONFIG                       -53
#define ERR_LOADING_CONFIG                  -54
#define ERR_FONT_SET                        -55
#define ERR_FONT_COLOUR                     -56
#define ERR_FONT_PAINT                      -57
#define ERR_FONT_FIND                       -58
#define ERR_PRINT_PAPER_DETAILS             -59
#define ERR_NOTWHILSTRECORDING              -60
#define ERR_ALREADYRECORDING                -61
#define ERR_KEY_DEF_IN_USE                  -62
#define ERR_DUPLICATE_FILE                  -63
#define ERR_NO_SPRITES                      -64
#define ERR_DUPLICATE_LEAFNAME              -65
#define ERR_UNKNOWN_FILETYPE                -66
#define ERR_EOF_BEFORE_FINISHED             -67
#define ERR_TYPE5_FONT_NOT_FOUND            -68
#define ERR_HOST_FONT_NOT_FOUND             -69
#define ERR_TEMPLATE_NOT_FOUND              -70
#define ERR_NO_HOST_FONTS                   -71
#define ERR_NAMED_STYLE_NOT_FOUND           -72
#define ERR_STYLE_IN_USE                    -73
#define ERR_PRINT_WONT_OPEN                 -74
#define ERR_PRINT_FAILED                    -75
#define ERR_PRINT_BUSY                      -76
#define ERR_PRINT_NO_DRIVER                 -77
#define ERR_BAD_FIND_STRING                 -78
#define ERR_NO_THESAURUS                    -79
#define ERR_NO_DATA_LOADED                  -80
#define ERR_NO_STUBS                        -81
#define ERR_NOTE_NOT_LOADED                 -82 /* for display */
#define ERR_NO_ALTERNATE_COMMAND            -83
#define ERR_WILL_SAVE_AS_FIREWORKZ          -84
#define ERR_NOTFOUND_REFERENCED_PICTURE     -85
#define ERR_AREA_PROTECTED                  -86
#define ERR_SUPPORTER_NOT_FOUND             -87
#define ERR_STYLE_WITH_SAME_NAME            -88
#define ERR_STYLE_MUST_HAVE_NAME            -89
#define ERR_MANDATORY_ARG_MISSING           -90
#define ERR_CANT_EDIT_SCALED_TO_FIT_NOTE    -91
#define ERR_HELP_FAILURE                    -92
#define ERR_HELP_URL_FAILURE                -93
#define ERR_DRAWFILE_FAILURE                -94
#define ERR_RAW_ARG_FAILURE                 -95
#define ERR_CFBF_SAVE_NEEDS_TEMPLATE        -96
#define ERR_SAVE_DRAG_TO_DIRECTORY          -97
#define ERR_RISCOS_NO_SCRAP                 -98
#define ERR_DATE_TIME_INVALID               -99

#define STATUS_MODULE_NOT_FOUND             -100
#define ERR_ARG_FAILURE                     -101
#define ERR_TOO_MANY_COLUMNS_FOR_EXPORT     -102
#define ERR_TOO_MANY_ROWS_FOR_EXPORT        -103
/*spares*/
#define ERR_READ_ONLY                       -120
#define ERR_TEMPLATE_DEST_EXISTS            -121
#define ERR_CANT_IMPORT_ISAFILE             -122
#define ERR_CANT_DELETE_MAIN_COL            -123
#define ERR_FILE_TOO_SMALL                  -124
#define ERR_FILE_TOO_LARGE                  -125
#define ERR_INVALID_UTF8_ENCODING           -126
#define ERR_UCS4_NONCHARACTER               -127
#define ERR_CLIP_DATA_TOO_LARGE             -128

#if RISCOS
#define STATUS_NOT_A_MODULE                 -202
#define STATUS_BAD_CHUNK                    -203
#define STATUS_BAD_VERSION                  -204
#define STATUS_NOT_ALL_LOADED               -205
#define STATUS_EXTERN_BIND_FAIL             -206
#define STATUS_NO_LONGER_AVAIL              -207
#define STATUS_BAD_INSTRUCT                 -208
#endif

#if WINDOWS
#define ERR_PRINT_UNKNOWN                   -211
#define ERR_PRINT_TERMINATED                -212
#define ERR_PRINT_TERMINATED_VIA_PM         -213
#define ERR_PRINT_DISK_FULL                 -214
#define ERR_PRINT_MEMORY_FULL               -215

#define ERR_CFBF_SAVE_NEEDS_FILENAME        -216
#endif

/*
string resource allocation
*/

#define MSG_SKEL_VERSION                        1
#define MSG_SKEL_DATE                           2
#define MSG_COPYRIGHT                           3

#define MSG_BUTTON_ADD                          90
#define MSG_BUTTON_CREATE                       91
#define MSG_BUTTON_REMOVE                       92
#define MSG_BUTTON_INSERT                       93
#define MSG_BUTTON_HELP                         94
#define MSG_BUTTON_APPLY                        95
#define MSG_BUTTON_NEW                          96
#define MSG_BUTTON_CHANGE                       97
#define MSG_BUTTON_DELETE                       98
#define MSG_BUTTON_REPLACE                      99

#define MSG_FILENAME_UNTITLED                   100
#define MSG_TITLEBAR_NORMAL                     101
#define MSG_TITLEBAR_READ_ONLY                  1032
#define MSG_TITLEBAR_MODIFIED                   102
#define MSG_AT_N_PERCENT                        103
#define MSG_PERCENT                             104
#define MSG_EMBED                               105
#define MSG_STATUS_SORTING                      106

#define MSG_SELECTION                           107
#define MSG_BUTTON_CLOSE                        108
#define MSG_BUTTON_CANCEL                       109
#define MSG_BUTTON_OK                           110
#define MSG_STATUS_CONVERTING                   111
#define MSG_STATUS_LOADING                      112
#define MSG_STATUS_INSERTING                    113
#define MSG_STATUS_SAVING                       114
#define MSG_STATUS_REFORMATTING                 115
#define MSG_STATUS_PASTING                      116
#define MSG_STATUS_CUTTING                      117
#define MSG_BUTTON_SAVE                         118
#define MSG_QUERY_SAVE                          MSG_BUTTON_SAVE
#define MSG_QUERY_DISCARD                       119

/*
long names for function keys
*/

#define MSG_FUNC_PRINT                          120
#define MSG_FUNC_F1                             121
#define MSG_FUNC_F2                             122
#define MSG_FUNC_F3                             123
#define MSG_FUNC_F4                             124
#define MSG_FUNC_F5                             125
#define MSG_FUNC_F6                             126
#define MSG_FUNC_F7                             127
#define MSG_FUNC_F8                             128
#define MSG_FUNC_F9                             129
#define MSG_FUNC_F10                            130
#define MSG_FUNC_F11                            131
#define MSG_FUNC_F12                            132
#define MSG_FUNC_INSERT                         133
#define MSG_FUNC_TAB                            134
#define MSG_FUNC_END                            135
#define MSG_FUNC_LEFT                           136
#define MSG_FUNC_RIGHT                          137
#define MSG_FUNC_DOWN                           138
#define MSG_FUNC_UP                             139
#define MSG_FUNC_DELETE                         140
#define MSG_FUNC_HOME                           141
#define MSG_FUNC_BACKSPACE                      142
#define MSG_FUNC_RETURN                         143

#define MSG_FUNC_LONG_SHIFT                     150
#define MSG_FUNC_LONG_CTRL                      151
#define MSG_FUNC_LONG_CTRL_SHIFT                152
#define MSG_FUNC_MENU_SHIFT                     153
#define MSG_FUNC_MENU_CTRL                      154
#define MSG_FUNC_MENU_CTRL_SHIFT                155

#define MSG_FUNC_PAGE_DOWN                      160
#define MSG_FUNC_PAGE_UP                        161

#define MSG_DIALOG_INSERT_FIELD_TIME_CAPTION        168
#define MSG_DIALOG_INSERT_FIELD_spare_169           169
#define MSG_DIALOG_INSERT_FIELD_DATE_CAPTION        170
#define MSG_DIALOG_INSERT_FIELD_PAGE_CAPTION        171
#define MSG_DIALOG_INSERT_FIELD_LIVE                172
#define MSG_DIALOG_INSERT_FIELD_spare_173           173
#define MSG_STATUS_INSERT_FIELD_DATE                174

#define MSG_DIALOG_INSERT_FIELD_PAGE_Y_LABEL        1038
#define MSG_DIALOG_INSERT_FIELD_PAGE_X_LABEL        1039
#define MSG_DIALOG_INSERT_FIELD_DATE_LABEL          1044
#define MSG_DIALOG_INSERT_FIELD_FILE_DATE_LABEL     1045
#define MSG_DIALOG_INSERT_FIELD_TIME_LABEL          1046
#define MSG_DIALOG_INSERT_FIELD_FILE_TIME_LABEL     1047

#define MSG_DIALOG_ES_STYLE_NAME                180
#define MSG_DIALOG_ES_STYLE_KEY                 181

#define MSG_DIALOG_VIEW_SCALE_GROUP             250
#define MSG_DIALOG_VIEW_SCALE_1                 251
#define MSG_DIALOG_VIEW_SCALE_2                 252
#define MSG_DIALOG_VIEW_SCALE_3                 253
#define MSG_DIALOG_VIEW_SCALE_4                 254
#define MSG_DIALOG_VIEW_SCALE_5                 255

#define MSG_DIALOG_VIEW_CONTROL_CAPTION         256
#define MSG_DIALOG_VIEW_CONTROL_HELP_TOPIC      1239
#define MSG_DIALOG_VIEW_SCALE_CAPTION           257
#define MSG_DIALOG_VIEW_SCALE_HELP_TOPIC        1240
#define MSG_DIALOG_VIEW_BORDER_HORZ             258
#define MSG_DIALOG_VIEW_BORDER_VERT             259

#define MSG_DIALOG_SAVE_SELECTION_CAPTION       1054
#define MSG_DIALOG_LOCATE_TEMPLATE_CAPTION      1055
#define MSG_DIALOG_LOCATE_TEMPLATE_WAFFLE_1     1056
#define MSG_DIALOG_LOCATE_TEMPLATE_WAFFLE_2     1057
#define MSG_DIALOG_LOCATE_TEMPLATE_WAFFLE_3     1058
#define MSG_DIALOG_LOCATE_TEMPLATE_WAFFLE_4     1059
#define MSG_DIALOG_LOCATE_TEMPLATE_WAFFLE_5     1060

#define MSG_DIALOG_SAVE_AS_FOREIGN_CAPTION      260
#define MSG_DIALOG_SAVE_AS_FOREIGN_NAME         261
#define MSG_DIALOG_SAVE_AS_DRAWFILE_CAPTION     MSG_DIALOG_SAVE_AS_FOREIGN_CAPTION
#define MSG_DIALOG_SAVE_AS_DRAWFILE_NAME        1061

#define MSG_DIALOG_VIEW_RULER_HORZ              262
#define MSG_DIALOG_VIEW_RULER_VERT              263
#define MSG_DIALOG_VIEW_NEW_VIEW                264
#define MSG_DIALOG_VIEW_DISPLAY                 265
#define MSG_DIALOG_VIEW_DISPLAY_1               266
#define MSG_DIALOG_VIEW_DISPLAY_2               267
#define MSG_DIALOG_VIEW_DISPLAY_3               268
#define MSG_DIALOG_VIEW_SCALE_FIT_H             269
#define MSG_DIALOG_VIEW_SCALE_FIT_V             270
#define MSG_DIALOG_VIEW_SPLIT_H                 272
#define MSG_DIALOG_VIEW_SPLIT_V                 273

#define MSG_STATUS_BORDER_HORZ_COL_AND_WIDTH    283
#define MSG_STATUS_BORDER_VERT_ROW_AND_HEIGHT   284

#define MSG_DIALOG_STYLE                        289

#define MSG_DIALOG_SELECT_TEMPLATE_CAPTION      290
#define MSG_DIALOG_SELECT_TEMPLATE_HELP_TOPIC   291
#define MSG_DIALOG_NEW_DOCUMENT_CAPTION         292

#define MSG_DIALOG_NEW_STYLE                    293
#define MSG_DIALOG_NEW_STYLE_SUGGESTED          294
#define MSG_DIALOG_NEW_STYLE_NAME               295
#define MSG_DIALOG_NEW_STYLE_BASED_ON           296
#define MSG_DIALOG_NEW_STYLE_DEFINING           297
#define MSG_DIALOG_EDITING_STYLE                298
#define MSG_DIALOG_APPLY_EFFECTS                299

#define MSG_DIALOG_PRINT                        306
#define MSG_DIALOG_PRINT_BASIC_HELP_TOPIC       300
#define MSG_DIALOG_PRINT_COPIES                 301
#define MSG_DIALOG_PRINT_PAGE_RANGE_OUTER       1048
#define MSG_DIALOG_PRINT_ALL_PAGES              302
#define MSG_DIALOG_PRINT_PAGES                  303
#define MSG_DIALOG_PRINT_PAGE_RANGE_Y0          1035
#define MSG_DIALOG_PRINT_PAGE_RANGE_Y1          1036
#define MSG_DIALOG_PRINT_TWO_UP                 304
#define MSG_DIALOG_PRINT_EXTRA_HELP_TOPIC       305
#define MSG_DIALOG_PRINT_NO_RISCOS              307
#define MSG_DIALOG_NO_PRINTER                   307 /* Windows uses this one! */
#define MSG_DIALOG_PRINT_REVERSE                321
#define MSG_DIALOG_PRINT_SORTED                 322
#define MSG_DIALOG_PRINT_ACROSS                 323
#define MSG_DIALOG_PRINT_PAMPHLET               324
#define MSG_DIALOG_PRINT_EXTRA                  343
#define MSG_DIALOG_PRINT_SIDES                  344
#define MSG_DIALOG_PRINT_BOTH                   345
#define MSG_DIALOG_PRINT_ODD                    346
#define MSG_DIALOG_PRINT_EVEN                   347
#define MSG_DIALOG_PRINT_DRAFT                  348
#define MSG_DIALOG_PRINT_MAILS                  1000

#define MSG_DIALOG_PAPER_SCALE_CAPTION          1020
#define MSG_DIALOG_PAPER_SCALE_FIT              1021
#define MSG_DIALOG_PAPER_SCALE_FIT_X            1022
#define MSG_DIALOG_PAPER_SCALE                  1023
#define MSG_DIALOG_PAPER_SCALE_100              1024
#define MSG_DIALOG_PAPER_SCALE_HELP_TOPIC       1025

#define MSG_DIALOG_ES_NAME                      310
#define MSG_DIALOG_ES_FS                        311
#define MSG_DIALOG_ES_PS1                       312
#define MSG_DIALOG_ES_PS2                       313
#define MSG_DIALOG_ES_PS3                       314
#define MSG_DIALOG_ES_PS4                       315
#define MSG_DIALOG_ES_RS                        316
#define MSG_DIALOG_ES_CS                        317

#define MSG_DIALOG_ES_PSG                       309

#define MSG_DIALOG_ES_PS2_CELL                  351

#define MSG_DIALOG_ES_PS_HORZ_JUSTIFY           308
#define MSG_DIALOG_ES_PS_VERT_JUSTIFY           350

#define MSG_DIALOG_ES_PS_NEW_OBJECT             329

#define MSG_DIALOG_ES_PS_NUMFORM_GROUP          349
#define MSG_DIALOG_ES_PS_NUMFORM_NU             318
#define MSG_DIALOG_ES_PS_NUMFORM_DT             319
#define MSG_DIALOG_ES_PS_NUMFORM_SE             320

#define MSG_DIALOG_ES_PS_PROTECTION             352

#define MSG_DIALOG_ES_FS_TYPEFACE               325
#define MSG_DIALOG_ES_FS_HEIGHT                 326
#define MSG_DIALOG_ES_FS_WIDTH                  327

#define MSG_MEASUREMENT_POINTS                  328

#define MSG_DIALOG_ES_PS_MARGIN_PARA            330
#define MSG_DIALOG_ES_PS_MARGIN_LEFT            331
#define MSG_DIALOG_ES_PS_MARGIN_RIGHT           332
#define MSG_DIALOG_ES_PS_TAB_LIST               333

#define MSG_DIALOG_ES_CS_WIDTH                  334
#define MSG_DIALOG_ES_CS_COLUMN_NAME            335
#define MSG_DIALOG_UNITS_CM                     336
#define MSG_DIALOG_UNITS_MM                     337
#define MSG_DIALOG_UNITS_INCHES                 338
#define MSG_DIALOG_UNITS_POINTS                 478
#define MSG_DIALOG_ES_CS_TAB_L                  339
#define MSG_DIALOG_ES_CS_TAB_C                  340
#define MSG_DIALOG_ES_CS_TAB_R                  341
#define MSG_DIALOG_ES_CS_TAB_D                  342

#define MSG_DIALOG_ES_RS_HEIGHT                 450
#define MSG_DIALOG_ES_RS_HEIGHT_FIXED           451
#define MSG_DIALOG_ES_RS_UNBREAKABLE            452
#define MSG_DIALOG_ES_RS_ROW_NAME               453

#define MSG_DIALOG_PAPER_CAPTION                455
#define MSG_DIALOG_PAPER_HELP_TOPIC             456
#define MSG_DIALOG_PAPER_PAPER                  457
#define MSG_DIALOG_PAPER_PAPER_MARGIN           458
#define MSG_DIALOG_PAPER_BINDING                459
#define MSG_DIALOG_PAPER_NAME                   460
#define MSG_DIALOG_PAPER_HEIGHT                 461
#define MSG_DIALOG_PAPER_WIDTH                  462
#define MSG_DIALOG_PAPER_MARGIN_TOP             463
#define MSG_DIALOG_PAPER_MARGIN_BOTTOM          464
#define MSG_DIALOG_PAPER_MARGIN_LEFT            465
#define MSG_DIALOG_PAPER_MARGIN_RIGHT           466
#define MSG_DIALOG_PAPER_MARGIN_BIND            467
#define MSG_DIALOG_PAPER_MARGIN_OE_SWAP         468
#define MSG_DIALOG_PAPER_BUTTON_READ            469
#define MSG_DIALOG_PAPER_BUTTON_NONE            470
#define MSG_DIALOG_PAPER_BUTTON_FROM_PRINTER    471
#define MSG_DIALOG_PAPER_PORTRAIT               472
#define MSG_DIALOG_PAPER_LANDSCAPE              473
#define MSG_DIALOG_PAPER_MARGIN_COL             474
#define MSG_DIALOG_PAPER_MARGIN_ROW             475
#define MSG_DIALOG_PAPER_GRID_SIZE              476
#define MSG_DIALOG_PAPER_GRID_FAINT             477
#define MSG_DIALOG_PAPER_GRID                   480

#define MSG_spares_481_489                      481 /* ... 489 */

#define MSG_DIALOG_RGB_COLOUR                   490
#define MSG_DIALOG_ES_PS_RGB_BACK               491
#define MSG_DIALOG_RGB_TX_R                     492
#define MSG_DIALOG_RGB_TX_G                     493
#define MSG_DIALOG_RGB_TX_B                     494
#define MSG_DIALOG_RGB_TX_T                     495
#define MSG_DIALOG_RGB_BUTTON                   1028

#define MSG_DIALOG_ES_PS_PARA_START             500
#define MSG_DIALOG_ES_PS_PARA_END               501
#define MSG_DIALOG_ES_PS_LINE_SPACE             502
#define MSG_DIALOG_ES_PS_PARA                   503
#define MSG_DIALOG_ES_PS_LINE                   504

#define MSG_DIALOG_ES_PS_BORDERS                510
#define MSG_DIALOG_ES_PS_GRID                   511
#define MSG_DIALOG_ES_PS_TX_LEFT                512
#define MSG_DIALOG_ES_PS_TX_TOP                 513
#define MSG_DIALOG_ES_PS_TX_RIGHT               514
#define MSG_DIALOG_ES_PS_TX_BOTTOM              515

#define MSG_DIALOG_ES_PS_MARGINS                516
#define MSG_DIALOG_ES_PS_TX_LINE                517

#define MSG_DIALOG_PBRK_PAGE_NUMBER             524

#define MSG_DIALOG_SETC_CAPTION                 550
#define MSG_DIALOG_SETC_HELP_TOPIC              551
#define MSG_DIALOG_SETC_GROUP                   552
#define MSG_DIALOG_SETC_UPPER                   553
#define MSG_DIALOG_SETC_LOWER                   554
#define MSG_DIALOG_SETC_INICAP                  554

#define MSG_PRINT_WARNING_TITLE                 600
#define MSG_PRINT_WARNING_MSG1                  601
#define MSG_PRINT_WARNING_MSG2                  602
#define MSG_BUTTON_PRINT                        603

#define MSG_STATUS_PAGE_ONE_NUM                 700
#define MSG_STATUS_PAGE_XY_NUM                  701
#define MSG_STATUS_VIEW_CONTROL                 702
#define MSG_STATUS_JUSTIFY_LEFT                 703
#define MSG_STATUS_JUSTIFY_CENTRE               704
#define MSG_STATUS_JUSTIFY_RIGHT                705
#define MSG_STATUS_JUSTIFY_FULL                 706
#define MSG_STATUS_APPLY_STYLE                  707
#define MSG_STATUS_APPLY_EFFECTS                708
#define MSG_STATUS_APPLY_BOLD                   709
#define MSG_STATUS_APPLY_ITALIC                 710
#define MSG_STATUS_TAB_LEFT                     711
#define MSG_STATUS_TAB_CENTRE                   712
#define MSG_STATUS_TAB_RIGHT                    713
#define MSG_STATUS_TAB_DECIMAL                  714
#define MSG_STATUS_TOGGLE_MARKS                 715
#define MSG_STATUS_COPY                         716
#define MSG_STATUS_CUT                          717
#define MSG_STATUS_PASTE                        718
#define MSG_STATUS_STATUS_LINE                  719
#define MSG_STATUS_SAVE                         720
#define MSG_STATUS_PRINT                        721
#define MSG_STATUS_SELECTION_TOO_BIG            722

#define MSG_STATUS_WORDS_COUNTED                726
#define MSG_STATUS_NOT_FOUND                    730
#define MSG_STATUS_N_REPLACED                   731
#define MSG_STATUS_DRAG_COL                     732
#define MSG_STATUS_DRAG_ROW                     733
#define MSG_STATUS_NOTE_ORDINATE_spare_         735 /* .. 737 */
#define MSG_STATUS_NOTE_COORDINATES_CM          738
#define MSG_STATUS_NOTE_COORDINATES_MM          739
#define MSG_STATUS_NOTE_COORDINATES_INCHES      740
#define MSG_STATUS_NOTE_COORDINATES_POINTS      479
#define MSG_STATUS_AUTO_WIDTH_NOT_DONE          741
#define MSG_STATUS_WORD_COUNTED                 742
#define MSG_STATUS_REGIONS_COUNTED              743
#define MSG_STATUS_REGION_COUNTED               1026

#define MSG_NUMFORM_1_DP                        744
#define MSG_NUMFORM_2_DP                        745
#define MSG_NUMFORM_3_DP                        746
#define MSG_NUMFORM_1_DP0                       747
#define MSG_NUMFORM_2_DP0                       748
#define MSG_NUMFORM_3_DP0                       749

#define MSG_DIALOG_BACKDROP_CAPTION             750
#define MSG_DIALOG_BACKDROP_PAGE_FIRST          751
#define MSG_DIALOG_BACKDROP_PAGE_ALL            752
#define MSG_DIALOG_BACKDROP_PRINT               753
#define MSG_DIALOG_BACKDROP_FIT                 754
#define MSG_DIALOG_BACKDROP_ORIGIN              755
#define MSG_DIALOG_BACKDROP_ORIGIN_AS_IS        756
#define MSG_DIALOG_BACKDROP_ORIGIN_WORK_AREA    757
#define MSG_DIALOG_BACKDROP_ORIGIN_PRINT_AREA   758
#define MSG_DIALOG_BACKDROP_ORIGIN_PAPER        759
#define MSG_DIALOG_BACKDROP_EMBED               760
#define MSG_DIALOG_BACKDROP_REMOVE              761
#define MSG_DIALOG_BACKDROP_HELP_TOPIC          762

#define MSG_DIALOG_SAVE_QUERY_LINKED            790
#define MSG_DIALOG_SAVE_ALL                     791
#define MSG_DIALOG_DISCARD_ALL                  792
#define MSG_DIALOG_QUIT_QUERY_SECOND            799
#define MSG_DIALOG_QUIT_QUERY_ONE               800
#define MSG_DIALOG_QUIT_QUERY_MANY              801
#define MSG_DIALOG_QUIT_QUERY_SURE              802
#define MSG_DIALOG_CLOSE_QUERY_1                803
#define MSG_DIALOG_CLOSE_QUERY_2                804
#define MSG_DIALOG_CLOSE_QUERY_LINKED           805
#define MSG_DIALOG_CLOSE_ALL                    806
#define MSG_DIALOG_CLOSE_DOCUMENT               807

#if RISCOS
#define MSG_DIALOG_INFO_NAME_LABEL              810
#else
#define MSG_DIALOG_INFO_NAME_LABEL              0
#endif
#define MSG_DIALOG_INFO_AUTHOR_LABEL            0
#define MSG_DIALOG_INFO_VERSION_LABEL           812
#define MSG_DIALOG_INFO_USER_LABEL              813
#define MSG_DIALOG_INFO_REGNO_LABEL             814
#define MSG_DIALOG_INFO_CAPTION                 816
#define MSG_DIALOG_INFO_WEB_BUTTON              817
#define MSG_DIALOG_INFO_WEB_URL                 818

#define MSG_DIALOG_INSERT_TABLE_TABLE           823
#define MSG_DIALOG_INSERT_TABLE_BASETABLE       824

#define MSG_DIALOG_SAVE_CAPTION                 857
#define MSG_DIALOG_SAVE_HELP_TOPIC              858

#define MSG_DIALOG_SAVE_TEMPLATE_CAPTION        859
#define MSG_DIALOG_SAVE_TEMPLATE_HELP_TOPIC     860
#define MSG_DIALOG_SAVE_TEMPLATE_NAME           861
#define MSG_DIALOG_SAVE_TEMPLATE_ALL            862
#define MSG_DIALOG_SAVE_TEMPLATE_ALL_STYLES     863
#define MSG_DIALOG_SAVE_TEMPLATE_ONE_STYLE      864

#define MSG_DIALOG_SAVE_PICTURE_CAPTION         865
#define MSG_DIALOG_SAVE_PICTURE_HELP_TOPIC      866
#define MSG_DIALOG_SAVE_PICTURE_NAME            867

#define MSG_DIALOG_SEARCH_CAPTION               870
#define MSG_DIALOG_SEARCH_FIND                  871
#define MSG_DIALOG_SEARCH_REPLACE               872
#define MSG_DIALOG_SEARCH_IGNORE_CAPITALS       873
#define MSG_DIALOG_SEARCH_COPY_CAPITALS         874
#define MSG_DIALOG_SEARCH_FROM_CARET            875
#define MSG_DIALOG_SEARCH_FROM_TOP              876
#define MSG_DIALOG_SEARCH_WHOLE_WORDS           877
#define MSG_DIALOG_SEARCH_FROM_TEXT_CURSOR      878
#define MSG_DIALOG_SEARCH_HELP_TOPIC            879

#define MSG_DIALOG_SEARCH_QUERY_CAPTION         880
#define MSG_DIALOG_SEARCH_QUERY_HELP_TOPIC      881
#define MSG_DIALOG_SEARCH_QUERY_NEXT            882
#define MSG_DIALOG_SEARCH_QUERY_REPLACE_ALL     883

#define MSG_FIELD                               890

#define MSG_DIALOG_GOTO_CAPTION                 891
#define MSG_DIALOG_GOTO_LABEL                   892
#define MSG_BUTTON_GOTO                         893
#define MSG_DIALOG_GOTO_HELP_TOPIC              894

#define MSG_DIALOG_STYLE_NUMFORM_POINTS         MSG_NUMFORM_2_DP
#define MSG_DIALOG_STYLE_NUMFORM_MM             MSG_NUMFORM_1_DP
#define MSG_DIALOG_STYLE_NUMFORM_CM             MSG_NUMFORM_2_DP
#define MSG_DIALOG_STYLE_NUMFORM_INCHES         MSG_NUMFORM_3_DP
#define MSG_DIALOG_STYLE_NUMFORM_MM_FINE        MSG_NUMFORM_2_DP
#define MSG_DIALOG_STYLE_NUMFORM_CM_FINE        MSG_NUMFORM_3_DP

#define MSG_spares_895_896                      895

#define MSG_DIALOG_CHOICES_CAPTION                          909
#define MSG_DIALOG_CHOICES_HELP_TOPIC                       910
#define MSG_DIALOG_CHOICES_MAIN_AUTO_SAVE                   911
#define MSG_DIALOG_CHOICES_MAIN_AUTO_SAVE_UNITS             912
#define MSG_DIALOG_CHOICES_MAIN_DISPLAY_PICTURES            913
#define MSG_DIALOG_CHOICES_MAIN_EMBED_INSERTED_FILES        914
#define MSG_DIALOG_CHOICES_MAIN_STATUS_LINE                 915
#define MSG_DIALOG_CHOICES_MAIN_TOOLBAR                     920
#define MSG_DIALOG_CHOICES_MAIN_UPDATE_STYLES_FROM_CHOICES  922
#define MSG_DIALOG_CHOICES_MAIN_RULERS_RULER                916
#define MSG_DIALOG_CHOICES_MAIN_KERNING                     917
#define MSG_DIALOG_CHOICES_MAIN_DITHERING                   918
#define MSG_DIALOG_CHOICES_MAIN_ASCII_LOAD_AS_DELIMITED     921
#define MSG_DIALOG_CHOICES_MAIN_ASCII_LOAD_AS_PARAGRAPHS    919

#define MSG_IN                                  930
#define MSG_OUT                                 931
#define MSG_STYLE_TEXT_CONVERT_NO_MORE          933
#define MSG_REGION                              934

#define MSG_STYLE_TEXT_ON                       938
#define MSG_STYLE_TEXT_OFF                      939

#define MSG_STYLE_TEXT_LEFT                     941
#define MSG_STYLE_TEXT_CENTRE                   942
#define MSG_STYLE_TEXT_RIGHT                    943
#define MSG_STYLE_TEXT_BOTH                     944
#define MSG_STYLE_TEXT_PERCENT_S                945

#define MSG_STYLE_TEXT_CONVERT_REGION           947
#define MSG_STYLE_TEXT_CONVERT_START            948
#define MSG_STYLE_TEXT_CONVERT_END              990

#define MSG_STYLE_TEXT_TOP                      995
#define MSG_STYLE_TEXT_BOTTOM                   996

#define MSG_CUSTOM_LIBRARY                      997

#define MSG_DIALOG_SORT_CAPTION                 998
#define MSG_DIALOG_SORT_HELP_TOPIC              1043
#define MSG_DIALOG_SORT_ORDER                   999
#define MSG_DIALOG_ES_PS_NUMFORM_DT_DROPDOWN    1001
#define MSG_DIALOG_ES_PS_NUMFORM_SE_DROPDOWN    1002

#define MSG_HELP_ICONBAR                        1011
#define MSG_STATUS_APPLY_UNDERLINE              1012
#define MSG_STATUS_BOX                          1013
#define MSG_STATUS_INSERT_TABLE                 1014
#define MSG_STATUS_SEARCH                       1015
#define MSG_STATUS_NEW_DOCUMENT                 1016
#define MSG_STATUS_OPEN_DOCUMENT                1017
#define MSG_STATUS_APPLY_SUPERSCRIPT            1018
#define MSG_STATUS_APPLY_SUBSCRIPT              1019

#define MSG_spares_1027_1029                    1027

#define MSG_spares_1038_1039                    1038

#define MSG_STATUS_SORT                         1030
#define MSG_STATUS_CHECK                        1051
#define MSG_STATUS_THESAURUS                    1052

#define MSG_PORTIONS_DIAL_SOLUTIONS             1031
#define MSG_USES_COMPONENTS                     1037

#define MSG_OBJECT_TYPE_TEXT                    1040
#define MSG_OBJECT_TYPE_REC                     1041
#define MSG_OBJECT_TYPE_SS                      1042

#define MSG_PORTIONS_R_COMP                     ((STATUS_MSG_INCREMENT * OBJECT_ID_RECB) + 0)

#endif /* __ob_skel_resource_resource_h */

/* end of ob_skel/resource/resource.h */
