#ifndef TAMER_EVENT_HH
#define TAMER_EVENT_HH 1
/* Copyright (c) 2007-2013, Eddie Kohler
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
#include <tamer/xbase.hh>
#include <tamer/rendezvous.hh>
#include <functional>
namespace tamer {

/** @file <tamer/event.hh>
 *  @brief  The event template classes and helper functions.
 */

template <typename T0=void, typename T1=void, typename T2=void, typename T3=void> struct value_pack;
template <typename T0, typename T1, typename T2, typename T3> struct value_pack {
    T0 v0;
    T1 v1;
    T2 v2;
    T3 v3;
};
template <typename T0, typename T1, typename T2> struct value_pack<T0, T1, T2, void> {
    T0 v0;
    T1 v1;
    T2 v2;
};
template <typename T0, typename T1> struct value_pack<T0, T1, void, void> {
    T0 v0;
    T1 v1;
};
template <typename T0> struct value_pack<T0, void, void, void> {
    T0 v0;
};
template <> struct value_pack<void, void, void, void> {
};

/** @class event tamer/event.hh <tamer/tamer.hh>
 *  @brief  A future occurrence.
 *
 *  An event object represents a future occurrence, such as the completion
 *  of a network read. When the occurrence becomes ready---for instance, a
 *  packet arrives---the event object is triggered via its trigger() method
 *  (or, equivalently, by operator()()). Code can block until an event
 *  triggers using @c twait special forms.
 *
 *  Events have from zero to four <em>results</em> of arbitrary type. For
 *  example, an event of type <code>event<int, char*, bool></code> has three
 *  results with types <code>int</code>, <code>char*</code>, and
 *  <code>bool</code>. Most constructors for <code>event<int, char*,
 *  bool></code> take three reference arguments of types <code>int&</code>,
 *  <code>char*&</code>, and <code>bool&</code>, which define where the
 *  results are stored. The <code>event<int, char*, bool>::trigger</code>
 *  method takes value arguments of types <code>int</code>,
 *  <code>char*</code>, and <code>bool</code>. Calling <code>e.trigger(1,
 *  "Hi", false)</code> will set the results to the corresponding values.
 *  This can be used to pass information back to the function waiting for
 *  the event.
 *
 *  Events may be <em>active</em> or <em>empty</em>.  An active event is ready
 *  to be triggered, while an empty event has already been triggered.  Events
 *  can be triggered at most once; triggering an empty event has no additional
 *  effect.  The empty() and operator unspecified_bool_type() member functions
 *  test whether an event is empty or active.
 *
 *  <pre>
 *      Constructors               Default constructor
 *           |                             |
 *           v                             v
 *         ACTIVE   ==== trigger ====>   EMPTY   =====+
 *                                         ^       trigger
 *                                         |          |
 *                                         +==========+
 *  </pre>
 *
 *  Multiple event objects may refer to the same underlying occurrence.
 *  Triggering an event can thus affect several event objects.  For instance,
 *  after an assignment <code>e1 = e2</code>, @c e1 and @c e2 refer to the
 *  same occurrence.  Either <code>e1.trigger()</code> or
 *  <code>e2.trigger()</code> would trigger the underlying occurrence, making
 *  both @c e1 and @c e2 empty.
 *
 *  The unblocker() method returns a version of the event with no results.
 *  The unblocker().trigger() method triggers the underlying occurrence, but
 *  does not change the values stored in the original results. For instance:
 *
 *  @code
 *     tvars { event<int> e; int x = 0; }
 *
 *     twait {
 *        e = make_event(x);
 *        e.trigger(1);
 *     }
 *     printf("%d\n", x);        // will print 1
 *
 *     twait {
 *        e = make_event(x);
 *        e.unblocker().trigger();
 *        e.trigger(2);               // ignored
 *     }
 *     printf("%d\n", x);        // will print 1
 *  @endcode
 *
 *  Tamer automatically triggers the unblocker for any active event when the
 *  last reference to its underlying occurrence is deleted. The results are
 *  not changed. Leaking an active event is usually considered a programming
 *  error, and a message is printed at run time to indicate that an event
 *  triggered abnormally. For example, the following code:
 *
 *  @code
 *     twait { (void) make_event(); }
 *  @endcode
 *
 *  will print a message like "<tt>ex4.tt:11: active event leaked</tt>".
 *  However, it is sometimes convenient to rely on this triggering behavior,
 *  so the error message is turned off for rendezvous declared as volatile:
 *
 *  @code
 *     twait volatile { (void) make_event(); }
 *     tamer::rendezvous<> r(tamer::rvolatile);
 *  @endcode
 *
 *  In any case, deleting the last reference to an empty event is not an
 *  error and does not produce an error message.
 *
 *  An event's <em>trigger notifiers</em> are triggered when the event
 *  itself is triggered. A trigger notifier is simply an
 *  <code>event<></code>.
 *
 *  Events are usually created with the make_event() helper function, which
 *  automatically detects the right type of event to return.
 */
template <typename T0, typename T1, typename T2, typename T3>
class event {
  public:

    typedef void result_type;
    typedef const T0 &first_argument_type;
    typedef const T1 &second_argument_type;
    typedef const T2 &third_argument_type;
    typedef const T3 &fourth_argument_type;

    typedef tamerpriv::simple_event::unspecified_bool_type unspecified_bool_type;

    inline event() TAMER_NOEXCEPT;
    template <typename R, typename I0, typename I1>
    inline event(R& r, const I0& i0, const I1& i1,
		 T0& x0, T1& x1, T2& x2, T3& x3);
    template <typename R, typename I0, typename I1>
    inline event(R& r, const I0& i0, const I1& i1,
		 value_pack<T0, T1, T2, T3>& x);
    template <typename R, typename I0>
    inline event(R& r, const I0& i0, T0& x0, T1& x1, T2& x2, T3& x3);
    template <typename R, typename I0>
    inline event(R& r, const I0& i0, value_pack<T0, T1, T2, T3>& x);
    template <typename R>
    inline event(R& r, T0& x0, T1& x1, T2& x2, T3& x3);
    template <typename R>
    inline event(R& r, value_pack<T0, T1, T2, T3>& x);
    inline event(const event<T0, T1, T2, T3>& x) TAMER_NOEXCEPT;
#if TAMER_HAVE_CXX_RVALUE_REFERENCES
    inline event(event<T0, T1, T2, T3>&& x) TAMER_NOEXCEPT;
#endif
    inline ~event() TAMER_NOEXCEPT;

    inline operator unspecified_bool_type() const;
    inline bool operator!() const;
    inline bool empty() const;

    inline void trigger(const T0& v0, const T1& v1, const T2& v2, const T3& v3);
    inline void trigger(const value_pack<T0, T1, T2, T3>& v);
    inline void operator()(const T0& v0, const T1& v1, const T2& v2, const T3& v3);
    inline void operator()(const value_pack<T0, T1, T2, T3>& v);
    inline void unblock() TAMER_NOEXCEPT;

    inline void at_trigger(const event<>& e);
#if TAMER_HAVE_CXX_RVALUE_REFERENCES
    inline void at_trigger(event<>&& e);
#endif

    inline event<> unblocker() const TAMER_NOEXCEPT;
    inline event<> bind_all() const TAMER_DEPRECATEDATTR;

    inline event<T0, T1, T2, T3>& operator=(const event<T0, T1, T2, T3>& x) TAMER_NOEXCEPT;
#if TAMER_HAVE_CXX_RVALUE_REFERENCES
    inline event<T0, T1, T2, T3>& operator=(event<T0, T1, T2, T3>&& x) TAMER_NOEXCEPT;
#endif

    inline tamerpriv::simple_event* __get_simple() const;

  private:
    tamerpriv::simple_event* se_;
    T0 *_s0;
    T1 *_s1;
    T2 *_s2;
    T3 *_s3;
};


/** @cond specialized_events
 *
 *  Events may be declared with three, two, one, or zero template arguments,
 *  as in <code>event<T0, T1, T2></code>, <code>event<T0, T1></code>,
 *  <code>event<T0></code>, and <code>event<></code>.  Each specialized event class
 *  has functions similar to the full-featured event, but with parameters to
 *  constructors and @c trigger methods appropriate to the template arguments.
 */

template <typename T0, typename T1, typename T2>
class event<T0, T1, T2, void> { public:

    typedef void result_type;
    typedef const T0 &first_argument_type;
    typedef const T1 &second_argument_type;
    typedef const T2 &third_argument_type;

    typedef tamerpriv::simple_event::unspecified_bool_type unspecified_bool_type;

