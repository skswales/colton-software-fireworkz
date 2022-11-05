/* riscos/osfile.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Header for RISC OS filing system calls */

/* SKS 24-May-1991 */

#ifndef __riscos_osfile_h
#define __riscos_osfile_h

/*
OS_File reason codes
*/

#define OSFile_Load        0xFF    /* Uses File$Path */
#define OSFile_Save        0
#define OSFile_WriteInfo   1
#define OSFile_WriteLoad   2
#define OSFile_WriteExec   3
#define OSFile_WriteAttr   4
#define OSFile_ReadInfo    5       /* Uses File$Path */
#define OSFile_Delete      6
#define OSFile_Create      7
#define OSFile_CreateDir   8
#define OSFile_SetStamp    9
#define OSFile_SaveStamp   10
#define OSFile_CreateStamp 11
#define OSFile_LoadPath    12      /* Uses given path */
#define OSFile_ReadPath    13      /* Uses given path */
#define OSFile_LoadPathVar 14      /* Uses given path variable */
#define OSFile_ReadPathVar 15      /* Uses given path variable */
#define OSFile_LoadNoPath  16      /* No nonsense load */
#define OSFile_ReadNoPath  17      /* No nonsense read */
#define OSFile_SetType     18
#define OSFile_MakeError   19

#define OSFile_ReasonCode int

/*
object types
*/

#define OSFile_ObjectType_None  0
#define OSFile_ObjectType_File  1
#define OSFile_ObjectType_Dir   2
#define OSFile_ObjectType_Image 3

#define OSFile_ObjectType int

/*
object attributes
*/

#define OSFile_ObjectAttribute_read         (1 << 0)
#define OSFile_ObjectAttribute_write        (1 << 1)
#define OSFile_ObjectAttribute_locked       (1 << 3)
#define OSFile_ObjectAttribute_public_read  (1 << 4)
#define OSFile_ObjectAttribute_public_write (1 << 5)

#define OSFile_ObjectAttribute int

/*
OS_GBPB reason codes
*/

#define OSGBPB_WriteAtGiven          1
#define OSGBPB_WriteAtPTR            2
#define OSGBPB_ReadFromGiven         3
#define OSGBPB_ReadFromPTR           4

#define OSGBPB_ReadDiscName          5
#define OSGBPB_ReadCSDName           6
#define OSGBPB_ReadLIBName           7
#define OSGBPB_ReadCSDEntries        8

#define OSGBPB_ReadDirEntries        9
#define OSGBPB_ReadDirEntriesInfo    10
#define OSGBPB_ReadDirEntriesCatInfo 11

#define OSGBPB_ReasonCode int

/*
OS_Args reason codes
*/

#define OSArgs_ReadPTR    0
#define OSArgs_SetPTR     1
#define OSArgs_ReadEXT    2
#define OSArgs_SetEXT     3
#define OSArgs_ReadSize   4
#define OSArgs_EOFCheck   5
#define OSArgs_EnsureSize 6

#define OSArgs_ReadInfo   0xFE
#define OSArgs_Flush      0xFF

#define OSArgs_ReasonCode int

/*
OS_Find reason codes
*/

#define OSFind_CloseFile    0x00
#define OSFind_OpenRead     0x40
#define OSFind_CreateUpdate 0x80
#define OSFind_OpenUpdate   0xC0

#define OSFind_ReasonCode int

#define OSFind_UseFilePath  0x00 /* these four are mutually exclusive */
#define OSFind_UsePath      0x01
#define OSFind_UsePathVar   0x02
#define OSFind_UseNoPath    0x03
#define OSFind_EnsureNoDir  0x04
#define OSFind_EnsureOpen   0x08 /* gives error instead of handle = 0 return */

#define OSFind_ExtraRCBits int

/*
OS_FSControl reason codes
*/

#define OSFSControl_Rename 25

#endif /* __riscos_osfile_h */

/* end of riscos/osfile.h */
