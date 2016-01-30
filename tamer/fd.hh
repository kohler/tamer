#ifndef TAMER_FD_HH
#define TAMER_FD_HH 1
/* Copyright (c) 2007-2015, Eddie Kohler
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
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <vector>
#include <string>
namespace tamer {

/** @file <tamer/fd.hh>
 *  @brief  Event-based file descriptor wrapper class.
 */

class fd {
    struct fdimp;

  public:
    typedef bool (fd::*unspecified_bool_type)() const;
    enum { default_backlog = 128 };

    inline fd();
    explicit inline fd(int f);
    inline fd(const fd& f);
    inline fd(fd&& f);
    inline ~fd();

    inline fd& operator=(const fd& f);
    inline fd& operator=(fd&& f);

    static void open(const char* filename, int flags, mode_t mode,
                     event<fd> result);
    static inline void open(const char* filename, int flags, event<fd> result);
    static fd open(const char* filename, int flags, mode_t mode = 0777);

    static fd socket(int domain, int type, int protocol);
    static int pipe(fd& rfd, fd& wfd);
    static inline int pipe(fd pfd[2]);

    inline bool valid() const;
    inline operator unspecified_bool_type() const;
    inline bool operator!() const;
    inline int error() const;
    inline int fdnum() const;
    inline int recent_fdnum() const;

    inline void at_close(event<> e);
    event<> closer();
    inline void close(event<int> done);
    inline void close();
    inline void close(int errcode);
    inline void error_close(int errcode);

    void read(void* buf, size_t size, event<size_t, int> done);
    void read(struct iovec* iov, int iov_count, event<size_t, int> done);
    void read_once(void* buf, size_t size, event<size_t, int> done);

    void write(const void* buf, size_t size, event<size_t, int> done);
    void write(struct iovec* iov, int iov_count, event<size_t, int> done);
    void write_once(const void* buf, size_t size, event<size_t, int> done);

    void sendmsg(const void* buf, size_t size, int transfer_fd, event<int> done);

    void fstat(struct stat& stat, event<int> done);

    int listen(int backlog = default_backlog);
    int bind(const struct sockaddr* addr, socklen_t addrlen);
    void accept(struct sockaddr* addr, socklen_t* addrlen, event<fd> result);
    inline void accept(event<fd> result);
    void connect(const struct sockaddr* addr, socklen_t addrlen,
                 event<int> done);
    inline int shutdown(int how);
    inline int socket_error() const;

    ssize_t direct_read(void* buf, size_t size);
    ssize_t direct_write(const void* buf, size_t size);

    static int open_limit();
    static int open_limit(int n);

    static int make_nonblocking(int f);
    static int make_blocking(int f);
    inline int make_nonblocking();

  private:
    struct fdimp {
        int fde_;
        int fdv_;
        event<> _at_close;
#if HAVE_TAMER_FDHELPER
        bool _is_file;
#endif
        unsigned ref_count_;
        unsigned weak_count_;

        fdimp(int fd)
            : fde_(fd < 0 ? fd : 0), fdv_(fd)
#if HAVE_TAMER_FDHELPER
            , _is_file(false)
#endif
            , ref_count_(1), weak_count_(0) {
        }
        void deref() {
            if (!--ref_count_)
                close();
            if (!ref_count_ && !weak_count_)
                delete this;
        }
        void weak_deref() {
            if (!--weak_count_ && !ref_count_)
                delete this;
        }
        int close(int leave_error = -EBADF);
    };

    struct fdcloser {
        fdcloser(fd::fdimp* imp)
            : imp_(imp) {
            ++imp_->weak_count_;
        }
        ~fdcloser() {
            imp_->weak_deref();
        }
        void operator()() {
            imp_->close();
        }
        fd::fdimp* imp_;
    };

    class closure__accept__P8sockaddrP9socklen_tQ2fd_; void accept(closure__accept__P8sockaddrP9socklen_tQ2fd_&);
    class closure__connect__PK8sockaddr9socklen_tQi_; void connect(closure__connect__PK8sockaddr9socklen_tQi_&);
    class closure__read__PvkQki_; void read(closure__read__PvkQki_&);
    class closure__read__P5ioveciQki_; void read(closure__read__P5ioveciQki_&);
    class closure__read_once__PvkQki_; void read_once(closure__read_once__PvkQki_&);
    class closure__write__PKvkQki_; void write(closure__write__PKvkQki_&);
    class closure__write__P5ioveciQki_; void write(closure__write__P5ioveciQki_&);
    class closure__write_once__PKvkQki_; void write_once(closure__write_once__PKvkQki_&);
    class closure__sendmsg__PKvkiQi_; void sendmsg(closure__sendmsg__PKvkiQi_ &);
    class closure__open__PKci6mode_tQ2fd_; static void open(closure__open__PKci6mode_tQ2fd_ &);

