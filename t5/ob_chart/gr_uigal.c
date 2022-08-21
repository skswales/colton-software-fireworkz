/* gr_spgal.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1993-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Handle split off chart galleries */

#include "common/gflags.h"

#include "ob_chart/ob_chart.h"

static const DIALOG_CONTROL
bl_gallery_pict_group =
{
    BL_GALLERY_ID_PICT_GROUP, DIALOG_MAIN_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { 0 },
    { DRT(LTRB, GROUPBOX) }
};

static const RESOURCE_BITMAP_ID
bar_gallery_pict_bitmap[8] =
{
    { OBJECT_ID_CHART, CHART_ID_BM_B0 },
    { OBJECT_ID_CHART, CHART_ID_BM_B1 },
    { OBJECT_ID_CHART, CHART_ID_BM_B2 },
    { OBJECT_ID_CHART, CHART_ID_BM_B3 },
    { OBJECT_ID_CHART, CHART_ID_BM_B4 },
    { OBJECT_ID_CHART, CHART_ID_BM_B5 },
    { OBJECT_ID_CHART, CHART_ID_BM_B6 },
    { OBJECT_ID_CHART, CHART_ID_BM_B7 }
};

static const DIALOG_CONTROL_DATA_RADIOPICTURE
bar_gallery_pict_data[8] =
{
    { { 0 }, 1, &bar_gallery_pict_bitmap[0] },
    { { 0 }, 2, &bar_gallery_pict_bitmap[1] },
    { { 0 }, 3, &bar_gallery_pict_bitmap[2] },
    { { 0 }, 4, &bar_gallery_pict_bitmap[3] },
    { { 0 }, 5, &bar_gallery_pict_bitmap[4] },
    { { 0 }, 6, &bar_gallery_pict_bitmap[5] },
    { { 0 }, 7, &bar_gallery_pict_bitmap[6] },
    { { 0 }, 8, &bar_gallery_pict_bitmap[7] }
};

static S32
bar_gallery_selected_pict = 1;

static const DIALOG_CONTROL
bl_gallery_pict[8] =
{
    {
        BL_GALLERY_ID_PICT_0, BL_GALLERY_ID_PICT_GROUP,
        { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT, DIALOG_CONTROL_SELF, DIALOG_CONTROL_SELF },
        { 0, 0, BAR_GALLERY_PICT_H, BAR_GALLERY_PICT_V },
        { DRT(LTLT, RADIOPICTURE), 1 /*tabstop*/ }
    },

    {
        BL_GALLERY_ID_PICT_1, BL_GALLERY_ID_PICT_GROUP,
        { BL_GALLERY_ID_PICT_0, BL_GALLERY_ID_PICT_0, DIALOG_CONTROL_SELF, BL_GALLERY_ID_PICT_0 },
        { 0, 0, BAR_GALLERY_PICT_H, 0 },
        { DRT(RTLB, RADIOPICTURE) }
    },

    {
        BL_GALLERY_ID_PICT_2, BL_GALLERY_ID_PICT_GROUP,
        { BL_GALLERY_ID_PICT_0, BL_GALLERY_ID_PICT_0, BL_GALLERY_ID_PICT_0, DIALOG_CONTROL_SELF },
        { 0, 0, 0, BAR_GALLERY_PICT_V },
        { DRT(LBRT, RADIOPICTURE) }
    },

    {
        BL_GALLERY_ID_PICT_3, BL_GALLERY_ID_PICT_GROUP,
        { BL_GALLERY_ID_PICT_2, BL_GALLERY_ID_PICT_2, DIALOG_CONTROL_SELF, BL_GALLERY_ID_PICT_2 },
        { 0, 0, BAR_GALLERY_PICT_H, 0 },
        { DRT(RTLB, RADIOPICTURE) }
    },

    {
        BL_GALLERY_ID_PICT_4, BL_GALLERY_ID_PICT_GROUP,
        { BL_GALLERY_ID_PICT_2, BL_GALLERY_ID_PICT_2, BL_GALLERY_ID_PICT_2, DIALOG_CONTROL_SELF },
        { 0, 0, 0, BAR_GALLERY_PICT_V },
        { DRT(LBRT, RADIOPICTURE) }
    },

    {
        BL_GALLERY_ID_PICT_5, BL_GALLERY_ID_PICT_GROUP,
        { BL_GALLERY_ID_PICT_4, BL_GALLERY_ID_PICT_4, DIALOG_CONTROL_SELF, BL_GALLERY_ID_PICT_4 },
        { 0, 0, BAR_GALLERY_PICT_H, 0 },
        { DRT(RTLB, RADIOPICTURE) }
    },

    {
        BL_GALLERY_ID_PICT_6, BL_GALLERY_ID_PICT_GROUP,
        { BL_GALLERY_ID_PICT_4, BL_GALLERY_ID_PICT_4, BL_GALLERY_ID_PICT_4, DIALOG_CONTROL_SELF },
        { 0, 0, 0, BAR_GALLERY_PICT_V },
        { DRT(LBRT, RADIOPICTURE) }
    },

    {
        BL_GALLERY_ID_PICT_7, BL_GALLERY_ID_PICT_GROUP,
        { BL_GALLERY_ID_PICT_6, BL_GALLERY_ID_PICT_6, DIALOG_CONTROL_SELF, BL_GALLERY_ID_PICT_6 },
        { 0, 0, BAR_GALLERY_PICT_H, 0 },
        { DRT(RTLB, RADIOPICTURE) }
    }
};

static const DIALOG_CONTROL
bar_gallery_3d =
{
    BL_GALLERY_ID_3D, DIALOG_MAIN_GROUP,
    { BL_GALLERY_ID_PICT_GROUP, BL_GALLERY_ID_PICT_GROUP },
    { DIALOG_GROUPSPACING_H, 0, DIALOG_CONTENTS_CALC, DIALOG_STDCHECK_V },
    { DRT(RTLT, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_CHECKBOX
bar_gallery_3d_data = { { 0 }, UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_BAR_GALLERY_3D) };

/*
series group
*/

static const DIALOG_CONTROL
bl_gallery_arrange_group =
{
    BL_GALLERY_ID_ARRANGE_GROUP, DIALOG_MAIN_GROUP,
    { BL_GALLERY_ID_3D, BL_GALLERY_ID_3D, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { 0, DIALOG_GROUPSPACING_V + 2 * DIALOG_SMALLSPACING_V, 0, 0 },
    { DRT(LBRB, GROUPBOX) }
};

static const DIALOG_CONTROL
bl_gallery_series_group =
{
    BL_GALLERY_ID_SERIES_GROUP, BL_GALLERY_ID_ARRANGE_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { 0, 0, 0, 0 },
    { DRT(LTRB, GROUPBOX) }
};

static const DIALOG_CONTROL
bl_gallery_series_text =
{
    BL_GALLERY_ID_SERIES_TEXT, BL_GALLERY_ID_SERIES_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { 0, 0, DIALOG_CONTENTS_CALC, DIALOG_STDRADIO_V },
    { DRT(LTLT, STATICTEXT) }
};

static const DIALOG_CONTROL_DATA_STATICTEXT
bl_gallery_series_text_data = { UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_BL_GALLERY_SERIES_TEXT), { 1 /*left_text*/ } };

static const DIALOG_CONTROL
bl_gallery_series_cols =
{
    BL_GALLERY_ID_SERIES_COLS, BL_GALLERY_ID_SERIES_GROUP,
    { BL_GALLERY_ID_SERIES_TEXT, BL_GALLERY_ID_SERIES_TEXT },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDRADIO_V },
    { DRT(LBLT, RADIOBUTTON), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_RADIOBUTTON
bl_gallery_series_cols_data = { { 0 }, 1, UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_BL_GALLERY_SERIES_COLS) };

static const DIALOG_CONTROL
bl_gallery_series_rows =
{
    BL_GALLERY_ID_SERIES_ROWS, BL_GALLERY_ID_SERIES_GROUP,
    { BL_GALLERY_ID_SERIES_COLS, BL_GALLERY_ID_SERIES_COLS, DIALOG_CONTROL_SELF, BL_GALLERY_ID_SERIES_COLS },
    { DIALOG_STDSPACING_H, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(RTLB, RADIOBUTTON) }
};

static const DIALOG_CONTROL_DATA_RADIOBUTTON
bl_gallery_series_rows_data = { { 0 }, 0, UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_BL_GALLERY_SERIES_ROWS) };

/*
first series group
*/

static const DIALOG_CONTROL
bl_gallery_first_series_group =
{
    BL_GALLERY_ID_FIRST_SERIES_GROUP, BL_GALLERY_ID_ARRANGE_GROUP,
    { BL_GALLERY_ID_SERIES_GROUP, BL_GALLERY_ID_SERIES_GROUP, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { 0, DIALOG_GROUPSPACING_V + 2 * DIALOG_SMALLSPACING_V, 0, 0 },
    { DRT(LBRB, GROUPBOX) }
};

static const DIALOG_CONTROL
bl_gallery_first_series_text =
{
    BL_GALLERY_ID_FIRST_SERIES_TEXT, BL_GALLERY_ID_FIRST_SERIES_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { 0, 0, DIALOG_CONTENTS_CALC, DIALOG_STDRADIO_V },
    { DRT(LTLT, STATICTEXT) }
};

static const DIALOG_CONTROL_DATA_STATICTEXT
bl_gallery_first_series_text_data = { UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_BL_GALLERY_FIRST_SERIES_TEXT), { 1 /*left_text*/ } };

static const DIALOG_CONTROL
bl_gallery_first_series_category_labels =
{
    BL_GALLERY_ID_FIRST_SERIES_CATEGORY_LABELS, BL_GALLERY_ID_FIRST_SERIES_GROUP,
    { BL_GALLERY_ID_FIRST_SERIES_TEXT, BL_GALLERY_ID_FIRST_SERIES_TEXT },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDRADIO_V },
    { DRT(LBLT, RADIOBUTTON), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_RADIOBUTTON
bl_gallery_first_series_category_labels_data = { { 0 }, 1, UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_BL_GALLERY_FIRST_SERIES_CATEGORY_X_LABELS) };

static const DIALOG_CONTROL
bl_gallery_first_series_series_data =
{
    BL_GALLERY_ID_FIRST_SERIES_SERIES_DATA, BL_GALLERY_ID_FIRST_SERIES_GROUP,
    { BL_GALLERY_ID_FIRST_SERIES_CATEGORY_LABELS, BL_GALLERY_ID_FIRST_SERIES_CATEGORY_LABELS },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDRADIO_V },
    { DRT(LBLT, RADIOBUTTON) }
};

static const DIALOG_CONTROL_DATA_RADIOBUTTON
bl_gallery_first_series_series_data_data = { { 0 }, 0, UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_BL_GALLERY_FIRST_SERIES_SERIES_DATA) };

/*
first category group
*/

static const DIALOG_CONTROL
bl_gallery_first_category_group =
{
    BL_GALLERY_ID_FIRST_CATEGORY_GROUP, BL_GALLERY_ID_ARRANGE_GROUP,
    { BL_GALLERY_ID_FIRST_SERIES_GROUP, BL_GALLERY_ID_FIRST_SERIES_GROUP, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { 0, DIALOG_GROUPSPACING_V + 2 * DIALOG_SMALLSPACING_V, 0, 0 },
    { DRT(LBRB, GROUPBOX) }
};

static const DIALOG_CONTROL
bl_gallery_first_category_text =
{
    BL_GALLERY_ID_FIRST_CATEGORY_TEXT, BL_GALLERY_ID_FIRST_CATEGORY_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { 0, 0, DIALOG_CONTENTS_CALC, DIALOG_STDRADIO_V },
    { DRT(LTLT, STATICTEXT) }
};

static const DIALOG_CONTROL_DATA_STATICTEXT
bl_gallery_first_category_text_data = { UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_BL_GALLERY_FIRST_CATEGORY_TEXT), { 1 /*left_text*/ } };

static const DIALOG_CONTROL
bl_gallery_first_category_series_labels =
{
    BL_GALLERY_ID_FIRST_CATEGORY_SERIES_LABELS, BL_GALLERY_ID_FIRST_CATEGORY_GROUP,
    { BL_GALLERY_ID_FIRST_CATEGORY_TEXT, BL_GALLERY_ID_FIRST_CATEGORY_TEXT },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDRADIO_V },
    { DRT(LBLT, RADIOBUTTON), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_RADIOBUTTON
bl_gallery_first_category_series_labels_data = { { 0 }, 1, UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_BL_GALLERY_FIRST_CATEGORY_SERIES_LABELS) };

static const DIALOG_CONTROL
bl_gallery_first_category_category_data =
{
    BL_GALLERY_ID_FIRST_CATEGORY_CATEGORY_DATA, BL_GALLERY_ID_FIRST_CATEGORY_GROUP,
    { BL_GALLERY_ID_FIRST_CATEGORY_SERIES_LABELS, BL_GALLERY_ID_FIRST_CATEGORY_SERIES_LABELS },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDRADIO_V },
    { DRT(LBLT, RADIOBUTTON) }
};

static const DIALOG_CONTROL_DATA_RADIOBUTTON
bl_gallery_first_category_category_data_data = { { 0 }, 0, UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_BL_GALLERY_FIRST_CATEGORY_CATEGORY_DATA) };

