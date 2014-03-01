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

#include <srs_core_rtmp.hpp>

#include <srs_core_log.hpp>
#include <srs_kernel_error.hpp>
#include <srs_core_socket.hpp>
#include <srs_core_protocol.hpp>
#include <srs_core_autofree.hpp>
#include <srs_core_amf0.hpp>
#include <srs_core_handshake.hpp>
#include <srs_core_config.hpp>

using namespace std;

/**
* the signature for packets to client.
*/
#define RTMP_SIG_FMS_VER 					"3,5,3,888"
#define RTMP_SIG_AMF0_VER 					0
#define RTMP_SIG_CLIENT_ID 					"ASAICiss"

/**
* onStatus consts.
*/
#define StatusLevel 						"level"
#define StatusCode 							"code"
#define StatusDescription 					"description"
#define StatusDetails 						"details"
#define StatusClientId 						"clientid"
// status value
#define StatusLevelStatus 					"status"
// status error
#define StatusLevelError                    "error"
// code value
#define StatusCodeConnectSuccess 			"NetConnection.Connect.Success"
#define StatusCodeConnectRejected 			"NetConnection.Connect.Rejected"
#define StatusCodeStreamReset 				"NetStream.Play.Reset"
#define StatusCodeStreamStart 				"NetStream.Play.Start"
#define StatusCodeStreamPause 				"NetStream.Pause.Notify"
#define StatusCodeStreamUnpause 			"NetStream.Unpause.Notify"
#define StatusCodePublishStart 				"NetStream.Publish.Start"
#define StatusCodeDataStart 				"NetStream.Data.Start"
#define StatusCodeUnpublishSuccess 			"NetStream.Unpublish.Success"

// FMLE
#define RTMP_AMF0_COMMAND_ON_FC_PUBLISH		"onFCPublish"
#define RTMP_AMF0_COMMAND_ON_FC_UNPUBLISH	"onFCUnpublish"

// default stream id for response the createStream request.
#define SRS_DEFAULT_SID 					1

SrsRequest::SrsRequest()
{
	objectEncoding = RTMP_SIG_AMF0_VER;
}

SrsRequest::~SrsRequest()
{
}

SrsRequest* SrsRequest::copy()
{
	SrsRequest* cp = new SrsRequest();
	
	cp->app = app;
	cp->objectEncoding = objectEncoding;
	cp->pageUrl = pageUrl;
	cp->port = port;
	cp->schema = schema;
	cp->stream = stream;
	cp->swfUrl = swfUrl;
	cp->tcUrl = tcUrl;
	cp->vhost = vhost;
	
	return cp;
}

int SrsRequest::discovery_app()
{
	int ret = ERROR_SUCCESS;
	
	size_t pos = std::string::npos;
	std::string url = tcUrl;
	
	if ((pos = url.find("://")) != std::string::npos) {
		schema = url.substr(0, pos);
		url = url.substr(schema.length() + 3);
		srs_verbose("discovery schema=%s", schema.c_str());
	}
	
	if ((pos = url.find("/")) != std::string::npos) {
		vhost = url.substr(0, pos);
		url = url.substr(vhost.length() + 1);
		srs_verbose("discovery vhost=%s", vhost.c_str());
	}

	port = RTMP_DEFAULT_PORTS;
	if ((pos = vhost.find(":")) != std::string::npos) {
		port = vhost.substr(pos + 1);
		vhost = vhost.substr(0, pos);
		srs_verbose("discovery vhost=%s, port=%s", vhost.c_str(), port.c_str());
	}
	
	app = url;
	srs_vhost_resolve(vhost, app);
	strip();
	
	// resolve the vhost from config
	SrsConfDirective* parsed_vhost = config->get_vhost(vhost);
	if (parsed_vhost) {
		vhost = parsed_vhost->arg0();
	}

	// TODO: discovery the params of vhost.
	
	srs_info("discovery app success. schema=%s, vhost=%s, port=%s, app=%s",
		schema.c_str(), vhost.c_str(), port.c_str(), app.c_str());
	
	if (schema.empty() || vhost.empty() || port.empty() || app.empty()) {
		ret = ERROR_RTMP_REQ_TCURL;
		srs_error("discovery tcUrl failed. "
			"tcUrl=%s, schema=%s, vhost=%s, port=%s, app=%s, ret=%d",
			tcUrl.c_str(), schema.c_str(), vhost.c_str(), port.c_str(), app.c_str(), ret);
		return ret;
	}
	
	strip();
	
	return ret;
}

string SrsRequest::get_stream_url()
{
	std::string url = "";
	
	url += vhost;
	url += "/";
	url += app;
	url += "/";
	url += stream;

	return url;
}

void SrsRequest::strip()
{
	trim(vhost, "/ \n\r\t");
	trim(app, "/ \n\r\t");
	trim(stream, "/ \n\r\t");
}

string& SrsRequest::trim(string& str, string chs)
{
	for (int i = 0; i < (int)chs.length(); i++) {
		char ch = chs.at(i);
		
		for (std::string::iterator it = str.begin(); it != str.end();) {
			if (ch == *it) {
				it = str.erase(it);
			} else {
				++it;
			}
		}
	}
	
	return str;
}

SrsResponse::SrsResponse()
{
	stream_id = SRS_DEFAULT_SID;
}

SrsResponse::~SrsResponse()
{
}

SrsRtmpClient::SrsRtmpClient(st_netfd_t _stfd)
{
	stfd = _stfd;
	protocol = new SrsProtocol(stfd);
}

SrsRtmpClient::~SrsRtmpClient()
{
	srs_freep(protocol);
}

