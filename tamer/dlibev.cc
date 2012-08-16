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

typedef eev<ev_io> eev_io;
typedef eev<ev_timer> eev_timer;

template<class T>
struct eev_cluster : public tamerutil::slist_element {
    eev<T> e[1];
};

template<class T>
class eev_collection {
public:

    eev_collection (driver_libev *d) : _driver (d) , _ecap (0) {}

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

    bool is_empty() const { return _active.empty(); }

protected:
    void expand() {

	size_t ncap = (_ecap ? _ecap * 2 : 16);
	
	// Allocate space ncap of them
	size_t sz = sizeof(eev_cluster<T>) + sizeof(eev<T>) * (ncap - 1);
	unsigned char *buf = new unsigned char[sz];
	eev_cluster<T> *cl = reinterpret_cast<eev_cluster<T> *>(buf);

	_clusters.push_front (cl);
    
	// Use placement new to call ncap constructors....
	new (cl->e) eev<T>[ncap];
	
	for (size_t i = 0; i < ncap; i++) {
	    eev<T> *e = &cl->e[i];
	    e->driver = _driver;
	    _free.push_front (e);
	}
    }

    tamerutil::dlist<eev<T> >         _free;
    tamerutil::dlist<eev<T> >         _active;
    tamerutil::slist<eev_cluster<T> > _clusters;
    driver_libev *_driver;
    size_t _ecap;
};

class eev_io_collection : public eev_collection<ev_io> {
public:
    eev_io_collection(driver_libev *d) : eev_collection<ev_io>(d) {}

    ~eev_io_collection() 
    {
	eev<ev_io> *e;
	while ((e = _active.pop_front())) stop(e); 
    }

    void empty ()
    {
	eev<ev_io> *e;
	while ((e = _active.front()) && !*e->se) {
	    stop(e);
	    tamerpriv::simple_event::unuse_clean(e->se);
	    deactivate(e);
	}
    }

    void stop(eev<ev_io> *e);


    eev<ev_io> *get(bool active) 
    {
	eev<ev_io> * e = this->eev_collection<ev_io>::get(active);
	if (active) activate(e);
	return e;
    }

    void activate(eev<ev_io> *e) 
    {
	this->eev_collection<ev_io>::activate(e);
	_lookup.insert(pair_t (e->libev.fd, e));
    }

    void deactivate(eev<ev_io> *e) 
    {
	this->eev_collection<ev_io>::deactivate(e);
	remove_from_lookup (e);
    }

    void kill_fd (int fd) 
    {
	lookup_t::iterator first = _lookup.find (fd);
	lookup_t::iterator it;
	for (it = first; it != _lookup.end() && it->first == fd; it++) {
	    eev<ev_io> *e = it->second;
	    stop(e);
	    if (*e->se && e->slot)
		*e->slot = -ECANCELED;
	    e->se->simple_trigger(true);
	    this->eev_collection<ev_io>::deactivate(e);
	}
	_lookup.erase(first, it);
    }
	
private:

    void remove_from_lookup(eev<ev_io> *e) 
    {
	int fd = e->libev.fd;
	for (lookup_t::iterator it = _lookup.find(fd);
	     it != _lookup.end() && it->first == fd; it++) {
	    if (it->second == e) {
		_lookup.erase(it, ++it);
		break;
	    }
	}
    }

    typedef std::map<int, eev<ev_io> *> lookup_t;
    typedef std::pair<int, eev<ev_io> *> pair_t;
    lookup_t _lookup;
};

class eev_timer_collection : public eev_collection<ev_timer> {
public:
    eev_timer_collection(driver_libev *d) : eev_collection<ev_timer>(d) {}
    ~eev_timer_collection() 
    {
	eev<ev_timer> *e;
	while ((e = _active.pop_front())) stop(e);
    }

    void empty ()
    {
	eev<ev_timer> *e;
	while ((e = _active.front()) && !*e->se) {
	    stop(e);
	    tamerpriv::simple_event::unuse_clean(e->se);
	    deactivate(e);
	}
    }

    void stop(eev<ev_timer> *e);
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

    eev_io_collection    _ios;
    eev_timer_collection _timers;
    eev<ev_io>           *_esignal;

private:

};


void eev_io_collection::stop(eev<ev_io> *e) { ::ev_io_stop(e->driver->_eloop, &e->libev); }
void eev_timer_collection::stop(eev<ev_timer> *e) { ::ev_timer_stop(e->driver->_eloop, &e->libev); }

extern "C" {

void libev_io_trigger(struct ev_loop *, ev_io *arg, int) 
{
    eev_io *e = reinterpret_cast<eev_io *>(arg);
    ::ev_io_stop (e->driver->_eloop, arg);
    if (*e->se && e->slot)
	*e->slot = 0;
    e->se->simple_trigger(true);
    e->driver->_ios.deactivate(e);
}

void libev_timer_trigger(struct ev_loop *, ev_timer *arg, int)
{
    eev_timer *e = reinterpret_cast<eev_timer *>(arg);
    ::ev_timer_stop (e->driver->_eloop, arg);
    if (*e->se && e->slot)
	*e->slot = 0;
    e->se->simple_trigger(true);
    e->driver->_timers.deactivate(e);
}

void libev_sigtrigger(struct ev_loop *, ev_io *arg, int)
{
    eev<ev_io> *e = reinterpret_cast<eev<ev_io> *>(arg);
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

    ev_io *tmp = &_esignal->libev;

    ev_io_init(tmp, libev_sigtrigger, sig_pipe[0], EV_READ);
    ev_io_start(_eloop, tmp);
    ev_set_priority(tmp, 0);
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
	ev_io *tmp = &e->libev;
	ev_io_init(tmp, libev_io_trigger, fd, action);
	ev_io_start(_eloop, tmp);

    }
}

void 
driver_libev::kill_fd(int fd)
{
    _ios.kill_fd (fd);
}

void 
driver_libev::store_time(const timeval &expiry,
			 tamerpriv::simple_event *se)
{
    if (se) {
	eev<ev_timer> *e = _timers.get(true);
	e->se = se;
	e->slot = 0;

	timeval timeout = expiry;
	timersub(&timeout, &now, &timeout);
	ev_tstamp d = timeout.tv_sec;

	ev_timer *tmp = &e->libev;
	ev_timer_init(tmp, libev_timer_trigger, d, 0);
	ev_timer_start(_eloop, tmp);
    }
}

bool 
driver_libev::empty()
{
    // remove dead events
    _timers.empty();
    _ios.empty();

    bool ret = _timers.is_empty() && _ios.is_empty() && 
	!sig_any_active && !tamerpriv::abstract_rendezvous::has_unblocked();
    return ret;
}

void 
driver_libev::once()
{
    int flags = EVRUN_ONCE;
    if (tamerpriv::abstract_rendezvous::has_unblocked()) 
	flags |= EVRUN_NOWAIT;

    ::ev_run(_eloop, flags);

    set_now();
    while (tamerpriv::abstract_rendezvous *r = tamerpriv::abstract_rendezvous::pop_unblocked())
	r->run();
}

void 
driver_libev::loop()
{
    while (1)
	once();
}

}

driver *driver::make_libev()
{
    return new driver_libev;
}

#else

driver *driver::make_libev()
{
    return 0;
}

#endif
}
