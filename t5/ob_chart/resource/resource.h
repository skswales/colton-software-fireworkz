/* resource.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* icon ids */
#if WINDOWS
#endif

/* bitmap ids */
#if RISCOS
#define CHART_ID_BM_B0 "b0"
#define CHART_ID_BM_B1 "b1"
#define CHART_ID_BM_B2 "b2"
#define CHART_ID_BM_B3 "b3"
#define CHART_ID_BM_B4 "b4"
#define CHART_ID_BM_B5 "b5"
#define CHART_ID_BM_B6 "b6"
#define CHART_ID_BM_B7 "b7"

#define CHART_ID_BM_L0 "l0"
#define CHART_ID_BM_L1 "l1"
#define CHART_ID_BM_L2 "l2"
#define CHART_ID_BM_L3 "l3"
#define CHART_ID_BM_L4 "l4"
#define CHART_ID_BM_L5 "l5"
#define CHART_ID_BM_L6 "l6"
#define CHART_ID_BM_L7 "l7"

#define CHART_ID_BM_S0 "s0"
#define CHART_ID_BM_S1 "s1"
#define CHART_ID_BM_S2 "s2"
#define CHART_ID_BM_S3 "s3"
#define CHART_ID_BM_S4 "s4"
#define CHART_ID_BM_S5 "s5"
#define CHART_ID_BM_S6 "s6"
#define CHART_ID_BM_S7 "s7"
#define CHART_ID_BM_S8 "s8"

#define CHART_ID_BM_O0 "o0"
#define CHART_ID_BM_O1 "o1"
#define CHART_ID_BM_O2 "o2"
#define CHART_ID_BM_O3 "o3"

#define CHART_ID_BM_PL0 "pl0"
#define CHART_ID_BM_PL1 "pl1"
#define CHART_ID_BM_PL2 "pl2"
#define CHART_ID_BM_PL3 "pl3"
#define CHART_ID_BM_PX1 "px1"
#define CHART_ID_BM_PX2 "px2"

#define CHART_ID_BM_TK_IV "tk_iv"
#define CHART_ID_BM_TK_FV "tk_fv"
#define CHART_ID_BM_TK_OV "tk_ov"
#define CHART_ID_BM_TK_NV "tk_nv"
#define CHART_ID_BM_TK_IH "tk_ih"
#define CHART_ID_BM_TK_OH "tk_oh"
#define CHART_ID_BM_TK_NH "tk_nh"
#define CHART_ID_BM_TK_FH "tk_fh"

#define CHART_ID_BM_LDASH    "ldash"
#define CHART_ID_BM_LDOT     "ldot"
#define CHART_ID_BM_LDASHDOT "ldashdot"
#define CHART_ID_BM_LDADODO  "ldashdotdot"
#elif WINDOWS
/* multi-bitmaps: 0x6n (+1 = high-dpi variant) */
#define CHART_ID_BM_COMBAR_ID T5_RESOURCE_COMMON_BMP_BASE(OBJECT_ID_CHART, 0) /* bar.bmp */
#define CHART_ID_BM_COMBAR(n) T5_RESOURCE_COMMON_BMP(CHART_ID_BM_COMBAR_ID, n)

#define CHART_ID_BM_COMLIN_ID T5_RESOURCE_COMMON_BMP_BASE(OBJECT_ID_CHART, 2) /* line.bmp */
#define CHART_ID_BM_COMLIN(n) T5_RESOURCE_COMMON_BMP(CHART_ID_BM_COMLIN_ID, n)

#define CHART_ID_BM_COMOVR_ID T5_RESOURCE_COMMON_BMP_BASE(OBJECT_ID_CHART, 4) /* overlay.bmp */
#define CHART_ID_BM_COMOVR(n) T5_RESOURCE_COMMON_BMP(CHART_ID_BM_COMOVR_ID, n)

#define CHART_ID_BM_COMPIE_ID T5_RESOURCE_COMMON_BMP_BASE(OBJECT_ID_CHART, 6) /* pie.bmp */
#define CHART_ID_BM_COMPIE(n) T5_RESOURCE_COMMON_BMP(CHART_ID_BM_COMPIE_ID, n)

