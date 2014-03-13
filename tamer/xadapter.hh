#ifndef TAMER_XADAPTER_HH
#define TAMER_XADAPTER_HH 1
/* Copyright (c) 2007-2014, Eddie Kohler
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
#include <string.h>
#include <errno.h>
#include <iterator>
namespace tamer {
namespace tamerpriv {

class with_helper_rendezvous : public functional_rendezvous,
			       public zero_argument_rendezvous_tag<with_helper_rendezvous> {
  public:
    with_helper_rendezvous(simple_event *e, int *s0, int v0)
	: functional_rendezvous(hook), e_(e), s0_(s0), v0_(v0) {
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
    event<> with = tamer::make_event(*r);
    e.at_trigger(with);
    return with;
}


template <typename T0, typename V0>
class bind_rendezvous : public functional_rendezvous,
			public zero_argument_rendezvous_tag<bind_rendezvous<T0, V0> > {
  public:
    bind_rendezvous(const event<T0> &e, const V0 &v0)
	: functional_rendezvous(hook), e_(e), v0_(v0) {
    }
  private:
    event<T0> e_;
    V0 v0_;
    static void hook(functional_rendezvous *, simple_event *, bool) TAMER_NOEXCEPT;
};

template <typename T0, typename V0>
void bind_rendezvous<T0, V0>::hook(functional_rendezvous *fr,
				   simple_event *, bool) TAMER_NOEXCEPT {
    bind_rendezvous *self = static_cast<bind_rendezvous *>(fr);
    self->e_.trigger(self->v0_);
    delete self;
}


template <typename T> struct decay { public: typedef T type; };
template <typename T0, typename T1> struct decay<T0(T1)> { public: typedef T0 (*type)(T1); };

template <typename S0, typename T0, typename F>
class map_rendezvous : public functional_rendezvous,
		       public zero_argument_rendezvous_tag<map_rendezvous<S0, T0, F> > {
  public:
    map_rendezvous(const F& f, const event<T0>& e)
	: functional_rendezvous(hook), f_(f), e_(e) {
    }
    S0& result0() {
	return s0_;
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


template <typename F, typename A1=void, typename A2=void>
class function_rendezvous;

template <typename F, typename A1, typename A2>
class function_rendezvous : public functional_rendezvous,
			    public zero_argument_rendezvous_tag<function_rendezvous<F, A1, A2> > {
  public:
    function_rendezvous(F f, A1 arg1, A2 arg2)
	: functional_rendezvous(hook), f_(TAMER_MOVE(f)),
	  arg1_(TAMER_MOVE(arg1)), arg2_(TAMER_MOVE(arg2)) {
    }
  private:
    F f_;
    A1 arg1_;
    A2 arg2_;
    static void hook(functional_rendezvous*, simple_event*, bool) TAMER_NOEXCEPT;
};

template <typename F, typename A1, typename A2>
void function_rendezvous<F, A1, A2>::hook(functional_rendezvous* fr,
					  simple_event*,
					  bool) TAMER_NOEXCEPT {
    function_rendezvous<F, A1, A2>* self = static_cast<function_rendezvous<F, A1, A2>*>(fr);
    // in case f_() refers to an event on this rendezvous:
    self->remove_waiting();
    self->f_(TAMER_MOVE(self->arg1_), TAMER_MOVE(self->arg2_));
    delete self;
}


template <typename F, typename A>
class function_rendezvous<F, A> : public functional_rendezvous,
				  public zero_argument_rendezvous_tag<function_rendezvous<F, A> > {
  public:
    function_rendezvous(F f, A arg)
	: functional_rendezvous(hook), f_(TAMER_MOVE(f)),
	  arg_(TAMER_MOVE(arg)) {
    }
  private:
    F f_;
    A arg_;
    static void hook(functional_rendezvous*, simple_event*, bool) TAMER_NOEXCEPT;
};

template <typename F, typename A>
void function_rendezvous<F, A, void>::hook(functional_rendezvous* fr,
					   simple_event*,
					   bool) TAMER_NOEXCEPT {
    function_rendezvous<F, A, void>* self = static_cast<function_rendezvous<F, A, void>*>(fr);
    // in case f_() refers to an event on this rendezvous:
    self->remove_waiting();
    self->f_(TAMER_MOVE(self->arg_));
    delete self;
}


template <typename F>
class function_rendezvous<F> : public functional_rendezvous,
			       public zero_argument_rendezvous_tag<function_rendezvous<F> > {
  public:
    function_rendezvous(F f)
	: functional_rendezvous(hook), f_(TAMER_MOVE(f)) {
    }
  private:
    F f_;
    static void hook(functional_rendezvous*, simple_event*, bool) TAMER_NOEXCEPT;
};

template <typename F>
void function_rendezvous<F, void, void>::hook(functional_rendezvous* fr,
					      simple_event*,
					      bool) TAMER_NOEXCEPT {
    function_rendezvous<F>* self = static_cast<function_rendezvous<F>*>(fr);
    // in case f_() refers to an event on this rendezvous:
    self->remove_waiting();
    self->f_();
    delete self;
}


template <typename T0 = void, typename T1 = void, typename T2 = void, typename T3 = void>
class distribute_rendezvous : public functional_rendezvous,
			      public zero_argument_rendezvous_tag<distribute_rendezvous<T0, T1, T2, T3> > {
  public:
    typedef tamer::event<T0, T1, T2, T3> event_type;
    inline distribute_rendezvous();
    inline ~distribute_rendezvous();
    inline void add(const event_type& e);
#if TAMER_HAVE_CXX_RVALUE_REFERENCES
    inline void add(event_type&& e);
#endif
    inline event<T0, T1, T2, T3> make_event() {
	return tamer::TAMER_MAKE_FN_ANNOTATED_EVENT(*this, vs_);
    }
    inline int nchildren() const {
        return nes_;
    }
  private:
    enum { nlocal = 2 };
    int nes_;
    int outstanding_;
    event_type* es_;
    tamer::value_pack<T0, T1, T2, T3> vs_;
    char local_es_[sizeof(event_type) * nlocal];
    static void hook(functional_rendezvous*, simple_event*, bool) TAMER_NOEXCEPT;
    static void clear_hook(void*);
    bool grow();
};

template <typename T0, typename T1, typename T2, typename T3>
inline distribute_rendezvous<T0, T1, T2, T3>::distribute_rendezvous()
    : functional_rendezvous(tamerpriv::rdistribute, hook),
      nes_(0), outstanding_(0), es_(reinterpret_cast<event_type*>(local_es_)) {
}

template <typename T0, typename T1, typename T2, typename T3>
inline distribute_rendezvous<T0, T1, T2, T3>::~distribute_rendezvous() {
    if (es_ != reinterpret_cast<event_type*>(local_es_))
        delete[] reinterpret_cast<char*>(es_);
}

template <typename T0, typename T1, typename T2, typename T3>
inline void distribute_rendezvous<T0, T1, T2, T3>::add(const event_type& e) {
    if (!e || (nes_ >= nlocal && (nes_ & (nes_ - 1)) == 0 && !grow()))
        return;
    new((void*) &es_[nes_]) event_type(TAMER_MOVE(e));
    if (es_[nes_].__get_simple()->shared()) {
        tamerpriv::simple_event::at_trigger(es_[nes_].__get_simple(), clear_hook, this);
        ++outstanding_;
    }
    ++nes_;
}

#if TAMER_HAVE_CXX_RVALUE_REFERENCES
template <typename T0, typename T1, typename T2, typename T3>
inline void distribute_rendezvous<T0, T1, T2, T3>::add(event_type&& e) {
    if (!e || (nes_ >= nlocal && (nes_ & (nes_ - 1)) == 0 && !grow()))
        return;
    new((void*) &es_[nes_]) event_type(TAMER_MOVE(e));
    if (es_[nes_].__get_simple()->shared()) {
        tamerpriv::simple_event::at_trigger(es_[nes_].__get_simple(), clear_hook, this);
        ++outstanding_;
    }
    ++nes_;
}
#endif

template <typename T0, typename T1, typename T2, typename T3>
bool distribute_rendezvous<T0, T1, T2, T3>::grow() {
    event_type* new_es = reinterpret_cast<event_type*>(new char[sizeof(event_type) * nes_ * 2]);
    if (new_es) {
        memcpy(new_es, es_, sizeof(event_type) * nes_);
        if (es_ != reinterpret_cast<event_type*>(local_es_))
            delete[] reinterpret_cast<char*>(es_);
        es_ = new_es;
        return true;
    } else
        return false;
}

template <typename T0, typename T1, typename T2, typename T3>
void distribute_rendezvous<T0, T1, T2, T3>::hook(functional_rendezvous* fr,
						 simple_event*,
						 bool values) TAMER_NOEXCEPT {
    distribute_rendezvous<T0, T1, T2, T3>* dr =
	static_cast<distribute_rendezvous<T0, T1, T2, T3>*>(fr);
    ++dr->outstanding_;		// keep memory around until we're done here
    if (values) {
        for (int i = 0; i != dr->nes_; ++i)
            dr->es_[i].trigger(dr->vs_);
    } else {
        for (int i = 0; i != dr->nes_; ++i)
            dr->es_[i].unblock();
    }
    if (--dr->outstanding_ == 0)
	delete dr;
}

template <typename T0, typename T1, typename T2, typename T3>
void distribute_rendezvous<T0, T1, T2, T3>::clear_hook(void* arg) {
    distribute_rendezvous<T0, T1, T2, T3>* dr =
	static_cast<distribute_rendezvous<T0, T1, T2, T3>*>(arg);
    if (--dr->outstanding_ == 0) {
	dr->remove_waiting();
	delete dr;
    }
}


template <typename C>
class push_back_rendezvous : public functional_rendezvous,
                             public zero_argument_rendezvous_tag<push_back_rendezvous<C> > {
  public:
    typedef typename C::value_type value_type;
    push_back_rendezvous(C& c)
	: functional_rendezvous(hook), c_(c) {
    }
    value_type& slot() {
        return slot_;
    }
  private:
    C& c_;
    value_type slot_;
    static void hook(functional_rendezvous*, simple_event*, bool) TAMER_NOEXCEPT;
};

template <typename C>
void push_back_rendezvous<C>::hook(functional_rendezvous* fr,
                                   simple_event*,
                                   bool values) TAMER_NOEXCEPT {
    push_back_rendezvous<C>* self = static_cast<push_back_rendezvous<C>*>(fr);
    self->remove_waiting();
    if (values)
        self->c_.push_back(TAMER_MOVE(self->slot_));
    delete self;
}


template <typename It>
class output_rendezvous : public functional_rendezvous,
                          public zero_argument_rendezvous_tag<output_rendezvous<It> > {
  public:
    typedef typename std::iterator_traits<It>::value_type value_type;
    output_rendezvous(It& it)
	: functional_rendezvous(hook), it_(it) {
    }
    value_type& slot() {
        return slot_;
    }
  private:
    It& it_;
    value_type slot_;
    static void hook(functional_rendezvous*, simple_event*, bool) TAMER_NOEXCEPT;
};

template <typename It>
void output_rendezvous<It>::hook(functional_rendezvous* fr,
                                 simple_event*,
                                 bool values) TAMER_NOEXCEPT {
    output_rendezvous<It>* self = static_cast<output_rendezvous<It>*>(fr);
    self->remove_waiting();
    if (values) {
        *self->it_ = TAMER_MOVE(self->slot_);
        ++self->it_;
    }
    delete self;
}

} // namespace tamerpriv
} // namespace tamer
#endif /* TAMER_XADAPTER_HH */
