//
// Copyright (c) 2013-2021 The SRS Authors
//
// SPDX-License-Identifier: MIT or MulanPSL-2.0
//

#include <srs_app_srt_server.hpp>

using namespace std;

#include <srs_kernel_log.hpp>
#include <srs_kernel_utility.hpp>
#include <srs_service_log.hpp>
#include <srs_app_config.hpp>
#include <srs_app_srt_conn.hpp>

#ifdef SRS_SRT
SrsSrtEventLoop* _srt_eventloop = NULL;
#endif

SrsSrtAcceptor::SrsSrtAcceptor(SrsSrtServer* srt_server)
{
    port_ = 0;
    srt_server_ = srt_server;
    listener_ = NULL;
}

SrsSrtAcceptor::~SrsSrtAcceptor()
{
    srs_freep(listener_);
}

srs_error_t SrsSrtAcceptor::listen(std::string ip, int port)
{
    srs_error_t err = srs_success;

    ip_ = ip;
    port_ = port;

    srs_freep(listener_);
    listener_ = new SrsSrtListener(this, ip_, port_);

    // Create srt socket.
    if ((err = listener_->create_socket()) != srs_success) {
        return srs_error_wrap(err, "message srt acceptor");
    }

    // Set all the srt option from config.
    if ((err = set_srt_opt()) != srs_success) {
        return srs_error_wrap(err, "set opt");
    }

    // Start listen srt socket, this function will set the socket in async mode.
    if ((err = listener_->listen()) != srs_success) {
        return srs_error_wrap(err, "message srt acceptor");
    }

    srs_trace("srt listen at udp://%s:%d, fd=%d", ip_.c_str(), port_, listener_->fd());

    return err;
}

srs_error_t SrsSrtAcceptor::set_srt_opt()
{
    srs_error_t err = srs_success;

    if ((err = srs_srt_set_maxbw(listener_->fd(), _srs_config->get_srto_maxbw())) != srs_success) {
        return srs_error_wrap(err, "set opt");
    }

    if ((err = srs_srt_set_mss(listener_->fd(), _srs_config->get_srto_mss())) != srs_success) {
        return srs_error_wrap(err, "set opt");
    }

    if ((err = srs_srt_set_tsbpdmode(listener_->fd(), _srs_config->get_srto_tsbpdmode())) != srs_success) {
        return srs_error_wrap(err, "set opt");
    }

    if ((err = srs_srt_set_latency(listener_->fd(), _srs_config->get_srto_latency())) != srs_success) {
        return srs_error_wrap(err, "set opt");
    }

    if ((err = srs_srt_set_rcv_latency(listener_->fd(), _srs_config->get_srto_recv_latency())) != srs_success) {
        return srs_error_wrap(err, "set opt");
    }

    if ((err = srs_srt_set_peer_latency(listener_->fd(), _srs_config->get_srto_peer_latency())) != srs_success) {
        return srs_error_wrap(err, "set opt");
    }

    if ((err = srs_srt_set_tlpktdrop(listener_->fd(), _srs_config->get_srto_tlpktdrop())) != srs_success) {
        return srs_error_wrap(err, "set opt");
    }

    if ((err = srs_srt_set_connect_timeout(listener_->fd(), srsu2msi(_srs_config->get_srto_conntimeout()))) != srs_success) {
        return srs_error_wrap(err, "set opt");
    }

    if ((err = srs_srt_set_peer_idle_timeout(listener_->fd(), srsu2msi(_srs_config->get_srto_peeridletimeout()))) != srs_success) {
        return srs_error_wrap(err, "set opt");
    }

    if ((err = srs_srt_set_sndbuf(listener_->fd(), _srs_config->get_srto_sendbuf())) != srs_success) {
        return srs_error_wrap(err, "set opt");
    }

    if ((err = srs_srt_set_rcvbuf(listener_->fd(), _srs_config->get_srto_recvbuf())) != srs_success) {
        return srs_error_wrap(err, "set opt");
    }

    if ((err = srs_srt_set_payload_size(listener_->fd(), _srs_config->get_srto_payloadsize())) != srs_success) {
        return srs_error_wrap(err, "set opt");
    }

    return err;
}

