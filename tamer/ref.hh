#ifndef TAMER_REF_HH
#define TAMER_REF_HH 1
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
namespace tamer {

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
	if (--_use_count == 0)
	    return _weak_count == 0;
	else
	    return false;
    }

    void weak_add_ref() {
	++_weak_count;
    }

    bool weak_release() {
	assert(_weak_count);
	if (--_weak_count == 0)
	    return _use_count == 0;
	else
	    return false;
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
	if (--_use_count == 0) {
	    static_cast<T *>(this)->full_release();
	    return _weak_count == 0;
	} else
	    return false;
    }

};


template <typename T> class ref_ptr { public:

    ref_ptr()
	: _t(0) {
    }

    template <typename U> explicit ref_ptr(U *u)
	: _t(u) {
	assert(!_t || _t->use_count() == 1);
    }

    ref_ptr(const ref_ptr<T> &r)
	: _t(r._t) {
	if (_t)
	    _t->add_ref_copy();
    }

    template <typename U> ref_ptr(const ref_ptr<U> &r)
	: _t(r._t) {
	if (_t)
	    _t->add_ref_copy();
    }

    ~ref_ptr() {
	if (_t && _t->release())
	    delete _t;
    }

    ref_ptr<T> &operator=(const ref_ptr<T> &r) {
	if (r._t)
	    r._t->add_ref_copy();
	if (_t && _t->release())
	    delete _t;
	_t = r._t;
	return *this;
    }

    template <typename U> ref_ptr<T> &operator=(const ref_ptr<U> &r) {
	if (r._t)
	    r._t->add_ref_copy();
	if (_t && _t->release())
	    delete _t;
	_t = r._t;
	return *this;
    }

    T &operator*() const {
	return *_t;
    }

    T *operator->() const {
	return _t;
    }

    T *get() const {
	return _t;
    }

    typedef T *ref_ptr::*unspecified_bool_type;

    operator unspecified_bool_type() const {
	return _t ? &ref_ptr::_t : 0;
    }

    bool operator!() const {
	return !_t;
    }

  private:

    T *_t;

};


template <typename T> class passive_ref_ptr { public:

    passive_ref_ptr()
	: _t(0) {
    }

    template <typename U> explicit passive_ref_ptr(U *u)
	: _t(u) {
	if (_t)
	    _t->weak_add_ref();
    }

    template <typename U> explicit passive_ref_ptr(const ref_ptr<U> &r)
	: _t(r.operator->()) {
	if (_t)
	    _t->weak_add_ref();
    }

    passive_ref_ptr(const passive_ref_ptr<T> &r)
	: _t(r._t) {
	if (_t)
	    _t->weak_add_ref();
    }

    template <typename U> passive_ref_ptr(const passive_ref_ptr<U> &r)
	: _t(r._t) {
	if (_t)
	    _t->weak_add_ref();
    }

    ~passive_ref_ptr() {
	if (_t && _t->weak_release())
	    delete _t;
    }

    passive_ref_ptr<T> &operator=(const passive_ref_ptr<T> &r) {
	if (r._t)
	    r._t->weak_add_ref();
	if (_t && _t->weak_release())
	    delete _t;
	_t = r._t;
	return *this;
    }

    template <typename U> passive_ref_ptr<T> &operator=(const passive_ref_ptr<U> &r) {
	if (r._t)
	    r._t->weak_add_ref();
	if (_t && _t->weak_release())
	    delete _t;
	_t = r._t;
	return *this;
    }

    T &operator*() const {
	return *_t;
    }

    T *operator->() const {
	return _t;
    }

    T *get() const {
	return _t;
    }

    typedef T *passive_ref_ptr::*unspecified_bool_type;

    operator unspecified_bool_type() const {
	return _t ? &passive_ref_ptr::_t : 0;
    }

    bool operator!() const {
	return !_t;
    }

  private:

    T *_t;

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
