#ifndef CACHE_HH
#define CACHE_HH 1
#include <tamer.hh>
#include <assert.h>
#include <stdlib.h>
#include <string>
#include "refptr.hh"

extern ssize_t g_cache_max;

struct cache_entry {

    cache_entry(const std::string &n, char *d, size_t len)
	: _filename(n), _data(d), _len(len), _refcount(1), _next(0), _prev(0) {
    }

    ~cache_entry() {
	assert(!_prev && !_next);
	delete[] _data;
    }

    void use() {
	_refcount++;
    }

    void unuse() {
	if (!--_refcount)
	    delete this;
    }

    char *data() const {
	return _data;
    }

    size_t size() const {
	return _len;
    }

  private:

    std::string _filename;
    char *_data;
    size_t _len;
    unsigned _refcount;
    cache_entry *_next;
    cache_entry *_prev;

    friend class cache;
    
};


class cache { public:

    cache(size_t c)
	: _load(0), _capacity(c), _head(0), _tail(0) {
    }

    void insert(refptr<cache_entry> ce);
    refptr<cache_entry> get(const std::string &fn);

    void empty();
    void remove(cache_entry *);
    void evict();

  private:

    size_t _load;
    size_t _capacity;
    cache_entry *_head;
    cache_entry *_tail;
    
};

void cache_get(const char *filename, tamer::event<refptr<cache_entry> > done);
void clear_cache();

#endif
