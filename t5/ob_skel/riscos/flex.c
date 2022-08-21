/************************************************************************/
/* © Acorn Computers Ltd, 1992.                                         */
/*                                                                      */
/* This file forms part of an unsupported source release of RISC_OSLib. */
/*                                                                      */
/* It may be freely used to create executable images for saleable       */
/* products but cannot be sold in source form or as an object library   */
/* without the prior written consent of Acorn Computers Ltd.            */
/*                                                                      */
/* If this file is re-distributed (even if modified) it should retain   */
/* this copyright notice.                                               */
/*                                                                      */
/************************************************************************/

/* Title: c.flex
 * Purpose: provide memory allocation for interactive programs requiring
 *          large chunks of store.
 * History: IDJ: 06-Feb-92: prepared for source release
 */

#if RISCOS

/* This implementation goes above the original value of GetEnv,
to memory specifically requested from the Wimp. The heap is kept
totally compacted all the time, with pages being given back to
the Wimp whenever possible. */

typedef struct _flex__rec
{
    flex_ptr anchor;    /* *(p->anchor) should point back to this actual core (p+1) */
    int      size;      /* Exact size of contained object (bytes) */
    /* actual store follows (p+1) */
}
flex__rec;

/* From start upwards, it's divided into store blocks of
 *   a flex__rec
 *   object bytes
 *   align up to next word.
*/

static BOOL
flex__ensure(
    _In_        int required)
{
    int top = (int) flex_.limit;
    int more;

    /* can the request be satisfied already? */
    more = required - (top - (int) flex_.freep);

    if(more <= 0)
        return(TRUE);

    /*nd 15jul96 try the allocation anyway and see if it fails - faster and works correctly with Virtualise */
    top += more;
    if(NULL == flex__area_change(&top))
    {
        flex_.limit = (char *) top;
        trace_1(TRACE_MODULE_ALLOC, TEXT("flex__ensure: top out: ") PTR_XTFMT, flex_.limit);
    }

    /* can the request be satisfied now? */
    more = required - (top - (int) flex_.freep);

    if(more <= 0)
        return(TRUE);

    return(FALSE);
}

static void
flex__give(void)
{
    int mask = (flex_granularity - 1); /* pagesize is a power of 2 */
    int lim = (mask + (int) flex_.limit) & ~mask;
    int fre = (mask + (int) flex_.freep) & ~mask;

    trace_2(TRACE_MODULE_ALLOC, TEXT("flex__give() flex_.limit: ") PTR_XTFMT TEXT(", flex_.freep: ") PTR_XTFMT, flex_.limit, flex_.freep);

    if(lim > (fre + flex_granularity /*SKS 04oct95 add a little hysteresis*/))
    {
        (void) flex__area_change((int *) &flex_.limit);

        trace_1(TRACE_MODULE_ALLOC, TEXT("flex__give: slot out: ") PTR_XTFMT, flex_.limit);
    }
}

extern BOOL
flex_alloc(
    flex_ptr anchor,
    int n)
{
    flex__rec * p;
    int required = sizeof32(flex__rec) + flex_roundup(n);

    trace_2(TRACE_MODULE_ALLOC, TEXT("flex_alloc(") PTR_XTFMT TEXT(", %d)"), anchor, n);

    if((n < 0)  ||  !flex__ensure(required))
    {
        *anchor = NULL;
        trace_1(TRACE_OUT | TRACE_MODULE_ALLOC, TEXT("flex_alloc(%d) yields NULL"), n);
        return(FALSE);
    }

    /* allocate at end of memory */
    p = (flex__rec *) (void *) flex_.freep;

    flex_.freep = flex_.freep + required;

    p->anchor = anchor;
    p->size   = n;              /* store requested amount, not allocated amount */

    *anchor   = flex_innards(p);     /* point to punter's part of allocated object */

    trace_1(TRACE_MODULE_ALLOC, TEXT("flex_alloc yields ") PTR_XTFMT, *anchor);
    return(TRUE);
}

