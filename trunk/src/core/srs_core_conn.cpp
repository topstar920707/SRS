/*
The MIT License (MIT)

Copyright (c) 2013 winlin

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

#include <srs_core_conn.hpp>

#include <srs_core_log.hpp>
#include <srs_core_error.hpp>
#include <srs_core_server.hpp>

SrsConnection::SrsConnection(SrsServer* srs_server, st_netfd_t client_stfd)
{
	server = srs_server;
	stfd = client_stfd;
}

SrsConnection::~SrsConnection()
{
}

int SrsConnection::start()
{
	int ret = ERROR_SUCCESS;
    
    if (st_thread_create(cycle_thread, this, 0, 0) == NULL) {
        ret = ERROR_ST_CREATE_CYCLE_THREAD;
        SrsError("st_thread_create conn cycle thread error. ret=%d", ret);
        return ret;
    }
    SrsVerbose("create st conn cycle thread success.");
	
	return ret;
}

int SrsConnection::do_cycle()
{
	int ret = ERROR_SUCCESS;
	return ret;
}

void SrsConnection::cycle()
{
	int ret = ERROR_SUCCESS;
	
	ret = do_cycle();
    
	// success.
	if (ret == ERROR_SUCCESS) {
		SrsInfo("client process normally finished. ret=%d", ret);
	}
	
	// client close peer.
	if (ret == ERROR_SOCKET_CLOSED) {
		SrsTrace("client disconnect peer. ret=%d", ret);
	}
	
	server->remove(this);
}

void* SrsConnection::cycle_thread(void* arg)
{
	SrsConnection* conn = (SrsConnection*)arg;
	SrsAssert(conn != NULL);
	
	conn->cycle();
	
	return NULL;
}

