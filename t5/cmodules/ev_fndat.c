/* ev_fndat.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Date and time function routines for evaluator */

#include "common/gflags.h"

#include "ob_ss/ob_ss.h"

#if RISCOS
#include "ob_skel/xp_skelr.h"
#endif

/******************************************************************************
*
* Date and time functions
*
******************************************************************************/

/******************************************************************************
*
* REAL age(date1, date2)
*
******************************************************************************/

PROC_EXEC_PROTO(c_age)
{
    F64 age_result;
    STATUS status;
    EV_DATE ev_date;
    S32 year, month, day;

    exec_func_ignore_parms();

    ev_date_init(&ev_date);

    if((EV_DATE_NULL != args[0]->arg.ev_date.date) && (EV_DATE_NULL != args[1]->arg.ev_date.date))
    {
        ev_date.date = args[0]->arg.ev_date.date - args[1]->arg.ev_date.date;

        { /* here 31/12/2015 00:00:00 == 31/12/2015 */
        EV_DATE_TIME time_1 = (EV_TIME_NULL != args[0]->arg.ev_date.time) ? args[0]->arg.ev_date.time : 0;
        EV_DATE_TIME time_2 = (EV_TIME_NULL != args[1]->arg.ev_date.time) ? args[1]->arg.ev_date.time : 0;
        ev_date.time = time_1 - time_2;
        } /*block*/

        ss_date_normalise(&ev_date);
    }

    if(status_fail(status = ss_dateval_to_ymd(&ev_date.date, &year, &month, &day)))
    {
        ev_data_set_error(p_ev_data_res, status);
        return;
    }

    age_result = ((F64) year - 1) + ((F64) month - 1) * 0.01;

    ev_data_set_real(p_ev_data_res, age_result);
}

/******************************************************************************
*
* DATE date(year, month, day)
*
******************************************************************************/

PROC_EXEC_PROTO(c_date)
{
    S32 year = args[0]->arg.integer;
    S32 our_year = year;
    S32 month = args[1]->arg.integer;
    S32 day = args[2]->arg.integer;

    exec_func_ignore_parms();

#if (RELEASED || 1) /* you may set the one to zero temporarily for testing Fireworkz serial numbers for years < 0100 in Debug build */
    if((our_year >= 0) && (our_year < 100))
        our_year = sliding_window_year(our_year);
#endif

    p_ev_data_res->did_num = RPN_DAT_DATE;
    ev_date_init(&p_ev_data_res->arg.ev_date);

    if(ss_ymd_to_dateval(&p_ev_data_res->arg.ev_date.date, our_year, month, day) < 0)
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
        return;
    }
}

/******************************************************************************
*
* DATE datevalue(text) - convert text into a date
*
******************************************************************************/

PROC_EXEC_PROTO(c_datevalue)
{
    UCHARZ buffer[BUF_EV_MAX_STRING_LEN];
    PC_UCHARS uchars;
    U32 wss, len;

    exec_func_ignore_parms();

    wss = ss_string_skip_leading_whitespace(args[0]);
    uchars = uchars_AddBytes_wr(args[0]->arg.string.uchars, wss);
    len = args[0]->arg.string.size - wss;

    len = MIN(len, sizeof32(buffer)-1);
    memcpy32(buffer, uchars, len);
    buffer[len] = CH_NULL;

    if((ss_recog_date_time(p_ev_data_res, ustr_bptr(buffer)) <= 0) || (EV_DATE_NULL == p_ev_data_res->arg.ev_date.date))
        ev_data_set_error(p_ev_data_res, EVAL_ERR_BAD_DATE);
}

/******************************************************************************
*
* INTEGER return day number of date
*
******************************************************************************/

PROC_EXEC_PROTO(c_day)
{
    S32 day_result;
    STATUS status;
    S32 year, month, day;

    exec_func_ignore_parms();

    if(status_fail(status = ss_dateval_to_ymd(&args[0]->arg.ev_date.date, &year, &month, &day)))
    {
        ev_data_set_error(p_ev_data_res, status);
        return;
    }

    day_result = day;

    ev_data_set_integer(p_ev_data_res, day_result);
}

/******************************************************************************
*
* STRING dayname(n | date)
*
******************************************************************************/

