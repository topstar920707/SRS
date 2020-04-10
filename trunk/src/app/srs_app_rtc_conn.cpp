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

#include <srs_app_rtc_conn.hpp>

using namespace std;

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include <sstream>

#include <srs_core_autofree.hpp>
#include <srs_kernel_buffer.hpp>
#include <srs_kernel_rtp.hpp>
#include <srs_kernel_error.hpp>
#include <srs_kernel_log.hpp>
#include <srs_stun_stack.hpp>
#include <srs_rtmp_stack.hpp>
#include <srs_rtmp_msg_array.hpp>
#include <srs_app_dtls.hpp>
#include <srs_app_utility.hpp>
#include <srs_app_config.hpp>
#include <srs_app_rtc.hpp>
#include <srs_app_source.hpp>
#include <srs_app_server.hpp>
#include <srs_service_utility.hpp>
#include <srs_http_stack.hpp>
#include <srs_app_http_api.hpp>
#include <srs_app_statistic.hpp>
#include <srs_app_pithy_print.hpp>

static bool is_stun(const uint8_t* data, const int size) 
{
    return data != NULL && size > 0 && (data[0] == 0 || data[0] == 1); 
}

static bool is_dtls(const uint8_t* data, size_t len) 
{
      return (len >= 13 && (data[0] > 19 && data[0] < 64));
}

static bool is_rtp_or_rtcp(const uint8_t* data, size_t len) 
{
      return (len >= 12 && (data[0] & 0xC0) == 0x80);
}

static bool is_rtcp(const uint8_t* data, size_t len)
{
    return (len >= 12) && (data[0] & 0x80) && (data[1] >= 200 && data[1] <= 209);
}

static string gen_random_str(int len)
{
    static string random_table = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

    string ret;
    ret.reserve(len);
    for (int i = 0; i < len; ++i) {
        ret.append(1, random_table[random() % random_table.size()]);
    }

    return ret;
}

const int SRTP_MASTER_KEY_KEY_LEN = 16;
const int SRTP_MASTER_KEY_SALT_LEN = 14;

static std::vector<std::string> get_candidate_ips()
{
    std::vector<std::string> candidate_ips;

    string candidate = _srs_config->get_rtc_server_candidates();
    if (candidate == "*" || candidate == "0.0.0.0") {
        std::vector<std::string> tmp = srs_get_local_ips();
        for (int i = 0; i < (int)tmp.size(); ++i) {
            if (tmp[i] != "127.0.0.1") {
                candidate_ips.push_back(tmp[i]);
            }
        }
    } else {
        candidate_ips.push_back(candidate);
    }

    return candidate_ips;
}

SrsDtlsSession::SrsDtlsSession(SrsRtcSession* s)
{
    rtc_session = s;

    dtls = NULL;
    bio_in = NULL;
    bio_out = NULL;

    client_key = "";
    server_key = "";

    srtp_send = NULL;
    srtp_recv = NULL;

    handshake_done = false;
}

SrsDtlsSession::~SrsDtlsSession()
{
    if (dtls) {
        // this function will free bio_in and bio_out
        SSL_free(dtls);
        dtls = NULL;
    }

    if (srtp_send) {
        srtp_dealloc(srtp_send);
    }

    if (srtp_recv) {
        srtp_dealloc(srtp_recv);
    }
}

srs_error_t SrsDtlsSession::initialize(const SrsRequest& req)
{    
    srs_error_t err = srs_success;

    if ((err = SrsDtls::instance()->init(req)) != srs_success) {
        return srs_error_wrap(err, "DTLS init");
    }

    // TODO: FIXME: Support config by vhost to use RSA or ECDSA certificate.
    if ((dtls = SSL_new(SrsDtls::instance()->get_dtls_ctx())) == NULL) {
        return srs_error_new(ERROR_OpenSslCreateSSL, "SSL_new dtls");
    }

    // Dtls setup passive, as server role.
    SSL_set_accept_state(dtls);

    if ((bio_in = BIO_new(BIO_s_mem())) == NULL) {
        return srs_error_new(ERROR_OpenSslBIONew, "BIO_new in");
    }

    if ((bio_out = BIO_new(BIO_s_mem())) == NULL) {
        BIO_free(bio_in);
        return srs_error_new(ERROR_OpenSslBIONew, "BIO_new out");
    }

    SSL_set_bio(dtls, bio_in, bio_out);

    return err;
}

srs_error_t SrsDtlsSession::handshake(SrsUdpMuxSocket* skt)
{
    srs_error_t err = srs_success;

    int ret = SSL_do_handshake(dtls);

    unsigned char *out_bio_data;
    int out_bio_len = BIO_get_mem_data(bio_out, &out_bio_data);

    int ssl_err = SSL_get_error(dtls, ret); 
    switch(ssl_err) {   
        case SSL_ERROR_NONE: {   
            if ((err = on_dtls_handshake_done(skt)) != srs_success) {
                return srs_error_wrap(err, "dtls handshake done handle");
            }
            break;
        }  

        case SSL_ERROR_WANT_READ: {   
            break;
        }   

        case SSL_ERROR_WANT_WRITE: {   
            break;
        }

        default: {   
            break;
        }   
    }   

    if (out_bio_len) {   
        if ((err = skt->sendto(out_bio_data, out_bio_len, 0)) != srs_success) {
            return srs_error_wrap(err, "send dtls packet");
        }
    }

    return err;
}

srs_error_t SrsDtlsSession::on_dtls(SrsUdpMuxSocket* skt)
{
    srs_error_t err = srs_success;
    if (BIO_reset(bio_in) != 1) {
        return srs_error_new(ERROR_OpenSslBIOReset, "BIO_reset");
    }
    if (BIO_reset(bio_out) != 1) {
        return srs_error_new(ERROR_OpenSslBIOReset, "BIO_reset");
    }

    if (BIO_write(bio_in, skt->data(), skt->size()) <= 0) {
        // TODO: 0 or -1 maybe block, use BIO_should_retry to check.
        return srs_error_new(ERROR_OpenSslBIOWrite, "BIO_write");
    }

    if (! handshake_done) {
        err = handshake(skt);
    } else {
        while (BIO_ctrl_pending(bio_in) > 0) {
            char dtls_read_buf[8092];
            int nb = SSL_read(dtls, dtls_read_buf, sizeof(dtls_read_buf));

            if (nb > 0) {
                if ((err =on_dtls_application_data(dtls_read_buf, nb)) != srs_success) {
                    return srs_error_wrap(err, "dtls application data process");
                }
            }
        }
    }

    return err;
}

