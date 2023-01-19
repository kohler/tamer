/* Copyright (c) 2007-2023, Eddie Kohler
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
#include <sys/select.h>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#if HAVE_SYS_EPOLL_H && !TAMER_NOEPOLL
# define DTAMER_EPOLL 1
# include <sys/epoll.h>
# ifndef EPOLLRDHUP
#  define EPOLLRDHUP 0
# endif
#else
# define DTAMER_EPOLL 0
#endif
#ifndef POLLRDHUP
# define POLLRDHUP 0
#endif

namespace tamer {
namespace {
using tamerpriv::make_fd_callback;
using tamerpriv::fd_callback_driver;
using tamerpriv::fd_callback_fd;

class xpollfds {
public:
    xpollfds();
    ~xpollfds();

    inline int events(int fd) const;
    void set_events(int fd, int events);

    pollfd* pollfds()                 { return pfds_; }
    const pollfd* pollfds() const     { return pfds_; }
    unsigned size() const             { return npfds_; }

    pollfd* begin()                   { return pfds_; }
    const pollfd* begin() const       { return pfds_; }
    const pollfd* cbegin()            { return pfds_; }
    pollfd* end()                     { return pfds_ + npfds_; }
    const pollfd* end() const         { return pfds_ + npfds_; }
    const pollfd* cend()              { return pfds_ + npfds_; }

private:
    pollfd* pfds_;
    unsigned* fdmap_;
    unsigned npfds_ = 0;
    unsigned pfdcap_ = npfdspace;
    unsigned nfdmap_ = nfdmapspace;
    static constexpr unsigned npfdspace = 64;
    static constexpr unsigned nfdmapspace = 256;
    pollfd pfdspace_[npfdspace];
    unsigned fdmapspace_[nfdmapspace];
};

xpollfds::xpollfds()
    : pfds_(pfdspace_), fdmap_(fdmapspace_) {
    memset(fdmapspace_, 0, sizeof(fdmapspace_));
}

xpollfds::~xpollfds() {
    if (pfds_ != pfdspace_) {
        delete[] pfds_;
    }
    if (fdmap_ != fdmapspace_) {
        delete[] fdmap_;
    }
}

int xpollfds::events(int fd) const {
    if ((unsigned) fd >= nfdmap_ || fdmap_[fd] == 0) {
        return 0;
    }
    return pfds_[fdmap_[fd] - 1].events;
}

void xpollfds::set_events(int fd, int events) {
    if ((unsigned) fd >= nfdmap_ || fdmap_[fd] == 0) {
        if (events == 0) {
            return;
        }
        if ((unsigned) fd >= nfdmap_) {
            unsigned newn = (fd | 0xFFF) + 1;
            unsigned* newmap = new unsigned[newn];
            memcpy(newmap, fdmap_, sizeof(unsigned) * nfdmap_);
            memset(newmap + nfdmap_, 0, sizeof(unsigned) * (newn - nfdmap_));
            if (fdmap_ != fdmapspace_) {
                delete[] fdmap_;
            }
            fdmap_ = newmap;
            nfdmap_ = newn;
        }
        if (npfds_ == pfdcap_) {
            pollfd* npfds = new pollfd[pfdcap_ * 2];
            memcpy(npfds, pfds_, sizeof(pollfd) * pfdcap_);
            if (pfds_ != pfdspace_) {
                delete[] pfds_;
            }
            pfds_ = npfds;
            pfdcap_ *= 2;
        }
        ++npfds_;
        fdmap_[fd] = npfds_;
        pfds_[npfds_ - 1].fd = fd;
        pfds_[npfds_ - 1].events = events;
        return;
    }
    unsigned fdidx = fdmap_[fd] - 1;
    pollfd& pfd = pfds_[fdidx];
    if (events != 0) {
        pfd.events = events;
    } else {
        fdmap_[fd] = 0;
        if (--npfds_ != 0) {
            pfd = pfds_[npfds_];
            fdmap_[pfd.fd] = fdidx + 1;
        }
    }
}

#if DTAMER_EPOLL
class xepoll_eventset {
  public:
    xepoll_eventset()
        : es_(new struct epoll_event[128]) {
    }
    ~xepoll_eventset() {
        delete[] es_;
    }
    int size() const {
        return 128;
    }
    struct epoll_event* data() {
        return es_;
    }
    epoll_event& operator[](int i) {
        return es_[i];
    }
  private:
    struct epoll_event* es_;
};
#endif

class driver_tamer;

struct fdp {
    inline fdp(driver_tamer*, int) {
    }
};

class driver_tamer : public driver {
  public:
    driver_tamer(int flags);
    ~driver_tamer();

    virtual void at_fd(int fd, int action, event<int> e);
    virtual void at_time(const timeval &expiry, event<> e, bool bg);
    virtual void at_asap(event<> e);
    virtual void at_preblock(event<> e);
    virtual void kill_fd(int fd);

    virtual void set_error_handler(error_handler_type errh);

    virtual void loop(loop_flags flags);
    virtual void break_loop();
    virtual timeval next_wake() const;

  private:
    tamerpriv::driver_fdset<fdp> fds_;
    xpollfds pfds_;
#if DTAMER_EPOLL
    int epollfd_ = -1;
#endif

    tamerpriv::driver_timerset timers_;
    tamerpriv::driver_asapset asap_;
    tamerpriv::driver_asapset preblock_;

    bool loop_state_ = false;
    bool sig_pipe_ = false;
#if DTAMER_EPOLL
    bool epoll_sig_pipe_ = false;
    pid_t epoll_pid_;
    enum { EPOLL_MAX_ERRCOUNT = 32 };
    int epoll_errcount_;
#endif
    int flags_;
    error_handler_type errh_ = 0;

    static void fd_disinterest(void* arg);
    void update_fds();
#if DTAMER_EPOLL
    void report_epoll_error(int fd, bool waspresent, int events);
    inline void mark_epoll(int fd, bool waspresent, int events);
    bool epoll_recreate();
#endif
};


driver_tamer::driver_tamer(int flags)
    : flags_(flags) {
#if DTAMER_EPOLL
    if (!(flags_ & init_no_epoll)) {
        epollfd_ = epoll_create1(EPOLL_CLOEXEC);
        epoll_pid_ = getpid();
    }
    epoll_errcount_ = epollfd_ >= 0 ? 0 : EPOLL_MAX_ERRCOUNT;
#else
    (void) flags_;
#endif
}

driver_tamer::~driver_tamer() {
#if DTAMER_EPOLL
    if (epollfd_ >= 0) {
        close(epollfd_);
    }
#endif
}

void driver_tamer::set_error_handler(error_handler_type errh) {
    errh_ = errh;
}

void driver_tamer::fd_disinterest(void* arg) {
    driver_tamer* d = static_cast<driver_tamer*>(fd_callback_driver(arg));
    d->fds_.push_change(fd_callback_fd(arg));
}

void driver_tamer::at_fd(int fd, int action, event<int> e) {
    assert(fd >= 0);
    if (e && (unsigned) action < nfdactions) {
        fds_.expand(this, fd);
        auto& x = fds_[fd];
        x.e[action] += TAMER_MOVE(e);
        tamerpriv::simple_event::at_trigger(x.e[action].__get_simple(),
                                            fd_disinterest,
                                            make_fd_callback(this, fd));
        fds_.push_change(fd);
    }
}

void driver_tamer::kill_fd(int fd) {
    if (fd >= 0 && fd < fds_.size()) {
        auto& x = fds_[fd];
        for (int action = 0; action < nfdactions; ++action) {
            x.e[action].trigger(-ECANCELED);
        }
        fds_.push_change(fd);
    }
}

static inline int poll_events(const tamerpriv::driver_fd<fdp>& x) {
    return (x.e[0] ? int(POLLIN) : 0)
        | (x.e[1] ? int(POLLOUT) : 0);
}

#if DTAMER_EPOLL
static inline int epoll_events(const tamerpriv::driver_fd<fdp>& x) {
    return (x.e[0] ? int(EPOLLIN | EPOLLRDHUP) : 0)
        | (x.e[1] ? int(EPOLLOUT) : 0);
}

void driver_tamer::report_epoll_error(int fd, bool waspresent, int events) {
    if (!events && (errno == EBADF || errno == ENOENT)) {
        return;
    }
    // the epoll file descriptor has gone wonky
    int ctl_errno = errno;
    ++epoll_errcount_;
    if (errh_) {
        char msg[1024];
        snprintf(msg, sizeof(msg), "epoll_ctl(%d, %s, %x) failure, %s",
                 fd, events ? (waspresent ? "MOD" : "ADD") : "DEL", events,
                 epoll_errcount_ < EPOLL_MAX_ERRCOUNT ? "retrying" : "giving up");
        errh_(fd, ctl_errno, msg);
    }
    close(epollfd_);
    epollfd_ = -1;
}

inline void driver_tamer::mark_epoll(int fd, bool waspresent, int events) {
    if (epollfd_ >= 0) {
        struct epoll_event ev;
        ev.events = events;
        ev.data.fd = fd;
        int action;
        if (!events) {
            action = EPOLL_CTL_DEL;
        } else if (waspresent) {
            action = EPOLL_CTL_MOD;
        } else {
            action = EPOLL_CTL_ADD;
        }
        int r = epoll_ctl(epollfd_, action, fd, &ev);
        if (r < 0) {
            report_epoll_error(fd, waspresent, events);
        }
    }
}

bool driver_tamer::epoll_recreate() {
    while (epollfd_ < 0
           && epoll_errcount_ < EPOLL_MAX_ERRCOUNT
           && (epollfd_ = epoll_create1(EPOLL_CLOEXEC)) >= 0) {
        auto endp = pfds_.end();
        for (auto p = pfds_.begin(); p != endp; ++p) {
            mark_epoll(p->fd, false, epoll_events(fds_[p->fd]));
        }
        if (epoll_sig_pipe_) {
            mark_epoll(sig_pipe[0], false, int(EPOLLIN | EPOLLRDHUP));
        }
        epoll_pid_ = getpid();
    }
    return epollfd_ >= 0;
}
#endif

void driver_tamer::update_fds() {
    int fd;
    while ((fd = fds_.pop_change()) >= 0) {
        tamerpriv::driver_fd<fdp>& x = fds_[fd];
        int old_events = pfds_.events(fd);
        int new_events = poll_events(x);
        if (old_events == new_events) {
            continue;
        }

        pfds_.set_events(fd, new_events);
#if DTAMER_EPOLL
        mark_epoll(fd, old_events != 0, epoll_events(x));
#endif
    }
}

void driver_tamer::at_time(const timeval &expiry, event<> e, bool bg) {
    if (e) {
        timers_.push(expiry, e.__release_simple(), bg);
    }
}

void driver_tamer::at_asap(event<> e) {
    if (e) {
        asap_.push(e.__release_simple());
    }
}

void driver_tamer::at_preblock(event<> e) {
    if (e) {
        preblock_.push(e.__release_simple());
    }
}

void driver_tamer::loop(loop_flags flags)
{
    if (flags == loop_forever) {
        loop_state_ = true;
    }
#if DTAMER_EPOLL
    xepoll_eventset epollnow;
    if (epollfd_ >= 0 && epoll_pid_ != getpid()) {
        close(epollfd_);
        epollfd_ = -1;
    }
#endif

 again:
    // process asap events
    while (!preblock_.empty()) {
        preblock_.pop_trigger();
    }
    run_unblocked();

    // fix file descriptors
    if (fds_.has_change()) {
        update_fds();
    }

    // determine timeout
    timers_.cull();
    int blockms;
    if (!asap_.empty() || sig_any_active || has_unblocked()) {
        blockms = 0;
    } else if (!timers_.empty()) {
        timeval tnow = now();
        if (!timercmp(&timers_.expiry(), &tnow, >)) {
            blockms = 0;
        } else {
            if (!timers_.has_foreground()
                && pfds_.size() <= sig_pipe_
                && sig_nforeground == 0) {
                return; // no more foreground events
            }
            if (tamerpriv::time_type == time_virtual) {
                blockms = 0;
            } else {
                timeval to;
                timersub(&timers_.expiry(), &tnow, &to);
                blockms = to.tv_sec * 1000 + (to.tv_usec + 250) / 1000;
            }
        }
    } else {
        if (pfds_.size() <= sig_pipe_
            && sig_nforeground == 0) {
            return; // no more foreground events
        }
        blockms = -1;
    }

    // select!
    int eventcount = 0;
#if DTAMER_EPOLL
    if (epollfd_ >= 0 || epoll_recreate()) {
        if (!epoll_sig_pipe_ && sig_pipe[0] >= 0) {
            mark_epoll(sig_pipe[0], false, int(EPOLLIN | EPOLLRDHUP));
            epoll_sig_pipe_ = true;
        }
        eventcount = epoll_wait(epollfd_, epollnow.data(), epollnow.size(),
                                blockms);
        goto after_poll;
    }
#endif

    if (!sig_pipe_ && sig_pipe[0] >= 0) {
        pfds_.set_events(sig_pipe[0], int(POLLIN | POLLRDHUP));
        sig_pipe_ = true;
    }
    if (pfds_.size() > sig_pipe_ || blockms != 0) {
        eventcount = ::poll(pfds_.pollfds(), pfds_.size(), blockms);
        goto after_poll;
    }

after_poll:
    // process signals
    set_recent();
    if (sig_any_active) {
        dispatch_signals();
    }

    // process fd events
#if DTAMER_EPOLL
    if (epollfd_ >= 0) {
        for (int i = 0; i < eventcount; ++i) {
            struct epoll_event& e = epollnow[i];
            if (e.data.fd == sig_pipe[0]) {
                continue;
            }
            auto& x = fds_[e.data.fd];
            if (e.events & (EPOLLERR | EPOLLHUP)) {
                x.e[0].trigger(e.events & EPOLLERR ? -ECONNRESET : 0);
                x.e[1].trigger(-ESHUTDOWN);
            } else {
                if (e.events & (EPOLLIN | EPOLLRDHUP)) {
                    x.e[0].trigger(0);
                }
                if (e.events & EPOLLOUT) {
                    x.e[1].trigger(0);
                }
            }
        }
        run_unblocked();
        goto after_trigger_fd;
    }
#endif

    if (eventcount > 0) {
        auto endp = pfds_.end();
        for (auto p = pfds_.begin(); p != endp; ++p) {
            if (p->revents == 0 || p->fd == sig_pipe[0]) {
                continue;
            }
            auto& x = fds_[p->fd];
            if (p->revents & int(POLLNVAL | POLLERR | POLLHUP)) {
                x.e[0].trigger(p->revents & int(POLLNVAL | POLLERR) ? -ECONNRESET : 0);
                x.e[1].trigger(-ESHUTDOWN);
            } else {
                if (p->revents & int(POLLIN | POLLRDHUP)) {
                    x.e[0].trigger(0);
                }
                if (p->revents & int(POLLOUT)) {
                    x.e[1].trigger(0);
                }
            }
        }
        run_unblocked();
        goto after_trigger_fd;
    }

after_trigger_fd:
    // process timer events
    if (!timers_.empty()
        && tamerpriv::time_type == time_virtual
        && eventcount == 0) {
        tamerpriv::recent = timers_.expiry();
    }
    while (!timers_.empty()
           && !timercmp(&timers_.expiry(), &recent(), >)) {
        timers_.pop_trigger();
    }
    run_unblocked();

    // process asap events
    while (!asap_.empty()) {
        asap_.pop_trigger();
    }
    run_unblocked();

    // check flags
    if (loop_state_) {
        goto again;
    }
}

void driver_tamer::break_loop() {
    loop_state_ = false;
}

timeval driver_tamer::next_wake() const {
    struct timeval tv = { 0, 0 };
    if (!asap_.empty()
        || sig_any_active
        || has_unblocked()) {
        /* already zero */;
    } else if (timers_.empty()) {
        tv.tv_sec = -1;
    } else {
        tv = timers_.expiry();
    }
    return tv;
}

} // namespace

driver *driver::make_tamer(int flags) {
    return new driver_tamer(flags);
}

} // namespace tamer
