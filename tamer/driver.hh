#ifndef TAMER_DRIVER_HH
#define TAMER_DRIVER_HH 1
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
#include <tamer/xdriver.hh>
namespace tamer {

/** @file <tamer/driver.hh>
 *  @brief  Functions for registering primitive events and managing the event
 *  loop.
 */

enum init_flags {
    init_tamer = 1,
    init_libevent = 2,
    init_libev = 4,
    init_sigpipe = 0x1000,
    init_strict = 0x2000,
    init_no_epoll = 0x4000
};

/** @brief  Initialize the Tamer event loop.
 *  @param  flags  Initialization flags, taken from init_flags.
 *
 *  Call tamer::initialize at least once before registering any primitive
 *  Tamer events. The @a flags argument may contain one or more
 *  constants to request a specific driver (init_tamer, init_libevent, or
 *  init_libev).
 *
 *  By default Tamer ignores the SIGPIPE signal, which is generally what
 *  event-driven programs want. Add init_sigpipe to @a flags if you
 *  want to turn off this behavior.
 */
bool initialize(int flags = 0);

/** @brief  Clean up the Tamer event loop.
 *
 *  Delete the driver. Should not be called unless all Tamer objects are
 *  deleted.
 */
void cleanup();

enum time_type_t {
    time_normal,
    time_virtual
};

void set_time_type(time_type_t tt);

/** @brief  Return a recent snapshot of the current time. */
inline const timeval& recent() {
    if (tamerpriv::need_recent) {
        tamerpriv::recent = now();
        tamerpriv::need_recent = false;
    }
    return tamerpriv::recent;
}

/** @brief  Sets Tamer's current time to the current timestamp.
 */
inline void set_recent() {
    tamerpriv::need_recent = true;
}

/** @brief  Translate a time to a double. */
inline double dtime(const timeval tv) {
    return tv.tv_sec + tv.tv_usec / 1000000.;
}

/** @brief  Return the current time as a double. */
inline double dnow() {
    return dtime(now());
}

/** @brief  Return a recent snapshot of the current time as a double. */
inline double drecent() {
    return dtime(recent());
}

/** @brief  Run driver loop according to @a flags. */
inline void loop(loop_flags flags) {
    driver::main->loop(flags);
}

/** @brief  Run driver loop indefinitely. */
inline void loop() {
    driver::main->loop(loop_forever);
}

/** @brief  Run driver loop once. */
inline void once() {
    driver::main->loop(loop_once);
}

/** @brief  Exit the current driver loop. */
inline void break_loop() {
    driver::main->break_loop();
}

/** @brief  Register event for file descriptor readability.
 *  @param  fd  File descriptor.
 *  @param  e   Event.
 *
 *  Triggers @a e when @a fd becomes readable.  Cancels @a e when @a fd is
 *  closed.
 */
inline void at_fd_read(int fd, event<> e) {
    driver::main->at_fd(fd, fd_read, e);
}

inline void at_fd_read(int fd, event<int> e) {
    driver::main->at_fd(fd, fd_read, e);
}

/** @brief  Register event for file descriptor writability.
 *  @param  fd  File descriptor.
 *  @param  e   Event.
 *
 *  Triggers @a e when @a fd becomes writable.  Cancels @a e when @a fd is
 *  closed.
 */
inline void at_fd_write(int fd, event<> e) {
    driver::main->at_fd(fd, fd_write, e);
}

inline void at_fd_write(int fd, event<int> e) {
    driver::main->at_fd(fd, fd_write, e);
}

/** @brief  Register event for a given time.
 *  @param  expiry  Time.
 *  @param  e       Event.
 *
 *  Triggers @a e at timestamp @a expiry, or soon afterwards.
 */
inline void at_time(const timeval &expiry, event<> e, bool bg = false) {
    driver::main->at_time(expiry, e, bg);
}

/** @overload */
inline void at_time(double expiry, event<> e, bool bg = false) {
    driver::main->at_time(expiry, e, bg);
}

/** @brief  Register event for a given delay.
 *  @param  delay  Delay time.
 *  @param  e      Event.
 *
 *  Triggers @a e when @a delay seconds have elapsed since @c recent(), or
 *  soon afterwards.
 */
inline void at_delay(const timeval &delay, event<> e, bool bg = false) {
    driver::main->at_delay(delay, e, bg);
}

/** @brief  Register event for a given delay.
 *  @param  delay  Delay time.
 *  @param  e      Event.
 *
 *  Triggers @a e when @a delay seconds have elapsed since @c recent(), or
 *  soon afterwards.
 */
inline void at_delay(double delay, event<> e, bool bg = false) {
    driver::main->at_delay(delay, e, bg);
}

/** @brief  Register event for a given delay.
 *  @param  delay  Delay time.
 *  @param  e      Event.
 *
 *  Triggers @a e when @a delay seconds have elapsed since @c recent(), or
 *  soon afterwards.
 */
inline void at_delay_sec(int delay, event<> e, bool bg = false) {
    driver::main->at_delay_sec(delay, e, bg);
}

/** @brief  Register event for a given delay.
 *  @param  delay  Delay time in milliseconds.
 *  @param  e      Event.
 *
 *  Triggers @a e when @a delay milliseconds have elapsed since @c recent(),
 *  or soon afterwards.
 */
inline void at_delay_msec(int delay, event<> e, bool bg = false) {
    driver::main->at_delay_msec(delay, e, bg);
}

/** @brief  Register event for a given delay.
 *  @param  delay  Delay time in microseconds.
 *  @param  e      Event.
 *
 *  Triggers @a e when @a delay microseconds have elapsed since @c recent(),
 *  or soon afterwards.
 */
inline void at_delay_usec(int delay, event<> e, bool bg = false) {
    driver::main->at_delay_usec(delay, e, bg);
}

/** @brief  Register event for signal occurrence.
 *  @param  signo  Signal number.
 *  @param  e      Event.
 *
 *  Triggers @a e soon after @a signo is received.  The signal @a signo
 *  is blocked while @a e is triggered and unblocked afterwards.
 */
inline void at_signal(int signo, event<> e) {
    driver::at_signal(signo, e);
}

/** @brief  Register event to trigger soon.
 *  @param  e  Event.
 *
 *  Triggers @a e the next time through the driver loop.
 */
inline void at_asap(event<> e) {
    driver::main->at_asap(e);
}

/** @brief  Register event to trigger before Tamer blocks.
 *  @param  e  Event.
 *
 *  Triggers @a e (and runs its closure) before the next blocking point.
 */
inline void at_preblock(event<> e) {
    driver::main->at_preblock(e);
}

namespace tamerpriv { extern time_type_t time_type; }
} // namespace tamer
#endif /* TAMER_DRIVER_HH */