srs_error_t SrsDtlsSession::on_dtls_handshake_done(SrsUdpMuxSocket* skt)
{
    srs_error_t err = srs_success;
    srs_trace("dtls handshake done");

    handshake_done = true;
    if ((err = srtp_initialize()) != srs_success) {
        return srs_error_wrap(err, "srtp init failed");
    }

    return rtc_session->on_connection_established(skt);
}

srs_error_t SrsDtlsSession::on_dtls_application_data(const char* buf, const int nb_buf)
{
    srs_error_t err = srs_success;

    // TODO: process SCTP protocol(WebRTC datachannel support)

    return err;
}

srs_error_t SrsDtlsSession::srtp_initialize() 
{
    srs_error_t err = srs_success;

    unsigned char material[SRTP_MASTER_KEY_LEN * 2] = {0};  // client(SRTP_MASTER_KEY_KEY_LEN + SRTP_MASTER_KEY_SALT_LEN) + server
    static const string dtls_srtp_lable = "EXTRACTOR-dtls_srtp";
    if (! SSL_export_keying_material(dtls, material, sizeof(material), dtls_srtp_lable.c_str(), dtls_srtp_lable.size(), NULL, 0, 0)) {   
        return srs_error_new(ERROR_RTC_SRTP_INIT, "SSL_export_keying_material failed");
    }   

    size_t offset = 0;

    std::string client_master_key(reinterpret_cast<char*>(material), SRTP_MASTER_KEY_KEY_LEN);
    offset += SRTP_MASTER_KEY_KEY_LEN;
    std::string server_master_key(reinterpret_cast<char*>(material + offset), SRTP_MASTER_KEY_KEY_LEN);
    offset += SRTP_MASTER_KEY_KEY_LEN;
    std::string client_master_salt(reinterpret_cast<char*>(material + offset), SRTP_MASTER_KEY_SALT_LEN);
    offset += SRTP_MASTER_KEY_SALT_LEN;
    std::string server_master_salt(reinterpret_cast<char*>(material + offset), SRTP_MASTER_KEY_SALT_LEN);

    client_key = client_master_key + client_master_salt;
    server_key = server_master_key + server_master_salt;

    if ((err = srtp_send_init()) != srs_success) {
        return srs_error_wrap(err, "srtp send init failed");
    }

    if ((err = srtp_recv_init()) != srs_success) {
        return srs_error_wrap(err, "srtp recv init failed");
    }

    return err;
}

srs_error_t SrsDtlsSession::srtp_send_init()
{
    srs_error_t err = srs_success;

    srtp_policy_t policy;
    bzero(&policy, sizeof(policy));

    // TODO: Maybe we can use SRTP-GCM in future.
    // @see https://bugs.chromium.org/p/chromium/issues/detail?id=713701
    // @see https://groups.google.com/forum/#!topic/discuss-webrtc/PvCbWSetVAQ
    srtp_crypto_policy_set_aes_cm_128_hmac_sha1_80(&policy.rtp);
    srtp_crypto_policy_set_aes_cm_128_hmac_sha1_80(&policy.rtcp);

    policy.ssrc.type = ssrc_any_outbound;
    
    policy.ssrc.value = 0;
    // TODO: adjust window_size
    policy.window_size = 8192;
    policy.allow_repeat_tx = 1;
    policy.next = NULL;

    uint8_t *key = new uint8_t[server_key.size()];
    memcpy(key, server_key.data(), server_key.size());
    policy.key = key;

    if (srtp_create(&srtp_send, &policy) != srtp_err_status_ok) {
        srs_freepa(key);
        return srs_error_new(ERROR_RTC_SRTP_INIT, "srtp_create failed");
    }

    srs_freepa(key);

    return err;
}

srs_error_t SrsDtlsSession::srtp_recv_init()
{
    srs_error_t err = srs_success;

    srtp_policy_t policy;
    bzero(&policy, sizeof(policy));

    srtp_crypto_policy_set_aes_cm_128_hmac_sha1_80(&policy.rtp);
    srtp_crypto_policy_set_aes_cm_128_hmac_sha1_80(&policy.rtcp);

    policy.ssrc.type = ssrc_any_inbound;

    policy.ssrc.value = 0;
    // TODO: adjust window_size
    policy.window_size = 8192;
    policy.allow_repeat_tx = 1;
    policy.next = NULL;

    uint8_t *key = new uint8_t[client_key.size()];
    memcpy(key, client_key.data(), client_key.size());
    policy.key = key;

    if (srtp_create(&srtp_recv, &policy) != srtp_err_status_ok) {
        srs_freepa(key);
        return srs_error_new(ERROR_RTC_SRTP_INIT, "srtp_create failed");
    }

    srs_freepa(key);

    return err;
}

srs_error_t SrsDtlsSession::protect_rtp(char* out_buf, const char* in_buf, int& nb_out_buf)
{
    srs_error_t err = srs_success;

    if (srtp_send) {
        memcpy(out_buf, in_buf, nb_out_buf);
        if (srtp_protect(srtp_send, out_buf, &nb_out_buf) != 0) {
            return srs_error_new(ERROR_RTC_SRTP_PROTECT, "rtp protect failed");
        }

        return err;
    }

    return srs_error_new(ERROR_RTC_SRTP_PROTECT, "rtp protect failed");
}

srs_error_t SrsDtlsSession::unprotect_rtp(char* out_buf, const char* in_buf, int& nb_out_buf)
{
    srs_error_t err = srs_success;

    if (srtp_recv) {
        memcpy(out_buf, in_buf, nb_out_buf);
        if (srtp_unprotect(srtp_recv, out_buf, &nb_out_buf) != 0) {
            return srs_error_new(ERROR_RTC_SRTP_UNPROTECT, "rtp unprotect failed");
        }

        return err;
    }

    return srs_error_new(ERROR_RTC_SRTP_UNPROTECT, "rtp unprotect failed");
}

