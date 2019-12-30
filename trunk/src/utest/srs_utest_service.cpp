/*
The MIT License (MIT)

Copyright (c) 2013-2020 Winlin

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
#include <srs_utest_service.hpp>

using namespace std;

#include <srs_kernel_error.hpp>
#include <srs_app_listener.hpp>
#include <srs_service_st.hpp>
#include <srs_service_utility.hpp>

// Disable coroutine test for OSX.
#if !defined(SRS_OSX)

#include <srs_service_st.hpp>

VOID TEST(ServiceTimeTest, TimeUnit)
{
    EXPECT_EQ(1000, SRS_UTIME_MILLISECONDS);
    EXPECT_EQ(1000*1000, SRS_UTIME_SECONDS);
    EXPECT_EQ(60*1000*1000, SRS_UTIME_MINUTES);
    EXPECT_EQ(3600*1000*1000LL, SRS_UTIME_HOURS);

    EXPECT_TRUE(srs_is_never_timeout(SRS_UTIME_NO_TIMEOUT));
    EXPECT_FALSE(srs_is_never_timeout(0));
}

class MockTcpHandler : public ISrsTcpHandler
{
private:
	srs_netfd_t fd;
public:
	MockTcpHandler();
	virtual ~MockTcpHandler();
public:
    virtual srs_error_t on_tcp_client(srs_netfd_t stfd);
};

MockTcpHandler::MockTcpHandler()
{
	fd = NULL;
}

MockTcpHandler::~MockTcpHandler()
{
	srs_close_stfd(fd);
}

srs_error_t MockTcpHandler::on_tcp_client(srs_netfd_t stfd)
{
	fd = stfd;
	return srs_success;
}

VOID TEST(TCPServerTest, PingPong)
{
	srs_error_t err;
	if (true) {
		MockTcpHandler h;
		SrsTcpListener l(&h, _srs_tmp_host, _srs_tmp_port);

		HELPER_EXPECT_SUCCESS(l.listen());
		EXPECT_TRUE(l.fd() > 0);
	}

	if (true) {
		MockTcpHandler h;
		SrsTcpListener l(&h, _srs_tmp_host, _srs_tmp_port);
		HELPER_EXPECT_SUCCESS(l.listen());

		SrsTcpClient c(_srs_tmp_host, _srs_tmp_port, _srs_tmp_timeout);
		HELPER_EXPECT_SUCCESS(c.connect());

		EXPECT_TRUE(h.fd != NULL);
	}

	if (true) {
		MockTcpHandler h;
		SrsTcpListener l(&h, _srs_tmp_host, _srs_tmp_port);
		HELPER_EXPECT_SUCCESS(l.listen());

		SrsTcpClient c(_srs_tmp_host, _srs_tmp_port, _srs_tmp_timeout);
		HELPER_EXPECT_SUCCESS(c.connect());

		SrsStSocket skt;
		ASSERT_TRUE(h.fd != NULL);
		HELPER_EXPECT_SUCCESS(skt.initialize(h.fd));

		HELPER_EXPECT_SUCCESS(c.write((void*)"Hello", 5, NULL));

		char buf[16] = {0};
		HELPER_EXPECT_SUCCESS(skt.read(buf, 5, NULL));
		EXPECT_STREQ(buf, "Hello");
	}

	if (true) {
		MockTcpHandler h;
		SrsTcpListener l(&h, _srs_tmp_host, _srs_tmp_port);
		HELPER_EXPECT_SUCCESS(l.listen());

		SrsTcpClient c(_srs_tmp_host, _srs_tmp_port, _srs_tmp_timeout);
		HELPER_EXPECT_SUCCESS(c.connect());

		SrsStSocket skt;
		ASSERT_TRUE(h.fd != NULL);
		HELPER_EXPECT_SUCCESS(skt.initialize(h.fd));

		HELPER_EXPECT_SUCCESS(c.write((void*)"Hello", 5, NULL));
		HELPER_EXPECT_SUCCESS(c.write((void*)" ", 1, NULL));
		HELPER_EXPECT_SUCCESS(c.write((void*)"SRS", 3, NULL));

		char buf[16] = {0};
		HELPER_EXPECT_SUCCESS(skt.read(buf, 9, NULL));
		EXPECT_STREQ(buf, "Hello SRS");
	}

	if (true) {
		MockTcpHandler h;
		SrsTcpListener l(&h, _srs_tmp_host, _srs_tmp_port);
		HELPER_EXPECT_SUCCESS(l.listen());

		SrsTcpClient c(_srs_tmp_host, _srs_tmp_port, _srs_tmp_timeout);
		HELPER_EXPECT_SUCCESS(c.connect());

		SrsStSocket skt;
		ASSERT_TRUE(h.fd != NULL);
		HELPER_EXPECT_SUCCESS(skt.initialize(h.fd));

		HELPER_EXPECT_SUCCESS(c.write((void*)"Hello SRS", 9, NULL));
		EXPECT_EQ(9, c.get_send_bytes());
		EXPECT_EQ(0, c.get_recv_bytes());
		EXPECT_TRUE(SRS_UTIME_NO_TIMEOUT == c.get_send_timeout());
		EXPECT_TRUE(SRS_UTIME_NO_TIMEOUT == c.get_recv_timeout());

		char buf[16] = {0};
		HELPER_EXPECT_SUCCESS(skt.read(buf, 9, NULL));
		EXPECT_STREQ(buf, "Hello SRS");
		EXPECT_EQ(0, skt.get_send_bytes());
		EXPECT_EQ(9, skt.get_recv_bytes());
		EXPECT_TRUE(SRS_UTIME_NO_TIMEOUT == skt.get_send_timeout());
		EXPECT_TRUE(SRS_UTIME_NO_TIMEOUT == skt.get_recv_timeout());
	}
}

VOID TEST(TCPServerTest, PingPongWithTimeout)
{
	srs_error_t err;

	if (true) {
		MockTcpHandler h;
		SrsTcpListener l(&h, _srs_tmp_host, _srs_tmp_port);
		HELPER_EXPECT_SUCCESS(l.listen());

		SrsTcpClient c(_srs_tmp_host, _srs_tmp_port, _srs_tmp_timeout);
		HELPER_EXPECT_SUCCESS(c.connect());

		SrsStSocket skt;
		ASSERT_TRUE(h.fd != NULL);
		HELPER_EXPECT_SUCCESS(skt.initialize(h.fd));
		skt.set_recv_timeout(1 * SRS_UTIME_MILLISECONDS);

		char buf[16] = {0};
		HELPER_EXPECT_FAILED(skt.read(buf, 9, NULL));
		EXPECT_TRUE(SRS_UTIME_NO_TIMEOUT == skt.get_send_timeout());
		EXPECT_TRUE(1*SRS_UTIME_MILLISECONDS == skt.get_recv_timeout());
	}

	if (true) {
		MockTcpHandler h;
		SrsTcpListener l(&h, _srs_tmp_host, _srs_tmp_port);
		HELPER_EXPECT_SUCCESS(l.listen());

		SrsTcpClient c(_srs_tmp_host, _srs_tmp_port, _srs_tmp_timeout);
		HELPER_EXPECT_SUCCESS(c.connect());

		SrsStSocket skt;
		ASSERT_TRUE(h.fd != NULL);
		HELPER_EXPECT_SUCCESS(skt.initialize(h.fd));
		skt.set_recv_timeout(1 * SRS_UTIME_MILLISECONDS);

		char buf[16] = {0};
		HELPER_EXPECT_FAILED(skt.read_fully(buf, 9, NULL));
		EXPECT_TRUE(SRS_UTIME_NO_TIMEOUT == skt.get_send_timeout());
		EXPECT_TRUE(1*SRS_UTIME_MILLISECONDS == skt.get_recv_timeout());
	}

	if (true) {
		MockTcpHandler h;
		SrsTcpListener l(&h, _srs_tmp_host, _srs_tmp_port);
		HELPER_EXPECT_SUCCESS(l.listen());

		SrsTcpClient c(_srs_tmp_host, _srs_tmp_port, _srs_tmp_timeout);
		HELPER_EXPECT_SUCCESS(c.connect());

		SrsStSocket skt;
		ASSERT_TRUE(h.fd != NULL);
		HELPER_EXPECT_SUCCESS(skt.initialize(h.fd));
		skt.set_recv_timeout(1 * SRS_UTIME_MILLISECONDS);

		HELPER_EXPECT_SUCCESS(c.write((void*)"Hello", 5, NULL));

		char buf[16] = {0};
		HELPER_EXPECT_FAILED(skt.read_fully(buf, 9, NULL));
		EXPECT_TRUE(SRS_UTIME_NO_TIMEOUT == skt.get_send_timeout());
		EXPECT_TRUE(1*SRS_UTIME_MILLISECONDS == skt.get_recv_timeout());
	}
}

VOID TEST(TCPServerTest, StringIsDigital)
{
    EXPECT_EQ(0, ::atoi("0"));
    EXPECT_EQ(0, ::atoi("0000000000"));
    EXPECT_EQ(1, ::atoi("01"));
    EXPECT_EQ(12, ::atoi("012"));
    EXPECT_EQ(1234567890L, ::atol("1234567890"));
    EXPECT_EQ(123456789L, ::atol("0123456789"));
    EXPECT_EQ(1234567890, ::atoi("1234567890a"));
    EXPECT_EQ(10, ::atoi("10e3"));
    EXPECT_EQ(0, ::atoi("!1234567890"));
    EXPECT_EQ(0, ::atoi(""));

    EXPECT_TRUE(srs_is_digit_number("0"));
    EXPECT_TRUE(srs_is_digit_number("0000000000"));
    EXPECT_TRUE(srs_is_digit_number("1234567890"));
    EXPECT_TRUE(srs_is_digit_number("0123456789"));
    EXPECT_FALSE(srs_is_digit_number("1234567890a"));
    EXPECT_FALSE(srs_is_digit_number("a1234567890"));
    EXPECT_FALSE(srs_is_digit_number("10e3"));
    EXPECT_FALSE(srs_is_digit_number("!1234567890"));
    EXPECT_FALSE(srs_is_digit_number(""));
}

VOID TEST(TCPServerTest, StringIsHex)
{
    if (true) {
        char* str = (char*)"0";
        char* parsed = str; errno = 0;
        EXPECT_EQ(0x0, ::strtol(str, &parsed, 16));
        EXPECT_EQ(0, errno);
        EXPECT_EQ(str + 1, parsed);
    }

    if (true) {
        char* str = (char*)"0";
        char* parsed = str; errno = 0;
        EXPECT_EQ(0x0, ::strtol(str, &parsed, 16));
        EXPECT_EQ(0, errno);
        EXPECT_EQ(str + 1, parsed);
    }

    if (true) {
        char* str = (char*)"0000000000";
        char* parsed = str; errno = 0;
        EXPECT_EQ(0x0, ::strtol(str, &parsed, 16));
        EXPECT_EQ(0, errno);
        EXPECT_EQ(str + 10, parsed);
    }

    if (true) {
        char* str = (char*)"01";
        char* parsed = str; errno = 0;
        EXPECT_EQ(0x1, ::strtol(str, &parsed, 16));
        EXPECT_EQ(0, errno);
        EXPECT_EQ(str + 2, parsed);
    }

    if (true) {
        char* str = (char*)"012";
        char* parsed = str; errno = 0;
        EXPECT_EQ(0x12, ::strtol(str, &parsed, 16));
        EXPECT_EQ(0, errno);
        EXPECT_EQ(str + 3, parsed);
    }

    if (true) {
        char* str = (char*)"1234567890";
        char* parsed = str; errno = 0;
        EXPECT_EQ(0x1234567890L, ::strtol(str, &parsed, 16));
        EXPECT_EQ(0, errno);
        EXPECT_EQ(str + 10, parsed);
    }

    if (true) {
        char* str = (char*)"0123456789";
        char* parsed = str; errno = 0;
        EXPECT_EQ(0x123456789L, ::strtol(str, &parsed, 16));
        EXPECT_EQ(0, errno);
        EXPECT_EQ(str + 10, parsed);
    }

    if (true) {
        char* str = (char*)"1234567890a";
        char* parsed = str; errno = 0;
        EXPECT_EQ(0x1234567890a, ::strtol(str, &parsed, 16));
        EXPECT_EQ(0, errno);
        EXPECT_EQ(str + 11, parsed);
    }

    if (true) {
        char* str = (char*)"0x1234567890a";
        char* parsed = str; errno = 0;
        EXPECT_EQ(0x1234567890a, ::strtol(str, &parsed, 16));
        EXPECT_EQ(0, errno);
        EXPECT_EQ(str + 13, parsed);
    }

    if (true) {
        char* str = (char*)"1234567890f";
        char* parsed = str; errno = 0;
        EXPECT_EQ(0x1234567890f, ::strtol(str, &parsed, 16));
        EXPECT_EQ(0, errno);
        EXPECT_EQ(str + 11, parsed);
    }

    if (true) {
        char* str = (char*)"10e3";
        char* parsed = str; errno = 0;
        EXPECT_EQ(0x10e3, ::strtol(str, &parsed, 16));
        EXPECT_EQ(0, errno);
        EXPECT_EQ(str + 4, parsed);
    }

    if (true) {
        char* str = (char*)"!1234567890";
        char* parsed = str; errno = 0;
        EXPECT_EQ(0x0, ::strtol(str, &parsed, 16));
        EXPECT_EQ(0, errno);
        EXPECT_EQ(str, parsed);
    }

    if (true) {
        char* str = (char*)"1234567890g";
        char* parsed = str; errno = 0;
        EXPECT_EQ(0x1234567890, ::strtol(str, &parsed, 16));
        EXPECT_EQ(0, errno);
        EXPECT_EQ(str + 10, parsed);
    }

    if (true) {
        char* str = (char*)"";
        char* parsed = str; errno = 0;
        EXPECT_EQ(0x0, ::strtol(str, &parsed, 16));
        EXPECT_EQ(0, errno);
        EXPECT_EQ(str, parsed);
    }

    if (true) {
        char* str = (char*)"1fffffffffffffffffffffffffffff";
        char* parsed = str; errno = 0;
        EXPECT_EQ(0x7fffffffffffffff, ::strtol(str, &parsed, 16));
        EXPECT_NE(0, errno);
        EXPECT_EQ(str+30, parsed);
    }
}

VOID TEST(TCPServerTest, WritevIOVC)
{
	srs_error_t err;
	if (true) {
		MockTcpHandler h;
		SrsTcpListener l(&h, _srs_tmp_host, _srs_tmp_port);
		HELPER_EXPECT_SUCCESS(l.listen());

		SrsTcpClient c(_srs_tmp_host, _srs_tmp_port, _srs_tmp_timeout);
		HELPER_EXPECT_SUCCESS(c.connect());

		SrsStSocket skt;
		ASSERT_TRUE(h.fd != NULL);
		HELPER_EXPECT_SUCCESS(skt.initialize(h.fd));

		iovec iovs[3];
		iovs[0].iov_base = (void*)"H";
		iovs[0].iov_len = 1;
		iovs[1].iov_base = (void*)"e";
		iovs[1].iov_len = 1;
		iovs[2].iov_base = (void*)"llo";
		iovs[2].iov_len = 3;

		HELPER_EXPECT_SUCCESS(c.writev(iovs, 3, NULL));

		char buf[16] = {0};
		HELPER_EXPECT_SUCCESS(skt.read(buf, 5, NULL));
		EXPECT_STREQ(buf, "Hello");
	}

	if (true) {
		MockTcpHandler h;
		SrsTcpListener l(&h, _srs_tmp_host, _srs_tmp_port);
		HELPER_EXPECT_SUCCESS(l.listen());

		SrsTcpClient c(_srs_tmp_host, _srs_tmp_port, _srs_tmp_timeout);
		HELPER_EXPECT_SUCCESS(c.connect());

		SrsStSocket skt;
		ASSERT_TRUE(h.fd != NULL);
		HELPER_EXPECT_SUCCESS(skt.initialize(h.fd));

		iovec iovs[3];
		iovs[0].iov_base = (void*)"H";
		iovs[0].iov_len = 1;
		iovs[1].iov_base = (void*)NULL;
		iovs[1].iov_len = 0;
		iovs[2].iov_base = (void*)"llo";
		iovs[2].iov_len = 3;

		HELPER_EXPECT_SUCCESS(c.writev(iovs, 3, NULL));

		char buf[16] = {0};
		HELPER_EXPECT_SUCCESS(skt.read(buf, 4, NULL));
		EXPECT_STREQ(buf, "Hllo");
	}
}

#endif
