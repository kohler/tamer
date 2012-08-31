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

    struct ttimer {
	timeval expiry_;
	unsigned order_;
	tamerpriv::simple_event *trigger_;
	ttimer(const timeval &expiry, unsigned order,
	       tamerpriv::simple_event *trigger)
	    : expiry_(expiry), order_(order), trigger_(trigger) {
	}
	bool operator>(const ttimer &x) const {
	    if (expiry_.tv_sec != x.expiry_.tv_sec)
		return expiry_.tv_sec > x.expiry_.tv_sec;
	    if (expiry_.tv_usec != x.expiry_.tv_usec)
		return expiry_.tv_usec > x.expiry_.tv_usec;
	    return (int) (order_ - x.order_) > 0;
	}
    };

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

    ttimer *t_;
    int nt_;
    int tcap_;
    unsigned torder_;

    driver_fdset<fdp> fds_;
    int fdbound_;
    xfd_set *_fdset[4];
    int _fdset_cap;

    tamerpriv::simple_event **asap_;
    unsigned asap_head_;
    unsigned asap_tail_;
    unsigned asap_capmask_;

    void expand_timers();
    void check_timers() const;
    void timer_reheapify_from(int pos);
    static void fd_disinterest(void *driver, int fd);
    void cull_timers();
    void update_fds();
    void expand_asap();

};


driver_tamer::driver_tamer()
    : t_(0), nt_(0), tcap_(0), torder_(0),
      fdbound_(0), _fdset_cap(sizeof(xfd_set) * 8),
      asap_(0), asap_head_(0), asap_tail_(0), asap_capmask_(-1U)
{
    assert(FD_SETSIZE <= _fdset_cap);
    expand_timers();
    for (int i = 0; i < 4; ++i) {
	_fdset[i] = reinterpret_cast<xfd_set *>(new char[_fdset_cap / 8]);
	if (i < 2)
	    memset(_fdset[i], 0, _fdset_cap / 8);
    }
    set_now();
}

driver_tamer::~driver_tamer()
{
    // destroy all active timers
    for (int i = 0; i < nt_; i++)
	tamerpriv::simple_event::unuse(t_[i].trigger_);
    delete[] reinterpret_cast<char *>(t_);

    // destroy all active asaps
    while (asap_head_ != asap_tail_) {
	tamerpriv::simple_event::unuse(asap_[asap_head_ & asap_capmask_]);
	++asap_head_;
    }
    delete[] asap_;

    // free fd_sets
    for (int i = 0; i < 4; ++i)
	delete[] reinterpret_cast<char *>(_fdset[i]);
}

void driver_tamer::expand_timers()
{
    int ntcap = (tcap_ ? ((tcap_ + 1) * 2 - 1) : 511);
    ttimer *nt = reinterpret_cast<ttimer *>(new char[sizeof(ttimer) * ntcap]);
    if (nt_ != 0)
	// take advantage of fact that memcpy() works on event<>
	memcpy(nt, t_, sizeof(ttimer) * nt_);
    delete[] reinterpret_cast<char *>(t_);
    t_ = nt;
    tcap_ = ntcap;
}

void driver_tamer::expand_asap()
{
    unsigned ncapmask =
	(asap_capmask_ + 1 ? ((asap_capmask_ + 1) * 4 - 1) : 31);
    tamerpriv::simple_event **na = new tamerpriv::simple_event *[ncapmask + 1];
    unsigned i = 0;
    for (unsigned x = asap_head_; x != asap_tail_; ++x, ++i)
	na[i] = asap_[x & asap_capmask_];
    delete[] asap_;
    asap_ = na;
    asap_capmask_ = ncapmask;
    asap_head_ = 0;
    asap_tail_ = i;
}

void driver_tamer::timer_reheapify_from(int pos)
{
    int npos;
    while (pos > 0
	   && (npos = (pos - 1) >> 1, t_[npos] > t_[pos])) {
	std::swap(t_[pos], t_[npos]);
	pos = npos;
    }

    while (1) {
	int smallest = pos;
	npos = 2*pos + 1;
	if (npos < nt_ && t_[smallest] > t_[npos])
	    smallest = npos;
	if (npos + 1 < nt_ && t_[smallest] > t_[npos + 1])
	    smallest = npos + 1, ++npos;
	if (smallest == pos)
	    break;
	std::swap(t_[pos], t_[smallest]);
	pos = smallest;
    }
#if 0
    if (_t + 1 < tend || !will_delete)
	_timer_expiry = tbegin[0]->expiry;
    else
	_timer_expiry = Timestamp();
#endif
}

