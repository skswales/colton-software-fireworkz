/* ev_fndat.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1991-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Date and time function routines for evaluator */

#include "common/gflags.h"

#include "ob_ss/ob_ss.h"

#if RISCOS
#define EXPOSE_RISCOS_SWIS 1
#include "ob_skel/xp_skelr.h"
#endif

#include <time.h> /* for struct tm */

/******************************************************************************
*
* Date and time functions
*
******************************************************************************/

_Check_return_
static inline SS_DATE_DATE
ss_data_get_date_date(
    _InRef_     PC_SS_DATA p_ss_data)
{
    assert(ss_data_is_date(p_ss_data));
    return(p_ss_data->arg.ss_date.date);
}

_Check_return_
static inline SS_DATE_TIME
ss_data_get_date_time(
    _InRef_     PC_SS_DATA p_ss_data)
{
    assert(ss_data_is_date(p_ss_data));
    return(p_ss_data->arg.ss_date.time);
}

/******************************************************************************
*
* REAL age(date1, date2)
*
******************************************************************************/

PROC_EXEC_PROTO(c_age)
{
    F64 age_result;
    STATUS status;
    SS_DATE ss_date;
    S32 year, month, day;

    exec_func_ignore_parms();

    ss_date_init(&ss_date);

    if( (SS_DATE_NULL != ss_data_get_date_date(args[0])) && (SS_DATE_NULL != ss_data_get_date_date(args[1])) )
    {
        ss_date.date = ss_data_get_date_date(args[0]) - ss_data_get_date_date(args[1]);

        if( (SS_TIME_NULL != ss_data_get_date_time(args[0])) || (SS_TIME_NULL != ss_data_get_date_time(args[1])) )
        {   /* here 31/12/2015 00:00:00 == 31/12/2015 */
            SS_DATE_TIME time_1 = (SS_TIME_NULL != ss_data_get_date_time(args[0])) ? ss_data_get_date_time(args[0]) : 0;
            SS_DATE_TIME time_2 = (SS_TIME_NULL != ss_data_get_date_time(args[1])) ? ss_data_get_date_time(args[1]) : 0;
            ss_date.time = time_1 - time_2;
        }

        ss_date_normalise(&ss_date);
    }

    status = ss_dateval_to_ymd(ss_date.date, &year, &month, &day);
    exec_func_status_return(p_ss_data_res, status);

    age_result = ((F64) year - 1) + ((F64) month - 1) * 0.01;

    ss_data_set_real(p_ss_data_res, age_result);
}

/******************************************************************************
*
* DATE date(year, month, day)
*
******************************************************************************/

PROC_EXEC_PROTO(c_date)
{
    S32 year = ss_data_get_integer(args[0]);
    S32 our_year = year;
    S32 month = ss_data_get_integer(args[1]);
    S32 day = ss_data_get_integer(args[2]);
    SS_DATE_DATE dateval;

    exec_func_ignore_parms();

#if (RELEASED || 1) /* you may set the one to zero temporarily for testing Fireworkz serial numbers for years < 0100 in Debug build */
    if((our_year >= 0) && (our_year < 100))
        our_year = sliding_window_year(our_year);
#endif

    if(ss_ymd_to_dateval(&dateval, our_year, month, day) < 0)
        exec_func_status_return(p_ss_data_res, EVAL_ERR_ARGRANGE);

    ss_data_set_date(p_ss_data_res, dateval, SS_TIME_NULL);
}

/******************************************************************************
*
* DATE datevalue(text) - convert text into a date
*
******************************************************************************/

_Check_return_
static STATUS
date_time_value_common_init(
    _InRef_     PC_SS_DATA p_ss_data,
    _InoutRef_  P_QUICK_UBLOCK p_quick_ublock)
{
    STATUS status;
    PC_UCHARS uchars = ss_data_get_string(p_ss_data);
    U32 uchars_n = ss_data_get_string_size(p_ss_data);

    uchars = ss_string_trim_leading_whitespace_uchars(uchars, &uchars_n);
    uchars = ss_string_trim_trailing_whitespace_uchars(uchars, &uchars_n);

    if(status_ok(status = quick_ublock_uchars_add(p_quick_ublock, uchars, uchars_n)))
        status = quick_ublock_nullch_add(p_quick_ublock);

    return(status);
}

