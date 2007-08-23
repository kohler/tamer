#ifndef TAMER_UTIL_HH
#define TAMER_UTIL_HH 1
/* Copyright (c) 2007, Eddie Kohler
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
namespace tamer {
namespace tamerutil {

/** @file <tamer/util.hh>
 *  @brief  Utility classes for the tamer implementation.
 */

/** @namespace tamer::tamerutil
 *  @brief  Namespace containing utility classes useful for Tamer
 *  implementation, and possibly elsewhere.
 */

template <typename T> struct ready_set_element {
    size_t next;
    T value;
};

/** @class ready_set tamer/util.hh <tamer/util.hh>
 *  @brief  Utility class: A combination of set and queue.
 *
 *  A ready_set is a multiset coupled with a queue.  Elements are allocated as
 *  part of the multiset.  Allocation returns a unique index identifying the
 *  element.  Later, the push_back_element function, which takes an index,
 *  shifts an element from the multiset to the end of the queue.  front_ptr()
 *  and pop_front() operations operate on the queue.
 *
 *  The type T must have a usable copy constructor and a usable destructor.
 *
 *  The second template argument is an allocator for type
 *  ready_set_element<T>.
 */
template <typename T, typename A = std::allocator<ready_set_element<T> > >
class ready_set { public:

    typedef size_t size_type;
    typedef size_t index_type;
    
    /** @brief  Default constructor creates an empty ready_set. */
    ready_set() throw ();

    /** @brief  Destroy the ready_set. */
    ~ready_set();

    /** @brief  Return number of elements in the multiset.
     *  @note   Does not count elements in the associated queue. */
    inline size_t multiset_size() const throw () {
	return _nunready;
    }

    /** @brief  Insert element into the multiset.
     *  @param  v  The element to insert (or any parameter to a T constructor).
     *  @return The new element's index. */
    template <typename V>
    inline index_type insert(const V &v) {
	index_type i = allocate_index();
	(void) new(static_cast<void *>(&_elts[i].value)) T(v);
	return i;
    }

    /** @brief  Insert element into the multiset.
     *  @param  v1  First parameter to a two-argument T constructor.
     *  @param  v2  Second parameter to a two-argument T constructor.
     *  @return The new element's index. */
    template <typename V1, typename V2>
    inline index_type insert(const V1 &v1, const V2 &v2) {
	index_type i = allocate_index();
	(void) new(static_cast<void *>(&_elts[i].value)) T(v1, v2);
	return i;
    }

    /** @brief  Remove element from the multiset.
     *  @param  i  Index of element to remove.
     *  @a i must correspond to a current element of the multiset. */
    inline void erase(index_type i);

    /** @brief  Shift a multiset element to the end of the queue.
     *  @param  i  Index of element to shift. */
    inline void push_back_element(index_type i) throw ();

    /** @brief  Return number of elements in the queue. */
    inline size_type size() const throw () {
	return _size;
    }
    
    /** @brief  Return a pointer to the front element of the queue, if any.
     *  @return Returns a null pointer if the queue is empty. */
    inline T *front_ptr() throw () {
	return (_front != rsnull ? &_elts[_front].value : 0);
    }

    /** @brief  Remove the front element from the queue.
     *  @pre    The queue must not be empty. */
    inline void pop_front();

    /** @brief  Remove all elements from the multiset and the queue. */
    void clear();

  private:

    enum { rsnull = (size_t) -1, rsunready = (size_t) -2 };
    
    ready_set_element<T> *_elts;
    size_t _cap;
    size_t _free;
    size_t _front;
    size_t _back;
    size_t _nunready;
    size_t _size;
    A _alloc;

    inline index_type allocate_index() {
	if (_free == rsnull)
	    expand();
	index_type i = _free;
	_free = _elts[i].next;
	_elts[i].next = rsunready;
	_nunready++;
	return i;
    }
    
    void expand();
    
};

template <typename T, typename A>
ready_set<T, A>::ready_set() throw ()
    : _elts(0), _cap(0), _free(rsnull),
      _front(rsnull), _back(rsnull), _nunready(0), _size(0)
{
}

template <typename T, typename A>
ready_set<T, A>::~ready_set()
{
    clear();
    _alloc.deallocate(_elts, _cap);
}

template <typename T, typename A>
inline void ready_set<T, A>::erase(index_type i)
{
    assert(i < _cap && _elts[i].next == rsunready);
    _alloc.destroy(&_elts[i]);
    _elts[i].next = _free;
    _free = i;
    _nunready--;
}

