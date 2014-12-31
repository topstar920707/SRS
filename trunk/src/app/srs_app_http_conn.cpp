/*
The MIT License (MIT)

Copyright (c) 2013-2015 winlin

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

#ifdef SRS_AUTO_HTTP_SERVER

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

#include <sstream>
using namespace std;

#include <srs_kernel_log.hpp>
#include <srs_kernel_error.hpp>
#include <srs_app_st_socket.hpp>
#include <srs_core_autofree.hpp>
#include <srs_app_config.hpp>
#include <srs_kernel_flv.hpp>
#include <srs_kernel_utility.hpp>
#include <srs_kernel_file.hpp>

#define SRS_HTTP_DEFAULT_PAGE "index.html"

SrsHttpRoot::SrsHttpRoot()
{
    // TODO: FIXME: support reload vhosts.
}

SrsHttpRoot::~SrsHttpRoot()
{
}

int SrsHttpRoot::initialize()
{
    int ret = ERROR_SUCCESS;
    
    bool default_root_exists = false;
    
    // add other virtual path
    SrsConfDirective* root = _srs_config->get_root();
    for (int i = 0; i < (int)root->directives.size(); i++) {
        SrsConfDirective* conf = root->at(i);
        
        if (!conf->is_vhost()) {
            continue;
        }
        
        std::string vhost = conf->arg0();
        if (!_srs_config->get_vhost_http_enabled(vhost)) {
            continue;
        }
        
        std::string mount = _srs_config->get_vhost_http_mount(vhost);
        std::string dir = _srs_config->get_vhost_http_dir(vhost);
        
        handlers.push_back(new SrsHttpVhost(vhost, mount, dir));
        
        if (mount == "/") {
            default_root_exists = true;
        }
    }
    
    if (!default_root_exists) {
        // add root
        handlers.push_back(new SrsHttpVhost(
            "__http__", "/", _srs_config->get_http_stream_dir()));
    }
    
    return ret;
}

int SrsHttpRoot::best_match(const char* path, int length, SrsHttpHandlerMatch** ppmatch)
{
    int ret = ERROR_SUCCESS;
        
    // find the best matched child handler.
    std::vector<SrsHttpHandler*>::iterator it;
    for (it = handlers.begin(); it != handlers.end(); ++it) {
        SrsHttpHandler* h = *it;
        
        // search all child handlers.
        h->best_match(path, length, ppmatch);
    }
    
    // if already matched by child, return.
    if (*ppmatch) {
        return ret;
    }
    
    // not matched, error.
    return ERROR_HTTP_HANDLER_MATCH_URL;
}

bool SrsHttpRoot::is_handler_valid(SrsHttpMessage* /*req*/, int& status_code, std::string& reason_phrase)
{
    status_code = SRS_CONSTS_HTTP_InternalServerError;
    reason_phrase = SRS_CONSTS_HTTP_InternalServerError_str;
    
    return false;
}

int SrsHttpRoot::do_process_request(SrsStSocket* /*skt*/, SrsHttpMessage* /*req*/)
{
    int ret = ERROR_SUCCESS;
    return ret;
}

SrsHttpVhost::SrsHttpVhost(std::string vhost, std::string mount, std::string dir)
{
    _vhost = vhost;
    _mount = mount;
    _dir = dir;
}

SrsHttpVhost::~SrsHttpVhost()
{
}

bool SrsHttpVhost::can_handle(const char* path, int length, const char** /*pchild*/)
{
    return srs_path_like(_mount.c_str(), path, length);
}

bool SrsHttpVhost::is_handler_valid(SrsHttpMessage* req, int& status_code, std::string& reason_phrase) 
{
    std::string fullpath = get_request_file(req);
    
    if (::access(fullpath.c_str(), F_OK | R_OK) < 0) {
        srs_warn("check file %s does not exists", fullpath.c_str());
        
        status_code = SRS_CONSTS_HTTP_NotFound;
        reason_phrase = SRS_CONSTS_HTTP_NotFound_str;
        return false;
    }
    
    return true;
}

