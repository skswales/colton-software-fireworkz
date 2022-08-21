/* guess.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1992-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Spelling guesser for Wordz */

/* RCM November 1992 */

#include "common/gflags.h"

#include "ob_spell/ob_spell.h"

static const PC_U8Z
special[] =
{
    "ance ance/ence",
    "ence ance/ence",
    "iour ur/our/iour",
    "ious us ous/ious",
     "ail ail/ale",
     "ale ail/ale",
     "ant ant/ent",
     "ent ant/ent",
     "ies ies/ys",
     "our ur/our/iour",
     "ous us/ous/ious",
      "cc cc/c/ss/s",
      "er ar/er/re",
      "ll l/ll",
      "ie ie/ee",
      "oo o/oo",
      "re ar/er/re",
      "ss ss/s/cc/c",
      "tt t/tt",
      "ur ur/our/iour",
      "us us/ous/ious/uc/ucc/uss",  /* 3 endings & 3 middles */
      "wh h/w/wh",
      "ys ys/ies",
       "c c/cc/s/ss",
       "h h/w/wh",
       "k c/k",
       "l l/ll",
       "o o/oo",
       "s s/ss/c/cc",
       "t t/tt",
       "w h/w/wh",
       "x ex/x",
        ""
};

typedef struct GUESS_REC
{
    SBCHARZ word[BUF_MAX_WORD];

    S32  method;
    S32  posn;
    U8   cycl;
    U8   _spare[3];

    PC_U8Z remap[BUF_MAX_WORD];
    S32  index[BUF_MAX_WORD];
    S32  last;
}
GUESS_REC;

static GUESS_REC * p_rcm_template;

extern void
guess_end(void)
{
    al_ptr_dispose(P_P_ANY_PEDANTIC(&p_rcm_template));
}

/******************************************************************************
*
* returns
*   =1 ok
*   =0 can't make up any alternative spellings
*   <0 error
*
******************************************************************************/

_Check_return_
extern STATUS
guess_init(
    _In_z_      PC_USTR word)
{
    STATUS status;
    S32 i = 0;
    S32 j, k;
    S32 found;

    PC_USTR src = word;
    const U32 elemof_buffer = elemof32(p_rcm_template->word);
    U32 dstidx = 0;

    if(NULL == (p_rcm_template = al_ptr_alloc_elem(GUESS_REC, 1, &status)))
        return(status);

    while((CH_NULL != PtrGetByte(src)) && (dstidx < elemof_buffer))
    {
        U8 ch = PtrGetByte(src);
        ustr_IncByte(src);
        ch = (U8) ob_spell_tolower(ch);
        p_rcm_template->word[dstidx++] = ch;
    }

    p_rcm_template->word[dstidx] = CH_NULL;

    p_rcm_template->method = GUESS_INIT;

    while(CH_NULL != PtrGetByteOff(word, i))
    {
        found = FALSE;
        p_rcm_template->last = i;

        for(j = 0; *special[j]; j++)
        {
            k = 0;

            while(PtrGetByteOff(word, i+k) == special[j][k])
            {
                k++;

                if(special[j][k] == CH_SPACE)
                {
                    found = TRUE;
                    p_rcm_template->remap[i]   = special[j];
                    p_rcm_template->index[i++] = k + 1;        /* ie >= 0 */

                    while(--k)
                        p_rcm_template->index[i++] = GUESS_SKIP;

                    break;
                }
            }

            if(found)
                break;
        }

        if(!found)
            p_rcm_template->index[i++] = GUESS_CHAR;
    }

#if TRUE
    {
    /*S32 i;*/
    S32 newln;

    trace_1(TRACE_APP_TYPE5_SPELL, TEXT("guess_init('%s')"), report_ustr(word));
    newln = TRUE;

    for(i = 0; CH_NULL != p_rcm_template->word[i]; i++)
    {
        switch(p_rcm_template->index[i])
        {
        case GUESS_CHAR:
            trace_1(TRACE_APP_TYPE5_SPELL, TEXT("|'%c'|"), p_rcm_template->word[i]);
            newln = FALSE;
            break;

        default:
            if(!newln)
                trace_0(TRACE_APP_TYPE5_SPELL, TEXT(""));

            trace_1(TRACE_APP_TYPE5_SPELL, TEXT("|'%s'"), report_sbstr(p_rcm_template->remap[i]));
            newln = TRUE;
            break;

        case GUESS_SKIP:
            break;
        }
    }

    if(!newln)
        trace_0(TRACE_APP_TYPE5_SPELL, TEXT(""));
    } /*block*/
#endif

    return(0);
}

/******************************************************************************
*
* get next guess
*
* returns
*   =2 ok, and word in dictionary
*   =1 ok, but word not in dictionary
*   =0 no more words
*   <0 error
*
******************************************************************************/