PROC_EXEC_PROTO(c_datevalue)
{
    STATUS status;
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 64);
    quick_ublock_with_buffer_setup(quick_ublock);

    exec_func_ignore_parms();

    if(status_ok(status = date_time_value_common_init(args[0], &quick_ublock)))
        status = ss_recog_date_time(p_ss_data_res, quick_ublock_ustr(&quick_ublock));

    quick_ublock_dispose(&quick_ublock);

    if(status_ok(status))
        if( (status /*recog_res*/ <= 0) || (SS_DATE_NULL == ss_data_get_date_date(p_ss_data_res)) )
            status = EVAL_ERR_BAD_DATE;

    exec_func_status_return(p_ss_data_res, status);
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

    status = ss_dateval_to_ymd(ss_data_get_date_date(args[0]), &year, &month, &day);
    exec_func_status_return(p_ss_data_res, status);

    day_result = day;

    ss_data_set_integer(p_ss_data_res, day_result);
}

/******************************************************************************
*
* STRING dayname(n | date {, mode})
*
******************************************************************************/

static void
ss_string_ini_cap(
    _InoutRef_  P_SS_DATA p_ss_data)
{
    U32 bytes_of_char;
    const UCS4 ucs4 = uchars_char_decode(ss_data_get_string(p_ss_data), bytes_of_char);
    const UCS4 ucs4_uc = t5_ucs4_uppercase(ucs4);
    if(ucs4_uc != ucs4)
    {
        const U32 new_bytes_of_char = uchars_bytes_of_char_encoding(ucs4_uc);
        assert(new_bytes_of_char == bytes_of_char);
        if(new_bytes_of_char == bytes_of_char)
            (void) uchars_char_encode(p_ss_data->arg.string_wr.uchars, bytes_of_char, ucs4_uc);
    }
}

PROC_EXEC_PROTO(c_dayname)
{
    PC_USTR ustr_dayname;
    S32 weekday;
    S32 remainder;
    S32 day_idx = 0;

    exec_func_ignore_parms();

    switch(ss_data_get_data_id(args[0]))
    {
    case DATA_ID_DATE:
        if(SS_DATE_NULL == ss_data_get_date_date(args[0]))
            exec_func_status_return(p_ss_data_res, EVAL_ERR_NODATE);

        weekday = ((ss_data_get_date_date(args[0]) + 1) % 7) + 1; /* [1 (Sunday),7 (Saturday)] - obviously can NOT be modified */

        day_idx = weekday - 1; /* [0,6] */

        if(n_args > 1)
            exec_func_status_return(p_ss_data_res, EVAL_ERR_TOO_MANY_FUNARGS);
        break;

    default: default_unhandled();
#if CHECKING
    case DATA_ID_LOGICAL:
    case DATA_ID_WORD16:
    case DATA_ID_WORD32:
#endif
        weekday = ss_data_get_integer(args[0]); /* usually  [1 (Sunday),7 (Saturday)] - may be modified - will range reduce in any case at the end */

        if(n_args > 1)
        {
            const S32 mode = ss_data_get_integer(args[1]);

            switch(mode)
            {
            case 1: /* Sunday is day one, system 1 */
                /* normal */
                break;

            case 2: /* Monday is day one, system 1 */
                weekday += 1;
                break;

            case 3: /* Monday is day zero */
                weekday += 2;
                break;

            case 11: /* Monday is day one, system 1 */
            case 12: /* Tuesday is day one, system 1 */
            case 13: /* Wednesday is day one, system 1 */
            case 14: /* Thursday is day one, system 1 */
            case 15: /* Friday is day one, system 1 */
            case 16: /* Saturday is day one, system 1 */
            case 17: /* Sunday is day one, system 1 */
                weekday += (mode - 10);
                break;

            case 21: /* Monday is day one, system 2 (ISO 8601) */
            case 150: /* Monday is day one, system 2 (ISO 8601 - as LibreOffice WEEKNUM() for interoperability with Gnumeric) */
                weekday += 1;
                break;

            default:
                exec_func_status_return(p_ss_data_res, EVAL_ERR_ODF_NUM);
            }
        }

        remainder = (weekday - 1) % 7; /* deal with implementation-defined negative behaviour */
        if(remainder < 0)
            remainder = remainder + 7; /* -> [0,6] */

        day_idx = remainder; /* [0,6] */
        break;
    }

    ustr_dayname = p_docu_from_config()->p_numform_context->day_names[day_idx]; /* [0 (Sunday),6 (Saturday)] */

    if(status_ok(ss_string_make_ustr(p_ss_data_res, ustr_dayname)))
        ss_string_ini_cap(p_ss_data_res);
}

