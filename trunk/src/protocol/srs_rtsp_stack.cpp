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

#include <srs_rtsp_stack.hpp>

#include <stdlib.h>
#include <map>
using namespace std;

#include <srs_rtmp_io.hpp>
#include <srs_kernel_buffer.hpp>
#include <srs_kernel_error.hpp>
#include <srs_kernel_log.hpp>
#include <srs_kernel_consts.hpp>
#include <srs_core_autofree.hpp>
#include <srs_kernel_utility.hpp>
#include <srs_kernel_stream.hpp>

#ifdef SRS_AUTO_STREAM_CASTER

#define __SRS_RTSP_BUFFER 4096

// get the status text of code.
string srs_generate_rtsp_status_text(int status)
{
    static std::map<int, std::string> _status_map;
    if (_status_map.empty()) {
        _status_map[SRS_CONSTS_RTSP_Continue                       ] = SRS_CONSTS_RTSP_Continue_str                        ;      
        _status_map[SRS_CONSTS_RTSP_OK                             ] = SRS_CONSTS_RTSP_OK_str                              ;      
        _status_map[SRS_CONSTS_RTSP_Created                        ] = SRS_CONSTS_RTSP_Created_str                         ;      
        _status_map[SRS_CONSTS_RTSP_LowOnStorageSpace              ] = SRS_CONSTS_RTSP_LowOnStorageSpace_str               ;      
        _status_map[SRS_CONSTS_RTSP_MultipleChoices                ] = SRS_CONSTS_RTSP_MultipleChoices_str                 ;      
        _status_map[SRS_CONSTS_RTSP_MovedPermanently               ] = SRS_CONSTS_RTSP_MovedPermanently_str                ;      
        _status_map[SRS_CONSTS_RTSP_MovedTemporarily               ] = SRS_CONSTS_RTSP_MovedTemporarily_str                ;      
        _status_map[SRS_CONSTS_RTSP_SeeOther                       ] = SRS_CONSTS_RTSP_SeeOther_str                        ;      
        _status_map[SRS_CONSTS_RTSP_NotModified                    ] = SRS_CONSTS_RTSP_NotModified_str                     ;      
        _status_map[SRS_CONSTS_RTSP_UseProxy                       ] = SRS_CONSTS_RTSP_UseProxy_str                        ;      
        _status_map[SRS_CONSTS_RTSP_BadRequest                     ] = SRS_CONSTS_RTSP_BadRequest_str                      ;      
        _status_map[SRS_CONSTS_RTSP_Unauthorized                   ] = SRS_CONSTS_RTSP_Unauthorized_str                    ;      
        _status_map[SRS_CONSTS_RTSP_PaymentRequired                ] = SRS_CONSTS_RTSP_PaymentRequired_str                 ;      
        _status_map[SRS_CONSTS_RTSP_Forbidden                      ] = SRS_CONSTS_RTSP_Forbidden_str                       ;      
        _status_map[SRS_CONSTS_RTSP_NotFound                       ] = SRS_CONSTS_RTSP_NotFound_str                        ;      
        _status_map[SRS_CONSTS_RTSP_MethodNotAllowed               ] = SRS_CONSTS_RTSP_MethodNotAllowed_str                ;      
        _status_map[SRS_CONSTS_RTSP_NotAcceptable                  ] = SRS_CONSTS_RTSP_NotAcceptable_str                   ;      
        _status_map[SRS_CONSTS_RTSP_ProxyAuthenticationRequired    ] = SRS_CONSTS_RTSP_ProxyAuthenticationRequired_str     ;      
        _status_map[SRS_CONSTS_RTSP_RequestTimeout                 ] = SRS_CONSTS_RTSP_RequestTimeout_str                  ;      
        _status_map[SRS_CONSTS_RTSP_Gone                           ] = SRS_CONSTS_RTSP_Gone_str                            ;      
        _status_map[SRS_CONSTS_RTSP_LengthRequired                 ] = SRS_CONSTS_RTSP_LengthRequired_str                  ;      
        _status_map[SRS_CONSTS_RTSP_PreconditionFailed             ] = SRS_CONSTS_RTSP_PreconditionFailed_str              ;      
        _status_map[SRS_CONSTS_RTSP_RequestEntityTooLarge          ] = SRS_CONSTS_RTSP_RequestEntityTooLarge_str           ;      
        _status_map[SRS_CONSTS_RTSP_RequestURITooLarge             ] = SRS_CONSTS_RTSP_RequestURITooLarge_str              ;      
        _status_map[SRS_CONSTS_RTSP_UnsupportedMediaType           ] = SRS_CONSTS_RTSP_UnsupportedMediaType_str            ;      
        _status_map[SRS_CONSTS_RTSP_ParameterNotUnderstood         ] = SRS_CONSTS_RTSP_ParameterNotUnderstood_str          ;      
        _status_map[SRS_CONSTS_RTSP_ConferenceNotFound             ] = SRS_CONSTS_RTSP_ConferenceNotFound_str              ;      
        _status_map[SRS_CONSTS_RTSP_NotEnoughBandwidth             ] = SRS_CONSTS_RTSP_NotEnoughBandwidth_str              ;      
        _status_map[SRS_CONSTS_RTSP_SessionNotFound                ] = SRS_CONSTS_RTSP_SessionNotFound_str                 ;      
        _status_map[SRS_CONSTS_RTSP_MethodNotValidInThisState      ] = SRS_CONSTS_RTSP_MethodNotValidInThisState_str       ;      
        _status_map[SRS_CONSTS_RTSP_HeaderFieldNotValidForResource ] = SRS_CONSTS_RTSP_HeaderFieldNotValidForResource_str  ;      
        _status_map[SRS_CONSTS_RTSP_InvalidRange                   ] = SRS_CONSTS_RTSP_InvalidRange_str                    ;      
        _status_map[SRS_CONSTS_RTSP_ParameterIsReadOnly            ] = SRS_CONSTS_RTSP_ParameterIsReadOnly_str             ;      
        _status_map[SRS_CONSTS_RTSP_AggregateOperationNotAllowed   ] = SRS_CONSTS_RTSP_AggregateOperationNotAllowed_str    ;      
        _status_map[SRS_CONSTS_RTSP_OnlyAggregateOperationAllowed  ] = SRS_CONSTS_RTSP_OnlyAggregateOperationAllowed_str   ;      
        _status_map[SRS_CONSTS_RTSP_UnsupportedTransport           ] = SRS_CONSTS_RTSP_UnsupportedTransport_str            ;      
        _status_map[SRS_CONSTS_RTSP_DestinationUnreachable         ] = SRS_CONSTS_RTSP_DestinationUnreachable_str          ;      
        _status_map[SRS_CONSTS_RTSP_InternalServerError            ] = SRS_CONSTS_RTSP_InternalServerError_str             ;      
        _status_map[SRS_CONSTS_RTSP_NotImplemented                 ] = SRS_CONSTS_RTSP_NotImplemented_str                  ;      
        _status_map[SRS_CONSTS_RTSP_BadGateway                     ] = SRS_CONSTS_RTSP_BadGateway_str                      ;     
        _status_map[SRS_CONSTS_RTSP_ServiceUnavailable             ] = SRS_CONSTS_RTSP_ServiceUnavailable_str              ;     
        _status_map[SRS_CONSTS_RTSP_GatewayTimeout                 ] = SRS_CONSTS_RTSP_GatewayTimeout_str                  ;     
        _status_map[SRS_CONSTS_RTSP_RTSPVersionNotSupported        ] = SRS_CONSTS_RTSP_RTSPVersionNotSupported_str         ;     
        _status_map[SRS_CONSTS_RTSP_OptionNotSupported             ] = SRS_CONSTS_RTSP_OptionNotSupported_str              ;        
    }
    
    std::string status_text;
    if (_status_map.find(status) == _status_map.end()) {
        status_text = "Status Unknown";
    } else {
        status_text = _status_map[status];
    }
    
    return status_text;
}

