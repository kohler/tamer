#ifndef TAMER_XADAPTER_HH
#define TAMER_XADAPTER_HH 1
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
    
    void complete(uintptr_t rid, bool success) {
	if (success && rid)
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
    
    void complete(uintptr_t rid, bool success) {
	if (success && rid)
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

class cancel_adapter_rendezvous : public abstract_rendezvous { public:

    template <typename T0, typename T1, typename T2, typename T3>
    cancel_adapter_rendezvous(const event<T0, T1, T2, T3> &e, int *result)
	: _e(e.__get_simple()), _result(result)
    {
	_e->use();
	_e->at_cancel(event<>(*this, -ECANCELED));
    }

    ~cancel_adapter_rendezvous() {
	_e->unuse();
    }

    void add(simple_event *e, uintptr_t what) {
	e->initialize(this, what);
    }
    
    void complete(uintptr_t rid, bool success) {
	if (success || rid == (uintptr_t) 0) {
	    if (_result)
		*_result = (success ? (int) rid : -ECANCELED);
	    _e->complete(true);
	    delete this;
	}
    }
    
  private:

    simple_event *_e;
    int *_result;

};

class canceler_rendezvous : public abstract_rendezvous { public:

    template <typename T0, typename T1, typename T2, typename T3>
    canceler_rendezvous(const event<T0, T1, T2, T3> &e)
	: _e(e.__get_simple())
    {
	_e->use();
	_e->at_cancel(event<>(*this));
    }

    ~canceler_rendezvous() {
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

inline void simple_event::at_cancel(const event<> &e)
{
    if (!_r)
	e._e->complete(true);
    else if (!_canceler) {
	_canceler = e._e;
	_canceler->use();
    } else {
	event<> comb = tamer::distribute(event<>::__take(_canceler), e);
	_canceler = comb._e;
	_canceler->use();
    }
}

}}
#endif /* TAMER_XADAPTER_HH */
