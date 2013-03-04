#ifndef TAMER_FD_HH
#define TAMER_FD_HH 1
/* Copyright (c) 2007-2013, Eddie Kohler
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
#include <vector>
namespace tamer {

/** @file <tamer/fd.hh>
 *  @brief  Event-based file descriptor wrapper class.
 */

class fd {
    struct fdimp;

  public:
    typedef ref_ptr<fdimp> fd::*unspecified_bool_type;
    enum { default_backlog = 128 };

    inline fd();
    explicit inline fd(int f);
    inline fd(const fd &f);
    inline ~fd();

    inline fd &operator=(const fd &f);

    static void open(const char *filename, int flags, mode_t mode,
		     event<fd> result);
    static inline void open(const char *filename, int flags, event<fd> result);
    static fd open(const char *filename, int flags, mode_t mode = 0777);

    static fd socket(int domain, int type, int protocol);
    static int pipe(fd &rfd, fd &wfd);

    inline bool valid() const;
    inline operator unspecified_bool_type() const;
    inline bool operator!() const;
    inline int error() const;
    inline int value() const;

    inline void at_close(event<> e);
    event<> closer();
    inline void close(event<int> done);
    inline void close();
    inline void error_close(int errcode);

    void read(void *buf, size_t size, size_t &nread, event<int> done);
    inline void read(void *buf, size_t size, size_t &nread, event<> done);
    inline void read(void *buf, size_t size, event<int> done);
    inline void read(void *buf, size_t size, event<> done);
    void read_once(void *buf, size_t size, size_t &nread, event<int> done);
    inline void read_once(void *buf, size_t size, size_t &nread, event<> done);

    void write(const void *buf, size_t size, size_t &nwritten, event<int> done);
    inline void write(const void *buf, size_t size, size_t &nwritten, event<> done);
    inline void write(const void *buf, size_t size, event<int> done);
    inline void write(const void *buf, size_t size, event<> done);
    void write(std::string buf, size_t &nwritten, event<int> done);
    inline void write(const std::string &buf, size_t &nwritten, event<> done);
    inline void write(const std::string &buf, event<int> done);
    inline void write(const std::string &buf, event<> done);
    void write_once(const void *buf, size_t size, size_t &nwritten, event<int> done);
    inline void write_once(const void *buf, size_t size, size_t &nwritten, event<> done);

    void sendmsg(const void *buf, size_t size, int transfer_fd, event<int> done);
    inline void sendmsg(const void *buf, size_t size, event<int> done);

    void fstat(struct stat &stat, event<int> done);

    int listen(int backlog = default_backlog);
    int bind(const struct sockaddr *addr, socklen_t addrlen);
    void accept(struct sockaddr *addr, socklen_t *addrlen, event<fd> result);
    inline void accept(event<fd> result);
    void connect(const struct sockaddr *addr, socklen_t addrlen,
		 event<int> done);

    static int make_nonblocking(int f);
    static int make_blocking(int f);
    inline int make_nonblocking();

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
#if HAVE_TAMER_FDHELPER
	bool _is_file;
#endif

	fdimp(int fd)
	    : _fd(fd)
#if HAVE_TAMER_FDHELPER
	    , _is_file(false)
#endif
	{
	}
	void full_release() {
	    if (_fd >= 0)
		close();
	}
	int close(int leave_error = -EBADF);
    };

    class closure__accept__P8sockaddrP9socklen_tQ2fd_; void accept(closure__accept__P8sockaddrP9socklen_tQ2fd_ &);
    class closure__connect__PK8sockaddr9socklen_tQi_; void connect(closure__connect__PK8sockaddr9socklen_tQi_ &);
    class closure__read__PvkRkQi_; void read(closure__read__PvkRkQi_ &);
    class closure__read_once__PvkRkQi_; void read_once(closure__read_once__PvkRkQi_ &);
    class closure__write__PKvkRkQi_; void write(closure__write__PKvkRkQi_ &);
    class closure__write__SsRkQi_; void write(closure__write__SsRkQi_ &);
    class closure__write_once__PKvkRkQi_; void write_once(closure__write_once__PKvkRkQi_ &);
    class closure__sendmsg__PKvkiQi_; void sendmsg(closure__sendmsg__PKvkiQi_ &);
    class closure__open__PKci6mode_tQ2fd_; static void open(closure__open__PKci6mode_tQ2fd_ &);

    ref_ptr<fdimp> _p;

    static size_t garbage_size;

    friend bool operator==(const fd &a, const fd &b);
    friend bool operator!=(const fd &a, const fd &b);

};

