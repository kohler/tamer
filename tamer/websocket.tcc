#include "websocket.hh"
#include <mbedtls/base64.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/sha1.h>
#include <unistd.h>
#include <iostream>
namespace tamer {

namespace {
int read_random_bytes(void* buf, size_t n) {
    static mbedtls_entropy_context entropy;
    static mbedtls_ctr_drbg_context ctr_drbg;
    static bool initialized = false;
    if (!initialized) {
        mbedtls_entropy_init(&entropy);
        pid_t ps[2];
        ps[0] = getpid();
        ps[1] = getppid();
        int r = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                                      (unsigned char*) ps, sizeof(ps));
        assert(r != 0); (void) r;
        initialized = true;
    }
    return mbedtls_ctr_drbg_random(&ctr_drbg, reinterpret_cast<unsigned char*>(buf), n);
}
}

static inline unsigned expected_length(unsigned char header1) {
    return 2
        + ((header1 & 127) == 126 ? 2 : 0)
        + ((header1 & 127) == 127 ? 8 : 0)
        + (header1 >= 128 ? 4 : 0);
}

union websocket_mask_union {
    uint32_t u;
    unsigned char c[4];
};

namespace {
void do_mask(unsigned char* x, size_t n, websocket_mask_union mask) {
    size_t i;
    uint32_t z;
    for (i = 0; i + 4 <= n; i += 4) {
        memcpy(&z, &x[i], 4);
        z ^= mask.u;
        memcpy(&x[i], &z, 4);
    }
    for (; i < n; ++i)
        x[i] ^= mask.c[i & 3];
}
void do_mask(char* x, size_t n, websocket_mask_union mask) {
    return do_mask(reinterpret_cast<unsigned char*>(x), n, mask);
}

bool validate_utf8(const char* first, const char* cur, const char* last,
                   bool complete) {
    while (cur > first) {
        --cur;
        if (((unsigned char) *cur & 0xC0) != 0x80)
            break;
    }
    int state = 0;
    for (; cur < last; ++cur) {
        int ch = (unsigned char) *cur;
        if (state == 0 && ch <= 127)
            continue;
        if ((state == 0) != ((ch & 0xC0) != 0x80)
            || ch == 0xC0
            || ch == 0xC1
            || ch >= 0xF5
            || (state == 2 && ch < 0xA0 && (unsigned char) cur[-1] == 0xE0)
            || (state == 2 && ch >= 0xA0 && (unsigned char) cur[-1] == 0xED)
            || (state == 3 && ch < 0x90 && (unsigned char) cur[-1] == 0xF0)
            || (state == 3 && ch >= 0x90 && (unsigned char) cur[-1] == 0xF4))
            return false;
        if (ch >= 0xF0)
            state = 3;
        else if (ch >= 0xE0)
            state = 2;
        else if (ch >= 0xC0)
            state = 1;
        else
            --state;
    }
    return state == 0 || !complete;
}
}

