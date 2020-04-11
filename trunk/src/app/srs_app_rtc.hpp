/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2013-2020 John
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef SRS_APP_RTC_HPP
#define SRS_APP_RTC_HPP

#include <srs_core.hpp>

#include <string>
#include <vector>
#include <map>

class SrsFormat;
class SrsSample;
class SrsSharedPtrMessage;
class SrsRtpSharedPacket;
class SrsRequest;
class SrsOriginHub;
class SrsAudioRecode;
class SrsBuffer;

// Rtp packet max payload size, not include rtp header.
// Must left some bytes to payload header, rtp header, udp header, ip header.
const int kRtpMaxPayloadSize    = 1200;
const int kRtpPacketSize        = 1500;

// Payload type will rewrite in srs_app_rtc_conn.cpp when send to client.
const uint8_t kOpusPayloadType  = 111;
const uint8_t kH264PayloadType  = 102;

// H.264 nalu header type mask.
const uint8_t kNalTypeMask      = 0x1F;

// @see: https://tools.ietf.org/html/rfc6184#section-5.2
const uint8_t kStapA            = 24;
const uint8_t kFuA              = 28;

// @see: https://tools.ietf.org/html/rfc6184#section-5.8
const uint8_t kStart            = 0x80; // Fu-header start bit
const uint8_t kEnd              = 0x40; // Fu-header end bit

const int kChannel              = 2;
const int kSamplerate           = 48000;

// SSRC will rewrite in srs_app_rtc_conn.cpp when send to client.
const uint32_t kAudioSSRC       = 1;
const uint32_t kVideoSSRC       = 2;

// TODO: Define interface class like ISrsRtpMuxer
class SrsRtpH264Muxer
{
private:
    uint16_t sequence;
    std::string sps;
    std::string pps;
public:
    bool discard_bframe;
public:
    SrsRtpH264Muxer();
    virtual ~SrsRtpH264Muxer();
public:
    srs_error_t frame_to_packet(SrsSharedPtrMessage* shared_video, SrsFormat* format);
private:
    srs_error_t packet_fu_a(SrsSharedPtrMessage* shared_frame, SrsFormat* format, SrsSample* sample, std::vector<SrsRtpSharedPacket*>& rtp_packets);
    srs_error_t packet_single_nalu(SrsSharedPtrMessage* shared_frame, SrsFormat* format, SrsSample* sample, std::vector<SrsRtpSharedPacket*>& rtp_packets);
    srs_error_t packet_stap_a(const std::string &sps, const std::string& pps, SrsSharedPtrMessage* shared_frame, std::vector<SrsRtpSharedPacket*>& rtp_packets);
};

// TODO: FIXME: It's not a muxer, but a transcoder.
class SrsRtpOpusMuxer
{
private:
    SrsAudioRecode* codec;
public:
    SrsRtpOpusMuxer();
    virtual ~SrsRtpOpusMuxer();
    virtual srs_error_t initialize();
public:
    srs_error_t transcode(SrsSharedPtrMessage* shared_audio, char* adts_audio, int nn_adts_audio);
};

class SrsRtc
{
private:
    SrsRequest* req;
    bool enabled;
    bool disposable;
    bool discard_aac;
    srs_utime_t last_update_time;
    SrsRtpH264Muxer* rtp_h264_muxer;
    SrsRtpOpusMuxer* rtp_opus_muxer;
    SrsOriginHub* hub;
public:
    SrsRtc();
    virtual ~SrsRtc();
public:
    virtual void dispose();
    virtual srs_error_t cycle();
public:
    virtual srs_error_t initialize(SrsOriginHub* h, SrsRequest* r);
    virtual srs_error_t on_publish();
    virtual void on_unpublish();
    virtual srs_error_t on_audio(SrsSharedPtrMessage* shared_audio, SrsFormat* format);
    virtual srs_error_t on_video(SrsSharedPtrMessage* shared_video, SrsFormat* format);
};

#endif