void tcp_listen(int port, int backlog, event<fd> result);
inline void tcp_listen(int port, event<fd> result);
fd tcp_listen(int port, int backlog);
inline fd tcp_listen(int port);
void tcp_connect(struct in_addr addr, int port, event<fd> result);
void udp_connect(struct in_addr addr, int port, event<fd> result);

struct exec_fd {
    enum fdtype {
	fdtype_newin, fdtype_newout, fdtype_share, fdtype_transfer
    };
    int child_fd;
    fdtype type;
    fd f;
    inline exec_fd(int child_fd, fdtype type, fd f = fd());
};

pid_t exec(std::vector<exec_fd> &exec_fds, const char *program, bool path,
	   const std::vector<const char *> &argv, char * const envp[]);
inline pid_t execv(fd &in, fd &out, const char *program,
		   const std::vector<const char *> &argv);
inline pid_t execv(fd &in, fd &out, fd &err, const char *program,
		   const std::vector<const char *> &argv);
inline pid_t execvp(fd &in, fd &out, const char *program,
		    const std::vector<const char *> &argv);
inline pid_t execvp(fd &in, fd &out, fd &err, const char *program,
		    const std::vector<const char *> &argv);


/** @brief  Construct an invalid file descriptor.
 *
 *  The resulting file descriptor has error() == -EBADF. This error code is
 *  also returned by any file descriptor operation.
 */
inline fd::fd()
    : _p() {
}

/** @brief  Creates a wrapper for the file descriptor @a f.
 *  @param  f  File descriptor value.
 *
 *  Somewhat like the stdio fdopen() function, this constructor takes control
 *  of its file descriptor argument. In particular, when the last reference to
 *  the resulting object is destroyed, @a f is closed automatically.
 *
 *  @a f may be a negative error code, in which case error() will return the
 *  given value.
 */
inline fd::fd(int value)
    : _p(value == -EBADF ? 0 : new fdimp(value)) {
}

/** @brief  Construct another wrapper for file descriptor @a f.
 *  @param  f  Source file descriptor.
 *
 *  The resulting object and the argument @a f both refer to the same file
 *  descriptor. The underlying file descriptor is reference counted, and will
 *  not be automatically destroyed until both @a f and the resulting object go
 *  out of scope.
 */
inline fd::fd(const fd &other)
    : _p(other._p) {
}

/** @brief  Destroy the file descriptor wrapper.
 *  @note   The underlying file descriptor is closed if this was the last
 *          remaining wrapper.
 */
inline fd::~fd() {
}

/** @brief  Assign this file descriptor to refer to @a f.
 *  @param  f  Source file descriptor.
 */
inline fd &fd::operator=(const fd &other) {
    _p = other._p;
    return *this;
}

/** @brief  Open a file descriptor.
 *  @param  filename  File name.
 *  @param  flags     Open flags (@c O_RDONLY, @c O_EXCL, and so forth).
 *  @param  result    Event triggered on completion.
 *
 *  Like open(const char *, int, mode_t, event<fd>), passing 0777 for the @a
 *  mode argument.
 *
 *  @sa open(const char *, int, mode_t, event<fd>)
 */
inline void fd::open(const char *filename, int flags, event<fd> f) {
    open(filename, flags, 0777, f);
}

/** @brief  Test if file descriptor is valid.
 *  @return  True if file descriptor is valid, false if not.
 */
inline bool fd::valid() const {
    return _p && _p->_fd >= 0;
}

/** @brief  Test if file descriptor is valid.
 *  @return  True if file descriptor is valid, false if not.
 */
inline fd::operator unspecified_bool_type() const {
    return _p && _p->_fd >= 0 ? &fd::_p : 0;
}

