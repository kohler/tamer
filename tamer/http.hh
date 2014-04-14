#ifndef TAMER_HTTP_HH
#define TAMER_HTTP_HH 1
#include "fd.hh"
#include "http_parser.h"
#include <vector>
#include <string>
#include <sstream>
#include <map>
#include <time.h>
namespace tamer {
class http_parser;

class http_message {
  public:
    inline http_message();

    inline bool ok() const;
    inline bool operator!() const;

    inline unsigned http_major() const;
    inline unsigned http_minor() const;

    inline unsigned status_code() const;
    inline const std::string& status_message() const;
    inline enum http_method method() const;
    inline const std::string& url() const;
    bool has_canonical_header(const std::string& key) const;
    inline bool has_header(const std::string& key) const;
    inline const std::string& body() const;

    void clear();
    void add_header(std::string key, std::string value);

    inline http_message& status_code(unsigned code);
    inline http_message& status_code(unsigned code, std::string message);
    inline http_message& method(enum http_method method);
    inline http_message& url(std::string url);
    inline http_message& header(std::string key, std::string value);
    inline http_message& header(std::string key, size_t value);
    inline http_message& date_header(std::string key, time_t value);
    inline http_message& body(std::string body);

    static std::string canonical_header(std::string x);
    static bool header_equals_canonical(const std::string& key,
                                        const std::string& canonical);
    static const char* default_status_message(unsigned code);

  private:
    unsigned short major_;
    unsigned short minor_;
    unsigned status_code_ : 16;
    unsigned method_ : 8;
    unsigned error_ : 7;
    unsigned upgrade_ : 1;

    std::string url_;
    std::string status_message_;
    std::vector<std::string> raw_headers_;
    std::string body_;

    friend class http_parser;
};

class http_parser {
  public:
    http_parser(enum http_parser_type type);

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
    return has_canonical_header(canonical_header(key));
}

inline const std::string& http_message::body() const {
    return body_;
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
    return *this;
}

inline http_message& http_message::header(std::string key, std::string value) {
    add_header(TAMER_MOVE(key), TAMER_MOVE(value));
    return *this;
}

inline http_message& http_message::header(std::string key, size_t value) {
    char buf[100];
    snprintf(buf, sizeof(buf), "%zu", value);
    add_header(TAMER_MOVE(key), std::string(buf));
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

inline bool http_message::header_equals_canonical(const std::string& a,
                                                  const std::string& b) {
    if (a.length() != b.length())
        return false;
    for (size_t i = 0; i != a.length(); ++i)
        if (tolower((unsigned char) a[i]) == b[i])
            return false;
    return true;
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
