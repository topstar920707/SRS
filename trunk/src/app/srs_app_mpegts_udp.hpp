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

#ifndef SRS_APP_MPEGTS_UDP_HPP
#define SRS_APP_MPEGTS_UDP_HPP

/*
#include <srs_app_mpegts_udp.hpp>
*/

#include <srs_core.hpp>

#ifdef SRS_AUTO_STREAM_CASTER

class sockaddr_in;
#include <string>

class SrsStream;
class SrsTsContext;
class SrsConfDirective;
class SrsSimpleBuffer;
class SrsRtmpClient;
class SrsStSocket;
class SrsRequest;
class SrsRawH264Stream;

#include <srs_app_st.hpp>
#include <srs_kernel_ts.hpp>

/**
* the udp packet handler.
*/
class ISrsUdpHandler
{
public:
    ISrsUdpHandler();
    virtual ~ISrsUdpHandler();
public:
    /**
    * when udp listener got a udp packet, notice server to process it.
    * @param type, the client type, used to create concrete connection,
    *       for instance RTMP connection to serve client.
    * @param from, the udp packet from address.
    * @param buf, the udp packet bytes, user should copy if need to use.
    * @param nb_buf, the size of udp packet bytes.
    * @remark user should never use the buf, for it's a shared memory bytes.
    */
    virtual int on_udp_packet(sockaddr_in* from, char* buf, int nb_buf) = 0;
};

/**
* the mpegts over udp stream caster.
*/
class SrsMpegtsOverUdp : virtual public ISrsTsHandler
    , virtual public ISrsUdpHandler
{
private:
    SrsStream* stream;
    SrsTsContext* context;
    SrsSimpleBuffer* buffer;
    std::string output;
private:
    SrsRequest* req;
    st_netfd_t stfd;
    SrsStSocket* io;
    SrsRtmpClient* client;
    int stream_id;
private:
    SrsRawH264Stream* avc;
    std::string h264_sps;
    bool h264_sps_changed;
    std::string h264_pps;
    bool h264_pps_changed;
    bool h264_sps_pps_sent;
public:
    SrsMpegtsOverUdp(SrsConfDirective* c);
    virtual ~SrsMpegtsOverUdp();
// interface ISrsUdpHandler
public:
    virtual int on_udp_packet(sockaddr_in* from, char* buf, int nb_buf);
// interface ISrsTsHandler
public:
    virtual int on_ts_message(SrsTsMessage* msg);
private:
    virtual int on_ts_video(SrsTsMessage* msg, SrsStream* avs);
    virtual int write_h264_raw_frame(char* frame, int frame_size, u_int32_t dts, u_int32_t pts);
    virtual int write_h264_sps_pps(u_int32_t dts, u_int32_t pts);
    virtual int write_h264_ipb_frame(char* frame, int frame_size, u_int32_t dts, u_int32_t pts);
    virtual int rtmp_write_packet(char type, u_int32_t timestamp, char* data, int size);
private:
    // connect to rtmp output url. 
    // @remark ignore when not connected, reconnect when disconnected.
    virtual int connect();
    virtual int connect_app(std::string ep_server, std::string ep_port);
    // close the connected io and rtmp to ready to be re-connect.
    virtual void close();
};

#endif

#endif