#if 0
void driver_tamer::check_timers() const
{
    fprintf(stderr, "---");
    for (int k = 0; k < _nt; k++)
	fprintf(stderr, " %p/%d.%06d", _t[k], _t[k]->expiry.tv_sec, _t[k]->expiry.tv_usec);
    fprintf(stderr, "\n");

    for (int i = 0; i < _nt / 2; i++)
	for (int j = 2*i + 1; j < 2*i + 3; j++)
	    if (j < _nt && *_t[i] > *_t[j]) {
		fprintf(stderr, "***");
		for (int k = 0; k < _nt; k++)
		    fprintf(stderr, (k == i || k == j ? " **%d.%06d**" : " %d.%06d"), _t[k]->expiry.tv_sec, _t[k]->expiry.tv_usec);
		fprintf(stderr, "\n");
		assert(0);
	    }
}
#endif

void driver_tamer::fd_disinterest(void *arg, int fd) {
    driver_tamer *d = static_cast<driver_tamer *>(arg);
    d->fds_.push_change(fd);
}

void driver_tamer::at_fd(int fd, int action, event<int> e) {
    assert(fd >= 0);
    if (e && (action == 0 || action == 1)) {
	fds_.expand(this, fd);
	driver_fd<fdp> &x = fds_[fd];
	if (x.e[action])
	    e = tamer::distribute(TAMER_MOVE(x.e[action]), TAMER_MOVE(e));
	x.e[action] = e;
	tamerpriv::simple_event::at_trigger(e.__get_simple(),
					    fd_disinterest, this, fd);
	fds_.push_change(fd);
    }
}

void driver_tamer::kill_fd(int fd) {
    if (fd >= 0 && fd < fds_.nfds_) {
	driver_fd<fdp> &x = fds_[fd];
	for (int action = 0; action < 2; ++action)
	    x.e[action].trigger(-ECANCELED);
	fds_.push_change(fd);
    }
}

void driver_tamer::update_fds() {
    int fd;
    while ((fd = fds_.pop_change()) >= 0) {
	driver_fd<fdp> &x = fds_[fd];
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

void driver_tamer::at_time(const timeval &expiry, event<> e)
{
    if (e) {
	if (nt_ == tcap_)
	    expand_timers();
	(void) new(static_cast<void *>(&t_[nt_])) ttimer(expiry, ++torder_,
							 e.__take_simple());
	++nt_;
	timer_reheapify_from(nt_ - 1);
    }
}

void driver_tamer::at_asap(event<> e)
{
    if (e) {
	if (asap_tail_ - asap_head_ == asap_capmask_ + 1)
	    expand_asap();
	asap_[asap_tail_ & asap_capmask_] = e.__take_simple();
	++asap_tail_;
    }
}

void driver_tamer::cull_timers() {
    while (nt_ != 0 && t_[0].trigger_->empty()) {
	tamerpriv::simple_event::unuse(t_[0].trigger_);
	--nt_;
	if (nt_ != 0) {
	    t_[0] = t_[nt_];
	    timer_reheapify_from(0);
	}
    }
}

void driver_tamer::loop(loop_flags flags)
{
 again:
    // fix file descriptors
    if (fds_.has_change())
	update_fds();

    // determine timeout
    cull_timers();
    struct timeval to, *toptr;
    if (asap_head_ != asap_tail_
	|| (nt_ != 0 && !timercmp(&t_[0].expiry_, &now, >))
	|| sig_any_active
	|| tamerpriv::blocking_rendezvous::has_unblocked()) {
	timerclear(&to);
	toptr = &to;
    } else if (nt_ != 0) {
	timersub(&t_[0].expiry_, &now, &to);
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
    while (asap_head_ != asap_tail_) {
	tamerpriv::simple_event *se = asap_[asap_head_ & asap_capmask_];
	++asap_head_;
	se->simple_trigger(false);
    }

    // run file descriptors
    if (nfds > 0) {
	for (int fd = 0; fd < fdbound_; ++fd) {
	    driver_fd<fdp> &x = fds_[fd];
	    for (int action = 0; action < 2; ++action)
		if (FD_ISSET(fd, &_fdset[2 + action]->fds) && x.e[action])
		    x.e[action].trigger(0);
	}
    }

    // run the timers that worked
    if (nt_ != 0) {
	set_now();
	while (nt_ != 0 && !timercmp(&t_[0].expiry_, &now, >)) {
	    tamerpriv::simple_event *trigger = t_[0].trigger_;
	    --nt_;
	    if (nt_ != 0) {
		t_[0] = t_[nt_];
		timer_reheapify_from(0);
	    }
	    trigger->simple_trigger(false);
	}
    }

    // run active closures
    while (tamerpriv::blocking_rendezvous *r = tamerpriv::blocking_rendezvous::pop_unblocked())
	r->run();

    // check flags
    if (flags == loop_forever)
	goto again;
}

} // namespace

driver *driver::make_tamer()
{
    return new driver_tamer;
}

} // namespace tamer
