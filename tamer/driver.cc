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
      _asap_head(0), _asap_tail(0), _asapcap(16),
      _tcap(0), _tgroup(0), _tfree(0),
      _fdcap(0), _fdgroup(0), _fdfree(0)
{
    expand_timers();
    FD_ZERO(&_readfds);
    FD_ZERO(&_writefds);
    _asap = reinterpret_cast<event<> *>(new unsigned char[sizeof(event<>) * _asapcap]);
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

    // free asap
    for (unsigned a = _asap_head; a != _asap_tail; a++)
	_asap[a & (_asapcap - 1)].~event();
    delete[] reinterpret_cast<unsigned char *>(_asap);
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

    ttimer **tend = _t + _nt;
    while (1) {
	ttimer *smallest = t;
	ttimer **tp = _t + 2*pos + 1;
	if (tp < tend && !timercmp(&tp[0]->expiry, &smallest->expiry, >))
	    smallest = tp[0];
	if (tp + 1 < tend && !timercmp(&tp[1]->expiry, &smallest->expiry, >))
	    smallest = tp[1], tp++;

	smallest->u.schedpos = pos;
	_t[pos] = smallest;

	if (smallest == t)
	    break;

	pos = tp - _t;
    }
#if 0
    if (_t + 1 < tend || !will_delete)
	_timer_expiry = tbegin[0]->expiry;
    else
	_timer_expiry = Timestamp();
#endif
}

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

void driver::expand_asap()
{
    int ncap = _asapcap * 2;
    event<> *nasap = reinterpret_cast<event<> *>(new unsigned char[sizeof(event<>) * ncap]);
    unsigned headoff = (_asap_head & (_asapcap - 1));
    unsigned tailoff = (_asap_tail & (_asapcap - 1));
    if (headoff <= tailoff)
	memcpy(nasap, _asap + headoff, sizeof(event<>) * (_asap_tail - _asap_head));
    else {
	memcpy(nasap, _asap + headoff, sizeof(event<>) * (_asapcap - headoff));
	memcpy(nasap + headoff, _asap, sizeof(event<>) * tailoff);
    }
    _asap_tail = _asap_tail - _asap_head;
    _asap_head = 0;
    delete[] reinterpret_cast<unsigned char *>(_asap);
    _asap = nasap;
    _asapcap = ncap;
}

void driver::at_fd(int fd, bool write, const event<> &trigger)
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
	
	t->fdaction = fd*2 + write;
	(void) new(static_cast<void *>(&t->e)) event<>(trigger);
	
	fd_set *fset = (write ? &_writefds : &_readfds);
	FD_SET(fd, fset);
	if (fd >= _nfds)
	    _nfds = fd + 1;
	
	t->e.at_cancel(make_event(_fdcancelr));
    }
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
    if (_asap_head != _asap_tail
	|| (_nt > 0 && !timercmp(&_t[0]->expiry, &now, >))
	|| sig_any_active) {
	timerclear(&to);
	toptr = &to;
    } else if (_nt == 0)
	toptr = 0;
    else {
#if 0
	for (int i = 0; i < _nt; i++)
	    fprintf(stderr, "%d.%06d ; ", _t[i]->expiry.tv_sec, _t[i]->expiry.tv_usec);
	fprintf(stderr, "(%d.%06d) \n", now.tv_sec, now.tv_usec);
#endif
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
	    if (t->e) {
		fd_set *fset = (t->fdaction & 1 ? &_writefds : &_readfds);
		FD_SET(t->fdaction >> 1, fset);
		pprev = &t->next;
		if ((t->fdaction >> 1) >= _nfds)
		    _nfds = (t->fdaction >> 1) + 1;
	    } else {
		t->e.~event();
		*pprev = t->next;
		t->next = _fdfree;
		_fdfree = t;
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
    while (_asap_head != _asap_tail) {
	_asap[_asap_head & (_asapcap - 1)].trigger();
	_asap[_asap_head & (_asapcap - 1)].~event();
	_asap_head++;
    }

    // run file descriptors
    if (nfds >= 0) {
	tfd **pprev = &_fd, *t;
	while ((t = *pprev)) {
	    fd_set *fset = (t->fdaction & 1 ? &wfds : &rfds);
	    if (FD_ISSET(t->fdaction >> 1, fset)) {
		fset = (t->fdaction & 1 ? &_writefds : &_readfds);
		FD_CLR(t->fdaction >> 1, fset);
		t->e.trigger();
		t->e.~event();
		*pprev = t->next;
		t->next = _fdfree;
		_fdfree = t;
	    } else
		pprev = &t->next;
	}
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
