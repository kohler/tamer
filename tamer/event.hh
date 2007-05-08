#ifndef TAMER_EVENT_HH
#define TAMER_EVENT_HH 1
#include <tamer/xevent.hh>
namespace tamer {

/** @file <tamer/event.hh>
 *  @brief  The event template classes and helper functions.
 */

/** @class event tamer/event.hh <tamer.hh>
 *  @brief  A future occurrence.
 *
 *  A Tamer event object represents a future occurrence.
 *
 *  Multiple event objects may refer to the same underlying occurrence.
 *  Triggering or canceling an event can thus affect several event objects.
 *  For instance, after an assignment <tt>e1 = e2;</tt>, @c e1 and @c e2 refer
 *  to the same occurrence, and either <tt>e1.trigger()</tt> or
 *  <tt>e2.trigger()</tt> would have the same observable effects.
 */
template <typename T0, typename T1, typename T2, typename T3>
class event { public:

    /** @brief  Default constructor creates an empty event. */
    event()
	: _e(new tamerpriv::eventx<T0, T1, T2, T3>()) {
    }

    /** @brief  Construct a two-ID, four-slot event on rendezvous @a r.
     *  @param  r   Rendezvous.
     *  @param  i0  First event ID.
     *  @param  i1  Second event ID.
     *  @param  t0  First trigger slot.
     *  @param  t1  Second trigger slot.
     *  @param  t2  Third trigger slot.
     *  @param  t3  Fourth trigger slot.
     */
    template <typename R, typename I0, typename I1>
    event(R &r, const I0 &i0, const I1 &i1, T0 &t0, T1 &t1, T2 &t2, T3 &t3)
	: _e(new tamerpriv::eventx<T0, T1, T2, T3>(r, i0, i1, t0, t1, t2, t3)) {
    }

    /** @brief  Construct a one-ID, four-slot event on rendezvous @a r.
     *  @param  r   Rendezvous.
     *  @param  i0  First event ID.
     *  @param  t0  First trigger slot.
     *  @param  t1  Second trigger slot.
     *  @param  t2  Third trigger slot.
     *  @param  t3  Fourth trigger slot.
     */
    template <typename R, typename I0>
    event(R &r, const I0 &i0, T0 &t0, T1 &t1, T2 &t2, T3 &t3)
	: _e(new tamerpriv::eventx<T0, T1, T2, T3>(r, i0, t0, t1, t2, t3)) {
    }

    /** @brief  Construct a no-ID, four-slot event on rendezvous @a r.
     *  @param  r   Rendezvous.
     *  @param  t0  First trigger slot.
     *  @param  t1  Second trigger slot.
     *  @param  t2  Third trigger slot.
     *  @param  t3  Fourth trigger slot.
     */
    template <typename R>
    event(R &r, T0 &t0, T1 &t1, T2 &t2, T3 &t3)
	: _e(new tamerpriv::eventx<T0, T1, T2, T3>(r, t0, t1, t2, t3)) {
    }

    /** @brief  Construct event for the same occurrence as @a e.
     *  @param  e  Source event.
     */
    event(const event<T0, T1, T2, T3> &e)
	: _e(e._e) {
	_e->use();
    }

    /** @brief  Destroy the event instance.
     *  @note   The underlying occurrence is canceled if this event was the
     *          last remaining reference.
     */
    ~event() {
	_e->unuse();
    }

    typedef tamerpriv::simple_event::unspecified_bool_type unspecified_bool_type;

    /** @brief  Test if event is active.
     *  @return  True if event is active, false if it is empty. */
    operator unspecified_bool_type() const {
	return *_e;
    }

    /** @brief  Test if event is empty.
     *  @return  True if event is empty, false if it is active. */
    bool operator!() const {
	return _e->empty();
    }

    /** @brief  Test if event is empty.
     *  @return  True if event is empty, false if it is active. */
    bool empty() const {
	return _e->empty();
    }

