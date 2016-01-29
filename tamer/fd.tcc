// -*- mode: c++ -*-
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
#include "config.h"
#include <tamer/fd.hh>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <sys/resource.h>
#include <sys/un.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <tamer/tamer.hh>
#if HAVE_TAMER_FDHELPER
# include <tamer/fdh.hh>
#endif
#include <algorithm>
extern char **environ;

namespace tamer {

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

#if HAVE_TAMER_FDHELPER
static fdhelper _fdhm;
#endif

/** @brief  Make a file descriptor use nonblocking I/O.
 *  @param  f  File descriptor value.
 *  @note   This function's argument is a file descriptor value, not an
 *          fd wrapper object.  Particularly useful for e.g.
 *          standard input.
 */
int fd::make_nonblocking(int f)
{
    if (f < 0)
        return -EBADF;
    int flags = 1;
    if (::ioctl(f, FIONBIO, &flags) == 0)
        return 0;
    flags = ::fcntl(f, F_GETFL);
    if (flags != -1
        && ::fcntl(f, F_SETFL, flags | O_NONBLOCK) != -1)
        return 0;
    return -errno;
}

/** @brief  Make a file descriptor use blocking I/O.
 *  @param  f  File descriptor value.
 *  @note   This function's argument is a file descriptor value, not an
 *          fd wrapper object.
 */
int fd::make_blocking(int f) {
    if (f < 0)
        return -EBADF;
    int flags = 0;
    if (::ioctl(f, FIONBIO, &flags) == 0)
        return 0;
    flags = ::fcntl(f, F_GETFL);
    if (flags != -1
        && ::fcntl(f, F_SETFL, flags & ~O_NONBLOCK) != -1)
        return 0;
    return -errno;
}

/** @brief  Return a closer event.
 *  @return  Closer event.
 *
 *  Triggering the returned event will immediately close the file descriptor.
 *  Returns an empty event if the file descriptor is invalid.
 */
event<> fd::closer()
{
    if (*this) {
        event<> e = fun_event(fdcloser(_p));
        at_close(e);
        return e;
    } else
        return event<>();
}

/** @brief  Open a file descriptor.
 *  @param  filename  File name.
 *  @param  flags     Open flags (@c O_RDONLY, @c O_EXCL, and so forth).
 *  @param  mode      Permissions mode (used when creating a file).
 *  @return File descriptor.
 *
 *  Opens and returns a file descriptor for the named file.  The returned
 *  file descriptor is <em>not</em> made nonblocking by default, but you
 *  may add @c O_NONBLOCK to @a flags yourself.  To check whether the open
 *  succeeded, use valid() or error() on the resulting file descriptor.
 *
 *  @sa open(const char *, int, event<fd>)
 */
fd fd::open(const char *filename, int flags, mode_t mode) {
    int f = ::open(filename, flags, mode);
    return fd(f == -1 ? -errno : f);
}

#if HAVE_TAMER_FDHELPER
tamed static void fd::open(const char *filename, int flags, mode_t mode, event<fd> done)
{
    tvars { int f(); fd nfd; }
    twait { _fdhm.open(filename, flags | O_NONBLOCK, mode, make_event(f)); }
    nfd = fd(f);
    nfd._p->_is_file = true;
    done.trigger(nfd);
}
#else
/** @brief  Open a file descriptor.
 *  @param  filename  File name.
 *  @param  flags     Open flags (@c O_RDONLY, @c O_EXCL, and so forth).
 *  @param  mode      Permissions mode (used when creating a file).
 *  @param  result    Event triggered on completion.
 *
 *  Opens a file descriptor for the named file, returning it via the @a result
 *  event. The returned file descriptor is made nonblocking. To check whether
 *  the open succeeded, use valid() or error() on the resulting file
 *  descriptor.
 *
 *  @sa open(const char *, int, event<fd>)
 */
void fd::open(const char *filename, int flags, mode_t mode, event<fd> done)
{
    fd nfd;
    int f = ::open(filename, flags | O_NONBLOCK, mode);
    nfd = fd(f == -1 ? -errno : f);
    done.trigger(nfd);
}
#endif

/** @brief  Create a pipe.
 *  @param  rfd  Set to file descriptor for read end of pipe.
 *  @param  wfd  Set to file descriptor for write end of pipe.
 *  @return  0 on success or a negative error code.
 *
 *  The returned file descriptors are nonblocking.
 */
int fd::pipe(fd& rfd, fd& wfd)
{
    int pfd[2], r = ::pipe(pfd);
    if (r < 0) {
        rfd = wfd = fd(-errno);
        return -errno;
    } else {
        (void) make_nonblocking(pfd[0]);
        (void) make_nonblocking(pfd[1]);
        rfd = fd(pfd[0]);
        wfd = fd(pfd[1]);
        return 0;
    }
}

/** @brief  Fetch file status.
 *  @param[out]  stat  Status structure.
 *  @param       done  Event triggered on completion.
 *
 *  @a done is triggered with 0 on success, or a negative error code.
 */
void fd::fstat(struct stat &stat_out, event<int> done)
{
    fdimp *fi = _p;
    if (fi && fi->fde_ >= 0) {
#if HAVE_TAMER_FDHELPER
        _fdhm.fstat(fi->fdv_, stat_out, done);
#else
        int x = ::fstat(fi->fdv_, &stat_out);
        done.trigger(x == -1 ? -errno : 0);
#endif
    } else
        done.trigger(-EBADF);
}

tamed void fd::read(void *buf, size_t size, size_t* nread_ptr, event<int> done)
{
    tvars {
        size_t pos = 0;
        ssize_t amt;
        fdref fi(*this, fdref::weak);
    }

    if (nread_ptr)
        *nread_ptr = 0;

    if (!fi) {
        done.trigger(-EBADF);
        return;
    }

#if HAVE_TAMER_FDHELPER
    if (fi.imp_->_is_file) {
        _fdhm.read(fi.fdnum(), buf, size, nread, done);
        return;
    }
#endif

    twait { fi.acquire_read(make_event()); }

    while (pos != size && done && fi) {
        amt = fi.read(static_cast<char*>(buf) + pos, size - pos);
        if (amt != 0 && amt != (ssize_t) -1) {
            pos += amt;
            if (nread_ptr)
                *nread_ptr = pos;
        } else if (amt == 0)
            break;
        else if (errno == EAGAIN || errno == EWOULDBLOCK) {
            twait { tamer::at_fd_read(fi.fdnum(), make_event()); }
        } else if (errno != EINTR) {
            done.trigger(-errno);
            break;
        }
    }

    done.trigger(pos == size || fi ? 0 : -ECANCELED);
}

tamed void fd::read(struct iovec* iov, int iov_count, size_t* nread_ptr, event<int> done)
{
    tvars {
        size_t pos = 0;
        size_t size = 0;
        ssize_t amt;
        fdref fi(*this, fdref::weak);
    }

    if (nread_ptr)
        *nread_ptr = 0;

    if (!fi) {
        done.trigger(-EBADF);
        return;
    }

    for (int i = 0; i != iov_count; ++i)
        size += iov[i].iov_len;

#if HAVE_TAMER_FDHELPER
    if (fi.imp_->_is_file) {
        _fdhm.read(fi.fdnum(), buf, size, nread, done);
        return;
    }
#endif

    twait { fi.acquire_read(make_event()); }

    while (pos != size && done && fi) {
        amt = ::readv(fi.fdnum(), iov, iov_count);
        if (amt != 0 && amt != (ssize_t) -1) {
            pos += amt;
            if (nread_ptr)
                *nread_ptr = pos;
            if (pos != size) {
                while (amt != 0 && (size_t) amt >= iov[0].iov_len) {
                    amt -= iov[0].iov_len;
                    ++iov, --iov_count;
                }
                if (amt != 0) {
                    iov[0].iov_base = (char*) iov[0].iov_base + amt;
                    iov[0].iov_len -= amt;
                }
            }
        } else if (amt == 0)
            break;
        else if (errno == EAGAIN || errno == EWOULDBLOCK) {
            twait { tamer::at_fd_read(fi.fdnum(), make_event()); }
        } else if (errno != EINTR) {
            done.trigger(-errno);
            break;
        }
    }

    done.trigger(pos == size || fi ? 0 : -ECANCELED);
}

tamed void fd::read_once(void* buf, size_t size, size_t& nread, event<int> done)
{
    tvars {
        ssize_t amt;
        fdref fi(*this, fdref::weak);
    }

    nread = 0;

    if (!fi) {
        done.trigger(-EBADF);
        return;
    }

    twait { fi.acquire_read(make_event()); }

    while (done && fi) {
        amt = fi.read(static_cast<char*>(buf), size);
        if (amt != (ssize_t) -1) {
            nread = amt;
            break;
        } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
            twait { tamer::at_fd_read(fi.fdnum(), make_event()); }
        } else if (errno != EINTR) {
            done.trigger(-errno);
            break;
        }
    }

