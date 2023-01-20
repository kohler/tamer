// -*- mode: c++; related-file-name: "lock.hh" -*-
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
#include <tamer/lock.hh>
#include <tamer/adapter.hh>
namespace tamer {

/** @class mutex tamer/lock.hh <tamer/lock.hh>
 *  @brief  An event-based mutual-exclusion lock.
 *
 *  A mutex object is an event-based mutual-exclusion and/or read/write lock.
 *  Acquire the mutex with the acquire() method, which triggers its event<>
 *  argument when the mutex is acquired.  Later, release it with the release()
 *  method.
 *
 *  The mutex class also provides shared access, as for a read lock.  Acquire
 *  the mutex for shared access with the acquire_shared() method, and release
 *  it thereafter with the release_shared() method.  More than one callback
 *  function can acquire_shared() a mutex at once, but all the shared
 *  acquirers must release_shared() the mutex before a subsequent acquire()
 *  will succeed.
 *
 *  Acquire requests are served strictly in order of their arrival.  For
 *  example, the following code:
 *
 *  @code
 *     tamer::mutex m;
 *     tamed void exclusive(int which) {
 *         twait { m.acquire(make_event()); }
 *         printf("%d: acquired\n", which);
 *         twait { tamer::at_delay_sec(1, make_event()); }
 *         m.release();
 *         printf("%d: released\n", which);
 *     }
 *     tamed void shared(int which) {
 *         twait { m.acquire_shared(make_event()); }
 *         printf("%d: acquired shared\n", which);
 *         twait { tamer::at_delay_sec(1, make_event()); }
 *         m.release_shared();
 *         printf("%d: released shared\n", which);
 *     }
 *     int main(int argc, char *argv[]) {
 *         tamer::initialize();
 *         exclusive(1);
 *         shared(2);
 *         shared(3);
 *         exclusive(4);
 *         shared(5);
 *         tamer::loop();
 *     }
 *  @endcode
 *
 *  will generate this output:
 *
 *  <pre>
 *     1: acquired
 *     1: released
 *     2: acquired shared
 *     3: acquired shared
 *     2: released shared
 *     3: released shared
 *     4: acquired
 *     4: released
 *     5: acquired shared
 *     5: released shared
 *  </pre>
 */

tamed void mutex::acquire(int shared, event<> done) {
    tvars {
        rendezvous<> r;
        wait w;
    }

    if (!done) {
        return;
    }
    if (!wait_ && (shared > 0 ? locked_ != -1 : locked_ == 0)) {
        locked_ += shared;
        done.trigger();
        return;
    }

    done.at_trigger(make_event(r));
    w.e = make_event(r);
    w.pprev = wait_tailp_;
    *w.pprev = &w;
    wait_tailp_ = &w.next;
    w.next = 0;

    while (true) {
        twait(r);
        if (!done) {
            // canceling an exclusive lock may let a shared lock through,
            // so always count canceled locks as shared
            shared = 1;
            break;
        } else if (shared > 0 ? locked_ != -1 : locked_ == 0) {
            // obtained lock
            locked_ += shared;
            break;
        } else {
            // must try again
            w.e = make_event(r); // w is still on front
        }
    }

    done.trigger();             // lock was obtained (or done is empty)
    r.clear();                  // not interested in r events, so clean up
    *w.pprev = w.next;          // remove wait object from list
    if (w.next) {
        w.next->pprev = w.pprev;
    } else {
        wait_tailp_ = w.pprev;
    }
    if (shared > 0) {           // next waiter might also get a shared lock
        wake();
    }
}

} // namespace tamer