    /** @brief  Register a cancel notifier.
     *  @param  e  Cancel notifier.
     *
     *  If event is empty, @a e is immediately triggered.  Otherwise,
     *  when this event is triggered, cancels @a e.  Otherwise, when
     *  this event is canceled (explicitly or implicitly), triggers @a e.
     */
    void at_cancel(const event<> &e) {
	_e->at_cancel(e);
    }

    /** @brief  Trigger event.
     *  @param  v0  First trigger value.
     *  @param  v1  Second trigger value.
     *  @param  v2  Third trigger value.
     *  @param  v3  Fourth trigger value.
     *
     *  Does nothing if event is empty.
     */
    void trigger(const T0 &v0, const T1 &v1, const T2 &v2, const T3 &v3) {
	_e->trigger(v0, v1, v2, v3);
    }

    /** @brief  Trigger event without trigger values.
     *
     *  Does nothing if event is empty.
     */
    void unbound_trigger() {
	_e->complete(true);
    }

    /** @brief  Cancel event.
     *
     *  Does nothing if this event is empty.
     */
    void cancel() {
	_e->complete(false);
    }

    /** @brief  Transfer trigger slots to a new event on @a r.
     *  @param  r   Rendezvous.
     *  @param  i0  First event ID.
     *  @param  i1  Second event ID.
     *  @return  Event on @a r with this event's slots.
     *
     *  If event is empty, returns an empty event.  Otherwise, transfers
     *  this event's slots to a new event on @a r with event IDs @a i0 and
     *  @a i1.  When this event is triggered, its trigger values will be
     *  ignored.
     */
    template <typename R, typename I0, typename I1>
    event<T0, T1, T2, T3> make_rebind(R &r, const I0 &i0, const I1 &i1) {
	if (*this) {
	    event<T0, T1, T2, T3> e(r, i0, i1, *_e->_t0, *_e->_t1, *_e->_t2, *_e->_t3);
	    _e->unbind();
	    return e;
	} else
	    return event<T0, T1, T2, T3>();
    }

    /** @brief  Transfer trigger slots to a new event on @a r.
     *  @param  r   Rendezvous.
     *  @param  i0  First event ID.
     *  @return  Event on @a r with this event's slots.
     */
    template <typename R, typename I0>
    event<T0, T1, T2, T3> make_rebind(R &r, const I0 &i0) {
	if (*this) {
	    event<T0, T1, T2, T3> e(r, i0, *_e->_t0, *_e->_t1, *_e->_t2, *_e->_t3);
	    _e->unbind();
	    return e;
	} else
	    return event<T0, T1, T2, T3>();
    }

    /** @brief  Transfer trigger slots to a new event on @a r.
     *  @param  r   Rendezvous.
     *  @return  Event on @a r with this event's slots.
     */
    template <typename R>
    event<T0, T1, T2, T3> make_rebind(R &r) {
	if (*this) {
	    event<T0, T1, T2, T3> e(r, *_e->_t0, *_e->_t1, *_e->_t2, *_e->_t3);
	    _e->unbind();
	    return e;
	} else
	    return event<T0, T1, T2, T3>();
    }

    /** @brief  Assign this event to the same occurrence as @a e.
     *  @param  e  Source event.
     *
     *  This event's occurrence is canceled if this event was its last
     *  reference.
     */
    event<T0, T1, T2, T3> &operator=(const event<T0, T1, T2, T3> &e) {
	e._e->use();
	_e->unuse();
	_e = e._e;
	return *this;
    }

    /** @internal
     *  @brief  Fetch underlying occurrence.
     *  @return  Underlying occurrence.
     */
    tamerpriv::simple_event *__get_simple() const {
	return _e;
    }
    
  private:

    tamerpriv::eventx<T0, T1, T2, T3> *_e;
    
};


/** @defgroup specialized_events Specialized event classes
 *
 *  Events may be declared with three, two, one, or zero template arguments,
 *  as in <tt>event<T0, T1, T2></tt>, <tt>event<T0, T1></tt>,
 *  <tt>event<T0></tt>, and <tt>event<></tt>.  Each specialized event class
 *  has functions similar to the full-featured event, but with parameters to
 *  constructors and @c trigger methods appropriate to the template arguments.
 *
 *  @{
 */
template <typename T0, typename T1, typename T2>
class event<T0, T1, T2, void> { public:

    event()
	: _e(new tamerpriv::eventx<T0, T1, T2, void>()) {
    }

    template <typename R, typename I0, typename I1>
    event(R &r, const I0 &i0, const I1 &i1, T0 &t0, T1 &t1, T2 &t2)
	: _e(new tamerpriv::eventx<T0, T1, T2, void>(r, i0, i1, t0, t1, t2)) {
    }

    template <typename R, typename I0>
    event(R &r, const I0 &i0, T0 &t0, T1 &t1, T2 &t2)
	: _e(new tamerpriv::eventx<T0, T1, T2, void>(r, i0, t0, t1, t2)) {
    }

    template <typename R>
    event(R &r, T0 &t0, T1 &t1, T2 &t2)
	: _e(new tamerpriv::eventx<T0, T1, T2, void>(r, t0, t1, t2)) {
    }

    event(const event<T0, T1, T2> &e)
	: _e(e._e) {
	_e->use();
    }
    
    ~event() {
	_e->unuse();
    }

    typedef tamerpriv::simple_event::unspecified_bool_type unspecified_bool_type;
    
    operator unspecified_bool_type() const {
	return *_e;
    }

    bool operator!() const {
	return _e->empty();
    }
    
    bool empty() const {
	return _e->empty();
    }

    void at_cancel(const event<> &e) {
	_e->at_cancel(e);
    }

    void trigger(const T0 &v0, const T1 &v1, const T2 &v2) {
	_e->trigger(v0, v1, v2);
    }

    void unbound_trigger() {
	_e->complete(true);
    }

    void cancel() {
	_e->complete(false);
    }

    template <typename R, typename I0, typename I1>
    event<T0, T1, T2> make_rebind(R &r, const I0 &i0, const I1 &i1) {
	if (*this) {
	    event<T0, T1, T2> e(r, i0, i1, *_e->_t0, *_e->_t1, *_e->_t2);
	    _e->unbind();
	    return e;
	} else
	    return event<T0, T1, T2>();
    }

    template <typename R, typename I0>
    event<T0, T1, T2> make_rebind(R &r, const I0 &i0) {
	if (*this) {
	    event<T0, T1, T2> e(r, i0, *_e->_t0, *_e->_t1, *_e->_t2);
	    _e->unbind();
	    return e;
	} else
	    return event<T0, T1, T2>();
    }

    template <typename R>
    event<T0, T1, T2> make_rebind(R &r) {
	if (*this) {
	    event<T0, T1, T2> e(r, *_e->_t0, *_e->_t1, *_e->_t2);
	    _e->unbind();
	    return e;
	} else
	    return event<T0, T1, T2>();
    }

    event<T0, T1, T2> &operator=(const event<T0, T1, T2> &e) {
	e._e->use();
	_e->unuse();
	_e = e._e;
	return *this;
    }
    
    tamerpriv::simple_event *__get_simple() const {
	return _e;
    }
    
  private:

    tamerpriv::eventx<T0, T1, T2, void> *_e;
    
};


template <typename T0, typename T1>
class event<T0, T1, void, void> { public:

    event()
	: _e(new tamerpriv::eventx<T0, T1, void, void>()) {
    }

    template <typename R, typename I0, typename I1>
    event(R &r, const I0 &i0, const I1 &i1, T0 &t0, T1 &t1)
	: _e(new tamerpriv::eventx<T0, T1, void, void>(r, i0, i1, t0, t1)) {
    }

    template <typename R, typename I0>
    event(R &r, const I0 &i0, T0 &t0, T1 &t1)
	: _e(new tamerpriv::eventx<T0, T1, void, void>(r, i0, t0, t1)) {
    }