#define CHART_ID_BM_COMSCT_ID T5_RESOURCE_COMMON_BMP_BASE(OBJECT_ID_CHART, 8) /* scatter.bmp */
#define CHART_ID_BM_COMSCT(n) T5_RESOURCE_COMMON_BMP(CHART_ID_BM_COMSCT_ID, n)

#define CHART_ID_BM_COM_BTN_ID T5_RESOURCE_COMMON_BMP_BASE(OBJECT_ID_CHART, 12)
#define CHART_ID_BM_COM_BTN(n) T5_RESOURCE_COMMON_BMP(CHART_ID_BM_COM_BTN_ID, n)

#define CHART_ID_BM_BASE        0x2000 /* 8192 */

#define CHART_ID_BM_B0 CHART_ID_BM_COMBAR(0)
#define CHART_ID_BM_B1 CHART_ID_BM_COMBAR(1)
#define CHART_ID_BM_B2 CHART_ID_BM_COMBAR(2)
#define CHART_ID_BM_B3 CHART_ID_BM_COMBAR(3)
#define CHART_ID_BM_B4 CHART_ID_BM_COMBAR(4)
#define CHART_ID_BM_B5 CHART_ID_BM_COMBAR(5)
#define CHART_ID_BM_B6 CHART_ID_BM_COMBAR(6)
#define CHART_ID_BM_B7 CHART_ID_BM_COMBAR(7)

#define CHART_ID_BM_L0 CHART_ID_BM_COMLIN(0)
#define CHART_ID_BM_L1 CHART_ID_BM_COMLIN(1)
#define CHART_ID_BM_L2 CHART_ID_BM_COMLIN(2)
#define CHART_ID_BM_L3 CHART_ID_BM_COMLIN(3)
#define CHART_ID_BM_L4 CHART_ID_BM_COMLIN(4)
#define CHART_ID_BM_L5 CHART_ID_BM_COMLIN(5)
#define CHART_ID_BM_L6 CHART_ID_BM_COMLIN(6)
#define CHART_ID_BM_L7 CHART_ID_BM_COMLIN(7)

#define CHART_ID_BM_S0 CHART_ID_BM_COMSCT(0)
#define CHART_ID_BM_S1 CHART_ID_BM_COMSCT(1)
#define CHART_ID_BM_S2 CHART_ID_BM_COMSCT(2)
#define CHART_ID_BM_S3 CHART_ID_BM_COMSCT(3)
#define CHART_ID_BM_S4 CHART_ID_BM_COMSCT(4)
#define CHART_ID_BM_S5 CHART_ID_BM_COMSCT(5)
#define CHART_ID_BM_S6 CHART_ID_BM_COMSCT(6)
#define CHART_ID_BM_S7 CHART_ID_BM_COMSCT(7)
#define CHART_ID_BM_S8 CHART_ID_BM_COMSCT(8)

#define CHART_ID_BM_O0 CHART_ID_BM_COMOVR(0)
#define CHART_ID_BM_O1 CHART_ID_BM_COMOVR(1)
#define CHART_ID_BM_O2 CHART_ID_BM_COMOVR(2)
#define CHART_ID_BM_O3 CHART_ID_BM_COMOVR(3)

#define CHART_ID_BM_PL0 CHART_ID_BM_COMPIE(0)
#define CHART_ID_BM_PL1 CHART_ID_BM_COMPIE(1)
#define CHART_ID_BM_PL2 CHART_ID_BM_COMPIE(2)
#define CHART_ID_BM_PL3 CHART_ID_BM_COMPIE(3)
#define CHART_ID_BM_PX1 CHART_ID_BM_COMPIE(4)
#define CHART_ID_BM_PX2 CHART_ID_BM_COMPIE(5)

