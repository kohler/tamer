#ifndef TAMER_WEBSOCKET_HH
#define TAMER_WEBSOCKET_HH 1
#include "http.hh"
namespace tamer {

class websocket_handshake {
  public:
    static bool request(http_message& req, std::string& key);
    static bool is_request(const http_message& req);
    static bool response(http_message& resp, const http_message& req);
    static bool is_response(const http_message& resp, const std::string& key);
};

enum websocket_opcode {
    WEBSOCKET_CONTINUATION = 0,
    WEBSOCKET_TEXT = 1,
    WEBSOCKET_BINARY = 2,
    WEBSOCKET_CLOSE = 8,
    WEBSOCKET_PING = 9,
    WEBSOCKET_PONG = 10
};

class websocket_message {
  public:
    inline websocket_message();

    inline bool ok() const;
    inline bool operator!() const;
    inline enum http_errno error() const;

    inline enum websocket_opcode opcode() const;
    inline bool incomplete() const;
    inline const std::string& body() const;
    inline std::string& body();

    inline websocket_message& clear();

    inline websocket_message& error(enum http_errno e);
    inline websocket_message& incomplete(bool c);
    inline websocket_message& opcode(enum websocket_opcode o);
    inline websocket_message& body(std::string body);

  private:
    unsigned error_ : 8;
    unsigned incomplete_ : 4;
    unsigned opcode_ : 4;
    std::string body_;
};

class websocket_parser : public tamed_class {
  public:
    inline websocket_parser(enum http_parser_type type);

    inline bool ok() const;
    inline bool operator!() const;
    inline bool closed() const;

    void receive_any(fd f, websocket_message& ctrl, websocket_message& data, event<int> done);
    void receive(fd f, event<websocket_message> done);
    void send(fd f, websocket_message m, event<> done);

  private:
    enum http_parser_type type_;
    bool closed_;
    uint16_t close_reason_;

    class closure__receive_any__2fdR17websocket_messageR17websocket_messageQi_;
    void receive_any(closure__receive_any__2fdR17websocket_messageR17websocket_messageQi_&);
    class closure__receive__2fdQ17websocket_message_;
    void receive(closure__receive__2fdQ17websocket_message_&);
    class closure__send__2fd17websocket_messageQ_;
    void send(closure__send__2fd17websocket_messageQ_&);
};

inline websocket_message::websocket_message()
    : error_(0), incomplete_(0), opcode_(0) {
}

inline bool websocket_message::ok() const {
    return error_ == 0;
}

inline bool websocket_message::operator!() const {
    return !ok();
}

inline enum http_errno websocket_message::error() const {
    return http_errno(error_);
}

inline enum websocket_opcode websocket_message::opcode() const {
    return websocket_opcode(opcode_);
}

inline bool websocket_message::incomplete() const {
    return incomplete_;
}

inline const std::string& websocket_message::body() const {
    return body_;
}

inline std::string& websocket_message::body() {
    return body_;
}

inline websocket_message& websocket_message::clear() {
    error_ = 0;
    incomplete_ = 0;
    opcode_ = 0;
    body_.clear();
    return *this;
}

inline websocket_message& websocket_message::error(enum http_errno e) {
    error_ = e;
    return *this;
}

inline websocket_message& websocket_message::incomplete(bool c) {
    incomplete_ = c;
    return *this;
}

inline websocket_message& websocket_message::opcode(enum websocket_opcode o) {
    opcode_ = o;
    return *this;
}

inline websocket_message& websocket_message::body(std::string s) {
    body_ = TAMER_MOVE(s);
    return *this;
}

inline websocket_parser::websocket_parser(enum http_parser_type type)
    : type_(type), closed_(false), close_reason_(0) {
}

inline bool websocket_parser::ok() const {
    return !closed_;
}

inline bool websocket_parser::operator!() const {
    return closed_;
}

inline bool websocket_parser::closed() const {
    return closed_;
}

}
#endif
