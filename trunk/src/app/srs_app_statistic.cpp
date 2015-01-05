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

#include <srs_app_statistic.hpp>

#include <srs_protocol_rtmp.hpp>

SrsStreamInfo::SrsStreamInfo()
{
    _req = NULL;
}

SrsStreamInfo::~SrsStreamInfo()
{
    srs_freep(_req);
}

SrsStatistic* SrsStatistic::_instance = NULL;

SrsStatistic::SrsStatistic()
{
}

SrsStatistic::~SrsStatistic()
{
    std::map<void*, SrsStreamInfo*>::iterator it;
    for (it = pool.begin(); it != pool.end(); it++) {
        SrsStreamInfo* si = it->second;
        srs_freep(si);
    }
}

SrsStatistic* SrsStatistic::instance()
{
    if (_instance == NULL) {
        _instance = new SrsStatistic();
    }
    return _instance;
}

std::map<void*, SrsStreamInfo*>* SrsStatistic::get_pool()
{
    return &pool;
}

SrsStreamInfo* SrsStatistic::get(void *p)
{
    std::map<void*, SrsStreamInfo*>::iterator it = pool.find(p);
    if (it == pool.end()) {
        SrsStreamInfo* si = new SrsStreamInfo();
        pool[p] = si;
        return si;
    } else {
        SrsStreamInfo* si = it->second;
        return si;
    }
}

void SrsStatistic::add_request_info(void *p, SrsRequest *req)
{
    SrsStreamInfo* info = get(p);
    if (info->_req == NULL) {
        info->_req = req->copy();
    }
}