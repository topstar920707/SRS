/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2013-2018 Winlin
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

#include <srs_app_kafka.hpp>

#include <vector>
using namespace std;

#include <srs_kernel_error.hpp>
#include <srs_kernel_log.hpp>
#include <srs_app_config.hpp>
#include <srs_app_async_call.hpp>
#include <srs_app_utility.hpp>
#include <srs_kernel_utility.hpp>
#include <srs_protocol_utility.hpp>
#include <srs_kernel_balance.hpp>
#include <srs_kafka_stack.hpp>
#include <srs_core_autofree.hpp>
#include <srs_protocol_json.hpp>

#ifdef SRS_AUTO_KAFKA

#define SRS_KAFKA_PRODUCER_TIMEOUT 30000
#define SRS_KAFKA_PRODUCER_AGGREGATE_SIZE 1

std::string srs_kafka_metadata_summary(SrsKafkaTopicMetadataResponse* metadata)
{
    vector<string> bs;
    for (int i = 0; i < metadata->brokers.size(); i++) {
        SrsKafkaBroker* broker = metadata->brokers.at(i);
        
        string hostport = srs_int2str(broker->node_id) + "/" + broker->host.to_str();
        if (broker->port > 0) {
            hostport += ":" + srs_int2str(broker->port);
        }
        
        bs.push_back(hostport);
    }
    
    vector<string> ps;
    for (int i = 0; i < metadata->metadatas.size(); i++) {
        SrsKafkaTopicMetadata* topic = metadata->metadatas.at(i);
        
        for (int j = 0; j < topic->metadatas.size(); j++) {
            string desc = "topic=" + topic->name.to_str();
            
            SrsKafkaPartitionMetadata* partition = topic->metadatas.at(j);
            
            desc += "?partition=" + srs_int2str(partition->partition_id);
            desc += "&leader=" + srs_int2str(partition->leader);
            
            vector<string> replicas = srs_kafka_array2vector(&partition->replicas);
            desc += "&replicas=" + srs_join_vector_string(replicas, ",");
            
            ps.push_back(desc);
        }
    }
    
    std::stringstream ss;
    ss << "brokers=" << srs_join_vector_string(bs, ",");
    ss << ", " << srs_join_vector_string(ps, ", ");
    
    return ss.str();
}

std::string srs_kafka_summary_partitions(const vector<SrsKafkaPartition*>& partitions)
{
    vector<string> ret;
    
    vector<SrsKafkaPartition*>::const_iterator it;
    for (it = partitions.begin(); it != partitions.end(); ++it) {
        SrsKafkaPartition* partition = *it;
        
        string desc = "tcp://";
        desc += partition->host + ":" + srs_int2str(partition->port);
        desc += "?broker=" + srs_int2str(partition->broker);
        desc += "&partition=" + srs_int2str(partition->id);
        ret.push_back(desc);
    }
    
    return srs_join_vector_string(ret, ", ");
}

void srs_kafka_metadata2connector(string topic_name, SrsKafkaTopicMetadataResponse* metadata, vector<SrsKafkaPartition*>& partitions)
{
    for (int i = 0; i < metadata->metadatas.size(); i++) {
        SrsKafkaTopicMetadata* topic = metadata->metadatas.at(i);
        
        for (int j = 0; j < topic->metadatas.size(); j++) {
            SrsKafkaPartitionMetadata* partition = topic->metadatas.at(j);
            
            SrsKafkaPartition* p = new SrsKafkaPartition();
            
            p->topic = topic_name;
            p->id = partition->partition_id;
            p->broker = partition->leader;
            
            for (int i = 0; i < metadata->brokers.size(); i++) {
                SrsKafkaBroker* broker = metadata->brokers.at(i);
                if (broker->node_id == p->broker) {
                    p->host = broker->host.to_str();
                    p->port = broker->port;
                    break;
                }
            }
            
            partitions.push_back(p);
        }
    }
}

SrsKafkaPartition::SrsKafkaPartition()
{
    id = broker = 0;
    port = SRS_CONSTS_KAFKA_DEFAULT_PORT;
    
    transport = NULL;
    kafka = NULL;
}

SrsKafkaPartition::~SrsKafkaPartition()
{
    disconnect();
}

string SrsKafkaPartition::hostport()
{
    if (ep.empty()) {
        ep = host + ":" + srs_int2str(port);
    }
    
    return ep;
}

