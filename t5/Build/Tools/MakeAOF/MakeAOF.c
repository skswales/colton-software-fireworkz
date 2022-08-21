/* MakeAOF.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1995-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* make object file from a lump of data
 *
 * Stuart K. Swales 10-May-1995
 *
 * History:
 *  0.10 10-May-95 SKS recreated to stop ObjMunge buggering up once and for all
 *  0.11 11-May-95 SKS add option for length word on front of data area to make sprite areas!
 *  0.12 15-May-95 SKS -spritearea to do length of file + 4. Ta Richard
 *  0.13 05-Apr-01 SKS fflush() before fsetpos() !!! new include file location. Add 32bit option
 *  0.14 28-Feb-11 SKS Output data to read-only C$$constdata
 *  0.15 09-Jan-12 SKS Bigger buffer for longer output filenames
 *  0.16 13-Feb-14 SKS MPL-ed
*/

#define VERSION "0.16"

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "kernel.h" /* for _kernel_oscli() */

#define STATUS long

#define FILEPOS_T unsigned long /* chunk files won't ever be >= 4GB 'cos of internal limits */

#define status_return(status) { STATUS x = (status); if(0 > x) return(x); }

static int output_is_code = 0;
static int output_length_word = 0;

static int Debug = 0;

static int g_f32bit = 0;

/**********************************************************************************
*
* Acorn Chunk File handling
*
**********************************************************************************/

#define CHUNKFILEID 0xC3CBC6C5

/*
object in the file
*/

typedef struct _CHUNKFILE_HEADER_ENTRY
{
    char            chunkID[8];
    FILEPOS_T       dataOffset;
    unsigned long   dataSize;
}
CHUNKFILE_HEADER_ENTRY;

typedef CHUNKFILE_HEADER_ENTRY * P_CHUNKFILE_HEADER_ENTRY;

#define CHUNKFILE_HEADER_ENTRY_ASSERT() assert(sizeof(CHUNKFILE_HEADER_ENTRY) == 4*4)

/*
program structure
*/

typedef struct _CHUNKFILE_HEADER_ENTRY_REF
{
    CHUNKFILE_HEADER_ENTRY  e;
    FILEPOS_T               patchOffset;
}
CHUNKFILE_HEADER_ENTRY_REF;

typedef CHUNKFILE_HEADER_ENTRY_REF * P_CHUNKFILE_HEADER_ENTRY_REF;

/*
these numbers have no relation to the real ids except through the table
*/

typedef enum _chunk_type
{
    OBJ_HEAD,
    OBJ_AREA,
    OBJ_IDFN,
    OBJ_SYMT,
    OBJ_STRT
}
chunk_type;

/*
one big string
*/

#define CHUNK_ID_DEFS \
static const char chunk_id[] = \
    "OBJ_HEAD" \
    "OBJ_AREA" \
    "OBJ_IDFN" \
    "OBJ_SYMT" \
    "OBJ_STRT"

#define chunk_full_id(id) (chunk_id + (id) * 8)

#define AOF_HEADER_CHUNK_AOF     0xC5E2D080
#define AOF_HEADER_CHUNK_VERSION 150

typedef struct _AOF_HEADER_CHUNK
{
    unsigned long   objectFileType;
    unsigned long   versionId; /* 150 */
    unsigned long   nAreas;
    unsigned long   nSymbols;
    long            entryAddrArea; /* 1..nChunks */
    long            entryAddrOffset;
    /* AOF_AREA_HEADERs follow */
}
AOF_HEADER_CHUNK;

#define AOF_HEADER_CHUNK_ASSERT() assert(sizeof(AOF_HEADER_CHUNK) == 6*4)

typedef struct _AOF_AREA_HEADER
{
    unsigned long   areaName; /* offset in string table */
    unsigned char   areaAlign;
    unsigned char   areaAttr;
    unsigned char   areaAttrExtra;
    unsigned char   filler;
    unsigned long   areaSize;
    unsigned long   nRelocs;
    void *          baseAddr;
}
AOF_AREA_HEADER;

#define AOF_AREA_HEADER_ASSERT() assert(sizeof(AOF_AREA_HEADER) == 5*4)

typedef AOF_AREA_HEADER * P_AOF_AREA_HEADER;

