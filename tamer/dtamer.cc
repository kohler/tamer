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
#include <sys/select.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#if HAVE_SYS_EPOLL_H && !TAMER_NOEPOLL
# include <sys/epoll.h>
# ifndef EPOLLRDHUP
#  define EPOLLRDHUP 0
# endif
#endif

namespace tamer {
namespace {
using tamerpriv::make_fd_callback;
using tamerpriv::fd_callback_driver;
using tamerpriv::fd_callback_fd;

union xfd_set {
    fd_set fds;
    char s[1];
    uint64_t q[1];
#if SIZEOF_FD_SET_ELEMENT == 1
    typedef uint8_t elt_type;
#elif SIZEOF_FD_SET_ELEMENT == 2
    typedef uint16_t elt_type;
#elif SIZEOF_FD_SET_ELEMENT == 4
    typedef uint32_t elt_type;
#elif SIZEOF_FD_SET_ELEMENT == 8
    typedef uint64_t elt_type;
#else
# undef SIZEOF_FD_SET_ELEMENT
    typedef uint8_t elt_type;
#endif
    elt_type elt[1];
};

class xfd_setpair {
#if SIZEOF_FD_SET_ELEMENT
    typedef xfd_set::elt_type elt_type;
    enum { elt_bits = sizeof(elt_type) * 8 };
    static inline unsigned elt_index(unsigned fd) {
        return fd / elt_bits;
    }
    static inline elt_type elt_bit(unsigned fd) {
        return elt_type(1) << (fd % elt_bits);
    }
#endif
  public:
    inline xfd_setpair() {
        f_[0] = f_[1] = 0;
        cap_ = 0;
    }
    inline ~xfd_setpair() {
        delete[] reinterpret_cast<char*>(f_[0]);
    }
    inline int size() const {
        return cap_;
    }
    inline void ensure(int fd) {
        if (unsigned(fd) >= cap_)
            hard_ensure(fd);
    }
    inline void copy(const xfd_setpair& x, int bound);
    inline void copy_combined(const xfd_setpair& x, int bound);
    inline void transfer_segment(int lb, int rb);
    inline void clear();
    inline bool isset(int action, int fd) const {
        assert(unsigned(action) < nfdactions && unsigned(fd) < cap_);
#if SIZEOF_FD_SET_ELEMENT
        return f_[action]->elt[elt_index(fd)] & elt_bit(fd);
#else
        return FD_ISSET(fd, &f_[action]->fds);
#endif
    }
    inline void set(int action, int fd) {
        assert(unsigned(action) < nfdactions && unsigned(fd) < cap_);
#if SIZEOF_FD_SET_ELEMENT
        f_[action]->elt[elt_index(fd)] |= elt_bit(fd);
#else
        FD_SET(fd, &f_[action]->fds);
#endif
    }
    inline void clear(int action, int fd) {
        assert(unsigned(action) < nfdactions && unsigned(fd) < cap_);
#if SIZEOF_FD_SET_ELEMENT
        f_[action]->elt[elt_index(fd)] &= ~elt_bit(fd);
#else
        FD_CLR(fd, &f_[action]->fds);
#endif
    }
    inline fd_set* get_fd_set(int action) {
        assert(unsigned(action) < nfdactions);
        return &f_[action]->fds;
    }
  private:
    xfd_set* f_[nfdactions];
    unsigned cap_;
    void hard_ensure(int fd);
};

void xfd_setpair::hard_ensure(int fd) {
    unsigned ncap = cap_ ? cap_ * 2 : sizeof(xfd_set) * 8;
    while (unsigned(fd) >= ncap)
        ncap *= 2;
    assert((ncap % 64) == 0);
    char* ndata = new char[ncap / 4];
    xfd_set* nf[nfdactions] = {
        reinterpret_cast<xfd_set*>(ndata),
        reinterpret_cast<xfd_set*>(&ndata[ncap / 8])
    };
    if (f_[0]) {
        memcpy(nf[0], f_[0], cap_ / 8);
        memcpy(nf[1], f_[1], cap_ / 8);
    }
    memset(&nf[0]->s[cap_ / 8], 0, (ncap - cap_) / 8);
    memset(&nf[1]->s[cap_ / 8], 0, (ncap - cap_) / 8);
    delete[] reinterpret_cast<char*>(f_[0]);
    f_[0] = nf[0];
    f_[1] = nf[1];
    cap_ = ncap;
}

inline void xfd_setpair::copy(const xfd_setpair& x, int bound) {
    ensure(bound);
    unsigned nb = ((unsigned(bound) + 63) & ~63) >> 3;
    memcpy(f_[0], x.f_[0], nb);
    memcpy(f_[1], x.f_[1], nb);
}

inline void xfd_setpair::copy_combined(const xfd_setpair& x, int bound) {
    ensure(bound);
    unsigned nb = ((unsigned(bound) + 63) & ~63) >> 3;
    memcpy(f_[0], x.f_[0], nb);
    for (unsigned i = 0; i != (nb >> 3); ++i)
        f_[0]->q[i] |= x.f_[1]->q[i];
}

inline void xfd_setpair::transfer_segment(int lb, int rb) {
    memset(f_[1], 0, lb);
    memcpy(&f_[1]->s[lb], &f_[0]->s[lb], rb - lb);
}

inline void xfd_setpair::clear() {
    memset(f_[0], 0, cap_ >> 2);
}

#if HAVE_SYS_EPOLL_H && !TAMER_NOEPOLL
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