    done.trigger(0);
}

tamed void fd::read_once(const struct iovec* iov, int iov_count, size_t& nread, event<int> done)
{
    tvars {
        ssize_t amt;
        fdref fi(*this, fdref::weak);
    }

    nread = 0;

    if (!fi) {
        done.trigger(-EBADF);
        return;
    }

    twait { fi.acquire_read(make_event()); }

    while (done && fi) {
        amt = ::readv(fi.fdnum(), iov, iov_count);
        if (amt != (ssize_t) -1) {
            nread = amt;
            break;
        } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
            twait { tamer::at_fd_read(fi.fdnum(), make_event()); }
        } else if (errno != EINTR) {
            done.trigger(-errno);
            break;
        }
    }

    done.trigger(0);
}

tamed void fd::write(const void* buf, size_t size, size_t* nwritten_ptr,
                     event<int> done)
{
    tvars {
        size_t pos = 0;
        ssize_t amt;
        fdref fi(*this, fdref::weak);
    }

    if (nwritten_ptr)
        *nwritten_ptr = 0;

    if (!fi) {
        done.trigger(-EBADF);
        return;
    }

#if HAVE_TAMER_FDHELPER
    if (fi.imp_->_is_file) {
        _fdhm.write(fi.fdnum(), buf, size, nwritten, done);
        return;
    }
#endif

    twait { fi.acquire_write(make_event()); }

    while (pos != size && done && fi) {
        amt = fi.write(static_cast<const char*>(buf) + pos, size - pos);
        if (amt != 0 && amt != (ssize_t) -1) {
            pos += amt;
            if (nwritten_ptr)
                *nwritten_ptr = pos;
        } else if (amt == 0)
            break;
        else if (errno == EAGAIN || errno == EWOULDBLOCK) {
            twait { tamer::at_fd_write(fi.fdnum(), make_event()); }
        } else if (errno != EINTR) {
            done.trigger(-errno);
            break;
        }
    }

    done.trigger(pos == size || fi ? 0 : -ECANCELED);
}