typedef enum _AOF_AREA_ATTRIBUTES_EXTRA
{
    aof_area_extra_32bit = 0x01
}
AOF_AREA_ATTRIBUTES_EXTRA;

typedef enum _AOF_AREA_ATTRIBUTES
{
  /*aof_area_absolute  = 0x01, withdrawn by PRM3 D4-428 */
    aof_area_code      = 0x02,
    aof_area_data      = 0x00,
    aof_area_commondef = 0x04,
    aof_area_common    = 0x08,
    aof_area_uninit    = 0x10,
    aof_area_readonly  = 0x20,
  /*aof_area_pic       = 0x40, withdrawn by PRM3 D4-428 */
    aof_area_debugging = 0x80
}
AOF_AREA_ATTRIBUTES;

typedef struct _AOF_AREA_REF
{
    AOF_AREA_HEADER h;
    const void *    areaData;
}
AOF_AREA_REF;

typedef AOF_AREA_REF * P_AOF_AREA_REF;

typedef struct _AOF_SYMBOL_TABLE_ENTRY
{
    unsigned long   symbolName;
    int             symbolAttr:6;
    int             filler:32-6;
    long            symbolValue; /* offset if relative */
    unsigned long   areaName;
}
AOF_SYMBOL_TABLE_ENTRY;

#define AOF_SYMBOL_TABLE_ENTRY_ASSERT() assert(sizeof(AOF_SYMBOL_TABLE_ENTRY) == 4*4)

typedef enum _AOF_SYMBOL_ATTRIBUTES
{
    aof_symbol_reserved  = 0x00,
    aof_symbol_local     = 0x01,
    aof_symbol_external  = 0x02,
    aof_symbol_global    = 0x03,
    aof_symbol_absolute  = 0x04,
    aof_symbol_relative  = 0x00,
    aof_symbol_nocase    = 0x08,
    aof_symbol_case      = 0x00,
    aof_symbol_weak      = 0x10,
    aof_symbol_strong    = 0x20
}
AOF_SYMBOL_ATTRIBUTES;

#define CHUNK_ASSERT() \
    CHUNKFILE_HEADER_ENTRY_ASSERT(); \
    AOF_HEADER_CHUNK_ASSERT(); \
    AOF_AREA_HEADER_ASSERT(); \
    AOF_SYMBOL_TABLE_ENTRY_ASSERT()

/*
exported functions
*/

extern int main(int argc, char * argv[]);

/*
internal functions
*/

static STATUS Area_Entries(void);
static STATUS Area_Init(void);
static STATUS Area_Output(FILE * fout);
static STATUS Area_OutputHeader(FILE * fout);

static STATUS AreaRef_Alloc(unsigned long * out);
static void
AreaRef_Init(
    unsigned long areaNumber,
    unsigned long areaName,
    const void * areaData,
    unsigned long areaSize,
    AOF_AREA_ATTRIBUTES areaAttr,
    AOF_AREA_ATTRIBUTES_EXTRA areaAttrExtra);

static STATUS ChunkFile_Init(FILE * fout);
static STATUS ChunkFile_Output(FILE * fout);

static STATUS ChunkRef_Alloc(void);
static STATUS ChunkRef_FixupHeader(FILE * fout, unsigned long chunkNumber, unsigned long chunkType, unsigned long chunkLen, const FILEPOS_T * pos);

static STATUS Header_Init(void);
static STATUS Header_Output(FILE * fout);

static STATUS Identification_Init(void);
static STATUS Identification_Output(FILE * fout);
static STATUS StringTable_Add(const char * string, unsigned long * out);

static STATUS StringTable_Init(void);
static STATUS StringTable_Output(FILE * fout);

static STATUS SymbolTable_Add(const char * symbolName, AOF_SYMBOL_ATTRIBUTES symbolAttr, unsigned long symbolValue, unsigned long areaName);
static STATUS SymbolTable_Entries(void);
static STATUS SymbolTable_Init(void);
static STATUS SymbolTable_Output(FILE * fout);

static STATUS create_aof_file(FILE * fout);
static STATUS end_aof_file(FILE * fout);
static void   give_help_on_app(void);
static STATUS make_aof_file(FILE * fin, FILE * fout, const char * symbolname);
static long   load_input_file(FILE * fin);

/* ----------------------------------------------------------------------- */

#define NUL '\0'

#define ARG_STROP_CHAR '-'
#define ARG_STROP_STR  "-"

