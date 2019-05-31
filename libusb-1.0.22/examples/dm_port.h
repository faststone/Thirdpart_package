/*****************************************************************//**
 *       @file  dm_port.h
 *      @brief  BRIEF DESCRIPTION HERE
 *
 *  Detailed description starts here
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
#ifndef _DM_PORT_H_
#define _DM_PORT_H_
#include <stdint.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include "libusb.h"

#if _MSC_VER
    #define snprintf _snprintf
#endif

/* use 64bit version ftell/fseek
 * -D_FILE_OFFSET_BITS=64 should be defined */
#define ftell(f) ftello(f)
#define fseek(f,o,s) fseeko(f,o,s)

#ifdef _WIN32
    #include <intrin.h>
    #include <windows.h>
    #define sleep(s) Sleep((s)*1000)
    #define usleep(s) Sleep((s)/1000)
    #define malloc_align(align, sz) _aligned_malloc(sz, align)
    #define free_align(p) _aligned_free(p)
#else
    #include <sys/time.h>
    #include <unistd.h>
    #define malloc_align(align, sz) memalign(align, sz)
    #define free_align(p) free(p)
#endif

#ifdef _WIN32
ssize_t getline(char **lineptr, size_t *n, FILE *stream);
time_t timegm(struct tm *);
#endif

#ifdef _WIN32
int gettimeofday(struct timeval *tp, void *tzp);
struct tm* localtime_r(const time_t *timep, struct tm *result);
struct tm* gmtime_r(const time_t *timep, struct tm *result);
#endif

#endif /* _DM_PORT_H_ */
