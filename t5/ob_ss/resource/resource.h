/* ob_ss/resource/resource.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* icon ids */

/* bitmap ids */
#if RISCOS
#define SS_ID_BM_TOOLBAR_FILL_DN        "rep_down"
#define SS_ID_BM_TOOLBAR_FILL_RT        "rep_right"
#define SS_ID_BM_TOOLBAR_AUTO_SUM       "auto_sum"
#define SS_ID_BM_TOOLBAR_CHART          "ss_chart"

#define SS_ID_BM_TOOLBAR_FUNCTION       "form_func"
#define SS_ID_BM_TOOLBAR_FORM_CAN       "form_cancel"
#define SS_ID_BM_TOOLBAR_FORM_ENT       "form_enter"

#define SS_ID_BM_TOOLBAR_PLUS           "plus"
#define SS_ID_BM_TOOLBAR_MINUS          "minus"
#define SS_ID_BM_TOOLBAR_TIMES          "times"
#define SS_ID_BM_TOOLBAR_DIVIDE         "divide"

#define SS_ID_BM_TOOLBAR_MAKE_TEXT      "make_text"
#define SS_ID_BM_TOOLBAR_MAKE_NUMBER    "make_number"
#define SS_ID_BM_TOOLBAR_MAKE_CONSTANT  "make_const"
#elif WINDOWS
/* multi-bitmaps: 0x1n (+1 = high-dpi variant) */
#define SS_ID_BM_TOOLBAR_COM_BTN_ID T5_RESOURCE_COMMON_BMP_BASE(OBJECT_ID_SS, 0)
#define SS_ID_BM_TOOLBAR_COM_BTN(n) T5_RESOURCE_COMMON_BMP(SS_ID_BM_TOOLBAR_COM_BTN_ID, n)

#define SS_ID_BM_TOOLBAR_FILL_DN        SS_ID_BM_TOOLBAR_COM_BTN(0)
#define SS_ID_BM_TOOLBAR_FILL_RT        SS_ID_BM_TOOLBAR_COM_BTN(1)
#define SS_ID_BM_TOOLBAR_AUTO_SUM       SS_ID_BM_TOOLBAR_COM_BTN(2)
#define SS_ID_BM_TOOLBAR_CHART          SS_ID_BM_TOOLBAR_COM_BTN(3)

#define SS_ID_BM_TOOLBAR_FUNCTION       SS_ID_BM_TOOLBAR_COM_BTN(4)
#define SS_ID_BM_TOOLBAR_FORM_CAN       SS_ID_BM_TOOLBAR_COM_BTN(5)
#define SS_ID_BM_TOOLBAR_FORM_ENT       SS_ID_BM_TOOLBAR_COM_BTN(6)

#define SS_ID_BM_TOOLBAR_PLUS           SS_ID_BM_TOOLBAR_COM_BTN(7)
#define SS_ID_BM_TOOLBAR_MINUS          SS_ID_BM_TOOLBAR_COM_BTN(8)
#define SS_ID_BM_TOOLBAR_TIMES          SS_ID_BM_TOOLBAR_COM_BTN(9)
#define SS_ID_BM_TOOLBAR_DIVIDE         SS_ID_BM_TOOLBAR_COM_BTN(10)

#define SS_ID_BM_TOOLBAR_MAKE_TEXT      SS_ID_BM_TOOLBAR_COM_BTN(11)
#define SS_ID_BM_TOOLBAR_MAKE_NUMBER    SS_ID_BM_TOOLBAR_COM_BTN(12)
#define SS_ID_BM_TOOLBAR_MAKE_CONSTANT  SS_ID_BM_TOOLBAR_COM_BTN(13)

#define SS_ID_BM_BASE           0x1C00
#endif /* OS */

#define SS_MSG_BASE (STATUS_MSG_INCREMENT * OBJECT_ID_SS)

#define SS_MSG(n)   (SS_MSG_BASE + (n))