tamed void fd::write(std::string s, size_t* nwritten_ptr, event<int> done)
{
    twait { // This twait block prevents s from being destroyed.
        done.at_trigger(make_event());
        write(s.data(), s.length(), nwritten_ptr, done);
    }
}

tamed void fd::write(struct iovec* iov, int iov_count, size_t* nwritten_ptr,
                     event<int> done)
{
    tvars {
        size_t pos = 0;
        size_t size = 0;
        ssize_t amt;
        fdref fi(*this, fdref::weak);
    }

    if (nwritten_ptr)
        *nwritten_ptr = 0;

    if (!fi) {
        done.trigger(-EBADF);
        return;
    }

#if HAVE_TAMER_FDHELPER
    if (fi.imp_->_is_file) {
        _fdhm.write(fi.fdnum(), buf, size, nwritten, done);
        return;
    }
#endif

    for (int i = 0; i != iov_count; ++i)
        size += iov[i].iov_len;

    twait { fi.acquire_write(make_event()); }

    while (pos != size && done && fi) {
        amt = ::writev(fi.fdnum(), iov, iov_count);
        if (amt != 0 && amt != (ssize_t) -1) {
            pos += amt;
            if (nwritten_ptr)
                *nwritten_ptr = pos;
            if (pos != size) {
                while (amt != 0 && (size_t) amt >= iov[0].iov_len) {
                    amt -= iov[0].iov_len;
                    ++iov, --iov_count;
                }
                if (amt != 0) {
                    iov[0].iov_base = (char*) iov[0].iov_base + amt;
                    iov[0].iov_len -= amt;
                }
            }
        } else if (amt == 0)
            break;
        else if (errno == EAGAIN || errno == EWOULDBLOCK) {
            twait { tamer::at_fd_write(fi.fdnum(), make_event()); }
        } else if (errno != EINTR) {
            done.trigger(-errno);
            break;
        }
    }

    done.trigger(pos == size || fi ? 0 : -ECANCELED);
}