tamed void websocket_parser::receive_any(fd f, websocket_message& ctrl, websocket_message& data, event<int> done) {
    tvars {
        unsigned char header[32];
        size_t nread, offset;
        int r;
        websocket_mask_union mask;
        websocket_message* m;
    }

    twait { f.read(header, 2, nread, make_event(r)); }
    if (r >= 0 && nread == 2 && header[1] >= 126) {
        twait { f.read(&header[2], expected_length(header[1]) - 2, nread, make_event(r)); }
        nread += 2;
    }

    if (r < 0                                     // I/O error
        || (nread && nread != expected_length(header[1]))) { // bad length
        r = -HPE_INVALID_EOF_STATE;
        goto protocol_error;
    } else if (!nread) {
        done(-HPE_CLOSED_CONNECTION);
        if (!closed_)
            close_code_ = 1006;
        closed_ = 3;
        return;
    } else if ((type_ == HTTP_RESPONSE) != !(header[1] & 128) // mask iff C->S
               || (header[0] & 0x70)              // reserved bits nonzero
               || (header[0] & 0x07) >= 3) {      // unknown opcode
        r = -HPE_INVALID_HEADER_TOKEN;
        goto protocol_error;
    } else if (header[0] & 0x08
               ? (header[0] & 0x80) == 0x00  // fragmented control
                 || (header[1] & 0x7F) > 125 // control too long
               : (data.opcode()
                  ? header[0] & 0x07         // new frame, incomplete fragment
                  : !(header[0] & 0x07))) {  // continuation, no fragment
        r = -HPE_INVALID_FRAGMENT;
        goto protocol_error;
    }

    if (header[0] & 0x08)
        m = &ctrl.clear();
    else
        m = &data;

    if (header[0] & 0x0F)
        m->opcode(websocket_opcode(header[0] & 0x0F));

    m->incomplete(!(header[0] & 0x80));

    {
        uint64_t sz = header[1] & 127;
        if (sz == 126)
            sz = (header[2] << 8) | header[3];
        else if (sz == 127)
            sz = (uint64_t(header[2]) << 56) | (uint64_t(header[3]) << 48)
                | (uint64_t(header[4]) << 40) | (uint64_t(header[5]) << 32)
                | (uint32_t(header[6]) << 24) | (header[7] << 16)
                | (header[8] << 8) | header[9];
        offset = 0;
        if (!(header[0] & 0x08))
            offset = m->body().length();
        sz += offset;
        size_t l = sizeof(uintptr_t);
        uint64_t cap = sz + (sz % l ? l - sz % l : 0);
        m->body().reserve(cap);
        m->body().resize(sz);
        if (m->body().length() != sz || m->body().capacity() < cap) {
            m->body().resize(0);
            goto too_big;
        }
    }

    // read data
    twait { f.read(&m->body().front() + offset, m->body().length() - offset,
                   nread, make_event(r)); }

    if (r < 0 || offset + nread != m->body().length()) {
        m->body().resize(offset + nread);
        r = -HPE_INVALID_EOF_STATE;
        goto protocol_error;
    }

    if ((header[1] & 0x80) && m->body().length() > offset) {
        memcpy(&mask, &header[expected_length(header[1]) - 4], 4);
        do_mask(&m->body().front() + offset, m->body().length() - offset, mask);
    }

    // validate UTF-8
    if (m->opcode() == WEBSOCKET_TEXT
        && !validate_utf8(m->body().data(), m->body().data() + offset, m->body().data() + m->body().length(), header[0] & 0x80)) {
        done(-HPE_STRICT);
        twait { close(f, 1007, make_event()); }
        return;
    }

    done(header[0] & 0x80 ? m->opcode() : 0);
    return;

 protocol_error:
    done(r);
    twait { close(f, 1002, make_event()); }
    return;

 too_big:
    done(-HPE_INVALID_CONTENT_LENGTH);
    twait { close(f, 1009, make_event()); }
}

tamed void websocket_parser::receive(fd f, event<websocket_message> done) {
    tvars {
        websocket_message ctrl, data;
        int r;
    }
    while (true) {
        twait { receive_any(f, ctrl, data, make_event(r)); }
        if (r < 0) {
            done(TAMER_MOVE(data.error(http_errno(-r))));
            return;
        } else if (r > 0 && r < 0x08) {
            done(TAMER_MOVE(data));
            return;
        } else if (r == WEBSOCKET_CLOSE) {
            if (ctrl.body().length() < 2)
                close_code_ = 1005;
            else {
                close_code_ = ((unsigned char) ctrl.body()[0] << 8) | (unsigned char) ctrl.body()[1];
                // invalid reasons => protocol error
                if (close_code_ < 1000
                    || (close_code_ >= 1004 && close_code_ <= 1006)
                    || (close_code_ >= 1014 && close_code_ <= 2999))
                    close_code_ = 1002;
                if (ctrl.body().length() > 2) {
                    if (validate_utf8(ctrl.body().data() + 2, ctrl.body().data() + 2, ctrl.body().data() + ctrl.body().length(), true))
                        close_reason_ = ctrl.body().substr(2);
                    else
                        close_code_ = 1002;
                }
            }
            closed_ |= 1;
            if (!(closed_ & 2))
                twait { close(f, close_code_, make_event()); }
            done(TAMER_MOVE(data.error(HPE_CLOSED_CONNECTION)));
            return;
        } else if (r == WEBSOCKET_PING) {
            twait { send(f, ctrl.opcode(WEBSOCKET_PONG), make_event()); }
        }
    }
}

tamed void websocket_parser::send(fd f, websocket_message m, event<> done) {
    tvars {
        unsigned char header[16];
        int nheader;
        struct iovec iov[2];
        size_t nread;
        int r;
    }
    assert(!(closed_ & 2));
    if (m.opcode() == WEBSOCKET_CLOSE)
        closed_ |= 2;

    {
        size_t l = m.body().length();
        header[0] = (m.incomplete() ? 0 : 0x80) | int(m.opcode());
        header[1] = (type_ == HTTP_RESPONSE ? 0x80 : 0)
            | (l < 126 ? l : (l <= 65535 ? 126 : 127));
        if (l < 126)
            nheader = 2;
        else if (l <= 65535) {
            header[2] = l >> 8;
            header[3] = l % 256;
            nheader = 4;
        } else {
            header[2] = uint64_t(l) >> 56;
            header[3] = (uint64_t(l) >> 48) % 256;
            header[4] = (uint64_t(l) >> 40) % 256;
            header[5] = (uint64_t(l) >> 32) % 256;
            header[6] = (l >> 24) % 256;
            header[7] = (l >> 16) % 256;
            header[8] = (l >> 8) % 256;
            header[9] = l % 256;
            nheader = 10;
        }
    }

    // mask
    if (header[1] & 0x80) {
        websocket_mask_union mask;
        if (read_random_bytes(&mask, 4) != 0) {
            done();
            return;
        }
        if (m.body().length())
            do_mask(&m.body().front(), m.body().length(), mask);
        memcpy(&header[nheader], &mask, 4);
        nheader += 4;
    }

    // send
    iov[0].iov_base = header;
    iov[0].iov_len = nheader;
    iov[1].iov_base = const_cast<char*>(m.body().data());
    iov[1].iov_len = m.body().length();

    twait { f.write(iov, 2, nullptr, rebind<int>(make_event())); }

    done();
}