srs_error_t SrsDtlsSession::protect_rtcp(char* out_buf, const char* in_buf, int& nb_out_buf)
{
    srs_error_t err = srs_success;

    if (srtp_send) {
        memcpy(out_buf, in_buf, nb_out_buf);
        if (srtp_protect_rtcp(srtp_send, out_buf, &nb_out_buf) != 0) {
            return srs_error_new(ERROR_RTC_SRTP_PROTECT, "rtcp protect failed");
        }

        return err;
    }

    return srs_error_new(ERROR_RTC_SRTP_PROTECT, "rtcp protect failed");
}

srs_error_t SrsDtlsSession::unprotect_rtcp(char* out_buf, const char* in_buf, int& nb_out_buf)
{
    srs_error_t err = srs_success;

    if (srtp_recv) {
        memcpy(out_buf, in_buf, nb_out_buf);
        if (srtp_unprotect_rtcp(srtp_recv, out_buf, &nb_out_buf) != srtp_err_status_ok) {
            return srs_error_new(ERROR_RTC_SRTP_UNPROTECT, "rtcp unprotect failed");
        }

        return err;
    }

    return srs_error_new(ERROR_RTC_SRTP_UNPROTECT, "rtcp unprotect failed");
}

SrsRtcSenderThread::SrsRtcSenderThread(SrsRtcSession* s, SrsUdpMuxSocket* u, int parent_cid)
    : sendonly_ukt(NULL)
{
    _parent_cid = parent_cid;
    trd = new SrsDummyCoroutine();

    rtc_session = s;
    sendonly_ukt = u->copy_sendonly();
}

SrsRtcSenderThread::~SrsRtcSenderThread()
{
    srs_freep(trd);
    srs_freep(sendonly_ukt);
}

srs_error_t SrsRtcSenderThread::initialize(const uint32_t& vssrc, const uint32_t& assrc, const uint16_t& v_pt, const uint16_t& a_pt)
{
    srs_error_t err = srs_success;

    video_ssrc = vssrc;
    audio_ssrc = assrc;

    video_payload_type = v_pt;
    audio_payload_type = a_pt;

    return err;
}

int SrsRtcSenderThread::cid()
{
    return trd->cid();
}

srs_error_t SrsRtcSenderThread::start()
{
    srs_error_t err = srs_success;
    
    srs_freep(trd);
    trd = new SrsSTCoroutine("rtc_sender", this, _parent_cid);
    
    if ((err = trd->start()) != srs_success) {
        return srs_error_wrap(err, "rtc_sender");
    }
    
    return err;
}

void SrsRtcSenderThread::stop()
{
    trd->stop();
}

void SrsRtcSenderThread::stop_loop()
{
    trd->interrupt();
}


srs_error_t SrsRtcSenderThread::cycle()
{
    srs_error_t err = srs_success;

    SrsSource* source = NULL;

    // TODO: FIXME: Should refactor it, directly use http server as handler.
    ISrsSourceHandler* handler = _srs_hybrid->srs()->instance();
    if ((err = _srs_sources->fetch_or_create(&rtc_session->request, handler, &source)) != srs_success) {
        return srs_error_wrap(err, "rtc fetch source failed");
    }

    srs_trace("source url=%s, source_id=[%d][%d], encrypt=%d",
        rtc_session->request.get_stream_url().c_str(), ::getpid(), source->source_id(), rtc_session->encrypt);

    SrsConsumer* consumer = NULL;
    SrsAutoFree(SrsConsumer, consumer);
    if ((err = source->create_consumer(NULL, consumer)) != srs_success) {
        return srs_error_wrap(err, "rtc create consumer, source url=%s", rtc_session->request.get_stream_url().c_str());
    }

    // TODO: FIXME: Support reload.
    SrsRequest* req = &rtc_session->request;
    bool realtime = _srs_config->get_realtime_enabled(req->vhost, true);
    srs_utime_t mw_sleep = _srs_config->get_mw_sleep(req->vhost, true);

    SrsMessageArray msgs(SRS_PERF_MW_MSGS);

    SrsPithyPrint* pprint = SrsPithyPrint::create_rtc_play();
    SrsAutoFree(SrsPithyPrint, pprint);

    while (true) {
        if ((err = trd->pull()) != srs_success) {
            return srs_error_wrap(err, "rtc sender thread");
        }

#ifdef SRS_PERF_QUEUE_COND_WAIT
        if (realtime) {
            // for realtime, min required msgs is 0, send when got one+ msgs.
            consumer->wait(0, mw_sleep);
        } else {
            // for no-realtime, got some msgs then send.
            consumer->wait(SRS_PERF_MW_MIN_MSGS_FOR_RTC, mw_sleep);
        }
#endif

        int msg_count = 0;
        if ((err = consumer->dump_packets(&msgs, msg_count)) != srs_success) {
            continue;
        }

        if (msg_count <= 0) { 
#ifndef SRS_PERF_QUEUE_COND_WAIT
            srs_usleep(mw_sleep);
#endif
            // ignore when nothing got.
            continue;
        }

        int nn = 0;
        int nn_rtp_pkts = 0;
        send_and_free_messages(msgs.msgs, msg_count, sendonly_ukt, &nn, &nn_rtp_pkts);

        pprint->elapse();
        if (pprint->can_print()) {
            // TODO: FIXME: Print stat like frame/s, packet/s, loss_packets.
            srs_trace("-> RTC PLAY %d msgs, %d packets, %d bytes", msg_count, nn_rtp_pkts, nn);
        }
    }
}

void SrsRtcSenderThread::update_sendonly_socket(SrsUdpMuxSocket* skt)
{
    srs_trace("session %s address changed, update %s -> %s", 
        rtc_session->id().c_str(), sendonly_ukt->get_peer_id().c_str(), skt->get_peer_id().c_str());

    srs_freep(sendonly_ukt);
    sendonly_ukt = skt->copy_sendonly();
}