PROC_EXEC_PROTO(c_dayname)
{
    PC_USTR ustr_dayname;
    S32 day = 1;

    exec_func_ignore_parms();

    switch(args[0]->did_num)
    {
    case RPN_DAT_DATE:
        if(EV_DATE_NULL == args[0]->arg.ev_date.date)
        {
            ev_data_set_error(p_ev_data_res, EVAL_ERR_NODATE);
            return;
        }
        day = ((args[0]->arg.ev_date.date + 1) % 7) + 1;
        break;

    case RPN_DAT_BOOL8:
    case RPN_DAT_WORD8:
    case RPN_DAT_WORD16:
    case RPN_DAT_WORD32:
        day = MAX(1, args[0]->arg.integer);
        break;

    default: default_unhandled(); break;
    }

    day = (day - 1) % 7;

    ustr_dayname = p_docu_from_config()->p_numform_context->day_names[day];

    if(status_ok(ss_string_make_ustr(p_ev_data_res, ustr_dayname)))
    {
        U32 bytes_of_char;
        const UCS4 ucs4 = uchars_char_decode(p_ev_data_res->arg.string.uchars, bytes_of_char);
        const UCS4 ucs4_uc = t5_ucs4_uppercase(ucs4);
        const U32 new_bytes_of_char = uchars_bytes_of_char_encoding(ucs4_uc);
        assert(new_bytes_of_char == bytes_of_char);
        if(new_bytes_of_char == bytes_of_char)
            (void) uchars_char_encode(p_ev_data_res->arg.string_wr.uchars, bytes_of_char, ucs4_uc);
    }
}

/******************************************************************************
*
* INTEGER days_360(start_date, end_date {, method})
*
******************************************************************************/

/*
Credit: http://en.wikipedia.org/wiki/360-day_calendar

A duration is calculated as an integral number of days between two dates A and B (where by convention A is earlier than B).
There are two methods commonly available which differ in the way that they handle the cases where the months are not 30 days long:

The European Method (30E/360)[1]
If either date A or B falls on the 31st of the month, that date will be changed to the 30th;
Where date B falls on the last day of February, the actual date B will be used.

The US/NASD Method (30US/360)[2]
If both date A and B fall on the last day of February, then date B will be changed to the 30th.
If date A falls on the 31st of a month or last day of February, then date A will be changed to the 30th.
If date A falls on the 30th of a month after applying (2) above and date B falls on the 31st of a month, then date B will be changed to the 30th.

In both cases the difference between the possibly-adjusted dates is then computed by treating all intervening months as being 30 days long.
*/

PROC_EXEC_PROTO(c_days_360)
{
    BOOL negate_result = FALSE;
    STATUS status;
    EV_DATE_DATE start_date = args[0]->arg.ev_date.date;
    EV_DATE_DATE end_date   = args[1]->arg.ev_date.date;
    BOOL european_method = FALSE; 
    S32 start_year, start_month, start_day;
    S32 end_year, end_month, end_day;
    S32 days_360_result;

    exec_func_ignore_parms();

    if(start_date > end_date)
    {
        memswap32(&start_date, &end_date, sizeof32(start_date));
        negate_result = TRUE;
    }

    if(status_fail(status = ss_dateval_to_ymd(&start_date, &start_year, &start_month, &start_day)))
    {
        ev_data_set_error(p_ev_data_res, status);
        return;
    }

    if(status_fail(status = ss_dateval_to_ymd(&end_date, &end_year, &end_month, &end_day)))
    {
        ev_data_set_error(p_ev_data_res, status);
        return;
    }

    if(n_args > 2)
        european_method = (0 != args[0]->arg.integer);

    if(european_method)
    {
        /* If either date A or B falls on the 31st of the month, that date will be changed to the 30th */
        if(start_day == 31)
            start_day = 30;

        if(end_day == 31)
            end_day = 30;

        /* Where date B falls on the last day of February, the actual date B will be used */
    }
    else
    {
        BOOL start_is_leap_year = LEAP_YEAR_ACTUAL(start_year);
        BOOL start_is_last_day_of_feb = (start_month == 2) && (start_day == (start_is_leap_year ? 29 : 28));

        /* If both date A and B fall on the last day of February, then date B will be changed to the 30th */
        if(start_is_last_day_of_feb && (start_month == end_month))
        {
            BOOL end_is_leap_year = LEAP_YEAR_ACTUAL(end_year);
            BOOL end_is_last_day_of_feb = /*(end_month == 2) &&*/ (end_day == (end_is_leap_year ? 29 : 28));

            if(/*start_is_last_day_of_feb &&*/ end_is_last_day_of_feb)
            {
                end_day = 30;
            }
        }

        /* If date A falls on the 31st of a month or last day of February, then date A will be changed to the 30th */
        if((start_day == 31) || start_is_last_day_of_feb)
        {
            start_day = 30;

            /* If date A falls on the 30th of a month after applying (2) above and date B falls on the 31st of a month, then date B will be changed to the 30th */
            if(end_day == 31)
                end_day = 30;
        }
    }

    days_360_result = (end_day - start_day);

    days_360_result += 30 * (end_month - start_month);

    days_360_result += 360 * (end_year - start_year);

    ev_data_set_integer(p_ev_data_res, negate_result ? -days_360_result : days_360_result);
}