/******************************************************************************
*
* INTEGER days(end_date, start_end)
*
******************************************************************************/

PROC_EXEC_PROTO(c_days)
{
    S32 days_result;

    exec_func_ignore_parms();

    if( (SS_DATE_NULL == ss_data_get_date_date(args[0])) || (SS_DATE_NULL == ss_data_get_date_date(args[1])) )
        exec_func_status_return(p_ss_data_res, EVAL_ERR_NODATE);

    /* return number of days between two dates */
    days_result = ss_data_get_date_date(args[0]) /* end_date */ - ss_data_get_date_date(args[1]) /* start_date */;

    ss_data_set_integer(p_ss_data_res, days_result);
}

/******************************************************************************
*
* INTEGER days_360(start_date, end_date {, method:Boolean=FALSE})
*
******************************************************************************/

/*
Credit: https://en.wikipedia.org/wiki/360-day_calendar

A duration is calculated as an integral number of days between two dates A and B (where by convention A is earlier than B).
There are two methods commonly available which differ in the way that they handle the cases where the months are not 30 days long:

The European Method (30E/360)[1]
If either date A or B falls on the 31st of the month, that date will be changed to the 30th;
Where date B falls on the last day of February, the actual date B will be used.

The National Association of Securities Dealers (NASD) 'US' Method (30US/360)[2]
If both date A and B fall on the last day of February, then date B will be changed to the 30th.
If date A falls on the 31st of a month or last day of February, then date A will be changed to the 30th.
If date A falls on the 30th of a month after applying (2) above and date B falls on the 31st of a month, then date B will be changed to the 30th.

In both cases the difference between the possibly-adjusted dates is then computed by treating all intervening months as being 30 days long.
*/

PROC_EXEC_PROTO(c_days_360)
{
    BOOL negate_result = FALSE;
    STATUS status;
    SS_DATE_DATE start_date = ss_data_get_date_date(args[0]);
    SS_DATE_DATE end_date   = ss_data_get_date_date(args[1]);
    const bool european_method = (n_args > 2) ? ss_data_get_logical(args[2]) : false;
    S32 start_year, start_month, start_day;
    S32 end_year, end_month, end_day;
    S32 days_360_result;

    exec_func_ignore_parms();

    if(start_date > end_date)
    {
        memswap32(&start_date, &end_date, sizeof32(start_date));
        negate_result = TRUE;
    }

    status = ss_dateval_to_ymd(start_date, &start_year, &start_month, &start_day);
    exec_func_status_return(p_ss_data_res, status);

    status = ss_dateval_to_ymd(end_date, &end_year, &end_month, &end_day);
    exec_func_status_return(p_ss_data_res, status);

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

    ss_data_set_integer(p_ss_data_res, negate_result ? -days_360_result : days_360_result);
}

/******************************************************************************
*
* DATE edate(date, delta_months)
*
******************************************************************************/

static void
edate_eomonth_calc(
    _OutRef_    P_SS_DATA p_ss_data_out,
    _InVal_     S32 start_year,
    _InVal_     S32 start_month,
    _InVal_     S32 start_day,
    _InVal_     S32 delta_months,
    _InVal_     BOOL f_is_eomonth)
{
    S32 result_year = start_year;
    S32 result_month = start_month + delta_months;
    S32 result_day = start_day;
    S32 monthdays;
    SS_DATE_DATE dateval;

    /* does adjusted month underflow or overflow the current year? */
    /* NB can be adjusting by more than one year */
    while(result_month < 1)
    {
        result_month += 12;
        result_year -= 1;
    }
    while(result_month > 12)
    {
        result_month -= 12;
        result_year += 1;
    }

    monthdays = (LEAP_YEAR_ACTUAL(result_year) ? ev_days_in_month_leap[result_month - 1] : ev_days_in_month[result_month - 1]);

    if(f_is_eomonth)
    {   /* always return the last day of the month */
        result_day = monthdays;
    }
    else
    {   /* try to return the same day of the month unless that's off the end */
        if(result_day > monthdays)
            result_day = monthdays;
    }

    if(ss_ymd_to_dateval(&dateval, result_year, result_month, result_day) < 0)
    {
        ss_data_set_error(p_ss_data_out, EVAL_ERR_ARGRANGE);
        return;
    }

    /* I did think about taking the time component across but Excel doesn't */
    ss_data_set_date(p_ss_data_out, dateval, SS_TIME_NULL);
}

