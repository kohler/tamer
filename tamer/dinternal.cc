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
#include "dinternal.hh"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sstream>

namespace tamer {
namespace tamerpriv {
timeval recent;
bool need_recent = true;
time_type_t time_type = time_normal;
static timeval virtual_offset = { 1000000000, 0 };
simple_driver simple_driver::immediate_driver;
} // namespace tamerpriv

driver* driver::main;
driver* driver::indexed[capacity];
int driver::next_index;

driver::driver() {
    if (next_index < capacity) {
        index_ = next_index;
        ++next_index;
    } else {
        for (index_ = 0; index_ != capacity && indexed[index_]; ++index_) {
        }
    }
    assert(index_ < capacity);
    indexed[index_] = this;
}

driver::~driver() {
    indexed[index_] = nullptr;
    if (main == this) {
        main = nullptr;
    }
}

void driver::blocked_locations(std::vector<std::string>& x) {
    for (unsigned i = 0; i != this->nclosure_slots(); ++i) {
        if (tamerpriv::closure* c = this->closure_slot(i))
            x.push_back(c->location_description());
    }
}

void driver::set_error_handler(error_handler_type) {
}

bool initialize(int flags) {
    if (driver::main) {
        return true;
    }

    if (!(flags & (init_tamer | init_libevent | init_libev | init_strict))) {
        const char* dname = getenv("TAMER_DRIVER");
        if (dname && strcmp(dname, "libev") == 0) {
            flags |= init_libev;
        } else if (dname && strcmp(dname, "libevent") == 0) {
            flags |= init_libevent;
        } else {
            flags |= init_tamer;
        }
    }

    if (!driver::main && (flags & init_libev)) {
        driver::main = driver::make_libev();
    }
    if (!driver::main && (flags & init_libevent)) {
        driver::main = driver::make_libevent();
    }
    if (!driver::main && ((flags & init_tamer) || !(flags & init_strict))) {
        driver::main = driver::make_tamer(flags);
    }
    if (!driver::main) {
        return false;
    }

    if (!(flags & init_sigpipe)) {
        signal(SIGPIPE, SIG_IGN);
    }
    return true;
}

void cleanup() {
    while (driver::main->has_unblocked()) {
        driver::main->run_unblocked();
    }
    delete driver::main;
    driver::main = 0;
}

void set_time_type(time_type_t tt) {
    if (tamerpriv::time_type != tt) {
        if (tamerpriv::time_type == time_virtual) {
            tamerpriv::virtual_offset = tamerpriv::recent;
        }
        tamerpriv::time_type = tt;
        tamerpriv::need_recent = true;
        if (tamerpriv::time_type == time_virtual) {
            tamerpriv::recent = tamerpriv::virtual_offset;
        }
    }
}

timeval now() {
    if (tamerpriv::time_type == time_normal) {
        gettimeofday(&tamerpriv::recent, 0);
    } else {
        ++tamerpriv::recent.tv_usec;
        if (tamerpriv::recent.tv_usec == 1000000) {
            ++tamerpriv::recent.tv_sec;
            tamerpriv::recent.tv_usec = 0;
        }
    }
    tamerpriv::need_recent = false;
    return tamerpriv::recent;
}

void driver::at_delay(double delay, event<> e, bool bg) {
    if (delay <= 0) {
        at_asap(e);
    } else {
        timeval tv = recent();
        long ldelay = (long) delay;
        tv.tv_sec += ldelay;
        tv.tv_usec += (long) ((delay - ldelay) * 1000000 + 0.5);
        if (tv.tv_usec >= 1000000) {
            tv.tv_sec++;
            tv.tv_usec -= 1000000;
        }
        at_time(tv, e, bg);
    }
}

timeval driver::next_wake() const {
    timeval unknown = { 0, 0 };
    return unknown;
}


namespace tamerpriv {

driver_asapset::~driver_asapset() {
    while (!empty()) {
        simple_event::unuse(ses_[head_ & capmask_]);
        ++head_;
    }
    delete[] ses_;
}

void driver_asapset::expand() {
    unsigned ncapmask = (capmask_ + 1 ? capmask_ * 4 + 3 : 31);
    tamerpriv::simple_event **na = new tamerpriv::simple_event *[ncapmask + 1];
    unsigned i = 0;
    for (unsigned x = head_; x != tail_; ++x, ++i) {
        na[i] = ses_[x & capmask_];
    }
    delete[] ses_;
    ses_ = na;
    capmask_ = ncapmask;
    head_ = 0;
    tail_ = i;
}

driver_timerset::~driver_timerset() {
    for (unsigned i = 0; i != nts_; ++i) {
        simple_event::unuse(ts_[i].se);
    }
    delete[] ts_;
}

void driver_timerset::check() {
#if 0
    fprintf(stderr, "---");
    for (unsigned k = 0; k != nts_; ++k) {
        std::string location;
        if (ts_[k].se->empty())
            location = "EMPTY";
        else {
            abstract_rendezvous* r = ts_[k].se->rendezvous();
            location = "?";
            if (r->rtype() == rgather || r->rtype() == rexplicit) {
                blocking_rendezvous* br = static_cast<blocking_rendezvous*>(r);
                if (br->blocked_closure_)
                    location = "@" + br->blocked_closure_->location_description();
            }
        }
        fprintf(stderr, " %3u: %ld.%06ld%s: %s\n", k, ts_[k].when.tv_sec, ts_[k].when.tv_usec,
                ts_[k].order & 1 ? "" : "-", location.c_str());
    }
    fprintf(stderr, "\n");

    for (unsigned i = 0; true; ++i) {
        unsigned trial = i * arity + (arity == 2 || i == 0),
            end_trial = trial + arity - (arity != 2 && i == 0);
        if (trial >= nts_)
            break;
        end_trial = (end_trial < nts_ ? end_trial : nts_);
        for (; trial < end_trial; ++trial)
            if (ts_[trial] < ts_[i]) {
                fprintf(stderr, "***");
                for (unsigned k = 0; k != nts_; ++k)
                    fprintf(stderr, (k == i || k == trial ? " **%ld.%06ld**" : " %ld.%06ld"), ts_[k].when.tv_sec, ts_[k].when.tv_usec);
                fprintf(stderr, "\n");
                assert(0);
            }
    }
#endif
}

void driver_timerset::expand() {
    unsigned ncap = (tcap_ ? (tcap_ * 4) + 3 : 31);
    trec *nts = new trec[ncap];
    if (ts_) {
        memcpy(nts, ts_, sizeof(trec) * nts_);
    }
    delete[] ts_;
    ts_ = nts;
    tcap_ = ncap;
}

void driver_timerset::push(timeval when, simple_event* se, bool bg) {
    using std::swap;
    assert(!se->empty());

    // Append new trec
    if (nts_ == tcap_) {
        expand();
    }
    unsigned pos = nts_;
    ts_[pos].when = when;
    order_ += 2;
    ts_[pos].order = order_ + !bg;
    ts_[pos].se = se;
    ++nts_;
    nfg_ += !bg;

    // Swap trec to proper position in heap
    while (pos != 0) {
        unsigned p = heap_parent(pos);
        if (!(ts_[pos] < ts_[p])) {
            break;
        }
        swap(ts_[pos], ts_[p]);
        pos = p;
    }

    // If heap is largish, check to see if a random element is empty.
    // If it's empty, remove it, and look for another empty element.
    // This should help keep the timer heap small even if we set many
    // more timers than get a chance to fire.
    while (nts_ >= 32) {
        pos = rand_ % nts_;
        rand_ = rand_ * 1664525 + 1013904223U; // Numerical Recipes LCG
        if (!ts_[pos].se->empty()) {
            break;
        }
        hard_cull(pos);
    }
}

void driver_timerset::hard_cull(unsigned pos) const {
    using std::swap;
    assert(nts_ != 0);

    if (pos == (unsigned) -1) {
        pos = 0;
        ts_[pos].se->simple_trigger(false);
    } else {
        ts_[pos].clean();
    }

    --nts_;
    nfg_ -= ts_[pos].order & 1;
    if (nts_ == pos) {
        return;
    }
    swap(ts_[nts_], ts_[pos]);

    if (pos == 0 || !(ts_[pos] < ts_[heap_parent(pos)])) {
        while (true) {
            unsigned smallest = pos;
            for (unsigned t = heap_first_child(pos);
                 t < heap_last_child(pos); ++t) {
                if (ts_[t] < ts_[smallest])
                    smallest = t;
            }
            if (smallest == pos) {
                break;
            }
            swap(ts_[pos], ts_[smallest]);
            pos = smallest;
        }
    } else {
        do {
            unsigned p = heap_parent(pos);
            swap(ts_[pos], ts_[p]);
            pos = p;
        } while (pos && ts_[pos] < ts_[heap_parent(pos)]);
    }
}

} // namespace tamerpriv
} // namespace tamer
