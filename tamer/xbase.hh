#ifndef TAMER_XBASE_HH
#define TAMER_XBASE_HH 1
/* Copyright (c) 2007-2012, Eddie Kohler
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
#include <stdexcept>
#include <stdint.h>
#include <tamer/autoconf.h>
namespace tamer {

template <typename I0=void, typename I1=void> class rendezvous;
template <typename T0=void, typename T1=void, typename T2=void, typename T3=void> class event;
inline event<> distribute(const event<> &, const event<> &);
inline event<> distribute(const event<> &, const event<> &, const event<> &);
template <typename T0> event<T0> unbind(const event<> &);
class driver;
class gather_rendezvous;

class tamer_error : public std::runtime_error { public:
    explicit tamer_error(const std::string &arg)
	: runtime_error(arg) {
    }
};

struct no_slot {
};

enum rendezvous_flags {
    rnormal,
    rvolatile
};

namespace tamerpriv {

class simple_event;
class abstract_rendezvous;
class tamer_closure;
class distribute_rendezvous;
event<> hard_distribute(const event<> &e1, const event<> &e2);

class simple_event { public:

    // DO NOT derive from this class!

    inline simple_event()
	: _refcount(1), _r(0) {
    }

    template <typename R, typename I0, typename I1>
    inline simple_event(R &r, const I0 &i0, const I1 &i1);

    template <typename R, typename I0>
    inline simple_event(R &r, const I0 &i0);

    template <typename R>
    inline simple_event(R &r);

#if TAMER_DEBUG
    inline ~simple_event() {
	assert(!_r);
    }
#endif

    static inline void use(simple_event *e) {
	if (e)
	    ++e->_refcount;
    }

    static inline void unuse(simple_event *e) {
	if (e && --e->_refcount == 0) {
	    if (e->_r)
		e->trigger_for_unuse();
	    delete e;
	}
    }
    static inline void unuse_clean(simple_event *e) {
	if (e && --e->_refcount == 0)
	    delete e;
    }

    unsigned refcount() const {
	return _refcount;
    }

    typedef unsigned (simple_event::*unspecified_bool_type)() const;

    operator unspecified_bool_type() const {
	return _r ? &simple_event::refcount : 0;
    }

    bool empty() const {
	return _r == 0;
    }

    inline abstract_rendezvous *rendezvous() const;
    inline uintptr_t rid() const;
    inline void precomplete();
    inline void postcomplete(bool nofree = false);

    inline void initialize(abstract_rendezvous *r, uintptr_t rid);

    inline void simple_trigger(bool values);
    void trigger_all_for_remove();

    static inline void at_trigger(simple_event *x, const event<> &at_e);

    inline bool has_at_trigger() const {
	return _r && _at_trigger && *_at_trigger;
    }

  protected:

    unsigned _refcount;
    abstract_rendezvous *_r;
    uintptr_t _rid;
    simple_event *_r_next;
    simple_event **_r_pprev;
    simple_event *_at_trigger;

    simple_event(const simple_event &);
    simple_event &operator=(const simple_event &);

    void trigger_for_unuse();

};


class abstract_rendezvous { public:

    abstract_rendezvous(rendezvous_flags flags = rnormal)
	: _events(0), _unblocked_next(0), _unblocked_prev(0),
	  _blocked_closure(0), _flags(flags) {
    }

    virtual inline ~abstract_rendezvous();

    virtual void complete(simple_event *e, bool values) = 0;

    virtual inline void clear();

    virtual bool is_distribute() const {
	return false;
    }

    bool is_volatile() const {
	return _flags == rvolatile;
    }

    void set_volatile(bool v) {
	_flags = (v ? rvolatile : rnormal);
    }

    virtual tamer_closure *linked_closure() const {
	return _blocked_closure;
    }

    tamer_closure *blocked_closure() const {
	return _blocked_closure;
    }

    inline void block(tamer_closure &c, unsigned where);
    inline void unblock();
    inline void run();
    inline void remove_all();

    class clearer { public:
	clearer(abstract_rendezvous &r)
	    : _r(&r) {
	}
	~clearer() {
	    if (_r)
		_r->clear();
	}
	void kill() {
	    _r = 0;
	}
      private:
	abstract_rendezvous *_r;
    };

    static abstract_rendezvous *unblocked;
    static abstract_rendezvous *unblocked_tail;

  private:

    simple_event *_events;
    abstract_rendezvous *_unblocked_next;
    abstract_rendezvous *_unblocked_prev;
    tamer_closure *_blocked_closure;
    unsigned _blocked_closure_blockid;
    rendezvous_flags _flags;

    abstract_rendezvous(const abstract_rendezvous &);
    abstract_rendezvous &operator=(const abstract_rendezvous &);

    friend class simple_event;
    friend class driver;

};


struct tamer_closure {

    tamer_closure()
	: _refcount(1) {
    }

    virtual ~tamer_closure() {
    }

    void use() {
	_refcount++;
    }

    void unuse() {
	if (--_refcount == 0)
	    delete this;
    }

    virtual void tamer_closure_activate(unsigned) = 0;

    virtual bool block_landmark(const char *&file, unsigned &line);

  protected:

    unsigned _refcount;

};


struct tamer_debug_closure : public tamer_closure {

    tamer_debug_closure()
	: _block_file(0), _block_line(0) {
    }

    bool block_landmark(const char *&file, unsigned &line);

    void set_block_landmark(const char *file, unsigned line) {
	_block_file = file;
	_block_line = line;
    }

  protected:

    const char *_block_file;
    unsigned _block_line;

};


namespace message {
void event_prematurely_dereferenced(simple_event *e, abstract_rendezvous *r);
}


inline void abstract_rendezvous::remove_all() {
    if (_events) {
	_events->trigger_all_for_remove();
	_events = 0;
    }
}

inline void abstract_rendezvous::clear() {
    remove_all();
}

inline abstract_rendezvous::~abstract_rendezvous() {
    // take all events off this rendezvous and call their triggerers
    remove_all();

    if (_unblocked_prev)
	_unblocked_prev->_unblocked_next = _unblocked_next;
    else if (unblocked == this)
	unblocked = _unblocked_next;
    if (_unblocked_next)
	_unblocked_next->_unblocked_prev = _unblocked_prev;
    else if (unblocked_tail == this)
	unblocked_tail = _unblocked_prev;
    if (_blocked_closure)
	_blocked_closure->unuse();
}

inline void simple_event::initialize(abstract_rendezvous *r, uintptr_t rid)
{
#if TAMER_DEBUG
    assert(_r != 0);
#endif
    // NB this can be called before e has been fully initialized.
    _r = r;
    _rid = rid;
    _r_pprev = &r->_events;
    if (r->_events)
	r->_events->_r_pprev = &_r_next;
    _r_next = r->_events;
    _at_trigger = 0;
    r->_events = this;
}

inline abstract_rendezvous *simple_event::rendezvous() const {
    return _r;
}

inline uintptr_t simple_event::rid() const {
    return _rid;
}

inline void simple_event::precomplete() {
    *_r_pprev = _r_next;
    if (_r_next)
	_r_next->_r_pprev = _r_pprev;
}

inline void simple_event::postcomplete(bool nofree) {
#if TAMER_DEBUG
    assert(!_r);
#endif
    if (_at_trigger) {
	if (abstract_rendezvous *r = _at_trigger->_r) {
	    _at_trigger->_r = 0;
	    r->complete(_at_trigger, false);
	} else
	    unuse_clean(_at_trigger);
    }
    if (!nofree)
	unuse_clean(this);
}

inline void simple_event::simple_trigger(bool values) {
#if TAMER_DEBUG
    assert(_r != 0 && _refcount != 0);
#endif
    // See also trigger_all_for_remove(), trigger_for_unuse().
    abstract_rendezvous *r = _r;
    _r = 0;
    r->complete(this, values);
}

inline void abstract_rendezvous::block(tamer_closure &c, unsigned where) {
    assert(!_blocked_closure && &c);
    c.use();
    _blocked_closure = &c;
    _blocked_closure_blockid = where;
}

inline void abstract_rendezvous::unblock() {
    if (_blocked_closure && !_unblocked_prev && unblocked != this) {
	_unblocked_next = 0;
	_unblocked_prev = unblocked_tail;
	if (unblocked_tail)
	    unblocked_tail->_unblocked_next = this;
	else
	    unblocked = this;
	unblocked_tail = this;
    }
}

inline void abstract_rendezvous::run() {
    tamer_closure *c = _blocked_closure;
    _blocked_closure = 0;
    if (_unblocked_prev)
	_unblocked_prev->_unblocked_next = _unblocked_next;
    else if (unblocked == this)
	unblocked = _unblocked_next;
    if (_unblocked_next)
	_unblocked_next->_unblocked_prev = _unblocked_prev;
    else if (unblocked_tail == this)
	unblocked_tail = _unblocked_next;
    _unblocked_next = _unblocked_prev = 0;
    c->tamer_closure_activate(_blocked_closure_blockid);
    c->unuse();
}

}}
#endif /* TAMER_BASE_HH */