    template <typename R>
    event(R &r, T0 &t0, T1 &t1)
	: _e(new tamerpriv::eventx<T0, T1, void, void>(r, t0, t1)) {
    }

    event(const event<T0, T1> &e)
	: _e(e._e) {
	_e->use();
    }
    
    ~event() {
	_e->unuse();
    }

    typedef tamerpriv::simple_event::unspecified_bool_type unspecified_bool_type;
    
    operator unspecified_bool_type() const {
	return *_e;
    }

    bool operator!() const {
	return _e->empty();
    }
    
    bool empty() const {
	return _e->empty();
    }

    void at_cancel(const event<> &e) {
	_e->at_cancel(e);
    }

    void trigger(const T0 &v0, const T1 &v1) {
	_e->trigger(v0, v1);
    }

    void unbound_trigger() {
	_e->complete(true);
    }

    void cancel() {
	_e->complete(false);
    }

    template <typename R, typename I0, typename I1>
    event<T0, T1> make_rebind(R &r, const I0 &i0, const I1 &i1) {
	if (*this) {
	    event<T0, T1> e(r, i0, i1, *_e->_t0, *_e->_t1);
	    _e->unbind();
	    return e;
	} else
	    return event<T0, T1>();
    }

    template <typename R, typename I0>
    event<T0, T1> make_rebind(R &r, const I0 &i0) {
	if (*this) {
	    event<T0, T1> e(r, i0, *_e->_t0, *_e->_t1);
	    _e->unbind();
	    return e;
	} else
	    return event<T0, T1>();
    }

    template <typename R>
    event<T0, T1> make_rebind(R &r) {
	if (*this) {
	    event<T0, T1> e(r, *_e->_t0, *_e->_t1);
	    _e->unbind();
	    return e;
	} else
	    return event<T0, T1>();
    }

    event<T0, T1> &operator=(const event<T0, T1> &e) {
	e._e->use();
	_e->unuse();
	_e = e._e;
	return *this;
    }
    
    tamerpriv::simple_event *__get_simple() const {
	return _e;
    }
    
  private:

    tamerpriv::eventx<T0, T1, void, void> *_e;
    
};


template <typename T0>
class event<T0, void, void, void> { public:

    event()
	: _e(new tamerpriv::eventx<T0, void, void, void>()) {
    }

    template <typename R, typename I0, typename I1>
    event(R &r, const I0 &i0, const I1 &i1, T0 &t0)
	: _e(new tamerpriv::eventx<T0, void, void, void>(r, i0, i1, t0)) {
    }

    template <typename R, typename I0>
    event(R &r, const I0 &i0, T0 &t0)
	: _e(new tamerpriv::eventx<T0, void, void, void>(r, i0, t0)) {
    }

    template <typename R>
    event(R &r, T0 &t0)
	: _e(new tamerpriv::eventx<T0, void, void, void>(r, t0)) {
    }

    event(const event<T0> &e)
	: _e(e._e) {
	_e->use();
    }
    
    ~event() {
	_e->unuse();
    }

    typedef tamerpriv::simple_event::unspecified_bool_type unspecified_bool_type;
    
    operator unspecified_bool_type() const {
	return *_e;
    }

    bool operator!() const {
	return _e->empty();
    }
    
    bool empty() const {
	return _e->empty();
    }

    void at_cancel(const event<> &e) {
	_e->at_cancel(e);
    }

    void trigger(const T0 &v0) {
	_e->trigger(v0);
    }

    void unbound_trigger() {
	_e->unbind();
	_e->complete(true);
    }

    void cancel() {
	_e->complete(false);
    }

    template <typename R, typename I0, typename I1>
    event<T0> make_rebind(R &r, const I0 &i0, const I1 &i1) {
	if (*this) {
	    event<T0> e(r, i0, i1, *_e->_t0);
	    _e->unbind();
	    return e;
	} else
	    return event<T0>();
    }

