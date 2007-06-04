#ifndef TAMER_FD_HH
#define TAMER_FD_HH 1
#include <tamer/tamer.hh>
#include <tamer/lock.hh>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
namespace tamer {

/** @file <tamer/fd.hh>
 *  @brief  Event-based file descriptor wrapper class.
 */

/** @class fd tamer/fd.hh <tamer/fd.hh>
 *  @brief  A file descriptor wrapper with event-based access functions.
 */
class fd {

    struct fdimp;

  public:

    /** @brief  Default constructor creates an invalid file descriptor.
     *
     *  The resulting file descriptor returns an error() of -EBADF.
     */
    inline fd();

    /** @brief  Creates a wrapper for the file descriptor @a f.
     *  @param  f  File descriptor value.
     *
     *  Somewhat like the stdio fdopen() function, this constructor takes
     *  control of its file descriptor argument.  In particular, when the
     *  last reference to the resulting object is destroyed, @a f is closed
     *  automatically.
     */
    explicit inline fd(int f);

    /** @brief  Construct another wrapper for file descriptor @a f.
     *  @param  f  Source file descriptor.
     *
     *  The resulting object and the argument @a f both refer to the same file
     *  descriptor.  The underlying file descriptor is reference counted, and
     *  will not be automatically destroyed until both @a f and the resulting
     *  object go out of scope.
     */
    inline fd(const fd &f);

    /** @brief  Destroy the file descriptor wrapper.
     *  @note   The underlying file descriptor is closed if this was the last
     *          remaining wrapper.
     */
    inline ~fd();

    /** @brief  Make a file descriptor use nonblocking I/O.
     *  @param  f  File descriptor.
     *  @note   This function's argument is a file descriptor value, not an
     *          fd wrapper object.  Particularly useful for e.g.
     *          standard input.
     */
    static int make_nonblocking(int f);

    /** @brief  Open a file descriptor.
     *  @param  filename  File name.
     *  @param  flags     Open flags (O_RDONLY, O_RDWR, and so forth).
     *  @param  mode      Open mode (used when creating a file).
     *  @param  result    Event triggered on completion.
     *
     *  Opens a file descriptor for the named file, returning it via the
     *  @a result event.  The returned file descriptor is made nonblocking.
     *  To check whether the open succeeded, use valid() or error().
     *
     *  @sa open(const char *, int, event<fd>)
     */
    static void open(const char *filename, int flags, mode_t mode,
		     event<fd> result);
    
    /** @brief  Open a file descriptor.
     *  @param  filename  File name.
     *  @param  flags     Open flags (O_RDONLY, O_RDWR, and so forth).
     *  @param  result    Event triggered on completion.
     *
     *  Like open(const char *, int, mode_t, event<fd>), passing 0777
     *  for the @a mode argument.
     *
     *  @sa open(const char *, int, mode_t, event<fd>)
     */
    static inline void open(const char *filename, int flags,
			    const event<fd> &result);

    /** @brief  Create a socket file descriptor.
     *  @param  domain    Socket domain.
     *  @param  type      Socket type.
     *  @param  protocol  Socket protocol.
     *
     *  The returned socket is made nonblocking.  Use valid() or error() to
     *  check for errors.
     */
    static fd socket(int domain, int type, int protocol);

    /** @brief  Test if file descriptor is valid.
     *  @return  True if file descriptor is valid, false if not.
     */
    inline bool valid() const;
    
    typedef fdimp *fd::*unspecified_bool_type;

    /** @brief  Test if file descriptor is valid.
     *  @return  True if file descriptor is valid, false if not.
     */
    inline operator unspecified_bool_type() const;

    /** @brief  Test if file descriptor is invalid.
     *  @return  False if file descriptor is valid, true if not.
     */
    inline bool operator!() const;

    /** @brief  Check for file descriptor error.
     *  @return  0 if file descriptor is valid, otherwise a negative
     *          error code.
     */
    inline int error() const;
    
    /** @brief  Return file descriptor value.
     *  @return  File descriptor value if file descriptor is valid, otherwise
     *          a negative error code.
     */
    inline int value() const;

