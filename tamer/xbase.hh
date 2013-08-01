#ifndef TAMER_XBASE_HH
#define TAMER_XBASE_HH 1
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
#include <stdexcept>
#include <stdint.h>
#include <assert.h>
#include <tamer/autoconf.h>
namespace tamer {

// Improve error messages from overloaded functions.
template <typename R> class two_argument_rendezvous_tag {};
template <typename R> class one_argument_rendezvous_tag {};
template <typename R> class zero_argument_rendezvous_tag {};

template <typename I0 = void, typename I1 = void> class rendezvous;
template <typename T0 = void, typename T1 = void, typename T2 = void,
          typename T3 = void> class event;
#if TAMER_HAVE_PREEVENT
template <typename R, typename T0 = void> class preevent;
#endif
class driver;

class tamer_error : public std::runtime_error { public:
    explicit tamer_error(const std::string &arg)
	: runtime_error(arg) {
    }
};

struct no_result {
};
typedef no_result no_slot TAMER_DEPRECATEDATTR;

enum rendezvous_flags {
    rnormal,
    rvolatile
};

namespace tamerpriv {

class simple_event;
class abstract_rendezvous;
class explicit_rendezvous;
struct tamer_closure;

class simple_event { public:

    // DO NOT derive from this class!

    typedef bool (simple_event::*unspecified_bool_type)() const;

    inline simple_event() TAMER_NOEXCEPT;
    inline simple_event(abstract_rendezvous& r, uintptr_t rid,
                        const char* file, int line) TAMER_NOEXCEPT;
#if TAMER_DEBUG
    inline ~simple_event() TAMER_NOEXCEPT;
#endif

    static inline void use(simple_event *e) TAMER_NOEXCEPT;
    static inline void unuse(simple_event *e) TAMER_NOEXCEPT;
    static inline void unuse_clean(simple_event *e) TAMER_NOEXCEPT;

    inline operator unspecified_bool_type() const;
    inline bool empty() const;
    inline abstract_rendezvous *rendezvous() const;
    inline uintptr_t rid() const;
    inline simple_event *next() const;

    inline const char* file_annotation() const;
    inline int line_annotation() const;

    inline void simple_trigger(bool values);
    static void simple_trigger(simple_event *x, bool values) TAMER_NOEXCEPT;
    void trigger_list_for_remove() TAMER_NOEXCEPT;

    static inline void at_trigger(simple_event* x, simple_event* at_trigger);
    static inline void at_trigger(simple_event* x, void (*f)(void*), void* arg);

  protected:

    abstract_rendezvous *_r;
    uintptr_t _rid;
    simple_event *_r_next;
    simple_event **_r_pprev;
    void (*at_trigger_f_)(void*);
    void* at_trigger_arg_;
    unsigned _refcount;
    int annotate_line_;
    const char* annotate_file_;

    simple_event(const simple_event &);
    simple_event &operator=(const simple_event &);

    void unuse_trigger() TAMER_NOEXCEPT;
    static inline event<> at_trigger_event(void (*f)(void*), void* arg);
    static void trigger_hook(void* arg);
    static void hard_at_trigger(simple_event* x, void (*f)(void*), void* arg);

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
    abstract_rendezvous(rendezvous_flags flags, rendezvous_type rtype) TAMER_NOEXCEPT
	: waiting_(0), rtype_(rtype), is_volatile_(flags == rvolatile) {
    }
#if TAMER_DEBUG
    inline ~abstract_rendezvous() TAMER_NOEXCEPT;
#endif

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

  protected:
    simple_event *waiting_;
    uint8_t rtype_;
    bool is_volatile_;

    static abstract_rendezvous *unblocked;
    static abstract_rendezvous **unblocked_ptail;
    static inline abstract_rendezvous *unblocked_sentinel() {
	return reinterpret_cast<abstract_rendezvous *>(uintptr_t(1));
    }

    inline void remove_waiting() TAMER_NOEXCEPT;

  private:
    abstract_rendezvous(const abstract_rendezvous &);
    abstract_rendezvous &operator=(const abstract_rendezvous &);

    friend class simple_event;
    friend class driver;
};

class blocking_rendezvous : public abstract_rendezvous {
  public:
    inline blocking_rendezvous(rendezvous_flags flags, rendezvous_type rtype) TAMER_NOEXCEPT;
    inline ~blocking_rendezvous() TAMER_NOEXCEPT;

    inline void block(tamer_closure &c, unsigned position,
		      const char *file, int line);
    inline void unblock();
    inline void run();

    static inline bool has_unblocked();
    static inline blocking_rendezvous *pop_unblocked();

  protected:
    tamer_closure *blocked_closure_;
    blocking_rendezvous *unblocked_next_;

    static blocking_rendezvous *unblocked;
    static blocking_rendezvous **unblocked_ptail;
    static inline blocking_rendezvous *unblocked_sentinel() {
	return reinterpret_cast<blocking_rendezvous *>(uintptr_t(1));
    }

    void hard_free();

