#ifndef REFPTR_HH
#define REFPTR_HH 1

template <typename T> class refptr { public:

    refptr()
	: _t(0) {
    }
    
    refptr(T *t)
	: _t(t) {
	if (_t)
	    _t->use();
    }
    
    refptr(const refptr<T> &r)
	: _t(r._t) {
	if (_t)
	    _t->use();
    }
    
    ~refptr() {
	if (_t)
	    _t->unuse();
    }

    T *operator->() const {
	return _t;
    }

    T *value() const {
	return _t;
    }

    refptr<T> &operator=(const refptr<T> &r) {
	if (r._t)
	    r._t->use();
	if (_t)
	    _t->unuse();
	_t = r._t;
	return *this;
    }

    refptr<T> &operator=(T *t) {
	if (_t && _t != t)
	    _t->unuse();
	_t = t;
	return *this;
    }

  private:

    T *_t;
    
};

#endif
