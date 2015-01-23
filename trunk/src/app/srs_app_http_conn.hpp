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

#ifndef SRS_APP_HTTP_CONN_HPP
#define SRS_APP_HTTP_CONN_HPP

/*
#include <srs_app_http_conn.hpp>
*/

#include <srs_core.hpp>

#ifdef SRS_AUTO_HTTP_SERVER

#include <srs_app_st.hpp>
#include <srs_app_conn.hpp>
#include <srs_app_http.hpp>
#include <srs_app_reload.hpp>
#include <srs_kernel_file.hpp>
#include <srs_app_thread.hpp>

class SrsSource;
class SrsRequest;
class SrsConsumer;
class SrsStSocket;
class SrsTsEncoder;
class SrsAacEncoder;
class SrsMp3Encoder;
class SrsFlvEncoder;
class SrsHttpParser;
class SrsHttpMessage;
class SrsHttpHandler;
class SrsMessageQueue;
class SrsSharedPtrMessage;

/**
* the flv vod stream supports flv?start=offset-bytes.
* for example, http://server/file.flv?start=10240
* server will write flv header and sequence header, 
* then seek(10240) and response flv tag data.
*/
class SrsVodStream : public SrsGoHttpFileServer
{
public:
    SrsVodStream(std::string root_dir);
    virtual ~SrsVodStream();
protected:
    virtual int serve_flv_stream(ISrsGoHttpResponseWriter* w, SrsHttpMessage* r, std::string fullpath, int offset);
    virtual int serve_mp4_stream(ISrsGoHttpResponseWriter* w, SrsHttpMessage* r, std::string fullpath, int start, int end);
};

/**
* for the srs http stream cache, 
* for example, the audio stream cache to make android(weixin) happy.
* we start a thread to shrink the queue.
*/
class SrsStreamCache : public ISrsThreadHandler
{
private:
    SrsMessageQueue* queue;
    SrsSource* source;
    SrsRequest* req;
    SrsThread* pthread;
public:
    SrsStreamCache(SrsSource* s, SrsRequest* r);
    virtual ~SrsStreamCache();
public:
    virtual int start();
    virtual int dump_cache(SrsConsumer* consumer);
// interface ISrsThreadHandler.
public:
    virtual int cycle();
};

/**
* the stream encoder in some codec, for example, flv or aac.
*/
class ISrsStreamEncoder
{
public:
    ISrsStreamEncoder();
    virtual ~ISrsStreamEncoder();
public:
    /**
    * initialize the encoder with file writer(to http response) and stream cache.
    * @param w the writer to write to http response.
    * @param c the stream cache for audio stream fast startup.
    */
    virtual int initialize(SrsFileWriter* w, SrsStreamCache* c) = 0;
    /**
    * write rtmp video/audio/metadata.
    */
    virtual int write_audio(int64_t timestamp, char* data, int size) = 0;
    virtual int write_video(int64_t timestamp, char* data, int size) = 0;
    virtual int write_metadata(int64_t timestamp, char* data, int size) = 0;
public:
    /**
    * for some stream, for example, mp3 and aac, the audio stream,
    * we use large gop cache in encoder, for the gop cache of SrsSource is ignore audio.
    * @return true to use gop cache of encoder; otherwise, use SrsSource.
    */
    virtual bool has_cache() = 0;
    /**
    * dumps the cache of encoder to consumer.
    */
    virtual int dump_cache(SrsConsumer* consumer) = 0;
};

/**
* the flv stream encoder, remux rtmp stream to flv stream.
*/
class SrsFlvStreamEncoder : public ISrsStreamEncoder
{
private:
    SrsFlvEncoder* enc;
public:
    SrsFlvStreamEncoder();
    virtual ~SrsFlvStreamEncoder();
public:
    virtual int initialize(SrsFileWriter* w, SrsStreamCache* c);
    virtual int write_audio(int64_t timestamp, char* data, int size);
    virtual int write_video(int64_t timestamp, char* data, int size);
    virtual int write_metadata(int64_t timestamp, char* data, int size);
public:
    virtual bool has_cache();
    virtual int dump_cache(SrsConsumer* consumer);
};

/**
* the ts stream encoder, remux rtmp stream to ts stream.
*/
class SrsTsStreamEncoder : public ISrsStreamEncoder
{
private:
    SrsTsEncoder* enc;
public:
    SrsTsStreamEncoder();
    virtual ~SrsTsStreamEncoder();
public:
    virtual int initialize(SrsFileWriter* w, SrsStreamCache* c);
    virtual int write_audio(int64_t timestamp, char* data, int size);
    virtual int write_video(int64_t timestamp, char* data, int size);
    virtual int write_metadata(int64_t timestamp, char* data, int size);
public:
    virtual bool has_cache();
    virtual int dump_cache(SrsConsumer* consumer);
};

