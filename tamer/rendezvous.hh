#ifndef TAMER_RENDEZVOUS_HH
#define TAMER_RENDEZVOUS_HH 1
#include <tamer/util.hh>
#include <tamer/xbase.hh>
namespace tamer {

/** @file <tamer/rendezvous.hh>
 *  @brief  The rendezvous template classes.
 */

/** @class rendezvous tamer/event.hh <tamer/tamer.hh>
 *  @brief  A set of watched events.
 *
 *  Rendezvous may also be declared with one or zero template arguments, as in
 *  <tt>rendezvous<T0></tt> or <tt>rendezvous<></tt>.  Each specialized
 *  rendezvous class has functions similar to the full-featured rendezvous,
 *  but with parameters to @c join appropriate to the template arguments.
 *  Specialized rendezvous implementations are often more efficient than the
 *  full @c rendezvous.
 */
template <typename I0, typename I1>
class rendezvous : public tamerpriv::abstract_rendezvous { public:

    /** @brief  Default constructor creates a fresh rendezvous. */
    inline rendezvous();

    /** @brief  Report how many events are ready.
     *  @return  The number of ready events.
     *
     *  An event is ready if it has triggered, but no @a join or @c twait
     *  has reported it yet.
     */
    unsigned nready() const {
	return _bs.size();
    }

    /** @brief  Report how many events are waiting.
     *  @return  The number of waiting events.
     *
     *  An event is waiting until it is either triggered or canceled.
     */
    unsigned nwaiting() const {
	return _bs.multiset_size();
    }

    /** @brief  Report how many events are ready or waiting.
     *  @return  The number of ready or waiting events.
     */
    unsigned nevents() const {
	return nready() + nwaiting();
    }

    /** @brief  Report the next ready event.
     *  @param[out]  i0  Set to the first event ID of the ready event.
     *  @param[out]  i1  Set to the second event ID of the ready event.
     *  @return  True if there was a ready event, false otherwise.
     *
     *  @a i0 and @a i1 were modified if and only if @a join returns true.
     */
    inline bool join(I0 &i0, I1 &i1);

    /** @brief  Remove all events from this rendezvous.
     *
     *  Every event waiting on this rendezvous is made empty.  After clear(),
     *  nevents() will return 0.
     */
    inline void clear();

    /** @internal
     *  @brief  Add an occurrence to this rendezvous.
     *  @param  e   The occurrence.
     *  @param  i0  The occurrence's first event ID.
     *  @param  i1  The occurrence's first event ID.
     */
    inline void add(tamerpriv::simple_event *e, const I0 &i0, const I1 &i1);

    /** @internal
     *  @brief  Mark the triggering of an occurrence.
     *  @param  rid  The occurrence's ID within this rendezvous.
     */
    inline void complete(uintptr_t rid);
    
  private:

    struct evtrec {
	I0 i0;
	I1 i1;
	evtrec(const I0 &i0_, const I1 &i1_)
	    : i0(i0_), i1(i1_) {
	}
    };

    tamerutil::ready_set<evtrec> _bs;
    
};

template <typename I0, typename I1>
rendezvous<I0, I1>::rendezvous()
{
}

template <typename I0, typename I1>
void rendezvous<I0, I1>::add(tamerpriv::simple_event *e, const I0 &i0, const I1 &i1)
{
    e->initialize(this, _bs.insert(i0, i1));
}

template <typename I0, typename I1>
void rendezvous<I0, I1>::complete(uintptr_t rid)
{
    _bs.push_back_element(rid);
    unblock();
}

template <typename I0, typename I1>
bool rendezvous<I0, I1>::join(I0 &i0, I1 &i1)
{
    if (evtrec *e = _bs.front_ptr()) {
	i0 = e->i0;
	i1 = e->i1;
	_bs.pop_front();
	return true;
    } else
	return false;
}

template <typename I0, typename I1>
inline void rendezvous<I0, I1>::clear()
{
    abstract_rendezvous::clear();
    _bs.clear();
}


/** @cond specialized_rendezvous */

template <typename I0>
class rendezvous<I0, void> : public tamerpriv::abstract_rendezvous { public:

    inline rendezvous();
    
    inline bool join(I0 &);
    inline void clear();

    unsigned nready() const	{ return _bs.size(); }
    unsigned nwaiting() const	{ return _bs.multiset_size(); }
    unsigned nevents() const	{ return nready() + nwaiting(); }
    
    inline void add(tamerpriv::simple_event *e, const I0 &i0);
    inline void complete(uintptr_t rid);

  private:

