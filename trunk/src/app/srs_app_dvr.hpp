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

#ifndef SRS_APP_DVR_HPP
#define SRS_APP_DVR_HPP

/*
#include <srs_app_dvr.hpp>
*/
#include <srs_core.hpp>

#include <string>

#ifdef SRS_AUTO_DVR

class SrsSource;
class SrsRequest;
class SrsStream;
class SrsRtmpJitter;
class SrsOnMetaDataPacket;
class SrsSharedPtrMessage;
class SrsFileWriter;
class SrsFlvEncoder;
class SrsDvrPlan;

#include <srs_app_source.hpp>
#include <srs_app_reload.hpp>

/**
* a piece of flv segment.
* when open segment, support start at 0 or not.
*/
class SrsFlvSegment : public ISrsReloadHandler
{
private:
    SrsSource* source;
    SrsRequest* req;
    SrsDvrPlan* plan;
private:
    /**
    * the underlayer dvr stream.
    * if close, the flv is reap and closed.
    * if open, new flv file is crote.
    */
    SrsFlvEncoder* enc;
    SrsRtmpJitter* jitter;
    SrsRtmpJitterAlgorithm jitter_algorithm;
    SrsFileWriter* fs;
private:
    std::string tmp_flv_file;
private:
    /**
    * current segment flv file path.
    */
    std::string path;
    /**
    * whether current segment has keyframe.
    */
    bool has_keyframe;
    /**
    * current segment starttime, RTMP pkt time.
    */
    int64_t starttime;
    /**
    * current segment duration
    */
    int64_t duration;
    /**
    * stream start time, to generate atc pts. abs time.
    */
    int64_t stream_starttime;
    /**
    * stream duration, to generate atc segment.
    */
    int64_t stream_duration;
    /**
    * previous stream RTMP pkt time, used to calc the duration.
    * for the RTMP timestamp will overflow.
    */
    int64_t stream_previous_pkt_time;
public:
    SrsFlvSegment(SrsDvrPlan* p);
    virtual ~SrsFlvSegment();
public:
    /**
    * initialize the segment.
    */
    virtual int initialize(SrsSource* s, SrsRequest* r);
    /**
    * whether segment is overflow.
    */
    virtual bool is_overflow(int64_t max_duration);
    /**
    * open new segment file, timestamp start at 0 for fresh flv file.
    * @remark ignore when already open.
    */
    virtual int open();
    /**
    * close current segment.
    * @remark ignore when already closed.
    */
    virtual int close();
    /**
    * write the metadata to segment.
    */
    virtual int write_metadata(SrsOnMetaDataPacket* metadata);
    /**
    * @param __audio, directly ptr, copy it if need to save it.
    */
    virtual int write_audio(SrsSharedPtrMessage* __audio);
    /**
    * @param __video, directly ptr, copy it if need to save it.
    */
    virtual int write_video(SrsSharedPtrMessage* __video);
private:
    /**
    * generate the flv segment path.
    */
    virtual std::string generate_path();
    /**
    * create flv jitter. load jitter when flv exists.
    * @param loads_from_flv whether loads the jitter from exists flv file.
    */
    virtual int create_jitter(bool loads_from_flv);
    /**
    * when update the duration of segment by rtmp msg.
    */
    virtual int on_update_duration(SrsSharedPtrMessage* msg);
// interface ISrsReloadHandler
public:
    virtual int on_reload_vhost_dvr(std::string vhost);
};

/**
* the plan for dvr.
* use to control the following dvr params:
* 1. filename: the filename for record file.
* 2. reap flv: when to reap the flv and start new piece.
*/
// TODO: FIXME: the plan is too fat, refine me.
class SrsDvrPlan
{
public:
    friend class SrsFlvSegment;
protected:
    SrsSource* source;
    SrsRequest* req;
    SrsFlvSegment* segment;
    bool dvr_enabled;
public:
    SrsDvrPlan();
    virtual ~SrsDvrPlan();
public:
    virtual int initialize(SrsSource* s, SrsRequest* r);
    virtual int on_publish() = 0;
    virtual void on_unpublish() = 0;
    /**
    * when got metadata.
    */
    virtual int on_meta_data(SrsOnMetaDataPacket* metadata);
    /**
    * @param __audio, directly ptr, copy it if need to save it.
    */
    virtual int on_audio(SrsSharedPtrMessage* __audio);
    /**
    * @param __video, directly ptr, copy it if need to save it.
    */
    virtual int on_video(SrsSharedPtrMessage* __video);
protected:
    virtual int on_dvr_request_sh();
    virtual int on_video_keyframe();
    virtual int64_t filter_timestamp(int64_t timestamp);
public:
    static SrsDvrPlan* create_plan(std::string vhost);
};

/**
* session plan: reap flv when session complete(unpublish)
*/
class SrsDvrSessionPlan : public SrsDvrPlan
{
public:
    SrsDvrSessionPlan();
    virtual ~SrsDvrSessionPlan();
public:
    virtual int on_publish();
    virtual void on_unpublish();
};

/**
* segment plan: reap flv when duration exceed.
*/
class SrsDvrSegmentPlan : public SrsDvrPlan
{
private:
    // in config, in ms
    int segment_duration;
    SrsSharedPtrMessage* sh_audio;
    SrsSharedPtrMessage* sh_video;
public:
    SrsDvrSegmentPlan();
    virtual ~SrsDvrSegmentPlan();
public:
    virtual int initialize(SrsSource* source, SrsRequest* req);
    virtual int on_publish();
    virtual void on_unpublish();
    /**
    * @param audio, directly ptr, copy it if need to save it.
    */
    virtual int on_audio(SrsSharedPtrMessage* audio);
    /**
    * @param video, directly ptr, copy it if need to save it.
    */
    virtual int on_video(SrsSharedPtrMessage* video);
private:
    virtual int update_duration(SrsSharedPtrMessage* msg);
};

/**
* dvr(digital video recorder) to record RTMP stream to flv file.
* TODO: FIXME: add utest for it.
*/
class SrsDvr
{
private:
    SrsSource* source;
private:
    SrsDvrPlan* plan;
public:
    SrsDvr(SrsSource* s);
    virtual ~SrsDvr();
public:
    /**
    * initialize dvr, create dvr plan.
    * when system initialize(encoder publish at first time, or reload),
    * initialize the dvr will reinitialize the plan, the whole dvr framework.
    */
    virtual int initialize(SrsRequest* r);
    /**
    * publish stream event, 
    * when encoder start to publish RTMP stream.
    */
    virtual int on_publish(SrsRequest* r);
    /**
    * the unpublish event.,
    * when encoder stop(unpublish) to publish RTMP stream.
    */
    virtual void on_unpublish();
    /**
    * get some information from metadata, it's optinal.
    */
    virtual int on_meta_data(SrsOnMetaDataPacket* m);
    /**
    * mux the audio packets to dvr.
    * @param __audio, directly ptr, copy it if need to save it.
    */
    virtual int on_audio(SrsSharedPtrMessage* __audio);
    /**
    * mux the video packets to dvr.
    * @param __video, directly ptr, copy it if need to save it.
    */
    virtual int on_video(SrsSharedPtrMessage* __video);
};

#endif

#endif

