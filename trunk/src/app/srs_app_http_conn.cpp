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

#include <srs_app_http_conn.hpp>

#include <srs_kernel_log.hpp>
#include <srs_kernel_error.hpp>

SrsHttpConn::SrsHttpConn(SrsServer* srs_server, st_netfd_t client_stfd) 
    : SrsConnection(srs_server, client_stfd)
{
}

SrsHttpConn::~SrsHttpConn() 
{
}

int SrsHttpConn::do_cycle() 
{
    int ret = ERROR_SUCCESS;
    
    if ((ret = get_peer_ip()) != ERROR_SUCCESS) {
        srs_error("get peer ip failed. ret=%d", ret);
        return ret;
    }
    srs_trace("http get peer ip success. ip=%s", ip);
    
    char data[] = "HTTP/1.1 200 OK\r\n"
        "Server: SRS/"RTMP_SIG_SRS_VERSION"\r\n"
        "Content-Length: 11\r\n"
        "Content-Type: text/html;charset=utf-8\r\n\r\n"
        "hello http~";
    st_write(stfd, data, sizeof(data), -1);
        
    return ret;
}