srs_error_t SrsSrtAcceptor::on_srt_client(srs_srt_t srt_fd)
{
    srs_error_t err = srs_success;

    // Notify srt server to accept srt client, and create new SrsSrtConn on it.
    if ((err = srt_server_->accept_srt_client(srt_fd)) != srs_success) {
        srs_warn("accept srt client failed, err is %s", srs_error_desc(err).c_str());
        srs_freep(err);
    }
    
    return err;
}

SrsSrtServer::SrsSrtServer()
{
    conn_manager_ = new SrsResourceManager("SRT", true);
}

SrsSrtServer::~SrsSrtServer()
{
    srs_freep(conn_manager_);
}

srs_error_t SrsSrtServer::initialize()
{
    srs_error_t err = srs_success;
    return err;
}

srs_error_t SrsSrtServer::listen()
{
    srs_error_t err = srs_success;
    
    // Listen mpegts over srt.
    if ((err = listen_srt_mpegts()) != srs_success) {
        return srs_error_wrap(err, "srt mpegts listen");
    }

    if ((err = conn_manager_->start()) != srs_success) {
        return srs_error_wrap(err, "srt connection manager");
    }

    return err;
}

srs_error_t SrsSrtServer::listen_srt_mpegts()
{
    srs_error_t err = srs_success;

    if (! _srs_config->get_srt_enabled()) {
        return err;
    }

    // Close all listener for SRT if exists.
    close_listeners();

    // Start a listener for SRT, we might need multiple listeners in the future.
    SrsSrtAcceptor* acceptor = new SrsSrtAcceptor(this);
    acceptors_.push_back(acceptor);

    int port; string ip;
    srs_parse_endpoint(srs_int2str(_srs_config->get_srt_listen_port()), ip, port);

    if ((err = acceptor->listen(ip, port)) != srs_success) {
        return srs_error_wrap(err, "srt listen %s:%d", ip.c_str(), port);
    }

    return err;
}

void SrsSrtServer::close_listeners()
{
    std::vector<SrsSrtAcceptor*>::iterator it;
    for (it = acceptors_.begin(); it != acceptors_.end();) {
        SrsSrtAcceptor* acceptor = *it;
        srs_freep(acceptor);

        it = acceptors_.erase(it);
    }
}

srs_error_t SrsSrtServer::accept_srt_client(srs_srt_t srt_fd)
{
    srs_error_t err = srs_success;

    ISrsStartableConneciton* conn = NULL;
    if ((err = fd_to_resource(srt_fd, &conn)) != srs_success) {
        //close fd on conn error, otherwise will lead to fd leak -gs
        // TODO: FIXME: Handle error.
        srs_srt_close(srt_fd);
        return srs_error_wrap(err, "srt fd to resource");
    }
    srs_assert(conn);
    
    // directly enqueue, the cycle thread will remove the client.
    conn_manager_->add(conn);

    if ((err = conn->start()) != srs_success) {
        return srs_error_wrap(err, "start srt conn coroutine");
    }
    
    return err;
}

srs_error_t SrsSrtServer::fd_to_resource(srs_srt_t srt_fd, ISrsStartableConneciton** pr)
{
    srs_error_t err = srs_success;
    
    string ip = "";
    int port = 0;
    if ((err = srs_srt_get_remote_ip_port(srt_fd, ip, port)) != srs_success) {
        return srs_error_wrap(err, "get srt ip port");
    }

    // TODO: FIXME: need to check max connection?

    // The context id may change during creating the bellow objects.
    SrsContextRestore(_srs_context->get_id());

    // Covert to SRT conection.
    *pr = new SrsMpegtsSrtConn(this, srt_fd, ip, port);
    
    return err;
}