    fdimp* _p;

    friend bool operator==(const fd &a, const fd &b);
    friend bool operator!=(const fd &a, const fd &b);
    friend class fdref;
};

class fdref {
  public:
    typedef fd::unspecified_bool_type unspecified_bool_type;

    enum ref_type { strong = 0, weak = 1 };
    inline fdref();
    explicit inline fdref(const fd& f);
    explicit inline fdref(fd&& f);
    inline fdref(const fd& f, ref_type ref);
    inline fdref(fd&& f, ref_type ref);
    inline ~fdref();

    inline operator unspecified_bool_type() const;
    inline bool operator!() const;
    inline int fdnum() const;

    inline ssize_t read(void* buf, size_t size);
    inline ssize_t write(const void* buf, size_t size);

    inline void close();
    inline void close(int errcode);

  private:
    fd::fdimp* imp_;
    int flags_;

    fdref(const fdref&) = delete;
    fdref& operator=(const fdref&) = delete;

    friend class fd;
};

inline fd tcp_listen(int port);
fd tcp_listen(int port, int backlog);
inline void tcp_listen(int port, event<fd> result);
void tcp_listen(int port, int backlog, event<fd> result);
void tcp_connect(struct in_addr addr, int port, event<fd> result);
inline void tcp_connect(int port, event<fd> result);

void udp_connect(struct in_addr addr, int port, event<fd> result);

fd unix_stream_listen(std::string path, int backlog);
inline fd unix_stream_listen(std::string path);
void unix_stream_connect(std::string path, event<fd> result);


struct exec_fd {
    enum fdtype {
        fdtype_newin, fdtype_newout, fdtype_share, fdtype_transfer
    };
    int child_fd;
    fdtype type;
    fd f;
    inline exec_fd(int child_fd, fdtype type, fd f = fd());
};

pid_t exec(std::vector<exec_fd>& exec_fds, const char* program, bool path,
           const std::vector<const char*> &argv, char* const envp[]);
inline pid_t execv(fd& in, fd& out, const char* program,
                   const std::vector<const char*>& argv);
inline pid_t execv(fd& in, fd& out, fd& err, const char* program,
                   const std::vector<const char*>& argv);
inline pid_t execvp(fd& in, fd& out, const char* program,
                    const std::vector<const char*>& argv);
inline pid_t execvp(fd& in, fd& out, fd& err, const char* program,
                    const std::vector<const char*>& argv);


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
inline fd::fd(const fd &f)
    : _p(f._p) {
    if (_p)
        ++_p->ref_count_;
}

inline fd::fd(fd&& f)
    : _p(f._p) {
    f._p = 0;
}

/** @brief  Destroy the file descriptor wrapper.
 *  @note   The underlying file descriptor is closed if this was the last
 *          remaining wrapper.
 */
inline fd::~fd() {
    if (_p)
        _p->deref();
}

/** @brief  Assign this file descriptor to refer to @a x.
 *  @param  f  Source file descriptor.
 */
inline fd& fd::operator=(const fd& f) {
    if (f._p)
        ++f._p->ref_count_;
    if (_p)
        _p->deref();
    _p = f._p;
    return *this;
}

