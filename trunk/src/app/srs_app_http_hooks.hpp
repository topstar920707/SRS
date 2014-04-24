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

#ifndef SRS_APP_HTTP_HOOKS_HPP
#define SRS_APP_HTTP_HOOKS_HPP

/*
#include <srs_app_http_hooks.hpp>
*/
#include <srs_core.hpp>

#ifdef SRS_AUTO_HTTP_CALLBACK

#include <http_parser.h>

class SrsHttpUri;
class SrsSocket;
class SrsRequest;
class SrsHttpParser;
class SrsFlvSegment;

#include <srs_app_st.hpp>

/**
* http client to GET/POST/PUT/DELETE uri
*/
class SrsHttpClient
{
private:
    bool connected;
    st_netfd_t stfd;
    SrsHttpParser* parser;
public:
    SrsHttpClient();
    virtual ~SrsHttpClient();
public:
    /**
    * to post data to the uri.
    * @param req the data post to uri.
    * @param res the response data from server.
    */
    virtual int post(SrsHttpUri* uri, std::string req, std::string& res);
private:
    virtual void disconnect();
    virtual int connect(SrsHttpUri* uri);
};

/**
* the http hooks, http callback api,
* for some event, such as on_connect, call
* a http api(hooks).
*/
class SrsHttpHooks
{
private:
    SrsHttpHooks();
public:
    virtual ~SrsHttpHooks();
public:
    /**
    * on_connect hook, when client connect to srs.
    * @param client_id the id of client on server.
    * @param url the api server url, to valid the client. 
    *         ignore if empty.
    * @return valid failed or connect to the url failed.
    */
    static int on_connect(std::string url, int client_id, std::string ip, SrsRequest* req);
    /**
    * on_close hook, when client disconnect to srs, where client is valid by on_connect.
    * @param client_id the id of client on server.
    * @param url the api server url, to process the event. 
    *         ignore if empty.
    */
    static void on_close(std::string url, int client_id, std::string ip, SrsRequest* req);
    /**
    * on_publish hook, when client(encoder) start to publish stream
    * @param client_id the id of client on server.
    * @param url the api server url, to valid the client. 
    *         ignore if empty.
    * @return valid failed or connect to the url failed.
    */
    static int on_publish(std::string url, int client_id, std::string ip, SrsRequest* req);
    /**
    * on_unpublish hook, when client(encoder) stop publish stream.
    * @param client_id the id of client on server.
    * @param url the api server url, to process the event. 
    *         ignore if empty.
    */
    static void on_unpublish(std::string url, int client_id, std::string ip, SrsRequest* req);
    /**
    * on_play hook, when client start to play stream.
    * @param client_id the id of client on server.
    * @param url the api server url, to valid the client. 
    *         ignore if empty.
    * @return valid failed or connect to the url failed.
    */
    static int on_play(std::string url, int client_id, std::string ip, SrsRequest* req);
    /**
    * on_stop hook, when client stop to play the stream.
    * @param client_id the id of client on server.
    * @param url the api server url, to process the event. 
    *         ignore if empty.
    */
    static void on_stop(std::string url, int client_id, std::string ip, SrsRequest* req);
public:
    /**
    * on_dvr_reap_flv_header hook, when dvr write flv file header.
    * @param url the api server url, to process the event. 
    *         ignore if empty.
    * @param header_file the flv header file.
    */
    static void on_dvr_reap_flv_header(std::string url, SrsRequest* req, std::string header_file);
    /**
    * on_dvr_reap_flv hook, when dvr close flv file.
    * @param url the api server url, to process the event. 
    *         ignore if empty.
    * @param segment the current flv segment.
    */
    static void on_dvr_reap_flv(std::string url, SrsRequest* req, SrsFlvSegment* segment);
};

#endif

#endif