/*
The MIT License (MIT)

Copyright (c) 2013 winlin

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

#include <srs_core_protocol.hpp>

#include <srs_core_log.hpp>
#include <srs_core_error.hpp>
#include <srs_core_socket.hpp>
#include <srs_core_buffer.hpp>

/**
* 6.1.2. Chunk Message Header
* There are four different formats for the chunk message header,
* selected by the "fmt" field in the chunk basic header.
*/
// 6.1.2.1. Type 0
// Chunks of Type 0 are 11 bytes long. This type MUST be used at the
// start of a chunk stream, and whenever the stream timestamp goes
// backward (e.g., because of a backward seek).
#define RTMP_FMT_TYPE0 0
// 6.1.2.2. Type 1
// Chunks of Type 1 are 7 bytes long. The message stream ID is not
// included; this chunk takes the same stream ID as the preceding chunk.
// Streams with variable-sized messages (for example, many video
// formats) SHOULD use this format for the first chunk of each new
// message after the first.
#define RTMP_FMT_TYPE1 1
// 6.1.2.3. Type 2
// Chunks of Type 2 are 3 bytes long. Neither the stream ID nor the
// message length is included; this chunk has the same stream ID and
// message length as the preceding chunk. Streams with constant-sized
// messages (for example, some audio and data formats) SHOULD use this
// format for the first chunk of each message after the first.
#define RTMP_FMT_TYPE2 2
// 6.1.2.4. Type 3
// Chunks of Type 3 have no header. Stream ID, message length and
// timestamp delta are not present; chunks of this type take values from
// the preceding chunk. When a single message is split into chunks, all
// chunks of a message except the first one, SHOULD use this type. Refer
// to example 2 in section 6.2.2. Stream consisting of messages of
// exactly the same size, stream ID and spacing in time SHOULD use this
// type for all chunks after chunk of Type 2. Refer to example 1 in
// section 6.2.1. If the delta between the first message and the second
// message is same as the time stamp of first message, then chunk of
// type 3 would immediately follow the chunk of type 0 as there is no
// need for a chunk of type 2 to register the delta. If Type 3 chunk
// follows a Type 0 chunk, then timestamp delta for this Type 3 chunk is
// the same as the timestamp of Type 0 chunk.
#define RTMP_FMT_TYPE3 3

/**
* 6. Chunking
* The chunk size is configurable. It can be set using a control
* message(Set Chunk Size) as described in section 7.1. The maximum
* chunk size can be 65536 bytes and minimum 128 bytes. Larger values
* reduce CPU usage, but also commit to larger writes that can delay
* other content on lower bandwidth connections. Smaller chunks are not
* good for high-bit rate streaming. Chunk size is maintained
* independently for each direction.
*/
#define RTMP_DEFAULT_CHUNK_SIZE 128

/**
* 6.1. Chunk Format
* Extended timestamp: 0 or 4 bytes
* This field MUST be sent when the normal timsestamp is set to
* 0xffffff, it MUST NOT be sent if the normal timestamp is set to
* anything else. So for values less than 0xffffff the normal
* timestamp field SHOULD be used in which case the extended timestamp
* MUST NOT be present. For values greater than or equal to 0xffffff
* the normal timestamp field MUST NOT be used and MUST be set to
* 0xffffff and the extended timestamp MUST be sent.
*/
#define RTMP_EXTENDED_TIMESTAMP 0xFFFFFF

SrsProtocol::SrsProtocol(st_netfd_t client_stfd)
{
	stfd = client_stfd;
	buffer = new SrsBuffer();
	skt = new SrsSocket(stfd);
	
	in_chunk_size = out_chunk_size = RTMP_DEFAULT_CHUNK_SIZE;
}

SrsProtocol::~SrsProtocol()
{
	std::map<int, SrsChunkStream*>::iterator it;
	
	for (it = chunk_streams.begin(); it != chunk_streams.end(); ++it) {
		SrsChunkStream* stream = it->second;
		
		if (stream) {
			delete stream;
		}
	}

	chunk_streams.clear();
	
	if (buffer) {
		delete buffer;
		buffer = NULL;
	}
	
	if (skt) {
		delete skt;
		skt = NULL;
	}
}

