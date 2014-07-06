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

#ifndef SRS_RTMP_PROTOCOL_RTMP_STACK_HPP
#define SRS_RTMP_PROTOCOL_RTMP_STACK_HPP

/*
#include <srs_protocol_rtmp_stack.hpp>
*/

#include <srs_core.hpp>

#include <map>
#include <string>

#include <srs_kernel_log.hpp>
#include <srs_kernel_error.hpp>

class ISrsProtocolReaderWriter;
class SrsBuffer;
class SrsPacket;
class SrsStream;
class SrsAmf0Object;
class SrsAmf0Any;
class SrsMessageHeader;
class SrsMessage;
class SrsChunkStream;
 
// the following is the timeout for rtmp protocol, 
// to avoid death connection.

// the timeout to wait client data,
// if timeout, close the connection.
#define SRS_SEND_TIMEOUT_US (int64_t)(30*1000*1000LL)

// the timeout to send data to client,
// if timeout, close the connection.
#define SRS_RECV_TIMEOUT_US (int64_t)(30*1000*1000LL)

// the timeout to wait for client control message,
// if timeout, we generally ignore and send the data to client,
// generally, it's the pulse time for data seding.
#define SRS_PULSE_TIMEOUT_US (int64_t)(200*1000LL)

/**
* max rtmp header size:
*     1bytes basic header,
*     11bytes message header,
*     4bytes timestamp header,
* that is, 1+11+4=16bytes.
*/
#define RTMP_MAX_FMT0_HEADER_SIZE 16
/**
* max rtmp header size:
*     1bytes basic header,
*     4bytes timestamp header,
* that is, 1+4=5bytes.
*/
// always use fmt0 as cache.
//#define RTMP_MAX_FMT3_HEADER_SIZE 5

/**
* the protocol provides the rtmp-message-protocol services,
* to recv RTMP message from RTMP chunk stream,
* and to send out RTMP message over RTMP chunk stream.
*/
class SrsProtocol
{
private:
    class AckWindowSize
    {
    public:
        int ack_window_size;
        int64_t acked_size;
        
        AckWindowSize();
    };
// peer in/out
private:
    /**
    * underlayer socket object, send/recv bytes.
    */
    ISrsProtocolReaderWriter* skt;
    /**
    * requests sent out, used to build the response.
    * key: transactionId
    * value: the request command name
    */
    std::map<double, std::string> requests;
// peer in
private:
    /**
    * chunk stream to decode RTMP messages.
    */
    std::map<int, SrsChunkStream*> chunk_streams;
    /**
    * bytes buffer cache, recv from skt, provide services for stream.
    */
    SrsBuffer* in_buffer;
    /**
    * input chunk size, default to 128, set by peer packet.
    */
    int32_t in_chunk_size;
    /**
    * input ack size, when to send the acked packet.
    */
    AckWindowSize in_ack_size;
// peer out
private:
    /**
    * output header cache.
    * used for type0, 11bytes(or 15bytes with extended timestamp) header.
    * or for type3, 1bytes(or 5bytes with extended timestamp) header.
    */
    char out_header_cache[RTMP_MAX_FMT0_HEADER_SIZE];
    /**
    * output chunk size, default to 128, set by config.
    */
    int32_t out_chunk_size;
public:
    /**
    * use io to create the protocol stack,
    * @param io, provides io interfaces, user must free it.
    */
    SrsProtocol(ISrsProtocolReaderWriter* io);
    virtual ~SrsProtocol();
public:
    /**
    * set/get the recv timeout in us.
    * if timeout, recv/send message return ERROR_SOCKET_TIMEOUT.
    */
    virtual void set_recv_timeout(int64_t timeout_us);
    virtual int64_t get_recv_timeout();
    /**
    * set/get the send timeout in us.
    * if timeout, recv/send message return ERROR_SOCKET_TIMEOUT.
    */
    virtual void set_send_timeout(int64_t timeout_us);
    virtual int64_t get_send_timeout();
    /**
    * get recv/send bytes.
    */
    virtual int64_t get_recv_bytes();
    virtual int64_t get_send_bytes();
public:
    /**
    * recv a RTMP message, which is bytes oriented.
    * user can use decode_message to get the decoded RTMP packet.
    * @param pmsg, set the received message, 
    *       always NULL if error, 
    *       NULL for unknown packet but return success.
    *       never NULL if decode success.
    */
    virtual int recv_message(SrsMessage** pmsg);
    /**
    * decode bytes oriented RTMP message to RTMP packet,
    * @param ppacket, output decoded packet, 
    *       always NULL if error, never NULL if success.
    * @return error when unknown packet, error when decode failed.
    */
    virtual int decode_message(SrsMessage* msg, SrsPacket** ppacket);
    /**
    * send the RTMP message and always free it.
    * user must never free or use the msg after this method,
    * for it will always free the msg.
    * @param msg, the msg to send out, never be NULL.
    * @param stream_id, the stream id of packet to send over, 0 for control message.
    */
    virtual int send_and_free_message(SrsMessage* msg, int stream_id);
    /**
    * send the RTMP packet and always free it.
    * user must never free or use the packet after this method,
    * for it will always free the packet.
    * @param packet, the packet to send out, never be NULL.
    * @param stream_id, the stream id of packet to send over, 0 for control message.
    */
    virtual int send_and_free_packet(SrsPacket* packet, int stream_id);
private:
    /**
    * send out the message, donot free it, the caller must free the param msg.
    * @param packet the packet of message, NULL for raw message.
    */
    virtual int do_send_message(SrsMessage* msg, SrsPacket* packet);
    /**
    * imp for decode_message
    */
    virtual int do_decode_message(SrsMessageHeader& header, SrsStream* stream, SrsPacket** ppacket);
    /**
    * recv bytes oriented RTMP message from protocol stack.
    * return error if error occur and nerver set the pmsg,
    * return success and pmsg set to NULL if no entire message got,
    * return success and pmsg set to entire message if got one.
    */
    virtual int recv_interlaced_message(SrsMessage** pmsg);
    /**
    * read the chunk basic header(fmt, cid) from chunk stream.
    * user can discovery a SrsChunkStream by cid.
    * @bh_size return the chunk basic header size, to remove the used bytes when finished.
    */
    virtual int read_basic_header(char& fmt, int& cid, int& bh_size);
    /**
    * read the chunk message header(timestamp, payload_length, message_type, stream_id) 
    * from chunk stream and save to SrsChunkStream.
    * @mh_size return the chunk message header size, to remove the used bytes when finished.
    */
    virtual int read_message_header(SrsChunkStream* chunk, char fmt, int bh_size, int& mh_size);
    /**
    * read the chunk payload, remove the used bytes in buffer,
    * if got entire message, set the pmsg.
    * @payload_size read size in this roundtrip, generally a chunk size or left message size.
    */
    virtual int read_message_payload(SrsChunkStream* chunk, int bh_size, int mh_size, int& payload_size, SrsMessage** pmsg);
    /**
    * when recv message, update the context.
    */
    virtual int on_recv_message(SrsMessage* msg);
    /**
    * when message sentout, update the context.
    */
    virtual int on_send_message(SrsMessage* msg, SrsPacket* packet);
private:
    /**
    * auto response the ack message.
    */
    virtual int response_acknowledgement_message();
    /**
    * auto response the ping message.
    */
    virtual int response_ping_message(int32_t timestamp);
};