srs_error_t SrsKafkaPartition::connect()
{
    srs_error_t err = srs_success;
    
    if (transport) {
        return err;
    }
    transport = new SrsTcpClient(host, port, SRS_KAFKA_PRODUCER_TIMEOUT);
    kafka = new SrsKafkaClient(transport);
    
    if ((err = transport->connect()) != srs_success) {
        disconnect();
        return srs_error_wrap(err, "connect to %s partition=%d failed", hostport().c_str(), id);
    }
    
    srs_trace("connect at %s, partition=%d, broker=%d", hostport().c_str(), id, broker);
    
    return err;
}

srs_error_t SrsKafkaPartition::flush(SrsKafkaPartitionCache* pc)
{
    return kafka->write_messages(topic, id, *pc);
}

void SrsKafkaPartition::disconnect()
{
    srs_freep(kafka);
    srs_freep(transport);
}

SrsKafkaMessage::SrsKafkaMessage(SrsKafkaProducer* p, int k, SrsJsonObject* j)
{
    producer = p;
    key = k;
    obj = j;
}

SrsKafkaMessage::~SrsKafkaMessage()
{
    srs_freep(obj);
}

srs_error_t SrsKafkaMessage::call()
{
    srs_error_t err = producer->send(key, obj);
    
    // the obj is manged by producer now.
    obj = NULL;
    
    return srs_error_wrap(err, "kafka send");
}

string SrsKafkaMessage::to_string()
{
    return "kafka";
}

SrsKafkaCache::SrsKafkaCache()
{
    count = 0;
    nb_partitions = 0;
}

SrsKafkaCache::~SrsKafkaCache()
{
    map<int32_t, SrsKafkaPartitionCache*>::iterator it;
    for (it = cache.begin(); it != cache.end(); ++it) {
        SrsKafkaPartitionCache* pc = it->second;
        
        for (vector<SrsJsonObject*>::iterator it2 = pc->begin(); it2 != pc->end(); ++it2) {
            SrsJsonObject* obj = *it2;
            srs_freep(obj);
        }
        pc->clear();
        
        srs_freep(pc);
    }
    cache.clear();
}

void SrsKafkaCache::append(int key, SrsJsonObject* obj)
{
    count++;
    
    int partition = 0;
    if (nb_partitions > 0) {
        partition = key % nb_partitions;
    }
    
    SrsKafkaPartitionCache* pc = NULL;
    map<int32_t, SrsKafkaPartitionCache*>::iterator it = cache.find(partition);
    if (it == cache.end()) {
        pc = new SrsKafkaPartitionCache();
        cache[partition] = pc;
    } else {
        pc = it->second;
    }
    
    pc->push_back(obj);
}

int SrsKafkaCache::size()
{
    return count;
}

bool SrsKafkaCache::fetch(int* pkey, SrsKafkaPartitionCache** ppc)
{
    map<int32_t, SrsKafkaPartitionCache*>::iterator it;
    for (it = cache.begin(); it != cache.end(); ++it) {
        int32_t key = it->first;
        SrsKafkaPartitionCache* pc = it->second;
        
        if (!pc->empty()) {
            *pkey = (int)key;
            *ppc = pc;
            return true;
        }
    }
    
    return false;
}

srs_error_t SrsKafkaCache::flush(SrsKafkaPartition* partition, int key, SrsKafkaPartitionCache* pc)
{
    srs_error_t err = srs_success;
    
    // ensure the key exists.
    srs_assert (cache.find(key) != cache.end());
    
    // the cache is vector, which is continous store.
    // we remember the messages we have written and clear it when completed.
    int nb_msgs = (int)pc->size();
    if (pc->empty()) {
        return err;
    }
    
    // connect transport.
    if ((err = partition->connect()) != srs_success) {
        return srs_error_wrap(err, "connect partition");
    }
    
    // write the json objects.
    if ((err = partition->flush(pc)) != srs_success) {
        return srs_error_wrap(err, "flush partition");
    }
    
    // free all wrote messages.
    for (vector<SrsJsonObject*>::iterator it = pc->begin(); it != pc->end(); ++it) {
        SrsJsonObject* obj = *it;
        srs_freep(obj);
    }
    
    // remove the messages from cache.
    if ((int)pc->size() == nb_msgs) {
        pc->clear();
    } else {
        pc->erase(pc->begin(), pc->begin() + nb_msgs);
    }
    
    return err;
}

ISrsKafkaCluster::ISrsKafkaCluster()
{
}

ISrsKafkaCluster::~ISrsKafkaCluster()
{
}

// @global kafka event producer, user must use srs_initialize_kafka to initialize it.
ISrsKafkaCluster* _srs_kafka = NULL;