/** @brief  Write once to file descriptor.
 *  @param       buf       Buffer.
 *  @param       size      Buffer size.
 *  @param[out]  nwritten  Number of characters written.
 *  @param       done      Event triggered on completion.
 *
 *  Writes at most @a size bytes to the file descriptor.  Blocks until at
 *  least one byte is written (or end-of-file or an error condition), but
 *  unlike write(), @a done is triggered after the @em first successful
 *  read, even if less than @a size bytes are written.  @a done is
 *  triggered with 0 on success, or a negative error code.  @a nwritten is
 *  kept up to date as the read progresses.
 *
 *  @sa write(const void *, size_t, size_t &, event<int>)
 */
tamed void fd::write_once(const void *buf, size_t size, size_t &nwritten, event<int> done)
{
    tvars {
        ssize_t amt;
        fdref fi(*this, fdref::weak);
    }

    nwritten = 0;

    if (!fi) {
        done.trigger(-EBADF);
        return;
    }

    twait { fi.acquire_write(make_event()); }

    while (done && fi) {
        amt = fi.write(static_cast<const char*>(buf), size);
        if (amt != (ssize_t) -1) {
            nwritten = amt;
            break;
        } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
            twait { tamer::at_fd_write(fi.fdnum(), make_event()); }
        } else if (errno != EINTR) {
            done.trigger(-errno);
            break;
        }
    }

    done.trigger(0);
}

/** @brief  Write once to file descriptor.
 *  @param       iov         I/O vector.
 *  @param       iov_count   Number of elements in I/O vector.
 *  @param[out]  nwritten    Number of characters written.
 *  @param       done        Event triggered on completion.
 *
 *  Writes at most @a size bytes to the file descriptor. Blocks until at
 *  least one byte is written (or end-of-file or an error condition), but
 *  unlike write(), @a done is triggered after the @em first successful
 *  read, even if less than @a size bytes are written. @a done is
 *  triggered with 0 on success, or a negative error code. @a nwritten is
 *  kept up to date as the read progresses. Unlike write(), @a iov is never
 *  modified.
 *
 *  @sa write(const void *, size_t, size_t &, event<int>)
 */
tamed void fd::write_once(const struct iovec* iov, int iov_count, size_t& nwritten, event<int> done)
{
    tvars {
        ssize_t amt;
        fdref fi(*this, fdref::weak);
    }

    nwritten = 0;

    if (!fi) {
        done.trigger(-EBADF);
        return;
    }

    twait { fi.acquire_write(make_event()); }

    while (done && fi) {
        amt = ::writev(fi.fdnum(), iov, iov_count);
        if (amt != (ssize_t) -1) {
            nwritten = amt;
            break;
        } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
            twait { tamer::at_fd_write(fi.fdnum(), make_event()); }
        } else if (errno != EINTR) {
            done.trigger(-errno);
            break;
        }
    }

    done.trigger(0);
}

