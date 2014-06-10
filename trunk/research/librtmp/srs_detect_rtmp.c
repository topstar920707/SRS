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
gcc srs_detect_rtmp.c ../../objs/lib/srs_librtmp.a -g -O0 -lstdc++ -o srs_detect_rtmp
*/

#include <stdio.h>
#include <stdlib.h>

#include "../../objs/include/srs_librtmp.h"

int main(int argc, char** argv)
{
    srs_rtmp_t rtmp;
    
    // time
    int64_t time_startup = srs_get_time_ms();
    int64_t time_dns_resolve = 0;
    int64_t time_socket_connect = 0;
    int64_t time_play_stream = 0;
    int64_t time_first_packet = 0;
    int64_t time_cleanup = 0;
    // delay = actual - expect time when quit.
    int delay = 0;
    
    // packet data
    int type, size;
    u_int32_t timestamp = 0;
    char* data;
    
    // user options
    const char* rtmp_url = NULL;
    int duration = 0;
    int timeout = 0;
    
    if (argc <= 3) {
        printf("detect stream on RTMP server\n"
            "Usage: %s <rtmp_url> <duration> <timeout>\n"
            "   rtmp_url     RTMP stream url to play\n"
            "   duration     how long to play, in seconds, stream time.\n"
            "   timeout      how long to timeout, in seconds, system time.\n"
            "For example:\n"
            "   %s rtmp://127.0.0.1:1935/live/livestream 3 10\n",
            argv[0]);
        int ret = 1;
        exit(ret);
        return ret;
    }
    
    rtmp_url = argv[1];
    duration = atoi(argv[2]);
    timeout = atoi(argv[3]);
    
    printf("detect rtmp stream\n");
    printf("srs(simple-rtmp-server) client librtmp library.\n");
    printf("version: %d.%d.%d\n", srs_version_major(), srs_version_minor(), srs_version_revision());
    printf("rtmp url: %s\n", rtmp_url);
    printf("duration: %ds, timeout:%ds\n", duration, timeout);
    
    if (duration <= 0 || timeout <= 0) {
        printf("duration and timeout must be positive.\n");
        exit(1);
        return 1;
    }
    
    rtmp = srs_rtmp_create(rtmp_url);
    
    if (__srs_dns_resolve(rtmp) != 0) {
        printf("dns resolve failed.\n");
        goto rtmp_destroy;
    }
    printf("dns resolve success\n");
    time_dns_resolve = srs_get_time_ms();
    
    if (__srs_connect_server(rtmp) != 0) {
        printf("socket connect failed.\n");
        goto rtmp_destroy;
    }
    printf("socket connect success\n");
    time_socket_connect = srs_get_time_ms();
    
    if (__srs_do_simple_handshake(rtmp) != 0) {
        printf("do simple handshake failed.\n");
        goto rtmp_destroy;
    }
    printf("do simple handshake success\n");
    
    if (srs_connect_app(rtmp) != 0) {
        printf("connect vhost/app failed.\n");
        goto rtmp_destroy;
    }
    printf("connect vhost/app success\n");
    
    if (srs_play_stream(rtmp) != 0) {
        printf("play stream failed.\n");
        goto rtmp_destroy;
    }
    printf("play stream success\n");
    time_play_stream = srs_get_time_ms();
    
    for (;;) {
        if (srs_read_packet(rtmp, &type, &timestamp, &data, &size) != 0) {
            goto rtmp_destroy;
        }
        printf("got packet: type=%s, time=%d, size=%d\n", srs_type2string(type), timestamp, size);
        
        if (time_first_packet <= 0) {
            time_first_packet = srs_get_time_ms();
        }
        
        free(data);
        
        if (srs_get_time_ms() - time_startup > timeout * 1000) {
            printf("timeout, terminate.\n");
            goto rtmp_destroy;
        }
        
        if (timestamp > duration * 1000) {
            printf("duration exceed, terminate.\n");
            goto rtmp_destroy;
        }
    }
    
rtmp_destroy:
    srs_rtmp_destroy(rtmp);
    time_cleanup = srs_get_time_ms();
    
    // print result to stderr.
    fprintf(stderr, "{"
        "\"%s\":%d, " //#1
        "\"%s\":%d, " // #2
        "\"%s\":%d, " // #3
        "\"%s\":%d, " // #4
        "\"%s\":%d, " // #5
        "\"%s\":%d, " // #6
        "\"%s\":%d, " // #7
        "%s}",
        // total = dns + tcp_connect + start_play + first_packet + last_packet
        "total", (int)(time_cleanup - time_startup), //#1
        "dns", (int)(time_dns_resolve - time_startup), //#2
        "tcp_connect", (int)(time_socket_connect - time_dns_resolve), //#3
        "start_play", (int)(time_play_stream - time_socket_connect), //#4
        "first_packet", (int)(time_first_packet - time_play_stream), //#5
        "last_packet", (int)(time_cleanup - time_first_packet), //#6
        // expect = time_cleanup - time_first_packet
        // actual = timestamp
        // delay = actual - expect
        "delay", (int)(timestamp - (time_cleanup - time_first_packet)), //#7
        // unit in ms.
        "\"unit\": \"ms\""
    );
    printf("\n");
    
    return 0;
}