void websocket_parser::close(fd f, uint16_t code, std::string reason, event<> done) {
    if (!(closed_ & 2)) {
        websocket_message m;
        m.opcode(WEBSOCKET_CLOSE);
        if (code && code != 1005) {
            char buf[256];
            buf[0] = code >> 8;
            buf[1] = code & 255;
            if (reason.length() > 254)
                m.body(std::string(buf, 2) + reason);
            else {
                memcpy(&buf[2], reason.data(), reason.length());
                m.body(std::string(buf, 2 + reason.length()));
            }
        }
        send(f, std::move(m), done);
    } else
        done();
}

bool websocket_handshake::request(http_message& req, std::string& key) {
    unsigned char rand[16], randenc[32];
    size_t olen;
    if (read_random_bytes(rand, 16) != 0
        || mbedtls_base64_encode(randenc, sizeof(randenc), &olen, rand, sizeof(rand)) != 0)
        return false;
    key = std::string(reinterpret_cast<char*>(randenc), olen);
    req.header("Connection", "Upgrade").header("Upgrade", "websocket")
        .header("Sec-WebSocket-Version", "13")
        .header("Sec-WebSocket-Key", key);
    return true;
}

bool websocket_handshake::is_request(const http_message& req) {
    if (!http_header::equals_canonical(req.canonical_header("upgrade"), "websocket", 9))
        return false;
    std::string connection = req.canonical_header("connection");
    const unsigned char* s = reinterpret_cast<const unsigned char*>(connection.data());
    const unsigned char* end = s + connection.length();
    while (s < end) {
        while (s < end && isspace(*s))
            ++s;
        const unsigned char* t = s;
        while (t < end && !isspace(*t) && *t != ',')
            ++t;
        if (t - s == 7U && http_header::equals_canonical(reinterpret_cast<const char*>(s), "upgrade", 7))
            return true;
        s = t;
        if (s < end && *s == ',')
            ++s;
    }
    return false;
}

static void websocket_key_translate(unsigned char digest[20], const std::string& key) {
    mbedtls_sha1_context sha1ctx;
    mbedtls_sha1_init(&sha1ctx);
    mbedtls_sha1_starts(&sha1ctx);
    mbedtls_sha1_update(&sha1ctx, reinterpret_cast<const unsigned char*>(key.data()), key.length());
    mbedtls_sha1_update(&sha1ctx, reinterpret_cast<const unsigned char*>("258EAFA5-E914-47DA-95CA-C5AB0DC85B11"), 36);
    mbedtls_sha1_finish(&sha1ctx, digest);
    mbedtls_sha1_free(&sha1ctx);
}

bool websocket_handshake::response(http_message& resp, const http_message& req) {
    if (req.canonical_header("sec-websocket-version") != "13") {
        resp.status_code(426).header("Sec-WebSocket-Version", "13");
        return false;
    }
    unsigned char sha1[20], sha1enc[40];
    websocket_key_translate(sha1, req.canonical_header("sec-websocket-key"));
    size_t olen;
    if (mbedtls_base64_encode(sha1enc, sizeof(sha1enc), &olen, sha1, sizeof(sha1)) != 0)
        return false;
    resp.status_code(101).header("Upgrade", "websocket")
        .header("Connection", "Upgrade")
        .date_header("Date", time(0))
        .header("Sec-WebSocket-Accept", std::string(reinterpret_cast<char*>(sha1enc), olen));
    return true;
}

bool websocket_handshake::is_response(const http_message& resp, const std::string& key) {
    if (resp.status_code() != 101 || !is_request(resp))
        return false;
    unsigned char want_sha1[20], have_sha1[20];
    size_t olen;
    std::string accept = resp.canonical_header("sec-websocket-accept");
    if (mbedtls_base64_decode(have_sha1, sizeof(have_sha1), &olen,
                              reinterpret_cast<const unsigned char*>(accept.data()),
                              accept.length()) != 0
        || olen != 20)
        return false;
    websocket_key_translate(want_sha1, key);
    return memcmp(want_sha1, have_sha1, 20) == 0;
}

}
