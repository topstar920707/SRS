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

#include <srs_app_http.hpp>

#ifdef SRS_AUTO_HTTP_PARSER

#include <stdlib.h>

using namespace std;

#include <srs_kernel_error.hpp>
#include <srs_kernel_log.hpp>
#include <srs_app_st_socket.hpp>
#include <srs_app_http_api.hpp>
#include <srs_app_http_conn.hpp>
#include <srs_app_json.hpp>
#include <srs_kernel_utility.hpp>
#include <srs_protocol_buffer.hpp>

#define SRS_DEFAULT_HTTP_PORT 80

#define SRS_HTTP_HEADER_BUFFER 1024

// for http parser macros
#define SRS_CONSTS_HTTP_OPTIONS HTTP_OPTIONS
#define SRS_CONSTS_HTTP_GET HTTP_GET
#define SRS_CONSTS_HTTP_POST HTTP_POST
#define SRS_CONSTS_HTTP_PUT HTTP_PUT
#define SRS_CONSTS_HTTP_DELETE HTTP_DELETE

bool srs_path_equals(const char* expect, const char* path, int nb_path)
{
    int size = strlen(expect);
    
    if (size != nb_path) {
        return false;
    }
    
    bool equals = !memcmp(expect, path, size);
    return equals;
}

bool srs_path_like(const char* expect, const char* path, int nb_path)
{
    int size = strlen(expect);
    bool equals = !strncmp(expect, path, srs_min(size, nb_path));
    return equals;
}

int srs_go_http_response_json(ISrsGoHttpResponseWriter* w, string data)
{
    w->header()->set_content_length(data.length());
    w->header()->set_content_type("application/json;charset=utf-8");
    
    return w->write((char*)data.data(), data.length());
}

// get the status text of code.
string srs_generate_status_text(int status)
{
    static std::map<int, std::string> _status_map;
    if (_status_map.empty()) {
        _status_map[SRS_CONSTS_HTTP_Continue                       ] = SRS_CONSTS_HTTP_Continue_str                     ;      
        _status_map[SRS_CONSTS_HTTP_SwitchingProtocols             ] = SRS_CONSTS_HTTP_SwitchingProtocols_str           ;      
        _status_map[SRS_CONSTS_HTTP_OK                             ] = SRS_CONSTS_HTTP_OK_str                           ;      
        _status_map[SRS_CONSTS_HTTP_Created                        ] = SRS_CONSTS_HTTP_Created_str                      ;      
        _status_map[SRS_CONSTS_HTTP_Accepted                       ] = SRS_CONSTS_HTTP_Accepted_str                     ;      
        _status_map[SRS_CONSTS_HTTP_NonAuthoritativeInformation    ] = SRS_CONSTS_HTTP_NonAuthoritativeInformation_str  ;      
        _status_map[SRS_CONSTS_HTTP_NoContent                      ] = SRS_CONSTS_HTTP_NoContent_str                    ;      
        _status_map[SRS_CONSTS_HTTP_ResetContent                   ] = SRS_CONSTS_HTTP_ResetContent_str                 ;      
        _status_map[SRS_CONSTS_HTTP_PartialContent                 ] = SRS_CONSTS_HTTP_PartialContent_str               ;      
        _status_map[SRS_CONSTS_HTTP_MultipleChoices                ] = SRS_CONSTS_HTTP_MultipleChoices_str              ;      
        _status_map[SRS_CONSTS_HTTP_MovedPermanently               ] = SRS_CONSTS_HTTP_MovedPermanently_str             ;      
        _status_map[SRS_CONSTS_HTTP_Found                          ] = SRS_CONSTS_HTTP_Found_str                        ;      
        _status_map[SRS_CONSTS_HTTP_SeeOther                       ] = SRS_CONSTS_HTTP_SeeOther_str                     ;      
        _status_map[SRS_CONSTS_HTTP_NotModified                    ] = SRS_CONSTS_HTTP_NotModified_str                  ;      
        _status_map[SRS_CONSTS_HTTP_UseProxy                       ] = SRS_CONSTS_HTTP_UseProxy_str                     ;      
        _status_map[SRS_CONSTS_HTTP_TemporaryRedirect              ] = SRS_CONSTS_HTTP_TemporaryRedirect_str            ;      
        _status_map[SRS_CONSTS_HTTP_BadRequest                     ] = SRS_CONSTS_HTTP_BadRequest_str                   ;      
        _status_map[SRS_CONSTS_HTTP_Unauthorized                   ] = SRS_CONSTS_HTTP_Unauthorized_str                 ;      
        _status_map[SRS_CONSTS_HTTP_PaymentRequired                ] = SRS_CONSTS_HTTP_PaymentRequired_str              ;      
        _status_map[SRS_CONSTS_HTTP_Forbidden                      ] = SRS_CONSTS_HTTP_Forbidden_str                    ;      
        _status_map[SRS_CONSTS_HTTP_NotFound                       ] = SRS_CONSTS_HTTP_NotFound_str                     ;      
        _status_map[SRS_CONSTS_HTTP_MethodNotAllowed               ] = SRS_CONSTS_HTTP_MethodNotAllowed_str             ;      
        _status_map[SRS_CONSTS_HTTP_NotAcceptable                  ] = SRS_CONSTS_HTTP_NotAcceptable_str                ;      
        _status_map[SRS_CONSTS_HTTP_ProxyAuthenticationRequired    ] = SRS_CONSTS_HTTP_ProxyAuthenticationRequired_str  ;      
        _status_map[SRS_CONSTS_HTTP_RequestTimeout                 ] = SRS_CONSTS_HTTP_RequestTimeout_str               ;      
        _status_map[SRS_CONSTS_HTTP_Conflict                       ] = SRS_CONSTS_HTTP_Conflict_str                     ;      
        _status_map[SRS_CONSTS_HTTP_Gone                           ] = SRS_CONSTS_HTTP_Gone_str                         ;      
        _status_map[SRS_CONSTS_HTTP_LengthRequired                 ] = SRS_CONSTS_HTTP_LengthRequired_str               ;      
        _status_map[SRS_CONSTS_HTTP_PreconditionFailed             ] = SRS_CONSTS_HTTP_PreconditionFailed_str           ;      
        _status_map[SRS_CONSTS_HTTP_RequestEntityTooLarge          ] = SRS_CONSTS_HTTP_RequestEntityTooLarge_str        ;      
        _status_map[SRS_CONSTS_HTTP_RequestURITooLarge             ] = SRS_CONSTS_HTTP_RequestURITooLarge_str           ;      
        _status_map[SRS_CONSTS_HTTP_UnsupportedMediaType           ] = SRS_CONSTS_HTTP_UnsupportedMediaType_str         ;      
        _status_map[SRS_CONSTS_HTTP_RequestedRangeNotSatisfiable   ] = SRS_CONSTS_HTTP_RequestedRangeNotSatisfiable_str ;      
        _status_map[SRS_CONSTS_HTTP_ExpectationFailed              ] = SRS_CONSTS_HTTP_ExpectationFailed_str            ;      
        _status_map[SRS_CONSTS_HTTP_InternalServerError            ] = SRS_CONSTS_HTTP_InternalServerError_str          ;      
        _status_map[SRS_CONSTS_HTTP_NotImplemented                 ] = SRS_CONSTS_HTTP_NotImplemented_str               ;      
        _status_map[SRS_CONSTS_HTTP_BadGateway                     ] = SRS_CONSTS_HTTP_BadGateway_str                   ;      
        _status_map[SRS_CONSTS_HTTP_ServiceUnavailable             ] = SRS_CONSTS_HTTP_ServiceUnavailable_str           ;      
        _status_map[SRS_CONSTS_HTTP_GatewayTimeout                 ] = SRS_CONSTS_HTTP_GatewayTimeout_str               ;      
        _status_map[SRS_CONSTS_HTTP_HTTPVersionNotSupported        ] = SRS_CONSTS_HTTP_HTTPVersionNotSupported_str      ;      
    }
    
    std::string status_text;
    if (_status_map.find(status) == _status_map.end()) {
        status_text = "Status Unknown";
    } else {
        status_text = _status_map[status];
    }
    
    return status_text;
}