static STATUS
file_getpos(FILE * f, FILEPOS_T * fp)
{
    fpos_t l;

    if(!fgetpos(f, &l))
    {
        *fp = * (FILEPOS_T *) &l;
        if(Debug) printf("fgetpos() got &%08X\n", * (unsigned int *) fp);
        return(0);
    }

    *fp = 0;
    fprintf(stderr, "fgetpos() failed\n");
    return(-1L);
}

static STATUS
file_setpos(FILE * f, const FILEPOS_T * fp)
{
    fpos_t l;

    memset(&l, NUL, sizeof(l));

    * (FILEPOS_T *) &l = *fp;

    /* need to flush before reposition with new SharedCLibrary 5.28 / RISC OS 4.02 !!! */
    (void) fflush(f);

    if(!fsetpos(f, &l))
    {
        if(Debug) printf("fsetpos(&%08X) ok\n", * (unsigned int *) fp);
        return(0);
    }

    fprintf(stderr, "fsetpos() failed\n");
    return(-1L);
}

static STATUS
file_write_err(const void * buffer, unsigned long size, unsigned long count, FILE * stream)
{
    if(Debug)
    {
        const char * p = buffer;
        unsigned long bytes = size * count;
        printf("file_write_err(%ld,%ld) ", size, count);
        if(bytes > 16)
        {
            printf("&%02X &%02X &%02X &%02X &%02X &%02X &%02X &%02X "
                   "&%02X &%02X &%02X &%02X &%02X &%02X &%02X &%02X ...",
                   p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7],
                   p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]);
        }
        else
        {
            unsigned long i;
            for(i = 0; i < bytes; i++)
            {
                printf("&%02X ", p[i]);
            }
        }
        printf("\n");
    }

    if(count == (unsigned long) fwrite(buffer, (size_t) size, (size_t) count, stream))
        return(0);

    fprintf(stderr, "file_write_err: fwrite() failed\n");
    return(-1L);
}

/********************************************************
*
* pad bytes out from seqptr to a power of two on stream
*
********************************************************/

extern long
file_pad(FILE * file_handle, unsigned int alignpower)
{
    long alignment;
    long alignmask;
    long res32;

    if(!alignpower)
        return(0);

    alignment = 1L << alignpower;
    alignmask = alignment - 1L;

    if(-1L == (res32 = ftell(file_handle)))
    {
        fprintf(stderr, "file_pad: ftell() failed\n");
        return(-1L);
    }

    /*fprintf(stderr, "file_pad: ftell() returned &%8.8lX\n", res32);*/

    if(res32 & alignmask)
    {
        alignment = alignment - (res32 & alignmask);

        /*fprintf(stderr, "file_pad: alignment &%8.8lX\n", alignment);*/

        do  {
            if(EOF == fputc(NUL, file_handle))
            {
                fprintf(stderr, "file_pad: fputc() failed\n");
                return(-1L);
            }
        }
        while(--alignment);
    }

    return(0);
}

/***********************************
*
* output little-endian 32-bit word
*
***********************************/

static STATUS
file_put32(FILE * fout, unsigned long word)
{
    int count = 0;

    if(Debug) printf("file_put32(&%08lX)\n", word);

    do  {
        if(EOF == fputc((int) word & 0xFF, fout))
        {
            fprintf(stderr, "file_put32: fputc() failed\n");
            return(-1L);
        }

        word >>= 8;
    }
    while(++count < sizeof(word));

    return(0);
}

static char * dataBlock = NULL;

