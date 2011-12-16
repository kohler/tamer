#ifndef TAMER_XADAPTER_HH
#define TAMER_XADAPTER_HH 1
/* Copyright (c) 2007, Eddie Kohler
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

    void complete(uintptr_t) {
	if (*_e) {
	    *_s0 = _v0;
	    _e->trigger(false);
	} else
	    *_s0 = int();
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

    void complete(uintptr_t) {
	_e.trigger(_v0);
	delete this;
    }

  private:

    event<T0> _e;
    V0 _v0;

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


inline void simple_event::at_trigger(const event<> &e)
{
    if (!_r || !e)
	e._e->trigger(false);
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