std::string srs_generate_rtsp_method_str(SrsRtspMethod method) 
{
    switch (method) {
        case SrsRtspMethodDescribe: return __SRS_METHOD_DESCRIBE;
        case SrsRtspMethodAnnounce: return __SRS_METHOD_ANNOUNCE;
        case SrsRtspMethodGetParameter: return __SRS_METHOD_GET_PARAMETER;
        case SrsRtspMethodOptions: return __SRS_METHOD_OPTIONS;
        case SrsRtspMethodPause: return __SRS_METHOD_PAUSE;
        case SrsRtspMethodPlay: return __SRS_METHOD_PLAY;
        case SrsRtspMethodRecord: return __SRS_METHOD_RECORD;
        case SrsRtspMethodRedirect: return __SRS_METHOD_REDIRECT;
        case SrsRtspMethodSetup: return __SRS_METHOD_SETUP;
        case SrsRtspMethodSetParameter: return __SRS_METHOD_SET_PARAMETER;
        case SrsRtspMethodTeardown: return __SRS_METHOD_TEARDOWN;
        default: return "Unknown";
    }
}

SrsRtpPacket::SrsRtpPacket()
{
    version = 2;
    padding = 0;
    extension = 0;
    csrc_count = 0;
    marker = 1;

    payload_type = 0;
    sequence_number = 0;
    timestamp = 0;
    ssrc = 0;

    payload = new SrsSimpleBuffer();
    chunked = false;
    completed = false;
}

SrsRtpPacket::~SrsRtpPacket()
{
    srs_freep(payload);
}

void SrsRtpPacket::copy(SrsRtpPacket* src)
{
    version = src->version;
    padding = src->padding;
    extension = src->extension;
    csrc_count = src->csrc_count;
    marker = src->marker;
    payload_type = src->payload_type;
    sequence_number = src->sequence_number;
    timestamp = src->timestamp;
    ssrc = src->ssrc;

    chunked = src->chunked;
    completed = src->completed;
}

