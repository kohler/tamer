/* Copyright (c) 2007, Eddie Kohler
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
#include <tamer/tamer.hh>
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

    virtual void at_fd(int fd, int action, const event<int> &e);
    virtual void at_time(const timeval &expiry, const event<> &e);
    virtual void at_asap(const event<> &e);
    virtual void kill_fd(int fd);

    virtual bool empty();
    virtual void once();
    virtual void loop();

  private:

    struct ttimer {
	union {
	    unsigned order;
	    ttimer *next;
	} u;
	timeval expiry;
	event<> trigger;
	ttimer(const timeval &expiry_, unsigned order_, const event<> &trigger_)
	    : expiry(expiry_), trigger(trigger_) {
	    u.order = order_;
	}
	bool operator>(const ttimer &o) const {
	    if (expiry.tv_sec != o.expiry.tv_sec)
		return expiry.tv_sec > o.expiry.tv_sec;
	    if (expiry.tv_usec != o.expiry.tv_usec)
		return expiry.tv_usec > o.expiry.tv_usec;
	    return (int) (u.order - o.u.order) > 0;
	}
    };

    struct ttimer_group {
	ttimer_group *next;
	ttimer t[1];
    };

    struct tfd {
	int fd : 30;
	unsigned action : 2;
	event<int> e;
	tfd *next;
    };

    struct tfd_group {
	tfd_group *next;
	tfd t[1];
    };

    union xfd_set {
	fd_set fds;
	char s[1];
    };

    ttimer **_t;
    int _nt;
    unsigned _torder;

    tfd *_fd;
    int _nfds;
    xfd_set *_fdset[4];
    int _fdset_cap;

    tamerutil::debuffer<event<> > _asap;

    int _tcap;
    ttimer_group *_tgroup;
    int _tgroup_cap;
    ttimer *_tfree;

    int _fdcap;
    tfd_group *_fdgroup;
    tfd *_fdfree;
    rendezvous<> _fdcancelr;

    void expand_timers();
    void check_timers() const;
    void timer_reheapify_from(int pos, ttimer *t, bool will_delete);
    void expand_fds();
    void cull_timers();

};


driver_tamer::driver_tamer()
    : _t(0), _nt(0), _torder(0),
      _fd(0), _nfds(0), _fdset_cap(sizeof(xfd_set) * 8),
      _tcap(0), _tgroup(0), _tgroup_cap(7), _tfree(0),
      _fdcap(0), _fdgroup(0), _fdfree(0)
{
    assert(FD_SETSIZE <= _fdset_cap);
    expand_timers();
    for (int i = 0; i < 4; ++i)
	_fdset[i] = reinterpret_cast<xfd_set *>(new char[_fdset_cap / 8]);
    FD_ZERO(&_fdset[fdread]->fds);
    FD_ZERO(&_fdset[fdwrite]->fds);
    set_now();
}

driver_tamer::~driver_tamer()
{
    // destroy all active timers
    for (int i = 0; i < _nt; i++)
	_t[i]->~ttimer();

    // free timer groups
    while (_tgroup) {
	ttimer_group *next = _tgroup->next;
	delete[] reinterpret_cast<unsigned char *>(_tgroup);
	_tgroup = next;
    }
    delete[] _t;

    // destroy all active file descriptors
    while (_fd) {
	_fd->e.~event();
	_fd = _fd->next;
    }

    // free file descriptor groups
    while (_fdgroup) {
	tfd_group *next = _fdgroup->next;
	delete[] reinterpret_cast<unsigned char *>(_fdgroup);
	_fdgroup = next;
    }

    // free fd_sets
    for (int i = 0; i < 4; ++i)
	delete[] reinterpret_cast<char *>(_fdset[i]);
}

void driver_tamer::expand_timers()
{
    if (_tgroup_cap <= 511)
	_tgroup_cap = (_tgroup_cap * 2) + 1;

    ttimer_group *ngroup = reinterpret_cast<ttimer_group *>(new unsigned char[sizeof(ttimer_group) + sizeof(ttimer) * (_tgroup_cap - 1)]);
    ngroup->next = _tgroup;
    _tgroup = ngroup;
    for (int i = 0; i < _tgroup_cap; i++) {
	ngroup->t[i].u.next = _tfree;
	_tfree = &ngroup->t[i];
    }

    ttimer **t = new ttimer *[_tcap + _tgroup_cap];
    memcpy(t, _t, sizeof(ttimer *) * _nt);
    delete[] _t;
    _t = t;
    _tcap = _tcap + _tgroup_cap;
}

void driver_tamer::timer_reheapify_from(int pos, ttimer *t, bool /*will_delete*/)
{
    int npos;
    while (pos > 0
	   && (npos = (pos-1) >> 1, *_t[npos] > *t)) {
	_t[pos] = _t[npos];
	pos = npos;
    }

    while (1) {
	ttimer *smallest = t;
	npos = 2*pos + 1;
	if (npos < _nt && *smallest > *_t[npos])
	    smallest = _t[npos];
	if (npos+1 < _nt && *smallest > *_t[npos+1])
	    smallest = _t[npos+1], npos++;

	_t[pos] = smallest;

	if (smallest == t)
	    break;

	pos = npos;
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

void driver_tamer::expand_fds()
{
    int ncap = (_fdcap ? _fdcap * 2 : 16);

    tfd_group *ngroup = reinterpret_cast<tfd_group *>(new unsigned char[sizeof(tfd_group) + sizeof(tfd) * (ncap - 1)]);
    ngroup->next = _fdgroup;
    _fdgroup = ngroup;
    for (int i = 0; i < ncap; i++) {
	ngroup->t[i].next = _fdfree;
	_fdfree = &ngroup->t[i];
    }
}

void driver_tamer::at_fd(int fd, int action, const event<int> &trigger)
{
    if (!_fdfree)
	expand_fds();
    if (trigger) {
	tfd *t = _fdfree;
	_fdfree = t->next;
	t->next = _fd;
	_fd = t;

	t->fd = fd;
	t->action = action;
	(void) new(static_cast<void *>(&t->e)) event<int>(trigger);

	if (fd >= FD_SETSIZE) {
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

	FD_SET(fd, &_fdset[action]->fds);
	if (fd >= _nfds)
	    _nfds = fd + 1;
	if (action <= fdwrite)
	    t->e.at_trigger(make_event(_fdcancelr));
    }
}

void driver_tamer::kill_fd(int fd)
{
    assert(fd >= 0);
    FD_CLR(fd, &_fdset[fdread]->fds);
    FD_CLR(fd, &_fdset[fdwrite]->fds);
    tfd **pprev = &_fd, *t;
    while ((t = *pprev))
	if (t->fd == fd) {
	    t->e.trigger(-ECANCELED);
	    t->e.~event();
	    *pprev = t->next;
	    t->next = _fdfree;
	    _fdfree = t;
	} else
	    pprev = &t->next;
}

void driver_tamer::at_time(const timeval &expiry, const event<> &e)
{
    if (!_tfree)
	expand_timers();
    ttimer *t = _tfree;
    _tfree = t->u.next;
    (void) new(static_cast<void *>(t)) ttimer(expiry, ++_torder, e);
    _t[_nt++] = 0;
    timer_reheapify_from(_nt-1, t, false);
}

void driver_tamer::at_asap(const event<> &e)
{
    _asap.push_back(e);
}

void driver_tamer::cull_timers()
{
    ttimer *t;
    while (_nt > 0 && (t = _t[0], !t->trigger)) {
	timer_reheapify_from(0, _t[_nt - 1], true);
	_nt--;
	t->~ttimer();
	t->u.next = _tfree;
	_tfree = t;
    }
}

bool driver_tamer::empty()
{
    cull_timers();
    if (_asap.size()
	|| _nt != 0
	|| sig_any_active
	|| tamerpriv::abstract_rendezvous::unblocked
	|| _nfds != 0)
	return false;
    return true;
}

void driver_tamer::once()
{
    // determine timeout
    cull_timers();
    struct timeval to, *toptr;
    if (_asap.size()
	|| (_nt > 0 && !timercmp(&_t[0]->expiry, &now, >))
	|| sig_any_active
	|| tamerpriv::abstract_rendezvous::unblocked) {
	timerclear(&to);
	toptr = &to;
    } else if (_nt == 0)
	toptr = 0;
    else {
	timersub(&_t[0]->expiry, &now, &to);
	toptr = &to;
    }

    // get rid of canceled descriptors, if any
    if (_fdcancelr.nready()) {
	while (_fdcancelr.join())
	    /* nada */;
	memset(_fdset[fdread]->s, 0, ((_nfds + 63) & ~63) >> 3);
	memset(_fdset[fdwrite]->s, 0, ((_nfds + 63) & ~63) >> 3);
	_nfds = 0;
	tfd **pprev = &_fd, *t;
	while ((t = *pprev))
	    if (!t->e) {
		t->e.~event();
		*pprev = t->next;
		t->next = _fdfree;
		_fdfree = t;
	    } else {
		FD_SET(t->fd, &_fdset[t->action]->fds);
		if (t->fd >= _nfds)
		    _nfds = t->fd + 1;
		pprev = &t->next;
	    }
    }

    // select!
    int nfds = _nfds;
    memcpy(_fdset[fdread + 2], _fdset[fdread], ((nfds + 63) & ~63) >> 3);
    memcpy(_fdset[fdwrite + 2], _fdset[fdwrite], ((nfds + 63) & ~63) >> 3);
    if (sig_pipe[0] >= 0) {
	FD_SET(sig_pipe[0], &_fdset[fdread + 2]->fds);
	if (sig_pipe[0] > nfds)
	    nfds = sig_pipe[0] + 1;
    }
    nfds = select(nfds, &_fdset[fdread + 2]->fds,
		  &_fdset[fdwrite + 2]->fds, 0, toptr);

    // run signals
    if (sig_any_active)
	dispatch_signals();

    // run asaps
    while (event<> *e = _asap.front_ptr()) {
	e->trigger();
	_asap.pop_front();
    }

    // run file descriptors
    if (nfds > 0) {
	tfd **pprev = &_fd, *t;
	while ((t = *pprev))
	    if (t->action <= fdwrite
		&& (t->fd >= FD_SETSIZE
		    || FD_ISSET(t->fd, &_fdset[t->action + 2]->fds))) {
		FD_CLR(t->fd, &_fdset[t->action]->fds);
		t->e.trigger(0);
		t->e.~event();
		_fdcancelr.join(); // reap the notifier we just triggered
		*pprev = t->next;
		t->next = _fdfree;
		_fdfree = t;
	    } else if (!t->e) {
		t->e.~event();
		*pprev = t->next;
		t->next = _fdfree;
		_fdfree = t;
	    } else
		pprev = &t->next;
    }

    // run the timers that worked
    set_now();
    ttimer *t;
    while (_nt > 0 && (t = _t[0], !timercmp(&t->expiry, &now, >))) {
	timer_reheapify_from(0, _t[_nt - 1], true);
	_nt--;
	t->trigger.trigger();
	t->~ttimer();
	t->u.next = _tfree;
	_tfree = t;
    }

    // run active closures
    while (tamerpriv::abstract_rendezvous *r = tamerpriv::abstract_rendezvous::unblocked)
	r->run();
}

#if 0
void driver_tamer::print_fds()
{
    tfd *t = _fd;
    while (t) {
	fprintf(stderr, "%d.%d ", t->fd, t->action);
	t = t->next;
    }
    if (_fd)
	fprintf(stderr, "\n");
}
#endif

void driver_tamer::loop()
{
    while (1)
	once();
}

}

driver *driver::make_tamer()
{
    return new driver_tamer;
}

}
