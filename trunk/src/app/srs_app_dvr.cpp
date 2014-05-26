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

#include <srs_app_dvr.hpp>

#ifdef SRS_AUTO_DVR

#include <fcntl.h>
#include <sstream>
using namespace std;

#include <srs_app_config.hpp>
#include <srs_kernel_error.hpp>
#include <srs_protocol_rtmp.hpp>
#include <srs_protocol_rtmp_stack.hpp>
#include <srs_app_source.hpp>
#include <srs_core_autofree.hpp>
#include <srs_kernel_stream.hpp>
#include <srs_kernel_utility.hpp>
#include <srs_app_http_hooks.hpp>
#include <srs_app_codec.hpp>
#include <srs_app_flv.hpp>

SrsFlvSegment::SrsFlvSegment()
{
    path = "";
    has_keyframe = false;
    duration = 0;
    starttime = -1;
    sequence_header_offset = 0;
    stream_starttime = 0;
    stream_previous_pkt_time = -1;
    stream_duration = 0;
}

void SrsFlvSegment::reset()
{
    has_keyframe = false;
    starttime = -1;
    duration = 0;
    sequence_header_offset = 0;
}

SrsDvrPlan::SrsDvrPlan()
{
    _source = NULL;
    _req = NULL;
    jitter = NULL;
    dvr_enabled = false;
    fs = new SrsFileStream();
    enc = new SrsFlvEncoder();
    segment = new SrsFlvSegment();
}

SrsDvrPlan::~SrsDvrPlan()
{
    srs_freep(jitter);
    srs_freep(fs);
    srs_freep(enc);
    srs_freep(segment);
}

int SrsDvrPlan::initialize(SrsSource* source, SrsRequest* req)
{
    int ret = ERROR_SUCCESS;
    
    _source = source;
    _req = req;

    return ret;
}

int SrsDvrPlan::on_publish()
{
    int ret = ERROR_SUCCESS;
    
    // support multiple publish.
    if (dvr_enabled) {
        return ret;
    }
    
    SrsRequest* req = _req;

    if (!_srs_config->get_dvr_enabled(req->vhost)) {
        return ret;
    }
    
    // jitter when publish, ensure whole stream start from 0.
    srs_freep(jitter);
    jitter = new SrsRtmpJitter();
    
    // always update time cache.
    srs_update_system_time_ms();
    
    // when republish, stream starting.
    segment->stream_previous_pkt_time = -1;
    segment->stream_starttime = srs_get_system_time_ms();
    segment->stream_duration = 0;
    
    if ((ret = open_new_segment()) != ERROR_SUCCESS) {
        return ret;
    }
    
    return ret;
}

int SrsDvrPlan::open_new_segment()
{
    int ret = ERROR_SUCCESS;
    
    SrsRequest* req = _req;
    
    // new flv file
    std::stringstream path;
    
    path << _srs_config->get_dvr_path(req->vhost)
        << "/" << req->app << "/" 
        << req->stream << "." << srs_get_system_time_ms() << ".flv";
    
    if ((ret = flv_open(req->get_stream_url(), path.str())) != ERROR_SUCCESS) {
        return ret;
    }
    dvr_enabled = true;
    
    return ret;
}

int SrsDvrPlan::on_dvr_request_sh()
{
    int ret = ERROR_SUCCESS;
    
    // the dvr is enabled, notice the source to push the data.
    if ((ret = _source->on_dvr_request_sh()) != ERROR_SUCCESS) {
        return ret;
    }
    
    return ret;
}

int SrsDvrPlan::on_video_keyframe()
{
    int ret = ERROR_SUCCESS;
    return ret;
}

int64_t SrsDvrPlan::filter_timestamp(int64_t timestamp)
{
    return timestamp;
}

int SrsDvrPlan::on_meta_data(SrsOnMetaDataPacket* metadata)
{
    int ret = ERROR_SUCCESS;
    
    if (!dvr_enabled) {
        return ret;
    }
    
    int size = 0;
    char* payload = NULL;
    if ((ret = metadata->encode(size, payload)) != ERROR_SUCCESS) {
        return ret;
    }
    SrsAutoFree(char, payload);
    
    if ((ret = enc->write_metadata(payload, size)) != ERROR_SUCCESS) {
        return ret;
    }
    
    return ret;
}

