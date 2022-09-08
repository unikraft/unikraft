/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
** This file is in the public domain, so clarified as of
** 1996-06-05 by Arthur David Olson.
*/


#include "time.h"
#include "errno.h"

#define FALSE	0
#define TRUE	1

#ifdef _MSC_VER
#include <stdint.h>
typedef int32_t	    int_fast32_t;
typedef int64_t     int_fast64_t;
#else
typedef __int32_t	int_fast32_t;
typedef __int64_t   int_fast64_t;
#endif

static struct tm    tmGlobal;

#ifndef TZ_MAX_TIMES
#define TZ_MAX_TIMES	1200
#endif /* !defined TZ_MAX_TIMES */

#ifndef TZ_MAX_TYPES
/* This must be at least 17 for Europe/Samara and Europe/Vilnius.  */
#define TZ_MAX_TYPES	256 /* Limited by what (unsigned char)'s can hold */
#endif /* !defined TZ_MAX_TYPES */

#ifndef TZ_MAX_CHARS
#define TZ_MAX_CHARS	50	/* Maximum number of abbreviation characters */
/* (limited by what unsigned chars can hold) */
#endif /* !defined TZ_MAX_CHARS */

#ifndef TZ_MAX_LEAPS
#define TZ_MAX_LEAPS	50	/* Maximum number of leap second corrections */
#endif /* !defined TZ_MAX_LEAPS */

#define BIGGEST(a, b)   (((a) > (b)) ? (a) : (b))

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunneeded-internal-declaration"
#endif
static const char gmt[] = "GMT";
#ifdef __clang__
#pragma clang diagnostic pop
#endif

#ifdef TZNAME_MAX
#define MY_TZNAME_MAX   TZNAME_MAX
#endif /* defined TZNAME_MAX */
#ifndef TZNAME_MAX
#define MY_TZNAME_MAX   255
#endif /* !defined TZNAME_MAX */

struct ttinfo {              /* time type information */
    int_fast32_t tt_gmtoff;  /* UT offset in seconds */
    int          tt_isdst;   /* used to set tm_isdst */
    int          tt_abbrind; /* abbreviation list index */
    int          tt_ttisstd; /* TRUE if transition is std time */
    int          tt_ttisgmt; /* TRUE if transition is UT */
};

struct lsinfo {              /* leap second information */
    time_t       ls_trans;   /* transition time */
    int_fast64_t ls_corr;    /* correction to apply */
};

struct state {
    int           leapcnt;
    int           timecnt;
    int           typecnt;
    int           charcnt;
    int           goback;
    int           goahead;
    time_t        ats[TZ_MAX_TIMES];
    unsigned char types[TZ_MAX_TIMES];
    struct ttinfo ttis[TZ_MAX_TYPES];
    char          chars[BIGGEST(BIGGEST(TZ_MAX_CHARS + 1, sizeof gmt),
                                (2 * (MY_TZNAME_MAX + 1)))];
    struct lsinfo lsis[TZ_MAX_LEAPS];
    int           defaulttype; /* for early times or if no transitions */
};

#define TM_SUNDAY	0
#define TM_MONDAY	1
#define TM_TUESDAY	2
#define TM_WEDNESDAY	3
#define TM_THURSDAY	4
#define TM_FRIDAY	5
#define TM_SATURDAY	6

#define TM_JANUARY	0
#define TM_FEBRUARY	1
#define TM_MARCH	2
#define TM_APRIL	3
#define TM_MAY		4
#define TM_JUNE		5
#define TM_JULY		6
#define TM_AUGUST	7
#define TM_SEPTEMBER	8
#define TM_OCTOBER	9
#define TM_NOVEMBER	10
#define TM_DECEMBER	11

#define SECSPERMIN	60
#define MINSPERHOUR	60
#define HOURSPERDAY	24
#define DAYSPERWEEK	7
#define DAYSPERNYEAR	365
#define DAYSPERLYEAR	366
#define SECSPERHOUR	(SECSPERMIN * MINSPERHOUR)
#define SECSPERDAY	((int_fast32_t) SECSPERHOUR * HOURSPERDAY)
#define MONSPERYEAR	12
#define EPOCH_YEAR	1970
#define EPOCH_WDAY	TM_THURSDAY

#define isleap(y) (((y) % 4) == 0 && (((y) % 100) != 0 || ((y) % 400) == 0))

