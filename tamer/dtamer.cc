#include <tamer/tamer.hh>
#include <sys/select.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

namespace tamer {
namespace {

class driver_tamer : public driver { public:

    driver_tamer();
    ~driver_tamer();

    virtual void at_fd(int fd, int action, const event<> &e);
    virtual void at_time(const timeval &expiry, const event<> &e);
    virtual void at_asap(const event<> &e);
    virtual void kill_fd(int fd);

    virtual bool empty();
    virtual void once();
    virtual void loop();
    
  private:
    
    struct ttimer {
	union {
	    int schedpos;
	    ttimer *next;
	} u;
	timeval expiry;
	event<> trigger;
	ttimer(int schedpos_, const timeval &expiry_, const event<> &trigger_) : expiry(expiry_), trigger(trigger_) { u.schedpos = schedpos_; }
    };

    struct ttimer_group {
	ttimer_group *next;
	ttimer t[1];
    };

    struct tfd {
	int fd : 30;
	unsigned action : 2;
	event<> e;
	tfd *next;
    };

    struct tfd_group {
	tfd_group *next;
	tfd t[1];
    };
    
    ttimer **_t;
    int _nt;

    tfd *_fd;
    int _nfds;
    fd_set _fdset[2];

    tamerpriv::debuffer<event<> > _asap;

    int _tcap;
    ttimer_group *_tgroup;
    ttimer *_tfree;

    int _fdcap;
    tfd_group *_fdgroup;
    tfd *_fdfree;
    rendezvous<> _fdcancelr;