void SrsRtmpClient::set_recv_timeout(int64_t timeout_us)
{
	protocol->set_recv_timeout(timeout_us);
}

void SrsRtmpClient::set_send_timeout(int64_t timeout_us)
{
	protocol->set_send_timeout(timeout_us);
}

int64_t SrsRtmpClient::get_recv_bytes()
{
	return protocol->get_recv_bytes();
}

int64_t SrsRtmpClient::get_send_bytes()
{
	return protocol->get_send_bytes();
}

int SrsRtmpClient::get_recv_kbps()
{
	return protocol->get_recv_kbps();
}

int SrsRtmpClient::get_send_kbps()
{
	return protocol->get_send_kbps();
}

int SrsRtmpClient::recv_message(SrsCommonMessage** pmsg)
{
	return protocol->recv_message(pmsg);
}

int SrsRtmpClient::send_message(ISrsMessage* msg)
{
	return protocol->send_message(msg);
}

int SrsRtmpClient::handshake()
{
	int ret = ERROR_SUCCESS;
	
    SrsSocket skt(stfd);
    
    skt.set_recv_timeout(protocol->get_recv_timeout());
    skt.set_send_timeout(protocol->get_send_timeout());
    
    SrsComplexHandshake complex_hs;
    SrsSimpleHandshake simple_hs;
    if ((ret = simple_hs.handshake_with_server(skt, complex_hs)) != ERROR_SUCCESS) {
        return ret;
    }
    
    return ret;
}

int SrsRtmpClient::connect_app(string app, string tc_url)
{
	int ret = ERROR_SUCCESS;
	
	// Connect(vhost, app)
	if (true) {
		SrsCommonMessage* msg = new SrsCommonMessage();
		SrsConnectAppPacket* pkt = new SrsConnectAppPacket();
		msg->set_packet(pkt, 0);
		
		pkt->command_object = new SrsAmf0Object();
		pkt->command_object->set("app", new SrsAmf0String(app.c_str()));
		pkt->command_object->set("swfUrl", new SrsAmf0String());
		pkt->command_object->set("tcUrl", new SrsAmf0String(tc_url.c_str()));
		pkt->command_object->set("fpad", new SrsAmf0Boolean(false));
		pkt->command_object->set("capabilities", new SrsAmf0Number(239));
		pkt->command_object->set("audioCodecs", new SrsAmf0Number(3575));
		pkt->command_object->set("videoCodecs", new SrsAmf0Number(252));
		pkt->command_object->set("videoFunction", new SrsAmf0Number(1));
		pkt->command_object->set("pageUrl", new SrsAmf0String());
		pkt->command_object->set("objectEncoding", new SrsAmf0Number(0));
		
		if ((ret = protocol->send_message(msg)) != ERROR_SUCCESS) {
			return ret;
		}
	}
	
	// Set Window Acknowledgement size(2500000)
	if (true) {
		SrsCommonMessage* msg = new SrsCommonMessage();
		SrsSetWindowAckSizePacket* pkt = new SrsSetWindowAckSizePacket();
	
		pkt->ackowledgement_window_size = 2500000;
		msg->set_packet(pkt, 0);
		
		if ((ret = protocol->send_message(msg)) != ERROR_SUCCESS) {
			return ret;
		}
	}
	
	// expect connect _result
	SrsCommonMessage* msg = NULL;
	SrsConnectAppResPacket* pkt = NULL;
	if ((ret = srs_rtmp_expect_message<SrsConnectAppResPacket>(protocol, &msg, &pkt)) != ERROR_SUCCESS) {
		srs_error("expect connect app response message failed. ret=%d", ret);
		return ret;
	}
	SrsAutoFree(SrsCommonMessage, msg, false);
	srs_info("get connect app response message");
	
    return ret;
}

int SrsRtmpClient::create_stream(int& stream_id)
{
	int ret = ERROR_SUCCESS;
	
	// CreateStream
	if (true) {
		SrsCommonMessage* msg = new SrsCommonMessage();
		SrsCreateStreamPacket* pkt = new SrsCreateStreamPacket();
	
		msg->set_packet(pkt, 0);
		
		if ((ret = protocol->send_message(msg)) != ERROR_SUCCESS) {
			return ret;
		}
	}

	// CreateStream _result.
	if (true) {
		SrsCommonMessage* msg = NULL;
		SrsCreateStreamResPacket* pkt = NULL;
		if ((ret = srs_rtmp_expect_message<SrsCreateStreamResPacket>(protocol, &msg, &pkt)) != ERROR_SUCCESS) {
			srs_error("expect create stream response message failed. ret=%d", ret);
			return ret;
		}
		SrsAutoFree(SrsCommonMessage, msg, false);
		srs_info("get create stream response message");

		stream_id = (int)pkt->stream_id;
	}
	
	return ret;
}

