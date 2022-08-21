/* callback.c */

/* Copyright (C) 1994-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*
Routines provided so that DPlib can call us back to
get its hands on the arguments which it requires.
*/

#include "common/gflags.h"

#include "ob_rec/ob_rec.h"

#include "_txtframe.h" /* for frame_play() */

#define flink_item(__u32) ( \
    (frameslink) (0xCC000000U | (U32) (__u32)) ) /* compose a pointer that would barf if accidentally dereferenced */

#define deflink(__flink) ( \
    0x00FFFFFFU & (U32) (__flink) )

#define flink_first() \
    flink_item(1U)

#define flink_next(__flink) \
    flink_item(deflink(__flink) + 1U)

/*
Bloody stupid global variable
*/

static P_OPENDB p_opendb_for_getting_at_things;

extern void
set_p_opendb_for_getting_at_things(
    P_OPENDB p_opendb)
{
    p_opendb_for_getting_at_things = p_opendb;
}

/*
 - Exported for use by DPlib (defined in "_txtframe.h")
 - 'Play' a file in response to the PLAY command being executed.
 - You may want to leave this doing nothing!
*/

os_error *
frame_play(
    os_filetype filetype,
    char * filename)
{
    IGNOREPARM(filetype);
    IGNOREPARM(filename);
    return(NULL);
}

/*
 - Exported for use by DPlib (defined in "framesflds.h")
 - Routines to gain access to fields via their containing 'frames'.
 - Caller passes a 'framesptr' to (eg) search_buildexpr, which then passes it on to here.
 - The framesptr is simply a handle which is used to communicate with these routines.
*/

fieldsptr
frames_fields(
    framesptr frames)
{
    P_OPENDB p_opendb = (P_OPENDB) frames;

    profile_ensure_frame();

    return(p_opendb ? p_opendb->table.h_fields : NULL);
}

/*
 - Exported for use by DPlib (defined in "framesflds.h")
 - Find field in the layout with the given name.
 - Use frames_grungeidentifier() on the fieldnames before comparing them.
*/

fieldptr
frames_findfield(
    framesptr frames,
    char * fieldname)
{
    P_OPENDB p_opendb = (P_OPENDB) frames;

    profile_ensure_frame();

    return(p_opendb ? fields_findfield(p_opendb->table.h_fields, fieldname) : NULL);
}

/*
 - Exported for use by DPlib (defined in "framesflds.h")
 - Return name of a field in the current layout.
 - If the field name is left unspecified, this should be taken from the field title.
 */

char *
frames_fieldname(
    framesptr frames,
    fieldptr field)
{
    profile_ensure_frame();

    return(frames ? field_name(field) : NULL);
}

/*
 - Exported for use by DPlib (defined in "framesflds.h")
 - Return fields in link_entry order (called by db-remap).
 - Returns fields from field list instead if no frames in layout (for CSV and Tab files).

 PMF says...Oh No! it wants me to RETURN a pointer to a frame as well

 Well I sharn't!

 Use a weird form of fieldcounter
*/

fieldptr
frames_nextfield(
    framesptr frames,
    frameslink * flinkp,
    fieldptr * fieldp)
{
    P_OPENDB p_opendb = (P_OPENDB) frames;
    frameslink flink = *flinkp;
    fieldptr field = *fieldp;

    if(NULL == p_opendb)
        return(NULL);

    if(flink != FLINK_FINISHED)
    {
        P_FIELDDEF p_fielddef;
        STATUS status = STATUS_OK;

        if(NULL == flink)
            /* clear all marks in the field set */
            fields_setflags(p_opendb->table.h_fields, (fieldflags) 0, fflag_Mark3);

        if(NULL == flink)
            flink = flink_first(); /* the first one */
        else
            flink = flink_next(flink); /* the next one */

        if(P_DATA_NONE == (p_fielddef = p_fielddef_from_number(&p_opendb->table.h_fielddefs, deflink(flink))))
            status = STATUS_FAIL;

        while((status_ok(status)) && (field_getflags(p_fielddef->p_field) & fflag_Mark3))
        {
            flink = flink_next(flink);

            if(P_DATA_NONE == (p_fielddef = p_fielddef_from_number(&p_opendb->table.h_fielddefs, deflink(flink))))
                status = STATUS_FAIL;
        }

        if(status_ok(status))
            *flinkp = flink;
        else
            *flinkp = FLINK_FINISHED;

        if(status_ok(status))
        {
            /* mark so it won't be returned again */
            field_setflags(p_fielddef->p_field, fflag_Mark3, fflag_Mark3);
            return(p_fielddef->p_field);
        }
    }

    if(field != FIELD_FINISHED)
    {
        field = (NULL == field) ? fields_head(p_opendb->table.h_fields) : field_next(field);

        while(field && (field_getflags(field) & (/* fflag_Surrogate | */ fflag_Mark3)))
             field = field_next(field);

        *fieldp = field ? field : FIELD_FINISHED;
        return(field);
    }

    return(NULL);
}