/******************************************************************************
*
* INTEGER return hour number of time
*
******************************************************************************/

PROC_EXEC_PROTO(c_hour)
{
    S32 hour_result;
    STATUS status;
    S32 hours, minutes, seconds;

    exec_func_ignore_parms();

    if(status_fail(status = ss_timeval_to_hms(&args[0]->arg.ev_date.time, &hours, &minutes, &seconds)))
    {
        ev_data_set_error(p_ev_data_res, status);
        return;
    }

    hour_result = hours;

    ev_data_set_integer(p_ev_data_res, hour_result);
}

/******************************************************************************
*
* INTEGER return minute number of time
*
******************************************************************************/

PROC_EXEC_PROTO(c_minute)
{
    S32 minute_result;
    STATUS status;
    S32 hours, minutes, seconds;

    exec_func_ignore_parms();

    if(status_fail(status = ss_timeval_to_hms(&args[0]->arg.ev_date.time, &hours, &minutes, &seconds)))
    {
        ev_data_set_error(p_ev_data_res, status);
        return;
    }

    minute_result = minutes;

    ev_data_set_integer(p_ev_data_res, minute_result);
}

/******************************************************************************
*
* INTEGER return month number of date
*
******************************************************************************/

PROC_EXEC_PROTO(c_month)
{
    S32 month_result;
    STATUS status;
    S32 year, month, day;

    exec_func_ignore_parms();

    if(status_fail(status = ss_dateval_to_ymd(&args[0]->arg.ev_date.date, &year, &month, &day)))
    {
        ev_data_set_error(p_ev_data_res, status);
        return;
    }

    month_result = month;

    ev_data_set_integer(p_ev_data_res, month_result);
}

/******************************************************************************
*
* INTEGER monthdays(date)
*
******************************************************************************/

PROC_EXEC_PROTO(c_monthdays)
{
    S32 monthdays_result;
    STATUS status;
    S32 year, month, day;

    exec_func_ignore_parms();

    if(status_fail(status = ss_dateval_to_ymd(&args[0]->arg.ev_date.date, &year, &month, &day)))
    {
        ev_data_set_error(p_ev_data_res, status);
        return;
    }

    monthdays_result = (LEAP_YEAR_ACTUAL(year) ? ev_days_in_month_leap[month - 1] : ev_days_in_month[month - 1]);

    ev_data_set_integer(p_ev_data_res, monthdays_result);
}

/******************************************************************************
*
* STRING monthname(n | date)
*
******************************************************************************/