    tamerutil::ready_set<I0> _bs;
    
};

template <typename I0>
rendezvous<I0, void>::rendezvous()
{
}

template <typename I0>
void rendezvous<I0, void>::add(tamerpriv::simple_event *e, const I0 &i0)
{
    e->initialize(this, _bs.insert(i0));
}

template <typename I0>
void rendezvous<I0, void>::complete(uintptr_t rid)
{
    _bs.push_back_element(rid);
    unblock();
}

template <typename I0>
bool rendezvous<I0, void>::join(I0 &i0)
{
    if (I0 *e = _bs.front_ptr()) {
	i0 = *e;
	_bs.pop_front();
	return true;
    } else
	return false;
}

template <typename I0>
void rendezvous<I0, void>::clear()
{
    abstract_rendezvous::clear();
    _bs.clear();
}


template <>
class rendezvous<uintptr_t> : public tamerpriv::abstract_rendezvous { public:

    inline rendezvous();

    inline bool join(uintptr_t &);
    inline void clear();

    unsigned nready() const	{ return _buf.size(); }
    unsigned nwaiting() const	{ return _nwaiting; }
    unsigned nevents() const	{ return nready() + nwaiting(); }

    inline void add(tamerpriv::simple_event *e, uintptr_t i0) throw ();
    inline void complete(uintptr_t rid);

  protected:

    tamerutil::debuffer<uintptr_t> _buf;
    size_t _nwaiting;
    
};

inline rendezvous<uintptr_t>::rendezvous()
    : _nwaiting(0)
{
}

inline void rendezvous<uintptr_t>::add(tamerpriv::simple_event *e, uintptr_t i0) throw ()
{
    _nwaiting++;
    e->initialize(this, i0);
}

inline void rendezvous<uintptr_t>::complete(uintptr_t rid)
{
    _nwaiting--;
    _buf.push_back(rid);
    unblock();
}

inline bool rendezvous<uintptr_t>::join(uintptr_t &i0)
{
    if (uintptr_t *x = _buf.front_ptr()) {
	i0 = *x;
	_buf.pop_front();
	return true;
    } else
	return false;
}

inline void rendezvous<uintptr_t>::clear()
{
    abstract_rendezvous::clear();
    _buf.clear();
    _nwaiting = 0;
}


template <typename T>
class rendezvous<T *> : public rendezvous<uintptr_t> { public:

    typedef rendezvous<uintptr_t> inherited;

    rendezvous() { }

    inline void add(tamerpriv::simple_event *e, T *i0) throw () {
	inherited::add(e, reinterpret_cast<uintptr_t>(i0));
    }
    
    inline bool join(T *&i0) {
	if (uintptr_t *x = _buf.front_ptr()) {
	    i0 = reinterpret_cast<T *>(*x);
	    _buf.pop_front();
	    return true;
	} else
	    return false;
    }
        
};


template <>
class rendezvous<int> : public rendezvous<uintptr_t> { public:

    typedef rendezvous<uintptr_t> inherited;

    rendezvous() { }

    inline void add(tamerpriv::simple_event *e, int i0) throw () {
	inherited::add(e, static_cast<uintptr_t>(i0));
    }
    
    inline bool join(int &i0) {
	if (uintptr_t *x = _buf.front_ptr()) {
	    i0 = static_cast<int>(*x);
	    _buf.pop_front();
	    return true;
	} else
	    return false;
    }

};


template <>
class rendezvous<bool> : public rendezvous<uintptr_t> { public:

    typedef rendezvous<uintptr_t> inherited;

    rendezvous() { }

    inline void add(tamerpriv::simple_event *e, bool i0) throw () {
	inherited::add(e, static_cast<uintptr_t>(i0));
    }
    
    inline bool join(bool &i0) {
	if (uintptr_t *x = _buf.front_ptr()) {
	    i0 = static_cast<bool>(*x);
	    _buf.pop_front();
	    return true;
	} else
	    return false;
    }

};


template <>
class rendezvous<void> : public tamerpriv::abstract_rendezvous { public:

    rendezvous() : _nwaiting(0), _nready(0) { }

    inline void add(tamerpriv::simple_event *e) throw () {
	_nwaiting++;
	e->initialize(this, 1);
    }
    
    inline void complete(uintptr_t) {
	_nwaiting--;
	_nready++;
	unblock();
    }
    
    inline bool join() {
	if (_nready) {
	    _nready--;
	    return true;
	} else
	    return false;
    }

    inline void clear() {
	abstract_rendezvous::clear();
	_nwaiting = _nready = 0;
    }

    unsigned nready() const	{ return _nready; }
    unsigned nwaiting() const	{ return _nwaiting; }
    unsigned nevents() const	{ return nready() + nwaiting(); }

  protected:

    unsigned _nwaiting;
    unsigned _nready;
    
};


class gather_rendezvous : public rendezvous<> { public:

    gather_rendezvous(tamerpriv::tamer_closure *c)
	: _linked_closure(c) {
    }

    tamerpriv::tamer_closure *linked_closure() const {
	return _linked_closure;
    }

    inline void complete(uintptr_t) {
	_nwaiting--;
	_nready++;
	if (_nwaiting == 0)
	    unblock();
    }

  private:

    tamerpriv::tamer_closure *_linked_closure;
    
};

/** @endcond */

}
#endif /* TAMER__RENDEZVOUS_HH */