    template <typename R, typename I0>
    event<T0> make_rebind(R &r, const I0 &i0) {
	if (*this) {
	    event<T0> e(r, i0, *_e->_t0);
	    _e->unbind();
	    return e;
	} else
	    return event<T0>();
    }

    template <typename R>
    event<T0> make_rebind(R &r) {
	if (*this) {
	    event<T0> e(r, *_e->_t0);
	    _e->unbind();
	    return e;
	} else
	    return event<T0>();
    }

    T0 *slot0() const {
	return _e->slot0();
    }
    
    event<T0> &operator=(const event<T0> &e) {
	e._e->use();
	_e->unuse();
	_e = e._e;
	return *this;
    }
    
    tamerpriv::simple_event *__get_simple() const {
	return _e;
    }
    
  private:

    tamerpriv::eventx<T0, void, void, void> *_e;
    
};


template <>
class event<void, void, void, void> { public:

    event() {
	if (!tamerpriv::simple_event::dead)
	    tamerpriv::simple_event::make_dead();
	_e = tamerpriv::simple_event::dead;
	_e->use();
    }

    template <typename R, typename I0, typename I1>
    event(R &r, const I0 &i0, const I1 &i1)
	: _e(new tamerpriv::simple_event(r, i0, i1)) {
    }

    template <typename R, typename I0>
    event(R &r, const I0 &i0)
	: _e(new tamerpriv::simple_event(r, i0)) {
    }

    template <typename R>
    explicit event(R &r)
	: _e(new tamerpriv::simple_event(r)) {
    }

    event(const event<> &e)
	: _e(e._e) {
	_e->use();
    }
    
    event(event<> &e)
	: _e(e._e) {
	_e->use();
    }
    
    ~event() {
	_e->unuse();
    }

    typedef tamerpriv::simple_event::unspecified_bool_type unspecified_bool_type;
    
    operator unspecified_bool_type() const {
	return *_e;
    }

    bool operator!() const {
	return _e->empty();
    }
    
    bool empty() const {
	return _e->empty();
    }

    void at_cancel(const event<> &e) {
	_e->at_cancel(e);
    }

    void trigger() {
	_e->complete(true);
    }

    void unbound_trigger() {
	_e->complete(true);
    }

    void cancel() {
	_e->complete(false);
    }

    template <typename R, typename I0, typename I1>
    event<> make_rebind(R &r, const I0 &i0, const I1 &i1) {
	return (*this ? event<>(r, i0, i1) : event<>());
    }

    template <typename R, typename I0>
    event<> make_rebind(R &r, const I0 &i0) {
	return (*this ? event<>(r, i0) : event<>());
    }

    template <typename R>
    event<> make_rebind(R &r) {
	return (*this ? event<>(r) : event<>());
    }

    event<> &operator=(const event<> &e) {
	e._e->use();
	_e->unuse();
	_e = e._e;
	return *this;
    }

    tamerpriv::simple_event *__get_simple() const {
	return _e;
    }

    static inline event<> __take(tamerpriv::simple_event *e) {
	return event<>(take_marker(), e);
    }

  private:

    tamerpriv::simple_event *_e;

    struct take_marker { };
    inline event(const take_marker &, tamerpriv::simple_event *e)
	: _e(e) {
    }
    