void SrsRtcSenderThread::send_and_free_messages(SrsSharedPtrMessage** msgs, int nb_msgs, SrsUdpMuxSocket* skt, int* pnn, int* pnn_rtp_pkts)
{
    if (!rtc_session->dtls_session) {
        return;
    }

    for (int i = 0; i < nb_msgs; i++) {
        SrsSharedPtrMessage* msg = msgs[i];
        bool is_video = msg->is_video();
        bool is_audio = msg->is_audio();

        int nn_rtp_pkts = (int)msg->rtp_packets.size();
        for (int j = 0; j < nn_rtp_pkts; j++) {
            SrsRtpSharedPacket* pkt = msg->rtp_packets[j];
            send_and_free_message(msg, is_video, is_audio, pkt, skt);
        }

        *pnn += msg->size;
        *pnn_rtp_pkts += nn_rtp_pkts;

        srs_freep(msg);
    }
}

void SrsRtcSenderThread::send_and_free_message(SrsSharedPtrMessage* msg, bool is_video, bool is_audio, SrsRtpSharedPacket* pkt, SrsUdpMuxSocket* skt)
{
    srs_error_t err = srs_success;

    if (is_video) {
        pkt->modify_rtp_header_payload_type(video_payload_type);
        pkt->modify_rtp_header_ssrc(video_ssrc);
        srs_verbose("send video, ssrc=%u, seq=%u, timestamp=%u", video_ssrc, pkt->rtp_header.get_sequence(), pkt->rtp_header.get_timestamp());
    } else if (is_audio) {
        pkt->modify_rtp_header_payload_type(audio_payload_type);
        pkt->modify_rtp_header_ssrc(audio_ssrc);
    }

    int length = pkt->size;
    // Fetch a cached message from queue.
    // TODO: FIXME: Maybe encrypt in async, so the state of mhdr maybe not ready.
    mmsghdr* mhdr = rtc_session->rtc_server->fetch();
    char* buf = (char*)mhdr->msg_hdr.msg_iov->iov_base;

    if (rtc_session->encrypt) {
        if ((err = rtc_session->dtls_session->protect_rtp(buf, pkt->payload, length)) != srs_success) {
            srs_warn("srtp err %s", srs_error_desc(err).c_str()); srs_freep(err); srs_freepa(buf);
            return;
        }
    } else {
        memcpy(buf, pkt->payload, length);
    }

    sockaddr_in* addr = (sockaddr_in*)skt->peer_addr();
    socklen_t addrlen = (socklen_t)skt->peer_addrlen();

    mhdr->msg_hdr.msg_name = (sockaddr_in*)addr;
    mhdr->msg_hdr.msg_namelen = (socklen_t)addrlen;
    mhdr->msg_hdr.msg_iov->iov_len = length;
    mhdr->msg_len = 0;

    rtc_session->rtc_server->sendmmsg(skt->stfd(), mhdr);
}

SrsRtcSession::SrsRtcSession(SrsRtcServer* rtc_svr, const SrsRequest& req, const std::string& un, int context_id)
{
    rtc_server = rtc_svr;
    session_state = INIT;
    dtls_session = new SrsDtlsSession(this);
    dtls_session->initialize(req);
    strd = NULL;

    username = un;
    
    last_stun_time = srs_get_system_time();

    request = req;
    source = NULL;

    cid = context_id;
    encrypt = true;

    // TODO: FIXME: Support reload.
    sessionStunTimeout = _srs_config->get_rtc_stun_timeout(req.vhost);
}

SrsRtcSession::~SrsRtcSession()
{
    srs_freep(dtls_session);

    if (strd) {
        strd->stop();
    }
    srs_freep(strd);
}

void SrsRtcSession::set_local_sdp(const SrsSdp& sdp)
{
    local_sdp = sdp;
}

void SrsRtcSession::switch_to_context()
{
    _srs_context->set_id(cid);
}

srs_error_t SrsRtcSession::on_stun(SrsUdpMuxSocket* skt, SrsStunPacket* stun_req)
{
    srs_error_t err = srs_success;

    if (stun_req->is_binding_request()) {
        if ((err = on_binding_request(skt, stun_req)) != srs_success) {
            return srs_error_wrap(err, "stun binding request failed");
        }

        last_stun_time = srs_get_system_time();

        if (strd && strd->sendonly_ukt) {
            // We are running in the ice-lite(server) mode. If client have multi network interface,
            // we only choose one candidate pair which is determined by client.
            if (stun_req->get_use_candidate() && strd->sendonly_ukt->get_peer_id() != skt->get_peer_id()) {
                strd->update_sendonly_socket(skt);
            }
        }
    }

    return err;
}

srs_error_t SrsRtcSession::check_source()
{
    srs_error_t err = srs_success;

    if (source == NULL) {
        // TODO: FIXME: Should refactor it, directly use http server as handler.
        ISrsSourceHandler* handler = _srs_hybrid->srs()->instance();
        if ((err = _srs_sources->fetch_or_create(&request, handler, &source)) != srs_success) {
            return srs_error_wrap(err, "create source");
        }
    }

    return err;
}

#ifdef SRS_AUTO_OSX
// These functions are similar to the older byteorder(3) family of functions.
// For example, be32toh() is identical to ntohl().
// @see https://linux.die.net/man/3/be32toh
#define be32toh ntohl
#endif

srs_error_t SrsRtcSession::on_binding_request(SrsUdpMuxSocket* skt, SrsStunPacket* stun_req)
{
    srs_error_t err = srs_success;

    bool strict_check = _srs_config->get_rtc_stun_strict_check(request.vhost);
    if (strict_check && stun_req->get_ice_controlled()) {
        // @see: https://tools.ietf.org/html/draft-ietf-ice-rfc5245bis-00#section-6.1.3.1
        // TODO: Send 487 (Role Conflict) error response.
        return srs_error_new(ERROR_RTC_STUN, "Peer must not in ice-controlled role in ice-lite mode.");
    }

    SrsStunPacket stun_binding_response;
    char buf[kRtpPacketSize];
    SrsBuffer* stream = new SrsBuffer(buf, sizeof(buf));
    SrsAutoFree(SrsBuffer, stream);

    stun_binding_response.set_message_type(BindingResponse);
    stun_binding_response.set_local_ufrag(stun_req->get_remote_ufrag());
    stun_binding_response.set_remote_ufrag(stun_req->get_local_ufrag());
    stun_binding_response.set_transcation_id(stun_req->get_transcation_id());
    // FIXME: inet_addr is deprecated, IPV6 support
    stun_binding_response.set_mapped_address(be32toh(inet_addr(skt->get_peer_ip().c_str())));
    stun_binding_response.set_mapped_port(skt->get_peer_port());

    if ((err = stun_binding_response.encode(get_local_sdp()->get_ice_pwd(), stream)) != srs_success) {
        return srs_error_wrap(err, "stun binding response encode failed");
    }

    if ((err = skt->sendto(stream->data(), stream->pos(), 0)) != srs_success) {
        return srs_error_wrap(err, "stun binding response send failed");
    }

    if (get_session_state() == WAITING_STUN) {
        set_session_state(DOING_DTLS_HANDSHAKE);

        peer_id = skt->get_peer_id();
        rtc_server->insert_into_id_sessions(peer_id, this);
    }

    return err;
}