void SrsRtpPacket::reap(SrsRtpPacket* src)
{
    copy(src);

    payload = src->payload;
    src->payload = NULL;
}

int SrsRtpPacket::decode(SrsStream* stream)
{
    int ret = ERROR_SUCCESS;

    // 12bytes header, atleast 2bytes content.
    if (!stream->require(14)) {
        ret = ERROR_RTP_HEADER_CORRUPT;
        srs_error("rtsp: rtp header corrupt. ret=%d", ret);
        return ret;
    }

    int8_t vv = stream->read_1bytes();
    version = (vv >> 6) & 0x03;
    padding = (vv >> 5) & 0x01;
    extension = (vv >> 4) & 0x01;
    csrc_count = vv & 0x0f;

    int8_t mv = stream->read_1bytes();
    marker = (mv >> 7) & 0x01;
    payload_type = mv & 0x7f;

    sequence_number = stream->read_2bytes();
    timestamp = stream->read_4bytes();
    ssrc = stream->read_4bytes();

    // frame type
    // 0... .... reserverd
    // .11. .... NALU[0]&0x60
    // ...1 11.. FU indicator
    // .... ..00 reserverd
    int8_t ftv = stream->read_1bytes();
    int8_t nalu_0x60 = ftv & 0x60;
    int8_t fu_indicator = ftv & 0x1c;

    // nri, whatever
    // 10.. .... first chunk.
    // 00.. .... continous chunk.
    // 01.. .... last chunk.
    // ...1 1111 NALU[0]&0x1f
    int8_t nriv = stream->read_1bytes();
    bool first_chunk = (nriv & 0xC0) == 0x80;
    bool last_chunk = (nriv & 0xC0) == 0x40;
    bool contious_chunk = (nriv & 0xC0) == 0x00;
    int8_t nalu_0x1f = nriv & 0x1f;

    // chunked, generate the first byte NALU.
    if (fu_indicator == 0x1c && (first_chunk || last_chunk || contious_chunk)) {
        chunked = true;
        completed = last_chunk;

        // generate and append the first byte NALU.
        if (first_chunk) {
            int8_t nalu_byte0 = nalu_0x60 | nalu_0x1f;
            payload->append((char*)&nalu_byte0, 1);
        }
        
        payload->append(stream->data() + stream->pos(), stream->size() - stream->pos());
        return ret;
    }

    // no chunked, append to payload.
    stream->skip(-2);
    payload->append(stream->data() + stream->pos(), stream->size() - stream->pos());
    completed = true;

    return ret;
}

SrsRtspSdp::SrsRtspSdp()
{
    state = SrsRtspSdpStateOthers;
}

SrsRtspSdp::~SrsRtspSdp()
{
}

