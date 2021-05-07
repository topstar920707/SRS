/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2013-2021 John
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

#ifndef SRS_APP_RTC_SOURCE_HPP
#define SRS_APP_RTC_SOURCE_HPP

#include <srs_core.hpp>

#include <vector>
#include <map>
#include <inttypes.h>
#include <vector>
#include <string>
#include <map>

#include <srs_app_rtc_sdp.hpp>
#include <srs_service_st.hpp>
#include <srs_app_source.hpp>
#include <srs_kernel_rtc_rtp.hpp>

class SrsRequest;
class SrsMetaCache;
class SrsSharedPtrMessage;
class SrsCommonMessage;
class SrsMessageArray;
class SrsRtcStream;
class SrsRtcFromRtmpBridger;
class SrsAudioTranscoder;
class SrsRtpPacket2;
class SrsRtpPacketCacheHelper;
class SrsSample;
class SrsRtcStreamDescription;
class SrsRtcTrackDescription;
class SrsRtcConnection;
class SrsRtpRingBuffer;
class SrsRtpNackForReceiver;
class SrsJsonObject;
class SrsErrorPithyPrint;

class SrsNtp
{
public:
    uint64_t system_ms_;
    uint64_t ntp_;
    uint32_t ntp_second_;
    uint32_t ntp_fractions_;
public:
    SrsNtp();
    virtual ~SrsNtp();
public:
    static SrsNtp from_time_ms(uint64_t ms);
    static SrsNtp to_time_ms(uint64_t ntp);
public:
    static uint64_t kMagicNtpFractionalUnit;
};

// When RTC stream publish and re-publish.
class ISrsRtcStreamChangeCallback
{
public:
    ISrsRtcStreamChangeCallback();
    virtual ~ISrsRtcStreamChangeCallback();
public:
    virtual void on_stream_change(SrsRtcStreamDescription* desc) = 0;
};

// The RTC stream consumer, consume packets from RTC stream source.
class SrsRtcConsumer
{
private:
    SrsRtcStream* source;
    std::vector<SrsRtpPacket2*> queue;
    // when source id changed, notice all consumers
    bool should_update_source_id;
    // The cond wait for mw.
    // @see https://github.com/ossrs/srs/issues/251
    srs_cond_t mw_wait;
    bool mw_waiting;
    int mw_min_msgs;
private:
    // The callback for stream change event.
    ISrsRtcStreamChangeCallback* handler_;
public:
    SrsRtcConsumer(SrsRtcStream* s);
    virtual ~SrsRtcConsumer();
public:
    // When source id changed, notice client to print.
    virtual void update_source_id();
    // Put RTP packet into queue.
    // @note We do not drop packet here, but drop it in sender.
    srs_error_t enqueue(SrsRtpPacket2* pkt);
    // For RTC, we only got one packet, because there is not many packets in queue.
    virtual srs_error_t dump_packet(SrsRtpPacket2** ppkt);
    // Wait for at-least some messages incoming in queue.
    virtual void wait(int nb_msgs);
public:
    void set_handler(ISrsRtcStreamChangeCallback* h) { handler_ = h; } // SrsRtcConsumer::set_handler()
    void on_stream_change(SrsRtcStreamDescription* desc);
};

class SrsRtcStreamManager
{
private:
    srs_mutex_t lock;
    std::map<std::string, SrsRtcStream*> pool;
public:
    SrsRtcStreamManager();
    virtual ~SrsRtcStreamManager();
public:
    //  create source when fetch from cache failed.
    // @param r the client request.
    // @param pps the matched source, if success never be NULL.
    virtual srs_error_t fetch_or_create(SrsRequest* r, SrsRtcStream** pps);
private:
    // Get the exists source, NULL when not exists.
    // update the request and return the exists source.
    virtual SrsRtcStream* fetch(SrsRequest* r);
};

// Global singleton instance.
extern SrsRtcStreamManager* _srs_rtc_sources;

// A publish stream interface, for source to callback with.
class ISrsRtcPublishStream
{
public:
    ISrsRtcPublishStream();
    virtual ~ISrsRtcPublishStream();
public:
    // Request keyframe(PLI) from publisher, for fresh consumer.
    virtual void request_keyframe(uint32_t ssrc) = 0;
};

