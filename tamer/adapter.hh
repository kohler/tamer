#ifndef TAMER_ADAPTER_HH
#define TAMER_ADAPTER_HH 1
#include <tamer/event.hh>
#include <tamer/driver.hh>
#include <vector>
#include <errno.h>
namespace tamer {
namespace tamerpriv {

template <typename T> class unbind_rendezvous : public abstract_rendezvous { public:

    unbind_rendezvous(const event<> &ein)
	: _ein(ein)
    {
	_ein.at_cancel(event<>(*this, 0));
    }

    void add(simple_event *e, uintptr_t what) {
	e->initialize(this, what);
    }
    
    void complete(uintptr_t rname, bool success) {
	if (success && rname)
	    _ein.trigger();
	else
	    _ein.cancel();
	if (success)
	    delete this;
    }

  private:

    event<> _ein;
    
};

template <typename T1> class bind_rendezvous : public abstract_rendezvous { public:

    template <typename X1>
    bind_rendezvous(const event<T1> &ein, const X1 &t1)
	: _ein(ein), _t1(t1)
    {
	_ein.at_cancel(event<>(*this, 0));
    }

    ~bind_rendezvous() {
    }

    void add(simple_event *e, uintptr_t what) {
	e->initialize(this, what);
    }
    
    void complete(uintptr_t rname, bool success) {
	if (success && rname)
	    _ein.trigger(_t1);
	else
	    _ein.cancel();
	if (success)
	    delete this;
    }
    
  private:

    event<T1> _ein;
    T1 _t1;

};

} /* namespace tamerpriv */


template <typename T1>
event<T1> ignore_slot(const event<> &e) {
    tamerpriv::unbind_rendezvous<T1> *ur = new tamerpriv::unbind_rendezvous<T1>(e);
    return event<T1>(*ur, 1, *((T1 *) 0));
}

template <typename T1, typename X1>
event<> bind(const event<T1> &e, const X1 &t1) {
    tamerpriv::bind_rendezvous<T1> *ur = new tamerpriv::bind_rendezvous<T1>(e, t1);
    return event<>(*ur, 1);
}


namespace outcome {
const int success = 0;
const int cancel = -ECANCELED;
const int timeout = -ETIMEDOUT;
const int signal = -EINTR;
}


namespace tamerpriv {

class canceler_rendezvous : public abstract_rendezvous { public:

    template <typename T1, typename T2, typename T3, typename T4>
    canceler_rendezvous(const event<T1, T2, T3, T4> &e, int *result)
	: _e(e.__get_simple()), _result(result)
    {
	_e->use();
	_e->at_cancel(event<>(*this, outcome::cancel));
    }

    ~canceler_rendezvous() {
	_e->unuse();
    }

    void add(simple_event *e, uintptr_t what) {
	e->initialize(this, what);
    }
    
    void complete(uintptr_t rname, bool success) {
	if (success || rname == (uintptr_t) outcome::success) {
	    if (_result)
		*_result = (success ? rname : outcome::cancel);
	    _e->complete(true);
	    delete this;
	}
    }
    
  private:

    simple_event *_e;
    int *_result;

};

}


template <typename T1, typename T2, typename T3, typename T4>
inline event<T1, T2, T3, T4> with_timeout(const timeval &delay, event<T1, T2, T3, T4> e) {
    tamerpriv::canceler_rendezvous *r = new tamerpriv::canceler_rendezvous(e, 0);
    at_delay(delay, event<>(*r, outcome::timeout));
    return e.make_rebind(*r, outcome::success);
}

template <typename T1, typename T2, typename T3, typename T4>
inline event<T1, T2, T3, T4> with_timeout(const timeval &delay, event<T1, T2, T3, T4> e, int &result) {
    result = outcome::success;
    tamerpriv::canceler_rendezvous *r = new tamerpriv::canceler_rendezvous(e, &result);
    at_delay(delay, event<>(*r, outcome::timeout));
    return e.make_rebind(*r, outcome::success);
}

template <typename T1, typename T2, typename T3, typename T4>
inline event<T1, T2, T3, T4> with_timeout_sec(int delay, event<T1, T2, T3, T4> e) {
    timeval tv;
    tv.tv_sec = delay;
    tv.tv_usec = 0;
    return with_timeout(tv, e);
}

template <typename T1, typename T2, typename T3, typename T4>
inline event<T1, T2, T3, T4> with_timeout_sec(int delay, event<T1, T2, T3, T4> e, int &result) {
    timeval tv;
    tv.tv_sec = delay;
    tv.tv_usec = 0;
    return with_timeout(tv, e, result);
}

template <typename T1, typename T2, typename T3, typename T4>
inline event<T1, T2, T3, T4> with_timeout_msec(int delay, event<T1, T2, T3, T4> e) {
    timeval tv;
    tv.tv_sec = delay / 1000;
    tv.tv_usec = delay % 1000;
    return with_timeout(tv, e);
}

template <typename T1, typename T2, typename T3, typename T4>
inline event<T1, T2, T3, T4> with_timeout_msec(int delay, event<T1, T2, T3, T4> e, int &result) {
    timeval tv;
    tv.tv_sec = delay / 1000;
    tv.tv_usec = delay % 1000;
    return with_timeout(tv, e, result);
}


inline event<int> add_timeout(const timeval &delay, event<int> e) {
    tamerpriv::canceler_rendezvous *r = new tamerpriv::canceler_rendezvous(e, e.slot1());
    at_delay(delay, event<>(*r, outcome::timeout));
    return e.make_rebind(*r, outcome::success);
}

inline event<int> add_timeout_sec(int delay, event<int> e) {
    timeval tv;
    tv.tv_sec = delay;
    tv.tv_usec = 0;
    return add_timeout(tv, e);
}

inline event<int> add_timeout_msec(int delay, event<int> e) {
    timeval tv;
    tv.tv_sec = delay / 1000;
    tv.tv_usec = delay % 1000;
    return add_timeout(tv, e);
}


template <typename T1, typename T2, typename T3, typename T4>
inline event<T1, T2, T3, T4> with_signal(int sig, event<T1, T2, T3, T4> e) {
    tamerpriv::canceler_rendezvous *r = new tamerpriv::canceler_rendezvous(e, 0);
    at_signal(sig, event<>(*r, outcome::signal));
    return e.make_rebind(*r, outcome::success);
}

template <typename T1, typename T2, typename T3, typename T4>
inline event<T1, T2, T3, T4> with_signal(int sig, event<T1, T2, T3, T4> e, int &result) {
    result = outcome::success;
    tamerpriv::canceler_rendezvous *r = new tamerpriv::canceler_rendezvous(e, &result);
    at_signal(sig, event<>(*r, outcome::signal));
    return e.make_rebind(*r, outcome::success);
}

template <typename T1, typename T2, typename T3, typename T4>
inline event<T1, T2, T3, T4> with_signal(const std::vector<int> &sig, event<T1, T2, T3, T4> e) {
    tamerpriv::canceler_rendezvous *r = new tamerpriv::canceler_rendezvous(e, 0);
    for (std::vector<int>::const_iterator i = sig.begin(); i != sig.end(); i++)
	at_signal(*i, event<>(*r, outcome::signal));
    return e.make_rebind(*r, outcome::success);
}

template <typename T1, typename T2, typename T3, typename T4>
inline event<T1, T2, T3, T4> with_signal(const std::vector<int> &sig, event<T1, T2, T3, T4> e, int &result) {
    result = outcome::success;
    tamerpriv::canceler_rendezvous *r = new tamerpriv::canceler_rendezvous(e, &result);
    for (std::vector<int>::const_iterator i = sig.begin(); i != sig.end(); i++)
	at_signal(*i, event<>(*r, outcome::signal));
    return e.make_rebind(*r, outcome::success);
}


inline event<int> add_signal(int sig, event<int> e) {
    tamerpriv::canceler_rendezvous *r = new tamerpriv::canceler_rendezvous(e, e.slot1());
    at_signal(sig, event<>(*r, outcome::signal));
    return e.make_rebind(*r, outcome::success);
}

inline event<int> add_signal(const std::vector<int> &sig, event<int> e) {
    tamerpriv::canceler_rendezvous *r = new tamerpriv::canceler_rendezvous(e, e.slot1());
    for (std::vector<int>::const_iterator i = sig.begin(); i != sig.end(); i++)
	at_signal(*i, event<>(*r, outcome::signal));
    return e.make_rebind(*r, outcome::success);
}


template <typename T1, typename T2, typename T3, typename T4>
inline event<T1, T2, T3, T4> with_cancel(event<T1, T2, T3, T4> e) {
    tamerpriv::canceler_rendezvous *r = new tamerpriv::canceler_rendezvous(e, 0);
    return e.make_rebind(*r, outcome::success);
}


inline event<int> add_cancel(event<int> e) {
    tamerpriv::canceler_rendezvous *r = new tamerpriv::canceler_rendezvous(e, e.slot1());
    return e.make_rebind(*r, outcome::success);
}


template <typename R, typename T1, typename T2, typename T3, typename T4>
inline event<T1, T2, T3, T4> make_cancel(R &r, event<T1, T2, T3, T4> e) {
    e.at_cancel(make_event(r));
    return e;
}

template <typename R, typename I1, typename T1, typename T2, typename T3, typename T4>
inline event<T1, T2, T3, T4> make_cancel(R &r, const I1 &i1, event<T1, T2, T3, T4> e) {
    e.at_cancel(make_event(r, i1));
    return e;
}

template <typename R, typename I1, typename I2, typename T1, typename T2, typename T3, typename T4>
inline event<T1, T2, T3, T4> make_cancel(R &r, const I1 &i1, const I2 &i2, event<T1, T2, T3, T4> e) {
    e.at_cancel(make_event(r, i1, i2));
    return e;
}

} /* namespace tamer */
#endif /* TAMER_ADAPTER_HH */