/**
* 4.1. Message Header
*/
class SrsMessageHeader
{
public:
    /**
    * One byte field to represent the message type. A range of type IDs
    * (1-7) are reserved for protocol control messages.
    */
    int8_t message_type;
    /**
    * Three-byte field that represents the size of the payload in bytes.
    * It is set in big-endian format.
    */
    int32_t payload_length;
    /**
    * Three-byte field that contains a timestamp delta of the message.
    * The 4 bytes are packed in the big-endian order.
    * @remark, only used for decoding message from chunk stream.
    */
    int32_t timestamp_delta;
    /**
    * Three-byte field that identifies the stream of the message. These
    * bytes are set in big-endian format.
    */
    int32_t stream_id;
    
    /**
    * Four-byte field that contains a timestamp of the message.
    * The 4 bytes are packed in the big-endian order.
    * @remark, used as calc timestamp when decode and encode time.
    * @remark, we use 64bits for large time for jitter detect and hls.
    */
    int64_t timestamp;
public:
    /**
    * get the perfered cid(chunk stream id) which sendout over.
    * set at decoding, and canbe used for directly send message,
    * for example, dispatch to all connections.
    */
    int perfer_cid;
public:
    SrsMessageHeader();
    virtual ~SrsMessageHeader();
public:
    bool is_audio();
    bool is_video();
    bool is_amf0_command();
    bool is_amf0_data();
    bool is_amf3_command();
    bool is_amf3_data();
    bool is_window_ackledgement_size();
    bool is_ackledgement();
    bool is_set_chunk_size();
    bool is_user_control_message();
    bool is_set_peer_bandwidth();
    bool is_aggregate();
public:
    /**
    * create a amf0 script header, set the size and stream_id.
    */
    void initialize_amf0_script(int size, int stream);
    /**
    * create a audio header, set the size, timestamp and stream_id.
    */
    void initialize_audio(int size, u_int32_t time, int stream);
    /**
    * create a video header, set the size, timestamp and stream_id.
    */
    void initialize_video(int size, u_int32_t time, int stream);
};

/**
* incoming chunk stream maybe interlaced,
* use the chunk stream to cache the input RTMP chunk streams.
*/
class SrsChunkStream
{
public:
    /**
    * represents the basic header fmt,
    * which used to identify the variant message header type.
    */
    char fmt;
    /**
    * represents the basic header cid,
    * which is the chunk stream id.
    */
    int cid;
    /**
    * cached message header
    */
    SrsMessageHeader header;
    /**
    * whether the chunk message header has extended timestamp.
    */
    bool extended_timestamp;
    /**
    * partially read message.
    */
    SrsMessage* msg;
    /**
    * decoded msg count, to identify whether the chunk stream is fresh.
    */
    int64_t msg_count;
public:
    SrsChunkStream(int _cid);
    virtual ~SrsChunkStream();
};