class ISrsRtcStreamEventHandler
{
public:
    ISrsRtcStreamEventHandler();
    virtual ~ISrsRtcStreamEventHandler();
public:
    // stream unpublish, sync API.
    virtual void on_unpublish() = 0;
    // no player subscribe this stream, sync API
    virtual void on_consumers_finished() = 0;
};

// SrsRtcStream bridge to SrsSource
class ISrsRtcSourceBridger
{
public:
    ISrsRtcSourceBridger();
    virtual ~ISrsRtcSourceBridger();
public:
    virtual srs_error_t on_publish() = 0;
    virtual srs_error_t on_rtp(SrsRtpPacket2 *pkt) = 0;
    virtual void on_unpublish() = 0;
};

// A Source is a stream, to publish and to play with, binding to SrsRtcPublishStream and SrsRtcPlayStream.
class SrsRtcStream : public ISrsFastTimer
{
private:
    // For publish, it's the publish client id.
    // For edge, it's the edge ingest id.
    // when source id changed, for example, the edge reconnect,
    // invoke the on_source_changed() to let all clients know.
    SrsContextId _source_id;
    // previous source id.
    SrsContextId _pre_source_id;
    SrsRequest* req;
    ISrsRtcPublishStream* publish_stream_;
    // Steam description for this steam.
    SrsRtcStreamDescription* stream_desc_;
    // The Source bridger, bridger stream to other source.
    ISrsRtcSourceBridger* bridger_;
private:
    // To delivery stream to clients.
    std::vector<SrsRtcConsumer*> consumers;
    // Whether stream is created, that is, SDP is done.
    bool is_created_;
    // Whether stream is delivering data, that is, DTLS is done.
    bool is_delivering_packets_;
    // Notify stream event to event handler
    std::vector<ISrsRtcStreamEventHandler*> event_handlers_;
private:
    // The PLI for RTC2RTMP.
    srs_utime_t pli_for_rtmp_;
    srs_utime_t pli_elapsed_;
public:
    SrsRtcStream();
    virtual ~SrsRtcStream();
public:
    virtual srs_error_t initialize(SrsRequest* r);
    // Update the authentication information in request.
    virtual void update_auth(SrsRequest* r);
private:
    // The stream source changed.
    virtual srs_error_t on_source_changed();
public:
    // Get current source id.
    virtual SrsContextId source_id();
    virtual SrsContextId pre_source_id();
public:
    void set_bridger(ISrsRtcSourceBridger *bridger);
public:
    // Create consumer
    // @param consumer, output the create consumer.
    virtual srs_error_t create_consumer(SrsRtcConsumer*& consumer);
    // Dumps packets in cache to consumer.
    // @param ds, whether dumps the sequence header.
    // @param dm, whether dumps the metadata.
    // @param dg, whether dumps the gop cache.
    virtual srs_error_t consumer_dumps(SrsRtcConsumer* consumer, bool ds = true, bool dm = true, bool dg = true);
    virtual void on_consumer_destroy(SrsRtcConsumer* consumer);
    // Whether we can publish stream to the source, return false if it exists.
    // @remark Note that when SDP is done, we set the stream is not able to publish.
    virtual bool can_publish();
    // For RTC, the stream is created when SDP is done, and then do DTLS
    virtual void set_stream_created();
    // When start publish stream.
    virtual srs_error_t on_publish();
    // When stop publish stream.
    virtual void on_unpublish();
public:
    // For event handler
    void subscribe(ISrsRtcStreamEventHandler* h);
    void unsubscribe(ISrsRtcStreamEventHandler* h);
public:
    // Get and set the publisher, passed to consumer to process requests such as PLI.
    ISrsRtcPublishStream* publish_stream();
    void set_publish_stream(ISrsRtcPublishStream* v);
    // Consume the shared RTP packet, user must free it.
    srs_error_t on_rtp(SrsRtpPacket2* pkt);
    // Set and get stream description for souce
    bool has_stream_desc();
    void set_stream_desc(SrsRtcStreamDescription* stream_desc);
    std::vector<SrsRtcTrackDescription*> get_track_desc(std::string type, std::string media_type);
// interface ISrsFastTimer
private:
    srs_error_t on_timer(srs_utime_t interval);
};