int SrsRtspSdp::parse(string token)
{
    int ret = ERROR_SUCCESS;

    if (token.empty()) {
        srs_info("rtsp: ignore empty token.");
        return ret;
    }
    
    size_t pos = string::npos;

    char* start = (char*)token.data();
    char* end = start + (int)token.length();
    char* p = start;

    // key, first 2bytes.
    // v=0
    // o=- 0 0 IN IP4 127.0.0.1
    // s=No Name
    // c=IN IP4 192.168.43.23
    // t=0 0
    // a=tool:libavformat 53.9.0
    // m=video 0 RTP/AVP 96
    // b=AS:850
    // a=rtpmap:96 H264/90000
    // a=fmtp:96 packetization-mode=1; sprop-parameter-sets=Z2QAKKzRwFAFu/8ALQAiEAAAAwAQAAADAwjxgxHg,aOmrLIs=
    // a=control:streamid=0
    // m=audio 0 RTP/AVP 97
    // b=AS:49
    // a=rtpmap:97 MPEG4-GENERIC/44100/2
    // a=fmtp:97 profile-level-id=1;mode=AAC-hbr;sizelength=13;indexlength=3;indexdeltalength=3; config=139056E5A0
    // a=control:streamid=1
    char key = p[0];
    p += 2;

    // left bytes as attr string.
    std::string attr_str;
    if (end - p) {
        attr_str.append(p, end - p);
    }

    // parse the attributes from left bytes.
    std::vector<std::string> attrs;
    while (p < end) {
        // parse an attribute, split by SP.
        char* pa = p;
        for (; p < end && p[0] != __SRS_RTSP_SP; p++) {
        }
        std::string attr;
        if (p > pa) {
            attr.append(pa, p - pa);
            attrs.push_back(attr);
        }
        p++;
    }

    // parse the first attr as desc, update the first elem for desc.
    // for example, the value can be "tool", "AS", "rtpmap", "fmtp", "control"
    std::string desc_key;
    if (attrs.size() > 0) {
        std::string attr = attrs.at(0);
        if ((pos = attr.find(":")) != string::npos) {
            desc_key = attr.substr(0, pos);
            attr = attr.substr(pos + 1);
            attr_str = attr_str.substr(pos + 1);
            attrs[0] = attr;
        } else {
            desc_key = attr;
        }
    }

    // interpret the attribute according by key.
    switch (key) {
        case 'v': version = attr_str; break;
        case 'o':
            owner_username = (attrs.size() > 0)? attrs[0]:"";
            owner_session_id = (attrs.size() > 1)? attrs[1]:"";
            owner_session_version = (attrs.size() > 2)? attrs[2]:"";
            owner_network_type = (attrs.size() > 3)? attrs[3]:"";
            owner_address_type = (attrs.size() > 4)? attrs[4]:"";
            owner_address = (attrs.size() > 5)? attrs[5]:"";
            break;
        case 's': session_name = attr_str; break;
        case 'c':
            connection_network_type = (attrs.size() > 0)? attrs[0]:"";
            connection_address_type = (attrs.size() > 0)? attrs[0]:"";
            connection_address = (attrs.size() > 0)? attrs[0]:"";
            break;
        case 'a':
            if (desc_key == "tool") {
                tool = attr_str;
            } else if (desc_key == "rtpmap") {
                if (state == SrsRtspSdpStateVideo) {
                    video_codec = (attrs.size() > 1)? attrs[1]:"";
                    if ((pos = video_codec.find("/")) != string::npos) {
                        video_sample_rate = video_codec.substr(pos + 1);
                        video_codec = video_codec.substr(0, pos);
                    }
                } else if (state == SrsRtspSdpStateAudio) {
                    audio_codec = (attrs.size() > 1)? attrs[1]:"";
                    if ((pos = audio_codec.find("/")) != string::npos) {
                        audio_sample_rate = audio_codec.substr(pos + 1);
                        audio_codec = audio_codec.substr(0, pos);
                    }
                    if ((pos = audio_sample_rate.find("/")) != string::npos) {
                        audio_channel = audio_sample_rate.substr(pos + 1);
                        audio_sample_rate = audio_sample_rate.substr(0, pos);
                    }
                }
            } else if (desc_key == "fmtp") {
                for (int i = 1; i < (int)attrs.size(); i++) {
                    std::string attr = attrs.at(i);
                    if ((ret = parse_fmtp_attribute(attr)) != ERROR_SUCCESS) {
                        srs_error("rtsp: parse fmtp failed, attr=%s. ret=%d", attr.c_str(), ret);
                        return ret;
                    }
                }
            } else if (desc_key == "control") {
                for (int i = 0; i < (int)attrs.size(); i++) {
                    std::string attr = attrs.at(i);
                    if ((ret = parse_control_attribute(attr)) != ERROR_SUCCESS) {
                        srs_error("rtsp: parse control failed, attr=%s. ret=%d", attr.c_str(), ret);
                        return ret;
                    }
                }
            }
            break;
        case 'm':
            if (desc_key == "video") {
                state = SrsRtspSdpStateVideo;
                video_port = (attrs.size() > 1)? attrs[1]:"";
                video_protocol = (attrs.size() > 2)? attrs[2]:"";
                video_transport_format = (attrs.size() > 3)? attrs[3]:"";
            } else if (desc_key == "audio") {
                state = SrsRtspSdpStateAudio;
                audio_port = (attrs.size() > 1)? attrs[1]:"";
                audio_protocol = (attrs.size() > 2)? attrs[2]:"";
                audio_transport_format = (attrs.size() > 3)? attrs[3]:"";
            }
            break;
        case 'b':
            if (desc_key == "AS") {
                if (state == SrsRtspSdpStateVideo) {
                    video_bandwidth_kbps = (attrs.size() > 0)? attrs[0]:"";
                } else if (state == SrsRtspSdpStateAudio) {
                    audio_bandwidth_kbps = (attrs.size() > 0)? attrs[0]:"";
                }
            }
            break;
        case 't':
        default: break;
    }

    return ret;
}

int SrsRtspSdp::parse_fmtp_attribute(string attr)
{
    int ret = ERROR_SUCCESS;
    
    size_t pos = string::npos;
    std::string token = attr;

    while (!token.empty()) {
        std::string item = token;
        if ((pos = item.find(";")) != string::npos) {
            item = token.substr(0, pos);
            token = token.substr(pos + 1);
        } else {
            token = "";
        }

        std::string item_key = item, item_value;
        if ((pos = item.find("=")) != string::npos) {
            item_key = item.substr(0, pos);
            item_value = item.substr(pos + 1);
        }

        if (state == SrsRtspSdpStateVideo) {
            if (item_key == "packetization-mode") {
                video_packetization_mode = item_value;
            } else if (item_key == "sprop-parameter-sets") {
                video_sps = item_value;
                if ((pos = video_sps.find(",")) != string::npos) {
                    video_pps = video_sps.substr(pos + 1);
                    video_sps = video_sps.substr(0, pos);
                }
                // decode the sps/pps by base64
                video_sps = base64_decode(video_sps);
                video_pps = base64_decode(video_pps);
            }
        } else if (state == SrsRtspSdpStateAudio) {
            if (item_key == "profile-level-id") {
                audio_profile_level_id = item_value;
            } else if (item_key == "mode") {
                audio_mode = item_value;
            } else if (item_key == "sizelength") {
                audio_size_length = item_value;
            } else if (item_key == "indexlength") {
                audio_index_length = item_value;
            } else if (item_key == "indexdeltalength") {
                audio_index_delta_length = item_value;
            } else if (item_key == "config") {
                audio_sh = base64_decode(item_value);
            }
        }
    }

    return ret;
}