int SrsRtmpClient::play(string stream, int stream_id)
{
	int ret = ERROR_SUCCESS;

	// Play(stream)
	if (true) {
		SrsCommonMessage* msg = new SrsCommonMessage();
		SrsPlayPacket* pkt = new SrsPlayPacket();
	
		pkt->stream_name = stream;
		msg->set_packet(pkt, stream_id);
		
		if ((ret = protocol->send_message(msg)) != ERROR_SUCCESS) {
			srs_error("send play stream failed. "
				"stream=%s, stream_id=%d, ret=%d", 
				stream.c_str(), stream_id, ret);
			return ret;
		}
	}
	
	// SetBufferLength(1000ms)
	int buffer_length_ms = 1000;
	if (true) {
		SrsCommonMessage* msg = new SrsCommonMessage();
		SrsUserControlPacket* pkt = new SrsUserControlPacket();
	
		pkt->event_type = SrcPCUCSetBufferLength;
		pkt->event_data = stream_id;
		pkt->extra_data = buffer_length_ms;
		msg->set_packet(pkt, 0);
		
		if ((ret = protocol->send_message(msg)) != ERROR_SUCCESS) {
			srs_error("send set buffer length failed. "
				"stream=%s, stream_id=%d, bufferLength=%d, ret=%d", 
				stream.c_str(), stream_id, buffer_length_ms, ret);
			return ret;
		}
	}
	
	// SetChunkSize
	if (true) {
		SrsCommonMessage* msg = new SrsCommonMessage();
		SrsSetChunkSizePacket* pkt = new SrsSetChunkSizePacket();
	
		pkt->chunk_size = SRS_CONF_DEFAULT_CHUNK_SIZE;
		msg->set_packet(pkt, 0);
		
		if ((ret = protocol->send_message(msg)) != ERROR_SUCCESS) {
			srs_error("send set chunk size failed. "
				"stream=%s, chunk_size=%d, ret=%d", 
				stream.c_str(), SRS_CONF_DEFAULT_CHUNK_SIZE, ret);
			return ret;
		}
	}
	
	return ret;
}

int SrsRtmpClient::publish(string stream, int stream_id)
{
	int ret = ERROR_SUCCESS;
	
	// SetChunkSize
	if (true) {
		SrsCommonMessage* msg = new SrsCommonMessage();
		SrsSetChunkSizePacket* pkt = new SrsSetChunkSizePacket();
	
		pkt->chunk_size = SRS_CONF_DEFAULT_CHUNK_SIZE;
		msg->set_packet(pkt, 0);
		
		if ((ret = protocol->send_message(msg)) != ERROR_SUCCESS) {
			srs_error("send set chunk size failed. "
				"stream=%s, chunk_size=%d, ret=%d", 
				stream.c_str(), SRS_CONF_DEFAULT_CHUNK_SIZE, ret);
			return ret;
		}
	}

	// publish(stream)
	if (true) {
		SrsCommonMessage* msg = new SrsCommonMessage();
		SrsPublishPacket* pkt = new SrsPublishPacket();
	
		pkt->stream_name = stream;
		msg->set_packet(pkt, stream_id);
		
		if ((ret = protocol->send_message(msg)) != ERROR_SUCCESS) {
			srs_error("send publish message failed. "
				"stream=%s, stream_id=%d, ret=%d", 
				stream.c_str(), stream_id, ret);
			return ret;
		}
	}
	
	return ret;
}

SrsRtmp::SrsRtmp(st_netfd_t client_stfd)
{
	protocol = new SrsProtocol(client_stfd);
	stfd = client_stfd;
}

SrsRtmp::~SrsRtmp()
{
	srs_freep(protocol);
}

SrsProtocol* SrsRtmp::get_protocol()
{
	return protocol;
}

void SrsRtmp::set_recv_timeout(int64_t timeout_us)
{
	protocol->set_recv_timeout(timeout_us);
}

int64_t SrsRtmp::get_recv_timeout()
{
	return protocol->get_recv_timeout();
}

void SrsRtmp::set_send_timeout(int64_t timeout_us)
{
	protocol->set_send_timeout(timeout_us);
}

int64_t SrsRtmp::get_send_timeout()
{
	return protocol->get_send_timeout();
}

int64_t SrsRtmp::get_recv_bytes()
{
	return protocol->get_recv_bytes();
}

int64_t SrsRtmp::get_send_bytes()
{
	return protocol->get_send_bytes();
}

int SrsRtmp::get_recv_kbps()
{
	return protocol->get_recv_kbps();
}

int SrsRtmp::get_send_kbps()
{
	return protocol->get_send_kbps();
}

int SrsRtmp::recv_message(SrsCommonMessage** pmsg)
{
	return protocol->recv_message(pmsg);
}

int SrsRtmp::send_message(ISrsMessage* msg)
{
	return protocol->send_message(msg);
}

int SrsRtmp::handshake()
{
	int ret = ERROR_SUCCESS;
	
    SrsSocket skt(stfd);
    
    skt.set_recv_timeout(protocol->get_recv_timeout());
    skt.set_send_timeout(protocol->get_send_timeout());
    
    SrsComplexHandshake complex_hs;
    SrsSimpleHandshake simple_hs;
    if ((ret = simple_hs.handshake_with_client(skt, complex_hs)) != ERROR_SUCCESS) {
        return ret;
    }
    
    return ret;
}

int SrsRtmp::connect_app(SrsRequest* req)
{
	int ret = ERROR_SUCCESS;
	
	SrsCommonMessage* msg = NULL;
	SrsConnectAppPacket* pkt = NULL;
	if ((ret = srs_rtmp_expect_message<SrsConnectAppPacket>(protocol, &msg, &pkt)) != ERROR_SUCCESS) {
		srs_error("expect connect app message failed. ret=%d", ret);
		return ret;
	}
	SrsAutoFree(SrsCommonMessage, msg, false);
	srs_info("get connect app message");
	
	SrsAmf0Any* prop = NULL;
	
	if ((prop = pkt->command_object->ensure_property_string("tcUrl")) == NULL) {
		ret = ERROR_RTMP_REQ_CONNECT;
		srs_error("invalid request, must specifies the tcUrl. ret=%d", ret);
		return ret;
	}
	req->tcUrl = srs_amf0_convert<SrsAmf0String>(prop)->value;
	
	if ((prop = pkt->command_object->ensure_property_string("pageUrl")) != NULL) {
		req->pageUrl = srs_amf0_convert<SrsAmf0String>(prop)->value;
	}
	
	if ((prop = pkt->command_object->ensure_property_string("swfUrl")) != NULL) {
		req->swfUrl = srs_amf0_convert<SrsAmf0String>(prop)->value;
	}
	
	if ((prop = pkt->command_object->ensure_property_number("objectEncoding")) != NULL) {
		req->objectEncoding = srs_amf0_convert<SrsAmf0Number>(prop)->value;
	}
	
	srs_info("get connect app message params success.");
	
	return req->discovery_app();
}

