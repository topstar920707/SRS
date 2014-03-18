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

#include <srs_app_thread.hpp>

#include <srs_kernel_error.hpp>
#include <srs_kernel_log.hpp>

ISrsThreadHandler::ISrsThreadHandler()
{
}

ISrsThreadHandler::~ISrsThreadHandler()
{
}

void ISrsThreadHandler::on_enter_loop()
{
}

int ISrsThreadHandler::on_before_cycle()
{
    int ret = ERROR_SUCCESS;
    return ret;
}

int ISrsThreadHandler::on_end_cycle()
{
    int ret = ERROR_SUCCESS;
    return ret;
}

void ISrsThreadHandler::on_leave_loop()
{
}

SrsThread::SrsThread(ISrsThreadHandler* thread_handler, int64_t interval_us)
{
    handler = thread_handler;
    cycle_interval_us = interval_us;
    
    tid = NULL;
    loop = false;
}

SrsThread::~SrsThread()
{
    stop();
}

int SrsThread::start()
{
    int ret = ERROR_SUCCESS;
    
    if(tid) {
        srs_info("thread already running.");
        return ret;
    }
    
    if((tid = st_thread_create(thread_fun, this, 1, 0)) == NULL){
        ret = ERROR_ST_CREATE_CYCLE_THREAD;
        srs_error("st_thread_create failed. ret=%d", ret);
        return ret;
    }
    
    return ret;
}

void SrsThread::stop()
{
    if (tid) {
        loop = false;
        
        // the interrupt will cause the socket to read/write error,
        // which will terminate the cycle thread.
        st_thread_interrupt(tid);
        
        // wait the thread to exit.
        st_thread_join(tid, NULL);
        
        tid = NULL;
    }
}

bool SrsThread::can_loop()
{
    return loop;
}

void SrsThread::thread_cycle()
{
    int ret = ERROR_SUCCESS;
    
    srs_assert(handler);
    
    _srs_context->generate_id();
    srs_trace("thread cycle start");

    handler->on_end_cycle();
    
    loop = true;
    while (loop) {
        if ((ret = handler->on_before_cycle()) != ERROR_SUCCESS) {
            srs_warn("thread on before cycle failed, ignored and retry, ret=%d", ret);
            goto failed;
        }
        srs_info("thread on before cycle success");
        
        if ((ret = handler->cycle()) != ERROR_SUCCESS) {
            srs_warn("thread cycle failed, ignored and retry, ret=%d", ret);
            goto failed;
        }
        srs_info("thread cycle success");
        
        if ((ret = handler->on_end_cycle()) != ERROR_SUCCESS) {
            srs_warn("thread on end cycle failed, ignored and retry, ret=%d", ret);
            goto failed;
        }
        srs_info("thread on end cycle success");

failed:
        if (!loop) {
            break;
        }
        
        st_usleep(cycle_interval_us);
    }
    
    handler->on_leave_loop();
    srs_trace("thread cycle finished");
}

void* SrsThread::thread_fun(void* arg)
{
    SrsThread* obj = (SrsThread*)arg;
    srs_assert(obj);
    
    obj->thread_cycle();
    
    return NULL;
}