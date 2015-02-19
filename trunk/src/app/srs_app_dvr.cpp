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

#include <srs_app_dvr.hpp>

#ifdef SRS_AUTO_DVR

#include <fcntl.h>
#include <sstream>
#include <sys/time.h>
using namespace std;

#include <srs_app_config.hpp>
#include <srs_rtmp_sdk.hpp>
#include <srs_core_autofree.hpp>
#include <srs_kernel_utility.hpp>
#include <srs_app_http_hooks.hpp>
#include <srs_kernel_codec.hpp>
#include <srs_kernel_flv.hpp>
#include <srs_kernel_file.hpp>

SrsFlvSegment::SrsFlvSegment()
{
    path = "";
    has_keyframe = false;
    duration = 0;
    starttime = -1;
    stream_starttime = 0;
    stream_previous_pkt_time = -1;
    stream_duration = 0;
}

SrsFlvSegment::~SrsFlvSegment()
{
}

void SrsFlvSegment::reset()
{
    has_keyframe = false;
    starttime = -1;
    duration = 0;
}

SrsDvrPlan::SrsDvrPlan()
{
    _source = NULL;
    _req = NULL;
    jitter = NULL;
    dvr_enabled = false;
    fs = new SrsFileWriter();
    enc = new SrsFlvEncoder();
    segment = new SrsFlvSegment();
    jitter_algorithm = SrsRtmpJitterAlgorithmOFF;
    
    _srs_config->subscribe(this);
}

