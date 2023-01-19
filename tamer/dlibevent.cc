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
#include "dinternal.hh"
#if HAVE_LIBEVENT
#include <event.h>
#endif

namespace tamer {
#if HAVE_LIBEVENT
namespace {
using tamerpriv::make_fd_callback;
using tamerpriv::fd_callback_driver;
using tamerpriv::fd_callback_fd;

extern "C" {
void libevent_fdtrigger(int fd, short, void *arg);
}

class driver_libevent : public driver {
  public:
    driver_libevent();
    ~driver_libevent();

    virtual void at_fd(int fd, int action, event<int> e);
    virtual void at_time(const timeval &expiry, event<> e, bool bg);
    virtual void at_asap(event<> e);
    virtual void at_preblock(event<> e);
    virtual void kill_fd(int fd);

    virtual void loop(loop_flags flags);
    virtual void break_loop();

    struct fdp {
        ::event base;

        inline fdp(driver_libevent *d, int fd) {
            ::event_set(&base, fd, 0, libevent_fdtrigger, d);
        }
        inline ~fdp() {
            ::event_del(&base);
        }
    };

    tamerpriv::driver_fdset<fdp> fds_;
    int fdactive_;
    ::event signal_base_;

    tamerpriv::driver_timerset timers_;

    tamerpriv::driver_asapset asap_;
    tamerpriv::driver_asapset preblock_;

    void expand_events();
    static void fd_disinterest(void* arg);
    void update_fds();
};


extern "C" {
void libevent_fdtrigger(int fd, short what, void *arg) {
    driver_libevent *d = static_cast<driver_libevent *>(arg);
    tamerpriv::driver_fd<driver_libevent::fdp> &x = d->fds_[fd];
    if (what & EV_READ) {
        x.e[0].trigger(0);
    }
    if (what & EV_WRITE) {
        x.e[1].trigger(0);
    }
    d->fds_.push_change(fd);
}

void libevent_timertrigger(int, short, void *) {
}

void libevent_sigtrigger(int, short, void *arg) {
    driver_libevent *d = static_cast<driver_libevent *>(arg);
    d->dispatch_signals();
}
} // extern "C"


driver_libevent::driver_libevent()
    : fdactive_(0)
{
    ::event_init();
    ::event_priority_init(3);
#if HAVE_EVENT_GET_STRUCT_EVENT_SIZE
    assert(sizeof(::event) >= event_get_struct_event_size());
#endif
    at_signal(0, event<>());    // create signal_fd pipe
    ::event_set(&signal_base_, sig_pipe[0], EV_READ | EV_PERSIST,
                libevent_sigtrigger, this);
    ::event_priority_set(&signal_base_, 0);
    ::event_add(&signal_base_, 0);
}

driver_libevent::~driver_libevent() {
    ::event_del(&signal_base_);
}

void driver_libevent::fd_disinterest(void* arg) {
    driver_libevent* d = static_cast<driver_libevent*>(fd_callback_driver(arg));
    d->fds_.push_change(fd_callback_fd(arg));
}

void driver_libevent::at_fd(int fd, int action, event<int> e) {
    assert(fd >= 0);
    if (e && (unsigned) action < nfdactions) {
        fds_.expand(this, fd);
        tamerpriv::driver_fd<fdp>& x = fds_[fd];
        x.e[action] += TAMER_MOVE(e);
        tamerpriv::simple_event::at_trigger(x.e[action].__get_simple(),
                                            fd_disinterest,
                                            make_fd_callback(this, fd));
        fds_.push_change(fd);
    }
}

void driver_libevent::kill_fd(int fd) {
    if (fd >= 0 && fd < fds_.size()) {
        tamerpriv::driver_fd<fdp> &x = fds_[fd];
        for (int action = 0; action < nfdactions; ++action) {
            x.e[action].trigger(-ECANCELED);
        }
        fds_.push_change(fd);
    }
}

void driver_libevent::update_fds() {
    int fd;
    while ((fd = fds_.pop_change()) >= 0) {
        tamerpriv::driver_fd<fdp> &x = fds_[fd];
        int want_what = (x.e[0] ? EV_READ : 0) | (x.e[1] ? EV_WRITE : 0);
        int have_what = ::event_pending(&x.base, EV_READ | EV_WRITE, 0)
            & (EV_READ | EV_WRITE);
        if (want_what != have_what) {
            fdactive_ += (want_what != 0) - (have_what != 0);
            if (have_what != 0) {
                ::event_del(&x.base);
            }
            if (want_what != 0) {
                ::event_set(&x.base, fd, want_what | EV_PERSIST,
                            libevent_fdtrigger, this);
                ::event_add(&x.base, 0);
            }
        }
    }
}

void driver_libevent::at_time(const timeval &expiry, event<> e, bool bg) {
    if (e) {
        timers_.push(expiry, e.__release_simple(), bg);
    }
}

void driver_libevent::at_asap(event<> e) {
    if (e) {
        asap_.push(e.__release_simple());
    }
}

void driver_libevent::at_preblock(event<> e) {
    if (e) {
        preblock_.push(e.__release_simple());
    }
}

void driver_libevent::loop(loop_flags flags)
{
    ::event timerev;
    bool timer_set = false;

 again:
    // process preblock events
    while (!preblock_.empty()) {
        preblock_.pop_trigger();
    }
    run_unblocked();

    // fix file descriptors
    if (fds_.has_change()) {
        update_fds();
    }

    int event_flags = EVLOOP_ONCE;
    timers_.cull();
    if (!asap_.empty()
        || (!timers_.empty() && !timercmp(&timers_.expiry(), &recent(), >))
        || sig_any_active
        || has_unblocked()) {
        event_flags |= EVLOOP_NONBLOCK;
    } else if (!timers_.has_foreground()
               && fdactive_ == 0
               && sig_nforeground == 0) {
        // no foreground events!
        return;
    } else if (!timers_.empty()) {
        if (!timer_set) {
            evtimer_set(&timerev, libevent_timertrigger, 0);
        }
        timer_set = true;
        timeval timeout = timers_.expiry();
        timersub(&timeout, &recent(), &timeout);
        evtimer_add(&timerev, &timeout);
    }

    // don't bother to run event loop if there is nothing it can do
    if (!(event_flags & EVLOOP_NONBLOCK) || fdactive_ != 0 || sig_ntotal != 0) {
        ::event_loop(event_flags);
    }

    // process fd events
    set_recent();
    run_unblocked();

    // process timer events
    if (!timers_.empty()) {
        if (!(event_flags & EVLOOP_NONBLOCK)) {
            evtimer_del(&timerev);
        }
        while (!timers_.empty() && !timercmp(&timers_.expiry(), &recent(), >)) {
            timers_.pop_trigger();
        }
        run_unblocked();
    }

    // process asap events
    while (!asap_.empty()) {
        asap_.pop_trigger();
    }
    run_unblocked();

    if (flags == loop_forever) {
        goto again;
    }
}

void driver_libevent::break_loop() {
    event_loopbreak();
}

} // namespace
#endif

driver *driver::make_libevent()
{
#if HAVE_LIBEVENT
    return new driver_libevent;
#else
    return 0;
#endif
}

} // namespace tamer