/*
 - Exported for use by DPlib (defined by us in "dplib.h" as per "extras.c" example)
 - Return fields in link_entry order (called by search_buildexpr).
 - Only returns fields which are visible in the set of frames.
 */

fieldptr
frames_nextfield_inlayout(
    framesptr frames,
    frameslink * flinkp)
{
    P_OPENDB p_opendb = (P_OPENDB) frames;
    frameslink flink = *flinkp;
    P_FIELDDEF p_fielddef;

    if(NULL == p_opendb)
        return(NULL);

    if(flink == FLINK_FINISHED)
        return(NULL);

    /* If first Start at the frames head ptr frame, else start at next frame */
    if(NULL == flink)
        flink = flink_first(); /* the first one */
    else
        flink = flink_next(flink); /* the next one */

    /* Skip any non field holding frames */
    if(P_DATA_NONE == (p_fielddef = p_fielddef_from_number(&p_opendb->table.h_fielddefs, deflink(flink))))
    {
        *flinkp = FLINK_FINISHED;
        return(NULL);
    }

    while(NULL == p_fielddef->p_field) /* Bloody unlikely to be null! */
    {
        flink = flink_next(flink);

        if(P_DATA_NONE == (p_fielddef = p_fielddef_from_number(&p_opendb->table.h_fielddefs, deflink(flink))))
        {
            *flinkp = FLINK_FINISHED;
            return(NULL);
        }
    }

    *flinkp = flink;

    return(p_fielddef->p_field);
}

/*
 - Exported for use by DPlib (defined in "framesflds.h")
 - Called by search_buildexpr to see what text is in a given field of the layout
*/

os_filetype
frames_readtext(
    frameslink flink,
    char *** resultp,
    int * sizep)
{
    P_OPENDB p_opendb = p_opendb_for_getting_at_things;

    if((NULL != flink) && (flink != FLINK_FINISHED) && (NULL != p_opendb))
    {
        P_FIELDDEF p_fielddef = p_fielddef_from_number(&p_opendb->table.h_fielddefs, deflink(flink));

        if(P_DATA_NONE != p_fielddef)
        {
            P_QUERY p_query = p_query_from_p_opendb(p_opendb, p_opendb->search.query_id);
            P_SEARCH_FIELD_PATTERN p_search_pattern = array_base(&p_query->h_search_pattern, SEARCH_FIELD_PATTERN);

            p_search_pattern += (deflink(flink)-1); /* it is an array of them */

            assert(p_search_pattern->field_id == p_fielddef->id);

            {
            static char * t_ptr;
            t_ptr = array_base(&p_search_pattern->h_text_ustr, char);
            *resultp = &t_ptr;
            } /*block*/

            *sizep = (int) (array_elements(&p_search_pattern->h_text_ustr)) - 1 /*remove CH_NULL*/;

            return(filetype_Text); /* Perhaps this should reflect the fielddef.type */
        }
    }

    return((os_filetype) 0);
}

/*
 - Exported for use by DPlib (defined by us in "dplib.h" as per "extras.c" example)
 - A field has gone wrong during a merge operation.
 - Prompt the user for a replacement value, with 3 possible values of *quietp on return:
 -              0 => try this value, but ask again if it goes wrong, or this field goes wrong later
 -              1 => try again 'quietly', ie. use this value on any subsequent errors in this field
 -              2 => give up (ie. cancel the merge, deleting any records which were imported
 */