PROC_EXEC_PROTO(c_edate)
{
    STATUS status;
    S32 delta_months = ss_data_get_integer(args[1]);
    S32 start_year, start_month, start_day;

    exec_func_ignore_parms();

    status = ss_dateval_to_ymd(ss_data_get_date_date(args[0]), &start_year, &start_month, &start_day);
    exec_func_status_return(p_ss_data_res, status);

    edate_eomonth_calc(p_ss_data_res, start_year, start_month, start_day, delta_months, FALSE);
}

/******************************************************************************
*
* DATE eomonth(date, delta_months)
*
******************************************************************************/

PROC_EXEC_PROTO(c_eomonth)
{
    STATUS status;
    S32 delta_months = ss_data_get_integer(args[1]);
    S32 start_year, start_month, start_day;

    exec_func_ignore_parms();

    status = ss_dateval_to_ymd(ss_data_get_date_date(args[0]), &start_year, &start_month, &start_day);
    exec_func_status_return(p_ss_data_res, status);

    edate_eomonth_calc(p_ss_data_res, start_year, start_month, start_day, delta_months, TRUE);
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

    status = ss_timeval_to_hms(ss_data_get_date_time(args[0]), &hours, &minutes, &seconds);
    exec_func_status_return(p_ss_data_res, status);

    hour_result = hours;

    ss_data_set_integer(p_ss_data_res, hour_result);
}

/******************************************************************************
*
* INTEGER return the week number for a date (ISO 8601)
*
******************************************************************************/

_Check_return_
static S32
calc_isoweeknum(
    _InVal_     S32 year,
    _InVal_     S32 month,
    _InVal_     S32 day)
{
    S32 weeknum;
    U8Z buffer[32];
    struct tm tm;

    zero_struct_fn(tm);
    tm.tm_year = (int) (year - 1900);
    tm.tm_mon  = (int) (month - 1);
    tm.tm_mday = (int) (day);

    /* actually needs wday and yday setting up! */
    consume(time_t, mktime(&tm)); /* normalise */

    consume(size_t, strftime(buffer, elemof32(buffer), "%V", &tm)); /* result in [01,53], as we want */

    weeknum = fast_strtoul(buffer, NULL);

    return(weeknum);
}

PROC_EXEC_PROTO(c_isoweeknum)
{
    S32 isoweeknum_result;
    STATUS status;
    S32 year, month, day;

    exec_func_ignore_parms();

    status = ss_dateval_to_ymd(ss_data_get_date_date(args[0]), &year, &month, &day);
    exec_func_status_return(p_ss_data_res, status); /* bad/missing date? */

    isoweeknum_result = calc_isoweeknum(year, month, day);

    ss_data_set_integer(p_ss_data_res, isoweeknum_result);
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

    status = ss_timeval_to_hms(ss_data_get_date_time(args[0]), &hours, &minutes, &seconds);
    exec_func_status_return(p_ss_data_res, status);

    minute_result = minutes;

    ss_data_set_integer(p_ss_data_res, minute_result);
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

    status = ss_dateval_to_ymd(ss_data_get_date_date(args[0]), &year, &month, &day);
    exec_func_status_return(p_ss_data_res, status);

    month_result = month;

    ss_data_set_integer(p_ss_data_res, month_result);
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

    status = ss_dateval_to_ymd(ss_data_get_date_date(args[0]), &year, &month, &day);
    exec_func_status_return(p_ss_data_res, status);

    monthdays_result = (LEAP_YEAR_ACTUAL(year) ? ev_days_in_month_leap[month - 1] : ev_days_in_month[month - 1]);

    ss_data_set_integer(p_ss_data_res, monthdays_result);
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

    switch(ss_data_get_data_id(args[0]))
    {
    case DATA_ID_DATE:
        {
        STATUS status;
        S32 year, month, day;
        status = ss_dateval_to_ymd(ss_data_get_date_date(args[0]), &year, &month, &day);
        exec_func_status_return(p_ss_data_res, status);
        month_idx = month - 1;
        break;
        }

    default: default_unhandled();
#if CHECKING
    case DATA_ID_LOGICAL:
    case DATA_ID_WORD16:
    case DATA_ID_WORD32:
#endif
        month_idx = MAX(0, ss_data_get_integer(args[0]) - 1);
        month_idx = month_idx % 12;
        break;
    }

    ustr_monthname = p_docu_from_config()->p_numform_context->month_names[month_idx];

    if(status_ok(ss_string_make_ustr(p_ss_data_res, ustr_monthname)))
        ss_string_ini_cap(p_ss_data_res);
}