    inline event() TAMER_NOEXCEPT;
    template <typename R, typename I0, typename I1>
    inline event(R& r, const I0& i0, const I1& i1, T0& x0, T1& x1, T2& x2);
    template <typename R, typename I0, typename I1>
    inline event(R& r, const I0& i0, const I1& i1, value_pack<T0, T1, T2>& x);
    template <typename R, typename I0>
    inline event(R& r, const I0& i0, T0& x0, T1& x1, T2& x2);
    template <typename R, typename I0>
    inline event(R& r, const I0& i0, value_pack<T0, T1, T2>& x);
    template <typename R>
    inline event(R& r, T0& x0, T1& x1, T2& x2);
    template <typename R>
    inline event(R& r, value_pack<T0, T1, T2>& x);

    event(const event<T0, T1, T2>& x) TAMER_NOEXCEPT
	: se_(x.se_), _s0(x._s0), _s1(x._s1), _s2(x._s2) {
	tamerpriv::simple_event::use(se_);
    }

#if TAMER_HAVE_CXX_RVALUE_REFERENCES
    event(event<T0, T1, T2>&& x) TAMER_NOEXCEPT
	: se_(x.se_), _s0(x._s0), _s1(x._s1), _s2(x._s2) {
	x.se_ = 0;
    }
#endif

    ~event() TAMER_NOEXCEPT {
	tamerpriv::simple_event::unuse(se_);
    }

    operator unspecified_bool_type() const {
	return se_ ? (unspecified_bool_type) *se_ : 0;
    }

    bool operator!() const {
	return !se_ || se_->empty();
    }

    bool empty() const {
	return !se_ || se_->empty();
    }

    inline void trigger(const T0 &v0, const T1 &v1, const T2 &v2);
    inline void trigger(const value_pack<T0, T1, T2>& v);
    inline void operator()(const T0 &v0, const T1 &v1, const T2 &v2);
    inline void operator()(const value_pack<T0, T1, T2>& v);
    inline void unblock() TAMER_NOEXCEPT;

    inline void at_trigger(const event<>& e);
#if TAMER_HAVE_CXX_RVALUE_REFERENCES
    inline void at_trigger(event<>&& e);
#endif

    inline event<> unblocker() const TAMER_NOEXCEPT;
    inline event<> bind_all() const TAMER_DEPRECATEDATTR;

    event<T0, T1, T2>& operator=(const event<T0, T1, T2>& x) TAMER_NOEXCEPT {
	tamerpriv::simple_event::use(x.se_);
	tamerpriv::simple_event::unuse(se_);
	se_ = x.se_;
	_s0 = x._s0;
	_s1 = x._s1;
	_s2 = x._s2;
	return *this;
    }

#if TAMER_HAVE_CXX_RVALUE_REFERENCES
    event<T0, T1, T2>& operator=(event<T0, T1, T2>&& x) TAMER_NOEXCEPT {
	tamerpriv::simple_event *se = se_;
	se_ = x.se_;
	_s0 = x._s0;
	_s1 = x._s1;
	_s2 = x._s2;
	x.se_ = se;
	return *this;
    }
#endif

    tamerpriv::simple_event* __get_simple() const {
	return se_;
    }

  private:

    tamerpriv::simple_event* se_;
    T0 *_s0;
    T1 *_s1;
    T2 *_s2;

};


template <typename T0, typename T1>
class event<T0, T1, void, void>
    : public std::binary_function<const T0 &, const T1 &, void> { public:

    typedef tamerpriv::simple_event::unspecified_bool_type unspecified_bool_type;

    inline event() TAMER_NOEXCEPT;
    template <typename R, typename I0, typename I1>
    inline event(R& r, const I0& i0, const I1& i1, T0& x0, T1& x1);
    template <typename R, typename I0, typename I1>
    inline event(R& r, const I0& i0, const I1& i1, value_pack<T0, T1>& x);
    template <typename R, typename I0>
    inline event(R& r, const I0& i0, T0& x0, T1& x1);
    template <typename R, typename I0>
    inline event(R& r, const I0& i0, value_pack<T0, T1>& x);
    template <typename R>
    inline event(R& r, T0& x0, T1& x1);
    template <typename R>
    inline event(R& r, value_pack<T0, T1>& x);

    event(const event<T0, T1>& x) TAMER_NOEXCEPT
	: se_(x.se_), _s0(x._s0), _s1(x._s1) {
	tamerpriv::simple_event::use(se_);
    }

#if TAMER_HAVE_CXX_RVALUE_REFERENCES
    event(event<T0, T1>&& x) TAMER_NOEXCEPT
	: se_(x.se_), _s0(x._s0), _s1(x._s1) {
	x.se_ = 0;
    }
#endif

    ~event() TAMER_NOEXCEPT {
	tamerpriv::simple_event::unuse(se_);
    }

    operator unspecified_bool_type() const {
	return se_ ? (unspecified_bool_type) *se_ : 0;
    }

    bool operator!() const {
	return !se_ || se_->empty();
    }

    bool empty() const {
	return !se_ || se_->empty();
    }

    inline void trigger(const T0& v0, const T1& v1);
    inline void trigger(const value_pack<T0, T1>& v);
    inline void operator()(const T0& v0, const T1& v1);
    inline void operator()(const value_pack<T0, T1>& v);
    inline void unblock() TAMER_NOEXCEPT;

    inline void at_trigger(const event<>& e);
#if TAMER_HAVE_CXX_RVALUE_REFERENCES
    inline void at_trigger(event<>&& e);
#endif

    inline event<> unblocker() const TAMER_NOEXCEPT;
    inline event<> bind_all() const TAMER_DEPRECATEDATTR;

    event<T0, T1>& operator=(const event<T0, T1>& x) TAMER_NOEXCEPT {
	tamerpriv::simple_event::use(x.se_);
	tamerpriv::simple_event::unuse(se_);
	se_ = x.se_;
	_s0 = x._s0;
	_s1 = x._s1;
	return *this;
    }

#if TAMER_HAVE_CXX_RVALUE_REFERENCES
    event<T0, T1>& operator=(event<T0, T1>&& x) TAMER_NOEXCEPT {
	tamerpriv::simple_event *se = se_;
	se_ = x.se_;
	_s0 = x._s0;
	_s1 = x._s1;
	x.se_ = se;
	return *this;
    }
#endif

    tamerpriv::simple_event* __get_simple() const {
	return se_;
    }

  private:

    tamerpriv::simple_event* se_;
    T0 *_s0;
    T1 *_s1;

};


template <typename T0>
class event<T0, void, void, void>
    : public std::unary_function<const T0 &, void> { public:

    typedef tamerpriv::simple_event::unspecified_bool_type unspecified_bool_type;

    inline event() TAMER_NOEXCEPT;
    template <typename R, typename I0, typename I1>
    inline event(R& r, const I0& i0, const I1& i1, T0& x0);
    template <typename R, typename I0, typename I1>
    inline event(R& r, const I0& i0, const I1& i1, value_pack<T0>& x);
    template <typename R, typename I0>
    inline event(R& r, const I0& i0, T0& x0);
    template <typename R, typename I0>
    inline event(R& r, const I0& i0, value_pack<T0>& x);
    template <typename R>
    inline event(R& r, T0& x0);
    template <typename R>
    inline event(R& r, value_pack<T0>& x);

    inline event(event<> e, const no_result& marker) TAMER_NOEXCEPT;

    event(const event<T0>& x) TAMER_NOEXCEPT
	: se_(x.se_), _s0(x._s0) {
	tamerpriv::simple_event::use(se_);
    }

#if TAMER_HAVE_CXX_RVALUE_REFERENCES
    event(event<T0>&& x) TAMER_NOEXCEPT
	: se_(x.se_), _s0(x._s0) {
	x.se_ = 0;
    }
#endif

#if TAMER_HAVE_PREEVENT
    template <typename R> inline event(preevent<R, T0>&& x);
#endif

    ~event() TAMER_NOEXCEPT {
	tamerpriv::simple_event::unuse(se_);
    }

    operator unspecified_bool_type() const {
	return se_ ? (unspecified_bool_type) *se_ : 0;
    }

    bool operator!() const {
	return !se_ || se_->empty();
    }

    bool empty() const {
	return !se_ || se_->empty();
    }

    inline void trigger(const T0& v0);
#if TAMER_HAVE_CXX_RVALUE_REFERENCES
    inline void trigger(T0&& v0);
#endif
    inline void trigger(const value_pack<T0>& v);
    template <typename V0>
    inline void trigger(V0 v0);
    inline void operator()(const T0& v0);
#if TAMER_HAVE_CXX_RVALUE_REFERENCES
    inline void operator()(T0&& v0);
#endif
    inline void operator()(const value_pack<T0>& v);
    template <typename V0>
    inline void operator()(V0 v0);
    inline void unblock() TAMER_NOEXCEPT;

    inline bool has_result() const TAMER_NOEXCEPT;
    inline T0& result() const TAMER_NOEXCEPT;
    inline T0* result_pointer() const TAMER_NOEXCEPT;

    inline void at_trigger(const event<>& e);
#if TAMER_HAVE_CXX_RVALUE_REFERENCES
    inline void at_trigger(event<>&& e);
#endif

    inline event<> unblocker() const TAMER_NOEXCEPT;
    inline event<> bind_all() const TAMER_DEPRECATEDATTR;

    event<T0>& operator=(const event<T0>& x) TAMER_NOEXCEPT {
	tamerpriv::simple_event::use(x.se_);
	tamerpriv::simple_event::unuse(se_);
	se_ = x.se_;
	_s0 = x._s0;
	return *this;
    }

#if TAMER_HAVE_CXX_RVALUE_REFERENCES
    event<T0>& operator=(event<T0>&& x) TAMER_NOEXCEPT {
	tamerpriv::simple_event *se = se_;
	se_ = x.se_;
	_s0 = x._s0;
	x.se_ = se;
	return *this;
    }
#endif

#if TAMER_HAVE_PREEVENT
    template <typename R> event<T0>& operator=(preevent<R, T0>&& x) {
        return *this = event<T0>(std::move(x));
    }
#endif

    tamerpriv::simple_event* __get_simple() const {
	return se_;
    }

    tamerpriv::simple_event* __take_simple() {
	tamerpriv::simple_event *se = se_;
	se_ = 0;
	return se;
    }

    static inline event<T0> __make(tamerpriv::simple_event *se, T0 *s0) {
	return event<T0>(take_marker(), se, s0);
    }

  private:

    tamerpriv::simple_event* se_;
    T0 *_s0;

    struct take_marker { };
    inline event(const take_marker &, tamerpriv::simple_event *se, T0 *s0)
	: se_(se), _s0(s0) {
    }

};