os_error *
badfield_replace(
    char * fname,
    int cardn,
    char * errmess,
    BOOL * quietp,
    remapstash stash,
    char ** anchor,
    int offset,
    int size,
    char *** anchorp,
    int * offsetp,
    int * sizep)
{
    profile_ensure_frame();

    IGNOREPARM(sizep);
    IGNOREPARM(offsetp);
    IGNOREPARM(anchorp);
    IGNOREPARM(size);
    IGNOREPARM(stash);
    IGNOREPARM(cardn);
    IGNOREPARM(fname);
    IGNOREPARM(anchor);
    IGNOREPARM(offset);

    *quietp = 0; /* 0 => try again, 1 => try again quietly (ie. don't ask again), 2 => give up quietly */

    return((os_error *) (errmess - sizeof32(long))); /* oh you sickos */
}

/*---------------------------------------------------------*
 * Perform case-insensitive comparison (eg. for filenames) *
 *---------------------------------------------------------*/

#ifdef __CC_NORCROFT
#pragma no_check_stack
#endif

/*
 - Exported for use by DPlib (defined by us in "dplib.h" as per "extras.c" example)
*/

extern int
strcmp_nocase(
    /*const*/ char * ptr1,
    /*const*/ char * ptr2)
{
    while(*ptr1 && *ptr2)
    {
        int ch1 = *ptr1++;
        int ch2 = *ptr2++;
        int temp = ch1 - ch2;
        if(0 == temp)
            continue;
        /* retry with case folding */
        temp = /*"C"*/tolower(ch1) - /*"C"*/tolower(ch2); /* ASCII, no remapping */
        if(0 != temp)
            return(temp);
    }

    return(*ptr1 - *ptr2); /* either or both are CH_NULL */
}

/*
 - Exported for use by DPlib (defined by us in "dplib.h" as per "extras.c" example)
*/

extern int
strncmp_nocase(
    /*const*/ char *ptr1,
    /*const*/ char *ptr2,
    int maxlen)
{
    if(maxlen <= 0)
        return(0);

    while(*ptr1 && *ptr2)
    {
        int ch1 = *ptr1++;
        int ch2 = *ptr2++;
        int temp = ch1 - ch2;
        if(0 == temp)
            continue;
        /* retry with case folding */
        temp = /*"C"*/tolower(ch1) - /*"C"*/tolower(ch2); /* ASCII, no remapping */
        if(temp)
            return(temp);
        if(--maxlen <= 0)
            return(0);
    }

    return(*ptr1 - *ptr2); /* either or both are CH_NULL */
}

#ifdef __CC_NORCROFT
#pragma check_stack
#endif

extern const char * /* deprecated - only called by Neil's binaries */
msgs_lookup(
    const char * tag_and_default);

extern void /* deprecated - only called by Neil's binaries */
wimpt_reporterror(
    _kernel_oserror * e,
    int errflags);

_Check_return_
_Ret_maybenull_
extern _kernel_oserror * /* deprecated - only called by Neil's binaries */
wimpt_complain(_kernel_oserror * e);

extern void /* deprecated - only called by Neil's binaries */
werr(
    int fatal,
    const char *format,
    /**/        ...);

extern const char * /* deprecated - only called by Neil's binaries */
msgs_lookup(
    const char * tag_and_default) /* needed for Neil's interface to msgs via ob_rec */
{
    return(string_for_object(tag_and_default, OBJECT_ID_REC));
}

extern void /* deprecated - only called by Neil's binaries */
wimpt_reporterror(
    _kernel_oserror * e,
    int errflags)
{
    int errflags_out;

    (void) wimp_reporterror_rf(e, errflags, &errflags_out, "Database Engine", 2);
}

_Check_return_
_Ret_maybenull_
extern _kernel_oserror * /* deprecated - only called by Neil's binaries */
wimpt_complain(_kernel_oserror * e)
{
    if(NULL != e)
        wimpt_reporterror(e, 0);

    return(e);
}

static const U8Z
error_serious_str[] =
    "error_serious:%s has suffered a serious error (%s). "
    "Click Continue to exit immediately, losing data, Cancel to attempt to resume execution.";

static void
werr_fatal(
    _In_        const _kernel_oserror * const e)
{
    _kernel_oserror err;
    int jump_back;
    int errflags_out;

    host_must_die_set(TRUE); /* trap exceptions in lookup/sprintf etc */

    err.errnum = e->errnum;
    consume_int(snprintf(err.errmess, elemof32(err.errmess),
                         string_for_object(error_serious_str, OBJECT_ID_SKEL),
                         product_ui_id(),
                         e->errmess));

    consume(_kernel_oserror *, wimp_reporterror_rf(&err, Wimp_ReportError_OK | Wimp_ReportError_Cancel, &errflags_out, NULL, 3));
    jump_back = ((errflags_out & Wimp_ReportError_Cancel) != 0);

    if(jump_back)
        /* give it your best shot else we come back and die soon */
        host_longjmp_to_event_loop();

    exit(EXIT_FAILURE);
}

