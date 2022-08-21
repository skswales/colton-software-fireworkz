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

/* SKS stripped down and modified for Colton Software use */

static
struct WIMPT_STATICS
{
    BOOL          fake_waiting;
    int           fake_event_code;
    WimpPollBlock fake_event_data;
}
wimpt_;

static void
wimpt_poll_coltsoft(
    int mask,
    WimpPollBlock *block,
    /*int *pollword,*/
    int *event_code)
{
    if(wimpt_.fake_waiting)
    {
        wimpt_.fake_waiting = FALSE;
        *event_code = wimpt_.fake_event_code;
        *block = wimpt_.fake_event_data;
        /*trace_1(TRACE_RISCOS_HOST, TEXT("wimpt_poll_coltsoft: returning faked event %s"), report_wimp_event(*event_code, block));*/
        return;
    }

    for(;;)
    {
        _kernel_oserror * err;

        /*trace_2(TRACE_RISCOS_HOST, TEXT("wimp_poll_coltsoft(") U32_XTFMT TEXT(", ") PTR_XTFMT TEXT(")"), mask, block);*/
        err = wimp_poll_coltsoft(mask, block, NULL /*pollword**/, event_code);
        /*trace_1(TRACE_RISCOS_HOST, TEXT("wimp_poll_coltsoft: %s"), report_wimp_event(*event_code, block));*/

        if(NULL != err)
        {
            reportf(TEXT("OS error from wimp_poll: %d:%s"), err->errnum, err->errmess);
            void_WrapOsErrorReporting(err);
            continue;
        }

        switch(*event_code)
        {
        case Wimp_EMouseClick:
            trace_1(TRACE_RISCOS_HOST, TEXT("wimp_poll_coltsoft: about to pp %s"), report_wimp_event(*event_code, block));
            if(!status_done(preprocess_mouse_click(block)))
                continue; /* get another event, this one was not good */
            break;

        default:
            break;
        }

        break; /* out of Wimp_Poll core loop */
    }

    /*trace_1(TRACE_RISCOS_HOST, TEXT("wimpt_poll_coltsoft returns real event %s"), report_wimp_event(*event_code, block));*/
    return;
}

extern void
wimpt_fake_event(
    _InVal_     int event_code,
    _In_        const WimpPollBlock * const p_event_data)
{
    trace_1(TRACE_RISCOS_HOST, TEXT("wimpt_fake_event(%s): "), report_wimp_event(event_code, p_event_data));

    if(!wimpt_.fake_waiting)
    {
        wimpt_.fake_waiting = TRUE;
        wimpt_.fake_event_code = event_code; /* copy event to buffer */
        wimpt_.fake_event_data = *p_event_data;
    }
#if TRACE_ALLOWED
    else
    {
        trace_2(TRACE_OUT | TRACE_RISCOS_HOST, TEXT("double fake event - event %s dropped because event %s still buffered"),
                report_wimp_event(            event_code,            p_event_data),
                report_wimp_event(wimpt_.fake_event_code, &wimpt_.fake_event_data));
    }
#endif
}

/* end of wimpt.c */