int SrsRtspSdp::parse_control_attribute(string attr)
{
    int ret = ERROR_SUCCESS;
    
    size_t pos = string::npos;
    std::string token = attr;

    while (!token.empty()) {
        std::string item = token;
        if ((pos = item.find(";")) != string::npos) {
            item = token.substr(0, pos);
            token = token.substr(pos + 1);
        } else {
            token = "";
        }

        std::string item_key = item, item_value;
        if ((pos = item.find("=")) != string::npos) {
            item_key = item.substr(0, pos);
            item_value = item.substr(pos + 1);
        }

        if (state == SrsRtspSdpStateVideo) {
            if (item_key == "streamid") {
                video_stream_id = item_value;
            }
        } else if (state == SrsRtspSdpStateAudio) {
            if (item_key == "streamid") {
                audio_stream_id = item_value;
            }
        }
    }

    return ret;
}

string SrsRtspSdp::base64_decode(string value)
{
    if (value.empty()) {
        return "";
    }

    int nb_output = (int)(value.length() * 2);
    u_int8_t* output = new u_int8_t[nb_output];
    SrsAutoFree(u_int8_t, output);

    int ret = srs_av_base64_decode(output, (char*)value.c_str(), nb_output);
    if (ret <= 0) {
        return "";
    }

    std::string plaintext;
    plaintext.append((char*)output, ret);
    return plaintext;
}

SrsRtspTransport::SrsRtspTransport()
{
    client_port_min = 0;
    client_port_max = 0;
}

SrsRtspTransport::~SrsRtspTransport()
{
}

int SrsRtspTransport::parse(string attr)
{
    int ret = ERROR_SUCCESS;
    
    size_t pos = string::npos;
    std::string token = attr;

    while (!token.empty()) {
        std::string item = token;
        if ((pos = item.find(";")) != string::npos) {
            item = token.substr(0, pos);
            token = token.substr(pos + 1);
        } else {
            token = "";
        }

        std::string item_key = item, item_value;
        if ((pos = item.find("=")) != string::npos) {
            item_key = item.substr(0, pos);
            item_value = item.substr(pos + 1);
        }

        if (transport.empty()) {
            transport = item_key;
            if ((pos = transport.find("/")) != string::npos) {
                profile = transport.substr(pos + 1);
                transport = transport.substr(0, pos);
            }
            if ((pos = profile.find("/")) != string::npos) {
                lower_transport = profile.substr(pos + 1);
                profile = profile.substr(0, pos);
            }
        }

        if (item_key == "unicast" || item_key == "multicast") {
            cast_type = item_key;
        } else if (item_key == "mode") {
            mode = item_value;
        } else if (item_key == "client_port") {
            std::string sport = item_value;
            std::string eport = item_value;
            if ((pos = eport.find("-")) != string::npos) {
                sport = eport.substr(0, pos);
                eport = eport.substr(pos + 1);
            }
            client_port_min = ::atoi(sport.c_str());
            client_port_max = ::atoi(eport.c_str());
        }
    }

    return ret;
}

SrsRtspRequest::SrsRtspRequest()
{
    seq = 0;
    content_length = 0;
    stream_id = 0;
    sdp = NULL;
    transport = NULL;
}

SrsRtspRequest::~SrsRtspRequest()
{
    srs_freep(sdp);
    srs_freep(transport);
}

bool SrsRtspRequest::is_options()
{
    return method == __SRS_METHOD_OPTIONS;
}

bool SrsRtspRequest::is_announce()
{
    return method == __SRS_METHOD_ANNOUNCE;
}

bool SrsRtspRequest::is_setup()
{
    return method == __SRS_METHOD_SETUP;
}

bool SrsRtspRequest::is_record()
{
    return method == __SRS_METHOD_RECORD;
}

SrsRtspResponse::SrsRtspResponse(int cseq)
{
    seq = cseq;
    status = SRS_CONSTS_RTSP_OK;
}

SrsRtspResponse::~SrsRtspResponse()
{
}