srs_error_t SrsRtcSession::on_rtcp_feedback(char* buf, int nb_buf, SrsUdpMuxSocket* skt)
{
    srs_error_t err = srs_success;

    if (nb_buf < 12) {
        return srs_error_new(ERROR_RTC_RTCP_CHECK, "invalid rtp feedback packet, nb_buf=%d", nb_buf);
    }

    SrsBuffer* stream = new SrsBuffer(buf, nb_buf);
    SrsAutoFree(SrsBuffer, stream);

    // @see: https://tools.ietf.org/html/rfc4585#section-6.1
    /*
        0                   1                   2                   3
        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |V=2|P|   FMT   |       PT      |          length               |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                  SSRC of packet sender                        |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                  SSRC of media source                         |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       :            Feedback Control Information (FCI)                 :
       :                                                               :
    */
    /*uint8_t first = */stream->read_1bytes();
    //uint8_t version = first & 0xC0;
    //uint8_t padding = first & 0x20;
    //uint8_t fmt = first & 0x1F;

    /*uint8_t payload_type = */stream->read_1bytes();
    /*uint16_t length = */stream->read_2bytes();
    /*uint32_t ssrc_of_sender = */stream->read_4bytes();
    /*uint32_t ssrc_of_media_source = */stream->read_4bytes();

    /*
         0                   1                   2                   3
         0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |            PID                |             BLP               |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    */

    uint16_t pid = stream->read_2bytes();
    int blp = stream->read_2bytes();

    srs_verbose("pid=%u, blp=%d", pid, blp);

    if ((err = check_source()) != srs_success) {
        return srs_error_wrap(err, "check");
    }
    if (! source) {
        return srs_error_new(ERROR_RTC_SOURCE_CHECK, "can not found source");
    }

    vector<SrsRtpSharedPacket*> resend_pkts;
    SrsRtpSharedPacket* pkt = source->find_rtp_packet(pid);
    if (pkt) {
        resend_pkts.push_back(pkt);
    }

    uint16_t mask = 0x01;
    for (int i = 1; i < 16 && blp; ++i, mask <<= 1) {
        if (! (blp & mask)) {
            continue;
        }

        uint32_t loss_seq = pid + i;

        SrsRtpSharedPacket* pkt = source->find_rtp_packet(loss_seq);
        if (! pkt) {
            continue;
        }

        resend_pkts.push_back(pkt);
    }

    for (int i = 0; i < (int)resend_pkts.size(); ++i) {
        if (dtls_session) {
            char protected_buf[kRtpPacketSize];
            int nb_protected_buf = resend_pkts[i]->size;

            srs_verbose("resend pkt sequence=%u", resend_pkts[i]->rtp_header.get_sequence());

            dtls_session->protect_rtp(protected_buf, resend_pkts[i]->payload, nb_protected_buf);
            skt->sendto(protected_buf, nb_protected_buf, 0);
        }
    }

    return err;
}

srs_error_t SrsRtcSession::on_rtcp_ps_feedback(char* buf, int nb_buf, SrsUdpMuxSocket* skt)
{
    srs_error_t err = srs_success;

    if (nb_buf < 12) {
        return srs_error_new(ERROR_RTC_RTCP_CHECK, "invalid rtp feedback packet, nb_buf=%d", nb_buf);
    }

    SrsBuffer* stream = new SrsBuffer(buf, nb_buf);
    SrsAutoFree(SrsBuffer, stream);

    uint8_t first = stream->read_1bytes();
    //uint8_t version = first & 0xC0;
    //uint8_t padding = first & 0x20;
    uint8_t fmt = first & 0x1F;

    // TODO: FIXME: Dead code?
    /*uint8_t payload_type = */stream->read_1bytes();
    /*uint16_t length = */stream->read_2bytes();
    /*uint32_t ssrc_of_sender = */stream->read_4bytes();
    /*uint32_t ssrc_of_media_source = */stream->read_4bytes();

    switch (fmt) {
        case kPLI: {
            srs_verbose("pli");
            break;
        }
        case kSLI: {
            srs_verbose("sli");
            break;
        }
        case kRPSI: {
            srs_verbose("rpsi");
            break;
        }
        case kAFB: {
            srs_verbose("afb");
            break;
        }
        default: {
            return srs_error_new(ERROR_RTC_RTCP, "unknown payload specific feedback=%u", fmt);
        }
    }

    return err;
}