int SrsProtocol::recv_message(SrsMessage** pmsg)
{
	int ret = ERROR_SUCCESS;
	
	while (true) {
		SrsMessage* msg = NULL;
		
		if ((ret = recv_interlaced_message(&msg)) != ERROR_SUCCESS) {
			srs_error("recv interlaced message failed. ret=%d", ret);
			return ret;
		}
		
		if (!msg) {
			continue;
		}
		
		// decode the msg
	}
	
	return ret;
}

int SrsProtocol::recv_interlaced_message(SrsMessage** pmsg)
{
	int ret = ERROR_SUCCESS;
	
	// chunk stream basic header.
	char fmt = 0;
	int cid = 0;
	int bh_size = 0;
	if ((ret = read_basic_header(fmt, cid, bh_size)) != ERROR_SUCCESS) {
		srs_error("read basic header failed. ret=%d", ret);
		return ret;
	}
	srs_info("read basic header success. fmt=%d, cid=%d, bh_size=%d", fmt, cid, bh_size);
	
	// get the cached chunk stream.
	SrsChunkStream* chunk = NULL;
	
	if (chunk_streams.find(cid) == chunk_streams.end()) {
		chunk = chunk_streams[cid] = new SrsChunkStream(cid);
		srs_info("cache new chunk stream: fmt=%d, cid=%d", fmt, cid);
	} else {
		chunk = chunk_streams[cid];
		srs_info("cached chunk stream: fmt=%d, cid=%d, size=%d, message(type=%d, size=%d, time=%d, sid=%d)",
			chunk->fmt, chunk->cid, (chunk->msg? chunk->msg->size : 0), chunk->header.message_type, chunk->header.payload_length,
			chunk->header.timestamp, chunk->header.stream_id);
	}

	// chunk stream message header
	int mh_size = 0;
	if ((ret = read_message_header(chunk, fmt, bh_size, mh_size)) != ERROR_SUCCESS) {
		srs_error("read message header failed. ret=%d", ret);
		return ret;
	}
	srs_info("read message header success. "
			"fmt=%d, mh_size=%d, ext_time=%d, size=%d, message(type=%d, size=%d, time=%d, sid=%d)", 
			fmt, mh_size, chunk->extended_timestamp, (chunk->msg? chunk->msg->size : 0), chunk->header.message_type, 
			chunk->header.payload_length, chunk->header.timestamp, chunk->header.stream_id);
	
	// read msg payload from chunk stream.
	SrsMessage* msg = NULL;
	int payload_size = 0;
	if ((ret = read_message_payload(chunk, bh_size, mh_size, payload_size, &msg)) != ERROR_SUCCESS) {
		srs_error("read message payload failed. ret=%d", ret);
		return ret;
	}
	
	// not got an entire RTMP message, try next chunk.
	if (!msg) {
		srs_info("get partial message success. chunk_payload_size=%d, size=%d, message(type=%d, size=%d, time=%d, sid=%d)",
				payload_size, (msg? msg->size : (chunk->msg? chunk->msg->size : 0)), chunk->header.message_type, chunk->header.payload_length,
				chunk->header.timestamp, chunk->header.stream_id);
		return ret;
	}
	
	*pmsg = msg;
	srs_info("get entire message success. chunk_payload_size=%d, size=%d, message(type=%d, size=%d, time=%d, sid=%d)",
			payload_size, (msg? msg->size : (chunk->msg? chunk->msg->size : 0)), chunk->header.message_type, chunk->header.payload_length,
			chunk->header.timestamp, chunk->header.stream_id);
			
	return ret;
}