    struct fdp {
        inline fdp(driver_tamer*, int) {
        }
    };

    tamerpriv::driver_fdset<fdp> fds_;
    unsigned fdbound_;
#if HAVE_SYS_EPOLL_H && !TAMER_NOEPOLL
    int epollfd_;
#endif
    xfd_setpair fdsets_;

    tamerpriv::driver_timerset timers_;

    tamerpriv::driver_asapset asap_;
    tamerpriv::driver_asapset preblock_;

    bool loop_state_;
#if HAVE_SYS_EPOLL_H && !TAMER_NOEPOLL
    bool epoll_sig_pipe_;
    pid_t epoll_pid_;
    int epoll_errcount_;
    enum { EPOLL_MAX_ERRCOUNT = 32 };
#endif
    int flags_;
    error_handler_type errh_;

    static void fd_disinterest(void* arg);
    void update_fds();
    int find_bad_fds(xfd_setpair&);
#if HAVE_SYS_EPOLL_H && !TAMER_NOEPOLL
    static inline int epoll_events(bool readable, bool writable);
    void report_epoll_error(int fd, int old_events, int events);
    inline void mark_epoll(int fd, int old_events, int events);
    bool epoll_recreate();
#endif
};


driver_tamer::driver_tamer(int flags)
    : fdbound_(0), loop_state_(false), flags_(flags), errh_(0) {
#if HAVE_SYS_EPOLL_H && !TAMER_NOEPOLL
    epollfd_ = -1;
    epoll_errcount_ = EPOLL_MAX_ERRCOUNT;
    if (!(flags_ & init_no_epoll)) {
        epollfd_ = epoll_create1(EPOLL_CLOEXEC);
        epoll_sig_pipe_ = false;
        epoll_pid_ = getpid();
    }
    if (epollfd_ >= 0)
        epoll_errcount_ = 0;
#else
    (void) flags_;
#endif
}

driver_tamer::~driver_tamer() {
#if HAVE_SYS_EPOLL_H && !TAMER_NOEPOLL
    if (epollfd_ >= 0)
        close(epollfd_);
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
        tamerpriv::driver_fd<fdp>& x = fds_[fd];
        x.e[action] += TAMER_MOVE(e);
        tamerpriv::simple_event::at_trigger(x.e[action].__get_simple(),
                                            fd_disinterest,
                                            make_fd_callback(this, fd));
        fds_.push_change(fd);
    }
}

void driver_tamer::kill_fd(int fd) {
    if (fd >= 0 && fd < fds_.size()) {
        tamerpriv::driver_fd<fdp> &x = fds_[fd];
        x.e[0].trigger(-ECANCELED);
        x.e[1].trigger(-ECANCELED);
        fds_.push_change(fd);
    }
}

#if HAVE_SYS_EPOLL_H && !TAMER_NOEPOLL
inline int driver_tamer::epoll_events(bool readable, bool writable) {
    return (readable ? int(EPOLLIN | EPOLLRDHUP) : 0) | (writable ? int(EPOLLOUT) : 0);
}

void driver_tamer::report_epoll_error(int fd, int old_events, int events) {
    if (!events && (errno == EBADF || errno == ENOENT))
        return;
    // the epoll file descriptor has gone wonky
    int ctl_errno = errno;
    ++epoll_errcount_;
    if (errh_) {
        char msg[1024];
        snprintf(msg, sizeof(msg), "epoll_ctl(%d, %x->%x) failure, %s",
                 fd, old_events, events,
                 epoll_errcount_ < EPOLL_MAX_ERRCOUNT ? "retrying" : "giving up");
        errh_(fd, ctl_errno, msg);
    }
    close(epollfd_);
    epollfd_ = -1;
}

