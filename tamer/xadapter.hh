#ifndef TAMER_XADAPTER_HH
#define TAMER_XADAPTER_HH 1
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
#include <tamer/driver.hh>
#include <vector>
#include <errno.h>
namespace tamer {
namespace tamerpriv {

class with_helper_rendezvous : public abstract_rendezvous { public:

    with_helper_rendezvous(simple_event *e, int *s0, int v0)
	: _e(e), _s0(s0), _v0(v0) {
    }
    ~with_helper_rendezvous() {
    }

    void add(simple_event *e) {
	e->initialize(this, 0);
    }

    void complete(simple_event *e, bool) {
	e->precomplete();
	if (*_e) {
	    *_s0 = _v0;
	    _e->simple_trigger(false);
	    _e = 0;
	} else
	    *_s0 = int();
	e->postcomplete();
	delete this;
    }

  private:

    simple_event *_e;
    int *_s0;
    int _v0;

};

template <typename T0, typename T1, typename T2, typename T3>
inline event<> with_helper(event<T0, T1, T2, T3> e, int *result, int value) {
    with_helper_rendezvous *r = new with_helper_rendezvous(e.__get_simple(), result, value);
    event<> with(*r);
    e.at_trigger(with);
    return with;
}


template <typename T0, typename V0>
class bind_rendezvous : public abstract_rendezvous { public:

    bind_rendezvous(const event<T0> &e, const V0 &v0)
	: _e(e), _v0(v0) {
    }
    ~bind_rendezvous() {
    }

    void add(simple_event *e) {
	e->initialize(this, 0);
    }

    void complete(simple_event *e, bool) {
	e->precomplete();
	_e.trigger(_v0);
	e->postcomplete();
	delete this;
    }

  private:

    event<T0> _e;
    V0 _v0;

};


template <typename T> struct decay { public: typedef T type; };
template <typename T0, typename T1> struct decay<T0(T1)> { public: typedef T0 (*type)(T1); };

template <typename S0, typename T0, typename F>
class map_rendezvous : public abstract_rendezvous { public:

    map_rendezvous(const F &f, const event<T0> &e)
	: _f(f), _e(e) {
    }
    ~map_rendezvous() {
    }
    S0 &slot0() {
	return _s0;
    }

    void add(simple_event *e) {
	e->initialize(this, 0);
    }

    void complete(simple_event *e, bool values) {
	e->precomplete();
	if (values)
	    _e.trigger(_f(_s0));
	else if (_e)
	    _e.unblocker().trigger();
	e->postcomplete();
	delete this;
    }

  private:

    S0 _s0;
    typename decay<F>::type _f;
    event<T0> _e;

};


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

    void add(simple_event *e) {
	e->initialize(this, 0);
    }

    void complete(simple_event *e, bool) {
	e->precomplete();
	// in case _f() refers to an event on this rendezvous:
	remove_all();
	_f();
	e->postcomplete();
	delete this;
    }

  private:

    F _f;

};


inline void simple_event::at_trigger(simple_event *x, const event<> &at_e)
{
    if (!at_e)
	/* ignore */;
    else if (!x || !x->_r) {
	at_e._e->simple_trigger(false);
	at_e._e = 0;
    } else if (!x->_at_trigger) {
	x->_at_trigger = at_e._e;
	use(x->_at_trigger);
    } else
	x->_at_trigger =
	    tamer::distribute(event<>::__make(x->_at_trigger), at_e)
	    .__take_simple();
}

}}
#endif /* TAMER_XADAPTER_HH */