/******************************************************************************
*
* DATE now
*
******************************************************************************/

PROC_EXEC_PROTO(c_now)
{
    exec_func_ignore_parms();
    UNREFERENCED_PARAMETER(args);

    ss_local_time_to_ss_date(&p_ss_data_res->arg.ss_date);
    ss_data_set_data_id(p_ss_data_res, DATA_ID_DATE);
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

    status = ss_timeval_to_hms(ss_data_get_date_time(args[0]), &hours, &minutes, &seconds);
    exec_func_status_return(p_ss_data_res, status);

    second_result = seconds;

    ss_data_set_integer(p_ss_data_res, second_result);
}

/******************************************************************************
*
* DATE time(hours, minutes, seconds)
*
******************************************************************************/

PROC_EXEC_PROTO(c_time)
{
    SS_DATE_TIME timeval;

    exec_func_ignore_parms();

    if(ss_hms_to_timeval(&timeval,
                         ss_data_get_integer(args[0]),
                         ss_data_get_integer(args[1]),
                         ss_data_get_integer(args[2])) < 0)
    {
        exec_func_status_return(p_ss_data_res, EVAL_ERR_ARGRANGE);
    }

    ss_data_set_date(p_ss_data_res, SS_DATE_NULL, timeval);

    ss_date_normalise(&p_ss_data_res->arg.ss_date);
}

/******************************************************************************
*
* DATE timevalue(text) - convert text to time value
*
******************************************************************************/

PROC_EXEC_PROTO(c_timevalue)
{
    STATUS status;
    QUICK_UBLOCK_WITH_BUFFER(quick_ublock, 64);
    quick_ublock_with_buffer_setup(quick_ublock);

    exec_func_ignore_parms();

    if(status_ok(status = date_time_value_common_init(args[0], &quick_ublock)))
        status = ss_recog_date_time(p_ss_data_res, quick_ublock_ustr(&quick_ublock));

    quick_ublock_dispose(&quick_ublock);

    if(status_ok(status))
        if( (status /*recog_res*/ <= 0)  /* NB if a date is specified as well, we should accept that (OASIS) */ )
            status = EVAL_ERR_BAD_TIME;

    exec_func_status_return(p_ss_data_res, status);
}

/******************************************************************************
*
* DATE return today's date (without time component)
*
******************************************************************************/