static const DIALOG_CONTROL
bl_gallery_ok =
{
    IDOK, DIALOG_CONTROL_WINDOW,
    { DIALOG_CONTROL_SELF, DIALOG_MAIN_GROUP, DIALOG_MAIN_GROUP, DIALOG_CONTROL_SELF },
#if RISCOS
    { DIALOG_CONTENTS_CALC, DIALOG_STDSPACING_V, 0, DIALOG_DEFPUSHBUTTON_V },
#else
    { DIALOG_DEFOK_H, DIALOG_STDSPACING_V, 0, DIALOG_DEFPUSHBUTTON_V },
#endif
    { DRT(RBRT, PUSHBUTTON), 1 /*tabstop*/, 1 /*logical_group*/ }
};

static const DIALOG_CTL_CREATE
bar_gallery_ctl_create[] =
{
    { &dialog_main_group },

    { &bl_gallery_pict_group, NULL },
    { &bl_gallery_pict[0], &bar_gallery_pict_data[0] },
    { &bl_gallery_pict[1], &bar_gallery_pict_data[1] },
    { &bl_gallery_pict[2], &bar_gallery_pict_data[2] },
    { &bl_gallery_pict[3], &bar_gallery_pict_data[3] },
    { &bl_gallery_pict[4], &bar_gallery_pict_data[4] },
    { &bl_gallery_pict[5], &bar_gallery_pict_data[5] },
    { &bl_gallery_pict[6], &bar_gallery_pict_data[6] },
    { &bl_gallery_pict[7], &bar_gallery_pict_data[7] },

    { &bar_gallery_3d, &bar_gallery_3d_data },

    { &bl_gallery_arrange_group, NULL },

    { &bl_gallery_series_group, NULL },
    { &bl_gallery_series_text, &bl_gallery_series_text_data },
    { &bl_gallery_series_cols, &bl_gallery_series_cols_data },
    { &bl_gallery_series_rows, &bl_gallery_series_rows_data },

    { &bl_gallery_first_series_group, NULL },
    { &bl_gallery_first_series_text, &bl_gallery_first_series_text_data },
    { &bl_gallery_first_series_category_labels, &bl_gallery_first_series_category_labels_data },
    { &bl_gallery_first_series_series_data, &bl_gallery_first_series_series_data_data },

    { &bl_gallery_first_category_group, NULL },
    { &bl_gallery_first_category_text, &bl_gallery_first_category_text_data },
    { &bl_gallery_first_category_series_labels, &bl_gallery_first_category_series_labels_data },
    { &bl_gallery_first_category_category_data, &bl_gallery_first_category_category_data_data },

    { &bl_gallery_ok, &defbutton_ok_data },
    { &stdbutton_cancel, &stdbutton_cancel_data }
};

static const RESOURCE_BITMAP_ID
line_gallery_pict_bitmap[8] =
{
    { OBJECT_ID_CHART, CHART_ID_BM_L0 },
    { OBJECT_ID_CHART, CHART_ID_BM_L1 },
    { OBJECT_ID_CHART, CHART_ID_BM_L2 },
    { OBJECT_ID_CHART, CHART_ID_BM_L3 },
    { OBJECT_ID_CHART, CHART_ID_BM_L4 },
    { OBJECT_ID_CHART, CHART_ID_BM_L5 },
    { OBJECT_ID_CHART, CHART_ID_BM_L6 },
    { OBJECT_ID_CHART, CHART_ID_BM_L7 }
};

static const DIALOG_CONTROL_DATA_RADIOPICTURE
line_gallery_pict_data[8] =
{
    { { 0 }, 1, &line_gallery_pict_bitmap[0] },
    { { 0 }, 2, &line_gallery_pict_bitmap[1] },
    { { 0 }, 3, &line_gallery_pict_bitmap[2] },
    { { 0 }, 4, &line_gallery_pict_bitmap[3] },
    { { 0 }, 5, &line_gallery_pict_bitmap[4] },
    { { 0 }, 6, &line_gallery_pict_bitmap[5] },
    { { 0 }, 7, &line_gallery_pict_bitmap[6] },
    { { 0 }, 8, &line_gallery_pict_bitmap[7] }
};

static S32
line_gallery_selected_pict = 1;

static const DIALOG_CTL_CREATE
line_gallery_ctl_create[] =
{
    { &dialog_main_group },

    { &bl_gallery_pict_group, NULL },
    { &bl_gallery_pict[0], &line_gallery_pict_data[0] },
    { &bl_gallery_pict[1], &line_gallery_pict_data[1] },
    { &bl_gallery_pict[2], &line_gallery_pict_data[2] },
    { &bl_gallery_pict[3], &line_gallery_pict_data[3] },
    { &bl_gallery_pict[4], &line_gallery_pict_data[4] },
    { &bl_gallery_pict[5], &line_gallery_pict_data[5] },
    { &bl_gallery_pict[6], &line_gallery_pict_data[6] },
    { &bl_gallery_pict[7], &line_gallery_pict_data[7] },

    { &bar_gallery_3d, &bar_gallery_3d_data },

    { &bl_gallery_arrange_group, NULL },

    { &bl_gallery_series_group, NULL },
    { &bl_gallery_series_text, &bl_gallery_series_text_data },
    { &bl_gallery_series_cols, &bl_gallery_series_cols_data },
    { &bl_gallery_series_rows, &bl_gallery_series_rows_data },

    { &bl_gallery_first_series_group, NULL },
    { &bl_gallery_first_series_text, &bl_gallery_first_series_text_data },
    { &bl_gallery_first_series_category_labels, &bl_gallery_first_series_category_labels_data },
    { &bl_gallery_first_series_series_data, &bl_gallery_first_series_series_data_data },

    { &bl_gallery_first_category_group, NULL },
    { &bl_gallery_first_category_text, &bl_gallery_first_category_text_data },
    { &bl_gallery_first_category_series_labels, &bl_gallery_first_category_series_labels_data },
    { &bl_gallery_first_category_category_data, &bl_gallery_first_category_category_data_data },

    { &stdbutton_cancel, &stdbutton_cancel_data },
    { &bl_gallery_ok, &defbutton_ok_data }
};

static const RESOURCE_BITMAP_ID
over_bl_gallery_pict_bitmap[3] =
{
    { OBJECT_ID_CHART, CHART_ID_BM_O0 },
    { OBJECT_ID_CHART, CHART_ID_BM_O1 },
    { OBJECT_ID_CHART, CHART_ID_BM_O2 }
};

static const DIALOG_CONTROL_DATA_RADIOPICTURE
over_bl_gallery_pict_data[3] =
{
    { { 0 }, 1, &over_bl_gallery_pict_bitmap[0] },
    { { 0 }, 2, &over_bl_gallery_pict_bitmap[1] },
    { { 0 }, 3, &over_bl_gallery_pict_bitmap[2] }
};

static const DIALOG_CONTROL
over_bl_gallery_pict[3] =
{
    {
        BL_GALLERY_ID_PICT_0, BL_GALLERY_ID_PICT_GROUP,
        { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT, DIALOG_CONTROL_SELF, DIALOG_CONTROL_SELF },
        { 0, 0, BAR_GALLERY_PICT_H, BAR_GALLERY_PICT_V }, { DRT(LTLT, RADIOPICTURE), 1 /*tabstop*/ }
    },

    {
        BL_GALLERY_ID_PICT_3, BL_GALLERY_ID_PICT_GROUP,
        { BL_GALLERY_ID_PICT_0, BL_GALLERY_ID_PICT_0, BL_GALLERY_ID_PICT_0, DIALOG_CONTROL_SELF },
        { 0, 0, 0, BAR_GALLERY_PICT_V }, { DRT(LBRT, RADIOPICTURE) }
    },

    {
        BL_GALLERY_ID_PICT_6, BL_GALLERY_ID_PICT_GROUP,
        { BL_GALLERY_ID_PICT_3, BL_GALLERY_ID_PICT_3, BL_GALLERY_ID_PICT_3, DIALOG_CONTROL_SELF },
        { 0, 0, 0, BAR_GALLERY_PICT_V }, { DRT(LBRT, RADIOPICTURE) }
    }
};

static S32
over_bl_gallery_selected_pict = 1;

static const DIALOG_CTL_CREATE
over_bl_gallery_ctl_create[] =
{
    { &dialog_main_group },

    { &bl_gallery_pict_group, NULL },
    { &over_bl_gallery_pict[0], &over_bl_gallery_pict_data[0] },
    { &over_bl_gallery_pict[1], &over_bl_gallery_pict_data[1] },
    { &over_bl_gallery_pict[2], &over_bl_gallery_pict_data[2] },

    { &bar_gallery_3d, &bar_gallery_3d_data },

    { &bl_gallery_arrange_group, NULL },

    { &bl_gallery_series_group, NULL },
    { &bl_gallery_series_text, &bl_gallery_series_text_data },
    { &bl_gallery_series_cols, &bl_gallery_series_cols_data },
    { &bl_gallery_series_rows, &bl_gallery_series_rows_data },

    { &bl_gallery_first_series_group, NULL },
    { &bl_gallery_first_series_text, &bl_gallery_first_series_text_data },
    { &bl_gallery_first_series_category_labels, &bl_gallery_first_series_category_labels_data },
    { &bl_gallery_first_series_series_data, &bl_gallery_first_series_series_data_data },

    { &bl_gallery_first_category_group, NULL },
    { &bl_gallery_first_category_text, &bl_gallery_first_category_text_data },
    { &bl_gallery_first_category_series_labels, &bl_gallery_first_category_series_labels_data },
    { &bl_gallery_first_category_category_data, &bl_gallery_first_category_category_data_data },

    { &stdbutton_cancel, &stdbutton_cancel_data },
    { &bl_gallery_ok, &defbutton_ok_data }
};

static const RESOURCE_BITMAP_ID
scat_gallery_pict_bitmap[9] =
{
    { OBJECT_ID_CHART, CHART_ID_BM_S0 },
    { OBJECT_ID_CHART, CHART_ID_BM_S1 },
    { OBJECT_ID_CHART, CHART_ID_BM_S2 },
    { OBJECT_ID_CHART, CHART_ID_BM_S3 },
    { OBJECT_ID_CHART, CHART_ID_BM_S4 },
    { OBJECT_ID_CHART, CHART_ID_BM_S5 },
    { OBJECT_ID_CHART, CHART_ID_BM_S6 },
    { OBJECT_ID_CHART, CHART_ID_BM_S7 },
    { OBJECT_ID_CHART, CHART_ID_BM_S8 }
};

static const DIALOG_CONTROL_DATA_RADIOPICTURE
scat_gallery_pict_data[9] =
{
    { { 0 }, 1, &scat_gallery_pict_bitmap[0] },
    { { 0 }, 2, &scat_gallery_pict_bitmap[1] },
    { { 0 }, 3, &scat_gallery_pict_bitmap[2] },
    { { 0 }, 4, &scat_gallery_pict_bitmap[3] },
    { { 0 }, 5, &scat_gallery_pict_bitmap[4] },
    { { 0 }, 6, &scat_gallery_pict_bitmap[5] },
    { { 0 }, 7, &scat_gallery_pict_bitmap[6] },
    { { 0 }, 8, &scat_gallery_pict_bitmap[7] },
    { { 0 }, 9, &scat_gallery_pict_bitmap[8] }
};

static const DIALOG_CONTROL
scat_gallery_pict[9] =
{
    {
        BL_GALLERY_ID_PICT_0, BL_GALLERY_ID_PICT_GROUP,
        { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT, DIALOG_CONTROL_SELF, DIALOG_CONTROL_SELF },
        { 0, 0, BAR_GALLERY_PICT_H, BAR_GALLERY_PICT_V }, { DRT(LTLT, RADIOPICTURE), 1 /*tabstop*/ }
    },

    {
        BL_GALLERY_ID_PICT_1, BL_GALLERY_ID_PICT_GROUP,
        { BL_GALLERY_ID_PICT_0, BL_GALLERY_ID_PICT_0, DIALOG_CONTROL_SELF, BL_GALLERY_ID_PICT_0 },
        { 0, 0, BAR_GALLERY_PICT_H, 0 }, { DRT(RTLB, RADIOPICTURE) }
    },

    {
        BL_GALLERY_ID_PICT_2, BL_GALLERY_ID_PICT_GROUP,
        { BL_GALLERY_ID_PICT_1, BL_GALLERY_ID_PICT_1, DIALOG_CONTROL_SELF, BL_GALLERY_ID_PICT_1 },
        { 0, 0, BAR_GALLERY_PICT_H, 0 }, { DRT(RTLB, RADIOPICTURE) }
    },

    {
        BL_GALLERY_ID_PICT_3, BL_GALLERY_ID_PICT_GROUP,
        { BL_GALLERY_ID_PICT_0, BL_GALLERY_ID_PICT_0, BL_GALLERY_ID_PICT_0, DIALOG_CONTROL_SELF },
        { 0, 0, 0, BAR_GALLERY_PICT_V }, { DRT(LBRT, RADIOPICTURE) }
    },

    {
        BL_GALLERY_ID_PICT_4, BL_GALLERY_ID_PICT_GROUP,
        { BL_GALLERY_ID_PICT_3, BL_GALLERY_ID_PICT_3, DIALOG_CONTROL_SELF, BL_GALLERY_ID_PICT_3 },
        { 0, 0, BAR_GALLERY_PICT_H, 0 }, { DRT(RTLB, RADIOPICTURE) }
    },

    {
        BL_GALLERY_ID_PICT_5, BL_GALLERY_ID_PICT_GROUP,
        { BL_GALLERY_ID_PICT_4, BL_GALLERY_ID_PICT_4, DIALOG_CONTROL_SELF, BL_GALLERY_ID_PICT_4 },
        { 0, 0, BAR_GALLERY_PICT_H, 0 }, { DRT(RTLB, RADIOPICTURE) }
    },

    {
        BL_GALLERY_ID_PICT_6, BL_GALLERY_ID_PICT_GROUP,
        { BL_GALLERY_ID_PICT_3, BL_GALLERY_ID_PICT_3, BL_GALLERY_ID_PICT_3, DIALOG_CONTROL_SELF },
        { 0, 0, 0, BAR_GALLERY_PICT_V }, { DRT(LBRT, RADIOPICTURE) }
    },

    {
        BL_GALLERY_ID_PICT_7, BL_GALLERY_ID_PICT_GROUP,
        { BL_GALLERY_ID_PICT_6, BL_GALLERY_ID_PICT_6, DIALOG_CONTROL_SELF, BL_GALLERY_ID_PICT_6 },
        { 0, 0, BAR_GALLERY_PICT_H, 0 }, { DRT(RTLB, RADIOPICTURE) }
    },

    {
        BL_GALLERY_ID_PICT_8, BL_GALLERY_ID_PICT_GROUP,
        { BL_GALLERY_ID_PICT_7, BL_GALLERY_ID_PICT_7, DIALOG_CONTROL_SELF, BL_GALLERY_ID_PICT_7 },
        { 0, 0, BAR_GALLERY_PICT_H, 0 }, { DRT(RTLB, RADIOPICTURE) }
    }
};

