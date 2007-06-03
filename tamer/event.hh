#ifndef TAMER_EVENT_HH
#define TAMER_EVENT_HH 1
#include <tamer/xevent.hh>
#include <functional>
namespace tamer {

/** @file <tamer/event.hh>
 *  @brief  The event template classes and helper functions.
 */

/** @class event tamer/event.hh <tamer/tamer.hh>
 *  @brief  A future occurrence.
 *
 *  Each event object represents a future occurrence, such as the completion
 *  of a network read.  When the expected occurrence actually happens---for
 *  instance, a packet arrives---the event is triggered via its trigger()
 *  method.  A function can wait for the event using @c twait special forms,
 *  which allow event-driven code to block.
 *
 *  The main operations on an event are trigger() and cancel().  trigger()
 *  marks the event's completion, and wakes up any blocked function waiting
 *  for the event.  cancel() marks that the event will never complete; for
 *  example, an event representing the completion of a network read might be
 *  canceled when the connection is closed.  cancel() does not wake up any
 *  blocked function.  An event is implicitly canceled when the last reference
 *  to its occurrence is destroyed.
 *
 *  Events have from zero to four trigger slots of arbitrary type.  For
 *  example, an event of type <tt>event<int, char*, bool></tt> has three
 *  trigger slots with types <tt>int</tt>, <tt>char*</tt>, and <tt>bool</tt>.
 *  The main constructors for <tt>event<int, char*, bool></tt> take three
 *  reference arguments of types <tt>int&</tt>, <tt>char*&</tt>, and
 *  <tt>bool&</tt>, and its trigger() method takes value arguments of types
 *  <tt>int</tt>, <tt>char*</tt>, and <tt>bool</tt>.  Calling <tt>e.trigger(1,
 *  "Hi", false)</tt> will set the trigger slots to the corresponding values.
 *  This can be used to pass information back to the function waiting for the
 *  event.
 *
 *  Each event is in one of two states, active or empty.  An active event is
 *  ready to be triggered.  An empty event has already been triggered or
 *  canceled.  Events can be triggered or canceled at most once; triggering or
 *  canceling an empty event has no additional effect.  The empty() and
 *  operator unspecified_bool_type() member functions test whether an event is
 *  empty or active.
 *
 *  <pre>
 *      Constructors                    Default constructor
 *           |                                  |
 *           v                                  v
 *         ACTIVE   === trigger/cancel ===>   EMPTY   =====+
 *                                              ^    trigger/cancel
 *                                              |          |
 *                                              +==========+
 *  </pre>
 *
 *  Multiple event objects may refer to the same underlying occurrence.
 *  Triggering or canceling an event can thus affect several event objects.
 *  For instance, after an assignment <tt>e1 = e2</tt>, @c e1 and @c e2 refer
 *  to the same occurrence.  Either <tt>e1.trigger()</tt> or
 *  <tt>e2.trigger()</tt> would trigger the underlying occurrence, making both
 *  @c e1 and @c e2 empty.
 *
 *  Events can have associated cancel notifiers, which are triggered when the
 *  event is canceled.  A cancel notifier is simply an <tt>event<></tt>
 *  object.  If the event is triggered, the cancel notifier is itself
 *  canceled.
 *
 *  Events are usually created with the make_event() helper function, which
 *  automatically detects the right type of event to return.
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
     *  @param  s0  First trigger slot.
     *  @param  s1  Second trigger slot.
     *  @param  s2  Third trigger slot.
     *  @param  s3  Fourth trigger slot.
     */
    template <typename R, typename I0, typename I1>
    event(R &r, const I0 &i0, const I1 &i1, T0 &s0, T1 &s1, T2 &s2, T3 &s3)
	: _e(new tamerpriv::eventx<T0, T1, T2, T3>(r, i0, i1, &s0, &s1, &s2, &s3)) {
    }

