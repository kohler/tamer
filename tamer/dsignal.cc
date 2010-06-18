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
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
namespace tamer {

volatile sig_atomic_t driver::sig_any_active;
int driver::sig_pipe[2] = { -1, -1 };

extern "C" { typedef void (*tamer_sighandler)(int); }
static int tamer_sigaction(int signo, tamer_sighandler handler)
{
    struct sigaction sa;
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    return sigaction(signo, &sa, 0);
}

namespace {

volatile sig_atomic_t sig_active[NSIG];
volatile int sig_installing = -1;
event<> sig_handlers[NSIG];
sigset_t sig_dispatching;

class sigcancel_rendezvous : public rendezvous<> { public:

    sigcancel_rendezvous() {
	sigemptyset(&sig_dispatching);
    }

    inline void add(tamerpriv::simple_event *e) throw () {
	assert(sig_installing >= 0 && sig_installing < NSIG);
	_nwaiting++;
	e->initialize(this, sig_installing);
    }

    void complete(uintptr_t rid) {
	if (!sig_handlers[rid] && !sigismember(&sig_dispatching, rid))
	    tamer_sigaction(rid, SIG_DFL);
	rendezvous<>::complete(rid);
    }

};

sigcancel_rendezvous sigcancelr;

}


extern "C" {
static void tamer_signal_handler(int signo) {
    driver::sig_any_active = sig_active[signo] = 1;
    // ensure select wakes up
    if (driver::sig_pipe[1] >= 0) {
	int save_errno = errno;
	ssize_t r = write(driver::sig_pipe[1], "", 1);
	(void) r;		// don't care if the write fails
	errno = save_errno;
    }
}
}


void driver::at_signal(int signo, const event<> &trigger)
{
    assert(signo >= 0 && signo < NSIG);

    if (sig_pipe[0] < 0 && pipe(sig_pipe) >= 0) {
	fcntl(sig_pipe[0], F_SETFL, O_NONBLOCK);
	fcntl(sig_pipe[1], F_SETFL, O_NONBLOCK);
	fcntl(sig_pipe[0], F_SETFD, FD_CLOEXEC);
	fcntl(sig_pipe[1], F_SETFD, FD_CLOEXEC);
    }

    if (!trigger)		// special case forces creation of signal pipe
	return;

    if (sig_handlers[signo]) {
	tamerpriv::simple_event *simple = sig_handlers[signo].__get_simple();
	sig_handlers[signo] = distribute(sig_handlers[signo], trigger);
	if (simple == sig_handlers[signo].__get_simple())
	    // we already have a canceler
	    return;
    } else {
	sig_handlers[signo] = trigger;
	if (!sigismember(&sig_dispatching, signo))
	    tamer_sigaction(signo, tamer_signal_handler);
    }

    sig_installing = signo;
    sig_handlers[signo].at_trigger(make_event(sigcancelr));
}


void driver::dispatch_signals()
{
    sig_any_active = 0;

    // kill crap data written to pipe
    char crap[64];
    while (read(sig_pipe[0], crap, 64) > 0)
	/* do nothing */;

    // find and block signals that have happened
    for (int signo = 0; signo < NSIG; ++signo)
	if (sig_active[signo])
	    sigaddset(&sig_dispatching, signo);
    sigprocmask(SIG_BLOCK, &sig_dispatching, 0);

    // trigger those signals
    for (int signo = 0; signo < NSIG; ++signo)
	if (sigismember(&sig_dispatching, signo)) {
	    sig_handlers[signo].trigger();
	    sig_active[signo] = 0;
	}

    // run closures activated by signals (plus maybe some others)
    while (tamerpriv::abstract_rendezvous *r = tamerpriv::abstract_rendezvous::unblocked)
	r->run();

    // reset signal handlers if appropriate
    for (int signo = 0; signo < NSIG; ++signo)
	if (sigismember(&sig_dispatching, signo) && !sig_active[signo])
	    tamer_sigaction(signo, SIG_DFL);

    // now that the signal responders have potentially reinstalled signal
    // handlers, unblock the signals
    sigprocmask(SIG_UNBLOCK, &sig_dispatching, 0);
    sigemptyset(&sig_dispatching);
}

}