/**
* message is raw data RTMP message, bytes oriented,
* protcol always recv RTMP message, and can send RTMP message or RTMP packet.
* the shared-ptr message is a special RTMP message, use ref-count for performance issue.
* 
* @remark, never directly new SrsMessage, the constructor is protected,
* for in the SrsMessage, we never know whether we should free the message,
* for SrsCommonMessage, we should free the payload,
* while for SrsSharedPtrMessage, we should use ref-count to free it.
* so, use these two concrete message, SrsCommonMessage or SrsSharedPtrMessage instread.
*/
class SrsMessage
{
// 4.1. Message Header
public:
    SrsMessageHeader header;
// 4.2. Message Payload
public:
    /**
    * The other part which is the payload is the actual data that is
    * contained in the message. For example, it could be some audio samples
    * or compressed video data. The payload format and interpretation are
    * beyond the scope of this document.
    */
    int32_t size;
    int8_t* payload;
protected:
    SrsMessage();
public:
    virtual ~SrsMessage();
};

/**
* the common message used free the payload in common way.
*/
class SrsCommonMessage : public SrsMessage
{
public:
    SrsCommonMessage();
    virtual ~SrsCommonMessage();
};

/**
* shared ptr message.
* for audio/video/data message that need less memory copy.
* and only for output.
*
* create first object by constructor and create(),
* use copy if need reference count message.
* 
* Usage:
*       SrsSharedPtrMessage msg;
*       
*/
class SrsSharedPtrMessage : public SrsMessage
{
private:
    class __SrsSharedPtr
    {
    public:
        char* payload;
        int size;
        int shared_count;
        
        __SrsSharedPtr();
        virtual ~__SrsSharedPtr();
    };
    __SrsSharedPtr* ptr;
public:
    SrsSharedPtrMessage();
    virtual ~SrsSharedPtrMessage();
public:
    /**
    * create shared ptr message, 
    * copy header, manage the payload of msg,
    * set the payload to NULL to prevent double free.
    * @remark payload of msg set to NULL if success.
    */
    virtual int create(SrsMessage* msg);
    /**
    * create shared ptr message,
    * from the header and payload.
    * @remark user should never free the payload.
    */
    virtual int create(SrsMessageHeader* pheader, char* payload, int size);
    /**
    * get current reference count.
    * when this object created, count set to 0.
    * if copy() this object, count increase 1.
    * if this or copy deleted, free payload when count is 0, or count--.
    * @remark, assert object is created.
    */
    virtual int count();
public:
    /**
    * copy current shared ptr message, use ref-count.
    * @remark, assert object is created.
    */
    virtual SrsSharedPtrMessage* copy();
};

/**
* the decoded message payload.
* @remark we seperate the packet from message,
*        for the packet focus on logic and domain data,
*        the message bind to the protocol and focus on protocol, such as header.
*         we can merge the message and packet, using OOAD hierachy, packet extends from message,
*         it's better for me to use components -- the message use the packet as payload.
*/
class SrsPacket
{
public:
    SrsPacket();
    virtual ~SrsPacket();
public:
    /**
    * the subpacket can override this encode,
    * for example, video and audio will directly set the payload withou memory copy,
    * other packet which need to serialize/encode to bytes by override the 
    * get_size and encode_packet.
    */
    virtual int encode(int& size, char*& payload);
// decode functions for concrete packet to override.
public:
    /**
    * subpacket must override to decode packet from stream.
    * @remark never invoke the super.decode, it always failed.
    */
    virtual int decode(SrsStream* stream);
// encode functions for concrete packet to override.
public:
    /**
    * the cid(chunk id) specifies the chunk to send data over.
    * generally, each message perfer some cid, for example, 
    * all protocol control messages perfer RTMP_CID_ProtocolControl,
    * SrsSetWindowAckSizePacket is protocol control message.
    */
    virtual int get_perfer_cid();
    /**
    * subpacket must override to provide the right message type.
    * the message type set the RTMP message type in header.
    */
    virtual int get_message_type();
protected:
    /**
    * subpacket can override to calc the packet size.
    */
    virtual int get_size();
    /**
    * subpacket can override to encode the payload to stream.
    * @remark never invoke the super.encode_packet, it always failed.
    */
    virtual int encode_packet(SrsStream* stream);
};

/**
* 4.1.1. connect
* The client sends the connect command to the server to request
* connection to a server application instance.
*/
class SrsConnectAppPacket : public SrsPacket
{
public:
    /**
    * Name of the command. Set to “connect”.
    */
    std::string command_name;
    /**
    * Always set to 1.
    */
    double transaction_id;
    /**
    * Command information object which has the name-value pairs.
    * @remark: alloc in packet constructor, user can directly use it, 
    *       user should never alloc it again which will cause memory leak.
    */
    SrsAmf0Object* command_object;
    /**
    * Any optional information
    */
    SrsAmf0Object* args;
public:
    SrsConnectAppPacket();
    virtual ~SrsConnectAppPacket();
// decode functions for concrete packet to override.
public:
    virtual int decode(SrsStream* stream);
// encode functions for concrete packet to override.
public:
    virtual int get_perfer_cid();
    virtual int get_message_type();
protected:
    virtual int get_size();
    virtual int encode_packet(SrsStream* stream);
};
/**
* response for SrsConnectAppPacket.
*/
class SrsConnectAppResPacket : public SrsPacket
{
public:
    /**
    * _result or _error; indicates whether the response is result or error.
    */
    std::string command_name;
    /**
    * Transaction ID is 1 for call connect responses
    */
    double transaction_id;
    /**
    * Name-value pairs that describe the properties(fmsver etc.) of the connection.
    */
    SrsAmf0Object* props;
    /**
    * Name-value pairs that describe the response from|the server. ‘code’,
    * ‘level’, ‘description’ are names of few among such information.
    */
    SrsAmf0Object* info;
public:
    SrsConnectAppResPacket();
    virtual ~SrsConnectAppResPacket();
// decode functions for concrete packet to override.
public:
    virtual int decode(SrsStream* stream);
// encode functions for concrete packet to override.
public:
    virtual int get_perfer_cid();
    virtual int get_message_type();
protected:
    virtual int get_size();
    virtual int encode_packet(SrsStream* stream);
};