template <typename T, typename A>
void ready_set<T, A>::expand()
{
    size_t new_cap = (_cap ? _cap * 4 : 8); // XXX integer overflow
    ready_set_element<T> *new_elts = _alloc.allocate(new_cap, this);
    for (size_t x = _free; x != rsnull; x = _elts[x].next)
	new_elts[x].next = _elts[x].next;
    if (_nunready)
	for (size_t i = 0; i != _cap; i++)
	    if (_elts[i].next == rsunready) {
		_alloc.construct(&new_elts[i], _elts[i]);
		_alloc.destroy(&_elts[i]);
	    }
    for (size_t x = _front; x != rsnull; x = new_elts[x].next) {
	_alloc.construct(&new_elts[x], _elts[x]);
	_alloc.destroy(&_elts[x]);
    }
    for (size_t i = _cap; i != new_cap; i++) {
	new_elts[i].next = _free;
	_free = i;
    }
    _alloc.deallocate(_elts, _cap);
    _elts = new_elts;
    _cap = new_cap;
}

template <typename T, typename A>
inline void ready_set<T, A>::push_back_element(index_type i) throw ()
{
    assert(i < _cap && _elts[i].next == rsunready);
    if (_front != rsnull)
	_elts[_back].next = i;
    else
	_front = i;
    _elts[i].next = rsnull;
    _back = i;
    _nunready--;
    _size++;
}

template <typename T, typename A>
inline void ready_set<T, A>::pop_front()
{
    assert(_front != rsnull);
    size_t next = _elts[_front].next;
    _alloc.destroy(&_elts[_front]);
    _elts[_front].next = _free;
    _free = _front;
    _front = next;
    _size--;
}

template <typename T, typename A>
inline void ready_set<T, A>::clear()
{
    for (size_t i = 0; _nunready && i < _cap; i++)
	if (_elts[i].next == rsunready)
	    erase(i);
    while (_front != rsnull)
	pop_front();
}


/** @class debuffer tamer/util.hh <tamer/util.hh>
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
    typedef size_t size_type;

    /** @brief  Default constructor creates an empty buffer. */
    inline debuffer() throw ()
	: _elts(reinterpret_cast<T *>(_local_elts)), _cap(nlocal),
	  _head(0), _tail(0) {
    }
    
    /** @brief  Destroy the buffer. */
    ~debuffer();
    
    /** @brief  Return the number of elements in the buffer. */
    inline size_type size() const throw () {
	return _tail - _head;
    }

    /** @brief  Return true iff the buffer contains no elements. */
    inline bool empty() const throw () {
	return _head == _tail;
    }
    
    /** @brief  Return a reference to the front element of the buffer.
     *  @pre    The buffer must not be empty.
     *  @sa     front_ptr() */
    inline T &front() throw () {
	assert(_head != _tail);
	return _elts[_head & (_cap - 1)];
    }
    
    /** @brief  Return a reference to the front element of the buffer.
     *  @pre    The buffer must not be empty.
     *  @sa     front_ptr() */
    inline const T &front() const throw () {
	assert(_head != _tail);
	return _elts[_head & (_cap - 1)];
    }

    /** @brief  Return a pointer to the front element of the buffer, if any.
     *  @return Returns a null pointer if the buffer is empty.
     *  @sa     front() */
    inline T *front_ptr() throw () {
	return (_head != _tail ? &_elts[_head & (_cap - 1)] : 0);
    }

    /** @brief  Return a pointer to the front element of the buffer, if any.
     *  @return Returns a null pointer if the buffer is empty.
     *  @sa     front() */
    inline const T *front_ptr() const throw () {
	return (_head != _tail ? &_elts[_head & (_cap - 1)] : 0);
    }

    /** @brief  Add an element to the back of the buffer.
     *  @param  v  Element to add.
     */
    inline void push_back(const T &v) {
	if (_tail - _head == _cap)
	    expand();
	_alloc.construct(&_elts[_tail & (_cap - 1)], v);
	++_tail;
    }
    
    /** @brief  Add an element to the front of the buffer.
     *  @param  v  Element to add.
     */
    inline void push_front(const T &v) {
	if (_tail - _head == _cap)
	    expand();
	_alloc.construct(&_elts[(_head - 1) & (_cap - 1)], v);
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

}}
#endif /* TAMER__UTIL_HH */