inline void driver_tamer::mark_epoll(int fd, int old_events, int events) {
    if (old_events != events && epollfd_ >= 0) {
        struct epoll_event ev;
        memset(&ev, 0, sizeof(ev)); // avoid valgrind complaints
        ev.events = events;
        ev.data.fd = fd;
        int action;
        if (!old_events)
            action = EPOLL_CTL_ADD;
        else if (events)
            action = EPOLL_CTL_MOD;
        else
            action = EPOLL_CTL_DEL;
        int r = epoll_ctl(epollfd_, action, fd, &ev);
        if (r < 0)
            report_epoll_error(fd, old_events, events);
    }
}

bool driver_tamer::epoll_recreate() {
    while (epollfd_ < 0 && epoll_errcount_ < EPOLL_MAX_ERRCOUNT
           && (epollfd_ = epoll_create1(EPOLL_CLOEXEC)) >= 0) {
        int max = std::min(fds_.size(), fdsets_.size());
        for (int fd = 0; fd < max; ++fd) {
            int events = epoll_events(fdsets_.isset(0, fd),
                                      fdsets_.isset(1, fd));
            if (events)
                mark_epoll(fd, 0, events);
        }
        if (epoll_sig_pipe_)
            mark_epoll(sig_pipe[0], 0, epoll_events(true, false));
        epoll_pid_ = getpid();
    }
    return epollfd_ >= 0;
}
#endif

void driver_tamer::update_fds() {
    int fd;
    while ((fd = fds_.pop_change()) >= 0) {
        tamerpriv::driver_fd<fdp>& x = fds_[fd];
        fdsets_.ensure(fd);

        bool wasset[nfdactions] = { false, false };
        for (int action = 0; action < nfdactions; ++action) {
            wasset[action] = fdsets_.isset(action, fd);
            if (x.e[action])
                fdsets_.set(action, fd);
            else
                fdsets_.clear(action, fd);
        }

#if HAVE_SYS_EPOLL_H && !TAMER_NOEPOLL
        if (epollfd_ >= 0)
            mark_epoll(fd, epoll_events(wasset[0], wasset[1]),
                       epoll_events(bool(x.e[0]), bool(x.e[1])));
#else
        (void) wasset;
#endif

        if (!x.empty()) {
            if ((unsigned) fd >= fdbound_)
                fdbound_ = fd + 1;
        } else if ((unsigned) fd + 1 == fdbound_) {
            do {
                --fdbound_;
            } while (fdbound_ > 0 && fds_[fdbound_ - 1].empty());
        }
    }
}

void driver_tamer::at_time(const timeval &expiry, event<> e, bool bg) {
    if (e)
        timers_.push(expiry, e.__release_simple(), bg);
}

void driver_tamer::at_asap(event<> e) {
    if (e)
        asap_.push(e.__release_simple());
}

void driver_tamer::at_preblock(event<> e) {
    if (e)
        preblock_.push(e.__release_simple());
}

