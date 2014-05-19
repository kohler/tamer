// -*- mode: c++ -*-
/* Copyright (c) 2014, Eddie Kohler
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, subject to the conditions
 * listed in the Tamer LICENSE file. These conditions include: you must
 * preserve this copyright notice, and you cannot mention the copyright
 * holders in advertising related to the Software without their permission.
 * The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
 * notice is a summary of the Tamer LICENSE file; the license in that file is
 * legally binding.
 */
#include "config.h"
#include <tamer/http.hh>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <sys/resource.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <tamer/tamer.hh>
#include <algorithm>

namespace {
struct status_code_map {
    unsigned code;
    const char* message;
} default_status_codes[] = {
    {100, "Continue"},
    {101, "Switching Protocols"},
    {102, "Processing"},                 // RFC 2518, obsoleted by RFC 4918
    {200, "OK"},
    {201, "Created"},
    {202, "Accepted"},
    {203, "Non-Authoritative Information"},
    {204, "No Content"},
    {205, "Reset Content"},
    {206, "Partial Content"},
    {207, "Multi-Status"},               // RFC 4918
    {300, "Multiple Choices"},
    {301, "Moved Permanently"},
    {302, "Moved Temporarily"},
    {303, "See Other"},
    {304, "Not Modified"},
    {305, "Use Proxy"},
    {307, "Temporary Redirect"},
    {400, "Bad Request"},
    {401, "Unauthorized"},
    {402, "Payment Required"},
    {403, "Forbidden"},
    {404, "Not Found"},
    {405, "Method Not Allowed"},
    {406, "Not Acceptable"},
    {407, "Proxy Authentication Required"},
    {408, "Request Time-out"},
    {409, "Conflict"},
    {410, "Gone"},
    {411, "Length Required"},
    {412, "Precondition Failed"},
    {413, "Request Entity Too Large"},
    {414, "Request-URI Too Large"},
    {415, "Unsupported Media Type"},
    {416, "Requested Range Not Satisfiable"},
    {417, "Expectation Failed"},
    {418, "I\'m a teapot"},              // RFC 2324
    {422, "Unprocessable Entity"},       // RFC 4918
    {423, "Locked"},                     // RFC 4918
    {424, "Failed Dependency"},          // RFC 4918
    {425, "Unordered Collection"},       // RFC 4918
    {426, "Upgrade Required"},           // RFC 2817
    {428, "Precondition Required"},      // RFC 6585
    {429, "Too Many Requests"},          // RFC 6585
    {431, "Request Header Fields Too Large"},// RFC 6585
    {500, "Internal Server Error"},
    {501, "Not Implemented"},
    {502, "Bad Gateway"},
    {503, "Service Unavailable"},
    {504, "Gateway Time-out"},
    {505, "HTTP Version Not Supported"},
    {506, "Variant Also Negotiates"},    // RFC 2295
    {507, "Insufficient Storage"},       // RFC 4918
    {509, "Bandwidth Limit Exceeded"},
    {510, "Not Extended"},               // RFC 2774
    {511, "Network Authentication Required"} // RFC 6585
};
struct status_code_map_comparator {
    bool operator()(const status_code_map& a, unsigned b) {
        return a.code < b;
    }
};
}

