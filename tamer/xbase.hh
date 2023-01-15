#ifndef TAMER_XBASE_HH
#define TAMER_XBASE_HH 1
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
#include <stdexcept>
#include <string>
#include <stdint.h>
#include <assert.h>
#include <tamer/autoconf.h>
namespace tamer {

// Improve error messages from overloaded functions.
template <typename R> class one_argument_rendezvous_tag {};
template <typename R> class zero_argument_rendezvous_tag {};

template <typename I = void> class rendezvous;
template <typename T0 = void, typename T1 = void, typename T2 = void,
          typename T3 = void> class event;
#if TAMER_HAVE_PREEVENT
template <typename R, typename T0 = void> class preevent;
#endif
class tamed_class;
class driver;

class tamer_error : public std::runtime_error { public:
    explicit tamer_error(const std::string& arg)
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
class blocking_rendezvous;
class explicit_rendezvous;
class closure;

class simple_event { public:
    // DO NOT derive from this class!

    inline simple_event() TAMER_NOEXCEPT;
    inline simple_event(abstract_rendezvous& r, uintptr_t rid,
                        const char* file, int line) TAMER_NOEXCEPT;
#if TAMER_DEBUG
    inline ~simple_event() TAMER_NOEXCEPT;
#endif

    static inline void use(simple_event *e) TAMER_NOEXCEPT;
    static inline void unuse(simple_event *e) TAMER_NOEXCEPT;
    static inline void unuse_clean(simple_event *e) TAMER_NOEXCEPT;
    inline bool unused() const;

    inline explicit operator bool() const;
    inline bool empty() const;
    inline abstract_rendezvous *rendezvous() const;
    inline uintptr_t rid() const;
    inline simple_event *next() const;
    inline bool shared() const;
    inline bool has_at_trigger() const;

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
    simple_event* _r_next;
    simple_event** _r_pprev;
    void (*at_trigger_f_)(void*);
    void* at_trigger_arg_;
    unsigned _refcount;
#if !TAMER_NOTRACE
    int line_annotation_;
    const char* file_annotation_;
#endif

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

  protected:
    simple_event *waiting_;
    uint8_t rtype_;
    bool is_volatile_;

    inline void remove_waiting() TAMER_NOEXCEPT;

  private:
    abstract_rendezvous(const abstract_rendezvous &);
    abstract_rendezvous &operator=(const abstract_rendezvous &);

    friend class simple_event;
    friend class driver;
};

class simple_driver {
  protected:
    simple_driver();
    ~simple_driver();

    inline void add_blocked(closure* c);
    inline void make_unblocked(closure* c);

    inline closure* pop_unblocked();

    inline unsigned nclosure_slots() const;
    inline closure* closure_slot(unsigned i) const;

  public:
    inline bool has_unblocked() const;
    inline void run_unblocked();

    static simple_driver immediate_driver;

  private:
    struct cptr {
        closure* c;
        unsigned next;
    };

    unsigned ccap_;
    mutable unsigned cfree_;
    mutable unsigned cunblocked_;
    unsigned cunblocked_tail_;
    cptr* cs_;

    void add(closure* c);
    void grow();

    friend class blocking_rendezvous;
    friend class closure;
};

class blocking_rendezvous : public abstract_rendezvous {
  public:
    inline blocking_rendezvous(rendezvous_flags flags, rendezvous_type rtype) TAMER_NOEXCEPT;
    inline ~blocking_rendezvous() TAMER_NOEXCEPT;

    inline bool blocked() const;

    inline void block(simple_driver* driver, closure& c, unsigned position);
    inline void block(closure& c, unsigned position);
    inline void unblock();

  protected:
    closure* blocked_closure_;

    void hard_free() TAMER_NOEXCEPT;