int SrsProtocol::read_basic_header(char& fmt, int& cid, int& size)
{
	int ret = ERROR_SUCCESS;
	
	int required_size = 1;
	if ((ret = buffer->ensure_buffer_bytes(skt, required_size)) != ERROR_SUCCESS) {
		srs_error("read 1bytes basic header failed. required_size=%d, ret=%d", required_size, ret);
		return ret;
	}
	
	char* p = buffer->bytes();
	
    fmt = (*p >> 6) & 0x03;
    cid = *p & 0x3f;
    size = 1;
    
    if (cid > 1) {
		srs_verbose("%dbytes basic header parsed. fmt=%d, cid=%d", size, fmt, cid);
        return ret;
    }

	if (cid == 0) {
		required_size = 2;
		if ((ret = buffer->ensure_buffer_bytes(skt, required_size)) != ERROR_SUCCESS) {
			srs_error("read 2bytes basic header failed. required_size=%d, ret=%d", required_size, ret);
			return ret;
		}
		
		cid = 64;
		cid += *(++p);
    	size = 2;
		srs_verbose("%dbytes basic header parsed. fmt=%d, cid=%d", size, fmt, cid);
	} else if (cid == 1) {
		required_size = 3;
		if ((ret = buffer->ensure_buffer_bytes(skt, 3)) != ERROR_SUCCESS) {
			srs_error("read 3bytes basic header failed. required_size=%d, ret=%d", required_size, ret);
			return ret;
		}
		
		cid = 64;
		cid += *(++p);
		cid += *(++p) * 256;
    	size = 3;
		srs_verbose("%dbytes basic header parsed. fmt=%d, cid=%d", size, fmt, cid);
	} else {
		srs_error("invalid path, impossible basic header.");
		srs_assert(false);
	}
	
	return ret;
}

