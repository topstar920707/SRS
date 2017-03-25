/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2013-2017 OSSRS(winlin)
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

#define SRS_KAKFA_CIMS 3000
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

int SrsKafkaPartition::connect()
{
    int ret = ERROR_SUCCESS;
    
    if (transport) {
        return ret;
    }
    transport = new SrsTcpClient(host, port, SRS_KAFKA_PRODUCER_TIMEOUT);
    kafka = new SrsKafkaClient(transport);
    
    if ((ret = transport->connect()) != ERROR_SUCCESS) {
        disconnect();
        srs_error("connect to %s partition=%d failed. ret=%d", hostport().c_str(), id, ret);
        return ret;
    }
    
    srs_trace("connect at %s, partition=%d, broker=%d", hostport().c_str(), id, broker);
    
    return ret;
}

int SrsKafkaPartition::flush(SrsKafkaPartitionCache* pc)
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

int SrsKafkaMessage::call()
{
    int ret = producer->send(key, obj);
    
    // the obj is manged by producer now.
    obj = NULL;
    
    return ret;
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

int SrsKafkaCache::flush(SrsKafkaPartition* partition, int key, SrsKafkaPartitionCache* pc)
{
    int ret = ERROR_SUCCESS;
    
    // ensure the key exists.
    srs_assert (cache.find(key) != cache.end());
    
    // the cache is vector, which is continous store.
    // we remember the messages we have written and clear it when completed.
    int nb_msgs = (int)pc->size();
    if (pc->empty()) {
        return ret;
    }
    
    // connect transport.
    if ((ret = partition->connect()) != ERROR_SUCCESS) {
        srs_error("kafka connect to partition failed. ret=%d", ret);
        return ret;
    }
    
    // write the json objects.
    if ((ret = partition->flush(pc)) != ERROR_SUCCESS) {
        srs_error("kafka write messages failed. ret=%d", ret);
        return ret;
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
    
    return ret;
}

ISrsKafkaCluster::ISrsKafkaCluster()
{
}

ISrsKafkaCluster::~ISrsKafkaCluster()
{
}

// @global kafka event producer, user must use srs_initialize_kafka to initialize it.
ISrsKafkaCluster* _srs_kafka = NULL;

int srs_initialize_kafka()
{
    int ret = ERROR_SUCCESS;
    
    SrsKafkaProducer* kafka = new SrsKafkaProducer();
    _srs_kafka = kafka;
    
    if ((ret = kafka->initialize()) != ERROR_SUCCESS) {
        srs_error("initialize the kafka producer failed. ret=%d", ret);
        return ret;
    }
    
    if ((ret = kafka->start()) != ERROR_SUCCESS) {
        srs_error("start kafka failed. ret=%d", ret);
        return ret;
    }
    
    return ret;
}

void srs_dispose_kafka()
{
    SrsKafkaProducer* kafka = dynamic_cast<SrsKafkaProducer*>(_srs_kafka);
    kafka->stop();
    
    srs_freep(kafka);
    _srs_kafka = NULL;
}

SrsKafkaProducer::SrsKafkaProducer()
{
    metadata_ok = false;
    metadata_expired = st_cond_new();
    
    lock = st_mutex_new();
    pthread = new SrsReusableThread("kafka", this, SRS_KAKFA_CIMS);
    worker = new SrsAsyncCallWorker();
    cache = new SrsKafkaCache();
    
    lb = new SrsLbRoundRobin();
}

SrsKafkaProducer::~SrsKafkaProducer()
{
    clear_metadata();
    
    srs_freep(lb);
    
    srs_freep(worker);
    srs_freep(pthread);
    srs_freep(cache);
    
    st_mutex_destroy(lock);
    st_cond_destroy(metadata_expired);
}

int SrsKafkaProducer::initialize()
{
    int ret = ERROR_SUCCESS;
    
    enabled = _srs_config->get_kafka_enabled();
    srs_info("initialize kafka ok, enabled=%d.", enabled);
    
    return ret;
}

int SrsKafkaProducer::start()
{
    int ret = ERROR_SUCCESS;
    
    if (!enabled) {
        return ret;
    }
    
    if ((ret = worker->start()) != ERROR_SUCCESS) {
        srs_error("start kafka worker failed. ret=%d", ret);
        return ret;
    }
    
    if ((ret = pthread->start()) != ERROR_SUCCESS) {
        srs_error("start kafka thread failed. ret=%d", ret);
    }
    
    refresh_metadata();
    
    return ret;
}

void SrsKafkaProducer::stop()
{
    if (!enabled) {
        return;
    }
    
    pthread->stop();
    worker->stop();
}

int SrsKafkaProducer::send(int key, SrsJsonObject* obj)
{
    int ret = ERROR_SUCCESS;
    
    // cache the json object.
    cache->append(key, obj);
    
    // too few messages, ignore.
    if (cache->size() < SRS_KAFKA_PRODUCER_AGGREGATE_SIZE) {
        return ret;
    }
    
    // too many messages, warn user.
    if (cache->size() > SRS_KAFKA_PRODUCER_AGGREGATE_SIZE * 10) {
        srs_warn("kafka cache too many messages: %d", cache->size());
    }
    
    // sync with backgound metadata worker.
    st_mutex_lock(lock);
    
    // flush message when metadata is ok.
    if (metadata_ok) {
        ret = flush();
    }
    
    st_mutex_unlock(lock);
    
    return ret;
}

int SrsKafkaProducer::on_client(int key, SrsListenerType type, string ip)
{
    int ret = ERROR_SUCCESS;
    
    if (!enabled) {
        return ret;
    }
    
    SrsJsonObject* obj = SrsJsonAny::object();
    
    obj->set("msg", SrsJsonAny::str("accept"));
    obj->set("type", SrsJsonAny::integer(type));
    obj->set("ip", SrsJsonAny::str(ip.c_str()));
    
    return worker->execute(new SrsKafkaMessage(this, key, obj));
}

int SrsKafkaProducer::on_close(int key)
{
    int ret = ERROR_SUCCESS;
    
    if (!enabled) {
        return ret;
    }
    
    SrsJsonObject* obj = SrsJsonAny::object();
    
    obj->set("msg", SrsJsonAny::str("close"));
    
    return worker->execute(new SrsKafkaMessage(this, key, obj));
}

int SrsKafkaProducer::cycle()
{
    int ret = ERROR_SUCCESS;
    
    if ((ret = do_cycle()) != ERROR_SUCCESS) {
        srs_warn("ignore kafka error. ret=%d", ret);
    }
    
    return ret;
}

int SrsKafkaProducer::on_before_cycle()
{
    // wait for the metadata expired.
    // when metadata is ok, wait for it expired.
    if (metadata_ok) {
        st_cond_wait(metadata_expired);
    }
    
    // request to lock to acquire the socket.
    st_mutex_lock(lock);
    
    return ERROR_SUCCESS;
}

int SrsKafkaProducer::on_end_cycle()
{
    st_mutex_unlock(lock);
    
    return ERROR_SUCCESS;
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

int SrsKafkaProducer::do_cycle()
{
    int ret = ERROR_SUCCESS;
    
    // ignore when disabled.
    if (!enabled) {
        return ret;
    }
    
    // when kafka enabled, request metadata when startup.
    if ((ret = request_metadata()) != ERROR_SUCCESS) {
        srs_error("request kafka metadata failed. ret=%d", ret);
        return ret;
    }
    
    return ret;
}

int SrsKafkaProducer::request_metadata()
{
    int ret = ERROR_SUCCESS;
    
    // ignore when disabled.
    if (!enabled) {
        return ret;
    }
    
    // select one broker to connect to.
    SrsConfDirective* brokers = _srs_config->get_kafka_brokers();
    if (!brokers) {
        srs_warn("ignore for empty brokers.");
        return ret;
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
    if ((ret = transport->connect()) != ERROR_SUCCESS) {
        srs_error("kafka connect %s:%d failed. ret=%d", server.c_str(), port, ret);
        return ret;
    }
    
    // do fetch medata from broker.
    SrsKafkaTopicMetadataResponse* metadata = NULL;
    if ((ret = kafka->fetch_metadata(topic, &metadata)) != ERROR_SUCCESS) {
        srs_error("kafka fetch metadata failed. ret=%d", ret);
        return ret;
    }
    SrsAutoFree(SrsKafkaTopicMetadataResponse, metadata);
    
    // we may need to request multiple times.
    // for example, the first time to create a none-exists topic, then query metadata.
    if (!metadata->metadatas.empty()) {
        SrsKafkaTopicMetadata* topic = metadata->metadatas.at(0);
        if (topic->metadatas.empty()) {
            srs_warn("topic %s metadata empty, retry.", topic->name.to_str().c_str());
            return ret;
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
    
    return ret;
}

void SrsKafkaProducer::refresh_metadata()
{
    clear_metadata();
    
    metadata_ok = false;
    st_cond_signal(metadata_expired);
    srs_trace("kafka async refresh metadata in background");
}

int SrsKafkaProducer::flush()
{
    int ret = ERROR_SUCCESS;
    
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
        if ((ret = cache->flush(partition, key, pc)) != ERROR_SUCCESS) {
            srs_error("flush partition failed. ret=%d", ret);
            return ret;
        }
    }
    
    return ret;
}

#endif

