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
class explicit_rendezvous;
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
class explicit_rendezvous;
struct tamer_closure;
struct tamer_debug_closure;
class distribute_rendezvous;
event<> hard_distribute(const event<> &e1, const event<> &e2);

class simple_event { public:

    // DO NOT derive from this class!

    inline simple_event() TAMER_NOEXCEPT
	: _refcount(1), _r(0) {
    }

    template <typename R, typename I0, typename I1>
    inline simple_event(R &r, const I0 &i0, const I1 &i1) TAMER_NOEXCEPT;

    template <typename R, typename I0>
    inline simple_event(R &r, const I0 &i0) TAMER_NOEXCEPT;

    template <typename R>
    inline simple_event(R &r) TAMER_NOEXCEPT;

#if TAMER_DEBUG
    inline ~simple_event() TAMER_NOEXCEPT {
	assert(!_r);
    }
#endif

    static inline void use(simple_event *e) TAMER_NOEXCEPT {
	if (e)
	    ++e->_refcount;
    }

    static inline void unuse(simple_event *e) TAMER_NOEXCEPT {
	if (e && --e->_refcount == 0) {
	    if (e->_r)
		e->trigger_for_unuse();
	    delete e;
	}
    }
    static inline void unuse_clean(simple_event *e) TAMER_NOEXCEPT {
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
    inline simple_event *next() const;

    inline void initialize(abstract_rendezvous *r, uintptr_t rid);

    inline void simple_trigger(bool values);
    static void simple_trigger(simple_event *x, bool values) TAMER_NOEXCEPT;
    void trigger_list_for_remove() TAMER_NOEXCEPT;

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

    void trigger_for_unuse() TAMER_NOEXCEPT;

    friend class explicit_rendezvous;

};


enum rendezvous_type {
    rgather,
    rexplicit,
    rfunctional,
    rdistribute
};

class abstract_rendezvous {
  public:
    abstract_rendezvous(rendezvous_flags flags, rendezvous_type rtype)
	: waiting_(0), _blocked_closure(0),
	  rtype_(rtype), is_volatile_(flags == rvolatile) {
    }
    inline ~abstract_rendezvous() TAMER_NOEXCEPT;

    inline rendezvous_type rtype() const {
	return rendezvous_type(rtype_);
    }

    inline bool is_volatile() const {
	return is_volatile_;
    }
    inline void set_volatile(bool v) {
	is_volatile_ = v;
    }

    tamer_closure *linked_closure() const;
    inline tamer_closure *blocked_closure() const {
	return _blocked_closure;
    }

    inline void block(tamer_closure &c, unsigned position,
		      const char *file, int line);
    inline void block(tamer_debug_closure &c, unsigned position,
		      const char *file, int line);
    inline void unblock();
    inline void run();

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

  protected:
    simple_event *waiting_;
    tamer_closure *_blocked_closure;
    uint8_t rtype_;
    bool is_volatile_;
    abstract_rendezvous *unblocked_next_;

    static abstract_rendezvous *unblocked;
    static abstract_rendezvous **unblocked_ptail;
    static inline abstract_rendezvous *unblocked_sentinel() {
	return reinterpret_cast<abstract_rendezvous *>(uintptr_t(1));
    }

    inline void remove_waiting() TAMER_NOEXCEPT;

  private:
    abstract_rendezvous(const abstract_rendezvous &);
    abstract_rendezvous &operator=(const abstract_rendezvous &);
    void hard_free();

    friend class simple_event;
    friend class driver;
};

class explicit_rendezvous : public abstract_rendezvous {
  public:
    inline explicit_rendezvous(rendezvous_flags flags)
	: abstract_rendezvous(flags, rexplicit),
	  ready_(), ready_ptail_(&ready_) {
    }
#if TAMER_DEBUG
    inline ~explicit_rendezvous() {
	assert(!ready_);
    }
#endif

  protected:
    simple_event *ready_;
    simple_event **ready_ptail_;

    inline uintptr_t pop_ready() {
	simple_event *e = ready_;
	if (!(ready_ = e->_r_next))
	    ready_ptail_ = &ready_;
	uintptr_t x = e->rid();
	simple_event::unuse_clean(e);
	return x;
    }
    inline void remove_ready() {
	while (simple_event *e = ready_) {
	    ready_ = e->_r_next;
	    simple_event::unuse_clean(e);
	}
	ready_ptail_ = &ready_;
    }

    friend class simple_event;
};

class functional_rendezvous : public abstract_rendezvous {
  public:
    typedef void (*hook_type)(functional_rendezvous *fr,
			      simple_event *e, bool values);

    inline functional_rendezvous(void (*f)(functional_rendezvous *fr,
					   simple_event *e, bool values) TAMER_NOEXCEPT)
	: abstract_rendezvous(rnormal, rfunctional), f_(f) {
    }
    inline ~functional_rendezvous() {
	remove_waiting();
    }

  private:
    void (*f_)(functional_rendezvous *r,
	       simple_event *e, bool values) TAMER_NOEXCEPT;

    friend class simple_event;
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
class closure_owner {
  public:
    inline closure_owner(T &c)
	: c_(&c) {
    }
    inline ~closure_owner() {
	delete c_;
    }
    inline void reset() {
	c_ = 0;
    }
  private:
    T *c_;
};

template <typename R>
class rendezvous_owner {
  public:
    inline rendezvous_owner(R &r)
	: r_(&r) {
    }
    inline ~rendezvous_owner() {
	if (r_)
	    r_->clear();
    }
    inline void reset() {
	r_ = 0;
    }
  private:
    R *r_;
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


inline void abstract_rendezvous::remove_waiting() TAMER_NOEXCEPT {
    if (waiting_) {
	waiting_->trigger_list_for_remove();
	waiting_ = 0;
    }
}

inline abstract_rendezvous::~abstract_rendezvous() TAMER_NOEXCEPT {
    // take all events off this rendezvous and call their triggerers
#if TAMER_DEBUG
    assert(!waiting_);
#endif
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
    _r_pprev = &r->waiting_;
    if (r->waiting_)
	r->waiting_->_r_pprev = &_r_next;
    _r_next = r->waiting_;
    _at_trigger = 0;
    r->waiting_ = this;
}

inline abstract_rendezvous *simple_event::rendezvous() const {
    return _r;
}

inline uintptr_t simple_event::rid() const {
    return _rid;
}

inline simple_event *simple_event::next() const {
    return _r_next;
}

inline void simple_event::simple_trigger(bool values) {
    simple_trigger(this, values);
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