int SrsRtmp::set_window_ack_size(int ack_size)
{
	int ret = ERROR_SUCCESS;
	
	SrsCommonMessage* msg = new SrsCommonMessage();
	SrsSetWindowAckSizePacket* pkt = new SrsSetWindowAckSizePacket();
	
	pkt->ackowledgement_window_size = ack_size;
	msg->set_packet(pkt, 0);
	
	if ((ret = protocol->send_message(msg)) != ERROR_SUCCESS) {
		srs_error("send ack size message failed. ret=%d", ret);
		return ret;
	}
	srs_info("send ack size message success. ack_size=%d", ack_size);
	
	return ret;
}

int SrsRtmp::set_peer_bandwidth(int bandwidth, int type)
{
	int ret = ERROR_SUCCESS;
	
	SrsCommonMessage* msg = new SrsCommonMessage();
	SrsSetPeerBandwidthPacket* pkt = new SrsSetPeerBandwidthPacket();
	
	pkt->bandwidth = bandwidth;
	pkt->type = type;
	msg->set_packet(pkt, 0);
	
	if ((ret = protocol->send_message(msg)) != ERROR_SUCCESS) {
		srs_error("send set bandwidth message failed. ret=%d", ret);
		return ret;
	}
	srs_info("send set bandwidth message "
		"success. bandwidth=%d, type=%d", bandwidth, type);
	
	return ret;
}

int SrsRtmp::response_connect_app(SrsRequest *req, const char* server_ip)
{
	int ret = ERROR_SUCCESS;
	
	SrsCommonMessage* msg = new SrsCommonMessage();
	SrsConnectAppResPacket* pkt = new SrsConnectAppResPacket();
	
	pkt->props->set("fmsVer", new SrsAmf0String("FMS/"RTMP_SIG_FMS_VER));
	pkt->props->set("capabilities", new SrsAmf0Number(127));
	pkt->props->set("mode", new SrsAmf0Number(1));
	
	pkt->info->set(StatusLevel, new SrsAmf0String(StatusLevelStatus));
	pkt->info->set(StatusCode, new SrsAmf0String(StatusCodeConnectSuccess));
	pkt->info->set(StatusDescription, new SrsAmf0String("Connection succeeded"));
	pkt->info->set("objectEncoding", new SrsAmf0Number(req->objectEncoding));
	SrsASrsAmf0EcmaArray* data = new SrsASrsAmf0EcmaArray();
	pkt->info->set("data", data);
	
	data->set("version", new SrsAmf0String(RTMP_SIG_FMS_VER));
	data->set("srs_sig", new SrsAmf0String(RTMP_SIG_SRS_KEY));
	data->set("srs_server", new SrsAmf0String(RTMP_SIG_SRS_KEY" "RTMP_SIG_SRS_VERSION" ("RTMP_SIG_SRS_URL_SHORT")"));
	data->set("srs_license", new SrsAmf0String(RTMP_SIG_SRS_LICENSE));
	data->set("srs_role", new SrsAmf0String(RTMP_SIG_SRS_ROLE));
	data->set("srs_url", new SrsAmf0String(RTMP_SIG_SRS_URL));
	data->set("srs_version", new SrsAmf0String(RTMP_SIG_SRS_VERSION));
	data->set("srs_site", new SrsAmf0String(RTMP_SIG_SRS_WEB));
	data->set("srs_email", new SrsAmf0String(RTMP_SIG_SRS_EMAIL));
	data->set("srs_copyright", new SrsAmf0String(RTMP_SIG_SRS_COPYRIGHT));
	data->set("srs_primary_authors", new SrsAmf0String(RTMP_SIG_SRS_PRIMARY_AUTHROS));
	
    if (server_ip) {
        data->set("srs_server_ip", new SrsAmf0String(server_ip));
    }
	
	msg->set_packet(pkt, 0);
	
	if ((ret = protocol->send_message(msg)) != ERROR_SUCCESS) {
		srs_error("send connect app response message failed. ret=%d", ret);
		return ret;
	}
	srs_info("send connect app response message success.");
	
    return ret;
}

void SrsRtmp::response_connect_reject(SrsRequest *req, const char* desc)
{
    int ret = ERROR_SUCCESS;

    SrsConnectAppResPacket* pkt = new SrsConnectAppResPacket();
    pkt->command_name = "_error";
    pkt->props->set(StatusLevel, new SrsAmf0String(StatusLevelError));
    pkt->props->set(StatusCode, new SrsAmf0String(StatusCodeConnectRejected));
    pkt->props->set(StatusDescription, new SrsAmf0String(desc));
    //pkt->props->set("objectEncoding", new SrsAmf0Number(req->objectEncoding));

    SrsCommonMessage* msg = (new SrsCommonMessage())->set_packet(pkt, 0);
    if ((ret = protocol->send_message(msg)) != ERROR_SUCCESS) {
        srs_error("send connect app response rejected message failed. ret=%d", ret);
        return;
    }
    srs_info("send connect app response rejected message success.");

    return;
}