// A helper class, to release the packet to cache.
class SrsRtpPacketCacheHelper
{
public:
    SrsRtpPacket2* pkt;
public:
    SrsRtpPacketCacheHelper();
    virtual ~SrsRtpPacketCacheHelper();
};

#ifdef SRS_FFMPEG_FIT
class SrsRtcFromRtmpBridger : public ISrsSourceBridger
{
private:
    SrsRequest* req;
    SrsRtcStream* source_;
    // The format, codec information.
    SrsRtmpFormat* format;
    // The metadata cache.
    SrsMetaCache* meta;
private:
    bool discard_aac;
    SrsAudioTranscoder* codec_;
    bool discard_bframe;
    bool merge_nalus;
    uint16_t audio_sequence;
    uint16_t video_sequence;
    uint32_t audio_ssrc;
    uint32_t video_ssrc;
public:
    SrsRtcFromRtmpBridger(SrsRtcStream* source);
    virtual ~SrsRtcFromRtmpBridger();
public:
    virtual srs_error_t initialize(SrsRequest* r);
    virtual srs_error_t on_publish();
    virtual void on_unpublish();
    virtual srs_error_t on_audio(SrsSharedPtrMessage* msg);
private:
    srs_error_t transcode(SrsAudioFrame* audio);
    srs_error_t package_opus(SrsAudioFrame* audio, SrsRtpPacketCacheHelper* helper);
public:
    virtual srs_error_t on_video(SrsSharedPtrMessage* msg);
private:
    srs_error_t filter(SrsSharedPtrMessage* msg, SrsFormat* format, bool& has_idr, std::vector<SrsSample*>& samples);
    srs_error_t package_stap_a(SrsRtcStream* source, SrsSharedPtrMessage* msg, SrsRtpPacketCacheHelper* helper);
    srs_error_t package_nalus(SrsSharedPtrMessage* msg, const std::vector<SrsSample*>& samples, std::vector<SrsRtpPacketCacheHelper*>& helpers);
    srs_error_t package_single_nalu(SrsSharedPtrMessage* msg, SrsSample* sample, std::vector<SrsRtpPacketCacheHelper*>& helpers);
    srs_error_t package_fu_a(SrsSharedPtrMessage* msg, SrsSample* sample, int fu_payload_size, std::vector<SrsRtpPacketCacheHelper*>& helpers);
    srs_error_t consume_packets(std::vector<SrsRtpPacketCacheHelper*>& helpers);
};

class SrsRtmpFromRtcBridger : public ISrsRtcSourceBridger
{
private:
    SrsSource *source_;
    SrsAudioTranscoder *codec_;
    bool is_first_audio;
    bool is_first_video;
    // The format, codec information.
    SrsRtmpFormat* format;

    //TODO:use SrsRtpRingBuffer
    //TODO:jitter buffer class
    struct RtcPacketCache {
        bool in_use;
        uint16_t sn;
        uint32_t ts;
        SrsRtpPacket2* pkt;
    };
    const static uint16_t s_cache_size = 512;
    RtcPacketCache cache_video_pkts_[s_cache_size];
    uint16_t header_sn_;
    uint16_t lost_sn_;
    int64_t key_frame_ts_;
public:
    SrsRtmpFromRtcBridger(SrsSource *src);
    virtual ~SrsRtmpFromRtcBridger();
public:
    srs_error_t initialize(SrsRequest* r);
public:
    virtual srs_error_t on_publish();
    virtual srs_error_t on_rtp(SrsRtpPacket2 *pkt);
    virtual void on_unpublish();
private:
    srs_error_t trancode_audio(SrsRtpPacket2 *pkt);
    void packet_aac(SrsCommonMessage* audio, char* data, int len, uint32_t pts, bool is_header);
    srs_error_t packet_video(SrsRtpPacket2* pkt);
    srs_error_t packet_video_key_frame(SrsRtpPacket2* pkt);
    srs_error_t packet_video_rtmp(const uint16_t start, const uint16_t end);
    int32_t find_next_lost_sn(uint16_t current_sn, uint16_t& end_sn);
    void clear_cached_video();
    inline uint16_t cache_index(uint16_t current_sn) {
        return current_sn%s_cache_size;
    }
    bool check_frame_complete(const uint16_t start, const uint16_t end);
};
#endif