srs_error_t SrsRtcSession::on_rtcp_receiver_report(char* buf, int nb_buf, SrsUdpMuxSocket* skt)
{
    srs_error_t err = srs_success;

    if (nb_buf < 8) {
        return srs_error_new(ERROR_RTC_RTCP_CHECK, "invalid rtp receiver report packet, nb_buf=%d", nb_buf);
    }

    SrsBuffer* stream = new SrsBuffer(buf, nb_buf);
    SrsAutoFree(SrsBuffer, stream);

    // @see: https://tools.ietf.org/html/rfc3550#section-6.4.2
    /*
        0                   1                   2                   3
        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
header |V=2|P|    RC   |   PT=RR=201   |             length            |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                     SSRC of packet sender                     |
       +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
report |                 SSRC_1 (SSRC of first source)                 |
block  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  1    | fraction lost |       cumulative number of packets lost       |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |           extended highest sequence number received           |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                      interarrival jitter                      |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                         last SR (LSR)                         |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                   delay since last SR (DLSR)                  |
       +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
report |                 SSRC_2 (SSRC of second source)                |
block  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  2    :                               ...                             :
       +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
       |                  profile-specific extensions                  |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    */
    uint8_t first = stream->read_1bytes();
    //uint8_t version = first & 0xC0;
    //uint8_t padding = first & 0x20;
    uint8_t rc = first & 0x1F;

    /*uint8_t payload_type = */stream->read_1bytes();
    uint16_t length = stream->read_2bytes();
    /*uint32_t ssrc_of_sender = */stream->read_4bytes();

    if (((length + 1) * 4) != (rc * 24 + 8)) {
        return srs_error_new(ERROR_RTC_RTCP_CHECK, "invalid rtcp receiver packet, length=%u, rc=%u", length, rc);
    }

    for (int i = 0; i < rc; ++i) {
        uint32_t ssrc = stream->read_4bytes();
        uint8_t fraction_lost = stream->read_1bytes();
        uint32_t cumulative_number_of_packets_lost = stream->read_3bytes();
        uint32_t highest_seq = stream->read_4bytes();
        uint32_t jitter = stream->read_4bytes();
        uint32_t lst = stream->read_4bytes();
        uint32_t dlsr = stream->read_4bytes();

        (void)ssrc; (void)fraction_lost; (void)cumulative_number_of_packets_lost; (void)highest_seq; (void)jitter; (void)lst; (void)dlsr;
        srs_verbose("ssrc=%u, fraction_lost=%u, cumulative_number_of_packets_lost=%u, highest_seq=%u, jitter=%u, lst=%u, dlst=%u",
            ssrc, fraction_lost, cumulative_number_of_packets_lost, highest_seq, jitter, lst, dlsr);
    }

    return err;
}

srs_error_t SrsRtcSession::on_connection_established(SrsUdpMuxSocket* skt)
{
    srs_trace("rtc session=%s, to=%dms connection established", id().c_str(), srsu2msi(sessionStunTimeout));
    return start_play(skt);
}

srs_error_t SrsRtcSession::start_play(SrsUdpMuxSocket* skt)
{
    srs_error_t err = srs_success;

    srs_freep(strd);
    strd = new SrsRtcSenderThread(this, skt, _srs_context->get_id());

    uint32_t video_ssrc = 0;
    uint32_t audio_ssrc = 0;
    uint16_t video_payload_type = 0;
    uint16_t audio_payload_type = 0;
    for (size_t i = 0; i < local_sdp.media_descs_.size(); ++i) {
        const SrsMediaDesc& media_desc = local_sdp.media_descs_[i];
        if (media_desc.is_audio()) {
            audio_ssrc = media_desc.ssrc_infos_[0].ssrc_;
            audio_payload_type = media_desc.payload_types_[0].payload_type_;
        } else if (media_desc.is_video()) {
            video_ssrc = media_desc.ssrc_infos_[0].ssrc_;
            video_payload_type = media_desc.payload_types_[0].payload_type_;
        }
    }

    if ((err =strd->initialize(video_ssrc, audio_ssrc, video_payload_type, audio_payload_type)) != srs_success) {
        return srs_error_wrap(err, "SrsRtcSenderThread init");
    }

    if ((err = strd->start()) != srs_success) {
        return srs_error_wrap(err, "start SrsRtcSenderThread");
    }

    return err;
}

bool SrsRtcSession::is_stun_timeout()
{
    return last_stun_time + sessionStunTimeout < srs_get_system_time();
}

srs_error_t SrsRtcSession::on_dtls(SrsUdpMuxSocket* skt)
{
    return dtls_session->on_dtls(skt);
}

srs_error_t SrsRtcSession::on_rtcp(SrsUdpMuxSocket* skt)
{
    srs_error_t err = srs_success;

    if (dtls_session == NULL) {
        return srs_error_new(ERROR_RTC_RTCP, "recv unexpect rtp packet before dtls done");
    }

    char unprotected_buf[kRtpPacketSize];
    int nb_unprotected_buf = skt->size();
    if ((err = dtls_session->unprotect_rtcp(unprotected_buf, skt->data(), nb_unprotected_buf)) != srs_success) {
        return srs_error_wrap(err, "rtcp unprotect failed");
    }

    char* ph = unprotected_buf;
    int nb_left = nb_unprotected_buf;
    while (nb_left) {
        uint8_t payload_type = ph[1];
        uint16_t length_4bytes = (((uint16_t)ph[2]) << 8) | ph[3];

        int length = (length_4bytes + 1) * 4;

        if (length > nb_unprotected_buf) {
            return srs_error_new(ERROR_RTC_RTCP, "invalid rtcp packet, length=%u", length);
        }

        srs_verbose("on rtcp, payload_type=%u", payload_type);

        switch (payload_type) {
            case kSR: {
                break;
            }
            case kRR: {
                err = on_rtcp_receiver_report(ph, length, skt);
                break;
            }
            case kSDES: {
                break;
            }
            case kBye: {
                break;
            }
            case kApp: {
                break;
            }
            case kRtpFb: {
                err = on_rtcp_feedback(ph, length, skt);
                break;
            }
            case kPsFb: {
                err = on_rtcp_ps_feedback(ph, length, skt);
                break;
            }
            default:{
                return srs_error_new(ERROR_RTC_RTCP_CHECK, "unknown rtcp type=%u", payload_type);
                break;
            }
        }

        if (err != srs_success) {
            return srs_error_wrap(err, "rtcp");
        }

        ph += length;
        nb_left -= length;
    }

    return err;
}

SrsRtcServer::SrsRtcServer()
{
    listener = NULL;
    timer = new SrsHourGlass(this, 1 * SRS_UTIME_SECONDS);

    mmstfd = NULL;
    waiting_msgs = false;
    cond = srs_cond_new();
    trd = new SrsDummyCoroutine();

    cache_pos = 0;

    _srs_config->subscribe(this);
}

SrsRtcServer::~SrsRtcServer()
{
    _srs_config->unsubscribe(this);

    srs_freep(listener);
    srs_freep(timer);

    srs_freep(trd);
    srs_cond_destroy(cond);

    free_mhdrs(hotspot);
    hotspot.clear();

    free_mhdrs(cache);
    cache.clear();
}

