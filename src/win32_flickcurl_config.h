/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * win32_flickcurl_config.h - Flickcurl WIN32 hard-coded config
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


#ifndef WIN32_CONFIG_H
#define WIN32_CONFIG_H


#ifdef __cplusplus
extern "C" {
#endif

/* For struct timeval */
#include <winsock2.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>
#include <memory.h>
#include <fcntl.h>
#include <bcrypt.h>
#include <errno.h>

/* MS names for these functions (vsnprintf only needed on old MSVC < 2015) */
#if _MSC_VER < 1900
#define vsnprintf _vsnprintf
#endif
#define access _access

/* define missing flag for access() - the only one used here */
#ifndef R_OK
#define R_OK 4
#endif

/* Implemented in win32_extras.c */

/* getentropy compatibility for Windows - wraps BCryptGenRandom */
#ifndef HAVE_GETENTROPY
int getentropy(void *buf, size_t nbytes);
/* Can't define HAVE_GETENTROPY since this leads to source files including nonexistent <sys/random.h>,
 * have to rely on defined(HAVE_GETENTROPY) || defined(WIN32) instead */
/* #define HAVE_GETENTROPY 1 */
#endif

#ifndef HAVE_GETTIMEOFDAY
int gettimeofday(struct timeval* tv, void* tz);
#define HAVE_GETTIMEOFDAY 1
#endif

#ifndef HAVE_MKSTEMP
int mkstemp(char* template);
#define HAVE_MKSTEMP 1
#endif


/* 
 * All defines from config.h should be added here with appropriate values
 */

/* vcpkg libcurl (>= 7.56) has the modern mime API
 * (configure would find it via AC_CHECK_FUNC(curl_mime_init)) */
#define HAVE_LIBCURL_CURL_MIME_INIT 1

#undef HAVE_NANOSLEEP

/* not quite true - but it's called something else and the define above
 * handles it
 */
#define HAVE_VSNPRINTF 1

/* UCRT (VS2015+) vsnprintf is C99 compliant */
#if _MSC_VER >= 1900
#define HAVE_C99_VSNPRINTF 1
#endif

/* use our own getopt */
#undef HAVE_GETOPT
#undef HAVE_GETOPT_LONG

/* Raptor RDF */
#undef HAVE_RAPTOR

/* Platform features */
#define HAVE_ERRNO_H 1
#define HAVE_STDINT_H 1
#define HAVE_STDLIB_H 1
#define HAVE_TIME_H 1

/* FIXME - rest of win32 config defines should go here */


#ifdef __cplusplus
}
#endif

#endif
