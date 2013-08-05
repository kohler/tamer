/* Copyright (c) 2007-2013, Eddie Kohler
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
using tamerpriv::make_fd_callback;
using tamerpriv::fd_callback_driver;
using tamerpriv::fd_callback_fd;

class driver_tamer : public driver {
  public:
    driver_tamer();
    ~driver_tamer();

    virtual void at_fd(int fd, int action, event<int> e);
    virtual void at_time(const timeval &expiry, event<> e);
    virtual void at_asap(event<> e);
    virtual void kill_fd(int fd);

    virtual void loop(loop_flags flags);
    virtual void break_loop();

  private:

    struct fdp {
	inline fdp(driver_tamer*, int) {
	}
    };

    union xfd_set {
	fd_set fds;
	char s[1];
        uint32_t u[1];
    };

    enum {
        fdreadnow = fdread + 2, fdwritenow = fdwrite + 2
    };

    tamerpriv::driver_fdset<fdp> fds_;
    int fdbound_;
    xfd_set *_fdset[4];
    int fdset_fdcap_;

    tamerpriv::driver_timerset timers_;

    tamerpriv::driver_asapset asap_;

    bool loop_state_;

    static void fd_disinterest(void* arg);
    void update_fds();
    int find_bad_fds();
};


driver_tamer::driver_tamer()
    : fdbound_(0), fdset_fdcap_(sizeof(xfd_set) * 8), loop_state_(false) {
    fdset_fdcap_ = (fdset_fdcap_ + 31) & ~31; // make multiple of 4 bytes
    assert(FD_SETSIZE <= fdset_fdcap_);
    assert((fdset_fdcap_ % 32) == 0);
    for (int i = 0; i < 4; ++i) {
	_fdset[i] = reinterpret_cast<xfd_set *>(new char[fdset_fdcap_ / 8]);
	if (i < fdreadnow)
	    memset(_fdset[i], 0, fdset_fdcap_ / 8);
    }
}

driver_tamer::~driver_tamer() {
    // free fd_sets
    for (int i = 0; i < 4; ++i)
	delete[] reinterpret_cast<char *>(_fdset[i]);
}

void driver_tamer::fd_disinterest(void* arg) {
    driver_tamer* d = static_cast<driver_tamer*>(fd_callback_driver(arg));
    d->fds_.push_change(fd_callback_fd(arg));
}

void driver_tamer::at_fd(int fd, int action, event<int> e) {
    assert(fd >= 0);
    if (e && (action == 0 || action == 1)) {
	fds_.expand(this, fd);
	tamerpriv::driver_fd<fdp> &x = fds_[fd];
	if (x.e[action])
	    e = tamer::distribute(TAMER_MOVE(x.e[action]), TAMER_MOVE(e));
	x.e[action] = e;
	tamerpriv::simple_event::at_trigger(e.__get_simple(), fd_disinterest,
                                            make_fd_callback(this, fd));
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
	if (fd >= fdset_fdcap_) {
	    int ncap = fdset_fdcap_ * 2;
	    while (ncap < fd)
		ncap *= 2;
	    for (int acti = 0; acti < 4; ++acti) {
		xfd_set *x = reinterpret_cast<xfd_set *>(new char[ncap / 8]);
		if (acti < 2) {
		    memcpy(x, _fdset[acti], fdset_fdcap_ / 8);
		    memset(&x->s[fdset_fdcap_ / 8], 0, (ncap - fdset_fdcap_) / 8);
		}
		delete[] reinterpret_cast<char *>(_fdset[acti]);
		_fdset[acti] = x;
	    }
	    fdset_fdcap_ = ncap;
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
    if (flags == loop_forever)
        loop_state_ = true;

 again:
    // fix file descriptors
    if (fds_.has_change())
	update_fds();

    // determine timeout
    struct timeval to, *toptr;
    timers_.cull();
    if (!asap_.empty()
	|| (!timers_.empty() && !timercmp(&timers_.expiry(), &now(), >))
	|| sig_any_active
	|| has_unblocked()) {
	timerclear(&to);
	toptr = &to;
    } else if (!timers_.empty()) {
	timersub(&timers_.expiry(), &now(), &to);
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
	memcpy(_fdset[fdreadnow], _fdset[fdread], ((nfds + 63) & ~63) >> 3);
	memcpy(_fdset[fdwritenow], _fdset[fdwrite], ((nfds + 63) & ~63) >> 3);
	if (sig_pipe[0] >= 0)
	    FD_SET(sig_pipe[0], &_fdset[fdreadnow]->fds);
    }
    if (nfds > 0 || !toptr || to.tv_sec != 0 || to.tv_usec != 0) {
	nfds = select(nfds, &_fdset[fdreadnow]->fds,
		      &_fdset[fdwritenow]->fds, 0, toptr);
         if (nfds == -1 && errno == EBADF)
             nfds = find_bad_fds();
    }
    set_now();

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
		if (FD_ISSET(fd, &_fdset[action + 2]->fds) && x.e[action])
		    x.e[action].trigger(0);
	}
    }

    // run the timers that worked
    while (!timers_.empty() && !timercmp(&timers_.expiry(), &now(), >))
	timers_.pop_trigger();

    // run active closures
    while (tamerpriv::blocking_rendezvous *r = pop_unblocked())
	r->run();

    // check flags
    if (flags == loop_forever && loop_state_)
	goto again;
}

int driver_tamer::find_bad_fds() {
    // first, combine all file descriptors from read & write
    memcpy(_fdset[fdreadnow], _fdset[fdread], (fdbound_ + 31) & ~31);
    for (int i = 0; i < fdbound_ / 32; ++i)
        _fdset[fdreadnow]->u[i] |= _fdset[fdwrite]->u[i];
    // use binary search to find a bad file descriptor
    int l = 0, r = (fdbound_ + 7) & ~7;
    while (r - l > 1) {
        int m = l + ((r - l) >> 1);
        memset(_fdset[fdwritenow], 0, l);
        memcpy(&_fdset[fdwritenow]->s[l], &_fdset[fdreadnow]->s[l], m - l);
        struct timeval tv = {0, 0};
        int nfds = select(m << 3, &_fdset[fdwritenow]->fds, 0, 0, &tv);
        if (nfds == -1 && errno == EBADF)
            r = m;
        else if (nfds != -1)
            l = m;
    }
    // down to <= 8 file descriptors; test them one by one
    // clear result sets
    memset(_fdset[fdreadnow], 0, ((fdbound_ + 63) & ~63) >> 3);
    memset(_fdset[fdwritenow], 0, ((fdbound_ + 63) & ~63) >> 3);
    // set up result sets
    int nfds = 0;
    for (int f = l * 8; f != r * 8; ++f) {
        int fr = FD_ISSET(f, &_fdset[fdread]->fds);
        int fw = FD_ISSET(f, &_fdset[fdwrite]->fds);
        if ((fr || fw) && fcntl(f, F_GETFL) == -1 && errno == EBADF) {
            if (fr)
                FD_SET(f, &_fdset[fdreadnow]->fds);
            if (fw)
                FD_SET(f, &_fdset[fdwritenow]->fds);
            ++nfds;
        }
    }
    return nfds;
}

void driver_tamer::break_loop() {
    loop_state_ = false;
}

} // namespace

driver *driver::make_tamer() {
    return new driver_tamer;
}

} // namespace tamer