/**
* 4.1.2. Call
* The call method of the NetConnection object runs remote procedure
* calls (RPC) at the receiving end. The called RPC name is passed as a
* parameter to the call command.
*/
class SrsCallPacket : public SrsPacket
{
public:
    /**
    * Name of the remote procedure that is called.
    */
    std::string command_name;
    /**
    * If a response is expected we give a transaction Id. Else we pass a value of 0
    */
    double transaction_id;
    /**
    * If there exists any command info this
    * is set, else this is set to null type.
    */
    SrsAmf0Any* command_object;
    /**
    * Any optional arguments to be provided
    */
    SrsAmf0Any* arguments;
public:
    SrsCallPacket();
    virtual ~SrsCallPacket();
// decode functions for concrete packet to override.
public:
    virtual int decode(SrsStream* stream);
// encode functions for concrete packet to override.
public:
    virtual int get_perfer_cid();
    virtual int get_message_type();
protected:
    virtual int get_size();
    virtual int encode_packet(SrsStream* stream);
};
/**
* response for SrsCallPacket.
*/
class SrsCallResPacket : public SrsPacket
{
public:
    /**
    * Name of the command. 
    */
    std::string command_name;
    /**
    * ID of the command, to which the response belongs to
    */
    double transaction_id;
    /**
    * If there exists any command info this is set, else this is set to null type.
    */
    SrsAmf0Any* command_object;
    /**
    * Response from the method that was called.
    */
    SrsAmf0Any* response;
public:
    SrsCallResPacket(double _transaction_id);
    virtual ~SrsCallResPacket();
// encode functions for concrete packet to override.
public:
    virtual int get_perfer_cid();
    virtual int get_message_type();
protected:
    virtual int get_size();
    virtual int encode_packet(SrsStream* stream);
};

/**
* 4.1.3. createStream
* The client sends this command to the server to create a logical
* channel for message communication The publishing of audio, video, and
* metadata is carried out over stream channel created using the
* createStream command.
*/
class SrsCreateStreamPacket : public SrsPacket
{
public:
    /**
    * Name of the command. Set to “createStream”.
    */
    std::string command_name;
    /**
    * Transaction ID of the command.
    */
    double transaction_id;
    /**
    * If there exists any command info this is set, else this is set to null type.
    */
    SrsAmf0Any* command_object; // null
public:
    SrsCreateStreamPacket();
    virtual ~SrsCreateStreamPacket();
// decode functions for concrete packet to override.
public:
    virtual int decode(SrsStream* stream);
// encode functions for concrete packet to override.
public:
    virtual int get_perfer_cid();
    virtual int get_message_type();
protected:
    virtual int get_size();
    virtual int encode_packet(SrsStream* stream);
};
/**
* response for SrsCreateStreamPacket.
*/
class SrsCreateStreamResPacket : public SrsPacket
{
public:
    /**
    * _result or _error; indicates whether the response is result or error.
    */
    std::string command_name;
    /**
    * ID of the command that response belongs to.
    */
    double transaction_id;
    /**
    * If there exists any command info this is set, else this is set to null type.
    */
    SrsAmf0Any* command_object; // null
    /**
    * The return value is either a stream ID or an error information object.
    */
    double stream_id;
public:
    SrsCreateStreamResPacket(double _transaction_id, double _stream_id);
    virtual ~SrsCreateStreamResPacket();
// decode functions for concrete packet to override.
public:
    virtual int decode(SrsStream* stream);
// encode functions for concrete packet to override.
public:
    virtual int get_perfer_cid();
    virtual int get_message_type();
protected:
    virtual int get_size();
    virtual int encode_packet(SrsStream* stream);
};

/**
* client close stream packet.
*/
class SrsCloseStreamPacket : public SrsPacket
{
public:
    /**
    * Name of the command, set to “closeStream”.
    */
    std::string command_name;
    /**
    * Transaction ID set to 0.
    */
    double transaction_id;
    /**
    * Command information object does not exist. Set to null type.
    */
    SrsAmf0Any* command_object; // null
public:
    SrsCloseStreamPacket();
    virtual ~SrsCloseStreamPacket();
// decode functions for concrete packet to override.
public:
    virtual int decode(SrsStream* stream);
};