int SrsRtmp::on_bw_done()
{
	int ret = ERROR_SUCCESS;
	
	SrsCommonMessage* msg = new SrsCommonMessage();
	SrsOnBWDonePacket* pkt = new SrsOnBWDonePacket();
	
	msg->set_packet(pkt, 0);
	
	if ((ret = protocol->send_message(msg)) != ERROR_SUCCESS) {
		srs_error("send onBWDone message failed. ret=%d", ret);
		return ret;
	}
	srs_info("send onBWDone message success.");
	
	return ret;
}

int SrsRtmp::identify_client(int stream_id, SrsClientType& type, string& stream_name)
{
	type = SrsClientUnknown;
	int ret = ERROR_SUCCESS;
	
	while (true) {
		SrsCommonMessage* msg = NULL;
		if ((ret = protocol->recv_message(&msg)) != ERROR_SUCCESS) {
			srs_error("recv identify client message failed. ret=%d", ret);
			return ret;
		}

		SrsAutoFree(SrsCommonMessage, msg, false);

		if (!msg->header.is_amf0_command() && !msg->header.is_amf3_command()) {
			srs_trace("identify ignore messages except "
				"AMF0/AMF3 command message. type=%#x", msg->header.message_type);
			continue;
		}
		
		if ((ret = msg->decode_packet(protocol)) != ERROR_SUCCESS) {
			srs_error("identify decode message failed. ret=%d", ret);
			return ret;
		}
		
		SrsPacket* pkt = msg->get_packet();
		if (dynamic_cast<SrsCreateStreamPacket*>(pkt)) {
			srs_info("identify client by create stream, play or flash publish.");
			return identify_create_stream_client(dynamic_cast<SrsCreateStreamPacket*>(pkt), stream_id, type, stream_name);
		}
		if (dynamic_cast<SrsFMLEStartPacket*>(pkt)) {
			srs_info("identify client by releaseStream, fmle publish.");
			return identify_fmle_publish_client(dynamic_cast<SrsFMLEStartPacket*>(pkt), type, stream_name);
		}
		if (dynamic_cast<SrsPlayPacket*>(pkt)) {
			srs_info("level0 identify client by play.");
			return identify_play_client(dynamic_cast<SrsPlayPacket*>(pkt), type, stream_name);
		}
		
		srs_trace("ignore AMF0/AMF3 command message.");
	}
	
	return ret;
}

int SrsRtmp::set_chunk_size(int chunk_size)
{
	int ret = ERROR_SUCCESS;
	
	SrsCommonMessage* msg = new SrsCommonMessage();
	SrsSetChunkSizePacket* pkt = new SrsSetChunkSizePacket();
	
	pkt->chunk_size = chunk_size;
	msg->set_packet(pkt, 0);
	
	if ((ret = protocol->send_message(msg)) != ERROR_SUCCESS) {
		srs_error("send set chunk size message failed. ret=%d", ret);
		return ret;
	}
	srs_info("send set chunk size message success. chunk_size=%d", chunk_size);
	
	return ret;
}

int SrsRtmp::start_play(int stream_id)
{
	int ret = ERROR_SUCCESS;
	
	// StreamBegin
	if (true) {
		SrsCommonMessage* msg = new SrsCommonMessage();
		SrsUserControlPacket* pkt = new SrsUserControlPacket();
		
		pkt->event_type = SrcPCUCStreamBegin;
		pkt->event_data = stream_id;
		msg->set_packet(pkt, 0);
		
		if ((ret = protocol->send_message(msg)) != ERROR_SUCCESS) {
			srs_error("send PCUC(StreamBegin) message failed. ret=%d", ret);
			return ret;
		}
		srs_info("send PCUC(StreamBegin) message success.");
	}
	
	// onStatus(NetStream.Play.Reset)
	if (true) {
		SrsCommonMessage* msg = new SrsCommonMessage();
		SrsOnStatusCallPacket* pkt = new SrsOnStatusCallPacket();
		
		pkt->data->set(StatusLevel, new SrsAmf0String(StatusLevelStatus));
		pkt->data->set(StatusCode, new SrsAmf0String(StatusCodeStreamReset));
		pkt->data->set(StatusDescription, new SrsAmf0String("Playing and resetting stream."));
		pkt->data->set(StatusDetails, new SrsAmf0String("stream"));
		pkt->data->set(StatusClientId, new SrsAmf0String(RTMP_SIG_CLIENT_ID));
		
		msg->set_packet(pkt, stream_id);
		
		if ((ret = protocol->send_message(msg)) != ERROR_SUCCESS) {
			srs_error("send onStatus(NetStream.Play.Reset) message failed. ret=%d", ret);
			return ret;
		}
		srs_info("send onStatus(NetStream.Play.Reset) message success.");
	}
	
	// onStatus(NetStream.Play.Start)
	if (true) {
		SrsCommonMessage* msg = new SrsCommonMessage();
		SrsOnStatusCallPacket* pkt = new SrsOnStatusCallPacket();
		
		pkt->data->set(StatusLevel, new SrsAmf0String(StatusLevelStatus));
		pkt->data->set(StatusCode, new SrsAmf0String(StatusCodeStreamStart));
		pkt->data->set(StatusDescription, new SrsAmf0String("Started playing stream."));
		pkt->data->set(StatusDetails, new SrsAmf0String("stream"));
		pkt->data->set(StatusClientId, new SrsAmf0String(RTMP_SIG_CLIENT_ID));
		
		msg->set_packet(pkt, stream_id);
		
		if ((ret = protocol->send_message(msg)) != ERROR_SUCCESS) {
			srs_error("send onStatus(NetStream.Play.Reset) message failed. ret=%d", ret);
			return ret;
		}
		srs_info("send onStatus(NetStream.Play.Reset) message success.");
	}
	
	// |RtmpSampleAccess(false, false)
	if (true) {
		SrsCommonMessage* msg = new SrsCommonMessage();
		SrsSampleAccessPacket* pkt = new SrsSampleAccessPacket();
		
		msg->set_packet(pkt, stream_id);
		
		if ((ret = protocol->send_message(msg)) != ERROR_SUCCESS) {
			srs_error("send |RtmpSampleAccess(false, false) message failed. ret=%d", ret);
			return ret;
		}
		srs_info("send |RtmpSampleAccess(false, false) message success.");
	}
	
	// onStatus(NetStream.Data.Start)
	if (true) {
		SrsCommonMessage* msg = new SrsCommonMessage();
		SrsOnStatusDataPacket* pkt = new SrsOnStatusDataPacket();
		
		pkt->data->set(StatusCode, new SrsAmf0String(StatusCodeDataStart));
		
		msg->set_packet(pkt, stream_id);
		
		if ((ret = protocol->send_message(msg)) != ERROR_SUCCESS) {
			srs_error("send onStatus(NetStream.Data.Start) message failed. ret=%d", ret);
			return ret;
		}
		srs_info("send onStatus(NetStream.Data.Start) message success.");
	}
	
	srs_info("start play success.");
	
	return ret;
}

