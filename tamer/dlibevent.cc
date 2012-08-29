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
#if HAVE_LIBEVENT
#include <event.h>
#endif

namespace tamer {
#if HAVE_LIBEVENT
namespace {

class driver_libevent : public driver { public:

    driver_libevent();
    ~driver_libevent();

    virtual void at_fd(int fd, int action, event<int> e);
    virtual void at_time(const timeval &expiry, event<> e);
    virtual void kill_fd(int fd);

    void cull();
    virtual bool empty();
    virtual void loop(loop_flags flags);

    struct eevent {
	::event libevent;
	tamerpriv::simple_event *se;
	int *slot;
	eevent *next;
	eevent **pprev;
	driver_libevent *driver;
    };

    struct event_group {
	event_group *next;
	eevent e[1];
    };

    eevent *_etimer;
    eevent *_efd;

    event_group *_egroup;
    eevent *_efree;
    size_t _ecap;
    eevent *_esignal;

    void expand_events();

};


extern "C" {
void libevent_trigger(int, short, void *arg)
{
    driver_libevent::eevent *e = static_cast<driver_libevent::eevent *>(arg);
    if (*e->se && e->slot)
	*e->slot = 0;
    e->se->simple_trigger(true);
    *e->pprev = e->next;
    if (e->next)
	e->next->pprev = e->pprev;
    e->next = e->driver->_efree;
    e->driver->_efree = e;
}

void libevent_sigtrigger(int, short, void *arg)
{
    driver_libevent::eevent *e = static_cast<driver_libevent::eevent *>(arg);
    e->driver->dispatch_signals();
}
}


driver_libevent::driver_libevent()
    : _etimer(0), _efd(0), _egroup(0), _efree(0), _ecap(0)
{
    set_now();
    event_init();
    event_priority_init(3);
    at_signal(0, event<>());	// create signal_fd pipe
    expand_events();
    _esignal = _efree;
    _efree = _efree->next;
    ::event_set(&_esignal->libevent, sig_pipe[0], EV_READ | EV_PERSIST,
		libevent_sigtrigger, 0);
    ::event_priority_set(&_esignal->libevent, 0);
    ::event_add(&_esignal->libevent, 0);
}

driver_libevent::~driver_libevent()
{
    // discard all active events
    while (_etimer) {
	_etimer->se->simple_trigger(false);
	::event_del(&_etimer->libevent);
	_etimer = _etimer->next;
    }
    while (_efd) {
	_efd->se->simple_trigger(false);
	if (_efd->libevent.ev_events)
	    ::event_del(&_efd->libevent);
	_efd = _efd->next;
    }
    ::event_del(&_esignal->libevent);

    // free event groups
    while (_egroup) {
	event_group *next = _egroup->next;
	delete[] reinterpret_cast<unsigned char *>(_egroup);
	_egroup = next;
    }
}

void driver_libevent::expand_events()
{
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

void driver_libevent::at_fd(int fd, int action, event<int> e)
{
    assert(fd >= 0);
    if (!_efree)
	expand_events();
    if (e) {
	eevent *ee = _efree;
	_efree = ee->next;
	event_set(&ee->libevent, fd, (action == fdwrite ? EV_WRITE : EV_READ),
		  libevent_trigger, ee);
	event_add(&ee->libevent, 0);

	ee->se = e.__take_simple();
	ee->slot = e.__get_slot0();
	ee->next = _efd;
	ee->pprev = &_efd;
	if (_efd)
	    _efd->pprev = &ee->next;
	_efd = ee;
    }
}

void driver_libevent::kill_fd(int fd)
{
    eevent **ep = &_efd;
    for (eevent *e = *ep; e; e = *ep)
	if (e->libevent.ev_fd == fd) {
	    event_del(&e->libevent);
	    if (*e->se && e->slot)
		*e->slot = -ECANCELED;
	    e->se->simple_trigger(true);
	    *ep = e->next;
	    if (*ep)
		(*ep)->pprev = ep;
	    e->next = _efree;
	    _efree = e;
	} else
	    ep = &e->next;
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
	ee->slot = 0;
	ee->next = _etimer;
	ee->pprev = &_etimer;
	if (_etimer)
	    _etimer->pprev = &ee->next;
	_etimer = ee;
    }
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
    while (_efd && !*_efd->se) {
	eevent *e = _efd;
	if (e->libevent.ev_events)
	    ::event_del(&e->libevent);
	tamerpriv::simple_event::unuse_clean(_etimer->se);
	if ((_efd = e->next))
	    _efd->pprev = &_efd;
	e->next = _efree;
	_efree = e;
    }
}

bool driver_libevent::empty() {
    cull();
    if (_etimer
	|| _efd
	|| sig_any_active
	|| tamerpriv::abstract_rendezvous::has_unblocked())
	return false;
    return true;
}

void driver_libevent::loop(loop_flags flags)
{
 again:
    int event_flags = EVLOOP_ONCE;
    if (sig_any_active
	|| tamerpriv::abstract_rendezvous::has_unblocked())
	event_flags |= EVLOOP_NONBLOCK;
    else {
	cull();
	if (!_etimer && !_efd && sig_nforeground == 0) // no events scheduled
	    return;
    }
    ::event_loop(event_flags);
    set_now();
    while (tamerpriv::abstract_rendezvous *r = tamerpriv::abstract_rendezvous::pop_unblocked())
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