// bodyAllowedForStatus reports whether a given response status code
// permits a body.  See RFC2616, section 4.4.
bool srs_go_http_body_allowd(int status)
{
    if (status >= 100 && status <= 199) {
        return false;
    } else if (status == 204 || status == 304) {
        return false;
    }
    
	return true;
}

// DetectContentType implements the algorithm described
// at http://mimesniff.spec.whatwg.org/ to determine the
// Content-Type of the given data.  It considers at most the
// first 512 bytes of data.  DetectContentType always returns
// a valid MIME type: if it cannot determine a more specific one, it
// returns "application/octet-stream".
string srs_go_http_detect(char* data, int size)
{
    return "application/octet-stream"; // fallback
}

// Error replies to the request with the specified error message and HTTP code.
// The error message should be plain text.
int srs_go_http_error(ISrsGoHttpResponseWriter* w, int code, string error)
{
    int ret = ERROR_SUCCESS;
    
    w->header()->set_content_type("text/plain; charset=utf-8");
    w->header()->set_content_length(error.length());
    w->write_header(code);
    w->write((char*)error.data(), (int)error.length());
    
    return ret;
}

SrsGoHttpHeader::SrsGoHttpHeader()
{
}

SrsGoHttpHeader::~SrsGoHttpHeader()
{
}

void SrsGoHttpHeader::set(string key, string value)
{
    headers[key] = value;
}

string SrsGoHttpHeader::get(string key)
{
    std::string v;
    
    if (headers.find(key) != headers.end()) {
        v = headers[key];
    }
    
    return v;
}

int64_t SrsGoHttpHeader::content_length()
{
    std::string cl = get("Content-Length");
    
    if (cl.empty()) {
        return -1;
    }
    
    return (int64_t)::atof(cl.c_str());
}

void SrsGoHttpHeader::set_content_length(int64_t size)
{
    char buf[64];
    snprintf(buf, sizeof(buf), "%"PRId64, size);
    set("Content-Length", buf);
}

string SrsGoHttpHeader::content_type()
{
    return get("Content-Type");
}

void SrsGoHttpHeader::set_content_type(string ct)
{
    set("Content-Type", ct);
}

void SrsGoHttpHeader::write(stringstream& ss)
{
    std::map<std::string, std::string>::iterator it;
    for (it = headers.begin(); it != headers.end(); ++it) {
        ss << it->first << ": " << it->second << __SRS_CRLF;
    }
}

ISrsGoHttpResponseWriter::ISrsGoHttpResponseWriter()
{
}

ISrsGoHttpResponseWriter::~ISrsGoHttpResponseWriter()
{
}

ISrsGoHttpHandler::ISrsGoHttpHandler()
{
}

ISrsGoHttpHandler::~ISrsGoHttpHandler()
{
}

SrsGoHttpRedirectHandler::SrsGoHttpRedirectHandler(string u, int c)
{
    url = u;
    code = c;
}

SrsGoHttpRedirectHandler::~SrsGoHttpRedirectHandler()
{
}

int SrsGoHttpRedirectHandler::serve_http(ISrsGoHttpResponseWriter* w, SrsHttpMessage* r)
{
    int ret = ERROR_SUCCESS;
    // TODO: FIXME: implements it.
    return ret;
}

SrsGoHttpNotFoundHandler::SrsGoHttpNotFoundHandler()
{
}

SrsGoHttpNotFoundHandler::~SrsGoHttpNotFoundHandler()
{
}

int SrsGoHttpNotFoundHandler::serve_http(ISrsGoHttpResponseWriter* w, SrsHttpMessage* r)
{
    return srs_go_http_error(w, 
        SRS_CONSTS_HTTP_NotFound, SRS_CONSTS_HTTP_NotFound_str);
}

SrsGoHttpMuxEntry::SrsGoHttpMuxEntry()
{
    explicit_match = false;
    handler = NULL;
}

SrsGoHttpMuxEntry::~SrsGoHttpMuxEntry()
{
    srs_freep(handler);
}

