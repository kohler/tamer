#include <tamer/driver.hh>
#include <tamer/adapter.hh>
#include <sys/select.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>

namespace tamer {

driver driver::main;

// using the "self-pipe trick" recommended by select(3) and sfslite.
namespace {

#define NSIGNALS 32
int sig_pipe[2] = { -1, -1 };
volatile unsigned sig_any_active;
volatile unsigned sig_active[NSIGNALS];
volatile int sig_installing = -1;
event<> sig_handlers[NSIGNALS];

class sigcancel_rendezvous : public rendezvous<> { public:

    sigcancel_rendezvous() {
    }

    inline void add(tamerpriv::simple_event *e) throw () {
	_nwaiting++;
	e->initialize(this, sig_installing);
    }
    
    void complete(uintptr_t rname, bool success) {
	if ((int) rname != sig_installing && success) {
	    struct sigaction sa;
	    sa.sa_handler = SIG_DFL;
	    sigemptyset(&sa.sa_mask);
	    sa.sa_flags = SA_RESETHAND;
	    sigaction(rname, &sa, 0);
	}
	rendezvous<>::complete(rname, success);
    }
    
};

sigcancel_rendezvous sigcancelr;

}


driver::driver()
    : _t(0), _nt(0),
      _fd(0), _nfds(0),
      _tcap(0), _tgroup(0), _tfree(0),
      _fdcap(0), _fdgroup(0), _fdfree(0)
{
    expand_timers();
    FD_ZERO(&_readfds);
    FD_ZERO(&_writefds);
    set_now();
}

driver::~driver()
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

void driver::expand_timers()
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

void driver::timer_reheapify_from(int pos, ttimer *t, bool /*will_delete*/)
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
void driver::check_timers() const
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

void driver::expand_fds()
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

void driver::at_fd(int fd, int action, const event<> &trigger)
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

	if (action <= fdwrite) {
	    fd_set *fset = (action == fdwrite ? &_writefds : &_readfds);
	    FD_SET(fd, fset);
	    if (fd >= _nfds)
		_nfds = fd + 1;
	    t->e.at_cancel(make_event(_fdcancelr));
	}
    }
}

void driver::kill_fd(int fd)
{
    assert(fd >= 0);
    if (fd < _nfds) {
	FD_CLR(fd, &_readfds);
	FD_CLR(fd, &_writefds);
    }
    tfd **pprev = &_fd, *t;
    while ((t = *pprev))
	if (t->fd == fd) {
	    if (t->action == fdclose)
		t->e.trigger();
	    t->e.~event();
	    *pprev = t->next;
	    t->next = _fdfree;
	    _fdfree = t;
	} else
	    pprev = &t->next;
}

void driver::at_delay(double delay, const event<> &e)
{
    if (delay <= 0)
	at_asap(e);
    else {
	timeval tv = now;
	long ldelay = (long) delay;
	tv.tv_sec += ldelay;
	tv.tv_usec += (long) ((delay - ldelay) * 1000000 + 0.5);
	if (tv.tv_usec >= 1000000) {
	    tv.tv_sec++;
	    tv.tv_usec -= 1000000;
	}
	at_time(tv, e);
    }
}

extern "C" {
static void tame_signal_handler(int signal) {
    sig_any_active = sig_active[signal] = 1;
    // ensure select wakes up, even if we get a signal in between setting the
    // timeout and calling select!
    write(sig_pipe[1], "", 1);

    // block signal until we trigger the event, giving the unblocked
    // rendezvous a chance to maybe install another handler
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, signal);
    sigprocmask(SIG_BLOCK, &sigset, NULL);
}
}

void driver::at_signal(int signal, const event<> &trigger)
{
    assert(signal < NSIGNALS);

    if (!trigger)
	return;
    
    if (sig_pipe[0] < 0) {
	pipe(sig_pipe);
	fcntl(sig_pipe[0], F_SETFL, O_NONBLOCK);
	fcntl(sig_pipe[0], F_SETFD, FD_CLOEXEC);
	fcntl(sig_pipe[1], F_SETFD, FD_CLOEXEC);
    }

    if (sig_handlers[signal])
	sig_handlers[signal] = distribute(sig_handlers[signal], trigger);
    else {
	sig_installing = signal;
    
	sig_handlers[signal] = trigger;
	sig_handlers[signal].at_cancel(make_event(sigcancelr));
    
	struct sigaction sa;
	sa.sa_handler = tame_signal_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESETHAND;
	sigaction(signal, &sa, 0);

	sig_installing = -1;
    }
}

bool driver::empty() const
{
    if (_nt != 0 || _nfds != 0 || _asap.size() != 0
	|| sig_any_active || tamerpriv::abstract_rendezvous::unblocked)
	return false;
    for (int i = 0; i < NSIGNALS; i++)
	if (sig_handlers[i])
	    return false;
    return true;
}

void driver::once()
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
	FD_ZERO(&_readfds);
	FD_ZERO(&_writefds);
	_nfds = 0;
	tfd **pprev = &_fd, *t;
	while ((t = *pprev))
	    if (!t->e) {
		t->e.~event();
		*pprev = t->next;
		t->next = _fdfree;
		_fdfree = t;
	    } else if (t->action <= fdwrite) {
		fd_set *fset = (t->action == fdwrite ? &_writefds : &_readfds);
		FD_SET(t->fd, fset);
		pprev = &t->next;
		if (t->fd >= _nfds)
		    _nfds = t->fd + 1;
	    }
    }
    
    // select!
    fd_set rfds = _readfds;
    fd_set wfds = _writefds;
    int nfds = _nfds;
    if (sig_pipe[0] >= 0) {
	FD_SET(sig_pipe[0], &rfds);
	if (sig_pipe[0] > nfds)
	    nfds = sig_pipe[0] + 1;
    }
    nfds = select(nfds, &rfds, &wfds, 0, toptr);

    // run signals
    if (sig_any_active) {
	sig_any_active = 0;
	sigset_t sigs_unblock;
	sigemptyset(&sigs_unblock);

	// check signals
	for (int sig = 0; sig < NSIGNALS; sig++)
	    if (sig_active[sig]) {
		sig_active[sig] = 0;
		sig_handlers[sig].trigger();
		sigaddset(&sigs_unblock, sig);
	    }

	// run closures activated by signals (plus maybe some others)
	while (tamerpriv::abstract_rendezvous *r = tamerpriv::abstract_rendezvous::unblocked)
	    r->run();

	// now that the signal responders have potentially reinstalled signal
	// handlers, unblock the signals
	sigprocmask(SIG_UNBLOCK, &sigs_unblock, 0);

	// kill crap data written to pipe
	char crap[64];
	while (read(sig_pipe[0], crap, 64) > 0)
	    /* do nothing */;
    }

    // run asaps
    while (event<> *e = _asap.front()) {
	e->trigger();
	_asap.pop_front();
    }

    // run file descriptors
    if (nfds >= 0) {
	tfd **pprev = &_fd, *t;
	while ((t = *pprev))
	    if ((t->action == fdread && FD_ISSET(t->fd, &rfds))
		|| (t->action == fdwrite && FD_ISSET(t->fd, &wfds))) {
		fd_set *fset = (t->action == fdwrite ? &_writefds : &_readfds);
		FD_CLR(t->fd, fset);
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

void driver::loop()
{
    while (1)
	once();
}

}