int SrsRtmp::on_play_client_pause(int stream_id, bool is_pause)
{
	int ret = ERROR_SUCCESS;
	
	if (is_pause) {
		// onStatus(NetStream.Pause.Notify)
		if (true) {
			SrsCommonMessage* msg = new SrsCommonMessage();
			SrsOnStatusCallPacket* pkt = new SrsOnStatusCallPacket();
			
			pkt->data->set(StatusLevel, new SrsAmf0String(StatusLevelStatus));
			pkt->data->set(StatusCode, new SrsAmf0String(StatusCodeStreamPause));
			pkt->data->set(StatusDescription, new SrsAmf0String("Paused stream."));
			
			msg->set_packet(pkt, stream_id);
			
			if ((ret = protocol->send_message(msg)) != ERROR_SUCCESS) {
				srs_error("send onStatus(NetStream.Pause.Notify) message failed. ret=%d", ret);
				return ret;
			}
			srs_info("send onStatus(NetStream.Pause.Notify) message success.");
		}
		// StreamEOF
		if (true) {
			SrsCommonMessage* msg = new SrsCommonMessage();
			SrsUserControlPacket* pkt = new SrsUserControlPacket();
			
			pkt->event_type = SrcPCUCStreamEOF;
			pkt->event_data = stream_id;
			msg->set_packet(pkt, 0);
			
			if ((ret = protocol->send_message(msg)) != ERROR_SUCCESS) {
				srs_error("send PCUC(StreamEOF) message failed. ret=%d", ret);
				return ret;
			}
			srs_info("send PCUC(StreamEOF) message success.");
		}
	} else {
		// onStatus(NetStream.Unpause.Notify)
		if (true) {
			SrsCommonMessage* msg = new SrsCommonMessage();
			SrsOnStatusCallPacket* pkt = new SrsOnStatusCallPacket();
			
			pkt->data->set(StatusLevel, new SrsAmf0String(StatusLevelStatus));
			pkt->data->set(StatusCode, new SrsAmf0String(StatusCodeStreamUnpause));
			pkt->data->set(StatusDescription, new SrsAmf0String("Unpaused stream."));
			
			msg->set_packet(pkt, stream_id);
			
			if ((ret = protocol->send_message(msg)) != ERROR_SUCCESS) {
				srs_error("send onStatus(NetStream.Unpause.Notify) message failed. ret=%d", ret);
				return ret;
			}
			srs_info("send onStatus(NetStream.Unpause.Notify) message success.");
		}
		// StreanBegin
		if (true) {
			SrsCommonMessage* msg = new SrsCommonMessage();
			SrsUserControlPacket* pkt = new SrsUserControlPacket();
			
			pkt->event_type = SrcPCUCStreamBegin;
			pkt->event_data = stream_id;
			msg->set_packet(pkt, 0);
			
			if ((ret = protocol->send_message(msg)) != ERROR_SUCCESS) {
				srs_error("send PCUC(StreanBegin) message failed. ret=%d", ret);
				return ret;
			}
			srs_info("send PCUC(StreanBegin) message success.");
		}
	}
	
	return ret;
}