static long
load_input_file(FILE * fin)
{
    unsigned long real_length;
    unsigned long alloc_length;
    unsigned long bodge;

    if(Debug) puts("load_input_file()");

    if(fseek(fin, 0L, SEEK_END))
    {
        fprintf(stderr, "load_input_file: fseek(END) failed\n");
        return(-1L);
    }

    {
    long res = ftell(fin);
    if(-1L == res)
    {
        fprintf(stderr, "load_input_file: ftell() failed\n");
        return(-1L);
    }
    real_length = (unsigned long) res;
    } /*block*/

    if(fseek(fin, 0L, SEEK_SET))
    {
        fprintf(stderr, "load_input_file: fseek(STT) failed\n");
        return(-1L);
    }

    alloc_length = real_length;

    /* new (> C3) linkers cannot cope with non-word sized objects! */
    bodge = real_length & 3;
    if(bodge)
    {
        bodge = 4 - bodge;
        alloc_length += bodge;
    }

    {
    char * load_address;

    if(output_length_word)
        alloc_length += 4;

    dataBlock = malloc((size_t) alloc_length);

    if(!dataBlock)
    {
        fprintf(stderr, "load_input_file: malloc(&%8.8lX) failed\n", alloc_length);
        return(-1L);
    }

    load_address = dataBlock;

    if(output_length_word)
    {
        * (unsigned long *) load_address = real_length + 4 /*'cos this has been chopped off*/;
        load_address += 4;
    }

    if(real_length != (unsigned long) fread(load_address, 1, (size_t) real_length, fin))
    {
        fprintf(stderr, "load_input_file: fread() failed\n");
        return(-1L);
    }
    } /*block*/

    if(bodge)
    {
        do  {
            dataBlock[alloc_length - bodge] = NUL;
        }
        while(--bodge);
    }

    return((long) alloc_length);
}

/**********************************************************************************
*
* Acorn Chunk File handling
*
**********************************************************************************/

CHUNK_ID_DEFS; /* dump the chunk identifiers in the object */

#define MAX_CHUNKS 8

static CHUNKFILE_HEADER_ENTRY_REF chunkref[MAX_CHUNKS];
static unsigned long nChunks = 0;
static unsigned long nChunksPos;

static STATUS
ChunkFile_Init(FILE * fout)
{
    static CHUNKFILE_HEADER_ENTRY initchunkheader = { 'U', 'n', 'u', 's', 'e', 'd', ' ', ' ' };

    CHUNK_ASSERT();

    /* chunkfile id word */
    if(fseek(fout, 0L, SEEK_SET))
    {
        fprintf(stderr, "ChunkFile_Init: fseek(STT) failed\n");
        return(-1L);
    }
    
    status_return(file_put32(fout, CHUNKFILEID));
    status_return(file_put32(fout, MAX_CHUNKS));
    status_return(file_getpos(fout, &nChunksPos));
    status_return(file_put32(fout, 0L));

    { /* output space for chunk entries */
    long res;
    long chunkNumber = 0;
    do  {
        if(0 > (res = file_getpos(fout, &chunkref[chunkNumber].patchOffset)))
            break;

        if(0 > (res = file_write_err(&initchunkheader, sizeof(initchunkheader), 1, fout)))
            break;
    }
    while(++chunkNumber < MAX_CHUNKS);
    return(res);
    } /*block*/
}

static STATUS
ChunkFile_Output(FILE * fout)
{
    FILEPOS_T pos;
    status_return(file_getpos(fout, &pos));
    status_return(file_setpos(fout, &nChunksPos));
    status_return(file_put32(fout, nChunks));
    status_return(file_setpos(fout, &pos));
    return(0);
}

/**********************
*
* allocate a chunkref
*
**********************/

static STATUS
ChunkRef_Alloc(void)
{
    if(nChunks < MAX_CHUNKS)
        return(nChunks++);

    fprintf(stderr, "ChunkRef_Alloc: too many chunks\n");
    return(-1L);
}

/******************************************
*
* about a to write a chunk - fixup header
*
******************************************/

static STATUS
ChunkRef_FixupHeader(FILE * fout, unsigned long chunkNumber, unsigned long chunkType, unsigned long chunkLen, const FILEPOS_T * pos)
{
    P_CHUNKFILE_HEADER_ENTRY_REF hrp = &chunkref[chunkNumber];
    P_CHUNKFILE_HEADER_ENTRY hep = &hrp->e;
    FILEPOS_T curpos;

    status_return(file_getpos(fout, &curpos));

    /*fprintf(stderr, "ChunkRef_FixupHeader: curpos = &%8.8lx\n", curpos);*/

    /* copy over full ID */
    memcpy(hep->chunkID, chunk_full_id(chunkType), sizeof(hep->chunkID));
    hep->dataOffset = pos ? *pos : curpos; /* if pos == NULL this means patch the header to the current point in file */
    hep->dataSize = chunkLen;

    status_return(file_setpos(fout, &hrp->patchOffset));
    status_return(file_write_err(hep, sizeof(*hep), 1, fout));
    status_return(file_setpos(fout, &curpos));
    return(0);
}