    friend class abstract_rendezvous;
    friend class simple_driver;
    friend struct driver_timerset;
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
    inline functional_rendezvous(rendezvous_flags rflags, rendezvous_type rtype,
                                 void (*f)(functional_rendezvous *fr,
                                           simple_event *e, bool values) TAMER_NOEXCEPT)
        : abstract_rendezvous(rflags, rtype), f_(f) {
    }
    inline ~functional_rendezvous() {
        remove_waiting();
    }

  private:
    void (*f_)(functional_rendezvous *r,
               simple_event *e, bool values) TAMER_NOEXCEPT;

    friend class simple_event;
};

typedef void (*closure_activator)(closure*);

class closure {
  public:
    typedef closure tamer_closure_type;
    closure_activator tamer_activator_;
    unsigned tamer_block_position_;
    unsigned tamer_driver_index_;
    simple_driver* tamer_blocked_driver_;
    closure* tamer_closures_next_;
    closure** tamer_closures_pprev_;
#if !TAMER_NOTRACE
    int tamer_location_line_;
    const char* tamer_location_file_;
    std::string* tamer_description_;
#endif
    inline ~closure();

    inline bool has_location() const;
    inline bool has_description() const;
    inline const char* location_file() const;
    inline int location_line() const;
    std::string location() const;
    inline std::string description() const;
    std::string location_description() const;

    inline void initialize_closure(closure_activator f, ...);
    inline void initialize_closure(closure_activator f, tamed_class* k);
    inline void set_location(const char* file, int line);
    inline void set_description(std::string description);

    inline void exit_at_destroy(tamed_class* k);