static const int mon_lengths[2][MONSPERYEAR] = {
        { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
        { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
};

static const int year_lengths[2] = {
        DAYSPERNYEAR, DAYSPERLYEAR
};

#define TYPE_SIGNED(type) (((type) -1) < 0)
#define	INT_MAX		0x7fffffff	/* max value for an int */
#define	INT_MIN		(-0x7fffffff-1)	/* min value for an int */

#define TM_YEAR_BASE	1900

/*
** Return the number of leap years through the end of the given year
** where, to make the math easy, the answer for year zero is defined as zero.
*/

static int
leaps_thru_end_of(const int y)
{
    return (y >= 0) ? (y / 4 - y / 100 + y / 400) :
           -(leaps_thru_end_of(-(y + 1)) + 1);
}


/*
** Normalize logic courtesy Paul Eggert.
*/

static int
increment_overflow(int *const ip, int j)
{
    int const i = *ip;

    /*
    ** If i >= 0 there can only be overflow if i + j > INT_MAX
    ** or if j > INT_MAX - i; given i >= 0, INT_MAX - i cannot overflow.
    ** If i < 0 there can only be overflow if i + j < INT_MIN
    ** or if j < INT_MIN - i; given i < 0, INT_MIN - i cannot overflow.
    */
    if ((i >= 0) ? (j > INT_MAX - i) : (j < INT_MIN - i))
        return TRUE;
    *ip += j;
    return FALSE;
}


static struct tm *
timesub(const time_t *const timep, const int_fast32_t offset,
        struct tm *const tmp)
{
    time_t       tdays;
    int          idays;  /* unsigned would be so 2003 */
    int_fast64_t rem;
    int                   y;
    const int *  ip;
    int_fast64_t corr;
    int          hit;

    corr = 0;
    hit = 0;

    y = EPOCH_YEAR;
    tdays = *timep / SECSPERDAY;
    rem = *timep - tdays * SECSPERDAY;
    while (tdays < 0 || tdays >= year_lengths[isleap(y)]) {
        int     newy;
        time_t tdelta;
        int    idelta;
        int    leapdays;

        tdelta = tdays / DAYSPERLYEAR;
        if (! ((! TYPE_SIGNED(time_t) || INT_MIN <= tdelta)
               && tdelta <= INT_MAX))
            return NULL;
        idelta = (int) tdelta;
        if (idelta == 0)
            idelta = (tdays < 0) ? -1 : 1;
        newy = y;
        if (increment_overflow(&newy, idelta))
            return NULL;
        leapdays = leaps_thru_end_of(newy - 1) -
                   leaps_thru_end_of(y - 1);
        tdays -= ((time_t) newy - y) * DAYSPERNYEAR;
        tdays -= leapdays;
        y = newy;
    }
    {
        int_fast32_t   seconds;

        seconds = (int_fast32_t) tdays * SECSPERDAY;
        tdays = seconds / SECSPERDAY;
        rem += seconds - tdays * SECSPERDAY;
    }
    /*
    ** Given the range, we can now fearlessly cast...
    */
    idays = (int) tdays;
    rem += offset - corr;
    while (rem < 0) {
        rem += SECSPERDAY;
        --idays;
    }
    while (rem >= SECSPERDAY) {
        rem -= SECSPERDAY;
        ++idays;
    }
    while (idays < 0) {
        if (increment_overflow(&y, -1))
            return NULL;
        idays += year_lengths[isleap(y)];
    }
    while (idays >= year_lengths[isleap(y)]) {
        idays -= year_lengths[isleap(y)];
        if (increment_overflow(&y, 1))
            return NULL;
    }
    tmp->tm_year = y;
    if (increment_overflow(&tmp->tm_year, -TM_YEAR_BASE))
        return NULL;
    tmp->tm_yday = idays;
    /*
    ** The "extra" mods below avoid overflow problems.
    */
    tmp->tm_wday = EPOCH_WDAY +
                   ((y - EPOCH_YEAR) % DAYSPERWEEK) *
                   (DAYSPERNYEAR % DAYSPERWEEK) +
                   leaps_thru_end_of(y - 1) -
                   leaps_thru_end_of(EPOCH_YEAR - 1) +
                   idays;
    tmp->tm_wday %= DAYSPERWEEK;
    if (tmp->tm_wday < 0)
        tmp->tm_wday += DAYSPERWEEK;
    tmp->tm_hour = (int) (rem / SECSPERHOUR);
    rem %= SECSPERHOUR;
    tmp->tm_min = (int) (rem / SECSPERMIN);
    /*
    ** A positive leap second requires a special
    ** representation. This uses "... ??:59:60" et seq.
    */
    tmp->tm_sec = (int) (rem % SECSPERMIN) + hit;
    ip = mon_lengths[isleap(y)];
    for (tmp->tm_mon = 0; idays >= ip[tmp->tm_mon]; ++(tmp->tm_mon))
        idays -= ip[tmp->tm_mon];
    tmp->tm_mday = (int) (idays + 1);
    tmp->tm_isdst = 0;
#ifdef TM_GMTOFF
    tmp->TM_GMTOFF = offset;
#endif /* defined TM_GMTOFF */
    return tmp;
}

struct tm *
sgxssl__gmtime64(const time_t * timep)
{
    if (timep == nullptr)
    {
        return nullptr;
    }

    struct tm* result;

    result = timesub(timep, 0L, &tmGlobal);

    return result;

}


/////////////////////////////////////////////////////////////////////////mktime functions
#ifndef WRONG
#define WRONG	(-1)
#endif /* !defined WRONG */

#define CHAR_BIT      8         // number of bits in a char
#define TYPE_BIT(type)	(sizeof (type) * CHAR_BIT)

#define LONG_MIN    (-2147483647L - 1) // minimum (signed) long value
#define LONG_MAX      2147483647L   // maximum (signed) long value

static struct state *	lclptr = NULL;

static struct tm *
localsub(const time_t *timep, long offset, struct tm *tmp)
{
    //reduced irrelvant code
    struct state *		sp;

    sp = lclptr;
    if (sp == NULL)
    {
        return timesub(timep,  (int_fast32_t) offset, tmp);
    }
    //WRONG
    return NULL;
}

static int
long_increment_overflow(long *lp, int m)
{
    long const	l = *lp;

    if ((l >= 0) ? (m > LONG_MAX - l) : (m < LONG_MIN - l))
        return TRUE;
    *lp += m;
    return FALSE;
}

static int
normalize_overflow(int *tensptr, int *unitsptr, int base)
{
    int	tensdelta;

    tensdelta = (*unitsptr >= 0) ?
                (*unitsptr / base) :
                (-1 - (-1 - *unitsptr) / base);
    *unitsptr -= tensdelta * base;
    return increment_overflow(tensptr, tensdelta);
}

static int
long_normalize_overflow(long *tensptr, int *unitsptr, int base)
{
    int	tensdelta;

    tensdelta = (*unitsptr >= 0) ?
                (*unitsptr / base) :
                (-1 - (-1 - *unitsptr) / base);
    *unitsptr -= tensdelta * base;
    return long_increment_overflow(tensptr, tensdelta);
}

static int
tmcomp(const struct tm *atmp, const struct tm *btmp)
{
    int	result;

    if ((result = (atmp->tm_year - btmp->tm_year)) == 0 &&
        (result = (atmp->tm_mon - btmp->tm_mon)) == 0 &&
        (result = (atmp->tm_mday - btmp->tm_mday)) == 0 &&
        (result = (atmp->tm_hour - btmp->tm_hour)) == 0 &&
        (result = (atmp->tm_min - btmp->tm_min)) == 0)
        result = atmp->tm_sec - btmp->tm_sec;
    return result;
}

static time_t
time2sub(struct tm *tmp, struct tm *(*funcp)(const time_t *, long, struct tm *),
         long offset, int *okayp, int do_norm_secs)
{
    int			dir;
    int			i;
    int			saved_seconds;
    long			li;
    time_t			lo;
    time_t			hi;
    long			y;
    time_t			newt;
    time_t			t;
    struct tm		yourtm, mytm;

    *okayp = FALSE;
    yourtm = *tmp;
    if (do_norm_secs) {
        if (normalize_overflow(&yourtm.tm_min, &yourtm.tm_sec,
                               SECSPERMIN))
            return WRONG;
    }
    if (normalize_overflow(&yourtm.tm_hour, &yourtm.tm_min, MINSPERHOUR))
        return WRONG;
    if (normalize_overflow(&yourtm.tm_mday, &yourtm.tm_hour, HOURSPERDAY))
        return WRONG;
    y = yourtm.tm_year;
    if (long_normalize_overflow(&y, &yourtm.tm_mon, MONSPERYEAR))
        return WRONG;
    /*
    ** Turn y into an actual year number for now.
    ** It is converted back to an offset from TM_YEAR_BASE later.
    */
    if (long_increment_overflow(&y, TM_YEAR_BASE))
        return WRONG;
    while (yourtm.tm_mday <= 0) {
        if (long_increment_overflow(&y, -1))
            return WRONG;
        li = y + (1 < yourtm.tm_mon);
        yourtm.tm_mday += year_lengths[isleap(li)];
    }
    while (yourtm.tm_mday > DAYSPERLYEAR) {
        li = y + (1 < yourtm.tm_mon);
        yourtm.tm_mday -= year_lengths[isleap(li)];
        if (long_increment_overflow(&y, 1))
            return WRONG;
    }
    for (;;) {
        i = mon_lengths[isleap(y)][yourtm.tm_mon];
        if (yourtm.tm_mday <= i)
            break;
        yourtm.tm_mday -= i;
        if (++yourtm.tm_mon >= MONSPERYEAR) {
            yourtm.tm_mon = 0;
            if (long_increment_overflow(&y, 1))
                return WRONG;
        }
    }
    if (long_increment_overflow(&y, -TM_YEAR_BASE))
        return WRONG;
    yourtm.tm_year = (int) y;
    if (yourtm.tm_year != y)
        return WRONG;
    if (yourtm.tm_sec >= 0 && yourtm.tm_sec < SECSPERMIN)
        saved_seconds = 0;
    else if (y + TM_YEAR_BASE < EPOCH_YEAR) {
        /*
        ** We can't set tm_sec to 0, because that might push the
        ** time below the minimum representable time.
        ** Set tm_sec to 59 instead.
        ** This assumes that the minimum representable time is
        ** not in the same minute that a leap second was deleted from,
        ** which is a safer assumption than using 58 would be.
        */
        if (increment_overflow(&yourtm.tm_sec, 1 - SECSPERMIN))
            return WRONG;
        saved_seconds = yourtm.tm_sec;
        yourtm.tm_sec = SECSPERMIN - 1;
    }
    else {
        saved_seconds = yourtm.tm_sec;
        yourtm.tm_sec = 0;
    }
    /*
    ** Do a binary search (this works whatever time_t's type is).
    */
    lo = 1;
    for (i = 0; i < (int)TYPE_BIT(time_t) - 1; ++i)
        lo *= 2;
    hi = -(lo + 1);
    for (;;) {
        t = lo / 2 + hi / 2;
        if (t < lo)
            t = lo;
        else if (t > hi)
            t = hi;
        if ((*funcp)(&t, offset, &mytm) == NULL) {
            /*
            ** Assume that t is too extreme to be represented in
            ** a struct tm; arrange things so that it is less
            ** extreme on the next pass.
            */
            dir = (t > 0) ? 1 : -1;
        }
        else
            dir = tmcomp(&mytm, &yourtm);
        if (dir != 0) {
            if (t == lo) {
                ++t;
                if (t <= lo)
                    return WRONG;
                ++lo;
            }
            else if (t == hi) {
                --t;
                if (t >= hi)
                    return WRONG;
                --hi;
            }
//            if (lo > hi) // FIXME it causes problems on Ubuntu 18, Clang 7, O2 flag, probably something is optimized out
//                return WRONG;
            if (dir > 0)
                hi = t;
            else
                lo = t;
            continue;
        }
        if (yourtm.tm_isdst < 0 || mytm.tm_isdst <= yourtm.tm_isdst)
            break;
    }

    newt = t + saved_seconds;
    if ((newt < t) != (saved_seconds < 0))
        return WRONG;
    t = newt;
    if ((*funcp)(&t, offset, tmp))
        *okayp = TRUE;
    return t;
}

static time_t
time2(struct tm *tmp, struct tm * (*funcp)(const time_t *, long, struct tm *),
      long offset, int *okayp)
{
    time_t	t;

    /*
    ** First try without normalization of seconds
    ** (in case tm_sec contains a value associated with a leap second).
    ** If that fails, try with normalization of seconds.
    */
    t = time2sub(tmp, funcp, offset, okayp, FALSE);
    return *okayp ? t : time2sub(tmp, funcp, offset, okayp, TRUE);
}

static time_t
time1(struct tm *tmp, struct tm * (*funcp)(const time_t *, long, struct tm *),
      long offset)
{
    time_t			t;
    int			okay;

    if (tmp == NULL) {
        return WRONG;
    }
    if (tmp->tm_isdst > 1)
        tmp->tm_isdst = 1;
    t = time2(tmp, funcp, offset, &okay);
    if (okay)
        return t;
    return WRONG;
}

time_t sgxssl_mktime(struct tm *tmp)
{
    //reduced irrelvant code from original function
    time_t ret;
    ret = time1(tmp, localsub, 0L);
    return ret;
}
