/* gflags.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Defines the compile-time flags for all of Fireworkz */

/* SKS 14-Feb-1989 */

#ifndef __gflags_h
#define __gflags_h

#if defined(__cplusplus)
extern "C" {
#endif

/* Define the version number of this release */
#define VERSION_NUMBER 20005
/* NB keep in step with other files (definitive list in common.mf_mid) */

#if defined(_PREFAST_)
#ifndef CODE_ANALYSIS
#define CODE_ANALYSIS 1
#endif
#endif /* _PREFAST_ */

#ifdef RELEASED
#undef UNRELEASED
#endif

#ifdef UNRELEASED
#undef UNRELEASED /* no further testing on this */
#define RELEASED 0
#else
#define RELEASED 1
#endif

#ifndef CHECKING
#if RELEASED
/* #error   Default state for RELEASED is CHECKING OFF */
#define CHECKING 0
#else
/* #error   Default state for DEBUG is CHECKING ON */
#define CHECKING 1
#endif
#endif /* CHECKING */

#if defined(CODE_ANALYSIS)
#define CHECKING_FOR_CODE_ANALYSIS 1
#else
#define CHECKING_FOR_CODE_ANALYSIS 0
#endif

#include "cmodules/coltsoft/defineos.h"

#if defined(FPROWORKZ)
/* Fireworkz Pro - deviant file suffix Y */
#define FIREWORKZ 0
#include "fprowrkz/pflags.h"

#else /* defined(FIREWORKZ) */
/* Fireworkz - deviant file suffix Z */
#ifndef FIREWORKZ
#define FIREWORKZ 1
#endif
#define FPROWORKZ 0
#include "firewrkz/pflags.h"
#endif /* PRODUCT */

#include "common/personal.h"

/* ------------------ features that are now standard --------------------- */

#ifndef BACKGROUND_LOAD_GRANULARITY
#define BACKGROUND_LOAD_GRANULARITY 200 /* ms */
#endif

#ifndef BACKGROUND_LOAD_INITIAL_RUN
#define BACKGROUND_LOAD_INITIAL_RUN 500 /* ms */
#endif

#define BOUND_ALL_MSGS 1
#define BOUND_ALL_SPRITES 1

/* affects just vi_cmd (sizing), ob_ruler (rendering) */
#define MARKERS_INSIDE_RULER_HORZ 1
#define MARKERS_INSIDE_RULER_VERT 1

#define COMPLEX_STRING 1

#if WINDOWS && 1
#define USE_GLOBAL_CLIPBOARD 1
#endif

/* -------------- new features, not in release version yet --------------- */

#if !RELEASED

#endif /* !RELEASED */

/* ------------- checking features, never in release version ------------- */

#if !RELEASED

#ifndef AL_PTR_MAP_TO_HANDLE
/*#define AL_PTR_MAP_TO_HANDLE*/
#endif

#if (1) && !defined(CODE_ANALYSIS)
#define UNUSED_KEEP_ALIVE 1
#endif

#endif /* !RELEASED */

/* ---------- paranoid checks to ensure kosher release version ----------- */

#if RELEASED

#if !CHECKING
#define NDEBUG 1
#endif

#if RISCOS
/* Leave stack checking enabled in 32-bit RISC OS release for the mo */
#endif

#endif /* RELEASED */

/* ----------------------- RISC OS specific flags ------------------------ */

#if RISCOS
#ifndef SHAKE_HEAP_VIOLENTLY
#define ALLOC_RELEASE_STORE_INFREQUENTLY 1
#endif

#define ALIGATOR_USE_ALLOC 1
#endif /* RISCOS */

/* ----------------------- WINDOWS specific flags ------------------------ */

#if defined(__cplusplus)
}   /* ... extern "C" */
#endif

/* standard header files after all flags defined - allow these to do their own C++ thing */
#include "cmodules/coltsoft/ansi.h"

#if defined(__cplusplus)
extern "C" {
#endif

/* much more of differences covered in coltsoft/coltsoft.h ... */
#include "cmodules/coltsoft/coltsoft.h"

/*
RISC OS Draw file stuff needed on all platforms
*/

#include "cmodules/coltsoft/drawxtra.h"

/* debug module includes */
#include "cmodules/debug.h"
#include "cmodules/myassert.h"

/* base Fireworkz definitions */
#include "ob_skel/xp_skel.h"

#if defined(_MSC_VER) && (!RELEASED || 0)
/* much speed up with precompiled header stuff on MSVC */

#ifndef         __xp_skeld_h
#include "ob_skel/xp_skeld.h"
#endif

#ifndef    __skel_flags_h
#include "ob_skel/flags.h"
#endif
#endif /* _MSC_VER */

#if defined(__cplusplus)
}   /* ... extern "C" */
#endif

#endif /* __gflags_h */

/* end of gflags.h */
