/*
 The MIT License (MIT)
 
 Copyright (c) 2013-2015 SRS(simple-rtmp-server)
 
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

#ifndef SRS_APP_KAFKA_HPP
#define SRS_APP_KAFKA_HPP

/*
#include <srs_app_kafka.hpp>
*/
#include <srs_core.hpp>

#include <vector>

class SrsLbRoundRobin;
class SrsAsyncCallWorker;
class SrsTcpClient;
class SrsKafkaClient;
class SrsJsonObject;
class SrsKafkaProducer;

#include <srs_app_thread.hpp>
#include <srs_app_server.hpp>
#include <srs_app_async_call.hpp>

#ifdef SRS_AUTO_KAFKA

/**
 * the kafka partition info.
 */
struct SrsKafkaPartition
{
private:
    std::string ep;
public:
    int id;
    // leader.
    int broker;
    std::string host;
    int port;
public:
    SrsKafkaPartition();
    virtual ~SrsKafkaPartition();
public:
    virtual std::string hostport();
};

/**
 * the following is all types of kafka messages.
 */
struct SrsKafkaMessageOnClient : public ISrsAsyncCallTask
{
public:
    SrsKafkaProducer* producer;
    SrsListenerType type;
    std::string ip;
public:
    SrsKafkaMessageOnClient(SrsKafkaProducer* p, SrsListenerType t, std::string i);
    virtual ~SrsKafkaMessageOnClient();
// interface ISrsAsyncCallTask
public:
    virtual int call();
    virtual std::string to_string();
};

/**
 * the kafka producer used to save log to kafka cluster.
 */
class SrsKafkaProducer : public ISrsReusableThreadHandler
{
private:
    st_mutex_t lock;
    SrsReusableThread* pthread;
private:
    bool metadata_ok;
    st_cond_t metadata_expired;
public:
    std::vector<SrsKafkaPartition*> partitions;
    std::vector<SrsJsonObject*> objects;
private:
    SrsLbRoundRobin* lb;
    SrsAsyncCallWorker* worker;
    SrsTcpClient* transport;
    SrsKafkaClient* kafka;
public:
    SrsKafkaProducer();
    virtual ~SrsKafkaProducer();
public:
    virtual int initialize();
    virtual int start();
    virtual void stop();
public:
    /**
     * when got any client connect to SRS, notify kafka.
     */
    virtual int on_client(SrsListenerType type, st_netfd_t stfd);
    /**
     * send json object to kafka cluster.
     * the producer will aggregate message and send in kafka message set.
     * @param obj the json object; user must never free it again.
     */
    virtual int send(SrsJsonObject* obj);
// interface ISrsReusableThreadHandler
public:
    virtual int cycle();
    virtual int on_before_cycle();
    virtual int on_end_cycle();
private:
    virtual int do_cycle();
    virtual int request_metadata();
    // set the metadata to invalid and refresh it.
    virtual void refresh_metadata();
    virtual int flush();
};

#endif

#endif

