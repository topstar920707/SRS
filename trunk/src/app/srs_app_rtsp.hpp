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

#ifndef SRS_APP_RTSP_HPP
#define SRS_APP_RTSP_HPP

/*
#include <srs_app_rtsp.hpp>
*/

#include <srs_core.hpp>

#include <string>

#ifdef SRS_AUTO_STREAM_CASTER

class SrsConfDirective;

/**
* the handler for rtsp handler.
*/
class ISrsRtspHandler
{
public:
    ISrsRtspHandler();
    virtual ~ISrsRtspHandler();
};

/**
* the connection for rtsp.
*/
class SrsRtspConn : public ISrsRtspHandler
{
private:
    std::string output;
public:
    SrsRtspConn(SrsConfDirective* c);
    virtual ~SrsRtspConn();
};

#endif

#endif