    /** @brief  Register a close notifier.
     *  @param  e  Close notifier.
     *
     *  If this file descriptor is invalid, triggers @a e immediately.
     *  Otherwise, triggers @a e when this file descriptor is closed
     *  (either by close() or as a result of references going out of scope).
     */
    inline void at_close(event<> e);

    /** @brief  Return a closer event.
     *  @return  Closer event.
     *
     *  Triggering the returned event will immediately close the file
     *  descriptor.  Returns an empty event if the file descriptor is invalid.
     */
    event<> closer();

    void fstat(struct stat &stat_out, event<int> done);

    void read(void *buf, size_t size, size_t &nread, event<int> done);
    inline void read(void *buf, size_t size, const event<int> &done);

    void write(const void *buf, size_t size, size_t &nwritten, event<int> done);
    inline void write(const void *buf, size_t size, const event<int> &done);
    void write(std::string buf, size_t &nwritten, event<int> done);
    inline void write(const std::string &buf, const event<int> &done);

    int listen(int backlog = 32);
    void accept(struct sockaddr *name, socklen_t *namelen, event<fd> done);
    void connect(const struct sockaddr *name, socklen_t namelen, event<int> done);
    
    void close(event<int> done);
    inline int close();
    
    inline fd &operator=(const fd &other);
    
  private:

    struct fdimp {

	int fd;
	mutex rlock;
	mutex wlock;
	event<> at_close;
	
	fdimp(int fd_)
	    : fd(fd_), _refcount(1), _weak_refcount(0) {
	}
	
	void use() {
	    ++_refcount;
	}
	
	void unuse() {
	    if (--_refcount == 0) {
		if (fd >= 0)
		    close();
		if (_weak_refcount == 0)
		    delete this;
	    }
	}
	
	void weak_use() {
	    ++_weak_refcount;
	}
	
	void weak_unuse() {
	    if (--_weak_refcount == 0 && _refcount == 0)
		delete this;
	}
	
	int close();

	struct closer;
	
      private:

	unsigned _refcount;
	unsigned _weak_refcount;
	
    };

    struct fdcloser;
    
    fdimp *_p;

    static size_t garbage_size;

    class closure__read__PvkRkQi_; void read(closure__read__PvkRkQi_ &, unsigned);
    class closure__write__PKvkRkQi_; void write(closure__write__PKvkRkQi_ &, unsigned);
    class closure__write__SsRkQi_; void write(closure__write__SsRkQi_ &, unsigned);
    class closure__accept__P8sockaddrP9socklen_tQ2fd_; void accept(closure__accept__P8sockaddrP9socklen_tQ2fd_ &, unsigned);
    class closure__connect__PK8sockaddr9socklen_tQi_; void connect(closure__connect__PK8sockaddr9socklen_tQi_ &, unsigned);

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
	_p->use();
}

inline fd::~fd() {
    if (_p)
	_p->unuse();
}

inline fd &fd::operator=(const fd &other) {
    if (other._p)
	other._p->use();
    if (_p)
	_p->unuse();
    _p = other._p;
    return *this;
}

inline void fd::open(const char *filename, int flags, const event<fd> &f) {
    open(filename, flags, 0777, f);
}

inline fd::operator unspecified_bool_type() const {
    return _p && _p->fd >= 0 ? &fd::_p : 0;
}

inline bool fd::valid() const {
    return _p && _p->fd >= 0;
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

inline void fd::read(void *buf, size_t size, const event<int> &done) {
    read(buf, size, garbage_size, done);
}

inline void fd::write(const void *buf, size_t size, const event<int> &done) {
    write(buf, size, garbage_size, done);
}

inline void fd::write(const std::string &buf, const event<int> &done) {
    write(buf, garbage_size, done);
}

inline int fd::close() {
    return (*this ? _p->close() : -EBADF);
}

inline bool operator==(const fd &a, const fd &b) {
    return a._p == b._p;
}

inline bool operator!=(const fd &a, const fd &b) {
    return a._p != b._p;
}

}
#endif /* TAMER_FD_HH */
