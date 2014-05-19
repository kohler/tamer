#ifndef TAMER_REF_HH
#define TAMER_REF_HH 1
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
namespace tamer {

class ref;

class ref_monitor {
  public:
    explicit inline ref_monitor(ref& m);
    inline ~ref_monitor() {
        if (pprev_)
            *pprev_ = next_;
        if (next_)
            next_->pprev_ = pprev_;
    }

    typedef bool (ref_monitor::*unspecified_bool_type)() const;

    bool empty() const {
        return !pprev_;
    }
    operator unspecified_bool_type() const {
        return pprev_ ? &ref_monitor::empty : 0;
    }
    bool operator!() const {
        return !pprev_;
    }

  private:
    ref_monitor** pprev_;
    ref_monitor* next_;

    ref_monitor(const ref_monitor&);
    ref_monitor& operator=(const ref_monitor&);
    friend class ref;
};

class ref {
  public:
    ref()
        : first_(0) {
    }
    ~ref() {
        while (ref_monitor* w = first_) {
            first_ = w->next_;
            w->pprev_ = 0;
            w->next_ = 0;
        }
    }

  private:
    ref_monitor* first_;

    ref(const ref&);
    ref& operator=(const ref&);
    friend class ref_monitor;
};

inline ref_monitor::ref_monitor(ref& m)
    : pprev_(&m.first_), next_(m.first_) {
    if (next_)
        next_->pprev_ = &next_;
    m.first_ = this;
}


template <typename T> class enable_ref_ptr_with_full_release;
template <typename T> class ref_ptr;
template <typename T> class passive_ref_ptr;

class enable_ref_ptr { public:

    enable_ref_ptr()
	: _use_count(1), _weak_count(0) {
    }

    ~enable_ref_ptr() {
	assert(!_use_count && !_weak_count);
    }

  private:

    uint32_t _use_count;
    uint32_t _weak_count;

    enable_ref_ptr(const enable_ref_ptr &);
    enable_ref_ptr &operator=(const enable_ref_ptr &);

    template <typename T> friend class enable_ref_ptr_with_full_release;
    template <typename T> friend class ref_ptr;
    template <typename T> friend class passive_ref_ptr;

    void add_ref_copy() {
	++_use_count;
    }

    bool release() {
	assert(_use_count);
	return --_use_count == 0 && _weak_count == 0;
    }

    void weak_add_ref() {
	++_weak_count;
    }

    bool weak_release() {
	assert(_weak_count);
	return --_weak_count == 0 && _use_count == 0;
    }

    uint32_t use_count() const {
	return _use_count;
    }

};


template <typename T> class enable_ref_ptr_with_full_release : public enable_ref_ptr { public:

    enable_ref_ptr_with_full_release() {
    }

    ~enable_ref_ptr_with_full_release() {
    }

  private:

    template <typename U> friend class ref_ptr;
    template <typename U> friend class passive_ref_ptr;

    bool release() {
	assert(_use_count);
	if (_use_count == 1)
	    static_cast<T *>(this)->full_release();
	return --_use_count == 0 && _weak_count == 0;
    }

};


template <typename T> class ref_ptr { public:

    ref_ptr()
	: _t(0) {
    }

    template <typename U> explicit ref_ptr(U* ptr)
	: _t(ptr) {
	assert(!_t || _t->use_count() == 1);
    }

    ref_ptr(const ref_ptr<T>& x)
	: _t(x._t) {
	if (_t)
	    _t->add_ref_copy();
    }

    ref_ptr(ref_ptr<T>&& x)
        : _t(x._t) {
        x._t = 0;
    }

    template <typename U> ref_ptr(const ref_ptr<U>& x)
	: _t(x._t) {
	if (_t)
	    _t->add_ref_copy();
    }

    ~ref_ptr() {
	if (_t && _t->release())
	    delete _t;
    }

    ref_ptr<T>& operator=(const ref_ptr<T>& x) {
	if (x._t)
	    x._t->add_ref_copy();
	if (_t && _t->release())
	    delete _t;
	_t = x._t;
	return *this;
    }

    ref_ptr<T>& operator=(ref_ptr<T>&& x) {
        std::swap(_t, x._t);
	return *this;
    }

    template <typename U> ref_ptr<T>& operator=(const ref_ptr<U>& x) {
	if (x._t)
	    x._t->add_ref_copy();
	if (_t && _t->release())
	    delete _t;
	_t = x._t;
	return *this;
    }

    T& operator*() const {
	return *_t;
    }

    T* operator->() const {
	return _t;
    }

    T* get() const {
	return _t;
    }

    typedef T* ref_ptr::*unspecified_bool_type;

    operator unspecified_bool_type() const {
	return _t ? &ref_ptr::_t : 0;
    }

    bool operator!() const {
	return !_t;
    }

  private:
    T* _t;
};


template <typename T> class passive_ref_ptr { public:

    passive_ref_ptr()
	: _t(0) {
    }

    template <typename U> explicit passive_ref_ptr(U* ptr)
	: _t(ptr) {
	if (_t)
	    _t->weak_add_ref();
    }

    template <typename U> explicit passive_ref_ptr(const ref_ptr<U>& x)
	: _t(x.operator->()) {
	if (_t)
	    _t->weak_add_ref();
    }

    passive_ref_ptr(const passive_ref_ptr<T>& x)
	: _t(x._t) {
	if (_t)
	    _t->weak_add_ref();
    }

    passive_ref_ptr(passive_ref_ptr<T>&& x)
	: _t(x._t) {
        x._t = 0;
    }

    template <typename U> passive_ref_ptr(const passive_ref_ptr<U>& x)
	: _t(x._t) {
	if (_t)
	    _t->weak_add_ref();
    }

    ~passive_ref_ptr() {
	if (_t && _t->weak_release())
	    delete _t;
    }

    passive_ref_ptr<T>& operator=(const passive_ref_ptr<T>& x) {
	if (x._t)
	    x._t->weak_add_ref();
	if (_t && _t->weak_release())
	    delete _t;
	_t = x._t;
	return *this;
    }

    passive_ref_ptr<T>& operator=(passive_ref_ptr<T>&& x) {
        std::swap(_t, x._t);
        return *this;
    }

    template <typename U> passive_ref_ptr<T>& operator=(const passive_ref_ptr<U>& x) {
	if (x._t)
	    x._t->weak_add_ref();
	if (_t && _t->weak_release())
	    delete _t;
	_t = x._t;
	return *this;
    }

    T& operator*() const {
	return *_t;
    }

    T* operator->() const {
	return _t;
    }

    T* get() const {
	return _t;
    }

    typedef T* passive_ref_ptr::*unspecified_bool_type;

    operator unspecified_bool_type() const {
	return _t ? &passive_ref_ptr::_t : 0;
    }

    bool operator!() const {
	return !_t;
    }

  private:
    T* _t;
};


template <typename T, typename U>
inline bool operator==(const ref_ptr<T> &a, const ref_ptr<T> &b)
{
    return a.get() == b.get();
}

template <typename T, typename U>
inline bool operator!=(const ref_ptr<T> &a, const ref_ptr<T> &b)
{
    return a.get() != b.get();
}

template <typename T, typename U>
inline bool operator==(const passive_ref_ptr<T> &a, const passive_ref_ptr<T> &b)
{
    return a.get() == b.get();
}

template <typename T, typename U>
inline bool operator!=(const passive_ref_ptr<T> &a, const passive_ref_ptr<T> &b)
{
    return a.get() != b.get();
}

}
#endif /* TAMER_REF_HH */