    void expand_timers();
    void check_timers() const;
    void timer_reheapify_from(int pos, ttimer *t, bool will_delete);
    void expand_fds();
    
};


driver_tamer::driver_tamer()
    : _t(0), _nt(0),
      _fd(0), _nfds(0),
      _tcap(0), _tgroup(0), _tfree(0),
      _fdcap(0), _fdgroup(0), _fdfree(0)
{
    expand_timers();
    FD_ZERO(&_fdset[fdread]);
    FD_ZERO(&_fdset[fdwrite]);
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
}

void driver_tamer::expand_timers()
{
    int ncap = (_tcap ? _tcap * 2 : 16);

    ttimer_group *ngroup = reinterpret_cast<ttimer_group *>(new unsigned char[sizeof(ttimer_group) + sizeof(ttimer) * (ncap - 1)]);
    ngroup->next = _tgroup;
    _tgroup = ngroup;
    for (int i = 0; i < ncap; i++) {
	ngroup->t[i].u.next = _tfree;
	_tfree = &ngroup->t[i];
    }

    ttimer **t = new ttimer *[ncap];
    memcpy(t, _t, sizeof(ttimer *) * _nt);
    delete[] _t;
    _t = t;
    _tcap = ncap;
}

void driver_tamer::timer_reheapify_from(int pos, ttimer *t, bool /*will_delete*/)
{
    int npos;
    while (pos > 0
	   && (npos = (pos-1) >> 1, timercmp(&_t[npos]->expiry, &t->expiry, >))) {
	_t[pos] = _t[npos];
	_t[npos]->u.schedpos = pos;
	pos = npos;
    }

    while (1) {
	ttimer *smallest = t;
	npos = 2*pos + 1;
	if (npos < _nt && !timercmp(&_t[npos]->expiry, &smallest->expiry, >))
	    smallest = _t[npos];
	if (npos + 1 < _nt && !timercmp(&_t[npos+1]->expiry, &smallest->expiry, >))
	    smallest = _t[npos+1], npos++;

	smallest->u.schedpos = pos;
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
	    if (j < _nt && timercmp(&_t[i]->expiry, &_t[j]->expiry, >)) {
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

void driver_tamer::at_fd(int fd, int action, const event<> &trigger)
{
    if (!_fdfree)
	expand_fds();
    if (fd >= FD_SETSIZE)
	throw tamer::tamer_error("file descriptor too large");
    if (trigger) {
	tfd *t = _fdfree;
	_fdfree = t->next;
	t->next = _fd;
	_fd = t;
	
	t->fd = fd;
	t->action = action;
	(void) new(static_cast<void *>(&t->e)) event<>(trigger);

	FD_SET(fd, &_fdset[action]);
	if (fd >= _nfds)
	    _nfds = fd + 1;
	if (action <= fdwrite)
	    t->e.at_cancel(make_event(_fdcancelr));
    }
}

void driver_tamer::kill_fd(int fd)
{
    assert(fd >= 0);
    if (fd < _nfds && (FD_ISSET(fd, &_fdset[fdread]) || FD_ISSET(fd, &_fdset[fdwrite]))) {
	FD_CLR(fd, &_fdset[fdread]);
	FD_CLR(fd, &_fdset[fdwrite]);
	tfd **pprev = &_fd, *t;
	while ((t = *pprev))
	    if (t->fd == fd) {
		t->e.~event();
		*pprev = t->next;
		t->next = _fdfree;
		_fdfree = t;
	    } else
		pprev = &t->next;
    }
}

void driver_tamer::at_time(const timeval &expiry, const event<> &e)
{
    if (!_tfree)
	expand_timers();
    ttimer *t = _tfree;
    _tfree = t->u.next;
    (void) new(static_cast<void *>(t)) ttimer(_nt, expiry, e);
    _t[_nt++] = 0;
    timer_reheapify_from(_nt - 1, t, false);
}

void driver_tamer::at_asap(const event<> &e)
{
    _asap.push_back(e);
}

bool driver_tamer::empty()
{
    if (_nt != 0 || _nfds != 0 || _asap.size() != 0
	|| sig_any_active || tamerpriv::abstract_rendezvous::unblocked)
	return false;
    return true;
}

void driver_tamer::once()
{
    // get rid of initial cancelled timers
    ttimer *t;
    while (_nt > 0 && (t = _t[0], !t->trigger)) {
	timer_reheapify_from(0, _t[_nt - 1], true);
	_nt--;
	t->~ttimer();
	t->u.next = _tfree;
	_tfree = t;
    }

    // determine timeout
    struct timeval to, *toptr;
    if (_asap.size()
	|| (_nt > 0 && !timercmp(&_t[0]->expiry, &now, >))
	|| sig_any_active) {
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
	FD_ZERO(&_fdset[fdread]);
	FD_ZERO(&_fdset[fdwrite]);
	_nfds = 0;
	tfd **pprev = &_fd, *t;
	while ((t = *pprev))
	    if (!t->e) {
		t->e.~event();
		*pprev = t->next;
		t->next = _fdfree;
		_fdfree = t;
	    } else {
		FD_SET(t->fd, &_fdset[t->action]);
		if (t->fd >= _nfds)
		    _nfds = t->fd + 1;
		pprev = &t->next;
	    }
    }
    
    // select!
    fd_set fds[2];
    fds[fdread] = _fdset[fdread];
    fds[fdwrite] = _fdset[fdwrite];
    int nfds = _nfds;
    if (sig_pipe[0] >= 0) {
	FD_SET(sig_pipe[0], &fds[fdread]);
	if (sig_pipe[0] > nfds)
	    nfds = sig_pipe[0] + 1;
    }
    nfds = select(nfds, &fds[fdread], &fds[fdwrite], 0, toptr);

    // run signals
    if (sig_any_active)
	dispatch_signals();

    // run asaps
    while (event<> *e = _asap.front()) {
	e->trigger();
	_asap.pop_front();
    }

    // run file descriptors
    if (nfds > 0) {
	tfd **pprev = &_fd, *t;
	while ((t = *pprev))
	    if (t->action <= fdwrite && FD_ISSET(t->fd, &fds[t->action])) {
		FD_CLR(t->fd, &_fdset[t->action]);
		t->e.trigger();
		t->e.~event();
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