/** @brief  Test if file descriptor is invalid.
 *  @return  False if file descriptor is valid, true if not.
 */
inline bool fd::operator!() const {
    return !_p || _p->_fd < 0;
}

/** @brief  Check for file descriptor error.
 *  @return  0 if file descriptor is valid, otherwise a negative
 *          error code.
 */
inline int fd::error() const {
    if (_p)
	return (_p->_fd >= 0 ? 0 : _p->_fd);
    else
	return -EBADF;
}

/** @brief  Return file descriptor value.
 *  @return  File descriptor value if file descriptor is valid, otherwise
 *          a negative error code.
 */
inline int fd::value() const {
    return (_p ? _p->_fd : -EBADF);
}

/** @brief  Register a close notifier.
 *  @param  e  Close notifier.
 *
 *  If this file descriptor is invalid, triggers @a e immediately. Otherwise,
 *  triggers @a e when this file descriptor is closed (either by an explicit
 *  close() or as a result of file descriptor references going out of scope).
 */
inline void fd::at_close(event<> e) {
    if (*this)
	_p->_at_close = distribute(_p->_at_close, e);
    else
	e.trigger();
}

/** @brief  Close file descriptor.
 *  @param  done  Event triggered on completion.
 *
 *  @a done is triggered with 0 on success, or a negative error code.
 */
inline void fd::close(event<int> done)
{
    done.trigger(*this ? _p->close() : -EBADF);
}

/** @brief  Close file descriptor.
 */
inline void fd::close()
{
    if (_p)
	_p->close();
}

/** @brief  Accept new connection on listening socket file descriptor.
 *  @param  result  Event triggered on completion.
 *
 *  Equivalent to accept(NULL, NULL, result).
 */
inline void fd::accept(event<fd> result) {
    accept(0, 0, result);
}

inline void fd::read(void *buf, size_t size, size_t &nread, event<> done) {
    read(buf, size, nread, unbind<int>(done));
}

/** @brief  Read from file descriptor.
 *  @param[out]  buf     Buffer.
 *  @param       size    Buffer size.
 *  @param       done    Event triggered on completion.
 *
 *  Reads @a size bytes from the file descriptor, blocking until @a size bytes
 *  are available (or end-of-file or an error condition). Similar to read(void
 *  *, size_t, size_t &, event<int>), but does not return the number of
 *  characters actually read.
 */
inline void fd::read(void *buf, size_t size, event<int> done) {
    read(buf, size, garbage_size, done);
}

inline void fd::read(void *buf, size_t size, event<> done) {
    read(buf, size, unbind<int>(done));
}

inline void fd::read_once(void *buf, size_t size, size_t &nread, event<> done) {
    read_once(buf, size, nread, unbind<int>(done));
}


inline void fd::write(const void *buf, size_t size, size_t &nwritten, event<> done) {
    write(buf, size, nwritten, unbind<int>(done));
}

/** @brief  Write to file descriptor.
 *  @param  buf   Buffer.
 *  @param  size  Buffer size.
 *  @param  done  Event triggered on completion.
 *
 *  Similar to write(const void *, size_t, size_t &, event<int>), but does
 *  not return the number of characters actually written.
 */
inline void fd::write(const void *buf, size_t size, event<int> done) {
    write(buf, size, garbage_size, done);
}

inline void fd::write(const void *buf, size_t size, event<> done) {
    write(buf, size, unbind<int>(done));
}

inline void fd::write(const std::string &buf, size_t &nwritten, event<> done) {
    write(buf, nwritten, unbind<int>(done));
}

/** @brief  Write string to file descriptor.
 *  @param  buf   Buffer.
 *  @param  done  Event triggered on completion.
 *
 *  Equivalent to write(buf.data(), buf.length(), done).
 */
inline void fd::write(const std::string &buf, event<int> done) {
    write(buf, garbage_size, done);
}

inline void fd::write(const std::string &buf, event<> done) {
    write(buf, unbind<int>(done));
}