static S32
scat_gallery_selected_pict = 1;

static const DIALOG_CONTROL
scat_gallery_arrange_group =
{
    BL_GALLERY_ID_ARRANGE_GROUP, DIALOG_MAIN_GROUP,
    { BL_GALLERY_ID_PICT_GROUP, BL_GALLERY_ID_PICT_GROUP, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { DIALOG_STDSPACING_H, 0, 0, 0 },
    { DRT(RTRB, GROUPBOX) }
};

static const DIALOG_CONTROL
scat_gallery_first_series_text =
{
    BL_GALLERY_ID_FIRST_SERIES_TEXT, BL_GALLERY_ID_FIRST_SERIES_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { 0, 0, DIALOG_CONTENTS_CALC, DIALOG_STDRADIO_V },
    { DRT(LTLT, STATICTEXT) }
};

static const DIALOG_CONTROL_DATA_STATICTEXT
scat_gallery_first_series_text_data = { UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_BL_GALLERY_FIRST_SERIES_TEXT1), { 1 /*left_text*/ } };

static const DIALOG_CONTROL
scat_gallery_first_series_xyy_data =
{
    BL_GALLERY_ID_FIRST_SERIES_CATEGORY_LABELS, BL_GALLERY_ID_FIRST_SERIES_GROUP,
    { BL_GALLERY_ID_FIRST_SERIES_TEXT, BL_GALLERY_ID_FIRST_SERIES_TEXT },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDRADIO_V },
    { DRT(LBLT, RADIOBUTTON), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_RADIOBUTTON
scat_gallery_first_series_xyy_data_data = { { 0 }, 1, UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_BL_GALLERY_FIRST_SERIES_XYY_DATA) };

static const DIALOG_CONTROL
scat_gallery_first_series_xyxy_data =
{
    BL_GALLERY_ID_FIRST_SERIES_SERIES_DATA, BL_GALLERY_ID_FIRST_SERIES_GROUP,
    { BL_GALLERY_ID_FIRST_SERIES_CATEGORY_LABELS, BL_GALLERY_ID_FIRST_SERIES_CATEGORY_LABELS },
    { DIALOG_STDSPACING_H, 0, DIALOG_CONTENTS_CALC, DIALOG_STDRADIO_V },
    { DRT(RTLT, RADIOBUTTON) }
};

static const DIALOG_CONTROL_DATA_RADIOBUTTON
scat_gallery_first_series_xyxy_data_data = { { 0 }, 0, UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_BL_GALLERY_FIRST_SERIES_XYXY_DATA) };

static const DIALOG_CONTROL_DATA_STATICTEXT
scat_gallery_first_point_text_data = { UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_BL_GALLERY_FIRST_POINT_TEXT), { 1 /*left_text*/ } };

static const DIALOG_CONTROL
scat_gallery_first_point_point_data =
{
    BL_GALLERY_ID_FIRST_CATEGORY_CATEGORY_DATA, BL_GALLERY_ID_FIRST_CATEGORY_GROUP,
    { BL_GALLERY_ID_FIRST_CATEGORY_SERIES_LABELS, BL_GALLERY_ID_FIRST_CATEGORY_SERIES_LABELS },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDRADIO_V },
    { DRT(LBLT, RADIOBUTTON) }
};

static const DIALOG_CONTROL_DATA_RADIOBUTTON
scat_gallery_first_point_point_data_data = { { 0 }, 0, UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_BL_GALLERY_FIRST_POINT_POINT_DATA) };

static const DIALOG_CTL_CREATE
scat_gallery_ctl_create[] =
{
    { &dialog_main_group },

    { &bl_gallery_pict_group, NULL },
    { &scat_gallery_pict[0], &scat_gallery_pict_data[0] },
    { &scat_gallery_pict[1], &scat_gallery_pict_data[1] },
    { &scat_gallery_pict[2], &scat_gallery_pict_data[2] },
    { &scat_gallery_pict[3], &scat_gallery_pict_data[3] },
    { &scat_gallery_pict[4], &scat_gallery_pict_data[4] },
    { &scat_gallery_pict[5], &scat_gallery_pict_data[5] },
    { &scat_gallery_pict[6], &scat_gallery_pict_data[6] },
    { &scat_gallery_pict[7], &scat_gallery_pict_data[7] },
    { &scat_gallery_pict[8], &scat_gallery_pict_data[8] },

    { &scat_gallery_arrange_group, NULL },

    { &bl_gallery_series_group, NULL },
    { &bl_gallery_series_text, &bl_gallery_series_text_data },
    { &bl_gallery_series_cols, &bl_gallery_series_cols_data },
    { &bl_gallery_series_rows, &bl_gallery_series_rows_data },

    { &bl_gallery_first_series_group, NULL },
    { &scat_gallery_first_series_text, &scat_gallery_first_series_text_data },
    { &scat_gallery_first_series_xyy_data, &scat_gallery_first_series_xyy_data_data },
    { &scat_gallery_first_series_xyxy_data, &scat_gallery_first_series_xyxy_data_data },

    { &bl_gallery_first_category_group, NULL },
    { &bl_gallery_first_category_text, &scat_gallery_first_point_text_data },
    { &bl_gallery_first_category_series_labels, &bl_gallery_first_category_series_labels_data },
    { &scat_gallery_first_point_point_data, &scat_gallery_first_point_point_data_data },

    { &stdbutton_cancel, &stdbutton_cancel_data },
    { &bl_gallery_ok, &defbutton_ok_data }
};

/*
label group
*/

static const DIALOG_CONTROL
pie_gallery_label_group =
{
    BL_GALLERY_ID_PICT_GROUP, DIALOG_MAIN_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { 0, 0, DIALOG_STDGROUP_RM, DIALOG_STDGROUP_BM },
    { DRT(LTRB, GROUPBOX) }
};

static const DIALOG_CONTROL_DATA_GROUPBOX
pie_gallery_label_group_data = { UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_PIE_GALLERY_LABEL), { FRAMED_BOX_GROUP } };

static const DIALOG_CONTROL
pie_gallery_label_none =
{
    PIE_GALLERY_ID_LABEL_NONE, PIE_GALLERY_ID_LABEL_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { DIALOG_STDGROUP_LM, DIALOG_STDGROUP_TM, PIE_GALLERY_LABELPICT_H, PIE_GALLERY_LABELPICT_V },
    { DRT(LTLT, RADIOPICTURE), 1 /*tabstop*/ }
};

static const RESOURCE_BITMAP_ID
pie_gallery_label_pict_bitmap[4] =
{
    { OBJECT_ID_CHART, CHART_ID_BM_PL0 },
    { OBJECT_ID_CHART, CHART_ID_BM_PL1 },
    { OBJECT_ID_CHART, CHART_ID_BM_PL2 },
    { OBJECT_ID_CHART, CHART_ID_BM_PL3 }
};

static const DIALOG_CONTROL_DATA_RADIOPICTURE
pie_gallery_label_pict_data[4] =
{
    { { 0 }, 1, &pie_gallery_label_pict_bitmap[0] },
    { { 0 }, 2, &pie_gallery_label_pict_bitmap[1] },
    { { 0 }, 3, &pie_gallery_label_pict_bitmap[2] },
    { { 0 }, 4, &pie_gallery_label_pict_bitmap[3] }
};

static const DIALOG_CONTROL
pie_gallery_label_label =
{
    PIE_GALLERY_ID_LABEL_LABEL, PIE_GALLERY_ID_LABEL_GROUP,
    { PIE_GALLERY_ID_LABEL_NONE, PIE_GALLERY_ID_LABEL_NONE, DIALOG_CONTROL_SELF, PIE_GALLERY_ID_LABEL_NONE },
    { 0, 0, PIE_GALLERY_LABELPICT_H, 0 },
    { DRT(RTLB, RADIOPICTURE) }
};

static const DIALOG_CONTROL
pie_gallery_label_value =
{
    PIE_GALLERY_ID_LABEL_VALUE, PIE_GALLERY_ID_LABEL_GROUP,
    { PIE_GALLERY_ID_LABEL_LABEL, PIE_GALLERY_ID_LABEL_LABEL, DIALOG_CONTROL_SELF, PIE_GALLERY_ID_LABEL_LABEL },
    { 0, 0, PIE_GALLERY_LABELPICT_H, 0 },
    { DRT(RTLB, RADIOPICTURE) }
};

static const DIALOG_CONTROL
pie_gallery_label_v_pct =
{
    PIE_GALLERY_ID_LABEL_V_PCT, PIE_GALLERY_ID_LABEL_GROUP,
    { PIE_GALLERY_ID_LABEL_VALUE, PIE_GALLERY_ID_LABEL_VALUE, DIALOG_CONTROL_SELF, PIE_GALLERY_ID_LABEL_VALUE },
    { 0, 0, PIE_GALLERY_LABELPICT_H, 0 },
    { DRT(RTLB, RADIOPICTURE) }
};

static S32
pie_gallery_selected_label_pict = 1;

/*
explode group
*/

static const DIALOG_CONTROL
pie_gallery_explode_group =
{
    PIE_GALLERY_ID_EXPLODE_GROUP, DIALOG_MAIN_GROUP,
    { PIE_GALLERY_ID_LABEL_GROUP, PIE_GALLERY_ID_LABEL_GROUP, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { 0, DIALOG_GROUPSPACING_V, DIALOG_STDGROUP_RM, DIALOG_STDGROUP_BM },
    { DRT(LBRB, GROUPBOX) }
};

static const DIALOG_CONTROL_DATA_GROUPBOX
pie_gallery_explode_group_data = { UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_PIE_GALLERY_EXPLODE), { FRAMED_BOX_GROUP } };

static const DIALOG_CONTROL
pie_gallery_explode_none =
{
    PIE_GALLERY_ID_EXPLODE_NONE, PIE_GALLERY_ID_EXPLODE_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { DIALOG_STDGROUP_LM, DIALOG_STDGROUP_TM, PIE_GALLERY_EXPLODEPICT_H, PIE_GALLERY_EXPLODEPICT_V },
    { DRT(LTLT, RADIOPICTURE), 1 /*tabstop*/ }
};

static const RESOURCE_BITMAP_ID
pie_gallery_explode_pict_bitmap[3] =
{
    { OBJECT_ID_CHART, CHART_ID_BM_PL0 }, /*shared*/
    { OBJECT_ID_CHART, CHART_ID_BM_PX1 },
    { OBJECT_ID_CHART, CHART_ID_BM_PX2 }
};

static const DIALOG_CONTROL_DATA_RADIOPICTURE
pie_gallery_explode_pict_data[3] =
{
    { { 0 }, 1, &pie_gallery_explode_pict_bitmap[0] },
    { { 0 }, 2, &pie_gallery_explode_pict_bitmap[1] },
    { { 0 }, 3, &pie_gallery_explode_pict_bitmap[2] }
};

static const DIALOG_CONTROL
pie_gallery_explode_first =
{
    PIE_GALLERY_ID_EXPLODE_FIRST, PIE_GALLERY_ID_EXPLODE_GROUP,
    { PIE_GALLERY_ID_EXPLODE_NONE, PIE_GALLERY_ID_EXPLODE_NONE, DIALOG_CONTROL_SELF, PIE_GALLERY_ID_EXPLODE_NONE },
    { 0, 0, PIE_GALLERY_EXPLODEPICT_H, 0 },
    { DRT(RTLB, RADIOPICTURE) }
};