srs_error_t srs_initialize_kafka()
{
    srs_error_t err = srs_success;
    
    SrsKafkaProducer* kafka = new SrsKafkaProducer();
    _srs_kafka = kafka;
    
    if ((err = kafka->initialize()) != srs_success) {
        return srs_error_wrap(err, "initialize kafka producer");
    }
    
    if ((err = kafka->start()) != srs_success) {
        return srs_error_wrap(err, "start kafka producer");
    }
    
    return err;
}

void srs_dispose_kafka()
{
    SrsKafkaProducer* kafka = dynamic_cast<SrsKafkaProducer*>(_srs_kafka);
    if (!kafka) {
        return;
    }
    
    kafka->stop();
    
    srs_freep(kafka);
    _srs_kafka = NULL;
}

SrsKafkaProducer::SrsKafkaProducer()
{
    metadata_ok = false;
    metadata_expired = srs_cond_new();
    
    lock = srs_mutex_new();
    trd = new SrsDummyCoroutine();
    worker = new SrsAsyncCallWorker();
    cache = new SrsKafkaCache();
    
    lb = new SrsLbRoundRobin();
}

SrsKafkaProducer::~SrsKafkaProducer()
{
    clear_metadata();
    
    srs_freep(lb);
    
    srs_freep(worker);
    srs_freep(trd);
    srs_freep(cache);
    
    srs_mutex_destroy(lock);
    srs_cond_destroy(metadata_expired);
}

srs_error_t SrsKafkaProducer::initialize()
{
    enabled = _srs_config->get_kafka_enabled();
    srs_info("initialize kafka ok, enabled=%d.", enabled);
    return srs_success;
}

srs_error_t SrsKafkaProducer::start()
{
    srs_error_t err = srs_success;
    
    if (!enabled) {
        return err;
    }
    
    if ((err = worker->start()) != srs_success) {
        return srs_error_wrap(err, "async worker");
    }
    
    srs_freep(trd);
    trd = new SrsSTCoroutine("kafka", this, _srs_context->get_id());
    if ((err = trd->start()) != srs_success) {
        return srs_error_wrap(err, "coroutine");
    }
    
    refresh_metadata();
    
    return err;
}

void SrsKafkaProducer::stop()
{
    if (!enabled) {
        return;
    }
    
    trd->stop();
    worker->stop();
}

srs_error_t SrsKafkaProducer::send(int key, SrsJsonObject* obj)
{
    srs_error_t err = srs_success;
    
    // cache the json object.
    cache->append(key, obj);
    
    // too few messages, ignore.
    if (cache->size() < SRS_KAFKA_PRODUCER_AGGREGATE_SIZE) {
        return err;
    }
    
    // too many messages, warn user.
    if (cache->size() > SRS_KAFKA_PRODUCER_AGGREGATE_SIZE * 10) {
        srs_warn("kafka cache too many messages: %d", cache->size());
    }
    
    // sync with backgound metadata worker.
    SrsLocker(lock);
    
    // flush message when metadata is ok.
    if (metadata_ok) {
        err = flush();
    }
    
    return err;
}

srs_error_t SrsKafkaProducer::on_client(int key, SrsListenerType type, string ip)
{
    srs_error_t err = srs_success;
    
    if (!enabled) {
        return err;
    }
    
    SrsJsonObject* obj = SrsJsonAny::object();
    
    obj->set("msg", SrsJsonAny::str("accept"));
    obj->set("type", SrsJsonAny::integer(type));
    obj->set("ip", SrsJsonAny::str(ip.c_str()));
    
    return worker->execute(new SrsKafkaMessage(this, key, obj));
}

srs_error_t SrsKafkaProducer::on_close(int key)
{
    srs_error_t err = srs_success;
    
    if (!enabled) {
        return err;
    }
    
    SrsJsonObject* obj = SrsJsonAny::object();
    
    obj->set("msg", SrsJsonAny::str("close"));
    
    return worker->execute(new SrsKafkaMessage(this, key, obj));
}

#define SRS_KAKFA_CIMS 3000

srs_error_t SrsKafkaProducer::cycle()
{
    srs_error_t err = srs_success;
    
    // wait for the metadata expired.
    // when metadata is ok, wait for it expired.
    if (metadata_ok) {
        srs_cond_wait(metadata_expired);
    }
    
    // request to lock to acquire the socket.
    SrsLocker(lock);
    
    while (true) {
        if ((err = do_cycle()) != srs_success) {
            srs_warn("KafkaProducer: Ignore error, %s", srs_error_desc(err).c_str());
            srs_freep(err);
        }
        
        if ((err = trd->pull()) != srs_success) {
            return srs_error_wrap(err, "kafka cycle");
        }
    
        srs_usleep(SRS_KAKFA_CIMS * 1000);
    }
    
    return err;
}