ssize_t fd::direct_read(void* buf, size_t size) {
    fdimp* fi = _p;
    if (fi && fi->fde_ >= 0)
        return ::read(fi->fdv_, buf, size);
    else {
        errno = EBADF;
        return (ssize_t) -1;
    }
}

ssize_t fd::direct_write(const void* buf, size_t size) {
    fdimp* fi = _p;
    if (fi && fi->fde_ >= 0)
        return ::write(fi->fdv_, buf, size);
    else {
        errno = EBADF;
        return (ssize_t) -1;
    }
}

/** @brief  Send a message on a file descriptor.
 *  @param  buf          Buffer.
 *  @param  size         Buffer size.
 *  @param  transfer_fd  File descriptor to send with the message.
 *  @param  done         Event triggered on completion.
 */
tamed void fd::sendmsg(const void *buf, size_t size, int transfer_fd,
                       event<int> done)
{
    tvars {
        struct msghdr msg;
        struct iovec iov;
        char transfer_fd_space[64];
        ssize_t amt;
        fdref fi(*this, fdref::weak);
    }

    if (!fi) {
        done.trigger(-EBADF);
        return;
    }

    // prepare message
    msg.msg_name = 0;
    msg.msg_namelen = 0;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_flags = 0;
    iov.iov_base = const_cast<void *>(buf);
    iov.iov_len = size;

    if (transfer_fd < 0) {
        msg.msg_control = 0;
        msg.msg_controllen = 0;
    } else {
        msg.msg_control = transfer_fd_space;
        msg.msg_controllen = CMSG_SPACE(sizeof(int));
        struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_RIGHTS;
        cmsg->cmsg_len = CMSG_LEN(sizeof(int));
        memcpy(CMSG_DATA(cmsg), &transfer_fd, sizeof(int));
    }

    // send message
    while (done && fi) {
        amt = ::sendmsg(fi.fdnum(), &msg, 0);
        if (amt != (ssize_t) -1)
            break;
        else if (errno == EAGAIN || errno == EWOULDBLOCK) {
            twait { tamer::at_fd_write(fi.fdnum(), make_event()); }
        } else if (errno != EINTR)
            done.trigger(-errno);
    }

    done.trigger(fi ? 0 : -ECANCELED);
}

/** @brief  Create a socket file descriptor.
 *  @param  domain    Socket domain.
 *  @param  type      Socket type.
 *  @param  protocol  Socket protocol.
 *
 *  The returned socket is nonblocking.  Use valid() or error() to check
 *  for errors.
 */
fd fd::socket(int domain, int type, int protocol)
{
    int f = ::socket(domain, type, protocol);
    if (f >= 0)
        make_nonblocking(f);
    return fd(f == -1 ? -errno : f);
}

/** @brief  Set socket file descriptor for listening.
 *  @param  backlog  Maximum length of pending connection queue.
 *
 *  Returns 0 on success, or a negative error code.
 *
 *  @sa tamer::tcp_listen
 */
int fd::listen(int backlog)
{
    if (*this)
        return (::listen(_p->fdv_, backlog) == 0 ? 0 : -errno);
    else
        return -EBADF;
}

/** @brief  Bind socket file descriptor to local address.
 *  @param  addr     Local socket address.
 *  @param  addrlen  Length of local socket address.
 *
 *  Returns 0 on success, or a negative error code.
 */
