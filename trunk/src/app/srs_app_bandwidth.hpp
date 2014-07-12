/*
The MIT License (MIT)

Copyright (c) 2013-2014 wenjiegit

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

#ifndef SRS_APP_BANDWIDTH_HPP
#define SRS_APP_BANDWIDTH_HPP

/*
#include <srs_app_bandwidth.hpp>
*/
#include <srs_core.hpp>

#include <string>

#include <srs_app_st.hpp>

class SrsRequest;
class SrsRtmpServer;

/**
* bandwidth test agent which provides the interfaces for bandwidth check.
* 1. if vhost disabled bandwidth check, ignore.
* 2. otherwise, check the key, error if verify failed.
* 3. check the interval limit, error if bandwidth in the interval window.
* 4. check the bandwidth under the max kbps.
* 5. send the bandwidth data to client.
* bandwidth workflow:
*  +------------+             +----------+
*  |  Client    |             |  Server  |
*  +-----+------+             +-----+----+
*        |                          |
*        |  connect vhost------>    | if vhost enable bandwidth,
*        |  <-----result(success)   | do bandwidth check.
*        |                          |
*        |  <----call(start play)   | onSrsBandCheckStartPlayBytes
*        |  result(playing)----->   | onSrsBandCheckStartingPlayBytes
*        |  <-------data(playing)   | onSrsBandCheckStartingPlayBytes
*        |  <-----call(stop play)   | onSrsBandCheckStopPlayBytes
*        |  result(stopped)----->   | onSrsBandCheckStoppedPlayBytes
*        |                          |
*        |  <-call(start publish)   | onSrsBandCheckStartPublishBytes
*        |  result(publishing)-->   | onSrsBandCheckStartingPublishBytes
*        |  data(publishing)---->   | onSrsBandCheckStartingPublishBytes
*        |  <--call(stop publish)   | onSrsBandCheckStopPublishBytes
*        |  result(stopped)(1)-->   | onSrsBandCheckStoppedPublishBytes
*        |                          |
*        |  <--------------report   |
*        |  final(2)------------>   | finalClientPacket
*        |          <END>           |
*
* 1. when flash client, server never wait the stop publish response,
*   for the flash client queue is fullfill with other packets.
* 2. when flash client, server never wait the final packet,
*   for the flash client directly close when got report packet.
*/
class SrsBandwidth
{
private:
    SrsRequest* _req;
    SrsRtmpServer* _rtmp;
public:
    SrsBandwidth();
    virtual ~SrsBandwidth();
public:
    /**
    * do the bandwidth check.
    * @param rtmp, server RTMP protocol object, send/recv RTMP packet to/from client.
    * @param req, client request object, specifies the request info from client.
    * @param local_ip, the ip of server which client connected at
    */
    virtual int bandwidth_check(SrsRtmpServer* rtmp, SrsRequest* req, std::string local_ip);
private:
    /**
    * used to process band width check from client.
    */
    virtual int do_bandwidth_check();
    virtual int check_play(int duration_ms, int interval_ms, int& actual_duration_ms, int& play_bytes, int max_play_kbps);
    virtual int check_publish(int duration_ms, int interval_ms, int& actual_duration_ms, int& publish_bytes, int max_pub_kbps);
};

#endif