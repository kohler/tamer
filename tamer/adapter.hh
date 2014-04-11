#ifndef TAMER_ADAPTER_HH
#define TAMER_ADAPTER_HH
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
#include <tamer/xadapter.hh>
namespace tamer {

/** @file <tamer/adapter.hh>
 *  @brief  Functions for adapting events.
 */

namespace outcome {
const int success = 0;
const int cancel = -ECANCELED;
const int timeout = -ETIMEDOUT;
const int signal = -EINTR;
const int overflow = -EOVERFLOW;
const int closed = -EPIPE;
}

/** @brief  Create event that triggers @a e1 and @a e2 when triggered.
 *  @param  e1  First event.
 *  @param  e2  Second event.
 *  @return  Distributer event.
 *
 *  Triggering the returned event instantly triggers @a e1 and @a e2 with the
 *  same values. The returned event is automatically triggered if @a e1 and @a
 *  e2 are both triggered separately.
 */
template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> distribute(event<T0, T1, T2, T3> e1,
                                        event<T0, T1, T2, T3> e2) {
    return e1 += TAMER_MOVE(e2);
}

/** @brief  Create event that triggers @a e1, @a e2, and @a e3 when triggered.
 *  @param  e1  First event.
 *  @param  e2  Second event.
 *  @param  e3  Third event.
 *  @return  Distributer event.
 *
 *  Equivalent to distribute(distribute(@a e1, @a e2), @a e3).
 */
template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> distribute(event<T0, T1, T2, T3> e1,
                                        event<T0, T1, T2, T3> e2,
                                        event<T0, T1, T2, T3> e3) {
    e1 += TAMER_MOVE(e2);
    return e1 += TAMER_MOVE(e3);
}

#if TAMER_HAVE_PREEVENT
template <typename R, typename T0>
inline event<T0> distribute(event<T0> e1, preevent<R, T0>&& e2) {
    return e1 += TAMER_MOVE(e2);
}

template <typename R, typename T0>
inline event<T0> distribute(preevent<R, T0>&& e1, event<T0> e2) {
    return e2 += TAMER_MOVE(e1);
}
#endif

/** @brief  Create bound event for @a e with @a v0.
 *  @param  e   Event.
 *  @param  v0  Trigger value.
 *  @return  Adapter event.
 *
 *  The result is bound to @a e: triggering either event automatically
 *  triggers the other. Triggering the result instantly triggers @a e with
 *  value @a v0; triggering @a e instantly triggers the result's unblocker.
 */
template <typename T0, typename V0>
event<> bind(event<T0> e, V0 v0) {
    tamerpriv::bind_rendezvous<T0, V0>* r =
	new tamerpriv::bind_rendezvous<T0, V0>(e, TAMER_MOVE(v0));
    event<> bound = TAMER_MAKE_FN_ANNOTATED_EVENT(*r);
    e.at_trigger(bound);
    return bound;
}

#if TAMER_HAVE_PREEVENT
template <typename R, typename T0, typename V0>
inline event<> bind(preevent<R, T0>&& pe, V0 v0) {
    return bind(event<T0>(std::move(pe)), std::move(v0));
}
#endif

/** @brief  Create bound event for @a e.
 *  @param  e  Event.
 *  @return  Adapter event.
 *
 *  The result is bound to @a e: triggering either event automatically
 *  triggers the other. Triggering the result instantly triggers @a e. (The
 *  result's @c T0 trigger value is ignored.) Triggering @a e instantly
 *  triggers the result's unblocker.
 */
template <typename T0>
inline event<T0> rebind(event<> e) {
    return tamerpriv::rebinder<T0>::make(e);
}

template <typename T0>
inline event<T0> rebind(event<> e, T0& s0) {
    return event<T0>(TAMER_MOVE(e), s0);
}

#if TAMER_HAVE_PREEVENT
template <typename T0, typename R>
inline event<T0> rebind(preevent<R>&& pe) {
    return tamerpriv::rebinder<T0>::make(std::move(pe));
}

template <typename T0, typename R>
inline event<T0> rebind(preevent<R>&& pe, T0& s0) {
    return event<T0>(TAMER_MOVE(pe), s0);
}
#endif

/** @brief  Create an event that triggers another event with a mapped value.
 *  @param  e  Destination event taking T0 values.
 *  @param  f  Map function taking S0 values to T0 values.
 *  @return  Adapter event taking S0 values.
 *
 *  Triggering the returned event with some value @c v0 of type @c S0
 *  instantly triggers @a e with value @a f(@c v0). (This value should have
 *  type T0.) Triggering the returned event's unblocker instantly triggers @a
 *  e's unblocker (the map function is not called since no value was
 *  provided). If @a e is triggered directly, then the returned event's
 *  unblocker is triggered. */
template <typename S0, typename T0, typename F>
event<S0> map(event<T0> e, const F &f) {
    tamerpriv::map_rendezvous<S0, T0, F> *r =
	new tamerpriv::map_rendezvous<S0, T0, F>(f, e);
    event<S0> mapped = TAMER_MAKE_FN_ANNOTATED_EVENT(*r, r->result0());
    e.at_trigger(mapped.unblocker());
    return mapped;
}

#if TAMER_HAVE_PREEVENT
template <typename S0, typename R, typename T0, typename F>
event<S0> map(preevent<R, T0>&& pe, const F &f) {
    event<T0> e(std::move(pe));
    tamerpriv::map_rendezvous<S0, T0, F> *r =
	new tamerpriv::map_rendezvous<S0, T0, F>(f, e);
    event<S0> mapped = TAMER_MAKE_FN_ANNOTATED_EVENT(*r, r->result0());
    e.at_trigger(mapped.unblocker());
    return mapped;
}
#endif


/** @brief  Add timeout to an event.
 *  @param  delay  Timeout duration.
 *  @param  e      Event.
 *  @param  v      Timeout trigger value.
 *  @return  @a e.
 *
 *  Adds a timeout to @a e. If @a delay time passes before @a e triggers
 *  naturally, then @a e is triggered with value @a v.
 *
 *  @note Versions of this function exist for @a delay values of types @c
 *  timeval, @c double, and under the names add_timeout_sec() and
 *  add_timeout_msec(), @c int numbers of seconds and milliseconds,
 *  respectively.
 */
template <typename T0>
inline event<T0> add_timeout(const timeval& delay, event<T0> e, const T0& v) {
    at_delay(delay, tamer::bind(e, v));
    return e;
}

template <typename T0>
inline event<T0> add_timeout(double delay, event<T0> e, const T0& v) {
    at_delay(delay, tamer::bind(e, v));
    return e;
}

template <typename T0>
inline event<T0> add_timeout_sec(int delay, event<T0> e, const T0& v) {
    at_delay_sec(delay, tamer::bind(e, v));
    return e;
}

template <typename T0>
inline event<T0> add_timeout_msec(int delay, event<T0> e, const T0& v) {
    at_delay_msec(delay, tamer::bind(e, v));
    return e;
}

#if TAMER_HAVE_PREEVENT
template <typename R, typename T0>
inline event<T0> add_timeout(const timeval& delay, preevent<R, T0>&& pe, const T0& v) {
    return add_timeout(delay, event<T0>(std::move(pe)), v);
}

template <typename R, typename T0>
inline event<T0> add_timeout(double delay, preevent<R, T0>&& pe, const T0& v) {
    return add_timeout(delay, event<T0>(std::move(pe)), v);
}

template <typename R, typename T0>
inline event<T0> add_timeout_sec(int delay, preevent<R, T0>&& pe, const T0& v) {
    return add_timeout_sec(delay, event<T0>(std::move(pe)), v);
}

template <typename R, typename T0>
inline event<T0> add_timeout_msec(int delay, preevent<R, T0>&& pe, const T0& v) {
    return add_timeout_msec(delay, event<T0>(std::move(pe)), v);
}
#endif

/** @brief  Add timeout to an event.
 *  @param  delay  Timeout duration.
 *  @param  e      Event.
 *  @return  @a e.
 *
 *  Adds a timeout to @a e. If @a delay time passes before @a e triggers
 *  naturally, then @a e is unblocked (triggered without a value).
 *
 *  @note Versions of this function exist for @a delay values of types @c
 *  timeval, @c double, and under the names add_timeout_sec() and
 *  add_timeout_msec(), @c int numbers of seconds and milliseconds,
 *  respectively.
 */
template <typename T0>
inline event<T0> add_timeout(const timeval& delay, event<T0> e) {
    at_delay(delay, e.unblocker());
    return e;
}

template <typename T0>
inline event<T0> add_timeout(double delay, event<T0> e) {
    at_delay(delay, e.unblocker());
    return e;
}

template <typename T0>
inline event<T0> add_timeout_sec(int delay, event<T0> e) {
    at_delay_sec(delay, e.unblocker());
    return e;
}

template <typename T0>
inline event<T0> add_timeout_msec(int delay, event<T0> e) {
    at_delay_msec(delay, e.unblocker());
    return e;
}

#if TAMER_HAVE_PREEVENT
template <typename T0, typename R>
inline event<T0> add_timeout(const timeval& delay, preevent<R, T0>&& pe) {
    return add_timeout(delay, event<T0>(std::move(pe)));
}

template <typename T0, typename R>
inline event<T0> add_timeout(double delay, preevent<R, T0>&& pe) {
    return add_timeout(delay, event<T0>(std::move(pe)));
}

template <typename T0, typename R>
inline event<T0> add_timeout_sec(int delay, preevent<R, T0>&& pe) {
    return add_timeout_sec(delay, event<T0>(std::move(pe)));
}

template <typename T0, typename R>
inline event<T0> add_timeout_msec(int delay, preevent<R, T0>&& pe) {
    return add_timeout_msec(delay, event<T0>(std::move(pe)));
}
#endif


/** @brief  Add signal interruption to an event.
 *  @param  signo  Signal number.
 *  @param  e      Event.
 *  @param  v      Signal trigger value.
 *  @return  @a e.
 *
 *  Adds signal interruption to @a e. If signal @a signo occurs before @a e
 *  triggers naturally, then @a e is triggered with value @a v.
 */
template <typename T0>
inline event<T0> add_signal(int signo, event<T0> e, const T0& v) {
    at_signal(signo, tamer::bind(e, v));
    return e;
}

#if TAMER_HAVE_PREEVENT
template <typename R, typename T0>
inline event<T0> add_signal(int signo, preevent<R, T0>&& pe, const T0& v) {
    return add_signal(signo, event<T0>(std::move(pe)), v);
}
#endif

/** @brief  Add signal interruption to an event.
 *  @param  first  Input iterator for start of signal collection.
 *  @param  last   Input iterator for end of signal collection.
 *  @param  e      Event.
 *  @param  v      Signal trigger value.
 *  @return  @a e.
 *
 *  Adds signal interruption to @a e. If any of the signals in [@a first, @a
 *  last) occur before @a e triggers naturally, then @a e is triggered with
 *  value @a v.
 */
template <typename T0, typename SigInputIterator>
inline event<T0> add_signal(SigInputIterator first, SigInputIterator last, event<T0> e, const T0& v) {
    event<> x = tamer::bind(e, v);
    for (; first != last; ++first)
	at_signal(*first, x);
    return e;
}

#if TAMER_HAVE_PREEVENT
template <typename R, typename T0, typename SigInputIterator>
inline event<T0> add_signal(SigInputIterator first, SigInputIterator last, preevent<R, T0>&& pe, const T0& v) {
    return add_signal(first, last, event<T0>(std::move(pe)), v);
}
#endif

/** @brief  Add signal interruption to an event.
 *  @param  signo  Signal number.
 *  @param  e      Event.
 *  @return  @a e.
 *
 *  Adds signal interruption to @a e. If signal @a signo occurs before @a e
 *  triggers naturally, then @a e is unblocked (triggered without a value).
 */
template <typename T0>
inline event<T0> add_signal(int signo, event<T0> e) {
    at_signal(signo, e.unblocker());
    return e;
}

#if TAMER_HAVE_PREEVENT
template <typename T0, typename R>
inline event<T0> add_signal(int signo, preevent<R, T0>&& pe) {
    return add_signal(signo, event<T0>(std::move(pe)));
}
#endif

/** @brief  Add signal interruption to an event.
 *  @param  first  Input iterator for start of signal collection.
 *  @param  last   Input iterator for end of signal collection.
 *  @param  e      Event.
 *  @return  @a e.
 *
 *  Adds signal interruption to @a e. If any of the signals in [@a first, @a
 *  last) occur before @a e triggers naturally, then @a e is unblocked
 *  (triggered without a value).
 */
template <typename T0, typename SigInputIterator>
inline event<T0> add_signal(SigInputIterator first, SigInputIterator last, event<T0> e) {
    for (; first != last; ++first)
	at_signal(*first, e);
    return e;
}

#if TAMER_HAVE_PREEVENT
template <typename T0, typename R, typename SigInputIterator>
inline event<T0> add_signal(SigInputIterator first, SigInputIterator last, preevent<R, T0>&& pe) {
    return add_signal(first, last, event<T0>(std::move(pe)));
}
#endif


/** @brief  Add silent timeout to an event.
 *  @param  delay  Timeout duration.
 *  @param  e      Event.
 *  @return  @a e.
 *
 *  Adds a timeout to @a e. If @a delay passes before @a e triggers
 *  naturally, then @a e is unblocked (triggered without a value).
 *
 *  @note Versions of this function exist for @a delay values of types @c
 *  timeval, @c double, and under the names with_timeout_sec() and
 *  with_timeout_msec(), @c int numbers of seconds and milliseconds,
 *  respectively.
 */
template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> with_timeout(const timeval &delay, event<T0, T1, T2, T3> e) {
    at_delay(delay, e.unblocker());
    return e;
}

template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> with_timeout(double delay, event<T0, T1, T2, T3> e) {
    at_delay(delay, e.unblocker());
    return e;
}

template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> with_timeout_sec(int delay, event<T0, T1, T2, T3> e) {
    at_delay_sec(delay, e.unblocker());
    return e;
}

template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> with_timeout_msec(int delay, event<T0, T1, T2, T3> e) {
    at_delay_msec(delay, e.unblocker());
    return e;
}

#if TAMER_HAVE_PREEVENT
template <typename R, typename T0>
inline event<T0> with_timeout(const timeval& delay, preevent<R, T0>&& pe) {
    return with_timeout(delay, event<T0>(std::move(pe)));
}

template <typename R, typename T0>
inline event<T0> with_timeout(double delay, preevent<R, T0>&& pe) {
    return with_timeout(delay, event<T0>(std::move(pe)));
}

template <typename R, typename T0>
inline event<T0> with_timeout_sec(int delay, preevent<R, T0>&& pe) {
    return with_timeout_sec(delay, event<T0>(std::move(pe)));
}

template <typename R, typename T0>
inline event<T0> with_timeout_msec(int delay, preevent<R, T0>&& pe) {
    return with_timeout_msec(delay, event<T0>(std::move(pe)));
}
#endif

/** @brief  Add silent signal interruption to an event.
 *  @param  signo  Signal number.
 *  @param  e      Event.
 *  @return  @a e.
 *
 *  Adds signal interruption to @a e. If signal @a signo occurs before @a e
 *  triggers naturally, then @a e is unblocked (triggered without a value).
 */
template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> with_signal(int signo, event<T0, T1, T2, T3> e) {
    at_signal(signo, e.unblocker());
    return e;
}

#if TAMER_HAVE_PREEVENT
template <typename R, typename T0>
inline event<T0> with_signal(int signo, preevent<R, T0>&& pe) {
    return with_signal(signo, event<T0>(std::move(pe)));
}
#endif

/** @brief  Add silent signal interruption to an event.
 *  @param  first  Input iterator for start of signal collection.
 *  @param  last   Input iterator for end of signal collection.
 *  @param  e      Event.
 *  @return  @a e.
 *
 *  Adds signal interruption to @a e. If a signal in [@a first, @a last)
 *  occurs before @a e triggers naturally, then @a e is unblocked (triggered
 *  without a value).
 */
template <typename T0, typename T1, typename T2, typename T3, typename SigInputIterator>
inline event<T0, T1, T2, T3> with_signal(SigInputIterator first, SigInputIterator last, event<T0, T1, T2, T3> e) {
    event<> x = e.unblocker();
    for (; first != last; ++first)
	at_signal(*first, x);
    return e;
}

#if TAMER_HAVE_PREEVENT
template <typename R, typename T0, typename SigInputIterator>
inline event<T0> with_signal(SigInputIterator first, SigInputIterator last, preevent<R, T0>&& pe) {
    return with_signal(first, last, event<T0>(std::move(pe)));
}
#endif


/** @brief  Add timeout to an event.
 *  @param       delay   Timeout duration.
 *  @param       e       Event.
 *  @param[out]  result  Result tracker.
 *  @return      @a e.
 *
 *  Adds a timeout to @a e. If @a e triggers naturally before @a delay time
 *  passes, then @a result is set to 0. Otherwise, @a e is unblocked
 *  (triggered without a value), and @a result is set to @c -ETIMEDOUT.
 *
 *  @note Versions of this function exist for @a delay values of types @c
 *  timeval, @c double, and under the names with_timeout_sec() and
 *  with_timeout_msec(), @c int numbers of seconds and milliseconds,
 *  respectively.
 */
template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> with_timeout(const timeval &delay, event<T0, T1, T2, T3> e, int &result) {
    at_delay(delay, tamerpriv::with_helper(e, &result, outcome::timeout));
    return e;
}

template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> with_timeout(double delay, event<T0, T1, T2, T3> e, int &result) {
    at_delay(delay, tamerpriv::with_helper(e, &result, outcome::timeout));
    return e;
}

template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> with_timeout_sec(int delay, event<T0, T1, T2, T3> e, int &result) {
    at_delay_sec(delay, tamerpriv::with_helper(e, &result, outcome::timeout));
    return e;
}

template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> with_timeout_msec(int delay, event<T0, T1, T2, T3> e, int &result) {
    at_delay_msec(delay, tamerpriv::with_helper(e, &result, outcome::timeout));
    return e;
}

#if TAMER_HAVE_PREEVENT
template <typename R, typename T0>
inline event<T0> with_timeout(const timeval& delay, preevent<R, T0>&& pe, int& result) {
    return with_timeout(delay, event<T0>(std::move(pe)), result);
}

template <typename R, typename T0>
inline event<T0> with_timeout(double delay, preevent<R, T0>&& pe, int& result) {
    return with_timeout(delay, event<T0>(std::move(pe)), result);
}

template <typename R, typename T0>
inline event<T0> with_timeout_sec(int delay, preevent<R, T0>&& pe, int& result) {
    return with_timeout_sec(delay, event<T0>(std::move(pe)), result);
}

template <typename R, typename T0>
inline event<T0> with_timeout_msec(int delay, preevent<R, T0>&& pe, int& result) {
    return with_timeout_msec(delay, event<T0>(std::move(pe)), result);
}
#endif

/** @brief  Add signal interruption to an event.
 *  @param       signo   Signal number.
 *  @param       e       Event.
 *  @param[out]  result  Result tracker.
 *  @return      @a e.
 *
 *  Adds signal interruption to @a e. If @a e triggers naturally before
 *  signal @a signo occurs, then @a result is set to 0. Otherwise, @a e is
 *  unblocked (triggered without a value), and @a result is set to @c
 *  -EINTR.
 */
template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> with_signal(int signo, event<T0, T1, T2, T3> e, int &result) {
    at_signal(signo, tamerpriv::with_helper(e, &result, outcome::signal));
    return e;
}

#if TAMER_HAVE_PREEVENT
template <typename R, typename T0>
inline event<T0> with_signal(int signo, preevent<R, T0>&& pe, int& result) {
    return with_signal(signo, event<T0>(std::move(pe)), result);
}
#endif

/** @brief  Add signal interruption to an event.
 *  @param       first   Input iterator for start of signal collection.
 *  @param       last    Input iterator for end of signal collection.
 *  @param       e       Event.
 *  @param[out]  result  Result tracker.
 *  @return      Adapter event.
 *
 *  Adds signal interruption to @a e. If @a e triggers naturally before a
 *  signal in [@a first, @a last) occurs, then @a result is set to 0.
 *  Otherwise, @a e is unblocked (triggered without a value), and @a result
 *  is set to @c -EINTR.
 */
template <typename T0, typename T1, typename T2, typename T3, typename SigInputIterator>
inline event<T0, T1, T2, T3> with_signal(SigInputIterator first, SigInputIterator last, event<T0, T1, T2, T3> e, int &result) {
    event<> x = tamerpriv::with_helper(e, &result, outcome::signal);
    for (; first != last; ++first)
	at_signal(*first, x);
    return e;
}

#if TAMER_HAVE_PREEVENT
template <typename R, typename T0, typename SigInputIterator>
inline event<T0> with_signal(SigInputIterator first, SigInputIterator last, preevent<R, T0>&& pe, int& result) {
    return with_signal(first, last, event<T0>(std::move(pe)), result);
}
#endif


/** @brief  Create event that calls a function when triggered.
 *  @param  f  Function object.
 *
 *  Returns an event that calls (a copy of) @a f() when triggered. The call
 *  happens immediately, at trigger time.
 *
 *  @note @a f must have a public move constructor and a public destructor.
 */
template <typename F>
inline event<> fun_event(F f) {
    tamerpriv::function_rendezvous<F> *innerr =
	new tamerpriv::function_rendezvous<F>(TAMER_MOVE(f));
    return tamer::make_event(*innerr);
}

/** @brief  Create event that calls a function when triggered.
 *  @param  f  Function object.
 *  @param  arg  Argument.
 *
 *  Returns an event that calls @a f(@a arg) when triggered. The call happens
 *  immediately, at trigger time.
 *
 *  @note @a f must have a public copy constructor and a public destructor.
 */
template <typename F, typename A>
inline event<> fun_event(F f, A arg) {
    tamerpriv::function_rendezvous<F, A> *innerr =
	new tamerpriv::function_rendezvous<F, A>(TAMER_MOVE(f),
						 TAMER_MOVE(arg));
    return TAMER_MAKE_FN_ANNOTATED_EVENT(*innerr);
}

/** @brief  Create event that calls a function when triggered.
 *  @param  f  Function object.
 *  @param  arg1  First argument.
 *  @param  arg2  Second argument.
 *
 *  Returns an event that calls @a f(@a arg1, @a arg2) when triggered. The
 *  call happens immediately, at trigger time.
 *
 *  @note @a f must have a public copy constructor and a public destructor.
 */
template <typename F, typename A1, typename A2>
inline event<> fun_event(F f, A1 arg1, A2 arg2) {
    tamerpriv::function_rendezvous<F, A1, A2> *innerr =
	new tamerpriv::function_rendezvous<F, A1, A2>(TAMER_MOVE(f),
						      TAMER_MOVE(arg1),
						      TAMER_MOVE(arg2));
    return TAMER_MAKE_FN_ANNOTATED_EVENT(*innerr);
}


template <typename C>
inline event<typename C::value_type> push_back_event(C& container) {
    tamerpriv::push_back_rendezvous<C>* r =
        new tamerpriv::push_back_rendezvous<C>(container);
    return TAMER_MAKE_FN_ANNOTATED_EVENT(*r, r->slot());
}

template <typename It>
inline event<typename std::iterator_traits<It>::value_type> output_event(It& iterator) {
    tamerpriv::output_rendezvous<It>* r =
        new tamerpriv::output_rendezvous<It>(iterator);
    return TAMER_MAKE_FN_ANNOTATED_EVENT(*r, r->slot());
}

} /* namespace tamer */
#endif /* TAMER_ADAPTER_HH */