SrsGoHttpServeMux::SrsGoHttpServeMux()
{
}

SrsGoHttpServeMux::~SrsGoHttpServeMux()
{
    std::map<std::string, SrsGoHttpMuxEntry*>::iterator it;
    for (it = entries.begin(); it != entries.end(); ++it) {
        SrsGoHttpMuxEntry* entry = it->second;
        srs_freep(entry);
    }
    entries.clear();
}

int SrsGoHttpServeMux::initialize()
{
    int ret = ERROR_SUCCESS;
    // TODO: FIXME: implements it.
    return ret;
}

int SrsGoHttpServeMux::handle(std::string pattern, ISrsGoHttpHandler* handler)
{
    int ret = ERROR_SUCCESS;
    
    srs_assert(handler);
    
    if (pattern.empty()) {
        ret = ERROR_HTTP_PATTERN_EMPTY;
        srs_error("http: empty pattern. ret=%d", ret);
        return ret;
    }
    
    if (entries.find(pattern) != entries.end()) {
        SrsGoHttpMuxEntry* exists = entries[pattern];
        if (exists->explicit_match) {
            ret = ERROR_HTTP_PATTERN_DUPLICATED;
            srs_error("http: multiple registrations for %s. ret=%d", pattern.c_str(), ret);
            return ret;
        }
    }
    
    if (true) {
        SrsGoHttpMuxEntry* entry = new SrsGoHttpMuxEntry();
        entry->explicit_match = true;
        entry->handler = handler;
        entry->pattern = pattern;
        
        if (entries.find(pattern) != entries.end()) {
            SrsGoHttpMuxEntry* exists = entries[pattern];
            srs_freep(exists);
        }
        entries[pattern] = entry;
    }

    // Helpful behavior:
    // If pattern is /tree/, insert an implicit permanent redirect for /tree.
    // It can be overridden by an explicit registration.
    if (!pattern.empty() && pattern.at(pattern.length() - 1) == '/') {
        std::string rpattern = pattern.substr(0, pattern.length() - 1);
        SrsGoHttpMuxEntry* entry = NULL;
        
        // free the exists not explicit entry
        if (entries.find(rpattern) != entries.end()) {
            SrsGoHttpMuxEntry* exists = entries[rpattern];
            if (!exists->explicit_match) {
                entry = exists;
            }
        }

        // create implicit redirect.
        if (!entry || entry->explicit_match) {
            srs_freep(entry);
            
            entry = new SrsGoHttpMuxEntry();
            entry->explicit_match = false;
            entry->handler = new SrsGoHttpRedirectHandler(pattern, SRS_CONSTS_HTTP_MovedPermanently);
            entry->pattern = pattern;
            
            entries[rpattern] = entry;
        }
    }
    
    return ret;
}

int SrsGoHttpServeMux::serve_http(ISrsGoHttpResponseWriter* w, SrsHttpMessage* r)
{
    int ret = ERROR_SUCCESS;
    
    ISrsGoHttpHandler* h = NULL;
    if ((ret = find_handler(r, &h)) != ERROR_SUCCESS) {
        srs_error("find handler failed. ret=%d", ret);
        return ret;
    }
    
    srs_assert(h);
    if ((ret = h->serve_http(w, r)) != ERROR_SUCCESS) {
        srs_error("handler serve http failed. ret=%d", ret);
        return ret;
    }
    
    return ret;
}

int SrsGoHttpServeMux::find_handler(SrsHttpMessage* r, ISrsGoHttpHandler** ph)
{
    int ret = ERROR_SUCCESS;
    
    // TODO: FIXME: support the path . and ..
    if (r->url().find("..") != std::string::npos) {
        ret = ERROR_HTTP_URL_NOT_CLEAN;
        srs_error("htt url not canonical, url=%s. ret=%d", r->url().c_str(), ret);
        return ret;
    }
    
    if ((ret = match(r, ph)) != ERROR_SUCCESS) {
        srs_error("http match handler failed. ret=%d", ret);
        return ret;
    }

    if (*ph == NULL) {
        *ph = new SrsGoHttpNotFoundHandler();
    }
    
    return ret;
}

int SrsGoHttpServeMux::match(SrsHttpMessage* r, ISrsGoHttpHandler** ph)
{
    int ret = ERROR_SUCCESS;
    
    std::string path = r->path();
    
    int nb_matched = 0;
    ISrsGoHttpHandler* h = NULL;
    
    std::map<std::string, SrsGoHttpMuxEntry*>::iterator it;
    for (it = entries.begin(); it != entries.end(); ++it) {
        std::string pattern = it->first;
        SrsGoHttpMuxEntry* entry = it->second;
        
        if (!path_match(pattern, path)) {
            continue;
        }
        
        if (!h || (int)pattern.length() > nb_matched) {
            nb_matched = (int)pattern.length();
            h = entry->handler;
        }
    }
    
    *ph = h;
    
    return ret;
}

bool SrsGoHttpServeMux::path_match(string pattern, string path)
{
    if (pattern.empty()) {
        return false;
    }
    
    int n = pattern.length();
    
    // not endswith '/', exactly match.
    if (pattern.at(n - 1) != '/') {
        return pattern == path;
    }
    
    // endswith '/', match any,
    // for example, '/api/' match '/api/[N]'
    if ((int)path.length() >= n) {
        if (memcmp(pattern.data(), path.data(), n) == 0) {
            return true;
        }
    }
    
    return false;
}

SrsGoHttpResponseWriter::SrsGoHttpResponseWriter(SrsStSocket* io)
{
    skt = io;
    hdr = new SrsGoHttpHeader();
    header_wrote = false;
    status = SRS_CONSTS_HTTP_OK;
    content_length = -1;
    written = 0;
    header_sent = false;
}

SrsGoHttpResponseWriter::~SrsGoHttpResponseWriter()
{
    srs_freep(hdr);
}

SrsGoHttpHeader* SrsGoHttpResponseWriter::header()
{
    return hdr;
}

