#ifndef TAMER_RENDEZVOUS_HH
#define TAMER_RENDEZVOUS_HH 1
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
#include <cassert>
#include <cstring>
#include <tamer/xbase.hh>
namespace tamer {

/** @file <tamer/rendezvous.hh>
 *  @brief  The rendezvous template classes.
 */

/** @class rendezvous tamer/rendezvous.hh <tamer/tamer.hh>
 *  @brief  A set of watched events.
 *
 *  Rendezvous may also be declared with one or zero template arguments, as in
 *  <tt>rendezvous<T></tt> or <tt>rendezvous<></tt>.  Each specialized
 *  rendezvous class has functions similar to the full-featured rendezvous,
 *  but with parameters to @c join appropriate to the template arguments.
 *  Specialized rendezvous implementations are often more efficient than the
 *  full @c rendezvous.
 */
template <typename I>
class rendezvous : public tamerpriv::explicit_rendezvous,
                   public one_argument_rendezvous_tag<rendezvous<I> > {
  public:
    inline rendezvous(rendezvous_flags flags = rnormal);
    inline ~rendezvous();

    template <typename T0, typename T1, typename T2, typename T3>
    inline event<T0, T1, T2, T3> make_event(const I& eid,
                                            T0& x0, T1& x1, T2& x2, T3& x3);
    template <typename T0, typename T1, typename T2>
    inline event<T0, T1, T2> make_event(const I& eid, T0& x0, T1& x1, T2& x2);
    template <typename T0, typename T1>
    inline event<T0, T1> make_event(const I& eid, T0& x0, T1& x1);
    template <typename T0>
    inline event<T0> make_event(const I& eid, T0& x0);
    inline event<> make_event(const I& eid);

    inline bool has_ready() const;
    inline bool has_waiting() const;
    inline bool has_events() const;

    inline bool join(I& eid);
    void clear();

    inline uintptr_t make_rid(const I& eid);
    using tamerpriv::blocking_rendezvous::block;
};


/** @brief  Construct rendezvous.
 *  @param  flags  tamer::rnormal (default) or tamer::rvolatile
 *
 *  Pass tamer::rvolatile as the @a flags parameter to create a volatile
 *  rendezvous.  Volatile rendezvous do not generate error messages when
 *  their events are dereferenced before trigger. */
template <typename I>
inline rendezvous<I>::rendezvous(rendezvous_flags flags)
    : explicit_rendezvous(flags) {
}

/** @brief  Destroy rendezvous. */
template <typename I>
inline rendezvous<I>::~rendezvous() {
    if (waiting_ || ready_)
	clear();
}

/** @brief  Test if any events are ready.
 *
 *  An event is ready if it has triggered, but no @a join or @c twait
 *  has reported it yet. */
template <typename I>
inline bool rendezvous<I>::has_ready() const {
    return ready_;
}

/** @brief  Test if any events are waiting.
 *
 *  An event is waiting until it is either triggered or canceled. */
template <typename I>
inline bool rendezvous<I>::has_waiting() const {
    return waiting_;
}

/** @brief  Test if any events are ready or waiting. */
template <typename I>
inline bool rendezvous<I>::has_events() const {
    return ready_ || waiting_;
}

/** @brief  Report the next ready event.
 *  @param[out]  eid  Set to the event ID of the ready event.
 *  @return  True if there was a ready event, false otherwise.
 *
 *  @a eid was modified if and only if @a join returns true.
 */
template <typename I>
inline bool rendezvous<I>::join(I& eid) {
    if (ready_) {
	I* eidp = reinterpret_cast<I*>(pop_ready());
	eid = TAMER_MOVE(*eidp);
	delete eidp;
	return true;
    } else
	return false;
}

/** @brief  Remove all events from this rendezvous.
 *
 *  Every event waiting on this rendezvous is made empty.  After clear(),
 *  has_events() will return false. */
template <typename I>
void rendezvous<I>::clear() {
    for (tamerpriv::simple_event *e = waiting_; e; e = e->next())
	delete reinterpret_cast<I*>(e->rid());
    abstract_rendezvous::remove_waiting();
    for (tamerpriv::simple_event *e = ready_; e; e = e->next())
	delete reinterpret_cast<I*>(e->rid());
    explicit_rendezvous::remove_ready();
}

/** @internal
 *  @brief  Add an occurrence to this rendezvous.
 *  @param  eid  The occurrence's event ID.
 */
template <typename I>
inline uintptr_t rendezvous<I>::make_rid(const I& eid) {
    return reinterpret_cast<uintptr_t>(new I(eid));
}


/** @cond specialized_rendezvous */

template <typename I>
class simple_rendezvous : public tamerpriv::explicit_rendezvous,
			  public one_argument_rendezvous_tag<simple_rendezvous<I> > {
  public:
    inline simple_rendezvous(rendezvous_flags flags = rnormal);
    inline ~simple_rendezvous();

    template <typename T0, typename T1, typename T2, typename T3>
    inline event<T0, T1, T2, T3> make_event(I eid,
                                            T0& x0, T1& x1, T2& x2, T3& x3);
    template <typename T0, typename T1, typename T2>
    inline event<T0, T1, T2> make_event(I eid, T0& x0, T1& x1, T2& x2);
    template <typename T0, typename T1>
    inline event<T0, T1> make_event(I eid, T0& x0, T1& x1);
    template <typename T0>
    inline event<T0> make_event(I eid, T0& x0);
    inline event<> make_event(I eid);

    inline bool has_ready() const	{ return ready_; }
    inline bool has_waiting() const	{ return waiting_; }
    inline bool has_events() const	{ return ready_ || waiting_; }

    inline bool join(I& eid);
    void clear();

    inline uintptr_t make_rid(I eid) TAMER_NOEXCEPT;
    using tamerpriv::blocking_rendezvous::block;
};

template <typename T>
inline simple_rendezvous<T>::simple_rendezvous(rendezvous_flags flags)
    : explicit_rendezvous(flags) {
}

template <typename T>
inline simple_rendezvous<T>::~simple_rendezvous() {
    if (waiting_ || ready_)
	clear();
}

template <typename T>
inline bool simple_rendezvous<T>::join(T& eid) {
    if (ready_) {
	eid = tamerpriv::rid_cast<T>::out(pop_ready());
	return true;
    } else
	return false;
}

template <typename T>
void simple_rendezvous<T>::clear() {
    abstract_rendezvous::remove_waiting();
    explicit_rendezvous::remove_ready();
}

template <typename T>
inline uintptr_t simple_rendezvous<T>::make_rid(T eid) TAMER_NOEXCEPT {
    return tamerpriv::rid_cast<T>::in(eid);
}


template <>
class rendezvous<uintptr_t> : public simple_rendezvous<uintptr_t> {
  public:
    inline rendezvous(rendezvous_flags flags = rnormal)
	: simple_rendezvous<uintptr_t>(flags) {
    }
};

template <typename T>
class rendezvous<T*> : public simple_rendezvous<T*> {
  public:
    inline rendezvous(rendezvous_flags flags = rnormal)
	: simple_rendezvous<T*>(flags) {
    }
};

template <>
class rendezvous<int> : public simple_rendezvous<int> {
  public:
    inline rendezvous(rendezvous_flags flags = rnormal)
	: simple_rendezvous<int>(flags) {
    }
};

template <>
class rendezvous<bool> : public simple_rendezvous<bool> {
  public:
    inline rendezvous(rendezvous_flags flags = rnormal)
	: simple_rendezvous<bool>(flags) {
    }
};


template <>
class rendezvous<> : public tamerpriv::explicit_rendezvous,
		     public zero_argument_rendezvous_tag<rendezvous<> > {
  public:
    inline rendezvous(rendezvous_flags flags = rnormal)
	: explicit_rendezvous(flags) {
    }
    inline ~rendezvous() {
	if (waiting_ || ready_)
	    clear();
    }

    template <typename T0, typename T1, typename T2, typename T3>
    inline event<T0, T1, T2, T3> make_event(T0& x0, T1& x1, T2& x2, T3& x3);
    template <typename T0, typename T1, typename T2>
    inline event<T0, T1, T2> make_event(T0& x0, T1& x1, T2& x2);
    template <typename T0, typename T1>
    inline event<T0, T1> make_event(T0& x0, T1& x1);
#if !TAMER_HAVE_PREEVENT || !TAMER_PREFER_PREEVENT
    template <typename T0>
    inline event<T0> make_event(T0& x0);
    inline event<> make_event();
#else
    template <typename T0>
    inline preevent<rendezvous<>, T0> make_event(T0& x0);
    inline preevent<rendezvous<> > make_event();
#endif
#if TAMER_HAVE_PREEVENT
    template <typename T0>
    inline preevent<rendezvous<>, T0> make_preevent(T0& x0);
    inline preevent<rendezvous<> > make_preevent();
#endif

    inline bool has_ready() const	{ return ready_; }
    inline bool has_waiting() const	{ return waiting_; }
    inline bool has_events() const	{ return ready_ || waiting_; }

    inline bool join() {
	if (ready_) {
	    (void) pop_ready();
	    return true;
	} else
	    return false;
    }
    void clear();

    using tamerpriv::blocking_rendezvous::block;
};


class gather_rendezvous : public tamerpriv::blocking_rendezvous,
			  public zero_argument_rendezvous_tag<gather_rendezvous> {
  public:
    inline gather_rendezvous()
        : blocking_rendezvous(rnormal, tamerpriv::rgather) {
    }
    inline ~gather_rendezvous() {
	if (waiting_)
	    clear();
    }

    template <typename T0, typename T1, typename T2, typename T3>
    inline event<T0, T1, T2, T3> make_event(T0& x0, T1& x1, T2& x2, T3& x3);
    template <typename T0, typename T1, typename T2>
    inline event<T0, T1, T2> make_event(T0& x0, T1& x1, T2& x2);
    template <typename T0, typename T1>
    inline event<T0, T1> make_event(T0& x0, T1& x1);
#if !TAMER_HAVE_PREEVENT || !TAMER_PREFER_PREEVENT
    template <typename T0>
    inline event<T0> make_event(T0& x0);
    inline event<> make_event();
#else
    template <typename T0>
    inline preevent<gather_rendezvous, T0> make_event(T0& x0);
    inline preevent<gather_rendezvous> make_event();
#endif
#if TAMER_HAVE_PREEVENT
    template <typename T0>
    inline preevent<gather_rendezvous, T0> make_preevent(T0& x0);
    inline preevent<gather_rendezvous> make_preevent();
#endif

    inline bool has_waiting() const {
	return waiting_;
    }

    inline bool join() {
        return !waiting_;
    }
    void clear();

    using tamerpriv::blocking_rendezvous::block;

  private:
    friend class tamerpriv::abstract_rendezvous;
};

/** @endcond */

/** @cond never */

namespace tamerpriv {
template <typename T0 = void, typename T1 = void, typename T2 = void, typename T3 = void>
class distribute_rendezvous : public functional_rendezvous,
			      public zero_argument_rendezvous_tag<distribute_rendezvous<T0, T1, T2, T3> > {
  public:
    typedef tamer::event<T0, T1, T2, T3> event_type;
    inline distribute_rendezvous();
    inline ~distribute_rendezvous();
    static inline void make(event_type& a, event_type b);
    inline event<T0, T1, T2, T3> make_event();
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
    void add(event_type e);
    static void hard_make(event_type& a, event_type b);
    bool grow();
};

template <typename T0, typename T1, typename T2, typename T3>
inline distribute_rendezvous<T0, T1, T2, T3>::distribute_rendezvous()
    : functional_rendezvous(tamerpriv::rdistribute, hook),
      nes_(0), outstanding_(0), es_(reinterpret_cast<event_type*>(local_es_)) {
}

template <typename T0, typename T1, typename T2, typename T3>
inline distribute_rendezvous<T0, T1, T2, T3>::~distribute_rendezvous() {
    for (int i = 0; i != nes_; ++i)
        es_[i].~event();
    if (es_ != reinterpret_cast<event_type*>(local_es_))
        delete[] reinterpret_cast<char*>(es_);
}

template <typename T0, typename T1, typename T2, typename T3>
void distribute_rendezvous<T0, T1, T2, T3>::add(event_type e) {
    if (nes_ >= nlocal && (nes_ & (nes_ - 1)) == 0 && !grow())
        return;
    new((void*) &es_[nes_]) event_type(TAMER_MOVE(e));
    if (es_[nes_].__get_simple()->shared()) {
        tamerpriv::simple_event::at_trigger(es_[nes_].__get_simple(), clear_hook, this);
        ++outstanding_;
    }
    ++nes_;
}

template <typename T0, typename T1, typename T2, typename T3>
inline void distribute_rendezvous<T0, T1, T2, T3>::make(event_type& a, event_type b) {
    if (!a || !b) {
        if (!a && b)
            a = TAMER_MOVE(b);
    } else
        hard_make(a, TAMER_MOVE(b));
}

template <typename T0, typename T1, typename T2, typename T3>
void distribute_rendezvous<T0, T1, T2, T3>::hard_make(event_type& a, event_type b) {
    simple_event* se = a.__get_simple();
    if (se == b.__get_simple())
        return;
    distribute_rendezvous<T0, T1, T2, T3>* r;
    if (!se->shared() && !se->has_at_trigger()
        && se->rendezvous()->rtype() == tamerpriv::rdistribute)
        r = static_cast<distribute_rendezvous<T0, T1, T2, T3>*>(se->rendezvous());
    else {
        r = new distribute_rendezvous<T0, T1, T2, T3>;
        r->add(TAMER_MOVE(a));
        a = r->make_event();
    }
    r->add(TAMER_MOVE(b));
}

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
        for (int i = 0; i != dr->nes_; ++i)
            if (dr->es_[i])
                return;
	dr->remove_waiting();
	delete dr;
    }
}
} // namespace tamerpriv

/** @endcond never */

} // namespace tamer
#endif /* TAMER__RENDEZVOUS_HH */