_Check_return_
extern STATUS
guess_next(
    _DocuRef_   P_DOCU p_docu,
    /*out*/ P_U8Z word,
    _InVal_     S32 sizeof_word)
{
    STATUS status;

    /*>>>DOUBLE & REPLACE may cause buffer to overflow*/

    *word = CH_NULL;       /* makes life easier elsewhere */

    switch(p_rcm_template->method)
    {
#if TRUE
        case GUESS_INIT:
            p_rcm_template->method = GUESS_CYCLE;
            p_rcm_template->posn   = strlen32(p_rcm_template->word) - 1;
            p_rcm_template->cycl   = 'a';

            /*FALLTHRU*/

        case GUESS_CYCLE:
            if((p_rcm_template->posn >= 0) && (p_rcm_template->cycl > 'z'))
            {
                p_rcm_template->cycl = 'a';
                p_rcm_template->posn--;
            }

            if(p_rcm_template->posn >= 0)
            {
                xstrkpy(word, sizeof_word, p_rcm_template->word);
                word[p_rcm_template->posn] = p_rcm_template->cycl++;

                break;          /* got another word */
            }

            p_rcm_template->method = GUESS_DOUBLE;
            p_rcm_template->posn   = strlen32(p_rcm_template->word);
            p_rcm_template->cycl   = 'a';

            /*FALLTHRU*/

        case GUESS_DOUBLE:
            if((p_rcm_template->posn >= 0) && (p_rcm_template->cycl > 'z'))
            {
                p_rcm_template->cycl = 'a';
                p_rcm_template->posn--;
            }

            if(p_rcm_template->posn >= 0)
            {
                xstrkpy(word, sizeof_word, p_rcm_template->word);
                xstrkpy(&word[p_rcm_template->posn + 1],
                                sizeof_word - (p_rcm_template->posn + 1),
                                &p_rcm_template->word[p_rcm_template->posn]);
                word[p_rcm_template->posn] = p_rcm_template->cycl++;

                break;          /* got another word */
            }

#else
        case GUESS_DOUBLE:
            if(p_rcm_template->posn >= 0)
            {
                S32 posn = p_rcm_template->posn--;

                xstrkpy(word, sizeof_word, p_rcm_template->word);
                xstrkpy(&word[posn + 1], sizeof_word - (posn + 1), &p_rcm_template->word[posn]);

                break;          /* got another word */
            }
#endif
            p_rcm_template->method = GUESS_REMOVE;
            p_rcm_template->posn   = strlen32(p_rcm_template->word) - 1;

            /*FALLTHRU*/

        case GUESS_REMOVE:
            if(p_rcm_template->posn >= 0)
            {
                S32 posn = p_rcm_template->posn--;

                xstrkpy(word, sizeof_word, p_rcm_template->word);
                xstrkpy(&word[posn], sizeof_word - posn, &p_rcm_template->word[posn + 1]);

                if(*word)
                    break;      /* got another word */
            }

            p_rcm_template->method = GUESS_SWAP;
            p_rcm_template->posn   = strlen32(p_rcm_template->word) - 1;

            /*FALLTHRU*/

        case GUESS_SWAP:
            if(p_rcm_template->posn > 0)
            {
                S32 posn = p_rcm_template->posn--;

                xstrkpy(word, sizeof_word, p_rcm_template->word);
                word[posn - 1] = p_rcm_template->word[posn];
                word[posn] = p_rcm_template->word[posn - 1];

                break;          /* got another word */
            }

            p_rcm_template->method = GUESS_REPLACE;

            /*FALLTHRU*/

        case GUESS_REPLACE:
            if(p_rcm_template->last >= 0)
            {
                S32  i, j, w;
                PC_U8Z remap;

                /* construct guess */
                w = 0;
                for(i = 0; p_rcm_template->word[i]; i++)
                {
                    switch(p_rcm_template->index[i])
                    {
                    case GUESS_CHAR:
                        word[w++] = p_rcm_template->word[i];
                        break;

                    default:
                        j = p_rcm_template->index[i];
                        remap = p_rcm_template->remap[i];
                        while(remap[j] && (remap[j] != CH_FORWARDS_SLASH))
                            word[w++] = remap[j++];
                        break;

                    case GUESS_SKIP:
                        break;
                    }
                }
                word[w] = CH_NULL;

                /* prepare for next guess */
                i = p_rcm_template->last;

                while(i >= 0)
                {
                    if(p_rcm_template->index[i] >= GUESS_REMAP)
                    {
                        remap = p_rcm_template->remap[i];
                        j     = p_rcm_template->index[i];

                        while(remap[j] && (remap[j] != CH_FORWARDS_SLASH))
                            j++;

                        if(remap[j])
                        {
                            p_rcm_template->index[i] = j + 1;

                            break;  /* i >= 0 means got new word to try */
                        }

                        /* reached end of remap string */

                        j = 0;
                        while(remap[j++] != CH_SPACE)
                        { /*EMPTY*/ }

                        p_rcm_template->index[i] = j;
                    }

                    i--;
                }

                /* i>=0 means got new word, i<0 no more words */
                if(i < 0)
                    p_rcm_template->last = -1;

                break;  /* got new word */
            }

            p_rcm_template->method = GUESS_FINISHED;

            /*FALLTHRU*/

        default:
        case GUESS_FINISHED:
            return(0);
    }

    /* got a word, now see if its in the dictionary */

    if((status = ob_spell_checkword(p_docu, (PC_USTR) word, FALSE, TRUE, TRUE)) > 0) /* don't search skip list, search all dictionaries */
    {
        ob_spell_setcase(word, (S32) status);
        return(2);  /* its in the dictionary */
    }

    if(SPELL_ERR_BADWORD == status)
        status = STATUS_OK;

    status_return(status);

    return(STATUS_DONE);
}

/* end of guess.c */
