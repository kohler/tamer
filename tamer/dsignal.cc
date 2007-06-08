#include <tamer/tamer.hh>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
namespace tamer {

volatile sig_atomic_t driver::sig_any_active;
int driver::sig_pipe[2] = { -1, -1 };

namespace {

volatile sig_atomic_t sig_active[NSIG];
volatile int sig_installing = -1;
event<> sig_handlers[NSIG];

class sigcancel_rendezvous : public rendezvous<> { public:

    sigcancel_rendezvous() {
    }

    inline void add(tamerpriv::simple_event *e) throw () {
	_nwaiting++;
	e->initialize(this, sig_installing);
    }
    
    void complete(uintptr_t rid) {
	if ((int) rid != sig_installing) {
	    struct sigaction sa;
	    sa.sa_handler = SIG_DFL;
	    sigemptyset(&sa.sa_mask);
	    sa.sa_flags = SA_RESETHAND;
	    sigaction(rid, &sa, 0);
	}
	rendezvous<>::complete(rid);
    }
    
};

sigcancel_rendezvous sigcancelr;

}


extern "C" {
static void tame_signal_handler(int signal) {
    int save_errno = errno;
    
    driver::sig_any_active = sig_active[signal] = 1;
    
    // ensure select wakes up
    write(driver::sig_pipe[1], "", 1);

    // block signal until we trigger the event, giving the unblocked
    // rendezvous a chance to maybe install a different handler
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, signal);
    sigprocmask(SIG_BLOCK, &sigset, NULL);

    errno = save_errno;
}
}


void driver::at_signal(int signal, const event<> &trigger)
{
    assert(signal >= 0 && signal < NSIG);

    if (sig_pipe[0] < 0) {
	pipe(sig_pipe);
	fcntl(sig_pipe[0], F_SETFL, O_NONBLOCK);
	fcntl(sig_pipe[0], F_SETFD, FD_CLOEXEC);
	fcntl(sig_pipe[1], F_SETFD, FD_CLOEXEC);
    }

    if (!trigger)		// special case forces creation of signal pipe
	return;

    if (sig_handlers[signal])
	sig_handlers[signal] = distribute(sig_handlers[signal], trigger);
    else {
	int old_sig_installing = sig_installing;
	sig_installing = signal;
    
	sig_handlers[signal] = trigger;
	sig_handlers[signal].at_trigger(make_event(sigcancelr));
	
	struct sigaction sa;
	sa.sa_handler = tame_signal_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESETHAND;
	sigaction(signal, &sa, 0);
	
	sig_installing = old_sig_installing;
    }
}


void driver::dispatch_signals()
{
    sig_any_active = 0;
    sigset_t sigs_unblock;
    sigemptyset(&sigs_unblock);

    // kill crap data written to pipe
    char crap[64];
    while (read(sig_pipe[0], crap, 64) > 0)
	/* do nothing */;

    // check signals
    for (int sig = 0; sig < NSIG; sig++)
	if (sig_active[sig]) {
	    sig_active[sig] = 0;
	    sig_installing = sig;
	    sig_handlers[sig].trigger();
	    sig_installing = -1;
	    sigaddset(&sigs_unblock, sig);
	}

    // run closures activated by signals (plus maybe some others)
    while (tamerpriv::abstract_rendezvous *r = tamerpriv::abstract_rendezvous::unblocked)
	r->run();

    // now that the signal responders have potentially reinstalled signal
    // handlers, unblock the signals
    sigprocmask(SIG_UNBLOCK, &sigs_unblock, 0);
}

}
