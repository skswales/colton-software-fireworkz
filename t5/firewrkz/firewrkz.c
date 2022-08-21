/* firewrkz.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#include "common/gflags.h"

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif

#include "ob_skel/prodinfo.h"

#include "ob_skel/prodinfo.c"

const UINT
g_product_id = PRODUCT_ID_FIREWORKZ;

const PCTSTR
__product_name = TEXT("Fireworkz");

const PCTSTR
__product_family_name = TEXT("Fireworkz");

const PCTSTR
__product_ui_name = TEXT("Fireworkz");

#if RISCOS
const PC_U8Z
g_product_sprite_name = "!fireworkz";

const PC_U8Z
g_dynamic_area_name = "Fireworkz workspace";

const PCTSTR
prefix_uri_userguide_content_tstr = "file:///FireworkzManuals:UserGuide/content/";
#endif

#if WINDOWS
const PCTSTR
key_program =
TEXT("Software") TEXT("\\")
TEXT("Colton Software") TEXT("\\")
TEXT("Fireworkz");

const PCTSTR
atom_program = TEXT("Fireworkz");

const PCTSTR
window_class[] =
{
    TEXT("T5DDE"),
    TEXT("T5BACK"),
    TEXT("T5PANE"),
    TEXT("T5BORDER"),
    TEXT("T5TOOLBAR"),
    TEXT("T5STATUS"),
    TEXT("T5SLE"),
    TEXT("T5SPLASH")
};

const PCTSTR
leafname_helpfile_tstr = TEXT("firewrkz.chm");
#endif

const PCTSTR
extension_document_tstr = TEXT("fwk");

const PCTSTR
extension_template_tstr = TEXT("fwt");

const int
has_real_database = 0;

const PCTSTR
dll_store =
TEXT("Objects") FILE_DIR_SEP_TSTR
TEXT("Ob") TEXT("%.2u") RESOURCE_DLL_SUFFIX;

#include "ob_skel/t5_glue.c"

/* end of firewrkz.c */