/**
* the aac stream encoder, remux rtmp stream to aac stream.
*/
class SrsAacStreamEncoder : public ISrsStreamEncoder
{
private:
    SrsAacEncoder* enc;
    SrsStreamCache* cache;
public:
    SrsAacStreamEncoder();
    virtual ~SrsAacStreamEncoder();
public:
    virtual int initialize(SrsFileWriter* w, SrsStreamCache* c);
    virtual int write_audio(int64_t timestamp, char* data, int size);
    virtual int write_video(int64_t timestamp, char* data, int size);
    virtual int write_metadata(int64_t timestamp, char* data, int size);
public:
    virtual bool has_cache();
    virtual int dump_cache(SrsConsumer* consumer);
};

/**
* the mp3 stream encoder, remux rtmp stream to mp3 stream.
*/
class SrsMp3StreamEncoder : public ISrsStreamEncoder
{
private:
    SrsMp3Encoder* enc;
    SrsStreamCache* cache;
public:
    SrsMp3StreamEncoder();
    virtual ~SrsMp3StreamEncoder();
public:
    virtual int initialize(SrsFileWriter* w, SrsStreamCache* c);
    virtual int write_audio(int64_t timestamp, char* data, int size);
    virtual int write_video(int64_t timestamp, char* data, int size);
    virtual int write_metadata(int64_t timestamp, char* data, int size);
public:
    virtual bool has_cache();
    virtual int dump_cache(SrsConsumer* consumer);
};

/**
* write stream to http response direclty.
*/
class SrsStreamWriter : public SrsFileWriter
{
private:
    ISrsGoHttpResponseWriter* writer;
public:
    SrsStreamWriter(ISrsGoHttpResponseWriter* w);
    virtual ~SrsStreamWriter();
public:
    virtual int open(std::string file);
    virtual void close();
public:
    virtual bool is_open();
    virtual int64_t tellg();
public:
    virtual int write(void* buf, size_t count, ssize_t* pnwrite);
};

/**
* the flv live stream supports access rtmp in flv over http.
* srs will remux rtmp to flv streaming.
*/
class SrsLiveStream : public ISrsGoHttpHandler
{
private:
    SrsRequest* req;
    SrsSource* source;
    SrsStreamCache* cache;
public:
    SrsLiveStream(SrsSource* s, SrsRequest* r, SrsStreamCache* c);
    virtual ~SrsLiveStream();
public:
    virtual int serve_http(ISrsGoHttpResponseWriter* w, SrsHttpMessage* r);
private:
    virtual int streaming_send_messages(ISrsStreamEncoder* enc, SrsSharedPtrMessage** msgs, int nb_msgs);
};

/**
* the srs live entry
*/
struct SrsLiveEntry
{
    std::string vhost;
    std::string mount;
    SrsLiveStream* stream;
    SrsStreamCache* cache;
    
    SrsLiveEntry();
};

/**
* the http server instance,
* serve http static file, flv vod stream and flv live stream.
*/
class SrsHttpServer : public ISrsReloadHandler
{
public:
    SrsGoHttpServeMux mux;
    // the flv live streaming template.
    std::map<std::string, SrsLiveEntry*> flvs;
public:
    SrsHttpServer();
    virtual ~SrsHttpServer();
public:
    virtual int initialize();
public:
    virtual int mount(SrsSource* s, SrsRequest* r);
    virtual void unmount(SrsSource* s, SrsRequest* r);
// interface ISrsThreadHandler.
public:
    virtual int on_reload_vhost_http_updated();
    virtual int on_reload_vhost_http_remux_updated();
private:
    virtual int mount_static_file();
    virtual int mount_flv_streaming();
};

class SrsHttpConn : public SrsConnection
{
private:
    SrsHttpParser* parser;
    SrsHttpServer* mux;
public:
    SrsHttpConn(SrsServer* svr, st_netfd_t fd, SrsHttpServer* m);
    virtual ~SrsHttpConn();
public:
    virtual void kbps_resample();
// interface IKbpsDelta
public:
    virtual int64_t get_send_bytes_delta();
    virtual int64_t get_recv_bytes_delta();
protected:
    virtual int do_cycle();
private:
    virtual int process_request(ISrsGoHttpResponseWriter* w, SrsHttpMessage* r);
};

#endif

#endif

