/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * win32_flickcurl_config.c - Flickcurl WIN32 function implementations
 *
 * Copyright (C) 2008-2012, David Beckett http://www.dajobe.org/
 * 
 * It is licensed under the following three licenses as alternatives:
 *   1. GNU Lesser General Public License (LGPL) V2.1 or any newer version
 *   2. GNU General Public License (GPL) V2 or any newer version
 *   3. Apache License, V2.0 or any newer version
 * 
 * You may not use this file except in compliance with at least one of
 * the above three licenses.
 * 
 * See LICENSE.html or LICENSE.txt at the top of this package for the
 * complete terms and further detail along with the license texts for
 * the licenses in COPYING.LIB, COPYING and LICENSE-2.0.txt respectively.
 * 
 * 
 */

#ifdef WIN32

#include <win32_flickcurl_config.h>
#include <sys/timeb.h>
#include <string.h>
#include <time.h>

#if !defined(_MSC_VER) || _MSC_VER < 1900

/* seconds between 1 Jan 1601 (windows epoch) and 1 Jan 1970 (unix epoch) */
#define EPOCH_WIN_UNIX_DELTA 11644473600.0

/* 100 nano-seconds ( = 1/10 usec) in seconds */
#define NSEC100 (1e-7)

/* factor to convert high-dword count into seconds = NSEC100 * (2<<32) */
#define FOUR_GIGA_NSEC100 (4294967296e-7)

int
gettimeofday(struct timeval* tp, void* tzp)
{
  FILETIME ft;
  double t;
  
  /* returns time since windows epoch in 100ns (1/10us) units */
  GetSystemTimeAsFileTime(&ft);

  /* convert time into seconds as a double */
  t = ((ft.dwHighDateTime * FOUR_GIGA_NSEC100) - EPOCH_WIN_UNIX_DELTA) +
      (ft.dwLowDateTime  * NSEC100);

  tp->tv_sec  = (long) t;
  tp->tv_usec = (long) ((t - tp->tv_sec) * 1e6);

  /* tzp is ignored */

  return 0;
}

#else // VS 2015+

int
gettimeofday(struct timeval* tp, void* tzp)
{
    struct timespec ts;

    if(timespec_get(&ts, TIME_UTC) != TIME_UTC)
        return -1;

    tp->tv_sec  = ts.tv_sec;
    tp->tv_usec = ts.tv_nsec / 1000;
    return 0;
}
#endif

int
mkstemp(char* template)
{
  int fd;
  errno_t err;

  err = _mktemp_s(template, strlen(template));
  if(err)
    return -1;

  fd = _open(template, _O_RDWR | _O_BINARY);
  if(fd < 0)
    return -1;

  return fd;
}

/* getentropy compatibility for Windows - wraps BCryptGenRandom
 * (same bcrypt import libcurl already pulls in) */
int 
getentropy(void *buf, size_t nbytes)
{
    NTSTATUS status = BCryptGenRandom(NULL, (PUCHAR)buf,
                                      (ULONG)nbytes,
                                      BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    if(status >= 0)
        return 0;
    errno = ENOSYS;
    return -1;
}

#endif