template <>
class event<void, void, void, void> { public:

    typedef void result_type;

    typedef tamerpriv::simple_event::unspecified_bool_type unspecified_bool_type;

    inline event() TAMER_NOEXCEPT;
    template <typename R, typename I0, typename I1>
    inline event(R& r, const I0& i0, const I1& i1);
    template <typename R, typename I0, typename I1>
    inline event(R& r, const I0& i0, const I1& i1, value_pack<>& x);
    template <typename R, typename I0>
    inline event(R& r, const I0& i0);
    template <typename R, typename I0>
    inline event(R& r, const I0& i0, value_pack<>& x);
    template <typename R>
    inline event(R& r);
    template <typename R>
    inline event(R& r, value_pack<>& x);

    event(const event<>& x) TAMER_NOEXCEPT
        : se_(x.se_) {
	tamerpriv::simple_event::use(se_);
    }

    event(event<>& x) TAMER_NOEXCEPT
	: se_(x.se_) {
	tamerpriv::simple_event::use(se_);
    }

#if TAMER_HAVE_CXX_RVALUE_REFERENCES
    event(event<>&& x) TAMER_NOEXCEPT
	: se_(x.se_) {
	x.se_ = 0;
    }
#endif

#if TAMER_HAVE_PREEVENT
    template <typename R> inline event(preevent<R>&& x);
#endif

    ~event() TAMER_NOEXCEPT {
	tamerpriv::simple_event::unuse(se_);
    }

    operator unspecified_bool_type() const {
	return se_ ? (unspecified_bool_type) *se_ : 0;
    }

    bool operator!() const {
	return !se_ || se_->empty();
    }

    bool empty() const {
	return !se_ || se_->empty();
    }

    inline void trigger() TAMER_NOEXCEPT;
    inline void trigger(const value_pack<>& v) TAMER_NOEXCEPT;
    inline void operator()() TAMER_NOEXCEPT;
    inline void operator()(const value_pack<>& v) TAMER_NOEXCEPT;
    inline void unblock() TAMER_NOEXCEPT;

    inline void at_trigger(const event<>& e) {
	tamerpriv::simple_event::use(e.__get_simple());
	tamerpriv::simple_event::at_trigger(se_, e.__get_simple());
    }
#if TAMER_HAVE_CXX_RVALUE_REFERENCES
    inline void at_trigger(event<>&& e) {
	tamerpriv::simple_event::at_trigger(se_, e.__take_simple());
    }
#endif

    event<>& unblocker() TAMER_NOEXCEPT {
	return *this;
    }

    event<> unblocker() const TAMER_NOEXCEPT {
	return *this;
    }

    event<>& bind_all() TAMER_DEPRECATEDATTR;
    event<> bind_all() const TAMER_DEPRECATEDATTR;

    event<>& operator=(const event<>& x) TAMER_NOEXCEPT {
	tamerpriv::simple_event::use(x.se_);
	tamerpriv::simple_event::unuse(se_);
	se_ = x.se_;
	return *this;
    }

#if TAMER_HAVE_CXX_RVALUE_REFERENCES
    event<>& operator=(event<>&& x) TAMER_NOEXCEPT {
	tamerpriv::simple_event *se = se_;
	se_ = x.se_;
	x.se_ = se;
	return *this;
    }
#endif

#if TAMER_HAVE_PREEVENT
    template <typename R> event<>& operator=(preevent<R>&& x) {
        return *this = event<>(std::move(x));
    }
#endif

    tamerpriv::simple_event* __get_simple() const {
	return se_;
    }

    tamerpriv::simple_event* __take_simple() {
	tamerpriv::simple_event *se = se_;
	se_ = 0;
	return se;
    }

    static inline event<> __make(tamerpriv::simple_event *se) {
	return event<>(take_marker(), se);
    }

  private:

    mutable tamerpriv::simple_event* se_;

    struct take_marker { };
    inline event(const take_marker &, tamerpriv::simple_event *se)
	: se_(se) {
    }

    friend class tamerpriv::simple_event;

};


#if TAMER_HAVE_PREEVENT
template <typename R, typename T0>
class preevent : public std::unary_function<const T0&, void> { public:

    typedef tamerpriv::simple_event::unspecified_bool_type unspecified_bool_type;

    inline TAMER_CONSTEXPR preevent(R& r, T0& x0) TAMER_NOEXCEPT
        : r_(&r), s0_(&x0) {
    }

    inline TAMER_CONSTEXPR preevent(const preevent<R, T0>& x) TAMER_NOEXCEPT
        : r_(x.r_), s0_(x.s0_) {
    }

    inline preevent(preevent<R, T0>&& x) TAMER_NOEXCEPT
	: r_(x.r_), s0_(x.s0_) {
	x.r_ = 0;
    }

    inline operator event<T0>() {
        return event<T0>(std::move(*this));
    }

    operator unspecified_bool_type() const {
	return r_ ? (unspecified_bool_type) &tamerpriv::simple_event::empty : 0;
    }

    bool operator!() const {
	return !r_;
    }

    bool empty() const {
	return !r_;
    }

    inline void trigger(const T0& v0) {
        if (r_) {
            *s0_ = v0;
            r_ = 0;
        }
    }
    inline void trigger(T0&& v0) {
        if (r_) {
            *s0_ = std::move(v0);
            r_ = 0;
        }
    }
    inline void trigger(const value_pack<T0>& v) {
        trigger(v.v0);
    }
    template <typename V0>
    inline void trigger(V0 v0) {
        if (r_) {
            *s0_ = v0;
            r_ = 0;
        }
    }
    inline void operator()(const T0& v0) {
        trigger(v0);
    }
    inline void operator()(T0&& v0) {
        trigger(std::move(v0));
    }
    inline void operator()(const value_pack<T0>& v) {
        trigger(v.v0);
    }
    template <typename V0>
    inline void operator()(V0 v0) {
        trigger(v0);
    }
    inline void unblock() TAMER_NOEXCEPT {
        r_ = 0;
    }

    inline bool has_result() const TAMER_NOEXCEPT {
        return r_;
    }
    inline T0& result() const TAMER_NOEXCEPT {
        assert(r_);
        return *s0_;
    }
    inline T0* result_pointer() const TAMER_NOEXCEPT {
        return r_ ? s0_ : 0;
    }

  private:
    R* r_;
    T0* s0_;

    template <typename TT0, typename TT1, typename TT2, typename TT3>
    friend class event;
};

template <typename R>
class preevent<R, void> { public:

    typedef void result_type;

    typedef tamerpriv::simple_event::unspecified_bool_type unspecified_bool_type;

    inline TAMER_CONSTEXPR preevent(R& r) TAMER_NOEXCEPT
        : r_(&r) {
    }

