// -*- mode: c++; tab-width: 8; c-basic-offset: 4;  -*-
/* Copyright (c) 2012-2013, Maxwell Krohn
 * Copyright (c) 2012-2015, Eddie Kohler
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
#include <stdio.h>
#include <string.h>
#if HAVE_LIBEV
#include "dinternal.hh"
#include <ev.h>
#endif

namespace tamer {
#if HAVE_LIBEV
namespace {
using tamerpriv::make_fd_callback;
using tamerpriv::fd_callback_driver;
using tamerpriv::fd_callback_fd;

extern "C" {
typedef void (*ev_watcher_type)(struct ev_loop *, ev_watcher *, int);
void libev_io_trigger(struct ev_loop *, ev_io *ev, int revents);
}

class driver_libev : public driver {
public:
    driver_libev();
    ~driver_libev();

    virtual void at_fd(int fd, int action, event<int> e);
    virtual void at_time(const timeval &expiry, event<> e, bool bg);
    virtual void at_asap(event<> e);
    virtual void at_preblock(event<> e);
    virtual void kill_fd(int fd);

    virtual void loop(loop_flags flags);
    virtual void break_loop();

    struct fdp {
        union {
            ev_watcher w;
            ev_io io;
        } base_;

        inline fdp(driver_libev *d, int fd) {
            ev_init(&base_.w, (ev_watcher_type) libev_io_trigger);
            ev_io_set(&base_.io, fd, 0);
            base_.io.data = d;
        }
    };

    struct ev_loop *eloop_;

    tamerpriv::driver_fdset<fdp> fds_;
    int fdactive_;

    tamerpriv::driver_timerset timers_;

    tamerpriv::driver_asapset asap_;
    tamerpriv::driver_asapset preblock_;

    union {
        ev_watcher w;
        ev_io io;
    } sigwatcher_;

    void update_fds();
    static void fd_disinterest(void* arg);

    friend class tfd;

};


extern "C" {
void libev_io_trigger(struct ev_loop *, ev_io *ev, int revents)
{
    driver_libev *d = static_cast<driver_libev *>(ev->data);
    tamerpriv::driver_fd<driver_libev::fdp> &x = d->fds_[ev->fd];
    if (revents & EV_READ)
        x.e[0].trigger(0);
    if (revents & EV_WRITE)
        x.e[1].trigger(0);
    d->fds_.push_change(ev->fd);
}

void libev_timer_trigger(struct ev_loop *, ev_timer *, int) {
}

void libev_sigtrigger(struct ev_loop *, ev_io *ev, int)
{
    driver_libev *d = static_cast<driver_libev *>(ev->data);
    d->dispatch_signals();
}
} // extern "C"


driver_libev::driver_libev()
    : eloop_(::ev_default_loop(0)), fdactive_(0) {
    at_signal(0, event<>());    // create signal_fd pipe

    ev_init(&sigwatcher_.w, (ev_watcher_type) libev_sigtrigger);
    ev_io_set(&sigwatcher_.io, sig_pipe[0], EV_READ);
    sigwatcher_.io.data = this;
    ev_io_start(eloop_, &sigwatcher_.io);
}

driver_libev::~driver_libev() {
    // Stop the special signal FD pipe.
    ev_io_stop(eloop_, &sigwatcher_.io);
    for (int fd = 0; fd < fds_.size(); ++fd)
        ev_io_stop(eloop_, &fds_[fd].base_.io);
}

void driver_libev::fd_disinterest(void* arg) {
    driver_libev* d = static_cast<driver_libev*>(fd_callback_driver(arg));
    d->fds_.push_change(fd_callback_fd(arg));
}

void driver_libev::at_fd(int fd, int action, event<int> e) {
    assert(fd >= 0);
    if (e && (action == 0 || action == 1)) {
        fds_.expand(this, fd);
        tamerpriv::driver_fd<fdp>& x = fds_[fd];
        x.e[action] += TAMER_MOVE(e);
        tamerpriv::simple_event::at_trigger(x.e[action].__get_simple(),
                                            fd_disinterest,
                                            make_fd_callback(this, fd));
        fds_.push_change(fd);
    }
}

void driver_libev::kill_fd(int fd) {
    if (fd >= 0 && fd < fds_.size()) {
        tamerpriv::driver_fd<fdp> &x = fds_[fd];
        for (int action = 0; action < 2; ++action)
            x.e[action].trigger(-ECANCELED);
        fds_.push_change(fd);
    }
}

void driver_libev::update_fds() {
    int fd;
    while ((fd = fds_.pop_change()) >= 0) {
        tamerpriv::driver_fd<fdp> &x = fds_[fd];
        unsigned want_what = (x.e[0] ? (int) EV_READ : 0) | (x.e[1] ? (int) EV_WRITE : 0);
        unsigned have_what = (ev_is_active(&x.base_.w) ? x.base_.io.events : 0) & (EV_READ | EV_WRITE);
        if (want_what != have_what) {
            fdactive_ += (want_what != 0) - (have_what != 0);
            if (have_what != 0)
                ev_io_stop(eloop_, &x.base_.io);
            if (want_what && (x.base_.io.events & (EV_READ | EV_WRITE)) != want_what)
                ev_io_set(&x.base_.io, fd, want_what);
            if (want_what != 0)
                ev_io_start(eloop_, &x.base_.io);
        }
    }
}

void driver_libev::at_time(const timeval &expiry, event<> e, bool bg) {
    if (e)
        timers_.push(expiry, e.__release_simple(), bg);
}

void driver_libev::at_asap(event<> e) {
    if (e)
        asap_.push(e.__release_simple());
}

void driver_libev::at_preblock(event<> e) {
    if (e)
        preblock_.push(e.__release_simple());
}

void driver_libev::loop(loop_flags flags) {
    union {
        ev_watcher w;
        ev_periodic p;
    } timerev;
    bool timer_set = false;

 again:
    // process preblock events
    while (!preblock_.empty())
        preblock_.pop_trigger();
    run_unblocked();

    // fix file descriptors
    if (fds_.has_change())
        update_fds();

    int event_flags = EVRUN_ONCE;
    timers_.cull();
    if (!asap_.empty()
        || (!timers_.empty() && !timercmp(&timers_.expiry(), &recent(), >))
        || sig_any_active
        || has_unblocked())
        event_flags |= EVRUN_NOWAIT;
    else if (!timers_.has_foreground()
             && fdactive_ == 0
             && sig_nforeground == 0)
        // no foreground events!
        return;
    else if (!timers_.empty()) {
        if (!timer_set) {
            ev_init(&timerev.w, (ev_watcher_type) libev_timer_trigger);
            ev_periodic_set(&timerev.p, 0, 0, 0);
            timer_set = true;
        }
        timeval to;
        timersub(&timers_.expiry(), &recent(), &to);
        timerev.p.offset = dtime(to);
        ev_periodic_again(eloop_, &timerev.p);
    } else if (timer_set)
        ev_periodic_stop(eloop_, &timerev.p);

    // run the event loop, unless there's nothing it can do
    if (!(event_flags & EVRUN_NOWAIT) || fdactive_ != 0 || sig_ntotal != 0)
        ::ev_run(eloop_, event_flags);

    // process fd events
    set_recent();
    run_unblocked();

    // process timer events
    while (!timers_.empty() && !timercmp(&timers_.expiry(), &recent(), >))
        timers_.pop_trigger();
    run_unblocked();

    // process asap events
    while (!asap_.empty())
        asap_.pop_trigger();
    run_unblocked();

    if (flags == loop_forever)
        goto again;
}

void driver_libev::break_loop() {
    ev_break(eloop_);
}

} // namespace
#endif

driver *driver::make_libev()
{
#if HAVE_LIBEV
    return new driver_libev;
#else
    return 0;
#endif
}

} // namespace tamer
