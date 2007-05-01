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
event<T1> unbind(const event<> &e) {
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

class connector_closure : public closure { public:

    template <typename T1, typename T2, typename T3, typename T4>
    connector_closure(const event<T1, T2, T3, T4> &e, int *result = 0)
	: _e(e.__get_simple()), _result(result) {
	_e->use();
    }

    ~connector_closure() {
	_e->unuse();
    }

    void _closure__activate(unsigned) {
	int x;
	if (!_r.join(x)) {
	    _r.block(*this, 0);
	    return;
	}
	_e->complete(x == outcome::success);
	if (_result && x != outcome::success)
	    *_result = x;
    }

    rendezvous<int> _r;
    simple_event *_e;
    int *_result; 

};

}


template <typename T1, typename T2, typename T3, typename T4>
inline event<T1, T2, T3, T4> with_timeout(const timeval &delay, event<T1, T2, T3, T4> e) {
    tamerpriv::connector_closure *c = new tamerpriv::connector_closure(e, 0);
    event<T1, T2, T3, T4> ret_e = e.make_rebind(c->_r, outcome::success);
    ret_e.at_cancel(make_event(c->_r, outcome::cancel));
    at_delay(delay, make_event(c->_r, outcome::timeout));
    c->_closure__activate(0);
    c->unuse();
    return ret_e;
}

template <typename T1, typename T2, typename T3, typename T4>
inline event<T1, T2, T3, T4> with_timeout(const timeval &delay, event<T1, T2, T3, T4> e, int &result) {
    result = outcome::success;
    tamerpriv::connector_closure *c = new tamerpriv::connector_closure(e, &result);
    event<T1, T2, T3, T4> ret_e = e.make_rebind(c->_r, outcome::success);
    ret_e.at_cancel(make_event(c->_r, outcome::cancel));
    at_delay(delay, make_event(c->_r, outcome::timeout));
    c->_closure__activate(0);
    c->unuse();
    return ret_e;
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


template <typename T1, typename T2, typename T3, typename T4>
inline event<T1, T2, T3, T4> with_signal(int sig, event<T1, T2, T3, T4> e) {
    tamerpriv::connector_closure *c = new tamerpriv::connector_closure(e, 0);
    event<T1, T2, T3, T4> ret_e = e.make_rebind(c->_r, outcome::success);
    ret_e.at_cancel(make_event(c->_r, outcome::cancel));
    at_signal(sig, make_event(c->_r, outcome::signal));
    c->_closure__activate(0);
    c->unuse();
    return ret_e;
}

template <typename T1, typename T2, typename T3, typename T4>
inline event<T1, T2, T3, T4> with_signal(int sig, event<T1, T2, T3, T4> e, int &result) {
    result = outcome::success;
    tamerpriv::connector_closure *c = new tamerpriv::connector_closure(e, &result);
    event<T1, T2, T3, T4> ret_e = e.make_rebind(c->_r, outcome::success);
    ret_e.at_cancel(make_event(c->_r, outcome::cancel));
    at_signal(sig, make_event(c->_r, outcome::signal));
    c->_closure__activate(0);
    c->unuse();
    return ret_e;
}

template <typename T1, typename T2, typename T3, typename T4>
inline event<T1, T2, T3, T4> with_signal(const std::vector<int> &sig, event<T1, T2, T3, T4> e) {
    tamerpriv::connector_closure *c = new tamerpriv::connector_closure(e, 0);
    event<T1, T2, T3, T4> ret_e = e.make_rebind(c->_r, outcome::success);
    ret_e.at_cancel(make_event(c->_r, outcome::cancel));
    for (std::vector<int>::const_iterator i = sig.begin(); i != sig.end(); i++)
	at_signal(*i, make_event(c->_r, outcome::signal));
    c->_closure__activate(0);
    c->unuse();
    return ret_e;
}

template <typename T1, typename T2, typename T3, typename T4>
inline event<T1, T2, T3, T4> with_signal(const std::vector<int> &sig, event<T1, T2, T3, T4> e, int &result) {
    result = outcome::success;
    tamerpriv::connector_closure *c = new tamerpriv::connector_closure(e, &result);
    event<T1, T2, T3, T4> ret_e = e.make_rebind(c->_r, outcome::success);
    ret_e.at_cancel(make_event(c->_r, outcome::cancel));
    for (std::vector<int>::const_iterator i = sig.begin(); i != sig.end(); i++)
	at_signal(*i, make_event(c->_r, outcome::signal));
    c->_closure__activate(0);
    c->unuse();
    return ret_e;
}

} /* namespace tamer */
#endif /* TAMER_TAME_ADAPTER_HH */