    inline TAMER_CONSTEXPR preevent(const preevent<R>& x) TAMER_NOEXCEPT
        : r_(x.r_) {
    }

    inline preevent(preevent<R>&& x) TAMER_NOEXCEPT
	: r_(x.r_) {
	x.r_ = 0;
    }

    inline operator event<>() {
        return event<>(std::move(*this));
    }

    operator unspecified_bool_type() const {
	return r_ ? (unspecified_bool_type) &tamerpriv::simple_event::empty : 0;
    }

    bool operator!() const {
	return !r_;
    }

    bool empty() const {
	return !r_;
    }

    inline void trigger() {
        r_ = 0;
    }
    inline void trigger(const value_pack<>&) {
        trigger();
    }
    inline void operator()() {
        trigger();
    }
    inline void operator()(const value_pack<>&) {
        trigger();
    }
    inline void unblock() TAMER_NOEXCEPT {
        r_ = 0;
    }

  private:
    R* r_;

    template <typename TT0, typename TT1, typename TT2, typename TT3>
    friend class event;
};
#endif

/** @endcond */


/** @brief  Default constructor creates an empty event. */
template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3>::event() TAMER_NOEXCEPT
    : se_(0), _s0(0), _s1(0), _s2(0), _s3(0) {
}

/** @brief  Construct a two-ID, four-result event on rendezvous @a r.
 *  @param  r   Rendezvous.
 *  @param  i0  First event ID.
 *  @param  i1  Second event ID.
 *  @param  x0  First result.
 *  @param  x1  Second result.
 *  @param  x2  Third result.
 *  @param  x3  Fourth result.
 */
template <typename T0, typename T1, typename T2, typename T3>
template <typename R, typename I0, typename I1>
inline event<T0, T1, T2, T3>::event(R& r, const I0& i0, const I1& i1,
				    T0& x0, T1& x1, T2& x2, T3& x3)
    : se_(new tamerpriv::simple_event(r, i0, i1)),
      _s0(&x0), _s1(&x1), _s2(&x2), _s3(&x3) {
}

/** @brief  Construct a one-ID, four-result event on rendezvous @a r.
 *  @param  r   Rendezvous.
 *  @param  i0  First event ID.
 *  @param  x0  First result.
 *  @param  x1  Second result.
 *  @param  x2  Third result.
 *  @param  x3  Fourth result.
 */
template <typename T0, typename T1, typename T2, typename T3>
template <typename R, typename I0>
inline event<T0, T1, T2, T3>::event(R& r, const I0& i0,
				    T0& x0, T1& x1, T2& x2, T3& x3)
    : se_(new tamerpriv::simple_event(r, i0)),
      _s0(&x0), _s1(&x1), _s2(&x2), _s3(&x3) {
}

/** @brief  Construct a no-ID, four-result event on rendezvous @a r.
 *  @param  r   Rendezvous.
 *  @param  x0  First result.
 *  @param  x1  Second result.
 *  @param  x2  Third result.
 *  @param  x3  Fourth result.
 */
template <typename T0, typename T1, typename T2, typename T3>
template <typename R>
inline event<T0, T1, T2, T3>::event(R& r, T0& x0, T1& x1, T2& x2, T3& x3)
    : se_(new tamerpriv::simple_event(r)),
      _s0(&x0), _s1(&x1), _s2(&x2), _s3(&x3) {
}

/** @brief  Copy-construct event from @a x.
 *  @param  x  Source event.
 */
template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3>::event(const event<T0, T1, T2, T3>& x) TAMER_NOEXCEPT
    : se_(x.se_), _s0(x._s0), _s1(x._s1), _s2(x._s2), _s3(x._s3) {
    tamerpriv::simple_event::use(se_);
}

#if TAMER_HAVE_CXX_RVALUE_REFERENCES
/** @brief  Move-construct event from @a x.
 *  @param  x  Source event.
 */
template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3>::event(event<T0, T1, T2, T3>&& x) TAMER_NOEXCEPT
    : se_(x.se_), _s0(x._s0), _s1(x._s1), _s2(x._s2), _s3(x._s3) {
    x.se_ = 0;
}
#endif

/** @brief  Destroy the event instance.
 *  @note   The underlying occurrence is canceled if this event was the
 *          last remaining reference.
 */
template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3>::~event() TAMER_NOEXCEPT {
    tamerpriv::simple_event::unuse(se_);
}

/** @brief  Test if event is active.
 *  @return  True if event is active, false if it is empty. */
template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3>::operator unspecified_bool_type() const {
    return se_ ? (unspecified_bool_type) *se_ : 0;
}

/** @brief  Test if event is empty.
 *  @return  True if event is empty, false if it is active. */
template <typename T0, typename T1, typename T2, typename T3>
inline bool event<T0, T1, T2, T3>::operator!() const {
    return !se_ || se_->empty();
}

/** @brief  Test if event is empty.
 *  @return  True if event is empty, false if it is active. */
template <typename T0, typename T1, typename T2, typename T3>
inline bool event<T0, T1, T2, T3>::empty() const {
    return !se_ || se_->empty();
}

/** @brief  Trigger event.
 *  @param  v0  First trigger value.
 *  @param  v1  Second trigger value.
 *  @param  v2  Third trigger value.
 *  @param  v3  Fourth trigger value.
 *
 *  Does nothing if event is empty.
 */
template <typename T0, typename T1, typename T2, typename T3>
inline void event<T0, T1, T2, T3>::trigger(const T0 &v0, const T1 &v1,
					   const T2 &v2, const T3 &v3) {
    if (se_ && *se_) {
	if (_s0) *_s0 = v0;
	if (_s1) *_s1 = v1;
	if (_s2) *_s2 = v2;
	if (_s3) *_s3 = v3;
	se_->simple_trigger(true);
	se_ = 0;
    }
}

/** @brief  Trigger event.
 *  @param  v  Value pack.
 *
 *  Does nothing if event is empty.
 */
