/*****************************************************************//**
 *       @file  dm_port.c
 *      @brief  Brief Decsription
 *
 *  Detail Decsription starts here
 *
 *   @internal
 *     Project  $Project$
 *     Created  3/13/2017 
 *    Revision  $Id$
 *     Company  Data Miracle, Shanghai
 *   Copyright  (C) 2017 Data Miracle Intelligent Technologies
 *    
 *    THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 *    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
 *    PARTICULAR PURPOSE.
 *
 * *******************************************************************/
#include "dm_port.h"


#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#ifndef SSIZE_MAX
#define SSIZE_MAX 32767
#endif

/*----------- gettimeofday definition -----------------*/
#include <time.h>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/time.h>
#endif

#ifdef _WIN32
int gettimeofday(struct timeval *tp, void *tzp)
{
    time_t clock;
    struct tm tm;
    SYSTEMTIME wtm;

    GetLocalTime(&wtm);
    tm.tm_year = wtm.wYear - 1900;
    tm.tm_mon = wtm.wMonth - 1;
    tm.tm_mday = wtm.wDay;
    tm.tm_hour = wtm.wHour;
    tm.tm_min = wtm.wMinute;
    tm.tm_sec = wtm.wSecond;
    tm.tm_isdst = -1;
    clock = mktime(&tm);
    tp->tv_sec = clock;
    tp->tv_usec = wtm.wMilliseconds * 1000;
    return (0);
}

struct tm* localtime_r(const time_t *timep, struct tm *result)
{
    return localtime_s(result, timep) ? NULL : result;
}

struct tm* gmtime_r(const time_t *timep, struct tm *result)
{
    return gmtime_s(result, timep) ? NULL : result;
}

time_t timegm(struct tm *tm)
{
    return _mkgmtime(tm);
}
#endif

#if defined(DM_ANDROID_SUPPORT)
time_t timegm(struct tm *tm)
{
    time_t ret;
    char *tz;

    tz = getenv("TZ"); 
    setenv("TZ", "", 1);
    tzset();
    ret = mktime(tm);
    if (tz)
        setenv("TZ", tz, 1);
    else
        unsetenv("TZ");
    tzset();
    return ret;
}
#endif

#if defined(_WIN32) || defined(DM_ANDROID_SUPPORT) || defined(DM_ARM_LINUX_SUPPORT)
    #include <errno.h>
    #include <limits.h>

ssize_t getdelim(char **lineptr, size_t *n, int delim, FILE *stream)
{
    char c, *cur_pos, *new_lineptr;
    size_t new_lineptr_len;

    if (lineptr == NULL || n == NULL || stream == NULL) {
        errno = EINVAL;
        return -1;
    }

    if (*lineptr == NULL) {
        *n = 128; /* init len */
        if ((*lineptr = (char *)malloc(*n)) == NULL) {
            errno = ENOMEM;
            return -1;
        }
    }

    cur_pos = *lineptr;
    for (;;) {
        c = getc(stream);

        if (ferror(stream) || (c == EOF && cur_pos == *lineptr))
            return -1;

        if (c == EOF)
            break;

        if ((*lineptr + *n - cur_pos) < 2) {
            int cur_pos_offset = cur_pos - *lineptr;

            if (SSIZE_MAX / 2 < *n) {
#ifdef EOVERFLOW
                errno = EOVERFLOW;
#else
                errno = ERANGE; /* no EOVERFLOW defined */
#endif
                return -1;
            }
            new_lineptr_len = *n * 2;

            if ((new_lineptr = (char *)realloc(*lineptr, new_lineptr_len)) == NULL) {
                errno = ENOMEM;
                return -1;
            }

            *lineptr = new_lineptr;
            *n = new_lineptr_len;

            cur_pos = new_lineptr + cur_pos_offset;
        }

        *cur_pos++ = c;

        if (c == delim)
            break;
    }

    *cur_pos = '\0';
    return (ssize_t)(cur_pos - *lineptr);
}

ssize_t getline(char **lineptr, size_t *n, FILE *stream)
{
    return getdelim(lineptr, n, '\n', stream);
}
#endif

