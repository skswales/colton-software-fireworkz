/* MakeMess.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1990-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Create a file containing pre-processed messages
 * suitable for MakeAOF or Fireworkz resource.c
 *
 * Stuart K. Swales 17-Jul-1990
 *
 * History:
 * 0.01 17-Jul-1990 SKS created
 * 0.10 11-Apr-1994 SKS made it output messages conditional on RISCOS/WINDOWS
 * 0.11 19-Sep-1994 SKS made it not identify comments that don't start at start of line
 * 0.12 09-Jan-2012 SKS default country string UK, fix comments
 * 0.13 13-Feb-2014 SKS MPL-ed
 * 0.14 17-Nov-2016 SKS Allow LF separator
 * 0.15 21-Aug-2019 SKS fprintf(stderr)
 * 0.16 20-May-2020 SKS (country) check back in concordance with calling Makefiles
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "kernel.h"

#include <assert.h>

/*
exported functions
*/

extern int
main(int argc, char *argv[]);

/*
internal functions
*/

static void
give_help_on_app(void);

static int
make_file_from_messages(FILE * fin, FILE * fout);

static int
messages_init(FILE * fin);

/* ----------------------------------------------------------------------- */

#define VERSION "0.16"

#define ARG_STROP_CHAR '-'
#define ARG_STROP_STR  "-"

#define NUL           '\0'
#define LF            10
#define CR            13
#define COMMENT_CH    '#'
#define TAG_DELIMITER ':'

#define TRACE 0

#define ERR_NOROOM        -3
#define ERR_NOTALLREAD    -4
#define ERR_INPUTTOOSMALL -5

static char *messages_block = NULL;

static int
g_lf_sep = 0;

static char
country_string[16] = "UK";

static int
messages_init(FILE * fin)
{
    size_t real_length;
    size_t alloc_length, new_length;
    const char *in;
    char *out;
    #if TRACE
    const char *start;
    #endif
    const char *end;
    int ch, lastch;
    int res;
    int new_line = 1;

    {
    long l_res;
    (void) fseek(fin,0L,SEEK_END);
    l_res = ftell(fin);
    (void) fseek(fin,0L,SEEK_SET);
    if(l_res <= 0)
        return((int) l_res);
    real_length = (size_t) l_res;
    }

    /* add 1 for terminating NUL if messages file doesn't have trailing linesep and 1 for NUL for end of list */
    alloc_length = real_length + 2;

    /* tracef1("[messages_init: length = %d]\n", alloc_length);  */
    messages_block = malloc(alloc_length);
    /* tracef1("[messages_init: messages_block now &%p]\n", messages_block);   */

    if(!messages_block)
        return(ERR_NOROOM);

    if((res = fread(messages_block, 1, real_length, fin)) != real_length)
    {
        if(res >= 0)
            res = ERR_NOTALLREAD;

        return(res);
    }

    /* loop over loaded messages: end -> real end of messages */
    in = messages_block;
    out = messages_block;
    #if TRACE
    start = messages_block;
    #endif
    end = in + real_length;
    lastch = NUL;

    do  {
        ch = (in != end) ? *in++ : LF;

        if((ch == COMMENT_CH) && new_line)
        {
            /* tracef1("[messages_init: found comment at &%p]\n", in - 1); */
            do
                ch = (in != end) ? *in++ : LF;
            while((ch != LF)  &&  (ch != CR));

            if(in == end)
                break;

            lastch = NUL;
        }

        if((ch == LF)  ||  (ch == CR))
        {
            if((ch ^ lastch) == (LF ^ CR))
            {
                /* tracef0("[messages_init: just got the second of a pair of LF,CR or CR,LF - NUL already placed so loop]\n"); */
                lastch = NUL;
                continue;
            }

            /* tracef1("[messages_init: got a line terminator at &%p - place a NUL]\n", in - 1); */
            lastch = ch;
            ch = NUL;
        }
        else
            lastch = ch;

        /* don't place two NULs together or one at the start (this also means you can have blank lines) */
        if(!ch)
        {
            /* tracef1("[messages_init: placing NUL at &%p]\n", out); */
            if((out == messages_block)  ||  !*(out - 1))
                continue;
            #if TRACE
            *out = ch;
            /* tracef1("[messages_init: ended line '%s']\n", start); */
            start = out + 1;
            #endif
            new_line = 1;
        }
        else
        {
            if(new_line)
            {
                /* first chars now conditional country string */
                if(ch == '(')
                {
                    const size_t csl = strlen(country_string);

                    if( (')' != in[csl]) || (0 != strncmp(in, country_string, csl)) )
                    {
                        /*fprintf(stderr, "rejects %10s", in);*/

                        do
                            ch = (in != end) ? *in++ : LF;   
                        while((ch != LF)  &&  (ch != CR));

                        if(in == end)
                            break;
                        lastch = NUL;
                        continue;
                    }

                    in += csl + 2 /* leading and trailing brackets */;
                    continue;
                }

                /* first char now conditional RISCOS/WINDOWS */
                if(ch == 'w')
                {
                    do
                        ch = (in != end) ? *in++ : LF;   
                    while((ch != LF)  &&  (ch != CR));

                    if(in == end)
                        break;
                    lastch = NUL;
                    continue;
                } 

                new_line = 0;

                if(ch == 'r')
                    continue;
            }
        }

        *out++ = ch;
    }
    while(in != end);

    /* tracef1("[messages_init: placing last NUL at &%p]\n", out); */
    *out++ = NUL; /* need this last byte as end marker */

    new_length = out - messages_block;
    /* tracef2("[messages_init: new_length = %d, length = %d]\n", new_length, alloc_length);  */

    return((int) new_length); /* length of all strings plus end marker */
}