// TODO: FIXME: Rename it.
class SrsCodecPayload
{
public:
    std::string type_;
    uint8_t pt_;
    // for publish, equals to PT of itself;
    // for subscribe, is the PT of publisher;
    uint8_t pt_of_publisher_;
    std::string name_;
    int sample_;

    std::vector<std::string> rtcp_fbs_;
public:
    SrsCodecPayload();
    SrsCodecPayload(uint8_t pt, std::string encode_name, int sample);
    virtual ~SrsCodecPayload();
public:
    virtual SrsCodecPayload* copy();
    virtual SrsMediaPayloadType generate_media_payload_type();
};

// TODO: FIXME: Rename it.
class SrsVideoPayload : public SrsCodecPayload
{
public:
    struct H264SpecificParameter
    {
        std::string profile_level_id;
        std::string packetization_mode;
        std::string level_asymmerty_allow;
    };
    H264SpecificParameter h264_param_;

public:
    SrsVideoPayload();
    SrsVideoPayload(uint8_t pt, std::string encode_name, int sample);
    virtual ~SrsVideoPayload();
public:
    virtual SrsVideoPayload* copy();
    virtual SrsMediaPayloadType generate_media_payload_type();
public:
    srs_error_t set_h264_param_desc(std::string fmtp);
};

// TODO: FIXME: Rename it.
class SrsAudioPayload : public SrsCodecPayload
{
    struct SrsOpusParameter
    {
        int minptime;
        bool use_inband_fec;
        bool usedtx;

        SrsOpusParameter() {
            minptime = 0;
            use_inband_fec = false;
            usedtx = false;
        }
    };

public:
    int channel_;
    SrsOpusParameter opus_param_;
public:
    SrsAudioPayload();
    SrsAudioPayload(uint8_t pt, std::string encode_name, int sample, int channel);
    virtual ~SrsAudioPayload();
public:
    virtual SrsAudioPayload* copy();
    virtual SrsMediaPayloadType generate_media_payload_type();
public:
    srs_error_t set_opus_param_desc(std::string fmtp);
};

// TODO: FIXME: Rename it.
class SrsRedPayload : public SrsCodecPayload
{
public:
    int channel_;
public:
    SrsRedPayload();
    SrsRedPayload(uint8_t pt, std::string encode_name, int sample, int channel);
    virtual ~SrsRedPayload();
public:
    virtual SrsRedPayload* copy();
    virtual SrsMediaPayloadType generate_media_payload_type();
};

class SrsRtxPayloadDes : public SrsCodecPayload
{
public:
    uint8_t apt_;
public:
    SrsRtxPayloadDes();
    SrsRtxPayloadDes(uint8_t pt, uint8_t apt);
    virtual ~SrsRtxPayloadDes();

public:
    virtual SrsRtxPayloadDes* copy();
    virtual SrsMediaPayloadType generate_media_payload_type();
};

class SrsRtcTrackDescription
{
public:
    // type: audio, video
    std::string type_;
    // track_id
    std::string id_;
    // ssrc is the primary ssrc for this track,
    // if sdp has ssrc-group, it is the first ssrc of the ssrc-group
    uint32_t ssrc_;
    // rtx ssrc is the second ssrc of "FEC" src-group,
    // if no rtx ssrc, rtx_ssrc_ = 0.
    uint32_t fec_ssrc_;
    // rtx ssrc is the second ssrc of "FID" src-group,
    // if no rtx ssrc, rtx_ssrc_ = 0.
    uint32_t rtx_ssrc_;
    // key: rtp header extension id, value: rtp header extension uri.
    std::map<int, std::string> extmaps_;
    // Whether this track active. default: active.
    bool is_active_;
    // direction
    std::string direction_;
    // mid is used in BOUNDLE
    std::string mid_;
    // msid_: track stream id
    std::string msid_;

