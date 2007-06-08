#ifndef TAMER_ADAPTER_HH
#define TAMER_ADAPTER_HH 1
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
 *  When the distributer event is triggered, @a e1 and @a e2 are both
 *  triggered instantly.  The distributer event is itself canceled when both
 *  @a e1 and @a e2 are canceled.
 */
inline event<> distribute(const event<> &e1, const event<> &e2) {
    if (e1.empty())
	return e2;
    else if (e2.empty())
	return e1;
    else
	return tamerpriv::hard_distribute(e1, e2);
}

/** @brief  Create event that triggers @a e1, @a e2, and @a e3 when triggered.
 *  @param  e1  First event.
 *  @param  e2  Second event.
 *  @param  e3  Third event.
 *  @return  Distributer event.
 *
 *  When the distributer event is triggered, @a e1, @a e2, and @a e3 are all
 *  triggered instantly.  When @a e1, @a e2, and @a e3 are all canceled, the
 *  distributer event is itself canceled.
 */
inline event<> distribute(const event<> &e1, const event<> &e2, const event<> &e3) {
    return distribute(distribute(e1, e2), e3);
}

/** @brief  Create event that triggers @a e with @a v0 when triggered.
 *  @param  e   Event.
 *  @param  v0  Trigger value.
 *  @return  Adapter event.
 *
 *  When the adapter event is triggered, @a e is triggered instantly with
 *  trigger value @a v0.  When @a e is canceled, the adapter event is itself
 *  canceled.
 */
template <typename T0, typename V0>
event<> bind(event<T0> e, const V0 &v0) {
    tamerpriv::function_rendezvous<tamerpriv::bind_function<T0> > *r =
	new tamerpriv::function_rendezvous<tamerpriv::bind_function<T0> >(e, v0);
    e.at_trigger(event<>(*r, r->canceler));
    return event<>(*r, r->triggerer);
}

/** @brief  Create 1-slot event that triggers @a e when triggered.
 *  @param  e  Event.
 *  @return  Adapter event.
 *
 *  When the adapter event is triggered, its @c T0 trigger value is ignored
 *  and @a e is triggered instantly.  When @a e is canceled, the adapter
 *  event is itself canceled.
 */
template <typename T0>
event<T0> unbind(const event<> &e) {
    return event<T0>(e, no_slot());
}


/** @brief  Add cancellation notification to an event.
 *  @param  e  Event.
 *  @return  Adapter event.
 *
 *  Returns an adapter that adds cancellation support to @a e.  If the adapter
 *  event is triggered, then @a e is triggered with the same value.  If the
 *  adapter event is canceled, then @a e is triggered with value @c
 *  -ECANCELED.  Conversely, if @a e is triggered or canceled, then the
 *  adapter event is canceled.
 *
 *  @note Event adapter functions like add_cancel() are particularly useful in
 *  @c twait{} blocks.  Consider this code:
 *  @code
 *  twait { at_fd_read(fd, make_event()); }
 *  @endcode
 *  If @c fd is closed, the created event is canceled, preventing the @c
 *  twait{} block from ever completing.  This version, using with_cancel(),
 *  handles this correctly; the @c twait{} block will complete when @c fd
 *  either becomes readable or is closed.
 *  @code
 *  twait { at_fd_read(fd, with_cancel(make_event())); }
 *  @endcode
 *  The @c add_ versions set the event's trigger value based on whether
 *  cancellation occurred; the @c with_ versions use
 *  event<>::unbound_trigger() in case of cancellation to trigger events
 *  without setting their trigger values.
 */
//XXXXXXXXXXXX


/** @brief  Add timeout to an event.
 *  @param  delay  Timeout duration.
 *  @param  e      Event.
 *  @return  Adapter event.
 *
 *  Returns an adapter that adds a timeout and cancellation support to @a e.
 *  If the adapter event is triggered, then @a e is triggered with the same
 *  value.  If the adapter event is canceled, then @a e is triggered with
 *  value @c -ECANCELED.  If @a delay time passes, then @a e is triggered with
 *  value @c -ETIMEDOUT.  Conversely, if @a e is triggered or canceled, then
 *  the adapter event is canceled.
 *
 *  @note Versions of this function exist for @a delay values of types @c
 *  timeval, @c double, and under the names add_timeout_sec() and
 *  add_timeout_msec(), @c int numbers of seconds and milliseconds,
 *  respectively.
 */
