/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2013-2020 Winlin
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

#include <srs_app_listener.hpp>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>
using namespace std;

#include <srs_core_autofree.hpp>
#include <srs_kernel_log.hpp>
#include <srs_kernel_error.hpp>
#include <srs_app_server.hpp>
#include <srs_app_utility.hpp>
#include <srs_kernel_utility.hpp>

// set the max packet size.
#define SRS_UDP_MAX_PACKET_SIZE 65535

// sleep in srs_utime_t for udp recv packet.
#define SrsUdpPacketRecvCycleInterval 0

ISrsUdpHandler::ISrsUdpHandler()
{
}

ISrsUdpHandler::~ISrsUdpHandler()
{
}

srs_error_t ISrsUdpHandler::on_stfd_change(srs_netfd_t /*fd*/)
{
    return srs_success;
}

ISrsUdpMuxHandler::ISrsUdpMuxHandler()
{
}

ISrsUdpMuxHandler::~ISrsUdpMuxHandler()
{
}

srs_error_t ISrsUdpMuxHandler::on_stfd_change(srs_netfd_t /*fd*/)
{
    return srs_success;
}

void ISrsUdpHandler::set_stfd(srs_netfd_t /*fd*/)
{
}

ISrsTcpHandler::ISrsTcpHandler()
{
}

ISrsTcpHandler::~ISrsTcpHandler()
{
}

SrsUdpListener::SrsUdpListener(ISrsUdpHandler* h, string i, int p)
{
    handler = h;
    ip = i;
    port = p;
    lfd = NULL;
    
    nb_buf = SRS_UDP_MAX_PACKET_SIZE;
    buf = new char[nb_buf];
    
    trd = new SrsDummyCoroutine();
}

SrsUdpListener::~SrsUdpListener()
{
    srs_freep(trd);
    srs_close_stfd(lfd);
    srs_freepa(buf);
}

int SrsUdpListener::fd()
{
    return srs_netfd_fileno(lfd);
}

srs_netfd_t SrsUdpListener::stfd()
{
    return lfd;
}

void SrsUdpListener::set_socket_buffer()
{
    int default_sndbuf = 0;
    // TODO: FIXME: Config it.
    int expect_sndbuf = 1024*1024*10; // 10M
    int actual_sndbuf = expect_sndbuf;
    int r0_sndbuf = 0;
    if (true) {
        socklen_t opt_len = sizeof(default_sndbuf);
        // TODO: FIXME: check err
        getsockopt(fd(), SOL_SOCKET, SO_SNDBUF, (void*)&default_sndbuf, &opt_len);

        if ((r0_sndbuf = setsockopt(fd(), SOL_SOCKET, SO_SNDBUF, (void*)&actual_sndbuf, sizeof(actual_sndbuf))) < 0) {
            srs_warn("set SO_SNDBUF failed, expect=%d, r0=%d", expect_sndbuf, r0_sndbuf);
        }

        opt_len = sizeof(actual_sndbuf);
        // TODO: FIXME: check err
        getsockopt(fd(), SOL_SOCKET, SO_SNDBUF, (void*)&actual_sndbuf, &opt_len);
    }

    int default_rcvbuf = 0;
    // TODO: FIXME: Config it.
    int expect_rcvbuf = 1024*1024*10; // 10M
    int actual_rcvbuf = expect_rcvbuf;
    int r0_rcvbuf = 0;
    if (true) {
        socklen_t opt_len = sizeof(default_rcvbuf);
        // TODO: FIXME: check err
        getsockopt(fd(), SOL_SOCKET, SO_RCVBUF, (void*)&default_rcvbuf, &opt_len);

        if ((r0_rcvbuf = setsockopt(fd(), SOL_SOCKET, SO_RCVBUF, (void*)&actual_rcvbuf, sizeof(actual_rcvbuf))) < 0) {
            srs_warn("set SO_RCVBUF failed, expect=%d, r0=%d", expect_rcvbuf, r0_rcvbuf);
        }

        opt_len = sizeof(actual_rcvbuf);
        // TODO: FIXME: check err
        getsockopt(fd(), SOL_SOCKET, SO_RCVBUF, (void*)&actual_rcvbuf, &opt_len);
    }

    srs_trace("UDP #%d LISTEN at %s:%d, SO_SNDBUF(default=%d, expect=%d, actual=%d, r0=%d), SO_RCVBUF(default=%d, expect=%d, actual=%d, r0=%d)",
        srs_netfd_fileno(lfd), ip.c_str(), port, default_sndbuf, expect_sndbuf, actual_sndbuf, r0_sndbuf, default_rcvbuf, expect_rcvbuf, actual_rcvbuf, r0_rcvbuf);
}


