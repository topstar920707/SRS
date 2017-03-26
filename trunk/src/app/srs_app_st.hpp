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

#ifndef SRS_APP_ST_HPP
#define SRS_APP_ST_HPP

#include <srs_core.hpp>

#include <string>

#include <srs_service_st.hpp>
#include <srs_protocol_io.hpp>

// the internal classes, user should never use it.
// user should use the public classes at the bellow:
// @see SrsEndlessThread, SrsOneCycleThread, SrsReusableThread
namespace internal
{
    /**
     * the handler for the thread, callback interface.
     * the thread model defines as:
     *     handler->on_thread_start()
     *     while loop:
     *        handler->on_before_cycle()
     *        handler->cycle()
     *        handler->on_end_cycle()
     *        if !loop then break for user stop thread.
     *        sleep(CycleIntervalMilliseconds)
     *     handler->on_thread_stop()
     * when stop, the thread will interrupt the st_thread,
     * which will cause the socket to return error and
     * terminate the cycle thread.
     *
     * @remark why should check can_loop() in cycle method?
     *       when thread interrupt, the socket maybe not got EINT,
     *       espectially on st_usleep(), so the cycle must check the loop,
     *       when handler->cycle() has loop itself, for example:
     *               while (true):
     *                   if (read_from_socket(skt) < 0) break;
     *       if thread stop when read_from_socket, it's ok, the loop will break,
     *       but when thread stop interrupt the s_usleep(0), then the loop is
     *       death loop.
     *       in a word, the handler->cycle() must:
     *               while (pthread->can_loop()):
     *                   if (read_from_socket(skt) < 0) break;
     *       check the loop, then it works.
     *
     * @remark why should use stop_loop() to terminate thread in itself?
     *       in the thread itself, that is the cycle method,
     *       if itself want to terminate the thread, should never use stop(),
     *       but use stop_loop() to set the loop to false and terminate normally.
     *
     * @remark when should set the interval_us, and when not?
     *       the cycle will invoke util cannot loop, eventhough the return code of cycle is error,
     *       so the interval_us used to sleep for each cycle.
     */
    class ISrsThreadHandler
    {
    public:
        ISrsThreadHandler();
        virtual ~ISrsThreadHandler();
    public:
        virtual void on_thread_start();
        virtual int on_before_cycle();
        virtual int cycle() = 0;
        virtual int on_end_cycle();
        virtual void on_thread_stop();
    };
    
    /**
     * provides servies from st_thread_t,
     * for common thread usage.
     */
    class SrsThread
    {
    private:
        st_thread_t trd;
        int context_id;
        bool loop;
        bool joinable;
        const char* name;
    private:
        ISrsThreadHandler* handler;
        // The cycle interval in ms.
        int64_t cims;
    public:
        /**
         * initialize the thread.
         * @param n, human readable name for st debug.
         * @param h, the cycle handler for the thread.
         * @param ims, the sleep interval in ms when cycle finished.
         * @param j, if joinable, other thread must stop the thread.
         * @remark if joinable, thread never quit itself, or memory leak.
         * @see: https://github.com/ossrs/srs/issues/78
         * @remark about st debug, see st-1.9/README, _st_iterate_threads_flag
         */
        /**
         * TODO: FIXME: maybe all thread must be reap by others threads,
         * @see: https://github.com/ossrs/srs/issues/77
         */
        SrsThread(const char* n, ISrsThreadHandler* h, int64_t ims, bool j);
        virtual ~SrsThread();
    public:
        /**
         * get the context id. @see: ISrsThreadContext.get_id().
         * used for parent thread to get the id.
         * @remark when start thread, parent thread will block and wait for this id ready.
         */
        virtual int cid();
        /**
         * start the thread, invoke the cycle of handler util
         * user stop the thread.
         * @remark ignore any error of cycle of handler.
         * @remark user can start multiple times, ignore if already started.
         * @remark wait for the cid is set by thread pfn.
         */
        virtual int start();
        /**
         * stop the thread, wait for the thread to terminate.
         * @remark user can stop multiple times, ignore if already stopped.
         */
        virtual void stop();
    public:
        /**
         * whether the thread should loop,
         * used for handler->cycle() which has a loop method,
         * to check this method, break if false.
         */
        virtual bool can_loop();
        /**
         * for the loop thread to stop the loop.
         * other thread can directly use stop() to stop loop and wait for quit.
         * this stop loop method only set loop to false.
         */
        virtual void stop_loop();
    private:
        virtual void cycle();
        static void* pfn(void* arg);
    };
}

#endif

