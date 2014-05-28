#ifndef TAMER_HTTP_HH
#define TAMER_HTTP_HH 1
#include "fd.hh"
#include "http_parser.h"
#include <vector>
#include <string>
#include <sstream>
#include <memory>
#include <unordered_map>
#include <time.h>
namespace tamer {
class http_parser;

struct http_header {
    std::string name;
    std::string value;
    inline http_header(std::string n, std::string v)
        : name(TAMER_MOVE(n)), value(TAMER_MOVE(v)) {
    }
    inline bool is(const char* s, size_t len) const {
        return name.length() == len && memcmp(name.data(), s, len) == 0;
    }
    inline bool is(const std::string& s) const {
        return is(s.data(), s.length());
    }
    inline bool is_canonical(const char* s, size_t len) const {
        if (name.length() != len)
            return false;
        const char* names = name.data();
        const char* end = s + len;
        for (; s != end; ++s, ++names)
            if (*s != *names
                && (*s < 'A' || *s > 'Z' || (*s - 'A' + 'a') != *names))
                return false;
        return true;
    }
    inline bool is_canonical(const std::string& s) const {
        return is_canonical(s.data(), s.length());
    }
    inline bool is_content_length() const {
        return is_canonical("content-length", 14);
    }
};

class http_message {
  public:
    inline http_message();

    inline bool ok() const;
    inline bool operator!() const;
    inline enum http_errno error() const;

    inline unsigned http_major() const;
    inline unsigned http_minor() const;

    inline unsigned status_code() const;
    inline const std::string& status_message() const;
    inline enum http_method method() const;
    inline const std::string& url() const;
    bool has_canonical_header(const std::string& name) const;
    inline bool has_header(const std::string& name) const;
    inline const std::string& body() const;

    std::string host() const;
    inline std::string url_schema() const;
    inline std::string url_host() const;
    std::string url_host_port() const;
    inline std::string url_path() const;
    inline bool has_query() const;
    inline std::string query() const;
    bool has_query(const std::string& name) const;
    std::string query(const std::string& name) const;

    typedef std::vector<http_header>::const_iterator header_iterator;
    inline header_iterator header_begin() const;
    inline header_iterator header_end() const;
    inline header_iterator query_begin() const;
    inline header_iterator query_end() const;

    void clear();
    void add_header(std::string key, std::string value);

    inline http_message& http_major(unsigned v);
    inline http_message& http_minor(unsigned v);
    inline http_message& error(enum http_errno e);
    inline http_message& status_code(unsigned code);
    inline http_message& status_code(unsigned code, std::string message);
    inline http_message& method(enum http_method method);
    inline http_message& url(std::string url);
    inline http_message& header(std::string key, std::string value);
    inline http_message& header(std::string key, size_t value);
    inline http_message& date_header(std::string key, time_t value);
    inline http_message& body(std::string body);

    static std::string canonicalize(std::string x);
    static const char* default_status_message(unsigned code);

  private:
    enum {
        info_url = 1, info_query = 2
    };

    struct info_type {
        unsigned flags;
        struct http_parser_url urlp;
        std::vector<http_header> raw_query;
        inline info_type()
            : flags(0) {
        }
    };

    unsigned short major_;
    unsigned short minor_;
    unsigned status_code_ : 16;
    unsigned method_ : 8;
    unsigned error_ : 7;
    unsigned upgrade_ : 1;

    std::string url_;
    std::string status_message_;
    std::vector<http_header> raw_headers_;
    std::string body_;

    mutable std::shared_ptr<info_type> info_;

    inline void kill_info(unsigned f) const;
    inline info_type& info(unsigned f) const;
    void make_info(unsigned f) const;
    inline bool has_url_field(int field) const;
    inline std::string url_field(int field) const;
    friend class http_parser;
};

class http_parser : public tamed_class {
  public:
    http_parser(enum http_parser_type type);

    void clear();
    inline bool ok() const;
    inline enum http_errno error() const;
    inline bool should_keep_alive() const;

    void receive(fd f, event<http_message> done);
    inline void send(fd f, const http_message& m, event<> done);
    static void send_request(fd f, const http_message& m, event<> done);
    static void send_response(fd f, const http_message& m, event<> done);
    static void send_response_headers(fd f, const http_message& m, event<> done);
    static void send_response_chunk(fd f, std::string s, event<> done);
    static void send_response_end(fd f, event<> done);

  private:
    ::http_parser hp_;

    struct message_data {
        http_message hm;
        std::string key;
        std::ostringstream sbuf;
        bool done;
    };

    static const http_parser_settings settings;
    static http_parser* get_parser(::http_parser* hp);
    static message_data* get_message_data(::http_parser* hp);
    static int on_message_begin(::http_parser* hp);
    static int on_url(::http_parser* hp, const char* s, size_t len);
    static int on_status(::http_parser* hp, const char* s, size_t len);
    static int on_header_field(::http_parser* hp, const char* s, size_t len);
    static int on_header_value(::http_parser* hp, const char* s, size_t len);
    static int on_headers_complete(::http_parser* hp);
    static int on_body(::http_parser* hp, const char* s, size_t len);
    static int on_message_complete(::http_parser* hp);
    inline void copy_parser_status(message_data& md);
    static void unparse_request_headers(std::ostringstream& buf,
                                        const http_message& m);
    static void unparse_response_headers(std::ostringstream& buf,
                                         const http_message& m,
                                         bool include_content_length);

