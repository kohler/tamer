#ifndef TAMER_FD_HH
#define TAMER_FD_HH 1
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
#include <tamer/tamer.hh>
#include <tamer/lock.hh>
#include <tamer/ref.hh>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
namespace tamer {

/** @file <tamer/fd.hh>
 *  @brief  Event-based file descriptor wrapper class.
 */

/** @class fd tamer/fd.hh <tamer/fd.hh>
 *  @brief  A file descriptor wrapper with event-based access functions.
 *
 *  The fd class wraps file descriptors in a convenient interface for Tamer
 *  event-based programming.  Its methods resemble Unix system calls,
 *  but adapted for Tamer events.
 *
 *  fd wrappers are reference-counted and may be freely passed as arguments,
 *  copied, assigned, and destroyed.  Many fd wrappers may refer to the same
 *  underlying file descriptor.  This file descriptor is closed when the last
 *  wrapper to reference it is destroyed.  Alternately, the close() member
 *  function explicitly closes the underlying file descriptor.
 *
 *  When a file descriptor is closed, any pending tamer::at_fd_read() and
 *  tamer::at_fd_write() events are canceled, and any pending read(), write(),
 *  accept(), connect(), and similar pending fd methods will terminate with
 *  the @c -ECANCELED error code (or, equivalently, tamer::outcome::cancel).
 *  Any fd methods on a closed file descriptor return the @c -EBADF error code.
 *
 *  The fd object ensures that reads complete in the order they are called,
 *  and similarly for writes.  Thus, the following code:
 *
 *  @code
 *     tamed void greeting(tamer::fd f) {
 *         tvars {
 *             int ret;
 *         }
 *         twait {
 *             f.write("Hello, ", make_event(ret));
 *             f.write("world", make_event(ret));
 *             f.write("!", make_event(ret));
 *         }
 *     }
 *  @endcode
 *
 *  will always output "<code>Hello, world!</code>", even though the three
 *  <code>f.write()</code> calls hypothetically happen in parallel.
 */
class fd {

    struct fdimp;

  public:

    /** @brief  Default constructor creates an invalid file descriptor.
     *
     *  The resulting file descriptor returns an error() of -EBADF.  This
     *  error code is also returned by any file descriptor operation.
     */
    inline fd();

    /** @brief  Creates a wrapper for the file descriptor @a f.
     *  @param  f  File descriptor value.
     *
     *  Somewhat like the stdio fdopen() function, this constructor takes
     *  control of its file descriptor argument.  In particular, when the
     *  last reference to the resulting object is destroyed, @a f is closed
     *  automatically.
     *
     *  @a f may be a negative error code, in which case error() will return
     *  the given value.
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
     *  @param  f  File descriptor value.
     *  @note   This function's argument is a file descriptor value, not an
     *          fd wrapper object.  Particularly useful for e.g.
     *          standard input.
     */
    static int make_nonblocking(int f);

    /** @brief  Open a file descriptor.
     *  @param  filename  File name.
     *  @param  flags     Open flags (@c O_RDONLY, @c O_EXCL, and so forth).
     *  @param  mode      Permissions mode (used when creating a file).
     *  @param  result    Event triggered on completion.
     *
     *  Opens a file descriptor for the named file, returning it via the
     *  @a result event.  The returned file descriptor is made nonblocking.
     *  To check whether the open succeeded, use valid() or error() on the
     *  resulting file descriptor.
     *
     *  @sa open(const char *, int, event<fd>)
     */
    static void open(const char *filename, int flags, mode_t mode,
		     event<fd> result);
    
    /** @brief  Open a file descriptor.
     *  @param  filename  File name.
     *  @param  flags     Open flags (@c O_RDONLY, @c O_EXCL, and so forth).
     *  @param  result    Event triggered on completion.
     *
     *  Like open(const char *, int, mode_t, event<fd>), passing 0777
     *  for the @a mode argument.
     *
     *  @sa open(const char *, int, mode_t, event<fd>)
     */
    static inline void open(const char *filename, int flags, event<fd> result);

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
    
    typedef ref_ptr<fdimp> fd::*unspecified_bool_type;

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
     *  Otherwise, triggers @a e when this file descriptor is closed (either
     *  by an explicit close() or as a result of file descriptor references
     *  going out of scope).
     */
    inline void at_close(event<> e);

    /** @brief  Return a closer event.
     *  @return  Closer event.
     *
     *  Triggering the returned event will immediately close the file
     *  descriptor.  Returns an empty event if the file descriptor is invalid.
     */
    event<> closer();