static const DIALOG_CONTROL
pie_gallery_explode_all =
{
    PIE_GALLERY_ID_EXPLODE_ALL, PIE_GALLERY_ID_EXPLODE_GROUP,
    { PIE_GALLERY_ID_EXPLODE_FIRST, PIE_GALLERY_ID_EXPLODE_FIRST, DIALOG_CONTROL_SELF, PIE_GALLERY_ID_EXPLODE_FIRST },
    { 0, 0, PIE_GALLERY_EXPLODEPICT_H, 0 },
    { DRT(RTLB, RADIOPICTURE) }
};

static const DIALOG_CONTROL
pie_gallery_explode_by_value =
{
    PIE_GALLERY_ID_EXPLODE_BY_VALUE, PIE_GALLERY_ID_EXPLODE_GROUP,
#if 1
    { PIE_GALLERY_ID_EXPLODE_NONE, PIE_GALLERY_ID_EXPLODE_NONE },
    { 0, DIALOG_STDSPACING_V, DIALOG_BUMP_H(4), DIALOG_STDBUMP_V },
    { DRT(LBLT, BUMP_F64), 1 /*tabstop*/ }
#else
    { PIE_GALLERY_ID_EXPLODE_ALL, PIE_GALLERY_ID_EXPLODE_ALL },
    { DIALOG_STDSPACING_H, 0, DIALOG_BUMP_H(4), DIALOG_STDBUMP_V },
    { DRT(RTLT, BUMP_F64), 1 /*tabstop*/ }
#endif
};

static const UI_CONTROL_F64
pie_gallery_explode_by_value_control = { 0.0, 100.0, 5.0 };

static const DIALOG_CONTROL_DATA_BUMP_F64
pie_gallery_explode_by_value_data = { { { { FRAMED_BOX_EDIT, 0, 1 /*right_text*/ } } /*EDIT_XX*/, &pie_gallery_explode_by_value_control } /*BUMP_XX*/ };

static const DIALOG_CONTROL
pie_gallery_explode_by_text =
{
    PIE_GALLERY_ID_EXPLODE_BY_TEXT, PIE_GALLERY_ID_EXPLODE_GROUP,
    { PIE_GALLERY_ID_EXPLODE_BY_VALUE, PIE_GALLERY_ID_EXPLODE_BY_VALUE, DIALOG_CONTROL_SELF, PIE_GALLERY_ID_EXPLODE_BY_VALUE },
    { DIALOG_SMALLSPACING_H, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(RTLB, STATICTEXT) }
};

static const DIALOG_CONTROL_DATA_STATICTEXT
pie_gallery_explode_by_text_data = { UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_PIE_GALLERY_EXPLODE_BY), { 1 /*left_text*/, 0 /*centre_text*/, 1 /*windows_no_colon*/ } };

/*
start angle group
*/

static const DIALOG_CONTROL
pie_gallery_start_position_group =
{
    PIE_GALLERY_ID_START_POSITION_GROUP, DIALOG_MAIN_GROUP,
    { PIE_GALLERY_ID_EXPLODE_GROUP, PIE_GALLERY_ID_EXPLODE_GROUP, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { 0, DIALOG_GROUPSPACING_V, DIALOG_STDGROUP_RM, DIALOG_STDGROUP_BM },
    { DRT(LBRB, GROUPBOX) }
};

static const DIALOG_CONTROL_DATA_GROUPBOX
pie_gallery_start_position_group_data = { UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_PIE_GALLERY_START_POSITION), { FRAMED_BOX_GROUP } };

#if 0

static const DIALOG_CONTROL
pie_gallery_start_position_circle =
{
    PIE_GALLERY_ID_START_POSITION_CIRCLE, PIE_GALLERY_ID_START_POSITION_GROUP,
    { PIE_GALLERY_ID_START_POSITION_CIRCLE_270, PIE_GALLERY_ID_START_POSITION_CIRCLE_000 },
    { -30 * PIXITS_PER_RISCOS, -30 * PIXITS_PER_RISCOS, 120 * PIXITS_PER_RISCOS, 120 * PIXITS_PER_RISCOS },
    { DRT(LTLT, STATICPICTURE) }
};

static const DIALOG_CONTROL_DATA_STATICPICTURE
pie_gallery_start_position_circle_data = { { OBJECT_ID_CHART, "pie_c" } };

#endif

static const DIALOG_CONTROL
pie_gallery_start_position_circle_000 =
{
    PIE_GALLERY_ID_START_POSITION_CIRCLE_000, PIE_GALLERY_ID_START_POSITION_GROUP,
    { PIE_GALLERY_ID_START_POSITION_CIRCLE_270, PIE_GALLERY_ID_START_POSITION_CIRCLE_270 },
    { -60 * PIXITS_PER_RISCOS, +60 * PIXITS_PER_RISCOS, DIALOG_STDRADIO_H, DIALOG_STDRADIO_V },
    { DRT(LTLT, RADIOBUTTON), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_RADIOBUTTON
pie_gallery_start_position_circle_000_data = { { 0 }, 0 };

static const DIALOG_CONTROL
pie_gallery_start_position_circle_045 =
{
    PIE_GALLERY_ID_START_POSITION_CIRCLE_045, PIE_GALLERY_ID_START_POSITION_GROUP,
    { PIE_GALLERY_ID_START_POSITION_CIRCLE_270, PIE_GALLERY_ID_START_POSITION_CIRCLE_270 },
    { -104 * PIXITS_PER_RISCOS, +44 * PIXITS_PER_RISCOS, DIALOG_STDRADIO_H, DIALOG_STDRADIO_V },
    { DRT(LTLT, RADIOBUTTON) }
};

static const DIALOG_CONTROL_DATA_RADIOBUTTON
pie_gallery_start_position_circle_045_data = { { 0 }, 45 };

static const DIALOG_CONTROL
pie_gallery_start_position_circle_090 =
{
    PIE_GALLERY_ID_START_POSITION_CIRCLE_090, PIE_GALLERY_ID_START_POSITION_GROUP,
    { PIE_GALLERY_ID_START_POSITION_CIRCLE_270, PIE_GALLERY_ID_START_POSITION_CIRCLE_270 },
    { -120 * PIXITS_PER_RISCOS, 0 * PIXITS_PER_RISCOS, DIALOG_STDRADIO_H, DIALOG_STDRADIO_V },
    { DRT(LTLT, RADIOBUTTON) }
};

static const DIALOG_CONTROL_DATA_RADIOBUTTON
pie_gallery_start_position_circle_090_data = { { 0 }, 90 };

static const DIALOG_CONTROL
pie_gallery_start_position_circle_135 =
{
    PIE_GALLERY_ID_START_POSITION_CIRCLE_135, PIE_GALLERY_ID_START_POSITION_GROUP,
    { PIE_GALLERY_ID_START_POSITION_CIRCLE_270, PIE_GALLERY_ID_START_POSITION_CIRCLE_270 },
    { -104 * PIXITS_PER_RISCOS, -44 * PIXITS_PER_RISCOS, DIALOG_STDRADIO_H, DIALOG_STDRADIO_V },
    { DRT(LTLT, RADIOBUTTON) }
};

static const DIALOG_CONTROL_DATA_RADIOBUTTON
pie_gallery_start_position_circle_135_data = { { 0 }, 135 };

static const DIALOG_CONTROL
pie_gallery_start_position_circle_180 =
{
    PIE_GALLERY_ID_START_POSITION_CIRCLE_180, PIE_GALLERY_ID_START_POSITION_GROUP,
    { PIE_GALLERY_ID_START_POSITION_CIRCLE_270, PIE_GALLERY_ID_START_POSITION_CIRCLE_270 },
    { -60 * PIXITS_PER_RISCOS, -60 * PIXITS_PER_RISCOS, DIALOG_STDRADIO_H, DIALOG_STDRADIO_V },
    { DRT(LTLT, RADIOBUTTON) }
};

static const DIALOG_CONTROL_DATA_RADIOBUTTON
pie_gallery_start_position_circle_180_data = { { 0 }, 180 };

static const DIALOG_CONTROL
pie_gallery_start_position_circle_225 =
{
    PIE_GALLERY_ID_START_POSITION_CIRCLE_225, PIE_GALLERY_ID_START_POSITION_GROUP,
    { PIE_GALLERY_ID_START_POSITION_CIRCLE_270, PIE_GALLERY_ID_START_POSITION_CIRCLE_270 },
    { -16 * PIXITS_PER_RISCOS, -44 * PIXITS_PER_RISCOS, DIALOG_STDRADIO_H, DIALOG_STDRADIO_V },
    { DRT(LTLT, RADIOBUTTON) }
};

static const DIALOG_CONTROL_DATA_RADIOBUTTON
pie_gallery_start_position_circle_225_data = { { 0 }, 225 };

static const DIALOG_CONTROL
pie_gallery_start_position_circle_270 =
{
    PIE_GALLERY_ID_START_POSITION_CIRCLE_270, PIE_GALLERY_ID_START_POSITION_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { DIALOG_STDGROUP_LM, DIALOG_STDGROUP_TM + 60 * PIXITS_PER_RISCOS, DIALOG_STDRADIO_H, DIALOG_STDRADIO_V },
    { DRT(LTLT, RADIOBUTTON) }
};

static const DIALOG_CONTROL_DATA_RADIOBUTTON
pie_gallery_start_position_circle_270_data = { { 0 }, 270 };

static const DIALOG_CONTROL
pie_gallery_start_position_circle_315 =
{
    PIE_GALLERY_ID_START_POSITION_CIRCLE_315, PIE_GALLERY_ID_START_POSITION_GROUP,
    { PIE_GALLERY_ID_START_POSITION_CIRCLE_270, PIE_GALLERY_ID_START_POSITION_CIRCLE_270 },
    { -16 * PIXITS_PER_RISCOS, +44 * PIXITS_PER_RISCOS, DIALOG_STDRADIO_H, DIALOG_STDRADIO_V },
    { DRT(LTLT, RADIOBUTTON) }
};

static const DIALOG_CONTROL_DATA_RADIOBUTTON
pie_gallery_start_position_circle_315_data = { { 0 }, 315 };

static const DIALOG_CONTROL
pie_gallery_start_position_angle_value =
{
    PIE_GALLERY_ID_START_POSITION_ANGLE_VALUE, PIE_GALLERY_ID_START_POSITION_GROUP,
    { PIE_GALLERY_ID_START_POSITION_CIRCLE_090, PIE_GALLERY_ID_START_POSITION_CIRCLE_000 },
    { DIALOG_STDSPACING_H, 0, DIALOG_BUMP_H(4), DIALOG_STDBUMP_V },
    { DRT(RTLT, BUMP_F64), 1 /*tabstop*/ }
};

static const UI_CONTROL_F64
pie_gallery_start_position_angle_value_control = { 0.0, 360.0, 5.0 };

static const DIALOG_CONTROL_DATA_BUMP_F64
pie_gallery_start_position_angle_value_data = { { { { FRAMED_BOX_EDIT, 0, 1 /*right_text*/ } } /*EDIT_XX*/, &pie_gallery_start_position_angle_value_control } /*BUMP_XX*/ };

static const DIALOG_CONTROL
pie_gallery_start_position_angle_text =
{
    PIE_GALLERY_ID_START_POSITION_ANGLE_TEXT, PIE_GALLERY_ID_START_POSITION_GROUP,
    { PIE_GALLERY_ID_START_POSITION_ANGLE_VALUE, PIE_GALLERY_ID_START_POSITION_ANGLE_VALUE, DIALOG_CONTROL_SELF, PIE_GALLERY_ID_START_POSITION_ANGLE_VALUE },
    { DIALOG_SMALLSPACING_H, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(RTLB, STATICTEXT) }
};

static const DIALOG_CONTROL_DATA_STATICTEXT
pie_gallery_start_position_angle_text_data = { UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_PIE_GALLERY_START_POSITION_ANGLE), { 1 /*left_text*/, 0 /*centre_text*/, 1 /*windows_no_colon*/ } };

static const DIALOG_CONTROL
pie_gallery_anticlockwise =
{
    PIE_GALLERY_ID_ANTICLOCKWISE, PIE_GALLERY_ID_START_POSITION_GROUP,
    { PIE_GALLERY_ID_START_POSITION_ANGLE_VALUE, DIALOG_CONTROL_SELF, DIALOG_CONTROL_SELF, PIE_GALLERY_ID_START_POSITION_CIRCLE_180 },
    { 0, DIALOG_STDCHECK_V, DIALOG_CONTENTS_CALC, 0 },
    { DRT(LBLB, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_CHECKBOX
pie_gallery_anticlockwise_data = { { 0 }, UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_PIE_GALLERY_ANTICLOCKWISE) };

static const DIALOG_CONTROL
pie_gallery_arrange_group =
{
    BL_GALLERY_ID_ARRANGE_GROUP, DIALOG_MAIN_GROUP,
    { PIE_GALLERY_ID_LABEL_GROUP, PIE_GALLERY_ID_LABEL_GROUP, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { DIALOG_GROUPSPACING_H, 0, 0, 0 },
    { DRT(RTRB, GROUPBOX) }
};

static const DIALOG_CONTROL_DATA_RADIOBUTTON
pie_gallery_first_series_category_labels_data = { { 0 }, 1, UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_PIE_GALLERY_FIRST_SERIES_CATEGORY_LABELS) };

static const DIALOG_CONTROL_DATA_RADIOBUTTON
pie_gallery_first_category_series_label_data = { { 0 }, 1, UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_PIE_GALLERY_FIRST_CATEGORY_SERIES_LABEL) };

static const DIALOG_CONTROL
pie_gallery_ok =
{
    IDOK, DIALOG_CONTROL_WINDOW,
    { DIALOG_CONTROL_SELF, DIALOG_CONTROL_SELF, DIALOG_MAIN_GROUP, PIE_GALLERY_ID_START_POSITION_GROUP },
#if RISCOS
    { DIALOG_CONTENTS_CALC, DIALOG_DEFPUSHBUTTON_V, 0, 0 },
#else
    { DIALOG_DEFOK_H, DIALOG_DEFPUSHBUTTON_V, 0, 0 },
#endif
    { DRT(RBRB, PUSHBUTTON), 1 /*tabstop*/, 1 /*logical_group*/ }
};

static const DIALOG_CTL_CREATE
pie_gallery_ctl_create[] =
{
    { &dialog_main_group },

    { &pie_gallery_label_group, &pie_gallery_label_group_data },
    { &pie_gallery_label_none,  &pie_gallery_label_pict_data[0] },
    { &pie_gallery_label_label, &pie_gallery_label_pict_data[1] },
    { &pie_gallery_label_value, &pie_gallery_label_pict_data[2] },
    { &pie_gallery_label_v_pct, &pie_gallery_label_pict_data[3] },

    { &pie_gallery_explode_group,    &pie_gallery_explode_group_data },
    { &pie_gallery_explode_none,     &pie_gallery_explode_pict_data[0] },
    { &pie_gallery_explode_first,    &pie_gallery_explode_pict_data[1] },
    { &pie_gallery_explode_all,      &pie_gallery_explode_pict_data[2] },
    { &pie_gallery_explode_by_value, &pie_gallery_explode_by_value_data },
    { &pie_gallery_explode_by_text,  &pie_gallery_explode_by_text_data },

    { &pie_gallery_start_position_group,       &pie_gallery_start_position_group_data },
    { &pie_gallery_start_position_circle_000,  &pie_gallery_start_position_circle_000_data },
    { &pie_gallery_start_position_circle_045,  &pie_gallery_start_position_circle_045_data },
    { &pie_gallery_start_position_circle_090,  &pie_gallery_start_position_circle_090_data },
    { &pie_gallery_start_position_circle_135,  &pie_gallery_start_position_circle_135_data },
    { &pie_gallery_start_position_circle_180,  &pie_gallery_start_position_circle_180_data },
    { &pie_gallery_start_position_circle_225,  &pie_gallery_start_position_circle_225_data },
    { &pie_gallery_start_position_circle_270,  &pie_gallery_start_position_circle_270_data },
    { &pie_gallery_start_position_circle_315,  &pie_gallery_start_position_circle_315_data },

    { &pie_gallery_start_position_angle_value, &pie_gallery_start_position_angle_value_data },
    { &pie_gallery_start_position_angle_text,  &pie_gallery_start_position_angle_text_data },

    { &pie_gallery_anticlockwise, &pie_gallery_anticlockwise_data },

    { &pie_gallery_arrange_group, NULL },

    { &bl_gallery_series_group, NULL },
    { &bl_gallery_series_text, &bl_gallery_series_text_data },
    { &bl_gallery_series_cols, &bl_gallery_series_cols_data },
    { &bl_gallery_series_rows, &bl_gallery_series_rows_data },

    { &bl_gallery_first_series_group, NULL },
    { &bl_gallery_first_series_text, &bl_gallery_first_series_text_data },
    { &bl_gallery_first_series_category_labels, &pie_gallery_first_series_category_labels_data },
    { &bl_gallery_first_series_series_data, &bl_gallery_first_series_series_data_data },

    { &bl_gallery_first_category_group, NULL },
    { &bl_gallery_first_category_text, &bl_gallery_first_category_text_data },
    { &bl_gallery_first_category_series_labels, &pie_gallery_first_category_series_label_data },
    { &bl_gallery_first_category_category_data, &bl_gallery_first_category_category_data_data },

    { &pie_gallery_ok, &defbutton_ok_data },
    { &stdbutton_cancel, &stdbutton_cancel_data }
};

T5_MSG_PROTO(extern, chart_msg_chart_gallery, _InoutRef_ P_T5_MSG_CHART_GALLERY_DATA p_t5_msg_chart_gallery_data)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    switch(p_t5_msg_chart_gallery_data->chart_type)
    {
    case GR_CHART_TYPE_BAR:
        p_t5_msg_chart_gallery_data->p_selected_pict = &bar_gallery_selected_pict;
        p_t5_msg_chart_gallery_data->resource_id = CHART_MSG_DIALOG_BAR_GALLERY_CAPTION;
        p_t5_msg_chart_gallery_data->help_topic_resource_id = CHART_MSG_DIALOG_BAR_GALLERY_HELP_TOPIC;
        p_t5_msg_chart_gallery_data->n_ctls = elemof32(bar_gallery_ctl_create);
        p_t5_msg_chart_gallery_data->p_ctl_create  = bar_gallery_ctl_create;
        return(STATUS_OK);

    case GR_CHART_TYPE_LINE:
        p_t5_msg_chart_gallery_data->p_selected_pict = &line_gallery_selected_pict;
        p_t5_msg_chart_gallery_data->resource_id = CHART_MSG_DIALOG_LINE_GALLERY_CAPTION;
        p_t5_msg_chart_gallery_data->help_topic_resource_id = CHART_MSG_DIALOG_LINE_GALLERY_HELP_TOPIC;
        p_t5_msg_chart_gallery_data->n_ctls = elemof32(line_gallery_ctl_create);
        p_t5_msg_chart_gallery_data->p_ctl_create  = line_gallery_ctl_create;
        return(STATUS_OK);

    case GR_CHART_TYPE_OVER_BL:
        p_t5_msg_chart_gallery_data->p_selected_pict = &over_bl_gallery_selected_pict;
        p_t5_msg_chart_gallery_data->resource_id = CHART_MSG_DIALOG_OVER_BL_GALLERY_CAPTION;
        p_t5_msg_chart_gallery_data->help_topic_resource_id = CHART_MSG_DIALOG_OVER_BL_GALLERY_HELP_TOPIC;
        p_t5_msg_chart_gallery_data->n_ctls = elemof32(over_bl_gallery_ctl_create);
        p_t5_msg_chart_gallery_data->p_ctl_create  = over_bl_gallery_ctl_create;
        return(STATUS_OK);

    case GR_CHART_TYPE_PIE:
        p_t5_msg_chart_gallery_data->p_selected_pict = &pie_gallery_selected_label_pict;
        p_t5_msg_chart_gallery_data->resource_id = CHART_MSG_DIALOG_PIE_GALLERY_CAPTION;
        p_t5_msg_chart_gallery_data->help_topic_resource_id = CHART_MSG_DIALOG_PIE_GALLERY_HELP_TOPIC;
        p_t5_msg_chart_gallery_data->n_ctls = elemof32(pie_gallery_ctl_create);
        p_t5_msg_chart_gallery_data->p_ctl_create  = pie_gallery_ctl_create;
        return(STATUS_OK);

    case GR_CHART_TYPE_SCAT:
        p_t5_msg_chart_gallery_data->p_selected_pict = &scat_gallery_selected_pict;
        p_t5_msg_chart_gallery_data->resource_id = CHART_MSG_DIALOG_SCAT_GALLERY_CAPTION;
        p_t5_msg_chart_gallery_data->help_topic_resource_id = CHART_MSG_DIALOG_SCAT_GALLERY_HELP_TOPIC;
        p_t5_msg_chart_gallery_data->n_ctls = elemof32(scat_gallery_ctl_create);
        p_t5_msg_chart_gallery_data->p_ctl_create  = scat_gallery_ctl_create;
        return(STATUS_OK);

    default: default_unhandled();
        return(STATUS_FAIL);
    }
}

#if RISCOS
#define GEN_AXIS_TICKS_H (((28) * PIXITS_PER_RISCOS) + DIALOG_PUSHPICTUREOVH_H)
#define GEN_AXIS_TICKS_V (((28) * PIXITS_PER_RISCOS) + DIALOG_PUSHPICTUREOVH_V)
#else
#define GEN_AXIS_TICKS_H PIXITS_PER_STDTOOL_H
#define GEN_AXIS_TICKS_V PIXITS_PER_STDTOOL_V
#endif

static const DIALOG_CONTROL
gen_axis_position_group =
{
    GEN_AXIS_ID_POSITION_GROUP, DIALOG_MAIN_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { 0, 0, DIALOG_STDGROUP_RM, DIALOG_STDGROUP_BM },
    { DRT(LTRB, GROUPBOX) }
};

static const DIALOG_CONTROL_DATA_GROUPBOX
gen_axis_position_group_data = { UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_GEN_AXIS_POSITION), { FRAMED_BOX_GROUP } };