    friend class abstract_rendezvous;
};

class explicit_rendezvous : public blocking_rendezvous {
  public:
    inline explicit_rendezvous(rendezvous_flags flags) TAMER_NOEXCEPT
	: blocking_rendezvous(flags, rexplicit),
	  ready_(), ready_ptail_(&ready_) {
    }
#if TAMER_DEBUG
    inline ~explicit_rendezvous() {
	TAMER_DEBUG_ASSERT(!ready_);
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
    inline functional_rendezvous(rendezvous_type rtype,
				 void (*f)(functional_rendezvous *fr,
					   simple_event *e, bool values) TAMER_NOEXCEPT)
	: abstract_rendezvous(rnormal, rtype), f_(f) {
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


namespace message {
void event_prematurely_dereferenced(simple_event *e, abstract_rendezvous *r);
}


inline void abstract_rendezvous::remove_waiting() TAMER_NOEXCEPT {
    if (waiting_) {
	waiting_->trigger_list_for_remove();
	waiting_ = 0;
    }
}

#if TAMER_DEBUG
inline abstract_rendezvous::~abstract_rendezvous() TAMER_NOEXCEPT {
    TAMER_DEBUG_ASSERT(!waiting_);
}
#endif


inline blocking_rendezvous::blocking_rendezvous(rendezvous_flags flags,
						rendezvous_type rtype) TAMER_NOEXCEPT
    : abstract_rendezvous(flags, rtype), blocked_closure_(), unblocked_next_() {
}

inline blocking_rendezvous::~blocking_rendezvous() TAMER_NOEXCEPT {
    if (blocked_closure_)
	hard_free();
}

inline void blocking_rendezvous::block(tamer_closure &c,
				       unsigned position,
				       const char *, int) {
    assert(!blocked_closure_ && &c);
    blocked_closure_ = &c;
    unblocked_next_ = unblocked_sentinel();
    c.tamer_block_position_ = position;
}

inline void blocking_rendezvous::unblock() {
    if (blocked_closure_ && unblocked_next_ == unblocked_sentinel()) {
	*unblocked_ptail = this;
	unblocked_next_ = 0;
	unblocked_ptail = &unblocked_next_;
    }
}

inline void blocking_rendezvous::run() {
    tamer_closure *c = blocked_closure_;
    blocked_closure_ = 0;
    c->tamer_activator_(c);
}

inline bool blocking_rendezvous::has_unblocked() {
    return unblocked;
}

inline blocking_rendezvous *blocking_rendezvous::pop_unblocked() {
    blocking_rendezvous *r = unblocked;
    if (r) {
	if (!(unblocked = r->unblocked_next_))
	    unblocked_ptail = &unblocked;
    }
    return r;
}


inline simple_event::simple_event() TAMER_NOEXCEPT
    : _r(0), _refcount(1), annotate_file_(0) {
}

inline simple_event::simple_event(abstract_rendezvous& r, uintptr_t rid,
                                  const char* file, int line) TAMER_NOEXCEPT
    : _r(&r), _rid(rid), _r_next(r.waiting_), _r_pprev(&r.waiting_),
      at_trigger_f_(0), at_trigger_arg_(0), _refcount(1),
      annotate_line_(line), annotate_file_(file) {
    if (r.waiting_)
	r.waiting_->_r_pprev = &_r_next;
    r.waiting_ = this;
#if TAMER_DEBUG > 1
    if (file && line)
	fprintf(stderr, "annotate simple_event(%p) %s:%d\n", this, file, line);
    else if (file)
	fprintf(stderr, "annotate simple_event(%p) %s\n", this, file);
#endif
}

#if TAMER_DEBUG
inline simple_event::~simple_event() TAMER_NOEXCEPT {
    assert(!_r);
# if TAMER_DEBUG > 1
    if (annotate_file_ && annotate_line_)
	fprintf(stderr, "destroy simple_event(%p) %s:%d\n", this, annotate_file_, annotate_line_);
    else if (annotate_file_)
	fprintf(stderr, "destroy simple_event(%p) %s\n", this, annotate_file_);
# endif
}
#endif

inline void simple_event::use(simple_event *e) TAMER_NOEXCEPT {
    if (e)
	++e->_refcount;
}

inline void simple_event::unuse(simple_event *e) TAMER_NOEXCEPT {
    if (e && --e->_refcount == 0)
	e->unuse_trigger();
}

inline void simple_event::unuse_clean(simple_event *e) TAMER_NOEXCEPT {
    if (e && --e->_refcount == 0)
	delete e;
}

inline simple_event::operator unspecified_bool_type() const {
    return _r ? &simple_event::empty : 0;
}

inline bool simple_event::empty() const {
    return _r == 0;
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

inline const char* simple_event::file_annotation() const {
    return annotate_file_;
}

inline int simple_event::line_annotation() const {
    return annotate_line_;
}

inline void simple_event::simple_trigger(bool values) {
    simple_trigger(this, values);
}

inline void simple_event::at_trigger(simple_event* x, simple_event* at_e) {
    if (at_e)
        at_trigger(x, trigger_hook, at_e);
}

inline void simple_event::at_trigger(simple_event* x, void (*f)(void*),
                                     void* arg) {
    if (x && *x && !x->at_trigger_f_) {
	x->at_trigger_f_ = f;
	x->at_trigger_arg_ = arg;
    } else
	hard_at_trigger(x, f, arg);
}

template <typename T> struct rid_cast {
    static inline uintptr_t in(T x) TAMER_NOEXCEPT {
	return static_cast<uintptr_t>(x);
    }
    static inline T out(uintptr_t x) TAMER_NOEXCEPT {
	return static_cast<T>(x);
    }
};

template <typename T> struct rid_cast<T *> {
    static inline uintptr_t in(T *x) TAMER_NOEXCEPT {
	return reinterpret_cast<uintptr_t>(x);
    }
    static inline T *out(uintptr_t x) TAMER_NOEXCEPT {
	return reinterpret_cast<T *>(x);
    }
};

} // namespace tamerpriv
} // namespace tamer
#endif /* TAMER_BASE_HH */
