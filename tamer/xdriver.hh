#ifndef TAMER_XDRIVER_HH
#define TAMER_XDRIVER_HH 1
/* Copyright (c) 2007-2013, Eddie Kohler
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
namespace tamerpriv {
extern struct timeval now;
extern bool now_updated;
} // namespace tamerpriv

enum loop_flags {
    loop_default = 0,
    loop_forever = 0,
    loop_once = 1
};

enum signal_flags {
    signal_default = 0,
    signal_background = 1
};

class driver {
  public:
    driver();
    virtual ~driver();

    enum { capacity = (unsigned) 256 };
    static inline driver* by_index(unsigned index);
    inline unsigned index() const;

    // basic functions
    enum { fdread = 0, fdwrite = 1 }; // the order is important
    virtual void at_fd(int fd, int action, event<int> e) = 0;
    virtual void at_time(const timeval &expiry, event<> e) = 0;
    virtual void at_asap(event<> e) = 0;
    virtual void kill_fd(int fd) = 0;

    inline void at_fd(int fd, int action, event<> e);
    inline void at_fd_read(int fd, event<int> e);
    inline void at_fd_read(int fd, event<> e);
    inline void at_fd_write(int fd, event<int> e);
    inline void at_fd_write(int fd, event<> e);

    inline void at_time(double expiry, event<> e);
    inline void at_delay(timeval delay, event<> e);
    void at_delay(double delay, event<> e);
    inline void at_delay_sec(int delay, event<> e);
    inline void at_delay_msec(int delay, event<> e);
    inline void at_delay_usec(int delay, event<> e);

    static void at_signal(int signo, event<> e,
			  signal_flags flags = signal_default);

    virtual void loop(loop_flags flag) = 0;
    virtual void break_loop() = 0;

    static driver* make_tamer();
    static driver* make_libevent();
    static driver* make_libev();

    static driver *main;

    static volatile sig_atomic_t sig_any_active;
    static int sig_pipe[2];
    static unsigned sig_nforeground;
    static unsigned sig_ntotal;
    static void dispatch_signals();

  private:
    unsigned index_;

    static driver* indexed[capacity];
    static int next_index;
};

inline const timeval &now();

inline driver* driver::by_index(unsigned index) {
    return index < capacity ? indexed[index] : 0;
}

inline unsigned driver::index() const {
    return index_;
}

inline void driver::at_fd(int fd, int action, event<> e) {
    at_fd(fd, action, event<int>(e, no_result()));
}

inline void driver::at_fd_read(int fd, event<int> e) {
    at_fd(fd, fdread, e);
}

inline void driver::at_fd_write(int fd, event<int> e) {
    at_fd(fd, fdwrite, e);
}

inline void driver::at_fd_read(int fd, event<> e) {
    at_fd(fd, fdread, e);
}

inline void driver::at_fd_write(int fd, event<> e) {
    at_fd(fd, fdwrite, e);
}

inline void driver::at_time(double expiry, event<> e) {
    timeval tv;
    tv.tv_sec = (long) expiry;
    tv.tv_usec = (long) ((expiry - tv.tv_sec) * 1000000);
    at_time(tv, e);
}

inline void driver::at_delay(timeval delay, event<> e) {
    timeradd(&delay, &now(), &delay);
    at_time(delay, e);
}

inline void driver::at_delay_sec(int delay, event<> e) {
    if (delay <= 0)
	at_asap(e);
    else {
	timeval tv = now();
	tv.tv_sec += delay;
	at_time(tv, e);
    }
}

inline void driver::at_delay_msec(int delay, event<> e) {
    if (delay <= 0)
	at_asap(e);
    else {
	timeval tv;
	tv.tv_sec = delay / 1000;
	tv.tv_usec = (delay % 1000) * 1000;
	at_delay(tv, e);
    }
}

inline void driver::at_delay_usec(int delay, event<> e) {
    if (delay <= 0)
	at_asap(e);
    else {
	timeval tv;
	tv.tv_sec = delay / 1000000;
	tv.tv_usec = delay % 1000000;
	at_delay(tv, e);
    }
}

} // namespace tamer
#endif /* TAMER_XDRIVER_HH */