    /** @brief  Fetch file status.
     *  @param[out]  stat  Status structure.
     *  @param       done  Event triggered on completion.
     *
     *  @a done is triggered with 0 on success, or a negative error code.
     */
    void fstat(struct stat &stat, event<int> done);

    enum { default_backlog = 128 };

    /** @brief  Set socket file descriptor for listening.
     *  @param  backlog  Maximum length of pending connection queue.
     *
     *  Returns 0 on success, or a negative error code.
     *
     *  @sa tamer::fdx::tcp_listen
     */
    int listen(int backlog = default_backlog);

    /** @brief  Bind socket file descriptor to local address.
     *  @param  addr     Local socket address.
     *  @param  addrlen  Length of local socket address.
     *
     *  Returns 0 on success, or a negative error code.
     */
    int bind(const struct sockaddr *addr, socklen_t addrlen);

    /** @brief  Accept new connection on listening socket file descriptor.
     *  @param[out]     addr     Socket address of connecting client.
     *  @param[in,out]  addrlen  Length of @a addr.
     *  @param          result   Event triggered on completion.
     *
     *  Accepts a new connection on a listening socket, returning it via the
     *  @a result event.  The returned file descriptor is made nonblocking.
     *  To check whether the accept succeeded, use valid() or error() on the
     *  resulting file descriptor.
     *
     *  If @a addr is not null, it is filled in with the connecting client's
     *  address.  On input, @a addrlen should equal the space available for @a
     *  addr; on output, it is set to the space used for @a addr.
     *
     *  @sa accept(const event<fd> &)
     */
    inline void accept(struct sockaddr *addr, socklen_t *addrlen,
		       event<fd> result);

    /** @brief  Accept new connection on listening socket file descriptor.
     *  @param  result  Event triggered on completion.
     *
     *  Equivalent to accept(NULL, NULL, result).
     */
    inline void accept(const event<fd> &result);

    /** @brief  Connect socket file descriptor.
     *  @param  addr     Remote address.
     *  @param  addrlen  Length of remote address.
     *  @param  done     Event triggered on completion.
     *
     *  @a done is triggered with 0 on success, or a negative error code.
     *
     *  @sa tamer::fdx::tcp_connect
     */
    inline void connect(const struct sockaddr *addr, socklen_t addrlen,
			event<int> done);
    
    /** @brief  Read from file descriptor.
     *  @param[out]  buf     Buffer.
     *  @param       size    Buffer size.
     *  @param[out]  nread   Number of characters read.
     *  @param       done    Event triggered on completion.
     *
     *  Reads @a size bytes from the file descriptor, blocking until @a size
     *  bytes are available (or end-of-file or an error condition).  @a done
     *  is triggered with 0 on success or end-of-file, or a negative error
     *  code.  @a nread is kept up to date as the read progresses.
     *
     *  @sa read_once(void *, size_t, size_t &, event<int>)
     */
    inline void read(void *buf, size_t size, size_t &nread, event<int> done);

    /** @brief  Read from file descriptor.
     *  @param[out]  buf     Buffer.
     *  @param       size    Buffer size.
     *  @param       done    Event triggered on completion.
     *
     *  Reads @a size bytes from the file descriptor, blocking until @a size
     *  bytes are available (or end-of-file or an error condition).  Similar
     *  to read(void *, size_t, size_t &, event<int>), but does not return the
     *  number of characters actually read.
     */
    inline void read(void *buf, size_t size, const event<int> &done);
    
    /** @brief  Read once from file descriptor.
     *  @param[out]  buf     Buffer.
     *  @param       size    Buffer size.
     *  @param[out]  nread   Number of characters read.
     *  @param       done    Event triggered on completion.
     *
     *  Reads at most @a size bytes from the file descriptor.  Blocks until at
     *  least one byte is read (or end-of-file or an error condition), but
     *  unlike read(), @a done is triggered after the @em first successful
     *  read, even if less than @a size bytes are read.  @a done is triggered
     *  with 0 on success, or a negative error code.  @a nread is kept up to
     *  date as the read progresses.
     *
     *  @sa read(void *, size_t, size_t &, event<int>)
     */
    inline void read_once(void *buf, size_t size, size_t &nread,
			  event<int> done);

    /** @brief  Write to file descriptor.
     *  @param       buf       Buffer.
     *  @param       size      Buffer size.
     *  @param[out]  nwritten  Number of characters written.
     *  @param       done      Event triggered on completion.
     *
     *  @a done is triggered with 0 on success or end-of-file, or a negative
     *  error code.  @a nwritten is kept up to date as the write progresses.
     *
     *  @sa write(std::string, size_t, event<int>)
     */
    inline void write(const void *buf, size_t size, size_t &nwritten,
		      event<int> done);