/**
* FMLE start publish: ReleaseStream/PublishStream
*/
class SrsFMLEStartPacket : public SrsPacket
{
public:
    /**
    * Name of the command
    */
    std::string command_name;
    /**
    * the transaction ID to get the response.
    */
    double transaction_id;
    /**
    * If there exists any command info this is set, else this is set to null type.
    */
    SrsAmf0Any* command_object; // null
    /**
    * the stream name to start publish or release.
    */
    std::string stream_name;
public:
    SrsFMLEStartPacket();
    virtual ~SrsFMLEStartPacket();
// decode functions for concrete packet to override.
public:
    virtual int decode(SrsStream* stream);
// encode functions for concrete packet to override.
public:
    virtual int get_perfer_cid();
    virtual int get_message_type();
protected:
    virtual int get_size();
    virtual int encode_packet(SrsStream* stream);
// factory method to create specified FMLE packet.
public:
    static SrsFMLEStartPacket* create_release_stream(std::string stream);
    static SrsFMLEStartPacket* create_FC_publish(std::string stream);
};
/**
* response for SrsFMLEStartPacket.
*/
class SrsFMLEStartResPacket : public SrsPacket
{
public:
    /**
    * Name of the command
    */
    std::string command_name;
    /**
    * the transaction ID to get the response.
    */
    double transaction_id;
    /**
    * If there exists any command info this is set, else this is set to null type.
    */
    SrsAmf0Any* command_object; // null
    /**
    * the optional args, set to undefined.
    */
    SrsAmf0Any* args; // undefined
public:
    SrsFMLEStartResPacket(double _transaction_id);
    virtual ~SrsFMLEStartResPacket();
// decode functions for concrete packet to override.
public:
    virtual int decode(SrsStream* stream);
// encode functions for concrete packet to override.
public:
    virtual int get_perfer_cid();
    virtual int get_message_type();
protected:
    virtual int get_size();
    virtual int encode_packet(SrsStream* stream);
};

/**
* FMLE/flash publish
* 4.2.6. Publish
* The client sends the publish command to publish a named stream to the
* server. Using this name, any client can play this stream and receive
* the published audio, video, and data messages.
*/
class SrsPublishPacket : public SrsPacket
{
public:
    /**
    * Name of the command, set to “publish”.
    */
    std::string command_name;
    /**
    * Transaction ID set to 0.
    */
    double transaction_id;
    /**
    * Command information object does not exist. Set to null type.
    */
    SrsAmf0Any* command_object; // null
    /**
    * Name with which the stream is published.
    */
    std::string stream_name;
    /**
    * Type of publishing. Set to “live”, “record”, or “append”.
    *   record: The stream is published and the data is recorded to a new file.The file
    *           is stored on the server in a subdirectory within the directory that
    *           contains the server application. If the file already exists, it is 
    *           overwritten.
    *   append: The stream is published and the data is appended to a file. If no file
    *           is found, it is created.
    *   live: Live data is published without recording it in a file.
    * @remark, SRS only support live.
    * @remark, optional, default to live.
    */
    std::string type;
public:
    SrsPublishPacket();
    virtual ~SrsPublishPacket();
// decode functions for concrete packet to override.
public:
    virtual int decode(SrsStream* stream);
// encode functions for concrete packet to override.
public:
    virtual int get_perfer_cid();
    virtual int get_message_type();
protected:
    virtual int get_size();
    virtual int encode_packet(SrsStream* stream);
};

/**
* 4.2.8. pause
* The client sends the pause command to tell the server to pause or
* start playing.
*/
class SrsPausePacket : public SrsPacket
{
public:
    /**
    * Name of the command, set to “pause”.
    */
    std::string command_name;
    /**
    * There is no transaction ID for this command. Set to 0.
    */
    double transaction_id;
    /**
    * Command information object does not exist. Set to null type.
    */
    SrsAmf0Any* command_object; // null
    /**
    * true or false, to indicate pausing or resuming play
    */
    bool is_pause;
    /**
    * Number of milliseconds at which the the stream is paused or play resumed.
    * This is the current stream time at the Client when stream was paused. When the
    * playback is resumed, the server will only send messages with timestamps
    * greater than this value.
    */
    double time_ms;
public:
    SrsPausePacket();
    virtual ~SrsPausePacket();
// decode functions for concrete packet to override.
public:
    virtual int decode(SrsStream* stream);
};