/*************************************************************************************
*
* Header Chunk (OBJ_HEAD)
*
*************************************************************************************/

static STATUS headerChunk;

static STATUS
Header_Init(void)
{
    headerChunk = ChunkRef_Alloc();

    return(0);
}

static STATUS
Header_Output(FILE * fout)
{
    AOF_HEADER_CHUNK ahc;
    unsigned long nAreas = Area_Entries();
    unsigned long nSymbols = SymbolTable_Entries();
    unsigned long headerSize = sizeof(AOF_HEADER_CHUNK) + sizeof(AOF_AREA_HEADER) * nAreas;

    ahc.objectFileType  = AOF_HEADER_CHUNK_AOF;
    ahc.versionId       = AOF_HEADER_CHUNK_VERSION;
    ahc.nAreas          = nAreas;
    ahc.nSymbols        = nSymbols;
    ahc.entryAddrArea   = 0; /* no entry defined */
    ahc.entryAddrOffset = 0;

    status_return(ChunkRef_FixupHeader(fout, headerChunk, OBJ_HEAD, headerSize, NULL));
    status_return(file_write_err(&ahc, sizeof(ahc), 1, fout));
    status_return(Area_OutputHeader(fout));
    /* no padding needed as all these are word multiples */
    return(0);
}

/*************************************************************************************
*
* Area Chunk(s) (OBJ_AREA)
*
* NB. all areas are stored within the one OBJ_AREA chunk
*
*************************************************************************************/

static STATUS areaChunk;

#define MAX_AREAS 8

static AOF_AREA_REF arearef[MAX_AREAS];
static unsigned long nAreas = 0UL;

static STATUS
Area_Entries(void)
{
    return(nAreas);
}

static STATUS
Area_Init(void)
{
    areaChunk = ChunkRef_Alloc();
    return(areaChunk);
}

static STATUS
Area_Output(FILE * fout)
{
    FILEPOS_T startpos;
    FILEPOS_T endpos;
    unsigned long areaNumber;
    long res = 0;

    status_return(file_getpos(fout, &startpos));

    for(areaNumber = 0; areaNumber < nAreas; ++areaNumber)
    {
        P_AOF_AREA_REF arp = &arearef[areaNumber];
        P_AOF_AREA_HEADER ahp = &arp->h;

        /* output data for this area */
        if((res = file_write_err(arp->areaData, (unsigned long) ahp->areaSize, 1, fout)) < 0)
            break;
        if((res = file_getpos(fout, &endpos)) < 0)
            break;
        if((res = file_pad(fout, 2)) < 0)
            break;

        /* output relocations for this area */
        if(ahp->nRelocs)
        {
            /* can't cope with relocations here */
            if((res = file_getpos(fout, &endpos)) < 0)
                break;
            if((res = file_pad(fout, 2)) < 0)
                break;
        }
    }

    if(res >= 0)
    {
        unsigned long areaSize = endpos - startpos;
        res = ChunkRef_FixupHeader(fout, areaChunk, OBJ_AREA, areaSize, &startpos /*where the area did start*/);
    }

    return(res);
}

static STATUS
Area_OutputHeader(FILE * fout)
{
    unsigned long areaNumber;

    for(areaNumber = 0; areaNumber < nAreas; ++areaNumber)
    {
        P_AOF_AREA_HEADER ahp = &arearef[areaNumber].h;

        status_return(file_write_err(ahp, sizeof(*ahp), 1, fout));
    }

    return(0);
}

static STATUS
AreaRef_Alloc(unsigned long * out)
{
    if(nAreas < MAX_AREAS)
    {
        *out = nAreas++;
        return(0);
    }

    fprintf(stderr, "AreaRef_Alloc: too many areas");
    return(-1L);
}

static void
AreaRef_Init(
    unsigned long areaNumber,
    unsigned long areaName,
    const void * areaData,
    unsigned long areaSize,
    AOF_AREA_ATTRIBUTES areaAttr,
    AOF_AREA_ATTRIBUTES_EXTRA areaAttrExtra)
{
    P_AOF_AREA_REF arp = &arearef[areaNumber];
    P_AOF_AREA_HEADER ahp = &arp->h;

    memset(arp, NUL, sizeof(*arp));

    ahp->areaName  = areaName;
    ahp->areaSize  = areaSize;
    ahp->areaAlign = 2; /* the only kosher value for AOF 150+ */
    ahp->areaAttr  = areaAttr;
    ahp->areaAttrExtra  = areaAttrExtra;

    arp->areaData  = areaData;
}