    /** @brief  Write to file descriptor.
     *  @param  buf   Buffer.
     *  @param  size  Buffer size.
     *  @param  done  Event triggered on completion.
     *
     *  Similar to write(const void *, size_t, size_t &, event<int>), but does
     *  not return the number of characters actually written.
     */
    inline void write(const void *buf, size_t size, const event<int> &done);

    /** @brief  Write string to file descriptor.
     *  @param       buf       Buffer.
     *  @param[out]  nwritten  Number of characters written.
     *  @param       done      Event triggered on completion.
     *
     *  Equivalent to write(buf.data(), buf.length(), nwritten, done).
     */
    inline void write(std::string buf, size_t &nwritten, event<int> done);

    /** @brief  Write string to file descriptor.
     *  @param  buf   Buffer.
     *  @param  done  Event triggered on completion.
     *
     *  Equivalent to write(buf.data(), buf.length(), done).
     */
    inline void write(const std::string &buf, const event<int> &done);

    /** @brief  Close file descriptor.
     *  @param  done  Event triggered on completion.
     *
     *  @a done is triggered with 0 on success, or a negative error code.
     */
    void close(event<int> done);

    /** @brief  Close file descriptor.
     *
     *  Equivalent to close(event<int>()).
     */
    inline void close();

    /** @brief  Close file descriptor, marking it with an error.
     *  @param  errcode  Negative error code.
     *
     *  After error_close(@a errcode), the valid() function will return false,
     *  and error() will return @a errcode.
     */
    inline void error_close(int errcode);

    /** @brief  Assign this file descriptor to refer to @a f.
     *  @param  f  Source file descriptor.
     */
    inline fd &operator=(const fd &f);
    
  private:

    struct fdcloser {
	fdcloser(fd::fdimp *f)
	    : _f(f) {
	}
	fdcloser(const ref_ptr<fd::fdimp> &f)
	    : _f(f) {
	}
	void operator()() {
	    _f->close();
	}
	passive_ref_ptr<fd::fdimp> _f;
    };
    
    struct fdimp : public enable_ref_ptr_with_full_release<fdimp> {

	int _fd;
	mutex _rlock;
	mutex _wlock;
	event<> _at_close;
	
	fdimp(int fd)
	    : _fd(fd) {
	}
	
	void accept(struct sockaddr *addr, socklen_t *addrlen,
		    event<fd> result);
	void connect(const struct sockaddr *addr, socklen_t addrlen,
		     event<int> done);
	void read(void *buf, size_t size, size_t &nread, event<int> done);
	void read_once(void *buf, size_t size, size_t &nread, event<int> done);
	void write(const void *buf, size_t size, size_t &nwritten,
		   event<int> done);
	void write(std::string buf, size_t &nwritten, event<int> done);
	void full_release() {
	    if (_fd >= 0)
		close();
	}
	int close(int leave_error = -EBADF);
	
      private:
	
	class closure__accept__P8sockaddrP9socklen_tQ2fd_; void accept(closure__accept__P8sockaddrP9socklen_tQ2fd_ &, unsigned);
	class closure__connect__PK8sockaddr9socklen_tQi_; void connect(closure__connect__PK8sockaddr9socklen_tQi_ &, unsigned);
	class closure__read__PvkRkQi_; void read(closure__read__PvkRkQi_ &, unsigned);
	class closure__read_once__PvkRkQi_; void read_once(closure__read_once__PvkRkQi_ &, unsigned);
	class closure__write__PKvkRkQi_; void write(closure__write__PKvkRkQi_ &, unsigned);
	class closure__write__SsRkQi_; void write(closure__write__SsRkQi_ &, unsigned);

    };

    struct fdcloser;
    
    ref_ptr<fdimp> _p;

    static size_t garbage_size;

