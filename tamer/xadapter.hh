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

class with_helper_rendezvous : public functional_rendezvous {
  public:
    with_helper_rendezvous(simple_event *e, int *s0, int v0)
	: functional_rendezvous(hook), e_(e), s0_(s0), v0_(v0) {
    }
    void add(simple_event *e) {
	e->initialize(this, 0);
    }
  private:
    simple_event *e_;		// An e_->at_trigger() holds a reference
    int *s0_;			// to this rendezvous, so this rendezvous
    int v0_;			// doesn't hold a reference to e_.
    static void hook(functional_rendezvous *fr, simple_event *, bool) TAMER_NOEXCEPT;
};

template <typename T0, typename T1, typename T2, typename T3>
inline event<> with_helper(event<T0, T1, T2, T3> e, int *result, int value) {
    with_helper_rendezvous *r = new with_helper_rendezvous(e.__get_simple(), result, value);
    event<> with(*r);
    e.at_trigger(with);
    return with;
}


template <typename T0, typename V0>
class bind_rendezvous : public functional_rendezvous {
  public:
    bind_rendezvous(const event<T0> &e, const V0 &v0)
	: functional_rendezvous(hook), e_(e), v0_(v0) {
    }
    void add(simple_event *e) {
	e->initialize(this, 0);
    }
  private:
    event<T0> e_;
    V0 v0_;
    static void hook(functional_rendezvous *, simple_event *, bool);
};

template <typename T0, typename V0>
void bind_rendezvous<T0, V0>::hook(functional_rendezvous *fr,
				   simple_event *, bool) {
    bind_rendezvous *self = static_cast<bind_rendezvous *>(fr);
    self->e_.trigger(self->v0_);
    delete self;
}


template <typename T> struct decay { public: typedef T type; };
template <typename T0, typename T1> struct decay<T0(T1)> { public: typedef T0 (*type)(T1); };

template <typename S0, typename T0, typename F>
class map_rendezvous : public functional_rendezvous {
  public:
    map_rendezvous(const F &f, const event<T0> &e)
	: functional_rendezvous(hook), f_(f), e_(e) {
    }
    S0 &slot0() {
	return s0_;
    }
    void add(simple_event *e) {
	e->initialize(this, 0);
    }
  private:
    S0 s0_;
    typename decay<F>::type f_;
    event<T0> e_;
    static void hook(functional_rendezvous *, simple_event *, bool) TAMER_NOEXCEPT;
};

template <typename S0, typename T0, typename F>
void map_rendezvous<S0, T0, F>::hook(functional_rendezvous *fr,
				     simple_event *, bool values) TAMER_NOEXCEPT {
    map_rendezvous *self = static_cast<map_rendezvous *>(fr);
    if (values)
	self->e_.trigger(self->f_(self->s0_));
    else if (self->e_)
	self->e_.unblocker().trigger();
    delete self;
}


template <typename F>
class function_rendezvous : public functional_rendezvous {
  public:
    function_rendezvous()
	: functional_rendezvous(hook), f_() {
    }
    template <typename X>
    function_rendezvous(X x)
	: functional_rendezvous(hook), f_(x) {
    }
    template <typename X, typename Y>
    function_rendezvous(X x, Y y)
	: functional_rendezvous(hook), f_(x, y) {
    }
    template <typename X, typename Y, typename Z>
    function_rendezvous(X x, Y y, Z z)
	: functional_rendezvous(hook), f_(x, y, z) {
    }
    void add(simple_event *e) {
	e->initialize(this, 0);
    }
  private:
    F f_;
    static void hook(functional_rendezvous *, simple_event *, bool) TAMER_NOEXCEPT;
};

template <typename F>
void function_rendezvous<F>::hook(functional_rendezvous *fr,
				  simple_event *, bool) TAMER_NOEXCEPT {
    function_rendezvous *self = static_cast<function_rendezvous *>(fr);
    // in case _f() refers to an event on this rendezvous:
    self->remove_waiting();
    self->f_();
    delete self;
}


inline void simple_event::at_trigger(simple_event *x, const event<> &at_e) {
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