    // meida payload, such as opus, h264.
    SrsCodecPayload* media_;
    SrsCodecPayload* red_;
    SrsCodecPayload* rtx_;
    SrsCodecPayload* ulpfec_;
public:
    SrsRtcTrackDescription();
    virtual ~SrsRtcTrackDescription();
public:
    // whether or not the track has ssrc.
    // for example:
    //    we need check track has the ssrc in the ssrc_group, then add ssrc_group to the track,
    bool has_ssrc(uint32_t ssrc);
public:
    void add_rtp_extension_desc(int id, std::string uri);
    void del_rtp_extension_desc(std::string uri);
    void set_direction(std::string direction);
    void set_codec_payload(SrsCodecPayload* payload);
    // auxiliary paylod include red, rtx, ulpfec.
    void create_auxiliary_payload(const std::vector<SrsMediaPayloadType> payload_types);
    void set_rtx_ssrc(uint32_t ssrc);
    void set_fec_ssrc(uint32_t ssrc);
    void set_mid(std::string mid);
    int get_rtp_extension_id(std::string uri);
public:
    SrsRtcTrackDescription* copy();
};

class SrsRtcStreamDescription
{
public:
    // the id for this stream;
    std::string id_;

    SrsRtcTrackDescription* audio_track_desc_;
    std::vector<SrsRtcTrackDescription*> video_track_descs_;
public:
    SrsRtcStreamDescription();
    virtual ~SrsRtcStreamDescription();

public:
    SrsRtcStreamDescription* copy();
    SrsRtcTrackDescription* find_track_description_by_ssrc(uint32_t ssrc);
};

class SrsRtcTrackStatistic
{
public:
	// packets received or sent.
	uint32_t packets;
	// packets received or sent at last statistic time.
    uint32_t last_packets;
    // bytes received or sent.
    uint64_t bytes;
    // bytes received or sent at last statistic time.
    uint32_t last_bytes;

    // nacks received or sent.
	uint32_t nacks;
    // nacks received or sent at last statistic time.
    uint32_t last_nacks;

    // padding packets received or sent.
	uint32_t padding_packets;
    // padding packets received or sent at last statistic time.
    uint32_t last_padding_packets;
    // padding bytes received or sent.
	uint32_t padding_bytes;
    // padding bytes received or sent at last statistic time.
    uint32_t last_padding_bytes;

    // replay packets received or sent.
	uint32_t replay_packets;
    // replay packets received or sent at last statistic time.
    uint32_t last_replay_packets;
    // replay bytes received or sent.
	uint64_t replay_bytes;
    // replay bytes received or sent at last statistic time.
    uint64_t last_replay_bytes;

public:
    SrsRtcTrackStatistic();
};

class SrsRtcRecvTrack
{
protected:
    SrsRtcTrackDescription* track_desc_;
    SrsRtcTrackStatistic* statistic_;
protected:
    SrsRtcConnection* session_;
    SrsRtpRingBuffer* rtp_queue_;
    SrsRtpNackForReceiver* nack_receiver_;
private:
    // By config, whether no copy.
    bool nack_no_copy_;
protected:
    // send report ntp and received time.
    SrsNtp last_sender_report_ntp;
    uint64_t last_sender_report_sys_time;
public:
    SrsRtcRecvTrack(SrsRtcConnection* session, SrsRtcTrackDescription* stream_descs, bool is_audio);
    virtual ~SrsRtcRecvTrack();
public:
    // SrsRtcSendTrack::set_nack_no_copy
    void set_nack_no_copy(bool v) { nack_no_copy_ = v; }
    bool has_ssrc(uint32_t ssrc);
    uint32_t get_ssrc();
    void update_rtt(int rtt);
    void update_send_report_time(const SrsNtp& ntp);
    srs_error_t send_rtcp_rr();
    srs_error_t send_rtcp_xr_rrtr();
    bool set_track_status(bool active);
    bool get_track_status();
    std::string get_track_id();
public:
    // Note that we can set the pkt to NULL to avoid copy, for example, if the NACK cache the pkt and
    // set to NULL, nack nerver copy it but set the pkt to NULL.
    srs_error_t on_nack(SrsRtpPacket2** ppkt);
public:
    virtual srs_error_t on_rtp(SrsRtcStream* source, SrsRtpPacket2* pkt) = 0;
    virtual srs_error_t check_send_nacks() = 0;
protected:
    virtual srs_error_t do_check_send_nacks(uint32_t& timeout_nacks);
};