/**
* 4.2.1. play
* The client sends this command to the server to play a stream.
*/
class SrsPlayPacket : public SrsPacket
{
public:
    /**
    * Name of the command. Set to “play”.
    */
    std::string command_name;
    /**
    * Transaction ID set to 0.
    */
    double transaction_id;
    /**
    * Command information does not exist. Set to null type.
    */
    SrsAmf0Any* command_object; // null
    /**
    * Name of the stream to play.
    * To play video (FLV) files, specify the name of the stream without a file
    *       extension (for example, "sample").
    * To play back MP3 or ID3 tags, you must precede the stream name with mp3:
    *       (for example, "mp3:sample".)
    * To play H.264/AAC files, you must precede the stream name with mp4: and specify the
    *       file extension. For example, to play the file sample.m4v, specify 
    *       "mp4:sample.m4v"
    */
    std::string stream_name;
    /**
    * An optional parameter that specifies the start time in seconds.
    * The default value is -2, which means the subscriber first tries to play the live 
    *       stream specified in the Stream Name field. If a live stream of that name is 
    *       not found, it plays the recorded stream specified in the Stream Name field.
    * If you pass -1 in the Start field, only the live stream specified in the Stream 
    *       Name field is played.
    * If you pass 0 or a positive number in the Start field, a recorded stream specified 
    *       in the Stream Name field is played beginning from the time specified in the 
    *       Start field.
    * If no recorded stream is found, the next item in the playlist is played.
    */
    double start;
    /**
    * An optional parameter that specifies the duration of playback in seconds.
    * The default value is -1. The -1 value means a live stream is played until it is no
    *       longer available or a recorded stream is played until it ends.
    * If u pass 0, it plays the single frame since the time specified in the Start field 
    *       from the beginning of a recorded stream. It is assumed that the value specified 
    *       in the Start field is equal to or greater than 0.
    * If you pass a positive number, it plays a live stream for the time period specified 
    *       in the Duration field. After that it becomes available or plays a recorded 
    *       stream for the time specified in the Duration field. (If a stream ends before the
    *       time specified in the Duration field, playback ends when the stream ends.)
    * If you pass a negative number other than -1 in the Duration field, it interprets the 
    *       value as if it were -1.
    */
    double duration;
    /**
    * An optional Boolean value or number that specifies whether to flush any
    * previous playlist.
    */
    bool reset;
public:
    SrsPlayPacket();
    virtual ~SrsPlayPacket();
// decode functions for concrete packet to override.
public:
    virtual int decode(SrsStream* stream);
// encode functions for concrete packet to override.
public:
    virtual int get_perfer_cid();
    virtual int get_message_type();
protected:
    virtual int get_size();
    virtual int encode_packet(SrsStream* stream);
};
/**
* response for SrsPlayPacket.
* @remark, user must set the stream_id in header.
*/
class SrsPlayResPacket : public SrsPacket
{
public:
    /**
    * Name of the command. If the play command is successful, the command
    * name is set to onStatus.
    */
    std::string command_name;
    /**
    * Transaction ID set to 0.
    */
    double transaction_id;
    /**
    * Command information does not exist. Set to null type.
    */
    SrsAmf0Any* command_object; // null
    /**
    * If the play command is successful, the client receives OnStatus message from
    * server which is NetStream.Play.Start. If the specified stream is not found,
    * NetStream.Play.StreamNotFound is received.
    */
    SrsAmf0Object* desc;
public:
    SrsPlayResPacket();
    virtual ~SrsPlayResPacket();
// encode functions for concrete packet to override.
public:
    virtual int get_perfer_cid();
    virtual int get_message_type();
protected:
    virtual int get_size();
    virtual int encode_packet(SrsStream* stream);
};

/**
* when bandwidth test done, notice client.
*/
class SrsOnBWDonePacket : public SrsPacket
{
public:
    /**
    * Name of command. Set to "onBWDone"
    */
    std::string command_name;
    /**
    * Transaction ID set to 0.
    */
    double transaction_id;
    /**
    * Command information does not exist. Set to null type.
    */
    SrsAmf0Any* args; // null
public:
    SrsOnBWDonePacket();
    virtual ~SrsOnBWDonePacket();
// encode functions for concrete packet to override.
public:
    virtual int get_perfer_cid();
    virtual int get_message_type();
protected:
    virtual int get_size();
    virtual int encode_packet(SrsStream* stream);
};

/**
* onStatus command, AMF0 Call
* @remark, user must set the stream_id by SrsMessage.set_packet().
*/
class SrsOnStatusCallPacket : public SrsPacket
{
public:
    /**
    * Name of command. Set to "onStatus"
    */
    std::string command_name;
    /**
    * Transaction ID set to 0.
    */
    double transaction_id;
    /**
    * Command information does not exist. Set to null type.
    */
    SrsAmf0Any* args; // null
    /**
    * Name-value pairs that describe the response from the server. 
    * ‘code’,‘level’, ‘description’ are names of few among such information.
    */
    SrsAmf0Object* data;
public:
    SrsOnStatusCallPacket();
    virtual ~SrsOnStatusCallPacket();
// encode functions for concrete packet to override.
public:
    virtual int get_perfer_cid();
    virtual int get_message_type();
protected:
    virtual int get_size();
    virtual int encode_packet(SrsStream* stream);
};

/**
* the special packet for the bandwidth test.
* actually, it's a SrsOnStatusCallPacket, but
* 1. encode with data field, to send data to client.
* 2. decode ignore the data field, donot care.
*/
class SrsBandwidthPacket : public SrsPacket
{
private:
    disable_default_copy(SrsBandwidthPacket);
public:
    /**
    * Name of command. 
    */
    std::string command_name;
    /**
    * Transaction ID set to 0.
    */
    double transaction_id;
    /**
    * Command information does not exist. Set to null type.
    */
    SrsAmf0Any* args; // null
    /**
    * Name-value pairs that describe the response from the server.
    * ‘code’,‘level’, ‘description’ are names of few among such information.
    */
    SrsAmf0Object* data;
public:
    SrsBandwidthPacket();
    virtual ~SrsBandwidthPacket();
// decode functions for concrete packet to override.
public:
    virtual int decode(SrsStream* stream);
// encode functions for concrete packet to override.
public:
    virtual int get_perfer_cid();
    virtual int get_message_type();
protected:
    virtual int get_size();
    virtual int encode_packet(SrsStream* stream);
// help function for bandwidth packet.
public:
    virtual bool is_starting_play();
    virtual bool is_stopped_play();
    virtual bool is_starting_publish();
    virtual bool is_stopped_publish();
    virtual bool is_flash_final();
    static SrsBandwidthPacket* create_finish();
    static SrsBandwidthPacket* create_start_play();
    static SrsBandwidthPacket* create_playing();
    static SrsBandwidthPacket* create_stop_play();
    static SrsBandwidthPacket* create_start_publish();
    static SrsBandwidthPacket* create_stop_publish();
private:
    virtual SrsBandwidthPacket* set_command(std::string command);
};