template <typename T0, typename T1, typename T2, typename T3>
inline void event<T0, T1, T2, T3>::trigger(const value_pack<T0, T1, T2, T3>& v) {
    trigger(v.v0, v.v1, v.v2, v.v3);
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
template <typename T0, typename T1, typename T2, typename T3>
inline void event<T0, T1, T2, T3>::operator()(const T0 &v0, const T1 &v1,
					      const T2 &v2, const T3 &v3) {
    trigger(v0, v1, v2, v3);
}

/** @brief  Trigger event.
 *  @param  v  Value pack.
 *
 *  Does nothing if event is empty.
 *
 *  @note   This is a synonym for trigger().
 */
template <typename T0, typename T1, typename T2, typename T3>
inline void event<T0, T1, T2, T3>::operator()(const value_pack<T0, T1, T2, T3>& v) {
    trigger(v);
}

/** @brief  Unblock event.
 *
 *  Like trigger(), but does not change results.
 *
 *  Does nothing if event is empty.
 */
template <typename T0, typename T1, typename T2, typename T3>
inline void event<T0, T1, T2, T3>::unblock() TAMER_NOEXCEPT {
    tamerpriv::simple_event::simple_trigger(se_, false);
    se_ = 0;
}

/** @brief  Register a trigger notifier.
 *  @param  e  Trigger notifier.
 *
 *  If this event is empty, @a e is triggered immediately.  Otherwise,
 *  when this event is triggered, triggers @a e.
 */
template <typename T0, typename T1, typename T2, typename T3>
inline void event<T0, T1, T2, T3>::at_trigger(const event<>& e) {
    tamerpriv::simple_event::use(e.__get_simple());
    tamerpriv::simple_event::at_trigger(se_, e.__get_simple());
}

#if TAMER_HAVE_CXX_RVALUE_REFERENCES
/** @overload */
template <typename T0, typename T1, typename T2, typename T3>
inline void event<T0, T1, T2, T3>::at_trigger(event<>&& e) {
    tamerpriv::simple_event::at_trigger(se_, e.__take_simple());
}
#endif

/** @brief  Return a no-result event for the same occurrence as @a e.
 *  @return  New event.
 *
 *  The returned event refers to the same occurrence as this event, so
 *  triggering either event makes both events empty. The returned event
 *  has no results, however, and unblocker().trigger() will leave
 *  this event's results unchanged.
 */
template <typename T0, typename T1, typename T2, typename T3>
inline event<> event<T0, T1, T2, T3>::unblocker() const TAMER_NOEXCEPT {
    tamerpriv::simple_event::use(se_);
    return event<>::__make(se_);
}

template <typename T0, typename T1, typename T2, typename T3>
inline event<> event<T0, T1, T2, T3>::bind_all() const {
    return unblocker();
}

/** @brief  Assign this event to @a x.
 *  @param  x  Source event.
 */
template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3>& event<T0, T1, T2, T3>::operator=(const event<T0, T1, T2, T3>& x) TAMER_NOEXCEPT {
    tamerpriv::simple_event::use(x.se_);
    tamerpriv::simple_event::unuse(se_);
    se_ = x.se_;
    _s0 = x._s0;
    _s1 = x._s1;
    _s2 = x._s2;
    _s3 = x._s3;
    return *this;
}

#if TAMER_HAVE_CXX_RVALUE_REFERENCES
/** @brief  Move-assign this event to @a x.
 *  @param  x  Source event.
 */
template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3>& event<T0, T1, T2, T3>::operator=(event<T0, T1, T2, T3>&& x) TAMER_NOEXCEPT {
    tamerpriv::simple_event *se = se_;
    se_ = x.se_;
    _s0 = x._s0;
    _s1 = x._s1;
    _s2 = x._s2;
    _s3 = x._s3;
    x.se_ = se;
    return *this;
}
#endif

/** @internal
 *  @brief  Fetch underlying occurrence.
 *  @return  Underlying occurrence.
 */
template <typename T0, typename T1, typename T2, typename T3>
inline tamerpriv::simple_event *event<T0, T1, T2, T3>::__get_simple() const {
    return se_;
}


/** @cond never */

template <typename T0, typename T1, typename T2>
inline event<T0, T1, T2>::event() TAMER_NOEXCEPT
    : se_(0), _s0(0), _s1(0), _s2(0) {
}

template <typename T0, typename T1, typename T2>
template <typename R, typename I0, typename I1>
inline event<T0, T1, T2>::event(R& r, const I0& i0, const I1& i1,
				T0& x0, T1& x1, T2& x2)
    : se_(new tamerpriv::simple_event(r, i0, i1)),
      _s0(&x0), _s1(&x1), _s2(&x2) {
}

template <typename T0, typename T1, typename T2>
template <typename R, typename I0, typename I1>
inline event<T0, T1, T2>::event(R& r, const I0& i0, const I1& i1,
				value_pack<T0, T1, T2>& s)
    : se_(new tamerpriv::simple_event(r, i0, i1)),
      _s0(&s.v0), _s1(&s.v1), _s2(&s.v2) {
}

template <typename T0, typename T1, typename T2>
template <typename R, typename I0>
inline event<T0, T1, T2>::event(R& r, const I0& i0, T0& x0, T1& x1, T2& x2)
    : se_(new tamerpriv::simple_event(r, i0)),
      _s0(&x0), _s1(&x1), _s2(&x2) {
}

template <typename T0, typename T1, typename T2>
template <typename R, typename I0>
inline event<T0, T1, T2>::event(R& r, const I0& i0, value_pack<T0, T1, T2>& s)
    : se_(new tamerpriv::simple_event(r, i0)),
      _s0(&s.v0), _s1(&s.v1), _s2(&s.v2) {
}

template <typename T0, typename T1, typename T2>
template <typename R>
inline event<T0, T1, T2>::event(R& r, T0& x0, T1& x1, T2& x2)
    : se_(new tamerpriv::simple_event(r)), _s0(&x0), _s1(&x1), _s2(&x2) {
}

template <typename T0, typename T1, typename T2>
template <typename R>
inline event<T0, T1, T2>::event(R& r, value_pack<T0, T1, T2>& s)
    : se_(new tamerpriv::simple_event(r)), _s0(&s.v0), _s1(&s.v1), _s2(&s.v2) {
}

template <typename T0, typename T1, typename T2>
inline void event<T0, T1, T2>::trigger(const T0 &v0, const T1 &v1, const T2 &v2) {
    if (se_ && *se_) {
	if (_s0) *_s0 = v0;
	if (_s1) *_s1 = v1;
	if (_s2) *_s2 = v2;
	se_->simple_trigger(true);
	se_ = 0;
    }
}

template <typename T0, typename T1, typename T2>
inline void event<T0, T1, T2>::trigger(const value_pack<T0, T1, T2>& v) {
    trigger(v.v0, v.v1, v.v2);
}

template <typename T0, typename T1, typename T2>
inline void event<T0, T1, T2>::operator()(const T0 &v0, const T1 &v1, const T2 &v2) {
    trigger(v0, v1, v2);
}

template <typename T0, typename T1, typename T2>
inline void event<T0, T1, T2>::operator()(const value_pack<T0, T1, T2>& v) {
    trigger(v);
}

template <typename T0, typename T1, typename T2>
inline void event<T0, T1, T2>::unblock() TAMER_NOEXCEPT {
    tamerpriv::simple_event::simple_trigger(se_, false);
    se_ = 0;
}


template <typename T0, typename T1>
inline event<T0, T1>::event() TAMER_NOEXCEPT
    : se_(0), _s0(0), _s1(0) {
}

template <typename T0, typename T1>
template <typename R, typename I0, typename I1>
inline event<T0, T1>::event(R& r, const I0& i0, const I1& i1, T0& x0, T1& x1)
    : se_(new tamerpriv::simple_event(r, i0, i1)), _s0(&x0), _s1(&x1) {
}

template <typename T0, typename T1>
template <typename R, typename I0, typename I1>
inline event<T0, T1>::event(R& r, const I0& i0, const I1& i1,
			    value_pack<T0, T1>& s)
    : se_(new tamerpriv::simple_event(r, i0, i1)), _s0(&s.v0), _s1(&s.v1) {
}

template <typename T0, typename T1>
template <typename R, typename I0>
inline event<T0, T1>::event(R& r, const I0& i0, T0& x0, T1& x1)
    : se_(new tamerpriv::simple_event(r, i0)), _s0(&x0), _s1(&x1) {
}

template <typename T0, typename T1>
template <typename R, typename I0>
inline event<T0, T1>::event(R& r, const I0& i0, value_pack<T0, T1>& s)
    : se_(new tamerpriv::simple_event(r, i0)), _s0(&s.v0), _s1(&s.v1) {
}

template <typename T0, typename T1>
template <typename R>
inline event<T0, T1>::event(R& r, T0& x0, T1& x1)
    : se_(new tamerpriv::simple_event(r)), _s0(&x0), _s1(&x1) {
}

template <typename T0, typename T1>
template <typename R>
inline event<T0, T1>::event(R& r, value_pack<T0, T1>& s)
    : se_(new tamerpriv::simple_event(r)), _s0(&s.v0), _s1(&s.v1) {
}

template <typename T0, typename T1>
inline void event<T0, T1>::trigger(const T0 &v0, const T1 &v1) {
    if (se_ && *se_) {
	if (_s0) *_s0 = v0;
	if (_s1) *_s1 = v1;
	se_->simple_trigger(true);
	se_ = 0;
    }
}

template <typename T0, typename T1>
inline void event<T0, T1>::trigger(const value_pack<T0, T1>& v) {
    trigger(v.v0, v.v1);
}

template <typename T0, typename T1>
inline void event<T0, T1>::operator()(const T0 &v0, const T1 &v1) {
    trigger(v0, v1);
}

template <typename T0, typename T1>
inline void event<T0, T1>::operator()(const value_pack<T0, T1>& v) {
    trigger(v);
}

template <typename T0, typename T1>
inline void event<T0, T1>::unblock() TAMER_NOEXCEPT {
    tamerpriv::simple_event::simple_trigger(se_, false);
    se_ = 0;
}


template <typename T0>
inline event<T0>::event() TAMER_NOEXCEPT
    : se_(0), _s0(0) {
}

template <typename T0> template <typename R, typename I0, typename I1>
inline event<T0>::event(R& r, const I0& i0, const I1& i1, T0& x0)
    : se_(new tamerpriv::simple_event(r, i0, i1)), _s0(&x0) {
}

template <typename T0> template <typename R, typename I0, typename I1>
inline event<T0>::event(R& r, const I0& i0, const I1& i1, value_pack<T0>& s)
    : se_(new tamerpriv::simple_event(r, i0, i1)), _s0(&s.v0) {
}

template <typename T0> template <typename R, typename I0>
inline event<T0>::event(R& r, const I0& i0, T0& x0)
    : se_(new tamerpriv::simple_event(r, i0)), _s0(&x0) {
}

template <typename T0> template <typename R, typename I0>
inline event<T0>::event(R& r, const I0& i0, value_pack<T0>& s)
    : se_(new tamerpriv::simple_event(r, i0)), _s0(&s.v0) {
}

template <typename T0> template <typename R>
inline event<T0>::event(R& r, T0& x0)
    : se_(new tamerpriv::simple_event(r)), _s0(&x0) {
}

template <typename T0> template <typename R>
inline event<T0>::event(R& r, value_pack<T0>& s)
    : se_(new tamerpriv::simple_event(r)), _s0(&s.v0) {
}

template <typename T0>
inline void event<T0>::trigger(const T0& v0) {
    if (se_ && *se_) {
	if (_s0) *_s0 = v0;
	se_->simple_trigger(true);
	se_ = 0;
    }
}

#if TAMER_HAVE_CXX_RVALUE_REFERENCES
template <typename T0>
inline void event<T0>::trigger(T0&& v0) {
    if (se_ && *se_) {
	if (_s0) *_s0 = std::move(v0);
	se_->simple_trigger(true);
	se_ = 0;
    }
}
#endif

template <typename T0>
inline void event<T0>::trigger(const value_pack<T0>& v) {
    trigger(v.v0);
}

template <typename T0> template <typename V0>
inline void event<T0>::trigger(V0 v0) {
    if (se_ && *se_) {
	if (_s0) *_s0 = v0;
	se_->simple_trigger(true);
	se_ = 0;
    }
}

template <typename T0>
inline void event<T0>::operator()(const T0& v0) {
    trigger(v0);
}

#if TAMER_HAVE_CXX_RVALUE_REFERENCES
template <typename T0>
inline void event<T0>::operator()(T0&& v0) {
    trigger(std::move(v0));
}
#endif

template <typename T0>
inline void event<T0>::operator()(const value_pack<T0>& v) {
    trigger(v);
}

template <typename T0> template <typename V0>
inline void event<T0>::operator()(V0 v0) {
    trigger(v0);
}

template <typename T0>
inline void event<T0>::unblock() TAMER_NOEXCEPT {
    tamerpriv::simple_event::simple_trigger(se_, false);
    se_ = 0;
}


inline event<>::event() TAMER_NOEXCEPT
    : se_(0) {
}

template <typename R, typename I0, typename I1>
inline event<>::event(R& r, const I0& i0, const I1& i1)
    : se_(new tamerpriv::simple_event(r, i0, i1)) {
}

template <typename R, typename I0, typename I1>
inline event<>::event(R& r, const I0& i0, const I1& i1, value_pack<>&)
    : se_(new tamerpriv::simple_event(r, i0, i1)) {
}

template <typename R, typename I0>
inline event<>::event(R& r, const I0& i0)
    : se_(new tamerpriv::simple_event(r, i0)) {
}

template <typename R, typename I0>
inline event<>::event(R& r, const I0& i0, value_pack<>&)
    : se_(new tamerpriv::simple_event(r, i0)) {
}

template <typename R>
inline event<>::event(R& r)
    : se_(new tamerpriv::simple_event(r)) {
}

template <typename R>
inline event<>::event(R& r, value_pack<>&)
    : se_(new tamerpriv::simple_event(r)) {
}

inline void event<>::trigger() TAMER_NOEXCEPT {
    tamerpriv::simple_event::simple_trigger(se_, false);
    se_ = 0;
}

inline void event<>::trigger(const value_pack<>&) TAMER_NOEXCEPT {
    trigger();
}

inline void event<>::operator()() TAMER_NOEXCEPT {
    trigger();
}

inline void event<>::operator()(const value_pack<>& v) TAMER_NOEXCEPT {
    trigger(v);
}

inline void event<>::unblock() TAMER_NOEXCEPT {
    trigger();
}

/** @endcond */

/** @brief  Test if this event has a valid result.
 *
 *  Returns true iff this event is nonempty and has a result.
 */
template <typename T0>
inline bool event<T0>::has_result() const TAMER_NOEXCEPT {
    return se_ && *se_ && _s0;
}

/** @brief  Return event's result.
 *  @pre    has_result()
 */
template <typename T0>
inline T0& event<T0>::result() const TAMER_NOEXCEPT {
    assert(has_result());
    return *_s0;
}

/** @brief  Return a pointer to event's result, if any. */
template <typename T0>
inline T0* event<T0>::result_pointer() const TAMER_NOEXCEPT {
    return se_ && *se_ ? _s0 : 0;
}


/** @defgroup make_event Helper functions for making events
 *
 *  The @c make_event() helper function simplifies event creation.  @c
 *  make_event() automatically selects the right type of event for its
 *  arguments.  There are 3*5 = 15 versions, one for each combination of event
 *  IDs and results.
 *
 *  @{ */

/** @brief  Construct a two-ID, four-result event on rendezvous @a r.
 *  @param  r   Rendezvous.
 *  @param  i0  First event ID.
 *  @param  i1  Second event ID.
 *  @param  x0  First result.
 *  @param  x1  Second result.
 *  @param  x2  Third result.
 *  @param  x3  Fourth result.
 *
 *  @note Versions of this function exist for any combination of two, one,
 *  or zero event IDs and four, three, two, one, or zero results. For
 *  example, <code>make_event(r)</code> creates a zero-ID, zero-result event
 *  on <code>rendezvous<> r</code>, while <code>make_event(r, 1, i,
 *  j)</code> might create a one-ID, two-result event on
 *  <code>rendezvous<int> r</code>.
 */
template <typename R, typename J0, typename J1, typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> make_event(two_argument_rendezvous_tag<R>& r, const J0& i0, const J1& i1, T0& x0, T1& x1, T2& x2, T3& x3)
{
    return event<T0, T1, T2, T3>(static_cast<R&>(r), i0, i1, x0, x1, x2, x3);
}

template <typename R, typename J0, typename J1, typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> make_event(two_argument_rendezvous_tag<R>& r, const J0& i0, const J1& i1, value_pack<T0, T1, T2, T3>& sp)
{
    return event<T0, T1, T2, T3>(static_cast<R&>(r), i0, i1, sp);
}

template <typename R, typename J0, typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> make_event(one_argument_rendezvous_tag<R>& r, const J0& i0, T0& x0, T1& x1, T2& x2, T3& x3)
{
    return event<T0, T1, T2, T3>(static_cast<R&>(r), i0, x0, x1, x2, x3);
}

template <typename R, typename J0, typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> make_event(one_argument_rendezvous_tag<R>& r, const J0& i0, value_pack<T0, T1, T2, T3>& sp)
{
    return event<T0, T1, T2, T3>(static_cast<R&>(r), i0, sp);
}

template <typename R, typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> make_event(zero_argument_rendezvous_tag<R>& r, T0& x0, T1& x1, T2& x2, T3& x3)
{
    return event<T0, T1, T2, T3>(static_cast<R&>(r), x0, x1, x2, x3);
}

template <typename R, typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> make_event(zero_argument_rendezvous_tag<R>& r, value_pack<T0, T1, T2, T3>& sp)
{
    return event<T0, T1, T2, T3>(static_cast<R&>(r), sp);
}

template <typename R, typename J0, typename J1, typename T0, typename T1, typename T2>
inline event<T0, T1, T2> make_event(two_argument_rendezvous_tag<R>& r, const J0& i0, const J1& i1, T0& x0, T1& x1, T2& x2)
{
    return event<T0, T1, T2>(static_cast<R&>(r), i0, i1, x0, x1, x2);
}

template <typename R, typename J0, typename T0, typename T1, typename T2>
inline event<T0, T1, T2> make_event(one_argument_rendezvous_tag<R>& r, const J0& i0, T0& x0, T1& x1, T2& x2)
{
    return event<T0, T1, T2>(static_cast<R&>(r), i0, x0, x1, x2);
}

template <typename R, typename T0, typename T1, typename T2>
inline event<T0, T1, T2> make_event(zero_argument_rendezvous_tag<R>& r, T0& x0, T1& x1, T2& x2)
{
    return event<T0, T1, T2>(static_cast<R&>(r), x0, x1, x2);
}

template <typename R, typename J0, typename J1, typename T0, typename T1>
inline event<T0, T1> make_event(two_argument_rendezvous_tag<R>& r, const J0& i0, const J1& i1, T0& x0, T1& x1)
{
    return event<T0, T1>(static_cast<R&>(r), i0, i1, x0, x1);
}

template <typename R, typename J0, typename T0, typename T1>
inline event<T0, T1> make_event(one_argument_rendezvous_tag<R>& r, const J0& i0, T0& x0, T1& x1)
{
    return event<T0, T1>(static_cast<R&>(r), i0, x0, x1);
}

template <typename R, typename T0, typename T1>
inline event<T0, T1> make_event(zero_argument_rendezvous_tag<R>& r, T0& x0, T1& x1)
{
    return event<T0, T1>(static_cast<R&>(r), x0, x1);
}

template <typename R, typename J0, typename J1, typename T0>
inline event<T0> make_event(two_argument_rendezvous_tag<R>& r, const J0& i0, const J1& i1, T0& x0)
{
    return event<T0>(static_cast<R&>(r), i0, i1, x0);
}

template <typename R, typename J0, typename T0>
inline event<T0> make_event(one_argument_rendezvous_tag<R>& r, const J0& i0, T0& x0)
{
    return event<T0>(static_cast<R&>(r), i0, x0);
}

#if TAMER_HAVE_PREEVENT
template <typename R, typename T0>
inline preevent<R, T0> make_event(zero_argument_rendezvous_tag<R>& r, T0& x0)
{
    return preevent<R, T0>(static_cast<R&>(r), x0);
}
#else
template <typename R, typename T0>
inline event<T0> make_event(zero_argument_rendezvous_tag<R>& r, T0& x0)
{
    return event<T0>(static_cast<R&>(r), x0);
}
#endif

template <typename R, typename J0, typename J1>
inline event<> make_event(two_argument_rendezvous_tag<R>& r, const J0& i0, const J1& i1)
{
    return event<>(static_cast<R&>(r), i0, i1);
}

template <typename R, typename J0>
inline event<> make_event(one_argument_rendezvous_tag<R>& r, const J0& i0)
{
    return event<>(static_cast<R&>(r), i0);
}

#if TAMER_HAVE_PREEVENT
template <typename R>
inline preevent<R> make_event(zero_argument_rendezvous_tag<R>& r)
{
    return preevent<R>(static_cast<R&>(r));
}
#else
template <typename R>
inline event<> make_event(zero_argument_rendezvous_tag<R>& r)
{
    return event<>(static_cast<R&>(r));
}
#endif

/** @} */


/** @cond never */
template <typename I0, typename I1> template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> rendezvous<I0, I1>::make_event(const I0& i0, const I1& i1, T0& x0, T1& x1, T2& x2, T3& x3)
{
    return tamer::make_event(*this, i0, i1, x0, x1, x2, x3);
}

template <typename I0> template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> rendezvous<I0>::make_event(const I0& i0, T0& x0, T1& x1, T2& x2, T3& x3)
{
    return tamer::make_event(*this, i0, x0, x1, x2, x3);
}

template <typename I0> template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> simple_rendezvous<I0>::make_event(I0 i0, T0& x0, T1& x1, T2& x2, T3& x3)
{
    return tamer::make_event(*this, i0, x0, x1, x2, x3);
}

template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> rendezvous<>::make_event(T0& x0, T1& x1, T2& x2, T3& x3)
{
    return tamer::make_event(*this, x0, x1, x2, x3);
}

template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> gather_rendezvous::make_event(T0& x0, T1& x1, T2& x2, T3& x3)
{
    return tamer::make_event(*this, x0, x1, x2, x3);
}

template <typename I0, typename I1> template <typename T0, typename T1, typename T2>
inline event<T0, T1, T2> rendezvous<I0, I1>::make_event(const I0& i0, const I1& i1, T0& x0, T1& x1, T2& x2)
{
    return tamer::make_event(*this, i0, i1, x0, x1, x2);
}

template <typename I0> template <typename T0, typename T1, typename T2>
inline event<T0, T1, T2> rendezvous<I0>::make_event(const I0& i0, T0& x0, T1& x1, T2& x2)
{
    return tamer::make_event(*this, i0, x0, x1, x2);
}

template <typename I0> template <typename T0, typename T1, typename T2>
inline event<T0, T1, T2> simple_rendezvous<I0>::make_event(I0 i0, T0& x0, T1& x1, T2& x2)
{
    return tamer::make_event(*this, i0, x0, x1, x2);
}

template <typename T0, typename T1, typename T2>
inline event<T0, T1, T2> rendezvous<>::make_event(T0& x0, T1& x1, T2& x2)
{
    return tamer::make_event(*this, x0, x1, x2);
}

template <typename T0, typename T1, typename T2>
inline event<T0, T1, T2> gather_rendezvous::make_event(T0& x0, T1& x1, T2& x2)
{
    return tamer::make_event(*this, x0, x1, x2);
}

template <typename I0, typename I1> template <typename T0, typename T1>
inline event<T0, T1> rendezvous<I0, I1>::make_event(const I0& i0, const I1& i1, T0& x0, T1& x1)
{
    return tamer::make_event(*this, i0, i1, x0, x1);
}

template <typename I0> template <typename T0, typename T1>
inline event<T0, T1> rendezvous<I0>::make_event(const I0& i0, T0& x0, T1& x1)
{
    return tamer::make_event(*this, i0, x0, x1);
}

template <typename I0> template <typename T0, typename T1>
inline event<T0, T1> simple_rendezvous<I0>::make_event(I0 i0, T0& x0, T1& x1)
{
    return tamer::make_event(*this, i0, x0, x1);
}

template <typename T0, typename T1>
inline event<T0, T1> rendezvous<>::make_event(T0& x0, T1& x1)
{
    return tamer::make_event(*this, x0, x1);
}

template <typename T0, typename T1>
inline event<T0, T1> gather_rendezvous::make_event(T0& x0, T1& x1)
{
    return tamer::make_event(*this, x0, x1);
}

template <typename I0, typename I1> template <typename T0>
inline event<T0> rendezvous<I0, I1>::make_event(const I0& i0, const I1& i1, T0& x0)
{
    return tamer::make_event(*this, i0, i1, x0);
}

template <typename I0> template <typename T0>
inline event<T0> rendezvous<I0>::make_event(const I0& i0, T0& x0)
{
    return tamer::make_event(*this, i0, x0);
}

template <typename I0> template <typename T0>
inline event<T0> simple_rendezvous<I0>::make_event(I0 i0, T0& x0)
{
    return tamer::make_event(*this, i0, x0);
}

#if TAMER_HAVE_PREEVENT
template <typename T0>
inline preevent<rendezvous<>, T0> rendezvous<>::make_event(T0& x0)
{
    return preevent<rendezvous<>, T0>(*this, x0);
}

template <typename T0>
inline preevent<gather_rendezvous, T0> gather_rendezvous::make_event(T0& x0)
{
    return preevent<gather_rendezvous, T0>(*this, x0);
}
#else
template <typename T0>
inline event<T0> rendezvous<>::make_event(T0& x0)
{
    return tamer::make_event(*this, x0);
}

template <typename T0>
inline event<T0> gather_rendezvous::make_event(T0& x0)
{
    return tamer::make_event(*this, x0);
}
#endif

template <typename I0, typename I1>
inline event<> rendezvous<I0, I1>::make_event(const I0& i0, const I1& i1)
{
    return tamer::make_event(*this, i0, i1);
}

template <typename I0>
inline event<> rendezvous<I0>::make_event(const I0& i0)
{
    return tamer::make_event(*this, i0);
}

template <typename I0>
inline event<> simple_rendezvous<I0>::make_event(I0 i0)
{
    return tamer::make_event(*this, i0);
}

#if TAMER_HAVE_PREEVENT
inline preevent<rendezvous<> > rendezvous<>::make_event()
{
    return preevent<rendezvous<> >(*this);
}

inline preevent<gather_rendezvous> gather_rendezvous::make_event()
{
    return preevent<gather_rendezvous>(*this);
}
#else
inline event<> rendezvous<>::make_event()
{
    return tamer::make_event(*this);
}

inline event<> gather_rendezvous::make_event()
{
    return tamer::make_event(*this);
}
#endif
/** @endcond never */


template <typename R, typename J0, typename J1, typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> make_annotated_event(const char *file, int line, two_argument_rendezvous_tag<R>& r, const J0& i0, const J1& i1, T0& x0, T1& x1, T2& x2, T3& x3)
{
    event<T0, T1, T2, T3> e = tamer::make_event(static_cast<R&>(r), i0, i1, x0, x1, x2, x3);
    e.__get_simple()->annotate(file, line);
    return e;
}

template <typename R, typename J0, typename J1, typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> make_annotated_event(const char *file, int line, two_argument_rendezvous_tag<R>& r, const J0& i0, const J1& i1, value_pack<T0, T1, T2, T3>& sp)
{
    event<T0, T1, T2, T3> e = tamer::make_event(static_cast<R&>(r), i0, i1, sp);
    e.__get_simple()->annotate(file, line);
    return e;
}

template <typename R, typename J0, typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> make_annotated_event(const char *file, int line, one_argument_rendezvous_tag<R>& r, const J0& i0, T0& x0, T1& x1, T2& x2, T3& x3)
{
    event<T0, T1, T2, T3> e = tamer::make_event(static_cast<R&>(r), i0, x0, x1, x2, x3);
    e.__get_simple()->annotate(file, line);
    return e;
}

template <typename R, typename J0, typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> make_annotated_event(const char *file, int line, one_argument_rendezvous_tag<R>& r, const J0& i0, value_pack<T0, T1, T2, T3>& sp)
{
    event<T0, T1, T2, T3> e = tamer::make_event(static_cast<R&>(r), i0, sp);
    e.__get_simple()->annotate(file, line);
    return e;
}

template <typename R, typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> make_annotated_event(const char *file, int line, zero_argument_rendezvous_tag<R>& r, T0& x0, T1& x1, T2& x2, T3& x3)
{
    event<T0, T1, T2, T3> e = tamer::make_event(static_cast<R&>(r), x0, x1, x2, x3);
    e.__get_simple()->annotate(file, line);
    return e;
}

template <typename R, typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> make_annotated_event(const char *file, int line, zero_argument_rendezvous_tag<R>& r, value_pack<T0, T1, T2, T3>& sp)
{
    event<T0, T1, T2, T3> e = tamer::make_event(static_cast<R&>(r), sp);
    e.__get_simple()->annotate(file, line);
    return e;
}

template <typename R, typename J0, typename J1, typename T0, typename T1, typename T2>
inline event<T0, T1, T2> make_annotated_event(const char *file, int line, two_argument_rendezvous_tag<R>& r, const J0& i0, const J1& i1, T0& x0, T1& x1, T2& x2)
{
    event<T0, T1, T2> e = tamer::make_event(static_cast<R&>(r), i0, i1, x0, x1, x2);
    e.__get_simple()->annotate(file, line);
    return e;
}

template <typename R, typename J0, typename T0, typename T1, typename T2>
inline event<T0, T1, T2> make_annotated_event(const char *file, int line, one_argument_rendezvous_tag<R>& r, const J0& i0, T0& x0, T1& x1, T2& x2)
{
    event<T0, T1, T2> e = tamer::make_event(static_cast<R&>(r), i0, x0, x1, x2);
    e.__get_simple()->annotate(file, line);
    return e;
}

template <typename R, typename T0, typename T1, typename T2>
inline event<T0, T1, T2> make_annotated_event(const char *file, int line, zero_argument_rendezvous_tag<R>& r, T0& x0, T1& x1, T2& x2)
{
    event<T0, T1, T2> e = tamer::make_event(static_cast<R&>(r), x0, x1, x2);
    e.__get_simple()->annotate(file, line);
    return e;
}

template <typename R, typename J0, typename J1, typename T0, typename T1>
inline event<T0, T1> make_annotated_event(const char *file, int line, two_argument_rendezvous_tag<R>& r, const J0& i0, const J1& i1, T0& x0, T1& x1)
{
    event<T0, T1> e = tamer::make_event(static_cast<R&>(r), i0, i1, x0, x1);
    e.__get_simple()->annotate(file, line);
    return e;
}

template <typename R, typename J0, typename T0, typename T1>
inline event<T0, T1> make_annotated_event(const char *file, int line, one_argument_rendezvous_tag<R>& r, const J0& i0, T0& x0, T1& x1)
{
    event<T0, T1> e = tamer::make_event(static_cast<R&>(r), i0, x0, x1);
    e.__get_simple()->annotate(file, line);
    return e;
}

template <typename R, typename T0, typename T1>
inline event<T0, T1> make_annotated_event(const char *file, int line, zero_argument_rendezvous_tag<R>& r, T0& x0, T1& x1)
{
    event<T0, T1> e = tamer::make_event(static_cast<R&>(r), x0, x1);
    e.__get_simple()->annotate(file, line);
    return e;
}

template <typename R, typename J0, typename J1, typename T0>
inline event<T0> make_annotated_event(const char *file, int line, two_argument_rendezvous_tag<R>& r, const J0& i0, const J1& i1, T0& x0)
{
    event<T0> e = tamer::make_event(static_cast<R&>(r), i0, i1, x0);
    e.__get_simple()->annotate(file, line);
    return e;
}

template <typename R, typename J0, typename T0>
inline event<T0> make_annotated_event(const char *file, int line, one_argument_rendezvous_tag<R>& r, const J0& i0, T0& x0)
{
    event<T0> e = tamer::make_event(static_cast<R&>(r), i0, x0);
    e.__get_simple()->annotate(file, line);
    return e;
}

template <typename R, typename T0>
inline event<T0> make_annotated_event(const char *file, int line, zero_argument_rendezvous_tag<R>& r, T0& x0)
{
    event<T0> e = tamer::make_event(static_cast<R&>(r), x0);
    e.__get_simple()->annotate(file, line);
    return e;
}

template <typename R, typename J0, typename J1>
inline event<> make_annotated_event(const char *file, int line, two_argument_rendezvous_tag<R>& r, const J0& i0, const J1& i1)
{
    event<> e = tamer::make_event(static_cast<R&>(r), i0, i1);
    e.__get_simple()->annotate(file, line);
    return e;
}

template <typename R, typename J0>
inline event<> make_annotated_event(const char *file, int line, one_argument_rendezvous_tag<R>& r, const J0& i0)
{
    event<> e = tamer::make_event(static_cast<R&>(r), i0);
    e.__get_simple()->annotate(file, line);
    return e;
}

template <typename R>
inline event<> make_annotated_event(const char *file, int line, zero_argument_rendezvous_tag<R>& r)
{
    event<> e = tamer::make_event(static_cast<R&>(r));
    e.__get_simple()->annotate(file, line);
    return e;
}


template <typename T0, typename T1, typename T2>
inline void event<T0, T1, T2>::at_trigger(const event<>& e) {
    tamerpriv::simple_event::use(e.__get_simple());
    tamerpriv::simple_event::at_trigger(se_, e.__get_simple());
}

template <typename T0, typename T1>
inline void event<T0, T1>::at_trigger(const event<>& e) {
    tamerpriv::simple_event::use(e.__get_simple());
    tamerpriv::simple_event::at_trigger(se_, e.__get_simple());
}

template <typename T0>
inline void event<T0>::at_trigger(const event<>& e) {
    tamerpriv::simple_event::use(e.__get_simple());
    tamerpriv::simple_event::at_trigger(se_, e.__get_simple());
}

#if TAMER_HAVE_CXX_RVALUE_REFERENCES
template <typename T0, typename T1, typename T2>
inline void event<T0, T1, T2>::at_trigger(event<>&& e) {
    tamerpriv::simple_event::at_trigger(se_, e.__take_simple());
}

template <typename T0, typename T1>
inline void event<T0, T1>::at_trigger(event<>&& e) {
    tamerpriv::simple_event::at_trigger(se_, e.__take_simple());
}

template <typename T0>
inline void event<T0>::at_trigger(event<>&& e) {
    tamerpriv::simple_event::at_trigger(se_, e.__take_simple());
}
#endif

template <typename T0, typename T1, typename T2>
inline event<> event<T0, T1, T2>::unblocker() const TAMER_NOEXCEPT {
    tamerpriv::simple_event::use(se_);
    return event<>::__make(se_);
}

template <typename T0, typename T1>
inline event<> event<T0, T1>::unblocker() const TAMER_NOEXCEPT {
    tamerpriv::simple_event::use(se_);
    return event<>::__make(se_);
}

template <typename T0>
inline event<> event<T0>::unblocker() const TAMER_NOEXCEPT {
    tamerpriv::simple_event::use(se_);
    return event<>::__make(se_);
}

template <typename T0, typename T1, typename T2>
inline event<> event<T0, T1, T2>::bind_all() const {
    return unblocker();
}

template <typename T0, typename T1>
inline event<> event<T0, T1>::bind_all() const {
    return unblocker();
}

template <typename T0>
inline event<> event<T0>::bind_all() const {
    return unblocker();
}

inline event<>& event<>::bind_all() {
    return unblocker();
}

inline event<> event<>::bind_all() const {
    return unblocker();
}

template <typename T0>
inline event<T0>::event(event<> e, const no_result&) TAMER_NOEXCEPT
    : se_(e.__take_simple()), _s0(0) {
}

#if TAMER_HAVE_PREEVENT
template <typename T0> template <typename R>
inline event<T0>::event(preevent<R, T0>&& x)
    : se_(x.r_ ? new tamerpriv::simple_event(*x.r_) : 0), _s0(x.s0_) {
    x.r_ = 0;
}

template <typename R>
inline event<>::event(preevent<R>&& x)
    : se_(x.r_ ? new tamerpriv::simple_event(*x.r_) : 0) {
    x.r_ = 0;
}
#endif

} // namespace tamer
#endif /* TAMER_EVENT_HH */