int SrsDvrPlan::on_audio(SrsSharedPtrMessage* audio)
{
    int ret = ERROR_SUCCESS;
    
    if (!dvr_enabled) {
        return ret;
    }
    
    if ((jitter->correct(audio, 0, 0)) != ERROR_SUCCESS) {
        return ret;
    }
    
    char* payload = (char*)audio->payload;
    int size = (int)audio->size;
    int64_t timestamp = filter_timestamp(audio->header.timestamp);
    if ((ret = enc->write_audio(timestamp, payload, size)) != ERROR_SUCCESS) {
        return ret;
    }
    
    if ((ret = update_duration(audio)) != ERROR_SUCCESS) {
        return ret;
    }
    
    return ret;
}

int SrsDvrPlan::on_video(SrsSharedPtrMessage* video)
{
    int ret = ERROR_SUCCESS;
    
    if (!dvr_enabled) {
        return ret;
    }
    
    char* payload = (char*)video->payload;
    int size = (int)video->size;
    
#ifdef SRS_AUTO_HTTP_CALLBACK
    bool is_key_frame = SrsCodec::video_is_h264((int8_t*)payload, size) 
        && SrsCodec::video_is_keyframe((int8_t*)payload, size) 
        && !SrsCodec::video_is_sequence_header((int8_t*)payload, size);
    if (is_key_frame) {
        segment->has_keyframe = true;
        if ((ret = on_video_keyframe()) != ERROR_SUCCESS) {
            return ret;
        }
    }
    srs_verbose("dvr video is key: %d", is_key_frame);
#endif
    
    if ((jitter->correct(video, 0, 0)) != ERROR_SUCCESS) {
        return ret;
    }
    
    int32_t timestamp = filter_timestamp(video->header.timestamp);
    if ((ret = enc->write_video(timestamp, payload, size)) != ERROR_SUCCESS) {
        return ret;
    }
    
    if ((ret = update_duration(video)) != ERROR_SUCCESS) {
        return ret;
    }
    
    return ret;
}

int SrsDvrPlan::flv_open(string stream, string path)
{
    int ret = ERROR_SUCCESS;
    
    segment->reset();
    
    std::string tmp_file = path + ".tmp";
    if ((ret = fs->open_write(tmp_file)) != ERROR_SUCCESS) {
        srs_error("open file stream for file %s failed. ret=%d", path.c_str(), ret);
        return ret;
    }
    
    if ((ret = enc->initialize(fs)) != ERROR_SUCCESS) {
        srs_error("initialize enc by fs for file %s failed. ret=%d", path.c_str(), ret);
        return ret;
    }
    
    if ((ret = write_flv_header()) != ERROR_SUCCESS) {
        return ret;
    }
    
    segment->path = path;
    
    srs_trace("dvr stream %s to file %s", stream.c_str(), path.c_str());
    return ret;
}

int SrsDvrPlan::flv_close()
{
    int ret = ERROR_SUCCESS;
    
    fs->close();
    
    std::string tmp_file = segment->path + ".tmp";
    if (rename(tmp_file.c_str(), segment->path.c_str()) < 0) {
        ret = ERROR_SYSTEM_FILE_RENAME;
        srs_error("rename flv file failed, %s => %s. ret=%d", 
            tmp_file.c_str(), segment->path.c_str(), ret);
        return ret;
    }
    
#ifdef SRS_AUTO_HTTP_CALLBACK
    if ((ret = on_dvr_hss_reap_flv()) != ERROR_SUCCESS) {
        return ret;
    }
#endif
    
    return ret;
}