inline fd& fd::operator=(fd&& f) {
    if (_p)
        _p->deref();
    _p = f._p;
    f._p = 0;
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
inline void fd::open(const char* filename, int flags, event<fd> f) {
    open(filename, flags, 0777, f);
}

/** @brief  Create a pipe
 *  @param  pfd  pfd[0] is the read end, pfd[1] is the write end.
 *  @return  0 on success or a negative error code.
 *
 *  The returned file descriptors are nonblocking.
 */
inline int fd::pipe(fd pfd[2]) {
    return pipe(pfd[0], pfd[1]);
}

/** @brief  Test if file descriptor is valid.
 *  @return  True if file descriptor is valid, false if not.
 */
inline bool fd::valid() const {
    return _p && _p->fde_ >= 0;
}

/** @brief  Test if file descriptor is valid.
 *  @return  True if file descriptor is valid, false if not.
 */
inline fd::operator unspecified_bool_type() const {
    return _p && _p->fde_ >= 0 ? &fd::valid : 0;
}

/** @brief  Test if file descriptor is invalid.
 *  @return  False if file descriptor is valid, true if not.
 */
inline bool fd::operator!() const {
    return !_p || _p->fde_ < 0;
}

/** @brief  Check for file descriptor error.
 *  @return  0 if file descriptor is valid, otherwise a negative
 *          error code.
 */
inline int fd::error() const {
    return _p ? _p->fde_ : -EBADF;
}

/** @brief  Return file descriptor number.
 *  @pre    valid()
 *  @return  File descriptor number.
 */
inline int fd::fdnum() const {
    assert(_p && _p->fde_ >= 0);
    return _p->fdv_;
}

/** @brief  Return most recent file descriptor number.
 *  @return  File descriptor number.
 */
inline int fd::recent_fdnum() const {
    return _p ? _p->fdv_ : -EBADF;
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
        _p->_at_close += TAMER_MOVE(e);
    else
        e();
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

/** @brief  Shut down a socket file descriptor for reading and/or writing.
    @param  how  SHUT_RD, SHUT_WR, or SHUT_RDWR */
inline int fd::shutdown(int how) {
    if (_p && _p->fde_ >= 0)
        return ::shutdown(_p->fdv_, how);
    else
        return -EBADF;
}

/** @brief  Return any socket error state for this file descriptor. */
inline int fd::socket_error() const {
    if (_p && _p->fde_ >= 0) {
        int error = 0;
        socklen_t len = sizeof(error);
        int r = getsockopt(_p->fdv_, SOL_SOCKET, SO_ERROR, &error, &len);
        return r ? -error : 0;
    } else
        return -EBADF;
}

/** @brief  Close file descriptor, marking it with an error.
 *  @param  errcode  Optional negative error code.
 *
 *  After close(@a errcode), the valid() function will return false. If @a
 *  errcode < 0, then error() will return @a errcode.
 */
inline void fd::close(int errcode) {
    if (_p)
        _p->close(errcode);
    else if (errcode < 0 && errcode != -EBADF)
        _p = new fdimp(errcode);
}

/** @cond never */
inline void fd::error_close(int errcode) {
    close(errcode);
}
/** @endcond never */

/** @brief  Make this file descriptor use nonblocking I/O.
 */
inline int fd::make_nonblocking() {
    return make_nonblocking(_p ? _p->fde_ : -EBADF);
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


inline fdref::fdref()
    : imp_(0), flags_(0) {
}

inline fdref::fdref(const fd& f)
    : imp_(f._p), flags_(0) {
    if (imp_)
        ++imp_->ref_count_;
}

inline fdref::fdref(fd&& f)
    : imp_(f._p), flags_(0) {
    f._p = 0;
}

inline fdref::fdref(const fd& f, ref_type ref)
    : imp_(f._p), flags_(ref) {
    if (imp_ && ref == weak)
        ++imp_->weak_count_;
    else if (imp_)
        ++imp_->ref_count_;
}

inline fdref::fdref(fd&& f, ref_type ref)
    : imp_(f._p), flags_(ref) {
    f._p = 0;
    if (imp_ && ref == weak) {
        ++imp_->weak_count_;
        imp_->deref();
    }
}

inline fdref::~fdref() {
    if (imp_ && (flags_ & weak))
        imp_->weak_deref();
    else if (imp_)
        imp_->deref();
}

inline fdref::operator unspecified_bool_type() const {
    return imp_ && imp_->fde_ >= 0 ? &fd::valid : 0;
}

inline bool fdref::operator!() const {
    return !imp_ || imp_->fde_ < 0;
}

inline int fdref::fdnum() const {
    assert(imp_ && imp_->fde_ >= 0);
    return imp_->fdv_;
}

inline ssize_t fdref::read(void* buf, size_t size) {
    if (imp_ && imp_->fde_ >= 0)
        return ::read(imp_->fdv_, buf, size);
    else {
        errno = EBADF;
        return -1;
    }
}

inline ssize_t fdref::write(const void* buf, size_t size) {
    if (imp_ && imp_->fde_ >= 0)
        return ::write(imp_->fdv_, buf, size);
    else {
        errno = EBADF;
        return -1;
    }
}

inline void fdref::close() {
    if (imp_)
        imp_->close();
}

inline void fdref::close(int errcode) {
    if (imp_)
        imp_->close(errcode);
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

inline void tcp_connect(int port, event<fd> result) {
    struct in_addr in;
    in.s_addr = ntohl(INADDR_LOOPBACK);
    tcp_connect(in, port, std::move(result));
}

inline fd unix_stream_listen(std::string path) {
    return unix_stream_listen(TAMER_MOVE(path), fd::default_backlog);
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