int SrsRtspResponse::encode(stringstream& ss)
{
    int ret = ERROR_SUCCESS;

    // status line
    ss << __SRS_VERSION << __SRS_RTSP_SP 
        << status << __SRS_RTSP_SP 
        << srs_generate_rtsp_status_text(status) << __SRS_RTSP_CRLF;

    // cseq
    ss << __SRS_TOKEN_CSEQ << ":" << __SRS_RTSP_SP << seq << __SRS_RTSP_CRLF;

    // others.
    ss << "Cache-Control: no-store" << __SRS_RTSP_CRLF
        << "Pragma: no-cache" << __SRS_RTSP_CRLF
        << "Server: " << RTMP_SIG_SRS_SERVER << __SRS_RTSP_CRLF;

    // session if specified.
    if (!session.empty()) {
        ss << __SRS_TOKEN_SESSION << ":" << session << __SRS_RTSP_CRLF;
    }

    if ((ret = encode_header(ss)) != ERROR_SUCCESS) {
        srs_error("rtsp: encode header failed. ret=%d", ret);
        return ret;
    };

    // header EOF.
    ss << __SRS_RTSP_CRLF;

    return ret;
}

int SrsRtspResponse::encode_header(std::stringstream& ss)
{
    return ERROR_SUCCESS;
}

SrsRtspOptionsResponse::SrsRtspOptionsResponse(int cseq) : SrsRtspResponse(cseq)
{
    methods = (SrsRtspMethod)(SrsRtspMethodDescribe | SrsRtspMethodOptions 
        | SrsRtspMethodPause | SrsRtspMethodPlay | SrsRtspMethodSetup | SrsRtspMethodTeardown
        | SrsRtspMethodAnnounce | SrsRtspMethodRecord);
}

SrsRtspOptionsResponse::~SrsRtspOptionsResponse()
{
}

int SrsRtspOptionsResponse::encode_header(stringstream& ss)
{
    SrsRtspMethod __methods[] = {
        SrsRtspMethodDescribe,
        SrsRtspMethodAnnounce,
        SrsRtspMethodGetParameter,
        SrsRtspMethodOptions,
        SrsRtspMethodPause,
        SrsRtspMethodPlay,
        SrsRtspMethodRecord,
        SrsRtspMethodRedirect,
        SrsRtspMethodSetup,
        SrsRtspMethodSetParameter,
        SrsRtspMethodTeardown,
    };

    ss << __SRS_TOKEN_PUBLIC << ":" << __SRS_RTSP_SP;

    bool appended = false;
    int nb_methods = (int)(sizeof(__methods) / sizeof(SrsRtspMethod));
    for (int i = 0; i < nb_methods; i++) {
        SrsRtspMethod method = __methods[i];
        if (((int)methods & (int)method) != (int)method) {
            continue;
        }

        if (appended) {
            ss << ", ";
        }
        ss << srs_generate_rtsp_method_str(method);
        appended = true;
    }
    ss << __SRS_RTSP_CRLF;

    return ERROR_SUCCESS;
}

SrsRtspSetupResponse::SrsRtspSetupResponse(int seq) : SrsRtspResponse(seq)
{
    local_port_min = 0;
    local_port_max = 0;
}

SrsRtspSetupResponse::~SrsRtspSetupResponse()
{
}

int SrsRtspSetupResponse::encode_header(stringstream& ss)
{
    ss << __SRS_TOKEN_SESSION << ":" << __SRS_RTSP_SP << session << __SRS_RTSP_CRLF;
    ss << __SRS_TOKEN_TRANSPORT << ":" << __SRS_RTSP_SP 
        << "RTP/AVP;unicast;client_port=" << client_port_min << "-" << client_port_max << ";"
        << "server_port=" << local_port_min << "-" << local_port_max
        << __SRS_RTSP_CRLF;
    return ERROR_SUCCESS;
}

SrsRtspStack::SrsRtspStack(ISrsProtocolReaderWriter* s)
{
    buf = new SrsSimpleBuffer();
    skt = s;
}

SrsRtspStack::~SrsRtspStack()
{
    srs_freep(buf);
}

int SrsRtspStack::recv_message(SrsRtspRequest** preq)
{
    int ret = ERROR_SUCCESS;

    SrsRtspRequest* req = new SrsRtspRequest();
    if ((ret = do_recv_message(req)) != ERROR_SUCCESS) {
        srs_freep(req);
        return ret;
    }

    *preq = req;

    return ret;
}

int SrsRtspStack::send_message(SrsRtspResponse* res)
{
    int ret = ERROR_SUCCESS;

    std::stringstream ss;
    // encode the message to string.
    res->encode(ss);

    std::string str = ss.str();
    srs_assert(!str.empty());

    if ((ret = skt->write((char*)str.c_str(), (int)str.length(), NULL)) != ERROR_SUCCESS) {
        if (!srs_is_client_gracefully_close(ret)) {
            srs_error("rtsp: send response failed. ret=%d", ret);
        }
        return ret;
    }
    srs_info("rtsp: send response ok");

    return ret;
}

