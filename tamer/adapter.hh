#ifndef TAMER_ADAPTER_HH
#define TAMER_ADAPTER_HH 1
#include <tamer/xadapter.hh>
namespace tamer {

namespace outcome {
const int success = 0;
const int cancel = -ECANCELED;
const int timeout = -ETIMEDOUT;
const int signal = -EINTR;
}

/** @brief  Create event that triggers @a e1 and @a e2 when triggered.
 *  @param  e1  First event.
 *  @param  e2  Second event.
 *  @return  Distributer event.
 *
 *  When the distributer event is triggered, @a e1 and @a e2 are both
 *  triggered instantly.  When both @a e1 and @a e2 are canceled, the
 *  distributer event is itself canceled.
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
 *  @param  v0  Trigger value
 *  @return  Adapter event.
 *
 *  When the adapter event is triggered, @a e is triggered instantly with
 *  trigger value @a v0.  When @a e is canceled, the adapter event is itself
 *  canceled.
 */
template <typename T0, typename V0>
event<> bind(const event<T0> &e, const V0 &v0) {
    tamerpriv::bind_rendezvous<T0> *ur = new tamerpriv::bind_rendezvous<T0>(e, v0);
    return event<>(*ur, 1);
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
event<T0> ignore_slot(const event<> &e) {
    tamerpriv::unbind_rendezvous<T0> *ur = new tamerpriv::unbind_rendezvous<T0>(e);
    return event<T0>(*ur, 1, *((T0 *) 0));
}

/** @brief  Create event that cancels @a e when triggered.
 *  @param  e  Event.
 *  @return  Canceler event.
 *
 *  When the canceler event is triggered, @a e is canceled instantly.  When
 *  @a e is canceled or triggered, the canceler event is itself canceled.
 */
template <typename T0, typename T1, typename T2, typename T3>
inline event<> make_canceler(const event<T0, T1, T2, T3> &e) {
    tamerpriv::canceler_rendezvous *r = new tamerpriv::canceler_rendezvous(e);
    return event<>(*r);
}

/** @brief  Cause @a e1 to cancel when @a e2 is canceled.
 *  @param  e1  Event.
 *  @param  e2  Event.
 *  @return  @a e2.
 *
 *  When @a e2 is canceled, @a e1 is canceled immediately.
 */
template <typename T0, typename T1, typename T2, typename T3,
	  typename U0, typename U1, typename U2, typename U3>
inline event<T0, T1, T2, T3> spread_cancel(const event<U0, U1, U2, U3> &e1, event<T0, T1, T2, T3> e2) {
    tamerpriv::canceler_rendezvous *r = new tamerpriv::canceler_rendezvous(e1);
    e2.at_cancel(event<>(*r));
    return e2;
}

/** @brief  Add a cancel notifier to an event.
 *  @param  r   Rendezvous.
 *  @param  i0  First event ID.
 *  @param  i1  Second event ID.
 *  @param  e   Event.
 *  @return  @a e.
 *
 *  Equivalent to
 *  @code
 *  e.at_cancel(make_event(r, i0, i1)); return e;
 *  @endcode
 */
template <typename R, typename I0, typename I1, typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> make_cancel(R &r, const I0 &i0, const I1 &i1, event<T0, T1, T2, T3> e) {
    e.at_cancel(make_event(r, i0, i1));
    return e;
}

template <typename R, typename I0, typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> make_cancel(R &r, const I0 &i0, event<T0, T1, T2, T3> e) {
    e.at_cancel(make_event(r, i0));
    return e;
}

template <typename R, typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> make_cancel(R &r, event<T0, T1, T2, T3> e) {
    e.at_cancel(make_event(r));
    return e;
}


template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> with_timeout(const timeval &delay, event<T0, T1, T2, T3> e) {
    tamerpriv::cancel_adapter_rendezvous *r = new tamerpriv::cancel_adapter_rendezvous(e, 0);
    at_delay(delay, event<>(*r, outcome::timeout));
    return e.make_rebind(*r, outcome::success);
}

template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> with_timeout(const timeval &delay, event<T0, T1, T2, T3> e, int &result) {
    result = outcome::success;
    tamerpriv::cancel_adapter_rendezvous *r = new tamerpriv::cancel_adapter_rendezvous(e, &result);
    at_delay(delay, event<>(*r, outcome::timeout));
    return e.make_rebind(*r, outcome::success);
}

template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> with_timeout_sec(int delay, event<T0, T1, T2, T3> e) {
    timeval tv;
    tv.tv_sec = delay;
    tv.tv_usec = 0;
    return with_timeout(tv, e);
}

template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> with_timeout_sec(int delay, event<T0, T1, T2, T3> e, int &result) {
    timeval tv;
    tv.tv_sec = delay;
    tv.tv_usec = 0;
    return with_timeout(tv, e, result);
}

template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> with_timeout_msec(int delay, event<T0, T1, T2, T3> e) {
    timeval tv;
    tv.tv_sec = delay / 1000;
    tv.tv_usec = delay % 1000;
    return with_timeout(tv, e);
}

template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> with_timeout_msec(int delay, event<T0, T1, T2, T3> e, int &result) {
    timeval tv;
    tv.tv_sec = delay / 1000;
    tv.tv_usec = delay % 1000;
    return with_timeout(tv, e, result);
}


inline event<int> add_timeout(const timeval &delay, event<int> e) {
    tamerpriv::cancel_adapter_rendezvous *r = new tamerpriv::cancel_adapter_rendezvous(e, e.slot0());
    at_delay(delay, event<>(*r, outcome::timeout));
    return e.make_rebind(*r, outcome::success);
}

inline event<int> add_timeout_sec(int delay, event<int> e) {
    timeval tv;
    tv.tv_sec = delay;
    tv.tv_usec = 0;
    return add_timeout(tv, e);
}

inline event<int> add_timeout_msec(int delay, event<int> e) {
    timeval tv;
    tv.tv_sec = delay / 1000;
    tv.tv_usec = delay % 1000;
    return add_timeout(tv, e);
}


template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> with_signal(int sig, event<T0, T1, T2, T3> e) {
    tamerpriv::cancel_adapter_rendezvous *r = new tamerpriv::cancel_adapter_rendezvous(e, 0);
    at_signal(sig, event<>(*r, outcome::signal));
    return e.make_rebind(*r, outcome::success);
}

template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> with_signal(int sig, event<T0, T1, T2, T3> e, int &result) {
    result = outcome::success;
    tamerpriv::cancel_adapter_rendezvous *r = new tamerpriv::cancel_adapter_rendezvous(e, &result);
    at_signal(sig, event<>(*r, outcome::signal));
    return e.make_rebind(*r, outcome::success);
}

template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> with_signal(const std::vector<int> &sig, event<T0, T1, T2, T3> e) {
    tamerpriv::cancel_adapter_rendezvous *r = new tamerpriv::cancel_adapter_rendezvous(e, 0);
    for (std::vector<int>::const_iterator i = sig.begin(); i != sig.end(); i++)
	at_signal(*i, event<>(*r, outcome::signal));
    return e.make_rebind(*r, outcome::success);
}

template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> with_signal(const std::vector<int> &sig, event<T0, T1, T2, T3> e, int &result) {
    result = outcome::success;
    tamerpriv::cancel_adapter_rendezvous *r = new tamerpriv::cancel_adapter_rendezvous(e, &result);
    for (std::vector<int>::const_iterator i = sig.begin(); i != sig.end(); i++)
	at_signal(*i, event<>(*r, outcome::signal));
    return e.make_rebind(*r, outcome::success);
}


inline event<int> add_signal(int sig, event<int> e) {
    tamerpriv::cancel_adapter_rendezvous *r = new tamerpriv::cancel_adapter_rendezvous(e, e.slot0());
    at_signal(sig, event<>(*r, outcome::signal));
    return e.make_rebind(*r, outcome::success);
}

inline event<int> add_signal(const std::vector<int> &sig, event<int> e) {
    tamerpriv::cancel_adapter_rendezvous *r = new tamerpriv::cancel_adapter_rendezvous(e, e.slot0());
    for (std::vector<int>::const_iterator i = sig.begin(); i != sig.end(); i++)
	at_signal(*i, event<>(*r, outcome::signal));
    return e.make_rebind(*r, outcome::success);
}


template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> with_cancel(event<T0, T1, T2, T3> e) {
    tamerpriv::cancel_adapter_rendezvous *r = new tamerpriv::cancel_adapter_rendezvous(e, 0);
    return e.make_rebind(*r, outcome::success);
}

inline event<int> add_cancel(event<int> e) {
    tamerpriv::cancel_adapter_rendezvous *r = new tamerpriv::cancel_adapter_rendezvous(e, e.slot0());
    return e.make_rebind(*r, outcome::success);
}

} /* namespace tamer */
#endif /* TAMER_ADAPTER_HH */