int fd::bind(const struct sockaddr *addr, socklen_t addrlen)
{
    if (*this)
        return (::bind(_p->fdv_, addr, addrlen) == 0 ? 0 : -errno);
    else
        return -EBADF;
}

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
tamed void fd::accept(struct sockaddr *addr_out, socklen_t *addrlen_out,
                      event<fd> done)
{
    tvars {
        int f = -ECANCELED;
        fdref fi(*this, fdref::weak);
    }

    if (!fi) {
        done.trigger(fd());
        return;
    }

    twait { fi.acquire_read(make_event()); }

    while (done && fi) {
        f = ::accept(fi.fdnum(), addr_out, addrlen_out);
        if (f >= 0) {
            make_nonblocking(f);
            break;
        } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
            twait { tamer::at_fd_read(fi.fdnum(), make_event()); }
        } else if (errno != EINTR) {
            f = -errno;
            break;
        }
    }

    done.trigger(fd(f));
}

/** @brief  Connect socket file descriptor.
 *  @param  addr     Remote address.
 *  @param  addrlen  Length of remote address.
 *  @param  done     Event triggered on completion.
 *
 *  @a done is triggered with 0 on success, or a negative error code.
 *
 *  @sa tamer::tcp_connect
 */
tamed void fd::connect(const struct sockaddr *addr, socklen_t addrlen,
                       event<int> done)
{
    tvars {
        int x, ret(0);
        fdref fi(*this, fdref::weak);
    }

    if (!fi) {
        done.trigger(-EBADF);
        return;
    }

    twait { fi.acquire_write(make_event()); }

    x = ::connect(fi.fdnum(), addr, addrlen);
    if (x == -1 && errno != EINPROGRESS)
        ret = -errno;
    else if (x == -1) {
        twait { tamer::at_fd_write(fi.fdnum(), make_event()); }
        socklen_t socklen = sizeof(x);
        if (!done || fi.fdnum() < 0)
            ret = -ECANCELED;
        else if (getsockopt(fi.fdnum(), SOL_SOCKET, SO_ERROR,
                            (void *) &x, &socklen) == -1)
            ret = -errno;
        else if (x != 0)
            ret = -x;
    }

    done.trigger(ret);
}


/** @brief  Close file descriptor.
 *
 *  Equivalent to close(event<int>()).
 */
int fd::fdimp::close(int leave_error) {
    int my_fd = fde_ < 0 ? fde_ : fdv_;
    if (leave_error >= 0)
        leave_error = -EBADF;
    if (my_fd >= 0 || leave_error != -EBADF)
        fde_ = leave_error;
    if (my_fd >= 0) {
        int x = ::close(my_fd);
        if (x == -1) {
            x = -errno;
            if (fde_ == -EBADF)
                fde_ = x;
        }
        if (driver::main)
            driver::main->kill_fd(my_fd);
        _at_close.trigger();
    }
    return fde_ < 0 ? fde_ : fdv_;
}


/** @brief Return the current limit on the number of open files for this
    process.
    @return The limit, or a negative error code. */
int fd::open_limit() {
    struct rlimit rl;
    int r = getrlimit(RLIMIT_NOFILE, &rl);
    return r == 0 ? rl.rlim_cur : -errno;
}


/** @brief Set the limit on the number of open files for this process.
    @return The new limit, or a negative error code.

    The returned value might be less than @a n. */
int fd::open_limit(int n) {
    struct rlimit rl;
    int r = getrlimit(RLIMIT_NOFILE, &rl);
    if (r == 0) {
        if (rl.rlim_max < (rlim_t) n && geteuid() == 0)
            rl.rlim_max = n;
        if (rl.rlim_max < (rlim_t) n)
            rl.rlim_cur = rl.rlim_max;
        else
            rl.rlim_cur = n;
        r = setrlimit(RLIMIT_NOFILE, &rl);
        if (r == 0)
            r = getrlimit(RLIMIT_NOFILE, &rl);
    }
    return r == 0 ? rl.rlim_cur : -errno;
}


/** @brief  Open a TCP listening socket receiving connections to @a port.
 *  @param  port     Listening port (in host byte order).
 *  @param  backlog  Maximum connection backlog.
 *  @return File descriptor.
 *
 *  The returned file descriptor is made nonblocking, and is opened with the
 *  @c SO_REUSEADDR option. A negative value is returned on error. To check
 *  whether the function succeeded, use valid() or error() on the resulting
 *  file descriptor.
 */
