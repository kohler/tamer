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
#if HAVE_LIBEVENT
#include <event.h>
#endif

namespace tamer {
#if HAVE_LIBEVENT
namespace {

extern "C" {
void libevent_fdtrigger(int fd, short, void *arg);
}

class driver_libevent : public driver {
  public:
    driver_libevent();
    ~driver_libevent();

    virtual void at_fd(int fd, int action, event<int> e);
    virtual void at_time(const timeval &expiry, event<> e);
    virtual void at_asap(event<> e);
    virtual void kill_fd(int fd);

    void cull();
    virtual void loop(loop_flags flags);

    struct eevent {
	::event libevent;
	tamerpriv::simple_event *se;
	eevent *next;
	eevent **pprev;
	driver_libevent *driver;
    };

    struct event_group {
	event_group *next;
	eevent e[1];
    };

    struct fdp {
	::event base;

	inline fdp(driver_libevent *d, int fd) {
	    ::event_set(&base, fd, 0, libevent_fdtrigger, d);
	}
	inline ~fdp() {
	    ::event_del(&base);
	}
	inline void move(driver_libevent *d, int fd, fdp &other) {
	    int x = ::event_pending(&other.base, EV_READ | EV_WRITE, 0)
		& (EV_READ | EV_WRITE);
	    ::event_del(&other.base);
	    ::event_set(&base, fd, x | EV_PERSIST, libevent_fdtrigger, d);
	    if (x)
		::event_add(&base, 0);
	}
    };

    tamerpriv::driver_fdset<fdp> fds_;
    int fdactive_;
    ::event signal_base_;

    eevent *_etimer;

    tamerpriv::driver_asapset asap_;

    event_group *_egroup;
    eevent *_efree;
    size_t _ecap;
    eevent *_esignal;