int SrsRtspStack::do_recv_message(SrsRtspRequest* req)
{
    int ret = ERROR_SUCCESS;

    // parse request line.
    if ((ret = recv_token_normal(req->method)) != ERROR_SUCCESS) {
        if (!srs_is_client_gracefully_close(ret)) {
            srs_error("rtsp: parse method failed. ret=%d", ret);
        }
        return ret;
    }

    if ((ret = recv_token_normal(req->uri)) != ERROR_SUCCESS) {
        if (!srs_is_client_gracefully_close(ret)) {
            srs_error("rtsp: parse uri failed. ret=%d", ret);
        }
        return ret;
    }

    if ((ret = recv_token_eof(req->version)) != ERROR_SUCCESS) {
        if (!srs_is_client_gracefully_close(ret)) {
            srs_error("rtsp: parse version failed. ret=%d", ret);
        }
        return ret;
    }

    // parse headers.
    for (;;) {
        // parse the header name
        std::string token;
        if ((ret = recv_token_normal(token)) != ERROR_SUCCESS) {
            if (ret == ERROR_RTSP_REQUEST_HEADER_EOF) {
                ret = ERROR_SUCCESS;
                srs_info("rtsp: message header parsed");
                break;
            }
            if (!srs_is_client_gracefully_close(ret)) {
                srs_error("rtsp: parse token failed. ret=%d", ret);
            }
            return ret;
        }

        // parse the header value according by header name
        if (token == __SRS_TOKEN_CSEQ) {
            std::string seq;
            if ((ret = recv_token_eof(seq)) != ERROR_SUCCESS) {
                if (!srs_is_client_gracefully_close(ret)) {
                    srs_error("rtsp: parse %s failed. ret=%d", __SRS_TOKEN_CSEQ, ret);
                }
                return ret;
            }
            req->seq = ::atol(seq.c_str());
        } else if (token == __SRS_TOKEN_CONTENT_TYPE) {
            std::string ct;
            if ((ret = recv_token_eof(ct)) != ERROR_SUCCESS) {
                if (!srs_is_client_gracefully_close(ret)) {
                    srs_error("rtsp: parse %s failed. ret=%d", __SRS_TOKEN_CONTENT_TYPE, ret);
                }
                return ret;
            }
            req->content_type = ct;
        } else if (token == __SRS_TOKEN_CONTENT_LENGTH) {
            std::string cl;
            if ((ret = recv_token_eof(cl)) != ERROR_SUCCESS) {
                if (!srs_is_client_gracefully_close(ret)) {
                    srs_error("rtsp: parse %s failed. ret=%d", __SRS_TOKEN_CONTENT_LENGTH, ret);
                }
                return ret;
            }
            req->content_length = ::atol(cl.c_str());
        } else if (token == __SRS_TOKEN_TRANSPORT) {
            std::string transport;
            if ((ret = recv_token_eof(transport)) != ERROR_SUCCESS) {
                if (!srs_is_client_gracefully_close(ret)) {
                    srs_error("rtsp: parse %s failed. ret=%d", __SRS_TOKEN_TRANSPORT, ret);
                }
                return ret;
            }
            if (!req->transport) {
                req->transport = new SrsRtspTransport();
            }
            if ((ret = req->transport->parse(transport)) != ERROR_SUCCESS) {
                srs_error("rtsp: parse transport failed, transport=%s. ret=%d", transport.c_str(), ret);
                return ret;
            }
        } else if (token == __SRS_TOKEN_SESSION) {
            if ((ret = recv_token_eof(req->session)) != ERROR_SUCCESS) {
                if (!srs_is_client_gracefully_close(ret)) {
                    srs_error("rtsp: parse %s failed. ret=%d", __SRS_TOKEN_SESSION, ret);
                }
                return ret;
            }
        } else {
            // unknown header name, parse util EOF.
            SrsRtspTokenState state = SrsRtspTokenStateNormal;
            while (state == SrsRtspTokenStateNormal) {
                std::string value;
                if ((ret = recv_token(value, state)) != ERROR_SUCCESS) {
                    if (!srs_is_client_gracefully_close(ret)) {
                        srs_error("rtsp: parse token failed. ret=%d", ret);
                    }
                    return ret;
                }
                srs_trace("rtsp: ignore header %s=%s", token.c_str(), value.c_str());
            }
        }
    }

    // for setup, parse the stream id from uri.
    if (req->is_setup()) {
        size_t pos = string::npos;
        std::string stream_id;
        if ((pos = req->uri.rfind("/")) != string::npos) {
            stream_id = req->uri.substr(pos + 1);
        }
        if ((pos = stream_id.find("=")) != string::npos) {
            stream_id = stream_id.substr(pos + 1);
        }
        req->stream_id = ::atoi(stream_id.c_str());
        srs_info("rtsp: setup stream id=%d", req->stream_id);
    }

    // parse rdp body.
    long consumed = 0;
    while (consumed < req->content_length) {
        if (!req->sdp) {
            req->sdp = new SrsRtspSdp();
        }

        int nb_token = 0;
        std::string token;
        if ((ret = recv_token_util_eof(token, &nb_token)) != ERROR_SUCCESS) {
            if (!srs_is_client_gracefully_close(ret)) {
                srs_error("rtsp: parse sdp token failed. ret=%d", ret);
            }
            return ret;
        }
        consumed += nb_token;

        if ((ret = req->sdp->parse(token)) != ERROR_SUCCESS) {
            srs_error("rtsp: sdp parse token failed, token=%s. ret=%d", token.c_str(), ret);
            return ret;
        }
        srs_info("rtsp: %s", token.c_str());
    }
    srs_info("rtsp: sdp parsed, size=%d", consumed);

    return ret;
}