int SrsGoHttpResponseWriter::write(char* data, int size)
{
    int ret = ERROR_SUCCESS;
    
    if (!header_wrote) {
        write_header(SRS_CONSTS_HTTP_OK);
    }
    
    written += size;
    if (content_length != -1 && written > content_length) {
        ret = ERROR_HTTP_CONTENT_LENGTH;
        srs_error("http: exceed content length. ret=%d", ret);
        return ret;
    }
    
    if ((ret = send_header(data, size)) != ERROR_SUCCESS) {
        srs_error("http: send header failed. ret=%d", ret);
        return ret;
    }
    
    return skt->write((void*)data, size, NULL);
}

void SrsGoHttpResponseWriter::write_header(int code)
{
    if (header_wrote) {
        srs_warn("http: multiple write_header calls, code=%d", code);
        return;
    }
    
    header_wrote = true;
    status = code;
    
    // parse the content length from header.
    content_length = hdr->content_length();
}

int SrsGoHttpResponseWriter::send_header(char* data, int size)
{
    int ret = ERROR_SUCCESS;
    
    if (header_sent) {
        return ret;
    }
    header_sent = true;
    
    std::stringstream ss;
    
    // status_line
    ss << "HTTP/1.1 " << status << " " 
        << srs_generate_status_text(status) << __SRS_CRLF;
        
    // detect content type
    if (srs_go_http_body_allowd(status)) {
        if (hdr->content_type().empty()) {
            hdr->set_content_type(srs_go_http_detect(data, size));
        }
    }
    
    // set server if not set.
    if (hdr->get("Server").empty()) {
        hdr->set("Server", RTMP_SIG_SRS_KEY"/"RTMP_SIG_SRS_VERSION);
    }
    
    // write headers
    hdr->write(ss);
    
    // header_eof
    ss << __SRS_CRLF;
    
    std::string buf = ss.str();
    return skt->write((void*)buf.c_str(), buf.length(), NULL);
}

SrsHttpHandlerMatch::SrsHttpHandlerMatch()
{
    handler = NULL;
}

SrsHttpHandler::SrsHttpHandler()
{
}

SrsHttpHandler::~SrsHttpHandler()
{
    std::vector<SrsHttpHandler*>::iterator it;
    for (it = handlers.begin(); it != handlers.end(); ++it) {
        SrsHttpHandler* handler = *it;
        srs_freep(handler);
    }
    handlers.clear();
}

int SrsHttpHandler::initialize()
{
    int ret = ERROR_SUCCESS;
    return ret;
}

bool SrsHttpHandler::can_handle(const char* /*path*/, int /*length*/, const char** /*pchild*/)
{
    return false;
}

int SrsHttpHandler::process_request(SrsStSocket* skt, SrsHttpMessage* req)
{
    if (req->method() == SRS_CONSTS_HTTP_OPTIONS) {
        req->set_requires_crossdomain(true);
        return res_options(skt);
    }

    int status_code;
    std::string reason_phrase;
    if (!is_handler_valid(req, status_code, reason_phrase)) {
        std::stringstream ss;
        
        ss << __SRS_JOBJECT_START
            << __SRS_JFIELD_ERROR(ERROR_HTTP_HANDLER_INVALID) << __SRS_JFIELD_CONT
            << __SRS_JFIELD_ORG("data", __SRS_JOBJECT_START)
                << __SRS_JFIELD_ORG("status_code", status_code) << __SRS_JFIELD_CONT
                << __SRS_JFIELD_STR("reason_phrase", reason_phrase) << __SRS_JFIELD_CONT
                << __SRS_JFIELD_STR("url", req->url())
            << __SRS_JOBJECT_END
            << __SRS_JOBJECT_END;
        
        return res_error(skt, req, status_code, reason_phrase, ss.str());
    }
    
    return do_process_request(skt, req);
}

bool SrsHttpHandler::is_handler_valid(SrsHttpMessage* req, int& status_code, string& reason_phrase) 
{
    if (!req->match()->unmatched_url.empty()) {
        status_code = SRS_CONSTS_HTTP_NotFound;
        reason_phrase = SRS_CONSTS_HTTP_NotFound_str;
        
        return false;
    }
    
    return true;
}

int SrsHttpHandler::do_process_request(SrsStSocket* /*skt*/, SrsHttpMessage* /*req*/)
{
    int ret = ERROR_SUCCESS;
    return ret;
}

int SrsHttpHandler::response_error(SrsStSocket* skt, SrsHttpMessage* req, int code, string desc)
{
    std::stringstream ss;
    ss << __SRS_JOBJECT_START
        << __SRS_JFIELD_ERROR(code) << __SRS_JFIELD_CONT
        << __SRS_JFIELD_STR("desc", desc)
        << __SRS_JOBJECT_END;
    
    return res_json(skt, req, ss.str());
}

int SrsHttpHandler::best_match(const char* path, int length, SrsHttpHandlerMatch** ppmatch)
{
    int ret = ERROR_SUCCESS;
    
    SrsHttpHandler* handler = NULL;
    const char* match_start = NULL;
    int match_length = 0;
    
    for (;;) {
        // ensure cur is not NULL.
        // ensure p not NULL and has bytes to parse.
        if (!path || length <= 0) {
            break;
        }
        
        const char* p = NULL;
        for (p = path + 1; p - path < length && *p != SRS_CONSTS_HTTP_PATH_SEP; p++) {
        }
        
        // whether the handler can handler the node.
        const char* pchild = p;
        if (!can_handle(path, p - path, &pchild)) {
            break;
        }
        
        // save current handler, it's ok for current handler atleast.
        handler = this;
        match_start = path;
        match_length = p - path;
        
        // find the best matched child handler.
        std::vector<SrsHttpHandler*>::iterator it;
        for (it = handlers.begin(); it != handlers.end(); ++it) {
            SrsHttpHandler* h = *it;
            
            // matched, donot search more.
            if (h->best_match(pchild, length - (pchild - path), ppmatch) == ERROR_SUCCESS) {
                break;
            }
        }
        
        // whatever, donot loop.
        break;
    }
    
    // if already matched by child, return.
    if (*ppmatch) {
        return ret;
    }
    
    // not matched, error.
    if (handler == NULL) {
        ret = ERROR_HTTP_HANDLER_MATCH_URL;
        return ret;
    }
    
    // matched by this handler.
    *ppmatch = new SrsHttpHandlerMatch();
    (*ppmatch)->handler = handler;
    (*ppmatch)->matched_url.append(match_start, match_length);
    
    int unmatch_length = length - match_length;
    if (unmatch_length > 0) {
        (*ppmatch)->unmatched_url.append(match_start + match_length, unmatch_length);
    }
    
    return ret;
}

