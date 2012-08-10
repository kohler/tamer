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
class tamer_debug_closure;
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

    inline void initialize(abstract_rendezvous *r, uintptr_t rid);

    void simple_trigger(bool values);
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
	: _events(0), _blocked_closure(0), _flags(flags) {
    }

    virtual inline ~abstract_rendezvous();

    virtual void do_complete(simple_event *e, bool values) = 0;

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

    inline void block(tamer_closure &c, unsigned position,
		      const char *file, int line);
    inline void block(tamer_debug_closure &c, unsigned position,
		      const char *file, int line);
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

    static inline bool has_unblocked() {
	return unblocked;
    }
    static inline abstract_rendezvous *pop_unblocked() {
	abstract_rendezvous *r = unblocked;
	if (r) {
	    if (!(unblocked = r->unblocked_next_))
		unblocked_ptail = &unblocked;
	}
	return r;
    }

  private:

    simple_event *_events;
    tamer_closure *_blocked_closure;
    rendezvous_flags _flags;
    abstract_rendezvous *unblocked_next_;

    static abstract_rendezvous *unblocked;
    static abstract_rendezvous **unblocked_ptail;
    static inline abstract_rendezvous *unblocked_sentinel() {
	return reinterpret_cast<abstract_rendezvous *>(uintptr_t(1));
    }
    void hard_free();

    abstract_rendezvous(const abstract_rendezvous &);
    abstract_rendezvous &operator=(const abstract_rendezvous &);

    friend class simple_event;
    friend class driver;

};


typedef void (*tamer_closure_activator)(tamer_closure *);

struct tamer_closure {
    tamer_closure(tamer_closure_activator activator)
	: tamer_activator_(activator), tamer_block_position_(0) {
    }
    tamer_closure_activator tamer_activator_;
    unsigned tamer_block_position_;
};

template <typename T>
struct closure_reference {
    closure_reference(T &c)
	: c_(&c) {
    }
    ~closure_reference() {
	delete c_;
    }
    void clear() {
	c_ = 0;
    }
    T *c_;
};

struct tamer_debug_closure : public tamer_closure {
    tamer_debug_closure(tamer_closure_activator activator)
	: tamer_closure(activator) {
    }
    const char *tamer_blocked_file_;
    int tamer_blocked_line_;
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
    if (_blocked_closure)
	hard_free();
}

inline void simple_event::initialize(abstract_rendezvous *r, uintptr_t rid)
{
#if TAMER_DEBUG
    assert(_r == 0 && r != 0);
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

inline void abstract_rendezvous::block(tamer_closure &c,
				       unsigned position,
				       const char *, int) {
    assert(!_blocked_closure && &c);
    _blocked_closure = &c;
    unblocked_next_ = unblocked_sentinel();
    c.tamer_block_position_ = position;
}

inline void abstract_rendezvous::block(tamer_debug_closure &c,
				       unsigned position,
				       const char *file, int line) {
    block(static_cast<tamer_closure &>(c), -position, file, line);
    c.tamer_blocked_file_ = file;
    c.tamer_blocked_line_ = line;
}

inline void abstract_rendezvous::unblock() {
    if (_blocked_closure && unblocked_next_ == unblocked_sentinel()) {
	*unblocked_ptail = this;
	unblocked_next_ = 0;
	unblocked_ptail = &unblocked_next_;
    }
}

inline void abstract_rendezvous::run() {
    tamer_closure *c = _blocked_closure;
    _blocked_closure = 0;
    c->tamer_activator_(c);
}

}}
#endif /* TAMER_BASE_HH */
