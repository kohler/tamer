#ifndef TAMER_UTILITY_HH
#define TAMER_UTILITY_HH 1
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
#include <memory>
#include <cassert>
#include <cstddef>
#include <tamer/autoconf.h>
namespace tamer {

/** @file <tamer/utility.hh>
 *  @brief  Utility classes provided by Tamer.
 */

/** @class debuffer tamer/utility.hh <tamer/utility.hh>
 *  @brief  Utility class: A simple deque.
 *
 *  This double-ended buffer supports front(), push_back(), push_front(), and
 *  pop_front() operations, like a doubly linked list.  Unlike a doubly linked
 *  list (but like a vector or deque), all objects in the buffer are stored in
 *  a contiguous array of memory, reducing the number of allocations required
 *  to store small numbers of elements.
 *
 *  The type T must have a usable copy constructor and a usable destructor.
 */
template <typename T, typename A = std::allocator<T> > class debuffer { public:

    typedef T value_type;
    typedef T *pointer;
    typedef T &reference;
    typedef const T &const_reference;
    typedef std::size_t size_type;

    /** @brief  Default constructor creates an empty buffer. */
    inline debuffer() TAMER_NOEXCEPT
	: _elts(reinterpret_cast<T *>(_local_elts)), _cap(nlocal),
	  _head(0), _tail(0) {
    }

    /** @brief  Destroy the buffer. */
    ~debuffer();

    /** @brief  Return the number of elements in the buffer. */
    inline size_type size() const TAMER_NOEXCEPT {
	return _tail - _head;
    }

    /** @brief  Return true iff the buffer contains no elements. */
    inline bool empty() const TAMER_NOEXCEPT {
	return _head == _tail;
    }

    /** @brief  Return a reference to the front element of the buffer.
     *  @pre    The buffer must not be empty.
     *  @sa     front_ptr() */
    inline T &front() TAMER_NOEXCEPT {
	assert(_head != _tail);
	return _elts[_head & (_cap - 1)];
    }

    /** @brief  Return a reference to the front element of the buffer.
     *  @pre    The buffer must not be empty.
     *  @sa     front_ptr() */
    inline const T &front() const TAMER_NOEXCEPT {
	assert(_head != _tail);
	return _elts[_head & (_cap - 1)];
    }

    /** @brief  Return a pointer to the front element of the buffer, if any.
     *  @return Returns a null pointer if the buffer is empty.
     *  @sa     front() */
    inline T *front_ptr() TAMER_NOEXCEPT {
	return (_head != _tail ? &_elts[_head & (_cap - 1)] : 0);
    }

    /** @brief  Return a pointer to the front element of the buffer, if any.
     *  @return Returns a null pointer if the buffer is empty.
     *  @sa     front() */
    inline const T *front_ptr() const TAMER_NOEXCEPT {
	return (_head != _tail ? &_elts[_head & (_cap - 1)] : 0);
    }

    /** @brief  Add an element to the back of the buffer.
     *  @param  x  Element to add.
     */
    inline void push_back(const T &x) {
	if (_tail - _head == _cap)
	    expand();
	_alloc.construct(&_elts[_tail & (_cap - 1)], x);
	++_tail;
    }

    /** @brief  Add an element to the front of the buffer.
     *  @param  x  Element to add.
     */
    inline void push_front(const T &x) {
	if (_tail - _head == _cap)
	    expand();
	_alloc.construct(&_elts[(_head - 1) & (_cap - 1)], x);
	--_head;
    }

    /** @brief  Remove the front element from the buffer.
     *  @pre    The buffer must not be empty.
     */
    inline void pop_front() {
	assert(_head != _tail);
	_alloc.destroy(&_elts[_head & (_cap - 1)]);
	++_head;
    }

    /** @brief  Remove all elements from the buffer. */
    inline void clear() {
	while (_head != _tail)
	    pop_front();
    }

  private:

    enum { nlocal = 4 };
    unsigned char _local_elts[nlocal * sizeof(T)];
    T *_elts;
    size_type _cap;
    size_type _head;
    size_type _tail;
    A _alloc;

    void expand();

};

template <typename T, typename A>
debuffer<T, A>::~debuffer()
{
    clear();
    if (_cap > nlocal)
	_alloc.deallocate(_elts, _cap);
}

template <typename T, typename A>
void debuffer<T, A>::expand()
{
    size_type new_cap = (_cap ? _cap * 4 : 8); // XXX integer overflow
    T *new_elts = _alloc.allocate(new_cap, this);
    for (size_type x = _head, y = 0; x != _tail; ++x, ++y) {
	_alloc.construct(&new_elts[y], _elts[x & (_cap - 1)]);
	_alloc.destroy(&_elts[x & (_cap - 1)]);
    }
    if (_cap > nlocal)
	_alloc.deallocate(_elts, _cap);
    _elts = new_elts;
    _cap = new_cap;
    _tail = size();
    _head = 0;
}

} // namespace tamer
#endif /* TAMER_UTILITY_HH */