SrsHttpHandler* SrsHttpHandler::res_status_line(stringstream& ss)
{
    ss << "HTTP/1.1 200 OK " << __SRS_CRLF
       << "Server: "RTMP_SIG_SRS_KEY"/"RTMP_SIG_SRS_VERSION"" << __SRS_CRLF;
    return this;
}

SrsHttpHandler* SrsHttpHandler::res_status_line_error(stringstream& ss, int code, string reason_phrase)
{
    ss << "HTTP/1.1 " << code << " " << reason_phrase << __SRS_CRLF
       << "Server: SRS/"RTMP_SIG_SRS_VERSION"" << __SRS_CRLF;
    return this;
}

SrsHttpHandler* SrsHttpHandler::res_content_type(stringstream& ss)
{
    ss << "Content-Type: text/html;charset=utf-8" << __SRS_CRLF
        << "Allow: DELETE, GET, HEAD, OPTIONS, POST, PUT" << __SRS_CRLF;
    return this;
}

SrsHttpHandler* SrsHttpHandler::res_content_type_xml(stringstream& ss)
{
    ss << "Content-Type: text/xml;charset=utf-8" << __SRS_CRLF
        << "Allow: DELETE, GET, HEAD, OPTIONS, POST, PUT" << __SRS_CRLF;
    return this;
}

SrsHttpHandler* SrsHttpHandler::res_content_type_javascript(stringstream& ss)
{
    ss << "Content-Type: text/javascript" << __SRS_CRLF
        << "Allow: DELETE, GET, HEAD, OPTIONS, POST, PUT" << __SRS_CRLF;
    return this;
}

SrsHttpHandler* SrsHttpHandler::res_content_type_swf(stringstream& ss)
{
    ss << "Content-Type: application/x-shockwave-flash" << __SRS_CRLF
        << "Allow: DELETE, GET, HEAD, OPTIONS, POST, PUT" << __SRS_CRLF;
    return this;
}

SrsHttpHandler* SrsHttpHandler::res_content_type_css(stringstream& ss)
{
    ss << "Content-Type: text/css;charset=utf-8" << __SRS_CRLF
        << "Allow: DELETE, GET, HEAD, OPTIONS, POST, PUT" << __SRS_CRLF;
    return this;
}

SrsHttpHandler* SrsHttpHandler::res_content_type_ico(stringstream& ss)
{
    ss << "Content-Type: image/x-icon" << __SRS_CRLF
        << "Allow: DELETE, GET, HEAD, OPTIONS, POST, PUT" << __SRS_CRLF;
    return this;
}

SrsHttpHandler* SrsHttpHandler::res_content_type_json(stringstream& ss)
{
    ss << "Content-Type: application/json;charset=utf-8" << __SRS_CRLF
        << "Allow: DELETE, GET, HEAD, OPTIONS, POST, PUT" << __SRS_CRLF;
    return this;
}

SrsHttpHandler* SrsHttpHandler::res_content_type_m3u8(stringstream& ss)
{
    ss << "Content-Type: application/x-mpegURL;charset=utf-8" << __SRS_CRLF
        << "Allow: DELETE, GET, HEAD, OPTIONS, POST, PUT" << __SRS_CRLF;
    return this;
}

SrsHttpHandler* SrsHttpHandler::res_content_type_mpegts(stringstream& ss)
{
    ss << "Content-Type: video/MP2T" << __SRS_CRLF
        << "Allow: DELETE, GET, HEAD, OPTIONS, POST, PUT" << __SRS_CRLF;
    return this;
}

SrsHttpHandler* SrsHttpHandler::res_content_type_flv(stringstream& ss)
{
    ss << "Content-Type: video/x-flv" << __SRS_CRLF
        << "Allow: DELETE, GET, HEAD, OPTIONS, POST, PUT" << __SRS_CRLF;
    return this;
}

SrsHttpHandler* SrsHttpHandler::res_content_length(stringstream& ss, int64_t length)
{
    ss << "Content-Length: "<< length << __SRS_CRLF;
    return this;
}

SrsHttpHandler* SrsHttpHandler::res_enable_crossdomain(stringstream& ss)
{
    ss << "Access-Control-Allow-Origin: *" << __SRS_CRLF
        << "Access-Control-Allow-Methods: "
        << "GET, POST, HEAD, PUT, DELETE" << __SRS_CRLF
        << "Access-Control-Allow-Headers: "
        << "Cache-Control,X-Proxy-Authorization,X-Requested-With,Content-Type" << __SRS_CRLF;
    return this;
}

SrsHttpHandler* SrsHttpHandler::res_header_eof(stringstream& ss)
{
    ss << __SRS_CRLF;
    return this;
}

SrsHttpHandler* SrsHttpHandler::res_body(stringstream& ss, string body)
{
    ss << body;
    return this;
}

int SrsHttpHandler::res_flush(SrsStSocket* skt, stringstream& ss)
{
    return skt->write((void*)ss.str().c_str(), ss.str().length(), NULL);
}

int SrsHttpHandler::res_options(SrsStSocket* skt)
{
    std::stringstream ss;
    
    res_status_line(ss)->res_content_type(ss)
        ->res_content_length(ss, 0)->res_enable_crossdomain(ss)
        ->res_header_eof(ss);
    
    return res_flush(skt, ss);
}

int SrsHttpHandler::res_text(SrsStSocket* skt, SrsHttpMessage* req, string body)
{
    std::stringstream ss;
    
    res_status_line(ss)->res_content_type(ss)
        ->res_content_length(ss, (int)body.length());
        
    if (req->requires_crossdomain()) {
        res_enable_crossdomain(ss);
    }
    
    res_header_eof(ss)
        ->res_body(ss, body);
    
    return res_flush(skt, ss);
}