PROC_EXEC_PROTO(c_monthname)
{
    PC_USTR ustr_monthname;
    S32 month_idx = 0;

    exec_func_ignore_parms();

    switch(args[0]->did_num)
    {
    case RPN_DAT_DATE:
        {
        STATUS status;
        S32 year, month, day;
        if(status_fail(status = ss_dateval_to_ymd(&args[0]->arg.ev_date.date, &year, &month, &day)))
        {
            ev_data_set_error(p_ev_data_res, status);
            return;
        }
        month_idx = month - 1;
        break;
        }

    case RPN_DAT_BOOL8:
    case RPN_DAT_WORD8:
    case RPN_DAT_WORD16:
    case RPN_DAT_WORD32:
        month_idx = MAX(0, args[0]->arg.integer - 1);
        month_idx = month_idx % 12;
        break;

    default: default_unhandled(); break;
    }

    ustr_monthname = p_docu_from_config()->p_numform_context->month_names[month_idx];

    if(status_ok(ss_string_make_ustr(p_ev_data_res, ustr_monthname)))
    {
        U32 bytes_of_char;
        const UCS4 ucs4 = uchars_char_decode(p_ev_data_res->arg.string.uchars, bytes_of_char);
        const UCS4 ucs4_uc = t5_ucs4_uppercase(ucs4);
        const U32 new_bytes_of_char = uchars_bytes_of_char_encoding(ucs4_uc);
        assert(new_bytes_of_char == bytes_of_char);
        if(new_bytes_of_char == bytes_of_char)
            (void) uchars_char_encode(p_ev_data_res->arg.string_wr.uchars, bytes_of_char, ucs4_uc);
    }
}

/******************************************************************************
*
* DATE now
*
******************************************************************************/

PROC_EXEC_PROTO(c_now)
{
    exec_func_ignore_parms();

    p_ev_data_res->did_num = RPN_DAT_DATE;
    ss_local_time_as_ev_date(&p_ev_data_res->arg.ev_date);
}

/******************************************************************************
*
* INTEGER return second number of time
*
******************************************************************************/

PROC_EXEC_PROTO(c_second)
{
    S32 second_result;
    STATUS status;
    S32 hours, minutes, seconds;

    exec_func_ignore_parms();

    if(status_fail(status = ss_timeval_to_hms(&args[0]->arg.ev_date.time, &hours, &minutes, &seconds)))
    {
        ev_data_set_error(p_ev_data_res, status);
        return;
    }

    second_result = seconds;

    ev_data_set_integer(p_ev_data_res, second_result);
}

/******************************************************************************
*
* DATE time(hours, minutes, seconds)
*
******************************************************************************/

PROC_EXEC_PROTO(c_time)
{
    exec_func_ignore_parms();

    p_ev_data_res->did_num = RPN_DAT_DATE;
    ev_date_init(&p_ev_data_res->arg.ev_date);

    if(ss_hms_to_timeval(&p_ev_data_res->arg.ev_date.time,
                         args[0]->arg.integer,
                         args[1]->arg.integer,
                         args[2]->arg.integer) < 0)
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_ARGRANGE);
        return;
    }

    ss_date_normalise(&p_ev_data_res->arg.ev_date);
}

/******************************************************************************
*
* DATE timevalue(text) - convert text to time value
*
******************************************************************************/

PROC_EXEC_PROTO(c_timevalue)
{
    UCHARZ buffer[BUF_EV_MAX_STRING_LEN];
    PC_UCHARS uchars;
    U32 wss, len;

    exec_func_ignore_parms();

    wss = ss_string_skip_leading_whitespace(args[0]);
    uchars = uchars_AddBytes_wr(args[0]->arg.string.uchars, wss);
    len = args[0]->arg.string.size - wss;

    len = MIN(len, sizeof32(buffer)-1);
    memcpy32(buffer, uchars, len);
    buffer[len] = CH_NULL;

    /* NB if a date is specified as well, we should accept that (OASIS) */
    if(ss_recog_date_time(p_ev_data_res, ustr_bptr(buffer)) <= 0)
        ev_data_set_error(p_ev_data_res, EVAL_ERR_BADTIME);
}

/******************************************************************************
*
* DATE return today's date (without time component)
*
******************************************************************************/

PROC_EXEC_PROTO(c_today)
{
    exec_func_ignore_parms();

    c_now(args, n_args, p_ev_data_res, p_cur_slr);

    if(RPN_DAT_DATE == p_ev_data_res->did_num)
        p_ev_data_res->arg.ev_date.time = EV_TIME_NULL;
}

/******************************************************************************
*
* INTEGER return the day of the week for a date
*
******************************************************************************/