/*
left/zero/right (bottom/zero/top)
*/

static const DIALOG_CONTROL
gen_axis_position_lzr_group =
{
    GEN_AXIS_ID_POSITION_LZR_GROUP, GEN_AXIS_ID_POSITION_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { DIALOG_STDGROUP_LM, DIALOG_STDGROUP_TM, 0, 0 },
    { DRT(LTRB, GROUPBOX), 0, 1 /*logical_group*/ }
};

static const DIALOG_CONTROL
gen_axis_position_lzr_lb =
{
    GEN_AXIS_ID_POSITION_LZR_LB, GEN_AXIS_ID_POSITION_LZR_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { 0, 0, DIALOG_CONTENTS_CALC, DIALOG_STDRADIO_V },
    { DRT(LTLT, RADIOBUTTON), 1 /*tabstop*/ }
};

static /*poked*/ DIALOG_CONTROL_DATA_RADIOBUTTON
gen_axis_position_lzr_lb_data = { { 0 }, GR_AXIS_POSITION_LZR_LEFT, UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_GEN_AXIS_POSITION_LZR_L) };

static const DIALOG_CONTROL
gen_axis_position_lzr_zero =
{
    GEN_AXIS_ID_POSITION_LZR_ZERO, GEN_AXIS_ID_POSITION_LZR_GROUP,
    { GEN_AXIS_ID_POSITION_LZR_LB, GEN_AXIS_ID_POSITION_LZR_LB, DIALOG_CONTROL_SELF, GEN_AXIS_ID_POSITION_LZR_LB },
    { DIALOG_STDSPACING_H, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(RTLB, RADIOBUTTON) }
};

static const DIALOG_CONTROL_DATA_RADIOBUTTON
gen_axis_position_lzr_zero_data = { { 0 }, GR_AXIS_POSITION_LZR_ZERO, UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_GEN_AXIS_POSITION_LZR_ZERO) };

static const DIALOG_CONTROL
gen_axis_position_lzr_rt =
{
    GEN_AXIS_ID_POSITION_LZR_RT, GEN_AXIS_ID_POSITION_LZR_GROUP,
    { GEN_AXIS_ID_POSITION_LZR_ZERO, GEN_AXIS_ID_POSITION_LZR_ZERO, DIALOG_CONTROL_SELF, GEN_AXIS_ID_POSITION_LZR_ZERO },
    { DIALOG_STDSPACING_H, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(RTLB, RADIOBUTTON) }
};

static /*poked*/ DIALOG_CONTROL_DATA_RADIOBUTTON
gen_axis_position_lzr_rt_data = { { 0 }, GR_AXIS_POSITION_LZR_RIGHT, UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_GEN_AXIS_POSITION_LZR_R) };

/*
front/auto/rear
*/

static const DIALOG_CONTROL
gen_axis_position_arf_group =
{
    GEN_AXIS_ID_POSITION_ARF_GROUP, GEN_AXIS_ID_POSITION_GROUP,
    { GEN_AXIS_ID_POSITION_LZR_GROUP, GEN_AXIS_ID_POSITION_LZR_GROUP, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { 0, DIALOG_STDSPACING_V, 0, 0 },
    { DRT(LBRB, GROUPBOX), 0, 1 /*logical_group*/ }
};

static const DIALOG_CONTROL
gen_axis_position_arf_front =
{
    GEN_AXIS_ID_POSITION_ARF_FRONT, GEN_AXIS_ID_POSITION_ARF_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { 0, 0, DIALOG_CONTENTS_CALC, DIALOG_STDRADIO_V },
    { DRT(LTLT, RADIOBUTTON), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_RADIOBUTTON
gen_axis_position_arf_front_data = { { 0 }, GR_AXIS_POSITION_ARF_FRONT, UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_GEN_AXIS_POSITION_ARF_FRONT) };

static const DIALOG_CONTROL
gen_axis_position_arf_auto =
{
    GEN_AXIS_ID_POSITION_ARF_AUTO, GEN_AXIS_ID_POSITION_ARF_GROUP,
    { GEN_AXIS_ID_POSITION_ARF_FRONT, GEN_AXIS_ID_POSITION_ARF_FRONT, DIALOG_CONTROL_SELF, GEN_AXIS_ID_POSITION_ARF_FRONT },
    { DIALOG_STDSPACING_H, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(RTLB, RADIOBUTTON) }
};

static const DIALOG_CONTROL_DATA_RADIOBUTTON
gen_axis_position_arf_auto_data = { { 0 }, GR_AXIS_POSITION_ARF_AUTO, UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_GEN_AXIS_POSITION_ARF_AUTO) };

static const DIALOG_CONTROL
gen_axis_position_arf_rear =
{
    GEN_AXIS_ID_POSITION_ARF_REAR, GEN_AXIS_ID_POSITION_ARF_GROUP,
    { GEN_AXIS_ID_POSITION_ARF_AUTO, GEN_AXIS_ID_POSITION_ARF_AUTO, DIALOG_CONTROL_SELF, GEN_AXIS_ID_POSITION_ARF_AUTO },
    { DIALOG_STDSPACING_H, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(RTLB, RADIOBUTTON) }
};

static const DIALOG_CONTROL_DATA_RADIOBUTTON
gen_axis_position_arf_rear_data = { { 0 }, GR_AXIS_POSITION_ARF_REAR,  UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_GEN_AXIS_POSITION_ARF_REAR) };