srs_error_t SrsRtcServer::initialize()
{
    srs_error_t err = srs_success;

    if ((err = timer->tick(1 * SRS_UTIME_SECONDS)) != srs_success) {
        return srs_error_wrap(err, "hourglass tick");
    }

    if ((err = timer->start()) != srs_success) {
        return srs_error_wrap(err, "start timer");
    }

    srs_freep(trd);
    trd = new SrsSTCoroutine("udp", this);
    if ((err = trd->start()) != srs_success) {
        return srs_error_wrap(err, "start coroutine");
    }

    max_sendmmsg = _srs_config->get_rtc_server_sendmmsg();
    srs_trace("RTC server init ok, max_sendmmsg=%d", max_sendmmsg);

    return err;
}

srs_error_t SrsRtcServer::listen_udp()
{
    srs_error_t err = srs_success;

    if (!_srs_config->get_rtc_server_enabled()) {
        return err;
    }

    int port = _srs_config->get_rtc_server_listen();
    if (port <= 0) {
        return srs_error_new(ERROR_RTC_PORT, "invalid port=%d", port);
    }

    string ip = srs_any_address_for_listener();

    srs_freep(listener);
    listener = new SrsUdpMuxListener(this, ip, port);

    if ((err = listener->listen()) != srs_success) {
        return srs_error_wrap(err, "listen %s:%d", ip.c_str(), port);
    }

    srs_trace("rtc listen at udp://%s:%d, fd=%d", ip.c_str(), port, listener->fd());

    return err;
}

srs_error_t SrsRtcServer::on_udp_packet(SrsUdpMuxSocket* skt)
{
    if (is_stun(reinterpret_cast<const uint8_t*>(skt->data()), skt->size())) {
        return on_stun(skt);
    } else if (is_dtls(reinterpret_cast<const uint8_t*>(skt->data()), skt->size())) {
        return on_dtls(skt);
    } else if (is_rtp_or_rtcp(reinterpret_cast<const uint8_t*>(skt->data()), skt->size())) {
        return on_rtp_or_rtcp(skt);
    } 

    return srs_error_new(ERROR_RTC_UDP, "unknown udp packet type");
}

srs_error_t SrsRtcServer::listen_api()
{
    srs_error_t err = srs_success;

    // TODO: FIXME: Fetch api from hybrid manager.
    SrsHttpServeMux* http_api_mux = _srs_hybrid->srs()->instance()->api_server();
    if ((err = http_api_mux->handle("/rtc/v1/play/", new SrsGoApiRtcPlay(this))) != srs_success) {
        return srs_error_wrap(err, "handle sdp");
    }

    return err;
}

SrsRtcSession* SrsRtcServer::create_rtc_session(const SrsRequest& req, const SrsSdp& remote_sdp, SrsSdp& local_sdp, const string& mock_eip)
{
    std::string local_pwd = gen_random_str(32);
    std::string local_ufrag = "";
    std::string username = "";
    while (true) {
        local_ufrag = gen_random_str(8);

        username = local_ufrag + ":" + remote_sdp.get_ice_ufrag();
        if (! map_username_session.count(username))
            break;
    }

    int cid = _srs_context->get_id();
    SrsRtcSession* session = new SrsRtcSession(this, req, username, cid);
    map_username_session.insert(make_pair(username, session));

    local_sdp.set_ice_ufrag(local_ufrag);
    local_sdp.set_ice_pwd(local_pwd);
    local_sdp.set_fingerprint_algo("sha-256");
    local_sdp.set_fingerprint(SrsDtls::instance()->get_fingerprint());

    // We allows to mock the eip of server.
    if (!mock_eip.empty()) {
        local_sdp.add_candidate(mock_eip, _srs_config->get_rtc_server_listen(), "host");
    } else {
        std::vector<string> candidate_ips = get_candidate_ips();
        for (int i = 0; i < (int)candidate_ips.size(); ++i) {
            local_sdp.add_candidate(candidate_ips[i], _srs_config->get_rtc_server_listen(), "host");
        }
    }

    session->set_remote_sdp(remote_sdp);
    session->set_local_sdp(local_sdp);

    session->set_session_state(WAITING_STUN);

    return session;
}

SrsRtcSession* SrsRtcServer::find_rtc_session_by_peer_id(const string& peer_id)
{
    map<string, SrsRtcSession*>::iterator iter = map_id_session.find(peer_id);
    if (iter == map_id_session.end()) {
        return NULL; 
    }

    return iter->second;
}

srs_error_t SrsRtcServer::on_stun(SrsUdpMuxSocket* skt)
{
    srs_error_t err = srs_success;

    SrsStunPacket stun_req;
    if ((err = stun_req.decode(skt->data(), skt->size())) != srs_success) {
        return srs_error_wrap(err, "decode stun packet failed");
    }

    srs_verbose("recv stun packet from %s, use-candidate=%d, ice-controlled=%d, ice-controlling=%d", 
        skt->get_peer_id().c_str(), stun_req.get_use_candidate(), stun_req.get_ice_controlled(), stun_req.get_ice_controlling());

    std::string username = stun_req.get_username();
    SrsRtcSession* rtc_session = find_rtc_session_by_username(username);
    if (rtc_session == NULL) {
        return srs_error_new(ERROR_RTC_STUN, "can not find rtc_session, stun username=%s", username.c_str());
    }

    // Now, we got the RTC session to handle the packet, switch to its context
    // to make all logs write to the "correct" pid+cid.
    rtc_session->switch_to_context();

    return rtc_session->on_stun(skt, &stun_req);
}

srs_error_t SrsRtcServer::on_dtls(SrsUdpMuxSocket* skt)
{
    SrsRtcSession* rtc_session = find_rtc_session_by_peer_id(skt->get_peer_id());

    if (rtc_session == NULL) {
        return srs_error_new(ERROR_RTC_DTLS, "can not find rtc session by peer_id=%s", skt->get_peer_id().c_str());
    }

    // Now, we got the RTC session to handle the packet, switch to its context
    // to make all logs write to the "correct" pid+cid.
    rtc_session->switch_to_context();

    return rtc_session->on_dtls(skt);
}

