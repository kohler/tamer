/* Copyright (c) 2007-2012, Eddie Kohler
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
#include <tamer/tamer.hh>
#include "dinternal.hh"
#include <sys/select.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

namespace tamer {
namespace {

class driver_tamer : public driver { public:

    driver_tamer();
    ~driver_tamer();

    virtual void at_fd(int fd, int action, event<int> e);
    virtual void at_time(const timeval &expiry, event<> e);
    virtual void at_asap(event<> e);
    virtual void kill_fd(int fd);

    virtual void loop(loop_flags flags);

  private:

    struct fdp {
	inline fdp(driver_tamer *, int) {
	}
	inline void move(driver_tamer *, int, fdp &) {
	}
    };

    union xfd_set {
	fd_set fds;
	char s[1];
    };

    tamerpriv::driver_fdset<fdp> fds_;
    int fdbound_;
    xfd_set *_fdset[4];
    int _fdset_cap;

    tamerpriv::driver_timerset timers_;

    tamerpriv::driver_asapset asap_;

    static void fd_disinterest(void *driver, int fd);
    void update_fds();
};


driver_tamer::driver_tamer()
    : fdbound_(0), _fdset_cap(sizeof(xfd_set) * 8) {
    assert(FD_SETSIZE <= _fdset_cap);
    for (int i = 0; i < 4; ++i) {
	_fdset[i] = reinterpret_cast<xfd_set *>(new char[_fdset_cap / 8]);
	if (i < 2)
	    memset(_fdset[i], 0, _fdset_cap / 8);
    }
    set_now();
}

driver_tamer::~driver_tamer() {
    // free fd_sets
    for (int i = 0; i < 4; ++i)
	delete[] reinterpret_cast<char *>(_fdset[i]);
}

void driver_tamer::fd_disinterest(void *arg, int fd) {
    driver_tamer *d = static_cast<driver_tamer *>(arg);
    d->fds_.push_change(fd);
}

void driver_tamer::at_fd(int fd, int action, event<int> e) {
    assert(fd >= 0);
    if (e && (action == 0 || action == 1)) {
	fds_.expand(this, fd);
	tamerpriv::driver_fd<fdp> &x = fds_[fd];
	if (x.e[action])
	    e = tamer::distribute(TAMER_MOVE(x.e[action]), TAMER_MOVE(e));
	x.e[action] = e;
	tamerpriv::simple_event::at_trigger(e.__get_simple(),
					    fd_disinterest, this, fd);
	fds_.push_change(fd);
    }
}

void driver_tamer::kill_fd(int fd) {
    if (fd >= 0 && fd < fds_.size()) {
	tamerpriv::driver_fd<fdp> &x = fds_[fd];
	for (int action = 0; action < 2; ++action)
	    x.e[action].trigger(-ECANCELED);
	fds_.push_change(fd);
    }
}

void driver_tamer::update_fds() {
    int fd;
    while ((fd = fds_.pop_change()) >= 0) {
	tamerpriv::driver_fd<fdp> &x = fds_[fd];
	if (fd >= _fdset_cap) {
	    int ncap = _fdset_cap * 2;
	    while (ncap < fd)
		ncap *= 2;
	    for (int acti = 0; acti < 4; ++acti) {
		xfd_set *x = reinterpret_cast<xfd_set *>(new char[ncap / 8]);
		if (acti < 2) {
		    memcpy(x, _fdset[acti], _fdset_cap / 8);
		    memset(x->s + _fdset_cap / 8, 0, (ncap - _fdset_cap) / 8);
		}
		delete[] reinterpret_cast<char *>(_fdset[acti]);
		_fdset[acti] = x;
	    }
	    _fdset_cap = ncap;
	}
	for (int action = 0; action < 2; ++action)
	    if (x.e[action])
		FD_SET(fd, &_fdset[action]->fds);
	    else
		FD_CLR(fd, &_fdset[action]->fds);
	if (x.e[0] || x.e[1]) {
	    if (fd >= fdbound_)
		fdbound_ = fd + 1;
	} else if (fd + 1 == fdbound_)
	    do {
		--fdbound_;
	    } while (fdbound_ > 0
		     && !fds_[fdbound_ - 1].e[0]
		     && !fds_[fdbound_ - 1].e[1]);
    }
}

void driver_tamer::at_time(const timeval &expiry, event<> e) {
    if (e)
	timers_.push(expiry, e.__take_simple());
}

void driver_tamer::at_asap(event<> e) {
    if (e)
	asap_.push(e.__take_simple());
}

void driver_tamer::loop(loop_flags flags)
{
 again:
    // fix file descriptors
    if (fds_.has_change())
	update_fds();

    // determine timeout
    struct timeval to, *toptr;
    timers_.cull();
    if (!asap_.empty()
	|| (!timers_.empty() && !timercmp(&timers_.expiry(), &now, >))
	|| sig_any_active
	|| tamerpriv::blocking_rendezvous::has_unblocked()) {
	timerclear(&to);
	toptr = &to;
    } else if (!timers_.empty()) {
	timersub(&timers_.expiry(), &now, &to);
	toptr = &to;
    } else if (fdbound_ == 0 && sig_nforeground == 0)
	// no events scheduled!
	return;
    else
	toptr = 0;

    // select!
    int nfds = fdbound_;
    if (sig_pipe[0] > nfds)
	nfds = sig_pipe[0] + 1;
    if (nfds > 0) {
	memcpy(_fdset[fdread + 2], _fdset[fdread], ((nfds + 63) & ~63) >> 3);
	memcpy(_fdset[fdwrite + 2], _fdset[fdwrite], ((nfds + 63) & ~63) >> 3);
	if (sig_pipe[0] >= 0)
	    FD_SET(sig_pipe[0], &_fdset[fdread + 2]->fds);
    }
    if (nfds > 0 || !toptr || to.tv_sec != 0 || to.tv_usec != 0)
	nfds = select(nfds, &_fdset[fdread + 2]->fds,
		      &_fdset[fdwrite + 2]->fds, 0, toptr);

    // run signals
    if (sig_any_active)
	dispatch_signals();

    // run asaps
    while (!asap_.empty())
	asap_.pop_trigger();

    // run file descriptors
    if (nfds > 0) {
	for (int fd = 0; fd < fdbound_; ++fd) {
	    tamerpriv::driver_fd<fdp> &x = fds_[fd];
	    for (int action = 0; action < 2; ++action)
		if (FD_ISSET(fd, &_fdset[2 + action]->fds) && x.e[action])
		    x.e[action].trigger(0);
	}
    }

    // run the timers that worked
    set_now();
    while (!timers_.empty() && !timercmp(&timers_.expiry(), &now, >))
	timers_.pop_trigger();

    // run active closures
    while (tamerpriv::blocking_rendezvous *r = tamerpriv::blocking_rendezvous::pop_unblocked())
	r->run();

    // check flags
    if (flags == loop_forever)
	goto again;
}

} // namespace

driver *driver::make_tamer() {
    return new driver_tamer;
}

} // namespace tamer
