/* Copyright (c) 2007-2014, Eddie Kohler
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
#include <sstream>

namespace tamer {
namespace tamerpriv {
timeval recent;
bool need_recent = true;
} // namespace tamerpriv

driver* driver::main;
driver* driver::indexed[capacity];
int driver::next_index;

driver::driver() {
    if (next_index < capacity) {
        index_ = next_index;
        ++next_index;
    } else
        for (index_ = 0; index_ != capacity && indexed[index_]; ++index_)
            /* do nothing */;
    assert(index_ < capacity);
    indexed[index_] = this;
}

driver::~driver() {
    indexed[index_] = 0;
    if (main == this)
        main = 0;
}

void driver::blocked_locations(std::vector<std::string>& x) {
    for (unsigned i = 0; i != this->nrendezvous(); ++i)
        if (tamerpriv::blocking_rendezvous* r = this->rendezvous(i)) {
            if (r->blocked())
                x.push_back(r->location_description());
        }
}

bool initialize(int flags) {
    if (driver::main)
        return true;

    if (!(flags & (use_tamer | use_libevent | use_libev))) {
        const char* dname = getenv("TAMER_DRIVER");
        if (dname && strcmp(dname, "libev") == 0)
            flags |= use_libev;
        else if (dname && strcmp(dname, "libevent") == 0)
            flags |= use_libevent;
        else
            flags |= use_tamer;
    }

    if (!driver::main && (flags & use_libev))
        driver::main = driver::make_libev();
    if (!driver::main && (flags & use_libevent))
        driver::main = driver::make_libevent();
    if (!driver::main && !(flags & use_tamer) && (flags & no_fallback))
        return false;
    if (!driver::main)
        driver::main = driver::make_tamer();

    if (!(flags & keep_sigpipe))
        signal(SIGPIPE, SIG_IGN);
    return true;
}

void cleanup() {
    delete driver::main;
    driver::main = 0;
}

void driver::at_delay(double delay, event<> e, bool bg)
{
    if (delay <= 0)
	at_asap(e);
    else {
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


namespace tamerpriv {

driver_asapset::~driver_asapset() {
    while (!empty())
	simple_event::unuse(ses_[head_ & capmask_]);
    delete[] ses_;
}

void driver_asapset::expand() {
    unsigned ncapmask = (capmask_ + 1 ? capmask_ * 4 + 3 : 31);
    tamerpriv::simple_event **na = new tamerpriv::simple_event *[ncapmask + 1];
    unsigned i = 0;
    for (unsigned x = head_; x != tail_; ++x, ++i)
	na[i] = ses_[x & capmask_];
    delete[] ses_;
    ses_ = na;
    capmask_ = ncapmask;
    head_ = 0;
    tail_ = i;
}

driver_timerset::~driver_timerset() {
    for (unsigned i = 0; i != nts_; ++i)
	simple_event::unuse(ts_[i].se);
    delete[] ts_;
}

#if 0
void driver_timerset::check() {
    fprintf(stderr, "---");
    for (unsigned k = 0; k != nts_; ++k)
	if (ts_[k].se->empty())
	    fprintf(stderr, " %3u: %ld.%06ld: EMPTY\n", k, ts_[k].when.tv_sec, ts_[k].when.tv_usec);
	else {
	    tamer_debug_closure *dc = ts_[k].se->rendezvous()->linked_debug_closure();
	    fprintf(stderr, " %3u: %ld.%06ld: @%s:%d\n", k, ts_[k].when.tv_sec, ts_[k].when.tv_usec, dc ? dc->tamer_blocked_file_ : "?", dc ? dc->tamer_blocked_line_ : 0);
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
}
#endif

void driver_timerset::expand() {
    unsigned ncap = (tcap_ ? (tcap_ * 4) + 3 : 31);
    trec *nts = new trec[ncap];
    memcpy(nts, ts_, sizeof(trec) * nts_);
    delete[] ts_;
    ts_ = nts;
    tcap_ = ncap;
}

void driver_timerset::push(timeval when, simple_event *se, bool bg) {
    using std::swap;

    // Remove empty trecs at heap's end
    while (nts_ != 0 && ts_[nts_ - 1].se->empty()) {
	--nts_;
	ts_[nts_].clean();
    }

    // Append new trec
    if (nts_ == tcap_)
	expand();
    unsigned top = nts_;
    ts_[top].when = when;
    order_ += 2;
    ts_[top].order = order_ + !bg;
    ts_[top].se = se;
    ++nts_;

    // Swap trec to proper position in heap
    unsigned i = top;
    while (i != 0) {
	unsigned trial = (i - (arity == 2)) / arity;
	if (ts_[trial].se->empty()) {
	    // Bubble empty trecs towards end of heap
	    unsigned xtrial = trial * arity + (arity == 2 || trial == 0),
		xendtrial = xtrial + arity - (arity != 2 && trial == 0),
		smallest = xtrial;
	    xendtrial = (xendtrial < nts_ ? xendtrial : nts_);
	    for (++xtrial; xtrial < xendtrial; ++xtrial)
		if (ts_[xtrial] < ts_[smallest])
		    smallest = xtrial;
	    ts_[trial].when = ts_[smallest].when;
	    ts_[trial].order = ts_[smallest].order;
	    swap(ts_[trial].se, ts_[smallest].se);
	    if (smallest != i)
		break;
	} else if (ts_[i] < ts_[trial]) {
	    swap(ts_[trial], ts_[i]);
	    i = trial;
	} else
	    break;
    }
}

void driver_timerset::pop_trigger() {
    assert(nts_ != 0);
    ts_[0].se->simple_trigger(false);
    hard_cull(true);
}

void driver_timerset::hard_cull(bool from_pop) const {
    using std::swap;
    assert(nts_ != 0);
    if (from_pop)
	goto skip_clean;

 again:
    assert(ts_[0].se->empty());
    ts_[0].clean();
 skip_clean:
    --nts_;
    if (nts_ == 0)
	return;
    ts_[0] = ts_[nts_];

    unsigned i = 0;
    while (1) {
	unsigned smallest = i,
	    trial = i * arity + (arity == 2 || i == 0),
	    end_trial = trial + arity - (arity != 2 && i == 0);
	end_trial = end_trial < nts_ ? end_trial : nts_;
	for (; trial < end_trial; ++trial)
	    if (ts_[trial] < ts_[smallest])
		smallest = trial;
	if (smallest == i)
	    break;
	swap(ts_[i], ts_[smallest]);
	i = smallest;
    }

    if (ts_[0].se->empty())
	goto again;
}

} // namespace tamerpriv
} // namespace tamer