int SrsHttpVhost::do_process_request(SrsStSocket* skt, SrsHttpMessage* req)
{
    std::string fullpath = get_request_file(req);
    
    // TODO: FIXME: support mp4, @see https://github.com/winlinvip/simple-rtmp-server/issues/174
    if (srs_string_ends_with(fullpath, ".ts")) {
        return response_ts_file(skt, req, fullpath);
    } else if (srs_string_ends_with(fullpath, ".flv") || srs_string_ends_with(fullpath, ".fhv")) {
        std::string start = req->query_get("start");
        if (start.empty()) {
            return response_flv_file(skt, req, fullpath);
        }

        int offset = ::atoi(start.c_str());
        if (offset <= 0) {
            return response_flv_file(skt, req, fullpath);
        }
        
        return response_flv_file2(skt, req, fullpath, offset);
    }

    return response_regular_file(skt, req, fullpath);
}

int SrsHttpVhost::response_regular_file(SrsStSocket* skt, SrsHttpMessage* req, string fullpath)
{
    int ret = ERROR_SUCCESS;

    SrsFileReader fs;
    
    if ((ret = fs.open(fullpath)) != ERROR_SUCCESS) {
        srs_warn("open file %s failed, ret=%d", fullpath.c_str(), ret);
        return ret;
    }

    int64_t length = fs.filesize();
    
    char* buf = new char[length];
    SrsAutoFree(char, buf);
    
    if ((ret = fs.read(buf, length, NULL)) != ERROR_SUCCESS) {
        srs_warn("read file %s failed, ret=%d", fullpath.c_str(), ret);
        return ret;
    }
    
    std::string str;
    str.append(buf, length);
    
    if (srs_string_ends_with(fullpath, ".ts")) {
        return res_mpegts(skt, req, str);
    } else if (srs_string_ends_with(fullpath, ".m3u8")) {
        return res_m3u8(skt, req, str);
    } else if (srs_string_ends_with(fullpath, ".xml")) {
        return res_xml(skt, req, str);
    } else if (srs_string_ends_with(fullpath, ".js")) {
        return res_javascript(skt, req, str);
    } else if (srs_string_ends_with(fullpath, ".json")) {
        return res_json(skt, req, str);
    } else if (srs_string_ends_with(fullpath, ".swf")) {
        return res_swf(skt, req, str);
    } else if (srs_string_ends_with(fullpath, ".css")) {
        return res_css(skt, req, str);
    } else if (srs_string_ends_with(fullpath, ".ico")) {
        return res_ico(skt, req, str);
    } else {
        return res_text(skt, req, str);
    }
    
    return ret;
}

int SrsHttpVhost::response_flv_file(SrsStSocket* skt, SrsHttpMessage* req, string fullpath)
{
    int ret = ERROR_SUCCESS;

    SrsFileReader fs;
    
    // TODO: FIXME: use more advance cache.
    if ((ret = fs.open(fullpath)) != ERROR_SUCCESS) {
        srs_warn("open file %s failed, ret=%d", fullpath.c_str(), ret);
        return ret;
    }

    int64_t length = fs.filesize();

    // write http header for ts.
    std::stringstream ss;

    res_status_line(ss)->res_content_type_flv(ss)
        ->res_content_length(ss, (int)length);
        
    if (req->requires_crossdomain()) {
        res_enable_crossdomain(ss);
    }
    
    res_header_eof(ss);
    
    // flush http header to peer
    if ((ret = res_flush(skt, ss)) != ERROR_SUCCESS) {
        return ret;
    }
    
    // write body.
    int64_t left = length;
    char* buf = req->http_ts_send_buffer();
    
    while (left > 0) {
        ssize_t nread = -1;
        if ((ret = fs.read(buf, __SRS_HTTP_TS_SEND_BUFFER_SIZE, &nread)) != ERROR_SUCCESS) {
            srs_warn("read file %s failed, ret=%d", fullpath.c_str(), ret);
            break;
        }
        
        left -= nread;
        if ((ret = skt->write(buf, nread, NULL)) != ERROR_SUCCESS) {
            break;
        }
    }
    
    return ret;
}