void driver_tamer::loop(loop_flags flags)
{
    if (flags == loop_forever)
        loop_state_ = true;
    xfd_setpair fdnow;
#if HAVE_SYS_EPOLL_H && !TAMER_NOEPOLL
    xepoll_eventset epollnow;
    if (epollfd_ >= 0 && epoll_pid_ != getpid()) {
        close(epollfd_);
        epollfd_ = -1;
    }
#endif

 again:
    // process asap events
    while (!preblock_.empty())
        preblock_.pop_trigger();
    run_unblocked();

    // fix file descriptors
    if (fds_.has_change())
        update_fds();

    // determine timeout
    timers_.cull();
    timeval to, *toptr = &to;
    timerclear(&to);
    if (asap_.empty() && !sig_any_active && !has_unblocked()) {
        if (timers_.empty()) {
            if (fdbound_ == 0 && sig_nforeground == 0)
                // no foreground events!
                return;
            toptr = 0;
        } else {
            timeval tnow = now();
            if (!timers_.has_foreground() && fdbound_ == 0 && sig_nforeground == 0
                && timercmp(&timers_.expiry(), &tnow, >))
                // no foreground events!
                return;
            if (tamerpriv::time_type != time_virtual
                && timercmp(&timers_.expiry(), &tnow, >))
                timersub(&timers_.expiry(), &tnow, &to);
        }
    }

    // select!
    int nfds = 0;
#if HAVE_SYS_EPOLL_H && !TAMER_NOEPOLL
    int nepoll = 0;
    if (epollfd_ >= 0 || epoll_recreate()) {
        if (!epoll_sig_pipe_ && sig_pipe[0] >= 0) {
            mark_epoll(sig_pipe[0], 0, epoll_events(true, false));
            epoll_sig_pipe_ = true;
        }
        int blockms;
        if (!toptr)
            blockms = -1;
        else
            blockms = toptr->tv_sec * 1000 + (toptr->tv_usec + 250) / 1000;
        nepoll = epoll_wait(epollfd_, epollnow.data(), epollnow.size(),
                            blockms);
        goto after_select;
    }
#endif

    nfds = fdbound_;
    if (sig_pipe[0] > nfds) {
        fdsets_.ensure(sig_pipe[0]);
        nfds = sig_pipe[0] + 1;
    }
    if (nfds > 0) {
        fdnow.copy(fdsets_, nfds);
        if (sig_pipe[0] >= 0)
            fdnow.set(0, sig_pipe[0]);
    }
    if (nfds > 0 || !toptr || to.tv_sec != 0 || to.tv_usec != 0) {
        nfds = select(nfds, fdnow.get_fd_set(0), fdnow.get_fd_set(1), 0, toptr);
        if (nfds == -1 && errno == EBADF)
            nfds = find_bad_fds(fdnow);
        goto after_select;
    }

 after_select:
    // process signals
    set_recent();
    if (sig_any_active)
        dispatch_signals();

    // process fd events
#if HAVE_SYS_EPOLL_H && !TAMER_NOEPOLL
    if (epollfd_ >= 0) {
        for (int i = 0; i < nepoll; ++i) {
            struct epoll_event& e = epollnow[i];
            if (e.data.fd == sig_pipe[0])
                continue;
            if (e.events & (EPOLLIN | EPOLLRDHUP))
                fds_[e.data.fd].e[0].trigger(0);
            else if (e.events & (EPOLLERR | EPOLLHUP))
                fds_[e.data.fd].e[0].trigger(-1);
            if (e.events & EPOLLOUT)
                fds_[e.data.fd].e[1].trigger(0);
            else if (e.events & (EPOLLERR | EPOLLHUP))
                fds_[e.data.fd].e[1].trigger(-1);
        }
        run_unblocked();
        goto after_fd;
    }
#endif

    if (nfds > 0) {
        for (unsigned fd = 0; fd < fdbound_; ++fd) {
            tamerpriv::driver_fd<fdp> &x = fds_[fd];
            for (int action = 0; action < nfdactions; ++action)
                if (fdnow.isset(action, fd))
                    x.e[action].trigger(0);
        }
        run_unblocked();
        goto after_fd;
    }

 after_fd:
    // process timer events
    if (!timers_.empty() && tamerpriv::time_type == time_virtual && nfds == 0)
        tamerpriv::recent = timers_.expiry();
    while (!timers_.empty() && !timercmp(&timers_.expiry(), &recent(), >))
        timers_.pop_trigger();
    run_unblocked();

    // process asap events
    while (!asap_.empty())
        asap_.pop_trigger();
    run_unblocked();

    // check flags
    if (loop_state_)
        goto again;
}

int driver_tamer::find_bad_fds(xfd_setpair& fdnow) {
    // first, combine all file descriptors from read & write
    fdnow.copy_combined(fdsets_, fdbound_);
    // use binary search to find a bad file descriptor
    int l = 0, r = (fdbound_ + 7) & ~7;
    while (r - l > 1) {
        int m = l + ((r - l) >> 1);
        fdnow.transfer_segment(l, m);
        struct timeval tv = {0, 0};
        int nfds = select(m << 3, fdnow.get_fd_set(1), 0, 0, &tv);
        if (nfds == -1 && errno == EBADF)
            r = m;
        else if (nfds != -1)
            l = m;
    }
    // down to <= 8 file descriptors; test them one by one
    // clear result sets
    fdnow.clear();
    // set up result sets
    int nfds = 0;
    for (int f = l * 8; f != r * 8; ++f) {
        int fr = fdsets_.isset(0, f);
        int fw = fdsets_.isset(1, f);
        if ((fr || fw) && fcntl(f, F_GETFL) == -1 && errno == EBADF) {
            if (fr)
                fdnow.set(0, f);
            if (fw)
                fdnow.set(1, f);
            ++nfds;
        }
    }
    return nfds;
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