namespace tamer {

const http_parser_settings http_parser::settings = {
    /* on_message_begin */    &http_parser::on_message_begin,
    /* on_url */              &http_parser::on_url,
    /* on_status */           &http_parser::on_status,
    /* on_header_field */     &http_parser::on_header_field,
    /* on_header_value */     &http_parser::on_header_value,
    /* on_headers_complete */ &http_parser::on_headers_complete,
    /* on_body */             &http_parser::on_body,
    /* on_message_complete */ &http_parser::on_message_complete
};


std::string http_message::canonicalize(std::string x) {
    for (size_t i = 0; i != x.length(); ++i) {
        unsigned char c = (unsigned char) x[i];
        if (c >= 'A' && c <= 'Z')
            x[i] = c - 'A' + 'a';
    }
    return x;
}

const char* http_message::default_status_message(unsigned code) {
    size_t ncodes = sizeof(default_status_codes) / sizeof(status_code_map);
    status_code_map* m = std::lower_bound(default_status_codes,
                                          default_status_codes + ncodes,
                                          code, status_code_map_comparator());
    if (m != default_status_codes + ncodes && m->code == code)
        return m->message;
    else
        return "unknown";
}

bool http_message::has_canonical_header(const std::string& key) const {
    for (std::vector<http_header>::const_iterator it = raw_headers_.begin();
         it != raw_headers_.end(); ++it)
        if (header_equals_canonical(it->name, key))
            return true;
    return false;
}

void http_message::clear() {
    major_ = minor_ = 1;
    status_code_ = 200;
    method_ = HTTP_GET;
    error_ = HPE_OK;
    upgrade_ = 0;
    url_ = status_message_ = body_ = std::string();
    raw_headers_.clear();
}

static inline bool string_equals(const std::string& s, const char* c_str, size_t len) {
    return s.length() == len && memcmp(s.data(), c_str, len) == 0;
}

#define STRING_EQUALS(s, c_str) string_equals((s), (c_str), sizeof(c_str) - 1)

void http_message::add_header(std::string key, std::string value) {
    raw_headers_.push_back(http_header(TAMER_MOVE(key), TAMER_MOVE(value)));
#if 0
    std::string lckey(raw_headers_[raw_headers_.size() - 2]);
    for (std::string::iterator it = lckey.begin();
         it != lckey.end(); ++it)
        *it = tolower((unsigned char) *it);

    if (STRING_EQUALS(lckey, "set-cookie")) {
        cookies_.push_back(raw_headers_.back());
        return;
    }

    header_map_type::iterator hit = headers_.lower_bound(lckey);
    if (hit == headers_.end() || hit->first != lckey)
        headers_.insert(hit, std::make_pair(TAMER_MOVE(lckey),
                                            raw_headers_.back()));
    else if (STRING_EQUALS(lckey, "content-type")
             || STRING_EQUALS(lckey, "content-length")
             || STRING_EQUALS(lckey, "user-agent")
             || STRING_EQUALS(lckey, "referer")
             || STRING_EQUALS(lckey, "host")
             || STRING_EQUALS(lckey, "authorization")
             || STRING_EQUALS(lckey, "proxy-authorization")
             || STRING_EQUALS(lckey, "if-modified-since")
             || STRING_EQUALS(lckey, "if-unmodified-since")
             || STRING_EQUALS(lckey, "from")
             || STRING_EQUALS(lckey, "location")
             || STRING_EQUALS(lckey, "max-forwards"))
        /* drop duplicate */;
    else {
        hit->second += ", ";
        hit->second += raw_headers_.back();
    }
#endif
}


http_parser::http_parser(enum http_parser_type hp_type) {
    http_parser_init(&hp_, hp_type);
}

inline void http_parser::copy_parser_status(message_data& md) {
    md.hm.major_ = hp_.http_major;
    md.hm.minor_ = hp_.http_minor;
    md.hm.status_code_ = hp_.status_code;
    md.hm.method_ = hp_.method;
    md.hm.error_ = hp_.http_errno;
    md.hm.upgrade_ = hp_.upgrade;
}

inline http_parser* http_parser::get_parser(::http_parser* hp) {
    return reinterpret_cast<http_parser*>(hp);
}

inline http_parser::message_data* http_parser::get_message_data(::http_parser* hp) {
    return static_cast<message_data*>(hp->data);
}

int http_parser::on_message_begin(::http_parser* hp) {
    message_data* md = get_message_data(hp);
    md->hm.clear();
    md->key.clear();
    md->sbuf.clear();
    return 0;
}

int http_parser::on_url(::http_parser* hp, const char* s, size_t len) {
    message_data* md = get_message_data(hp);
    md->hm.url_.append(s, len);
    return 0;
}

int http_parser::on_status(::http_parser* hp, const char* s, size_t len) {
    message_data* md = get_message_data(hp);
    if (md->hm.major_ == 0)
        get_parser(hp)->copy_parser_status(*md);
    md->hm.status_message_.append(s, len);
    return 0;
}

int http_parser::on_header_field(::http_parser* hp, const char* s, size_t len) {
    message_data* md = get_message_data(hp);
    if (!md->key.empty()) {
        md->hm.add_header(TAMER_MOVE(md->key), md->sbuf.str());
        md->key = std::string();
        md->sbuf.str(std::string());
    }
    md->sbuf.write(s, len);
    return 0;
}

int http_parser::on_header_value(::http_parser* hp, const char* s, size_t len) {
    message_data* md = get_message_data(hp);
    if (md->key.empty()) {
        md->key = md->sbuf.str();
        md->sbuf.str(std::string());
    }
    md->sbuf.write(s, len);
    return 0;
}

int http_parser::on_headers_complete(::http_parser* hp) {
    message_data* md = get_message_data(hp);
    if (!md->key.empty()) {
        md->hm.add_header(TAMER_MOVE(md->key), md->sbuf.str());
        md->key = md->sbuf.str();
        md->sbuf.str(std::string());
    }
    get_parser(hp)->copy_parser_status(*md);
    return 0;
}

int http_parser::on_body(::http_parser* hp, const char* s, size_t len) {
    message_data* md = get_message_data(hp);
    md->sbuf.write(s, len);
    return 0;
}

int http_parser::on_message_complete(::http_parser* hp) {
    message_data* md = get_message_data(hp);
    md->hm.body_ = md->sbuf.str();
    md->done = true;
    return 0;
}

tamed void http_parser::receive(fd f, event<http_message> done) {
    tamed {
        message_data md;
    }
    md.done = false;

    while (f && done) {
        {
            char mbuf[32768];
            ssize_t nread = f.direct_read(mbuf, sizeof(mbuf));

            if (nread != (ssize_t) -1) {
                hp_.data = &md;
                size_t nconsumed =
                    http_parser_execute(&hp_, &settings, mbuf, nread);
                if (hp_.upgrade || nconsumed != (size_t) nread || md.done) {
                    copy_parser_status(md);
                    break;
                }
            } else if (errno == EAGAIN || errno == EWOULDBLOCK)
                /* fall through to blocking */;
            else if (errno != EINTR) {
                f.close(errno);
                break;
            } else
                continue;
        }

        twait { tamer::at_fd_read(f.value(), make_event()); }
    }

    if (!md.done && !md.hm.error_)
        hp_.http_errno = md.hm.error_ = HPE_UNKNOWN;
    done(TAMER_MOVE(md.hm));
}

void http_parser::unparse_request_headers(std::ostringstream& buf,
                                          const http_message& m) {
    buf << http_method_str(m.method()) << " " << m.url()
        << " HTTP/" << m.http_major() << "." << m.http_minor() << "\r\n";
    bool need_content_length = !m.body_.empty();
    for (std::vector<http_header>::const_iterator it = m.raw_headers_.begin();
         it != m.raw_headers_.end(); ++it) {
        buf << it->name << ": " << it->value << "\r\n";
        need_content_length = need_content_length && !it->is_content_length();
    }
    if (need_content_length)
        buf << "Content-Length: " << m.body_.length() << "\r\n";
    buf << "\r\n";
}

tamed static void http_parser::send_request(fd f, const http_message& m,
                                            event<> done) {
    tamed { std::ostringstream buf; std::string body; }
    unparse_request_headers(buf, m);
    body = m.body();
    if (body.length() + buf.str().length() < 16384) {
        buf << TAMER_MOVE(body);
        f.write(buf.str(), done);
    } else {
        twait { f.write(buf.str(), make_event()); }
        f.write(TAMER_MOVE(body), done);
    }
}

void http_parser::unparse_response_headers(std::ostringstream& buf,
                                           const http_message& m,
                                           bool include_content_length) {
    buf << "HTTP/" << m.http_major() << "." << m.http_minor()
        << " " << m.status_code() << " ";
    if (m.status_message().empty())
        buf << m.default_status_message(m.status_code());
    else
        buf << m.status_message();
    buf << "\r\n";
    bool need_content_length = !m.body_.empty() && include_content_length;
    for (std::vector<http_header>::const_iterator it = m.raw_headers_.begin();
         it != m.raw_headers_.end(); ++it) {
        buf << it->name << ": " << it->value << "\r\n";
        need_content_length = need_content_length && !it->is_content_length();
    }
    if (need_content_length)
        buf << "Content-Length: " << m.body_.length() << "\r\n";
    buf << "\r\n";
}

tamed static void http_parser::send_response(fd f, const http_message& m,
                                             event<> done) {
    tamed { std::ostringstream buf; std::string body; }
    unparse_response_headers(buf, m, true);
    body = m.body();
    if (body.length() + buf.str().length() <= 16384) {
        buf << TAMER_MOVE(body);
        f.write(buf.str(), done);
    } else {
        twait { f.write(buf.str(), make_event()); }
        f.write(TAMER_MOVE(body), done);
    }
}

void http_parser::send_response_headers(fd f, const http_message& m,
                                        event<> done) {
    std::ostringstream buf;
    unparse_response_headers(buf, m, false);
    f.write(buf.str(), done);
}

tamed static void http_parser::send_response_chunk(fd f, std::string s,
                                                   event<> done) {
    tamed { std::ostringstream buf; }
    buf << s.length() << "\r\n";
    if (s.length() <= 16384) {
        buf << s << "\r\n";
        f.write(buf.str(), done);
    } else {
        twait { f.write(buf.str(), make_event()); }
        twait { f.write(TAMER_MOVE(s), make_event()); }
        f.write("\r\n", 2, done);
    }
}

void http_parser::send_response_end(fd f, event<> done) {
    f.write("0\r\n\r\n", 5, done);
}

} // namespace tamer