srs_error_t SrsUdpListener::listen()
{
    srs_error_t err = srs_success;

    if ((err = srs_udp_listen(ip, port, &lfd)) != srs_success) {
        return srs_error_wrap(err, "listen %s:%d", ip.c_str(), port);
    }

    set_socket_buffer();

    handler->set_stfd(lfd);
    
    srs_freep(trd);
    trd = new SrsSTCoroutine("udp", this, _srs_context->get_id());
    if ((err = trd->start()) != srs_success) {
        return srs_error_wrap(err, "start thread");
    }
    
    return err;
}

srs_error_t SrsUdpListener::cycle()
{
    srs_error_t err = srs_success;

    while (true) {
        if ((err = trd->pull()) != srs_success) {
            return srs_error_wrap(err, "udp listener");
        }

        int nread = 0;
        sockaddr_storage from;
        int nb_from = sizeof(from);
        if ((nread = srs_recvfrom(lfd, buf, nb_buf, (sockaddr*)&from, &nb_from, SRS_UTIME_NO_TIMEOUT)) <= 0) {
            return srs_error_new(ERROR_SOCKET_READ, "udp read, nread=%d", nread);
        }

        // Drop UDP health check packet of Aliyun SLB.
        //      Healthcheck udp check
        // @see https://help.aliyun.com/document_detail/27595.html
        if (nread == 21 && buf[0] == 0x48 && buf[1] == 0x65 && buf[2] == 0x61 && buf[3] == 0x6c
            && buf[19] == 0x63 && buf[20] == 0x6b) {
            continue;
        }

        if ((err = handler->on_udp_packet((const sockaddr*)&from, nb_from, buf, nread)) != srs_success) {
            return srs_error_wrap(err, "handle packet %d bytes", nread);
        }
        
        if (SrsUdpPacketRecvCycleInterval > 0) {
            srs_usleep(SrsUdpPacketRecvCycleInterval);
        }
    }
    
    return err;
}

SrsTcpListener::SrsTcpListener(ISrsTcpHandler* h, string i, int p)
{
    handler = h;
    ip = i;
    port = p;

    lfd = NULL;
    
    trd = new SrsDummyCoroutine();
}

SrsTcpListener::~SrsTcpListener()
{
    srs_freep(trd);
    srs_close_stfd(lfd);
}

int SrsTcpListener::fd()
{
    return srs_netfd_fileno(lfd);;
}

srs_error_t SrsTcpListener::listen()
{
    srs_error_t err = srs_success;

    if ((err = srs_tcp_listen(ip, port, &lfd)) != srs_success) {
        return srs_error_wrap(err, "listen at %s:%d", ip.c_str(), port);
    }
    
    srs_freep(trd);
    trd = new SrsSTCoroutine("tcp", this);
    if ((err = trd->start()) != srs_success) {
        return srs_error_wrap(err, "start coroutine");
    }
    
    return err;
}

