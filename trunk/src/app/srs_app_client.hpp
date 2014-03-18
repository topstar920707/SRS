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

#ifndef SRS_APP_CLIENT_HPP
#define SRS_APP_CLIENT_HPP

/*
#include <srs_app_client.hpp>
*/

#include <srs_core.hpp>

#include <srs_app_st.hpp>
#include <srs_app_conn.hpp>
#include <srs_app_reload.hpp>

class SrsRtmpServer;
class SrsRequest;
class SrsResponse;
class SrsSource;
class SrsRefer;
class SrsConsumer;
class SrsCommonMessage;
class SrsSocket;
#ifdef SRS_HTTP_CALLBACK    
class SrsHttpHooks;
#endif
class SrsBandwidth;

/**
* the client provides the main logic control for RTMP clients.
*/
class SrsClient : public SrsConnection, public ISrsReloadHandler
{
private:
    char* ip;
    SrsRequest* req;
    SrsResponse* res;
    SrsSocket* skt;
    SrsRtmpServer* rtmp;
    SrsRefer* refer;
#ifdef SRS_HTTP_CALLBACK    
    SrsHttpHooks* http_hooks;
#endif
    SrsBandwidth* bandwidth;
public:
    SrsClient(SrsServer* srs_server, st_netfd_t client_stfd);
    virtual ~SrsClient();
protected:
    virtual int do_cycle();
// interface ISrsReloadHandler
public:
    virtual int on_reload_vhost_removed(std::string vhost);
private:
    // when valid and connected to vhost/app, service the client.
    virtual int service_cycle();
    // stream(play/publish) service cycle, identify client first.
    virtual int stream_service_cycle();
    virtual int check_vhost();
    virtual int playing(SrsSource* source);
    virtual int fmle_publish(SrsSource* source);
    virtual int flash_publish(SrsSource* source);
    virtual int process_publish_message(SrsSource* source, SrsCommonMessage* msg);
    virtual int get_peer_ip();
    virtual int process_play_control_msg(SrsConsumer* consumer, SrsCommonMessage* msg);
private:
    virtual int on_connect();
    virtual void on_close();
    virtual int on_publish();
    virtual void on_unpublish();
    virtual int on_play();
    virtual void on_stop();
};

#endif
