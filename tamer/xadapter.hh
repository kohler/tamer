#ifndef TAMER_XADAPTER_HH
#define TAMER_XADAPTER_HH 1
#include <tamer/event.hh>
#include <tamer/driver.hh>
#include <vector>
#include <errno.h>
namespace tamer {
namespace tamerpriv {

template <typename F> class function_rendezvous : public abstract_rendezvous { public:

    function_rendezvous()
	: _f() {
    }

    template <typename X>
    function_rendezvous(X x)
	: _f(x) {
    }

    template <typename X, typename Y>
    function_rendezvous(X x, Y y)
	: _f(x, y) {
    }

    template <typename X, typename Y, typename Z>
    function_rendezvous(X x, Y y, Z z)
	: _f(x, y, z) {
    }

    ~function_rendezvous() {
    }

    enum function_type {
	triggerer, canceler
    };

    void add(simple_event *e, function_type t) {
	e->initialize(this, t);
    }

    void complete(uintptr_t rid) {
	// in case _f() refers to an event on this rendezvous:
	remove_all();
	if (rid == triggerer)
	    _f();
	delete this;
    }

  private:

    F _f;

};


template <typename T0> class bind_function { public:

    template <typename V0>
    bind_function(const event<T0> &ein, const V0 &v0)
	: _ein(ein), _v0(v0) {
    }

    void operator()() {
	_ein.trigger(_v0);
    }

    event<T0> _ein;
    T0 _v0;

};


template <typename T0> class assign_trigger_function { public:

    template <typename V0>
    assign_trigger_function(simple_event *e, T0 *s0, const V0 &v0)
	: _e(e), _s0(s0), _v0(v0) {
	_e->use();
    }

    ~assign_trigger_function() {
	_e->unuse();
    }

    void operator()() {
	if (_e->trigger()) {
	    *_s0 = _v0;
	}
    }

  private:

    simple_event *_e;
    T0 *_s0;
    T0 _v0;

    assign_trigger_function(const assign_trigger_function<T0> &);
    assign_trigger_function<T0> &operator=(const assign_trigger_function<T0> &);
    
};


inline void simple_event::at_trigger(const event<> &e)
{
    if (!_r || !e)
	e._e->trigger();
    else if (!_at_trigger) {
	_at_trigger = e._e;
	_at_trigger->use();
    } else {
	event<> comb = tamer::distribute(event<>::__take(_at_trigger), e);
	_at_trigger = comb._e;
	_at_trigger->use();
    }
}

}}
#endif /* TAMER_XADAPTER_HH */