int SrsProtocol::read_message_header(SrsChunkStream* chunk, char fmt, int bh_size, int& mh_size)
{
	int ret = ERROR_SUCCESS;
	
	// when not exists cached msg, means get an new message,
	// the fmt must be type0 which means new message.
	if (!chunk->msg && fmt != RTMP_FMT_TYPE0) {
		ret = ERROR_RTMP_CHUNK_START;
		srs_error("chunk stream start, "
			"fmt must be %d, actual is %d. ret=%d", RTMP_FMT_TYPE0, fmt, ret);
		return ret;
	}

	// when exists cache msg, means got an partial message,
	// the fmt must not be type0 which means new message.
	if (chunk->msg && fmt == RTMP_FMT_TYPE0) {
		ret = ERROR_RTMP_CHUNK_START;
		srs_error("chunk stream exists, "
			"fmt must not be %d, actual is %d. ret=%d", RTMP_FMT_TYPE0, fmt, ret);
		return ret;
	}
	
	// create msg when new chunk stream start
	if (!chunk->msg) {
		srs_assert(fmt == RTMP_FMT_TYPE0);
		chunk->msg = new SrsMessage();
		srs_verbose("create message for new chunk, fmt=%d, cid=%d", fmt, chunk->cid);
	}

	// read message header from socket to buffer.
	static char mh_sizes[] = {11, 7, 1, 0};
	mh_size = mh_sizes[(int)fmt];
	srs_verbose("calc chunk message header size. fmt=%d, mh_size=%d", fmt, mh_size);
	
	int required_size = bh_size + mh_size;
	if ((ret = buffer->ensure_buffer_bytes(skt, required_size)) != ERROR_SUCCESS) {
		srs_error("read %dbytes message header failed. required_size=%d, ret=%d", mh_size, required_size, ret);
		return ret;
	}
	char* p = buffer->bytes() + bh_size;
	
	// parse the message header.
	// see also: ngx_rtmp_recv
	if (fmt <= RTMP_FMT_TYPE2) {
		int32_t timestamp_delta;
        char* pp = (char*)&timestamp_delta;
        pp[2] = *p++;
        pp[1] = *p++;
        pp[0] = *p++;
        pp[3] = 0;
        
        if (fmt == RTMP_FMT_TYPE0) {
			// 6.1.2.1. Type 0
			// For a type-0 chunk, the absolute timestamp of the message is sent
			// here.
            chunk->header.timestamp = timestamp_delta;
        } else {
            // 6.1.2.2. Type 1
            // 6.1.2.3. Type 2
            // For a type-1 or type-2 chunk, the difference between the previous
            // chunk's timestamp and the current chunk's timestamp is sent here.
            chunk->header.timestamp += timestamp_delta;
        }
        
        // fmt: 0
        // timestamp: 3 bytes
        // If the timestamp is greater than or equal to 16777215
        // (hexadecimal 0x00ffffff), this value MUST be 16777215, and the
        // ‘extended timestamp header’ MUST be present. Otherwise, this value
        // SHOULD be the entire timestamp.
        //
        // fmt: 1 or 2
        // timestamp delta: 3 bytes
        // If the delta is greater than or equal to 16777215 (hexadecimal
        // 0x00ffffff), this value MUST be 16777215, and the ‘extended
        // timestamp header’ MUST be present. Otherwise, this value SHOULD be
        // the entire delta.
        chunk->extended_timestamp = (timestamp_delta >= RTMP_EXTENDED_TIMESTAMP);
        if (chunk->extended_timestamp) {
			chunk->header.timestamp = RTMP_EXTENDED_TIMESTAMP;
        }
        
        if (fmt <= RTMP_FMT_TYPE1) {
            pp = (char*)&chunk->header.payload_length;
            pp[2] = *p++;
            pp[1] = *p++;
            pp[0] = *p++;
            pp[3] = 0;
            
            chunk->header.message_type = *p++;
            
            if (fmt == 0) {
                pp = (char*)&chunk->header.stream_id;
                pp[0] = *p++;
                pp[1] = *p++;
                pp[2] = *p++;
                pp[3] = *p++;
				srs_verbose("header read completed. fmt=%d, mh_size=%d, ext_time=%d, time=%d, payload=%d, type=%d, sid=%d", 
					fmt, mh_size, chunk->extended_timestamp, chunk->header.timestamp, chunk->header.payload_length, 
					chunk->header.message_type, chunk->header.stream_id);
			} else {
				srs_verbose("header read completed. fmt=%d, mh_size=%d, ext_time=%d, time=%d, payload=%d, type=%d", 
					fmt, mh_size, chunk->extended_timestamp, chunk->header.timestamp, chunk->header.payload_length, 
					chunk->header.message_type);
			}
		} else {
			srs_verbose("header read completed. fmt=%d, mh_size=%d, ext_time=%d, time=%d", 
				fmt, mh_size, chunk->extended_timestamp, chunk->header.timestamp);
		}
	} else {
		srs_verbose("header read completed. fmt=%d, size=%d, ext_time=%d", 
			fmt, mh_size, chunk->extended_timestamp);
	}
	
	if (chunk->extended_timestamp) {
		mh_size += 4;
		required_size = bh_size + mh_size;
		srs_verbose("read header ext time. fmt=%d, ext_time=%d, mh_size=%d", fmt, chunk->extended_timestamp, mh_size);
		if ((ret = buffer->ensure_buffer_bytes(skt, required_size)) != ERROR_SUCCESS) {
			srs_error("read %dbytes message header failed. required_size=%d, ret=%d", mh_size, required_size, ret);
			return ret;
		}

        char* pp = (char*)&chunk->header.timestamp;
        pp[3] = *p++;
        pp[2] = *p++;
        pp[1] = *p++;
        pp[0] = *p++;
		srs_verbose("header read ext_time completed. time=%d", chunk->header.timestamp);
	}
	
	// valid message
	if (chunk->header.payload_length < 0) {
		ret = ERROR_RTMP_MSG_INVLIAD_SIZE;
		srs_error("RTMP message size must not be negative. size=%d, ret=%d", 
			chunk->header.payload_length, ret);
		return ret;
	}
	
	// copy header to msg
	chunk->msg->header = chunk->header;
	
	return ret;
}