int SrsHttpHandler::res_xml(SrsStSocket* skt, SrsHttpMessage* req, string body)
{
    std::stringstream ss;
    
    res_status_line(ss)->res_content_type_xml(ss)
        ->res_content_length(ss, (int)body.length());
        
    if (req->requires_crossdomain()) {
        res_enable_crossdomain(ss);
    }
    
    res_header_eof(ss)
        ->res_body(ss, body);
    
    return res_flush(skt, ss);
}

int SrsHttpHandler::res_javascript(SrsStSocket* skt, SrsHttpMessage* req, string body)
{
    std::stringstream ss;
    
    res_status_line(ss)->res_content_type_javascript(ss)
        ->res_content_length(ss, (int)body.length());
        
    if (req->requires_crossdomain()) {
        res_enable_crossdomain(ss);
    }
    
    res_header_eof(ss)
        ->res_body(ss, body);
    
    return res_flush(skt, ss);
}

int SrsHttpHandler::res_swf(SrsStSocket* skt, SrsHttpMessage* req, string body)
{
    std::stringstream ss;
    
    res_status_line(ss)->res_content_type_swf(ss)
        ->res_content_length(ss, (int)body.length());
        
    if (req->requires_crossdomain()) {
        res_enable_crossdomain(ss);
    }
    
    res_header_eof(ss)
        ->res_body(ss, body);
    
    return res_flush(skt, ss);
}

int SrsHttpHandler::res_css(SrsStSocket* skt, SrsHttpMessage* req, string body)
{
    std::stringstream ss;
    
    res_status_line(ss)->res_content_type_css(ss)
        ->res_content_length(ss, (int)body.length());
        
    if (req->requires_crossdomain()) {
        res_enable_crossdomain(ss);
    }
    
    res_header_eof(ss)
        ->res_body(ss, body);
    
    return res_flush(skt, ss);
}

int SrsHttpHandler::res_ico(SrsStSocket* skt, SrsHttpMessage* req, string body)
{
    std::stringstream ss;
    
    res_status_line(ss)->res_content_type_ico(ss)
        ->res_content_length(ss, (int)body.length());
        
    if (req->requires_crossdomain()) {
        res_enable_crossdomain(ss);
    }
    
    res_header_eof(ss)
        ->res_body(ss, body);
    
    return res_flush(skt, ss);
}

int SrsHttpHandler::res_m3u8(SrsStSocket* skt, SrsHttpMessage* req, string body)
{
    std::stringstream ss;
    
    res_status_line(ss)->res_content_type_m3u8(ss)
        ->res_content_length(ss, (int)body.length());
        
    if (req->requires_crossdomain()) {
        res_enable_crossdomain(ss);
    }
    
    res_header_eof(ss)
        ->res_body(ss, body);
    
    return res_flush(skt, ss);
}

int SrsHttpHandler::res_mpegts(SrsStSocket* skt, SrsHttpMessage* req, string body)
{
    std::stringstream ss;
    
    res_status_line(ss)->res_content_type_mpegts(ss)
        ->res_content_length(ss, (int)body.length());
        
    if (req->requires_crossdomain()) {
        res_enable_crossdomain(ss);
    }
    
    res_header_eof(ss)
        ->res_body(ss, body);
    
    return res_flush(skt, ss);
}

int SrsHttpHandler::res_json(SrsStSocket* skt, SrsHttpMessage* req, string json)
{
    std::stringstream ss;
    
    res_status_line(ss)->res_content_type_json(ss)
        ->res_content_length(ss, (int)json.length());
        
    if (req->requires_crossdomain()) {
        res_enable_crossdomain(ss);
    }
    
    res_header_eof(ss)
        ->res_body(ss, json);
    
    return res_flush(skt, ss);
}

int SrsHttpHandler::res_error(SrsStSocket* skt, SrsHttpMessage* req, int code, string reason_phrase, string body)
{
    std::stringstream ss;

    res_status_line_error(ss, code, reason_phrase)->res_content_type_json(ss)
        ->res_content_length(ss, (int)body.length());
        
    if (req->requires_crossdomain()) {
        res_enable_crossdomain(ss);
    }
    
    res_header_eof(ss)
        ->res_body(ss, body);
    
    return res_flush(skt, ss);
}

#ifdef SRS_AUTO_HTTP_SERVER
SrsHttpHandler* SrsHttpHandler::create_http_stream()
{
    return new SrsHttpRoot();
}
#endif

SrsHttpMessage::SrsHttpMessage()
{
    _body = new SrsSimpleBuffer();
    _state = SrsHttpParseStateInit;
    _uri = new SrsHttpUri();
    _match = NULL;
    _requires_crossdomain = false;
    _http_ts_send_buffer = new char[__SRS_HTTP_TS_SEND_BUFFER_SIZE];
}

SrsHttpMessage::~SrsHttpMessage()
{
    srs_freep(_body);
    srs_freep(_uri);
    srs_freep(_match);
    srs_freep(_http_ts_send_buffer);
}

int SrsHttpMessage::initialize()
{
    int ret = ERROR_SUCCESS;
    
    // parse uri to schema/server:port/path?query
    if ((ret = _uri->initialize(_url)) != ERROR_SUCCESS) {
        return ret;
    }
    
    // must format as key=value&...&keyN=valueN
    std::string q = _uri->get_query();
    size_t pos = string::npos;
    while (!q.empty()) {
        std::string k = q;
        if ((pos = q.find("=")) != string::npos) {
            k = q.substr(0, pos);
            q = q.substr(pos + 1);
        } else {
            q = "";
        }
        
        std::string v = q;
        if ((pos = q.find("&")) != string::npos) {
            v = q.substr(0, pos);
            q = q.substr(pos + 1);
        } else {
            q = "";
        }
        
        _query[k] = v;
    }
    
    return ret;
}

char* SrsHttpMessage::http_ts_send_buffer()
{
    return _http_ts_send_buffer;
}