PROC_EXEC_PROTO(c_weekday)
{
    S32 weekday;

    exec_func_ignore_parms();

    if(EV_DATE_NULL == args[0]->arg.ev_date.date)
    {
        ev_data_set_error(p_ev_data_res, EVAL_ERR_NODATE);
        return;
    }

    weekday = (args[0]->arg.ev_date.date + 1) % 7;

    ev_data_set_integer(p_ev_data_res, weekday + 1);
}

/******************************************************************************
*
* INTEGER return the week number for a date
*
******************************************************************************/

#if RISCOS

typedef struct FIVEBYTE
{
    U8 utc[5];
}
FIVEBYTE, * P_FIVEBYTE;

_Check_return_
static STATUS
fivebytetime_from_date(
    _OutRef_    P_FIVEBYTE p_fivebyte,
    _InRef_     PC_EV_DATE_DATE p_ev_date_date)
{
    STATUS status;
    S32 year, month, day;
    RISCOS_TIME_ORDINALS time_ordinals;
    _kernel_swi_regs rs;

    if(status_fail(status = ss_dateval_to_ymd(p_ev_date_date, &year, &month, &day)))
    {
        zero_struct_ptr(p_fivebyte);
        return(status);
    }

    zero_struct(time_ordinals);
    time_ordinals.year  = year;
    time_ordinals.month = month;
    time_ordinals.day   = day;

    rs.r[0] = -1; /* use current territory */
    rs.r[1] = (int) &p_fivebyte->utc[0];
    rs.r[2] = (int) &time_ordinals;
    if(NULL != WrapOsErrorChecking(_kernel_swi(/*Territory_ConvertOrdinalsToTime*/ 0x43051, &rs, &rs)))
        zero_struct_ptr(p_fivebyte);

    return(STATUS_OK);
}

#endif

PROC_EXEC_PROTO(c_weeknumber)
{
    S32 weeknumber_result;
    STATUS status;

    exec_func_ignore_parms();

    {
#if RISCOS
    FIVEBYTE fivebyte;

    if(status_ok(status = fivebytetime_from_date(&fivebyte, &args[0]->arg.ev_date.date)))
    {
        U8Z buffer[32];
        _kernel_swi_regs rs;

        rs.r[0] = -1; /* use current territory */
        rs.r[1] = (int) &fivebyte.utc[0];
        rs.r[2] = (int) buffer;
        rs.r[3] = sizeof32(buffer);
        rs.r[4] = (int) "%WK";
        if(NULL != WrapOsErrorChecking(_kernel_swi(/*Territory_ConvertDateAndTime*/ 0x4304B, &rs, &rs)))
            weeknumber_result = 0; /* a result of zero -> info not available */
        else
            weeknumber_result = (S32) fast_strtoul(buffer, NULL);

        assert(weeknumber_result >= 0);
        ev_data_set_integer(p_ev_data_res, weeknumber_result);
    }
    else
        ev_data_set_error(p_ev_data_res, status); /* bad/missing date */
#else
    S32 year, month, day;

    if(status_ok(status = ss_dateval_to_ymd(&args[0]->arg.ev_date.date, &year, &month, &day)))
    {
        U8Z buffer[32];
        struct tm tm;

        zero_struct(tm);
        tm.tm_year = (int) (year - 1900);
        tm.tm_mon  = (int) (month - 1);
        tm.tm_mday = (int) (day);

        /* actually needs wday and yday setting up! */
        (void) mktime(&tm); /* normalise */

        strftime(buffer, elemof32(buffer), "%W", &tm);

        weeknumber_result = fast_strtoul(buffer, NULL);

        weeknumber_result += 1;

        ev_data_set_integer(p_ev_data_res, weeknumber_result);
    }
    else
        ev_data_set_error(p_ev_data_res, status); /* bad/missing date */
#endif
    } /*block*/
}

/******************************************************************************
*
* INTEGER return year number of date
*
******************************************************************************/

PROC_EXEC_PROTO(c_year)
{
    S32 year_result;
    STATUS status;
    S32 year, month, day;

    exec_func_ignore_parms();

    if(status_fail(status = ss_dateval_to_ymd(&args[0]->arg.ev_date.date, &year, &month, &day)))
    {
        ev_data_set_error(p_ev_data_res, status);
        return;
    }

    year_result = year;

    ev_data_set_integer(p_ev_data_res, year_result);
}

/* end of ev_fndat.c */