inline void fd::write_once(const void *buf, size_t size, size_t &nwritten, event<> done) {
    write_once(buf, size, nwritten, unbind<int>(done));
}


/** @overload */
inline void fd::sendmsg(const void *buf, size_t size, event<int> done) {
    sendmsg(buf, size, -1, done);
}

/** @brief  Close file descriptor, marking it with an error.
 *  @param  errcode  Negative error code.
 *
 *  After error_close(@a errcode), the valid() function will return false,
 *  and error() will return @a errcode.
 */
inline void fd::error_close(int errcode) {
    assert(errcode < 0);
    if (_p)
	_p->close(errcode);
    else if (errcode != -EBADF)
	_p = ref_ptr<fdimp>(new fdimp(errcode));
}

/** @brief  Make this file descriptor use nonblocking I/O.
 */
inline int fd::make_nonblocking() {
    return make_nonblocking(_p ? _p->_fd : -EBADF);
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
inline void tcp_listen(int port, int backlog, event<fd> result) {
    result.trigger(tcp_listen(port, backlog));
}

/** @brief  Open a nonblocking TCP connection on port @a port.
 *  @param  port     Listening port (in host byte order).
 *  @param  result   Event triggered on completion.
 *
 *  Equivalent to tcp_listen(port, fd::default_backlog, result).
 */
inline void tcp_listen(int port, event<fd> result) {
    tcp_listen(port, fd::default_backlog, result);
}

/** @brief  Open a nonblocking TCP connection on port @a port.
 *  @param  port     Listening port (in host byte order).
 *  @return  File descriptor.
 *
 *  Equivalent to tcp_listen(port, fd::default_backlog).
 */
inline fd tcp_listen(int port) {
    return tcp_listen(port, fd::default_backlog);
}

inline exec_fd::exec_fd(int child_fd, fdtype type, fd f)
    : child_fd(child_fd), type(type), f(f) {
}

inline pid_t execv(fd &in, fd &out, const char *program,
		   const std::vector<const char *> &argv) {
    std::vector<exec_fd> efd;
    efd.push_back(exec_fd(STDIN_FILENO, exec_fd::fdtype_newin));
    efd.push_back(exec_fd(STDOUT_FILENO, exec_fd::fdtype_newout));
    pid_t r = exec(efd, program, false, argv, 0);
    in = efd[0].f;
    out = efd[1].f;
    return r;
}

inline pid_t execv(fd &in, fd &out, fd &err, const char *program,
		   const std::vector<const char *> &argv) {
    std::vector<exec_fd> efd;
    efd.push_back(exec_fd(STDIN_FILENO, exec_fd::fdtype_newin));
    efd.push_back(exec_fd(STDOUT_FILENO, exec_fd::fdtype_newout));
    efd.push_back(exec_fd(STDERR_FILENO, exec_fd::fdtype_newout));
    pid_t r = exec(efd, program, false, argv, 0);
    in = efd[0].f;
    out = efd[1].f;
    err = efd[2].f;
    return r;
}

inline pid_t execvp(fd &in, fd &out, const char *program,
		    const std::vector<const char *> &argv) {
    std::vector<exec_fd> efd;
    efd.push_back(exec_fd(STDIN_FILENO, exec_fd::fdtype_newin));
    efd.push_back(exec_fd(STDOUT_FILENO, exec_fd::fdtype_newout));
    pid_t r = exec(efd, program, true, argv, 0);
    in = efd[0].f;
    out = efd[1].f;
    return r;
}

inline pid_t execvp(fd &in, fd &out, fd &err, const char *program,
		    const std::vector<const char *> &argv) {
    std::vector<exec_fd> efd;
    efd.push_back(exec_fd(STDIN_FILENO, exec_fd::fdtype_newin));
    efd.push_back(exec_fd(STDOUT_FILENO, exec_fd::fdtype_newout));
    efd.push_back(exec_fd(STDERR_FILENO, exec_fd::fdtype_newout));
    pid_t r = exec(efd, program, true, argv, 0);
    in = efd[0].f;
    out = efd[1].f;
    err = efd[2].f;
    return r;
}

} // namespace tamer
#endif /* TAMER_FD_HH */