int SrsHttpVhost::response_flv_file2(SrsStSocket* skt, SrsHttpMessage* req, string fullpath, int offset)
{
    int ret = ERROR_SUCCESS;
    
    SrsFileReader fs;
    
    // open flv file
    if ((ret = fs.open(fullpath)) != ERROR_SUCCESS) {
        return ret;
    }
    
    if (offset > fs.filesize()) {
        ret = ERROR_HTTP_FLV_OFFSET_OVERFLOW;
        srs_warn("http flv streaming %s overflow. size=%"PRId64", offset=%d, ret=%d", 
            fullpath.c_str(), fs.filesize(), offset, ret);
        return ret;
    }
    
    SrsFlvVodStreamDecoder ffd;
    
    // open fast decoder
    if ((ret = ffd.initialize(&fs)) != ERROR_SUCCESS) {
        return ret;
    }
    
    // save header, send later.
    char flv_header[13];
    
    // send flv header
    if ((ret = ffd.read_header_ext(flv_header)) != ERROR_SUCCESS) {
        return ret;
    }
    
    // save sequence header, send later
    char* sh_data = NULL;
    int sh_size = 0;
    
    if (true) {
        // send sequence header
        int64_t start = 0;
        if ((ret = ffd.read_sequence_header_summary(&start, &sh_size)) != ERROR_SUCCESS) {
            return ret;
        }
        if (sh_size <= 0) {
            ret = ERROR_HTTP_FLV_SEQUENCE_HEADER;
            srs_warn("http flv streaming no sequence header. size=%d, ret=%d", sh_size, ret);
            return ret;
        }
    }
    sh_data = new char[sh_size];
    SrsAutoFree(char, sh_data);
    if ((ret = fs.read(sh_data, sh_size, NULL)) != ERROR_SUCCESS) {
        return ret;
    }

    // seek to data offset
    int64_t left = fs.filesize() - offset;

    // write http header for ts.
    std::stringstream ss;

    res_status_line(ss)->res_content_type_flv(ss)
        ->res_content_length(ss, (int)(sizeof(flv_header) + sh_size + left));
        
    if (req->requires_crossdomain()) {
        res_enable_crossdomain(ss);
    }
    
    res_header_eof(ss);
    
    // flush http header to peer
    if ((ret = res_flush(skt, ss)) != ERROR_SUCCESS) {
        return ret;
    }
    
    if ((ret = skt->write(flv_header, sizeof(flv_header), NULL)) != ERROR_SUCCESS) {
        return ret;
    }
    if (sh_size > 0 && (ret = skt->write(sh_data, sh_size, NULL)) != ERROR_SUCCESS) {
        return ret;
    }
    
    // write body.
    char* buf = req->http_ts_send_buffer();
    if ((ret = ffd.lseek(offset)) != ERROR_SUCCESS) {
        return ret;
    }
    
    // send data
    while (left > 0) {
        ssize_t nread = -1;
        if ((ret = fs.read(buf, __SRS_HTTP_TS_SEND_BUFFER_SIZE, &nread)) != ERROR_SUCCESS) {
            return ret;
        }
        
        left -= nread;
        if ((ret = skt->write(buf, nread, NULL)) != ERROR_SUCCESS) {
            return ret;
        }
    }
    
    return ret;
}

int SrsHttpVhost::response_ts_file(SrsStSocket* skt, SrsHttpMessage* req, string fullpath)
{
    int ret = ERROR_SUCCESS;
    
    SrsFileReader fs;
    
    // TODO: FIXME: use more advance cache.
    if ((ret = fs.open(fullpath)) != ERROR_SUCCESS) {
        srs_warn("open file %s failed, ret=%d", fullpath.c_str(), ret);
        return ret;
    }

    int64_t length = fs.filesize();

    // write http header for ts.
    std::stringstream ss;

    res_status_line(ss)->res_content_type_mpegts(ss)
        ->res_content_length(ss, (int)length);
        
    if (req->requires_crossdomain()) {
        res_enable_crossdomain(ss);
    }
    
    res_header_eof(ss);
    
    // flush http header to peer
    if ((ret = res_flush(skt, ss)) != ERROR_SUCCESS) {
        return ret;
    }
    
    // write body.
    int64_t left = length;
    char* buf = req->http_ts_send_buffer();
    
    while (left > 0) {
        ssize_t nread = -1;
        if ((ret = fs.read(buf, __SRS_HTTP_TS_SEND_BUFFER_SIZE, &nread)) != ERROR_SUCCESS) {
            srs_warn("read file %s failed, ret=%d", fullpath.c_str(), ret);
            break;
        }
        
        left -= nread;
        if ((ret = skt->write(buf, nread, NULL)) != ERROR_SUCCESS) {
            break;
        }
    }
    
    return ret;
}