/**
* onStatus data, AMF0 Data
* @remark, user must set the stream_id by SrsMessage.set_packet().
*/
class SrsOnStatusDataPacket : public SrsPacket
{
public:
    /**
    * Name of command. Set to "onStatus"
    */
    std::string command_name;
    /**
    * Name-value pairs that describe the response from the server.
    * ‘code’, are names of few among such information.
    */
    SrsAmf0Object* data;
public:
    SrsOnStatusDataPacket();
    virtual ~SrsOnStatusDataPacket();
// encode functions for concrete packet to override.
public:
    virtual int get_perfer_cid();
    virtual int get_message_type();
protected:
    virtual int get_size();
    virtual int encode_packet(SrsStream* stream);
};

/**
* AMF0Data RtmpSampleAccess
* @remark, user must set the stream_id by SrsMessage.set_packet().
*/
class SrsSampleAccessPacket : public SrsPacket
{
public:
    /**
    * Name of command. Set to "|RtmpSampleAccess".
    */
    std::string command_name;
    /**
    * whether allow access the sample of video.
    * @see: https://github.com/winlinvip/simple-rtmp-server/issues/49
    * @see: http://help.adobe.com/en_US/FlashPlatform/reference/actionscript/3/flash/net/NetStream.html#videoSampleAccess
    */
    bool video_sample_access;
    /**
    * whether allow access the sample of audio.
    * @see: https://github.com/winlinvip/simple-rtmp-server/issues/49
    * @see: http://help.adobe.com/en_US/FlashPlatform/reference/actionscript/3/flash/net/NetStream.html#audioSampleAccess
    */
    bool audio_sample_access;
public:
    SrsSampleAccessPacket();
    virtual ~SrsSampleAccessPacket();
// encode functions for concrete packet to override.
public:
    virtual int get_perfer_cid();
    virtual int get_message_type();
protected:
    virtual int get_size();
    virtual int encode_packet(SrsStream* stream);
};

/**
* the stream metadata.
* FMLE: @setDataFrame
* others: onMetaData
*/
class SrsOnMetaDataPacket : public SrsPacket
{
public:
    /**
    * Name of metadata. Set to "onMetaData"
    */
    std::string name;
    /**
    * Metadata of stream.
    */
    SrsAmf0Object* metadata;
public:
    SrsOnMetaDataPacket();
    virtual ~SrsOnMetaDataPacket();
// decode functions for concrete packet to override.
public:
    virtual int decode(SrsStream* stream);
// encode functions for concrete packet to override.
public:
    virtual int get_perfer_cid();
    virtual int get_message_type();
protected:
    virtual int get_size();
    virtual int encode_packet(SrsStream* stream);
};

/**
* 5.5. Window Acknowledgement Size (5)
* The client or the server sends this message to inform the peer which
* window size to use when sending acknowledgment.
*/
class SrsSetWindowAckSizePacket : public SrsPacket
{
public:
    int32_t ackowledgement_window_size;
public:
    SrsSetWindowAckSizePacket();
    virtual ~SrsSetWindowAckSizePacket();
// decode functions for concrete packet to override.
public:
    virtual int decode(SrsStream* stream);
// encode functions for concrete packet to override.
public:
    virtual int get_perfer_cid();
    virtual int get_message_type();
protected:
    virtual int get_size();
    virtual int encode_packet(SrsStream* stream);
};

/**
* 5.3. Acknowledgement (3)
* The client or the server sends the acknowledgment to the peer after
* receiving bytes equal to the window size.
*/
class SrsAcknowledgementPacket : public SrsPacket
{
public:
    int32_t sequence_number;
public:
    SrsAcknowledgementPacket();
    virtual ~SrsAcknowledgementPacket();
// encode functions for concrete packet to override.
public:
    virtual int get_perfer_cid();
    virtual int get_message_type();
protected:
    virtual int get_size();
    virtual int encode_packet(SrsStream* stream);
};

/**
* 7.1. Set Chunk Size
* Protocol control message 1, Set Chunk Size, is used to notify the
* peer about the new maximum chunk size.
*/
class SrsSetChunkSizePacket : public SrsPacket
{
public:
    /**
    * The maximum chunk size can be 65536 bytes. The chunk size is
    * maintained independently for each direction.
    */
    int32_t chunk_size;
public:
    SrsSetChunkSizePacket();
    virtual ~SrsSetChunkSizePacket();
// decode functions for concrete packet to override.
public:
    virtual int decode(SrsStream* stream);
// encode functions for concrete packet to override.
public:
    virtual int get_perfer_cid();
    virtual int get_message_type();
protected:
    virtual int get_size();
    virtual int encode_packet(SrsStream* stream);
};

/**
* 5.6. Set Peer Bandwidth (6)
* The client or the server sends this message to update the output
* bandwidth of the peer.
*/
class SrsSetPeerBandwidthPacket : public SrsPacket
{
public:
    int32_t bandwidth;
    int8_t type;
public:
    SrsSetPeerBandwidthPacket();
    virtual ~SrsSetPeerBandwidthPacket();
// encode functions for concrete packet to override.
public:
    virtual int get_perfer_cid();
    virtual int get_message_type();
protected:
    virtual int get_size();
    virtual int encode_packet(SrsStream* stream);
};

// 3.7. User Control message
enum SrcPCUCEventType
{
    // generally, 4bytes event-data
    
    /**
    * The server sends this event to notify the client
    * that a stream has become functional and can be
    * used for communication. By default, this event
    * is sent on ID 0 after the application connect
    * command is successfully received from the
    * client. The event data is 4-byte and represents
    * the stream ID of the stream that became
    * functional.
    */
    SrcPCUCStreamBegin              = 0x00,

