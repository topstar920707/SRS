/*
The MIT License (MIT)

Copyright (c) 2013-2014 winlin

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

#ifndef SRS_APP_LOG_HPP
#define SRS_APP_LOG_HPP

/*
#include <srs_app_log.hpp>
*/

#include <srs_core.hpp>

#include <srs_app_st.hpp>
#include <srs_kernel_log.hpp>

#include <string.h>

#include <string>
#include <map>

/**
* st thread context, get_id will get the st-thread id, 
* which identify the client.
*/
class SrsThreadContext : public ISrsThreadContext
{
private:
    std::map<st_thread_t, int> cache;
public:
    SrsThreadContext();
    virtual ~SrsThreadContext();
public:
    virtual void generate_id();
    virtual int get_id();
};

/**
* the log level, for example:
* if specified Debug level, all level messages will be logged.
* if specified Warn level, only Warn/Error/Fatal level messages will be logged.
*/
class SrsLogLevel
{
public:
    // only used for very verbose debug, generally, 
    // we compile without this level for high performance.
    static const int Verbose = 0x01;
    static const int Info = 0x02;
    static const int Trace = 0x03;
    static const int Warn = 0x04;
    static const int Error = 0x05;
};

/**
* we use memory/disk cache and donot flush when write log.
*/
class SrsFastLog : public ISrsLog
{
private:
    // defined in SrsLogLevel.
    int level;
    char* log_data;
    // log to file if specified srs_log_file
    int fd;
public:
    SrsFastLog();
    virtual ~SrsFastLog();
public:
    virtual void verbose(const char* tag, int context_id, const char* fmt, ...);
    virtual void info(const char* tag, int context_id, const char* fmt, ...);
    virtual void trace(const char* tag, int context_id, const char* fmt, ...);
    virtual void warn(const char* tag, int context_id, const char* fmt, ...);
    virtual void error(const char* tag, int context_id, const char* fmt, ...);
private:
    virtual bool generate_header(const char* tag, int context_id, const char* level_name, int* header_size);
    virtual void write_log(char* str_log, int size, int _level);
};

#endif
