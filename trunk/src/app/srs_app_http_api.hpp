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

#ifndef SRS_APP_HTTP_API_HPP
#define SRS_APP_HTTP_API_HPP

/*
#include <srs_app_http_api.hpp>
*/

#include <srs_core.hpp>

#ifdef SRS_HTTP_API

class SrsSocket;
class SrsHttpMessage;
class SrsHttpParser;
class SrsHttpHandler;

#include <srs_app_st.hpp>
#include <srs_app_conn.hpp>
#include <srs_app_http.hpp>

// for http root.
class SrsApiRoot : public SrsHttpHandler
{
public:
    SrsApiRoot();
    virtual ~SrsApiRoot();
public:
    virtual bool can_handle(const char* path, int length);
    virtual int process_request(SrsSocket* skt, SrsHttpMessage* req, const char* path, int length);
};

class SrsHttpApi : public SrsConnection
{
private:
    SrsHttpParser* parser;
    SrsHttpHandler* handler;
public:
    SrsHttpApi(SrsServer* srs_server, st_netfd_t client_stfd, SrsHttpHandler* _handler);
    virtual ~SrsHttpApi();
protected:
    virtual int do_cycle();
private:
    virtual int process_request(SrsSocket* skt, SrsHttpMessage* req);
};

#endif

#endif