    /** @brief  Construct a one-ID, four-slot event on rendezvous @a r.
     *  @param  r   Rendezvous.
     *  @param  i0  First event ID.
     *  @param  s0  First trigger slot.
     *  @param  s1  Second trigger slot.
     *  @param  s2  Third trigger slot.
     *  @param  s3  Fourth trigger slot.
     */
    template <typename R, typename I0>
    event(R &r, const I0 &i0, T0 &s0, T1 &s1, T2 &s2, T3 &s3)
	: _e(new tamerpriv::eventx<T0, T1, T2, T3>(r, i0, &s0, &s1, &s2, &s3)) {
    }

    /** @brief  Construct a no-ID, four-slot event on rendezvous @a r.
     *  @param  r   Rendezvous.
     *  @param  s0  First trigger slot.
     *  @param  s1  Second trigger slot.
     *  @param  s2  Third trigger slot.
     *  @param  s3  Fourth trigger slot.
     */
    template <typename R>
    event(R &r, T0 &s0, T1 &s1, T2 &s2, T3 &s3)
	: _e(new tamerpriv::eventx<T0, T1, T2, T3>(r, &s0, &s1, &s2, &s3)) {
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

    /** @brief  Trigger event.
     *  @param  v0  First trigger value.
     *  @param  v1  Second trigger value.
     *  @param  v2  Third trigger value.
     *  @param  v3  Fourth trigger value.
     *
     *  Does nothing if event is empty.
     *
     *  @note   This is a synonym for trigger().
     */
    void operator()(const T0 &v0, const T1 &v1, const T2 &v2, const T3 &v3) {
	_e->trigger(v0, v1, v2, v3);
    }

    typedef void result_type;
    typedef const T0 &first_argument_type;
    typedef const T1 &second_argument_type;
    typedef const T2 &third_argument_type;
    typedef const T3 &fourth_argument_type;

    /** @brief  Trigger event without setting trigger values.
     *
     *  Does nothing if event is empty.
     */
    void unbound_trigger() {
	_e->complete(true);
    }

    /** @brief  Cancel event.
     *
     *  Does nothing if event is empty.
     */
    void cancel() {
	_e->complete(false);
    }

    /** @brief  Register a completion notifier.
     *  @param  e  Completion notifier.
     *
     *  If this event is empty, @a e is triggered immediately.  Otherwise,
     *  when this event is triggered or canceled, triggers @a e.
     */
    void at_complete(const event<> &e) {
	_e->at_complete(e);
    }

    /** @brief  Register a cancel notifier.
     *  @param  e  Cancel notifier.
     *
     *  If this event is empty, @a e is triggered immediately.  Otherwise,
     *  when this event is triggered, cancels @a e; when this event is
     *  canceled, triggers @a e.
     */
    void at_cancel(const event<> &e) {
	_e->at_cancel(e);
    }

    /** @brief  Return a canceler event.
     *  @return  An event that, when triggered, cancels this event.
     *
     *  If this event is empty, returns another empty event.  Otherwise,
     *  when the returned event is triggered, this event will be canceled;
     *  and when this event is triggered or canceled, the returned event will
     *  be canceled.
     */
    inline event<> canceler();

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
     *
     *  @note Versions of this function exist for rendezvous with two, one,
     *  and zero event IDs.
     */
    template <typename R, typename I0, typename I1>
    event<T0, T1, T2, T3> make_rebind(R &r, const I0 &i0, const I1 &i1) {
	if (*this) {
	    event<T0, T1, T2, T3> e(r, i0, i1, *_e->_s0, *_e->_s1, *_e->_s2, *_e->_s3);
	    _e->unbind();
	    return e;
	} else
	    return event<T0, T1, T2, T3>();
    }

    template <typename R, typename I0>
    event<T0, T1, T2, T3> make_rebind(R &r, const I0 &i0) {
	if (*this) {
	    event<T0, T1, T2, T3> e(r, i0, *_e->_s0, *_e->_s1, *_e->_s2, *_e->_s3);
	    _e->unbind();
	    return e;
	} else
	    return event<T0, T1, T2, T3>();
    }

    template <typename R>
    event<T0, T1, T2, T3> make_rebind(R &r) {
	if (*this) {
	    event<T0, T1, T2, T3> e(r, *_e->_s0, *_e->_s1, *_e->_s2, *_e->_s3);
	    _e->unbind();
	    return e;
	} else
	    return event<T0, T1, T2, T3>();
    }

    /** @brief  Assign this event to the same occurrence as @a e.
     *  @param  e  Source event.
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


/** @cond specialized_events
 *
 *  Events may be declared with three, two, one, or zero template arguments,
 *  as in <tt>event<T0, T1, T2></tt>, <tt>event<T0, T1></tt>,
 *  <tt>event<T0></tt>, and <tt>event<></tt>.  Each specialized event class
 *  has functions similar to the full-featured event, but with parameters to
 *  constructors and @c trigger methods appropriate to the template arguments.
 */

template <typename T0, typename T1, typename T2>
class event<T0, T1, T2, void> { public:

    event()
	: _e(new tamerpriv::eventx<T0, T1, T2, void>()) {
    }

    template <typename R, typename I0, typename I1>
    event(R &r, const I0 &i0, const I1 &i1, T0 &s0, T1 &s1, T2 &s2)
	: _e(new tamerpriv::eventx<T0, T1, T2, void>(r, i0, i1, &s0, &s1, &s2)) {
    }

    template <typename R, typename I0>
    event(R &r, const I0 &i0, T0 &s0, T1 &s1, T2 &s2)
	: _e(new tamerpriv::eventx<T0, T1, T2, void>(r, i0, &s0, &s1, &s2)) {
    }

    template <typename R>
    event(R &r, T0 &s0, T1 &s1, T2 &s2)
	: _e(new tamerpriv::eventx<T0, T1, T2, void>(r, &s0, &s1, &s2)) {
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

    void trigger(const T0 &v0, const T1 &v1, const T2 &v2) {
	_e->trigger(v0, v1, v2);
    }

    void operator()(const T0 &v0, const T1 &v1, const T2 &v2) {
	_e->trigger(v0, v1, v2);
    }

    typedef void result_type;
    typedef const T0 &first_argument_type;
    typedef const T1 &second_argument_type;
    typedef const T2 &third_argument_type;

    void unbound_trigger() {
	_e->complete(true);
    }

    void cancel() {
	_e->complete(false);
    }

    void at_complete(const event<> &e) {
	_e->at_complete(e);
    }

    void at_cancel(const event<> &e) {
	_e->at_cancel(e);
    }

    inline event<> canceler();

    template <typename R, typename I0, typename I1>
    event<T0, T1, T2> make_rebind(R &r, const I0 &i0, const I1 &i1) {
	if (*this) {
	    event<T0, T1, T2> e(r, i0, i1, *_e->_s0, *_e->_s1, *_e->_s2);
	    _e->unbind();
	    return e;
	} else
	    return event<T0, T1, T2>();
    }

    template <typename R, typename I0>
    event<T0, T1, T2> make_rebind(R &r, const I0 &i0) {
	if (*this) {
	    event<T0, T1, T2> e(r, i0, *_e->_s0, *_e->_s1, *_e->_s2);
	    _e->unbind();
	    return e;
	} else
	    return event<T0, T1, T2>();
    }

    template <typename R>
    event<T0, T1, T2> make_rebind(R &r) {
	if (*this) {
	    event<T0, T1, T2> e(r, *_e->_s0, *_e->_s1, *_e->_s2);
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
class event<T0, T1, void, void>
    : public std::binary_function<const T0 &, const T1 &, void> { public:

    event()
	: _e(new tamerpriv::eventx<T0, T1, void, void>()) {
    }

    template <typename R, typename I0, typename I1>
    event(R &r, const I0 &i0, const I1 &i1, T0 &s0, T1 &s1)
	: _e(new tamerpriv::eventx<T0, T1, void, void>(r, i0, i1, &s0, &s1)) {
    }

    template <typename R, typename I0>
    event(R &r, const I0 &i0, T0 &s0, T1 &s1)
	: _e(new tamerpriv::eventx<T0, T1, void, void>(r, i0, &s0, &s1)) {
    }

    template <typename R>
    event(R &r, T0 &s0, T1 &s1)
	: _e(new tamerpriv::eventx<T0, T1, void, void>(r, &s0, &s1)) {
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

    void trigger(const T0 &v0, const T1 &v1) {
	_e->trigger(v0, v1);
    }

    void operator()(const T0 &v0, const T1 &v1) {
	_e->trigger(v0, v1);
    }

    void unbound_trigger() {
	_e->complete(true);
    }

    void cancel() {
	_e->complete(false);
    }

    void at_complete(const event<> &e) {
	_e->at_complete(e);
    }

    void at_cancel(const event<> &e) {
	_e->at_cancel(e);
    }

    inline event<> canceler();

    template <typename R, typename I0, typename I1>
    event<T0, T1> make_rebind(R &r, const I0 &i0, const I1 &i1) {
	if (*this) {
	    event<T0, T1> e(r, i0, i1, *_e->_s0, *_e->_s1);
	    _e->unbind();
	    return e;
	} else
	    return event<T0, T1>();
    }

    template <typename R, typename I0>
    event<T0, T1> make_rebind(R &r, const I0 &i0) {
	if (*this) {
	    event<T0, T1> e(r, i0, *_e->_s0, *_e->_s1);
	    _e->unbind();
	    return e;
	} else
	    return event<T0, T1>();
    }

    template <typename R>
    event<T0, T1> make_rebind(R &r) {
	if (*this) {
	    event<T0, T1> e(r, *_e->_s0, *_e->_s1);
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
class event<T0, void, void, void>
    : public std::unary_function<const T0 &, void> { public:

    event()
	: _e(new tamerpriv::eventx<T0, void, void, void>()) {
    }

    template <typename R, typename I0, typename I1>
    event(R &r, const I0 &i0, const I1 &i1, T0 &s0)
	: _e(new tamerpriv::eventx<T0, void, void, void>(r, i0, i1, &s0)) {
    }

    template <typename R, typename I0>
    event(R &r, const I0 &i0, T0 &s0)
	: _e(new tamerpriv::eventx<T0, void, void, void>(r, i0, &s0)) {
    }

    template <typename R>
    event(R &r, T0 &s0)
	: _e(new tamerpriv::eventx<T0, void, void, void>(r, &s0)) {
    }

    template <typename R, typename I0, typename I1>
    event(R &r, const I0 &i0, const I1 &i1, const empty_slot &)
	: _e(new tamerpriv::eventx<T0, void, void, void>(r, i0, i1, 0)) {
    }

    template <typename R, typename I0>
    event(R &r, const I0 &i0, const empty_slot &)
	: _e(new tamerpriv::eventx<T0, void, void, void>(r, i0, 0)) {
    }

    template <typename R>
    event(R &r, const empty_slot &)
	: _e(new tamerpriv::eventx<T0, void, void, void>(r, 0)) {
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

    void trigger(const T0 &v0) {
	_e->trigger(v0);
    }

    void operator()(const T0 &v0) {
	_e->trigger(v0);
    }

    void unbound_trigger() {
	_e->unbind();
	_e->complete(true);
    }

    void cancel() {
	_e->complete(false);
    }

    void at_complete(const event<> &e) {
	_e->at_complete(e);
    }

    void at_cancel(const event<> &e) {
	_e->at_cancel(e);
    }

    inline event<> canceler();

    template <typename R, typename I0, typename I1>
    event<T0> make_rebind(R &r, const I0 &i0, const I1 &i1) {
	if (*this) {
	    event<T0> e(r, i0, i1, *_e->_s0);
	    _e->unbind();
	    return e;
	} else
	    return event<T0>();
    }

    template <typename R, typename I0>
    event<T0> make_rebind(R &r, const I0 &i0) {
	if (*this) {
	    event<T0> e(r, i0, *_e->_s0);
	    _e->unbind();
	    return e;
	} else
	    return event<T0>();
    }

    template <typename R>
    event<T0> make_rebind(R &r) {
	if (*this) {
	    event<T0> e(r, *_e->_s0);
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

    void trigger() {
	_e->complete(true);
    }

    void operator()() {
	_e->complete(true);
    }

    typedef void result_type;

    void unbound_trigger() {
	_e->complete(true);
    }

    void cancel() {
	_e->complete(false);
    }

    void at_complete(const event<> &e) {
	_e->at_complete(e);
    }

    void at_cancel(const event<> &e) {
	_e->at_cancel(e);
    }

    event<> canceler() {
	return _e->canceler();
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

/** @endcond */


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
inline event<T0, T1, T2, T3> make_event(rendezvous<I0, I1> &r, const J0 &i0, const J1 &i1, T0 &s0, T1 &s1, T2 &s2, T3 &s3)
{
    return event<T0, T1, T2, T3>(r, i0, i1, s0, s1, s2, s3);
}

template <typename I0, typename J0, typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> make_event(rendezvous<I0> &r, const J0 &i0, T0 &s0, T1 &s1, T2 &s2, T3 &s3)
{
    return event<T0, T1, T2, T3>(r, i0, s0, s1, s2, s3);
}

template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> make_event(rendezvous<> &r, T0 &s0, T1 &s1, T2 &s2, T3 &s3)
{
    return event<T0, T1, T2, T3>(r, s0, s1, s2, s3);
}

template <typename I0, typename I1, typename J0, typename J1, typename T0, typename T1, typename T2>
inline event<T0, T1, T2> make_event(rendezvous<I0, I1> &r, const J0 &i0, const J1 &i1, T0 &s0, T1 &s1, T2 &s2)
{
    return event<T0, T1, T2>(r, i0, i1, s0, s1, s2);
}

template <typename I0, typename J0, typename T0, typename T1, typename T2>
inline event<T0, T1, T2> make_event(rendezvous<I0> &r, const J0 &i0, T0 &s0, T1 &s1, T2 &s2)
{
    return event<T0, T1, T2>(r, i0, s0, s1, s2);
}

template <typename T0, typename T1, typename T2>
inline event<T0, T1, T2> make_event(rendezvous<> &r, T0 &s0, T1 &s1, T2 &s2)
{
    return event<T0, T1, T2>(r, s0, s1, s2);
}

template <typename I0, typename I1, typename J0, typename J1, typename T0, typename T1>
inline event<T0, T1> make_event(rendezvous<I0, I1> &r, const J0 &i0, const J1 &i1, T0 &s0, T1 &s1)
{
    return event<T0, T1>(r, i0, i1, s0, s1);
}

template <typename I0, typename J0, typename T0, typename T1>
inline event<T0, T1> make_event(rendezvous<I0> &r, const J0 &i0, T0 &s0, T1 &s1)
{
    return event<T0, T1>(r, i0, s0, s1);
}

template <typename T0, typename T1>
inline event<T0, T1> make_event(rendezvous<> &r, T0 &s0, T1 &s1)
{
    return event<T0, T1>(r, s0, s1);
}

template <typename I0, typename I1, typename J0, typename J1, typename T0>
inline event<T0> make_event(rendezvous<I0, I1> &r, const J0 &i0, const J1 &i1, T0 &s0)
{
    return event<T0>(r, i0, i1, s0);
}

template <typename I0, typename J0, typename T0>
inline event<T0> make_event(rendezvous<I0> &r, const J0 &i0, T0 &s0)
{
    return event<T0>(r, i0, s0);
}

template <typename T0>
inline event<T0> make_event(rendezvous<> &r, T0 &s0)
{
    return event<T0>(r, s0);
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

template <typename T0, typename T1, typename T2, typename T3>
inline event<> event<T0, T1, T2, T3>::canceler() {
    return _e->canceler();
}

template <typename T0, typename T1, typename T2>
inline event<> event<T0, T1, T2, void>::canceler() {
    return _e->canceler();
}

template <typename T0, typename T1>
inline event<> event<T0, T1, void, void>::canceler() {
    return _e->canceler();
}

template <typename T0>
inline event<> event<T0, void, void, void>::canceler() {
    return _e->canceler();
}

}
#endif /* TAMER__EVENT_HH */