srs_error_t SrsTcpListener::cycle()
{
    srs_error_t err = srs_success;
    
    while (true) {
        if ((err = trd->pull()) != srs_success) {
            return srs_error_wrap(err, "tcp listener");
        }
        
        srs_netfd_t fd = srs_accept(lfd, NULL, NULL, SRS_UTIME_NO_TIMEOUT);
        if(fd == NULL){
            return srs_error_new(ERROR_SOCKET_ACCEPT, "accept at fd=%d", srs_netfd_fileno(lfd));
        }
        
        if ((err = srs_fd_closeexec(srs_netfd_fileno(fd))) != srs_success) {
            return srs_error_wrap(err, "set closeexec");
        }
        
        if ((err = handler->on_tcp_client(fd)) != srs_success) {
            return srs_error_wrap(err, "handle fd=%d", srs_netfd_fileno(fd));
        }
    }
    
    return err;
}

SrsUdpMuxSocket::SrsUdpMuxSocket(srs_netfd_t fd)
{
    nb_buf = SRS_UDP_MAX_PACKET_SIZE;
    buf = new char[nb_buf];
    nread = 0;

    lfd = fd;

    fromlen = 0;
    peer_port = 0;
}

SrsUdpMuxSocket::~SrsUdpMuxSocket()
{
    srs_freepa(buf);
}

int SrsUdpMuxSocket::recvfrom(srs_utime_t timeout)
{
    fromlen = sizeof(from);
    nread = srs_recvfrom(lfd, buf, nb_buf, (sockaddr*)&from, &fromlen, timeout);

    // Drop UDP health check packet of Aliyun SLB.
    //      Healthcheck udp check
    // @see https://help.aliyun.com/document_detail/27595.html
    if (nread == 21 && buf[0] == 0x48 && buf[1] == 0x65 && buf[2] == 0x61 && buf[3] == 0x6c
        && buf[19] == 0x63 && buf[20] == 0x6b) {
        return 0;
    }

    if (nread > 0) {
        // TODO: FIXME: Maybe we should not covert to string for each packet.
        char address_string[64];
        char port_string[16];
        if (getnameinfo((sockaddr*)&from, fromlen, 
                       (char*)&address_string, sizeof(address_string),
                       (char*)&port_string, sizeof(port_string),
                       NI_NUMERICHOST|NI_NUMERICSERV)) {
            return -1;
        }

        peer_ip = std::string(address_string);
        peer_port = atoi(port_string);    
    }

    return nread;
}

srs_error_t SrsUdpMuxSocket::sendto(void* data, int size, srs_utime_t timeout)
{
    srs_error_t err = srs_success;

    int nb_write = srs_sendto(lfd, data, size, (sockaddr*)&from, fromlen, timeout);

    if (nb_write <= 0) {
        if (nb_write < 0 && errno == ETIME) {
            return srs_error_new(ERROR_SOCKET_TIMEOUT, "sendto timeout %d ms", srsu2msi(timeout));
        }   
    
        return srs_error_new(ERROR_SOCKET_WRITE, "sendto");
    }   

    return err;
}

srs_netfd_t SrsUdpMuxSocket::stfd()
{
    return lfd;
}

sockaddr_in* SrsUdpMuxSocket::peer_addr()
{
    return (sockaddr_in*)&from;
}

socklen_t SrsUdpMuxSocket::peer_addrlen()
{
    return (socklen_t)fromlen;
}

char* SrsUdpMuxSocket::data()
{
    return buf;
}

int SrsUdpMuxSocket::size()
{
    return nread;
}

std::string SrsUdpMuxSocket::get_peer_ip() const
{
    return peer_ip;
}

int SrsUdpMuxSocket::get_peer_port() const
{
    return peer_port;
}

std::string SrsUdpMuxSocket::peer_id()
{
    char id_buf[1024];
    int len = snprintf(id_buf, sizeof(id_buf), "%s:%d", peer_ip.c_str(), peer_port);

    return string(id_buf, len);
}

