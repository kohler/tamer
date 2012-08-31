#ifndef TAMER_LOCK_HH
#define TAMER_LOCK_HH 1
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
namespace tamer {

/** @file <tamer/lock.hh>
 *  @brief  Classes for event-based synchronization.
 */

class mutex {
  public:
    inline mutex();

    inline void acquire(event<> done);
    inline void release();

    inline void acquire_shared(event<> done);
    inline void release_shared();

  private:
    struct wait {
	wait **pprev;
	wait *next;
	event<> e;
    };

    int locked_;
    wait *wait_;
    wait **wait_tailp_;

    void acquire(int shared, event<> done);
    class closure__acquire__iQ_;
    void acquire(closure__acquire__iQ_ &);

    void wake() {
	if (wait_)
	    wait_->e.trigger();
    }

    mutex(const mutex &);
    mutex &operator=(const mutex &);
};

/** @brief  Default constructor creates a mutex. */
inline mutex::mutex()
    : locked_(0), wait_(), wait_tailp_(&wait_) {
}

/** @brief  Acquire the mutex for exclusive access.
 *  @param  done  Event triggered when the mutex is acquired.
 *
 *  The mutex must later be released with the release() method.
 */
inline void mutex::acquire(event<> done) {
    acquire(-1, done);
}

/** @brief  Release a mutex acquired for exclusive access.
 *  @pre    The mutex must currently be acquired for exclusive access.
 *  @sa     acquire()
 */
inline void mutex::release() {
    assert(locked_ == -1);
    ++locked_;
    wake();
}

/** @brief  Acquire the mutex for shared access.
 *  @param  done  Event triggered when the mutex is acquired.
 *
 *  The mutex must later be released with the release_shared() method.
 */
inline void mutex::acquire_shared(event<> done) {
    acquire(1, done);
}

/** @brief  Release a mutex acquired for shared access.
 *  @pre    The mutex must currently be acquired for shared access.
 *  @sa     acquire_shared()
 */
inline void mutex::release_shared() {
    assert(locked_ != -1 && locked_ != 0);
    --locked_;
    wake();
}

} /* namespace tamer */
#endif /* TAMER_LOCK_HH */
