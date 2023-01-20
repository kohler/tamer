/* Copyright (c) 2007-2015, Eddie Kohler
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
#include <csignal>
#include <cerrno>
#include <cstdio>
#include <fcntl.h>
#include <unistd.h>
namespace tamer {

volatile sig_atomic_t driver::sig_any_active;
int driver::sig_pipe[2] = { -1, -1 };
unsigned driver::sig_nforeground = 0;
unsigned driver::sig_ntotal = 0;

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
class sigcancel_rendezvous : public tamerpriv::functional_rendezvous,
                             public one_argument_rendezvous_tag<sigcancel_rendezvous> { public:
    sigcancel_rendezvous()
        : functional_rendezvous(hook) {
    }
    inline uintptr_t make_rid(int sig) TAMER_NOEXCEPT {
        assert(sig >= 0 && sig < 2*NSIG);
        return sig;
    }
    static void hook(tamerpriv::functional_rendezvous *fr,
                     tamerpriv::simple_event *e, bool values) TAMER_NOEXCEPT;
};

sigcancel_rendezvous sigcancelr;
volatile sig_atomic_t sig_active[NSIG];
event<> sig_handlers[NSIG];
sigset_t sig_dispatching;

void sigcancel_rendezvous::hook(tamerpriv::functional_rendezvous *,
                                tamerpriv::simple_event *e, bool) TAMER_NOEXCEPT {
    uintptr_t rid = e->rid();
    int signo = rid >> 1;
    if (!sig_handlers[signo] && sigismember(&sig_dispatching, signo) == 0)
        tamer_sigaction(signo, SIG_DFL);
    if (rid & 1)
        --driver::sig_nforeground;
    --driver::sig_ntotal;
}
} // namespace


extern "C" {
static void tamer_signal_handler(int signo) {
    driver::sig_any_active = sig_active[signo] = 1;
    // ensure select wakes up
    if (driver::sig_pipe[1] >= 0) {
        int save_errno = errno;
        ssize_t r = write(driver::sig_pipe[1], "", 1);
        (void) r;               // don't care if the write fails
        errno = save_errno;
    }
}
}


void driver::at_signal(int signo, event<> trigger, signal_flags flags)
{
    assert(signo >= 0 && signo < NSIG);

    if (sig_pipe[0] < 0 && pipe(sig_pipe) >= 0) {
        fcntl(sig_pipe[0], F_SETFL, O_NONBLOCK);
        fcntl(sig_pipe[1], F_SETFL, O_NONBLOCK);
        fcntl(sig_pipe[0], F_SETFD, FD_CLOEXEC);
        fcntl(sig_pipe[1], F_SETFD, FD_CLOEXEC);
        sigemptyset(&sig_dispatching);
    }

    if (!trigger)               // special case forces creation of signal pipe
        return;

    bool foreground = (flags & signal_background) == 0;
    trigger.at_trigger(make_event(sigcancelr, (signo << 1) + foreground));
    sig_nforeground += foreground;
    sig_ntotal += 1;

    sig_handlers[signo] += std::move(trigger);
    if (sigismember(&sig_dispatching, signo) == 0)
        tamer_sigaction(signo, tamer_signal_handler);
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
        if (sigismember(&sig_dispatching, signo) > 0) {
            sig_active[signo] = 0;
            sig_handlers[signo].trigger();
        }

    // run closures activated by signals (plus maybe some others)
    run_unblocked();

    // reset signal handlers if appropriate
    for (int signo = 0; signo < NSIG; ++signo)
        if (sigismember(&sig_dispatching, signo) > 0
            && !sig_handlers[signo])
            tamer_sigaction(signo, SIG_DFL);

    // now that the signal responders have potentially reinstalled signal
    // handlers, unblock the signals
    sigprocmask(SIG_UNBLOCK, &sig_dispatching, 0);
    sigemptyset(&sig_dispatching);
}

} // namespace tamer