inline event<int> add_timeout(const timeval &delay, event<int> e) {
    at_delay(delay, bind(e, outcome::timeout));
    return e;
}

inline event<int> add_timeout(double delay, event<int> e) {
    at_delay(delay, bind(e, outcome::timeout));
    return e;
}

inline event<int> add_timeout_sec(int delay, event<int> e) {
    at_delay_sec(delay, bind(e, outcome::timeout));
    return e;
}

inline event<int> add_timeout_msec(int delay, event<int> e) {
    at_delay_msec(delay, bind(e, outcome::timeout));
    return e;
}

/** @brief  Add signal interruption to an event.
 *  @param  sig  Signal number.
 *  @param  e    Event.
 *  @return  Adapter event.
 *
 *  Returns an adapter that adds signal interruption and cancellation support
 *  to @a e.  If the adapter event is triggered, then @a e is triggered with
 *  the same value.  If the adapter event is canceled, then @a e is triggered
 *  with value @c -ECANCELED.  If signal @a sig occurs, then @a e is triggered
 *  with value @c -EINTR.  Conversely, if @a e is triggered or canceled, then
 *  the adapter event is canceled.
 */
inline event<int> add_signal(int sig, event<int> e) {
    at_signal(sig, bind(e, outcome::signal));
    return e;
}

/** @brief  Add signal interruption to an event.
 *  @param  first  Input iterator for start of signal collection.
 *  @param  last   Input iterator for end of signal collection.
 *  @param  e      Event.
 *  @return  Adapter event.
 *
 *  Returns an adapter that adds signal interruption and cancellation support
 *  to @a e.  If the adapter event is triggered, then @a e is triggered with
 *  the same value.  If the adapter event is canceled, then @a e is triggered
 *  with value @c -ECANCELED.  If any of the signals in [@a first, @a last)
 *  occur, then @a e is triggered with value @c -EINTR.  Conversely, if @a e
 *  is triggered or canceled, then the adapter event is canceled.
 */
template <class SigInputIterator>
inline event<int> add_signal(SigInputIterator first, SigInputIterator last, event<int> e) {
    event<> x = bind(e, outcome::signal);
    for (; first != last; ++first)
	at_signal(*first, x);
    return e;
}


/** @brief  Add silent cancellation notification to an event.
 *  @param  e  Event.
 *  @return  Adapter event.
 *
 *  Returns an adapter that adds cancellation support to @a e.  If the adapter
 *  event is triggered, then @a e is triggered with the same values.  If the
 *  adapter event is canceled, then @a e is triggered via
 *  event<>::unbound_trigger(), which leaves the trigger slots unchanged.
 *  Conversely, if @a e is triggered or canceled, then the adapter event is
 *  canceled.
 *
 *  @note Unlike the @c add_ adapters, such as add_cancel(), the @c with_
 *  adapters don't provide any information about whether an event actually
 *  triggered.  The programmer should use trigger values to distinguish the
 *  cases.
 */
// XXXXX

/** @brief  Add silent timeout to an event.
 *  @param  delay  Timeout duration.
 *  @param  e      Event.
 *  @return  Adapter event.
 *
 *  Returns an adapter that adds a timeout and cancellation support to @a e.
 *  If the adapter event is triggered, then @a e is triggered with the same
 *  values.  If the adapter event is canceled, or if @a delay seconds pass
 *  first, then @a e is triggered via event<>::unbound_trigger(), which leaves
 *  the trigger slots unchanged.  Conversely, if @a e is triggered or
 *  canceled, then the adapter event is canceled.
 *
 *  @note Versions of this function exist for @a delay values of types @c
 *  timeval, @c double, and under the names with_timeout_sec() and
 *  with_timeout_msec(), @c int numbers of seconds and milliseconds,
 *  respectively.
 */
