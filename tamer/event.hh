#ifndef TAMER_EVENT_HH
#define TAMER_EVENT_HH 1
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
#include <tamer/xbase.hh>
#include <tamer/rendezvous.hh>
#include <functional>
namespace tamer {

/** @file <tamer/event.hh>
 *  @brief  The event template classes and helper functions.
 */

/** @class event tamer/event.hh <tamer/tamer.hh>
 *  @brief  A future occurrence.
 *
 *  An event object represents a future occurrence, such as the completion
 *  of a network read. When the occurrence becomes ready---for instance, a
 *  packet arrives---the event object is triggered via its trigger() method
 *  (or, equivalently, by operator()()). Code can block until an event
 *  triggers using @c twait special forms.
 *
 *  Events have from zero to four <em>results</em> of arbitrary type. Result
 *  values are set when the event is triggered and passed to the function
 *  waiting for the event. For example, an event of type <code>event<int,
 *  char*, bool></code> has three results. Most constructors for
 *  <code>event<int, char*, bool></code> take three reference arguments of
 *  types <code>int&</code>, <code>char*&</code>, and <code>bool&</code>. To
 *  trigger the event, the caller supplies actual values for these results.
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
 *  does not change the caller’s result values. For instance:
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
 *  last reference to its underlying occurrence is deleted. Leaking an active
 *  event is usually considered a programming error, and a message is printed
 *  at run time to indicate that an event triggered abnormally. For example,
 *  the following code:
 *
 *  @code
 *     twait { (void) make_event(); }
 *  @endcode
 *
 *  will print a message like "<tt>ex4.tt:11: dropping last reference to
 *  active event</tt>". However, it is sometimes convenient to rely on this
 *  triggering behavior, so the error message is turned off for rendezvous
 *  declared as volatile:
 *
 *  @code
 *     twait volatile { (void) make_event(); }
 *     tamer::rendezvous<> r(tamer::rvolatile);
 *  @endcode
 *
 *  Deleting the last reference to an empty event is not an error and does not
 *  produce an error message.
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
    typedef T0 first_argument_type;
    typedef T1 second_argument_type;
    typedef T2 third_argument_type;
    typedef T3 fourth_argument_type;
    typedef std::tuple<T0, T1, T2, T3> results_tuple_type;

    typedef tamerpriv::simple_event::unspecified_bool_type unspecified_bool_type;

    inline event() noexcept;
    inline event(T0& x0, T1& x1, T2& x2, T3& x3) noexcept;
    inline event(results_tuple_type& xs) noexcept;
    inline event(const event<T0, T1, T2, T3>& x) noexcept;
    inline event(event<T0, T1, T2, T3>&& x) noexcept;
    inline ~event() noexcept;

    inline operator unspecified_bool_type() const;
    inline bool operator!() const;
    inline bool empty() const;

    inline void operator()(T0 v0, T1 v1, T2 v2, T3 v3);
    inline void trigger(T0 v0, T1 v1, T2 v2, T3 v3);
    inline void tuple_trigger(const results_tuple_type& vs);
    inline void unblock() noexcept;
    template <size_t I> inline void set_result(typename std::tuple_element<I, results_tuple_type>::type vi);

    inline void at_trigger(const event<>& e);
    inline void at_trigger(event<>&& e);

    inline event<> unblocker() const noexcept;
    inline event<> bind_all() const TAMER_DEPRECATEDATTR;

    inline event<T0, T1, T2, T3>& operator=(const event<T0, T1, T2, T3>& x) noexcept;
    inline event<T0, T1, T2, T3>& operator=(event<T0, T1, T2, T3>&& x) noexcept;

    inline event<T0, T1, T2, T3>& operator+=(event<T0, T1, T2, T3> x);

    inline const char* file_annotation() const;
    inline int line_annotation() const;

    inline event<T0, T1, T2, T3>& __instantiate(tamerpriv::abstract_rendezvous& r, uintptr_t rid, const char* file = 0, int line = 0);
    inline tamerpriv::simple_event* __get_simple() const;
    inline tamerpriv::simple_event* __release_simple();

  private:
    tamerpriv::simple_event* se_;
    std::tuple<T0*, T1*, T2*, T3*> sv_;
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
    typedef T0 first_argument_type;
    typedef T1 second_argument_type;
    typedef T2 third_argument_type;
    typedef std::tuple<T0, T1, T2> results_tuple_type;

    typedef tamerpriv::simple_event::unspecified_bool_type unspecified_bool_type;

    inline event() noexcept;
    inline event(T0& x0, T1& x1, T2& x2) noexcept;
    inline event(results_tuple_type& xs) noexcept;

    event(const event<T0, T1, T2>& x) noexcept
        : se_(x.se_), sv_(x.sv_) {
        tamerpriv::simple_event::use(se_);
    }

    event(event<T0, T1, T2>&& x) noexcept
        : se_(x.se_), sv_(x.sv_) {
        x.se_ = 0;
    }

    ~event() noexcept {
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

    inline void operator()(T0 v0, T1 v1, T2 v2);
    inline void trigger(T0 v0, T1 v1, T2 v2);
    inline void tuple_trigger(const results_tuple_type& v);
    inline void unblock() noexcept;
    template <size_t I> inline void set_result(typename std::tuple_element<I, results_tuple_type>::type vi);

    inline void at_trigger(const event<>& e);
    inline void at_trigger(event<>&& e);

    inline event<> unblocker() const noexcept;
    inline event<> bind_all() const TAMER_DEPRECATEDATTR;

    event<T0, T1, T2>& operator=(const event<T0, T1, T2>& x) noexcept {
        tamerpriv::simple_event::use(x.se_);
        tamerpriv::simple_event::unuse(se_);
        se_ = x.se_;
        sv_ = x.sv_;
        return *this;
    }

    event<T0, T1, T2>& operator=(event<T0, T1, T2>&& x) noexcept {
        tamerpriv::simple_event *se = se_;
        se_ = x.se_;
        sv_ = x.sv_;
        x.se_ = se;
        return *this;
    }

    inline event<T0, T1, T2>& operator+=(event<T0, T1, T2> x);

    inline const char* file_annotation() const;
    inline int line_annotation() const;

    event<T0, T1, T2>& __instantiate(tamerpriv::abstract_rendezvous& r, uintptr_t rid, const char* file = 0, int line = 0) {
        TAMER_DEBUG_ASSERT(!se_);
        se_ = new tamerpriv::simple_event(r, rid, file, line);
        return *this;
    }
    tamerpriv::simple_event* __get_simple() const {
        return se_;
    }
    tamerpriv::simple_event* __release_simple() {
        tamerpriv::simple_event *se = se_;
        se_ = 0;
        return se;
    }

  private:
    tamerpriv::simple_event* se_;
    std::tuple<T0*, T1*, T2*> sv_;
};


template <typename T0, typename T1>
class event<T0, T1, void, void>
    : public std::binary_function<T0, T1, void> { public:
    typedef tamerpriv::simple_event::unspecified_bool_type unspecified_bool_type;
    typedef std::tuple<T0, T1> results_tuple_type;

    inline event() noexcept;
    inline event(T0& x0, T1& x1) noexcept;
    inline event(results_tuple_type& xs) noexcept;

    event(const event<T0, T1>& x) noexcept
        : se_(x.se_), sv_(x.sv_) {
        tamerpriv::simple_event::use(se_);
    }

    event(event<T0, T1>&& x) noexcept
        : se_(x.se_), sv_(x.sv_) {
        x.se_ = 0;
    }

    ~event() noexcept {
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

    inline void operator()(T0 v0, T1 v1);
    inline void trigger(T0 v0, T1 v1);
    inline void tuple_trigger(const results_tuple_type& vs);
    inline void unblock() noexcept;
    template <size_t I> inline void set_result(typename std::tuple_element<I, results_tuple_type>::type vi);

    inline void at_trigger(const event<>& e);
    inline void at_trigger(event<>&& e);

    inline event<> unblocker() const noexcept;
    inline event<> bind_all() const TAMER_DEPRECATEDATTR;

    event<T0, T1>& operator=(const event<T0, T1>& x) noexcept {
        tamerpriv::simple_event::use(x.se_);
        tamerpriv::simple_event::unuse(se_);
        se_ = x.se_;
        sv_ = x.sv_;
        return *this;
    }

    event<T0, T1>& operator=(event<T0, T1>&& x) noexcept {
        tamerpriv::simple_event *se = se_;
        se_ = x.se_;
        sv_ = x.sv_;
        x.se_ = se;
        return *this;
    }

    inline event<T0, T1>& operator+=(event<T0, T1> x);

    inline const char* file_annotation() const;
    inline int line_annotation() const;

    event<T0, T1>& __instantiate(tamerpriv::abstract_rendezvous& r, uintptr_t rid, const char* file = 0, int line = 0) {
        TAMER_DEBUG_ASSERT(!se_);
        se_ = new tamerpriv::simple_event(r, rid, file, line);
        return *this;
    }
    tamerpriv::simple_event* __get_simple() const {
        return se_;
    }
    tamerpriv::simple_event* __release_simple() {
        tamerpriv::simple_event *se = se_;
        se_ = 0;
        return se;
    }

  private:
    tamerpriv::simple_event* se_;
    std::tuple<T0*, T1*> sv_;
};


template <typename T0>
class event<T0, void, void, void>
    : public std::unary_function<T0, void> { public:
    typedef tamerpriv::simple_event::unspecified_bool_type unspecified_bool_type;
    typedef std::tuple<T0> results_tuple_type;

    inline event() noexcept;
    inline event(T0& x0) noexcept;
    inline event(results_tuple_type& xs) noexcept;

    event(const event<T0>& x) noexcept
        : se_(x.se_), s0_(x.s0_) {
        tamerpriv::simple_event::use(se_);
    }

    event(event<T0>&& x) noexcept
        : se_(x.se_), s0_(x.s0_) {
        x.se_ = 0;
    }

#if TAMER_HAVE_PREEVENT
    template <typename R> inline event(preevent<R, T0>&& x);
#endif

    inline event(const event<>& x, results_tuple_type& xs);
    inline event(event<>&& x, results_tuple_type& xs) noexcept;

    ~event() noexcept {
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

    inline void operator()(T0 v0);
    inline void trigger(T0 v0);
    inline void tuple_trigger(const results_tuple_type& vs);
    inline void unblock() noexcept;
    template <size_t I = 0> inline void set_result(typename std::tuple_element<I, results_tuple_type>::type vi);

    inline T0& result() const noexcept;
    inline T0* result_pointer() const noexcept;

    inline void at_trigger(const event<>& e);
    inline void at_trigger(event<>&& e);

    inline event<> unblocker() const noexcept;
    inline event<> bind_all() const TAMER_DEPRECATEDATTR;

    event<T0>& operator=(const event<T0>& x) noexcept {
        tamerpriv::simple_event::use(x.se_);
        tamerpriv::simple_event::unuse(se_);
        se_ = x.se_;
        s0_ = x.s0_;
        return *this;
    }

    event<T0>& operator=(event<T0>&& x) noexcept {
        tamerpriv::simple_event *se = se_;
        se_ = x.se_;
        s0_ = x.s0_;
        x.se_ = se;
        return *this;
    }

#if TAMER_HAVE_PREEVENT
    template <typename R> event<T0>& operator=(preevent<R, T0>&& x) {
        return *this = event<T0>(std::move(x));
    }
#endif

    inline event<T0>& operator+=(event<T0> x);

    inline const char* file_annotation() const;
    inline int line_annotation() const;

    event<T0>& __instantiate(tamerpriv::abstract_rendezvous& r, uintptr_t rid, const char* file = 0, int line = 0) {
        TAMER_DEBUG_ASSERT(!se_);
        se_ = new tamerpriv::simple_event(r, rid, file, line);
        return *this;
    }
    tamerpriv::simple_event* __get_simple() const {
        return se_;
    }
    tamerpriv::simple_event* __release_simple() {
        tamerpriv::simple_event *se = se_;
        se_ = 0;
        return se;
    }

    static inline event<T0> __make(tamerpriv::simple_event* se, T0* s0) {
        return event<T0>(take_marker(), se, s0);
    }

  private:
    tamerpriv::simple_event* se_;
    T0* s0_;

    struct take_marker { };
    inline event(const take_marker &, tamerpriv::simple_event* se, T0* s0)
        : se_(se), s0_(s0) {
    }
};


template <>
class event<void, void, void, void> { public:
    typedef void result_type;
    typedef std::tuple<> results_tuple_type;

    typedef tamerpriv::simple_event::unspecified_bool_type unspecified_bool_type;

    inline event() noexcept;
    inline event(results_tuple_type& xs) noexcept;

    event(const event<>& x) noexcept
        : se_(x.se_) {
        tamerpriv::simple_event::use(se_);
    }

    event(event<>& x) noexcept
        : se_(x.se_) {
        tamerpriv::simple_event::use(se_);
    }

    event(event<>&& x) noexcept
        : se_(x.se_) {
        x.se_ = 0;
    }

#if TAMER_HAVE_PREEVENT
    template <typename R> inline event(preevent<R>&& x);
#endif

    ~event() noexcept {
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

    inline void operator()() noexcept;
    inline void trigger() noexcept;
    inline void tuple_trigger(const results_tuple_type& vs) noexcept;
    inline void unblock() noexcept;

    inline void at_trigger(const event<>& e) {
        tamerpriv::simple_event::use(e.__get_simple());
        tamerpriv::simple_event::at_trigger(se_, e.__get_simple());
    }
    inline void at_trigger(event<>&& e) {
        tamerpriv::simple_event::at_trigger(se_, e.__release_simple());
    }

    event<>& unblocker() noexcept {
        return *this;
    }
    event<> unblocker() const noexcept {
        return *this;
    }

    event<>& bind_all() TAMER_DEPRECATEDATTR;
    event<> bind_all() const TAMER_DEPRECATEDATTR;

    event<>& operator=(const event<>& x) noexcept {
        tamerpriv::simple_event::use(x.se_);
        tamerpriv::simple_event::unuse(se_);
        se_ = x.se_;
        return *this;
    }

    event<>& operator=(event<>&& x) noexcept {
        tamerpriv::simple_event *se = se_;
        se_ = x.se_;
        x.se_ = se;
        return *this;
    }

#if TAMER_HAVE_PREEVENT
    template <typename R> event<>& operator=(preevent<R>&& x) {
        return *this = event<>(std::move(x));
    }
#endif

    inline event<>& operator+=(event<> x);

    inline const char* file_annotation() const;
    inline int line_annotation() const;

    event<>& __instantiate(tamerpriv::abstract_rendezvous& r, uintptr_t rid, const char* file = 0, int line = 0) {
        TAMER_DEBUG_ASSERT(!se_);
        se_ = new tamerpriv::simple_event(r, rid, file, line);
        return *this;
    }
    tamerpriv::simple_event* __get_simple() const {
        return se_;
    }
    tamerpriv::simple_event* __release_simple() {
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
class preevent : public std::unary_function<const T0&, void> {
  public:
    typedef tamerpriv::simple_event::unspecified_bool_type unspecified_bool_type;
    typedef std::tuple<T0> results_tuple_type;

    inline constexpr preevent(R& r, T0& x0, const char* file = 0, int line = 0) noexcept;
    inline preevent(preevent<R, T0>&& x) noexcept;

    operator unspecified_bool_type() const {
        return r_ ? (unspecified_bool_type) &tamerpriv::simple_event::empty : 0;
    }
    bool operator!() const {
        return !r_;
    }
    bool empty() const {
        return !r_;
    }

    inline void operator()(T0 v0) {
        if (r_) {
            *s0_ = std::move(v0);
            r_ = 0;
        }
    }
    inline void trigger(T0 v0) {
        operator()(v0);
    }
    inline void tuple_trigger(const results_tuple_type& vs) {
        operator()(std::get<0>(vs));
    }
    inline void unblock() noexcept {
        r_ = 0;
    }

    inline T0& result() const noexcept {
        assert(r_);
        return *s0_;
    }
    inline T0* result_pointer() const noexcept {
        return r_ ? s0_ : 0;
    }

  private:
    R* r_;
    T0* s0_;
# if !TAMER_NOTRACE
    const char* file_annotation_;
    int line_annotation_;
# endif

    preevent(const preevent<R, T0>& x) noexcept;
    template <typename TT0, typename TT1, typename TT2, typename TT3>
    friend class event;
};

template <typename R>
class preevent<R, void> {
  public:
    typedef void result_type;
    typedef std::tuple<> results_tuple_type;
    typedef tamerpriv::simple_event::unspecified_bool_type unspecified_bool_type;

    inline constexpr preevent(R& r, const char* file = 0, int line = 0) noexcept;
    inline preevent(preevent<R>&& x) noexcept;

    operator unspecified_bool_type() const {
        return r_ ? (unspecified_bool_type) &tamerpriv::simple_event::empty : 0;
    }
    bool operator!() const {
        return !r_;
    }
    bool empty() const {
        return !r_;
    }

    inline void operator()() {
        r_ = 0;
    }
    inline void trigger() {
        operator()();
    }
    inline void tuple_trigger(const results_tuple_type&) {
        operator()();
    }
    inline void unblock() noexcept {
        r_ = 0;
    }

  private:
    R* r_;
# if !TAMER_NOTRACE
    const char* file_annotation_;
    int line_annotation_;
# endif

    preevent(const preevent<R>& x) noexcept;
    template <typename TT0, typename TT1, typename TT2, typename TT3>
    friend class event;
};
#endif

/** @endcond */


/** @brief  Default constructor creates an empty event. */
template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3>::event() noexcept
    : se_(0), sv_(nullptr, nullptr, nullptr, nullptr) {
}

/** @brief  Construct an empty four-result event on rendezvous @a r.
 *  @param  x0  First result.
 *  @param  x1  Second result.
 *  @param  x2  Third result.
 *  @param  x3  Fourth result.
 */
template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3>::event(T0& x0, T1& x1, T2& x2, T3& x3) noexcept
    : se_(0), sv_(&x0, &x1, &x2, &x3) {
}

/** @brief  Construct an empty four-result event on rendezvous @a r.
 *  @param  x0  First result.
 *  @param  x1  Second result.
 *  @param  x2  Third result.
 *  @param  x3  Fourth result.
 */
template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3>::event(results_tuple_type& xs) noexcept
    : se_(0), sv_(&std::get<0>(xs), &std::get<1>(xs), &std::get<2>(xs), &std::get<3>(xs)) {
}

/** @brief  Copy-construct event from @a x.
 *  @param  x  Source event.
 */
template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3>::event(const event<T0, T1, T2, T3>& x) noexcept
    : se_(x.se_), sv_(x.sv_) {
    tamerpriv::simple_event::use(se_);
}

/** @brief  Move-construct event from @a x.
 *  @param  x  Source event.
 */
template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3>::event(event<T0, T1, T2, T3>&& x) noexcept
    : se_(x.se_), sv_(x.sv_) {
    x.se_ = 0;
}

/** @brief  Destroy the event instance.
 *  @note   The underlying occurrence is canceled if this event was the
 *          last remaining reference.
 */
template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3>::~event() noexcept {
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
inline void event<T0, T1, T2, T3>::operator()(T0 v0, T1 v1, T2 v2, T3 v3) {
    if (se_ && *se_) {
        *std::get<0>(sv_) = std::move(v0);
        *std::get<1>(sv_) = std::move(v1);
        *std::get<2>(sv_) = std::move(v2);
        *std::get<3>(sv_) = std::move(v3);
        se_->simple_trigger(true);
        se_ = 0;
    }
}

/** @brief  Trigger event.
 *  @param  v0  First trigger value.
 *  @param  v1  Second trigger value.
 *  @param  v2  Third trigger value.
 *  @param  v3  Fourth trigger value.
 *
 *  Does nothing if event is empty.
 *
 *  @note   This is a synonym for operator()().
 */
template <typename T0, typename T1, typename T2, typename T3>
inline void event<T0, T1, T2, T3>::trigger(T0 v0, T1 v1, T2 v2, T3 v3) {
    operator()(v0, v1, v2, v3);
}

/** @brief  Trigger event.
 *  @param  vs  Trigger values.
 *
 *  Does nothing if event is empty.
 *
 *  @note   This is a synonym for trigger().
 */
template <typename T0, typename T1, typename T2, typename T3>
inline void event<T0, T1, T2, T3>::tuple_trigger(const results_tuple_type& vs) {
    operator()(vs);
}

/** @brief  Unblock event.
 *
 *  Like trigger(), but does not change results.
 *
 *  Does nothing if event is empty.
 */
template <typename T0, typename T1, typename T2, typename T3>
inline void event<T0, T1, T2, T3>::unblock() noexcept {
    tamerpriv::simple_event::simple_trigger(se_, false);
    se_ = 0;
}

template <typename T0, typename T1, typename T2, typename T3>
template <size_t I>
inline void event<T0, T1, T2, T3>::set_result(typename std::tuple_element<I, results_tuple_type>::type vi) {
    if (se_ && *se_)
        *std::get<I>(sv_) = std::move(vi);
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

/** @overload */
template <typename T0, typename T1, typename T2, typename T3>
inline void event<T0, T1, T2, T3>::at_trigger(event<>&& e) {
    tamerpriv::simple_event::at_trigger(se_, e.__release_simple());
}

/** @brief  Return a no-result event for the same occurrence as @a e.
 *  @return  New event.
 *
 *  The returned event refers to the same occurrence as this event, so
 *  triggering either event makes both events empty. The returned event
 *  has no results, however, and unblocker().trigger() will leave
 *  this event's results unchanged.
 */
template <typename T0, typename T1, typename T2, typename T3>
inline event<> event<T0, T1, T2, T3>::unblocker() const noexcept {
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
inline event<T0, T1, T2, T3>& event<T0, T1, T2, T3>::operator=(const event<T0, T1, T2, T3>& x) noexcept {
    tamerpriv::simple_event::use(x.se_);
    tamerpriv::simple_event::unuse(se_);
    se_ = x.se_;
    sv_ = x.sv_;
    return *this;
}

/** @brief  Move-assign this event to @a x.
 *  @param  x  Source event.
 */
template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3>& event<T0, T1, T2, T3>::operator=(event<T0, T1, T2, T3>&& x) noexcept {
    tamerpriv::simple_event *se = se_;
    se_ = x.se_;
    sv_ = x.sv_;
    x.se_ = se;
    return *this;
}

/** @internal
 *  @brief  Set underlying occurrence.
 *  @return  *this
 */
template <typename T0, typename T1, typename T2, typename T3>
event<T0, T1, T2, T3>& event<T0, T1, T2, T3>::__instantiate(tamerpriv::abstract_rendezvous& r, uintptr_t rid, const char* file, int line) {
    TAMER_DEBUG_ASSERT(!se_);
    se_ = new tamerpriv::simple_event(r, rid, file, line);
    return *this;
}

/** @internal
 *  @brief  Fetch underlying occurrence.
 *  @return  Underlying occurrence.
 */
template <typename T0, typename T1, typename T2, typename T3>
inline tamerpriv::simple_event* event<T0, T1, T2, T3>::__get_simple() const {
    return se_;
}

/** @internal
 *  @brief  Return underlying occurrence, making this event empty.
 *  @return  Underlying occurrence.
 */
template <typename T0, typename T1, typename T2, typename T3>
inline tamerpriv::simple_event* event<T0, T1, T2, T3>::__release_simple() {
    tamerpriv::simple_event *se = se_;
    se_ = 0;
    return se;
}


/** @cond never */

template <typename T0, typename T1, typename T2>
inline event<T0, T1, T2>::event() noexcept
    : se_(0), sv_(nullptr, nullptr, nullptr) {
}

template <typename T0, typename T1, typename T2>
inline event<T0, T1, T2>::event(T0& x0, T1& x1, T2& x2) noexcept
    : se_(0), sv_(&x0, &x1, &x2) {
}

template <typename T0, typename T1, typename T2>
inline event<T0, T1, T2>::event(results_tuple_type& xs) noexcept
    : se_(0), sv_(&std::get<0>(xs), &std::get<1>(xs), &std::get<2>(xs)) {
}

template <typename T0, typename T1, typename T2>
inline void event<T0, T1, T2>::operator()(T0 v0, T1 v1, T2 v2) {
    if (se_ && *se_) {
        *std::get<0>(sv_) = std::move(v0);
        *std::get<1>(sv_) = std::move(v1);
        *std::get<2>(sv_) = std::move(v2);
        se_->simple_trigger(true);
        se_ = 0;
    }
}

template <typename T0, typename T1, typename T2>
inline void event<T0, T1, T2>::trigger(T0 v0, T1 v1, T2 v2) {
    operator()(v0, v1, v2);
}

template <typename T0, typename T1, typename T2>
inline void event<T0, T1, T2>::tuple_trigger(const results_tuple_type& vs) {
    operator()(std::get<0>(vs), std::get<1>(vs), std::get<2>(vs));
}

template <typename T0, typename T1, typename T2>
inline void event<T0, T1, T2>::unblock() noexcept {
    tamerpriv::simple_event::simple_trigger(se_, false);
    se_ = 0;
}

template <typename T0, typename T1, typename T2>
template <size_t I>
inline void event<T0, T1, T2>::set_result(typename std::tuple_element<I, results_tuple_type>::type vi) {
    if (se_ && *se_)
        *std::get<I>(sv_) = std::move(vi);
}


template <typename T0, typename T1>
inline event<T0, T1>::event() noexcept
    : se_(0), sv_(nullptr, nullptr) {
}

template <typename T0, typename T1>
inline event<T0, T1>::event(T0& x0, T1& x1) noexcept
    : se_(0), sv_(&x0, &x1) {
}

template <typename T0, typename T1>
inline event<T0, T1>::event(results_tuple_type& xs) noexcept
    : se_(0), sv_(&std::get<0>(xs), &std::get<1>(xs)) {
}

template <typename T0, typename T1>
inline void event<T0, T1>::operator()(T0 v0, T1 v1) {
    if (se_ && *se_) {
        *std::get<0>(sv_) = std::move(v0);
        *std::get<1>(sv_) = std::move(v1);
        se_->simple_trigger(true);
        se_ = 0;
    }
}

template <typename T0, typename T1>
inline void event<T0, T1>::trigger(T0 v0, T1 v1) {
    operator()(v0, v1);
}

template <typename T0, typename T1>
inline void event<T0, T1>::tuple_trigger(const results_tuple_type& vs) {
    operator()(std::get<0>(vs), std::get<1>(vs));
}

template <typename T0, typename T1>
inline void event<T0, T1>::unblock() noexcept {
    tamerpriv::simple_event::simple_trigger(se_, false);
    se_ = 0;
}

template <typename T0, typename T1>
template <size_t I>
inline void event<T0, T1>::set_result(typename std::tuple_element<I, results_tuple_type>::type vi) {
    if (se_ && *se_)
        *std::get<I>(sv_) = std::move(vi);
}


template <typename T0>
inline event<T0>::event() noexcept
    : se_(0), s0_(0) {
}

template <typename T0>
inline event<T0>::event(T0& x0) noexcept
    : se_(0), s0_(&x0) {
}

template <typename T0>
inline event<T0>::event(results_tuple_type& xs) noexcept
    : se_(0), s0_(&std::get<0>(xs)) {
}

template <typename T0>
inline void event<T0>::operator()(T0 v0) {
    if (se_ && *se_) {
        *s0_ = std::move(v0);
        se_->simple_trigger(true);
        se_ = 0;
    }
}

template <typename T0>
inline void event<T0>::trigger(T0 v0) {
    operator()(v0);
}

template <typename T0>
inline void event<T0>::tuple_trigger(const results_tuple_type& vs) {
    operator()(std::get<0>(vs));
}

template <typename T0>
inline void event<T0>::unblock() noexcept {
    tamerpriv::simple_event::simple_trigger(se_, false);
    se_ = 0;
}

template <typename T0>
template <size_t I>
inline void event<T0>::set_result(typename std::tuple_element<I, results_tuple_type>::type vi) {
    if (se_ && *se_)
        *s0_ = std::move(vi);
}


inline event<>::event() noexcept
    : se_(0) {
}

inline event<>::event(results_tuple_type&) noexcept
    : se_(0) {
}

inline void event<>::operator()() noexcept {
    tamerpriv::simple_event::simple_trigger(se_, false);
    se_ = 0;
}

inline void event<>::trigger() noexcept {
    operator()();
}

inline void event<>::tuple_trigger(const results_tuple_type&) noexcept {
    operator()();
}

inline void event<>::unblock() noexcept {
    operator()();
}

/** @endcond */

/** @brief  Return event's result.
 *  @pre    !empty()
 */
template <typename T0>
inline T0& event<T0>::result() const noexcept {
    assert(!empty());
    return *s0_;
}

/** @brief  Return a pointer to the event's result, or null if empty. */
template <typename T0>
inline T0* event<T0>::result_pointer() const noexcept {
    return se_ && *se_ ? s0_ : 0;
}


#if TAMER_HAVE_PREEVENT
template <typename R, typename T0>
inline constexpr preevent<R, T0>::preevent(R& r, T0& x0, const char* file, int line) noexcept
    : r_(&((void) file, (void) line, r)), s0_(&x0) TAMER_IFTRACE(, file_annotation_(file), line_annotation_(line)) {
    // NB "(void) file, (void) line" in initializer b/c constexpr doesn't
    // allow body
}

template <typename R, typename T0>
inline preevent<R, T0>::preevent(preevent<R, T0>&& x) noexcept
    : r_(x.r_), s0_(x.s0_) TAMER_IFTRACE(, file_annotation_(x.file_annotation_), line_annotation_(x.line_annotation_)) {
    x.r_ = 0;
}

template <typename R>
inline constexpr preevent<R>::preevent(R& r, const char* file, int line) noexcept
    : r_(&((void) file, (void) line, r)) TAMER_IFTRACE(, file_annotation_(file), line_annotation_(line)) {
}

template <typename R>
inline preevent<R>::preevent(preevent<R>&& x) noexcept
    : r_(x.r_) TAMER_IFTRACE(, file_annotation_(x.file_annotation_), line_annotation_(x.line_annotation_)) {
    x.r_ = 0;
}

# undef TAMER_PREEVENT_CTOR
#endif


/** @defgroup make_event Helper functions for making events
 *
 *  The @c make_event() helper function simplifies event creation.  @c
 *  make_event() automatically selects the right type of event for its
 *  arguments.  There are 3*5 = 15 versions, one for each combination of event
 *  IDs and results.
 *
 *  @{ */

/** @brief  Construct a four-result event on rendezvous @a r with ID @a eid.
 *  @param  r   Rendezvous.
 *  @param  eid Event ID.
 *  @param  x0  First result.
 *  @param  x1  Second result.
 *  @param  x2  Third result.
 *  @param  x3  Fourth result.
 *
 *  @note Versions of this function exist for any combination of one
 *  or zero event IDs and four, three, two, one, or zero results. For
 *  example, <code>make_event(r)</code> creates a zero-ID, zero-result event
 *  on <code>rendezvous<> r</code>, while <code>make_event(r, 1, i,
 *  j)</code> might create a one-ID, two-result event on
 *  <code>rendezvous<int> r</code>.
 */
template <typename R, typename I, typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> make_event(one_argument_rendezvous_tag<R>& r, const I& eid, T0& x0, T1& x1, T2& x2, T3& x3)
{
    return event<T0, T1, T2, T3>(x0, x1, x2, x3).__instantiate(static_cast<R&>(r), static_cast<R&>(r).make_rid(eid));
}

template <typename R, typename I, typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> make_event(one_argument_rendezvous_tag<R>& r, const I& eid, typename event<T0, T1, T2, T3>::results_tuple_type& xs)
{
    return event<T0, T1, T2, T3>(xs).__instantiate(static_cast<R&>(r), static_cast<R&>(r).make_rid(eid));
}

template <typename R, typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> make_event(zero_argument_rendezvous_tag<R>& r, T0& x0, T1& x1, T2& x2, T3& x3)
{
    return event<T0, T1, T2, T3>(x0, x1, x2, x3).__instantiate(static_cast<R&>(r), 0);
}

template <typename R, typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> make_event(zero_argument_rendezvous_tag<R>& r, typename event<T0, T1, T2, T3>::results_tuple_type& xs)
{
    return event<T0, T1, T2, T3>(xs).__instantiate(static_cast<R&>(r), 0);
}

template <typename R, typename I, typename T0, typename T1, typename T2>
inline event<T0, T1, T2> make_event(one_argument_rendezvous_tag<R>& r, const I& eid, T0& x0, T1& x1, T2& x2)
{
    return event<T0, T1, T2>(x0, x1, x2).__instantiate(static_cast<R&>(r), static_cast<R&>(r).make_rid(eid));
}

template <typename R, typename T0, typename T1, typename T2>
inline event<T0, T1, T2> make_event(zero_argument_rendezvous_tag<R>& r, T0& x0, T1& x1, T2& x2)
{
    return event<T0, T1, T2>(x0, x1, x2).__instantiate(static_cast<R&>(r), 0);
}

template <typename R, typename I, typename T0, typename T1>
inline event<T0, T1> make_event(one_argument_rendezvous_tag<R>& r, const I& eid, T0& x0, T1& x1)
{
    return event<T0, T1>(x0, x1).__instantiate(static_cast<R&>(r), static_cast<R&>(r).make_rid(eid));
}

template <typename R, typename T0, typename T1>
inline event<T0, T1> make_event(zero_argument_rendezvous_tag<R>& r, T0& x0, T1& x1)
{
    return event<T0, T1>(x0, x1).__instantiate(static_cast<R&>(r), 0);
}

template <typename R, typename I, typename T0>
inline event<T0> make_event(one_argument_rendezvous_tag<R>& r, const I& eid, T0& x0)
{
    return event<T0>(x0).__instantiate(static_cast<R&>(r), static_cast<R&>(r).make_rid(eid));
}

#if !TAMER_HAVE_PREEVENT || !TAMER_PREFER_PREEVENT
template <typename R, typename T0>
inline event<T0> make_event(zero_argument_rendezvous_tag<R>& r, T0& x0)
{
    return event<T0>(x0).__instantiate(static_cast<R&>(r), 0);
}
#else
template <typename R, typename T0>
inline preevent<R, T0> make_event(zero_argument_rendezvous_tag<R>& r, T0& x0)
{
    return preevent<R, T0>(static_cast<R&>(r), x0);
}
#endif

#if TAMER_HAVE_PREEVENT
template <typename R, typename T0>
inline preevent<R, T0> make_preevent(zero_argument_rendezvous_tag<R>& r, T0& x0)
{
    return preevent<R, T0>(static_cast<R&>(r), x0);
}
#endif

template <typename R, typename I>
inline event<> make_event(one_argument_rendezvous_tag<R>& r, const I& eid)
{
    return event<>().__instantiate(static_cast<R&>(r), static_cast<R&>(r).make_rid(eid));
}

#if !TAMER_HAVE_PREEVENT || !TAMER_PREFER_PREEVENT
template <typename R>
inline event<> make_event(zero_argument_rendezvous_tag<R>& r)
{
    return event<>().__instantiate(static_cast<R&>(r), 0);
}
#else
template <typename R>
inline preevent<R> make_event(zero_argument_rendezvous_tag<R>& r)
{
    return preevent<R>(static_cast<R&>(r));
}
#endif

#if TAMER_HAVE_PREEVENT
template <typename R>
inline preevent<R> make_preevent(zero_argument_rendezvous_tag<R>& r)
{
    return preevent<R>(static_cast<R&>(r));
}
#endif

/** @} */


/** @cond never */
template <typename I> template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> rendezvous<I>::make_event(const I& eid, T0& x0, T1& x1, T2& x2, T3& x3)
{
    return tamer::make_event(*this, eid, x0, x1, x2, x3);
}

template <typename I> template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> simple_rendezvous<I>::make_event(I eid, T0& x0, T1& x1, T2& x2, T3& x3)
{
    return tamer::make_event(*this, eid, x0, x1, x2, x3);
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

template <typename I> template <typename T0, typename T1, typename T2>
inline event<T0, T1, T2> rendezvous<I>::make_event(const I& eid, T0& x0, T1& x1, T2& x2)
{
    return tamer::make_event(*this, eid, x0, x1, x2);
}

template <typename I> template <typename T0, typename T1, typename T2>
inline event<T0, T1, T2> simple_rendezvous<I>::make_event(I eid, T0& x0, T1& x1, T2& x2)
{
    return tamer::make_event(*this, eid, x0, x1, x2);
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

template <typename I> template <typename T0, typename T1>
inline event<T0, T1> rendezvous<I>::make_event(const I& eid, T0& x0, T1& x1)
{
    return tamer::make_event(*this, eid, x0, x1);
}

template <typename I> template <typename T0, typename T1>
inline event<T0, T1> simple_rendezvous<I>::make_event(I eid, T0& x0, T1& x1)
{
    return tamer::make_event(*this, eid, x0, x1);
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

template <typename I> template <typename T0>
inline event<T0> rendezvous<I>::make_event(const I& eid, T0& x0)
{
    return tamer::make_event(*this, eid, x0);
}

template <typename I> template <typename T0>
inline event<T0> simple_rendezvous<I>::make_event(I eid, T0& x0)
{
    return tamer::make_event(*this, eid, x0);
}

#if !TAMER_HAVE_PREEVENT || !TAMER_PREFER_PREEVENT
template <typename T0>
inline event<T0> rendezvous<>::make_event(T0& x0) {
    return tamer::make_event(*this, x0);
}

template <typename T0>
inline event<T0> gather_rendezvous::make_event(T0& x0) {
    return tamer::make_event(*this, x0);
}
#else
template <typename T0>
inline preevent<rendezvous<>, T0> rendezvous<>::make_event(T0& x0) {
    return tamer::make_preevent(*this, x0);
}

template <typename T0>
inline preevent<gather_rendezvous, T0> gather_rendezvous::make_event(T0& x0) {
    return tamer::make_preevent(*this, x0);
}
#endif

#if TAMER_HAVE_PREEVENT
template <typename T0>
inline preevent<rendezvous<>, T0> rendezvous<>::make_preevent(T0& x0)
{
    return preevent<rendezvous<>, T0>(*this, x0);
}

template <typename T0>
inline preevent<gather_rendezvous, T0> gather_rendezvous::make_preevent(T0& x0)
{
    return preevent<gather_rendezvous, T0>(*this, x0);
}
#endif

template <typename I>
inline event<> rendezvous<I>::make_event(const I& eid)
{
    return tamer::make_event(*this, eid);
}

template <typename I>
inline event<> simple_rendezvous<I>::make_event(I eid)
{
    return tamer::make_event(*this, eid);
}

#if !TAMER_HAVE_PREEVENT || !TAMER_PREFER_PREEVENT
inline event<> rendezvous<>::make_event() {
    return tamer::make_event(*this);
}

inline event<> gather_rendezvous::make_event() {
    return tamer::make_event(*this);
}
#else
inline preevent<rendezvous<> > rendezvous<>::make_event() {
    return tamer::make_preevent(*this);
}

inline preevent<gather_rendezvous> gather_rendezvous::make_event() {
    return tamer::make_preevent(*this);
}
#endif

#if TAMER_HAVE_PREEVENT
inline preevent<rendezvous<> > rendezvous<>::make_preevent() {
    return preevent<rendezvous<> >(*this);
}

inline preevent<gather_rendezvous> gather_rendezvous::make_preevent() {
    return preevent<gather_rendezvous>(*this);
}
#endif
/** @endcond never */


template <typename R, typename I, typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> make_annotated_event(const char *file, int line, one_argument_rendezvous_tag<R>& r, const I& eid, T0& x0, T1& x1, T2& x2, T3& x3)
{
    return event<T0, T1, T2, T3>(x0, x1, x2, x3).__instantiate(static_cast<R&>(r), static_cast<R&>(r).make_rid(eid), file, line);
}

template <typename R, typename I, typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> make_annotated_event(const char *file, int line, one_argument_rendezvous_tag<R>& r, const I& eid, typename event<T0, T1, T2, T3>::results_tuple_type& xs)
{
    return event<T0, T1, T2, T3>(xs).__instantiate(static_cast<R&>(r), static_cast<R&>(r).make_rid(eid), file, line);
}

template <typename R, typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> make_annotated_event(const char *file, int line, zero_argument_rendezvous_tag<R>& r, T0& x0, T1& x1, T2& x2, T3& x3)
{
    return event<T0, T1, T2, T3>(x0, x1, x2, x3).__instantiate(static_cast<R&>(r), 0, file, line);
}

template <typename R, typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> make_annotated_event(const char *file, int line, zero_argument_rendezvous_tag<R>& r, typename event<T0, T1, T2, T3>::results_tuple_type& xs)
{
    return event<T0, T1, T2, T3>(xs).__instantiate(static_cast<R&>(r), 0, file, line);
}

template <typename R, typename I, typename T0, typename T1, typename T2>
inline event<T0, T1, T2> make_annotated_event(const char *file, int line, one_argument_rendezvous_tag<R>& r, const I& eid, T0& x0, T1& x1, T2& x2)
{
    return event<T0, T1, T2>(x0, x1, x2).__instantiate(static_cast<R&>(r), static_cast<R&>(r).make_rid(eid), file, line);
}

template <typename R, typename I, typename T0, typename T1, typename T2>
inline event<T0, T1, T2> make_annotated_event(const char *file, int line, one_argument_rendezvous_tag<R>& r, const I& eid, typename event<T0, T1, T2>::results_tuple_type& xs)
{
    return event<T0, T1, T2>(xs).__instantiate(static_cast<R&>(r), static_cast<R&>(r).make_rid(eid), file, line);
}

template <typename R, typename T0, typename T1, typename T2>
inline event<T0, T1, T2> make_annotated_event(const char *file, int line, zero_argument_rendezvous_tag<R>& r, T0& x0, T1& x1, T2& x2)
{
    return event<T0, T1, T2>(x0, x1, x2).__instantiate(static_cast<R&>(r), 0, file, line);
}

template <typename R, typename T0, typename T1, typename T2>
inline event<T0, T1, T2> make_annotated_event(const char *file, int line, zero_argument_rendezvous_tag<R>& r, typename event<T0, T1, T2>::results_tuple_type& xs)
{
    return event<T0, T1, T2>(xs).__instantiate(static_cast<R&>(r), 0, file, line);
}

template <typename R, typename I, typename T0, typename T1>
inline event<T0, T1> make_annotated_event(const char *file, int line, one_argument_rendezvous_tag<R>& r, const I& eid, T0& x0, T1& x1)
{
    return event<T0, T1>(x0, x1).__instantiate(static_cast<R&>(r), static_cast<R&>(r).make_rid(eid), file, line);
}

template <typename R, typename I, typename T0, typename T1>
inline event<T0, T1> make_annotated_event(const char *file, int line, one_argument_rendezvous_tag<R>& r, const I& eid, typename event<T0, T1>::results_tuple_type& xs)
{
    return event<T0, T1>(xs).__instantiate(static_cast<R&>(r), static_cast<R&>(r).make_rid(eid), file, line);
}

template <typename R, typename T0, typename T1>
inline event<T0, T1> make_annotated_event(const char *file, int line, zero_argument_rendezvous_tag<R>& r, T0& x0, T1& x1)
{
    return event<T0, T1>(x0, x1).__instantiate(static_cast<R&>(r), 0, file, line);
}

template <typename R, typename T0, typename T1>
inline event<T0, T1> make_annotated_event(const char *file, int line, zero_argument_rendezvous_tag<R>& r, typename event<T0, T1>::results_tuple_type& xs)
{
    return event<T0, T1>(xs).__instantiate(static_cast<R&>(r), 0, file, line);
}

template <typename R, typename I, typename T0>
inline event<T0> make_annotated_event(const char *file, int line, one_argument_rendezvous_tag<R>& r, const I& eid, T0& x0)
{
    return event<T0>(x0).__instantiate(static_cast<R&>(r), static_cast<R&>(r).make_rid(eid), file, line);
}

template <typename R, typename I, typename T0>
inline event<T0> make_annotated_event(const char *file, int line, one_argument_rendezvous_tag<R>& r, const I& eid, std::tuple<T0>& xs)
{
    return event<T0>(xs).__instantiate(static_cast<R&>(r), static_cast<R&>(r).make_rid(eid), file, line);
}

#if !TAMER_HAVE_PREEVENT || !TAMER_PREFER_PREEVENT
template <typename R, typename T0>
inline event<T0> make_annotated_event(const char *file, int line, zero_argument_rendezvous_tag<R>& r, T0& x0)
{
    return event<T0>(x0).__instantiate(static_cast<R&>(r), 0, file, line);
}

template <typename R, typename T0>
inline event<T0> make_annotated_event(const char *file, int line, zero_argument_rendezvous_tag<R>& r, std::tuple<T0>& xs)
{
    return event<T0>(xs).__instantiate(static_cast<R&>(r), 0, file, line);
}
#else
template <typename R, typename T0>
inline preevent<R, T0> make_annotated_event(const char *file, int line, zero_argument_rendezvous_tag<R>& r, T0& x0)
{
    return preevent<R, T0>(static_cast<R&>(r), x0, file, line);
}

template <typename R, typename T0>
inline preevent<T0> make_annotated_event(const char *file, int line, zero_argument_rendezvous_tag<R>& r, std::tuple<T0>& xs)
{
    return preevent<R, T0>(static_cast<R&>(r), xs, file, line);
}
#endif

#if TAMER_HAVE_PREEVENT
template <typename R, typename T0>
inline preevent<R, T0> make_annotated_preevent(const char *file, int line, zero_argument_rendezvous_tag<R>& r, T0& x0)
{
    return preevent<R, T0>(static_cast<R&>(r), x0, file, line);
}

template <typename R, typename T0>
inline preevent<R, T0> make_annotated_preevent(const char *file, int line, zero_argument_rendezvous_tag<R>& r, std::tuple<T0>& xs)
{
    return preevent<R, T0>(static_cast<R&>(r), xs, file, line);
}
#endif

template <typename R, typename I>
inline event<> make_annotated_event(const char *file, int line, one_argument_rendezvous_tag<R>& r, const I& eid)
{
    return event<>().__instantiate(static_cast<R&>(r), static_cast<R&>(r).make_rid(eid), file, line);
}

template <typename R, typename I>
inline event<> make_annotated_event(const char *file, int line, one_argument_rendezvous_tag<R>& r, const I& eid, typename event<>::results_tuple_type& xs)
{
    return event<>(xs).__instantiate(static_cast<R&>(r), static_cast<R&>(r).make_rid(eid), file, line);
}

#if !TAMER_HAVE_PREEVENT || !TAMER_PREFER_PREEVENT
template <typename R>
inline event<> make_annotated_event(const char *file, int line, zero_argument_rendezvous_tag<R>& r)
{
    return event<>().__instantiate(static_cast<R&>(r), 0, file, line);
}

template <typename R>
inline event<> make_annotated_event(const char *file, int line, zero_argument_rendezvous_tag<R>& r, typename event<>::results_tuple_type& xs)
{
    return event<>(xs).__instantiate(static_cast<R&>(r), 0, file, line);
}
#else
template <typename R>
inline preevent<R> make_annotated_event(const char *file, int line, zero_argument_rendezvous_tag<R>& r)
{
    return preevent<R>(static_cast<R&>(r), file, line);
}

template <typename R>
inline preevent<R> make_annotated_event(const char *file, int line, zero_argument_rendezvous_tag<R>& r, typename event<>::results_tuple_type& xs)
{
    (void) xs;
    return preevent<R>(static_cast<R&>(r), file, line);
}
#endif

#if TAMER_HAVE_PREEVENT
template <typename R>
inline preevent<R> make_annotated_preevent(const char *file, int line, zero_argument_rendezvous_tag<R>& r)
{
    return preevent<R>(static_cast<R&>(r), file, line);
}

template <typename R>
inline event<> make_annotated_preevent(const char *file, int line, zero_argument_rendezvous_tag<R>& r, typename event<>::results_tuple_type& xs)
{
    (void) xs;
    return preevent<R>(static_cast<R&>(r), file, line);
}
#endif

namespace tamerpriv {
template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> distribute_rendezvous<T0, T1, T2, T3>::make_event() {
    return tamer::TAMER_MAKE_FN_ANNOTATED_EVENT(*this, vs_);
}
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

template <typename T0, typename T1, typename T2>
inline void event<T0, T1, T2>::at_trigger(event<>&& e) {
    tamerpriv::simple_event::at_trigger(se_, e.__release_simple());
}

template <typename T0, typename T1>
inline void event<T0, T1>::at_trigger(event<>&& e) {
    tamerpriv::simple_event::at_trigger(se_, e.__release_simple());
}

template <typename T0>
inline void event<T0>::at_trigger(event<>&& e) {
    tamerpriv::simple_event::at_trigger(se_, e.__release_simple());
}

template <typename T0, typename T1, typename T2>
inline event<> event<T0, T1, T2>::unblocker() const noexcept {
    tamerpriv::simple_event::use(se_);
    return event<>::__make(se_);
}

template <typename T0, typename T1>
inline event<> event<T0, T1>::unblocker() const noexcept {
    tamerpriv::simple_event::use(se_);
    return event<>::__make(se_);
}

template <typename T0>
inline event<> event<T0>::unblocker() const noexcept {
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

#if TAMER_HAVE_PREEVENT
template <typename T0> template <typename R>
inline event<T0>::event(preevent<R, T0>&& x)
    : se_(x.r_ ? new tamerpriv::simple_event(*x.r_, 0, TAMER_IFTRACE_ELSE(x.file_annotation_, 0), TAMER_IFTRACE_ELSE(x.line_annotation_, 0)) : 0), s0_(x.s0_) {
    x.r_ = 0;
}

template <typename R>
inline event<>::event(preevent<R>&& x)
    : se_(x.r_ ? new tamerpriv::simple_event(*x.r_, 0, TAMER_IFTRACE_ELSE(x.file_annotation_, 0), TAMER_IFTRACE_ELSE(x.line_annotation_, 0)) : 0) {
    x.r_ = 0;
}
#endif

template <typename T0>
inline event<T0>::event(const event<>& x, results_tuple_type& vs)
    : se_(x.__get_simple()), s0_(&std::get<0>(vs)) {
    tamerpriv::simple_event::use(se_);
}

template <typename T0>
inline event<T0>::event(event<>&& x, results_tuple_type& vs) noexcept
    : se_(x.__release_simple()), s0_(&std::get<0>(vs)) {
}

template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3>& event<T0, T1, T2, T3>::operator+=(event<T0, T1, T2, T3> x) {
    typedef tamerpriv::distribute_rendezvous<T0, T1, T2, T3> rendezvous_type;
    rendezvous_type::make(*this, TAMER_MOVE(x));
    return *this;
}

template <typename T0, typename T1, typename T2>
inline event<T0, T1, T2>& event<T0, T1, T2>::operator+=(event<T0, T1, T2> x) {
    typedef tamerpriv::distribute_rendezvous<T0, T1, T2> rendezvous_type;
    rendezvous_type::make(*this, TAMER_MOVE(x));
    return *this;
}

template <typename T0, typename T1>
inline event<T0, T1>& event<T0, T1>::operator+=(event<T0, T1> x) {
    typedef tamerpriv::distribute_rendezvous<T0, T1> rendezvous_type;
    rendezvous_type::make(*this, TAMER_MOVE(x));
    return *this;
}

template <typename T0>
inline event<T0>& event<T0>::operator+=(event<T0> x) {
    typedef tamerpriv::distribute_rendezvous<T0> rendezvous_type;
    rendezvous_type::make(*this, TAMER_MOVE(x));
    return *this;
}

inline event<>& event<>::operator+=(event<> x) {
    typedef tamerpriv::distribute_rendezvous<> rendezvous_type;
    rendezvous_type::make(*this, TAMER_MOVE(x));
    return *this;
}

template <typename T0, typename T1>
inline event<T0, T1> operator+(event<T0, T1> a, event<T0, T1> b) {
    return a += b;
}

template <typename T0, typename T1, typename T2, typename T3>
inline const char* event<T0, T1, T2, T3>::file_annotation() const {
    return se_ ? se_->file_annotation() : 0;
}

template <typename T0, typename T1, typename T2>
inline const char* event<T0, T1, T2>::file_annotation() const {
    return se_ ? se_->file_annotation() : 0;
}

template <typename T0, typename T1>
inline const char* event<T0, T1>::file_annotation() const {
    return se_ ? se_->file_annotation() : 0;
}

template <typename T0>
inline const char* event<T0>::file_annotation() const {
    return se_ ? se_->file_annotation() : 0;
}

inline const char* event<>::file_annotation() const {
    return se_ ? se_->file_annotation() : 0;
}

template <typename T0, typename T1, typename T2, typename T3>
inline int event<T0, T1, T2, T3>::line_annotation() const {
    return se_ ? se_->line_annotation() : 0;
}

template <typename T0, typename T1, typename T2>
inline int event<T0, T1, T2>::line_annotation() const {
    return se_ ? se_->line_annotation() : 0;
}

template <typename T0, typename T1>
inline int event<T0, T1>::line_annotation() const {
    return se_ ? se_->line_annotation() : 0;
}

template <typename T0>
inline int event<T0>::line_annotation() const {
    return se_ ? se_->line_annotation() : 0;
}

inline int event<>::line_annotation() const {
    return se_ ? se_->line_annotation() : 0;
}

} // namespace tamer
#endif /* TAMER_EVENT_HH */