static void
flex__reanchor(
    flex__rec * startp,
    _In_        int by)
{
    flex__rec * p = startp;

    /* Move all the anchors from p upwards. This is in anticipation
     * of that block of the heap being shifted.
    */
    trace_3(TRACE_MODULE_ALLOC, TEXT("flex__reanchor(") PTR_XTFMT TEXT(", by = %d): flex_.freep = ") PTR_XTFMT, p, by, flex_.freep);

    while(p < (flex__rec *) (void *) flex_.freep)
    {
#if TRACE_ALLOWED
        if(by)
            trace_3(TRACE_MODULE_ALLOC, TEXT("reanchoring object ") PTR_XTFMT TEXT(" (") PTR_XTFMT TEXT(") to ") PTR_XTFMT TEXT("  "),
                    flex_innards(p), p->anchor, (char *) flex_innards(p) + by);
#endif

#if CHECKING
        /* check current registration */
        if(*(p->anchor) != flex_innards(p))
        {
            if(__myasserted(TEXT("flex__reanchor"), TEXT(__FILE__), __LINE__,
                            TEXT("p->anchor ") PTR_XTFMT TEXT(" *(p->anchor) ") PTR_XTFMT TEXT(" != object ") PTR_XTFMT TEXT(" (motion %d)"),
                            p->anchor, *(p->anchor), flex_innards(p), by))
                __crash_and_burn_here();
        }
#endif

        /* point anchor to where block will be moved */
        *(p->anchor) = (char *) flex_innards(p) + by;

        /* does anchor needs moving (is it above the reanchor start and in the flex area?) */
        if(((char *) p->anchor >= (char *) startp)  &&  ((char *) p->anchor < flex_.freep))
        {
            trace_3(TRACE_MODULE_ALLOC, TEXT("moving anchor for object ") PTR_XTFMT TEXT(" from ") PTR_XTFMT TEXT(" to ") PTR_XTFMT TEXT("  "),
                    flex_innards(p), p->anchor, (char *) p->anchor + by);
            p->anchor = (flex_ptr) (void *) ((char *) p->anchor + by);
        }

        p = flex_next_block(p);
    }
}

extern void
flex_free(
    flex_ptr anchor)
{
    flex__rec * p = (flex__rec *) *anchor;
    flex__rec * next;
    int nbytes_above;
    int blksize;

    trace_3(TRACE_MODULE_ALLOC, TEXT("flex_free(") PTR_XTFMT TEXT(" -> ") PTR_XTFMT TEXT(" (size %d))"), anchor, p, flex_size(anchor));

    if(!p--)
        return;

    next = flex_next_block(p);
    nbytes_above = flex_.freep - (char *) next;
    blksize = sizeof32(flex__rec) + flex_roundup(p->size);

    if(nbytes_above)
    {
        flex__reanchor(next, -blksize);

        memmove32(p, next, nbytes_above);
    }

    flex_.freep = flex_.freep - blksize;

    *anchor = NULL;

    flex__give();
}

extern int
flex_extend(
    flex_ptr anchor,
    int newsize)
{
    int cursize = flex_size(anchor);

    return(flex_midextend(anchor, cursize, newsize - cursize));
}

extern BOOL
flex_midextend(
    flex_ptr anchor,
    int at,
    int by)
{
    flex__rec * p = ((flex__rec *) *anchor) - 1;
    flex__rec * next;
    int growth, shrinkage;

    trace_4(TRACE_MODULE_ALLOC, TEXT("flex_midextend(") PTR_XTFMT TEXT(" -> ") PTR_XTFMT TEXT(", at = %d, by = %d)"), anchor, p, at, by);

    if(by > 0)
    {
        /* Amount by which the block will actually grow. */
        growth = flex_roundup(p->size + by) - flex_roundup(p->size);

        /* subsequent block motion might not be needed for small extensions */
        if(growth)
        {
            if(!flex__ensure(growth))
                return(FALSE);

            next = flex_next_block(p);

            /* The move has to happen in two parts because the moving
             * of objects above is word-aligned, while the extension within
             * the object may not be.
            */

            /* move subsequent blocks up to new position */
            flex__reanchor(next, growth);

            memmove32(((char *) next) + flex_roundup(growth),
                      next,
                      flex_.freep - (char *) next);
        }

        /* move end of this object upwards */
        memmove32((char *) flex_innards(p) + at + by,
                  (char *) flex_innards(p) + at,
                  p->size - at);

        flex_.freep = flex_.freep + growth;

        p->size = p->size + by;
    }
    else if(by < 0)
    {
        /* Amount by which the block will actually shrink. */
        shrinkage = flex_roundup(p->size) - flex_roundup(p->size + by);
        /* a positive value */

        /* subsequent block motion might not be needed for small shrinkages */
        if(shrinkage)
            next = flex_next_block(p);
        else
            next = NULL; /* keep complier dataflow analyser happy */

        /* move end of this block downwards */
        memmove32((char *) flex_innards(p) + at + by,
                  (char *) flex_innards(p) + at,
                  p->size - at);

        p->size = p->size + by;

        if(shrinkage)
        {
            /* move subsequent blocks down to new position */
            flex__reanchor(next, - shrinkage);

            memmove32(((char *) next) - shrinkage,
                      next,
                      flex_.freep - (char *) next);

            flex_.freep = flex_.freep - shrinkage;

            flex__give();
        }
    }

    return(TRUE);
}

extern int
flex_size(
    flex_ptr anchor)
{
    flex__rec * p = (flex__rec *) *anchor;

    if(!p--)
        return(0);

    return(p->size);
}

#endif /* RISCOS */

/* end of flex.c */
