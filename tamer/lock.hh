#ifndef TAMER_LOCK_HH
#define TAMER_LOCK_HH 1
#include <tamer/event.hh>
namespace tamer {

/** @file <tamer/lock.hh>
 *  @brief  Classes for event-based synchronization.
 */

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
 *  <code>
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
 *         exclusive(1);
 *         shared(2);
 *         shared(3);
 *         exclusive(4);
 *         shared(5);
 *     }
 *  </code>
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
class mutex { public:

    /** @brief  Default constructor creates a mutex. */
    mutex()
	: _locked(0) {
    }

    /** @brief  Acquire the mutex for exclusive access.
     *  @param  done  Event triggered when the mutex is acquired.
     *
     *  The mutex must later be released with the release() method.
     */
    void acquire(const event<> &done) {
	acquire(-1, done);
    }

    /** @brief  Release a mutex acquired for exclusive access.
     *  @pre    The mutex must currently be acquired for exclusive access.
     *  @sa     acquire()
     */
    void release() {
	assert(_locked == -1);
	++_locked;
	wake();
    }

    /** @brief  Acquire the mutex for shared access.
     *  @param  done  Event triggered when the mutex is acquired.
     *
     *  The mutex must later be released with the release_shared() method.
     */
    void acquire_shared(const event<> &done) {
	acquire(1, done);
    }

    /** @brief  Release a mutex acquired for shared access.
     *  @pre    The mutex must currently be acquired for shared access.
     *  @sa     acquire_shared()
     */
    void release_shared() {
	assert(_locked != -1 && _locked != 0);
	--_locked;
	wake();
    }
    
  private:

    int _locked;
    tamerpriv::debuffer<event<> > _waiters;

    class closure__acquire__iQ_;
    void acquire(int shared, event<> done);
    void acquire(closure__acquire__iQ_ &, unsigned);

    void wake();
    
    mutex(const mutex &);
    mutex &operator=(const mutex &);
    
};

} /* namespace tamer */
#endif /* TAMER_LOCK_HH */
