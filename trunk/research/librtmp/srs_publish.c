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
/**
gcc srs_publish.c ../../objs/lib/srs_librtmp.a -g -O0 -lstdc++ -o srs_publish
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../../objs/include/srs_librtmp.h"

int main(int argc, char** argv)
{
    printf("publish rtmp stream to server like FMLE/FFMPEG/Encoder\n");
    printf("srs(simple-rtmp-server) client librtmp library.\n");
    printf("version: %d.%d.%d\n", srs_version_major(), srs_version_minor(), srs_version_revision());
    
    if (argc <= 1) {
        printf("Usage: %s <rtmp_url>\n"
            "   rtmp_url     RTMP stream url to publish\n"
            "For example:\n"
            "   %s rtmp://127.0.0.1:1935/live/livestream\n",
            argv[0], argv[0]);
        exit(-1);
    }
    
    // warn it .
    // @see: https://github.com/winlinvip/simple-rtmp-server/issues/126
    srs_lib_trace("\033[33m%s\033[0m", 
        "[warning] it's only a sample to use librtmp. "
        "please never use it to publish and test forward/transcode/edge/HLS whatever. "
        "you should refer to this tool to use the srs-librtmp to publish the real media stream."
        "read about: https://github.com/winlinvip/simple-rtmp-server/issues/126");
    srs_lib_trace("rtmp url: %s", argv[1]);
    srs_rtmp_t rtmp = srs_rtmp_create(argv[1]);
    
    if (srs_simple_handshake(rtmp) != 0) {
        srs_lib_trace("simple handshake failed.");
        goto rtmp_destroy;
    }
    srs_lib_trace("simple handshake success");
    
    if (srs_connect_app(rtmp) != 0) {
        srs_lib_trace("connect vhost/app failed.");
        goto rtmp_destroy;
    }
    srs_lib_trace("connect vhost/app success");
    
    if (srs_publish_stream(rtmp) != 0) {
        srs_lib_trace("publish stream failed.");
        goto rtmp_destroy;
    }
    srs_lib_trace("publish stream success");
    
    u_int32_t timestamp = 0;
    for (;;) {
        int type = SRS_RTMP_TYPE_VIDEO;
        int size = 4096;
        char* data = (char*)malloc(4096);
        
        timestamp += 40;
        
        if (srs_write_packet(rtmp, type, timestamp, data, size) != 0) {
            goto rtmp_destroy;
        }
        srs_lib_trace("sent packet: type=%s, time=%d, size=%d", 
            srs_type2string(type), timestamp, size);
        
        usleep(40 * 1000);
    }
    
rtmp_destroy:
    srs_rtmp_destroy(rtmp);
    
    return 0;
}