SrsUdpMuxSocket* SrsUdpMuxSocket::copy_sendonly()
{
    SrsUdpMuxSocket* sendonly = new SrsUdpMuxSocket(lfd);

    // Don't copy buffer
    srs_freepa(sendonly->buf);
    sendonly->nb_buf    = 0;
    sendonly->nread     = 0;
    sendonly->lfd       = lfd;
    sendonly->from      = from;
    sendonly->fromlen   = fromlen;
    sendonly->peer_ip   = peer_ip;
    sendonly->peer_port = peer_port;

    return sendonly;
}

SrsUdpMuxListener::SrsUdpMuxListener(ISrsUdpMuxHandler* h, std::string i, int p)
{
    handler = h;

    ip = i;
    port = p;
    lfd = NULL;
    
    nb_buf = SRS_UDP_MAX_PACKET_SIZE;
    buf = new char[nb_buf];

    trd = new SrsDummyCoroutine();
    cid = _srs_context->generate_id();
}

SrsUdpMuxListener::~SrsUdpMuxListener()
{
    srs_freep(trd);
    srs_close_stfd(lfd);
    srs_freepa(buf);
}

int SrsUdpMuxListener::fd()
{
    return srs_netfd_fileno(lfd);
}

srs_netfd_t SrsUdpMuxListener::stfd()
{
    return lfd;
}

srs_error_t SrsUdpMuxListener::listen()
{
    srs_error_t err = srs_success;

    if ((err = srs_udp_listen(ip, port, &lfd)) != srs_success) {
        return srs_error_wrap(err, "listen %s:%d", ip.c_str(), port);
    }
    
    srs_freep(trd);
    trd = new SrsSTCoroutine("udp", this, cid);
    if ((err = trd->start()) != srs_success) {
        return srs_error_wrap(err, "start thread");
    }
    
    return err;
}

void SrsUdpMuxListener::set_socket_buffer()
{
    int default_sndbuf = 0;
    // TODO: FIXME: Config it.
    int expect_sndbuf = 1024*1024*10; // 10M
    int actual_sndbuf = expect_sndbuf;
    int r0_sndbuf = 0;
    if (true) {
        socklen_t opt_len = sizeof(default_sndbuf);
        getsockopt(fd(), SOL_SOCKET, SO_SNDBUF, (void*)&default_sndbuf, &opt_len);

        if ((r0_sndbuf = setsockopt(fd(), SOL_SOCKET, SO_SNDBUF, (void*)&actual_sndbuf, sizeof(actual_sndbuf))) < 0) {
            srs_warn("set SO_SNDBUF failed, expect=%d, r0=%d", expect_sndbuf, r0_sndbuf);
        }

        opt_len = sizeof(actual_sndbuf);
        getsockopt(fd(), SOL_SOCKET, SO_SNDBUF, (void*)&actual_sndbuf, &opt_len);
    }

    int default_rcvbuf = 0;
    // TODO: FIXME: Config it.
    int expect_rcvbuf = 1024*1024*10; // 10M
    int actual_rcvbuf = expect_rcvbuf;
    int r0_rcvbuf = 0;
    if (true) {
        socklen_t opt_len = sizeof(default_rcvbuf);
        getsockopt(fd(), SOL_SOCKET, SO_RCVBUF, (void*)&default_rcvbuf, &opt_len);

        if ((r0_rcvbuf = setsockopt(fd(), SOL_SOCKET, SO_RCVBUF, (void*)&actual_rcvbuf, sizeof(actual_rcvbuf))) < 0) {
            srs_warn("set SO_RCVBUF failed, expect=%d, r0=%d", expect_rcvbuf, r0_rcvbuf);
        }

        opt_len = sizeof(actual_rcvbuf);
        getsockopt(fd(), SOL_SOCKET, SO_RCVBUF, (void*)&actual_rcvbuf, &opt_len);
    }

    srs_trace("UDP #%d LISTEN at %s:%d, SO_SNDBUF(default=%d, expect=%d, actual=%d, r0=%d), SO_RCVBUF(default=%d, expect=%d, actual=%d, r0=%d)",
        srs_netfd_fileno(lfd), ip.c_str(), port, default_sndbuf, expect_sndbuf, actual_sndbuf, r0_sndbuf, default_rcvbuf, expect_rcvbuf, actual_rcvbuf, r0_rcvbuf);
}