template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> with_timeout(const timeval &delay, event<T0, T1, T2, T3> e) {
    at_delay(delay, e.bind_all());
    return e;
}

template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> with_timeout(double delay, event<T0, T1, T2, T3> e) {
    at_delay(delay, e.bind_all());
    return e;
}

template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> with_timeout_sec(int delay, event<T0, T1, T2, T3> e) {
    at_delay_sec(delay, e.bind_all());
    return e;
}

template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> with_timeout_msec(int delay, event<T0, T1, T2, T3> e) {
    at_delay_msec(delay, e.bind_all());
    return e;
}

/** @brief  Add silent signal interruption to an event.
 *  @param  sig  Signal number.
 *  @param  e    Event.
 *  @return  Adapter event.
 *
 *  Returns an adapter that adds signal interruption and cancellation support
 *  to @a e.  If the adapter event is triggered, then @a e is triggered with
 *  the same values.  If the adapter event is canceled, or signal @a sig is
 *  received first, then @a e is triggered via event<>::unbound_trigger(),
 *  which leaves the trigger slots unchanged.  Conversely, if @a e is
 *  triggered or canceled, then the adapter event is canceled.
 */
template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> with_signal(int sig, event<T0, T1, T2, T3> e) {
    at_signal(sig, e.bind_all());
    return e;
}

/** @brief  Add silent signal interruption to an event.
 *  @param  first  Input iterator for start of signal collection.
 *  @param  last   Input iterator for end of signal collection.
 *  @param  e      Event.
 *  @return  Adapter event.
 *
 *  Returns an adapter that adds signal interruption and cancellation support
 *  to @a e.  If the adapter event is triggered, then @a e is triggered with
 *  the same values.  If the adapter event is canceled, or one of the signals
 *  in [@a first, @a last) is received first, then @a e is triggered via
 *  event<>::unbound_trigger(), which leaves the trigger slots unchanged.
 *  Conversely, if @a e is triggered or canceled, then the adapter event is
 *  canceled.
 */
template <typename T0, typename T1, typename T2, typename T3, typename SigInputIterator>
inline event<T0, T1, T2, T3> with_signal(SigInputIterator first, SigInputIterator last, event<T0, T1, T2, T3> e) {
    event<> x = e.bind_all();
    for (; first != last; ++first)
	at_signal(*first, x);
    return e;
}


/** @brief  Add cancellation notification to an event.
 *  @param       e       Event.
 *  @param[out]  result  Result tracker.
 *  @return      Adapter event.
 *
 *  Returns an adapter that adds cancellation support to @a e.  The variable
 *  @a result is initially set to 0.  If the adapter event is triggered, then
 *  @a e is triggered with the same values.  If the adapter event is canceled,
 *  then @a e is triggered via event<>::unbound_trigger(), which leaves the
 *  trigger slots unchanged, and @a result is immediately set to @c
 *  -ECANCELED.  Conversely, if @a e is triggered or canceled, then the
 *  adapter event is canceled.
 */
// XXXXXX

/** @brief  Add timeout to an event.
 *  @param       delay   Timeout duration.
 *  @param       e       Event.
 *  @param[out]  result  Result tracker.
 *  @return  Adapter event.
 *
 *  Returns an adapter that adds a timeout and cancellation support to @a e.
 *  Initially sets the variable @a to 0.  If the adapter event is
 *  triggered, then @a e is triggered with the same values.  If the adapter
 *  event is canceled, or if @a delay seconds pass first, then @a e is
 *  triggered via event<>::unbound_trigger(), which leaves the trigger slots
 *  unchanged, and @a result is immediately set to @c -ECANCELED or @c
 *  -ETIMEDOUT, respectively.  Conversely, if @a e is triggered or canceled,
 *  then the adapter event is canceled.
 *
 *  @note Versions of this function exist for @a delay values of types @c
 *  timeval, @c double, and under the names with_timeout_sec() and
 *  with_timeout_msec(), @c int numbers of seconds and milliseconds,
 *  respectively.
 */