void SrsSrtServer::remove(ISrsResource* c)
{
    // TODO: FIXME: add some statistic of srt.
    // ISrsStartableConneciton* conn = dynamic_cast<ISrsStartableConneciton*>(c);

    // SrsStatistic* stat = SrsStatistic::instance();
    // stat->kbps_add_delta(c->get_id().c_str(), conn);
    // stat->on_disconnect(c->get_id().c_str());

    // use manager to free it async.
    conn_manager_->remove(c);
}

SrsSrtServerAdapter::SrsSrtServerAdapter()
{
    srt_server_ = new SrsSrtServer();
}

SrsSrtServerAdapter::~SrsSrtServerAdapter()
{
    srs_freep(srt_server_);
}

srs_error_t SrsSrtServerAdapter::initialize()
{
    srs_error_t err = srs_success;

    if ((err = srs_srt_log_initialie()) != srs_success) {
        return srs_error_wrap(err, "srt log initialize");
    }

    _srt_eventloop = new SrsSrtEventLoop();

    if ((err = _srt_eventloop->initialize()) != srs_success) {
        return srs_error_wrap(err, "srt poller initialize");
    }

    if ((err = _srt_eventloop->start()) != srs_success) {
        return srs_error_wrap(err, "srt poller start");
    }

    return err;
}

srs_error_t SrsSrtServerAdapter::run(SrsWaitGroup* wg)
{
    srs_error_t err = srs_success;

    // Initialize the whole system, set hooks to handle server level events.
    if ((err = srt_server_->initialize()) != srs_success) {
        return srs_error_wrap(err, "srt server initialize");
    }

    if ((err = srt_server_->listen()) != srs_success) {
        return srs_error_wrap(err, "srt listen");
    }

    return err;
}

void SrsSrtServerAdapter::stop()
{
}

SrsSrtServer* SrsSrtServerAdapter::instance()
{
    return srt_server_;
}

SrsSrtEventLoop::SrsSrtEventLoop()
{
    srt_poller_ = NULL;
    trd_ = NULL;
}

SrsSrtEventLoop::~SrsSrtEventLoop()
{
    srs_freep(trd_);
    srs_freep(srt_poller_);
}

srs_error_t SrsSrtEventLoop::initialize()
{
    srs_error_t err = srs_success;

    srt_poller_ = srs_srt_poller_new();

    if ((err = srt_poller_->initialize()) != srs_success) {
        return srs_error_wrap(err, "srt poller initialize");
    }

    return err;
}

srs_error_t SrsSrtEventLoop::start()
{
    srs_error_t err = srs_success;

    trd_ = new SrsSTCoroutine("srt_listener", this);
    if ((err = trd_->start()) != srs_success) {
        return srs_error_wrap(err, "start coroutine");
    }

    return err;
}

srs_error_t SrsSrtEventLoop::cycle()
{
    srs_error_t err = srs_success;
    
    while (true) {
        if ((err = trd_->pull()) != srs_success) {
            return srs_error_wrap(err, "srt listener");
        }
       
        // Check and notify fired SRT events by epoll.
        //
        // Note that the SRT poller use a dedicated and isolated epoll, which is not the same as the one of SRS, in
        // short, the wait won't switch to other coroutines when no fd is active, so we must use timeout(0) to make sure
        // to return directly, then use srs_usleep to do the coroutine switch.
        int n_fds = 0;
        if ((err = srt_poller_->wait(0, &n_fds)) != srs_success) {
            srs_warn("srt poll wait failed, n_fds=%d, err=%s", n_fds, srs_error_desc(err).c_str());
            srs_error_reset(err);
        }

        // We use sleep to switch to other coroutines, because the SRT poller is not possible to do this.
        srs_usleep((n_fds ? 1 : 10) * SRS_UTIME_MILLISECONDS);
    }
    
    return err;
}

