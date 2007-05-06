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

template <typename T0> class bind_rendezvous : public abstract_rendezvous { public:

    template <typename V0>
    bind_rendezvous(const event<T0> &ein, const V0 &v0)
	: _ein(ein), _v0(v0)
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
	    _ein.trigger(_v0);
	else
	    _ein.cancel();
	if (success)
	    delete this;
    }
    
  private:

    event<T0> _ein;
    T0 _v0;

};

} /* namespace tamerpriv */


template <typename T0>
event<T0> ignore_slot(const event<> &e) {
    tamerpriv::unbind_rendezvous<T0> *ur = new tamerpriv::unbind_rendezvous<T0>(e);
    return event<T0>(*ur, 1, *((T0 *) 0));
}

template <typename T0, typename V0>
event<> bind(const event<T0> &e, const V0 &v0) {
    tamerpriv::bind_rendezvous<T0> *ur = new tamerpriv::bind_rendezvous<T0>(e, v0);
    return event<>(*ur, 1);
}


namespace outcome {
const int success = 0;
const int cancel = -ECANCELED;
const int timeout = -ETIMEDOUT;
const int signal = -EINTR;
}


namespace tamerpriv {

class cancel_adapter_rendezvous : public abstract_rendezvous { public:

    template <typename T0, typename T1, typename T2, typename T3>
    cancel_adapter_rendezvous(const event<T0, T1, T2, T3> &e, int *result)
	: _e(e.__get_simple()), _result(result)
    {
	_e->use();
	_e->at_cancel(event<>(*this, outcome::cancel));
    }

