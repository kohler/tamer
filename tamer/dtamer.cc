/* Copyright (c) 2007-2014, Eddie Kohler
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
#if HAVE_SYS_EPOLL_H
# include <sys/epoll.h>
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
};

class xfd_setpair {
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
    inline fd_set& operator[](int action) {
        assert(unsigned(action) < 2);
        return f_[action]->fds;
    }
    inline const fd_set& operator[](int action) const {
        assert(unsigned(action) < 2);
        return f_[action]->fds;
    }
  private:
    xfd_set* f_[2];
    unsigned cap_;
    void hard_ensure(int fd);
};

void xfd_setpair::hard_ensure(int fd) {
    unsigned ncap = cap_ ? cap_ * 2 : sizeof(xfd_set) * 8;
    while (unsigned(fd) >= ncap)
        ncap *= 2;
    assert((ncap % 64) == 0);
    xfd_set* nf = reinterpret_cast<xfd_set*>(new char[ncap / 4]);
    memcpy(&nf->s[0], f_[0], cap_ / 8);
    memset(&nf->s[cap_ / 8], 0, (ncap - cap_) / 8);
    memcpy(&nf->s[ncap / 8], f_[1], cap_ / 8);
    memset(&nf->s[(ncap + cap_) / 8], 0, (ncap - cap_) / 8);
    delete[] reinterpret_cast<char*>(f_[0]);
    f_[0] = nf;
    f_[1] = reinterpret_cast<xfd_set*>(&nf->s[ncap / 8]);
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

#if HAVE_SYS_EPOLL_H
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
    driver_tamer();
    ~driver_tamer();

    virtual void at_fd(int fd, int action, event<int> e);
    virtual void at_time(const timeval &expiry, event<> e, bool bg);
    virtual void at_asap(event<> e);
    virtual void at_preblock(event<> e);
    virtual void kill_fd(int fd);

    virtual void loop(loop_flags flags);
    virtual void break_loop();

  private:

    struct fdp {
	inline fdp(driver_tamer*, int) {
	}
    };

    tamerpriv::driver_fdset<fdp> fds_;
    unsigned fdbound_;
#if HAVE_SYS_EPOLL_H
    int epollfd_;
#endif
    xfd_setpair fdsets_;

    tamerpriv::driver_timerset timers_;

    tamerpriv::driver_asapset asap_;
    tamerpriv::driver_asapset preblock_;

    bool loop_state_;
#if HAVE_SYS_EPOLL_H
    bool epoll_sig_pipe_;
    pid_t epoll_pid_;
#endif

    static void fd_disinterest(void* arg);
    void update_fds();
    int find_bad_fds(xfd_setpair&);
#if HAVE_SYS_EPOLL_H
    static inline int epoll_fd_events(bool readable, bool writable);
    inline void epoll_fd(int fd, int old_events, int events);
    void epoll_recreate();
#endif
};


driver_tamer::driver_tamer()
    : fdbound_(0), loop_state_(false) {
#if HAVE_SYS_EPOLL_H
    epollfd_ = epoll_create1(EPOLL_CLOEXEC);
    epoll_sig_pipe_ = false;
    epoll_pid_ = getpid();
#endif
}

driver_tamer::~driver_tamer() {
#if HAVE_SYS_EPOLL_H
    if (epollfd_ >= 0)
        close(epollfd_);
#endif
}

void driver_tamer::fd_disinterest(void* arg) {
    driver_tamer* d = static_cast<driver_tamer*>(fd_callback_driver(arg));
    d->fds_.push_change(fd_callback_fd(arg));
}

void driver_tamer::at_fd(int fd, int action, event<int> e) {
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

void driver_tamer::kill_fd(int fd) {
    if (fd >= 0 && fd < fds_.size()) {
	tamerpriv::driver_fd<fdp> &x = fds_[fd];
        x.e[0].trigger(-ECANCELED);
        x.e[1].trigger(-ECANCELED);
	fds_.push_change(fd);
    }
}

#if HAVE_SYS_EPOLL_H
inline int driver_tamer::epoll_fd_events(bool readable, bool writable) {
    return (readable ? int(EPOLLIN) : 0) | (writable ? int(EPOLLOUT) : 0);
}

inline void driver_tamer::epoll_fd(int fd, int old_events, int events) {
    if (old_events != events) {
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
        if (r < 0 && (action != EPOLL_CTL_DEL || errno != EBADF)) {
            close(epollfd_);
            epollfd_ = -1;
        }
    }
}

void driver_tamer::epoll_recreate() {
    close(epollfd_);
    epollfd_ = epoll_create1(EPOLL_CLOEXEC);
    if (epollfd_ < 0)
        return;
    int max = std::min(fds_.size(), fdsets_.size());
    for (int fd = 0; fd < max; ++fd) {
        int events = epoll_fd_events(FD_ISSET(fd, &fdsets_[0]),
                                     FD_ISSET(fd, &fdsets_[1]));
        if (events)
            epoll_fd(fd, 0, events);
    }
    if (epoll_sig_pipe_)
        epoll_fd(sig_pipe[0], 0, EPOLLIN);
    epoll_pid_ = getpid();
}
#endif

void driver_tamer::update_fds() {
    int fd;
    while ((fd = fds_.pop_change()) >= 0) {
	tamerpriv::driver_fd<fdp>& x = fds_[fd];
        fdsets_.ensure(fd);

        bool wasset[2] = { false, false };
	for (int action = 0; action < 2; ++action) {
            wasset[action] = FD_ISSET(fd, &fdsets_[action]);
	    if (x.e[action])
		FD_SET(fd, &fdsets_[action]);
	    else
		FD_CLR(fd, &fdsets_[action]);
        }

#if HAVE_SYS_EPOLL_H
        if (epollfd_ >= 0)
            epoll_fd(fd, epoll_fd_events(wasset[0], wasset[1]),
                     epoll_fd_events(x.e[0], x.e[1]));
#else
        (void) wasset;
#endif

	if (x.e[0] || x.e[1]) {
	    if ((unsigned) fd >= fdbound_)
		fdbound_ = fd + 1;
	} else if ((unsigned) fd + 1 == fdbound_)
	    do {
		--fdbound_;
	    } while (fdbound_ > 0
		     && !fds_[fdbound_ - 1].e[0]
		     && !fds_[fdbound_ - 1].e[1]);
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
#if HAVE_SYS_EPOLL_H
    xepoll_eventset epollnow;
    if (epollfd_ >= 0 && epoll_pid_ != getpid())
        epoll_recreate();
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
    struct timeval to, *toptr;
    timers_.cull();
    if (!asap_.empty()
	|| (!timers_.empty() && !timercmp(&timers_.expiry(), &recent(), >))
        || (!timers_.empty() && tamerpriv::time_type == time_virtual)
	|| sig_any_active
	|| has_unblocked()) {
	timerclear(&to);
	toptr = &to;
    } else if (!timers_.has_foreground()
               && fdbound_ == 0
               && sig_nforeground == 0)
        // no foreground events!
        return;
    else if (!timers_.empty()) {
	timersub(&timers_.expiry(), &recent(), &to);
	toptr = &to;
    } else
	toptr = 0;

    // select!
    int nfds = 0;
#if HAVE_SYS_EPOLL_H
    int nepoll = 0;
    if (epollfd_ >= 0 && !epoll_sig_pipe_ && sig_pipe[0] >= 0) {
        epoll_fd(sig_pipe[0], 0, EPOLLIN);
        epoll_sig_pipe_ = true;
    }
    if (epollfd_ >= 0 || !toptr || to.tv_sec != 0 || to.tv_usec != 0) {
        int blockms;
        if (!toptr)
            blockms = -1;
        else
            blockms = to.tv_sec * 1000 + (to.tv_usec + 250) / 1000;
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
            FD_SET(sig_pipe[0], &fdnow[0]);
    }
    if (nfds > 0 || !toptr || to.tv_sec != 0 || to.tv_usec != 0) {
	nfds = select(nfds, &fdnow[0], &fdnow[1], 0, toptr);
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
#if HAVE_SYS_EPOLL_H
    if (epollfd_ >= 0) {
        for (int i = 0; i < nepoll; ++i) {
            struct epoll_event& e = epollnow[i];
            if (e.data.fd == sig_pipe[0])
                continue;
            if (e.events & EPOLLIN)
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
	    for (int action = 0; action < 2; ++action)
		if (FD_ISSET(fd, &fdnow[action]))
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
        int nfds = select(m << 3, &fdnow[1], 0, 0, &tv);
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
        int fr = FD_ISSET(f, &fdsets_[0]);
        int fw = FD_ISSET(f, &fdsets_[1]);
        if ((fr || fw) && fcntl(f, F_GETFL) == -1 && errno == EBADF) {
            if (fr)
                FD_SET(f, &fdnow[0]);
            if (fw)
                FD_SET(f, &fdnow[1]);
            ++nfds;
        }
    }
    return nfds;
}

void driver_tamer::break_loop() {
    loop_state_ = false;
}

} // namespace

driver *driver::make_tamer() {
    return new driver_tamer;
}

} // namespace tamer
