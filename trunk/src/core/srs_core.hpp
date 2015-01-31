/*
The MIT License (MIT)

Copyright (c) 2013-2015 winlin

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef SRS_CORE_HPP
#define SRS_CORE_HPP

/*
#include <srs_core.hpp>
*/

// current release version
#define VERSION_MAJOR       2
#define VERSION_MINOR       0
#define VERSION_REVISION    110

// server info.
#define RTMP_SIG_SRS_KEY "SRS"
#define RTMP_SIG_SRS_ROLE "origin/edge server"
#define RTMP_SIG_SRS_NAME RTMP_SIG_SRS_KEY"(Simple RTMP Server)"
#define RTMP_SIG_SRS_URL_SHORT "github.com/winlinvip/simple-rtmp-server"
#define RTMP_SIG_SRS_URL "https://"RTMP_SIG_SRS_URL_SHORT
#define RTMP_SIG_SRS_WEB "http://blog.csdn.net/win_lin"
#define RTMP_SIG_SRS_EMAIL "winlin@vip.126.com"
#define RTMP_SIG_SRS_LICENSE "The MIT License (MIT)"
#define RTMP_SIG_SRS_COPYRIGHT "Copyright (c) 2013-2015 winlin"
#define RTMP_SIG_SRS_PRIMARY "winlin"
#define RTMP_SIG_SRS_AUTHROS "wenjie.zhao"
#define RTMP_SIG_SRS_CONTRIBUTORS_URL RTMP_SIG_SRS_URL"/blob/master/AUTHORS.txt"
#define RTMP_SIG_SRS_HANDSHAKE RTMP_SIG_SRS_KEY"("RTMP_SIG_SRS_VERSION")"
#define RTMP_SIG_SRS_RELEASE RTMP_SIG_SRS_URL"/tree/1.0release"
#define RTMP_SIG_SRS_ISSUES(id) RTMP_SIG_SRS_URL"/issues/"#id
#define RTMP_SIG_SRS_VERSION __SRS_XSTR(VERSION_MAJOR)"."__SRS_XSTR(VERSION_MINOR)"."__SRS_XSTR(VERSION_REVISION)

// internal macros, covert macro values to str,
// see: read https://gcc.gnu.org/onlinedocs/cpp/Stringification.html#Stringification
#define __SRS_XSTR(v) __SRS_STR(v)
#define __SRS_STR(v) #v

/**
* the core provides the common defined macros, utilities,
* user must include the srs_core.hpp before any header, or maybe 
* build failed.
*/

// for 32bit os, 2G big file limit for unistd io, 
// ie. read/write/lseek to use 64bits size for huge file.
#ifndef _FILE_OFFSET_BITS
    #define _FILE_OFFSET_BITS 64
#endif

// for int64_t print using PRId64 format.
#ifndef __STDC_FORMAT_MACROS
    #define __STDC_FORMAT_MACROS
#endif

// for srs-librtmp, @see https://github.com/winlinvip/simple-rtmp-server/issues/213
#ifndef _WIN32
#include <inttypes.h>
#endif

#include <assert.h>
#define srs_assert(expression) assert(expression)

#include <stddef.h>
#include <sys/types.h>

// generated by configure.
#include <srs_auto_headers.hpp>

// free the p and set to NULL.
// p must be a T*.
#define srs_freep(p) \
    if (p) { \
        delete p; \
        p = NULL; \
    } \
    (void)0
// sometimes, the freepa is useless,
// it's recomments to free each elem explicit.
// so we remove the srs_freepa utility.

/**
* disable copy constructor of class,
* to avoid the memory leak or corrupt by copy instance.
*/
#define disable_default_copy(className)\
    private:\
        /** \
        * disable the copy constructor and operator=, donot allow directly copy. \
        */ \
        className(const className&); \
        className& operator= (const className&)

#endif