string SrsHttpVhost::get_request_file(SrsHttpMessage* req)
{
    std::string fullpath = _dir + "/"; 
    
    // if root, directly use the matched url.
    if (_mount == "/") {
        // add the dir
        fullpath += req->match()->matched_url;
        // if file speicified, add the file.
        if (!req->match()->unmatched_url.empty()) {
            fullpath += "/" + req->match()->unmatched_url;
        }
    } else {
        // virtual path, ignore the virutal path.
        fullpath += req->match()->unmatched_url;
    }
    
    // add default pages.
    if (srs_string_ends_with(fullpath, "/")) {
        fullpath += SRS_HTTP_DEFAULT_PAGE;
    }
    
    return fullpath;
}

string SrsHttpVhost::vhost()
{
    return _vhost;
}

string SrsHttpVhost::mount()
{
    return _mount;
}

string SrsHttpVhost::dir()
{
    return _dir;
}

SrsHttpConn::SrsHttpConn(SrsServer* srs_server, st_netfd_t client_stfd, SrsHttpHandler* _handler) 
    : SrsConnection(srs_server, client_stfd)
{
    parser = new SrsHttpParser();
    handler = _handler;
    requires_crossdomain = false;
}

SrsHttpConn::~SrsHttpConn()
{
    srs_freep(parser);
}

void SrsHttpConn::kbps_resample()
{
    // TODO: FIXME: implements it
}

int64_t SrsHttpConn::get_send_bytes_delta()
{
    // TODO: FIXME: implements it
    return 0;
}

int64_t SrsHttpConn::get_recv_bytes_delta()
{
    // TODO: FIXME: implements it
    return 0;
}

int SrsHttpConn::do_cycle()
{
    int ret = ERROR_SUCCESS;
    
    srs_trace("HTTP client ip=%s", ip.c_str());
    
    // initialize parser
    if ((ret = parser->initialize(HTTP_REQUEST)) != ERROR_SUCCESS) {
        srs_error("http initialize http parser failed. ret=%d", ret);
        return ret;
    }
    
    // underlayer socket
    SrsStSocket skt(stfd);
    
    // process http messages.
    for (;;) {
        SrsHttpMessage* req = NULL;
        
        // get a http message
        if ((ret = parser->parse_message(&skt, &req)) != ERROR_SUCCESS) {
            return ret;
        }

        // if SUCCESS, always NOT-NULL and completed message.
        srs_assert(req);
        srs_assert(req->is_complete());
        
        // always free it in this scope.
        SrsAutoFree(SrsHttpMessage, req);
        
        // ok, handle http request.
        if ((ret = process_request(&skt, req)) != ERROR_SUCCESS) {
            return ret;
        }
    }
        
    return ret;
}

int SrsHttpConn::process_request(SrsStSocket* skt, SrsHttpMessage* req) 
{
    int ret = ERROR_SUCCESS;

    // parse uri to schema/server:port/path?query
    if ((ret = req->parse_uri()) != ERROR_SUCCESS) {
        return ret;
    }
    
    srs_trace("HTTP %s %s, content-length=%"PRId64"", 
        req->method_str().c_str(), req->url().c_str(), req->content_length());
    
    // TODO: maybe need to parse the url.
    std::string url = req->path();
    
    SrsHttpHandlerMatch* p = NULL;
    if ((ret = handler->best_match(url.data(), url.length(), &p)) != ERROR_SUCCESS) {
        srs_warn("failed to find the best match handler for url. ret=%d", ret);
        return ret;
    }
    
    // if success, p and pstart should be valid.
    srs_assert(p);
    srs_assert(p->handler);
    srs_assert(p->matched_url.length() <= url.length());
    srs_info("best match handler, matched_url=%s", p->matched_url.c_str());
    
    req->set_match(p);
    req->set_requires_crossdomain(requires_crossdomain);
    
    // use handler to process request.
    if ((ret = p->handler->process_request(skt, req)) != ERROR_SUCCESS) {
        srs_warn("handler failed to process http request. ret=%d", ret);
        return ret;
    }
    
    if (req->requires_crossdomain()) {
        requires_crossdomain = true;
    }
    
    return ret;
}

#endif