class SrsRtcAudioRecvTrack : virtual public SrsRtcRecvTrack, virtual public ISrsRtpPacketDecodeHandler
{
public:
    SrsRtcAudioRecvTrack(SrsRtcConnection* session, SrsRtcTrackDescription* track_desc);
    virtual ~SrsRtcAudioRecvTrack();
public:
    virtual void on_before_decode_payload(SrsRtpPacket2* pkt, SrsBuffer* buf, ISrsRtpPayloader** ppayload, SrsRtpPacketPayloadType* ppt);
public:
    virtual srs_error_t on_rtp(SrsRtcStream* source, SrsRtpPacket2* pkt);
    virtual srs_error_t check_send_nacks();
};

class SrsRtcVideoRecvTrack : virtual public SrsRtcRecvTrack, virtual public ISrsRtpPacketDecodeHandler
{
public:
    SrsRtcVideoRecvTrack(SrsRtcConnection* session, SrsRtcTrackDescription* stream_descs);
    virtual ~SrsRtcVideoRecvTrack();
public:
    virtual void on_before_decode_payload(SrsRtpPacket2* pkt, SrsBuffer* buf, ISrsRtpPayloader** ppayload, SrsRtpPacketPayloadType* ppt);
public:
    virtual srs_error_t on_rtp(SrsRtcStream* source, SrsRtpPacket2* pkt);
    virtual srs_error_t check_send_nacks();
};

class SrsRtcSendTrack
{
protected:
    // send track description
    SrsRtcTrackDescription* track_desc_;
    SrsRtcTrackStatistic* statistic_;
protected:
    // The owner connection for this track.
    SrsRtcConnection* session_;
    // NACK ARQ ring buffer.
    SrsRtpRingBuffer* rtp_queue_;
private:
    // By config, whether no copy.
    bool nack_no_copy_;
    // The pithy print for special stage.
    SrsErrorPithyPrint* nack_epp;
public:
    SrsRtcSendTrack(SrsRtcConnection* session, SrsRtcTrackDescription* track_desc, bool is_audio);
    virtual ~SrsRtcSendTrack();
public:
    // SrsRtcSendTrack::set_nack_no_copy
    void set_nack_no_copy(bool v) { nack_no_copy_ = v; }
    bool has_ssrc(uint32_t ssrc);
    SrsRtpPacket2* fetch_rtp_packet(uint16_t seq);
    bool set_track_status(bool active);
    bool get_track_status();
    std::string get_track_id();
public:
    // Note that we can set the pkt to NULL to avoid copy, for example, if the NACK cache the pkt and
    // set to NULL, nack nerver copy it but set the pkt to NULL.
    srs_error_t on_nack(SrsRtpPacket2** ppkt);
public:
    virtual srs_error_t on_rtp(SrsRtpPacket2* pkt) = 0;
    virtual srs_error_t on_rtcp(SrsRtpPacket2* pkt) = 0;
    virtual srs_error_t on_recv_nack(const std::vector<uint16_t>& lost_seqs);
};

class SrsRtcAudioSendTrack : public SrsRtcSendTrack
{
public:
    SrsRtcAudioSendTrack(SrsRtcConnection* session, SrsRtcTrackDescription* track_desc);
    virtual ~SrsRtcAudioSendTrack();
public:
    virtual srs_error_t on_rtp(SrsRtpPacket2* pkt);
    virtual srs_error_t on_rtcp(SrsRtpPacket2* pkt);
};

class SrsRtcVideoSendTrack : public SrsRtcSendTrack
{
public:
    SrsRtcVideoSendTrack(SrsRtcConnection* session, SrsRtcTrackDescription* track_desc);
    virtual ~SrsRtcVideoSendTrack();
public:
    virtual srs_error_t on_rtp(SrsRtpPacket2* pkt);
    virtual srs_error_t on_rtcp(SrsRtpPacket2* pkt);
};

class SrsRtcSSRCGenerator
{
private:
    static SrsRtcSSRCGenerator* _instance;
private:
    uint32_t ssrc_num;
private:
    SrsRtcSSRCGenerator();
    virtual ~SrsRtcSSRCGenerator();
public:
    static SrsRtcSSRCGenerator* instance();
    uint32_t generate_ssrc();
};

#endif