void SrsHttpMessage::reset()
{
    _state = SrsHttpParseStateInit;
    _body->erase(_body->length());
    _url = "";
}

bool SrsHttpMessage::is_complete()
{
    return _state == SrsHttpParseStateComplete;
}

u_int8_t SrsHttpMessage::method()
{
    return (u_int8_t)_header.method;
}

u_int16_t SrsHttpMessage::status_code()
{
    return (u_int16_t)_header.status_code;
}

string SrsHttpMessage::method_str()
{
    if (is_http_get()) {
        return "GET";
    }
    if (is_http_put()) {
        return "PUT";
    }
    if (is_http_post()) {
        return "POST";
    }
    if (is_http_delete()) {
        return "DELETE";
    }
    if (is_http_options()) {
        return "OPTIONS";
    }
    
    return "OTHER";
}

bool SrsHttpMessage::is_http_get()
{
    return _header.method == SRS_CONSTS_HTTP_GET;
}

bool SrsHttpMessage::is_http_put()
{
    return _header.method == SRS_CONSTS_HTTP_PUT;
}

bool SrsHttpMessage::is_http_post()
{
    return _header.method == SRS_CONSTS_HTTP_POST;
}

bool SrsHttpMessage::is_http_delete()
{
    return _header.method == SRS_CONSTS_HTTP_DELETE;
}

bool SrsHttpMessage::is_http_options()
{
    return _header.method == SRS_CONSTS_HTTP_OPTIONS;
}

string SrsHttpMessage::uri()
{
    std::string uri = _uri->get_schema();
    if (uri.empty()) {
        uri += "http://";
    }
    
    uri += host();
    uri += path();
    return uri;
}

string SrsHttpMessage::url()
{
    return _uri->get_url();
}

string SrsHttpMessage::host()
{
    return get_request_header("Host");
}

string SrsHttpMessage::path()
{
    return _uri->get_path();
}

string SrsHttpMessage::body()
{
    std::string b;
    
    if (_body && _body->length() > 0) {
        b.append(_body->bytes(), _body->length());
    }
    
    return b;
}

char* SrsHttpMessage::body_raw()
{
    return _body? _body->bytes() : NULL;
}

int64_t SrsHttpMessage::body_size()
{
    return (int64_t)_body->length();
}

int64_t SrsHttpMessage::content_length()
{
    return _header.content_length;
}

SrsHttpHandlerMatch* SrsHttpMessage::match()
{
    return _match;
}

bool SrsHttpMessage::requires_crossdomain()
{
    return _requires_crossdomain;
}

void SrsHttpMessage::set_url(string url)
{
    _url = url;
}

void SrsHttpMessage::set_state(SrsHttpParseState state)
{
    _state = state;
}

void SrsHttpMessage::set_header(http_parser* header)
{
    memcpy(&_header, header, sizeof(http_parser));
}

void SrsHttpMessage::set_match(SrsHttpHandlerMatch* match)
{
    srs_freep(_match);
    _match = match;
}

void SrsHttpMessage::set_requires_crossdomain(bool requires_crossdomain)
{
    _requires_crossdomain = requires_crossdomain;
}

void SrsHttpMessage::append_body(const char* body, int length)
{
    _body->append(body, length);
}

string SrsHttpMessage::query_get(string key)
{
    std::string v;
    
    if (_query.find(key) != _query.end()) {
        v = _query[key];
    }
    
    return v;
}

int SrsHttpMessage::request_header_count()
{
    return (int)headers.size();
}

string SrsHttpMessage::request_header_key_at(int index)
{
    srs_assert(index < request_header_count());
    SrsHttpHeaderField item = headers[index];
    return item.first;
}

string SrsHttpMessage::request_header_value_at(int index)
{
    srs_assert(index < request_header_count());
    SrsHttpHeaderField item = headers[index];
    return item.second;
}

void SrsHttpMessage::set_request_header(string key, string value)
{
    headers.push_back(std::make_pair(key, value));
}

string SrsHttpMessage::get_request_header(string name)
{
    std::vector<SrsHttpHeaderField>::iterator it;
    
    for (it = headers.begin(); it != headers.end(); ++it) {
        SrsHttpHeaderField& elem = *it;
        std::string key = elem.first;
        std::string value = elem.second;
        if (key == name) {
            return value;
        }
    }
    
    return "";
}

SrsHttpParser::SrsHttpParser()
{
    msg = NULL;
}

SrsHttpParser::~SrsHttpParser()
{
    srs_freep(msg);
}

int SrsHttpParser::initialize(enum http_parser_type type)
{
    int ret = ERROR_SUCCESS;
    
    memset(&settings, 0, sizeof(settings));
    settings.on_message_begin = on_message_begin;
    settings.on_url = on_url;
    settings.on_header_field = on_header_field;
    settings.on_header_value = on_header_value;
    settings.on_headers_complete = on_headers_complete;
    settings.on_body = on_body;
    settings.on_message_complete = on_message_complete;
    
    http_parser_init(&parser, type);
    // callback object ptr.
    parser.data = (void*)this;
    
    return ret;
}

int SrsHttpParser::parse_message(SrsStSocket* skt, SrsHttpMessage** ppmsg)
{
    *ppmsg = NULL;
    
    int ret = ERROR_SUCCESS;

    // the msg must be always NULL
    srs_assert(msg == NULL);
    msg = new SrsHttpMessage();
    
    // reset request data.
    filed_name = "";
    
    // reset response header.
    msg->reset();
    
    // do parse
    if ((ret = parse_message_imp(skt)) != ERROR_SUCCESS) {
        if (!srs_is_client_gracefully_close(ret)) {
            srs_error("parse http msg failed. ret=%d", ret);
        }
        srs_freep(msg);
        return ret;
    }

    // initalize http msg, parse url.
    if ((ret = msg->initialize()) != ERROR_SUCCESS) {
        srs_error("initialize http msg failed. ret=%d", ret);
        srs_freep(msg);
        return ret;
    }
    
    // parse ok, return the msg.
    *ppmsg = msg;
    msg = NULL;
    
    return ret;
}

