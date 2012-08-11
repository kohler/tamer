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

    virtual void at_fd(int fd, int action, const event<int> &e);
    virtual void store_time(const timeval &expiry, tamerpriv::simple_event *se);
    virtual void kill_fd(int fd);

    virtual bool empty();
    virtual void once();
    virtual void loop();

    struct eevent {
	::event libevent;
	event<int> trigger;
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
    e->trigger.trigger(0);
    e->trigger.~event();
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
	_etimer->trigger.~event();
	::event_del(&_etimer->libevent);
	_etimer = _etimer->next;
    }
    while (_efd) {
	_efd->trigger.~event();
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

void driver_libevent::at_fd(int fd, int action, const event<int> &trigger)
{
    assert(fd >= 0);
    if (!_efree)
	expand_events();

    eevent *e = _efree;
    _efree = e->next;
    event_set(&e->libevent, fd, (action == fdwrite ? EV_WRITE : EV_READ),
	      libevent_trigger, e);
    event_add(&e->libevent, 0);

    e->next = _efd;
    e->pprev = &_efd;
    if (_efd)
	_efd->pprev = &e->next;
    _efd = e;

    (void) new(static_cast<void *>(&e->trigger)) event<int>(trigger);
}

void driver_libevent::kill_fd(int fd)
{
    eevent **ep = &_efd;
    for (eevent *e = *ep; e; e = *ep)
	if (e->libevent.ev_fd == fd) {
	    event_del(&e->libevent);
	    e->trigger.trigger(-ECANCELED);
	    e->trigger.~event();
	    *ep = e->next;
	    if (*ep)
		(*ep)->pprev = ep;
	    e->next = _efree;
	    _efree = e;
	} else
	    ep = &e->next;
}

void driver_libevent::store_time(const timeval &expiry,
				 tamerpriv::simple_event *se)
{
    if (!_efree)
	expand_events();
    eevent *e = _efree;
    _efree = e->next;

    evtimer_set(&e->libevent, libevent_trigger, e);
    timeval timeout = expiry;
    timersub(&timeout, &now, &timeout);
    evtimer_add(&e->libevent, &timeout);

    tamerpriv::simple_event::use(se);
    (void) new(static_cast<void *>(&e->trigger)) event<int>(event<>::__make(se), no_slot());
    e->next = _etimer;
    e->pprev = &_etimer;
    if (_etimer)
	_etimer->pprev = &e->next;
    _etimer = e;
}

bool driver_libevent::empty()
{
    // remove dead events
    while (_etimer && !_etimer->trigger) {
	eevent *e = _etimer;
	::event_del(&e->libevent);
	e->trigger.~event();
	if ((_etimer = e->next))
	    _etimer->pprev = &_etimer;
	e->next = _efree;
	_efree = e;
    }
    while (_efd && !_efd->trigger) {
	eevent *e = _efd;
	if (e->libevent.ev_events)
	    ::event_del(&e->libevent);
	e->trigger.~event();
	if ((_efd = e->next))
	    _efd->pprev = &_efd;
	e->next = _efree;
	_efree = e;
    }

    if (_etimer || _efd
	|| sig_any_active || tamerpriv::abstract_rendezvous::has_unblocked())
	return false;
    return true;
}

void driver_libevent::once()
{
    if (tamerpriv::abstract_rendezvous::has_unblocked())
	::event_loop(EVLOOP_ONCE | EVLOOP_NONBLOCK);
    else
	::event_loop(EVLOOP_ONCE);
    set_now();
    while (tamerpriv::abstract_rendezvous *r = tamerpriv::abstract_rendezvous::pop_unblocked())
	r->run();
}

void driver_libevent::loop()
{
    while (1)
	once();
}

}

driver *driver::make_libevent()
{
    return new driver_libevent;
}

#else

driver *driver::make_libevent()
{
    return 0;
}

#endif
}