    inline void unblock();
};


template <typename T>
class closure_owner {
  public:
    inline closure_owner(T& c)
        : c_(&c) {
    }
    inline ~closure_owner() {
        delete c_;
    }
    inline void reset() {
        c_ = 0;
    }
  private:
    T* c_;
};

template <typename R>
class rendezvous_owner {
  public:
    inline rendezvous_owner(R& r)
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
    R* r_;
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


inline bool simple_driver::has_unblocked() const {
    while (cunblocked_ && !cs_[cunblocked_].c) {
        unsigned next = cs_[cunblocked_].next;
        cs_[cunblocked_].next = cfree_;
        cfree_ = cunblocked_;
        cunblocked_ = next;
    }
    return cunblocked_ != 0;
}

inline closure* simple_driver::pop_unblocked() {
    if (has_unblocked()) {
        unsigned i = cunblocked_;
        closure* c = cs_[i].c;
        cunblocked_ = cs_[i].next;
        cs_[i].c = 0;
        cs_[i].next = cfree_;
        cfree_ = i;
        return c;
    } else
        return 0;
}

inline void simple_driver::run_unblocked() {
    while (closure* c = pop_unblocked())
        c->tamer_activator_(c);
}

inline void simple_driver::add_blocked(closure* c) {
#if TAMER_NOTRACE
    c->tamer_driver_index_ = 0;
#else
    add(c);
#endif
}

inline void simple_driver::make_unblocked(closure* c) {
    if (!c->tamer_driver_index_)
        add(c);
    if (cunblocked_)
        cs_[cunblocked_tail_].next = c->tamer_driver_index_;
    else
        cunblocked_ = c->tamer_driver_index_;
    cunblocked_tail_ = c->tamer_driver_index_;
}

inline unsigned simple_driver::nclosure_slots() const {
    return ccap_;
}

inline closure* simple_driver::closure_slot(unsigned i) const {
    assert(i < ccap_);
    return cs_[i].c;
}


inline blocking_rendezvous::blocking_rendezvous(rendezvous_flags flags,
                                                rendezvous_type rtype) TAMER_NOEXCEPT
    : abstract_rendezvous(flags, rtype), blocked_closure_() {
}

inline blocking_rendezvous::~blocking_rendezvous() TAMER_NOEXCEPT {
    if (blocked_closure_)
        hard_free();
}

inline bool blocking_rendezvous::blocked() const {
    return blocked_closure_ && blocked_closure_->tamer_blocked_driver_;
}

inline void blocking_rendezvous::block(simple_driver* driver, closure& c,
                                       unsigned position) {
    assert(!c.tamer_blocked_driver_);
    blocked_closure_ = &c;
    c.tamer_block_position_ = position;
    c.tamer_blocked_driver_ = driver;
    driver->add_blocked(&c);
}

inline void closure::unblock() {
    if (simple_driver* d = tamer_blocked_driver_) {
        tamer_blocked_driver_ = 0;
        if (d != &simple_driver::immediate_driver)
            d->make_unblocked(this);
        else
            tamer_activator_(this);
    }
}

inline void blocking_rendezvous::unblock() {
    if (closure* cl = blocked_closure_) {
        blocked_closure_ = 0;
        cl->unblock();
    }
}


inline closure::~closure() {
    if (tamer_closures_pprev_
        && (*tamer_closures_pprev_ = tamer_closures_next_))
        tamer_closures_next_->tamer_closures_pprev_ = tamer_closures_pprev_;
    delete tamer_description_;
}

inline bool closure::has_location() const {
    return tamer_location_file_ || tamer_location_line_;
}

inline bool closure::has_description() const {
    return tamer_description_ && !tamer_description_->empty();
}

inline void closure::set_location(const char* file, int line) {
    TAMER_IFTRACE(tamer_location_file_ = file);
    TAMER_IFTRACE(tamer_location_line_ = line);
    TAMER_IFNOTRACE((void) file, (void) line);
}

inline void closure::set_description(std::string description) {
#if !TAMER_NOTRACE
    if (tamer_description_)
        *tamer_description_ = TAMER_MOVE(description);
    else if (!description.empty())
        tamer_description_ = new std::string(TAMER_MOVE(description));
#else
    (void) description;
#endif
}

inline const char* closure::location_file() const {
    return TAMER_IFTRACE_ELSE(tamer_location_file_, 0);
}

inline int closure::location_line() const {
    return TAMER_IFTRACE_ELSE(tamer_location_line_, 0);
}

inline std::string closure::description() const {
#if !TAMER_NOTRACE
    if (tamer_description_)
        return *tamer_description_;
#endif
    return std::string();
}


inline simple_event::simple_event() TAMER_NOEXCEPT
    : _r(0), _refcount(1) TAMER_IFTRACE(, file_annotation_(0)) {
}

inline simple_event::simple_event(abstract_rendezvous& r, uintptr_t rid,
                                  const char* file, int line) TAMER_NOEXCEPT
    : _r(&r), _rid(rid), _r_next(r.waiting_), _r_pprev(&r.waiting_),
      at_trigger_f_(0), at_trigger_arg_(0), _refcount(1)
      TAMER_IFTRACE(, line_annotation_(line), file_annotation_(file)) {
    if (r.waiting_)
        r.waiting_->_r_pprev = &_r_next;
    r.waiting_ = this;
    TAMER_IFNOTRACE((void) file, (void) line);
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
    if (file_annotation() && line_annotation())
        fprintf(stderr, "destroy simple_event(%p) %s:%d\n", this, file_annotation(), line_annotation());
    else if (file_annotation())
        fprintf(stderr, "destroy simple_event(%p) %s\n", this, file_annotation());
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

inline bool simple_event::unused() const {
    return _refcount == 0;
}

inline simple_event::operator bool() const {
    return _r;
}

inline bool simple_event::empty() const {
    return !_r;
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

inline bool simple_event::shared() const {
    return _r && _refcount > 1;
}

inline bool simple_event::has_at_trigger() const {
    return at_trigger_f_;
}

inline const char* simple_event::file_annotation() const {
#if !TAMER_NOTRACE
    return file_annotation_;
#else
    return 0;
#endif
}

inline int simple_event::line_annotation() const {
#if !TAMER_NOTRACE
    return line_annotation_;
#else
    return 0;
#endif
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
