#ifndef TAMER_UTIL_HH
#define TAMER_UTIL_HH 1
#include <memory>
#include <cassert>
namespace tamer {
namespace tamerpriv {

template <typename T, typename A = std::allocator<T> > class blockset { public:

    blockset() throw ();
    ~blockset();

    inline T &allocate_passive(size_t &i);
    inline void activate(size_t i) throw ();

    inline size_t npassive() const throw ()	{ return _npassive; }
    inline size_t nactive() const throw ()	{ return _nactive; }

    inline T *active_front() const throw ();
    inline void pop_active();
    inline void kill(size_t i);
    void clear();

  private:

    enum { tbnull = (size_t) -1, tbpassive = (size_t) -2 };
    
    T *_elts;
    size_t _cap;
    size_t _free;
    size_t _active;
    size_t _active_tail;
    size_t _npassive;
    size_t _nactive;
    A _alloc;

    void expand();
    
};

template <typename T, typename A>
blockset<T, A>::blockset() throw ()
    : _elts(0), _cap(0), _free(tbnull),
      _active(tbnull), _active_tail(tbnull), _npassive(0), _nactive(0)
{
}

template <typename T, typename A>
blockset<T, A>::~blockset()
{
    clear();
    _alloc.deallocate(_elts, _cap);
}

template <typename T, typename A>
inline T &blockset<T, A>::allocate_passive(size_t &i)
{
    if (_free == tbnull)
	expand();
    i = _free;
    _free = _elts[i].next;
    _elts[i].next = tbpassive;
    _npassive++;
    return _elts[i];
}

template <typename T, typename A>
void blockset<T, A>::expand()
{
    size_t new_cap = (_cap ? _cap * 4 : 8); // XXX integer overflow
    T *new_elts = _alloc.allocate(new_cap, this);
    for (size_t x = _free; x != tbnull; x = _elts[x].next)
	new_elts[x].next = _elts[x].next;
    if (_npassive)
	for (size_t i = 0; i != _cap; i++)
	    if (_elts[i].next == tbpassive) {
		_alloc.construct(&new_elts[i], _elts[i]);
		_alloc.destroy(&_elts[i]);
	    }
    for (size_t x = _active; x != tbnull; x = new_elts[x].next) {
	_alloc.construct(&new_elts[x], _elts[x]);
	_alloc.destroy(&_elts[x]);
    }
    for (size_t i = _cap; i != new_cap; i++) {
	_elts[i].next = _free;
	_free = i;
    }
    _alloc.deallocate(_elts, _cap);
    _elts = new_elts;
    _cap = new_cap;
}

template <typename T, typename A>
inline void blockset<T, A>::activate(size_t i) throw ()
{
    assert(i < _cap && _elts[i].next == tbpassive);
    if (_active != tbnull)
	_elts[_active_tail].next = i;
    else
	_active = i;
    _elts[i].next = tbnull;
    _active_tail = i;
    _npassive--;
    _nactive++;
}

template <typename T, typename A>
inline T *blockset<T, A>::active_front() const throw ()
{
    return (_active != tbnull ? &_elts[_active] : 0);
}

template <typename T, typename A>
inline void blockset<T, A>::pop_active()
{
    assert(_active != tbnull);
    _alloc.destroy(&_elts[_active]);
    _active = _elts[_active].next;
    _elts[_active].next = _free;
    _free = _active;
    _nactive--;
}

template <typename T, typename A>
inline void blockset<T, A>::kill(size_t i)
{
    assert(i < _cap && _elts[i].next == tbpassive);
    _alloc.destroy(&_elts[i]);
    _elts[i].next = _free;
    _free = i;
    _npassive--;
}

template <typename T, typename A>
inline void blockset<T, A>::clear()
{
    for (size_t i = 0; _npassive && i < _cap; i++)
	if (_elts[i].next == tbpassive)
	    kill(i);
    while (_active != tbnull)
	pop_active();
}


template <typename T, typename A = std::allocator<T> > class debuffer { public:

    debuffer() throw ();
    ~debuffer();

    inline void push_back(const T &);
    inline void push_front(const T &);

    inline size_t size() const throw ()		{ return _tail - _head; }

    inline T *front() const throw ();
    inline void pop_front();
    void clear();

  private:

    enum { nlocal = 4 };
    unsigned char _local_elts[nlocal * sizeof(T)];
    T *_elts;
    size_t _cap;
    size_t _head;
    size_t _tail;
    A _alloc;

    void expand();
    
};

template <typename T, typename A>
debuffer<T, A>::debuffer() throw ()
    : _elts(reinterpret_cast<T *>(_local_elts)), _cap(nlocal), _head(0), _tail(0)
{
}

template <typename T, typename A>
debuffer<T, A>::~debuffer()
{
    clear();
    if (_cap > nlocal)
	_alloc.deallocate(_elts, _cap);
}

template <typename T, typename A>
inline void debuffer<T, A>::push_back(const T &t)
{
    if (_tail - _head == _cap)
	expand();
    _alloc.construct(&_elts[_tail & (_cap - 1)], t);
    _tail++;
}

template <typename T, typename A>
void debuffer<T, A>::expand()
{
    size_t new_cap = (_cap ? _cap * 4 : 8); // XXX integer overflow
    T *new_elts = _alloc.allocate(new_cap, this);
    for (size_t x = _head; x != _tail; x++) {
	_alloc.construct(&new_elts[x & (new_cap - 1)], _elts[x & (_cap - 1)]);
	_alloc.destroy(&_elts[x & (_cap - 1)]);
    }
    if (_cap > nlocal)
	_alloc.deallocate(_elts, _cap);
    _elts = new_elts;
    _cap = new_cap;
}

template <typename T, typename A>
inline T *debuffer<T, A>::front() const throw ()
{
    return (_head != _tail ? &_elts[_head & (_cap - 1)] : 0);
}

template <typename T, typename A>
inline void debuffer<T, A>::pop_front()
{
    assert(_head != _tail);
    _alloc.destroy(&_elts[_head & (_cap - 1)]);
    _head++;
}

template <typename T, typename A>
inline void debuffer<T, A>::push_front(const T &t)
{
    if (_tail - _head == _cap)
	expand();
    _alloc.construct(&_elts[(_head - 1) & (_cap - 1)], t);
    _head--;
}

template <typename T, typename A>
inline void debuffer<T, A>::clear()
{
    while (_head != _tail)
	pop_front();
}

}}
#endif /* TAMER__UTIL_HH */