    friend class tamerpriv::simple_event;
    
};

/** @} */


/** @defgroup make_event Helper functions for making events
 *
 *  The @c make_event() helper function simplifies event creation.  @c
 *  make_event() automatically selects the right type of event for its
 *  arguments.  There are 3*5 = 15 versions, one for each combination of event
 *  IDs and trigger slots.
 *
 *  @{ */
 
/** @brief  Construct a two-ID, four-slot event on rendezvous @a r.
 *  @param  r   Rendezvous.
 *  @param  i0  First event ID.
 *  @param  i1  Second event ID.
 *  @param  t0  First trigger slot.
 *  @param  t1  Second trigger slot.
 *  @param  t2  Third trigger slot.
 *  @param  t3  Fourth trigger slot.
 *
 *  @note Versions of this function exist for any combination of two, one, or
 *  zero event IDs and four, three, two, one, or zero trigger slots.  For
 *  example, <tt>make_event(r)</tt> creates a zero-ID, zero-slot event on
 *  <tt>rendezvous<> r</tt>, while <tt>make_event(r, 1, i, j)</tt> might
 *  create a one-ID, two-slot event on <tt>rendezvous<int> r</tt>.
 */
template <typename I0, typename I1, typename J0, typename J1, typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> make_event(rendezvous<I0, I1> &r, const J0 &i0, const J1 &i1, T0 &t0, T1 &t1, T2 &t2, T3 &t3)
{
    return event<T0, T1, T2, T3>(r, i0, i1, t0, t1, t2, t3);
}

template <typename I0, typename J0, typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> make_event(rendezvous<I0> &r, const J0 &i0, T0 &t0, T1 &t1, T2 &t2, T3 &t3)
{
    return event<T0, T1, T2, T3>(r, i0, t0, t1, t2, t3);
}

template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> make_event(rendezvous<> &r, T0 &t0, T1 &t1, T2 &t2, T3 &t3)
{
    return event<T0, T1, T2, T3>(r, t0, t1, t2, t3);
}

template <typename I0, typename I1, typename J0, typename J1, typename T0, typename T1, typename T2>
inline event<T0, T1, T2> make_event(rendezvous<I0, I1> &r, const J0 &i0, const J1 &i1, T0 &t0, T1 &t1, T2 &t2)
{
    return event<T0, T1, T2>(r, i0, i1, t0, t1, t2);
}

template <typename I0, typename J0, typename T0, typename T1, typename T2>
inline event<T0, T1, T2> make_event(rendezvous<I0> &r, const J0 &i0, T0 &t0, T1 &t1, T2 &t2)
{
    return event<T0, T1, T2>(r, i0, t0, t1, t2);
}

template <typename T0, typename T1, typename T2>
inline event<T0, T1, T2> make_event(rendezvous<> &r, T0 &t0, T1 &t1, T2 &t2)
{
    return event<T0, T1, T2>(r, t0, t1, t2);
}

template <typename I0, typename I1, typename J0, typename J1, typename T0, typename T1>
inline event<T0, T1> make_event(rendezvous<I0, I1> &r, const J0 &i0, const J1 &i1, T0 &t0, T1 &t1)
{
    return event<T0, T1>(r, i0, i1, t0, t1);
}

template <typename I0, typename J0, typename T0, typename T1>
inline event<T0, T1> make_event(rendezvous<I0> &r, const J0 &i0, T0 &t0, T1 &t1)
{
    return event<T0, T1>(r, i0, t0, t1);
}

template <typename T0, typename T1>
inline event<T0, T1> make_event(rendezvous<> &r, T0 &t0, T1 &t1)
{
    return event<T0, T1>(r, t0, t1);
}

template <typename I0, typename I1, typename J0, typename J1, typename T0>
inline event<T0> make_event(rendezvous<I0, I1> &r, const J0 &i0, const J1 &i1, T0 &t0)
{
    return event<T0>(r, i0, i1, t0);
}

template <typename I0, typename J0, typename T0>
inline event<T0> make_event(rendezvous<I0> &r, const J0 &i0, T0 &t0)
{
    return event<T0>(r, i0, t0);
}

template <typename T0>
inline event<T0> make_event(rendezvous<> &r, T0 &t0)
{
    return event<T0>(r, t0);
}

template <typename I0, typename I1, typename J0, typename J1>
inline event<> make_event(rendezvous<I0, I1> &r, const J0 &i0, const J1 &i1)
{
    return event<>(r, i0, i1);
}

template <typename I0, typename J0>
inline event<> make_event(rendezvous<I0> &r, const J0 &i0)
{
    return event<>(r, i0);
}

inline event<> make_event(rendezvous<> &r)
{
    return event<>(r);
}

/** @} */

}
#endif /* TAMER__EVENT_HH */