/*************************************************************************************
*
* Identification Chunk (OBJ_IDFN)
*
*************************************************************************************/

static const char identificationString[] = "Colton Software MakeAOF vsn " VERSION " [" __DATE__ "]";

static STATUS identificationChunk;

static STATUS
Identification_Init(void)
{
    identificationChunk = ChunkRef_Alloc();
    return(identificationChunk);
}

static STATUS
Identification_Output(FILE * fout)
{
    size_t identificationSize = strlen(identificationString) + 1;
    status_return(ChunkRef_FixupHeader(fout, identificationChunk, OBJ_IDFN, identificationSize, NULL));
    status_return(file_write_err(identificationString, identificationSize, 1, fout));
    status_return(file_pad(fout, 2));
    return(0);
}

/*************************************************************************************
*
* String Table Chunk (OBJ_STRT)
*
*************************************************************************************/

#define STRINGTABLE_INITSIZE 4096
#define STRINGTABLE_INCSIZE  1024

static STATUS stringTableChunk;

typedef struct _stringTableStr
{
    unsigned long size;
    char data[1];
}
stringTableStr;

#define STRINGTABLE_HEADSIZE offsetof(stringTableStr, data)

static stringTableStr * stringTable = NULL;
static unsigned long stringTableOff = 0UL;

/*************************************************
*
* add a string to the string table
* returns offset in string table where allocated
*
*************************************************/

static STATUS
StringTable_Add(const char * string, unsigned long * out)
{
    size_t len = strlen(string) + 1;

    if((stringTableOff + len) > stringTable->size)
    {
        unsigned long newsize = stringTable->size + STRINGTABLE_INCSIZE;
        void * newptr = realloc(stringTable, (size_t) newsize);
        if(!newptr)
        {
            fprintf(stderr, "StringTable_Add: realloc(&%8.8lX) failed\n", newsize);
            return(-1L);
        }
        stringTable = newptr;
        stringTable->size = newsize;
    }

    memcpy(&stringTable->data[stringTableOff - STRINGTABLE_HEADSIZE], string, len);
    *out = stringTableOff;
    stringTableOff += len;
    return(0);
}

static STATUS
StringTable_Init(void)
{
    {
    unsigned long newsize = STRINGTABLE_INITSIZE;
    void * newptr = malloc((size_t) newsize);
    if(!newptr)
    {
        fprintf(stderr, "StringTable_Init: realloc(&%8.8lX) failed\n", newsize);
        return(-1L);
    }
    stringTable = newptr;
    stringTable->size = newsize; 
    }

    stringTableOff = STRINGTABLE_HEADSIZE;

    stringTableChunk = ChunkRef_Alloc();

    return(stringTableChunk);
}

static STATUS
StringTable_Output(FILE * fout)
{
    unsigned long stringTableSize;
    long res;

    /* first word is currently allocated length of table, make for file format */
    stringTableSize = stringTableOff;
    stringTable->size = stringTableSize;

    status_return(ChunkRef_FixupHeader(fout, stringTableChunk, OBJ_STRT, stringTableSize, NULL));
    status_return(file_write_err(stringTable, stringTableSize, 1, fout));
    status_return(file_pad(fout, 2));

    return(res);
}

/*************************************************************************************
*
* Symbol Table Chunk (OBJ_SYMT)
*
*************************************************************************************/

static STATUS symbolTableChunk;

#define MAX_SYMBOLS 1024UL

static AOF_SYMBOL_TABLE_ENTRY symbolTable[MAX_SYMBOLS];
static unsigned long nSymbols = 0;

static STATUS
SymbolTable_Add(const char * symbolName, AOF_SYMBOL_ATTRIBUTES symbolAttr, unsigned long symbolValue, unsigned long areaName)
{
    AOF_SYMBOL_TABLE_ENTRY * sp;
    unsigned long symbolNumber = nSymbols++;

    if(nSymbols > MAX_SYMBOLS)
    {
        fprintf(stderr, "SymbolTable_Add: too many symbols\n");
        return(-1L);
    }

    sp = &symbolTable[symbolNumber];
    (void) StringTable_Add(symbolName, &sp->symbolName);
    sp->symbolAttr  = symbolAttr;
    sp->symbolValue = symbolValue;
    sp->areaName    = areaName;

    return(sp->symbolName);
}