int SrsDvrPlan::update_duration(SrsSharedPtrMessage* msg)
{
    int ret = ERROR_SUCCESS;

    // we must assumpt that the stream timestamp is monotonically increase,
    // that is, always use time jitter to correct the timestamp.
    
    // set the segment starttime at first time
    if (segment->starttime < 0) {
        segment->starttime = msg->header.timestamp;
    }
    
    // no previous packet or timestamp overflow.
    if (segment->stream_previous_pkt_time < 0 || segment->stream_previous_pkt_time > msg->header.timestamp) {
        segment->stream_previous_pkt_time = msg->header.timestamp;
    }
    
    // collect segment and stream duration, timestamp overflow is ok.
    segment->duration += msg->header.timestamp - segment->stream_previous_pkt_time;
    segment->stream_duration += msg->header.timestamp - segment->stream_previous_pkt_time;
    
    // update previous packet time
    segment->stream_previous_pkt_time = msg->header.timestamp;
    
    return ret;
}

int SrsDvrPlan::write_flv_header()
{
    int ret = ERROR_SUCCESS;
    
    if ((ret = enc->write_header()) != ERROR_SUCCESS) {
        srs_error("write flv header failed. ret=%d", ret);
        return ret;
    }
    
    return ret;
}

int SrsDvrPlan::on_dvr_hss_reap_flv()
{
    int ret = ERROR_SUCCESS;
    
#ifdef SRS_AUTO_HTTP_CALLBACK
    // HTTP: on_dvr_hss_reap_flv 
    SrsConfDirective* on_dvr_hss_reap_flv = _srs_config->get_vhost_on_dvr_hss_reap_flv(_req->vhost);
    if (!on_dvr_hss_reap_flv) {
        srs_info("ignore the empty http callback: on_dvr_hss_reap_flv");
        return ret;
    }
    
    for (int i = 0; i < (int)on_dvr_hss_reap_flv->args.size(); i++) {
        std::string url = on_dvr_hss_reap_flv->args.at(i);
        SrsHttpHooks::on_dvr_hss_reap_flv(url, _req, segment);
    }
#endif
    
    return ret;
}

SrsDvrPlan* SrsDvrPlan::create_plan(string vhost)
{
    std::string plan = _srs_config->get_dvr_plan(vhost);
    if (plan == SRS_CONF_DEFAULT_DVR_PLAN_SEGMENT) {
        return new SrsDvrSegmentPlan();
    } else if (plan == SRS_CONF_DEFAULT_DVR_PLAN_SESSION) {
        return new SrsDvrSessionPlan();
    } else if (plan == SRS_CONF_DEFAULT_DVR_PLAN_HSS) {
        return new SrsDvrHssPlan();
    } else {
        return new SrsDvrSessionPlan();
    }
}

SrsDvrSessionPlan::SrsDvrSessionPlan()
{
}

SrsDvrSessionPlan::~SrsDvrSessionPlan()
{
}

void SrsDvrSessionPlan::on_unpublish()
{
    // support multiple publish.
    if (!dvr_enabled) {
        return;
    }
    
    // ignore error.
    int ret = flv_close();
    if (ret != ERROR_SUCCESS) {
        srs_warn("ignore flv close error. ret=%d", ret);
    }
    
    dvr_enabled = false;
}

SrsDvrSegmentPlan::SrsDvrSegmentPlan()
{
    segment_duration = -1;
}

SrsDvrSegmentPlan::~SrsDvrSegmentPlan()
{
}

int SrsDvrSegmentPlan::initialize(SrsSource* source, SrsRequest* req)
{
    int ret = ERROR_SUCCESS;
    
    if ((ret = SrsDvrPlan::initialize(source, req)) != ERROR_SUCCESS) {
        return ret;
    }
    
    segment_duration = _srs_config->get_dvr_duration(req->vhost);
    // to ms
    segment_duration *= 1000;
    
    return ret;
}

int SrsDvrSegmentPlan::on_publish()
{
    int ret = ERROR_SUCCESS;
    
    // if already opened, continue to dvr.
    // the segment plan maybe keep running longer than the encoder.
    // for example, segment running, encoder restart,
    // the segment plan will just continue going and donot open new segment.
    if (fs->is_open()) {
        dvr_enabled = true;
        return ret;
    }
    
    return SrsDvrPlan::on_publish();
}

void SrsDvrSegmentPlan::on_unpublish()
{
    // support multiple publish.
    if (!dvr_enabled) {
        return;
    }
    dvr_enabled = false;
}