extern void /* deprecated - only called by Neil's binaries */
werr(
    int fatal,
    const char * format,
    /**/        ...)
{
    _kernel_oserror err;
    va_list va;

    err.errnum = 0;

    va_start(va, format);
    consume_int(vsnprintf(err.errmess, elemof32(err.errmess), format, va));
    va_end(va);

    trace_v0(TRACE_RISCOS_HOST, err.errmess);

    if(fatal)
        werr_fatal(&err);
    else
        (void) wimpt_complain(&err);
}

/*
 - Exported for use by DPlib (defined in "WindLibC:winderror.h")
*/

extern os_error *
wind_complain(
    os_error * e)
{
    /* why is Neil doing this */
    return(wimpt_complain(e));
}

/*
 - Exported for use by DPlib (defined in "WindLibC:windquery2.h")
 - Used for password entry if the user needs to enter one to gain access to the file.
 - windquery_doit() should open a temporary (menu) dbox, then poll the Window Manager until a choice is made or the window disappears:
 -      *result == 1 => user clicked on icon 1 (OK)
 -      *result == 2 => user clicked on icon 3 (not used by this dbox)
 -      *result == 3 => user clicked on icon 2 (Cancel), or closed the menu by clicking elsewhere or pressing escape
*/

#define PASSWORD3_OK        1           /* first 3 correspond to save/discard/cancel for windquery2 */
#define PASSWORD3_CANCEL    2
#define PASSWORD3_BLANKOK   3
#define PASSWORD3_PASSWORD  4

extern os_error *
windquery2_doit(
    int x,
    int y,
    WimpWindowWithBitset * wind,
    char * message,
    int * result)
{
    STATUS status = STATUS_FAIL;
    const WimpIconBlockWithBitset * const icons = (const WimpIconBlockWithBitset *) (wind + 1);
    BOOL blankok = !(icons[PASSWORD3_BLANKOK].flags.bits.deleted);
    char * buffer = icons[PASSWORD3_PASSWORD].data.it.buffer;
    int buffsize  = icons[PASSWORD3_PASSWORD].data.it.buffer_size;

    IGNOREPARM(message);
    IGNOREPARM(x);
    IGNOREPARM(y);

    /* Call a routine to get the password into the buffer */
    status = get_db_password(P_DOCU_NONE, buffer, buffsize, blankok);

    /*if(status == STATUS_FAIL)
    {
        *result =  PASSWORD3_CANCEL; * why not do this - user hit cancel *
        return(NULL);
    }
    */

    if(status_fail(status))
        return((os_error *) 1); /* cancel: tell DPlib to give up quietly */

    *result = PASSWORD3_OK; /* user clicked OK: now see if the password is all right */
    return(NULL);
}

/*
 - Exported for use by DPlib (defined by us in "dplib.h" as per "extras.c" example)
 - Given a file of a given actual filetype, allows further investigation (or asking the user) to determine the actual type.
*/

extern os_filetype
import_filetype(
    os_filetype filetype)
{
    return(filetype);
}

/*
 - Exported for use by DPlib (defined by us in "dplib.h" as per "extras.c" example)
 - Routine to copy the 'system' table from one file to another.
 - You may want to copy records verbatim, or to relocate certain relative references on the way.
*/

extern os_error *
import_copyimports(
    fields_file fp1,
    fields_file fp2,
    recflags flags)
{
    IGNOREPARM(flags);
    IGNOREPARM(fp1);
    IGNOREPARM(fp2);
    return(NULL);
}

/*
 - Exported for use by DPlib (defined in "date.h")
 - This gets called by DPlib to locate the "DateFormat" and "Holidays" files as a result of the call to date_init()
 - This then loads them in, possibly unsquashing them via RISC OS 3 Squash, parses them to ensure that they are valid
   and sets up a load of pointers to various parts of the loaded data
*/

extern void
preferences_findname(
    char * leafname,
    char * buffer)
{
    (void) strcat(strcat(strcpy(buffer, product_id()), ":"), leafname); /* SKS changed 27sep94 */
}

/* end of callback.c */