fd tcp_listen(int port, int backlog)
{
    fd f = fd::socket(AF_INET, SOCK_STREAM, 0);
    if (f) {
        // Default to reusing port addresses.  Don't worry if it fails
        int yes = 1;
        (void) setsockopt(f.fdnum(), SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        struct sockaddr_in saddr;
        saddr.sin_family = AF_INET;
        saddr.sin_port = htons(port);
        saddr.sin_addr.s_addr = INADDR_ANY;
        int ret = f.bind((struct sockaddr *) &saddr, sizeof(saddr));
        if (ret >= 0)
            ret = f.listen(backlog);
        if (ret < 0 && f)
            f.close(ret);
    }
    return f;
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
tamed void tcp_connect(struct in_addr addr, int port, event<fd> result)
{
    tvars {
        fd f = fd::socket(AF_INET, SOCK_STREAM, 0);
        int ret = 0;
        struct sockaddr_in saddr;
    }
    if (f)
        twait {
            memset(&saddr, 0, sizeof(saddr));
            saddr.sin_family = AF_INET;
            saddr.sin_addr = addr;
            saddr.sin_port = htons(port);
            f.connect((struct sockaddr *) &saddr, sizeof(saddr), make_event(ret));
        }
    if (ret < 0 && f)
        f.close(ret);
    result.trigger(f);
}

tamed void udp_connect(struct in_addr addr, int port, event<fd> result) {
    tvars {
        fd f = fd::socket(AF_INET, SOCK_DGRAM, 0);
        int ret = 0;
        struct sockaddr_in saddr;
    }
    if (f)
        twait {
            memset(&saddr, 0, sizeof(saddr));
            saddr.sin_family = AF_INET;
            saddr.sin_addr = addr;
            saddr.sin_port = htons(port);
            f.connect((struct sockaddr *) &saddr, sizeof(saddr), make_event(ret));
        }
    if (ret < 0 && f)
        f.close(ret);
    result.trigger(f);
}


/** @brief  Open a nonblocking TCP connection on port @a port.
 *  @param  port     Listening port (in host byte order).
 *  @param  backlog  Maximum connection backlog.
 *  @return File descriptor.
 *
 *  The returned file descriptor is made nonblocking, and is opened with the
 *  @c SO_REUSEADDR option. A negative value is returned on error. To check
 *  whether the function succeeded, use valid() or error() on the resulting
 *  file descriptor.
 */
fd unix_stream_listen(std::string path, int backlog)
{
    struct sockaddr_un saddr;
    if (path.length() >= sizeof(saddr.sun_path))
        return fd(-ENAMETOOLONG);
    fd f = fd::socket(AF_UNIX, SOCK_STREAM, 0);
    if (f) {
        saddr.sun_family = AF_UNIX;
        memcpy(saddr.sun_path, path.data(), path.length());
        saddr.sun_path[path.length()] = 0;
        int ret = f.bind((struct sockaddr *) &saddr, sizeof(saddr));
        if (ret >= 0)
            ret = f.listen(backlog);
        if (ret < 0 && f)
            f.close(ret);
    }
    return f;
}

tamed void unix_stream_connect(std::string path, event<fd> result) {
    tvars {
        fd f;
        int ret = 0;
        struct sockaddr_un saddr;
    }
    if (path.length() < sizeof(saddr.sun_path))
        f = fd::socket(AF_UNIX, SOCK_STREAM, 0);
    else
        f = fd(-ENAMETOOLONG);
    if (f)
        twait {
            saddr.sun_family = AF_UNIX;
            memcpy(saddr.sun_path, path.data(), path.length());
            saddr.sun_path[path.length()] = '\0';
            f.connect((struct sockaddr *) &saddr, sizeof(saddr), make_event(ret));
        }
    if (ret < 0 && f)
        f.close(ret);
    result.trigger(f);
}


static int kill_exec_fds(std::vector<exec_fd>& exec_fds,
                         std::vector<int>& inner_fds, int error) {
    assert(error < 0);
    for (std::vector<exec_fd>::size_type i = 0; i != exec_fds.size(); ++i) {
        exec_fds[i].f = fd(error);
        if ((exec_fds[i].type == exec_fd::fdtype_newin
             || exec_fds[i].type == exec_fd::fdtype_newout)
            && inner_fds[i] >= 0)
            close(inner_fds[i]);
    }
    return error;
}

pid_t exec(std::vector<exec_fd>& exec_fds, const char* program, bool path,
           const std::vector<const char*>& argv, char* const envp[])
{
    std::vector<int> inner_fds(exec_fds.size(), -1);
    int r, pfd[2];

    // open pipes
    for (std::vector<exec_fd>::size_type i = 0; i != exec_fds.size(); ++i)
        if (exec_fds[i].type == exec_fd::fdtype_newin
            || exec_fds[i].type == exec_fd::fdtype_newout) {
            if ((r = ::pipe(pfd)) < 0)
                return kill_exec_fds(exec_fds, inner_fds, -errno);
            bool isoutput = (exec_fds[i].type == exec_fd::fdtype_newout);
            exec_fds[i].f = fd(pfd[!isoutput]);
            (void) fd::make_nonblocking(exec_fds[i].f.fdnum());
            inner_fds[i] = pfd[isoutput];
        } else
            inner_fds[i] = exec_fds[i].f.fdnum();

    // create child
    pid_t child = fork();
    if (child < 0)
        return kill_exec_fds(exec_fds, inner_fds, -errno);
    else if (child == 0) {
        if (exec_fds.size() > 0) {
            // close parent's descriptors
            for (std::vector<exec_fd>::iterator it = exec_fds.begin();
                 it != exec_fds.end(); ++it)
                if (it->type == exec_fd::fdtype_newin
                    || it->type == exec_fd::fdtype_newout)
                    it->f.close();

            // duplicate our descriptors into their proper places
            for (std::vector<exec_fd>::size_type i = 0;
                 i != exec_fds.size(); ++i) {
                if (exec_fds[i].child_fd == inner_fds[i])
                    continue;
                std::vector<int>::iterator xt =
                    std::find(inner_fds.begin() + i, inner_fds.end(),
                              exec_fds[i].child_fd);
                if (xt != inner_fds.end())
                    if ((*xt = ::dup(*xt)) < 0)
                        exit(1);
                if (inner_fds[i] >= 0) {
                    if (::dup2(inner_fds[i], exec_fds[i].child_fd) < 0)
                        exit(1);
                    (void) ::close(inner_fds[i]);
                } else
                    (void) ::close(exec_fds[i].child_fd);
            }
        }

        // create true argv array
        char **xargv = new char *[argv.size() + 1];
        if (!xargv)
            abort();
        for (std::vector<const char *>::size_type i = 0; i != argv.size(); ++i)
            xargv[i] = (char *) argv[i];
        xargv[argv.size()] = 0;

        // execute
        if (!path)
            r = ::execve(program, (char * const *) xargv,
                         envp ? envp : environ);
        else
            r = ::execvp(program, (char * const *) xargv);
        exit(1);
    }

    // close relevant descriptors and return
    for (std::vector<exec_fd>::size_type i = 0; i != exec_fds.size(); ++i)
        if (exec_fds[i].type == exec_fd::fdtype_newin
            || exec_fds[i].type == exec_fd::fdtype_newout) {
            ::close(inner_fds[i]);
            (void) ::fcntl(exec_fds[i].f.fdnum(), F_SETFD, FD_CLOEXEC);
        } else if (exec_fds[i].type == exec_fd::fdtype_transfer)
            exec_fds[i].f.close();

    return child;
}

} // namespace tamer