    friend bool operator==(const fd &, const fd &);
    friend bool operator!=(const fd &, const fd &);

};

inline fd::fd()
    : _p() {
}

inline fd::fd(int value)
    : _p(value == -EBADF ? 0 : new fdimp(value)) {
}

inline fd::fd(const fd &other)
    : _p(other._p) {
}

inline fd::~fd() {
}

inline fd &fd::operator=(const fd &other) {
    _p = other._p;
    return *this;
}

inline void fd::open(const char *filename, int flags, event<fd> f) {
    open(filename, flags, 0777, f);
}

inline fd::operator unspecified_bool_type() const {
    return _p && _p->_fd >= 0 ? &fd::_p : 0;
}

inline bool fd::valid() const {
    return _p && _p->_fd >= 0;
}

inline bool fd::operator!() const {
    return !_p || _p->_fd < 0;
}

inline int fd::error() const {
    if (_p)
	return (_p->_fd >= 0 ? 0 : _p->_fd);
    else
	return -EBADF;
}

inline int fd::value() const {
    return (_p ? _p->_fd : -EBADF);
}

inline void fd::at_close(event<> e) {
    if (*this)
	_p->_at_close = distribute(_p->_at_close, e);
    else
	e.trigger();
}

inline void fd::accept(struct sockaddr *addr, socklen_t *addrlen, event<fd> result) {
    if (_p)
	_p->accept(addr, addrlen, result);
    else
	result.trigger(fd());
}

inline void fd::accept(const event<fd> &result) {
    accept(0, 0, result);
}

inline void fd::connect(const struct sockaddr *addr, socklen_t addrlen, event<int> done) {
    if (_p)
	_p->connect(addr, addrlen, done);
    else
	done.trigger(-EBADF);
}

inline void fd::read(void *buf, size_t size, size_t &nread, event<int> done) {
    if (_p)
	_p->read(buf, size, nread, done);
    else
	done.trigger(-EBADF);
}

inline void fd::read(void *buf, size_t size, const event<int> &done) {
    read(buf, size, garbage_size, done);
}

inline void fd::read_once(void *buf, size_t size, size_t &nread, event<int> done) {
    if (_p)
	_p->read_once(buf, size, nread, done);
    else
	done.trigger(-EBADF);
}

inline void fd::write(const void *buf, size_t size, size_t &nwritten, event<int> done) {
    if (_p)
	_p->write(buf, size, nwritten, done);
    else
	done.trigger(-EBADF);
}

inline void fd::write(const void *buf, size_t size, const event<int> &done) {
    write(buf, size, garbage_size, done);
}

inline void fd::write(std::string buf, size_t &nwritten, event<int> done) {
    if (_p)
	_p->write(buf, nwritten, done);
    else
	done.trigger(-EBADF);
}	

inline void fd::write(const std::string &buf, const event<int> &done) {
    write(buf, garbage_size, done);
}

inline void fd::close() {
    if (*this)
	_p->close();
}

inline void fd::error_close(int errcode) {
    assert(errcode < 0);
    if (_p)
	_p->close(errcode);
    else if (errcode != -EBADF)
	_p = ref_ptr<fdimp>(new fdimp(errcode));
}

/** @brief  Test whether two file descriptors refer to the same object.
 *  @param  a  First file descriptor.
 *  @param  b  Second file descriptor.
 *  @return  True iff @a a and @a b refer to the same file descriptor.
 */
inline bool operator==(const fd &a, const fd &b) {
    return a._p == b._p;
}

/** @brief  Test whether two file descriptors refer to the same object.
 *  @param  a  First file descriptor.
 *  @param  b  Second file descriptor.
 *  @return  True iff @a a and @a b do not refer to the same file descriptor.
 */
inline bool operator!=(const fd &a, const fd &b) {
    return a._p != b._p;
}


namespace fdx {

/** @namespace tamer::fdx
 *  @brief  Namespace containing extensions to Tamer's file descriptor
 *  support, such as helper functions for creating TCP connections.
 */

/** @brief  Open a nonblocking TCP connection on port @a port.
 *  @param  port     Listening port (in host byte order).
 *  @param  backlog  Maximum connection backlog.
 *  @param  result   Event triggered on completion.
 *
 *  Returns the new listening file descriptor via the @a result event.  The
 *  returned file descriptor is made nonblocking, and is opened with the @c
 *  SO_REUSEADDR option.  To check whether the function succeeded, use valid()
 *  or error() on the resulting file descriptor.
 */
void tcp_listen(int port, int backlog, event<fd> result);


/** @brief  Open a nonblocking TCP connection on port @a port.
 *  @param  port     Listening port (in host byte order).
 *  @param  result   Event triggered on completion.
 *
 *  Equivalent to tcp_listen(port, fd::default_backlog, result).
 */
inline void tcp_listen(int port, event<fd> result) {
    tcp_listen(port, fd::default_backlog, result);
}


/** @brief  Create a nonblocking TCP connection to @a addr:@a port.
 *  @param  addr    Remote host.
 *  @param  port    Remote port (in host byte order).
 *  @param  result  Event triggered on completion.
 *
 *  Returns the connected file descriptor via the @a result event.  The
 *  returned file descriptor is made nonblocking.  To check whether the
 *  connect attempt succeeded, use valid() or error() on the resulting file
 *  descriptor.
 */
void tcp_connect(struct in_addr addr, int port, event<fd> result);

void udp_connect(struct in_addr addr, int port, event<fd> result);

}}
#endif /* TAMER_FD_HH */