/* NB these need to be in the same order as EV_RESO_ENGINEER etc. */
#define SS_MSG_DIALOG_FUNCTIONS_ENGINEER        SS_MSG(1)
#define SS_MSG_DIALOG_FUNCTIONS_DATABASE        SS_MSG(2)
#define SS_MSG_DIALOG_FUNCTIONS_DATE            SS_MSG(3)
#define SS_MSG_DIALOG_FUNCTIONS_FINANCE         SS_MSG(4)
#define SS_MSG_DIALOG_FUNCTIONS_MATHS           SS_MSG(5)
#define SS_MSG_DIALOG_FUNCTIONS_MATRIX          SS_MSG(6)
#define SS_MSG_DIALOG_FUNCTIONS_MISC            SS_MSG(7)
#define SS_MSG_DIALOG_FUNCTIONS_STATS           SS_MSG(8)
#define SS_MSG_DIALOG_FUNCTIONS_STRING          SS_MSG(9)
#define SS_MSG_DIALOG_FUNCTIONS_TRIG            SS_MSG(10)
#define SS_MSG_DIALOG_FUNCTIONS_CONTROL         SS_MSG(11)
#define SS_MSG_DIALOG_FUNCTIONS_LOOKUP          SS_MSG(12)
#define SS_MSG_DIALOG_FUNCTIONS_CUSTOM          SS_MSG(13)
#define SS_MSG_DIALOG_FUNCTIONS_LOGICAL         SS_MSG(14)
#define SS_MSG_DIALOG_FUNCTIONS_COMPAT          SS_MSG(15)

#define SS_MSG_DIALOG_FUNCTIONS_NAMES           SS_MSG(16)
#define SS_MSG_DIALOG_FUNCTIONS_ALL             SS_MSG(17)
#define SS_MSG_DIALOG_FUNCTIONS_SHORT           SS_MSG(18)

/* NB these also need to be in the same order as EV_RESO_ENGINEER etc. */
#define SS_MSG_DLG_HT_FUNCTIONS_ENGINEER        SS_MSG(65)
#define SS_MSG_DLG_HT_FUNCTIONS_DATABASE        SS_MSG(66)
#define SS_MSG_DLG_HT_FUNCTIONS_DATE            SS_MSG(67)
#define SS_MSG_DLG_HT_FUNCTIONS_FINANCE         SS_MSG(68)
#define SS_MSG_DLG_HT_FUNCTIONS_MATHS           SS_MSG(69)
#define SS_MSG_DLG_HT_FUNCTIONS_MATRIX          SS_MSG(70)
#define SS_MSG_DLG_HT_FUNCTIONS_MISC            SS_MSG(71)
#define SS_MSG_DLG_HT_FUNCTIONS_STATS           SS_MSG(72)
#define SS_MSG_DLG_HT_FUNCTIONS_STRING          SS_MSG(73)
#define SS_MSG_DLG_HT_FUNCTIONS_TRIG            SS_MSG(74)
#define SS_MSG_DLG_HT_FUNCTIONS_CONTROL         SS_MSG(75)
#define SS_MSG_DLG_HT_FUNCTIONS_LOOKUP          SS_MSG(76)
#define SS_MSG_DLG_HT_FUNCTIONS_CUSTOM          SS_MSG(77)
#define SS_MSG_DLG_HT_FUNCTIONS_LOGICAL         SS_MSG(78)
#define SS_MSG_DLG_HT_FUNCTIONS_COMPAT          SS_MSG(79)

#define SS_MSG_DLG_HT_FUNCTIONS_NAMES           SS_MSG(80)
#define SS_MSG_DLG_HT_FUNCTIONS_ALL             SS_MSG(81)
#define SS_MSG_DLG_HT_FUNCTIONS_SHORT           SS_MSG(82)

#define SS_MSG_STATUS_ARG_DEFAULT_STRING        SS_MSG(32)
#define SS_MSG_STATUS_ARG_DEFAULT_HELP          SS_MSG(33)
#define SS_MSG_STATUS_STATUS_LINE_BASE          SS_MSG(34)

#define SS_MSG_DIALOG_NAME_INTRO_CAPTION        SS_MSG(38)
#define SS_MSG_DIALOG_NAME_INTRO_HELP_TOPIC     SS_MSG(39)
#define SS_MSG_DIALOG_NAME_INTRO_INSERT_HELP_TOPIC SS_MSG(53)
#define SS_MSG_DIALOG_NAME_NAME                 SS_MSG(40)
#define SS_MSG_DIALOG_NAME_VALUE                SS_MSG(41)
#define SS_MSG_DIALOG_NAME_DESC                 SS_MSG(42)
#define SS_MSG_DIALOG_NAME_ADD                  SS_MSG(43)
#define SS_MSG_DIALOG_NAME_EDIT                 SS_MSG(44)
#define SS_MSG_DIALOG_NAME_INSERT               SS_MSG(45)
#define SS_MSG_DIALOG_NAME_DELETE               SS_MSG(52)