template <typename T0, typename T1, typename T2, typename T3>
inline event<> __with_helper(event<T0, T1, T2, T3> e, int *result, int value) {
    tamerpriv::function_rendezvous<tamerpriv::assign_trigger_function<int> > *r =
	new tamerpriv::function_rendezvous<tamerpriv::assign_trigger_function<int> >(e.__get_simple(), result, value);
    e.at_trigger(event<>(*r, r->canceler));
    return event<>(*r, r->triggerer);
}

template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> with_timeout(const timeval &delay, event<T0, T1, T2, T3> e, int &result) {
    at_delay(delay, __with_helper(e, &result, outcome::timeout));
    return e;
}

template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> with_timeout(double &delay, event<T0, T1, T2, T3> e, int &result) {
    at_delay(delay, __with_helper(e, &result, outcome::timeout));
    return e;
}

template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> with_timeout_sec(int delay, event<T0, T1, T2, T3> e, int &result) {
    at_delay_sec(delay, __with_helper(e, &result, outcome::timeout));
    return e;
}

template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> with_timeout_msec(int delay, event<T0, T1, T2, T3> e, int &result) {
    at_delay_msec(delay, __with_helper(e, &result, outcome::timeout));
    return e;
}

/** @brief  Add signal interruption to an event.
 *  @param       sig     Signal number.
 *  @param       e       Event.
 *  @param[out]  result  Result tracker.
 *  @return      Adapter event.
 *
 *  Returns an adapter that adds signal interruption and cancellation support
 *  to @a e.  Initially sets the variable @a to 0.  If the adapter event is
 *  triggered, then @a e is triggered with the same values.  If the adapter
 *  event is canceled, or if signal @a sig is received first, then @a e is
 *  triggered via event<>::unbound_trigger(), which leaves the trigger slots
 *  unchanged, and @a result is immediately set to @c -ECANCELED or @c -EINTR,
 *  respectively.  Conversely, if @a e is triggered or canceled, then the
 *  adapter event is canceled.
 */
template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> with_signal(int sig, event<T0, T1, T2, T3> e, int &result) {
    at_signal(sig, __with_helper(e, &result, outcome::signal));
    return e;
}

/** @brief  Add signal interruption to an event.
 *  @param       first   Input iterator for start of signal collection.
 *  @param       last    Input iterator for end of signal collection.
 *  @param       e       Event.
 *  @param[out]  result  Result tracker.
 *  @return      Adapter event.
 *
 *  Returns an adapter that adds signal interruption and cancellation support
 *  to @a e.  Initially sets the variable @a to 0.  If the adapter event is
 *  triggered, then @a e is triggered with the same values.  If the adapter
 *  event is canceled, or if any signal in [@a first, @a last) is received
 *  first, then @a e is triggered via event<>::unbound_trigger(), which leaves
 *  the trigger slots unchanged, and @a result is immediately set to @c
 *  -ECANCELED or @c -EINTR, respectively.  Conversely, if @a e is triggered
 *  or canceled, then the adapter event is canceled.
 */
template <typename T0, typename T1, typename T2, typename T3, typename SigInputIterator>
inline event<T0, T1, T2, T3> with_signal(SigInputIterator first, SigInputIterator last, event<T0, T1, T2, T3> e, int &result) {
    event<> x = __with_helper(e, &result, outcome::signal);
    for (; first != last; ++first)
	at_signal(*first, x);
    return e;
}


/** @brief  Create event that calls a function when triggered.
 *  @param  f  Function object.
 *
 *  Returns an event that calls @a f() immediately when it is triggered.  Once
 *  the event is completed (triggered or canceled), the @a f object is
 *  destroyed.
 */
template <typename F>
inline event<> fun_event(const F &f) {
    tamerpriv::function_rendezvous<F> *innerr =
	new tamerpriv::function_rendezvous<F>(f);
    return event<>(*innerr, innerr->triggerer);
}

} /* namespace tamer */
#endif /* TAMER_ADAPTER_HH */