int SrsRtspStack::recv_token_normal(std::string& token)
{
    int ret = ERROR_SUCCESS;

    SrsRtspTokenState state;

    if ((ret = recv_token(token, state)) != ERROR_SUCCESS) {
        if (ret == ERROR_RTSP_REQUEST_HEADER_EOF) {
            return ret;
        }
        if (!srs_is_client_gracefully_close(ret)) {
            srs_error("rtsp: parse token failed. ret=%d", ret);
        }
        return ret;
    }

    if (state != SrsRtspTokenStateNormal) {
        ret = ERROR_RTSP_TOKEN_NOT_NORMAL;
        srs_error("rtsp: parse normal token failed, state=%d. ret=%d", state, ret);
        return ret;
    }

    return ret;
}

int SrsRtspStack::recv_token_eof(std::string& token)
{
    int ret = ERROR_SUCCESS;

    SrsRtspTokenState state;

    if ((ret = recv_token(token, state)) != ERROR_SUCCESS) {
        if (ret == ERROR_RTSP_REQUEST_HEADER_EOF) {
            return ret;
        }
        if (!srs_is_client_gracefully_close(ret)) {
            srs_error("rtsp: parse token failed. ret=%d", ret);
        }
        return ret;
    }

    if (state != SrsRtspTokenStateEOF) {
        ret = ERROR_RTSP_TOKEN_NOT_NORMAL;
        srs_error("rtsp: parse eof token failed, state=%d. ret=%d", state, ret);
        return ret;
    }

    return ret;
}

int SrsRtspStack::recv_token_util_eof(std::string& token, int* pconsumed)
{
    int ret = ERROR_SUCCESS;

    SrsRtspTokenState state;

    // use 0x00 as ignore the normal token flag.
    if ((ret = recv_token(token, state, 0x00, pconsumed)) != ERROR_SUCCESS) {
        if (ret == ERROR_RTSP_REQUEST_HEADER_EOF) {
            return ret;
        }
        if (!srs_is_client_gracefully_close(ret)) {
            srs_error("rtsp: parse token failed. ret=%d", ret);
        }
        return ret;
    }

    if (state != SrsRtspTokenStateEOF) {
        ret = ERROR_RTSP_TOKEN_NOT_NORMAL;
        srs_error("rtsp: parse eof token failed, state=%d. ret=%d", state, ret);
        return ret;
    }

    return ret;
}

int SrsRtspStack::recv_token(std::string& token, SrsRtspTokenState& state, char normal_ch, int* pconsumed)
{
    int ret = ERROR_SUCCESS;

    // whatever, default to error state.
    state = SrsRtspTokenStateError;

    // when buffer is empty, append bytes first.
    bool append_bytes = buf->length() == 0;

    // parse util token.
    for (;;) {
        // append bytes if required.
        if (append_bytes) {
            append_bytes = false;

            char buffer[__SRS_RTSP_BUFFER];
            ssize_t nb_read = 0;
            if ((ret = skt->read(buffer, __SRS_RTSP_BUFFER, &nb_read)) != ERROR_SUCCESS) {
                if (!srs_is_client_gracefully_close(ret)) {
                    srs_error("rtsp: io read failed. ret=%d", ret);
                }
                return ret;
            }
            srs_info("rtsp: io read %d bytes", nb_read);

            buf->append(buffer, nb_read);
        }

        // parse one by one.
        char* start = buf->bytes();
        char* end = start + buf->length();
        char* p = start;

        // find util SP/CR/LF, max 2 EOF, to finger out the EOF of message.
        for (; p < end && p[0] != normal_ch && p[0] != __SRS_RTSP_CR && p[0] != __SRS_RTSP_LF; p++) {
        }

        // matched.
        if (p < end) {
            // finger out the state.
            if (p[0] == normal_ch) {
                state = SrsRtspTokenStateNormal;
            } else {
                state = SrsRtspTokenStateEOF;
            }
            
            // got the token.
            int nb_token = p - start;
            // trim last ':' character.
            if (nb_token && p[-1] == ':') {
                nb_token--;
            }
            if (nb_token) {
                token.append(start, nb_token);
            } else {
                ret = ERROR_RTSP_REQUEST_HEADER_EOF;
            }

            // ignore SP/CR/LF
            for (int i = 0; i < 2 && p < end && (p[0] == normal_ch || p[0] == __SRS_RTSP_CR || p[0] == __SRS_RTSP_LF); p++, i++) {
            }

            // consume the token bytes.
            srs_assert(p - start);
            buf->erase(p - start);
            if (pconsumed) {
                *pconsumed = p - start;
            }
            break;
        }

        // append more and parse again.
        append_bytes = true;
    }

    return ret;
}

#endif