int SrsProtocol::read_message_payload(SrsChunkStream* chunk, int bh_size, int mh_size, int& payload_size, SrsMessage** pmsg)
{
	int ret = ERROR_SUCCESS;
	
	// empty message
	if (chunk->header.payload_length == 0) {
		// need erase the header in buffer.
		buffer->erase(bh_size + mh_size);
		
		srs_warn("get an empty RTMP "
				"message(type=%d, size=%d, time=%d, sid=%d)", chunk->header.message_type, 
				chunk->header.payload_length, chunk->header.timestamp, chunk->header.stream_id);
				
		return ret;
	}
	srs_assert(chunk->header.payload_length > 0);
	
	// the chunk payload size.
	payload_size = chunk->header.payload_length - chunk->msg->size;
	if (payload_size > in_chunk_size) {
		payload_size = in_chunk_size;
	}
	srs_verbose("chunk payload size is %d, message_size=%d, received_size=%d, in_chunk_size=%d", 
		payload_size, chunk->header.payload_length, chunk->msg->size, in_chunk_size);

	// create msg payload if not initialized
	if (!chunk->msg->payload) {
		chunk->msg->payload = new int8_t[chunk->header.payload_length];
		memset(chunk->msg->payload, 0, chunk->header.payload_length);
		srs_verbose("create empty payload for RTMP message. size=%d", chunk->header.payload_length);
	}
	
	// copy payload from buffer.
	int copy_size = buffer->size() - bh_size - mh_size;
	if (copy_size > payload_size) {
		copy_size = payload_size;
	}
	memcpy(chunk->msg->payload + chunk->msg->size, buffer->bytes() + bh_size + mh_size, copy_size);
	buffer->erase(bh_size + mh_size + copy_size);
	chunk->msg->size += copy_size;
	
	// when empty, read the left bytes from socket.
	int left_size = payload_size - copy_size;
	if (left_size > 0) {
		ssize_t nread;
		if ((ret = skt->read_fully(chunk->msg->payload + chunk->msg->size, left_size, &nread)) != ERROR_SUCCESS) {
			srs_error("read chunk payload from socket error. "
				"payload_size=%d, copy_size=%d, left_size=%d, size=%d, msg_size=%d, ret=%d",
				payload_size, copy_size, left_size, chunk->msg->size, chunk->header.payload_length, ret);
			return ret;
		}
	}
	
	srs_verbose("chunk payload read complted. bh_size=%d, mh_size=%d, payload_size=%d", bh_size, mh_size, payload_size);
	
	// got entire RTMP message?
	if (chunk->header.payload_length == chunk->msg->size) {
		*pmsg = chunk->msg;
		chunk->msg = NULL;
		srs_verbose("get entire RTMP message(type=%d, size=%d, time=%d, sid=%d)", 
				chunk->header.message_type, chunk->header.payload_length, 
				chunk->header.timestamp, chunk->header.stream_id);
		return ret;
	}
	
	srs_verbose("get partial RTMP message(type=%d, size=%d, time=%d, sid=%d), partial size=%d", 
			chunk->header.message_type, chunk->header.payload_length, 
			chunk->header.timestamp, chunk->header.stream_id,
			chunk->msg->size);
	
	return ret;
}

SrsMessageHeader::SrsMessageHeader()
{
	message_type = 0;
	payload_length = 0;
	timestamp = 0;
	stream_id = 0;
}

SrsMessageHeader::~SrsMessageHeader()
{
}

SrsChunkStream::SrsChunkStream(int _cid)
{
	fmt = 0;
	cid = _cid;
	extended_timestamp = false;
	msg = NULL;
}

SrsChunkStream::~SrsChunkStream()
{
	if (msg) {
		delete msg;
		msg = NULL;
	}
}

SrsMessage::SrsMessage()
{
	size = 0;
	payload = NULL;
}

SrsMessage::~SrsMessage()
{
	if (payload) {
		delete[] payload;
		payload = NULL;
	}
}