int SrsRtmp::start_fmle_publish(int stream_id)
{
	int ret = ERROR_SUCCESS;
	
	// FCPublish
	double fc_publish_tid = 0;
	if (true) {
		SrsCommonMessage* msg = NULL;
		SrsFMLEStartPacket* pkt = NULL;
		if ((ret = srs_rtmp_expect_message<SrsFMLEStartPacket>(protocol, &msg, &pkt)) != ERROR_SUCCESS) {
			srs_error("recv FCPublish message failed. ret=%d", ret);
			return ret;
		}
		srs_info("recv FCPublish request message success.");
		
		SrsAutoFree(SrsCommonMessage, msg, false);
		fc_publish_tid = pkt->transaction_id;
	}
	// FCPublish response
	if (true) {
		SrsCommonMessage* msg = new SrsCommonMessage();
		SrsFMLEStartResPacket* pkt = new SrsFMLEStartResPacket(fc_publish_tid);
		
		msg->set_packet(pkt, 0);
		
		if ((ret = protocol->send_message(msg)) != ERROR_SUCCESS) {
			srs_error("send FCPublish response message failed. ret=%d", ret);
			return ret;
		}
		srs_info("send FCPublish response message success.");
	}
	
	// createStream
	double create_stream_tid = 0;
	if (true) {
		SrsCommonMessage* msg = NULL;
		SrsCreateStreamPacket* pkt = NULL;
		if ((ret = srs_rtmp_expect_message<SrsCreateStreamPacket>(protocol, &msg, &pkt)) != ERROR_SUCCESS) {
			srs_error("recv createStream message failed. ret=%d", ret);
			return ret;
		}
		srs_info("recv createStream request message success.");
		
		SrsAutoFree(SrsCommonMessage, msg, false);
		create_stream_tid = pkt->transaction_id;
	}
	// createStream response
	if (true) {
		SrsCommonMessage* msg = new SrsCommonMessage();
		SrsCreateStreamResPacket* pkt = new SrsCreateStreamResPacket(create_stream_tid, stream_id);
		
		msg->set_packet(pkt, 0);
		
		if ((ret = protocol->send_message(msg)) != ERROR_SUCCESS) {
			srs_error("send createStream response message failed. ret=%d", ret);
			return ret;
		}
		srs_info("send createStream response message success.");
	}
	
	// publish
	if (true) {
		SrsCommonMessage* msg = NULL;
		SrsPublishPacket* pkt = NULL;
		if ((ret = srs_rtmp_expect_message<SrsPublishPacket>(protocol, &msg, &pkt)) != ERROR_SUCCESS) {
			srs_error("recv publish message failed. ret=%d", ret);
			return ret;
		}
		srs_info("recv publish request message success.");
		
		SrsAutoFree(SrsCommonMessage, msg, false);
	}
	// publish response onFCPublish(NetStream.Publish.Start)
	if (true) {
		SrsCommonMessage* msg = new SrsCommonMessage();
		SrsOnStatusCallPacket* pkt = new SrsOnStatusCallPacket();
		
		pkt->command_name = RTMP_AMF0_COMMAND_ON_FC_PUBLISH;
		pkt->data->set(StatusCode, new SrsAmf0String(StatusCodePublishStart));
		pkt->data->set(StatusDescription, new SrsAmf0String("Started publishing stream."));
		
		msg->set_packet(pkt, stream_id);
		
		if ((ret = protocol->send_message(msg)) != ERROR_SUCCESS) {
			srs_error("send onFCPublish(NetStream.Publish.Start) message failed. ret=%d", ret);
			return ret;
		}
		srs_info("send onFCPublish(NetStream.Publish.Start) message success.");
	}
	// publish response onStatus(NetStream.Publish.Start)
	if (true) {
		SrsCommonMessage* msg = new SrsCommonMessage();
		SrsOnStatusCallPacket* pkt = new SrsOnStatusCallPacket();
		
		pkt->data->set(StatusLevel, new SrsAmf0String(StatusLevelStatus));
		pkt->data->set(StatusCode, new SrsAmf0String(StatusCodePublishStart));
		pkt->data->set(StatusDescription, new SrsAmf0String("Started publishing stream."));
		pkt->data->set(StatusClientId, new SrsAmf0String(RTMP_SIG_CLIENT_ID));
		
		msg->set_packet(pkt, stream_id);
		
		if ((ret = protocol->send_message(msg)) != ERROR_SUCCESS) {
			srs_error("send onStatus(NetStream.Publish.Start) message failed. ret=%d", ret);
			return ret;
		}
		srs_info("send onStatus(NetStream.Publish.Start) message success.");
	}
	
	srs_info("FMLE publish success.");
	
	return ret;
}

int SrsRtmp::fmle_unpublish(int stream_id, double unpublish_tid)
{
	int ret = ERROR_SUCCESS;
	
	// publish response onFCUnpublish(NetStream.unpublish.Success)
	if (true) {
		SrsCommonMessage* msg = new SrsCommonMessage();
		SrsOnStatusCallPacket* pkt = new SrsOnStatusCallPacket();
		
		pkt->command_name = RTMP_AMF0_COMMAND_ON_FC_UNPUBLISH;
		pkt->data->set(StatusCode, new SrsAmf0String(StatusCodeUnpublishSuccess));
		pkt->data->set(StatusDescription, new SrsAmf0String("Stop publishing stream."));
		
		msg->set_packet(pkt, stream_id);
		
		if ((ret = protocol->send_message(msg)) != ERROR_SUCCESS) {
			srs_error("send onFCUnpublish(NetStream.unpublish.Success) message failed. ret=%d", ret);
			return ret;
		}
		srs_info("send onFCUnpublish(NetStream.unpublish.Success) message success.");
	}
	// FCUnpublish response
	if (true) {
		SrsCommonMessage* msg = new SrsCommonMessage();
		SrsFMLEStartResPacket* pkt = new SrsFMLEStartResPacket(unpublish_tid);
		
		msg->set_packet(pkt, stream_id);
		
		if ((ret = protocol->send_message(msg)) != ERROR_SUCCESS) {
			srs_error("send FCUnpublish response message failed. ret=%d", ret);
			return ret;
		}
		srs_info("send FCUnpublish response message success.");
	}
	// publish response onStatus(NetStream.Unpublish.Success)
	if (true) {
		SrsCommonMessage* msg = new SrsCommonMessage();
		SrsOnStatusCallPacket* pkt = new SrsOnStatusCallPacket();
		
		pkt->data->set(StatusLevel, new SrsAmf0String(StatusLevelStatus));
		pkt->data->set(StatusCode, new SrsAmf0String(StatusCodeUnpublishSuccess));
		pkt->data->set(StatusDescription, new SrsAmf0String("Stream is now unpublished"));
		pkt->data->set(StatusClientId, new SrsAmf0String(RTMP_SIG_CLIENT_ID));
		
		msg->set_packet(pkt, stream_id);
		
		if ((ret = protocol->send_message(msg)) != ERROR_SUCCESS) {
			srs_error("send onStatus(NetStream.Unpublish.Success) message failed. ret=%d", ret);
			return ret;
		}
		srs_info("send onStatus(NetStream.Unpublish.Success) message success.");
	}
	
	srs_info("FMLE unpublish success.");
	
	return ret;
}

