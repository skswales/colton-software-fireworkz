/* ho_gdip.cpp */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2013-2015 Stuart Swales */

/* Remember to disable use of precompiled headers for this file */

#define CPLUSPLUSINTERFACE

#include "common/gflags.h"

#pragma warning(disable:4263) /* 'x' : member function does not override any base class virtual member function */
#pragma warning(disable:4264) /* 'x' : no override available for virtual member function from base 'y'; function is hidden */

#include <gdiplus.h>
using namespace Gdiplus;

#pragma comment(lib, "delayimp.lib")

#pragma comment(lib, "Gdiplus.lib")

static ULONG_PTR g_gdiplusToken;

extern "C" void
host_gdiplus_startup(void)
{
    GdiplusStartupInput gdiplusStartupInput;

    Status status = GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, NULL);

    if(Ok != status)
    {
    }
}

extern "C" void
host_gdiplus_shutdown(void)
{
    GdiplusShutdown(g_gdiplusToken);
}

/* end of ho_gdip.cpp */