#define CHART_ID_BM_TK_IV CHART_ID_BM_COM_BTN(6)
#define CHART_ID_BM_TK_FV CHART_ID_BM_COM_BTN(5)
#define CHART_ID_BM_TK_OV CHART_ID_BM_COM_BTN(7)
#define CHART_ID_BM_TK_NV CHART_ID_BM_COM_BTN(4)
#define CHART_ID_BM_TK_IH CHART_ID_BM_COM_BTN(2)
#define CHART_ID_BM_TK_OH CHART_ID_BM_COM_BTN(3)
#define CHART_ID_BM_TK_NH CHART_ID_BM_COM_BTN(0)
#define CHART_ID_BM_TK_FH CHART_ID_BM_COM_BTN(1)

#define CHART_ID_BM_LDASH       (CHART_ID_BM_BASE + 243)
#define CHART_ID_BM_LDOT        (CHART_ID_BM_BASE + 244)
#define CHART_ID_BM_LDASHDOT    (CHART_ID_BM_BASE + 245)
#define CHART_ID_BM_LDADODO     (CHART_ID_BM_BASE + 246)
#endif /* OS */

/*
error numbers
*/

#define CHART_ERR_BASE  (STATUS_ERR_INCREMENT * OBJECT_ID_CHART)

#define CHART_ERR(n)    (CHART_ERR_BASE - (n))

#define CHART_ERR_NOT_ENOUGH_INPUT_PIE                              CHART_ERR(0)
#define CHART_ERR_NEGATIVE_OR_ZERO_IGNORED_PIE                      CHART_ERR(2)
#define CHART_ERR_EXCEPTION                                         CHART_ERR(3)
#define CHART_ERR_NEGATIVE_OR_ZERO_IGNORED_STACK                    CHART_ERR(4)
#define CHART_ERR_LOAD_NO_MEM                                       CHART_ERR(5)
#define CHART_ERR_NO_DATA                                           CHART_ERR(6)
#define CHART_ERR_FILETYPE_BAD                                      CHART_ERR(7)

/*
messages
*/

#define CHART_MSG_BASE  (STATUS_MSG_INCREMENT * OBJECT_ID_CHART)

#define CHART_MSG(n)    (CHART_MSG_BASE + (n))

#define CHART_MSG_DIALOG_PIE_GALLERY_CAPTION                        CHART_MSG(0)
#define CHART_MSG_DIALOG_PIE_GALLERY_HELP_TOPIC                     CHART_MSG(1)
#define CHART_MSG_DIALOG_PIE_GALLERY_LABEL                          CHART_MSG(2)
#define CHART_MSG_DIALOG_PIE_GALLERY_EXPLODE                        CHART_MSG(3)
#define CHART_MSG_DIALOG_PIE_GALLERY_EXPLODE_BY                     CHART_MSG(4)
#define CHART_MSG_DIALOG_PIE_GALLERY_START_POSITION                 CHART_MSG(5)
#define CHART_MSG_DIALOG_ANGLE                                      CHART_MSG(6)
#define CHART_MSG_DIALOG_PIE_GALLERY_ANTICLOCKWISE                  CHART_MSG(7)

#define CHART_MSG_SUGGESTED_LEAFNAME                                CHART_MSG(10)

#define CHART_MSG_DIALOG_BAR_GALLERY_CAPTION                        CHART_MSG(100)
#define CHART_MSG_DIALOG_BAR_GALLERY_HELP_TOPIC                     CHART_MSG(101)
#define CHART_MSG_DIALOG_BAR_GALLERY_HORIZONTAL                     CHART_MSG(102)

#define CHART_MSG_DIALOG_BL_GALLERY_3D                              CHART_MSG(103)

#define CHART_MSG_DIALOG_OVER_BL_GALLERY_CAPTION                    CHART_MSG(105)
#define CHART_MSG_DIALOG_OVER_BL_GALLERY_HELP_TOPIC                 CHART_MSG(106)

