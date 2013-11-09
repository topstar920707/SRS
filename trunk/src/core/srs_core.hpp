/*
The MIT License (MIT)

Copyright (c) 2013 winlin

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

/**
* the core provides the common defined macros, utilities,
* user must include the srs_core.hpp before any header, or maybe 
* build failed.
*/

// for int64_t print using PRId64 format.
#ifndef __STDC_FORMAT_MACROS
    #define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>

#include <assert.h>
#define srs_assert(expression) assert(expression)

#include <stddef.h>
#include <sys/types.h>

// free the p and set to NULL.
// p must be a T*.
#define srs_freep(p) \
	if (p) { \
		delete p; \
		p = NULL; \
	} \
	(void)0
// free the p which represents a array
#define srs_freepa(p) \
	if (p) { \
		delete[] p; \
		p = NULL; \
	} \
	(void)0

// current release version
#define RTMP_SIG_SRS_VERSION "0.3.0"
// server info.
#define RTMP_SIG_SRS_KEY "srs"
#define RTMP_SIG_SRS_ROLE "origin server"
#define RTMP_SIG_SRS_NAME RTMP_SIG_SRS_KEY"(simple rtmp server)"
#define RTMP_SIG_SRS_URL "https://"RTMP_SIG_SRS_URL_SHORT
#define RTMP_SIG_SRS_URL_SHORT "github.com/winlinvip/simple-rtmp-server"
#define RTMP_SIG_SRS_WEB "http://blog.csdn.net/win_lin"
#define RTMP_SIG_SRS_EMAIL "winterserver@126.com"
#define RTMP_SIG_SRS_LICENSE "The MIT License (MIT)"
#define RTMP_SIG_SRS_COPYRIGHT "Copyright (c) 2013 winlin"

// compare
#define srs_min(a, b) (((a) < (b))? (a) : (b))
#define srs_max(a, b) (((a) < (b))? (b) : (a))

// get current system time in ms, use cache to avoid performance problem
extern int64_t srs_get_system_time_ms();
// the deamon st-thread will update it.
extern void srs_update_system_time_ms();

// signal defines.
#define SIGNAL_RELOAD SIGHUP

#endif