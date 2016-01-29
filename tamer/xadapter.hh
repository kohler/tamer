#ifndef TAMER_XADAPTER_HH
#define TAMER_XADAPTER_HH 1
/* Copyright (c) 2007-2015, Eddie Kohler
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

typedef union {
    int i;
    bool b;
} placeholder_buffer_type;
extern placeholder_buffer_type placeholder_buffer;

class with_helper_rendezvous : public functional_rendezvous,
                               public zero_argument_rendezvous_tag<with_helper_rendezvous> {
  public:
    with_helper_rendezvous(simple_event *e, int *s0, int v0)
        : functional_rendezvous(hook), e_(e), s0_(s0), v0_(v0) {
    }
  private:
    simple_event *e_;           // An e_->at_trigger() holds a reference
    int *s0_;                   // to this rendezvous, so this rendezvous
    int v0_;                    // doesn't hold a reference to e_.
    static void hook(functional_rendezvous *fr, simple_event *, bool) TAMER_NOEXCEPT;
};

template <typename... TS>
inline event<> with_helper(event<TS...> e, int *result, int value) {
    with_helper_rendezvous *r = new with_helper_rendezvous(e.__get_simple(), result, value);
    event<> with = tamer::make_event(*r);
    e.at_trigger(with);
    return with;
}


template <size_t I, typename VI, typename... TS>
class bind_rendezvous : public functional_rendezvous,
                        public zero_argument_rendezvous_tag<bind_rendezvous<I, VI, TS...> > {
  public:
    bind_rendezvous(const event<TS...>& e, VI vi)
        : functional_rendezvous(hook), e_(e), vi_(TAMER_MOVE(vi)) {
    }
  private:
    event<TS...> e_;
    VI vi_;
    static void hook(functional_rendezvous*, simple_event*, bool) TAMER_NOEXCEPT;
};

template <size_t I, typename VI, typename... TS>
void bind_rendezvous<I, VI, TS...>::hook(functional_rendezvous *fr, simple_event*, bool) TAMER_NOEXCEPT {
    bind_rendezvous<I, VI, TS...>* self = static_cast<bind_rendezvous<I, VI, TS...>*>(fr);
    self->e_.template set_result<I>(self->vi_);
    if (simple_event* se = self->e_.__release_simple())
        if (*se)
            se->simple_trigger(true);
    delete self;
}


template <typename T0>
struct rebinder {
    static event<T0> make(event<> e) {
        T0* v0 = new T0;
        simple_event::at_trigger(e.__get_simple(), deleter, v0);
        return event<T0>(TAMER_MOVE(e), *v0);
    }
    static void deleter(void* x) {
        delete static_cast<T0*>(x);
    }
};

template <>
struct rebinder<int> {
    static event<int> make(event<> e) {
        return event<int>(TAMER_MOVE(e), placeholder_buffer.i);
    }
};

template <>
struct rebinder<bool> {
    static event<bool> make(event<> e) {
        return event<bool>(TAMER_MOVE(e), placeholder_buffer.b);
    }
};


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