SrsDvrPlan::~SrsDvrPlan()
{
    _srs_config->unsubscribe(this);
    
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
    
    jitter_algorithm = (SrsRtmpJitterAlgorithm)_srs_config->get_dvr_time_jitter(_req->vhost);

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
    
    // the path in config, for example, 
    //      /data/[vhost]/[app]/[stream]/[2006]/[01]/[02]/[15].[04].[05].[999].flv
    std::string path_config = _srs_config->get_dvr_path(req->vhost);
    
    // add [stream].[timestamp].flv as filename for dir
    if (path_config.find(".flv") != path_config.length() - 4) {
        path_config += "/[stream].[timestamp].flv";
    }
    
    // the flv file path
    std::string path = path_config;
    
    // variable [vhost]
    path = srs_string_replace(path, "[vhost]", req->vhost);
    // variable [app]
    path = srs_string_replace(path, "[app]", req->app);
    // variable [stream]
    path = srs_string_replace(path, "[stream]", req->stream);
    
    // date and time substitude
    // clock time
    timeval tv;
    if (gettimeofday(&tv, NULL) == -1) {
        return ERROR_SYSTEM_TIME;
    }
    
    // to calendar time
    struct tm* tm;
    if ((tm = localtime(&tv.tv_sec)) == NULL) {
        return ERROR_SYSTEM_TIME;
    }
    
    // the buffer to format the date and time.
    char buf[64];
    
    // [2006], replace with current year.
    if (true) {
        snprintf(buf, sizeof(buf), "%d", 1900 + tm->tm_year);
        path = srs_string_replace(path, "[2006]", buf);
    }
    // [2006], replace with current year.
    if (true) {
        snprintf(buf, sizeof(buf), "%d", 1900 + tm->tm_year);
        path = srs_string_replace(path, "[2006]", buf);
    }
    // [01], replace this const to current month.
    if (true) {
        snprintf(buf, sizeof(buf), "%d", 1 + tm->tm_mon);
        path = srs_string_replace(path, "[01]", buf);
    }
    // [02], replace this const to current date.
    if (true) {
        snprintf(buf, sizeof(buf), "%d", tm->tm_mday);
        path = srs_string_replace(path, "[02]", buf);
    }
    // [15], replace this const to current hour.
    if (true) {
        snprintf(buf, sizeof(buf), "%d", tm->tm_hour);
        path = srs_string_replace(path, "[15]", buf);
    }
    // [04], repleace this const to current minute.
    if (true) {
        snprintf(buf, sizeof(buf), "%d", tm->tm_min);
        path = srs_string_replace(path, "[04]", buf);
    }
    // [05], repleace this const to current second.
    if (true) {
        snprintf(buf, sizeof(buf), "%d", tm->tm_sec);
        path = srs_string_replace(path, "[05]", buf);
    }
    // [999], repleace this const to current millisecond.
    if (true) {
        snprintf(buf, sizeof(buf), "%03d", (int)(tv.tv_usec / 1000));
        path = srs_string_replace(path, "[999]", buf);
    }
    // [timestamp],replace this const to current UNIX timestamp in ms.
    if (true) {
        int64_t now_us = ((int64_t)tv.tv_sec) * 1000 * 1000 + (int64_t)tv.tv_usec;
        snprintf(buf, sizeof(buf), "%"PRId64, now_us / 1000);
        path = srs_string_replace(path, "[timestamp]", buf);
    }
    
    // create dir first.
    std::string dir = path.substr(0, path.rfind("/"));
    if ((ret = srs_create_dir_recursively(dir)) != ERROR_SUCCESS) {
        srs_error("create dir=%s failed. ret=%d", dir.c_str(), ret);
        return ret;
    }
    srs_info("create dir=%s ok", dir.c_str());
    
    if ((ret = flv_open(req->get_stream_url(), path)) != ERROR_SUCCESS) {
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
    
    if ((ret = enc->write_metadata(18, payload, size)) != ERROR_SUCCESS) {
        return ret;
    }
    
    return ret;
}

int SrsDvrPlan::on_audio(SrsSharedPtrMessage* __audio)
{
    int ret = ERROR_SUCCESS;
    
    if (!dvr_enabled) {
        return ret;
    }

    SrsSharedPtrMessage* audio = __audio->copy();
    SrsAutoFree(SrsSharedPtrMessage, audio);
    
    if ((jitter->correct(audio, 0, 0, jitter_algorithm)) != ERROR_SUCCESS) {
        return ret;
    }
    
    char* payload = audio->payload;
    int size = audio->size;
    int64_t timestamp = filter_timestamp(audio->timestamp);
    if ((ret = enc->write_audio(timestamp, payload, size)) != ERROR_SUCCESS) {
        return ret;
    }
    
    if ((ret = update_duration(audio)) != ERROR_SUCCESS) {
        return ret;
    }
    
    return ret;
}

int SrsDvrPlan::on_video(SrsSharedPtrMessage* __video)
{
    int ret = ERROR_SUCCESS;
    
    if (!dvr_enabled) {
        return ret;
    }

    SrsSharedPtrMessage* video = __video->copy();
    SrsAutoFree(SrsSharedPtrMessage, video);
    
    char* payload = video->payload;
    int size = video->size;
    
#ifdef SRS_AUTO_HTTP_CALLBACK
    bool is_key_frame = SrsFlvCodec::video_is_h264(payload, size) 
        && SrsFlvCodec::video_is_keyframe(payload, size) 
        && !SrsFlvCodec::video_is_sequence_header(payload, size);
    if (is_key_frame) {
        segment->has_keyframe = true;
        if ((ret = on_video_keyframe()) != ERROR_SUCCESS) {
            return ret;
        }
    }
    srs_verbose("dvr video is key: %d", is_key_frame);
#endif
    
    if ((jitter->correct(video, 0, 0, jitter_algorithm)) != ERROR_SUCCESS) {
        return ret;
    }
    
    // update segment duration, session plan just update the duration,
    // the segment plan will reap segment if exceed, this video will write to next segment.
    if ((ret = update_duration(video)) != ERROR_SUCCESS) {
        return ret;
    }
    
    int32_t timestamp = filter_timestamp(video->timestamp);
    if ((ret = enc->write_video(timestamp, payload, size)) != ERROR_SUCCESS) {
        return ret;
    }
    
    return ret;
}

int SrsDvrPlan::on_reload_vhost_dvr(std::string /*vhost*/)
{
    int ret = ERROR_SUCCESS;
    
    jitter_algorithm = (SrsRtmpJitterAlgorithm)_srs_config->get_dvr_time_jitter(_req->vhost);
    
    return ret;
}

int SrsDvrPlan::flv_open(string stream, string path)
{
    int ret = ERROR_SUCCESS;
    
    segment->reset();

    if (srs_path_exists(path)) {
        // when path exists, always append to it.
        // so we must use the target flv path as output flv.
        tmp_flv_file = path;
    } else {
        // when path not exists, dvr to tmp file.
        tmp_flv_file = path + ".tmp";
    }
    
    if (srs_path_exists(path)) {
        if ((ret = fs->open_append(tmp_flv_file)) != ERROR_SUCCESS) {
            srs_error("append file stream for file %s failed. ret=%d", path.c_str(), ret);
            return ret;
        }
        srs_trace("dvr: always append to when exists, file=%s.", path.c_str());
    } else {
        if ((ret = fs->open(tmp_flv_file)) != ERROR_SUCCESS) {
            srs_error("open file stream for file %s failed. ret=%d", path.c_str(), ret);
            return ret;
        }
    }
    
    if ((ret = enc->initialize(fs)) != ERROR_SUCCESS) {
        srs_error("initialize enc by fs for file %s failed. ret=%d", path.c_str(), ret);
        return ret;
    }
    
    // when exists, donot write flv header.
    if (tmp_flv_file != path) {
        if ((ret = write_flv_header()) != ERROR_SUCCESS) {
            return ret;
        }
    }
    
    segment->path = path;
    
    srs_trace("dvr stream %s to file %s", stream.c_str(), path.c_str());
    return ret;
}

int SrsDvrPlan::flv_close()
{
    int ret = ERROR_SUCCESS;
    
    fs->close();
    
    // when tmp flv file exists, reap it.
    if (tmp_flv_file != segment->path) {
        if (rename(tmp_flv_file.c_str(), segment->path.c_str()) < 0) {
            ret = ERROR_SYSTEM_FILE_RENAME;
            srs_error("rename flv file failed, %s => %s. ret=%d", 
                tmp_flv_file.c_str(), segment->path.c_str(), ret);
            return ret;
        }
    }
    
#ifdef SRS_AUTO_HTTP_CALLBACK
    SrsRequest* req = _req;
    if (_srs_config->get_vhost_http_hooks_enabled(req->vhost)) {
        // HTTP: on_dvr 
        SrsConfDirective* on_dvr = _srs_config->get_vhost_on_dvr(req->vhost);
        if (!on_dvr) {
            srs_info("ignore the empty http callback: on_dvr");
            return ret;
        }
        
        int connection_id = _srs_context->get_id();
        std::string ip = req->ip;
        std::string cwd = _srs_config->cwd();
        std::string file = segment->path;
        for (int i = 0; i < (int)on_dvr->args.size(); i++) {
            std::string url = on_dvr->args.at(i);
            if ((ret = SrsHttpHooks::on_dvr(url, connection_id, ip, req, cwd, file)) != ERROR_SUCCESS) {
                srs_error("hook client on_dvr failed. url=%s, ret=%d", url.c_str(), ret);
                return ret;
            }
        }
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
        segment->starttime = msg->timestamp;
    }
    
    // no previous packet or timestamp overflow.
    if (segment->stream_previous_pkt_time < 0 || segment->stream_previous_pkt_time > msg->timestamp) {
        segment->stream_previous_pkt_time = msg->timestamp;
    }
    
    // collect segment and stream duration, timestamp overflow is ok.
    segment->duration += msg->timestamp - segment->stream_previous_pkt_time;
    segment->stream_duration += msg->timestamp - segment->stream_previous_pkt_time;
    
    // update previous packet time
    segment->stream_previous_pkt_time = msg->timestamp;
    
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

SrsDvrPlan* SrsDvrPlan::create_plan(string vhost)
{
    std::string plan = _srs_config->get_dvr_plan(vhost);
    if (plan == SRS_CONF_DEFAULT_DVR_PLAN_SEGMENT) {
        return new SrsDvrSegmentPlan();
    } else if (plan == SRS_CONF_DEFAULT_DVR_PLAN_SESSION) {
        return new SrsDvrSessionPlan();
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
    sh_video = sh_audio = NULL;
}

SrsDvrSegmentPlan::~SrsDvrSegmentPlan()
{
    srs_freep(sh_video);
    srs_freep(sh_audio);
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

int SrsDvrSegmentPlan::on_audio(SrsSharedPtrMessage* audio)
{
    if (SrsFlvCodec::audio_is_sequence_header(audio->payload, audio->size)) {
        srs_freep(sh_audio);
        sh_audio = audio->copy();
    }
    
    return SrsDvrPlan::on_audio(audio);
}

int SrsDvrSegmentPlan::on_video(SrsSharedPtrMessage* video)
{
    if (SrsFlvCodec::video_is_sequence_header(video->payload, video->size)) {
        srs_freep(sh_video);
        sh_video = video->copy();
    }
    
    return SrsDvrPlan::on_video(video);
}

int SrsDvrSegmentPlan::update_duration(SrsSharedPtrMessage* msg)
{
    int ret = ERROR_SUCCESS;
    
    if ((ret = SrsDvrPlan::update_duration(msg)) != ERROR_SUCCESS) {
        return ret;
    }
    
    srs_assert(segment);
    
    // ignore if duration ok.
    if (segment_duration <= 0 || segment->duration < segment_duration) {
        return ret;
    }
    
    // when wait keyframe, ignore if no frame arrived.
    // @see https://github.com/winlinvip/simple-rtmp-server/issues/177
    if (_srs_config->get_dvr_wait_keyframe(_req->vhost)) {
        if (!msg->is_video()) {
            return ret;
        }
        
        char* payload = msg->payload;
        int size = msg->size;
        bool is_key_frame = SrsFlvCodec::video_is_h264(payload, size) 
            && SrsFlvCodec::video_is_keyframe(payload, size) 
            && !SrsFlvCodec::video_is_sequence_header(payload, size);
        if (!is_key_frame) {
            return ret;
        }
    }
    
    // reap segment
    if ((ret = flv_close()) != ERROR_SUCCESS) {
        segment->reset();
        return ret;
    }
    on_unpublish();
    
    // open new flv file
    if ((ret = open_new_segment()) != ERROR_SUCCESS) {
        return ret;
    }
    
    // update sequence header
    if (sh_video && (ret = SrsDvrPlan::on_video(sh_video)) != ERROR_SUCCESS) {
        return ret;
    }
    if (sh_audio && (ret = SrsDvrPlan::on_audio(sh_audio)) != ERROR_SUCCESS) {
        return ret;
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

int SrsDvr::on_audio(SrsSharedPtrMessage* __audio)
{
    return plan->on_audio(__audio);
}

int SrsDvr::on_video(SrsSharedPtrMessage* __video)
{
    return plan->on_video(__video);
}

#endif