    class closure__receive__2fdQ12http_message_; void receive(closure__receive__2fdQ12http_message_&);
    class closure__send_request__2fdRK12http_messageQ_; static void send_request(closure__send_request__2fdRK12http_messageQ_&);
    class closure__send_response__2fdRK12http_messageQ_; static void send_response(closure__send_response__2fdRK12http_messageQ_&);
    class closure__send_response_chunk__2fdSsQ_; static void send_response_chunk(closure__send_response_chunk__2fdSsQ_&);
};

inline http_message::http_message()
    : major_(1), minor_(1), status_code_(200), method_(HTTP_GET),
      error_(HPE_OK), upgrade_(0) {
}

inline void http_message::kill_info(unsigned f) const {
    if (info_)
        info_->flags &= ~f;
}

inline unsigned http_message::http_major() const {
    return major_;
}

inline unsigned http_message::http_minor() const {
    return minor_;
}

inline bool http_message::ok() const {
    return error_ == HPE_OK;
}

inline bool http_message::operator!() const {
    return !ok();
}

inline enum http_errno http_message::error() const {
    return (enum http_errno) error_;
}

inline unsigned http_message::status_code() const {
    return status_code_;
}

inline const std::string& http_message::status_message() const {
    return status_message_;
}

inline enum http_method http_message::method() const {
    return (enum http_method) method_;
}

inline const std::string& http_message::url() const {
    return url_;
}

inline bool http_message::has_header(const std::string& key) const {
    return has_canonical_header(canonicalize(key));
}

inline const std::string& http_message::body() const {
    return body_;
}

inline bool http_message::has_url_field(int field) const {
    return info(info_url).urlp.field_set & (1 << field);
}

inline std::string http_message::url_field(int field) const {
    const info_type& i = info(info_url);
    if (i.urlp.field_set & (1 << field))
        return url_.substr(i.urlp.field_data[field].off,
                           i.urlp.field_data[field].len);
    else
        return std::string();
}

inline bool http_message::has_query() const {
    return has_url_field(UF_QUERY);
}

inline std::string http_message::query() const {
    return url_field(UF_QUERY);
}

inline std::string http_message::url_schema() const {
    return url_field(UF_SCHEMA);
}

inline std::string http_message::url_host() const {
    return url_field(UF_HOST);
}

inline std::string http_message::url_path() const {
    return url_field(UF_PATH);
}

inline http_message& http_message::http_major(unsigned v) {
    major_ = v;
    return *this;
}

inline http_message& http_message::http_minor(unsigned v) {
    minor_ = v;
    return *this;
}

inline http_message& http_message::error(enum http_errno e) {
    error_ = e;
    return *this;
}

inline http_message& http_message::status_code(unsigned code) {
    status_code_ = code;
    status_message_ = std::string();
    return *this;
}

inline http_message& http_message::status_code(unsigned code, std::string message) {
    status_code_ = code;
    status_message_ = TAMER_MOVE(message);
    return *this;
}

inline http_message& http_message::method(enum http_method method) {
    method_ = (unsigned) method;
    return *this;
}

inline http_message& http_message::url(std::string url) {
    url_ = TAMER_MOVE(url);
    kill_info(info_url | info_query);
    return *this;
}

inline http_message& http_message::header(std::string key, std::string value) {
    add_header(TAMER_MOVE(key), TAMER_MOVE(value));
    return *this;
}

inline http_message& http_message::header(std::string key, size_t value) {
    std::ostringstream buf;
    buf << value;
    add_header(TAMER_MOVE(key), buf.str());
    return *this;
}

inline http_message& http_message::date_header(std::string key, time_t value) {
    char buf[128];
    // XXX current locale
    strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", gmtime(&value));
    add_header(TAMER_MOVE(key), std::string(buf));
    return *this;
}

inline http_message& http_message::body(std::string body) {
    body_ = TAMER_MOVE(body);
    return *this;
}

inline http_message::info_type& http_message::info(unsigned f) const {
    if (!info_ || !info_.unique() || (info_->flags & f) != f)
        make_info(f);
    return *info_.get();
}

inline http_message::header_iterator http_message::header_begin() const {
    return raw_headers_.begin();
}

inline http_message::header_iterator http_message::header_end() const {
    return raw_headers_.end();
}

inline http_message::header_iterator http_message::query_begin() const {
    return info(info_query).raw_query.begin();
}

inline http_message::header_iterator http_message::query_end() const {
    return info(info_query).raw_query.end();
}

inline bool http_parser::ok() const {
    return hp_.http_errno == (unsigned) HPE_OK;
}

inline enum http_errno http_parser::error() const {
    return (enum http_errno) hp_.http_errno;
}

inline bool http_parser::should_keep_alive() const {
    return http_should_keep_alive(&hp_);
}

inline void http_parser::send(fd f, const http_message& m, event<> done) {
    if (hp_.type == (int) HTTP_RESPONSE)
        send_request(f, m, done);
    else if (hp_.type == (int) HTTP_REQUEST)
        send_response(f, m, done);
    else
        assert(0);
}

} // namespace tamer
#endif