int SrsHttpParser::parse_message_imp(SrsStSocket* skt)
{
    int ret = ERROR_SUCCESS;
    
    // the msg should never be NULL
    srs_assert(msg != NULL);
    
    // parser header.
    char buf[SRS_HTTP_HEADER_BUFFER];
    for (;;) {
        ssize_t nread;
        if ((ret = skt->read(buf, (size_t)sizeof(buf), &nread)) != ERROR_SUCCESS) {
            if (!srs_is_client_gracefully_close(ret)) {
                srs_error("read body from server failed. ret=%d", ret);
            }
            return ret;
        }
        
        ssize_t nparsed = http_parser_execute(&parser, &settings, buf, nread);
        srs_info("read_size=%d, nparsed=%d", (int)nread, (int)nparsed);

        // check header size.
        if (msg->is_complete()) {
            return ret;
        }
        
        if (nparsed != nread) {
            ret = ERROR_HTTP_PARSE_HEADER;
            srs_error("parse response error, parsed(%d)!=read(%d), ret=%d", (int)nparsed, (int)nread, ret);
            return ret;
        }
    }

    return ret;
}

int SrsHttpParser::on_message_begin(http_parser* parser)
{
    SrsHttpParser* obj = (SrsHttpParser*)parser->data;
    obj->msg->set_state(SrsHttpParseStateStart);
    
    srs_info("***MESSAGE BEGIN***");
    
    return 0;
}

int SrsHttpParser::on_headers_complete(http_parser* parser)
{
    SrsHttpParser* obj = (SrsHttpParser*)parser->data;
    obj->msg->set_header(parser);
    
    srs_info("***HEADERS COMPLETE***");
    
    // see http_parser.c:1570, return 1 to skip body.
    return 0;
}

int SrsHttpParser::on_message_complete(http_parser* parser)
{
    SrsHttpParser* obj = (SrsHttpParser*)parser->data;
    // save the parser when header parse completed.
    obj->msg->set_state(SrsHttpParseStateComplete);
    
    srs_info("***MESSAGE COMPLETE***\n");
    
    return 0;
}

int SrsHttpParser::on_url(http_parser* parser, const char* at, size_t length)
{
    SrsHttpParser* obj = (SrsHttpParser*)parser->data;
    
    if (length > 0) {
        std::string url;
        
        url.append(at, (int)length);
        
        obj->msg->set_url(url);
    }
    
    srs_info("Method: %d, Url: %.*s", parser->method, (int)length, at);
    
    return 0;
}

int SrsHttpParser::on_header_field(http_parser* parser, const char* at, size_t length)
{
    SrsHttpParser* obj = (SrsHttpParser*)parser->data;
    
    if (length > 0) {
        srs_assert(obj);
        obj->filed_name.append(at, (int)length);
    }
    
    srs_info("Header field: %.*s", (int)length, at);
    return 0;
}

int SrsHttpParser::on_header_value(http_parser* parser, const char* at, size_t length)
{
    SrsHttpParser* obj = (SrsHttpParser*)parser->data;
    
    if (length > 0) {
        srs_assert(obj);
        srs_assert(obj->msg);
        
        std::string field_value;
        field_value.append(at, (int)length);

        obj->msg->set_request_header(obj->filed_name, field_value);
        obj->filed_name = "";
    }
    
    srs_info("Header value: %.*s", (int)length, at);
    return 0;
}

int SrsHttpParser::on_body(http_parser* parser, const char* at, size_t length)
{
    SrsHttpParser* obj = (SrsHttpParser*)parser->data;
    
    if (length > 0) {
        srs_assert(obj);
        srs_assert(obj->msg);
        
        obj->msg->append_body(at, (int)length);
    }
    
    srs_info("Body: %.*s", (int)length, at);

    return 0;
}

SrsHttpUri::SrsHttpUri()
{
    port = SRS_DEFAULT_HTTP_PORT;
}

SrsHttpUri::~SrsHttpUri()
{
}

int SrsHttpUri::initialize(string _url)
{
    int ret = ERROR_SUCCESS;
    
    url = _url;
    const char* purl = url.c_str();
    
    http_parser_url hp_u;
    if((ret = http_parser_parse_url(purl, url.length(), 0, &hp_u)) != 0){
        int code = ret;
        ret = ERROR_HTTP_PARSE_URI;
        
        srs_error("parse url %s failed, code=%d, ret=%d", purl, code, ret);
        return ret;
    }
    
    std::string field = get_uri_field(url, &hp_u, UF_SCHEMA);
    if(!field.empty()){
        schema = field;
    }
    
    host = get_uri_field(url, &hp_u, UF_HOST);
    
    field = get_uri_field(url, &hp_u, UF_PORT);
    if(!field.empty()){
        port = atoi(field.c_str());
    }
    
    path = get_uri_field(url, &hp_u, UF_PATH);
    srs_info("parse url %s success", purl);
    
    query = get_uri_field(url, &hp_u, UF_QUERY);
    srs_info("parse query %s success", query.c_str());
    
    return ret;
}

const char* SrsHttpUri::get_url()
{
    return url.data();
}

const char* SrsHttpUri::get_schema()
{
    return schema.data();
}

const char* SrsHttpUri::get_host()
{
    return host.data();
}

int SrsHttpUri::get_port()
{
    return port;
}

const char* SrsHttpUri::get_path()
{
    return path.data();
}

const char* SrsHttpUri::get_query()
{
    return query.data();
}

string SrsHttpUri::get_uri_field(string uri, http_parser_url* hp_u, http_parser_url_fields field)
{
    if((hp_u->field_set & (1 << field)) == 0){
        return "";
    }
    
    srs_verbose("uri field matched, off=%d, len=%d, value=%.*s", 
        hp_u->field_data[field].off, 
        hp_u->field_data[field].len, 
        hp_u->field_data[field].len, 
        uri.c_str() + hp_u->field_data[field].off);
    
    int offset = hp_u->field_data[field].off;
    int len = hp_u->field_data[field].len;
    
    return uri.substr(offset, len);
}

#endif