static STATUS
SymbolTable_Entries(void)
{
    return(nSymbols);
}

static STATUS
SymbolTable_Init(void)
{
    symbolTableChunk = ChunkRef_Alloc();
    return(symbolTableChunk);
}

static STATUS
SymbolTable_Output(FILE * fout)
{
    unsigned long symbolTableSize = nSymbols * sizeof(AOF_SYMBOL_TABLE_ENTRY);

    status_return(ChunkRef_FixupHeader(fout, symbolTableChunk, OBJ_SYMT, symbolTableSize, NULL));

    {
    unsigned long symbolNumber;

    for(symbolNumber = 0; symbolNumber < nSymbols; ++symbolNumber)
    {
        AOF_SYMBOL_TABLE_ENTRY * sp = &symbolTable[symbolNumber];

        status_return(file_write_err(sp, sizeof(*sp), 1, fout));
    }
    } /*block*/

    return(0);
}

static STATUS
create_aof_file(FILE * fout)
{
    if(Debug) puts("create_aof_file()");

    /* initialise in conventional order for aof chunk files */
    if(Debug) puts("ChunkFile_Init()");
    status_return(ChunkFile_Init(fout));

    if(Debug) puts("Header_Init()");
    status_return(Header_Init());

    if(Debug) puts("Area_Init()");
    status_return(Area_Init());

    if(Debug) puts("Identification_Init()");
    status_return(Identification_Init());

    if(Debug) puts("StringTable_Init()");
    status_return(StringTable_Init());

    if(Debug) puts("SymbolTable_Init()");
    status_return(SymbolTable_Init());

    if(Debug) puts("create_aof_file() ok");
    return(0);
}

static STATUS
end_aof_file(FILE * fout)
{
    if(Debug) puts("end_aof_file()");

    /* now all data structures ok, output them */
    if(Debug) puts("ChunkFile_Output()");
    status_return(ChunkFile_Output(fout));

    if(Debug) puts("Area_Output()");
    status_return(Area_Output(fout));

    if(Debug) puts("Identification_Output()");
    status_return(Identification_Output(fout));

    if(Debug) puts("StringTable_Output()");
    status_return(StringTable_Output(fout));

    if(Debug) puts("SymbolTable_Output()");
    status_return(SymbolTable_Output(fout));

    if(Debug) puts("Header_Output()");
    status_return(Header_Output(fout));

    if(Debug) puts("end_aof_file() ok");
    return(0);
}

static STATUS
make_aof_file(FILE * fin, FILE * fout, const char * dataSymbolName)
{
    const void *  codeAreaData = NULL;
    unsigned long codeAreaSize;
    unsigned long codeAreaName;
    unsigned long codeAreaNumber;

    const void *  dataAreaData = NULL;
    unsigned long dataAreaSize;
    unsigned long dataAreaName;
    unsigned long dataAreaNumber;

    STATUS status;
    long res32;

    if(Debug) puts("make_aof_file()");

    res32 = load_input_file(fin);

    if(res32 > 0)
    {
        unsigned long dataLength = (unsigned long) res32;

        status = create_aof_file(fout);

        if((status >= 0) && output_is_code)
        {
            codeAreaData = dataBlock;
            codeAreaSize = dataLength;

            if(codeAreaData  &&  codeAreaSize)
            {
                AOF_AREA_ATTRIBUTES areaAttr = (AOF_AREA_ATTRIBUTES) (aof_area_code | aof_area_readonly);
                AOF_AREA_ATTRIBUTES_EXTRA areaAttrExtra = (AOF_AREA_ATTRIBUTES_EXTRA) 0;

                if(g_f32bit) areaAttrExtra = aof_area_extra_32bit;

                status_return(AreaRef_Alloc(&codeAreaNumber));

                status_return(StringTable_Add("C$$code",&codeAreaName));

                AreaRef_Init(codeAreaNumber, codeAreaName, codeAreaData, codeAreaSize, areaAttr, areaAttrExtra);

                status = SymbolTable_Add(dataSymbolName, aof_symbol_global, 0, codeAreaName);
            }
        }

        if((status >= 0) && !output_is_code)
        {
            dataAreaData = dataBlock;
            dataAreaSize = dataLength;

            if(dataAreaData  &&  dataAreaSize)
            {
                AOF_AREA_ATTRIBUTES areaAttr = (AOF_AREA_ATTRIBUTES) (aof_area_data | aof_area_readonly);
                AOF_AREA_ATTRIBUTES_EXTRA areaAttrExtra = (AOF_AREA_ATTRIBUTES_EXTRA) 0;

                status_return(AreaRef_Alloc(&dataAreaNumber));

                status_return(StringTable_Add("C$$constdata", &dataAreaName));

                AreaRef_Init(dataAreaNumber, dataAreaName, dataAreaData, dataAreaSize, areaAttr, areaAttrExtra);

                status = SymbolTable_Add(dataSymbolName, aof_symbol_global, 0, dataAreaName);
            }
        }

        if(status >= 0)
            status = end_aof_file(fout);
    }
    else if(res32 == 0)
    {
        fprintf(stderr, "input file zero length\n");
        status = -1L;
    }
    else
        status = res32;

    return(status);
}

