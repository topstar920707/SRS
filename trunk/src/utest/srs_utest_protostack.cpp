/*
The MIT License (MIT)

Copyright (c) 2013-2019 Winlin

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
#include <srs_utest_protostack.hpp>

using namespace std;

#include <srs_kernel_error.hpp>
#include <srs_core_autofree.hpp>
#include <srs_protocol_utility.hpp>
#include <srs_rtmp_msg_array.hpp>
#include <srs_rtmp_stack.hpp>
#include <srs_kernel_utility.hpp>
#include <srs_app_st.hpp>
#include <srs_protocol_amf0.hpp>
#include <srs_rtmp_stack.hpp>
#include <srs_service_http_conn.hpp>
#include <srs_kernel_buffer.hpp>

class MockErrorPacket : public SrsPacket
{
protected:
    virtual int get_size() {
        return 1024;
    }
};

VOID TEST(ProtoStackTest, PacketEncode)
{
    srs_error_t err;

    int size;
    char* payload;

    if (true) {
        MockErrorPacket pkt;
        HELPER_EXPECT_FAILED(pkt.encode(size, payload));
    }

    if (true) {
        MockErrorPacket pkt;
        SrsBuffer b;
        HELPER_EXPECT_FAILED(pkt.decode(&b));
    }

    if (true) {
        SrsPacket pkt;
        EXPECT_EQ(0, pkt.get_prefer_cid());
        EXPECT_EQ(0, pkt.get_message_type());
        EXPECT_EQ(0, pkt.get_size());
    }

    if (true) {
        MockErrorPacket pkt;
        EXPECT_EQ(1024, pkt.get_size());
    }
}