#define SS_MSG_DIALOG_NAME_EDIT_CAPTION         SS_MSG(50)
#define SS_MSG_DIALOG_NAME_ADD_CAPTION          SS_MSG(51)

#define SS_MSG_DIALOG_ALERT_CAPTION             SS_MSG(55)
#define SS_MSG_DIALOG_ALERT_HELP_TOPIC          SS_MSG(56)

#define SS_MSG_DIALOG_INPUT_CAPTION             SS_MSG(57)
#define SS_MSG_DIALOG_INPUT_HELP_TOPIC          SS_MSG(58)

#define SS_MSG_STATUS_RECALC_N                  SS_MSG(28)
#define SS_MSG_STATUS_ERR                       SS_MSG(29)
#define SS_MSG_STATUS_CUSTOM_ERR                SS_MSG(30)
#define SS_MSG_STATUS_PROPAGATED_ERR            SS_MSG(31)

#define SS_MSG_STATUS_FILL_DOWN                 SS_MSG(90)
#define SS_MSG_STATUS_FILL_RIGHT                SS_MSG(91)
#define SS_MSG_STATUS_AUTO_SUM                  SS_MSG(92)
#define SS_MSG_STATUS_CHART                     SS_MSG(93)
#define SS_MSG_STATUS_FORMULA_FUNCTION          SS_MSG(94)
#define SS_MSG_STATUS_FORMULA_CANCEL            SS_MSG(95)
#define SS_MSG_STATUS_FORMULA_ENTER             SS_MSG(96)
#define SS_MSG_STATUS_FORMULA_LINE              SS_MSG(97)


#define SS_MSG_STATUS_FUNCTIONS                 SS_MSG(101)

/* get the argument message id for a given RPN number. Assumes abs() first function */
#define SS_MSG_STATUS_GET_NUMBER_FROM_RPN(x)    (SS_MSG_STATUS_FUNCTIONS - RPN_FNF_ABS + (x))


#define SS_MSG_FUNCTIONS                        SS_MSG(501)

/* get the function name id for a given RPN number. Assumes abs() first function */
#define SS_MSG_FUNCTION_FROM_RPN(x)             (SS_MSG_FUNCTIONS - RPN_FNF_ABS + (x))


#define SS_MSG_DIALOG_CHOICES_GROUP                     SS_MSG(935)
#define SS_MSG_DIALOG_CHOICES_CALC_BACKGROUND           SS_MSG(936)
#define SS_MSG_DIALOG_CHOICES_CALC_AUTO                 SS_MSG(937)
#define SS_MSG_DIALOG_CHOICES_CALC_ON_LOAD              SS_MSG(938)
#define SS_MSG_DIALOG_CHOICES_CALC_ADDITIONAL_ROUNDING  SS_MSG(950)
#define SS_MSG_DIALOG_CHOICES_EDIT_IN_CELL              SS_MSG(948)
#define SS_MSG_DIALOG_CHOICES_ALTERNATE_FORMULA_STYLE   SS_MSG(949)

#define SS_MSG_STATUS_PLUS                      SS_MSG(951)
#define SS_MSG_STATUS_MINUS                     SS_MSG(952)
#define SS_MSG_STATUS_TIMES                     SS_MSG(953)
#define SS_MSG_STATUS_DIVIDE                    SS_MSG(954)
#define SS_MSG_STATUS_MAKE_TEXT                 SS_MSG(955)
#define SS_MSG_STATUS_MAKE_NUMBER               SS_MSG(956)
#define SS_MSG_STATUS_MAKE_CONSTANT             SS_MSG(957)
#define SS_MSG_STATUS_INSERT_DATE               SS_MSG(958)
#define SS_MSG_STATUS_INSERT_NAME               SS_MSG(959)

#define SS_MSG_DIALOG_CHOICES_CHART_GROUP       SS_MSG(960)
#define SS_MSG_DIALOG_CHOICES_CHART_UPDATE_AUTO SS_MSG(961)


#define SS_ERR_BASE ((STATUS_ERR_INCREMENT * OBJECT_ID_SS) - 100)

#define SS_ERR(n)   (SS_ERR_BASE - (n))

#define SS_ERR_AREA_NOT_BLANK                   SS_ERR(0)
#define STATUS_NOT_AVAILABLE                    SS_ERR(1)
#define SS_ERR_NAME_BLANK_NAME                  SS_ERR(2)
#define SS_ERR_NAME_BLANK_VALUE                 SS_ERR(3)

/* end of ob_ss/resource/resource.h */
