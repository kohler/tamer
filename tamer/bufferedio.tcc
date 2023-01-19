// -*- mode: c++ -*-
/* Copyright (c) 2007-2015, Eddie Kohler
 * Copyright (c) 2007, Regents of the University of California
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
#include <tamer/bufferedio.hh>
#include <string.h>

namespace tamer {

buffer::buffer(size_t initial_capacity) {
    for (size_ = 1; size_ < initial_capacity && size_ != 0; size_ *= 2) {
    }
    if (size_ == 0) {
        size_ = 1024;
    }
    buf_ = new char[size_];
}

buffer::~buffer() {
    delete[] buf_;
}

std::string buffer::str() const {
    size_t headpos = head_ & (size_ - 1);
    size_t tailpos = tail_ & (size_ - 1);
    if (headpos <= tailpos) {
        return std::string(buf_ + headpos, tailpos - headpos);
    } else {
        std::string s(buf_ + headpos, size_ - headpos);
        s.append(buf_, tailpos);
        return s;
    }
}

ssize_t buffer::fill_more(fd f, const event<int>& done) {
    if (!done || !f) {
        return -ECANCELED;
    }
    if (head_ + size_ == tail_) {
        char *new_buf = new char[size_ * 2];
        if (!new_buf) {
            return -ENOMEM;
        }
        size_t off = head_ & (size_ - 1);
        memcpy(new_buf, buf_ + off, size_ - off);
        memcpy(new_buf + size_ - off, buf_, off);
        head_ = 0;
        tail_ = size_;
        delete[] buf_;
        buf_ = new_buf;
        size_ = 2 * size_;
    }

    ssize_t amt;
    size_t headpos = head_ & (size_ - 1);
    size_t tailpos = tail_ & (size_ - 1);
    if (headpos <= tailpos) {
        amt = ::read(f.fdnum(), buf_ + tailpos, size_ - tailpos);
    } else {
        amt = ::read(f.fdnum(), buf_ + tailpos, headpos - tailpos);
    }

    if (amt != (ssize_t) -1) {
        return amt;
    } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return -EAGAIN;
    } else {
        return -errno;
    }
}

tamed void buffer::fill_until(fd f, char c, size_t max_size, size_t &out_size,
                              event<int> done) {
    tamed {
        int ret = -ECANCELED;
        size_t pos = this->head_;
        ssize_t amt;
    }

    out_size = 0;

    while (done) {
        for (; pos != tail_ && pos != head_ + max_size; ++pos) {
            if (buf_[pos & (size_ - 1)] == c) {
                ++pos;
                ret = 0;
                goto done;
            }
        }

        if (pos == head_ + max_size || !f) {
            ret = -E2BIG;
            break;
        }

        assert(pos == tail_);
        amt = fill_more(f, done);
        pos = tail_;
        if (amt == -EAGAIN) {
            twait volatile { tamer::at_fd_read(f.fdnum(), make_event()); }
        } else if (amt <= 0) {
            ret = (amt == 0 ? tamer::outcome::closed : amt);
            break;
        } else {
            tail_ += amt;
        }
    }

  done:
    out_size = pos - head_;
    done.trigger(ret);
}

tamed void buffer::take_until(fd f, char c, size_t max_size, std::string& str,
                              event<int> done) {
    tamed {
        size_t size;
        int ret;
        rendezvous<> r;
    }

    str = std::string();

    done.at_trigger(make_event(r));
    fill_until(f, c, max_size, size, make_event(r, ret));
    twait(r);

    if (done && ret == 0) {
        assert(size > 0);
        size_t a = head_ & (size_ - 1);
        size_t b = (head_ + size) & (size_ - 1);
        if (a < b) {
            str = std::string(buf_ + a, buf_ + b);
        } else {
            str = std::string(buf_ + a, buf_ + size_) + std::string(buf_, buf_ + b);
        }
        head_ += size;
    }
    done.trigger(ret);
}

}
