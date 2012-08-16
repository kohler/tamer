// -*- mode: c++; tab-width: 8; c-basic-offset: 4;  -*-

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
#include <tamer/util.hh>
#include <map>
#if HAVE_LIBEV
#include <ev.h>
#endif

namespace tamer {
#if HAVE_LIBEV
namespace {

class driver_libev;

template<class T>
struct eev : public tamerutil::dlist_element {
    T libev;
    tamerpriv::simple_event *se;
    int *slot;
    driver_libev *driver;
};

template<class T>
struct eev_cluster : public tamerutil::slist_element {
    eev<T> e[1];
};

template<class T>
class eev_collection {
public:

    eev_collection (driver_libev *d) : _driver (d) {}

    ~eev_collection() 
    {
	while (!_clusters.empty()) {
	    delete[] reinterpret_cast<unsigned char *>(_clusters.pop_front());
	}
    }

    eev<T> *get(bool active)
    {
	eev<T> *ret = _free.pop_front();
	if (!ret) expand();
	ret = _free.pop_front();
	if (active) activate (ret);
	return ret;
    }

    void activate(eev<T> *e) { _active.push_back(e); }
    void deactivate(eev<T> *e) 
    { 
	_active.remove(e); 
	_free.push_front(e);
    }
    

private:
    void expand() {

	size_t ncap = (_ecap ? _ecap * 2 : 16);
	
	// Allocate space ncap of them
	size_t sz = sizeof(eev_group) + sizeof(eev) * (ncap - 1);
	unsigned char *buf = new unsigned char[sz];
	eev_cluster<T> *cl = reinterpret_cast<eev_cluster<T> *>(buf);

	_clusters.push_front (cl);
    
	// Use placement new to call ncap constructors....
	new (cl->e) eev<T>[ncap];
	
	for (size_t i = 0; i < ncap; i++) {
	    eev<T> *e = &ngroup->e[i];
	    e->driver = _driver;
	    _free.push_front (e);
	}
    }


    tamer::dlist<eev<T> >         _free;
    tamer::dlist<eev<T> >         _active;
    tamer::slist<eev_cluster<T> > _clusters;
    driver_libev *_driver;
};

class eev_io_collection : public eev_collection<ev_io> {
public:
    ~eev_io_collection() 
    {
	eev<ev_io> *e;
	while ((e = _active.pop_front())) {
	    ::ev_io_stop (&e->libev);
	}
    }
    eev<T> *get(bool active) 
    {
	eev<T> * e = this->eev_collection<ev_io>::get(active);
	if (active) activate(e);
	return e;
    }

    void activate(eev<evio> *e) 
    {
	this->eev_collection<ev_io>::activate(e);
	_lookup.insert(std::pair<e->libev->fd, e>);
    }

    void deactivate(eev<evio> *e) 
    {
	this->eev_collection<ev_io>::deactivate(e);
	int fd = e->libev->fd;
	for (lookup_t::iterator it = _lookup.find(fd);
	     it != _lookup.end() && it->first == fd; it++) {
	    if (it->second == e) {
		_lookup.erase(it, it+1);
	    }
	}
    }

    void kill_fd (int fd) 
    {
	lookup_t::iterator first = _lookup.find (fd);
	lookup_t::iterator it;
	for (it = first; it != _lookup.end() && it->first == fd; it++) {
	    eev<ev_io> *e = it.second;
	    ::ev_io_stop(_eloop, e->libev);
	    if (*e->se && e->slot)
		*e->slot = -ECANCELED;
	    e->se->simple_trigger(true);
	    this->eev_collection<ev_io>::deactivate(e);
	}
	_lookup.erase(first, it);
    }
}
	
private:
    typedef std::map<int, eev<eio> *> lookup_t;
    lookup_t _lookup;
};

class eev_timer_collection : public eev_collection<ev_timer> {
public:
    ~eev_timer_collection() 
    {
	eev<ev_timer> *e;
	while ((e = _active.pop_front())) {
	    ::ev_timer_stop (&e->libev);
	}
    }
};

class driver_libev: public driver {
public:

    driver_libev();
    ~driver_libev();

    virtual void store_fd(int fd, int action, tamerpriv::simple_event *se,
                          int *slot);
    virtual void store_time(const timeval &expiry, tamerpriv::simple_event *se);
    virtual void kill_fd(int fd);

    virtual bool empty();
    virtual void once();
    virtual void loop();

    struct ev_loop *_eloop;

    eevent *_etimer;
    eevent *_efd;

    eev_io_collection    _ios;
    eev_timer_collection _timers;

    size_t _ecap;

    eev<ev_io>               *_esignal;

private:

};


extern "C" {
void libev_trigger(EVP_P_ ev_io *arg, int revents) 
{
    eev<ev_io> *e = static_cast<eev<ev_io> *>(arg);
    ev_io_stop (e->driver->_eloop, w);
    if (*e->se && e->slot)
	*e->slot = 0;
    e->se->simple_trigger(true);
    e->driver->_ios.deactivate(e);
}

void libev_sigtrigger(EVP_P_ ev_io *arg, int revents) {
{
    eev<ev_io> *e = static_cast<eev<ev_io> *>(arg);
    e->driver->dispatch_signals();
}
}


driver_libev::driver_libev()
    : _eloop  (::ev_default_loop(0)),
      _ios    (this),
      _timers (this)
{
    set_now();
    at_signal(0, event<>());	// create signal_fd pipe
    _esignal = _ios.get(false);

    ::ev_io_init(&_esignal->libev, libev_sigtrigger, sig_pipe[0], EV_READ);
    ::ev_io_start(_eloop, &_esignal->libev);
    ::ev_set_priroty(&_esignal->libev, 0);
}

driver_libev::~driver_libev() 
{
    // Stop the special signal FD pipe.
    ::ev_io_stop (_eloop, &_esignal->libev);
}

void 
driver_libev::store_fd(int fd, int action,
		       tamerpriv::simple_event *se, int *slot)
{
    assert(fd >= 0);
    if (se) {
	eev<ev_io> *e = _ios.get(true);
	e->se = se;
	e->slot = slot;

	action = (action == fdwrite ? EV_WRITE : EV_READ);
	::ev_io_init(&e->libev, libev_trigger, fd, action);
	::ev_io_start(_eloop, &e->libev);

    }
}

void 
driver_libev::kill_fd(int fd)
{
    _ios.kill_fd (fd);
}

void driver_libevent::store_time(const timeval &expiry,
				 tamerpriv::simple_event *se)
{
    if (se) {
	eev<ev_timer> *e = _timers.get(true);
	e->se = se;
	e->slot = 0;

	timeval timeout = expiry;
	timersub(&timeout, &now, &timeout);
	ev_tstamp d = timeout.tv_sec;

	::ev_timer_init(&e->libev, libevent_trigger, d, 0);
	::ev_timer_start(_eloop, &e->libev);
    }
}

bool driver_libevent::empty()
{
    // remove dead events
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