static int
messages_write(size_t msgs_length, FILE * fout)
{
    int res;

    if(g_lf_sep)
    {   /* write one at a time, separated by LF, no terminating NUL */
        const char * tag_and_message = messages_block;

        for(;;)
        {
            const size_t length = strlen(tag_and_message);

            if(0 == length)
                break;

            res = fwrite(tag_and_message, 1, length, fout);

            if(res < 0)
                return(res);

            res = fputc(LF, fout);

            if(res < 0)
                return(res);

            tag_and_message += length + 1; /* skip tag:message and NUL */
        }

        res = 0;
    }
    else
    {   /* write entire block, including terminating NUL */
        res = fwrite(messages_block, 1, msgs_length, fout);
    }

    return(0);
}

static int
make_file_from_messages(FILE * fin, FILE * fout)
{
    int res;
    int res32;

    /* tracef2("[make_file_from_messages(&%p, &%p)]\n", fin, fout); */

    res32 = messages_init(fin);

    if(res32 > 0)
        res = messages_write((size_t) res32, fout);
    else if(res32 == 0)
        res = ERR_INPUTTOOSMALL;
    else
        res = (int) res32;

    return(res);
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
    puts("Colton Software MakeMess vsn " VERSION " [" __DATE__ "]");
    puts("\nMakeMess [-from] infile [-to] outfile [-lf]");
}

extern int
main(int argc, char *argv[])
{
    const char * infilename  = NULL;
    const char * outfilename = NULL;
    FILE * fin;
    FILE * fout;
    int res, res1;
    int argi = 0;
    const char * arg;

    while(++argi < argc)
    {
        arg = argv[argi];

        /* tracef2("[main: decoding arg %d, '%s']\n", argi, arg); */

        if(*arg == ARG_STROP_CHAR)
        {
            ++arg;

            if(argmatch(arg, "help"))
            {
                give_help_on_app();
                return(EXIT_SUCCESS);
            }

            else if(argmatch(arg, "from"))
            {
                infilename = argv[++argi];
                continue;
            }
            else if(argmatch(arg, "o") || argmatch(arg, "to"))
            {
                outfilename = argv[++argi];
                continue;
            }

            else if(argmatch(arg, "lf"))
            {
                g_lf_sep = 1;
                continue;
            }

            else if(argmatch(arg, "country"))
            {
                strcpy(country_string, argv[++argi]);
                continue;
            }

            fprintf(stderr, "%s: unrecognized arg " ARG_STROP_STR "%s --- ignored\n", argv[0], arg);
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

            fprintf(stderr, "%s: extra args --- ignored\n", argv[0]);
            break;
        }
    }

    if(!infilename)
    {
        fprintf(stderr, "%s: no input file specified\n", argv[0]);
        return(EXIT_FAILURE);
    }

    if(!outfilename)
    {
        fprintf(stderr, "%s: no output file specified\n", argv[0]);
        return(EXIT_FAILURE);
    }

    /* tracef1("[main: infilename  %s]\n", infilename);   */
    /* tracef1("[main: outfilename %s]\n", outfilename);  */

res = EXIT_FAILURE ;

    fin = fopen(infilename, "rb");
    if(fin)
    {
        fout = fopen(outfilename, "wb");
        if(fout)
        {
            res = make_file_from_messages(fin, fout);
            res1 = fclose(fout);
            if(res1 < 0)
                res = res1;
            if(res >= 0)
            {
                if(g_lf_sep)
                {
                    char command_buffer[32 + 256];
                    (void) sprintf(command_buffer, "SetType %s Text", outfilename);
                    (void) _kernel_oscli(command_buffer);
                }
            }
            else
            {
                remove(outfilename);
            }
            res = (res < 0) ? EXIT_FAILURE : EXIT_SUCCESS;
        }
        else
        {
            fprintf(stderr, "%s: can't open output file %s: %s\n", argv[0], outfilename, strerror(errno));
            res = EXIT_FAILURE;
        }

        fclose(fin);
    }
    else
    {
        fprintf(stderr, "%s: can't open input file %s: %s\n", argv[0], infilename, strerror(errno));
        res = EXIT_FAILURE;
    }

    return(res);
}

/* end of MakeMess.c */
