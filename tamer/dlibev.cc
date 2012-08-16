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
struct wrapper : public T, 
		 public tamerutil::dlist_element {
    tamerpriv::simple_event *se;
    int *slot;
    driver_libev *driver;
};

typedef wrapper<ev_io> wrapper_io;
typedef wrapper<ev_timer> wrapper_timer;

template<class T>
struct wrapper_cluster : public tamerutil::slist_element {
    wrapper<T> e[1];
};

template<class T>
class wrapper_collection {
public:

    wrapper_collection (driver_libev *d) : _driver (d) , _ecap (0) {}

    ~wrapper_collection() 
    {
	while (!_clusters.empty()) {
	    delete[] reinterpret_cast<unsigned char *>(_clusters.pop_front());
	}
    }

    wrapper<T> *get(bool active)
    {
	wrapper<T> *ret = _free.pop_front();
	if (!ret) expand();
	ret = _free.pop_front();
	if (active) activate (ret);
	return ret;
    }

    void activate(wrapper<T> *e) { _active.push_back(e); }
    void deactivate(wrapper<T> *e) 
    { 
	_active.remove(e); 
	_free.push_front(e);
    }

    bool is_empty() const { return _active.empty(); }

protected:
    void expand() {

	size_t ncap = (_ecap ? _ecap * 2 : 16);
	
	// Allocate space ncap of them
	size_t sz = sizeof(wrapper_cluster<T>) + sizeof(wrapper<T>) * (ncap - 1);
	unsigned char *buf = new unsigned char[sz];
	wrapper_cluster<T> *cl = reinterpret_cast<wrapper_cluster<T> *>(buf);

	_clusters.push_front (cl);
    
	// Use placement new to call ncap constructors....
	new (cl->e) wrapper<T>[ncap];
	
	for (size_t i = 0; i < ncap; i++) {
	    wrapper<T> *e = &cl->e[i];
	    e->driver = _driver;
	    _free.push_front (e);
	}
    }

    tamerutil::dlist<wrapper<T> >         _free;
    tamerutil::dlist<wrapper<T> >         _active;
    tamerutil::slist<wrapper_cluster<T> > _clusters;
    driver_libev *_driver;
    size_t _ecap;
};

class wrapper_io_collection : public wrapper_collection<ev_io> {
public:
    wrapper_io_collection(driver_libev *d) : wrapper_collection<ev_io>(d) {}

    ~wrapper_io_collection() 
    {
	wrapper<ev_io> *e;
	while ((e = _active.pop_front())) stop(e); 
    }

    void empty ()
    {
	wrapper<ev_io> *e;
	while ((e = _active.front()) && !*e->se) {
	    stop(e);
	    tamerpriv::simple_event::unuse_clean(e->se);
	    deactivate(e);
	}
    }

    void stop(wrapper<ev_io> *e);


    wrapper<ev_io> *get(bool active) 
    {
	wrapper<ev_io> * e = this->wrapper_collection<ev_io>::get(active);
	if (active) activate(e);
	return e;
    }

    void activate(wrapper<ev_io> *e) 
    {
	this->wrapper_collection<ev_io>::activate(e);
	_lookup.insert(pair_t (e->fd, e));
    }

    void deactivate(wrapper<ev_io> *e) 
    {
	this->wrapper_collection<ev_io>::deactivate(e);
	remove_from_lookup (e);
    }

    void kill_fd (int fd) 
    {
	lookup_t::iterator first = _lookup.find (fd);
	lookup_t::iterator it;
	for (it = first; it != _lookup.end() && it->first == fd; it++) {
	    wrapper<ev_io> *e = it->second;
	    stop(e);
	    if (*e->se && e->slot)
		*e->slot = -ECANCELED;
	    e->se->simple_trigger(true);
	    this->wrapper_collection<ev_io>::deactivate(e);
	}
	_lookup.erase(first, it);
    }
	
private:

    void remove_from_lookup(wrapper<ev_io> *e) 
    {
	int fd = e->fd;
	for (lookup_t::iterator it = _lookup.find(fd);
	     it != _lookup.end() && it->first == fd; it++) {
	    if (it->second == e) {
		_lookup.erase(it, ++it);
		break;
	    }
	}
    }

    typedef std::map<int, wrapper<ev_io> *> lookup_t;
    typedef std::pair<int, wrapper<ev_io> *> pair_t;
    lookup_t _lookup;
};

class wrapper_timer_collection : public wrapper_collection<ev_timer> {
public:
    wrapper_timer_collection(driver_libev *d) : wrapper_collection<ev_timer>(d) {}
    ~wrapper_timer_collection() 
    {
	wrapper<ev_timer> *e;
	while ((e = _active.pop_front())) stop(e);
    }

    void empty ()
    {
	wrapper<ev_timer> *e;
	while ((e = _active.front()) && !*e->se) {
	    stop(e);
	    tamerpriv::simple_event::unuse_clean(e->se);
	    deactivate(e);
	}
    }

    void stop(wrapper<ev_timer> *e);
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

    wrapper_io_collection    _ios;
    wrapper_timer_collection _timers;
    wrapper<ev_io>           *_esignal;

private:

};


void wrapper_io_collection::stop(wrapper<ev_io> *e) 
{ ::ev_io_stop(e->driver->_eloop, e); }

void wrapper_timer_collection::stop(wrapper<ev_timer> *e) 
{ ::ev_timer_stop(e->driver->_eloop, e); }

extern "C" {

void libev_io_trigger(struct ev_loop *, ev_io *arg, int) 
{
    wrapper_io *e = static_cast<wrapper_io *>(arg);
    ::ev_io_stop (e->driver->_eloop, arg);
    if (*e->se && e->slot)
	*e->slot = 0;
    e->se->simple_trigger(true);
    e->driver->_ios.deactivate(e);
}

void libev_timer_trigger(struct ev_loop *, ev_timer *arg, int)
{
    wrapper_timer *e = static_cast<wrapper_timer *>(arg);
    ::ev_timer_stop (e->driver->_eloop, arg);
    if (*e->se && e->slot)
	*e->slot = 0;
    e->se->simple_trigger(true);
    e->driver->_timers.deactivate(e);
}

void libev_sigtrigger(struct ev_loop *, ev_io *arg, int)
{
    wrapper<ev_io> *e = static_cast<wrapper<ev_io> *>(arg);
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

    ev_io *tmp = _esignal;

    ev_io_init(tmp, libev_sigtrigger, sig_pipe[0], EV_READ);
    ev_io_start(_eloop, tmp);
    ev_set_priority(tmp, 0);
}

driver_libev::~driver_libev() 
{
    // Stop the special signal FD pipe.
    ::ev_io_stop (_eloop, _esignal);
}

void 
driver_libev::store_fd(int fd, int action,
		       tamerpriv::simple_event *se, int *slot)
{
    assert(fd >= 0);
    if (se) {
	wrapper<ev_io> *e = _ios.get(true);
	e->se = se;
	e->slot = slot;

	action = (action == fdwrite ? EV_WRITE : EV_READ);
	ev_io *tmp = e;
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
	wrapper<ev_timer> *e = _timers.get(true);
	e->se = se;
	e->slot = 0;

	timeval timeout = expiry;
	timersub(&timeout, &now, &timeout);
	ev_tstamp d = timeout.tv_sec;

	ev_timer *tmp = e;
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