/*
major group
*/

static const DIALOG_CONTROL
gen_axis_major_group =
{
    GEN_AXIS_ID_MAJOR_GROUP, DIALOG_MAIN_GROUP,
    { GEN_AXIS_ID_POSITION_GROUP, GEN_AXIS_ID_POSITION_GROUP, GEN_AXIS_ID_POSITION_GROUP /*GEN_AXIS_ID_MAJOR_SPACING*/, DIALOG_CONTROL_CONTENTS },
    { 0, DIALOG_STDSPACING_V, 0 /*DIALOG_STDGROUP_RM*/, DIALOG_STDGROUP_BM },
    { DRT(LBRB, GROUPBOX) }
};

static const DIALOG_CONTROL_DATA_GROUPBOX
gen_axis_major_group_data = { UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_GEN_AXIS_MAJOR), { FRAMED_BOX_GROUP } };

static const DIALOG_CONTROL
gen_axis_major_auto =
{
    GEN_AXIS_ID_MAJOR_AUTO, GEN_AXIS_ID_MAJOR_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { DIALOG_STDGROUP_LM, DIALOG_STDGROUP_TM, DIALOG_CONTENTS_CALC, DIALOG_STDCHECK_V },
    { DRT(LTLT, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_CHECKBOX
gen_axis_major_auto_data = { { 0 }, UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_GEN_AXIS_MAJOR_AUTO) };

static const DIALOG_CONTROL
gen_axis_major_spacing_text =
{
    GEN_AXIS_ID_MAJOR_SPACING_TEXT, GEN_AXIS_ID_MAJOR_GROUP,
    { GEN_AXIS_ID_MAJOR_AUTO, GEN_AXIS_ID_MAJOR_SPACING, DIALOG_CONTROL_SELF, GEN_AXIS_ID_MAJOR_SPACING },
    { 0, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(LTLB, STATICTEXT) }
};

static const DIALOG_CONTROL_DATA_STATICTEXT
gen_axis_major_spacing_text_data = { UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_GEN_AXIS_MAJOR_SPACING), { 1 /*left_text*/ } };

static const DIALOG_CONTROL
val_axis_major_spacing =
{
    GEN_AXIS_ID_MAJOR_SPACING, GEN_AXIS_ID_MAJOR_GROUP,
    { GEN_AXIS_ID_MAJOR_AUTO, GEN_AXIS_ID_MAJOR_AUTO },
    { -DIALOG_STDCHECK_H, DIALOG_STDSPACING_V, DIALOG_BUMP_H(6), DIALOG_STDBUMP_V },
    { DRT(RBLT, BUMP_F64), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL
cat_axis_major_spacing =
{
    GEN_AXIS_ID_MAJOR_SPACING, GEN_AXIS_ID_MAJOR_GROUP,
    { GEN_AXIS_ID_MAJOR_AUTO, GEN_AXIS_ID_MAJOR_AUTO },
    { -DIALOG_STDCHECK_H, DIALOG_STDSPACING_V, DIALOG_BUMP_H(6), DIALOG_STDBUMP_V },
    { DRT(RBLT, BUMP_S32), 1 /*tabstop*/ }
};

static /*poked*/ UI_CONTROL_S32
cat_axis_major_control = { 1, S32_MAX };

static const DIALOG_CONTROL_DATA_BUMP_S32
cat_axis_major_spacing_data = { { { { FRAMED_BOX_EDIT, 0, 1 /*right_text*/ } } /*EDIT_XX*/, &cat_axis_major_control } /*BUMP_XX*/ };

static /*poked*/ DIALOG_CONTROL_DATA_BUMP_F64
val_axis_major_spacing_data = { { { { FRAMED_BOX_EDIT, 0, 1 /*right_text*/ } } /*EDIT_XX*/ /*, &val_axis_major_control*//*poked*/ } /*BUMP_XX*/ };

static const DIALOG_CONTROL
gen_axis_major_grid =
{
    GEN_AXIS_ID_MAJOR_GRID, GEN_AXIS_ID_MAJOR_GROUP,
    { GEN_AXIS_ID_MAJOR_SPACING_TEXT, GEN_AXIS_ID_MAJOR_SPACING },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDCHECK_V },
    { DRT(LBLT, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_CHECKBOX
gen_axis_major_grid_data = { { 0 }, UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_GEN_AXIS_MAJOR_GRID) };

static const DIALOG_CONTROL
gen_axis_major_ticks_text =
{
    GEN_AXIS_ID_MAJOR_TICKS_TEXT, GEN_AXIS_ID_MAJOR_GROUP,
    { GEN_AXIS_ID_MAJOR_SPACING_TEXT, GEN_AXIS_ID_MAJOR_TICKS_NONE, DIALOG_CONTROL_SELF, GEN_AXIS_ID_MAJOR_TICKS_NONE },
    { 0, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(LTLB, STATICTEXT) }
};

static const DIALOG_CONTROL_DATA_STATICTEXT
gen_axis_major_ticks_text_data = { UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_GEN_AXIS_MAJOR_TICKS), { 1 /*left_text*/ } };

static const DIALOG_CONTROL
gen_axis_major_ticks_none =
{
    GEN_AXIS_ID_MAJOR_TICKS_NONE, GEN_AXIS_ID_MAJOR_GROUP,
    { GEN_AXIS_ID_MAJOR_SPACING, GEN_AXIS_ID_MAJOR_GRID },
    { 0, DIALOG_STDSPACING_V, GEN_AXIS_TICKS_H, GEN_AXIS_TICKS_V },
    { DRT(LBLT, RADIOPICTURE), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL
gen_axis_major_ticks_full =
{
    GEN_AXIS_ID_MAJOR_TICKS_FULL, GEN_AXIS_ID_MAJOR_GROUP,
    { GEN_AXIS_ID_MAJOR_TICKS_NONE, GEN_AXIS_ID_MAJOR_TICKS_NONE, DIALOG_CONTROL_SELF, GEN_AXIS_ID_MAJOR_TICKS_NONE },
    { 0, 0, GEN_AXIS_TICKS_H, 0 },
    { DRT(RTLB, RADIOPICTURE) }
};

static const DIALOG_CONTROL
gen_axis_major_ticks_in =
{
    GEN_AXIS_ID_MAJOR_TICKS_IN, GEN_AXIS_ID_MAJOR_GROUP,
    { GEN_AXIS_ID_MAJOR_TICKS_FULL, GEN_AXIS_ID_MAJOR_TICKS_FULL, DIALOG_CONTROL_SELF, GEN_AXIS_ID_MAJOR_TICKS_FULL },
    { 0, 0, GEN_AXIS_TICKS_H, 0 },
    { DRT(RTLB, RADIOPICTURE) }
};

static const DIALOG_CONTROL
gen_axis_major_ticks_out =
{
    GEN_AXIS_ID_MAJOR_TICKS_OUT, GEN_AXIS_ID_MAJOR_GROUP,
    { GEN_AXIS_ID_MAJOR_TICKS_IN, GEN_AXIS_ID_MAJOR_TICKS_IN, DIALOG_CONTROL_SELF, GEN_AXIS_ID_MAJOR_TICKS_IN },
    { 0, 0, GEN_AXIS_TICKS_H, 0 },
    { DRT(RTLB, RADIOPICTURE) }
};

/*
minor group
*/

static const DIALOG_CONTROL
gen_axis_minor_group =
{
    GEN_AXIS_ID_MINOR_GROUP, DIALOG_MAIN_GROUP,

    { GEN_AXIS_ID_MAJOR_GROUP, GEN_AXIS_ID_MAJOR_GROUP, GEN_AXIS_ID_MAJOR_GROUP, DIALOG_CONTROL_CONTENTS },

    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDGROUP_BM },

    { DRT(LBRB, GROUPBOX) }
};

static const DIALOG_CONTROL_DATA_GROUPBOX
gen_axis_minor_group_data = { UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_GEN_AXIS_MINOR), { FRAMED_BOX_GROUP } };

static const DIALOG_CONTROL
gen_axis_minor_auto =
{
    GEN_AXIS_ID_MINOR_AUTO, GEN_AXIS_ID_MINOR_GROUP,
    { GEN_AXIS_ID_MAJOR_AUTO, DIALOG_CONTROL_PARENT, GEN_AXIS_ID_MAJOR_AUTO },
    { 0, DIALOG_STDGROUP_TM, 0, DIALOG_STDCHECK_V },
    { DRT(LTRT, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL
gen_axis_minor_spacing_text =
{
    GEN_AXIS_ID_MINOR_SPACING_TEXT, GEN_AXIS_ID_MINOR_GROUP,
    { GEN_AXIS_ID_MAJOR_SPACING_TEXT, GEN_AXIS_ID_MINOR_SPACING, GEN_AXIS_ID_MAJOR_SPACING_TEXT, GEN_AXIS_ID_MINOR_SPACING },
    { 0 },
    { DRT(LTRB, STATICTEXT) }
};

static const DIALOG_CONTROL
val_axis_minor_spacing =
{
    GEN_AXIS_ID_MINOR_SPACING, GEN_AXIS_ID_MINOR_GROUP,
    { GEN_AXIS_ID_MAJOR_SPACING, GEN_AXIS_ID_MINOR_AUTO, GEN_AXIS_ID_MAJOR_SPACING },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDBUMP_V },
    { DRT(LBRT, BUMP_F64), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL
cat_axis_minor_spacing =
{
    GEN_AXIS_ID_MINOR_SPACING, GEN_AXIS_ID_MINOR_GROUP,
    { GEN_AXIS_ID_MAJOR_SPACING, GEN_AXIS_ID_MINOR_AUTO, GEN_AXIS_ID_MAJOR_SPACING },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDBUMP_V },
    { DRT(LBRT, BUMP_S32), 1 /*tabstop*/ }
};

static /*poked*/ UI_CONTROL_S32
cat_axis_minor_control = { 1, S32_MAX };

static const DIALOG_CONTROL_DATA_BUMP_S32
cat_axis_minor_spacing_data = { { { { FRAMED_BOX_EDIT, 0, 1 /*right_text*/ } } /*EDIT_XX*/, &cat_axis_minor_control } /*BUMP_XX*/ };

static /*poked*/ DIALOG_CONTROL_DATA_BUMP_F64
val_axis_minor_spacing_data = { { { { FRAMED_BOX_EDIT, 0, 1 /*right_text*/ } } /*EDIT_XX*/ /*, &val_axis_minor_control*//*poked*/ } /*BUMP_XX*/ };

static const DIALOG_CONTROL
gen_axis_minor_grid =
{
    GEN_AXIS_ID_MINOR_GRID, GEN_AXIS_ID_MINOR_GROUP,
    { GEN_AXIS_ID_MINOR_SPACING_TEXT, GEN_AXIS_ID_MINOR_SPACING },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDCHECK_V },
    { DRT(LBLT, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL
gen_axis_minor_ticks_text =
{
    GEN_AXIS_ID_MINOR_TICKS_TEXT, GEN_AXIS_ID_MINOR_GROUP,
    { GEN_AXIS_ID_MINOR_SPACING_TEXT, GEN_AXIS_ID_MINOR_TICKS_NONE, GEN_AXIS_ID_MINOR_SPACING_TEXT, GEN_AXIS_ID_MINOR_TICKS_NONE },
    { 0 },
    { DRT(LTRB, STATICTEXT) }
};

static const DIALOG_CONTROL
gen_axis_minor_ticks_none =
{
    GEN_AXIS_ID_MINOR_TICKS_NONE, GEN_AXIS_ID_MINOR_GROUP,
    { GEN_AXIS_ID_MINOR_SPACING, GEN_AXIS_ID_MINOR_GRID },
    { 0, DIALOG_STDSPACING_V, GEN_AXIS_TICKS_H, GEN_AXIS_TICKS_V },
    { DRT(LBLT, RADIOPICTURE), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL
gen_axis_minor_ticks_full =
{
    GEN_AXIS_ID_MINOR_TICKS_FULL, GEN_AXIS_ID_MINOR_GROUP,
    { GEN_AXIS_ID_MINOR_TICKS_NONE, GEN_AXIS_ID_MINOR_TICKS_NONE, DIALOG_CONTROL_SELF, GEN_AXIS_ID_MINOR_TICKS_NONE },
    { 0, 0, GEN_AXIS_TICKS_H, 0 },
    { DRT(RTLB, RADIOPICTURE) }
};

static const DIALOG_CONTROL
gen_axis_minor_ticks_in =
{
    GEN_AXIS_ID_MINOR_TICKS_IN, GEN_AXIS_ID_MINOR_GROUP,
    { GEN_AXIS_ID_MINOR_TICKS_FULL, GEN_AXIS_ID_MINOR_TICKS_FULL, DIALOG_CONTROL_SELF, GEN_AXIS_ID_MINOR_TICKS_FULL },
    { 0, 0, GEN_AXIS_TICKS_H, 0 },
    { DRT(RTLB, RADIOPICTURE) }
};

static const DIALOG_CONTROL
gen_axis_minor_ticks_out =
{
    GEN_AXIS_ID_MINOR_TICKS_OUT, GEN_AXIS_ID_MINOR_GROUP,
    { GEN_AXIS_ID_MINOR_TICKS_IN, GEN_AXIS_ID_MINOR_TICKS_IN, DIALOG_CONTROL_SELF, GEN_AXIS_ID_MINOR_TICKS_IN },
    { 0, 0, GEN_AXIS_TICKS_H, 0 },
    { DRT(RTLB, RADIOPICTURE) }
};

static const RESOURCE_BITMAP_ID
gen_axis_ticks_none_bitmap[2] = { { OBJECT_ID_CHART, CHART_ID_BM_TK_NH }, { OBJECT_ID_CHART, CHART_ID_BM_TK_NV } };

static /*poked*/ DIALOG_CONTROL_DATA_RADIOPICTURE
gen_axis_ticks_none_data = { { 0 }, GR_AXIS_TICK_POSITION_NONE };

static const RESOURCE_BITMAP_ID
gen_axis_ticks_full_bitmap[2] = { { OBJECT_ID_CHART, CHART_ID_BM_TK_FH }, { OBJECT_ID_CHART, CHART_ID_BM_TK_FV } };

static /*poked*/ DIALOG_CONTROL_DATA_RADIOPICTURE
gen_axis_ticks_full_data = { { 0 }, GR_AXIS_TICK_POSITION_FULL };

static const RESOURCE_BITMAP_ID
gen_axis_ticks_in_bitmap[2] = { { OBJECT_ID_CHART, CHART_ID_BM_TK_IH }, { OBJECT_ID_CHART, CHART_ID_BM_TK_IV } };

static /*poked*/ DIALOG_CONTROL_DATA_RADIOPICTURE
gen_axis_ticks_in_data = { { 0 }, GR_AXIS_TICK_POSITION_HALF_RIGHT /* == BOTTOM */ };

static const RESOURCE_BITMAP_ID
gen_axis_ticks_out_bitmap[2] = { { OBJECT_ID_CHART, CHART_ID_BM_TK_OH }, { OBJECT_ID_CHART, CHART_ID_BM_TK_OV } };

static /*poked*/ DIALOG_CONTROL_DATA_RADIOPICTURE
gen_axis_ticks_out_data = { { 0 }, GR_AXIS_TICK_POSITION_HALF_LEFT  /* == TOP */ };

/*
cat axis specifics
*/

static const DIALOG_CONTROL
cat_axis_ok =
{
    IDOK, DIALOG_CONTROL_WINDOW,
    { DIALOG_CONTROL_SELF, GEN_AXIS_ID_MINOR_GROUP, GEN_AXIS_ID_MINOR_GROUP, DIALOG_CONTROL_SELF },
    { DIALOG_CONTENTS_CALC, DIALOG_STDSPACING_V, 0, DIALOG_DEFPUSHBUTTON_V },
    { DRT(RBRT, PUSHBUTTON), 1 /*tabstop*/, 1 /*logical_group*/ }
};

static const DIALOG_CTL_CREATE
cat_axis_ctl_create[] =
{
    { &dialog_main_group },

    { &gen_axis_position_group,     &gen_axis_position_group_data },
    { &gen_axis_position_lzr_group, NULL /*&gen_axis_position_lzr_group_data*/ },
    { &gen_axis_position_lzr_lb,    &gen_axis_position_lzr_lb_data },
    { &gen_axis_position_lzr_zero,  &gen_axis_position_lzr_zero_data },
    { &gen_axis_position_lzr_rt,    &gen_axis_position_lzr_rt_data },
    { &gen_axis_position_arf_group, NULL /*&gen_axis_position_arf_group_data*/ },
    { &gen_axis_position_arf_front, &gen_axis_position_arf_front_data },
    { &gen_axis_position_arf_auto,  &gen_axis_position_arf_auto_data },
    { &gen_axis_position_arf_rear,  &gen_axis_position_arf_rear_data },

    { &gen_axis_major_group,        &gen_axis_major_group_data },
    { &gen_axis_major_auto,         &gen_axis_major_auto_data },
    { &gen_axis_major_spacing_text, &gen_axis_major_spacing_text_data },
    { &cat_axis_major_spacing,      &cat_axis_major_spacing_data },
    { &gen_axis_major_grid,         &gen_axis_major_grid_data },
    { &gen_axis_major_ticks_text,   &gen_axis_major_ticks_text_data },
    { &gen_axis_major_ticks_none,   &gen_axis_ticks_none_data },
    { &gen_axis_major_ticks_full,   &gen_axis_ticks_full_data },
    { &gen_axis_major_ticks_in,     &gen_axis_ticks_in_data },
    { &gen_axis_major_ticks_out,    &gen_axis_ticks_out_data },

    { &gen_axis_minor_group,        &gen_axis_minor_group_data },
    { &gen_axis_minor_auto,         &gen_axis_major_auto_data },
    { &gen_axis_minor_spacing_text, &gen_axis_major_spacing_text_data },
    { &cat_axis_minor_spacing,      &cat_axis_minor_spacing_data },
    { &gen_axis_minor_grid,         &gen_axis_major_grid_data },
    { &gen_axis_minor_ticks_text,   &gen_axis_major_ticks_text_data },
    { &gen_axis_minor_ticks_none,   &gen_axis_ticks_none_data },
    { &gen_axis_minor_ticks_full,   &gen_axis_ticks_full_data },
    { &gen_axis_minor_ticks_in,     &gen_axis_ticks_in_data },
    { &gen_axis_minor_ticks_out,    &gen_axis_ticks_out_data },

    { &cat_axis_ok, &defbutton_ok_persist_data },
    { &stdbutton_cancel, &stdbutton_cancel_data }
};

/*
val axis specifics
*/

static const DIALOG_CONTROL
val_axis_scaling_group =
{
    VAL_AXIS_ID_SCALING_GROUP, DIALOG_MAIN_GROUP,
    { GEN_AXIS_ID_POSITION_GROUP, GEN_AXIS_ID_POSITION_GROUP, DIALOG_CONTROL_CONTENTS, DIALOG_CONTROL_CONTENTS },
    { DIALOG_STDSPACING_H, 0, DIALOG_STDGROUP_RM, DIALOG_STDGROUP_BM },
    { DRT(RTRB, GROUPBOX) }
};

static const DIALOG_CONTROL_DATA_GROUPBOX
val_axis_scaling_group_data = { UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_VAL_AXIS_SCALING), { FRAMED_BOX_GROUP } };

static const DIALOG_CONTROL
val_axis_scaling_auto =
{
    VAL_AXIS_ID_SCALING_AUTO, VAL_AXIS_ID_SCALING_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { DIALOG_STDGROUP_LM, DIALOG_STDGROUP_TM, DIALOG_CONTENTS_CALC, DIALOG_STDCHECK_V },
    { DRT(LTLT, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_CHECKBOX
val_axis_scaling_auto_data = { { 0 }, UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_VAL_AXIS_SCALING_AUTO) };

static const DIALOG_CONTROL
val_axis_scaling_minimum_text =
{
    VAL_AXIS_ID_SCALING_MINIMUM_TEXT, VAL_AXIS_ID_SCALING_GROUP,
    { VAL_AXIS_ID_SCALING_AUTO, VAL_AXIS_ID_SCALING_MINIMUM, DIALOG_CONTROL_SELF, VAL_AXIS_ID_SCALING_MINIMUM },
    { 0, 0, DIALOG_CONTENTS_CALC, 0 },
    { DRT(LTLB, STATICTEXT) }
};

static const DIALOG_CONTROL_DATA_STATICTEXT
val_axis_scaling_minimum_text_data = { UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_VAL_AXIS_SCALING_MINIMUM), { 1 /*left_text*/ } };

static const DIALOG_CONTROL
val_axis_scaling_minimum =
{
    VAL_AXIS_ID_SCALING_MINIMUM, VAL_AXIS_ID_SCALING_GROUP,
    { VAL_AXIS_ID_SCALING_MINIMUM_TEXT, VAL_AXIS_ID_SCALING_AUTO },
    { DIALOG_STDSPACING_H, DIALOG_STDSPACING_V, DIALOG_BUMP_H(6), DIALOG_STDBUMP_V },
    { DRT(RBLT, BUMP_F64), 1 /*tabstop*/ }
};

static /*poked*/ UI_CONTROL_F64
val_axis_scaling_minimum_control = { -DBL_MAX, +DBL_MAX };

static const DIALOG_CONTROL_DATA_BUMP_F64
val_axis_scaling_minimum_data = { { { { FRAMED_BOX_EDIT, 0, 1 /*right_text*/ } } /*EDIT_XX*/, &val_axis_scaling_minimum_control } /*BUMP_XX*/ };

static const DIALOG_CONTROL
val_axis_scaling_maximum_text =
{
    VAL_AXIS_ID_SCALING_MAXIMUM_TEXT, VAL_AXIS_ID_SCALING_GROUP,
    { VAL_AXIS_ID_SCALING_MINIMUM_TEXT, VAL_AXIS_ID_SCALING_MAXIMUM, VAL_AXIS_ID_SCALING_MINIMUM_TEXT, VAL_AXIS_ID_SCALING_MAXIMUM },
    { 0 },
    { DRT(LTRB, STATICTEXT) }
};

static const DIALOG_CONTROL_DATA_STATICTEXT
val_axis_scaling_maximum_text_data = { UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_VAL_AXIS_SCALING_MAXIMUM), { 1 /*left_text*/ } };

static const DIALOG_CONTROL
val_axis_scaling_maximum =
{
    VAL_AXIS_ID_SCALING_MAXIMUM, VAL_AXIS_ID_SCALING_GROUP,
    { VAL_AXIS_ID_SCALING_MINIMUM, VAL_AXIS_ID_SCALING_MINIMUM, VAL_AXIS_ID_SCALING_MINIMUM },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDBUMP_V },
    { DRT(LBRT, BUMP_F64), 1 /*tabstop*/ }
};

static /*poked*/ UI_CONTROL_F64
val_axis_scaling_maximum_control = { -DBL_MAX, +DBL_MAX };

static const DIALOG_CONTROL_DATA_BUMP_F64
val_axis_scaling_maximum_data = { { { { FRAMED_BOX_EDIT, 0, 1 /*right_text*/ } } /*EDIT_XX*/, &val_axis_scaling_maximum_control } /*BUMP_XX*/ };

static const DIALOG_CONTROL
val_axis_scaling_include_zero =
{
    VAL_AXIS_ID_SCALING_INCLUDE_ZERO, VAL_AXIS_ID_SCALING_GROUP,
    { VAL_AXIS_ID_SCALING_AUTO, VAL_AXIS_ID_SCALING_MAXIMUM },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDCHECK_V },
    { DRT(LBLT, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_CHECKBOX
val_axis_scaling_include_zero_data = { { 0 }, UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_VAL_AXIS_SCALING_INCLUDE_ZERO) };

static const DIALOG_CONTROL
val_axis_scaling_logarithmic =
{
    VAL_AXIS_ID_SCALING_LOGARITHMIC, VAL_AXIS_ID_SCALING_GROUP,
    { VAL_AXIS_ID_SCALING_INCLUDE_ZERO, VAL_AXIS_ID_SCALING_INCLUDE_ZERO },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDCHECK_V },
    { DRT(LBLT, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_CHECKBOX
val_axis_scaling_logarithmic_data = { { 0 }, UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_VAL_AXIS_SCALING_LOGARITHMIC) };

static const DIALOG_CONTROL
val_axis_scaling_log_labels =
{
    VAL_AXIS_ID_SCALING_LOG_LABELS, VAL_AXIS_ID_SCALING_GROUP,
    { VAL_AXIS_ID_SCALING_LOGARITHMIC, VAL_AXIS_ID_SCALING_LOGARITHMIC },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDCHECK_V },
    { DRT(LBLT, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_CHECKBOX
val_axis_scaling_log_labels_data = { { 0 }, UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_VAL_AXIS_SCALING_LOG_LABELS) };

/*
series defaults group
*/

static const DIALOG_CONTROL
val_axis_series_group =
{
    VAL_AXIS_ID_SERIES_GROUP, DIALOG_MAIN_GROUP,
    { VAL_AXIS_ID_SCALING_GROUP, VAL_AXIS_ID_SCALING_GROUP, VAL_AXIS_ID_SCALING_GROUP, DIALOG_CONTROL_CONTENTS },
    { 0, DIALOG_STDSPACING_V, 0, DIALOG_STDGROUP_BM },
    { DRT(LBRB, GROUPBOX) }
};

static const DIALOG_CONTROL_DATA_GROUPBOX
val_axis_series_group_data = { UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_VAL_AXIS_SERIES), { FRAMED_BOX_GROUP } };

static const DIALOG_CONTROL
val_axis_series_cumulative =
{
    VAL_AXIS_ID_SERIES_CUMULATIVE, VAL_AXIS_ID_SERIES_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { DIALOG_STDGROUP_LM, DIALOG_STDGROUP_TM, DIALOG_CONTENTS_CALC, DIALOG_STDCHECK_V },
    { DRT(LTLT, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_CHECKBOX
val_axis_series_cumulative_data = { { 0 }, UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_VAL_AXIS_SERIES_CUMULATIVE) };

static const DIALOG_CONTROL
val_axis_series_vary_by_point =
{
    VAL_AXIS_ID_SERIES_VARY_BY_POINT, VAL_AXIS_ID_SERIES_GROUP,
    { VAL_AXIS_ID_SERIES_CUMULATIVE, VAL_AXIS_ID_SERIES_CUMULATIVE },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDCHECK_V },
    { DRT(LBLT, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_CHECKBOX
val_axis_series_vary_by_point_data = { { 0 }, UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_VAL_AXIS_SERIES_VARY_BY_POINT) };

static const DIALOG_CONTROL
val_axis_series_best_fit =
{
    VAL_AXIS_ID_SERIES_BEST_FIT, VAL_AXIS_ID_SERIES_GROUP,
    { VAL_AXIS_ID_SERIES_VARY_BY_POINT, VAL_AXIS_ID_SERIES_VARY_BY_POINT },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDCHECK_V },
    { DRT(LBLT, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_CHECKBOX
val_axis_series_best_fit_data = { { 0 }, UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_VAL_AXIS_SERIES_BEST_FIT) };

static const DIALOG_CONTROL
val_axis_series_fill_to_axis =
{
    VAL_AXIS_ID_SERIES_FILL_TO_AXIS, VAL_AXIS_ID_SERIES_GROUP,
    { VAL_AXIS_ID_SERIES_BEST_FIT, VAL_AXIS_ID_SERIES_BEST_FIT },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDCHECK_V },
    { DRT(LBLT, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_CHECKBOX
val_axis_series_fill_to_axis_data = { { 0 }, UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_VAL_AXIS_SERIES_FILL_TO_AXIS) };

static const DIALOG_CONTROL
val_axis_series_stack =
{
    VAL_AXIS_ID_SERIES_STACK, DIALOG_MAIN_GROUP,
    { VAL_AXIS_ID_SERIES_FILL_TO_AXIS, VAL_AXIS_ID_SERIES_GROUP },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDCHECK_V },
    { DRT(LBLT, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_CHECKBOX
val_axis_series_stack_data = { { 0 }, UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_VAL_AXIS_SERIES_STACK) };

static const DIALOG_CONTROL
val_axis_ok =
{
    IDOK, DIALOG_CONTROL_WINDOW,
    { DIALOG_CONTROL_SELF, VAL_AXIS_ID_SERIES_STACK, DIALOG_MAIN_GROUP, DIALOG_CONTROL_SELF },
    { DIALOG_CONTENTS_CALC, 0, 0, DIALOG_DEFPUSHBUTTON_V },
    { DRT(RBRT, PUSHBUTTON), 1 /*tabstop*/, 1 /*logical_group*/ }
};

static const DIALOG_CTL_CREATE
val_axis_ctl_create[] =
{
    { &dialog_main_group },

    { &gen_axis_position_group,     &gen_axis_position_group_data },
    { &gen_axis_position_lzr_group, NULL /*&gen_axis_position_lzr_group_data*/ },
    { &gen_axis_position_lzr_lb,    &gen_axis_position_lzr_lb_data },
    { &gen_axis_position_lzr_zero,  &gen_axis_position_lzr_zero_data },
    { &gen_axis_position_lzr_rt,    &gen_axis_position_lzr_rt_data },
    { &gen_axis_position_arf_group, NULL /*&gen_axis_position_arf_group_data*/ },
    { &gen_axis_position_arf_front, &gen_axis_position_arf_front_data },
    { &gen_axis_position_arf_auto,  &gen_axis_position_arf_auto_data },
    { &gen_axis_position_arf_rear,  &gen_axis_position_arf_rear_data },

    { &gen_axis_major_group,        &gen_axis_major_group_data },
    { &gen_axis_major_auto,         &gen_axis_major_auto_data },
    { &gen_axis_major_spacing_text, &gen_axis_major_spacing_text_data },
    { &val_axis_major_spacing,      &val_axis_major_spacing_data },
    { &gen_axis_major_grid,         &gen_axis_major_grid_data },
    { &gen_axis_major_ticks_text,   &gen_axis_major_ticks_text_data },
    { &gen_axis_major_ticks_none,   &gen_axis_ticks_none_data },
    { &gen_axis_major_ticks_full,   &gen_axis_ticks_full_data },
    { &gen_axis_major_ticks_in,     &gen_axis_ticks_in_data },
    { &gen_axis_major_ticks_out,    &gen_axis_ticks_out_data },

    { &gen_axis_minor_group,        &gen_axis_minor_group_data },
    { &gen_axis_minor_auto,         &gen_axis_major_auto_data },
    { &gen_axis_minor_spacing_text, &gen_axis_major_spacing_text_data },
    { &val_axis_minor_spacing,      &val_axis_minor_spacing_data },
    { &gen_axis_minor_grid,         &gen_axis_major_grid_data },
    { &gen_axis_minor_ticks_text,   &gen_axis_major_ticks_text_data },
    { &gen_axis_minor_ticks_none,   &gen_axis_ticks_none_data },
    { &gen_axis_minor_ticks_full,   &gen_axis_ticks_full_data },
    { &gen_axis_minor_ticks_in,     &gen_axis_ticks_in_data },
    { &gen_axis_minor_ticks_out,    &gen_axis_ticks_out_data },

    { &val_axis_scaling_group,        &val_axis_scaling_group_data },
    { &val_axis_scaling_auto,         &val_axis_scaling_auto_data },
    { &val_axis_scaling_minimum_text, &val_axis_scaling_minimum_text_data },
    { &val_axis_scaling_minimum,      &val_axis_scaling_minimum_data },
    { &val_axis_scaling_maximum_text, &val_axis_scaling_maximum_text_data },
    { &val_axis_scaling_maximum,      &val_axis_scaling_maximum_data },
    { &val_axis_scaling_include_zero, &val_axis_scaling_include_zero_data },
    { &val_axis_scaling_logarithmic,  &val_axis_scaling_logarithmic_data },
    { &val_axis_scaling_log_labels,   &val_axis_scaling_log_labels_data },

    { &val_axis_series_group,         &val_axis_series_group_data },
    { &val_axis_series_cumulative,    &val_axis_series_cumulative_data },
    { &val_axis_series_vary_by_point, &val_axis_series_vary_by_point_data },
    { &val_axis_series_best_fit,      &val_axis_series_best_fit_data },
    { &val_axis_series_fill_to_axis,  &val_axis_series_fill_to_axis_data },

    { &val_axis_series_stack,         &val_axis_series_stack_data },

    { &val_axis_ok, &defbutton_ok_persist_data },
    { &stdbutton_cancel, &stdbutton_cancel_data }
};

static const DIALOG_CONTROL
series_series_cumulative =
{
    VAL_AXIS_ID_SERIES_CUMULATIVE, DIALOG_MAIN_GROUP,
    { DIALOG_CONTROL_PARENT, DIALOG_CONTROL_PARENT },
    { DIALOG_STDGROUP_LM, DIALOG_STDGROUP_TM, DIALOG_CONTENTS_CALC, DIALOG_STDCHECK_V },
    { DRT(LTLT, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL
series_vary_by_point =
{
    VAL_AXIS_ID_SERIES_VARY_BY_POINT, DIALOG_MAIN_GROUP,
    { VAL_AXIS_ID_SERIES_CUMULATIVE, VAL_AXIS_ID_SERIES_CUMULATIVE },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDCHECK_V },
    { DRT(LBLT, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL
series_best_fit =
{
    VAL_AXIS_ID_SERIES_BEST_FIT, DIALOG_MAIN_GROUP,
    { VAL_AXIS_ID_SERIES_VARY_BY_POINT, VAL_AXIS_ID_SERIES_VARY_BY_POINT },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDCHECK_V },
    { DRT(LBLT, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL
series_fill_to_axis =
{
    VAL_AXIS_ID_SERIES_FILL_TO_AXIS, DIALOG_MAIN_GROUP,
    { VAL_AXIS_ID_SERIES_BEST_FIT, VAL_AXIS_ID_SERIES_BEST_FIT },
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDCHECK_V },
    { DRT(LBLT, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL
series_in_overlay =
{
    SERIES_ID_IN_OVERLAY, DIALOG_MAIN_GROUP,
    { VAL_AXIS_ID_SERIES_FILL_TO_AXIS, VAL_AXIS_ID_SERIES_FILL_TO_AXIS},
    { 0, DIALOG_STDSPACING_V, DIALOG_CONTENTS_CALC, DIALOG_STDCHECK_V },
    { DRT(LBLT, CHECKBOX), 1 /*tabstop*/ }
};

static const DIALOG_CONTROL_DATA_CHECKBOX
series_in_overlay_data = { { 0 }, UI_TEXT_INIT_RESID(CHART_MSG_DIALOG_SERIES_IN_OVERLAY) };

static const DIALOG_CTL_CREATE
series_ctl_create[] =
{
    { &dialog_main_group },

    { &series_series_cumulative, &val_axis_series_cumulative_data },
    { &series_vary_by_point,     &val_axis_series_vary_by_point_data },
    { &series_best_fit,          &val_axis_series_best_fit_data },
    { &series_fill_to_axis,      &val_axis_series_fill_to_axis_data },
    { &series_in_overlay,        &series_in_overlay_data },

    { &defbutton_ok,     &defbutton_ok_persist_data },
    { &stdbutton_cancel, &stdbutton_cancel_data }
};

T5_MSG_PROTO(extern, chart_msg_chart_dialog, _InoutRef_ P_T5_MSG_CHART_DIALOG_DATA p_msg_chart_dialog_data)
{
    UNREFERENCED_PARAMETER_DocuRef_(p_docu);
    UNREFERENCED_PARAMETER_InVal_(t5_message);

    switch(p_msg_chart_dialog_data->reason)
    {
    case CHART_DIALOG_SERIES:
        p_msg_chart_dialog_data->p_ctl_create = series_ctl_create;
        p_msg_chart_dialog_data->n_ctls = elemof32(series_ctl_create);
        p_msg_chart_dialog_data->help_topic_resource_id = CHART_MSG_DIALOG_SERIES_EDIT_HELP_TOPIC;
        return(STATUS_OK);

    case CHART_DIALOG_AXIS_CAT:
    case CHART_DIALOG_AXIS_VAL:
        gen_axis_ticks_none_data.p_bitmap_id_offon = &gen_axis_ticks_none_bitmap[(Y_AXIS_IDX == p_msg_chart_dialog_data->modifying_axis_idx)];
        gen_axis_ticks_full_data.p_bitmap_id_offon = &gen_axis_ticks_full_bitmap[(Y_AXIS_IDX == p_msg_chart_dialog_data->modifying_axis_idx)];
        gen_axis_ticks_in_data.p_bitmap_id_offon   = &gen_axis_ticks_in_bitmap[  (Y_AXIS_IDX == p_msg_chart_dialog_data->modifying_axis_idx)];
        gen_axis_ticks_out_data.p_bitmap_id_offon  = &gen_axis_ticks_out_bitmap[ (Y_AXIS_IDX == p_msg_chart_dialog_data->modifying_axis_idx)];

        gen_axis_position_lzr_lb_data.caption.text.resource_id = (X_AXIS_IDX == p_msg_chart_dialog_data->modifying_axis_idx)
                                                               ? CHART_MSG_DIALOG_GEN_AXIS_POSITION_LZR_B
                                                               : CHART_MSG_DIALOG_GEN_AXIS_POSITION_LZR_L;
        gen_axis_position_lzr_rt_data.caption.text.resource_id = (X_AXIS_IDX == p_msg_chart_dialog_data->modifying_axis_idx)
                                                               ? CHART_MSG_DIALOG_GEN_AXIS_POSITION_LZR_T
                                                               : CHART_MSG_DIALOG_GEN_AXIS_POSITION_LZR_R;

        if(CHART_DIALOG_AXIS_CAT == p_msg_chart_dialog_data->reason)
        {
            p_msg_chart_dialog_data->p_ctl_create = cat_axis_ctl_create;
            p_msg_chart_dialog_data->n_ctls = elemof32(cat_axis_ctl_create);
            p_msg_chart_dialog_data->help_topic_resource_id = CHART_MSG_DIALOG_AXIS_CAT_EDIT_HELP_TOPIC;
        }
        else
        {
            p_msg_chart_dialog_data->p_ctl_create = val_axis_ctl_create;
            p_msg_chart_dialog_data->n_ctls = elemof32(val_axis_ctl_create);
            p_msg_chart_dialog_data->help_topic_resource_id = CHART_MSG_DIALOG_AXIS_VAL_EDIT_HELP_TOPIC;
        }

        return(STATUS_OK);

    default: default_unhandled();
        return(STATUS_FAIL);
    }
}

/* end of gr_spgal.c */
