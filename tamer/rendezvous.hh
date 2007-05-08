#ifndef TAMER_RENDEZVOUS_HH
#define TAMER_RENDEZVOUS_HH 1
#include <tamer/util.hh>
#include <tamer/base.hh>
namespace tamer {

/** @file <tamer/rendezvous.hh>
 *  @brief  The rendezvous template classes.
 */

/** @class rendezvous tamer/event.hh <tamer.hh>
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

    /** @brief  Report the next ready event.
     *  @param[out]  i0  Set to the first event ID of the ready event.
     *  @param[out]  i1  Set to the second event ID of the ready event.
     *  @return  True if there was a ready event, false otherwise.
     *
     *  @a i0 and @a i1 were modified if and only if @a join returns true.
     */
    inline bool join(I0 &i0, I1 &i1);

    /** @brief  Report how many events are ready.
     *  @return  The number of ready events.
     *
     *  An event is ready if it has triggered, but no @a join or @c twait
     *  has reported it yet.
     */
    unsigned nready() const {
	return _bs.nactive();
    }

    /** @brief  Report how many events are waiting.
     *  @return  The number of waiting events.
     *
     *  An event is waiting until it is either triggered or canceled.
     */
    unsigned nwaiting() const {
	return _bs.npassive();
    }

    /** @brief  Report how many events are ready or waiting.
     *  @return  The number of ready or waiting events.
     */
    unsigned nevents() const {
	return nready() + nwaiting();
    }

    /** @internal
     *  @brief  Add an occurrence to this rendezvous.
     *  @param  e   The occurrence.
     *  @param  i0  The occurrence's first event ID.
     *  @param  i1  The occurrence's first event ID.
     */
    inline void add(tamerpriv::simple_event *e, const I0 &i0, const I1 &i1);

    /** @internal
     *  @brief  Mark the triggering or cancellation of an occurrence.
     *  @param  rid  The occurrence's ID within this rendezvous.
     *  @param  success  True if the occurrence was triggered, false otherwise.
     */
    inline void complete(uintptr_t rid, bool success);
    
  private:

    struct evtrec {
	size_t next;
	I0 i0;
	I1 i1;
    };

    tamerpriv::blockset<evtrec> _bs;
    
};

template <typename I0, typename I1>
rendezvous<I0, I1>::rendezvous()
{
}

template <typename I0, typename I1>
void rendezvous<I0, I1>::add(tamerpriv::simple_event *e, const I0 &i0, const I1 &i1)
{
    uintptr_t u;
    evtrec &ew = _bs.allocate_passive(u);
    (void) new(static_cast<void *>(&ew.i0)) I0(i0);
    (void) new(static_cast<void *>(&ew.i1)) I1(i1);
    e->initialize(this, u);
}

template <typename I0, typename I1>
void rendezvous<I0, I1>::complete(uintptr_t rid, bool success)
{
    if (success) {
	_bs.activate(rid);
	unblock();
    } else {
	_bs.kill(rid);
	if (blocked_closure() && nwaiting() == 0)
	    tamerpriv::message::rendezvous_dead(this);
    }
}

template <typename I0, typename I1>
bool rendezvous<I0, I1>::join(I0 &i0, I1 &i1)
{
    if (evtrec *e = _bs.active_front()) {
	i0 = e->i0;
	i1 = e->i1;
	_bs.pop_active();
	return true;
    } else
	return false;
}


/** @cond specialized_rendezvous */

template <typename I0>
class rendezvous<I0, void> : public tamerpriv::abstract_rendezvous { public:

    inline rendezvous();
    
    inline void add(tamerpriv::simple_event *e, const I0 &i0);
    inline void complete(uintptr_t rid, bool success);
    inline bool join(I0 &);

    unsigned nready() const	{ return _bs.nactive(); }
    unsigned nwaiting() const	{ return _bs.npassive(); }
    unsigned nevents() const	{ return nready() + nwaiting(); }

  private:

    struct evtrec {
	size_t next;
	I0 i0;
    };

    tamerpriv::blockset<evtrec> _bs;
    
};

template <typename I0>
rendezvous<I0, void>::rendezvous()
{
}

template <typename I0>
void rendezvous<I0, void>::add(tamerpriv::simple_event *e, const I0 &i0)
{
    uintptr_t u;
    evtrec &erec = _bs.allocate_passive(u);
    (void) new(static_cast<void *>(&erec.i0)) I0(i0);
    e->initialize(this, u);
}

template <typename I0>
void rendezvous<I0, void>::complete(uintptr_t rid, bool success)
{
    if (success) {
	_bs.activate(rid);
	unblock();
    } else {
	_bs.cancel(rid);
	if (blocked_closure() && nwaiting() == 0)
	    tamerpriv::message::rendezvous_dead(this);
    }
}

template <typename I0>
bool rendezvous<I0, void>::join(I0 &i0)
{
    if (evtrec *e = _bs.active_front()) {
	i0 = e->i0;
	_bs.pop_active();
	return true;
    } else
	return false;
}


template <>
class rendezvous<uintptr_t> : public tamerpriv::abstract_rendezvous { public:

    inline rendezvous();

    inline void add(tamerpriv::simple_event *e, uintptr_t i0) throw ();
    inline void complete(uintptr_t rid, bool success);
    inline bool join(uintptr_t &);

    unsigned nready() const	{ return _buf.size(); }
    unsigned nwaiting() const	{ return _nwaiting; }
    unsigned nevents() const	{ return nready() + nwaiting(); }

  protected:

    tamerpriv::debuffer<uintptr_t> _buf;
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

inline void rendezvous<uintptr_t>::complete(uintptr_t rid, bool success)
{
    _nwaiting--;
    if (success) {
	_buf.push_back(rid);
	unblock();
    } else if (blocked_closure() && nwaiting() == 0)
	tamerpriv::message::rendezvous_dead(this);
}

inline bool rendezvous<uintptr_t>::join(uintptr_t &i0)
{
    if (uintptr_t *x = _buf.front()) {
	i0 = *x;
	_buf.pop_front();
	return true;
    } else
	return false;
}



template <typename T>
class rendezvous<T *> : public rendezvous<uintptr_t> { public:

    typedef rendezvous<uintptr_t> inherited;

    rendezvous() { }

    inline void add(tamerpriv::simple_event *e, T *i0) throw () {
	inherited::add(e, reinterpret_cast<uintptr_t>(i0));
    }
    
    inline bool join(T *&i0) {
	if (uintptr_t *x = _buf.front()) {
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
	if (uintptr_t *x = _buf.front()) {
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
	if (uintptr_t *x = _buf.front()) {
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
    
    inline void complete(uintptr_t, bool success) {
	_nwaiting--;
	if (success) {
	    _nready++;
	    unblock();
	} else if (blocked_closure() && nwaiting() == 0)
	    tamerpriv::message::rendezvous_dead(this);
    }
    
    inline bool join() {
	if (_nready) {
	    _nready--;
	    return true;
	} else
	    return false;
    }

    unsigned nready() const	{ return _nready; }
    unsigned nwaiting() const	{ return _nwaiting; }
    unsigned nevents() const	{ return nready() + nwaiting(); }

  protected:

    unsigned _nwaiting;
    unsigned _nready;
    
};


class gather_rendezvous : public rendezvous<> { public:

    gather_rendezvous()
	: _dead(false) {
    }

    inline void complete(uintptr_t, bool success) {
	_nwaiting--;
	if (success) {
	    _nready++;
	    if (_nwaiting == 0 && !_dead)
		unblock();
	} else if (!_dead) {
	    _dead = true;
	    if (blocked_closure())
		tamerpriv::message::gather_rendezvous_dead(this);
	}
    }

  private:

    bool _dead;
    
};

/** @endcond */

}
#endif /* TAMER__RENDEZVOUS_HH */