#define CHART_MSG_DIALOG_BL_GALLERY_SERIES_TEXT                     CHART_MSG(110)
#define CHART_MSG_DIALOG_BL_GALLERY_SERIES_COLS                     CHART_MSG(111)
#define CHART_MSG_DIALOG_BL_GALLERY_SERIES_ROWS                     CHART_MSG(112)
#define CHART_MSG_DIALOG_BL_GALLERY_FIRST_SERIES_TEXT               CHART_MSG(113)
#define CHART_MSG_DIALOG_BL_GALLERY_FIRST_SERIES_CATEGORY_X_LABELS  CHART_MSG(114)
#define CHART_MSG_DIALOG_BL_GALLERY_FIRST_SERIES_SERIES_DATA        CHART_MSG(115)
#define CHART_MSG_DIALOG_BL_GALLERY_FIRST_CATEGORY_TEXT             CHART_MSG(116)
#define CHART_MSG_DIALOG_BL_GALLERY_FIRST_CATEGORY_SERIES_LABELS    CHART_MSG(117)
#define CHART_MSG_DIALOG_BL_GALLERY_FIRST_CATEGORY_CATEGORY_DATA    CHART_MSG(118)
#define CHART_MSG_DIALOG_BL_GALLERY_FIRST_SERIES_CATEGORY_Y_LABELS  CHART_MSG(119)
#define CHART_MSG_DIALOG_BL_GALLERY_FIRST_SERIES_TEXT1              CHART_MSG(120)
#define CHART_MSG_DIALOG_BL_GALLERY_FIRST_SERIES_XYY_DATA           CHART_MSG(121)
#define CHART_MSG_DIALOG_BL_GALLERY_FIRST_SERIES_XYXY_DATA          CHART_MSG(122)
#define CHART_MSG_DIALOG_BL_GALLERY_FIRST_POINT_TEXT                CHART_MSG(123)
#define CHART_MSG_DIALOG_BL_GALLERY_FIRST_POINT_POINT_DATA          CHART_MSG(124)
#define CHART_MSG_DIALOG_PIE_GALLERY_FIRST_SERIES_CATEGORY_LABELS   CHART_MSG(125)
#define CHART_MSG_DIALOG_PIE_GALLERY_FIRST_CATEGORY_SERIES_LABEL    CHART_MSG(126)

#define CHART_MSG_STATUS_LINE_NO_SELECTION                          CHART_MSG(130)

#define CHART_MSG_DIALOG_BAR_PROCESS                                CHART_MSG(150)
#define CHART_MSG_DIALOG_BAR_PROCESS_1                              CHART_MSG(151)
#define CHART_MSG_DIALOG_BAR_PROCESS_2                              CHART_MSG(152)
#define CHART_MSG_DIALOG_BAR_PROCESS_3                              CHART_MSG(153)
#define CHART_MSG_DIALOG_BAR_PROCESS_HELP_TOPIC                     CHART_MSG(160)

#define CHART_MSG_DIALOG_BL_PROCESS_3D                              CHART_MSG(190)
#define CHART_MSG_DIALOG_BL_PROCESS_3D_DROOP                        CHART_MSG(191)
#define CHART_MSG_DIALOG_BL_PROCESS_3D_TURN                         CHART_MSG(192)

#define CHART_MSG_DIALOG_LINE_GALLERY_CAPTION                       CHART_MSG(200)
#define CHART_MSG_DIALOG_LINE_GALLERY_HELP_TOPIC                    CHART_MSG(201)
#define CHART_MSG_DIALOG_LINE_GALLERY_3D                            CHART_MSG(202)

#define CHART_MSG_DIALOG_LINE_PROCESS                               CHART_MSG(250)
#define CHART_MSG_DIALOG_LINE_PROCESS_1                             CHART_MSG(251)
#define CHART_MSG_DIALOG_LINE_PROCESS_2                             CHART_MSG(252)
#define CHART_MSG_DIALOG_LINE_PROCESS_3                             CHART_MSG(253)
#define CHART_MSG_DIALOG_LINE_PROCESS_HELP_TOPIC                    CHART_MSG(260)

#define CHART_MSG_DIALOG_SCAT_GALLERY_CAPTION                       CHART_MSG(300)
#define CHART_MSG_DIALOG_SCAT_GALLERY_HELP_TOPIC                    CHART_MSG(301)

#define CHART_MSG_DIALOG_SCAT_PROCESS                               CHART_MSG(350)
#define CHART_MSG_DIALOG_SCAT_PROCESS_1                             CHART_MSG(351)
#define CHART_MSG_DIALOG_SCAT_PROCESS_HELP_TOPIC                    CHART_MSG(360)

