#ifndef TAMER_LOCK_HH
#define TAMER_LOCK_HH 1
#include <tamer/event.hh>
namespace tamer {

class mutex { public:

    mutex()
	: _locked(0) {
    }

    void acquire(const event<> &done) {
	acquire(-1, done);
    }

    void release() {
	assert(_locked == 1);
	_locked--;
	wake();
    }
    
    void acquire_shared(const event<> &done) {
	acquire(1, done);
    }
    
    void release_shared() {
	assert(_locked < 0);
	_locked++;
	wake();
    }
    
  private:

    int _locked;
    tamerpriv::debuffer<event<> > _waiters;

    class acquire__closure; friend class acquire__closure;
    void acquire(int shared, event<> done);
    void acquire(acquire__closure &, unsigned);

    void wake();
    
    mutex(const mutex &);
    mutex &operator=(const mutex &);
    
};

} /* namespace tamer */
#endif /* TAMER_LOCK_HH */
