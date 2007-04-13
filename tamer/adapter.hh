#ifndef TAMER_TAME_ADAPTER_HH
#define TAMER_TAME_ADAPTER_HH 1
#include "tame_event.hh"
#include "tame_driver.hh"
#include <vector>
namespace tame {

template <typename T> class _unbind_rendezvous : public _rendezvous_superbase { public:

    _unbind_rendezvous(const event<> &ein)
	: _ein(ein), _eout(this, ubtrigger)
    {
	_ein.at_cancel(event<>(this, ubcancel));
    }

    void _complete(uintptr_t rname, bool success) {
	if (success) {
	    if (rname == ubcancel)
		_eout.cancel();
	    else
		_ein.trigger();
	    delete this;
	}
    }

    const event<T> &eout() const {
	return _eout;
    }

  private:

    event<> _ein;
    event<T> _eout;
    
    enum { ubcancel = 0, ubtrigger = 1 };
    
};

template <typename T1> class _bind_rendezvous : public _rendezvous_superbase { public:

    template <typename X1>
    _bind_rendezvous(const event<T1> &ein, const X1 &t1)
	: _ein(ein), _eout(this, ubtrigger), _t1(t1)
    {
	_ein.at_cancel(event<>(this, ubcancel));
    }

    ~_bind_rendezvous() {
    }

    void _complete(uintptr_t rname, bool success) {
	if (success) {
	    if (rname == ubcancel)
		_eout.cancel();
	    else
		_ein.trigger(_t1);
	    delete this;
	}
    }

    const event<> &eout() const {
	return _eout;
    }
    
  private:

    event<T1> _ein;
    event<> _eout;
    T1 _t1;

    enum { ubcancel = 0, ubtrigger = 1 };
    
};

template <typename T1>
const event<T1> &unbind(const event<> &e) {
    _unbind_rendezvous<T1> *ur = new _unbind_rendezvous<T1>(e);
    return ur->eout();
}

template <typename T1, typename X1>
const event<> &bind(const event<T1> &e, const X1 &t1) {
    _bind_rendezvous<T1> *ur = new _bind_rendezvous<T1>(e, t1);
    return ur->eout();
}




class _connector_closure : public _closure_base { public:

    template <typename T1, typename T2, typename T3, typename T4>
    _connector_closure(const event<T1, T2, T3, T4> &e, int *result = 0)
	: _e(e.__superbase()), _result(result) {
	_e->use();
    }

    ~_connector_closure() {
	_e->unuse();
    }

    void _closure__activate(unsigned) {
	int x;
	if (!_r.join(x)) {
	    _r._block(*this, 0);
	    return;
	}
	_e->complete(x == 1);
	if (_result)
	    *_result = x;
    }

    rendezvous<int> _r;
    _event_superbase *_e;
    int *_result; 

};


template <typename T1, typename T2, typename T3, typename T4>
inline event<T1, T2, T3, T4> with_timeout(const timeval &delay, event<T1, T2, T3, T4> e) {
    _connector_closure *c = new _connector_closure(e, 0);
    event<T1, T2, T3, T4> ret_e = e.rebind(c->_r, 1);
    ret_e.at_cancel(make_event(c->_r, 0));
    at_delay(delay, make_event(c->_r, 0));
    c->_closure__activate(0);
    c->unuse();
    return ret_e;
}

template <typename T1, typename T2, typename T3, typename T4>
inline event<T1, T2, T3, T4> with_timeout(const timeval &delay, event<T1, T2, T3, T4> e, int &result) {
    _connector_closure *c = new _connector_closure(e, &result);
    event<T1, T2, T3, T4> ret_e = e.rebind(c->_r, 1);
    ret_e.at_cancel(make_event(c->_r, 0));
    at_delay(delay, make_event(c->_r, 0));
    c->_closure__activate(0);
    c->unuse();
    return ret_e;
}

template <typename T1, typename T2, typename T3, typename T4>
inline event<T1, T2, T3, T4> with_signal(int sig, event<T1, T2, T3, T4> e) {
    _connector_closure *c = new _connector_closure(e, 0);
    event<T1, T2, T3, T4> ret_e = e.rebind(c->_r, 1);
    ret_e.at_cancel(make_event(c->_r, 0));
    at_signal(sig, make_event(c->_r, 0));
    c->_closure__activate(0);
    c->unuse();
    return ret_e;
}

template <typename T1, typename T2, typename T3, typename T4>
inline event<T1, T2, T3, T4> with_signal(int sig, event<T1, T2, T3, T4> e, int &result) {
    _connector_closure *c = new _connector_closure(e, &result);
    event<T1, T2, T3, T4> ret_e = e.rebind(c->_r, 1);
    ret_e.at_cancel(make_event(c->_r, 0));
    at_signal(sig, make_event(c->_r, 0));
    c->_closure__activate(0);
    c->unuse();
    return ret_e;
}

template <typename T1, typename T2, typename T3, typename T4>
inline event<T1, T2, T3, T4> with_signal(const std::vector<int> &sig, event<T1, T2, T3, T4> e) {
    _connector_closure *c = new _connector_closure(e, 0);
    event<T1, T2, T3, T4> ret_e = e.rebind(c->_r, 1);
    ret_e.at_cancel(make_event(c->_r, 0));
    for (std::vector<int>::const_iterator i = sig.begin(); i != sig.end(); i++)
	at_signal(*i, make_event(c->_r, 0));
    c->_closure__activate(0);
    c->unuse();
    return ret_e;
}

template <typename T1, typename T2, typename T3, typename T4>
inline event<T1, T2, T3, T4> with_signal(const std::vector<int> &sig, event<T1, T2, T3, T4> e, int &result) {
    _connector_closure *c = new _connector_closure(e, &result);
    event<T1, T2, T3, T4> ret_e = e.rebind(c->_r, 1);
    ret_e.at_cancel(make_event(c->_r, 0));
    for (std::vector<int>::const_iterator i = sig.begin(); i != sig.end(); i++)
	at_signal(*i, make_event(c->_r, 0));
    c->_closure__activate(0);
    c->unuse();
    return ret_e;
}

}
#endif /* TAMER_TAME_ADAPTER_HH */