#define CHART_MSG_DIALOG_GEN_AXIS_POSITION                          CHART_MSG(400)
#define CHART_MSG_DIALOG_GEN_AXIS_POSITION_LZR_L                    CHART_MSG(401)
#define CHART_MSG_DIALOG_GEN_AXIS_POSITION_LZR_T                    CHART_MSG(402)
#define CHART_MSG_DIALOG_GEN_AXIS_POSITION_LZR_R                    CHART_MSG(403)
#define CHART_MSG_DIALOG_GEN_AXIS_POSITION_LZR_B                    CHART_MSG(404)
#define CHART_MSG_DIALOG_GEN_AXIS_POSITION_LZR_ZERO                 CHART_MSG(405)
#define CHART_MSG_DIALOG_GEN_AXIS_POSITION_ARF_FRONT                CHART_MSG(406)
#define CHART_MSG_DIALOG_GEN_AXIS_POSITION_ARF_AUTO                 CHART_MSG(407)
#define CHART_MSG_DIALOG_GEN_AXIS_POSITION_ARF_REAR                 CHART_MSG(408)

#define CHART_MSG_DIALOG_GEN_AXIS_MAJOR                             CHART_MSG(409)
#define CHART_MSG_DIALOG_GEN_AXIS_MAJOR_AUTO                        CHART_MSG(410)
#define CHART_MSG_DIALOG_GEN_AXIS_MAJOR_SPACING                     CHART_MSG(411)
#define CHART_MSG_DIALOG_GEN_AXIS_MAJOR_GRID                        CHART_MSG(412)
#define CHART_MSG_DIALOG_GEN_AXIS_MAJOR_TICKS                       CHART_MSG(413)
#define CHART_MSG_DIALOG_GEN_AXIS_MINOR                             CHART_MSG(414)

#define CHART_MSG_DIALOG_VAL_AXIS_SCALING                           CHART_MSG(420)
#define CHART_MSG_DIALOG_VAL_AXIS_SCALING_AUTO                      CHART_MSG(421)
#define CHART_MSG_DIALOG_VAL_AXIS_SCALING_MINIMUM                   CHART_MSG(422)
#define CHART_MSG_DIALOG_VAL_AXIS_SCALING_MAXIMUM                   CHART_MSG(423)
#define CHART_MSG_DIALOG_VAL_AXIS_SCALING_INCLUDE_ZERO              CHART_MSG(424)
#define CHART_MSG_DIALOG_VAL_AXIS_SCALING_LOGARITHMIC               CHART_MSG(425)
#define CHART_MSG_DIALOG_VAL_AXIS_SCALING_LOG_LABELS                CHART_MSG(426)

#define CHART_MSG_DIALOG_VAL_AXIS_SERIES                            CHART_MSG(427)
#define CHART_MSG_DIALOG_VAL_AXIS_SERIES_CUMULATIVE                 CHART_MSG(428)
#define CHART_MSG_DIALOG_VAL_AXIS_SERIES_VARY_BY_POINT              CHART_MSG(429)
#define CHART_MSG_DIALOG_VAL_AXIS_SERIES_BEST_FIT                   CHART_MSG(430)
#define CHART_MSG_DIALOG_VAL_AXIS_SERIES_FILL_TO_AXIS               CHART_MSG(431)
#define CHART_MSG_DIALOG_VAL_AXIS_SERIES_STACK                      CHART_MSG(432)

#define CHART_MSG_DIALOG_SERIES_IN_OVERLAY                          CHART_MSG(433)

#define CHART_MSG_DIALOG_STYLE_LINE_COLOUR                          CHART_MSG(500)
#define CHART_MSG_DIALOG_STYLE_LINE_STYLE                           CHART_MSG(501)