int SrsRtmp::start_flash_publish(int stream_id)
{
	int ret = ERROR_SUCCESS;
	
	// publish response onStatus(NetStream.Publish.Start)
	if (true) {
		SrsCommonMessage* msg = new SrsCommonMessage();
		SrsOnStatusCallPacket* pkt = new SrsOnStatusCallPacket();
		
		pkt->data->set(StatusLevel, new SrsAmf0String(StatusLevelStatus));
		pkt->data->set(StatusCode, new SrsAmf0String(StatusCodePublishStart));
		pkt->data->set(StatusDescription, new SrsAmf0String("Started publishing stream."));
		pkt->data->set(StatusClientId, new SrsAmf0String(RTMP_SIG_CLIENT_ID));
		
		msg->set_packet(pkt, stream_id);
		
		if ((ret = protocol->send_message(msg)) != ERROR_SUCCESS) {
			srs_error("send onStatus(NetStream.Publish.Start) message failed. ret=%d", ret);
			return ret;
		}
		srs_info("send onStatus(NetStream.Publish.Start) message success.");
	}
	
	srs_info("flash publish success.");
	
	return ret;
}

int SrsRtmp::identify_create_stream_client(SrsCreateStreamPacket* req, int stream_id, SrsClientType& type, string& stream_name)
{
	int ret = ERROR_SUCCESS;
	
	if (true) {
		SrsCommonMessage* msg = new SrsCommonMessage();
		SrsCreateStreamResPacket* pkt = new SrsCreateStreamResPacket(req->transaction_id, stream_id);
		
		msg->set_packet(pkt, 0);
		
		if ((ret = protocol->send_message(msg)) != ERROR_SUCCESS) {
			srs_error("send createStream response message failed. ret=%d", ret);
			return ret;
		}
		srs_info("send createStream response message success.");
	}
	
	while (true) {
		SrsCommonMessage* msg = NULL;
		if ((ret = protocol->recv_message(&msg)) != ERROR_SUCCESS) {
			srs_error("recv identify client message failed. ret=%d", ret);
			return ret;
		}

		SrsAutoFree(SrsCommonMessage, msg, false);

		if (!msg->header.is_amf0_command() && !msg->header.is_amf3_command()) {
			srs_trace("identify ignore messages except "
				"AMF0/AMF3 command message. type=%#x", msg->header.message_type);
			continue;
		}
		
		if ((ret = msg->decode_packet(protocol)) != ERROR_SUCCESS) {
			srs_error("identify decode message failed. ret=%d", ret);
			return ret;
		}
		
		SrsPacket* pkt = msg->get_packet();
		if (dynamic_cast<SrsPlayPacket*>(pkt)) {
			srs_info("level1 identify client by play.");
			return identify_play_client(dynamic_cast<SrsPlayPacket*>(pkt), type, stream_name);
		}
		if (dynamic_cast<SrsPublishPacket*>(pkt)) {
			srs_info("identify client by publish, falsh publish.");
			return identify_flash_publish_client(dynamic_cast<SrsPublishPacket*>(pkt), type, stream_name);
		}
		
		srs_trace("ignore AMF0/AMF3 command message.");
	}
	
	return ret;
}

int SrsRtmp::identify_fmle_publish_client(SrsFMLEStartPacket* req, SrsClientType& type, string& stream_name)
{
	int ret = ERROR_SUCCESS;
	
	type = SrsClientFMLEPublish;
	stream_name = req->stream_name;
	
	// releaseStream response
	if (true) {
		SrsCommonMessage* msg = new SrsCommonMessage();
		SrsFMLEStartResPacket* pkt = new SrsFMLEStartResPacket(req->transaction_id);
		
		msg->set_packet(pkt, 0);
		
		if ((ret = protocol->send_message(msg)) != ERROR_SUCCESS) {
			srs_error("send releaseStream response message failed. ret=%d", ret);
			return ret;
		}
		srs_info("send releaseStream response message success.");
	}
	
	return ret;
}

int SrsRtmp::identify_flash_publish_client(SrsPublishPacket* req, SrsClientType& type, string& stream_name)
{
	int ret = ERROR_SUCCESS;
	
	type = SrsClientFlashPublish;
	stream_name = req->stream_name;
	
	return ret;
}

int SrsRtmp::identify_play_client(SrsPlayPacket* req, SrsClientType& type, string& stream_name)
{
	int ret = ERROR_SUCCESS;
	
	type = SrsClientPlay;
	stream_name = req->stream_name;
	
	srs_trace("identity client type=play, stream_name=%s", stream_name.c_str());

	return ret;
}