    /**
    * The server sends this event to notify the client
    * that the playback of data is over as requested
    * on this stream. No more data is sent without
    * issuing additional commands. The client discards
    * the messages received for the stream. The
    * 4 bytes of event data represent the ID of the
    * stream on which playback has ended.
    */
    SrcPCUCStreamEOF                = 0x01,

    /**
    * The server sends this event to notify the client
    * that there is no more data on the stream. If the
    * server does not detect any message for a time
    * period, it can notify the subscribed clients 
    * that the stream is dry. The 4 bytes of event 
    * data represent the stream ID of the dry stream. 
    */
    SrcPCUCStreamDry                = 0x02,

    /**
    * The client sends this event to inform the server
    * of the buffer size (in milliseconds) that is 
    * used to buffer any data coming over a stream.
    * This event is sent before the server starts  
    * processing the stream. The first 4 bytes of the
    * event data represent the stream ID and the next
    * 4 bytes represent the buffer length, in 
    * milliseconds.
    */
    SrcPCUCSetBufferLength          = 0x03, // 8bytes event-data

    /**
    * The server sends this event to notify the client
    * that the stream is a recorded stream. The
    * 4 bytes event data represent the stream ID of
    * the recorded stream.
    */
    SrcPCUCStreamIsRecorded         = 0x04,

    /**
    * The server sends this event to test whether the
    * client is reachable. Event data is a 4-byte
    * timestamp, representing the local server time
    * when the server dispatched the command. The
    * client responds with kMsgPingResponse on
    * receiving kMsgPingRequest.  
    */
    SrcPCUCPingRequest              = 0x06,

    /**
    * The client sends this event to the server in
    * response to the ping request. The event data is
    * a 4-byte timestamp, which was received with the
    * kMsgPingRequest request.
    */
    SrcPCUCPingResponse             = 0x07,
};

/**
* 5.4. User Control Message (4)
* 
* for the EventData is 4bytes.
* Stream Begin(=0)              4-bytes stream ID
* Stream EOF(=1)                4-bytes stream ID
* StreamDry(=2)                 4-bytes stream ID
* SetBufferLength(=3)           8-bytes 4bytes stream ID, 4bytes buffer length.
* StreamIsRecorded(=4)          4-bytes stream ID
* PingRequest(=6)               4-bytes timestamp local server time
* PingResponse(=7)              4-bytes timestamp received ping request.
* 
* 3.7. User Control message
* +------------------------------+-------------------------
* | Event Type ( 2- bytes ) | Event Data
* +------------------------------+-------------------------
* Figure 5 Pay load for the ‘User Control Message’.
*/
class SrsUserControlPacket : public SrsPacket
{
public:
    /**
    * Event type is followed by Event data.
    * @see: SrcPCUCEventType
    */
    int16_t event_type;
    int32_t event_data;
    /**
    * 4bytes if event_type is SetBufferLength; otherwise 0.
    */
    int32_t extra_data;
public:
    SrsUserControlPacket();
    virtual ~SrsUserControlPacket();
// decode functions for concrete packet to override.
public:
    virtual int decode(SrsStream* stream);
// encode functions for concrete packet to override.
public:
    virtual int get_perfer_cid();
    virtual int get_message_type();
protected:
    virtual int get_size();
    virtual int encode_packet(SrsStream* stream);
};

/**
* expect a specified message, drop others util got specified one.
* @pmsg, user must free it. NULL if not success.
* @ppacket, store in the pmsg, user must never free it. NULL if not success.
* @remark, only when success, user can use and must free the pmsg/ppacket.
* for example:
         SrsCommonMessage* msg = NULL;
        SrsConnectAppResPacket* pkt = NULL;
        if ((ret = srs_rtmp_expect_message<SrsConnectAppResPacket>(protocol, &msg, &pkt)) != ERROR_SUCCESS) {
            return ret;
        }
        // use pkt
* user should never recv message and convert it, use this method instead.
* if need to set timeout, use set timeout of SrsProtocol.
*/
template<class T>
int srs_rtmp_expect_message(SrsProtocol* protocol, SrsMessage** pmsg, T** ppacket)
{
    *pmsg = NULL;
    *ppacket = NULL;
    
    int ret = ERROR_SUCCESS;
    
    while (true) {
        SrsMessage* msg = NULL;
        if ((ret = protocol->recv_message(&msg)) != ERROR_SUCCESS) {
            srs_error("recv message failed. ret=%d", ret);
            return ret;
        }
        srs_verbose("recv message success.");
        
        SrsPacket* packet = NULL;
        if ((ret = protocol->decode_message(msg, &packet)) != ERROR_SUCCESS) {
            srs_error("decode message failed. ret=%d", ret);
            srs_freep(msg);
            srs_freep(packet);
            return ret;
        }
        
        T* pkt = dynamic_cast<T*>(packet);
        if (!pkt) {
            srs_info("drop message(type=%d, size=%d, time=%"PRId64", sid=%d).", 
                msg->header.message_type, msg->header.payload_length,
                msg->header.timestamp, msg->header.stream_id);
            srs_freep(msg);
            srs_freep(packet);
            continue;
        }
        
        *pmsg = msg;
        *ppacket = pkt;
        break;
    }
    
    return ret;
}

#endif