#define CHART_MSG_DIALOG_STYLE_FILL                                 CHART_MSG(503)
#define CHART_MSG_DIALOG_STYLE_FILL_COLOUR                          CHART_MSG(504)
#define CHART_MSG_DIALOG_STYLE_FILL_SOLID                           CHART_MSG(505)
#define CHART_MSG_DIALOG_STYLE_FILL_PICTURE                         CHART_MSG(506)
#define CHART_MSG_DIALOG_STYLE_FILL_ASPECT                          CHART_MSG(507)
#define CHART_MSG_DIALOG_STYLE_FILL_RECOLOUR                        CHART_MSG(508)

#define CHART_MSG_DIALOG_STYLE_FILL_HELP_TOPIC                      CHART_MSG(510)
#define CHART_MSG_DIALOG_STYLE_LINE_HELP_TOPIC                      CHART_MSG(511)

#define CHART_MSG_DIALOG_CHART_MARGINS_CAPTION                      CHART_MSG(520)
#define CHART_MSG_DIALOG_CHART_MARGINS_HELP_TOPIC                   CHART_MSG(521)
#define CHART_MSG_DIALOG_CHART_MARGIN_TOP                           CHART_MSG(522)
#define CHART_MSG_DIALOG_CHART_MARGIN_BOTTOM                        CHART_MSG(523)
#define CHART_MSG_DIALOG_CHART_MARGIN_LEFT                          CHART_MSG(524)
#define CHART_MSG_DIALOG_CHART_MARGIN_RIGHT                         CHART_MSG(525)

#define CHART_MSG_DIALOG_CHART_LEGEND_CAPTION                       CHART_MSG(530)
#define CHART_MSG_DIALOG_CHART_LEGEND_HELP_TOPIC                    CHART_MSG(531)
#define CHART_MSG_DIALOG_CHART_LEGEND_ON                            CHART_MSG(532)
#define CHART_MSG_DIALOG_CHART_LEGEND_HORZ                          CHART_MSG(533)

#define CHART_MSG_DEFAULT_FONT                                      CHART_MSG(600)
#define CHART_MSG_DEFAULT_CHARTZD                                   CHART_MSG(602)

#define CHART_MSG_OBJNAME_CHART                                     CHART_MSG(700)
#define CHART_MSG_OBJNAME_PLOTAREA_1                                CHART_MSG(701)
#define CHART_MSG_OBJNAME_PLOTAREA_2                                CHART_MSG(702)
#define CHART_MSG_OBJNAME_PLOTAREA_3                                CHART_MSG(703)
#define CHART_MSG_OBJNAME_LEGEND                                    CHART_MSG(704)
#define CHART_MSG_OBJNAME_TEXT                                      CHART_MSG(705)
#define CHART_MSG_OBJNAME_SERIES                                    CHART_MSG(706)
#define CHART_MSG_OBJNAME_POINT                                     CHART_MSG(707)
#define CHART_MSG_OBJNAME_DROPSERIES                                CHART_MSG(708)
#define CHART_MSG_OBJNAME_DROPPOINT                                 CHART_MSG(709)
#define CHART_MSG_OBJNAME_AXIS                                      CHART_MSG(710)
#define CHART_MSG_OBJNAME_AXIS_X                                    CHART_MSG(711)
#define CHART_MSG_OBJNAME_AXIS_Y                                    CHART_MSG(712)
#define CHART_MSG_OBJNAME_AXIS_X_CAT                                CHART_MSG(713)
#define CHART_MSG_OBJNAME_AXIS_Y_VAL                                CHART_MSG(714)
#define CHART_MSG_OBJNAME_AXIS_Z_SER                                CHART_MSG(715)
#define CHART_MSG_OBJNAME_AXISGRID                                  CHART_MSG(716)
#define CHART_MSG_OBJNAME_AXISTICK                                  CHART_MSG(717)
#define CHART_MSG_OBJNAME_BESTFITSER                                CHART_MSG(718)

#define CHART_MSG_DIALOG_SERIES_EDIT_HELP_TOPIC                     CHART_MSG(800)
#define CHART_MSG_DIALOG_AXIS_CAT_EDIT_HELP_TOPIC                   CHART_MSG(801)
#define CHART_MSG_DIALOG_AXIS_VAL_EDIT_HELP_TOPIC                   CHART_MSG(802)

/* end of ob_chart/resource/resource.h */