void SrsKafkaProducer::clear_metadata()
{
    vector<SrsKafkaPartition*>::iterator it;
    
    for (it = partitions.begin(); it != partitions.end(); ++it) {
        SrsKafkaPartition* partition = *it;
        srs_freep(partition);
    }
    
    partitions.clear();
}

srs_error_t SrsKafkaProducer::do_cycle()
{
    srs_error_t err = srs_success;
    
    // ignore when disabled.
    if (!enabled) {
        return err;
    }
    
    // when kafka enabled, request metadata when startup.
    if ((err = request_metadata()) != srs_success) {
        return srs_error_wrap(err, "request metadata");
    }
    
    return err;
}

srs_error_t SrsKafkaProducer::request_metadata()
{
    srs_error_t err = srs_success;
    
    // ignore when disabled.
    if (!enabled) {
        return err;
    }
    
    // select one broker to connect to.
    SrsConfDirective* brokers = _srs_config->get_kafka_brokers();
    if (!brokers) {
        srs_warn("ignore for empty brokers.");
        return err;
    }
    
    std::string server;
    int port = SRS_CONSTS_KAFKA_DEFAULT_PORT;
    if (true) {
        srs_assert(!brokers->args.empty());
        std::string broker = lb->select(brokers->args);
        srs_parse_endpoint(broker, server, port);
    }
    
    std::string topic = _srs_config->get_kafka_topic();
    if (true) {
        std::string senabled = srs_bool2switch(enabled);
        std::string sbrokers = srs_join_vector_string(brokers->args, ",");
        srs_trace("kafka request enabled:%s, brokers:%s, current:[%d]%s:%d, topic:%s",
                  senabled.c_str(), sbrokers.c_str(), lb->current(), server.c_str(), port, topic.c_str());
    }
    
    SrsTcpClient* transport = new SrsTcpClient(server, port, SRS_CONSTS_KAFKA_TMMS);
    SrsAutoFree(SrsTcpClient, transport);
    
    SrsKafkaClient* kafka = new SrsKafkaClient(transport);
    SrsAutoFree(SrsKafkaClient, kafka);
    
    // reconnect to kafka server.
    if ((err = transport->connect()) != srs_success) {
        return srs_error_wrap(err, "connect %s:%d failed", server.c_str(), port);
    }
    
    // do fetch medata from broker.
    SrsKafkaTopicMetadataResponse* metadata = NULL;
    if ((err = kafka->fetch_metadata(topic, &metadata)) != srs_success) {
        return srs_error_wrap(err, "fetch metadata");
    }
    SrsAutoFree(SrsKafkaTopicMetadataResponse, metadata);
    
    // we may need to request multiple times.
    // for example, the first time to create a none-exists topic, then query metadata.
    if (!metadata->metadatas.empty()) {
        SrsKafkaTopicMetadata* topic = metadata->metadatas.at(0);
        if (topic->metadatas.empty()) {
            srs_warn("topic %s metadata empty, retry.", topic->name.to_str().c_str());
            return err;
        }
    }
    
    // show kafka metadata.
    string summary = srs_kafka_metadata_summary(metadata);
    srs_trace("kafka metadata: %s", summary.c_str());
    
    // generate the partition info.
    srs_kafka_metadata2connector(topic, metadata, partitions);
    srs_trace("kafka connector: %s", srs_kafka_summary_partitions(partitions).c_str());
    
    // update the total partition for cache.
    cache->nb_partitions = (int)partitions.size();
    
    metadata_ok = true;
    
    return err;
}

void SrsKafkaProducer::refresh_metadata()
{
    clear_metadata();
    
    metadata_ok = false;
    srs_cond_signal(metadata_expired);
    srs_trace("kafka async refresh metadata in background");
}

srs_error_t SrsKafkaProducer::flush()
{
    srs_error_t err = srs_success;
    
    // flush all available partition caches.
    while (true) {
        int key = -1;
        SrsKafkaPartitionCache* pc = NULL;
        
        // all flushed, or no kafka partition to write to.
        if (!cache->fetch(&key, &pc) || partitions.empty()) {
            break;
        }
        
        // flush specified partition.
        srs_assert(key >= 0 && pc);
        SrsKafkaPartition* partition = partitions.at(key % partitions.size());
        if ((err = cache->flush(partition, key, pc)) != srs_success) {
            return srs_error_wrap(err, "flush partition");
        }
    }
    
    return err;
}

#endif