int SrsDvrSegmentPlan::update_duration(SrsSharedPtrMessage* msg)
{
    int ret = ERROR_SUCCESS;
    
    if ((ret = SrsDvrPlan::update_duration(msg)) != ERROR_SUCCESS) {
        return ret;
    }
    
    srs_assert(segment);
    
    // reap if exceed duration.
    if (segment_duration > 0 && segment->duration > segment_duration) {
        if ((ret = flv_close()) != ERROR_SUCCESS) {
            segment->reset();
            return ret;
        }
        on_unpublish();
        
        if ((ret = open_new_segment()) != ERROR_SUCCESS) {
            return ret;
        }
    }
    
    return ret;
}

SrsDvrHssPlan::SrsDvrHssPlan()
{
    segment_duration = -1;
    expect_reap_time = 0;
}

SrsDvrHssPlan::~SrsDvrHssPlan()
{
}

int SrsDvrHssPlan::initialize(SrsSource* source, SrsRequest* req)
{
    int ret = ERROR_SUCCESS;
    
    if ((ret = SrsDvrPlan::initialize(source, req)) != ERROR_SUCCESS) {
        return ret;
    }
    
    // TODO: FIXME: support reload
    segment_duration = _srs_config->get_dvr_duration(req->vhost);
    // to ms
    segment_duration *= 1000;
    
    return ret;
}

int SrsDvrHssPlan::on_publish()
{
    int ret = ERROR_SUCCESS;
    
    // if already opened, continue to dvr.
    // the segment plan maybe keep running longer than the encoder.
    // for example, segment running, encoder restart,
    // the segment plan will just continue going and donot open new segment.
    if (fs->is_open()) {
        dvr_enabled = true;
        return ret;
    }
    
    if ((ret = SrsDvrPlan::on_publish()) != ERROR_SUCCESS) {
        return ret;
    }
    
    // expect reap flv time
    expect_reap_time = segment->stream_starttime + segment_duration;
    
    return ret;
}

void SrsDvrHssPlan::on_unpublish()
{
    // support multiple publish.
    if (!dvr_enabled) {
        return;
    }
    dvr_enabled = false;
}

int SrsDvrHssPlan::on_meta_data(SrsOnMetaDataPacket* metadata)
{
    int ret = ERROR_SUCCESS;
    
    SrsRequest* req = _req;
    
    // new flv file
    std::stringstream path;
    path << _srs_config->get_dvr_path(req->vhost)
        << "/" << req->app << "/" 
        << req->stream << ".header.flv";
        
    SrsFileStream fs;
    if ((ret = fs.open_write(path.str().c_str())) != ERROR_SUCCESS) {
        return ret;
    }
    
    SrsFlvEncoder enc;
    if ((ret = enc.initialize(&fs)) != ERROR_SUCCESS) {
        return ret;
    }
    
    if ((ret = enc.write_header()) != ERROR_SUCCESS) {
        return ret;
    }
    
    int size = 0;
    char* payload = NULL;
    if ((ret = metadata->encode(size, payload)) != ERROR_SUCCESS) {
        return ret;
    }
    SrsAutoFree(char, payload);
    
    if ((ret = enc.write_metadata(payload, size)) != ERROR_SUCCESS) {
        return ret;
    }
    
#ifdef SRS_AUTO_HTTP_CALLBACK
    if ((ret = on_dvr_hss_reap_flv_header(path.str())) != ERROR_SUCCESS) {
        return ret;
    }
#endif
    
    return ret;
}

int SrsDvrHssPlan::write_flv_header()
{
    int ret = ERROR_SUCCESS;
    return ret;
}

int SrsDvrHssPlan::on_dvr_request_sh()
{
    int ret = ERROR_SUCCESS;
    return ret;
}

int SrsDvrHssPlan::on_video_keyframe()
{
    int ret = ERROR_SUCCESS;
    
    segment->sequence_header_offset = fs->tellg();
    if ((ret = SrsDvrPlan::on_dvr_request_sh()) != ERROR_SUCCESS) {
        return ret;
    }
    
    return ret;
}

int64_t SrsDvrHssPlan::filter_timestamp(int64_t timestamp)
{
    //return timestamp;
    srs_assert(segment);
    srs_verbose("filter timestamp from %"PRId64" to %"PRId64, timestamp, segment->stream_starttime + timestamp);
    return segment->stream_starttime + timestamp;
}