srs_error_t SrsRtcServer::on_rtp_or_rtcp(SrsUdpMuxSocket* skt)
{
    srs_error_t err = srs_success;

    SrsRtcSession* rtc_session = find_rtc_session_by_peer_id(skt->get_peer_id());

    if (rtc_session == NULL) {
        return srs_error_new(ERROR_RTC_RTP, "can not find rtc session by peer_id=%s", skt->get_peer_id().c_str());
    }

    // Now, we got the RTC session to handle the packet, switch to its context
    // to make all logs write to the "correct" pid+cid.
    rtc_session->switch_to_context();

    if (is_rtcp(reinterpret_cast<const uint8_t*>(skt->data()), skt->size())) {
        err = rtc_session->on_rtcp(skt);
    } else {
        // We disable it because no RTP for player.
        // see https://github.com/ossrs/srs/blob/018577e685a07d9de7a47354e7a9c5f77f5f4202/trunk/src/app/srs_app_rtc_conn.cpp#L1081
        // err = rtc_session->on_rtp(skt);
    }

    return err;
}

SrsRtcSession* SrsRtcServer::find_rtc_session_by_username(const std::string& username)
{
    map<string, SrsRtcSession*>::iterator iter = map_username_session.find(username);
    if (iter == map_username_session.end()) {
        return NULL; 
    }

    return iter->second;
}

bool SrsRtcServer::insert_into_id_sessions(const string& peer_id, SrsRtcSession* rtc_session)
{
    return map_id_session.insert(make_pair(peer_id, rtc_session)).second;
}

void SrsRtcServer::check_and_clean_timeout_session()
{
    map<string, SrsRtcSession*>::iterator iter = map_username_session.begin();
    while (iter != map_username_session.end()) {
        SrsRtcSession* session = iter->second;
        if (session == NULL) {
            map_username_session.erase(iter++);
            continue;
        }

        if (session->is_stun_timeout()) {
            // Now, we got the RTC session to cleanup, switch to its context
            // to make all logs write to the "correct" pid+cid.
            session->switch_to_context();

            srs_trace("rtc session=%s, stun timeout", session->id().c_str());
            map_username_session.erase(iter++);
            map_id_session.erase(session->get_peer_id());
            delete session;
            continue;
        }

        ++iter;
    }
}

srs_error_t SrsRtcServer::notify(int type, srs_utime_t interval, srs_utime_t tick)
{
    check_and_clean_timeout_session();
    return srs_success;
}

srs_error_t SrsRtcServer::on_reload_rtc_server()
{
    int v = _srs_config->get_rtc_server_sendmmsg();
    if (max_sendmmsg != v) {
        max_sendmmsg = v;
        srs_trace("Reload max_sendmmsg=%d", max_sendmmsg);
    }

    return srs_success;
}

mmsghdr* SrsRtcServer::fetch()
{
    // TODO: FIXME: Maybe need to shrink?
    if (cache_pos >= (int)cache.size()) {
        mmsghdr mhdr;
        memset(&mhdr, 0, sizeof(mmsghdr));

        mhdr.msg_hdr.msg_iovlen = 1;
        mhdr.msg_hdr.msg_iov = new iovec();
        mhdr.msg_hdr.msg_iov->iov_base = new char[kRtpPacketSize];
        mhdr.msg_hdr.msg_iov->iov_len = kRtpPacketSize;
        mhdr.msg_len = 0;

        cache.push_back(mhdr);
    }

    return &cache[cache_pos++];
}

void SrsRtcServer::sendmmsg(srs_netfd_t stfd, mmsghdr* /*hdr*/)
{
    mmstfd = stfd;

    if (waiting_msgs) {
        waiting_msgs = false;
        srs_cond_signal(cond);
    }
}

void SrsRtcServer::free_mhdrs(std::vector<mmsghdr>& mhdrs)
{
    for (int i = 0; i < (int)mhdrs.size(); i++) {
        mmsghdr* hdr = &mhdrs[i];

        for (int j = (int)hdr->msg_hdr.msg_iovlen - 1; j >= 0 ; j--) {
            iovec* iov = hdr->msg_hdr.msg_iov + j;
            char* data = (char*)iov->iov_base;
            srs_freep(data);
            srs_freep(iov);
        }
    }
}

srs_error_t SrsRtcServer::cycle()
{
    srs_error_t err = srs_success;

    uint64_t nn_msgs = 0;
    SrsStatistic* stat = SrsStatistic::instance();

    SrsPithyPrint* pprint = SrsPithyPrint::create_rtc_send();
    SrsAutoFree(SrsPithyPrint, pprint);

    while (true) {
        if ((err = trd->pull()) != srs_success) {
            return err;
        }

        int pos = cache_pos;
        if (pos <= 0) {
            waiting_msgs = true;
            srs_cond_wait(cond);
            continue;
        }

        // We are working on hotspot now.
        cache.swap(hotspot);
        cache_pos = 0;

        mmsghdr* p = &hotspot[0];
        for (mmsghdr* end = p + pos; p < end; p += max_sendmmsg) {
            int vlen = (int)(end - p);
            vlen = srs_min(max_sendmmsg, vlen);

            int r0 = srs_sendmmsg(mmstfd, p, (unsigned int)vlen, 0, SRS_UTIME_NO_TIMEOUT);
            if (r0 != vlen) {
                srs_warn("sendmsg %d msgs, %d done", vlen, r0);
            }

            stat->perf_mw_on_packets(vlen);
        }

        // Increase total messages.
        nn_msgs += pos;

        pprint->elapse();
        if (pprint->can_print()) {
            srs_trace("-> RTC SEND %d by sendmmsg %d, total %" PRId64 " msgs", pos, max_sendmmsg, nn_msgs);
        }
    }

    return err;
}

RtcServerAdapter::RtcServerAdapter()
{
    rtc = new SrsRtcServer();
}

RtcServerAdapter::~RtcServerAdapter()
{
    srs_freep(rtc);
}

srs_error_t RtcServerAdapter::initialize()
{
    srs_error_t err = srs_success;

    if ((err = rtc->initialize()) != srs_success) {
        return srs_error_wrap(err, "rtc server initialize");
    }

    return err;
}

srs_error_t RtcServerAdapter::run()
{
    srs_error_t err = srs_success;

    if ((err = rtc->listen_udp()) != srs_success) {
        return srs_error_wrap(err, "listen udp");
    }

    if ((err = rtc->listen_api()) != srs_success) {
        return srs_error_wrap(err, "listen api");
    }

    return err;
}

void RtcServerAdapter::stop()
{
}

