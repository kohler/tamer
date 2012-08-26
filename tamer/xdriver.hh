#ifndef TAMER_XDRIVER_HH
#define TAMER_XDRIVER_HH 1
/* Copyright (c) 2007-2012, Eddie Kohler
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
#include <tamer/event.hh>
#include <sys/time.h>
#include <signal.h>
namespace tamer {

class driver { public:

    inline driver();
    virtual inline ~driver();

    // basic functions
    enum { fdread = 0, fdwrite = 1 }; // the order is important
    virtual void store_fd(int fd, int action, tamerpriv::simple_event *se, int *slot) = 0;
    virtual void store_time(const timeval &expiry, tamerpriv::simple_event *se) = 0;
    virtual void store_asap(tamerpriv::simple_event *se);
    virtual void kill_fd(int fd);

    inline void at_fd(int fd, int action, const event<int> &e);
    inline void at_fd(int fd, int action, const event<> &e);
    inline void at_fd_read(int fd, const event<int> &e);
    inline void at_fd_read(int fd, const event<> &e);
    inline void at_fd_write(int fd, const event<int> &e);
    inline void at_fd_write(int fd, const event<> &e);

    inline void at_time(const timeval &expiry, const event<> &e);
    inline void at_delay(timeval delay, const event<> &e);
    void at_delay(double delay, const event<> &e);
    inline void at_delay_sec(int delay, const event<> &e);
    inline void at_delay_msec(int delay, const event<> &e);
    inline void at_delay_usec(int delay, const event<> &e);

    inline void at_asap(const event<> &e);

#if TAMER_HAVE_CXX_RVALUE_REFERENCES
    inline void at_fd(int fd, int action, event<int> &&e);
    inline void at_fd(int fd, int action, event<> &&e);
    inline void at_fd_read(int fd, event<int> &&e);
    inline void at_fd_read(int fd, event<> &&e);
    inline void at_fd_write(int fd, event<int> &&e);
    inline void at_fd_write(int fd, event<> &&e);
    inline void at_time(const timeval &expiry, event<> &&e);
    inline void at_delay(timeval delay, event<> &&e);
    void at_delay(double delay, event<> &&e);
    inline void at_delay_sec(int delay, event<> &&e);
    inline void at_delay_msec(int delay, event<> &&e);
    inline void at_delay_usec(int delay, event<> &&e);
    inline void at_asap(event<> &&e);
#endif

    static void at_signal(int signo, const event<> &e);

    timeval now;
    inline void set_now();

    virtual bool empty() = 0;
    virtual void once() = 0;
    virtual void loop() = 0;

    static driver *make_tamer();
    static driver *make_libevent();

    static driver *main;

    static volatile sig_atomic_t sig_any_active;
    static int sig_pipe[2];
    static void dispatch_signals();

};

inline driver::driver() {
}

inline driver::~driver() {
}

inline void driver::store_asap(tamerpriv::simple_event *se) {
    at_time(now, event<>::__make(se));
}

inline void driver::kill_fd(int) {
}

inline void driver::at_fd(int fd, int action, const event<int> &e) {
    tamerpriv::simple_event *se = e.__get_simple();
    tamerpriv::simple_event::use(se);
    store_fd(fd, action, se, e.__get_slot0());
}

inline void driver::at_fd(int fd, int action, const event<> &e) {
    tamerpriv::simple_event *se = e.__get_simple();
    tamerpriv::simple_event::use(se);
    store_fd(fd, action, se, 0);
}

inline void driver::at_time(const timeval &expiry, const event<> &e) {
    tamerpriv::simple_event *se = e.__get_simple();
    tamerpriv::simple_event::use(se);
    store_time(expiry, se);
}

inline void driver::at_asap(const event<> &e) {
    tamerpriv::simple_event *se = e.__get_simple();
    tamerpriv::simple_event::use(se);
    store_asap(se);
}

#if TAMER_HAVE_CXX_RVALUE_REFERENCES
inline void driver::at_fd(int fd, int action, event<int> &&e) {
    store_fd(fd, action, e.__take_simple(), e.__get_slot0());
}

inline void driver::at_fd(int fd, int action, event<> &&e) {
    store_fd(fd, action, e.__take_simple(), 0);
}

inline void driver::at_time(const timeval &expiry, event<> &&e) {
    store_time(expiry, e.__take_simple());
}

inline void driver::at_asap(event<> &&e) {
    store_asap(e.__take_simple());
}
#endif

inline void driver::set_now() {
    gettimeofday(&now, 0);
}

inline void driver::at_fd_read(int fd, const event<int> &e) {
    at_fd(fd, fdread, e);
}

inline void driver::at_fd_write(int fd, const event<int> &e) {
    at_fd(fd, fdwrite, e);
}

inline void driver::at_fd_read(int fd, const event<> &e) {
    at_fd(fd, fdread, e);
}

inline void driver::at_fd_write(int fd, const event<> &e) {
    at_fd(fd, fdwrite, e);
}

#if TAMER_HAVE_CXX_RVALUE_REFERENCES
inline void driver::at_fd_read(int fd, event<int> &&e) {
    at_fd(fd, fdread, TAMER_MOVE(e));
}

inline void driver::at_fd_write(int fd, event<int> &&e) {
    at_fd(fd, fdwrite, TAMER_MOVE(e));
}

inline void driver::at_fd_read(int fd, event<> &&e) {
    at_fd(fd, fdread, TAMER_MOVE(e));
}

inline void driver::at_fd_write(int fd, event<> &&e) {
    at_fd(fd, fdwrite, TAMER_MOVE(e));
}
#endif

inline void driver::at_delay(timeval delay, const event<> &e) {
    timeradd(&delay, &now, &delay);
    at_time(delay, e);
}

inline void driver::at_delay_sec(int delay, const event<> &e) {
    if (delay <= 0)
	at_asap(e);
    else {
	timeval tv = now;
	tv.tv_sec += delay;
	at_time(tv, e);
    }
}

inline void driver::at_delay_msec(int delay, const event<> &e) {
    if (delay <= 0)
	at_asap(e);
    else {
	timeval tv;
	tv.tv_sec = delay / 1000;
	tv.tv_usec = (delay % 1000) * 1000;
	at_delay(tv, e);
    }
}

inline void driver::at_delay_usec(int delay, const event<> &e) {
    if (delay <= 0)
	at_asap(e);
    else {
	timeval tv;
	tv.tv_sec = delay / 1000000;
	tv.tv_usec = delay % 1000000;
	at_delay(tv, e);
    }
}

#if TAMER_HAVE_CXX_RVALUE_REFERENCES
inline void driver::at_delay(timeval delay, event<> &&e) {
    timeradd(&delay, &now, &delay);
    at_time(delay, TAMER_MOVE(e));
}

inline void driver::at_delay_sec(int delay, event<> &&e) {
    if (delay <= 0)
	at_asap(TAMER_MOVE(e));
    else {
	timeval tv = now;
	tv.tv_sec += delay;
	at_time(tv, TAMER_MOVE(e));
    }
}

inline void driver::at_delay_msec(int delay, event<> &&e) {
    if (delay <= 0)
	at_asap(TAMER_MOVE(e));
    else {
	timeval tv;
	tv.tv_sec = delay / 1000;
	tv.tv_usec = (delay % 1000) * 1000;
	at_delay(tv, TAMER_MOVE(e));
    }
}

inline void driver::at_delay_usec(int delay, event<> &&e) {
    if (delay <= 0)
	at_asap(TAMER_MOVE(e));
    else {
	timeval tv;
	tv.tv_sec = delay / 1000000;
	tv.tv_usec = delay % 1000000;
	at_delay(tv, TAMER_MOVE(e));
    }
}
#endif

}
#endif /* TAMER_XDRIVER_HH */