int SrsDvrHssPlan::on_dvr_hss_reap_flv_header(string path)
{
    int ret = ERROR_SUCCESS;
    
#ifdef SRS_AUTO_HTTP_CALLBACK
    // HTTP: on_dvr_hss_reap_flv_header 
    SrsConfDirective* on_dvr_hss_reap_flv = _srs_config->get_vhost_on_dvr_hss_reap_flv(_req->vhost);
    if (!on_dvr_hss_reap_flv) {
        srs_info("ignore the empty http callback: on_dvr_hss_reap_flv");
        return ret;
    }
    
    for (int i = 0; i < (int)on_dvr_hss_reap_flv->args.size(); i++) {
        std::string url = on_dvr_hss_reap_flv->args.at(i);
        SrsHttpHooks::on_dvr_hss_reap_flv_header(url, _req, path);
    }
#endif
    
    return ret;
}

int SrsDvrHssPlan::update_duration(SrsSharedPtrMessage* msg)
{
    int ret = ERROR_SUCCESS;
    
    if ((ret = SrsDvrPlan::update_duration(msg)) != ERROR_SUCCESS) {
        return ret;
    }
    
    srs_assert(segment);
    
    // if not initialized, ignore reap.
    if (expect_reap_time <= 0 
        || segment->stream_starttime <= 0 
        || segment->stream_duration <= 0
    ) {
        return ret;
    }
    
    // reap if exceed atc expect time.
    if (segment->stream_starttime + segment->stream_duration > expect_reap_time) {
        srs_verbose("hss reap start=%"PRId64", duration=%"PRId64", expect=%"PRId64
            ", segment(start=%"PRId64", duration=%"PRId64", file=%s", 
            segment->stream_starttime, segment->stream_duration, expect_reap_time,
            segment->stream_starttime + segment->starttime, 
            segment->duration, segment->path.c_str());
            
        // update expect reap time
        expect_reap_time += segment_duration;
        
        if ((ret = flv_close()) != ERROR_SUCCESS) {
            segment->reset();
            return ret;
        }
        on_unpublish();
        
        if ((ret = open_new_segment()) != ERROR_SUCCESS) {
            return ret;
        }
    }
    
    return ret;
}

SrsDvr::SrsDvr(SrsSource* source)
{
    _source = source;
    plan = NULL;
}

SrsDvr::~SrsDvr()
{
    srs_freep(plan);
}

int SrsDvr::initialize(SrsRequest* req)
{
    int ret = ERROR_SUCCESS;
    
    srs_freep(plan);
    plan = SrsDvrPlan::create_plan(req->vhost);

    if ((ret = plan->initialize(_source, req)) != ERROR_SUCCESS) {
        return ret;
    }
    
    return ret;
}

int SrsDvr::on_publish(SrsRequest* /*req*/)
{
    int ret = ERROR_SUCCESS;
    
    if ((ret = plan->on_publish()) != ERROR_SUCCESS) {
        return ret;
    }
    
    return ret;
}

void SrsDvr::on_unpublish()
{
    plan->on_unpublish();
}

int SrsDvr::on_meta_data(SrsOnMetaDataPacket* metadata)
{
    int ret = ERROR_SUCCESS;
    
    if ((ret = plan->on_meta_data(metadata)) != ERROR_SUCCESS) {
        return ret;
    }
    
    return ret;
}

int SrsDvr::on_audio(SrsSharedPtrMessage* audio)
{
    int ret = ERROR_SUCCESS;
    
    SrsAutoFree(SrsSharedPtrMessage, audio);
    
    if ((ret = plan->on_audio(audio)) != ERROR_SUCCESS) {
        return ret;
    }
    
    return ret;
}

int SrsDvr::on_video(SrsSharedPtrMessage* video)
{
    int ret = ERROR_SUCCESS;
    
    SrsAutoFree(SrsSharedPtrMessage, video);
    
    if ((ret = plan->on_video(video)) != ERROR_SUCCESS) {
        return ret;
    }
    
    return ret;
}

#endif