    void expand_events();
    static void fd_disinterest(void *driver, int fd);
    void update_fds();
};


extern "C" {
void libevent_fdtrigger(int fd, short what, void *arg) {
    driver_libevent *d = static_cast<driver_libevent *>(arg);
    tamerpriv::driver_fd<driver_libevent::fdp> &x = d->fds_[fd];
    if (what & EV_READ)
	x.e[0].trigger(0);
    if (what & EV_WRITE)
	x.e[1].trigger(0);
    d->fds_.push_change(fd);
}

void libevent_trigger(int, short, void *arg) {
    driver_libevent::eevent *e = static_cast<driver_libevent::eevent *>(arg);
    e->se->simple_trigger(true);
    *e->pprev = e->next;
    if (e->next)
	e->next->pprev = e->pprev;
    e->next = e->driver->_efree;
    e->driver->_efree = e;
}

void libevent_sigtrigger(int, short, void *arg) {
    driver_libevent *d = static_cast<driver_libevent *>(arg);
    d->dispatch_signals();
}
} // extern "C"


driver_libevent::driver_libevent()
    : fdactive_(0), _etimer(0), _egroup(0), _efree(0), _ecap(0)
{
    ::event_init();
    ::event_priority_init(3);
#if HAVE_EVENT_GET_STRUCT_EVENT_SIZE
    assert(sizeof(::event) >= event_get_struct_event_size());
#endif
    set_now();
    at_signal(0, event<>());	// create signal_fd pipe
    ::event_set(&signal_base_, sig_pipe[0], EV_READ | EV_PERSIST,
		libevent_sigtrigger, this);
    ::event_priority_set(&signal_base_, 0);
    ::event_add(&signal_base_, 0);
}

driver_libevent::~driver_libevent() {
    // discard all active events
    while (_etimer) {
	_etimer->se->simple_trigger(false);
	::event_del(&_etimer->libevent);
	_etimer = _etimer->next;
    }
    ::event_del(&signal_base_);

    // free event groups
    while (_egroup) {
	event_group *next = _egroup->next;
	delete[] reinterpret_cast<unsigned char *>(_egroup);
	_egroup = next;
    }
}

void driver_libevent::expand_events() {
    size_t ncap = (_ecap ? _ecap * 2 : 16);

    event_group *ngroup = reinterpret_cast<event_group *>(new unsigned char[sizeof(event_group) + sizeof(eevent) * (ncap - 1)]);
    ngroup->next = _egroup;
    _egroup = ngroup;
    for (size_t i = 0; i < ncap; i++) {
	ngroup->e[i].driver = this;
	ngroup->e[i].next = _efree;
	_efree = &ngroup->e[i];
    }
}

void driver_libevent::fd_disinterest(void *arg, int fd) {
    driver_libevent *d = static_cast<driver_libevent *>(arg);
    d->fds_.push_change(fd);
}

void driver_libevent::at_fd(int fd, int action, event<int> e) {
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

void driver_libevent::kill_fd(int fd) {
    if (fd >= 0 && fd < fds_.size()) {
	tamerpriv::driver_fd<fdp> &x = fds_[fd];
	for (int action = 0; action < 2; ++action)
	    x.e[action].trigger(-ECANCELED);
	fds_.push_change(fd);
    }
}

void driver_libevent::update_fds() {
    int fd;
    while ((fd = fds_.pop_change()) >= 0) {
	tamerpriv::driver_fd<fdp> &x = fds_[fd];
	int want_what = (x.e[0] ? EV_READ : 0) | (x.e[1] ? EV_WRITE : 0);
	int have_what = ::event_pending(&x.base, EV_READ | EV_WRITE, 0)
	    & (EV_READ | EV_WRITE);
	if (want_what != have_what) {
	    fdactive_ += (want_what != 0) - (have_what != 0);
	    if (have_what != 0)
		::event_del(&x.base);
	    if (want_what != 0) {
		::event_set(&x.base, fd, want_what | EV_PERSIST,
			    libevent_fdtrigger, this);
		::event_add(&x.base, 0);
	    }
	}
    }
}

void driver_libevent::at_time(const timeval &expiry, event<> e)
{
    if (!_efree)
	expand_events();
    if (e) {
	eevent *ee = _efree;
	_efree = ee->next;

	evtimer_set(&ee->libevent, libevent_trigger, ee);
	timeval timeout = expiry;
	timersub(&timeout, &now, &timeout);
	evtimer_add(&ee->libevent, &timeout);

	ee->se = e.__take_simple();
	ee->next = _etimer;
	ee->pprev = &_etimer;
	if (_etimer)
	    _etimer->pprev = &ee->next;
	_etimer = ee;
    }
}

void driver_libevent::at_asap(event<> e) {
    if (e)
	asap_.push(e.__take_simple());
}

void driver_libevent::cull() {
    while (_etimer && !*_etimer->se) {
	eevent *e = _etimer;
	::event_del(&e->libevent);
	tamerpriv::simple_event::unuse_clean(_etimer->se);
	if ((_etimer = e->next))
	    _etimer->pprev = &_etimer;
	e->next = _efree;
	_efree = e;
    }
}

void driver_libevent::loop(loop_flags flags)
{
 again:
    // fix file descriptors
    if (fds_.has_change())
	update_fds();

    int event_flags = EVLOOP_ONCE;
    if (!asap_.empty()
	|| sig_any_active
	|| tamerpriv::blocking_rendezvous::has_unblocked())
	event_flags |= EVLOOP_NONBLOCK;
    else {
	cull();
	if (!_etimer && fdactive_ == 0 && sig_nforeground == 0)
	    // no events scheduled
	    return;
    }

    ::event_loop(event_flags);

    set_now();

    // run asaps
    while (!asap_.empty()) {
	tamerpriv::simple_event *se = asap_.pop();
	se->simple_trigger(false);
    }

    // run rendezvous
    while (tamerpriv::blocking_rendezvous *r = tamerpriv::blocking_rendezvous::pop_unblocked())
	r->run();

    if (flags == loop_forever)
	goto again;
}

} // namespace
#endif

driver *driver::make_libevent()
{
#if HAVE_LIBEVENT
    return new driver_libevent;
#else
    return 0;
#endif
}

} // namespace tamer