PROC_EXEC_PROTO(c_today)
{
    exec_func_ignore_parms();

    c_now(args, n_args, p_ss_data_res, p_cur_slr);

    if(ss_data_is_date(p_ss_data_res))
        p_ss_data_res->arg.ss_date.time = SS_TIME_NULL;
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

    if(SS_DATE_NULL == ss_data_get_date_date(args[0]))
        exec_func_status_return(p_ss_data_res, EVAL_ERR_NODATE);

    weekday = ((ss_data_get_date_date(args[0]) + 1) % 7) + 1; /* [1 (Sunday),7 (Saturday)] */

    if(n_args > 1)
    {
        const S32 mode = ss_data_get_integer(args[1]);
        S32 remainder;

        switch(mode)
        {
        case 1: /* Sunday is day one, system 1 */
            /* normal */
            break;

        case 2: /* Monday is day one, system 1 */
            weekday -= 1;
            break;

        case 3: /* Monday is day zero */
            weekday -= 2;
            break;

        case 11: /* Monday is day one, system 1 */
        case 12: /* Tuesday is day one, system 1 */
        case 13: /* Wednesday is day one, system 1 */
        case 14: /* Thursday is day one, system 1 */
        case 15: /* Friday is day one, system 1 */
        case 16: /* Saturday is day one, system 1 */
        case 17: /* Sunday is day one, system 1 */
            weekday -= (mode - 10);
            break;

        case 21: /* Monday is day one, system 2 (ISO 8601) */
        case 150: /* Monday is day one, system 2 (ISO 8601 - as LibreOffice WEEKNUM() for interoperability with Gnumeric) */
            weekday -= 1;
            break;

        default:
            exec_func_status_return(p_ss_data_res, EVAL_ERR_ODF_NUM);
        }

        remainder = (weekday - 1) % 7; /* deal with implementation-defined negative behaviour */
        if(remainder < 0)
            remainder = remainder + 7; /* -> [0,6] */

        weekday = remainder + 1; /* back to [1,7] */

        if(3 == mode) /* remap to [0,6] for this peculiar mode */
            if(/*Monday*/ 7 == weekday)
                weekday = 0;
    }

    ss_data_set_integer(p_ss_data_res, weekday);
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
    _InVal_     SS_DATE_DATE ss_date_date)
{
    STATUS status;
    S32 year, month, day;
    RISCOS_TIME_ORDINALS time_ordinals;
    _kernel_swi_regs rs;

    if(status_fail(status = ss_dateval_to_ymd(ss_date_date, &year, &month, &day)))
    {
        zero_struct_ptr(p_fivebyte);
        return(status);
    }

    zero_struct_fn(time_ordinals);
    time_ordinals.year  = year;
    time_ordinals.month = month;
    time_ordinals.day   = day;

    rs.r[0] = -1; /* use current territory */
    rs.r[1] = (int) &p_fivebyte->utc[0];
    rs.r[2] = (int) &time_ordinals;
    if(NULL != WrapOsErrorChecking(_kernel_swi(Territory_ConvertOrdinalsToTime, &rs, &rs)))
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
    U8Z buffer[32];
    _kernel_swi_regs rs;

    status = fivebytetime_from_date(&fivebyte, ss_data_get_date_date(args[0]));
    exec_func_status_return(p_ss_data_res, status); /* bad/missing date? */

    rs.r[0] = -1; /* use current territory */
    rs.r[1] = (int) &fivebyte.utc[0];
    rs.r[2] = (int) buffer;
    rs.r[3] = sizeof32(buffer);
    rs.r[4] = (int) "%WK";
    if(NULL != WrapOsErrorChecking(_kernel_swi(Territory_ConvertDateAndTime, &rs, &rs)))
        weeknumber_result = 0; /* a result of zero -> info not available */
    else
        weeknumber_result = (S32) fast_strtoul(buffer, NULL);

    assert(weeknumber_result >= 0);
    ss_data_set_integer(p_ss_data_res, weeknumber_result);
#else
    S32 year, month, day;
    U8Z buffer[32];
    struct tm tm;

    status = ss_dateval_to_ymd(ss_data_get_date_date(args[0]), &year, &month, &day);
    exec_func_status_return(p_ss_data_res, status); /* bad/missing date? */

    zero_struct_fn(tm);
    tm.tm_year = (int) (year - 1900);
    tm.tm_mon  = (int) (month - 1);
    tm.tm_mday = (int) (day);

    /* actually needs wday and yday setting up! */
    (void) mktime(&tm); /* normalise */

    strftime(buffer, elemof32(buffer), "%W", &tm);

    weeknumber_result = fast_strtoul(buffer, NULL);

    weeknumber_result += 1;

    ss_data_set_integer(p_ss_data_res, weeknumber_result);
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

    status = ss_dateval_to_ymd(ss_data_get_date_date(args[0]), &year, &month, &day);
    exec_func_status_return(p_ss_data_res, status);

    year_result = year;

    ss_data_set_integer(p_ss_data_res, year_result);
}

/* end of ev_fndat.c */