srs_error_t SrsUdpMuxListener::cycle()
{
    srs_error_t err = srs_success;

    SrsPithyPrint* pprint = SrsPithyPrint::create_rtc_recv(srs_netfd_fileno(lfd));
    SrsAutoFree(SrsPithyPrint, pprint);

    uint64_t nn_msgs = 0;
    uint64_t nn_msgs_stage = 0;
    uint64_t nn_msgs_last = 0;
    uint64_t nn_loop = 0;
    srs_utime_t time_last = srs_get_system_time();

    SrsErrorPithyPrint* pp_pkt_handler_err = new SrsErrorPithyPrint();
    SrsAutoFree(SrsErrorPithyPrint, pp_pkt_handler_err);

    set_socket_buffer();
    
    while (true) {
        if ((err = trd->pull()) != srs_success) {
            return srs_error_wrap(err, "udp listener");
        }

        nn_loop++;

        // TODO: FIXME: Refactor the memory cache for receiver.
        // Because we have to decrypt the cipher of received packet payload,
        // and the size is not determined, so we think there is at least one copy,
        // and we can reuse the plaintext h264/opus with players when got plaintext.
        SrsUdpMuxSocket skt(lfd);

        int nread = skt.recvfrom(SRS_UTIME_NO_TIMEOUT);
        if (nread <= 0) {
            if (nread < 0) {
                srs_warn("udp recv error nn=%d", nread);
            }
            // remux udp never return
            continue;
        }

        nn_msgs++;
        nn_msgs_stage++;

        // Restore context when packets processed.
        if (true) {
            SrsContextRestore(cid);
            err = handler->on_udp_packet(&skt);
        }
        // Use pithy print to show more smart information.
        if (err != srs_success) {
            uint32_t nn = 0;
            if (pp_pkt_handler_err->can_print(err, &nn)) {
                // Append more information.
                err = srs_error_wrap(err, "size=%u, data=[%s]", skt.size(), srs_string_dumps_hex(skt.data(), skt.size(), 8).c_str());
                srs_warn("handle udp pkt, count=%u/%u, err: %s", pp_pkt_handler_err->nn_count, nn, srs_error_desc(err).c_str());
            }
            srs_freep(err);
        }

        pprint->elapse();
        if (pprint->can_print()) {
            int pps_average = 0; int pps_last = 0;
            if (true) {
                if (srs_get_system_time() > srs_get_system_startup_time()) {
                    pps_average = (int)(nn_msgs * SRS_UTIME_SECONDS / (srs_get_system_time() - srs_get_system_startup_time()));
                }
                if (srs_get_system_time() > time_last) {
                    pps_last = (int)((nn_msgs - nn_msgs_last) * SRS_UTIME_SECONDS / (srs_get_system_time() - time_last));
                }
            }

            string pps_unit = "";
            if (pps_last > 10000 || pps_average > 10000) {
                pps_unit = "(w)"; pps_last /= 10000; pps_average /= 10000;
            } else if (pps_last > 1000 || pps_average > 1000) {
                pps_unit = "(k)"; pps_last /= 10000; pps_average /= 10000;
            }

            srs_trace("<- RTC RECV #%d, udp %" PRId64 ", pps %d/%d%s, schedule %" PRId64,
                srs_netfd_fileno(lfd), nn_msgs_stage, pps_average, pps_last, pps_unit.c_str(), nn_loop);
            nn_msgs_last = nn_msgs; time_last = srs_get_system_time();
            nn_loop = 0; nn_msgs_stage = 0;
        }
    
        if (SrsUdpPacketRecvCycleInterval > 0) {
            srs_usleep(SrsUdpPacketRecvCycleInterval);
        }   
    }   
    
    return err;
}