static int
argmatch(const char * arg, const char * test)
{
    if(!strcmp(arg, test))
        return(1);

    if(*arg  &&  !arg[1]  &&  (tolower(*arg) == tolower(*test)))
        return(1);

    return(0);
}

static void
give_help_on_app(void)
{
    puts(identificationString);
    puts("\nMakeAOF [-symbol] symbol [-from] infile [-to] outfile [-codeseg] [-spritearea] [-32bit]");
}

extern int
main(int argc, char * argv[])
{
    const char * infilename  = NULL;
    const char * outfilename = NULL;
    const char * symbolname  = NULL;
    int res;
    int argi = 0;

    while(++argi < argc)
    {
        const char * arg = argv[argi];

        if(*arg == ARG_STROP_CHAR)
        {
            ++arg;

            if(argmatch(arg, "help"))
            {
                give_help_on_app();
                return(EXIT_SUCCESS);
            }

            if(argmatch(arg, "from"))
            {
                infilename = argv[++argi];
                continue;
            }

            if(argmatch(arg, "o") || argmatch(arg, "to"))
            {
                outfilename = argv[++argi];
                continue;
            }

            if(argmatch(arg, "symbol"))
            {
                symbolname = argv[++argi];
                continue;
            }

            if(argmatch(arg, "codeseg"))
            {
                output_is_code = 1;
                continue;
            }

            if(argmatch(arg, "spritearea"))
            {
                output_length_word = 1;
                continue;
            }

            if(argmatch(arg, "32bit"))
            {
                g_f32bit = 1;
                continue;
            }

            fprintf(stderr, "Unknown arg '-%s' given\n", arg);
        }
        else
        {
            /* args by position */

            if(!infilename)
            {
                infilename = arg;
                continue;
            }

            if(!outfilename)
            {
                outfilename = arg;
                continue;
            }

            if(!symbolname)
            {
                symbolname = arg;
                continue;
            }

            puts("extra args --- ignored");
            break;
        }
    }

    if(!infilename)
    {
        fprintf(stderr, "No input file defined\n");
        return(EXIT_FAILURE);
    }

    if(!outfilename)
    {
        fprintf(stderr, "No output file defined\n");
        return(EXIT_FAILURE);
    }

    if(!symbolname)
    {
        fprintf(stderr, "No symbol defined\n");
        return(EXIT_FAILURE);
    }

    {
    FILE * fin = fopen(infilename, "rb");

    if(fin)
    {
        FILE * fout = fopen(outfilename, "wb");

        if(fout)
        {
            STATUS status = make_aof_file(fin, fout, symbolname);

            (void) fclose(fout);

            if(status >= 0)
            {
                char buffer[512];
                strcpy(buffer, "settype ");
                strcat(buffer, outfilename);
                strcat(buffer, " aof");
                (void) _kernel_oscli(buffer);
            }
            else
                remove(outfilename);

            res = (status < 0) ? EXIT_FAILURE : EXIT_SUCCESS;
        }
        else
        {
            fprintf(stderr, "Can't open output %s\n", infilename);
            res = EXIT_FAILURE;
        }

        (void) fclose(fin);
    }
    else
    {
        fprintf(stderr, "Can't open input %s\n", infilename);
        res = EXIT_FAILURE;
    }
    } /*block*/

    return(res);
}

/* end of MakeAOF.c */