    ~cancel_adapter_rendezvous() {
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

class cancel_spread_rendezvous : public abstract_rendezvous { public:

    template <typename T0, typename T1, typename T2, typename T3>
    cancel_spread_rendezvous(const event<T0, T1, T2, T3> &e)
	: _e(e.__get_simple())
    {
	_e->use();
    }

    ~cancel_spread_rendezvous() {
	_e->unuse();
    }

    void add(simple_event *e) {
	e->initialize(this, 0);
    }
    
    void complete(uintptr_t, bool success) {
	if (success)
	    _e->complete(false);
	delete this;
    }
    
  private:

    simple_event *_e;

};

}


template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> with_timeout(const timeval &delay, event<T0, T1, T2, T3> e) {
    tamerpriv::cancel_adapter_rendezvous *r = new tamerpriv::cancel_adapter_rendezvous(e, 0);
    at_delay(delay, event<>(*r, outcome::timeout));
    return e.make_rebind(*r, outcome::success);
}

template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> with_timeout(const timeval &delay, event<T0, T1, T2, T3> e, int &result) {
    result = outcome::success;
    tamerpriv::cancel_adapter_rendezvous *r = new tamerpriv::cancel_adapter_rendezvous(e, &result);
    at_delay(delay, event<>(*r, outcome::timeout));
    return e.make_rebind(*r, outcome::success);
}

template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> with_timeout_sec(int delay, event<T0, T1, T2, T3> e) {
    timeval tv;
    tv.tv_sec = delay;
    tv.tv_usec = 0;
    return with_timeout(tv, e);
}

template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> with_timeout_sec(int delay, event<T0, T1, T2, T3> e, int &result) {
    timeval tv;
    tv.tv_sec = delay;
    tv.tv_usec = 0;
    return with_timeout(tv, e, result);
}

template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> with_timeout_msec(int delay, event<T0, T1, T2, T3> e) {
    timeval tv;
    tv.tv_sec = delay / 1000;
    tv.tv_usec = delay % 1000;
    return with_timeout(tv, e);
}

template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> with_timeout_msec(int delay, event<T0, T1, T2, T3> e, int &result) {
    timeval tv;
    tv.tv_sec = delay / 1000;
    tv.tv_usec = delay % 1000;
    return with_timeout(tv, e, result);
}


inline event<int> add_timeout(const timeval &delay, event<int> e) {
    tamerpriv::cancel_adapter_rendezvous *r = new tamerpriv::cancel_adapter_rendezvous(e, e.slot0());
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


template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> with_signal(int sig, event<T0, T1, T2, T3> e) {
    tamerpriv::cancel_adapter_rendezvous *r = new tamerpriv::cancel_adapter_rendezvous(e, 0);
    at_signal(sig, event<>(*r, outcome::signal));
    return e.make_rebind(*r, outcome::success);
}

template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> with_signal(int sig, event<T0, T1, T2, T3> e, int &result) {
    result = outcome::success;
    tamerpriv::cancel_adapter_rendezvous *r = new tamerpriv::cancel_adapter_rendezvous(e, &result);
    at_signal(sig, event<>(*r, outcome::signal));
    return e.make_rebind(*r, outcome::success);
}

template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> with_signal(const std::vector<int> &sig, event<T0, T1, T2, T3> e) {
    tamerpriv::cancel_adapter_rendezvous *r = new tamerpriv::cancel_adapter_rendezvous(e, 0);
    for (std::vector<int>::const_iterator i = sig.begin(); i != sig.end(); i++)
	at_signal(*i, event<>(*r, outcome::signal));
    return e.make_rebind(*r, outcome::success);
}

template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> with_signal(const std::vector<int> &sig, event<T0, T1, T2, T3> e, int &result) {
    result = outcome::success;
    tamerpriv::cancel_adapter_rendezvous *r = new tamerpriv::cancel_adapter_rendezvous(e, &result);
    for (std::vector<int>::const_iterator i = sig.begin(); i != sig.end(); i++)
	at_signal(*i, event<>(*r, outcome::signal));
    return e.make_rebind(*r, outcome::success);
}


inline event<int> add_signal(int sig, event<int> e) {
    tamerpriv::cancel_adapter_rendezvous *r = new tamerpriv::cancel_adapter_rendezvous(e, e.slot0());
    at_signal(sig, event<>(*r, outcome::signal));
    return e.make_rebind(*r, outcome::success);
}

inline event<int> add_signal(const std::vector<int> &sig, event<int> e) {
    tamerpriv::cancel_adapter_rendezvous *r = new tamerpriv::cancel_adapter_rendezvous(e, e.slot0());
    for (std::vector<int>::const_iterator i = sig.begin(); i != sig.end(); i++)
	at_signal(*i, event<>(*r, outcome::signal));
    return e.make_rebind(*r, outcome::success);
}


template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> with_cancel(event<T0, T1, T2, T3> e) {
    tamerpriv::cancel_adapter_rendezvous *r = new tamerpriv::cancel_adapter_rendezvous(e, 0);
    return e.make_rebind(*r, outcome::success);
}


template <typename T0, typename T1, typename T2, typename T3,
	  typename U0, typename U1, typename U2, typename U3>
inline event<T0, T1, T2, T3> spread_cancel(const event<U0, U1, U2, U3> &cancelee, event<T0, T1, T2, T3> e) {
    tamerpriv::cancel_spread_rendezvous *r = new tamerpriv::cancel_spread_rendezvous(cancelee);
    e.at_cancel(event<>(*r));
    return e;
}


inline event<int> add_cancel(event<int> e) {
    tamerpriv::cancel_adapter_rendezvous *r = new tamerpriv::cancel_adapter_rendezvous(e, e.slot0());
    return e.make_rebind(*r, outcome::success);
}


template <typename R, typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> make_cancel(R &r, event<T0, T1, T2, T3> e) {
    e.at_cancel(make_event(r));
    return e;
}

template <typename R, typename I0, typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> make_cancel(R &r, const I0 &i0, event<T0, T1, T2, T3> e) {
    e.at_cancel(make_event(r, i0));
    return e;
}

template <typename R, typename I0, typename I1, typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> make_cancel(R &r, const I0 &i0, const I1 &i1, event<T0, T1, T2, T3> e) {
    e.at_cancel(make_event(r, i0, i1));
    return e;
}

} /* namespace tamer */
#endif /* TAMER_ADAPTER_HH */
