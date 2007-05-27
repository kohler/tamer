#ifndef TAMER_FD_HH
#define TAMER_FD_HH 1
#include <tamer/tamer.hh>
#include <tamer/lock.hh>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
namespace tamer {

class fd {

    struct fdimp;

  public:

    inline fd();
    explicit inline fd(int value);
    inline fd(const fd &other);
    inline ~fd();

    static int make_nonblocking(int fd);
    static fd socket(int domain, int type, int protocol);

    typedef fdimp *fd::*unspecified_bool_type;
    inline operator unspecified_bool_type() const;
    inline bool operator!() const;

    inline int error() const;
    inline int value() const;

    inline void at_close(event<> e);

    void read(void *buf, size_t size, size_t &nread, event<int> done);
    void write(const void *buf, size_t size, size_t &nwritten, event<int> done);
    inline void write(const void *buf, size_t size, const event<int> &done);
    void write(std::string buf, size_t &nwritten, event<int> done);
    inline void write(const std::string &buf, const event<int> &done);

    int listen(int backlog = 32);
    void accept(struct sockaddr *name, socklen_t *namelen, fd &newfd, event<int> done);
    void connect(const struct sockaddr *name, socklen_t namelen, event<int> done);
    
    void close(event<int> done);
    inline void close();
    
    inline fd &operator=(const fd &other);
    
  private:

    struct fdimp {
	int fd;
	unsigned refcount;
	mutex rlock;
	mutex wlock;
	event<> at_close;
	fdimp(int fd_) : fd(fd_), refcount(1) { }
    };

    fdimp *_p;

    class closure__read__PvkRkQ; void read(closure__read__PvkRkQ &, unsigned);
    class closure__write__PKvkRkQ; void write(closure__write__PKvkRkQ &, unsigned);
    class closure__write__SsRkQ; void write(closure__write__SsRkQ &, unsigned);
    class closure__accept__P8sockaddrP9socklen_tR2fdQ; void accept(closure__accept__P8sockaddrP9socklen_tR2fdQ &, unsigned);
    class closure__connect__PK8sockaddr9socklen_tQ; void connect(closure__connect__PK8sockaddr9socklen_tQ &, unsigned);

    friend bool operator==(const fd &, const fd &);
    friend bool operator!=(const fd &, const fd &);

};

inline fd::fd()
    : _p(0) {
}

inline fd::fd(int value)
    : _p(value == -EBADF ? 0 : new fdimp(value)) {
}

inline fd::fd(const fd &other)
    : _p(other._p) {
    if (_p)
	++_p->refcount;
}

inline fd::~fd() {
    if (_p && --_p->refcount == 0)
	close();
}

inline fd &fd::operator=(const fd &other) {
    if (other._p)
	++other._p->refcount;
    if (_p && --_p->refcount == 0)
	close();
    _p = other._p;
    return *this;
}

inline fd::operator unspecified_bool_type() const {
    return _p && _p->fd >= 0 ? &fd::_p : 0;
}

inline bool fd::operator!() const {
    return !_p || _p->fd < 0;
}

inline int fd::error() const {
    if (_p)
	return (_p->fd >= 0 ? 0 : _p->fd);
    else
	return -EBADF;
}

inline int fd::value() const {
    return (_p ? _p->fd : -EBADF);
}

inline void fd::at_close(event<> e) {
    if (!*this)
	e.trigger();
    else
	_p->at_close = distribute(_p->at_close, e);
}

inline void fd::write(const void *buf, size_t size, const event<int> &done) {
    write(buf, size, *(size_t *) 0, done);
}

inline void fd::write(const std::string &buf, const event<int> &done) {
    write(buf, *(size_t *) 0, done);
}

inline void fd::close() {
    close(event<int>());
}

inline bool operator==(const fd &a, const fd &b) {
    return a._p == b._p;
}

inline bool operator!=(const fd &a, const fd &b) {
    return a._p != b._p;
}

}
#endif /* TAMER_FD_HH */
